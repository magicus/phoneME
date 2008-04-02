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

#if ENABLE_FLOAT

// Macros for NaN (Not-A-Number) and Infinity for floats and doubles
#define F_POS_INFINITY    0x7F800000
#define F_NEG_INFINITY    0xFF800000
#define F_L_POS_NAN       0x7F800001
#define F_H_POS_NAN       0x7FFFFFFF
#define F_JUST_NAN        0x7FC00000
#define F_L_NEG_NAN       0xFF800001
#define F_H_NEG_NAN       0xFFFFFFFF


#define D_POS_INFINITY    JVM_LL (0x7FF0000000000000)
#define D_NEG_INFINITY    JVM_ULL(0xFFF0000000000000)
#define D_L_POS_NAN       JVM_LL (0x7FF0000000000001)
#define D_H_POS_NAN       JVM_LL (0x7FFFFFFFFFFFFFFF)
#define D_JUST_NAN        JVM_LL (0x7ff8000000000000)
#define D_L_NEG_NAN       JVM_ULL(0xFFF0000000000001)
#define D_H_NEG_NAN       JVM_ULL(0xFFFFFFFFFFFFFFFF)

// MAX and MIN values for INT and LONG
#define MAX_INT           0x7FFFFFFF
#define MIN_INT           0x80000000
#define MAX_LONG          max_jlong
#define MIN_LONG          min_jlong

// Macros used by the trigonometric functions
// Note: never use those as LHS, as it leads to subtle 
//       aliasing problems, use jdouble_from_msw_lsw instead
#if MSW_FIRST_FOR_DOUBLE
  #define __JHI(x) (((jdouble_accessor *)&x)->words[0])
  #define __JLO(x) (((jdouble_accessor *)&x)->words[1])
#else
  #define __JHI(x) (((jdouble_accessor *)&x)->words[1])
  #define __JLO(x) (((jdouble_accessor *)&x)->words[0])
#endif

/*=========================================================================
 * Function prototypes for the internal library routines that we use
 *=======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

double sine_kernel(double x, double y, int iy);
double cosine_kernel(double x, double y);
double tangent_kernel(double x, double y, int iy);
int    remainder_pio2_kernel(double *x, double *y, int e0, int nx, int prec, const int *ipio2);

double ieee754_sqrt(double x);
int    ieee754_rem_pio2(double x, double *y);

double ieee754_fmod(double x, double y);

double jvm_fplib_copysign(double x, double y);
double jvm_fplib_fabs(double x);
double jvm_fplib_scalbn(double x, int n);

double jvm_fplib_sin(double x);
double jvm_fplib_cos(double x);
double jvm_fplib_tan(double x);
double jvm_fplib_floor(double x);
double jvm_fplib_ceil(double x);
double jvm_fplib_sqrt(double x);

#if ENABLE_CLDC_111

double jvm_fplib_asin(double x);
double jvm_fplib_acos(double x);
double jvm_fplib_atan(double x);
double jvm_fplib_atan2(double x, double y);

#endif

#ifdef __cplusplus
}
#endif

#endif  // ENABLE_FLOAT
