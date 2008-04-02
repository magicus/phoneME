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

class TaskMirror : public Oop {
public:
  HANDLE_DEFINITION(TaskMirror, Oop);

  static int real_java_mirror_offset() {
    return FIELD_OFFSET(TaskMirrorDesc, _real_java_mirror);
  }

  static int containing_class_offset() {
    return FIELD_OFFSET(TaskMirrorDesc, _containing_class);
  }

  // IMPL_NOTE: Object size can be deduced from statics size and vtable length
  // of the class being mirror. Since a TaskMirror is always
  // allocated with a java mirror object, that means it is possible to
  // always get the size of static variables and vtable length
  static int object_size_offset() {
    return FIELD_OFFSET(TaskMirrorDesc, _object_size);
  }

  // IMPL_NOTE: remove this and replace with an exogen list to reduce space overhead of 
  // Task mirror
  static int next_in_clinit_list_offset(){
    return FIELD_OFFSET(TaskMirrorDesc, _next_in_clinit_list);
  }

  static int array_class_offset() {
    return FIELD_OFFSET(TaskMirrorDesc, _array_class);
  }

  static int init_thread_offset() {
    return FIELD_OFFSET(TaskMirrorDesc, _init_thread);
  }

  ReturnOop next_in_clinit_list( void ) const {
    return obj_field(next_in_clinit_list_offset());
  }
  void clear_next_in_clinit_list( void ) {
    obj_field_clear(next_in_clinit_list_offset());
  }
  void set_next_in_clinit_list(Oop* next) {
    obj_field_put(next_in_clinit_list_offset(), next);
  }
  void set_next_in_clinit_list(OopDesc* next) {
    obj_field_put(next_in_clinit_list_offset(), next);
  }

  ReturnOop real_java_mirror() {
    return obj_field(real_java_mirror_offset());
  }
  void set_real_java_mirror(Oop *o) {
#ifdef AZZERT
    GUARANTEE(o->obj() != obj(), "setting real_java_mirror to self");
#endif
    obj_field_put(real_java_mirror_offset(), o);
  }

  ReturnOop containing_class() {
    return obj_field(containing_class_offset());
  }

  void set_containing_class(Oop *o) {
    obj_field_put(containing_class_offset(), o);
  }

  // Static variables are embedded in the mirror object
  // Returns the offset (in bytes) where the static field section
  // starts and ends.
  static int static_field_start() {
    return sizeof(TaskMirrorDesc);
  }

  int object_size() {
    return int_field(object_size_offset());
  }
  void set_object_size(jint value) {
    int_field_put(object_size_offset(), value);
  }

  ReturnOop array_class() {
    return obj_field(array_class_offset());
  }
  void set_array_class(Oop *ac) {
    obj_field_put(array_class_offset(), ac);
  }

  ReturnOop init_thread() {
    return obj_field(init_thread_offset());
  }
  void set_init_thread(Oop *ac) {
    obj_field_put(init_thread_offset(), ac);
  }
  void clear_init_thread() {
    obj_field_clear(init_thread_offset());
  }

  bool is_being_initialized_mirror() {
    return TaskMirrorDesc::is_being_initialized_mirror((TaskMirrorDesc*)obj());
  }

  static ReturnOop clinit_list_lookup(const Oop *containing_class);
  static ReturnOop clinit_list_lookup(TaskMirror *found,
                                      const Oop *containing_class);

  static void clinit_list_add(TaskMirror *task_class_mirror);

  static ReturnOop clinit_list_remove(Oop *containing_class);

  static void clinit_list_add(Task *task, TaskMirror *list, TaskMirror *tm);

  static ReturnOop clinit_list_remove(TaskMirror *list, Oop *containing_class);

  static int header_size() { return sizeof(TaskMirrorDesc); }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

  void iterate(OopVisitor* /*visitor*/) PRODUCT_RETURN;
  static void iterate_oopmaps(oopmaps_doer /*do*/, void* /*parm*/) PRODUCT_RETURN;
  ReturnOop find_owner(void) PRODUCT_RETURN0;
};
