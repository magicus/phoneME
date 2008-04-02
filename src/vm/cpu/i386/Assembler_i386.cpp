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
#include "incls/_Assembler_i386.cpp.incl"

// This file contains the shared part of the source and binary assembler.
// Currently this is unused for X86. Ideally, this structure should match
// the structure for the ARM port.

#ifndef PRODUCT

const char* Assembler::name_for_byte_register(const Register reg) {
  static const char* names[number_of_registers] = 
//  { "al", "cl", "dl", "bl" };
  { "al", "cl", "dl", "bl", "noreg", "noreg", "noreg", "noreg", 
    "noreg", "noreg", "noreg", "noreg", "noreg", "noreg", "noreg", "noreg",
    "ah", "ch", "dh", "bh", "al", "cl", "dl", "bl" };
  return is_valid_byte_register(reg) ? names[reg] : "noreg";
}

const char* Assembler::name_for_work_register(const Register reg) {
  static const char* names[number_of_registers] = 
  { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
  return is_valid_int_register(reg) ? names[reg] : "noreg";
}

const char* Assembler::name_for_long_register(const Register reg) {
  static const char* names[number_of_registers] = 
  { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", 
    "fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7" };
  return is_valid_register(reg) ? names[reg] : "noreg";
}

#endif
