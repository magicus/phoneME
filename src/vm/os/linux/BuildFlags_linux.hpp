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
 * BuildFlags_linux.hpp: compile-time configuration options for the 
 * Linux platform.
 */

// We use BSDSocket.cpp to implement sockets on this platform
#define USE_BSD_SOCKET 1

// Override with -DSUPPORTS_TIMER_THREAD=<value> in your gcc command-line.
#ifndef SUPPORTS_TIMER_THREAD
#define SUPPORTS_TIMER_THREAD 1
#endif

// All Linux builds usual support timer interrupts. To override,
// pass -DSUPPORTS_TIMER_INTERRUPT=0 in your gcc command-line.
#ifndef SUPPORTS_TIMER_INTERRUPT 
#define SUPPORTS_TIMER_INTERRUPT 1
#endif

// The Linux port support adjustable memory chunks for
// implementing the Java heap, but uses the default heap size adjustment
// policy implemented in src/vm/share/runtime/OsMemory.cpp

#ifndef SUPPORTS_ADJUSTABLE_MEMORY_CHUNK
#define SUPPORTS_ADJUSTABLE_MEMORY_CHUNK 1
#endif

// In the default the VM Linux port, we don't support custom heap adjustment.
#define SUPPORTS_CUSTOM_HEAP_ADJUSTMENT  0

// Linux port supports mmap-like interface for mapping files into memory
// Override with -DSUPPORTS_MEMORY_MAPPED_FILES=<value> in your gcc 
// command-line.
#ifndef SUPPORTS_MEMORY_MAPPED_FILES
#define SUPPORTS_MEMORY_MAPPED_FILES 1
#endif
