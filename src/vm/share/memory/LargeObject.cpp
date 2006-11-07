/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

void LargeObject::update_pointer( OopDesc** p ) {
  const LargeObject* const obj = (LargeObject*) *p;
  if( obj >= bottom() && obj < current_limit ) {
    *p = DERIVED( OopDesc*, obj, current_delta );
  }
}

void LargeObject::update_pointer_in_instance_class( OopDesc** p ) {
  const LargeObject* const obj = (LargeObject*) *p;
  if( obj && !((OopDesc*)obj)->is_class_info() &&
      obj >= bottom() && obj < current_limit ) {
    *p = DERIVED( OopDesc*, obj, current_delta );
  }
}

inline void LargeObject::update_pointers( OopDesc* p, OopDesc limit[] ) {
  while( p < limit ) {
    const FarClassDesc* const blueprint = p->blueprint();
    // const jint instance_size = blueprint->instance_size_as_jint();
    if (p->is_instance_class() || p->is_obj_array_class()) {
      p->oops_do_for(blueprint, update_pointer_in_instance_class);
    } else {
      p->oops_do_for( blueprint, update_pointer );
    }
    p = DERIVED( OopDesc*, p, p->object_size_for( blueprint ) );
  }
} 

inline void LargeObject::update_classinfo_pointers() {
#if ENABLE_ISOLATES
  for (int i = Task::FIRST_TASK; i < MAX_TASKS; i++) {
    Task::Raw task = Task::get_task(i);
    if (task.is_null()) {
      continue;
    }
    ObjArray::Raw class_list = task().class_list();
    for( int len = class_list().length(); --len >= 0; ) {
      JavaClass::Raw jc = class_list().obj_at(len);
      if( jc.not_null() ) {
        ClassInfo::Raw ci = jc().class_info();
        if ((LargeObject*)ci.obj() >= bottom() &&
            (LargeObject*)ci.obj() < current_limit) {
          OopDesc **p = jc.obj()->obj_field_addr(InstanceClass::class_info_offset());
          *p = DERIVED(OopDesc *, ci.obj(), current_delta);
          GUARANTEE((*p)->is_class_info(), "not class info");
        }
      }
    }
  }
#else
  GUARANTEE(ENABLE_LIB_IMAGES, "this is only for multiple images support!!!");
  ObjArray::Raw class_list = Universe::class_list()->obj();
  for( int len = class_list().length(); --len >= 0; ) {
    JavaClass::Raw jc = class_list().obj_at(len);
    if( jc.not_null() ) {
      ClassInfo::Raw ci = jc().class_info();
      if ((LargeObject*)ci.obj() >= bottom() &&
          (LargeObject*)ci.obj() < current_limit) {
        OopDesc **p = jc.obj()->obj_field_addr(InstanceClass::class_info_offset());
        *p = DERIVED(OopDesc *, ci.obj(), current_delta);
        GUARANTEE((*p)->is_class_info(), "not class info");
      }
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
#if 0 && ENABLE_ISOLATES
  {
    // First collect all owners of moved objects
    unsigned owners_of_moved_objects = 1 << SYSTEM_TASK;
    for( const LargeObject* p = src; p < limit; p = p->next() ) {
      owners_of_moved_objects |= 1 << p->task();
    }
    update_pointers( _heap_start, _inline_allocation_top );
  }
#endif
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
  LargeObject *new_bottom =
    (LargeObject*)jvm_memmove(dst, src, DISTANCE(src, limit));

  update_classinfo_pointers();

  set_bottom(new_bottom);
  if( !gc_active ) {
    Scheduler::gc_epilogue();
  }
#undef is_moving
#undef new_location
}

LargeObject* LargeObject::allocate( const unsigned size, const unsigned task ) {
  ObjectHeap::shrink_with_compiler_area( size );
  LargeObject* p = DERIVED( LargeObject*, bottom(), -int(size) );
  if( p < start() ) {
    return NULL;
  }
  set_bottom( p );
  p->set_attributes( size, task );
  jvm_memset( p->bitvector(), 0, bitvector_size( size ) );
  return p;
}

void LargeObject::free( void ) {
  if( this == NULL ) {
    return;
  }

  must_be_aligned( body() );
  GUARANTEE(contains(this), "Large object outside of large object area");

  const LargeObject* p = this;
  const unsigned task = this->task();
  const LargeObject* const limit = end();
  // A few consequent objects can belong to one task
  do {
    p = p->next();
  } while( p < limit && p->task() == task );
  move( DISTANCE( this, p ), this );
  verify();
}

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

#ifndef PRODUCT
void LargeObject::verify( void ) {
#if USE_LARGE_OBJECT_DUMMY
  LargeObjectDummy::check();
#endif
  { // References between large objects must only be up-going
    // No pointers from large objects to ObjectHeap allowed
    for( const LargeObject* obj = bottom(); obj < end(); obj = obj->next() ) {
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
    (LargeObjectDummy*) allocate( align_size( sizeof *p ), 0 );
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
