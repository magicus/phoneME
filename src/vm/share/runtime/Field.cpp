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
# include "incls/_Field.cpp.incl"

// Find field matching (name,signature) in this class or in superclass
void Field::initialize(InstanceClass* ic, Symbol* name, Symbol* signature) {
  _index = -1;
  
  // Refer to JVM Spec 5.4.3.2 for an in-depth explanation
  // For resolving a field in class C
  // 1. Look in the class C
  // 2. Otherwise recursively look in the *direct* superinterfaces of class C
  // 3. Otherwise recursively look in the superclasses of class C
  //    following the same order as in 1 & 2.
   
  while (ic->not_null()) {
    // look for the field in the class and its *direct* superinterfaces 
    int field_index = find_field_in_class_and_interfaces(ic, name, signature);
    if (field_index >= 0) {
      initialize(ic, field_index);
      return;
    }
    *ic = ic->super();
  }
}

Field::Field(InstanceClass* ic, jint index) {
  initialize(ic, index);
}

void Field::initialize(InstanceClass* ic, jint index) {
  _ic              = ic;
  _index           = index;
  TypeArray::Raw f = get_fields_for(_ic);
  jint flags       = f().ushort_at(index + ACCESS_FLAGS_OFFSET);
  _access.set_flags(flags);
  _name_index      = f().ushort_at(index + NAME_OFFSET);
  _signature_index = f().ushort_at(index + SIGNATURE_OFFSET);
  _initval_index   = f().ushort_at(index + INITVAL_OFFSET);
  _offset          = f().ushort_at(index + OFFSET_OFFSET);
}

ReturnOop Field::name() PRODUCT_CONST {
  ConstantPool::Raw cp = get_constants_for(_ic);
  Symbol::Raw n = cp().symbol_at(_name_index);
  return n;
}

ReturnOop Field::signature() PRODUCT_CONST {
  ConstantPool::Raw cp = get_constants_for(_ic);
  return cp().symbol_at(_signature_index);
}

BasicType Field::type() PRODUCT_CONST {
  FieldType::Raw sig = signature();
  return sig().basic_type();
}

void Field::check_access_by(InstanceClass* sender_class,
                            InstanceClass* static_receiver_class,
                            FailureMode fail_mode JVM_TRAPS) {
  InstanceClass* field_class = ic();
  if (is_public()) {
    return;
  }
  if (field_class->equals(sender_class)) {
    return;
  }
  if (!is_private()) {
    if (field_class->is_same_class_package(sender_class)) {
      return;
    }
    if (is_protected()) {
      if (sender_class->is_subclass_of(field_class)) {
        if (static_receiver_class->equals(sender_class) || 
            static_receiver_class->is_subclass_of(sender_class) || 
            sender_class->is_subclass_of(static_receiver_class)) {
          return;
        }
      }
    }
  }
  Throw::illegal_access(fail_mode JVM_NO_CHECK_AT_BOTTOM);
}

int Field::find_field_in_class_and_interfaces(InstanceClass* ic, 
                                              Symbol* name, 
                                              Symbol* signature) {
  AllocationDisabler shouldnt_allocate_in_this_function;
  int field_index = -1;

  // look for the field in this interface
  field_index = find_field_index(ic, name, signature);
  if (field_index >= 0) {
    return field_index;
  }

  InstanceClass::Raw interface_class;
  TypeArray::Raw interfaces = ic->local_interfaces();
  const int n_interfaces = interfaces().length();

  // recursively look for the field in superinterfaces
  for (int i = 0; i < n_interfaces; ++i) {
    interface_class = Universe::class_from_id(interfaces().ushort_at(i));

    field_index = find_field_in_class_and_interfaces(&interface_class, 
                                                     name, signature);
    if (field_index >= 0) {
      *ic = &interface_class;
      return field_index;
    }
  }

  return -1;
}

#ifndef PRODUCT
bool OriginalField::is_renamed() {
  if (ROM::get_original_fields(_ic) == NULL) {
    return false;
  }

  ConstantPool::Raw orig_cp = ROM::alternate_constant_pool(_ic);
  Symbol::Raw orig_name = orig_cp().symbol_at(_name_index);

  TypeArray::Raw new_fields = _ic->fields();
  ConstantPool::Raw new_cp = _ic->constants();

  int len = new_fields().length();
  for (int i=0; i<len; i+=5) {
    int name_index = new_fields().ushort_at(i + Field::NAME_OFFSET);
#if ENABLE_ISOLATES
    // IMPL_NOTE: this should be removed once ConstantPoolRewriter::rewrite_index()
    // is fixed.
    if (name_index == 0) {
      return true;
    }
#endif
    Symbol::Raw new_name = new_cp().symbol_at(name_index);
    if (new_name.equals(&orig_name)) {
      return false;
    }
  }

  return true;
}

void Field::p() {
  print_on(tty);
}

void Field::print_on(Stream *st) {
#if USE_DEBUG_PRINTING
  st->print_cr("Field (StackObj) = { ");
  st->print("    ic              ");
  if (_ic->not_null()) {
    _ic->print_name_on(st);
    st->cr();
  } else {
    st->print_cr("null");
  }

  st->print_cr("    index           %d",   _index);

  st->print   ("    name_index      %d: ", _name_index);
  print_cp_symbol_on(st, _name_index);
  st->cr();

  st->print   ("    signature_index %d: ", _name_index);
  print_cp_symbol_on(st, _signature_index);
  st->cr();

  st->print_cr("    initval_index   %d",   _initval_index);
  // IMPL_NOTE: print textual names of access flags
  st->print_cr("    access          0x%x", _access.as_int());
  st->print_cr("    offset          %d",   _name_index);
  st->print_cr("}");
#endif
}

void Field::print_cp_symbol_on(Stream *st, int cp_index) {
#if USE_DEBUG_PRINTING
  if (_ic->not_null()) {
    ConstantPool::Raw cp = get_constants_for(_ic);
    Symbol::Raw s = cp().symbol_at(cp_index);
    s().print_symbol_on(st);
  } else {
    st->print("??");
  }
#endif
}

#endif
