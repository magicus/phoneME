/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_RefArray.cpp.incl"

HANDLE_CHECK(RefArray, is_type_array())

int RefArray::add(Oop* referent, const int type JVM_TRAPS) {
  GUARANTEE(referent->not_null(), "NULL not supported for reference!");
  const int length = this->length();

  // look for an empty slot in the ref array
  int i;
  for( i = 0;; i++ ) {
    if( i == length ) {
      // reallocate ref array if full
#ifdef AZZERT
      handle_uniqueness_verification();
#endif
      RefArray::Raw copy = Universe::new_int_array(length * 2 JVM_NO_CHECK);
      if( copy.is_null() ) {
        Thread::clear_current_pending_exception();
        return -1;
      }
      jvm_memcpy( copy().base(), base(), length * sizeof(juint) );
      set_obj( copy.obj() );
      *current() = copy;
#if ENABLE_ISOLATES
      Task::current()->set_global_references(copy);
#endif
      break;
    }
    if( obj_at(i) == NULL ) {
      break;
    }
  }

  set_obj_at( i, make( referent->obj(), type ) );
  return i;
}

void RefArray::oops_do(void do_oop(OopDesc**), const int mask) {
  if( not_null() ) {
    OopDesc** p = base();
    for( OopDesc** const max = p + length(); p < max; p++ ) {
      OopDesc* obj = *p;
      if( not_null_or_dead( obj ) ) {
        const unsigned type = get_type( obj );
        if( type & mask ) {
          *p = get_value( obj );
          do_oop( p );
          obj = *p;
          if( not_null_or_dead( obj ) ) {
            *p = make( obj, type );
          }
        }
      }
    }
  }
}

void RefArray::clear_non_marked( OopDesc** p ) {
  if( ObjectHeap::in_collection_area_unmarked( *p ) ) {
    *p = dead();  // object is not marked. It will be GC'ed
  }
}
