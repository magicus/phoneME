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
#include "incls/_TypeSymbol.cpp.incl"

HANDLE_CHECK(TypeSymbol, is_symbol())

/*-----------------------------------------------------------------------------
 * Encoding of a FieldType (primitive type):
 * Signature::_obj ->  [near : 4 bytes] the near pointer
 *                     [blen : 2 bytes] always equals 1
 *                     [type : 1 byte ] one of 'B','C','D','F','I','J','S','Z'
 *
 * Encoding of a FieldType (non-primitive):
 * Signature::_obj ->  [near : 4 bytes] the near pointer
 *                     [blen : 2 bytes] always equals 2
 *                     [type : 2 bytes] type
 *
 * Encoding of a Signature (method type):
 * Signature::_obj ->  [near : 4 bytes] the near pointer
 *                     [blen : 2 bytes] total number of bytes that follow
 *                     [pwords : 2 bytes] word size of parameters (encoded)
 *                     [type:  1 or 2 byte] return_type
 *                     [p1:    1 or 2 byte] parameter 1
 *                     [p2:    1 or 2 byte] parameter 2
 *                     ....
 *                     [pn:    1 or 2 byte] parameter n
 *
 * near:   = Universe::field_type_near for valid field signatures
 *         = Universe::method_signature_near for valid method signatures
 *
 * blen:   The total number of bytes that follows blen
 * pwords: (method signature only) The number of words occupied by
 *         the method's declared parameters. A long/double parameter
 *         counts as two. The implicit <this> pointer is not counted,
 *         since method signatures do not specify staticness.
 * type:   For field signatures, this is the field type
 *         For method signatures, this is the return type
 * p1:     1st declared parameter for method signature
 * ....
 * pn:     n-th declared parameter for method signature
 *
 * Each of type and pn may be 1 or 2 bytes:
 *       - basic type = 1 byte (one of 'B','C','D','F','I','J','S','Z','V')
 *       - obj/array type = 2 byte: encoded class_id of the JavaClass that
 *         represents this type.
 *
 * Note: for compatibility between the Signatures for primitive and
 * non-primitive types, we use only the lower 7 bits of the two bytes
 * in <pwords> and the 2-byte variants of <type> and <pn>. The highest
 * bit of the two bytes are always set to 1.
 *
 * E.g., an encoded value 0b1000:0010:1000:0001 in pwords is decoded as
 *                        0b0000:0001:0000:0001
 *-----------------------------------------------------------------------------
 */

static inline void throw_class_format_error(JVM_SINGLE_ARG_TRAPS) {
  Throw::class_format_error(invalid_signature JVM_THROW);
}

#define ts_check(b) \
  if (!(b)) { \
    throw_class_format_error(JVM_SINGLE_ARG_THROW); \
    return; \
  }

#define ts_check_0(b) \
  if (!(b)) { \
    throw_class_format_error(JVM_SINGLE_ARG_THROW_0); \
    return 0; \
  }

TypeSymbol::ParseStream* TypeSymbol::ParseStream::_list = NULL;

void TypeSymbol::ParseStream::gc_prologue() {
  for (ParseStream*p = _list; p; p=p->_next) {
    if (p->_heap_input != NULL) {
      if (VerboseGC) {
        TTY_TRACE_CR(("GC: Updating ParseStream relative pointers"));
      }
      int base = (int)(p->_heap_input->base_address());
      int cur  = (int)(p->_current_ptr);
      p->_current_ptr = (char*)(cur - base);

      // Mangle it so we can catch errors sooner
      AZZERT_ONLY(p->_start_ptr = (char*)0xffffffff);
      AZZERT_ONLY(p->_end_ptr = (char*)0x0);
    }
  }
}

void TypeSymbol::ParseStream::gc_epilogue() {
  for (ParseStream*p = _list; p; p=p->_next) {
    if (p->_heap_input != NULL) {
      char *base = (char*)(p->_heap_input->base_address());
      int pos = (int)p->_current_ptr;

      p->_start_ptr = base;
      p->_current_ptr = base + pos;
      p->_end_ptr = base + p->_len;
    }
  }
}

void TypeSymbol::ParseStream::initialize(utf8 stack_input, int len, 
                                         TypeArray * heap_input JVM_TRAPS) {
  _stack_input = stack_input;
  _len = len;
  _token_count = 0;
  _token_word_count = 0;
  _parameter_word_count = 0;
  _close_paren_position = -1;
  _heap_input = heap_input;

  char *base = (char*)stack_input;
  if (base == NULL) {
    base = (char*)heap_input->base_address();
  }

  _start_ptr = base;
  _current_ptr = base;
  _end_ptr = base + len;

  if (peek_char() == '(') {
    ts_check(len >= 3);
    next_char();
    _is_method_signature = true;
  } else {
    _is_method_signature = false;
  }
}

ReturnOop TypeSymbol::ParseStream::get_class(JVM_SINGLE_ARG_TRAPS) {
  int start_pos = pos();

  // (1) Get the class identifier. This is called a lot so let's cache
  //     the pointers in registers.
  char c = 0x0;
  char *cur = _current_ptr;
  char *end = _end_ptr;
  while (cur < end) {
    c = *cur ++;
    if (c == ';') {
      break;
    }
  }
  _current_ptr = cur;
  ts_check_0(c == ';');

  // (2) Get a handle to this class
  int name_len = pos() - start_pos - 1; // don't include ';'
  char *start = address_for_pos(start_pos);

  UsingFastOops fast_oops;
  Symbol::Fast class_name = SymbolTable::symbol_for(_heap_input, start,
                                                    name_len JVM_CHECK_0);
  ts_check_0(class_name().is_valid_class_name());
  InstanceClass::Fast klass = SystemDictionary::semi_resolve(&class_name
                                                             JVM_CHECK_0);
  return klass.obj();
}

void TypeSymbol::ParseStream::skip_optional_size() {
  while (_current_ptr < _end_ptr) {
    char c = next_char();
    if ('0' <= c && c <= '9') {
      // go on
    } else {
      -- _current_ptr;
      return;
    }
  }
}

ReturnOop TypeSymbol::ParseStream::get_array_class(JVM_SINGLE_ARG_TRAPS) {
  int dimension;

  for (dimension = 1; dimension < 256; dimension++) {
    skip_optional_size();
    if (_current_ptr >= _end_ptr) {
      goto error;
    }

    juint byte = next_char();
    switch (byte) {
    case 'L':
      {
        UsingFastOops fast_oops; 
        InstanceClass::Fast ic = get_class(JVM_SINGLE_ARG_CHECK_0);
        if (!UseROM && Universe::is_bootstrapping()) {
          Symbol::Raw class_name = ic().name();
          if (class_name.equals(Symbols::java_lang_Object())) {
            // We're still loading java.lang.Object and we need an Object
            // array.
            return Universe::object_array_class()->get_array_class(dimension
                                                                  JVM_CHECK_0);
          }
        }
        return ic().get_array_class(dimension JVM_NO_CHECK_AT_BOTTOM);
      }
    case '[':
      break;

    default:
      {
        int index;

        if ((byte >= 'B') && (byte <= 'Z') &&
            ((index = simple_type_symbol_table[byte - 'B']) != 0)) {
          index = Universe::bool_array_class_index + index - 1;
          TypeArrayClass* ac = (TypeArrayClass*)(&persistent_handles[index]);
          return ac->get_array_class(dimension JVM_NO_CHECK_AT_BOTTOM);
        } else {
          goto error;
        }
      }
    }
  }
error:
  throw_class_format_error(JVM_SINGLE_ARG_THROW_0);
}

juint TypeSymbol::ParseStream::next(JVM_SINGLE_ARG_TRAPS) {
  if (_current_ptr >= _end_ptr) {
    // finished
    return PARSE_END;
  }

  juint byte = next_char();

  if (byte == ')') {
    if (_close_paren_position == -1) {
      _close_paren_position = _token_count;
      _parameter_word_count = _token_word_count;
      byte = next_char();
    } else {
      throw_class_format_error(JVM_SINGLE_ARG_THROW_0);
    }
  }

  _token_count ++;
  _token_word_count ++;

  if (byte == 'V' || is_simple_type_symbol(byte)) {
    if (byte == 'J' || byte == 'D') {
      _token_word_count ++; // this token takes 2 words
    }
    return byte;
  }
  else if (byte == 'L') {
    InstanceClass::Raw ic = get_class(JVM_SINGLE_ARG_CHECK_0);
    return encode_ushort(ic().class_id());
  }
  else if (byte == '[') {
    ArrayClass::Raw ac = get_array_class(JVM_SINGLE_ARG_CHECK_0);
    jushort class_id = ac().class_id();
    // You'd hit the following if you put something like "([C[C)"
    // into Symbols.hpp, but forget to call 
    // register_java_class(char_array_class()) in Universe::genesis()
    GUARANTEE(ac.obj() == Universe::class_from_id(class_id),
              "VM has not been fully initialized");
    return encode_ushort(class_id);
  }
  else {
    throw_class_format_error(JVM_SINGLE_ARG_THROW_0);
  }
}

const jbyte TypeSymbol::_basic_type_from_literal[] = {
  /*A*/ T_ILLEGAL,
  /*B*/ T_BYTE,
  /*C*/ T_CHAR,
  /*D*/ T_DOUBLE,
  /*E*/ T_ILLEGAL,
  /*F*/ T_FLOAT,
  /*G*/ T_ILLEGAL,
  /*H*/ T_ILLEGAL,
  /*I*/ T_INT,
  /*J*/ T_LONG,
  /*K*/ T_ILLEGAL,
  /*L*/ T_ILLEGAL,
  /*M*/ T_ILLEGAL,
  /*N*/ T_ILLEGAL,
  /*O*/ T_ILLEGAL,
  /*P*/ T_ILLEGAL,
  /*Q*/ T_ILLEGAL,
  /*R*/ T_ILLEGAL,
  /*S*/ T_SHORT,
  /*T*/ T_ILLEGAL,
  /*U*/ T_ILLEGAL,
  /*V*/ T_VOID,
  /*W*/ T_ILLEGAL,
  /*X*/ T_ILLEGAL,
  /*Y*/ T_ILLEGAL,
  /*Z*/ T_BOOLEAN,
};

#define STACK_BUF_SIZE 1024

// Parse a TypeSymbol that consists of a single byte. The only valid
// inputs are the primitive field types (i.e., excludes "V").
ReturnOop TypeSymbol::parse_simple_field_type(char c JVM_TRAPS) {
  if (is_simple_type_symbol(c)) {
    Symbol::Raw result = SymbolTable::symbol_for((utf8)&c, 1 JVM_CHECK_0);
    if (ObjectHeap::contains((OopDesc*)result.obj())) {
      result.set_klass(Universe::field_type_near());
    }
    return result.obj();
  } else {
    throw_class_format_error(JVM_SINGLE_ARG_THROW_0);
  }
}

ReturnOop TypeSymbol::internal_parse(utf8 stack_input, int len, 
                                     TypeArray *heap_input
                                     JVM_TRAPS) {
  GUARANTEE(!ObjectHeap::contains((OopDesc*)stack_input),
            "heap pointer may move during parsing!");

  if (heap_input != NULL) {
    GUARANTEE(stack_input == NULL, "sanity");
    len = heap_input->length();
    if (len == 1) {
     return parse_simple_field_type(heap_input->byte_at(0) 
                                    JVM_NO_CHECK_AT_BOTTOM);
    }
  } else {
    GUARANTEE(stack_input != NULL, "sanity");
    if (len == 1) {
      return parse_simple_field_type(stack_input[0] JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  ts_check_0(len > 0);

  // Java allow at most 255 parameters, so buffer should be plenty big
  DECLARE_STATIC_BUFFER(jbyte, buffer, STACK_BUF_SIZE);
  GUARANTEE(STACK_BUF_SIZE > (255 * 2 + 20), "sanity");

  // To stress test the GC prologue/epilogue code in ParseStream, uncomment
  // the following line.
  //
  // ExcessiveGCBooster boost_while_in_parse_stream(true);

  ParseStream ps;
  ps.initialize(stack_input, len, heap_input JVM_CHECK_0);
  int out_pos = 4;
  int out_pos_limit = out_pos + 256 * 2;
  juint last_type = 0x0;
  for (;;) {
    GUARANTEE(out_pos < STACK_BUF_SIZE, "sanity");

    juint type = ps.next(JVM_SINGLE_ARG_CHECK_0);
    if (type == ParseStream::PARSE_END) {
      // done
      break;
    } else if (type < 128) {
      buffer[out_pos] = (jbyte)type;
      out_pos ++;
    } else {
      GUARANTEE((type & 0xc0000) == 0, "class_id overflow!");
      put_encoded_ushort((utf8)buffer, out_pos, type);
      out_pos += 2;
    }
    last_type = type;

    ts_check_0(out_pos < out_pos_limit);
  }

  int start_pos;
  if (ps.is_method_signature()) {
    juint parameter_words = (juint) ps.parameter_word_count();
    ts_check_0(ps.is_valid_method_signature() && (parameter_words <= 255));

    // Copy the return_type to the front of the signature.
    if (last_type < 128) {
      buffer[3] = (jbyte)last_type;
      start_pos = 1;
      out_pos --;
    } else {
      put_encoded_ushort((utf8)buffer, 2, last_type);
      start_pos = 0;
      out_pos -= 2;
    }

    // Set the number of words in parameters.
    put_encoded_ushort((utf8)buffer, start_pos,encode_ushort(parameter_words));
  } else {
    ts_check_0((ps.token_count() == 1) && (last_type != 'V'));
    start_pos = 4;
  }

  utf8 encoded_type = ((utf8)buffer) + start_pos;
  int encoded_len = out_pos - start_pos;
  Symbol::Raw result = SymbolTable::symbol_for(encoded_type, encoded_len 
                                               JVM_CHECK_0);
  if (ObjectHeap::contains((OopDesc*)result.obj())) {
    Near* klass;
    if (ps.is_method_signature()) {
      klass = Universe::method_signature_near();
    } else {
      klass = Universe::field_type_near();
    }
    result.set_klass(klass);
  }

  return result.obj();
}

// Called when creating a new object array
ReturnOop TypeSymbol::obj_array_class_name(ObjArrayClass *klass JVM_TRAPS) {
  DECLARE_STATIC_BUFFER(char, buffer, 2);
  put_encoded_ushort((utf8)buffer, 0, encode_ushort(klass->class_id()));

  Symbol::Raw result = SymbolTable::symbol_for((utf8)buffer, 2 JVM_CHECK_0);
  if (ObjectHeap::contains((OopDesc*)result.obj())) {
    result.set_klass(Universe::field_type_near());
  }

  return result.obj();
}

// Call by the verifier
//
// Returns the signature of a class that contains <element_class_name> as
// its element. E.g.
//
// encode("[I")       -> encode("[[I")
// encode("[[Z")      -> encode("[[[Z")
// "java/lang/Object" -> encode("[Ljava/lang/Object;")
ReturnOop TypeSymbol::obj_array_class_name(Symbol *element_class_name
                                           JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast klass;

  if (element_class_name->is_valid_field_type() &&
      element_class_name->length() > 1) {
    FieldType::Raw ft = element_class_name->obj();
    GUARANTEE(ft().basic_type() == T_ARRAY, "must be array type");
    klass = ft().object_type();
  } else {
    // Note: things like "B" are both valid field types and valid
    // class names, and must be processed in this block.
    klass = SystemDictionary::semi_resolve(element_class_name JVM_CHECK_0);
  }

  ArrayClass::Fast ac = klass().get_array_class(1 JVM_CHECK_0);
  return ac().name();
}

ReturnOop TypeSymbol::array_class_for_encoded_name(Symbol *encoded_name) {
  if (encoded_name->is_valid_field_type()) {
    FieldType::Raw ft = encoded_name->obj();
    if (ft().basic_type() == T_ARRAY) {
      return ft().object_type();
    }
  }

  // encoded_name does not represent an array class
  return NULL;
}

ReturnOop TypeSymbol::parse_array_class_name(Symbol *external_name JVM_TRAPS) {
  // The extra copy is slow, but it's called infrequently (by
  // Class.forName() or KNI_FindClass())
  UsingFastOops fast_oops;
  int length = external_name->length();
  TypeArray::Fast byte_array = Universe::new_byte_array(length JVM_CHECK_0);
  jvm_memcpy(byte_array().base_address(), external_name->base_address(),
             length);
  return parse(&byte_array JVM_NO_CHECK_AT_BOTTOM_0);
}

BasicType TypeSymbol::object_basic_type_at(int index) const {
  int klass_index = decode_ushort_at(index);
  OopDesc *klass = Universe::class_from_id(klass_index);  
  if (klass->is_instance_class()) {
    return T_OBJECT;
  } else {
    GUARANTEE(klass->is_type_array_class() || klass->is_obj_array_class(),
              "sanity");
    return T_ARRAY;
  }
}

#if !defined(PRODUCT) || ENABLE_WTK_PROFILER || ENABLE_PERFORMANCE_COUNTERS \
   || ENABLE_JVMPI_PROFILE || USE_AOT_COMPILATION || ENABLE_TTY_TRACE
   // Print the type at the given index
int TypeSymbol::print_type_at(Stream* st, int index) {
  juint byte0 = byte_at(index);
  if (byte0 < 128) {
    st->print("%c", byte0);
    return 1;
  } else {
    juint class_id = decode_ushort_at(index);
#if ENABLE_ISOLATES
    // If class_id is in ROM then this TypeSymbol is valid in 
    // current task.  If class_id >= ROM classes then this TypeSymbol
    // class is valid in the task it was defined in
    int task_id = ObjectHeap::owner_task_id( obj() );
    if( task_id == MAX_TASKS ) {
      task_id = TaskContext::current_task_id();
    }
    const TaskGCContext tmp(task_id);
#endif
    JavaClass::Raw klass = Universe::class_from_id(class_id);
    if (klass.is_instance_class()) {
      InstanceClass::Raw ic = klass().obj();
      Symbol::Raw name = ic().original_name();
      st->put('L');
      name().print_symbol_on(st);
      st->put(';');
    } else {
      for (;;) {
        if (klass.is_obj_array_class()) {
          st->put('[');
          ObjArrayClass::Raw oac = klass.obj();
          klass = oac().element_class();
        } else if (klass.is_type_array_class()) {
          st->put('[');
          TypeArrayClass::Raw tac = klass.obj();
          char c;
          switch (tac().type()) {
          case T_BOOLEAN:   c = 'Z'; break;
          case T_CHAR:      c = 'C'; break;
          case T_FLOAT:     c = 'F'; break;
          case T_DOUBLE:    c = 'D'; break;
          case T_BYTE:      c = 'B'; break;
          case T_SHORT:     c = 'S'; break;
          case T_INT:       c = 'I'; break;
          case T_LONG:      c = 'J'; break;
          default: SHOULD_NOT_REACH_HERE(); c = 0;
          }
          st->put(c);
          break;
        } else {
          GUARANTEE(klass.is_instance_class(), "sanity");
          InstanceClass::Raw ic = klass().obj();
          Symbol::Raw name = ic().original_name();
          st->put('L');
          if (name.not_null()) {
            name().print_symbol_on(st);
          } else {
            tty->print_cr("not defined yet!");
          }
          st->put(';');
          break;
        }
      }
    }
    return 2;
  }
}

void TypeSymbol::print_decoded_on(Stream* st) {
  if (is_valid_field_type()) {
    print_type_at(st, 0);
  } else {
    juint len = length();
    juint byte0 = byte_at(2);
    juint param_pos;
    if (byte0 < 128) {
      param_pos = 3;
    } else {
      param_pos = 4;
    }

    st->put('(');
    while (param_pos < len) {
      param_pos += print_type_at(st, param_pos);
    }
    st->put(')');
    print_type_at(st, 2);
  }
}

#endif
