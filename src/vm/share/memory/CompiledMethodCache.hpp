/*
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

class CompiledMethodDesc;

class CompiledMethodCache {
public:
  typedef CompiledMethodDesc Item;
  enum { MaxMethods = method_execution_sensor_size };
  enum { UndefIndex = -1 };

  static unsigned size;

  static void init  ( void );
  static void term  ( void ) {} // No cleanup required

  static int  alloc ( void );   // UndefIndex if overflow

  // Allocate index, compile method, insert to the cache, unlock.
  // insert() may relocate index.
  static void insert( Item* p );

  // Compute amount of ready-to-evict compiled code
  static int zero_weight_space( void );

  // Degrade statistic counters
  static void degrade( void );

  // Summarize local statistics
  static void on_timer_tick( void );

  // Set the generation boundary
  static void on_promotion( void ) {
    last_old = upb;
  }

  // External policy has to calculate code_size_to_evict taking into account
  // heap size, amount of objects survived previous GC and current
  // CompiledMethodCache::size.
  // code_size_to_evict must be strictly positive
  // Returns the number of bytes evicted.
  static int evict ( int code_size_to_evict );

  static int evict_underweight        ( void );
  static int smart_evict_underweight  ( void );

  static inline bool has_index ( const Item* p ) {
    return _compiler_area_start <= (OopDesc**) p &&
      (OopDesc**) p < _compiler_area_top;
  }

#if ENABLE_APPENDED_CALLINFO
  static Item * item_for_address( const address addr );
#endif
#ifndef PRODUCT
  static void dump( void );
#endif
//private:      // ADS compiler does not like private datatypes
  typedef unsigned char Byte;   // The shortest fast native integer
  typedef unsigned long DWord;  // The longest of the fast aligned integer types
 private:
  enum { BytesPerDWord = 4 };

  static inline int DWordFloor( const int x ) {
    return x & -BytesPerDWord;
  }

  static inline DWord QuadMask( const DWord x ) {
    return ((x) << 24) | ((x) << 16) | ((x) << 8) | (x);
  }

  static Item* Map[ MaxMethods ];
  static int upb;       // inclusive
  static int last_old;  // old generation boundary

  // Array of weights
  enum {
    WeightBits  = 6,    // Must be < 8 (post-factum overflow correction)
    ChargeBits  = 4,    // ChargeBits < WeightBits
    LockBit     = 7,    // LockBit > WeightBits

    WeightMask  = 1 << WeightBits,
    ChargeMask  = 1 << ChargeBits,
    LockMask    = 1 << LockBit,
    InitCharge  = ChargeMask
  };

  union weight_type {
    Byte  _weights[ MaxMethods ];
    DWord _dummy;
  };

  static weight_type _aligned_weights;

  static inline void weights_reset( void );
  static inline void lock  ( const int i );
  static inline void unlock( const int i );
  static inline void unlock( void );

  static void   lock_unevictable( void );
  static void   evict_zero_weight( void );

  static inline const Item* compute_threshold( void );
  static void  compute_last_old( const Item* const threshold );
  static inline void set_upb( const int i ) { upb = i; }

  // Method execution sensor interface
  enum { MethodExecutionSensorUndef = 0xFF }; // Any odd value
  static inline void method_execution_sensor_reset( void );

  // Item interface
  static inline size_t  get_size  ( const Item* p );
  static inline int     get_index ( const Item* p );
  static inline void    set_index ( Item* p, const int i );

  static inline void    lock      ( const Item* p );
  static inline void    free      ( Item* p );

#if ENABLE_COMPILER && ENABLE_ISOLATES
  static inline bool    marked    ( const Item* p );
#endif

#ifndef PRODUCT
  static inline void item_print ( const Item* p );
  static inline void weights_dump( void );
  static inline void map_dump( void );
#endif

  static void verify() PRODUCT_RETURN;

#if ENABLE_COMPILER && ENABLE_ISOLATES
  static void cleanup_unmarked ( void );
#endif

#if ENABLE_CODE_PATCHING && USE_PATCHED_METHOD_CACHE
public:
  static Item* patched_method() {
    return _patched_method;
  }

  static void set_patched_method( Item* p ) {
    _patched_method = p;
  }

  static void reset_patched_method( void ) {
    _patched_method = NULL;
  }
private:
  static Item* _patched_method;
#endif

  friend class ObjectHeap;
#if (ENABLE_INLINE || ENABLE_TRAMPOLINE) && ARM
public:
  static int get_upb(void) {
    return upb;
  }
  static CompiledMethodDesc* get_item(int i) {
    return Map[i];
  }
#endif
#if ENABLE_TRAMPOLINE  && !CROSS_GENERATOR
  friend class BranchTable;
#endif

};
