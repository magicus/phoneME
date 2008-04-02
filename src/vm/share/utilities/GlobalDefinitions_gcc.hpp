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
#if defined(LINUX) && !defined(SOLARIS)
# include <stdint.h>
#endif
# include <stdarg.h>
# include <stdlib.h>
# include <stddef.h>
# include <math.h>
# include <stdio.h>
# include <time.h>
# include <fcntl.h>
# include <unistd.h>
#ifdef SOLARIS
# include <ieeefp.h>
#endif
//----------------------------------------------------------------------------
// Additional Java basic types

typedef unsigned char  jubyte;
typedef unsigned short jushort;
typedef unsigned int   juint;
typedef unsigned long long  julong;

#ifdef NO_STDINT
#ifndef intptr_t
typedef int   intptr_t;
#define intptr_t intptr_t
#endif
typedef unsigned int   uintptr_t;
#endif

//---------------------------------------------------------------------------
// Special (possibly not-portable) casts
// Cast floats into same-size integers and vice-versa w/o changing bit-pattern

inline jint    jint_cast   (jfloat  x)           { return *(jint*   )&x; }
inline jlong   jlong_cast  (jdouble x)           { return *(jlong*  )&x; }

inline jfloat  jfloat_cast (jint    x)           { return *(jfloat* )&x; }
inline jdouble jdouble_cast(jlong   x)           { return *(jdouble*)&x; }

//---------------------------------------------------------------------------
// Debugging

#if ARM && !CROSS_GENERATOR

#if ENABLE_THUMB_VM

#  define BREAKPOINT __asm__ ("swi 0x01")
#  define panic() __asm__ ("swi 0x01")
inline unsigned int _rotr(unsigned int x, unsigned int y) {
    int result;
    asm("ror %0, %2"         : "=l" (result) : "0" (x), "l" (y));
    return result;
}

#else

#  define BREAKPOINT __asm__ ("swi 0x9f0001")
#  define panic()  __asm__(".word 0xE6000010")
inline unsigned int _rotr(unsigned int x, unsigned int y) {
    int result;
    asm("mov %0, %1, ror %2" : "=r" (result) : "r" (x), "r" (y));
    return result;
}

#endif


#elif HITACHI_SH && !CROSS_GENERATOR

#  define BREAKPOINT   __asm__ ("trapa #0xC3")
#  define panic()      __asm__ ("trapa #0xC3")
inline unsigned int _rotr(unsigned int x, unsigned int y) {
  return (x >> y) | (x << (32 - y));
}

#elif defined(SPARC)

#  define BREAKPOINT __asm__ ("ta 0x1")
#  define panic()
inline unsigned int _rotr(unsigned int x, unsigned int y) {
  return (x >> y) | (x << (32 - y));
}

#elif defined(POWERPC)

#define BREAKPOINT __asm__("tw 12, r0, r0")
#define panic()
inline unsigned int _rotr(unsigned int x, unsigned int y) {
   return (x >> y) | (x << (32 - y));
}

#elif defined(MIPS)

#define BREAKPOINT __asm__("break #0xc3")
#define panic()    __asm__("break #0xc3")
inline unsigned int _rotr(unsigned int x, unsigned int y) {
   return (x >> y) | (x << (32 - y));
}

#else

// if everything else fails - we assume x86
#  define BREAKPOINT __asm__ ("int $03")
#  define panic() __asm__ ("int $03")
inline unsigned int _rotr(unsigned int x, unsigned int y) {
  return (x >> y) | (x << (32 - y));
}

#endif

inline unsigned int _rotl(unsigned int x, unsigned int y) {
  return _rotr(x, 32 - y);
}

#if !defined(ARM) && !defined(HITACHI_SH)
//---------------------------------------------------------------------------
// Checking for NaN-ness (x86 only)

inline int g_isnan(jfloat  f)                    { return isnan(f); }
inline int g_isnan(jdouble f)                    { return isnan(f); }

//---------------------------------------------------------------------------
// Checking for finiteness (x86 only)

inline int g_isfinite(jfloat  f)                 { return finite(f); }
inline int g_isfinite(jdouble f)                 { return finite(f); }

#endif // !defined(ARM) && !defined(HITACHI_SH)

//---------------------------------------------------------------------------
// Constant for jlong (specifying an long long constant is C++ compiler
// specific)

const jlong min_jlong = ((long long)1 << 63);
const jlong max_jlong = (min_jlong - 1);

//---------------------------------------------------------------------------
// Miscellaneous
//---------------------------------------------------------------------------

// Portability macros
#ifdef LINUX
#define __cdecl
#define _cdecl
#endif
