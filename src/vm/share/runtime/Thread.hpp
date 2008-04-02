/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

/** \class Thread.hpp
 * This class contains the VM specific information about Java threads.
 *
 * Thread.hpp is used for VM threading control, scheduling, waits and
 * notifies.  This class also has support for stack overflow
 * handling, ExecutionStack growing/shrinking.
 *
 * Note that this class is distinct from the Java class java.lang.Thread,
 * which is represented in C++ code by the C++ class ThreadObj.
 */

// Forward declarations
extern int _system_address;

#if ARM && ENABLE_XSCALE_WMMX_TIMER_TICK
  // To cache _current_stack_limit
  extern "C" {
    void wmmx_set_timer_tick();
    void wmmx_clear_timer_tick();
  }
#endif

class Thread: public Oop {
 public:
  HANDLE_DEFINITION_CHECK(Thread, Oop);

  // Accessors for current (static that points to the running thread)
  static Thread* current() {
    return (Thread*)&_current_thread;
  }

  // To avoid endless lists of friends the static offset computation 
  // routines are all public.
  static jint last_java_sp_offset() {
    return FIELD_OFFSET(ThreadDesc, _last_java_sp_or_frame);
  }
  static jint last_java_fp_offset() {
    return FIELD_OFFSET(ThreadDesc, _last_java_fp);
  }
  static jint next_offset() {
    return FIELD_OFFSET(ThreadDesc, _next);
  }
  static jint previous_offset() {
    return FIELD_OFFSET(ThreadDesc, _previous);
  }
  static jint global_next_offset() {
    return FIELD_OFFSET(ThreadDesc, _global_next);
  }
  static jint next_waiting_offset() {
    return FIELD_OFFSET(ThreadDesc, _next_waiting);
  }
  static jint wait_obj_offset() {
    return FIELD_OFFSET(ThreadDesc, _wait_obj);
  }
  static jint wait_stack_lock_offset() {
    return FIELD_OFFSET(ThreadDesc, _wait_stack_lock);
  }
  static jint wakeup_time_offset() {
    return FIELD_OFFSET(ThreadDesc, _wakeup_time);
  }
  static jint pending_exception_offset() {
    return FIELD_OFFSET(ThreadDesc, _pending_exception);
  }
  static jint thread_obj_offset() {
    return FIELD_OFFSET(ThreadDesc, _thread_obj);
  }
  static jint stack_limit_offset() {
    return FIELD_OFFSET(ThreadDesc, _stack_limit);
  }
  static jint pending_entries_offset() {
    return FIELD_OFFSET(ThreadDesc, _pending_entries);
  }
  static jint execution_stack_offset() {
    return FIELD_OFFSET(ThreadDesc, _execution_stack);
  }
  static jint stack_pointer_offset() {
    return FIELD_OFFSET(ThreadDesc, _stack_pointer);
  }
  static jint status_offset() {
    return FIELD_OFFSET(ThreadDesc, _status);
  }
  static jint suspend_count_offset() {
    return FIELD_OFFSET(ThreadDesc, _suspend_count);
  }
  static jint step_info_offset() {
    return FIELD_OFFSET(ThreadDesc, _step_info);
  }
  static jint obj_value_offset() {
    return FIELD_OFFSET(ThreadDesc, _obj_value);
  }
  static jint int1_value_offset() {
    return FIELD_OFFSET(ThreadDesc, _int1_value);
  }
  static jint int2_value_offset() {
    return FIELD_OFFSET(ThreadDesc, _int2_value);
  }
  static jint async_info_offset() {
    return FIELD_OFFSET(ThreadDesc, _async_info);
  }
  static jint async_redo_offset() {
    return FIELD_OFFSET(ThreadDesc, _async_redo);
  }
  static jint task_id_offset() {
    return FIELD_OFFSET(ThreadDesc, _task_id);
  }
  static jint cached_async_info_offset() {
    return FIELD_OFFSET(ThreadDesc, _cached_async_info);
  }
  static jint id_offset() {
    return FIELD_OFFSET(ThreadDesc, _id);
  }
  static jint profiler_info_offset() {
    return FIELD_OFFSET(ThreadDesc, _profiler_info);
  }

  static const address lowest_stack_value;

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);
  void initialize_main(JVM_SINGLE_ARG_TRAPS);

  void start(JVM_SINGLE_ARG_TRAPS);
  static void finish();

  // Dispose the thread
  static void dispose();

  void must_be_current_thread() const {
    GUARANTEE(this->equals(Thread::current()), "Must be current thread");
  }

  void must_not_be_current_thread() const {
    GUARANTEE(!this->equals(Thread::current()), "Must not be current thread");
  }

  // The pending exception for the current thread is stored in the global
  // variable _current_pending_exception.  
  // These methods are defined so that we use a different method, depending
  // on whether we are looking at the current thread or a different thread.

  ReturnOop noncurrent_pending_exception() const { 
    must_not_be_current_thread();
    return obj_field(pending_exception_offset());     
  }
  void set_noncurrent_pending_exception(Oop* value) { 
    must_not_be_current_thread();
    obj_field_put(pending_exception_offset(), value); 
  }
  void clear_noncurrent_pending_exception() { 
    must_not_be_current_thread();
    obj_field_clear(pending_exception_offset()); 
  }

  static ReturnOop current_pending_exception()  { 
    return _current_pending_exception;
  }
  static void set_current_pending_exception(Oop* value) { 
    _current_pending_exception = value->obj();
  }
  static void set_current_pending_exception(OopDesc* value) { 
    _current_pending_exception = value;
  }
  static void clear_current_pending_exception() { 
    _current_pending_exception = NULL;
  }
  static bool current_has_pending_exception() { 
    return _current_pending_exception != NULL; 
  }

  // Store _current_pending_exception into the current thread.
  static void decache_current_pending_exception() {
    current()->obj_field_put(pending_exception_offset(), 
                             (Oop*)&_current_pending_exception);
    _current_pending_exception = NULL;
  }
    
  // Cache the value of the current thread's pending exception into the
  // global variable _current_pending_exception;
  static void cache_current_pending_exception() {
    _current_pending_exception = 
        current()->obj_field(pending_exception_offset());
    current()->obj_field_clear(pending_exception_offset());
  }

#if ENABLE_ISOLATES
  void enter_isolate_context(Oop *new_isolate_context JVM_TRAPS);
  void exit_isolate_context(JVM_SINGLE_ARG_TRAPS);
  static void terminate_task(Thread *);
#endif

  // Accessors for delayed execution ^EntryActivation
  ReturnOop pending_entries() const {
    return obj_field(pending_entries_offset());
  }
  void set_pending_entries(Oop* value) {
    obj_field_put(pending_entries_offset(), value);
  }

  // Accessors for execution stack ^ExecutionStack
  ReturnOop execution_stack() const {
    return obj_field(execution_stack_offset());
  }
  void set_execution_stack(Oop* value) {
    obj_field_put(execution_stack_offset(), value);
  }
  void clear_execution_stack() {
    obj_field_clear(execution_stack_offset());
  }

  void append_pending_entry(EntryActivation* entry);

  bool has_pending_entries()   const {
    return (obj_field(pending_entries_offset()) != NULL);
  }

  // checks if thread has user frames in it, until
  // first EntryFrame or num_frames, whatever comes first 
  bool has_user_frames_until(int num_frames);

  // Accessors for thread object (Java level thread object).
  ReturnOop thread_obj() const {
    return obj_field(thread_obj_offset());
  }
  void set_thread_obj(ThreadObj* value); 
  void clear_thread_obj() {
    obj_field_clear(thread_obj_offset());
  }

  // Accessors for isolate identifier
  int task_id( void ) const {
    return int_field(task_id_offset());
  }
  void set_task_id(int value) {
    int_field_put(task_id_offset(), value);
  }
  ReturnOop task_for_thread( void ) const;

  int id() const {
    return int_field(id_offset());
  }
  void set_id(int value) {
    int_field_put(id_offset(), value);
  }

  // Accessors for async_info
  ReturnOop  async_info() const {
    return obj_field(async_info_offset()); 
  }

  void set_async_info(TypeArray* value) {
    obj_field_put(async_info_offset(), (Oop*)value); 
  }

  ReturnOop  cached_async_info() const {
    return obj_field(cached_async_info_offset()); 
  }

  void  set_cached_async_info(TypeArray* value) {
   obj_field_put (cached_async_info_offset(), (Oop*)value); 
  }

  void clear_async_info() {
    obj_field_clear(async_info_offset());
  }

  // Accessors for async_redo
  int async_redo() const {
    return int_field(async_redo_offset());
  }
  void set_async_redo(int value) {
    int_field_put(async_redo_offset(), value);
  }

  // Accessors for last java stack pointer and last java frame pointer
  address last_java_sp() const {
    return (address)int_field(last_java_sp_offset());
  }
  void set_last_java_sp(address value) {
    int_field_put(last_java_sp_offset(), (jint)value);
  }
  address last_java_fp() const {
    return (address)int_field(last_java_fp_offset());
  }
  void set_last_java_fp(address value) {
    int_field_put(last_java_fp_offset(), (jint)value);
  }
  bool last_java_frame_exists() const {
    return (last_java_sp() != NULL) && (last_java_fp() != NULL);
  }

  static void    set_current(Thread* value);

  // Accessors for the next list (all threads are linked in a single list)
  ReturnOop next() {
    return obj_field(next_offset());
  }
  void set_next(Thread* value) {
    obj_field_put(next_offset(), value);
  }
  void clear_next() {
    obj_field_clear(next_offset());
  }

  // Accessors for the global  list (all threads are linked in a single list)
  ReturnOop global_next( void ) const {
    return obj_field(global_next_offset());
  }
  void set_global_next(Thread* value) {
    obj_field_put(global_next_offset(), value);
  }
  void clear_global_next() {
    obj_field_clear(global_next_offset());
  }

  ReturnOop previous() const {
    return obj_field(previous_offset());
  }
  void set_previous(Thread* value) {
    obj_field_put(previous_offset(), value);
  }
  void clear_previous() {
    obj_field_clear(previous_offset());
  }

  ReturnOop next_waiting() const {
    return obj_field(next_waiting_offset());
  }
  void set_next_waiting(Thread* value) {
    obj_field_put(next_waiting_offset(), value);
  }
  void clear_next_waiting() {
    obj_field_clear(next_waiting_offset());
  }

  ReturnOop wait_obj() const {
    return obj_field(wait_obj_offset());
  }
  void set_wait_obj(Oop* value) {
    obj_field_put(wait_obj_offset(), value);
  }
  void clear_wait_obj() {
    obj_field_clear(wait_obj_offset());
  }


  // Accessors for wait stack lock
  // (used during synchronization and java.lang.Object.wait). 
  StackLock* wait_stack_lock() const {
    jint stack_lock_offset = int_field(wait_stack_lock_offset());
    GUARANTEE(stack_lock_offset != 0, "Sanity check");
    return DERIVED(StackLock*, stack_lock_offset, stack_base());
  }

  void set_wait_stack_lock(StackLock* value) {
    GUARANTEE(value != NULL, "Sanity - Should call clear_wait_stack_lock");
    int_field_put(wait_stack_lock_offset(), DISTANCE(stack_base(), value));
  }

  void clear_wait_stack_lock() {
#ifdef AZZERT
    int_field_put(wait_stack_lock_offset(), 0);
#endif
  }

  // Accessors for wake-up time (used by Scheduler when sleeping).
  jlong wakeup_time() const {
    return long_field(wakeup_time_offset());
  }
  void set_wakeup_time(jlong value) {
    long_field_put(wakeup_time_offset(), value);
  }

  // Accessors for stack_pointer
  jint stack_pointer() {
    return int_field(stack_pointer_offset());
  }

  void set_stack_pointer(jint value) {
    int_field_put(stack_pointer_offset(), value);
  }

  // Accessors for the stack limit
  address stack_limit()         const {
    return (address) int_field(stack_limit_offset());
  }
  void set_stack_limit();
  void set_stack_limit(address value);

  // Accessors for debugger information
  jint status() {
    return int_field(status_offset());
  }

  void set_status (jint val) {
    int_field_put(status_offset(), val);
  }

// Used by debugger
#define THREAD_ACTIVE                   0
#define THREAD_SUSPENDED                1
#define THREAD_DBG_SUSPENDED           (1 << 1)
#define THREAD_DBG_SUSPENDED_BY_EVENT  (1 << 2)
#define THREAD_MONITOR_WAIT            (1 << 3)
#define THREAD_CONVAR_WAIT             (1 << 4)
#define THREAD_SLEEPING                (1 << 5)
#define THREAD_STEPPING                (1 << 6)
#define THREAD_ASYNC                   (1 << 7)
#define THREAD_JUST_BORN               (1 << 8)
#define THREAD_DEAD                    (1 << 9)
#define THREAD_IN_INVOKE               (1 << 10)
#define THREAD_NOT_ACTIVE_MASK \
    (THREAD_SUSPENDED | THREAD_MONITOR_WAIT | THREAD_CONVAR_WAIT | \
     THREAD_SLEEPING | THREAD_DEAD)


// Used by Thread.interrupt
#define THREAD_PENDING_INTERRUPT       (unsigned(1) << 31)

// Use to mark threads of terminated isolates.
#define THREAD_TERMINATING             (1 << 30)

  juint suspend_count() {
    return uint_field(suspend_count_offset());
  }

  void set_suspend_count(juint val) {
    uint_field_put(suspend_count_offset(), val);
  }

  jint inc_suspend_count() {
    jint c = suspend_count();
    int_field_put(suspend_count_offset(), c+1);
    return c;
  }
  jint dec_suspend_count() {
    jint c = suspend_count() - 1;
    int_field_put(suspend_count_offset(), (c < 0 ? 0 : c));
    return c;
  }

  void set_dbg_suspended(bool is_event) {
    set_status(status() | THREAD_DBG_SUSPENDED |
               (is_event ? THREAD_DBG_SUSPENDED_BY_EVENT : 0));
  }
  void clear_dbg_suspended() {
    set_status(status() &
               ~(THREAD_DBG_SUSPENDED | THREAD_DBG_SUSPENDED_BY_EVENT));
    set_suspend_count(0);
  }

  void clear_suspended() {
    set_status(status() & ~THREAD_SUSPENDED);
  }
  void set_async() {
    set_status(status() | THREAD_ASYNC);
  }
  void clear_async() {
    set_status(status() & ~THREAD_ASYNC);
  }

  bool is_stepping() {
    return (status() & THREAD_STEPPING);
  }
  void set_is_stepping(bool val) {
    if (val == true) {
      set_status(status() | THREAD_STEPPING);
    } else {
      set_status(status() & ~THREAD_STEPPING);
    }
  }
  ReturnOop step_info() {
    return obj_field(step_info_offset());
  }
  void set_step_info(Oop *p) {
    obj_field_put(step_info_offset(), p);
  }

  void set_suspended() {
    set_status(status() | THREAD_SUSPENDED);
  }
  void set_active() {
    set_status(status() & ~THREAD_NOT_ACTIVE_MASK);
  }

#if ENABLE_WTK_PROFILER
  ReturnOop profiler_info() {
    return obj_field(profiler_info_offset());
  }

  void set_profiler_info(OopDesc* value) {
    obj_field_put(profiler_info_offset(), value);
  }
#endif

#if ENABLE_CLDC_11

  bool is_pending_interrupt() { 
    return (status() & THREAD_PENDING_INTERRUPT) != 0;
  }

  void set_pending_interrupt() { 
    set_status(status() | THREAD_PENDING_INTERRUPT);
  }

  void clear_pending_interrupt() { 
    set_status(status() & ~THREAD_PENDING_INTERRUPT);
  }
#endif

  bool is_terminating() { 
    return (status() & THREAD_TERMINATING) != 0;
  }

  void set_terminating() { 
    set_status(status() | THREAD_TERMINATING);
  }

  void clear_terminating() { 
    set_status(status() & ~THREAD_TERMINATING);
  }

  jint priority() {
    ThreadObj::Raw tobj = thread_obj();
    return tobj().priority();
  }
  static jlong _global_tick_count;

  static jlong global_tick_count() {
    return _global_tick_count;
  }

  // Call back from interpreter and compiled code when stack overflow happens.
  static void stack_overflow(Thread *thread, address stack_pointer);
    
  // Call back when a timer tick occurs
  static void timer_tick();

#if ENABLE_INTERPRETATION_LOG
  static void process_interpretation_log();
#else
  static void process_interpretation_log() {}
#endif

#if ENABLE_COMPILER
  static void invoke_compiler(Thread *thread);
#else
  static void invoke_compiler(Thread* /*thread*/) {}
#endif

  static void update_current_stack_limit(Thread *thread);

  static void set_timer_tick() {
    _real_time_has_ticked = true;
#if ENABLE_PAGE_PROTECTION
    OsMisc_page_protect();
#elif ARM && ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    wmmx_set_timer_tick();
#else
    _rt_timer_ticks++;
#endif

    if (JavaStackDirection > 0) {
      _compiler_stack_limit = (address)0x0;
    } else {
      _compiler_stack_limit = (address)0xffffffff;
    }
  }

  static void clear_timer_tick() {
#if ENABLE_PAGE_PROTECTION
    OsMisc_page_unprotect();
#elif ARM && ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    wmmx_clear_timer_tick();
#else
    _rt_timer_ticks = 0;
#endif
    _compiler_stack_limit = _current_stack_limit;
  }

  void grow_execution_stack(int new_length JVM_TRAPS);
  void shrink_execution_stack(int new_length);

  // Prints the pending exception and a stack trace for error reporting.
  static void print_current_pending_exception_stack_trace();

  // Garbage collection support.
  void stack_oops_do(void do_oop(OopDesc**));
  void nonstack_oops_do(void do_oop(OopDesc**));
  void gc_prologue(void do_oop(OopDesc**));
  void gc_epilogue(void);

  // Printing and debugging support.
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void trace_stack(Stream *st);
  void trace_stack_from(Frame* frame, Stream* st);
  void print_value() { print_value_on(tty); }
  void print_value_on(Stream* st);
  bool is_on_stack(address addr);
  static void iterate_oopmaps(oopmaps_doer do_map, void *param);
  void iterate(OopVisitor* visitor);
#endif

  static bool shrink_execution_stacks();

  address stack_base() const;
  static void start_lightweight_thread(void);
  static void lightweight_thread_exit();
  static void lightweight_thread_uncaught_exception();

 private:
  void setup_lightweight_stack(JVM_SINGLE_ARG_TRAPS);
  static void maybe_shrink_execution_stack(Thread* thread, oop_doer do_oop);

  friend class Scheduler;
  friend class Oop;
  static int _current_resource_owner_id;
  static int _shrunk_stack_count;
  static bool _real_time_has_ticked;
};

#define ForAllThreads( var )    \
  for( Thread::Raw var = Universe::global_threadlist()->obj();  \
       var.not_null(); var = var().global_next() )              \
    if( var().last_java_frame_exists() )
