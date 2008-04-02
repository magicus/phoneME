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

# include "incls/_precompiled.incl"
# include "incls/_Array.cpp.incl"

#if ENABLE_REFLECTION
ReturnOop Array::shrink(int new_length) {
  GUARANTEE(new_length >= 0, "Negative array size prohibited");
  int scale;
  if (is_type_array()) {
    TypeArrayClass::Raw array_class = blueprint();
    scale = array_class().scale();
  } else {
    GUARANTEE(is_obj_array(), "sanity");
    scale = sizeof(OopDesc*);
  }

  Universe::shrink_object(this, ArrayDesc::allocation_size(new_length, scale));
  set_length(new_length);
  return obj();
}
#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int Array::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  // near
  map_index = Near::generate_fieldmap(field_map);

  //_length
  field_map->byte_at_put(map_index++, T_INT);

  // map size for the elements
  jint elements_map_size = length();
  if (map_index + elements_map_size > field_map->length()) {
    // need more space
    return map_index + elements_map_size;
  }

  int type;
  if (is_obj_array()) {
    type = T_OBJECT;
  } else {
    TypeArrayClass::Raw cls = blueprint();
    type = cls().type();
  }

  for (int i = 0; i < elements_map_size; i++) {
    field_map->byte_at_put(map_index++, type);
  }

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */
