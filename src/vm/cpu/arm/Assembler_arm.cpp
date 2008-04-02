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

#if !ENABLE_THUMB_COMPILER
#if !defined(PRODUCT) || ENABLE_COMPILER
#include "incls/_Assembler_arm.cpp.incl"

// Implementation of Assembler

#ifdef PRODUCT

void Macros::ldr_big_integer(Register rd, int imm32, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->ldr_big_integer(rd, imm32, cond);
}

void Macros::get_bitvector_base(Register rd, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->get_bitvector_base(rd, cond);
}

void Macros::get_bit_selector(Register rd, Condition cond) {
  BinaryAssembler* ba = (BinaryAssembler*) this;
  ba->get_bit_selector(rd, cond);
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

bool Assembler::is_rotated_imm_slow(int x, Address1& result) {
  // find the smallest even r such that x = rotr(y, r), and 0 <= y < 0xff
  for (int r = 0; r < 32; r += 2) {
    const int y = _rotl(x, r);
    if ((y & 0xffffff00) == 0) {
      result = imm_rotate(y, r);
      return true;
    }
  }
  return false;
}

Assembler::Address1 Assembler::imm_slow(int imm_32) {
#if NOT_CURRENTLY_USED
  /* Uncomment this to see commonly used values */
  static int done = 0;
  if (!done) {
    done = 1;
    Address1 res;
    for (int i=-256; i<256; i++) {
      tty->print("  /* %8x */ ", i);
      if (is_rotated_imm_slow(i, res)) {
        tty->print_cr("0x%x,", (int)res);
      } else {
        tty->print_cr("-1,");
      }
    }
  }
#endif

  Address1 result;
  if (is_rotated_imm(imm_32, result)) {
    return result;
  }
  JVM_FATAL(cannot_represent_imm_32_as_rotated_imm_8);
  return zero;
}

// Implementation of Macros

enum { alt_NONE, alt_NEG, alt_NOT };

struct OpcodeInfo { 
  jubyte alt_opcode_type;   // Is there equivalent bytecode with ~arg or -arg
  jubyte alt_opcode;        // Equivalent bytecode with ~arg or -arg
  jubyte twoword_allowed;   // Can immediate argument be split into two?
  jubyte twoword_opcode;    // Use for second piece of split immediate

  static const struct OpcodeInfo table[16];
};

const struct OpcodeInfo OpcodeInfo::table[16] = {
 // Note, when we split imm32 into two pieces, a and b, the two halves are
 // always bitwise exclusive, so that imm32 = a + b = a | b = a ^ b

 /* andr */  { alt_NOT,  Assembler::_bic,  false,  0    },
 /* eor  */  { alt_NONE, 0 ,               true,   Assembler::_eor },
 /* sub  */  { alt_NEG,  Assembler::_add,  true,   Assembler::_sub },
 /* rsb  */  { alt_NONE, 0 ,               true,   Assembler::_add },
 /* add  */  { alt_NEG,  Assembler::_sub,  true,   Assembler::_add },
 /* adc  */  { alt_NONE, 0 ,               true,   Assembler::_add },
 /* sbc  */  { alt_NONE, 0 ,               true,   Assembler::_sub },
 /* rsc  */  { alt_NONE, 0 ,               true,   Assembler::_add },
 /* tst  */  { alt_NONE, 0 ,               false,  0    },
 /* teq  */  { alt_NONE, 0 ,               false,  0    },
 /* cmp  */  { alt_NEG,  Assembler::_cmn,  false,  0    },
 /* cmn  */  { alt_NEG,  Assembler::_cmp,  false,  0    },
 /* orr  */  { alt_NONE, 0 ,               true,   Assembler::_orr },
 /* mov  */  { alt_NOT,  Assembler::_mvn,  true,   Assembler::_add },
 /* bic  */  { alt_NOT,  Assembler::_andr, true,   Assembler::_bic },
 /* mvn  */  { alt_NOT,  Assembler::_mov,  true,   Assembler::_bic }
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

void Macros::arith_imm(Opcode opcode, Register rd, Register rn, int imm32,
                       LiteralAccessor* la, CCMode s, Condition cond) {
  GUARANTEE(rd <= r15 && rn <= r15, "Invalid register used");
  
  Address1 result;
  if (is_rotated_imm(imm32, result)) {
    // Simplest case.  We can handle the immediate directly
    arith(opcode, rd, rn, result, s, cond);
    return;
  }
  int alt_opcode_type = OpcodeInfo::table[opcode].alt_opcode_type;
  int alt_imm32 = alt_opcode_type == alt_NEG ? -imm32 : ~imm32;
  Opcode alt_opcode = (Opcode)OpcodeInfo::table[opcode].alt_opcode;

  if (alt_opcode_type != alt_NONE && is_rotated_imm(alt_imm32, result)) {
    // We can handle the negated/complemented immediate
    arith(alt_opcode, rd, rn, result, s, cond);
    return;
  }

  // Is the imm32, or some rotated or shifted form of it already available
  if (la != NULL && la->has_literal(imm32, result)) {
    arith(opcode, rd, rn, result, s, cond);
    return;
  }
  if (alt_opcode_type != alt_NONE && 
      la != NULL && la->has_literal(alt_imm32, result)) {
    arith(alt_opcode, rd, rn, result, s, cond);
    return;
  }

  // Let's see if we can split either imm32 or alt_imm32 into two pieces,
  // each of which can be represented as an immediate.
  if ((rd != pc) && (s == no_CC || opcode <= _eor || opcode >= _orr)) {

    // Don't even try if we're setting the pc, or if this is an "arithmetic"
    // rather than a logical operation.  (For arithmetic operations, the C
    // and V bit have meanings that cannot be reproduced by splitting)
    if (OpcodeInfo::table[opcode].twoword_allowed) {
      if (arith2_imm(opcode, rd, rn, imm32, s, cond)) {
        return;
      }
    }
    if (alt_opcode_type != alt_NONE
           && OpcodeInfo::table[alt_opcode].twoword_allowed){
      // Can we break up alt_imm32 into two pieces, each an immediate?
      if (arith2_imm(alt_opcode, rd, rn, alt_imm32, s, cond)) {
        return;
      }
    }
  }

  if (opcode == _eor && imm32 == -1) {
    // This is the only chance we have of optimizing ~X
    mvn(rd, reg(rn), s, cond);
    return;
  }

  if (opcode == _mov) {
    ldr_big_integer(rd, imm32, cond);
    return;
  }

  // We include the (opcode != _mov) below, even though it isn't necessary,
  // since on the XScale we may want to get of the preceding clause.
  if (opcode != _mov) {
    Register tmp = (la != NULL) ? la->get_literal(imm32) : Assembler::no_reg;
    if (tmp != no_reg) {
      GUARANTEE(rn != tmp, "register must be different");
      arith(opcode, rd, rn, reg(tmp), s, cond);
      la->free_literal();
      return;
    }
  }

  // We are desperate.  We are clearly in some test suite situation that
  // is purposely generating a large immediate when that shouldn't
  // normally happen.  Let's just deal with it
  if ((rd != pc) && (s == no_CC || opcode <= _eor || opcode >= _orr)) {
    if (OpcodeInfo::table[opcode].twoword_allowed) {
      arith4_imm(opcode, rd, rn, imm32, s, cond);
      return;
    }
    if (alt_opcode_type != alt_NONE &&
      OpcodeInfo::table[alt_opcode].twoword_allowed){
      arith4_imm(alt_opcode, rd, rn, alt_imm32, s, cond);
      return;
    }
  }

  SHOULD_NOT_REACH_HERE();
}

bool Macros::is_twoword_immediate(int imm32,
                                  Address1& result1, Address1& result2) {
  GUARANTEE( !is_rotated_imm(imm32), "Shouldn't call this");
  if ((unsigned)imm32 < 0x10000) {
    // This takes care of 99% of all cases in which we return "true".
    // The following may not give the canonical form [the form with the
    // smallest possible value for "rotate"] for the immediates, but we don't
    // really care.
    result1 = imm_rotate(imm32 & 0xFF, 0);
    result2 = imm_rotate(imm32 >> 8,  24);
    return true;
  }
  for (int r = 0; r < 32; r += 2) {
    // If this can be decomposed into two 8-bit immediates, then there must
    // be at least 8 zeros in a row.  Hence there must be at least one point
    // when rotating the number in which the high byte is zero, and one of
    // the low two bits isn't.
    unsigned int y = _rotl(imm32, r);
    if (((y & 3) != 0) && (y & 0xFF000000) == 0) {
      // If y can be separated into two ARM immediates, then one of the
      // pieces must be the low byte, and the other piece must be some field
      // in the rest of y.  Let's try to find that other piece.
      // Since the high byte of y is zero, we only need to use shifts, not
      // rotates.
      for (int x = y >> 8, s = r - 8;   ; x >>=2, s -= 2) {
        if (x <= 0xFF) {
          // result1 will be in canonical form.  result2 may not be.  Oh well.
          result1 = imm_rotate(y & 0xFF, r);
          result2 = imm_rotate(x,        s & 30);
          return true;
        } else if ((x & 3) != 0) {
          // Shifting any more would lose bits
          // If we can't succeed when y is in this canonical form, then we
          // can't ever succeed.  No use continuing in the outer loop
          return false;
        }
      }
    }
  }
  return false;
}

bool Macros::arith2_imm(Opcode opcode, Register rd, Register rn, int imm32,
                        CCMode s, Condition cond) {
  // Try pretty hard to get the imm32 to be two immediates.  
  // Return true if we succeed in generating appropriate code, false
  // otherwise.
  Address1 result1, result2;
  if (is_twoword_immediate(imm32, result1, result2)) {
    Opcode opcode2 = (Opcode)OpcodeInfo::table[opcode].twoword_opcode;
    arith(opcode,  rd, rn, result1, no_CC, cond);
    arith(opcode2, rd, rd, result2, s,     cond);
    return true;
  }
  return false;
}

void Macros::arith4_imm(Opcode opcode, Register rd, Register rn, int imm32,
                        CCMode s, Condition cond) {
  // We don't try too hard to optimize.
  // Generate up to four instructions to perform the arithmetic.
  Opcode opcode2 = (Opcode)OpcodeInfo::table[opcode].twoword_opcode;
  unsigned int y = (unsigned)imm32;
  int rotate = 32;
  for(;;) {
    GUARANTEE(y != 0, "Sanity of loop below");
    while ((y & 3) == 0) {
      y = y >> 2; rotate -= 2;
    }
    if ((y & 0xFF) == y) {
      arith(opcode, rd, rn, imm_rotate(y, rotate & 30), s, cond);
      break;
    } else {
      arith(opcode, rd, rn, imm_rotate(y & 0xFF, rotate & 30), no_CC, cond);
      opcode = opcode2;
      rn = rd;
      y = (y >> 8);
      rotate -= 8;
    }
  }
}

bool Macros::is_mul_imm_simple(Register rd, Register rm, int imm32,
                               CCMode s, Condition cond) {
  bool is_simple = true;
         if (is_power_of_2(imm32 - 1)) {
    add(rd, rm, imm_shift(rm, lsl, exact_log2(imm32 - 1)), s, cond);
  } else if (is_power_of_2(imm32    )) {
    mov(rd,     imm_shift(rm, lsl, exact_log2(imm32    )), s, cond);
  } else if (is_power_of_2(imm32 + 1)) {
    rsb(rd, rm, imm_shift(rm, lsl, exact_log2(imm32 + 1)), s, cond);
  } else {
    is_simple = false;
  }
  return is_simple;
}

void Macros::mul_imm(Register rd, Register rm, int imm32, Register tmp,
                     CCMode s, Condition cond) {
  // Note: mul_imm could be generalized significantly (e.g. by using a
  //   recursive form of is_mul_imm_simple, which recursively checks for
  //   numbers such as x*(2^n +/- 1) and then generate a sequence of
  //   instructions). However, the current implementation covers all integers
  //   from -1 .. 10, and then some, which should be good enough for now (all
  //   other integers require an explicit mul)
  GUARANTEE(rm != tmp, "registers must be different");
  if (is_mul_imm_simple(rd, rm, imm32, s, cond)) {
    // imm32 is of the form 2^n -1/+0/+1 (e.g. 0, 1, 2, 3, 4, 5, 7, 8, 9, etc.)
    // => nothing else to do
  } else if (is_even(imm32) && is_mul_imm_simple(tmp, rm, imm32 >> 1,
                                                 no_CC, cond)) {
    // imm32 is of the form 2*(2^n -1/+0/+1) (e.g., 6 = 2*3, 10 = 2*5, etc.)
    // => need to multiply with 2
    mov(rd, imm_shift(tmp, lsl, 1), s, cond);
  } else {
    mov_imm(tmp, imm32, cond);
    mul(rd, rm, tmp, s, cond);
  }
}

void Macros::mov_reg(Register rd, Register rs, CCMode s, Condition cond) {
  if (rd != rs || s == set_CC) mov(rd, reg(rs), s, cond);
}

void Macros::oop_write_barrier(Register dst, Register tmp1, Register tmp2,
                               Register tmp3, bool range_check) {
  NOT_PRODUCT(comment("oop_write_barrier"));
  Condition cond = al;
  if (range_check) {
    get_heap_start(tmp1);
    get_old_generation_end(tmp2);
    cmp(dst, reg(tmp1));
    // don't care of one byte beyond old_generation_end
    cmp(tmp2, reg(dst), hs);
    cond = hs;
  } else if (GenerateDebugAssembly || 
             (!GenerateAssemblyCode && GenerateCompilerAssertions)) {
    get_heap_start(tmp1);
    get_heap_top(tmp2);
    cmp(dst, reg(tmp1));
    breakpoint(lo);
    cmp(dst, reg(tmp2));
    breakpoint(hs);
  }

  Register bitvector_base = tmp1;
  Register bit_selector   = tmp2; // magic constant, see below
  Register mem_value      = tmp3;

#ifdef SOLARIS
  // We only use Solaris/ARM to generate precompiled methods inside
  // a ROM image. For the time being, only little-endian ARM targets are 
  // supported.
  const bool little_end = true;
#else
  const bool little_end = HARDWARE_LITTLE_ENDIAN;
#endif

  if (little_end) {
    get_bitvector_base(bitvector_base, cond);
    get_bit_selector(bit_selector, cond);          // constant 0x80808080
    ldrb(mem_value, add_index(bitvector_base, dst, lsr, 5, pre_indexed), cond);
    mvn(dst, imm_shift(dst, lsr, LogBytesPerWord), cond);
    orr(mem_value, mem_value, reg_shift(bit_selector, ror, dst), cond);
    strb(mem_value, imm_index(bitvector_base), cond);
  } else {
    get_bitvector_base(bitvector_base, cond);
    mov(bit_selector, imm_rotate(2, 2), cond);     // constant 0x80000000
    mov(mem_value, imm_shift(dst, lsr, 7), cond);
    ldr(mem_value, add_index(bitvector_base, mem_value, lsl, 2, pre_indexed),
                                                                       cond);
    mvn(dst, imm_shift(dst, lsr, LogBytesPerWord), cond);
    orr(mem_value, mem_value, reg_shift(bit_selector, ror, dst), cond);
    str(mem_value, imm_index(bitvector_base), cond);
  }
}
#endif /*#ifdef PRODUCT */

#endif /*#if !ENABLE_THUMB_COMPILER*/
