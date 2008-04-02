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

#if !ENABLE_THUMB_COMPILER

// This file is #include'd by src/vm/share/compiler/CodeGenerator.hpp inside
// the declaration of the CodeGenerator class.

private:
  void method_prolog(Method* method JVM_TRAPS);
  int get_jsp_shift(Method* method) {
    int extra_locals = method->max_locals() - method->size_of_parameters();
    int jsp_shift = extra_locals*BytesPerStackElement +
                    JavaFrame::frame_desc_size();
    return jsp_shift;
  }
  void link_frame_step1();
  void call_through_gp(address* target_ptr JVM_TRAPS) {
     call_through_gp(target_ptr, /*speed=*/true JVM_NO_CHECK_AT_BOTTOM);
  }
  void call_through_gp(address* target_ptr, bool speed JVM_TRAPS);

  void call_from_compiled_code(Register dst, int offset, 
                               int parameters_size JVM_TRAPS) {
    call_from_compiled_code(dst, offset, parameters_size, 
                            /*indirect=*/ false,
                            /*speed=*/ true
                            JVM_NO_CHECK_AT_BOTTOM);
  }
#if ENABLE_TRAMPOLINE
  void call_from_compiled_code(const Method* callee, Register dst, int offset, 
                               int parameters_size JVM_TRAPS) {
    GUARANTEE(!GenerateROMImage, "Should not be used for precompiled methods");
    call_from_compiled_code(callee, dst, offset, parameters_size, 
                            /*indirect=*/ false,
                            /*speed=*/ true
                            JVM_NO_CHECK_AT_BOTTOM);
  }

void call_from_compiled_code(const Method* callee, Register dst, int offset, 
                               int parameters_size, bool indirect,
                               bool speed JVM_TRAPS);
#endif
  void call_from_compiled_code(Register dst, int offset, 
                               int parameters_size, bool indirect,
                               bool speed JVM_TRAPS);

  // BinaryAssembler.
  void arithmetic(Opcode opcode, Value& result, Value& op1, Value& op2,
                  const CCMode cc = no_CC);
  void larithmetic(Opcode opcode1, Opcode opcode2,
                  Value& result, Value& op1, Value& op2);
  void shift(Shift shifter, Value& result, Value& op1, Value& op2,
                  const CCMode cc = no_CC);

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
   //check null point status by signal handler
  void maybe_null_check_2_by_npce(Value& object,
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
    USE_OVERFLOW_STUB = (ENABLE_XSCALE_WMMX_TIMER_TICK||ENABLE_ARM_V6)
  };

  void load_float_from_address(Value& result, BasicType type,
                               MemoryAddress& address,
                               Assembler::Condition cond);

  PRODUCT_INLINE void set_result_register(Value& result);

#if USE_FP_RESULT_IN_VFP_REGISTER
  void setup_fp_result(Value& result);
#endif

#if ENABLE_ARM_VFP
  void ensure_in_float_register(Value& value);
  void ensure_not_in_float_register(Value& value, bool need_to_copy=true); 
  void move_vfp_immediate(const Register dst,const jint src, const Condition cond = al);  
  void move_float_immediate(const Register dst, const jint src, const Condition cond = al);
  void move_double_immediate(const Register dst, const jint src_lo, const jint src_hi, 
                             const Condition cond = al);
#else
  void ensure_in_float_register(Value& /*value*/) {}
  void ensure_not_in_float_register(Value& /*value*/) {}
  void ensure_not_in_float_register(Value& /*value*/, bool /*need_to_copy*/) {}
#endif

  static bool _interleave_frame_linking;

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

  void increment_stack_pointer_by(int adjustment) {
    add_imm(jsp, jsp, JavaStackDirection * adjustment * BytesPerStackElement);
  }

  void cmp_values(Value& op1, Value& op2, 
                  BytecodeClosure::cond_op condition) {
    (void)condition;
    cmp_values(op1, op2);
  }

private:
  bool fold_arithmetic(Value& result, Value& op1, Value& op2 JVM_TRAPS);

  void imla(Value& result, Value& op1, Value& op2, Value& op3 JVM_TRAPS);

  void cmp_values(Value& op1, Value& op2, Assembler::Condition cond = al);

#if ENABLE_INLINED_ARRAYCOPY
public:
  class RegisterSetIterator;

  /*
   * A set of registers.
   * Invariant: each register in the set has exactly one reference.
   */
  class RegisterSet : public StackObj {
    juint _set;
    int   _length;
   public:
    RegisterSet() : _set(0), _length(0) {
      GUARANTEE(Assembler::number_of_gp_registers < BitsPerWord, 
                "Cannot have register mask as int");
    }

    /*
     * Removes all registers from the set and decrements their ref counts.
     */
    ~RegisterSet();

    bool is_empty() const {
      return _set == 0;
    }

    int length() const {
      return _length;
    }

    /* 
     * Adds given register to the set. 
     * Does not increase reference count for this register.
     */
    void add(Register reg);

    /* 
     * Removes given register from the set.
     * Decrements reference count for this register.
     */
    void remove(Register reg);

    /* 
     * Returns arbitrary register from the set. 
     */
    Register get_register() const;
    
    friend class RegisterSetIterator;
  };

  /*
   * Iterates over the given register set.
   * If the underlying set is modified during iteration, 
   * the behavior is undefined.
   */
  class RegisterSetIterator : public StackObj {
    juint _set;
    int   _index;
   public:
    RegisterSetIterator(const RegisterSet& reg_set) : 
     _set(reg_set._set), _index(0) {}

    bool has_next() const {
      return _set != 0;
    }

    Register next();
  };

private:
  /* Returns JVM.unchecked_XXX_arraycopy() for the given type. */
  static ReturnOop unchecked_arraycopy_method(BasicType type);

  /* Helper function to compute data start from offset and element size. */
  void compute_data_start(Value& result,
                          Value& array, Value& offset, int log_element_size,
                          bool& offset_word_aligned);

  /* 
   * Helper functions to emit appropriate load/store instructions depending on 
   * element size and direction of copying.
   */
  void ldrx(Register rd, Register rn, int element_size, Mode mode, 
            bool is_backward, Condition cond = always);
  void strx(Register rd, Register rn, int element_size, Mode mode, 
            bool is_backward, Condition cond = always);
  void ldmxa(Register rn, Address4 reg_set, WritebackMode mode, 
             bool is_backward, Condition cond = always);
  void stmxa(Register rn, Address4 reg_set, WritebackMode mode, 
             bool is_backward, Condition cond = always);

#endif
#endif /*#if !ENABLE_THUMB_COMPILER*/
