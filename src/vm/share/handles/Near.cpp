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
# include "incls/_Near.cpp.incl"

#ifndef PRODUCT

void Near::iterate(OopVisitor* visitor) {
  Oop::iterate(visitor);
}

void Near::print_value_on(Stream* st) {
  st->print("Near");
}

#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int Near::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  //_klass
  field_map->byte_at_put(map_index++, T_OBJECT);
#if ENABLE_OOP_TAG
  if (sizeof(OopDesc) > 4) {
    for (int i = 4; i < sizeof(OopDesc); i+=4) {
      field_map->byte_at_put(map_index++, T_INT);
    }
  }
#endif
  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */
