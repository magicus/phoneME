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
# include "incls/_ClassParserState.cpp.incl"

// Push new element onto the top of the stack
void ClassParserState::push(ClassParserState *new_top) {
  ((ClassParserStateDesc*)new_top->obj())->_next = (ClassParserStateDesc*)obj();
  this->set_obj(new_top->obj());
}

void ClassParserState::push(Symbol *class_name JVM_TRAPS) {
  UsingFastOops fast_oops;
  ClassParserState::Fast new_top = allocate(class_name JVM_CHECK);
  push(&new_top);
}

// Remove top element from the stack
void ClassParserState::pop() {
  ClassParserStateDesc * next = ((ClassParserStateDesc*)obj())->_next;
  set_obj(next);
}

ReturnOop ClassParserState::allocate(Symbol *class_name JVM_TRAPS) {
  ClassParserStateDesc* result = (ClassParserStateDesc*)
      Universe::new_mixed_oop(MixedOopDesc::Type_ClassParserState,
                              ClassParserStateDesc::allocation_size(),
                              ClassParserStateDesc::pointer_count()
                              JVM_CHECK_0);
  result->initialize(class_name->obj());
  return result;
}
