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

/** \class Scheduler.hpp
 * The VM scheduler.
 * This class implements the VM scheduler.  The scheduler is a 
 * simple priority based scheme.  There are 10 levels of priority
 * in Java threads, 1 to 10.  Normal priority is 5.  There are 10 
 * queues that are set up as an ObjArray, accessed via
 * Universe::scheduler_priority_queues().  Since most Java programs
 * probably don't take advantage of priorities, we try to optimize
 * for the 'Normal' case of priority 5.  Scheduler::_next_runnable_thread is a
 * cache of the next thread that will be run.  In the case of all threads
 * running at normal priority (5) and no Java Debugger code compiled in the
 * call to Scheduler::get_next_runnable_thread() would just return
 * _next_runnable_thread.
 */

// The scheduler handles all thread switching

typedef void (*do_thread_proc)(Thread*, void do_oop(OopDesc**));

class Scheduler : public AllStatic {
private:
#ifndef PRODUCT
  static void trace(const char* msg, Thread* first, Thread* second = NULL);
#else
  static void trace(const char*, Thread*, Thread* second = NULL) {
    (void)second;
  }
#endif

  static ReturnOop find_waiting_thread(Oop* obj);

  static bool is_in_list(Thread* thread, Thread* list);
#if ENABLE_ISOLATES
  static void check_active_queues() PRODUCT_RETURN; // CLEANUP
  static int  live_thread_count_for_task(int /*task_id*/) PRODUCT_RETURN0;
#endif

  static bool is_in_some_active_queue(Thread *);
  static void add_to_list(Thread* thread, Thread* list);
  static void remove_from_list(Thread* thread, Thread* list);

  static void remove_from_active(Thread* thread);

  static ReturnOop check_debug_state(Thread *, bool);

  static void adjust_priority_list(Thread *thread, bool is_add);
  static void set_priority_valid(Thread *thread, int priority) {
#if ENABLE_ISOLATES
    Task::set_priority_queue_valid(thread->task_id(), priority);
#else
    (void)thread;
    _priority_queue_valid = (short)(_priority_queue_valid  | (1<<(priority)));
#endif
  }
  static void clear_priority_valid(Thread *thread, int priority) {
#if ENABLE_ISOLATES
    Task::clear_priority_queue_valid(thread->task_id(), priority);
#else
    (void)thread;
    _priority_queue_valid = (short)(_priority_queue_valid &(~(1<<(priority))));
#endif
  }
  static void set_last_priority_queue(Thread *thread, int priority) {
#if ENABLE_ISOLATES
    Task::set_last_priority_queue(thread->task_id(), priority);
#else
    (void)thread;
    _last_priority_queue = priority;
#endif
  }
#ifndef PRODUCT
  static bool threads_in_queue(int priority, Task *task) {
#if ENABLE_ISOLATES
    return (task->priority_queue_valid() & (1 << priority)) != 0;
#else
    (void)task;
    return (_priority_queue_valid & (1 << priority)) != 0;
#endif
  }
#endif
  const static char lowbits[16];
  static int lowest_priority() {
    int __base_pri = 0;

    if (_priority_queue_valid == 0) {
      return 1;
    }
    // bit 0 not used for priorities
    int __priority = _priority_queue_valid >> 1;

    if ((__priority & 0xf) == 0) {
      // nothing in priorites 1 - 4
      __priority >>= 4;
      __base_pri = 4;
      if ((__priority & 0xf) == 0) {
        // nothing in priorities 5-8, just test priorities 9 and 10 directly
        if (__priority & 0x10) {
          return 9;
        } else {
          return 10;
        }
      }
    }
    return __base_pri + lowbits[__priority & 0xf];
  }
  static bool have_higher_priority_threads(int priority) {
    // this tests with a bit mask of all the bits that are higher
    // than the bit position specified by priority.
    // Remember, priorities are 1 based (1 to 10), bit 0 therefore
    // is unused
    return (_priority_queue_valid &&
            (_priority_queue_valid & (~((1 << (priority + 1)) - 1))));
  }
  static bool no_higher_priority_threads(int priority_queue_valid, int last_priority_queue) {
    // this tests with a bit mask of all the bits that are higher
    // than the bit position specified by _last_priority_queue.
    // So if _last_priority_queue == 5 then this returns 0xFFC0.
    // Testing _priority_queue_valid against this mask will tell you if
    // there are any threads with higher priority than _last_priority_queue
    // ready to run.
    // Remember, priorities are 1 based (1 to 10), bit 0 therefore
    // is unused
    return (priority_queue_valid &&
            !(priority_queue_valid & (~((1 << (last_priority_queue + 1)) - 1))));
  }

  static bool have_lower_priority_threads(int priority_queue_valid, int priority) {
    // this tests with a bit mask of all the bits that are lower
    // than the bit position specified by priority.
    // Remember, priorities are 1 based (1 to 10), bit 0 therefore
    // is unused
    return (priority_queue_valid &&
            (priority_queue_valid & ((1 << (priority)) - 1)));
  }

#define NORMAL_QUEUE_VALUE (1 << ThreadObj::PRIORITY_NORMAL)

  static void add_to_asynchronous(Thread* thread);
  static void remove_from_asynchronous(Thread* thread);

  static ReturnOop add_waiting_thread(Thread* thread, JavaOop *obj);
  static ReturnOop add_sync_thread(Thread* thread, Thread *pending_waiters,
                                   JavaOop *obj);
  static void remove_waiting_thread(Thread* thread);
  static void add_to_sleeping(Thread* thread);
  static void wake_up_timed_out_sleepers(JVM_SINGLE_ARG_TRAPS);
  static void check_blocked_threads(jlong timeout);
  static bool wait_for_event_or_timer(bool sleeper_found,
                                      jlong min_wakeup_time);
  static void notify_wakeup(Thread* thread JVM_TRAPS);

  static unsigned char _thread_execute_counts[MAX_TASKS][ThreadObj::NUM_PRIORITY_SLOTS+1];
#if ENABLE_ISOLATES
#define TASK_PRIORITY_SCALE_MAX 6
  const static unsigned int sched_priority[TASK_PRIORITY_SCALE_MAX][Task::PRIORITY_MAX+1];
  static void add_to_suspend(Thread* thread);
  static void remove_from_suspend(Thread* thread);
  static unsigned int _task_execute_counts[];
  static void reset_task_counts() {
    for (int i = 0; i < Task::PRIORITY_MAX+1; i++) {
      _task_execute_counts[i] = 0;
    }
  }
  static void reset_thread_counts(int task_id) {
    for (int i = 0; i <= ThreadObj::NUM_PRIORITY_SLOTS; i++) {
      _thread_execute_counts[task_id][i] = 0;
    }
  }
#else
  static void reset_thread_counts(int dummy) {
    (void)dummy;
    for (int i = 0; i <= ThreadObj::NUM_PRIORITY_SLOTS; i++) {
      _thread_execute_counts[0][i] = 0;
    }
  }
#endif

  static void master_mode_wait_for_event_or_timer(jlong sleep_time);
  static void slave_mode_wait_for_event_or_timer(jlong sleep_time);

  static void oops_doer(Thread* thread, void do_oop(OopDesc**));

  static OopDesc* _gc_global_head;
  static OopDesc* _gc_current_thread;

  static int      _active_count;
  static int      _async_count;
  static long     _exit_async_pending;
  static short    _priority_queue_valid;
  static jbyte    _last_priority_queue;
  static bool     _slave_mode_yielding;
  static bool     _yield_on_thread_switch;
  static jlong    _slave_mode_timeout;
  static bool     _timer_has_ticked;
  static int      _estimated_event_readiness;

#if ENABLE_PERFORMANCE_COUNTERS
  static jlong    _slave_mode_yield_start_time;
#endif

  static void update_blocked_threads_buffer();
  inline static void switch_thread_slave_mode(Thread *next_thread, 
                                              Thread* thread JVM_TRAPS);
  inline static void switch_thread_master_mode(Thread *next_thread, 
                                               Thread* thread JVM_TRAPS);
 public:
  static void set_timer_tick() {
    _timer_has_ticked = true;
  }
  static bool has_waiters(Oop* obj);
  static void allocate_blocked_threads_buffer(int target_size JVM_TRAPS);
  static void allocate_blocked_threads_buffer(JVM_SINGLE_ARG_TRAPS) {
    allocate_blocked_threads_buffer(_active_count JVM_CHECK);
  }
  static void add_to_active(Thread*);
  static void wait_for_remaining_threads();
  static OopDesc* get_gc_current_thread() {
    return _gc_current_thread;
  }

  // add to global list
  static void add_to_global_list(Thread *);

  // Remove the thread
  static void terminate(Thread* thread JVM_TRAPS);

  // Terminate all threads
  static void terminate_all();

  static bool initialize();
  static void dispose();

  // Returns number of active threads in the VM
  static int active_count() {
    return _active_count;
  }

#if ENABLE_ISOLATES
  // Support for Isolate suspend and resume
  static void suspend_threads(int task_id);
  static void resume_threads(int task_id);

  // Support for Isolate termination
  static void wake_up_async_threads(int task_id);
  static void wake_up_terminated_sleepers(int task_id JVM_TRAPS);
#endif

  static Thread * get_next_runnable_thread(Thread *thread = NULL);
  static void set_next_runnable_thread(Thread *thread) {
    _next_runnable_thread = thread->obj();
  }
  static void clear_next_runnable_thread() {
    _next_runnable_thread = (OopDesc *)0;
  }

  // GC support
  static void threads_do_list(do_thread_proc do_thread,
                              void do_oop(OopDesc**),
                              OopDesc* list_head);

  static void iterate(do_thread_proc do_thread) {
    threads_do_list(do_thread, NULL, Universe::global_threadlist()->obj());
  }

  static void oops_do(void do_oop(OopDesc**));

  static void gc_prologue(void do_oop(OopDesc**));
  static void gc_epilogue(void);

  // Makes current thread wait for condition
  static void wait_for(Thread*, jlong timeout = 0);

  // Forwarded from java.lang.Object

  // Causes current thread to wait until either another thread invokes
  // the notify() or notify_all() for obj, or a specified amount of
  // time has elapsed.  This function should only be called by a
  // thread that is the owner of obj's monitor
  static void wait(JavaOop* obj, jlong millis JVM_TRAPS);

  static void notify(JavaOop* obj, bool all, bool must_be_owner
                     JVM_TRAPS);

  // Wakes up a single thread that is waiting on obj's monitor. If any thread
  // is are waiting on obj, one of them is chosen to be awakened.
  // This function should only be called by a thread that is the owner
  // of obj's monitor
  static void notify(JavaOop* obj, bool all JVM_TRAPS) {
    notify(obj, all, /*must_be_owner=*/true JVM_CHECK);
  }

  // Forwarded from java.lang.Thread
  static void yield();
  static void sleep_current_thread(jlong millis);
  static void start(Thread* thread JVM_TRAPS);
  static void set_priority(Thread* thread, int old_priority);
#if ENABLE_CLDC_11
  static void interrupt_thread(Thread* thread JVM_TRAPS);
  static void handle_pending_interrupt(JVM_SINGLE_ARG_TRAPS);
#endif

  static void block_current_thread();
  static void unblock_thread(Thread *thread);
  static void *allocate_blocked_thread_data(int data_size JVM_TRAPS);
  static void *get_blocked_thread_data(int *data_size);
  static JVMSPI_BlockedThreadInfo *get_blocked_threads(int *blocked_threads_count);

  static void sleep_forever();

  static jlong time_slice(JVM_SINGLE_ARG_TRAPS);

  static bool is_slave_mode() {
    return SlaveMode;
  }

#ifndef PRODUCT
  static void print();
  static void check_deadlocks();
#endif
  friend void switch_thread(Thread* thread);
  friend class ThreadReferenceImpl;
  friend class Synchronizer;
};
