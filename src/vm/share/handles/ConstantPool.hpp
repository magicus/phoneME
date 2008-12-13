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

/*
 * These two cp_check() macros were originally written as inlined
 * check(... JVM_TRAPS) function. Rewritting them in macros helps the C++ 
 * compiler generate smaller code.
 */
#define cp_check(b) \
    if (!(b)) { \
      throw_invalid_constant_error(JVM_SINGLE_ARG_THROW); \
    }

#define cp_check_0(b) \
    if (!(b)) { \
      throw_invalid_constant_error(JVM_SINGLE_ARG_THROW_0); \
    }

class ConstantPool: public Oop {
 public:
  HANDLE_DEFINITION_CHECK(ConstantPool, Oop);
  ~ConstantPool() {}

  // Used by TemplateTable
  static int base_offset() {
    return ConstantPoolDesc::header_size();
  }
  // The address of the first entry (i.e, address of obj_field(0));
  address base_address() {
    return ((address)obj()) + base_offset();
  }
  static int length_offset() {
    return FIELD_OFFSET(ConstantPoolDesc, _length);
  }

  static int tags_offset() {
    return FIELD_OFFSET(ConstantPoolDesc, _tags);
  }

  // Returns the length
  jushort length() const {
    return ushort_field(length_offset());
  }

  // Tells whether index is within bounds.
  bool is_within_bounds(int index) const {
    return ((juint)index) < ((juint)length());
  }
  inline static bool is_within_bounds(int index, int len) {
    return ((juint)index) < ((juint)len);
  }
  inline ReturnOop tags() const {
    return obj_field(tags_offset());
  }

 private:
  static void throw_invalid_constant_error(JVM_SINGLE_ARG_TRAPS);

  // Use this version instead of the one below only
  // in those cases that are not called indirectly by the ClassFileParser.
  int offset_from_index(int index) const {
    GUARANTEE(is_within_bounds(index), "Constant pool index out of bounds");
    return base_offset() + (index * sizeof(jint));
  }

  // Use this version instead of the one above
  // when indirectly called by the ClassFileParser.
  int offset_from_checked_index(int index JVM_TRAPS) const {
    cp_check_0(is_within_bounds(index));
    int result = base_offset() + (index * sizeof(jint));
    GUARANTEE(result != 0, "sanity for JVM_ZCHECK");
    return result;
  }

 public:

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
    
 public:
  // Tag query
  ConstantTag tag_at(int index) const;

  jubyte tag_value_at(int index) const {
    GUARANTEE(((juint)index) < length(), "sanity"); 
    jubyte* ptr = (jubyte*)tags();
    ptr += Array::base_offset();
    return ptr[index];
  }

  void tag_at_put(int index, jubyte t)  {
    GUARANTEE(((juint)index) < length(), "sanity"); 
    jubyte* ptr = (jubyte*)tags();
    ptr += Array::base_offset();
    ptr[index] = t;
  }

  // Fetching constants

  ReturnOop klass_at(int index JVM_TRAPS);
  ReturnOop name_of_klass_at(int index JVM_TRAPS);
  ReturnOop klass_ref_at(int index JVM_TRAPS);
  ReturnOop resolved_klass_at(int index JVM_TRAPS) const;

  jint int_at(int index) {
    GUARANTEE(tag_at(index).is_int(), "Corrupted constant pool");
    return int_field(offset_from_index(index));
  }

  jlong long_at(int index) {
    GUARANTEE(tag_at(index).is_long(), "Corrupted constant pool");
    return long_field(offset_from_index(index));
  }

  jfloat float_at(int index) {
    GUARANTEE(tag_at(index).is_float(), "Corrupted constant pool");
    return float_field(offset_from_index(index));
  }

  jdouble double_at(int index) {
    GUARANTEE(tag_at(index).is_double(), "Corrupted constant pool");
    return double_field(offset_from_index(index));
  }

  // Use this version instead of the one below only
  // in those cases that are not called by the ClassFileParser.
  ReturnOop symbol_at(int index) {
    // The symbol tags in the tags array are skipped in system ROM
    // to save space.
    GUARANTEE(ROM::system_text_contains(obj()) || tag_at(index).is_symbol(), 
              "Corrupted constant pool");
    return obj_field(offset_from_index(index));
  }

  // Use this version instead of the one above from the ClassFileParser
  ReturnOop checked_symbol_at(int index JVM_TRAPS);
  ReturnOop checked_type_symbol_at(int index JVM_TRAPS);
  ReturnOop checked_class_name_at(int index JVM_TRAPS);

  ReturnOop string_at(int index JVM_TRAPS);

  ReturnOop resolve_string_at(int index JVM_TRAPS);

  ReturnOop resolved_string_at(int index) const {
    GUARANTEE(tag_at(index).is_string(), "Corrupted constant pool");
    return obj_field(offset_from_index(index));
  }

  ReturnOop resolve_symbol_at_offset(int offset JVM_TRAPS);
  ReturnOop resolve_type_symbol_at_offset(int offset JVM_TRAPS);

  // declaring_class: the class where the field is declared.
  BasicType field_type_at(int index, int& offset, bool is_static, bool is_get,
                          InstanceClass *sender_class, 
                          InstanceClass *declaring_class JVM_TRAPS);

  jushort extract_high_jushort_at(int index) const {
    GUARANTEE(tag_at(index).is_short_pair(), "Corrupted constant pool");
    return extract_high_jushort_from_jint(int_field(offset_from_index(index)));
  }
  jushort extract_low_jushort_at(int index) const {
    GUARANTEE(tag_at(index).is_short_pair(), "Corrupted constant pool");
    return extract_low_jushort_from_jint(int_field(offset_from_index(index)));
  }

  BasicType resolved_field_type_at(int index, int& offset) const {
    GUARANTEE(tag_at(index).is_resolved_field(), "Corrupted constant pool");
    offset = extract_high_jushort_at(index);
    return ConstantTag::resolved_field_type(tag_value_at(index));
  }

  BasicType resolved_field_type_at(int index, int& offset, int& class_id) const
  {
    GUARANTEE(tag_at(index).is_resolved_field(), "Corrupted constant pool");
    jint value = int_field(offset_from_index(index));
    offset   = extract_high_jushort_from_jint(value);
    class_id = extract_low_jushort_from_jint(value);
    return ConstantTag::resolved_field_type(tag_value_at(index));
  }

  ReturnOop resolved_static_method_at(int index) const {
    GUARANTEE(tag_at(index).is_resolved_static_method() || 
              tag_at(index).is_resolved_final_uncommon_interface_method(),
              "Corrupt constant pool");
    return obj_field(offset_from_index(index));
  }

  ReturnOop resolved_uncommon_final_method_at(int index) const {
    GUARANTEE(tag_at(index).is_resolved_final_uncommon_interface_method(),
              "Corrupt constant pool");
    return obj_field(offset_from_index(index));
  }

  BasicType resolved_virtual_method_at(int index, int& vtable_index, 
                                       int& class_id) const ;

  void resolved_interface_method_at(int index, int& itable_index, 
                                    int& class_id) const {
    GUARANTEE(tag_at(index).is_resolved_interface_method(),
              "Corrupt constant pool");
    itable_index = extract_low_jushort_at(index);
    class_id  = extract_high_jushort_at(index);
  }

  // Resolves the MethodRef at index to a fixed method
  // Returns whether the class has been initialized
  bool resolve_invoke_static_at(InstanceClass *sender_class, int index,
                                bool do_init JVM_TRAPS);

  // Resolves the MethodRef to either:
  // - a fixed method entry if the resolved method is final (true is returned)
  // - a virtual method entry otherwise (false is returned)
  bool resolve_invoke_virtual_at(InstanceClass *sender_class, int index 
                                 JVM_TRAPS);

  // Resolves the MethodRef to either:
  // - a fixed method entry if the resolved method is final (true is returned)
  // - a virtual method entry otherwise (false is returned)
  bool resolve_invoke_special_at(InstanceClass *sender_class, int index 
                                 JVM_TRAPS);

  // Resolves the MethodRef to a interface method entry
  void resolve_invoke_interface_at(InstanceClass *sender_class, int index
                                   JVM_TRAPS);

  bool resolve_method_ref(int index, InstanceClass* klass, Method* method
                          JVM_TRAPS);

  void resolved_static_method_at_put(int index, Oop* m);

  // Returns whether method is virtual after resolution
  // (i.e. method is not final)
  bool resolve_virtual_method_at(int index JVM_TRAPS);

  void iterate(OopVisitor*) PRODUCT_RETURN;
  void fully_resolve(JVM_SINGLE_ARG_TRAPS) PRODUCT_RETURN;
  static void iterate_oopmaps(oopmaps_doer /*do_map*/, void* /*param*/)
                              PRODUCT_RETURN;

#if !defined(PRODUCT) || USE_DEBUG_PRINTING
  void print_value_on(Stream*);
  void print_entry_on(Stream* st, int index JVM_TRAPS);
  void print_class_on(Stream* st, int index JVM_TRAPS);
  void print_field_ref_on(Stream* st, int index JVM_TRAPS);
  void print_virtual_method_ref_on(Stream* st, int index JVM_TRAPS);
  void print_interface_method_ref_on(Stream* st, int index JVM_TRAPS);
  void print_unresolved_ref_on(Stream* st, int index JVM_TRAPS);
  void print_verbose_field_on(Stream* st, InstanceClass *klass, 
                              int offset, bool is_static);
  char* comment_text(int index);
#else
  void print_value_on(Stream*) {}
#endif

 private:
  ReturnOop oop_at(int index) {
    GUARANTEE(tag_at(index).is_oop(), "sanity");
    return obj_field(offset_from_index(index));
  }

  ReturnOop lookup_method_at(InstanceClass *sender_class, int index,
                             Symbol* name, Symbol* signature,
                             InstanceClass* static_receiver_class JVM_TRAPS);

  // JVM_CONSTANT_ClassIndex
  int klass_index_at(int index JVM_TRAPS) {
    int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
    cp_check_0(ConstantTag::is_klass_index(tag_value_at(index)));
    return int_field(offset);
  }

  ReturnOop unresolved_klass_at(int index JVM_TRAPS) const;

  ReturnOop unchecked_unresolved_klass_at(int index) {
    int offset = offset_from_index(index);
    GUARANTEE(tag_at(index).is_unresolved_klass(), "sanity");
    return obj_field(offset);
  }

  int string_index_at(int index) {
    GUARANTEE(tag_at(index).is_string_index(), "Corrupted constant pool");
    return int_field(offset_from_index(index));
  }

  ReturnOop unresolved_string_at(int index) {
    GUARANTEE(tag_at(index).is_unresolved_string(), "Corrupted constant pool");
    return obj_field(offset_from_index(index));
  }

  jint field_or_method_at(int index JVM_TRAPS) {
    int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
    cp_check_0(ConstantTag::is_field_or_method(tag_value_at(index)));
    int result = int_field(offset);
    GUARANTEE(result != 0, "sanity for JVM_ZCHECK");
    return result;
  }

  jint name_and_type_at(int index JVM_TRAPS) const;

  void klass_at_put(int index, JavaClass* k);

  void klass_index_at_put(int index, int name_index) {
    tag_at_put(index, JVM_CONSTANT_ClassIndex);
    int_field_put(offset_from_index(index), name_index);
  }

  void unresolved_klass_at_put(int index, Symbol* symbol) {
    tag_at_put(index, JVM_CONSTANT_UnresolvedClass);
    obj_field_put(offset_from_index(index), symbol);
  }

  void unresolved_string_at_put(int index, Symbol* symbol) {
    tag_at_put(index, JVM_CONSTANT_UnresolvedString);
    obj_field_put(offset_from_index(index), symbol);
  }

  void int_at_put(int index, jint i) {
    tag_at_put(index, JVM_CONSTANT_Integer);
    int_field_put(offset_from_index(index), i);
  }

  void long_at_put(int index, jlong l) {
    tag_at_put(index, JVM_CONSTANT_Long);
    long_field_put(offset_from_index(index), l);
  }

  void float_at_put(int index, jfloat f) {
    tag_at_put(index, JVM_CONSTANT_Float);
    float_field_put(offset_from_index(index), f);
  }

  void double_at_put(int index, jdouble d) {
    tag_at_put(index, JVM_CONSTANT_Double);
    double_field_put(offset_from_index(index), d);
  }

  void double_parts_at_put(int index, jint msw, jint lsw) {
    tag_at_put(index, JVM_CONSTANT_Double);
    if (MSW_FIRST_FOR_DOUBLE) { 
      int_field_put(offset_from_index(index), msw);
      int_field_put(offset_from_index(index+1), lsw);
    } else { 
      int_field_put(offset_from_index(index), lsw);
      int_field_put(offset_from_index(index+1), msw);
    }
  }

  void long_parts_at_put(int index, jint msw, jint lsw) {
    tag_at_put(index, JVM_CONSTANT_Long);
    if (MSW_FIRST_FOR_LONG) { 
      int_field_put(offset_from_index(index), msw);
      int_field_put(offset_from_index(index+1), lsw);
    } else { 
      int_field_put(offset_from_index(index), lsw);
      int_field_put(offset_from_index(index+1), msw);
    }
  }

  void symbol_at_put(int index, Oop* oop) {
    tag_at_put(index, JVM_CONSTANT_Utf8);
    GUARANTEE(oop->is_symbol() || oop->is_byte_array(), "Sanity");
    obj_field_put(offset_from_index(index), oop);
  }

  void string_at_put(int index, Oop* str) {
    tag_at_put(index, JVM_CONSTANT_String);
    obj_field_put(offset_from_index(index), str);
  }

   void string_index_at_put(int index, int string_index) {
    tag_at_put(index, JVM_CONSTANT_StringIndex);
    int_field_put(offset_from_index(index), string_index);
  }

  void field_at_put(int index, jushort class_index, jushort name_and_type_index)
  {
    tag_at_put(index, JVM_CONSTANT_Fieldref);
    int_field_put(offset_from_index(index), 
              construct_jint_from_jushorts(name_and_type_index, class_index));
  }

  void method_at_put(int index, jushort class_index, jushort name_and_type_index)
  {
    tag_at_put(index, JVM_CONSTANT_Methodref);
    int_field_put(offset_from_index(index),
              construct_jint_from_jushorts(name_and_type_index, class_index));
  }

  void interface_method_at_put(int index, jushort class_id, 
                               jushort name_and_type_index)
  {
    tag_at_put(index, JVM_CONSTANT_InterfaceMethodref);
    int_field_put(offset_from_index(index), 
              construct_jint_from_jushorts(name_and_type_index, class_id));
  }

  void name_and_type_at_put(int index, jushort name_index, 
                            jushort signature_index) {
    tag_at_put(index, JVM_CONSTANT_NameAndType);
    int_field_put(offset_from_index(index),
                  construct_jint_from_jushorts(signature_index, name_index));
  }

  // may be either static or non-static field
  void resolved_field_at_put(int index, jushort class_id, jushort offset, 
                             BasicType type, bool is_static)
  {
    tag_at_put(index, 
               (jubyte)((is_static ? JVM_CONSTANT_ResolvedStaticBooleanFieldref : JVM_CONSTANT_ResolvedBooleanFieldref) + type - T_BOOLEAN));
    int_field_put(offset_from_index(index),
           construct_jint_from_jushorts(offset, class_id));
  }


  void resolved_virtual_method_at_put(int index, jushort class_id,
                                      jushort vtable_index, BasicType type) {
    tag_at_put(index, 
         (jubyte)(JVM_CONSTANT_ResolvedBooleanVirtualMethod + type - T_BOOLEAN));
    int_field_put(offset_from_index(index),
         construct_jint_from_jushorts(class_id, vtable_index));
  }

  void resolved_interface_method_at_put(int index, jushort vtable_index, 
                                        jushort class_id, BasicType type) {
    tag_at_put(index, 
       (jubyte)(JVM_CONSTANT_ResolvedBooleanInterfaceMethod + type - T_BOOLEAN));
    int_field_put(offset_from_index(index),
       construct_jint_from_jushorts(class_id, vtable_index));
  }

  // Lookup for entries consisting of (name_index, signature_index)
  int klass_ref_index_at(int index JVM_TRAPS);
  int name_and_type_ref_index_at(int index JVM_TRAPS);
  int name_ref_index_at(int index JVM_TRAPS);
  int signature_ref_index_at(int index JVM_TRAPS);

  bool check_klass_at(int index) const;
  void check_constant_at(int index, FieldType* type JVM_TRAPS) const;

  void set_tags(TypeArray* value) {
    obj_field_put(tags_offset(), (Oop*) value);
  }
  void trace_no_such_method_error(InstanceClass* /*sender_class*/,
                                  InstanceClass* /*receiver_class*/,
                                  Symbol* /*name*/,
                                  Symbol* /*signature*/) PRODUCT_RETURN;
  void set_length(int length) {
    ushort_field_put(length_offset(), (jushort)length);
  }
 private:
  void resolve_helper_0(int index, Symbol* name, Symbol* signature,       
                        InstanceClass* klass, Symbol* klass_name JVM_TRAPS);

 public:
  // make this public for the verifier
  inline void resolve_helper(int index, Symbol* name, Symbol* signature,       
                             InstanceClass* klass JVM_TRAPS) {
    resolve_helper_0(index, name, signature, klass, NULL 
                     JVM_NO_CHECK_AT_BOTTOM);
  }
  inline void resolve_helper(int index, Symbol* name, Symbol* signature,
                             Symbol* klass_name JVM_TRAPS) {
    resolve_helper_0(index, name, signature, NULL, klass_name 
                     JVM_NO_CHECK_AT_BOTTOM);
  }

  jint value32_at(int index) {
    return int_field(offset_from_index(index));
  }
  void value32_at_put(int index, jint value) {
    int_field_put(offset_from_index(index), value);
  }
  jlong value64_at(int index) {
    GUARANTEE(tag_at(index).is_long() || tag_at(index).is_double(),
              "Corrupted constant pool");
    return long_field(offset_from_index(index));
  }

  // Try to resolve an UnresolvedClass without causing any class loading
  // or class initialization. This is used by the compiler.
  ReturnOop try_resolve_klass_at(int index);

  static void suspend_class_loading() {
    GUARANTEE(_class_loading_suspended == false, "cannot be nested");
    _class_loading_suspended = true;
  }
  static void resume_class_loading() {
    GUARANTEE(_class_loading_suspended == true, "cannot be nested");
    _class_loading_suspended = false;
  }

  // This means we can reuse the NameAndType entries for something
  // else, such as inlining.
  bool needs_name_and_type_entries() PRODUCT_RETURN0;
 private:
  // check field access in quickened case
  void check_quickened_field_access(int index, InstanceClass* sender_class, 
                                    bool is_static, bool is_get JVM_TRAPS);

  // The class file parser is allowed to call the at_put methods
  friend class ClassFileParser;
  friend class Universe;
  friend class ROM;
  friend class ROMOptimizer;
  // The romizer is allowed to call offset_from_index
  friend class ROMWriter;
  friend class ROMInliner;
  friend class ConstantPoolRewriter;
  static bool _class_loading_suspended;
};
