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

/*
 * sni.cpp: Synchronous Native Interface.
 */

#include "incls/_precompiled.incl"
#include "incls/_sni.cpp.incl"

void *SNI_GetRawArrayPointer(jarray array) {
  Array::Raw oop = *(OopDesc**) array;
  GUARANTEE(oop.not_null(), "null argument to SNI_GetRawArrayPointer()");
  return &((char*)oop.obj())[oop().base_offset()];
}

void *SNI_AllocateReentryData(int reentry_data_size) {
  SETUP_ERROR_CHECKER_ARG;
  return Scheduler::allocate_blocked_thread_data(reentry_data_size JVM_NO_CHECK);
}

void *SNI_GetReentryData(int* reentry_data_size) {
  return Scheduler::get_blocked_thread_data(reentry_data_size);
}

void SNI_BlockThread() {
  Scheduler::block_current_thread();
}

void SNI_UnblockThread(JVMSPI_ThreadID thread_id) {
  UsingFastOops fast_oops;
  Thread::Fast thread = (OopDesc*) thread_id;
  Scheduler::unblock_thread(&thread);
}

JVMSPI_BlockedThreadInfo *SNI_GetBlockedThreads(int *number_of_blocked_threads) {
  return Scheduler::get_blocked_threads(number_of_blocked_threads);
}

void SNI_SetSpecialThread(int isolateIndex) {
#if ENABLE_ISOLATES
  UsingFastOops fast_oops;
  TaskList::Fast tlist = Universe::task_list();
  GUARANTEE(!tlist().is_null(), "Task is NULL");
  Task::Fast task = tlist().obj_at(isolateIndex);
  task().set_special_thread(Thread::current());
#else
  (void)isolateIndex;
  *Universe::special_thread() = Thread::current()->obj();
#endif
}

JVMSPI_ThreadID SNI_GetSpecialThread(int isolateIndex) {
#if ENABLE_ISOLATES
  UsingFastOops fast_oops;
  TaskList::Fast tlist = Universe::task_list();
  GUARANTEE(!tlist().is_null(), "Task list is NULL");
  Task::Fast task = tlist().obj_at(isolateIndex);
  if (task.is_null()) {
    return NULL;
  } else {
    return task().special_thread();
  }
#else
  (void)isolateIndex;
  return Universe::special_thread()->obj();
#endif
}

void SNI_ClearSpecialThread(int isolateIndex) {
#if ENABLE_ISOLATES
  TaskList::Raw tlist = Universe::task_list();
  GUARANTEE(!tlist().is_null(), "Task is NULL");
  Task::Raw task = tlist().obj_at(isolateIndex);
  task().clear_special_thread();
#else
  (void)isolateIndex;
  Universe::special_thread()->set_null();
#endif
}

void *SNI_GetParameterAsRawPointer(jint index) {
  GUARANTEE(_jvm_in_raw_pointers_block > 0,
            "must be in SNI_BEGIN_RAW_POINTERS block!");

  /*
   * Return the Raw pointer to an Object on the stack
   * just past it's classpath
   */
  return (void *) GET_PARAMETER_AS_OOP(index);
}

KNIEXPORT void SNI_NewArray(jint type, jint size, jarray arrayHandle) {
  GUARANTEE(SNI_BOOLEAN_ARRAY == T_BOOLEAN, "sanity");
  GUARANTEE(SNI_CHAR_ARRAY    == T_CHAR,    "sanity");
  GUARANTEE(SNI_FLOAT_ARRAY   == T_FLOAT,   "sanity");
  GUARANTEE(SNI_DOUBLE_ARRAY  == T_DOUBLE,  "sanity");
  GUARANTEE(SNI_BYTE_ARRAY    == T_BYTE ,   "sanity");
  GUARANTEE(SNI_SHORT_ARRAY   == T_SHORT,   "sanity");
  GUARANTEE(SNI_INT_ARRAY     == T_INT,     "sanity");
  GUARANTEE(SNI_LONG_ARRAY    == T_LONG,    "sanity");

  SETUP_ERROR_CHECKER_ARG;
  OopDesc *array;

  if (SNI_BOOLEAN_ARRAY <= type && type <= SNI_LONG_ARRAY) {
    int index = Universe::bool_array_class_index + type - SNI_BOOLEAN_ARRAY;
    int element_size = byte_size_for((BasicType)type);
    TypeArrayClass* ac = (TypeArrayClass*)(&persistent_handles[index]);
    array = Universe::allocate_array(ac, size, element_size JVM_NO_CHECK);
  }
  else if (type == SNI_OBJECT_ARRAY) {
    array = Universe::new_obj_array(size JVM_NO_CHECK);
  }
  else {
    GUARANTEE(type == SNI_STRING_ARRAY, "sanity");
    array = Universe::new_obj_array(Universe::string_class(), size JVM_NO_CHECK);
  }

  *((OopDesc**)arrayHandle) = array;
  if (array == NULL) {
    // The caller is responsible to check for failure and throw
    // OutOfMemoryError if necessary.
    Thread::clear_current_pending_exception();
  }
}

KNIEXPORT void SNI_NewObjectArray(jclass elementType, jint size,
                                  jarray arrayHandle) {
  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  OopDesc *array;

  JavaClassObj::Fast mirror = *(OopDesc**)elementType;
  JavaClass::Fast klass = mirror().java_class();

  array = Universe::new_obj_array(&klass, size JVM_NO_CHECK);
  *((OopDesc**)arrayHandle) = array;
  if (array == NULL) {
    // The caller is responsible to check for failure and throw
    // OutOfMemoryError if necessary.
    Thread::clear_current_pending_exception();
  }
}

KNIEXPORT jint SNI_AddReference(jobject objectHandle, jboolean isStrong) {
  Oop oop = *(OopDesc**)objectHandle;
  const ObjectHeap::ReferenceType type =
      (isStrong) ? ObjectHeap::STRONG: ObjectHeap::WEAK;
  SETUP_ERROR_CHECKER_ARG;
  jint ret = ObjectHeap::register_global_ref_object(&oop, type JVM_MUST_SUCCEED);
  return ret;
}

KNIEXPORT void SNI_GetReference(jint ref, jobject objectHandle) {
  OopDesc* referent = ObjectHeap::get_global_ref_object(ref);
  *(OopDesc**)objectHandle = referent;
}

KNIEXPORT void SNI_DeleteReference(jint ref) {
  ObjectHeap::unregister_global_ref_object(ref);
}
