/*
 *   
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
  jlong   jvm_f2l(jfloat x)              { return (jlong)x;   }

  JVM_SOFTFP_LINKAGE 
  jint    jvm_f2i(jfloat x)              { return (jint)x;    }  

  JVM_SOFTFP_LINKAGE 
  jlong   jvm_d2l(jdouble x)             { return (jlong)x;   }

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
