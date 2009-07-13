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

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR

class ROMBitSet: public TypeArray {
public:
  HANDLE_DEFINITION_CHECK(ROMBitSet, TypeArray);

  void set_bit( const int i ) {
    check( i );
    Bits::set( data(), i );
  }

  Bits::item_type get_bit( const int i ) {
    check( i );
    return Bits::get( data(), i );
  }

  void copy( OopDesc* src ) {
    ROMBitSet s( src );
    GUARANTEE( length() == s.length(), "Sanity" );
    Bits::copy( data(), s.data(), length() );
  }
  void and( OopDesc* src ) {
    ROMBitSet s( src );
    GUARANTEE( length() == s.length(), "Sanity" );
    Bits::and( data(), s.data(), length() );
  }
  void or( OopDesc* src ) {
    ROMBitSet s( src );
    GUARANTEE( length() == s.length(), "Sanity" );
    Bits::or( data(), s.data(), length() );
  }
  void sub( OopDesc* src ) {
    ROMBitSet s( src );
    GUARANTEE( length() == s.length(), "Sanity" );
    Bits::sub( data(), s.data(), length() );
  }

  int not_empty( void ) {
    return Bits::not_empty( data(), length() );
  }

  void compute_range( void ) {
    const jubyte* base = bytes();

    const jubyte* end = DERIVED(const jubyte*, base, length() * BytesPerInt);
    while( --end >= base && *end == 0 ) {
    }

    const jubyte* start = base;
    for( ; start < end && *start == 0; start++ ) {
    }

    _range_start = start - base;
    _range_length = end - start + 1;
  }

  void print_class_names( Stream* out, const char prefix[] );
  void print_bytes( Stream* out, const int from, int count );

  static void initialize( const int n ) {
    _bit_size = n;
    _size = Bits::compute_length( n );
    _range_start = 0;
    _range_length = 0;
  }

  static int bit_size    ( void ) { return _bit_size; }
  static int range_start ( void ) { return _range_start; }
  static int range_length( void ) { return _range_length; }

  static OopDesc* create( JVM_SINGLE_ARG_TRAPS ) {
    GUARANTEE( _size != 0, "Sanity" );
    return Universe::new_int_array( _size JVM_NO_CHECK );
  }

private:
  static int _size;
  static int _bit_size;
  static int _range_start;
  static int _range_length;

  static void check( const unsigned bit_index ) {
    GUARANTEE( bit_index < unsigned(bit_size()), "Bit index out of range" );
  }
  Bits::item_type* data( void ) { return int_base_address(); }
  const jubyte* bytes( void ) { return (const jubyte*) data(); }
};

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR

