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

// This VM specific constant tags.
enum {
  // See jvm.h for JVM_CONSTANT_XXX tags as specified by JLS.
  JVM_CONSTANT_Invalid  =  0,    // For bad value initialization

  JVM_CONSTANT_UnresolvedClass = JVM_CONSTANT_NameAndType + 1,
  JVM_CONSTANT_ClassIndex,
  JVM_CONSTANT_UnresolvedString,
  JVM_CONSTANT_StringIndex,

  JVM_CONSTANT_ResolvedBooleanFieldref,
  JVM_CONSTANT_ResolvedCharFieldref,
  JVM_CONSTANT_ResolvedFloatFieldref,
  JVM_CONSTANT_ResolvedDoubleFieldref,
  JVM_CONSTANT_ResolvedByteFieldref,
  JVM_CONSTANT_ResolvedShortFieldref,
  JVM_CONSTANT_ResolvedIntFieldref,
  JVM_CONSTANT_ResolvedLongFieldref,
  JVM_CONSTANT_ResolvedObjectFieldref,
  JVM_CONSTANT_ResolvedArrayFieldref,

  JVM_CONSTANT_ResolvedStaticBooleanFieldref,
  JVM_CONSTANT_ResolvedStaticCharFieldref,
  JVM_CONSTANT_ResolvedStaticFloatFieldref,
  JVM_CONSTANT_ResolvedStaticDoubleFieldref,
  JVM_CONSTANT_ResolvedStaticByteFieldref,
  JVM_CONSTANT_ResolvedStaticShortFieldref,
  JVM_CONSTANT_ResolvedStaticIntFieldref,
  JVM_CONSTANT_ResolvedStaticLongFieldref,
  JVM_CONSTANT_ResolvedStaticObjectFieldref,
  JVM_CONSTANT_ResolvedStaticArrayFieldref,

  JVM_CONSTANT_ResolvedStaticMethod,

  JVM_CONSTANT_ResolvedBooleanVirtualMethod,
  JVM_CONSTANT_ResolvedCharVirtualMethod,
  JVM_CONSTANT_ResolvedFloatVirtualMethod,
  JVM_CONSTANT_ResolvedDoubleVirtualMethod,
  JVM_CONSTANT_ResolvedByteVirtualMethod,
  JVM_CONSTANT_ResolvedShortVirtualMethod,
  JVM_CONSTANT_ResolvedIntVirtualMethod,
  JVM_CONSTANT_ResolvedLongVirtualMethod,
  JVM_CONSTANT_ResolvedObjectVirtualMethod,
  JVM_CONSTANT_ResolvedArrayVirtualMethod,
  JVM_CONSTANT_ResolvedVoidVirtualMethod,

  JVM_CONSTANT_ResolvedBooleanInterfaceMethod,
  JVM_CONSTANT_ResolvedCharInterfaceMethod,
  JVM_CONSTANT_ResolvedFloatInterfaceMethod,
  JVM_CONSTANT_ResolvedDoubleInterfaceMethod,
  JVM_CONSTANT_ResolvedByteInterfaceMethod,
  JVM_CONSTANT_ResolvedShortInterfaceMethod,
  JVM_CONSTANT_ResolvedIntInterfaceMethod,
  JVM_CONSTANT_ResolvedLongInterfaceMethod,
  JVM_CONSTANT_ResolvedObjectInterfaceMethod,
  JVM_CONSTANT_ResolvedArrayInterfaceMethod,
  JVM_CONSTANT_ResolvedVoidInterfaceMethod,

  JVM_CONSTANT_ResolvedUncommonInterfaceMethod,
  JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod,

  CONSTANT_POOL_TAG_LIMIT
};

/**
 * Wrappers for Java class file constant tag queries.
 */
class ConstantTag {
 public:
  static bool is_resolved_klass(jubyte tag) {
    return tag == JVM_CONSTANT_Class;
  }
  static bool is_unresolved_klass(jubyte tag) {
    return tag == JVM_CONSTANT_UnresolvedClass;
  }
  static bool is_klass(jubyte tag) {
    return is_resolved_klass(tag) || is_unresolved_klass(tag);
  }
  static bool is_field (jubyte tag) {
    return tag == JVM_CONSTANT_Fieldref;
  }
  static bool is_method(jubyte tag) {
    return tag == JVM_CONSTANT_Methodref;
  }
  static bool is_interface_method(jubyte tag) {
    return tag == JVM_CONSTANT_InterfaceMethodref;
  }
  static bool is_string(jubyte tag) {
    return tag == JVM_CONSTANT_String;
  }
  static bool is_int(jubyte tag) {
    return tag == JVM_CONSTANT_Integer;
  }
  static bool is_float(jubyte tag) {
    return tag == JVM_CONSTANT_Float;
  }
  static bool is_long(jubyte tag) {
    return tag == JVM_CONSTANT_Long;
  }
  static bool is_double(jubyte tag) {
    return tag == JVM_CONSTANT_Double;
  }
  static bool is_name_and_type(jubyte tag) {
    return tag == JVM_CONSTANT_NameAndType;
  }
  static bool is_utf8(jubyte tag) {
    return tag == JVM_CONSTANT_Utf8;
  }
  static bool is_invalid(jubyte tag) {
    return tag == JVM_CONSTANT_Invalid;
  }

  static bool is_resolved_field(jubyte tag) {
    return (JVM_CONSTANT_ResolvedBooleanFieldref <= tag &&
            tag <=   JVM_CONSTANT_ResolvedStaticArrayFieldref);
  }
  static BasicType resolved_field_type(jubyte tag) {
    return (BasicType)
      ((tag < JVM_CONSTANT_ResolvedStaticBooleanFieldref ?
        (tag - JVM_CONSTANT_ResolvedBooleanFieldref + T_BOOLEAN) :
        (tag - JVM_CONSTANT_ResolvedStaticBooleanFieldref + T_BOOLEAN)));
  }

  static bool is_resolved_static_field(jubyte tag) {
    return (tag >= JVM_CONSTANT_ResolvedStaticBooleanFieldref);
  }

  static bool is_resolved_static_method(jubyte tag) { 
    return (tag == JVM_CONSTANT_ResolvedStaticMethod);
  }

  static bool is_resolved_virtual_method(jubyte tag) {
    return (JVM_CONSTANT_ResolvedBooleanVirtualMethod <= tag 
            && tag <= JVM_CONSTANT_ResolvedVoidVirtualMethod);
  }

  static BasicType resolved_virtual_method_type(jubyte tag) {
    return (BasicType)
        (tag - JVM_CONSTANT_ResolvedBooleanVirtualMethod + T_BOOLEAN);
  }

  static bool is_resolved_interface_method(jubyte tag) { 
    return (JVM_CONSTANT_ResolvedBooleanInterfaceMethod <= tag &&
            tag <= JVM_CONSTANT_ResolvedVoidInterfaceMethod);
  }
  static BasicType resolved_interface_method_type(jubyte tag) {
    return (BasicType)
        (tag - JVM_CONSTANT_ResolvedBooleanInterfaceMethod + T_BOOLEAN);
  }

  static bool is_resolved_uncommon_interface_method(jubyte tag) { 
    // See comments in ConstantPool::resolve_invoke_interface_at about
    // uncommon interface methods.
    return (tag == JVM_CONSTANT_ResolvedUncommonInterfaceMethod ||
            tag == JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod);
  }

  static bool is_resolved_final_uncommon_interface_method(jubyte tag) { 
   return (tag == JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod);
  }

  static bool is_klass_index(jubyte tag) {
    return tag == JVM_CONSTANT_ClassIndex;
  }
  static bool is_unresolved_string(jubyte tag) {
    return tag == JVM_CONSTANT_UnresolvedString;
  }
  static bool is_string_index(jubyte tag) {
    return tag == JVM_CONSTANT_StringIndex;
  }

  static bool is_klass_reference(jubyte tag) {
    return is_klass_index(tag) || is_unresolved_klass(tag);
  }
  static bool is_field_or_method(jubyte tag) {
    return is_field(tag) || is_method(tag) || is_interface_method(tag);
  }
  static bool is_symbol(jubyte tag) {
    return is_utf8(tag);
  }

  static bool is_oop(jubyte tag) { 
    return (is_utf8(tag) || 
            is_unresolved_klass(tag) ||
            is_string(tag) ||
            is_unresolved_string(tag) ||   
            is_resolved_static_method(tag) ||
            is_resolved_final_uncommon_interface_method(tag));
  }
  static bool is_short_pair(jubyte tag) { 
    return (is_resolved_field(tag) ||
            is_resolved_virtual_method(tag) ||
            is_resolved_interface_method(tag));
  }

  static bool is_valid_tag(jubyte tag) {
    return (tag < CONSTANT_POOL_TAG_LIMIT);
  }

  ConstantTag(jubyte tag) { 
    GUARANTEE(is_valid_tag(tag), "Invalid constant tag");
    _tag = tag; 
  }

  jubyte value() {
    return _tag;
  }

  // Convenience instance methods: these are slower than the static version
  // above, so don't use them in tight loops!

  bool is_resolved_klass() const {
     return is_resolved_klass(_tag);
  }
  bool is_unresolved_klass() const {
    return is_unresolved_klass(_tag);
  }
  bool is_klass() const {
    return is_klass(_tag);
  }
  bool is_field () const {
    return is_field(_tag);
  }
  bool is_method() const {
    return is_method(_tag);
  }
  bool is_interface_method() const {
    return is_interface_method(_tag);
  }
  bool is_string() const {
    return is_string(_tag);
  }
  bool is_int() const {
    return is_int(_tag);
  }
  bool is_float() const {
    return is_float(_tag);
  }
  bool is_long() const {
    return is_long(_tag);
  }
  bool is_double() const {
    return is_double(_tag);
  }
  bool is_name_and_type() const {
    return is_name_and_type(_tag);
  }
  bool is_utf8() const {
    return is_utf8(_tag);
  }
  bool is_invalid() const {
    return is_invalid(_tag);
  }
  bool is_resolved_field() const {
    return is_resolved_field(_tag);
  }
  bool is_resolved_static_field() const {
    return is_resolved_static_field(_tag);
  }
  bool is_resolved_static_method() const {
    return is_resolved_static_method(_tag);
  }
  bool is_resolved_virtual_method() const {
    return is_resolved_virtual_method(_tag);
  }
  bool is_resolved_interface_method() const {
    return is_resolved_interface_method(_tag);
  }
  bool is_resolved_uncommon_interface_method() const {
    return is_resolved_uncommon_interface_method(_tag);
  }
  bool is_resolved_final_uncommon_interface_method() const {
    return is_resolved_final_uncommon_interface_method(_tag);
  }
  bool is_klass_index() const {
    return is_klass_index(_tag);
  }
  bool is_unresolved_string() const {
    return is_unresolved_string(_tag);
  }
  bool is_string_index() const {
    return is_string_index(_tag);
  }
  bool is_klass_reference() const {
    return is_klass_reference(_tag);
  }
  bool is_field_or_method() const {
    return is_field_or_method(_tag);
  }
  bool is_symbol() const {
    return is_symbol(_tag);
  }
  bool is_oop() const {
    return is_oop(_tag);
  }
  bool is_short_pair() const { 
    return is_short_pair(_tag);
  }
  BasicType resolved_field_type() const {
    return resolved_field_type(_tag);
  }
  BasicType resolved_virtual_method_type() const {
    return resolved_virtual_method_type(_tag);
  }
  BasicType resolved_interface_method_type() const {
    return resolved_interface_method_type(_tag);
  }

  void print_on(Stream*) const PRODUCT_RETURN;
  static char * name_for(jubyte /*tag*/, bool /*use_prefix*/) PRODUCT_RETURN0;
  static void print_definitions() PRODUCT_RETURN;
 private:
  jubyte _tag;
};
