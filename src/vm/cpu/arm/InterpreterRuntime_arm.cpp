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
#include "incls/_InterpreterRuntime_arm.cpp.incl"

extern "C" {

OopDesc* _newobject(Thread*thread, OopDesc* raw_klass JVM_TRAPS) {
  (void)thread;
  UsingFastOops fast_oops;
  InstanceClass::Fast klass = raw_klass;
  return klass().new_instance(ErrorOnFailure JVM_NO_CHECK_AT_BOTTOM_0);
}

jint _instanceof(Thread*thread, OopDesc* raw_object_class,      
                 OopDesc* raw_test_class) {
  (void)thread;
  JavaClass::Raw object_class = raw_object_class;
  JavaClass::Raw test_class = raw_test_class;

  return (object_class().is_subtype_of(&test_class) ? 1 : 0);
}


OopDesc* _anewarray(Thread *thread, OopDesc* raw_base_klass, int length
                    JVM_TRAPS) {
  UsingFastOops fast_oops;
  (void)thread;
  // Called from compiled code when it can't do the allocation
#ifdef AZZERT
  JavaFrame frame(thread);
  frame.verify();
#endif
  JavaClass::Fast base_klass = raw_base_klass;
  return Universe::new_obj_array(&base_klass, length JVM_NO_CHECK_AT_BOTTOM_0);
}

#if ENABLE_THUMB_COMPILER
  jlong jvm_lmul(jlong op1, jlong op2) {
    return op1 * op2;
  }

  jlong jvm_ladd(jlong op1, jlong op2) {
    return op1 + op2;
  }

  jlong jvm_lsub(jlong op1, jlong op2) {
    return op1 - op2;
  }

  jlong jvm_land(jlong op1, jlong op2) {
    return op1 & op2;
  }

  jlong jvm_lor(jlong op1, jlong op2) {
    return (julong)op1 | (julong)op2;
  }

  jlong jvm_lxor(jlong op1, jlong op2) {
    return (julong)op1 ^ (julong)op2;
  }

  jlong jvm_lshl(jlong op1, jint op2) {
    return ((julong)op1) << op2;
  }

  jlong jvm_lshr(jlong op1, jint op2) {
    return op1 >> op2;
  }

  jlong jvm_lushr(jlong op1, jint op2) {
    return ((julong)op1) >> op2;
  }

  jint jvm_lcmp(jlong op1, jlong op2) {
    return (op1 < op2 ?  -1 : op1 == op2 ? 0 : 1);
  }

  jlong jvm_lmin(jlong op1, jlong op2) {
    return ((op1 <= op2) ? op1 : op2);
  }

  jlong jvm_lmax(jlong op1, jlong op2) {
    return ((op1 >= op2) ? op1 : op2);
  }
#endif /*#if ENABLE_THUMB_COMPILER*/

  address setup_stack_asm(address sp) {
    jint* xsp = (jint*)sp;
    *xsp = TestCompiler;
      xsp += JavaStackDirection;
    *xsp = (jint)force_terminated;
      xsp += JavaStackDirection;
    *xsp = (jint)Thread::finish;
      xsp += JavaStackDirection;
    *xsp = (jint)Thread::lightweight_thread_uncaught_exception;
      xsp += JavaStackDirection;
    *xsp = (jint)Thread::lightweight_thread_exit;
      xsp += JavaStackDirection;
    *xsp = (jint)start_lightweight_thread_asm;
      xsp += JavaStackDirection;
    *xsp = 0;                   // value of frame pointer
    return (address)xsp;
  }
} // extern "C"
