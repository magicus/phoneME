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

#if ENABLE_COMPILER

#define COMPILER_OBJECTS_DO(template) \
  template( PointerArray            ) \
  template( ByteArray               ) \
  template( ShortArray              ) \
  template( IntArray                ) \
  template( Entry                   ) \
  template( CompilationQueueElement ) \
  template( VirtualStackFrame       ) \
  template( LiteralPoolElement      ) \
  template( CodeGenerator           )

#ifdef DEBUG
# define USE_COMPILER_OBJECT_HEADER 1
#else
# define USE_COMPILER_OBJECT_HEADER 0
#endif

class CompilerObject {
public:
  enum Type {
    #define DEFINE_COMPILER_OBJECT_TYPE(name) name##_type,
    COMPILER_OBJECTS_DO( DEFINE_COMPILER_OBJECT_TYPE )
    #undef DEFINE_COMPILER_OBJECT_TYPE
    number_of_compiler_object_types,
  };
private:
  PRODUCT_ONLY( CompilerObject( void ) {} )

  enum {
    align_bits = 2,
    align_mask = (1 << align_bits) - 1,
  };

  static int align_size( const int size ) {
    return (size + align_mask) & ~align_mask;
  }

#if USE_COMPILER_OBJECT_HEADER
  enum {
    size_bits = 24,
    type_bits = 32 - size_bits,

    size_shift = 0,
    type_shift = size_bits,

    size_mask  = (1 << size_bits) - 1
  };

  unsigned header;

protected:
  static int size( const unsigned header ) {
    return header & size_mask;
  }
  static Type type( const unsigned header ) {
    return Type(header >> type_shift);
  }
  int  size( void ) const { return size( header ); }
  Type type( void ) const { return type( header ); }

private:
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
  static CompilerObject* allocate( const Type type, int size JVM_TRAPS ) {
    size = align_size( size );
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

#if USE_COMPILER_OBJECT_HEADER
  bool is ( const Type t ) const { return this == NULL || type() == t; }
#else
  bool is ( const Type   ) const { return true; }
#endif
  
  void check_type( const Type t ) const {
    GUARANTEE( is( t ), "Wrong type" );
  }
};

#define COMPILER_OBJECT_ALLOCATE(type)\
  (type*) CompilerObject::allocate( \
  CompilerObject::type##_type, sizeof(type) JVM_NO_CHECK)


#if USE_COMPILER_OBJECT_HEADER
# define IMPLEMENT_COMPILER_ARRAY_BOUNDS_CHECK                \
  unsigned length( void ) const {                             \
    return (size() - header_size()) / sizeof(element_type);   \
  }                                                           \
  void check_bounds( const unsigned i ) const {               \
    GUARANTEE( this != NULL, "NULL array" );                  \
    GUARANTEE( i < length(), "Array index is out of bounds" );\
  }
#else //!USE_COMPILER_OBJECT_HEADER
# define IMPLEMENT_COMPILER_ARRAY_BOUNDS_CHECK                \
  static void check_bounds( const int ) {}
#endif

#define MAKE_COMPILER_ARRAY_NAME(ARRAY_TYPE) Compiler##ARRAY_TYPE##Array

#define DEFINE_COMPILER_ARRAY(ELEMENT_TYPE, ARRAY_TYPE) \
  class MAKE_COMPILER_ARRAY_NAME(ARRAY_TYPE): public CompilerObject {         \
    typedef ELEMENT_TYPE element_type;                                        \
    typedef MAKE_COMPILER_ARRAY_NAME(ARRAY_TYPE) array_type;                  \
    element_type _data[ 1 ];                                                  \
                                                                              \
    static int header_size( void ) { return FIELD_OFFSET(array_type, _data); }\
                                                                              \
    IMPLEMENT_COMPILER_ARRAY_BOUNDS_CHECK                                     \
                                                                              \
  public:                                                                     \
    const element_type* base( void ) const { return _data; }                  \
          element_type* base( void )       { return _data; }                  \
                                                                              \
    const element_type& at ( const int i ) const {                            \
      check_bounds( i );                                                      \
      return _data[ i ];                                                      \
    }                                                                         \
    element_type& at ( const int i ) {                                        \
      check_bounds( i );                                                      \
      return _data[ i ];                                                      \
    }                                                                         \
    void at_put ( const int i, const element_type val ) {                     \
      at( i ) = val;                                                          \
    }                                                                         \
                                                                              \
    static array_type* allocate( const int n JVM_TRAPS ) {                    \
      const int size = header_size() + n * sizeof(element_type);              \
      return (array_type*) CompilerObject::allocate(                          \
        ARRAY_TYPE##Array_type, size JVM_NO_CHECK);                           \
    }                                                                         \
  }

DEFINE_COMPILER_ARRAY( jubyte, Byte   );
DEFINE_COMPILER_ARRAY( jshort, Short  );
DEFINE_COMPILER_ARRAY( jint,   Int    );
DEFINE_COMPILER_ARRAY( void*,  Pointer);

#undef MAKE_COMPILER_ARRAY_NAME
#undef IMPLEMENT_COMPILER_ARRAY_BOUNDS_CHECK
#undef DEFINE_COMPILER_ARRAY

#endif
