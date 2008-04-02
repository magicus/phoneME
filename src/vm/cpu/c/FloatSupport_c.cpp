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

#include "incls/_precompiled.incl"
#include "incls/_FloatSupport_c.cpp.incl"

#if ENABLE_FLOAT

#if defined(LINUX) && PROCESSOR_ARCHITECTURE_X86
#include <fpu_control.h>
#endif

#if ENABLE_INTERPRETER_GENERATOR
void FloatSupport::generate() {
}
#endif

extern "C" {
  void InitFPU() {
#if defined(LINUX) && PROCESSOR_ARCHITECTURE_X86
    /* Set the precision FPU to double precision */
    fpu_control_t cw = (_FPU_DEFAULT & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(cw);
#endif

  } 

  extern double fmod(double a, double b);
#define DOUBLE_REMAINDER(x, y) fmod(x, y)

#if PROCESSOR_ARCHITECTURE_X86
  extern double JFP_lib_dmul_x86(double lvalue, double rvalue);

  extern double JFP_lib_ddiv_x86(double lvalue, double rvalue);

  int JFP_lib_fcmpl_x86(float lvalue, float rvalue) {
    long  rrawbits = float_bits(rvalue);
    long  lrawbits = float_bits(lvalue);

    if (((lrawbits >= F_L_POS_NAN) && (lrawbits <= F_H_POS_NAN)) ||
	((lrawbits >= F_L_NEG_NAN) && (lrawbits <= F_H_NEG_NAN)) ||
	((rrawbits >= F_L_POS_NAN) && (rrawbits <= F_H_POS_NAN)) ||
	((rrawbits >= F_L_NEG_NAN) && (rrawbits <= F_H_NEG_NAN))) {
      return -1;
    } 

    return (lvalue >  rvalue) ?  1 : (lvalue == rvalue) ?  0 : -1;
  }

  int JFP_lib_fcmpg_x86(float lvalue, float rvalue) {
    jint  rrawbits = float_bits(rvalue);
    jint  lrawbits = float_bits(lvalue);
  
    if (((lrawbits >= F_L_POS_NAN) && (lrawbits <= F_H_POS_NAN)) ||
	((lrawbits >= F_L_NEG_NAN) && (lrawbits <= F_H_NEG_NAN)) ||
	((rrawbits >= F_L_POS_NAN) && (rrawbits <= F_H_POS_NAN)) ||
	((rrawbits >= F_L_NEG_NAN) && (rrawbits <= F_H_NEG_NAN))) {
      return 1;
    } 
  
    return (lvalue >  rvalue) ?  1 : (lvalue == rvalue) ?  0 : (lvalue <  rvalue) ? -1 : 1;
  }

  int JFP_lib_dcmpl_x86(double lvalue, double rvalue) {
    jlong rrawbits = double_bits(rvalue);
    jlong lrawbits = double_bits(lvalue);
  
    if (((lrawbits >= D_L_POS_NAN) && (lrawbits <= D_H_POS_NAN)) ||
	((lrawbits >= D_L_NEG_NAN) && (lrawbits <= D_H_NEG_NAN)) ||
	((rrawbits >= D_L_POS_NAN) && (rrawbits <= D_H_POS_NAN)) ||
	((rrawbits >= D_L_NEG_NAN) && (rrawbits <= D_H_NEG_NAN))) {
      return -1;
    }
    return  (lvalue >  rvalue) ?  1 : (lvalue == rvalue) ?  0 : -1;
  }

  int JFP_lib_dcmpg_x86(double lvalue, double rvalue) {
    jlong rrawbits = double_bits(rvalue);
    jlong lrawbits = double_bits(lvalue);

    if (((lrawbits >= D_L_POS_NAN) && (lrawbits <= D_H_POS_NAN)) ||
        ((lrawbits >= D_L_NEG_NAN) && (lrawbits <= D_H_NEG_NAN)) ||
        ((rrawbits >= D_L_POS_NAN) && (rrawbits <= D_H_POS_NAN)) ||
        ((rrawbits >= D_L_NEG_NAN) && (rrawbits <= D_H_NEG_NAN))) {
      return 1;
    } 
    return (lvalue >  rvalue) ?  1 : (lvalue == rvalue) ?  0 : (lvalue <  rvalue) ? -1 : 1;
  }

  float JFP_lib_frem_x86(float lvalue, float rvalue) {
    float result;
    jint  rrawbits = float_bits(rvalue);
    jint  lrawbits = float_bits(lvalue);

    if (((rrawbits == F_POS_INFINITY) || (rrawbits == F_NEG_INFINITY)) &&
        ((lrawbits & 0x7FFFFFFF) < F_POS_INFINITY)) {
      return lvalue;
    } 
    
    result = (float)DOUBLE_REMAINDER(lvalue, rvalue);
    /* Retrieve the sign bit to find +/- 0.0 */
    if ((lrawbits & 0x80000000) == 0x80000000) {
      if ((float_bits(result) & 0x80000000) != 0x80000000) {
	result *= -1.0f;
      }
    }

    return result;
  }


  double JFP_lib_drem_x86(double lvalue, double rvalue) {
    double result;
    jlong rrawbits = double_bits(rvalue);
    jlong lrawbits = double_bits(lvalue);

    if (((rrawbits == D_POS_INFINITY) || (rrawbits == D_NEG_INFINITY)) &&
        ((lrawbits & JVM_LL(0x7FFFFFFFFFFFFFFF)) < D_POS_INFINITY)) {
      return lvalue;
    }
    result = DOUBLE_REMAINDER(lvalue, rvalue);
    /* Retrieve the sign bit to find +/- 0.0 */
    if ((lrawbits & JVM_LL(0x8000000000000000)) == JVM_LL(0x8000000000000000)) {
      if ((double_bits(result) & JVM_LL(0x8000000000000000)) != JVM_LL(0x8000000000000000)) {
	result *= -1.0;
      }
    }
    return result;
  }

#endif /* PROCESSOR_ARCHITECTURE_X86 */

  // use compiler support
  jfloat jvm_fadd(jfloat x, jfloat y)            { return x + y; }

  jfloat jvm_fsub(jfloat x, jfloat y)            { return x - y; }

  jfloat jvm_fmul(jfloat x, jfloat y)            { return x * y; }

  jfloat jvm_fdiv(jfloat x, jfloat y)            { return x / y; }

  jfloat jvm_frem(jfloat x, jfloat y)            { 
#if PROCESSOR_ARCHITECTURE_X86
    /* On x86, we have to do some additional magic */
    /* to get correct strictfp results.            */
    return JFP_lib_frem_x86(x, y);
#else
    return (jfloat)DOUBLE_REMAINDER(x, y);
#endif
  }

  jdouble jvm_dadd(jdouble x, jdouble y)         { return x + y; }

  jdouble jvm_dsub(jdouble x, jdouble y)         { return x - y; }

  jdouble jvm_dmul(jdouble x, jdouble y)         { 
#if PROCESSOR_ARCHITECTURE_X86
    /* On x86, we have to do some additional magic */
    /* to get correct strictfp results.            */
    return JFP_lib_dmul_x86(x, y);
#else
    return x * y;
#endif
  }

  jdouble jvm_ddiv(jdouble x, jdouble y)         { 
#if PROCESSOR_ARCHITECTURE_X86
    /* On x86, we have to do some additional magic */
    /* to get correct strictfp results.            */
    return JFP_lib_ddiv_x86(x, y);
#else
    return x / y;
#endif
  }

  jdouble jvm_drem(jdouble x, jdouble y)         { 
#if PROCESSOR_ARCHITECTURE_X86
    /* On x86, we have to do some additional magic */
    /* to get correct strictfp results.            */
    return JFP_lib_drem_x86(x, y);
#else
    return DOUBLE_REMAINDER(x, y);
#endif
  }
  
  jdouble jvm_l2d(jlong x)                       { return (jdouble)x; }

  jint    jvm_f2i(jfloat x)                      { 
    jint rv, bits = float_bits(x);
    
    if ((bits & 0x7FFFFFFF) >= 0x4F000000) {
      if ((bits >= F_L_POS_NAN && bits <= F_H_POS_NAN) ||
	  (bits >= F_L_NEG_NAN && bits <= F_H_NEG_NAN)) {
	rv = 0;   /* NaN */
      } else if (bits > 0) {
	rv = MAX_INT;  /* +Infinity */
      } else if (bits < 0) {
	rv = MIN_INT;  /* -Infinity */
      } else {
	rv = 0;
      }
    } else {
      rv = (jint)x;
    }
    return rv;
  }

  jdouble jvm_f2d(jfloat x)                      { return (jdouble)x; }

  jlong   jvm_f2l(jfloat x)                      { 
    jint bits = float_bits(x);
    jlong rv;  
    /*
     * 0x5F000000 = (0x4F000000 + 2E28) magic number for Float
     * any number >= this number will be a special case
     */
    if ((bits & 0x7FFFFFFF) >= 0x5F000000) {
      if ((bits >= F_L_POS_NAN && bits <= F_H_POS_NAN) ||
	  (bits >= F_L_NEG_NAN && bits <= F_H_NEG_NAN)) {
	rv = 0;   /* NaN */
      } else if (bits > 0) {
	rv = MAX_LONG;  /* +Infinity */
      } else if (bits < 0) {
        rv = MIN_LONG;  /* -Infinity */
      } else {
	rv = 0;
      }
    } else {
      rv = (jlong)x;
    }
    return rv; 
  }

  jfloat  jvm_i2f(jint x)                        { return (jfloat)x; }

  jdouble jvm_i2d(jint x)                        { return (jdouble)x; }

  jfloat  jvm_l2f(jlong x)                       { return (jfloat)x; }

  jfloat  jvm_d2f(jdouble x)                     { return (jfloat)x; }  

  jlong   jvm_d2l(jdouble x)                     { 
    jlong bits = double_bits(x);
    jlong rv;
    /*
     * 0x43E0000000000000L = (0x41E0000000000000L + 2e57) magic number
     * for Float any number >= this number will be a special case
     */
    if ((bits & JVM_LL(0x7FFFFFFFFFFFFFFF)) >= JVM_LL(0x43E0000000000000)) {
      if ((bits >= D_L_POS_NAN && bits <= D_H_POS_NAN) ||
	  (bits >= D_L_NEG_NAN && bits <= D_H_NEG_NAN)) {
	rv =  0;   /* NaN */
      } else if (bits > 0) {
	rv =  MAX_LONG;  /* +Infinity */
      } else if (bits < 0) {
	rv =  MIN_LONG;  /* -Infinity */
      } else {
	rv = 0;
      }
    } else {
      rv = (jlong)x;
    }
    return rv; 
  }

  jint    jvm_d2i(jdouble x)                     { 
    jlong bits = double_bits(x);
    jint rv;
    if ((bits & JVM_LL(0x7FFFFFFFFFFFFFFF)) >= JVM_LL(0x41E0000000000000)) {
      if ((bits >= D_L_POS_NAN && bits <= D_H_POS_NAN) ||
	  (bits >= D_L_NEG_NAN && bits <= D_H_NEG_NAN)) {
	rv = 0;   /* NaN */
      } else if (bits > 0) {
	rv = MAX_INT;  /* +Infinity */
      } else if (bits < 0) {
	rv = MIN_INT;  /* -Infinity */
      } else {
	rv = 0;
      }
    } else {
      rv = (jint)x;
    }
    return rv; 
  }
  
  jint    jvm_fcmpg(jfloat x, jfloat y)          { 
#if PROCESSOR_ARCHITECTURE_X86
    return JFP_lib_fcmpg_x86(x, y); 
#else
    return  ((x > y)   ?  1   : 
	     (x == y)  ?  0 : 
	     (x < y)   ? -1 : 1);
#endif
  }

  jint    jvm_fcmpl(jfloat x, jfloat y)          {
#if PROCESSOR_ARCHITECTURE_X86
    return JFP_lib_fcmpl_x86(x, y); 
#else
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
#endif
  }

  jint    jvm_dcmpg(jdouble x, jdouble y)        { 
    #if PROCESSOR_ARCHITECTURE_X86
    return JFP_lib_dcmpg_x86(x, y); 
#else
    return  ((x > y)   ?  1   : 
	     (x == y)  ?  0 : 
	     (x < y)   ? -1 : 1);
#endif
  }
  
  jint    jvm_dcmpl(jdouble x, jdouble y)        { 
#if PROCESSOR_ARCHITECTURE_X86
    return JFP_lib_dcmpl_x86(x, y); 
#else
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
#endif
  }
    
  jdouble jvm_dneg(jdouble x)                    { return -x; } 

} // extern "C"

#endif // ENABLE_FLOAT
