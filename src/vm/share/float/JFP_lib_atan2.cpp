/*
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

/* atan2(x, y)
 * Return atan2 function of x, y.
 *
 */
#include "incls/_precompiled.incl"
#include "incls/_JFP_lib_sin.cpp.incl"

#if ENABLE_FLOAT && ENABLE_CLDC_111

static const double
atan2_zero  = 0.0,
atan2_tiny  = 1.0e-300,
atan2_pi_o_4  = 7.8539816339744827900E-01, /* 0x3FE921FB, 0x54442D18 */
atan2_pi_o_2  = 1.5707963267948965580E+00, /* 0x3FF921FB, 0x54442D18 */
atan2_pi      = 3.1415926535897931160E+00, /* 0x400921FB, 0x54442D18 */
atan2_pi_lo   = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */

#ifdef __cplusplus
extern "C" {
#endif

double jvm_fplib_atan2(double x, double y) {
  double z;
  int k,m,hx,hy,ix,iy;
  unsigned lx,ly;

  hx = __JHI(x); ix = hx&0x7fffffff;
  lx = __JLO(x);
  hy = __JHI(y); iy = hy&0x7fffffff;
  ly = __JLO(y);
  if(((ix|((lx|-(int)lx)>>31))>0x7ff00000)||
     ((iy|((ly|-(int)ly)>>31))>0x7ff00000))	/* x or y is NaN */
    return x+y;
  if(((hx-0x3ff00000)|lx)==0) return jvm_fplib_atan(y);   /* x=1.0 */
  m = ((hy>>31)&1)|((hx>>30)&2);	/* 2*sign(x)+sign(y) */
  
  /* when y = 0 */
  if((iy|ly)==0) {
    switch(m) {
    case 0:
    case 1: return y; 	/* atan(+-0,+anything)=+-0 */
    case 2: return  atan2_pi+atan2_tiny;/* atan(+0,-anything) = pi */
    case 3: return -atan2_pi-atan2_tiny;/* atan(-0,-anything) =-pi */
    }
  }
  /* when x = 0 */
  if((ix|lx)==0) {
    return (hy<0)?  -atan2_pi_o_2-atan2_tiny: atan2_pi_o_2+atan2_tiny;
  }
  
  /* when x is INF */
  if(ix==0x7ff00000) {
    if(iy==0x7ff00000) {
      switch(m) {
      case 0: return  atan2_pi_o_4+atan2_tiny;/* atan(+INF,+INF) */
      case 1: return -atan2_pi_o_4-atan2_tiny;/* atan(-INF,+INF) */
      case 2: return  3.0*atan2_pi_o_4+atan2_tiny;/*atan(+INF,-INF)*/
      case 3: return -3.0*atan2_pi_o_4-atan2_tiny;/*atan(-INF,-INF)*/
      }
    } else {
      switch(m) {
      case 0: return  atan2_zero  ;	/* atan(+...,+INF) */
      case 1: return -atan2_zero  ;	/* atan(-...,+INF) */
      case 2: return  atan2_pi+atan2_tiny  ;	/* atan(+...,-INF) */
      case 3: return -atan2_pi-atan2_tiny  ;	/* atan(-...,-INF) */
      }
    }
  }
  /* when y is INF */
  if(iy==0x7ff00000) return (hy<0)? -atan2_pi_o_2-atan2_tiny: atan2_pi_o_2+atan2_tiny;
  
  /* compute y/x */
  k = (iy-ix)>>20;
  if(k > 60) z=atan2_pi_o_2+0.5*atan2_pi_lo; 	/* |y/x| >  2**60 */
  else if(hx<0&&k<-60) z=0.0; 	/* |y|/x < -2**60 */
  else z=jvm_fplib_atan(jvm_fabs(y/x));		/* safe to do y/x */
  switch (m) {
  case 0: return       z  ;	/* atan(+,+) */
  case 1: __JHI(z) ^= 0x80000000;
    return       z  ;	/* atan(-,+) */
  case 2: return  atan2_pi-(z-atan2_pi_lo);/* atan(+,-) */
  default: /* case 3 */
    return  (z-atan2_pi_lo)-atan2_pi;/* atan(-,-) */
  }
}

#ifdef __cplusplus
}
#endif

#endif // ENABLE_FLOAT && ENABLE_CLDC_111
