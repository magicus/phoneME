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

// All classes in the virtual machine must be subclassed
// by one of the following allocation classes:
//
// For objects allocated in the C-heap (managed by: free & malloc).
// - GlobalObj
//
// For objects allocated on the stack.
// - StackObj
//
// For classes used as name spaces.
// - AllStatic
//
// The following macros and should be used to allocate arrays
// directly in the global heap:
//
//   NEW_GLOBAL_HEAP_ARRAY(type,size)
//   FREE_GLOBAL_HEAP_ARRAY(pointer)
//
// Global heap allocation can be traced using +PrintMallocFree.
// malloc and free should therefore never called directly.

#define NEW_GLOBAL_HEAP_ARRAY(type, size, what) \
  ((type*) (GlobalObj::malloc_bytes((size) * sizeof(type))))

#define FREE_GLOBAL_HEAP_ARRAY(p, what) \
  (GlobalObj::free_bytes((void*) p))

// Base class for objects allocated in the global C heap

class GlobalObj {
public:
  static void* malloc_bytes(size_t size) {
    void* result = OsMemory_allocate(size);
    return result;
  }

  static void free_bytes(void* p) {
    OsMemory_free(p);
  }

  void* operator new(size_t size) {
    return malloc_bytes(size);
  }
  void operator delete(void* p) {
    free_bytes(p);
  }
};

// Base class for objects allocated on the stack only.
// Calling new or delete will result in fatal error.

class StackObj {
 public:
  void* operator new(size_t /*size*/) {
    SHOULD_NOT_REACH_HERE();
    return (void *)0;
  }
  void operator delete(void* /*p*/) {
    SHOULD_NOT_REACH_HERE();
  }
};

// Base class for classes that constitute name spaces.

class AllStatic {
 public:
  void* operator new(size_t /*size*/) {
    SHOULD_NOT_REACH_HERE();
    return (void *)0;
  }
  void operator delete(void* /*p*/) {
    SHOULD_NOT_REACH_HERE();
  }
};
