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
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_Thread.cpp.incl"

int Thread::_shrunk_stack_count;
bool Thread::_real_time_has_ticked;

// the global 'time of day' in ticks for scheduling
jlong Thread::_global_tick_count = 1;

static int _thread_creation_count;

HANDLE_CHECK(Thread, is_jvm_thread())

void Thread::append_pending_entry(EntryActivation* entry) {
  EntryActivation::Raw current = pending_entries();
  if (current.is_null()) {
    set_pending_entries(entry);
  } else {
    EntryActivation::Raw next = current().next();
    while (!next.is_null()) {
      current = next;
      next    = current().next();
    }
    current().set_next(entry);
  }
}

void Thread::setup_lightweight_stack(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  const int stack_size = StackSize;

  GUARANTEE(wakeup_time_offset() % 8 == 0,
            "jlongs in ThreadDesc must be 8 byte aligned");

  // it's here  as Thread::allocate not invoked for the main thread 
  // in ROMized case, and we still need it
#if ENABLE_WTK_PROFILER
  OopDesc* info = WTKProfiler::allocate_thread_data(JVM_SINGLE_ARG_CHECK);
  set_profiler_info(info);
#endif

  // Allocate stack and set stack pointer to bottom
  ExecutionStack::Fast stack = Universe::new_execution_stack(stack_size JVM_CHECK);

  address sp =(address)stack.field_base(JavaStackDirection < 0 ? stack_size
                                        : ExecutionStackDesc::header_size());
#if defined(UNDER_CE)
  // I thought I only needed this for debugging under EVC++, but when I ran
  // cldc_vm under the test utility on the iPaq it hung unless this was
  // uncommented.  Run mintck uses CreateProcess to run the mintck tests.
  // If I run the mintck tests using a .lnk file it works without the
  // following line
  sp = (address)((int)sp |_system_address);
#endif
  set_execution_stack(&stack);
  stack().set_thread(this);
  set_stack_limit();

  sp = setup_stack_asm(sp);
  set_stack_pointer((jint)sp);
}

void force_terminated(Thread* thread) {
  ThreadObj::Raw thread_obj = thread->thread_obj();
  thread_obj().set_terminated();
}

#if ENABLE_ISOLATES

ReturnOop Thread::task_for_thread( void ) const {
  return Task::get_task(task_id());
}

extern "C" {
  void thread_task_cleanup()  {
    SETUP_ERROR_CHECKER_ARG;
    // We come in here on primordial stack
    UsingFastOops fast_oops;
    Thread *thread = Thread::current();
    // cleanup any taskmirrors awaiting initialization.  They may be
    // on the clinit list because of an exception being thrown during
    // static initialization
    int tid = thread->task_id();
    GUARANTEE(tid >= 0 && tid < MAX_TASKS, "Not a valid task id");
    Task::Fast this_task = Task::get_task(tid);
    GUARANTEE(!this_task.is_null(), "Not a valid task");
    TaskMirror::Raw tm = this_task().clinit_list();
    while (!tm.is_null()) {
      TaskMirror::Raw next = tm().next_in_clinit_list();
      if (tm().init_thread() == thread->obj()) {
        JavaClass::Raw jc = tm().containing_class();
        TaskMirror::clinit_list_remove(&jc);
        Universe::mirror_list()->obj_at_put(jc().class_id(),
                                 Universe::task_class_init_marker());
      }
      tm = next.obj();
    }

    bool all_threads_terminated = this_task().remove_thread();
    if (all_threads_terminated) {
      if (TraceThreadsExcessive) {
        TTY_TRACE_CR(("Cleanup task 0x%x", tid));
      }
      // if this is the last thread of the last task to exit, its
      // exit code will get returned.
      JVM::set_exit_code(this_task().exit_code());
      Thread::clear_current_pending_exception();
      // will invoke task termination to produce events and close links.
      this_task().terminate_current_isolate(thread JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}
#endif

#if !defined(ARM) || CROSS_GENERATOR
void Thread::start_lightweight_thread() {
  Thread* thread = Thread::current();
  // idea here is that some threads can't be terminated
  // by throwing an uncatchable exceptions if they're in 'just started'
  // state (i.e. never executed), as this exception will be silently ignored
  // by shared_entry(), thus we have to check thread status before
  // actual execution of pending entries, and null them out, 
  // if thread is terminating
  if (thread->is_terminating()) {
    Oop::Raw null;
    thread->set_pending_entries(&null);
  } else {
    invoke_pending_entries(thread);
  }

  if (!Universe::is_stopping() && !TestCompiler) {
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      call_on_primordial_stack(lightweight_thread_uncaught_exception);
    }
    call_on_primordial_stack(finish);
    invoke_pending_entries(thread);
    // Just in case the append_pending_entry() failed inside
    // Thread::finish()
    force_terminated(thread);
#if ENABLE_ISOLATES
#if !ARM && !HITACHI_SH
    {
      call_on_primordial_stack(thread_task_cleanup);
      invoke_pending_entries(thread);
    }
#endif
#endif
  }
  // The following returns to another Java thread, unless this is
  // the only remaining Java thread.
  call_on_primordial_stack(lightweight_thread_exit);

  // We can only return here if we are the last thread.
  // Running on the Java stack at this point, no handles please
  GUARANTEE(Scheduler::get_next_runnable_thread()->is_null(),
            "Must be last thread");

  // Back to C land
  current_thread_to_primordial();

  SHOULD_NOT_REACH_HERE();
}
#endif

void Thread::lightweight_thread_uncaught_exception() {
  Oop::Raw exception = Thread::current_pending_exception();
  if (exception.equals(Universe::string_class())) {
    // This was thrown using Throw::uncatchable() or with
    // Task::get_termination_object()
    clear_current_pending_exception();
  } else {
    tty->print(MSG_UNCAUGHT_EXCEPTIONS);
    print_current_pending_exception_stack_trace();
    clear_current_pending_exception();
  }
}

void Thread::lightweight_thread_exit() {
  SETUP_ERROR_CHECKER_ARG;
  Thread *thread = Thread::current();
  if (TraceThreadsExcessive) {
    TTY_TRACE_CR(("thread exit 0x%x", thread->obj()));
  }
  if (!TestCompiler) {
    Thread::clear_current_pending_exception();
  }
#if ENABLE_ISOLATES
  int tid = thread->task_id();
#endif
  // remove thread from scheduler
  Scheduler::terminate(thread JVM_NO_CHECK);
  {
    // Warning: ExecutionStack::~ExecutionStack() must be called before we
    // switch threads below.
    ExecutionStack::Raw stack = thread->execution_stack();
    stack().clear_thread();
    thread->clear_execution_stack();
  }
  Scheduler::yield();
  // At this point we either have another thread to run OR this is the
  // very last thread in the system and the VM will exit
#if ENABLE_ISOLATES
  if (!Scheduler::get_next_runnable_thread()->is_null()) {
    // Another thread will run, cleanup the task that may have just
    // terminated.
    int thread_count = 0;
    {
      Task::Raw task = Task::get_task(tid);
      thread_count = task().thread_count();
    }
    if (thread_count == 0) {
#if ENABLE_PROFILER
      if (UseProfiler) {
        Profiler::dump_and_clear_profile_data(tid);
      }
#endif
#if ENABLE_WTK_PROFILER
      if (UseExactProfiler) {
        WTKProfiler::dump_and_clear_profile_data(tid);
      }
#endif
      // After profiler data is dumped _current_thread is no longer used
      // and should be cleared in order to dispose the reference
      // to the task being terminated
      _current_thread = NULL;
      Task::cleanup_terminated_task(tid JVM_NO_CHECK);
    }
  } else {
    // This GUARANTEE is too strict: in SlaveMode the GUI may call back to
    // the VM too soon. Just ignore and continue.
    //
    // GUARANTEE(Scheduler::active_count() == 0,
    //          "Active threads but nothing to run?");
  }
#endif
  while (_debugger_active && !Scheduler::is_slave_mode() &&
         Scheduler::get_next_runnable_thread()->is_null() &&
         Scheduler::active_count() > 0) {
      // All threads are suspended by the debugger, wait until one gets resumed
      Scheduler::yield();
  }
  if (!Scheduler::get_next_runnable_thread()->is_null()) {
    // We will return to a different thread that the one that sent us here
    Thread* next = Scheduler::get_next_runnable_thread();
    Thread::set_current(next);
    // Ok to allocate now, thread is not visible to GC
    Scheduler::allocate_blocked_threads_buffer(JVM_SINGLE_ARG_NO_CHECK);
    if (TraceThreadsExcessive) {
      TTY_TRACE_CR(("thread exit: next 0x%x", Thread::current()->obj()));
    }
  }
}

ReturnOop Thread::allocate(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  GUARANTEE(!Universe::is_bootstrapping(),
            "Cannot allocate thread while bootstrapping");
  
  Thread::Fast thread = Universe::new_thread(JVM_SINGLE_ARG_CHECK_0);
  thread().setup_lightweight_stack(JVM_SINGLE_ARG_CHECK_0);
  thread().set_id(_thread_creation_count++);

  return thread;
}

void Thread::set_current(Thread* value) {
  GUARANTEE(_last_handle == NULL, "No handles when switching threads");
  GUARANTEE(last_kni_handle_info == NULL,
                                 "No KNI handles when switching threads");
  GUARANTEE(!_jvm_in_quick_native_method,
            "cannot switch thread in quick native methods");

  if (VerifyOnly) {
    GUARANTEE(current()->is_null() || value->is_null() ||
              current()->equals(value),
              "cannot switch threads during Romization or VerifyOnly");
  }

  if (current()->not_null()) {
    decache_current_pending_exception();
  }
  _current_thread = value->obj();
  cache_current_pending_exception();

  if (TraceThreadsExcessive) {
    TTY_TRACE_CR(("set_current_thread: 0x%x (id=%d)", value->obj(),
                  (value->is_null() ? -1 : value->id())));
  }
#if ENABLE_ISOLATES
  {
    int tid = value->task_id();
    Task::Raw task = Universe::task_from_id(tid);
    Universe::set_current_task(tid);
    // have to be here, not in Universe::set_current_task()
    ObjectHeap::on_task_switch(tid);

    if (task().is_terminating()){
#ifdef AZZERT
      GUARANTEE(!task.is_null() && task().status() >= Task::TASK_STOPPING,
                "task of terminating thread must be stopping");
#endif
      // The task that this thread belongs to is dead
      if (TraceThreadsExcessive) { 
        TTY_TRACE_CR(("set_current: task dead: thread 0x%x",
                      current()->obj()));
      }
      current()->set_terminating();
      set_current_pending_exception(Task::get_termination_object());
    } 
  }
#endif

  // Update the current stack limit
  update_current_stack_limit(value);

#if ENABLE_JAVA_DEBUGGER
  if (_debugger_active) {
    JavaDebugger::set_stepping(value->is_stepping());
  }
#endif
}

void Thread::start(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ThreadObj::Fast receiver = thread_obj();
  InstanceClass::Fast thread_class = receiver.blueprint();

  // Find the 'run' method to invoke
  Method::Fast run_method = thread_class().lookup_method(Symbols::run_name(),
                                                 Symbols::void_signature());
  if (run_method.is_null()) {
#ifndef PRODUCT
    tty->print_cr("Error: run method not found in ");
    thread_class.print();
    tty->cr();
#endif
    JVM_FATAL(run_method_not_found);
  }

  // Setup execution entry
  EntryActivation::Fast run_entry =
      Universe::new_entry_activation(&run_method, 1 JVM_CHECK);
  run_entry().obj_at_put(0, &receiver);
  append_pending_entry(&run_entry);

  Scheduler::start(this JVM_NO_CHECK_AT_BOTTOM);
}

void Thread::finish() {
  SETUP_ERROR_CHECKER_ARG;
  Thread *thread = Thread::current();
  ThreadObj receiver = thread->thread_obj();
  GUARANTEE(receiver.is_alive(), "Sanity check");

  if (TraceThreadsExcessive) {
    TTY_TRACE_CR(("thread dying 0x%x", thread->obj()));
  }
  // Set thread state to terminated
  receiver.set_terminated();
  // Signal waiters waiting on thread
  Scheduler::notify(&receiver, true, false JVM_NO_CHECK_AT_BOTTOM);
}

void Thread::initialize_main(JVM_SINGLE_ARG_TRAPS) {
  _thread_creation_count = 1;
  // StackPadding is the amount of spaces we put at the top of the stack
  // to be used by the OS (to handle interrupt) and by "leaf compiled methods",
  // which do not contain stack overflow checks:
  //
  // LeafMethodStackPadding                = reserved for leaf compiled methods
  // StackPadding - LeafMethodStackPadding = reserved for OS
  GUARANTEE(LeafMethodStackPadding < StackPadding, "sanity");

  Scheduler::allocate_blocked_threads_buffer(1 JVM_CHECK);
  setup_lightweight_stack(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void Thread::dispose() {
  _current_thread      = NULL;
  _current_stack_limit = NULL;
}

void Thread::print_current_pending_exception_stack_trace() {
  UsingFastOops fast_oops;
  Throwable::Fast exception = Thread::current_pending_exception();
  GUARANTEE(!exception.is_null(), "sanity");
  exception().print_stack_trace();
}

void Thread::set_thread_obj(ThreadObj* value) {
  obj_field_put(thread_obj_offset(), value);
  if (value->not_null()) value->set_thread(this);
}

void Thread::stack_oops_do(void do_oop(OopDesc**)) {
  if (this->task_id() == Task::INVALID_TASK_ID) {
    // If we continue we'll break JVMTaskGCContext with a -1 task id
    GUARANTEE(!last_java_frame_exists(), "thread is already dead");
    return;
  }

  // See Scheduler.cpp threads_do_list() for the
  // reason why we do this.
  const TaskGCContext tmp(this->task_id());

  // Java frames
  if (last_java_frame_exists()) {
    Frame fr(this);
    while (true) {
      if (fr.is_entry_frame()) {
        fr.as_EntryFrame().oops_do(do_oop);
        if (fr.as_EntryFrame().is_first_frame()) {
          break;
        }
        fr.as_EntryFrame().caller_is(fr);
      } else {
        fr.as_JavaFrame().oops_do(do_oop);
        fr.as_JavaFrame().caller_is(fr);
      }
    }
  }
}

void Thread::nonstack_oops_do(void do_oop(OopDesc**)) {
  if (_debugger_active) {
    OopDesc *step = (OopDesc *)int_field(step_info_offset());
    if (step != NULL) {
      do_oop(&step);
    }
  }
}

void Thread::gc_prologue(void do_oop(OopDesc**)) {
  // Java frames
  if (last_java_frame_exists()) {
    Frame fr(this);
    while (true) {
      if (fr.is_entry_frame()) {
        fr.as_EntryFrame().gc_prologue(do_oop);
        if (fr.as_EntryFrame().is_first_frame()) {
          break;
        }
        fr.as_EntryFrame().caller_is(fr);
      } else {
        fr.as_JavaFrame().gc_prologue(do_oop);
        fr.as_JavaFrame().caller_is(fr);
      }
    }
  }
}

void Thread::gc_epilogue(void) {
  // Java frames
  if (last_java_frame_exists()) {
    Frame fr(this);
    while (true) {
      if (fr.is_entry_frame()) {
        fr.as_EntryFrame().gc_epilogue();
        if (fr.as_EntryFrame().is_first_frame()) {
          break;
        }
        fr.as_EntryFrame().caller_is(fr);
      } else {
        fr.as_JavaFrame().gc_epilogue();
        fr.as_JavaFrame().caller_is(fr);
      }
    }
  }
}

void Thread::stack_overflow(Thread *thread, address stack_pointer) {
  SETUP_ERROR_CHECKER_ARG;
  if (!Universe::is_stopping() &&
       (JavaStackDirection < 0 ? (stack_pointer <= _current_stack_limit)
                               : (stack_pointer >= _current_stack_limit))) {
    ExecutionStack::Raw old_stack = thread->execution_stack();
    jint old_stack_size = old_stack().length();
    jint new_stack_size;
    jint stack_needed_size =
        JavaStackDirection * (stack_pointer - thread->stack_limit());
    if (stack_needed_size > StackSizeIncrement) {
      new_stack_size = old_stack_size + stack_needed_size + StackPadding;
    } else {
      new_stack_size = old_stack_size + StackSizeIncrement + StackPadding;
    }
    if (new_stack_size <= StackSizeMaximum) {
      thread->grow_execution_stack(new_stack_size JVM_NO_CHECK_AT_BOTTOM);
    } else {
      // Throw out of memory error and not stack overflow exception
      // since that exception is not part of CLDC 1.0
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
    }
  }
}

void Thread::timer_tick() {
  SETUP_ERROR_CHECKER_ARG;
  Thread *thread = Thread::current();
#ifndef PRODUCT
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "\n*** Timer tick beg ***" ));
  }
#endif

  _global_tick_count++;

#if ENABLE_PROFILER
  // We must check is_ready() to distinguish real time ticks
  // from ticks generated by shared_invoke_compiler
  if (Profiler::is_ready()) {
    JavaFrame f(thread);
    Method method(f.method());
    Profiler::profile_method(&method, f.is_compiled_frame());
  }
#endif

#if ENABLE_JVMPI_PROFILE && ENABLE_JVMPI_PROFILE_VERIFY 
  // Notice: To ensure that dump method is in the same thread with the 
  // Compiler,  so we dump here.
  if(JVMPIProfile::need_dump && UseJvmpiProfiler) {
     JVMPIProfile::need_dump = false;
     JVMPIProfile::VMjvmpiPostDumpJVMPIEventsInfo();
  }
#endif

#if ENABLE_COMPILER

  ObjectHeap::accumulate_current_task_memory_usage();
  Compiler::on_timer_tick(_real_time_has_ticked JVM_MUST_SUCCEED);
  _real_time_has_ticked = false;
#endif

  if (Universe::is_stopping()) {
    if (!CURRENT_HAS_PENDING_EXCEPTION) {
      Throw::uncatchable(JVM_SINGLE_ARG_THROW);
    }
  }
  // Do the preemption. yield will transfer the control
  // to the next ready thread in the scheduler.
  ObjectHeap::accumulate_current_task_memory_usage();
  Scheduler::yield();
  Thread::clear_timer_tick();

#if ENABLE_CPU_VARIANT
  if (EnableCPUVariant) {
    enable_cpu_variant();
  }
#endif

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "\n*** Timer tick end ***" ));
  }
  (void)thread;
}

void Thread::set_stack_limit() {
  ExecutionStack::Raw stack = execution_stack();
  const int stack_padding_size = StackPadding;
  if (JavaStackDirection < 0) {
    set_stack_limit((address)stack.field_base(stack_padding_size));
  } else {
    jint stack_size = stack().length();
    set_stack_limit((address)stack.field_base(stack_size - stack_padding_size));
  }
}

void Thread::set_stack_limit(address value) {
#ifdef UNDER_CE
  value = (address)((int)value | _system_address);
#endif
  int_field_put(stack_limit_offset(), (jint) value);
  if (Scheduler::get_gc_current_thread() != NULL) {
    // in a gc so check the old current thread pointer
    if (this->obj() == Scheduler:: get_gc_current_thread()) {
      update_current_stack_limit(this);
    }
  } else {
    if (Thread::current()->equals(this)) {
      update_current_stack_limit(this);
    }
  }
}

void Thread::update_current_stack_limit(Thread *thread) {
  // Set the current stack limit to the thread stack limit.
  _current_stack_limit = (address)thread->stack_limit();
  _compiler_stack_limit = _current_stack_limit;
}

void Thread::grow_execution_stack(int new_stack_size JVM_TRAPS) {
  ExecutionStack::Raw new_stack = Universe::new_execution_stack(new_stack_size
                                                                JVM_CHECK);
  ExecutionStack::Raw old_stack = execution_stack();
  jint           old_stack_size = old_stack().length();
  GUARANTEE(new_stack_size > old_stack_size, "sanity check");

  GCDisabler dont_gc_for_rest_of_this_method;

  address   old_stack_ptr   = (address)stack_pointer();
  jint      stack_used_size;
  jint      delta;
  if (JavaStackDirection < 0) {
    address old_stack_end = (address)old_stack().field_base(old_stack_size);
    address new_stack_end = (address)new_stack().field_base(new_stack_size);
#ifdef UNDER_CE
    stack_used_size = (address)((int)old_stack_end | _system_address)
                                    - old_stack_ptr;
#else
    stack_used_size = old_stack_end - old_stack_ptr;
#endif
    delta = new_stack_end - old_stack_end;
  } else {
    const int offset          = ExecutionStackDesc::header_size();
    address   old_stack_start = (address)old_stack().field_base(offset);
    address   new_stack_start = (address)new_stack().field_base(offset);
#ifdef UNDER_CE
    stack_used_size = old_stack_ptr -
        (address)((int)old_stack_start | _system_address);
#else
    stack_used_size = old_stack_ptr - old_stack_start;
#endif
    delta = new_stack_start - old_stack_start;
  }
  address   new_stack_ptr   = old_stack_ptr + delta;

  ((ExecutionStackDesc*)(old_stack.obj()))->relocate_internal_pointers(delta,
                                                                       this,
                                                                       true);
  GUARANTEE((address)stack_pointer() == new_stack_ptr, "sanity");

  if (JavaStackDirection < 0) {
    jvm_memcpy(new_stack_ptr, old_stack_ptr  , stack_used_size);
  } else {
    jvm_memcpy(new_stack_ptr - stack_used_size,
               old_stack_ptr - stack_used_size,
               stack_used_size + 4);
  }
  old_stack().clear_thread();

  set_execution_stack(&new_stack);
  new_stack().set_thread(this);

  set_stack_limit();
}

bool Thread::shrink_execution_stacks() {
  _shrunk_stack_count = 0;
  Scheduler::iterate(maybe_shrink_execution_stack);
  return _shrunk_stack_count > 0;
}

void Thread::maybe_shrink_execution_stack(Thread* thread, oop_doer /*dummy*/) {
  const ExecutionStack::Raw stack(thread->execution_stack());
  const jint stack_size = stack().length();

  if (stack_size > StackSize) {
    address stack_limit = (address)thread->stack_pointer();
    {
      const Frame frame(thread);
      if (frame.is_java_frame()) {
        const Method::Raw m(frame.as_JavaFrame().method());
        stack_limit = frame.fp() +
          (JavaStackDirection * m().max_execution_stack_count() *
           BytesPerStackElement);
      }
    }
    // The following is just some random extra space.  We may need more
    stack_limit += JavaStackDirection * 10 * BytesPerLong;
    // stack has grown beyond the original size

    const address stack_end =
        (address)stack.field_base( JavaStackDirection < 0 ? stack_size
                                         : ExecutionStackDesc::header_size() );
    const jint length_used_size =
        JavaStackDirection < 0 ? (stack_end - stack_limit)
                               : (stack_limit - stack_end) ;
    jint length_needed_size =
        length_used_size + StackPadding + StackSizeIncrement;

    if (length_needed_size < StackSize) {
      length_needed_size = StackSize;
    }
    const jint parts = (stack_size - length_needed_size) / StackSizeIncrement;
    if (parts > 0) {
      const juint new_stack_size = stack_size - parts * StackSizeIncrement;
      thread->shrink_execution_stack(new_stack_size);
      _shrunk_stack_count ++;
    }
  }
}

void Thread::shrink_execution_stack(int new_stack_size) {
  GCDisabler dont_gc_for_rest_of_this_method;
  ExecutionStack::Raw old_stack = execution_stack();
  GUARANTEE(new_stack_size < old_stack().length(), "sanity check");

  ExecutionStack::Raw new_stack = old_stack().shrink_in_place(new_stack_size);
  // We shrink the stack in place, so we do not need to discard the
  // old stack in any way.  Universe::shrink_object takes care of that.
  set_execution_stack(&new_stack);
  new_stack().set_thread(this);

  set_stack_limit();
}

// real_time_tick is our timer interrupt.
void real_time_tick(int delay_time) {
  (void)delay_time;
  if (!VerifyOnly) {
    // No thread switch should happen in VerifyOnly mode
    // conversion.
    Thread::set_timer_tick();
    Scheduler::set_timer_tick();
  }

#if ENABLE_PERFORMANCE_COUNTERS
  jvm_perf_count.num_of_timer_ticks ++;
#endif

#if ENABLE_PROFILER
  Profiler::tick(delay_time);
#endif

#if ENABLE_JVMPI_PROFILE_VERIFY && ENABLE_JVMPI_PROFILE
  JVMPIProfile::VMTick(delay_time);
#endif

#if ENABLE_CPU_VARIANT
  if (EnableCPUVariant) {
    disable_cpu_variant();
  }
#endif
}

address Thread::stack_base() const {
  const ExecutionStack::Raw stack(execution_stack());
  return (address)stack().field_base(
                          JavaStackDirection < 0 ? stack().length() : 0 );
}

bool Thread::has_user_frames_until(int num_frames) {
  Frame fr(this);
  bool forever = (num_frames == 0);
  while (forever || num_frames-- > 0) {
    if (fr.is_entry_frame()) {
      break;
    } else {
      const JavaFrame jf = fr.as_JavaFrame();
      const Method::Raw m = jf.method();
      const JavaClass::Raw klass = m().holder();
      if (!klass().is_preloaded()) {
        return true;
      }
      jf.caller_is(fr);
    }
  }
  
  return false;
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void Thread::iterate_oopmaps(oopmaps_doer do_map, void *param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, previous);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, global_next);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next_waiting);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, wait_obj);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, pending_exception);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, thread_obj);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, pending_entries);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, execution_stack);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, step_info);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, obj_value);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, async_info);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, cached_async_info);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    id);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    last_java_sp);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    last_java_fp);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    async_redo);
  OOPMAP_ENTRY_4(do_map, param, T_LONG,   wakeup_time);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    wait_stack_lock);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    stack_limit);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    stack_pointer);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    status);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    suspend_count);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    int1_value);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    int2_value);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    task_id);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, profiler_info);
#endif
}

void Thread::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  iterate_oopmaps(BasicOop::iterate_one_oopmap_entry, (void*)visitor);
#endif
}

void Thread::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("Thread 0x%lx (id=%d) ", obj(), id());
#if ENABLE_ISOLATES
  st->print("task ID 0x%lx ", task_id());
#endif
#endif
}

void Thread::trace_stack(Stream* st) {
#if ENABLE_STACK_TRACE
#if ENABLE_ISOLATES
  TaskContext maybeSwitchTask(task_id());
  st->print_cr("task ID 0x%lx ", task_id());
#endif
  Frame fr(this);
  trace_stack_from(&fr, st);
#endif
}

void Thread::trace_stack_from(Frame* frame, Stream* st) {
#if ENABLE_STACK_TRACE
  st->print_cr("Stack Trace (id=%d) [", id());
  Frame fr(*frame);
  int index = 0;
  while (true) {
    if (fr.is_entry_frame()) {
      fr.as_EntryFrame().print_on(st, index++);
      if (fr.as_EntryFrame().is_first_frame()) {
        break;
      }
      fr.as_EntryFrame().caller_is(fr);
    } else {
      fr.as_JavaFrame().print_on(st, index++);
      fr.as_JavaFrame().caller_is(fr);
    }
  }
  st->print_cr("]");
  st->cr();
#endif
}

bool Thread::is_on_stack(address addr) {
#if defined (UNDER_CE)
  //  addr = (address)((int)addr & 0x00ffffff);
#endif
  const int header_size = ExecutionStackDesc::header_size();

  const ExecutionStack::Raw stack = execution_stack();
  const int length = stack().length();

  return addr >= (address)stack.field_base(header_size)
      && addr <  (address)stack.field_base(length + header_size);
}

#endif // !PRODUCT

