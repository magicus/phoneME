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

/* __ieee754_rem_pio2(x,y)
 *
 * return the remainder of x rem pi/2 in y[0]+y[1]
 * use __kernel_rem_pio2()
 */

#include "incls/_precompiled.incl"
#include "incls/_IEEE754_rem_pio2.cpp.incl"

#if ENABLE_FLOAT

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Table of constants for 2/pi, 396 Hex digits (476 decimal) of 2/pi
 */
static const int two_over_pi[] = {
0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62,
0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129,
0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8,
0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5,
0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3,
0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
};

static const int npio2_hw[] = {
0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C,
0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C,
0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB, 0x403AB41B, 0x403C463A,
0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C,
0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB,
0x404858EB, 0x404921FB,
};

/*
 * invpio2:  53 bits of 2/pi
 * pio2_1:   first  33 bit of pi/2
 * pio2_1t:  pi/2 - pio2_1
 * pio2_2:   second 33 bit of pi/2
 * pio2_2t:  pi/2 - (pio2_1+pio2_2)
 * pio2_3:   third  33 bit of pi/2
 * pio2_3t:  pi/2 - (pio2_1+pio2_2+pio2_3)
 */

static const double
zero_r =  0.00000000000000000000e+00, /* 0x00000000, 0x00000000 */
half_r =  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
two24_r =  1.67772160000000000000e+07, /* 0x41700000, 0x00000000 */
invpio2 =  6.36619772367581382433e-01, /* 0x3FE45F30, 0x6DC9C883 */
pio2_1  =  1.57079632673412561417e+00, /* 0x3FF921FB, 0x54400000 */
pio2_1t =  6.07710050650619224932e-11, /* 0x3DD0B461, 0x1A626331 */
pio2_2  =  6.07710050630396597660e-11, /* 0x3DD0B461, 0x1A600000 */
pio2_2t =  2.02226624879595063154e-21, /* 0x3BA3198A, 0x2E037073 */
pio2_3  =  2.02226624871116645580e-21, /* 0x3BA3198A, 0x2E000000 */
pio2_3t =  8.47842766036889956997e-32; /* 0x397B839A, 0x252049C1 */

int ieee754_rem_pio2(double x, double *y) {
  double z, w, t, r, fn;
  double tx[3];
  int e0, i, j, nx, n, ix, hx;

  hx = __JHI(x);       /* high word of x */
  ix = hx & 0x7fffffff;
  if (ix <= 0x3fe921fb) {  /* |x| ~<= pi/4 , no need for reduction */
    y[0] = x;
    y[1] = 0;
    return 0;
  }
  if (ix < 0x4002d97c) {  /* |x| < 3pi/4, special case with n=+-1 */
    if (hx > 0) {
      z = jvm_dsub(x, pio2_1);
      if (ix != 0x3ff921fb) {    /* 33+53 bit pi is good enough */
        y[0] = jvm_dsub(z, pio2_1t);
        y[1] = jvm_dsub(jvm_dsub(z, y[0]), pio2_1t);
      } else {        /* near pi/2, use 33+33+53 bit pi */
        z = jvm_dsub(z, pio2_2);
        y[0] = jvm_dsub(z, pio2_2t);
        y[1] = jvm_dsub(jvm_dsub(z, y[0]), pio2_2t);
      }
      return 1;
    } else {    /* negative x */
      z = jvm_dadd(x, pio2_1);
      if (ix != 0x3ff921fb) {    /* 33+53 bit pi is good enough */
        y[0] = jvm_dadd(z, pio2_1t);
        y[1] = jvm_dadd(jvm_dsub(z,y[0]), pio2_1t);
      } else {        /* near pi/2, use 33+33+53 bit pi */
        z = jvm_dadd(z, pio2_2);
        y[0] = jvm_dadd(z, pio2_2t);
        y[1] = jvm_dadd(jvm_dsub(z, y[0]), pio2_2t);
      }
      return -1;
    }
  }
  if (ix <= 0x413921fb) { /* |x| ~<= 2^19*(pi/2), medium size */
    t  = jvm_fplib_fabs(x);
    n  = jvm_d2i(jvm_dadd(jvm_dmul(t, invpio2), half_r));
    fn = jvm_i2d(n);
    r  = jvm_dsub(t, jvm_dmul(fn, pio2_1));
    w  = jvm_dmul(fn, pio2_1t);    /* 1st round good to 85 bit */
    if (n < 32 && ix != npio2_hw[n-1]) {
      y[0] = jvm_dsub(r, w); /* quick check no cancellation */
    } else {
      j  = ix >> 20;
      y[0] = jvm_dsub(r, w);
      i = j - (((__JHI(y[0])) >> 20) & 0x7ff);
      if(i>16) {  /* 2nd iteration needed, good to 118 */
        t  = r;
        w  = jvm_dmul(fn, pio2_2);
        r  = jvm_dsub(t, w);
        w  = jvm_dsub(jvm_dmul(fn, pio2_2t), jvm_dsub(jvm_dsub(t,r),w));
        y[0] = jvm_dsub(r, w);
        i = j - (((__JHI(y[0])) >> 20) & 0x7ff);
        if (i > 49)  { /* 3rd iteration need, 151 bits acc */
          t  = r; /* will cover all possible cases */
          w  = jvm_dmul(fn, pio2_3);
          r  = jvm_dsub(t, w);
          w  = jvm_dsub(jvm_dmul(fn,pio2_3t), jvm_dsub(jvm_dsub(t,r), w));
          y[0] = jvm_dsub(r, w);
        }
      }
    }
    y[1] = jvm_dsub(jvm_dsub(r, y[0]), w);
    if (hx < 0) {
      y[0] = jvm_dneg(y[0]);
      y[1] = jvm_dneg(y[1]);
      return -n;
    } else {
      return n;
    }
  }
  /*
   * all other (large) arguments
   */
  if (ix >= 0x7ff00000) {        /* x is inf or NaN */
    y[0] = y[1] = jvm_dsub(x, x);
    return 0;
  }
  /* set z = scalbn(|x|,ilogb(x)-23) */
  e0  = (ix >> 20) - 1046;    /* e0 = ilogb(z)-23; */
  z = jdouble_from_msw_lsw(ix - (e0 << 20), __JLO(x));
  for (i = 0; i < 2; i++) {
    tx[i] = jvm_i2d(jvm_d2i(z));
    z     = jvm_dmul(jvm_dsub(z, tx[i]), two24_r);
  }
  tx[2] = z;
  nx = 3;
  while (jvm_dcmpl(tx[nx-1], zero_r) == 0) {
    nx--; /* skip zero term */
  }
  n  =  remainder_pio2_kernel(tx, y, e0, nx, 2, two_over_pi);
  if (hx < 0) {
    y[0] = jvm_dneg(y[0]);
    y[1] = jvm_dneg(y[1]);
    return -n;
  }
  return n;
}

#ifdef __cplusplus
}
#endif

#endif // ENABLE_FLOAT
