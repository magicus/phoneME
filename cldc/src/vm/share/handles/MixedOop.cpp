/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_MixedOop.cpp.incl"

HANDLE_CHECK(MixedOop, is_mixed_oop())

#define MIXED_OOP_TYPE_STRING(x, y) \
    case MixedOopDesc::Type_ ## x: return STR(x);

#if !defined(PRODUCT)
const char *MixedOop::type_string() {
  switch (type()) {
#if USE_DEBUG_PRINTING
    MIXED_OOP_TYPES_DO(MIXED_OOP_TYPE_STRING)
#endif
  default:
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }

}

void MixedOop::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);
  { 
    NamedField id("size", true);
    visitor->do_ushort(&id, size_offset(), true);
  }
  { 
    NamedField id("type", true);
    visitor->do_ubyte(&id, type_offset(), true);
  }
  { 
    NamedField id("pointer count", true);
    visitor->do_ubyte(&id, pointer_count_offset(), true);
  }
  switch (type()) {
  case MixedOopDesc::Type_JarFileParser:
    ((JarFileParser*)this)->iterate(visitor);
    break;
  case MixedOopDesc::Type_Task:
    ((Task*)this)->iterate(visitor);
    break;
  case MixedOopDesc::Type_Thread:
    ((Thread*)this)->iterate(visitor);
    break;
#if ENABLE_COMPILER && USE_COMPILER_LITERALS_MAP && ENABLE_THUMB_COMPILER
  // IMPL_NOTE: support SH and ARM compiler as well.
  case MixedOopDesc::Type_LiteralPoolElement:
    ((BinaryAssembler::LiteralPoolElement*)this)->iterate(visitor);
    break;
#endif
  // IMPL_NOTE: fill in the other cases!
  }
#endif
}

#endif /* PRODUCT || USE_PRODUCT_BINARY_IMAGE_GENERATOR */

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int MixedOop::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  //klass
  map_index = Near::generate_fieldmap(field_map);

  //size
  field_map->byte_at_put(map_index++, T_SHORT);
  //type table
  field_map->byte_at_put(map_index++, T_BYTE);
  //pointer_count
  field_map->byte_at_put(map_index++, T_BYTE);

  switch (type()) {
  case MixedOopDesc::Type_Condition:
    field_map->byte_at_put(map_index++, T_OBJECT);
    field_map->byte_at_put(map_index++, T_OBJECT);
    field_map->byte_at_put(map_index++, T_OBJECT);
    field_map->byte_at_put(map_index++, T_OBJECT);
    break;
  default:
    UNIMPLEMENTED();
    return -1;
  }

  return map_index;
}

#endif
