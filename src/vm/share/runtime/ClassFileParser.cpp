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

#include "incls/_precompiled.incl"
#include "incls/_ClassFileParser.cpp.incl"

#define HAS_BIT(bitmask, index) ((bitmask & (1 << index)) != 0)
#define SET_BIT(bitmask, index) bitmask |= (1 << index)

ClassFileParser* ClassFileParser::_head = NULL;

#if ENABLE_ROM_GENERATOR
int ClassFileParser::_total_classfile_bytes = 0;
int ClassFileParser::_total_bytecode_bytes  = 0;
int ClassFileParser::_total_stackmap_bytes  = 0;
#endif

#ifndef PRODUCT
inline void classfile_parse_error(ErrorMsgTag err JVM_TRAPS) {
  Throw::class_format_error(err JVM_NO_CHECK_AT_BOTTOM);
}
#else
void classfile_parse_error() {
  Throw::class_format_error(invalid_class_file);
}
inline void classfile_parse_error(ErrorMsgTag /*ignored*/) {
  classfile_parse_error();
}
#endif

inline void truncated_class_file_error(JVM_SINGLE_ARG_TRAPS) {
  classfile_parse_error(truncated_class_file JVM_NO_CHECK_AT_BOTTOM);
}

jubyte ClassFileParser::get_u1(JVM_SINGLE_ARG_TRAPS) {
  jubyte *ptr = _bufptr;
  jubyte *end = _bufend;
  jubyte b = *ptr++;

  _bufptr = ptr;
  if (ptr <= end) {
    return b;
  } else {
    truncated_class_file_error(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return 0;
  }
}

juint ClassFileParser::get_u4(JVM_SINGLE_ARG_TRAPS) {
  juint val = Bytes::get_Java_u4((address)_bufptr);
  _bufptr += 4;
  if (_bufptr > _bufend) {
    truncated_class_file_error(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return 0;
  } else {
    return val;
  }
}

void ClassFileParser::skip_u1(int length JVM_TRAPS) {
  _bufptr += length;
  if (_bufptr > _bufend) {
    truncated_class_file_error(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void ClassFileParser::gc_prologue() {
  for (ClassFileParser* cfp = _head; cfp != NULL; cfp = cfp->previous()) {
    // change from direct pointer to offset inside _buffer
    jubyte *base = (jubyte*)(cfp->_buffer->base_address());
    cfp->_bufptr = (jubyte *)(cfp->_bufptr - base);
    cfp->_bufend = (jubyte *)(cfp->_bufend - base);
  }
}
void ClassFileParser::gc_epilogue() {
  for (ClassFileParser* cfp = _head; cfp != NULL; cfp = cfp->previous()) {
    // change from offset back to direct pointer to _buffer
    jubyte *base = (jubyte*)(cfp->_buffer->base_address());
    cfp->_bufptr = base + (int)(cfp->_bufptr);
    cfp->_bufend = base + (int)(cfp->_bufend);
  }
}

void ClassFileParser::set_buffer_position(int pos) {
  _bufptr = (jubyte*)_buffer->base_address();
  _bufend = _bufptr + _buffer->length();
  _bufptr += pos;
}

// Verification of class file properties:

#define cpf_check(b, err) \
  if (!(b)) { \
    classfile_parse_error((err) JVM_NO_CHECK_AT_BOTTOM); \
    return; \
  }

#define cpf_check_0(b, err) \
  if (!(b)) { \
    classfile_parse_error((err) JVM_NO_CHECK_AT_BOTTOM); \
    return 0; \
  }

// Does the current buffer contain a valid UTF8 string of the given length
inline bool ClassFileParser::is_valid_utf8_in_buffer(jint utf8_length) {
  const jbyte* p = (jbyte*)_bufptr;
  const jbyte* const e = p + utf8_length;

  if (e > (jbyte*)_bufend) {
    return false;
  }

  while( p < e ) {
    if (*p++ <= 0) {
      // This UTF8 string contains an escape sequence (which is very rare),
      // so let's do it the more proper way.
      ByteStream bs(_buffer, get_buffer_position(), utf8_length);
      return bs.is_valid();
    }
  }
  return true;
}

// Constant pool parsing:
inline
void ClassFileParser::parse_constant_pool_utf8_entry(ConstantPool* cp,
                                                     int index JVM_TRAPS) {
  jint utf8_length = (jint)get_u2(JVM_SINGLE_ARG_CHECK);
  cpf_check(is_valid_utf8_in_buffer(utf8_length), invalid_utf8_string);

  // OK to use new_byte_array_raw(): its entire content will be filled.
  TypeArray::Raw bytearray = 
      Universe::new_byte_array_raw(utf8_length JVM_NO_CHECK);
  if (bytearray.not_null()) {
    TypeArray::array_copy(_buffer, get_buffer_position(),
                          &bytearray, 0, utf8_length);
    _bufptr += utf8_length;
    GUARANTEE(_bufptr <= _bufend, 
              "we have already checked that buffer has available data");
    cp->symbol_at_put(index, &bytearray);
  }
}

inline void
ClassFileParser::parse_constant_pool_entries(ConstantPool* cp JVM_TRAPS) {
  int index = 1;
  int len = cp->length();
  for (index = 1; index < len; index++) {
    jubyte t = get_u1(JVM_SINGLE_ARG_CHECK);
    cpf_check(ConstantTag::is_valid_tag(t), invalid_constant_tag);
    switch (t) {
      case JVM_CONSTANT_Class :
        {
          int name_index = get_u2(JVM_SINGLE_ARG_NO_CHECK);
          cp->klass_index_at_put(index, name_index);
        }
        break;
      case JVM_CONSTANT_String :
        {
          int string_index = get_u2(JVM_SINGLE_ARG_NO_CHECK);
          cpf_check(string_index > 0 && string_index < cp->length(),
                    bad_constantpool_index);
          cp->string_index_at_put(index, string_index);
        }
        break;
      case JVM_CONSTANT_Integer :
        {
          juint bytes = get_u4(JVM_SINGLE_ARG_NO_CHECK);
          cp->int_at_put(index, (jint) bytes);
        }
        break;
      case JVM_CONSTANT_Float :
#if ENABLE_FLOAT
        {
          juint bytes = get_u4(JVM_SINGLE_ARG_CHECK);
          cp->float_at_put(index, *(jfloat*)&bytes);
        }
        break;
#else
        classfile_parse_error(invalid_constant_tag JVM_NO_CHECK_AT_BOTTOM);
        return;
#endif
      case JVM_CONSTANT_Long :
        if (index + 1 == cp->length()) {
          classfile_parse_error(constantpool_overflow JVM_NO_CHECK_AT_BOTTOM);
          return;
        } else {
          juint msw = get_u4(JVM_SINGLE_ARG_CHECK);
          juint lsw = get_u4(JVM_SINGLE_ARG_NO_CHECK);
          cp->long_parts_at_put(index, msw, lsw);
          index++;
        }
        break;
      case JVM_CONSTANT_Double :
#if ENABLE_FLOAT
        if (index + 1 == cp->length()) {
          classfile_parse_error(constantpool_overflow JVM_THROW);
        } else {
          juint msw = get_u4(JVM_SINGLE_ARG_CHECK);
          juint lsw = get_u4(JVM_SINGLE_ARG_NO_CHECK);
          cp->double_parts_at_put(index, msw, lsw);
          // Skip entry following eight-byte constant, see JVM book p. 98
          index++;
        }
        break;
#else
        classfile_parse_error(invalid_constant_tag JVM_THROW);
#endif
      case JVM_CONSTANT_Fieldref:
      case JVM_CONSTANT_Methodref:
      case JVM_CONSTANT_InterfaceMethodref:
      case JVM_CONSTANT_NameAndType:
        {
          // All of these 4 types are handled exactly the same way, so
          // let's merged them into a single case.
          jushort index1 = get_u2(JVM_SINGLE_ARG_CHECK);
          jushort index2 = get_u2(JVM_SINGLE_ARG_NO_CHECK);

          // Note the order of index2 and index1. See definitions of 
          // ConstantPool::field_at_put, method_at_put, interface_method_at_put
          // and name_and_type_at_put.
          cp->tag_at_put(index, t);
          cp->int_field_put(cp->offset_from_index(index), 
              construct_jint_from_jushorts(index2, index1));
        }
        break;
      case JVM_CONSTANT_Utf8 :
        parse_constant_pool_utf8_entry(cp, index JVM_NO_CHECK);
        break;
      default:
        classfile_parse_error(unknown_constant_tag JVM_NO_CHECK_AT_BOTTOM);
        return;
    } // switch(t)
    // JVM_NO_CHECK is used instead of JVM_CHECK in the above switch statement.
    // For that reason an explicit exception check is needed here.
    JVM_DELAYED_CHECK;
  }
}

// Validate cross references and fixup class and string constants
inline void
ClassFileParser::validate_and_fixup_constant_pool_entries(ConstantPool* cp
                                                          JVM_TRAPS) {
  int index;
  int length = cp->length();

  // [Pass #1] - parse only the JVM_CONSTANT_StringIndex entries. We must
  // do this before the UTF8 entries are modified by 
  // cp->checked_type_symbol_at(), etc.
  for (index = 1; index < length; index++) {
    jubyte tag = cp->tag_value_at(index);
    switch (tag) {
    case JVM_CONSTANT_String:
    case JVM_CONSTANT_UnresolvedString :
      // Only JVM_CONSTANT_StringIndex should be present at this point
      SHOULD_NOT_REACH_HERE();
      break;
    case JVM_CONSTANT_StringIndex:
      {
        if (!ROMIZED_PRODUCT && Universe::before_main()) {
          // Can't resolve String yet, as the String class is not yet loaded
          int string_index = cp->string_index_at(index);
          Symbol::Raw sym = cp->checked_symbol_at(string_index JVM_CHECK);
          cp->unresolved_string_at_put(index, &sym);
        } else {
          cp->resolve_string_at(index JVM_CHECK);
        }
      }
      break;
    }
  }

  // [Pass #2] - parse everything other than String.
  for (index = 1; index < length; index++) {
    jubyte tag = cp->tag_value_at(index);
    switch (tag) {
      case JVM_CONSTANT_Class:
        // Only JVM_CONSTANT_ClassIndex should be present
        SHOULD_NOT_REACH_HERE();
        break;
      case JVM_CONSTANT_Fieldref:
        // Fall through
      case JVM_CONSTANT_Methodref:
        // Fall through
      case JVM_CONSTANT_InterfaceMethodref:
        {
          cp->klass_ref_index_at(index JVM_CHECK);
          int name_type_index = cp->name_and_type_ref_index_at(index JVM_CHECK);
          int type_index = cp->signature_ref_index_at(name_type_index JVM_CHECK);
          Symbol::Raw signature = cp->checked_type_symbol_at(type_index JVM_CHECK);
          if (tag == JVM_CONSTANT_Fieldref) {
            cpf_check(signature().is_valid_field_type(), bad_type_reference);
          } else {
            cpf_check(signature().is_valid_method_signature(), bad_type_reference);
          }
        }
        break;
      case JVM_CONSTANT_String:
      case JVM_CONSTANT_UnresolvedString:
      case JVM_CONSTANT_StringIndex:
        // Already taken care of in the loop above
        break;
      case JVM_CONSTANT_Integer:
        break;
      case JVM_CONSTANT_Float:
        break;
      case JVM_CONSTANT_Long:
      case JVM_CONSTANT_Double:
        index++;
        break;
      case JVM_CONSTANT_NameAndType:
        {
          UsingFastOops fast_oops;
          int name_index = cp->name_ref_index_at(index JVM_CHECK);
          Symbol::Fast name = cp->checked_symbol_at(name_index JVM_CHECK);
          int signature_index = cp->signature_ref_index_at(index JVM_CHECK);
          Symbol::Fast signature= cp->checked_type_symbol_at(signature_index
                                                             JVM_CHECK);
          if (signature().is_valid_method_signature(&name)) {
            if (name().byte_at(0) == '<') {
              // Only <init> can be in constant pool
              cpf_check(name.equals(Symbols::object_initializer_name()),
                        invalid_field_name);
            }
            cpf_check(name().is_valid_method_name(), invalid_method_name);
          } else if (signature().is_valid_field_type()) {
            cpf_check(name().is_valid_field_name(), invalid_field_name);
          } else {
            classfile_parse_error(invalid_signature JVM_THROW);
          }
        }
        break;
      case JVM_CONSTANT_Utf8:
        break;
      case JVM_CONSTANT_UnresolvedClass :
        // Only JVM_CONSTANT_ClassIndex should be present
        SHOULD_NOT_REACH_HERE();
        break;
      case JVM_CONSTANT_ClassIndex:
        {
          int class_index = cp->klass_index_at(index JVM_CHECK);
          Symbol::Raw sym = cp->checked_class_name_at(class_index JVM_CHECK);
          cpf_check(sym().is_valid_class_name(), invalid_class_name);
          cp->unresolved_klass_at_put(index, &sym);
        }
        break;
      default:
        AZZERT_ONLY(tty->print_cr("Bad constant pool tag value %d", 
                                  cp->tag_value_at(index));)
        SHOULD_NOT_REACH_HERE();
        break;
    }
  }
}

inline void ClassFileParser::remove_unused_utf8_entries(ConstantPool* cp) {
  AllocationDisabler raw_pointers_used_in_this_function;

  TypeArray::Raw tags = cp->tags();
  OopDesc **entryptr = (OopDesc**)cp->base_address();
  jubyte *tagptr = (jubyte*)tags().base_address();
  jubyte *tagend = tagptr + cp->length();

  // skip entry 0, which is always Invalid
  while (++tagptr < tagend) {
    entryptr++;
    const jubyte tag = *tagptr;
    if (tag == JVM_CONSTANT_Utf8) {
      OopDesc *oop = *entryptr;
      if (ObjectHeap::contains(oop) && oop->is_byte_array()) {
        // This UTF8 entry was used only to create a constant String
        // object. We have now created all constant Strings in this
        // pool so we can safely remove this UTF8 entry.

        // Make sure we clear the write barrier bit to avoid GC problems
        AZZERT_ONLY(ObjectHeap::clear_bit_for(entryptr));
        AZZERT_ONLY(*entryptr = (OopDesc*)0xdeadbeef);

        *tagptr = (jubyte)JVM_CONSTANT_Invalid;
      } else {
        GUARANTEE(oop->is_symbol(), "only other possible value");
      }
    }
  }
}

inline ReturnOop ClassFileParser::parse_constant_pool(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  const int length = get_u2(JVM_SINGLE_ARG_CHECK_0);
  ConstantPool::Fast cp = Universe::new_constant_pool(length JVM_CHECK_0);
  parse_constant_pool_entries(&cp JVM_CHECK_0);
  validate_and_fixup_constant_pool_entries(&cp JVM_CHECK_0);

  return cp;
}

inline
ReturnOop ClassFileParser::parse_interface_indices(ClassParserState *stack, 
                                                   ConstantPool* cp, 
                                                   bool& resolved JVM_TRAPS) {
  resolved = true;
  const int length = get_u2(JVM_SINGLE_ARG_CHECK_0);
  if (length == 0) {    
    return Universe::empty_short_array()->obj();
  }

  UsingFastOops fast_oops;
  Symbol::Fast interface_name;
  InstanceClass::Fast interf;
  // OK to use RAW array: If an error happens inside this function, the array
  // is discarded.
  TypeArray::Fast interface_indices =
      Universe::new_short_array_raw(length JVM_CHECK_0);

  for (int index = 0; index < length; index++) {
    jushort interface_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
    interface_indices().ushort_at_put(index, interface_index);

    interface_name = cp->unresolved_klass_at(interface_index JVM_CHECK_0);
    cpf_check_0(!FieldType::is_array(&interface_name), 
                class_implements_array_class);

    LoaderContext interface_ctx(&interface_name, ErrorOnFailure, _loader_ctx);
    interf = SystemDictionary::find(&interface_ctx, /*lookup_only=*/false,
                                    /*check_only=*/ true JVM_CHECK_0);
    if (interf.is_null()) {
      stack->push(&interface_name JVM_CHECK_0);
      resolved = false;
    }
  }
  return interface_indices;
}

inline
ReturnOop ClassFileParser::parse_interfaces(ConstantPool* cp,
                                            TypeArray* interface_indices
                                            JVM_TRAPS) {
  const int length = interface_indices->length();
  if (length == 0) {
    return Universe::empty_short_array()->obj();
  }

  UsingFastOops fast_oops;
  // OK to use RAW array: If an error happens inside this function, the array
  // is discarded.
  TypeArray::Fast interfaces =
      Universe::new_short_array_raw(length JVM_CHECK_0);
  Symbol::Fast interface_name;
  InstanceClass::Fast interf;
  for (int index = 0; index < length; index++) {
    jushort interface_index = (jushort)interface_indices->short_at(index);
    interface_name = cp->unresolved_klass_at(interface_index JVM_CHECK_0);

    LoaderContext interface_ctx(&interface_name, ErrorOnFailure, _loader_ctx);
    interf = SystemDictionary::find(&interface_ctx, /*lookup_only=*/ false,
                                    /*check_only=*/ false
                                    JVM_CHECK_0);
    if (!interf().is_interface()) {
      Throw::throw_exception(Symbols::java_lang_IncompatibleClassChangeError()
                             JVM_THROW_0);
    }
    interfaces().ushort_at_put(index, interf().class_id());
  }
  return interfaces;
}

// Parse attributes for a field.
// The returned value in constantvalue_index_addr is the constant pool
// index of the initial field value.
// This can be a constant integer, long, float, double or string. 0
// means no initial value for the field.
inline void
ClassFileParser::parse_field_attributes(ConstantPool* cp, 
                                        jushort& constantvalue_index_addr,
                                        FieldType* type, 
                                        bool& is_synthetic_addr JVM_TRAPS) {
  int attributes_count = get_u2(JVM_SINGLE_ARG_CHECK);
  jushort constantvalue_index = 0;
  bool is_synthetic = false;
  while (attributes_count--) {
    jushort attribute_name_index = get_u2(JVM_SINGLE_ARG_CHECK);
    juint attribute_length = get_u4(JVM_SINGLE_ARG_CHECK);
    Symbol::Raw attribute_name = cp->checked_symbol_at(attribute_name_index
                                                       JVM_CHECK);
    if (attribute_name.equals(Symbols::tag_constant_value())) {
      cpf_check((constantvalue_index == 0) &&
                (attribute_length == 2), invalid_attribute);
      constantvalue_index = get_u2(JVM_SINGLE_ARG_CHECK);
      cpf_check((constantvalue_index != 0) &&
                (constantvalue_index < cp->length()), invalid_attribute);
      cp->check_constant_at(constantvalue_index, type JVM_CHECK);
    } else if (attribute_name.equals(Symbols::tag_synthetic())) {
      cpf_check(attribute_length == 0, invalid_synthetic_attribute);
      is_synthetic = true;
    } else {
      // Skip unknown attributes.
      skip_u1(attribute_length JVM_CHECK);
    }
  }
  constantvalue_index_addr = constantvalue_index;
  is_synthetic_addr = is_synthetic;
}

bool
ClassFileParser::are_valid_field_access_flags(const AccessFlags field_access_flags,
                                              const AccessFlags class_access_flags) {
  if (class_access_flags.is_interface()) {
    const int flag = field_access_flags.as_int();
    if (flag != (JVM_ACC_PUBLIC | JVM_ACC_FINAL | JVM_ACC_STATIC)) {
      return false;
    }
  } else {
    const int subset = field_access_flags.as_int() &
                       (JVM_ACC_PUBLIC | JVM_ACC_PRIVATE | JVM_ACC_PROTECTED);
    switch (subset) { // Must be at most one of the following:
      case JVM_ACC_PUBLIC:
      case JVM_ACC_PRIVATE:
      case JVM_ACC_PROTECTED:
      case 0:
        if (field_access_flags.is_final() && field_access_flags.is_volatile()) {
          // A field can't be both final and volatile
      default:
          return false;
        }
    }
  }
  return true;
}

// Fields must not have the same name AND type
inline void ClassFileParser::check_for_duplicate_fields(ConstantPool* cp, 
                                          const TypeArray& fields JVM_TRAPS) {
  {
    // This is a hot loop, so we're using raw pointers here to help C++
    // compiler generate better code. If a GC happens we're
    // in deep trouble!

    AllocationDisabler raw_pointers_used_in_this_block;
    const int imax = fields.length() - Field::NUMBER_OF_SLOTS;
    const int jmax = fields.length();
    const int step = Field::NUMBER_OF_SLOTS;
    const jushort* const field_base = (jushort*)fields.base_address();
    const OopDesc* const* cp_base = (const OopDesc* const*)cp->base_address();

    //
    // Note: this algorithm has quadratic complexity
    //
    for (int i = 0; i < imax; i += step) {
      const jushort* ibase = field_base + i;
      const OopDesc* name_i = cp_base[ibase[Field::NAME_OFFSET]];

      for (int j = i + step; j < jmax; j += step) {
        const jushort* jbase = field_base + j;
        const OopDesc* name_j = cp_base[jbase[Field::NAME_OFFSET]];

        if (name_i == name_j) {
          const OopDesc* type_i = cp_base[ibase[Field::SIGNATURE_OFFSET]];
          const OopDesc* type_j = cp_base[jbase[Field::SIGNATURE_OFFSET]];

          if (type_i == type_j) {
            goto error;
          }
        }
      }
    }
    return;
  }

error:
  classfile_parse_error(duplicate_field JVM_NO_CHECK_AT_BOTTOM);
}

inline
ReturnOop ClassFileParser::parse_fields(ConstantPool* cp,
                                        int& nonstatic_field_size,
                                        int& static_field_size,
                                        AccessFlags& class_access_flags
                                        JVM_TRAPS) {
  const jushort length = get_u2(JVM_SINGLE_ARG_CHECK_0);
  if (length == 0) {
    return Universe::empty_short_array()->obj();
  }

  UsingFastOops fast_oops;
  // 5-tuples of shorts [access, name index, sig index, initial value index,
  // offset]
  TypeArray::Fast fields =
      Universe::new_short_array(length * Field::NUMBER_OF_SLOTS JVM_CHECK_0);
  Symbol::Fast name;
  FieldType::Fast type;
  
  int field_index = 0;
  for (int n = 0; n < length; n++) {
    AccessFlags access_flags;
    jushort flags = get_u2(JVM_SINGLE_ARG_CHECK_0);
    access_flags.set_flags(flags & JVM_RECOGNIZED_FIELD_MODIFIERS);
    cpf_check_0(are_valid_field_access_flags(access_flags, class_access_flags),
            invalid_field_access_flags);

    jushort name_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
    jushort type_index = get_u2(JVM_SINGLE_ARG_CHECK_0);

    name= cp->checked_symbol_at(name_index JVM_CHECK_0);
    type= cp->checked_type_symbol_at(type_index JVM_CHECK_0);

    cpf_check_0(name().is_valid_field_name(), invalid_field_name);
    cpf_check_0(type().is_valid_field_type(), invalid_field_type);

#ifndef ENABLE_FLOAT
    // Don't allow float fields (would confuse quickening of
    // fast_getfield)
    cpf_check_0((type().basic_type() != T_DOUBLE &&
                 type().basic_type() != T_FLOAT), invalid_field_type);
#endif

    // Parse field attributes
    jushort constantvalue_index = 0;
    {
      bool is_synthetic = false;
      parse_field_attributes(cp, constantvalue_index, &type, is_synthetic
                             JVM_CHECK_0);
      if (is_synthetic) {
        access_flags.set_is_synthetic();
      }
    }
    // Compute field sizes, offsets will be computed later
    {
      int field_size = type().allocation_byte_size(access_flags.is_static());
      if (access_flags.is_static()) {
        static_field_size    += field_size;
      } else {
        nonstatic_field_size += field_size;
      }
    }
    {
      AllocationDisabler raw_pointers_used_in_this_block;

      jushort *base = fields().ushort_base_address();
      base += field_index;
      base[Field::ACCESS_FLAGS_OFFSET] = (jushort)access_flags.as_short();
      base[Field::NAME_OFFSET]         = name_index;
      base[Field::SIGNATURE_OFFSET]    = type_index;
      base[Field::INITVAL_OFFSET]      = constantvalue_index;
      // The value at Field::OFFSET_OFFSET will be filled in later
    }

    field_index += Field::NUMBER_OF_SLOTS;
  }
  // Align per-subclass nonstatic field size to words
  nonstatic_field_size = align_size_up(nonstatic_field_size, wordSize);

  if (get_UseVerifier()) {
    check_for_duplicate_fields(cp, fields() JVM_CHECK_0);
  }
  return fields;
}

inline bool ClassFileParser::are_valid_method_access_flags(
                                        ClassParserState *state, Symbol *name, 
                                        const AccessFlags method_access_flags,
                                        const AccessFlags class_access_flags) {
  const int flags = method_access_flags.as_int();
  {
    int flag = flags & (JVM_ACC_PUBLIC | JVM_ACC_PRIVATE | JVM_ACC_PROTECTED);
    switch (flag) { // Must have one of these 4 legal values:
      case 0: case JVM_ACC_PUBLIC: case JVM_ACC_PRIVATE: case JVM_ACC_PROTECTED:
        break;
      default:
        return false;
    }
  }
  if (class_access_flags.is_interface()) {
    if ((flags & (JVM_ACC_ABSTRACT | JVM_ACC_PUBLIC | JVM_ACC_STATIC)) 
        != (JVM_ACC_ABSTRACT | JVM_ACC_PUBLIC)) {
        return false;
    }
  } else {
    if (method_access_flags.is_abstract()) {
      //This check has been to pass CLDC TCK 1.1 test
      //javasoft.sqe.tests.vm.constantpool.resolveMethod.resolveMethod005.
      //resolveMethod00501m1.resolveMethod00501
      //
      //if (!class_access_flags.is_abstract()) {
      //  //abstract methods must be declared in an abstract class, 
      //  //   JVMS §2.10.3
      //  return false;
      //}
      if( flags & JVM_ACC_STRICT ) {
        // The class is built with JDK 1.1 and this combination
        // is deemed OK. See CR 6363911
        if (state->major_version() != 45 || state->minor_version() != 3) {
          return false;
        }
      }
      if( flags & (JVM_ACC_FINAL | JVM_ACC_NATIVE | JVM_ACC_SYNCHRONIZED |
                   JVM_ACC_PRIVATE | JVM_ACC_STATIC) ) {
        return false;
      }
    }
  }
  if (name->equals(Symbols::object_initializer_name())) {
    int other_flags = flags & ~(JVM_ACC_PUBLIC | JVM_ACC_PROTECTED | JVM_ACC_PRIVATE);
    if (other_flags != 0 && other_flags != JVM_ACC_STRICT) {
      return false;
    }
  }
  return true;
}

// Parse stackmaps that are used by the verifier later
juint ClassFileParser::parse_stackmaps(ConstantPool *cp, jushort num_stackmaps,
                                       jushort max_stack, jushort frame_size,
                                       juint code_length, ObjArray* stackmaps 
                                       JVM_TRAPS) {
  UsingFastOops fast_oops;
  TypeArray::Fast stackmap_scalars;
  ObjArray::Fast stackmap_classes;

  int temp_size = max_stack + frame_size + 4;
  int startpos = _bufptr - _buffer->ubyte_base_address();
  int stackmap_index = 0;
  int max_stackmaps = 2 * num_stackmaps;

  while (stackmap_index < max_stackmaps) {
    // Allocate new stackmap arrays
    // Stores all the scalar types associated with the stackmap:
    stackmap_scalars = Universe::new_int_array(temp_size JVM_CHECK_0);

    // Stores all the classes information for stack types of Item_Object
    stackmap_classes = Universe::new_obj_array(temp_size JVM_CHECK_0);

    // Read in the offset
    jushort stackmap_offset = get_u2(JVM_SINGLE_ARG_CHECK_0);

    // Save the bytecode offset for this stackmap entry at the beginning
    stackmap_scalars().int_at_put(0, stackmap_offset);

    for (juint index = 1, i = 0; i < 2; i++) {
      // read the size
      juint size = get_u2(JVM_SINGLE_ARG_CHECK_0);
      juint size_delta = 0;
      juint size_index = index++;
      juint max_size = (i == 0 ? frame_size : max_stack);

      for (jushort j = 0; j < size; j++) {
        // get the locals register or stack type
        jubyte stack_type = get_u1(JVM_SINGLE_ARG_CHECK_0);
        // Reading the j-th element of the stackmap.
        // This corresponds to the value in the j + size_delta'th
        // local register or stack location
        if (j + size_delta >= max_size) {
          classfile_parse_error(bad_stackmap_size JVM_THROW_0);
        } else if (stack_type == ITEM_NewObject) {
          jushort new_instr = get_u2(JVM_SINGLE_ARG_CHECK_0);
          if (new_instr >= code_length) {
            classfile_parse_error(bad_stackmap_type JVM_THROW_0);
          }
          // save the encoded object for NEW
          stackmap_scalars().int_at_put(index++, ENCODE_NEWOBJECT(new_instr));
        } else if (stack_type < ITEM_Object) {
          // save the stack type for the primitive types
          //<ITEM_Integer, ITEM_Float, ITEM_Double, ITEM_Long>
          stackmap_scalars().int_at_put(index++, stack_type);
          if (stack_type == ITEM_Long || stack_type == ITEM_Double) {
            if (j + size_delta + 1 >= max_size) {
              classfile_parse_error(bad_stackmap_size JVM_THROW_0);
            }
            stackmap_scalars().int_at_put(index++, 
                                          (stack_type == ITEM_Long) ? 
                                            ITEM_Long_2 : ITEM_Double_2);
            size_delta++;
          }
        } else if (stack_type == ITEM_Object) {
          jushort class_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
          cpf_check_0(class_index != 0 && class_index < cp->length(),
                      bad_constantpool_index);
          jubyte tag = cp->tag_value_at(class_index);
          cpf_check_0(ConstantTag::is_unresolved_klass(tag),
                      bad_constantpool_index);
          Symbol::Raw class_name = 
              cp->unresolved_klass_at(class_index JVM_CHECK_0);
          // save the stack tag type and the class name
          stackmap_scalars().int_at_put(index, ITEM_Object);
          stackmap_classes().obj_at_put(index++, &class_name);
        } else {
          classfile_parse_error(bad_stackmap_type JVM_THROW_0);
        }
        // save the size of the locals or stack
        stackmap_scalars().int_at_put(size_index, size + size_delta);
      }
    }

    // Save the stackmaps arrays
    stackmaps->obj_at_put(stackmap_index, &stackmap_scalars);
    stackmaps->obj_at_put(stackmap_index+1, &stackmap_classes);

    stackmap_index = stackmap_index + 2; // go to the next stackmap entry
  }

  // return the number of bytes read bytes read while parsing the stackmaps
  const int endpos = _bufptr - _buffer->ubyte_base_address();
  return (endpos - startpos) + 2;
}

inline ReturnOop ClassFileParser::parse_exception_table(int code_length, 
                                                 ConstantPool* cp JVM_TRAPS) {
  const int table_length = get_u2(JVM_SINGLE_ARG_CHECK_0);
  if (table_length == 0) {
    return NULL;
  }
  if ((_bufend - _bufptr) < (table_length * 4)) {
    truncated_class_file_error(JVM_SINGLE_ARG_THROW_0);
  }
  {
    // 4-tuples of ints [start_pc, end_pc, handler_pc, catch_type index]
    TypeArray::Raw table =
      Universe::new_short_array_raw(table_length * 4 JVM_CHECK_0);
    AllocationDisabler raw_pointers_used_in_this_block;
    {
      jushort *tableptr = table().ushort_base_address();
      jushort *tableend = tableptr + table_length * 4;
      jushort *bufptr = (jushort*)_bufptr;

      while (tableptr < tableend) {
        jushort start_pc   = Bytes::get_Java_u2((address)bufptr++);
        jushort end_pc     = Bytes::get_Java_u2((address)bufptr++);
        jushort handler_pc = Bytes::get_Java_u2((address)bufptr++);

        *tableptr++ = start_pc;
        *tableptr++ = end_pc;
        *tableptr++ = handler_pc;

        if (!((start_pc   <  code_length)  &&
              (end_pc     <= code_length)  &&
              (start_pc   <  end_pc)  &&
              (handler_pc <  code_length))) {
          goto corrupt;
        }

        jushort exception_index = Bytes::get_Java_u2((address)bufptr++);
        *tableptr++ = exception_index;
        if (exception_index != 0 && !cp->check_klass_at(exception_index)) {
          goto invalid;
        }
      }
      GUARANTEE(tableptr == tableend, "sanity");
      GUARANTEE(bufptr <= (jushort*)_bufend, "read past end of class data");
      _bufptr = (jubyte*)bufptr;
    }
    return table;
  }
corrupt:
  classfile_parse_error(corrupted_exception_handler JVM_THROW_0);

invalid:
  classfile_parse_error(invalid_constant JVM_THROW_0);
}

ReturnOop ClassFileParser::new_lazy_error_method(Method* method, 
                                                 address native_function
                                                 JVM_TRAPS)
{
  const int code_length = Bytecodes::length_for(Bytecodes::_fast_invokenative) ;

  // All sizing information for a Method is finally available, now create it.
  AccessFlags access = method->access_flags();
  access.atomic_set_bits(JVM_ACC_NATIVE);
  Method::Raw m = Universe::new_method(code_length, access JVM_CHECK_0);

  // Fill in information from fixed part (access_flags already set)
  ConstantPool::Raw cp = method->constants();
  m().set_constants(&cp);

  m().set_name_index(method->name_index());
  m().set_signature_index(method->signature_index());
  m().set_holder_id(method->holder_id());
  m().set_max_locals(m().compute_size_of_parameters());
  
  init_native_method(&m, native_function);
  m().set_default_entry(false);

#if ENABLE_JVMPI_PROFILE 
   //copy the method id
   m().set_method_id(method->method_id());     
#endif 

  if (ENABLE_BYTECODE_FLUSHING) {
    m().bytecode_at_flush_icache(0, Bytecodes::_fast_invokenative);
  }
 
  return m;
}

// Note: the parse_method below is big and clunky because all parsing
// of the code and exceptions attribute is inlined. This is cumbersome
// to avoid since we inline most of the parts in the methodOop to save
// footprint, so we only know the size of the resulting methodOop when
// the entire method attribute is parsed.

inline ReturnOop
ClassFileParser::parse_method(ClassParserState *state, ConstantPool* cp,
                              const AccessFlags class_access_flags JVM_TRAPS)
{
  UsingFastOops fast_oops;
  // Parse fixed parts
  const int flags = get_u2(JVM_SINGLE_ARG_CHECK_0);
  jushort name_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
  Symbol::Fast name = cp->checked_symbol_at(name_index JVM_CHECK_0);
  jushort signature_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
  Symbol::Fast signature = cp->checked_type_symbol_at(signature_index JVM_CHECK_0);
  Symbol::Fast method_attribute_name;
  AccessFlags access_flags;
  LineVarTable::Fast line_var_table;

  cpf_check_0(name().is_valid_method_name(), invalid_method_name);
  if (name.equals(Symbols::class_initializer_name())) {
    // We ignore the access flags for a class initializer. (JVM Spec. p. 116)
    access_flags.set_flags(JVM_ACC_STATIC);
  } else {
    access_flags.set_flags(flags & JVM_RECOGNIZED_METHOD_MODIFIERS);
    // shared_lock_synchronized_method() on ARM assume that the method
    // cannot belong to String so that it can skip a few checks for interned
    // Strings.
    GUARANTEE_R(!_loader_ctx->class_name()->equals(Symbols::java_lang_String())||
                !access_flags.is_synchronized(),
                "No String methods can be synchronized");
    cpf_check_0(
      are_valid_method_access_flags(state, &name, access_flags, 
                                    class_access_flags),
      invalid_method_access_flags);
  }

  cpf_check_0(signature().is_valid_method_signature(&name),
          invalid_method_signature);

  // Default values for code and exceptions attribute elements
  jushort max_stack = 0;
  jushort max_locals = 0;
  juint max_stack_count = 0;
  juint code_length = 0;
  jint code_offset = 0;
  TypeArray::Fast exception_table;
  bool is_code_required = 
      !(access_flags.is_native() || access_flags.is_abstract());
  bool parsed_checked_exceptions_attribute = false;
  ObjArray::Fast stackmaps;

#if ENABLE_REFLECTION
  TypeArray::Fast thrown_exceptions;
#endif

  // Parse code and exceptions attribute
  int method_attributes_count = get_u2(JVM_SINGLE_ARG_CHECK_0);
  while (method_attributes_count--) {
    jushort method_attribute_name_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
    juint method_attribute_length = get_u4(JVM_SINGLE_ARG_CHECK_0);
    int expected_attribute_end_position =
        get_buffer_position() + method_attribute_length;

    method_attribute_name = cp->checked_symbol_at(method_attribute_name_index
                                                  JVM_CHECK_0);
    if (method_attribute_name.equals(Symbols::tag_code())) {
      // Parse Code attribute
      cpf_check_0(is_code_required, superfluous_code_attribute);
      is_code_required = false;

      // Stack and locals size
      max_stack = get_u2(JVM_SINGLE_ARG_CHECK_0);
      max_locals = get_u2(JVM_SINGLE_ARG_CHECK_0);

      // Code size and pointer
      code_length = get_u4(JVM_SINGLE_ARG_CHECK_0);

      cpf_check_0(code_length < 65536, excessive_code_length); // JVM Spec 4.8

      code_offset = get_buffer_position();
      skip_u1(code_length JVM_CHECK_0);

      // Exception handler table
      exception_table = parse_exception_table(code_length, cp JVM_CHECK_0);

      // Stackmaps
      stackmaps = parse_code_attributes(cp, max_stack, max_locals,
                                        code_length, &line_var_table
                                        JVM_CHECK_0);
    } else if (method_attribute_name.equals(Symbols::tag_exceptions())) {
      cpf_check_0(!parsed_checked_exceptions_attribute, 
                  duplicate_exception_table);
      parsed_checked_exceptions_attribute = true;
      jushort checked_exceptions_length = get_u2(JVM_SINGLE_ARG_CHECK_0);
#if ENABLE_REFLECTION
      thrown_exceptions =
        Universe::new_short_array_raw(checked_exceptions_length JVM_CHECK_0);
#endif
      for (int i = 0; i < checked_exceptions_length; i++) {
        jushort exception_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
        cpf_check_0(cp->check_klass_at(exception_index), invalid_constant);
#if ENABLE_REFLECTION
        thrown_exceptions().ushort_at_put(i, exception_index);
#endif
      }
    } else if (method_attribute_name.equals(Symbols::tag_synthetic())) {
      // Should we check that there hasn't already been a synthetic attribute?
      access_flags.set_is_synthetic();
    } else {
      // Skip unknown attributes
      skip_u1(method_attribute_length JVM_CHECK_0);
    }
    cpf_check_0(get_buffer_position() == expected_attribute_end_position,
            corrupted_attribute);
  }
  cpf_check_0(!is_code_required, missing_code_attribute);

  // Setup bytecode for native methods.
  if (access_flags.is_native() || access_flags.is_abstract()) {
    GUARANTEE(code_length == 0, "Sanity check");
    code_length = Bytecodes::length_for(Bytecodes::_fast_invokenative);
  }

  // All sizing information for a Method is finally available, now create it.
  Method::Fast m = Universe::new_method(code_length, access_flags JVM_CHECK_0);

  // set to an illegal value to catch accesses
  m().set_holder_id(0xFFFF);

  // Fill in information from fixed part (access_flags already set)
  m().set_constants(cp);
  m().set_name_index(name_index);
  m().set_signature_index(signature_index);
  m().compute_size_of_parameters();
#if ENABLE_ROM_JAVA_DEBUGGER
  m().set_line_var_table(&line_var_table);
#endif

#if  ENABLE_JVMPI_PROFILE 
  // Set the current compiled method ID. 
  if(UseJvmpiProfiler) {
    m().set_method_id(Universe::number_of_java_methods());
    Universe::inc_number_of_java_methods();
  }
#endif

  // See JVM Spec 4.10
  cpf_check_0(m().size_of_parameters() <= 255, too_many_method_parameters);

  // Fill in code attribute information
  if (m().is_native() || m().is_abstract()) {
    max_locals = m().size_of_parameters();
  } else {
    cpf_check_0(m().size_of_parameters() <= max_locals, invalid_frame_size);
  }
  m().set_max_locals(max_locals);

  // Note: this will make sure we won't overflow the stack -- in the
  // worst case, we will need (max_stack + max_locals + StackLock::size() * 8)
  // number of words on the stack before we can enter a method.
  GUARANTEE(StackPadding >= (int)(StackLock::size() * 8), "sanity");
  max_stack_count = (juint)max_stack + (juint)max_locals;

  if (max_stack_count >= 0x00008000) {
    // Make sure we don't overflow Method::max_execution_stack_count.
    // Make it less than 0x8000, in case we have subtle sign-extension bugs.
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_(0));
  }

  m().set_max_execution_stack_count((jushort)max_stack_count);
  m().set_exception_table(&exception_table);

  if (stackmaps.is_null()) {
    stackmaps = Universe::empty_obj_array();
  }

#if ENABLE_REFLECTION
  m().set_thrown_exceptions(&thrown_exceptions);
#endif
 
  // Copy byte codes
  if (m().access_flags().is_native() || m().access_flags().is_abstract()) {
    // Setup unimplemented native code reference.
    if (m().is_abstract()) {
      init_native_method(&m, (address) Java_abstract_method_execution);
    } else {
#if ENABLE_DYNAMIC_NATIVE_METHODS && !USE_SOURCE_IMAGE_GENERATOR
      Signature::Fast signature = m().signature();

#define INIT_NATIVE_METHOD_CASE(jtype, ttype, arg)             \
          case ttype :                                         \
            init_native_method(                                \
              &m, (address) Java_ ## jtype ## _unimplemented); \
            break;      

      switch(signature().return_type()) {
        FOR_ALL_TYPES(INIT_NATIVE_METHOD_CASE)
          default:
        SHOULD_NOT_REACH_HERE();
      }
#else
      init_native_method(&m, (address) Java_void_unimplemented);
#endif
    }
  } else {
    if (code_length > 0) {
      // This is safe wrt. buffer bounds, because they have already
      // been checked when skipping over the code (see
      // 'skip_u1(code_length JVM_CHECK_0)' above):
      jvm_memcpy(m().code_base(), raw_u1_buffer(code_offset), code_length);

#if ENABLE_ROM_GENERATOR
      _total_bytecode_bytes += code_length;
#endif
    }
  }

  m().set_default_entry(false);
  m().check_bytecodes(JVM_SINGLE_ARG_CHECK_0);
  m().set_stackmaps(&stackmaps);

  if (ENABLE_BYTECODE_FLUSHING && code_length > 0) {
    m().code_flush_icache();
  }
  return m;
}

// Parse the attributes contained by a method's Code attribute.
inline ReturnOop ClassFileParser::parse_code_attributes(ConstantPool* cp,
                   jushort max_stack, jushort max_locals, juint code_length,
                   LineVarTable *line_var_table JVM_TRAPS)
{
  UsingFastOops fast_oops;
  ObjArray::Fast stackmaps;
  jushort num_stackmaps;
  int attributes_count = get_u2(JVM_SINGLE_ARG_CHECK_0);

  while (attributes_count--) {
    jushort attribute_name_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
    juint attribute_length = get_u4(JVM_SINGLE_ARG_CHECK_0);
    cpf_check_0((attribute_name_index > 0) &&
                (attribute_name_index < cp->length()) &&
                ConstantTag::is_utf8(cp->tag_value_at(attribute_name_index)),
                invalid_attribute);

    Symbol::Raw attribute_name_sym =
          cp->checked_symbol_at(attribute_name_index JVM_CHECK_0);
#if ENABLE_ROM_JAVA_DEBUGGER
    // See if this is a line number table If so, then read in the table
    if (GenerateROMImage &&
        attribute_name_sym.equals(Symbols::tag_line_number_table())) {
      jushort line_number_entries = get_u2(JVM_SINGLE_ARG_CHECK_0);
      jushort start_pc, line_number;
      cpf_check_0((((line_number_entries * 2 * sizeof(jushort)) +
              sizeof(jushort)) == attribute_length), invalid_attribute);
      if (line_number_entries > 0) {
        UsingFastOops fast_oops2;
        // 2 shorts per entry
        if (line_var_table->is_null()) {
          *line_var_table = LineVarTable::new_line_var_table(JVM_SINGLE_ARG_CHECK_0);
        }
        LineNumberTable::Fast line_number_table =
          Universe::new_short_array((line_number_entries * 2) JVM_CHECK_0);
        line_var_table->set_line_number_table(line_number_table.obj());
        for (int i = 0; i < line_number_entries; i ++) {
          // start_pc
          start_pc = get_u2(JVM_SINGLE_ARG_CHECK_0);
          line_number_table().set_pc(i, start_pc);
          // line_number
          line_number = get_u2(JVM_SINGLE_ARG_CHECK_0);
          line_number_table().set_line_number(i, line_number );
        }
      }
    } else if (GenerateROMImage &&
               attribute_name_sym.equals(Symbols::tag_local_var_table())) {
      jushort local_var_entries = get_u2(JVM_SINGLE_ARG_CHECK_0);
      jushort start_pc, code_length, slot_index;
      cpf_check_0((((local_var_entries * 5 * sizeof(jushort)) +
            sizeof(jushort)) == attribute_length), invalid_attribute);
      if (local_var_entries > 0) {
        UsingFastOops fast_oops3;
        if (line_var_table->is_null()) {
          *line_var_table = LineVarTable::new_line_var_table(JVM_SINGLE_ARG_CHECK_0);
        }
        LocalVariableTable::Fast local_var_table =
          Universe::new_short_array((local_var_entries * 3) JVM_CHECK_0);
        line_var_table->set_local_variable_table(local_var_table.obj());
        for (int i = 0; i < local_var_entries; i++) {
          start_pc = get_u2(JVM_SINGLE_ARG_CHECK_0);
          local_var_table().set_start_pc(i, start_pc);
          code_length = get_u2(JVM_SINGLE_ARG_CHECK_0);
          local_var_table().set_code_length(i, code_length);
          // name index and descriptor index are not used
          (void)get_u2(JVM_SINGLE_ARG_CHECK_0);
          (void)get_u2(JVM_SINGLE_ARG_CHECK_0);
          slot_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
          local_var_table().set_slot(i, slot_index);
        }
      }
    } else 
#else
    (void)line_var_table;
#endif
    if (attribute_name_sym.equals(Symbols::tag_stackmap())) {
      // parse for the stackmap attribute
      if (stackmaps.not_null()) {
        // Duplicate stackmap attribute
        classfile_parse_error(duplicate_stackmap_attribute 
                              JVM_NO_CHECK_AT_BOTTOM);
        return 0;
      }

      num_stackmaps = get_u2(JVM_SINGLE_ARG_CHECK_0);
      stackmaps = Universe::new_obj_array(2*num_stackmaps JVM_CHECK_0);

      // parse the stackmap entries for this method
      juint stackmap_attribute_length =
               parse_stackmaps(cp, num_stackmaps, max_stack, max_locals,
                                   code_length, &stackmaps JVM_CHECK_0);
      if (stackmap_attribute_length != attribute_length) {
        // Bad stackmap attribute size
        classfile_parse_error(bad_stackmap_attribute_size 
                              JVM_NO_CHECK_AT_BOTTOM);
        return 0;
      }
#if ENABLE_ROM_GENERATOR
      _total_stackmap_bytes += attribute_length;
#endif
    } else {
      // skip unknown attributes
      skip_u1(attribute_length JVM_CHECK_0);
    }
  }

  return stackmaps.obj();
}

void ClassFileParser::init_native_method(Method *m, address native_function)
{
  Signature::Raw signature = m->signature();
  BasicType type = signature().return_type();
  m->bytecode_at_put_raw(0, Bytecodes::_fast_invokenative);
  m->byte_at_put(1, stack_type_for(type));
  m->set_native_code(native_function);

  // No need to flush the icache, since this is only called by 
  // init_native_method() or new_lazy_error_method(), both of which would
  // flush the icache after we return.
}

// Methods must not have the same name AND type
inline void ClassFileParser::check_for_duplicate_methods(ConstantPool *cp, 
                                                  const ObjArray& methods JVM_TRAPS)
{
  {
    // This is a hot loop, so we're using raw pointers here to help C++
    // compiler generate better code. If a GC happens we're
    // in deep trouble!

    AllocationDisabler raw_pointers_used_in_this_block;
    const int len = methods.length();
    OopDesc ** cp_base = (OopDesc**)cp->base_address();

    // Note: this algorithm has quadratic complexity
    //
    // We can't just check (method_a().name_index() != method_b().name_index())
    // because two different indices in the same CP may contain the same 
    // symbol!
    for (int i = 0; i < len - 1; i++) {
      Method::Raw method_a = methods.obj_at(i);
      const jint name_a = method_a().name_index();

      for (int j = i + 1; j < len; j++) {
        Method::Raw method_b = methods.obj_at(j);
        const jint name_b = method_b().name_index();
        if (cp_base[name_a] == cp_base[name_b]) {
          const jint signature_a = method_a().signature_index();
          const jint signature_b = method_b().signature_index();
          if (cp_base[signature_a] == cp_base[signature_b]) {
            goto error;
          }
        }
      }
    }

    return; // no duplicates found
  }

error:
  classfile_parse_error(duplicate_field JVM_NO_CHECK_AT_BOTTOM);
}

inline ReturnOop
ClassFileParser::parse_methods(ClassParserState *state, ConstantPool* cp,
                               const AccessFlags class_access_flags,
                               bool& has_native_methods JVM_TRAPS) {
  UsingFastOops fast_oops;
  has_native_methods = false;
  int length = get_u2(JVM_SINGLE_ARG_CHECK_0);
  ObjArray::Fast methods = Universe::new_obj_array(length JVM_CHECK_0);
  for (int index = 0; index < length; index++) {
    Method::Raw method = parse_method(state, cp, class_access_flags JVM_CHECK_0);
    has_native_methods |= method().is_native();
    methods().obj_at_put(index, &method);
  }
  if (get_UseVerifier()) {
    check_for_duplicate_methods(cp, methods() JVM_CHECK_0);
  }
  return methods;
}

inline
void ClassFileParser::parse_classfile_sourcefile_attribute(ConstantPool* cp
                                                           JVM_TRAPS) {
  const jushort sourcefile_index = get_u2(JVM_SINGLE_ARG_CHECK);
  // IMPL_NOTE: no need to intern the Symbol, unless ENABLE_JAVA_DEBUGGER
  (void) cp->checked_symbol_at(sourcefile_index JVM_NO_CHECK_AT_BOTTOM);
}

inline void ClassFileParser::parse_classfile_inner_classes_attribute(
                      ConstantPool* cp, InstanceClass* c JVM_TRAPS) {
  juint length = get_u2(JVM_SINGLE_ARG_CHECK);

  if (length > 0) {
#if ENABLE_REFLECTION
    UsingFastOops fast_oops;
    TypeArray::Fast inner_classes = Universe::new_short_array(length JVM_CHECK);
    Symbol::Fast outer_name;
    juint inner_count = 0;
#endif
    for (juint n = 0; n < length; n++) {
      // Inner class index
      juint inner_class_info_index = get_u2(JVM_SINGLE_ARG_CHECK);
      cpf_check(inner_class_info_index == 0 ||
                ConstantTag::is_klass_reference(
                  cp->tag_value_at(inner_class_info_index)),
                invalid_class_file);

      // Outer class index
      juint outer_class_info_index = get_u2(JVM_SINGLE_ARG_CHECK);
      cpf_check(outer_class_info_index == 0 ||
                ConstantTag::is_klass_reference(
                  cp->tag_value_at(outer_class_info_index)),
                invalid_class_file);

      // Inner class name
      juint inner_name_index = get_u2(JVM_SINGLE_ARG_CHECK);
      cpf_check(inner_name_index == 0 ||
                ConstantTag::is_utf8(cp->tag_value_at(inner_name_index)),
                invalid_class_file);

      // Access flags
      /* juint flags = */ get_u2(JVM_SINGLE_ARG_CHECK);

#if ENABLE_REFLECTION
      if (inner_class_info_index != 0 && outer_class_info_index != 0) {
        outer_name = cp->unchecked_unresolved_klass_at(outer_class_info_index);
        if (outer_name().equals(c->name())) {
          inner_classes().ushort_at_put(inner_count++,
                                        (jushort) inner_class_info_index);
        }
      }
#endif
    }

#if ENABLE_REFLECTION
    if (inner_count > 0) {
      inner_classes().shrink(inner_count);
      c->set_inner_classes(&inner_classes);
    }
#endif
  (void)c;
  }
}

inline void
ClassFileParser::parse_classfile_attributes(ConstantPool* cp, 
                                            InstanceClass* c JVM_TRAPS) {
#if ENABLE_REFLECTION
  c->set_inner_classes(Universe::empty_short_array());
#endif
  bool have_inner_class_attribute = false;

  int attributes_count = get_u2(JVM_SINGLE_ARG_CHECK);
  // Iterate over attributes
  while (attributes_count--) {
    int attribute_name_index = get_u2(JVM_SINGLE_ARG_CHECK);
    juint attribute_length = get_u4(JVM_SINGLE_ARG_CHECK);
    Symbol::Raw tag = cp->checked_symbol_at(attribute_name_index JVM_CHECK);

    if (tag.equals(Symbols::tag_source_file())) {
      parse_classfile_sourcefile_attribute(cp JVM_NO_CHECK_AT_BOTTOM);
    } else if (tag.equals(Symbols::tag_inner_classes())) {
      if (have_inner_class_attribute) {
        classfile_parse_error(multiple_inner_class_attribute JVM_THROW);
      }
      parse_classfile_inner_classes_attribute(cp, c JVM_NO_CHECK_AT_BOTTOM);
      have_inner_class_attribute = true;
    } else if (tag.equals(Symbols::tag_synthetic())) {
      // Shouldn't we check that the synthetic flags wasn't already set?
      if (attribute_length != 0) {
        classfile_parse_error(invalid_synthetic_attribute JVM_THROW);
      }
      parse_classfile_synthetic_attribute(c);
    } else {
      // Unknown attribute
      skip_u1(attribute_length JVM_CHECK);
    }
  }
}

inline bool ClassFileParser::is_circular(InstanceClass* this_class) {
  for( InstanceClass::Raw c = this_class;; ) {
    c = c().super();
    if( c().is_null() ) {
      return false;
    }
    if (c.equals(this_class)) {
      return true;
    }
  }
}

inline void
ClassFileParser::check_local_interfaces(InstanceClass* this_class JVM_TRAPS) {
  UsingFastOops fast_oops;
  TypeArray::Fast local_interfaces = this_class->local_interfaces();
  InstanceClass::Fast interf;
  const int len = local_interfaces().length();

  for (int i = 0; i < len; i++) {
    const int class_id = local_interfaces().ushort_at(i);
    interf = Universe::class_from_id(class_id);
    cpf_check(!interf.equals(this_class) &&
              interf().is_interface(), invalid_class_file)
    interf().check_access_by(this_class, ErrorOnFailure JVM_CHECK);

    for (int j = i; --j >= 0;) {
      int other_class_id = local_interfaces().ushort_at(j);
      cpf_check(other_class_id != class_id, circular_interfaces);
    }
  }
}

inline
void ClassFileParser::check_for_circular_class_parsing(ClassParserState *stack
                                                       JVM_TRAPS) {
  // Check whether this is represented twice on the static class file
  // parse list.
  // IMPL_NOTE: is this still needed?? (don't delete unless you're 100% sure)
  {
    for (ClassFileParser* check = previous(); check != NULL;
         check = check->previous()) {
      if (name()->equals(check->name())) {
        classfile_parse_error(recursive_class_structure JVM_THROW);
      }
    }
  }

  // Check whether this is represented twice on the class parsing stack
  for(ClassParserState::Raw chk = stack->next(); chk.not_null();
                                                 chk = chk().next()) {
    Symbol::Raw class_name = chk().class_name();
    if (name()->equals(&class_name) && chk().stage() != 0) {
      classfile_parse_error(recursive_class_structure JVM_THROW);
    }
  }
}

bool ClassFileParser::is_package_restricted(Symbol *class_name) {
  // This check is done only if we've romized the system classes, otherwise
  // we won't even be able to load java.lang.Object!
  GUARANTEE(UseROM, "sanity");

  const char* ptr = class_name->base_address();
  const char* s = ptr + class_name->length();
  do {
    if( --s <= ptr ) {
      // This class is in the 'default' package, which is always unrestricted
      return false;
    }
  } while (*s != '/');

  const int pkg_length = s - ptr;
  bool is_restricted = ROM::is_restricted_package(ptr, pkg_length);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  if (!is_restricted) {
    is_restricted = ROM::is_restricted_package_in_profile(ptr, pkg_length);
  }
#endif
#if ENABLE_DYNAMIC_RESTRICTED_PACKAGE
  if (!is_restricted) {
    is_restricted = JVMSPI_IsRestrictedPackage(ptr, pkg_length);
  }
#endif
#if ENABLE_ISOLATES
  if (!is_restricted) {
    is_restricted = Task::current()->is_restricted_package(ptr, pkg_length);
  }
#endif
  return is_restricted;
}

// Perform "stage 0" parsing of a class file
inline bool ClassFileParser::parse_class_0(ClassParserState *stack JVM_TRAPS) {
  UsingFastOops fast_oops;
  ClassParserState::Fast state = stack->top();

  // Magic value
  juint magic = get_u4(JVM_SINGLE_ARG_CHECK_0);
  cpf_check_0(magic == 0xCAFEBABE, incompatible_magic_value);

  // Version numbers -- checked by class format checker.
  int minor_version = get_u2(JVM_SINGLE_ARG_CHECK_0);
  int major_version = get_u2(JVM_SINGLE_ARG_CHECK_0);

  if ((major_version < JAVA_MIN_SUPPORTED_VERSION) ||
      (major_version > JAVA_MAX_SUPPORTED_VERSION)) {
          Throw::class_format_error(invalid_class_version JVM_CHECK_0);
  }

  state().set_major_version(major_version);
  state().set_minor_version(minor_version);

  // Constant pool
  ConstantPool::Fast cp = parse_constant_pool(JVM_SINGLE_ARG_CHECK_0);
  state().set_cp(&cp);

  // Access flags
  AccessFlags access_flags;
  jint flags = get_u2(JVM_SINGLE_ARG_CHECK_0);
  //preloaded flag might be already set!
  state().set_access_flags(state().access_flags() | flags);
  access_flags.set_flags(flags & JVM_RECOGNIZED_CLASS_MODIFIERS);
  if (access_flags.is_interface()) {
    cpf_check_0(access_flags.is_abstract() && !access_flags.is_final(), 
            bad_class_flags);
  } else {
    cpf_check_0(!(access_flags.is_final() && access_flags.is_abstract()), 
            bad_class_flags);
  }

  // This class and superclass
  InstanceClass::Fast super_class;
  bool super_class_resolved = true;
  int this_class_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
  Symbol::Fast class_name = cp().unresolved_klass_at(this_class_index 
                                                     JVM_CHECK_0);
  Symbol::Fast super_class_name;

  // Checks if name in class file matches requested name
  cpf_check_0(!class_name.is_null(), wrong_class_name);
  if (!class_name().matches(name())) {
    Throw::class_not_found(name(), ErrorOnFailure JVM_THROW_0);
  }

  int super_class_index = get_u2(JVM_SINGLE_ARG_CHECK_0);
  if (super_class_index == 0) {
    if (ROMIZED_PRODUCT) {
      // In a romized product build, you can't redefine java/lang/Object
      classfile_parse_error(invalid_superclass JVM_THROW_0);
    } else {
      cpf_check_0(class_name.equals(Symbols::java_lang_Object()),
                  invalid_superclass);
    }
  } else {
    // Check if super class exists
    super_class_name = cp().unresolved_klass_at(super_class_index JVM_CHECK_0);

    // The test
    // "javasoft.sqe.tests.vm.classfmt.clf.clfsup006.clfsup00602m1"
    // requires that we perform the following check BEFORE trying to
    // resolve a bogus superclass for an interface in the next
    // statement
    cpf_check_0(!access_flags.is_interface() || 
            super_class_name.equals(Symbols::java_lang_Object()),
            interfaces_must_extend_JLO);

    // Find the superclass
    LoaderContext super_ctx(&super_class_name, ErrorOnFailure, _loader_ctx);
    super_class = SystemDictionary::find(&super_ctx, /*lookup_only=*/ false,
                                         /*check_only=*/ true JVM_CHECK_0);
    if (super_class.is_null()) {
      stack->push(&super_class_name JVM_CHECK_0);
      super_class_resolved = false;
    }
  }
  state().set_super_class_index(super_class_index);

  // Interfaces
  bool interfaces_resolved;
  {
    TypeArray::Raw interface_indices = 
      parse_interface_indices(stack, &cp, interfaces_resolved JVM_CHECK_0);
    state().set_interface_indices(&interface_indices);
  }

  // The unresolved superclass and/or unresolved interface(s) have been
  // placed on the stack. They (and this class) will be resolved later.
  return super_class_resolved && interfaces_resolved;
}

inline void ClassFileParser::clone_invoke_special_virtual_conflicts(
                                  InstanceClass* this_klass, 
                                  ConstantPool* old_cp, ConstantPool* new_cp, 
                                  TypeArray* relocation_map, int map_size)
{
  {
    TypeArray::Raw old_tags = old_cp->tags();
    const int cp_length = old_cp->length();
    TypeArray::Raw new_tags = new_cp->tags();
    TypeArray::array_copy(&old_tags, 0, &new_tags, 0, cp_length);
    const void* old_cp_base_address = old_cp->base_address();
    void* new_cp_base_address = new_cp->base_address();
    jvm_memcpy(new_cp_base_address, old_cp_base_address, cp_length*sizeof(int));
  }
  {    
    for( int i = 0; i < map_size; i+=2 ) {
      const int old_offset = relocation_map->ushort_at(i);
      const int new_offset = relocation_map->ushort_at(i+1);
      new_cp->tag_at_put(new_offset, old_cp->tag_value_at(old_offset));
      new_cp->value32_at_put(new_offset, old_cp->value32_at(old_offset));
    }
  }
  {
    ObjArray::Raw methods = this_klass->methods();
    const int methods_length = methods().length();
    //updating invoke_special indexes in methods
    for (int i = 0; i < methods_length; i++) {
      Method::Raw method = methods().obj_at(i);
      if (method.is_null()) continue;
      if (!old_cp->equals(method().constants())) {
        GUARANTEE(method().is_native() &&
                  method().get_native_code() ==
                  (address)Java_abstract_method_execution,
                  "Must be a miranda method");
        continue;
      }
      method().set_constants(new_cp);
      const int code_length = method().code_size();
      for( int bci = 0; bci < code_length; ) {
        const Bytecodes::Code code = method().bytecode_at(bci);
        if (code == Bytecodes::_invokespecial) {
          const int cp_index = method().get_java_ushort(bci+1);
          for (int j = 0; j < map_size; j+=2) {
            const int old_cp_index = relocation_map->ushort_at(j);
            if (old_cp_index == cp_index) {
              method().put_java_ushort(bci+1, relocation_map->ushort_at(j+1));
            }
          }
        }
        bci = method().next_bci( bci );
      }
    }
  }
}

inline void ClassFileParser::fill_in_invoke_indexes(InstanceClass* this_klass,
                                             TypeArray* invoke_sp_ids, 
                                             TypeArray* invoke_vi_ids) {
  ObjArray::Raw methods = this_klass->methods();
  const int methods_length = methods().length();
  for (int i = 0; i < methods_length; i++) {
    Method::Raw method = methods().obj_at(i);  
    const int code_length = method().code_size() - 2;
    for( int bci = 0; bci < code_length; bci = method().next_bci( bci ) ) {
      TypeArray* invoke_ids;
      const Bytecodes::Code code = method().bytecode_at(bci);
      switch( code ) {
        case Bytecodes::_invokespecial:
          invoke_ids = invoke_sp_ids;
          break;
        case Bytecodes::_invokevirtual:
          invoke_ids = invoke_vi_ids;
          break;
        default: continue;
      }
      const int index = method().get_java_ushort(bci+1);
      const int idx1 = index / BitsPerWord;
      const int idx2 = index % BitsPerWord;
      const int map = invoke_ids->int_at(idx1) | (1 << idx2);
      invoke_ids->int_at_put(idx1, map);
    }
  }  
}

inline void ClassFileParser::resolve_invoke_special_virtual_conflicts(
                                     InstanceClass* this_klass JVM_TRAPS) {
  UsingFastOops fast;
  ConstantPool::Fast cp = this_klass->constants();
  const int cp_length = cp().length();
  const int bitmap_length = cp_length / BitsPerWord + 1;
  TypeArray::Fast invoke_virtual_indexes =
    Universe::new_int_array(bitmap_length JVM_CHECK);
  TypeArray::Fast invoke_special_indexes =
    Universe::new_int_array(bitmap_length JVM_CHECK);  
  
  fill_in_invoke_indexes(this_klass, &invoke_special_indexes, &invoke_virtual_indexes);

  int count = 0;
  int i;
  for (i = 0; i < bitmap_length; i++) {
    for( int collision = invoke_special_indexes().int_at(i) & 
                         invoke_virtual_indexes().int_at(i);
         collision; collision >>= 1 ) {
      if (collision & 1) {
        count++;
      }      
    }
  }
  if (count == 0) return; //nothing to do
  if (count + cp_length > 65535) { //overflow
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
  }
  TypeArray::Fast relocation_map = Universe::new_short_array(2*count JVM_CHECK);

  count = 0;
  for (i = 0; i < bitmap_length; i++) {
    int offset = 0;
    for( int collision = invoke_special_indexes().int_at(i) &
                         invoke_virtual_indexes().int_at(i);
         collision; collision >>= 1 ) {
      if( collision & 1 ) {
        relocation_map().ushort_at_put(2*count, i*BitsPerWord + offset);
        relocation_map().ushort_at_put(2*count + 1, cp_length + count);          
        count++;
      }
      offset++;
    }
  }

  ConstantPool::Fast new_cp =
    Universe::new_constant_pool(count + cp_length JVM_CHECK);
  clone_invoke_special_virtual_conflicts(this_klass, &cp, &new_cp,
                                         &relocation_map, 2*count);
  ClassInfo::Raw klass_info = this_klass->class_info();
  klass_info().set_constants(&new_cp);
}

ReturnOop ClassFileParser::parse_class_internal(ClassParserState *stack JVM_TRAPS) {
  UsingFastOops fast_oops;
  check_for_circular_class_parsing(stack JVM_CHECK_0);
  
  ClassParserState::Fast state = stack->top();
  if (state().stage() == 0) {
    bool resolved = ClassFileParser::parse_class_0(stack JVM_CHECK_0);
    state().set_stage(1);
    state().set_buffer_pos(get_buffer_position());
    if (!resolved) {
      return NULL;
    }
  }

  ConstantPool::Fast cp = state().cp();
  set_buffer_position(state().buffer_pos());
  AccessFlags access_flags;
  access_flags.set_flags(state().access_flags() & 
                         (JVM_RECOGNIZED_CLASS_MODIFIERS | JVM_ACC_PRELOADED));

  Symbol::Fast class_name = name();
  Symbol::Fast super_class_name;
  InstanceClass::Fast super_class;

  jushort super_class_index = (jushort)state().super_class_index();
  if (super_class_index != 0) {
    super_class_name = cp().unresolved_klass_at(super_class_index JVM_CHECK_0);
    LoaderContext super_ctx(&super_class_name, ErrorOnFailure, _loader_ctx);
    super_class = SystemDictionary::find(&super_ctx, /*lookup_only=*/ false,
                                         /*check_only=*/ false
                                         JVM_CHECK_0);

    cpf_check_0(!super_class().is_final() &&
                !super_class().is_interface(), invalid_class_file);
  }

  // Interfaces
  TypeArray::Fast interface_indices = state().interface_indices();
  TypeArray::Fast local_interfaces = parse_interfaces(&cp, &interface_indices
                                                      JVM_CHECK_0);

  // Fields
  // Total size of fields is computed.
  // Relative offsets are filled in, real offsets are computed below
  int nonstatic_field_size = 0;
  int static_field_size = 0;
  TypeArray::Fast fields = parse_fields(&cp, nonstatic_field_size,
                                        static_field_size, access_flags
                                        JVM_CHECK_0);

  // Methods
  bool has_native_methods;
  ObjArray::Fast methods = parse_methods(&state, &cp,
                                         access_flags, has_native_methods
                                         JVM_CHECK_0);

  TypeArray::Fast all_super_interfaces;
  TypeArray::Fast bitmap;
  if (!access_flags.is_interface() && access_flags.is_abstract() && local_interfaces().length() > 0) {    
    int bitmap_size = Universe::number_of_java_classes() / BitsPerWord + 1;
    bitmap = Universe::new_int_array(bitmap_size JVM_CHECK_0);
    int all_interface_count = count_interfaces(local_interfaces.obj(), bitmap.obj());    
    all_super_interfaces = Universe::new_short_array(all_interface_count JVM_CHECK_0);
    jvm_memset(bitmap().base_address(), 0, bitmap_size*sizeof(int));
    int found = found_all_interfaces(local_interfaces.obj(), all_super_interfaces.obj(), 0, bitmap.obj());
    GUARANTEE(found == all_interface_count, "calculation check");
    (void)found;
    methods = InstanceClass::add_miranda_methods(&super_class,
                                                 &methods,
                                                 &all_super_interfaces,
                                                 &class_name JVM_CHECK_0);
  }

  // Size of Java vtable (in words)
  int vtable_length = 0;
  JavaVTable::compute_vtable_size(vtable_length,
                           &super_class,
                           &methods,
                           access_flags,
                           &class_name);
  // Size of Java itable (in words)
  int itable_length = 0;
  int itable_size = JavaITable::compute_itable_size(&local_interfaces,
                                                    &super_class,
                                                    itable_length);

  // Finalization support
  if (super_class.not_null()) {  // Don't set flag for java.lang.Object
    Method::Raw fm = 
      InstanceClass::find_method(&methods, Symbols::finalize_name(),
                                 Symbols::void_signature());
    if (fm.not_null() && fm().is_private() && fm().is_native()) {
#if !USE_SOURCE_IMAGE_GENERATOR
      if (fm().get_native_code() == (address)Java_void_unimplemented) {
        access_flags.set_has_unresolved_finalizer();
      } else 
#endif
      {
        access_flags.set_has_finalizer();
      }
    }
  }
#if ENABLE_ISOLATES
  if (Task::current()->is_hidden_class(&class_name)) {
    access_flags.set_is_hidden();
  }
#endif
  // Iterate over fields again and compute correct offsets.  The
  // relative offset (starting at zero) was temporarily stored in the
  // offset slot.
  // Also compute oop maps for nonstatic and static fields
#if ENABLE_ISOLATES
  // Static variables are embedded in mirror objects.
  // The part of the oop map dedicated to static variable refers
  // to offset in the mirror object.
  int first_static_offset = TaskMirror::static_field_start();
#else //ENABLE_ISOLATES
  int first_static_offset = JavaClassDesc::header_size();
#endif //ENABLE_ISOLATES
  int prev_static_oop_offset = 0;
  int static_map_size = 1;    // sentinel for termination

  int first_nonstatic_offset;
  int prev_nonstatic_oop_offset = 0;
  int nonstatic_map_size = 1; // sentinel for termination
  if (super_class.not_null()) {
    first_nonstatic_offset = super_class().instance_size().fixed_value();
    // Copy down oop map sizes from superclass
    nonstatic_map_size = super_class().nonstatic_map_size();
    prev_nonstatic_oop_offset = super_class().last_nonstatic_oop_offset();
  } else {
    first_nonstatic_offset = Instance::header_size();
  }

  // Size of instances
  int instance_size = align_allocation_size(first_nonstatic_offset + 
                                            nonstatic_field_size);

  // Allocate temporary holders for oop maps (guaranteed to be large enough)
  TypeArray::Fast static_map = Universe::new_byte_array(static_field_size + 1
                                                        JVM_CHECK_0);
  int static_map_index = 0;
  TypeArray::Fast nonstatic_map = Universe::new_byte_array(instance_size + 1
                                                           JVM_CHECK_0);
  int nonstatic_map_index = 0;
  if (super_class.not_null()) {
    // Copy down oop map contents from superclass
    size_t map_offset = super_class().first_nonstatic_map_offset();
    jubyte value;
    while ((value = super_class().oop_map_at(map_offset++)) != OopMapSentinel) {
      nonstatic_map().ubyte_at_put(nonstatic_map_index++, value);
    }
  }

  // Update non-static field offsets and non-static oop map
  update_fields(&cp, &fields, false, first_nonstatic_offset, 
                prev_nonstatic_oop_offset,
                &nonstatic_map, nonstatic_map_size, nonstatic_map_index);

  // Update static field offsets and static oop map
  update_fields(&cp, &fields, true, first_static_offset, prev_static_oop_offset,
                &static_map, static_map_size, static_map_index);

  // We can now create the basic InstanceClass.
  InstanceClass::Fast this_class = Universe::new_instance_class(vtable_length,
                                           itable_size,
                                           itable_length,
                                           static_field_size,
                                           nonstatic_map_size+static_map_size,
                                           instance_size
                                           JVM_CHECK_0);

  // Fill in information already parsed.
  ClassInfo::Fast klass_info = this_class().class_info();
  this_class().set_super(&super_class);
  this_class().set_local_interfaces(&local_interfaces);
  klass_info().set_access_flags(access_flags);
  klass_info().set_constants(&cp);
  klass_info().set_fields(&fields);
  klass_info().set_methods(&methods);

  // Make sure we didn't set the JVM_ACC_ARRAY_CLASS flag, or else GC may be
  // messed up.
  GUARANTEE(!klass_info().access_flags().is_array_class(), "bad access flags");

  int i; // used locally in several loops
  const int len = methods().length();
  const int this_class_id = this_class().class_id();
  for (i = 0; i < len; i++) {
    Method::Raw m = methods().obj_at(i);
    m().set_holder_id(this_class_id);
  }
  if (!get_UseVerifier()) {
    StackmapGenerator::compress_verifier_stackmaps(&this_class JVM_CHECK_0);
  }
  klass_info().set_name(&class_name);

  // Fill in oop maps
  int offset = this_class().first_nonstatic_map_offset();
  for (i = 0; i < nonstatic_map_size; i++) {
    this_class().oop_map_at_put(offset++, nonstatic_map().ubyte_at(i));
  }
  for (i = 0; i < static_map_size; i++) {
    this_class().oop_map_at_put(offset++, static_map().ubyte_at(i));
  }

  // Additional attributes
  parse_classfile_attributes(&cp, &this_class JVM_CHECK_0);

  // By now we must have reached the exact end of the class file data:
  cpf_check_0(get_buffer_position() == _buffer->length(),
          inconsistent_classfile_size);

#if ENABLE_ROM_GENERATOR
  _total_classfile_bytes += get_buffer_position();
#endif

#if !ENABLE_ISOLATES 
  // can't do this if isolates are active since mirror not set up yet
  // Initialize static fields
  this_class().initialize_static_fields();
#endif
  // Setup up native code.
  if (has_native_methods || !UseROM) {
    Natives::register_natives_for(&this_class JVM_CHECK_0);
  }

  // Check if this klass has a vanilla default constructor
  if (!ROMIZED_PRODUCT && super_class.is_null()) {
    // java.lang.Object has empty default constructor
    klass_info().set_has_vanilla_constructor();
  } else {
    cpf_check_0(!is_circular(&this_class), circular_superclasses);
    bool status = super_class().check_access_by(&this_class, ErrorOnFailure 
                                                JVM_ZCHECK_0(status));
    if (super_class().has_vanilla_constructor()) {
      Method::Raw constructor = 
          this_class().find_local_method(Symbols::object_initializer_name(),
                                         Symbols::void_signature());
      if (constructor.not_null() && constructor().is_vanilla_constructor()) {
        klass_info().set_has_vanilla_constructor();
      }
    }
  }

  // initialized vtable & itable
  this_class().initialize_vtable();
  check_local_interfaces(&this_class JVM_CHECK_0);
  this_class().check_and_initialize_itable(JVM_SINGLE_ARG_CHECK_0);

  if (VerboseClassLoading) {
    TTY_TRACE(("[Loaded "));
    Symbol sym = this_class().name();
    sym.print_symbol_on(tty);
    TTY_TRACE_CR(("]"));
  }
  if (PrintLoadedClasses) {
    this_class.print();
    klass_info.print();
  }

#if ENABLE_JVMPI_PROFILE
  // Send the class load event to the profiler
  if(UseJvmpiProfiler && JVMPIProfile::VMjvmpiEventClassLoadIsEnabled()) {
    JVMPIProfile::VMjvmpiPostClassLoadEvent(
                  (InstanceClassDesc*)(this_class().obj()) JVM_CHECK_0);
  }
#endif

  remove_unused_utf8_entries(&cp);

  // completed loading of the top stack element
  stack->pop();
  state().set_result(&this_class);
  if (this_class().is_super()) { //see CR6552040
    resolve_invoke_special_virtual_conflicts(&this_class JVM_CHECK_0);
  }
  return this_class.obj();
}

int ClassFileParser::count_interfaces(OopDesc* local_interface_indices_obj,
                                      OopDesc* bitmap_obj) {
  TypeArray::Raw local_interface_indices = local_interface_indices_obj;
  TypeArray::Raw bitmap = bitmap_obj;
  const int length = local_interface_indices().length();
  int total_size = length;
  for(int i = 0; i < length; i++) {
    const int id = local_interface_indices().ushort_at(i);
    int bitmap_offset = id / BitsPerWord;
    const int bitmap_mask = 1 << (id % BitsPerWord);
    if (bitmap().int_at(bitmap_offset) & bitmap_mask) continue; //were here
    bitmap().int_at_put(bitmap_offset, bitmap().int_at(bitmap_offset) | bitmap_mask);
    InstanceClass::Raw cls = Universe::class_from_id(id);    
    total_size += count_interfaces(cls().local_interfaces(), bitmap_obj);
  }
  return total_size;
}
int ClassFileParser::found_all_interfaces(OopDesc* local_interface_indices_obj, 
                                          OopDesc* all_interface_indices_obj, 
                                          int already_in_table,
                                          OopDesc* bitmap_obj) {
  TypeArray::Raw local_interface_indices = local_interface_indices_obj;
  TypeArray::Raw all_interface_indices = all_interface_indices_obj;
  TypeArray::Raw bitmap = bitmap_obj;
  const int size = local_interface_indices().length();
  if (size != 0) {
    TypeArray::array_copy(&local_interface_indices, 0, &all_interface_indices,
                          already_in_table, size);
    already_in_table += size;  
    for (int i = 0; i < size; i++) {
      const int id = local_interface_indices().ushort_at(i);
      const int bitmap_offset = id / BitsPerWord;
      const int bitmap_mask = 1 << (id % BitsPerWord);
      if (bitmap().int_at(bitmap_offset) & bitmap_mask) continue; //were here
      bitmap().int_at_put(bitmap_offset, bitmap().int_at(bitmap_offset) | bitmap_mask);
      InstanceClass::Raw cls = Universe::class_from_id(id);    
      already_in_table = found_all_interfaces(cls().local_interfaces(), all_interface_indices_obj, 
                                              already_in_table, bitmap_obj);
    }
  }
  return already_in_table;
}

void ClassFileParser::update_fields(ConstantPool* cp, TypeArray* fields, 
                                    bool is_static, int next_offset,
                                    int prev_oop_offset, TypeArray* map,
                                    int& map_size, int map_index) {
  AllocationDisabler raw_pointers_used_in_this_function;
  jubyte *map_ptr = map->ubyte_base_address() + map_index;
  jubyte *map_start_ptr = map_ptr;

  int current_max = is_static ? T_BOOLEAN_byte_size : T_INT_byte_size;
  for (; current_max > 0; current_max /= 2) {
    jushort *field = fields->ushort_base_address();
    jushort *field_end = field + fields->length();

    for (; field < field_end; field += Field::NUMBER_OF_SLOTS) {
      int offset = field[Field::OFFSET_OFFSET];
      // Did we process field already?
      if (offset != 0) {
        // We process this field already?
        continue;
      }
      // Get flags
      AccessFlags access_flags;
      access_flags.set_flags(field[Field::ACCESS_FLAGS_OFFSET]);
      // Do we have the right "staticness"?
      if (access_flags.is_static() != is_static) {
        continue;
      }

      const int type_index = field[Field::SIGNATURE_OFFSET];
      FieldType::Raw type = cp->symbol_at(type_index);
      const int field_size = type().allocation_byte_size(is_static);

#ifndef PRODUCT
      if (type().basic_type() == T_OBJECT) {
        // Due to the current design of the VM, we cannot have static
        // final String fields in the bootstrap classes:
        // InstanceClass::initialize_static_fields() is called during
        // class parsing, but during bootstrap, the String class may
        // not yet be loaded.
        if (field[Field::INITVAL_OFFSET] != 0 && Universe::before_main()) {
          SHOULD_NOT_REACH_HERE();
        }
      }
#endif

      // Are we processing fields of this size
      if (field_size >= current_max) {
        // Compute real offset
        int field_offset = next_offset;
        next_offset += field_size;
        BasicType basic_type = type().basic_type();
        if (basic_type == T_OBJECT || basic_type == T_ARRAY) {
          // Compute delta in words from previous oop offset
          size_t word_delta = (field_offset - prev_oop_offset) / oopSize;
          GUARANTEE(word_delta > 0, "sanity check");
          while (word_delta >= OopMapEscape) {
            // Fill in escape value for ranges larger than 0x7f
            *map_ptr++ = OopMapEscape;
            word_delta -= (OopMapEscape - 1);
          }
          // Fill in oop map entry
          *map_ptr++ = (jubyte)word_delta;
          prev_oop_offset = field_offset;
        }
        // Set real offset
        field[Field::OFFSET_OFFSET] = (jushort)field_offset;
      }
    }
  }

  map_size += map_ptr - map_start_ptr;
  *map_ptr = OopMapSentinel;   // Terminate oop map
}

#ifndef PRODUCT

void ClassFileParser::p() {
#if USE_DEBUG_PRINTING
  print_on(tty);
#endif
}

void ClassFileParser::print_on(Stream *st) {
#if USE_DEBUG_PRINTING
  (void)st;
  tty->print_cr("ClassFileParser (StackObj) = 0x%x", (int)this);
  tty->print_cr("  _pos      = %d", get_buffer_position());
  tty->print_cr("  _buffer   = 0x%x", _buffer->obj());
  tty->print   ("  _name     = ");
  _name->print_symbol_on(tty);
  tty->print_cr("  _previous = 0x%x", _previous);

  // IMPL_NOTE: Need revisit LoaderContext*   _loader_ctx;
#endif
}

#endif
