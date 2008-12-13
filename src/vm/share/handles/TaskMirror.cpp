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

# include "incls/_precompiled.incl"
# include "incls/_TaskMirror.cpp.incl"

#if ENABLE_ISOLATES

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int TaskMirror::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  // near
  map_index = Near::generate_fieldmap(field_map);

  // _real_java_mirror
  field_map->byte_at_put(map_index++, T_OBJECT);
  // _containing_class
  field_map->byte_at_put(map_index++, T_OBJECT);
  // _object_size_offset
  field_map->byte_at_put(map_index++, T_INT);
  // _next_in_clinit_list
  field_map->byte_at_put(map_index++, T_OBJECT);
  // _array_class
  field_map->byte_at_put(map_index++, T_OBJECT);
  //_init_thread
  field_map->byte_at_put(map_index++, T_OBJECT);

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */

ReturnOop TaskMirror::clinit_list_lookup(const Oop *containing_class){
  TaskMirror::Raw found = Task::current()->clinit_list();
  return clinit_list_lookup(&found, containing_class);
}

ReturnOop TaskMirror::clinit_list_lookup(TaskMirror *found,
                                         const Oop *containing_class) {
  while(found->not_null()){
    if (containing_class->equals(found->containing_class())){
      return found->obj();
    }
    *found = found->next_in_clinit_list();
  }
  return NULL;
}

void TaskMirror::clinit_list_add(TaskMirror *tm){
  TaskMirror::Raw list = Task::current()->clinit_list();
  clinit_list_add(Task::current(), &list, tm);
}

void TaskMirror::clinit_list_add(Task *task, TaskMirror *list,
                                 TaskMirror *tm) {
  if (Verbose) {
    tty->print_cr("TaskM adding: tm: 0x%x, ic: 0x%x id: 0x%x, thrd: 0x%x, "
                  "task: %d",
                  (int)tm->obj(), (int)tm->containing_class(),
                  ((JavaClass)(tm->containing_class())).class_id(),
                  (int)Thread::current()->obj(), TaskContext::current_task_id());
  }
  TaskMirror::Raw new_head = tm;
  TaskMirror::Raw old_head = list->obj();
  new_head().set_next_in_clinit_list(old_head());
  task->set_clinit_list(new_head);
}

ReturnOop TaskMirror::clinit_list_remove(Oop *containing_class){
  // first, locate the mirror in the list and remove it if found
  Task::Raw task = Task::current()->obj();
  TaskMirror::Raw list = task().clinit_list();
  Oop::Raw cc = containing_class;
  TaskMirror::Raw prev;
  TaskMirror::Raw found = list.obj();
  TaskMirror::Raw next;
  GUARANTEE(found.not_null(), "clinit list must not be null");
  do {
    if (cc == found().containing_class()){
      break;
    }
    prev = found;
    found = found().next_in_clinit_list();
  } while(found.not_null());
  GUARANTEE(found.not_null(), "element must be found in clinit list");
  next = found().next_in_clinit_list();
  found().clear_next_in_clinit_list();
  if( prev.not_null() ) {
    prev().set_next_in_clinit_list(next);
  } else {
    task().set_clinit_list(next);
  }
  if (Verbose) {
    tty->print_cr("TaskM: reming tm: 0x%x, ic: 0x%x id: 0x%x, thrd: 0x%x, "
                  "task: %d",
                  (int)found().obj(), (int)found().containing_class(),
                  ((JavaClass)(found().containing_class())).class_id(),
                  (int)Thread::current()->obj(), TaskContext::current_task_id());
  }
  found().clear_init_thread();
  return found.obj();
}

#ifndef PRODUCT

ReturnOop TaskMirror::find_owner() {
  JavaClassObj::Raw m = real_java_mirror();
  if (m.not_null()) {
    JavaClass::Raw java_class = m().java_class();
    if (java_class.not_null()) {
      return java_class.obj();
    }
  }

  for (SystemClassStream st(true); st.has_next();) {
    InstanceClass::Raw klass = st.next();
    int class_id = klass().class_id();
    ObjArray::Raw task_list = Universe::task_list();
    // Task 0 reserved for system use
    for (int i = Task::FIRST_TASK; i<task_list().length(); i++){
      Task::Raw t = task_list().obj_at(i);
      if (t.not_null()) {
        ObjArray::Raw ml = t().mirror_list();
        TaskMirror::Raw tm = ml().obj_at(class_id);
        if (tm.not_null() && tm.equals(this)) {
          return klass.obj();
        }
      }
    }
  }
  return NULL;
}

void TaskMirror::iterate(OopVisitor* visitor) {
  Oop::iterate(visitor);
  {
    NamedField id("real_java_mirror", true);
    visitor->do_oop(&id, real_java_mirror_offset(), true);
  }

  {
    NamedField id("containing_class", true);
    visitor->do_oop(&id, containing_class_offset(), true);
  }

  {
    NamedField id("object_size_offset", true);
    visitor->do_int(&id, object_size_offset(), true);
  }

  {
    NamedField id("next_in_clinit_list", true);
    visitor->do_oop(&id, next_in_clinit_list_offset(), true);
  }

  {
    NamedField id("array_class", true);
    visitor->do_oop(&id, array_class_offset(), true);
  }

  {
    NamedField id("init_thread", true);
    visitor->do_oop(&id, init_thread_offset(), true);
  }

  if (equals(Universe::task_class_init_marker())) {
    // IMPL_NOTE: the logic with find_owner() is incorrect and we will have error 
    // inside static field printing. Luckily, task_class_init_marker
    // has no static fields.
    visitor->do_comment("task_class_init_marker, no static fields");
    return;
  }

  /*
   * Print the static fields
   */

  JavaClass::Raw o = find_owner();
  if (o.is_null()) {
    visitor->do_comment("Owner not found. Static fields unknown");
    return;
  }
  if (!o.is_instance_class()) {
    return;
  }

  InstanceClass::Raw owner = o.obj();
  bool has_renamed_fields = (ROM::get_original_fields(&owner) != NULL);

  // Print static fields defined by local class.
  // Static fields are already sorted by offset.
  if (has_renamed_fields) {
    visitor->do_comment("Static fields (++: renamed fields)");
  } else {
    visitor->do_comment("Static fields");
  }

  TypeArray::Raw fields = owner().original_fields();
  if (fields.is_null()) {
    return;
  }
  int index=0;
  for (index = 0; index < fields().length(); index += 5) {
    OriginalField f(&owner, index);
    if (f.is_static()) {
      Symbol name = f.name();
      SymbolField field(&name, false);
      field.set_renamed(f.is_renamed());

      switch(f.type()) {
      case T_BOOLEAN: visitor->do_bool(&field, f.offset(), true);   break;
      case T_CHAR:    visitor->do_char(&field, f.offset(), true);   break;
      case T_FLOAT :  visitor->do_float(&field, f.offset(), true);  break;
      case T_DOUBLE:  visitor->do_double(&field, f.offset(), true); break;
      case T_BYTE:    visitor->do_byte(&field, f.offset(), true);   break;
      case T_SHORT:   visitor->do_short(&field, f.offset(), true);  break;
      case T_INT:     visitor->do_int(&field, f.offset(), true);    break;
      case T_LONG:    visitor->do_long(&field, f.offset(), true);   break;
      case T_OBJECT:  visitor->do_oop(&field, f.offset(), true);    break;
      case T_ARRAY:   visitor->do_oop(&field, f.offset(), true);    break;
      }
    }
  }
}

void TaskMirror::iterate_oopmaps(oopmaps_doer do_map, void *param) {
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, real_java_mirror);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, containing_class);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next_in_clinit_list);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, array_class);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, init_thread);
}

#endif  // !PRODUCT
#endif  // ENABLE_ISOLATES
