/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_CompiledMethod_arm.cpp.incl"

#if ENABLE_COMPILER && !ENABLE_THUMB_COMPILER

#if ENABLE_CODE_PATCHING

bool Compiler::_is_undoing_patching;
int BytecodeCompileClosure::_jump_from_bci;

bool Compiler::can_patch(int bci_from, int bci_to) {
  if (bci_from >= bci_to) {
    return false;
  }

  register jubyte *bcptr = (jubyte*)Compiler::method()->code_base();
  for(int bci = bci_from; bci < bci_to;) {    
    const Bytecodes::Code code = (Bytecodes::Code)*(bcptr + bci);
    if (Bytecodes::disables_code_patching(code)) {
      return false;
    }

    const int simple_len = Bytecodes::length_for(code);
    if (simple_len > 0) {
      bci += simple_len;
    } else if (simple_len == 0) {
      const int wide_len = Bytecodes::wide_length_for(Compiler::method(), bci, code);
      if (wide_len > 0) {
        bci += wide_len;
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    } else {
      SHOULD_NOT_REACH_HERE();
    }
  }
  return true;
}

void Compiler::update_checkpoints_table(CompiledMethod* cm) {
  GUARANTEE((cm != NULL) && cm->not_null(), "Sanity");

  for (RelocationReader stream(cm); !stream.at_end(); stream.advance()) {
    if (stream.is_checkpoint_info()) {          
      const int offset = stream.code_offset();

      const int pos = offset + CompiledMethod::base_offset();

      const int initial_instruction = 
        (stream.current(1) << BitsPerShort) | stream.current(2);

      const int current_instruction = cm->int_field(pos);      
      if (initial_instruction != current_instruction) {
        cm->short_field_put(stream.current_offset(1), 
          current_instruction >> BitsPerShort);
        cm->short_field_put(stream.current_offset(2), 
          current_instruction & 0xFFFF);
      }
    }
  }
}

CompiledMethodDesc* Compiler::get_current_compiled_method(address current_pc) {
  GUARANTEE(Thread::current() != NULL, "Sanity");
  GUARANTEE(!ENABLE_TIMER_THREAD, "Not supported for timer threads");
  GUARANTEE(!ObjectHeap::is_gc_active(), "Wrong frame state");

  CompiledMethodDesc* cm = NULL;
  address pc = 0;
  if (Thread::current()->last_java_frame_exists()) {
    JavaFrame fr(Thread::current());
    if (fr.is_heap_compiled_frame()) {
      pc = fr.pc();
    }
  } 

  if (pc == 0) {            
    pc = current_pc;
  }  
  return CompiledMethodCache::item_for_address(pc);      
}

void Compiler::patch_compiled_method(CompiledMethod* cm) {
  GUARANTEE(cm != NULL, "Sanity");
  GUARANTEE(cm->not_null(), "Sanity");

  bool patched_anything = false;

  for (RelocationReader stream(cm); !stream.at_end(); stream.advance()) {
    if (stream.is_checkpoint_info()) {          
      const int offset = stream.code_offset();           
      const int end_bit = 1 << (BitsPerShort - 1);

      int branch_offset = 0;
      // skip type and offset (1) + skip original instruction (2)
      int index = 1 + 2;
      if(!(end_bit & stream.current(index))) {
        branch_offset = bitfield(stream.current(index++), 0, BitsPerShort - 1);
      }

      branch_offset = (branch_offset << (BitsPerShort - 1)) | 
        bitfield(stream.current(index), 0, BitsPerShort - 1);
      
      GUARANTEE(stream.current(index) & end_bit, "Sanity");

      //
      // Branch instruction:
      //   (cond << 28) | (0xA << 24) | ((offset_26 - 8) >> 2)
      //
      branch_offset -= 8;
      GUARANTEE(abs(offset) <= 0x003ffffff, "branch offset limit");
      const int new_instruction = 0xEA000000 | ((branch_offset & 0x3ffffff) >> 2);

      const int pos = offset + CompiledMethod::base_offset();      
      cm->int_field_put(pos, new_instruction);      
      patched_anything = true;

      // flush icache
#if !USE_SINGLE_METHOD_FLUSHING
      OsMisc_flush_icache((address)cm->obj() + pos, BytesPerInt);
#endif
    }
  }
#if USE_SINGLE_METHOD_FLUSHING
  if (patched_anything) {
    cm->flush_icache();
    CompiledMethodCache::set_patched_method((CompiledMethodDesc*)cm->obj());
  }
#endif
}

// 
// Patches checkpoints from the current java method.
// Only one method can be patched at the given moment.
// 
void Compiler::patch_checkpoints(address current_pc) {
  if (Compiler::is_undoing_patching() || ObjectHeap::is_gc_active()) {
    return;
  }

  // retrieve the current java method from the frame
  CompiledMethod::Raw cm = get_current_compiled_method(current_pc);
  
  
#if USE_PATCHED_METHOD_CACHE
  CompiledMethod::Raw patched_cm = CompiledMethodCache::patched_method();    
  if (cm().not_null() && (cm().obj() != patched_cm().obj())) {
    {
      // if patched method is set then unpatch it first        
      if (patched_cm().not_null()) {
        unpatch_compiled_method(&patched_cm);

      }    
    }
    patch_compiled_method(&cm);
  }
#else
  if (cm().not_null()) {
    // patch current java method and reset patched_method
    patch_compiled_method(&cm);
  }  
#endif
}

void Compiler::unpatch_compiled_method(CompiledMethod* cm) {
  GUARANTEE(cm != NULL, "Sanity");
  GUARANTEE(cm->not_null(), "Sanity");
  GUARANTEE(!Compiler::is_undoing_patching(), "Sanity");

  TemporaryModifyGlobal(&_is_undoing_patching, !_is_undoing_patching);

  for (RelocationReader stream(cm); !stream.at_end(); stream.advance()) {
    if (stream.is_checkpoint_info()) {
      const int offset = stream.code_offset() + CompiledMethod::base_offset();
      const int old_instruction =
         (stream.current(1) << BitsPerShort) | (stream.current(2));
      cm->int_field_put(offset, old_instruction);
#if !USE_SINGLE_METHOD_FLUSHING
      OsMisc_flush_icache((address)cm->obj() + offset, BytesPerInt);
#endif
    }
  }
#if USE_SINGLE_METHOD_FLUSHING
  cm->flush_icache();
#endif
}

void Compiler::unpatch_checkpoints() {
#if USE_PATCHED_METHOD_CACHE
  CompiledMethod::Raw cm = CompiledMethodCache::patched_method();

  if (cm().not_null()) {
    unpatch_compiled_method(&cm);
    CompiledMethodCache::reset_patched_method();
  }
#else
  CompiledMethod::Raw cm = get_current_compiled_method(0);
  if (cm().not_null()) {
    unpatch_compiled_method(&cm);
  }
#endif
}
#endif // ENABLE_CODE_PATCHING

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

#if USE_DEBUG_PRINTING
static void decode_instruction(Stream* st, int* addr, int offset) {
  Disassembler(st).disasm(addr, *addr, offset);
}
#endif

void CompiledMethod::print_code_on(Stream* st) {
#if USE_DEBUG_PRINTING
  int end_offset = RelocationReader::code_length(this);
  for (int offset = 0; offset < end_offset; offset += 4) { 
    int* instruction_start = (int *)(entry() + offset);
    print_comment_for(offset, st);

    bool is_oop = false;
    bool is_address = false;
    for (RelocationReader stream(this); !stream.at_end(); stream.advance()) {
      if (stream.code_offset() == offset) { 
        if (stream.is_oop() || stream.is_rom_oop()) { 
          is_oop = true;
          break;
        } else if (stream.is_compiler_stub()) { 
          GUARANTEE(GenerateROMImage, "Must be generating ROM Image");
          is_address = true;
          break;
        }
      }
    }
    st->print(" %4d: ", offset);
    if (VerbosePointers) {
      st->print("0x%08x: ", instruction_start);
    }
    if (is_oop) { 
      Oop o = (OopDesc*)*instruction_start;
      o.print_value_on(st);
    } else if (is_address) { 
      st->print("Address");
      if (VerbosePointers) {
        st->print(" 0x%x", *instruction_start);
      }
    } else { 
      decode_instruction(st, instruction_start, offset);
    }
    st->cr();
  }
#endif
}

#endif // !PRODUCT

#endif


#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int CompiledMethod::generate_fieldmap(TypeArray* field_map) {
#if !ENABLE_COMPILER
  SHOULD_NOT_REACH_HERE();
  return 0;
#else
  int map_index = 0;
  int max_length = size() + 3;
  if (field_map->length() < max_length) {
    // Treat everything as a byte after the header
    return max_length;
  }
  // Generic object near
  map_index = Near::generate_fieldmap(field_map);

  // flags_and_size
  field_map->byte_at_put(map_index++, T_INT);
  // method
  field_map->byte_at_put(map_index++, T_OBJECT);

#if ENABLE_JVMPI_PROFILE
  // code_size
  field_map->byte_at_put(map_index++, T_INT);
#endif    

  int offset;
  int end_offset = RelocationReader::code_length(this);
  for (offset = 0; offset < end_offset; offset += 4) { 
    bool is_oop = false;
    bool is_address = false;
    for (RelocationReader stream(this); !stream.at_end(); stream.advance()) {
      if (stream.code_offset() == offset) { 
        if (stream.is_oop() || stream.is_rom_oop()) { 
          is_oop = true;
          break;
        } else if (stream.is_compiler_stub()) { 
          GUARANTEE(GenerateROMImage, "Must be generating ROM Image");
          is_address = true;
          break;
        }
      }
    }
    if (is_oop) { 
        // field_map->int_at_put(map_index++, T_INT);
      field_map->byte_at_put(map_index++, T_OBJECT);
    } else if (is_address) { 
      field_map->byte_at_put(map_index++, T_SYMBOLIC);
    } else { 
      field_map->byte_at_put(map_index++, T_INT);
    }
  }
  // Now deal with the relocation table
  for (; offset < size(); offset += 2) {
    field_map->byte_at_put(map_index++, T_SHORT);
  }
  if ((offset % 4) != 0) {
    // IMPL_NOTE: why will this happen? Anyway, let's just add a Short to be sure.
    field_map->byte_at_put(map_index++, T_SHORT);
  }
  return map_index;
#endif
}

#endif /* #if ENABLE_ROM_GENERATOR*/
