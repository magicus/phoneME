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

#include "incls/_precompiled.incl"
#include "incls/_Compiler.cpp.incl"

#if ENABLE_COMPILER

CompilerStatic  Compiler::_state;
CompilerContext Compiler::_suspended_compiler_context;

inline void CompilerStatic::cleanup( void ) {
  jvm_memset( this, 0, sizeof *this );
}

inline void CompilerContext::oops_do( void do_oop(OopDesc**) ) {
  if( valid() ) {
    OopDesc** p = (OopDesc**) this;
    for( int i = pointer_count(); --i >= 0; p++ ) {
      do_oop( p );
    }
    if (parent() != NULL) {
      parent()->context()->oops_do( do_oop );
    }
  }
}

inline void CompilerContext::cleanup( void ) {
  if( valid() ) {
    jvm_memset( this, 0, sizeof *this );
  }
}

jlong                         Compiler::_estimated_frame_time;
jlong                         Compiler::_last_frame_time_stamp;
Compiler::CompilationFailure  Compiler::_failure;
#ifndef PRODUCT
Compiler::CompilationHistory* Compiler::_history_head;
Compiler::CompilationHistory* Compiler::_history_tail;
#endif

Compiler::Compiler( Method* method, const int active_bci ) {
  jvm_memset(&_context, 0, sizeof _context );
#if ENABLE_INLINE
  if (is_active()) {
    // Dump cached values to the context
    current()->set_saved_bci(Compiler::bci());
    current()->set_saved_num_stack_lock_words(
      Compiler::num_stack_lock_words());

    // Set local base for the new compiler
    GUARANTEE(frame() != NULL, "Frame must be created by the caller");
    set_local_base(frame()->virtual_stack_pointer() - 
                   method->size_of_parameters() + 1);
  } else 
#else
  GUARANTEE(!is_active(), "Only one compiler at a time");
#endif
  {
    set_root(this);
    set_local_base(0);
  }
  set_parent(current());
  set_current(this);

  set_method( method );
  _failure = reservation_failed;
  _closure.initialize(this, method, active_bci);

#if ENABLE_CSE
  VirtualStackFrame::init_status_of_current_snippet_tracking();
  RegisterAllocator::wipe_all_notations();
#endif
  _compiler_method  = method;
  _compiler_closure = &_closure;
}

#ifndef PRODUCT
Compiler::Compiler() {
  GUARANTEE(!is_active(), "Only one compiler at a time");
  set_root(this);
  set_parent(current());
  set_current(this);
}
#endif

Compiler::~Compiler() {
  GUARANTEE(is_active(), "Sanity check");
  Compiler * parent_compiler = parent();

  GUARANTEE(parent_compiler == NULL || ENABLE_INLINE, 
            "Only one compiler if not inlining");

  if (parent_compiler != NULL) {
#ifdef AZZERT
    if (!CURRENT_HAS_PENDING_EXCEPTION) {
      Signature::Raw signature = current_method()->signature();
      const BasicType return_type = signature().return_type(true);
      GUARANTEE(frame()->virtual_stack_pointer() == 
                local_base() - 1 + word_size_for(return_type), "Sanity");
    }
#endif

    _compiler_closure     = &parent_compiler->_closure;
    _compiler_method      = parent_compiler->method();
    _compiler_bci         = parent_compiler->saved_bci();
    _num_stack_lock_words = parent_compiler->saved_num_stack_lock_words();
  } else {
    set_root(NULL);
  }
  set_current(parent_compiler);
}

void Compiler::on_timer_tick(bool is_real_time_tick JVM_TRAPS) {
  if( !UseCompiler || !Universe::is_compilation_allowed() ) {
    return;
  }

  if( TestCompiler ) {
    // We are testing the compiler (which may the ARM compiler running
    // inside an x86-hosted romgen). At this point we're initializing
    // the test classes and get a timer tick. Don't compile, or else
    // we will soon be executing JIT code of the wrong architecture!
    return;
  }

#if ENABLE_INTERPRETATION_LOG
  if (is_real_time_tick) {
    process_interpretation_log();
  }
#endif

  JavaFrame frame(Thread::current());

  // We don't instrument backward branches to update _method_execution_sensor,
  // so it is necessary to supplement the detection of execution of
  // loop-intensive compiled methods by sampling on timer ticks
  if( frame.is_compiled_frame() ) {
    const OopDesc* cm = frame.as_JavaFrame().compiled_method();
    if( !ROM::system_contains( cm ) ) {
      _method_execution_sensor[((CompiledMethodDesc*)cm)->get_cache_index()] = 0;
    }
  }

  CompiledMethodCache::on_timer_tick();
  if( --Universe::_compilation_abstinence_ticks >= 0 ) {
    return;
  }

  const jint bci = frame.bci();
#if ENABLE_JAVA_DEBUGGER
    // If we are single stepping in this thread, and we are at bci == 0
    // we might be stepping into this method.  So don't compile it.
    // The debugger code will mark it as impossible_to_compile if it
    // actually steps into it.

    if (Thread::current()->is_stepping() && bci == 0) {
      return;
    }
#endif

  const bool resume = Compiler::is_suspended();

  UsingFastOops fast_oops;
  Method::Fast current_compiling;
  if( resume ) {
    GUARANTEE(!Compiler::is_inlining(), 
              "Suspend during inlining is not supported");
    // IMPL_NOTE: (1) don't resume compilation if we have spent too many ticks
    //            on it already.
    //        (2) if current_compiling is the current method, set active_bci
    //            so that it can be more easily OSR'ed.
    CompiledMethod::Raw suspended_compiled_method = 
      Compiler::current_compiled_method();
    if (suspended_compiled_method.not_null()) {
      current_compiling = suspended_compiled_method().method();
    }
  } else {
    if (!frame.is_compiled_frame()) {
      current_compiling = frame.method();
      if (bci == 0) {
        Symbol::Raw name = current_compiling().name();
        if (name.equals(Symbols::class_initializer_name())) {
          // Don't compile <clinit> methods on-entry. If this method 
          // contains a hot loop, it will eventually be discovered later and
          // we will compile it then. But in most cases, <clinit> methods
          // aren't worth compiling.
          return;
        }
      }
    }
  }
  if( current_compiling.not_null() ) {
#ifndef PRODUCT
    if( TraceCompiledMethodCache ) {
      tty->print_cr("Current method ");
      current_compiling().print_name_on( tty );
      if( current_compiling().is_impossible_to_compile() ) {
        tty->print( " - impossible to compile" );
      }
      else if( current_compiling().has_compiled_code() ) {
        tty->print( " - already compiled" );
      }
      tty->cr();
    }
#endif
    const bool is_compiled =
      current_compiling().compile(bci, resume JVM_MUST_SUCCEED);
    if( is_compiled && InstallCompiledCode && !frame.is_compiled_frame()
        && current_compiling.obj() == frame.method() ) {
      frame.osr_replace_frame(bci);
    }
  }
}

#if ENABLE_INTERPRETATION_LOG
void Compiler::process_interpretation_log() {
  jlong now = Os::java_time_millis();
  if (now < _last_frame_time_stamp + _estimated_frame_time) {
    Universe::reset_interpretation_log();
    return;
  }

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "Interpretation log scan beg" ));
  }

#if ENABLE_PERFORMANCE_COUNTERS
  if( InterpretationLogSize < INTERP_LOG_SIZE ) {
    // Clear interpretation log tail
    jvm_memset(&_interpretation_log[InterpretationLogSize], 0,
               (INTERP_LOG_SIZE - InterpretationLogSize) * 
               sizeof(_interpretation_log[0]));
  }
#endif


  // Mark all the recently interpreted methods to be
  // compiled-on-invocation
  unsigned possible_to_compile_count = 0;
  ForInterpretationLog( p ) {
    Method::Raw m = *p;

#ifndef PRODUCT
    if( TraceCompiledMethodCache ) {
      m().print_name_on( tty );
      if( m().is_impossible_to_compile() ) {
        tty->print( " - impossible to compile" );
      } else if( m().has_compiled_code() ) {
        tty->print( " - already compiled" );
      }
      tty->cr();
    }
#endif

    if( m().can_be_compiled() ) {
      m().set_execution_entry((address) shared_invoke_compiler);
      possible_to_compile_count++;
    }
    *p = NULL;
  }
  _interpretation_log_idx = 0;

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "Interpretation log scan end, "
                   "scanned %d entries, scheduled %d",
                   p - _interpretation_log, possible_to_compile_count ));
  }

  enum { InterpreterFeedbackThreshold = 3 };
  if( possible_to_compile_count > InterpreterFeedbackThreshold ) {
      CompiledMethodCache::degrade();
  }
}
#endif // ENABLE_INTERPRETATION_LOG


void Compiler::oops_do( void do_oop(OopDesc**) ) {
  _suspended_compiler_context.oops_do( do_oop );
  if (is_active()) {
    current()->context()->oops_do( do_oop );
  }
  code_generator()->oops_do( do_oop );
}

void Compiler::terminate ( OopDesc* result ) {
#if ENABLE_TTY_TRACE
  if (PrintCompilation) {
    if (result != NULL) {
      TTY_TRACE_CR(("<done %d>", ((CompiledMethodDesc*)result)->object_size()));
    } else {
      TTY_TRACE_CR(("<done>"));
    }
  }
#endif
  if(_failure != none || result == NULL) {
#if ENABLE_TRAMPOLINE
    if (!GenerateROMImage) {
      BranchTable::revoke(
        (address) Compiler::current_compiled_method()->obj());
    }
#endif
  }
  CodeGenerator::terminate();
  _state.cleanup();
  _suspended_compiler_context.cleanup();
  ObjectHeap::update_compiler_area_top( result );
}

void Compiler::set_impossible_to_compile(Method *method, const char why[]) {
  if (!method->is_impossible_to_compile()) {
    (void)why;
#if ENABLE_TTY_TRACE
    if (PrintCompilation || TraceFailedCompilation) {
      tty->print("[impossible to compile: ");
      method->print_name_on(tty);
      tty->print_cr("] - %s", why);
    }
#endif
#if ENABLE_PERFORMANCE_COUNTERS
    jvm_perf_count.num_of_compilations_failed ++;
#endif
  }

  method->set_impossible_to_compile();
}

inline void Compiler::check_free_space( JVM_SINGLE_ARG_TRAPS ) const {
  code_generator()->check_free_space( JVM_SINGLE_ARG_NO_CHECK );
}

inline
ReturnOop Compiler::allocate_and_compile(const int compiled_code_factor
                                         JVM_TRAPS) {
#ifndef PRODUCT
  if( TraceCompiledMethodCache ) {
    tty->print( "\n*** Compiler::allocate_and_compile( " );
    method()->print_name_on( tty );
    tty->print_cr( ") beg ***" );
  }
#endif

  jvm_fast_globals.compiler_code_generator = NULL;

  const jint size = align_allocation_size(1024 + (method()->code_size() *
                                            compiled_code_factor));

  if (!reserve_compiler_area((size_t)size)) {
    // We don't have enough space (yet) to compile this method. We'll try to
    // compile it later.
    return NULL;
  }

  if( CompiledMethodCache::alloc() == CompiledMethodCache::UndefIndex ) {
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(("out of cache indices"));
    };
    return NULL;
  }

  if( TraceCompiledMethodCache ) {
    TTY_TRACE_CR(( "allocation size: %d, CompiledCodeFactor: %d",
        size, compiled_code_factor ));
  };

  set_bci(0);

  // Allocate a compiled method.
  CodeGenerator* gen = CodeGenerator::allocate(size JVM_NO_CHECK);
  if( gen == NULL ) {
    if( TraceCompiledMethodCache ) {
      TTY_TRACE_CR(( "Compilation FAILED - out of memory" ));
    }
    return NULL;
  }
  set_code_generator();
  current_compiled_method()->set_method(method());

#if USE_COMPILER_COMMENTS
  if (PrintCompilation || PrintCompiledCode || PrintCompiledCodeAsYouGo) {
#if ENABLE_PERFORMANCE_COUNTERS
    if (VerbosePointers) {
      tty->print("#%d: ", jvm_perf_count.num_of_compilations);
    }
#endif
    tty->print("(bci=%d) Compiling ", 
               closure()->active_bci());
    method()->print_name_on(tty);
    tty->cr();
  }
#endif

  begin_compile( JVM_SINGLE_ARG_NO_CHECK );
  if( CURRENT_HAS_PENDING_EXCEPTION ) {
    handle_out_of_memory();
  } else {
    process_compilation_queue( JVM_SINGLE_ARG_NO_CHECK );
  }

  UsingFastOops fast_oops;
  CompiledMethod::Fast result = current_compiled_method();
  switch( _failure ) {
    default:
      // Zap the unused contents in order not to confuse ObjectHeap::verify()
      result().shrink(0, 0);
      if( TraceCompiledMethodCache ) {
        TTY_TRACE_CR(( "Compilation failed - exception thrown" ));
      };
      // Intentionally no break here
    case out_of_time:
      result.set_null();
    case none:
      break;
  }

  if (PrintCompilationAtExit) {
    append_compilation_history();
  }
  return result;
}

inline ReturnOop Compiler::try_to_compile(Method* method,
  const int active_bci, const int compiled_code_factor JVM_TRAPS)
{
  OopDesc* result;
  {
    Compiler compiler( method, active_bci );
    compiler.init_performance_counters(false);
    result = compiler.allocate_and_compile(compiled_code_factor JVM_NO_CHECK);
    compiler.update_performance_counters(false, result);
    Thread::clear_current_pending_exception();
  }

  if( _failure != out_of_time ) {
    terminate( result );
  }
  return result;
}

/*
 * The chain of command:
 * Compiler::compile()
 *  calls Compiler::try_to_compile()
 *   calls Compiler::allocate_and_compile()
 *    calls - begin_compile()
 *          - process_compilation_queue()
 */
ReturnOop Compiler::compile(Method* method, int active_bci JVM_TRAPS) {
  // Bail out if the method is impossible to compile
  if (method->is_impossible_to_compile()) {
    return NULL;
  }

  EnforceCompilerJavaStackDirection enfore_java_stack_direction;

  EventLogger::start(EventLogger::COMPILE);

  CompilationQueueElement::reset_pool();
  ObjectHeap::save_compiler_area_top();

  // We need a bigger compiled code factor if any of these are set.
  // The following values are justs guesses.  They may need to be fixed.
  int compiled_code_factor = CompiledCodeFactor * (1 + 10 * (
    int(GenerateCompilerComments) +
    int(Deterministic) +
    int(TraceBytecodesCompiler)));
  if( method->is_double_size() ) {
    compiled_code_factor <<= 1;
  }

  CompiledMethod::Raw result =
    try_to_compile( method, active_bci, compiled_code_factor JVM_MUST_SUCCEED);

  EventLogger::end(EventLogger::COMPILE);
  return result();
}

inline void Compiler::setup_for_compile( const Method::Attributes& attributes
                                         JVM_TRAPS ) {
  // Mark the compiler as being outside any loops.
  mark_as_outside_loop();

  BytecodeCompileClosure::set_jump_from_bci(0);
  
  Compiler::set_entry_counts_table( attributes.entry_counts );
  Compiler::set_bci_flags_table( attributes.bci_flags );

  Compiler::set_num_stack_lock_words(
    attributes.num_locks * 
    ((BytesPerWord + StackLock::size()) / sizeof(jobject)));

#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
  Compiler::set_codes_can_throw_null_point_exception(
    attributes.num_bytecodes_can_throw_npe << 1);
#endif

  Compiler::set_has_loops( attributes.has_loops );

  { // Allocate object array for the entry table
    EntryTableType* p =
      EntryArray::allocate( method()->code_size() JVM_NO_CHECK_AT_BOTTOM );
    set_entry_table( p );
  }
}

inline void Compiler::begin_compile( JVM_SINGLE_ARG_TRAPS ) {
  Method::Attributes attributes;
  const Method* const mthd = method();
  mthd->compute_attributes( attributes JVM_CHECK );

  Compiler::setup_for_compile( attributes JVM_CHECK );
  code_generator()->set_omit_stack_frame( 
    OmitLeafMethodFrames &&
    (!ENABLE_WTK_PROFILER || TestCompiler) &&
    !attributes.can_throw_exceptions &&
    !attributes.has_loops && 
    mthd->max_execution_stack_count() <= 4 && 
    mthd->code_size() <= 20 &&
    !mthd->uses_monitors() );

  {// Instantiate a virtual stack frame for this method.    
    VirtualStackFrame* p = VirtualStackFrame::create(method() JVM_ZCHECK(p) );
    set_frame( p );
  }
  // A different allocation table may be used depending on frame omission.
  RegisterAllocator::initialize();

  {
    COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(method_entry);
    // Compile the method entry.
    code_generator()->method_entry(method() JVM_CHECK);
  }

  
#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
   //init the table which will record all the null point related
   //LDR instr in current CC.
  {
    Compiler::current()->set_null_point_record_counter(0);
    CompilerIntArray* p =
      CompilerIntArray::allocate( codes_can_throw_null_point_exception() JVM_ZCHECK(p));
    Compiler::current()->set_null_point_exception_ins_table(p);
  }
#endif

  // Preload the parameter of array type
  // also preload the array length
  code_generator()->preload_parameter(method());

  {    
    // Add the initial compilation continuation (bci = 0) to the
    // compilation queue.
    BinaryAssembler::Label unused;
    CompilationContinuation* stub =
      CompilationContinuation::insert(0, unused JVM_ZCHECK( stub ) );

    // A simple method that has no loops and no invocation will return
    // quickly. Let's save space by no generating the OSR entry.
    // In comp mode with OmitLeafMethodFrames off OSR entry is always generated
    // No need for OSR entry at first compilation continuation if we're
    // doing AOT compilation.
    if( !GenerateROMImage && ( has_loops() || !method()->is_leaf() ||
         !(MixedMode || OmitLeafMethodFrames) ) ) {
      stub->set_need_osr_entry();
    }
  }
  check_free_space(JVM_SINGLE_ARG_CHECK);
}

inline void Compiler::handle_out_of_memory( void ) {
  method()->set_double_size();
}

inline void Compiler::suspend( void ) {
  GUARANTEE(parent() == NULL, "Cannot suspend while inlining");
  _suspended_compiler_context = *context();
}

#if  ENABLE_INTERNAL_CODE_OPTIMIZER
BinaryAssembler::Label Compiler::get_next_pinned_entry( void ) {
  while( _next_element ) {
    const CompilationQueueElement* e = _next_element;
    _next_element = _next_element->next();

    switch( e->type() ) {
      case CompilationQueueElement::osr_stub:
      case CompilationQueueElement::entry_stub:
        return e->entry_label();
    }
  }
  BinaryAssembler::Label label;
  return label;
}
  
#if ENABLE_NPCE
#define is_shared_npe_stub(queue)      (queue->type() == CompilationQueueElement::throw_exception_stub &&\
                             ( (ThrowExceptionStub*) queue)->get_rte() ==\
                            ThrowExceptionStub::rte_null_pointer &&\
                            !((ThrowExceptionStub*) queue)->is_persistent())

void Compiler::record_instr_offset_of_null_point_stubs(int start_offset) {
  for( CompilationQueueElement* queue = compilation_queue(); queue; queue = queue->next() ) {
    if (is_shared_npe_stub(queue)) {
      //entry_label record the offset the stub should patch
      const int offset = queue->entry_label().position();
      if (code_generator()->is_branch_instr(offset)){
         //not a npce optimized stub. use old cmp, b approach
         //we won't do extend basic block schedule on them so continue.
        continue;
      }

      //only ins of current cc will be taken into account
      if (offset >= start_offset) {
        _internal_code_optimizer.record_npe_ins_with_stub( offset>>2 ,0);
        if( ((NullCheckStub*)queue)->is_two_words() ) {
          jint offset_of_second_ins = ((NullCheckStub*)queue)->offset_of_second_instr_in_words();
          _internal_code_optimizer.record_npe_ins_with_stub(((offset>>2) + offset_of_second_ins), 0);
        }
      }
      VERBOSE_SCHEDULING_AS_YOU_GO(("\t\t[%d: is a NPE instruction]", 
        queue->entry_label().position()));
    }
    VERBOSE_SCHEDULING_AS_YOU_GO(("\t\t[%d: is a NOT a NPE instruction]", 
        queue->entry_label().position()));
  }
}

void Compiler::update_null_check_stubs() {
  VERBOSE_SCHEDULING_AS_YOU_GO(("\tenter update_null_check_stubs "));
  VERBOSE_SCHEDULING_AS_YOU_GO(("\tNPE record count is %d", 
      _internal_code_optimizer.record_count_of_npe_ins_with_stub()));
  for( int index = 0; index < 
       _internal_code_optimizer.record_count_of_npe_ins_with_stub(); index++) {
    short new_offset =
     _internal_code_optimizer.scheduled_offset_of_npe_ins_with_stub(index);
    int old_offset = _internal_code_optimizer.offset_of_npe_ins_with_stub(index);
    for( CompilationQueueElement* queue = compilation_queue(); queue;
         queue = queue->next() ) {
      VERBOSE_SCHEDULING_AS_YOU_GO(("\tCurrent CC label is %d",
          queue->entry_label().position()));
      if (is_shared_npe_stub(queue)) { 
          //for double and long
          //for those ins, we must record the offset of ins who emitted first after scheduling.
          //somtimes, the second ins will be schedule ahead the first.
        if (queue->entry_label().position() == old_offset << 2) {  
          if (((NullCheckStub*)queue)->is_two_words()) {
            int new_offset_of_second_ins = 0;
            new_offset_of_second_ins = 
              _internal_code_optimizer.scheduled_offset_of_npe_ins_with_stub(index + 1);
            if (new_offset_of_second_ins == 0) {
                //new_offset is zero means the offset of ins is unchanged after scheduling.             
              new_offset_of_second_ins = 
                _internal_code_optimizer.offset_of_npe_ins_with_stub(index + 1);
            }
            if ((new_offset == 0 && new_offset_of_second_ins  < old_offset) ||
                 new_offset_of_second_ins < new_offset) {
              //first ins is behind second ins after scheduling   
              new_offset = new_offset_of_second_ins;
            }
            index++;
          }
                  
          if (new_offset == 0) {  
            break;
          }
                  
          //patch back   
          {
            BinaryAssembler::Label label = queue->entry_label();
            label.link_to(new_offset << 2);
            queue->set_entry_label(label);
          }
                  
          VERBOSE_SCHEDULING_AS_YOU_GO(("\tUpdate NPCE stub label old position is %d ", old_offset << 2));
          VERBOSE_SCHEDULING_AS_YOU_GO(("new position is %d", new_offset << 2));
          break;
        }
      }
    }
  }
}
#undef is_shared_npe_stub
#endif //ENABLE_NPCE
#endif //ENABLE_INTERNAL_CODE_OPTIMIZER

void Compiler::process_compilation_queue( JVM_SINGLE_ARG_TRAPS ) {
  // Tell OS to interrupt compilation after a platform-specific amount
  // of time. This avoid long compilation pauses when compiling big methods.
  if( !Compiler::is_inlining() ) { 
    Os::start_compiler_timer();
  }

  // Prevent spending run time on compilation
  // for a specified number of subsequent ticks:
  Universe::_compilation_abstinence_ticks = CompilationAbstinenceTicks;
  _failure = out_of_memory;

  UsingFastOops fast_oops;
  CompiledMethod::Fast result;

  // Compile until the continuation queue is empty
  while (!is_compilation_queue_empty()) {
    // Get and compile the first element from the compile queue.
    CompilationQueueElement* element = current_compilation_queue_element();
    const bool finished_one_element = element->compile(JVM_SINGLE_ARG_NO_CHECK);
    if( _failure == out_of_stack ) {
      if( Verbose ) {
        TTY_TRACE_CR(("Compilation of method %s failed due to out of stack "
                      "condition", method()->name()));
      }
      set_impossible_to_compile(method(), "out of stack" );
      return;
    }
    if( CURRENT_HAS_PENDING_EXCEPTION ) {
      goto Failure;
    }

    if (finished_one_element) {
      RegisterAllocator::guarantee_all_free();
      clear_current_element();
      element->free();
    } else {
      // We will resume compilation of the current element in the next
      // call to Compiler::resume()
    }

    check_free_space(JVM_SINGLE_ARG_NO_CHECK);
    if( CURRENT_HAS_PENDING_EXCEPTION ) {
      goto Failure;
    }
    if( !GenerateROMImage &&!is_compilation_queue_empty()
                          && is_time_to_suspend() ) {
      _failure = out_of_time;
      suspend();
      return;
    }
  }

  if (is_inlining()) {
    return;
  }

  result = code_generator()->finish();

#if !ENABLE_INTERNAL_CODE_OPTIMIZER
  optimize_code(JVM_SINGLE_ARG_NO_CHECK);
  if( CURRENT_HAS_PENDING_EXCEPTION ) {
    goto Failure;
  }
#endif

#ifdef FUNNY_CROSS_COMPILER
  // This is a fix used only for testing new ports.
  // We create a compiler that generates code for the wrong platform!
  // This lets us compile the code and print it, but we can't possibly run it.
  set_impossible_to_compile( method(), "funny cross-compiler" );
  return;
#endif

  if( !GenerateROMImage ) {
    CompiledMethodCache::insert( (CompiledMethodDesc*) result().obj() );
  }

#if ENABLE_TTY_TRACE
  if( PrintCompiledCode ) {
    if ( OptimizeCompiledCodeVerbose ) {
      tty->print_cr("After Optimization:");
    }
    result().print_code_on(tty);
  }
#endif

#if ENABLE_JVMPI_PROFILE 
  // Set the actual generated code size.
  if ( result.not_null() ) {
   ((CompiledMethodDesc*)result().obj())->jvmpi_set_code_size(
                                          code_generator()->code_size());
  }

  // Send the compiled method load event.
  if(UseJvmpiProfiler && 
    JVMPIProfile::VMjvmpiEventCompiledMethodLoadIsEnabled() &&
    result.not_null()) {
    JVMPIProfile::VMjvmpiPostCompiledMethodLoadEvent(result);
  }
#endif

  if (InstallCompiledCode) {   
    result().flush_icache();
    method()->set_compiled_execution_entry(result().entry());
    GUARANTEE(method()->has_compiled_code(), "check bit");
  } else {
    method()->set_default_interpreter_entry();
  }

#if ENABLE_CODE_PATCHING
  GUARANTEE(result().not_null(), "Sanity");
  //
  // Update checkpoints table here as the offsets of ldr instuctions
  // could be updated after we added the record for that instruction.
  //
  Compiler::update_checkpoints_table(&result);
#endif
  _failure = none;
  return;

Failure:
  if( _failure == out_of_memory ) {
    handle_out_of_memory();

    method()->set_double_size();
  }
}

#if ENABLE_INLINE
void Compiler::internal_compile_inlined( Method::Attributes& attributes
                                         JVM_TRAPS ) {
  Compiler::setup_for_compile(attributes JVM_CHECK);

  {
    BinaryAssembler::Label return_label;
    set_inline_return_label(return_label);
  }

  {    
    // Add the initial compilation continuation (bci = 0) to the
    // compilation queue.
    BinaryAssembler::Label unused;
    CompilationContinuation* stub =
      CompilationContinuation::insert(0, unused JVM_ZCHECK(stub));
    
    // Invalidate VSF of the parent compilation element
    // The proper VSF will be set during compilation of this child
    clear_parent_frame();
  }

  process_compilation_queue(JVM_SINGLE_ARG_CHECK);

  {
    BinaryAssembler::Label return_label = inline_return_label();
    if (!return_label.is_unused()) {
      code_generator()->bind(return_label);
    }
  }

  // Set the return frame as the current frame for the parent
  Compiler::set_frame( parent_frame() );
}
#endif  

inline bool Compiler::reserve_compiler_area(size_t compiled_method_size) {
  // We need about 75 bytes of compiler data buffer per byte of Java
  // bytecode for small methods (less than ~1000 bytes). The factor
  // for larger methods is typically smaller because we can re-use
  // CompilationQueueElements.
  const size_t factor = 75;
  size_t temp_data_size = factor * method()->code_size();
  size_t max_temp_data = HeapCapacity * CompilerAreaPercentage / 100 / 4;
  if (temp_data_size > max_temp_data) {
    // Don't use more than a quarter of the compiler area to compile any
    // method. This means very big methods would fail to compile and
    // would be marked as impossible to compile.
    temp_data_size = max_temp_data;
  }
  const int needed = compiled_method_size + temp_data_size;

  // IMPL_NOTE: make sure that we don't thrash with compiler_area_collect if
  // we're interpreting a large method but we can't collect enough space
  // at every timer tick.
  return ObjectHeap::compiler_area_soft_collect(needed) >= needed;
}

#if !ENABLE_INTERNAL_CODE_OPTIMIZER      
inline void Compiler::optimize_code( JVM_SINGLE_ARG_TRAPS ) {
#if ENABLE_CODE_OPTIMIZER
  if( OptimizeCompiledCode ) {
#if ENABLE_PERFORMANCE_COUNTERS
    JvmTimer opt_timer;
    opt_timer.start();
#endif
    UsingFastOops fast_oops;
    CompiledMethod::Fast result = current_compiled_method();

    int* code_begin = (int*)result().entry();
    int* code_end = (int*)(result().entry() + code_generator()->code_size());
      
    CodeOptimizer optimizer(&result, code_begin, code_end);
    optimizer.optimize_code(JVM_SINGLE_ARG_NO_CHECK); 
    if(CURRENT_HAS_PENDING_EXCEPTION) {
      Thread::clear_current_pending_exception();
      if( OptimizeCompiledCodeVerbose ) {
        TTY_TRACE_CR(( "Code optimization FAILED - out of memory" ));
      }
    }
#if ENABLE_TTY_TRACE
    opt_timer.stop();
    if( result().not_null() ) {
      if( PrintCompilation ) {
        tty->print_cr(" [optimized method size=%d, code=%d "
                      "(%.1fms = %.0f bytes/sec)]",
                      result().size(), code_generator()->code_size(),
                      jvm_f2d(jvm_fmul(jvm_d2f(opt_timer.seconds()), 1000.0f)),
                      jvm_f2d(jvm_fdiv(jvm_i2f(method()->code_size()),
                                     jvm_d2f(opt_timer.seconds()))));
      }
    }
#endif
  }
#else 
  JVM_IGNORE_TRAPS;
#endif /* ENABLE_CODE_OPTIMIZER*/
}
#endif

void Compiler::abort_suspended_compilation( void ) {
  if( is_suspended() ) {
    GUARANTEE(!is_active(), "sanity");
    _failure = none;
    terminate( NULL );
  }
}

void Compiler::abort_active_compilation(bool is_permanent JVM_TRAPS) {
  GUARANTEE(is_active(), "sanity");
  if (is_permanent) {
    set_impossible_to_compile(current()->method(), 
                              "compilation aborted permanently");
  }
  Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
}

inline void Compiler::restore_and_compile( JVM_SINGLE_ARG_TRAPS ) {
  set_code_generator();
  *context() = _suspended_compiler_context;
  GUARANTEE(parent() == NULL, "Cannot suspend while inlining");
  process_compilation_queue( JVM_SINGLE_ARG_NO_CHECK );
}

// Resume a compilation that has been suspended.
ReturnOop Compiler::resume_compilation(Method *method JVM_TRAPS) {
  EventLogger::start(EventLogger::COMPILE);

  OopDesc* result;

  if (PrintCompilation) {
    TTY_TRACE(("<c>"));
  }
  {
    // Put the handles in a separate scope, so when we call
    // ObjectHeap::update_compiler_area_top() we have no more handles pointing
    // into unused space in compiler_area.
    Compiler compiler(method, 0);
    compiler.init_performance_counters(true);
    compiler.restore_and_compile( JVM_SINGLE_ARG_NO_CHECK );
    Thread::clear_current_pending_exception();
    result = _failure == none ? current_compiled_method()->obj() : NULL;
    compiler.update_performance_counters(true, result);
  }

  if( _failure != out_of_time ) {
    terminate( result );
  }
  EventLogger::end(EventLogger::COMPILE);

  return result;
}

#ifndef PRODUCT

void Compiler::append_compilation_history() {
  CompilationHistory *h = 
      (CompilationHistory*)OsMemory_allocate(sizeof(CompilationHistory));
  if (_history_head == NULL) {
    _history_head = _history_tail = h;
  } else {
    _history_tail->_next = h;
    _history_tail = h;
  }
  h->_next = NULL;
  method()->print_name_to(h->_method_name, sizeof(h->_method_name));
}

void Compiler::print_compilation_history( void ) {
  int i = 0;
  CompilationHistory* h = _history_head;
  while( h ) {
    tty->print_cr("Compiled: #%d %s", i++, h->_method_name);
    CompilationHistory* next = h->_next;
    OsMemory_free( h );
    h = next;
  }
  _history_head = _history_tail = h;
}

#endif // PRODUCT

#if USE_DEBUG_PRINTING
void CompilerStatic::print_on(Stream *st) {
  #define COMPILER_STATIC_FIELDS_PRINT( type, name ) \
          st->print_cr("%-30s = %d", STR(name), _##name);
  COMPILER_STATIC_FIELDS_DO(COMPILER_STATIC_FIELDS_PRINT);
}

void Compiler::print_on(Stream *st) {
  st->print_cr("Compiler::_current = 0x%08x", current());
  _state.print_on(st);
  st->print_cr("Compiler::_closure = 0x%08x", &_closure);
  _closure.print_on(st);
}

void Compiler::p() {
  current()->print_on(tty);
}
#endif


#if ENABLE_PERFORMANCE_COUNTERS
void Compiler::init_performance_counters(bool is_resume) {
  _start_time = Os::elapsed_counter();
  _mem_before_compile = 
      ObjectHeap::used_memory() + jvm_perf_count.total_bytes_collected;
  _gc_before_compile = jvm_perf_count.num_of_gc;

  if (is_resume) {
    jvm_perf_count.compilation_resume_count ++;
  }
}

void Compiler::update_performance_counters(bool /*is_resume*/, 
                                           OopDesc *result) {
  {
    const jlong elapsed = Os::elapsed_counter() - _start_time;
    jvm_perf_count.total_compile_hrticks += elapsed;
    if (jvm_perf_count.max_compile_hrticks < elapsed) {
      jvm_perf_count.max_compile_hrticks = elapsed;
      //Symbol n = method()->name();
      //tty->print("<<<%d (%d)", (jint)(elapsed), is_resume);
      //n.print_symbol_on(tty);
      //tty->print(">>>");
    }
  }

  jvm_perf_count.num_of_compilations ++;
  {
    const int mem_used = ObjectHeap::used_memory() +
      jvm_perf_count.total_bytes_collected - _mem_before_compile;
    jvm_perf_count.total_compile_mem += mem_used;
    if (jvm_perf_count.max_compile_mem < mem_used) {
      jvm_perf_count.max_compile_mem = mem_used;
    }
  }

  if( result ) {
    {
      int code_size = ((CompiledMethodDesc*)result)->object_size();
      jvm_perf_count.total_compiled_methods += code_size;
      if (jvm_perf_count.max_compiled_method < code_size) {
        jvm_perf_count.max_compiled_method = code_size;
      }
    }
    {
      const int code_size = method()->code_size();
      jvm_perf_count.total_compiled_bytecodes += code_size;
      if (jvm_perf_count.max_compiled_bytecodes < code_size) {
        jvm_perf_count.max_compiled_bytecodes = code_size;
      }
    }
    jvm_perf_count.num_of_compilations_finished ++;
  }

  jvm_perf_count.num_of_compiler_gc +=
    jvm_perf_count.num_of_gc - _gc_before_compile;
}

#if ENABLE_DETAILED_PERFORMANCE_COUNTERS
Compiler_PerformanceCounters comp_perf_counts;

#define PRINT_COMPILER_SIZE_COUNTER(name, level)                    \
  JVM::P_INT(true, #name " size", comp_perf_counts. name ## _size); \
  JVM::P_INT(true, #name " avg ",                                   \
             comp_perf_counts. name ## _count ?                     \
             comp_perf_counts. name ## _size /                      \
             comp_perf_counts. name ## _count : 0); 

#define PRINT_COMPILER_COUNT_COUNTER(name, level) \
  JVM::P_INT(true, #name " count", comp_perf_counts. name ## _count);

#define PRINT_COMPILER_TIME_COUNTER(name, level) \
  JVM::P_HRT(true, #name " time", comp_perf_counts. name ## _time);

void Compiler::print_detailed_performance_counters() {
  tty->cr();

  FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(PRINT_COMPILER_SIZE_COUNTER)

  tty->cr();

  FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(PRINT_COMPILER_COUNT_COUNTER)

  tty->cr();

  FOR_ALL_COMPILER_PERFORMANCE_COUNTERS(PRINT_COMPILER_TIME_COUNTER)
}

#undef PRINT_COMPILER_SIZE_COUNTER
#undef PRINT_COMPILER_COUNT_COUNTER
#undef PRINT_COMPILER_TIME_COUNTER

#endif // ENABLE_DETAILED_PERFORMANCE_COUNTERS

#endif // ENABLE_PERFORMANCE_COUNTERS

#endif // ENABLE_COMPILER
