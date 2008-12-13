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

//

class TaskMirrorDesc : public OopDesc {
public:
  // The InstanceClassDesc contains MaxIsolate's pointers to 
  // TaskMirrorDesc's. 
  //
  
  // Support for class initialization barriers
  // NEW CLASS INITIALIZATION BARRIER:
  // Entries of the TaskMirrorDesc table of InstanceClassDesc can have one
  // of the three value:
  // - NULL : when the class isn't initialized for the corresponding task
  // - *Universe::being_initialized_marker(): when the class is being
  //   initialized. The actual TaskMirrorDesc for the class being
  //   initialized is in the clinit_list of the thread doing the
  //   initialization.
  // - An instance of TaskMirrorDesc with an
  //   address != *Universe::task_class_init_marker()
  //   In this case, the class is initialized and the TaskMirrorDesc at
  //   this entry is the one holding the isolate-dependent state of the
  //   class for the current isolate.

  static bool is_initialized_mirror(TaskMirrorDesc *tmd){
    return (tmd != NULL && tmd != _task_class_init_marker);
  }

  static bool is_being_initialized_mirror(TaskMirrorDesc *tmd){
    return (tmd == (TaskMirrorDesc *)_task_class_init_marker);
  }

  size_t object_size() {
    return _object_size;
  }

  void variable_oops_do(void do_oop(OopDesc **));

protected:
  static jint header_size() { return sizeof(TaskMirrorDesc); }

private:

  static size_t allocation_size(jint statics_size, jint vtable_length) {
    size_t size = header_size() + statics_size;

#if USE_EMBEDDED_VTABLE_BITMAP
    size += bitmap_size(vtable_length);
#else
    (void)vtable_length;
#endif

    return align_allocation_size(size);
  }

  void initialize(OopDesc *klass, jint statics_size, jint vtable_length) {
    OopDesc::initialize(klass);
    _object_size = allocation_size(statics_size, vtable_length);
  }

  InstanceDesc*         _real_java_mirror;
  InstanceClassDesc*    _containing_class;
  jint                  _object_size;
  TaskMirrorDesc *      _next_in_clinit_list;
  // ptr to FarClassDesc of class holding elements of our containing class
  FarClassDesc*         _array_class;
  ThreadDesc*           _init_thread;

  // Here starts the static fields
#if USE_EMBEDDED_VTABLE_BITMAP
  // Here starts vtable methods bitmap
#endif
  // <-- END OF DATA STRUCTURE

  // It's very important that no other fields are stored here,
  // otherwise it will complicate the bitvector cleaning code in
  // ROMOptimizer::compact_static_field_containers().

  friend class Universe;
  friend class TaskMirror;
  friend class FarClassDesc;
  friend class OopDesc;
};
