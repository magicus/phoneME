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
 * BuildFlags_win32.hpp: compile-time
 * configuration options for the Win32 platform.
 */

// The Win32 platform uses UNICODE for JvmPathChar. The main purpose is to
// test that this VM shared filename handling code is UNICODE safe.
#ifndef USE_UNICODE_FOR_FILENAMES
#define USE_UNICODE_FOR_FILENAMES 1
#endif

// We use BSDSocket.cpp to implement sockets on this platform
#define USE_BSD_SOCKET 1

// The Win32 port supports TIMER_THREAD but not TIMER_INTERRUPT
#define SUPPORTS_TIMER_THREAD        1
#define SUPPORTS_TIMER_INTERRUPT     0

// The Win32 port support adjustable memory chunks for
// implementing the Java heap, but uses the default heap size adjustment
// policy implemented in src/vm/share/runtime/OsMemory.cpp
#define SUPPORTS_ADJUSTABLE_MEMORY_CHUNK 1
#define SUPPORTS_CUSTOM_HEAP_ADJUSTMENT  0
