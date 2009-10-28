/*
 * @(#)stdlib.h	1.12 06/10/10
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

#ifndef _INCLUDED_PORTING_ANSI_STDLIB_H
#define _INCLUDED_PORTING_ANSI_STDLIB_H

#include "javavm/include/porting/ansi/stddef.h"

/*
 * ANSI <stdlib.h>
 *
 * Functions:
 *
 * void* malloc(size_t size);
 * void* calloc(size_t nelem, size_t elsize);
 * void* realloc(void* ptr, size_t size);
 * char* strdup(const char *s);
 * void  free(void* ptr);
 *
 * long  strtol(const char *str, char **endptr, int base);
 *
 * void  qsort(void* array, size_t numElements, size_t elementSize,
 *	       int (*)(const void *, const void *));
 */

#include "javavm/include/defs_md.h"
#ifdef CVM_HDR_ANSI_STDLIB_H
#include CVM_HDR_ANSI_STDLIB_H
#else
#include <stdlib.h>
#endif

#endif /* _INCLUDED_PORTING_ANSI_STDLIB_H */
