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

#ifdef UNDER_ADS
# define MAKE_IMM(x) (static_cast<int&>(x))
#else
# define MAKE_IMM(x) (x)
#endif

#if ENABLE_THUMB_COMPILER
  enum {
    zero = 0,
    one  = 1
  };
#endif

  friend class Disassembler;
  friend struct OpcodeInfo;

#if ENABLE_CODE_OPTIMIZER
  friend class CodeOptimizer;
#endif

 public:
  // can the immediate fit in size bits?
  static bool has_room_for_imm(const int imm, const int size) {
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

  enum Opcode {
    // position and order are relevant!
#if !ENABLE_THUMB_COMPILER
    _andr, _eor, _sub, _rsb, _add, _adc, _sbc, _rsc,
    _tst,  _teq, _cmp, _cmn, _orr, _mov, _bic, _mvn,
#else
    _and, _eor, _lsl, _lsr, _asr, _adc, _sbc, _ror,
    _tst, _neg, _cmp, _cmn, _orr, _mul, _bic, _mvn,
    _add, _sub, _mov, _rsb, _rsc,
#endif
    number_of_opcodes
  };
  static Opcode as_opcode( const unsigned encoding ) {
    GUARANTEE(encoding < unsigned(number_of_opcodes), "illegal opcode");
    return Opcode(encoding);
  }

  enum Condition {
    eq, ne, cs, cc, mi, pl, vs, vc,
    hi, ls, ge, lt, gt, le, al, nv,
    number_of_conditions,
    // alternative names
    hs = cs,
    lo = cc,
    always = al                 // used in generic code
  };
  static Condition as_condition( const unsigned encoding ) {
    GUARANTEE( encoding < unsigned(number_of_conditions), "illegal condition");
    return Condition(encoding);
  }
  static Condition not_cond( const Condition cond ) {
    return Condition(cond ^ 1);
  }

  enum Shift {
    // position and order is relevant!
#if !ENABLE_THUMB_COMPILER
    lsl, lsr, asr, ror,
#else
    lsl_shift, lsr_shift, asr_shift, ror_shift,
#endif
    number_of_shifts
  };
  static Shift as_shift( const unsigned encoding ) {
    GUARANTEE( encoding < unsigned(number_of_shifts), "illegal shift");
    return Shift(encoding);
  }

  static Register as_register(const unsigned encoding) {
    GUARANTEE( encoding < unsigned(number_of_registers), "illegal register");
    return Register(encoding);
  };

#if ENABLE_ARM_VFP
  static bool is_vfp_register(const Register reg) {
    return unsigned(reg - s0) < unsigned(number_of_float_registers);
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

  enum {
#if !ENABLE_THUMB_COMPILER
    instruction_alignment = 4
#else
    instruction_alignment = 2
#endif
  };

 protected:
  // assertion checking
  static void check_imm(const int imm, const int size) {
    GUARANTEE( has_room_for_imm(imm, size), "illegal immediate value");
    (void)imm; (void)size;
  }
