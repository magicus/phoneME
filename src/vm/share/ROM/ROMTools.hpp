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

/** \file ROMTools.hpp
 * This header file declares miscellaneous "tools" classes used during
 * romization. E.g., for sorting log file output, or for implementing fast
 * lookup tables.
 */

#if ENABLE_ROM_GENERATOR 

class ROMLookupTable;

class ROMLookupTableDesc : public MixedOopDesc {
protected:
  typedef juint (ROMLookupTable::*hashcode_func_type)(Oop *key);
  typedef bool  (ROMLookupTable::*equals_func_type)(Oop* obj1, Oop* obj2);

private:
  OopDesc *          _buckets;
  int                _num_buckets;
  int                _num_attributes;
  hashcode_func_type _hashcode_func;
  equals_func_type   _equals_func;

  static size_t allocation_size() { 
    return align_allocation_size(sizeof(ROMLookupTableDesc));
  }
  static int pointer_count() {
    return 1;
  }

public:
  friend class ROMLookupTable;
};

/** 
 * An abstract base class for fast lookup of objects that are written into
 * the ROM image. This is essentially a hash table. The hash key must be
 * an Oop. Optionally, along each key, you can store a variable number of
 * attributes.
 */
class ROMLookupTable : public MixedOop {
  enum {
    NEXT_INDEX = 0,
    KEY_INDEX = 1,
    NUM_HEADER_ELEMENTS = 2
  };

  /**
   * Returns the record (a ObjArray) for the given key, or NULL if the key
   * doesn't exist in this ROMLookupTable.
   */
  ReturnOop get_record_for(Oop *key);

public:
  typedef ROMLookupTableDesc::hashcode_func_type hashcode_func_type;
  typedef ROMLookupTableDesc::equals_func_type   equals_func_type;

  HANDLE_DEFINITION(ROMLookupTable, MixedOop);

  DEFINE_ACCESSOR_OBJ(ROMLookupTable, ObjArray, buckets)
  DEFINE_ACCESSOR_NUM(ROMLookupTable, int,      num_buckets)
  DEFINE_ACCESSOR_NUM(ROMLookupTable, int,      num_attributes)

public:
  hashcode_func_type hashcode_func() {
    return ((ROMLookupTableDesc*)obj())->_hashcode_func;

  }
  void set_hashcode_func(hashcode_func_type value) {
    ((ROMLookupTableDesc*)obj())->_hashcode_func = value;
  }
  equals_func_type equals_func() {
    return ((ROMLookupTableDesc*)obj())->_equals_func;

  }
  void set_equals_func(equals_func_type value) {
    ((ROMLookupTableDesc*)obj())->_equals_func = value;
  }

public:
  /**
   * Initialize a ROMLookupTable with the given number of buckets. Each
   * key (to be added later using put()) in the ROMLookupTable may
   * hold up to the given number of attributes.
   */
  void initialize(int num_buckets, int num_attributes JVM_TRAPS);

  /**
   * Adds the given Oop into this ROMLookupTable. Note that this function
   * does not search for duplication, so you must call exist() first to
   * find out if the key already exists in this ROMLookupTable.
   */
  void put(Oop *key JVM_TRAPS);

  /**
   * Adds the given key into this ROMLookupTable, and sets its object
   * attribute #0 to the given value.  Note that this function does
   * not search for duplication, so you must call exist() first to
   * find out if the key already exists in this ROMLookupTable.
   */
  void put(Oop *key, Oop *value JVM_TRAPS) {
    put(key JVM_CHECK);
    set_obj_attribute(key, 0, value);
  }

  /**
   * This is a convenience function for getting the object attribute #0 for the
   * given key.
   */
  ReturnOop get(Oop *key) {
    return get_obj_attribute(key, 0);
  }

  /**
   * Does the given key already exist in this table?
   */
  bool exists(Oop *key);

  /**
   * Returns an integer attribute of the given key. This assumes that you
   * have previous stored an integer attribute for this key at the given
   * index.
   */
  jint get_int_attribute(Oop *key, jint index);

  /**
   * Sets an integer attribute for the key at the given index. You must
   * have 0 <= index < this.num_attributes().
   */
  void set_int_attribute(Oop *key, jint index, jint value JVM_TRAPS);

  /**
   * Returns an object attribute of the given key. This assumes that you
   * have previous stored an object attribute for this key at the given
   * index.
   */
  ReturnOop get_obj_attribute(Oop *key, jint index);

  /**
   * Sets an object attribute for the key at the given index. You must
   * have 0 <= index < this.num_attributes().
   */
  void set_obj_attribute(Oop *key, jint index, Oop *value);

  /**
   * Computes the hashcode for the given key. We need to use
   * function pointers because we can't have virtual functions inside
   * Oops.
   */
  juint hashcode(Oop *key) {
    hashcode_func_type func = hashcode_func();
    return (this->*func)(key);
  }

  juint _hashcode(Oop *key);

  /**
   * Checks if two objects are equal. By default calls Oop.equals(), but
   * may be overridden to do other types of equality checks.
   */
  bool equals(Oop* obj1, Oop* obj2) {
    equals_func_type func = equals_func();
    return (this->*func)(obj1, obj2);
  }

  bool _equals(Oop* obj1, Oop* obj2);

  /**
   * Returns the first record at the given index. index must be less than
   * this->_num_buckets. 
   */
  ReturnOop get_record_at(int index);

  /**
   * Returns the key stored in this record. record must be a value returned
   * by get_record_at().
   */
  ReturnOop get_key_from_record(Oop *record);

  /**
   * Returns record following this record. record must be a value returned
   * by get_record_at().
   */
  ReturnOop get_next_record(Oop *record);
};

class ROMVector;
class ROMVectorDesc : public MixedOopDesc {
protected:
  typedef jint (ROMVector::*compare_to_func_type)(Oop *obj1, Oop* obj2);

private:
  /**
   * Stores the elements of this ROMVector. It's expanded as necessary.
   */
  ObjArrayDesc* _array;

  /**
   * The value returned by size().
   */
  jint _size;

  bool _sort_strings_by_descending_size;

  compare_to_func_type _compare_to_func;

  static size_t allocation_size() { 
    return align_allocation_size(sizeof(ROMVectorDesc));
  }

  static int pointer_count() {
    return 1;
  }

public:
  friend class ROMVector;
  friend class Universe;
};

/** \class ROMVector
 *
 * An class for storing a variable number of Oops. It's mainly used by
 * the ROMizer to sort the output in ROMLog.txt
 */
class ROMVector : public MixedOop {
public:
  typedef ROMVectorDesc::compare_to_func_type compare_to_func_type;

  HANDLE_DEFINITION(ROMVector, MixedOop);

  DEFINE_ACCESSOR_OBJ(ROMVector, ObjArray, array)
  DEFINE_ACCESSOR_NUM(ROMVector, int,      size)
  DEFINE_ACCESSOR_NUM(ROMVector, bool,     sort_strings_by_descending_size)

public:
  compare_to_func_type compare_to_func() {
    return ((ROMVectorDesc*)obj())->_compare_to_func;

  }
  void set_compare_to_func(compare_to_func_type value) {
    ((ROMVectorDesc*)obj())->_compare_to_func = value;
  }

  /**
   * Initialize the ROMVector with the default initial capacity.
   */
  void initialize(JVM_SINGLE_ARG_TRAPS);

  /**
   * Initialize the ROMVector with the given initial capacity.
   */
  void initialize(int capacity JVM_TRAPS);

  /**
   * Adds a new element into the ROMVector.
   */
  void add_element(Oop *oop JVM_TRAPS);

  /**
   * Adds a new element into the ROMVector without expanding the internal
   * array. This must be called when the ROMVector is not completely full.
   */
  void add_no_expand(Oop *oop);

  /**
   * Forget about all objects that have been stored in the ROMVector
   */
  void flush() {
    set_size(0);
  }

  /**
   * Returns the element at the given index. The first added element has
   * index=0, etc.
   */
  ReturnOop element_at(jint index) {
    GUARANTEE(index < size(), "sanity");
    ObjArray::Raw array = this->array();
    return array().obj_at(index);
  }

  /**
   * Sets the element at the given index.
   */
  void element_at_put(jint index, Oop *obj) {
    GUARANTEE(index < size(), "sanity");
    ObjArray::Raw array = this->array();
    array().obj_at_put(index, obj);
  }

  /**
   * Sorts the elements. If this.compare_to(obj1, obj2) returns -1, obj1
   * appears in before obj2 after sorting.
   */
  void sort();

  /**
   * Implements simple comparison for sorting the elements of this
   * ROMVector. It knows about Method and InstanceClass. If you want
   * to sort other types of Oops, override this method.
   */
  jint compare_to(Oop *obj1, Oop* obj2) {
    compare_to_func_type func = compare_to_func();
    return (this->*func)(obj1, obj2);
  }

  jint _compare_to(Oop *obj1, Oop* obj2);

  /**
   * Returns the internal array for quick access.
   */
  ReturnOop raw_array() {
    return array();
  }

  /**
   * Returns true if this Vector contains the specified element.
   */
  bool contains(Oop *value) {
    return index_of(value) != -1;
  }

  /**
   * Returns the index of an object in ROMVector.
   */
  int index_of(Oop *value);
protected:
  /**
   * Compares the order of two Method objects.
   */
  static jint compare_to(Method *m1, Method *m2);

  /**
   * Compares the order of two InstanceClass objects.
   */
  static jint compare_to(InstanceClass *c1, InstanceClass *c2);

  /**
   * Compares the order of two Symbol objects.
   */
  static jint compare_to(Symbol *s1, Symbol *s2);

  /**
   * Compares the order of two String objects.
   */
  static jint compare_to(String *s1, String *s2);

  /**
   * Compares the order of two UTF8 strings stored inside TypeArray
   */
  static jint compare_to(TypeArray *s1, TypeArray *s2);

private:

  /**
   * The vector that's currently being sorted
   */
  static ROMVector * _current_sorting_vector;

  /**
   * This method forwards the jvm_qsort callback to the current instance
   * of ROMVector that's being sorted. Because of the limitation of the 
   * jvm_qsort API, only one instance of ROMVector may be sorted at any time.
   */
  static jint compare_to_forwarder(const void* p1, const void *p2);
};

/** 
 * This class is a collection of useful static functions used by
 * various parts of the romizer.
 */
class ROMTools : public AllStatic {
public:
  /**
   * Shrinks the given object by the number of bytes specified in reduction.
   * This is done by placing a dummy (garbage) object at the end of the
   * given object. This function is necessary because ObjectHeap requires
   * that all objects must be consecutively laid out without holes.
   *
   * It's the caller's responsibility to manipulate obj so that the GC will
   * recognize its new size.
   */
  static void shrink_object(Oop *obj, size_t old_size, size_t reduction);
};

#endif /* ENABLE_ROM_GENERATOR */
