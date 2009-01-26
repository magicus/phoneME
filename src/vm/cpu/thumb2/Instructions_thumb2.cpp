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

# include "incls/_precompiled.incl"
# include "incls/_Instructions_thumb2.cpp.incl"

#if ENABLE_COMPILER

void Branch::set_imm( const int target ) const {
  const short instr = encoding();
  int offset = target << 1;

  if (is_long_encoding(instr)) {
    const short instr_next = encoding_next();
    if (instr_next & (1 << 12)) {
      // unconditional
      GUARANTEE((offset >> 1) < (1 << 23) &&
                (offset >> 1) >= -(1 << 23), "Imm too large");
      GUARANTEE(!(offset & 0x1), "Imm not halfword-aligned");

      const int S = (offset >= 0) ? 0:1;
      const int I1 = ((~(offset >> 23) & 0x1) ^ S) & 0x1;
      const int I2 = ((~(offset >> 22) & 0x1) ^ S) & 0x1;
      set_encoding(0xF << 12 | S << 10 | (offset & 0x3FF000) >> 12);
      set_encoding_next(1 << 15 | I1 << 13 | I2 << 11 | 1 << 12 |
                        (offset & 0xFFF) >> 1);
    } else {
      // conditional
      GUARANTEE((offset >> 1) < (1 << 19) &&
                (offset >> 1) >= -(1 << 19), "Imm too large");
      GUARANTEE(!(offset & 0x1), "Imm not halfword-aligned");

      const int S = (offset >= 0) ? 0:1;
      const int J1 = ((offset >> 19) & 0x1);
      const int J2 = ((offset >> 18) & 0x1);
      set_encoding(0xF << 12 | S << 10 | (instr & 0x3C0) |
                   (offset & 0x3F000) >> 12);
      set_encoding_next(J1 << 13 | J2 << 11 | 1 << 15 |
                        (offset & 0xFFF) >> 1);
      }
  } else {
    const short type = (instr >> 13) & 0x7;
    if (type == 0x6) {
      offset = offset << 23 >> 23;
      GUARANTEE(offset >= -256 && offset < 255,
                "b<cond> <target>: Invalid offset");
      GUARANTEE(target < 128 && target >= -128,
                "b<cond> <target>: Invalid offset");
      set_encoding((instr & 0xFF00) | (target & 0xFF));
      GUARANTEE(imm() < 255 && imm() >= -256,
                "b<cond> <target>: Invalid offset");
    } else if (type == 0x7) {
      GUARANTEE(target < 2047 && target >= -2048,
                "b <target>: Invalid offset");
      set_encoding((instr & 0xF800) | (target & 0x7FF));
      GUARANTEE(imm() < 2047 && imm() >= -2048,
                "b <target>: Invalid offset");
    } else {
      set_encoding(instr & 0xF100 | target);
    }
  }
}

address Branch::target( void ) const {
  const short instr = encoding();
  if (is_long_encoding(instr)) {
    const short instr_next = encoding_next();
    int offset = (instr_next & 0x7FF) << 1;
    if (is_conditional()) {
      offset |= (instr & 0x3F) << 12;

      offset |= (instr_next & (1 << 13)) << 5;
      offset |= (instr_next & (1 << 11)) << 8;
      offset |= (instr & (1 << 10)) << 10;
      offset = offset << 11 >> 11;
    } else {
      offset |= (instr & 0x3FF) << 12;

      const int S = (instr >> 10) & 0x1;
      const int I1 = (instr_next >> 13) & 0x1;
      const int I2 = (instr_next >> 11) & 0x1;

      offset |= (S == I2 ? 1 : 0) << 22;
      offset |= (S == I1 ? 1 : 0) << 23;
      offset |= S << 24;
      offset = offset << 7 >> 7;
    }
    return addr() + 4 + offset;
  } else {
    return addr() + 4 + imm();
  }
}

#endif
