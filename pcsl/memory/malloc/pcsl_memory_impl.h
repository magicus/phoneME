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

#include <pcsl_memory_port.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * We're not using our own memory manager, use the standard library
 * function calls to allocate memory.
 */

/**
 * Allocates memory of the given size from the private PCSL memory
 * pool.
 */
#define pcsl_mem_malloc_impl(x)     pcsl_malloc_port((x))

/**
 * Allocates and clears the given number of elements of the given size
 * from the private PCSL memory pool.
 */
#define pcsl_mem_calloc_impl(x, y)  pcsl_calloc_port((x), (y))

/**
 * Duplicates the given string after allocating the memory for it.
 */
#define pcsl_mem_strdup_impl(x)     pcsl_strdup_port((x))

/**
 * Frees memory at the given pointer in the private PCSL memory pool.
 */
#define pcsl_mem_free_impl(x)       do { \
                                        void* __pcslFree_ptr__ = (void*)(x); \
                                        if(__pcslFree_ptr__ != NULL) { \
                                            pcsl_free_port(__pcslFree_ptr__); \
                                        } \
                                    } while (0)

/**
 * Re-allocates memory at the given pointer location in the private
 * PCSL memory pool (or null for new memory) so that it is the given
 * size.
 */
#define pcsl_mem_realloc_impl(x, y) pcsl_realloc_port((x), (y))

/**
 * Initializes a PCSL memory pool of the given size and starting at the specified
 * address (if not NULL). <b>NOTE:</b> This
 * function must only be called once.
 */
#define pcsl_mem_initialize_impl(x, y) (0)

/**
 * Takes any actions necessary to safely terminate the memory
 * subsystem.
 */
#define pcsl_mem_finalize_impl()

/**
 * Gets the maximum amount of heap space, in bytes, available to the
 * system for allocation. This value is constant throughout the
 * lifetime of the process.
 */
#define pcsl_mem_get_total_heap_impl() (-1)

/**
 * Returns the current amount of unused heap space, in bytes. This
 * value changes with every <tt>malloc</tt> and <tt>free</tt> function
 * call.
 */
#define pcsl_mem_get_free_heap_impl()  (-1)

/**
 * Displays the current state of the memory sub-system.
 */
#define pcsl_mem_malloc_dump_impl(x) (-1)

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_MEMORY_IMPL_H_ */

