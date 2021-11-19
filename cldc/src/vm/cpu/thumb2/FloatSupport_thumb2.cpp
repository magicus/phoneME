/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_FloatSupport_thumb2.cpp.incl"

#if ENABLE_FLOAT

#define floatNaN 0x7fc00000

#if ENABLE_INTERPRETER_GENERATOR

void FloatSupport::generate() {

  Segment seg(this, code_segment, "Floating point routines");
  generate_jvm_fadd();
  generate_jvm_fsub();
  generate_jvm_fmul();
  generate_jvm_fdiv();
  generate_jvm_frem();  
  generate_jvm_dcmp();
  generate_jvm_fcmp();
  generate_jvm_dadd();
  generate_jvm_dsub();
  generate_jvm_dmul();
  generate_jvm_ddiv();
  generate_jvm_drem();  
  
  generate_jvm_i2d();
  generate_jvm_i2f();
  generate_jvm_l2f();
  generate_jvm_l2d();
  generate_jvm_f2i();
  generate_jvm_f2l();
  generate_jvm_f2d();
  generate_jvm_d2i();
  generate_jvm_d2l();
  generate_jvm_d2f();
}


#if ENABLE_SOFT_FLOAT
#define STUBIT(name)                             \
    void FloatSupport::generate_jvm_ ## name() { \
    }

  STUBIT(fadd)
  STUBIT(fsub)
  STUBIT(fmul)
  STUBIT(fdiv)
  STUBIT(frem)  
  STUBIT(dadd)
  STUBIT(dsub)
  STUBIT(dmul)
  STUBIT(ddiv)
  STUBIT(drem)  
  STUBIT(fcmp)
  STUBIT(dcmp)

  STUBIT(i2f)
  STUBIT(i2d)
  STUBIT(l2f)
  STUBIT(l2d)
  STUBIT(f2i)
  STUBIT(f2l)
  STUBIT(f2d)
  STUBIT(d2i)
  STUBIT(d2l)
  STUBIT(d2f) 

#endif // ENABLE_SOFT_FLOAT   
#endif // ENABLE_INTERPRETER_GENERATOR

extern "C" {

#if !ENABLE_INTERPRETER_GENERATOR && !CROSS_GENERATOR

  JVM_SOFTFP_LINKAGE 
  jfloat jvm_fadd(jfloat x, jfloat y)    { return x + y; }

  JVM_SOFTFP_LINKAGE 
  jfloat jvm_fsub(jfloat x, jfloat y)    { return x - y; }

  JVM_SOFTFP_LINKAGE 
  jfloat jvm_fmul(jfloat x, jfloat y)    { return x * y; }

  JVM_SOFTFP_LINKAGE 
  jfloat jvm_fdiv(jfloat x, jfloat y)    { return x / y; }

  JVM_SOFTFP_LINKAGE 
  jfloat jvm_frem(jfloat x, jfloat y)    { 
    return jvm_d2f(ieee754_fmod(jvm_f2d(x), jvm_f2d(y)));
  }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_dadd(jdouble x, jdouble y) { return x + y; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_dsub(jdouble x, jdouble y) { return x - y; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_dmul(jdouble x, jdouble y) { return x * y; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_ddiv(jdouble x, jdouble y) { return x / y; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_drem(jdouble x, jdouble y) { 
    return ieee754_fmod(x, y);
  }

  JVM_SOFTFP_LINKAGE 
  jfloat  jvm_i2f(jint x)                { return (jfloat)x;  }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_i2d(jint x)                { return (jdouble)x; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_l2d(jlong x)               { return (jdouble)x; }

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_f2d(jfloat x)              { return (jdouble)x; }

  JVM_SOFTFP_LINKAGE 
  jfloat  jvm_l2f(jlong x)               { return (jfloat)x;  }

  JVM_SOFTFP_LINKAGE 
  jfloat  jvm_d2f(jdouble x)             { return (jfloat)x;  }

  JVM_SOFTFP_LINKAGE 
  jlong   jvm_f2l(jfloat x)              {
    union {
      jint output;
      jfloat input;
    } convert;

    convert.input = x;

    jint raw = convert.output;

    // expp is the unbiased exponent and mantissa
    jint expp = (raw & 0x7FFFFFFF) - (0x7F << 23);
    if (expp < 0) {
      // value is less 1.0.  We can just return 0.
      return 0;
    } else if (expp >= (63 << 23)) {
      // Value is either too large to fit into a long, or else it is NaN
      if (expp > ((0xFF - 0x7F) << 23)) {
        // Nan.  By definition, we return 0.
        return 0;
      } else {
        return raw < 0 ? min_jlong : max_jlong;
      }
    } else {
      // The implicit mantissa
      juint  mantissa = (raw << 8) | 0x80000000;
      // The amount by which this has to be shifted
      int shift = 63 - (expp >> 23);
      GUARANTEE(shift >= 1 && shift <= 63, "Sanity");

      julong uresult = jlong_from_msw_lsw(mantissa, 0);
      uresult =  uresult >> shift;

      jlong result = (raw < 0) ?  -(jlong)uresult : (jlong)uresult;

      //printf("f2l: %f %lld raw=%d %lld\n", x, result, raw, uresult);

      return result;
    }
  }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_f2i(jfloat x)              { return (jint)x;    }  

  JVM_SOFTFP_LINKAGE 
  jlong   jvm_d2l(jdouble x)             { 
    juint raw_hi = __JHI(x);
    juint raw_lo = __JLO(x);
    jint expp = (raw_hi & 0x7FFFFFFF) - (right_n_bits(11 - 1) << 20);
    if (expp < 0) {
      return 0;
    } else if (expp >= (63 << 20)) {
      if (expp >= ((0x7FF - right_n_bits(10)) << 20)) {
      // Infinite or NaN.  Very rare, I hope
        if (expp > ((0x7FF - right_n_bits(10)) << 20) || raw_lo != 0) {
          // NaN
          return 0;
        }
      }
      return ((jint)raw_hi) < 0 ? min_jlong : max_jlong;
    } else {
      juint mantissa_hi =  (((raw_hi & right_n_bits(20)) << 11) + (raw_lo >> 21))
        | 0x80000000;
      juint mantissa_lo = (raw_lo << 11);
      julong mantissa = jlong_from_msw_lsw(mantissa_hi, mantissa_lo);
      int shift = 63 - (expp >> 20);
      GUARANTEE(shift >= 1 && shift <= 63, "Sanity");
      julong uresult = mantissa >> shift;
      jlong result = ((jint)raw_hi < 0) ?  -(jlong)uresult : (jlong)uresult;
      return result;
    }
  }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_d2i(jdouble x)             { return (jint)x;    }
  

  JVM_SOFTFP_LINKAGE 
  jint    jvm_fcmpl(jfloat x, jfloat y)  {
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
  }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_dcmpl(jdouble x, jdouble y) {
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
  }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_fcmpg(jfloat x, jfloat y)   { 
    return  ((x > y)   ?  1   : 
             (x == y)  ?  0 : 
             (x < y)   ? -1 : 1);
  }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_dcmpg(jdouble x, jdouble y)  { 
    return  ((x > y)   ?  1   : 
             (x == y)  ?  0 : 
             (x < y)   ? -1 : 1);
  }

#endif // !ENABLE_INTERPRETER_GENERATOR && !CROSS_GENERATOR

  JVM_SOFTFP_LINKAGE 
  jdouble jvm_dneg(jdouble x)           { return -x; }

} // extern "C"


#endif // ENABLE_FLOAT
