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
 * OsMemory_win32.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_OsMemory_win32.cpp.incl"

#if !ENABLE_PCSL

#ifdef __cplusplus
extern "C" {
#endif

void *OsMemory_allocate(size_t size) {
  return malloc(size);
}

void OsMemory_free(void *p) {
  free(p);
}


address OsMemory_allocate_chunk(size_t initial_size,
                                size_t max_size, size_t alignment)
{
  address chunk = (address)VirtualAlloc(NULL, max_size, MEM_RESERVE, 
                                        PAGE_EXECUTE_READWRITE);
  GUARANTEE(((int)chunk) % alignment == 0, "must be aligned");

  if (chunk != NULL) {
    if (VirtualAlloc((LPVOID)chunk, initial_size, MEM_COMMIT,
                     PAGE_EXECUTE_READWRITE) == NULL) {
      // OS out of memory:
      VirtualFree(chunk, 0, MEM_RELEASE);
      chunk = NULL;
    }
  }

  return chunk;
}

// Expand or shrink a chunk returned by allocate_chunk().
// The chunk is never moved.
//
// Returns 0 if fail to expand (shrink will always succeed). Returns old
// size if successful.
size_t OsMemory_adjust_chunk(address chunk_ptr, size_t new_committed_size) {
  MEMORY_BASIC_INFORMATION info;
  VirtualQuery((LPVOID)chunk_ptr, &info, sizeof(info));
  size_t old_committed_size = info.RegionSize;

  if (new_committed_size > old_committed_size) {
    if (VirtualAlloc((LPVOID)chunk_ptr, new_committed_size, MEM_COMMIT,
                     PAGE_EXECUTE_READWRITE) == NULL) {
      AZZERT_ONLY(int err = GetLastError());
      return 0;
    } else {
      return old_committed_size;
    }
  } else {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    new_committed_size = align_size_up(new_committed_size,sys_info.dwPageSize);
    if (new_committed_size < old_committed_size) {
      address addr_to_free = chunk_ptr + new_committed_size;
      size_t bytes_to_free = old_committed_size - new_committed_size;
      //VirtualFree((LPVOID)addr_to_free, bytes_to_free, MEM_DECOMMIT);
    }
    return old_committed_size;
  }
}

void OsMemory_free_chunk(address chunk_ptr) {
  VirtualFree(chunk_ptr, 0, MEM_RELEASE);
}

#ifdef __cplusplus
}
#endif

#endif // !ENABLE_PCSL
