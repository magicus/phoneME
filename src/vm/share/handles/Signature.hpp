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
 * Represents method and field signatures.
 */
class Signature : public TypeSymbol {
public:
  HANDLE_DEFINITION_CHECK(Signature, TypeSymbol);
  ~Signature() {}
  // Returns the parameter size (in words) for a method call based
  // static-ness and signature
  jint parameter_word_size(bool is_static);

  // Returns the return type for a signature
  BasicType return_type(bool fast = false);
  ReturnOop return_type_symbol();

  static bool skip_simple_identifier(Symbol* signature, int& position);
  static bool is_class_identifier_quick(Symbol* signature);
  static bool skip_class_identifier(Symbol* signature, int& position);

  static BasicType parse_type(Symbol* signature, int& position);

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_return_type_on(Stream* st);
  void print_parameters_on(Stream* st);
#endif

private:

  static void verify_tables() PRODUCT_RETURN;
  
};

/**
 * SignatureStream streams over:
 *    the receiver (if not static),
 *    the arguments, and
 *    the return type
 *
 * Usage:
 *  for (SignatureStream ss(signature, is_static; !ss.eos(); ss.next()) {
 *    .. now we can use ss.index(), ss.type(), and ss.size()
 *  }
 *  .. ss.type() and ss.size() now yield return_type information.
 */
class SignatureStream : public StackObj {
 public:
  // Constructor for stream takes signature and static-ness.
  SignatureStream(Signature* signature, bool is_static, bool fast=false);
  
  // Advance to the element in stream
  void next();

  // Tells whether we are at the end of the stream
  bool eos() {
    return _num_param_words_processed > _num_param_words;
  }

  // Tells whether we are processing the return type
  bool is_return_type() {
    return _is_return_type;
  }

  // Returns the word index of the current parameter
  jint index() {
    return _word_index;
  }

  // Returns the basic type of the current parameter
  BasicType type() {
    return _type;
  }

  // Returns a string categorizing the type of the current parameter concisely
  const char* type_string();

  // Returns the current parameter's type as a symbol
  // The qualifier separator used here is '/'
  ReturnOop type_symbol();

  // Returns the word size of the current parameter
  jint word_size() {
    GUARANTEE(T_BOOLEAN <= type() && type() <= T_VOID, "sanity");
    jint size = word_size_for(type());
    GUARANTEE(size == T_VOID_word_size || size == 1 || size == 2, "sanity");
    return size;
  }
#if ENABLE_JAVA_DEBUGGER || ENABLE_COMPILER_TYPE_INFO
  ReturnOop type_klass();
#endif

#ifndef PRODUCT
  // Prints 'type_symbol()' on 'st' replacing '/' by '.'
  void print_dotted_type_symbol_on(Stream* st JVM_TRAPS);
#endif

 private:
  Signature* _signature;
  jint       _num_param_words;
  jint       _num_param_words_processed;
  jint       _current_class_id;
  jint       _word_index;
  jint       _position;
  BasicType  _type;
  bool       _fast;
  bool       _is_return_type;

  static const jbyte _word_size_for_parameter_type[];
};
