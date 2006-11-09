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

/** \class Condition
    Used to handle wait and sleep in Scheduler and Synchronizer.

*/

class Condition : public MixedOop {
public:
  HANDLE_DEFINITION_CHECK(Condition, MixedOop);

private:  
  static jint wait_object_offset() {
    return FIELD_OFFSET(ConditionDesc, _wait_object);
  }
  static jint waiters_head_offset() {
    return FIELD_OFFSET(ConditionDesc, _waiters_head);
  }
  static jint waiters_tail_offset() {
    return FIELD_OFFSET(ConditionDesc, _waiters_tail);
  }
  static jint next_offset() {
    return FIELD_OFFSET(ConditionDesc, _next);
  }

public:
  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);

  ReturnOop wait_object() const {
    return obj_field(wait_object_offset());
  }
  void set_wait_object(Oop* value) {
    obj_field_put(wait_object_offset(), value);
  }
  void clear_wait_object() {
    obj_field_clear(wait_object_offset());
  }

  ReturnOop waiters_head() const {
    return obj_field(waiters_head_offset());
  }
  void set_waiters_head(Oop* value) {
    obj_field_put(waiters_head_offset(), value);
  }
  void clear_waiters_head() {
    obj_field_clear(waiters_head_offset());
  }

  ReturnOop waiters_tail() const {
    return obj_field(waiters_tail_offset());
  }
  void set_waiters_tail(Oop* value) {
    obj_field_put(waiters_tail_offset(), value);
  }
  void clear_waiters_tail() {
    obj_field_clear(waiters_tail_offset());
  }

  ReturnOop next() const {
    return obj_field(next_offset());
  }
  void set_next(Oop* value) {
    obj_field_put(next_offset(), value);
  }
  void clear_next() {
    obj_field_clear(next_offset());
  }

  bool has_waiters() {
    return waiters_head() != NULL;
  }
  void add_waiter(Thread* waiter);
  void remove_waiter(Thread* waiter);
  ReturnOop remove_first_waiter();

#ifndef PRODUCT
  void print();
  static void iterate_oopmaps(oopmaps_doer do_map, void *param);
  void iterate(OopVisitor* visitor);
#endif

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

};
