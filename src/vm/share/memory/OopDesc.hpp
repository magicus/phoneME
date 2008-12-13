/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

// An OopDesc is the abstract class for all object in the heap.
// All they have in common is the klass field.

// Write barrier support -- You must call either oop_write_barrier() or
// oop_write_barrier_from_young_space() when storing a pointer in an
// object in the java heap. Do not write the pointer directly.

#ifndef PRODUCT
bool oop_check_barrier(OopDesc** addr);
bool oop_in_old_space(OopDesc** addr);
#endif

// Call this if addr is guaranteed to be in the young space
inline void oop_write_barrier_for_young_obj(OopDesc** addr, OopDesc* value)
{
  GUARANTEE(!oop_in_old_space(addr), "must not be in old space");
  *addr = value;
}

// Call this if addr is not guaranteed to be in the young space
void oop_write_barrier(OopDesc** addr, OopDesc* value);

class OopDesc {
public:
  bool is_rom_symbol() const;

  bool is_rom_method() const;


  // Accessors for klass
#ifndef PRODUCT
  OopDesc* klass() const;
#else
  OopDesc* klass() const { return _klass; }
#endif

  void initialize(OopDesc* klass) {
    GUARANTEE(klass != NULL, "Cannot set class of object to NULL");
    oop_write_barrier_for_young_obj(&this->_klass, klass);
  }

  void reinitialize(OopDesc* klass) {
    GUARANTEE(klass != NULL, "Cannot set class of object to NULL");
    oop_write_barrier(&this->_klass, klass);
  }
  
  FarClassDesc* blueprint() const { 
    return (FarClassDesc*) ((void*)klass()->klass());
  }

  // GC support.
  size_t object_size() const {
    return object_size_for(blueprint());
  }
  inline void oops_do(void do_oop(OopDesc**)) {
    oops_do_for(blueprint(), do_oop);
  }
  inline void oops_do_inline(void do_oop(OopDesc**)) {
    oops_do_for(blueprint(), do_oop);
  }

  inline void near_do(void do_oop(OopDesc**));  // Visits near pointer only
  inline void map_oops_do(jubyte* map, void do_oop(OopDesc**));
  void* field_base(int offset) const { return (void*)&((char*)this)[offset];  }

  // simple type tests:
  inline bool is_instance() const;
  inline bool is_bool_array() const;
  inline bool is_char_array() const;
  inline bool is_int_array() const;
  inline bool is_byte_array() const;
  inline bool is_short_array() const;
  inline bool is_long_array() const;
  inline bool is_float_array() const;
  inline bool is_double_array() const;
  inline bool is_obj_array() const;
  inline bool is_type_array() const;
  inline bool is_string() const;
  inline bool is_jvm_thread() const;
  inline bool is_throwable() const;

  inline bool is_compiled_method() const;
  inline bool is_symbol() const;
  inline bool is_method() const;
  inline bool is_constant_pool() const;
  inline bool is_entry_activation() const;
  inline bool is_execution_stack() const;
  inline bool is_class_parser_state() const;
  inline bool is_condition() const;
  inline bool is_entry() const;
  inline bool is_compilation_queue_element() const;
  inline bool is_mixed_oop() const;
  inline bool is_task() const;
  inline bool is_boundary() const;
  inline bool is_virtual_stack_frame() const;
  inline bool is_class_info() const;
  inline bool is_stackmap_list() const;

  inline bool is_instance_class() const;
  inline bool is_type_array_class() const;
  inline bool is_obj_array_class() const;
  inline bool is_method_class() const;

  inline bool is_obj_near() const;
  inline bool is_java_near() const;
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  inline bool is_generic_near() const;
  inline bool is_near() const;
  inline bool is_far() const;
#endif

  inline bool is_task_mirror() const;
  inline bool is_thread() const;

  inline bool is_file_decoder() const;
  inline bool is_inflater() const;  
  inline bool is_jar_file_parser() const;  

  // field address computation:
  OopDesc** obj_field_addr(int offset) const {
    return (OopDesc**)&((char*)this)[offset];
  }
  jbyte* byte_field_addr(int offset) const {
    return (jbyte*)&((char*)this)[offset];
  }
  jchar* char_field_addr(int offset) const {
    return (jchar*)&((char*)this)[offset];
  }
  jboolean* bool_field_addr(int offset) const {
    return (jboolean*)&((char*)this)[offset];
  }
  jint* int_field_addr(int offset) const {
    return (jint*)&((char*)this)[offset];
  }
  jshort* short_field_addr(int offset) const {
    return (jshort*)&((char*)this)[offset];
  }
  jlong* long_field_addr(int offset) const {
    return (jlong*)&((char*)this)[offset];
  }
  jfloat* float_field_addr(int offset) const {
    return (jfloat*)&((char*)this)[offset];
  }
  jdouble* double_field_addr(int offset) const {
    return (jdouble*)&((char*)this)[offset];
  }
  jubyte* ubyte_field_addr(int offset) const {
    return (jubyte*)&((char*)this)[offset];
  }
  juint* uint_field_addr(int offset) const {
    return (juint*)&((char*)this)[offset];
  }
  jushort* ushort_field_addr(int offset) const {
    return (jushort*)&((char*)this)[offset];
  }
  julong* ulong_field_addr(int offset) const {
    return (julong*)&((char*)this)[offset];
  }

private:
  OopDesc* _klass;
#if ENABLE_OOP_TAG
  // Useful for extreme debugging of objects.  Each object can get
  // some identifier applied (see Oop.hpp).  In one implementation
  // used to track which task owns which objects.  When a task is
  // created a sequence number is stored for that task and all objects
  // created in that task have that sequence number.  You can then do stuff
  // like:
  //  if (do_oop == ObjectHeap::mark_and_push) {
  //    // see if we're marking outside of our task
  //    Method::Raw m = this;
  //    int tag = m.oop_tag();
  //    int task_id = tag & Oop::TASK_ID_MASK;
  //    int seq_num = tag >> Oop::TASK_SEQ_SHIFT;
  //    Task::Raw t = Universe::task_from_id(task_id);
  //    if (t.is_null() || t().seq() != seq_num) {
  //      BREAKPOINT;
  //    }
  //  }
  //
  int      _oop_tag;
#endif
  size_t object_size_for(const FarClassDesc* blueprint) const;
  void oops_do_for(const FarClassDesc* blueprint, void do_oop(OopDesc**));

  friend class BasicOop;
  friend class LinkedBasicOop;
  friend class ObjectHeap;
  friend class Oop;
  friend class ROM;
  friend class ROMBundle;
  friend class StackLock;
#if USE_LARGE_OBJECT_AREA
  friend class LargeObject;
#endif
#if ENABLE_TRAMPOLINE
  friend class BranchTable;
#endif
};
