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

#if ENABLE_THUMB_COMPILER && ENABLE_COMPILER

#include "incls/_RegisterAllocator_thumb.cpp.incl"

const static Assembler::Register
next_register_table[Assembler::number_of_registers] = {
    /* r0  -> r1    */  Assembler::r1,
    /* r1  -> r2    */  Assembler::r2,
    /* r2  -> r3    */  Assembler::r3,
    /* r3  -> r4    */  Assembler::r7,
    /* r4  is fp    */  Assembler::no_reg,
    /* r5  is gp    */  Assembler::no_reg,
    /* r6  is jsp   */  Assembler::no_reg,
    /* r7  -> r0    */  Assembler::r0,
    /* r8  unused   */  Assembler::no_reg,
    /* r9  unused   */  Assembler::no_reg,
    /* r10 unused   */  Assembler::no_reg,
    /* r11 unused   */  Assembler::no_reg,
    /* r12 unused   */  Assembler::no_reg,
    /* r13 is sp    */  Assembler::no_reg,
    /* r14 unused   */  Assembler::no_reg,
    /* r15 is pc    */  Assembler::no_reg
};

void RegisterAllocator::initialize() {
  _next_register_table = (Assembler::Register*)next_register_table;

  _next_allocate       = Assembler::r7;
  _next_byte_allocate  = Assembler::no_reg;
  _next_float_allocate = Assembler::r7;

  _next_spill          = Assembler::r7;
  _next_byte_spill     = Assembler::no_reg;
  _next_float_spill    = Assembler::r7;

  initialize_register_references();
}

#endif
