/*
 * jvmti hprof
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
 *
 */

/* ***********************************************************************
 *
 * The source file debug_malloc.c should be included with your sources.
 *
 * The object file debug_malloc.o should be included with your object files.
 *
 *   WARNING: Any memory allocattion from things like memalign(), valloc(),
 *            or any memory not coming from these macros (malloc, realloc,
 *            calloc, and strdup) will fail miserably.
 *
 * ***********************************************************************
 */

#ifndef _DEBUG_MALLOC_H
#define _DEBUG_MALLOC_H

#ifdef DEBUG

#include <stdlib.h>
#include <string.h>

/* The real functions behind the macro curtains. */

void           *debug_malloc(size_t, const char *, int);
void           *debug_realloc(void *, size_t, const char *, int);
void           *debug_calloc(size_t, size_t, const char *, int);
char           *debug_strdup(const char *, const char *, int);
void            debug_free(void *, const char *, int);

#endif

void            debug_malloc_verify(const char*, int);
#undef malloc_verify
#define malloc_verify()     debug_malloc_verify(__FILE__, __LINE__)

void            debug_malloc_police(const char*, int);
#undef malloc_police
#define malloc_police()     debug_malloc_police(__FILE__, __LINE__)

#endif
