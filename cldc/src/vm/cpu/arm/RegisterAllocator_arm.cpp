/*
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

#include "incls/_precompiled.incl"

#if !ENABLE_THUMB_COMPILER
#if ENABLE_COMPILER
#include "incls/_RegisterAllocator_arm.cpp.incl"

static const Assembler::Register
next_register_table_frame[Assembler::number_of_registers] = {
    /* r0  -> r1    */  Assembler::r1,
    /* r1  -> r2    */  Assembler::r2,
    /* r2  -> r3    */  Assembler::r3,
    /* r3  -> r4    */  Assembler::r4,
    /* r4  -> r6/r7 */  ((Assembler::r6 == Assembler::jsp) ? Assembler::r7
                                                           : Assembler::r6),
    /* r5  is gp    */  Assembler::no_reg,
    /* r6  -> r7?   */  ((Assembler::r6 == Assembler::jsp) ? Assembler::no_reg
                                                           : Assembler::r7),
    /* r7  -> r8    */  Assembler::r8,
    /* r8  -> r9    */  Assembler::r9,
    /* r9  -> r10   */  Assembler::r10,
    /* r10 -> r12   */  Assembler::r12,
    /* r11 is fp    */  Assembler::no_reg,
    /* r12 -> r14   */  Assembler::r14,
    /* r13 is sp    */  Assembler::no_reg,
    /* r14 -> r0    */  Assembler::r0,
    /* r15 is pc    */  Assembler::no_reg
};

static const Assembler::Register
next_register_table_noframe[Assembler::number_of_registers] = {
    /* r0  -> r1    */  Assembler::r1,
    /* r1  -> r2    */  Assembler::r2,
    /* r2  -> r3    */  Assembler::r3,
    /* r3  -> r4    */  Assembler::r4,
    /* r4  -> r6/r7 */  ((Assembler::r6 == Assembler::jsp) ? Assembler::r7
                                                           : Assembler::r6),
    /* r5  is gp    */  Assembler::no_reg,
    /* r6  -> r7?   */  ((Assembler::r6 == Assembler::jsp) ? Assembler::no_reg
                                                           : Assembler::r7),
    /* r7  -> r8    */  Assembler::r8,
    /* r8  -> r9    */  Assembler::r9,
    /* r9  -> r10   */  Assembler::r10,
    /* r10 -> r12   */  Assembler::r12,
    /* r11 is fp    */  Assembler::no_reg,
    /* r12 -> r0    */  Assembler::r0,
    /* r13 is sp    */  Assembler::no_reg,
    /* r14 is lr    */  Assembler::no_reg,
    /* r15 is pc    */  Assembler::no_reg
};

void RegisterAllocator::initialize() {
  Assembler::Register next;

  if (Compiler::omit_stack_frame()) {
    _next_register_table = (Assembler::Register*)next_register_table_noframe;
    next = Assembler::r0; // IMPL_NOTE: this needs fine tuning.
  } else {
    _next_register_table = (Assembler::Register*)next_register_table_frame;
    next = Assembler::r0; // IMPL_NOTE: this needs fine tuning.
  }

  _next_allocate       = next;
  _next_byte_allocate  = Assembler::no_reg;
  _next_float_allocate = next;

  _next_spill          = next;
  _next_byte_spill     = Assembler::no_reg;
  _next_float_spill    = next;

  initialize_register_references();
}

#endif

#endif /*#if !ENABLE_THUMB_COMPILER*/
