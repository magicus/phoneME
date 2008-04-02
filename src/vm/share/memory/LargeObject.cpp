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

#include "incls/_precompiled.incl"
#include "incls/_LargeObject.cpp.incl"

#if USE_LARGE_OBJECT_AREA
inline unsigned LargeObject::bitvector_size ( const unsigned size ) {
  return size >> (LogBytesPerWord+LogBitsPerByte);
}

const LargeObject*  LargeObject::current_limit;
int                 LargeObject::current_delta;

#if ENABLE_LIB_IMAGES
LargeObject** LargeObject::interval_table_beg;
LargeObject** LargeObject::interval_table_end;
#endif

inline bool LargeObject::is_moving( const OopDesc* p ) {
  return p >= ((const OopDesc*) bottom()) && p < ((const OopDesc*)current_limit);
}

inline int LargeObject::delta( const OopDesc* p ) {
  GUARANTEE( is_moving( p ), "Not a large object to move" );
#if ENABLE_LIB_IMAGES
  const OopDesc* const* q = (const OopDesc* const*) interval_table_end;
  do {
    q -= 2;
  } while ( q[-1] > p );
  return int( *q );
#else
  (void) p;
  return current_delta;
#endif
}

void LargeObject::update_pointer( OopDesc** p ) {
  const OopDesc* const obj = *p;
  if( is_moving( obj ) ) {
    *p = DERIVED( OopDesc*, obj, delta( obj ) );
  }
}

void LargeObject::update_pointer_in_instance_class( OopDesc** p ) {
  const OopDesc* const obj = *p;
  if( is_moving( obj ) && !obj->is_class_info() ) {
    *p = DERIVED( OopDesc*, obj, delta( obj ) );
  }
}

inline void LargeObject::update_pointers( OopDesc* p, OopDesc limit[] ) {
  while( p < limit ) {
    const FarClassDesc* const blueprint = p->blueprint();
    p->oops_do_for(blueprint, p->is_instance_class() || p->is_obj_array_class()
                              ? update_pointer_in_instance_class
                              : update_pointer);
    p = DERIVED( OopDesc*, p, p->object_size_for( blueprint ) );
  }
} 

inline void LargeObject::update_classinfo_pointers( void ) {
#if ENABLE_ISOLATES
  for( int i = Task::FIRST_TASK; i < MAX_TASKS; i++ ) {
    Task::Raw task = Task::get_task(i);
    if( task.not_null() ) {
      ObjArray::Raw class_list = task().class_list();
#else
      ObjArray::Raw class_list = Universe::class_list();
#endif
      for( int len = class_list().length(); --len >= 0; ) {
        OopDesc* jc = class_list().obj_at(len);
        if( jc ) {
          OopDesc** p = jc->obj_field_addr(InstanceClass::class_info_offset());
          update_pointer( p );
          GUARANTEE((*p)->is_class_info(), "not class info");
        }
      }
#if ENABLE_ISOLATES
    }
  }
#endif
}

#if 0
void VerifyReferencesToLargeObjectArea::check ( OopDesc** p ) {
  LargeObject* const obj = (LargeObject*) *p;
#if 0
  GUARANTEE( !LargeObject::contains(obj),
             "Unexpected reference to large object area" );
#endif
  if( LargeObject::contains(obj) ) {
    tty->print_cr("Unexpected reference to large object area %p", obj);
  }
}
#endif

LargeObject* LargeObject::allocate( const unsigned size ) {
  ObjectHeap::shrink_with_compiler_area( size );
  LargeObject* p = DERIVED( LargeObject*, bottom(), -int(size) );
  if( p < start() ) {
    return NULL;
  }
  set_bottom( p );
  p->set_size( size );
  jvm_memset( p->bitvector(), 0, bitvector_size( size ) );
  return p;
}

#if ENABLE_LIB_IMAGES

inline void LargeObject::move( LargeObject dst[],
                         const LargeObject beg[], const LargeObject end[] ) {
  
  juint* const bit_beg = beg->bitvector();
  juint* const bit_end = end->bitvector();

  { // Relocate interior pointers
    // There cannot be any pointers from fixed to moving large objects
    OopDesc** p = (OopDesc**)beg->body();
    for( juint* bitp = bit_beg; bitp < bit_end; p += BitsPerWord, bitp++ ) {
      juint bitword = *bitp;
      if( bitword ) {
        OopDesc** pp = p;
        do {
#define SHIFT_ZEROS(n) if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; pp += n; }
          SHIFT_ZEROS(16)
          SHIFT_ZEROS( 8)
          SHIFT_ZEROS( 4)
          SHIFT_ZEROS( 2)
          SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS
          update_pointer( pp );
          pp++;
        } while( (bitword >>= 1) != 0 );
      }
    }
  }
  
  // Move bitvector
  jvm_memmove( dst->bitvector(), bit_beg, DISTANCE( bit_beg, bit_end ) );

  // Move objects
  jvm_memmove(dst, beg, DISTANCE(beg, end));
}

void LargeObject::compact( LargeObject* table_beg[], LargeObject* table_end[] ){
  // {Live Dead}* Live
  GUARANTEE( (table_end - table_beg) & 1,
             "Number of elements in interval table must be odd" );

  interval_table_beg = table_beg;
  interval_table_end = table_end;
  current_limit = table_end[-2];  // Beginning of the last gap

  // Compute offsets
  // {Live Offset}* Live
  {
    unsigned delta = 0;
    LargeObject** p = table_end - 2;
    do {
      delta += DISTANCE( p[0], p[1] );
      p[0] = (LargeObject*) delta;
    } while( (p -= 2) > table_beg );
  }

#if 0
  // Dump the modified interval table
  {
    tty->print_cr( "Modified interval table:" );
    const LargeObject* const* p;
    for( p = table_beg; (p += 2) < table_end; ) {
      tty->print_cr( "%p %d", p[-2], p[-1] );
    }
    tty->print_cr( "%p", p[-2] );
  }
#endif


  // Relocate pointers
  const bool gc_active = ObjectHeap::is_gc_active();
  if( !gc_active ) {
    Scheduler::gc_prologue(ObjectHeap::do_nothing);
  }

  update_pointers( (OopDesc*) _heap_start, (OopDesc*) _inline_allocation_top );

  // Update global_refs_array
  RefArray::current()->oops_do( update_pointer,
                                ObjectHeap::STRONG|ObjectHeap::WEAK );
  // Update GC roots
  ObjectHeap::roots_do( update_pointer );

#if USE_LARGE_OBJECT_DUMMY
  update_pointer( (OopDesc**) &LargeObjectDummy::object );
#endif

// This should not be necessary anymore because all pointers to LargeObjects
// are stored inside ObjArrays, which should be relocated automatically.
//
// #if USE_BINARY_IMAGE_LOADER && !USE_IMAGE_PRELOADING
//   ROMImage::oops_do( update_pointer );
// #endif
// 

  // Move objects
  LargeObject* new_bottom;
  {
    int delta = 0;
    LargeObject** p = table_end - 2;
    do {
      const int offset = int( p[0] );
      const LargeObject* dead = DERIVED(LargeObject*, p[1], delta - offset);
      delta = offset;
      const LargeObject* live = p[-1];
      move( DERIVED(LargeObject*, live, offset), live, dead );
    } while( (p -= 2) > table_beg );
    new_bottom = DERIVED(LargeObject*, bottom(), delta);
  }

  update_classinfo_pointers();

  set_bottom(new_bottom);

  if( !gc_active ) {
    Scheduler::gc_epilogue();
  }

#if 1
  // Restore interval table - for testing only, MUST BE REMOVED
  // {Live Dead}* Live
  // Dump the restored interval table
  {
    int delta = 0;
    LargeObject** p = table_end - 2;
    do {
      const int offset = int( p[0] );
      p[0] = DERIVED(LargeObject*, p[1], delta - offset);
      delta = offset;
    } while( (p -= 2) > table_beg );
  }

  {
    tty->print_cr( "Restored interval table:" );
    for( const LargeObject* const* p = table_beg; p < table_end; p++ ) {
      tty->print_cr( "%p", *p );
    }
  }
#endif

  verify();
}

// Dispose all large objects marked as free
void LargeObject::compact( void ) {
  // Setup the interval table
  LargeObject** table_beg = (LargeObject**) _inline_allocation_top;
  LargeObject** table_end = (LargeObject**) _compiler_area_start;
  {
    OopDesc** const heap_top   = _heap_top;
    OopDesc** const heap_limit = _heap_limit;
    if( DISTANCE( heap_top, heap_limit ) > DISTANCE( table_beg, table_end ) ) {
      table_beg = (LargeObject**) heap_top;
      table_end = (LargeObject**) heap_limit;
    }
  }

  // Fill the interval table
  {
    LargeObject** table = table_beg;
    unsigned current_mark = mark_mask;

    LargeObject* obj = bottom();    
    for( LargeObject* const limit = end(); obj < limit; ) {
      unsigned size = obj->_size;
      const unsigned mark = marked( size );
      if( mark != current_mark ) {
        current_mark = mark;
        GUARANTEE( table < table_end, "Interval table overflow" );
        *table++ = obj;
      }
      if( mark ) {
        size = unmark( size );
        obj->set_size( size );
      }
      obj = (LargeObject*) obj->next( size );
    }
    if( current_mark == mark_mask ) {
      GUARANTEE( table < table_end, "Interval table overflow" );
      *table++ = obj;
    }
    table_end = table;
  }
  
  // Free dead objects at the bottom
  set_bottom( *table_beg );

#if 0
  // Dump the interval table
  {
    tty->print_cr( "Interval table created:" );
    for( const LargeObject* const* p = table_beg; p < table_end; p++ ) {
      tty->print_cr( "%p", *p );
    }
  }
#endif

  // Optimized cases
  if( DISTANCE(table_beg, table_end) == sizeof *table_beg ) {      
    return; // Nothing to do
  }

  compact( table_beg, table_end );
}

void LargeObject::move ( const int delta, const LargeObject limit[] ) {
  if( !delta ) {
    return;
  }

  must_be_aligned( delta );  

  const LargeObject* const src = bottom();
  if( src == limit ) {
    set_bottom( DERIVED( LargeObject*, src, delta ) );
    return;
  }

  LargeObject* interval_table[ 3 ];
  interval_table[ 0 ] = (LargeObject*) src;
  interval_table[ 1 ] = (LargeObject*) limit;
  interval_table[ 1 ] = DERIVED( LargeObject*, limit, delta );

  compact( interval_table, interval_table + 3 );
}

#else //!ENABLE_LIB_IMAGES

void LargeObject::move ( const int delta, const LargeObject limit[] ) {
#define is_moving(obj) \
 (((OopDesc*)(obj)) >= ((OopDesc*)src) && ((OopDesc*)(obj)) < ((OopDesc*)limit))
#define new_location(type, obj) DERIVED( type, obj, delta )

  if( !delta ) {
    return;
  }

  must_be_aligned( delta );  
  const LargeObject* const src = bottom();
  if( src == limit ) {
    set_bottom( DERIVED( LargeObject*, src, delta ) );
    return;
  }

#if USE_LARGE_OBJECT_DUMMY
  LargeObjectDummy::check();
#endif

  current_limit = limit;
  current_delta = delta;

  const bool gc_active = ObjectHeap::is_gc_active();
  if( !gc_active ) {
    Scheduler::gc_prologue(ObjectHeap::do_nothing);
  }

  update_pointers( (OopDesc*) _heap_start, (OopDesc*) _inline_allocation_top );

  // Update global_refs_array
  RefArray::current()->oops_do( update_pointer,
                                ObjectHeap::STRONG|ObjectHeap::WEAK );
  // Update GC roots
  ObjectHeap::roots_do( update_pointer );

#if USE_LARGE_OBJECT_DUMMY
  update_pointer( (OopDesc**) &LargeObjectDummy::object );
#endif

// This should not be necessary anymore because all pointers to LargeObjects
// are stored inside ObjArrays, which should be relocated automatically.
//
// #if USE_BINARY_IMAGE_LOADER && !USE_IMAGE_PRELOADING
//   ROMImage::oops_do( update_pointer );
// #endif
// 

  juint* const bit_beg = src  ->bitvector();
  juint* const bit_end = limit->bitvector();

  { // Relocate interior pointers
    // There cannot be any pointers from fixed to moving large objects
    OopDesc** p = (OopDesc**)src->body();
    for( juint* bitp = bit_beg; bitp < bit_end; p += BitsPerWord, bitp++ ) {
      juint bitword = *bitp;
      if( bitword ) {
        OopDesc** pp = p;
        do {
#define SHIFT_ZEROS(n) if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; pp += n; }
          SHIFT_ZEROS(16)
          SHIFT_ZEROS( 8)
          SHIFT_ZEROS( 4)
          SHIFT_ZEROS( 2)
          SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS
          OopDesc* const obj = *pp;
          if( is_moving( obj ) ) {
            *pp = new_location( OopDesc*, obj );
          }
          pp++;
        } while( (bitword >>= 1) != 0 );
      }
    }
  }
  
  LargeObject* const dst = new_location( LargeObject*, src );

  // Move bitvector
  jvm_memmove( dst->bitvector(), bit_beg, DISTANCE( bit_beg, bit_end ) );

  // Move objects, adjust top
  LargeObject* new_bottom =
    (LargeObject*)jvm_memmove(dst, src, DISTANCE(src, limit));

  update_classinfo_pointers();

  set_bottom(new_bottom);
  if( !gc_active ) {
    Scheduler::gc_epilogue();
  }
#undef is_moving
#undef new_location
}

void LargeObject::free( void ) {
  if( this ) {
    (void) body();
    GUARANTEE(contains(this), "Large object outside of large object area");
    move( DISTANCE(this, this->next()), this );
    verify();
  }
}
#endif //!ENABLE_LIB_IMAGES

#if 0 // Not currently used
const LargeObject* LargeObject::find( const OopDesc* object ) {
  const LargeObject* obj = (const LargeObject*) object;
  const LargeObject* prev = NULL;
  if( obj < end() ) {
    for( const LargeObject* p = bottom(); p < obj; p = p->next() ) {
      prev = p;
    }
  }
  return prev;
}
#endif

#ifndef PRODUCT
void LargeObject::verify( void ) {
#if USE_LARGE_OBJECT_DUMMY
  LargeObjectDummy::check();
#endif
  { // References between large objects must only be up-going
    // No pointers from large objects to ObjectHeap allowed
    for( const LargeObject* obj = bottom(); obj < end(); obj = obj->next() ) {
#if ENABLE_LIB_IMAGES
      GUARANTEE( !obj->marked(), "No large objects can be marked here" );
#endif
      const juint* const bit_end = obj->next()->bitvector();
      const OopDesc* const* p = (const OopDesc* const*) obj->body();
      for( const juint* bitp = obj->bitvector(); bitp < bit_end;
                                                   p += BitsPerWord, bitp++ ) {
        juint bitword = *bitp;
        if( bitword ) {
          const OopDesc* const* pp = p;
          do {
#define SHIFT_ZEROS(n) if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; pp += n; }
            SHIFT_ZEROS(16)
            SHIFT_ZEROS( 8)
            SHIFT_ZEROS( 4)
            SHIFT_ZEROS( 2)
            SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS
            const OopDesc* const q = *pp;            
            GUARANTEE_R( !ObjectHeap::contains( q ),
                         "Reference from large object area to object heap");
            GUARANTEE_R( !(q >= ((OopDesc*) bottom()) && q < ((OopDesc*) obj)),
                         "Backward pointer in large object area" );
            pp++;
          } while( (bitword >>= 1) != 0 );
        }
      }
    }
  }
}

#if USE_LARGE_OBJECT_DUMMY
LargeObjectDummy* LargeObjectDummy::object;

void LargeObjectDummy::create( void ) {
  GUARANTEE( object == NULL, "Only one dummy large object at a time" );
  LargeObjectDummy* p =
    (LargeObjectDummy*) allocate( align_size( sizeof *p ) );
  object = p;
  GUARANTEE( p != NULL, "Large object dummy allocation failed" );
  p->signature = expected_signature;
  p->p1 = &p->p2;
  p->p2 = &p->p1;
  *( p->bitvector() ) = expected_bitvector;
  verify();
}

void LargeObjectDummy::dispose( void ) {
  check();
  LargeObject* p = object;
  object = NULL;  
  p->free();
}

void LargeObjectDummy::check( void ) {
  LargeObjectDummy* p = object;
  GUARANTEE( p != NULL, "Large object dummy must be created first" );
  GUARANTEE( contains( p ),"Large object dummy outside of large object area" );
  if( p ) {
    (void) p->body(); // Check for alignment
    GUARANTEE( p->signature == expected_signature, "Wrong signature" );
    GUARANTEE( p->p1 == &p->p2, "Wrong p1" );
    GUARANTEE( p->p2 == &p->p1, "Wrong p2" );
    GUARANTEE( *( p->bitvector() ) == expected_bitvector, "Wrong bitvector" );
  }
}

#endif

#endif
#endif
