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
 * OS_generic.cpp: generic implementation of the VM
 *               operating system porting interface
 *
 * This file defines the generic-specific implementation
 * of the OS porting interface (class Os). Refer to file
 * "/src/vm/share/runtime/OS.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS_generic.cpp.incl"

static bool             ticker_stopping = false;
static bool             ticker_running = false;
static int              sock_initialized = 0;

static bool  _has_offset = false;
static jlong _offset     = 0;

void ads_panic() {
  /* Add this empty method just for building generic_arm smoothly */ 
  return;
}

#if ENABLE_DYNAMIC_NATIVE_METHODS
void* Os::loadLibrary(const char* libName) {
  return 0; //Library loading not supported for this OS
}
void* Os::getSymbol(void* handle, const char* name) {
  return 0; //Library loading not supported for this OS
}
#endif

jlong offset() {
  /* Calculate the OS dependent offset time, if no offset exist, 
   * this function is not necessary */
  _offset = 0;
  return _offset;
}

jlong Os::java_time_millis() {
  /*
   * Get the current system time, unit: millisecond, count time from
   * Jan. 1st, 1970. Need reduce the OS dependent offset time value, if
   * the offset exist
   */
  return 0;
}

void Os::sleep(jlong ms) {
  /* let the current process sleep for ms seconds */
}


bool Os::start_ticks() {
  /* 
   * Enable periodic calls to real_time_tick().
   * This is called at VM startup.
   */
  return true;
}

void Os::stop_ticks() {
  /*
   * Permanently disable calls to real_time_tick().
   * This is called at VM shut-down.
   */
  return;
}

void Os::suspend_ticks() {
  /*
   * Temporarily turn off calls to real_time_tick().
   * This is called when the VM is about to sleep
   * (when there's no Java thread to execute)
   */
  return;
}

void Os::resume_ticks() {
  /*
   * Reverse the effect of suspend_ticks().
   * This is called when the VM
   * wakes up and continues executing Java threads.
   */
  return;
}

void Os::start_compiler_timer() {
  /*
   * Start a timer to check if the compiler has spent too much time and
   * needs to be suspended. See ../ads/OS_ads.cpp for an example.
   */
  return;
}

bool Os::check_compiler_timer() {
  /*
   * Returns true iff the current compilation has taken too long and
   * should be suspended and resumed later.
   */
  return false;
}

void Os::initialize() {
  /*
   * This function is used to initialize the OS structure. 
   * This is where timers and threads get started for the first
   * real_time_tick event, and where signal handlers and other I/O
   * initialization should occur.
   */
  return;
}

void Os::dispose() {
  /*
   * This method needs to correctly clean-up
   * all threads and other OS related activity to allow
   * for a clean and complete restart.  This should undo
   * all the work that initialize does.
   */
  return;
}

#ifndef PRODUCT
static bool  _has_performance_frequency = false;
static jlong _performance_frequency     = 0;


jlong Os::elapsed_counter() {
  /*
   * Retrieve the current time in high-resolution.
   * The resolution is decided by elapsed_frequency() method
   */
  return 0;
}

jlong Os::elapsed_frequency() {
  /*
   * Retrieve the system-support highest time resolution,
   * The return value is equal to  1s / the small unit of system-support unit
   * For example, if system support millisecond, the return valuse is 1000
   */
  return 0;
}

#endif // PRODUCT
