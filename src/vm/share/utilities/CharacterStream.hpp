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

/**
 * CharacterStream is used as a unified way to ready different kinds
 * of strings. The CharacterStream also makes sure no interior pointers
 * are created to the inside of strings.
 *
 * Hierarchy
 *
 * - CharacterStream     
 *   - UTF8Stream        
 *     - LiteralStream   "dfhjdjfd"
 *     - ByteStream      a TypeArray with jbytes
 *     - SymbolStream    an internal symbol
 *   - UnicodeStream
 *     - CharStream      a TypeArray with jchars
 */
class CharacterStream: public StackObj {
 protected:
  // Exchanges occurrences of ch1 and ch2 characters
  jchar translate(jchar value) {
    if (value == _ch1) return _ch2;
    if (value == _ch2) return _ch1;
    return value;
  }

 public:
  // Enable single character translation
  // Exchanges occurrences of ch1 and ch2 characters
  // Mainly for '.' -> '/' and reverse
  void set_translation(jchar ch1, jchar ch2) {
    _ch1 = ch1;
    _ch2 = ch2;
  }

  CharacterStream() { set_translation(0, 0); }

  jchar read() { return translate(raw_read()); }

  // Returns the unicode length
  virtual jint length()      JVM_PURE_VIRTUAL_0;

  // Returns the ut8f length
  virtual jint utf8_length() JVM_PURE_VIRTUAL_0;

  // Resets the stream
  virtual void reset()       JVM_PURE_VIRTUAL;

  // Does other match this stream?
  bool matches(CharacterStream* other);

#ifndef PRODUCT
  void print_on(Stream* st);
  void print() { print_on(tty); }
#endif

 private:
  virtual jchar raw_read() JVM_PURE_VIRTUAL_0;
  jchar _ch1;
  jchar _ch2;
};

class UTF8Stream : public CharacterStream {
 private:
  virtual void reset()         { _utf8_index = _utf8_start; }
  virtual jbyte byte_at(int /*index*/)                 JVM_PURE_VIRTUAL_0;
  virtual void byte_at_put(int /*index*/, jbyte /*value*/) JVM_PURE_VIRTUAL;

 protected:
  virtual int utf8_length()   { return _utf8_length; }
  int utf8_start()    { return _utf8_start; }

 public:
  UTF8Stream(jint utf8_start, jint utf8_length) : CharacterStream() { 
    _utf8_start  = utf8_start;
    _utf8_length = utf8_length;
    reset();
  }
  
  enum { UTF8_ERROR = -1 };

  // Returns 'UTF8_ERROR' if UTF8 format error detected.
  // Returns the index in this stream after scanning the unicode char 
  // starting at 'index',
  // The result char is returned in '*value'.
  jint get_next_jchar_from_utf8(int index, jchar* value);
  jint get_next_jchar_slow(int index, unsigned char ch1, jchar* value);
  // Returns whether this stream contains a string in valid UTF8 format
  bool is_valid();

  // Returns 'UTF8_ERROR' if UTF8 format error detected.
  // Returns unicode length.
  virtual jint length();

  jint utf8_write(jint index, jchar ch);

 private:
  virtual jchar raw_read();

  jint _utf8_index;
  jint _utf8_start;
  jint _utf8_length;
  friend class CharacterStream;
  friend class SymbolStream;
};

class LiteralStream : public UTF8Stream {
 private:
  virtual jbyte byte_at(jint index) { return (jbyte) _str[index]; }
  virtual void byte_at_put(int index, jbyte value) { _str[index] = value;  }
  char* _str;

 public:
  LiteralStream(const char* str, jint utf8_start, jint uft8_length) : UTF8Stream(utf8_start, uft8_length) {
    _str = (char*)str;
  }
  LiteralStream(const char* str) : UTF8Stream(0, jvm_strlen(str)) {
    _str = (char*)str;
  }
};

// UFT8ByteStream is over TypeArray[jbyte]

class ByteStream : public UTF8Stream {
 private:
  virtual jbyte byte_at(int index) { return _array->byte_at(index); }
  virtual void byte_at_put(int index, jbyte value) { _array->byte_at_put(index, value); }
  TypeArray* _array;
 public:
  ByteStream(TypeArray* array) : UTF8Stream(0, array->length()) {
    _array = array;
  }
  ByteStream(TypeArray* array, jint utf8_start, jint uft8_length): UTF8Stream(utf8_start, uft8_length) {
    _array = array;
  }
};

class SymbolStream : public UTF8Stream {
 private:
  jbyte byte_at_quick(int index) { return _symbol->byte_at(index); }
  virtual jbyte byte_at(int index)       { return byte_at_quick(index);    }
  virtual void byte_at_put(int index, jbyte value) { _symbol->byte_at_put(index, value); }
  Symbol* _symbol;

 public:
  SymbolStream(Symbol* symbol) : UTF8Stream(0, symbol->length()) {
    _symbol = symbol;
  }
  SymbolStream(Symbol* symbol, jint utf8_start, jint uft8_length): UTF8Stream(utf8_start, uft8_length) {
    _symbol = symbol;
  }

  // This is a quicker implementation than CharacterStream::next_unicode
  // by avoiding the virtual method call to byte_at().
  //
  // This is a special case to make Signature::skip_simple_identifier() run
  // faster, so as to improve class loading time.
  jint get_next_jchar_from_utf8(int index, jchar* value) {
    if (index < _utf8_start || index >= _utf8_start + _utf8_length) {
       return UTF8_ERROR;
    }
    unsigned char ch1 = byte_at_quick(index);
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

  friend class UTF8Stream;
  friend class CharacterStream;
};

#ifndef PRODUCT
class UnicodeStream : public CharacterStream {
 private:
  virtual jchar raw_read() { return char_at(_unicode_index++); }
  virtual void reset() { _unicode_index = _unicode_start; }

 public:
  virtual int length() { return _unicode_length; }

  UnicodeStream(jint unicode_start, jint unicode_length) : CharacterStream() { 
    _unicode_start  = unicode_start;
    _unicode_length = unicode_length;
    reset();
  }

 private:
  virtual jchar char_at(int index) JVM_PURE_VIRTUAL_1_PARAM_0(index);
  virtual int utf8_length();
  int utf8_size(jchar c);

 private:
  jint _unicode_index;
  jint _unicode_start;
  jint _unicode_length;
};

// These two classes are nice abstractions that waste space. They
// are used only in non-product builds
class CharStream : public UnicodeStream {
 public:
  CharStream(TypeArray* array) : UnicodeStream(0, array->length()) {
    _array  = array;
  }
  CharStream(TypeArray* array, jint unicode_start, jint unicode_length)
      : UnicodeStream(unicode_start, unicode_length) {
    _array  = array;
  }
  virtual jchar char_at(int index) { return (jchar) _array->char_at(index); }

 private:
  TypeArray* _array;
};

class ConcatenatedStream : public CharacterStream {
 public:
  ConcatenatedStream(CharacterStream* stream1, CharacterStream* stream2)
      : CharacterStream() {
    _stream1 = stream1;
    _stream2 = stream2;
    _stream1_length = stream1->length();
    _stream2_length = stream2->length();
    _index = 0;
  }
  
  virtual jint length();

  virtual jint utf8_length();

  virtual void reset();

 private:
  virtual jchar raw_read();
  CharacterStream* _stream1;
  CharacterStream* _stream2;
  jint _stream1_length;
  jint _stream2_length;
  jint _index;
};

#endif // !PRODUCT
