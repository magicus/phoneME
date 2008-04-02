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

/** \class ThreadReferenceImpl
    Handles support for commands from the debugger to suspend/resume
    threads in the VM.  Also handles commands to return stack frames.

*/
#if ENABLE_JAVA_DEBUGGER

class ThreadReferenceImpl {
public:
  static void suspend_specific_thread(Thread *, int task_id, bool is_event);
  static void suspend_all_threads(int task_id, bool is_event);
  static void resume_all_threads(int task_id);
  static void resume_specific_thread(Thread *, int task_id);
  static jint get_jdwp_thread_status(Thread *);
  static jint get_jdwp_thread_suspend_status(Thread *);
  static void *thread_reference_cmds[]; 

private:
  static void thread_reference_name(COMMAND_ARGS);
  static void thread_reference_suspend(COMMAND_ARGS);
  static void thread_reference_resume(COMMAND_ARGS);
  static void thread_reference_status(COMMAND_ARGS);
  static void thread_reference_frames(COMMAND_ARGS);
  static void thread_reference_frame_count(COMMAND_ARGS);
  static void thread_reference_suspend_count(COMMAND_ARGS);
  static int get_frame_count(Frame);
};
#endif
