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
// EntryActivation is used for implementing delayed execution of Java code.
// The data structure captures a method and a number of parameters sufficient
// for invoking the method with the parameters at a later point.
// The primary use is to avoid call backs from the runtime system to Java, a
// key property when implementing light weight threads
//

class EntryActivation: public Oop {
 public:
  HANDLE_DEFINITION_CHECK(EntryActivation, Oop);

  static int length_offset() {
    return FIELD_OFFSET(EntryActivationDesc, _length);
  }
  static int method_offset() {
    return FIELD_OFFSET(EntryActivationDesc, _method);
  }
  static int next_offset() {
    return FIELD_OFFSET(EntryActivationDesc, _next);
  }
#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  static int return_point_offset() {
    return FIELD_OFFSET(EntryActivationDesc, _return_point);
  }

  // Return Type accessor
  void set_return_point(address value) {
    ((EntryActivationDesc*) obj())->_return_point = value;
  }
#endif

  // Returns the length
  jint length() const {
   return int_field(length_offset());
  }

 private:
  jint tag_offset(int index) const {
    return EntryActivationDesc::tag_offset(index, length());
  }
  jint value_offset(int index) const {
    return EntryActivationDesc::value_offset(index, length());
  }

 public:
  // method accessors
  ReturnOop method() const {
   return obj_field(method_offset());
  }
  void set_method(Method* value) {
   obj_field_put(method_offset(), (Oop*) value);
  }

  // next accessors
  ReturnOop next() const {
   return obj_field(next_offset());
  }
  void set_next(EntryActivation* value) {
   obj_field_put(next_offset(), (Oop*) value);
  }

  // Tag accessors
  jint tag_at(jint index) {
   return int_field(tag_offset(index));
  }
  void tag_at_put(jint index, jint value) {
   int_field_put(tag_offset(index), value);
  }

  // int accessors
  jint int_at(jint index) { 
    GUARANTEE(tag_at(index) == int_tag, "Must be int");
    return int_field(value_offset(index)); 
  }

  void int_at_put(jint index, jint value) {
    tag_at_put(index, int_tag);
    int_field_put(value_offset(index), value);
  }

  // float accessors
  jfloat float_at(jint index) { 
    GUARANTEE(tag_at(index) == float_tag, "Must be float");
    return float_field(value_offset(index)); 
  }

  void float_at_put(jint index, jfloat value) {
    tag_at_put(index, float_tag);
    float_field_put(value_offset(index), value);
  }

  // double accessors
  jdouble double_at(jint index) { 
    jint word0 = int_field(value_offset(index));
    jint word1 = int_field(value_offset(index + 1));
    if (JavaStackDirection > 0) {
      return jdouble_from_low_high(word0, word1);
    } else {
      return jdouble_from_low_high(word1, word0);
    }
  }

  void double_at_put(jint index, jdouble value) {
    jdouble_accessor result;
    result.double_value = value;
    tag_at_put(index,   double_tag);
    tag_at_put(index+1, double2_tag);
    if (JavaStackDirection > 0) { 
      int_field_put(value_offset(index),     result.words[0]);
      int_field_put(value_offset(index + 1), result.words[1]);
    } else {
      int_field_put(value_offset(index),     result.words[1]);
      int_field_put(value_offset(index + 1), result.words[0]);
    }
  }

  // obj accessors
  ReturnOop obj_at(jint index) { 
    GUARANTEE(tag_at(index) == obj_tag, "Must be obj");
    return obj_field(value_offset(index)); 
  }

  void obj_at_put(jint index, Oop* value) {
    tag_at_put(index, obj_tag);
    obj_field_put(value_offset(index), value);
  }

  // long accessors
  jlong long_at(jint index) { 
    jint word0 = int_field(value_offset(index));
    jint word1 = int_field(value_offset(index + 1));
    if (JavaStackDirection > 0) {
      return jlong_from_low_high(word0, word1);
    } else {
      return jlong_from_low_high(word1, word0);
    }
  }

  void long_at_put(jint index, jlong value) {
    jlong_accessor result;
    result.long_value = value;
    tag_at_put(index,   long_tag);
    tag_at_put(index+1, long2_tag);
    if (JavaStackDirection > 0) { 
      int_field_put(value_offset(index),     result.words[0]);
      int_field_put(value_offset(index + 1), result.words[1]);
    } else {
      int_field_put(value_offset(index),     result.words[1]);
      int_field_put(value_offset(index + 1), result.words[0]);
    }
  }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_value_on(Stream* s);
  void print_list_on(Stream* s, int indent, int index);
  void iterate(OopVisitor* visitor);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif
};
