/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _TYPES_MD_H_
#define _TYPES_MD_H_

#include <sys/types.h>
#include <sys/stat.h>

#ifdef SOLARIS2
/* don't redefine typedef's on Solaris 2.6 or Later */

#if !defined(_ILP32) && !defined(_LP64)

#ifndef    _UINT64_T
#define    _UINT64_T
typedef unsigned long long uint64_t;
#define _UINT32_T
typedef unsigned long uint32_t;
#endif

#ifndef    _INT64_T
#define    _INT64_T
typedef long long int64_t;
#define _INT32_T
typedef long int32_t;
#endif

#endif    /* !defined(_ILP32) && !defined(_LP64) */
#endif /* SOLARIS2 */

#ifdef LINUX
#ifndef       _UINT64_T
#define       _UINT64_T
typedef unsigned long long uint64_t;
#define _UINT32_T
typedef unsigned long uint32_t;
#endif
#endif /* LINUX */

#ifdef WIN32
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef unsigned int uint_t;
#endif

/* use these macros when the compiler supports the long long type */

#define ll_high(a)    ((long)((a)>>32))
#define ll_low(a)    ((long)(a))
#define int2ll(a)    ((int64_t)(a))
#define ll2int(a)    ((int)(a))
#define ll_add(a, b)    ((a) + (b))
#define ll_and(a, b)    ((a) & (b))
#define ll_div(a, b)    ((a) / (b))
#define ll_mul(a, b)    ((a) * (b))
#define ll_neg(a)    (-(a))
#define ll_not(a)    (~(a))
#define ll_or(a, b)    ((a) | (b))
#define ll_shl(a, n)    ((a) << (n))
#define ll_shr(a, n)    ((a) >> (n))
#define ll_sub(a, b)    ((a) - (b))
#define ll_ushr(a, n)    ((unsigned long long)(a) >> (n))
#define ll_xor(a, b)    ((a) ^ (b))
#define uint2ll(a)    ((uint64_t)(unsigned long)(a))
#define ll_rem(a,b)    ((a) % (b))

#define INT_OP(x,op,y)  ((x) op (y))
#define NAN_CHECK(l,r,x) x
#define IS_NAN(x) isnand(x)

/* On i386 these conversions have to be method calls and not typecasts.
   See the win32 typedefs_md.h file */
#if defined(i386) || defined (__i386)

extern int32_t float2l(float f);
extern int32_t double2l(double d);
extern int64_t float2ll(float f);
extern int64_t double2ll(double d);

#else /* not i386 */

#define float2l(f)    (f)
#define double2l(f)    (f)
#define float2ll(f)    ((int64_t) (f))
#define double2ll(f)    ((int64_t) (f))

#endif /* i386 */

#define ll2float(a)    ((float) (a))
#define ll2double(a)    ((double) (a))

/* comparison operators */
#define ll_ltz(ll)    ((ll)<0)
#define ll_gez(ll)    ((ll)>=0)
#define ll_eqz(a)    ((a) == 0)
#define ll_eq(a, b)    ((a) == (b))
#define ll_ne(a,b)    ((a) != (b))
#define ll_ge(a,b)    ((a) >= (b))
#define ll_le(a,b)    ((a) <= (b))
#define ll_lt(a,b)    ((a) < (b))
#define ll_gt(a,b)    ((a) > (b))

#define ll_zero_const    ((int64_t) 0)
#define ll_one_const    ((int64_t) 1)

extern void ll2str(int64_t a, char *s, char *limit);

#ifdef ppc
#define HAVE_ALIGNED_DOUBLES
#define HAVE_ALIGNED_LONGLONGS
#endif

#ifdef SOLARIS2
#include <sys/byteorder.h>
#endif

#ifdef LINUX
#include <asm/byteorder.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#endif /* !_TYPES_MD_H_ */
