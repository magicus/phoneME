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

/* __kernel_sin( x, y, iy)
 * kernel sin function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input iy indicates whether y is 0. (if iy=0, y assume to be 0).
 *
 * Algorithm
 *  1. Since sin(-x) = -sin(x), we need only to consider positive x.
 *  2. if x < 2^-27 (hx<0x3e400000 0), return x with inexact if x!=0.
 *  3. sin(x) is approximated by a polynomial of degree 13 on
 *     [0,pi/4]
 *                   3            13
 *      sin(x) ~ x + S1*x + ... + S6*x
 *     where
 *
 *  |sin(x)         2     4     6     8     10     12  |     -58
 *  |----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
 *  |  x                               |
 *
 *  4. sin(x+y) = sin(x) + sin'(x')*y
 *          ~ sin(x) + (1-x*x/2)*y
 *     For better accuracy, let
 *           3      2      2      2      2
 *      r = x *(S2+x *(S3+x *(S4+x *(S5+x *S6))))
 *     then                   3    2
 *      sin(x) = x + (S1*x + (x *(r-y/2)+y))
 */

#include "incls/_precompiled.incl"
#include "incls/_Sine_kernel.cpp.incl"

#if ENABLE_FLOAT

#ifdef __cplusplus
extern "C" {
#endif

static const double
half_s =  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
S1  = -1.66666666666666324348e-01, /* 0xBFC55555, 0x55555549 */
S2  =  8.33333333332248946124e-03, /* 0x3F811111, 0x1110F8A6 */
S3  = -1.98412698298579493134e-04, /* 0xBF2A01A0, 0x19C161D5 */
S4  =  2.75573137070700676789e-06, /* 0x3EC71DE3, 0x57B1FE7D */
S5  = -2.50507602534068634195e-08, /* 0xBE5AE5E6, 0x8A2B9CEB */
S6  =  1.58969099521155010221e-10; /* 0x3DE5D93A, 0x5ACFD57C */

double sine_kernel(double x, double y, int iy) {
  double z, r, v;
  int ix;
  ix = __JHI(x) & 0x7fffffff;    /* high word of x */
  if (ix < 0x3e400000) {  /* |x| < 2**-27 */
    if (jvm_d2i(x) == 0) {
      return x; /* generate inexact */
    }
  }
  z   =  jvm_dmul(x, x);
  v   =  jvm_dmul(z, x);
  r   =  jvm_dadd(S2, jvm_dmul(z, (jvm_dadd(S3, jvm_dmul(z, (jvm_dadd(S4, (jvm_dmul(z, (jvm_dadd(S5, jvm_dmul(z,S6))))))))))));
  if (iy == 0) {
    return jvm_dadd(x, jvm_dmul(v,(jvm_dadd(S1, jvm_dmul(z,r)))));
  } else {
    return jvm_dsub(x, jvm_dsub((jvm_dsub(jvm_dmul(z, (jvm_dsub(jvm_dmul(half_s,y), jvm_dmul(v,r)))), y)), jvm_dmul(v, S1)));
  }
}

#ifdef __cplusplus
}
#endif

#endif // ENABLE_FLOAT
