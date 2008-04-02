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

#if ENABLE_COMPILER

// The compiler translates bytecodes into native instructions.

#define COMPILER_CONTEXT_HANDLES FIELD( Method, method )

class CompilerContextPointers {
  #define FIELD( type, name ) \
    type##Desc* _##name;    \
    type* name ( void ) const           { return (type*) &_##name;     }  \
    void clear_##name ( void )          { _##name = NULL;              }  \
    void set_##name ( OopDesc* val )    { _##name = (type##Desc*) val; }  \
    void set_##name ( const type* val ) { set_##name( val->obj() );    }

public:
  COMPILER_CONTEXT_HANDLES
  #undef FIELD

  static int pointer_count( void ) {
    return sizeof(CompilerContextPointers) / sizeof(OopDesc*);
  }
};

#define GENERIC_COMPILER_CONTEXT_FIELDS_DO(template)\
  template( CompilationQueueElement*, compilation_queue              )\
  template( CompilationQueueElement*, current_element                )\
  template( EntryTableType*,          entry_table                    )\
  template( Compiler*,                parent                         )\
  template( CompilerByteArray*,       entry_counts_table             )\
  template( CompilerByteArray*,       bci_flags_table                )\
  template( int,                      saved_bci                      )\
  template( int,                      saved_num_stack_lock_words     )\
  template( int,                      local_base                     )\
  template( bool,                     in_loop                        )\
  template( bool,                     has_loops                      )

#if ENABLE_INLINE
#define INLINER_COMPILER_CONTEXT_FIELDS_DO(template)  \
  template( int, inline_return_label_encoding )
#else
#define INLINER_COMPILER_CONTEXT_FIELDS_DO(template)
#endif

#if ENABLE_CODE_OPTIMIZER && ENABLE_NPCE
#define SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) \
  template( CompilerIntArray*,  null_point_exception_ins_table       )\
  template( int,                codes_can_throw_null_point_exception )\
  template( int,                null_point_record_counter            )
#else
#define SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) 
#endif

#define COMPILER_CONTEXT_FIELDS_DO(template) \
        GENERIC_COMPILER_CONTEXT_FIELDS_DO(template) \
        INLINER_COMPILER_CONTEXT_FIELDS_DO(template) \
        SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) 

class CompilerContext: public CompilerContextPointers {
public:
  typedef EntryArray EntryTableType;

  #define FIELD( type, name ) \
    type _##name;             \
    type name         ( void ) const { return _##name;  }\
    void set_##name   ( type val )   { _##name = val;   }\
    void clear_##name ( void )       { set_##name( 0 ); }

  COMPILER_CONTEXT_FIELDS_DO(FIELD)
  #undef FIELD

  bool valid ( void ) const { return method()->not_null(); }

  void oops_do( void do_oop(OopDesc**) );
  void cleanup( void );

#if USE_DEBUG_PRINTING
  void print_on(Stream *st);
#endif
};

#define COMPILER_STATIC_FIELDS_DO(template)\
  template( VirtualStackFrame*, frame                   )\
  template( VirtualStackFrame*, conforming_frame        )\
  template( VirtualStackFrame*, cached_preserved_frame  )\
  template( Compiler*,          root                    )\
  template( Compiler*,          current                 )

class CompilerStatic {
 public:
  #define DECLARE_FIELD( type, name ) type _##name;
  COMPILER_STATIC_FIELDS_DO(DECLARE_FIELD)

  ThrowExceptionStub* _rte_handlers[ThrowExceptionStub::number_of_runtime_exceptions];

#if USE_DEBUG_PRINTING
  void print_on(Stream *st);
#endif
  void cleanup( void );
};

class Compiler: public StackObj {
 private:
  // The compiler state.
  static CompilerStatic _state;

  // Info used to tune down compilation for smoother animation.
  static jlong          _estimated_frame_time;
  static jlong          _last_frame_time_stamp;

 public:
  typedef CompilerContext::EntryTableType EntryTableType;

  #define DEFINE_ACCESSOR( type, name ) \
    static type name       ( void )          { return _state._##name;    } \
    static void set_##name ( type name )     { _state._##name = name;    }
  COMPILER_STATIC_FIELDS_DO(DEFINE_ACCESSOR)
  #undef DEFINE_ACCESSOR

  #define DEFINE_ACCESSOR( type, name ) \
    type name                  ( void ) const { return _context.name();   } \
    void set_##name            ( type name )  { _context.set_##name(name);} \
    void clear_##name          ( void )       { _context.clear_##name();  } \
    static type current_##name ( void )       { return current()->name(); }
  COMPILER_CONTEXT_FIELDS_DO(DEFINE_ACCESSOR)
  #undef DEFINE_ACCESSOR

  #define FIELD( type, name ) \
    type* name       ( void ) const      { return _context.name();     } \
    void clear_##name( void )            { _context.clear_##name();    } \
    void set_##name  ( OopDesc* val )    { _context.set_##name( val ); } \
    void set_##name  ( const type* val ) { _context.set_##name( val ); } \
    static type* current_##name ( void ) { return current()->name();   }
  COMPILER_CONTEXT_HANDLES
  #undef FIELD

  // Constructor and deconstructor.
  Compiler( Method* method, const int active_bci );

#ifndef PRODUCT
  // Create a dummy compiler that does nothing, but just make is_active()
  // to true.
  Compiler();
#endif
  ~Compiler();

  // Called during VM start-up
  static void initialize( void ) {
    jvm_memset(&_state, 0, sizeof _state);
    jvm_memset(&_suspended_compiler_context, 0, 
               sizeof _suspended_compiler_context);
#if ENABLE_PERFORMANCE_COUNTERS && ENABLE_DETAILED_PERFORMANCE_COUNTERS
    jvm_memset(&comp_perf_counts, 0, sizeof comp_perf_counts );
#endif
    _estimated_frame_time = 30;
    _last_frame_time_stamp = Os::java_time_millis();
  }

  // Compiles the method and returns the result.
  // ^CompiledMethod
  static ReturnOop compile(Method* method, int active_bci JVM_TRAPS);

  // Resume a compilation that has been suspended.
  static ReturnOop resume_compilation(Method* method JVM_TRAPS);

  // Abort current suspended compilation.
  static void abort_suspended_compilation( void );

  // Abort the current active compilation. This method must be called
  // when compilation is actually taking place. It's usually used
  // during the development of the compiler to stop compilation when
  // an unimplemented feature is used.
  static void abort_active_compilation(bool is_permanent JVM_TRAPS);

  static CodeGenerator* code_generator( void ) {
    return _compiler_code_generator;
  }

  static CompiledMethod* current_compiled_method( void ) {
    return code_generator()->compiled_method();
  }

  static BytecodeCompileClosure* closure( void ) {
    return _compiler_closure;
  }

  static VirtualStackFrame* get_cached_preserved_frame( void ) {
    VirtualStackFrame* p = cached_preserved_frame();
    set_cached_preserved_frame( NULL );
    return p;
  }

  // Accessors for the compilation queue.
  CompilationQueueElement* current_compilation_queue_element( void ) {
    CompilationQueueElement* p = current_element();
    if( !p ) {
      p = compilation_queue();
      set_compilation_queue( p->next() );
      set_current_element( p );
    }
    return p;
  }

  CompilerContext* context( void ) {
    return &_context;
  }

#if ENABLE_NPCE
  //get the exception stub whose's entry label is still unset.
  //the function will find the stub and let the store_to_add_xx() to fill the 
  //address in LDR instr  in it.
  NullCheckStub* get_unlinked_exception_stub(const jint bci) const {
    CompilationQueueElement* next_element;
    //Try to found the unlinked null exception stub.
    for( next_element = compilation_queue();
      next_element &&
      !(next_element->bci() == bci && 
        next_element->type() == CompilationQueueElement::throw_exception_stub &&
        ((ThrowExceptionStub*) next_element)->get_rte() == ThrowExceptionStub::rte_null_pointer);
      next_element = next_element->next() ) {}
    return (NullCheckStub*) next_element;
  }

#if ENABLE_INTERNAL_CODE_OPTIMIZER
  //Null pointer exception accessor
  void record_null_point_exception_inst( const int offset ) {
    const int index = null_point_record_counter();
    null_point_exception_ins_table()->at_put( index, offset ); 
    set_null_point_record_counter( index+1 );     
  }

  //return the null point related instr indexed by index parameter
  int null_point_exception_abort_point( const int index) const {
    return null_point_exception_ins_table()->at( index);
  }

  //record the instr offset of those null point stubs into a npe_table.
  //we will track the offset changing of those instr during scheduling and
  //update the entry label of those instr after scheduling.
  void record_instr_offset_of_null_point_stubs(int start_offset);

  //update the entry lable of those null point stubs base on the information
  //of npe_table after scheduling
  void update_null_check_stubs();
#endif // ENABLE_INTERNAL_CODE_OPTIMIZER 
#endif // ENABLE_NPCE 

#if ENABLE_LOOP_OPTIMIZATION && ARM
  //get the first compilation queue item.
  CompilationQueueElement* get_first_compilation_queue_element( void ) const {
    return compilation_queue();
  }
  
  //get the next element in the current compilation queue. 
  static CompilationQueueElement*
  get_next_compilation_queue_element(CompilationQueueElement* p) {
    return p->next();
  }
  static const CompilationQueueElement*
  get_next_compilation_queue_element(const CompilationQueueElement* p) {
    return p->next();
  }
#endif //#if ENABLE_LOOP_OPTIMIZATION && ARM

  void insert_compilation_queue_element(CompilationQueueElement* value) {
    value->set_next( compilation_queue() );
    set_compilation_queue( value );
  }

  bool is_compilation_queue_empty( void ) const {
    return compilation_queue() == NULL && current_element() == NULL;
  }

  // Entry counts accessor.
  jubyte entry_count_for(const jint bci) const {
    return entry_counts_table()->at(bci);
  }

  bool exception_has_osr_entry(const jint bci) const {
    return (bci_flags_table()->at(bci) &
            Method::bci_exception_has_osr_entry) != 0;
  }

  void set_exception_has_osr_entry(const jint bci) const {
    bci_flags_table()->at(bci) |= Method::bci_exception_has_osr_entry;
  }

  bool is_branch_taken(const jint bci) const {
    return (bci_flags_table()->at(bci) & Method::bci_branch_taken) != 0;
  }

  void set_branch_taken(const jint bci) const {
    bci_flags_table()->at(bci) |= Method::bci_branch_taken;
  }

  // Entry accessor.
  Entry* entry_for(const jint bci) const {
    return entry_table()->at( bci );
  }
  void set_entry_for(const jint bci, Entry* entry) {
    entry_table()->at_put( bci, entry );
  }

  bool method_aborted_for_exception_at(const int bci) {
    const AccessFlags flags = method()->access_flags();
    return !( flags.is_synchronized()
           || flags.has_monitor_bytecodes()
           || method()->exception_handler_exists_for(bci)
           || _debugger_active );
  }

  // Tells whether is compiler is active
  static bool is_active( void ) { return current() != NULL; }

  // Support for sharing of exception thrower stubs.
  typedef ThrowExceptionStub::RuntimeException RuntimeException;
  static ThrowExceptionStub* rte_handler(const RuntimeException rte) {
    return _state._rte_handlers[rte];
  }
  static void set_rte_handler(const RuntimeException rte, ThrowExceptionStub* value) {
    _state._rte_handlers[rte] = value;
  }
#if ENABLE_INLINE
  void internal_compile_inlined( Method::Attributes& attributes JVM_TRAPS );

  BinaryAssembler::Label inline_return_label( void ) const {
    BinaryAssembler::Label label;
    label._encoding = inline_return_label_encoding();
    return label;
  }

  void set_inline_return_label(const BinaryAssembler::Label label) {
    set_inline_return_label_encoding(label._encoding );
  }

 private:
  VirtualStackFrame* parent_frame( void ) const {
    GUARANTEE(is_inlining(), "Can only be called during inlining");
    const Compiler* parent_compiler = parent();
    GUARANTEE(parent_compiler != NULL, "Cannot be null when inlining");
    GUARANTEE(parent_compiler != this, "Sanity");
    const CompilationQueueElement* parent_element = 
      parent_compiler->current_element();
    GUARANTEE(parent_element, "Cannot be null when inlining");
    return parent_element->frame();
  }

  void set_parent_frame( VirtualStackFrame* frame ) {
    GUARANTEE(is_inlining(), "Can only be called during inlining");
    const Compiler* parent_compiler = parent();
    GUARANTEE(parent_compiler != NULL, "Cannot be null when inlining");
    GUARANTEE(parent_compiler != this, "Sanity");
    CompilationQueueElement* parent_element = 
      parent_compiler->current_element();
    GUARANTEE(parent_element, "Cannot be null when inlining");
    parent_element->set_frame( frame );
  }

  void clear_parent_frame( void ) {
    set_parent_frame( NULL );
  }
 public:
#endif
  static int bci( void ) {
    return _compiler_bci;
  }
  static void set_bci( int bci ) {
    _compiler_bci = bci;
  }

  static int  num_stack_lock_words(void) {
    return _num_stack_lock_words;
  }
  static void set_num_stack_lock_words(int num_lock_words) {
    _num_stack_lock_words = num_lock_words;
  }

  static bool is_in_loop           ( void ) { 
    return Compiler::current()->in_loop();     
  }
  static void mark_as_in_loop      ( void ) { 
    Compiler::current()->set_in_loop( true );  
  }
  static void mark_as_outside_loop ( void ) { 
    Compiler::current()->set_in_loop( false ); 
  }

  static bool is_inlining( void ) { 
    return current() != root();
  }

  int compiler_bci( void ) const {
    return current() == this ? bci() : saved_bci();
  }

  enum CompilationFailure {
    none,
    reservation_failed,
    out_of_time,
    out_of_memory,
    out_of_stack
  };

  static CompilationFailure _failure;
  static void print_compilation_history() PRODUCT_RETURN;

 private:
  static CompilerContext _suspended_compiler_context;
 public:
  static void on_timer_tick(bool is_real_time_tick JVM_TRAPS);
  static void process_interpretation_log();

  static void set_hint(const int hint) {
    switch (hint) {
    case JVM_HINT_VISUAL_OUTPUT:
      _estimated_frame_time = 300;
      _last_frame_time_stamp = Os::java_time_millis();
      break;
    case JVM_HINT_END_STARTUP_PHASE:
      break;
    }
  }

  static bool is_time_to_suspend( void ) {
    return !is_inlining() &&
           (ExcessiveSuspendCompilation || Os::check_compiler_timer());
  }
  static CompilerContext* suspended_compiler_context( void ) {
    return &_suspended_compiler_context;
  }
  static bool is_suspended ( void ) {
    return _compiler_code_generator != NULL;
  }

  static void oops_do( void do_oop(OopDesc**) );

#if ENABLE_PERFORMANCE_COUNTERS && ENABLE_DETAILED_PERFORMANCE_COUNTERS
  static void print_detailed_performance_counters();
#else 
  static void print_detailed_performance_counters() {}
#endif
 private:
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  InternalCodeOptimizer _internal_code_optimizer;
  FastOopInStackObj    __must_be_first_item__;
#endif

  // The compiler closure.
  BytecodeCompileClosure _closure;

  // The compiler context.
  CompilerContext        _context;

#if ENABLE_INTERNAL_CODE_OPTIMIZER && ARM &&ENABLE_CODE_OPTIMIZER
  CompilationQueueElement* _next_element;
  CompilationQueueElement* _cur_element;
  LiteralPoolElement*      _next_bound_literal;

  InternalCodeOptimizer* optimizer( void ) {
    return &_internal_code_optimizer;
  }

  void prepare_for_scheduling_of_current_cc(CompiledMethod* cm) {
#if ENABLE_NPCE
    //reset counter before compilation of cc.
    set_null_point_record_counter(0);
#endif
    _internal_code_optimizer.prepare_for_scheduling_of_current_cc(
              cm, code_generator()->code_size());
  }
  
  bool  schedule_current_cc(CompiledMethod* cm JVM_TRAPS) {
#if ENABLE_NPCE
    //should be GUARANTEE(Compiler::current()->null_point_record_counter() <= 
    //                     Compiler::null_point_exception_ins_table()->length(), "there're more npe related ins");
    //if the table is smaller than the real number of npe ins, we don't scheduling code
    //since we have no enough information to maintain the npe relationship during scheduling.
    //null_point_record_counter is the real number of npe ins appeared in current cc.
    if( null_point_record_counter() > codes_can_throw_null_point_exception() ) {
      return false;
    }             
#endif
      
    return _internal_code_optimizer.schedule_current_cc(
      cm, code_generator()->code_size() JVM_NO_CHECK_AT_BOTTOM);
  }

  //get the number of unbound literal count
  //if the count>0, scheduler should track the 
  //literal access instruction during scheduling
  int get_unbound_literal_count() {
    return this->code_generator()->unbound_literal_count();
  }

  //get the next branch instruction of the chain 
  //pointed to the same unbind index check stub
  int next_schedulable_branch(int* begin, 
      int& next, BinaryAssembler::Label* label) {

    if (label->is_unused()) {
      //only entered for the first call of Compiler::next_scheduable_branch()
      //in a loop.
              
      CompilationQueueElement* stub = this->rte_handler(
        ThrowExceptionStub::rte_array_index_out_of_bounds);

      // There's no array boundary checking code in current emitted code.       
      if( stub == NULL ) {
        return BinaryAssembler::stop_searching;
      }

      (*label) = stub->entry_label();

      // Stub is emitted. is_unused should be replace by assertion.
      if ( label->is_unused() || label->is_bound()) {
        return BinaryAssembler::stop_searching;
      }
    }

    // Stub isn't emitted.     we are going to find each branch
    // in the chain.
    return code_generator()->next_schedulable_branch(
      *label, (address)begin, next);
 
  }

  //get the offset of bound index check stub
  int index_check_stub_offset() {
    CompilationQueueElement* stub = 
      this->rte_handler(ThrowExceptionStub::rte_array_index_out_of_bounds);
    if (!stub || !stub->entry_label().is_bound()) {
     return -1;
   }
    return stub->entry_label().position();
  }

  //for the ldr ins accessing  the same literal, we find out the first one, if the chain 
  //start from current CC. Otherwise, we  
  //return the last literal access LDR of previous CC
  //if the chain starts from there.
  //please refer: 
  // "Figure.3.8.2.2.2 algorithm for maintaining the literal accessing chain"
  //of optimization document.
  void get_first_literal_ldrs(int* begin_offset_of_block) {
    for( LiteralPoolElement* literal = code_generator()->_first_unbound_literal;
         literal; literal = literal->next() ) { 
      BinaryAssembler::Label label = literal->label();

      const int offset = code_generator()->first_instr_of_literal_loading(
                           label, (address)begin_offset_of_block);
      //if offset is -1, means the this literal is not used 
      //in current compilation continuals
      if (offset > BinaryAssembler::literal_not_used_in_current_cc) {
        VERBOSE_SCHEDULING_AS_YOU_GO(("\t\t[%d] =>literal[%d] ", offset, index));
      //record the offset into table  
        _internal_code_optimizer.record_offset_of_unbound_literal_access_ins(offset);
      }
    }
  }

  //modify the lables of literal pool element based on the scheduling result
  //of literal access instructions
  void patch_unbound_literal_elements(int begin_offset_of_block) {
    int index = 0; 
    for( LiteralPoolElement* literal = code_generator()->_first_unbound_literal;
         literal; literal = literal->next() ) { 
      BinaryAssembler::Label tmp = literal->label();
      if( tmp.position() < begin_offset_of_block ){
          continue;
      }
      const int new_offset =
        _internal_code_optimizer.offset_of_scheduled_unbound_literal_access_ins(index); 
      if(new_offset !=0 ){
        tmp.link_to( new_offset);
        literal->set_label(tmp);
      }
      index++;   
    }
  }

  //update the entry label of a unbind index check stub.
  //the entry label point to the tail of a chain of branch.
  void update_shared_index_check_stub(const int position) const {
    CompilationQueueElement* stub =
      rte_handler(ThrowExceptionStub::rte_array_index_out_of_bounds);
    if (stub && !stub->entry_label().is_bound() ) {
      BinaryAssembler::Label label;
      label.link_to(position);
      stub->set_entry_label(label);
    }
  }

  //two methods for getting the jitted code offset recorded 
  //in uncompiled OSRStub or
  //EntryFrame.We won't schedule those instruction since 
  //other code will jump to those places later.
  void begin_pinned_entry_search( void ) {
    _next_element = compilation_queue();
  }

  BinaryAssembler::Label get_next_pinned_entry();

  //two method for getting the jittted code offset of the
  //literal who has been written
  //out. We won't unpack those place during scheduling
  void begin_bound_literal_search( void ) {
    _next_bound_literal  =  this->code_generator()->_first_literal;
  }

  BinaryAssembler::Label get_next_bound_literal( void ) {
    while( _next_bound_literal ) {
      const LiteralPoolElement* literal = _next_bound_literal;
      _next_bound_literal = _next_bound_literal->next();
      if( literal->is_bound() ) {
        return literal->label();
      }
    }
    BinaryAssembler::Label label;
    return label;
  }

  friend class CodeOptimizer;
  friend class InternalCodeOptimizer;
#endif

  friend class BytecodeCompileClosure;
  friend class CodeGenerator;
  friend class CompilationQueueElement;
  friend class OSRStub;

  bool check_if_stack_frame_may_be_omitted();
  static ReturnOop try_to_compile(Method* method, const int active_bci,
                                  const int compiled_code_factor JVM_TRAPS);

  ReturnOop allocate_and_compile( const int compiled_code_factor JVM_TRAPS );

  inline void check_free_space ( JVM_SINGLE_ARG_TRAPS ) const;
  void begin_compile           ( JVM_SINGLE_ARG_TRAPS );
  void suspend                 ( void );
  void restore_and_compile     ( JVM_SINGLE_ARG_TRAPS );
  void optimize_code           ( JVM_SINGLE_ARG_TRAPS );
  void setup_for_compile( const Method::Attributes& attributes JVM_TRAPS );

  void process_compilation_queue ( JVM_SINGLE_ARG_TRAPS );
  static void terminate ( OopDesc* result );
  bool reserve_compiler_area(size_t compiled_method_size);
  void handle_out_of_memory( void );
  static void set_impossible_to_compile(Method *method, const char why[]);

  void set_code_generator( CodeGenerator* gen ) {
    _closure.set_code_generator( gen );
  }
  void set_code_generator( void ) {
    set_code_generator( _compiler_code_generator );
  }

#if ENABLE_PERFORMANCE_COUNTERS
  void init_performance_counters(bool is_resume);
  void update_performance_counters(bool is_resume, OopDesc* result);

  jlong _start_time;
  int   _mem_before_compile;
  int   _gc_before_compile;
#else
  void init_performance_counters(bool /*is_resume*/) {}
  void update_performance_counters(bool /*is_resume*/, OopDesc* /*result*/) {}
#endif

#ifndef PRODUCT
  class CompilationHistory {
  public:
    // IMPL_NOTE: add more info such as start/stop time, etc.
    CompilationHistory *_next;
    char _method_name[256];
  };

  static CompilationHistory *_history_head;
  static CompilationHistory *_history_tail;
#endif

  void append_compilation_history() PRODUCT_RETURN;

#if USE_DEBUG_PRINTING
  void print_on(Stream *st);
  static void p();
#endif

private:

#if ENABLE_CODE_PATCHING
public:
  static void update_checkpoints_table(CompiledMethod* cm);
  static void patch_checkpoints(address current_pc);
  static void unpatch_checkpoints();
  static bool is_undoing_patching() {
    return _is_undoing_patching;
  }
  static bool can_patch(int bci_from, int bci_to);

private:
  static void patch_compiled_method(CompiledMethod* cm);
  static void unpatch_compiled_method(CompiledMethod* cm);

  static CompiledMethodDesc* get_current_compiled_method(address current_pc);

  static bool _is_undoing_patching;
#endif
};

inline VirtualStackFrame* CodeGenerator::frame ( void ) {
  return Compiler::current()->frame();
}

inline VirtualStackFrame* GenericAddress::frame ( void ) {
  return Compiler::current()->frame();
}

class VirtualStackFrameContext: public StackObj {
 private:
  VirtualStackFrame* _saved_frame;
 
 public:
  VirtualStackFrameContext( VirtualStackFrame* context ) {
    GUARANTEE(context != NULL, "Sanity");
    _saved_frame = Compiler::frame();
    Compiler::set_frame(context);
  }
 ~VirtualStackFrameContext( void ) {
    Compiler::frame()->conform_to(_saved_frame);
    Compiler::set_frame(_saved_frame);
  }
};

#if ENABLE_PERFORMANCE_COUNTERS && ENABLE_DETAILED_PERFORMANCE_COUNTERS

#define FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(template) \
  template(method_entry, 0)                             \
  template(begin_compile, 0)                            \
  template(end_compile, 0)                              \
  template(conformance_entry, 0)                        \
  template(conform_to_entry, 0)                         \
  template(jmp_to_entry, 0)                             \
                                                        \
  template(bytecode_compile, 0)                         \
  template(bytecode_prolog, 0)                          \
  template(bytecode_epilog, 0)                          \
                                                        \
  template(push_int, 0)                                 \
  template(push_long, 0)                                \
  template(push_obj, 0)                                 \
  template(push_float, 0)                               \
  template(push_double, 0)                              \
                                                        \
  template(load_local, 0)                               \
  template(store_local, 0)                              \
  template(increment_local_int, 0)                      \
                                                        \
  template(array_length, 0)                             \
  template(load_array, 0)                               \
  template(store_array, 0)                              \
                                                        \
  template(binary, 1)                                   \
  template(unary, 0)                                    \
  template(convert, 0)                                  \
                                                        \
  template(pop, 0)                                      \
  template(pop_and_npe_if_null, 0)                      \
  template(pop2, 0)                                     \
  template(dup, 0)                                      \
  template(dup2, 0)                                     \
  template(dup_x1, 0)                                   \
  template(dup2_x1, 0)                                  \
  template(dup_x2, 0)                                   \
  template(dup2_x2, 0)                                  \
  template(swap, 0)                                     \
                                                        \
  template(branch, 0)                                   \
  template(branch_if, 0)                                \
  template(branch_if_icmp, 0)                           \
  template(branch_if_acmp, 0)                           \
                                                        \
  template(compare, 0)                                  \
                                                        \
  template(check_cast, 0)                               \
  template(instance_of, 0)                              \
  template(throw_exception, 0)                          \
  template(return_op, 0)                                \
  template(table_switch, 0)                             \
  template(lookup_switch, 0)                            \
                                                        \
  template(get_field, 0)                                \
  template(put_field, 0)                                \
  template(fast_get_field, 0)                           \
  template(fast_put_field, 0)                           \
                                                        \
  template(get_static, 0)                               \
  template(put_static, 0)                               \
                                                        \
  template(new_object, 0)                               \
  template(new_basic_array, 0)                          \
  template(new_object_array, 0)                         \
  template(new_multi_array, 0)                          \
                                                        \
  template(monitor_enter, 0)                            \
  template(monitor_exit, 0)                             \
                                                        \
  template(invoke_static, 0)                            \
  template(invoke_interface, 0)                         \
  template(fast_invoke_virtual, 0)                      \
  template(fast_invoke_virtual_final, 0)                \
  template(fast_invoke_special, 0)                      \
  template(invoke_native, 0)                            \
                                                        \
  template(invoke_special, 0)                           \
  template(invoke_virtual, 0)                           \
                                                        \
  template(throw_exception_stub, 0)                     \
  template(check_cast_stub, 0)                          \
  template(osr_stub, 0)                                 \
                                                        \
  template(instance_of_stub, 0)                         \
  template(type_check_stub, 0)                          \
  template(stack_overflow_stub, 0)                      \
  template(timer_tick_stub, 0)                          \
                                                        \
  template(new_object_stub, 0)                          \
  template(new_type_array_stub, 0)                      \
                                                        \
  template(generic_compile, 1)                          \
  template(custom_compile, 1)                           \
                                                        \
  template(sentinel, 0)                                 \
  template(relocation, 0)                               \
  template(init_static_array, 0)

#define DEFINE_COUNTER_FIELDS(name, counter_level) \
  julong name ## _time;                            \
  jint   name ## _size;                            \
  jint   name ## _count;

struct Compiler_PerformanceCounters {
  int level;
  FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(DEFINE_COUNTER_FIELDS)
};

#undef DEFINE_COUNTER_FIELDS

extern Compiler_PerformanceCounters comp_perf_counts;

#define DEFINE_COUNTER_WRAPPER_CLASS(name, counter_level)                  \
class counter_ ## name {                                                   \
public:                                                                    \
  counter_ ## name() {                                                     \
    GUARANTEE(comp_perf_counts.level <= counter_level, "Sanity");          \
    _level = comp_perf_counts.level;                                       \
    comp_perf_counts.level = counter_level + 1;                            \
                                                                           \
    _start_offset = Compiler::current()->code_generator()->code_size();    \
    _start_time = Os::elapsed_counter();                                   \
  }                                                                        \
                                                                           \
  ~counter_ ## name() {                                                    \
    comp_perf_counts.name ## _time +=                                      \
      Os::elapsed_counter() - _start_time;                                 \
    comp_perf_counts.name ## _size +=                                      \
      Compiler::current()->code_generator()->code_size() - _start_offset;  \
    comp_perf_counts.name ## _count++;                                     \
                                                                           \
    GUARANTEE(comp_perf_counts.level > counter_level, "Sanity");           \
    comp_perf_counts.level = _level;                                       \
  }                                                                        \
private:                                                                   \
  jlong _start_time;                                                       \
  jint  _start_offset;                                                     \
  int   _level;                                                            \
};

FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(DEFINE_COUNTER_WRAPPER_CLASS)

#undef DEFINE_COUNTER_WRAPPER_CLASS

#define DEFINE_COUNTER_LEVEL(name, counter_level) \
const int name ## _level = counter_level;

FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(DEFINE_COUNTER_LEVEL)

#undef DEFINE_COUNTER_LEVEL

#define COMPILER_PERFORMANCE_COUNTER_ACTIVE() (comp_perf_counts.level > 0)

#define COMPILER_PERFORMANCE_COUNTER_START(name)                   \
    GUARANTEE(comp_perf_counts.level <= name ## _level, "Sanity"); \
    int _level = comp_perf_counts.level;                           \
    comp_perf_counts.level = name ## _level + 1;                   \
                                                                   \
    jint  __start_offset =                                         \
      Compiler::current()->code_generator()->code_size();          \
    jlong __start_time = Os::elapsed_counter()

#define COMPILER_PERFORMANCE_COUNTER_END(name)                             \
    comp_perf_counts.name ## _time +=                                      \
      Os::elapsed_counter() - __start_time;                                \
    comp_perf_counts.name ## _size +=                                      \
      Compiler::current()->code_generator()->code_size() - __start_offset; \
    comp_perf_counts.name ## _count++;                                     \
                                                                           \
    GUARANTEE(comp_perf_counts.level > name ## _level, "Sanity");          \
    GUARANTEE(_level <= name ## _level, "Sanity");                         \
    comp_perf_counts.level = _level                           

#define INCREMENT_COMPILER_PERFORMANCE_COUNTER(name, value) \
    comp_perf_counts.name ## _size += (value);              \
    comp_perf_counts.name ## _count++;

#define COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(name) \
  counter_ ## name __counter_ ## name;

#else
#define COMPILER_PERFORMANCE_COUNTER_ACTIVE() false

#define COMPILER_PERFORMANCE_COUNTER_START()
#define COMPILER_PERFORMANCE_COUNTER_END(name)
#define INCREMENT_COMPILER_PERFORMANCE_COUNTER(name, value)
#define COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(name) 
#endif

#else
// !ENABLE_COMPILER
class Compiler: public StackObj {
public:
  static bool is_active() {
    return false;
  }
  static void abort_suspended_compilation() {}
  static void on_timer_tick() {}
};
#endif
