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

// Array of soft references

#if USE_SOFT_REFERENCES
class SoftRefArray: public Array {
 public:
  HANDLE_DEFINITION_CHECK(SoftRefArray, Array);

  static unsigned get_counter ( const OopDesc* obj ) {
    return unsigned(obj) & CounterMask;
  }
  static OopDesc* get_value ( OopDesc* obj ) {
    return (OopDesc*) (unsigned(obj) & ValueMask);
  }
  static OopDesc* make ( OopDesc* obj, const unsigned counter ) {
    return (OopDesc*) (unsigned(obj) | counter);
  }

 private:
  void set_obj( OopDesc* value ) {
    Array::set_obj( value );
  }

  const static int max_counter() {
    return CounterMask;
  }
  
  enum {
    CounterMask = 0x3,
    ValueMask = ~CounterMask
  };
  static bool not_null_or_unused( OopDesc* obj ) {
    return unsigned(unsigned(obj)+1) > unsigned(1);
  }

  OopDesc** base( void ) const {
    return DERIVED( OopDesc**, obj(), base_offset() );
  }

  OopDesc** obj_addr_at(const int index) const {
    GUARANTEE( index >= 0 && index < length(), "sanity");
    return base() + index;
  }

 public:
  static OopDesc* unused ( void ) {
    return (OopDesc*) -1;
  }
  static OopDesc* dead ( void ) {
    return (OopDesc*) NULL;
  }

  static ReturnOop create(JVM_SINGLE_ARG_TRAPS) {
    return create(16 JVM_NO_CHECK_AT_BOTTOM);
  }

  static ReturnOop create(int length JVM_TRAPS);

  void obj_at_put(const int index, Oop* oop) {
    obj_at_put(index, oop->obj());
  }

  void obj_at_put(const int index, OopDesc* obj) {
    *obj_addr_at(index) = (OopDesc*)(unsigned(obj)|max_counter());
  }

  OopDesc* obj_at(const int index) const {
    return *obj_addr_at(index);
  }

  void remove(const int index) {
    obj_at_put( index, unused() );
    // IMPL_NOTE: shrink array if possible
  }

  OopDesc* get(const int index) const {
    OopDesc* obj = obj_at(index);
    return obj == unused() ? (OopDesc*)NULL : obj;
  }

  void increase_counter(int refIndex) {
    OopDesc** obj = obj_addr_at(refIndex);
    if (not_null_or_unused(*obj)){
      int counter = get_counter(*obj);
      if (counter < max_counter()) {
        *obj = make(get_value(*obj), counter+1);
      }
    }
  }

  void oops_do(void do_oop(OopDesc**));
  void mark(bool is_full_collect);
  static void clear_non_marked( OopDesc** p );
};
#endif
