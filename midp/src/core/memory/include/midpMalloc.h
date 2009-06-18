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
 * @brief Interface for handling MIDP's memory pool.
 *  This interface is replaced by PCSL Memory.
 *
 * <p>MIDP's memory pool is one contiguous chunk of memory split up into
 * free or allocated blocks. All blocks have the same layout:</p>
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
 * tracing is enabled.</p>
 *
 * <p>Having MIDP manage its own memory pool avoids memory
 * fragmentation in the overall system.  It does not address the
 * potential problem of memory fragmentation within the MIDP
 * application itself. <!-- ??? I thought MIDP defragged its memory
 * block ??? --></p>

 * @warning This code is not thread safe
 */

#ifndef _MIDP_MALLOC_H_
#define _MIDP_MALLOC_H_

#include <stdlib.h>

#include <midp_logging.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enables the private handling of memory allocations
 */
#if ENABLE_MIDP_MALLOC

/**
 * @note midpMalloc debug information:
 *
 * When <code>REPORT_LEVEL <= LOG_WARNING</code>  midpMalloc trace info is 
 * output by the log service.  This traces where memory is being 
 * allocated and freed on report channel <code>LC_MALLOC.</code>
 *
 * When <code>REPORT_LEVEL <= LOG_INFORMATION</code> midpMalloc debug info 
 * is output by the log service.  This enables debugging output from 
 * the memory manager to appear on report channel <code>LC_MALLOC.</code>
 */

/**
 * Initializes the MIDP memory pool.
 * <p><b>NOTE:</b> This function must only be called once.
 * 
 * @param size size, in bytes, of the memory pool; if size is -1, the
 *        default memory pool size will be used
 *
 * @return 0 if the function returns successfully; a non-zero value
 * otherwise
 */
int midpInitializeMemory(int);

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
void midpFinalizeMemory(void);

/**
 * Gets the maximum amount of heap space, in bytes, available to the system
 * for allocation. This value is constant throughout the lifetime of the
 * process.
 *
 * @return the maximum number of bytes available on the heap, or -1 if the
 * information is not available
 */
int   midpGetTotalHeap();

/**
 * Gets the current amount of unused heap space, in bytes. This value
 * changes with every <tt>midpMalloc</tt> and <tt>midpFree</tt>
 * function call.
 *
 * @return the number of bytes of heap space that are currently unused, or -1
 * if the information is not available
 */
int   midpGetFreeHeap();

/*
 * Private implementations for memory allocation
 */

/**
 * Allocates memory from the private MIDP memory pool.
 * 
 * @param size number of bytes to allocate
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occurred (for
 *        tracing purposes)
 *
 * @return pointer to the newly allocated memory, or NULL if the system cannot
 * fulfill the allocation request
 */
void* midpMallocImpl(unsigned int, char*, int);

/**
 * Allocates memory from the private MIDP memory pool, and clears the
 * memory.
 *
 * @param nelem number of elements to allocate
 * @param elsize size of one element
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occurred (for
 *        tracing purposes)
 *
 * @return pointer to the newly allocated and cleared memory, or NULL if the
 * system cannot fulfill the allocation request
 */
void* midpCallocImpl(unsigned int, unsigned int, char*, int);

/**
 * Re-allocates memory from the private MIDP memory pool.
 * 
 * @param ptr original memory pointer, or null if this function should
 *        act like a call to midpMallocImpl
 * @param size new amount of memory needed, in bytes, or 0 to have
 *        this function act like a call to midpFreeImpl
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occurred (for
 *        tracing purposes)
 *
 * @return pointer to the re-allocated memory, or NULL if the system cannot
 * fulfill the allocation request
 */
void* midpReallocImpl(void*, unsigned int, char*, int);

/**
 * Duplicates the given string after allocating the memory for it.

 * @param s1 string to duplicate
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occurred (for
 *        tracing purposes)
 *
 * @return pointer to the duplicate string, or NULL if the system cannot
 * fulfill the allocation request
 */
char* midpStrdupImpl(const char*, char*, int);

/**
 * Frees memory allocated from the private MIDP memory pool
 *
 * @param ptr pointer to the allocated memory
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occurred (for
 *        tracing purposes)
 */
void  midpFreeImpl(void*, char*, int);

/**
 * Displays the current state of the memory sub-system. 
 * 
 * @param countMemoryLeaksOnly amount of data to gather: if is non-zero,
 *        display allocated blocks of memory; otherwise display both allocated
 *        and free blocks
 *
 * @return the number of allocated blocks, or -1 if there was an error
 */
int midpMallocDumpMemory(int countMemoryLeaksOnly);

#if REPORT_LEVEL <= LOG_WARNING

/**
 * Allocates the given number of bytes from the private MIDP memory
 * pool.
 */
#define midpMalloc(x)     midpMallocImpl((x), __FILE__, __LINE__)

/**
 * Allocates and clears the given number of elements of the given size
 * from the private MIDP memory pool.
 */
#define midpCalloc(x, y)  midpCallocImpl((x), (y), __FILE__, __LINE__)

/**
 * Re-allocates memory at the given pointer in the private MIDP memory
 * pool (or null for new memory) so that it is the given size.
 */
#define midpRealloc(x, y) midpReallocImpl((x), (y), __FILE__, __LINE__)

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define midpStrdup(x)     midpStrdupImpl((x), __FILE__, __LINE__)

/**
 * Frees the memory at the given pointer in the private MIDP memory
 * pool.
 */
#define midpFree(x)       midpFreeImpl((x), __FILE__, __LINE__)

#else  

/**
 * Allocates memory of the given size from the private MIDP memory
 * pool.
 */
#define midpMalloc(x)     midpMallocImpl((x), NULL, 0)

/**
 * Allocates and clears the given number of elements of the given size
 * from the private MIDP memory pool.
 */
#define midpCalloc(x, y)  midpCallocImpl((x), (y), NULL, 0)

/**
 * Re-allocates memory at the given pointer location in the private
 * MIDP memory pool (or null for new memory) so that it is the given
 * size.
 */
#define midpRealloc(x, y) midpReallocImpl((x), (y), NULL, 0)

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define midpStrdup(x)     midpStrdupImpl((x), NULL, 0)

/**
 * Frees memory at the given pointer in the private MIDP memory pool.
 */
#define midpFree(x)       midpFreeImpl((x), NULL, 0)

#endif /* if REPORT_LEVEL <= LOG_WARNING */

#else /* DON'T ENABLE_MIDP_MALLOC */

/*
 * We're not using our own memory manager, use the standard library
 * function calls to allocate memory.
 */

/**
 * Initializes a MIDP memory pool of the given size. <b>NOTE:</b> This
 * function must only be called once.
 */
#define midpInitializeMemory(x) (0)

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
#define midpFinalizeMemory()

/**
 * Gets the maximum amount of heap space, in bytes, available to the
 * system for allocation. This value is constant throughout the
 * lifetime of the process.
 */
#define midpGetTotalHeap() (-1)

/**
 * Returns the current amount of unused heap space, in bytes. This
 * value changes with every <tt>malloc</tt> and <tt>free</tt> function
 * call.
 */
#define midpGetFreeHeap()  (-1)

/**
 * Allocates memory of the given size from the private MIDP memory
 * pool.
 */
#define midpMalloc(x)     malloc((x))
#define midpMallocImpl(x, a, b)     midpMalloc((x))

/**
 * Allocates and clears the given number of elements of the given size
 * from the private MIDP memory pool.
 */
#define midpCalloc(x, y)  calloc((x), (y))
#define midpCallocImpl(x, y, a, b)  midpCalloc((x), (y))

#if NO_STRDUP
/**
 * IMPL_NOTE:Document this version of strdup.
 */
extern char* strdup(const char*);
#endif

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define midpStrdup(x)     strdup((x))
#define midpStrdupImpl(x, a, b)     midpStrdup((x))

/**
 * Frees memory at the given pointer in the private MIDP memory pool.
 */
#define midpFree(x)       do { \
                              void* __midpFree_ptr__ = (void*)(x); \
                              if(__midpFree_ptr__ != NULL) { \
                                  free(__midpFree_ptr__); \
                              } \
                          } while (0)
#define midpFreeImpl(x, a, b)       midpFree((x))

/**
 * Re-allocates memory at the given pointer location in the private
 * MIDP memory pool (or null for new memory) so that it is the given
 * size.
 */
#define midpRealloc(x, y) realloc((x), (y))
#define midpReallocImpl(x, y, a, b) midpRealloc((x), (y))

/**
 * Displays the current state of the memory sub-system. 
 */
#define midpMallocDumpMemory(x) (-1)

#endif /* ENABLE_MIDP_MALLOC */

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_MALLOC_H_ */

