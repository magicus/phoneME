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
 * OsMemory_generic.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_OsMemory_generic.cpp.incl"


#include "../utilities/JVM_Malloc.hpp"


#ifdef __cplusplus
extern "C" {
#endif

void *OsMemory_allocate(size_t size) {
	/* 
	 * Allocates memory blocks in memory pool (heap)
	 * size: bytes to be allocated 
	 * malloc returns a void pointer to the allocated space.  
	 * if there is insufficient memory available, it returns NULL (0)
	 */
  return JVM_Malloc(size);
}

void OsMemory_free(void *p) {
  /* Deallocates or frees a memory block (pointer *p ) */
  JVM_Free(p);
}

#ifdef __cplusplus
}
#endif
