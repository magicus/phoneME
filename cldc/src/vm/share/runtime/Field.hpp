/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/** \class Field
    Describes a field variable in a class.

 */

class Field: public StackObj {
 public:
  Field(InstanceClass* ic, Symbol* name, Symbol* signature) {
    initialize(ic, name, signature);
  }
  Field(InstanceClass* ic, const jint index) {
    initialize(ic, index);
  }
  Field(InstanceClass* ic, const jint index, TypeArray* fields) {
    initialize(ic, index, fields);
  }

  // ^Symbol
  ReturnOop name                  (void) const;
  ReturnOop signature             (void) const;
  const AccessFlags access_flags  (void) const { return _access; }
  jushort offset                  (void) const { return _offset; }
  bool is_valid                   (void) const { return _index >= 0; }
  bool is_valid_in_current_profile(void) const {
    return is_valid() && !ROM::is_hidden_field(_ic->class_id(), _index);
  }

  // Initial field value
  bool has_initial_value  (void) const { return _initval_index != 0; }
  int  initval_index      (void) const { return _initval_index; }

  // Field signature type
  BasicType type(void) const;

  // Access flags
  bool is_public          (void) const { return access_flags().is_public();    }
  bool is_private         (void) const { return access_flags().is_private();   }
  bool is_protected       (void) const { return access_flags().is_protected(); }
  bool is_static          (void) const { return access_flags().is_static();    }
  bool is_final           (void) const { return access_flags().is_final();     }
  bool is_volatile        (void) const { return access_flags().is_volatile();  }
  bool is_transient       (void) const { return access_flags().is_transient(); }
  bool is_synthetic       (void) const { return access_flags().is_synthetic(); }
  bool is_package_private (void) const { return access_flags().is_package_private(); }

  void check_access_by(InstanceClass* sender_class, 
                       InstanceClass* static_receiver_class,
                       FailureMode fail_mode JVM_TRAPS) const;

  // Offsets of array slots in 'InstanceClass::fields()' as provided
  // by the class file parser.
  // See also the implementation of 'Field::initialize()'
  typedef enum {
    ACCESS_FLAGS_OFFSET   = 0,
    NAME_OFFSET           = 1,
    SIGNATURE_OFFSET      = 2,
    INITVAL_OFFSET        = 3,
    OFFSET_OFFSET         = 4,
    NUMBER_OF_SLOTS       = 5 // per field
  } FieldAttributeArrayIndex;

  InstanceClass* holder(void) const { return _ic; }

  int byte_size(void) const {
    return ::byte_size_for(type());
  }
  void print_on(Stream*) const PRODUCT_RETURN;
  void p(void) const PRODUCT_RETURN;
  void print_cp_symbol_on(Stream *, int /*cp_index*/) const PRODUCT_RETURN;

#ifndef PRODUCT
  virtual ReturnOop get_fields_for(const InstanceClass* ic) const {
    return ic->fields();
  }

  virtual ReturnOop get_constants_for(const InstanceClass* ic) const {
    return ic->constants();
  }
#else
#if USE_PRODUCT_BINARY_IMAGE_GENERATOR
  virtual ReturnOop get_fields_for(const InstanceClass* ic) const {
    return ic->fields();
  }

  virtual ReturnOop get_constants_for(const InstanceClass* ic) const {
    return ic->constants();
  }
#else
  ReturnOop get_fields_for(const InstanceClass* ic) const  {
    return ic->fields();
  }

  ReturnOop get_constants_for(const InstanceClass* ic) const {
    return ic->constants();
  }
#endif
#endif

 protected:
  Field() {}
  void initialize(InstanceClass* ic, Symbol* name, Symbol* signature);
  void initialize(InstanceClass* ic, const jint index);
  void initialize(InstanceClass* ic, const jint index, const TypeArray* fields);
  InstanceClass* ic(void) const { return _ic; }
  int find_field_index(InstanceClass* ic, Symbol* name, Symbol* signature) const;
  int find_field_in_class_and_interfaces(InstanceClass* ic, 
                                         Symbol* name, 
                                         Symbol* signature) const;
  
  InstanceClass* _ic;
  jint           _index;
  jushort        _name_index;
  jushort        _signature_index;
  jushort        _initval_index;
  AccessFlags    _access;
  jushort        _offset;
};

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || USE_DEBUG_PRINTING

/** \class OriginalField
    The class is similar to Field, except it operates on the original
    set of fields of an InstanceClass (before the fields are renamed by
    the romizer. <p>

    When the romizer renames the fields of an InstanceClass, it saves
    the original set of fields in ROM::_original_fields_list. See ROM.hpp
    for more information.
 */

class OriginalField: public Field {
protected:
  virtual ReturnOop get_fields_for(const InstanceClass* ic) const {
    return ic->original_fields();
  }

  virtual ReturnOop get_constants_for(const InstanceClass* ic) const {
    ReturnOop cp = ROM::alternate_constant_pool(ic);
    return cp ? cp : ic->constants();
  }

public:
  OriginalField(InstanceClass* ic, Symbol* name, Symbol* signature): Field() {
    initialize(ic, name, signature);
  }
  OriginalField(InstanceClass* ic, const jint index): Field() {
    initialize(ic, index);
  }
  OriginalField(InstanceClass* ic, const jint index, const TypeArray* fields):
  Field() {
    initialize(ic, index, fields);
  }

  bool is_renamed(void) const PRODUCT_RETURN0;
};

#endif
