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
# include "incls/_ExecutionStack.cpp.incl"

HANDLE_CHECK(ExecutionStack, is_execution_stack())

ReturnOop ExecutionStack::shrink_in_place(jint new_length) {
  // Shrink compiled method object
  size_t new_size = ExecutionStackDesc::allocation_size(new_length);

  // create a backup, as shrink_object() damages object
  ExecutionStackDesc* old_stack = (ExecutionStackDesc*)obj();
  ExecutionStackDesc* old_stack_next = old_stack->_next_stack; 

  ExecutionStack::Raw result = 
    Universe::shrink_object(this, new_size, JavaStackDirection > 0);
  result().set_length(new_length);
  GUARANTEE(result.object_size() == new_size, "invalid shrunk size");
  // The _thread field must be set by the caller

  // now we need to update stacks list to let it know that this stack has been shrunk
  ExecutionStackDesc::update_list(old_stack,
                                  old_stack_next,
                                  (ExecutionStackDesc*)result().obj());
  
  return result;
}

#ifndef PRODUCT

void ExecutionStack::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);
  NamedField id("thread", true);
  visitor->do_oop(&id, thread_offset(), true);
#endif
}

void ExecutionStack::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("An execution stack [%d] for thread 0x%x", length(), thread());
#endif
}

#endif
