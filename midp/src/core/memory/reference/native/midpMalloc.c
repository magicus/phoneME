/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * MIDP native public memory functions.
 *
 * With these functions, we have the ability to manage our own
 * memory pool. This avoids memory fragmentation in the overall
 * system. However, it does not address the potential problem
 * of memory fragmentation within the MIDP application itself.
 *
 * The memory pool is one contiguous chunk of memory split up
 * into various free or allocated blocks. All blocks have the
 * same layout.
 *
 * <table border=1>
 * <tr><th scope=col>Contents of the Memory Block</th></tr>
 * <tr><td>magic (value of 0xCAFE)</td></tr>
 * <tr><td>free (value of 0 or 1)</td></tr>
 * <tr><td>reserved/<sup>[*]</sup>guardSize</td></tr>
 * <tr><td>size</td></tr>
 * <tr><td><sup>[*]</sup>filename</td></tr>
 * <tr><td><sup>[*]</sup>lineno</td></tr>
 * <tr><td>1 .. size</td></tr>
 * <tr><td><sup>[*]</sup>1 .. guardSize</td></tr>
 * </table>
 *
 * <p>Items that have the prefix <sup>[*]</sup> are only enabled if memory
 * tracing is enabled.
 *
 * @warning This code is not thread safe.
 *
 * @note  In this midpMalloc implementation we wrap the pcslMemory functions
 * defined in pcsl_memory.h.
 */

#include <stdio.h>
#include <string.h>
//#include <malloc.h>
#include <midpMalloc.h>
#include <midp_logging.h>
#include <pcsl_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ENABLE_MIDP_MALLOC

/**
 * FUNCTION:      midpInitializeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the MIDP memory pool
 *                 NOTE: This must only be called once
 * INTERFACE:
 *   parameters:  size   Size of memory pool to use; if size is '-1',
 *                        the default memory pool size will be used
 *   returns:     0 on success; != 0 on failure
 *
 */
int
midpInitializeMemory(int size) {
    int tmp = size;
    tmp++; /* avoid compile warning for unused variable */
    return pcsl_mem_initialize(NULL, size);
}


/**
 * FUNCTION:      midpFinalizeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the MIDP memory pool
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *
 */
void
midpFinalizeMemory() {

    pcsl_mem_finalize();
}

/**
 * FUNCTION:      midpMallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occurred
 *                lineno     Line number where allocation occurred
 *   returns:     pointer to the newly allocated memory
 *
 */
void*
midpMallocImpl(unsigned int size, char* filename, int lineno) {
    (void) filename;
    (void) lineno;
    return pcsl_mem_malloc(size);
}


/**
 * FUNCTION:      midpCallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private MIDP memory pool,
 *                 memory contents are cleared
 * INTERFACE:
 *   parameters:  nelem      Number of elements to allocate
 *                elsize     Size of one element
 *                filename   Filename where allocation occurred
 *                lineno     Line number where allocation occurred
 *   returns:     pointer to the newly allocated and cleared memory
 *
 */
void*
midpCallocImpl(unsigned int nelem, unsigned int elsize,
               char* filename, int lineno) {
    (void) filename;
    (void) lineno;
    return pcsl_mem_calloc(nelem, elsize);
}

/**
 * FUNCTION:      midpReallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Re-allocate memory from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  ptr        Original memory pointer
 *                size       New size
 *                filename   Filename where allocation occurred
 *                lineno     Line number where allocation occurred
 *   returns:     pointer to the re-allocated memory
 *
 */
void*
midpReallocImpl(void* ptr, unsigned int size, char* filename, int lineno) {
    (void) filename;
    (void) lineno;
    return pcsl_mem_realloc(ptr, size);
}

#ifdef UNDER_CE
char * strdup(const char *s) {
    char *result = (char*)malloc(strlen(s)+1);
    if (result) {
        strcpy(result, s);
    }
    return result;
}
#endif

/**
 * FUNCTION:      midpStrdupImpl()
 * TYPE:          public operation
 * OVERVIEW:      Duplicate the given string
 * INTERFACE:
 *   parameters:  s1         String to duplicate
 *                filename   Filename where allocation occurred
 *                lineno     Line number where allocation occurred
 *   returns:     pointer to the duplicate string
 *
 */
char*
midpStrdupImpl(const char *s1, char* filename, int lineno) {
    (void) filename;
    (void) lineno;
    return (char*)pcsl_mem_strdup(s1);
}


/**
 * FUNCTION:      midpFreeImpl()
 * TYPE:          public operation
 * OVERVIEW:      Free memory allocated from the private MIDP memory pool
 * INTERFACE:
 *   parameters:  ptr        Pointer to allocated memory
 *                filename   Filename where allocation occurred
 *                lineno     Line number where allocation occurred
 *   returns:     <nothing>
 *
 */
void
midpFreeImpl(void *ptr, char *filename, int lineno) {
    (void) filename;
    (void) lineno;
    pcsl_mem_free(ptr);
}

/**
 *
 * FUNCTION:      midpGetTotalHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the total amount of available heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The total amount of available heap
 *
 */
int
midpGetTotalHeap() {

    return pcsl_mem_get_total_heap();
}


/**
 * FUNCTION:      midpGetFreeHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the current amount of unused heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The current amount of unused heap
 *
 */
int
midpGetFreeHeap() {

    return pcsl_mem_get_free_heap();
}

/* Set countMemoryLeaksOnly = 0 in order to get more verbose information */
int midpMallocDumpMemory(int countMemoryLeaksOnly) {
    int tmp = countMemoryLeaksOnly;
    tmp++; /* avoid compile warning for unused variable */
    return pcsl_mem_malloc_dump(countMemoryLeaksOnly);
}

#endif  /* ENABLE_MIDP_MALLOC*/

#ifdef __cplusplus
}
#endif
