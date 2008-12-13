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

const int LogBytesPerInstr = 2;
const int BytesPerInstr    = 1 << LogBytesPerInstr;

#ifndef CROSS_GENERATOR
#define CROSS_GENERATOR 0
#endif

#if !CROSS_GENERATOR
  // Unless we're build a cross-generator (E.g., a "C"-loop romgen with an 
  // ARM compiler, used for AOT compilation), we don't need to use the
  // compiler.
#ifdef ENABLE_COMPILER
#undef ENABLE_COMPILER
#endif
#define ENABLE_COMPILER 0
#endif // CROSS_GENERATOR

// Used by ARM-AOT compiler
extern "C" {
  extern int _gp_bytecode_counter;
  extern int _gp_misc[];
}

// on x86 we give very special treatment to floating point operations,
// as x86 coprocessor has 80 bits of precision, and Java (IEEE) expects 64
#if defined(_MSC_VER) || defined(__i386__)
#define PROCESSOR_ARCHITECTURE_X86 1
#endif
