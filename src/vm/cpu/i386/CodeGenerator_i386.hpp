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

// This file is #include'd by src/vm/share/compiler/CodeGenerator.hpp inside
// the declaration of the CodeGenerator class.

private:
  BinaryAssembler::generic_binary_op_1 convert_to_generic_binary_1(BytecodeClosure::binary_op op);

  void call_from_compiled_code(Register reg, int offset, 
                               int parameters_size JVM_TRAPS);
  void call_from_compiled_code(address entry, int parameters_size JVM_TRAPS);

  // JVM_TRAPS sometimes requires that an optional argument be in the middle.  
  void call_from_compiled_code(Register reg, int offset JVM_TRAPS) {
    call_from_compiled_code(reg, offset, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
  void call_from_compiled_code(address entry JVM_TRAPS) { 
    call_from_compiled_code(entry, 0 JVM_NO_CHECK_AT_BOTTOM);
  }

  void ishift_helper(Value& result, Value& op1, Value& op2);
  void idiv_helper(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void verify_fpu() PRODUCT_RETURN;

public:
  // Intel x86 specific float arithmetic support
  void fpu_clear(bool flush = false);
  void fpu_prepare_unary(Value& op);
  void fpu_prepare_binary_arithmetic(Value& op1, Value& op2, bool& reversed);
  void fpu_prepare_binary_fprem(Value& op1, Value& op2);
  void fpu_load_constant(const Value& dst, const Value& src, const double value);

  void write_call_info(int parameters_size JVM_TRAPS);

  enum {
    // number of bytes between the start of the callinfo word and the start
    // of the first word of tagging
    extended_callinfo_tag_word_0_offset = 5,

    // number of bytes between the start of a word of tagging and subsequent
    // words of tagging.
    extended_callinfo_tag_word_n_offset = 5,
  };


  // increment the CPU stack pointer without affecting CPU flags
  void increment_stack_pointer_by(int increment);

  void cmp_values(Value& op1, Value& op2, 
                  BytecodeClosure::cond_op condition) {
    (void)condition;
    cmp_values(op1, op2);
  }

private:
  // load value from the given memory address
  void store_tag_to_address(BasicType type, StackAddress& address);

  void cmp_values(Value& op1, Value& op2);
