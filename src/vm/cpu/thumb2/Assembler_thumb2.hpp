/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

// The Assembler class provides minimal functionality to generate
// machine code. All functions representing machine instructions
// generate exactly one instruction (no optimizations!).

class PostIndex {
public:
  int value;
  PostIndex(int p_value) {value = p_value;};
};

class PreIndex {
public :
  int value;
  PreIndex(int p_value) {value = p_value;};
};

class Assembler: public AssemblerCommon {
#if !PRODUCT || ENABLE_COMPILER || USE_COMPILER_STRUCTURES
 public:
  enum Register {
#include "../arm/Register_armthumb.hpp"
#if ENABLE_ARM_V7
    tos_val =  r0,
    tos_tag =  r1,
    tmp0    =  r2,
    tmp1    =  r3,
    tmp3    =  r4,
    tmp2    =  r5,
    tmp4    =  r6,
    bcode   =  r7,
    tmp5    =  r7,
    locals  =  r8,
    jsp     =  r9,  // Accessed via ldr_r9 and str_r9
    gp      =  r10, // Accessed via ldr_r10
    fp      =  r11,
    cpool   =  r12,
    sp      =  r13, // FIXED BY ARM CALLING CONVENTION
    bcp     =  r14,
    lr      =  r14,
    pc      =  r15, // FIXED BY HARDWARE
#else
    tos_val =  r0,
    tos_tag =  r1,
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
#endif
  };
#include "../arm/AssemblerCommon_armthumb.hpp"

#if !PRODUCT
private:
  int current_it_scope_depth;
public:
  Assembler() : current_it_scope_depth(0) {
  }
#endif // !PRODUCT

protected:
  void set_in_it_scope(int new_depth) PRODUCT_RETURN;
  bool is_in_it_scope() PRODUCT_RETURN0;
  void decrease_current_it_depth() PRODUCT_RETURN;

 protected:
  // The implementation of emit is assembler specific: the source assembler
  // will disassemble the instruction and emit the corresponding assembly
  // source code, the binary assembler will emit the binary instruction.
  //
  // In product mode, only one implementation (the one for the binary
  // assembler) exists, and then the call is statically bound. In all
  // other modes, this is a virtual call so we can have both the text
  // and binary output with the same build.

  NOT_PRODUCT(virtual) void
    emit(short instr) NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
    emit_int(int instr) NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
    emit_w(int instr) NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
    emit_2w(short hw1, short hw2) NOT_PRODUCT(JVM_PURE_VIRTUAL);

 public:
  enum {
    ARM_CODE,
    THUMB_CODE,
    THUMB2_CODE    = THUMB_CODE,
    THUMB2EE_CODE,
  };

  // The Imm12 type is used to force you to call imm12() or modified_imm12(),
  // instead of calculating the imm12 bits manually (which is error prone)
  enum Imm12 {
    invalid_imm12 = 0xffffffff
  };

  enum RegisterSet {
    // interpreter usage conventions
    emptySet = 0,
    tos   = 1 << tos_val,
    tmp01 = 1 << tmp0   ,
    tmp23 = 1 << tmp2   ,
    tmp45 = 1 << tmp4   ,
    forceRegisterSet=0x10000000  // force Address4 to be int size
  };

  // addressing mode 2 - load and store word or unsigned byte
  enum Address2 {
    forceaddress2=0x10000000  // force Address2 to be int size
  };

  static Address2 imm_index(int offset) {
    check_imm(abs(offset), 12);
    return (Address2)(offset);
  }

  static Address2 post_index(int offset) {
    check_imm(offset, 8);
    int U = offset >= 0 ? 1 : 0;
    offset = abs(offset);
    return (Address2)(0x9 << 8 | U << 9 | offset);
  }

  static Address2 pre_index(int offset) {
    check_imm(offset, 8);
    int U = offset >= 0 ? 1 : 0;
    offset = abs(offset);
    return (Address2)(0xD << 8 | U << 9 | offset);
  }

  // Optional parameter for IT instruction
  // to support IT blocks of more than one instruction
  enum ConditionMask {
    SINGLE         = 0x8,
    THEN           = 0x4,
    ELSE           = 0xc,
    THEN_THEN      = 0x2,
    ELSE_THEN      = 0xa,
    THEN_ELSE      = 0x6,
    ELSE_ELSE      = 0xe,
    THEN_THEN_THEN = 0x1,
    ELSE_THEN_THEN = 0x9,
    THEN_ELSE_THEN = 0x5,
    ELSE_ELSE_THEN = 0xd,
    THEN_THEN_ELSE = 0x3,
    ELSE_THEN_ELSE = 0xb,
    THEN_ELSE_ELSE = 0x7,
    ELSE_ELSE_ELSE = 0xf
  };

  static bool is_c_saved_register(Register x) {
    return x >= r4 && x != r12 && x != r14;
  }

  static Imm12 imm12(int value) {
    GUARANTEE(0 <= value && value <= 0xff, "sanity");
    return (Imm12)value;
  }
  static Imm12 modified_imm12(int seven_bits, int ror_count);
  static Imm12 modified_imm12(int seven_bits, int ror_count, int checksum) {
    GUARANTEE(((seven_bits + 0x80) << (32 - ror_count)) == checksum, "sanity");
    return modified_imm12(seven_bits, ror_count);
  }

  enum {
   lsl_shift_1 = 1,
   lsl_shift_2 = 2,
   lsl_shift_3 = 3
  };

 // only used in our macros
  enum StackDirectionMode { ascending, descending };
  enum StackTypeMode { full, empty };
  enum WritebackMode { no_writeback, writeback };

  // support for indices/offsets (make sure these get inlined!)
  static int abs(int x)   { return x < 0 ? -x : x; }
  static int up (int x)   { return x < 0 ?  0 : 1; }

  static Register reg(Register rm) { return rm; }

  static RegisterSet set(Register reg) {
    return (RegisterSet)(1 << reg);
  }

  static RegisterSet set(Register reg1, Register reg2) {
    GUARANTEE(reg1 < reg2, "Invalid register set ordering");
    return (RegisterSet)((1 << reg1) | (1 << reg2));
  }

  static RegisterSet set(Register reg1, Register reg2,
                            Register reg3) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3,
              "Invalid register set ordering");
    return (RegisterSet)((1 << reg1) | (1 << reg2) |
                            (1 << reg3));
  }

  static RegisterSet set(Register reg1, Register reg2,
                      Register reg3, Register reg4) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3 && reg3 < reg4,
               "Invalid register set ordering");
    return (RegisterSet)((1 << reg1) | (1 << reg2) |
                            (1 << reg3) | (1 << reg4));
  }

  static RegisterSet range(Register beg, Register end) {
    GUARANTEE(beg <= end, "illegal range");
    return (RegisterSet)((1 << (end + 1)) - (1 << beg));
  }

  static RegisterSet join(RegisterSet set1, RegisterSet set2) {
    return (RegisterSet)(set1 | set2);
  }

  // move data-processing instructions
  enum CCMode { no_CC=0, set_CC=1, any_CC=2 };

 public:

   /***THUMB INSTRUCTION SET***************************************/
#define F(mnemonic, opcode)                               \
  void mnemonic(Register rd, Register rm) {               \
    GUARANTEE(rd < r8 && rm < r8, "invalid register");    \
    emit(1 << 14 | opcode << 6 | rm << 3 | rd);           \
  }
  F(mvn, _mvn)
  F(cmp, _cmp)
  F(cmn, _cmn)
  F(tst, _tst)
  F(adc, _adc)
  F(sbc, _sbc)
  F(neg, _neg)
  F(mul, _mul)
  F(lsl, _lsl)
  F(lsr, _lsr)
  F(asr, _asr)
  F(ror, _ror)
  F(andr, _and)
  F(eor, _eor)
  F(orr, _orr)
  F(bic, _bic)
#undef F


  void mov(Register rd, Register rm) {
    emit(7 << 10 | rm << 3 | rd);
  }

#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, int imm) {                     \
     GUARANTEE(rd < r8, "invalid register");                \
     check_imm(imm, 8);                                     \
     emit(1 << 13 | opcode << 11 | rd << 8 | imm);          \
  }
  F(mov_imm8, 0x0)
  F(cmp_imm8, 0x1)
  F(add_imm8, 0x2)
  F(sub_imm8, 0x3)
#undef F

#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rm, int imm) {        \
    GUARANTEE(rd < r8 && rm < r8, "invalid register");      \
    check_imm(imm, 5);                                      \
    emit((opcode-2) << 11 | imm << 6 | rm << 3 | rd);       \
  }
  F(lsl_imm5, _lsl)
  F(lsr_imm5, _lsr)
  F(asr_imm5, _asr)
#undef F

  // add, sub register instructions
#define F(mnemonic, a)                                             \
  void mnemonic(Register rd, Register rn, Register rm) {           \
     GUARANTEE(rd < r8 && rn < r8 && rm < r8, "invalid register"); \
     emit(6 << 10 | a << 9 | rm << 6 | rn << 3 | rd);              \
  }
  F(add, 0)
  F(sub, 1)
#undef F

#define F(mnemonic, a)                                      \
  void mnemonic(Register rd, Register rn, int imm = 0) {    \
    check_imm(abs(imm), 3);                                 \
    GUARANTEE(rd < r8 && rn < r8, "Invalid immediate");     \
      emit(0x7 << 10 | a << 9 | abs(imm) << 6               \
                     | rn << 3 | rd);                       \
  }
  F(add_imm3, 0)
  F(sub_imm3, 1)
#undef F

  // add, cmp, mov high register instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rm) {                 \
    GUARANTEE(rd > r7 || rm > r7,                           \
              "Unpredictable instruction H1==0 and H2==0"); \
    emit(17 << 10 | opcode << 8 | ((rd & 0x8) >> 3) << 7 |  \
                    rm << 3 | rd & 0x7);                    \
  }
  F(add_hi, 0x0)
  F(cmp_hi, 0x1)
  F(mov_hi, 0x2)
#undef F

  // add high register 8 bit immediate instructions
#define F(mnemonic, r)                                      \
  void mnemonic(Register rd, int imm8) {                    \
    emit(10 << 12 | r << 11 | rd << 8 | imm8);              \
  }
  F(add_imm8_cmn, 0)
  F(sub_imm8_cmn, 1)
#undef F

  // add & sub sp 7 bit immediate instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(int imm) {               \
      emit(11 << 12 | opcode << 7 | abs(imm));             \
  }
  F(add_sp_imm7x4, 0)
  F(sub_sp_imm7x4, 1)
#undef F

 // add PC +  8 bit immediate instructions
#define F(mnemonic)                                         \
  void mnemonic(Register rd, int imm) {                     \
    GUARANTEE(has_room_for_imm(imm, 8),                     \
     "add <Rd>, pc, imm - Invalid immediate");              \
      emit(5 << 13 | rd << 8 | abs(imm));                   \
  }
  F(add_pc_imm8x4)
#undef F

// load offset instructions
#define F(mnemonic, l, b)                                   \
  void mnemonic(Register rd, int offset = 0) {              \
    GUARANTEE(rd < r8, "invalid register");                 \
    check_imm(abs(offset), 8);                              \
    emit(9 << 12 | l << 11 | rd << 8 | abs(offset));        \
  }
  F(ldr_sp_imm8x4,  1, 0)
  F(str_sp_imm8x4,  0, 0)
#undef F

// load offset instructions
#define F(mnemonic, l, b)                                   \
  void mnemonic(Register rd, const int offset = 0) {        \
     check_imm(offset, 8);                                  \
     emit(4 << 12 | l << 11 | rd << 8 | offset);            \
  }
  F(ldr_pc_imm8x4,  1, 0)
#undef F

  void ldr_pc_stub( Register rd, const int offset ) {
    check_imm( abs(offset), 10 );
    emit( short(1 << 14 | rd << 11 | (offset & 0x7FF)) );
  }

// load offset instructions
#define F(mnemonic, l, b)                                   \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    GUARANTEE(rd < r8, "invalid register");                 \
    GUARANTEE(rn < r8, "invalid register");                 \
    emit(3 << 13 | b << 12 | l << 11 |                      \
                 (offset) << 6 | rn << 3 | rd);             \
  }
  F(ldr_imm5x4,  1, 0)
  F(ldrb_imm5, 1, 1)
  F(str_imm5x4,  0, 0)//untested
  F(strb_imm5, 0, 1)//untested
#undef F


  // load register instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    GUARANTEE(rd < r8 && rn < r8 && rm < r8,                \
              "invalid register");                          \
    emit(5 << 12 | opcode << 9 | rm << 6 | rn << 3 | rd);   \
  }
  F(ldrsh, 0x7)
  F(ldrh,  0x5)
  F(ldrsb, 0x3)
#undef F

  // load register instructions
#define F(mnemonic, b)                                      \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    GUARANTEE(rd < r8 && rn < r8 && rm < r8,                \
               "invalid register");                         \
    emit(5 << 12 | 1 << 11 | b << 10 | rm << 6 |            \
         rn << 3 | rd);                                     \
  }
  F(ldr,  0)
  F(ldrb, 1)
#undef F

// store register instructions
#define F(mnemonic, b)                                      \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    GUARANTEE(rd < r8 && rn < r8 && rm < r8,                \
               "invalid register");                         \
    emit(5 << 12 | b << 9 | rm << 6 |                       \
         rn << 3 | rd);                                     \
  }
  F(str,  0)
  F(strh, 1)
#undef F

  void strb(Register rd, Register rn, Register rm) {
    emit(0x15 << 10 | rm << 6 | rn << 3 | rd);
  }

  void ldrh_imm5x2(Register rd, Register rn, int offset = 0) {
    check_imm(offset, 5);
    GUARANTEE(rd < r8 && rd < r8, "invalid register");
    emit(8 << 12 | 1 << 11 | offset << 6 | rn << 3 | rd);
  }

  void strh_imm5x2(Register rd, Register rn, int offset = 0) {
    check_imm(offset, 5);
    GUARANTEE(rd < r8 && rn < r8, "invalid register");
    emit(1 << 15 | offset << 6 | rn << 3 | rd);
  }

#define F(mnemonic, l)                                      \
  void mnemonic(Register rn, int reg_set) {                 \
    GUARANTEE(rn < r8, "invalid register");                 \
    emit(12 << 12 | l << 11 | rn << 8 | reg_set);           \
  }
  F(ldmia, 1)
  F(stmia, 0)
#undef F

#define F(mnemonic, w, u, v, l)                             \
  void mnemonic(Register rn, int reg_set) {                 \
    GUARANTEE(has_room_for_imm(reg_set, 16),                \
      "Wrong register set");                                \
    emit_w(14 << 28 | 1 << 27 | v << 24| u << 23 |          \
      w << 21 | l << 20 | rn << 16 | reg_set);              \
  }
  F(ldmia_w,      0, 1, 0, 1)
  F(stmia_w,      0, 1, 0, 0)
  F(ldmdb_w,      0, 0, 1, 1)
  F(stmdb_w,      0, 0, 1, 0)
  F(ldmia_post_w, 1, 1, 0, 1)
  F(stmia_post_w, 1, 1, 0, 0)
  F(ldmdb_post_w, 1, 0, 1, 1)
  F(stmdb_post_w, 1, 0, 1, 0)
#undef F

  // Push a set of registers to the C stack. E.g., push_sp(range(r1, r2));
  // Note that it's called push_via_sp() to distinguish with
  // SourceMacros::push(), which pushes onto the Java stack
#define F(mnemonic, ispop, R)                               \
  void mnemonic(int set) {                                  \
    emit(0x5a << 9 | ispop << 11 | R << 8 | set);           \
  }                                                         \
  void mnemonic(Register rn) {                              \
    mnemonic(set(rn));                                      \
  }
  F(push_via_sp,   0, 0)
  F(pushlr_via_sp, 0, 1)
  F(pop_via_sp,    1, 0)
  F(poplr_via_sp,  1, 1)
#undef F
  // miscellaneous instructions
  void nop() {
    emit(0xBF00);
  }

  void bx(Register rm) {
    emit(0x8E << 7 | rm << 3);
  }

  void blx(Register rm) {
    emit(0x8F << 7 | rm << 3);
  }

#if ENABLE_ARM_VFP
  #define SINGLE_ARG_VFP_COND   ) { const Condition cond = al;
  #define VFP_COND              SINGLE_ARG_VFP_COND
  #define VFP_EMIT              emit_w
  #include "../arm/Assembler_vfp.hpp"

  void flds_stub(const Register sd, const int offset) {
    GUARANTEE( is_vfp_register(sd), "VFP register expected" );
    check_imm( abs(offset), 10 );
    emit_w( (int(offset & 0x7FF) << 16) | int(sd - s0) );
  }
#endif

  void swi(int imm_8) {
    check_imm(imm_8, 8);
    emit(0xdf << 8 | imm_8);
  }

  void breakpoint(Condition cond = al) {
    // IMPL_NOTE: we need to make this portable on platforms
    // that may use a different method of breakpoints.
    if (cond != al) {
      it(cond);
    }
    swi(0xFF); // Faking swi here
  }

  NOT_PRODUCT(virtual) void
    ldr_big_integer(Register rd, int imm32, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  //----------------------------------------------------------------------
  // Thumb-2 instructions
  //----------------------------------------------------------------------
#define OOXYOOXY_FORMAT (1 << 8)
#define XYOOXYOO_FORMAT (2 << 8)
#define XYXYXYXY_FORMAT (3 << 8)

  bool encode_imm(unsigned int p_value, int& result) {
    if (p_value < 256) {
      result = p_value;
      return true;
    }
    if ((p_value >> 16) == (p_value & 0xFFFF)) {
      int first_8_bits = p_value & 0xFF;
      int second_8_bits = (p_value >> 8) & 0xFF;
      if (p_value & 0xFF00FF00 == 0) {
        result = first_8_bits | OOXYOOXY_FORMAT;
        return true;
      } else if (p_value & 0x00FF00FF == 0) {
        result = second_8_bits | XYOOXYOO_FORMAT;
        return true;
      } else {
        const unsigned first_16_bits = p_value & 0xFFFF;
        if((first_16_bits >> 8) == (first_16_bits & 0xFF)) {
          result = first_8_bits | XYXYXYXY_FORMAT;
          return true;
        }
      }
    }
    //check if it could be shifted 8-bits value
    int low_bit = -1;
    int hi_bit = -1;
    int bit_num;
    int temp_value = p_value;
    for (bit_num = 0; bit_num < 32; bit_num++) {
      if (temp_value & 1) {
        if (low_bit == -1) low_bit = bit_num;
        hi_bit = bit_num;
      }
      temp_value >>= 1;
    }
    if (hi_bit - low_bit < 8) {
      int shift = (32 + 7 - hi_bit);
      result = (((unsigned int)p_value >> (hi_bit - 7)) & ~0x80) | shift << 7;
      return true;
    }
    return false;
  }

#define MASK_11   0x1 << 11
#define MASK_8_10 0x7 << 8
#define MASK_0_7  0xFF

#define ENCODE_RD rd << 8
#define ENCODE_IMM (int(imm) & MASK_11)   << 15| \
                   (int(imm) & MASK_8_10) << 4 | \
                   (int(imm) & MASK_0_7)
#define ENCODE_RN rn  << 16
// Data procesing with immediate constant
#define F(mnemonic, op)                                                   \
  void mnemonic(Register rd, Register rn, Imm12 imm, CCMode S = no_CC) {  \
    check_imm(imm, 12);                                                   \
    emit_w(0x1e << 27 | op << 21 | S << 20 |                              \
           ENCODE_RD | ENCODE_RN | ENCODE_IMM );                          \
  }
  F(adc_imm12_w,  0xA)
  F(add_imm12_w,  0x8)
  F(and_imm12_w,  0x0)
  F(bic_imm12_w,  0x1)
  F(eor_imm12_w,  0x4)
  F(orn_imm12_w,  0x3)
  F(orr_imm12_w,  0x2)
  F(rsb_imm12_w,  0xE)
  F(sbc_imm12_w,  0xB)
  F(sub_imm12_w,  0xD)
#undef F

#define F(mnemonic, op, S)                                                \
  void mnemonic(Register rn, Imm12 imm) {                                 \
    check_imm(imm, 12);                                                   \
    emit_w(0x1e << 27 | op << 21 | S << 20 | 0xF << 8 |                   \
          ENCODE_RN | ENCODE_IMM );                                       \
  }
  F(cmn_imm12_w,  0x8, 1)
  F(cmp_imm12_w,  0xD, 1)
  F(teq_imm12_w,  0x4, 1)
  F(tst_imm12_w,  0x0, 1)
#undef F

// mov rd, #12-bit immediate
#define F(mnemonic, op)                                                   \
  void mnemonic(Register rd, Imm12 imm, CCMode S = no_CC) {               \
    check_imm(imm, 12);                                                   \
    GUARANTEE(rd != pc, "pc is not allowed as argument");                 \
    emit_w(0x1e << 27 | op << 21 | S << 20 | 0xF << 16 |                  \
          ENCODE_RD | ENCODE_IMM );                                       \
  }
  F(mov_imm12_w, 0x2)
  F(mvn_imm12_w, 0x3)
#undef F

//unmodified 12bit constants are used here!
#define F(mnemonic, op1, op2)                                             \
  void mnemonic(Register rd, Register rn, int imm) {                      \
    check_imm(imm, 12);                                                   \
    emit_w(0x1e << 27 | 0x1 << 25 | op1 << 23 |                           \
    op2 << 20 | ENCODE_RN | ENCODE_RD | ENCODE_IMM );                     \
  }
  F(addw_imm12_w, 0, 0)
  F(subw_imm12_w, 1, 2)
#undef F

#define MASK_12_15 (0xF << 12)
#define ENCODE_IMM_16 (imm & MASK_12_15) << 4 | (imm & MASK_11) << 15 | (imm & MASK_8_10) << 4 | (imm & MASK_0_7)

#define F(mnemonic, op1, op2)                                             \
  void mnemonic(Register rd, int imm) {                                   \
    check_imm(imm, 16);                                                   \
    emit_w(0x1e << 27 | 0x1 << 25 | op1 << 23 |                           \
      1 << 22 | op2 << 20 | ENCODE_RD | ENCODE_IMM_16 );                  \
  }
  F(movt_imm12_w, 1, 0)
  F(movw_imm12_w, 0, 0)
#undef F

#define ENCODE_LSB(lsb) (lsb & 0x7 << 2) << 10 | (lsb & 0x3) << 6

  void bfc_w(Register rd, int lsb, int width) {
    check_imm(lsb, 5);
    check_imm(width + lsb - 1, 5);
    emit_w(0x1e << 27 | 0x3 << 24 | 0x3 << 21 | \
      0xF << 16 | ENCODE_RD | ENCODE_LSB(lsb) | (width + lsb - 1));
  }

  void bfi_w(Register rd, Register rn, int lsb, int width) {
    check_imm(lsb, 5);
    check_imm(width + lsb - 1, 5);
    emit_w(0x1e << 27 | 0x3 << 24 | 0x3 << 21 | \
      ENCODE_RN | ENCODE_RD | ENCODE_LSB(lsb) | (width + lsb - 1));
  }

  void sbfx_w(Register rd, Register rn, int lsb, int width, int is_signed = 0) {
    check_imm(lsb, 5);
    check_imm(width - 1, 5);
    emit_w(0x1e << 27 | 0x3 << 24 | is_signed << 23 | 2 << 21 | \
      ENCODE_RN | ENCODE_RD | ENCODE_LSB(lsb) | (width- 1));
  }

  void ubfx_w(Register rd, Register rn, int lsb, int width) {
    sbfx_w(rd, rn, lsb, width, 1);
  }

  void ssat_w(Register rd, int imm, Register rn,
              Shift shift = lsl_shift, int shift_count = 0, int is_signed = 0) {
    check_imm(imm, 5);
    GUARANTEE(shift == lsl_shift || shift == asr_shift, "only this shifts are supported");
    emit_w(0x1e << 27 | 0x3 << 24 | is_signed << 23 | shift << 20 | \
      ENCODE_RN | ENCODE_RD | ENCODE_LSB(imm) | shift_count);
  }

  void usat_w(Register rd, int imm, Register rn, Shift shift = lsl_shift, int shift_count = 0) {
    ssat_w(rd, imm, rn, shift, shift_count , 1);
  }

  void ssat16_w(Register rd, int imm, Register rn, int is_signed = 0) {
    check_imm(imm - 1, 5);
    emit_w(0x1e << 27 | 0x3 << 24 | is_signed << 23 | 0x1 << 21 | \
      ENCODE_RN | ENCODE_RD | (imm - 1));
  }

  void usat16_w(Register rd, int imm, Register rn) {
    ssat16_w(rd, imm, rn, 1);
  }

#define ENCODE_SHIFT_COUNT(shift_count) \
    (shift_count & (0x7 << 2)) << 10 | (shift_count & 0x3) << 6

// Data processing with register shift
#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rn, Register rm, \
                    Shift shift = lsl_shift,\
                    int shift_count = 0, CCMode S = no_CC) { \
    GUARANTEE(rd != pc, "see specification") \
    emit_w(0x1D << 27 | 1 << 25 | op << 21 | S << 20 \
      | ENCODE_RN | ENCODE_RD | rm | ENCODE_SHIFT_COUNT(shift_count) \
      | shift << 4); \
  }
  F(adc_w,  0xA)
  F(add_w,  0x8)
  F(and_w,  0x0)
  F(bic_w,  0x1)
  F(eor_w,  0x4)
  F(orn_w,  0x3)
  F(orr_w,  0x2)
  F(rsb_w,  0xE)
  F(sbc_w,  0xB)
  F(sub_w,  0xD)
#undef F

#define F(mnemonic, op, S) \
  void mnemonic(Register rn, Register rm, Shift shift = lsl_shift, int shift_count = 0) {    \
    emit_w(0x1D << 27 | 1 << 25 | op << 21 | S << 20 \
      | ENCODE_RN | 0xF << 8| rm | ENCODE_SHIFT_COUNT(shift_count) | shift << 4); \
  }
  F(cmn_w, 0x8, 1)
  F(cmp_w, 0xD, 1)
  F(teq_w, 0x4, 1)
  F(tst_w, 0x0, 1)
#undef F

// LDR [PC, +-imm12]
#define F(mnemonic, S, size) \
  void mnemonic(Register rxf, int offset) {    \
    check_imm(abs(offset), 12); \
    GUARANTEE(rxf != 0xF || (S == 0 && size == 2), "sanity"); \
    const int U = offset >= 0 ? 1 : 0; \
    emit_w(0x7C << 25 | S << 24 | U << 23 | size << 21 | 1 << 20 \
            | 0xF << 16 | rxf << 12 | abs(offset)); \
  }
  F(ldrb_pc_imm12_w,  0, 0)
  F(ldrsb_pc_imm12_w, 1, 0)
  F(ldrh_pc_imm12_w,  0, 1)
  F(ldrsh_pc_imm12_w, 1, 1)
  F(ldr_pc_imm12_w,   0, 2)
#undef F

 // LDR/STR [Rn, +imm12], or LDR [Rn, -imm8]
#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, int offset = 0) {    \
    GUARANTEE((offset <= 0xFFF) && (offset >= -0xFF), "Wrong offset"); \
    GUARANTEE(rxf != 0xF || (S == 0 && size == 2), "sanity"); \
    int U = offset >= 0 ? 1 : 0; \
    offset = offset >= 0 ? offset : (-offset | 0xC00); \
    emit_w(0x7C << 25 | S << 24 | U << 23 | size << 21 | L << 20 \
            | rn << 16 | rxf << 12 | offset); \
  }
  F(ldrb_imm12_w,  0, 0, 1)
  F(ldrsb_imm12_w, 1, 0, 1)
  F(ldrh_imm12_w,  0, 1, 1)
  F(ldrsh_imm12_w, 1, 1, 1)
  F(ldr_imm12_w,   0, 2, 1)
  F(strb_imm12_w,  0, 0, 0)
  F(strh_imm12_w,  0, 1, 0)
  F(str_imm12_w,   0, 2, 0)
#undef F

// LDR/STR [Rn, +-imm8, post-indexed]
#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, PostIndex offset) { \
    check_imm(abs(offset.value), 8); \
    GUARANTEE(rxf != 0xF || (S == 0 && size == 2), "sanity"); \
    int U = (offset.value >= 0) ? 1 : 0; \
    int offset8 = abs(offset.value); \
    emit_w(0x7C << 25 | S << 24 | size << 21 | L << 20 \
            | rn << 16 | rxf << 12 | 0x9 << 8 | U << 9 | offset8); \
  }
  F(ldrb_post_imm12_w,  0, 0, 1)
  F(ldrsb_post_imm12_w, 1, 0, 1)
  F(ldrh_post_imm12_w,  0, 1, 1)
  F(ldrsh_post_imm12_w, 1, 1, 1)
  F(ldr_post_imm12_w,   0, 2, 1)
  F(strb_post_imm12_w,  0, 0, 0)
  F(strh_post_imm12_w,  0, 1, 0)
  F(str_post_imm12_w,   0, 2, 0)
#undef F

// LDR/STR [Rn, +-imm8, pre-indexed]
#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, PreIndex offset) {    \
    check_imm(abs(offset.value), 8); \
    GUARANTEE(rxf != 0xF || (S == 0 && size == 2), "sanity"); \
    int U = (offset.value >= 0) ? 1 : 0; \
    int offset8 = abs(offset.value); \
    emit_w(0x7C << 25 | S << 24 | size << 21 | L << 20 \
            | rn << 16 | rxf << 12 | 0xD << 8 | U << 9 | offset8); \
  }
  F(ldrb_pre_imm12_w,  0, 0, 1)
  F(ldrsb_pre_imm12_w, 1, 0, 1)
  F(ldrh_pre_imm12_w,  0, 1, 1)
  F(ldrsh_pre_imm12_w, 1, 1, 1)
  F(ldr_pre_imm12_w,   0, 2, 1)
  F(strb_pre_imm12_w,  0, 0, 0)
  F(strh_pre_imm12_w,  0, 1, 0)
  F(str_pre_imm12_w,   0, 2, 0)
#undef F

// LDR/STR [Rn, RM << lshift]
#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, Register rm, int lshift) { \
    check_imm(lsl_shift, 2); \
    GUARANTEE(rxf != 0xF || (S == 0 && size == 2), "sanity"); \
    emit_w(0x7C << 25 | S << 24 | size << 21 | L << 20 \
           | rn << 16 | rxf << 12 | lshift << 4 | rm); \
  }
  F(ldrb_w,  0, 0, 1)
  F(ldrsb_w, 1, 0, 1)
  F(ldrh_w,  0, 1, 1)
  F(ldrsh_w, 1, 1, 1)
  F(ldr_w,   0, 2, 1)
  F(strb_w,  0, 0, 0)
  F(strh_w,  0, 1, 0)
  F(str_w,   0, 2, 0)
#undef F

#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, int offset) {    \
    check_imm(offset, 8); \
    GUARANTEE(rxf != 0xF, "see documentation"); \
    emit_w(0x7C << 25 | S << 24 | size << 21 | L << 20 \
            | rn << 16 | rxf << 12 | 0xE << 8 | offset); \
  }
  F(ldrbt_imm12_w,  0, 0, 1)
  F(ldrsbt_imm12_w, 1, 0, 1)
  F(ldrht_imm12_w,  0, 1, 1)
  F(ldrsht_imm12_w, 1, 1, 1)
  F(ldrt_imm12_w,   0, 2, 1)
  F(strbt_imm12_w,  0, 0, 0)
  F(strht_imm12_w,  0, 1, 0)
  F(strt_imm12_w,   0, 2, 0)
#undef F

#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rm, Shift shift = lsl_shift,      \
                    int shift_count = 0, CCMode S = no_CC) {            \
    GUARANTEE(rd != pc, "pc is not allowed as argument");               \
    emit_w(0x1D << 27 | 1 << 25 | op << 21 | S << 20                    \
      | ENCODE_RD | 0xF << 16 | rm | ENCODE_SHIFT_COUNT(shift_count)    \
      | shift << 4); \
  }
  F(mov_w, 0x2)
  F(mvn_w, 0x3)
#undef F

#define F(mnemonic, shift_type) \
  void mnemonic(Register rd, Register rn, Register rm, int shift_count = 0) {    \
    emit_w(0x1D << 27 | 1 << 25 | 0x6 << 21 \
      | ENCODE_RN | ENCODE_RD | rm | ENCODE_SHIFT_COUNT(shift_count) | shift_type << 4); \
  }
  F(pkhbt_w, 0x0)
  F(pkhtb_w, 0x2)
#undef F

#define F(mnemonic, op1, S) \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    emit_w(0x1F << 27 | 1 << 25 | 0xF << 12 | S << 20 \
      | ENCODE_RN | ENCODE_RD | rm | op1 << 21); \
  }
  F(lsl_w,  0x0, 0)
  F(lsls_w, 0x0, 1)
  F(lsr_w,  0x1, 0)
  F(lsrs_w, 0x1, 1)
  F(asr_w,  0x2, 0)
  F(asrs_w, 0x2, 1)
  F(ror_w,  0x3, 0)
  F(rors_w, 0x3, 1)
#undef F


#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rn, Register rm, int rotate = 0) {    \
    check_imm(rotate, 2); \
    emit_w(0x1F4 << 23 | op << 20 | 0xF << 12 \
      | ENCODE_RN | ENCODE_RD | rm | 1 << 7 | rotate << 4); \
  }
  F(sxtab_w,   0x4)
  F(sxtab16_w, 0x2)
  F(sxtah_w,   0x0)
  F(uxtab_w,   0x5)
  F(uxtab16_w, 0x3)
  F(uxtah_w,   0x1)
#undef F

#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rm, int rotate = 0) {    \
    check_imm(rotate, 2); \
    emit_w(0x1F4 << 23 | op << 20 | 0xF << 12 \
      | 0xF << 16| ENCODE_RD | rm | 1 << 7 | rotate << 4); \
  }
  F(sxtb_w,   0x4)
  F(sxtb16_w, 0x2)
  F(sxth_w,   0x0)
  F(uxtb_w,   0x5)
  F(uxtb16_w, 0x3)
  F(uxth_w,   0x1)
#undef F

//SIMD instuctions
#define F(mnemonic, op, prefix) \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    emit_w(0x1F5 << 23 | op << 20 | 0xF << 12 \
      | ENCODE_RN | ENCODE_RD | rm | prefix << 4); \
  }
  F(sadd16_w    ,   0x1, 0x0)
  F(sadd8_w     ,   0x0, 0x0)
  F(saddsubx_w  ,   0x2, 0x0)
  F(ssub16_w    ,   0x5, 0x0)
  F(ssub8_w     ,   0x4, 0x0)
  F(ssubaddx_w  ,   0x6, 0x0)

  F(qadd16_w    ,   0x1, 0x1)
  F(qadd8_w     ,   0x0, 0x1)
  F(qaddsubx_w  ,   0x2, 0x1)
  F(qsub16_w    ,   0x5, 0x1)
  F(qsub8_w     ,   0x4, 0x1)
  F(qsubaddx_w  ,   0x6, 0x1)

  F(shadd16_w   ,   0x1, 0x2)
  F(shadd8_w    ,   0x0, 0x2)
  F(shaddsubx_w ,   0x2, 0x2)
  F(shsub16_w   ,   0x5, 0x2)
  F(shsub8_w    ,   0x4, 0x2)
  F(shsubaddx_w ,   0x6, 0x2)

  F(uadd16_w    ,   0x1, 0x4)
  F(uadd8_w     ,   0x0, 0x4)
  F(uaddsubx_w  ,   0x2, 0x4)
  F(usub16_w    ,   0x5, 0x4)
  F(usub8_w     ,   0x4, 0x4)
  F(usubaddx_w  ,   0x6, 0x4)

  F(uqadd16_w   ,   0x1, 0x5)
  F(uqadd8_w    ,   0x0, 0x5)
  F(uqaddsubx_w ,   0x2, 0x5)
  F(uqsub16_w   ,   0x5, 0x5)
  F(uqsub8_w    ,   0x4, 0x5)
  F(uqsubaddx_w ,   0x6, 0x5)

  F(uhadd16_w   ,   0x1, 0x6)
  F(uhadd8_w    ,   0x0, 0x6)
  F(uhaddsubx_w ,   0x2, 0x6)
  F(uhsub16_w   ,   0x5, 0x6)
  F(uhsub8_w    ,   0x4, 0x6)
  F(uhsubaddx_w ,   0x6, 0x6)
#undef F

#define F(mnemonic, op1, op2) \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    emit_w(0x1F5 << 23 | op1 << 20 | 0xF << 12 \
      | ENCODE_RN | ENCODE_RD | rm | op2 << 4 | 1 << 7); \
  }
  F(qadd_w , 0x0, 0x0)
  F(qdadd_w, 0x0, 0x1)
  F(qsub_w , 0x0, 0x2)
  F(qdsub_w, 0x0, 0x3)
  F(sel_w  , 0x2, 0x0)
#undef F

#define F(mnemonic, op1, op2) \
  void mnemonic(Register rd, Register rm) {    \
    emit_w(0x1F5 << 23 | op1 << 20 | 0xF << 12 \
      | ENCODE_RD | rm | (rm << 16) | op2 << 4 | 1 << 7); \
  }
  F(rev_w  , 0x1, 0x0)
  F(rev16_w, 0x1, 0x1)
  F(rbit_w , 0x1, 0x2)
  F(revsh_w, 0x1, 0x3)
  F(clz_w  , 0x3, 0x0)
#undef F

#define ENCODE_RACC racc << 12

#define F(mnemonic, op1, op2) \
  void mnemonic(Register rd, Register rn, Register rm, Register racc) {    \
    emit_w(0x1F6 << 23 | op1 << 20 | ENCODE_RN \
      | ENCODE_RACC | ENCODE_RD | op2 << 4 | rm ); \
  }
  F(mla_w   ,   0x0, 0x0)
  F(mls_w   ,   0x0, 0x1)
  F(smlabb_w,   0x1, 0x0)
  F(smlabt_w,   0x1, 0x1)
  F(smlatb_w,   0x1, 0x2)
  F(smlatt_w,   0x1, 0x3)
  F(smlad_w ,   0x2, 0x0)
  F(smladx_w,   0x2, 0x1)
  F(smlawb_w,   0x3, 0x0)
  F(smlawt_w,   0x3, 0x1)
  F(smlsd_w ,   0x4, 0x0)
  F(smlsdx_w,   0x4, 0x1)
  F(smmla_w ,   0x5, 0x0)
  F(smmlar_w,   0x5, 0x1)
  F(smmls_w ,   0x6, 0x0)
  F(smmlsr_w,   0x6, 0x1)
  F(usada8_w,   0x7, 0x0)
#undef F

#define F(mnemonic, op1, op2) \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    emit_w(0x1F6 << 23 | op1 << 20 | ENCODE_RN \
      | 0xF << 12 | ENCODE_RD | op2 << 4 | rm ); \
  }
  F(mul_w   ,   0x0, 0x0)
  F(smmul_w ,   0x5, 0x0)
  F(smmulr_w,   0x5, 0x1)
  F(smuad_w ,   0x2, 0x0)
  F(smuadx_w,   0x2, 0x1)
  F(smulbb_w,   0x1, 0x0)
  F(smulbt_w,   0x1, 0x1)
  F(smultb_w,   0x1, 0x2)
  F(smultt_w,   0x1, 0x3)
  F(smulwb_w,   0x3, 0x0)
  F(smulwt_w,   0x3, 0x1)
  F(smusd_w ,   0x4, 0x0)
  F(smusdx_w,   0x4, 0x1)
  F(usad8_w ,   0x7, 0x0)
#undef F

#define F(mnemonic, op1, op2) \
  void mnemonic(Register rd_lo, Register rd, Register rn, Register rm) {    \
  emit_w(0x1F7 << 23 | op1 << 20 | ENCODE_RN \
    | rd_lo << 12 | ENCODE_RD | op2 << 4 | rm ); \
  }
  F(smull_w  ,   0x0, 0x0)
  F(umull_w  ,   0x2, 0x0)
  F(udiv_w   ,   0x3, 0xF)
  F(smlal_w  ,   0x4, 0x0)
  F(smlalbb_w,   0x4, 0x8)
  F(smlalbt_w,   0x4, 0x9)
  F(smlaltb_w,   0x4, 0xA)
  F(smlaltt_w,   0x4, 0xB)
  F(smlald_w ,   0x4, 0xC)
  F(smlaldx_w,   0x4, 0xD)
  F(smlsld_w ,   0x5, 0xC)
  F(smlsldx_w,   0x5, 0xD)
  F(umlal_w  ,   0x6, 0x0)
  F(umaal_w  ,   0x6, 0x6)
#undef F

  void sdiv_w(Register rd, Register rn, Register rm) {
    emit_w(0x1F7 << 23 | 1 << 20 | rn << 16
      | 0xF << 12 | rd << 8 | 0xF << 4 | rm );
  }

#define F(mnemonic, L) \
  void mnemonic(Register rxf, Register rxf2, Register rn, PreIndex offset, int write_back) {    \
    check_imm(abs(offset.value), 8); \
    GUARANTEE(!(write_back & ~0x1), " should be 0 or 1"); \
    int U = offset.value > 0 ? 1 : 0; \
    emit_w(0x74 << 25 | 1 << 24 | U << 23 | 1 << 22 | write_back << 21 | L << 20 \
            | rn << 16 | rxf << 12 | rxf2 << 8 | abs(offset.value)); \
  }
  F(ldrd_w,  1)
  F(strd_w,  0)
#undef F

#define F(mnemonic, L) \
  void mnemonic(Register rxf, Register rxf2, Register rn, PostIndex offset) {    \
    check_imm(abs(offset.value), 8); \
    int U = offset.value > 0 ? 1 : 0; \
    emit_w(0x74 << 25 | 1 << 24 | U << 23 | 1 << 22 | 1 << 21 | L << 20 \
            | rn << 16 | rxf << 12 | rxf2 << 8 | abs(offset.value)); \
  }
  F(ldrd_w,  1)
  F(strd_w,  0)
#undef F

#define F(mnemonic, L) \
  void mnemonic(Register rxf, Register rn, int offset) {    \
    check_imm(offset, 8); \
    emit_w(0x74 << 25 |  1 << 22 | L << 20 \
            | rn << 16 | rxf << 12 | 0xF << 8 | offset); \
  }
  F(ldrex_w,  1)
  F(strex_w,  0)
#undef F

#define F(mnemonic, op) \
  void mnemonic(Register rxf, Register rn) {    \
    GUARANTEE(rxf != 0xF, " see limits"); \
    GUARANTEE(rn != 0xF, " see limits"); \
    emit_w(0x746 << 21 | 1 << 20 \
            | rn << 16 | rxf << 12 | 0xF << 8 | op << 4 | 0xF); \
  }
  F(ldrexb_w, 0x4)
  F(ldrexh_w, 0x5)
#undef F

#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rxf, Register rn) {    \
    GUARANTEE(rxf != 0xF, " see limits"); \
    GUARANTEE(rn != 0xF, " see limits"); \
    GUARANTEE(rd != 0xF, " see limits"); \
    emit_w(0x746 << 21 \
            | rn << 16 | rxf << 12 | 0xF << 8 | op << 4 | rd); \
  }
  F(strexb_w, 0x4)
  F(strexh_w, 0x5)
#undef F

  void ldrexd_w(Register rxf, Register rxf2, Register rn) {
    GUARANTEE(rxf != 0xF, " see limits");
    GUARANTEE(rn != 0xF, " see limits");
    GUARANTEE(rxf2 != 0xF, " see limits");
    emit_w(0x746 << 21 | 1 << 20\
            | rn << 16 | rxf << 12 | rxf2 << 8 | 0x7 << 4 | 0xF);
  }

  void ldrexd_w(Register rd, Register rxf, Register rxf2, Register rn) {
    GUARANTEE(rxf != 0xF, " see limits");
    GUARANTEE(rn != 0xF, " see limits");
    GUARANTEE(rxf2 != 0xF, " see limits");
    GUARANTEE(rd != 0xF, " see limits");
    emit_w(0x746 << 21 \
            | rn << 16 | rxf << 12 | rxf2 << 8 | 0x7 << 4 | rd);
  }

 #define F(mnemonic, op) \
  void mnemonic(Register rn, Register rm) {    \
    GUARANTEE(rm != 0xF, " see limits"); \
    emit_w(0x746 << 21 | 1 << 20 \
            | rn << 16 | 0xF << 12 | op << 4 | rm); \
  }
  F(tbb_w, 0x0)
  F(tbh_w, 0x1)
#undef F

 #define F(mnemonic, L, UV) \
  void mnemonic(Register rn, unsigned short reg_set, int write_back = 0) {\
    GUARANTEE(!((reg_set & (1 << 14)) && (reg_set & (1 << 15))), " see limits");\
    GUARANTEE(!(reg_set & (1 << 13)), " see limits"); \
    emit_w(0x74 << 25 | UV << 23 | write_back << 21 | L << 20 \
            | rn << 16 | reg_set); \
  }
  F(ldmia_w, 1, 0x1)
  F(ldmdb_w, 1, 0x2)
  F(stmia_w, 0, 0x1)
  F(stmdb_w, 0, 0x2)
#undef F

#define F(mnemonic, UU) \
  void mnemonic(Register rn, int write_back = 0) {    \
    emit_w(0x74 << 25 | UU << 23 | write_back << 21 | 1 << 20 \
            | rn << 16 | 0x3 << 14); \
  }
  F(rfeia_w, 0x3)
  F(rfedb_w, 0x0)
#undef F

#define F(mnemonic, UU) \
  void mnemonic(Register rn, int r13_mode, int write_back = 0) {    \
  check_imm(r13_mode, 5); \
  emit_w(0x74 << 25 | UU << 23 | write_back << 21 \
           | rn << 16 | 0x3 << 14 | r13_mode); \
  }
  F(srsia_w, 0x3)
  F(srsdb_w, 0x0)
#undef F

  void b_short(int offset, Condition cond = al) {
    GUARANTEE(!(offset & 0x1), "Imm not half-word aligned");
    offset = offset - 4; /* Offset from the current insn */
    if (cond == al) {
      GUARANTEE(offset <= 2046  &&
                offset >= -2048, "Imm too large, use b_wide");
      emit(0x7 << 13 | (offset & 0xFFF) >> 1);
    } else {
      GUARANTEE(offset <= 254 &&
                offset >= -256, "Imm too large, use b_wide");
      GUARANTEE(cond != al, "This is a conditional branch!");
      emit(0xD << 12 | cond << 8 | (offset & 0x1FF) >> 1);
    }
  }

#define F(mnemonic, L, A)                                    \
  void mnemonic(int offset) {                         \
    offset = offset - 4; /* Offset from the current insn */  \
    GUARANTEE((offset >> 1) < (1 << 23) &&                   \
              (offset >> 1) >= -(1 << 23), "Imm too large"); \
    GUARANTEE(!(offset & 0x1), "Imm not half-word aligned"); \
    GUARANTEE(A || !(offset & 0x3), " see limits");          \
    const int S = (offset >= 0) ? 0 : 1;                     \
    const int I1 = ((~(offset >> 23) & 0x1) ^ S) & 0x1;      \
    const int I2 = ((~(offset >> 22) & 0x1) ^ S) & 0x1;      \
    emit_w(0xF << 28 | S << 26 | 1 << 15 | I1 << 13 |        \
           I2 << 11 | L << 14 | A << 12 |                    \
           (offset & 0x3FF000) << 4 |                        \
           (offset & 0xFFF) >> 1);                           \
  }
  F(b_w,   0, 1)
  F(bl_w,  1, 1)
  F(blx_w, 1, 0)
#undef F

  void b_w(int offset, Condition cond) {                     \
    if (cond == al) {                                        \
      b_w(offset);                                           \
      return;                                                \
    }                                                        \
    offset = offset - 4; /* Offset from the current insn */  \
    GUARANTEE((offset >> 1) < (1 << 19) &&                   \
              (offset >> 1) >= -(1 << 19), "Imm too large"); \
    GUARANTEE(!(offset & 0x1), "Imm not half-word aligned"); \
    GUARANTEE(cond != al, "This is a conditional branch!");  \
    const int S = (offset >= 0) ? 0 : 1;                     \
    const int J1 = ((offset >> 19) & 0x1);                   \
    const int J2 = ((offset >> 18) & 0x1);                   \
    emit_w(0xF << 28 | S << 26 | J1 << 13 | J2 << 11 |       \
           1 << 15 | cond << 22 |                            \
           (offset & 0x3F000) << 4 | (offset & 0xFFF) >> 1); \
  }

  void smi_w(short imm) {
    emit_w(0xF7F << 20 | (imm & 0xF) << 16 | 1 << 15 \
          | (imm & 0xFF0) | (imm & 0xF000) >> 12 );
  }

  void msr_w(Register rn, bool spsr, int control, int extension, int status,
            int flags) {
    GUARANTEE(!(control & ~0x1), " see limits"); \
    GUARANTEE(!(extension & ~0x1), " see limits"); \
    GUARANTEE(!(status & ~0x1), " see limits"); \
    GUARANTEE(!(flags & ~0x1), " see limits"); \
    emit_w(0xF38 << 20 | rn << 16 | (spsr ? 1 : 0) << 20 | 1 << 15 \
          | control << 11 | extension << 10 | status << 9 | flags << 8);
  }

#define F(mnemonic, effect) \
  void mnemonic(int a_bit, int i_bit, int f_bit, int mode = 0) { \
    GUARANTEE(!(a_bit & ~0x1), " it is bit"); \
    GUARANTEE(!(i_bit & ~0x1), " it is bit"); \
    GUARANTEE(!(f_bit & ~0x1), " it is bit"); \
    GUARANTEE(!(mode & ~0x1F), " see limits"); \
    int M = (mode == 0) ? 0 : 1;\
    emit_w(0xF3A << 20 | 0xF << 16 | 1 << 15 \
          | effect << 9 | M << 8 | a_bit << 7 | i_bit << 6 | f_bit << 5 | mode);\
  }
  F(cpsie_w, 0x2)
  F(cpsid_w, 0x3)
#undef F

  void bxj_w(Register rm) {
    emit_w(0xF3B << 20 | rm << 16 | 1 << 15 | 0xF << 8);
  }

  void msr_w(Register rd, bool spsr) {
    emit_w(0xF3F << 20 | 0xF << 16 | (spsr ? 1 : 0) << 20 | 1 << 15 | rd << 8);
  }

  void bxj_w(int imm8) {
    check_imm(abs(imm8), 8);
    emit_w(0xF3DE8F<< 8 | (imm8 & 0xFF));
  }

#define F(mnemonic, hint) \
  void mnemonic() { \
    emit_w(0xF3AF80 << 8 | hint); \
  }
  F(nop_w,   0x0)
  F(yield_w, 0x1)
  F(wfe_w,   0x2)
  F(wfi_w,   0x3)
  F(sev_w,   0x4)
#undef F

#define F(mnemonic, op) \
  void mnemonic() { \
    emit_w(0xF3BF8F0F  | op < 4); \
  }
  F(clrex_w,   0x2)
  F(dsb_w,     0x4)
  F(dmb_w,     0x5)
  F(isb_w,     0x6)
#undef F

#define F(mnemonic, N) \
  void mnemonic(Register rn, int offset) { \
    GUARANTEE((offset > 0) && !(offset & ~0x7E), " limits"); \
    offset -= 4;\
    emit((short)(0xB1 << 8 | rn | N << 11 | \
            (offset & (0x1 << 7)) << 2 | (offset & (0x3E << 1)) << 1)); \
  }
  F(czbne_w,  0x1)
  F(czbeq_w,  0x0)
#undef F

  // IT cond, mask
  void it(const Condition condition, const ConditionMask mask = SINGLE) {
    if( condition != always ) {
      check_imm(mask, 8);
      int it_scope_size = 1;
      if ((condition & 1) == 1) {
        it_scope_size = 4;
      } else if (condition >> 1 == 1) {
        it_scope_size = 3;
      } else if (condition >> 3 == 1) {
        it_scope_size = 2;
      }
      set_in_it_scope(it_scope_size);
      emit((short)(0xBF << 8 | condition << 4 | ((condition & 1) ? 16 - mask : mask)));
    }
  }

#if ENABLE_ARM_V7
#define F(mnemonic, J) \
  void mnemonic() { \
    emit_w(0xF3BF870F | (J << 4)); \
  }

  F(enterx,  0x1)
  F(leavex,  0x0)
#undef F

#if 0
#define F(mnemonic, J) \
  void mnemonic(Register rm) { \
    emit(0x4700 | J << 7 | rm << 3); \
  }
  F(blx, 1)
  F(bx , 0)
#undef F
#endif

  void chka(Register rn, Register rm) {
    emit(0xCA00 | (rn & 0x7) | (rm << 3) | (rn & 0x8) << 4);
  }

#define F(mnemonic, L) \
  void mnemonic(unsigned int handler_id) {\
    check_imm(handler_id, 8);\
    emit(0xC200 | handler_id | L << 8);\
  }
  F(hb,   0)
  F(hbl,  1)
#undef F

  void hblp(unsigned int handler_id, int param) {
    check_imm(handler_id, 5);
    check_imm(param, 5);
    emit(0xC400 | handler_id | param << 5);
  }

  void hbp(unsigned int handler_id, int param) {
    check_imm(handler_id, 5);
    check_imm(param, 3);
    emit(0xC000 | handler_id | param << 5);
  }

// str rd, [r9, imm6 * 4]
#define F(mnemonic, L) \
  void mnemonic(Register rd, int imm6) { \
    check_imm(imm6, 6); \
    GUARANTEE((rd & ~0x7) == 0, "only general purpose registers are allowed!");\
    emit(0xCC00 | imm6 << 3 | rd| L << 9); \
  }
  F(ldr_r9, 0)
  F(str_r9, 1)
#undef F

  void ldr_r10(Register rd, int imm5) {
    check_imm(imm5, 5);
    GUARANTEE((rd & ~0x7) == 0, "only general purpose registers are allowed!");
    emit(0xCB00 | imm5 << 3 | rd);
  }

  // IMPL_NOTE:refactoring required!!! ldr should be the single function,
  // but it can affect performance.
  void ldr_neg(Register rd, Register rn, int imm3) {
    check_imm(abs(imm3), 3);
    GUARANTEE((rd & ~0x7) == 0, "only general purpose registers are allowed!");
    GUARANTEE((rn & ~0x7) == 0, "only general purpose registers are allowed!");
    emit(0xC800 | abs(imm3) << 6| rn << 3 | rd);
  }
#else  // ENABLE_ARM_V7
  void enterx() {}
  void leavex() {}
#endif // ENABLE_ARM_V7

  enum CRegister {
    // position and order is relevant!
    c0, c1, c2 , c3 , c4 , c5 , c6 , c7 ,
    c8, c9, c10, c11, c12, c13, c14, c15
  };

  enum Coprocessor {
    p0, p1, p2 , p3 , p4 , p5 , p6 , p7 ,
    p8, p9, p10, p11, p12, p13, p14, p15
  };

#define F(mnemonic, l, k) \
  void mnemonic(Coprocessor coproc, int opcode1, Register rd,      \
                CRegister crn, CRegister crm,                      \
                int opcode2 = 0) {                                 \
    check_imm(opcode1, 3);                                         \
    check_imm(opcode2, 3);                                         \
    emit_w(0xEE000000 | (k << 28) | (opcode1 << 21)                \
         | (l << 20) | (crn << 16) | (rd << 12) | (coproc << 8)    \
         | (opcode2 << 5) | (1 << 4) | crm);                       \
  }
  F(mcr_w,  0, 0)
  F(mcr2_w, 0, 1)
  F(mrc_w,  1, 0)
  F(mrc2_w, 1, 1)
#undef F
protected:
  bool try_alloc_tmp(int nregisters = 1);
  Register alloc_tmp_register(bool hi = true);
  void release_tmp_register(Register tmp);
#endif /* !PRODUCT || ENABLE_COMPILER */
};

class Macros;

class LiteralAccessor {
#if !PRODUCT || ENABLE_COMPILER
public:
  virtual bool has_literal(int imm32,
                           Assembler::Register& result) {
    return false;
  }
  virtual Assembler::Register get_literal(int imm32) {
    return Assembler::no_reg;
  }
  virtual void free_literal() {}
#endif
};


// The Macros class implements frequently used instruction
// sequences or macros that can be shared between the source
// and binary assembler.
class Macros: public Assembler {
void add_sub_big_integer(Register rd, Register rn, int imm, bool is_add);
protected:
  Imm12 try_modified_imm12(int big_number);
  Imm12 modified_imm12(int big_number);
public:

  void it(Condition cond, ConditionMask mask = SINGLE) {
    if (cond != al) {
      Assembler::it(cond, mask);
    }
  }

/***ADD ZONE***********************************************************/

// add/sub rd, rn, <imm> instructions
#define F(mnemonic, add_operation)                          \
  void mnemonic(Register rd, Register rn, int imm = 0,      \
      CCMode S = any_CC) {                                  \
    bool is_add = add_operation;                            \
    if (imm < 0) {                                          \
      imm = -imm;                                           \
      is_add = !is_add;                                     \
    }                                                       \
    if (rd == rn && rn == sp && imm % 4 == 0 &&             \
          has_room_for_imm(imm / 4, 7)) {                   \
      if (is_add) {                                         \
        add_sp_imm7x4(imm / 4);                             \
      } else {                                              \
        sub_sp_imm7x4(imm / 4);                             \
      }                                                     \
      return;                                               \
    }                                                       \
    if (has_room_for_imm(imm, 8)) {                         \
      if (rd == rn && rd < r8 && S != no_CC) {              \
        if (is_add) {                                       \
          add_imm8(rd, imm);                                \
        } else {                                            \
          sub_imm8(rd, imm);                                \
        }                                                   \
        return;                                             \
      } else if (has_room_for_imm(imm, 3) &&                \
                 rd < r8 && rn < r8 && S != no_CC) {        \
        if (is_add) {                                       \
          add_imm3(rd, rn, imm);                            \
        } else {                                            \
          sub_imm3(rd, rn, imm);                            \
        }                                                   \
        return;                                             \
      }                                                     \
    }                                                       \
    /* No short instruction helped - try wide one */        \
    if (S == any_CC) {                                      \
      S = no_CC;                                            \
    }                                                       \
    Imm12 modified_imm12 = try_modified_imm12(imm);         \
    if (modified_imm12 != invalid_imm12) {                  \
      if (is_add) {                                         \
        add_imm12_w(rd, rn, modified_imm12, S);             \
      } else {                                              \
        sub_imm12_w(rd, rn, modified_imm12, S);             \
      }                                                     \
    } else {                                                \
      GUARANTEE(S != set_CC, "Invalid CC mode");            \
      add_sub_big_integer(rd, rn, imm, is_add);             \
    }                                                       \
  }
  F(add, 1)
  F(sub, 0)
#undef F

  // Data processing with register shift
#define F(mnemonic, op) \
  void mnemonic(Register rd, Register rn, Register rm, Shift shift= lsl_shift,\
                int shift_count = 0, CCMode S = any_CC) { \
    if (shift_count == 0 && rd < r8 && rn < r8 && rm < r8 && S != set_CC/*IMPL_NOTE:should it be here?*/) { \
      GUARANTEE(S != set_CC || !is_in_it_scope(), \
        "In IT scope 16-bit instructions do not effect CC flags"); \
      Assembler:: mnemonic(rd, rn, rm); \
    } else { \
      if (S == any_CC) { \
        S = no_CC; \
      } \
      Assembler:: mnemonic##_w(rd, rn, rm, shift, shift_count, S); \
    } \
  }

  F(add,  0x8)
  F(sub,  0xD)
#undef F

// IMPL_NOTE: implement proper Macros instructions
#define F(mnemonic) \
  inline void mnemonic(Register rd, int imm) {              \
    Imm12 modified_imm12 = try_modified_imm12(imm); \
    GUARANTEE(modified_imm12 >= 0, "Wrong immediate");      \
    Assembler:: mnemonic##_imm12_w(rd, modified_imm12);     \
  }
  F(teq)
  F(tst)
  F(cmn)
#undef F


  // mov, cmp immediate instructions
  void cmp(Register rd, int imm, Condition cond = al) {
    if (has_room_for_imm(abs(imm), 8) && (rd < r8)) {
      it(cond);
      if (imm >= 0) {
        cmp_imm8(rd, imm);
      } else {
        cmn_imm12_w(rd, imm12(-imm));
      }
    } else {
      Imm12 modified_imm12 = try_modified_imm12(imm);
      if (modified_imm12 != invalid_imm12) {
        it(cond);
        cmp_imm12_w(rd, modified_imm12);
      } else {
        modified_imm12 = try_modified_imm12(-imm);
        if (modified_imm12 != invalid_imm12) {
          it(cond);
          cmn_imm12_w(rd, modified_imm12);
        } else {
          Register rn = alloc_tmp_register(false);
          ldr_big_integer(rn, imm, cond);
          // IMPL_NOTE: eliminate extra it instructions
          it(cond);
          if (rd < r8 && rn < r8) {
            Assembler::cmp(rd, rn);
          } else {
            Assembler::cmp_w(rd, rn);
          }
          release_tmp_register(rn);
        }
      }
    }
  }

  void cmp(Register rd, Register rn, Condition cond = al) {
    it(cond);
    if (rd < r8 && rn < r8) {
      Assembler::cmp(rd, rn);
    } else {
      cmp_hi(rd, rn);
    }

  }

  void mov(Register rd, int imm, Condition cond = al, CCMode S = any_CC) {
    if (has_room_for_imm(abs(imm), 8) && (rd < r8) && (S != no_CC)) {
      if (imm >= 0) {
        it(cond);
        mov_imm8(rd, imm);
      } else {
        it(cond, THEN);
        mov_imm8(rd, abs(imm));
        neg(rd, rd);
      }
    } else {
      if (S == any_CC) {
        S = no_CC;
      }
      Imm12 modified_imm12 = try_modified_imm12(imm);
      if (modified_imm12 != invalid_imm12) {
        it(cond);
        mov_imm12_w(rd, modified_imm12, S);
      } else {
        modified_imm12 = try_modified_imm12(~imm);
        if (modified_imm12 != invalid_imm12) {
          it(cond);
          mvn_imm12_w(rd, modified_imm12, S);
        } else {
          GUARANTEE(S != set_CC, "Shouldn't set CC with such an imm");
          ldr_big_integer(rd, imm, cond);
        }
      }
    }
  }

#define F(mnemonic) \
  void mnemonic(Register rn, Register rm, Shift shift = lsl_shift, int shift_count = 0) {    \
    Assembler:: mnemonic##_w(rn, rm, shift, shift_count); \
  }
  F(teq)
#undef F

  void mov(Register rd, Register rm, Condition cond = al, CCMode S = any_CC) {
    it(cond);
    if (rd < r8 && rm < r8 && S != no_CC) {
      Assembler::mov(rd, rm);
    } else if (rd != pc && ((rd < r8 && rm < r8) || S == set_CC)) {
      if (S == any_CC) {
        S = no_CC;
      }
      Assembler::mov_w(rd, rm, lsl_shift, 0, S);
    } else {
      GUARANTEE(S != set_CC, "mov_hi doesn't change conditional flags");
      GUARANTEE(rd > r7 || rm > r7, "Wrong arguments for mov_hi");
      mov_hi(rd, rm);
    }
  }

/***LDR/STR SECTION*****************************************************************************/
// LDR/STR [Rn, +-imm8, post-indexed]
#define F(mnemonic, S, size, L) \
  inline void mnemonic(Register rxf, Register rn, PostIndex offset) { \
    Assembler:: mnemonic##_post_imm12_w(rxf, rn, offset); \
  }
  F(ldrb,  0, 0, 1)
  F(ldrsb, 1, 0, 1)
  F(ldrh,  0, 1, 1)
  F(ldrsh, 1, 1, 1)
  F(ldr,   0, 2, 1)
  F(strb,  0, 0, 0)
  F(strh,  0, 1, 0)
  F(str,   0, 2, 0)
#undef F

// LDR/STR [Rn, +-imm8, pre-indexed]
#define F(mnemonic, S, size, L) \
  inline void mnemonic(Register rxf, Register rn, PreIndex offset) { \
    Assembler:: mnemonic##_pre_imm12_w(rxf, rn, offset); \
  }
  F(ldrb,  0, 0, 1)
  F(ldrsb, 1, 0, 1)
  F(ldrh,  0, 1, 1)
  F(ldrsh, 1, 1, 1)
  F(ldr,   0, 2, 1)
  F(strb,  0, 0, 0)
  F(strh,  0, 1, 0)
  F(str,   0, 2, 0)
#undef F

// LDR/STR [Rn, RM << lshift]
#define F(mnemonic, S, size, L) \
  void mnemonic(Register rxf, Register rn, Register rm, int lshift) { \
    Assembler:: mnemonic##_w(rxf, rn, rm, lshift); \
  }
  F(ldrb,  0, 0, 1)
  F(ldrsb, 1, 0, 1)
  F(ldrh,  0, 1, 1)
  F(ldrsh, 1, 1, 1)
  F(ldr,   0, 2, 1)
  F(strb,  0, 0, 0)
  F(strh,  0, 1, 0)
  F(str,   0, 2, 0)
#undef F

  void ldr(Register rd, Register rn, int offset = 0, Condition cond = al) {
    GUARANTEE((offset % 4) == 0, "Invalid offset alignment");

    Register rm = rd;
    // IMPL_NOTE: reorder the condition checks
    if(rn == sp && has_room_for_imm(offset / 4, 8) && (offset % 4) == 0) {
      it(cond);
      ldr_sp_imm8x4(rd, offset / 4);
    } else if(rn == pc && has_room_for_imm(offset / 4, 8)
        && (offset % 4) == 0) {
      it(cond);
      ldr_pc_imm8x4(rd, offset / 4);
    } else if((rd < r8) && (rn < r8) && has_room_for_imm(offset / 4, 5)
        && (offset % 4) == 0) {
      it(cond);
      ldr_imm5x4(rd, rn, offset / 4);
    } else {
      if (offset >= -0xFF && offset <= 0xFFF) {
        it(cond);
        ldr_imm12_w(rd, rn, offset);
      } else {
        if (ENABLE_ARM_V7) {
          offset >>= 2;
        }
        if (rd == rn) {
          rm = alloc_tmp_register(false);
          GUARANTEE(rm != rd, "ldr/ldrb: Invalid register allocation");
        }
        ldr_big_integer(rm, offset, cond);
        it(cond);
        if (rd < r8 && rn < r8 && rm < r8) {
          Assembler::ldr(rd, rn, rm);
        } else {
          Assembler::ldr_w(rd, rn, rm, 0);
        }
        if (rd == rn) {
          release_tmp_register(rm);
        }
      }
    }
  }

  void ldr(Register rd, Register rn, Register rm) {
    if (rd < r8 && rn < r8 && rm < r8) {
      Assembler::ldr(rd, rn, rm);
    } else {
      Assembler::ldr_w(rd, rn, rm, 0);
    }
  }

  void ldrb(Register rd, Register rn, int offset = 0) {

    Register rm = rd;
    if ((rd < r8) && (rn < r8) && has_room_for_imm(offset, 5)) {
      ldrb_imm5(rd, rn, offset);
    } else if((offset <= 0xFFF) && (offset >= -0xFF)) {
      ldrb_imm12_w(rd, rn, offset);
    } else {
      Imm12 imm12 = try_modified_imm12(offset);
      if (imm12 != invalid_imm12) {
        ldr_imm12_w(rd, rn, imm12);
      } else {
        if (rd == rn) {
          rm = alloc_tmp_register(false);
          GUARANTEE(rm != rd, "ldr/ldrb: Invalid register allocation");
        }
        ldr_big_integer(rm, offset);
        if (rd < r8 && rn < r8 && rm < r8) {
          Assembler::ldrb(rd, rn, rm);
        } else {
          Assembler::ldrb_w(rd, rn, rm, 0);
        }
        if (rd == rn) {
          release_tmp_register(rm);
        }
      }
    }
  }

  void ldrh(Register rd, Register rn, int offset = 0,
            int code_type = THUMB2_CODE) {
    GUARANTEE((offset & 1) == 0, "Invalid offset alignment");

    Register rm = rd;
    if (rd < r8 && rn < r8 &&
      offset >= 0 && has_room_for_imm(offset / 2, 5)) {
      ldrh_imm5x2(rd, rn, offset / 2);
    } else if ((offset <= 0xFFF) && (offset >= -0xFF)) {
      ldrh_imm12_w(rd, rn, offset);
    } else {

      if (ENABLE_ARM_V7 && code_type == THUMB2EE_CODE) {
        offset >>= 1;
      } else {
        GUARANTEE(code_type == THUMB2_CODE, "Invalid code type");
      }

      if (rd == rn) {
        rm = alloc_tmp_register(false) ;
        GUARANTEE(rm != rd, "ldrh: Invalid register allocation");
      }
      mov(rm, offset);
      if (rd < r8 && rn < r8 && rm < r8) {
        Assembler::ldrh(rd, rn, rm);
      } else {
        Assembler::ldrh_w(rd, rn, rm, 0);
      }
      if (rd == rn) {
        release_tmp_register(rm);
      }
    }
  }

  // store halfword (offset/reg) instruction
  void strh(Register rd, Register rn, int offset = 0,
            int code_type = THUMB2_CODE) {
    GUARANTEE((offset & 1) == 0, "Invalid offset alignment");

    if (offset >= 0 && has_room_for_imm(offset/2, 5)) {
      strh_imm5x2(rd, rn, offset/2);
    } else if ((offset <= 0xFFF) && (offset >= -0xFF)) {
      strh_imm12_w(rd, rn, offset);
    } else {

      if (ENABLE_ARM_V7 && code_type == THUMB2EE_CODE) {
        offset >>= 1;
      } else {
        GUARANTEE(code_type == THUMB2_CODE, "Invalid code type");
      }

      Register rm = alloc_tmp_register(false);
      GUARANTEE(rm != rd && rm != rn, "strh: Invalid register allocation");
      ldr_big_integer(rm, offset);
      if (rd < r8 && rn < r8 && rm < r8) {
        Assembler::strh(rd, rn, rm);
      } else {
        Assembler::strh_w(rd, rn, rm, 0);
      }
      release_tmp_register(rm);
    }
  }

  // store byte (offset/reg) instruction
  void strb(Register rd, Register rn, int offset = 0) {
    if (offset >= 0 && has_room_for_imm(offset, 5) && rd < r8 && rn < r8) {
      strb_imm5(rd, rn, offset);
    } else if ((offset <= 0xFFF) && (offset >= -0xFF)) {
      strb_imm12_w(rd, rn, offset);
    } else {
      Register rm = alloc_tmp_register(false);
      GUARANTEE(rm != rd && rm != rn, "strb: Invalid register allocation");
      ldr_big_integer(rm, offset);
      if (rd < r8 && rn < r8 && rm < r8) {
        Assembler::strb(rd, rn, rm);
      } else {
        Assembler::strb_w(rd, rn, rm, 0);
      }
      release_tmp_register(rm);
    }
  }

  void str(Register rd, Register rn, int offset = 0, Condition cond = al) {
    GUARANTEE(offset % 4 == 0, "Invalid offset alignment");

    if ((rn == sp) && (rd < r8) && has_room_for_imm(offset / 4, 8)) {
      it(cond);
      str_sp_imm8x4(rd, offset / 4);
    } else {
      if ((rd < r8) && (rn < r8) &&
          has_room_for_imm(offset / 4, 5)) {
        it(cond);
        str_imm5x4(rd, rn, offset / 4);
      } else if ((offset <= 0xFFF) && (offset >= -0xFF)) {
        it(cond);
        str_imm12_w(rd, rn, offset);
      } else {
        if (ENABLE_ARM_V7) {
          offset >>= 2;
        }
        Register rm = alloc_tmp_register(false);
        GUARANTEE(rm != rd && rm != rn,
               "str: Invalid register allocation");
        ldr_big_integer(rm, offset, cond);
        // IMPL_NOTE: avoid extra it instruction
        it(cond);
        if (rd < r8 && rn < r8 && rm < r8) {
          Assembler::str(rd, rn, rm);
        } else {
          Assembler::str_w(rd, rn, rm, 0);
        }
        release_tmp_register(rm);
      }
    }
  }

#if ENABLE_ARM_V7
  // Store/Load a word at a positive immediate offset off jsp (this
  // assumes descending Java stack)
  // str rd, [jsp, #imm]
#define F(mnemonic) \
  void mnemonic##_jsp(Register rd, int imm) { \
    GUARANTEE(imm >= 0 && has_room_for_imm(imm, 8) && (imm%4) == 0, "sanity");\
    if (rd < r8) { \
      mnemonic##_r9(rd, imm >> 2); \
    } else { \
      mnemonic(rd, jsp, imm); \
    } \
  }
#else
#define F(mnemonic) \
  void mnemonic##_jsp(Register rd, int imm) { \
    GUARANTEE(imm >= 0 && has_room_for_imm(imm, 7) && (imm%4) == 0, "sanity");\
    if (rd < r8) { \
      mnemonic(rd, jsp, imm); \
    } else { \
      mnemonic(rd, jsp, imm); \
    } \
  }
#endif
  F(str)
  F(ldr)
#undef F


/***END OF LDR/STR SECTION**********************************************************************/
  void arith(Opcode opcode, Register rd, Register rm) {
    if ((opcode == 0xA) && ((rd > r7) || (rm > r7))) {
      cmp_hi(rd, rm);
      return;
    }
    // Not expected to handle hi registers in other cases
    GUARANTEE(rd <= r7 && rm <= r7, "arith: Invalid register");
    switch(opcode) {
      case _add:
        add(rd, rd, rm);
        break;
      case _sub:
        sub(rd, rd, rm);
        break;
      case _rsb:
      case _rsc:
        SHOULD_NOT_REACH_HERE();
        break;
      default:
        emit(1 << 14 | opcode << 6 | rm << 3 | rd);
    }
  }

#if !PRODUCT || ENABLE_COMPILER

 private:
  bool is_mul_imm_simple(Register rd, Register rm, int imm32);

 public:

#if ENABLE_ARM_V7
  void hbl_with_parameter(unsigned int handler, int param) {
    if (has_room_for_imm(param, 5)) {
      hblp(handler, param);
    } else {
      mov(r8, imm12(param));
      hbl(handler);
    }
  }
#endif

  void arith_imm(Opcode opcode, Register rd, int imm32,
                 LiteralAccessor& la);

  // IMPL_NOTE: implent proper Macros for instructions below
  void orr(Register rd, Register rm) {
    if (rd < r8 && rm < r8) {
      Assembler::orr(rd, rm);
    } else {
      orr_w(rd, rd, rm);
    }
  }

  void andr(Register rd, Register rm) {
    if (rd < r8 && rm < r8) {
      Assembler::andr(rd, rm);
    } else {
      and_w(rd, rd, rm);
    }
  }

  void bic(Register rd, Register rm) {
    if (rd < r8 && rm < r8) {
      Assembler::bic(rd, rm);
    } else {
      bic_w(rd, rd, rm);
    }
  }

  void adc(Register rd, Register rn, Register rm, CCMode S = any_CC) {
    adc_w(rd, rn, rm, lsl_shift, 0, S);
  }

  void orr(Register rd, Register rn, int imm) {
    orr_imm12_w(rd, rn, modified_imm12(imm));
  }

  void andr(Register rd, Register rn, int imm) {
    and_imm12_w(rd, rn, modified_imm12(imm));
  }

  void bic(Register rd, Register rn, int imm) {
    bic_imm12_w(rd, rn, modified_imm12(imm));
  }

  void cmp_imm_literal(Register rn, int imm32, LiteralAccessor& la);

  // immediate operands for multiplication
  void mul_imm(Register rd, Register rm, int imm32, Register tmp);

  // bit manipulations
  void oop_write_barrier(Register dst, const Register tmp1, Register tmp2,
                         Register tmp3, bool bounds_check);

  NOT_PRODUCT(virtual) void
      get_bitvector_base(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_heap_start(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_heap_top(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_old_generation_end(Register rd, Condition cond = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);

  NOT_PRODUCT(virtual) void comment(const char* fmt, ...) {}
#endif
};
