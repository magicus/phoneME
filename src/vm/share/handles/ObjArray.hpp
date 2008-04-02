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

// An ObjArray is an array containing Oops

class ObjArray: public Array {
 private:
  int offset_from_index(int index) const { 
    GUARANTEE(is_within_bounds(index), "Array index out of bounds");
    return base_offset() + (index * sizeof(jobject));
  }

 public:
  HANDLE_DEFINITION_CHECK(ObjArray, Array);

  // Accessors
  ReturnOop obj_at(int index) const {
    return obj_field(offset_from_index(index));
  }

  void obj_at_put(int index, OopDesc* value);
    
  void obj_at_put(int index, Oop* value) { 
    obj_at_put(index, value->obj());
  }

  void obj_at_clear(int index) { 
    obj_field_clear(offset_from_index(index));
  }

  static void fill_zero(OopDesc* array);

  // dispatch from Java_java_lang_System_arraycopy
  static void array_copy(ObjArray* src, jint src_pos, ObjArray* dst, jint dst_pos, jint length JVM_TRAPS);

  // Printing
#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* s);
#endif
};
