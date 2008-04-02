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
# include "incls/_ObjArray.cpp.incl"

HANDLE_CHECK(ObjArray, is_obj_array())

void ObjArray::fill_zero(OopDesc *obj_array) {
  ObjArray::Raw array = obj_array;
  jvm_memset(array().base_address(), 0, array().length() * sizeof(OopDesc*));
}

void ObjArray::obj_at_put(int index, OopDesc* value) {
  obj_field_put(offset_from_index(index), value);
}

void oop_write_barrier_range(OopDesc** start, int len) {
  juint start_offset;
  juint head, main, tail;
  OopDesc **p;
  const juint BLOCK = 32;

  if (len < BLOCK) {
    GUARANTEE(len > 0, "ObjectHeap::set_bit_range() cannot handle len <= 0");
    // sets one bit at a time.
    ObjectHeap::set_bit_range(start, len);
  } else {
    // split the range into <head>, <main> and <tail> portions, so that the
    // <main> portion can be set using memset.
    start_offset = start - _heap_start;

    head = BLOCK - (start_offset % BLOCK);
    main = (len - head) & (~(BLOCK-1));
    tail = len - (head + main);

    GUARANTEE(head <= BLOCK, "sanity");
    GUARANTEE(tail <  BLOCK, "sanity");

#ifdef AZZERT
    int i;
    p = start;
    for (i = 0; i < len; i++) {
      ObjectHeap::clear_bit_for(p);
      p++;
    }
#endif

    p = start;

    if (head > 0) {
      ObjectHeap::set_bit_range(p, head);
      p += head;
    }

    if (main > 0) {
      juint *bvw = ObjectHeap::get_bitvectorword_for_aligned(p);
      jvm_memset(bvw, 0xff, main / BitsPerByte);
      p += main;
    }

    if (tail > 0) {
      ObjectHeap::set_bit_range(p, tail);
    }

#ifdef AZZERT
    p = start;
    for (i = 0; i < len; i++) {
      GUARANTEE(ObjectHeap::test_bit_for(p), "bit must be set");
      p++;
    }
#endif
  }
}

void ObjArray::array_copy(ObjArray* src, jint src_pos, 
                          ObjArray* dst, jint dst_pos, jint length JVM_TRAPS) {
  // Special case. Boundary cases must be checked first
  // This allows the following call: copy_array(s, s.length(),
  // d.length(), 0).  This is correct, since the position is supposed
  // to be an 'in between point', i.e., s.length(), points to the
  // right of the last element.
  if (length == 0) {
    return;
  }

  OopDesc** src_start =
      (OopDesc**) src->field_base(base_offset() + src_pos * oopSize);
  OopDesc** dst_start =
      (OopDesc**) dst->field_base(base_offset() + dst_pos * oopSize);
  if (src->equals(dst)) {
    // since source and destination are equal we do not need conversion checks.
    jvm_memmove(dst_start, src_start, length * oopSize);
    oop_write_barrier_range(dst_start, length);
  } else {
    // We have to make sure all elements conform to the destination array
    ObjArrayClass::Raw dst_class = dst->blueprint();
    ObjArrayClass::Raw src_class = src->blueprint();
    JavaClass::Raw bound = dst_class().element_class();
    JavaClass::Raw stype = src_class().element_class();
    if (stype.equals(&bound) || stype().is_subtype_of(&bound)) {
      // elements are guaranteed to be subtypes, so no check necessary
      jvm_memmove(dst_start, src_start, length * oopSize);
      oop_write_barrier_range(dst_start, length);
      return;
    }
    
    // Slow case: need individual subtype checks
    // Do store checks first so they're guaranteed to be done even if
    // an exception is thrown obj_at_put contains a write barrier already
    Oop::Raw       element;
    JavaClass::Raw element_class;
    for (int index =0; index < length; index++) {
      element = src->obj_at( src_pos + index);
      if (element.is_null()) {
        dst->obj_at_put(dst_pos + index, &element);
      } else {
        element_class = element.blueprint();
        if (element_class().is_subtype_of(&bound)) {
          dst->obj_at_put(dst_pos + index, &element);
        } else {
          Throw::array_store_exception(subtype_check_failed JVM_THROW);
        }
      }
    }
  }
}

#ifndef PRODUCT

void ObjArray::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  NamedField field("length", true);
  visitor->do_uint(&field, length_offset(), true);

  for (int index = 0; index < length(); index++) {
    IndexableField field(index, true);
    visitor->do_oop(&field, offset_from_index(index), true);
  }
#endif
}

void ObjArray::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("Object Array [%d]", length());
#endif
}

#endif
