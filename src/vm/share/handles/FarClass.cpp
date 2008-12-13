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
# include "incls/_FarClass.cpp.incl"

#ifndef PRODUCT

void FarClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);
  if (!Verbose) return;
  { 
    NamedField id("object size", true);
    visitor->do_ushort(&id, object_size_offset(), true);
  }
  { 
    NamedField id("instance size", true);
    visitor->do_short(&id, instance_size_offset(), true);
  }
  { 
    NamedField id("oop map", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, oop_map_offset(), true);
  }
  if (!is_instance_class()) {
    jubyte *map = ((FarClassDesc*)obj())->extern_oop_map();
    char buffer[500];
    char *p = buffer;
    jvm_sprintf(buffer, "External oop map = "); p += jvm_strlen(p);
    for (int i = 0; ; i++) {
      jvm_sprintf(p, "[%d]=%d ", i, map[i]);
      p += jvm_strlen(p);
      if (map[i] == 0) break;
    }
    visitor->do_comment(buffer);
  }
  { 
    NamedField id("prototypical near", true);
    visitor->do_oop(&id, prototypical_near_offset(), true);
  }
#endif
}

void FarClass::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  object_size);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  instance_size);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    oop_map);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, prototypical_near);
#endif
}
#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int FarClass::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  // near
  map_index = Near::generate_fieldmap(field_map);

  // _object_size
  field_map->byte_at_put(map_index++, T_SHORT);

  // _instance_size
  field_map->byte_at_put(map_index++, T_SHORT);

  // oopmap
  if (!is_instance_class()) {
    field_map->byte_at_put(map_index++, T_SYMBOLIC);
  } else {
    field_map->byte_at_put(map_index++, T_INT);
  }

  // _prototypical_near
  field_map->byte_at_put(map_index++, T_OBJECT); 

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */
