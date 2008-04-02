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
 * OS_wince.cpp: PocketPC implementation of the VM
 *               operating system porting interface
 *
 * This file defines the PocketPC-specific implementation
 * of the OS porting interface (class Os).  Refer to file
 * "/src/vm/share/runtime/OS.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS_wince.cpp.incl"

#undef FIELD_OFFSET
#include <windows.h>

static HANDLE           main_process = NULL;
static HANDLE           ticker_thread = NULL;
static bool             ticker_stopping = false;
static bool             ticker_running = false;
static jlong            _compiler_timer_start;

jlong Os::_java_time_millis_offset = 0;

#if ENABLE_DYNAMIC_NATIVE_METHODS
void* Os::loadLibrary(const char* libName) {
  return 0; //Library loading not supported for this OS
}
void* Os::getSymbol(void* handle, const char* name) {
  return 0; //Library loading not supported for this OS
}
#endif

void Os::dispose() {
#if !defined(ARM) || _WIN32_WCE < 400
  // On Microsoft Device Emulator (8.0.50215.35) with PocketPC 2003,
  // doing this would cause the OS to spin after the VM exits.
  CloseHandle(main_process);
#endif
}

static bool  _has_offset = false;
static jlong _offset     = 0;

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
      SHOULD_NOT_REACH_HERE();
      // windows_error()
    }
    _offset = jlong_from_msw_lsw(jot.dwHighDateTime, jot.dwLowDateTime);
    _has_offset = true;
  }
  return _offset;
}

jlong Os::java_time_millis() {
  jlong since_start = GetTickCount();
  return since_start + _java_time_millis_offset;
}

void Os::sleep(jlong ms) {
  // win32 Sleep takes 32-bit unsigned argument
  // Should really do loop here if sleeping more than 49 days
  DWORD duration = (ms > (jlong) INFINITE) ? INFINITE : (DWORD) ms;
  Sleep(duration);
}

DWORD WINAPI TickerMain(LPVOID lpvParam) {
  const int delay_interval = 40; // Delay 40 ms

  while (!ticker_stopping) {
    ::Sleep(delay_interval);
    if (ticker_running) {
      real_time_tick(delay_interval);
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

bool Os::start_ticks() {
  if (!EnableTicks || Deterministic || ticker_thread != NULL) {
    return true;
  }

  DWORD ticker_id;
  ticker_running = true;
  ticker_stopping = false;
  ticker_thread = CreateThread(NULL, 0, &TickerMain, 0, 0, &ticker_id);
  if (ticker_thread != NULL) {
    if (SetThreadPriority(ticker_thread, THREAD_PRIORITY_TIME_CRITICAL) != 0) {
      if (CeGetThreadQuantum(ticker_thread) != MAXDWORD) {
        return true;
      }
    }
  }
  return false;
}

void Os::start_compiler_timer() {
  // This is NOT an efficient implementaion. See ../ads/OS_ads.cpp for
  // a better one.
  _compiler_timer_start = Os::java_time_millis();
}

/*
 * Returns true iff the current compilation has taken too long and
 * should be suspended and resumed later.
 */
bool Os::check_compiler_timer() {
  jint elapsed_ms = (jint)(Os::java_time_millis() - _compiler_timer_start);
  return (elapsed_ms >= MaxCompilationTime);
}

void Os::initialize() {
  // initialize the timing
  SYSTEMTIME st;
  // This structure is a 64-bit value representing the number of
  // 100-nanosecond intervals since January 1, 1601
  FILETIME wt;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &wt);
  jlong a = jlong_from_msw_lsw(wt.dwHighDateTime, wt.dwLowDateTime);
  jlong millis = (a - offset()) / 10000;

  // the resolution of millis result is in seconds only;
  // increase the resolution by using the millis

  jlong millis_since_start = GetTickCount(); // milliseconds since WinCE was started
  _java_time_millis_offset = millis - millis_since_start;

  // Initialize main_process and main_thread
  main_process = GetCurrentProcess();  // Remember main_process is a pseudo handle

  OsMisc_set_process(main_process);
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
