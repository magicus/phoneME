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

/** \file GlobalDefinitions_ads.hpp
 * Global definitions for ADS C++
 *
 * This file holds compiler-dependent includes,
 * globally used constants & types, class (forward)
 * declarations and a few frequently used utility functions.
 */

# include <ctype.h>
# include <string.h>
# include <stdint.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stddef.h>
# include <math.h> 
# include <stdio.h> 
# include <time.h>

//----------------------------------------------------------------------
// Additional Java basic types

typedef unsigned char       jubyte;
typedef unsigned short      jushort;
typedef unsigned int        juint;
typedef unsigned long long  julong;

typedef unsigned int   uintptr_t;

//----------------------------------------------------------------------
// Special (possibly not-portable) casts
// Cast floats into same-size integers and vice-versa w/o changing bit-pattern

inline jint    jint_cast   (jfloat  x)           { return *(jint*   )&x; }
inline jlong   jlong_cast  (jdouble x)           { return *(jlong*  )&x; }

inline jfloat  jfloat_cast (jint    x)           { return *(jfloat* )&x; }
inline jdouble jdouble_cast(jlong   x)           { return *(jdouble*)&x; }

//----------------------------------------------------------------------
// Debugging

extern "C" void ads_panic(void);

#define BREAKPOINT ads_panic()
#define panic()    ads_panic()

inline unsigned int _rotr(unsigned int x, unsigned int y) { 
#if ENABLE_THUMB_VM 
 // ADS does not let us generate assembly code in thumb mode.
  // We would like to generate 
  //    MOV  result, x
  //    ROR  result, y
  return (x >> y) | (x << (32 - y));
#else
  int result; 
  __asm {
    MOV result, x, ROR y
   }
  return result;
#endif
}

inline unsigned int _rotl(unsigned int x, unsigned int y) {
  return _rotr(x, 32 - y);
}

//----------------------------------------------------------------------
// Constant for jlong (specifying an long long constant is C++ compiler 
// specific)

const jlong min_jlong = 0x8000000000000000ULL;
const jlong max_jlong = 0x7fffffffffffffffULL;

//----------------------------------------------------------------------
// Miscellaneous

//----------------------------------------------------------------------

// Portability macros
#define _cdecl 
#define __cdecl

inline int stricmp(const char *s1, const char *s2) {
   int result;
   while ((result = tolower(*s1) - tolower(*s2++)) != 0) {
      if (*s1++ == '\0') break;
   }
   return result; 
}
