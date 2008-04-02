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

class JavaClass: public FarClass {
 public:
  HANDLE_DEFINITION_CHECK(JavaClass, FarClass);
  ~JavaClass() {}
  enum AccessType {
    acc_private         = 0,
    acc_package_private = 1,
    acc_publicprotected = 2
  };

  // To avoid endless lists of friends the static offset computation routines
  // are all public.
  static int class_info_offset() {
    return FIELD_OFFSET(JavaClassDesc, _class_info);
  }

  static int array_class_offset() {
    return FIELD_OFFSET(JavaClassDesc, _array_class);
  }

#if !ENABLE_ISOLATES
  static int java_mirror_offset() {
    return FIELD_OFFSET(JavaClassDesc, _java_mirror);
  }
#endif

  static int subtype_cache_1_offset() {
    return FIELD_OFFSET(JavaClassDesc, _subtype_cache_1);
  }
  static int subtype_cache_2_offset() {
    return FIELD_OFFSET(JavaClassDesc, _subtype_cache_2);
  }
  static int super_offset() {
    return FIELD_OFFSET(JavaClassDesc, _super);
  }

  ReturnOop class_info() const {
    return obj_field(class_info_offset());
  }
  void set_class_info(ClassInfo* value) {
    obj_field_put(class_info_offset(), value);
  }

 protected:
  // for accessing ClassInfo without creating a handle
  ClassInfoDesc *raw_class_info() const {
    return (ClassInfoDesc *)class_info();
  }

 public:

  ReturnOop super() const       { return obj_field(super_offset()); }
  void set_super(Oop* value)    { obj_field_put(super_offset(), value); }

  // get the class loader for this java class
  ReturnOop class_loader();

  // is_a_cache
  ReturnOop subtype_cache_1() const {
    return obj_field(subtype_cache_1_offset());
  }
  void set_subtype_cache_1(JavaClass* value) {
    obj_field_put(subtype_cache_1_offset(), value);
  }
  void clear_subtype_cache_1() {
    obj_field_clear(subtype_cache_1_offset());
  }

  ReturnOop subtype_cache_2() const {
    return obj_field(subtype_cache_2_offset());
  }
  void set_subtype_cache_2(JavaClass* value) {
    obj_field_put(subtype_cache_2_offset(), value);
  }
  void set_subtype_cache_2(OopDesc* value) {
    obj_field_put(subtype_cache_2_offset(), value);
  }
  void clear_subtype_cache_2() {
    obj_field_clear(subtype_cache_2_offset());
  }

#if ENABLE_ISOLATES
  // Static variables are embedded in the TaskMirror object
  // Returns the offset where the static field section starts and ends in the
  // TaskMirror object.
  static jint static_field_start() {
    return TaskMirror::static_field_start();
  }

  jint static_field_end() const {
    return static_field_size() + TaskMirror::header_size();
  }

  // Initialize all the entries in the mirror table to the class
  // initialization barrier mark.

  void set_task_mirror(Oop *tm) {
    GUARANTEE(!tm->is_null(), "Setting null task mirror");
    ObjArray::Raw ml = Universe::mirror_list();
    ml().obj_at_put(class_id(), tm);
  }

  inline static bool is_being_initialized_mirror(TaskMirror *tm) {
    return
      TaskMirrorDesc::is_being_initialized_mirror((TaskMirrorDesc*)tm->obj());
  }

public:  
  // Return without cleaning of barrier mark. Caller must take care of that.
  TaskMirrorDesc* task_mirror_desc() const {
    return (TaskMirrorDesc*)Universe::task_mirror_from_id(class_id());
  }

  // Task mirror of current task. Must be called only if mirror is initialized
  ReturnOop task_mirror() const {
    GUARANTEE(Universe::is_bootstrapping() ||
              TaskMirrorDesc::is_initialized_mirror(task_mirror_desc()), 
              "Must not be a barrier");
    return (OopDesc*)task_mirror_desc();
  }

  // Task mirror of current task.
  ReturnOop task_mirror_no_check() const {
    return (OopDesc*)task_mirror_desc();
  }

  ReturnOop real_task_mirror() const;

  ReturnOop setup_task_mirror(int static_size,
                              int vtable_length,
                              bool set_init_barrier JVM_TRAPS);

  ReturnOop java_mirror();

  ReturnOop get_or_allocate_java_mirror(JVM_SINGLE_ARG_TRAPS);

  //  ReturnOop array_class() { return obj_field(array_class_offset()); }
  ReturnOop array_class();

  void set_array_class(ObjArrayClass* value JVM_TRAPS);

#else
//!ENABLE_ISOLATES

  ReturnOop java_mirror();

  ReturnOop get_or_allocate_java_mirror(JVM_SINGLE_ARG_TRAPS) {
    return java_mirror();
    JVM_IGNORE_TRAPS;
  }

  // Returns the offset where the static field section starts and ends.
  static jint static_field_start() {
    return JavaClassDesc::header_size();
  }
  jint static_field_end() const {
    return embedded_oop_map_start();
  }

  // Array class holding elements of this class
  ReturnOop array_class() { return obj_field(array_class_offset()); }

  void set_array_class(Oop* value JVM_TRAPS) {
    obj_field_put(array_class_offset(), value);
    JVM_IGNORE_TRAPS;
  }
#endif // ENABLE_ISOLATES

  void setup_java_mirror(JVM_SINGLE_ARG_TRAPS);

  // ^ObjArrayClass,
  // distance == 1, returns the array class that contains elements of this
  //                type.
  // distance == 2, returns the array class that contains elements of arrays
  //                of this type.
  // ...
  ReturnOop get_array_class(jint distance JVM_TRAPS);

  // sub class check
  bool is_subclass_of(JavaClass* other_class);
  bool is_subtype_of(JavaClass* other_class);

  bool compute_and_set_is_subtype_of(JavaClass* other_class);
  bool compute_is_subtype_of(JavaClass* other_class);

  void clear_subtype_caches() {
    clear_subtype_cache_1();
    clear_subtype_cache_2();
  }

  bool check_access_by(InstanceClass* sender_class, FailureMode fail_mode
                       JVM_TRAPS);

  // Allocates a new instance and calls <init>
  // Throws 'InstantiationException' if this class must not be instantiated
  // Throws 'IllegalAccessException'
  // Checks access by 'sender_class': the class which is initiating the
  // instantiation,
  // (e.g. as holder of a method that executes a 'new' bytecode)
  ReturnOop
  new_initialized_instance(InstanceClass* sender_class, Thread *t JVM_TRAPS);

  void iterate(OopVisitor* /*visitor*/) PRODUCT_RETURN;
#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE ||\
                         ENABLE_PERFORMANCE_COUNTERS
  void print_name_on(Stream*);
#endif
  static void iterate_oopmaps(oopmaps_doer /*do_map*/, void* /*param*/) 
                              PRODUCT_RETURN;

  // Initialize the virtual dispatch table
  void initialize_vtable();
  int copy_super_vtable();

  // Returns the accessibility at index.
  AccessType vtable_accessibility_at(int index);

  // Number of bytes used to store static fields.
  jint static_field_size() const {
#if ENABLE_ISOLATES
    ClassInfo::Raw info = class_info();
    return info().static_field_end() - TaskMirror::header_size();
#else
    return (static_field_end() - static_field_start());
#endif
  }

  // Convenience functions for accessing stuff inside class_info()
  ReturnOop name() const {
    return raw_class_info()->_name;
  }         

  void set_name(Symbol* value) {
    ClassInfo::Raw info = class_info();
    info().set_name(value);
  }
  AccessFlags access_flags() const {
    return raw_class_info()->_access_flags;
  }
  void set_access_flags(AccessFlags access_flags) {
    ClassInfo::Raw info = class_info();
    info().set_access_flags(access_flags);
  }
  void set_is_non_optimizable( void ) {
    AccessFlags flags = access_flags();
    flags.set_is_non_optimizable();
    set_access_flags(flags);
  }

  jushort class_id() const {
    return raw_class_info()->_class_id;
  }
  jint vtable_length() const {
    return raw_class_info()->_vtable_length;
  }
  jint itable_length() const {
    return raw_class_info()->_itable_length;
  }
  bool is_array_class() const {
    return access_flags().is_array_class();
  }

  bool is_interface()      const { return access_flags().is_interface(); }
  bool is_final()          const { return access_flags().is_final();     }
  bool is_abstract()       const { return access_flags().is_abstract();  }
  bool is_super()          const { return access_flags().is_super();     }
  bool is_public()         const { return access_flags().is_public();    }
  bool is_private()        const { return access_flags().is_private();   }
  bool is_preloaded()      const { return access_flags().is_preloaded(); }
  bool is_converted()      const { return access_flags().is_converted(); }
  bool is_romized()        const { return access_flags().is_romized();   }
  bool is_optimizable()    const { return access_flags().is_optimizable();}
  bool is_fake_class()     const { return access_flags().is_fake_class();}  
  bool is_hidden()         const { return access_flags().is_hidden() || 
                                          is_hidden_in_profile();        }
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  bool is_hidden_in_profile() const;
#else
  bool is_hidden_in_profile() const { return false; }
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#if ENABLE_COMPILER_TYPE_INFO
  // Returns true if this class doesn't have any subtypes except for itself
  bool is_final_type() const;
#endif

  bool has_finalizer()     const { return access_flags().has_finalizer(); }
  bool has_vanilla_constructor() const {
    return access_flags().has_vanilla_constructor();
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

  friend class ConstantPoolRewriter;
};
