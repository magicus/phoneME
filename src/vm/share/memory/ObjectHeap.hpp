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

// Support for inline contiguous allocation by interpreted and compiled code.
// top and end will point to an area currently designated for inline allocation.

// Object Heap

// The GC algorithm is a two-generational mark-sweep-compact collector.
//
// Marking is done in a side bitvector covering the entire heap,
// one bit per possible object location, or one bit per heap word.
// The bitvector also serves as a "card marking" write barrier,
// except that exact pointer locations rather than "cards" are marked.
//
// Flag YoungGenerationTarget specifies how much of the heap should
// be used as "young generation", and the collector does a mark-compact
// of this area only, using the "bit card marks" as roots.
// The young generation is a window sliding up towards the end of the
// heap. When the free area gets too small the heap is allocated full
// and a full collection is done.

// The collector only scans live objects in all phases: the bit vector
// is scanned whenever dead objects need to be skipped.
//
// For objects that move, forwarding pointers are installed in high bits
// of the near pointer, and the real near pointer is made relative to
// heap start and stored in lower bits. The high bits contain a relative
// offset to the start of a "slice", with absolute offsets stored in a
// small fixed size "slice offset table".
// The collector later has to find the far class by jumping through
// potentially "encoded" near pointers.
//
// The marking stack is allocated at the top of the heap.
//
// No extra data structures are used.
//
// Finalization support is present.
//
//
// Heap layout (two disjoint memory chunks with adjustable size)
//
//  CHUNK 1 (contiguous low to high addresses):
//  ----------------------------
//                                <-- _preallocated_space (optional)
//  Monet Binary Image (SVM mode only)
//                                <-- _glue_code
//  Compiler glue code
//                                <-- _heap_start
//  Old generation (full)
//                                <-- _collection_area_start
//  Young generation (full)
//                                <-- _inline_allocation_top
//  Young generation (empty)
//                                <-- _inline_allocation_end
//  Unused heap (if any)
//                                <-- _compiler_area_start
//  Compiler area
//                                <-- _compiler_area_top
//  Unused compiler area (if any)
//                                <-- _large_object_area_bottom
//  Large objects (if any)
//                                <-- _heap_top
//  Minimum marking stack
//                                <-- _heap_limit
//  User-administered extra space
//  reservable by set_heap_limit()
//                                <-- _heap_end
//  ----------------------------
//
//  CHUNK 2 (contiguous low to high addresses):
//  ----------------------------
//                                <-- _bitvector_start
//  Marking bit vector
//                                <-- _slices_start
//  Slice offset table
//  ----------------------------
//
//
//
// IMPL_NOTE:
// - Fnumber_of_conider improved marking scheme, using both explicit stack
//   and limited recursion.
// - For the "update_object_pointers" pass, consider making it the
//   invariant that near objs are always allocated on a higher address
//   (by allocating the reflective objects high->low), and then scanning
//   from high to low. This would fold the three passes into one.
// - Deadwood compaction.
// - Fold bit vector chunk into heap chunk,
//   avoid wasting unneeded marking bit vector space
//   when set_heap_limit() effectively shrinks the heap

#if ENABLE_ISOLATES
class BoundaryDesc;

class TaskMemoryInfo {
 public:
  unsigned reserve;
  unsigned limit;
  unsigned usage;     // computed at the latest GC
  unsigned estimate;  // estimated upper bound
#if ENABLE_PERFORMANCE_COUNTERS
  unsigned max_usage;
#endif

  void reset              ( void );
#if ENABLE_PERFORMANCE_COUNTERS
  void compute_max_usage  ( void );
#endif
  int  unused             ( void ) const;
};
#endif

class ObjectHeap : public AllStatic {
private:
  // These are the global variables that frequently used by small
  // functions inside the GC. These variables can be loaded a lot
  // faster on ARM (and possibly other CPUs) if they are access as
  // fields in a struct, instead of from disparate global vars.
  class QuickVars {
  public:
    OopDesc**  heap_start;
    OopDesc**  compaction_start;
    OopDesc**  collection_area_start;
    OopDesc**  collection_area_end;
    OopDesc*** slices_start;

    size_t     slice_shift;
    size_t     slice_offset_bits;
    size_t     slice_size;
    size_t     near_mask;
    size_t     slice_offset_mask;

    OopDesc**  rom_text_start;
    size_t     rom_text_size;
    OopDesc**  rom_data_start;
  };

  static QuickVars _quick_vars;

  static void collect(size_t min_free_after_collection JVM_TRAPS);

#if ENABLE_COMPILER
  static OopDesc* compiler_area_allocate_code (const int size JVM_TRAPS);
  static OopDesc* compiler_area_allocate_temp (const int size JVM_TRAPS);  
  static void compiler_area_initialize( OopDesc** start, OopDesc** end ) {
    _compiler_area_temp_bottom = start;
    _compiler_area_temp_top = end;
    _compiler_area_top = end;
  }
  static void compiler_area_terminate( OopDesc** end ) {
    _compiler_area_top = end;
#ifndef PRODUCT
    _compiler_area_temp_top = NULL;
    _compiler_area_temp_bottom = NULL;
#endif   
  }

#endif

public:
  
  static OopDesc* allocate(size_t size JVM_TRAPS);
  // This function implements JVM_GarbageCollect, which is an external
  // interface for MIDP to invoke GC.
  static int jvm_garbage_collect(int flags, int requested_free_bytes);
  // Allocate normal object in the heap without clearing the size to null.
#if defined(AZZERT)
  inline static OopDesc* allocate_raw(size_t size JVM_TRAPS) {
    // In AZZERT mode there are too many checks needed that live inside
    // allocate. There's no reason to duplicate them here.
    OopDesc* p = allocate(size JVM_NO_CHECK_AT_BOTTOM);
    if (p) {
      DIRTY_HEAP(p, size);
    }
    return p;
  }
#elif ENABLE_ISOLATES
  inline static OopDesc* allocate_raw(size_t size JVM_TRAPS) {
    // In MVM mode there are too many checks needed that live inside
    // allocate. There's no reason to duplicate them here.
    return allocate(size JVM_NO_CHECK_AT_BOTTOM);
  }
#else
  // A quick version for (!AZZERT && !ENABLE_ISOLATES). This version makes
  // class loading about 1% faster.
  static OopDesc* allocate_raw(size_t size JVM_TRAPS);
#endif

  // Creates an identical copy of an object in the heap
  // (including _klass field) - use with CARE
  static OopDesc* clone(OopDesc* source JVM_TRAPS);
  
  inline static OopDesc **allocation_top() { return _inline_allocation_top; }

  // Deallocate the heap
  static void dispose();

  // Set the size of the object heap and the adjacent user allocation space
  // by setting the pointer delineating the border between the two spaces
  static void* set_heap_limit ( void* new_heap_limit );
  static void set_heap_top    ( OopDesc** const new_heap_top );
  static void get_heap_info   ( void **heap_start, void **heap_limit, 
                                void **heap_end);

  // Expand the young generation to the entire heap
  static void expand_young_generation();

  // Expand the young generation
  static void expand_young_generation ( const int size );

  static void clear_inline_allocation_area() {
#if ENABLE_ZERO_YOUNG_GENERATION
    fast_memclear(_inline_allocation_top,
                  DISTANCE(_inline_allocation_top, _inline_allocation_end));
#endif
        }

  // Collection
  static void full_collect(JVM_SINGLE_ARG_TRAPS);
  static void safe_collect(size_t min_free_after_collection JVM_TRAPS)
#if ENABLE_ISOLATES
    ;
#else
    {
      collect( min_free_after_collection JVM_NO_CHECK );
      clear_inline_allocation_area();
    }
#endif
  static OopDesc** disable_allocation_trap( void ) {
#if ENABLE_ISOLATES
    OopDesc** const allocation_end = _inline_allocation_end;
    _inline_allocation_end = _real_inline_allocation_end;
    return allocation_end;
#else
    return NULL;
#endif
  }

  static void enable_allocation_trap(OopDesc** allocation_end) {
#if ENABLE_ISOLATES
    _inline_allocation_end = allocation_end == NULL ? allocation_end :
      current_task_allocation_end();
#else
   (void)allocation_end;
#endif
  }

  static void allocation_trap_must_be_disabled( void ) {
#if ENABLE_ISOLATES
    GUARANTEE(_inline_allocation_end == _real_inline_allocation_end,
      "allocation trap must be disabled");
#endif
  }

  static void accumulate_current_task_memory_usage( void )
#if ENABLE_ISOLATES
  ;
#else
  {}
#endif

  static int compiler_area_soft_collect(size_t min_free_after_collection);

  // Global reference support (bitwise mask)
  enum ReferenceType {
    STRONG = 0x01,
    WEAK   = 0x02
  };
  static int register_global_ref_object(Oop* referent,
                       ReferenceType type JVM_TRAPS); // Cannot throw OOME
  static void unregister_global_ref_object  (const int ref_index);
  static OopDesc* get_global_ref_object     (const int ref_index);
  static int make_global_reference( const int i ) {
#if ENABLE_ISOLATES
    return (i << LOG_MAX_TASKS) | _current_task_id;
#else
    return i;
#endif
  }
  static int get_global_reference_owner( const int ref ) {
#if ENABLE_ISOLATES
    return ref & (MAX_TASKS-1);
#else
    (void)ref;
    return 0;
#endif
  }
  static int get_global_reference_index( const int ref ) {
#if ENABLE_ISOLATES
    return ref >> LOG_MAX_TASKS;
#else
    return ref;
#endif
  }

  //found the compiled method which contains the instruction pointed 
  //by pc parameter
  static CompiledMethodDesc* method_contains_instruction_of(void* pc);

  // Finalization support
private:
  // Finalizer lists
  enum {
#if ENABLE_ISOLATES
    NUM_OF_FINALIZERS = MAX_TASKS
#else
    NUM_OF_FINALIZERS = 1
#endif
  };

  static FinalizerConsDesc* _finalizer_reachable[ NUM_OF_FINALIZERS ];
  static FinalizerConsDesc* _finalizer_pending  [ NUM_OF_FINALIZERS ];

  static void init_finalizers( void );

  static void register_finalizer_reachable_object(Oop* referent JVM_TRAPS);
  static void mark_finalizers( FinalizerConsDesc** list );
  static void unmark_pending_finalizers( void );
  static void update_interior_pointers( FinalizerConsDesc** list );
  static void finalize( FinalizerConsDesc** list );
  static void finalize( FinalizerConsDesc** list, const int task );
  static void set_heap_limit0( void* new_heap_limit );
public:
  static address glue_code_start() {
    return _glue_code;
  }
  static void finalize_all( void ) {
    finalize( _finalizer_reachable );
    finalize( _finalizer_pending   );
  }

#if ENABLE_ISOLATES
  static void finalize_task( const int task ) {
    finalize( _finalizer_reachable, task );
    finalize( _finalizer_pending,   task );
  }

  // Notifications  
  static int  on_task_switch      ( const int task_id ); // Returns previous task
  static void on_task_termination ( OopDesc* task );

  // Resource ownership override
  // Thread switch may restore local mode.
  static int start_system_allocation ( void ) {
    return on_task_switch( SYSTEM_TASK );
  }

  static void finish_system_allocation ( const int task ) {
    on_task_switch( task );
  }

  // Accessors
  static unsigned get_task_memory_usage( const int task ) {
    return get_task_info( task ).usage;
  }

  static unsigned get_task_memory_estimate( const int task ) {
    return get_task_info( task ).estimate;
  }

  static unsigned get_task_memory_reserve( const int task ) {
    return get_task_info( task ).reserve;
  }

  static unsigned get_task_memory_limit( const int task ) {
    return get_task_info( task ).limit;
  }

  static void set_task_memory_quota(const int task,
                      const unsigned reserve, const unsigned limit JVM_TRAPS);

  static void reset_task_memory_usage( const int task );

#if ENABLE_PERFORMANCE_COUNTERS
  static void print_max_memory_usage ( void );
#endif

  static int owner_task_id( const OopDesc* const object );

#else // !ENABLE_ISOLATES
  static int  start_system_allocation ( void ) { return 0; }
  static void finish_system_allocation( const int /* task */ ) {}
#endif

  // Sizing info
  static size_t used_memory( void ) {
    // All space in compiler and large object areas is considered to be used
    return DISTANCE(_heap_start, _old_generation_end)
         + DISTANCE(_young_generation_start, _inline_allocation_top)
         + DISTANCE(_compiler_area_start, _heap_top);
  }
  static size_t free_memory( void ) {
    // Space in compiler and large object areas is not counted
    return DISTANCE(_old_generation_end, _young_generation_start)
         + DISTANCE(_inline_allocation_top, _compiler_area_start);
  }
  static size_t total_memory( void ) {
    return _heap_size;
  }

#if ENABLE_ISOLATES
  static int available_for_current_task();
#else
  static inline int available_for_current_task() {
    return free_memory();
  }
#endif

  static OopDesc** mark_area_end (void) {
    return _large_object_area_bottom;
  }

  static bool compiler_area_in_use( void );

#if ENABLE_COMPILER
  static void save_compiler_area_top( void ) {
    GUARANTEE(_saved_compiler_area_top == NULL, "cannot be nested");
    _saved_compiler_area_top = _compiler_area_top;
  }

  static void update_compiler_area_top(const OopDesc* latest_compiled_method) {
    OopDesc** compiler_area_top = _saved_compiler_area_top;
    if( latest_compiled_method ) {
      GUARANTEE(latest_compiled_method == (OopDesc*)compiler_area_top, "sanity");
      compiler_area_top = DERIVED(OopDesc**, compiler_area_top,
                                   latest_compiled_method->object_size());
    }
    compiler_area_terminate( compiler_area_top );
  #ifndef PRODUCT
    _saved_compiler_area_top = NULL;
  #endif
    if (VerifyGC >= 2) {
      verify();
    }
  }

  static int free_memory_for_compiler_without_gc( void ) {
    GUARANTEE(_compiler_area_temp_bottom != NULL, 
              "temp objects must be allocated after new compiled code has "
              "been allocated");
    return DISTANCE( _compiler_area_temp_bottom, _compiler_area_temp_top);
  }
#endif



#if USE_SET_HEAP_LIMIT || USE_LARGE_OBJECT_AREA
  static void shrink_with_compiler_area( const int size );
#endif

  static OopDesc** compiler_area_end (void) {
    return mark_area_end();
  }

#if ENABLE_COMPILER
  static int compiler_area_tail (OopDesc** start) {
    return DISTANCE(start, compiler_area_end());
  }
  static int compiler_area_size (void) {
    return compiler_area_tail(_compiler_area_start);
  }
  static int compiler_area_free (void) {
    return compiler_area_tail(_compiler_area_top);
  }
  static int compiler_area_used (void) {
    return DISTANCE(_compiler_area_start, _compiler_area_top);
  }
  static OopDesc* allocate_code( const int size JVM_TRAPS ) {
    return compiler_area_allocate_code(size JVM_NO_CHECK);
  }
  static OopDesc* allocate_temp( const int size JVM_TRAPS ) {
    return compiler_area_allocate_temp(size JVM_NO_CHECK);
  }
#endif

  // Initialization
  static void initialize( void ) {  // initialize data structures
#if USE_IMAGE_PRELOADING
    _preallocated_space_size = 0;
#endif
  }
  static bool create( void );       // allocate heap space

#if USE_IMAGE_PRELOADING
  static void set_preallocated_space_size(size_t size) {
    _preallocated_space_size = size;
  }
  static address get_preallocated_space() {
    return _preallocated_space;
  }
#endif

  static size_t update_slices_size(size_t object_heap_size);
  static bool adjust_heap_size(size_t target_heap_size);
  static void recalc_slices_for_binary(void) {
    adjust_heap_size(_heap_size);
  }
  static void set_collection_area_boundary(size_t min_free_after_collection,
                                           bool reuse_previous_young_space);
  static void set_collection_area_boundary_reuse();
  static void set_collection_area_boundary_no_reuse(
                                           size_t min_free_after_collection);
  static void rom_init_heap_bounds(OopDesc **init_heap_bound, 
                                   OopDesc **permanent_top);
  // Iteration
#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
  static void iterate(ObjectHeapVisitor* visitor);
  static void iterate(ObjectHeapVisitor* visitor, OopDesc** from, OopDesc** to);
#else
  static void iterate(ObjectHeapVisitor*) PRODUCT_RETURN;
#endif

  // Printing
  struct ReachLink {
    OopDesc *node;
    int parent;
  };

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  static void print(Stream* = tty);
  static void print_all_objects(Stream* = tty);
  static void print_all_objects(const JvmPathChar* /*file*/);
  static void print_all_classes();
  static void print_task_usage(Stream* = tty);
  static void dump_histogram_data();
  static void save_java_stack_snapshot();
  static void find(OopDesc* /*target*/, bool /*is_verbose*/);
  static void reach(OopDesc* /*target*/);
  static bool check_reach_root(OopDesc* /*n*/, ReachLink* /*stack*/,
                               int /*parent*/);
  static bool reach_seen(OopDesc* /*n*/, ReachLink* /*stack*/,
                         int /*endstack*/);
#else
  static void print(Stream* = tty) PRODUCT_RETURN;
  static void print_all_objects(Stream* = tty) PRODUCT_RETURN;
  static void print_all_objects(const JvmPathChar* /*file*/) PRODUCT_RETURN;
  static void print_all_classes() PRODUCT_RETURN;
  static void print_task_usage(Stream* = tty) PRODUCT_RETURN;
  static void dump_histogram_data() PRODUCT_RETURN;
  static void save_java_stack_snapshot() PRODUCT_RETURN;
  static void find(OopDesc* /*target*/, bool /*is_verbose*/) PRODUCT_RETURN;
  static void reach(OopDesc* /*target*/) PRODUCT_RETURN;
  static bool check_reach_root(OopDesc* /*n*/, ReachLink* /*stack*/,
                               int /*parent*/) PRODUCT_RETURN0;
  static bool reach_seen(OopDesc* /*n*/, ReachLink* /*stack*/,
                         int /*endstack*/) PRODUCT_RETURN0;
#endif

  // Counting
  static int count_objects();
  static int code_size_summary();
  static int code_item_summary();

  // Testing
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  static bool contains_live(OopDesc** /*target*/);
#else
  static bool contains_live(OopDesc** /*target*/) { return false; }
#endif
  static bool contains_live(const OopDesc* target) {
    return contains_live((OopDesc**) target);
  }
  static bool contains(OopDesc** target) {
    return _heap_start <= target && target < mark_area_end();
  }
  static bool contains(const OopDesc* target)  {
      return contains((OopDesc**) target);
  }
  static bool in_collection_area(OopDesc** target) {
    return _debugger_active && _collection_area_start <= target
#if ENABLE_COMPILER
      && target < compiler_area_end()
#endif
      ;
  }
  static bool in_collection_area_unmarked( OopDesc** obj ) {
    return _collection_area_start <= obj && obj < mark_area_end()
      && !test_bit_for( obj );
  }
  static bool in_collection_area_unmarked( const OopDesc* obj ) {
    return in_collection_area_unmarked( (OopDesc**) obj );
  }

  static bool permanent_contains(OopDesc** /*target*/) PRODUCT_RETURN0;

  static bool contains_moveable(const OopDesc* target) {
    return _heap_start <= ((OopDesc**)target)&&((OopDesc**)target)<_heap_top;
  }

  // Bitvector access
  static inline bool test_bit_for(OopDesc** p, 
                                  address bitvector_base = _bitvector_base);
  static inline void set_bit_for(OopDesc** p,
                                  address bitvector_base = _bitvector_base);
  static inline void clear_bit_for(OopDesc** p, 
                                  address bitvector_base = _bitvector_base);
  static inline bool test_and_set_bit_for(OopDesc** p, 
                                  address bitvector_base = _bitvector_base);
  static inline void set_bit_range(OopDesc** start, int len);
  static void clear_bit_range(OopDesc** start, OopDesc** exclusive_end);
  static void do_nothing(OopDesc**);

  // Debugging support
  static OopDesc* slow_object_start(OopDesc** /*target*/) PRODUCT_RETURN0;
  static void verify_near_oop(OopDesc** /*p*/) PRODUCT_RETURN;
  static void verify_other_oop(OopDesc** /*p*/) PRODUCT_RETURN;
  static void verify_only_permanent_pointers(OopDesc** /*p*/) PRODUCT_RETURN;
  static void verify() PRODUCT_RETURN;
  static void verify_bitvector_range(OopDesc** /*verify_start*/) 
                                     PRODUCT_RETURN;
  static void verify_bitvector_alignment(OopDesc** /*p*/) PRODUCT_RETURN;
  static void verify_layout() PRODUCT_RETURN;

#ifdef AZZERT
  static void nuke_raw_handles();
  static bool is_finalizing(void) { return _is_finalizing; }
#endif

  static bool is_gc_active(void) { return _is_gc_active; }
  static void force_full_collect(void);
  static bool expand_current_compiled_method(int delta);

#if ENABLE_ISOLATES && (USE_IMAGE_MAPPING || USE_LARGE_OBJECT_AREA)
#if ENABLE_LIB_IMAGES
  static bool in_dead_bundles ( const OopDesc* obj );
#else
  static bool in_dead_bundles ( const OopDesc* obj ) {
    return obj < dead_range_end && obj >= dead_range_beg;
  }
#endif
private:  
#if ENABLE_LIB_IMAGES
  static const OopDesc* _dead_task_images;    
#else
  static const OopDesc* dead_range_beg;
  static const OopDesc* dead_range_end;    // exclusive
#endif //ENABLE_LIB_IMAGES
  
  static OopDesc* dead_task;
#else
  static bool in_dead_bundles ( const OopDesc* /*obj*/ ) { return false; }
#endif

#if ENABLE_JVMPI_PROFILE_VERIFY && ENABLE_JVMPI_PROFILE
  // Dump the Compiled method info to log files for the JVMPI verification.
  static void ObjectHeap::dumpAllCompiledMethodsInfo();
#endif

 private:
  // Bitvector access implementation
  static unsigned oop_index (OopDesc** p) {
    GUARANTEE(contains(p), "Should be in object heap");
    return unsigned(p) >> LogBytesPerWord;
  }

  static unsigned bitvector_word_index  (const unsigned i) {
    return i >> LogBitsPerWord;
  }

  static unsigned bitvector_bit_index   (const unsigned i) {
    return i & (BitsPerWord-1);
  }

  static unsigned bitvector_bit_mask    (const unsigned i) {
    return 1 << bitvector_bit_index(i);
  }

  static unsigned& bitvector_word( const unsigned i,
                                   address bitvector_base = _bitvector_base) {
    return ((unsigned*)bitvector_base)[ bitvector_word_index( i ) ];
  }

#if ENABLE_ISOLATES
  static int      _current_task_id;          // Current resource owner
  static int      _previous_task_id;         // Previous task that allocated memory

  static OopDesc**_real_inline_allocation_end;
  static OopDesc**_task_allocation_start;

  static unsigned _reserved_memory_deficit;
  static unsigned _current_deficit;
  static bool     _some_tasks_terminated;

  static TaskMemoryInfo _task_info [MAX_TASKS];

 public:
  static TaskMemoryInfo& get_task_info ( const int task_id ) {
    GUARANTEE( unsigned(task_id) < unsigned(MAX_TASKS), "Invalid task id" );
    return _task_info[ task_id ];
  }
 
  static OopDesc** current_task_allocation_end ( void );

 private:
  static OopDesc**      get_boundary_classes( void );
  static BoundaryDesc** get_boundary_list   ( void );
  static int            get_current_task    ( void );

  // Pointer to classes is cached in a temporary variable
  static int get_owner( const BoundaryDesc* p, const OopDesc* const classes[] );

  static void create_boundary( OopDesc** p, const int task );
  static void accumulate_memory_usage( OopDesc* lwb[], OopDesc* upb[] );
  static void set_task_memory_reserve_limit(const int task,
                const unsigned reserve, const unsigned limit) {
    TaskMemoryInfo& task_info = get_task_info( task );
    task_info.limit = limit;
    task_info.reserve = reserve;
    _reserved_memory_deficit += reserve;
  }

  enum { OverReservation = 1, OverLimit = 2 };
  static unsigned detect_out_of_memory_tasks( const size_t /*alloc_size*/ );
#endif

  static inline void set_task_allocation_start( OopDesc** p );
  static inline void set_inline_allocation_end( OopDesc** p ) {
    _inline_allocation_end = p;
#if ENABLE_ISOLATES
    _real_inline_allocation_end = p;
#endif
  }

  // Marking support
  static void roots_do_to( void do_oop(OopDesc**), const bool young_only,
                                                                const int upb );
  static void roots_do( void do_oop(OopDesc**), const bool young_only = false ){
#if ENABLE_COMPILER
    const int upb = CompiledMethodCache::upb;
#else
    const int upb = -1;
#endif
    roots_do_to( do_oop, young_only, upb );
  }

  static void setup_marking_stack(void);
  static void mark_remembered_set(void);
  static juint mark_and_stack_pointers(OopDesc** p, juint bitword);
  static void mark_pointer_to_young_generation  (OopDesc** p);
  static void mark_and_push(OopDesc** p);
  static void mark_and_push_compiled_method(CompiledMethodDesc *cm);
  static void mark_root_and_stack(OopDesc** p);
  static void mark_and_stack_root_and_interior_pointers(OopDesc** p);
  static void continue_marking(void);
  static void check_marking_stack_overflow(void);

#if ENABLE_ISOLATES && ENABLE_COMPILER
  static void cleanup_compiled_method_cache( void );
#endif

  // Pointer updating
  static void update_other_interior_pointers( const bool is_full_collect );
  static void update_execution_stack_interior_pointers();
  static void update_interior_pointer(OopDesc** p);
  static void update_interior_pointer_delimited(OopDesc** p);
  static void mark_forward_pointer(OopDesc** p);
  static OopDesc** mark_forward_pointers();

  static void update_moving_object_interior_pointers(OopDesc** p);
  static void update_moving_object_near_pointer(OopDesc** p);

  // Four main GC phases
  static void mark_objects( const bool is_full_collect );
  static void compute_new_object_locations();
  static void update_object_pointers();
  static void compact_objects(bool reuse_young_generation);

  // Near and forwarding pointer encoding/decoding support
  static OopDesc* decode_near(OopDesc* obj, const QuickVars& qv = _quick_vars);
  inline static size_t   rom_offset_of(OopDesc* obj);
  inline static OopDesc* rom_oop_from_offset(size_t offset,
                                             const QuickVars& qv);
  inline static OopDesc* rom_oop_from_offset(size_t offset);
  inline static OopDesc* decode_near(OopDesc* obj, OopDesc **heap_start, 
                                     size_t near_mask);
  inline static FarClassDesc* decode_far_class(const OopDesc* obj);
  inline static FarClassDesc* decode_far_class_with_real_near(OopDesc* obj);
  inline static FarClassDesc* decode_far_class_with_encoded_near(OopDesc* obj,
                                             const QuickVars& qv = _quick_vars);
  inline static OopDesc* decode_destination(OopDesc* obj,
                                             const QuickVars& qv = _quick_vars);

  // Global reference support
  static void global_refs_do(void do_oop(OopDesc**), const int mask);

  // Finalization support
  static void discover_finalizer_reachable_objects();

  // Bitvector support

  static OopDesc** align_down ( OopDesc** p ) {
    return (OopDesc**) align_size_down((size_t)p, alignment);
  }
  static unsigned align_down ( const unsigned n ) {
    return align_size_down(n, alignment);
  }
  static OopDesc** align_up ( OopDesc** p ) {
    return (OopDesc**) align_size_up((size_t)p, alignment);
  }
  static unsigned align_up ( const unsigned n ) {
    return align_size_up(n, alignment);
  }
  static void must_be_aligned( const unsigned n ) {
    (void)n;
    GUARANTEE_R( n % alignment == 0, "must be aligned" );    
  }
public:
  static juint* get_bitvectorword_for_aligned(OopDesc** p) {
    AZZERT_ONLY(verify_bitvector_alignment(p));
    return DERIVED(juint*, _bitvector_base,
                     ((uintptr_t)p) >> (LogBytesPerWord+LogBitsPerByte));
  }
private:
  static juint* get_bitvectorword_for_unaligned(OopDesc** p) {
    return get_bitvectorword_for_aligned( align_down( p ) );
  }

  static OopDesc** get_aligned_for_bitvectorword(juint *ptr) {
    return (OopDesc**)
        (DISTANCE(_bitvector_base, ptr) << (LogBytesPerWord + LogBitsPerByte));
  }
  static void write_barrier_oops_do(void do_oop(OopDesc**),
                                    OopDesc** start, OopDesc** end);
  static void write_barrier_oops_update_interior_pointers(OopDesc** start,
                                                          OopDesc** end);
  static void write_barrier_oops_update_moving_object_interior_pointers(
                               OopDesc** start, OopDesc** end);
  static void write_barrier_oops_update_moving_object_near_pointer(
                               OopDesc** start, OopDesc** end);
  static void write_barrier_oops_unencode_moving_object_near_pointer(
                               OopDesc** start, OopDesc** end);

  static void finalizer_oops_do(FinalizerConsDesc** list, void do_oop(OopDesc**));

  // Code flushing support
  static void mark_or_flush_compiled_methods();

  // Collection
  static void try_to_shrink(int min_free_after_collection);
  static void try_to_grow(int requested_free_memory, const bool is_full_collect);
  static bool internal_collect(size_t min_free_after_collection JVM_TRAPS);
  static void internal_collect_prologue(size_t min_free_after_collection);
  static void internal_collect_epilogue(bool is_full_collect, 
                                        bool reuse_young_generation);
  // Compiler area
#if ENABLE_COMPILER
  static size_t reduce_compiler_usage(size_t requested);
  static void increase_compiler_usage(size_t min_free_after_collection);
  static bool shrink_compiled_method(CompiledMethodDesc *cm, size_t old_size,
                                     size_t new_size);
  static int count_compiled_methods() PRODUCT_RETURN0;

  static int  compiler_area_compute_new_locations ( CompiledMethodDesc* dst );
  static void compiler_area_compact               ( const int last_moving_up );
  static void compiler_area_update_pointers       ( void );

  static void compiler_area_move_compiled_method  ( 
    CompiledMethodDesc* dst, const CompiledMethodDesc* src, const int index );
#endif
  static void compact_and_move_compiler_area      ( const int delta );

  enum {
    alignment = BitsPerWord * BytesPerWord
  };

#if USE_IMAGE_PRELOADING  
  // number of bytes to reserved at the low-end of the heap for Monet image
  static size_t     _preallocated_space_size;
#else
  enum { _preallocated_space_size = 0 };
#endif

#if ENABLE_COMPILER
  static size_t     _glue_code_size;
#else
  enum { _glue_code_size = 0 };
#endif

  static address    _preallocated_space;
  static address    _glue_code;

  // Size-adjustable chunk for all heap data structures
  static address    _heap_chunk;
  static address    _bitv_chunk;

  // Static variables for heap boundaries
  static address    _bitvector_start;
  static OopDesc**  _permanent_generation_top;

  // Static variables for bit masks and sizes

  // Static variables for mark-sweep-compact
  static bool      _is_gc_active;

#ifdef AZZERT
  static bool      _is_finalizing;
#endif

  // Saving heap config during expansion
  struct SavedHeapConfig {
    size_t    _old_near_mask;
    size_t    _old_slice_shift;
    size_t    _old_slice_offset_bits;
    size_t    _old_slice_offset_mask;
    size_t    _old_slice_size;
  };
  static void save_heap_config    ( SavedHeapConfig&        state );
  static void restore_heap_config ( const SavedHeapConfig&  state );

  static bool      _last_heap_expansion_failed;

#if ENABLE_COMPILER
  static OopDesc** _saved_compiler_area_top;
#endif

#if ENABLE_INTERNAL_CODE_OPTIMIZER
public:
  //save the compiler area top before doing code scheduling
  static void save_compiler_area_top_fast( void ) {
    _saved_compiler_area_top_quick = _compiler_area_top;
  }

  //restore the compiler area top to the value stored before scheduling.
  //free the memory allocated during code scheduling quickly
  static void update_compiler_area_top_fast( void ) {
    _compiler_area_top = _saved_compiler_area_top_quick;
  }
private:
  static OopDesc** _saved_compiler_area_top_quick;
#endif

#if ENABLE_PERFORMANCE_COUNTERS || ENABLE_TTY_TRACE
  static jlong  _internal_collect_start_time;
  static size_t _old_gen_size_before;
  static size_t _young_gen_size_before;
#endif

#ifndef PRODUCT
  static OopDesc** _heap_start_bitvector_verify;
  static int       _excessive_gc_countdown;

  friend class GCDisabler;
  friend class AllocationDisabler;
#endif

#if ENABLE_ISOLATES
  friend class AllocationTrapDisabler;
#endif

  friend class LargeObject;
  friend class Universe;
  friend void oop_write_barrier_range(OopDesc** start, int len);
  friend void garbageCollect(int moreMemory);
#if ENABLE_TRAMPOLINE  && !CROSS_GENERATOR
  friend class BranchTable;
#endif
};

// Object heap iteration

class ObjectHeapVisitor : public StackObj {
 public:
  virtual void do_obj(Oop* /*obj*/) JVM_PURE_VIRTUAL;
};

// Write barrier for object array copy
void oop_write_barrier_range(OopDesc** start, int len);

#ifndef PRODUCT

extern jint AllocationDisabler__disabling_count;
extern bool AllocationDisabler__suspended; // Use with care!
extern jint GCDisabler__disabling_count;

class GCDisabler {
private:
  friend class ObjectHeap;
public:
  GCDisabler()  {
    GCDisabler__disabling_count++;
  }
  ~GCDisabler() {
    GUARANTEE(GCDisabler__disabling_count > 0, "unbalanced GC disabling");
    GCDisabler__disabling_count--;
  }

  static bool gc_okay() {
    return GCDisabler__disabling_count == 0;
  }
};

class AllocationDisabler {
private:
  friend class ObjectHeap;
  static OopDesc** _current_allocation_top;
public:
  AllocationDisabler()  {
    if (AllocationDisabler__disabling_count == 0) {
      _current_allocation_top = _inline_allocation_top;
    } else {
      GUARANTEE(_current_allocation_top == _inline_allocation_top,
                "allocation happened while AllocationDisabler is active");
    }
    AllocationDisabler__disabling_count++;
  }
  ~AllocationDisabler() {
    GUARANTEE(AllocationDisabler__disabling_count > 0,
              "unbalanced Allocation disabling");
    GUARANTEE(_inline_allocation_top == _current_allocation_top,
              "Someone has allocated something");
    AllocationDisabler__disabling_count--;
  }
  static bool allocation_okay() {
    return  AllocationDisabler__suspended ||
           (AllocationDisabler__disabling_count == 0);
  }
  static void suspend() {
    AllocationDisabler__suspended = true;
  }
  static void resume() {
    _current_allocation_top = _inline_allocation_top;
    AllocationDisabler__suspended = false;
  }
};

class ExcessiveGCBooster {
public:
  ExcessiveGCBooster(bool is_boost_requested) {
    old_value = ExcessiveGC;
    if (is_boost_requested) {
      ExcessiveGC = true;
    }
  }
  ~ExcessiveGCBooster() {
    ExcessiveGC = old_value;
  }
private:
  bool old_value;
};

#else

class GCDisabler {
public:
  GCDisabler()  { }
  static bool gc_okay() { return true; }
};

class AllocationDisabler {
public:
  AllocationDisabler() {}
  static bool allocation_okay() { return true; }
  inline static void suspend() {}
  inline static void resume()  {}
};

class ExcessiveGCBooster {
public:
  ExcessiveGCBooster(bool /*is_boost_requested*/) { }
};

#endif

#if USE_GENERIC_BIT_SETTING_FUNCS
/*
 * Currently only x86 port uses an alternative set of bit-setting functions.
 */
inline unsigned int* word_for(unsigned int offset, unsigned int* map) { 
  return map + (offset/BitsPerWord); 
}

inline void bit_clear(unsigned int offset, unsigned int* map) {
  intptr_t* w = (intptr_t*)word_for(offset, map);
  int pos = offset%BitsPerWord;
  clear_nth_bit(*w, pos);
}

inline void bit_set(unsigned int offset, unsigned int* map) {
  intptr_t* w = (intptr_t*)word_for(offset, map);
  int pos = offset%BitsPerWord;
  set_nth_bit(*w, pos);
}

inline bool is_bit_set(unsigned int offset, unsigned int* map) {
  return is_set_nth_bit(*word_for(offset, map), offset%BitsPerWord); 
}

inline void ObjectHeap::set_bit_for(OopDesc** p, address bitvector_base) {
  const unsigned i = oop_index(p);
  bitvector_word( i, bitvector_base ) |= bitvector_bit_mask( i );
}

inline void ObjectHeap::clear_bit_for(OopDesc** p, address bitvector_base) {
  const unsigned i = oop_index(p);
  bitvector_word( i, bitvector_base ) &=~bitvector_bit_mask( i );
}

inline bool ObjectHeap::test_bit_for(OopDesc** p, address bitvector_base) {
  const unsigned i = oop_index(p);
  return (bitvector_word( i, bitvector_base ) & bitvector_bit_mask( i )) != 0;
}

inline bool ObjectHeap::test_and_set_bit_for(OopDesc** p,
                                             address bitvector_base) {
  const unsigned i = oop_index(p);
  const unsigned bitword = bitvector_word( i, bitvector_base );
  const unsigned mask = bitvector_bit_mask( i );
  bitvector_word( i, bitvector_base ) = bitword | mask;
  return (bitword & mask) != 0;
}

// Consider using word stores here, depends on the average length
inline void ObjectHeap::set_bit_range(OopDesc** start, int len) {
  unsigned int t = oop_index(start);
  unsigned int* const bitvector_base = (unsigned int*)_bitvector_base;
  for (int i = 0; i < len; i++, t++) {
    bit_set(t, bitvector_base);
  }
}

#endif /* USE_GENERIC_BIT_SETTING_FUNCS */
