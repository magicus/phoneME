/*
 * @(#)jfdlibm.h	1.18 06/10/10
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
 *
 */

#ifndef _JFDLIBM_H
#define _JFDLIBM_H

#define _IEEE_LIBM

/*
 * In order to resolve the conflict between fdlibm and compilers
 * (such as keywords and built-in functions), the following
 * function names have to be re-mapped.
 */

#define huge    HUGE_NUMBER

#define acos CVMfdlibmAcos
#define asin CVMfdlibmAsin
#define atan CVMfdlibmAtan
#define atan2 CVMfdlibmAtan2
#define cos CVMfdlibmCos
#define sin CVMfdlibmSin
#define tan CVMfdlibmTan

#define cosh CVMfdlibmCosh
#define sinh CVMfdlibmSinh
#define tanh CVMfdlibmTanh

#define exp CVMfdlibmExp
#define frexp CVMfdlibmFrexp
#define ldexp CVMfdlibmLdexp
#define log CVMfdlibmLog
#define log10 CVMfdlibmLog10
#define modf CVMfdlibmModf

#define pow CVMfdlibmPow
#define sqrt CVMfdlibmSqrt

#define ceil CVMfdlibmCeil
#define fabs CVMfdlibmFabs
#define floor CVMfdlibmFloor
#define fmod CVMfdlibmFmod

#define erf CVMfdlibmErf
#define erfc CVMfdlibmErfc
#define gamma CVMfdlibmGamma
#define hypot CVMfdlibmHypot
#undef isnan
#define isnan CVMfdlibmIsnan
#define finite CVMfdlibmFinite
#define j0 CVMfdlibmJ0
#define j1 CVMfdlibmJ1
#define jn CVMfdlibmJn
#define lgamma CVMfdlibmLgamma
#define y0 CVMfdlibmY0
#define y1 CVMfdlibmY1
#define yn CVMfdlibmYn

#define acosh CVMfdlibmAcosh
#define asinh CVMfdlibmAsinh
#define atanh CVMfdlibmAtanh
#define cbrt CVMfdlibmCbrt
#define logb CVMfdlibmLogb
#define nextafter CVMfdlibmNextafter
#define remainder CVMfdlibmRemainder
#define scalb CVMfdlibmScalb

/* We need to #undef this because it conflicts with math.h on win32 */
#undef matherr
#define matherr CVMfdlibmMatherr

/*
 * IEEE Test Vector
 */
#define significand CVMfdlibmSignificand

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
#define copysign CVMfdlibmCopysign
#define ilogb CVMfdlibmIlogb
#define rint CVMfdlibmRint
#define scalbn CVMfdlibmScalbn

/*
 * BSD math library entry points
 */
#define expm1 CVMfdlibmExpm1
#define log1p CVMfdlibmLog1p

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
#define gamma_r CVMfdlibmGamma_r
#define lgamma_r CVMfdlibmLgamma_r

/* ieee style elementary functions */
#define __ieee754_sqrt CVMfdlibm__ieee754_sqrt
#define __ieee754_acos CVMfdlibm__ieee754_acos
#define __ieee754_acosh CVMfdlibm__ieee754_acosh
#define __ieee754_log CVMfdlibm__ieee754_log
#define __ieee754_atanh CVMfdlibm__ieee754_atanh
#define __ieee754_asin CVMfdlibm__ieee754_asin
#define __ieee754_atan2 CVMfdlibm__ieee754_atan2
#define __ieee754_exp CVMfdlibm__ieee754_exp
#define __ieee754_cosh CVMfdlibm__ieee754_cosh
#define __ieee754_fmod CVMfdlibm__ieee754_fmod
#define __ieee754_pow CVMfdlibm__ieee754_pow
#define __ieee754_lgamma_r CVMfdlibm__ieee754_lgamma_r
#define __ieee754_gamma_r CVMfdlibm__ieee754_gamma_r
#define __ieee754_lgamma CVMfdlibm__ieee754_lgamma
#define __ieee754_gamma CVMfdlibm__ieee754_gamma
#define __ieee754_log10 CVMfdlibm__ieee754_log10
#define __ieee754_sinh CVMfdlibm__ieee754_sinh
#define __ieee754_hypot CVMfdlibm__ieee754_hypot
#define __ieee754_j0 CVMfdlibm__ieee754_j0
#define __ieee754_j1 CVMfdlibm__ieee754_j1
#define __ieee754_y0 CVMfdlibm__ieee754_y0
#define __ieee754_y1 CVMfdlibm__ieee754_y1
#define __ieee754_jn CVMfdlibm__ieee754_jn
#define __ieee754_yn CVMfdlibm__ieee754_yn
#define __ieee754_remainder CVMfdlibm__ieee754_remainder
#define __ieee754_rem_pio2 CVMfdlibm__ieee754_rem_pio2
#define __ieee754_scalb CVMfdlibm__ieee754_scalb

/* fdlibm kernel function */
#define __kernel_standard CVMfdlibm__kernel_standard
#define __kernel_sin CVMfdlibm__kernel_sin
#define __kernel_cos CVMfdlibm__kernel_cos
#define __kernel_tan CVMfdlibm__kernel_tan
#define __kernel_rem_pio2 CVMfdlibm__kernel_rem_pio2

/* ETC */
#define signgam CVMfdlibmSigngam

#endif/*_JFDLIBM_H*/
