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
# include "incls/_Symbol.cpp.incl"

HANDLE_CHECK(Symbol, is_symbol())

void Symbol::string_copy(char* buffer, int buffer_size) {
  GUARANTEE(length() < buffer_size, "buffer too small for string_copy");
  if (buffer_size <= 0) {
    return;
  }
  
  int i = 0;
  for (; i < buffer_size && i < length(); i++) {
    buffer[i] = byte_at(i);
  }

  if (i == buffer_size) {
    i = i - 1;
  }

  buffer[i] = 0;

  return;
}

juint Symbol::hash() {
  return SymbolTable::hash(utf8_data(), length());
}

bool Symbol::matches(Symbol *other_symbol) {
  return symbol()->matches(other_symbol->symbol());
}

int Symbol::strrchr(jbyte c) {
  int len = length();
  GUARANTEE(len >= 0, "sanity check");  
  // If we ever change jbyte to jubyte, check instead that c < 128
  GUARANTEE(0 <= c, "does not work for non-ASCII characters");
  // Skip backwards in string until 'c' is found or end is reached
  while(--len >= 0 && byte_at(len) != c)
    continue;
  return len;
}

bool Symbol::is_same_class_package(Symbol* other) {
  // The symbolOop's are in UTF8 encoding. Since we only need to check
  // explicitly for ASCII characters ('/', 'L', '['), we can keep them
  // in UTF8 encoding.
  // Otherwise, we just compare jbyte values between the strings.

  int end1 =        strrchr('/');
  int end2 = other->strrchr('/');
  
  if ((end1 < 0) || (end2 < 0)) {
    // One of the two doesn't have a package.  Only return true
    // if the other one also doesn't have a package.
    return end1 == end2; 
  }

  int start1 = 0;
  int start2 = 0;

  // Skip over '['s
  if (byte_at(start1) == '[') {
    do {
      start1++;
    } while (byte_at(start1) == '[');
    if (byte_at(start1) != 'L') {
      // Something is terribly wrong.  Shouldn't be here.
      return false;
    }
  }
  if (byte_at(start2) == '[') {
    do {
      start2++;
    } while (byte_at(start2) == '[');
    if (byte_at(start2) != 'L') {
      // Something is terribly wrong.  Shouldn't be here.
      return false;
    }
  }

  // Length must be the same
  if ((end1 - start1) != (end2 - start2)) return false;
  for (int i = 0; i < (end1 - start1); i++) {  
    if (byte_at(i + start1) != other->byte_at(i + start2)) return false;
  } 
  return true;  
}

void Symbol::print_symbol_on(Stream* st, bool dottified) {
  for (int index = 0; index < length(); index++) {
    jbyte b = byte_at(index);
    if (b >= 0) {
      const char *format = (b == '\\')             ?  "\\\\"
                         : (b == '\n')             ?  "\\n"
                         : (b == '\t')             ?  "\\t"
                         : (b == '/' && dottified) ?  "."
                         : (isprint(b))            ?  "%c"
                         :                            "\\x%02x";
      st->print(format, b);
    } else {
      static const char array[] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
      };
      juint i = ((juint)b) & 0xff;
      st->print("\\x%c%c", array[i/16], array[i%16]); 
    }
  }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  if ((is_valid_method_signature(NULL) || is_valid_field_type())
      && length() > 1) {
    TypeSymbol::Raw type_symbol = this->obj();
    st->print(" \"");
    type_symbol().print_decoded_on(st);
    st->print("\"");
  }
#endif

}

bool Symbol::is_valid_field_type() {
  OopDesc *obj = this->obj();
  if (ROM::is_valid_field_type(obj)) {
    return true;
  }

  if (ROM::system_contains(obj)) {
    // Is a symbol in ROM, but not a valid field type (or else
    // it would have fallen in the range checked by the previous block.)
    GUARANTEE(this->klass() != Universe::field_type_near()->obj(),
              "misplaced romized field type");
    return false;
  }

  if (this->klass() == Universe::field_type_near()->obj()) {
    return true;
  } else {
    if (length() == 1) {
      if (is_simple_type_symbol(byte_at(0))) {
        if (ObjectHeap::contains(obj)) {
          // Cache the result, so it will be faster on the next call
          set_klass(Universe::field_type_near());
        }
        return true;
      }
    }
  }

  return false;
}

bool Symbol::is_valid_method_signature(Symbol* name) {
  OopDesc * obj = this->obj();
  if (ROM::is_rom_symbol(obj)) {
    if (ROM::is_valid_method_signature(obj)) {
      // continue below
    } else {
      return false;
    }
  } else { 
    if (this->klass() != Universe::method_signature_near()->obj()) {
      return false;
    }
  }

  if (name != NULL && name->byte_at(0) == '<') {
    // All internal methods must return void:
    Signature::Raw sig = this->obj();
    return (sig().return_type() == T_VOID);
  } else {
    return true;
  }
}

bool Symbol::is_valid_field_name() {
  int position = 0;
  if (!Signature::skip_simple_identifier(this, position)) {
    return false;
  }
  return position == length();
}

bool Symbol::is_valid_class_name() {
  if (Signature::is_class_identifier_quick(this)) {
    return true;
  }
  if (is_valid_field_type()) {
    // All the field names of length 1 are also valid class names
    if (length() == 1) {
      return true;
    }
    // A classfile may use a valid array signature to refer to a class. E.g.,
    // newarray "[[Ljava/lang/String;"
    FieldType::Raw ft = this->obj();
    return (ft().basic_type() == T_ARRAY);
  }
  int position = 0;
  if (!Signature::skip_class_identifier(this, position)) {
    return false;
  }
  return position == length();
}

bool Symbol::is_valid_method_name() {
  // is_valid_field_name() is the most common case, so do it first
  if (is_valid_field_name() ||
      equals(Symbols::object_initializer_name()) ||
      equals(Symbols::class_initializer_name())) {
    return true;
  } else {
    return false;
  }
}

#ifndef PRODUCT

void Symbol::iterate(OopVisitor* visitor) {
  Oop::iterate(visitor);

  NamedField field("length", true);
  visitor->do_ushort(&field, length_offset(), true);
}

#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void Symbol::print_value_on(Stream* st) {
  st->print("#");
  print_symbol_on(st);
}
#endif

static inline bool is_c_identifier_char(jbyte c) {
  if ('0' <= c && c <= '9') {
    return true;
  }
  if ('A' <= c && c <= 'Z') {
    return true;
  }
  if ('a' <= c && c <= 'z') {
    return true;
  }
  if (c == '_') {
    return true;
  }

  return false;
}

void Symbol::put_unicode_char(utf8 &p, jchar ch) {
  if ((ch != 0) && (ch <=0x7f)) {
    *p++ = (jubyte) ch;
  } else if (ch <= 0x7FF) {
    /* 11 bits or less. */
    unsigned char high_five = (jubyte) (ch >> 6);
    unsigned char low_six = (jubyte) (ch & 0x3F);
    *p++ = (char) (high_five | 0xC0); /* 110xxxxx */
    *p++ = (char) (low_six   | 0x80); /* 10xxxxxx */
  } else {
    /* possibly full 16 bits. */
    char high_four = (char) (ch >> 12);
    char mid_six = (char) ((ch >> 6) & 0x3F);
    char low_six = (char) (ch & 0x3f);
    *p++ = (char) (high_four | 0xE0); /* 1110xxxx */
    *p++ = (char) (mid_six   | 0x80); /* 10xxxxxx */
    *p++ = (char) (low_six   | 0x80); /* 10xxxxxx */
  }
}

ReturnOop Symbol::copy_string_to_byte_array(OopDesc* str, bool slashify JVM_TRAPS) {
  // Speculatively allocate 4 bytes per jchar, in any case more than
  // the maximum possible space needed for UTF-8 conversion.
  UsingFastOops fast_oops;
  String::Fast string = str;
  int bytes_len = string().count()*4;
  TypeArray::Fast byte_array = Universe::new_byte_array(bytes_len JVM_CHECK_0);
  
  utf8 start = (utf8)byte_array().base_address();
  utf8 to = start;

  TypeArray::Fast jchar_array = string().value();
  jchar *from = (jchar *) jchar_array().base_address();
  from += string().offset();

  for( jchar* const end = from + string().count(); from < end; from++) {
    jchar c = *from; 
    if( slashify ) {
      switch( c ) {
        case '.': c = '/'; break;
        case '/': c = '.'; break;
      }
    }
    put_unicode_char(to, c);
   }
  int utf8_len = to - start;
  TypeArray::Fast result = Universe::new_byte_array(utf8_len JVM_CHECK_0);
  TypeArray::array_copy(&byte_array, 0, &result, 0, utf8_len);  
  return result.obj();
}


#if ENABLE_ROM_GENERATOR

// This symbol is the name of a native method. Print the name of
// the native method in as it would appear in a C source file.
void Symbol::print_as_c_source_on(Stream* st) {
  for (int index = 0; index < length(); index++) {
    jbyte b = byte_at(index);
    const char *format = (b == '\\')             ?  "\\\\"
                       : (b == '\n')             ?  "\\n"
                       : (b == '\t')             ?  "\\t"
                       : (b == '\"')             ?  "\\\""
                       : is_c_identifier_char(b) ?  "%c"
                       :                            "_%03o";
    st->print(format, b);
  }
}

// Print the content as a "C" character array. This is used by the
// romizer -- some C compilers may be recognize arbitrary characters
// in C literal strings. E.g., if we have the byte 0xab in a Symbol,
// it's not safe to print it as "foo\xab", or "foo[byte value of 0xab]".
void Symbol::print_as_c_array_on(Stream* st) {
  for (int index = 0; index < length(); index++) {
    jubyte b = (jubyte)byte_at(index);
    const char *format = (b == '\\')             ?  "'\\\\'"
                       : (b == '\n')             ?  "'\\n'"
                       : (b == '\t')             ?  "'\\t'"
                       : (b == '\"')             ?  "'\\\"'"
                       : (isprint(b))            ?  "'%c'"
                       :                            "(char)0x%x";
    st->print(format, b);
    st->print(",");
  }
}

// generate a map of all the field types in this object
int Symbol::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  // near
  map_index = Near::generate_fieldmap(field_map);

  //_length
  field_map->byte_at_put(map_index++, T_SHORT);

  // map the symbol chars
  jint chars_map_size = length();
  if (map_index + chars_map_size > field_map->length()) {
    return map_index + chars_map_size;
  }
  
  for (int i = 0; i < chars_map_size; i++) {
      field_map->byte_at_put(map_index++, T_BYTE);
  }

  return map_index;
}
#endif /* #if ENABLE_ROM_GENERATOR */

#if ENABLE_ROM_GENERATOR
bool Symbol::eq(char *str) {
  if (length() != jvm_strlen(str)) {
    return false;
  }
  for (int index = 0; index < length(); index++) {
    char b = (char)byte_at(index);
    if (b != str[index]) {
      return false;
    }
  }

  return true;
}
#endif /* #if ENABLE_ROM_GENERATOR  */

#if ENABLE_ROM_GENERATOR || ENABLE_PERFORMANCE_COUNTERS
// 1 if s1 > s2; 0 if the same; -1 if s1 < s2
jint Symbol::compare_to(Symbol *s1, Symbol *s2) {
  int len1 = s1->length();
  int len2 = s2->length();
  int len = (len1 < len2) ? len1 : len2;
  
  for (int i=0; i<len; i++) {
    jbyte b1 = s1->byte_at(i);
    jbyte b2 = s2->byte_at(i);
  
    if (b1 > b2) {
      return 1;
    } else if (b1 < b2) {
      return -1;
    }
  }
  
  if (len1 > len2) {
    return 1;
  } else if (len1 < len2) {
    return -1;
  } else {
    return 0;
  }
}
#endif /* ENABLE_ROM_GENERATOR || ENABLE_PERFORMANCE_COUNTERS */
