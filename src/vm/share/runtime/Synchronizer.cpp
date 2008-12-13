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
# include "incls/_Synchronizer.cpp.incl"

void StackLock::set_waiters(Thread* thread) {
  GUARANTEE(!thread->is_null(), "Sanity");
#ifdef AZZERT
  StackLock* stack_lock = thread->wait_stack_lock();
  GUARANTEE(stack_lock != NULL, "null stack lock");
#endif
  // Don't use JavaNear since this is a stack lock
  JavaNear::Raw n = java_near();
  n().set_lock(JavaNear::waiters_value);
  _waiters = thread->obj();
}

void StackLock::lock(Thread* thread, JavaOop *obj) {
  AllocationDisabler no_allocation;
  // May not necessarily be the current thread
  GUARANTEE(owner() == NULL || owner() == obj->obj(), "Sanity Check");
  GUARANTEE(thread->is_on_stack((address)this), "Sanity check");

  JavaNear::Raw real_near = obj->klass();
  GUARANTEE(!real_near().is_locked(), "Sanity check");

  // Must not use JavaNear for stack nears.
  JavaNear::Raw stack_near = java_near(&real_near);
  stack_near().set_lock(JavaNear::locked_value);

  // Object is unlocked, so we're free to lock it.
  set_owner(obj);
  set_thread(thread);
  set_real_java_near(&real_near);
  _waiters = NULL;
  obj->set_klass(&stack_near);
  GUARANTEE(Synchronizer::is_locked_by(obj, thread), "Sanity check");
}

static int local_rand() {
  static unsigned int state = 0x23451921;
  const unsigned int multiplier = 0xDEECE66DL;
  const unsigned int addend = 0xBL;
  state = state * multiplier + addend;
  return (int)state;
}

jint Synchronizer::hash_code(JavaOop* obj JVM_TRAPS) {

  if (obj->klass() == _interned_string_near_addr) {
    JavaOop::Raw new_obj = get_lock_object_ref(obj, Thread::current(), false
                                               JVM_ZCHECK_0(new_obj));
    GUARANTEE(!obj->is_null(), "lock object is null!");
    *obj = new_obj;
  }
  JavaNear::Raw java_near = obj->klass();

  // Check if hash has been computed
  if (!java_near().has_hash()) { 
    UsingFastOops fast_oops;
    bool is_locked = java_near().is_locked();
    JavaClass::Fast c = java_near().klass();
    // Create a new near.
    java_near = Universe::new_java_near(&c JVM_CHECK_0);

    // use a pseudo-random 32-bit number as the hash value
    jint new_hash;
    do { 
      new_hash = local_rand() & JavaNear::hash_mask;
    } while (new_hash == JavaNear::no_hash_value);
    java_near().set_hash(new_hash);

    if (!is_locked) {
      // Simple case.  Just change object to point to new near.
      obj->set_klass(&java_near);
    } else {
      // Object is pointing at a stack near.   We must change the stack lock
      // to point to the new near, and we must also change the hash code in 
      // the stack near.
      StackLock::from_java_oop(obj)->set_real_java_near(&java_near);
      // Get back the stack near that the object is pointing at.
      java_near = obj->klass();
      java_near().set_hash(new_hash);
    }
  }
  return java_near().hash();
}

bool Synchronizer::enter(Thread* thread, StackLock* stack_lock JVM_TRAPS) {
  UsingFastOops fast_oops;
  // This might not be the current thread.
  NOT_PRODUCT(Scheduler::trace("Enter lock", thread));
    
  GUARANTEE(Scheduler::is_in_some_active_queue(thread),
            "must be active thread");

  JavaOop::Fast    obj = stack_lock->owner();
  GUARANTEE(obj.not_null(), "Attempting to unlock NULL object");

  // Synchronizing on an interned string object is handled by
  // allocating a 'proxy' object that is actually stored in the
  // stack lock
  if (obj.klass() == _interned_string_near_addr) {
    // store stack lock temporarily so that we can survive a GC
    thread->set_wait_stack_lock(stack_lock);
    obj = get_lock_object_ref(&obj, thread, false JVM_CHECK_0);
    GUARANTEE(!obj.is_null(), "lock object is null!");
    stack_lock = thread->wait_stack_lock();
    thread->clear_wait_stack_lock();
    stack_lock->set_owner(&obj);
    // handle recursive case here.
    JavaNear::Raw java_near = obj.klass();
    if (java_near().is_locked()) {
      if (StackLock::from_java_oop(&obj)->thread() == thread->obj()) {
        // recursive lock
        stack_lock->clear_real_java_near();
        return true;
      }
    }
  }
  JavaNear::Raw java_near = obj.klass();
  if (TraceThreadsExcessive) {
    TTY_TRACE_CR(("enter: thread: 0x%x, obj: 0x%x, locked %d",
                  thread->obj(), obj.obj(), java_near().is_locked()));
  }
  if (!java_near().is_locked()) {
     // Grab the lock
     stack_lock->lock(thread, &obj);
     return true;
  } else { 
    UsingFastOops rare_inside_case;
    if (TraceThreadsExcessive) {
      TTY_TRACE_CR(("locked by: 0x%x",
                    StackLock::from_java_oop(&obj)->thread()));
    }
    // Add THREAD to the list of waiters. 
    GUARANTEE(StackLock::from_java_oop(&obj)->thread() != 
              thread->obj(), "Reentrancy stack lock not allowed");
    thread->set_wait_stack_lock(stack_lock);
    stack_lock->clear_owner();
    AZZERT_ONLY(stack_lock = (StackLock*)-1); // Not GC Safe
 
    Thread::Fast pending_waiters = StackLock::from_java_oop(&obj)->waiters(); 

    // Add this thread to the list of waiters
    // Note that this thread is >>not<< put onto the
    // Universe::scheduler_waiting() list.  This is for
    // synchronizing rather than for waiting.
    pending_waiters = Scheduler::add_sync_thread(thread, &pending_waiters,
                                                 &obj);
    StackLock::from_java_oop(&obj)->set_waiters(&pending_waiters);
    // Wait until object has been unlocked, then lock it.
    if (_debugger_active) {
      thread->set_status((thread->status() & ~THREAD_NOT_ACTIVE_MASK) |THREAD_MONITOR_WAIT);
    }
    Scheduler::wait_for(thread);
    return false;
  }
}

void Synchronizer::exit(StackLock* stack_lock) {
  // The stack_lock does not necessarily belong to the current thread
  JavaOop::Raw obj = stack_lock->owner();

  // Unlock the object by restoring the original java near
  Oop::Raw real_near = stack_lock->real_java_near();
  obj.set_klass(&real_near);
  stack_lock->clear_owner();
  // See if someone else wants to run
  signal_waiters(stack_lock);
}

void Synchronizer::unlock_stack_lock(StackLock* stack_lock) {
  GUARANTEE(stack_lock->owner() != NULL, "Must be free lock");
  JavaOop::Raw owner = stack_lock->owner();
  JavaNear::Raw real_near = stack_lock->real_java_near();
  stack_lock->clear_owner();
  if (!real_near.is_null()) {
    owner().set_klass(&real_near);
    signal_waiters(stack_lock);
  }
}

void Synchronizer::signal_waiters(StackLock* stack_lock) {
  // This might not a lock on the current thread.
  GUARANTEE(stack_lock->owner() == NULL, "Must be free lock");
  GUARANTEE(stack_lock->real_java_near() != NULL, "Must still be a lock");
  Thread::Raw waiters = stack_lock->waiters();
  // We can now clear the lock
  stack_lock->clear_real_java_near();      
  if (waiters.not_null()) {
    // Wake up the first thread on the queue
    Thread::Raw first_waiter = waiters.obj();
    Thread::Raw next_waiter = first_waiter().next_waiting();
    if (TraceThreadsExcessive) {
      TTY_TRACE_CR(("signal: 0x%x (id=%d), curr: 0x%x (id=%d), obj: 0x%x, next: 0x%x, lock: 0x%x",
                    first_waiter().obj(), first_waiter().id(),
                    Thread::current()->obj(),
                    Thread::current()->id(),
                    first_waiter().wait_obj(),
                    next_waiter().obj(),
                    (int)stack_lock));
    }
    Scheduler::add_to_active(&first_waiter);
    JavaOop::Raw obj = first_waiter().wait_obj();
    first_waiter().wait_stack_lock()->lock(&first_waiter, &obj);
    if (next_waiter.not_null()) {
      // If there are more waiters, transfer this information to
      // the new lock
      if (TraceThreadsExcessive) {
        TTY_TRACE_CR(("signal: obj 0x%x, next waiter: 0x%x (id=%d)",
                    first_waiter().wait_obj(),
                      next_waiter().obj(), next_waiter().id()));
      }
      first_waiter().wait_stack_lock()->set_waiters(&next_waiter);
    }
    first_waiter().clear_wait_stack_lock();
    first_waiter().clear_next_waiting();
    first_waiter().clear_wait_obj();
  }
}

void StackLock::oops_do(void do_oop(OopDesc**)) {
  if (_real_java_near != NULL) {
    do_oop((OopDesc**) &_thread);
    do_oop((OopDesc**) &_waiters);
    // Visit real java near saved in StackLock
    do_oop((OopDesc**) &_real_java_near);
    // And the embedded oop
    // Can the following be done more cleanly?
    OopDesc** embedded_near = (OopDesc**)(this + 1);
    do_oop(embedded_near);
    do_oop((OopDesc**)(((address)embedded_near) 
                            + JavaNear::class_info_offset()));
  }
}

void StackLock::reverse_locked_header() {
  if (_real_java_near != NULL) {
    JavaNearDesc* stack_near = (JavaNearDesc*) (this+1);
    // Pointer to locked object is just below stack near.
    OopDesc* obj = *(OopDesc**)(stack_near + 1);
    if (obj != NULL) {
      GUARANTEE(stack_near == obj->klass(), "check loop");
      // Locked object, set object near pointer back to the real java near.
      obj->_klass = _real_java_near;
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: 0x%p restore header from stack lock 0x%p",
                      obj, stack_near));
      }
    } else {
      // Unlocked stack lock - do nothing.
    }
  }
}

void StackLock::restore_locked_header() {
  if (_real_java_near != NULL) {
    // During GC we changed all near pointers in locked object to point
    // to their real java near instead of the stack near.
    // Set near pointer in object back to point to stack near.
    JavaNearDesc* stack_near = (JavaNearDesc*) (this+1);
    // Pointer to locked object is just below stack near
    OopDesc* obj = *(OopDesc**)(stack_near + 1);
    if (obj != NULL) {
      GUARANTEE(ObjectHeap::contains(obj) || ROM::system_contains(obj),
                "Must be valid near");
      GUARANTEE(ObjectHeap::contains(obj->klass()) || 
                ROM::system_contains(obj->klass()), "Must be be valid near");
      obj->_klass = stack_near;
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: 0x%p reset locked header to stack lock 0x%p",
                      obj, stack_near));
      }
    } else {
      // Unlocked stack lock - do nothing.
    }
  }
}

void StackLock::relocate_internal_pointers(int delta) {
  if (_real_java_near != NULL) {
    JavaNearDesc* stack_near = (JavaNearDesc*) (this+1);
    // Pointer to locked object is just below stack near.
    OopDesc* obj = *(OopDesc**)(stack_near + 1);
    if (obj != NULL) {
      GUARANTEE(stack_near == obj->klass(), "check loop");
      // Locked object, relocate pointer to stack near
      obj->_klass = (OopDesc *)((int)obj->_klass + delta);
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: 0x%p relocate object near from "
                      "stack lock 0x%p", obj, stack_near));
      }
    } else {
      // Unlocked stack lock - do nothing.
    }
  }
}
    
bool Synchronizer::is_locked_by(JavaOop* obj, Thread* thread) {
  JavaNear::Raw java_near = obj->klass();
  return java_near().is_locked() && 
          StackLock::from_java_oop(obj)->thread() == thread->obj();
}

// Interned strings are not used for synchronization.  We allocate
// a 'proxy' object which is used to sync with.  This way we can ROMize
// strings into the TEXT section and eliminate their near pointer.  
// In the MVM case we need this so that sync objects are task local.
// If lookup_only flag set, don't increment reference count
// This could be done much faster with a hash on the string but this
// should be called so infrequently that it's better to keep it simple
ReturnOop Synchronizer::get_lock_object_ref(JavaOop *obj, Thread *thread,
                                            bool lookup_only JVM_TRAPS) {

  UsingFastOops fast_oops;
  GUARANTEE(obj->is_string(), "Lock object must be a string");
  GUARANTEE(obj->klass() == _interned_string_near_addr,
            "Interned string with wrong near pointer");
  String::Fast string(obj->obj());
  ObjArray::Fast lock_table = Universe::lock_obj_table();
  int len = lock_table().length();
  ObjArray::Fast lock_obj;
  TypeArray::Fast info;
  int first_null_index = -1;
  int index;
  for (index = 0; index < len; index++) {
    lock_obj = lock_table().obj_at(index);
    if (lock_obj.not_null()) {
      String::Raw lock_string = lock_obj().obj_at(LOCK_OBJ_STRING_INDEX);
      if (lock_string.equals(string)) {
        info = lock_obj().obj_at(LOCK_OBJ_INFO_INDEX);
#if ENABLE_ISOLATES
        if (info().int_at(INFO_TASKID_INDEX) == thread->task_id()) {
          if (!lookup_only) {
            info().int_at_put(INFO_REF_INDEX, info().int_at(INFO_REF_INDEX)+1);
          }
          return lock_obj;
        }
#else
        if (!lookup_only) {
          info().int_at_put(INFO_REF_INDEX, info().int_at(INFO_REF_INDEX)+1);
        }
        return lock_obj;
#endif
      }
    } else {
      if (first_null_index == -1) {
        first_null_index = index;
      }
    }
  }
    
  if (lookup_only) {
    return NULL;
  }
  // allocate new entry
  lock_obj = Universe::new_obj_array(LOCK_OBJ_NUM_ENTRIES JVM_CHECK_0);
  info = Universe::new_int_array(INFO_NUM_ENTRIES JVM_CHECK_0);
  lock_obj().obj_at_put(LOCK_OBJ_STRING_INDEX, obj);
  info().int_at_put(INFO_REF_INDEX, 1);
#if ENABLE_ISOLATES
  info().int_at_put(INFO_TASKID_INDEX, thread->task_id());
#else
  (void)thread;
#endif
  lock_obj().obj_at_put(LOCK_OBJ_INFO_INDEX, &info);
  if (first_null_index != -1) {
    lock_table().obj_at_put(first_null_index, &lock_obj);
  } else {
    UsingFastOops fast_oops;
    ObjArray::Fast old_lock_obj;
    juint new_table_size = len + 8;
    if (Verbose) {
      TTY_TRACE_CR(("Expanding lock_table to %d entries", new_table_size));
    }
    ObjArray::Fast new_lock_table = Universe::new_obj_array(new_table_size
                                                            JVM_CHECK_0);
    for (int i = 0; i < lock_table().length(); i++) {
      old_lock_obj = lock_table().obj_at(i);
      new_lock_table().obj_at_put(i, &old_lock_obj);
    }
    *Universe::lock_obj_table() = new_lock_table.obj();
    new_lock_table().obj_at_put(len, &lock_obj);
  }
  return lock_obj;
}

void Synchronizer::release_lock_object_ref(JavaOop *obj) {
  ObjArray::Raw lock_obj(obj->obj());
  TypeArray::Raw info = lock_obj().obj_at(LOCK_OBJ_INFO_INDEX);
  ObjArray::Raw lock_table;
  if (info().int_at(INFO_REF_INDEX) == 1) {
    lock_table = Universe::lock_obj_table();
    int len = lock_table().length();
    for (int i=0; i < len; i++) {
      lock_obj = lock_table().obj_at(i);
      if (lock_obj.equals(obj)) {
        lock_table().obj_at_clear(i);
        return;
      }
    }
  } else {
    info().int_at_put(INFO_REF_INDEX, info().int_at(INFO_REF_INDEX) - 1);
  }
  return;
}

#if ENABLE_ISOLATES
// Clear all lock objects that match task_id.
void Synchronizer::release_locks_for_task(const int task_id) {
  ObjArray::Raw lock_table = Universe::lock_obj_table();
  const int len = lock_table().length();

  for( int index = 0; index < len; index++ ) {
    ObjArray::Raw lock_obj = lock_table().obj_at(index);
    if (lock_obj.not_null()) {
      TypeArray::Raw info = lock_obj().obj_at(LOCK_OBJ_INFO_INDEX);
      if (info().int_at(INFO_TASKID_INDEX) == task_id) {
        lock_table().obj_at_clear(index);
      }
    }
  }
}

#endif

#ifndef PRODUCT

void StackLock::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  JavaOop  obj  = owner();

  // Unused
  if (obj.is_null()) {
    st->print("(unused)");
    return;
  }

  // Reentrant lock
  JavaNear real = real_java_near();
  if (real.is_null()) {
    st->print("Reentrant lock for ");
    obj.print_value_on(st);
    return;
  }

  // Lock
  st->print("Lock for ");
  obj.print_value_on(st);
  JavaNear stack_near = java_near();
  if (!stack_near.has_waiters()) return;

  st->print(", waiters ");
  Oop w = waiters();
  w.print_value_on(st);
#endif
}

void StackLock::print() {
#if USE_DEBUG_PRINTING
  print_value_on(tty);
  tty->cr();
#endif
}

#endif
