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

/** \file GlobalDefinitions_gcc.hpp
 * Global definitions for GNU C++
 *
 * This file holds compiler-dependent includes,
 * globally used constants & types, class (forward)
 * declarations and a few frequently used utility functions.
 */

# include <ctype.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stddef.h>
# include <math.h> 
# include <stdioLite.h> 
# include <time.h>

//-----------------------------------------------------------------------------
// Additional Java basic types

typedef unsigned char  jubyte;
typedef    unsigned short jushort;
typedef unsigned int   juint;
typedef unsigned long long  julong;

//-----------------------------------------------------------------------------
// Special (possibly not-portable) casts
// Cast floats into same-size integers and vice-versa w/o changing bit-pattern

inline jint    jint_cast   (jfloat  x)           { return *(jint*   )&x; }
inline jlong   jlong_cast  (jdouble x)           { return *(jlong*  )&x; }

inline jfloat  jfloat_cast (jint    x)           { return *(jfloat* )&x; }
inline jdouble jdouble_cast(jlong   x)           { return *(jdouble*)&x; }

//-----------------------------------------------------------------------------
// Debugging

#ifdef ARM
#ifndef CROSS_GENERATOR

#if ENABLE_THUMB_VM
#  define BREAKPOINT 
#  define panic()
#else
#  define BREAKPOINT __asm__ ("swi 0x9f0001")
#  define panic()  __asm__(".word 0xE6000010")
#endif

inline unsigned int _rotr(unsigned int x, unsigned int y) {
    int result; 
#if ENABLE_THUMB_VM
    asm("ror %0, %2"         : "=l" (result) : "0" (x), "l" (y));
#else
    asm("mov %0, %1, ror %2" : "=r" (result) : "r" (x), "r" (y));
#endif
    return result;
}

#else
#  define BREAKPOINT __asm__ ("int $03")
#  define panic() 
inline unsigned int _rotr(unsigned int x, unsigned int y)
{
  return (x >> y) | (x << (32 - y));
}

#endif

inline unsigned int _rotl(unsigned int x, unsigned int y) {
  return _rotr(x, 32 - y);
}

#else
#  define BREAKPOINT __asm__ ("int $03")
#endif

//-----------------------------------------------------------------------------
// Constant for jlong (specifying an long long constant is C++ compiler
// specific)

const jlong min_jlong = ((long long)1 << 63);
const jlong max_jlong = (min_jlong - 1);

//-----------------------------------------------------------------------------
// Miscellaneous
//-----------------------------------------------------------------------------
#define vsnprintf(b,s,f,a) vsprintf(b,f,a)

// Portability macros
#define PRAGMA_INTERFACE      
#define PRAGMA_IMPLEMENTATION
#define PRAGMA_IMPLEMENTATION_(arg)

#define __cdecl
#define _cdecl
