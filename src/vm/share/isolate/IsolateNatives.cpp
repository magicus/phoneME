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
#include "incls/_IsolateNatives.cpp.incl"

extern "C" {

/*----------------------------------------------------------------
 * Isolate support
 *----------------------------------------------------------------*/

// private native static Isolate[] getIsolates0();
ReturnOop Java_com_sun_cldc_isolate_Isolate_getIsolates0(JVM_SINGLE_ARG_TRAPS){
  return Task::current()->get_visible_active_isolates(
                  JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

// private native static Isolate currentIsolate0();
ReturnOop Java_com_sun_cldc_isolate_Isolate_currentIsolate0() {
  return Task::current()->primary_isolate_obj();
}

// private native void registerNewIsolate();
void Java_com_sun_cldc_isolate_Isolate_registerNewIsolate() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  isolate_obj().assign_unique_id();
  Task::current()->add_to_seen_isolates(&isolate_obj);
}

// private native int usedMemory0();
int Java_com_sun_cldc_isolate_Isolate_usedMemory0() {
  IsolateObj::Raw isolate = GET_PARAMETER_AS_OOP(0);
  const jint task = isolate().task_id();
  return task == Task::INVALID_TASK_ID ? 0 :
    ObjectHeap::get_task_memory_estimate( task );
}

/*
 * void nativeStart(Object [] startupState)
 * where startupState contains the following:
 * String classname
 * String [] args
 * String [][] context
 *
 * Note: all objects passed in parameters belongs to the new isolate
 * and no copy are necessary. See comments in com.sun.cldc.isolate.Isolate.
 */
void Java_com_sun_cldc_isolate_Isolate_nativeStart(JVM_SINGLE_ARG_TRAPS) {
  Thread::Raw saved;
  bool has_exception = false;
  const int id = Task::allocate_task_id(JVM_SINGLE_ARG_CHECK);
  {
    UsingFastOops fast_oops;
    IsolateObj::Fast isolate = GET_PARAMETER_AS_OOP(0);
    ObjectHeap::set_task_memory_quota(id, isolate().memory_reserve(),
                                          isolate().memory_limit() JVM_CHECK);

    const int prev = ObjectHeap::on_task_switch( id );
    saved = Task::create_task(id, &isolate JVM_NO_CHECK);    
    ObjectHeap::on_task_switch( prev );
    
    if( saved.is_null() ) {
      ObjectHeap::reset_task_memory_usage(id);
      Task::cleanup_unstarted_task(id);
      isolate().terminate(-1 JVM_NO_CHECK_AT_BOTTOM);
      return;
    }
  }

  // save current thread pointer, no non-raw handles please!
  Thread::Raw current = Thread::current();
  Thread::set_current(&saved);
  {
    // handles ok now
    UsingFastOops fast_oops;
    Thread::Fast orig = &current();
    Thread::Fast t = &saved();
    // IMPL_NOTE: SHOULDN'T WE DO SOMETHING HERE IF THERE IS SOME EXCEPTION
    // RELATED TO STARTUP FAILURE (i.e., reclaim the task's id and
    // proceed to task termination cleanup, including sending of
    // isolate events)
    Task::start_task(&t JVM_NO_CHECK);
    has_exception = (CURRENT_HAS_PENDING_EXCEPTION != NULL);
    
    current = orig;  // GC may have happened. Reload.
    saved = t;       // GC may have happened. Reload.
  }

  // we set the current thread back to the original so that the C interpreter
  // will switch threads correctly.  The new thread/task is set to be the
  // next thread/task to run when switch_thread is called a few instructions
  // from now.
  Thread::set_current(&current);
  if (has_exception) {
    UsingFastOops fast_oops;
    // new isolate got an exception somewhere during start_task().  The isolate
    // may not be able to handle the exception since we don't know at which
    // point it got the exception. Some classes may not be initialized,
    // task mirrors may not be setup etc.  We just tear down the new isolate
    // and return an exception to the parent.
    Task::Fast task = Task::get_task(id);
    IsolateObj::Fast isolate = GET_PARAMETER_AS_OOP(0);
    Thread::Fast thrd = &saved();
    GUARANTEE(!task.is_null(), "Task should not be null at this point");
    task().set_status(Task::TASK_STOPPED);
    task().set_thread_count(0);
    isolate().terminate(-1 JVM_NO_CHECK);
    saved = thrd;       // GC may have happened. Reload.
  }
  if (has_exception) {
    // Convoluted dance here.  Child isolate failed to start
    // We check the type of exception
    //  if (exception == oome)
    //    throw oome to parent
    //  else if (exception == isolateresourceerror)
    //    throw IsolateResourceError to parent
    //  else
    //    throw java.lang.Error to parent
    // Make sure we don't have any handles to child objects before
    // calling Task::cleanup_terminated_task()
    //    
    UsingFastOops fast_oops;
    Thread::Fast thrd = &saved();
    JavaOop::Fast exception = thrd().noncurrent_pending_exception();
    String::Fast str;
    {
      ThreadObj::Raw thread_obj = thrd().thread_obj();
      // Mark thread as terminated even though it never really started
      if (!thread_obj.is_null()) {
        thread_obj().set_terminated();
      }
    }
    Scheduler::terminate(&thrd JVM_NO_CHECK);
    thrd.set_null();  // thrd handle has to be disposed here

    JavaOop::Fast new_exception = Universe::out_of_memory_error_instance();
    JavaClass::Fast exception_class = exception().blueprint();
    // oome class *must* be loaded in parent, how can it not be?
    InstanceClass::Fast oome_class =
      SystemDictionary::resolve(Symbols::java_lang_OutOfMemoryError(),
                               ErrorOnFailure JVM_NO_CHECK);
    InstanceClass::Fast ire_class =
      SystemDictionary::resolve(Symbols::com_sun_cldc_isolate_IsolateResourceError(),
                               ErrorOnFailure JVM_NO_CHECK);

    if( oome_class.not_null() && !oome_class.equals(&exception_class)
        && ire_class.not_null() ) {
      new_exception = Throw::allocate_exception(
        ire_class.equals(&exception_class) ? 
          Symbols::com_sun_cldc_isolate_IsolateResourceError() :
          Symbols::java_lang_Error(),
        &str JVM_NO_CHECK);
    }
    exception_class.set_null(); // cleared handles to child objects
    exception.set_null();       //
    Task::cleanup_terminated_task(id JVM_NO_CHECK);
    Thread::set_current_pending_exception(&new_exception);
  }
}

/* void stop(int exit_code, int exit_reason); */
void Java_com_sun_cldc_isolate_Isolate_stop(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  IsolateObj::Fast isolate_obj = GET_PARAMETER_AS_OOP(0);
  Task::Fast task = isolate_obj().task();
  const int exit_code = KNI_GetParameterAsInt(1);
  const int exit_reason = KNI_GetParameterAsBoolean(2);

  switch (isolate_obj().status()) {
  case Task::TASK_NEW:
    isolate_obj().terminate(exit_code JVM_NO_CHECK_AT_BOTTOM);
    // Isolate was never started, so no threads to kill.
    break;

  case Task::TASK_STARTED:
    GUARANTEE(task.not_null(), "Task must exist for running Isolate");
    task().stop(exit_code, exit_reason JVM_NO_CHECK_AT_BOTTOM);
    break;

  default:
    // OK to call stop() multiple times. Just ignore it.
    break;
  }
}

/* int getSatus() */
jint Java_com_sun_cldc_isolate_Isolate_getStatus() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  return isolate_obj().status();
}

void Java_com_sun_cldc_isolate_Isolate_suspend0() {
  UsingFastOops fast_oops;
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  Task::Fast task = isolate_obj().task();
  if (task.not_null()) {
    task().suspend();
  }
}

int Java_com_sun_cldc_isolate_Isolate_isSuspended0() {
  UsingFastOops fast_oops;
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  Task::Fast task = isolate_obj().task();
  return task.not_null() ? task().is_suspended() : 0;
}

void Java_com_sun_cldc_isolate_Isolate_resume0() {
  UsingFastOops fast_oops;
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  Task::Fast task = isolate_obj().task();
  if (task.not_null()) {
    task().resume();
  }
}

void Java_com_sun_cldc_isolate_Isolate_setPriority0() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  const jint new_priority = KNI_GetParameterAsInt(1);
  // new_priority tested at java level, should never be out of range
  GUARANTEE(new_priority >= Task::PRIORITY_MIN &&
            new_priority <= Task::PRIORITY_MAX, "priority out of range");
  Task::Raw t = isolate_obj().task();
  if (t.not_null()) {
    t().set_priority(new_priority);
  }
}

jint Java_com_sun_cldc_isolate_Isolate_getPriority0() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  return isolate_obj().priority();
}

// private native int id0();
jint Java_com_sun_cldc_isolate_Isolate_id0() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  return isolate_obj().task_id();
}

// private native void waitStatus(int maxStatus) throws InterruptedException;
void Java_com_sun_cldc_isolate_Isolate_waitStatus(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  IsolateObj::Fast isolate_obj = GET_PARAMETER_AS_OOP(0);
  const jint maxStatus = KNI_GetParameterAsInt(1);
  if (isolate_obj().status() < maxStatus) {
    Scheduler::wait(&isolate_obj, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
}

jint Java_com_sun_cldc_isolate_Isolate_exitCode0() {
  IsolateObj::Raw isolate_obj = GET_PARAMETER_AS_OOP(0);
  return isolate_obj().exit_code();
}

// Wake up all threads that are waiting on the Isolate objects that
// are equivalent to this Isolate object. Note: the current thread
// may not hold the monitor of the equivalent Isolate objects, so
// the waiting threads must have been blocked using Isolate.waitStatus().
//
// private native void notifyStatus();
void Java_com_sun_cldc_isolate_Isolate_notifyStatus(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  IsolateObj::Fast isolate_obj = GET_PARAMETER_AS_OOP(0);
  isolate_obj().notify_all_waiters(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

/*----------------------------------------------------------------
 * Link support
 *----------------------------------------------------------------*/

/* Used to generate unique link id. Cannot be done at Java level because
 * this counter must be system-wide (i.e., must encompass all isolates).
 */
static jint _linkIdGenerator = 0;

jint Java_com_sun_cldc_isolate_Link_nativeCreateLinkId() {
  /* Shouldn't need synchronization due to the way threads are currently
   * scheduled with the VM
   */
  return ++_linkIdGenerator;
}

void Java_com_sun_cldc_isolate_Isolate_setProfile(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  UsingFastOops fast_oops;  
  IsolateObj::Fast isolate = GET_PARAMETER_AS_OOP(0);

  Task::Fast task = isolate().task();
  if (task.not_null() && task().status() == Task::TASK_STARTED) {
    Throw::isolate_state_exception(isolate_already_started JVM_THROW);
  }

  String::Fast string = GET_PARAMETER_AS_OOP(1);
  TypeArray::Raw cstring = string().to_cstring(JVM_SINGLE_ARG_CHECK);  
  const char* profile_name = (const char*) cstring().data();
  const int profile_id = Universe::profile_id_by_name(profile_name);
  isolate().set_profile_id(profile_id);

  if (profile_id == Universe::DEFAULT_PROFILE_ID) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException() 
      JVM_THROW);
  }
#endif
}

// private native static attachDebugger();
void Java_com_sun_cldc_isolate_Isolate_attachDebugger0(JVM_SINGLE_ARG_TRAPS){
#if ENABLE_JAVA_DEBUGGER
  IsolateObj::Raw isolate = GET_PARAMETER_AS_OOP(0);
  Task::Raw task = isolate().task();
  const int task_id = task().task_id();
  if (task_id != -1) {
    TaskGCContext tmp(task_id);
    JavaDebugger::initialize_java_debugger_task(JVM_SINGLE_ARG_CHECK);
  }
#endif
}

jboolean 
Java_com_sun_cldc_isolate_Isolate_isDebuggerConnected(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_JAVA_DEBUGGER
  IsolateObj::Raw isolate = GET_PARAMETER_AS_OOP(0);
  Task::Raw task = isolate().task();
  Transport::Raw t = task().transport();
  return t().not_null() && t().connection_confirmed();
#else
  return false;
#endif
}

} // extern "C"


