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
#include "incls/_VirtualStackFrame_i386.cpp.incl"

#if ENABLE_COMPILER

// Special casing of float/double constants 0.0 and 1.0
// Watch out for negative zero and nan

bool is_zero_float(jfloat f) {
  return jint_cast(f) == jint_cast(0.0f);
}

bool is_one_float(jfloat f) {
  return !g_isnan(f) && g_isfinite(f) && f == 1.0f;
}

bool is_zero_double(jdouble d) {
  return jlong_cast(d) == jlong_cast(0.0);
}

bool is_one_double(jdouble d) {
  return !g_isnan(d) && g_isfinite(d) && d == 1.0;
}

// Loading of float and double constants.
// On x86 loading float/double constants from a fixed address
// is much faster than materializing the constant on the stack.
// Since compiled methods may move at GC time, storing the constants
// inside the compiled method requires appropriate relocation info
// and fixup. For now, we just have two small global tables holding
// constants, and revert to slow case (materialize on stack) if the
// tables overflow. Most programs contain no or very few float/double
// constants.

const  int max_float_constants    = 16;
const  int max_double_constants   = 16;
static int float_constants_index  = 0;
static int double_constants_index = 0;
static jint  float_constants [max_float_constants ];
static jlong double_constants[max_double_constants];

address lookup_float_constant(jfloat value) {
  // Use raw bits as key
  jint v = jint_cast(value);
  // Search for existing float constant
  for (int i = 0; i < float_constants_index; i++) {
    if (v == float_constants[i]) {
      return (address) &float_constants[i];
    }
  }
  // Add new float constant to table
  if (float_constants_index < max_float_constants) {
    float_constants[float_constants_index] = v;
    return (address) &float_constants[float_constants_index++];
  }
  // Table overflow, revert to slow case
  return NULL;
}

address lookup_double_constant(jdouble value) {
  // Use raw bits as key
  jlong v = jlong_cast(value);
  // Search for existing double constant
  for (int i = 0; i < double_constants_index; i++) {
    if (v == double_constants[i]) {
      return (address) &double_constants[i];
    }
  }
  // Add new double constant to table
  if (double_constants_index < max_double_constants) {
    double_constants[double_constants_index] = v;
    return (address) &double_constants[double_constants_index++];
  }
  // Table overflow, revert to slow case
  return NULL;
}

bool VirtualStackFrame::flush_quick( void ) {
  // not supported on x86
  return false;
}

#ifndef PRODUCT
void VirtualStackFrame::dump_fp_registers(bool as_comment) const {
  if (!as_comment || !fpu_register_map().is_empty()) {
    // No need including a code comment when the FPU is empty
    fpu_register_map().dump( as_comment );
  }
}

#endif

#endif
