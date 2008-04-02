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
#include "incls/_FloatNatives.cpp.incl"

#if ENABLE_FLOAT

extern "C" {

JVM_SOFTFP_LINKAGE jdouble jvm_sin(jdouble x) {
  return jvm_fplib_sin(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_cos(jdouble x) {
  return jvm_fplib_cos(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_tan(jdouble x) {
  return jvm_fplib_tan(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_sqrt(jdouble x) {
  return ieee754_sqrt(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_ceil(jdouble x) {
  return jvm_fplib_ceil(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_floor(jdouble x) {
  return jvm_fplib_floor(x);
}

#if ENABLE_CLDC_111

JVM_SOFTFP_LINKAGE jdouble jvm_asin(jdouble x) {
  return jvm_fplib_asin(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_acos(jdouble x) {
  return jvm_fplib_acos(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_atan(jdouble x) {
  return jvm_fplib_atan(x);
}

JVM_SOFTFP_LINKAGE jdouble jvm_atan2(jdouble x, jdouble y) {
  return jvm_fplib_atan2(x, y);
}

#endif

// Note the name of this function follows the C library's fabs(), which
// takes a *double*. Incidentally, the version that takes in a *float* is named
// fabsf().
JVM_SOFTFP_LINKAGE jdouble jvm_fabs(jdouble x) {
  return jvm_fplib_fabs(x);
}

JVM_SOFTFP_LINKAGE jint jvm_floatToIntBits(jfloat x) {
  jint bits = float_bits(x);
  if ((bits & 0x7FFFFFFF) >= 0x5F000000) {
    if ((bits >= F_L_POS_NAN && bits <= F_H_POS_NAN) ||
	(bits >= F_L_NEG_NAN && bits <= F_H_NEG_NAN)) {
      bits = F_JUST_NAN;
    }
  }
  return bits;
}

JVM_SOFTFP_LINKAGE jlong jvm_doubleToLongBits(jdouble x) {
  jlong bits = double_bits(x);
  if ((bits & JVM_LL(0x7FFFFFFFFFFFFFFF)) >= JVM_LL(0x43E0000000000000)) {
    if ((bits >= D_L_POS_NAN && bits <= D_H_POS_NAN) ||
	(bits >= D_L_NEG_NAN && bits <= D_H_NEG_NAN)) {
      bits = D_JUST_NAN;
    }
  }
  return bits;
}

JVM_SOFTFP_LINKAGE jdouble jvm_double_from_msw_lsw(jint msw, jint lsw) {
  return jdouble_from_msw_lsw(msw, lsw);
}

JVM_SOFTFP_LINKAGE jint jvm_double_msw(jdouble d) {
  return __JHI(d);
}

JVM_SOFTFP_LINKAGE jint jvm_double_lsw(jdouble d) {
  return __JLO(d);
}

KNI_RETURNTYPE_FLOAT Java_java_lang_Float_intBitsToFloat() {
  return float_from_bits(KNI_GetParameterAsInt(1));
}

jint Java_java_lang_Float_floatToIntBits() {
  return jvm_floatToIntBits(KNI_GetParameterAsFloat(1));
}
  
KNI_RETURNTYPE_DOUBLE Java_java_lang_Double_longBitsToDouble() {
  return double_from_bits(KNI_GetParameterAsLong(1));
}

jlong Java_java_lang_Double_doubleToLongBits() {
  return jvm_doubleToLongBits(KNI_GetParameterAsDouble(1));
}
} // extern C

#endif // ENABLE_FLOAT
