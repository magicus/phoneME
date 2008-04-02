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
# include "incls/_VMImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

// Return a list of all known classes that are loaded into the
// System dictionary

void VMImpl::virtualmachine_all_classes(PacketInputStream *in, 
                           PacketOutputStream *out)
{
  (void)in;
  UsingFastOops fast_oops;
  
  JavaClass::Fast jc;
  jint i;
  jint count = 0;

#if ENABLE_ISOLATES
  TaskGCContext tmp(in->transport()->task_id());
#endif

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("VM All Classes");
  }
#endif

  jint num_classes = Universe::number_of_java_classes();
  for (i = 0; i < num_classes; i++) {
    jc = Universe::class_from_id(i);
    if (!jc().is_fake_class() && jc().is_instance_class()) {
      count++;
    }
  }

  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on classname being about 128 bytes long'
  out->write_int(count);
  for (i = 0; i < num_classes; i++) {
    jc = Universe::class_from_id(i);
    if (!jc().is_fake_class() && jc().is_instance_class()) {
      out->write_byte(JavaDebugger::get_jdwp_tagtype(&jc));
      out->write_class(&jc);
      out->write_class_name(&jc);
      out->write_int(JavaDebugger::get_jdwp_class_status(&jc));
    }
  }
  out->send_packet();
}

void VMImpl::virtualmachine_classes_by_signature(PacketInputStream *in, 
                           PacketOutputStream *out)
{
  UsingFastOops fast_oops;
  int length;
  JavaClass::Fast jc;
  Symbol::Fast class_name;
  int i;
  int array_depth = 0;
  int num_classes = 0;  
  Symbol::Fast tmpName;
  SETUP_ERROR_CHECKER_ARG;

  TypeArray::Fast ta = in->read_raw_string(length JVM_NO_CHECK);
  int src_pos = 0;
  // Either caught exception or KDP sent a null string
  if (ta.is_null()) {
    goto bailout;
  }
  while (ta().byte_at(src_pos) == '[') {
    array_depth++;
    src_pos++;
  }
  if (src_pos >= (array_depth == 0 ? (length - 2) : length)) {
    goto bailout;
  }

  if (ta().byte_at(src_pos) == 'L') {
    // object of some sort, skip the 'L' and don't copy trailing ';'
    tmpName = Universe::new_symbol(&ta,
                                   (utf8)(ta().base_address() + (src_pos + 1)),
                                   (length - src_pos - 2) JVM_NO_CHECK);
    if (tmpName.is_null()) {
      goto bailout;
    }
  } else {
    // MUST be a type_array_class of some sort
    int index;
    jbyte byte = ta().byte_at(src_pos);
    GUARANTEE(array_depth >=1, "Should be array");
    GUARANTEE(byte >= 'B' && byte <= 'Z', "Bogus type array");
    if ((byte >= 'B') && (byte <= 'Z') &&
        ((index = simple_type_symbol_table[byte - 'B']) != 0)) {
      index = Universe::bool_array_class_index + index - 1;
      // Really this is a TypeArrayClass...
      JavaClass* ta = (JavaClass*)(&persistent_handles[index]);
      jc = ta->obj();
      // Already at depth of 1 for array class so decrement depth
      array_depth--;
      while (array_depth--) {
        if (jc.is_null()) {
          // sent in a bogus signature, no such array class here
          goto bailout;
        }
        jc = jc().array_class();
      }
      if (jc.is_null()) {
        goto bailout;
      }
      out->write_int(1);
      out->write_byte(JavaDebugger::get_jdwp_tagtype(&jc));
      out->write_class(&jc);
      out->write_int(JavaDebugger::get_jdwp_class_status(&jc));
      out->send_packet();
      return;
    } else {
      goto bailout;
    }
  }
  {
#if ENABLE_ISOLATES
    TaskGCContext tmp(in->transport()->task_id());
#endif
    num_classes = Universe::number_of_java_classes();
    for (i = 0; i < num_classes; i++) {
      jc = Universe::class_from_id(i);
      if (!jc().is_fake_class()) {
        class_name = jc().name();
        if (tmpName().matches(&class_name)) {
          if (array_depth == 0) {
            out->write_int(1);
            out->write_byte(JavaDebugger::get_jdwp_tagtype(&jc));
            out->write_class(&jc);
            out->write_int(JavaDebugger::get_jdwp_class_status(&jc));
            out->send_packet();
            return;
          } else {
            UsingFastOops fastoops2;
            // Have the base element class in hand, follow the pointers
            // to the array class we need
            ObjArrayClass::Fast oa = jc().array_class();
            while (--array_depth) {
              if (oa.is_null()) {
                // sent in a bogus signature, no such array class here
                goto bailout;
              }
              oa = oa().array_class();
            }
            if (oa.is_null()) {
              goto bailout;
            }
            out->write_int(1);
            out->write_byte(JavaDebugger::get_jdwp_tagtype(&oa));
            out->write_class(&oa);
            out->write_int(JavaDebugger::get_jdwp_class_status(&oa));
            out->send_packet();
            return;
          }
        }
      }
    }
  }
 bailout:
  out->write_int(0);
  out->send_packet();
}

// Return a list of all threads

void VMImpl::virtualmachine_all_threads(PacketInputStream *in,
                                             PacketOutputStream *out)
{
  (void)in;
  UsingFastOops fast_oops;

  jint count=0;

  Thread::Fast thread = Universe::global_threadlist();
#if ENABLE_ISOLATES
  Transport* t = in->transport();
  int task_id = t->task_id();
#endif

  while (thread.not_null()) {
#if ENABLE_ISOLATES
      if (task_id != -1 && (thread().task_id() != task_id)) {
        thread = thread().global_next();
        continue;
      }
#endif
    if (!(thread().status() & THREAD_DEAD)) {
      count++;
    }
    // If this thread is "just born" since we are sending it to the
    // debugger, unmark it so we don't send a new_thread event as well
    thread().set_status(thread().status() & ~THREAD_JUST_BORN);
    thread = thread().global_next();
  }

#ifdef AZZERT
  if (TraceDebugger) {
    //    tty->print_cr("VM All Threads: xprt = 0x%x, count=%d",
    //                  in->transport()->obj(), count);
  }
#endif
  // len is number of threads * 4 + the count
  out->write_int(count);
  thread = Universe::global_threadlist();
  while (thread.not_null()) {
#if ENABLE_ISOLATES
      if (task_id != -1 && (thread().task_id() != task_id)) {
        thread = thread().global_next();
        continue;
      }
#endif
    if (!(thread().status() & (THREAD_DEAD | THREAD_JUST_BORN))) {
      out->write_thread(&thread);
#ifdef AZZERT
      if (TraceDebugger) {
        // ThreadObj::Raw tobj = thread().thread_obj();
        //  tty->print_cr("Thread: 0x%x",
        //                JavaDebugger::get_object_id_by_ref_nocreate(&tobj));
      }
#endif
    }
    thread = thread().global_next();
  }
  out->send_packet();
}

void VMImpl::virtualmachine_dispose(PacketInputStream *in, 
                                    PacketOutputStream* /*out*/)
{
  JavaDebugger::close_java_debugger(in->transport());
}

// Suspend all threads

void VMImpl::virtualmachine_suspend(PacketInputStream *in, 
                       PacketOutputStream *out)
{
  int task_id = in->transport()->task_id();

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("VM Suspend All");
  }
#endif
    ThreadReferenceImpl::suspend_all_threads(task_id, false);
    out->send_packet();  // no error, just send reply
}

// Resume the VM

void VMImpl::virtualmachine_resume(PacketInputStream *in,
                                   PacketOutputStream *out)
{
  UsingFastOops fast_oops;
  int task_id = in->transport()->task_id();

  Thread::Fast thread = Universe::global_threadlist();

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("VM Resume All");
  }
#endif

  //  CurrentThread = NIL;
  while (thread.not_null()) {
    if (JavaDebugger::is_valid_thread(&thread)) {
      ThreadReferenceImpl::resume_specific_thread(&thread, task_id);
    } else {
      out->send_error(JDWP_Error_INVALID_THREAD);
      return;
    }
    thread = thread().global_next();
  }

  out->send_packet();  // no error, just send reply
  //  JavaDebugger::allThreadsSuspended(false);
  JavaDebugger::set_loop_count(-1);
}

// Kill this VM

void VMImpl::virtualmachine_exit(PacketInputStream *in, 
                    PacketOutputStream *out)
{
  jint exit_code;
  exit_code = in->read_int();

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("VM Exit: Code = %d", exit_code);
  }
#endif

  //  VMEvent::set_notification(Dbg_EventKind_NONE);
  out->send_packet();  // we're closing down so send it now
  JavaDebugger::set_loop_count(-1);
  Transport t = in->transport();
  JavaDebugger::close_java_debugger(&t);
#if ENABLE_ISOLATES
  {
    SETUP_ERROR_CHECKER_ARG;
    UsingFastOops fast_oops;
    int task_id = in->transport()->task_id();
    if (task_id != -1) {
      Task::Fast task = Task::get_task(task_id);
      GUARANTEE(task.not_null(), "Java code cannot run without supporting task");
      // current thread will most likely get exception set, so don't check it
      // now.
      task().forward_stop(exit_code, Task::SELF_EXIT JVM_NO_CHECK);
    }
    // If we are getting terminated during initialization, need to set
    // exit code since this task will never run so it will never 
    // terminate normally (in thread_task_cleanup)
    JVM::set_exit_code(exit_code);
  }
#else
  JVM::stop(exit_code);
#endif
}

void VMImpl::virtualmachine_createstring(PacketInputStream *in,
                                         PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  int length;
  SETUP_ERROR_CHECKER_ARG;
  TypeArray::Fast ta = in->read_raw_string(length JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
      out->send_error(JDWP_Error_OUT_OF_MEMORY);
      return;
  }
  LiteralStream ls((char *)ta().base_address(), 0, length);
  String::Fast s = Universe::new_string(&ls JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
      out->send_error(JDWP_Error_OUT_OF_MEMORY);
      return;
  }
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print("VM Createstring: string - ");
    s().print_value_on(tty);
    tty->print_cr(", ObjectID %d", JavaDebugger::get_object_id_by_ref(&s));
  }
#endif
  out->write_object(&s);
  out->send_packet();
}

void *VMImpl::virtualmachine_cmds[] = { 
  (void *)16
  ,(void *)JavaDebugger::nop        // VirtualMachine_SendVersion
  ,(void *)VMImpl::virtualmachine_classes_by_signature
  ,(void *)VMImpl::virtualmachine_all_classes
  ,(void *)VMImpl::virtualmachine_all_threads
  ,(void *)JavaDebugger::nop        // VirtualMachine_TopLevelThreadGroups
  ,(void *)VMImpl::virtualmachine_dispose
  ,(void *)JavaDebugger::nop        // VirtualMachine_IDSizes
  ,(void *)VMImpl::virtualmachine_suspend
  ,(void *)VMImpl::virtualmachine_resume
  ,(void *)VMImpl::virtualmachine_exit
  ,(void *)VMImpl::virtualmachine_createstring
  ,(void *)JavaDebugger::nop        // VirtualMachine_Capabilities
  ,(void *)JavaDebugger::nop        // VirtualMachine_ClassPaths
  ,(void *)JavaDebugger::nop        // VirtualMachine_DisposeObjects
  ,(void *)JavaDebugger::nop        // holdEvents
  ,(void *)JavaDebugger::nop        // releaseEvents
};

#endif
