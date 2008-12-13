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
# include "incls/_TypeArrayClass.cpp.incl"

HANDLE_CHECK(TypeArrayClass, is_type_array_class())

bool TypeArrayClass::compute_is_subtype_of(JavaClass* other_class) {
  return equals(other_class)
      || ArrayClass::compute_is_subtype_of(other_class);
}

ReturnOop TypeArrayClass::get_array_class(jint distance JVM_TRAPS) {
  if (distance == 1) {
    return obj();
  }
#if ENABLE_ISOLATES
  TaskMirror::Raw tm;
  if (Universe::before_main()) {
    tm = Universe::new_task_mirror(0, 0 JVM_CHECK_0);
    tm().set_containing_class(this);
    TaskMirror::clinit_list_add(&tm);
  }
#endif
  UsingFastOops fast_oops;
  ObjArrayClass::Fast ac = compute_higher_dimension(JVM_SINGLE_ARG_CHECK_0);
  return ac().get_array_class(distance - 1 JVM_NO_CHECK_AT_BOTTOM_0);
}

#ifndef PRODUCT

void TypeArrayClass::print_type_on(Stream* st) {
#if USE_DEBUG_PRINTING
 switch(type()) {
   case T_BOOLEAN:   st->print("Bool");   break;
   case T_CHAR:      st->print("Char");   break;
   case T_FLOAT:     st->print("Float");  break;
   case T_DOUBLE:    st->print("Double"); break;
   case T_BYTE:      st->print("Byte");   break;
   case T_SHORT:     st->print("Short");  break;
   case T_INT:       st->print("Int");    break;
   case T_LONG:      st->print("Long");   break;
   default: SHOULD_NOT_REACH_HERE();
 }
#endif
}

void TypeArrayClass::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
 st->print("The ");
 print_type_on(st);
 st->print(" Array Class");
#endif
}

void TypeArrayClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  ArrayClass::iterate(visitor);
#endif
}

#endif
