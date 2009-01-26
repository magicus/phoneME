/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_THUMB_COMPILER
#if !defined(PRODUCT) || ENABLE_TTY_TRACE


/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

class Disassembler: public StackObj {
#include "../arm/Disassembler_armthumb.hpp"
 private:
  static Assembler::Register _last_imm_register;
  static int _last_imm_offset;
  static int _last_imm;

  // instruction fields
  static const bool bit(short instr, int i) {
    return (instr >> i & 0x1) == 1;
  }

  static Register reg_field(const short instr, const int shift = 0) {
    return Assembler::as_register((instr >> shift) & 0x7);
  }

  static Register reg_field_w(const int instr, const int shift = 0) {
    return Assembler::as_register((instr >> shift) & 0x0f);
  }

  // disassembler
  void emit_unknown(short instr, const char* comment = NULL);
  void emit_register_list(short instr);

  void print_gp_name(int imm);
  const char *find_gp_name(int imm);
};

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE

#endif /*#if ENABLE_THUMB_COMPILER*/
