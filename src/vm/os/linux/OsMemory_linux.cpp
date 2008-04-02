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
 * OsMemory_linux.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_OsMemory_linux.cpp.incl"

#if ENABLE_PCSL

extern "C" void init_jvm_chunk_manager() {
  // PCSL memory initialization/finalization would be done outside of
  // the VM module (by MIDP start-up code, or Main_linux.cpp)
}

#else
// !ENABLE_PCSL
/*
 * If PCSL (Portable Common Services Library) is enabled, we use the
 * memory allocation functions defined in PCSL.
 */

#ifdef __cplusplus
extern "C" {
#endif

void *OsMemory_allocate(size_t size) {
  return jvm_malloc(size);
}

void OsMemory_free(void *p) {
  jvm_free(p);
}

#if SUPPORTS_ADJUSTABLE_MEMORY_CHUNK

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

#if !defined(_SC_PAGE_SIZE) && defined(_SC_PAGESIZE)
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif

#define MAX_CHUNKS 8

typedef struct {
  address addr;
  size_t  size, mmaped_size;  
} ChunkInfo;

static ChunkInfo chunk_info[MAX_CHUNKS];
static int SysPageSize;

void init_jvm_chunk_manager() {
  jvm_memset(&chunk_info[0], 0, sizeof(chunk_info));
  SysPageSize = jvm_sysconf(_SC_PAGE_SIZE);
}

static inline ChunkInfo* get_chunk_info(address addr) {
  for (int i=0; i<MAX_CHUNKS; i++) {
    if (chunk_info[i].addr == addr) {
      return &chunk_info[i];
    }
  }

  return NULL;
}

static inline ChunkInfo* alloc_chunk(address addr, 
				     size_t  initial_size,
				     size_t  max_size) {
  int i;
  bool found = false;

  for (i=0; i<MAX_CHUNKS; i++) {
    if (chunk_info[i].addr == NULL) {
      found = true; 
      break;
    }
  }

  GUARANTEE(found, "maximal number of chunks exceeded");
  
  ChunkInfo* ci = &chunk_info[i];
  ci->addr = addr;
  ci->size = initial_size;
  ci->mmaped_size = max_size;

  return ci;
}

static inline void release_chunk(address addr) {
  int i;
  bool found = false;

  for (i=0; i<MAX_CHUNKS; i++) {
    if (chunk_info[i].addr == addr) {
      found = true; 
      break;
    }
  }

  GUARANTEE(found, "release nonexisting chunk");
  
  chunk_info[i].addr = NULL;
}

static inline size_t page_align_up(size_t size) {
  return align_size_up(size, SysPageSize);
}

static inline address anon_mmap(address addr, size_t size) {
  return (address)jvm_mmap(addr, size, 
			   PROT_EXEC | PROT_WRITE | PROT_READ,
			   MAP_PRIVATE | MAP_ANONYMOUS | (addr ? MAP_FIXED : 0),
			   -1, 0);
}

static inline int protect_area(address ptr, size_t size) { 
  return mprotect(ptr, size, PROT_NONE);
}

static inline int unprotect_area(address ptr, size_t size) { 
  return mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
}

address OsMemory_allocate_chunk(size_t initial_size,
                                size_t max_size, size_t alignment)
{
  // make it page aligned
  max_size = page_align_up(max_size);

  address chunk = anon_mmap(NULL, max_size);
  
  if (chunk == MAP_FAILED) {
    return NULL;
  }

  GUARANTEE((juint)chunk % alignment == 0, "must be aligned");
  GUARANTEE((juint)chunk % SysPageSize == 0, "must be page aligned");
  
  size_t aligned_size = page_align_up(initial_size);
  
  alloc_chunk(chunk, aligned_size, max_size);

  if (max_size > aligned_size) {
    protect_area(chunk + aligned_size, max_size - aligned_size);
  }

  return chunk;
}

// Expand or shrink a chunk returned by allocate_chunk().
// The chunk is never moved.
//
// Returns 0 if fail to expand (shrink will always succeed). Returns old
// size if successful.
size_t OsMemory_adjust_chunk(address chunk_ptr, size_t new_committed_size) {
  ChunkInfo* ci = get_chunk_info(chunk_ptr);

  size_t old_size = ci->size;
  size_t new_size = page_align_up(new_committed_size);

  if (new_size <= ci->mmaped_size) {
    int rv;
    if (new_size < old_size) {
      rv = protect_area(chunk_ptr + new_size, old_size - new_size);
    } else {
      rv = unprotect_area(chunk_ptr, new_size);
    }
    GUARANTEE(rv == 0, "mprotect must succeed");

    ci->size = new_size;

    return old_size;
  }

  new_size = page_align_up(new_size - ci->mmaped_size);
  if (anon_mmap(chunk_ptr + ci->mmaped_size, new_size) == NULL) {
    return 0;
  }

  ci->mmaped_size += new_size; 
  ci->size = ci->mmaped_size;
  unprotect_area(chunk_ptr, ci->size);
  
  return old_size;
}

void OsMemory_free_chunk(address chunk_ptr) {
  ChunkInfo* ci = get_chunk_info(chunk_ptr);
  jvm_munmap(ci->addr, ci->mmaped_size);
  release_chunk(chunk_ptr);
}

#endif // SUPPORTS_ADJUSTABLE_MEMORY_CHUNK

#ifdef __cplusplus
}
#endif

#endif // !ENABLE_PCSL
