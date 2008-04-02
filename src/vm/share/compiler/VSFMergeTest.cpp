/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include "incls/_precompiled.incl"
#include "incls/_VSFMergeTest.cpp.incl"

#ifndef PRODUCT
#if ENABLE_COMPILER && ENABLE_VSF_MERGE_TEST

#if ENABLE_PERFORMANCE_COUNTERS
void VSFMergeTest::run(OsFile_Handle config_file) {
  SETUP_ERROR_CHECKER_ARG;

  AccessFlags access_flags;
  access_flags.set_flags(0);
  Method method = Universe::new_method(0, access_flags JVM_CHECK);
  method.set_max_locals(MAX_TEST_LOCATION_COUNT);

  Compiler test_compiler(&method, -1);

  GUARANTEE(Compiler::current() != NULL, "Sanity");

  Compiler * const compiler = Compiler::current();

  CompiledMethod compiled_method = 
    Universe::new_compiled_method(MAX_VSF_MERGE_CODE_SIZE JVM_NO_CHECK);

  compiler->set_current_compiled_method(&compiled_method);

  CodeGenerator code_generator( &compiled_method );
  compiler->set_code_generator( &code_generator  );
  execute_test_cases(config_file JVM_NO_CHECK_AT_BOTTOM);
}

void VSFMergeTest::execute_test_cases(OsFile_Handle config_file JVM_TRAPS) {
  static MergeTestCase test_case;

  while (read_test_case(config_file, &test_case)) {
    execute_test_case(&test_case JVM_CHECK);
  }
}

bool VSFMergeTest::read_test_case(OsFile_Handle file, 
                                  MergeTestCase * const test_case) {
  enum {
    BUFFER_SIZE = 256
  };

  char read_buffer[BUFFER_SIZE];

  enum { 
    FIRST_STATE = 0,
    LOCATION_COUNT = FIRST_STATE,    
    TYPE,
    SRC_STATUS,
    DST_STATUS,
    SRC_WHERE,
    DST_WHERE,
    SRC_VALUE,
    DST_VALUE,
    LAST_STATE = DST_VALUE,
    STATE_COUNT
  };

  int parse_state = FIRST_STATE;

  static const char * const end_of_suite_marker = "VSF_MERGE_SUITE_END";

  static const char * const keywords[STATE_COUNT] = {
    "Location_count", "Types",
    "Source_status", "Target_status",
    "Source_where", "Target_where",
    "Source_value", "Target_value"
  };

  const int TYPE_COUNT = 2;

  static const char * const type_names[TYPE_COUNT] = {
    "int", "long"
  };

  static const BasicType types[TYPE_COUNT] = {
    T_INT, T_LONG
  };

  static const char * const status_names[] = {
    "flushed", "cached", "changed"
  };

  static const char * const where_names[] = {
    "nowhere", "immediate", "register"
  };

  unsigned int location_count = 0;
  unsigned int index = 0;
  int value = 0;

  VSFMergeTest::initialize();

  while (true) {
    const int bytes_read = 
      CompilerTest::read_line(file, read_buffer, BUFFER_SIZE);
    if (bytes_read == 0) {
      // End-of-file.
      break;
    }

    if (bytes_read == BUFFER_SIZE) {
      tty->print_cr("Line too long");
      break;
    }

    if (read_buffer[0] == '#') {
      continue;
    }

    char buffer[BUFFER_SIZE];
    const char * p = read_buffer;

    // Read the first word to the buffer.
    if (jvm_sscanf(p, "%255s", buffer) != 1) {
      break;
    }

    // Check for the end of suite marker.
    if (parse_state == FIRST_STATE) {
      if (jvm_strcmp(buffer, end_of_suite_marker) == 0) {
        break;
      }
    }

    if (jvm_strcmp(buffer, keywords[parse_state]) != 0) {
      tty->print_cr("Unexpected keyword: %s", buffer);
      break;
    }

    switch (parse_state) {
    case LOCATION_COUNT:
      p = next_word(p);

      if (jvm_sscanf(p, "%d", &location_count) != 1) {
        tty->print_cr("Cannot read location count");
        return false;        
      }
      if (location_count > MAX_TEST_LOCATION_COUNT) {
        tty->print_cr("Too many locations %d > %d", location_count,
                      MAX_TEST_LOCATION_COUNT);
        return false;        
      }
      test_case->location_count = location_count;
      break;
    case TYPE:
      for (index = 0; index < location_count; index++) {
        p = next_word(p);

        if (jvm_sscanf(p, "%255s", buffer) != 1) {
          tty->print_cr("Cannot read type for location %d", index);
          return false;        
        }

        value = CompilerTest::get_keyword_index(type_names, 
                                                ARRAY_SIZE(type_names), 
                                                buffer);
     
        if (value < 0 || value >= TYPE_COUNT) {
          tty->print_cr("Invalid type \"%s\" for location %d", buffer, index);
          return false;        
        }

        BasicType type = types[value];
        // For two-word types the type of the second location must be T_ILLEGAL.
        if (type == T_LONG) {
          if (index > 0 && test_case->types[index - 1] == T_LONG) {
            type = T_ILLEGAL;
          }
        }
        test_case->types[index] = type;
      }
      break;
    case SRC_STATUS: 
    case DST_STATUS: 
    {
      const bool is_source = parse_state == SRC_STATUS;
      const char * const frame_name = is_source ? "source" : "target";
      for (index = 0; index < location_count; index++) {
        p = next_word(p);

        if (jvm_sscanf(p, "%255s", buffer) != 1) {
          tty->print_cr("Cannot read status for %s location %d", 
                        frame_name, index);
          return false;        
        }

        value = CompilerTest::get_keyword_index(status_names, 
                                                ARRAY_SIZE(status_names), 
                                                buffer);
     
        if (value == -1) {
          tty->print_cr("Invalid status \"%s\" for %s location %d", 
                        buffer, frame_name, index);
          return false;        
        }
        if (is_source) {
          test_case->src_status[index] = (RawLocation::Status)value;
        } else {
          test_case->dst_status[index] = (RawLocation::Status)value;
        }
      }
      break;
    }
    case SRC_WHERE: 
    case DST_WHERE: 
    {
      const bool is_source = parse_state == SRC_WHERE;
      const char * const frame_name = is_source ? "source" : "target";
      for (index = 0; index < location_count; index++) {
        p = next_word(p);

        if (jvm_sscanf(p, "%255s", buffer) != 1) {
          tty->print_cr("Cannot read where for %s location %d", 
                        frame_name, index);
          return false;        
        }     

        value = CompilerTest::get_keyword_index(where_names, 
                                                ARRAY_SIZE(where_names), 
                                                buffer);

        if (value == -1) {
          tty->print_cr("Invalid where \"%s\" for %s location %d", 
                        buffer, frame_name, index);
          return false;        
        }
        if (is_source) {
          test_case->src_where[index] = (Value::ValueLocation)value;
        } else {
          test_case->dst_where[index] = (Value::ValueLocation)value;
        }
      }
      break;
    }
    case SRC_VALUE: 
    case DST_VALUE: 
    {
      const bool is_source = parse_state == SRC_VALUE;
      const char * const frame_name = is_source ? "source" : "target";

      for (index = 0; index < location_count; index++) {
        p = next_word(p);

        if (jvm_sscanf(p, "%d", &value) != 1) {
          tty->print_cr("Cannot read value for %s location %d", 
                        frame_name, index);
          return false;        
        }     

        const Value::ValueLocation where = 
          is_source ? test_case->src_where[index] : test_case->dst_where[index];
        int location_value = value;

        if (where == Value::T_REGISTER) {
          if (!is_valid_register(value)) {
            tty->print_cr("Invalid int register %d for %s location %d", 
                          value, frame_name, index);
            return false;        
          }

          location_value = VSFMergeTest::available_int_register(value);
        }

        if (is_source) {
          test_case->src_value[index] = location_value;
        } else {
          test_case->dst_value[index] = location_value;
        }
      }
      break;
    }
    }
    
    if (parse_state == LAST_STATE) {
      return true;
    }

    parse_state++;
  }

  return false;
}

const char * VSFMergeTest::next_word(const char * p) {
  while (*p && !isspace(*p)) {
    p++;
  }

  while (*p && isspace(*p)) {
    p++;
  }

  return p;
}

void VSFMergeTest::execute_test_case(const MergeTestCase * const test_case 
                                     JVM_TRAPS) {
  VirtualStackFrame::set_location_map_size(
    Location::size() * test_case->location_count );

  VirtualStackFrame* src_frame = 
    VirtualStackFrame::allocate( JVM_SINGLE_ARG_ZCHECK(src_frame) );
  VirtualStackFrame* dst_frame = 
    VirtualStackFrame::allocate( JVM_SINGLE_ARG_ZCHECK(src_frame) );

  src_frame->set_virtual_stack_pointer(test_case->location_count - 1);
  dst_frame->set_virtual_stack_pointer(test_case->location_count - 1);

  {
    RawLocation* src = src_frame->raw_location_at(0);
    RawLocation* end = src_frame->raw_location_end(src);
    RawLocation* dst = dst_frame->raw_location_at(0);
    unsigned int index = 0;

    while (src < end) {
      GUARANTEE(index < test_case->location_count, "Sanity");

      initialize_location(src, test_case->types[index], 
                          test_case->src_status[index], 
                          test_case->src_where[index], 
                          test_case->src_value[index]);
      initialize_location(dst, test_case->types[index], 
                          test_case->dst_status[index], 
                          test_case->dst_where[index], 
                          test_case->dst_value[index]);

      src++;
      dst++;
      index++;
    }
  }

  src_frame->verify_conform_to( dst_frame );
}
#endif

void VSFMergeTest::initialize_location(RawLocation * const loc,
                                       const BasicType type,
                                       const RawLocation::Status status,
                                       const Value::ValueLocation where,
                                       const int value) {
  loc->set_type(type);
  loc->set_status(status);
  loc->set_where(where);
  loc->set_value(value);

  if (status != RawLocation::flushed && where == Value::T_REGISTER) {
    GUARANTEE(Assembler::first_register <= value && 
              value <= Assembler::last_register, "Must be a valid register");
  }    
}

void VSFMergeTest::verify_merge(VirtualStackFrame* src_frame, 
                                VirtualStackFrame* dst_frame JVM_TRAPS) {
  Compiler * const compiler = Compiler::current();

  GUARANTEE(compiler != NULL && compiler->code_generator() != NULL, "Sanity");

  // Merge code modifies the source frame, so use a copy.
  VirtualStackFrame* src_copy = src_frame->clone(JVM_SINGLE_ARG_CHECK);
  src_frame = src_copy;

  CodeGenerator* saved_code_generator = compiler->code_generator();
  VirtualStackFrame* saved_src_frame = compiler->frame();

  CompilerState state;
  saved_code_generator->save_state(&state);

  // Unlink unbound literals to prevent VSFMergeTester from writing literals.
  {
#if defined(ARM) || defined(HITACHI_SH) || ENABLE_THUMB_COMPILER
    state.set_first_literal( NULL );
    state.set_first_unbound_literal( NULL );
    state.set_last_literal( NULL );
#endif

#if ENABLE_THUMB_COMPILER
    state.set_first_unbound_branch_literal( NULL );
    state.set_last_unbound_branch_literal( NULL );
#endif
  }

  VSFMergeTester tester(src_frame, dst_frame, &state);

#if defined(ARM) || defined(HITACHI_SH) || ENABLE_THUMB_COMPILER
  // Zero literal count to prevent VSFMergeTester from writing literals.
  tester.zero_literal_count();
#endif

  compiler->set_frame(src_frame);

  {
    REGISTER_REFERENCES_CHECKER;
    src_frame->conform_to_impl(dst_frame);
  }

  tester.verify_merge();

  tester.cleanup();

  GUARANTEE(saved_code_generator->code_size() == state.code_size(), "Sanity");

  compiler->set_frame( saved_src_frame );
  compiler->set_code_generator( saved_code_generator );
  jvm_fast_globals.compiler_code_generator = saved_code_generator;

}  

bool VSFMergeTest::_initialized = false;
bool VSFMergeTest::_is_available_register[Assembler::number_of_registers];
Assembler::Register VSFMergeTest::_available_int_registers[Assembler::number_of_registers];
unsigned int VSFMergeTest::_available_int_registers_count = 0;

void VSFMergeTest::initialize() {
  if (_initialized) {
    return;
  }

  unsigned int int_registers_count = 0;

  Assembler::Register int_reg;
  Assembler::Register first_allocatable_reg;
  int i = 0;
  
  // Find the first allocatable register most CPU this is r0 on SH it is r1 
  int_reg = RegisterAllocator::_next_register_table[i];    
  while (int_reg == Assembler::no_reg) {    
    int_reg = RegisterAllocator::_next_register_table[++i];
  }
  first_allocatable_reg = int_reg;
  
  do {
    _available_int_registers[int_registers_count++] = int_reg;
    int_reg = RegisterAllocator::next_for(int_reg);
  } while (int_reg != first_allocatable_reg);

  _available_int_registers_count = int_registers_count;

  for (int index = 0; index < Assembler::number_of_registers; index++) {
    const Assembler::Register reg = (Assembler::Register)index;
    const Assembler::Register next_reg = RegisterAllocator::next_for(reg);
    
    if (next_reg != Assembler::no_reg) {
      _is_available_register[next_reg] = true;
    }
  }

  _initialized = true;
}

int VSFMergeTester::_location_values[VSFMergeTester::MAX_STACK_LOCATIONS];
int VSFMergeTester::_memory_values[VSFMergeTester::MAX_STACK_LOCATIONS];
int VSFMergeTester::_register_values[Assembler::number_of_registers];
int VSFMergeTester::_register_hi_values[Assembler::number_of_registers];

VSFMergeTester::VSFMergeTester(VirtualStackFrame* src_frame, 
                               VirtualStackFrame* dst_frame,
                               CompilerState* state) :
 CodeGenerator( state, Compiler::current()->current_compiled_method() ), 
 _src_frame(src_frame), _dst_frame(dst_frame), _state(state) {
  if (src_frame->virtual_stack_pointer() + 1 > MAX_STACK_LOCATIONS) {
    return;
  }
  Compiler::current()->set_code_generator(this);

  VSFMergeTest::initialize();

  int index;
  {
    // These two arrays contain for each register R the index of the lowest
    // location mapped to this register or -1 if this register is not mapped to
    // any location. 
    int src_mappings[Assembler::number_of_registers];
    int dst_mappings[Assembler::number_of_registers];

    for (index = 0; index < Assembler::number_of_registers; index++) {
      _register_values[index] = original_register_value(index);
      _register_hi_values[index] = ILLEGAL_VALUE;
      src_mappings[index] = -1;
      dst_mappings[index] = -1;
    }

    for (index = 0; index < MAX_STACK_LOCATIONS; index++) {
      _location_values[index] = ILLEGAL_VALUE;
      _memory_values[index] = ILLEGAL_VALUE;
    }

    {
      AllocationDisabler allocation_not_allowed_in_this_block;
      RawLocation *src = src_frame->raw_location_at(0);
      RawLocation *end = src_frame->raw_location_end(src);
      RawLocation *dst = dst_frame->raw_location_at(0);
      index = 0;

      while (src < end) {
        GUARANTEE(index < MAX_STACK_LOCATIONS, "Location index out of bounds");
        const bool two_word = is_two_word(src->type());
        if (src->type() == T_ILLEGAL || src->stack_type() != dst->stack_type()) {
          _location_values[index] = ILLEGAL_VALUE;
          if (two_word) {
            _location_values[index + 1] = ILLEGAL_VALUE;
          }
        } else {
          if (!src->is_flushed() && src->is_immediate()) {
            GUARANTEE(src->is_cached() || src->is_changed(), "Sanity");

            _location_values[index] = validate(src->value());
            if (two_word) {
              _location_values[index + 1] = validate((src + 1)->value());
            }
          } else if (!dst->is_flushed() && dst->is_immediate()) {
            GUARANTEE(dst->is_cached() || dst->is_changed(), "Sanity");
            GUARANTEE(src->is_flushed(), "Illegal location");

            _location_values[index] = validate(dst->value());
            if (two_word) {
              _location_values[index + 1] = validate((dst + 1)->value());
            }
          } else {
            int value_index = index;
            
            if (!dst->is_flushed() && dst->in_register()) {
              GUARANTEE(dst->is_cached() || dst->is_changed(), "Sanity");
              const Assembler::Register lo_reg = dst->get_register();
              GUARANTEE(is_available_register(lo_reg), 
                        "Must be available for allocation");
              
              if (dst_mappings[lo_reg] != -1) {
                // Some other location is also mapped to this register, 
                // so both the must have the same value.
                value_index = 
                  location_index(_location_values[dst_mappings[lo_reg]]);
              } else {
                dst_mappings[lo_reg] = index;
              }
            }
            
            if (!src->is_flushed() && src->in_register()) {
              GUARANTEE(src->is_cached() || src->is_changed(), "Sanity");
              const Assembler::Register lo_reg = src->get_register();
              GUARANTEE(is_available_register(lo_reg), 
                        "Must be available for allocation");
              
              if (src_mappings[lo_reg] != -1) {
                // Some other location is also mapped to this register, 
                // so both the must have the same value.
                value_index = 
                  location_index(_location_values[src_mappings[lo_reg]]);
              } else {
                src_mappings[lo_reg] = index;
              }
            }
            
            _location_values[index] = location_value(value_index);
            if (two_word) {
              _location_values[index + 1] = location_value(value_index + 1);
            }
          }
            
          if (!src->is_flushed() && src->in_register()) {
            GUARANTEE(src->is_cached() || src->is_changed(), "Sanity");
            const Value dummy(src->type());
            const bool two_registers = dummy.use_two_registers();

            const Assembler::Register lo_reg = src->get_register();
            GUARANTEE(is_available_register(lo_reg), 
                      "Must be available for allocation");
            _register_values[lo_reg] = _location_values[index];
            if (two_registers) {
              GUARANTEE(two_word, "Must be a two-word value");

              const Assembler::Register hi_reg = (src + 1)->get_register();
              GUARANTEE(is_available_register(hi_reg), 
                        "Must be available for allocation");
              _register_values[hi_reg] = _location_values[index + 1];

              GUARANTEE(_register_hi_values[lo_reg] == ILLEGAL_VALUE && 
                        _register_hi_values[hi_reg] == ILLEGAL_VALUE,
                        "One-word registers");
            } else if (two_word) {
              _register_hi_values[lo_reg] = _location_values[index + 1];
            } else {
              GUARANTEE(_register_hi_values[lo_reg] == ILLEGAL_VALUE,
                        "One-word register");
            }
          } else {
            GUARANTEE(src->is_flushed() || src->is_immediate(), 
                      "Illegal location");
          }
          
          if (src->is_cached() || src->is_flushed()) {
            _memory_values[index] = _location_values[index];
            if (two_word) {
              _memory_values[index + 1] = _location_values[index + 1];
            }
          } else {
            GUARANTEE(src->is_changed(), "Sanity");
          }
        }

        if (two_word) {
          src++;
          dst++;
          index++;
        }
        src++;
        dst++;
        index++;
      }
    } 
  }
}

// Verify the merge results.
void VSFMergeTester::verify_merge() {
#ifdef AZZERT
  VirtualStackFrame* src_frame = _src_frame;
  VirtualStackFrame* dst_frame = _dst_frame;

  if (src_frame->virtual_stack_pointer() + 1 > MAX_STACK_LOCATIONS) {
    return;
  }

  AllocationDisabler allocation_not_allowed_in_this_block;
  RawLocation *src = src_frame->raw_location_at(0);
  RawLocation *end = src_frame->raw_location_end(src);

  RawLocation *dst = dst_frame->raw_location_at(0);
  int index = 0;

  while (src < end) {
    GUARANTEE(index < MAX_STACK_LOCATIONS, "Location index out of bounds");
    const bool two_word = is_two_word(src->type());
    if (src->type() != T_ILLEGAL && dst->type() != T_ILLEGAL &&
        src->stack_type() == dst->stack_type()) {
      const int location_value = _location_values[index];
      const int memory_value = _memory_values[index];

      GUARANTEE(location_value != ILLEGAL_VALUE, "Sanity");
      AZZERT_ONLY_VAR(memory_value);
      AZZERT_ONLY_VAR(location_value);

      // Verify value in memory
      if (dst->is_flushed() || dst->is_cached()) {
        GUARANTEE(memory_value != ILLEGAL_VALUE, "Merge error");

        GUARANTEE(location_value == ILLEGAL_VALUE || 
                  location_value == memory_value, "Merge error");

        if (two_word) {
          GUARANTEE(is_two_word(dst->type()), "Sanity");
          const int hi_location_value = _location_values[index + 1];
          const int hi_memory_value = _memory_values[index + 1];

          GUARANTEE(hi_memory_value != ILLEGAL_VALUE, "Merge error");

          GUARANTEE(hi_location_value == ILLEGAL_VALUE || 
                    hi_location_value == hi_memory_value, "Merge error");
          AZZERT_ONLY_VAR(hi_memory_value);
          AZZERT_ONLY_VAR(hi_location_value);
        }
      }

      // Verify value in register
      if (!dst->is_flushed() && dst->in_register()) {
        GUARANTEE(dst->is_cached() || dst->is_changed(), "Sanity");
        const Value dummy(dst->type());
        const bool two_registers = dummy.use_two_registers();

        const Assembler::Register dst_lo_reg = dst->get_register();
        GUARANTEE(is_available_register(dst_lo_reg), 
                  "Must be available for allocation");
        
        GUARANTEE(_register_values[dst_lo_reg] == location_value, 
                  "Merge error");
        AZZERT_ONLY_VAR(dst_lo_reg);

        if (two_registers) {
          GUARANTEE(two_word && is_two_word(dst->type()), "Sanity");
          const Assembler::Register dst_hi_reg = (dst + 1)->get_register();
          const int hi_location_value = _location_values[index + 1];
          GUARANTEE(is_available_register(dst_hi_reg), 
                    "Must be available for allocation");

          GUARANTEE(_register_values[dst_hi_reg] == hi_location_value, 
                    "Merge error");

          // Verify that one-word registers are used properly.
          GUARANTEE(_register_hi_values[dst_hi_reg] == ILLEGAL_VALUE, 
                    "Merge error");
          GUARANTEE(_register_hi_values[dst_lo_reg] == ILLEGAL_VALUE, 
                    "Merge error");
          AZZERT_ONLY_VAR(dst_hi_reg);
          AZZERT_ONLY_VAR(hi_location_value);
        } else if (two_word) {
          const int hi_location_value = _location_values[index + 1];
          GUARANTEE(_register_hi_values[dst_lo_reg] == hi_location_value, 
                    "Merge error");
          AZZERT_ONLY_VAR(hi_location_value);
        } else {
          // Verify that one-word register are used properly.
          GUARANTEE(_register_hi_values[dst_lo_reg] == ILLEGAL_VALUE, 
                    "Merge error");
        }

        // Verify immediate
        if (!dst->is_flushed() && dst->is_immediate()) {
          GUARANTEE(dst->is_cached() || dst->is_changed(), "Sanity");
          GUARANTEE(location_value == validate(dst->value()), 
                    "Merge error");

          if (two_word) {
            const int hi_location_value = _location_values[index + 1];
            
            GUARANTEE(hi_location_value == validate((dst+1)->value()), 
                      "Merge error");
            AZZERT_ONLY_VAR(hi_location_value);
          }
        }
      }
    }
    if (two_word) {
      src++;
      dst++;
      index++;
    }
    src++;
    dst++;
    index++;
  }

  // Verify that registers not available for allocation are not changed.
  for (index = 0; index < Assembler::number_of_registers; index++) {
    if (!is_available_register(index)) {
      GUARANTEE(_register_values[index] == original_register_value(index), 
                "Merge error");
    }
  }
#endif
}

void VSFMergeTester::cleanup() {
#if USE_COMPILER_LITERALS_MAP
  LiteralPoolElement* last_literal = _state->last_literal();
  // Discard all literals appended by the VSFMergeTester.
  if( last_literal ) {
    last_literal->set_next( NULL );
  }
#endif
}

void VSFMergeTester::load_from_location(Value& result, 
                                        jint index, 
                                        Condition cond) {
  GUARANTEE(index < MAX_STACK_LOCATIONS, "Location index out of bounds");
  GUARANTEE(cond == always, "Conditional moves not supported");

  CodeGenerator::load_from_location(result, index, cond);

  if (result.in_register()) {
    _register_values[result.lo_register()] = _memory_values[index];
    if (result.is_two_word()) {
      GUARANTEE(index + 1 < MAX_STACK_LOCATIONS, 
                "Location index out of bounds");
      const int hi_value = _memory_values[index + 1];

      if (result.use_two_registers()) {
        _register_values[result.hi_register()] = hi_value;

        GUARANTEE(_register_hi_values[result.lo_register()] == ILLEGAL_VALUE && 
                  _register_hi_values[result.hi_register()] == ILLEGAL_VALUE,
                  "One-word registers");
      } else {
        _register_hi_values[result.lo_register()] = hi_value;
      }
    } else {
      GUARANTEE(_register_hi_values[result.lo_register()] == ILLEGAL_VALUE,
                "One-word register");
    }
  }
}

void VSFMergeTester::store_to_location(Value& value, jint index) {
  GUARANTEE(index < MAX_STACK_LOCATIONS, "Location index out of bounds");

  if (value.in_register()) {
    _memory_values[index] = _register_values[value.lo_register()];
    if (value.is_two_word()) {
      GUARANTEE(index + 1 < MAX_STACK_LOCATIONS, 
                "Location index out of bounds");

      int hi_value = ILLEGAL_VALUE;

      if (value.use_two_registers()) {
        hi_value = _register_values[value.hi_register()];

        GUARANTEE(_register_hi_values[value.lo_register()] == ILLEGAL_VALUE && 
                  _register_hi_values[value.hi_register()] == ILLEGAL_VALUE,
                  "One-word registers");
      } else {
        hi_value = _register_hi_values[value.lo_register()];
      }

      _memory_values[index + 1] = hi_value;
    }
  } else if (value.is_immediate()) {
    _memory_values[index] = _location_values[index];
    if (value.is_two_word()) {
      _memory_values[index + 1] = _location_values[index + 1];
    }
  }

  CodeGenerator::store_to_location(value, index);
}

void VSFMergeTester::move(Assembler::Register dst, 
                          Assembler::Register src,
                          Condition cond) {
  GUARANTEE(cond == always, "Conditional moves not supported");
  GUARANTEE(Assembler::first_register <= src &&
            (int)src < Assembler::number_of_registers,
            "Src bounds check");
  GUARANTEE(Assembler::first_register <= dst &&
            (int)dst < Assembler::number_of_registers,
            "Dst bounds check");
  _register_values[dst] = _register_values[src];
  _register_hi_values[dst] = _register_hi_values[src];

  CodeGenerator::move(dst, src, cond);
}

void VSFMergeTester::move(const Value& dst, const Value& src, 
                          const Condition cond) {
  // if the source isn't present there's nothing left to do
  if (!src.is_present()) return;

  GUARANTEE(cond == always, "Conditional moves not supported");
  GUARANTEE(dst.type() == src.type(), "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");
  if (src.is_immediate()) {
    if (src.is_two_word()) {
      const int target_lo_bits = validate(src.lo_bits());
      const int target_hi_bits = validate(src.hi_bits());
      int host_lo_bits = target_lo_bits;
      int host_hi_bits = target_hi_bits;

#if CROSS_GENERATOR    
      // For cross-generator we should handle the case when the layout of long 
      // or double values is different on host and target platforms. 
      if ((TARGET_MSW_FIRST_FOR_LONG != MSW_FIRST_FOR_LONG && 
           src.type() == T_LONG) ||
          (TARGET_MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_DOUBLE &&
           ENABLE_FLOAT && src.type() == T_DOUBLE)) {
        host_lo_bits = target_hi_bits;
        host_hi_bits = target_lo_bits;
      }
#endif // CROSS_GENERATOR

      if (src.use_two_registers()) {
        _register_values[dst.lo_register()] = host_lo_bits;
        _register_values[dst.hi_register()] = host_hi_bits;

        GUARANTEE(_register_hi_values[dst.lo_register()] == ILLEGAL_VALUE && 
                  _register_hi_values[dst.hi_register()] == ILLEGAL_VALUE,
                  "One-word registers");
      } else {
        _register_values[dst.lo_register()] = host_lo_bits;
        _register_hi_values[dst.lo_register()] = host_hi_bits;
      }
    } else {
      _register_values[dst.lo_register()] = validate(src.lo_bits());

      GUARANTEE(_register_hi_values[dst.lo_register()] == ILLEGAL_VALUE,
                "One-word register");
    }
  } else {
    GUARANTEE(src.in_register(), "source must be in register");
    _register_values[dst.lo_register()] = 
      _register_values[src.lo_register()];

    if (src.use_two_registers()) {
      GUARANTEE(src.is_two_word(), "Must be a two-word value");

      _register_values[dst.hi_register()] = 
        _register_values[src.hi_register()];

      GUARANTEE(_register_hi_values[dst.lo_register()] == ILLEGAL_VALUE &&
                _register_hi_values[src.lo_register()] == ILLEGAL_VALUE &&
                _register_hi_values[dst.hi_register()] == ILLEGAL_VALUE &&
                _register_hi_values[src.hi_register()] == ILLEGAL_VALUE,
                "One-word registers");
    } else if (src.is_two_word()) {
      _register_hi_values[dst.lo_register()] = 
        _register_hi_values[src.lo_register()];
    } else {
      GUARANTEE(_register_hi_values[dst.lo_register()] == ILLEGAL_VALUE &&
                _register_hi_values[src.lo_register()] == ILLEGAL_VALUE,
                "One-word registers");
    }
  }

  CodeGenerator::move(dst, src, cond);
}

#endif /* ENABLE_COMPILER */
#endif /* !PRODUCT */
