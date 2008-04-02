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
# include "incls/_CharacterStream.cpp.incl"

#ifndef PRODUCT

void CharacterStream::print_on(Stream* st) {
  reset();
  int len = length();
  while (len-- > 0) {
    st->put((char) read());
  }
}

#endif

#ifndef PRODUCT

bool CharacterStream::matches(CharacterStream* other) {
  other->reset();
  int len = length();
  if (len != other->length()) {
    return false;
  }
  reset();
  while (len-- > 0) {
    if (read() != other->read()) {
      return false;
    }
  }
  return true;
}
#endif

jint UTF8Stream::get_next_jchar_from_utf8(int index, jchar* value) {
  if (index < _utf8_start || index >= _utf8_start + _utf8_length) {
     return UTF8_ERROR;
  }
  unsigned char ch1 = byte_at(index);
  index++;
  if (ch1 == '\0') {
    return UTF8_ERROR;
  }
  if (ch1 >= 128) {
    return get_next_jchar_slow(index, ch1, value);
  } else {
    *value = ch1;
    return index;
  }
}

/** Given a utf8 character stream, return the jchar value that
 * corresponds to the byte(s) at index.
 */
jint UTF8Stream::get_next_jchar_slow(int index,
                                     unsigned char ch1, jchar* value) {
  GUARANTEE(ch1 >= 128, "sanity");

  switch (ch1 >> 4) {
  case 0x8: case 0x9: case 0xA: case 0xB: case 0xF:
    return UTF8_ERROR;
  case 0xC: case 0xD:    
    /* 110xxxxx  10xxxxxx */
    if (index < _utf8_start + _utf8_length) {
      unsigned char ch2 = byte_at(index);
      index++;
      if ((ch2 & 0xC0) == 0x80) {
        unsigned char high_five = (jubyte) (ch1 & 0x1F);
        unsigned char low_six = (jubyte) (ch2 & 0x3F);
        *value = (jchar) ((high_five << 6) + low_six);
        return index;
      }
    }
    return UTF8_ERROR;
  case 0xE:
    /* 1110xxxx 10xxxxxx 10xxxxxx */
    if (index < _utf8_start + _utf8_length) {
      unsigned char ch2 = byte_at(index);
      index++;
      if ((ch2 & 0xC0) == 0x80 && index < _utf8_start + _utf8_length) {
        unsigned char ch3 = byte_at(index);
        if ((ch3 & 0xC0) == 0x80) {
          index++;
          unsigned char high_four = (jubyte) (ch1 & 0x0f);
          unsigned char mid_six = (jubyte) (ch2 & 0x3f);
          unsigned char low_six = (jubyte) (ch3 & 0x3f);
          *value = (jchar) ((((high_four << 6) + mid_six) << 6) + low_six);
        } else {
          unsigned char high_four = (jubyte) (ch1 & 0x0f);
          unsigned char low_six = (jubyte) (ch2 & 0x3f);
          *value = (jchar) ((high_four << 6) + low_six);
        }
        return index;
      }
    }
    return UTF8_ERROR;
  default:
    break;
  }

  *value = ch1;
  return index;
}

jint UTF8Stream::length() {
  int result = 0;
  jchar value;
  int index = _utf8_start;
  while (index < _utf8_start + _utf8_length) {
    index = get_next_jchar_from_utf8(index, &value);
    if (index == UTF8_ERROR) {
      return UTF8_ERROR;
    }
    result++;
  }
  return result;
}

bool UTF8Stream::is_valid() {
  return length() != UTF8_ERROR;
}

jchar UTF8Stream::raw_read() {
  jchar value;
  jint index = get_next_jchar_from_utf8(_utf8_index, &value);
  if (index == UTF8_ERROR) {
    _utf8_index = _utf8_index + 1; // make progress somehow
  } else {
    _utf8_index = index;
  }
  return value;
}

#if !defined(PRODUCT) || ENABLE_JAVA_DEBUGGER

jint UTF8Stream::utf8_write(jint index, jchar ch) {
  if ((ch != 0) && (ch <=0x7f)) {
    byte_at_put(index, (jubyte) ch);
    return index + 1;
  }

  if (ch <= 0x7FF) { 
    /* 11 bits or less. */
    unsigned char high_five = (jubyte) (ch >> 6);
    unsigned char low_six = (jubyte) (ch & 0x3F);
    byte_at_put(index + 0, (jbyte) (high_five | 0xC0)); /* 110xxxxx */
    byte_at_put(index + 1, (jbyte) (low_six | 0x80)); /* 10xxxxxx */
    return index + 2;
  }
  /* possibly full 16 bits. */
  char high_four = (char) (ch >> 12);
  char mid_six = (char) ((ch >> 6) & 0x3F);
  char low_six = (char) (ch & 0x3f);
  byte_at_put(index + 0, (jbyte) (high_four | 0xE0)); /* 1110xxxx */
  byte_at_put(index + 1, (jbyte) (mid_six   | 0x80)); /* 10xxxxxx */
  byte_at_put(index + 2, (jbyte) (low_six   | 0x80)); /* 10xxxxxx */
  return index + 3;
}
#endif

#ifndef PRODUCT

int UnicodeStream::utf8_size(jchar c) {
  if ((0x0001 <= c) && (c <= 0x007F)) return 1;
  if (c <= 0x07FF) return 2;
  return 3;
}

int UnicodeStream::utf8_length() {
  int result = 0;
  for (int index = _unicode_start; 
       index < _unicode_start + _unicode_length;
       index++) {
    result += utf8_size(char_at(index));
  }
  return result;
}

jint ConcatenatedStream::length() {
  return _stream1_length + _stream2_length;
}

jint ConcatenatedStream::utf8_length() {
  return _stream1->utf8_length() + _stream2->utf8_length();
}

void ConcatenatedStream::reset() {
  _stream1->reset();
  _stream2->reset();
  _index = 0;
}

jchar ConcatenatedStream::raw_read() {
  if (_index < _stream1_length) {
    jchar result = _stream1->read();
    _index++;
    return result;
  } else if (_index < _stream1_length + _stream2_length) {
    jchar result = _stream2->read();
    _index++;
    return result;
  }
  return 0;
}

#endif // !PRODUCT
