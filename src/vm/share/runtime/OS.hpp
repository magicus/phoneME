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

/** \class Os
 * OS.hpp: Operating system porting interface.
 *
 * Class "Os" defines the operating system (OS) porting
 * interface of the VM.  Each new OS port of
 * the system should provide an implementation of this
 * interface in the port-specific
 * "/src/vm/os/<os_name>/OS_<os_name>.cpp" file.
 *
 * Logically, the porting interface has been divided
 * into a number of subareas that must be supported
 * by each new OS port:
 *
 * (1) threading and thread synchronization operations
 * (3) miscellaneous OS-specific operations
 *
 * For more information on the OS porting interface,
 * refer to the Porting Guide.
 */

class Os {
 private:
  static jlong _java_time_millis_offset;
 public:
  // Returns the same value as java.lang.System.currentTimeMillis() would:
  // i.e., the difference, measured in milliseconds, between the current
  // time and midnight, January 1, 1970 UTC.
  static jlong java_time_millis();

#if USE_HIGH_RESOLUTION_TIMER
  // Returns the current value of a system-dependent high-resolution counter.
  // Ideally, this counter should be light-weight and should have higher
  // resolution than java_time_millis(). Also, it does not need to start
  // at January 1, 1970. 
  static jlong elapsed_counter();

  // The frequency of elapsed_counter(), in the number of periods per second. 
  // E.g., for a 1 microsecond resolution counter, this function should
  // return 1,000,000.
  static jlong elapsed_frequency();
#endif

  // Enable periodic calls to real_time_tick(). This is call at
  // VM shut-down.
  static bool start_ticks();
  // Permanently disable calls to real_time_tick(). This is call at
  // VM shut-down.
  static void stop_ticks();

  // Temporary turns off calls to real_time_tick(). This is called when
  // the VM is about to sleep (when there's no Java thread to execute)
  static void suspend_ticks();
  // Reverse the effect of suspend_ticks(). This is called when the VM
  // wakes up and continues executing Java threads.
  static void resume_ticks();

  static void sleep(jlong ms);

  // Returns a handle to the terminal for this OS
  static class Stream * get_tty();

  // Initializes the os component (invoked from Universe::genesis)
  static void initialize();
  static void dispose();

#if ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_JVMPI_PROFILE
  static void* loadLibrary(const char* libName);
  static void* getSymbol(void* handle, const char* name);
#endif

  enum {
    SuspendProfiler,
    ResumeProfiler
  } ProfilerControlCode;

#if SUPPORTS_PROFILER_CONTROL
  // These APIs are used to suspend and resume a native profiler. It's
  // mainly used by the CompilerTest class to avoid profiling sections
  // of the VM that are not involved in compilation.
  static void suspend_profiler();
  static void resume_profiler();
#else
  static void suspend_profiler() {}
  static void resume_profiler()  {}
#endif

  // Start the timer for suspending compilation that takes a long time.
  static void start_compiler_timer();

  // Returns true iff the compiler timer has expired (which means
  // the current compilation should be expired.
  //
  // See OS.cpp for a sample implementation of start_compiler_timer()
  // and check_compiler_timer().
  static bool check_compiler_timer();
};
