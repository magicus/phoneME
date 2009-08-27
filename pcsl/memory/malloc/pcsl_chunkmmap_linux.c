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
 */

#include <pcsl_memory.h>
#include <pcsl_print.h>

#ifdef SOLARIS
  // otherwise mmap/munmap has weird signature
#define _XPG4_2 1
#endif
#include <sys/mman.h>
#include <unistd.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

#define MAX_CHUNKS 8

typedef struct _ChunkInfo {
    void * addr;
    unsigned int size;
    unsigned int mmaped_size;  
    struct _ChunkInfo *next;
} ChunkInfo;

static ChunkInfo *chunks = NULL;

static ChunkInfo *get_chunk_info(void* chunk_ptr) {
    ChunkInfo *ci;
    for (ci = chunks; ci; ci = ci->next) {
        if (ci->addr == chunk_ptr) {
            return ci;
        }
    }

#ifdef PCSL_DEBUG
    /* should not reach here */
    pcsl_print("ERROR: get_chunk_info failed to find requested chunk\n");
#endif
    return NULL;
}

static ChunkInfo* alloc_chunk(void * addr, 
                              unsigned int  initial_size,
                              unsigned int  max_size) {
    ChunkInfo *ci = (ChunkInfo*)pcsl_mem_malloc(sizeof(ChunkInfo));
    ci->addr = addr;
    ci->size = initial_size;
    ci->mmaped_size = max_size;

    ci->next = chunks;
    chunks = ci;

    return ci;
}

static unsigned int page_align_up(unsigned int size) {
    int SysPageSize = sysconf(_SC_PAGE_SIZE);
    return (size + (SysPageSize - 1)) & ~(SysPageSize - 1);
}

static void* anon_mmap(void * addr, unsigned int size) {
    return (void *)mmap(addr, size, 
                        PROT_EXEC | PROT_WRITE | PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | (addr ? MAP_FIXED:0),
                        -1, 0);
}

static int protect_area(void * ptr, unsigned int size) { 
    return mprotect(ptr, size, PROT_NONE);
}

static int unprotect_area(void * ptr, unsigned int size) { 
    return mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
}


void * pcsl_mem_allocate_chunk(unsigned int initial_size,
                               unsigned int max_size, 
                               unsigned int alignment)
{
    void *chunk;
    unsigned int aligned_size;

    max_size = page_align_up(max_size);
    chunk = anon_mmap(NULL, max_size);
  
    if (chunk == MAP_FAILED) {
        return NULL;
    }

#ifdef PCSL_DEBUG
    if (((unsigned int)chunk % alignment) != 0) {
        pcsl_print("ERROR: anon_mmap() must return algined address");
    }
    if (((unsigned int)chunk % sysconf(_SC_PAGE_SIZE)) != 0) {
        pcsl_print("ERROR: anon_mmap() must return page-algined address");
    }
#endif  

    aligned_size = page_align_up(initial_size);  
    alloc_chunk(chunk, aligned_size, max_size);

    if (max_size > aligned_size) {
        protect_area(chunk + aligned_size, max_size - aligned_size);
    }

    return chunk;
}

/* Expand or shrink a chunk returned by allocate_chunk().
 * The chunk is never moved.
 *
 * Returns 0 if fail to expand (shrink will always succeed). Returns old
 * size if successful.
 */
unsigned int pcsl_mem_adjust_chunk(void *chunk_ptr, unsigned int new_size) {
    ChunkInfo* ci = get_chunk_info(chunk_ptr);

    unsigned int old_size = ci->size;
    new_size = page_align_up(new_size);

    if (new_size <= ci->mmaped_size) {
        int rv;
        if (new_size < old_size) {
            rv = protect_area(chunk_ptr + new_size, old_size - new_size);
        } else {
            rv = unprotect_area(chunk_ptr, new_size);
        }

        ci->size = new_size;

        return old_size;
    } else {
#ifdef PCSL_DEBUG
	pcsl_print("DEBUG:  pcsl_mem_adjust_chunk size check failed\n");
#endif
      	return 0;
    }

}

void pcsl_mem_free_chunk(void *chunk_ptr) {
    ChunkInfo *ci;
    ChunkInfo *last = NULL;
    for (ci = chunks; ci; last = ci, ci = ci->next) {
        if (ci->addr == chunk_ptr) {
            if (last == NULL) {
                chunks = ci->next;
            } else {
                last->next = ci->next;
            }
            munmap(ci->addr, ci->mmaped_size);
            pcsl_mem_free((void*)ci);
            return;
        }
    }
#ifdef PCSL_DEBUG
    /* should not reach here! */
    pcsl_print("ERROR: pcsl_free_chunk() could not find the required chunk to free\n");
#endif
}
