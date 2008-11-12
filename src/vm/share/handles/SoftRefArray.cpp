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

#if USE_SOFT_REFERENCES
# include "incls/_precompiled.incl"
# include "incls/_SoftRefArray.cpp.incl"

HANDLE_CHECK(SoftRefArray, is_type_array())

ReturnOop SoftRefArray::create(int length JVM_TRAPS) {
  GUARANTEE(length > 0, "Invalid length");
  // Reserve the first element for null references
  Array::Raw array = Universe::new_int_array(length + 1 JVM_CHECK_0);
  address base = array().base_address();
  memset(base + sizeof(jint), -1, length * sizeof(jint));
  return array.obj();
}

void SoftRefArray::oops_do(void do_oop(OopDesc**)) {
  if( not_null() ) {
    OopDesc** p = base();
    for( OopDesc** const max = p + length(); p < max; p++ ) {
      OopDesc* obj = *p;
      if (not_null_or_unused(obj)) {
        unsigned counter = get_counter( *p );
        *p = get_value( *p );
        do_oop( p );
        *p = make(*p, counter);
      }
    }
  }
}

void SoftRefArray::mark(bool is_full_collect) {
  if( not_null() ) {
    const int decrement = is_full_collect ? 1 : 0;
    const unsigned max = max_counter();
    OopDesc** p = base();
    for( OopDesc** const limit = p + length(); p < limit; p++ ) {
      OopDesc* obj = *p;
      if( ObjectHeap::in_collection_area( (OopDesc**)obj )) {
        const unsigned counter = get_counter( obj );
        obj = get_value( obj );
        if( ObjectHeap::test_bit_for( (OopDesc**) obj ) ) {
          if( counter < max ) {
            *p = make(obj, counter+1);
          }
        } else {
          if( counter > 0 ) {
            *p = obj;
            ObjectHeap::mark_root_and_stack( p );
            *p = make(obj, counter-decrement);
          }
        }
      }
    }
  }
}

void SoftRefArray::clear_non_marked( OopDesc** p ) {
  if( ObjectHeap::in_collection_area_unmarked( *p ) ) {
    *p = dead();  // object is not marked. It will be GC'ed
  }
}
#endif
