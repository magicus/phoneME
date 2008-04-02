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

/*
 * scalbn (double x, int n)
 * scalbn(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#include "incls/_precompiled.incl"
#include "incls/_JFP_lib_scalbn.cpp.incl"

#if ENABLE_FLOAT

#ifdef __cplusplus
extern "C" {
#endif

static const double
two54   =  1.80143985094819840000e+16, /* 0x43500000, 0x00000000 */
twom54  =  5.55111512312578270212e-17, /* 0x3C900000, 0x00000000 */
huge_number_sca   = 1.0e+300,
tiny_sca   = 1.0e-300;

double jvm_fplib_scalbn (double x, int n) {
  int  k, hx, lx;
  hx = __JHI(x);
  lx = __JLO(x);

  k = (hx & 0x7ff00000) >> 20;        /* extract exponent */
  if (k == 0) {             /* 0 or subnormal x */
    if ((lx | (hx & 0x7fffffff)) == 0) {
      return x; /* +-0 */
    }
    x = jvm_dmul(x, two54);
    hx = __JHI(x);
    k = ((hx & 0x7ff00000) >>20) - 54;
    if (n < -50000) {
      return jvm_dmul(tiny_sca, x);   /*underflow*/
    }
  }
  if (k == 0x7ff) {
    return jvm_dadd(x, x);       /* NaN or Inf */
  }
  k = k + n;
  if (k >  0x7fe) {
    return jvm_dmul(huge_number_sca, jvm_fplib_copysign(huge_number_sca,x)); /* overflow  */
  }
  if (k > 0) {             /* normal result */
    x = jdouble_from_msw_lsw((hx & 0x800fffff) | (k << 20), lx);
    return x;
  }
  if (k <= -54) {
   if (n > 50000) {  /* in case integer overflow in n+k */
     return jvm_dmul(huge_number_sca, jvm_fplib_copysign(huge_number_sca, x));    /*overflow*/
   } else {
     return jvm_dmul(tiny_sca, jvm_fplib_copysign(tiny_sca, x));   /*underflow*/
   }
  }
  k += 54;                /* subnormal result */
  x = jdouble_from_msw_lsw((hx & 0x800fffff) | (k << 20), lx);
  return jvm_dmul(x, twom54);
}

#ifdef __cplusplus
}
#endif

#endif // ENABLE_FLOAT
