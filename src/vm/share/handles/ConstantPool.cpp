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

# include "incls/_precompiled.incl"
# include "incls/_ConstantPool.cpp.incl"

HANDLE_CHECK(ConstantPool, is_constant_pool())

bool ConstantPool::_class_loading_suspended = false;

void ConstantPool::throw_invalid_constant_error(JVM_SINGLE_ARG_TRAPS) {
  Throw::error(invalid_constant JVM_THROW);
}

ReturnOop ConstantPool::checked_symbol_at(int index JVM_TRAPS) {
  if (is_within_bounds(index)) {
    int offset = offset_from_index(index);
    if (ConstantTag::is_symbol(tag_value_at(index))) {
      Oop::Raw result(obj_field(offset));
      if (result().is_symbol()) {
        return result.obj();
      } else {
        return resolve_symbol_at_offset(offset JVM_NO_CHECK_AT_BOTTOM);
      }
    }
  }

  Throw::error(invalid_constant JVM_NO_CHECK_AT_BOTTOM);
  return 0;
}

jint ConstantPool::name_and_type_at(int index JVM_TRAPS) const {
  int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
  cp_check_0(ConstantTag::is_name_and_type(tag_value_at(index)));
  int result = int_field(offset);
  GUARANTEE(result != 0, "sanity for JVM_ZCHECK");
  return result;
}

ReturnOop ConstantPool::unresolved_klass_at(int index JVM_TRAPS) const {
  int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
  cp_check_0(ConstantTag::is_unresolved_klass(tag_value_at(index)));
  return obj_field(offset);
}

ReturnOop ConstantPool::resolved_klass_at(int index JVM_TRAPS) const {
  int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
  cp_check_0(ConstantTag::is_resolved_klass(tag_value_at(index)));
  return Universe::class_from_id(int_field(offset));
}

BasicType ConstantPool::resolved_virtual_method_at(int index, int& vtable_index, 
                                                   int& class_id) const {
  GUARANTEE(tag_at(index).is_resolved_virtual_method(), 
            "Corrupt constant pool");
  vtable_index = extract_low_jushort_at(index);
  class_id  = extract_high_jushort_at(index);
  return ConstantTag::resolved_virtual_method_type(tag_value_at(index));
}

void ConstantPool::resolved_static_method_at_put(int index, Oop* m) {
  tag_at_put(index, JVM_CONSTANT_ResolvedStaticMethod);
  obj_field_put(offset_from_index(index), m);
}

void ConstantPool::klass_at_put(int index, JavaClass* k) {
  GUARANTEE(k->is_java_class(), "sanity");
  GUARANTEE(k->obj() == Universe::class_from_id(k->class_id()), "sanity");
  tag_at_put(index, JVM_CONSTANT_Class);
  // We may have put a symbol pointer here, so clear the write barrier bit
  OopDesc** klass = obj()->obj_field_addr(offset_from_index(index));
  // another possible location for the klass is mmaped/malloced Monet bundle
  if (ObjectHeap::contains(klass)) {
    ObjectHeap::clear_bit_for(klass);
  }
  int_field_put(offset_from_index(index), k->class_id());
}

bool ConstantPool::check_klass_at(int index) const {
  if (is_within_bounds(index)) {
    jubyte t = tag_value_at(index);
    return (ConstantTag::is_klass_index(t) ||
            ConstantTag::is_resolved_klass(t) || 
            ConstantTag::is_unresolved_klass(t));
  } else {
    return false;
  }
}

void ConstantPool::check_constant_at(int index, FieldType* type JVM_TRAPS) const
{
  jubyte t = tag_value_at(index);
  BasicType basic_type = type->basic_type();

  switch (basic_type) {
  case T_INT:
  case T_SHORT:
  case T_CHAR:
  case T_BYTE:
  case T_BOOLEAN:
    if (ConstantTag::is_int(t)) {
      return;
    }
    break;
  case T_LONG:
    if (ConstantTag::is_long(t)) {
      return;
    }
    break;
#if ENABLE_FLOAT
  case T_FLOAT:
    if (ConstantTag::is_float(t)) {
      return;
    }
    break;
  case T_DOUBLE:
    if (ConstantTag::is_double(t)) {
      return;
    }
    break;
#endif
  case T_OBJECT:
    if (ConstantTag::is_string(t) ||
        ConstantTag::is_unresolved_string(t)) {
      JavaClass::Raw cls = type->object_type();
      Symbol::Raw class_name = cls().name();
      if (class_name.equals(Symbols::java_lang_String())) {
        return;
      }
    }
    break;
  }

  Throw::class_format_error(invalid_constant JVM_THROW);
}

ReturnOop ConstantPool::resolve_symbol_at_offset(int offset JVM_TRAPS) {
  UsingFastOops fast_oops;
  TypeArray::Fast byte_array(obj_field(offset));
  {
    Symbol::Raw result= SymbolTable::symbol_for(&byte_array JVM_OZCHECK(result));
    obj_field_put(offset, &result);
    return result.obj();
  }
}

ReturnOop ConstantPool::checked_type_symbol_at(int index JVM_TRAPS) {
  if (is_within_bounds(index)) {
    int offset = offset_from_index(index);
    if (ConstantTag::is_symbol(tag_value_at(index))) {
      Oop::Raw result(obj_field(offset));
      if (result().is_symbol()) {
        return result.obj();
      } else {
        return resolve_type_symbol_at_offset(offset JVM_NO_CHECK_AT_BOTTOM);
      }
    }
  }

  Throw::error(invalid_constant JVM_NO_CHECK_AT_BOTTOM);
  return 0;
}

ReturnOop ConstantPool::resolve_type_symbol_at_offset(int offset JVM_TRAPS) {
  UsingFastOops fast_oops;
  TypeArray::Fast byte_array(obj_field(offset));

  {
    TypeSymbol::Raw type_symbol = 
      TypeSymbol::parse(&byte_array JVM_OZCHECK(type_symbol));
    obj_field_put(offset, &type_symbol);
    return type_symbol.obj();
  }
}

// A class name may be either a plain Symbol e.g., "java/lang/String"
// or a TypeSymbol, e.g., "[[Ljava/lang/String;"
ReturnOop ConstantPool::checked_class_name_at(int index JVM_TRAPS) {
  int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
  cp_check_0(ConstantTag::is_symbol(tag_value_at(index)));
  Oop::Raw result(obj_field(offset));
  if (result().is_symbol()) {
    return result.obj();
  } else {
    TypeArray::Raw byte_array(obj_field(offset));
    if (byte_array().length() > 1 && byte_array().byte_at(0) == '[') {
      return resolve_type_symbol_at_offset(offset JVM_NO_CHECK_AT_BOTTOM_0);
    } else {
      return resolve_symbol_at_offset(offset JVM_NO_CHECK_AT_BOTTOM_0);
    }
  }
}

ReturnOop ConstantPool::string_at(int index JVM_TRAPS) {
  if (ConstantTag::is_unresolved_string(tag_value_at(index))) {
    UsingFastOops fast_oops;
    Oop::Fast oop = unresolved_string_at(index);
    {
      String::Raw string =
        Universe::interned_string_from_utf8(&oop JVM_OZCHECK(string));
      string_at_put(index, &string);
      return string.obj();
    }
  } else {
    return resolved_string_at(index);
  }
}

ReturnOop ConstantPool::resolve_string_at(int index JVM_TRAPS) {
  int string_index = string_index_at(index);
  cp_check_0(is_within_bounds(string_index) &&
             ConstantTag::is_symbol(tag_value_at(string_index)));

  UsingFastOops fast_oops;
  Oop::Fast oop = obj_field(offset_from_index(string_index));
  {
    String::Raw string =
      Universe::interned_string_from_utf8(&oop JVM_OZCHECK(string));
    string_at_put(index, &string);
    return string;
  }
}

ReturnOop ConstantPool::klass_at(int index JVM_TRAPS) {
  cp_check_0(is_within_bounds(index));
  if (ConstantTag::is_unresolved_klass(tag_value_at(index))) {
    if (_class_loading_suspended) {
      GUARANTEE(ResolveConstantPoolInCompiler, "sanity");
      if (try_resolve_klass_at(index) == NULL) {
        Throw::error(class_not_resolved_during_compilation JVM_THROW_0);
      }
    } else {
      UsingFastOops fast_oops;
      Symbol::Fast class_name = unchecked_unresolved_klass_at(index);
      {
        JavaClass::Raw result =
          SystemDictionary::resolve(&class_name, ErrorOnFailure 
                                    JVM_OZCHECK(result));
        klass_at_put(index, &result);
      }
    }
  }
  return resolved_klass_at(index JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop ConstantPool::try_resolve_klass_at(int index) {
  GUARANTEE(tag_at(index).is_unresolved_klass(),
            "must be a valid unresolved class");
  Symbol::Raw class_name = unchecked_unresolved_klass_at(index);
  JavaClass::Raw result = SystemDictionary::find_class_or_null(&class_name);
  if (result.not_null()) {
    klass_at_put(index, &result);
  }
  return result.obj();
}

ReturnOop ConstantPool::name_of_klass_at(int index JVM_TRAPS) {
  cp_check_0(is_within_bounds(index));
  if (ConstantTag::is_unresolved_klass(tag_value_at(index))) {
    Symbol::Raw class_name = unchecked_unresolved_klass_at(index);
    return class_name;
  } else {
    JavaClass::Raw klass = resolved_klass_at(index JVM_OZCHECK(klass));
    return klass().name();
  }
}

ReturnOop ConstantPool::klass_ref_at(int index JVM_TRAPS) {
  jint class_index = klass_ref_index_at(index JVM_ZCHECK_0(class_index));
  return klass_at(class_index JVM_NO_CHECK_AT_BOTTOM);
}

// Returns the class in the field reference in static_receiver_class.
BasicType ConstantPool::field_type_at(int index, int& offset, bool is_static,
                                      bool is_get, InstanceClass*sender_class, 
                                      InstanceClass *declaring_class
                                      JVM_TRAPS) {
  if (ConstantTag::is_resolved_field(tag_value_at(index))) {
    if (!(GenerateROMImage && !ENABLE_MONET)
        && !sender_class->is_preloaded()) {
      // do following checks:
      // - check if staticness of access matches
      // - check if no write access to final take place, even if field 
      //   is already resolved
      //
      // We do this only for application classes -- we assume all
      // system classes are built with javac and will never contain
      // such bad bytecodes.
      if (ConstantTag::is_resolved_static_field(tag_value_at(index)) != is_static) {
        Throw::incompatible_class_change_error(field_changed 
                                               JVM_THROW_(T_ILLEGAL));
      }
      check_quickened_field_access(index, sender_class, is_static, is_get
                                   JVM_CHECK_(T_ILLEGAL));
    }

    int class_id;
    /*BasicType kind =*/ resolved_field_type_at(index, offset, class_id);
    *declaring_class = Universe::class_from_id(class_id);
    return resolved_field_type_at(index, offset);
  } else {
    GUARANTEE(tag_at(index).is_field(), "Index must point to a Fieldref");
    UsingFastOops fast_oops;
    // Get name, signature, and specified class
    Symbol::Fast field_name;
    FieldType::Fast field_type;
    InstanceClass::Fast static_receiver_class;
    resolve_helper(index, &field_name, &field_type,
                   &static_receiver_class JVM_CHECK_(T_ILLEGAL));

    // Resolve offset and declaring class (can be superclass or interface)
    *declaring_class = static_receiver_class.obj();
    Field f(declaring_class, &field_name, &field_type);

    // At this point, declaring_class points the actual class where the field
    // is defined -- this may be in an interface implemented by 
    // static_receiver_class, or a superclass of static_receiver_class.

    if (!f.is_valid()) {
      Throw::no_such_field_error(JVM_SINGLE_ARG_THROW_(T_ILLEGAL));
    }

    if (!TestCompiler) {
      // Check the accessibility of this field. Note: f.check_access_by()
      // also performs checks with respect to declaring_class!
      static_receiver_class().check_access_by(sender_class, ErrorOnFailure
                                              JVM_CHECK_(T_ILLEGAL));
      f.check_access_by(sender_class, &static_receiver_class,
                        ErrorOnFailure JVM_CHECK_(T_ILLEGAL));
      if (!is_get && f.is_final() && !sender_class->equals(declaring_class)) {
        Throw::illegal_access(ErrorOnFailure JVM_THROW_(T_ILLEGAL));
      }

      if (f.is_static() != is_static) {
        Throw::incompatible_class_change_error(field_changed 
                                               JVM_THROW_(T_ILLEGAL));
      }
    }

    BasicType type = field_type().basic_type();
    offset = f.offset(); // returns offset to caller
    resolved_field_at_put(index, declaring_class->class_id(), 
                          (jushort)offset, type, is_static);

    return type;
  }
}

/// Look up a Method for invoke_static, invoke_special and invoke_virtual.
/// Note that invoke_interface uses an alternate lookup routine.
ReturnOop
ConstantPool::lookup_method_at(InstanceClass *sender_class, int index,
                               Symbol* name, Symbol* signature,
                               InstanceClass* static_receiver_class JVM_TRAPS)
{
  UsingFastOops fast_oops;
  if (tag_at(index).is_resolved_virtual_method()) {
    //see CR6463588
    //TCK test:javasoft.sqe.tests.vm.instr.invokestatic.invokestatic009.invokestatic00902m1
    Throw::incompatible_class_change_error(method_changed JVM_THROW_0)
  }
  GUARANTEE(tag_at(index).is_method(), "Corrupted constant pool");

  resolve_helper(index, name, signature, static_receiver_class JVM_CHECK_0);

  Method::Fast m = static_receiver_class->lookup_method(name, signature);
  if (m.is_null()) {
    trace_no_such_method_error(sender_class, static_receiver_class, name,
                               signature);
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
  }

  static_receiver_class->check_access_by(sender_class, ErrorOnFailure JVM_CHECK_0);

  m().check_access_by(sender_class, static_receiver_class, 
                      ErrorOnFailure JVM_CHECK_0);

  return m;
}

bool ConstantPool::resolve_method_ref(int index, 
                                      InstanceClass* static_receiver_class, 
                                      Method* method JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (   static_receiver_class->is_final()
      || method->is_final()
      || method->is_static()
      || method->is_object_initializer()) {
    resolved_static_method_at_put(index, method);
    return true;
  } else {
    int class_id = static_receiver_class->class_id();
    GUARANTEE(method->vtable_index() >= 0, "must have a vtable");
    Signature::Raw signature = method->signature();
    resolved_virtual_method_at_put(index, (jushort) class_id, (jushort) method->vtable_index(),
                                   signature().return_type());
    return false;
  }
}

// return code: true  == method is static
// return code: false == method is virtual
bool
ConstantPool::resolve_invoke_special_at(InstanceClass *sender_class,
                                        int index JVM_TRAPS) {
  jubyte t = tag_value_at(index);
  if (ConstantTag::is_resolved_static_method(t)) {
    // CR 4862713/6324543, If method was resolved by some good reference but
    // this one is some hacked class file with an invokespecial opcode
    // but the index of a static method then we must check this case.
    Method::Raw m = resolved_static_method_at(index);
    GUARANTEE(!m.is_null(), "Resolved method is null");
    if (m().is_static()) {
      Throw::incompatible_class_change_error(empty_message JVM_THROW_0);
    } 
    return true;
  }
  if (ConstantTag::is_resolved_virtual_method(t)) {
    return false;
  }

  UsingFastOops fast_oops;
  Symbol::Fast method_name, method_signature;
  InstanceClass::Fast static_receiver_class;
  InstanceClass::Fast holder_class;
  Method::Fast m = lookup_method_at(sender_class, index, &method_name,
                              &method_signature, &static_receiver_class
                              JVM_CHECK_0);

  if (m().is_static()) {
    Throw::incompatible_class_change_error(method_changed JVM_THROW_0);
  }

  if (method_name.equals(Symbols::object_initializer_name())) {
    holder_class = m().holder();
    if (!static_receiver_class.equals(&holder_class)) {
      trace_no_such_method_error(sender_class, &static_receiver_class, 
                                 &method_name, &method_signature);
      Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
    }
  } else { //see CR6539744
    if (sender_class->is_super() && !sender_class->equals(static_receiver_class)) {
      if (sender_class->is_subclass_of(&static_receiver_class)) {
        InstanceClass::Raw super_class = sender_class->super();
        m = super_class().lookup_method(&method_name, &method_signature);
        if (m.is_null()) {
          Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
        }
        static_receiver_class = m().holder();        
      }      
    }
  }

  return resolve_method_ref(index, &static_receiver_class, &m 
                            JVM_NO_CHECK_AT_BOTTOM_0);
}

// return code: true == class has been initialized
bool
ConstantPool::resolve_invoke_static_at(InstanceClass *sender_class,
                                       int index, bool do_init JVM_TRAPS) {
  UsingFastOops fast_oops;

  InstanceClass::Fast receiver_class;
  Method::Fast method;
  Symbol::Fast method_name, method_signature;

  if (ConstantTag::is_resolved_static_method(tag_value_at(index))) {
    method = resolved_static_method_at(index);
  } else {
    method = lookup_method_at(sender_class, index, &method_name,
                              &method_signature, &receiver_class 
                              JVM_OZCHECK(method));
    resolved_static_method_at_put(index, &method);
  }

  if (!method().is_static()) {
    Throw::incompatible_class_change_error(method_changed JVM_THROW_0);
  }

  receiver_class = method().holder();

  if (!sender_class->is_preloaded() && receiver_class().is_hidden()) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);   
  }

  if (do_init) {
    // Initialize the receiver_class
#if ENABLE_ISOLATES
    task_barrier(Thread::current(), receiver_class.obj() JVM_CHECK_0);
#else
    receiver_class().initialize(JVM_SINGLE_ARG_CHECK_0);
#endif
  } else {
    GUARANTEE(GenerateROMImage || _class_loading_suspended,
              "This should happen only during romization or compilation");
  }

  return receiver_class().is_initialized();
}

bool ConstantPool::resolve_invoke_virtual_at(InstanceClass *sender_class,
                                             int index JVM_TRAPS) {
  jubyte t = tag_value_at(index);
  if (ConstantTag::is_resolved_virtual_method(t)) {
    return false;
  }
  if (ConstantTag::is_resolved_static_method(t)) {
    // CR 4862713/6324540, If method was resolved by some good reference but
    // this one is some hacked class file with an invokevirtual opcode
    // but the index of a static method then we must check this case.
    Method::Raw m = resolved_static_method_at(index);
    GUARANTEE(!m.is_null(), "Resolved method is null");
    if (m().is_static()) {
      Throw::incompatible_class_change_error(empty_message JVM_THROW_0);
    } 
    return true;
  }

  UsingFastOops fast_oops;

  Symbol::Fast method_name, method_signature;
  InstanceClass::Fast static_receiver_class;
  Method::Fast method = lookup_method_at(sender_class, index, &method_name,
                                   &method_signature, &static_receiver_class
                                   JVM_OZCHECK(method));

  if (static_receiver_class().is_interface()) {
    Throw::incompatible_class_change_error(class_changed JVM_THROW_0);
  }

  if (method().is_static()) {
    Throw::incompatible_class_change_error(empty_message JVM_THROW_0);
  }

  return resolve_method_ref(index, &static_receiver_class, &method 
                            JVM_NO_CHECK_AT_BOTTOM_0);
}

void ConstantPool::resolve_invoke_interface_at(InstanceClass *sender_class,
                                               int index JVM_TRAPS) {
  // Return if the entry has been resolved
  jubyte t = tag_value_at(index);
  if (ConstantTag::is_resolved_interface_method(t)) {
    return;
  }
  if (ConstantTag::is_resolved_uncommon_interface_method(t)) {
    return;
  }

  if (ConstantTag::is_resolved_static_method(t)) {
    return;
  }

  if (ConstantTag::is_resolved_virtual_method(t)) {
    return;
  }

  GUARANTEE(tag_at(index).is_interface_method(), "Corrupted constant pool");

  UsingFastOops fast_oops;
  // Get name, signature, and interface_class
  Symbol::Fast method_name;
  Signature::Fast method_signature;
  InstanceClass::Fast interface_class;
  resolve_helper(index, &method_name, &method_signature, &interface_class
                 JVM_CHECK);

  // Check class change error
  if (!interface_class().is_interface()) {
    Throw::incompatible_class_change_error(class_changed JVM_THROW);
  }

  InstanceClass::Fast current_class = sender_class;
  interface_class().check_access_by(&current_class, ErrorOnFailure JVM_CHECK);
  int interface_class_id;
  // Resolve named method
  int itable_index = 0;
  bool is_uncommon = false;
  Method::Fast named_method = 
      interface_class().lookup_method_in_all_interfaces(&method_name,
                        &method_signature, interface_class_id, itable_index);
  if (named_method.is_null()) {
    // This is actually a java.lang.Object method 
    // masquerading as an interface method. E.g.,
    //     aload_0
    //     invokeinterface SomeInterface.getClass()
    named_method = interface_class().lookup_method(&method_name, 
                                                   &method_signature);
    is_uncommon = true;
  }

  if (named_method.is_null()) {
    trace_no_such_method_error(sender_class, &interface_class, &method_name,
      &method_signature);
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW);
  }
  if (!named_method().is_public()) {
    Throw::illegal_access(ErrorOnFailure JVM_THROW);
  }

  if (is_uncommon) {
    if (named_method().is_static() || named_method().is_object_initializer()) {
      // Sorry, you're twisting the JLS too much. You can't use invokevirtual
      // to invoke a static method or initializer in java.lang.Object. 
      Throw::incompatible_class_change_error(class_changed JVM_THROW);
    }
    InstanceClass::Raw declaring_class = named_method().holder();
    bool is_final = resolve_method_ref(index, &declaring_class,
                                       &named_method JVM_CHECK);
    if (is_final) {
      tag_at_put(index, JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod);
    } else {
      tag_at_put(index, JVM_CONSTANT_ResolvedUncommonInterfaceMethod);
    }
  } else {
#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES)

    TypeArray::Raw implementation_cache = 
       ((ROMWriter::_singleton)->_optimizer).
            interface_implementation_cache()->obj();
    TypeArray::Raw direct_implementation_cache = 
       ((ROMWriter::_singleton)->_optimizer).
            direct_interface_implementation_cache()->obj();
    int implementing_class_id = -1;
    int direct_implementing_class_id = -1;
    if (GenerateROMImage) {
      implementing_class_id =implementation_cache().int_at(interface_class_id);
      direct_implementing_class_id = direct_implementation_cache().int_at(interface_class_id);
    }
    
    if( implementing_class_id >= 0) { //single implementing class
      InstanceClass::Raw implement_cls = 
        Universe::class_from_id(implementing_class_id);        
      Method::Raw implementing_method = 
        implement_cls().lookup_method(&method_name, &method_signature);  

      // Note: the method can be omitted in the class that is declared to 
      // implement this interface (tested by TCK)
      if (implementing_method.not_null() && 
          implementing_method().is_public() && 
          !implementing_method().is_static()) {
        resolved_static_method_at_put(index, &implementing_method);
        return;
      }
    } else if (direct_implementing_class_id >= 0) { //single direct implementing class
      InstanceClass::Raw implement_cls = 
        Universe::class_from_id(direct_implementing_class_id);        
      Method::Raw implementing_method = 
        implement_cls().lookup_method(&method_name, &method_signature);

      // Note: the method can be omitted in the class that is declared to 
      // implement this interface (tested by TCK)
      if (implementing_method.not_null() &&
          implementing_method().is_public() && 
          !implementing_method().is_static()) {
        resolve_method_ref(index, &implement_cls, 
                           &implementing_method JVM_CHECK);
        return;
      }
    }
#endif  
    BasicType return_type = method_signature().return_type();
    resolved_interface_method_at_put(index, (jushort) itable_index, 
                                     (jushort) interface_class_id, return_type);                                                
  }
}

int ConstantPool::klass_ref_index_at(int index JVM_TRAPS) {
  int offset = offset_from_checked_index(index JVM_ZCHECK_0(offset));
  jint ref_index = int_field(offset);
  jint class_index = extract_low_jushort_from_jint(ref_index);
  cp_check_0(check_klass_at(class_index));
  GUARANTEE(class_index != 0, "sanity for JVM_ZCHECK");
  return class_index;
}

int ConstantPool::name_ref_index_at(int index JVM_TRAPS) {
  jint ref_index = name_and_type_at(index JVM_ZCHECK_0(ref_index));
  jint name_index = extract_low_jushort_from_jint(ref_index);
  cp_check_0(is_within_bounds(name_index) &&
             ConstantTag::is_utf8(tag_value_at(name_index)));
  GUARANTEE(name_index != 0, "sanity for JVM_ZCHECK");
  return name_index;
}

int ConstantPool::signature_ref_index_at(int index JVM_TRAPS)  {
  jint ref_index = name_and_type_at(index JVM_ZCHECK_0(ref_index));
  jint signature_index = extract_high_jushort_from_jint(ref_index);
  cp_check_0(is_within_bounds(signature_index) &&
             ConstantTag::is_utf8(tag_value_at(signature_index)));
  GUARANTEE(signature_index != 0, "sanity for JVM_ZCHECK");
  return signature_index;
}

#ifndef PRODUCT

void ConstantPool::trace_no_such_method_error(InstanceClass* sender_class,
                                              InstanceClass* receiver_class,
                                              Symbol* name, Symbol* signature){
  if (GenerateROMImage) {
    Symbol sender_name = sender_class->name();
    Symbol receiver_name = receiver_class->name();

    tty->print("No such method: ");
    receiver_name.print_value_on(tty);
    tty->print(".");
    name->print_value_on(tty);
    signature->print_value_on(tty);
    tty->cr();
    tty->print("    (used in ");
    sender_name.print_value_on(tty);
    tty->print_cr(")");

    if (VerboseROMComments || 1) {
      tty->print_cr("Dump of constant pool");
      print_on(tty);
    }
  }
}

#endif

#ifndef PRODUCT

void ConstantPool::iterate(OopVisitor* visitor) {
#if USE_DEBUG_PRINTING
  Oop::iterate(visitor);

  { NamedField field("length", true);
    visitor->do_ushort(&field, length_offset(), true);
  }

  { NamedField field("tags", true);
    visitor->do_oop(&field, tags_offset(), true);
  }

  // Note: index 0 may be used during romization to rename non-public symbols.
  for (int index = 0; index < length(); index++) {
    ConstantTag tag = tag_at(index);
    switch (tag.value()) {
    case JVM_CONSTANT_Class : {
      // IMPL_NOTE: print the class name
      IndexableField field(index, true, "Class");
      visitor->do_int(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_ResolvedBooleanFieldref:
    case JVM_CONSTANT_ResolvedCharFieldref:
    case JVM_CONSTANT_ResolvedFloatFieldref:
    case JVM_CONSTANT_ResolvedDoubleFieldref:
    case JVM_CONSTANT_ResolvedByteFieldref:
    case JVM_CONSTANT_ResolvedShortFieldref:
    case JVM_CONSTANT_ResolvedIntFieldref:
    case JVM_CONSTANT_ResolvedLongFieldref:
    case JVM_CONSTANT_ResolvedObjectFieldref:
    case JVM_CONSTANT_ResolvedArrayFieldref:     
    case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
    case JVM_CONSTANT_ResolvedStaticCharFieldref:
    case JVM_CONSTANT_ResolvedStaticFloatFieldref:
    case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
    case JVM_CONSTANT_ResolvedStaticByteFieldref:
    case JVM_CONSTANT_ResolvedStaticShortFieldref:
    case JVM_CONSTANT_ResolvedStaticIntFieldref:
    case JVM_CONSTANT_ResolvedStaticLongFieldref:
    case JVM_CONSTANT_ResolvedStaticObjectFieldref:
    case JVM_CONSTANT_ResolvedStaticArrayFieldref:

    case JVM_CONSTANT_ResolvedStaticMethod:
    case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
    case JVM_CONSTANT_ResolvedCharVirtualMethod:
    case JVM_CONSTANT_ResolvedFloatVirtualMethod:
    case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
    case JVM_CONSTANT_ResolvedByteVirtualMethod:
    case JVM_CONSTANT_ResolvedShortVirtualMethod:
    case JVM_CONSTANT_ResolvedIntVirtualMethod:
    case JVM_CONSTANT_ResolvedLongVirtualMethod:
    case JVM_CONSTANT_ResolvedObjectVirtualMethod:
    case JVM_CONSTANT_ResolvedArrayVirtualMethod:
    case JVM_CONSTANT_ResolvedVoidVirtualMethod:      

    case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
    case JVM_CONSTANT_ResolvedCharInterfaceMethod:
    case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
    case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
    case JVM_CONSTANT_ResolvedByteInterfaceMethod:
    case JVM_CONSTANT_ResolvedShortInterfaceMethod:
    case JVM_CONSTANT_ResolvedIntInterfaceMethod:
    case JVM_CONSTANT_ResolvedLongInterfaceMethod:
    case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
    case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
    case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
      {
        IndexableField field(index, true, comment_text(index));
        visitor->do_int(&field, offset_from_index(index), true);
      }
      break;

    case JVM_CONSTANT_Fieldref:
    case JVM_CONSTANT_Methodref:
    case JVM_CONSTANT_InterfaceMethodref: {
      IndexableField field(index, true, comment_text(index));
      visitor->do_int(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_UnresolvedString: {
      IndexableField field(index, true, "UnresolvedString");
      visitor->do_oop(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_String: {
      IndexableField field(index, true, "String");
      visitor->do_oop(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_Integer: {
      IndexableField field(index, true, "Integer");
      visitor->do_int(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_Float: {
      IndexableField field(index, true, "Float");
      visitor->do_float(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_Long: {
      IndexableField field(index, true, "Long");
      visitor->do_long(&field, offset_from_index(index), true);
    }
      index++;   // Skip entry following eight-byte constant
      break;
    case JVM_CONSTANT_Double:{
      IndexableField field(index, true, "Double");
      visitor->do_double(&field, offset_from_index(index), true);
    }
      index++;   // Skip entry following eight-byte constant
      break;
    case JVM_CONSTANT_NameAndType: {
      IndexableField field(index, true, "NameAndType");
      visitor->do_int(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_Utf8: {
      IndexableField field(index, true, "Utf8");
      visitor->do_oop(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_UnresolvedClass: {
      IndexableField field(index, true, "UnresolvedClass");
      visitor->do_oop(&field, offset_from_index(index), true);
    }
      break;
    case JVM_CONSTANT_Invalid: {
      IndexableField field(index, true, "Invalid");
      visitor->do_int(&field, offset_from_index(index), true);
    }
      break;
    default:
      SHOULD_NOT_REACH_HERE();
      break;
    }
  }
#endif
}

void ConstantPool::iterate_oopmaps(oopmaps_doer do_map, void* param) {
  OOPMAP_ENTRY_4(do_map, param, T_INT,    length);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, tags);
}

bool ConstantPool::needs_name_and_type_entries() {
  int i;

  for (i = 0; i < length(); i++) {
    ConstantTag tag = tag_at(i);
    switch (tag.value()) {
    case JVM_CONSTANT_Fieldref:
    case JVM_CONSTANT_Methodref:
    case JVM_CONSTANT_InterfaceMethodref:
      return true;
    }
  }

  return false;
}

#endif

#if !defined(PRODUCT) || USE_DEBUG_PRINTING

void ConstantPool::print_value_on(Stream* st) {
  st->print("ConstantPool");
}

#if USE_DEBUG_PRINTING
// Note: the comment is returned in a static buffer, so its value is valid
// only up to the next call to comment_text()
char* ConstantPool::comment_text(int index) {
  SETUP_ERROR_CHECKER_ARG;
  static char buffer[1024]; // OK, don't use very very long names.
  FixedArrayOutputStream faos(buffer, sizeof(buffer));

  Oop saved_exception = Thread::current_pending_exception();
  print_entry_on(&faos, index JVM_NO_CHECK);
  Thread::clear_current_pending_exception();
  Thread::set_current_pending_exception(&saved_exception);

  return buffer;
}
#endif

void ConstantPool::print_entry_on(Stream* st, int index JVM_TRAPS) {
#if USE_DEBUG_PRINTING
  ConstantTag tag = tag_at(index);
  switch (tag.value()) {
  case JVM_CONSTANT_Class:
    print_class_on(st, index JVM_CHECK);
    break;
  case JVM_CONSTANT_ResolvedBooleanFieldref:
  case JVM_CONSTANT_ResolvedCharFieldref:
  case JVM_CONSTANT_ResolvedFloatFieldref:
  case JVM_CONSTANT_ResolvedDoubleFieldref:
  case JVM_CONSTANT_ResolvedByteFieldref:
  case JVM_CONSTANT_ResolvedShortFieldref:
  case JVM_CONSTANT_ResolvedIntFieldref:
  case JVM_CONSTANT_ResolvedLongFieldref:
  case JVM_CONSTANT_ResolvedObjectFieldref:
  case JVM_CONSTANT_ResolvedArrayFieldref:
  case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
  case JVM_CONSTANT_ResolvedStaticCharFieldref:
  case JVM_CONSTANT_ResolvedStaticFloatFieldref:
  case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
  case JVM_CONSTANT_ResolvedStaticByteFieldref:
  case JVM_CONSTANT_ResolvedStaticShortFieldref:
  case JVM_CONSTANT_ResolvedStaticIntFieldref:
  case JVM_CONSTANT_ResolvedStaticLongFieldref:
  case JVM_CONSTANT_ResolvedStaticObjectFieldref:
  case JVM_CONSTANT_ResolvedStaticArrayFieldref:
    print_field_ref_on(st, index JVM_CHECK);
    break;  
  case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
  case JVM_CONSTANT_ResolvedCharVirtualMethod:
  case JVM_CONSTANT_ResolvedFloatVirtualMethod:
  case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
  case JVM_CONSTANT_ResolvedByteVirtualMethod:
  case JVM_CONSTANT_ResolvedShortVirtualMethod:
  case JVM_CONSTANT_ResolvedIntVirtualMethod:
  case JVM_CONSTANT_ResolvedLongVirtualMethod:
  case JVM_CONSTANT_ResolvedObjectVirtualMethod:
  case JVM_CONSTANT_ResolvedArrayVirtualMethod:
  case JVM_CONSTANT_ResolvedVoidVirtualMethod:
    print_virtual_method_ref_on(st, index JVM_CHECK);
    break;
  case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
  case JVM_CONSTANT_ResolvedCharInterfaceMethod:
  case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
  case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
  case JVM_CONSTANT_ResolvedByteInterfaceMethod:
  case JVM_CONSTANT_ResolvedShortInterfaceMethod:
  case JVM_CONSTANT_ResolvedIntInterfaceMethod:
  case JVM_CONSTANT_ResolvedLongInterfaceMethod:
  case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
  case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
  case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
    print_interface_method_ref_on(st, index JVM_CHECK);
    break;
  case JVM_CONSTANT_Integer:
    st->print("Integer: %d", int_at(index));
    break;
  case JVM_CONSTANT_Float:
    //st->print("Float: %f", float_at(index)); IMPL_NOTE: %f not portable
    break;
  case JVM_CONSTANT_Fieldref:
  case JVM_CONSTANT_Methodref:
  case JVM_CONSTANT_InterfaceMethodref:
    print_unresolved_ref_on(st, index JVM_CHECK);
    break;
  case JVM_CONSTANT_ResolvedStaticMethod:
    st->print("%s", ConstantTag::name_for(tag.value(), false)); // IMPL_NOTE 
    break;
  default:
    st->print("%s", ConstantTag::name_for(tag.value(), false));
    break;
  }
#endif
}

void ConstantPool::print_class_on(Stream* st, int index JVM_TRAPS) {
#if USE_DEBUG_PRINTING
  if (tag_at(index).is_resolved_klass()) {
    st->print("ResolvedClass: ");
    JavaClass klass = klass_at(index JVM_CHECK);
    klass.print_name_on(st);
  } else {
    st->print("Class: ");
    // IMPL_NOTE: get its name from UTF8???
    st->print("<unresolved>");
  }
#endif
}

void ConstantPool::print_unresolved_ref_on(Stream* st, int index JVM_TRAPS) {
#if USE_DEBUG_PRINTING
  UsingFastOops fast_oops;
  Symbol::Fast name;
  Signature::Fast sig;
  InstanceClass::Fast klass;
  resolve_helper(index, &name, &sig, &klass JVM_CHECK);

  if (!GenerateCompilerComments || Verbose) {
    st->print("(Unresolved) %s: ", ConstantTag::name_for(tag_value_at(index),
                                                         false));
  }
  klass().print_name_on(st);
  st->print(".");
  name().print_symbol_on(st);
  st->print(":");
  if (sig().is_valid_method_signature(NULL) || sig().is_valid_field_type()) {
    sig().print_decoded_on(st);
  } else {
    sig().print_symbol_on(st);
  }
#endif
}

void ConstantPool::print_field_ref_on(Stream* st, int index JVM_TRAPS) {  
#if USE_DEBUG_PRINTING
  if (tag_at(index).is_resolved_field()) {
    st->print("Resolved FieldRef: ");
    // Note: Assume resolved fields are always static -- after CP rewriting,
    // we have no instance fields refs in the CP anymore.
    int class_id, offset;
    resolved_field_type_at(index, offset, class_id);
    InstanceClass::Raw klass = Universe::class_from_id(class_id);
    klass().print_name_on(st);
    print_verbose_field_on(st, &klass, offset, 
                           tag_at(index).is_resolved_static_field());
    st->print(" (offset=%d)", offset);
  } else {
    print_unresolved_ref_on(st, index JVM_CHECK);
  }
#endif
}

void ConstantPool::print_verbose_field_on(Stream* st, InstanceClass *klass, 
                                          int offset, bool is_static) {
#if USE_DEBUG_PRINTING
  Symbol::Raw name;
  InstanceClass::Raw ic = klass->obj();
  while (ic.not_null()) {
    TypeArray::Raw f = ic().original_fields();
    for (int index = 0; index < f().length(); index += 5) {
      OriginalField field(&ic, index);
      if (field.is_static() == is_static && field.offset() == offset) {
        st->print(".");
        name = field.name();
        name().print_symbol_on(st);
        return;
      }
    }
    // Could this be in super class?
    ic = ic().super();
  }
  st->print(".????");
#endif
}

void ConstantPool::print_virtual_method_ref_on(Stream* st, int index JVM_TRAPS)
{
#if USE_DEBUG_PRINTING
  if (tag_at(index).is_resolved_virtual_method()) {
    st->print("Resolved VirtualMethodRef: ");

    int class_id, vtable_index;
    resolved_virtual_method_at(index, vtable_index, class_id);
    InstanceClass::Raw klass = Universe::class_from_id(class_id);
    ClassInfo::Raw info = klass().class_info();
    Method::Raw m = info().vtable_method_at(vtable_index);
    if (m.not_null()) {
      m().print_name_on(st);
    }
  } else {
    print_unresolved_ref_on(st, index JVM_CHECK);
  }
#endif
}

void ConstantPool::print_interface_method_ref_on(Stream* st, int index 
                                                 JVM_TRAPS) {
#if USE_DEBUG_PRINTING
  if (tag_at(index).is_resolved_interface_method()) {
    st->print("Resolved InterfaceMethodRef: ");

    int class_id, itable_index;
    resolved_interface_method_at(index, itable_index, class_id);
    InstanceClass::Raw klass = Universe::class_from_id(class_id);
    Method::Raw m = klass().interface_method_at(itable_index);
    if (m.not_null()) {
      m().print_name_on(st);
    }
  } else {
    print_unresolved_ref_on(st, index JVM_CHECK);
  }
#endif
}


#endif /* #if !defined(PRODUCT) || USE_DEBUG_PRINTING */

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int ConstantPool::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;
  int max_length = length() * 2 + 4;
  
  // Make sure the field map has enough space
  if (field_map->length() < max_length) {
    // In the worst case every entry is a short pair.
    return max_length;
  }
  
  // Generic object near
  map_index = Near::generate_fieldmap(field_map);

  // _length + pad;
  field_map->byte_at_put(map_index++, T_SHORT);
  field_map->byte_at_put(map_index++, T_SHORT);
  // _tags;   
  field_map->byte_at_put(map_index++, T_OBJECT);

  // Make sure the assumption is correct
  GUARANTEE((sizeof(OopDesc)/BytesPerWord + 2) * sizeof(int) == sizeof(ConstantPoolDesc), 
            "Mismatch between field map and ConstantPoolDesc definition");
  
  // map the constant pool tags
  for (int i = 0; i < length(); ) {
    if (tag_at(i).is_oop()) {
      field_map->byte_at_put(map_index++, T_OBJECT);
      i++;
    }
    else if (tag_at(i).is_resolved_klass()) {
      field_map->byte_at_put(map_index++, T_INT);
      i++;
    }
    else if (tag_at(i).is_double()) {
      field_map->byte_at_put(map_index++, T_DOUBLE);
      i+= 2;
    }
    else if (tag_at(i).is_long()) {
      field_map->byte_at_put(map_index++, T_LONG);
      i+= 2;
    }
    else {
      field_map->byte_at_put(map_index++, T_INT);
      i++;
    }
  }

  return map_index;
}

#endif /* #if ENABLE_ROM_GENERATOR */

void ConstantPool::check_quickened_field_access(int index, 
                                                InstanceClass* sender_class, 
                                                bool is_static, bool is_get
                                                JVM_TRAPS)
{

  juint value = value32_at(index);
  jint offset   = extract_high_jushort_from_jint(value);
  jint class_id = extract_low_jushort_from_jint(value);

  InstanceClass::Raw klass = Universe::class_from_id(class_id);
  TypeArray::Raw fields = klass().fields();
  AccessFlags af;
  af.set_flags(0);

  jint flags = -1;

  // find proper field in field list of the class
  int len = fields().length();
  for (int i=0; i < len; i += Field::NUMBER_OF_SLOTS) {
    if (fields().ushort_at(i + Field::OFFSET_OFFSET) == offset) {
      flags = fields().ushort_at(i + Field::ACCESS_FLAGS_OFFSET);
      af.set_flags(flags);
      if (af.is_static() == is_static) {
        break;
      }
      flags = -1;
    }
  }

  // field not found or staticness mismatch
  if (flags == -1) {
    if (klass().is_optimizable()) {
#if ENABLE_MONET
      if (klass().is_converted()) {
        // For a converted application class the field can removed by
        // CompactROMFieldTables optimization, so we consider the check is
        // passed if we don't find the field and the class is marked as
        // converted and optimizable.
        return;
      }
#endif

      // Mark the class as non-optimizable to disable CompactROMFieldTables
      // optimization for this class. This allows to detect this failure at
      // run-time.
      if (GenerateROMImage) {
        klass().set_is_non_optimizable();
      }
    }

    Throw::incompatible_class_change_error(field_changed JVM_THROW);
  }

  // check for final access
  if (!is_get && af.is_final() && !sender_class->equals(&klass)) {
    // Mark the class as non-optimizable to disable CompactROMFieldTables
    // optimization for this class. This allows to detect this failure at
    // run-time.
    if (GenerateROMImage) {
      klass().set_is_non_optimizable();
    }

    Throw::illegal_access(ErrorOnFailure JVM_THROW);
  }
}
