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

class ExecutionStackDesc: public OopDesc { 
 public:
  static jint header_size() { return sizeof(ExecutionStackDesc); }

  // Computes the allocation size
  static size_t allocation_size(int length) {
    // length in bytes
    return align_allocation_size(header_size() + length);
  }
  
  // Returns the object size (in bytes)
  size_t object_size() { return allocation_size(_length); }

  // GC support
 void relocate_all_pointers(int delta, void do_oop(OopDesc**));
 void relocate_internal_pointers(int delta, Thread* thread,
                                  bool do_locks = false);

  void variable_oops_do(void do_oop(OopDesc**)) {
    Thread::Raw thread = _thread;
    if (thread.not_null()) { 
      // GC handles &_next_stack
      do_oop(&_thread);
      thread().stack_oops_do(do_oop);
    }
  }

 private:
  // Initializes the object after allocation
  void initialize(OopDesc* klass, jint length) {
    OopDesc::initialize(klass);
    _length = length;
    _next_stack = _stack_list;
    _stack_list = this;
  }

  // Returns true if the address is within this stack.
  bool contains(address ptr) {
    juint t = (juint) this;
    juint p = (juint) ptr;
    return ((p - t) < (juint)object_size());
  }
  
  // update stack list replacing old_stack with new_stack
  // old_stack_next argument needed as content of old_stack is corrupted
  static void update_list(ExecutionStackDesc* old_stack,
			  ExecutionStackDesc* old_stack_next,
			  ExecutionStackDesc* new_stack);

  jint                _length;
  OopDesc*            _thread;
  ExecutionStackDesc* _next_stack;  // GC used only

  static  ExecutionStackDesc* _stack_list;  // used only by GC;

  friend class Universe;
  friend class ExecutionStack;
  friend class ObjectHeap;
};
