/*
 *   
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
#include "incls/_CodeGenerator_i386.cpp.incl"

#if ENABLE_COMPILER
#if ENABLE_ISOLATES

// Load task mirror, perform class initialization before hand if needed.
// Barrier is not necessary when the class that holds the static variables:
//  - is a ROMIZED class,
//  - has no static initializer
//  - has a static initializer but it does not invoke any methods (therefore
//  no compiled method can see the class in the being initialized state)
//  Methods that are used in static initializers and that may hit a barrier
//  may also be tagged so that only tagged methods have to clear a barrier.
//  This however is harder to achieve.
//
//
void CodeGenerator::load_task_mirror(Oop*klass, Value& statics_holder,
                                     bool needs_cib JVM_TRAPS){
  {
    JavaClass::Raw jc = klass;
    statics_holder.assign_register();
    movl(statics_holder.lo_register(), Address((int)&_mirror_list_base));
    movl(statics_holder.lo_register(), 
         Address(statics_holder.lo_register(),
                 (int)jc().class_id() * sizeof(OopDesc *)));
  }
  if (needs_cib){
    Label class_is_initialized, need_init;
    // Can we make the flush conditional for  get/put static ?
    //  see if register usage cross compiled bytecode.
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    // The marker is at the start of the heap or in ROM text, so it can be
    // treated as a constant value for the cib test.
    cmpl(statics_holder.lo_register(), (int)_task_class_init_marker);
    jcc(not_equal, class_is_initialized);
bind(need_init);
    // Call the runtime system.
    // pass klass as extra args, move in correct register beforehand
    // if necessary
    // Passing the klass in parameter.
    {
      // KEEP these brackets: without them the klass_parameter's destructor would
      // not be called before call_vm and cause an error.
      Value klass_parameter(T_OBJECT);
      klass_parameter.set_obj(klass);
      if (klass_parameter.lo_register() != edx) {
        call_vm_extra_arg(klass_parameter.lo_register());
      }
    }
    call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
    // Need to move mirror to expected register
    if (statics_holder.lo_register()!= eax){
      movl(statics_holder.lo_register(), eax);
    }
    bind(class_is_initialized);
  }
}

// Class initialization barrier when the mirror is not needed
// (i.e., for bytecode instruction new and invokestatic)
void CodeGenerator::check_cib(Oop* klass JVM_TRAPS){
  Label class_is_initialized, need_init;
  // IMPL_NOTE: Cannot make the flush conditionally.
  //  see how this can be made conditional!
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  
  { // Scope for task_mirror_value. Use destroy explicitly if you remove this.
    Value task_mirror_value(T_OBJECT);
  {
    JavaClass::Raw jc = klass;
    task_mirror_value.assign_register();
    movl(task_mirror_value.lo_register(), Address((int)&_mirror_list_base));
    movl(task_mirror_value.lo_register(), 
         Address(task_mirror_value.lo_register(),
                 (int)jc().class_id() * sizeof(OopDesc*)));
  }

  {
    Value klass_value(T_OBJECT);
    klass_value.set_obj(klass);
    // The marker is at the start of the heap or in ROM text, so it can be
    // treated as a constant value for the cib test.
    cmpl(task_mirror_value.lo_register(), (jint)_task_class_init_marker);
    jcc(not_equal, class_is_initialized);
bind(need_init);
    // Call the runtime system.
    // pass klass as extra args (obtain class by substracting the offset again)
    if (klass_value.lo_register() != edx){
      call_vm_extra_arg(klass_value.lo_register());
    }
  }
  }
  call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
  bind(class_is_initialized);
}

#endif //ENABLE_ISOLATES

#if ENABLE_INLINED_ARRAYCOPY
bool CodeGenerator::arraycopy(JVM_SINGLE_ARG_TRAPS) { 
  return false;
}

bool CodeGenerator::unchecked_arraycopy(BasicType type JVM_TRAPS) {
  return false;
}
#endif

void CodeGenerator::bytecode_prolog() {
}

void CodeGenerator::flush_epilogue(JVM_SINGLE_ARG_TRAPS) {
}

void CodeGenerator::load_from_address(Value& result, BasicType type,
                                      MemoryAddress& address, Condition cond) {
  // illegal types do not require any loading
  if (type == T_ILLEGAL) return;

  GUARANTEE(stack_type_for(type) == result.stack_type(),
            "types must match (taking stack types into account)");
  result.assign_register();
  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
      movsxb (result.lo_register(), address.lo_address());
      break;

    case T_CHAR    :
      movzxw (result.lo_register(), address.lo_address());
      break;

    case T_SHORT   :
      movsxw (result.lo_register(), address.lo_address());
      break;

    case T_INT     : // fall through
    case T_ARRAY   : // fall through
    case T_OBJECT  :
      movl   (result.lo_register(), address.lo_address());
      break;

    case T_LONG    :
      movl   (result.lo_register(), address.lo_address());
      movl   (result.hi_register(), address.hi_address());
      break;

#if ENABLE_FLOAT
    case T_FLOAT   :
      fld_s  (result.lo_register(), address.lo_address());
       break;

    case T_DOUBLE  :
      {
        BinaryAssembler::Address lo = address.lo_address();
        BinaryAssembler::Address hi = address.hi_address();
        if (hi._disp == lo._disp + 4) {
          fld_d  (result.lo_register(), address.lo_address());
        } else {
          if (lo._base == esp) {
            lo._disp += 4;
          }
          pushl(hi);
          pushl(lo);
          fld_d (result.lo_register(), Address(esp));
          addl(esp, 8);
        }
      }
      break;

#endif
    default        :
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::store_to_address(Value& value, BasicType type,
                                     MemoryAddress& address) {
  // if the value to store isn't present there nothing left to do
  if (!value.is_present()) return;

  GUARANTEE(stack_type_for(type) == value.stack_type(),
  "types must match (taking stack types into account)");
  if (value.is_immediate()) {
    switch(type) {
      case T_BOOLEAN : // fall through
      case T_BYTE    :
        movb(address.lo_address(), value.lo_bits());
        break;

      case T_CHAR    : // fall through
      case T_SHORT   :
        movw(address.lo_address(), value.lo_bits());
        break;

      case T_ARRAY   :
      case T_OBJECT  :
        GUARANTEE(value.must_be_null(), "Must be null");
        // fall through.  Don't need bit setting for NULL

      case T_FLOAT   : // fall through
      case T_INT     :
        movl(address.lo_address(), value.lo_bits());
        break;

      case T_DOUBLE  : // fall through
      case T_LONG    :
        movl(address.lo_address(), value.lo_bits());
        movl(address.hi_address(), value.hi_bits());
        break;

      default        : SHOULD_NOT_REACH_HERE();
                       break;
    }
    return;
  }

  GUARANTEE(value.in_register(), "only case left");
  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
      value.force_to_byte_register();
      movb(address.lo_address(), value.lo_register());
      break;

    case T_CHAR    : // fall through
    case T_SHORT   :
      movw(address.lo_address(), value.lo_register());
      break;

    case T_ARRAY   : // fall through
    case T_OBJECT  :
      if (!value.not_on_heap()) {
        // value >>may<< be a heap object.  Need to do pointer setting
        address.write_barrier_prolog();
        movl(address.lo_address(), value.lo_register());
        address.write_barrier_epilog();
        break;
      } // else fall through.  No write barrier needed for NULL

    case T_INT     :
      movl(address.lo_address(), value.lo_register());
      break;

    case T_LONG    :
      movl(address.lo_address(), value.lo_register());
      movl(address.hi_address(), value.hi_register());
      break;

#if ENABLE_FLOAT
    case T_FLOAT   :
      fpu_prepare_unary(value);
      fstp_s(address.lo_address(), value.lo_register());
      break;

    case T_DOUBLE  :
      {
        fpu_prepare_unary(value);
        BinaryAssembler::Address lo = address.lo_address();
        BinaryAssembler::Address hi = address.hi_address();
        if (hi._disp == lo._disp + 4) {
          fstp_d(lo, value.lo_register());
        } else {
          subl(esp, 8);
          fstp_d(Address(esp), value.lo_register());
          if (lo._base == esp) {
            lo._disp += 4;
          }
          popl(lo);
          popl(hi);
        }
      }
      break;

#endif
    default        :
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::store_tag_to_address(BasicType type, StackAddress& address){
  GUARANTEE(TaggedJavaStack, "Sanity");
  movl(address.tag_address(), ::basic_type2tag(type));
  // The tag for the second word of a long/double is always twice the
  // basic tag.
  if (::is_two_word(type)) {
    movl(address.tag2_address(), ::basic_type2tag(type) * 2);
  }
}

void CodeGenerator::clear_object_location(jint index) {
  LocationAddress address(index, T_OBJECT);
  movl(address.lo_address(), 0);
}

void CodeGenerator::overflow(const Assembler::Register& stack_pointer,
                             const Assembler::Register& method_pointer) {

  GUARANTEE(method_pointer == Assembler::ebx, "Method pointer in wrong register");
  GUARANTEE(stack_pointer == Assembler::edx, "Stack pointer in wrong register");
  jmp((address)interpreter_method_entry);
}

void CodeGenerator::method_entry(Method* method JVM_TRAPS) {
  movb( Address( unsigned(_method_execution_sensor) ), 0 );


  // let's see if this method will fit on this stack
  bool need_stack_overflow_check = true;
  if (method->is_native() || !method->access_flags().has_invoke_bytecodes()) {
    int stack_needed = JavaFrame::frame_desc_size() - 2*BytesPerWord +
      (method->max_execution_stack_count() * BytesPerStackElement);
    if ((stack_needed) < LeafMethodStackPadding) {
      // We're sure this method won't cause a stack overflow.
      need_stack_overflow_check = false;
    }
  }
  if (need_stack_overflow_check) {
    check_stack_overflow(method JVM_CHECK);
  }
  const int extra_locals = method->max_locals() - method->size_of_parameters();
  if (extra_locals >= 1) {
    comment("Pop the return address");
    popl(eax);
    comment("Make room for locals");
    subl(esp, BytesPerStackElement * extra_locals);

    comment("Push the return address again");
    pushl(eax);
  }

#if NOT_CURRENTLY_USED
  // Consider whether it's really needed.
  // IMPL_NOTE: Profiler can't work well with this line. 
  if (UseProfiler) {
    comment("Store compiled method pointer in new frame");
    // ...and do this before ebp is updated so the profiler won't get confused.
    movl(Address(esp, -2 * BytesPerWord), compiled_method());
  }
#endif

  comment("Setup frame link");
  pushl(ebp);
  movl(ebp, esp);

  comment("Make room for frame descriptor");
  subl(esp, JavaFrame::frame_desc_size() - 2*BytesPerWord);

  if (method->access_flags().is_synchronized()) {
    if (method->access_flags().is_static()) {
      comment("Static method. Synchronize on the class");

      // Get the class mirror object.
#if ENABLE_ISOLATES
      UsingFastOops fast_oops;
      InstanceClass::Fast klass = method->holder();
      Value klass_value(T_OBJECT);
      klass_value.set_obj(&klass);

      if (StopAtRealMirrorAccess){
          breakpoint();
      }
      load_task_mirror(&klass, klass_value, true JVM_CHECK);
      // Now load the real mirror
      movl(eax, Address(klass_value.lo_register(),
                        TaskMirror::real_java_mirror_offset()));
#else
      JavaClass::Raw klass = method->holder();
      Instance::Raw mirror = klass().java_mirror();
      comment("Static method. Synchronize on the class mirror object");
      movl(eax, &mirror);
#endif
    } else {
      comment("Non-static method. Synchronize on the receiver");
      LocationAddress obj(0, T_OBJECT);
      movl(eax, obj.lo_address());
    }
    comment("Make room for the stack lock and setup the stack bottom pointer");
    subl(esp, BytesPerWord + StackLock::size());
    movl(Address(ebp, JavaFrame::stack_bottom_pointer_offset()), esp);
    call_from_compiled_code((address)shared_lock_synchronized_method
                            JVM_NO_CHECK_AT_BOTTOM);
  } else {
    if (method->access_flags().has_monitor_bytecodes()) {
      // Method isn't synchronized, but it has monitor bytecodes.
      comment("Setup stack bottom pointer");
      movl(Address(ebp, JavaFrame::stack_bottom_pointer_offset()), esp);
    }
  }

  // IMPL_NOTE: add support for method entry/exit event later...
  if (_debugger_active) {
    // Let's see if this thread is stepping, if so call handle_single_step
    Label not_stepping;
    get_thread(eax);
    testl(Address(eax, Thread::status_offset()), THREAD_STEPPING);
    jcc(zero, not_stepping);
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    // Call the runtime system.
    call_vm((address) handle_single_step, T_VOID JVM_CHECK);
    bind(not_stepping);
  }

#if ENABLE_WTK_PROFILER
  // Generate this always since profiler can be enabled dynamically
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif

#if ENABLE_PROFILER
  check_timer_tick(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
#endif

#if defined(AZZERT) && 0
  {
    // Uncomment this to embed the name of the method into the compiled
    // code. IMPL_NOTE: make this a Globals.hpp option.
    Label done_comments;
    jmp(done_comments);
    // Do a few nops to avoid upsetting disassemblers
    emit_byte(0x90); emit_byte(0x90); emit_byte(0x90); emit_byte(0x90);
    int i, len;
    InstanceClass::Raw klass = method->holder();
    Symbol::Raw name = klass().name();

    for (i=0,len=name().length(); i<len; i++) {
      emit_byte(name().byte_at(i));
    }

    emit_byte('.');
    name = method->name();
    for (i=0,len=name().length(); i<len; i++) {
      emit_byte(name().byte_at(i));
    }
    emit_byte(':');

    Symbol::Raw sig = method->signature();
    for (i=0,len=sig().length(); i<len; i++) {
      emit_byte(sig().byte_at(i));
    }

    // Do a few nops to avoid upsetting disassemblers
    emit_byte(0x90); emit_byte(0x90); emit_byte(0x90); emit_byte(0x90);
    emit_byte(0x90); emit_byte(0x90); emit_byte(0x90); emit_byte(0x90);
    emit_byte(0x90); emit_byte(0x90); emit_byte(0x90); emit_byte(0x90);
    emit_byte(0x90); emit_byte(0x90); emit_byte(0x90); emit_byte(0x90);
    bind(done_comments);
  }
#endif
}

void CodeGenerator::call_from_compiled_code(Register reg, int offset,
                                            int parameters_size JVM_TRAPS) {
  if (offset != 0) {
    addl(reg, offset);
  }
  // call the code
  call(reg);
  write_call_info(parameters_size JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::call_from_compiled_code(address target,
                                            int parameters_size JVM_TRAPS) {
  // call the code
  call(target);
  write_call_info(parameters_size JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::write_call_info(int parameters_size JVM_TRAPS) {
  // The actual callinfo on the x86 starts >>after<< the bytecode which
  // encodes "testl eax, ..."
  const jint code_offset = code_size();
#if ENABLE_EMBEDDED_CALLINFO
  GUARANTEE(!Compiler::is_inlining(), 
            "Not tested: need to write root bci in the callinfo");
  const jint callinfo_start = code_offset + 1;
  comment("Embedded call information");
  if (CallInfo::fits_compiled_compact_format(bci(),
                                             callinfo_start,
                                             frame()->virtual_stack_pointer() + 1)
        && frame()->fits_compiled_compact_format()) {
    CallInfo info = CallInfo::compiled_compact(bci(), callinfo_start);
    frame()->fill_in_tags(info, parameters_size);
    if (PrintCompiledCodeAsYouGo) {
      info.print(tty);
    }
    testl(eax, info.raw());
  } else {
    UsingFastOops fast_oops;
    TypeArray::Fast extended_tag_info =
        frame()->generate_callinfo_stackmap(JVM_SINGLE_ARG_CHECK);

    CallInfo info = CallInfo::compiled(bci(), callinfo_start JVM_CHECK);
    if (PrintCompiledCodeAsYouGo) {
      info.print(tty);
    }
    testl(eax, info.raw());
    for (int i = 0; i < extended_tag_info().length(); i++) {
      testl(eax, extended_tag_info().int_at(i));
    }
  }
#endif // ENABLE_EMBEDDED_CALLINFO

#if ENABLE_APPENDED_CALLINFO
  append_callinfo_record(code_offset JVM_NO_CHECK_AT_BOTTOM);
#endif
  (void)parameters_size;
}

void CodeGenerator::increment_stack_pointer_by(int adjustment) {
  if (adjustment != 0) {
    // adjust the stack pointer without affecting the CPU flags
    leal(esp, Address(esp, -BytesPerStackElement * adjustment));
  }
}

void CodeGenerator::clear_stack() {
  // if the method is synchronized or has monitor bytecodes the
  // stack bottom pointer in the frame descriptor is filled in
  if (method()->access_flags().is_synchronized() || method()->access_flags().has_monitor_bytecodes()) {
    movl(esp, Address(ebp, JavaFrame::stack_bottom_pointer_offset()));
  } else {
    leal(esp, Address(ebp, JavaFrame::stack_bottom_pointer_offset()));
  }
}

void
CodeGenerator::move(const Value& dst, const Value& src, const Condition cond) {
  GUARANTEE(cond == always, "Conditional moves not supported on i86");
  // if the source isn't present there's nothing left to do
  if (!src.is_present()) return;

  GUARANTEE(dst.type() == src.type(), "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");

  if (src.is_immediate()) {
    switch(dst.type()) {
      case T_OBJECT :
      case T_ARRAY  :
        GUARANTEE(src.must_be_null(), "Must be null");

      case T_INT    :
        if (src.as_int() == 0) {
          xorl(dst.lo_register(), dst.lo_register());
        } else {
          movl(dst.lo_register(), src.as_int());
        }
        break;

      case T_LONG   :
        movl(dst.lo_register(), lsw(src.as_long()));
        movl(dst.hi_register(), msw(src.as_long()));
        break;
#if ENABLE_FLOAT
      case T_FLOAT  :
        fpu_load_constant(dst, src, src.as_float());
        break;

      case T_DOUBLE:
        fpu_load_constant(dst, src, src.as_double());
        break;
#endif
      default       : SHOULD_NOT_REACH_HERE(); break;
    }
  } else {
    GUARANTEE(src.in_register(), "source must be in register");
    switch (dst.type()) {
      case T_INT    : // fall-through
      case T_ARRAY  :
      case T_OBJECT : movl(dst.lo_register(), src.lo_register());
                      break;

      case T_LONG   : movl(dst.lo_register(), src.lo_register());
                      movl(dst.hi_register(), src.hi_register());
                      break;
#if ENABLE_FLOAT

      case T_FLOAT  : // fall-through
      case T_DOUBLE : fld(dst.lo_register(), src.lo_register());
                      break;
#endif
      default       : SHOULD_NOT_REACH_HERE(); break;
    }
  }
}

#if ENABLE_FLOAT

void CodeGenerator::fpu_load_constant(const Value& dst, const Value& src,
                                                        const double value) {
  if (value == 0.0) {
    fldz(dst.lo_register());
  } else if (value == 1.0) {
    fld1(dst.lo_register());
  } else if (dst.type() == T_FLOAT) {
    pushl(src.as_raw_int());
    fld_s(dst.lo_register(), Address(esp));
    addl(esp, 4);
  } else {
     jlong l = src.as_raw_long();
     pushl(msw(l));
     pushl(lsw(l));
     fld_d(dst.lo_register(), Address(esp));
     addl(esp, 8);
  }
}
#endif

void CodeGenerator::move(Value& dst, Oop* obj, Condition cond) {
  GUARANTEE(cond == always, "Conditional moves not supported on i86");
  GUARANTEE(dst.type() == T_OBJECT || dst.type() == T_ARRAY, "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");

  movl(dst.lo_register(), obj);
}

void CodeGenerator::move(Assembler::Register dst, Assembler::Register src,
                         Condition cond) {
  GUARANTEE(cond == always, "Conditional moves not supported on i86");
  movl(dst, src);
}

void CodeGenerator::array_check(Value& array, Value& index JVM_TRAPS) {
  FieldAddress length_address(array, Array::length_offset(), T_INT);

  maybe_null_check(array JVM_CHECK);
  // do the comparison
  if (index.is_immediate()) {
    cmpl(length_address.lo_address(), index.as_int());
  } else {
    cmpl(length_address.lo_address(), index.lo_register());
  }

  // insert stub to handle uncommon case where the index is out of bounds
  IndexCheckStub* check_stub =
    IndexCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(check_stub));
  jcc(below_equal, check_stub);
}

void CodeGenerator::null_check( const Value& object JVM_TRAPS) {
  testl( object.lo_register(), object.lo_register() );
  NullCheckStub* check_stub = 
    NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(check_stub));
  jcc(zero, check_stub);
}

void CodeGenerator::return_error(Value& value JVM_TRAPS) {
  // This looks almost like return_void, except that we save
  // the return address in edx, and put the error in eax.
  movl(esp, ebp);
  popl(ebp);
  if (value.lo_register() != edx) {
    movl(edx, value.lo_register());
  }
#if ENABLE_FLOAT
  // Ensure x86 fpu stack is empty
  fpu_clear();
#endif
  popl(eax);    // return address
  addl(esp, BytesPerStackElement * method()->max_locals());
  pushl(eax);   // put return address back on the stack
  jmp((address)shared_call_vm_exception);
}

void CodeGenerator::return_void(JVM_SINGLE_ARG_TRAPS) {
  comment("Restore the frame and stack pointers.");
  movl(esp, ebp);
  popl(ebp);

#if ENABLE_FLOAT
  // Ensure x86 fpu stack is empty
  fpu_clear();
#endif

  comment("Return and pop the locals.");
  ret(BytesPerStackElement * method()->max_locals());
}

void CodeGenerator::return_result(Value& value JVM_TRAPS) {
  jint value_word_size = value.is_two_word() ? 2 : 1;

  // Mark all locations as flushed.
  // The reason is we don't want spilling code to appear
  // when storing the return value on the stack.
  frame()->mark_as_flushed();

  if (method()->max_locals() < value_word_size) {
    // This is the uncommon case where there is no room for
    // the result in the local area.
    Register ret_adr = RegisterAllocator::allocate();

    comment("Restore the frame and stack pointers.");
    movl(esp, ebp);
    popl(ebp);

    // Get the return address from the stack.
    popl(ret_adr);

    // Make room on stack for result
    increment_stack_pointer_by(value_word_size - method()->max_locals());

    // Compute the top of stack address
    StackAddress address(esp, value.type());

    // Store the result onto the top of stack
    store_to_address(value, value.type(), address);
    if (TaggedJavaStack) {
      store_tag_to_address(value.type(), address);
    }

#if ENABLE_FLOAT
    // Ensure x86 fpu stack is empty
    fpu_clear();
#endif

    comment("Jump to the return address.");
    jmp(ret_adr);

    // Free the return address register.
    RegisterAllocator::dereference(ret_adr);
  } else {
    // the result is stored as the first local
    LocationAddress address(0, value.type());
    store_to_address(value, value.type(), address);
    if (TaggedJavaStack) {
      store_tag_to_address(value.type(), address);
    }

    // restore frame and stack pointers
    movl(esp, ebp);
    popl(ebp);
 #if ENABLE_FLOAT
   // verify that the fpu stack is empty
    fpu_clear();
#endif
    // return and pop excessive locals
    ret(BytesPerStackElement * (method()->max_locals() - value_word_size));
  }
}

// See ThrowExceptionStub::compile in CompilationQueue.cpp about 3 ways of
// throwing runtime exceptions.
void CodeGenerator::throw_simple_exception(int rte JVM_TRAPS) {
  // Create an exception object and do a simple return from the method
  frame()->clear();
  Value exception(T_OBJECT);
  call_vm(ThrowExceptionStub::exception_allocator(rte), T_OBJECT JVM_CHECK);
  exception.set_register(
              RegisterAllocator::allocate(Assembler::return_register));
  return_error(exception JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::monitor_enter(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor enter stub.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Make sure the object is in register eax.
  if (object.lo_register() != eax) {
    movl(eax, object.lo_register());
  }

  // Call the compiler stub.
  call_from_compiled_code((address)shared_monitor_enter JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::monitor_exit(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor exit stub.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Make sure the object is in register eax.
  if (object.lo_register() != eax) {
    movl(eax, object.lo_register());
  }

  // Call the compiler stub.
  call_from_compiled_code((address)shared_monitor_exit JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::unlock_activation(JVM_SINGLE_ARG_TRAPS) {
  // BinaryAssembler::Address adr(0);

  GUARANTEE(method()->access_flags().is_synchronized(), "Sanity check");
  GUARANTEE(ROM::is_synchronized_method_allowed(method()), "sanity");

  frame()->flush(JVM_SINGLE_ARG_CHECK);
  call_from_compiled_code((address)shared_unlock_synchronized_method
                          JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::check_monitors(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_FLOAT
  // Since this code may call a stub that then returns here, we must make sure
  // that the x86 fpu stack is empty
  fpu_clear(true);
#endif

  // Make sure there are no locked monitors on the stack.
  Register first_stack_lock = RegisterAllocator::allocate();
  Register last_stack_lock  = RegisterAllocator::allocate();

  comment("Get a pointer to the first stack lock");
  leal(first_stack_lock, Address(ebp, JavaFrame::stack_bottom_pointer_offset()));

  comment("Get a pointer to the last stack lock");
  movl(last_stack_lock, Address(ebp, JavaFrame::stack_bottom_pointer_offset()));

  NearLabel loop, entry;
  UnlockExceptionStub* unlock_exception_stub = 
    UnlockExceptionStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(unlock_exception_stub));

  Label unlock_exception_done;
  jmp(entry);

  bind(loop);
  cmpl(Address(last_stack_lock, StackLock::size()), 0);
  jcc(not_equal, unlock_exception_stub);
  bind(unlock_exception_done);
  addl(last_stack_lock, (4 + StackLock::size()));

  bind(entry);
  cmpl(last_stack_lock, first_stack_lock);
  jcc(not_equal, loop);

  // Free the stack lock registers.
  RegisterAllocator::dereference(first_stack_lock);
  RegisterAllocator::dereference(last_stack_lock);

}

void CodeGenerator::cmp_values(Value& op1, Value& op2) {
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  if (op2.is_immediate()) {
    if (op2.as_int() == 0) {
      testl(op1.lo_register(), op1.lo_register());
    } else {
      cmpl(op1.lo_register(),  op2.as_int());
    }
  } else {
    cmpl(op1.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::if_then_else(Value& result,
                                 BytecodeClosure::cond_op condition, 
                                 Value& op1, Value& op2, 
                                 ExtendedValue& result_true, 
                                 ExtendedValue& result_false JVM_TRAPS) {
  NearLabel false_label, join_label;

  if (result_true.is_value() && result_true.value().is_immediate()) {
    // This can modify the condition codes
    result_true.value().materialize();
  }
  cmp_values(op1, op2);
  op1.destroy();
  op2.destroy();
  if (result_true.is_value() && result_true.value().in_register()) {
    // The test for in_register() above is at present redundant.  But
    // we may want to get rid of the materialize() above or only do it
    // for single-word items.
    result_true.value().writable_copy(result);
    jcc(convert_condition(condition), join_label);
  } else {
    result.assign_register();
    jcc(convert_condition(BytecodeClosure::negate(condition)), false_label);
    move(result, result_true);
    jmp(join_label);
  }

  bind(false_label);
    move(result, result_false);
  bind(join_label);
}


void CodeGenerator::if_iinc(Value& result, BytecodeClosure::cond_op condition,
                            Value& op1, Value& op2,
                            Value& arg, int increment JVM_TRAPS){ 
  if (arg.is_immediate()) {
    // should happen before cmp_values, because it can affect CPU flags
    arg.materialize();
  }

  cmp_values(op1, op2);
  op1.destroy(); op2.destroy();

  arg.writable_copy(result);
  NearLabel false_label;
  jcc(convert_condition(BytecodeClosure::negate(condition)), false_label);
  switch(increment) {
    case 1:   incl(result.lo_register());            break;
    case -1:  decl(result.lo_register());            break;
    default:  addl(result.lo_register(), increment); break;
  }
  bind(false_label);
}


void CodeGenerator::new_object(Value& result, JavaClass* klass JVM_TRAPS) {
  InstanceSize size = klass->instance_size();
  GUARANTEE(size.is_fixed(), "Size must be fixed in order to do allocation");

  // Do flushing, and remember to unmap.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Handle finalization by going slow-case for objects with finalizers.
  if (klass->has_finalizer()) {
    // Slow-case: Call InterpreterRuntime::newobject.
    call_vm((address)::newobject, T_OBJECT JVM_CHECK);
  } else {
    UsingFastOops fast_oops;

    // Get the prototypical near object from the klass and put it into
    // register ebx
    Oop::Fast near_object = klass->prototypical_near();
    movl(ebx, &near_object);

    // Setup edx to hold the instance size in bytes
    movl(edx, size.fixed_value());

    // Call the allocation routine.
    call_from_compiled_code((address)compiler_new_object JVM_CHECK);
  }

  // Let the result be in eax (encoding 0).
  result.set_register(RegisterAllocator::allocate(eax));
}

void CodeGenerator::new_object_array(Value& result, JavaClass* element_class,
                                     Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast array_class = element_class->get_array_class(1 JVM_CHECK);

  // Push array length.
  frame()->push(length);

  // Flush the virtual stack frame.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Get the prototypical near object from the klass and put it into register
  // ebx
  Oop::Fast near_object = array_class().prototypical_near();
  movl(ebx, &near_object);

  // Call the allocation routine.
  call_from_compiled_code((address)compiler_new_obj_array JVM_CHECK);

  // Remove length from stack.
  frame()->pop(T_INT);
  length.destroy();

  // Let the result be in eax.
  result.set_register(RegisterAllocator::allocate(eax));
}

void CodeGenerator::new_basic_array(Value& result, BasicType type,
                                    Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  // Push array length.
  frame()->push(length);

  // Do flushing, and remember to unmap.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Figure out which class to use.
  TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);

  if (length.is_immediate()) {
    if (length.as_int() >= 0) {
      // Compute the array size and store it in register ecx
      movl(ecx, ArrayDesc::allocation_size(length.as_int(),
                                           array_class->scale()));
    }
  } else {
    // Compute the array size and store it in register ecx
    leal(ecx, Address(no_reg, length.lo_register(), (ScaleFactor) jvm_log2(array_class->scale()), Array::base_offset() + 4 - 1));
    andl(ecx, ~3);
  }

  // Get the prototypical near object from the klass and put it into register
  // ebx
  Oop::Fast near_object = array_class->prototypical_near();
  movl(ebx, &near_object);

  // Call the allocation routine.
  call_from_compiled_code((address)compiler_new_type_array JVM_CHECK);

  // Remove length from stack.
  frame()->pop(T_INT);
  length.destroy();

  // Let the result be in eax.
  result.set_register(RegisterAllocator::allocate(eax));
}

void CodeGenerator::new_multi_array(Value& result JVM_TRAPS) {
  // Do flushing, and remember to unmap.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Call the runtime system.
  call_vm((address) multianewarray, T_ARRAY JVM_CHECK);

  // Let the result be in eax (encoding 0).
  result.set_register(RegisterAllocator::allocate(eax));
}

void CodeGenerator::check_cast(Value& object, Value& klass, int class_id
                               JVM_TRAPS) {
  Label slow_case, done_checking;

#if ENABLE_FLOAT
  // Since this code may call a stub that then returns here, we must make sure
  // that the x86 fpu stack is empty
  fpu_clear(true);
#endif

  comment("Typecast type check");
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all* registers
  // before checking for null object.
  Value object_class(T_OBJECT);
  object_class.assign_register();

  comment("Check for NULL object");
  testl(object.lo_register(), object.lo_register());
  jcc(zero, done_checking);

  comment("Get the class for the object");
  movl(object_class.lo_register(), Address(object.lo_register()));
  movl(object_class.lo_register(), Address(object_class.lo_register()));

  comment("Check the subtype caches");
  cmpl(Address(object_class.lo_register(), JavaClass::subtype_cache_1_offset()), klass.lo_register());
  jcc(equal, done_checking);
  cmpl(Address(object_class.lo_register(), JavaClass::subtype_cache_2_offset()), klass.lo_register());
  jcc(not_equal, slow_case);

  bind(done_checking);

  CheckCastStub::insert(bci(), class_id, slow_case, done_checking JVM_CHECK);

  frame()->pop(object);
}

void CodeGenerator::instance_of(Value& result, Value& object, Value& klass,
                                int class_id JVM_TRAPS) {
  Label slow_case, done_checking;
  result.assign_register();

#if ENABLE_FLOAT
  // Since this code may call a stub that then returns here, we must make sure
  // that the x86 fpu stack is empty
  fpu_clear(true);
#endif

  comment("Instance-of type check");
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all* registers
  // before checking for null object.
  Value object_class(T_OBJECT);
  object_class.assign_register();

  comment("Check for NULL object");
  xorl(result.lo_register(), result.lo_register());
  testl(object.lo_register(), object.lo_register());
  jcc(zero, done_checking);

  comment("Get the class for the object");
  movl(object_class.lo_register(), Address(object.lo_register()));
  movl(object_class.lo_register(), Address(object_class.lo_register()));

  // This is likely to slow it down
  // comment("Compare the classes");
  // cmpl(object_class.lo_register(), klass.lo_register());
  // jcc(equal, done_checking);

  comment("Check the subtype caches");
  incl(result.lo_register());
  cmpl(Address(object_class.lo_register(),
               JavaClass::subtype_cache_1_offset()), klass.lo_register());
  jcc(equal, done_checking);
  cmpl(Address(object_class.lo_register(),
               JavaClass::subtype_cache_2_offset()), klass.lo_register());
  jcc(not_equal, slow_case);

  bind(done_checking);

  InstanceOfStub* stub =
    InstanceOfStub::allocate( bci(), class_id, slow_case, done_checking,
                               result.lo_register() JVM_ZCHECK(stub) );
  stub->insert();
  frame()->pop(object);
}

void CodeGenerator::check_cast_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  (void)cqe;
  call_vm((address) checkcast, T_VOID JVM_NO_CHECK_AT_BOTTOM);
}


void CodeGenerator::instance_of_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  (void)cqe;
  call_vm((address) instanceof, T_INT JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_INLINE_COMPILER_STUBS
void CodeGenerator::new_object_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  (void)cqe;
  UNIMPLEMENTED();
}

void CodeGenerator::new_type_array_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  (void)cqe;
  UNIMPLEMENTED();
}
#endif

void CodeGenerator::type_check(Value& array, Value& index, Value& object JVM_TRAPS){
  Label slow_case, done_checking;

#if ENABLE_FLOAT
  // Since this code may call a stub that then returns here, we must make sure
  // that the x86 fpu stack is empty
  fpu_clear(true);
#endif

  comment("Array store type check");

  frame()->push(array);
  frame()->push(index);
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers
  // before checking for for null object.
  Value object_class(T_OBJECT);
  Value element_class(T_OBJECT);
  object_class.assign_register();
  element_class.assign_register();

  // Check for null object.
  testl(object.lo_register(), object.lo_register());
  jcc(zero, done_checking);

  // Get the class.
  movl(object_class.lo_register(), Address(object.lo_register()));
  movl(object_class.lo_register(), Address(object_class.lo_register()));

  // Get the element class of the array.
  movl(element_class.lo_register(), Address(array.lo_register()));
  movl(element_class.lo_register(), Address(element_class.lo_register()));
  movl(element_class.lo_register(), Address(element_class.lo_register(), ObjArrayClass::element_class_offset()));

  // Fast check against the subtype check caches.
  cmpl(Address(object_class.lo_register(), JavaClass::subtype_cache_1_offset()), element_class.lo_register());
  jcc(equal, done_checking);
  cmpl(Address(object_class.lo_register(), JavaClass::subtype_cache_2_offset()), element_class.lo_register());
  jcc(not_equal, slow_case);

  // Cache hit.
  bind(done_checking);

  TypeCheckStub* stub =
    TypeCheckStub::allocate( bci(), slow_case, done_checking JVM_ZCHECK(stub) );
  stub->insert();
  frame()->pop(object);
  frame()->pop(index);
  frame()->pop(array);
}

void CodeGenerator::i2b(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  if (is_valid_byte_register(value.lo_register())) {
    result.assign_register();
    movsxb(result.lo_register(), value.lo_register());
  } else {
    // Allocate a new byte register.
    result.set_register(RegisterAllocator::allocate_byte_register());
    movl(result.lo_register(), value.lo_register());
    movsxb(result.lo_register(), result.lo_register());
  }
}

void CodeGenerator::i2c(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  result.assign_register();
  movzxw(result.lo_register(), value.lo_register());
}

void CodeGenerator::i2s(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  result.assign_register();
  movsxw(result.lo_register(), value.lo_register());
}

void CodeGenerator::i2l(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  RegisterAllocator::reference(value.lo_register());
  result.set_registers(value.lo_register(), RegisterAllocator::allocate());

  movl(result.hi_register(), value.lo_register());
  sarl(result.hi_register(), 31);
}

#if ENABLE_FLOAT
void CodeGenerator::i2f(Value& result, Value& value JVM_TRAPS) {
  result.assign_register();
  if (value.is_immediate()) {
    result.set_float((float)value.as_int());
  } else {
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
//IMPL_NOTE: consider whether it should be fixed.
#if 0
    pushl(value.lo_register());
    fild_s(result.lo_register(), Address(esp));
    addl(esp, 4);
#endif
  }
}

// IMPL_NOTE: Share code with i2f
// May be a lot easier with Lars' new Values?
void CodeGenerator::i2d(Value& result, Value& value JVM_TRAPS) {
  result.assign_register();
  if (value.is_immediate()) {
    result.set_double((jdouble)value.as_int());
  } else {
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
//IMPL_NOTE: consider whether it should be fixed.
#if 0
    pushl(value.lo_register());
    fild_s(result.lo_register(), Address(esp));
    addl(esp, 4);
#endif
  }
}

void CodeGenerator::l2f(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    result.set_float((jfloat)value.as_long());
  } else {
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
//IMPL_NOTE: consider whether it should be fixed.
#if 0
    // This code is also in VirtualStackFrame!!!!!
    // Factor code
    result.assign_register();
    pushl(value.hi_register());
    pushl(value.lo_register());
    fild_d(result.lo_register(), Address(esp));
    addl(esp, 8);
#endif
  }
}

void CodeGenerator::l2d(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    result.set_double((jdouble) value.as_long());
  } else {
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);

//IMPL_NOTE: consider whether it should be fixed.
#if 0
    result.assign_register();
    pushl(value.hi_register());
    pushl(value.lo_register());
    fild_d(result.lo_register(), Address(esp));
    addl(esp, 8);
#endif
  }
}

void CodeGenerator::f2i(Value& result, Value& value JVM_TRAPS) {
  result.assign_register();
  if (value.is_immediate()) {
    if ((value.as_raw_int() & 0x7FFFFFFF) >= 0x4F000000) {
      if (g_isnan(value.as_float())) {
       result.set_int(0);
      } else if (value.as_raw_int() > 0) {
       result.set_int(MAX_INT);
      } else if (value.as_raw_int() < 0) {
       result.set_int(MIN_INT);
      }
    } else {
      result.set_int((jint) value.as_float());
    }
  } else {
  // Jump to the interpreter
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
// IMPL_NOTE: We need to fix coping of floats from the FPU to the stack
#if 0
      pushl(result.lo_register());  // Make room on stack
      fpu_prepare_unary(value);
      fistp_s(Address(esp), value.lo_register());
      popl(result.lo_register());
#endif
  }
}

void CodeGenerator::f2l(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    if ((value.as_raw_int() & 0x7FFFFFFF) >= 0x5F000000) {
      if (g_isnan(value.as_float())) {
         result.set_long(0);
       } else if (value.as_raw_int() > 0) {
         result.set_long(MAX_LONG);
       } else if (value.as_raw_int() < 0) {
         result.set_long(MIN_LONG);
       }
    } else {
    // Should possibly be as_immediate_int()?
    result.set_long((jlong)value.as_float());
    }
  } else {
  // Jump to the interpreter
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
// IMPL_NOTE: We need to fix coping of floats from the FPU to the stack
#if 0
      result.assign_register();
      pushl(result.hi_register()); // Make room on stack.
      pushl(result.lo_register());
      fpu_prepare_unary(value);
      fistp_d(Address(esp), value.lo_register());
      popl(result.lo_register());
      popl(result.hi_register());
#endif
  }
}

void CodeGenerator::f2d(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    result.set_double((jdouble) value.as_float());
  } else {
    value.copy(result);
  }
}

void CodeGenerator::d2i(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    if ((value.as_raw_long() & JVM_LL(0x7FFFFFFFFFFFFFFF))
        >= JVM_LL(0x41E0000000000000)) {
      if (g_isnan(value.as_double())) {
         result.set_int(0);
       } else if (value.as_raw_long() > 0) {
         result.set_int(MAX_INT);
       } else if (value.as_raw_long() < 0) {
         result.set_int(MIN_INT);
       }
    } else {
        result.set_int((jint) value.as_double());
    }
  } else {
    // Jump to the interpreter
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
// IMPL_NOTE: We need to fix coping of floats from the FPU to the stack
#if 0
    result.assign_register();
    // Incomplete, should do fldcw etc. here
    fpu_prepare_unary(value);
    pushl(result.lo_register());  // Make room on stack
    fistp_s(Address(esp), value.lo_register());
    popl(result.lo_register());
#endif
  }
}

void CodeGenerator::d2l(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    if ((value.as_raw_long() & JVM_LL(0x7FFFFFFFFFFFFFFF))
        >= JVM_LL(0x43E0000000000000)) {
      if (g_isnan(value.as_double())) {
         result.set_long(0);
       } else if (value.as_raw_long() > 0) {
         result.set_long(MAX_LONG);
       } else if (value.as_raw_long() < 0) {
         result.set_long(MIN_LONG);
       }
    } else {
      result.set_long((jlong) value.as_double());
    }
  } else {
    // Jump to the interpreter
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
// IMPL_NOTE: We need to fix coping of floats from the FPU to the stack
#if 0
    result.assign_register();
    pushl(result.hi_register());              // Make room on CPU stack.
    pushl(result.lo_register());
    fpu_prepare_unary(value);                      // Move value to top of FPU stack and ensure it is flushed.
    fistp_d(Address(esp), value.lo_register());    // Store top of FPU stack on CPU stack.
    popl(result.lo_register());                // Store result in assigned registers.
    popl(result.hi_register());
#endif
  }
}

void CodeGenerator::d2f(Value& result, Value& value JVM_TRAPS) {
  if (value.is_immediate()) {
    result.set_float((jfloat) value.as_double());
  } else {
    // Jump to the interpreter
    frame()->push(value);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
// IMPL_NOTE: We need to fix coping of floats from the FPU to the stack
//    value.copy(result);
  }
}
#endif

void CodeGenerator::long_cmp(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  GUARANTEE(!op1.is_immediate() || !op2.is_immediate(),
            "Immediate case handled by generic code");
  if (op1.is_immediate()) op1.materialize();
  if (op2.is_immediate()) op2.materialize();

  // Long compare for Java (semantics as described in JVM spec.)
  NearLabel L1, L2;
  Value op3(T_LONG);
  op1.writable_copy(op3);

  subl(op3.lo_register(),  op2.lo_register());
  sbbl(op3.hi_register(), op2.hi_register());
  jcc(no_overflow, L1);
  comment("If underflow happened then invert the result");
  notl(op3.hi_register());

  bind(L1);

  result.set_register(op3.hi_register());
  RegisterAllocator::reference(result.lo_register());

  orl(op3.lo_register(), op3.hi_register());
  jcc(zero, L2);
  sarl(result.lo_register(), 31 - 1);
  andl(result.lo_register(), -2);
  incl(result.lo_register());

  bind(L2);
}

#if ENABLE_FLOAT
void CodeGenerator::fpu_cmp_helper(Value& result, Value& op1, Value& op2, bool cond_is_less) {
  { // This is a somewhat simplified version of fpu_prepare_binary(), as we won't be
    // destroying the first operand.
    const FPURegisterMap& fpu_map = frame()->fpu_register_map();
    Value b(op2.type());

    // Make sure the operands are in a register...
    if (op1.is_immediate()) op1.materialize();
    if (op2.is_immediate()) op2.materialize();

    // ...and that op2 is writable, as we'll destroy it.
    op2.writable_copy(b);

    Register tos_register = fpu_map.top_of_stack_register();
    // The normal x86 FPU operations are ST(i) := ST(i) [op] ST(0), so the second operand must be TOS.
    if (tos_register != b.lo_register()) {
      // Swap TOS with b to keep operand order.
      fxch(b.lo_register());
    }

    GUARANTEE(op1.lo_register() != b.lo_register(), "Attempting to compare an FPU register with itself");

    b.copy(op2);
  }

  fucomip(op1.lo_register(), op2.lo_register());
  result.assign_register();

  Label L;
  movl(result.lo_register(), cond_is_less ? -1 : 1);
  jcc(Assembler::parity, L);  // parity is only set if the comparison was unordered
  if (!cond_is_less) {
      movl(result.lo_register(), -1);
  }
  jcc(Assembler::above , L);  // op1  < op2
  movl(result.lo_register(), 0);
  jcc(Assembler::equal , L);  // op1 == op2
  movl(result.lo_register(), 1);
  bind(L);
}

void CodeGenerator::float_cmp(Value& result, BytecodeClosure::cond_op cond, Value& op1, Value& op2 JVM_TRAPS) {
  GUARANTEE(cond == BytecodeClosure::lt || cond == BytecodeClosure::gt, "sanity check");
  bool cond_is_less = cond == BytecodeClosure::lt;

  if (op1.is_immediate() && op2.is_immediate()) {
    jfloat f1 = op1.as_float();
    jfloat f2 = op2.as_float();

    if (g_isnan(f1) || g_isnan(f2)) {
        result.set_int(cond_is_less ? -1 : 1);
    } else if (f1 < f2) {
        result.set_int(-1);
    } else if (f1 > f2) {
        result.set_int(1);
    } else {
        result.set_int(0);
    }
  } else {
    fpu_cmp_helper(result, op1, op2, cond_is_less);
  }
}

void CodeGenerator::double_cmp(Value& result, BytecodeClosure::cond_op cond, Value& op1, Value& op2 JVM_TRAPS) {
  GUARANTEE(cond == BytecodeClosure::lt || cond == BytecodeClosure::gt, "sanity check");
  bool cond_is_less = cond == BytecodeClosure::lt;

  if (op1.is_immediate() && op2.is_immediate()) {
    jdouble d1 = op1.as_double();
    jdouble d2 = op2.as_double();

    if (g_isnan(d1) || g_isnan(d2)) {
        result.set_int(cond_is_less ? -1 : 1);
    } else if (d1 < d2) {
        result.set_int(-1);
    } else if (d1 > d2) {
        result.set_int(1);
    } else {
        result.set_int(0);
    }
  } else {
    fpu_cmp_helper(result, op1, op2, cond_is_less);
  }
}

// End new infrastructure for codegenerator

void CodeGenerator::fpu_clear(bool flush_stack) {
  if (flush_stack) {
    frame()->flush_fpu();
  }
  // fpu_clear() doesn't check if fpu_map.is_clearable(), this should be done by the caller!
  FPURegisterMap& fpu_map = frame()->fpu_register_map();
  if (!fpu_map.is_empty()) {
    comment("Clear FPU stack");
    fpu_map.clear();
  }
}

void CodeGenerator::fpu_prepare_unary(Value& op) {
  GUARANTEE(op.type() == T_FLOAT || op.type() == T_DOUBLE, "T_FLOAT or T_DOUBLE expected");
  const FPURegisterMap& fpu_map = frame()->fpu_register_map();

  Value a(op.type());
  op.writable_copy(a);
  if (!fpu_map.is_top_of_stack(a.lo_register())) {
    fxch(a.lo_register());
  }
  a.copy(op);
}

void CodeGenerator::fpu_prepare_binary_arithmetic(Value& op1, Value& op2, bool& reversed) {
  GUARANTEE(op1.type() == op2.type(), "FPU type mismatch");
  GUARANTEE(op1.type() == T_FLOAT || op2.type() == T_DOUBLE, "T_FLOAT or T_DOUBLE expected");

  const FPURegisterMap& fpu_map = frame()->fpu_register_map();
  Value a(op1.type());
  Value b(op2.type());

  // Make sure the operands are in a register...
  if (op1.is_immediate()) op1.materialize();
  if (op2.is_immediate()) op2.materialize();

  // ...and are writable, as we'll be destroying both.
  op1.writable_copy(a);
  op2.writable_copy(b);

  reversed = false;

  Register tos_register = fpu_map.top_of_stack_register();
  // The normal x86 FPU operations are ST(i) := ST(i) [op] ST(0), so the second operand must be TOS.
  if (tos_register != a.lo_register() && tos_register != b.lo_register()) {
    // One of the operands must be on TOS. Swap TOS with b to keep operand order.
    fxch(b.lo_register());
  } else if (tos_register == a.lo_register()) {
    // Then b != TOS, and we've reversed the order of the operands (a is on TOS).
    reversed = true;
  }

  GUARANTEE(a.lo_register() != b.lo_register(), "Sanity check");

  a.copy(op1);
  b.copy(op2);
}

void CodeGenerator::fpu_prepare_binary_fprem(Value& op1, Value& op2) {
  GUARANTEE(op1.type() == op2.type(), "type mismatch");
  GUARANTEE(op1.type() == T_FLOAT || op1.type() == T_DOUBLE, "T_FLOAT or T_DOUBLE expected");

  const FPURegisterMap& fpu_map = frame()->fpu_register_map();

  // Make sure the operands are in a register...
  if (op2.is_immediate()) op2.materialize();
  if (op1.is_immediate()) op1.materialize();

  // ...and are writable as we'll be destroying both.
  { Value a(op1.type());
    Value b(op2.type());
    op2.writable_copy(b);
    b.copy(op2);
    op1.writable_copy(a);
    a.copy(op1);
  }

  // Make sure op2 is in second topmost register on the FPU stack
  if (fpu_map.index_for(op2.lo_register()) != 1)  {
    if (!fpu_map.is_top_of_stack(op2.lo_register())) {
      fxch(op2.lo_register());     // move op2 to top of FPU stack
    }
    fxch(fpu_map.register_for(1)); // move op2 directly beneath top of FPU stack
  }
  // Make sure op1 is on top of the FPU stack
  if (!fpu_map.is_top_of_stack(op1.lo_register())) fxch(op1.lo_register());
}

void CodeGenerator::float_binary_do(Value& result, Value& op1, Value& op2,
                                    BytecodeClosure::binary_op op JVM_TRAPS) {
  // IMPL_NOTE: Currently add, sub,mul, div, rem jumps into the interpreter we need
  // to write the compiled version of this code.
  if (op == BytecodeClosure::bin_add ||
      op == BytecodeClosure::bin_sub ||
      op == BytecodeClosure::bin_mul ||
      op == BytecodeClosure::bin_div ||
      op == BytecodeClosure::bin_rem ) {
    // Jump to the interpreter
    frame()->push(op1);
    frame()->push(op2);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
    return;
  }

#if 0
  // IMPL_NOTE: Currently bin_rem jumps into the interpreter we need
  // to write the compiled version of this code
  if (op1.is_immediate() && op2.is_immediate() && op != BytecodeClosure::bin_rem) {
    jfloat value;
    switch (op) {
    // IMPL_NOTE: div-by-zero check ???
    case BytecodeClosure::bin_add : value = op1.as_float() + op2.as_float();      break;
    case BytecodeClosure::bin_sub : value = op1.as_float() - op2.as_float();      break;
    case BytecodeClosure::bin_mul : value = op1.as_float() * op2.as_float();      break;
    case BytecodeClosure::bin_div : value = op1.as_float() / op2.as_float();      break;
    default  :                  SHOULD_NOT_REACH_HERE();                          break;
    }
    result.set_float(value);
    return;
  }

  bool reversed;
  if (op == BytecodeClosure::bin_rem) {
    fpu_prepare_binary_fprem(op1, op2);
    reversed = true; // the result will be in op2, not op1.
  } else {
    fpu_prepare_binary_arithmetic(op1, op2, reversed);
  }
// IMPL_NOTE: values are not computed correctly this may be a rounding problem
  switch (op) {
  case BytecodeClosure::bin_add :
    if (reversed) faddp(op2.lo_register(), op1.lo_register());
    else faddp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_sub :
    if (reversed) fsubrp(op2.lo_register(), op1.lo_register());
    else fsubp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_mul :
    if (reversed) fmulp(op2.lo_register(), op1.lo_register());
    else fmulp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_div :
    if (reversed) fdivrp(op2.lo_register(), op1.lo_register());
    else fdivp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_rem :
    // op1 dividend
    // op2 divisor
    fremp(op1.lo_register(), op2.lo_register());
    break;
  default  :
    SHOULD_NOT_REACH_HERE();                                    break;
  }
  if (reversed) {
    op2.copy(result);
  } else {
    op1.copy(result);
  }
#endif
}

void CodeGenerator::float_unary_do(Value& result, Value& op1,
                                   BytecodeClosure::unary_op op JVM_TRAPS) {
  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
      "Only unary op implemented");
  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  fpu_prepare_unary(op1);
  if (op == BytecodeClosure::una_neg) {
    fchs(op1.lo_register());
  } else {
    fabs(op1.lo_register());
  }
  op1.copy(result);
}

// IMPL_NOTE: Better code factoring, almost identical to Value arithmetic
extern "C" {
    double fmod(double a, double b);
}

void CodeGenerator::double_binary_do(Value& result, Value& op1, Value& op2,
                                     BytecodeClosure::binary_op op JVM_TRAPS) {
  // IMPL_NOTE: Need revisit. Currently add, bub, mul, div, rem jumps into the interpreter we need
  // to write the compiled version of this code.
  if (op == BytecodeClosure::bin_add ||
      op == BytecodeClosure::bin_sub ||
      op == BytecodeClosure::bin_mul ||
      op == BytecodeClosure::bin_div ||
      op == BytecodeClosure::bin_rem ) {
    // Jump to the interpreter
    frame()->push(op1);
    frame()->push(op2);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
    return;
  }

//  values are not always computed correctly.  The temporary fix
// is to jump to the interpreter for now.
#if 0
  if (op1.is_immediate() && op2.is_immediate()) {
    jdouble value;
    switch (op) {
    // IMPL_NOTE: div-by-zero check ???
    case BytecodeClosure::bin_add :
      value = op1.as_double() + op2.as_double();      break;
    case BytecodeClosure::bin_sub :
      value = op1.as_double() - op2.as_double();      break;
// IMPL_NOTE: we need to do strictfp for mul and div and
// infinity checking for rem
    case BytecodeClosure::bin_mul :
      value = op1.as_double() * op2.as_double();      break;
    case BytecodeClosure::bin_div :
      value = op1.as_double() / op2.as_double();      break;
    case BytecodeClosure::bin_rem :
      value = fmod(op1.as_double(), op2.as_double()); break;
    default  :
      SHOULD_NOT_REACH_HERE();                        break;
    }
    result.set_double(value);
    return;
  }

  bool reversed;
  if (op == BytecodeClosure::bin_rem) {
    fpu_prepare_binary_fprem(op1, op2);
    reversed = true; // the result will be in op2, not op1.
  } else {
    fpu_prepare_binary_arithmetic(op1, op2, reversed);
  }
  switch (op) {
  case BytecodeClosure::bin_add :
    if (reversed) faddp(op2.lo_register(), op1.lo_register());
    else faddp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_sub :
    if (reversed) fsubrp(op2.lo_register(), op1.lo_register());
    else fsubp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_mul :
    if (reversed) fmulp(op2.lo_register(), op1.lo_register());
    else fmulp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_div :
    if (reversed) fdivrp(op2.lo_register(), op1.lo_register());
    else fdivp(op1.lo_register(), op2.lo_register());
    break;
  case BytecodeClosure::bin_rem :
    fremp(op1.lo_register(), op2.lo_register());
    break;
  default  :
    SHOULD_NOT_REACH_HERE();
    break;
  }
  if (reversed) {
    op2.copy(result);
  } else {
    op1.copy(result);
  }
#endif
}

void CodeGenerator::double_unary_do(Value& result, Value& op1,
                                    BytecodeClosure::unary_op op JVM_TRAPS) {
  // Amazingly enough. . .
  float_unary_do(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
}
#endif

BinaryAssembler::generic_binary_op_1 CodeGenerator::convert_to_generic_binary_1(BytecodeClosure::binary_op op) {
  static BinaryAssembler::generic_binary_op_1 conversion_table[BytecodeClosure::number_of_binary_ops] =
  {
      BinaryAssembler::bin_add,
      BinaryAssembler::bin_sub,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_illegal,
      BinaryAssembler::bin_and,
      BinaryAssembler::bin_or,
      BinaryAssembler::bin_xor,
      BinaryAssembler::bin_illegal
  };
  generic_binary_op_1 g_op = conversion_table[op];
  GUARANTEE(g_op != bin_illegal, "Operation must be legal");
  return g_op;
}

void CodeGenerator::int_binary_do(Value& result, Value& op1, Value& op2, BytecodeClosure::binary_op op JVM_TRAPS) {
  GUARANTEE(!result.is_present(), "result must not be present anywhere");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(), "op2 must be in a register or an immediate");

  switch(op) {
    case BytecodeClosure::bin_rsb  :
      irsb (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_shr  :
      ishr (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_shl  :
      ishl (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_ushr :
      iushr(result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_mul  :
      imul (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_div  :
      idiv (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_rem  :
      irem (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); return;
    case BytecodeClosure::bin_min:
    case BytecodeClosure::bin_max:
      {
        NearLabel done;
        op1.writable_copy(result);
        if (op2.is_immediate()) { op2.materialize(); }
        cmpl(result.lo_register(), op2.lo_register());
        jcc((op == BytecodeClosure::bin_max ? greater_equal : less_equal),
      done);
        movl(result.lo_register(), op2.lo_register());
        bind(done);
        return;
      }
  }

  GUARANTEE(op == BytecodeClosure::bin_sub || op == BytecodeClosure::bin_add || op == BytecodeClosure::bin_and ||
            op == BytecodeClosure::bin_xor || op == BytecodeClosure::bin_or, "only cases left");

  op1.writable_copy(result);
  if (op2.is_immediate()) {
    // Handle subtractions and additions with +1 or -1.
    switch (op) {
      case BytecodeClosure::bin_sub  : if (op2.as_int() == +1) { decl(result.lo_register()); return; }
                                   if (op2.as_int() == -1) { incl(result.lo_register()); return; }
      case BytecodeClosure::bin_add  : if (op2.as_int() == -1) { decl(result.lo_register()); return; }
                                   if (op2.as_int() == +1) { incl(result.lo_register()); return; }
    }
    generic_binary_1(result.lo_register(), op2.as_int(), convert_to_generic_binary_1(op));
  } else {
    generic_binary_1(result.lo_register(), op2.lo_register(), convert_to_generic_binary_1(op));
  }
}

void CodeGenerator::int_unary_do(Value& result, Value& op1,
          BytecodeClosure::unary_op op JVM_TRAPS) {
  op1.writable_copy(result);

  NearLabel done;
  switch(op) {
    case BytecodeClosure::una_abs:
      testl(result.lo_register(), result.lo_register());
      jcc(greater, done);
      // fall through

    case BytecodeClosure::una_neg:
      negl(result.lo_register());
      bind(done);
      return;
    default:
      SHOULD_NOT_REACH_HERE();
      return;
  }
}

void CodeGenerator::irsb(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  GUARANTEE(op2.is_immediate(),
           "Reverse subtract should only be called with an immediate argument");
  op1.writable_copy(result);
  negl(result.lo_register());
  addl(result.lo_register(), op2.as_int());
}

void CodeGenerator::imul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    imull(result.lo_register(), op2.as_int());
  } else {
    imull(result.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::idiv_helper(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  // For now we flush before calling the compiler div stub.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // If eax is the divisor make sure we do not overwrite the value before
  // using it.
  if (op2.in_register() && op2.lo_register() == eax) {
    Value reg(T_INT);
    reg.assign_register();
    movl(reg.lo_register(), eax);
    reg.copy(op2);
  }

  // Fill in parameters.
  if (op1.lo_register() != eax) {
    movl(eax, op1.lo_register());
  }
  op1.destroy();

  if (op2.is_immediate()) {
    movl(ebx, op2.as_int());
  } else if (op2.lo_register() != ebx) {
    movl(ebx, op2.lo_register());
  }
  op2.destroy();

  call_from_compiled_code((address)compiler_idiv_irem JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::idiv(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  idiv_helper(result, op1, op2 JVM_CHECK);
  result.set_register(RegisterAllocator::allocate(eax));
}

void CodeGenerator::irem(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  idiv_helper(result, op1, op2 JVM_CHECK);
  result.set_register(RegisterAllocator::allocate(edx));
}

void CodeGenerator::ishift_helper(Value& result, Value& op1, Value& op2) {
  op1.destroy(); // In case it uses ECX.
  if (result.lo_register() == ecx) { // We need ECX for the shift count.
    Register temp = RegisterAllocator::allocate();
    movl(temp, result.lo_register());
    result.set_register(temp);
  }
  if (op2.lo_register() != ecx) { // Make sure the shift count is in ECX.
    Register temp = RegisterAllocator::allocate(ecx);
    movl(temp, op2.lo_register());
    op2.set_register(temp);
  }
  // Note: There is no need to make sure that the value in ecx is
  //       in the range 0..31 for Pentium class CPUs. The shift
  //       instructions only use the bottom 5 bits (gri - 9/18/01).
}

void CodeGenerator::ishl(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    int shift_count = op2.as_int() & 0x1f;
    if (shift_count != 0) shll(result.lo_register(), shift_count);
  } else {
    ishift_helper(result, op1, op2);
    shll(result.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::ishr(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    int shift_count = op2.as_int() & 0x1f;
    if (shift_count != 0) sarl(result.lo_register(), shift_count);
  } else {
    ishift_helper(result, op1, op2);
    sarl(result.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::iushr(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    int shift_count = op2.as_int() & 0x1f;
    if (shift_count != 0) shrl(result.lo_register(), shift_count);
  } else {
    ishift_helper(result, op1, op2);
    shrl(result.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::long_binary_do(Value& result, Value& op1, Value& op2, BytecodeClosure::binary_op op JVM_TRAPS) {
  GUARANTEE(!result.is_present(), "result must not be present anywhere");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  switch (op) {
    case BytecodeClosure::bin_sub  :
      lsub (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_rsb  :
      lrsb (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_add  :
      ladd (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_and  :
      land (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_xor  :
      lxor (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_or   :
      lor  (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_shr  :
      lshr (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_shl  :
      lshl (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_ushr :
      lushr(result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_mul  :
      lmul (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_div  :
      ldiv (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;
    case BytecodeClosure::bin_rem  :
      lrem (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM); break;

    case BytecodeClosure::bin_min:
    case BytecodeClosure::bin_max:
      {
        NearLabel bigger, smaller;
        op1.writable_copy(result);
        if (op2.is_immediate()) { op2.materialize(); }
        cmpl(op1.hi_register(), op2.hi_register());
        jcc(greater, bigger);
        jcc(less,    smaller);
        cmpl(op1.lo_register(), op2.lo_register());
        if (op == BytecodeClosure::bin_min) {
          jcc(below, smaller);
          bind(bigger);
          move(result, op2);
          bind(smaller);
        } else {
          jcc(above, bigger);
          bind(smaller);
          move(result, op2);
          bind(bigger);
        }
        return;
      }

    default                    :
      SHOULD_NOT_REACH_HERE();
      break;
  }

}

void CodeGenerator::long_unary_do(Value& result, Value& op1,
          BytecodeClosure::unary_op op JVM_TRAPS) {
  op1.writable_copy(result);

  NearLabel done;
  switch(op) {
    case BytecodeClosure::una_abs:
      testl(result.hi_register(), result.hi_register());
      jcc(greater_equal, done);
      /* Fall through */
    case BytecodeClosure::una_neg:
      negl(result.lo_register());
      adcl(result.hi_register(), 0);
      negl(result.hi_register());

      bind(done);
      break;
    default:
      SHOULD_NOT_REACH_HERE();
  }
}

void CodeGenerator::ladd(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    addl(result.lo_register(),  lsw(op2.as_long()));
    adcl(result.hi_register(), msw(op2.as_long()));
  } else {
    addl(result.lo_register(),  op2.lo_register());
    adcl(result.hi_register(), op2.hi_register());
  }
}

void CodeGenerator::lsub(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    subl(result.lo_register(),  lsw(op2.as_long()));
    sbbl(result.hi_register(), msw(op2.as_long()));
  } else {
    subl(result.lo_register(), op2.lo_register());
    sbbl(result.hi_register(), op2.hi_register());
  }
}

void CodeGenerator::lrsb(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  GUARANTEE(op2.is_immediate(),
           "Reverse subtract should only be called with an immediate argument");
  op1.writable_copy(result);

  // First negate the value
  negl(result.lo_register());
  adcl(result.hi_register(), 0);
  negl(result.hi_register());

  // Then add the constant
  addl(result.lo_register(),  lsw(op2.as_long()));
  adcl(result.hi_register(), msw(op2.as_long()));
}

void CodeGenerator::runtime_long_op(Value& result, Value& op1, Value& op2,
                                    bool check_zero, address routine JVM_TRAPS) {
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  if (check_zero) {
    GUARANTEE(op2.stack_type() == T_LONG, "Sanity");
    if (op2.in_register() || (op2.is_immediate() && op2.as_long() == 0)) {
      ZeroDivisorCheckStub* zero =
        ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(zero));
      if (op2.is_immediate()) {
        jmp(zero);
      } else {
        Register temp = RegisterAllocator::allocate();
        movl(temp, op2.lo_register());
        orl(temp,  op2.hi_register());
        jcc(equal, zero);
        RegisterAllocator::dereference(temp);
      }
    }
  }
  int total_size = 0;
  for (int i = 2; i >= 1; i--) {
    Value *op = (i == 2 ? &op2 : &op1);
    int size = op->stack_type() == T_LONG ? 2 : 1;
    total_size += size;
    if (op->is_immediate()) {
      if (size == 2) { pushl(op->hi_bits()); }
      pushl(op->lo_bits());
    } else {
      if (size == 2) { pushl(op->hi_register()); }
      pushl(op->lo_register());
    }
  }
  op1.destroy();
  op2.destroy();
  call(routine);
  addl(esp, total_size * BytesPerWord);
  result.set_registers(RegisterAllocator::allocate(eax),
                       RegisterAllocator::allocate(edx));
}

void CodeGenerator::lmul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, false,
                  (address)jvm_lmul JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::ldiv(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, true,
                  (address)jvm_ldiv JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lrem(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, true,
                  (address)jvm_lrem JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lshl(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, false,
                  (address)jvm_lshl JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lshr(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, false,
                  (address)jvm_lshr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lushr(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, false,
                  (address)jvm_lushr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::land(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    andl(result.lo_register(),  lsw(op2.as_long()));
    andl(result.hi_register(), msw(op2.as_long()));
  } else {
    andl(result.lo_register(), op2.lo_register());
    andl(result.hi_register(), op2.hi_register());
  }
}

void CodeGenerator::lor(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    orl(result.lo_register(),  lsw(op2.as_long()));
    orl(result.hi_register(), msw(op2.as_long()));
  } else {
    orl(result.lo_register(), op2.lo_register());
    orl(result.hi_register(), op2.hi_register());
  }
}

void CodeGenerator::lxor(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  op1.writable_copy(result);
  if (op2.is_immediate()) {
    xorl(result.lo_register(), lsw(op2.as_long()));
    xorl(result.hi_register(), msw(op2.as_long()));
  } else {
    xorl(result.lo_register(), op2.lo_register());
    xorl(result.hi_register(), op2.hi_register());
  }
}

void CodeGenerator::conditional_jump_do(BytecodeClosure::cond_op condition,
                                        Label& destination) {
  jcc(convert_condition(condition), destination);
}

void CodeGenerator::table_switch(Value& index, jint table_index,
                                 jint default_dest, jint low, jint high JVM_TRAPS) {
  GUARANTEE(index.in_register(), "Immediates handled by caller");

  for (int i = 0; i < (high - low + 1); i++) {
    int jump_offset = method()->get_java_switch_int(4 * i + table_index + 12);
    if (jump_offset <= 0) {
      // Negative offset in a branch table is not a usual case
      Compiler::abort_active_compilation(true JVM_THROW);
    }
    cmpl(index.lo_register(), i + low);
    conditional_jump(BytecodeClosure::eq, bci() + jump_offset, false JVM_CHECK);
  }
  branch(default_dest JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lookup_switch(Value& index, jint table_index,
                                  jint default_dest,
                                  jint num_of_pairs JVM_TRAPS) { 
  for (int i = 0; i < num_of_pairs; i++) { 
    int key = method()->get_java_switch_int(8 * i + table_index + 8);
    int jump_offset = method()->get_java_switch_int(8 * i + table_index + 12);
    if (jump_offset <= 0) {
      // Negative offset in a branch table is not a usual case
      Compiler::abort_active_compilation(true JVM_THROW);
    }
    cmpl(index.lo_register(),  key);
    conditional_jump(BytecodeClosure::eq, bci() + jump_offset, false JVM_CHECK);
  }
  branch(default_dest JVM_NO_CHECK_AT_BOTTOM);
}


void CodeGenerator::invoke(const Method* method, 
                           bool must_do_null_check JVM_TRAPS) {
  // If the method we are calling is a vanilla constructor we don't have to
  // do anything.
  BinaryAssembler::Address adr(0);

  // Flush the virtual stack frame and an unmap everything.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  verify_fpu();

  // Put the method into register ebx.
  movl(ebx, method);

  // Do null check if required
  if (must_do_null_check) {
    // Load the receiver into register eax.
    movl(edx, Address(esp,
              JavaFrame::arg_offset_from_sp(method->size_of_parameters() - 1)));
    // Null check of receiver.
    { Value receiver(T_OBJECT);
      receiver.set_register(RegisterAllocator::allocate(edx));
      null_check(receiver JVM_CHECK);
    }
  }

  if (method->is_impossible_to_compile()) {
    address target = method->execution_entry();
    call_from_compiled_code(target, method->size_of_parameters() JVM_CHECK);
  } else {
    if (ObjectHeap::contains_moveable(method->obj())) {
      int heap_offset = Method::heap_execution_entry_offset();
      movl(esi, Address(ebx, heap_offset));
    } else {
      movl(esi, Address(ebx, Method::variable_part_offset()));
      movl(esi, Address(esi));
    }
    call_from_compiled_code(esi, 0, method->size_of_parameters() JVM_CHECK);
  }

  // Update the virtual stack frame
  Signature::Raw signature = method->signature();
  frame()->adjust_for_invoke(method->size_of_parameters(),
                             signature().return_type());

#if ENABLE_WTK_PROFILER
  // Generate this always since profiler can be enabled dynamically
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::invoke_virtual(Method* method, int vtable_index,
                                   BasicType return_type JVM_TRAPS) {
  int size_of_parameters = method->size_of_parameters();

  // Flush the virtual stack frame and unmap everything.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  verify_fpu();

  // Get the receiver.
  movl(ecx, Address(esp, JavaFrame::arg_offset_from_sp(size_of_parameters - 1)));

  // Null check of receiver.
  { Value receiver(T_OBJECT);
    receiver.set_register(RegisterAllocator::allocate(ecx));
    null_check(receiver JVM_CHECK);
  }

  movl(edi, Address(ecx));    // JavaNear
  movl(edi, Address(edi, JavaNear::class_info_offset())); // ClassInfo

  movl(ebx, Address(edi, vtable_index * 4 + ClassInfoDesc::header_size()));

  if (ObjectHeap::contains_moveable(method->obj())) {
    int heap_offset = Method::heap_execution_entry_offset();
    movl(esi, Address(ebx, heap_offset));
  } else {
    movl(esi, Address(ebx, Method::variable_part_offset()));
    movl(esi, Address(esi));
  }
  call_from_compiled_code(esi, 0, size_of_parameters JVM_CHECK);

  // Update the virtual stack frame
  frame()->adjust_for_invoke(size_of_parameters, return_type);

#if ENABLE_WTK_PROFILER
  // Generate this always since profiler can be enabled dynamically
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::invoke_interface(JavaClass* klass, int itable_index,
                                     int parameters_size,
                                     BasicType return_type JVM_TRAPS) {
  // Flush the virtual stack frame and an unmap everything.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  verify_fpu();

  // Load the receiver into register edx.
  movl(edx, Address(esp, JavaFrame::arg_offset_from_sp(parameters_size - 1)));

  // Null check of receiver.
  { Value receiver(T_OBJECT);
    receiver.set_register(RegisterAllocator::allocate(edx));
    null_check(receiver JVM_CHECK);
  }

  // Get the prototype and the class of the receiver.
  movl(edx, Address(edx));
  movl(edx, Address(edx));
  // Get the itable from the class of the receiver object.
  movl(ecx, Address(edx, JavaClass::class_info_offset()));

  movzxw(edi, Address(ecx, ClassInfo::vtable_length_offset()));
  movzxw(eax, Address(ecx, ClassInfo::itable_length_offset()));
  leal(edi, Address(ecx, edi, times_4, ClassInfoDesc::header_size()));

  // Load the class_id into register ebx.
  movl(ebx, klass->class_id());

  // Lookup interface method table by linear search
  NearLabel lookup, found;
  IncompatibleClassChangeStub* error =
    IncompatibleClassChangeStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(error));

  bind(lookup);
  subl(eax, 1);
  jcc(less, error);
  cmpl(Address(edi), ebx);
  jcc(equal, found);
  addl(edi, 8);
  jmp(lookup);

  // Found the itable entry - now get the method table offset from there
  bind(found);
  movl(ebx, Address(edi, 4));
  leal(ebx, Address(ecx, ebx, times_1));

  // Get the method from the method table
  movl(ebx, Address(ebx, 4 * itable_index));

  // Get the method entry from the method.
  movl(esi, Address(ebx, Method::variable_part_offset()));
  movl(esi, Address(esi));

  // Call the method entry.
  call_from_compiled_code(esi, 0, parameters_size JVM_CHECK);

  // Update the virtual stack frame
  frame()->adjust_for_invoke(parameters_size, return_type);


#if ENABLE_WTK_PROFILER
  // Generate this always since profiler can be enabled dynamically
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::invoke_native(BasicType return_kind, address entry JVM_TRAPS) {
  NearLabel redo, next;

  bind(redo);

  // Configuration of the stack. Note: SP on x86 points to
  // the top element in the stack.
  // BP             -> saved BP
  // BP+4           -> return address
  // BP+8           -> tag   for parameter (n-1)
  // BP+12          -> value for parameter (n-1)
  // BP+16          -> tag   for parameter (n-2)
  // ...
  // BP+(4 + n * 8) -> value for paramater (0)     = _kni_parameter_base

  if (method()->max_locals() > 0) {
    LocationAddress base(0, T_OBJECT); // Type is immaterial
    BinaryAssembler::Address base_address = base.lo_address();
    if (method()->is_static()) {
      base_address._disp += BytesPerStackElement;
    }
    leal(ebx, base_address);
  } else {
    xorl(ebx, ebx);
  }
  movl(Address((int) &_kni_parameter_base), ebx);

  call_vm(entry, return_kind JVM_CHECK);
  comment("Check if native method needs redoing");
  get_thread(ebx);
  movl(ecx, Address(ebx, Thread::async_redo_offset()));
  testl(ecx, ecx);
  jcc(zero, next);

  comment("Clear Thread::async_redo");
  xorl(ecx, ecx);
  movl(Address(ebx, Thread::async_redo_offset()), ecx);
  jmp(redo);

  bind(next);
  comment("Clear Thread::async_info (ecx is already zero)");
  movl(Address(ebx, Thread::async_info_offset()), ecx);

  if (return_kind != T_VOID) {
    Value value(return_kind);
    switch(return_kind) {
      case T_OBJECT:
      case T_INT:
        value.set_register(RegisterAllocator::allocate(eax));
        break;
      case T_LONG:
        value.set_registers(RegisterAllocator::allocate(eax),
                            RegisterAllocator::allocate(edx));
        break;
#if ENABLE_FLOAT
    case T_FLOAT:
    case T_DOUBLE:
      Assembler::Register freg = RegisterAllocator::allocate_float_register();
      FPURegisterMap& fpu_map = frame()->fpu_register_map();

      value.set_register(freg);
      fpu_map.push(freg);
      break;
#endif
    }
    frame()->push(value);
  }
}

bool CodeGenerator::quick_catch_exception(const Value &exception_obj,
                                          JavaClass* catch_type, 
                                          int handler_bci JVM_TRAPS) {
  // Fast exception catching not implemented on x86.
  return false;
}

void CodeGenerator::call_vm_extra_arg(const Register extra_arg) {
  movl(edx, extra_arg);
}

void CodeGenerator::call_vm_extra_arg(const int extra_arg) {
  movl(edx, extra_arg);
}

void CodeGenerator::call_vm(address entry, BasicType return_value_type JVM_TRAPS) {
  // All registers should be flushed (not necessarily unmapped) before
  // calling call_vm.
  address target;
  if (return_value_type != T_ILLEGAL) {
    movl(eax, (int) entry);
  }
  if (stack_type_for(return_value_type) == T_OBJECT) {
#if ENABLE_ISOLATES
#ifndef PRODUCT
    if (StopAtCIBHit && entry==(address)compiled_code_task_barrier){
      breakpoint();
    }
#endif
#endif
    target = (address)shared_call_vm_oop;
  } else if (return_value_type == T_ILLEGAL) {
    target = (address)shared_call_vm_exception;
  } else {
    target = (address)shared_call_vm;
  }
  call_from_compiled_code(target JVM_CHECK);
}

void CodeGenerator::check_bytecode_counter() {
  if (Deterministic) {
    NearLabel det_done;
    Register reg;

    reg = RegisterAllocator::allocate();

    movl(reg, Address((int) &_bytecode_counter));
    decl(reg);
    jcc(not_zero, det_done);
    movl(reg, -1);
    movl(Address((int) &_current_stack_limit), reg);
    movl(reg, RESCHEDULE_COUNT);
    bind(det_done);
    movl(Address((int) &_bytecode_counter), reg);

    RegisterAllocator::dereference(reg);
  }
}

void CodeGenerator::check_stack_overflow(Method* m JVM_TRAPS) {
  Label stack_overflow, done;

  comment("Stack overflow check");
  movl(ebx, m);
  leal(edx, Address(esp, -(JavaFrame::frame_desc_size() +
             (m->max_execution_stack_count() * BytesPerStackElement))));
  cmpl(Address((int)&_current_stack_limit), edx);
  jcc(above_equal, stack_overflow);
bind(done);

  StackOverflowStub* stub =
    StackOverflowStub::allocate(stack_overflow, done, edx, ebx JVM_ZCHECK(stub));
  stub->insert();
}

void CodeGenerator::check_timer_tick(JVM_SINGLE_ARG_TRAPS) {
  Label timer_tick, done;
  comment("check for clock tick");

#if ENABLE_PAGE_PROTECTION
  movl(Address((int)&_protected_page[COMPILER_TIMER_TICK_SLOT]), eax);
#else
  cmpl(Address((int)&_rt_timer_ticks), 0);
  jcc(not_equal, timer_tick);
#endif
  bind(done);
  
  TimerTickStub* stub =
    TimerTickStub::allocate(Compiler::current()->bci(),
      timer_tick, done JVM_ZCHECK(stub));
  stub->insert();
}

CodeGenerator::Condition
CodeGenerator::convert_condition( const BytecodeClosure::cond_op condition) {
  switch (condition) {
    case BytecodeClosure::null   : // fall through
    case BytecodeClosure::eq     : return equal;
    case BytecodeClosure::nonnull: // fall through
    case BytecodeClosure::ne     : return not_equal;
    case BytecodeClosure::lt     : return less;
    case BytecodeClosure::le     : return less_equal;
    case BytecodeClosure::gt     : return greater;
    case BytecodeClosure::ge     : return greater_equal;
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
    case BytecodeClosure::negative: return negative;
    case BytecodeClosure::positive: return positive;
#endif
  }
  SHOULD_NOT_REACH_HERE();
  return (CodeGenerator::Condition) -1;
}

#ifndef PRODUCT

void CodeGenerator::verify_fpu() {
#if ENABLE_FLOAT
  GUARANTEE(Compiler::frame()->fpu_register_map().is_empty(),
            "FPU stack must be empty");
#endif
}

void CodeGenerator::verify_location_is_constant(jint index,
                                                const Value& constant) { 
  LocationAddress address(index, constant.type());
  Label ok1, ok2;
  cmpl(address.lo_address(), constant.lo_bits());
  jcc(equal, ok1);
  int3();
bind(ok1);
  if (constant.is_two_word()) {
    cmpl(address.hi_address(), constant.hi_bits());
    jcc(equal, ok2);
    int3();
  }
bind(ok2);
}

#endif

void CodeGenerator::init_static_array(Value& array JVM_TRAPS) {  
  // Flush the virtual stack frame.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  movl(edi, array.lo_register());

  // Load the address of init_static_array bc into ESI.
  Method::Raw cur_method = compiled_method()->method();
  movl(esi, &cur_method);
  addl(esi, Method::base_offset() + bci());
  
  // Type Size Factor @ bci + 1.  
  // Elements count   @ bci + 2.
  addl(esi, 2);
  // Loading elements count.
  movzxw(ebx, Address(esi));

  // Put into EBX the number of elements in array.
  movzxb(ecx, Address(esi, -1));
  // size in bytes = count * element_size = count * (1 << size factor).
  // ebx = ebx * (1 << ecx).
  shll(ebx, ecx); 
  
  // Initialize the counter.
  movl(ecx, 0);

  // Source array data @ bci + 4
  addl(esi, 2);

  // Go to the first element in the array.
  addl(edi, Array::base_offset());

  // Start copying data.
  Label copy_dword;
  bind(copy_dword);
  movl(edx, Address(esi, ecx, times_1));
  movl(Address(edi, ecx, times_1), edx);
  addl(ecx, 4);
  // Check if the required number of elements are copied.
  cmpl(ecx, ebx);
  jcc(less, copy_dword);
}
#endif
