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
public:
  enum Type {
    #define DEFINE_COMPILER_OBJECT_TYPE(name) COMPILER_OBJECT_TYPE(name),
    COMPILER_OBJECTS_DO( DEFINE_COMPILER_OBJECT_TYPE )
    #undef DEFINE_COMPILER_OBJECT_TYPE
    number_of_compiler_object_types
  };
private:
#if USE_COMPILER_OBJECT_HEADER
  enum {
    size_bits = 16,
    type_bits = 32 - size_bits,

    size_shift = 0,
    type_shift = size_bits,

    size_mask  = (1 << size_bits) - 1
  };

  unsigned header;

  static int size( const unsigned header ) {
    return header & size_mask;
  }
  static Type type( const unsigned header ) {
    return Type(header >> type_shift);
  }
  int  size( void ) const { return size( header ); }
  Type type( void ) const { return type( header ); }

  static unsigned make_header( const Type type, const int size ) {
    GUARANTEE( unsigned(type) < number_of_compiler_object_types, "sanity" );
    GUARANTEE( (size & ~size_mask) == 0, "Compiler object is too large" );
    return (type << type_shift) | size;
  }
  void set_header( const Type type, const int size ) {
    header = make_header( type, size );
  }

  static const char* const _names[];
  static const char* name( const Type type ) {
    GUARANTEE( unsigned(type) < number_of_compiler_object_types, "sanity" );
    return _names[ type ];
  }

  static CompilerObject* start ( void ) {
    return (CompilerObject*)_compiler_area_temp_top;
  }
  static CompilerObject* end ( void ) {
    return (CompilerObject*)_compiler_area_top;
  }

  static const CompilerObject* find( const void* ref );
  static const CompilerObject* next( const CompilerObject* p ) {
    return DERIVED( const CompilerObject*, p, p->size() );
  }
  const CompilerObject* next( void ) const { return next( this ); }
#endif
public:
  static CompilerObject* allocate( const Type type, const int size JVM_TRAPS ) {
    CompilerObject* p = (CompilerObject*)
      ObjectHeap::allocate_temp(size JVM_NO_CHECK);
    (void)type;
#if USE_COMPILER_OBJECT_HEADER
    if( p ) {
      p->set_header( type, size );
    }
#endif
    return p;
  }
};

#endif
