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

// IMPL_NOTE:
// The various occurrences  of ObjectHeap::is_gc_active() is so that this
// code can be called from GenerateStackMaps during the middle of GC.
// Can we find a better way?

class TypeArray: public Array {
 public:
  HANDLE_DEFINITION_CHECK(TypeArray, Array);
  ~TypeArray() {}

 protected:
  int offset_from_byte_index(int index) const { 
    GUARANTEE( ObjectHeap::is_gc_active() || is_byte_array(),
              "Array must be byte array");
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jbyte));
  }
  int offset_from_ubyte_index(int index) const { 
    GUARANTEE(ObjectHeap::is_gc_active() || is_byte_array(),
               "Array must be byte array");
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jubyte));
  }
  int offset_from_bool_index(int index) const { 
    GUARANTEE(is_bool_array(), "Array must be bool array");
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jboolean));
  }
  int offset_from_char_index(int index) const { 
    GUARANTEE(is_char_array(), "Array must be char array");
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jchar));
  }
  int offset_from_int_index(int index) const { 
    GUARANTEE(is_int_array(), "Array must be int array");   
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(int));
  }
  int offset_from_short_index(int index) const {
    GUARANTEE(ObjectHeap::is_gc_active() || is_short_array(),
              "Array must be short array"); 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jshort));
  }
  int offset_from_ushort_index(int index) const { 
    GUARANTEE(ObjectHeap::is_gc_active() || is_short_array(),
              "Array must be short array"); 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jushort));
  }
  int offset_from_long_index(int index) const { 
    GUARANTEE(is_long_array(), "Array must be long array"); 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jlong));
  }
  int offset_from_float_index(int index) const { 
    GUARANTEE(is_float_array(), "Array must be float array"); 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jfloat));
  }
  int offset_from_double_index(int index) const { 
    GUARANTEE(is_double_array(), "Array must be double array"); 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jdouble));
  }

 public:
  jbyte byte_at(int index) const {
    return byte_field(offset_from_byte_index(index));
  }
  void byte_at_put(int index, jbyte value) {
    byte_field_put(offset_from_byte_index(index), value);
  }

  jubyte ubyte_at(int index) const {
    return ubyte_field(offset_from_ubyte_index(index));
  }
  void ubyte_at_put(int index, jubyte value) {
    ubyte_field_put(offset_from_ubyte_index(index), value);
  }

  jboolean bool_at(int index) const {
    return bool_field(offset_from_bool_index(index));
  }
  void bool_at_put(int index, jboolean value) {
    bool_field_put(offset_from_bool_index(index), value);
  }

  jchar char_at(int index) const {
    return char_field(offset_from_char_index(index));
  }
  void char_at_put(int index, jchar value) {
    char_field_put(offset_from_char_index(index), value);
  }

  jint int_at(int index) const {
    return int_field(offset_from_int_index(index));
  }
  void int_at_put(int index, jint value) {
    int_field_put(offset_from_int_index(index), value);
  }

  jshort short_at(int index) const {
    return short_field(offset_from_short_index(index));
  }
  void short_at_put(int index, jshort value) {
    short_field_put(offset_from_short_index(index), value);
  }

  jushort ushort_at(int index) const {
    return ushort_field(offset_from_ushort_index(index));
  }
  void ushort_at_put(int index, jushort value) {
    ushort_field_put(offset_from_ushort_index(index), value);
  }

  jlong long_at(int index) const {
    return long_field(offset_from_long_index(index));
  }
  void long_at_put(int index, jlong value) {
    long_field_put(offset_from_long_index(index), value);
  }

  jfloat float_at(int index) const {
    return float_field(offset_from_float_index(index));
  }
  void float_at_put(int index, jfloat value) {
    float_field_put(offset_from_float_index(index), value);
  }

  jdouble double_at(int index) const {
    return double_field(offset_from_double_index(index));
  }
  void double_at_put(int index, jdouble value) {
   double_field_put(offset_from_double_index(index), value);
  }

  // dispatch from Java_java_lang_System_arraycopy
  static void array_copy(TypeArray* src, jint src_pos, 
                         TypeArray* dst, jint dst_pos, jint length);

  // Operations for char array
  // Tells whether the char array includes c
  void *data() {
    return ((ArrayDesc*)obj())->data();
  }

  jbyte *byte_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_byte_array(), "type check");
    return (jbyte*)base_address();
  }
  jubyte *ubyte_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_byte_array(), "type check");
    return (jubyte*)base_address();
  }
  jshort *short_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_short_array(), "type check");
    return (jshort*)base_address();
  }
  jushort *ushort_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_short_array(), "type check");
    return (jushort*)base_address();
  }
  jchar *char_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_char_array(), "type check");
    return (jchar*)base_address();
  }
  jint *int_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_int_array(), "type check");
    return (jint*)base_address();
  }
  juint *uint_base_address() {
    GUARANTEE(ObjectHeap::is_gc_active() || is_int_array(), "type check");
    return (juint*)base_address();
  }

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* s);
#endif

};
