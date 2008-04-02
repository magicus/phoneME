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

#include "incls/_precompiled.incl"
#include "incls/_Signature.cpp.incl"

HANDLE_CHECK(Signature, is_valid_method_signature(NULL))

// Returns the number of parameters in this signature, including the 
// implied <this> paramater for a virtual method.
jint Signature::parameter_word_size(bool is_static) {
  GUARANTEE(ObjectHeap::is_gc_active()||is_valid_method_signature(), "sanity");

  jint size = decode_ushort_at(0);
  if (!is_static) {
    size += 1;
  }
  return size;
}

BasicType Signature::return_type(bool fast) {
  GUARANTEE(ObjectHeap::is_gc_active() || is_valid_method_signature(),
            "sanity");
  juint chr = byte_at(2);
  if (chr < 0x80) {
    return primitive_field_basic_type_for(chr);
  } else if (fast) {
    return T_OBJECT;
  } else {
    return object_basic_type_at(2);
  }
}

ReturnOop Signature::return_type_symbol() {
  juint class_id = decode_ushort_at(2);
  JavaClass::Raw klass = Universe::class_from_id(class_id);
  return klass().name();
}

// This table records whether a character is letter, digit, or neither.
// letter_digit_table[i] == 2:  i is a digit
// letter_digit_table[i] == 1:  i is a letter
// letter_digit_table[i] == 0:  i is neither a letter nor a digit
const static jubyte letter_digit_table[] = {
  /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0,
  /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0,
  /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0,
  /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0, /* */ 0,
  /* */ 0, /*!*/ 0, /* */ 0, /*#*/ 0, /*$*/ 1, /*%*/ 0, /*&*/ 0, /*'*/ 0,
  /*(*/ 0, /*)*/ 0, /***/ 0, /*+*/ 0, /*,*/ 0, /*-*/ 0, /*.*/ 0, /* / */ 0,
  /*0*/ 2, /*1*/ 2, /*2*/ 2, /*3*/ 2, /*4*/ 2, /*5*/ 2, /*6*/ 2, /*7*/ 2,
  /*8*/ 2, /*9*/ 2, /*:*/ 0, /*;*/ 0, /*<*/ 0, /*=*/ 0, /*>*/ 0, /*?*/ 0,
  /*@*/ 0, /*A*/ 1, /*B*/ 1, /*C*/ 1, /*D*/ 1, /*E*/ 1, /*F*/ 1, /*G*/ 1,
  /*H*/ 1, /*I*/ 1, /*J*/ 1, /*K*/ 1, /*L*/ 1, /*M*/ 1, /*N*/ 1, /*O*/ 1,
  /*P*/ 1, /*Q*/ 1, /*R*/ 1, /*S*/ 1, /*T*/ 1, /*U*/ 1, /*V*/ 1, /*W*/ 1,
  /*X*/ 1, /*Y*/ 1, /*Z*/ 1, /*[*/ 0, /*\*/ 0, /*]*/ 0, /*^*/ 0, /*_*/ 1,
  /*`*/ 0, /*a*/ 1, /*b*/ 1, /*c*/ 1, /*d*/ 1, /*e*/ 1, /*f*/ 1, /*g*/ 1,
  /*h*/ 1, /*i*/ 1, /*j*/ 1, /*k*/ 1, /*l*/ 1, /*m*/ 1, /*n*/ 1, /*o*/ 1,
  /*p*/ 1, /*q*/ 1, /*r*/ 1, /*s*/ 1, /*t*/ 1, /*u*/ 1, /*v*/ 1, /*w*/ 1,
  /*x*/ 1, /*y*/ 1, /*z*/ 1, /*{*/ 0, /*|*/ 0, /*}*/ 0, /*~*/ 0, /* */ 0,
};

static inline bool is_letter_or_digit(const jchar ch) {
  // Simplification: we treat everything that is a non-ASCII unicode as
  // a letter instead of checking it carefully.
  // This is incorrect, but it spares footprint (and KVM does the same).
  return ch > 127 || letter_digit_table[ch] != 0;
}

static inline bool is_letter(const jchar ch) {
  return ch > 127 || letter_digit_table[ch] == 1;
}

bool Signature::skip_simple_identifier(Symbol* signature, int& position) {
  AZZERT_ONLY(verify_tables());

  int len = signature->length();
  utf8 ptr = signature->utf8_data();
  utf8 end1 = ptr + position; // end of the first loop;
  utf8 end2 = ptr + len;      // end of the second loop;

  const jubyte* const table = (jubyte*)letter_digit_table;

  // filter out non-UTF cases before position
  while (ptr < end1) {
    const unsigned char ch = *ptr++;

    // go by slow path if anything suspicious
    if (ch > 127 || ch == 0) {
      goto slow_path;
    }
  }

  // check that first symbol of identifier is
  // the letter
  if (ptr < end2) {
    const unsigned char ch = *ptr++;

    // go by slow path if anything suspicious
    if (ch > 127 || ch == 0) {
      goto slow_path;
    }
    if (table[ch] != 1) {
      return false;
    }
  } else {
    // Can't find anything past <position>!
    return false;
  }

  // really find end of identifier
  while (ptr < end2) {
    const unsigned char ch = *ptr++;

    // go by slow path if anything suspicious
    if (ch > 127 || ch == 0) {
      goto slow_path;
    }

    if (table[ch] == 0) { // not letter or digit
      ptr --;
      break;
    }
  }

  position = ptr - signature->utf8_data();
  return true;

slow_path:
  SymbolStream ss(signature);
  jchar ch = 0;
  int next_position = ss.get_next_jchar_from_utf8(position, &ch);
  if (next_position == UTF8Stream::UTF8_ERROR || !is_letter(ch)) {
    return false;
  }
  position = next_position; 
  while (position < len) {
    next_position = ss.get_next_jchar_from_utf8(position, &ch);
    if (next_position == UTF8Stream::UTF8_ERROR || !is_letter_or_digit(ch)) {
      return true;
    }
    position = next_position;
  }
  return true;
}

// Is this a class identifier that contains only ASCII? If the answer
// is no, you should call skip_class_identifier() instead.
bool Signature::is_class_identifier_quick(Symbol* signature) {
  AZZERT_ONLY(verify_tables());

  utf8 ptr = signature->utf8_data();
  utf8 end = ptr + signature->length();
  jubyte *table = (jubyte*)&letter_digit_table[0];
  unsigned char ch;

  while (ptr < end) {

    // (1) need one letter
    ch = *ptr++;
    if (ch > 127) {
      return false;
    } else if (table[ch] != 1) {
      return false;
    }

    // (2) need 0 or more digits
    while (ptr < end) {
      ch = *ptr++;
      if (ch <= 127) {
        if (table[ch] != 0) {
          // is a letter or digit, keep going
          continue;
        } else {
          if (ch == '/') {
            break;
          } else {
            return false;
          }
        }
      } else {
        return false;
      }
    }
  }

  if (ptr == end) {
    // This is a valid class name with only ASCII.
    return true;
  } else {
    // Consider whether this is a valid class name. Call skip_class_identifier
    // instead.
    return false;
  }
}

bool Signature::skip_class_identifier(Symbol* signature, int& position) {
  while (1) {
    if (!skip_simple_identifier(signature, position)) {
      return false;
    }
    if (position >= signature->length()) {
      return true;
    }
    if (signature->byte_at(position) != '/') {
      return true;
    }
    position++;
  }
}

SignatureStream::SignatureStream(Signature* signature,
                                 bool is_static, bool fast) {
  _signature = signature;
  _num_param_words = _signature->decode_ushort_at(0);
  _position = 2 + _signature->size_at(2);
  _word_index = 0;
  _num_param_words_processed = 0;
  _is_return_type = false;
  _fast = fast;                 // don't distinguish T_OBJECT, T_ARRAY

  if (is_static) {
    _type = T_VOID;
    next();
  } else {
    _type = T_OBJECT;
  }
}

void SignatureStream::next() {
  GUARANTEE(_num_param_words_processed <= _num_param_words, "sanity");

  _word_index += word_size();
  if (_num_param_words_processed < _num_param_words) {
    juint chr = (juint)_signature->byte_at(_position);
    if (chr < 0x80) {
      _type = TypeSymbol::primitive_field_basic_type_for(chr);
      _position ++;
    } else {
      _type = T_OBJECT;
      _current_class_id = _signature->decode_ushort_at(_position);
      if(!_fast) {
        JavaClass::Raw klass = Universe::class_from_id(_current_class_id);
        if (!klass.is_instance_class()) {
          _type = T_ARRAY;
        }
      }
      _position += 2;
    }
    _num_param_words_processed += word_size_for(_type);
  } else {
    _is_return_type = true;
    _type = _signature->return_type(_fast);
    _num_param_words_processed += 1; // because return type may be T_VOID!
  }
}

ReturnOop SignatureStream::type_symbol() {
  GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "sanity");
  JavaClass::Raw klass = Universe::class_from_id(_current_class_id);
  return klass().name();
}

#if ENABLE_JAVA_DEBUGGER || ENABLE_COMPILER_TYPE_INFO
ReturnOop SignatureStream::type_klass() {
  GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "sanity");
  return Universe::class_from_id(_current_class_id);
}
#endif

#ifndef PRODUCT

// Verify the correctness of the letter_digit_table[].
void Signature::verify_tables() {
  static bool verified = false;
  if (!verified) {
    for (jchar ch = 0; ch < 512; ch++) {
      if (('A' <= ch && ch <= 'Z') ||
          ('a' <= ch && ch <= 'z') || ch == '_' || ch == '$') {
        GUARANTEE(is_letter_or_digit(ch), "sanity");
        GUARANTEE(is_letter(ch),"sanity");
      } else if ('0' <= ch && ch <= '9') {
        GUARANTEE(is_letter_or_digit(ch), "sanity");
        GUARANTEE(!is_letter(ch),"sanity");
      } else if (ch >= 128) {
        GUARANTEE(is_letter_or_digit(ch), "sanity");
        GUARANTEE(is_letter(ch),"sanity");
      } else {
        GUARANTEE(!is_letter_or_digit(ch), "sanity");
        GUARANTEE(!is_letter(ch),"sanity");
      }
    }
    verified = true;
  }
}

#endif //!PRODUCT

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void Signature::print_parameters_on(Stream* st) {
  print_decoded_on(st);
}

void Signature::print_return_type_on(Stream* st) {
  print_type_at(st, 2);
}

#endif //!PRODUCT || ENABLE_TTY_TRACE
