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

class InstanceSize {
public:
  InstanceSize(jint value) {
    GUARANTEE((value < 0) || (((juint)value) < 0x8000), 
              "InstanceSize must fit in a single jshort");
    _value = value;
  }

  bool is_fixed() const {
    return _value > 0;
  }
  size_t fixed_value() const {
    return _value;
  }
  jint value() const {
    return _value;
  }

  // The order and values of the first 5 entries is important as it is
  // used in cpu/arm/NativeGenerator_arm.cpp to jump to the appropriate
  // array copy routine

  enum {
    size_obj_array          = -1,   // instance is array of Java objects
    size_type_array_1       = -2,   // instance is boolean[], byte[]
    size_type_array_2       = -3,   // instance is short[], char[]
    size_type_array_4       = -4,   // instance is int[], float[]
    size_type_array_8       = -5,   // instance is long[], double[]

    size_instance_class     = -6,   // instance is an instance class
    size_obj_array_class    = -7,   // instance is an object array class
    size_type_array_class   = -8,   // instance is a typed array class 

    size_generic_near       = -9,   // instance is the simplest near
    size_java_near          = -10,  // instance is a near for a Java object
    size_obj_near           = -11,  // instance is a near for a method
    size_far_class          = -12,  // instance is a simple far class

    size_mixed_oop          = -13,  // instance is a MixedOop
    size_task_mirror        = -14,  // isolate task mirror
    size_boundary           = -15,  // instance is a Boundary

    size_maximum_heap_value = -15,

    // The types above *may* appear in a romized heap section, just fits
    // in 4 bits...

    size_entry_activation   = -16,  // instance is an entry activation
    size_execution_stack    = -17,  // instance is an execution stack
    size_symbol             = -18,  // instance is symbol
    size_method             = -19,  // instance is method
    size_constant_pool      = -20,  // instance is constant pool
    size_class_info         = -21,  // instance is an class info
    size_compiled_method    = -22,  // instance is compiled method
    size_stackmap_list      = -23,  // instance is a stackmap list
    size_refnode            = -24,  // debugger mapping from object ID's to
                                    // objects

    size_minimum_value      = -24
  };
private:
  jint _value;
};

class FarClassDesc: public OopDesc {
 public:

#ifdef __GNUC__
  FarClassDesc(): _instance_size(0) { SHOULD_NOT_REACH_HERE(); }
#endif

  void initialize(OopDesc* klass, size_t object_size, jint instance_size,
                  jubyte* extern_oop_map) {
    GUARANTEE((object_size <= 0x7fff), "Range check");
    OopDesc::initialize(klass);
    _object_size = (jushort)object_size;
    InstanceSize is(instance_size);
    _instance_size = (jshort)instance_size;
    GUARANTEE((instance_size > 0) == (extern_oop_map == NULL), "Consistency");
    GUARANTEE((instance_size >= InstanceSize::size_minimum_value) &&
              (instance_size <= 0x7fff), "Range check");
    _oop_map._extern = extern_oop_map;
  }

  /// size of instances whose blueprint is this FarClassDesc object
  InstanceSize instance_size() const {
    InstanceSize is(_instance_size);
    return is;
  }

  jint instance_size_as_jint() const {
    return (jint)_instance_size;
  }

  /// size of this FarClassDesc object
  size_t object_size() const {
    return _object_size;
  }

  jubyte* embedded_oop_map() const { 
    GUARANTEE(instance_size().value() > 0, "Not an instance");
    return (jubyte*) this + _oop_map._embedded_start;
  }

  jubyte* extern_oop_map() const {
    GUARANTEE(instance_size().value() < 0, 
              "Instances don't have external oop map");
    return _oop_map._extern; 
  }

  bool instance_is_compiled_method() {
    return instance_size_as_jint() == InstanceSize::size_compiled_method;
  }
  bool instance_is_method() {
    return instance_size_as_jint() == InstanceSize::size_method;
  }
  bool instance_is_execution_stack() {
    return instance_size_as_jint() == InstanceSize::size_execution_stack;
  }
  bool instance_is_task_mirror() {
    return instance_size_as_jint() == InstanceSize::size_task_mirror;
  }
  bool instance_is_boundary() {
    return instance_size_as_jint() == InstanceSize::size_boundary;
  }


protected:
  /**
   * Size of this FarClassDesc object
   */
  jushort            _object_size;

  /**
   * Size of an instance whose blueprint() is this FarClassDesc object
   */
  jshort             _instance_size;

  union {
    jint             _embedded_start; // offset of embedded oop map (if any)
    jubyte*          _extern;         // pointer to pregenerated oop map
  } _oop_map;
  OopDesc*           _prototypical_near;

  friend class FarClass;
};
