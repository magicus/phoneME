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

# include "incls/_precompiled.incl"
# include "incls/_IsolateObj.cpp.incl"

bool IsolateObj::represents(const Task *t) const {
  IsolateObj::Raw primary = t->primary_isolate_obj();
  return primary().is_equivalent_to(this);
}

jint IsolateObj::status( void ) const {
  Task::Raw t = task();
  if (t.is_null()) {
    if (is_terminated()) {
      return Task::TASK_STOPPED;
    } else {
      return Task::TASK_NEW;
    }
  } else {
    return t().status();
  }
}

ReturnOop IsolateObj::task( void ) const {
  TaskList::Raw tlist = Universe::task_list();
  const int len = tlist().length();

  for (int id = Task::FIRST_TASK; id < len; id++) {
    Task::Raw t( tlist().obj_at(id) );
    if (!t.is_null() && this->represents(&t)) {
      return t.obj();
    }
  }

  return NULL;
}

ReturnOop IsolateObj::duplicate(JVM_SINGLE_ARG_TRAPS) const {
  UsingFastOops fast_oops;
  IsolateObj::Fast dup = 
      Universe::new_instance(Universe::isolate_class() JVM_CHECK_0);

  dup().set_unique_id( unique_id() );
  dup().set_use_verifier( use_verifier() );
  dup().set_api_access_init( api_access_init() );
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  dup().set_profile_id( profile_id() );
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

  OopDesc* p;
  // Do not use JVM_ZCHECK because result can be NULL
  p = Universe::deep_copy(main_class() JVM_CHECK_0);
  dup().set_main_class(p);
  p = Universe::deep_copy(main_args() JVM_CHECK_0);
  dup().set_main_args(p);
  p = Universe::deep_copy(app_classpath() JVM_CHECK_0);
  dup().set_app_classpath(p);

  return dup.obj();
}

void IsolateObj::terminate(int exit_code JVM_TRAPS) {
  set_is_terminated(1);
  set_saved_exit_code(exit_code);
  Scheduler::notify(this, /*all=*/true, /*must_be_owner=*/false
                    JVM_NO_CHECK_AT_BOTTOM);
}

void IsolateObj::notify_all_waiters(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  TaskList::Fast tlist = Universe::task_list();
  Task::Fast t;
  IsolateObj::Fast iso;
  const int len = tlist().length();

  // Task 0 reserved for system use
  for (int id = Task::FIRST_TASK; id < len; id++) {
    t = tlist().obj_at(id);
    if (t.is_null()) {
      continue;
    }
    for (iso = t().seen_isolates(); iso.not_null(); iso = iso().next()) {
      if (iso().is_equivalent_to(this)) {
        // <iso> repesents the same task as myself. Notify
        // all threads waiting for it.
        Scheduler::notify(&iso, /*all=*/true, /*must_be_owner=*/false
                          JVM_CHECK);
        // IMPL_NOTE: if we really run out of memory here some tasks may be
        // in limbo. Also, what happens when the current task has run out
        // of quota but the system still has plenty heap?
      }
    }
  }
}

// [1] Store the exit code in all equivalent Isolate objects, so that
//     the Isolate.exitCode() method would contine to work after the
//     Task associated with these Isolate objects have vanished.
// [2] Remove all equivalent Isolate objects Task from the
//     seen_isolates lists of all active Tasks.
// [3] Notify all waiters on all equivalent Isolate objects (to complete
//     Isolate.start() or Isolate.waitForExit() calls.
void IsolateObj::mark_equivalent_isolates_as_terminated(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fastoops;
  TaskList::Fast tlist = Universe::task_list();
  Task::Fast terminating_task = task();
  const int len = tlist().length();

  // Task 0 reserved for system use
  for (int id = Task::FIRST_TASK; id < len; id++) {
    UsingFastOops fastoops2;
    Task::Fast t (tlist().obj_at(id));
    if (t.is_null()) {
      continue;
    }

    IsolateObj::Fast iso, last;
    for (iso = t().seen_isolates(); iso.not_null(); iso = iso().next()) {
      if (iso().is_equivalent_to(this)) {
        iso().set_priority(terminating_task().priority());
        iso().terminate(terminating_task().exit_code() JVM_CHECK);
        IsolateObj::Raw next_iso = iso().next();
        if (iso.obj() == t().seen_isolates()) {
          t().set_seen_isolates(&next_iso);
        } else {
          last().set_next(&next_iso);
        }
        break;
      }
      last = iso.obj();
    }
  }
}

jint IsolateObj::task_id( void ) const {
  Task::Raw t = task();
  return t.not_null() ? t().task_id() : Task::INVALID_TASK_ID;
}

jint IsolateObj::exit_code( void ) const {
  // Always fetch the exit code from the Task, if the Task still exists
  // (even if it is in the STOPPING state).
  Task::Raw t = task();
  return t.not_null() ? t().exit_code() : saved_exit_code();
}

jint IsolateObj::static_int_field(int offset) const {
  Task::Raw this_task = task();
  GUARANTEE(!this_task.is_null(), "Task is null");
  ObjArray::Raw mirror_list = this_task().mirror_list();
  GUARANTEE(!mirror_list.is_null(), "Null Mirror list");
  TaskMirror::Raw tm =
    mirror_list().obj_at(Universe::isolate_class()->class_id());
  GUARANTEE(!tm.is_null(), "null task mirror");
  return tm().int_field(offset);
}

void IsolateObj::static_int_field_put(int offset, int value) {
  Task::Raw this_task = task();
  GUARANTEE(!this_task.is_null(), "Task is null");
  ObjArray::Raw mirror_list = this_task().mirror_list();
  GUARANTEE(!mirror_list.is_null(), "Null Mirror list");
  TaskMirror::Raw tm =
    mirror_list().obj_at(Universe::isolate_class()->class_id());
  GUARANTEE(!tm.is_null(), "null task mirror");
  tm().int_field_put(offset, value);
}


#ifndef PRODUCT

void IsolateObj::verify_fields() {
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = Universe::isolate_class();
  ic().verify_instance_field("_next",     "Lcom/sun/cldc/isolate/Isolate;",
                                          next_offset()); 
  ic().verify_instance_field("_uniqueId", "J",
                                          unique_id_offset()); 
  ic().verify_instance_field("_terminated","I",
                                          is_terminated_offset());
  ic().verify_instance_field("_priority","I",
                                          priority_offset());
  ic().verify_instance_field("_saved_exit_code","I",
                                          saved_exit_code_offset());
  ic().verify_instance_field("_mainClass",  "Ljava/lang/String;", 
                                          main_class_offset()); 
  ic().verify_instance_field("_mainArgs",  "[Ljava/lang/String;", 
                                          main_args_offset()); 
  ic().verify_instance_field("_app_classpath",  "[Ljava/lang/String;", 
                                          app_classpath_offset()); 
  ic().verify_instance_field("_sys_classpath",  "[Ljava/lang/String;", 
                                          sys_classpath_offset());
  ic().verify_instance_field("_hidden_packages",  "[Ljava/lang/String;", 
                                          hidden_packages_offset()); 
  ic().verify_instance_field("_restricted_packages",  "[Ljava/lang/String;", 
                                          restricted_packages_offset()); 
  ic().verify_instance_field("_memoryReserve",  "I", 
                                          memory_reserve_offset()); 
  ic().verify_instance_field("_memoryLimit",  "I",
                                          memory_limit_offset()); 
  ic().verify_instance_field("_profileId", "I",
                                          profile_id_offset()); 
}

#endif
