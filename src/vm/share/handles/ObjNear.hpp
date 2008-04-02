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

// ObjNear describes near objects with an extra object field.
// Methods use ObjNear to store the exception handler table.

class ObjNear: public Near {
 public:
  HANDLE_DEFINITION_CHECK(ObjNear, Near);

 public:
  static int object_offset() {
    return FIELD_OFFSET(ObjNearDesc, _object);
  }

 public:
  // ^Oop
  ReturnOop object() const {
    return obj_field(object_offset());
  }
  void set_object(Oop* value) {
    obj_field_put(object_offset(), value);
  }

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* st);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
 
};
