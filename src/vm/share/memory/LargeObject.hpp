/*
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

class LargeObject {
protected:
  enum {
    alignment   = BitsPerWord * BytesPerWord,
    align_mask  = alignment-1,
  };
  
  unsigned _size;

  static void must_be_aligned ( const unsigned x ) {
    (void)x;
#ifndef PRODUCT
    GUARANTEE( (x & align_mask) == 0, "must be aligned" );
#endif
  }

  static void must_be_aligned ( const void* const x ) {
    must_be_aligned( unsigned(x) );
  }

  const LargeObject* next ( const unsigned size ) const {
    return DERIVED( const LargeObject*, this, size );
  }
  const LargeObject* next ( void ) const {
    return next( size() );
  }

#if ENABLE_LIB_IMAGES && USE_LARGE_OBJECT_AREA
  enum { mark_mask = 1 };

  static unsigned marked  ( const unsigned x ) { return x & mark_mask; }
  static unsigned mark    ( const unsigned x ) { return x | mark_mask; }
  static unsigned unmark  ( const unsigned x ) { return x &~mark_mask; }

  unsigned marked ( void ) const { return marked( _size ); }
  void mark       ( void )       { _size |= mark_mask; }
  void unmark     ( void )       { _size &=~mark_mask; }

  static LargeObject** interval_table_beg;
  static LargeObject** interval_table_end;

  static void move( LargeObject dst[],
                    const LargeObject beg[], const LargeObject end[] );
  static void compact( LargeObject* table_beg[], LargeObject* table_end[] );
#endif

  void set_size( const unsigned size ) {
    must_be_aligned( size );
    _size = size;
  }
public:
  const unsigned size( void ) const { return _size; }

//  static const LargeObject* find( const OopDesc* object );

  const void* body( void ) const {
    const void* p = this + 1;
    must_be_aligned(p);
    return p;
  }
  void* body( void ) {
    void* p = this + 1;
    must_be_aligned(p);
    return p;
  }

  static const LargeObject* head( const void* const p ) {
    must_be_aligned(p);
    const LargeObject* obj = ((const LargeObject*)p)-1;
    return obj;
  }
  static LargeObject* head( void* const p ) {
    must_be_aligned(p);
    LargeObject* obj = ((LargeObject*)p)-1;
    return obj;
  }
protected:
  static unsigned align_size  ( const unsigned size ) {
    return align_size_up( size, alignment );
  }

  juint* bitvector( void ) const {
    return bitvector( body() );
  }
  static unsigned bitvector_size  ( const unsigned size );

  static void set_bottom( LargeObject new_bottom[] ) {
    _large_object_area_bottom = (OopDesc**) new_bottom;
  }

  static void move ( const int delta, const LargeObject limit[] );
  static bool is_moving ( const OopDesc* p );
  static int  delta     ( const OopDesc* p );

  static const LargeObject* current_limit;
  static int                current_delta;
  static void update_pointer                  ( OopDesc** p );
  static void update_classinfo_pointers       ( void );
  static void update_pointer_in_instance_class( OopDesc** p );
  static void update_pointers ( OopDesc* beg, OopDesc exclusive_end[] );
public:
  static LargeObject* start ( void ) {
    return (LargeObject*) _compiler_area_top;
  }
  static LargeObject* end   ( void ) {
#if USE_LARGE_OBJECT_AREA
    return ((LargeObject*) _heap_top) - 1;
#else
    return (LargeObject*) _heap_top;
#endif
  }
  static LargeObject* bottom ( void ) {
    return (LargeObject*) _large_object_area_bottom;
  }
  static unsigned used ( void ) {
#if USE_LARGE_OBJECT_AREA
    return DISTANCE( bottom(), end() );
#else
    return 0;
#endif
  }
  static unsigned available ( void ) {
    return DISTANCE( start(), bottom() );
  }

  static void initialize( LargeObject end[] ) {
    must_be_aligned(end);
#if USE_LARGE_OBJECT_AREA
    set_bottom( end-1 );
#else
    set_bottom( end );
#endif
  }
  static void terminate ( void ) {}

  static void move ( const int delta, OopDesc* end[] ) {
#if USE_LARGE_OBJECT_AREA
    move( delta, ((LargeObject*) end)-1 );
#else
    set_bottom( DERIVED( LargeObject*, end, delta ) );
#endif
  }

  static unsigned allocation_size( const unsigned size ) {
    return  align_size( size + sizeof( LargeObject ) );
  }

#if USE_LARGE_OBJECT_AREA
  static LargeObject* allocate( const unsigned alloc_size );
  void free( void )
#if ENABLE_LIB_IMAGES
    { mark(); }
#else
    ;
#endif

  static void compact( void )
#if ENABLE_LIB_IMAGES
    ;
#else
    {}
#endif

#endif  // USE_LARGE_OBJECT_AREA

  static juint* bitvector( const void* p ) {
    return ObjectHeap::get_bitvectorword_for_aligned( (OopDesc**) p );
  }
  juint* next_bitvector( void ) { return next()->bitvector(); }

  static bool contains( const LargeObject* p ) {
    return bottom() <= p && p < end();
  }

#if USE_LARGE_OBJECT_AREA
  static void verify( void ) PRODUCT_RETURN;
#else
  static void verify( void ) {}
#endif
};

//#ifndef PRODUCT
//  #define USE_LARGE_OBJECT_DUMMY  0 USE_LARGE_OBJECT_AREA
//#else
  #define USE_LARGE_OBJECT_DUMMY  0
//#endif

#if USE_LARGE_OBJECT_DUMMY
class LargeObjectDummy: public LargeObject {
  enum {
    expected_signature = 0x12345678,
    expected_bitvector = 6
  };
  unsigned signature;
  void* p1;
  void* p2;
public:
  static void create  ( void );
  static void dispose ( void );
  static void check   ( void );

  static LargeObjectDummy* object;
  static void uninitialize ( void ) {
     object = NULL;
  }
};
#else
class LargeObjectDummy: public LargeObject {
public:
  static void create       ( void ) {}
  static void uninitialize ( void ) {}
};
#endif
