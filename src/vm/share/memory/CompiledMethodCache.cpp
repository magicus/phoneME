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

#include "incls/_precompiled.incl"
#include "incls/_CompiledMethodCache.cpp.incl"

#if ENABLE_COMPILER

CompiledMethodCache::Item*
CompiledMethodCache::Map[ CompiledMethodCache::MaxMethods ];

unsigned CompiledMethodCache::size;

#if ENABLE_CODE_PATCHING && USE_PATCHED_METHOD_CACHE
CompiledMethodCache::Item* CompiledMethodCache::_patched_method;
#endif

CompiledMethodCache::weight_type CompiledMethodCache::_aligned_weights;
#define weights _aligned_weights._weights

inline void CompiledMethodCache::lock( const int i ) {
  weights[ i ] |= Byte(LockMask);
}

inline void CompiledMethodCache::unlock( const int i ) {
  weights[ i ] &=~LockMask;
}

int CompiledMethodCache::upb;
int CompiledMethodCache::last_old;

inline void CompiledMethodCache::method_execution_sensor_reset( void ) {
  jvm_memset( _method_execution_sensor, -1, method_execution_sensor_size );
}

// CompiledMethod interface
#ifndef PRODUCT
void CompiledMethodCache::item_print( const Item* p ) {
  TTY_TRACE(( "%p ", p ));
  if(p) {
    Method::Raw( p->method() )().print_name_on( tty );   // C++ must die
  }
}
#endif

inline size_t CompiledMethodCache::get_size( const Item* p ) {
  return p->object_size();
}

inline int CompiledMethodCache::get_index( const Item* p ) {
  return p->get_cache_index();
}

inline void CompiledMethodCache::set_index( Item* p, const int i ) {
  p->set_cache_index( i );
}

inline void CompiledMethodCache::lock ( const Item* p ) {
  if( has_index( p ) ) {
    const int i = get_index( p );
    GUARANTEE( i <= upb, "cache index out of range" );
    lock( i );
#ifndef PRODUCT
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "\nLocking " ));
      item_print( p );
      tty->cr();
    }
#endif
  }
}

#if ENABLE_APPENDED_CALLINFO
CompiledMethodCache::Item * 
CompiledMethodCache::item_for_address( const address addr ) {
  if (!has_index((const Item*)addr)) {
    return NULL;
  }

  GUARANTEE(CompiledMethodCache::upb >= 0, "Cache cannot be empty");
  return (Item*)CompiledMethodDesc::find(CompiledMethodCache::Map,
                                         CompiledMethodCache::upb + 1,
                                         addr);
}
#endif // ENABLE_APPENDED_CALLINFO

#ifndef PRODUCT
inline void CompiledMethodCache::weights_dump( void ) {
#if USE_DEBUG_PRINTING
  TTY_TRACE(( "\nCurrent weights:" ));
  const int upb = CompiledMethodCache::upb;
  for( int i = 0; i <= upb; i++ ) {
    TTY_TRACE(( "\n[%u]: %u", i, weights[i] ));
  }
#endif
}

inline void CompiledMethodCache::map_dump( void ) {
#if USE_DEBUG_PRINTING
  tty->print( "\n*** Map dump ***" );
  {
   const int upb = CompiledMethodCache::upb;
    for( int i = 0; i <= upb; i++ ) {
      tty->print( "\n%04u: ", i );
      item_print( Map[i] );
    }
  }
  tty->print( "\n\nupb: %d\n*** End map dump ***", upb );
#endif
}

void CompiledMethodCache::dump( void ) {
  map_dump();
  weights_dump();
}
#endif

void CompiledMethodCache::insert( Item* p ) {
  const int i = ++CompiledMethodCache::upb;
  GUARANTEE( i < MaxMethods, "CompiledMethodCache index overflow" );
  set_index( p, i );
  weights[ i ] = InitCharge;  // Compiled in the middle of execution
  _method_execution_sensor[ i ] = 0xFF; // To avoid double increment
  Map[ i ] = p;
  size += get_size( p );

#if ENABLE_PERFORMANCE_COUNTERS && SUPPORTS_PROFILER_CONTROL
  {
    DECLARE_STATIC_BUFFER(char, name, 300);
    Os::suspend_profiler();
    Method::Raw m = p->method();
    m().print_name_to(name, 300);
    send_name_profiler(name, p->entry(), p->code_size());
    Os::resume_profiler();
  }
#endif
}

void CompiledMethodCache::init( void ) {
  size = 0;
  upb = -1;
  last_old = -1;
  jvm_memset( Map, 0, sizeof Map );
  jvm_memset( weights, 0, sizeof weights );
  method_execution_sensor_reset();
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::init()" ));
  }
}

inline void CompiledMethodCache::free( Item* p ) {
#ifndef PRODUCT
  if( TraceCompiledMethodCache ) {
    tty->print( "\nDeoptimizing" );
    item_print( p );
    tty->cr();
  }
#endif

#if ENABLE_JVMPI_PROFILE 
  // Send the compiled method unload event. 
  if(UseJvmpiProfiler && 
     JVMPIProfile::VMjvmpiEventCompiledMethodUnloadIsEnabled()) {
    jint method_id = Method::Raw(p->method())().method_id();
    JVMPIProfile::VMjvmpiPostCompiledMethodUnloadEvent(method_id);
  }
#endif
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR
BranchTable::remove_only((address) p);
#endif

#if 1
  Method::Raw( p->method() )().unlink_compiled_code();
#else
  // On x86 speeds up the execution (about 10%) for small heaps.
  // On ARM does the opposite.
  // Reserved for use in the future.
  Method::Raw( p->method() )().
    set_execution_entry((address) shared_invoke_compiler);
#endif
}

void CompiledMethodCache::lock_unevictable ( void ) {
  if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "CompiledMethodCache::lock_unevictable()" ));
  }

  {
    OopDesc* const compiled_method_class =
      Universe::compiled_method_class()->prototypical_near();
    CodeGenerator* gen = CodeGenerator::current();
    OopDesc* const current = gen ? gen->compiled_method()->obj() : NULL;
    ForAllHandles( handle ) {
      OopDesc* const p = handle->obj();
      if( p && p->klass() == compiled_method_class && p != current ) {
        lock( (Item*)p );
      }
    }
  }

  ForAllThreads( thread ) {
    for( Frame frame( &thread() );; ) {
      if( frame.is_entry_frame() ) {
        #define entry_frame frame.as_EntryFrame()
        if( entry_frame.is_first_frame() ) break;
        entry_frame.caller_is( frame );
        #undef entry_frame
      } else {
        #define java_frame frame.as_JavaFrame()
        if( java_frame.is_compiled_frame() ) {
          lock( (CompiledMethodDesc*) java_frame.compiled_method() );
        }
        java_frame.caller_is( frame );
        #undef java_frame
      }
    }
  }
}

// This function is always called in non-empty-cache context
void CompiledMethodCache::unlock ( void ) {
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::unlock()" ));
  }
  for( int i = DWordFloor( upb ); i >= 0; i -= BytesPerDWord ) {
    *(DWord*)(weights+i) &= ~QuadMask( LockMask );
  }
}

int CompiledMethodCache::alloc( void ) {
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "\nCompiledMethodCache::alloc()" ));
  }

  if( upb == MaxMethods-1 ) {
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "No free indices left,"
        "looking for a zero-weight item to evict" ));
    }
    if( !smart_evict_underweight() ) {
      return UndefIndex;
    }
  }

  const int i = upb + 1;
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "Allocated compiled method cache index %d", i ));
  }
  return i;
}

void CompiledMethodCache::degrade( void ) {     // Shift right by quads
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::degrade()" ));
  }

  for( int i = DWordFloor( upb ); i >= 0; i -= BytesPerDWord ) {
    DWord* const p = (DWord*) (weights + i);
    *p = (*p >> 1) & QuadMask( WeightMask-1 );
  };
}

void CompiledMethodCache::on_timer_tick( void ) {
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::on_timer_tick()" ));
  }
  int i;

#if USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  for( i = upb; i >= 0; i -- ) {
    CompiledMethodDesc* cm = Map[ i ];
    MethodDesc * m = cm->method();
    
    address entry = cm->entry();
#if ENABLE_THUMB_COMPILER
    // Set the low bit so that BX will automatically switch into
    // THUMB mode.
    entry += 1;
#endif
    m->set_execution_entry_inline( entry );
  }
#endif

  DWord overflow = 0;
  for( i = DWordFloor( upb ); i >= 0; i -= BytesPerDWord ) {
    DWord* const p = (DWord*) (weights + i);
    DWord* const q = (DWord*) (_method_execution_sensor + i);
    const DWord x = ((~*q & QuadMask( 1 )) << ChargeBits) + *p;
    overflow |= x;
    *p = x;
    *q = (DWord) -1;
  }
  if( overflow & QuadMask( WeightMask ) ) {
    degrade();
  }
#ifndef PRODUCT
  if( TraceCompiledMethodCache ) {
    weights_dump();
  }
#endif
}

// [input]  size must be strictly positive
// [output] returns the number of bytes evicted.
int CompiledMethodCache::evict ( int size ) {
  GUARANTEE( size > 0, "sanity check" );

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::evict( %d )", size ));
  }

  int upb = CompiledMethodCache::upb;
  if( upb < 0 ) {
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "Compiled method cache is empty" ));
    }
    return 0;
  }

  int requested_size = size;
  lock_unevictable();

  enum { HistogrammSize = 1 << WeightBits };
  // GBA's thumb compiler dislikes stack arrays
#ifdef GBA
  static
#endif
  int histogramm[ HistogrammSize ];
  jvm_memset( histogramm, 0, sizeof histogramm );

  // First pass through the map to gather statistics
  {
    int i = upb; do {
      const int weight = weights[ i ];
      if( !( weight & LockMask ) ) {
        histogramm[ weight ] += get_size( Map[ i ] );
      }
    } while( --i >= 0 );
  }

  // Look through the histogramm to determine the cut-off weight
  unsigned cutoff_weight = 0;
  {
    do {
      const int weight_size = histogramm[ cutoff_weight ];
      if( weight_size >= size ) break;
      size -= weight_size;
    } while( ++cutoff_weight < HistogrammSize );
#ifndef PRODUCT
    if( TraceCompiledMethodCache ) {
      TTY_TRACE(( "\nHistogramm:" ));
      int i = 0; do {
        TTY_TRACE(( "\n[%u]: %u", i, histogramm[ i ] ));
      } while( ++i < HistogrammSize );
      TTY_TRACE(( "\nCut-off weight: %u Remaining size: %u",
                  cutoff_weight, size ));
    }
#endif
  }

  // Second pass through the map does actual eviction
  {
    int i = upb; do {
      const unsigned weight = weights[ i ];
      if( weight < cutoff_weight ) {
        weights[ i ] = 0;
      } else if( weight == cutoff_weight && size > 0 ) {
        size -= get_size( Map[ i ] );
        weights[ i ] = 0;
      }
    } while( --i >= 0 );
    evict_zero_weight();
  }

  unlock();

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache under-eviction: %d", size ));
  }

  if ( VerifyGC ) {
    verify();
  }
  // returns the number of bytes evicted.
  return requested_size - size;
}

int CompiledMethodCache::zero_weight_space( void ) {
  int s = 0;
  for( int i = CompiledMethodCache::upb; i >= 0; i-- ) {
    if( weights[ i ] == 0 ) {
      s += get_size( Map[ i ] );
    }
  }
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::zero_weight_space: %d", s ));
  }
  return s;
}

inline const CompiledMethodCache::Item*
CompiledMethodCache::compute_threshold( void ) {
  const Item* p = NULL;
  const int i = last_old;
  if( i >= 0 ) {
    p = Map[ i ];
  }
  return p;
}

void CompiledMethodCache::evict_zero_weight( void ) {
  const int upb = CompiledMethodCache::upb;
  int src = 0;

  // Skip ordered live methods at the bottom
  for(;; src++ ) {
    if( src > upb ) {
      return;     // No methods freed, no need to continue
    }
    if( !weights[ src ] ) {
      break;
    }
  }

  const Item* threshold = compute_threshold();
  int dst = src - 1;
  
#if ENABLE_CODE_PATCHING && USE_PATCHED_METHOD_CACHE
  {
    if (_patched_method != NULL) {
      GUARANTEE(has_index(_patched_method), "Sanity");
      const int cached_index = get_index(_patched_method);
      if (weights[cached_index] == 0) {
        _patched_method = NULL;
      }
    }
  }
#endif

  for( ; src <= upb; src++ ) {
    Item* p = Map[ src ];
    const Byte weight = weights[ src ];
    if( weight ) {
      weights[ ++dst ] = weight;
      _method_execution_sensor[ dst ] = _method_execution_sensor[ src ];
      Map[ dst ] = p;
      set_index( p, dst );
    } else {
      size -= get_size( p );
      free( p );
    }
  }  
  set_upb( dst );
  compute_last_old( threshold );

  if ( VerifyGC ) {
    verify();
  }
}

int CompiledMethodCache::evict_underweight( void ) {
  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "CompiledMethodCache::evict_underweight()" ));
  }

  const int old_size = size;
  lock_unevictable();
  evict_zero_weight();
  unlock();
  return old_size - size;
}

int CompiledMethodCache::smart_evict_underweight ( void ) {
  // Check whether zero-weight items exist
  for( int i = CompiledMethodCache::upb; i >= 0; i-- ) {
    if( weights[ i ] == 0 ) {
      return evict_underweight();
    }
  }
  return 0;
}

void CompiledMethodCache::compute_last_old( const Item* const threshold ) {
  int i = last_old;
  {
    const int upb = CompiledMethodCache::upb;
    if( i > upb ) i = upb;
  }

  for( ; i >= 0 && Map[ i ] > threshold; i-- ) {}
  last_old = i;
}

#ifndef PRODUCT
void CompiledMethodCache::verify() {
  ForAllThreads( thread ) {
    for( Frame frame( &thread() );; ) {
      if( frame.is_entry_frame() ) {
        #define entry_frame frame.as_EntryFrame()
        if( entry_frame.is_first_frame() ) break;
        entry_frame.caller_is( frame );
        #undef entry_frame
      } else {
        #define java_frame frame.as_JavaFrame()
        if( java_frame.is_compiled_frame() ) {
          Item* m = (Item*) java_frame.compiled_method();
          GUARANTEE_R( !has_index(m) || get_index( m ) <= upb,
            "cache index out of range" );
        }
        java_frame.caller_is( frame );
        #undef java_frame
      }
    }
  }
}
#endif

#if ENABLE_ISOLATES
inline bool CompiledMethodCache::marked( const Item* p ) {
  const MethodDesc* method = p->method();
  return !ObjectHeap::in_dead_bundles( method ) &&
         !ObjectHeap::in_collection_area_unmarked( method );
}

void CompiledMethodCache::cleanup_unmarked( void ) {  
  // Keep the index ordered
  int src = 0;

  // Skip ordered live methods at the bottom
  if (TraceGC) {
    TTY_TRACE_CR(("TraceGC: cache: upb %d", upb));
  }
  for(;; src++ ) {
    if( src > upb ) {
      return;     // No methods freed, no need to continue
    }
    if( !marked( Map[src] ) ) {
      break;
    } else {
#if ENABLE_TTY_TRACE
      if (TraceGC) {
        Item* p = Map[ src ];
        TTY_TRACE_CR(("TraceGC: cache: skipping, meth: 0x%x, cm: 0x%x",
                      (int)p->method(), (int)p));
      }
#endif
    }
  }
    
  const Item* threshold = compute_threshold();
  int dst = src - 1;

#if ENABLE_CODE_PATCHING && USE_PATCHED_METHOD_CACHE
  {
    if (_patched_method != NULL) {
      GUARANTEE(has_index(_patched_method), "Sanity");
      if (marked(_patched_method)) {
        _patched_method = NULL;
      }
    }
  }
#endif

  for( ; src <= upb; src++ ) {
    Item* p = Map[ src ];
    if( marked( p ) ) {
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: cache: marked, meth: 0x%x, cm: 0x%x",
                      (int)p->method(), (int)p));
      }
      weights[ ++dst ] = weights[ src ];
      _method_execution_sensor[ dst ] = _method_execution_sensor[ src ];
      Map[ dst ] = p;
      set_index( p, dst );
    } else {
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: cache: unmarked, meth: 0x%x, cm: 0x%x",
                      (int)p->method(), (int)p));
      }
      free( p );
      size -= get_size( p );
    }
  }
  set_upb( dst );
  compute_last_old( threshold );

  if ( VerifyGC ) {
    verify();
  }
}
#endif

#undef weights

#endif //ENABLE_COMPILER
