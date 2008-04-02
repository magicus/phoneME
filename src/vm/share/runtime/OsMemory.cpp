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
 * OsMemory.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_OsMemory.cpp.incl"

#if ENABLE_PCSL
extern "C" {
#include <pcsl_memory.h>
}

#ifndef UNDER_CE
void *OsMemory_allocate(size_t size) {
  return pcsl_mem_malloc(size);
}

void OsMemory_free(void *p) {
  pcsl_mem_free(p);
}
#endif

address OsMemory_allocate_chunk(size_t initial_size,
                                size_t max_size, size_t alignment) {
  return (address)pcsl_mem_allocate_chunk((unsigned int)initial_size,
                                          (unsigned int)max_size, 
                                          (unsigned int)alignment);
}

size_t OsMemory_adjust_chunk(address chunk_ptr, size_t new_size) {
  return (size_t)
      pcsl_mem_adjust_chunk((void *)chunk_ptr, (unsigned int)new_size);
}

void OsMemory_free_chunk(address chunk_ptr) {
  pcsl_mem_free_chunk((void *)chunk_ptr);
}
#endif // ENABLE_PCSL

#if !ENABLE_PCSL && !SUPPORTS_ADJUSTABLE_MEMORY_CHUNK

// Here's a default implementation of OsMemory_allocate_chunk
// and OsMemory_adjust_chunk, based on OsMemory_malloc_bytes
//
// To use an alternative (real) implementation for your
// OS, define SUPPORTS_ADJUSTABLE_MEMORY_CHUNK=1 in your
// OS_<os_family>.hpp file

#define MAX_CHUNKS 2
static address chunks[MAX_CHUNKS] = {NULL, NULL};
static address chunks_orig[MAX_CHUNKS] = {NULL, NULL};
static size_t max_chunk_size[MAX_CHUNKS];
static size_t cur_chunk_size[MAX_CHUNKS];

//#ifndef PRODUCT 
#ifdef AZZERT
static int _cnt[MAX_CHUNKS] = {0, 0};

static void mark_unused_chunk_space(int i) {
  _cnt[i] ++;

  unsigned int base = (int)chunks[i];
  int *start = (int *)((base + cur_chunk_size[i]) + 0x03 & (~0x03));
  int *end   = (int *)((base + max_chunk_size[i])        & (~0x03));
  int *p;

  int marker = _cnt[i] + 0xdeadbeef;
  for (p = start; p<end; p++) {
    *p = marker;
  }
}

static void check_unused_chunk_space(int i) {
  unsigned int base = (unsigned int)chunks[i];
  int *start = (int *)((base + cur_chunk_size[i]) + 0x03 & (~0x03));
  int *end   = (int *)((base + max_chunk_size[i])        & (~0x03));
  int *p;

  int marker = _cnt[i] + 0xdeadbeef;
  for (p = start; p<end; p++) {
    GUARANTEE(*p == marker, "Uncommitted chunk was written into");
  }
  AZZERT_ONLY_VAR(marker);
}
#endif

address OsMemory_allocate_chunk(size_t initial_size,
                                size_t max_size, size_t alignment)
{
  GUARANTEE(chunks[0] == NULL || chunks[1] == NULL,
            "OsMemory_allocate_chunk supports only 2 chunks");
  int i;

  if (chunks[0] == NULL) {
    i = 0;
  } else {
    i = 1;
  }

  chunks_orig[i] = (address)OsMemory_allocate(max_size + alignment);
  if (chunks_orig[i] == NULL) {
    return NULL;
  }

  chunks[i] = (address)align_size_up((size_t)chunks_orig[i], alignment);

  max_chunk_size[i] = max_size;
  cur_chunk_size[i] = initial_size;

  AZZERT_ONLY(mark_unused_chunk_space(i));

  return chunks[i];
}

// Expand or shrink a chunk returned by allocate_chunk().
// The chunk is never moved.
//
// Returns 0 if fail to expand (shrink will always succeed). Returns old
// size if successful.
size_t OsMemory_adjust_chunk(address chunk_ptr, size_t new_size) {
  GUARANTEE(chunks[0] == chunk_ptr || chunks[1] == chunk_ptr, "Sanity check");
  int i;

  if (chunks[0] == chunk_ptr) {
    i = 0;
  } else {
    i = 1;
  }

  AZZERT_ONLY(check_unused_chunk_space(i));
  size_t old_size = cur_chunk_size[i];
  if (new_size <= max_chunk_size[i]) {
    cur_chunk_size[i] = new_size;
    AZZERT_ONLY(mark_unused_chunk_space(i));
    return old_size;
  } else {
    return 0;
  }
}

void OsMemory_free_chunk(address chunk_ptr) {
  GUARANTEE(chunks[0] == chunk_ptr || chunks[1] == chunk_ptr, "Sanity check");
  int i;

  if (chunks[0] == chunk_ptr) {
    i = 0;
  } else {
    i = 1;
  }

  OsMemory_free(chunks_orig[i]);
  chunks[i] = NULL;
}

size_t OsMemory_heap_initial_size(size_t /*min_size*/, size_t max_size) {
  return max_size;
}

size_t OsMemory_heap_expansion_target(size_t /*current_size*/, 
                                      size_t /*required_size*/, 
                                      size_t max_size,
                                      const bool /*is_full_collect*/) {
  return max_size;
}
size_t OsMemory_heap_reduction_target(size_t current_size,
                                      size_t /*used_size*/,
                                      size_t /*min_size*/) {
  return current_size;
}

#endif /* !ENABLE_PCSL && !SUPPORTS_ADJUSTABLE_MEMORY_CHUNK */

#if (ENABLE_PCSL || SUPPORTS_ADJUSTABLE_MEMORY_CHUNK) && \
    !SUPPORTS_CUSTOM_HEAP_ADJUSTMENT

/*
 * This is a default heap size adjustmemt policy:
 *
 * + initial size is 10% of HeapCapacity, but no less than 100K and no more
 *   than 1M.
 * + When we have done one GC and there still insufficient space, we double
 *   the heap size.
 * + After a collection, if the free space is more than half of the heap
 *   size, shrink heap by half.
 */

size_t OsMemory_heap_initial_size(size_t min_size, size_t max_size) {
  GUARANTEE(min_size <= max_size, "sanity");
  size_t init_size = min_size;
  if (init_size == 0) {
    // min_size is 0 means "pick the right initial size"
    init_size = max_size / 10;
  }

  if (UseCompiler) {
    // For better performance, we need at least 100KB, which would
    // give 20KB for the compiler_area
    if (init_size < 100 * 1024) {
      init_size = 100 * 1024;
    }
  }
  if (init_size > 1024 * 1024) {
    init_size = 1024 * 1024;
  }
  if (min_size != 0 && init_size < min_size) {
    init_size = min_size;
  }
  if (init_size > max_size) {
    init_size = max_size;
  }

  return init_size;
}

size_t OsMemory_heap_expansion_target(size_t current_size, 
                                      size_t required_size, 
                                      size_t max_size,
                                      const bool is_full_collect) {
  // In this sample implementation, we grow the heap only after a full
  // GC.
  //
  // To achieve better performance, some platforms may try to grow
  // the heap eagerly just after a young GC has executed.
  if (is_full_collect && current_size < required_size) {
    size_t new_size = required_size + current_size;
    if (new_size > max_size) {
      new_size = max_size;
    }
    return new_size;
  } else {
    return current_size;
  }
}

size_t OsMemory_heap_reduction_target(size_t current_size,
                                      size_t used_size,
                                      size_t min_size) {
  // Pretty naive heap shrinking strategy: If we have a lot
  // of unused space after a full collection, reduce the
  // unused space by half.
  size_t free_unused = current_size - used_size;
  if (free_unused > (current_size / 2)) {
    size_t new_size = current_size * 3 / 4;
    if (new_size < min_size) {
      new_size = min_size;
    }
    if (new_size < used_size) {
      new_size = used_size;
    }
    if (UseCompiler && new_size < 100 * 1024) {
      // For better performance, we need at least 100KB, which would
      // give 20KB for the compiler_area
      new_size = 100 * 1024;
    }

    return new_size;
  } else {
    return current_size;
  }
}
#endif // (ENABLE_PCSL || SUPPORTS_ADJUSTABLE_MEMORY_CHUNK) &&
       // !SUPPORTS_CUSTOM_HEAP_ADJUSTMENT
