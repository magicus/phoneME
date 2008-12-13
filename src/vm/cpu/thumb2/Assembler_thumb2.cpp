/*
 * @(#)Assembler_thumb2.cpp     1.9 06/01/26 16:37:23
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

# include "incls/_precompiled.incl"
# include "incls/_Assembler_thumb2.cpp.incl"

#if !PRODUCT || ENABLE_COMPILER
// Implementation of Assembler

#ifdef PRODUCT

void Assembler::emit(short instr) {
  assembler()->emit(instr);
}
void Assembler::emit_int(int instr) {
  assembler()->emit(instr);
}
void Assembler::emit_w(int instr) {
  assembler()->emit_w(instr);
}
void Assembler::ldr_big_integer(Register rd, int imm32, Condition cond) {
  assembler()->ldr_big_integer(rd, imm32, cond);
}

void Macros::get_bitvector_base(Register rd, Condition cond) {
  assembler()->get_bitvector_base(rd, cond);
}
void Macros::get_heap_start(Register rd, Condition cond) {
  assembler()->get_heap_start(rd, cond);
}
void Macros::get_heap_top(Register rd, Condition cond) {
  assembler()->get_heap_top(rd, cond);
}
void Macros::get_old_generation_end(Register rd, Condition cond) {
  assembler()->get_old_generation_end( rd, cond );
}
#else
void Assembler::set_in_it_scope(int new_depth) {
  current_it_scope_depth = new_depth;
}
bool Assembler::is_in_it_scope() {
  return current_it_scope_depth > 0;
}
void Assembler::decrease_current_it_depth() {
  if (current_it_scope_depth > 0) {
    current_it_scope_depth--;
  }
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

 /* and */   { alt_NOT,  Assembler::_bic,  false,  0    },
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
                       const LiteralAccessor& la) {
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
    lsl_imm5(rn, rm,  exact_log2(imm32 - 1));
    add(rd, rm, rn);
    release_tmp_register(rn);
  } else if (is_power_of_2(imm32    )) {
    lsl_imm5(rd, rm,  exact_log2(imm32));
  } else if (is_power_of_2(imm32 + 1)) {
    Register rn = alloc_tmp_register(false);
    lsl_imm5(rn, rm,  exact_log2(imm32 + 1));
    sub(rd, rn, rm);
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
    lsl_imm5(tmp, tmp, 1);
    mov(rd, tmp);
  } else {
    if (rd == rm) {
      mov(tmp, imm32);
      mul(tmp, rm);
      mov(rd, tmp);
    }else {
      mov(rd, imm32);
      mul(rd, rm);
    }
  }
}

void Macros::cmp_imm_literal(Register rn, int imm32, const LiteralAccessor& la) {
  Register rm;
  if (la.has_literal(imm32, rm)) {
    cmp(rn, rm);
  } else {
    cmp(rn, imm12(imm32));
  }
}

bool Assembler::try_alloc_tmp(int nregisters) {
  GUARANTEE(!GenerateAssemblyCode, "not supported in source assembler");
#if ENABLE_COMPILER
  return RegisterAllocator::has_free(nregisters, true);
#else
 return false;
#endif
}

Assembler::Register Assembler::alloc_tmp_register(bool hi) {
  GUARANTEE(!GenerateAssemblyCode, "not supported in source assembler");
#if ENABLE_COMPILER
  if (hi) {
    return r12;
  } else {
    if (try_alloc_tmp(1)) {
      return RegisterAllocator::allocate();
    } else {
      breakpoint(); // XENON: should not reach here!
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

// Produce a constant that can be constructed by
// (0x80 + seven_bits) << left_shift_count.
// <seven_bits> must be in the range of 0x00 ~ 0x7f.
Assembler::Imm12 Assembler::modified_imm12(int seven_bits,
                                           int ror_count) {
  GUARANTEE(juint(seven_bits) <= 0x7f, "sanity");
  GUARANTEE(ror_count >=  1 &&
            ror_count <= 31, "sanity");
  return (Imm12)((ror_count << 7) | seven_bits);
}
void Macros::add_sub_big_integer(Register rd, Register rn, int imm,
                                    bool is_add) {
  // GUARANTEE(rd < 8 && rn < 8, "sanity"); // IMPL_NOTE: use add_w!
  Register rm = rd;
  if (rd == rn) {
    rm = alloc_tmp_register(false);
    GUARANTEE(rm != rd, "add|sub: Invalid register allocation");
  }
  mov(rm, abs(imm));
  if (is_add) {
    if(imm >= 0) {
      add(rd, rn, rm);
    } else {
      sub(rd, rn, rm);
    }
  } else {
    if (imm < 0) {
      add(rd, rn, rm);
    } else {
      sub(rd, rn, rm);
    }
  }
  if (rd == rn) {
    release_tmp_register(rm);
  }
}

void Macros::oop_write_barrier(Register dst, Register tmp1, Register tmp2,
                               Register tmp3, bool range_check) {
  Register base       = tmp1;
  Register bit_offset = dst;
  Register tmp        = tmp3;

  NOT_PRODUCT(comment("oop_write_barrier"));
  if (range_check) {
    get_heap_start(tmp1);
    get_old_generation_end(tmp2);
    cmp(dst, reg(tmp1));
    it(hs, THEN_THEN_THEN);
    add(tmp2, tmp2, one);
    cmp(tmp2, reg(dst));
  }

  get_bitvector_base(base);
  // bit_offset starting from base
  mov_w(bit_offset, dst, lsr_shift, LogBytesPerWord);

  if (HARDWARE_LITTLE_ENDIAN) {
    const int rem_mask = right_n_bits(LogBitsPerByte);
    Register byte_value = tmp2;
    Register byte_bit_offset = bit_offset;

    if (range_check) {
      it(hs, THEN_THEN_THEN);
    }
    // Get the byte, update base
    add(base, base, bit_offset, lsr_shift, LogBitsPerByte);
    ldrb_imm12_w(byte_value, base, 0);
    // Compute the bit offset (dst % bitsPerByte) inside the mask
    mov(tmp, imm12(1));
    andr(byte_bit_offset, bit_offset, imm12(rem_mask));
    if (range_check) {
      it(hs, THEN_THEN);
    }
    lsl_w(tmp, tmp, byte_bit_offset);
    orr_w(byte_value, byte_value, tmp);
    strb_imm12_w(byte_value, base, 0);
  } else {
    const int rem_mask = right_n_bits(LogBitsPerWord);
    Register word_value = tmp2;
    Register word_bit_offset = bit_offset;

    // With big-endian, we must set a word at a time, so that we match
    // the behavior of ObjectHeap_arm.cpp.
    if (range_check) {
      it(hs, THEN_THEN_THEN);
    }
    mov_w(tmp, dst, lsr_shift, LogBitsPerWord);
    add(base, base, tmp, lsl_shift, LogBytesPerWord);
    ldr_imm12_w(word_value, base);
    mov_imm12_w(tmp, imm12(1));
    if (range_check) {
      it(hs, THEN_THEN_THEN);
    }
    andr(word_bit_offset, bit_offset, imm12(rem_mask));
    lsl_w(tmp, tmp, word_bit_offset);
    orr_w(word_value, word_value, tmp);
    str_imm12_w(word_value, base);
  }
}

#endif /* #if !PRODUCT || ENABLE_COMPILER */

Assembler::Imm12 Macros::try_modified_imm12(int big_number) {
  // This function should be used only by the source assembler to dynamically
  // generate an encoded Imm12. The compiler should be coded different
  if (0 <= big_number && big_number <= 0xff) {
    return imm12(big_number);
  }
  juint n = (juint)big_number;

  // Shift until the most significant bit is at bit 31
  int i = 0;
  while (!(n >> 31)) {
    i++;
    n <<= 1;
  }
  GUARANTEE(i < 32, "the immediate value cannot be zero");

  // Rotate until the MSB is at bit 7
  n = (n << 8) | (n >> 24);
  i += 8;

  GUARANTEE(n & (1 << 7), "sanity");

  if ((n & 0xffffff00) != 0) {
    return invalid_imm12;
  }
  return Assembler::modified_imm12(int(n) & 0x7f, i, big_number);
}

Assembler::Imm12 Macros::modified_imm12(int big_number) {
  Assembler::Imm12 imm = try_modified_imm12(big_number);
  GUARANTEE(imm != invalid_imm12, "big number can not be modified");
  return imm;
}
