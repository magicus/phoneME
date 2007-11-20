/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_COMPILER

#define COMPILER_OBJECTS_DO(template)\
  template( Entry )

#define COMPILER_OBJECT_TYPE(name) name##_type

#define USE_COMPILER_OBJECT_HEADER 1

class CompilerObject {
#if USE_COMPILER_OBJECT_HEADER
  enum {
    size_bits = 16,
    type_bits = 32 - size_bits,

    size_shift = 0,
    type_shift = size_bits,

    size_mask  = (1 << size_bits) - 1
  };
  unsigned header;

  static int get_size( const unsigned header ) {
    return header & size_mask;
  }
  static Type get_type( const unsigned header ) {
    return Type(header >> type_shift);
  }
  int  get_size( void ) const { return get_size( header ); }
  Type get_type( void ) const { return get_type( header ); }
#endif
public:
  enum Type {
    #define DEFINE_COMPILER_OBJECT_TYPE(name) COMPILER_OBJECT_TYPE(name),
    COMPILER_OBJECTS_DO( DEFINE_COMPILER_OBJECT_TYPE )
    #undef DEFINE_COMPILER_OBJECT_TYPE
    number_of_compiler_object_types
  };

  static CompilerObject* allocate( const Type type, const int size JVM_TRAPS ) {
    CompilerObject* p = (CompilerObject*)
      ObjectHeap::allocate_temp(size JVM_NO_CHECK);
#if USE_COMPILER_OBJECT_HEADER
    if( p ) {
      set_header( type, size );
    }
#endif
    return p;
  }
};

#endif
