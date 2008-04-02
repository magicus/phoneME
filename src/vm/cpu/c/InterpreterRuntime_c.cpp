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
#include "incls/_InterpreterRuntime_c.cpp.incl"

extern "C" {

OopDesc* _newobject(Thread*thread, OopDesc* raw_klass JVM_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast klass = raw_klass;
  return klass().new_instance(ErrorOnFailure JVM_NO_CHECK_AT_BOTTOM_0);
}

jint _instanceof(Thread*thread, OopDesc* raw_object_class,      
                 OopDesc* raw_test_class) {
  (void)thread;
  InstanceClass::Raw object_class = raw_object_class;
  InstanceClass::Raw test_class = raw_test_class;

  return (object_class().is_subtype_of(&test_class) ? 1 : 0);
}

OopDesc* _anewarray(Thread *thread, OopDesc* raw_base_klass, int length
                    JVM_TRAPS) {
  UsingFastOops fast_oops;
  // Called from compiled code when it can't do the allocation
#ifdef AZZERT
  JavaFrame frame(thread);
  frame.verify();
#endif
  JavaClass::Fast base_klass = raw_base_klass;
  return Universe::new_obj_array(&base_klass, length JVM_NO_CHECK_AT_BOTTOM_0);
}

address setup_stack_asm(address xsp) {
  address* sp = (address*)xsp;
  *--sp = NULL; // frame pointer
  *--sp = NULL; // dummy
  *--sp = NULL; // return_point
  return (address)sp;
}

} // extern "C"
