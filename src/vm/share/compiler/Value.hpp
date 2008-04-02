/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

#if ENABLE_COMPILER

class RawLocation;
class Location;

class Value : public StackObj {
 public:
  // creation/destruction
  Value(const BasicType type = T_ILLEGAL)   { initialize(type); }
  Value(RawLocation* loc, const int index)  { initialize(loc, index); }
  Value(Location* loc)                      { initialize(loc); }
  ~Value() { destroy(); }

  void set_type(BasicType type) {
    GUARANTEE(_type == T_ILLEGAL, "Can only set type of uninitialized");
    _type = (jubyte)type;
  }

  // attributes
  BasicType type() const          { return (BasicType)_type; }
  BasicType stack_type()  const   { return stack_type_for(type()); }

  // testers
  bool is_present()   const       { return _where != 0;}
  bool is_immediate() const       { return _where == T_IMMEDIATE; }
  bool in_register()  const       { return _where == T_REGISTER;  }

  bool is_one_word() const        { return !::is_two_word(type()); }
  bool is_two_word() const        { return  ::is_two_word(type()); }

  bool use_two_registers() const {
    // IMPL_NOTE: replace with !USE_FPU or similar
#if defined(ARM) || defined(HITACHI_SH)
    return is_two_word();
#else
    return type() == T_LONG;
#endif
  }

  bool must_be_null() const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    // Eventually, we'll preserve the _flags in the frame, but not yet.
    return (_flags & F_MUST_BE_NULL) != 0;
  }

  bool must_be_nonnull() const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    return (_flags & F_MUST_BE_NONNULL) != 0;
  }

  bool has_known_min_length(int& length) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    length = _length;
    return (_flags & F_HAS_KNOWN_MIN_LENGTH) != 0;
  }

  bool not_on_heap(void) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    return (_flags & F_NOT_ON_HEAP) != 0;
  }

  bool is_string(void) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
#if ENABLE_COMPILER_TYPE_INFO
    GUARANTEE(Universe::string_class()->is_final_type(), 
              "Assuming String is final");
    return must_be_nonnull() &&
      class_id() == Universe::string_class()->class_id();
#else
    return (_flags & F_IS_STRING) != 0;
#endif
  }

  bool is_string_array(void) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
#if ENABLE_COMPILER_TYPE_INFO    
    GUARANTEE(Universe::string_class()->is_final_type(), 
              "Assuming String is final");
    return must_be_nonnull() && class_id() == 
      JavaClass::Raw(Universe::string_class()->array_class())().class_id();
#else
    return (_flags & F_IS_STRING_ARRAY) != 0;
#endif
  }

  bool is_object_array(void) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
#if ENABLE_COMPILER_TYPE_INFO
    return must_be_nonnull() && is_exact_type() &&
      class_id() == Universe::object_array_class()->class_id();
#else
    return (_flags & F_IS_OBJECT_ARRAY) != 0;
#endif
  }

 void set_must_be_null(void) {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    GUARANTEE(!must_be_nonnull(), "Can't be both null and nonnull");
    _flags = (jubyte) (_flags | F_MUST_BE_NULL);
  }

  void set_must_be_nonnull(void)    {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    GUARANTEE(!must_be_null(), "Can't be both null and nonnull");
   _flags = (jubyte) (_flags | F_MUST_BE_NONNULL);
  }

#if ENABLE_REMEMBER_ARRAY_LENGTH

#if ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE
  //The local variable has be taken as an array index
  //and emitted array length checking code.
  void set_must_be_index_checked(void) { 
    GUARANTEE(type() == T_INT || type() == T_LONG, "int type");
   _flags = (jubyte) (_flags | F_HAS_INDEX_CHECKED);
  }
#endif

  //check whether the value is first time access
  bool is_not_first_time_access() const { 
    return (_flags & F_IS_NOT_FIRST_TIME_ACCESS) != 0;
  }

  //set the value as not first time access.
  void set_is_not_first_time_access(void) {
    _flags = (jubyte) (_flags | F_IS_NOT_FIRST_TIME_ACCESS);
  }

    
  //we will load the array length into the same register
  //when jump into the jit code through a osr entry, so we check
  //the location of the VirtualStackFrame whether a register has been
  //bundle with it.
  void try_to_assign_register() {
    if (!is_not_first_time_access()) {
      assign_register();
    }
  }
#else

  void try_to_assign_register() {
    assign_register();
  }
#endif

  void set_has_known_min_length(int length)    {
     GUARANTEE(!must_be_null(), "Can't be both null and nonnull");
     _flags = (jubyte) (_flags | (F_MUST_BE_NONNULL | F_HAS_KNOWN_MIN_LENGTH));
     _length = length;
  }

  void set_not_on_heap(void) {
    _flags = (jubyte) (_flags | F_NOT_ON_HEAP);
  }

#if ENABLE_COMPILER_TYPE_INFO

  void set_class_id(jushort class_id) {
    GUARANTEE(class_id < Universe::number_of_java_classes(), "Sanity");
    _class_id = class_id;
  }

  jushort class_id() const {
    GUARANTEE(_class_id < Universe::number_of_java_classes(), "Sanity");
    return _class_id;
  }

  void set_is_exact_type(void) {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    _flags = (jubyte) (_flags | F_IS_EXACT_TYPE);
  }

  bool is_exact_type(void) const {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
    return (_flags & F_IS_EXACT_TYPE) != 0;
  }
#else

  void set_is_string(void) {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
   _flags = (jubyte) (_flags | F_IS_STRING);
  }

  void set_is_string_array(void) {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
   _flags = (jubyte) (_flags | F_IS_STRING_ARRAY);
  }

  void set_is_object_array(void) {
    GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "object type");
   _flags = (jubyte) (_flags | F_IS_OBJECT_ARRAY);
  }
#endif

  // set immediate
  void set_int(jint value);
#if ENABLE_FLOAT
  void set_float(jfloat value);
  void set_double(jdouble value);
#endif
  void set_long(jlong value);
  void set_raw_int(jint value);
  void set_raw_long(jlong value);
  void set_obj(Oop* value);

  // set immediate from static field (if final)
  void set_immediate_from_static_field(InstanceClass* ic, int offset);

  // get immediate
  jint as_int(void) const {
    GUARANTEE(is_immediate(), "check");
    GUARANTEE(type() == T_BOOLEAN || type() == T_INT || type() == T_BYTE ||
              type() == T_CHAR || type() == T_SHORT ||
              type() == T_OBJECT || type() == T_ARRAY, "check type");
    return _low;
  }

  jlong  as_long(void) const    {
    GUARANTEE(is_immediate() && type() == T_LONG,   "check");
    return *(jlong*)&_low;
  }

#if ENABLE_FLOAT
  jfloat  as_float() const   {
    GUARANTEE(is_immediate() && type() == T_FLOAT,  "check");
    return *(jfloat*)(&_low);
  }

  jdouble as_double() const  {
    GUARANTEE(is_immediate() && type() == T_DOUBLE, "check");
    return *(jdouble*)&_low;
  }

  jint    as_raw_int() const {
    GUARANTEE(is_immediate() && type() == T_FLOAT,   "check");
    return _low;
  }

  jlong   as_raw_long() const{
    GUARANTEE(is_immediate() && type() == T_DOUBLE,  "check");
    return *(jlong*)&_low;
  }
#endif

  jint lo_bits() const {
    // the part of an immediate value to be stored in lo_register

#if CROSS_GENERATOR
    // For cross-generator we should handle the case when the layout of long
    // or double values is different on host and target platforms.
#if TARGET_MSW_FIRST_FOR_LONG != MSW_FIRST_FOR_LONG
    if (type() == T_LONG) {
      return _high;
    }
#endif // TARGET_MSW_FIRST_FOR_LONG != MSW_FIRST_FOR_LONG
#if ENABLE_FLOAT
#if TARGET_MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_DOUBLE
    if (type() == T_DOUBLE) {
      return _high;
    }
#endif // TARGET_MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_DOUBLE
#endif // ENABLE_FLOAT
#endif // CROSS_GENERATOR

    return _low;
  }

  jint hi_bits() const {
    // the part of an immediate value to be stored in hi_register

#if CROSS_GENERATOR
    // For cross-generator we should handle the case when the layout of long
    // or double values is different on host and target platforms.
#if TARGET_MSW_FIRST_FOR_LONG != MSW_FIRST_FOR_LONG
    if (type() == T_LONG) {
      return _low;
    }
#endif // TARGET_MSW_FIRST_FOR_LONG != MSW_FIRST_FOR_LONG
#if ENABLE_FLOAT
#if TARGET_MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_DOUBLE
    if (type() == T_DOUBLE) {
      return _low;
    }
#endif // TARGET_MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_DOUBLE
#endif // ENABLE_FLOAT
#endif // CROSS_GENERATOR

    return _high;
  }

  jint lsw_bits() const {
    GUARANTEE(is_immediate() && type() == T_LONG,   "check");
    return (MSW_FIRST_FOR_LONG ? _high : _low);
  }

  jint msw_bits() const {
    GUARANTEE(is_immediate() && type() == T_LONG,   "check");
    return (MSW_FIRST_FOR_LONG ? _low : _high);
  }

  // set registers
  void set_register(Assembler::Register reg);
  void set_registers(Assembler::Register low, Assembler::Register high);

#if ENABLE_ARM_VFP
  void set_vfp_double_register(Assembler::Register reg);
#endif

  void assign_register();
  void materialize();

  void copy(Value& result);
  void writable_copy(Value& result);

#ifndef ARM
  void force_to_byte_register();
#endif

  // get registers
  Assembler::Register lo_register() const {
    GUARANTEE(in_register(), "value must be in a register");
    return (Assembler::Register)_low;
  }

  Assembler::Register hi_register() const {
    GUARANTEE(use_two_registers(), "must be a two-register value");
    return (Assembler::Register)_high;
  }

  Assembler::Register lsw_register() const {
    GUARANTEE(in_register() && type() == T_LONG,   "check");
    return (Assembler::Register)(TARGET_MSW_FIRST_FOR_LONG ? _high : _low);
  }

  Assembler::Register msw_register() const {
    GUARANTEE(in_register() && type() == T_LONG,   "check");
    return (Assembler::Register)(TARGET_MSW_FIRST_FOR_LONG ? _low : _high);
  }

  void destroy();

  enum ValueLocation {
    T_NOWHERE   = 0,
    // The following two flags are mutually exclusive
    T_IMMEDIATE = 1,
    T_REGISTER  = 2
  };

  enum ValueNullFlags {
    // The following two are mutually exclusive
    F_MUST_BE_NULL             = 1, // value is guaranteed to be null
    F_MUST_BE_NONNULL          = 2, // value is guaranteed not to be null
    F_HAS_KNOWN_MIN_LENGTH     = 4, // _length field contains min array length
    F_NOT_ON_HEAP              = 8, // NULL or not on heap

#if ENABLE_COMPILER_TYPE_INFO
    F_IS_EXACT_TYPE            = 16 // _class_id is the exact value class id
                                    // otherwise it can be a superclass id
#else
    F_IS_STRING                = 16, // String.  Not NULL
    F_IS_STRING_ARRAY          = 32, // String[].  Not NULL
    F_IS_OBJECT_ARRAY          = 64  // Object[].  Not NULL.
#endif

#if ENABLE_REMEMBER_ARRAY_LENGTH
     ,
#if ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE
    F_HAS_INDEX_CHECKED = 4, //Value is integer type and taken as array index variable
#endif 
    F_IS_NOT_FIRST_TIME_ACCESS = 128 // Array Length, Not the first time access
#endif    
  };

 private:
  // The value type enumeration.
  // Both type and where is packed into on integer

  jubyte  _type;
  jubyte  _where;
  jubyte  _flags;               // Cleared on merge
  jubyte  _unused_padding;      // Cleared on merge

  jint    _length;              // When T_KNOWN_LENGTH.

  jint    _low;                 // low register or bits of low address
  jint    _high;                // high register or bits of high address

#if ENABLE_COMPILER_TYPE_INFO
  jushort _class_id;
#endif

#ifdef SPARC
  jdouble _unused_dummy1;       // Force gcc to put this stack value on
  jlong   _unused_dummy2;       // 8-byte boundaries.
#endif

  ValueLocation where() const {
    return (ValueLocation) _where;
  }

  void set_where(ValueLocation value) {
    GUARANTEE(value == T_NOWHERE || value == T_IMMEDIATE || value == T_REGISTER,
              "must set zero or one bit only");
    _where = (jubyte)value;
  }

  jubyte flags() const { return _flags; }
  void set_flags(jubyte flags) { _flags = flags; }

  jint  length() const { return _length; }
  void  set_length(jint length) { _length = length; }

  void set_low_word(jint lo) {
    _low = lo;
  }
  void set_high_word(jint hi) {
    _high = hi;
  }
  jint low_word(void) const {
    return _low;
  }
  jint high_word(void) const {
    return _high;
  }

  void initialize( const BasicType type ) {
    _type = (jubyte)type;
    _where = 0;
    _flags = 0;
#if ENABLE_COMPILER_TYPE_INFO
    GUARANTEE(Universe::object_class()->class_id() == 0,
              "java/lang/Object must be the first on class list");
    _class_id = 0;
#endif
  }

  void initialize(RawLocation* loc, const int index);
  void initialize(Location*    loc);

  friend class RawLocation;
  friend class ExtendedValue;
  friend class ForwardBranchOptimizer;
};

class ExtendedValue {
  // An extended value represents one of
  //   *) A Value (an immediate result or a result in a register)
  //   *) An immediate oop (which can't be represented by a Value)
  //   *) A location in memory (a local or stack value)
  //
  //  Note, we use the type() and flag() fields of the Value to represent
  //  the type and flags() of the extended value
private:
  int                _xvalue_type;
  Value              _value;
  FastOopInStackObj  _must_precede_fast_oop;
  Oop::Fast          _oop;
  int                _index;
public:
  ExtendedValue() : _xvalue_type(XIllegal) {}

  enum { XIllegal, XValue, XOop, XIndex };

  void is_value(BasicType kind) {
    GUARANTEE(_xvalue_type == XIllegal, "Sanity");
    _value.set_type(kind);
    _xvalue_type = XValue;
  }

  Value& value(void) {
    GUARANTEE(_xvalue_type == XValue, "Sanity");
    return _value;
  }

  const Value& value(void) const {
    GUARANTEE(_xvalue_type == XValue, "Sanity");
    return _value;
  }

  Oop* oop() {
    GUARANTEE(_xvalue_type == XOop, "Sanity");
    return &_oop;
  }

  void set_oop(Oop* value) {
    GUARANTEE(_xvalue_type == XIllegal, "Sanity");
    GUARANTEE(value->not_null(), "Sanity");
    _xvalue_type = XOop;
    _oop = value->obj();
    _value.set_type(T_OBJECT);
  }

  int index() const {
    GUARANTEE(_xvalue_type == XIndex, "Sanity");
    return _index;
  }
  void  set_index(int index, BasicType kind) {
    GUARANTEE(_xvalue_type == XIllegal, "Sanity");
    _xvalue_type = XIndex;
    _index = index;
    _value.set_type(kind);
  }

  BasicType type(void) const { return _value.type(); }
  jubyte flags  (void) const { return _value.flags(); }
  void set_flags(jubyte flags) { _value.set_flags(flags); }

  bool is_value (void) const { return _xvalue_type == XValue; }
  bool is_index (void) const { return _xvalue_type == XIndex; }
  bool is_oop   (void) const { return _xvalue_type == XOop;   }

  void set_obj(Oop* value);
};

#endif
