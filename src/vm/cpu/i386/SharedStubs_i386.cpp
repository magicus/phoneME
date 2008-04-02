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

#if ENABLE_INTERPRETER_GENERATOR
#include "incls/_SharedStubs_i386.cpp.incl"

void SharedStubs::generate() {
  Label shared_entry_return_point;

  generate_shared_invoke_compiler();
#if ENABLE_JAVA_DEBUGGER
  generate_shared_invoke_debug();
#endif
  generate_shared_fast_accessors();

  generate_call_on_primordial_stack();

#if ENABLE_METHOD_TRAPS
  generate_cautious_invoke();
#endif

  generate_shared_call_vm(shared_entry_return_point, T_VOID);
  generate_shared_call_vm(shared_entry_return_point, T_OBJECT);
  generate_shared_call_vm(shared_entry_return_point, T_ILLEGAL);

  generate_invoke_pending_entries();
  
  generate_shared_entry(shared_entry_return_point);
  
  generate_shared_forward_exception(shared_entry_return_point);

  generate_shared_monitor_enter();
  generate_shared_monitor_exit();
}

void SharedStubs::generate_call_on_primordial_stack() {
  comment_section("Call on C stack");
  entry("call_on_primordial_stack");

  comment("Get address of function");
  movl(eax, Address(esp, Constant(4)));
  get_thread(ecx);
  pushl(ebp);
  movl(Address(ecx, Constant(Thread::stack_pointer_offset())), esp);

  movl(esp, Address(Constant("_primordial_sp")));
  movl(ebp, Address(esp));

  get_thread_handle(ecx);
  pushl(ecx);
  call(eax);

  get_thread(ecx);
  movl(esp, Address(ecx, Constant(Thread::stack_pointer_offset())));
  popl(ebp);
  ret();

  entry_end(); // call_on_primordial_stack
}

#if ENABLE_METHOD_TRAPS
/*
 * Trap function that
 *  (1) Calls interrupt_or_invoke on primordial stack to determine if it is
 *      time for JVM to take special action (e.g. to stop VM or to stop current
 *      isolate). The method of switching to primordial stack is similar to
 *      call_on_primordial_stack invocation except that we need to pass
 *      an additional parameter (MethodTrapDesc) to the function we call.
 *  (2) If no special action is required, the control is transparently
 *      yielded to the original method entry.
 */
void SharedStubs::generate_cautious_invoke() {
  Label trap_function, call_original_entry, has_java_handler;
  
  align(trap_entry_offset);
  entry("cautious_invoke");
  comment("Generate a number of trap entries for different methods");
  for (int i = 0; i < max_traps; i++) {
    comment("Trap entry no.%d", i);
    movl(eax, Constant(i));
    jmp(Constant(trap_function));
    align(trap_entry_offset);
  }
  
bind(trap_function);
  comment("eax = &_method_trap[trap_no]");
  imull(eax, Constant(sizeof(MethodTrapDesc)));
  addl(eax, Constant("_method_trap"));

  comment("Test if this trap has Java handler");
  movl(ecx, Address(eax, Constant(MethodTrapDesc::handler_method_offset())));
  orl(ecx, ecx);
  jcc(not_zero, Constant(has_java_handler));

  comment("Switch to C stack");
  get_thread(ecx);
  pushl(ebp);
  movl(Address(ecx, Constant(Thread::stack_pointer_offset())), esp);
  movl(esp, Address(Constant("_primordial_sp")));
  movl(ebp, Address(esp));

  comment("Pass MethodTrapDesc argument and call test function");
  pushl(eax);
  call(Constant("interrupt_or_invoke"));

  comment("Get saved MethodTrapDesc out of stack");
  popl(eax);

  comment("Get back to our current Java stack");
  get_thread(ecx);
  movl(esp, Address(ecx, Constant(Thread::stack_pointer_offset())));
  popl(ebp);

bind(call_original_entry);
  comment("Jump to stored original method entry");
  jmp(Address(eax, Constant(MethodTrapDesc::old_entry_offset())));

bind(has_java_handler);
  comment("Check if we are calling directly from the same handler");
  cmpl(ecx, Address(ebp, Constant(JavaFrame::method_offset())));
  jcc(equal, Constant(call_original_entry));

  comment("Replace the execution entry and yield control to the trap handler");
  movl(ebx, ecx);
  jmp(Constant("interpreter_method_entry"));
  entry_end();
}
#endif

void SharedStubs::generate_shared_entry(Label& shared_entry_return_point) {
  comment_section("Shared entry");
  entry("shared_entry");

  // edx:eax = return value from initial call vm (must be preserved)
  // ecx     = return value tag (either int or object)
  
  comment("Push faked entry frame return address");
  popl(ebx);
  pushl(Constant(EntryFrame::FakeReturnAddress));
  pushl(ebx);

  comment("Get the current thread");
  get_thread(ebx);

  movl(ebp, esp);

  comment("Push previous frame pointer and stack pointer from thread");
  pushl(Address(ebx, Constant(Thread::last_java_fp_offset())));
  pushl(Address(ebx, Constant(Thread::last_java_sp_offset())));

  comment("Reset the last frame pointer and last stack pointer");
  xorl(edi, edi);
  movl(Address(ebx, Constant(Thread::last_java_fp_offset())), edi);
  movl(Address(ebx, Constant(Thread::last_java_sp_offset())), edi);
  
  // edx:eax = return value from initial call vm (must be preserved)
  // ecx     = return value tag (either int or object)

  Label is_object;
  comment("Save values");
  pushl(edx);           // stored int_value1
  pushl(eax);           // stored int_value2
  pushl(ecx);           // stored_obj_value
  
  comment("Save the pending exception onto the stack");
  pushl(Address(Constant("_current_pending_exception")));

  comment("Get the pending activation");
  movl(edi, Address(ebx, Constant(Thread::pending_entries_offset())));

  comment("Clear the pending exception and the pending activations in the thread");
  xorl(eax, eax);
  movl(Address(ebx, Constant(Thread::pending_entries_offset())), eax);
  movl(Address(Constant("_current_pending_exception")), eax);
  jmp(Constant("pending_activations_loop"));
  entry_end();

  Label check_for_more_parameters, push_parameter;
  entry("pending_activations_loop");

  comment("edi contains the current pending activation");
  pushl(Address(edi, Constant(EntryActivation::next_offset())));

  comment("Get the length of the parameters and a pointer to the first parameter");
  movl(ecx, Address(edi, Constant(EntryActivation::length_offset())));
  leal(edx, Address(edi, Constant(EntryActivationDesc::header_size())));
  jmp(Constant(check_for_more_parameters));

  comment("Push the next parameter onto the stack");
  bind(push_parameter);
  pushl(Address(edx, Constant(0 * BytesPerWord))); // value
  if (TaggedJavaStack) {
    pushl(Address(edx, Constant(1 * BytesPerWord))); // tag
  }
  
  comment("Increment the parameters pointer and decrement the parameter length");
  addl(edx, Constant(2 * BytesPerWord));
  decl(ecx);

  comment("Check if there are more parameters");
  bind(check_for_more_parameters);
  testl(ecx, ecx);
  jcc(not_zero, Constant(push_parameter));

  comment("Get the method from the pending activation");
  movl(ebx, Address(edi, Constant(EntryActivation::method_offset())));

  comment("Get the execution entry");
  movl(esi, Address(ebx, Constant(Method::variable_part_offset())));
  movl(esi, Address(esi));

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  if (TaggedJavaStack) {
    comment("TaggedJavaStack not supported with ENABLE_REFLECTON");
    int3();
  }

  comment("Choose entry point for the method according to its return type");
  pushl(Address(edi, Constant(EntryActivation::return_point_offset())));
  comment("Emulate CALL instruction with variable return point");
  jmp(esi);

  entry_end(); // pending_activations_loop

  entry("shared_entry_return_point");
#else
  call(esi);
  define_call_info();
#endif

  if (GenerateDebugAssembly) {
    comment("Assertion code: now stack must be empty");
    Label stack_is_empty;
    leal(ecx, Address(ebp, Constant(EntryFrame::empty_stack_offset())));
    cmpl(esp, ecx);
    jcc(zero, Constant(stack_is_empty));
    int3();
    bind(stack_is_empty);
  }
  
  if (ENABLE_WTK_PROFILER) {
    comment("Setup the pointer to the vm routine");
    movl(eax, Constant("jprof_record_method_transition"));
    call_shared_call_vm(T_VOID);
  }

  comment("Make the next pending activation the current one");
  popl(edi);
  comment("Check if there are more pending activations");
  testl(edi, edi);
  jcc(not_zero, Constant("pending_activations_loop"));

  comment("Get the current thread");
  get_thread(ebx);

  comment("Pop pending exception");
  popl(Address(Constant("_current_pending_exception")));
  comment("Get stored values");
  popl(ecx);
  popl(eax);
  popl(edx);

  comment("Restore the original frame pointer and the last java sp/fp");
  popl(Address(ebx, Constant(Thread::last_java_sp_offset())));
  popl(ebp);
  movl(Address(ebx, Constant(Thread::last_java_fp_offset())), ebp);

  comment("Remove fake frame pointer and return");
  ret(Constant(BytesPerWord));

  entry_end(); // shared_entry

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  if (TaggedJavaStack) {
    comment("TaggedJavaStack not supported with ENABLE_REFLECTON");
    int3();
  }

  entry("entry_return_object");
  define_call_info();
  popl(Address(ebp, Constant(EntryFrame::stored_obj_value_offset())));
  jmp(Constant("shared_entry_return_point"));
  entry_end();

  entry("entry_return_double");
  define_call_info();
  fld_d(Address(esp));
  addl(esp, Constant(2 * BytesPerWord));
  jmp(Constant("shared_entry_return_point"));
  entry_end();

  entry("entry_return_float");
  define_call_info();
  fld_f(Address(esp));
  popl(eax);
  jmp(Constant("shared_entry_return_point"));
  entry_end();

  entry("entry_return_long");
  define_call_info();
  popl(Address(ebp, Constant(EntryFrame::stored_int_value2_offset())));
  popl(Address(ebp, Constant(EntryFrame::stored_int_value1_offset())));
  jmp(Constant("shared_entry_return_point"));
  entry_end();

  entry("entry_return_word");
  define_call_info();
  popl(Address(ebp, Constant(EntryFrame::stored_int_value2_offset())));
  jmp(Constant("shared_entry_return_point"));
  entry_end();

  entry("entry_return_void");
  define_call_info();
  jmp(Constant("shared_entry_return_point"));
  entry_end();
#endif
}

void SharedStubs::generate_shared_invoke_compiler() {
  comment_section("Shared invoke compiler");
  entry("shared_invoke_compiler");

#if ENABLE_PAGE_PROTECTION
  call(Constant("OsMisc_page_protect"));
#else
  incl(Address(Constant("_rt_timer_ticks")));
#endif

  if (AddExternCUnderscore) {
    emit_instruction("jmp _interpreter_method_entry");
  } else {
    emit_instruction("jmp  interpreter_method_entry");
  }
  entry_end(); // shared_invoke_compiler
}

void SharedStubs::generate_shared_invoke_debug() {
  comment_section("Shared invoke debug");
  entry("shared_invoke_debug");
  if (AddExternCUnderscore) {
    emit_instruction("jmp _interpreter_method_entry");
  } else {
    emit_instruction("jmp  interpreter_method_entry");
  }
  entry_end(); // shared_invoke_debug
}

void SharedStubs::generate_shared_fast_accessors() {
  comment_section("Shared fast accessors");

  const struct {
    BasicType   type;
    const char* name;
  } desc[] = {
    { T_BYTE  , "byte"},
    { T_SHORT , "short"},
    { T_CHAR  , "char"},
    { T_FLOAT , "float"},
    { T_INT   , "int"},
    { T_DOUBLE, "double"},
    { T_LONG  , "long"},
  };

  bool is_static = false;
  do {
    for (int i = 0; i < sizeof(desc)/sizeof(desc[0]); i++) {
      char tmp[128];
      sprintf(tmp, "shared_fast_get%s%s_accessor", desc[i].name,
              is_static ? "_static" : "");
      rom_linkable_entry(tmp);

      if ((desc[i].type == T_FLOAT) || (desc[i].type == T_DOUBLE)) {
        sprintf(tmp, "shared_fast_get%s%s_accessor", 
                desc[i].type == T_FLOAT ? "int" : "long",
                is_static ? "_static" : "");
        jmp(Constant(tmp));
        rom_linkable_entry_end();
        continue;
      }


      if (::byte_size_for(desc[i].type) <= BytesPerWord) {
        // this is the top of the stack, except for the return address
        comment("Get this");
        movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0) +
                                        BytesPerWord)));

        if (is_static) {
          comment("Null check");
          testl(edx, edx);
          jcc(zero, Constant("interpreter_throw_NullPointerException"));
        }

        comment("Get the %s field from the 'this' object", desc[i].name);
        movzxw(eax, Address(ebx, 
                            Constant(Method::fast_accessor_offset_offset())));

        switch (desc[i].type) {
        case T_BYTE:
          movsxb(eax, Address(edx, eax, times_1));
          break;
        case T_SHORT:
          movsxw(eax, Address(edx, eax, times_1));
          break;
        case T_CHAR:
          movzxw(eax, Address(edx, eax, times_1));
          break;
        default:
          movl(eax, Address(edx, eax, times_1));
          break;
        }
        movl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0) +
                                   BytesPerWord)),
             eax);
        ret();
      } else {
        popl(esi);                  // Get return address
        pop_obj(edx, edx);          // Get this

        if (is_static) {
          comment("Null check");
          testl(edx, edx);
          jcc(zero, Constant("null_receiver"));
        }

        comment("Push the %s field from the 'this' object onto the stack", 
                desc[i].name);
        movzxw(eax, Address(ebx, 
                            Constant(Method::fast_accessor_offset_offset())));

        if (desc[i].type == T_LONG) {
          push_long(Address(edx, eax, times_1),
                    Address(edx, eax, times_1, Constant(4)));
        } else if (desc[i].type == T_DOUBLE) {
          push_double(Address(edx, eax, times_1),
                      Address(edx, eax, times_1, Constant(4)));
        }
        jmp(esi);
      }
      rom_linkable_entry_end(); // shared_fast_get%s_accessor
    }
  } while (!is_static++);
  
 entry("null_receiver");
  comment("Restore stack state and continue with exception throwing");
  push_obj(edx);
  pushl(esi);
  // static fast accessors should not be called from compiler,
  // because they are inlined into compiled caller
  jmp(Constant("interpreter_throw_NullPointerException"));
  entry_end(); // null_receiver
}

void SharedStubs::generate_shared_call_vm(Label& shared_entry_return_point,
                                          BasicType return_value_type) {
  const char * main_entry = NULL;
  const char * return_entry = NULL;
  
  switch(return_value_type) {     
    case T_OBJECT:
      main_entry   = "shared_call_vm_oop";
      return_entry = "shared_call_vm_oop_return";
      break;
    case T_VOID:
      main_entry   = "shared_call_vm";
      return_entry = "shared_call_vm_return";
      break;
    case T_ILLEGAL:
      main_entry   = "shared_call_vm_exception";
      return_entry = "shared_call_vm_exception_return";
      break;
    default:           SHOULD_NOT_REACH_HERE();
  }

  GUARANTEE(main_entry != NULL && return_entry != NULL, "Sanity");

  comment_section("Shared call VM for ");

  entry(main_entry);

#if ENABLE_EMBEDDED_CALLINFO
  if (GenerateDebugAssembly) {
    Label done;
    comment("Make sure the return address points to a test or breakpoint instruction");
    movl(esi, Address(esp));
    cmpb(Address(esi), Constant(0xa9));
    jcc(equal, Constant(done));
    cmpb(Address(esi), Constant(0xcc));
    jcc(equal, Constant(done));
    comment("Return address does not point to test instruction");
    int3();
    bind(done);
  }
#endif // ENABLE_EMBEDDED_CALLINFO

  comment("Calculate the last java stack pointer");
  leal(esi, Address(esp, Constant(4)));

  if (return_value_type == T_ILLEGAL) { 
    // If the top-most frame catches the exception, we need to make sure that
    // there is sufficient space for at least one element on its expression
    // stack.
    comment("Leave space to expand stack, if necessary");
    pushl(Constant(0));
  }

  comment("Get the current thread");
  get_thread(ecx);

  comment("LWT Switch stack to primordial thread");
  pushl(Constant(return_entry, /*is_proc*/true, /*offset*/0));
  pushl(ebp);

  comment("Save last java stack pointer and last java frame pointer in thread");
  movl(Address(ecx, Constant(Thread::stack_pointer_offset())), esp);
  movl(Address(ecx, Constant(Thread::last_java_fp_offset())), ebp);
  movl(Address(ecx, Constant(Thread::last_java_sp_offset())), esi);

  movl(esp, Address(Constant("_primordial_sp")));
  movl(ebp, Address(esp));

  comment("Push optional argument");
  pushl(edx);
  
  comment("Push the thread");
  get_thread_handle(ecx);
  pushl(ecx);

  comment("Call the native entry");
  if (return_value_type == T_ILLEGAL) { 
    call(Constant("find_exception_frame"));
  } else { 
    /*
    if (ENABLE_PERFORMANCE_COUNTERS) {
      pushl(eax);
      call(Constant("jvm_start_call_vm"));
      popl(eax);
    }
    */
    call(eax);
  }
  /*
  if (ENABLE_PERFORMANCE_COUNTERS) {
    if (return_value_type != T_ILLEGAL) { 
      pushl(eax);
      call(Constant("jvm_end_call_vm"));
      popl(eax);
    }
  }
  */
  comment("Remove the thread and the optional argument from the stack");
  addl(esp, Constant(8));
  
  comment("save the return values in the thread");
  get_thread(ecx);
  if (return_value_type == T_OBJECT) {
    leal(edx, Address(ecx, Constant(Thread::obj_value_offset())));
    movl(Address(edx), eax);
    oop_write_barrier(edx, eax, false);
  } else { 
    movl(Address(ecx, Constant(Thread::int1_value_offset())), eax);
    if (return_value_type == T_VOID) {
      movl(Address(ecx, Constant(Thread::int2_value_offset())), edx);
    }
  }

  comment("Check for pending activations and exceptions");

  // before switching back to the Java stack, we need to see if there is enough
  // stack space available for the parameters in the pending activation.  We
  // check it here rather than in the activation code since we may need to 
  // call into the VM again and we want to avoid a potential recursive call
  // into shared entry.
  Label check_stack, get_next, no_entries;
  comment("check for pending entries and if so, check for enough stack space");
  cmpl(Address(ecx, Constant(Thread::pending_entries_offset())), Constant(0));
  jcc(equal, Constant(no_entries));

  movl(esi, Address(ecx, Constant(Thread::pending_entries_offset())));
  comment("get parameter size");
  movl(eax, Address(esi, Constant(EntryActivation::length_offset())));
  comment("go to next activation if present");
  jmp(Constant(get_next));

  bind(check_stack);
  comment("check parameter size");
  cmpl(eax, Address(esi, Constant(EntryActivation::length_offset())));
  jcc(above, Constant(get_next));
  comment("store the maximum size into eax");
  movl(eax, Address(esi, Constant(EntryActivation::length_offset())));
  comment("get the next activation");
  bind(get_next);
  movl(esi, Address(esi, Constant(EntryActivation::next_offset())));
  testl(esi, esi);
  jcc(not_zero, Constant(check_stack));
  comment("convert number of params to bytes plus a fudge factor");
  shll(eax, Constant(LogBytesPerStackElement));
  comment("don't bother checking, add in stack lock size");
  addl(eax, Constant(StackLock::size()));
  movl(esi, Address(ecx, Constant(Thread::last_java_sp_offset())));
  comment("see if there's enough stack");
  subl(esi, eax);
  cmpl(esi, Address(Constant("_current_stack_limit")));
  jcc(above_equal, Constant(no_entries));
  comment("would run out of stack, so we must expand it before calling the entry");
  comment("Push optional argument");
  pushl(esi);
  
  comment("Push the thread");
  get_thread_handle(ecx);
  pushl(ecx);

  comment("Call the native entry");
  movl(eax, Constant("stack_overflow"));
  call(eax);

  comment("Remove the thread and the optional argument from the stack");
  addl(esp, Constant(8));

  bind(no_entries);

  comment("Call switch_thread(JVM_TRAPS)");
  get_thread_handle(ecx); 
  pushl(ecx);
  call(Constant("switch_thread"));
  addl(esp, Constant(BytesPerWord));

  comment("Switch back to the java stack");
  get_thread(ebx);
  movl(esp, Address(ebx, Constant(Thread::stack_pointer_offset())));
  popl(ebp);
  ret();

  entry_end();
  entry(return_entry);

  get_thread(ebx);

  if (return_value_type == T_ILLEGAL) { 
    // We are guaranteed to have a one-item stack,
    // with return address just after
    movl(ebp, Address(ebx, Constant(Thread::last_java_fp_offset())));
    movl(ecx, Address(ebx, Constant(Thread::last_java_sp_offset())));
    leal(esp, Address(ecx, Constant(-4)));
  }

  comment("restore the return values in the thread");
  if (return_value_type == T_OBJECT) {
    movl(ecx, Address(ebx, Constant(Thread::obj_value_offset())));
    movl(Address(ebx, Constant(Thread::obj_value_offset())), Constant(0));
  } else {
    movl(eax, Address(ebx, Constant(Thread::int1_value_offset())));
    if (return_value_type == T_VOID) {
      movl(edx, Address(ebx, Constant(Thread::int2_value_offset())));
    }
  }

  Label check_for_pending_exceptions;

  comment("Check for pending activations");
  cmpl(Address(ebx, Constant(Thread::pending_entries_offset())), Constant(0));
  jcc(equal, Constant(check_for_pending_exceptions));
  
  comment("Invoke the pending activations");
  if (return_value_type != T_OBJECT) {
    xorl(ecx, ecx);
  }
  call(Constant("shared_entry"));

  comment("Restore the thread");
  get_thread(ebx);
  bind(check_for_pending_exceptions);
  if (return_value_type == T_OBJECT) {
    movl(eax, ecx);
  }

  comment("Reset last java stack pointer and last java frame pointer in thread");
  // Clear last_java_sp_offset first to avoid race condition in profiler.
  xorl(ecx, ecx);
  movl(Address(ebx, Constant(Thread::last_java_sp_offset())), ecx);
  movl(Address(ebx, Constant(Thread::last_java_fp_offset())), ecx);

  if (return_value_type == T_ILLEGAL) { 
    comment("A return value of zero means the original entry frame");
    Label done;
    testl(eax, eax);
    jcc(not_equal, Constant(done));
    movl(eax, Address(ebp, Constant(EntryFrame::pending_exception_offset())));
    leal(esp, Address(ebp, Constant(EntryFrame::real_return_address_offset())));
    movl(Address(Constant("_current_pending_exception")), eax);
    ret(Constant(BytesPerWord));
    bind(done);
  }


  comment("Check for pending exceptions");
  cmpl(Address(Constant("_current_pending_exception")), Constant(0));
  jcc(not_equal, Constant("shared_call_vm_forward_exception"));

  comment("Return to the caller");
  ret();

  entry_end(); // shared_call_vm*
}

void SharedStubs::generate_shared_forward_exception(Label& shared_entry_return_point) {
  comment_section("Shared forward exception");  
  entry("shared_call_vm_forward_exception", 0);
    xorl(eax, eax);
    movl(edx, Address(Constant("_current_pending_exception")));
    movl(Address(Constant("_current_pending_exception")), eax);
    jmp(Constant("shared_call_vm_exception"));
  entry_end(); // shared_call_vm_forward_exception
}

void SharedStubs::generate_invoke_pending_entries() {
  comment_section("Invoke pending entries");
  entry("invoke_pending_entries");

  comment("Setup frame pointer");
  pushl(ebp);
  movl(ebp, esp);

  comment("Push padding (if necessary) onto stack to make double cells 8-byte aligned");
  Label no_padding;
  testl(esp, Constant(7));
  jcc(zero, Constant(no_padding));
  pushl(Constant(0xbeefdead));
  bind(no_padding);

  comment("Preserve all registers -- this must be done because this function");
  comment("is called by C code. VC++'s code generator expects the callee to");
  comment("preserve non-temporary registers");
  // IMPL_NOTE: to save a few cycles, we may not have to preserve registers
  // that the compiler considers as 'scratch' registers. We'd need
  // a compiler-specific macro to do this
  comment("save original sp");
  movl(ecx, esp);
  pushal();

  Label done;
  comment("Check if there's pending entries at all");
  get_thread(ebx);
  cmpl(Address(ebx, Constant(Thread::pending_entries_offset())), Constant(0));
  jcc(equal, Constant(done));  

  comment("Invoke the pending entries");
  xorl(ecx, ecx);
  call(Constant("shared_entry"));
  
  bind(done);
  comment("Restore all registers");
  popal();

  comment("relocate ebp");
  subl(ecx, esp);
  subl(ebp, ecx);
  comment("Restore frame pointer and return");
  movl(esp, ebp);
  popl(ebp);
  subl(ebp, ecx);
  ret();
  entry_end(); // invoke_pending_entries
}

void SharedStubs::generate_shared_monitor_enter() {
  comment_section("Shared monitor enter stub");

  entry("shared_lock_synchronized_method");
  comment("The object to lock on is already in eax");
  comment("The stack lock is just below the return address");
  leal(edx, Address(esp, Constant(BytesPerWord)));
  jmp(Constant("_shared_lock_allocated_monitor"));
  entry_end();

  entry("shared_monitor_enter");
  comment("Initialize stack lock pointer");
  xorl(edx, edx);
  
  comment("Find free slot in monitor block");
  Label enter_loop, loop, exit_loop;
  movl(ecx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  leal(ebx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  jmp(Constant(enter_loop));

  Label skip_store;
  bind(loop);
  comment("Start the loop by checking if the current stack lock is empty");
  cmpl(Address(ecx, Constant(StackLock::size())), Constant(0));
  jcc(not_equal, Constant(skip_store));
  movl(edx, ecx);
  bind(skip_store);

  comment("Check to see if the object in the current stack lock is equal "
          "to the object from the stack");
  cmpl(eax, Address(ecx, Constant(StackLock::size())));
  jcc(equal, Constant(exit_loop));

  comment("Go to next stack lock in monitor block");
  addl(ecx, Constant(4 + StackLock::size()));

  bind(enter_loop);
  comment("Check to see whether or not we're at the end of the monitor block");
  cmpl(ecx, ebx);                              
  jcc(not_equal, Constant(loop));
  bind(exit_loop);
  
  comment("Check if we've found an entry");
  testl(edx, edx);
  jcc(zero, Constant("_shared_lock_allocate"));

  if (GenerateInlineAsm)
      jmp(Constant("_shared_lock_allocated_monitor"));
  else
      comment("FALLTHROUGH");
  entry_end(); // shared_monitor_enter

  entry("_shared_lock_allocated_monitor");
  comment("We now have a stack lock (pointed to by register edx) in "
          "the monitor block");

  Label maybe_slow_case, slow_case, done;
  comment("Store the object in the stack lock and lock it");
  movl(Address(edx, Constant(StackLock::size())), eax);

  comment("Get the near object");
  movl(ebx, Address(eax));
  comment("see if it's an interned string");
  movl(ecx, Address(Constant("_interned_string_near_addr")));
  cmpl(ebx, ecx);
  jcc(equal, Constant(slow_case));

  comment("Check the lock bit");
  testl(Address(ebx, Constant(JavaNear::raw_value_offset())), Constant(0x1));
  jcc(not_zero, Constant(maybe_slow_case));
  
  comment("Put the real java near pointer in the stack lock");
  movl(Address(edx, Constant(StackLock::real_java_near_offset())), ebx);
  
  comment("Put thread in the stack lock");
  get_thread(ecx);
  movl(Address(edx, Constant(StackLock::thread_offset())), ecx);

  comment("Clear waiters");
  xorl(ecx, ecx);
  movl(Address(edx, Constant(StackLock::waiters_offset())), ecx);

  comment("Update the near pointer in the object");
  leal(ecx, Address(edx, Constant(StackLock::copied_near_offset())));
  movl(Address(eax), ecx);
  
  comment("Copy the locked near object to the stack");
#if ENABLE_OOP_TAG
  GUARANTEE(sizeof(JavaNearDesc) == 16, "Sanity");
  GUARANTEE(JavaNear::raw_value_offset() == 12, "Sanity");
#else
  GUARANTEE(sizeof(JavaNearDesc) == 12, "Sanity");
  GUARANTEE(JavaNear::raw_value_offset() == 8, "Sanity");
#endif
  movl(eax, Address(ebx));
  movl(ecx, Address(ebx, Constant(4)));
  movl(Address(edx, Constant(StackLock::copied_near_offset() + 0)), eax);
  movl(Address(edx, Constant(StackLock::copied_near_offset() + 4)), ecx); 
#if ENABLE_OOP_TAG
  movl(ecx, Address(ebx, Constant(8)));
  movl(Address(edx, Constant(StackLock::copied_near_offset() + 8)), ecx); 
  movl(eax, Address(ebx, Constant(12)));  
  orl(eax, Constant(1));
  movl(Address(edx, Constant(StackLock::copied_near_offset() + 12)), eax);   
#else
  movl(eax, Address(ebx, Constant(8)));  
  orl(eax, Constant(1));
  movl(Address(edx, Constant(StackLock::copied_near_offset() + 8)), eax);   
#endif
  jmp(Constant(done));

  // Maybe slow case
  bind(maybe_slow_case);
  get_thread(ecx);
  cmpl(Address(ebx, Constant(StackLock::thread_offset() -
                             StackLock::copied_near_offset())), ecx);
  jcc(not_equal, Constant(slow_case));
  movl(Address(edx, Constant(StackLock::real_java_near_offset())),Constant(0));
  jmp(Constant(done));

  // Slow case.
  bind(slow_case);

  comment("Call runtime system to lock the stack lock");
  movl(Address(edx, Constant(StackLock::real_java_near_offset())),Constant(0));
  leal(eax, Address(Constant("lock_stack_lock")));
  goto_shared_call_vm(T_VOID);

  bind(done);
  comment("Return to the compiled code");
  ret();
  entry_end(); // _shared_lock_allocated_monitor


  entry("_shared_lock_allocate");
  comment("Allocate a stack lock in the monitor block");
  { Label enter_loop, loop;
    comment("Set edx to old expression stack bottom");
    movl(edx, Address(ebp,Constant(JavaFrame::stack_bottom_pointer_offset())));

    comment("Move expression stack top and bottom");
    subl(esp, Constant(StackLock::size() + 4));
    subl(edx, Constant(StackLock::size() + 4));
    movl(Address(ebp,Constant(JavaFrame::stack_bottom_pointer_offset())), edx);

    comment("Move the expression stack contents");
    movl(ecx, esp);
    jmp(Constant(enter_loop));

    bind(loop);
    comment("Copy one word at a time");
    movl(ebx, Address(ecx, Constant(StackLock::size() + 4)));
    movl(Address(ecx), ebx);
    addl(ecx, Constant(4));
    
    bind(enter_loop);
    comment("Check if bottom is reached");
    cmpl(ecx, edx);
    jcc(not_equal, Constant(loop));
    jmp(Constant("_shared_lock_allocated_monitor"));
  }
  entry_end(); // _shared_lock_allocate
}

void SharedStubs::generate_shared_monitor_exit() {
  comment_section("Shared monitor exit stub");
  entry("shared_monitor_exit");

  comment("Find matching slot in monitor block");
  Label enter_loop, loop;
  movl(ecx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  leal(ebx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  comment("Get near object");
  movl(edx, Address(eax));
  cmpl(edx, Address(Constant("_interned_string_near_addr")));
  jcc(not_equal, Constant(enter_loop));
  movl(edx, eax);
  leal(eax, Address(Constant("unlock_special_stack_lock")));
  goto_shared_call_vm(T_VOID);

  bind(loop);
  comment("Check to see if the object in the current stack lock is equal "
          "to the object from the stack");
  cmpl(eax, Address(ecx, Constant(StackLock::size())));
  jcc(equal, Constant("_shared_do_unlock"));

  comment("Go to next stack lock in monitor block");
  addl(ecx, Constant(4 + StackLock::size()));

  bind(enter_loop);
  comment("Check to see whether or not we're at the end of the monitor block");
  cmpl(ecx, ebx);                              
  jcc(not_equal, Constant(loop));

  comment("Throw an IllegalMonitorStateException");
  leal(eax, Address(Constant("illegal_monitor_state_exception")));
  goto_shared_call_vm(T_VOID);

  entry_end(); // shared_monitor_exit

  entry("shared_unlock_synchronized_method");
  leal(ecx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset() -
                                 (BytesPerWord + StackLock::size()))));
  movl(eax, Address(ecx, Constant(StackLock::size())));
  testl(eax, eax);

  jcc(not_equal, Constant("_shared_do_unlock"));

  // We could jump to the code in shared_monitor_exit above to throw
  // the exception (and shave off one instruction) but only in the
  // plain asm version, not in inline asm.  So it's cleaner to always
  // provide own copy of this code.
  comment("Throw an IllegalMonitorStateException");
  leal(eax, Address(Constant("illegal_monitor_state_exception")));
  goto_shared_call_vm(T_VOID);
  entry_end(); // shared_unlock_synchronized_method

  entry("_shared_do_unlock");
  comment("eax contains the object");
  comment("ecx contains the stack lock");
  // eax contains the object

  comment("Found monitor to unlock - now get/reset the object");
  movl(Address(ecx, Constant(StackLock::size())), Constant(0));

  comment("Get the real java near pointer from the stack lock");
  movl(edx, Address(ecx,Constant( StackLock::real_java_near_offset())));

  comment("Check if zero (reentrant case)");
  Label done;
  testl(edx, edx);
  jcc(zero, Constant(done));

  comment("Set the object near pointer");
  movl(Address(eax), edx);
  oop_write_barrier(eax, edx, false, true);

  comment("Check for waiters");
  testl(Address(ecx, Constant(StackLock::copied_near_offset() + 
                              JavaNear::raw_value_offset())), Constant(0x2));
  jcc(zero, Constant(done));
  
  comment("Save argument to vm call in edx and do the call");
  movl(edx, ecx);

  comment("Call the runtime system to signal the waiters");
  leal(eax, Address(Constant("signal_waiters")));
  goto_shared_call_vm(T_VOID);

  // And we're done.
  bind(done);
  ret();

  entry_end(); // _shared_do_unlock
}

#endif // ENABLE_INTERPRETER_GENERATOR
