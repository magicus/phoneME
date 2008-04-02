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

class InstanceClass: public JavaClass {
 public:
  HANDLE_DEFINITION_CHECK(InstanceClass, JavaClass);

  static int next_offset() {
    return FIELD_OFFSET(JavaClassDesc, instance._next);
  }

 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.

   // Unimplemented stuff...
  void set_is_synthetic() {
  }

  // Returns the package Name of a Class.
  ReturnOop package_name(JVM_SINGLE_ARG_TRAPS);

  // Next InstanceClass with the same hash value
  ReturnOop next() const {
    return obj_field(next_offset());
  }

  void set_next(JavaClass* value) {
    obj_field_put(next_offset(), (Oop*)value);
  }


  static int header_size() { return sizeof(InstanceClassDesc); }

  // oop maps
  size_t first_nonstatic_map_offset() const {
    return embedded_oop_map_start();
  }
  size_t first_static_map_offset() const;
  jubyte oop_map_at(size_t offset) const {
    return ubyte_field(offset);
  }
  size_t nonstatic_map_size() const;
  size_t last_nonstatic_oop_offset() const;
  size_t static_map_size() const;

  // Does this InstanceClass object embed a static field that's an oop?
  // (always false in MVM).
  bool has_embedded_static_oops() const {
    return (oop_map_at(first_static_map_offset()) != OopMapSentinel);
  }

  ReturnOop lookup_method(Symbol* name, Symbol* signature,
                          const bool non_static_only = false);

  // Returns the Method.
  // interface_class_id = class_id of interface that declares the method
  // itable_index       = index of this method in the interface class
  //                      with the given interface_class_id
  ReturnOop lookup_method_in_all_interfaces(Symbol* name, Symbol* signature,
                                            int& interface_class_id,
                                            int& itable_index);
  bool is_same_class_package(Symbol* other_class_name);
  bool is_same_class_package(InstanceClass* other_class);
  bool check_access_by(InstanceClass* sender_class, FailureMode fail_mode
                       JVM_TRAPS);

  static ReturnOop add_miranda_methods(InstanceClass* super,
                                       ObjArray* methods, 
                                       TypeArray* local_interfaces,
                                       Symbol *class_name  JVM_TRAPS);

  // Convenience functions for accessing stuff inside class_info()
  ReturnOop constants() const {
    return (ReturnOop) ((void*)raw_class_info()->instance._constants);
  }
  ReturnOop fields() const {
    return (ReturnOop) ((void*)raw_class_info()->instance._fields);
  }
  void set_fields(TypeArray* value) {
    ClassInfo::Raw info = class_info();
    info().set_fields(value);
  }
  ReturnOop methods() const {
    return (ReturnOop) ((void*)raw_class_info()->instance._methods);
  }
  void set_methods(ObjArray* value) {
    ClassInfo::Raw info = class_info();
    info().set_methods(value);
  }
  ReturnOop local_interfaces() const {
    return (ReturnOop) ((void*)raw_class_info()->instance._local_interfaces);
  }
  void set_local_interfaces(TypeArray* value) {
    ClassInfo::Raw info = class_info();
    info().set_local_interfaces(value);
  }
  bool is_fake_class() {
    return access_flags().is_fake_class();
  }

#if ENABLE_REFLECTION
  ReturnOop inner_classes() const {
    return (ReturnOop) ((void*)raw_class_info()->instance._inner_classes);
  }
  void set_inner_classes(TypeArray* value) {
    ClassInfo::Raw info = class_info();
    info().set_inner_classes(value);
  }
#endif

  ReturnOop find_local_method(Symbol* name, Symbol* signature,
                              const bool non_static_only = false);

  static ReturnOop find_method(ObjArray* class_methods, Symbol* name,
                     Symbol* signature, const bool non_static_only = false);

  ReturnOop interface_method_at(jint itable_index);

  // update the vtable with the local methods
  void update_vtable(int super_vtable_length);
  static bool needs_new_vtable_entry(Method* method, InstanceClass* super,
                                     Symbol* classname,
                                     const AccessFlags access_flags, 
                                     ClassInfo* cl_info, //don't need if update_entries == false
                                     bool update_entries);

  void check_and_initialize_itable(JVM_SINGLE_ARG_TRAPS);

  void set_verified();
  bool is_verified();
  void set_initialized();

  bool is_initialized();

  void clinit(JVM_SINGLE_ARG_TRAPS);

  bool itable_contains(InstanceClass* instance_class);

  void initialize_static_fields();
void initialize_static_fields(Oop * o);
#if ENABLE_COMPILER && ENABLE_INLINE
  // Track all the methods overridden by this class and update vtable bitmaps
  // in all super classes accordingly
  void update_vtable_bitmaps(JVM_SINGLE_ARG_TRAPS) const;
#endif

  // perform class initialization
  void bootstrap_initialize(JVM_SINGLE_ARG_TRAPS);
  void initialize(JVM_SINGLE_ARG_TRAPS);
  void verify(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_ISOLATES
  ReturnOop initialize_for_task(JVM_SINGLE_ARG_TRAPS);
#endif
  void remove_clinit();

  bool compute_is_subtype_of(JavaClass* other_class);

  // Allocates a new instance and does NOT call init
  ReturnOop new_instance(FailureMode fail_mode JVM_TRAPS);

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void iterate_static_fields(OopVisitor* visitor);
  void iterate_oop_maps(OopVisitor* visitor);
  void iterate_non_static_fields(OopVisitor* visitor);
  void print_value_on(Stream* st);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);

  // Verifies the size and field offsets of an instance class
  void internal_error_hint();
  void internal_class_error();
  void internal_field_error(Symbol *field_name, Symbol *field_sig);
  void verify_instance_size(size_t size);
  void verify_field(const char* name, const char* signature, 
                    jint field_offset, bool is_static);
  void verify_static_field(const char* name, const char* signature, 
                           jint field_offset);
  void verify_instance_field(const char* name, const char* signature, 
                             jint field_offset);
#endif

  // Methods for accessing symbolic information that has been 
  // renamed/removed by the romizer.
#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR \
      ||ENABLE_JVMPI_PROFILE
  ReturnOop original_name();
  ReturnOop original_fields();
#else
  ReturnOop original_name()   {return name();}
  ReturnOop original_fields() {return fields();}
#endif

  bool is_renamed();

#if USE_EMBEDDED_VTABLE_BITMAP
 private:
  bool is_vtable_bitmap_installed() const {
#if ENABLE_ISOLATES
    TaskMirror::Raw tm = task_mirror_no_check();
    if (is_being_initialized_mirror(&tm)) {
      tm = TaskMirror::clinit_list_lookup(this);
    }
    return tm.not_null();
#else
    return true;
#endif
  }

  void vtable_bitmap_offset(int vtable_index, 
                            int& byte_offset, int& bit_offset) const {
    byte_offset = vtable_bitmap_start() + (vtable_index >> LogBitsPerByte);
    bit_offset = vtable_index & (BitsPerByte - 1);
  }

  void set_vtable_bitmap_bit(int vtable_index) {
    int byte_offset, bit_offset;
    vtable_bitmap_offset(vtable_index, byte_offset, bit_offset);
    const jbyte bitmap_byte = vtable_bitmap_byte(byte_offset);
    vtable_bitmap_byte_put(byte_offset, bitmap_byte | (1 << bit_offset));
  }

  bool vtable_bitmap_bit(int vtable_index) const {
    int byte_offset, bit_offset;
    vtable_bitmap_offset(vtable_index, byte_offset, bit_offset);
    return (vtable_bitmap_byte(byte_offset) & (1 << bit_offset)) != 0;
  }

  void vtable_bitmap_byte_put(int offset, jbyte value) {    
    GUARANTEE(is_vtable_bitmap_installed(), "Sanity");
#if ENABLE_ISOLATES
    TaskMirror::Raw tm = real_task_mirror();
    tm().byte_field_put(offset, value);
#else
    byte_field_put(offset, value);
#endif
  }

  bool vtable_bitmap_byte(int offset) const {
    GUARANTEE(is_vtable_bitmap_installed(), "Sanity");
#if ENABLE_ISOLATES
    TaskMirror::Raw tm = real_task_mirror();
    return tm().byte_field(offset);
#else
    return byte_field(offset);
#endif
  }
 public:
#if ENABLE_ISOLATES
  jint vtable_bitmap_start() const {
    return static_field_end();
  }
#else
  jint vtable_bitmap_start() const {
    return first_static_map_offset() + static_map_size();
  }
#endif

  // Marks the specified vtable entry as overridden in a subclass
  void set_is_method_overridden(int vtable_index);

  // Returns if the specified vtable entry is overridden in any subclass
  bool is_method_overridden(int vtable_index) const;
#endif

 private:
  void initialize_internal(Thread *thread, Oop *m JVM_TRAPS);
  void itable_copy_down(InstanceClass* ic, int& index, int& method_offset
                        JVM_TRAPS);
  void update_local_itables(InstanceClass* ic, int& index, int& method_offset
                            JVM_TRAPS);

  // oop maps
  void oop_map_at_put(size_t offset, jubyte value) {
    ubyte_field_put(offset, value);
  }

  friend class ClassFileParser;
  friend class SystemDictionary;
};

#if ENABLE_ROM_GENERATOR || !defined(PRODUCT)
// For interating over all loaded system classes. Example:
// 
//  for (SystemClassStream st; st.next();) {
//    InstanceClass klass = st.klass();
//    ......
//  }

class SystemClassStream {
  int _found_class_id;
  int _search_class_id;
public:
  /**
   * Initialize the SystemClassStream. If do_all is false, we just 
   * iterate over the classes loaded by this VM:
   * - for the source romizer: these are the classes in the source ROM image.
   * - for the binary romizer: these are the classes in the binary ROM image.
   * If do_all is true, we iterate all classes. In the MVM-case, these
   * are all the classes accessible to the current isolate.
   */
  SystemClassStream(bool do_all = false);

  /**
   * Returns false if we have iterated over all classes
   */
  bool has_next(bool only_optimizable = false, bool include_fake = false);

  ReturnOop next() {
    GUARANTEE(_found_class_id >= 0, 
              "don't call next() unless has_next() has returned true");
    return Universe::class_from_id(_found_class_id);
  }
  bool has_next_optimizable() {
    return has_next(PostponeErrorsUntilRuntime);
  }
};

#endif
