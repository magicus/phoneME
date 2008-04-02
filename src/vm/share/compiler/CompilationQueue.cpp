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
# include "incls/_CompilationQueue.cpp.incl"

#if ENABLE_COMPILER
CompilationQueueElement* CompilationQueueElement::_pool;
#if ENABLE_CSE
#define ABORT_CSE_TRACKING VirtualStackFrame::abort_tracking_of_current_snippet();\
      RegisterAllocator::wipe_all_notations();
#else
#define ABORT_CSE_TRACKING
#endif
CompilationQueueElement* CompilationQueueElement::allocate(
  const CompilationQueueElementType type, const jint bci JVM_TRAPS )
{
  VirtualStackFrame* frame = Compiler::frame();

  CompilationQueueElement* element = _pool;
  if( element ) {    
    _pool = element->next();  // Reuse CompilationQueueElement
    frame->copy_to( element->frame() );

    // These fields will be zero in a newly allocated object, but not 
    // necessarily in a reused object.
    element->set_info(0);
    element->clear_entry_label();
    element->clear_return_label();
    element->set_is_suspended(false);
  } else {
    // Allocate the new CompilationQueueElement.
    VirtualStackFrame* clone =
      frame->clone(JVM_SINGLE_ARG_ZCHECK_0( clone ) );
    element = COMPILER_OBJECT_ALLOCATE(CompilationQueueElement);
    if( !element ) {
      return element;
    }

    element->set_frame( clone );
  }

  // Fill out instance fields.
  element->set_type(type);
  element->set_bci(bci);

  // Clear the registers.
  element->set_register_0(Assembler::no_reg);
  element->set_register_1(Assembler::no_reg);

  return element;
}

bool CompilationQueueElement::compile(JVM_SINGLE_ARG_TRAPS) {
  // Set the virtual stack frame and the bytecode index in the compiler.
  if (!is_suspended()) {
    Compiler::current()->set_bci(bci());
  }
  Compiler::set_frame( frame() );
  Compiler::code_generator()->ensure_compiled_method_space();

  typedef void (CompilationQueueElement::*compile_func)(JVM_SINGLE_ARG_TRAPS);

  bool finished = true;
  static const compile_func funcs[] = {
    /* compilation_continuation */ NULL,
    /* throw_exception_stub     */ (compile_func) &ThrowExceptionStub::compile,
    /* type_check_stub          */ (compile_func) &TypeCheckStub::compile,
    /* check_cast_stub          */ (compile_func) &CheckCastStub::compile,
    /* instance_of_stub         */ (compile_func) &InstanceOfStub::compile,
#if ENABLE_INLINE_COMPILER_STUBS
    /* new_object_stub          */ (compile_func) &NewObjectStub::compile,
    /* new_type_array_stub      */ (compile_func) &NewTypeArrayStub::compile,
#endif
    /* osr_stub                 */ (compile_func) &OSRStub::compile,
    /* stack_overflow_stub      */ (compile_func) &StackOverflowStub::compile,
    /* timer_tick_stub          */ (compile_func) &TimerTickStub::compile,
    /* quick_catch_stub         */ (compile_func) &QuickCatchStub::compile
  };

  if (type() == compilation_continuation) {
    // All types will finish in one cycle, except for 
    // CompilationContinuation, which may be suspended in the middle if we
    // are compiling a very long sequence of bytecode.
#if ENABLE_INTERNAL_CODE_OPTIMIZER
    // IMPL_NOTE: need to revisit for inlining
    if (!Compiler::is_inlining()) {
      if (!is_suspended()) {
          //entry to internal code optimizer
        Compiler::current()->prepare_for_scheduling_of_current_cc( Compiler::current()->
                             code_generator()->compiled_method());
      }
    }
   finished =  
     CompilationContinuation::cast(this)->compile(JVM_SINGLE_ARG_CHECK_(true));
    // IMPL_NOTE: need to revisit for inlining
    if (!Compiler::is_inlining()) {
      // could add !is_suspended() && if problem happens
      if( finished ) {
        //if the CompliationConinuation is suspended, we skiped the schedule stage
        Compiler::current()->schedule_current_cc( Compiler::current()->
          code_generator()->compiled_method() JVM_CHECK_(true));
      }
    }
#else
    finished =  
        CompilationContinuation::cast(this)->compile(JVM_SINGLE_ARG_NO_CHECK);
#endif 
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  } else if (type() == entry_stub) {
    return finished;
#endif
  } else {
    GUARANTEE(throw_exception_stub <= type() && type() <= quick_catch_stub,
              "sanity");
    compile_func compile = funcs[type()];
    (this->*compile)(JVM_SINGLE_ARG_NO_CHECK);
  }

  return finished;
}

void CompilationQueueElement::insert() {
  Compiler::current()->insert_compilation_queue_element(this);
}

void CheckCastStub::insert( const int bci, const int class_id,
                       const BinaryAssembler::Label entry_label,
                       const BinaryAssembler::Label return_label JVM_TRAPS) {
  CheckCastStub* stub = (CheckCastStub*)
    CompilationQueueElement::allocate(check_cast_stub, bci JVM_ZCHECK(stub));
  stub->set_register_0(Assembler::no_reg);
  stub->set_class_id(class_id);
  stub->set_entry_label(entry_label);
  stub->set_return_label(return_label);
  Compiler::current()->insert_compilation_queue_element(stub);
}

CompilationContinuation* CompilationContinuation::insert( 
    const jint bci, const BinaryAssembler::Label entry_label JVM_TRAPS) {
  return insert(Compiler::current(), bci, entry_label JVM_NO_CHECK_AT_BOTTOM);
}

CompilationContinuation* CompilationContinuation::insert( Compiler * const compiler,
                                                          const jint bci,
                           const BinaryAssembler::Label entry_label JVM_TRAPS) {
  CompilationContinuation* stub = (CompilationContinuation*)
    CompilationQueueElement::allocate( compilation_continuation, bci
                                       JVM_ZCHECK_0(stub));
  stub->set_entry_label(entry_label);
  if( compiler->compiler_bci() < bci ) {
    // Mark the continuation as a forward branch target to disable 
    // loop peeling for it.
    stub->set_forward_branch_target();
  }

  compiler->insert_compilation_queue_element(stub);

  // We return the stub, since the caller may want to add some flags
  return stub;
}

void CompilationContinuation::begin_compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(begin_compile);

  CodeGenerator* gen = Compiler::code_generator();
  VirtualStackFrame* frame = Compiler::frame();
  frame->clear_flush_count();

  // If run_immediately() is true, then the preceding CompilerStub is expecting
  // to fall right into this one.  Currently, that is always the case, but
  // if this code ever changes, be careful

#if USE_COMPILER_COMMENTS
  COMPILER_COMMENT(("CompilationContinuation: bci = %d", Compiler::bci()));
  if (GenerateCompilerComments) {
    frame->dump(true);
  }
#endif

  set_code_size_before(gen->code_size());

  if (need_osr_entry()) {
    gen->osr_entry(JVM_SINGLE_ARG_CHECK);
  }

  set_entry_has_been_bound(false);
}

// Returns true if the element finished compilation, false if it
// has been suspended and needs to be resumed in the future.
bool CompilationContinuation::compile_bytecodes(JVM_SINGLE_ARG_TRAPS) {
  CodeGenerator* gen = Compiler::code_generator();

  // Compilation loop.
  for(;;) {
    // Get the entry from the compiler.
    Entry* entry = Compiler::current()->entry_for(Compiler::bci());
    if( entry ) {      
      //flush all the cached regiser
      VERBOSE_CSE(("clear notation for has_entry"));
      ABORT_CSE_TRACKING;

      // We enter here if we already have generated code for this closure;
      // We will not compile this closure again, but we may need to emit
      // additional merging code
      VirtualStackFrame* entry_frame = entry->frame();
      VirtualStackFrame* frame = Compiler::frame();

      // If we're not already in a loop and the entry we've found is on
      // the current (fall-through) compilation string we assume we've
      // found a loop. For now we do one level of loop peeling.

      // Don't do this if we've generated too much code for the first run of
      // the loop (gen->code_size() - entry.code_size() > X), or if the virtual
      // stack frame has been flushed since this entry was created
      // (frame->flush_count() != entry_frame.flush_count()).

      if (OptimizeLoops && !forward_branch_target()
          && !Compiler::is_in_loop() && entry->bci() == bci()
          && frame->flush_count() == entry_frame->flush_count()
          && gen->code_size() - entry->code_size() <= LoopPeelingSizeLimit) {
#if ENABLE_CODE_PATCHING
        const int end_bci = BytecodeCompileClosure::jump_from_bci(); 
        if (!Compiler::can_patch(Compiler::bci(), end_bci)) {
          BytecodeCompileClosure::set_jump_from_bci(0);
        }
#endif
        Compiler::mark_as_in_loop();
      } else {
        // Otherwise, we emit frame merge code and jump to the entry.
        Compiler::set_conforming_frame( entry_frame );
        {
          COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(conform_to_entry);
          frame->conform_to( entry_frame );
        }
        Compiler::set_conforming_frame( NULL );
        BinaryAssembler::Label branch_label = entry->label();

        BinaryAssembler::Label compile_entry_label = entry_label();
        if (gen->code_size()== code_size_before() && !entry_has_been_bound()) {
          // We've generated no code at all for this compilation queue
          // element.  Just have whoever created us branch directly to the
          // conformance entry.
          GUARANTEE(branch_label.is_bound(), "Entry labels are always bound");
          gen->bind_to(compile_entry_label, branch_label.position());
          set_entry_has_been_bound(true);
        }

#if ENABLE_LOOP_OPTIMIZATION && ARM
        //create a Label bind to current  position
        bool need_jmp = true;
        if ( Compiler::current()->has_loops() ) {
          BinaryAssembler::Label cur_label;
          gen->bind(cur_label);
    
          //current position
          int cur_pos = cur_label.position();   
          //jmp destination position
          int brc_pos = branch_label.position();    
    
          // Must be a backward jump
          GUARANTEE(brc_pos < cur_pos, "Sanity");
          { 
            COMPILER_COMMENT(("Jump back in loop"));
            bool found_jmp = false;
            int next_pos = brc_pos;     
            Assembler::Condition cond;
            int offset;
          
            //op_pc save the status of whether contain pc Operation
            bool link, op_pc;   
            int count = 0;

            // search the jump instruction within max_search_length
            // insturctions range          
            while(next_pos < cur_pos && count < max_search_length) { 
              // get the instruction in specified address
              int instr = gen->instruction_at(next_pos);   
              int next_instr = gen->instruction_at(next_pos + BytesPerWord);   
              //chech whether the instruction is jump instruction. And retrive the cond, link status if so
              // retrive the cond, link status if so
              found_jmp = gen->is_jump_instr(instr, next_instr, cond, link, op_pc, offset);

              // if found pc operation in current instruction, then do
              // nothing optimization
              if( op_pc || found_jmp ) {
                break;
              }
              next_pos += BytesPerWord;
              count ++;
            }

            // if found conditional jump instruction, then...
            if(found_jmp && cond < Assembler::al) {
              COMPILER_COMMENT(("Begin -- Loop Optimization"));
              // copy the instructions between the branch position and
              // conditional jump position to current position
              int ins_pos = brc_pos;
              while(ins_pos < next_pos) {
                int instr = gen->instruction_at(ins_pos);
                gen->emit(instr);
                ins_pos += BytesPerWord;
              }
          
              // emit a reverse conditional jump instruction, jump to
              // the next instruction following the found conditional
              // jump instruction
              BinaryAssembler::Label jmp_label;
              jmp_label.bind_to(next_pos + BytesPerWord);
              gen->back_branch(jmp_label, link, gen->get_reverse_cond( cond));

              if(offset != -8) {
                COMPILER_COMMENT(("Jump back"));
                jmp_label.bind_to(offset + next_pos + 8);
                gen->back_branch(jmp_label, false, Assembler::al);
                need_jmp = false;
              } else {
                COMPILER_COMMENT(("Jump forward"));
              
                // modify the branch label to bind to the found
                // conditional jump instruction
                branch_label.bind_to(next_pos);
              
                int jmp_bci = -1;
                Compiler* const compiler = Compiler::current();
                CompilationQueueElement* element =
                  compiler->get_first_compilation_queue_element();  
                
                for( CompilationQueueElement* pre_element = element; element;
                     element = element->next() ) {
                  const BinaryAssembler::Label compile_entry_label_elem =
                    element->entry_label();
                  if( compile_entry_label_elem.is_linked() &&
                      compile_entry_label_elem.position() == next_pos ) {
                    if( pre_element != element ) {
                      CompilationQueueElement* next = element->next();
                      pre_element->set_next(next);
                      compiler->insert_compilation_queue_element(element);
                    }
                    need_jmp = false;
                    break;
                  }
                  pre_element = element;
                }
                if(need_jmp)
                  COMPILER_COMMENT(("Can not find the Entry label"));
              }
            }
          }
        } 
        if(need_jmp) {
#endif //#if ENABLE_LOOP_OPTIMIZATION && ARM

        // We finish by branching to the conformance entry.
        // [FY: We generate the "jmp" even if the immediately previous "if" 
        // statement is true.  Code below might have bound some entries
        // to this location.  It's easier to just always generate the jmp 
        // rather than figure out whether we need it or not.]
        {
          COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(jmp_to_entry);
          gen->jmp(branch_label);
        }

#if ENABLE_LOOP_OPTIMIZATION && ARM
        }
#endif
        // Terminate this compilation string and any loops.
        Compiler::mark_as_outside_loop();
        BytecodeCompileClosure::set_jump_from_bci(0);
        return true;
      }
    }

    if (Compiler::current()->entry_count_for(Compiler::bci()) > 1) {
      VirtualStackFrame* frame = Compiler::frame();
      // Make sure any other frame can be made conformant to this one.
      {
        COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(conformance_entry);
        frame->conformance_entry(true);
      }

      // Emit code for an entry.
      BinaryAssembler::Label entry_label;
      if (Compiler::is_in_loop()) {
        gen->bind(entry_label, oopSize);
      } else {
        gen->bind(entry_label);
      }
      {
        // Register the entry in the compiler.
        Entry* entry = Entry::allocate(bci(), frame, entry_label,
                                           gen->code_size()
                                           JVM_ZCHECK_0(entry));
        Compiler::current()->set_entry_for(Compiler::bci(), entry);
      }
#if ENABLE_INTERNAL_CODE_OPTIMIZER
      {
        EntryStub* stub =
          EntryStub::allocate( bci(), entry_label JVM_ZCHECK_0(stub));
        stub->insert();
      }
#endif

      VERBOSE_CSE(("clear notation for bci with multiple entry "));
      ABORT_CSE_TRACKING;
    }
    if (PrintCompiledCodeAsYouGo && GenerateCompilerComments) {
      tty->cr();
    }
    const bool continue_compilation =
      Compiler::closure()->compile(JVM_SINGLE_ARG_CHECK_0);
    if( !continue_compilation ) {
      return true; // compilation has finished
    }
    if( Compiler::is_time_to_suspend() ) {
      set_is_suspended(true);
      return false; // compilation needs to be resumed later.
    }
  }
}

void CompilationContinuation::end_compile( void ) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(end_compile);

  CodeGenerator* gen = Compiler::code_generator();

  // Bind the entry label, if we haven't already done so.
  if (!entry_has_been_bound()) { 
    BinaryAssembler::Label compile_entry_label = entry_label();
    gen->bind_to(compile_entry_label, code_size_before());
  }
}

bool CompilationContinuation::compile(JVM_SINGLE_ARG_TRAPS) {
  if (!is_suspended()) {
    VERBOSE_CSE(("clear notation for a new Compilation Continuation "));
    ABORT_CSE_TRACKING;
    CompilationContinuation::begin_compile(JVM_SINGLE_ARG_CHECK_0);
  }

  const bool finished = compile_bytecodes(JVM_SINGLE_ARG_CHECK_0);
  if (finished) {
    end_compile();
  } else {
    set_is_suspended(true);
  }
  return finished;
}


void OSRStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(osr_stub);


  COMPILER_COMMENT(("OSR bci = %d: Restore register mapping and continue in "
                   "compiled code", bci()));

  CodeGenerator* gen = Compiler::code_generator();
  emit_osr_entry_and_callinfo(gen JVM_CHECK);

  VirtualStackFrame* frame = Compiler::current()->frame();
  VirtualStackFrame* new_frame = frame->clone(JVM_SINGLE_ARG_ZCHECK(new_frame));
  new_frame->mark_as_flushed();
  new_frame->set_real_stack_pointer( frame->virtual_stack_pointer() );

  // make the new frame the current one
  Compiler::current()->set_frame( new_frame );

#ifndef USE_COMPILER_COMMENTS
  if (GenerateCompilerComments) {
    // frame.dump() is potentially quite expensive, so we'll avoid
    // doing it here unless we want to generate compiler comments.
    COMPILER_COMMENT(("Stack before merge"));
    new_frame().dump(true);
  }
#endif

  COMPILER_COMMENT(("Merging"));
  new_frame->conform_to( frame );

  BinaryAssembler::Label entry = entry_label();
  gen->jmp(entry);
}

void CompilationQueueElement::generic_compile(address addr JVM_TRAPS) {
  generic_compile(addr, Assembler::no_reg JVM_CHECK);
}

void CompilationQueueElement::generic_compile(address addr,
                                              const Assembler::Register reg_arg
                                              JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(generic_compile);

  CodeGenerator* gen = Compiler::code_generator();
  VirtualStackFrame* frame = Compiler::current()->frame();

  BinaryAssembler::Label stub = entry_label();
  gen->bind(stub);
  { 
    PreserveVirtualStackFrameState state(frame JVM_CHECK);
    if (reg_arg != Assembler::no_reg) {
      gen->call_vm_extra_arg(reg_arg);
    }
    gen->call_vm(addr, T_VOID JVM_CHECK);
  }

  BinaryAssembler::Label returner = return_label();
  gen->jmp(returner);
}

void CompilationQueueElement::custom_compile(custom_compile_func func
                                             JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(custom_compile);

  CodeGenerator* gen = Compiler::code_generator();
  VirtualStackFrame* frame = Compiler::current()->frame();

  BinaryAssembler::Label stub = entry_label();
  gen->bind(stub);

  Assembler::Register result_register = register_0();
  if (result_register == Assembler::no_reg) {
    PreserveVirtualStackFrameState state(frame JVM_CHECK);
    (gen->*func)(this JVM_CHECK);
  } else {
    {
      PreserveVirtualStackFrameState state(frame JVM_CHECK);
      (gen->*func)(this JVM_CHECK);
      if (result_register != BinaryAssembler::return_register) {
        gen->move(result_register, BinaryAssembler::return_register);
      }

      // Add a reference to prevent result_register from being reused during 
      // the VSF merge.
      RegisterAllocator::reference(result_register);
    }
    RegisterAllocator::dereference(result_register);
  }

  BinaryAssembler::Label returner = return_label();
  gen->jmp(returner);
}

void StackOverflowStub::compile(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(stack_overflow_stub);

  CodeGenerator* gen = Compiler::code_generator();
  COMPILER_COMMENT(("Stack overflow check failed"));

  BinaryAssembler::Label stub = entry_label();
  gen->bind(stub);
  // pass stack_pointer and method
  gen->overflow(register_0(), register_1());  // ain't never returning here...
}

QuickCatchStub* QuickCatchStub::allocate( const jint bci,
                                   const Value& value, const jint handler_bci,
                                   const BinaryAssembler::Label entry_label
                                   JVM_TRAPS) {
  QuickCatchStub* stub = (QuickCatchStub*)
    CompilationQueueElement::allocate(quick_catch_stub, bci JVM_ZCHECK_0(stub));
  stub->frame()->push(value);
  stub->set_info(handler_bci);
  stub->set_entry_label(entry_label);
  return stub;
}

void QuickCatchStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_COMMENT(("Quick exception catch"));

  const int handler_bci = info();
  BinaryAssembler::Label stub = entry_label();
  Compiler::code_generator()->bind(stub);

#ifdef AZZERT
  GUARANTEE( frame()->virtual_stack_pointer() ==
             Compiler::root()->method()->max_locals(),
             "must have exactly one stack item");
#endif

  BinaryAssembler::Label branch_label;
  Compiler::code_generator()->jmp(branch_label);
  CompilationContinuation::insert(handler_bci, branch_label
                                  JVM_NO_CHECK_AT_BOTTOM);
}

void TimerTickStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(timer_tick_stub);
  COMPILER_COMMENT(("Timer Tick"));

#if ENABLE_CODE_PATCHING
  const int instr_offset = info();
  if (instr_offset >= 0) {
    CodeGenerator* gen = Compiler::code_generator();
    gen->emit_checkpoint_info_record(instr_offset, 
        gen->code_size() - instr_offset);
  }
#endif

  generic_compile((address) timer_tick JVM_NO_CHECK_AT_BOTTOM);
}

void InstanceOfStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(instance_of_stub);
  COMPILER_COMMENT(("instance_of stub"));

  custom_compile(&CodeGenerator::instance_of_stub JVM_NO_CHECK_AT_BOTTOM);
}

void CheckCastStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(check_cast_stub);
  COMPILER_COMMENT(("check_cast stub"));

  custom_compile(&CodeGenerator::check_cast_stub JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_INLINE_COMPILER_STUBS
void NewObjectStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_object_stub);
  COMPILER_COMMENT(("new_object stub"));

  RegisterAllocator::reference(java_near());
  custom_compile(&CodeGenerator::new_object_stub JVM_NO_CHECK);
  RegisterAllocator::dereference(java_near());
}

void NewTypeArrayStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_type_array_stub);
  COMPILER_COMMENT(("new_type_array stub"));

  RegisterAllocator::reference(java_near());
  RegisterAllocator::reference(length());
  custom_compile(&CodeGenerator::new_type_array_stub JVM_NO_CHECK);
  RegisterAllocator::dereference(length());
  RegisterAllocator::dereference(java_near());
}
#endif // ENABLE_INLINE_COMPILER_STUBS

void TypeCheckStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(type_check_stub);
  COMPILER_COMMENT(("Ask the VM to check the types"));

  generic_compile((address) array_store_type_check JVM_NO_CHECK_AT_BOTTOM);
}

ThrowExceptionStub* ThrowExceptionStub::allocate(const RuntimeException rte,
                                                 const jint bci JVM_TRAPS) {
  ThrowExceptionStub* stub = (ThrowExceptionStub*)
    CompilationQueueElement::allocate(throw_exception_stub, bci JVM_NO_CHECK);
  if( stub ) {
    stub->set_rte( rte );
  }
  return stub;
}

ThrowExceptionStub*
ThrowExceptionStub::allocate_or_share(const RuntimeException rte JVM_TRAPS) {
  Compiler * const compiler = Compiler::root();
  const jint bci = compiler->compiler_bci();
  bool set_rte_handler = false;
  if( ShareExceptionStubs && 
      compiler->method_aborted_for_exception_at(bci)) {
    ThrowExceptionStub* stub = Compiler::rte_handler(rte);
    if( stub ) { 
      return stub;
    } else { 
      set_rte_handler = true;
    }
  }

  ThrowExceptionStub* stub = allocate(rte, bci JVM_NO_CHECK );
  if( stub ) {
    compiler->insert_compilation_queue_element(stub);
    if( set_rte_handler ) { 
      stub->set_is_persistent();
      Compiler::set_rte_handler( rte, stub );
    }
  }
  return stub;
}

// (a) A runtime-exception *uncaught* by the current method can be thrown 
//     in 3 ways:
//     [1] Thrown inline -- this is platform-specific code. For example, ARM
//         can generate the following sequence in-line for a NULL check
//             cmp r0, #0
//             ldreq pc, [r5, #compiler_throw_NullPointerException_3]
//         Throwing inline avoids the use of ThrowExceptionStub so
//         it's preferred.  But some platforms (such as
//         THUMB_COMPILER) may not do it because it may cause lower
//         run-time speed.
//     [2] Codegenerator::throw_simple_exception().
//     [3] ThrowExceptionStub::Use exception_thrower().
//
// [3] Must be used if the method may have a stack lock. Otherwise
// [2] and/or [1] can be used in the other
//
// (b) A runtime-exception *caught* by the current method must be
//     handled in ThrowExceptionStub::compile().
void ThrowExceptionStub::compile(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(throw_exception_stub);

  CodeGenerator* gen = Compiler::code_generator();
  VirtualStackFrame* frame = Compiler::frame();

  // We don't support inlining of methods with exception handlers,
  // so only root method can have exception handlers
  UsingFastOops fast_oops;
  Method::Fast method = Compiler::root()->method();
  const int current_bci = Compiler::root()->compiler_bci();

  Value exception(T_OBJECT);
  const int handler_bci = method().exception_handler_bci_for(
                      exception_class(get_rte()), current_bci JVM_CHECK);
  BinaryAssembler::Label stub = entry_label();

#if ENABLE_NPCE
  if (get_rte() == rte_null_pointer ) {
#if ENABLE_CODE_OPTIMIZER
    //emit record in relocation stream
    //the record item is
    //first short record stub_offset
    //second short record  ldr_offset
    gen->emit_null_point_callback_record(stub, 
                    ((NullCheckStub*)this)->offset_of_second_instr_in_words());
#else
    gen->emit_null_point_callback_record(stub);
#endif
  } else {
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  if (get_rte() == rte_array_index_out_of_bounds) {
       if(is_persistent()) {
                Compiler::current()->code_generator()->
               emit_pre_load_item(0);
         }
  }
#endif
    gen->bind(stub);
#if ENABLE_NPCE
  }
#endif

  // We need to remember the bound value if "this" is persistent.
  set_entry_label(stub);
  
  // Discard stack elements.  Don't generate any code
  frame->clear_stack();

#ifndef USE_COMPILER_COMMENTS
  if (GenerateCompilerComments) {
    char buffer[200];
    Symbol::Raw exception_name = klass().name();
    exception_name().string_copy(buffer, sizeof(buffer));
    bool has_monitors = method().access_flags().is_synchronized() ||
                        method().access_flags().has_monitor_bytecodes();
    jvm_sprintf(buffer + jvm_strlen(buffer), 
                " bci=%d, max_locals=%d, handler=%d%s", 
                current_bci, method().max_locals(), handler_bci, 
                (has_monitors ? ", hasmonitors" : ""));
    COMPILER_COMMENT((buffer));
    frame->dump(true);
  }
#endif

  if (_debugger_active) {
    // deoptimize and let interpreter code handle it
    frame->flush(JVM_SINGLE_ARG_CHECK);
    // Do it the hard way.
    gen->call_vm(exception_thrower(), T_VOID JVM_CHECK);
    return;
  }
  if (handler_bci != -1) { 
    // We know the target, so we can just fix the stack and then goto it
    if (method().bytecode_at(handler_bci) == Bytecodes::_pop) {
      // IMPL_NOTE: isn't a frame->flush() needed here?

      // This happens often enough to optimize for it.  Don't even bother
      // building an exception object if we're just going to throw it away!
      Oop::Raw null_obj;
      exception.set_obj(&null_obj);
    } else {
      frame->flush(JVM_SINGLE_ARG_CHECK);
      gen->call_vm(exception_allocator(get_rte()), T_OBJECT JVM_CHECK);       
      exception.set_register(
              RegisterAllocator::allocate(Assembler::return_register));
    }
    frame->push(exception);

    // For now we don't support inlining of methods with exception handlers,
    // so only root method can have a handler
    const Entry* entry = Compiler::root()->entry_for(handler_bci);
    if( entry && entry->frame()->is_conformant_to(frame) ) {
      // The exception handler has already been compiled and has the same
      // VSF as here, so we can just branch to it. No need to create
      // CompilationContinuation.
      BinaryAssembler::Label branch_label = entry->label();
      COMPILER_COMMENT(("Conformant frames -> direct branch.")); 
      gen->jmp(branch_label);
    } else {
      BinaryAssembler::Label branch_label;
      // IMPL_NOTE:  This branch is always to the next instruction.
      // Can we always be guaranteed that the compilation continuation gets
      // inserted at the front of the queue and immediately follows?
      gen->jmp(branch_label);
      CompilationContinuation::insert(Compiler::root(), 
                                      handler_bci, branch_label JVM_CHECK);
    }
  } else {
    const bool has_monitors = method().access_flags().is_synchronized() ||
                        method().access_flags().has_monitor_bytecodes();
    // Generate code. . . .
    if (!has_monitors) {
      // We are immediately returning from this method
      gen->throw_simple_exception(get_rte() JVM_NO_CHECK_AT_BOTTOM);
    } else {
      // There may be monitors.  Do it the hard way.
      frame->flush(JVM_SINGLE_ARG_CHECK);
      gen->call_vm(exception_thrower(), T_VOID JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}

address ThrowExceptionStub::exception_thrower( void ) const {
  static const address addresses[number_of_runtime_exceptions] = {
    (address)null_pointer_exception,
    (address)array_index_out_of_bounds_exception,
    (address)illegal_monitor_state_exception,
    (address)division_by_zero_exception,
    (address)get_incompatible_class_change_error
  };
  return addresses[get_rte()];
}

address ThrowExceptionStub::exception_allocator(int rte) {
  static const address addresses[number_of_runtime_exceptions] = {
    (address)get_null_pointer_exception,
    (address)get_array_index_out_of_bounds_exception,
    (address)get_illegal_monitor_state_exception,
    (address)get_division_by_zero_exception,
    (address)get_incompatible_class_change_error
  };
  return addresses[rte];
}

ReturnOop ThrowExceptionStub::exception_class(int rte) {
#ifdef AZZERT
  const int n = Universe::null_pointer_exception_class_index;
  GUARANTEE(Universe::array_index_out_of_bounds_exception_class_index - n ==
            rte_array_index_out_of_bounds, "sanity");
  GUARANTEE(Universe::illegal_monitor_state_exception_class_index - n == 
            rte_illegal_monitor_state, "sanity");
  GUARANTEE(Universe::arithmetic_exception_class_index - n ==
            rte_division_by_zero, "sanity");
  GUARANTEE(Universe::incompatible_class_change_error_class_index - n ==
            rte_incompatible_class_change, "sanity");

  GUARANTEE(rte >= rte_null_pointer &&
            rte <= rte_incompatible_class_change, "sanity");
#endif

  const int index = Universe::null_pointer_exception_class_index + rte;
  return (ReturnOop) persistent_handles[index];
}

#ifndef PRODUCT
#if 0
void CompilationQueueElement::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  iterate_oopmaps(BasicOop::iterate_one_oopmap_entry, (void*)visitor);
#endif
}

void CompilationQueueElement::iterate_oopmaps(oopmaps_doer do_map, void *param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT,  next);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT,  frame);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     type);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     bci);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     entry_label);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     return_label);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     register_0);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     register_1);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     info);
  OOPMAP_ENTRY_4(do_map, param, T_INT,     code_size_before);
  OOPMAP_ENTRY_4(do_map, param, T_BOOLEAN, is_suspended);
  OOPMAP_ENTRY_4(do_map, param, T_BOOLEAN, entry_has_been_bound);
#endif
}
#endif

#endif // PRODUCT


#ifdef ABORT_CSE_TRACKING
#undef ABORT_CSE_TRACKING
#endif
#endif // ENABLE_COMPILER
