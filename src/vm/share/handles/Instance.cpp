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
# include "incls/_Instance.cpp.incl"

#ifndef PRODUCT

static void iterate_fields(InstanceClass* ic, OopVisitor* visitor, 
                           bool visit_value) {
  InstanceClass s = ic->super();
  // Print fields defined by super class
  if (!s.is_null()) {
    iterate_fields(&s, visitor, visit_value);
  }
  // Print fields define by local class
  TypeArray fields = ic->original_fields(); 
  int last_field_offset = -1;
  for (;;) {
    int next_field_index = -1;
    int min_offset = 0x7fffffff;

    for(int index = 0; index < fields.length(); index += 5) {
      OriginalField f(ic, index);
      if (!f.is_static() &&
          f.offset() > last_field_offset && 
          f.offset() < min_offset) {
        next_field_index = index;
        min_offset = f.offset();
      }
    }

    last_field_offset = min_offset;
    if (next_field_index < 0) {
      break;
    }

    OriginalField f(ic, next_field_index);
    Symbol name = f.name();
    SymbolField field(&name, false);
    switch(f.type()) {
    case T_BOOLEAN:
      visitor->do_bool(&field, f.offset(), visit_value);
      break;
    case T_CHAR:
      visitor->do_char(&field, f.offset(), visit_value);
      break;
    case T_FLOAT :
      visitor->do_float(&field, f.offset(), visit_value);
      break;
    case T_DOUBLE:
      visitor->do_double(&field, f.offset(), visit_value);
      break;
    case T_BYTE:
      visitor->do_byte(&field, f.offset(), visit_value);
      break;
    case T_SHORT:
      visitor->do_short(&field, f.offset(), visit_value);
      break;
    case T_INT:
      visitor->do_int(&field, f.offset(), visit_value);
      break;
    case T_LONG:
      visitor->do_long(&field, f.offset(), visit_value);
      break;
    case T_OBJECT:
      visitor->do_oop(&field, f.offset(), visit_value);
      break;
    case T_ARRAY:
      visitor->do_oop(&field, f.offset(), visit_value);
      break;
    }
  }
}

void Instance::iterate(OopVisitor* visitor) {
  Oop::iterate(visitor);
  visitor->do_comment("fields");
  InstanceClass ic = blueprint();
  iterate_fields(&ic, visitor, true);
}

void Instance::print_value_on(Stream* st) {
  st->print("an instance of class ");
  InstanceClass ic = blueprint();
  ic.print_name_on(st);
}

#endif
