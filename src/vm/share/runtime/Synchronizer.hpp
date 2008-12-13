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

/** \class StackLock
    The monitor structure used for object locking in the VM.
    Stack locks reside on the execution stack and can be in three different states.
    -# Cleared, not used current
      - { owner == null }
      - monitor exit clears the associated stack lock
      - during wait the associated stack lock is cleared
       until the thread has been notified or timed out.
    -# Reentrant locked
      - { owner != null && real_java_near == null }
      - used if the owner has been locked more than once by this thread.
    -# Locked
      - { owner != null && real_java_near != null }
      - used for the non-reentrant lock
*/
class StackLock {
 public:
  // ^JavaNear
  ReturnOop java_near()      { return ReturnOop((JavaNearDesc*)(this+1)); }
  ReturnOop java_near(JavaNear* prototype) {
     JavaNearDesc* stack_near = (JavaNearDesc*)(this + 1);
     stack_near->_klass      = prototype->klass();
     stack_near->_class_info = (ClassInfoDesc*)prototype->class_info();
     stack_near->_value      = prototype->raw_value();
     return ReturnOop(stack_near);
  }

  ReturnOop thread()             { return _thread;         }
  void set_thread(Thread* value) { _thread = value->obj(); }

  ReturnOop real_java_near() { return ReturnOop(_real_java_near); }
  void set_real_java_near(JavaNear* value) {
    _real_java_near = (JavaNearDesc*) value->obj();
  }
  void clear_real_java_near() { _real_java_near = (JavaNearDesc*) 0; }

  ReturnOop waiters()                 { return _waiters; }
  void set_waiters(Thread* value);
  void clear_waiters() {_waiters = NULL;}

  static size_t size() {
    return sizeof(StackLock) + JavaNearDesc::allocation_size();
  }

  // Compute the StackLock* based on a locked java_near
  static StackLock* from_java_oop(JavaOop* java_oop) {
#ifdef AZZERT
    JavaNear::Raw java_near = java_oop->klass();
    GUARANTEE(java_near().is_locked(), "Must be on stack");
#endif
    return ((StackLock*) java_oop->klass()) - 1;
  }

  static ReturnOop waiters_from_java_oop(JavaOop* java_oop) {
    JavaNear::Raw java_near = java_oop->klass();
    if (!java_near().is_locked()) {
      return NULL;
    } else {
      return ((StackLock*)(((StackLock*) java_oop->klass()) - 1))->_waiters;
    }
  }

  // GC support
  void oops_do(void do_oop(OopDesc**));
  void reverse_locked_header();
  void restore_locked_header();
  void relocate_internal_pointers(int);

  void lock(Thread* THREAD, JavaOop *obj);

  // Printing
  void print()                    PRODUCT_RETURN;
  void print_value_on(Stream*)    PRODUCT_RETURN;

 private:
  OopDesc*      _thread;
  JavaNearDesc* _real_java_near;
  OopDesc*      _waiters;        // Condition variable the waiters are waiting for.

 public:
  static int thread_offset() { 
    return FIELD_OFFSET(StackLock, _thread);
  }
  static int real_java_near_offset() {
    return FIELD_OFFSET(StackLock, _real_java_near);
  }
  static int waiters_offset() {
    return FIELD_OFFSET(StackLock, _waiters);
  }
  static int copied_near_offset() {
    return sizeof(StackLock);
  }

  OopDesc** owner_address()    { return (OopDesc**) (((char*) this) + size()); }
  ReturnOop owner()            { return *owner_address(); }
  void set_owner(JavaOop* value) { *owner_address() = value->obj(); }
  void clear_owner() { *owner_address() = (OopDesc*)0; }

  friend class ObjectHeap;
};

/** \class Synchronizer
  Used to synchronize threads on objects.
  Calls into the Scheduler to suspend threads that will block on an object.

*/
class Synchronizer : public AllStatic {
 public:
  enum {
    LOCK_OBJ_STRING_INDEX = 0,
    LOCK_OBJ_INFO_INDEX,
    LOCK_OBJ_NUM_ENTRIES
  };
  enum {
    INFO_REF_INDEX = 0,
#if ENABLE_ISOLATES
    INFO_TASKID_INDEX,
#endif
    INFO_NUM_ENTRIES
  };

  // Hash code
  static jint hash_code(JavaOop* obj JVM_TRAPS);

  // Locks obj
  static bool enter(Thread *thread, StackLock* stack_lock JVM_TRAPS);
  // Unlocks object
  static void exit(StackLock* stack_lock);
  // Unlock a (possibly recursive) stack lock
  static void unlock_stack_lock(StackLock* stack_lock);
  // Allows other waiting threads to run
  static void signal_waiters(StackLock* stack_lock);
    

  // Tells whether 'obj' is locked by the thread
  static bool is_locked_by(JavaOop* obj, Thread* THREAD);
  static ReturnOop get_lock_object_ref(JavaOop *obj, Thread *thread,
                                       bool lookup_only JVM_TRAPS);

  static void release_lock_object_ref(JavaOop *obj);

#if ENABLE_ISOLATES
  static void release_locks_for_task(const int task_id);
#endif

private:
  static void do_lock_object(Thread *THREAD);
};
