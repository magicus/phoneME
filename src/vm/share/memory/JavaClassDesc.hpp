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

/**
 * This class describes the layout of the fields that are common to
 * Java InstanceClass and ArrayClass.
 */
class JavaClassDesc: public FarClassDesc {
 public:
  ClassInfoDesc* _class_info;     // Info for this class that remains constant
  FarClassDesc*  _subtype_cache_1;// used for fast is_a test
  FarClassDesc*  _subtype_cache_2;

  // array class holding elements of this class
  FarClassDesc* _array_class;
#if ENABLE_ISOLATES 
  // The JavaClassDesc object does not contain static variable, java mirror
  // or references to array_class. Instead, a Task Mirror object referenced
  // from the current mirror list at the index equal to the class's id
  // holds these items.
#else
  InstanceDesc*  _java_mirror;    // instance of java/lang/Class mirroring
                                  // this class
#endif

  FarClassDesc*  _super;          // superclass

  struct _instance {
    JavaClassDesc* _next;         // Next Java class with same hash value
  };
  struct _array {
    JavaClassDesc* _element_class;// The klass of the elements of this
                                  // array type
  };

  union {
    _instance instance;
    _array    array;
  };

#if !ENABLE_ISOLATES 
  // Here starts the static fields
#endif

  // Here starts the instance oop map
  // Here starts the static oop map
#if !ENABLE_ISOLATES && USE_EMBEDDED_VTABLE_BITMAP
  // Here starts vtable methods bitmap
#endif
  // <-- END OF DATA STRUCTURE

  // It's very important that no other fields are stored here,
  // otherwise it will complicate the bitvector cleaning code in
  // ROMOptimizer::compact_static_field_containers()

  static jint header_size() {
    return sizeof(JavaClassDesc);
  }

 private:
  // Compute allocation size
  static size_t allocation_size(size_t static_field_size,size_t oop_map_size) {
    size_t size = sizeof(JavaClassDesc) + oop_map_size;

#if !ENABLE_ISOLATES 
    size += static_field_size;
#else
    (void)static_field_size;
#endif

    return align_allocation_size(size);
  }

  void initialize(OopDesc* klass, size_t object_size, jint instance_tag,
                  jubyte* extern_oop_map) {
    FarClassDesc::initialize(klass, object_size, instance_tag, extern_oop_map);
  }

  friend class JavaClass;
  friend class ClassFileParser;
  friend class Universe;
};
