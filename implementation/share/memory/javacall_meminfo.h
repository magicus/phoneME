/*
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

#ifndef __JAVACALL_MEMINFO_H
#define __JAVACALL_MEMINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_NAME_LENGTH 4096

typedef struct _mem_alloc_info {
    int size;
	void* buffer;
    char fileName[MAX_FILE_NAME_LENGTH];
	unsigned int line;
	struct _malloc_info* next;
	struct _malloc_info* prev;
} mem_alloc_info;

typedef struct _meminfo_stat{
	int maxMemoryUsage;
	int currentMemeoryUsage;
}meminfo_stat;


void* javacall_meminfo_malloc(unsigned int size, char* fileName, unsigned int line);
void  javacall_meminfo_free(void* ptr, char* fileName, unsigned int line);
void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line);
void* javacall_meminfo_calloc(unsigned int numberOfElements, unsigned int elementSize, char* fileName, unsigned int line);
void* javacall_meminfo_strdup(const char* str, char* fileName, unsigned int line);

#ifdef __cplusplus
}
#endif

#endif /* __JAVACALL_MEMINFO_H*/
