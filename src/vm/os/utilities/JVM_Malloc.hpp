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

/**
 * @file
 */

/**
 * Functions and data structures that enable JVM to manage its own
 * memory pool. This avoids memory fragmentation in the overall
 * system, but it does not address the potential problem of memory
 * fragmentation within the JVM application itself.
 *
 * <p>The memory pool is one contiguous chunk of memory split up into
 * various free or allocated blocks. All blocks have the same layout:
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
 * @warning This code is not thread safe
 */

#ifndef _JVM_MALLOC_H_
#define _JVM_MALLOC_H_

#include <stdlib.h> 

#ifdef REPORT_LEVEL
#include <logging.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Enables the private handling of memory allocations
 */
#if ENABLE_JVM_MALLOC

/*
 * Enables the private handling of memset & memcpy APIs 
 */
#if ENABLE_JVM_MEM_API

void * JVM_Memset ( void *dst, int val, int count);

void * JVM_Memcpy ( void * dstPtr, const void * srcPtr, int count);

#else

#define JVM_Memset(x, y, z) memset(x, y, z)

#define JVM_Memcpy(x, y, z) memcpy(x, y, z)

#endif

/**
 * @note JVM_Malloc debug information:
 *
 * When <code>REPORT_LEVEL <= LOG_WARNING</code>  JVM_Malloc trace info is 
 * output by the log service.  This traces where memory is being 
 * allocated and freed on report channel <code>LC_MALLOC.</code>
 *
 * When <code>REPORT_LEVEL <= LOG_INFORMATION</code> JVM_Malloc debug info 
 * is output by the log service.  This enables debugging output from 
 * the memory manager to appear on report channel <code>LC_MALLOC.</code>
 */

/**
 * Initializes the JVM memory pool.
 * <p><b>NOTE:</b> This function must only be called once.
 * 
 * @param size size, in bytes, of the memory pool; if size is -1, the
 *        default memory pool size will be used
 *
 * @return 0 if the function returns successfully; a non-zero value
 * otherwise
 */
 int JVM_InitializeMemory(char*, int);

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
void JVM_FinalizeMemory(void);

/**
 * Gets the maximum amount of heap space, in bytes, available to the system
 * for allocation. This value is constant throughout the lifetime of the
 * process.
 *
 * @return the maximum number of bytes available on the heap, or -1 if the
 * information is not available
 */
int   JVM_GetTotalHeap();

/**
 * Gets the current amount of unused heap space, in bytes. This value
 * changes with every <tt>JVM_Malloc</tt> and <tt>JVM_Free</tt>
 * function call.
 *
 * @return the number of bytes of heap space that are currently unused, or -1
 * if the information is not available
 */
int   JVM_GetFreeHeap();

/*
 * Private implementations for memory allocation
 */

/**
 * Allocates memory from the private JVM memory pool.
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
void* jvmMallocImpl(unsigned int, char*, int);

/**
 * Allocates memory from the private JVM memory pool, and clears the
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
void* jvmCallocImpl(unsigned int, unsigned int, char*, int);

/**
 * Re-allocates memory from the private JVM memory pool.
 * 
 * @param ptr original memory pointer, or null if this function should
 *        act like a call to jvmMallocImpl
 * @param size new amount of memory needed, in bytes, or 0 to have
 *        this function act like a call to jvmFreeImpl
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the re-allocated memory, or NULL if the system cannot
 * fulfil the allocation request
 */
void* jvmReallocImpl(void*, unsigned int, char*, int);

/**
 * Duplicates the given string after allocating the memory for it.

 * @param s1 string to duplicate
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 *
 * @return pointer to the duplicate string, or NULL if the system cannot
 * fulfil the allocation request
 */
char* jvmStrdupImpl(const char*, char*, int);

/**
 * Frees memory allocated from the private JVM memory pool
 *
 * @param ptr pointer to the allocated memory
 * @param filename name of the file where the allocation call occurred
 *        (for tracing purposes)
 * @param lineno line number where the allocation call occured (for
 *        tracing purposes)
 */
void  jvmFreeImpl(void*, char*, int);

/**
 * Displays the current state of the memory sub-system. 
 * 
 * @param countMemoryLeaksOnly amount of data to gather: if is non-zero,
 *        display allocated blocks of memory; otherwise display both allocated
 *        and free blocks
 *
 * @return the number of allocated blocks, or -1 if there was an error
 */
int JVM_MallocDumpMemory(int countMemoryLeaksOnly);



/**
 * IMPL_NOTE:IMPL_NOTE: consider whether tracenativememory should be removed.
 */
extern int tracenativememory;

/**
 * Allocates the given number of bytes from the private JVM memory
 * pool.
 */
#define JVM_Malloc(x)     jvmMallocImpl((x), __FILE__, __LINE__)

/**
 * Allocates and clears the given number of elements of the given size
 * from the private JVM memory pool.
 */
#define JVM_Calloc(x, y)  jvmCallocImpl((x), (y), __FILE__, __LINE__)

/**
 * Re-allocates memory at the given pointer in the private JVM memory
 * pool (or null for new memory) so that it is the given size.
 */
#define JVM_Realloc(x, y) jvmReallocImpl((x), (y), __FILE__, __LINE__)

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define JVM_Strdup(x)     jvmStrdupImpl((x), __FILE__, __LINE__)

/**
 * Frees the memory at the given pointer in the private JVM memory
 * pool.
 */
#define JVM_Free(x)       jvmFreeImpl((x), __FILE__, __LINE__)


#else /* DON'T ENABLE_JVM_MALLOC */

/*
 * We're not using our own memory manager, use the standard library
 * function calls to allocate memory.
 */

/**
 * Initializes a JVM memory pool of the given size. <b>NOTE:</b> This
 * function must only be called once.
 */
#define JVM_InitializeMemory(x, y)  (0)

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
#define JVM_FinalizeMemory()

/**
 * Gets the maximum amount of heap space, in bytes, available to the
 * system for allocation. This value is constant throughout the
 * lifetime of the process.
 */
#define JVM_GetTotalHeap() (-1)

/**
 * Returns the current amount of unused heap space, in bytes. This
 * value changes with every <tt>malloc</tt> and <tt>free</tt> function
 * call.
 */
#define JVM_GetFreeHeap()  (-1)

/**
 * Allocates memory of the given size from the private JVM memory
 * pool.
 */
#define JVM_Malloc(x)     malloc((x))
#define jvmMallocImpl(x, a, b)     JVM_Malloc((x))

/**
 * Allocates and clears the given number of elements of the given size
 * from the private JVM memory pool.
 */
#define JVM_Calloc(x, y)  calloc((x), (y))
#define jvmCallocImpl(x, y, a, b)  JVM_Calloc((x), (y))

#if NO_STRDUP
/**
 * IMPL_NOTE:Document this version of strdup.
 */
extern char* strdup(const char*);
#endif

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define JVM_Strdup(x)     strdup((x))
#define jvmStrdupImpl(x, a, b)     JVM_Strdup((x))

/**
 * Frees memory at the given pointer in the private JVM memory pool.
 */
#define JVM_Free(x)       do { \
                              void* __jvmFree_ptr__ = (void*)(x); \
                              if(__jvmFree_ptr__ != NULL) { \
                                  free(__jvmFree_ptr__); \
                              } \
                          } while (0)
#define jvmFreeImpl(x, a, b)       JVM_Free((x))

/**
 * Re-allocates memory at the given pointer location in the private
 * JVM memory pool (or null for new memory) so that it is the given
 * size.
 */
#define JVM_Realloc(x, y) realloc((x), (y))
#define jvmReallocImpl(x, y, a, b) JVM_Realloc((x), (y))

/**
 * Displays the current state of the memory sub-system. 
 */
#define JVM_MallocDumpMemory(x) (-1)

#endif /* ENABLE_JVM_MALLOC */

#ifdef __cplusplus
}
#endif
#endif /* _JVM_MALLOC_H_ */
