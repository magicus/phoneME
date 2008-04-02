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

/**
 * This structure describes the internal layout of one location inside
 * the VirtualStackFrame. It's good for quick access of the VirtualStackFrame,
 * but it works only when GC doesn't occur. For the general case, use the
 * Location class instead (see Location.hpp)
 */
class CodeGenerator;
class RawLocation : public RawLocationData {
public:
  enum Status {
    flushed = 0, 
    cached  = 1,
    changed = 2
  };

  BasicType type() const {
    return (BasicType)_type;
  }
  void set_type(BasicType type) {
    _type = (jubyte)type;
  }

  BasicType stack_type() const {
    return stack_type_for(type());
  }
  bool is_two_word() const {
    return ::is_two_word(type());
  }

  Value::ValueLocation where() const {
    return (Value::ValueLocation)_where;
  }
  void set_where(Value::ValueLocation where) {
    _where = (jubyte)where;
  }
  Status status() const {
    return (Status)_status;
  }
  void set_status(Status status) {
    _status = (jubyte)status;
  }
  jubyte flags() const {
    return _flags;
  }
  void set_flags(jubyte flags) {
    _flags = flags;
  }
  void clear_flags() {
    set_flags(0);
  }
  jint value() const {
    return _value;
  }
  void set_value(jint value) {
    _value = value;
  }
  jint length() const {
    return _length;
  }
  void set_length(jint length) {
    _length = length;
  }
#if ENABLE_COMPILER_TYPE_INFO
  jushort class_id() const {
    GUARANTEE(_class_id < Universe::number_of_java_classes(), "Sanity");
    return _class_id;
  }
  void set_class_id(jushort class_id) {
    GUARANTEE(class_id < Universe::number_of_java_classes(), "Sanity");
    _class_id = class_id;
  }
  void reset_class_id() {
    GUARANTEE(Universe::object_class()->class_id() == 0,
              "java/lang/Object must be the first on class list");
    set_class_id(0);
  }
  void set_is_exact_type(void) {
    _flags |= Value::F_IS_EXACT_TYPE;
  }
  bool is_exact_type(void) {
    return (_flags & Value::F_IS_EXACT_TYPE) != 0;
  }
#endif

  Assembler::Register get_register() const {
    return (Assembler::Register)value();
  }

  // Status operations:
  bool is_flushed() const {
    return status() == flushed;
  }
  bool is_cached() const {
    return status() == cached;
  }
  bool is_changed() const {
    return status() == changed;
  }

  // Where operations
  bool is_present() const {
    return where() != Value::T_NOWHERE;
  }
  bool is_immediate() const {
    return where() == Value::T_IMMEDIATE;
  }
  bool in_register() const {
    return where() == Value::T_REGISTER;
  }

  void mark_as_flushed() {
    set_status(flushed);
  }
  void mark_as_cached() {
    set_status(cached);
  }
  void mark_as_changed() {
    set_status(changed);
  }

#if ENABLE_REMEMBER_ARRAY_LENGTH
  //mark the location is not first time access if the location is assigned 
  //same register. This is called when do set_is_not_first_time_access in
  //VirtualStackFrame::set_is_not_first_time_access().
  bool set_is_not_first_time_access(Assembler::Register reg ) {
    if (!is_flushed() && in_register() && uses_register(reg) ) {
      _flags |= Value::F_IS_NOT_FIRST_TIME_ACCESS;
      return true;
    }
    return false;
  }

  //set the value as not first time access.
  void set_is_first_time_access(void) {
    _flags = (jubyte) (_flags &  (~Value::F_IS_NOT_FIRST_TIME_ACCESS)); 
  }

#if ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE  
  //clear array length checked tag if the value
  //is modified.
  void set_is_not_index_checked() {
      if (type() == T_INT || type() == T_LONG) {
      set_flags(flags() & ~Value::F_HAS_INDEX_CHECKED);
    }
  }
#else
  void set_is_not_index_checked() {}
#endif

#else
  void set_is_not_index_checked() {}
  void set_is_first_time_access(void) {}
#endif
  
  // Read a value from the VSF
  void read_value(Value& value, int index);

  // Write a value to the VSF
  void write_value(const Value& value);

  // Mark this location as illegal
  void mark_as_illegal() {
     set_type(T_ILLEGAL);
     set_where(Value::T_NOWHERE);
     set_flags(0);
  }

  // spill a specific register into this location if it mapped by it
  void spill_register(const Assembler::Register reg, const int index) {
    if (!is_flushed() && in_register() && uses_register(reg)) {
      flush(index);
    }
  }

  void write_changes(const Assembler::Register reg, const int index) {
    if (is_changed() && in_register() && uses_register(reg)) {
      write_changes(index);
    }
  }

  // Modify a location which had been in Register src to instead be in
  // in Register dst.  All flags remain the same.
  void change_register(Assembler::Register dst, Assembler::Register src);

  void write_changes(int index);

  void flush(int index);

  bool has_register_conflict_with(int my_index,
                                  VirtualStackFrame *other_frame,
                                  RawLocation* other,
                                  int other_index);

  /*
   * Given the location and a bitwise mask of allowed actions in 
   * allowed_actions, determines the set S of actions required to conform this
   * location to the other one, performs all allowed actions from S and returns
   * a bitwise mask of all disallowed actions from S.
   */
  typedef unsigned int Actions;
  Actions do_conform_to(int my_index, RawLocation* other, int other_index, 
                        Actions allowed_actions);

  inline void conform_to(int my_index, RawLocation* other, int other_index) {
    do_conform_to(my_index, other, other_index, ALL_ACTIONS);
  }

  enum { LOC_LOAD    = 1 << 0, 
         LOC_STORE   = 1 << 1, 
         REG_STORE   = 1 << 2,
         ALL_ACTIONS = LOC_LOAD | LOC_STORE | REG_STORE,
         NO_ACTIONS  = 0
  };

 private:
  inline Actions merge_actions( const RawLocation* other ) const;
  static inline CodeGenerator* code_generator( void );

  RawLocation *next_location(void) {
    return this + 1;
  }

  const RawLocation *next_location(void) const {
    return this + 1;
  }

  bool uses_register( const Assembler::Register reg ) const {
    return get_register() == reg ||
           ( is_two_word() && next_location()->get_register() == reg );
  }

  static bool is_used_in(const VirtualStackFrame* frame, const Value& value);

  bool is_register_identical_to(int my_index, RawLocation* other,
                                int other_index);
  void update_cache(int index);
};

class Location: public StackObj {
 public:

  // get the size of a location
  static int size(void) {
    return sizeof(RawLocation);
  }
  int index(void) const {
    return _index;
  }
  VirtualStackFrame* frame(void) const {
    return _frame;
  }

 public:
  Location(VirtualStackFrame* frame, const int index) :
      _index(index), _frame(frame) { }

  inline RawLocation *frame_raw_location_at(int index) const {
    return frame()->raw_location_at(index);
  }

  RawLocation *raw_location(void) const {
    return frame_raw_location_at(index());
  }

  void read_value(Value& value) const {
    raw_location()->read_value(value, index());
  }
  void write_value (const Value& value) const {
    raw_location()->write_value(value);
  }

  // flush cached or changed values into memory
  void flush(void) const {
    raw_location()->flush(index());
  }

  BasicType type(void) const {
    return frame_raw_location_at(index())->type();
  }

#if ENABLE_COMPILER_TYPE_INFO
  jushort class_id() const {
    return frame_raw_location_at(index())->class_id();
  }
#endif

  // accessors on the embedded value
  bool is_two_word(void) const { return ::is_two_word(type()); }

  bool is_immediate(void) const {
    return raw_location()->where() == Value::T_IMMEDIATE;
  }

  // accessors for status
  bool is_flushed(void) const {
    return raw_location()->is_flushed();
  }
  bool is_cached(void) const {
    return raw_location()->is_cached();
  }

  void mark_as_flushed(void) const {
    raw_location()->mark_as_flushed();
  }
  void mark_as_changed(void) const {
    raw_location()->mark_as_changed();
  }

  bool check_flags(const jubyte mask) const {
    return (raw_location()->flags() & mask) != 0;
  }

  bool must_be_null(void) const { 
    return check_flags(Value::F_MUST_BE_NULL);
  }

  bool must_be_nonnull(void) const { 
    return check_flags(Value::F_MUST_BE_NONNULL);
  }

  jint length(void) const {
    return raw_location()->length();
  }

  bool has_known_min_length(void) const { 
    return check_flags(Value::F_HAS_KNOWN_MIN_LENGTH);
  }

  bool not_on_heap(void) const {
    return check_flags(Value::F_NOT_ON_HEAP);
  }

  bool is_string(void) const {
#if ENABLE_COMPILER_TYPE_INFO
    return must_be_nonnull() && 
      raw_location()->class_id() == Universe::string_class()->class_id();
#else
    return check_flags(Value::F_IS_STRING);
#endif
  }

  bool is_string_array(void) const {
#if ENABLE_COMPILER_TYPE_INFO
    return must_be_nonnull() && 
      raw_location()->class_id() == 
      JavaClass::Raw(Universe::string_class()->array_class())().class_id();
#else
    return check_flags(Value::F_IS_STRING_ARRAY);
#endif
  }

  bool is_object_array(void) const {
#if ENABLE_COMPILER_TYPE_INFO
    return must_be_nonnull() && raw_location()->is_exact_type() &&
      raw_location()->class_id() == Universe::object_array_class()->class_id();
#else
    return check_flags(Value::F_IS_OBJECT_ARRAY);
#endif
  }

  // Modify a location which had been in Register src to instead be in
  // in Register dst.  All flags remain the same.
  void change_register(Assembler::Register dst, Assembler::Register src) const {
    raw_location()->change_register(dst, src);
  }

 private:
  const int           _index;
  VirtualStackFrame*  _frame;
};

// This class is used to iterate over the locations
// in the virtual stack frame.
class VSFStream {
 public:
  VSFStream(VirtualStackFrame* vsf) :
    _vsf(vsf), _index( 0 ),
    _vsf_virtual_stack_pointer( vsf->virtual_stack_pointer() ) {}

  // Return current location index
  int index(void) const {
    return _index;
  }

  // Have we reached the end of stream?
  bool eos(void) PRODUCT_CONST {
    GUARANTEE(_vsf_virtual_stack_pointer == _vsf->virtual_stack_pointer(),
              "virtual_stack_pointer cannot change when VSFStream is in use");
    return index() > _vsf_virtual_stack_pointer;
  }

  RawLocation* raw_location( void ) const {
    return _vsf->raw_location_at(_index);
  }

  // The BasicType of the current index
  BasicType type( void ) const {
   return raw_location()->type();
  }

  // Advance to next index
  void next( void ) {
    BasicType t = type();
    _index += (is_two_word(t)) ? 2 : 1;
  }

 private:
  // IMPL_NOTE: change the two ints to shorts for better performance on 16-bit
  // memory?
  int                _index;
  int                _vsf_virtual_stack_pointer;
  VirtualStackFrame* _vsf;
};

#endif // ENABLE_COMPILER
