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
#include "incls/_InterpreterStubs_i386.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

void InterpreterStubs::generate() {
  generate_interpreter_call_vm_redo();
  generate_interpreter_call_vm_dispatch();

  generate_interpreter_throw_exceptions();

  // generate_interpreter_rethrow_exception_init();
  // generate_interpreter_rethrow_exception();
  // generate_interpreter_unwind_activation();

#if ENABLE_EMBEDDED_CALLINFO
  if (TaggedJavaStack) { 
    generate_interpreter_fill_in_tags();
  }
#else
  GUARANTEE(!TaggedJavaStack, "Tagged stack not supported");
#endif // ENABLE_EMBEDDED_CALLINFO
  generate_interpreter_deoptimization_entry();
  generate_interpreter_timer_tick();

  generate_primordial_to_current_thread();
  generate_current_thread_to_primordial();
}

void InterpreterStubs::generate_primordial_to_current_thread() {
  entry("primordial_to_current_thread");
  pushal();
  pushl(ebp);
  movl(Address(Constant("_primordial_sp")), esp);
  get_thread(ecx);
  movl(esp, Address(ecx, Constant(Thread::stack_pointer_offset())));
  popl(ebp);
  ret();
  entry_end(); // primordial_to_current_thread

  entry("start_lightweight_thread_asm");
  // Should never reach here on x86
  int3();
  entry_end(); // start_lightweight_thread_asm
}

void InterpreterStubs::generate_current_thread_to_primordial() {
  entry("current_thread_to_primordial");
  // We're never going to return to this thread, so it doesn't matter if
  // it doesn't look like a stopped Java thread anymore.
  // pushl(ebp);
  // get_thread(ecx);
  // movl(Address(ecx, Constant(Thread::stack_pointer_offset())), esp);
  movl(esp, Address(Constant("_primordial_sp")));
  popl(ebp);
  popal();
  ret();
  entry_end(); // current_thread_to_primordial
}

void InterpreterStubs::generate_interpreter_call_vm_dispatch() {
  comment_section("Interpreter call VM - and dispatch to the bytecode returned by the VM upon termination");
  entry("interpreter_call_vm_dispatch");

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  comment("Call the shared call vm and disregard any return value");
  call_shared_call_vm(T_INT);
 
  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));

  comment("Restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  comment("Dispatch to next byte code");
  jmp(Address(no_reg, eax, times_4, Constant("interpreter_dispatch_table")));

  entry_end(); // interpreter_call_vm_dispatch
}

void InterpreterStubs::generate_interpreter_call_vm_redo() {
#if ENABLE_JAVA_DEBUGGER
  Label check_breakpoint, no_breakpoint;
#endif
  comment_section("Interpreter call VM - and repeat current bytecode upon termination");
  entry("interpreter_call_vm_redo");

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  comment("Call the shared call vm and disregard any return value");
  call_shared_call_vm(T_VOID);
 
  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));

  comment("Restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

#if ENABLE_JAVA_DEBUGGER
  comment("Check to see if we are connected to a debugger");
  cmpb(Address(Constant("_debugger_active")), Constant(0));
  jcc(not_zero, Constant(check_breakpoint));
  bind(no_breakpoint);
  comment("Not debugging, so just dispatch");
  dispatch_next(0);
  bind(check_breakpoint);
  comment("We are debugging, so let's see if we just replaced a breakpoint opcode");
  cmpb(Address(esi), Constant(Bytecodes::_breakpoint));
  jcc(not_zero, Constant(no_breakpoint));
  comment("There is a breakpoint in the code, so that means that eax has the correct opcode");
  comment("So just jmp directly without using esi");
  andl(eax, Constant(0xFF));
  movl(ebx, eax);
  jmp(Address(no_reg, ebx, times_4, Constant("interpreter_dispatch_table")));
#else
  dispatch_next(0);
#endif
  entry_end(); // interpreter_call_vm_redo
}

void InterpreterStubs::generate_interpreter_throw_exceptions() {
  comment_section("Interpreter exception throwers");

  entry("interpreter_throw_ArrayIndexOutOfBoundsException", 0);
  interpreter_call_vm(Constant("array_index_out_of_bounds_exception"), T_VOID);
  entry_end(); // interpreter_throw_ArrayIndexOutOfBoundsException

  entry("interpreter_throw_NullPointerException", 0);
  interpreter_call_vm(Constant("null_pointer_exception"), T_VOID);
  entry_end(); // interpreter_throw_NullPointerException

  entry("interpreter_throw_IllegalMonitorStateException", 0);
  interpreter_call_vm(Constant("illegal_monitor_state_exception"), T_VOID);
  entry_end(); // interpreter_throw_IllegalMonitorStateException

  entry("interpreter_throw_ArithmeticException", 0);
  interpreter_call_vm(Constant("arithmetic_exception"), T_VOID);
  entry_end(); // interpreter_throw_ArithmeticException

  entry("interpreter_throw_IncompatibleClassChangeError", 0);
  interpreter_call_vm(Constant("incompatible_class_change_error"), T_VOID);
  entry_end(); // interpreter_throw_IncompatibleClassChangeError

  if (GenerateDebugAssembly) {
    entry("interpreter_throw_InternalStackTagException", 0);
    interpreter_call_vm(Constant("internal_stack_tag_exception"), T_VOID);
    entry_end(); // interpreter_throw_InternalStackTagException
  }
}

void InterpreterStubs::generate_interpreter_rethrow_exception_init() {
  comment_section("Interpreter rethrow exception init");
  comment("Register eax holds the exception; Interpreter state is not in registers");
  entry("interpreter_rethrow_exception_init");
#if ENABLE_JAVA_DEBUGGER
  Label skip;
  cmpb(Address(Constant("_debugger_active")), Constant(0));
  jcc(equal, Constant(skip));
  comment("push the exception object so we don't nuke it");
  push_obj(eax);
  comment("call debugger code to store relevant info about where exception happened");
  interpreter_call_vm(Constant("handle_exception_info"), T_VOID);
  pop_obj(eax, edx);
  bind(skip);
#endif
  if (GenerateInlineAsm)
      jmp(Constant("interpreter_rethrow_exception_init"));
  else
      comment("fall through to rethrow_exception"); // IMPL_NOTE: FALLTHROUGH

  entry_end(); // interpreter_rethrow_exception_init
}

void InterpreterStubs::generate_interpreter_rethrow_exception() {
  comment_section("Interpreter rethrow exception");
  comment("Register eax holds the exception; Interpreter state is not in registers");

  entry("interpreter_rethrow_exception");
  comment("Restore bytecode and locals pointers");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  comment("Mark the bytecode pointer as being inside an exception");
  addl(esi, Constant(JavaFrame::exception_frame_flag));

  comment("Clear the expression stack");
  movl(esp, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  
  comment("Push the exception on the expression stack");
  push_obj(eax);

  comment("Get exception handler bci for exception");
  interpreter_call_vm(Constant("exception_handler_bci_for_exception"), T_INT);
  
  comment("Check if we got a bci - otherwise unwind the activation");
  cmpl(eax, Constant(-1));
  jcc(equal, Constant("interpreter_unwind_activation"));
#if ENABLE_JAVA_DEBUGGER
  Label skip;
  cmpb(Address(Constant("_debugger_active")), Constant(0));
  jcc(equal, Constant(skip));
  movl(edx, eax);
  interpreter_call_vm(Constant("handle_caught_exception"), T_VOID);
  comment("Re-get exception handler bci for exception");
  interpreter_call_vm(Constant("exception_handler_bci_for_exception"), T_INT);
  bind(skip);
#endif
  comment("Convert the bytecode index into a bytecode pointer");
  movl(ecx, Address(ebp, Constant(JavaFrame::method_offset())));
  leal(esi, Address(ecx, eax, times_1, Constant(Method::base_offset())));

  // Dispatch to the exception handler.
  dispatch_next();

  entry_end(); // interpreter_rethrow_exception
}

void InterpreterStubs::generate_interpreter_unwind_activation() {
  comment_section("Interpreter unwind activation");
  entry("interpreter_unwind_activation");

  comment("The exception is the single item on the interpreter stack");

  comment("Unlock and remove the activation");
  unlock_activation(true);

  comment("Remove exception from the stack");
  pop_obj(eax, eax);

  remove_activation(edx);
  get_thread(ecx);
  
  jmp(Constant("shared_code_for_handling_of_exception_forwarding"));

  entry_end(); // interpreter_unwind_activation
}

#if ENABLE_EMBEDDED_CALLINFO
void InterpreterStubs::generate_interpreter_fill_in_tags() {
  comment_section("Interpreter fill in tags");
  entry("interpreter_fill_in_tags");
  comment("eax: return address of method");
  comment("ebx: method");
  comment("ecx: size of parameters.  Guaranteed to be >= 1");
  comment("edx: call info from call site");
  comment("Must preserve eax, ebx, ecx");

  // stack layout:
  //   sp return address of caller
  //      --> argument n
  //      -->    ...
  //      --> argument 0

  Label extended_call_info;

  comment("Compact call info or normal call info?");
  testl(edx, edx); 
  jcc(positive, Constant(extended_call_info));

  Label loop_entry, loop_condition;
  comment("We have a compact call info");
  movl(edi, ecx);

bind(loop_entry);
  decl(edi);
  comment("Store int tag");
  movl(Address(esp, edi, times_8, Constant(BytesPerWord)), Constant(int_tag));
  comment("Test the bit in the call info");
  GUARANTEE(CallInfo::format1_tag_start == 0, "Tag must start at bit position 0 for this code to work");
  btl(edx, edi);
  jcc(carry_clear, Constant(loop_condition));
  comment("Store obj tag");
  movl(Address(esp, edi, times_8, Constant(BytesPerWord)), Constant(obj_tag));
  bind(loop_condition);
  testl(edi, edi);
  jcc(not_zero, Constant(loop_entry));
  ret();

bind(extended_call_info);
  comment("Normal call info");
  // The following code is slightly complicated.  "Bit offset" below
  // pretends like the callinfo's are in a bit array, as follows:
  //     Callinfo describing bci and offset
  //     Size [16 bits] and stack info 0-3
  //     Stack info 4-11
  // We ignore the fact that each of these words is preceded by a byte
  // that makes it look like an instruction.
  pushl(ecx); 
  pushl(ebx);
  Label loopx_entry, loopx_done;
 
  comment("Bit offset of first argument in CallInfo array");
  movzxw(edx, Address(eax, Constant(5 + 1)));  // total number of locals/expr
  subl(edx, ecx);               // number of locals/expr belonging to callee
  shll(edx, Constant(2));       // number of bits per nybble
  addl(edx, Constant(32 + 16)); // 48 bits is the 32 bit callinfo and 16bit size info

  comment("Decrement argument count; move to more convenient register");
  leal(esi, Address(ecx, Constant(-1)));

  comment("Location of tag of esi-th local");
  leal(ebx, Address(esp, Constant(3 * BytesPerWord)));

bind(loopx_entry);
  comment("eax holds the return address");
  comment("ebx holds address of the esi-th tag");
  comment("esi is the local whose tag we are setting");
  comment("edx contains the bit offset of Local 0 in the CallInfo array");
  comment("Get bit offset of esi-th local");
  leal(ecx, Address(edx, esi, times_4));

  comment("From bit offset, get word offset, then multiply by 5");
  movl(edi, ecx);
  shrl(edi, Constant(5));
  leal(edi, Address(edi, edi, times_4));

  comment("Get the appropriate CallInfo word; extract the nybble");
  movl(edi, Address(eax, edi, times_1, Constant(1)));
  shrl(edi);
  andl(edi, Constant(0xF));

  comment("Tag is (1 << value) >> 1.  This is 0 when value == 0");
  movl(ecx, edi);
  movl(edi, Constant(1));
  shll(edi);
  shrl(edi, Constant(1));

  comment("Store the tag");
  movl(Address(ebx), edi);

  comment("Are we done?");
  decl(esi); 
  addl(ebx, Constant(8));
  testl(esi, esi);
  jcc(greater_equal, Constant(loopx_entry));
bind(loopx_done);
  popl(ebx); 
  popl(ecx);
  ret();

  entry_end(); // interpreter_fill_in_tags
}
#endif // ENABLE_EMBEDDED_CALLINFO

void InterpreterStubs::generate_interpreter_deoptimization_entry() {
  comment_section("Interpreter deoptimization entry");
  entry("interpreter_deoptimization_entry");

  // Define an interpreter call info.
  define_call_info();

  comment("Restore bytecode and locals pointers");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  // Dispatch to the next bytecode.
  dispatch_next();

  entry_end(); // interpreter_deoptimization_entry
}

void InterpreterStubs::generate_interpreter_timer_tick() {
  comment_section("Interpreter call timer_tick");
  entry("interpreter_timer_tick");
  interpreter_call_vm(Constant("timer_tick"), T_VOID);
  dispatch_next();
  entry_end(); // interpreter_timer_tick

#if ENABLE_PAGE_PROTECTION
  stop_code_segment();
  start_data_segment();
  if (GenerateGNUCode || GenerateInlineAsm) {
    align(PROTECTED_PAGE_SIZE);
    define_array_begin("unsigned char", "_protected_page");
    for (int i = 0; i < PROTECTED_PAGE_SIZE; i++) {
      define_byte_element(Constant(0));
    }
    define_array_end();
  } else {
    // MASM doesn't allow 4096-byte alignment,
    // so surround the protected area with 4K padding.
    // This will certainly add 8K of static footprint,
    // but who cares about the size of win32_i386 binary!
    define_byte(Constant(0), PROTECTED_PAGE_SIZE);
    define_long(Constant(0), PROTECTED_PAGE_SIZE / BytesPerWord,
                "_protected_page");
    define_byte(Constant(0), PROTECTED_PAGE_SIZE);
  }
  stop_data_segment();
  start_code_segment();
#endif
}

#endif // ENABLE_INTERPRETER_GENERATOR
