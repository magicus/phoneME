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
# include "incls/_String.cpp.incl"

bool String::matches(String *that_string) {
  if (this->count() != that_string->count()) {
    return false;
  }
  TypeArray::Raw this_array = this->value();
  TypeArray::Raw that_array = that_string->value();
  address this_base = this_array().base_address();
  address that_base = that_array().base_address();
  this_base += sizeof(jchar) * this->offset();
  that_base += sizeof(jchar) * that_string->offset();

  if (jvm_memcmp(this_base, that_base, count() * 2) == 0) {
    return true;
  } else {
    return false;
  }
}

juint String::hash() {
  AllocationDisabler raw_pointers_used_in_this_function;

  juint value = 0;
  TypeArray::Raw char_array = this->value();
  jchar *ptr = (jchar*) char_array().base_address();
  ptr += this->offset();
  jchar *end = ptr + this->count();

  while (ptr < end) {
    juint chr = (juint)(*ptr);
    value = 31 * value + chr;
    ptr++;
  }
  return value;
}

ReturnOop String::to_cstring(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  int len = count();
  int off = offset();
  TypeArray::Fast ustring = value();
  TypeArray::Fast cstring = Universe::new_byte_array(len+1 JVM_CHECK_0);
  jushort *uptr = ((jushort*)ustring().base_address()) + off;
  jubyte  *cptr = (jubyte*)cstring().base_address();

  for (int i=0; i<len; i++) {
    jushort c = *uptr++;
    *cptr++ = (jubyte)c;
  }

  return cstring;
}

void String::print_string_on(Stream* st, int max_len) {
  UsingFastOops fast_oops;
  TypeArray::Fast t = value();
  int index = offset();
  int end_index = index + count();
  int max_index = -1;
  if (max_len >= 0) {
    max_index = index + 100;
  }
  for (; index < end_index; index++) {
    jchar c =  t().char_at(index);
    const char *format = (c == '"')                ? "\\\""
                       : (c <= 0xFF && isprint(c)) ? "%c"
                       : (c == '\n')               ? "\\n"
                       : (c == '\t')               ? "\\t"
                       :                           "\\u%04x";
    st->print(format, c);
    if (index == max_index) {
      st->print("...");
      break;
    }
  }
}

jchar String::char_at(int index) {
  TypeArray::Raw ta = value();
  return ta().char_at(index + offset());
}

jint String::last_index_of(jchar ch, jint fromIndex) {
  if (count() == 0 || fromIndex < 0) {
    return -1;
  }

  if (fromIndex >= count()) {
    fromIndex = count() - 1;
  }
   
  TypeArray::Raw array = value();
  const jchar* const base = (jchar*)array().base_address() + offset();

  // Pointer to the start point of the search.
  const jchar* p = base + fromIndex;

  do {
    if (*p == ch) {
      return p - base;
    }
  } while (--p >= base);

  return -1;
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void String::print_value_on(Stream* st) {
  TypeArray::Raw t = value();
  if (t.is_null()) {
    st->print("(uninitialized)");
  } else {
    st->print("String \"");
    print_string_on(st, 100);
    st->print("\"");
  }
}


#endif
