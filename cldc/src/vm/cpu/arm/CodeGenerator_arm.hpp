/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#if !ENABLE_THUMB_COMPILER

// This file is #include'd by src/vm/share/compiler/CodeGenerator.hpp inside
// the declaration of the CodeGenerator class.

private:
  void method_prolog(Method* method JVM_TRAPS);
  void call_through_gp(address& target JVM_TRAPS) {
     call_through_gp(target, /*speed=*/true JVM_NO_CHECK_AT_BOTTOM);
  }
  void call_through_gp(address& target, bool speed JVM_TRAPS);

  void call_from_compiled_code(Register dst, int offset, 
                               int parameters_size JVM_TRAPS) {
    call_from_compiled_code(dst, offset, parameters_size, 
                            /*indirect=*/ false,
                            /*speed=*/ true
                            JVM_NO_CHECK_AT_BOTTOM);
  }
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR
  void call_from_compiled_code(Method* callee, Register dst, int offset, 
                               int parameters_size JVM_TRAPS) {
    call_from_compiled_code(callee, dst, offset, parameters_size, 
                            /*indirect=*/ false,
                            /*speed=*/ true
                            JVM_NO_CHECK_AT_BOTTOM);
  }

void call_from_compiled_code(Method* callee, Register dst, int offset, 
                               int parameters_size, bool indirect,
                               bool speed JVM_TRAPS);
#endif
  void call_from_compiled_code(Register dst, int offset, 
                               int parameters_size, bool indirect,
                               bool speed JVM_TRAPS);

  // BinaryAssembler.
  void arithmetic(Opcode opcode, Value& result, Value& op1, Value& op2);
  void larithmetic(Opcode opcode1, Opcode opcode2, Value& result, Value& op1,
                   Value& op2);
  void shift(Shift shifter, Value& result, Value& op1, Value& op2);

  void call_simple_c_runtime(Value& result, address runtime_func, 
                          Value& op1, Value& op2) { 
    vcall_simple_c_runtime(result, runtime_func, &op1, &op2, NULL);
  }
  void call_simple_c_runtime(Value& result, address runtime_func, 
                          Value& op1) {
    vcall_simple_c_runtime(result, runtime_func, &op1, NULL);
  }

  void call_simple_c_runtime(Value& result, address runtime_func, 
                          Value& op1, Value& op2, Value& op3) {
    vcall_simple_c_runtime(result, runtime_func, &op1, &op2, &op3, NULL);
  }

  void vcall_simple_c_runtime(Value& result, address runtime_func, ...);

  void idiv_rem(Value& result, Value& op1, Value& op2,
                bool isRemainder JVM_TRAPS);

  void lshift(Shift type, Value& result, Value& op1, Value& op2);
  void lshift_reg(Shift type, Value& result, Value& op1, Value& op2);
  void lshift_imm(Shift type, Value& result, Value& op1, int shift);

  void adjust_for_invoke(int parameters_size, BasicType return_type,
                         bool native = false);
   
  void setup_c_args(int ignored, ...);
  void vsetup_c_args(va_list ap);

  void shuffle_registers(Register* dstReg, Register* srcReg, int regCount);

  Assembler::Condition maybe_null_check_1(Value& object);
  void maybe_null_check_2(Assembler::Condition cond JVM_TRAPS);
#if ENABLE_NPCE
  Assembler::Condition maybe_null_check_1_signal(Value& object, bool& is_npe);
  void maybe_null_check_2_signal(Assembler::Condition cond,
                                 bool is_npe JVM_TRAPS);
  void maybe_null_check_3_signal(Assembler::Condition cond, bool is_npe,
                                 BasicType type JVM_TRAPS);
#endif

  // Assign registers to result.  Try to reuse op if possible
  void assign_register(Value& result, Value& op);

  void restore_last_frame(JVM_SINGLE_ARG_TRAPS);

  void lookup_switch(Register index, jint table_index, 
                     jint start, jint end, jint default_dest JVM_TRAPS);
  bool dense_lookup_switch(Value& index, jint table_index, jint default_dest,
                           jint num_of_pairs JVM_TRAPS);

  int get_inline_thrower_gp_index(int rte JVM_TRAPS);
  enum {
    // Don't compile any tableswitch that's bigger than this, or else
    // we may easily run out of range for ldr_literal. See unit test
    // case
    // vm.cpu.arm.CodeGenerator_arm.table_switch1
    MAX_TABLE_SWITCH_SIZE = 128,

    // On xscale and ARMv6, use an overflow stub instead of ldr pc for
    // stack overflow checking. ldr pc is slower.
    USE_OVERFLOW_STUB = (ENABLE_XSCALE_WMMX_INSTRUCTIONS||ENABLE_ARM_V6)
  };

public:
  void write_call_info(int parameters_size JVM_TRAPS);
  enum {
    // number of bytes between the start of the callinfo word and the start
    // of the first word of tagging
    extended_callinfo_tag_word_0_offset = -4,

    // number of bytes between the start of a word of tagging and the next
    // word of tagging.
    extended_callinfo_tag_word_n_offset = -4
  };

  friend class CompilerLiteralAccessor;
  #if ENABLE_INLINE && ARM
  friend class BytecodeCompileClosure;
  #endif

  void increment_stack_pointer_by(int adjustment) {
    add_imm(jsp, jsp, JavaStackDirection * adjustment * BytesPerStackElement);
  }

#endif /*#if !ENABLE_THUMB_COMPILER*/
