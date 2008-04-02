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

#if ENABLE_COMPILER
#include "incls/_RegisterAllocator_i386.cpp.incl"

static Assembler::Register 
next_register_table[Assembler::number_of_registers] = {
  Assembler::ecx,    // eax -> ecx
  Assembler::edx,    // ecx -> edx
  Assembler::ebx,    // edx -> ebx
  Assembler::esi,    // ebx -> esi
  Assembler::no_reg, // esp INVALID
  Assembler::no_reg, // ebp INVALID
  Assembler::edi,    // esi -> edi
  Assembler::eax,    // edi -> eax
  Assembler::fp1,    // fp0 -> fp1
  Assembler::fp2,    // fp1 -> fp2
  Assembler::fp3,    // fp2 -> fp3
  Assembler::fp4,    // fp3 -> fp4
  Assembler::fp5,    // fp4 -> fp5
  Assembler::fp6,    // fp5 -> fp6
  Assembler::fp7,    // fp6 -> fp7
  Assembler::fp0     // fp7 -> fp0
};

static Assembler::Register 
next_byte_register_table[Assembler::number_of_registers] = {
  Assembler::ecx,    // eax -> ecx
  Assembler::edx,    // ecx -> edx
  Assembler::ebx,    // edx -> ebx
  Assembler::eax,    // ebx -> eax
};

void RegisterAllocator::initialize() {
  _next_register_table      = next_register_table;
  _next_byte_register_table = next_byte_register_table;

  _next_allocate       = Assembler::edi;
  _next_byte_allocate  = Assembler::ebx;
  _next_float_allocate = Assembler::fp0;

  _next_spill          = Assembler::edi;
  _next_byte_spill     = Assembler::ebx;
  _next_float_spill    = Assembler::fp0;

  initialize_register_references();
}

#endif
