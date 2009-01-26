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
# include "incls/_RegisterAllocator_thumb2.cpp.incl"

#if ENABLE_COMPILER

const static Assembler::Register
next_register_table[Assembler::number_of_registers] = {
#if ENABLE_ARM_V7
    /* r0  -> r1    */  Assembler::r1,
    /* r1  -> r2    */  Assembler::r2,
    /* r2  -> r3    */  Assembler::r3,
    /* r3  -> r4    */  Assembler::r4,
    /* r4  -> r5    */  Assembler::r5,
    /* r5  -> r6    */  Assembler::r6,
    /* r6  -> r7    */  Assembler::r7,
    /* r7  -> r0    */  Assembler::r0,
    /* r8  : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r9  : jsp    */  Assembler::no_reg,
    /* r10 : gp     */  Assembler::no_reg,
    /* r11 : fp     */  Assembler::no_reg,
    /* r12 : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r13 : sp     */  Assembler::no_reg,
    /* r14 : lr     */  Assembler::no_reg,
    /* r15 : pc     */  Assembler::no_reg
#else
    /* r0  -> r1    */  Assembler::r1,
    /* r1  -> r2    */  Assembler::r2,
    /* r2  -> r3    */  Assembler::r3,
    /* r3  -> r7    */  Assembler::r7,
    /* r4  : fp     */  Assembler::no_reg,
    /* r5  : gp     */  Assembler::no_reg,
    /* r6  : jsp    */  Assembler::no_reg,
    /* r7  -> r0    */  Assembler::r0,
    /* r8  : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r9  : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r10 : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r11 : unused */  Assembler::no_reg,
    /* r12 : unused */  Assembler::no_reg, // IMPL_NOTE: use it for something!
    /* r13 : sp     */  Assembler::no_reg,
    /* r14 : lr     */  Assembler::no_reg,
    /* r15 : pc     */  Assembler::no_reg
#endif
};

void RegisterAllocator::initialize() {
  _next_register_table = (Assembler::Register*)next_register_table;

  const Assembler::Register next = Assembler::r7;

  _next_allocate       = next;
  _next_spill          = next;

  _next_byte_allocate  = Assembler::no_reg;
  _next_byte_spill     = Assembler::no_reg;

#if ENABLE_ARM_VFP
  _next_float_allocate = Assembler::s0;
  _next_float_spill    = Assembler::s0;
#else
  _next_float_allocate = next;
  _next_float_spill    = next;
#endif

  initialize_register_references();
}

#endif
