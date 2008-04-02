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

/*
 * GlobalDefinitions.cpp
 *
 * This file holds all globally used utility functions
 * that don't belong to any particular class.  The 
 * functions in this file are intended to be fully
 * portable across all ports.
 */

# include "incls/_precompiled.incl"
# include "incls/_GlobalDefinitions.cpp.incl"

// Support for 64-bit integer arithmetic

// The following code is mostly taken from JVM typedefs_md.h and system_md.c

#if UNDER_ADS
static const jlong  high_bit  = 0x8000000000000000ULL;
static const jlong other_bits = 0x7fffffffffffffffLL;
#else
static const jlong  high_bit  = (jlong)1 << (jlong)63;
static const jlong other_bits = ~high_bit;
#endif

#if CROSS_GENERATOR
extern "C" {
  // Note: CompilerJavaStackDirection is overrided in JVM.cpp if we're
  // building an AOT compiler for Jazelle.
  int JavaStackDirection = (ENABLE_INCREASING_JAVA_STACK) ? 1 : -1;
  int CompilerJavaStackDirection = (ENABLE_INCREASING_JAVA_STACK) ? 1 : -1;
}
#endif

/*
 * A non-zero in the following indicating that the corresponding ASCII
 * character, such as 'B', is a valid character that represents a simple
 * type in a Java method/field signature.
 *
 * Note that the non-zero values are from 1 through 8, so that you can
 * use it to index Universe::bool_array_class ~ Universe::long_array_class
 * See Universe.hpp for order of these array classes.
 */

const jubyte simple_type_symbol_table[] = {
  /* B */    5, // byte_array_class
  /* C */    2, // char_array_class
  /* D */    4, // double_array_class
  /* E */    0,
  /* F */    3, // float_array_class
  /* G */    0,
  /* H */    0,
  /* I */    7, // int_array_class
  /* J */    8, // long_array_class
  /* K */    0,
  /* L */    0,
  /* M */    0,
  /* N */    0,
  /* O */    0,
  /* P */    0,
  /* Q */    0,
  /* R */    0,
  /* S */    6,  // short_array_class
  /* T */    0,
  /* U */    0,
  /* V */    0,
  /* W */    0,
  /* X */    0,
  /* Y */    0,
  /* Z */    1, // bool_array_class
};

Tag basic_type2tag(BasicType type) {
  static const jubyte table[] = {
    /*  -xxx-      =  0 */ (jubyte) uninitialized_tag,
    /*  -xxx-      =  1 */ (jubyte) uninitialized_tag,
    /*  -xxx-      =  2 */ (jubyte) uninitialized_tag,
    /*  -xxx-      =  3 */ (jubyte) uninitialized_tag,
    /* T_BOOLEAN   =  4 */ (jubyte) int_tag,
    /* T_CHAR      =  5 */ (jubyte) int_tag,
    /* T_FLOAT     =  6 */ (jubyte) float_tag,
    /* T_DOUBLE    =  7 */ (jubyte) double_tag,
    /* T_BYTE      =  8 */ (jubyte) int_tag,
    /* T_SHORT     =  9 */ (jubyte) int_tag,
    /* T_INT       = 10 */ (jubyte) int_tag,
    /* T_LONG      = 11 */ (jubyte) long_tag,
    /* T_OBJECT    = 12 */ (jubyte) obj_tag,
    /* T_ARRAY     = 13 */ (jubyte) obj_tag,
    /* T_VOID      = 14 */ (jubyte) uninitialized_tag,
  };

  GUARANTEE(type >= T_BOOLEAN, "sanity");

  if ((juint)type <= T_VOID) {
    return (Tag)table[(int)type];
  } else {
    GUARANTEE(type == T_ILLEGAL, "sanity");
    return uninitialized_tag;
  }
}

const jubyte word_size_for_table[] = {
  /* unused      =  0: */ 0xff,
  /* unused      =  1: */ 0xff,
  /* unused      =  2: */ 0xff,
  /* unused      =  3: */ 0xff,
  /* T_BOOLEAN   =  4: */ 1,
  /* T_CHAR      =  5: */ 1,
  /* T_FLOAT     =  6: */ 1,
  /* T_DOUBLE    =  7: */ 2,
  /* T_BYTE      =  8: */ 1,
  /* T_SHORT     =  9: */ 1,
  /* T_INT       = 10: */ 1,
  /* T_LONG      = 11: */ 2,
  /* T_OBJECT    = 12: */ 1,
  /* T_ARRAY     = 13: */ 1,
  /* T_VOID      = 14: */ T_VOID_word_size,
  /* T_SYMBOLIC  = 15: */ T_SYMBOLIC_word_size,
};

int byte_size_for(BasicType type) {
  static const jubyte table[] = {
    /* unused      =  0: */ 0,
    /* unused      =  1: */ 0,
    /* unused      =  2: */ 0,
    /* unused      =  3: */ 0,
    /* T_BOOLEAN   =  4: */ T_BOOLEAN_byte_size,
    /* T_CHAR      =  5: */ T_CHAR_byte_size,
    /* T_FLOAT     =  6: */ T_FLOAT_byte_size,
    /* T_DOUBLE    =  7: */ T_DOUBLE_byte_size,
    /* T_BYTE      =  8: */ T_BYTE_byte_size,
    /* T_SHORT     =  9: */ T_SHORT_byte_size,
    /* T_INT       = 10: */ T_INT_byte_size,
    /* T_LONG      = 11: */ T_LONG_byte_size,
    /* T_OBJECT    = 12: */ T_OBJECT_byte_size,
    /* T_ARRAY     = 13: */ T_OBJECT_byte_size,
    /* T_VOID      = 14: */ T_VOID_byte_size,
#if ENABLE_ROM_GENERATOR
    /* T_SYMBOLIC  = 15: */ T_SYMBOLIC_byte_size,
#endif
  };

#ifdef AZZERT
#if ENABLE_ROM_GENERATOR
  const int max = T_SYMBOLIC;
#else
  const int max = T_VOID;
#endif
  GUARANTEE(type >= T_BOOLEAN && type <= max, "sanity");
#endif

  return table[(int)type];
}

const jubyte stack_type_for_table[] = {
  /* unused      =  0: */ 0,
  /* unused      =  1: */ 0,
  /* unused      =  2: */ 0,
  /* unused      =  3: */ 0,
  /* T_BOOLEAN   =  4: */ T_INT,
  /* T_CHAR      =  5: */ T_INT,
  /* T_FLOAT     =  6: */ T_FLOAT,
  /* T_DOUBLE    =  7: */ T_DOUBLE,
  /* T_BYTE      =  8: */ T_INT,
  /* T_SHORT     =  9: */ T_INT,
  /* T_INT       = 10: */ T_INT,
  /* T_LONG      = 11: */ T_LONG,
  /* T_OBJECT    = 12: */ T_OBJECT,
  /* T_ARRAY     = 13: */ T_OBJECT,
  /* T_VOID      = 14: */ T_VOID,
  /* unused      = 15: */ 0,

  /* unused      = 16: */ 0,
  /* unused      = 17: */ 0,
  /* unused      = 18: */ 0,
  /* unused      = 19: */ 0,
  /* unused      = 20: */ 0,
  /* unused      = 21: */ 0,
  /* unused      = 22: */ 0,
  /* unused      = 23: */ 0,
  /* unused      = 24: */ 0,
  /* unused      = 25: */ 0,
  /* unused      = 26: */ 0,
  /* unused      = 27: */ 0,
  /* unused      = 28: */ 0,
  /* unused      = 29: */ 0,
  /* unused      = 30: */ 0,
  /* unused      = 31: */ 0,
  /* T_ILLEGAL   = 32: */ T_ILLEGAL,
};

#ifndef PRODUCT
const char * name_for(BasicType type) {
  switch (type) {
  case T_BOOLEAN: return "boolean";
  case T_CHAR:    return "char";
  case T_FLOAT:   return "float";
  case T_DOUBLE:  return "double";
  case T_BYTE:    return "byte";
  case T_SHORT:   return "short";
  case T_INT:     return "int";
  case T_LONG:    return "long";
  case T_OBJECT:  return "object";
  case T_ARRAY:   return "array";
  case T_SYMBOLIC:return "symbolic";
  case T_VOID:    return "void";
  default:        return "???";
  }
}
#endif

void global_breakpoint() {
#if defined(__GNUC__)
#ifdef LINUX
    BREAKPOINT;
#else
    // IMPL_NOTE: move this into OS class
    panic();
#endif
#endif

#if defined(_MSC_VER)
#ifdef UNDER_CE
    DebugBreak(); // emit (undefined) instruction 0xE6000010
    JVM::exit(EXIT_FAILURE);
#else
    __asm {
      int 3
    }
#endif
#endif

}
#ifndef PRODUCT
jint global_check_count;
#endif

#ifdef AZZERT

void power_of_2_fatal() {
  JVM_FATAL(must_be_a_power_of_2);
}

void check_basic_types() {
  GUARANTEE( 4 == sizeof( jobject),   "wrong size for basic type");
  GUARANTEE( 1 == sizeof( jbyte),     "wrong size for basic type");
  GUARANTEE( 2 == sizeof( jchar),     "wrong size for basic type");
  GUARANTEE( 2 == sizeof( jshort),    "wrong size for basic type");
  GUARANTEE( 4 == sizeof( jint),      "wrong size for basic type");
  GUARANTEE( 1 == sizeof( jboolean),  "wrong size for basic type");
  GUARANTEE( 8 == sizeof( jlong),     "wrong size for basic type");
  GUARANTEE( 4 == sizeof( jfloat),    "wrong size for basic type");
  GUARANTEE( 8 == sizeof( jdouble),   "wrong size for basic type");
  GUARANTEE( 1 == sizeof( jubyte),    "wrong size for basic type");
  GUARANTEE( 2 == sizeof( jushort),   "wrong size for basic type");
  GUARANTEE( 4 == sizeof( juint),     "wrong size for basic type");

  Oop::verify_basic_properties();
}

#endif // AZZERT

#ifdef USE_LIBC_GLUE

// This is a bubble sort.  
// Slow, but easy to program.   And we only use it in debugging mode
void  jvm_slow_sort(void *base, int nmemb, int size,
                             int(*compar)(const void *, const void *)) { 
  address start = (address)base;
  address end   = start + nmemb * size;
  address i, j;
  for (i = start; i < end; i += size) { 
    for (j = i; j > start && compar((void*)(j-size), (void*)j) > 0; j -= size){ 
      // Swap the element pointed at by j, and the previous element
      for (int k = 0; k < size; k++) {
          char temp = j[k-size]; 
          j[k-size] = j[k];
          j[k] = temp;
      }
    }
  }
}

#endif

bool get_UseVerifier() {
#if ENABLE_ISOLATES
  if (!UseVerifier || Task::current()->is_null()) {
    return UseVerifier;
  }
  IsolateObj::Raw iso = Task::current()->primary_isolate_obj();
  if (iso.is_null()) {
    return UseVerifier;
  }
  return (iso().use_verifier() == 1 ? true : false);
#else
  return UseVerifier;
#endif
}

address ArrayPointer::base_address() {
  if (_heap_array == NULL) {
    return (address) _offset;
  } else {
    return _heap_array->base_address() + _offset;
  }
}
