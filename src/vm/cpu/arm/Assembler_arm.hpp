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

#define MAKE_IMM(x) (x)

// The Assembler class provides minimal functionality to generate
// machine code. All functions representing machine instructions
// generate exactly one instruction (no optimizations!).

class Assembler: public GlobalObj {

#if !defined(PRODUCT) || ENABLE_COMPILER || USE_COMPILER_STRUCTURES

friend class Disassembler;
friend struct OpcodeInfo;

#if ENABLE_CODE_OPTIMIZER
  friend class CodeOptimizer;
#endif

 protected:
  // The implementation of emit is assembler specific: the source assembler
  // will disassemble the instruction and emit the corresponding assembly
  // source code, the binary assembler will emit the binary instruction.
  //
  // In product mode, only one implementation (the one for the binary
  // assembler) exists, and then the call is statically bound. In all
  // other modes, this is a virtual call so we can have both the text
  // and binary output with the same build.
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  virtual void emit(int instr) JVM_PURE_VIRTUAL_1_PARAM(instr);
#else
  inline static void emit(int instr);
#endif

  // assertion checking
#ifndef PRODUCT
  static void check_imm(int imm, int size) {
    if(!has_room_for_imm(imm, size)) {
       tty->print_cr("size %d too big for %d bits", imm, size);

    }
    GUARANTEE(has_room_for_imm(imm, size), "illegal immediate value");
  }
#else
  static void check_imm(int /*imm*/, int /*size*/) {}
#endif

 public:
  // can the immediate fit in size bits?
  static bool has_room_for_imm(int imm, int size) {
    return (imm & -(1 << size)) == 0;
  }
  enum {
    // Max number of method locals (exclusive) that are allowed to use
    // in-line exception thrower. For example: if a method has 3 local
    // words:
    //      cmp r0, #0
    //      ldreq pc, [r5, #compiler_throw_NullPointerException_3]
    MAX_INLINE_THROWER_METHOD_LOCALS = 10
  };
 protected:
  enum Opcode {
    // position and order is relevant!
    _andr, _eor, _sub, _rsb, _add, _adc, _sbc, _rsc,
    _tst, _teq, _cmp, _cmn, _orr, _mov, _bic, _mvn,
    number_of_opcodes
  };

  static Opcode as_opcode(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_opcodes, "illegal opcode");
    return (Opcode)encoding;
  }

 public:
  enum Register {
    // position and order is relevant!
    r0, r1, r2 , r3 , r4 , r5 , r6 , r7 ,
    r8, r9, r10, r11, r12, r13, r14, r15,

#if ENABLE_ARM_VFP
    // Single-precision floating point registers.
    s0, s1, s2, s3, s4, s5, s6, s7,
    s8, s9, s10, s11, s12, s13, s14, s15,
    s16, s17, s18, s19, s20, s21, s22, s23,
    s24, s25, s26, s27, s28, s29, s30, s31,
#endif

    number_of_registers,

#if ENABLE_ARM_VFP
    // Double-precision floating point registers.
    d0 = s0,
    d1 = s2,
    d2 = s4,
    d3 = s6,
    d4 = s8,
    d5 = s10,
    d6 = s12,
    d7 = s14,
    d8 = s16,
    d9 = s18,
    d10 = s20,
    d11 = s22,
    d12 = s24,
    d13 = s26,
    d14 = s28,
    d15 = s30,
#endif

    // for platform-independant code
    return_register = r0,
    stack_lock_register = r1,

    // for instruction assembly only
    sbz =  r0,
    sbo = r15,

#if !ENABLE_THUMB_REGISTER_MAPPING
    // interpreter register conventions for ARM COMPILER
    // (registers valid only during 30interpretation!)
    tos_val =  r0,
    tos_tag =  r1,
    callee  =  r0,  // stores MethodDesc on method entry
    tmp0    =  r2,
    tmp1    =  r3,
    tmp2    =  r4,
    gp      =  r5,
    jsp     =  r6,
    locals  =  r7,
    cpool   =  r8,
    tmp3    =  r9,
    tmp4    =  r10,
    fp      =  r11, // FIXED BY ARM CALLING CONVENTION
    tmp5    =  r12,
    bcode   =  r12,
    sp      =  r13, // FIXED BY ARM CALLING CONVENTION
    bcp     =  r14,
    lr      =  r14,
    pc      =  r15, // FIXED BY HARDWARE
#else
    // interpreter register conventions for THUMB COMPILER
    tos_val =  r0,
    tos_tag =  r1,
    callee  =  r0,  // stores MethodDesc on method entry
    tmp0    =  r2,
    tmp1    =  r3,
    fp      =  r4,
    gp      =  r5,
    jsp     =  r6,
    bcode   =  r7,
    tmp5    =  r7,
    locals  =  r8,
    cpool   =  r9,
    tmp2    =  r10,
    tmp3    =  r11,
    tmp4    =  r12,
    sp      =  r13, // FIXED BY ARM CALLING CONVENTION
    bcp     =  r14,
    lr      =  r14,
    pc      =  r15, // FIXED BY HARDWARE
#endif /*if !ENABLE_THUMB_REGISTER_MAPPING*/

    method_return_type = r2, // set to stack type on method return

    // for linkage w/ shared code - will probably need to change
    no_reg                    =  -1,
    first_register            =  r0,
#if ENABLE_ARM_VFP
    last_register             = s31,
    number_of_float_registers =  32,
#else
    last_register             = r15,
    number_of_float_registers =   0,
#endif
    number_of_gp_registers    =  16,

    first_allocatable_register = r0,

    // Force Register to be int size. Otherwise ADS would treat
    // Register as an unsigned byte type, and would emit a large number
    // of unnecessary opcodes to coerce int values such as Value::_low,
    // Value::_high, etc to unsigned bytes.
    _force_32bit_Register     = 0x10000000
  };

  static Register as_register(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_registers,
              "illegal register");
    return (Register)encoding;
  };

#if ENABLE_ARM_VFP
  static bool is_vfp_register(const Register reg) {
    return (unsigned(reg) - unsigned(s0)) < unsigned(number_of_float_registers);
  }

  static bool is_arm_register(const Register reg) {
    return unsigned(reg) < unsigned(s0);
  }
  
  enum VFPSystemRegister {
    fpsid = 0,
    fpscr = 1,
    fpexc = 8
  };
#endif

  enum Condition {
    eq, ne, cs, cc, mi, pl, vs, vc,
    hi, ls, ge, lt, gt, le, al, nv,
    number_of_conditions,
    // alternative names
    hs = cs,
    lo = cc,
    always = al                 // used in generic code
  };

  enum {
    instruction_alignment = 4
  };

  static bool is_c_saved_register(Register x) {
    return x >= r4 && x != r12 && x != r14;
  }

  static Condition as_condition(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_conditions,
              "illegal condition");
    return (Condition)encoding;
  }

  static Condition not_cond(Condition cond) {
    return (Condition)(cond ^ 1);
  }

  enum Shift {
   // position and order is relevant!
   lsl, lsr, asr, ror,
   number_of_shifts
  };

  static Shift as_shift(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_shifts, "illegal shift");
    return (Shift)encoding;
  }

  // addressing mode 1 - data-processing operand
  enum Address1 {
    // frequently used constants
    zero  = 1 << 25 | 0,
    one   = 1 << 25 | 1
  };

  static bool is_rotated_imm_slow(int x, Address1& result);

  static bool is_rotated_imm(int x, Address1& result) {
    if (((juint)x) < 0xff) {
      result = (Address1)(0x2000000 | x);
#ifdef AZZERT
      Address1 chk;
      GUARANTEE(is_rotated_imm_slow(x, chk) == true, "sanity");
      GUARANTEE(chk == result, "sanity");
#endif
      return true;
    } else if (((juint)-x) < 0xff) {
      // All small negative numbers are not rotated_imm's
#ifdef AZZERT
      Address1 chk;
      GUARANTEE(is_rotated_imm_slow(x, chk) == false, "sanity");
#endif
      return false;
    } else {
      return is_rotated_imm_slow(x, result);
    }
  }

  static bool is_rotated_imm(int x) {
    Address1 result;
    return is_rotated_imm(x, result);
  }

  static Address1 imm_rotate(int imm_8, int rotate_imm = 0) {
    check_imm(imm_8     , 8);
    check_imm(rotate_imm, 5);
    GUARANTEE((rotate_imm & 1) == 0, "rotate_imm must be multiple of 2");
    return (Address1)(1 << 25 | rotate_imm << 7 | imm_8);
  }

  static Address1 imm_shift(Register rm, Shift shift, int shift_imm) {
    check_imm(shift_imm, 5);
    return (Address1)(shift_imm << 7 | shift << 5 | rm);
  }

  static Address1 reg_shift(Register rm, Shift shift, Register rs) {
    GUARANTEE(rm != r15 && rs != r15, "unpredictable instruction");
    return (Address1)(rs << 8 | shift << 5 | 1 << 4 | rm);
  }

  // alternative forms
  static Address1 reg(Register rm) { return imm_shift(rm, lsl, 0); }
  static Address1 imm_slow(int imm_32);
#define imm(imm_32) \
        ((imm_32 & 0xFFFFFF00) == 0 ?  imm_rotate(imm_32, 0) \
                                     : imm_slow(imm_32))

  // addressing mode 2 - load and store word or unsigned byte
  enum Address2 {
    forceaddress2=0x10000000  // force Address2 to be int size
  };

  enum Mode {
    offset       = 1 << 24,
    pre_indexed  = 1 << 24 | 1 << 21,
    post_indexed = 0,
    mode_flags   = 1 << 24| 1 << 21
  };

  // support for indices/offsets (make sure these get inlined!)
  static int abs(int x)                          { return x < 0 ? -x : x; }
  static int up (int x)                          { return x < 0 ?  0 : 1; }

  static Address2 imm_index(Register rn, int offset_12 = 0, Mode mode = offset){
    GUARANTEE(rn != r15 || mode == offset, "unpredictable instruction");
    check_imm(abs(offset_12), 12);
    return (Address2)(mode | up(offset_12) << 23 | rn << 16 | abs(offset_12));
  }

  static Address2 add_index(Register rn, Register rm, Shift shift = lsl,
                            int shift_imm = 0, Mode mode = offset) {
    GUARANTEE((rn != r15 && rn != rm || mode == offset) && rm != r15,
              "unpredictable instruction");
    check_imm(shift_imm, 4);
    return (Address2)(1 << 25 | mode | 1 << 23 | rn << 16
                         | shift_imm << 7 | shift << 5 | rm );
  }

  static Address2 sub_index(Register rn, Register rm, Shift shift = lsl,
                            int shift_imm = 0, Mode mode = offset) {
    GUARANTEE((rn != r15 && rn != rm || mode == offset) && rm != r15,
              "unpredictable instruction");
    check_imm(shift_imm, 4);
    return (Address2)(1 << 25 | mode | rn << 16 | shift_imm << 7
                         | shift << 5 | rm );
  }

  // addressing mode 3 - miscellaneous load and store
  enum Address3 {
    forceaddress3=0x10000000  // force Address3 to be int size
  };

  static Address3 imm_index3(Register rn, int offset_8 = 0,
                             Mode mode = offset) {
    GUARANTEE(rn != r15 || mode == offset, "unpredictable instruction");
    check_imm(abs(offset_8), 8);
    return (Address3)(mode | up(offset_8) << 23 | 1 << 22 | rn << 16
                      | (abs(offset_8) & 0xf0) << 4 | abs(offset_8) & 0xf);
  }

  static Address3 add_index3(Register rn, Register rm, Mode mode = offset) {
    GUARANTEE((rn != r15 && rn != rm || mode == offset) && rm != r15,
              "unpredictable instruction");
    return (Address3)(mode | 1 << 23 | rn << 16 | rm );
  }

  static Address3 sub_index3(Register rn, Register rm, Mode mode = offset) {
    GUARANTEE((rn != r15 && rn != rm || mode == offset) && rm != r15,
              "unpredictable instruction");
    return (Address3)(mode |           rn << 16 | rm );
  }

  // addressing mode 4 - load and store multiple
  //
  // usage: tos                             = {tos_val, tos_tag}
  //        set(r0)                         = {r0}
  //        set(r0, r4)                     = {r0, r4}
  //        range(r0, r4))                  = {r0 - r4}

  enum Address4 {
    // interpreter usage conventions
    emptySet = 0,
    tos   = 1 << tos_val | (TaggedJavaStack ? 1 << tos_tag : 0),
    tmp01 = 1 << tmp0    | (TaggedJavaStack ? 1 << tmp1    : 0),
    tmp23 = 1 << tmp2    | (TaggedJavaStack ? 1 << tmp3    : 0),
    tmp45 = 1 << tmp4    | (TaggedJavaStack ? 1 << tmp5    : 0),
    forceaddress4=0x10000000  // force Address4 to be int size
  };


  static Address4 set(Register reg) {
    return (Address4)(1 << reg);
  }

  static Address4 set(Register reg1, Register reg2) {
    GUARANTEE(reg1 < reg2, "Address4 ordering");
    return (Address4)((1 << reg1) | (1 << reg2));
  }

  static Address4 set(Register reg1, Register reg2, Register reg3) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3, "Address4 ordering");
    return (Address4)((1 << reg1) | (1 << reg2) | (1 << reg3));
  }

  static Address4 set(Register reg1, Register reg2,
                      Register reg3, Register reg4) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3 && reg3 < reg4, "Address4 ordering");
    return (Address4)((1 << reg1) | (1 << reg2) | (1 << reg3) | (1 << reg4));
  }

  static Address4 range(Register beg, Register end) {
    GUARANTEE(beg <= end, "illegal range");
    return (Address4)((1 << (end + 1)) - (1 << beg));
  }

  static Address4 join(Address4 set1, Address4 set2) {
#ifdef AZZERT
    GUARANTEE((set1 & set2) == 0, "Address4 sets are disjoint");
    GUARANTEE(set1 != 0 && set2 != 0, "Empty sets");
    for (int i = 0; i < number_of_registers; i++) {
      if ((1 << i) & set2) {
        // This is the first bit set in set2
        GUARANTEE(set1 < (1 << i), "set1 must be smaller than set2");
        break;
      }
    }
#endif
    return (Address4)(set1 | set2);
  }

  static Address4 join(Address4 set1, Address4 set2, Address4 set3) {
    return join(join(set1, set2), set3);
  }

  static Address4 join(Address4 set1, Address4 set2,
                       Address4 set3, Address4 set4) {
    return join(join(join(set1, set2), set3), set4);
  }

  // addressing mode 5 - coprocessor
  enum Address5 {
    forceaddress5=0x10000000  // force Address3 to be int size
  };

  static Address5 imm_index5(Register rn, int offset_8 = 0, Mode mode = offset)
  {
    GUARANTEE(rn != r15 || mode == offset, "unpredictable instruction");
    GUARANTEE(offset_8 % 4 == 0, "Offset must be multiple of 4");
    check_imm(abs(offset_8 >> 2), 8);
    if (mode == post_indexed) {
      // I don't know why these is different for coprocessors
      mode = (Mode)(1 << 21);
    }
    return (Address5)(mode | (up(offset_8) << 23) | rn << 16 |
                      abs(offset_8>>2) & 0xff);
  }

  static Address5 unindexed5(Register /*rn*/, int options) {
    check_imm(options, 8);
    // The "sign" bit is required to be set.
    return (Address5) ((1 << 23) | options);
  }

  // move data-processing instructions (form 1)
  enum CCMode { no_CC, set_CC }; // position and order is relevant!

 protected:
  void arith(Opcode opcode, Register rd, Register rn, Address1 shifter_op,
                  CCMode s = no_CC, Condition cond = al) {
    GUARANTEE(rd <= r15 && rn <= r15, "Invalid register used");
    // Note: The immediate bit is provided via the shifter operand!
    emit(cond << 28 | opcode << 21 | s << 20 | rn << 16 |
         rd << 12 | shifter_op);
  }

  void mulop(Register rd, Register rn, Register rs, Register rm,
             int lua, CCMode s = no_CC, Condition cond = al) {
    GUARANTEE(rd <= r15 && rn <= r15 && rs <= r15 && rm <= r15, "Invalid register used");
    // Note: rd & rn are exchanged for these instructions!
    emit(cond << 28 | lua << 21 | s << 20 | rd << 16 | rn << 12
             | rs << 8 | 0x9 << 4 | rm);
  }

 public:
#define F(mnemonic, opcode) \
  void mnemonic(Register rd, Address1 shifter_op,        \
                CCMode s = no_CC, Condition cond = al) { \
    arith(opcode, rd, sbz, shifter_op, s, cond);         \
  }                                                      \
  void mnemonic(Register rd, Address1 shifter_op, Condition cond) { \
    arith(opcode, rd, sbz, shifter_op, no_CC, cond);     \
  }

  F(mov, _mov)
  F(mvn, _mvn)
#undef F

  // compare & test data-processing instructions (form 2)
#define F(mnemonic, opcode) \
  void mnemonic(Register rn, Address1 shifter_op, Condition cond = al) { \
    arith(opcode, sbz, rn, shifter_op, set_CC, cond); \
  }
  F(tst, _tst)
  F(teq, _teq)
  F(cmp, _cmp)
  F(cmn, _cmn)
#undef F

  // 3-operand data-processing instructions (form 3)
#define F(mnemonic, opcode) \
  void mnemonic(Register rd, Register rn, Address1 shifter_op, \
                CCMode s = no_CC, Condition cond = al) {       \
    arith(opcode, rd, rn, shifter_op, s, cond);                \
  }                                                            \
  void mnemonic(Register rd, Register rn, Address1 shifter_op, \
                Condition cond) {                              \
    arith(opcode, rd, rn, shifter_op, no_CC, cond);            \
  }

  F(andr, _andr)               // and is a keyword on some machines
  F(eor, _eor)
  F(sub, _sub)
  F(rsb, _rsb)
  F(add, _add)
  F(adc, _adc)
  F(sbc, _sbc)
  F(rsc, _rsc)
  F(orr, _orr)
  F(bic, _bic)
#undef F

  // load & store word or unsigned byte instructions
#define F(mnemonic, l, b) \
  void mnemonic(Register rd, Address2 addr, Condition cond = al) {    \
    GUARANTEE(rd <= r15, "Invalid register used");                     \
    emit(cond << 28 | 1 << 26 | b << 22 | l << 20 | rd << 12 | addr); \
  }
  F(ldr , 1, 0)
  F(ldrb, 1, 1)
  F(str , 0, 0)
  F(strb, 0, 1)
#undef F

  // miscellaneous load and store instructions
#define F(mnemonic, l, sh) \
  void mnemonic(Register rd, Address3 addr, Condition cond = al) { \
    GUARANTEE(rd <= r15, "Invalid register used");                  \
    emit(cond << 28 | l << 20 | rd << 12 | 1 << 7 | sh << 5 | 1 << 4 | addr); \
  }
  F(strh , 0, 1)
  F(ldrd , 0, 2)
  F(strd , 0, 3)
  F(ldrh , 1, 1)
  F(ldrsb, 1, 2)
  F(ldrsh, 1, 3)
#undef F

  // multiple load & store instructions
  enum WritebackMode { no_writeback, writeback }; // order is relevant!

  // only used in our macros
  enum StackDirectionMode { ascending, descending };
  enum StackTypeMode { full, empty };

#if ENABLE_THUMB_VM
#define F(mnemonic, l, pu) \
  void mnemonic(Register rn, Address4 reg_set,                                \
                WritebackMode w = no_writeback, Condition cond = al) {        \
    GUARANTEE(reg_set != 0 && ((reg_set & set(rn)) == 0 || w == no_writeback),\
              "unpredictable instruction");                                   \
    GUARANTEE((reg_set & set(pc)) == 0 || l == 0,                             \
              "we should not load pc to avoid jumps without mode switching"); \
    check_imm(reg_set, 16);                                                   \
    emit(cond << 28 | 0x4 << 25 | pu << 23 | w << 21 | l << 20 | rn << 16 |   \
         reg_set);                                                            \
  }

#else
#define F(mnemonic, l, pu) \
  void mnemonic(Register rn, Address4 reg_set,                                \
                WritebackMode w = no_writeback, Condition cond = al) {        \
    GUARANTEE(reg_set != 0 && ((reg_set & set(rn)) == 0 || w == no_writeback),\
              "unpredictable instruction");                                   \
    check_imm(reg_set, 16);                                                   \
    emit(cond << 28 | 0x4 << 25 | pu << 23 | w << 21 | l << 20 | rn << 16 |   \
         reg_set);                                                            \
  }
#endif

  // non-stack         stack       addressing modes
  F(ldmda, 1, 0)   F(ldmfa, 1, 0)
  F(ldmia, 1, 1)   F(ldmfd, 1, 1)
  F(ldmdb, 1, 2)   F(ldmea, 1, 2)
  F(ldmib, 1, 3)   F(ldmed, 1, 3)
  F(stmda, 0, 0)   F(stmed, 0, 0)
  F(stmia, 0, 1)   F(stmea, 0, 1)
  F(stmdb, 0, 2)   F(stmfd, 0, 2)
  F(stmib, 0, 3)   F(stmfa, 0, 3)
#undef F

  // multiply instructions
  void mul(Register rd, Register rm, Register rs,
           CCMode s = no_CC, Condition cond = al) {
    mulop(rd, sbz, rs, rm, 0x0, s, cond);
  }

  void mla(Register rd, Register rm, Register rs, Register rn,
           CCMode s = no_CC, Condition cond = al) {
    mulop(rd, rn, rs, rm, 0x1, s, cond);
  }

#define F(mnemonic, lua) \
  void mnemonic(Register rdlo, Register rdhi, Register rm, Register rs,  \
                CCMode s = no_CC, Condition cond = al) { \
    mulop(rdhi, rdlo, rs, rm, lua, s, cond); \
  }
  F(umull, 0x4)
  F(umlal, 0x5)
  F(smull, 0x6)
  F(smlal, 0x7)
#undef F

  // miscellaneous instructions
  void nop()               { orr(r0, r0, reg(r0)); }

  // swap instructions
#define F(mnemonic, b) \
  void mnemonic(Register rd, Register rm, Register rn, Condition cond = al) { \
    emit(cond << 28 | 0x2 << 23 | b << 22 | rn << 16 | rd << 12  \
             | 0x9 << 4 | rm);                                   \
  }
  F(swp , 0)
  F(swpb, 1)
#undef F

  enum StatusRegister {
    cpsr = 0,
    spsr = 1
  };

  enum FieldMask {
    c_mask = 1 << 0,
    x_mask = 1 << 1,
    s_mask = 1 << 2,
    f_mask = 1 << 3
  };

  void mrs(Register rd, StatusRegister sreg, Condition cond = al) {
      emit(cond << 28 | 0x010F0000 | (rd << 12) | (sreg << 22));
  }

  void msr(StatusRegister sreg, int imm32, Condition cond = al) {
    int field = f_mask;         // required
    emit(cond << 28 | 0x0120f000 | (sreg << 22) | (field << 16) | imm(imm32));
  }

  void msr(StatusRegister sreg, int field, Register rm, Condition cond = al) {
    check_imm(field, 4);
    emit(cond << 28 | 0x0120F000 | (sreg << 22) | (field << 16) | reg(rm));
  }

  void clz(Register rd, Register rm, Condition cond = al) {
    emit(cond << 28 | 0x016f0f10 | rd << 12 | rm);
  }

  void bx(Register rm, Condition cond = al) {
    emit(cond << 28 | 0x012fff10 | rm);
  }

  void blx(Register rm, Condition cond = al) {
    emit(cond << 28 | 0x012fff30 | rm);
  }

  void bl(int offset, Condition cond = al) {
    GUARANTEE(abs(offset) <= 0x003ffffff, "branch offset limit");
    emit(cond << 28 | 0x0b000000 | ((offset & 0x3ffffff) >> 2));
  }

  enum CRegister {
    // position and order is relevant!
    c0, c1, c2 , c3 , c4 , c5 , c6 , c7 ,
    c8, c9, c10, c11, c12, c13, c14, c15
  };

  enum Coprocessor {
    p0, p1, p2 , p3 , p4 , p5 , p6 , p7 ,
    p8, p9, p10, p11, p12, p13, p14, p15
  };

#define F(mnemonic, mnemonic2, l) \
  void mnemonic(Coprocessor coproc, int opcode1, Register rd,      \
                CRegister crn, CRegister crm,                      \
                int opcode2 = 0, Condition cond = al) {            \
    check_imm(opcode1, 3);                                         \
    check_imm(opcode2, 3);                                         \
    emit(  (cond << 28) + 0x0E000000 + (opcode1 << 21) + (l << 20) \
         + (crn << 16) + (rd << 12) + (coproc << 8)                \
         + (opcode2 << 5) + (1 << 4) + (crm << 0));                \
  }                                                                \
  void mnemonic2(Coprocessor coproc, int opcode1, Register rd, \
                CRegister crn, CRegister crm,                      \
                int opcode2 = 0) {                                 \
    mnemonic(coproc, opcode1, rd, crn, crm, opcode2, nv);          \
  }
  F(mcr, mcr2, 0)
  F(mrc, mrc2, 1)
#undef F


#define F(mnemonic, mnemonic2, l) \
  void mnemonic(Coprocessor coproc, CRegister crd,            \
                Address5 address, Condition cond = al) {      \
    emit( (cond << 28) + (6 << 25) + (l << 20) + (crd << 12)  \
        + (coproc << 8) + address);                           \
  }                                                           \
  void mnemonic2(Coprocessor coproc, CRegister crd,       \
                Address5 address) {                           \
    mnemonic(coproc, crd, address, nv);                       \
  }
  F(stc, std2, 0)
  F(ldc, ldc2, 1)
#undef F

   void cdp(Coprocessor coproc, int opcode1,
            CRegister crd, CRegister crn, CRegister crm,
            int opcode2 = 0, Condition cond = al) {
    check_imm(opcode1, 4);
    check_imm(opcode2, 3);
     emit(   (cond << 28) + (0xE << 24) + (opcode1 << 20) + (crn << 16)
           + (crd << 12) + (coproc << 8) + (opcode2 << 5) + crm);
  }

  void cdp2(Coprocessor coproc, int opcode1,
            CRegister crd, CRegister crn, CRegister crm,
            int opcode2 = 0) {
    cdp(coproc, opcode1, crd, crn, crm, opcode2, nv);
  }

#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
  // To support Xscale WMMX instructions
  #include "Assembler_wmmx.hpp"
#endif

  void pld(Address2 addr) {
    GUARANTEE((addr & mode_flags) == offset, "must be offset");
    emit(0xF450F000 | addr);
  }

#if ENABLE_ARM_VFP
   // sn <- rd
   void fmsr(Register sn, Register rd, Condition cond = al) {
     jint n = sn - s0;
     jint Fn = n >> 1;   // top 4 bits
     jint N  = n & 0x01; // bottom bit
     emit(cond << 28 | 0xe0 << 20 | Fn << 16 | rd << 12 |
          0x0a << 8 | N << 7 | 0x10);
   }
   // rd <- sn
   void fmrs(Register rd, Register sn, Condition cond = al) {
     jint n = sn - s0;
     jint Fn = n >> 1;   // top 4 bits
     jint N  = n & 0x01; // bottom bit
     emit(cond << 28 | 0xe1 << 20 | Fn << 16 | rd << 12 |
          0x0a << 8 | N << 7 | 0x10);
   }
   // rd <- system reg
   void fmrx(Register rd, VFPSystemRegister reg, Condition cond = al) {
     emit(cond << 28 | 0xef << 20 | reg << 16 | rd << 12 |
          0x0a << 8 | 0x10);
   }
   // system reg <- rd
   void fmxr(VFPSystemRegister reg, Register rd, Condition cond = al) {
     emit(cond << 28 | 0xee << 20 | reg << 16 | rd << 12 |
          0x0a << 8 | 0x10);
   }

   // dm <- rd rn
   void fmdrr(Register sm, Register rd, Register rn, Condition cond = al) {
     juint m = sm - s0;
     jint  Fm = m >> 1;    // top 4 bits
     jint  M  = m & 0x01;  // bottom bits
     emit(cond << 28 | 0xc4 << 20 | rn << 16 | rd << 12 |
          0x0b << 8 | M << 5 | 0x01 << 4 | Fm);
   }
   // rd rn <- dm
   void fmrrd(Register rd, Register rn, Register sm, Condition cond = al) {
     juint m = sm - s0;
     jint  Fm = m >> 1;    // top 4 bits
     jint  M  = m & 0x01;  // bottom bits
     emit(cond << 28 | 0xc5 << 20 | rn << 16 | rd << 12 |
          0x0b << 8 | M << 5 | 0x01 << 4 | Fm);
   }

   void fmstat(Condition cond = al) {
     emit(cond << 28 | 0xef << 20 | 0x01 << 16 | 0x0f << 12 |
          0x0a << 8 | 0x10);
   }

#define F(mnemonic, Fn, N, cpnum) \
  void mnemonic(Register sd, Register sm, Condition cond = al) { \
     jint d = sd - s0; \
     jint m = sm - s0; \
     jint Fd = d >> 1;   /* top 4 bits */ \
     jint Fm = m >> 1;   /* top 4 bits */ \
     jint D  = d & 0x01; /* bottom bit */ \
     jint M  = m & 0x01; /* bottom bit */ \
     \
     emit(cond << 28 | 0x1d << 23 | D << 22 | 3 << 20 | Fn << 16 | Fd << 12 | \
          cpnum << 8 | N << 7 | 1 << 6 | M << 5 | Fm); \
   }
   F(fcpys,   0x0, 0, 10) // sd =  sm
   F(fabss,   0x0, 1, 10) // sd =  abs(sm)
   F(fnegs,   0x1, 0, 10) // sd = -sm
   F(fsqrts,  0x1, 1, 10) // sd =  sqrt(sm)
   F(fcmps,   0x4, 0, 10)
   F(fcmpes,  0x4, 1, 10)
   F(fcmpzs,  0x5, 0, 10)
   F(fcmpezs, 0x5, 1, 10)
   F(fcvtds,  0x7, 1, 10)
   F(fuitos,  0x8, 0, 10)
   F(fsitos,  0x8, 1, 10)
   F(ftouis,  0xc, 0, 10)
   F(ftouizs, 0xc, 1, 10)
   F(ftosis,  0xd, 0, 10)
   F(ftosizs, 0xd, 1, 10)

   F(fcpyd,   0x0, 0, 11) // sd =  sm
   F(fabsd,   0x0, 1, 11) // sd =  abs(sm)
   F(fnegd,   0x1, 0, 11) // sd = -sm
   F(fsqrtd,  0x1, 1, 11) // sd =  sqrt(sm)
   F(fcmpd,   0x4, 0, 11)
   F(fcmped,  0x4, 1, 11)
   F(fcmpzd,  0x5, 0, 11)
   F(fcmpezd, 0x5, 1, 11)
   F(fcvtsd,  0x7, 1, 11)
   F(fuitod,  0x8, 0, 11) 
   F(fsitod,  0x8, 1, 11)
   F(ftouid,  0xc, 0, 11)
   F(ftouizd, 0xc, 1, 11)
   F(ftosid,  0xd, 0, 11)
   F(ftosizd, 0xd, 1, 11)

#undef F

#define F(mnemonic, p, q, r, s, cpnum) \
   void mnemonic(Register sd, Register sn, Register sm, Condition cond = al) {\
     sd = Register(sd - s0); \
     sm = Register(sm - s0); \
     sn = Register(sn - s0); \
     jint Fd = sd >> 1;   /* top 4 bits */ \
     jint Fm = sm >> 1;   /* top 4 bits */ \
     jint Fn = sn >> 1;   /* top 4 bits */ \
     jint D  = sd & 0x01; /* bottom bit */ \
     jint M  = sm & 0x01; /* bottom bit */ \
     jint N  = sn & 0x01; /* bottom bit */ \
     \
     emit(cond << 28 | 0x0e << 24 | p << 23 | D << 22 | q << 21 | \
          r << 20 | Fn << 16 |  \
          Fd << 12 | cpnum << 8 | N << 7 | s << 6 | M << 5 | Fm); \
   }
   F(fmacs,  0, 0, 0, 0, 10)
   F(fnmacs, 0, 0, 0, 1, 10)
   F(fmscs,  0, 0, 1, 0, 10)
   F(fnmscs, 0, 0, 1, 1, 10)
   F(fmuls,  0, 1, 0, 0, 10)
   F(fnmuls, 0, 1, 0, 1, 10)
   F(fadds,  0, 1, 1, 0, 10)
   F(fsubs,  0, 1, 1, 1, 10)
   F(fdivs,  1, 0, 0, 0, 10)

   F(fmacd,  0, 0, 0, 0, 11)
   F(fnmacd, 0, 0, 0, 1, 11)
   F(fmscd,  0, 0, 1, 0, 11)
   F(fnmscd, 0, 0, 1, 1, 11)
   F(fmuld,  0, 1, 0, 0, 11)
   F(fnmuld, 0, 1, 0, 1, 11)
   F(faddd,  0, 1, 1, 0, 11)
   F(fsubd,  0, 1, 1, 1, 11)
   F(fdivd,  1, 0, 0, 0, 11)
#undef F

  // Place holder for flds to use a 12 bit offset during compilation.
  // The place holder will be replaced by flsd when the literal is bound.
  // This way prevents us from using fldd to access the literal pool
  enum Address5_stub {
    forceaddress5_stub=0x10000000  // force Address3 to be int size
  };

  static Address5_stub imm_index5_stub(Register rn, int offset_12) {
    GUARANTEE(offset_12 % 4 == 0, "Offset must be multiple of 4");
    check_imm(abs(offset_12 >> 2), 12);
    return (Address5_stub)(offset | (up(offset_12) << 23) | rn << 16 | abs(offset_12>>2));
  }
  
  void flds_stub(Register sd, Address5_stub address5, Condition cond = al) {
    sd = Register(sd - s0); 
    jint Fd = sd >> 1;   /* top 4 bits */ 
    jint D  = sd & 0x01; /* bottom bit */

    emit(cond << 28 | 0x06 << 25 | D << 22 | 
        1 << 20 | Fd << 12 | 0 << 8 | address5); 
  }


#define F(mnemonic, L, cpnum) \
   void mnemonic(Register sd, Address5 address5, Condition cond = al) { \
     sd = Register(sd - s0); \
     jint Fd = sd >> 1;   /* top 4 bits */ \
     jint D  = sd & 0x01; /* bottom bit */ \
     \
     emit(cond << 28 | 0x06 << 25 | 1 << 24 | D << 22 | \
          L << 20 | Fd << 12 | cpnum << 8 | address5); \
   }

   F(flds, 1, 10)
   F(fsts, 0, 10)
   F(fldd, 1, 11)
   F(fstd, 0, 11)
#undef F

#define F(mnemonic, P, U, L, cpnum) \
   void mnemonic(Register rn, Register beg, int size, WritebackMode w = no_writeback, Condition cond = al) { \
     beg = Register(beg - s0); \
     jint Fd = beg >> 1;   /* top 4 bits */ \
     jint D  = beg & 0x01; /* bottom bit */ \
     \
     GUARANTEE(size != 0 && beg >= 0 && beg+size <= number_of_float_registers, "Invalid Register List"); \
     emit(cond << 28 | 0x06 << 25 | P << 24 | U << 23 | D << 22 | w << 21 | \
          L << 20 | rn << 16 | Fd << 12 | cpnum << 8 | size); \
   }

   // Non stack                // stack
   F(fldmiad, 0, 1, 1, 11)   F(fldmfdd, 0, 1, 1, 11)
   F(fldmias, 0, 1, 1, 10)   F(fldmfds, 0, 1, 1, 10)
   F(fldmdbd, 1, 0, 1, 11)   F(fldmead, 1, 0, 1, 11)
   F(fldmdbs, 1, 0, 1, 10)   F(fldmeas, 1, 0, 1, 10)
   F(fstmiad, 0, 1, 0, 11)   F(fstmead, 0, 1, 0, 11)
   F(fstmias, 0, 1, 0, 10)   F(fstmeas, 0, 1, 0, 10)
   F(fstmdbd, 1, 0, 0, 11)   F(fstmfdd, 1, 0, 0, 11)
   F(fstmdbs, 1, 0, 0, 10)   F(fstmfds, 1, 0, 0, 10)
#undef F

#endif


  void swi(int imm_24, Condition cond = al) {
    check_imm(imm_24, 24);
    emit(cond << 28 | 0xf << 24 | imm_24);
  }

  void breakpoint(Condition cond = al) {
    // IMPL_NOTE: we need to make this portable on platforms that may
    // use a different method of breakpoints. Make sure the fix works
    // with cross compilation -- remember the loop generator is compiled
    // with the host compiler!
#ifdef UNDER_CE
    emit(0x1200070 | (cond << 28));
#else
    swi(0x9f0001, cond);
#endif
  }
#endif /* !PRODUCT || ENABLE_COMPILER */
};

class Macros;

class LiteralAccessor {
#if !defined(PRODUCT) || ENABLE_COMPILER
public:
  virtual bool has_literal(int /*imm32*/, Assembler::Address1& /*result*/) {
    return false;
  }
  virtual Assembler::Register get_literal(int /*imm32*/) {
    return Assembler::no_reg;
  }
  virtual void free_literal() {}
#endif
};

// The Macros class implements frequently used instruction sequences
// or macros that can be shared between the source and binary assembler.

class Macros: public Assembler {

#if !defined(PRODUCT) || ENABLE_COMPILER

 private:
  bool is_mul_imm_simple(Register rd, Register rm, int imm32,
                         CCMode s, Condition cond);

  bool is_twoword_immediate(int imm32, Address1& result1, Address1& result2);

  bool arith2_imm(Opcode opcode, Register rd, Register rn, int imm32,
                  CCMode s, Condition cond);

  void arith4_imm(Opcode opcode, Register rd, Register rn, int imm32,
                  CCMode s, Condition cond);

 public:
  // immediate operands for move data-processing instructions (form 1)
  void mov_imm(Register rd, int imm32, CCMode s = no_CC, Condition cond=al) {
    arith_imm(_mov, rd, sbz, imm32, NULL, s, cond);
  }
  void mov_imm(Register rd, int imm32, Condition cond) {
    arith_imm(_mov, rd, sbz, imm32, NULL, no_CC, cond);
  }
  void mov_imm(Register rd, int imm32,
               LiteralAccessor* la, CCMode s = no_CC, Condition cond=al) {
    arith_imm(_mov, rd, sbz, imm32, la, s, cond);
  }

  // immediate operands for compare & test data-processing instructions (form 2)
#define F(mnemonic, opcode) \
  void mnemonic(Register rn, int imm32, LiteralAccessor* la,    \
                Condition cond = al) {                          \
    arith_imm(opcode, sbz, rn, imm32, la, set_CC, cond);        \
  }

  F(tst_imm, _tst)
  F(teq_imm, _teq)
  F(cmp_imm, _cmp)
#undef F

  // immediate operands for 3-operand data-processing instructions (form 3)
#define F(mnemonic, opcode) \
  void mnemonic(Register rd, Register rn, int imm32,                      \
                CCMode s = no_CC, Condition cond = al) {                  \
    arith_imm(opcode, rd, rn, imm32, NULL, s, cond);                      \
  }                                                                       \
  void mnemonic(Register rd, Register rn, int imm32, LiteralAccessor* la, \
                CCMode s = no_CC, Condition cond = al) {                  \
    arith_imm(opcode, rd, rn, imm32, la, s, cond);                        \
  }

  F(andr_imm, _andr)
  F(eor_imm,  _eor)
  F(sub_imm,  _sub)
  F(rsb_imm,  _rsb)
  F(add_imm,  _add)
  F(adc_imm,  _adc)
  F(sbc_imm,  _sbc)
  F(rsc_imm,  _rsc)
  F(orr_imm,  _orr)
  F(bic_imm,  _bic)
#undef F

  void arith_imm(Opcode opcode, Register rd, Register rn, int imm32,
                 LiteralAccessor* la, CCMode s = no_CC, Condition cond = al);

  // immediate operands for multiplication
  void mul_imm(Register rd, Register rm, int imm32, Register tmp,
               CCMode s = no_CC, Condition cond = al);

  // register moves (do nothing if registers are equal, unless condition
  // codes are required)
  void mov_reg(Register rd, Register rs, CCMode s = no_CC, Condition cond = al);

  // bit manipulations
  void oop_write_barrier(Register dst, const Register tmp1, Register tmp2,
                         Register tmp3, bool bounds_check);

  NOT_PRODUCT(virtual) void
      get_bitvector_base(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_2_PARAM(rd, cond));
  NOT_PRODUCT(virtual) void
      get_bit_selector(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_2_PARAM(rd, cond));
  NOT_PRODUCT(virtual) void
      ldr_big_integer(Register rd, int imm32, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_3_PARAM(rd, imm32, cond));
  NOT_PRODUCT(virtual) void
      get_heap_start(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_2_PARAM(rd, cond));
  NOT_PRODUCT(virtual) void
      get_heap_top(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_2_PARAM(rd, cond));
  NOT_PRODUCT(virtual) void
      get_old_generation_end(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL_2_PARAM(rd, cond));

  NOT_PRODUCT(virtual) void comment(const char* /*fmt*/, ...) {}

#endif
};

#endif /*#if !ENABLE_THUMB_COMPILER*/
