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
# include "incls/_ThreadReferenceImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER
// Return the JDWP thread status for this thread

jint ThreadReferenceImpl::get_jdwp_thread_status(Thread *thread)
{
  
  if (!JavaDebugger::is_valid_thread(thread)) {
    return JDWP_ThreadStatus_ZOMBIE;
  }

  int state = thread->status();
  if (state == THREAD_SUSPENDED) {
    return JDWP_ThreadStatus_RUNNING;
  } else {
    state &= (THREAD_NOT_ACTIVE_MASK & ~(THREAD_SUSPENDED | THREAD_DBG_SUSPENDED));
    switch (state) {
    case THREAD_ACTIVE:
      return JDWP_ThreadStatus_RUNNING;
    case THREAD_DEAD:
      return JDWP_ThreadStatus_ZOMBIE;
    case THREAD_MONITOR_WAIT:
      return JDWP_ThreadStatus_MONITOR;
    case THREAD_CONVAR_WAIT:
      return JDWP_ThreadStatus_WAIT;
    case THREAD_SLEEPING:
      return JDWP_ThreadStatus_SLEEPING;
    default:
      //      return JDWP_ThreadStatus_RUNNING;
      return JDWP_ThreadStatus_UNKNOWN;
    }
  }
}

// Return the JDWP suspend status for this thread

jint ThreadReferenceImpl::get_jdwp_thread_suspend_status(Thread *thread) 
{
    jint ret = 0;
    int state = thread->status();
    
    //    if (state == THREAD_SUSPENDED) { 
    //        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    //    }

    if (state & THREAD_DBG_SUSPENDED) { 
        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    }
        
    //    if ((state & THREAD_MONITOR_WAIT) || (state & THREAD_CONVAR_WAIT)) { 
    //        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    //    }

    return ret;
}

void ThreadReferenceImpl::suspend_all_threads(int task_id, bool is_event)
{

  UsingFastOops fast_oops;
  Thread::Fast current;
  Thread::Fast thread = Universe::global_threadlist();
  Oop null_oop;
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadSuspendAllThreads: task_id: %d", task_id);
  }
#endif

  while (thread.not_null()) {
#if ENABLE_ISOLATES
    if (task_id != -1 && thread().task_id() != task_id) {
      thread = thread().global_next();
      continue;
    }
#endif
    suspend_specific_thread(&thread, task_id, is_event);
    thread = thread().global_next();
  }
  Scheduler::set_next_runnable_thread((Thread *)&null_oop);
  JavaDebugger::set_loop_count(1);
}

void ThreadReferenceImpl::resume_all_threads(int task_id)
{

  UsingFastOops fast_oops;
  Thread::Fast current;
  Thread::Fast thread = Universe::global_threadlist();
  Oop null_oop;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadResumeAllThreads: task_id: %d", task_id);
  }
#endif
  while (thread.not_null()) {
#if ENABLE_ISOLATES
    if (task_id != -1 && thread().task_id() != task_id) {
      thread = thread().global_next();
      continue;
    }
#endif
    resume_specific_thread(&thread, task_id);
    thread = thread().global_next();
  }
}

void ThreadReferenceImpl::suspend_specific_thread(Thread *thread, int task_id,
                                                  bool is_event)
{
  Oop null_oop;

  if (!JavaDebugger::is_valid_thread(thread)) {
    return;
  }
#if ENABLE_ISOLATES
    if (task_id != -1 && thread->task_id() != task_id) {
      return;
    }
#else
    (void)task_id;
#endif
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadSuspend: ObjectID=%lx, Thread: 0x%x, id = 0x%x", 
                  JavaDebugger::get_thread_id_by_ref (thread), (int)thread->obj(), thread->id());
  }
#endif
  thread->inc_suspend_count();
  thread->set_dbg_suspended(is_event);
  Scheduler::set_next_runnable_thread((Thread *)&null_oop);
}

void ThreadReferenceImpl::resume_specific_thread(Thread *thread, int task_id)
{
  if (JavaDebugger::is_valid_thread(thread)) {
#if ENABLE_ISOLATES
    if (task_id != -1 && thread->task_id() != task_id) {
      return;
    }
#else
    (void)task_id;
#endif
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadResume: ObjectID=%lx, Thread: 0x%x, id = 0x%x", 
                  JavaDebugger::get_thread_id_by_ref(thread), (int)thread->obj(), thread->id());
  }
#endif
    if (thread->dec_suspend_count() <= 0) {
      thread->clear_dbg_suspended();
    }
  }
}

// Return the name of this thread.  We make one up based on the ID.

void ThreadReferenceImpl::thread_reference_name(PacketInputStream *in, 
                                                PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Thread::Fast thread;
  int thread_id = in->read_int();
  thread = JavaDebugger::get_thread_by_id(thread_id);

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadName: xptr = 0x%x, Thread%lx",
                  in->transport()->obj(), thread_id);
  }
#endif

  if (JavaDebugger::is_valid_thread(&thread)) {
    char name[32];
    jvm_sprintf(name, "Thread%x", JavaDebugger::get_thread_id_by_ref(&thread));
    out->write_raw_string(name);
  } else { 
    const char *nullString = "";
    out->write_raw_string(nullString);
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr(" null thread");
  }
#endif
  }
  out->send_packet();
}

// Suspend this particular thread

void ThreadReferenceImpl::thread_reference_suspend(PacketInputStream *in, 
                                           PacketOutputStream *out)
{
  UsingFastOops fast_oops;
  Thread::Fast thread;
  thread = in->read_thread_ref();
  int task_id = -1;
#if ENABLE_ISOLATES
  task_id = in->transport()->task_id();
#endif

  if (JavaDebugger::is_valid_thread(&thread)) {
    suspend_specific_thread(&thread, task_id, false);
  } else {
    out->set_error(JDWP_Error_INVALID_THREAD);
  }
  out->send_packet(); // no error
}

// Resume this thread

void ThreadReferenceImpl::thread_reference_resume(PacketInputStream *in, 
                       PacketOutputStream *out)
{
  UsingFastOops fast_oops;
  Thread::Fast thread;
  thread = in->read_thread_ref();

  if (JavaDebugger::is_valid_thread(&thread)) {
    ThreadReferenceImpl::resume_specific_thread(&thread,
                                                in->transport()->task_id());
  } else {
    out->set_error(JDWP_Error_INVALID_THREAD);
  }
  out->send_packet();  // no error
}

// Return the JDWP thread status

void ThreadReferenceImpl::thread_reference_status(PacketInputStream *in, 
                       PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Thread::Fast thread;
  thread = in->read_thread_ref();

#ifdef AZZERT
  if (TraceDebugger) {
    //    tty->print_cr("ThreadStatus:  xprt = 0x%x, ThreadID=%lx,",
    //                  in->transport()->obj(),
    //                  JavaDebugger::get_thread_id_by_ref(&thread));
  }
#endif

  if (JavaDebugger::is_valid_thread(&thread)) {
    out->write_int(get_jdwp_thread_status(&thread));
    out->write_int(get_jdwp_thread_suspend_status(&thread));
#ifdef AZZERT
    if (TraceDebugger) {
      //      tty->print_cr("    status= %d, suspend status = %d",
      //                    get_jdwp_thread_status(&thread),
      //                    get_jdwp_thread_suspend_status(&thread));
    }
#endif
  } else {
    // out->set_error(out, JDWP_Error_INVALID_THREAD);
    out->write_int(JDWP_ThreadStatus_ZOMBIE);
    out->write_int(!JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED);
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("    status= Invalid Thread");
    }
#endif
  }
  out->send_packet();
}

// Return how many times this thread was suspended

void ThreadReferenceImpl::thread_reference_suspend_count(PacketInputStream *in, 
                             PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Thread::Fast thread;
  thread = in->read_thread_ref();

  if (JavaDebugger::is_valid_thread(&thread)) {
    out->write_int(thread().suspend_count());
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("ThreadSuspendCount: threadID=%ld", 
                    JavaDebugger::get_thread_id_by_ref(&thread));
      tty->print_cr("    count = %d", thread().suspend_count());
    }
#endif
  } else {
    out->set_error(JDWP_Error_INVALID_THREAD);
  }
  out->send_packet();
}

// Return call stack information for a give number of frames for this thread

void ThreadReferenceImpl::thread_reference_frames(PacketInputStream *in, 
                       PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Thread::Fast thread;
  thread = in->read_thread_ref();
  jint start_index = in->read_int();
  jint len = in->read_int();
  int index, frame_count;

        
  if (!JavaDebugger::is_valid_thread(&thread)) {
    out->send_error(JDWP_Error_INVALID_THREAD);
    return;
  }
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ThreadFrames: xptr = 0x%x, threadID=0x%x, start=%ld, length=%ld", 
                  in->transport()->obj(),
                  JavaDebugger::get_thread_id_by_ref(&thread), 
                  start_index, len);
  }
#endif
  if (!thread().last_java_frame_exists()) {
    // in MVM mode we may be asked about a thread that is running so
    // we can't get to the frame
    out->write_int(0);
    out->send_packet();
    return;
  }
  Frame frx(&thread);
  /* first get the count of frames on the stack */
  frame_count = get_frame_count(frx);
  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on frame count
  if (frame_count < 0) {
    out->send_error(JDWP_Error_INVALID_INDEX);
    return;
  }
  if (start_index >= frame_count) {
    out->send_error(JDWP_Error_INVALID_INDEX);
    return;
  }
 
  // Set frameCount to be the index of the last frame to retrieve
  if ((len != -1) && (len < (frame_count - start_index))) { 
    frame_count = start_index + len;
  } else { 
    // do nothing
  }

  out->write_int(frame_count - start_index);

  Frame fr(&thread);
  // frame id's are 1 based in StackFrameImpl
  if (!StackFrameImpl::get_frame(fr, start_index + 1)) {
    out->send_error(JDWP_Error_INVALID_INDEX);
    return;
  }

  // fr now points to first frame to send over
  for (index = start_index; index < frame_count; index++) { 
    UsingFastOops fast_oops_2;
    while (fr.is_entry_frame()) {
      EntryFrame e = fr.as_EntryFrame();
      if (e.is_first_frame()) {
        // we hit the end of the road, bail out
        out->send_error(JDWP_Error_INVALID_INDEX);
        return;
      }
      e.caller_is(fr);
    }

    JavaFrame jf = fr.as_JavaFrame();
    out->write_int(index + 1);  // one based, zero may mean something special
    LocationModifier::Fast location = LocationModifier::new_location(&jf);
    if (location.is_null()) {
      out->send_error(JDWP_Error_INVALID_INDEX);
      return;
    }
    location().write(out);
#ifdef AZZERT
    if (TraceDebugger) {
      UsingFastOops fast_oops;
      Method::Fast m = jf.method();
      Symbol::Fast method_name = m().name();
      InstanceClass::Fast holder = m().holder(&thread);
      tty->print_cr("    %ld: ", index); 
      JavaDebugger::print_class(&holder);
      tty->print(".");
      method_name().print_symbol_on(tty, true);
      tty->print_cr("");
    }
#endif
    jf.caller_is(fr);
  }
  out->send_packet();
}

// Return the number of frames for this thread

void ThreadReferenceImpl::thread_reference_frame_count(PacketInputStream *in, 
                           PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Thread::Fast thread;
  thread = in->read_thread_ref();

  if (JavaDebugger::is_valid_thread(&thread)) {
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("ThreadFrameCount: threadID=%lx", 
                    JavaDebugger::get_thread_id_by_ref(&thread));
    }
#endif
    if (!(thread().status() & THREAD_DBG_SUSPENDED)) {
#ifdef AZZERT
      if (TraceDebugger) { 
        tty->print_cr("    Thread not suspended\n");
      }
#endif
      out->send_error(JDWP_Error_THREAD_NOT_SUSPENDED);
      return;
    }
    if (thread().last_java_fp() == NULL ||
      thread().last_java_sp() == NULL) {
      out->write_int(0);    // no frames to report
      out->send_packet();
      return;
    }
    if (thread().status() & THREAD_DBG_SUSPENDED) {
      Frame fr(&thread);
      // first get the count of frames on the stack
      int frame_count = get_frame_count(fr);
      if (frame_count < 0) {
        out->send_error(JDWP_Error_INVALID_INDEX);
        return;
      } else {
        out->write_int(frame_count);
      }
#ifdef AZZERT
      if (TraceDebugger) { 
        tty->print_cr("    frame count is %ld\n", frame_count);
      }
#endif
    }
    out->send_packet();
  } else {
    out->send_error(JDWP_Error_INVALID_THREAD);
  }
}

int ThreadReferenceImpl::get_frame_count(Frame fr) {

  int frame_count = 0;

  while (true) {
    if (fr.is_entry_frame()) {
      EntryFrame e = fr.as_EntryFrame();
      if (e.is_first_frame()) {
        break;
      }
      e.caller_is(fr);
    } else if (fr.is_java_frame()) {
      JavaFrame jf = fr.as_JavaFrame();
#if !ENABLE_ROM_JAVA_DEBUGGER
      GUARANTEE(UseROM, "Sanity");
      Method::Raw m = jf.method();
      if (ROM::in_any_loaded_bundle(m.obj())) {
        break;
      }
#endif 
      frame_count++;
      jf.caller_is(fr);
    } else {
      return -1;
    }
  }
  return frame_count;
}

void *ThreadReferenceImpl::thread_reference_cmds[] = { 
  (void *)12
  ,(void *)ThreadReferenceImpl::thread_reference_name
  ,(void *)ThreadReferenceImpl::thread_reference_suspend
  ,(void *)ThreadReferenceImpl::thread_reference_resume
  ,(void *)ThreadReferenceImpl::thread_reference_status
  ,(void *)JavaDebugger::nop                            // ThreadReference_ThreadGroup
  ,(void *)ThreadReferenceImpl::thread_reference_frames
  ,(void *)ThreadReferenceImpl::thread_reference_frame_count
  ,(void *)JavaDebugger::nop                            // ownedMonitors
  ,(void *)JavaDebugger::nop                            // currentContendedMonitor
  ,(void *)JavaDebugger::nop                            // stop
  ,(void *)JavaDebugger::nop                            // interrupt
  ,(void *)ThreadReferenceImpl::thread_reference_suspend_count
};

#endif
