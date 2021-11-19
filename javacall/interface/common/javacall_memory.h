/*
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

#ifndef __JAVACALL_MEMORY_H_
#define __JAVACALL_MEMORY_H_

/**
 * @file javacall_memory.h
 * @ingroup Memory
 * @brief Javacall interfaces for memory
 */

#ifdef __cplusplus
extern "C" {
#endif
    
    
/** 
 * @defgroup Memory Memory API
 * @ingroup JTWI
 *
 * The Java VM handles memory allocation internally. 
 * As a result, upon VM startup, Java asks for a big continous memory range and frees this memory only upon VM shutdown. 
 * Memory API specification allows the platform to allocate the big memory range using a specialized function called 
 * javacall_memory_heap_allocate.
 * 
 * @{
 */

/** @defgroup MandatoryMemory Mandatory Memory API
 *  @ingroup Memory
 * 
 * The Java VM handles memory allocation internally. As a result, upon VM startup, Java asks for a big
 * continous memory range and frees this memory only upon VM shutdown.\n
 * Memory API specification allows the platform to allocate the big memory range using a specialized
 * function called javacall_memory_heap_allocate.
 *  
 *  @{
 */


/** 
 * Allocates large memory heap
 * VM will use this memory heap for internal memory allocation/deallocation
 * Will be called ONCE during VM startup!
 * 
 * @param    size required heap size in bytes
 * @param    outSize actual size of memory allocated
 * @return	  a pointer to the newly allocated memory, or <tt>0</tt> if not available
 */
void* javacall_memory_heap_allocate(long size, /*OUT*/ long* outSize);
    
/** 
 * Free large memory heap
 * VM will call this function once when VM is shutdown to free large memory heap 
 * Will be called ONCE during VM shutdown!
 * 
 * @param    heap memory pointer to free
 */
void javacall_memory_heap_deallocate(void* heap);
    
/** 
 * Allocates memory of the given size from the private JAVACALL memory
 * pool.
 * 
 * @param    size Number of byte to allocate
 * @return	  a pointer to the newly allocated memory
 */
void* javacall_malloc(unsigned int size);

/** 
 * Reallocates memory of the given size from the private JAVACALL memory
 * pool. If memory could not be reallocated function returns null,
 * in this case old pointer is not released.
 * 
 * @param    ptr  Pointer to previously allocated memory
 * @param    size Number of byte to allocate
 * @return	  a pointer to the reallocated memory or null if memory could not be reallocated
 */
void* javacall_realloc(void* ptr, unsigned int size);
    


/**
 * Frees memory at the given pointer in the private JAVACALL memory pool.
 * 
 * @param    ptr	Pointer to allocated memory
 */
void  javacall_free(void* ptr);
        
/** @} */
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/** @defgroup OptionalMemory Optional Memory API
 *  @ingroup Memory
 * 
 * The following functions \n
 * - malloc \n
 * - free \n
 * - calloc \n
 * - realloc \n
 * - strdup \n
 * 
 * \n
 * can be implemented using basic malloc functionality, but using platform's optimized implementation is
 * preferred as these function are commonly-used functions.\n
 * The following definitions declare the standard memory allocation functions malloc and free
 * 
 *  @{
 */    



/** 
 * Allocates and clears the given number of elements of the given size
 * from the private JAVACALL memory pool.
 * 
 * @param    numberOfElements Number of elements to allocate 
 * @param    elementSize Size of one element 
 * @return	  pointer to the newly allocated and cleared memory 
 */
void* /*OPTIONAL*/ javacall_calloc(unsigned int numberOfElements, unsigned int elementSize );
    
/**
 * Re-allocates memory at the given pointer location in the private
 * JAVACALL memory pool (or null for new memory) so that it is the given
 * size.
 * 
 * @param  ptr		Original memory pointer
 * @param  size		New size 
 * @return	  pointer to the re-allocated memory 
 */
void* /*OPTIONAL*/ javacall_realloc(void* ptr, unsigned int size);
    
    /**
 * Duplicates the given string after allocating the memory for it.
 * 
 * @param    str	String to duplicate
 * @return	pointer to the duplicate string
 */
char* /*OPTIONAL*/ javacall_strdup(const char* str);
    
/** @} */

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif 


