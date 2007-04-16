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

#ifndef _PCSL_MEMORY_PORT_IMPL_H_
#define _PCSL_MEMORY_PORT_IMPL_H_

#include <stdlib.h> 

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * This is posix-based implementation of 
 * <b>void* pcsl_heap_allocate_port(long size, long* outSize);</b>
 * 
 * @param    size required heap size in bytes
 * @param    outSize actual size of memory allocated
 * @return     a pointer to the newly allocated memory, or <tt>0</tt> if not available
 */
extern void *
pcsl_heap_allocate_port_impl(long size, /*OUT*/ long* outSize);

/**
 * This is a posix-based implementation of 
 * <b>void pcsl_heap_deallocate_port(void* heap);</b>
 *
 * @param    x memory pointer to free
 */
#define pcsl_heap_deallocate_port_impl(x)   free((x))

/**
 * This is a posix-based implementation of 
 * <b>void* pcsl_malloc_port(unsigned int size);</b>
 *
 * @param    x Number of byte to allocate
 * @return	  a pointer to the newly allocated memory
 */
#define pcsl_malloc_port_impl(x)            malloc((x))

/**
 * This is a posix-based implementation of 
 * <b>void  pcsl_free_port(void* ptr);</b>
 *
 * @param    x Pointer to allocated memory
 */
#define pcsl_free_port_impl(x)              free((x))

/**
 * This is a posix-based implementation of 
 * <b>void* pcsl_realloc_port(void* ptr, unsigned int size);</b>
 *
 * @param  ptr		Original memory pointer
 * @param  size		New size 
 * @return	  pointer to the re-allocated memory 
 */
#define pcsl_realloc_port_impl(x,y)         realloc((x),(y))

/**
 * This is a posix-based implementation of 
 * <b>void* pcsl_calloc_port(unsigned int numberOfElements, unsigned int elementSize );</b>
 *
 * @param    x Number of elements to allocate 
 * @param    y Size of one element 
 * @return	  pointer to the newly allocated and cleared memory 
 */
#define pcsl_calloc_port_impl(x,y)          calloc((x),(y))

/**
 * This is a posix-based implementation of 
 * <b>char* pcsl_strdup_port(const char* str);</b>
 *
 * @param    str	String to duplicate
 * @return	pointer to the duplicate string
 */
#define pcsl_strdup_port_impl(x)            strdup((x))

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_MEMORY_PORT_IMPL_H_ */

