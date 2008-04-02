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

// Interface to java.lang.Class objects

class JavaClassObj : public Instance {
 public:
  HANDLE_DEFINITION(JavaClassObj, Instance);

  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int java_class_offset() {
    return header_size() + 0;
  } 
  static int status_offset() {
    return header_size() + sizeof(jobject);
  }
  static int thread_offset() {
    return header_size() + 2* sizeof(jobject);
  }

#if ENABLE_REFLECTION
  static int type_symbol_offset() {
    return header_size() + 3* sizeof(jobject);
  }

  jchar type_symbol() const {
    return char_field(type_symbol_offset());
  }
  void set_type_symbol(jchar value) {
    char_field_put(type_symbol_offset(), value);
  }
  bool is_primitive() const {
    return type_symbol() != 0;
  }
#endif

  // ^JavaClass
  ReturnOop java_class() const {
    return obj_field(java_class_offset());
  }
  void set_java_class(JavaClass* value) {
    obj_field_put(java_class_offset(), value);
  }

  // Returns the initialization status of the class
  jint status() const {
    return int_field(status_offset());
  }
  void set_status(jint value) {
    int_field_put(status_offset(), value);
  }

  // ^ThreadObj
  ReturnOop thread() const {
    return obj_field(thread_offset());
  }
  void set_thread(ThreadObj* value) {
    obj_field_put(thread_offset(), value);
  }

  // Constants copied from java.lang.Class
  enum {
   IN_PROGRESS = 1,
   VERIFIED    = 2,
   INITIALIZED = 4,
   ERROR_FLAG  = 8
  };

  void set_verified() {
    set_status(status() | VERIFIED);
  }
  bool is_verified() const {
    return (status() & VERIFIED) != 0;
  }

  void set_initialized() {
    set_status(status() | INITIALIZED);
  }
  bool is_initialized() const {
    return (status() & INITIALIZED) != 0;
  }

  // Tells whether the class initialization is in program
  bool in_progress() const {
    return (status() & IN_PROGRESS) != 0;
  }

  void print_value_on(Stream* /*st*/) PRODUCT_RETURN;
  static void verify_fields() PRODUCT_RETURN;

};
