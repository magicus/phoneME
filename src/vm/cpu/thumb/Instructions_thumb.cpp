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

#if ENABLE_COMPILER

#include "incls/_Instructions_thumb.cpp.incl"

address Branch::target() const {
  if (!is_conditional()) {
    short instr = encoding();
    if (((instr >> 11) & 0x3) == 0x2) {
      Branch lo_b_instr(addr() + 2);
      GUARANTEE((lo_b_instr.encoding() >> 11) & 0x1, "Sanity");
      int hi_imm = imm();
      int lo_imm = lo_b_instr.imm();
      return addr() + 4 + (hi_imm << 11) + lo_imm;
    }
    
    GUARANTEE(((instr >> 11) & 0x3) == 0, "Sanity");
  }

  return addr() + 4 + imm();
}

#endif
