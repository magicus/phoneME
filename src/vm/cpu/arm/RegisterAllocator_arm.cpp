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
#if ENABLE_ARM_VFP
    ,
    /* s0  -> s1    */  Assembler::s1,
    /* s1  -> s2    */  Assembler::s2,
    /* s2  -> s3    */  Assembler::s3,
    /* s3  -> s4    */  Assembler::s4,
    /* s4  -> s5    */  Assembler::s5,
    /* s5  -> s6    */  Assembler::s6,
    /* s6  -> s7    */  Assembler::s7,
    /* s7  -> s8    */  Assembler::s8,
    /* s8  -> s9    */  Assembler::s9,
    /* s9  -> s10   */  Assembler::s10,
    /* s10 -> s11   */  Assembler::s11,
    /* s11 -> s12   */  Assembler::s12,
    /* s12 -> s13   */  Assembler::s13,
    /* s13 -> s14   */  Assembler::s14,
    /* s14 -> s15   */  Assembler::s15,
    /* s15 -> s16   */  Assembler::s16,
    /* s16 -> s17   */  Assembler::s17,
    /* s17 -> s18   */  Assembler::s18,
    /* s18 -> s19   */  Assembler::s19,
    /* s19 -> s20   */  Assembler::s20,
    /* s20 -> s21   */  Assembler::s21,
    /* s21 -> s22   */  Assembler::s22,
    /* s22 -> s23   */  Assembler::s23,
    /* s23 -> s24   */  Assembler::s24,
    /* s24 -> s25   */  Assembler::s25,
    /* s25 -> s26   */  Assembler::s26,
    /* s26 -> s27   */  Assembler::s27,
    /* s27 -> s28   */  Assembler::s28,
    /* s28 -> s29   */  Assembler::s29,
    /* s29 -> s30   */  Assembler::s30,
    /* s30 -> s31   */  Assembler::s31,
    /* s31 -> s0    */  Assembler::s0,
#endif
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
#if ENABLE_ARM_VFP
    ,
    /* s0  -> s1    */  Assembler::s1,
    /* s1  -> s2    */  Assembler::s2,
    /* s2  -> s3    */  Assembler::s3,
    /* s3  -> s4    */  Assembler::s4,
    /* s4  -> s5    */  Assembler::s5,
    /* s5  -> s6    */  Assembler::s6,
    /* s6  -> s7    */  Assembler::s7,
    /* s7  -> s8    */  Assembler::s8,
    /* s8  -> s9    */  Assembler::s9,
    /* s9  -> s10   */  Assembler::s10,
    /* s10 -> s11   */  Assembler::s11,
    /* s11 -> s12   */  Assembler::s12,
    /* s12 -> s13   */  Assembler::s13,
    /* s13 -> s14   */  Assembler::s14,
    /* s14 -> s15   */  Assembler::s15,
    /* s15 -> s16   */  Assembler::s16,
    /* s16 -> s17   */  Assembler::s17,
    /* s17 -> s18   */  Assembler::s18,
    /* s18 -> s19   */  Assembler::s19,
    /* s19 -> s20   */  Assembler::s20,
    /* s20 -> s21   */  Assembler::s21,
    /* s21 -> s22   */  Assembler::s22,
    /* s22 -> s23   */  Assembler::s23,
    /* s23 -> s24   */  Assembler::s24,
    /* s24 -> s25   */  Assembler::s25,
    /* s25 -> s26   */  Assembler::s26,
    /* s26 -> s27   */  Assembler::s27,
    /* s27 -> s28   */  Assembler::s28,
    /* s28 -> s29   */  Assembler::s29,
    /* s29 -> s30   */  Assembler::s30,
    /* s30 -> s31   */  Assembler::s31,    
    /* s15 -> s0    */  Assembler::s0,
#endif
};

void RegisterAllocator::initialize() {
  Assembler::Register next;

  if (code_generator()->omit_stack_frame()) {
    _next_register_table = (Assembler::Register*)next_register_table_noframe;
  } else {
    _next_register_table = (Assembler::Register*)next_register_table_frame;
  }
  next = Assembler::r0; // IMPL_NOTE: this needs fine tuning.

  _next_allocate       = next;
  _next_byte_allocate  = Assembler::no_reg;

  _next_spill          = next;
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

#endif /*#if !ENABLE_THUMB_COMPILER*/
