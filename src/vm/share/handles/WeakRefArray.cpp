/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_WeakRefArray.cpp.incl"

HANDLE_CHECK(WeakRefArray, is_type_array())

ReturnOop WeakRefArray::create(int length JVM_TRAPS) {
  GUARANTEE(length > 0, "Invalid length");
  // Reserve the first element for null references
  Array::Raw array = Universe::new_int_array(length + 1 JVM_CHECK_0);
  address base = array().base_address();
  memset(base + sizeof(jint), -1, length * sizeof(jint));
  return array.obj();
}

void WeakRefArray::oops_do(void do_oop(OopDesc**)) {
  if( not_null() ) {
    OopDesc** p = base();
    for( OopDesc** const max = p + length(); p < max; p++ ) {
      OopDesc* obj = *p;
      if (not_null_or_unused(obj)) {
        do_oop( p );
      }
    }
  }
}

void WeakRefArray::clear_non_marked( OopDesc** p ) {
  if( ObjectHeap::in_collection_area_unmarked( *p ) ) {
    *p = dead();  // object is not marked. It will be GC'ed
  }
}
