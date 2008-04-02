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

#ifdef UNDER_ADS
#define MAKE_IMM(x) (static_cast<int&>(x))
#else
#define MAKE_IMM(x) (x)
#endif

#define zero       0
#define one        1


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

  NOT_PRODUCT(virtual) void
    emit(short /*instr*/) NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
    emit_int(int /*instr*/) NOT_PRODUCT(JVM_PURE_VIRTUAL);
#if ENABLE_ARM_V6T2
  NOT_PRODUCT(virtual) void
    emit_w(int /*instr*/) NOT_PRODUCT(JVM_PURE_VIRTUAL);
#endif

  // assertion checking
  static void check_imm(int imm, int size) {
    GUARANTEE(has_room_for_imm(imm, size),
              "illegal immediate value");
    (void)imm;
    (void)size;
  }

 public:
  // can the immediate fit in size bits?
  static bool has_room_for_imm(int imm, int size) {
    return (imm & -(1 << size)) == 0;
  }

 protected:
  enum Opcode {
    // position and order is relevant!
    _and, _eor, _lsl, _lsr, _asr, _adc, _sbc, _ror,
    _tst, _neg, _cmp, _cmn, _orr, _mul, _bic, _mvn,
    _add, _sub, _mov, _rsb, _rsc,
    number_of_opcodes
  };

  static Opcode as_opcode(int encoding) {
    GUARANTEE(0 <= encoding &&
              encoding < number_of_opcodes, "illegal opcode");
    return (Opcode)encoding;
  }

 public:
  enum Register {
    // position and order is relevant!
    r0, r1, r2 , r3 , r4 , r5 , r6 , r7 ,
    r8, r9, r10, r11, r12, r13, r14, r15,
    number_of_registers,

    // for platform-independant code
    return_register = r0,
    stack_lock_register = r1,

    // for instruction assembly only
    sbz =  r0,
    sbo = r15,

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

  ////////

   // set to stack type on method return
    method_return_type = r2,

    // for linkage w/ shared code - will probably need to change
    no_reg                    =  -1,
    first_register            =  r0,
    last_register             = r15,
    number_of_float_registers =   0,

    first_allocatable_register = r0
  };

  static Register as_register(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_registers,
              "illegal register");
    return (Register)encoding;
  };

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
    instruction_alignment = 2
  };

  static bool is_c_saved_register(Register x) {
    return x >= r5 && x != r12 && x != r14;
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
   lsl_shift, lsr_shift, asr_shift, ror_shift,
   number_of_shifts
  };

  static Shift as_shift(int encoding) {
    GUARANTEE(0 <= encoding && encoding < number_of_shifts,
              "illegal shift");
    return (Shift)encoding;
  }

  void imm_shift(Register rd, Register rm, Opcode shifter, int imm);

  enum Mode {
    offset       = 1 << 24,
    pre_indexed  = 1 << 24 | 1 << 21,
    post_indexed = 0
  };

 // only used in our macros
  enum StackDirectionMode { ascending, descending };
  enum StackTypeMode { full, empty };

  // support for indices/offsets (make sure these get inlined!)
  static int abs(int x)   { return x < 0 ? -x : x; }
  static int up (int x)   { return x < 0 ?  0 : 1; }

  static Register reg(Register rm) { return rm; }

  static unsigned short set(Register reg) {
    return (unsigned short)(1 << reg);
  }

  static unsigned short set(Register reg1, Register reg2) {
    GUARANTEE(reg1 < reg2, "Invalid register set ordering");
    return (unsigned short)((1 << reg1) | (1 << reg2));
  }

  static unsigned short set(Register reg1, Register reg2,
                            Register reg3) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3,
              "Invalid register set ordering");
    return (unsigned short)((1 << reg1) | (1 << reg2) |
                            (1 << reg3));
  }

  static unsigned short set(Register reg1, Register reg2,
                      Register reg3, Register reg4) {
    GUARANTEE(reg1 < reg2 && reg2 < reg3 && reg3 < reg4,
               "Invalid register set ordering");
    return (unsigned short)((1 << reg1) | (1 << reg2) |
                            (1 << reg3) | (1 << reg4));
  }

  static unsigned short range(Register beg, Register end) {
    GUARANTEE(beg <= end, "illegal range");
    return (unsigned short)((1 << (end + 1)) - (1 << beg));
  }

  // move data-processing instructions
  enum CCMode { no_CC, set_CC };

 public:

#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rm) {                 \
    if ((opcode == 0xA) && ((rd > r7) || (rm > r7)))        \
      cmp_hi(rd, rm);                                       \
    else                                                    \
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

#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rm, int imm) {        \
    GUARANTEE(rd < r8 && rm < r8, "invalid register");      \
    GUARANTEE(imm >=0 && imm < 32, "invalid shift");        \
    emit((opcode-2) << 11 | imm << 6 | rm << 3 | rd);       \
  }
  F(lsl_imm, _lsl)
  F(lsr_imm, _lsr)
  F(asr_imm, _asr)
#undef F

  // add, sub register instructions
#define F(mnemonic, a)                                      \
  void mnemonic(Register rd, Register rn, Register rm) {    \
      emit(6 << 10 | a << 9 | rm << 6 | rn << 3 | rd);      \
  }                                                         \

 F(add_regs, 0)
 F(sub_regs, 1)
#undef F

#define F(mnemonic, a)                                      \
  void mnemonic(Register rd, Register rn, int imm = 0) {    \
    GUARANTEE(has_room_for_imm(abs(imm), 3) &&              \
              rd < r8 && rn < r8, "Invalid immediate");     \
      emit(0x7 << 10 | a << 9 | abs(imm) << 6               \
                     | rn << 3 | rd);                       \
  }
  F(add_3bit_imm, 0)
  F(sub_3bit_imm, 1)
#undef F

   // add/sub rd, rm, rn instructions
#define F(mnemonic, a)                                      \
  void mnemonic(Register rd, Register rn, int imm = 0) {    \
    if (has_room_for_imm(abs(imm), 3) &&                    \
        rd < r8 && rn < r8) {                               \
      if (a == 0) {                                         \
        if(imm >= 0) add_3bit_imm(rd, rn, imm);             \
        else sub_3bit_imm(rd, rn, imm);                     \
      } else {                                              \
        if(imm < 0) add_3bit_imm(rd, rn, imm);              \
          else sub_3bit_imm(rd, rn, imm);                   \
      }                                                     \
    } else {                                                \
      Register rm = rd;                                     \
      if (rd == rn) {                                       \
        rm = alloc_tmp_register(false);                     \
        GUARANTEE(rm != rd,                                 \
                  "add|sub: Invalid register allocation");  \
      }                                                     \
      mov_imm(rm, abs(imm));                                \
      if (a == 0) {                                         \
        if(imm >= 0) add_regs(rd, rn, rm);                  \
        else sub_regs(rd, rn, rm);                          \
      } else {                                              \
        if(imm < 0) add_regs(rd, rn, rm);                   \
          else sub_regs(rd, rn, rm);                        \
      }                                                     \
      if (rd == rn) {                                       \
        release_tmp_register(rm);                           \
      }                                                     \
    }                                                       \
  }
  F(add, 0)
  F(sub, 1)
#undef F

  // add, sub, mov, cmp immediate instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, int imm) {                     \
    if (has_room_for_imm(abs(imm), 8) && (rd < r8)) {       \
      if (imm >= 0) {                                       \
        emit(1 << 13 | opcode << 11 | rd << 8 | abs(imm));  \
      } else {                                              \
         if (opcode == 1) {                                 \
          Register rm = alloc_tmp_register(false);          \
          mov_imm(rm, abs(imm));                            \
          cmn(rd, rm) ;                                     \
          release_tmp_register(rm);                         \
        } else if (opcode == 2) {                           \
          emit(1 << 13 | 3 << 11 | rd << 8 | abs(imm));     \
        }  else {                                           \
          emit(1 << 13 | 2 << 11 | rd << 8 | abs(imm));     \
        }                                                   \
      }                                                     \
    } else {                                                \
      Register rn = alloc_tmp_register(false);              \
      ldr_big_integer(rn, imm);                             \
      if (opcode == 1)         cmp(rd, rn);                 \
      else if (opcode == 2) add_regs(rd, rd, rn);           \
      else                  sub_regs(rd, rd, rn);           \
      release_tmp_register(rn);                             \
    }                                                       \
  }
  F(cmp_imm, 0x1)
  F(add_imm, 0x2)
  F(sub_imm, 0x3)
#undef F

    // mov rd, <immed_8> instruction
#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, int imm) {                     \
    if (has_room_for_imm(abs(imm), 8) && (rd < r8)) {       \
      emit(1 << 13 | opcode << 11 | rd << 8 | abs(imm));    \
      if (imm < 0) neg(rd, rd);                             \
    } else if (rd < r8) {                                   \
      ldr_big_integer(rd, imm);                             \
    } else {                                                \
      Register tmp = alloc_tmp_register(false);             \
      if (has_room_for_imm(abs(imm), 8)) {                  \
        emit(1 << 13 | opcode << 11 | tmp << 8 | abs(imm)); \
        if (imm < 0) neg(tmp, tmp);                         \
      } else {                                              \
        ldr_big_integer(tmp, imm);                          \
      }                                                     \
      mov_hi (rd, tmp);                                     \
      release_tmp_register(tmp);                            \
    }                                                       \
  }
  F(mov_imm, 0x0)
#undef F

#define F(mnemonic)                                         \
  void mnemonic(Register rd, Register rm) {                 \
    if (rd > r7 || rm > r7) {                               \
      mov_hi(rd, rm);                                       \
    } else {                                                \
      emit(7 << 10 | rm << 3 | rd);                         \
    }                                                       \
  }
  F(mov)
  F(mov_reg)
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
  F(add_equal_imm, 0)
  F(sub_equal_imm, 1)
#undef F

  // add & sub sp 7 bit immediate instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(int imm) {                                  \
    if (has_room_for_imm(abs(imm), 7)) {                    \
      emit(10 << 12 | opcode << 11 | abs(imm));             \
    }                                                       \
    else {                                                  \
      Register tmp = alloc_tmp_register();                  \
      ldr_big_integer(tmp, imm);                            \
      if (opcode == 1) neg(tmp, tmp);                       \
      add(sp, tmp);                                         \
      release_tmp_register(tmp);                            \
    }                                                       \
  }
  F(add_sp, 0)
  F(sub_sp, 1)
#undef F

 // add PC +  8 bit immediate instructions
#define F(mnemonic)                                         \
  void mnemonic(Register rd, int imm) {                     \
    GUARANTEE(has_room_for_imm(abs(imm/4), 8) &&            \
      imm >= 0 && (imm % 4 == 0),                           \
     "add <Rd>, pc, immed - Invalid immediate");            \
    if (has_room_for_imm(abs(imm/4), 8)) {                  \
      emit(5 << 13 | rd << 8 | abs(imm/4));                 \
    }                                                       \
  }
  F(add_pc)
#undef F

  // load register instructions
#define F(mnemonic, opcode)                                 \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    GUARANTEE(rd < r8 && rn < r8 && rm < r8,                \
               "invalid register");                         \
    emit(5 << 12 | opcode << 9 | rm << 6 | rn << 3 | rd);   \
  }                                                         \

  F(ldrsh_regs, 0x7)
  F(ldrsb_regs, 0x3)
#undef F

#define F(mnemonic, opcode)                                 \
 void mnemonic(Register rd, Register rn, int offset = 0) {  \
    GUARANTEE(rd < r8 && rn < r8,                           \
                 "invalid register");                       \
    Register rm = rd;                                       \
    if (rd == rn) {                                         \
      rm = alloc_tmp_register(false);                       \
      GUARANTEE(rm != rd,                                   \
                "ldrsh/ldrsb: Invalid register allocation");\
    }                                                       \
    mov_imm(rm, offset);                                    \
    emit(5 << 12 | opcode << 9 | rm << 6 | rn << 3 | rd);   \
    if (rd == rn) {                                         \
      release_tmp_register(rm);                             \
    }                                                       \
  }

  F(ldrsh, 0x7)
  F(ldrsb, 0x3)
#undef F

  // load register instructions
#define F(mnemonic, b)                                      \
  void mnemonic(Register rd, Register rn, Register rm) {    \
    GUARANTEE(rd < r8 && rn < r8 && rm < r8,                \
               "invalid register");                         \
    emit(5 << 12 | 1 << 11 | b << 10 | rm << 6 |            \
         rn << 3 | rd);                                     \
  }                                                         \

  F(ldr_regs,  0)
  F(ldrb_regs, 1)
#undef F

  // load offset instructions
#define F(mnemonic, l, b)                                   \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    Register rm = rd;                                       \
    if (b == 0 && offset > 0) offset /= 4;                  \
    GUARANTEE(rd < r8, "invalid register");                 \
    GUARANTEE(rn < r8 || rn != lr || rn == sp || rn == pc,  \
              "invalid register");                          \
    switch (rn) {                                           \
      case sp:                                              \
           check_imm(abs(offset), 8);                       \
           emit(9 << 12 | l << 11 | rd << 8 | abs(offset)); \
           break;                                           \
      case pc:                                              \
           check_imm(abs(offset), 8);                       \
           emit(4 << 12 | l << 11 | rd << 8 | abs(offset)); \
           break;                                           \
      default:                                              \
           if (has_room_for_imm(abs(offset), 5)){           \
             if (offset >= 0) {                             \
               emit(3 << 13 | b << 12 | l << 11 |           \
                  (offset) << 6 | rn << 3 | rd);            \
             } else {                                       \
               if (rn != rd) {                              \
                 mov_imm(rd, 0);                            \
                 sub_imm(rd, abs(offset));                  \
                 emit(5 << 12 | l << 11 | b << 10 |         \
                      rd << 6 | rn << 3 | rd);              \
               } else {                                     \
                 sub_imm(rn, abs(offset));                  \
                 emit(3 << 13 | b << 12 | l << 11 |         \
                      rn << 3 | rd);                        \
                 add_imm(rn, abs(offset));                  \
               }                                            \
             }                                              \
           } else {                                         \
             if (b == 0 && offset > 0) offset *= 4;         \
             if (offset != 0 && rd == rn) {                 \
               rm = alloc_tmp_register(false);              \
               GUARANTEE(rm != rd,                          \
                  "ldr/ldrb: Invalid register allocation"); \
             }                                              \
             ldr_big_integer(rm, offset);                   \
             emit(5 << 12 | l << 11 | b << 10 |             \
                  rm << 6 | rn << 3 | rd);                  \
             if (rd == rn) {                                \
               release_tmp_register(rm);                    \
             }                                              \
           }                                                \
           break;                                           \
    }                                                       \
  }
  F(ldr,  1, 0)
  F(ldrb, 1, 1)
#undef F

  // load halfword (offset/reg) instructions
#define F(mnemonic, l)                                      \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    Register rm = rd;                                       \
    GUARANTEE(rd < r8 && rn < r8, "invalid register");      \
    if (offset >= 0 && has_room_for_imm(offset/2, 5)) {     \
      emit(8 << 12 | l << 11 | (offset/2) << 6 |            \
           rn << 3 | rd);                                   \
    } else {                                                \
      if (rd == rn) {                                       \
        rm = alloc_tmp_register(false) ;                    \
        GUARANTEE(rm != rd,                                 \
                  "ldrh: Invalid register allocation");     \
      }                                                     \
      mov_imm(rm, offset);                                  \
      emit(5 << 12 | l << 11 | 1 << 9 |                     \
           rm << 6 | rn << 3 | rd);                         \
      if (rd == rn) {                                       \
        release_tmp_register(rm);                           \
      }                                                     \
    }                                                       \
  }
  F(ldrh,  1)
#undef F

  // store halfword (offset/reg) instructions
#define F(mnemonic, l)                                      \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    GUARANTEE(rd < r8 && rn < r8, "invalid register");      \
    if (offset >= 0 && has_room_for_imm(offset/2, 5)) {     \
      emit(1 << 15 | (offset/2) << 6 | rn << 3 | rd);       \
    } else {                                                \
      Register rm = alloc_tmp_register(false);              \
      GUARANTEE(rm != rd || rm != rn,                       \
                  "strh: Invalid register allocation");     \
      ldr_big_integer(rm, offset);                          \
      emit(5 << 12 | 1 << 9 | rm << 6 | rn << 3 | rd);      \
      release_tmp_register(rm);                             \
    }                                                       \
  }
  F(strh,  0)
#undef F

  // store byte (offset/reg) instructions
#define F(mnemonic, l)                                      \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    GUARANTEE(rd < r8 && rn < r8, "invalid register");      \
    if (offset >= 0 && has_room_for_imm(offset, 5)) {       \
      emit(7 << 12 | offset << 6 | rn << 3 | rd);           \
    } else {                                                \
      Register rm = alloc_tmp_register(false);              \
      GUARANTEE(rm != rd && rm != rn,                       \
                  "strb: Invalid register allocation");     \
      ldr_big_integer(rm, offset);                          \
      emit(0x15 << 10 | rm << 6 | rn << 3 | rd);            \
      release_tmp_register(rm);                             \
    }                                                       \
  }
  F(strb,  0)
#undef F

  // store instructions
#define F(mnemonic, l, b)                                   \
  void mnemonic(Register rd, Register rn, int offset = 0) { \
    GUARANTEE(rd < r8, "invalid register");                 \
    GUARANTEE(rn < r8 || rn != sp, "invalid register");     \
    switch (rn) {                                           \
      case sp:                                              \
        check_imm(abs(offset), 8);                          \
        emit(9 << 12 | rd << 8 | offset);                   \
        break;                                              \
      default:                                              \
        if (offset >= 0 && has_room_for_imm(offset/4, 5)) { \
          emit(3 << 13 | b << 12 | (offset/4) << 6 |        \
               rn << 3 | rd);                               \
        } else {                                            \
           if (has_room_for_imm(abs(offset), 8)) {          \
             if(offset < 0) {                               \
               sub_imm(rn, abs(offset));                    \
               emit(3 << 13 | b << 12 | rn << 3 | rd);      \
               add_imm(rn, abs(offset));                    \
             } else {                                       \
               add_imm(rn, abs(offset));                    \
               emit(3 << 13 | b << 12 | rn << 3 | rd);      \
               sub_imm(rn, abs(offset));                    \
             }                                              \
           } else {                                         \
             Register rm = alloc_tmp_register(false);       \
             GUARANTEE(rm != rd && rm != rn,                \
                    "str: Invalid register allocation");    \
             ldr_big_integer(rm, offset);                   \
             emit(5 << 12 | l << 11 | b << 10 |             \
                  rm << 6 | rn << 3 | rd);                  \
             release_tmp_register(rm);                      \
           }                                                \
        }                                                   \
        break;                                              \
    }                                                       \
  }
  F(str,  0, 0)
#undef F

  // multiple register data transfer instructions
#define F(mnemonic, l)                                      \
  void mnemonic(Register rn, int set, int regs_in_list) {   \
    emit(12 << 12 | l << 11 | rn << 8 | set);               \
    sub_imm(rn, regs_in_list * 4);                          \
  }
  F(ldmia, 1)
  F(stmia, 0)
#undef F


#if ENABLE_ARM_V6T2

  // load word/short/byte (immediate offset)
#define F(mnemonic, S, type) \
  void mnemonic(Register rxf, Register rn, int offset) {    \
    emit_w(0x7c << 25 | S << 24 | 1 << 23 | type << 22 | 1 << 20 | \
           rn << 16 | rxf << 12 | offset); \
  }

  F(ldrb_w,  0, 0)
  F(ldrsb_w, 1, 0)
  F(ldrh_w,  0, 1)
  F(ldrsh_w, 1, 1)
  F(ldr_w,   0, 2)
#undef F

   // store unsigned word/short/byte (immediate offset)
#define F(mnemonic, type) \
  void mnemonic(Register rxf, Register rn, int offset) {    \
    emit_w(0x1f1 << 23 | type << 21 | rn << 16 | rxf << 12 | offset); \
  }

  F(strb_w,  0)
  F(strh_w,  1)
  F(str_w,   2)
#undef F

#endif //ENABLE_ARM_V6T2

  // ARM mode multiply instructions
  void mulop(Register rd, Register rn, Register rs, Register rm,
             int lua, CCMode s = no_CC, Condition cond = al) {
    // Note: rd & rn are exchanged for these instructions!
    emit_int(cond << 28 | lua << 21 | s << 20 | rd << 16 |
             rn << 12 | rs << 8 | 0x9 << 4 | rm);
  }

  void mla(Register rd, Register rm, Register rs, Register rn,
           CCMode s = no_CC, Condition cond = al) {
    mulop(rd, rn, rs, rm, 0x1, s, cond);
  }

#define F(mnemonic, lua) \
  void mnemonic(Register rdlo, Register rdhi, Register rm,  \
                Register rs, CCMode s = no_CC,              \
                Condition cond = al) {                      \
    mulop(rdhi, rdlo, rs, rm, lua, s, cond);                \
  }
  F(umull, 0x4)
  F(umlal, 0x5)
  F(smull, 0x6)
  F(smlal, 0x7)
#undef F

  // miscellaneous instructions
  void nop()               { orr(r0, r0); }

  void bx(Register rm) {
    emit(0x8E << 7 | rm << 3);
  }

  void blx(Register rm) {
    emit(0x1E << 11 | rm);
  }


  void swi(int imm_8) {
    check_imm(imm_8, 8);
    emit(0xdf << 8 | imm_8);
  }

  void arith(Opcode opcode, Register rd, Register rm) {             
    if ((opcode == 0xA) && ((rd > r7) || (rm > r7))) {
      cmp_hi(rd, rm);
      return;
    }
    // Not expected to handle hi registers in other cases
    GUARANTEE(rd <= r7 && rm <= r7, "arith: Invalid register");
    switch(opcode) {
      case _add:
        add_regs(rd, rd, rm);
        break;
      case _sub:
        sub_regs(rd, rd, rm);
        break;
      case _rsb:
      case _rsc:    
        SHOULD_NOT_REACH_HERE();
        break;
      default:
        emit(1 << 14 | opcode << 6 | rm << 3 | rd);
    }
  }

  void reg_shift(Register rd, Register rm, Opcode shifter) {
    GUARANTEE(rm < r8 && rd < r8,
            "imm_shift: invalid register in thumb mode");
    arith(shifter, rd, rm);
  }

  void breakpoint() {
    // IMPL_NOTE: we need to make this portable on platforms 
    // that may use a different method of breakpoints. Make 
    // sure the fix works with cross compilation -- remember 
    // the loop generator is compiled with the host compiler!

    // THUMB IMPL_NOTE : We need to switch into ARM mode
    // here if we want to make the correct swi call
    // into the operating system, however that opens
    // up problems like alignment etc. For later
    //swi(0x9f0001);
    swi(0xFF); // Faking swi here
  }

  NOT_PRODUCT(virtual) void
    ldr_big_integer(Register /*rd*/, int /*imm32*/, Condition /*cond*/ = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);

protected:

  bool try_alloc_tmp(int nregisters = 1);
  Register alloc_tmp_register(bool hi = true);
  void release_tmp_register(Register tmp);


#endif /* !PRODUCT || ENABLE_COMPILER */
};

class Macros;

class LiteralAccessor {
#if !defined(PRODUCT) || ENABLE_COMPILER
public:
  virtual bool has_literal(int /*imm32*/, Assembler::Register& /*result*/) {
    return false;
  }
  virtual Assembler::Register get_literal(int /*imm32*/) {
    return Assembler::no_reg;
  }
  virtual void free_literal() {}
#endif
};


// The Macros class implements frequently used instruction
// sequences or macros that can be shared between the source
// and binary assembler.

class Macros: public Assembler {

#if !defined(PRODUCT) || ENABLE_COMPILER

 private:
  bool is_mul_imm_simple(Register rd, Register rm, int imm32);

 public:

  void arith_imm(Opcode opcode, Register rd, int imm32,
                 LiteralAccessor& la);  

  void tst_imm(Register rm, int rn_val) {
    Register rn = alloc_tmp_register(false);
    mov_imm(rn, rn_val);
    tst(rm, rn);
    release_tmp_register(rn);
  }

  void andr_imm(Register rd, int rn_val) {
    Register rn = alloc_tmp_register(false);
    mov_imm(rn, rn_val);
    andr(rd, rn);
    release_tmp_register(rn);
  }

  void rsb(Register rd, Register rm, int imm);
  void rsb_imm(Register rd, Register rm, int imm, LiteralAccessor& la);
  void rsc(Register rd, Register rm, int imm);

  void cmp_imm_literal(Register rn, int imm32, LiteralAccessor& la);

  // immediate operands for multiplication
  void mul_imm(Register rd, Register rm, int imm32, Register tmp);

  // bit manipulations
  void oop_write_barrier(Register /*dst*/, const Register /*tmp1*/,
                         Register /*tmp2*/,
                         Register /*tmp3*/, bool /*bounds_check*/) {}

  NOT_PRODUCT(virtual) void
      get_bitvector_base(Register /*rd*/, Condition /*cond*/ = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_heap_start(Register /*rd*/, Condition /*cond*/ = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_heap_top(Register /*rd*/, Condition /*cond*/ = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);
  NOT_PRODUCT(virtual) void
      get_old_generation_end(Register /*rd*/, Condition /*cond*/ = al)
         NOT_PRODUCT(JVM_PURE_VIRTUAL);

  NOT_PRODUCT(virtual) void comment(const char* /*fmt*/, ...) {}
#endif
};
