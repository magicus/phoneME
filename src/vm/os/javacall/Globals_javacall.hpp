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
 * Globals_javacall.hpp: Command line switches and compile-time
 * configuration options for the Generic platform.
 */

// Enable the following flag if you want to test the UNICODE
// FilePath handling under Generic
// #define USE_UNICODE_FOR_FILENAMES 1

// We don't use BSDSocket.cpp to implement sockets on this platform
#define USE_BSD_SOCKET 0

// The Generic port support TIMER_THREAD but not TIMER_INTERRUPT
#define SUPPORTS_TIMER_THREAD        1
#define SUPPORTS_TIMER_INTERRUPT     1

// The Generic port does not support adjustable memory chunks for
// implementing the Java heap.
#define SUPPORTS_ADJUSTABLE_MEMORY_CHUNK 0


#define MAX_METHOD_TO_COMPILE 6000


#define PLATFORM_RUNTIME_FLAGS(develop, product)                            \
  product(int, TickInterval, 30,                                            \
          "Milliseconds between each timer tick")                           \
  product(int, CompilerTimerInterval, 1,                                    \
          "Milliseconds for compiler to run")                  \
  product(int, ExecutionLoops, 1,                                           \
          "the number of times we run the VM (for measuring start-up time)")\
  develop(bool, PrintStackTraceOnCrash, true,                               \
          "print the stack of all Java threads when crashing")              \
  develop(bool, ExitOnCrash, false,                                         \
          "when crashing, exit with error code 99 and don't call debugger") \
  develop(bool, CatchExceptions, true,                                      \
          "when crashing, exit with error code 99 and don't call debugger")

