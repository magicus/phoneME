/*
 *   
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

# include "incls/_precompiled.incl"
# include "incls/_ObjectHeap.cpp.incl"

// Static variables
#if USE_IMAGE_PRELOADING
size_t    ObjectHeap::_preallocated_space_size;
#endif
#if ENABLE_COMPILER
size_t    ObjectHeap::_glue_code_size;
#endif

address   ObjectHeap::_preallocated_space;
address   ObjectHeap::_glue_code;
address   ObjectHeap::_heap_chunk = NULL;
address   ObjectHeap::_bitv_chunk = NULL;
address   ObjectHeap::_bitvector_start;

bool      ObjectHeap::_is_gc_active;
bool      ObjectHeap::_last_heap_expansion_failed;

OopDesc** ObjectHeap::_permanent_generation_top;

#ifdef AZZERT
bool      ObjectHeap::_is_finalizing;
#endif

#if ENABLE_COMPILER
OopDesc** ObjectHeap::_saved_compiler_area_top;
#endif

#if ENABLE_INTERNAL_CODE_OPTIMIZER
OopDesc** ObjectHeap::_saved_compiler_area_top_quick;
#endif 

#define CACHE_QUICK_VAR(v) _quick_vars.v = _ ## v

/*
 * This code is ugly, but we save more than 10% of full GC time by 
 * in-lining write_barrier_oops_do() calls (so that local variables
 * can be cached in the loop body, and does not need to be reloaded
 * in each function pointer invocation).
 */
#define WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p) \
  /* Compute value of last_bitvector_word_ptr */ \
  /* Compute value of next_bitvector_word_ptr, next_p and bits */ \
  OopDesc** p = align_down( start ); \
  juint* bitvector_word_ptr = get_bitvectorword_for_aligned(p); \
  juint* last_bitvector_word_ptr = get_bitvectorword_for_unaligned(end); \
  AZZERT_ONLY( \
      int trash_bits = (start - p); \
      GUARANTEE(0 <= trash_bits && trash_bits <= 31, "Sanity"); \
  ); \
  juint bitword = *bitvector_word_ptr & ~((1 << (start - p)) - 1); \
  \
  /* Iterate over bitmap from start to end. */ \
  while (true) { \
    while (bitvector_word_ptr <= last_bitvector_word_ptr && bitword == 0) { \
      /* rapidly skip over zeros */ \
      bitword = *++bitvector_word_ptr; \
    } \
    p = ObjectHeap::get_aligned_for_bitvectorword(bitvector_word_ptr); \
    if (bitvector_word_ptr < last_bitvector_word_ptr) { \
      /* This is the normal case.  Handle it first and quickly */ \
    } else if (bitvector_word_ptr == last_bitvector_word_ptr) { \
      int keep = end - p; \
      GUARANTEE(0 <= keep && keep <= 31, "Sanity"); \
      bitword &= right_n_bits(keep); \
    } else { \
      break; \
    } \
    \
    while (bitword) { \
      if ((bitword & 0xFFFF) == 0) { bitword >>= 16; p += 16; } \
      if ((bitword &   0xFF) == 0) { bitword >>=  8; p +=  8; } \
      if ((bitword &    0xF) == 0) { bitword >>=  4; p +=  4; } \
      if ((bitword &    0x3) == 0) { bitword >>=  2; p +=  2; } \
      if ((bitword &    0x1) == 0) { bitword >>=  1; p +=  1; } \
      GUARANTEE(p >= start && p < end && test_bit_for(p), "Sanity"); \
      if (TraceGC) { \
        TTY_TRACE_CR(("TraceGC: 0x%x write barrier entry", p)); \
      }

#define WRITE_BARRIER_OOPS_LOOP_END \
      bitword >>= 1; \
      p++; \
    } \
  } \
  bitword = *++bitvector_word_ptr;

inline void ObjectHeap::set_task_allocation_start( OopDesc** p ) {
#if ENABLE_ISOLATES
  _task_allocation_start = p;
#else
  (void)p;
#endif
}

#if ENABLE_ISOLATES
int       ObjectHeap::_current_task_id;
int       ObjectHeap::_previous_task_id;
OopDesc** ObjectHeap::_real_inline_allocation_end;
OopDesc** ObjectHeap::_task_allocation_start;
unsigned  ObjectHeap::_reserved_memory_deficit;
unsigned  ObjectHeap::_current_deficit;
bool      ObjectHeap::_some_tasks_terminated;

TaskMemoryInfo  ObjectHeap::_task_info [MAX_TASKS];

inline void TaskMemoryInfo::reset( void ) {
  usage     = 0;
  estimate  = 0;
  reserve   = 0;
#if ENABLE_PERFORMANCE_COUNTERS
  max_usage = 0;
#endif
  limit     = unsigned(max_jint);
}

inline int TaskMemoryInfo::unused( void ) const {
  return reserve - estimate;
}

#if ENABLE_PERFORMANCE_COUNTERS
inline void TaskMemoryInfo::compute_max_usage( void ) {
  if( max_usage < estimate ) {
    max_usage = estimate;
  }
}
#endif


#define ForTask(task) for( int task = 0; task < MAX_TASKS; task++ )

inline OopDesc** ObjectHeap::get_boundary_classes ( void ) {
  return (OopDesc**) Universe::boundary_near_list()->obj_at(0);
}

inline BoundaryDesc** ObjectHeap::get_boundary_list ( void ) {
  return (BoundaryDesc**) (persistent_handles + Universe::boundary_list_index);
}

inline int ObjectHeap::get_current_task ( void ) { return _current_task_id; }

inline int
ObjectHeap::get_owner( const BoundaryDesc* p, const OopDesc* const classes[] ) {
  const int n = (((OopDesc**) p->klass()) - (OopDesc**)classes)/(sizeof(NearDesc)/BytesPerWord);
  GUARANTEE( unsigned( n ) < unsigned( MAX_TASKS ), "sanity" );
  return n;
}

inline void ObjectHeap::create_boundary( OopDesc** p, const int task ) {
  BoundaryDesc* q = (BoundaryDesc*) p;
  q->_klass = (OopDesc*) (get_boundary_classes() + (task * (sizeof(NearDesc)/BytesPerWord)));
  BoundaryDesc** const list = ObjectHeap::get_boundary_list();
  q->_next = *list; *list = q;
}

void ObjectHeap::accumulate_memory_usage( OopDesc* _lwb[], OopDesc* _upb[] ) {
  {
    const BoundaryDesc* lwb = (const BoundaryDesc*) _lwb;
    const BoundaryDesc* upb = (const BoundaryDesc*) _upb;
    OopDesc** const classes = get_boundary_classes();

    unsigned previous_id = _previous_task_id;
    BoundaryDesc** root = get_boundary_list();
    BoundaryDesc* p = *root;

    // Do not remove lonely boundary object on the top of the heap
    while( p >= upb ) { // Optimize only
      const unsigned current_id = get_owner( p, classes );
      BoundaryDesc* next = p->_next;
      if( previous_id == current_id || (next + 1) == p ) {      
        *root = next;      
      } else {
        previous_id = current_id;
        root = &p->_next;
      }
      p = next;
    }

    while( p > lwb ) { // Optimize and accumulate
      get_task_info( previous_id ).estimate += DISTANCE( (p+1), upb );
      upb = p;

      const unsigned current_id = get_owner( p, classes );
      BoundaryDesc* next = p->_next;
      if( previous_id == current_id || (next + 1) == p ) {      
        *root = next;      
      } else {
        previous_id = current_id;
        root = &p->_next;
      }
      p = next;
    }

    get_task_info( previous_id ).estimate += DISTANCE( lwb, upb );
  }
  {
    unsigned s = 0;
    ForTask( task ) {
      const int unused = _task_info[ task ].unused();
      if( unused > 0 ) {
        s += unused;
      }
    }
    _reserved_memory_deficit = s;
  }
}

void ObjectHeap::accumulate_current_task_memory_usage( void ) {
  const int current_task_id = _current_task_id;
  TaskMemoryInfo& task_info = get_task_info( current_task_id );
  int reserved_memory_deficit = int(_reserved_memory_deficit);
  int estimate = task_info.estimate;
  const int reserve = task_info.reserve;
  {
    const int unused = reserve - estimate;
    if( unused > 0 ) {
      reserved_memory_deficit -= unused;
    }
  }
  {
    OopDesc** const task_allocation_start = _inline_allocation_top;
    GUARANTEE( task_allocation_start >= _task_allocation_start, "sanity" );
    estimate += DISTANCE( _task_allocation_start, task_allocation_start );
    _task_allocation_start = task_allocation_start;
    task_info.estimate = estimate;
  }
  {
    const int unused = reserve - estimate;
    if( unused > 0 ) {
      reserved_memory_deficit += unused;
    }
  }        
  _reserved_memory_deficit = reserved_memory_deficit;
}

OopDesc** ObjectHeap::current_task_allocation_end ( void ) {
  GUARANTEE( _inline_allocation_top == _task_allocation_start,
    "no allocations should happen here" );
  int available = free_memory() - int(_reserved_memory_deficit);
  if( available < 0 ) {
    available = 0;
  }
  const TaskMemoryInfo& task_info = get_task_info( _current_task_id );
  const int estimate = task_info.estimate;
  {
    const int unused = task_info.reserve - estimate;
    if( unused > 0 ) {
      available += unused;
    }
  }
  {
    const int unused = task_info.limit - estimate;
    if( unused < available ) {
      available = unused;
    }
  }
  available = align_size_down( available, BytesPerWord );

  OopDesc** allocation_end =
    DERIVED( OopDesc**, _inline_allocation_top, available );
  {
    OopDesc** const real_inline_allocation_end = _real_inline_allocation_end;
    if( real_inline_allocation_end < allocation_end ) {
      allocation_end = real_inline_allocation_end;
    }
  }
  GUARANTEE(allocation_end <= _compiler_area_start, "overlap");
  return allocation_end;
}

int ObjectHeap::available_for_current_task() {
  OopDesc** const allocation_end = disable_allocation_trap();
  accumulate_current_task_memory_usage();

  int available = free_memory() - (int)_reserved_memory_deficit;
  const TaskMemoryInfo& task_info = get_task_info(_current_task_id);
  const int estimate = task_info.estimate;
  {
    const int unused = task_info.reserve - estimate;
    if (unused > 0) {
      available += unused;
    }
  }
  {
    const int unused = task_info.limit - estimate;
    if (unused < available) {
      available = unused;
    }
  }

  enable_allocation_trap(allocation_end);
  GUARANTEE(available >= 0, "sanity");
  return available;
}

int ObjectHeap::on_task_switch ( const int task_id ) {
  GUARANTEE( unsigned(task_id) < unsigned(MAX_TASKS), "Invalid task id" );
  GUARANTEE( !_is_gc_active, "No task switching allowed during GC" );

  accumulate_current_task_memory_usage();

  const int current_task_id = _current_task_id;
  _current_task_id = task_id;

  _inline_allocation_end = _previous_task_id != task_id ? NULL :
    current_task_allocation_end();

  return current_task_id;
}

void ObjectHeap::on_task_termination ( OopDesc* p ) {
  const Task::Raw task( p );

#if USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA
  GUARANTEE( !dead_task, "Sanity" );
  dead_task = p;
  {
    ObjArray::Raw images = task().binary_images();
    
#if ENABLE_LIB_IMAGES
    task().remove_shared_images();
    _dead_task_images = images;    
#else //ENABLE_LIB_IMAGES
    if( images.not_null() ) {
      GUARANTEE(images().length() == 1, "sanity");
      const ROMBundle* bundle = (ROMBundle*)images().obj_at(0);
      dead_range_beg = (const OopDesc*)bundle;
      dead_range_end = (const OopDesc*)bundle->symbol_table();
    }
#endif //ENABLE_LIB_IMAGES
  }
#endif

  const int task_id = task().task_id();
#if ENABLE_PERFORMANCE_COUNTERS
  if( PrintIsolateMemoryUsage ) {
    tty->print_cr( "Max memory usage for task %u = %u",
                   task_id, get_task_info( task_id ).max_usage );
  }
#endif
  _some_tasks_terminated = true;
  reset_task_memory_usage( task_id );
}

#if ENABLE_ISOLATES && ENABLE_MONET && ENABLE_LIB_IMAGES
  bool ObjectHeap::in_dead_bundles ( const OopDesc* obj ) {    
    if (_dead_task_images == NULL) return false;
    ObjArray::Raw images = (OopDesc*)_dead_task_images;
    for (int i = 0; i < images().length(); i++) {
      const ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
      if (bundle == NULL) continue;
      const OopDesc* image_beg = (const OopDesc*)bundle;
      const OopDesc* image_end = (const OopDesc*)bundle->symbol_table();
      if (obj < image_end && obj >= image_beg) 
        return true;
    }        
    return false;
  }
#endif


void ObjectHeap::reset_task_memory_usage( const int task ) {
  TaskMemoryInfo& task_info = get_task_info( task );
  {
    const int unused = task_info.unused();
    if( unused > 0 ) {
      _reserved_memory_deficit -= unused;
    }
  }
  task_info.reset();
  _current_deficit = _reserved_memory_deficit;
}

void ObjectHeap::set_task_memory_quota( const int task_id,
                      const unsigned reserve, const unsigned limit JVM_TRAPS ) {
  const int avail = free_memory() - _reserved_memory_deficit - reserve;
#if ENABLE_COMPILER
  if( avail < 0 && (CompiledMethodCache::zero_weight_space() + avail) < 0 ) {
#else
  if( avail < 0 ) {
#endif
    // Simulate allocation of the reserve by the task
    const int current_task_id = _current_task_id;
    _current_task_id = task_id;

    force_full_collect();
    {      
      OopDesc** const allocation_end = disable_allocation_trap();
      collect( reserve JVM_NO_CHECK );
      clear_inline_allocation_area();
      set_collection_area_boundary( 0, false );
      enable_allocation_trap( allocation_end );
    }

    _current_task_id = current_task_id;

    // Undo possible side-effects of the simulated allocation
    _current_deficit = _reserved_memory_deficit;

    // Propagate the exception set by collect()
    if(CURRENT_HAS_PENDING_EXCEPTION) {
      return;
    }
  }    
  set_task_memory_reserve_limit(task_id, reserve, limit);
}

unsigned ObjectHeap::detect_out_of_memory_tasks( const size_t alloc_size ) {  
#if ENABLE_PERFORMANCE_COUNTERS
  {
    ForTask( task ) {
      _task_info[ task ].compute_max_usage();
    }
  }
#endif

  const int current_task_id = _current_task_id;
  const TaskMemoryInfo& current_task_info = get_task_info( current_task_id );
  const unsigned old_estimate = current_task_info.estimate;
  const unsigned new_estimate = old_estimate + alloc_size;
  const unsigned reserve = current_task_info.reserve;

  unsigned deficit = _reserved_memory_deficit;
  // Calculate the after-allocation deficit
  {
    int delta = int(reserve - old_estimate);
    if( delta > 0 ) {
      deficit -= delta;
      delta -= alloc_size;
      if( delta > 0 ) {
        deficit += delta;
      }
    }
  }
  _current_deficit = deficit;

  unsigned violations = 0;
  if( new_estimate > current_task_info.limit ) {
    violations = OverLimit;
  }
  if( new_estimate > reserve && (deficit + alloc_size) > free_memory() ) {
    violations |= OverReservation;
  }
  return violations;
}


#if ENABLE_PERFORMANCE_COUNTERS
void ObjectHeap::print_max_memory_usage ( void ) {
  if( PrintIsolateMemoryUsage ) {
    ForTask( task ) {
      const unsigned m = _task_info[ task ].max_usage;
      if( m ) {
        tty->print_cr( "Max memory usage for task %u = %u", task, m );
        _task_info[ task ].max_usage = 0;
      }
    }
  }
}
#endif


void ObjectHeap::safe_collect(size_t min_free_after_collection JVM_TRAPS) {
  OopDesc** const allocation_end = disable_allocation_trap();
  collect( min_free_after_collection JVM_NO_CHECK );
  clear_inline_allocation_area();
  enable_allocation_trap( allocation_end );
}


int ObjectHeap::owner_task_id( const OopDesc* const object ) {
  if( contains( object ) ) {
    GUARANTEE( contains_live( object ), "applicable to live heap objects only" );
    BoundaryDesc* p = *get_boundary_list();
    if (p < object) {
      return _previous_task_id;
    }

    BoundaryDesc* prev;
    do {
      prev = p;
      p = p->_next;
    } while (p >= object);

    return get_owner(prev, get_boundary_classes());
  }
#if USE_BINARY_IMAGE_LOADING
  ForTask( task_if ) {
    Task::Raw task = Task::get_task( task_id );
    if( task.not_null() ) {
      ObjArray::Raw images = task().binary_images();
      if( images.not_null() ) {
        for( int i = images().length(); --i >= 0; ) {
          const ROMBundle* bundle = (const ROMBundle*)images().obj_at(i);
          if( bundle->contains( obj ) ) {
            return task_id;
          }
        }
      }
    }
  }
#endif
  return MAX_TASKS;
}

#if USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA
#if ENABLE_LIB_IMAGES
const OopDesc* ObjectHeap::_dead_task_images;
#else
const OopDesc* ObjectHeap::dead_range_beg;
const OopDesc* ObjectHeap::dead_range_end;
#endif
OopDesc* ObjectHeap::dead_task;
#endif // USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA

#endif  // ENABLE_ISOLATES

inline bool ObjectHeap::compiler_area_in_use( void ) {
#if ENABLE_COMPILER
  return Compiler::is_active() || Compiler::is_suspended();
#else
  return false;
#endif
}

FinalizerConsDesc* ObjectHeap::_finalizer_reachable [ NUM_OF_FINALIZERS ];
FinalizerConsDesc* ObjectHeap::_finalizer_pending   [ NUM_OF_FINALIZERS ];

inline void ObjectHeap::init_finalizers( void ) {
  jvm_memset( _finalizer_reachable, 0, sizeof _finalizer_reachable );
  jvm_memset( _finalizer_pending,   0, sizeof _finalizer_pending   );
}

void ObjectHeap::finalizer_oops_do( FinalizerConsDesc** list,
                                    void do_oop(OopDesc**) ) {
  // Iterate over all elements in the finalization list
  for( int i = 0; i < NUM_OF_FINALIZERS; list++, i++ ) {
    FinalizerConsDesc** pp = list;
    do_oop((OopDesc**) pp);
    for( FinalizerConsDesc* p; (p = *pp) != NULL; ) {
      pp = p->next_addr();
      do_oop((OopDesc**) pp );
      do_oop((OopDesc**) p->referent_addr());
    }
  }
}

inline void ObjectHeap::mark_finalizers( FinalizerConsDesc** list ) {
  finalizer_oops_do( list, mark_root_and_stack );
}

void ObjectHeap::update_interior_pointers( FinalizerConsDesc** list ) {
  for( int i = 0; i < NUM_OF_FINALIZERS; list++, i++ ) {
    update_interior_pointer((OopDesc **) list);
  }
}

void ObjectHeap::register_finalizer_reachable_object(Oop* referent JVM_TRAPS) {
  // Link finalizer reachable objects using cons cells. For now we
  // just use a 2-element object array as the cons cell.
  FinalizerConsDesc* cons = (FinalizerConsDesc*)
    Universe::new_obj_array(FinalizerConsDesc::Size JVM_CHECK);
  cons->set_referent(referent->obj());

#if ENABLE_ISOLATES
  const int task_id = TaskContext::current_task_id();
  GUARANTEE( unsigned(task_id) < MAX_TASKS, "Invalid task id" );
#else
  const int task_id = 0;
#endif
  cons->set_next( _finalizer_reachable[task_id] );
  _finalizer_reachable[task_id] = cons;

  if (TraceFinalization) {
    TTY_TRACE_CR(("TraceGC: register finalizer 0x%x", referent->obj()));
  }
}

inline void ObjectHeap::discover_finalizer_reachable_objects() {
  // Iterate over finalizer reachable lists, keeping marked object on
  // the reachable list and moving unmarked objects to the pending list.
  OopDesc** const bottom = _collection_area_start;
  for( int i = 0; i < NUM_OF_FINALIZERS; i++ ) {
    FinalizerConsDesc* pending = NULL;
    FinalizerConsDesc** pp = _finalizer_reachable + i;
    for( FinalizerConsDesc* p;
         (p = *pp) != NULL && p->referent() >= (FinalizerConsDesc*) bottom; ) {
      GUARANTEE(!test_bit_for((OopDesc**) p),
        "cons cell should not be marked yet");
      OopDesc** referent = (OopDesc**) p->referent();
      if( test_bit_for( referent ) ) {
        // Referent is reachable, keep on reachable list
        pp = p->next_addr();
        if (TraceFinalization) {
          TTY_TRACE_CR(("TraceGC: finalizer 0x%x reachable", referent));
        }
      } else {
        // Referent is not reachable, move to pending list
        *pp = p->next();
        *(p->next_addr()) = pending;
        pending = p;
        if (TraceFinalization) {
          TTY_TRACE_CR(("TraceGC: finalizer 0x%x now pending", referent));
        }
      }
    }
    _finalizer_pending[i] = pending;
  }
}

inline void ObjectHeap::unmark_pending_finalizers( void ) {
  address bitvector_base = _bitvector_base;
  for( int i = 0; i < NUM_OF_FINALIZERS; i++ ) {
    for( FinalizerConsDesc* p = _finalizer_pending[i]; p; p = p->next() ) {
      clear_bit_for( (OopDesc**) p->referent(), bitvector_base );
    }
  }
}

void ObjectHeap::finalize( FinalizerConsDesc** list, const int task_id ) {
  list += task_id;

  FinalizerConsDesc* p = *list;
  if( p ) {
#if ENABLE_ISOLATES
    // We must switch to the context of the task, as the native finalizers
    // may execute things like KNI_GetStaticIntField().

    // Thread::current_task_id may differ from ObjectHeap::_current_task
    // Tasks with pending finalizers should not be cleaned up
    GUARANTEE( Universe::task_from_id(task_id) != NULL,
               "cannot run pending finalizers");
    TaskGCContext tmp(task_id);
#endif
    do {
      AZZERT_ONLY(_is_finalizing = true);
      p->run_finalizer();
      AZZERT_ONLY(_is_finalizing = false);
    } while( (p = p->next()) != NULL );
    *list = p;
  }
}

void ObjectHeap::finalize( FinalizerConsDesc** list ) {
  for( int i = 0; i < NUM_OF_FINALIZERS; i++ ) {
    finalize( list, i );
  }
}

#if ENABLE_PERFORMANCE_COUNTERS || ENABLE_TTY_TRACE
jlong     ObjectHeap::_internal_collect_start_time;
size_t    ObjectHeap::_old_gen_size_before;
size_t    ObjectHeap::_young_gen_size_before;
#endif

#ifndef PRODUCT
OopDesc** ObjectHeap::_heap_start_bitvector_verify;
int       ObjectHeap::_excessive_gc_countdown;

jint      AllocationDisabler__disabling_count = 0;
bool      AllocationDisabler__suspended = false;
OopDesc** AllocationDisabler::_current_allocation_top = NULL;
jint      GCDisabler__disabling_count = 0;

extern "C" {
 int       _jvm_in_raw_pointers_block       = 0;
}

bool oop_check_barrier(OopDesc** addr) {
  return _heap_start <= addr && addr < _old_generation_end &&
         ObjectHeap::test_bit_for(addr);
}
bool oop_in_old_space(OopDesc** addr) {
  return _heap_start <= addr && addr < _old_generation_end;
}
#endif

// Utility class describing a live range
// If two dead words are available the format is:
//   next_live
//   next_dead
// If only one dead word is available the format is:
//   next_dead | 0x1
// in which case next_live implicitly is this+1.

class LiveRange: public StackObj {
 private:
  OopDesc** _position;
 public:
  LiveRange(OopDesc** position) : _position(position) {};
  void set_next_live(OopDesc** p);
  void set_next_dead(OopDesc** p);
  void get_range(OopDesc** &next_live, OopDesc** &next_dead);
};

void LiveRange::set_next_live(OopDesc** p) {
  *_position = (_position + 1 == p) ? (OopDesc*) 0x1 : (OopDesc*) p;
}

void LiveRange::set_next_dead(OopDesc** p) {
  size_t first_word = (size_t) *_position;
  if (first_word == 0x1) {
    _position[0] = (OopDesc*) ((size_t) p | 0x1);
  } else {
    _position[1] = (OopDesc*) p;
  }
}

void LiveRange::get_range(OopDesc** &next_live, OopDesc** &next_dead) {
  size_t first_word = (size_t) *_position;
  if (first_word & 0x1) {
    next_live = _position + 1;
    next_dead = (OopDesc**) (first_word & ~0x1);
  } else {
    next_live = (OopDesc**) first_word;
    next_dead = (OopDesc**) *(_position+1);
  }
}

#ifdef AZZERT
void ObjectHeap::nuke_raw_handles() {
  if (Universe::before_main()) {
    return;
  }
  Oop::Raw* ptr;
  for (ptr = (Oop::Raw*) last_raw_handle; ptr; ptr=ptr->_previous) {
    if (((Oop*)ptr)->not_null()) {
      ((Oop*)ptr)->set_obj((OopDesc*)0xdeadc0de);
    }
  }
}
#endif

inline void ObjectHeap::global_refs_do(void do_oop(OopDesc**), const int mask) {
#if ENABLE_ISOLATES
  ForTask( task ) {
    Task::Raw t = Task::get_task(task);
    if( t.not_null() ) {
      RefArray::Raw refs = t().global_references();
      refs().oops_do( do_oop, mask );
    }
  }
#else
  RefArray::current()->oops_do( do_oop, mask );
#endif
}

int ObjectHeap::register_global_ref_object(Oop* referent,
                                           ReferenceType type JVM_TRAPS) {
  const int i = RefArray::current()->add(referent, type JVM_MUST_SUCCEED);
  return make_global_reference( i );
}

OopDesc* ObjectHeap::get_global_ref_object(const int ref) {
#if ENABLE_ISOLATES
  Task::Raw task = Task::get_task( get_global_reference_owner(ref) );
  GUARANTEE( task.not_null(), "Wrong task" );
  RefArray::Raw refs = task().global_references();
#else
  RefArray::Raw refs = RefArray::current();
#endif
  return refs().get( get_global_reference_index( ref ) );
}

void ObjectHeap::unregister_global_ref_object(const int ref) {
#if ENABLE_ISOLATES
  Task::Raw task = Task::get_task( get_global_reference_owner(ref) );
  GUARANTEE( task.not_null(), "Wrong task" );
  RefArray::Raw refs = task().global_references();
#else
  RefArray::Raw refs = RefArray::current();
#endif
  refs().remove( get_global_reference_index( ref ) );
}

#if !defined(AZZERT) && !ENABLE_ISOLATES
OopDesc* ObjectHeap::allocate_raw(size_t size JVM_TRAPS) {
  if (GenerateROMImage && USE_SOURCE_IMAGE_GENERATOR) {
    // Some licensees require that ROMImage.cpp must be the exactly same
    // different romization runs. In this case, we disable raw allocation
    // altogether, or else some romized Symbols may contain random bits
    // in unused parts.
    return ObjectHeap::allocate(size JVM_NO_CHECK_AT_BOTTOM);
  } 
  OopDesc** inline_top = _inline_allocation_top;
  OopDesc** inline_end = _inline_allocation_end;
  OopDesc** new_top = DERIVED(OopDesc**, inline_top, size);

  if (new_top <= inline_end && new_top >= inline_top) {
    PERFORMANCE_COUNTER_INCREMENT(num_of_c_alloc_objs, 1);
    _inline_allocation_top = new_top;
    return (OopDesc*)inline_top;
  } else {
    // Too complicated to handle here. Let's defer to ObjectHeap::allocate().
    return ObjectHeap::allocate(size JVM_NO_CHECK_AT_BOTTOM);
  }
}
#endif

OopDesc* ObjectHeap::clone(OopDesc* source JVM_TRAPS) {
  size_t size = source->object_size();
  UsingFastOops allocation_below;
  Oop::Fast source_handle = source;
  OopDesc* result = allocate_raw(size JVM_ZCHECK_0(result));
  // Don't care about write_barrier - the result object is always in youngen
  jvm_memcpy(result, source_handle.obj(), size);
  return result;
}
  
OopDesc* ObjectHeap::allocate(size_t size JVM_TRAPS) {
  AZZERT_ONLY(nuke_raw_handles());

  GUARANTEE(_jvm_in_raw_pointers_block == 0,
            "no GC should happen in SNI_BEGIN_RAW_POINTERS block!")
  GUARANTEE(!_jvm_in_quick_native_method, "cannot GC in quick native methods");

  // Check alignment
  GUARANTEE(align_allocation_size(size) == size,
            "Size must be allocation aligned");
  GUARANTEE(AllocationDisabler::allocation_okay(),
            "Allocation must be enabled");

#if !defined( PRODUCT )
  if (ExcessiveGC && GCDisabler::gc_okay() && !Universe::before_main()) {
    if (_excessive_gc_countdown <= 1) {
      safe_collect(size JVM_CHECK_0);
      _excessive_gc_countdown = ExcessiveGC - 1;
    } else {
      _excessive_gc_countdown --;
    }
  }
#endif

  GUARANTEE((int)_inline_allocation_top % 4 == 0, "Sanity");
  PERFORMANCE_COUNTER_INCREMENT(num_of_c_alloc_objs, 1);

#if ENABLE_ISOLATES
  const int previous_task_id = _previous_task_id;
  const int current_task_id = get_current_task();
  if( current_task_id != previous_task_id ) {
    size += sizeof(BoundaryDesc);
  }

  OopDesc** const saved_inline_end = _inline_allocation_end;
#endif

#ifdef AZZERT
  int loop_check = 0;
#endif

  for(;;) {
    GUARANTEE(loop_check++ < 2, "No more than 2 iterations");

    // Prefetch the globals
    OopDesc** inline_top = _inline_allocation_top;
    OopDesc** new_top = DERIVED(OopDesc**, inline_top, size);
    if( new_top < inline_top ) {
      // (new_top < _inline_allocation_top) means that we wrapped around
      // 32-bit boundary
      Throw::out_of_memory_error( JVM_SINGLE_ARG_THROW_0 );
    }

    OopDesc** inline_end = _inline_allocation_end;
#if ENABLE_ISOLATES
    // First allocation after task switching
    if( inline_end == NULL ) {
      inline_end = current_task_allocation_end();
      _inline_allocation_end = inline_end;
    }
#endif

    if (new_top <= inline_end) {
      // Allocate the object
      _inline_allocation_top = new_top;

#if ENABLE_REMOTE_TRACER
      if (RemoteTracePort > 0) {
        RemoteTracer::set_heap_size(total_memory());
        RemoteTracer::set_used_memory(used_memory());
        RemoteTracer::set_free_memory(free_memory());
      }
#endif

#if ENABLE_ISOLATES
      if( current_task_id != previous_task_id ) {
        GUARANTEE( unsigned(current_task_id) < unsigned(MAX_TASKS),"sanity");
        _previous_task_id = current_task_id;
        create_boundary( inline_top, previous_task_id );
        inline_top += sizeof(BoundaryDesc)/BytesPerWord;
        size -= sizeof(BoundaryDesc);        
      }
#endif
#if !ENABLE_ZERO_YOUNG_GENERATION
      jvm_memset(inline_top, 0, size);
#endif
      return (OopDesc*)inline_top;
    }
#if ENABLE_ISOLATES
    _inline_allocation_end = _real_inline_allocation_end;
#endif
    collect(size JVM_NO_CHECK);
    clear_inline_allocation_area();
#if ENABLE_ISOLATES
    _inline_allocation_end = current_task_allocation_end();
#endif
    if( CURRENT_HAS_PENDING_EXCEPTION ) {
#if ENABLE_ISOLATES
      // If necessary, re-enable allocation trap
      if( saved_inline_end == NULL ) {
        _inline_allocation_end = saved_inline_end;
      }
#endif
      return NULL;
    }
  }
}

#if ENABLE_COMPILER

// Layout of the compiler area:
//
// [1] When there's no active or suspended compilation:
//     <--------------------_compiler_area_start
//     [CompiledMethod #0]
//     [CompiledMethod #1]
//     [CompiledMethod #...]
//     <--------------------_compiler_area_top
//
// [2] When there IS an active or suspended compilation:
//     <--------------------_compiler_area_start
//     [Old CompiledMethod #0]
//     [Old CompiledMethod #1]
//     [Old CompiledMethod #...]
//     <--------------------_saved_compiler_area_top
//     [New CompiledMethod, under construction]
//     <--------------------_compiler_area_temp_bottom
//     ... empty space ...
//     <--------------------_compiler_area_temp_top
//     [temp object#...]
//     [temp object#2]
//     [temp object#1]
//     [temp object#0]: CodeGenerator
//     <--------------------_compiler_area_top == _heap_top
//
// Note that in case [2], the temp objects are allocated from the top of
// the compiler area downwards. This makes it possible to collect the
// old CompiledMethods if the New CompiledMethod needs more space to
// compile. Empty space shrinks when new CompiledMethod expands or
// temp objects are allocated.
OopDesc* ObjectHeap::compiler_area_allocate_code(const int size JVM_TRAPS) {
  GUARANTEE(_compiler_area_temp_bottom == NULL, 
            "no temp objects can be alive when allocating new compiled code");
  {
    const int free_bytes = DISTANCE(_compiler_area_top, compiler_area_end());
    if( free_bytes < size ) {
      const size_t slack = _heap_size / 32 + 4 * 1024;
      if( compiler_area_soft_collect(size + slack) < size ) {
        Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
      }
    }
  }

  PERFORMANCE_COUNTER_INCREMENT(num_of_c_alloc_objs, 1);

  // [1] Allocate new CompiledMethod.
  OopDesc* const result = (OopDesc*)_compiler_area_top;
  OopDesc** const end = compiler_area_end();
  GUARANTEE( DISTANCE(result, end) >= (int)size, "sanity" );

  // [2] Set compiler temporary data boundaries
  compiler_area_initialize( DERIVED(OopDesc**, result, size), end );

  // [3] Clear the new CompiledMethod.
  return (OopDesc*) jvm_memset(result, 0, size);
}

OopDesc* ObjectHeap::compiler_area_allocate_temp(const int size JVM_TRAPS){
  if( free_memory_for_compiler_without_gc() < size ) {
    // IMPL_NOTE: in the future we should consider
    // (1) evicting and compacting compiled methods
    // (2) shifting the compiler_area downwards while pinning all temp objects
    // or continue allocating temp object below compiler_area_start
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

  OopDesc** p = DERIVED(OopDesc**, _compiler_area_temp_top, -size);
  _compiler_area_temp_top = p;
  GUARANTEE( _compiler_area_temp_bottom <= _compiler_area_temp_top, "sanity" );

  PERFORMANCE_COUNTER_INCREMENT(num_of_c_alloc_objs, 1);

  return (OopDesc*)jvm_memset((void*)p, 0, size);
}

bool ObjectHeap::expand_current_compiled_method(const int delta) {
  // This function actually just shrinks the space above
  // the current compiled method. It's up to CompiledMethod::expand()
  // to modify the compiled method's contents.
  GUARANTEE(align_allocation_size(delta) == (size_t)delta, "must be aligned");

  if( free_memory_for_compiler_without_gc() < delta ) {
    return false;
  }

  _compiler_area_temp_bottom =
    DERIVED(OopDesc**, _compiler_area_temp_bottom, delta);

  PERFORMANCE_COUNTER_INCREMENT(num_of_c_alloc_objs, 1);
  return true;
}

#endif

void ObjectHeap::dispose() {
  _inline_allocation_top = NULL;
  set_inline_allocation_end(NULL);
  _bitvector_base        = NULL;

  if (_heap_chunk != NULL) {
    OsMemory_free_chunk(_heap_chunk);
    _heap_chunk = NULL;
  }
  if (_bitv_chunk != NULL) {
    OsMemory_free_chunk(_bitv_chunk);
    _bitv_chunk = NULL;
  }

  _heap_start            = NULL;
  _collection_area_start = NULL;
  _collection_area_end   = NULL;
  _young_generation_start= NULL;
  _old_generation_end    = NULL;
  _heap_top              = NULL;
  _heap_limit            = NULL;
  _heap_end              = NULL;
  _bitvector_start       = NULL;
  _slices_start          = NULL;
  // IMPL_NOTE: not all of these are necessary ....

#if ENABLE_COMPILER
  _saved_compiler_area_top   = NULL;
  _compiler_area_temp_top    = NULL;
  _compiler_area_temp_bottom = NULL;
#endif

  LargeObjectDummy::uninitialize();

  _slice_shift       = 0;
  _slice_offset_bits = 0;
  _slice_size        = 0;
  _nof_slices        = 0;

  _near_mask         = 0;
  _slice_offset_mask = 0;
  _young_generation_target_size = 0;

  _marking_stack_start   = NULL;
  _marking_stack_top     = NULL;
  _marking_stack_end     = NULL;
  _compaction_start      = NULL;
  _compaction_top        = NULL;
  _end_fixed_objects     = NULL;

  _marking_stack_overflow = false;
  _is_gc_active = false;
  _last_heap_expansion_failed = false;

#ifdef AZZERT
  GCDisabler__disabling_count = 0;
  AllocationDisabler__disabling_count = 0;
  AllocationDisabler__suspended = false;
  _is_finalizing = false;
#endif

  init_finalizers();

#ifndef PRODUCT
  _heap_start_bitvector_verify = NULL;
#endif

}

bool ObjectHeap::create() {
#if USE_SET_HEAP_LIMIT
  HeapMin = HeapCapacity;
#endif 

#if !ENABLE_COMPILER
  // safety measure
  CompilerAreaPercentage = 0;
  CompilerAreaSlack = 0;
#else
  /*
   * UseCompiler is false for source romizer, but we still need compiler area
   * for AOT compilation. See 6432131.
   */
  if( !UseCompiler && !GenerateROMImage ) {
    CompilerAreaPercentage = 0;
    CompilerAreaSlack = 0;
  }

  if (CompilerAreaPercentage < 0) {
    CompilerAreaPercentage = 0;
  } else if (CompilerAreaPercentage > 99) {
    CompilerAreaPercentage = 99;
  }

  if( CompilerAreaSlack < 0 ) {
    CompilerAreaSlack = 0;
  }

  GUARANTEE(!YoungGenerationAtEndOfHeap, "sanity");
#endif

#ifndef PRODUCT
  _jvm_in_raw_pointers_block = 0;
  _excessive_gc_countdown = 0;
#endif

#if ENABLE_ISOLATES
  _current_task_id          = 0;
  _previous_task_id         = SYSTEM_TASK;
  _current_deficit          = 0;
  _reserved_memory_deficit  = 0;
  _some_tasks_terminated    = false;

  {
    TaskMemoryInfo* p = _task_info;
    ForTask( task ) {
      p++->reset();
    }
  }
#endif

  if (HeapMin > HeapCapacity) { // Just for convenience.
    HeapMin = HeapCapacity;
  }
  _heap_min = align_up(HeapMin);
  _heap_capacity = align_down(HeapCapacity);
  if (_heap_min > _heap_capacity) {
      _heap_min = _heap_capacity;
  }
#if USE_IMAGE_PRELOADING
  _preallocated_space_size = align_up(_preallocated_space_size);
  if (_preallocated_space_size < (size_t)_heap_capacity) {
    _heap_capacity -= (int)_preallocated_space_size;
    if (_heap_min > _heap_capacity) {
      _heap_min = _heap_capacity;
    }
  } else {
    _preallocated_space_size = 0;
  }
#endif

#if USE_COMPILER_GLUE_CODE
  //  GUARANTEE(size_t(&compiler_glue_code_end)>=size_t(&compiler_glue_code_start), "sanity");
  // Using newer GCC 4.1 and 'C' interpreter these labels are backwards
  if (size_t(&compiler_glue_code_end) <= size_t(&compiler_glue_code_start)) {
    _glue_code_size = 0;
  } else {
    _glue_code_size = align_up(size_t(&compiler_glue_code_end) -
                               size_t(&compiler_glue_code_start));
  }
  if (_glue_code_size < (size_t)_heap_capacity) {
    _heap_capacity -= (int)_glue_code_size;
  } else {
    _glue_code_size = 0;
  }
#endif

  if (_heap_min > _heap_capacity) {
    _heap_min = _heap_capacity;
  }

  int min_heap_size = 0;
#if ENABLE_ISOLATES
  const int reserved = align_up( ReservedMemory );
#else
  const int reserved = 0;
#endif

  if( min_heap_size < reserved ) {
      min_heap_size = reserved;
  }
  
  if (UseROM) {
    // We need enough space to load the HEAP_BLOCK, plus a few kilos for
    // loading the first thread.
    int min_size_for_rom = _rom_heap_block_size + reserved + 7 * 1024;
#if ENABLE_ISOLATES
    // Note: in SVM mode we use the class_list in the ROM, so we don't need
    // to allocate it.
    min_size_for_rom += _rom_number_of_java_classes * 4; // class_list
#if ENABLE_PREINITED_TASK_MIRRORS
    min_size_for_rom += _rom_task_mirrors_size; 
#else
    min_size_for_rom += _rom_number_of_java_classes * 4; // mirror_list
#endif
#endif

#if ENABLE_JAVA_DEBUGGER
    if (JavaDebugger::is_debugger_option_on()) {
      min_size_for_rom += 100 * 1024;
    }
#endif
    if (min_heap_size < min_size_for_rom) {
        min_heap_size = min_size_for_rom;
    }
  }

#if !defined (PRODUCT) || ENABLE_TTY_TRACE
  int regular_space_min = min_heap_size;
#endif

  if (CompilerAreaPercentage > 0) {
    int perc = 100 - CompilerAreaPercentage;
    int new_min_heap_size = (min_heap_size + perc - 1) / perc * 100 + perc;
#ifdef AZZERT
    int comp_size = new_min_heap_size * CompilerAreaPercentage / 100;
    GUARANTEE(new_min_heap_size - comp_size >= min_heap_size, "sanity");
#endif
    min_heap_size = new_min_heap_size;
  }

  if (min_heap_size < _heap_min) {
      min_heap_size = _heap_min;
  }

  if (min_heap_size > _heap_capacity) {
    // On some platforms we cannot grow HeapCapacity (heap may be statically
    // allocated. At this point we don't have enough minimum heap to load
    // the ROM image, so don't bother starting up the heap
    tty->print_cr("HeapCapacity (%d bytes) too low to start VM: need %d bytes.",
                  HeapCapacity, align_up(min_heap_size));
    tty->print("[ROM=%d, ", _rom_heap_block_size);
#if ENABLE_ISOLATES
    tty->print("Task#1=%d, ", ReservedMemory);
#endif
    tty->print_cr("Compiler=%d, boot=remainder]", 
                  align_up(min_heap_size) * CompilerAreaPercentage / 100);
    return false;
  }

  size_t initial_size = OsMemory_heap_initial_size(min_heap_size,
                                                   _heap_capacity);
  GUARANTEE(initial_size >= (size_t)min_heap_size, "sanity");
  GUARANTEE(initial_size <= (size_t)_heap_capacity, "sanity");

  if (!adjust_heap_size(initial_size)) {
    // mmap or malloc probably failed - can't continue without heap
    tty->print_cr("Could not allocate VM heap, try smaller HeapCapacity.");
    return false;
  }

  if (VerboseGC || TraceGC || TraceHeapSize) {
    TTY_TRACE_CR(("initial heap size        = %dK", _heap_size/1024));
    TTY_TRACE_CR(("young gen size           = %dK", 
                  DISTANCE(_heap_start, _inline_allocation_end)/1024));
    TTY_TRACE_CR(("young gen min (estimate) = %dK", regular_space_min/1024));
#if ENABLE_COMPILER
    TTY_TRACE_CR(("compiler area            = %dK", compiler_area_size()/1024));
#endif
  }

  GUARANTEE_R(DISTANCE(_heap_start, _inline_allocation_end)>= regular_space_min,
            "bad initial heap size calculation");

  _preallocated_space = address(_heap_chunk);
  _glue_code = DERIVED(address, _preallocated_space, _preallocated_space_size);

#if USE_COMPILER_GLUE_CODE
  if (_glue_code_size > 0) {
    size_t copy_size = size_t(&compiler_glue_code_end) -
                       size_t(&compiler_glue_code_start);
    jvm_memcpy(_glue_code, address(&compiler_glue_code_start), copy_size);
    OsMisc_flush_icache(_glue_code, copy_size);
  }
#endif

  return true;
}

void ObjectHeap::rom_init_heap_bounds(OopDesc **init_heap_bound, 
                                      OopDesc **permanent_top) {
  _collection_area_start   = init_heap_bound;
  _old_generation_end      = init_heap_bound;
  _young_generation_start  = init_heap_bound;
  _permanent_generation_top = permanent_top;
}

size_t ObjectHeap::update_slices_size(size_t object_heap_size) {
  GUARANTEE(object_heap_size > 0, "Invalid heap size");
  must_be_aligned( object_heap_size );

  // Determine number of bits needed for representing near pointer relative
  // to heap start
  size_t near_bits = 0;
  size_t s;
  for (s = (object_heap_size - 1) >> 2; s != 0; s >>= 1) {
    near_bits++;
  }

  int rom_max_offset = ROM::get_max_offset();
  size_t rom_near_bits = 0;
  for (s = rom_max_offset; s != 0; s >>= 1) {
    rom_near_bits++;
  }

  if (near_bits < rom_near_bits) {
    near_bits = rom_near_bits;
  }
  _near_mask = right_n_bits(near_bits);

  // The highest bit indicates if the near object lives in ROM
  _slice_shift = near_bits;
  _slice_offset_bits = BitsPerWord - _slice_shift - 1;
  _slice_offset_mask = (~_near_mask) & 0x7fffffff;
  _slice_size = 1 << _slice_offset_bits;

  size_t slice_size_in_bytes = _slice_size * BytesPerWord;
  _nof_slices  =
      (object_heap_size + slice_size_in_bytes - 1) / slice_size_in_bytes;
  size_t slices_size = _nof_slices * sizeof(OopDesc**);

  return slices_size;
}

inline void ObjectHeap::save_heap_config( SavedHeapConfig& state ) {
  // Save old global values that may be changed
  state._old_near_mask          = _near_mask;
  state._old_slice_shift        = _slice_shift;
  state._old_slice_offset_bits  = _slice_offset_bits;
  state._old_slice_offset_mask  = _slice_offset_mask;
  state._old_slice_size         = _slice_size;
}

inline void ObjectHeap::restore_heap_config( const SavedHeapConfig& state ) {
  // This function is called when a heap expansion fails.
  _near_mask                 = state._old_near_mask;
  _slice_shift               = state._old_slice_shift;
  _slice_offset_bits         = state._old_slice_offset_bits;
  _slice_offset_mask         = state._old_slice_offset_mask;
  _slice_size                = state._old_slice_size;
}

bool ObjectHeap::adjust_heap_size(size_t target_heap_size) {
  target_heap_size = align_up(target_heap_size);
  GUARANTEE(target_heap_size > 0, "Invalid heap size");
  must_be_aligned( target_heap_size );

  const size_t minimum_marking_stack_size =
      MimimumMarkingStackSize * sizeof(OopDesc *);
  must_be_aligned(minimum_marking_stack_size);

  const size_t bitvector_size = target_heap_size/BitsPerWord;

  // Note: the order of the next two lines is important (each call changes
  // _near_mask, etc.)
  const size_t max_slices_size = update_slices_size(_heap_capacity);
  const size_t slices_size = update_slices_size(target_heap_size);

  const size_t extra_size = _preallocated_space_size + _glue_code_size;
  const size_t heap_chunk_size = 
     extra_size + target_heap_size + minimum_marking_stack_size;
  const size_t bitv_chunk_size = bitvector_size + slices_size;

  const bool is_new_heap = (_heap_chunk == NULL);
  if (is_new_heap) {
    size_t max_heap, max_bitv;
    if (_heap_min == _heap_capacity) {
      // No need to grow or shrink heap
      max_heap = heap_chunk_size;
      max_bitv = bitv_chunk_size;
    } else {
      // max_bitv is an over-estimate, but should be OK because we're
      // only reserving virtual address space. If you OS doesn't have a
      // distinction between 'reserving' and 'committing' memory, you should
      // always set HeapMin=_heap_capacity.
      max_heap = extra_size + _heap_capacity + minimum_marking_stack_size;
      max_bitv = _heap_capacity/BitsPerWord + max_slices_size;

      // This should always hold up, unless the calculation for
      // slices_size is changed.
      GUARANTEE(max_bitv >=  bitv_chunk_size, "sanity");
    }

    _heap_chunk =
        (address)OsMemory_allocate_chunk(heap_chunk_size, max_heap, alignment);
    _bitv_chunk =
        (address)OsMemory_allocate_chunk(bitv_chunk_size, max_bitv, alignment);
    if (_heap_chunk == NULL || _bitv_chunk == NULL) {
      return false;
    }
  } else {
    if (VerboseGC || TraceGC || TraceHeapSize) {
      TTY_TRACE_CR(("HEAP_SIZE %dK -> %dK", _heap_size/1024, 
                    target_heap_size/1024));
    }

    SavedHeapConfig saved_state;
    save_heap_config(saved_state);

    const size_t old_heap_size =
      OsMemory_adjust_chunk(_heap_chunk, heap_chunk_size);
    const size_t old_bitv_size =
      OsMemory_adjust_chunk(_bitv_chunk, bitv_chunk_size);
    if (old_heap_size == 0 || old_bitv_size == 0) {
      // At least one adjustment failed. Restore old size
      if (old_heap_size != 0) {
        OsMemory_adjust_chunk(_heap_chunk, old_heap_size);
      }
      if (old_bitv_size != 0) {
        OsMemory_adjust_chunk(_bitv_chunk, old_bitv_size);
      }
      // if we fail to adjust the heap size, all globals should remain
      // unchanged.
      restore_heap_config(saved_state);

      _last_heap_expansion_failed = true;
      return false;
    } else {
      _last_heap_expansion_failed = false;
    }
  }

  must_be_aligned( unsigned(_heap_chunk));
  must_be_aligned( unsigned(_bitv_chunk));

  _heap_size       = target_heap_size;
  _heap_start      = DERIVED(OopDesc**, _heap_chunk, extra_size);

  OopDesc** const heap_top = _heap_start + _heap_size/BytesPerWord;
  _heap_top         = heap_top;
  _heap_limit       = heap_top + MimimumMarkingStackSize;
  _heap_end         = _heap_limit;
  _bitvector_start  = (address) _bitv_chunk;
  _slices_start     = (OopDesc***) (_bitvector_start + bitvector_size);

  _bitvector_base =
      _bitvector_start - (((uintptr_t)_heap_start >> 2) / BitsPerByte);

  _young_generation_target_size = 
     align_size_up(_heap_size/YoungGenerationTarget, BytesPerWord);
  if (is_new_heap) {
    LargeObject::initialize((LargeObject*)heap_top);
    // set allocation area to cover entire heap while bootstrapping, see
    // set_collection_area_boundary()
    {
      OopDesc** const inline_allocation_top = _heap_start;
      _inline_allocation_top  = inline_allocation_top;
      set_task_allocation_start( inline_allocation_top );
      _collection_area_start  = inline_allocation_top;
      _old_generation_end     = inline_allocation_top;
      _young_generation_start = inline_allocation_top;
    }
    {
      const size_t compiler_area_size =
        align_size_up(_heap_size * CompilerAreaPercentage / 100, BytesPerWord);
      OopDesc** const inline_allocation_end =
        DERIVED(OopDesc**, compiler_area_end(), -int(compiler_area_size));
      _compiler_area_start    = inline_allocation_end;
      _compiler_area_top      = inline_allocation_end;
      set_inline_allocation_end( inline_allocation_end );
    }
    clear_bit_range(_heap_start, compiler_area_end());

    NOT_PRODUCT(_heap_start_bitvector_verify = _inline_allocation_top;)
    DIRTY_HEAP(_heap_start, DISTANCE(_heap_start, _heap_top));
    clear_inline_allocation_area();

    LargeObjectDummy::create();
  } else {
    allocation_trap_must_be_disabled();
    // compact_and_move_compiler_area() should have already set all
    // the boundaries correctly.
    GUARANTEE(_compiler_area_top <= compiler_area_end(), "sanity");
  }

  GUARANTEE((address)get_bitvectorword_for_aligned(_heap_start)
                  == _bitvector_start, "sanity check");
  GUARANTEE((address)get_bitvectorword_for_aligned(_heap_top)
                == _bitvector_start + bitvector_size, "sanity check");

  if (VerboseGC) {
    ObjectHeap::print();
  }

  verify_layout();
  return true;
}

void ObjectHeap::force_full_collect() {
  _collection_area_start = _heap_start;
}

inline void ObjectHeap::set_collection_area_boundary_reuse(void) {
  // Let's reuse the same young space, rather than sliding it upwards.
#ifndef PRODUCT
  // so GUARANTEES in Oop::XXX_field_put() can catch overwrites
  // of object fields
  clear_bit_range(_collection_area_start, compiler_area_end());
#endif
  ExecutionStackDesc *this_stack = ExecutionStackDesc::_stack_list;
  while (this_stack) {
    if ((address)this_stack < (address)_collection_area_start) {
      int size = this_stack->object_size();
      OopDesc** start =
        (OopDesc**)this_stack->field_base(ExecutionStackDesc::header_size());
      OopDesc** end  =  (OopDesc**)this_stack->field_base(size);
      clear_bit_range(start, end);
    }
    this_stack = this_stack->_next_stack;
  }
}

inline void ObjectHeap::set_collection_area_boundary_no_reuse(
                          size_t min_free_after_collection)
{
  size_t target_size =
      (_young_generation_target_size + min_free_after_collection);
  size_t available_size = 
      DISTANCE(_inline_allocation_top, _compiler_area_start);
  target_size = (target_size + 3) & ~3;

  if (target_size > available_size) {
    target_size = available_size;
  }
  if (!YoungGenerationAtEndOfHeap) {
    OopDesc** p = _inline_allocation_top;
    set_inline_allocation_end( DERIVED(OopDesc**, p, target_size) );
    _old_generation_end    = p;
    _collection_area_start = p;
  } else {
    _old_generation_end    = _inline_allocation_top;
    _collection_area_start = DERIVED(OopDesc**, _compiler_area_start, 
                                                  -(int)target_size);
    _inline_allocation_top = _collection_area_start;
    set_task_allocation_start( _inline_allocation_top );
    set_inline_allocation_end( _compiler_area_start );
  }

  _young_generation_start = _collection_area_start;
  verify_layout();
  // Clear entire bitvector
  clear_bit_range(_heap_start, _old_generation_end);
#ifdef PRODUCT
  clear_bit_range(_young_generation_start, _inline_allocation_top);
#else
  // so GUARANTEES in Oop::XXX_field_put() can catch
  clear_bit_range(_young_generation_start, compiler_area_end() );
#endif
}

void
ObjectHeap::set_collection_area_boundary(size_t min_free_after_collection,
                                         bool reuse_previous_young_space) {
  if (reuse_previous_young_space) {
    set_collection_area_boundary_reuse();
  } else {
    set_collection_area_boundary_no_reuse(min_free_after_collection);
  }

  int free_young_space =
    (size_t) _inline_allocation_end -
        ((size_t) _inline_allocation_top + min_free_after_collection);
  int minimum_young_space =  min(_young_generation_target_size / 4, 5 * 1024);
  if (free_young_space < minimum_young_space) {
    force_full_collect();
  }

  if (YoungGenerationAtEndOfHeap) {
    int delta = DISTANCE(_old_generation_end, _young_generation_start);
    if (delta == 0) {
    } else if (delta == 4) {
      _old_generation_end[0] = Universe::object_class()->prototypical_near();
    } else {
      _old_generation_end[0] = Universe::byte_array_class()->prototypical_near();
      _old_generation_end[1] = (OopDesc*)(delta - Array::base_offset());
    }
  } else {
    GUARANTEE(DISTANCE(_old_generation_end, _young_generation_start) == 0,
              "Young generation not split");
  }

  NOT_PRODUCT(_heap_start_bitvector_verify = _collection_area_start;)

  verify_layout();
}

void ObjectHeap::mark_and_stack_root_and_interior_pointers(OopDesc** p) {
  // This function is used when we have a marking stack overflow.
  // We must mark both the object and all the interior pointers of the object
  OopDesc* obj = (OopDesc*) p;
  // Follow near pointer
  mark_root_and_stack(&(obj->_klass));
  // Follow interior pointers.  not oops_do_inline because this is very rare
  obj->oops_do(mark_root_and_stack);
}

#if ENABLE_ISOLATES && ENABLE_COMPILER
inline void ObjectHeap::cleanup_compiled_method_cache( void ) {
  CompiledMethodCache::cleanup_unmarked();
}
#endif

juint ObjectHeap::mark_and_stack_pointers(OopDesc** p, juint bitword) {
  GUARANTEE( bitword != 0, "Should not be called for zero bitwords" );

  OopDesc** const collection_area_start = _collection_area_start;
  OopDesc** const heap_top              = compiler_area_end();
  address   const bitvector_base        = _bitvector_base;
  OopDesc**       marking_stack_top     = _marking_stack_top;  

  juint valid = 0;
  int i = 0;
  do {
    if ((bitword & 0xFFFF) == 0) { bitword >>= 16; i += 16; }
    if ((bitword &   0xFF) == 0) { bitword >>=  8; i +=  8; }
    if ((bitword &    0xF) == 0) { bitword >>=  4; i +=  4; }
    if ((bitword &    0x3) == 0) { bitword >>=  2; i +=  2; }
    if ((bitword &    0x1) == 0) { bitword >>=  1; i +=  1; }

    if( TraceGC ) {
      TTY_TRACE_CR(("TraceGC: 0x%x write barrier entry", p+i));
    }
    OopDesc** const obj = (OopDesc**) p[i];
    if( collection_area_start <= obj && obj < heap_top ) {
      // Is object already marked?
      if( !test_and_set_bit_for(obj, bitvector_base) ) {
#if ENABLE_REMOTE_TRACER
        if (RemoteTracePort > 0) {
          RemoteTracer::update_stats((OopDesc*)obj);
        }
#endif
        // No marking stack overflow is possible here
        *marking_stack_top++ = (OopDesc*)obj;
        if (TraceGC) {
          TTY_TRACE_CR(("TraceGC: 0x%x marked", obj));
        }
      }
      valid |= 1 << i;
    }
    i++;
  } while( (bitword >>= 1) != 0 );

  _marking_stack_top = marking_stack_top;

  if( valid ) {
    continue_marking();
  }
  return valid;
}

inline void ObjectHeap::mark_remembered_set(void) {
  OopDesc** p = align_down( _heap_start );
  juint* bitp = get_bitvectorword_for_aligned(p);
  OopDesc** const end = _old_generation_end - BitsPerWord;
  for( ; p <= end; bitp++, p += BitsPerWord ) {
    const juint bitword = *bitp;
    if( bitword ) {
      *bitp = mark_and_stack_pointers(p, bitword);
    }
  }
  const juint mask = right_n_bits( end - p + BitsPerWord );
  juint bitword = *bitp & mask;
  if( bitword ) {
    bitword = mark_and_stack_pointers(p, bitword);
    // The order is important here:
    // *bitp can contain object marks in the upper bits
    // and pointer marks in the lower bits.
    // mark_and_stack_pointers can update object marks
    *bitp = (*bitp & ~mask) | bitword;
    if( bitword ) {
      continue_marking();
    }
  }
}

void ObjectHeap::roots_do_to( void do_oop(OopDesc**), const bool young_only,
                                                      const int upb ) {
#if ENABLE_PROFILER
  Profiler::oops_do( do_oop );
#endif
  Universe::oops_do( do_oop );
#if ENABLE_COMPILER
  Compiler::oops_do( do_oop );
#endif
#if ENABLE_ROM_GENERATOR
  ROMWriter::oops_do( do_oop );
#endif
#if ENABLE_METHOD_TRAPS
  MethodTrap::oops_do(do_oop);
#endif
#if ENABLE_INTERPRETATION_LOG
  {
    ForInterpretationLog( p ) do_oop( p );
  }
#endif
  Scheduler::oops_do( do_oop );
#if ENABLE_COMPILER
  if( GenerateROMImage && USE_AOT_COMPILATION ) {
    // We ignore the only_young flag here -- all pointers from
    // the compiled methods are scanned. This is safe, just not the fastest.
    CompiledMethodDesc* cm  = (CompiledMethodDesc*)_compiler_area_start;
    CompiledMethodDesc* end = (CompiledMethodDesc*)_compiler_area_top;
    {
      const CodeGenerator* gen = _compiler_code_generator;
      if( gen ) {
        end = (CompiledMethodDesc*)gen->compiled_method()->obj();
      }
    }
    while (cm < end) {
      const FarClassDesc* far_class = decode_far_class(cm);
      const size_t size = cm->object_size_for(far_class);
      cm->oops_do_for(far_class, do_oop);
      cm = DERIVED(CompiledMethodDesc*, cm, size);
    }
  } else {
#if ENABLE_TTY_TRACE
    if (TraceGC) {
      int start = young_only ? CompiledMethodCache::last_old : -1;
      TTY_TRACE_CR(("TraceGC: root_do_to: young_only %d, start %d, upb %d",
                    young_only, start, upb));
    }
#endif
    for (int i= young_only ? CompiledMethodCache::last_old : -1; ++i <= upb;) {
      CompiledMethodCache::Map[i]->oops_do( do_oop );
    }
  }
  {
    const CodeGenerator* gen = CodeGenerator::current();
    if( gen ) {
      gen->compiled_method()->obj()->oops_do( do_oop );
    }
  }
#endif
}

inline void ObjectHeap::mark_objects( const bool is_full_collect ) {
#if ENABLE_COMPILER
  int upb = CompiledMethodCache::upb;
#else
  int upb = -1;
#endif

#if ENABLE_ISOLATES
#if USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA
  OopDesc* binary_images;
#endif

  if( is_full_collect && _some_tasks_terminated ) {
#if ENABLE_COMPILER
    upb = -1;
#endif
#if USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA
    binary_images = Universe::global_binary_images()->obj(); 
    Universe::global_binary_images()->set_null(); 
#endif
  }
#endif

  // Mark roots
  roots_do_to( mark_root_and_stack, !is_full_collect, upb );

  // Mark pointers from data segment into heap
  ROM::oops_do(mark_root_and_stack, is_full_collect, false);

  if( !is_full_collect ) {
    // Mark "young generation" objects referred from "old generation"
    mark_remembered_set();
  }
  // Mark global references
  global_refs_do(mark_root_and_stack, STRONG);

  // All non-finalizer-reachable roots are marked, handle potential marking
  // stack overflow
  check_marking_stack_overflow();

  // Find finalizer reachable objects
  discover_finalizer_reachable_objects();

  // Keep cons cells for reachable referents alive,
  // keep any new pending referents, their cons cells
  // and everything reachable from them alive
  mark_finalizers( _finalizer_reachable );
  mark_finalizers( _finalizer_pending   );

  // All roots are marked, handle potential marking stack overflow again
  check_marking_stack_overflow();

#if ENABLE_JAVA_DEBUGGER
  // at this point all live objects should be marked.  Now we traverse the
  // debugger objectID hash maps and if an object in the map is unmarked we
  // delete it from the table.
  //  mark_root_and_stack((OopDesc **)Universe::objects_by_ref_map());
  //  mark_root_and_stack((OopDesc **)Universe::objects_by_id_map());
  JavaDebugger::flush_refnodes();
#endif

  // Temporarily unmark the pending finalizers (just them, nothing else)
  // so that weak references to them can be cleared in the next statement below
  unmark_pending_finalizers();

  // Clear all unmarked weak references
  global_refs_do(RefArray::clear_non_marked, WEAK);

  // Mark the pending finalizers once again,
  // so that finalization can happen with them around
  mark_finalizers( _finalizer_pending );

#if ENABLE_ISOLATES
  if( is_full_collect && _some_tasks_terminated ) {
    _some_tasks_terminated = false;
#if ENABLE_COMPILER
    cleanup_compiled_method_cache();
#endif
#if USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA
    GUARANTEE( !( _collection_area_start <= ((OopDesc**)binary_images) &&
                  ((OopDesc**)binary_images) < mark_area_end() &&
                  test_bit_for( (OopDesc**)binary_images ) ),
      "Universe::binary_images must be unreachable" );

    GUARANTEE(dead_task && !test_bit_for((OopDesc**)dead_task),
              "Dead task must be unreachable");
    Universe::global_binary_images()->set_obj(binary_images); 
#if !ENABLE_LIB_IMAGES
    Task::Raw task( dead_task );
    task().free_binary_images();
#endif

    dead_task = NULL;
#if ENABLE_LIB_IMAGES
    _dead_task_images = NULL;
#else
    dead_range_end = NULL;
#endif
    mark_root_and_stack( &binary_images );
#endif
  }
#endif
}

void ObjectHeap::check_marking_stack_overflow() {
  while (_marking_stack_overflow) {
    if (TraceGC) {
      TTY_TRACE_CR(("TraceGC: Rescan all marked objects due to overflow"));
    }
    _marking_stack_overflow = false;
    if (_heap_start == _collection_area_start) {
      write_barrier_oops_do(mark_and_stack_root_and_interior_pointers,
                            _heap_start, _old_generation_end);
    }
    write_barrier_oops_do(mark_and_stack_root_and_interior_pointers,
                          _young_generation_start, _inline_allocation_top);
  }
}

void ObjectHeap::mark_forward_pointer(OopDesc** p) {
  GUARANTEE(p >= _collection_area_start && p < _inline_allocation_top,"Sanity");
  OopDesc** obj = (OopDesc**)(*p);
  if (obj > p && obj < _inline_allocation_top) {
    set_bit_for(p);
    if (TraceGC) {
       TTY_TRACE_CR(("   0x%x => 0x%x forward marked", p, obj));
    }
  }
}

inline OopDesc** ObjectHeap::mark_forward_pointers( void ) {
  OopDesc** p = _collection_area_start;
  OopDesc** const inline_allocation_top = _inline_allocation_top;
  OopDesc** const end_scan = inline_allocation_top;
  address bitvector_base = _bitvector_base;
  while (p < end_scan && test_bit_for(p, bitvector_base)) {
    // By marking the pointers in the fixed part of young space, we can
    // just treat it as part of a slightly longer old space.  We really only
    // need to mark pointers to "moving space".  But we don't yet know where
    // that barrier is, so we mark all forward pointers.
    FarClassDesc* const blueprint = ((OopDesc*)p)->blueprint();
    jint instance_size = blueprint->instance_size_as_jint();
    if (instance_size > 0) {
      // This is a common case: (non-array) Java object instance. In-line
      // OopDesc::oops_do_for() to make it run faster.
      const jbyte* map = (jbyte*)blueprint->embedded_oop_map();      
      for( OopDesc** base = p;; ) {
        const jint entry = jint(*map++);
        if (entry > 0) {
          base += entry;
          OopDesc** obj = (OopDesc**)(*base);
          if (obj > base && obj < inline_allocation_top) {
            set_bit_for(base, bitvector_base);
            if (TraceGC) {
              TTY_TRACE_CR(("   0x%x => 0x%x forward marked", base, obj));
            }
          }
        } else if (entry == 0) {
          break;
        } else {
          GUARANTEE((entry & 0xff) == OopMapEscape, "sanity")
          base += (OopMapEscape - 1);
        }
      }
    } else {
      ((OopDesc*)p)->oops_do_for(blueprint, mark_forward_pointer);
      instance_size = ((OopDesc*)p)->object_size();

    }
    if (TraceGC) {
      TTY_TRACE_CR(("TraceGC: 0x%x - 0x%x (size %d) fixed",
                    p, DERIVED(OopDesc**, p, instance_size), instance_size));
    }
    p = DERIVED(OopDesc**, p, instance_size);
  }
  return p;
}
#if !ENABLE_HEAP_NEARS_IN_HEAP 
inline size_t ObjectHeap::rom_offset_of(OopDesc* obj) {
  size_t offset_plus_flag;

  offset_plus_flag = DISTANCE(_quick_vars.rom_text_start, obj);
  if (offset_plus_flag < _quick_vars.rom_text_size) {
  } else {
    offset_plus_flag = DISTANCE(_quick_vars.rom_data_start, obj);
    offset_plus_flag ++;
  }

  return offset_plus_flag;
}
inline OopDesc* ObjectHeap::rom_oop_from_offset(size_t offset,
                                                const QuickVars& qv) {
  if ((offset & 0x1) == 0) {
    return (OopDesc*)(int(qv.rom_text_start) + offset);
  } else {
    offset --;
    return (OopDesc*)(int(qv.rom_data_start) + offset);
  }
}

inline OopDesc* ObjectHeap::rom_oop_from_offset(size_t offset) {
  if ((offset & 0x1) == 0) {
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
    return (OopDesc*)(int(ROM::min_text_seg_addr()) + offset);
#else
    return (OopDesc*)(int(_rom_text_block) + offset);
#endif
  } else {
    offset --;
    return (OopDesc*)(int(_rom_data_block) + offset);
  }
}
#endif // !ENABLE_HEAP_NEARS_IN_HEAP 
inline void ObjectHeap::compute_new_object_locations() {
  OopDesc** this_slice = NULL;
  OopDesc** this_slice_destination = NULL;
  OopDesc** next_slice = _heap_start;
  OopDesc** last_dead = NULL;
  OopDesc** prev_last_dead = NULL;
  OopDesc** first_dead = NULL;

  // Cache some unchanging global variables into locals
  OopDesc**  heap_start             = _heap_start;
  OopDesc**  inline_allocation_top  = _inline_allocation_top;
  OopDesc**  old_generation_end     = _old_generation_end;
  OopDesc**  young_generation_start = _young_generation_start;
  OopDesc*** slices_start           = _slices_start;
  const int  slice_size             = _slice_size;
  const int  slice_shift            = _slice_shift;
  address bitvector_base            = _bitvector_base;

  const juint* const last_bitvector_word_ptr =
    get_bitvectorword_for_unaligned(inline_allocation_top);

  bool split_space = (old_generation_end != young_generation_start);
  bool compaction_started = false;
  OopDesc** compaction_top;

  // Iterate over live objects, installing forwarding pointers in high
  // bits of near pointer if object moves.
  // Insert live range information in first word(s) of dead ranges, but
  // don't scan dead objects, rather, scan bitmap for next live object.
  OopDesc** p;

  if (_collection_area_start == _heap_start || !split_space) {
    p = mark_forward_pointers();
    _end_fixed_objects = p;
    compaction_top = p;
  } else {
    p = young_generation_start;
    _end_fixed_objects = old_generation_end;
    compaction_top = old_generation_end;
  }

  while (p < inline_allocation_top) {
    // Check if we passed a slice boundary
    while (p >= next_slice) {
      this_slice = next_slice;
      next_slice = this_slice + slice_size;
      this_slice_destination = compaction_top;
      *slices_start++ = this_slice_destination;
      GUARANTEE(slices_start - _slices_start <= (int)_nof_slices, "sanity");
    }
    if (test_bit_for(p, bitvector_base)) {
      // Current object is live
      OopDesc* obj = (OopDesc*) p;
      size_t size = obj->object_size_for(decode_far_class_with_real_near(obj));
      // Did we start compacting? If so this object is moving.
      GUARANTEE(p != compaction_top, "p must be moving");
      if (!compaction_started) {
        compaction_started = true;
        _compaction_start = p;  // This is the first object moving
        CACHE_QUICK_VAR(compaction_start);
      }
      // encode the near pointer and the future location of this object
      OopDesc* obj_near = obj->klass();
      size_t slice_offset = compaction_top - this_slice_destination;
      GUARANTEE(slice_offset < _slice_size, "sanity check");

      size_t near_offset;
#if ENABLE_HEAP_NEARS_IN_HEAP 
      GUARANTEE(contains(obj_near), "check");
      // Compute near pointer relative to heap start
      near_offset = ((OopDesc**)(obj_near) - heap_start);
#else
      if (contains(obj_near)) {
        // Compute near pointer relative to heap start
        near_offset = ((OopDesc**)(obj_near) - heap_start);
      } else {
        GUARANTEE(ROM::system_contains(obj_near), "must be valid ROM near");
        size_t offset = rom_offset_of(obj_near);
        GUARANTEE((offset & ~(_near_mask)) == 0, "offset too large");
        // Add flag to distinguish from the case above
        near_offset = (offset | 0x80000000);
      }    
#endif  
      obj->_klass = (OopDesc*) ((slice_offset << slice_shift) | near_offset);

      if (last_dead != NULL) {
        // Object is the first live object in a live range, set live range info
        LiveRange lr(last_dead);
        lr.set_next_live(p);
        prev_last_dead = last_dead;
        last_dead = NULL;
      }
#if ENABLE_TTY_TRACE
      if (TraceGC) {
        if (split_space && _collection_area_start != _heap_start) {
          int alt_delta = DISTANCE(old_generation_end, young_generation_start);
          OopDesc** alt_destination =
              DERIVED(OopDesc**, compaction_top, alt_delta);
          TTY_TRACE_CR(("TraceGC: 0x%x - 0x%x (size %d) => 0x%x [alt 0x%x]",
                        p, DERIVED(OopDesc**, p, size), size,
                        compaction_top,  alt_destination));
        } else {
          TTY_TRACE_CR(("TraceGC: 0x%x - 0x%x (size %d) => 0x%x",
                        p, DERIVED(OopDesc**, p, size), size,
                        compaction_top));
        }
      }
#endif
      // size is in bytes rather than words
      compaction_top = DERIVED(OopDesc**, compaction_top, size);
      p              = DERIVED(OopDesc**, p, size);
    } else {
      // Current object is first dead object in dead range
      GUARANTEE(last_dead == NULL, "sanity check");
      // Update live range info
      if (prev_last_dead != NULL) {
        LiveRange lr(prev_last_dead);
        lr.set_next_dead(p);
      } else {
        first_dead = p;
      }
      last_dead = p;

      // Find next live object by scanning bitmap
      // Compute value of next_bitvector_word_ptr, next_p and bits
      juint* bitvector_word_ptr = get_bitvectorword_for_unaligned(p);
      const int trash_bits =
          p - ObjectHeap::get_aligned_for_bitvectorword(bitvector_word_ptr);
      GUARANTEE(0 <= trash_bits && trash_bits <= 31, "Sanity");

      juint bitword = *bitvector_word_ptr & ~((1 << (trash_bits)) - 1);

      // Find non-zero bitvector word (if any)
      while (bitvector_word_ptr <= last_bitvector_word_ptr && bitword == 0) {
        bitword = *++bitvector_word_ptr;
      }
      p = ObjectHeap::get_aligned_for_bitvectorword(bitvector_word_ptr);
      // Find first bit set in word (if any)
      if ((bitword & 0xFFFF) == 0) { bitword >>= 16; p += 16; }
      if ((bitword &   0xFF) == 0) { bitword >>=  8; p +=  8; }
      if ((bitword &    0xF) == 0) { bitword >>=  4; p +=  4; }
      if ((bitword &    0x3) == 0) { bitword >>=  2; p +=  2; }
      if ((bitword &    0x1) == 0) { bitword >>=  1; p +=  1; }
      GUARANTEE(p >= inline_allocation_top || test_bit_for(p),
                      "Sanity of optimization");
      if (TraceGC) {
       TTY_TRACE_CR(("TraceGC: 0x%x - 0x%x (size %d) dead",
                     last_dead, p, DISTANCE(last_dead, p)));
      }
    }
  }
  // Update last live range in heap
  if (last_dead != NULL) {
    // We exited the loop while scanning dead objects.  Indicate that this
    // is the end
    LiveRange lr(last_dead);
    lr.set_next_live(inline_allocation_top);
    lr.set_next_dead(inline_allocation_top);
  } else if (prev_last_dead != NULL) {
    // We exited the loop while scanning live objects, and there was
    // at least one dead object.
    LiveRange lr(prev_last_dead);
    lr.set_next_dead(inline_allocation_top); // next_live already set
  }

  if (split_space && (_collection_area_start != _heap_start)){
    // We have to add the entire range from _old_generation_end to
    // _young_generation_start as a dead object
    LiveRange lr(old_generation_end);
    if (_compaction_start == young_generation_start) {
      // The first object in young space is alive.  Just create a new
      // dead range and put it at the beginning
      GUARANTEE(test_bit_for(young_generation_start), "Must be alive");
      lr.set_next_live(young_generation_start);
      lr.set_next_dead(first_dead == NULL ? inline_allocation_top : first_dead);
    } else {
      // The first object in young space is dead.
      GUARANTEE(!test_bit_for(young_generation_start), "Must be dead");
      OopDesc **next_live, **next_dead;
      LiveRange lrx(young_generation_start);
      lrx.get_range(next_live, next_dead);
      lr.set_next_live(next_live);
      lr.set_next_dead(next_dead);
    }
  }

  _compaction_top = compaction_top;
}

void ObjectHeap::update_interior_pointer(OopDesc** p) {
  OopDesc* obj = *p;
  const QuickVars& qv = _quick_vars;

  // Is object pointed to moving? This does null check as well.
  if ((OopDesc**)obj >= qv.compaction_start &&
      (OopDesc**)obj <  qv.collection_area_end) {
    // Decode destination encoded in object's near pointer
    *p = decode_destination(obj, qv);
    GUARANTEE(contains(*p), "sanity");
    if (TraceGC) {
      TTY_TRACE_CR(("  interior: 0x%x -> 0x%x now 0x%x", p, obj, *p));
    }
  }
}

void ObjectHeap::write_barrier_oops_update_interior_pointers(OopDesc** start,
                                                             OopDesc** end) {
  OopDesc* lower_limit = (OopDesc*)_compaction_start;
  OopDesc* upper_limit = (OopDesc*)_collection_area_end;
  const QuickVars& qv = _quick_vars;

  WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p);
  {
    OopDesc* obj = *p;
    if (lower_limit <= obj && obj < upper_limit) {
      *p = decode_destination(obj, qv);
      GUARANTEE(contains(*p), "sanity");
      if (TraceGC) {
        TTY_TRACE_CR(("  interior: 0x%x -> 0x%x now 0x%x", p, obj, *p));
      }
    }
  }
  WRITE_BARRIER_OOPS_LOOP_END;
}

#define TRACE_OTHER_UPDATE_INTERIOR(x) \
  AZZERT_ONLY(if (TraceGC) tty->print_cr("TraceGC: Update interior (%s)", x));

// Update interior pointers that live outside of the heap
inline void
ObjectHeap::update_other_interior_pointers( const bool is_full_collect ) {
  TRACE_OTHER_UPDATE_INTERIOR("global_refs_array");
  global_refs_do(update_interior_pointer, STRONG|WEAK);

  TRACE_OTHER_UPDATE_INTERIOR("Roots");
  roots_do( update_interior_pointer, !is_full_collect );

  TRACE_OTHER_UPDATE_INTERIOR("ROM");
  ROM::oops_do(update_interior_pointer, false, false);

  TRACE_OTHER_UPDATE_INTERIOR("_finalizer_reachable");
  update_interior_pointers( _finalizer_reachable );

  TRACE_OTHER_UPDATE_INTERIOR("_finalizer_pending"  );
  update_interior_pointers( _finalizer_pending   );
}

// Update interior pointers in execution stacks
inline void ObjectHeap::update_execution_stack_interior_pointers() {
  ExecutionStackDesc **previous_stack_addr = &ExecutionStackDesc::_stack_list;
  ExecutionStackDesc *this_stack = ExecutionStackDesc::_stack_list;

  while (this_stack != NULL) {
    // store next stack pointer
    ExecutionStackDesc* next_stack = this_stack->_next_stack;

    // Grab the next stack field
    GUARANTEE(contains(this_stack), "All execution stacks are in heap");

    if ((OopDesc**)this_stack < _end_fixed_objects) {
      // We are not subject to GC.  Our pointers have mark bits.
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: Stack 0x%x not being collected", this_stack));
      }
      GUARANTEE(*previous_stack_addr == this_stack, "No update needed");

      previous_stack_addr = &this_stack->_next_stack;
    } else if (!test_bit_for((OopDesc**)this_stack)) {
      // This stack is being deleted.  Remove it from the linked list
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: Stack 0x%x deleted", this_stack));
      }
      *previous_stack_addr = next_stack;
    } else {
      GUARANTEE((OopDesc**)this_stack >= _compaction_start, "Must be moving");
      // This execution stack is being relocated
#ifdef AZZERT
      FarClassDesc* far_class =
          decode_far_class_with_encoded_near((OopDesc*) this_stack);
      GUARANTEE(far_class->instance_is_execution_stack(), "Sanity");
      GUARANTEE(far_class->extern_oop_map()[0] == OopMapSentinel, "sanity");
#endif
      // This is a stack that is about to be relocated;
      ExecutionStackDesc* destination =
        (ExecutionStackDesc*)decode_destination((OopDesc*)this_stack);
      int delta = (int)destination - (int)this_stack;

      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: Stack 0x%x relocated => 0x%x",
                       this_stack, destination));
      }
      this_stack->relocate_all_pointers(delta, update_interior_pointer);
      // modify previous_stack to point to the new location
      *previous_stack_addr = destination;
      previous_stack_addr = &this_stack->_next_stack;
    }
    this_stack = next_stack;
  }
  *previous_stack_addr = NULL;
}

// This is called before compaction: update all internal pointers of
// live objects to point to the new locations of their destinations.

inline void ObjectHeap::update_object_pointers() {
  // (1) Update execution stacks, before anything else moves
  update_execution_stack_interior_pointers();

  OopDesc** const compaction_start      = _compaction_start;
  OopDesc** const old_generation_end    = _old_generation_end;
  OopDesc** const end_fixed_objects     = _end_fixed_objects;

  const bool is_full_collect = compaction_start <= old_generation_end;

  // (2) Update random pointers;
  update_other_interior_pointers( is_full_collect );

  OopDesc** const young_generation_start= _young_generation_start;
  OopDesc** const inline_allocation_top = _inline_allocation_top;
  
  // (3) Update interior object pointers (near pointers unchanged yet)
  // Execution stacks in compaction space have already been handled.
  if( is_full_collect ) {
    write_barrier_oops_update_moving_object_interior_pointers(
                          compaction_start, old_generation_end);
    OopDesc** start = young_generation_start;
    if (start < end_fixed_objects) {
      start = end_fixed_objects;
    }
    write_barrier_oops_update_moving_object_interior_pointers(
                          start, inline_allocation_top);
  } else {
    write_barrier_oops_update_moving_object_interior_pointers(
                          compaction_start,
                          inline_allocation_top);
  }

  // _end_fixed_objects == compaction_start always,
  // see compute_new_object_locations

  TRACE_OTHER_UPDATE_INTERIOR("write_barrier");

  if (end_fixed_objects <= old_generation_end) {
    write_barrier_oops_update_interior_pointers(_heap_start, 
                                                end_fixed_objects);
  } else {
    write_barrier_oops_update_interior_pointers(_heap_start,
                                                old_generation_end);
    write_barrier_oops_update_interior_pointers(young_generation_start,
                                                end_fixed_objects);
  }

  // (4) Update near object pointers -- near pointers can be updated
  // only at this point.
  if( is_full_collect ) {
    write_barrier_oops_update_moving_object_near_pointer(
                          compaction_start, old_generation_end);
    write_barrier_oops_update_moving_object_near_pointer(
                          young_generation_start, inline_allocation_top);

    write_barrier_oops_unencode_moving_object_near_pointer(
                          compaction_start, old_generation_end);
    write_barrier_oops_unencode_moving_object_near_pointer(
                          young_generation_start, inline_allocation_top);
  } else {
    write_barrier_oops_update_moving_object_near_pointer(
                          compaction_start, inline_allocation_top);
    write_barrier_oops_unencode_moving_object_near_pointer(
                          compaction_start, inline_allocation_top);
  }
}

void ObjectHeap::write_barrier_oops_update_moving_object_interior_pointers(
                               OopDesc** start, OopDesc** end) {
  const QuickVars& qv = _quick_vars;
  WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p);
  {
    FarClassDesc* far_class = decode_far_class_with_encoded_near((OopDesc*)p,
                                                                 qv);
    int instance_size = far_class->instance_size_as_jint();
    OopDesc* destination = decode_destination((OopDesc*)p, qv);
    if (instance_size > 0) {
      // This is a common case: (non-array) Java object instance. In-line
      // OopDesc::oops_do_for() to make it run faster.
#if ENABLE_TTY_TRACE
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: 0x%x => 0x%x updating interior",
                      p, destination));
      }
#endif
      jbyte* map = (jbyte*)far_class->embedded_oop_map();
      OopDesc** interiorp = p;
      OopDesc** lower_limit = qv.compaction_start;
      OopDesc** upper_limit = qv.collection_area_end;
      while (true) {
        jint entry = (jint)(*map++);
        if (entry > 0) {
          interiorp += entry;
          OopDesc* obj = *interiorp;
          // Here's a hand-lined version of update_interior_pointer().
          if ((OopDesc**)obj >= lower_limit && (OopDesc**)obj < upper_limit) {
            *interiorp = decode_destination(obj, qv);
            GUARANTEE(contains(*interiorp), "sanity");
            if (TraceGC) {
              TTY_TRACE_CR(("  interior: 0x%x -> 0x%x now 0x%x", interiorp, 
                            obj, *interiorp));
            }
          }
        } else if (entry == 0) {
          break;
        } else {
          GUARANTEE((entry & 0xff) == OopMapEscape, "sanity")
          interiorp += (OopMapEscape - 1);
        }
      }
    } else if (instance_size != InstanceSize::size_execution_stack) {
      // Handle relative offsets in compiled method separately. Must
      // be done before we update the interior pointers.
#if ENABLE_TTY_TRACE
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: 0x%x => 0x%x updating interior",
                      p, destination));
      }
#endif

#if ENABLE_COMPILER
      if (instance_size == InstanceSize::size_compiled_method) {
        int delta = (int)destination - (int)p;
        ((CompiledMethodDesc*) p)->update_relative_offsets(delta);
      }
#endif

      ((OopDesc*) p)->oops_do_for(far_class, update_interior_pointer);

      if (instance_size == InstanceSize::size_method) {
        int delta = (int)destination - (int)p;
        ((MethodDesc*) p)->relocate_variable_part(delta);
      }
    }
  }
  WRITE_BARRIER_OOPS_LOOP_END;
}

void ObjectHeap::write_barrier_oops_update_moving_object_near_pointer(
                               OopDesc** start, OopDesc** end) {
  const QuickVars& qv = _quick_vars;
  OopDesc ** compaction_start    = _compaction_start;
  OopDesc ** collection_area_end = _collection_area_end;
  OopDesc **heap_start           = _heap_start;
  size_t    near_mask            = _near_mask;
  size_t    slice_offset_mask    = _slice_offset_mask;

  WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p);
  {
    GUARANTEE(p >= _compaction_start && p < _collection_area_end, "Sanity");
    OopDesc *obj = (OopDesc *) p;
    // this near pointer is encoded
    OopDesc* n = decode_near(obj, heap_start, near_mask);
    // Is near object moving?
    if ((OopDesc**)n >= compaction_start &&
        (OopDesc**)n <  collection_area_end) {
      // Decode destination encoded in near's near pointer
      OopDesc* new_near = decode_destination(n, qv);
      // Set new destination in lower bits, but leave the destination for this
      // object (encoded in the upper bits) in place, since it might be
      // needed later.
      const size_t new_near_offset = (OopDesc**)new_near - heap_start;
      const size_t encoded_dest = ((size_t)*p) & slice_offset_mask;
      if (TraceGC) {
        TTY_TRACE_CR(("  near: 0x%x -> 0x%x now 0x%x (encoded 0x%x => 0x%x)",
                       p, n, new_near, *p, (encoded_dest | new_near_offset)));
      }
      *p = (OopDesc*)(encoded_dest | new_near_offset);
    }
  }
  WRITE_BARRIER_OOPS_LOOP_END;
}

void ObjectHeap::write_barrier_oops_unencode_moving_object_near_pointer(
                               OopDesc** start, OopDesc** end) {
  OopDesc **heap_start = _heap_start;
  size_t near_mask = _near_mask;
  WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p);
  {
    OopDesc* new_near = decode_near((OopDesc*)p, heap_start, near_mask);
    if (TraceGC) {
      TTY_TRACE_CR(("  0x%x: unencode near 0x%x => 0x%x", p, *p, new_near));
    }
    *p = new_near;
  }
  WRITE_BARRIER_OOPS_LOOP_END;
}

inline void ObjectHeap::compact_objects(bool reuse_young_generation) {
  bool split = _young_generation_start != _old_generation_end;
  if (_compaction_start != _collection_area_end) {
    GUARANTEE(_end_fixed_objects < (OopDesc**) _compaction_start,
              "sanity check");
    OopDesc** next_live = NULL;
    OopDesc** current_dead = _end_fixed_objects;
    OopDesc** next_dead = NULL;
    OopDesc** destination = (split && reuse_young_generation)
                                ? _young_generation_start : _end_fixed_objects;
    OopDesc** first_destination = destination;
    // Iterate over all live ranges
    while (true) {
      LiveRange lr(current_dead);
      lr.get_range(next_live, next_dead);
      if (next_live == _inline_allocation_top) {
        break;
      }
      GUARANTEE(next_live >= (OopDesc**) _compaction_start
                   && (OopDesc**)next_live < _collection_area_end,
                "range should be moving");
      GUARANTEE(current_dead < next_live && next_live < next_dead,
                "sanity check");
      // Copy live range
      size_t live_size = DISTANCE(next_live, next_dead);
      GUARANTEE(contains(destination), "sanity");
      GUARANTEE(contains(next_live),   "sanity");
      // possibly overlapping regions
      if (destination != next_live) {
        jvm_memmove(destination, next_live, live_size);
      }
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: move live range 0x%x->0x%x %u bytes",
                      next_live, destination, live_size));
      }
      current_dead = next_dead;
      destination = DERIVED(OopDesc**, destination, live_size);
      if (current_dead == _inline_allocation_top) {
        break;
      }
      GUARANTEE(contains(destination), "sanity");
    }
    GUARANTEE(destination == _compaction_top, "Sanity");
    _inline_allocation_top = destination;

    if (ENABLE_CPU_VARIANT) {
      // If we're using compiler area, and Jazelle is not enabled,
      // there's no executable code inside the regular area of the heap.
      OsMisc_flush_icache((address)first_destination,
                          DISTANCE(first_destination, destination));
    }
  } else {
    // nothing has moved.  Lots of live stuff followed by lots of dead stuff
    if (reuse_young_generation &&
             (_young_generation_start != _old_generation_end)) {
      // The entire young generation must be dead
      _inline_allocation_top = _young_generation_start;
    } else {
      _inline_allocation_top = _end_fixed_objects;
    }
  }
  if (split && reuse_young_generation) {
    DIRTY_HEAP(_old_generation_end, 
               DISTANCE(_old_generation_end, _young_generation_start));
    DIRTY_HEAP(_inline_allocation_top,
               DISTANCE(_inline_allocation_top, _collection_area_end));
  } else {
    DIRTY_HEAP(_inline_allocation_top,
               DISTANCE(_inline_allocation_top, _collection_area_end));
  }
}

void ObjectHeap::full_collect(JVM_SINGLE_ARG_TRAPS) {
  force_full_collect();
  // IMPL_NOTE: this place is questionable, and probably needs fixing
  // full_collect() is being invoked by System.gc() and application developer
  // could expect it to really free up some memory, and probably call finalizers
  // of dead objects.
  // For this behavior we should call collect() with bigger argument,
  // like heap size or something similar. But doing this will somewhat slowdown
  // application doing System.gc() without understanding what they doing,
  // just in case. So here I'm using compromise which frees up some memory, but
  // does almost nothing in most cases.
  safe_collect(1 JVM_NO_CHECK);
  Thread::clear_current_pending_exception();
  GUARANTEE(_interned_string_near_addr == 
            Universe::interned_string_near()->obj(), "must be the same");
}

#if ENABLE_TTY_TRACE
void print_size(Stream* st, size_t size) {
  st->print("%dK", size/1024);
}
#endif

inline void ObjectHeap::try_to_shrink(int min_free_after_collection) {
  allocation_trap_must_be_disabled();

  // This function is called only after a full GC, which is a good time to
  // shrink the heap size if we have too much free space. The exact policy 
  // is decided in OsMemory_heap_reduction_target().
  size_t used_size = used_memory() + min_free_after_collection;

#if ENABLE_ISOLATES
  used_size += _current_deficit;
#endif

  if (used_size < _heap_size) {
    const size_t new_size = align_up(
      OsMemory_heap_reduction_target(_heap_size, used_size, _heap_min) );

    GUARANTEE(new_size <= _heap_size, "sanity");
    GUARANTEE(new_size >= used_size, "sanity");
    if (new_size < _heap_size) {
      if (VerboseGC || TraceHeapSize) {
        TTY_TRACE(("shrinking heap from %d KB to %d KB\n",
                   _heap_size / 1024, new_size / 1024));
      }

      // Shift the whole compiler_area downwards.
      const int delta = new_size - _heap_size;
      compact_and_move_compiler_area( delta );
      LargeObject::move( delta, _heap_top );
      
      const bool t = adjust_heap_size(new_size);
      GUARANTEE(t, "heap shrinking should never fail");
      (void)t;
    }

    GUARANTEE(_inline_allocation_end <= _compiler_area_start, "Sanity");
  }
  verify_layout();
}

void ObjectHeap::try_to_grow(int requested_free_memory, 
                             const bool is_full_collect)
{
  allocation_trap_must_be_disabled();
  if( compiler_area_in_use() ) {
    // Don't grow the heap during active compilation.
    // IMPL_NOTE: We should have grown the heap already when we started
    // the compilation.  At this point we would have temp compiler
    // data structures in compiler_area, and we don't know how to
    // mark/compact the temp stuff.
    // Don't worry. The worst thing that would happen here is the
    // current compilation would fail, but it will not causes an
    // unwanted OutOfMemoryError in Java execution.
    return;
  }
  {
    // This check is useful for certain programs that try to make an
    // outrageously large allocation in order to force a full GC. In this
    // case we should avoid expanding the heap to conserve system memory.
    // We don't now for much the compiler area can shrink. To make a
    // conservative LOWER bound estimate we suppose that all compiled code
    // can be evicted.
    const int min_heap_size_needed = DISTANCE(_heap_start, _old_generation_end)
      + DISTANCE(_young_generation_start, _inline_allocation_top)
      + DISTANCE(_large_object_area_bottom, _heap_top)
      + requested_free_memory;
    if (min_heap_size_needed > _heap_capacity) {
      // No point to grow, we can never satisfy the request.
      return;
    }
  }

  // We grow at least one young gen size, to avoid excessive number of
  // growths.
  if( requested_free_memory < int(_young_generation_target_size) ) {
    requested_free_memory = _young_generation_target_size;
  }

  int required_free_memory = requested_free_memory;
#if ENABLE_ISOLATES
  required_free_memory += _current_deficit;
#endif

  if( (required_free_memory -= free_memory()) <= 0 ) {    
    return; // no need to grow
  }

  verify_layout();

  const size_t required_heap_size = _heap_size + required_free_memory;
  const size_t new_heap_size = 
      OsMemory_heap_expansion_target(_heap_size, required_heap_size,
                                     (size_t)_heap_capacity, is_full_collect);
  const int heap_size_deficit = int(required_heap_size - new_heap_size);
  if( heap_size_deficit > 0 ) {    
    if( !is_full_collect ) {
      // The Os doesn't want to grow
      return; 
    }
#if ENABLE_COMPILER
    if( heap_size_deficit > compiler_area_size() ) {
      // Shrinking the compiler area is not enough memory
      return; 
    }

    // IMPL_NOTE: We should enter this block even if we're not doing a full 
    // collect. Currently, we might have excessive YG collection while
    // the compiler_area is relatvely empty.
    const int compiler_area_reduction = ObjectHeap::compiler_area_size() >> 1;
    if( required_free_memory > compiler_area_reduction ) {
      required_free_memory = compiler_area_reduction;
    }
    if( required_free_memory > 0 ) {
      reduce_compiler_usage(align_allocation_size(required_free_memory));
    }
#endif
  }

  GUARANTEE(new_heap_size <= (size_t)_heap_capacity, "sanity");

  if (VerboseGC || TraceHeapSize) {
    TTY_TRACE(("growing heap from %d KB to %d KB\n",
               _heap_size / 1024, new_heap_size / 1024));
  }
  OopDesc** const old_heap_top = _heap_top;
  if (adjust_heap_size(new_heap_size)) {
    const int delta = DISTANCE(old_heap_top, _heap_top);
    LargeObject::move( delta, old_heap_top );
    compact_and_move_compiler_area( delta );

    // Since there is extra space available now,
    // we can make sure here that the next GC is not a full one:
    OopDesc** const allocation_top = _inline_allocation_top;
    _collection_area_start  = allocation_top;
    _old_generation_end     = allocation_top;
    _young_generation_start = allocation_top;

    // Expand the young generation to its normal size
    // (but not beyond the heap)
    OopDesc** allocation_end =
      DERIVED(OopDesc**, allocation_top, requested_free_memory);
    if( allocation_end > _compiler_area_start ) {
      allocation_end = _compiler_area_start;
    }
    set_inline_allocation_end( allocation_end );
  }
  verify_layout();
}

#if ENABLE_COMPILER
// Returns the actual number of bytes freed from compiler usage.
size_t ObjectHeap::reduce_compiler_usage(size_t requested) {
  if (GenerateROMImage && USE_AOT_COMPILATION) {
    // Never evict compiled methods when doing AOT
    return 0;
  }

  allocation_trap_must_be_disabled();
  Compiler::abort_suspended_compilation();

  if (Compiler::is_active()) {
    // We don't know how to collect temporary compiler data.
    return 0;
  }

  {
    // Add a little slack, so we don't need to come back here over
    // and over again.
    const size_t slack = align_allocation_size((_heap_size / 32 + 5 * 1024));
    requested += slack;
  }
  
  {
    size_t min_size = CompilerAreaPercentage;
    if (_heap_size >= size_t(_heap_capacity) || _last_heap_expansion_failed) {
      // If the heap is not fully expanded yet, don't shrink the compiler area
      min_size = 2 * MinimumCompilerAreaPercentage;
    }
    min_size = align_allocation_size( min_size * _heap_size / 200 );

    const size_t allowed_reduction = ObjectHeap::compiler_area_size()-min_size;
    if( int(allowed_reduction) <= 0 ) {
      return 0;
    }
    if( requested > allowed_reduction ) {
      requested = allowed_reduction;
    }
  }

  int compiler_area_shift = int(requested);
  {
    const size_t free_compiler_area = ObjectHeap::compiler_area_free();
    if( int(requested -= free_compiler_area) > 0 ) {
      const int bytes_evicted = CompiledMethodCache::evict(requested);
      compiler_area_shift = free_compiler_area + bytes_evicted;
      if (TraceCompilerGC) {
        TTY_TRACE_CR(("TraceGC: bytes_evicted = %d", bytes_evicted));
      }
    }
  }

  // Shift the whole compiler area to higher address.
  if (compiler_area_shift) {
    compact_and_move_compiler_area( compiler_area_shift );
    set_inline_allocation_end( _compiler_area_start );
  }
  return compiler_area_shift;
}
#endif

void ObjectHeap::collect(size_t min_free_after_collection JVM_TRAPS) {
  min_free_after_collection = align_allocation_size(min_free_after_collection);

  allocation_trap_must_be_disabled();
  accumulate_current_task_memory_usage();

  // We cannot collect unless the world is properly setup.
  if( Universe::before_main() ) {
#if ENABLE_COMPILER
    // Shrink and relocate empty compiler area without real GC
    OopDesc** p = _compiler_area_start;
    if( p == _compiler_area_top ) {
#if ENABLE_ISOLATES
      min_free_after_collection +=
        align_allocation_size(_reserved_memory_deficit);
#endif
      p = DERIVED( OopDesc**, p, min_free_after_collection );
      if( p < _large_object_area_bottom ) {
        _compiler_area_start = p;
        _compiler_area_top = p;
        _inline_allocation_end = p;
        return;
      }
    }
#endif
    // Print a better diagnostics for MVM here: _reserved_memory_deficit
    TTY_TRACE_CR(("Not enough memory to bootstrap VM: %d bytes", _heap_size));
    JVM_FATAL(bootstrap_heap_too_small);
    JVM::exit(1);
  }
  if (!GCDisabler::gc_okay()) {
    if (GenerateROMImage) {
      TTY_TRACE_CR(("Not enough memory to run Romizer. "
                    "Please increase heap size"));
    }
    JVM_FATAL(allocation_failed_while_GC_disabled);
  }

  GUARANTEE(!_jvm_in_quick_native_method, "cannot GC in quick native methods");
  GUARANTEE(AllocationDisabler::allocation_okay(),
            "Allocation must be enabled");

#if ENABLE_ISOLATES
  #define DETECT_QUOTA_VIOLATIONS \
    violations = detect_out_of_memory_tasks(min_free_after_collection);
  unsigned violations;
  bool is_full_collect;
  for( ;; force_full_collect() ) {
    is_full_collect = internal_collect(min_free_after_collection JVM_CHECK);
    DETECT_QUOTA_VIOLATIONS
    if( !(violations & OverLimit) ) break;
    if( is_full_collect ) {
      Throw::out_of_memory_error( JVM_SINGLE_ARG_THROW );
    }
  }
#else
  #define DETECT_QUOTA_VIOLATIONS
  bool is_full_collect = internal_collect(min_free_after_collection JVM_CHECK);
#endif
  if (!is_full_collect) {
    // In some cases we may want to grow without doing a full gc. Let the
    // platform decide.
    if( !YoungGenerationAtEndOfHeap ) {
      try_to_grow(min_free_after_collection, false);
      DETECT_QUOTA_VIOLATIONS
    }

    if(
#if ENABLE_ISOLATES
        !violations &&
#endif
        free_memory() > min_free_after_collection) {
      return;
    }

    // IMPL_NOTE: try to reduce compiler area first??

    // Insufficient space freed, so escalate to full collection:
    force_full_collect();
    is_full_collect = internal_collect(min_free_after_collection JVM_CHECK);
    DETECT_QUOTA_VIOLATIONS
  }

  // Once we arrived here, we are sure we did execute a full collection.
  // We may or may not have enough space to satisfy the current allocation
  // request. Even if there's enough space, try_to_grow() may still
  // decide to expand the heap (if the heap is "pretty full").
  try_to_grow(min_free_after_collection, true);
  DETECT_QUOTA_VIOLATIONS

  int memory_to_free = min_free_after_collection - free_memory();

#if ENABLE_ISOLATES
  if( violations ) {
    memory_to_free += _current_deficit;
  }
#endif

  if( memory_to_free <= 0 ) {
    return;
  }

#if ENABLE_COMPILER
  memory_to_free -= reduce_compiler_usage(memory_to_free);
  if (memory_to_free <= 0) {
    // We have completely satisfied the request by sliding the compiler area.
    GUARANTEE(free_memory() >= min_free_after_collection,
              "compiler area must have been compacted");
    set_collection_area_boundary(min_free_after_collection, false);
    return;
  }
#endif

  { // We still need <memory_to_free> bytes, even after code eviction.
    if (VerboseGC) {
      TTY_TRACE(("Shrinking execution stacks and flushing caches ... "));
    }
    bool more_free_space = false;
    more_free_space |= Thread::shrink_execution_stacks();
    more_free_space |= Verifier::flush_cache();

    if (!more_free_space) {
      if (VerboseGC) {
        TTY_TRACE_CR(("nope"));
      }
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
    } else {
      if (VerboseGC) {
        TTY_TRACE_CR(("got more space"));
      }
    }
  }

  force_full_collect();
  internal_collect(min_free_after_collection JVM_CHECK);
#if ENABLE_ISOLATES
  DETECT_QUOTA_VIOLATIONS
  if( violations ) {
    Throw::out_of_memory_error( JVM_SINGLE_ARG_THROW );
  }
#endif

  if( free_memory() < min_free_after_collection ) {
    set_collection_area_boundary(0, false);
    Throw::out_of_memory_error( JVM_SINGLE_ARG_THROW );
  }
#undef DETECT_QUOTA_VIOLATIONS
} 

// This function contains misc debug and tracing code that are mostly unused by
// PRODUCT mode.
inline void ObjectHeap::internal_collect_prologue(size_t min_free_after_collection) {
  EnforceRuntimeJavaStackDirection enfore_java_stack_direction;

  EventLogger::start(EventLogger::GC);

  _collection_area_end = _inline_allocation_top;
  CACHE_QUICK_VAR(heap_start);
//  CACHE_QUICK_VAR(heap_top);
  CACHE_QUICK_VAR(compaction_start);
  CACHE_QUICK_VAR(collection_area_start);
  CACHE_QUICK_VAR(collection_area_end);
  CACHE_QUICK_VAR(slices_start);
  
  CACHE_QUICK_VAR(near_mask);
  CACHE_QUICK_VAR(slice_shift);
  CACHE_QUICK_VAR(slice_offset_bits);
  CACHE_QUICK_VAR(slice_size);
  CACHE_QUICK_VAR(near_mask);
  CACHE_QUICK_VAR(slice_offset_mask);

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  _quick_vars.rom_text_start = (OopDesc**)ROM::min_text_seg_addr();
  _quick_vars.rom_text_size  = ROM::text_total_size();
#else
  _quick_vars.rom_text_start = (OopDesc**)&_rom_text_block[0];
  _quick_vars.rom_text_size  = _rom_text_block_size_fast;
#endif
  _quick_vars.rom_data_start = (OopDesc**)&_rom_data_block[0];

  if( TraceCompiledMethodCache ) {
    (void)min_free_after_collection;
    TTY_TRACE_CR(( "\n*** ObjectHeap::internal_collect( %d ) beg ***",
      min_free_after_collection ));
  }

  PERFORMANCE_COUNTER_INCREMENT(num_of_gc, 1);
  if (_collection_area_start == _heap_start) {
    PERFORMANCE_COUNTER_INCREMENT(num_of_full_gc, 1);
  }

#if ENABLE_TTY_TRACE
  if (TraceGC) {
#if ENABLE_PERFORMANCE_COUNTERS
    TTY_TRACE_CR(("TraceGC: Starting GC #%d", jvm_perf_count.num_of_gc));
#endif
    TTY_TRACE_CR(("Heap 0x%x-0x%x, 0x%x-0x%x",
                  _heap_start, _old_generation_end,
                  _young_generation_start, _inline_allocation_top));
    ObjectHeap::print_all_objects();
  }
#endif

  if (VerifyGC) {
    verify();
  }

#if ENABLE_REMOTE_TRACER
  if (RemoteTracePort > 0) {
    RemoteTracer::update_stats_from_heap(_heap_start, _collection_area_start);
  }
#endif

  save_java_stack_snapshot();
  Universe::release_gc_dummy();

#if ENABLE_PERFORMANCE_COUNTERS
  _internal_collect_start_time = Os::elapsed_counter();
#endif

#if ENABLE_PERFORMANCE_COUNTERS || USE_DEBUG_PRINTING
  _old_gen_size_before =
      DISTANCE(_heap_start, _old_generation_end);
  _young_gen_size_before =
      DISTANCE(_young_generation_start, _inline_allocation_top);
#endif
}

inline void ObjectHeap::setup_marking_stack(void) {
#if ENABLE_COMPILER || USE_LARGE_OBJECT_AREA
  OopDesc** marking_stack_start = _inline_allocation_top;
  OopDesc** marking_stack_end   = _compiler_area_start;
  const int size = DISTANCE( marking_stack_start, marking_stack_end );

  OopDesc** const heap_top      = (OopDesc**) LargeObject::end();
  OopDesc** const heap_limit    = _heap_limit;
  if( DISTANCE( heap_top, heap_limit ) > size ) {
    marking_stack_start = heap_top;
    marking_stack_end = heap_limit;
  }
#else
  OopDesc** const marking_stack_start = _inline_allocation_top;
  OopDesc** const marking_stack_end   = _heap_limit;
#endif

  _marking_stack_start = marking_stack_start;
  _marking_stack_top   = marking_stack_start;
  _marking_stack_end   = marking_stack_end;
}

inline void ObjectHeap::internal_collect_epilogue(bool is_full_collect, 
                                           bool reuse_young_generation) {
  (void)reuse_young_generation;
#if ENABLE_PERFORMANCE_COUNTERS || ENABLE_TTY_TRACE
  size_t old_gen_size_after =
      DISTANCE(_heap_start, _old_generation_end);
  size_t young_gen_size_after =
      DISTANCE(_young_generation_start, _inline_allocation_top);

  int collected = (_old_gen_size_before - old_gen_size_after) +
                  (_young_gen_size_before - young_gen_size_after);

  const jlong elapsed = Os::elapsed_counter() - _internal_collect_start_time;

  PERFORMANCE_COUNTER_INCREMENT(total_bytes_collected, collected);
  PERFORMANCE_COUNTER_INCREMENT(total_gc_hrticks, elapsed);
  PERFORMANCE_COUNTER_SET_MAX(max_gc_hrticks, elapsed);
#endif

#if ENABLE_TTY_TRACE
  if (VerboseGC || TraceGC) {
    if (is_full_collect) {
      TTY_TRACE(("[full  GC <"));
    } else {
      TTY_TRACE(("[young GC <"));
    }

    size_t regular_area_size  = DISTANCE(_heap_start, _compiler_area_start);
#if ENABLE_COMPILER
    size_t compiler_area_size = ObjectHeap::compiler_area_size();
#endif
    if (_heap_min != _heap_capacity) {
      print_size(tty, _heap_min);
      TTY_TRACE((":"));
      print_size(tty, regular_area_size);
#if ENABLE_COMPILER
      TTY_TRACE(("+"));
      print_size(tty, compiler_area_size);
#endif
      TTY_TRACE(("="));
      print_size(tty, _heap_size);
      TTY_TRACE((":"));
    } else {
      print_size(tty, regular_area_size);
#if ENABLE_COMPILER
      TTY_TRACE(("+"));
      print_size(tty, compiler_area_size);
#endif
      TTY_TRACE(("="));
    }
    print_size(tty, _heap_capacity);
    TTY_TRACE(("> "));

    print_size(tty, _old_gen_size_before);
    TTY_TRACE(("+"));
    print_size(tty, _young_gen_size_before);
    TTY_TRACE((" -> "));
    if (is_full_collect) {
      print_size(tty, old_gen_size_after);
    } else if (reuse_young_generation) {
      print_size(tty, young_gen_size_after);
      TTY_TRACE(("(reused)"));
    } else {
      print_size(tty, old_gen_size_after - _old_gen_size_before);
      TTY_TRACE(("(tenured)"));
    }

    TTY_TRACE((", %d hrticks", (int)elapsed));

    TTY_TRACE_CR(("]"));
    TTY_TRACE_CR(("Heap 0x%x-0x%x, 0x%x, 0x%x-0x%x",
                  _heap_start, _old_generation_end,
                  _young_generation_start, _inline_allocation_top,
                  _inline_allocation_end));

  }
#endif

  // IMPL_NOTE: notify OsMemory about the heap statistics

#if ENABLE_REMOTE_TRACER
  if (RemoteTracePort > 0) {
    RemoteTracer::set_heap_size(total_memory());
    RemoteTracer::set_used_memory(used_memory());
    RemoteTracer::set_free_memory(free_memory());
  }
#endif

  if (VerifyGC) {
    verify();
  }

#if ENABLE_COMPILER
  if( is_full_collect ) {
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "Full collection: free %d, cache: %d",
        free_memory(), CompiledMethodCache::size ));
    }
  }
#endif

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "\n*** ObjectHeap::internal_collect end ***" ));
  }

  EventLogger::end(EventLogger::GC);
}

bool ObjectHeap::internal_collect(size_t min_free_after_collection JVM_TRAPS) {
  LargeObject::verify();

  JVM_IGNORE_TRAPS;
  internal_collect_prologue(min_free_after_collection);
  _is_gc_active = true;

  // Evict compiled methods, etc
  const bool is_full_collect = _collection_area_start == _heap_start;

  // Make bci and pc relative.  Mark bits on stack
  Scheduler::gc_prologue(is_full_collect ? do_nothing
                                         : mark_pointer_to_young_generation);
  // Make buffer location pointers relative.
  ClassFileParser::gc_prologue();
  TypeSymbol::ParseStream::gc_prologue();

  setup_marking_stack();

  // Clear bitvector for target collection area (can contain "dirty" bits)
  clear_bit_range( is_full_collect ? _heap_start : _young_generation_start,
                   _inline_allocation_top );
  _compaction_start = _collection_area_end; // sentinel value
  CACHE_QUICK_VAR(compaction_start);

  // Phase1: Mark objects transitively from roots
  if (TraceGC) {
    TTY_TRACE_CR(("TraceGC:  *** MARKING PHASE ***"));
  }
  mark_objects( is_full_collect );

  // Phase2: Insert forward pointers in unused near object bits
  if (TraceGC) {
    TTY_TRACE_CR(("TraceGC:  *** COMPUTE NEW OBJECT LOCATIONS ***"));
  }
  compute_new_object_locations();

  OopDesc** const old_generation_end = _old_generation_end;

  bool reuse_young_generation = false;
  if (_heap_start != _collection_area_start) {
    size_t old_size = DISTANCE(_collection_area_start, _inline_allocation_end);
    size_t new_size = DISTANCE( old_generation_end,    _compaction_top);
    size_t new_free_if_reuse =
         DISTANCE(_young_generation_start, _collection_area_end) - new_size;
    if (// there is enough free space
           new_free_if_reuse >= min_free_after_collection
        && (int)new_free_if_reuse >= MinimumCompileSpace
        && (new_size * 100)
                 < old_size * YoungGenerationSurvivalTargetPercentage) {
      reuse_young_generation = true;
    }
    if ( old_generation_end != _young_generation_start) {
      if (reuse_young_generation) {
        // Change the target location of all the slices.
        OopDesc*** this_slice = _slices_start;
        OopDesc*** end_slice = _slices_start + _nof_slices;
        int delta = (DISTANCE( old_generation_end, _young_generation_start));
        for ( ; this_slice < end_slice; this_slice++) {
          *this_slice = DERIVED(OopDesc**, *this_slice, delta);
        }
        _compaction_top = DERIVED(OopDesc**, _compaction_top, delta);
      }
    }
  }

  // Phase3: Adjust interior pointers using forward pointers from phase2
  if (TraceGC) {
    TTY_TRACE_CR(("TraceGC:  *** UPDATE OBJECT POINTERS ***"));
  }
  update_object_pointers();

  // Phase4; Compact
  if (TraceGC) {
    TTY_TRACE_CR(("TraceGC:  *** COMPACT OBJECTS ***"));
  }
  compact_objects(reuse_young_generation);

  // Update _class_list_base, etc
  Universe::update_relative_pointers();

  // Restore bci, pc, and stack pointer locks in heap
  Scheduler::gc_epilogue();

  // Restore buffer relative pointers.
  ClassFileParser::gc_epilogue();
  TypeSymbol::ParseStream::gc_epilogue();

  set_collection_area_boundary(min_free_after_collection,
                               reuse_young_generation);
#if ENABLE_REMOTE_TRACER
  if (RemoteTracePort > 0) {
    RemoteTracer::freeze_stats();
  }
#endif

  _is_gc_active = false;

#if ENABLE_JAVA_DEBUGGER
  // we must now re-hash our ID maps so that the new object addresses
  // will hash to the correct bucket
  JavaDebugger::rehash();
#endif

#if ENABLE_ROM_GENERATOR
  ROMWriter::rehash();
#endif

  // Process all pending internal finalizable objects
  finalize( _finalizer_pending );

#if ENABLE_COMPILER
  {
    int compilation_abstinence_ticks = CompilationAbstinenceTicks;
    if (is_full_collect) {
      // Given that we just underwent a large pause,
      // let's prevent spending run time on compilation
      // for a relatively large number of subsequent ticks:
      compilation_abstinence_ticks *= YoungGenerationTarget;
    }
    Universe::_compilation_abstinence_ticks += compilation_abstinence_ticks;
  }
#endif
  if (is_full_collect && !compiler_area_in_use() ) {
    // Don't shrink the heap during active compilation.
    // At this point we would have temp compiler data structures 
    // in compiler_area, and we don't know how to mark/compact the temp stuff.
    try_to_shrink(min_free_after_collection);
  }
  internal_collect_epilogue(is_full_collect, reuse_young_generation);
#if ENABLE_ISOLATES
  if( !GenerateROMImage ) {
    if( is_full_collect ) {
      ForTask( task ) {
        unsigned estimate = 0;
#if USE_LARGE_OBJECT_AREA
        Task::Raw t = Task::get_task(task);
        if( t.not_null() ) {
          ObjArray::Raw binary_images = t().binary_images();
          if( binary_images.not_null() ) {
            for( int i = binary_images().length(); --i >= 0; ) {
              const ROMBundle* bun = (const ROMBundle*)binary_images().obj_at( i );
              estimate += bun->heap_used();
            }
          }
        }
#endif
        _task_info[ task ].estimate = estimate;
      }
      accumulate_memory_usage( _heap_start + 1, _inline_allocation_top );
    } else {
      // old_generation_end may be below _old_generation_end
      // if some objects got promoted to old generation

      // DO NOT TRY TO AVOID CALL TO THIS FUNCTION
      // WHEN old_generation_end == _old_generation_end:
      // in addition to memory usage accounting it performs important
      // optimizations on the boundary list
      ForTask(task) {
        _task_info[ task ].estimate = _task_info[ task ].usage;
      }
      accumulate_memory_usage( old_generation_end, _old_generation_end );
    }
    ForTask(task) {
      _task_info[ task ].usage = _task_info[ task ].estimate;
    }
    if( reuse_young_generation ) {
      accumulate_memory_usage( _young_generation_start, _inline_allocation_top );
    }
    print_task_usage(tty);
  }
#endif

#if ENABLE_COMPILER
  if (is_full_collect || !reuse_young_generation) {
    CompiledMethodCache::on_promotion();
  }
#endif

  set_task_allocation_start( _inline_allocation_top );
  verify_layout();
  return is_full_collect;
}

inline
OopDesc* ObjectHeap::decode_near(OopDesc* obj, const QuickVars& qv ) {
  GUARANTEE(contains(obj)
               && (OopDesc**)obj >= _compaction_start
               && (OopDesc**)obj <  _collection_area_end,
            "near should be encoded");
  GUARANTEE(test_bit_for((OopDesc**)obj),
            "object must be marked before encoded");
  const OopDesc* klass = obj->klass();
  const size_t near_offset = (size_t) klass & qv.near_mask;
  OopDesc* n;
#if ENABLE_HEAP_NEARS_IN_HEAP 
    GUARANTEE((int) klass >= 0, "optimization check");
    n = (OopDesc*)((OopDesc**) qv.heap_start + near_offset);
    GUARANTEE(contains(n), "must be in heap");
#else
  if (((int) klass) >= 0) {
    // bit 31 is not set -- this means we're in heap

    // Lower bits contains relative offset to heap start
    n = (OopDesc*)((OopDesc**) qv.heap_start + near_offset);
    GUARANTEE(contains(n), "must be in heap");
  } else {
    // Lower bits contains relative offset to ROM
    n = rom_oop_from_offset(near_offset, qv);
    GUARANTEE(ROM::system_contains(n), "must be valid ROM near");
  }
#endif
  return n;
}

inline OopDesc* ObjectHeap::decode_near(OopDesc* obj, OopDesc **heap_start, 
                                        size_t near_mask) {
  GUARANTEE(contains(obj)
               && (OopDesc**)obj >= _compaction_start
               && (OopDesc**)obj <  _collection_area_end,
            "near should be encoded");
  GUARANTEE(test_bit_for((OopDesc**)obj),
            "object must be marked before encoded");
  const OopDesc* klass = obj->klass();
  const size_t near_offset = (size_t) klass & near_mask;
  OopDesc* n;
#if ENABLE_HEAP_NEARS_IN_HEAP 
    GUARANTEE((int) klass >= 0, "optimization check");
    n = (OopDesc*)((OopDesc**) heap_start + near_offset);
    GUARANTEE(contains(n), "must be in heap");
#else
  if (((int) klass) >= 0) {
    // bit 31 is not set -- this means we're in heap

    // Lower bits contains relative offset to heap start
    n = (OopDesc*)((OopDesc**)heap_start + near_offset);
    GUARANTEE(contains(n), "must be in heap");
  } else {
    // Lower bits contains relative offset to ROM
    n = rom_oop_from_offset(near_offset);
    GUARANTEE(ROM::system_contains(n), "must be valid ROM near");
  }
#endif
  return n;
}

inline FarClassDesc* ObjectHeap::decode_far_class(const OopDesc* obj) {
  GUARANTEE(contains(obj), "must be in heap");
  OopDesc* n = obj->klass();
  GUARANTEE(contains(n) || ROM::system_contains(n), "must be valid near");
  OopDesc* f = n->klass();
  GUARANTEE(contains(f) || ROM::system_contains(f), "must be in valid near");
  return (FarClassDesc*) f;
}

inline FarClassDesc* ObjectHeap::decode_far_class_with_real_near(OopDesc* obj)
{
  // NOTE: don't use QuickVars, as _compaction_start is being updated
  // when this function is called.
  GUARANTEE(contains(obj), "must be in heap");
  // Near pointer is not encoded yet
  OopDesc* n = obj->klass();
  GUARANTEE(contains(n) || ROM::system_contains(n), "must be valid near");
  // Near's near pointer 'f' is encoded if near 'n' >= compaction_start
  // but < current scanning location
  OopDesc* f = ((OopDesc**)n >= _compaction_start && n < obj)
             ? decode_near(n)
             : n->klass();
  GUARANTEE(contains(f) || ROM::system_contains(f), "must be in valid near");
  return (FarClassDesc*) f;
}

inline FarClassDesc*
ObjectHeap::decode_far_class_with_encoded_near(OopDesc* obj, 
                                               const QuickVars& qv) {
  GUARANTEE(contains(obj), "must be in heap");
  // Near pointer is encoded if obj is gte compaction_start
  OopDesc* n = ((OopDesc**)obj >= qv.compaction_start &&
                (OopDesc**)obj <  qv.collection_area_end)
               ? decode_near(obj, qv) : obj->klass();
  GUARANTEE(contains(n) || ROM::system_contains(n), "must be valid near");
  // Near's near pointer is encoded if near is gte compaction_start
  n = ((OopDesc**)n >= qv.compaction_start &&
       (OopDesc**)n <  qv.collection_area_end)
    ? decode_near(n, qv) : n->klass();
  GUARANTEE(contains(n) || ROM::system_contains(n), "must be valid near");
  return (FarClassDesc*) n;
}

inline OopDesc* ObjectHeap::decode_destination(OopDesc* obj, 
                                               const QuickVars& qv) {
  GUARANTEE((OopDesc**)obj >= _compaction_start &&
            (OopDesc**)obj <  _collection_area_end,
            "obj should be moving");
  // Figure out which slice obj is in
  size_t slice_index = 
      ((OopDesc**) obj - qv.heap_start) >> qv.slice_offset_bits;
  GUARANTEE(slice_index < _nof_slices, "invalid slice index");
  // Where does the base of this slice move to?
  OopDesc** slice_start = qv.slices_start[slice_index];
  // Where does obj move relative to base? Relative offset in high bits of
  // near pointer.
  const size_t slice_offset =
      ((size_t) obj->klass() & qv.slice_offset_mask) >> qv.slice_shift;
  GUARANTEE(slice_offset < _slice_size, "invalid slice offset");
  // Destination is base + relative offset
  OopDesc* destination = (OopDesc*) (slice_start + slice_offset);
  GUARANTEE(contains(destination), "must be in heap");  
  return destination;
}

void ObjectHeap::clear_bit_range(OopDesc** start, OopDesc** end) {
  OopDesc** aligned_start = align_up( start );
  OopDesc** aligned_end   = align_down( end );
  address bitvector_base  = _bitvector_base;

  if (aligned_start >= aligned_end) {
    // This is a small bit range.  Just do it by hand
    while (start < end) {
      clear_bit_for(start++, bitvector_base);
    }
    return;
  }

  while (start < aligned_start) {
    clear_bit_for(start++, bitvector_base);
  }
  while (end > aligned_end) {
    clear_bit_for(--end, bitvector_base);
  }
  juint* s = get_bitvectorword_for_aligned(aligned_start);
  juint* e = get_bitvectorword_for_aligned(aligned_end);

  GUARANTEE(s >= (juint*)_bitvector_start, "sanity");
  GUARANTEE(e <= (juint*)_slices_start,    "sanity");

  jvm_memset(s, 0, DISTANCE(s, e));
}

void ObjectHeap::write_barrier_oops_do(void do_oop(OopDesc**),
                                       OopDesc** start, OopDesc** end) {
  WRITE_BARRIER_OOPS_LOOP_BEGIN(start, end, p);
  {
    do_oop(p);
  }
  WRITE_BARRIER_OOPS_LOOP_END;
}

void ObjectHeap::expand_young_generation( void ) {
  set_inline_allocation_end( _compiler_area_start );
  verify_layout();
}

void ObjectHeap::expand_young_generation ( const int size ) {
  GUARANTEE( size >= 0 && (size & 3) == 0, "Sanity check" );
  OopDesc** p = DERIVED( OopDesc**, _inline_allocation_end, size );
  OopDesc** top = _compiler_area_start;
  if( p > top ) {
    p = top;
  }
  set_inline_allocation_end( p );
  verify_layout();
}

#if ENABLE_COMPILER
inline
void ObjectHeap::increase_compiler_usage(size_t min_free_after_collection) {
  allocation_trap_must_be_disabled();

  const size_t slack = align_allocation_size((_heap_size / 32 + 5 * 1024));
  const size_t max_bytes =
    align_allocation_size(_heap_size * CompilerAreaPercentage / 100);
  const size_t total_bytes = compiler_area_size();
  const size_t free_bytes  = compiler_area_free();
  const size_t free_normal_bytes =
    DISTANCE(_inline_allocation_top, _compiler_area_start);

  if (free_normal_bytes <= slack) {
    // don't make the normal area too tight.
    return;
  }

  if (total_bytes >= max_bytes) {
    // Can't grow the compiler_area anymore: it's already bigger than 
    // max allowed
    return;
  }

  GUARANTEE(min_free_after_collection > free_bytes, "sanity");
  size_t growth_target = min_free_after_collection + slack - free_bytes;
  if (growth_target + slack > free_normal_bytes) {
    growth_target = free_normal_bytes - slack;
  }
  if (growth_target + total_bytes > max_bytes) {
    growth_target = max_bytes - total_bytes;
  }

  GUARANTEE(int(growth_target) > 0, "sanity");
  growth_target = align_allocation_size(growth_target);
  int compiler_area_shift = 0 - int(growth_target);

  compact_and_move_compiler_area( compiler_area_shift );
  verify_layout();
}

// Returns number of free bytes in the compiler area
int ObjectHeap::compiler_area_soft_collect(size_t min_free_after_collection){
  accumulate_current_task_memory_usage();

  // (0) check if collection is indeed necessary
  size_t free_bytes = compiler_area_free();
  if (free_bytes >= min_free_after_collection) {
    return free_bytes;
  }

  OopDesc** allocation_end = disable_allocation_trap();
  // (1) See if we have room to expand the compiler_area
  increase_compiler_usage(min_free_after_collection);

  free_bytes = compiler_area_free();
  if (free_bytes < min_free_after_collection
      && CompiledMethodCache::smart_evict_underweight() ) {
    compact_and_move_compiler_area( 0 );

    // New number of free bytes in the compiler area
    free_bytes = compiler_area_free();
  }
  clear_inline_allocation_area();

  enable_allocation_trap( allocation_end );
  return int(free_bytes);
}

inline int
ObjectHeap::compiler_area_compute_new_locations ( CompiledMethodDesc* dst ) {
  // We don't insert compiled methods into the cache when generating ROM image,
  // so references from methods to compiled methods will not be updated.
  // Something went wrong if we get here during image generation.
  GUARANTEE(!GenerateROMImage, 
            "Shouldn't get here when generating ROM image");

  if (TraceGC || TraceCompilerGC) {
    TTY_TRACE_CR(("TraceGC: compute compiled method new locations"));
  }

  const int upb = CompiledMethodCache::upb;
  int last_moving_up = -1;

  for( int i = 0; i <= upb; i++ ) {
    CompiledMethodDesc* p = CompiledMethodCache::Map[i];
    const int delta = DISTANCE(p, dst);
    if( delta ) {
      if( delta > 0 ) {
        last_moving_up = i;
      }
      // Change all relative offsets inside code
      if( p->code_size() > 0 ) {
        // need revisit : if code_size() == 0
        p->update_relative_offsets(delta);
      }
      address entry = dst->entry();
#if ENABLE_THUMB_COMPILER
      // Set the low bit so that BX will automatically switch into
      // THUMB mode.
      entry += 1;
#endif
      {
        MethodDesc* m = p->method();
        Method::Raw rm = m;
        GUARANTEE((m->has_compiled_code() &&
                   p == rm().compiled_code()) ==
                  rm().has_compiled_code( p ), "Sanity");

        if( rm().has_compiled_code( p ) ) {
          m->set_execution_entry(entry);
        }
      }
    }

    // Store delta in _klass to relocate frames.
    p->_klass = (OopDesc*)delta;

    if (TraceGC || TraceCompilerGC) {
      TTY_TRACE_CR(("TraceGC: 0x%p (%u bytes) => 0x%p (delta=%d)", p,
                     p->object_size(), dst, delta));
    }

    dst = DERIVED( CompiledMethodDesc*, dst, p->object_size() );
  }
  return last_moving_up;
}

inline void ObjectHeap::compiler_area_move_compiled_method ( 
  CompiledMethodDesc* dst, const CompiledMethodDesc* src, const int index )
{
  if (TraceGC || TraceCompilerGC) {
    TTY_TRACE_CR(("TraceGC: moving compiled method [%d] %p => %p, %d bytes",
      index, src, dst, src->object_size() ));
  }
#if ENABLE_JVMPI_PROFILE 
  if( UseJvmpiProfiler && 
      JVMPIProfile::VMjvmpiEventCompiledMethodUnloadIsEnabled() ){
    const MethodDesc* method_desc = src->method();
    const jint method_id = method_desc->method_id();
    JVMPIProfile::VMjvmpiPostCompiledMethodUnloadEvent(method_id);
  }
#endif

  CompiledMethodCache::Map[index] = dst;
  jvm_memmove(dst, src, src->object_size() );

#if ENABLE_JVMPI_PROFILE 
  if( UseJvmpiProfiler &&
      JVMPIProfile::VMjvmpiEventCompiledMethodLoadIsEnabled() ) {
    CompiledMethod::Raw compiled_method( dst );   
    JVMPIProfile::VMjvmpiPostCompiledMethodLoadEvent(compiled_method);
  }
#endif
}

inline void ObjectHeap::compiler_area_compact( const int last_moving_up ) {
  if (TraceGC || TraceCompilerGC) {
    TTY_TRACE_CR(("TraceGC: compact the compiled methods"));
  }

  OopDesc* const compiled_method_class =
    Universe::compiled_method_class()->prototypical_near();
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR 
   BranchTable::remove();  
#endif

#if ENABLE_CODE_PATCHING && USE_PATCHED_METHOD_CACHE
  {
    const CompiledMethodDesc* const cache = 
      CompiledMethodCache::patched_method();
    if (cache != NULL) {
      CompiledMethodDesc* const dst =
        DERIVED(CompiledMethodDesc*, cache, (int)cache->_klass);
      CompiledMethodCache::set_patched_method(dst);
    }
  }
#endif  

  {
    for( int i = last_moving_up; i >= 0; --i ) {
      const CompiledMethodDesc* const src = CompiledMethodCache::Map[i];
      CompiledMethodDesc* const dst =
        DERIVED(CompiledMethodDesc*, src, (int)src->_klass);
      compiler_area_move_compiled_method( dst, src, i );
      dst->_klass = compiled_method_class;
    }
  }
  {
    const int upb = CompiledMethodCache::upb;
    for( int i = last_moving_up; ++i <= upb; ) {
      const CompiledMethodDesc* const src = CompiledMethodCache::Map[i];
      CompiledMethodDesc* const dst =
        DERIVED(CompiledMethodDesc*, src, (int)src->_klass);
      if( src != dst ) {
        compiler_area_move_compiled_method( dst, src, i );
      }
      dst->_klass = compiled_method_class;
    }
  }
}

inline void ObjectHeap::compiler_area_update_pointers( void ) {
  if (TraceGC || TraceCompilerGC) {
    TTY_TRACE_CR(("TraceGC: update references to the compiled methods"));
  }

  OopDesc** const compiler_area_start = _compiler_area_start;
  OopDesc** const compiler_area_top   = _compiler_area_top;

  #define compiler_area_contains( p ) \
    compiler_area_start <= ((OopDesc**)p) && ((OopDesc**)p) < compiler_area_top

  // (1) Pointers in CompiledMethodCache are updated during compaction phase
  // (2) Update pointers in handles
  {
    ForAllHandles( handle ) {
      const CompiledMethodDesc* p = (const CompiledMethodDesc*)handle->obj();
      if( compiler_area_contains( p ) ) {
        const int delta = (int)p->_klass;
        if( delta ) {
          handle->set_obj( DERIVED( OopDesc*, p, delta ) );
        }
      }
    }
  }

  // (3) Update return addresses in Java stack frames
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
          CompiledMethodDesc* p = 
            (CompiledMethodDesc*)java_frame.compiled_method();
          if( compiler_area_contains( p ) ) {
            const int delta = (int)p->_klass;
            if( delta ) {
              JavaFrame copy = java_frame;
              java_frame.caller_is(frame);
              copy.set_pc( DERIVED( address, copy.pc(), delta ) );
              continue;
            }
          }
        }
        java_frame.caller_is(frame);
        #undef java_frame
      }
    }
  }
  #undef compiler_area_contains
}

CompiledMethodDesc* ObjectHeap::method_contains_instruction_of(void* pc){
  if( pc < (CompiledMethodDesc*)_compiler_area_top ) {
    CompiledMethodDesc* next = (CompiledMethodDesc*)_compiler_area_start;
    if( pc > next ) {
      CompiledMethodDesc* p;
      while( (p = next) < pc ) {
        next = DERIVED(CompiledMethodDesc*, p, p->object_size());
      }
      return p;
    }
  }
  return NULL;
}


#ifndef PRODUCT
int ObjectHeap::count_compiled_methods(void) {
  const CompiledMethodDesc* p = (CompiledMethodDesc*)_compiler_area_start;
  const CompiledMethodDesc* const end = (CompiledMethodDesc*)_compiler_area_top;
  int n = 0;
  for(; p < end; p = DERIVED(const CompiledMethodDesc*, p, p->object_size())) {
    n++;
  }
  return n;
}
#endif // PRODUCT

#endif // ENABLE_COMPILER

#if ENABLE_JVMPI_PROFILE_VERIFY && ENABLE_JVMPI_PROFILE
// Dump the Compiled method info to log files for the JVMPI verification.
void ObjectHeap::dumpAllCompiledMethodsInfo() {
   static int file_no;
   char file_name[256];
   sprintf(file_name, "cldc_dump_%d.txt", file_no++);

   FILE* dfp = fopen(file_name, "w+"); 
   if(dfp == NULL) {
     printf("Can't open cldc_dump.txt\n");
     return; 
   }       

  CompiledMethodDesc* p   = (CompiledMethodDesc*) _compiler_area_start;
  //CompiledMethodDesc* end = (CompiledMethodDesc*) _compiler_area_top;
  CompiledMethodDesc* end;
  
  if(_saved_compiler_area_top)
    end = (CompiledMethodDesc*) _saved_compiler_area_top;
  else
    end = (CompiledMethodDesc*) _compiler_area_top;
    
  char tmp_str[1024];
  
  while (p < end) {
    UsingFastOops fast_oops;
    
    CompiledMethod::Fast compiled_method  = p;
    Method::Fast method = compiled_method().method();
    InstanceClass::Fast insclass = method().holder();

    //dump class name                                                                         
    Symbol::Fast symbol_class_name = (Symbol::Fast)(insclass().original_name());
    symbol_class_name().string_copy(tmp_str, symbol_class_name().length() + 1);
    tmp_str[symbol_class_name().length()] = '\0';
    fprintf(dfp, "Class Name = %s\n", tmp_str);

    // dump method name
    bool renamed;
    Symbol::Fast symbol_method_name= method().get_original_name(renamed);
    int method_name_length = symbol_method_name().length();
    symbol_method_name().string_copy(tmp_str, method_name_length + 1);
    tmp_str[method_name_length] = '\0';
    fprintf(dfp, "Method Name = %s\n", tmp_str);
    
    // dump method signature
    TypeSymbol::Fast symbol_signature = method().signature();
    ByteArrayOutputStream baos;
    symbol_signature().print_decoded_on(&baos);
    char *q = baos.get_chars();
    fprintf(dfp, "Method Signature = %s\n", q);

    // dump method id
    int method_id = method().method_id();
    fprintf(dfp, "Method ID = %d\n", method_id);

    //dump compiled method start address
    address code_addr_start = compiled_method().entry();
    fprintf(dfp, "Start Address = 0x%x\n", (int)code_addr_start);

    //dump compiled method end address
    address code_addr_end = p->jvmpi_get_code_size() + code_addr_start;
    fprintf(dfp, "End Address = 0x%x\n", (int)code_addr_end);
    fprintf(dfp, "\n");

    fflush(dfp);
    
    p = DERIVED(CompiledMethodDesc*, p, p->object_size());
  }   

  fclose(dfp);
  
}
#endif

void ObjectHeap::compact_and_move_compiler_area(const int compiler_area_shift) {
#if ENABLE_COMPILER
  EnforceRuntimeJavaStackDirection enfore_java_stack_direction;

#if ENABLE_TTY_TRACE
  const size_t old_compiler_area_size = compiler_area_size();
  const size_t old_used_compiler_area = compiler_area_used();
  const int old_num_compiled_methods = count_compiled_methods();
#endif

  PERFORMANCE_COUNTER_INCREMENT(num_of_compiler_gc, 1);
  EventLogger::start(EventLogger::COMPILER_GC);

  if (_saved_compiler_area_top != NULL) {
    GUARANTEE_R(_saved_compiler_area_top == _compiler_area_top, 
                "compiler_area collect can happen only when no temporary "
                "compiler object has been created");
  }

  if (TraceGC || TraceCompilerGC) {
    TTY_TRACE_CR(("TraceGC: compiler_area = %dK, used=%dK", 
                  DISTANCE(_compiler_area_start, compiler_area_end())/1024,
                  DISTANCE(_compiler_area_start, _compiler_area_top)/1024));
  }

  _is_gc_active = true;
  {
    OopDesc** const compiler_area_start =
      DERIVED( OopDesc**, _compiler_area_start, compiler_area_shift );
    if( _compiler_area_start != _compiler_area_top ) {
      const int last_moving_up = compiler_area_compute_new_locations(
        (CompiledMethodDesc*) compiler_area_start );
      compiler_area_update_pointers();
      compiler_area_compact( last_moving_up );
    }
    _compiler_area_start = compiler_area_start;
    _compiler_area_top =
      DERIVED( OopDesc**, compiler_area_start, CompiledMethodCache::size );

    if( _inline_allocation_end > compiler_area_start ) {
      set_inline_allocation_end( compiler_area_start );
    }
    GUARANTEE(_inline_allocation_top <= compiler_area_start, "sanity");
    verify_layout();
  }
  _is_gc_active = false;

#if ENABLE_TTY_TRACE
  const size_t new_compiler_area_size = compiler_area_size();
  const size_t new_used_compiler_area = compiler_area_used();
  if (VerboseGC ||
      (TraceHeapSize && (old_compiler_area_size != new_compiler_area_size))) {
    char dir;
    if (old_compiler_area_size > new_compiler_area_size) {
      dir = 'v';
    } else if (old_compiler_area_size < new_compiler_area_size) {
      dir = '^';
    } else {
      dir = '=';
    }
    TTY_TRACE_CR(("COMPILER_AREA: %dK(%d=%dK) %c-> %dK(%d=%dK)", 
                  old_compiler_area_size/1024,
                  old_num_compiled_methods,
                  old_used_compiler_area/1024,
                  dir,
                  new_compiler_area_size/1024,
                  count_compiled_methods(),
                  new_used_compiler_area/1024));
  }
#endif


  if (_saved_compiler_area_top != NULL) {
    _saved_compiler_area_top = _compiler_area_top;
  }

  if (_compiler_area_start != _compiler_area_top) {
    OsMisc_flush_icache((address)_compiler_area_start,
                        DISTANCE(_compiler_area_start, _compiler_area_top));
  }
  EventLogger::end(EventLogger::COMPILER_GC);

  DIRTY_HEAP(_inline_allocation_top,
             DISTANCE(_inline_allocation_top, _compiler_area_start));
  DIRTY_HEAP(_compiler_area_top, compiler_area_free());

  if (VerifyGC) {
    verify();
  }
#else
  // !ENABLE_COMPILER: make _compiler_area_start == _compiler_area_top 
  OopDesc** const compiler_area_start =
    DERIVED( OopDesc**, _compiler_area_start, compiler_area_shift );
  _compiler_area_start = compiler_area_start;
  _compiler_area_top = compiler_area_start;

  if( !YoungGenerationAtEndOfHeap &&
      _inline_allocation_end > compiler_area_start ) {
    set_inline_allocation_end( compiler_area_start );
  }
#endif
}

#if USE_SET_HEAP_LIMIT || USE_LARGE_OBJECT_AREA
#if !USE_LARGE_OBJECT_AREA
inline 
#endif

void ObjectHeap::shrink_with_compiler_area( const int size ) {
#if ENABLE_COMPILER
  Compiler::abort_suspended_compilation();
#endif
  accumulate_current_task_memory_usage();
  OopDesc** const saved_allocation_end = ObjectHeap::disable_allocation_trap();

  int deficit = align_up( size - LargeObject::available() );
  if( deficit > 0 ) {
    OopDesc** allocation_end =
      YoungGenerationAtEndOfHeap ? _old_generation_end : _inline_allocation_top;
    int gap = DISTANCE( allocation_end, _compiler_area_top );
#if ENABLE_COMPILER
    if( deficit <= gap ) {
      gap -= DISTANCE( _compiler_area_start, _compiler_area_top );
      if( deficit > gap &&
          deficit <= (gap + CompiledMethodCache::zero_weight_space()) ) {
        gap += CompiledMethodCache::evict_underweight();
      }
    }
#endif
    if( deficit > gap ) {
      SETUP_ERROR_CHECKER_ARG;
      collect(size JVM_NO_CHECK);
      Thread::clear_current_pending_exception();

      deficit = size - LargeObject::available();
      allocation_end = YoungGenerationAtEndOfHeap ? _old_generation_end
                                                  : _inline_allocation_top;
    }
    gap = align_down( DISTANCE( allocation_end, _compiler_area_start ) );
    if( deficit > gap ) {
      deficit = gap;
    }
    compact_and_move_compiler_area( -int(deficit) );  
    clear_inline_allocation_area();
  }

  ObjectHeap::enable_allocation_trap( saved_allocation_end );
}
#endif

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
void
ObjectHeap::iterate(ObjectHeapVisitor* visitor, OopDesc** p, OopDesc** to) {
#if !defined(PRODUCT) && !defined(UNDER_ADS)
  OopDesc** previous = NULL;            // Useful for debugging
#endif
  while( p < to ) {
    visitor->do_obj((Oop*)&p);
#if !defined(PRODUCT) && !defined(UNDER_ADS)
    previous = p;
#endif
    p = DERIVED( OopDesc**, p, ((Oop*)&p)->object_size() );
  }
}

void ObjectHeap::iterate(ObjectHeapVisitor* visitor) {
  iterate( visitor, _heap_start, _inline_allocation_top );
#if ENABLE_COMPILER
  {
    const int upb = CompiledMethodCache::upb;
    for( int i = 0; i <= upb; i++ ) {
      visitor->do_obj( (Oop*)&CompiledMethodCache::Map[i]);
    }
  }

  CodeGenerator* gen = CodeGenerator::current();
  if( gen ) {
    visitor->do_obj( gen->compiled_method() );
  }
#endif
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

bool ObjectHeap::contains_live(OopDesc** target) {
  if (_heap_start <= target && target < _inline_allocation_top) {
    return true;
  }
  if (_compiler_area_start <= target && target < _compiler_area_top) {
    return true;
  }
#if USE_LARGE_OBJECT_AREA
  if( LargeObject::contains( (LargeObject*) target ) ) {
    return true;
  }
#endif
  return false;
}

#endif

#ifndef PRODUCT

void ObjectHeap::verify_layout() {
  // Main chunk
  GUARANTEE_R(_heap_start            <= _collection_area_start, "sanity");
  GUARANTEE_R(_collection_area_start <= _inline_allocation_top, "sanity");
  {
    OopDesc** p = _inline_allocation_end;
#if ENABLE_ISOLATES
    if( !p ) p = _real_inline_allocation_end;
#endif
    GUARANTEE_R(_inline_allocation_top <= p, "sanity");
  }
  GUARANTEE_R(_inline_allocation_end    <= _compiler_area_start,      "sanity");
  GUARANTEE_R(_compiler_area_start      <= _compiler_area_top,        "sanity");
  GUARANTEE_R(_compiler_area_top        <= _large_object_area_bottom, "sanity");
  GUARANTEE_R(_large_object_area_bottom <= _heap_top,                 "sanity");
  GUARANTEE_R(_heap_top                 <= _heap_limit,               "sanity");
  GUARANTEE_R(_heap_limit               <= _heap_end,                 "sanity");

  if( _compiler_area_temp_bottom ) {
    GUARANTEE_R(_compiler_area_start       <= _compiler_area_temp_bottom, "sanity");
    GUARANTEE_R(_compiler_area_temp_bottom <= _compiler_area_temp_top,    "sanity");
    GUARANTEE_R(_compiler_area_temp_top    <= _compiler_area_top,         "sanity");
  }

  // Bit-vector chunk
  GUARANTEE_R(_bitvector_start       <  (address)_slices_start, "sanity");

  GUARANTEE_R(DISTANCE(_heap_top, _heap_limit)
              == (int)(MimimumMarkingStackSize * sizeof(OopDesc*)),
              "Inconsistent minimum marking stack size");
}

OopDesc* ObjectHeap::slow_object_start(OopDesc** target) {
  // We do not use ObjectHeap::iterate here in since that
  // will iterate over the entire heap.
  {
    OopDesc** p = _heap_start;
    while (p < _inline_allocation_top) {
      OopDesc** next_p = (OopDesc**)((address)p + ((OopDesc*) p)->object_size());
      if (target >= p && target < next_p) {
        return (OopDesc*) p;
      }
      p = next_p;
    }
  }
  {
    CompiledMethodDesc* p   = (CompiledMethodDesc *)_compiler_area_start;
    CompiledMethodDesc* end = (CompiledMethodDesc *)_compiler_area_top;

    while (p < end) {
      size_t object_size = p->object_size();
      OopDesc** next_p = DERIVED(OopDesc**, p, object_size);

      if (target >= (OopDesc**)p && target < next_p) {
        return (OopDesc*) p;
      }

      p = (CompiledMethodDesc*)next_p;
    }
  }
  return NULL;
}

void ObjectHeap::verify_bitvector_alignment(OopDesc** p) {
  GUARANTEE_R((size_t) p % (BitsPerByte * BytesPerWord) == 0,
            "Bit for p should be on word boundary");
}

// Using static variables here is not too nice
OopDesc** _verify_barrier_start = NULL;

void verify_barrier_oop(OopDesc** p) {
  OopDesc** obj = (OopDesc**) *p;
  if (_verify_barrier_start <= obj && obj < ObjectHeap::compiler_area_end() ) {
    if (_heap_start <= p && p < _verify_barrier_start) {
#ifdef PRODUCT // IMPL_NOTE: consider whether it should be fixed. 
      GUARANTEE_R(ObjectHeap::test_bit_for(p), "missing oop write barrier");
#endif
    }
  }
}

class VerifyOopWriteBarrier : public ObjectHeapVisitor {
 public:
  virtual void do_obj(Oop* obj) {
    obj->obj()->near_do(verify_barrier_oop);
    obj->obj()->oops_do(verify_barrier_oop);
  }
};

void ObjectHeap::verify_bitvector_range(OopDesc** verify_start) {
  _verify_barrier_start = verify_start;
  VerifyOopWriteBarrier closure;
  ObjectHeap::iterate(&closure);
}

#if USE_DEBUG_PRINTING
class DumpHistogram : public ObjectHeapVisitor {
  virtual void do_obj(Oop* obj) {
    tty->print("%8d 0x%08x:  ", obj->object_size(), obj->obj());
    if (obj->is_instance_class()) {
      tty->print("Class\t");
      InstanceClass c = obj;
      Symbol name = c.name();
      name.print_symbol_on(tty);
      tty->cr();
    } else if (obj->is_method()) {
      Method m = obj;
      Symbol name = m.name();
      InstanceClass c = m.holder();
      Symbol classname = c.name();
      Symbol signature = m.signature();
      tty->print("Method\t");
      classname.print_symbol_on(tty);
      tty->print("\t");
      name.print_symbol_on(tty);
      signature.print_symbol_on(tty);
      tty->cr();
#if ENABLE_COMPILER
    } else if (obj->is_compiled_method()) {
      CompiledMethod cm = obj;
      Method m = cm.method();
      bool is_live = ((address)cm.obj() == m.execution_entry());
      Symbol name = m.name();
      Symbol signature = m.signature();
      InstanceClass c = m.holder();
      Symbol classname = c.name();
      if (is_live) {
        tty->print("LiveCode\t");
      } else {
        tty->print("DeadCode\t");
      }
      classname.print_symbol_on(tty);
      tty->print("\t");
      name.print_symbol_on(tty);
      signature.print_symbol_on(tty);
      tty->cr();
#endif
    } else {
      tty->print("Other\t");
      obj->print_value_on(tty);
      tty->cr();
    }
  }
};

void ObjectHeap::dump_histogram_data() {
  tty->cr();
  tty->print_cr("%%HistogramHeader");
  DumpHistogram closure;
  ObjectHeap::iterate(&closure);
  tty->print_cr("Free\t%d", ObjectHeap::free_memory());
  tty->print_cr("HeapSize\t%d", ObjectHeap::total_memory());
}
#endif

void ObjectHeap::verify_near_oop(OopDesc** p) {
  GUARANTEE_R(contains_live(p) || ROM::in_any_loaded_bundle((OopDesc*)p),
              "must be in object space");

  if (ROM::system_text_contains((OopDesc*)p)) {
    // p must point to the start of a valid ROM object
    GUARANTEE_R(ROM::is_valid_text_object((OopDesc*)p), "must be valid");

    // rom_near  - near pointer as stored in ROM
    // real_near - real near pointer, if this object were stored in heap.
    OopDesc* rom_near  = *p;
    OopDesc* real_near = ROM::text_klass_of((OopDesc*)p);
    if (rom_near != real_near) {
      // p has a skipped header. We have no near pointer to follow.
      return;
    }
  }

  OopDesc* obj = *p;
  GUARANTEE_R(obj != NULL, "near obj cannot be NULL");
  if (!contains_live(obj) && !ROM::system_contains(obj) ) {
    // Near class points outside heap, should point to a JavaNearDesc on the
    // stack
    // We could potentially verify that obj is inside a valid Java stack area
    GUARANTEE_R(!contains(obj), "should not point to dead heap area");
    JavaNearDesc* stack_near = (JavaNearDesc*) obj;
    OopDesc* locked_obj_on_stack = *(OopDesc**)(stack_near + 1);
    GUARANTEE_R((OopDesc*)p == locked_obj_on_stack,
              "pointer to locked object should be just below stack near");
    StackLock* lock = (StackLock*) stack_near - 1;
    GUARANTEE_R(contains_live(lock->real_java_near()),
              "invalid real near obj for locked object");
  }
  // Jump through farclass->near->farclassclass->near->metaclass pointers
  for (int i = 0; i < 5; i++) {
    obj = *(OopDesc**)obj;
    GUARANTEE_R(contains_live(obj) || ROM::system_contains(obj),
              "invalid near obj or far class");
  }
#if AZZERT && !ENABLE_HEAP_NEARS_IN_HEAP 
  // We should be at meta class now.
  // but there are cloned meta class in case of ENABLE_HEAP_NEARS_IN_HEAP 
  OopDesc* the_meta_class = Universe::meta_class()->obj();
  GUARANTEE_R(obj == the_meta_class, "invalid meta class");
#endif
}

void ObjectHeap::verify_other_oop(OopDesc** p) {
  OopDesc* obj = *p;
  if (obj != NULL) {
    verify_near_oop((OopDesc**) obj);
  }
}

class VerifyObjects : public ObjectHeapVisitor {
  virtual void do_obj(Oop* obj) {
    // Verify oops
    GUARANTEE( !(_compiler_area_top <= (OopDesc**)obj && ObjectHeap::compiler_area_end() > (OopDesc**) obj),
      "Temp compiler object" );
    obj->obj()->near_do(ObjectHeap::verify_near_oop);
    obj->obj()->oops_do(ObjectHeap::verify_other_oop);
  }
};

void ObjectHeap::verify_only_permanent_pointers(OopDesc** p) {
  OopDesc* obj = *p;
  GUARANTEE_R(!contains(obj) || permanent_contains((OopDesc**)obj),
              "unscanned DATA block must not contain moveable pointers");
}

void ObjectHeap::verify() {
  if (Verbose || VerboseGC) {
    tty->print("[Verify heap");
  }
  verify_layout();

  OopDesc* saved_exception = Thread::current_pending_exception();
  Thread::clear_current_pending_exception();

  ExecutionStackDesc *this_stack = ExecutionStackDesc::_stack_list;
  while (this_stack) {
    if ((address)this_stack < (address)_collection_area_start) {
      int size = this_stack->object_size();
      OopDesc** start =
          (OopDesc**)this_stack->field_base(ExecutionStackDesc::header_size());
      OopDesc** end  =  (OopDesc**)this_stack->field_base(size);
      for (OopDesc** p = start; p < end; p++) {
        GUARANTEE_R(!test_bit_for(p), "No bits set in Execution stack");
      }
    }
    this_stack = this_stack->_next_stack;
  }

  // Note: we need to convert derived pointers into real pointers
  //       before iterating over the oops.

  Scheduler::gc_prologue(do_nothing);

  // verify thread stacks
  this_stack = ExecutionStackDesc::_stack_list;
  while (this_stack) {
    this_stack->oops_do(verify_other_oop);
    OopDesc* thread = this_stack->_thread;
    if (thread) {
       thread->oops_do(verify_other_oop);
    }
    this_stack = this_stack->_next_stack;
  }

  // Verify external roots
  roots_do( verify_other_oop );

  ROM::oops_do(verify_other_oop, true, true);
  finalizer_oops_do(_finalizer_reachable, verify_other_oop);
  finalizer_oops_do(_finalizer_pending,   verify_other_oop);

  // Verify heap contents
  VerifyObjects closure;
  ObjectHeap::iterate(&closure);
  verify_bitvector_range(_heap_start_bitvector_verify);
  Scheduler::gc_epilogue();
  {
    // Pointers in the 'unscanned' part of the ROM DATA block can point
    // only to permanent part of object heap.
    OopDesc *obj, *end;
    obj = DERIVED(OopDesc*, _rom_data_block, _rom_data_block_scanned_size);
    end = DERIVED(OopDesc*, _rom_data_block, _rom_data_block_size);

    while (obj < end) {
      obj->oops_do(verify_only_permanent_pointers);
      obj = DERIVED(OopDesc*, obj, obj->object_size());
    }
  }

#if ENABLE_COMPILER
  CompiledMethodCache::verify();
#endif

  LargeObject::verify();

  Thread::set_current_pending_exception(saved_exception);

  if (Verbose || VerboseGC) {
    TTY_TRACE(("]"));
  }
}

class CountObjects : public ObjectHeapVisitor {
  public:
      CountObjects() { _n = 0; }
      virtual void do_obj(Oop* obj) { _n++; (void)obj; }
      int result() { return _n; }
  private:
      int _n;
};

class CodeSummary : public ObjectHeapVisitor {
  public:
      CodeSummary() { _total = 0; }

      virtual void do_obj(Oop* obj) {
        if (obj->is_compiled_method()) {
            _total += obj->object_size();
        }
      }

      int result() { return _total; }
  private:
      int _total;
};

class CodeItemSummary : public ObjectHeapVisitor {
  public:
      CodeItemSummary() { _total = 0; }

      virtual void do_obj(Oop* obj) {
        if (obj->is_compiled_method()) {
            _total ++;
        }
      }

      int result() { return _total; }
  private:
      int _total;
};

int ObjectHeap::code_size_summary() {
  CodeSummary closure;
  ObjectHeap::iterate(&closure);
  return closure.result();
}

int ObjectHeap::code_item_summary() {
  CodeItemSummary closure;
  ObjectHeap::iterate(&closure);
  return closure.result();
}

int ObjectHeap::count_objects() {
  CountObjects closure;
  ObjectHeap::iterate(&closure);
  return closure.result();
}

bool ObjectHeap::permanent_contains(OopDesc** obj) {
  return _heap_start <= obj && obj < _permanent_generation_top;
}
#endif

// This function implements JVM_GarbageCollect, which is an external
// interface for MIDP to invoke GC.
int ObjectHeap::jvm_garbage_collect(int flags, int requested_free_bytes) {
  OopDesc** const allocation_end = disable_allocation_trap();

  SETUP_ERROR_CHECKER_ARG;

  if ((flags & JVM_COLLECT_COMPILER_AREA) == 0) {
    if ((flags & JVM_COLLECT_YOUNG_SPACE_ONLY) == 0) {
      force_full_collect();
    }
    internal_collect(requested_free_bytes JVM_NO_CHECK);
  } else {
    if ((flags & JVM_COLLECT_YOUNG_SPACE_ONLY) == 0) {
      // compaction if compiler area is allowed only if doing full collect.
      collect(requested_free_bytes JVM_NO_CHECK);
    } else {
      internal_collect(requested_free_bytes JVM_NO_CHECK);
    }
  }
  clear_inline_allocation_area();

  Thread::clear_current_pending_exception();

  const int avail = free_memory();
  enable_allocation_trap( allocation_end );
  return avail;
}

extern "C" int JVM_GarbageCollect(int flags, int requested_free_bytes) {
  return ObjectHeap::jvm_garbage_collect(flags, requested_free_bytes);
}

#if USE_SET_HEAP_LIMIT
inline void* ObjectHeap::set_heap_limit( void* new_heap_limit ) {
  // Some implementation of malloc() may inadvertently recurse into
  // ObjectHeap::set_heap_limit() -- e.g., if a finalizer is called
  // which calls free(). This check makes it more robust. See CR 4969013.
  static bool in_use = false;
  if( !in_use && _heap_min == _heap_capacity ) {
    in_use = true;

    new_heap_limit = (void*)align_down(unsigned(new_heap_limit));
#ifdef AZZERT
    const size_t minimum_marking_stack_size = DISTANCE(_heap_top, _heap_limit);
    GUARANTEE(minimum_marking_stack_size
                    == MimimumMarkingStackSize * sizeof(OopDesc *),
              "Inconsistent minimum marking stack size");
    GUARANTEE(!YoungGenerationAtEndOfHeap, 
              "not supported with user-administered space");
#endif
    int reduction = DISTANCE(new_heap_limit, _heap_limit);
    if( reduction ) {
      if( reduction > 0 ) {
        shrink_with_compiler_area( reduction );
        const int available = align_down( LargeObject::available() );
        if( reduction > available ) {
          reduction = available;
        }
      }
#if !ENABLE_COMPILER
      else {
        OopDesc** const compiler_area_start =
          DERIVED( OopDesc**, _compiler_area_start, -reduction );
        _compiler_area_start = compiler_area_start;
        _compiler_area_top = compiler_area_start;
      }
#endif

      OopDesc** const old_heap_top = _heap_top;
      OopDesc** const heap_top = DERIVED( OopDesc**, old_heap_top, -reduction );
      LargeObject::move( -reduction, old_heap_top );
      _heap_top = heap_top;
      _heap_limit = heap_top + MimimumMarkingStackSize;
      _young_generation_target_size = align_up(
        DISTANCE(_heap_start, heap_top) / YoungGenerationTarget );
      verify_layout();
    }

    in_use = false;
  }
  return (void*)_heap_limit; 
}

void ObjectHeap::get_heap_info(void **heap_start, void **heap_limit, 
                               void **heap_end) {
  GUARANTEE(!YoungGenerationAtEndOfHeap, 
            "not supported with user-administered space");
  /*
   * When this method is called, we assume that ObjectHeap::set_heap_limit()
   * would be called soon to manipulate the user-administered space.
   * Since set_heap_limit() does not support adjustable-size heaps, let's
   * grow the heap to the full size.
   *
   * NOTE: this code is necessary only on Linux and Win32, for testing
   * purposes. Few embedded platforms would require both adjustable-size heap
   * *and* user-administered space.
   */

  OopDesc** const allocation_end = disable_allocation_trap();
  accumulate_current_task_memory_usage();
  if (_heap_min != _heap_capacity) {
    const size_t requested_bytes = free_memory() + 
                              ((size_t)_heap_capacity - _heap_size);
    try_to_grow(requested_bytes, true);
    clear_inline_allocation_area();
    GUARANTEE((int)_heap_size >= _heap_capacity, "Heap must be fully expanded");
    _heap_min = _heap_capacity;
  }

  *heap_start = (void*) _heap_start;
  *heap_limit = (void*) _heap_limit;
  *heap_end   = (void*) _heap_end;

  enable_allocation_trap( allocation_end );
}

void JVM_GetHeapInfo(void **heap_start, void **heap_limit, void **heap_end) {
  ObjectHeap::get_heap_info(heap_start, heap_limit, heap_end);
}

void* JVM_SetHeapLimit(void *new_heap_limit) {
  GUARANTEE(HeapMin == HeapCapacity, "Does not work with expanding heap");
  return ObjectHeap::set_heap_limit(new_heap_limit);
}

#endif // USE_SET_HEAP_LIMIT

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void ObjectHeap::save_java_stack_snapshot() {
  ::save_java_stack_snapshot();
}

void ObjectHeap::print(Stream* st) {
  st->print_cr("Object heap       [0x%x,0x%x), %d bytes", 
                                   _heap_start, _heap_top,
                                   int(_heap_top) - int(_heap_start));
  st->print_cr("total/free bytes    [%d,%d]", total_memory(), free_memory());
  st->print_cr("- Collection area   [0x%x,0x%x, 0x%x)",
    _collection_area_start, _inline_allocation_top, _inline_allocation_end);
  st->print_cr("- Compiler area     [0x%x,0x%x, 0x%x)",
    _compiler_area_start, _compiler_area_top, compiler_area_end());
  st->print_cr("- Large object area [0x%x,0x%x, 0x%x)",
    LargeObject::start(), LargeObject::bottom(), LargeObject::end());
  st->print_cr("Min marking stack   [0x%x,0x%x)", _heap_top, _bitvector_start);
  st->print_cr("Bit vector          [0x%x,0x%x), %d bytes",
                                  _bitvector_start, _slices_start,
                                  int(_slices_start) - int(_bitvector_start));
  st->print_cr("Slices start      0x%x",        _slices_start);
  st->print_cr("Slice size        %d",          _slice_size);
  st->print_cr("Number of slices  %d (=%d bytes)", _nof_slices,
                                               _nof_slices * sizeof(OopDesc*));
  if (Verbose) {
    for (size_t i = 0; i < _nof_slices; i++) {
      st->print_cr("- Slice[%4d] 0x%x", i, _slices_start[i]);
    }
  }
  print_task_usage(st);
}

void ObjectHeap::print_task_usage(Stream *st) {
#if ENABLE_ISOLATES
  if (VerboseGC || TraceGC || TraceHeapSize) {
    ForTask(task) {
      st->print_cr("Task %2d usage %d", task, _task_info[task].usage);
      if (!Universe::before_main()) {
        Task::Raw t = Task::get_task(task);
        if (!t.is_null()) {
          st->print_cr("   Task string count %d", t().string_table_count());
          st->print_cr("   Task symbol count %d", t().symbol_table_count());
        }
      }
    }
  }
#else
  (void)st;
#endif
}

class PrintObjects : public ObjectHeapVisitor {
public:
  PrintObjects(Stream* st) { _st = st;}
  virtual void do_obj(Oop* obj) {
#if ENABLE_ISOLATES
    if (obj->is_boundary()) {
      OopDesc *n = obj->klass();
      for (int i=0; i<MAX_TASKS; i++) {
        if (n == Universe::boundary_near_list()->obj_at(i)) {
          _st->print("^^^^^^^^^^^^^^^^^^[task %d]^^^^^^^^^^^^^^^ ", i);
          break;
        }
      }
    }
#endif
    int size = obj->object_size();
    _st->print("0x%x (%d) ", obj->obj(), size);
    obj->print_value_on(_st);
    _st->cr();
  }
private:
  Stream* _st;
};

void ObjectHeap::print_all_objects(const JvmPathChar *file_name) {
  FileStream file(file_name);
  print_all_objects(&file);
}

void ObjectHeap::print_all_objects(Stream *st) {
  PrintObjects closure(st);
  ObjectHeap::iterate(&closure);
}

class PrintClasses : public ObjectHeapVisitor {
  virtual void do_obj(Oop* obj) {
    if (obj->is_instance_class()) obj->print();
  }
};

void ObjectHeap::print_all_classes() {
  PrintClasses closure;
  ObjectHeap::iterate(&closure);
}

// Find all pointers in the heap that points to the given object.
void ObjectHeap::find(OopDesc *target, bool verbose_owner) {
  for (OopDesc** ptr = _heap_start; ptr < _inline_allocation_top; ptr++) {
    if (*ptr == target) {
      OopDesc *owner = ObjectHeap::slow_object_start(ptr);
      int offset = DISTANCE(owner, ptr);
      tty->print("0x%08x <- [ 0x%08x + %3d ]", target, owner, offset);
      if (verbose_owner) {
        VerbosePointers --;
        Oop::Raw oop = owner;
        tty->print(" ");
        oop().print_value_on(tty);
        VerbosePointers ++;
      }
      tty->cr();
    }
  }
}

bool ObjectHeap::reach_seen(OopDesc *n, ObjectHeap::ReachLink *stack, 
                            int endstack) {
  for (int i=0; i<=endstack; i++) {
    if (stack[i].node == n) {
      return true;
    }
  }
  return false;
}

bool ObjectHeap::check_reach_root(OopDesc *n, ObjectHeap::ReachLink *stack,
                                  int parent) {
  bool found = false;
  int max = Universe::__number_of_persistent_handles;
  for (int i=0; i<max; i++) {
    if (persistent_handles[i] == n) {
      tty->print_cr("0x%08x <- persistent_handles[%d]", n, i);
      found = true;
      break;
    }
  }

  if (!found) {
    Oop *o = (Oop *)_last_handle;
    while (o != NULL) {
      if (o->obj()== n) {
        tty->print_cr("0x%08x <- [stack_handle: 0x%08x ]", n, o);
        found = true;
        break;
      }
      o = (Task*)o->previous();
    }
  }

  if (!found) {
    if (n == Thread::current()->obj()) {
      tty->print_cr("0x%08x <- current_thread", n);
      found = true;
    }
  }

  if (found && stack != NULL) {
    Verbose++;
    for (;;) {
      Oop::Raw oop = n;
      oop().print_value_on(tty);
      tty->cr();

      if (parent == -1) {
        break;
      } else {
        n = stack[parent].node;
        parent = stack[parent].parent;
      }
    }
    Verbose--;
  }

  return found;
}

void ObjectHeap::reach(OopDesc *target) {
  ReachLink stack[2048];
  const int max_stack = sizeof(stack)/sizeof(stack[0]);

  stack[0].parent = -1;
  stack[0].node = target;

  int current = 0;
  int endstack = 0;

  while (current <= endstack) {
    // [1] Discover all objects that are reachable from the current tree
    int next_current = endstack + 1;
    int loop_end = endstack;
    for (int i=current; i<=loop_end; i++) {
      OopDesc *n = stack[i].node;
      for (OopDesc**ptr = _heap_start; ptr < _inline_allocation_top; ptr++) {
        if (*ptr == n) {
          OopDesc *owner = ObjectHeap::slow_object_start(ptr);
          if (!reach_seen(owner, stack, endstack)) {
            if (check_reach_root(owner, stack, i)) {
              return;
            }
            if (endstack >= max_stack) {
              tty->print_cr("reachability stack overflow");
              return;
            }
            endstack ++;
            stack[endstack].parent = i;
            stack[endstack].node = owner;
            if (Verbose) {
              tty->print_cr("%4d = 0x%08x", endstack, owner);
            }
          }
        }
      }
    }
    // [2] Advance to the next set
    current = next_current;
  }

  tty->print_cr("0x%08x doesn't seem to be reachable", target);
  return;
}

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE

#if ENABLE_ISOLATES
  #undef ForTask
#endif
