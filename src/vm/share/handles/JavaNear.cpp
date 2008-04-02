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
# include "incls/_JavaNear.cpp.incl"

// Bit-format of value (most significant first):
//
//  hash:30 lock:2 = 32 bits 
//
//  - hash contains the identity hash value.
//    0 is sentinel value, no hash value
//
//  - the two lock bits are used to describe three states: locked/unlocked and monitor.
//    00 means object is unlocked
//    01 means object is locked but no contention
//    11 means object is locked with contention

HANDLE_CHECK(JavaNear, is_java_near())

void JavaNear::set_hash(jint hash_value) {
  GUARANTEE(!has_hash(), "can only set hash once");
  jint rest = raw_value() & (~hash_mask_in_place);
  set_raw_value(rest | (hash_value << hash_shift));
  GUARANTEE(has_hash(), "hash must be valid");
}

void JavaNear::set_lock(jint lock_value) {
  jint rest = raw_value() & (~lock_mask_in_place);
  set_raw_value(rest | (lock_value << lock_shift));
}

#ifndef PRODUCT

void JavaNear::print_value_on(Stream* st) {
  if (is_locked()) {
    st->print("locked ");
  }
  st->print("JavaNear");
  if (has_hash()) {
    st->print(" [hash = %d]", hash_value());
  }
  JavaClass::Raw klass     = this->klass();
  JavaNear::Raw  protoNear = klass().prototypical_near();
  if (this->equals(&protoNear)) { 
    st->print(" prototype for ");
    klass().print_name_on(st);
  }
}

void JavaNear::iterate(OopVisitor* visitor) {
  Oop::iterate(visitor);
  {
     NamedField id("class_info", true);
     visitor->do_oop(&id, class_info_offset(), true);
  }
  if (Verbose && (raw_value() != 0)) {
     NamedField id("raw value", true);
     visitor->do_int(&id, raw_value_offset(), true);
  }

}

#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int JavaNear::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  map_index = Near::generate_fieldmap(field_map);
  
  // _class_info
  field_map->byte_at_put(map_index++, T_OBJECT);
  // _value
  field_map->byte_at_put(map_index++, T_INT);

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */
