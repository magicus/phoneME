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

#include <pcsl_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * If this callback function is set with set_pcsl_memory_allocationerror_callback(), then
 * it is invoked whenever pcsl_mem_malloc fails to allocate. After this callback returns,
 * allocation routine is invoked again
 */   
void (* pcsl_memory_allocation_failed_callback)() = NULL;

void set_pcsl_memory_allocationerror_callback(void (*callback_pointer)()) {
    pcsl_memory_allocation_failed_callback = callback_pointer;
}

/** 
 * Allocates memory of the given size from the private PCSL memory
 * pool.
 */
#ifdef PCSL_DEBUG

void*
pcsl_mem_malloc(unsigned int size, char* filename, int lineno) {
    void * ptr =  pcsl_mem_malloc_impl(size, filename, lineno);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_malloc_impl(size, filename, lineno);
    }
    return ptr;
}

#else 
 
void * pcsl_mem_malloc(unsigned int size) {
    void * ptr =  pcsl_mem_malloc_impl(size);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_malloc_impl(size);
    }
    return ptr;
}

#endif

/** 
 * Allocates and clears the given number of elements of the given size
 * from the private PCSL memory pool.
 */
#ifdef PCSL_DEBUG

void*
pcsl_mem_calloc(unsigned int nelem, unsigned int elsize, 
                     char* filename, int lineno) {  
    void * ptr =  pcsl_mem_calloc_impl(nelem, elsize, filename, lineno);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_calloc_impl(nelem, elsize, filename, lineno);
    }
    return ptr;
}
 
#else
 
void * pcsl_mem_calloc(unsigned int nelem, unsigned int elsize) {  
    void * ptr =  pcsl_mem_calloc_impl(nelem, elsize);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_calloc_impl(nelem, elsize);
    }
    return ptr;
}

#endif

/**
 * Re-allocates memory at the given pointer location in the private
 * PCSL memory pool (or null for new memory) so that it is the given
 * size.
 */

#ifdef PCSL_DEBUG

void*
pcsl_mem_realloc(void* ptr, unsigned int size, char* filename, int lineno) {
    void * retptr =  pcsl_mem_realloc_impl(ptr, size, fileanme, lineno);
    if (retptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        retptr = pcsl_mem_realloc_impl(ptr, size, fileanme, lineno);
    }
    return retptr;
}
 
#else
 
void * pcsl_mem_realloc(void* ptr, unsigned int size) {
    void * retptr =  pcsl_mem_realloc_impl(ptr, size);
    if (retptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        retptr = pcsl_mem_realloc_impl(ptr, size);
    }
    return retptr;
}

#endif

/**
 * Duplicates the given string after allocating the memory for it.
 */
#ifdef PCSL_DEBUG

char*
pcsl_mem_strdup(const char *s1, char* filename, int lineno) {
    char * ptr =  pcsl_mem_strdup_impl(s1, filename, lineno);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_strdup_impl(s1);
    }
    return ptr;
} 

#else 

char * pcsl_mem_strdup(const char *s1) {
    char * ptr =  pcsl_mem_strdup_impl(s1);
    if (ptr == NULL && pcsl_memory_allocation_failed_callback != NULL) {
        pcsl_memory_allocation_failed_callback();
        ptr = pcsl_mem_strdup_impl(s1);
    }
    return ptr;
}    

#endif

#ifdef __cplusplus
}
#endif
