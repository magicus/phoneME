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

/*
 * OS_win32.cpp: Win32 implementation of the VM
 *               operating system porting interface
 *
 * This file defines the Win32-specific implementation
 * of the OS porting interface (class Os).  Refer to file
 * "/src/vm/share/runtime/OS.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS_win32.cpp.incl"

static HANDLE           main_process = NULL;
static HANDLE           ticker_thread = NULL;
static bool             ticker_stopping = false;
static bool             ticker_running = false;
static int              sock_initialized = 0;

static bool             _has_offset = false;
static jlong            _offset     = 0;
static bool             _compiler_timer_has_ticked = false;
static jlong            _compiler_timer_start;

jlong offset() {
  if (!_has_offset) {
    SYSTEMTIME java_origin;
    java_origin.wYear          = 1970;
    java_origin.wMonth         = 1;
    java_origin.wDayOfWeek     = 0; // ignored
    java_origin.wDay           = 1;
    java_origin.wHour          = 0;
    java_origin.wMinute        = 0;
    java_origin.wSecond        = 0;
    java_origin.wMilliseconds  = 0;
    FILETIME jot;
    if (!SystemTimeToFileTime(&java_origin, &jot)) {
      // windows_error()
    }
    _offset = jlong_from_msw_lsw(jot.dwHighDateTime, jot.dwLowDateTime);
    _has_offset = true;
  }
  return _offset;
}
 
#if ENABLE_DYNAMIC_NATIVE_METHODS
void* Os::loadLibrary(const char* libName) {
  return 0; //Library loading not supported for this OS
}
void* Os::getSymbol(void* handle, const char* name) {
  return 0; //Library loading not supported for this OS
}
#endif

jlong Os::java_time_millis() {
  FILETIME wt;
  GetSystemTimeAsFileTime(&wt);

  // Convert to Java time.
  jlong a = jlong_from_msw_lsw(wt.dwHighDateTime, wt.dwLowDateTime);
  return (a - offset()) / 10000;
}

void Os::sleep(jlong ms) {
  // win32 Sleep takes 32-bit unsigned argument
  // Should really do loop here if sleeping more than 49 days
  DWORD duration = (ms > (jlong) INFINITE) ? INFINITE : (DWORD) ms;
  Sleep(duration);
}

DWORD WINAPI TickerMain(LPVOID lpvParam) {
  while (!ticker_stopping) {
    ::Sleep(TickInterval);
    if (ticker_running) {
      real_time_tick(TickInterval);
      _compiler_timer_has_ticked = true;
#if ENABLE_REMOTE_TRACER
      if (RemoteTracePort > 0 && RemoteTracer::tick()) {
        RemoteTracer::send_snapshot();
      }
#endif
    } 
  }

  CloseHandle(ticker_thread);
  ticker_thread = NULL;
  return 0;
}

bool Os::start_ticks() {
  if (!EnableTicks || Deterministic || ticker_thread != NULL) {
    return true;
  }

  DWORD ticker_id;
  ticker_stopping = false;
  ticker_running = true;
  ticker_thread = CreateThread(NULL, 0, &TickerMain, 0, 0, &ticker_id);
  if (ticker_thread != NULL) {
    if (SetThreadPriority(ticker_thread, THREAD_PRIORITY_HIGHEST) != 0) {
      return true;
    }
  }
  return false;
}

void Os::stop_ticks() {
  while (ticker_thread != NULL) {
    ticker_stopping = true;
    Os::sleep(10);
  }
}

void Os::suspend_ticks() {
  ticker_running = false;
}

void Os::resume_ticks() {
  ticker_running = true;
}

/*
 * This is an example implementation of compiler timer. We try to base
 * the compiler timer on real_time_tick, as much as possible. This
 * way, we can avoid the overhead of Os::java_time_millis(), which
 * might be significant on actual devices.
 *
 * On an actual device, if MaxCompilationTime is a fixed value, it may
 * be better to use a dedicated OS timer resource to implement the
 * compiler timer. Alternatively, you can make TickInterval a multiple
 * of MaxCompilationTime, so that you can use the same OS timer
 * resource to serve both real_time_tick and the compiler timer.
 */

void Os::start_compiler_timer() {
  if (MaxCompilationTime == TickInterval) {
    // Note: this tend to make the average compilation period to be
    // 0.5 * MaxCompilationTime.
    _compiler_timer_start = (jlong)0;
    _compiler_timer_has_ticked = false;
  } else {
    _compiler_timer_start = Os::java_time_millis();
  }
}

/*
 * Returns true iff the current compilation has taken too long and
 * should be suspended and resumed later.
 */
bool Os::check_compiler_timer() {
  if (_compiler_timer_start == (jlong)0) {
    return _compiler_timer_has_ticked;
  } else {
    jint elapsed_ms = (jint)(Os::java_time_millis() - _compiler_timer_start);
    return (elapsed_ms >= MaxCompilationTime);
  }
}

void Os::initialize() {
  main_process = GetCurrentProcess();
  OsMisc_set_process(main_process);
}

void Os::dispose() {
  CloseHandle(main_process);
}

#if USE_HIGH_RESOLUTION_TIMER
static bool  _has_performance_frequency = false;
static jlong _performance_frequency     = 0;

jlong as_jlong(LARGE_INTEGER x) {
  return jlong_from_msw_lsw(x.HighPart, x.LowPart);
}

jlong Os::elapsed_counter() {
  LARGE_INTEGER count;
  count.HighPart = count.LowPart = 0;
  QueryPerformanceCounter(&count);
  return as_jlong(count);
}

jlong Os::elapsed_frequency() {
  if (!_has_performance_frequency) {
    LARGE_INTEGER freq;
    freq.HighPart = freq.LowPart = 0;
    QueryPerformanceFrequency(&freq);
    _performance_frequency = as_jlong(freq);
    _has_performance_frequency = true;
  }
  return _performance_frequency;
}

#endif // USE_HIGH_RESOLUTION_TIMER
