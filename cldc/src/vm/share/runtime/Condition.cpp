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

#include "incls/_precompiled.incl"
#include "incls/_Condition.cpp.incl"

HANDLE_CHECK(Condition, is_condition())

ReturnOop Condition::allocate(JVM_SINGLE_ARG_TRAPS) {
  return Universe::allocate_condition(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void Condition::add_waiter(Thread* waiter) {
  waiter->clear_next();
  Thread::Raw tmp = waiters_tail();
  if (tmp.not_null()) {
    tmp().set_next(waiter);
  } else {
    set_waiters_head(waiter);
  }
  set_waiters_tail(waiter);
}

void Condition::remove_waiter(Thread* waiter) {
  Thread::Raw prev;
  Thread::Raw current = waiters_head();
  Thread::Raw next;
  while (current.not_null() && !current.equals(waiter)) {
    prev    = current;
    current = current().next();
  }
  if (current.not_null()) {
    next = current().next();
    if (prev.not_null()) {
      prev().set_next(&next);
    } else {
      set_waiters_head(&next);
    }
    if (next.is_null()) {
      set_waiters_tail(&prev);
    }
    waiter->clear_next();
  }
}

ReturnOop Condition::remove_first_waiter() {
  Thread::Raw head = waiters_head();
  if (head.is_null()) {
    return head;
  }
  Thread::Raw new_head = head().next();
  if (new_head.is_null()) {
    clear_waiters_tail(); // no more waiters
    clear_waiters_head();
  } else { 
    set_waiters_head(&new_head);
  }
  return head;
}

#ifndef PRODUCT

void Condition::print() {
#if USE_DEBUG_PRINTING
  Condition tmp = Universe::scheduler_waiting();
  bool is_waiter = false;
  for ( ; !tmp.is_null(); tmp = tmp.next()) {
    if (tmp.equals(this)) {
      is_waiter = true;
      break;
    }
  }
  Oop obj = wait_object();
  if (obj.is_null()) {
    GUARANTEE(is_waiter, "Sanity");
    tty->print("Sleeper 0x%x\n", this->obj());
  } else {
    tty->print("%s 0x%x for ",  
                (is_waiter ? "Waiter" : "Synchronizer"), this->obj());
    Verbose++;
    obj.print_value_on(tty);
    Verbose--;
    tty->cr();
  }
  Thread current = waiters_head();
  int i = 0;
  while (current.not_null()) {
    tty->print("  %d: ", i++);
    current.print_value_on(tty);
    tty->print(" 0x%x\n", current.wait_stack_lock());
    current = current.next();
  }
#endif
}

void Condition::iterate_oopmaps(oopmaps_doer do_map, void *param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, wait_object);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, waiters_head);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, waiters_tail);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next);
#endif
}

void Condition::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  iterate_oopmaps(BasicOop::iterate_one_oopmap_entry, (void*)visitor);
#endif
}

#endif

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int Condition::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  //klass
  map_index = Near::generate_fieldmap(field_map);

  // wait_object
  field_map->byte_at_put(map_index++, T_OBJECT);
  // waiters_head
  field_map->byte_at_put(map_index++, T_OBJECT);
  // waiters_tail
  field_map->byte_at_put(map_index++, T_OBJECT);
  // next
  field_map->byte_at_put(map_index++, T_OBJECT);

  return map_index;
}
#endif
