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

// An Instance is a handle to a Java object.
// Example: new java.lang.Object();

class String : public Instance {
 public:
  static int value_offset()  { return header_size() + 0; }
  void set_value(Oop* value)  { obj_field_put(value_offset(), value);  }

  HANDLE_DEFINITION(String, Instance);

 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int offset_offset() {
    return header_size() + sizeof(jobject);
  }
  static int count_offset(){
    return header_size() + sizeof(jobject) + sizeof(jint);
  }

 public:
  // ^TypeArray
  ReturnOop value() {
    return obj_field(value_offset());
  }

  jint offset() {
    return int_field(offset_offset());
  }
  void set_offset(jint value) {
    int_field_put(offset_offset(), value);
  }

  jint count() {
    return int_field(count_offset());
  }
  void set_count(jint value) {
    int_field_put(count_offset(), value);
  }

  jint length() {
    return count();
  }
  void set_length(jint value) {
    set_count(value);
  }

  bool matches(String *other_string);
  juint hash();

  // Return the content of the string as a NUL-terminated "C" string
  // (the upper byte of each character is stripped, and a NUL character
  // is appended to the end).
  ReturnOop to_cstring(JVM_SINGLE_ARG_TRAPS);

  void print_string_on(Stream* st, int max_len=-1);
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_value_on(Stream* /*st*/);
#endif

  jchar char_at(int index);

  jint last_index_of(jchar ch, jint fromIndex);
};
