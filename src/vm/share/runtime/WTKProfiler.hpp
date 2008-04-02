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

#if ENABLE_WTK_PROFILER

/** \class WTKProfiler
     This profiler collects exact execution time information.
*/
class WTKProfiler : public AllStatic {
private:
  static jlong _totalCycles;
  static jlong _vmStartTime;
  static jint  _dumpedProfiles;
public:
  // Not good to be public, but should be visible to optimize a little
  static jlong _lastCycles;

  // Initialize resources needed by the profiler
  static void initialize();

  // Activate the profiler
  static void engage();

  // Deactivate the profiler -- it may be activated again
  static void disengage();

  // Free all resources needed by the profiler
  //  (in MVM case, only ones specific for id)
  static void dispose(int id);

  // Called when current metod changes
  // information about current method can be read from thread stack
  static void record_method_transition(Thread*);

  // Called when current thread changes
  static void record_thread_transition();

  // Called when exception is being thrown
  static void record_exception(Thread*, Frame* new_current_frame);

  // Prints the collected profiling information
  static void print(Stream* out, int id);

  // suspend/resume profiler while waiting for external events
  static void suspend();
  static void resume();

  /// allocate thread-specific profiler data
  static ReturnOop allocate_thread_data(JVM_SINGLE_ARG_TRAPS);

  // saves profiler data on disk and returns unique integer 
  // for stored profile, also removes profiler's internal 
  // structure related to particulatar task_id (in MVM case)
  // in SVM case the only reasonable value for task_id is -1
  static int dump_and_clear_profile_data(int task_id);

};

#endif
