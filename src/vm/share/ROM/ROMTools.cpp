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

/** \file ROMTools.cpp
 *
 * This file implements various "tools" classes used during
 * romization. E.g., for sorting log file output, or for implementing
 * fast lookup tables.
 */

#include "incls/_precompiled.incl"
#include "incls/_ROMTools.cpp.incl"

#if ENABLE_ROM_GENERATOR

void ROMLookupTable::initialize(int num_buckets, int num_attributes JVM_TRAPS)
{
  ROMLookupTableDesc *obj = (ROMLookupTableDesc*)
      Universe::new_mixed_oop(MixedOopDesc::Type_ROMLookupTable,
                              ROMLookupTableDesc::allocation_size(),
                              ROMLookupTableDesc::pointer_count()
                              JVM_CHECK);
  set_obj(obj);
  ObjArray::Raw buckets = Universe::new_obj_array(num_buckets JVM_CHECK);
  set_num_buckets(num_buckets);
  set_num_attributes(num_attributes);
  set_buckets(&buckets);

  // Because virtual functions are not allowed in Oop classes, we must
  // use function pointers. See TextKlassLookupTable in SourceROMWriter.cpp
  // for an example of overridding these two built-in functions.
  set_hashcode_func((hashcode_func_type)&ROMLookupTable::_hashcode);
  set_equals_func  ((equals_func_type)  &ROMLookupTable::_equals);
}

juint ROMLookupTable::_hashcode(Oop *key) {
  if (key->is_symbol()) {
    return ((Symbol*)key)->hash();
  } else if (key->is_string()) {
    return ((String*)key)->hash();
  } else {
    return 0;
  }
}

bool ROMLookupTable::_equals(Oop* obj1, Oop* obj2) {
  return obj1->equals(obj2);
}

void ROMLookupTable::put(Oop *key JVM_TRAPS) {
  UsingFastOops fast_oops;
  int record_size = NUM_HEADER_ELEMENTS + num_attributes();
  ObjArray::Fast record = Universe::new_obj_array(record_size JVM_CHECK);

  int n = hashcode(key) % num_buckets();
  ObjArray::Fast buckets = this->buckets();
  ObjArray::Fast next_record = buckets().obj_at(n);

  record().obj_at_put(NEXT_INDEX, &next_record);
  record().obj_at_put(KEY_INDEX, key);
  buckets().obj_at_put(n, &record);
}

ReturnOop ROMLookupTable::get_record_for(Oop *key) {
  int n = hashcode(key) % num_buckets();
  ObjArray::Raw buckets = this->buckets();
  ObjArray::Raw record = buckets().obj_at(n);

  while (record.not_null()) {
    Oop obj = record().obj_at(KEY_INDEX);
    if (equals(&obj, key)) {
      return record.obj();
    }
    record = record().obj_at(NEXT_INDEX);
  }

  return NULL;
}

bool ROMLookupTable::exists(Oop *key) {
  return get_record_for(key) != NULL;
}

jint ROMLookupTable::get_int_attribute(Oop *key, jint index) {
  ObjArray::Raw record = get_record_for(key);
  GUARANTEE(record.not_null(), "key must already exist");

  TypeArray::Raw int_array = record().obj_at(NUM_HEADER_ELEMENTS + index);
  GUARANTEE(int_array.not_null(), "attribute must already exist");

  return int_array().int_at(0);
}

void ROMLookupTable::set_int_attribute(Oop *key, jint index, jint value
                                       JVM_TRAPS)
{
  UsingFastOops fast_oops;

  ObjArray::Fast record = get_record_for(key);
  GUARANTEE(record.not_null(), "key must already exist");

  TypeArray::Fast int_array = record().obj_at(NUM_HEADER_ELEMENTS + index);
  if (int_array.is_null()) {
    int_array = Universe::new_int_array(1 JVM_CHECK);
    record().obj_at_put(NUM_HEADER_ELEMENTS + index, &int_array);
  }
  int_array().int_at_put(0, value);
}

ReturnOop ROMLookupTable::get_obj_attribute(Oop *key, jint index) {
  ObjArray::Raw record = get_record_for(key);
  GUARANTEE(record.not_null(), "key must already exist");

  return record().obj_at(NUM_HEADER_ELEMENTS + index);
}

void ROMLookupTable::set_obj_attribute(Oop *key, jint index, Oop *value)
{
  ObjArray::Raw record = get_record_for(key);
  GUARANTEE(record.not_null(), "key must already exist");

  record().obj_at_put(NUM_HEADER_ELEMENTS + index, value);
}

ReturnOop ROMLookupTable::get_record_at(int index) {
  ObjArray::Raw buckets = this->buckets();
  return buckets().obj_at(index);
}

ReturnOop ROMLookupTable::get_key_from_record(Oop *r) {
  ObjArray::Raw record = r->obj();
  return record().obj_at(KEY_INDEX);
}

ReturnOop ROMLookupTable::get_next_record(Oop *r) {
  ObjArray::Raw record = r->obj();
  return record().obj_at(NEXT_INDEX);
}

ROMVector * ROMVector::_current_sorting_vector = NULL;

void ROMVector::initialize(JVM_SINGLE_ARG_TRAPS) {
  initialize(20 JVM_NO_CHECK_AT_BOTTOM);
}

void ROMVector::initialize(jint capacity JVM_TRAPS) {
  ROMVectorDesc *obj = (ROMVectorDesc*)
      Universe::new_mixed_oop(MixedOopDesc::Type_ROMVector,
                              ROMVectorDesc::allocation_size(),
                              ROMVectorDesc::pointer_count()
                              JVM_CHECK);
  set_obj(obj);
  ObjArray::Raw array = Universe::new_obj_array(capacity JVM_CHECK);
  set_array(&array);
  set_size(0);
  set_sort_strings_by_descending_size(false);

  // Because virtual functions are not allowed in Oop classes, we must
  // use function pointers. See OffsetVector in SourceROMWriter.cpp
  // for an example of overridding this built-in function.
  set_compare_to_func((compare_to_func_type)&ROMVector::_compare_to);
}

void ROMVector::add_element(Oop *oop JVM_TRAPS) {
  ObjArray array = this->array();
  if (size() == array.length()) {
    ObjArray new_array = Universe::new_obj_array(size() * 2 JVM_CHECK);
    ObjArray::array_copy(&array, 0, &new_array, 0, size() JVM_CHECK);
    array = new_array.obj();
    set_array(&array);
  }
  array.obj_at_put(size(), oop);
  set_size(size() + 1);
}

void ROMVector::add_no_expand(Oop *oop) {
  ObjArray::Raw array = this->array();
  GUARANTEE(size() <= array().length(), "sanity");

  array().obj_at_put(size(), oop);
  set_size(size() + 1);
}

void ROMVector::sort() {
  GUARANTEE(_current_sorting_vector == NULL, "no recursive sorting!");
  _current_sorting_vector = this;
  ObjArray array = this->array();
  void *base = array.base_address();
  int length = size();
  if (length > 0) {
    jvm_qsort(base, length, sizeof(OopDesc*), ROMVector::compare_to_forwarder);
    // some of array items might be in young gen while the others are in old gen
    oop_write_barrier_range((OopDesc**)base, length);
  }
  _current_sorting_vector = NULL;
}

jint ROMVector::_compare_to(Oop *obj1, Oop* obj2) {
  if (obj1->is_instance_class() && obj2->is_instance_class()) {
    InstanceClass::Raw klass1 = obj1->obj();
    InstanceClass::Raw klass2 = obj2->obj();

    return compare_to(&klass1, &klass2);
  }

  if (obj1->is_method() && obj2->is_method()) {
    Method::Raw m1 = obj1->obj();
    Method::Raw m2 = obj2->obj();

    return compare_to(&m1, &m2);
  }

  if (obj1->is_string() && obj2->is_string()) {
    String::Raw s1 = obj1->obj();
    String::Raw s2 = obj2->obj();

    return compare_to(&s1, &s2);
  }

  if (obj1->is_symbol() && obj2->is_symbol()) {
    Symbol::Raw s1 = obj1->obj();
    Symbol::Raw s2 = obj2->obj();

    return compare_to(&s1, &s2);
  }

  if (obj1->is_type_array() && obj2->is_type_array()) {
    TypeArray::Raw s1 = obj1->obj();
    TypeArray::Raw s2 = obj2->obj();

    return compare_to(&s1, &s2);
  }

#if ENABLE_COMPILER
  if (obj1->is_compiled_method() && obj2->is_compiled_method()) {
    CompiledMethod::Raw cm1 = obj1->obj();
    CompiledMethod::Raw cm2 = obj2->obj();
    Method::Raw m1 = cm1().method();
    Method::Raw m2 = cm2().method();
    return compare_to(&m1, &m2);
  }
#endif

  SHOULD_NOT_REACH_HERE();
  // IMPL_NOTE: compiler warning if no return value
  return 0;
}

jint ROMVector::compare_to(Method *m1, Method *m2) {
  jint result;
  bool dummy;

  InstanceClass::Raw holder1 = m1->holder();
  InstanceClass::Raw holder2 = m2->holder();
  result = compare_to(&holder1, &holder2);
  if (result != 0) {
    return result;
  }

  Symbol::Raw name1 = m1->get_original_name(dummy);
  Symbol::Raw name2 = m2->get_original_name(dummy);
  result = compare_to(&name1, &name2);
  if (result != 0) {
    return result;
  }

  Symbol::Raw sig1 = m1->signature();
  Symbol::Raw sig2 = m2->signature();
  return  compare_to(&sig1, &sig2);
}

jint ROMVector::compare_to(InstanceClass *c1, InstanceClass *c2) {
  Symbol::Raw name1 = c1->original_name();
  Symbol::Raw name2 = c2->original_name();

  return compare_to(&name1, &name2);
}

jint ROMVector::compare_to(Symbol *s1, Symbol *s2) {
  return Symbol::compare_to(s1, s2);
}

jint ROMVector::compare_to(String *s1, String *s2) {
  int len1 = s1->count();
  int len2 = s2->count();

  if (s1->equals(s2)) {
    return 0;
  }

  if (_current_sorting_vector->sort_strings_by_descending_size()) {
    if (len1 > len2) {
      // String 1 is longer, it should appear first
      return -1;
    } if (len1 < len2) {
      return 1;
    } else {
      for (int i=0; i<len1; i++) {
        jchar c1 = s1->char_at(i);
        jchar c2 = s2->char_at(i);
        if (c1 < c2) {
          return -1;
        } else if (c1 == c2) {
          continue;
        } else {
          return 1;
        }
      }
      int o1 = (int)(s1->obj());
      int o2 = (int)(s2->obj());

      if (o1 > o2) {
        return -1;
      } else {
        return 1;
      }
    }
  }

  // Sort Strings in alphabetical order
  int len = (len1 < len2) ? len1 : len2;
  for (int i=0; i<len; i++) {
    jchar c1 = s1->char_at(i);
    jchar c2 = s2->char_at(i);
    if (c1 < c2) {
      return -1;
    } else if (c1 == c2) {
      continue;
    } else {
      return 1;
    }
  }

  if (len1 > len2) {
    return 1;
  } if (len1 < len2) {
    return -1;
  } else {
    return 0;
  }
}

jint ROMVector::compare_to(TypeArray *s1, TypeArray *s2) {
  int len1 = s1->length();
  int len2 = s2->length();

  if (s1->equals(s2)) {
    return 0;
  }

  // Sort Strings in alphabetical order
  int len = (len1 < len2) ? len1 : len2;
  for (int i=0; i<len; i++) {
    jubyte c1 = s1->ubyte_at(i);
    jubyte c2 = s2->ubyte_at(i);
    if (c1 < c2) {
      return -1;
    } else if (c1 == c2) {
      continue;
    } else {
      return 1;
    }
  }

  if (len1 > len2) {
    return 1;
  } if (len1 < len2) {
    return -1;
  } else {
    return 0;
  }
}

jint ROMVector::compare_to_forwarder(const void* p1, const void *p2) {
  Oop::Raw obj1 = *(OopDesc**)p1;
  Oop::Raw obj2 = *(OopDesc**)p2;

  return _current_sorting_vector->compare_to(&obj1, &obj2);
}

// This is a fix that carves off the end of an object. Since
// the GC wants all the heap objects to be contiguous, we must fill
// the hole with a valid heap object of the desired size.
void ROMTools::shrink_object(Oop *obj, size_t old_size, size_t reduction) {  
  GUARANTEE((reduction & 0x03) == 0, "must be aligned");

  if (reduction < sizeof(OopDesc)) {
    return;
  }

  OopDesc *stuffing = (OopDesc*)(((int)obj->obj()) + old_size - reduction);
  Oop *oop = (Oop*)&stuffing;

  if (reduction == sizeof(OopDesc)) {
    // OK, I want to make <stuffing> into an object whose size is
    // is exactly sizeof(OopDesc) bytes. The easiest way to do this is to make
    // it into a near object of Symbol.
    Oop symbol_near = Universe::symbol_class()->prototypical_near();
    Oop klass = symbol_near.klass();
    oop->set_klass(&klass);
    GUARANTEE(oop->object_size() == sizeof(OopDesc), "sanity");
  } else {
    // Clear everything inside, in case some of the filler words have
    // write-barrier bits (IMPL_NOTE: this might be unnecessary).
    jvm_memset(stuffing, 0, reduction);

    // Clear thr write barrier bit for the word that would be occupied by the
    // _length field of the byte array
    OopDesc **length_word = DERIVED(OopDesc**, stuffing, sizeof(OopDesc));
    ObjectHeap::clear_bit_for(length_word);

    // Make <stuffing> into a bytearray of the desired length. This assumes
    // sizeof(ArrayDesc) is 8 bytes.
    Oop byte_array_near = Universe::byte_array_class()->prototypical_near();
    oop->set_klass(&byte_array_near);

    int array_length = (int)reduction - (int)sizeof(ArrayDesc);
    GUARANTEE(array_length >= 0, "sanity");
    ((Array*)oop)->set_length(array_length);
  }

  GUARANTEE(oop->object_size() == reduction, "sanity");
}

int ROMVector::index_of(Oop *value) {  
  const int vect_size = size();
  for (int i = 0; i < vect_size; i++) {
    Oop::Raw element = element_at(i);
    if (element.equals(value)) {
      return i;
    }
  }
  return -1;
}

#endif /* ENABLE_ROM_GENERATOR */
