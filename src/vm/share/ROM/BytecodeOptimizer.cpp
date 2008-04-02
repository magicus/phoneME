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
#include "incls/_BytecodeOptimizer.cpp.incl"

#if ENABLE_ROM_GENERATOR && !USE_PRODUCT_BINARY_IMAGE_GENERATOR && !ENABLE_CPU_VARIANT

/*
here we are searching for the following pattern:
dup
load index(iconst, b(s)ipush)
load value (*const, ldc(_w))
save to array (*astore)
*/
bool BytecodeOptimizer::has_static_arrays(Method *method, int& new_method_size) {
#ifdef AZZERT
  void * base = _owner->_new_bytecode_address.base_address();
  jvm_memset(base, 0xff, sizeof(jushort) * _owner->_new_bytecode_address.length());
#endif

  int old_bci, new_bci = 0;  
  int start_bci = 0;  // bci of first instruction which could be transformed
                      // into _init_static_array
  int old_bcis[4];    // bcis of instructions which could be transformed
                      // into _init_static_array
  bool result = false;

  reset_parser();
  Bytecodes::Code code_for_array_type = Bytecodes::_nop;

  for (old_bci = 0; old_bci != method->code_size();) {
    GUARANTEE(old_bci < method->code_size(), "invalid bytecode");

    const Bytecodes::Code code = method->bytecode_at( old_bci );
    const int len = method->bytecode_length_for( old_bci ); 
    int new_len = len;

    if (code == Bytecodes::_tableswitch || code == Bytecodes::_lookupswitch) {
      const int old_aligned = align_size_up(old_bci + 1, sizeof(jint));
      const int new_aligned = align_size_up(new_bci + 1, sizeof(jint));
      const int old_pad = old_aligned - old_bci;
      const int new_pad = new_aligned - new_bci;
      new_len = len + (new_pad - old_pad);
      result = result || ((new_pad - old_pad) != 0); //it's unnecessary, cause old_bci already ne new_bci, but....
    }
    
    _owner->_new_bytecode_address.short_at_put(old_bci, new_bci);

    new_bci += new_len;
    old_bcis[parser_state] = old_bci;

    if (_owner->is_branch_target(old_bci)) {
      reset_parser();
    } else {
      switch (parser_state) {
        case 0: //only dup succeed
        {
          if (code == Bytecodes::_dup) {
            if (array_element_count == 0) {
              code_for_array_type = Bytecodes::_nop; //nulling out array type
              start_bci = new_bci - new_len;
            }                        
            parser_state++;                        
          } else {
            reset_parser();
          }
        }
        break;     
        case 1:
        {
          if (check_array_index(code, method, old_bci)) {
            parser_state++;
          } else {
            reset_parser();
          }
        }
        break;
        case 2: 
        {
          if (code > 0 && code <= Bytecodes::_ldc2_w) {
            parser_state++;
          } else {
#if ENABLE_JAVA_STACK_TAGS
            if (code >= Bytecodes::_fast_ildc && code <= Bytecodes::_fast_dldc_w) {
#else
            if (code >= Bytecodes::_fast_1_ldc && code <= Bytecodes::_fast_2_ldc_w) {
#endif
              parser_state++;
            } else {
              reset_parser();
            }
          }
        }
        break;
        case 3: 
        {
          if (code >= Bytecodes::_iastore && code <= Bytecodes::_sastore && code != Bytecodes::_aastore) {
            if (code_for_array_type == Bytecodes::_nop) {
              code_for_array_type = code;
            }
            if (code != code_for_array_type) {
              reset_parser();
              break;
            }

            int size_factor = 0;
            switch (code_for_array_type) {
            case Bytecodes::_bastore:
              size_factor = 1;
              break;
            case Bytecodes::_castore:
            case Bytecodes::_sastore:
              size_factor = 2;
              break;
            case Bytecodes::_iastore:
            case Bytecodes::_fastore:
              size_factor = 4;
              break;
            case Bytecodes::_lastore:
            case Bytecodes::_dastore:
              size_factor = 8;              
            }
            _owner->_new_bytecode_address.short_at_put(old_bcis[0], start_bci);
            _owner->_new_bytecode_address.short_at_put(old_bcis[1], start_bci);
            _owner->_new_bytecode_address.short_at_put(old_bcis[2], start_bci);
            _owner->_new_bytecode_address.short_at_put(old_bci, start_bci);

            parser_state = 0;
            array_element_count++; //increase element count
            new_bci = start_bci + 4 + array_element_count*size_factor;
            result = true;
          } else {
            reset_parser();
          }
        }
        break;
        default:
        SHOULD_NOT_REACH_HERE();
      }
    }
    old_bci += len;
  }
  // Mark the address of code[code_length] so that we can rewrite the
  // exception header correctly.  The end_pc of an exception range is one
  // beyond the last pc that could cause an exception
  _owner->_new_bytecode_address.short_at_put(old_bci, new_bci);
  
  new_method_size = new_bci;
  return result;  
}

ReturnOop BytecodeOptimizer::optimize_static_arrays(Method *method JVM_TRAPS) {
  int new_size = 0;
  _owner->init_branch_targets(method JVM_CHECK_0);      
  if (!has_static_arrays(method, new_size)) {
    GUARANTEE(new_size == method->code_size(), "sanity");
    return method->obj();
  }
  if (new_size > _owner->_new_bytecode_address.length()) {
    _owner->_new_bytecode_address = Universe::new_short_array(new_size + 1 JVM_CHECK_0);
    has_static_arrays(method, new_size);
  }
  int current_value1 = 0, current_value2 = 0;   
  Bytecodes::Code code_for_array_type = Bytecodes::_nop;

  // Allocate the method and set up its header (to use the old CP for the
  // time being)
  Method new_method = _owner->copy_method(method, new_size JVM_CHECK_0);  

  int old_bci, new_bci;
  int bci_start = 0;//bci from which _init_static_array may start
  int bci_stackmap_update_from = 0;
  int stackmap_shift = 0;  
  reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);

  for (old_bci = 0, new_bci = 0; old_bci != method->code_size();) {
    GUARANTEE(old_bci < method->code_size(), "sanity");
    //new_bci could be larger than code size if init_static_array is the last bytecode 
    //and code size is decreased
    //GUARANTEE(new_bci < new_method.code_size(), "sanity");

    // (1) determine the length of this bytecode
    const Bytecodes::Code code = method->bytecode_at(old_bci);
    const int old_len = method->bytecode_length_for(old_bci);
    int new_len = old_len;

    // (2) Stream the bytecode, using new CP index and branch offsets
    //     if necessary
    switch (code) {

    case Bytecodes::_ifeq:
    case Bytecodes::_ifne:
    case Bytecodes::_iflt:
    case Bytecodes::_ifge:
    case Bytecodes::_ifgt:
    case Bytecodes::_ifle:
    case Bytecodes::_if_icmpeq:
    case Bytecodes::_if_icmpne:
    case Bytecodes::_if_icmplt:
    case Bytecodes::_if_icmpge:
    case Bytecodes::_if_icmpgt:
    case Bytecodes::_if_icmple:
    case Bytecodes::_if_acmpeq:
    case Bytecodes::_if_acmpne:
    case Bytecodes::_goto:
    case Bytecodes::_jsr:
    case Bytecodes::_ifnull:
    case Bytecodes::_ifnonnull:
      GUARANTEE(old_len == 3, "sanity");
      _owner->stream_branch2(method, &new_method, old_bci, new_bci);
      break;

    case Bytecodes::_goto_w:
    case Bytecodes::_jsr_w:
      GUARANTEE(old_len == 5, "sanity");
      _owner->stream_branch4(method, &new_method, old_bci, new_bci);
      break;

    case Bytecodes::_tableswitch:
      new_len = _owner->stream_tableswitch(method, &new_method, old_bci,
                                           new_bci);
      break;

    case Bytecodes::_lookupswitch:
      new_len = _owner->stream_lookupswitch(method, &new_method, old_bci,
                                            new_bci);
      break;

    default:
      // none of the remaining bytecodes need rewriting.
      if (new_bci + old_len <= new_method.code_size()) { //they would be rewritten otherwise
        _owner->stream_raw_bytes(method, &new_method, old_bci, new_bci, old_len);
      }
      break;      
    }

    //we should update new_bci now, cause it could be rewritten later!
    new_bci += new_len;


    //check if we need to insert _init_static_array here
    if (_owner->is_branch_target(old_bci)) {
      reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);
    } else {
      switch (parser_state) {
        case 0: //only dup succeed
        {
          old_code_size += new_len;
          if (code == Bytecodes::_dup) {
            if (array_element_count == 0) {
              code_for_array_type = Bytecodes::_nop; //nulling out array type
              bci_start = new_bci - new_len;                            
            }           
            parser_state++;
          } else {
            reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);
          }
        }
        break;     
        case 1:
        {          
          old_code_size += new_len;
          if (check_array_index(code, method, old_bci)) {
            parser_state++;
          } else {
            reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);
          }
        }
        break;
        case 2: 
        {
          old_code_size += new_len;
          if (get_values(code, method, old_bci, current_value1, current_value2)) {
            parser_state++;
          } else {
            reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);
          }          
        }
        break;
        case 3: 
        {
          old_code_size += new_len;
          if (code >= Bytecodes::_iastore && code <= Bytecodes::_sastore && code != Bytecodes::_aastore) {
            if (code_for_array_type == Bytecodes::_nop) {
              code_for_array_type = code;
            }
            if (code != code_for_array_type) {
              reset_parser(method, bci_stackmap_update_from, stackmap_shift JVM_CHECK_0);
              break;
            }

            int size_factor = 0;
            switch (code_for_array_type) {
            case Bytecodes::_bastore:
              size_factor = 1;
              break;
            case Bytecodes::_castore:
            case Bytecodes::_sastore:
              size_factor = 2;
              break;
            case Bytecodes::_iastore:
            case Bytecodes::_fastore:
              size_factor = 4;
              break;
            case Bytecodes::_lastore:
            case Bytecodes::_dastore:
              size_factor = 8;              
            }

            if (array_element_count == 0) { //add header
              add_static_array_header(&new_method, bci_start,
                                      code_for_array_type);              
            }
            
            add_static_array_item(&new_method, current_value1, current_value2,
                                  code_for_array_type, array_element_count,
                                  bci_start);            
            parser_state = 0;
            array_element_count++;                        
            stackmap_shift = 4 + array_element_count * size_factor 
                             - old_code_size;
            bci_stackmap_update_from = bci_start + old_code_size;            
            new_bci = bci_start + 4 + array_element_count * size_factor;
          } else {
            reset_parser(method, bci_stackmap_update_from, stackmap_shift
                         JVM_CHECK_0);
            break;              
          }
        }
        break;
        default:
        SHOULD_NOT_REACH_HERE();              
      }
    }
    // (4) Advance to next bytecode
    old_bci += old_len;

    // (5) update the stackmaps for this shift in bci
    if (new_len != old_len) {
      short bci_shift = new_len - old_len;      
      StackmapGenerator::rewrite_stackmap_bcis(method, new_bci - bci_shift,
                                               bci_shift JVM_CHECK_0);
    }
  }

  TypeArray exception_table = new_method.exception_table();
  if (!exception_table.is_null() && exception_table.length() > 0) {
    // 4-tuples of ints [start_pc, end_pc, handler_pc, catch_type index]
    GUARANTEE(!new_method.is_abstract() && !new_method.is_native(), "sanity");
    int len = exception_table.length();
    TypeArray new_exception_table = Universe::new_short_array(len JVM_CHECK_0);

    for (int i=0; i<len; i+=4) {
      jushort start_pc   = exception_table.ushort_at(i + 0);
      jushort end_pc     = exception_table.ushort_at(i + 1);
      jushort handler_pc = exception_table.ushort_at(i + 2);      
      jushort type_index = exception_table.ushort_at(i + 3);

      start_pc   = _owner->_new_bytecode_address.ushort_at(start_pc);
      end_pc     = _owner->_new_bytecode_address.ushort_at(end_pc);
      handler_pc = _owner->_new_bytecode_address.ushort_at(handler_pc);

      new_exception_table.ushort_at_put(i + 0, start_pc);
      new_exception_table.ushort_at_put(i + 1, end_pc);
      new_exception_table.ushort_at_put(i + 2, handler_pc);
      new_exception_table.ushort_at_put(i + 3, type_index);
    }
    new_method.set_exception_table(&new_exception_table);  
  }  
  return new_method;  
}

bool BytecodeOptimizer::get_values(Bytecodes::Code code, Method* method, int old_bci, 
                                   int& current_value1, int& current_value2) {
  current_value1 = 0;
  current_value2 = 0;
  if (code > 0 && code <= Bytecodes::_ldc2_w) {
    switch (code) {
    case(Bytecodes::_aconst_null):
      return true;
    case(Bytecodes::_iconst_m1):
    case(Bytecodes::_iconst_0):
    case(Bytecodes::_iconst_1):
    case(Bytecodes::_iconst_2):
    case(Bytecodes::_iconst_3):
    case(Bytecodes::_iconst_4):
    case(Bytecodes::_iconst_5):
      current_value1 = code - 3;
      return true;

    case(Bytecodes::_lconst_0):
    case(Bytecodes::_lconst_1):
      current_value1 = code - Bytecodes::_lconst_0;
      return true;
#if ENABLE_FLOAT
    case(Bytecodes::_fconst_0):
    case(Bytecodes::_fconst_1):
    case(Bytecodes::_fconst_2): 
    {             
      float value = jvm_i2f(code - Bytecodes::_fconst_0);
      current_value1 = *(int*)(&value);
      return true;
    }
    case(Bytecodes::_dconst_0):
    case(Bytecodes::_dconst_1): 
    {
      jlong dbits = double_bits(jvm_i2d(code - Bytecodes::_dconst_0));
      current_value1 = lsw(dbits);
      current_value2 = msw(dbits);
      return true;
    }
#endif
    case(Bytecodes::_bipush):
      current_value1 = method->get_byte(old_bci + 1);
      return true;
    case(Bytecodes::_sipush):
      current_value1 = method->get_java_short(old_bci + 1);
      return true;
    case(Bytecodes::_ldc):
      get_values_from_cp(method, method->get_ubyte(old_bci + 1), current_value1, current_value2);              
      return true;
    case(Bytecodes::_ldc_w):
    case(Bytecodes::_ldc2_w):
      get_values_from_cp(method, method->get_java_ushort(old_bci + 1), current_value1, current_value2);              
      return true;
    }
  } else {
#if ENABLE_JAVA_STACK_TAGS
    if (code >= Bytecodes::_fast_ildc && code <= Bytecodes::_fast_dldc_w) {
      switch (code) {
      case(Bytecodes::_fast_aldc ):
        current_value1 = method->get_ubyte(old_bci + 1);
        return true;
      case(Bytecodes::_fast_aldc_w):
        current_value1 = method->get_java_ushort(old_bci + 1);
        return true;
      case(Bytecodes::_fast_ildc ):
      case(Bytecodes::_fast_fldc ):              
        get_values_from_cp(method, method->get_ubyte(old_bci + 1), current_value1, current_value2);              
        return true;
      case(Bytecodes::_fast_ildc_w ):
      case(Bytecodes::_fast_fldc_w ):
      case(Bytecodes::_fast_lldc_w ):
      case(Bytecodes::_fast_dldc_w ):
        get_values_from_cp(method, method->get_java_ushort(old_bci + 1), current_value1, current_value2);              
        return true;
      }
#else
    if (code >= Bytecodes::_fast_1_ldc && code <= Bytecodes::_fast_2_ldc_w) {
      switch (code) {
        case(Bytecodes::_fast_1_ldc ):
          get_values_from_cp(method, method->get_ubyte(old_bci + 1), current_value1, current_value2);
          return true;
        case(Bytecodes::_fast_1_ldc_w ):
        case(Bytecodes::_fast_2_ldc_w ):
          get_values_from_cp(method, method->get_java_ushort(old_bci + 1), current_value1, current_value2);
          return true;
        }
#endif              
    } else {
    return false;
    }
  }
  return false;
}

bool BytecodeOptimizer::check_array_index(Bytecodes::Code code, Method* method, int bci) {
  switch (code) {
  case Bytecodes::_iconst_0: 
  case Bytecodes::_iconst_1:
  case Bytecodes::_iconst_2:
  case Bytecodes::_iconst_3:
  case Bytecodes::_iconst_4:
  case Bytecodes::_iconst_5:
  {
    if (array_element_count == (code - Bytecodes::_iconst_0)) {
       return true;
    } else {
       return false;
    }
  }
  break;
  case Bytecodes::_bipush  :
  {
    if (method->get_ubyte(bci + 1) == array_element_count) {
      return true;
    } else {
      return false;
    }
  }
  break;
  case Bytecodes::_sipush  :
  {
    if (method->get_java_short(bci + 1) == array_element_count) {
      return true;
    } else {
      return false;
    }
  } 
  break;
  }
  return false;
}

void BytecodeOptimizer::add_static_array_header(Method *method, 
                                      int bci_start, int code_for_array_type) {
  
  method->bytecode_at_put_raw(bci_start, Bytecodes::_init_static_array);
  //here we put log2(type_size)
  switch(code_for_array_type) {
    case(Bytecodes::_bastore):
      method->byte_at_put(bci_start + 1, 0);
      break;
    case(Bytecodes::_castore):
    case(Bytecodes::_sastore):
      method->byte_at_put(bci_start + 1, 1);
      break;
    case(Bytecodes::_iastore):
#if ENABLE_FLOAT
    case(Bytecodes::_fastore):
#endif
      method->byte_at_put(bci_start + 1, 2);
      break;
    case(Bytecodes::_lastore):
#if ENABLE_FLOAT
    case(Bytecodes::_dastore):
#endif
      method->byte_at_put(bci_start + 1, 3);
      break;
    default:
      SHOULD_NOT_REACH_HERE();
  }
}

//we assume here that values are added sequentially
void BytecodeOptimizer::add_static_array_item(Method *method, 
                              int value1, int value2, 
                              int code_for_array_type, int number,
                              int bci_start) {
  method->put_native_ushort(bci_start + 2, number + 1);
  //the header contains bytecode + type + array size = 1 + 1 + 2 = 4 bytes.
  int bci = bci_start + 4;
  switch (code_for_array_type) {
    case Bytecodes::_bastore:
      bci += number;
      method->byte_at_put(bci, value1);
      bci++;
      break;
    case Bytecodes::_castore:
    case Bytecodes::_sastore:
      bci += 2 * number;
      method->put_native_short(bci, value1);
      bci += 2;
      break;
    case Bytecodes::_iastore:
#if ENABLE_FLOAT
    case Bytecodes::_fastore:    
#endif
      bci += 4 * number;
      method->put_native_int(bci, value1);
      bci += 4;
      break;
    case Bytecodes::_lastore:
      bci += 8 * number;
      method->put_native_int(bci, TARGET_MSW_FIRST_FOR_LONG ? value2 : value1);
      method->put_native_int(bci + 4, TARGET_MSW_FIRST_FOR_LONG ? value1 : value2);
      break;
#if ENABLE_FLOAT
    case Bytecodes::_dastore:
      bci += 8 * number;
      method->put_native_int(bci, TARGET_MSW_FIRST_FOR_DOUBLE ? value2 : value1);
      method->put_native_int(bci + 4, TARGET_MSW_FIRST_FOR_DOUBLE ? value1 : value2);
      bci += 8;
      break;
#endif
    default:
      SHOULD_NOT_REACH_HERE();
   }  
}

void BytecodeOptimizer::get_values_from_cp(Method* method, int index, int& value1, int& value2) {
  //we cannot use raw handles here due to allocation which could happen
  ConstantPool cp = method->constants();
  switch (cp.tag_value_at(index)) {
  case JVM_CONSTANT_Integer:
    value1 = cp.int_at(index);
    value2 = 0;
    break;
  case JVM_CONSTANT_Long: {
    jlong lvalue = cp.long_at(index);
    value1 = lsw(lvalue);
    value2 = msw(lvalue);
    break;
    }
#if ENABLE_FLOAT
  case JVM_CONSTANT_Float: {
    float fvalue = cp.float_at(index);
    value1 = *(int*)&fvalue;
    value2 = 0;
    break;
    }
  case JVM_CONSTANT_Double: {
    jlong dbits = double_bits(cp.double_at(index));
    value1 = lsw(dbits);
    value2 = msw(dbits);
    break;
    }
#endif
//  default: 
    //we can come here to load string but we won't succeed later
  }
}

void BytecodeOptimizer::print_bytecode_statistics(Stream *stream) {
  int i;
  for (i=0; i<Bytecodes::number_of_java_codes; i++) {
    int n = _num_optimized_bytecodes[i];
    if (n > 0) {
      Bytecodes::Code code = (Bytecodes::Code)i;
      stream->print_cr("\t %-25s =%5d", Bytecodes::name(code), n);
    }
  }
  stream->cr();
}

// Replace some bytecodes with equivalent but shorter codes.
ReturnOop BytecodeOptimizer::optimize_bytecodes(Method *p_method JVM_TRAPS) {
 
  UsingFastOops fast1;
  Method::Fast result = optimize_static_arrays(p_method JVM_CHECK_0);  

  int bci;
  //
  // (1) First loop: fast_{i,a}getfield -> fast_{i,a}getfield_1
  //
  for (bci = 0; bci != result().code_size();) {
    GUARANTEE(bci < result().code_size(), "invalid bytecode");

    const Bytecodes::Code code = result().bytecode_at(bci);
    const int len = result().bytecode_length_for(bci);

    switch (code) {
    case Bytecodes::_fast_igetfield:
    case Bytecodes::_fast_agetfield:
      {
        const int offset = ENABLE_NATIVE_ORDER_REWRITING
                           ? result().get_native_ushort(bci+1)
                           : result().get_java_ushort  (bci+1);
        if( offset < 256 ) {
          const Bytecodes::Code new_code = code == Bytecodes::_fast_igetfield
                           ? Bytecodes::_fast_igetfield_1
                           : Bytecodes::_fast_agetfield_1;
          result().bytecode_at_put_raw(bci, new_code);
          result().ubyte_at_put(bci+1, offset);
          result().bytecode_at_put_raw(bci+2, Bytecodes::_nop);

          ++_num_optimized_bytecodes[new_code];
        }
      }
      break;
    }
    bci += len;
  }

  //
  // (2) Second loop: (3 bytes) aload_0,fast_{i,a}getfield_1 ->   
  //                  (2 bytes) aload_0_fast_{i,a}getfield_1
  //               or (1 byte)  aload_0_fast_{i,a}getfield_{4,8}
  _owner->init_branch_targets(&result JVM_CHECK_0);
  Bytecodes::Code last_code = Bytecodes::number_of_java_codes;

  for (bci = 0; bci != result().code_size();) {
    GUARANTEE(bci < result().code_size(), "invalid bytecode");

    const Bytecodes::Code code = result().bytecode_at(bci);
    const int len = result().bytecode_length_for(bci);

    switch (code) {
    case Bytecodes::_fast_igetfield_1:
    case Bytecodes::_fast_agetfield_1:
      if (last_code == Bytecodes::_aload_0 && !_owner->is_branch_target(bci)) {
        const int offset = (result().ubyte_at(bci + 1)) * BytesPerWord;
        Bytecodes::Code new_code;

        if (offset == 4) {
          if (code == Bytecodes::_fast_igetfield_1) {
            new_code = Bytecodes::_aload_0_fast_igetfield_4;
          } else {
            new_code = Bytecodes::_aload_0_fast_agetfield_4;
          }
        }
        else if (offset == 8) {
          if (code == Bytecodes::_fast_igetfield_1) {
            new_code = Bytecodes::_aload_0_fast_igetfield_8;
          } else {
            new_code = Bytecodes::_aload_0_fast_agetfield_8;
          }
        }
        else if (code == Bytecodes::_fast_igetfield_1) {
          new_code = Bytecodes::_aload_0_fast_igetfield_1;
        } else {
          new_code = Bytecodes::_aload_0_fast_agetfield_1;
        }

        result().bytecode_at_put_raw(bci-1, Bytecodes::_nop);
        result().bytecode_at_put_raw(bci,   new_code);

        if (offset == 4 || offset == 8) {
          result().bytecode_at_put_raw(bci+1, Bytecodes::_nop);
        }
        ++_num_optimized_bytecodes[new_code];
      }
      break;
    }
    last_code = code;
    bci += len;
  }
  return result.obj();
}

void  BytecodeOptimizer::reset_parser() {
  delta_offset = parser_state = array_element_count = 0;
}

void  BytecodeOptimizer::reset_parser(Method* method, int& bci_stackmap_update_from, 
                                                    int& stackmap_shift JVM_TRAPS) {
  if (stackmap_shift != 0) {    
    StackmapGenerator::rewrite_stackmap_bcis(method, bci_stackmap_update_from,
                                               stackmap_shift JVM_CHECK);      
  }
  old_code_size = delta_offset = parser_state = array_element_count = 0;
  bci_stackmap_update_from = stackmap_shift = 0;
}

#endif //ENABLE_ROM_GENERATOR && !USE_PRODUCT_BINARY_IMAGE_GENERATOR
