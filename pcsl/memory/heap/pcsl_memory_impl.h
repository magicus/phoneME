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

#ifndef _PCSL_MEMORY_IMPL_H_
#define _PCSL_MEMORY_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a PCSL memory pool.
 * <p><b>NOTE:</b> This function must only be called once.
 *
 * @param size size, in bytes, of the memory pool; if size is -1, the
 *	  default memory pool size will be used
 *
 * @return 0 if the function returns successfully; a non-zero value
 * otherwise
 */
extern int pcsl_mem_initialize_impl0(void *, int);

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
extern void pcsl_mem_finalize_impl0();

/**
 * Gets the maximum amount of heap space, in bytes, available to the system
 * for allocation. This value is constant throughout the lifetime of the
 * process.
 *
 * @return the maximum number of bytes available on the heap, or -1 if the
 * information is not available
 */
extern int   pcsl_mem_get_total_heap_impl0();

/**
 * Gets the current amount of unused heap space, in bytes. This value
 * changes with every <tt>pcslMalloc</tt> and <tt>pcslFree</tt>
 * function call.
 *
 * @return the number of bytes of heap space that are currently unused, or -1
 * if the information is not available
 */
extern int   pcsl_mem_get_free_heap_impl0();

/**
 * Displays the current state of the memory sub-system.
 *
 * @param countMemoryLeaksOnly amount of data to gather: if is non-zero,
 *	  display allocated blocks of memory; otherwise display both allocated
 *	  and free blocks
 *
 * @return the number of allocated blocks, or -1 if there was an error
 */
int pcsl_mem_malloc_dump_impl0(int countMemoryLeaksOnly);

#define pcsl_mem_initialize_impl(x, y) pcsl_mem_initialize_impl0((x), (y))
#define pcsl_mem_finalize_impl() pcsl_mem_finalize_impl0()
#define pcsl_mem_get_total_heap_impl()  pcsl_mem_get_total_heap_impl0()
#define pcsl_mem_get_free_heap_impl()  pcsl_mem_get_free_heap_impl0()
#define pcsl_mem_malloc_dump_impl(x)	 pcsl_mem_malloc_dump_impl0((x))

#ifdef PCSL_DEBUG

/**
 * Allocates memory from the private PCSL memory pool.
 * 
 * @param size number of bytes to allocate
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the newly allocated memory, or NULL if the system cannot
 * fulfil the allocation request
 */
extern void* pcsl_mem_malloc_impl0(unsigned int, const char*, int);

/**
 * Allocates memory from the private PCSL memory pool, and clears the
 * memory.
 *
 * @param nelem number of elements to allocate
 * @param elsize size of one element
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the newly allocated and cleared memory, or NULL if the
 * system cannot fulfil the allocation request
 */
extern void* pcsl_mem_calloc_impl0(unsigned int, unsigned int, const char*, int);

/**
 * Re-allocates memory from the private PCSL memory pool.
 * 
 * @param ptr original memory pointer, or null if this function should
 *        act like a call to pcsl_malloc_impl
 * @param size new amount of memory needed, in bytes, or 0 to have
 *        this function act like a call to pcslFreeImpl
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the re-allocated memory, or NULL if the system cannot
 * fulfil the allocation request
 */
extern void* pcsl_mem_realloc_impl0(void*, unsigned int, const char*, int);

/**
 * Duplicates the given string after allocating the memory for it.
 *
 * @param s1 string to duplicate
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the duplicate string, or NULL if the system cannot
 * fulfil the allocation request
 */
extern char* pcsl_mem_strdup_impl0(const char*, const char*, int);

/**
 * Frees memory allocated from the private PCSL memory pool
 *
 * @param ptr pointer to the allocated memory
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 */
extern void  pcsl_mem_free_impl0(void*, const char*, int);

/**
 * Allocates the given number of bytes from the private PCSL memory
 * pool.
 */
#define pcsl_mem_malloc_impl(x)     pcsl_mem_malloc_impl0((x), __FILE__, __LINE__)

/**
 * Allocates and clears the given number of elements of the given size
 * from the private PCSL memory pool.
 */
#define pcsl_mem_calloc_impl(x, y)  pcsl_mem_calloc_impl0((x), (y), __FILE__, __LINE__)

/**
 * Re-allocates memory at the given pointer in the private PCSL memory
 * pool (or null for new memory) so that it is the given size.
 */
#define pcsl_mem_realloc_impl(x, y) pcsl_mem_realloc_impl0((x), (y), __FILE__, __LINE__)

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define pcsl_mem_strdup_impl(x)     pcsl_mem_strdup_impl0((x), __FILE__, __LINE__)

/**
 * Frees the memory at the given pointer in the private PCSL memory
 * pool.
 */
#define pcsl_mem_free_impl(x)       pcsl_mem_free_impl0((x), __FILE__, __LINE__)

#else  

/**
 * Allocates memory from the private PCSL memory pool.
 * 
 * @param size number of bytes to allocate
 *
 * @return pointer to the newly allocated memory, or NULL if the system cannot
 * fulfil the allocation request
 */
extern void* pcsl_mem_malloc_impl0(unsigned int);

/**
 * Allocates memory from the private PCSL memory pool, and clears the
 * memory.
 *
 * @param nelem number of elements to allocate
 * @param elsize size of one element
 *
 * @return pointer to the newly allocated and cleared memory, or NULL if the
 * system cannot fulfil the allocation request
 */
extern void* pcsl_mem_calloc_impl0(unsigned int, unsigned int);

/**
 * Re-allocates memory from the private PCSL memory pool.
 * 
 * @param ptr original memory pointer, or null if this function should
 *        act like a call to pcsl_malloc_impl
 * @param size new amount of memory needed, in bytes, or 0 to have
 *        this function act like a call to pcslFreeImpl
 *
 * @return pointer to the re-allocated memory, or NULL if the system cannot
 * fulfil the allocation request
 */
extern void* pcsl_mem_realloc_impl0(void*, unsigned int);

/**
 * Duplicates the given string after allocating the memory for it.
 *
 * @param s1 string to duplicate
 *
 * @return pointer to the duplicate string, or NULL if the system cannot
 * fulfil the allocation request
 */
extern char* pcsl_mem_strdup_impl0(const char*);

/**
 * Frees memory allocated from the private PCSL memory pool
 *
 * @param ptr pointer to the allocated memory
 */
extern void  pcsl_mem_free_impl0(void*);

/**
 * Allocates the given number of bytes from the private PCSL memory
 * pool.
 */
#define pcsl_mem_malloc_impl(x)     pcsl_mem_malloc_impl0((x))

/**
 * Allocates and clears the given number of elements of the given size
 * from the private PCSL memory pool.
 */
#define pcsl_mem_calloc_impl(x, y)  pcsl_mem_calloc_impl0((x), (y))

/**
 * Re-allocates memory at the given pointer in the private PCSL memory
 * pool (or null for new memory) so that it is the given size.
 */
#define pcsl_mem_realloc_impl(x, y) pcsl_mem_realloc_impl0((x), (y))

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define pcsl_mem_strdup_impl(x)     pcsl_mem_strdup_impl0((x))

/**
 * Frees the memory at the given pointer in the private PCSL memory
 * pool.
 */
#define pcsl_mem_free_impl(x)       pcsl_mem_free_impl0((x))

#endif /* if PCSL_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_MEMORY_IMPL_H_ */

