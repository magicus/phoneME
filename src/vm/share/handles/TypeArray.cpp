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
# include "incls/_TypeArray.cpp.incl"

HANDLE_CHECK(TypeArray, is_type_array())

void TypeArray::array_copy(TypeArray* src, jint src_pos,
                           TypeArray* dst, jint dst_pos, jint length) {
  TypeArrayClass::Raw src_class = src->blueprint();
  jint scale = src_class().scale();

#ifdef AZZERT
  TypeArrayClass::Raw dst_class = dst->blueprint();
  GUARANTEE(scale == dst_class().scale(), "sanity check");
#endif

  address src_start =(address)src->field_base(base_offset() + src_pos * scale);
  address dst_start =(address)dst->field_base(base_offset() + dst_pos * scale);
  jvm_memmove(dst_start, src_start, length * src_class().scale());
}

#ifndef PRODUCT

void TypeArray::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  TypeArrayClass c = blueprint();
  c.print_type_on(st);
  st->print(" Array [%d]", length());
#endif
}

void TypeArray::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  NamedField field("length", true);
  visitor->do_uint(&field, length_offset(), true);

  TypeArrayClass c = blueprint();
  switch(c.type()) {
   case T_BOOLEAN: {
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_bool(&field, offset_from_bool_index(index), true);
     }
     break;
   }
   case T_CHAR: {
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_char(&field, offset_from_char_index(index), true);
     }
     break;
   }
   case T_FLOAT: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_float(&field, offset_from_float_index(index), true);
     }
     break;
   }
   case T_DOUBLE: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_double(&field, offset_from_double_index(index), true);
     }
     break;
   }
   case T_BYTE: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_byte(&field, offset_from_byte_index(index), true);
     }
     break;
   }
   case T_SHORT: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_short(&field, offset_from_short_index(index), true);
     }
     break;
   }
   case T_INT: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_int(&field, offset_from_int_index(index), true);
     }
     break;
   }
   case T_LONG: { 
     for (int index = 0; index < length(); index++) {
       IndexableField field(index, true);
       visitor->do_long(&field, offset_from_long_index(index), true);
     }
     break;
   }
   default: SHOULD_NOT_REACH_HERE();
 }
#endif
}

#endif
