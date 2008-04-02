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

// This type is used by ObjectHeap.cpp to implement global references
// Each slot contains one of the following
//
// array[i] = NULL       -> This slot is not used: No reference has index i
// array[i] = obj | 0x01 -> It contains a strong reference whose index is i
// array[i] = obj | 0x02 -> It contains a weak reference whose index is i. 
//                          The object HAS NOT been GC'ed
// array[i] = 0xffffffff -> It contains a weak reference whose index is i. 
//                          The object HAS been GC'ed

class RefArray: public Array {
 public:
  HANDLE_DEFINITION_CHECK(RefArray, Array);

 private:
  void set_obj( OopDesc* value ) {
    Array::set_obj( value );
  }

  enum {
    TypeMask = 0x3,
    ValueMask = ~TypeMask
  };
  static unsigned get_type ( const OopDesc* obj ) {
    return unsigned(obj) & TypeMask;
  }
  static OopDesc* get_value ( OopDesc* obj ) {
    return (OopDesc*) (unsigned(obj) & ValueMask);
  }
  static OopDesc* make ( OopDesc* obj, const unsigned type ) {
    return (OopDesc*) (unsigned(obj) | type);
  }
  static OopDesc* null ( void ) {
    return (OopDesc*) NULL;
  }
  static OopDesc* dead ( void ) {
    return (OopDesc*) -1;
  }
  static bool not_null_or_dead( OopDesc* obj ) {
    return unsigned(unsigned(obj)+1) > unsigned(1);
  }

  OopDesc** base( void ) const {
    return DERIVED( OopDesc**, obj(), base_offset() );
  }

  OopDesc** obj_addr_at(const int index) const {
    GUARANTEE( index >= 0 && index < length(), "sanity");
    return base() + index;
  }

  OopDesc* obj_at(const int index) const {
    return *obj_addr_at(index);
  }

  void set_obj_at(const int index, OopDesc* obj) {
    *obj_addr_at(index) = obj;
  }

 public:
  static RefArray* current( void ) {
    return Universe::global_refs_array();
  }

  static ReturnOop initialize(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_int_array(5 JVM_NO_CHECK_AT_BOTTOM);
  }

  // Can allocate, but cannot throw exceptions
  int add(Oop* referent, const int type JVM_TRAPS);

  void remove(const int index) {
    set_obj_at( index, null() );
    // IMPL_NOTE: shrink array if possible
  }

  OopDesc* get(const int index) const {
    OopDesc* obj = obj_at( index );
    return obj == dead() ? null() : get_value( obj );
  }

  void oops_do(void do_oop(OopDesc**), const int mask);
  static void clear_non_marked( OopDesc** p );
};
