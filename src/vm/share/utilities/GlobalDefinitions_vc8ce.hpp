/*
 * @(#)GlobalDefinitions_evc.hpp	1.26 06/02/17 16:38:39
 *
 * Copyright © 2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** \file GlobalDefinitions_evc.hpp
 * Global definitions for eMbedded Visual C++
 *
 * This file holds compiler-dependent includes,
 * globally used constants & types, class (forward)
 * declarations and a few frequently used utility functions.
 */

# include <ctype.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stdio.h> // for va_list
# include <float.h> // for _isnan
# include <winbase.h>


// Compiler-specific primitive types
typedef unsigned int     uintptr_t;
typedef signed   int     intptr_t;

// offsetof
#define offsetof(s,m) ((size_t)&(((s*)0)->m))

//----------------------------------------------------------------------
// Additional Java basic types

typedef unsigned  char   jubyte;
typedef    unsigned __int16 jushort;
typedef unsigned __int32 juint;
typedef unsigned __int64 julong;

//----------------------------------------------------------------------
// Special (possibly not-portable) casts
// Cast floats into same-size integers and vice-versa w/o changing bit-pattern

inline jint    jint_cast   (jfloat  x)           { return *(jint*   )&x; }
inline jlong   jlong_cast  (jdouble x)           { return *(jlong*  )&x; }

inline jfloat  jfloat_cast (jint    x)           { return *(jfloat* )&x; }
inline jdouble jdouble_cast(jlong   x)           { return *(jdouble*)&x; }

//----------------------------------------------------------------------
// Debugging

void global_breakpoint();

#define BREAKPOINT { global_breakpoint(); }

//----------------------------------------------------------------------------
// Checking for NaN-ness

inline int g_isnan(jfloat  f)                    { return _isnan(f); }
inline int g_isnan(jdouble f)                    { return _isnan(f); }

//----------------------------------------------------------------------------
// Checking for finiteness

inline int g_isfinite(jfloat  f)                 { return _finite(f); }
inline int g_isfinite(jdouble f)                 { return _finite(f); }

//----------------------------------------------------------------------
// Constant for jlong (specifying an long long constant is C++
// compiler specific)

const jlong min_jlong = 0x8000000000000000L;
const jlong max_jlong = 0x7fffffffffffffffL;

//----------------------------------------------------------------------
// Miscellaneous

inline int vsnprintf(char* buf, size_t count, const char* fmt, va_list argptr)
{
  return _vsnprintf(buf, count, fmt, argptr);
}
