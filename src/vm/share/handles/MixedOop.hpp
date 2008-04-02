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

class MixedOop: public Oop {
 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static jint size_offset() {
    return FIELD_OFFSET(MixedOopDesc, _size);
  }
  static jint type_offset() {
    return FIELD_OFFSET(MixedOopDesc, _type);
  }
  static jint pointer_count_offset() {
    return FIELD_OFFSET(MixedOopDesc, _pointer_count);
  }

  const char *type_string();

 public:
  HANDLE_DEFINITION_CHECK(MixedOop, Oop);

  int type() const {
    return (int)(ubyte_field(type_offset()));
  }
  int object_size() const {
    return (int)(ushort_field(size_offset()));
  }

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
  void iterate(OopVisitor* visitor);
#endif

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

};
