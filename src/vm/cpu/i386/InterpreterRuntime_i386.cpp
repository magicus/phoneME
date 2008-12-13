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
#include "incls/_InterpreterRuntime_i386.cpp.incl"

extern "C" {

jlong jvm_lmul(jlong value1, jlong value2) { 
  return value1 * value2;
}

jlong jvm_lshl(jlong value1, jint value2) {
  value2 = value2 & 0x3f;
  return value1 << value2;
}

jlong jvm_lshr(jlong value1, jint value2) {
  value2 = value2 & 0x3f; // According to JVM spec.
  return value1 >> value2;
}

jlong jvm_lushr(jlong value1, jint value2) {
  value2 = value2 & 0x3f; // According to JVM spec.
  return (julong)value1 >> value2;
}

OopDesc* newarray(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaFrame frame(Thread::current());
  Method::Fast method = frame.method();
  BasicType type  = (BasicType) method().ubyte_at(frame.bci() + 1);
  int length = frame.expression_at(0)->as_int();

  GUARANTEE(method().bytecode_at(frame.bci()) == Bytecodes::_newarray,
            "Sanity check");
  TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);
  OopDesc *arr = Universe::new_type_array(array_class, length JVM_NO_CHECK_AT_BOTTOM);
  return arr;
}

address setup_stack_asm(address sp) {
  jint* xsp = (jint*)sp;
  *--xsp = (jint)Thread::start_lightweight_thread;
  *--xsp = 0;                 // value of frame pointer
  return (address)xsp;
}

#if ENABLE_INLINEASM_INTERPRETER
int memcmp_from_interpreter(const void *s1, const void *s2, size_t n) {
  return jvm_memcmp(s1, s2, n);
}
#endif
} // extern "C"
