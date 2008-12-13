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
#include "incls/_CallInfo.cpp.incl"

#if ENABLE_EMBEDDED_CALLINFO

// A CompiledMethod start at a workboundary which means we can shave
// the first two bits of the start_offset without loosing information.
inline int CallInfo::stored_start_offset(int start_offset) {
  return start_offset >> LogBytesPerWord;
}

inline int CallInfo::restored_start_offset(int stored_start_offset) {
  int alignment = bitfield((jint) this, 0, LogBytesPerWord); // compute the jint alignment of this
  int start_offset =  stored_start_offset << LogBytesPerWord;
  return start_offset + alignment;
}

bool CallInfo::fits_compiled_compact_format(int bci, int start_offset, int number_of_tags) {
  // Verify parameters can be used for compact information
  GUARANTEE(bci >= 0, "bci must be positive");
  if (bci > right_n_bits(format1_bci_width)) return false;
  
  GUARANTEE(start_offset >= 0, "offset_to_start must be positive");
  if (stored_start_offset(start_offset) > right_n_bits(format1_start_offset_width)) return false;

  GUARANTEE(number_of_tags >= 0, "number_of_locations must be positive");
  if (number_of_tags > format1_tag_width) return false;

  return true;
}

CallInfo CallInfo::compiled_compact(int bci, int offset_to_start) {
  int sso = stored_start_offset(offset_to_start);
  GUARANTEE(   bci < (1 << format1_bci_width) 
            && sso < (1 << format1_start_offset_width),
            "Shouldn't have called compiled_compact");
  return CallInfo( (((juint)bci) << format1_bci_start)
                |  (((juint)sso) << format1_start_offset_start)
                |  (((juint)1)   << compact_start));
}

CallInfo CallInfo::compiled(int bci, int offset_to_start JVM_TRAPS) {
  int sso = stored_start_offset(offset_to_start) + 1;
  if (   bci >= (1 << format2_bci_width)
      || sso >= (1 << format2_start_offset_width)) {
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_(0));
  }
  return CallInfo( (bci  << format2_bci_start)
                |  (sso  << format2_start_offset_start));
}

int CallInfo::bci() {
  GUARANTEE(in_compiled_code(), "must be in compiled code");
  return is_compact_format()
       ? bitfield(_value, format1_bci_start, format1_bci_width)
       : bitfield(_value, format2_bci_start, format2_bci_width);
}

int CallInfo::start_offset() {
  GUARANTEE(in_compiled_code(), "must be in compiled code");
  return restored_start_offset(
           is_compact_format()
           ? bitfield(_value, format1_start_offset_start, format1_start_offset_width)
           : (bitfield(_value, format2_start_offset_start, format2_start_offset_width) - 1));
}

void CallInfo::set_oop_tag_at(int index) {
  GUARANTEE(is_compact_format(), "must be in compiled compact code");
  GUARANTEE(index >= 0 && index < format1_tag_width, "index must be within bit field");
  set_nth_bit(_value, index);  
}

Tag CallInfo::compact_tag_at(int index) { 
 GUARANTEE(is_compact_format(), "Must be compact format");
 GUARANTEE(index >= 0 && index < format1_tag_width, 
           "index must be within bit field");
 return is_set_nth_bit(_value, index) ? obj_tag : int_tag;
}

#if ENABLE_COMPILER
Tag CallInfo::extended_tag_at(int index) { 
  GUARANTEE(in_compiled_code() && !is_compact_format(), "CallInfo format");
  GUARANTEE(index >= 0, "Index must be positive");
  int word_offset, bit_offset;
  index_to_offsets(index, word_offset, bit_offset);
  jint *word = (jint *)
      // Two constants below are architecture-dependent.
      (((address) this) + CodeGenerator::extended_callinfo_tag_word_0_offset
         + (word_offset * CodeGenerator::extended_callinfo_tag_word_n_offset));
  int nybble = bitfield(*word, bit_offset, 4);
  Tag result = nybble_to_stack_tag(nybble);
  return result;
} 

ReturnOop CallInfo::new_tag_array(int size JVM_TRAPS) {
  size_t length = (size + 7 + tag_array_overhead) >> 3;
  ReturnOop obj;

  if (Compiler::is_active()) {
    obj = Universe::new_int_array_in_compiler_area(length JVM_CHECK_0);
  } else {
    obj = Universe::new_int_array(length JVM_CHECK_0);
  }
  TypeArray::Raw result(obj);
  result().int_at_put(0, size);
  return result;
}
#endif

void CallInfo::set_tag(TypeArray* array, int index, BasicType type) {
  Tag tag = basic_type2tag(type);
  int nybble = stack_tag_to_nybble(tag);
  int word_offset, bit_offset;
  CallInfo::index_to_offsets(index, word_offset, bit_offset);
  array->int_at_put(word_offset, 
                    array->int_at(word_offset) | (nybble << bit_offset));
  if (is_two_word(type)) {
    CallInfo::index_to_offsets(index+1, word_offset, bit_offset);
    GUARANTEE(stack_tag_to_nybble(long2_tag) == stack_tag_to_nybble(long_tag) + 1
        &&    stack_tag_to_nybble(double2_tag) == stack_tag_to_nybble(double_tag) + 1,
        "Sanity check for calculation below");
    array->int_at_put(word_offset, 
                    array->int_at(word_offset) | ((nybble + 1) << bit_offset));
  }
}

#if USE_COMPILER_STRUCTURES

CompiledMethodDesc* CallInfo::compiled_method() {
  GUARANTEE(in_compiled_code(), "must be in compiled code");
  CompiledMethodDesc* result = DERIVED(CompiledMethodDesc*, this, 
                            - start_offset() - CompiledMethod::base_offset());
  if (!ObjectHeap::is_gc_active() || CompilerAreaPercentage == 0) {
    // If we use a CompilerArea, during compaction of the compiler area
    // result->_klass contains the offset.
    GUARANTEE(result->is_compiled_method(), "must be a compiled method");
  }
  return result;
}

#endif

#ifndef PRODUCT

void CallInfo::print(Stream* st) {
  if (in_interpreter()) {
    st->print_cr("CallInfo (interpreter)");
    return;
  }

  st->print("CallInfo (compiled): bci=%d, start_offset=%d", bci(), start_offset());
  if (is_compact_format()) {
    st->print(" [");
    for (int index = 0; index < format1_tag_width; index++) {
      st->print(compact_tag_at(index) == obj_tag ? "O" : "I"); 
    }
    st->print("]");
  }
  st->cr();
}

#endif

#endif // ENABLE_EMBEDDED_CALLINFO
