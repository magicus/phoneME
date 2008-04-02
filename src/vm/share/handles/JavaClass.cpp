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

# include "incls/_precompiled.incl"
# include "incls/_JavaClass.cpp.incl"

HANDLE_CHECK(JavaClass, is_java_class())

bool JavaClass::check_access_by(InstanceClass* sender_class, 
                                FailureMode fail_mode JVM_TRAPS) {
  if (sender_class->access_flags().is_preloaded()) {
    // We're checking access on a non-quickened bytecode that lives in
    // ROM. The check was already done during romization. No need to
    // repeat. In addition, the check may fail if the sender_class was
    // renamed.
    return true;
  }

  GUARANTEE(!sender_class->is_renamed(), "must not be renamed by converter");

  // check that no access to hidden classes from user classes take place
  if (!sender_class->is_preloaded() && this->is_hidden()) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);   
  }

  if (is_obj_array_class()) {
    ObjArrayClass* obj_array_class = (ObjArrayClass*)this;
    return obj_array_class->check_access_by(sender_class, fail_mode
                                            JVM_NO_CHECK_AT_BOTTOM);
  } else if (!is_type_array_class()) {
    GUARANTEE(is_instance_class(), "must be instance class");
    InstanceClass* instance_class = (InstanceClass*)this;
    GUARANTEE(!instance_class->is_renamed(),
              "must not be renamed by converter");
    return instance_class->check_access_by(sender_class, fail_mode
                                           JVM_NO_CHECK_AT_BOTTOM);
  }
  return true;
}

ReturnOop JavaClass::new_initialized_instance(InstanceClass* sender_class,
                                              Thread *thread
                                              JVM_TRAPS) {
  if (!is_instance_class() || is_abstract() || is_interface()) {
    Throw::instantiation(ExceptionOnFailure JVM_THROW_0);
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast instance_class = this;
  bool status = instance_class().check_access_by(sender_class,
                                                 ExceptionOnFailure 
                                                 JVM_ZCHECK_0(status));

  Method::Fast init = 
      instance_class().find_local_method(Symbols::object_initializer_name(),
                                         Symbols::void_signature());
  if (init.is_null()) {
    Throw::instantiation(ExceptionOnFailure JVM_THROW_0);
  }

  InstanceClass::Fast holder_class = init().holder();
  if (!holder_class().equals(this)) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
  }

  init().check_access_by(sender_class, sender_class, ExceptionOnFailure
                         JVM_CHECK_0);

  // Special test for JLS 6.6.2
  if (init().is_protected() &&
      !instance_class().is_same_class_package(sender_class)) {
    Throw::illegal_access(ExceptionOnFailure JVM_THROW_0);
  }

  Instance::Fast instance =
      instance_class().new_instance(ExceptionOnFailure JVM_CHECK_0);

  // Delay invokation of instance initialization:
  EntryActivation::Fast entry = Universe::new_entry_activation(&init, 1
                                                               JVM_CHECK_0);
  entry().obj_at_put(0, &instance);
  thread->append_pending_entry(&entry);

  return instance;
}

ReturnOop JavaClass::class_loader() {
  return NULL;
}

#if ENABLE_ISOLATES
ReturnOop JavaClass::real_task_mirror() const { 
  TaskMirror::Raw tm = task_mirror_no_check();
  if (is_being_initialized_mirror(&tm)) {
    tm = TaskMirror::clinit_list_lookup(this);
    GUARANTEE(!tm.is_null(), "Task mirror must be non-null during <clinit>");
  }
  return tm.obj();
}

ReturnOop JavaClass::array_class() { 
  // First check to see if the array_class field holds the arrayclass
  // If not, then check for a TaskMirror
  Oop::Raw o = obj_field(array_class_offset());
  if (!o.is_null()) {
    return o.obj();
  }
  // get task mirror for this class
  TaskMirror::Raw tm = task_mirror_no_check();
  // get task mirror for array class that has elements of *this* class
  if (is_being_initialized_mirror(&tm)) {
    // return real array class
    // class is being initialized.
    tm = TaskMirror::clinit_list_lookup(this);
    if (tm.is_null()) {
      return NULL;
    }
  }
  return tm().array_class();
}

void JavaClass::set_array_class(ObjArrayClass* value JVM_TRAPS) {
  // If arrayclass AND element class are both in ROM or both out of
  // ROM then we can use arrayclass field in JavaClass to store arrayclass
  if ((GenerateROMImage && !ENABLE_MONET)
      ||
      (UseROM &&
       ((class_id() < ROM::number_of_system_classes() &&
         value->class_id() < ROM::number_of_system_classes())
        ||
        (class_id() >= ROM::number_of_system_classes() && 
         value->class_id() >= ROM::number_of_system_classes())))) {
    obj_field_put(array_class_offset(), value);
  } else {
    // set arrayclass for this class
    TaskMirror::Raw tm = task_mirror_no_check();
    GUARANTEE(!tm.is_null(), "Task mirror is null");
    if (is_being_initialized_mirror(&tm)) {
      // class is being initialized.
      tm = TaskMirror::clinit_list_lookup(this);
      if (is_instance_class()) {
        GUARANTEE(!tm.is_null(), "Task mirror is null");
      } else {
        if (tm.is_null()) {
          // 'this' is an array class, ok to allocate mirror now
          setup_task_mirror(0, 0, false JVM_CHECK);
          tm = task_mirror_no_check();
          GUARANTEE(!tm.is_null(), "Task mirror is null");
        }
      }
    }
    tm().set_array_class(value);
  }
}
#endif //ENABLE_ISOLATES

ReturnOop JavaClass::get_array_class(jint distance JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArrayClass::Fast ac = array_class();
  ObjArrayClass::Fast tmp;
  if (ac.is_null()) {
#if ENABLE_ISOLATES
    // THe logic is as follows, if generating the System ROM then don't create
    // TaskMirrors for arrayclasses as the arrayclass and element class
    // are both being ROMized.
    // If we are just running the VM, then if there is no ROM, use a
    // TaskMirror.  If 'this' is in ROM then we have the element class in
    // ROM but we will create arrayclass in this new task so we need a
    // TaskMirror.
    // If both element class (this) and arrayclass (being created below) are
    // not in ROM then we don't need TaskMirror
    if (!(GenerateROMImage && !ENABLE_MONET) &&
        (!UseROM || (class_id() < ROM::number_of_system_classes())))  {
      TaskMirror::Raw tm = task_mirror_no_check();
      if (is_being_initialized_mirror(&tm)) {
        if (is_instance_class()) {
          tm = setup_task_mirror(static_field_size(), vtable_length(),
                                 true JVM_CHECK_0);
          InstanceClass::Raw ic = obj();
          ic().initialize_static_fields(&tm);
        } else {
          tm = setup_task_mirror(0, 0, false JVM_CHECK_0);
        }
      }
    }
#endif
    ac = Universe::new_obj_array_class(this JVM_CHECK_0);
    set_array_class(&ac JVM_CHECK_0);
    if (_debugger_active) {
      // With fixes to ClassBySig in VMImpl.cpp we don't need to do this
      //      VMEvent::class_prepare_event(&ac);
    }
  }
  return ac().get_array_class(distance JVM_NO_CHECK_AT_BOTTOM_0);
}

#if ENABLE_ISOLATES

ReturnOop JavaClass::java_mirror() {
  TaskMirror::Raw tm = task_mirror_desc();

  GUARANTEE(!tm.is_null(), "cannot be null");
  return tm().real_java_mirror();
}

ReturnOop JavaClass::get_or_allocate_java_mirror(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = obj();
  TaskMirror::Fast tm = task_mirror_desc(); // IMPL_NOTE: can be RAW
  if (is_being_initialized_mirror(&tm)) {
    tm = TaskMirror::clinit_list_lookup(&ic);
    if (tm.is_null()) {
      // let's init this class for this task
      if (is_instance_class()) {
        tm = setup_task_mirror(ic().static_field_size(), 
                               ic().vtable_length(), true JVM_ZCHECK_0(tm));
        ic().initialize_static_fields(&tm);
      } else {
        tm = setup_task_mirror(0, 0, false JVM_ZCHECK_0(tm));
      }
      GUARANTEE(!tm.is_null(), "Task mirror cannot be null");
    }
  }
  JavaClassObj::Raw m = tm().real_java_mirror();
  GUARANTEE(!m.is_null(), "Mirror cannot be null");
  return m;
}

// Initialize the task mirror entry for the task and setup a java mirror
// for the class.
ReturnOop JavaClass::setup_task_mirror(int statics_size,
                                       int vtable_length, 
                     bool set_initializing_barrier JVM_TRAPS) {
  if (Universe::java_lang_Class_class()->is_null()) {
    return NULL;
  }
  UsingFastOops fast_oops;
  TaskMirror::Fast tm = 
    Universe::new_task_mirror(statics_size, vtable_length JVM_CHECK_0);
  JavaClassObj::Fast m = 
      Universe::new_instance(Universe::java_lang_Class_class() JVM_CHECK_0);
  m().set_java_class(this);
  InstanceClass::Fast ic = this;

  tm().set_real_java_mirror(&m);
  tm().set_containing_class(&ic);
  if (set_initializing_barrier) {
#ifdef AZZERT
    TaskMirror::Raw tm0 = (OopDesc*)task_mirror_desc();
    GUARANTEE(is_being_initialized_mirror(&tm0()),
              "Should be task mirror initializing marker");
#endif
    TaskMirror::clinit_list_add(&tm);
    tm().set_init_thread(Thread::current());
  } else {
    set_task_mirror(&tm);
  }
  return tm.obj();
}

#else

ReturnOop JavaClass::java_mirror() {
  ReturnOop result = obj_field(java_mirror_offset());

  GUARANTEE(result != NULL, "cannot be null");
  return result;
}

void JavaClass::setup_java_mirror(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  GUARANTEE(obj_field(java_mirror_offset()) == NULL, "mirror must be null");
  // java.lang.Class is not loaded when setup_java_mirror is called
  // for the initial classes (ex. java.lang.Object). During
  // this part of bootstrapping setup_java_mirror is ignored.
  // Fixup of these classes takes places after java.lang.Class is loaded.
  if (Universe::java_lang_Class_class()->is_null()) {
    return;
  }
  JavaClassObj::Fast m = 
      Universe::new_instance(Universe::java_lang_Class_class() JVM_CHECK);
  m().set_java_class(this);
  obj_field_put(java_mirror_offset(), &m);
}

#endif

bool JavaClass::compute_and_set_is_subtype_of(JavaClass* other_class) {
  bool result = compute_is_subtype_of(other_class);
  if (result) {
    set_subtype_cache_2(subtype_cache_1());
    set_subtype_cache_1(other_class);
  }
  return result;
}

bool JavaClass::is_subtype_of(JavaClass* other_class) {
  JavaClass::Raw a = subtype_cache_1();
  JavaClass::Raw b = subtype_cache_2();
  return a.equals(other_class) // Fast check
      || b.equals(other_class) // Fast check
      // Computes the result and fills the cache
      || compute_and_set_is_subtype_of(other_class);
}

bool JavaClass::is_subclass_of(JavaClass* other_class) {
  // This is a hot loop, so we're using raw pointers here to help C++
  // compiler generate better code. If a GC happens we're
  // in deep trouble!

  AllocationDisabler raw_pointers_used_in_this_function;

  JavaClassDesc *current = (JavaClassDesc *)this->obj();
  JavaClassDesc *other   = (JavaClassDesc *)other_class->obj();

  do {
    if (other == current) {
      return true;
    }
    current = (JavaClassDesc*)current->_super;
  } while (current != NULL);

  return false;
}

bool JavaClass::compute_is_subtype_of(JavaClass* other_class) {
  if (equals(other_class)) {
    return true;
  }

  if (is_instance_class()) {
    InstanceClass* ic = (InstanceClass*)this;
    return ic->compute_is_subtype_of(other_class);
  }

  if (is_obj_array_class()) {
    ObjArrayClass *oc = (ObjArrayClass*)this;
    return oc->compute_is_subtype_of(other_class);
  }
  GUARANTEE(is_type_array_class(), "Sanity");
  {
    TypeArrayClass* tc = (TypeArrayClass*)this;
    return tc->compute_is_subtype_of(other_class);
  }
}

int JavaClass::copy_super_vtable() {
  InstanceClass::Raw s = super();
  if (s.is_null()) {
    return 0;
  }

  ClassInfo::Raw myinfo = class_info();
  ClassInfo::Raw sinfo  = s().class_info();

  // copy methods from super
  const int length = sinfo().vtable_length();
  GUARANTEE(length <= myinfo().vtable_length(), "vtable too short");
  (void)length;

  for (int index = 0; index < length; index++) {
    myinfo().vtable_at_put(index, sinfo().vtable_method_at(index));
  }
  return length;
}

void JavaClass::initialize_vtable() {
  int super_length = copy_super_vtable();

  if (is_instance_class()) {
    InstanceClass::Raw c = obj();
    c().update_vtable(super_length);
  }
}

JavaClass::AccessType JavaClass::vtable_accessibility_at(int index) {
  ClassInfo::Raw info = class_info();

  // This vtable is not implementing the specific method
  if (index >= info().vtable_length()) {
    return acc_private;
  }

  // Compute AccessType for current method. public or protected we are done.
  Method::Raw m = info().vtable_method_at(index);
  if (m().is_protected() || m().is_public()) {
    return acc_publicprotected;
  }

  AccessType acc = m().is_package_private() ? acc_package_private : acc_private;

  // Compute AccessType for method in super classes
  InstanceClass::Raw s = super();
  AccessType super_acc = !s.is_null()
                         ? s().vtable_accessibility_at(index)
                         : acc_private;

  // Merge
  return (AccessType)max((int)acc, (int)super_acc);
}

#ifndef PRODUCT

void JavaClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  FarClass::iterate(visitor);

  {
    NamedField id("class_info", true);
    visitor->do_oop(&id, class_info_offset(), true);
  }

  {
    NamedField id("subtype_cache_1", true);
    visitor->do_oop(&id, subtype_cache_1_offset(), true);
  }

  {
    NamedField id("subtype_cache_2", true);
    visitor->do_oop(&id, subtype_cache_2_offset(), true);
  }
  {
    NamedField id("array_class", true);
    visitor->do_oop(&id, array_class_offset(), true);
  }

#if !ENABLE_ISOLATES
  {
    NamedField id("java_mirror", true);
    visitor->do_oop(&id, java_mirror_offset(), true);
  }
#endif

  {
    NamedField id("super", true);
    visitor->do_oop(&id, super_offset(), true);
  }
#endif
}

#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE ||\
                         ENABLE_PERFORMANCE_COUNTERS
void JavaClass::print_name_on(Stream* st) {
  ClassInfo::Raw info = class_info();
  info().print_name_on(st);
}
#endif

#if !defined(PRODUCT)

void JavaClass::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  FarClass::iterate_oopmaps(do_map, param);

  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, class_info);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, subtype_cache_1);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, subtype_cache_2);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, array_class);
#if !ENABLE_ISOLATES
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, java_mirror);
#endif
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, super);
#endif
}

#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int JavaClass::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  // Far Class fields
  map_index = FarClass::generate_fieldmap(field_map);
  int needed = map_index + 8;

  if (needed > field_map->length()) {
    // Call me again with the needed field size.
    return needed;
  }

  // _class_info
  field_map->byte_at_put(map_index++, T_OBJECT); 

  // _subtype_cache_1
  field_map->byte_at_put(map_index++, T_OBJECT); 

  // _subtype_cache_2
  field_map->byte_at_put(map_index++, T_OBJECT); 


  // _array_class
  field_map->byte_at_put(map_index++, T_OBJECT);
#if !ENABLE_ISOLATES
  // _java_mirror
  field_map->byte_at_put(map_index++, T_OBJECT);
#endif

  // _super
  field_map->byte_at_put(map_index++, T_OBJECT); 

  // aliased field: _next / _element_class
  field_map->byte_at_put(map_index++, T_OBJECT); 

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
bool JavaClass::is_hidden_in_profile() const {
  return ROM::class_is_hidden_in_profile(this);
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#if ENABLE_COMPILER_TYPE_INFO
// Returns true if this class doesn't have any subtypes except for itself
bool JavaClass::is_final_type() const {
  if (is_instance_class()) {
    // ROM optimizer can mark hidden interface or abstract class as final
    return !is_interface() && !is_abstract() && is_final();
  } else if (is_type_array_class()) {
    return true;
  } else {
    GUARANTEE(is_obj_array_class(), "Must be an object array class");
    ObjArrayClass::Raw obj_array_class = this->obj();
    JavaClass::Raw element_class = obj_array_class().element_class();
    return element_class().is_final_type();
  }
}
#endif

