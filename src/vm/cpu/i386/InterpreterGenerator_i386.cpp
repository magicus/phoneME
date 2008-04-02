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
#include "incls/_InterpreterGenerator_i386.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

#define  LOAD_CPOOL(reg)                                            \
 {                                                                  \
  Label method_pool, done_pool;                                     \
  load_unsigned_word(reg, Address(ebx, Constant(Method::access_flags_offset()))); \
  testl(reg, Constant(JVM_ACC_HAS_COMPRESSED_HEADER));              \
  jcc(zero, Constant(method_pool));                                 \
  comment("load ROM constants pool");                               \
  movl(reg, Address(Constant("_rom_constant_pool")));               \
  jmp(Constant(done_pool));                                         \
 bind(method_pool);                                                 \
  comment("load method specific constants pool");                   \
  movl(reg, Address(ebx, Constant(Method::constants_offset())));    \
 bind(done_pool);                                                   \
  addl(reg, Constant(ConstantPool::base_offset()));                 \
  pushl(reg);                                                       \
 }

void InterpreterGenerator::generate() {
  stop_code_segment();
  start_data_segment();

  generate_fast_globals();

  stop_data_segment();
  start_code_segment();

  generate_interpreter_signature();
  generate_interpreter_grow_stack();
  generate_interpreter_method_entry();
  for (int i = 0; i < 5; i++) {
    generate_interpreter_fast_method_entry(i);
  }
  generate_quick_native_method_entry(T_VOID);
  generate_quick_native_method_entry(T_INT);
  generate_quick_native_method_entry(T_OBJECT);

  generate_interpreter_bytecode_templates();
  generate_interpreter_dispatch_table();

  if (ENABLE_FAST_MEM_ROUTINES) {
    generate_fast_memroutines();
  }

  stop_code_segment();
  start_data_segment();

  generate_interpreter_bytecode_counters_table();
  generate_interpreter_pair_counters_table();  
  
#if ENABLE_FLOAT
  // generate globals to store FPU CW
  define_long_variable("int",   "local_saved_cw",   Constant(0));
  define_long_variable("int",   "local_masked_cw",  Constant(0));
  define_word_variable("short", "native_saved_cw",  Constant(0));
  define_word_variable("short", "native_masked_cw", Constant(0));
#endif  

  stop_data_segment();
  start_code_segment();
}

void InterpreterGenerator::generate_interpreter_signature() {
  comment_section("Interpreter signature");
  define_long_variable("int", "assembler_loop_type",
                       Constant(AssemblerLoopFlags::get_flags()));
  if (GenerateDebugAssembly) {
    comment("This is (never executed) code that uses data only available in debug builds.");
    entry("_ensure_debug");
      jmp(Constant("please_use_optimized_interpreter_with_release_or_product_builds"));
    entry_end();
  }
}

void InterpreterGenerator::generate_interpreter_grow_stack() {
  comment_section("Interpreter grow stack");
  rom_linkable_entry("interpreter_grow_stack");

  comment("slow case, re-allocate the stack then re-try this method entry");
  comment("first make the stack look correct for the call into the VM");
  comment("on entry, ebx = method ptr, edi = locals, esi = bcp");

  comment("get local return address; leave method return address on stack");
  popl(edx);

  comment("Setup frame pointer");
  pushl(ebp);
  movl(ebp, esp);

  comment("Store the method pointer on the stack");
  pushl(ebx);

  comment("Store the constant pool pointer on the stack");
  LOAD_CPOOL(esi);  

  comment("Save the current locals pointer (edi)");
  pushl(edi);

  comment("Put a bytecode pointer in the byte code pointer store (bci == 0)");
  leal(esi, Address(ebx, Constant(Method::base_offset() +
                                  JavaFrame::overflow_frame_flag)));
  pushl(esi);
 
  comment("Setup pointer to expression stack bottom");
  pushl(ebp);
  movl(Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())), esp);

  comment("Save local return address as an integer on the stack");
  push_int(edx);

  comment("get derived stack pointer value into extra argument");
  movl(edx, ecx);

  interpreter_call_vm(Constant("stack_overflow"), T_VOID);

  comment("get method return address back");
  pop_int(edx, edx);

  comment("restore method pointer");
  movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
  comment("restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));
  comment("clean up the stack");
  addl(esp, Constant(JavaFrame::frame_desc_size() - 2*BytesPerWord));
  comment("get frame pointer back");
  popl(ebp);
  comment("get original return address on the stack");
  popl(eax);

  // Push return address back onto the stack
  pushl(edx);
  comment("restore count of parameters");
  movzxw(ecx, Address(ebx, Constant(
      Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
  comment("Get the size of the locals");
  load_unsigned_word(edx, Address(ebx, Constant(Method::max_locals_offset())));

  comment("Regenerate bytecode pointer");
  leal(esi, Address(ebx, Constant(Method::base_offset())));

  ret();
  rom_linkable_entry_end(); // interpreter_grow_stack
}

void InterpreterGenerator::generate_interpreter_method_entry() {
  comment_section("Interpreter method entry (method in ebx)");
  rom_linkable_entry("fixed_interpreter_method_entry");
  jmp(Constant("interpreter_method_entry"));
  rom_linkable_entry_end(); // fixed_interpreter_method_entry

  rom_linkable_entry("interpreter_method_entry");

#if ENABLE_JAVA_DEBUGGER

  Label normal_call;
  cmpb(Address(Constant("_debugger_active")), Constant(0));
  jcc(equal, Constant(normal_call));
  comment("Get execution entry point");
  movl(eax, Address(ebx, Constant(Method::variable_part_offset())));
  movl(eax, Address(eax));
  comment("See if this method is a ROM method being debugged");
  cmpl(eax, Constant("shared_invoke_debug", true, 0));
  jcc(not_equal, Constant(normal_call));
  comment("get code base");
  movl(edx, ebx);
  movl(eax, Constant("get_rom_debug_method"));
  call_shared_call_vm(T_INT);
  movl(ebx, eax);
  bind(normal_call);
#endif
  // record this method so that it gets a higher chance of being compiled
  update_interpretation_log();

  comment("Get the method parameter size");
  movzxw(ecx, Address(ebx, 
         Constant(Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
  comment("Get the return address");
  popl(eax);

#if ENABLE_EMBEDDED_CALLINFO
  if (TaggedJavaStack) {
    Label(no_tagging);
    comment("No need to tag the stack if no arguments");
    testl(ecx, ecx);
    jcc(zero, Constant(no_tagging));
  
    comment("Get the call info from the return address");
    movl(edx, Address(eax, Constant(1))); // skip test opcode
    testl(edx, edx);
    jcc(zero, Constant(no_tagging));

    call(Constant("interpreter_fill_in_tags"));
    bind(no_tagging);
  }
#else
  GUARANTEE(!TaggedJavaStack, "Tagged stack not supported");
#endif // ENABLE_EMBEDDED_CALLINFO

  // After moving the stacks into the heap, we can no longer guarantee that
  // the stacks are 8-byte aligned.
  /*if (GenerateDebugAssembly) {
    comment("Check that the stack is properly aligned");
    testl(esp, Constant(7));
    jcc(zero, Constant("interpreter_throw_InternalStackAlignmentException"));
  }*/

  comment("Compute the start of the parameters - the locals pointer");
  leal(edi, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4,
                    Constant(-BytesPerWord)));
  comment("Put a bytecode pointer in the byte code pointer store (bci == 0)");
  leal(esi, Address(ebx, Constant(Method::base_offset())));

  comment("Get the size of the locals");
  load_unsigned_word(edx, Address(ebx, Constant(Method::max_locals_offset())));

  comment("Setup and clear the locals");
  Label loop_start, loop_end;
  subl(edx, ecx);

  // we need to check for potential stack overflow but we can't just call
  // check_stack_overflow since that makes assumptions about what the current
  // frame looks like.  We're sort of in the middle of creating this frame.
  // So what we do is set a flag on the stack to say we need to expand this
  // stack, fake the locals pointer to be zero locals re-allocate the stack
  // then continue.

  Label stack_ok;
  comment("First, let's see if we have enough stack available");
  movzxw(ecx, Address(ebx, Constant(Method::max_execution_stack_count_offset())));
  shll(ecx, Constant(LogBytesPerStackElement));
  addl(ecx, Constant(JavaFrame::frame_desc_size()));
  negl(ecx);
  addl(ecx, esp);
  comment("get lowest stack value in ecx");
  cmpl(ecx, Address(Constant("_current_stack_limit")));
  jcc(above, Constant(stack_ok));
  pushl(eax);

  call(Constant("interpreter_grow_stack"));

  subl(edx, ecx);

  bind(stack_ok);
  if (TaggedJavaStack) {
    cmpl(edx, Constant(0));
    jcc(equal, Constant(loop_end));
    bind(loop_start);
    pushl(Constant(0));
    pushl(Constant(uninitialized_tag));
    decl(edx);
    jcc(not_zero, Constant(loop_start));
    bind(loop_end);
  } else {
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
    cmpl(edx, Constant(0));
    jcc(equal, Constant(loop_end));
    bind(loop_start);
    pushl(Constant(0));
    decl(edx);
    jcc(not_zero, Constant(loop_start));
    bind(loop_end);
#else
    shll(edx, Constant(LogBytesPerStackElement));
    subl(esp, edx);
#endif
  }

  comment("Push the return address");
  pushl(eax);

  comment("Setup frame pointer");
  pushl(ebp);
  movl(ebp, esp);

  comment("Store the method pointer on the stack");
  pushl(ebx);

  comment("Save the constant pool pointer on the stack");
  LOAD_CPOOL(eax); 

  comment("Save the current locals pointer (edi)");
  pushl(edi);

  comment("Put a bytecode pointer in the byte code pointer store (bci == 0)");
  pushl(esi);
 
  comment("Setup pointer to expression stack bottom");
  pushl(ebp);
  movl(Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())), esp);

  comment("Check if the method is synchronized");
  Label synchronize, synchronization_done;
  load_unsigned_word(eax, Address(ebx, 
                                  Constant(Method::access_flags_offset())));
  testl(eax, Constant(JVM_ACC_SYNCHRONIZED));
  jcc(not_zero, Constant(synchronize));
  bind(synchronization_done);

  if (GenerateDebugAssembly && TaggedJavaStack) {
    // Check the interpreter parameters
    interpreter_call_vm(Constant("check_interpreter_parameter_tags"), T_VOID);
  }
  /*
  if (ENABLE_PERFORMANCE_COUNTERS) {
    Label not_main;
    movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
    load_unsigned_word(eax, Address(ebx, 
                                    Constant(Method::access_flags_offset())));
    movl(ecx, eax);
    andl(ecx, Constant(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    cmpl(ecx, Constant(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    jcc(not_zero, Constant(not_main));
    andl(eax, Constant(~JVM_ACC_PRIVATE));
    movw(Address(ebx, Constant(Method::access_flags_offset())), eax);
    interpreter_call_vm(Constant("jvm_set_main_hrtick_count"), T_VOID);
    bind(not_main);
  }
  */
  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
  }
  check_timer_tick();

  // Dispatch to the first bytecode
  dispatch_next();

  // Synchronization code.  
  Label synchronize_eax, static_method;
  bind(synchronize); 
  comment("Synchronized method - register eax holds the access flags");
  testl(eax, Constant(JVM_ACC_STATIC));
  jcc(not_zero, Constant(static_method));

  comment("Instance method - get receiver from parameter 1");  
  movl(eax, Address(edi));

  bind(synchronize_eax);
  comment("Object to synchronize on is in register eax");
  decrement(esp, BytesPerWord + StackLock::size());

  comment("Correct the stack bottom pointer");
  movl(Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())), esp);

  comment("Lock the stack lock");
  // Next instruction may not be necessary, since the esi is already
  // stored on the stack.
  save_interpreter_state();
  call_from_interpreter(Constant("shared_lock_synchronized_method"));
  restore_interpreter_state();
  // movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
  jmp(Constant(synchronization_done));

  bind(static_method);
  comment("Static method - synchronize on the mirror of the holder of "
          "constant pool of current method");
#if ENABLE_ISOLATES
  Label class_is_initialized;
  movzxw(eax, Address(ebx, Constant(Method::holder_id_offset())));
  // Class initialization barrier can be implemented by testing if
  // the task mirror has an address strictly greater than the marker.
  // ( 0 < marker < any task mirror of an initialized class)
  comment("load task mirror from class list in eax");
  load_task_mirror_from_list(eax, ecx);
  cib_with_marker(eax, class_is_initialized);
  comment("Class must be being initialized by the current thread then");
  comment("Get the class to search the clinit list of current thread");
  movzxw(ecx, Address(ebx, Constant(Method::holder_id_offset())));
  load_class_from_list(ecx, eax);
  get_mirror_from_clinit_list(eax, ecx);
  bind(class_is_initialized);
  // load the instance of java.lang.Class
  movl(eax, Address(eax, Constant(TaskMirror::real_java_mirror_offset())));
#else
  movzxw(ecx, Address(ebx, Constant(Method::holder_id_offset())));
  load_class_from_list(ecx, eax);
  movl(eax, Address(ecx, Constant(JavaClass::java_mirror_offset())));
#endif
  jmp(Constant(synchronize_eax));

  rom_linkable_entry_end(); // interpreter_method_entry
}

void InterpreterGenerator::generate_interpreter_fast_method_entry(int extra_locals) {
  GUARANTEE(extra_locals >= 0, "Sanity check");

  comment_section("Fast interpreter method entry (%d extra local%s)", 
                     extra_locals, (extra_locals != 1 ? "s" : ""));
  char name[64], fixed_name[74];
  sprintf(name, "interpreter_fast_method_entry_%d", extra_locals);
  sprintf(fixed_name, "fixed_%s", name);

  rom_linkable_entry(fixed_name);
  jmp(Constant(name));
  rom_linkable_entry_end(); // fixed_interpreter_fast_method_entry_%d

  rom_linkable_entry(name);

  // record this method so that it gets a higher chance of being compiled
  update_interpretation_log();

  comment("Get the size of the parameters");
  load_unsigned_word(ecx, Address(ebx, 
      Constant(Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
  comment("Pop the return address");
  popl(eax);

  if (TaggedJavaStack) { 
    Label no_tagging;
    comment("No need to tag the stack if no arguments");
    testl(ecx, ecx);
    jcc(zero, Constant(no_tagging));
  
    comment("Get the call info from the return address");
    movl(edx, Address(eax, Constant(1))); // skip test opcode
    testl(edx, edx);
    jcc(zero, Constant(no_tagging));

    call(Constant("interpreter_fill_in_tags"));
    bind(no_tagging);
  }

  comment("Get the bytecode pointer from the method");
  leal(esi, Address(ebx, Constant(Method::base_offset())));
  
  comment("Compute the start of the parameters - the locals pointer");
  leal(edi, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4,
                    Constant(-BytesPerWord)));

  if (extra_locals >= 1) {
    comment("Push cleared locals");
  }

  // we need to check for potential stack overflow but we can't just call
  // check_stack_overflow since that makes assumptions about what the current
  // frame looks like.  We're sort of in the middle of creating this frame.
  // So what we do is set a flag on the stack to say we need to expand this
  // stack, fake the locals pointer to be zero locals re-allocate the stack
  // then continue.

  Label stack_ok;
  comment("First, let's see if we have enough stack available");
  movzxw(ecx, Address(ebx, Constant(Method::max_execution_stack_count_offset())));
  shll(ecx, Constant(LogBytesPerStackElement));
  addl(ecx, Constant(JavaFrame::frame_desc_size()));
  negl(ecx);
  addl(ecx, esp);
  comment("get lowest stack value in ecx");
  cmpl(ecx, Address(Constant("_current_stack_limit")));
  jcc(above, Constant(stack_ok));

  pushl(eax);
  call(Constant("interpreter_grow_stack"));
  subl(edx, ecx);

  bind(stack_ok);

  if (TaggedJavaStack) { 
    for (int i = 0; i < extra_locals; i++) {
      pushl(Constant(0));
      pushl(Constant(uninitialized_tag));
    }
  } else {
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
    for (int i = 0; i < extra_locals; i++) {
      pushl(Constant(0));
    }
#else
    subl(esp, Constant(4 * extra_locals));
#endif
  }
  
  comment("Push the return address");
  pushl(eax);
  
  comment("Setup frame pointer");
  pushl(ebp);
  movl(ebp, esp);

  comment("Push the method onto the stack");
  pushl(ebx);

  comment("Push constants pool on stack");
  LOAD_CPOOL(ecx);

  comment("Push the locals pointer onto the stack");
  pushl(edi);

  comment("Push the bytecode pointer onto the stack");
  pushl(esi);

  comment("Setup the stack bottom pointer");
  pushl(ebp);
  movl(Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())), esp);

  if (GenerateDebugAssembly && TaggedJavaStack) {
    // Check the interpreter parameters
    interpreter_call_vm(Constant("check_interpreter_parameter_tags"), T_VOID);
  }
  /*
  if (ENABLE_PERFORMANCE_COUNTERS) {
    Label not_main;
    movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
    load_unsigned_word(eax, Address(ebx, 
                                    Constant(Method::access_flags_offset())));
    movl(ecx, eax);
    andl(ecx, Constant(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    cmpl(ecx, Constant(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    jcc(not_zero, Constant(not_main));
    andl(eax, Constant(~JVM_ACC_PRIVATE));
    movw(Address(ebx, Constant(Method::access_flags_offset())), eax);
    interpreter_call_vm(Constant("jvm_set_main_hrtick_count"), T_VOID);
bind(not_main);
  }
  */
  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
  }

  check_timer_tick();

  // Dispatch to the first bytecode.
  dispatch_next();

  rom_linkable_entry_end(); // interpreter_fast_method_entry_%d
}

void
InterpreterGenerator::generate_quick_native_method_entry(BasicType return_type)
{
  GUARANTEE(word_size_for(return_type) <= 1,
            "two-word return values not supported");
  char *type;
  switch (return_type) {
  case T_OBJECT:
    type = "obj";
    break;
  case T_INT:
    type = "int";
    break;
  case T_VOID:
  default:
    type = "void";
  }
  comment_section("Quick native method entry (%s)", type);
  char name[64];
  sprintf(name, "quick_%s_native_method_entry", type);
  rom_linkable_entry(name);

  if (GenerateDebugAssembly && TaggedJavaStack) {
    comment("Quick native methods are supported only for non-tagged stack.");
    comment("Natives::rewrite_quick_native_function() should have checked!");
    int3();
  }

  wtk_profile_quick_call();

  comment("Get the method parameter size");
  movzxw(ecx, Address(ebx, 
      Constant(Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
  
  comment("As we're called using \"call\" one more argument on stack"
          " is return address");
  leal(ecx, Address(esp, ecx, times_4));

  comment("Set space for fake parameter for static method (KNI-ism)");
  load_unsigned_word(eax, Address(ebx, 
                                  Constant(Method::access_flags_offset())));

  if (GenerateDebugAssembly) {
    Label ok;
    comment("QuickNative method must not be synchronized.");
    testl(eax, Constant(JVM_ACC_SYNCHRONIZED));
    jcc(zero, Constant(ok));
    int3();
    bind(ok);
  }

  andl(eax, Constant(JVM_ACC_STATIC));
  cmpl(eax, Constant(JVM_ACC_STATIC));
  Label set_param, done;
  jcc(not_zero, Constant(set_param));
  addl(ecx, Constant(BytesPerStackElement));
  
  bind(set_param);
  comment("Point _kni_parameter_base to the first parameter");
  movl(Address(Constant("_kni_parameter_base")), ecx);


  comment("Get the quick native function entry and call it");
  movl(eax, Address(ebx, Constant(Method::quick_native_code_offset())));

  if (TraceNativeCalls) {
    trace_native_call(eax);
  }

  comment("Tell VM we're in quick native methods");
  movl(Address(Constant("_jvm_in_quick_native_method")), Constant(1));

  pushl(ebx);
  pushl(eax);
  call(Constant("call_on_primordial_stack"));

  if (word_size_for(return_type) == 1) {
    comment("return value is in eax");
  }

  comment("clear junk from stack");
  popl(ebx);

  comment("restore ebx");
  popl(ebx);

  comment("Get the return address to my caller");
  popl(esi);

  comment("Get the number of my parameters and pop them");
  movzxw(ecx, Address(ebx, 
      Constant(Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
  leal(esp, Address(esp, ecx, times_4));

  if (word_size_for(return_type) == 1) {
    comment("Push return value (1 word)");
    pushl(eax);
  }

  movl(Address(Constant("_jvm_in_quick_native_method")), Constant(0));

  comment("Check if native method has generated exception");
  cmpl(Address(Constant("_jvm_quick_native_exception")), Constant(0));
  jcc(zero, Constant(done));

  comment("Push return address of my caller");
  pushl(esi);

  comment("Let the exception be thrown by System.quickNativeThrow()");
  movl(ebx, Address(Constant("persistent_handles",
            (Universe::quick_native_throw_method_index) * BytesPerWord)));

  if(AddExternCUnderscore) {
    emit_instruction("jmp  _interpreter_method_entry");
  } else {
    emit_instruction("jmp  interpreter_method_entry");
  }

bind(done);
  comment("Jump to return address");
  jmp(esi);

  rom_linkable_entry_end(); // quick_%s_native_method_entry
}

void InterpreterGenerator::generate_interpreter_bytecode_templates() {
  // Initialize the template table.
  TemplateTable::initialize(this);

  // Generate the bytecode templates for all the bytecodes.
  comment_section("Interpreter bytecode templates");
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    Bytecodes::Code bc = (Bytecodes::Code) i;
    if (Bytecodes::is_defined(bc)) {
      if (!TaggedJavaStack && !GenerateInlineAsm && TemplateTable::is_duplicate(bc)) {
        Bytecodes::Code duplicate = TemplateTable::get_duplicate(bc);
        comment("%s is a duplicate of %s", 
                Bytecodes::name(bc), Bytecodes::name(duplicate));
      } else { 
        generate_interpreter_bytecode_templates_for(bc);
      }
    }
  }
}

void InterpreterGenerator::generate_interpreter_bytecode_templates_for(Bytecodes::Code bc) {
  GUARANTEE(Bytecodes::is_defined(bc), "Cannot generate template for undefined bytecode");
  bool has_wide_variant = Bytecodes::wide_is_defined(bc);
  Template* t = TemplateTable::template_for(bc);

#if !ENABLE_FLOAT
  if (t->does_floating_point()) {
    return;
  }
#endif

  GUARANTEE(t != 0, "Cannot generate code for bytecode without a template");

  if (has_wide_variant) {
    Label wide_code_snippet;
    Template* wt = TemplateTable::template_for_wide(bc);
    GUARANTEE(wt != 0,
              "Cannot generate code for wide bytecode without a template");

    comment("WIDE BYTECODE: %s", Bytecodes::name(bc));

    // also note duplicate bytecodes that will be merged with this one
    if (!TaggedJavaStack && !GenerateInlineAsm) {
      for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
        Bytecodes::Code bcx = (Bytecodes::Code) i;
        if (Bytecodes::is_defined(bcx)
                 && TemplateTable::is_duplicate(bcx)
                 && TemplateTable::get_duplicate(bcx) == bc) {
          comment("WIDE BYTECODE: %s", Bytecodes::name(bcx));
        }
      }
    }

    // Generate the template for the wide variant of the bytecode.
    if (!GenerateInlineAsm) {
      // in the pure assembler output the entry point for the wide
      // bytecode is unnamed because bc_wide finds it in the dword
      // preceding the entry point of the non-wide bytecode.
      bind(wide_code_snippet, wt->must_align() ? 16 : 0);
      generate_interpreter_bytecode_template(wt, true);

      comment("Data containing entry for wide bytecode");
      if (t->must_align())
        align(16);
      define_long(Constant(wide_code_snippet));
    }
    else {
      // for inline assmebler we need an explicit entry point (i.e. function)
      char wide[256];
      sprintf(wide, "bc_wide_%s", Bytecodes::name(t->bytecode()));

      entry(wide);
      generate_interpreter_bytecode_template(wt, true);
    }
  }

  // Generate the template for the non-wide variant of the bytecode.
  char buffer[256];
  sprintf(buffer, "bc_%s", Bytecodes::name(bc));

  entry(buffer, (t->must_align() && !has_wide_variant) ? 16 : 0);

  // additional entry points for duplicate bytecodes
  if (!TaggedJavaStack && !GenerateInlineAsm) {
    for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
      Bytecodes::Code bcx = (Bytecodes::Code) i;
      if (Bytecodes::is_defined(bcx)
             && TemplateTable::is_duplicate(bcx)
             && TemplateTable::get_duplicate(bcx) == bc) {
          sprintf(buffer, "bc_%s", Bytecodes::name(bcx));
          alt_entry(buffer);
      }
    }
  }

  generate_interpreter_bytecode_template(t, false);
}

void InterpreterGenerator::generate_interpreter_bytecode_template(Template* t, bool is_wide) {
  int step = is_wide ? Bytecodes::wide_length_for(t->bytecode())
                     : Bytecodes::length_for(t->bytecode());

  if (!t->dispatches()) {
    dispatch_prologue(step);
  }

  // Generate call to runtime system to trace bytecodes.
  if (TraceBytecodes) {
    Label(skip);
    cmpl(Address(Constant("TraceBytecodes")), Constant(0));
    jcc(zero, Constant(skip));
    interpreter_call_vm(Constant("trace_bytecode"), T_VOID);
    bind(skip);
  }
  
  if (Deterministic) {
    Label done;

    movl(ecx, Address(Constant("_bytecode_counter")));
    decl(ecx);
    jcc(not_zero, Constant(done));

    movl(ecx, Constant(-1));
    movl(Address(Constant("_current_stack_limit")), ecx);

    movl(ecx, Constant(RESCHEDULE_COUNT/10));
    bind(done);
    movl(Address(Constant("_bytecode_counter")), ecx);
  }  

  // Update the bytecode histogram.
  if (PrintBytecodeHistogram) {
    increment_bytecode_counter(t->bytecode());
  }
  if (PrintPairHistogram) {
    increment_pair_counter(t->bytecode());
  }
#if ENABLE_JAVA_DEBUGGER
  switch(t->bytecode()) {
  case Bytecodes::_getstatic:
  case Bytecodes::_putstatic:
  case Bytecodes::_getfield:
  case Bytecodes::_putfield:
  case Bytecodes::_ldc:
  case Bytecodes::_ldc_w:
  case Bytecodes::_ldc2_w:
  case Bytecodes::_invokeinterface:
  case Bytecodes::_invokestatic:
  case Bytecodes::_invokespecial:
  case Bytecodes::_invokevirtual:
  case Bytecodes::_breakpoint:
    break;
  default:
    Label not_stepping;
    testl(Address(Constant("_debugger_active")),
          Constant(DEBUGGER_STEPPING));
    jcc(equal, Constant(not_stepping));
    interpreter_call_vm(Constant("handle_single_step"), T_BOOLEAN);
    bind(not_stepping);
  }
#endif

  // _internal entry point(s)
  char internal[256];
  sprintf(internal, "bc%s_%s_internal",
          is_wide ? "_wide" : "", Bytecodes::name(t->bytecode()));

  if (GenerateInlineAsm) {
    // cannot fall through into another function, need explicit jump.
    jmp(Constant(internal));
    entry_end(); // bc_...

    entry(internal);
  }
  else {
    alt_entry(internal);

    // additional _interal entry points for duplicate bytecodes
    if (!TaggedJavaStack) {
      for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
        Bytecodes::Code bc = (Bytecodes::Code) i;
        if (Bytecodes::is_defined(bc)) {
          if (TemplateTable::is_duplicate(bc)
              && TemplateTable::get_duplicate(bc) == t->bytecode()) {
            sprintf(internal, "bc%s_%s_internal",
                    is_wide ? "_wide" : "", Bytecodes::name(bc));
            alt_entry(internal);
          }
        }
      }
    }
  }

  // Generate the code for the template.
  t->generate();

  // If the template does not dispatch itself generate the standard dispatch
  // epilogue. 
  if (!t->dispatches()) { 
      dispatch_epilogue(step);
  }

  if (GenerateDebugAssembly) {
    comment("Make sure we do not fall through between bytecodes");
    int3();
  }

  entry_end(); // bc_..._internal
}

void InterpreterGenerator::generate_interpreter_dispatch_table() {
  comment_section("Interpreter dispatch table");

  define_array_begin("void * const", "interpreter_dispatch_table");

  for (int i = 0; i < 256; i++) {
    Bytecodes::Code bc = (Bytecodes::Code) i;

#if ENABLE_FLOAT
    bool has_entry  = Bytecodes::is_defined(bc);
#else 
    bool has_entry  = Bytecodes::is_defined(bc) ? !TemplateTable::template_for(bc)->does_floating_point(): false;
#endif
    if (has_entry) {
      char buffer[256];
      strcpy(buffer, "bc_");
      strcat(buffer, Bytecodes::name(bc));
      define_long_element(Constant(buffer));
    } else {
      define_long_element(Constant(0));
    }
  }

  define_array_end(); // interpreter_dispatch_table

  if (GenerateInlineAsm) {
    define_array_begin("void * const", "interpreter_wide_dispatch_table");
    for (int i = 0; i < 256; i++) {
      const Bytecodes::Code bc = (Bytecodes::Code) i;
      if (Bytecodes::wide_is_defined(bc)) {
        char wide[256];
        sprintf(wide, "bc_wide_%s", Bytecodes::name(bc));
        define_long_element(Constant(wide));
      } else {
        define_long_element(Constant(0));
      }
    }
    define_array_end(); // interpreter_wide_dispatch_table
  }
}

void InterpreterGenerator::generate_fast_globals() {
  define_struct_begin("JVMFastGlobals", "jvm_fast_globals"); 

#define DECLARE_STRUCT_FIELD(DUMMY, type, name) \
  define_struct_field("jvm_fast_globals", "void * const", "_" XSTR(name), \
                      long_operand);

  FORALL_JVM_FAST_GLOBALS(DECLARE_STRUCT_FIELD, DUMMY);

  define_struct_end("jvm_fast_globals"); 
}

void InterpreterGenerator::generate_interpreter_bytecode_counters_table() {
  comment_section("Interpreter bytecode counters table");
  define_long_variable("int", "_bytecode_counter",
                       Constant(RESCHEDULE_COUNT/10));  
  if (PrintBytecodeHistogram) {
    if (!GenerateInlineAsm)
      define_long(Constant(0), Bytecodes::number_of_java_codes * 2,
                  "interpreter_bytecode_counters");
    else {
      define_array_begin("int", "interpreter_bytecode_counters",
                         Bytecodes::number_of_java_codes * 2);
      define_long_element(Constant(0)); // keep compiler happy
      define_array_end();
    }
  } else {
    define_array_begin("int", "interpreter_bytecode_counters");
    define_long_element(Constant(0));
    comment("The bytecode counters table is intentionally left empty.");
    comment("To use bytecode counters, turn on +PrintBytecodeHistogram");
    comment("during interpreter loop generation.");
    define_array_end();
  }
}

void InterpreterGenerator::generate_interpreter_pair_counters_table() {
  comment_section("Interpreter pair counters table");
  size_t ncounters = Bytecodes::number_of_java_codes
                         * Bytecodes::number_of_java_codes * 2;
  if (PrintPairHistogram) {
    if (!GenerateInlineAsm)
      define_long(Constant(0), ncounters, "interpreter_pair_counters");
    else {
      define_array_begin("int", "interpreter_pair_counters", ncounters);
      define_long_element(Constant(0)); // keep compiler happy
      define_array_end();
    }
  } else {
    define_array_begin("int", "interpreter_pair_counters");
    define_long_element(Constant(0));
    comment("The pair counters table is intentionally left empty.");
    comment("To use pair counters, turn on +PrintPairHistogram");
    comment("during interpreter loop generation.");
    define_array_end();
  }
}

void InterpreterGenerator::generate_fast_memroutines() {  
}

void InterpreterGenerator::print_register_definitions() {
  tty->print_cr("    ebx  current method");
  tty->print_cr("    esi  current bytecode pointer");
  tty->print_cr("    ebp  frame pointer");
  tty->print_cr("    edi  locals");
}

#endif // ENABLE_INTERPRETER_GENERATOR
