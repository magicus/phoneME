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

// IMPL_NOTE: merge _object_size and _name to
//
// jushort _object_size;
// jushort _name_index;
//
// if _name_index is 0, the name is stored in array._array_name, otherwise
// it's an index into the CP

class GenericClassInfoDesc : public OopDesc {
protected:
  jushort     _object_size;      // size of this ClassInfoDesc object.
  jushort     _vtable_length;    // length of Java vtable (in words)
  jushort     _itable_length;    // length of Java itable (in words)
  jushort     _class_id;         // Index for this class in 
                                 // Universe::class_list().
  SymbolDesc* _name;             // Name of this array class
  AccessFlags _access_flags;     // access flags
};

// Contains constant information for a JavaClassDesc. The data contained
// in ClassInfoDesc never changes after initialization, so it can be
// safely placed in ROM.
//
// ClassInfoDesc contains all fields defined in GenericClassInfoDesc, plus
// fields specific to InstanceClass and ArrayClass. The main reason of
// having GenericClassInfoDesc is to make it easy to do GC. It also
// makes life easier for ROMWriter::stream_class_info().
//
// Note: oopmap_ClassInfo knows about fields in GenericClassInfoDesc
// only. The fields declared in ClassInfoDesc must be marked individually.
class ClassInfoDesc : public GenericClassInfoDesc {
  // Union of fields valid for InstanceClass and ArrayClass.
  // This is used to make sure the Java level virtual dispatch table
  // starts at the same offset for both InstanceClass and ArrayClass.

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  public:
#endif

  struct _instance {
    ObjArrayDesc*      _methods;           // Method array
    TypeArrayDesc*     _fields;            // Instance and static variable
                                           // information, 5-tuples of
                                           // shorts [access, name_index,
                                           // sig_index, initval_index,
                                           // offset]
#if ENABLE_ISOLATES
    jint               _static_field_end;  // Need to quickly determine size
                                           // of mirror object.
#endif //ENABLE_ISOLATES

    TypeArrayDesc*     _local_interfaces;  // Interfaces this class declares
                                           // locally to implement (an array
                                           // of class_id)
#if ENABLE_REFLECTION
    TypeArrayDesc*     _inner_classes;     // Array of the inner classes
                                           // declared by this class
#endif

    ConstantPoolDesc*  _constants;         // Constant pool for this class
  };

  struct _array {
    // Fields for typeArrayClass
    jint _type;                            // type of the elements
    jint _scale;                           // size of the elements
  };

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  private:
#endif

  union {
    _instance instance;
    _array    array;
  };

public:
  static jint header_size() {
    return sizeof(ClassInfoDesc);
  }

  // Here starts the virtual dispatch table, followed by the interface
  // dispatch table

  static size_t allocation_size(int vtable_length, size_t itable_size) {
    return align_allocation_size(sizeof(ClassInfoDesc)
         + vtable_length * sizeof(jobject)
         + itable_size);
  }

  void initialize(OopDesc* klass, jushort object_size, jushort vtable_length,
                  jushort itable_length,
                  ISOLATES_PARAM(jint static_field_size)
                  bool is_array)
  {
    _object_size   = object_size;
    _vtable_length = vtable_length;
    _itable_length = itable_length;
    if (is_array) {
      _access_flags.atomic_set_bits(JVM_ACC_ARRAY_CLASS |
                                    JVM_ACC_ABSTRACT    |
                                    JVM_ACC_FINAL);
    } else {
#if ENABLE_ISOLATES
      instance._static_field_end =
          static_field_size + TaskMirror::header_size();
#endif
    }
    OopDesc::initialize(klass);
  }

  void variable_oops_do(void do_oop(OopDesc**));

  size_t object_size() const {
    return _object_size;
  }

  OopDesc** vtable_base() {
    return (OopDesc**)  (((address)this) + header_size());
  }

friend class Method;
friend class ClassInfo;
friend class ROMWriter;
friend class ROMOptimizer;
friend class JavaClass;
friend class InstanceClass;
#if USE_LARGE_OBJECT_AREA
friend class LargeObject;
#endif
};
