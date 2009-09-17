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


class Bits: public AllStatic {
public:
  typedef jint item_type;
  enum {
    LogBitSize = LogBitsPerInt,
    BitSize = 1 << LogBitSize
  };
private:
  enum {
    BitIndexMask = BitSize - 1
  };
  static int index( const int i ) {
    return i >> LogBitSize;
  }
  static int bit_index( const int i ) {
    return i & BitIndexMask;
  }
public:
  static item_type& at( item_type p[], const int i ) {
    return p[ index(i) ];
  }
  static const item_type& at( const item_type p[], const int i ) {
    return p[ index(i) ];
  }
  static item_type bit( const int i ) {
    return 1 << bit_index(i);
  }
  static item_type get( const item_type p[], const int i ) {
    return ( at( p, i ) >> bit_index( i ) ) & 1;
  }
  static void set( item_type p[], const int i ) {
    at( p, i ) |= bit( i );
  }
  static void clear( item_type p[], const int i ) {
    at( p, i ) &=~ bit( i );
  }
  static void copy( item_type* dst, const item_type* src, int length ) {
    do {
      *dst++ = *src++;
    } while( --length > 0 );
  }
  static void _or( item_type* dst, const item_type* src, int length ) {
    do {
      *dst++ |= *src++;
    } while( --length > 0 );
  }
  static void _and( item_type* dst, const item_type* src, int length ) {
    do {
      *dst++ &= *src++;
    } while( --length > 0 );
  }
  static void _sub( item_type* dst, const item_type* src, int length ) {
    do {
      *dst++ &= ~*src++;
    } while( --length > 0 );
  }
  static item_type test_and( item_type* dst, const item_type* src, int length ) {
    item_type s = 0;
    do {
      const item_type x = *dst & *src++;
      *dst++ = x;
      s |= x;
    } while( --length > 0 );
    return s;
  }
  static item_type test_and_not( item_type* dst, const item_type* src, int length ) {
    item_type s = 0;
    do {
      const item_type x = *dst &~ *src++;
      *dst++ = x;
      s |= x;
    } while( --length > 0 );
    return s;
  }
  static int not_empty( const item_type* src, int length ) {
    do {
      if( *src++ ) {
        break;
      }
    } while( --length > 0 );
    return length;
  }
  static int compute_length( const int bit_size ) {
    return index( bit_size + BitIndexMask );
  }
};
