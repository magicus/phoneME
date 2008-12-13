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

class FarClass: public Oop {
public:
  HANDLE_DEFINITION(FarClass, Oop);
  ~FarClass() {}

 public:
  // To avoid endless lists of friends the static offset computation 
  // routines are all public.
  static int prototypical_near_offset() {
    return FIELD_OFFSET(FarClassDesc, _prototypical_near);
  }
  static int object_size_offset() {
    return FIELD_OFFSET(FarClassDesc, _object_size);
  }
  static int instance_size_offset() {
    return FIELD_OFFSET(FarClassDesc, _instance_size);
  }
  static int oop_map_offset() {
    return FIELD_OFFSET(FarClassDesc, _oop_map);
  }

 public:
  InstanceSize instance_size() const {
    InstanceSize is(short_field(instance_size_offset()));
    return is;
  }

  jint instance_size_as_jint() const {
    return (jint)short_field(instance_size_offset());
  }

#if ENABLE_ROM_GENERATOR
  // Used only by source ROM generator to delete unused static fields
  void set_object_size(jushort new_size) {
    ushort_field_put(object_size_offset(), new_size);
  }
#endif

  // ^Oop
  ReturnOop prototypical_near() const {
    return obj_field(prototypical_near_offset());
  }
  void set_prototypical_near(Oop* value) { 
    obj_field_put(prototypical_near_offset(), value);
  }

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif

  // Offset where embedded oop maps start
  size_t embedded_oop_map_start() const {
    return uint_field(oop_map_offset());
  }
  void set_embedded_oop_map_start(size_t value) {
    uint_field_put(oop_map_offset(), value);
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

  jubyte* embedded_oop_map() const {
    return ((FarClassDesc*)obj())->embedded_oop_map();
  }
};
