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

class ClassInfo: public Oop {
 public:
  HANDLE_DEFINITION(ClassInfo, Oop);
  ~ClassInfo() {}

 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int access_flags_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _access_flags);
  }
  static int object_size_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _object_size);
  }
  static int vtable_length_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _vtable_length);
  }
  static int itable_length_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _itable_length);
  }
  static int class_id_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _class_id);
  }
  static int name_offset() {
    return FIELD_OFFSET(ClassInfoDesc, _name);
  }
  static int methods_offset() {
    return FIELD_OFFSET(ClassInfoDesc, instance._methods);
  }
  static int local_interfaces_offset() {
    return FIELD_OFFSET(ClassInfoDesc, instance._local_interfaces);
  }
  static int fields_offset() {
    return FIELD_OFFSET(ClassInfoDesc, instance._fields);
  }
#if ENABLE_REFLECTION
  static int inner_classes_offset(){
    return FIELD_OFFSET(ClassInfoDesc, instance._inner_classes);
  }
#endif

#if ENABLE_ISOLATES
  static int static_field_end_offset(){
    return FIELD_OFFSET(ClassInfoDesc, instance._static_field_end);
  }
#endif //ENABLE_ISOLATES
  static int constants_offset() {
    return FIELD_OFFSET(ClassInfoDesc, instance._constants);
  }
  static int type_offset() {
    return FIELD_OFFSET(ClassInfoDesc, array._type);
  }
  static int scale_offset() {
    return FIELD_OFFSET(ClassInfoDesc, array._scale);
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
  
  //
  // Accessors to access flags
  //
  AccessFlags access_flags() const { 
    AccessFlags access_flags; 
    jint flags = int_field(access_flags_offset());
    access_flags.set_flags(flags);
    return access_flags; 
  }

  bool is_interface() const {
    return access_flags().is_interface();
  }
  bool is_final() const {
    return access_flags().is_final();
  }
  bool is_abstract() const {
    return access_flags().is_abstract();
  }
  bool is_public() const {
    return access_flags().is_public();
  }
  bool is_private() const {
    return access_flags().is_private();
  }
  bool is_verified() const {
    return access_flags().is_verified();
  }
  bool has_finalizer() const {
    return access_flags().has_finalizer();
  }
  bool has_vanilla_constructor() const {
    return access_flags().has_vanilla_constructor();
  }

  void set_access_flags(AccessFlags access_flags) {
    int_field_put(access_flags_offset(), access_flags.as_int());
  }
  void set_is_verified( void ) {
    AccessFlags flags = access_flags();
    flags.set_is_verified();
    set_access_flags(flags);
  }
  void set_has_vanilla_constructor( void ) {
    AccessFlags flags = access_flags();
    flags.set_has_vanilla_constructor();
    set_access_flags(flags);
  }
  void set_is_fake_class( void ) {
    AccessFlags flags = access_flags();
    flags.set_is_fake_class();
    set_access_flags(flags);
  }

  // type of the elements
  jint type() const {
    return int_field(type_offset());
  }
  void set_type(jint value) {
    int_field_put(type_offset(), value);
  }

  // size of the elements
  jint scale() const {
    return int_field(scale_offset());
  }
  void set_scale(jint value) {
    int_field_put(scale_offset(), value);
  }

  //
  // Accessors to name
  //
  ReturnOop name() const {
    return obj_field(name_offset());
  }
  void set_name(Symbol* value) {
    obj_field_put(name_offset(), (Oop*)value);
  }

  // Index of this class in Universe::class_list()
  jushort class_id() const {
    return ushort_field(class_id_offset());
  }
  void set_class_id(jint value) {
    ushort_field_put(class_id_offset(), (jushort) value);
  }

  // Method array
  ReturnOop methods() const {
    return obj_field(methods_offset());
  }
  void set_methods(ObjArray* value) {
    obj_field_put(methods_offset(), (Oop*)value);
  }

  // Instance and static variable information, 5-tuples of shorts
  // [access, name index, sig index, initval index, offset]
  ReturnOop fields() const {
    return obj_field(fields_offset());
  }
  void set_fields(TypeArray* value) {
    obj_field_put(fields_offset(), (Oop*)value);
  }

#if ENABLE_REFLECTION
  ReturnOop inner_classes() const {
    return obj_field(inner_classes_offset());
  }
  void set_inner_classes(TypeArray* value) {
    obj_field_put(inner_classes_offset(), value);
  }
#endif

#if ENABLE_ISOLATES
  jint static_field_end() const {
    return int_field(static_field_end_offset());
  }
  void set_static_field_end(jint value) {
    int_field_put(static_field_end_offset(), value);
  }
#endif

  // Constant pool for this class
  ReturnOop constants() const {
    return obj_field(constants_offset());
  }
  void set_constants(Oop* value) {
    obj_field_put(constants_offset(), value);
  }

  //
  // Virtual dispatch table access
  //

  // Returns the number of entries in the vtable
  jushort vtable_length() const {
    return ushort_field(vtable_length_offset());
  }
  static int vtable_offset_from_index(int index) {
    return ClassInfoDesc::header_size() + (index *  sizeof(jobject));
  }

  // Interfaces this class declares locally to implement
  ReturnOop local_interfaces() const {
    return obj_field(local_interfaces_offset());
  }
  void set_local_interfaces(TypeArray* value) {
    obj_field_put(local_interfaces_offset(), (Oop*)value);
  }

  // Returns the Method at index
  ReturnOop vtable_method_at(int index) {
    return obj_field(vtable_offset_from_index(index));
  }

  //
  // Interface dispatch table access
  //

  int itable_offset_from_index(int index) {
    int itable_start = vtable_offset_from_index(vtable_length());
    return itable_start
         + (index *  (sizeof(int) + sizeof(int)));
  }

  // Returns the number of interfaces in the table
  jushort itable_length() const {
    return ushort_field(itable_length_offset());
  }

  // Returns the class_id of the interface at index
  int itable_interface_class_id_at(int index) {
    return int_field(itable_offset_from_index(index));
  }
  ReturnOop itable_interface_at(int index) {
    int class_id = int_field(itable_offset_from_index(index));
    return Universe::class_from_id(class_id);
  }
  // Returns the interface at index
  int itable_offset_at(int index) {
    return int_field(itable_offset_from_index(index) + sizeof(int));
  }

  jint itable_size();
  static jint itable_size(int nof_interfaces, int nof_methods);

  void vtable_at_put(int index, Oop* value) {
    obj_field_put(vtable_offset_from_index(index), value);
  }
  void vtable_at_put(int index, OopDesc* value) {
    obj_field_put(vtable_offset_from_index(index), value);
  }
  void vtable_at_clear(int index) {
    obj_field_clear(vtable_offset_from_index(index));
  }

  int interface_offset_at(int index) {
    return int_field(itable_offset_from_index(index) + sizeof(int));
  }

  void itable_interface_at_put(int index, int class_id) {
#ifdef AZZERT
    Oop cls = Universe::class_from_id(class_id);
    GUARANTEE(!cls.is_null(), "sanity");
#endif
    int_field_put(itable_offset_from_index(index), class_id);
  }
  // Returns the interface at index
  void itable_offset_at_put(int index, int value) {
    int_field_put(itable_offset_from_index(index) + sizeof(int), value);
  }

  int itable_methods_offset();

  // Is this a ClassInfo for an ArrayClass?
  bool is_array() {
    return access_flags().is_array_class();
  }

  static int generic_header_size() {
    return sizeof(GenericClassInfoDesc);
  }
  static int header_size() {
    return sizeof(ClassInfoDesc);
  }

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || ENABLE_TTY_TRACE
  // used by romizer
  int itable_end_offset() const {
    return ushort_field(object_size_offset());
  }
  void iterate_tables(OopROMVisitor* visitor);
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE ||\
                         ENABLE_PERFORMANCE_COUNTERS
  void print_name_on(Stream* st);
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE
  void print_value_on(Stream* st);
#endif

#if !defined(PRODUCT)
  void iterate(OopVisitor* visitor);

  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif
};
