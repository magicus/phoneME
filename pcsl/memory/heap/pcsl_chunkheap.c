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
 * PCSL chunk memory functions for use by the CLDC Java heap.
 *
 * Memory allocated via these methods is manged outside of
 * the pcls_memory pool.  These functions exist to support
 * the JVM's use of a dynamicly sized Java heap.
 *
 * Note: This implementation is based on pcsl_mem_malloc(). If your
 * platform supports dynamic memory page management, see
 * ../chunk_mmap/pcsl_chunkmem.c for an example.
 */

#include <pcsl_memory.h>
#include <pcsl_print.h>


/*
 * ChunkInfo data structure stores details about allocated memory chunks.
 */
typedef struct _ChunkInfo {
    void* ptr;
    void* aligned_start_address;
    unsigned int cur_size;
    unsigned int max_size;
    struct _ChunkInfo *next;
} ChunkInfo;

static ChunkInfo *chunks = NULL;


static ChunkInfo *get_chunk_info(void* chunk_ptr) {
    ChunkInfo *ci;
    for (ci = chunks; ci; ci = ci->next) {
        if (ci->aligned_start_address == chunk_ptr) {
            return ci;
        }
    }

#ifdef PCSL_DEBUG
    /* should not reach here */
    pcsl_print("ERROR: get_chunk_info failed to find requested chunk\n");
#endif
    return NULL;
}

/*
 * Converts <code>ptr</code> to a multiple of <code>alignment</code> equal 
 * to or greater than <code>ptr</code> 
 */
void* align_size_up(void *ptr, unsigned int alignment) {
    return (void*)(((unsigned int)ptr + alignment - 1) & ~(alignment - 1));
}

/*
 * Allocate a memory chunk that can be shrunk, or expanded (up to max_size)
 * The returned pointer is <alignment>-bytes aligned.
 * Returns NULL if unsuccessful
 */
void* pcsl_mem_allocate_chunk(unsigned int initial_size,
                              unsigned int max_size, 
                              unsigned int alignment){
    ChunkInfo *ci;
    void *ptr;
    ci = (ChunkInfo*)pcsl_mem_malloc(sizeof(ChunkInfo));
    if (ci == NULL) {
        return NULL;
    }

    ptr = pcsl_mem_malloc(max_size + alignment);
    if (ptr == NULL) {
	pcsl_mem_free(ci);
        return NULL;
    }

    ci->ptr = ptr;
    ci->aligned_start_address = align_size_up(ptr, alignment);
    ci->max_size = max_size;
    ci->cur_size = initial_size;
    ci->next = chunks;
    chunks = ci;

    return ci->aligned_start_address;
}
   
/*
 * Expand or shrink a chunk returned by allocate_chunk().
 * The chunk is never moved.
 *
 * Returns 0 if fail to expand (shrink will always succeed). Returns old
 * size if successful.
 */
unsigned int pcsl_mem_adjust_chunk(void *chunk_ptr, unsigned int new_size){

    unsigned int old_size;
    ChunkInfo* ci = get_chunk_info(chunk_ptr);

    if (new_size > ci->max_size) {
#ifdef PCSL_DEBUG
        pcsl_print("DEBUG:  pcsl_mem_adjust_chunk size check failed\n");
#endif
        return 0;
    }

    old_size = ci->cur_size;
    ci->cur_size = new_size;
    return old_size;
}


/*
 * Free a chunk returned by pcsl_mem_allocate_chunk() 
 */
void pcsl_mem_free_chunk(void *chunk_ptr){
    ChunkInfo *ci;
    ChunkInfo *last = NULL;
    for (ci = chunks; ci; last = ci, ci = ci->next) {
        if (ci->aligned_start_address == chunk_ptr) {
            if (last == NULL) {
                chunks = ci->next;
            } else {
                last->next = ci->next;
            }
            pcsl_mem_free((void*)ci->ptr);
            pcsl_mem_free((void*)ci);
            return;
        }
    }
#ifdef PCSL_DEBUG
    /* should not reach here! */
    pcsl_print("ERROR: pcsl_free_chunk() could not find the required chunk to free\n");
#endif
}
