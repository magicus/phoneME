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

/**
 * TypeSymbol - a subclass of Symbol used to represent field type and
 *              method signature.
 *
 * This is a base class for FieldType and Signature.
 */

class TypeSymbol: public Symbol {
public:
  HANDLE_DEFINITION_CHECK(TypeSymbol, Symbol);
  ~TypeSymbol() {}

  static ReturnOop parse(TypeArray *byte_array JVM_TRAPS) {
    return internal_parse(NULL, 0, byte_array JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop parse(const char *s JVM_TRAPS) {
    return internal_parse((utf8)s, jvm_strlen(s), NULL JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop obj_array_class_name(Symbol *element_class_name JVM_TRAPS);
  static ReturnOop obj_array_class_name(ObjArrayClass *klass JVM_TRAPS);

  // Call by SystemDictionary
  // E.g.: "java/lang/String" or encode("[[B")
  static ReturnOop array_class_for_encoded_name(Symbol *encoded_name);

  // Call by Class.forName or KNI_FindClass()
  // E.g.: "java/lang/String" or "[[B"
  static ReturnOop parse_array_class_name(Symbol *external_name JVM_TRAPS);

#if !defined(PRODUCT) || ENABLE_WTK_PROFILER || ENABLE_PERFORMANCE_COUNTERS \
     || ENABLE_JVMPI_PROFILE || USE_AOT_COMPILATION || ENABLE_TTY_TRACE
  void print_decoded_on(Stream* st);
  int print_type_at(Stream* st, int index);
#else
  void print_decoded_on(Stream* /*st*/) PRODUCT_RETURN;
  int print_type_at(Stream* /*st*/, int /*index*/) PRODUCT_RETURN0; 
#endif

  jushort decode_ushort_at_raw(jbyte *ptr) const {
    jushort hibyte = *ptr++;
    jushort lobyte = *ptr;

    GUARANTEE((hibyte & 0x80) != 0, "high bit must be set");
    GUARANTEE((lobyte & 0x80) != 0, "high bit must be set");

    return (jushort)(((hibyte & 0x7f) << 7) | (lobyte & 0x7f));
  }

  jushort decode_ushort_at(int index) const {
    jbyte *ptr = (jbyte*)base_address();
    return decode_ushort_at_raw(ptr + index);
  }

  static inline juint encode_ushort(juint value) {
    GUARANTEE((value & 0xffffc000) == 0, "sanity!");
    juint hibyte = (value & 0xff80) << 1;
    juint lobyte = value & 0x7f;
    return (hibyte | lobyte | 0x8080);
  }

  static inline void put_encoded_ushort(utf8 buffer, int i, juint value) {
    GUARANTEE((value & 0xffff8080) == 0x8080, "sanity!");
    juint hibyte = value >> 8;
    juint lobyte = value & 0xff;

    buffer[i]   = (jbyte)hibyte;
    buffer[i+1] = (jbyte)lobyte;
  }

  static BasicType primitive_field_basic_type_for(juint chr) {
    return (BasicType)(_basic_type_from_literal[chr - 'A']);
  }

protected:
  bool is_primitive_type_at(int index) const {
    juint byte0 = byte_at(index);
    return byte0 < 128;
  }
  int size_at(int index) const {
    if (is_primitive_type_at(index)) {
      return 1;
    } else {
      return 2;
    }
  }

  BasicType primitive_field_basic_type_at(int index) const {
    juint chr = (juint)byte_at(index);
    BasicType t = primitive_field_basic_type_for(chr);
    return t;
  }

  BasicType object_basic_type_at(int index) const;

  ReturnOop object_type_at(int index) const {
    int klass_index = decode_ushort_at(index);
    return Universe::class_from_id(klass_index);
  }

private:
  static ReturnOop internal_parse(utf8 s, int utflen, TypeArray *byte_array
                                  JVM_TRAPS);
  static ReturnOop parse_simple_field_type(char c JVM_TRAPS);

  static const jbyte _basic_type_from_literal[];

friend class SignatureStream;

public:
  class ParseStream {
  private:
    ParseStream * _next;            // Link-list of all active ParseStream,
                                    // Used by GC
    char* _start_ptr;                // Direct pointer to the beginning of
                                    // input. Must be updated explicitly
                                    // during GC.
    char* _current_ptr;              // Direct pointer to current input char.
                                    // Must be updated explicitly
                                    // during GC.
    char* _end_ptr;                  // Direct pointer to end of input. Must
                                    // be updated explicitly during GC.
    utf8 _stack_input;              // If non null, points to the beginning
                                    // of the input that lives in C stack
                                    // (non-movable)
    TypeArray * _heap_input;        // If non null, points to the beginning
                                    // of the input that lives in Java heap
                                    // (may move during GC)
    int _len;                       // Length of input.

    bool _is_method_signature;
    int _token_count;               // Number of token parsed. A token
                                    // is either a parameter type (regardless
                                    // of word size), or a return type.
    int _token_word_count;          // Total word size of token parsed.
                                    // A jdouble or jlong token have word size
                                    // of 2. All other tokens have word size
                                    // of 1.
    int _parameter_word_count;      // Total word size of parameters passed.
                                    // In contrast to _token_word_count, this
                                    // does not count the retutn type.
    int _close_paren_position;

    int pos() const {
      return (_current_ptr - _start_ptr);
    }

    char peek_char() const {
      GUARANTEE(pos() < _len, "read past end");
      return *_current_ptr;
    }

    char next_char() {
      GUARANTEE(pos() < _len, "read past end");
      return *_current_ptr++;
    }

    char * address_for_pos(int position) {
      GUARANTEE(position >= 0, "sanity past end");
      GUARANTEE(position < _len, "read past end");
      return _start_ptr + position;
    }

    void push() {
      _next = _list;
      _list = this;
    }
    void pop() {
      GUARANTEE(_list == this, "unbalanced pop");
      _list = _next;
    }

    static ParseStream * _list;

  public:
    enum {
      PARSE_END = 0xffffffff
    };
    ParseStream() {
      push();
    }
    void initialize(utf8 stack_input, int len, TypeArray * heap_input 
                    JVM_TRAPS);
    ReturnOop get_class(JVM_SINGLE_ARG_TRAPS);
    void skip_optional_size();
    ReturnOop get_array_class(JVM_SINGLE_ARG_TRAPS);
    juint next(JVM_SINGLE_ARG_TRAPS);

    bool is_method_signature() {
      return _is_method_signature;
    }

    bool is_valid_method_signature() {
      if (_token_count == _close_paren_position + 1) {
        return true;
      } else {
        return false;
      }
    }

    int token_count() {
      return _token_count;
    }
    int parameter_word_count() {
      return _parameter_word_count;
    }

    // The number of tokens that precede the first close parenthesis
    int close_paren_position() {
      return _close_paren_position;
    }

    ~ParseStream() {
      pop();
    }

    // We use GC prologue/epilogue to update hard pointers used by
    // peek_char() and next_char()
    static void gc_prologue();
    static void gc_epilogue();
  };
};
