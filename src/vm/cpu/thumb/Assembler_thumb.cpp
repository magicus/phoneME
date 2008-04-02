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

/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

#if ENABLE_THUMB_COMPILER
#if !defined(PRODUCT) || ENABLE_COMPILER

#include "incls/_Assembler_thumb.cpp.incl"

// Implementation of Assembler

#ifdef PRODUCT

void Assembler::emit(short instr) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->emit(instr);
}

void Assembler::emit_int(int instr) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->emit(instr);
}

#if ENABLE_ARM_V6T2
void Assembler::emit_w(int instr) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->emit_w(instr);
}
#endif

void Assembler::ldr_big_integer(Register rd, int imm32, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->ldr_big_integer(rd, imm32, cond);
}

void Macros::get_bitvector_base(Register rd, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->get_bitvector_base(rd, cond);
}

void Macros::get_heap_start(Register rd, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->get_heap_start(rd, cond);
}

void Macros::get_heap_top(Register rd, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->get_heap_top(rd, cond);
}

void Macros::get_old_generation_end(Register rd, Condition cond) {
  ((BinaryAssembler*) this)->get_old_generation_end( rd, cond );
}

#endif

//  bool Assembler::is_rotated_imm(int x, Address1& result) {
//    // find the smallest even r such that x = rotr(y, r), and 0 <= y < 0xff
//    for (int r = 0; r < 32; r += 2) {
//      const int y = _rotl(x, r);
//      if ((y & 0xffffff00) == 0) {
//        result = imm_rotate(y, r);
//        return true;
//      }
//    }
//    return false;
//  }

//  Assembler::Address1 Assembler::imm(int& imm_32) {
//    Address1 result;
//    if (is_rotated_imm(imm_32, result)) {
//      return result;
//    }
//    JVM_FATAL(cannot_represent_imm_32_as_rotated_imm_8);
//    return zero;
//  }

// Implementation of Macros


enum { alt_NONE, alt_NEG, alt_NOT };

struct OpcodeInfo { 
  jubyte alt_opcode_type;   // Is there equivalent bytecode with ~arg or -arg
  jubyte alt_opcode;        // Equivalent bytecode with ~arg or -arg
  jubyte twoword_allowed;   // Can immediate argument be split into two?
  jubyte twoword_opcode;    // Use for second piece of split immediate

  static const struct OpcodeInfo table[20];
};

const struct OpcodeInfo OpcodeInfo::table[20] = {
 // Note, when we split imm32 into two pieces, a and b, the two halves are
 // always bitwise exclusive, so that imm32 = a + b = a | b = a ^ b

 /* andr */  { alt_NOT,  Assembler::_bic,  false,  0    },
 /* eor  */  { alt_NONE, 0 ,               true,   Assembler::_eor },
 /* lsl  */  { alt_NONE, 0 ,               false,  0    },
 /* lsr  */  { alt_NONE, 0 ,               false,  0    },
 /* asr  */  { alt_NONE, 0 ,               false,  0    },
 /* adc  */  { alt_NONE, 0 ,               true,   Assembler::_add },
 /* sbc  */  { alt_NONE, 0 ,               true,   Assembler::_sub },
 /* ror  */  { alt_NONE, 0 ,               false,  0    },
 /* tst  */  { alt_NONE, 0 ,               false,  0    },
 /* neg  */  { alt_NONE, 0 ,               false,  0    },
 /* cmp  */  { alt_NEG,  Assembler::_cmn,  false,  0    },
 /* cmn  */  { alt_NEG,  Assembler::_cmp,  false,  0    },
 /* orr  */  { alt_NONE, 0 ,               true,   Assembler::_orr },
 /* mul  */  { alt_NONE, 0 ,               false,  0    },
 /* bic  */  { alt_NOT,  Assembler::_and,  true,   Assembler::_bic },
 /* mvn  */  { alt_NOT,  Assembler::_mov,  true,   Assembler::_bic },
 /* add  */  { alt_NEG,  Assembler::_sub,  true,   Assembler::_add },
 /* sub  */  { alt_NEG,  Assembler::_add,  true,   Assembler::_sub },
 /* mov  */  { alt_NOT,  Assembler::_mvn,  true,   Assembler::_add }
};

/*
 * NOTE: This function is slightly different when called with _mov (to handle
 * mov_imm) than when called with other operands.
 *
 * With _mov, this function is guaranteed not to call
 *     la.get_literal(...)
 * and hence it will never create a literal on its own.  It will try to
 * generate one or two instructions to create the literal, and if not, it
 *  will generate a literal using ldr_big_integer.
 *
 * With other opcodes, the code is willing to call
 *     la.ge_literal(...)
 * if necessary to create the literal.  If an actual LiteralAccessor has been
 * provided, this will allocate a register, and move the immediate to that
 * register, using a recursive call to mov_imm.
 *
 * Current, we try to generate the necessary code in two instructions before
 * giving up and loading memory (_mov) or creating a literal (other opcodes).
 * On the XScale, it may make sense to skip the step of loading from memory.
 * Just a thought
 */

void Macros::arith_imm(Opcode opcode, Register rd, int imm32,
                       LiteralAccessor& la) {
  Register  result;

  int alt_opcode_type = OpcodeInfo::table[opcode].alt_opcode_type;
  int alt_imm32 = alt_opcode_type == alt_NEG ? -imm32 : ~imm32;
  Opcode alt_opcode = (Opcode)OpcodeInfo::table[opcode].alt_opcode;

  // Is the imm32, or some rotated or shifted form of it already available
  if (la.has_literal(imm32, result)) {
    arith(opcode, rd, result);
    return;
  }
  if (alt_opcode_type != alt_NONE && la.has_literal(alt_imm32, result)) { 
    arith(alt_opcode, rd, result);
    return;
  }

  if (opcode == _eor && imm32 == -1) {
    // This is the only chance we have of optimizing ~X
    mvn(rd, rd);
    return;
  }

  if (opcode == _mov) {
    ldr_big_integer(rd, imm32);
    return;
  }

  // We include the (opcode != _mov) below, even though it isn't necessary,
  // since on the XScale we may want to get of the preceding clause.
  if (opcode != _mov) {
    Register tmp = la.get_literal(imm32);
    if (tmp != no_reg) { 
      GUARANTEE(rd != tmp, "register must be different");
      arith(opcode, rd, tmp);
      la.free_literal();
      return;
    }
  }

  SHOULD_NOT_REACH_HERE();
}

bool Macros::is_mul_imm_simple(Register rd, Register rm, int imm32) {
  bool is_simple = true;

  if (is_power_of_2(imm32 - 1)) {
    Register rn = alloc_tmp_register(false);
    lsl_imm(rn, rm,  exact_log2(imm32 - 1));
    add_regs(rd, rm, rn);
    release_tmp_register(rn);
  } else if (is_power_of_2(imm32    )) {
    lsl_imm(rd, rm,  exact_log2(imm32));
  } else if (is_power_of_2(imm32 + 1)) {
    Register rn = alloc_tmp_register(false);
    lsl_imm(rn, rm,  exact_log2(imm32 + 1));
    sub_regs(rd, rn, rm);
    release_tmp_register(rn);
  } else {
    is_simple = false;
  }


  
  return is_simple;
}

void Macros::mul_imm(Register rd, Register rm, int imm32, Register tmp) {
  // Note: mul_imm could be generalized significantly (e.g. by using a
  //   recursive form of is_mul_imm_simple, which recursively checks for
  //   numbers such as x*(2^n +/- 1) and then generate a sequence of
  //   instructions). However, the current implementation covers all integers
  //   from -1 .. 10, and then some, which should be good enough for now (all
  //   other integers require an explicit mul)
  GUARANTEE(rm != tmp, "registers must be different");
  if (is_mul_imm_simple(rd, rm, imm32)) {
    // imm32 is of the form 2^n -1/+0/+1 (e.g. 0, 1, 2, 3, 4, 5, 7, 8, 9, etc.)
    // => nothing else to do
  } else if (is_even(imm32) && is_mul_imm_simple(tmp, rm, imm32 >> 1)) {
    // imm32 is of the form 2*(2^n -1/+0/+1) (e.g., 6 = 2*3, 10 = 2*5, etc.)
    // => need to multiply with 2
    lsl_imm(tmp, tmp, 1);
    mov_reg(rd, tmp);
  } else {
    if (rd == rm) {
      mov_imm(tmp, imm32);
      mul(tmp, rm);
      mov(rd, tmp);
    }else {
      mov_imm(rd, imm32);
      mul(rd, rm);
    }
  }
}

void Macros::rsb(Register rd, Register rm, int imm) {
  Register rn = rd;
  if (rd == rm) {
    rn = alloc_tmp_register(false);
  }
  mov_imm(rn, imm);
  sub_regs(rd, rn, rm);

  if (rd == rm) {
    release_tmp_register(rn);
  }
}

void Macros::rsb_imm(Register /*rd*/, Register /*rm*/, int /*imm32*/,
                     LiteralAccessor& /*la*/){
  SHOULD_NOT_REACH_HERE();
}

void Macros::rsc(Register rd, Register rm, int imm) {
  Register rn = rd;
  if (rd == rm) {
    rn = alloc_tmp_register(false);
  }
  mov_imm(rn, imm);
  sub_regs(rd, rn, rm);

  if (rd == rm) {
    release_tmp_register(rn);
  }
}

void Macros::cmp_imm_literal(Register rn, int imm32, LiteralAccessor& la) {
  Register rm;
  if (la.has_literal(imm32, rm)) {
    Assembler::cmp_imm(rn, rm);
  } else {
    Assembler::cmp_imm(rn, imm32);
  }
}

bool Assembler::try_alloc_tmp(int nregisters) {
#if ENABLE_COMPILER
  return RegisterAllocator::has_free(nregisters, true);
#else
 return false;
#endif
}

Assembler::Register Assembler::alloc_tmp_register(bool hi) {
#if ENABLE_COMPILER
  if (hi) {
    return r12;
  } else {
    if (try_alloc_tmp(1)) {
      return RegisterAllocator::allocate();
    } else {
    // handle register exhaustion
    // Most likely a test case
      mov_hi(r9, gp);
      return gp;
    }
  }
#else
  return r12;
#endif
}

void Assembler::release_tmp_register(Register tmp) {
  if (tmp == gp) {
    mov_hi(gp, r9);
    return;
  }
  
#if ENABLE_COMPILER
  if (tmp < r8) {
    RegisterAllocator::dereference(tmp);
  }
#endif
}

void Assembler::imm_shift(Register rd, Register rm, Opcode shifter, int imm) {
  GUARANTEE(rm < r8 && rd < r8, "imm_shift: invalid register in thumb mode");

  // We have to treat 0 as a special case since "asr 0" and "lsr 0"
  // don't actually mean "shift right by zero"
  int shift = (imm & 0x1f);
  if (shift == 0) {
    mov_reg(rd, rm);
    return;
  }

  switch(shifter) {
    case _lsl:
      lsl_imm(rd, rm, shift);
      break;

    case _lsr:
      lsr_imm(rd, rm, shift);
      break;

    case _asr:
      asr_imm(rd, rm, shift);
      break;

    case _ror:
    {
      Register rs = alloc_tmp_register(false);
      mov_imm(rs, shift);
      mov_reg(rd, rm);
      ror(rd, rs);
      release_tmp_register(rs);
      break;
    }
    default:
    {
      SHOULD_NOT_REACH_HERE();
      break;
    }
  }
}

#endif /* #if !PRODUCT || ENABLE_COMPILER */
#endif /* #if ENABLE_THUMB_COMPILER */
