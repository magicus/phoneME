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
# include "incls/_EntryActivation.cpp.incl"

HANDLE_CHECK(EntryActivation, is_entry_activation())

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void EntryActivation::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("EntryActivation");
#endif
}

void EntryActivation::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);
  { 
    NamedField id("length", true);
    visitor->do_int(&id, length_offset(), true);
  }
  { 
    NamedField id("method", true);
    visitor->do_oop(&id, method_offset(), true);
  }
  { 
    NamedField id("next", true);
    visitor->do_oop(&id, next_offset(), true);
  }
#if ENABLE_REFLECTION
  { 
    NamedField id("return_point", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, return_point_offset(), true);
  }
#endif
  for (int index = 0; index < length(); index++) {
    IndexableField id(index, true);
    // visitor->do_int(&id, tag_offset(index), true);
    switch(tag_at(index)) { 
      case float_tag: visitor->do_float(&id, value_offset(index), true); break;
      case obj_tag:   visitor->do_oop(&id, value_offset(index), true); break;
      default:        visitor->do_int(&id, value_offset(index), true); break;
    }
  }
#endif
}

// Print this EntryActivation and all other EntryActivation's that follow it.
void EntryActivation::print_list_on(Stream* st, int indent, int index) {
#if USE_DEBUG_PRINTING
  int last_indent = st->indentation();
  st->set_indentation(indent);
  st->indent();

  st->print("[%d] ", index);
  Method m = method();
  m.print_value_on(st);
  st->cr();

  for (int i = 0; i < length(); i++) {
    st->indent();
    st->print("    ");

    switch(tag_at(i)) { 
    case float_tag:
      st->print("(float)  %f", jvm_f2d(float_at(i)));
      break;
    case double_tag:
      st->print("(double)  %d", double_at(i));
      i++;
      break;
    case long_tag:
      st->print("(long)   ");
      st->print(OsMisc_jlong_format_specifier(), long_at(i));
      i++;
      break;
    case obj_tag:
      {
        st->print("(obj)  ");
        Oop obj = obj_at(i);
        obj.print_value_on(st);
      }
      break;
    case int_tag:
      st->print("(int)    %d", int_at(i));
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
    st->cr();
  }

  EntryActivation mynext = next();
  if (mynext.not_null()) {
    mynext.print_list_on(st, indent, index+1);
  }
  st->set_indentation(last_indent);
#endif
}

void EntryActivation::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_INT,    length);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, method);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next);
#if ENABLE_REFLECTION
  OOPMAP_ENTRY_4(do_map, param, T_INT,    return_point);
#endif
#endif
}

#endif
