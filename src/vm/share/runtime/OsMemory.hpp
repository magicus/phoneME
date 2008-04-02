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

/*
 * OsMemory.hpp:
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * OsMemory_allocate_chunk()
 * OsMemory_adjust_chunk()
 * OsMemory_free_chunk()
 *
 * These 3 functions are called by the ObjectHeap class to manage memory space
 * for the Java heap.
 *
 * If the macro SUPPORTS_ADJUSTABLE_MEMORY_CHUNK=1 is defined (in
 * Globals_<os_family>.hpp), these 3 functions should be implemented
 * in the OsMemory_<os_family>.cpp file. Otherwise, a default implementation
 * is provided in OsMemory.cpp.
 */

// Allocate a memort chunk that can be shrunk, or expanded (up to max_size)
// The returned pointer is <alignment>-bytes aligned.
address OsMemory_allocate_chunk(size_t initial_size,
                                size_t max_size, size_t alignment);
// Expand or shrink a chunk returned by allocate_chunk().
// The chunk is never moved.
// Returns 0 if fail to expand (shrink will always succeed). Returns old
// size if successful.
size_t OsMemory_adjust_chunk(address chunk_ptr, size_t new_size);
void   OsMemory_free_chunk(address chunk_ptr);

void  *OsMemory_allocate(size_t size);
void   OsMemory_free(void *p);

/*
 * OsMemory_heap_initial_size()
 * OsMemory_heap_expansion_target()
 * OsMemory_heap_reduction_target()
 *
 * These 3 functions are called by the ObjectHeap class to control the
 * expansion and shrinking of the Java heap.
 *
 * If the macro SUPPORTS_CUSTOM_HEAP_ADJUSTMENT=1 is defined (in
 * Globals_<os_family>.hpp), these 3 functions should be implemented
 * in the OsMemory_<os_family>.cpp file. Otherwise, a default implementation
 * is provided in OsMemory.cpp.
 */
size_t OsMemory_heap_initial_size(size_t min_size, size_t max_size);

/*
 * This function is called when at least one GC has occurred and we still
 * do not have enough heap space to satisfy an allocation request.
 *
 * The returned value specify the new heap size. If the returned value is
 * larger than current_size, OsMemory_adjust_chunk will be subsequently
 * invoked to expand the Java heap.

 * current_size  = current heap size (bytes)
 * required_size = min heap size to satisfy the allocation request
 * max_size      = maximum max heap size allowed
 *
 * The returned value n must be in the range: required_size <= n <= max_size
 */
size_t OsMemory_heap_expansion_target(size_t current_size, 
                                      size_t required_size, 
                                      size_t max_size,
                                      const bool is_full_collect);

/*
 * This function is called whenever a full GC has been executed.
 *
 * The returned value specify the new heap size. If the returned value is
 * lower than current_size, OsMemory_adjust_chunk will be subsequently
 * invoked to shrink the Java heap.
 *
 * The returned value n must be in the range: used_size <= n <= current_size
 */
size_t OsMemory_heap_reduction_target(size_t current_size,
                                      size_t used_size,
                                      size_t min_size);


#ifdef __cplusplus
}
#endif
