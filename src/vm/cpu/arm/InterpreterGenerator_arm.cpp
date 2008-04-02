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
#include "incls/_InterpreterGenerator_arm.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

void InterpreterGenerator::generate() {
  generate_interpreter_signature();
  // generate_test_code();
  generate_interpreter_method_entry();
  for (int i = 0; i < 5; i++) {
    generate_interpreter_fast_method_entry(i);
  }
  generate_quick_native_method_entry(T_VOID);
  generate_quick_native_method_entry(T_INT);
  generate_quick_native_method_entry(T_OBJECT);

#if GENERATE_LIBC_GLUE
  if (GenerateGNUCode) {
    // These are used only for GCC/linux/Thumb
    generate_libc_glue();
    generate_fast_routines();
  }
#endif

  if (ENABLE_FAST_MEM_ROUTINES) {
    generate_fast_memroutines();
  }

  generate_interpreter_grow_stack();
  generate_interpreter_bytecode_templates();

  generate_interpreter_dispatch_table();

  generate_interpreter_bytecode_counters_table();
  generate_interpreter_pair_counters_table();
}

void InterpreterGenerator::generate_interpreter_signature() {
  Segment seg(this, code_segment, "Interpreter signature area");
  comment_section("Interpreter signature");
  if (GenerateDebugAssembly) {
    comment("This is (never executed) code that uses data only "
            "available in debug builds.");
    static Label please("please_use_optimized_interpreter_with_release_"
                        "or_product_builds");
    import(please);
    define_long(please);
  }
  // Create the constant 1 to ind
  bind("assembler_loop_type");
  define_long(AssemblerLoopFlags::get_flags());
}

void InterpreterGenerator::generate_interpreter_method_entry() {
  Segment seg(this, code_segment, "Interpreter method entry");

  comment_section("standard interpreter method entry");

#if ENABLE_COMPILER
  bind_rom_linkable("interpreter_method_entry", true);
#else
  bind_rom_linkable("interpreter_method_entry");
#endif

#if ENABLE_JAVA_DEBUGGER
  comment("if tmp0==0: then this is a call into a romized method with a breakpoint");
  Label normal_call;

  get_debugger_active(tmp1);
  tst(tmp1, imm(DEBUGGER_ACTIVE));
  b(normal_call, eq);
  comment("Get execution entry point");
  ldr(tmp0, imm_index(callee, Method::variable_part_offset()));
  ldr(tmp0, imm_index(tmp0));
  comment("See if this method is a ROM method being debugged");
  ldr_label(tmp1, "shared_invoke_debug", false);
  cmp(tmp0, reg(tmp1));
  comment("If eq, we were called from shared_invoke_debug.");
  b(normal_call, ne);
  push(lr);
  mov(r1, reg(callee));
  interpreter_call_vm("get_rom_debug_method", T_INT, false);
  comment("r0 returns with correct method pointer");
  mov(callee, reg(r0));
  pop(lr);
bind(normal_call);
#endif

// record this method so that it gets a higher chance of being compiled
  update_interpretation_log();

  comment("get parameter size in words");
  get_method_parameter_size(tmp0, callee);

#if ENABLE_EMBEDDED_CALLINFO
  if (TaggedJavaStack) {
    comment("get the call info from the return address");
    ldr(tmp1, imm_index(lr, -BytesPerWord));

    Label tags_filled;
    comment("test for compact compiled call info");
    cmp(tmp0, zero);
    cmp(tmp1, zero, ne);
    ldr_nearby_label(locals, tags_filled, ne);
    b("interpreter_fill_in_tags", lt);
    b("interpreter_fill_in_extended_tags", gt);
    bind(tags_filled);
  }
#else
  GUARANTEE(!TaggedJavaStack, "Tagged stack not supported");
#endif // ENABLE_EMBEDDED_CALLINFO

  verify_gp_register(tmp2);

  // Set locals to point to the start of the current locals
  if (JavaStackDirection < 0) {
    add(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  } else {
    sub(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  }
  add_imm(locals, locals, JavaFrame::arg_offset_from_sp(-1));

  // we need to check for potential stack overflow but we can't just call
  // check_stack_overflow since that makes assumptions about what the current
  // frame looks like.  We're sort of in the middle of creating this frame.

  Label stack_grown;
  Register stack_limit = (callee == tos_val) ? tos_tag : tos_val;

  comment("get maximum stack execution size");
  ldrh(tmp1, imm_index3(callee, Method::max_execution_stack_count_offset()));
  get_current_stack_limit(stack_limit);
  if (JavaStackDirection < 0) {
    sub(tmp1, jsp, imm_shift(tmp1, lsl,LogBytesPerStackElement));
    sub_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(stack_limit));
    ldr_nearby_label(stack_limit, stack_grown, ls);
    b("interpreter_grow_stack", ls);
  } else {
    add(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    add_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(stack_limit));
    ldr_nearby_label(stack_limit, stack_grown, hs);
    b("interpreter_grow_stack", hs);
  }

bind(stack_grown);

  comment("get number of locals");
  ldrh(tmp1, imm_index3(callee, Method::max_locals_offset()));

  comment("pushing cleared locals");
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
    mov_imm(tmp2, 0);
    if (TaggedJavaStack) {
      mov(tmp3, imm(uninitialized_tag));
    }
    Label loop;
    sub(tmp1, tmp1, reg(tmp0), set_CC);
    bind(loop);
    push(tmp23, gt);
    sub(tmp1, tmp1, imm(1), set_CC, gt);
    b(loop, gt);
#else
  if (GenerateDebugAssembly || TaggedJavaStack) {
    mov_imm(tmp2, GenerateDebugAssembly ? 0xF000000F : 0);
    if (TaggedJavaStack) {
      mov(tmp3, imm(uninitialized_tag));
    }
    Label loop;
    sub(tmp1, tmp1, reg(tmp0), set_CC);
    bind(loop);
    push(tmp23, gt);
    sub(tmp1, tmp1, imm(1), set_CC, gt);
    b(loop, gt);
  } else {
    sub(tmp1, tmp1, reg(tmp0));
    if (JavaStackDirection < 0) {
      sub(jsp, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    } else {
      add(jsp, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    }
  }
#endif

  // Get constant pool;
  get_cpool(cpool, true /* method in Assembler::callee */);

  comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();
  add(cpool, cpool, imm(ConstantPool::base_offset()));
  // These are increasing address order.  JScale seems to like a stream of
  // writes one after the other
  str(jsp,   imm_index(jsp, offset + JavaFrame::stack_bottom_pointer_offset()));
  str(locals,imm_index(jsp, offset + JavaFrame::locals_pointer_offset()));
  str(cpool, imm_index(jsp, offset + JavaFrame::cpool_offset()));
  str(callee,imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));

  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  comment("set bcp");
  add(bcp, callee, imm(Method::base_offset()));

  comment("check if the method is synchronized");
  Label synchronize, synchronization_done;

  ldrh(tmp0, imm_index3(callee, Method::access_flags_offset()));
  tst(tmp0, imm(JVM_ACC_SYNCHRONIZED));
  b(synchronize, ne);

bind(synchronization_done);
  if (GenerateDebugAssembly && TaggedJavaStack) {
    interpreter_call_vm("check_interpreter_parameter_tags", T_VOID);
  }

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm("jprof_record_method_transition", T_VOID);
  }

  restore_stack_state_from(tos_on_stack);
  prefetch(0);
  check_timer_tick();
  dispatch(tos_interpreter_basic);

  // Synchronization code
bind_local("synchronize_interpreted_method");
bind(synchronize);
  comment("synchronize method, register tmp0(r2) holds the access flags");
  tst(tmp0, imm(JVM_ACC_STATIC));

  comment("CC = ne: Synchronize on current class");
  comment("CC = eq: Synchronize on local 0");
  ldrh(tmp3, imm_index3(callee, Method::holder_id_offset()),       ne);
  get_class_list_base(tmp2,                                        ne);
  ldr(tos_val, local_addr_at(0, 0),                                eq);
  ldr_class_from_index_and_base(tmp1, tmp3, tmp2,                  ne);
  save_interpreter_state();
#if ENABLE_ISOLATES
  Label  mirror_loaded;
  Label class_is_initialized, need_init;
  comment("if 'eq' then synchronizing on a local so skip all this mirror stuff");
  b(class_is_initialized, eq);
  get_mirror_list_base(tmp2);
  ldr(tmp2, add_index(tmp2, tmp3, lsl, times_4));
  get_task_class_init_marker(tmp3);
  cmp(tmp2, reg(tmp3));
  b(class_is_initialized, ne);
bind(need_init);
  comment("Class must be being initialized by the current thread then");
  comment("Get the class to search the clinit list of current thread");
  comment("get_mirror_from_clinit_list(mirror, klass, temp reg");
  get_mirror_from_clinit_list(tmp2, tmp1, tmp3);
  ldr(tos_val, imm_index(tmp2, TaskMirror::real_java_mirror_offset()));
  b(mirror_loaded);
bind(class_is_initialized);
  ldr(tos_val, imm_index(tmp2, TaskMirror::real_java_mirror_offset()),   ne);
bind(mirror_loaded);
#else
  ldr(tos_val, imm_index(tmp1, JavaClass::java_mirror_offset()),   ne);
#endif

  comment("object to lock is in %s", reg_name(tos_val));
  call_from_interpreter("shared_lock_synchronized_method");
  restore_interpreter_state();
  b(synchronization_done);
}

// Note: fast entries are not synchronized
void
InterpreterGenerator::generate_interpreter_fast_method_entry(int extra_locals) {
  GUARANTEE(extra_locals >= 0, "sanity check");

  Segment seg(this, code_segment);
  comment_section("Fast interpreter method entry (%d extra locals)",
                  extra_locals);
  char name[64];
  jvm_sprintf(name, "interpreter_fast_method_entry_%d", extra_locals);
  bind_rom_linkable(name, true);

  // record this method so that it gets a higher chance of being compiled
  update_interpretation_log();

  comment("get parameter size in words");
  get_method_parameter_size(tmp0, callee);

#if ENABLE_EMBEDDED_CALLINFO
  if (TaggedJavaStack) {
    comment("get the call info from the return address");
    ldr(tmp1, imm_index(lr, -BytesPerWord));

    Label tags_filled;
    comment("test for compact compiled call info");
    cmp(tmp0, zero);
    cmp(tmp1, zero, ne);
    ldr_nearby_label(locals, tags_filled, ne);
    b("interpreter_fill_in_tags", lt);
    b("interpreter_fill_in_extended_tags", gt);
    bind(tags_filled);
  }
#else
  GUARANTEE(!TaggedJavaStack, "Tagged stack not supported");
#endif // ENABLE_EMBEDDED_CALLINFO

  verify_gp_register(tmp2);

  // Set locals to point to the start of the current locals
  if (JavaStackDirection < 0) {
    add(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  } else {
    sub(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  }
  add_imm(locals, locals, JavaFrame::arg_offset_from_sp(-1));

  // we need to check for potential stack overflow but we can't just call
  // check_stack_overflow since that makes assumptions about what the current
  // frame looks like.  We're sort of in the middle of creating this frame.

  Label stack_grown;
  Register stack_limit = (callee == tos_val) ? tos_tag : tos_val;

  comment("get maximum stack execution size");
  ldrh(tmp1, imm_index3(callee, Method::max_execution_stack_count_offset()));
  get_current_stack_limit(stack_limit);
  if (JavaStackDirection < 0) {
    sub(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    sub_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(stack_limit));
    ldr_nearby_label(stack_limit, stack_grown, ls);
    b("interpreter_grow_stack", ls);
  } else {
    add(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    add_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(stack_limit));
    ldr_nearby_label(stack_limit, stack_grown, hs);
    b("interpreter_grow_stack", hs);
  }
bind(stack_grown);

  if (extra_locals >= 1) {
    Register tmp = (callee == tos_val) ? tos_tag : tos_val;
    comment("push cleared locals");
    add_imm(jsp, jsp, BytesPerStackElement * extra_locals * JavaStackDirection);
    if (TaggedJavaStack) {
      mov(tmp, imm(uninitialized_tag));
      for (int i = 0; i < extra_locals; i++) {
        str(tmp,
            imm_index(jsp, JavaFrame::arg_offset_from_sp(i) + BytesPerWord));
      }
    }
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
    mov(tmp, imm(0));
    for (int i = 0; i < extra_locals; i++) {
      str(tmp, imm_index(jsp, JavaFrame::arg_offset_from_sp(i)));
    }
#else
    if (GenerateDebugAssembly) {
      mov(tmp, imm(0xF000000F));
      for (int i = 0; i < extra_locals; i++) {
        str(tmp, imm_index(jsp, JavaFrame::arg_offset_from_sp(i)));
      }
    }
#endif
  }

  // Get constant pool
  get_cpool(cpool, true /* method in Assembler::callee */);

  comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();
  add(cpool, cpool, imm(ConstantPool::base_offset()));
  // These are increasing address order.  JScale seems to like a stream of
  // writes one after the other
  str(jsp,   imm_index(jsp, offset + JavaFrame::stack_bottom_pointer_offset()));
  str(locals,imm_index(jsp, offset + JavaFrame::locals_pointer_offset()));
  str(cpool, imm_index(jsp, offset + JavaFrame::cpool_offset()));
  str(callee,imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));


  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  comment("set bcp");
  add(bcp, callee, imm(Method::base_offset()));

  if (GenerateDebugAssembly && TaggedJavaStack) {
    interpreter_call_vm("check_interpreter_parameter_tags", T_VOID);
  }

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm("jprof_record_method_transition", T_VOID);
  }

  restore_stack_state_from(tos_on_stack);
  prefetch(0);
  check_timer_tick();
  dispatch(tos_interpreter_basic);
}

void InterpreterGenerator::generate_interpreter_grow_stack() {
  Segment seg(this, code_segment, "Interpreter grow stack");
  Register ret_addr = (callee == tos_val) ? tos_tag : tos_val;

bind_global("interpreter_grow_stack");
  comment("r%d   contains the method", callee);
  comment("r%d   contains the return address of the interpreter_xxx",ret_addr);
  comment("lr   contains whoever called us");
  comment("tmp1 contains the amount of stack we need") ;
  comment("locals contains the current locals pointer");

  // Push method, and leave space for bcp
  get_cpool(cpool, true /* method in Assembler::callee */);

  comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();

  add(cpool, cpool, imm(ConstantPool::base_offset()));
  // These are increasing address order.  JScale seems to like a stream of
  // writes one after the other
  str(jsp,   imm_index(jsp, offset + JavaFrame::stack_bottom_pointer_offset()));
  str(locals,imm_index(jsp, offset + JavaFrame::locals_pointer_offset()));
  str(cpool, imm_index(jsp, offset + JavaFrame::cpool_offset()));
  str(callee,imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));

  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  // Create a fake bcp with a new flag set.
  // Note that bcp is the same as lr, so we can't do this until we've
  // done a "Save last frame"

  add(bcp, callee, imm(Method::base_offset()));
  add(bcp, bcp, imm(JavaFrame::overflow_frame_flag));
  save_interpreter_state();

  if (TaggedJavaStack) {
    mov(r2, imm(int_tag));
    push(set(ret_addr, r2));
  } else {
    push(ret_addr);
  }

  comment("stack_overflow(limit)");
  mov(r1, reg(tmp1));
  interpreter_call_vm("stack_overflow", T_VOID, false);

  comment("Get real return address");
  ldr(tmp2, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

  ldr(callee, imm_index(fp, JavaFrame::method_offset()));
  ldr(locals, imm_index(fp, JavaFrame::locals_pointer_offset()));


  ldr(lr, imm_index(fp, JavaFrame::return_address_offset()));
  // We would like to do
  //    add_imm(jsp, fp, JavaFrame::end_of_locals_offset());
  //    ldr(fp, imm_index(fp, JavaFrame::caller_fp_offset()));
  // but that would leave the fp beyond the end of stack, and on some
  // systems the contents might get corrupted
  mov(jsp, reg(fp));
  GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
  ldr(fp, imm_index(jsp, JavaFrame::end_of_locals_offset(), post_indexed));
  get_method_parameter_size(tmp0, callee);
  mov(pc, reg(tmp2));
}

void InterpreterGenerator::generate_interpreter_bytecode_templates() {
  // Initialize the template table.
  TemplateTable::initialize(this);

  // Generate the bytecode templates for all the bytecodes.
  comment_section("Interpreter bytecode templates");
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    Bytecodes::Code bc = (Bytecodes::Code) i;
    if (Bytecodes::is_defined(bc)) {
      if (!TaggedJavaStack && TemplateTable::is_duplicate(bc)) {
        Bytecodes::Code duplicate = TemplateTable::get_duplicate(bc);
        comment("%s is a duplicate of %s",
                Bytecodes::name(bc), Bytecodes::name(duplicate));
      } else {
        generate_interpreter_bytecode_templates_for(bc);
      }
    }
  }
}

// "Quick" natives methods provides a quick transition from Java execution
// to C native methods. We save on the following things:
//
// [1] We don't create and Java frame.
// [2] quick native methods are never compiled (they don't need to).
// [3] no checks for thread switching
// [4] no checks for pending entry activation
// [5] no need to worry about GC.
//
// You must hand configure which native methods are executed in quick mode.
// You must make sure these methods don't cause GC or thread switching.
void
InterpreterGenerator::generate_quick_native_method_entry(BasicType return_type)
{
  Segment seg(this, code_segment);
  GUARANTEE(word_size_for(return_type) <= 1, "2-word return not supported");
  char *type;
  char name[64];

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
  jvm_sprintf(name, "quick_%s_native_method_entry", type);
  bind_rom_linkable(name, false);

  Register new_jsp;
  Register saved_lr = locals; // we don't use locals in quick native

  if (sp != jsp) {
    new_jsp = jsp;
  } else {
    new_jsp = cpool; // we don't use cpool in quick native
  }

  GUARANTEE(is_c_saved_register(new_jsp), "sanity");

  if (GenerateDebugAssembly && TaggedJavaStack) {
    comment("Quick native methods are supported only for non-tagged stack.");
    comment("Natives::rewrite_quick_native_function() should have checked!");
    breakpoint();
  }

  wtk_profile_quick_call();

  mov_imm(tmp1, 1);
  set_jvm_in_quick_native_method(tmp1);

#if ENABLE_TTY_TRACE
  if (TraceNativeCalls) {
    Label skip;

    comment("Trace native calls");
    ldr_label(tmp1, "TraceNativeCalls");
    ldr(tmp1, imm_index(tmp1));
    cmp(tmp1, zero);
    b(skip, eq);

    ldr(tmp1, imm_index(callee, Method::quick_native_code_offset()));
    ldr_label(tmp2, "_current_native_function");
    str(tmp1, imm_index(tmp2));

    mov(cpool, reg(callee));
    mov(saved_lr, reg(lr));
    ldr_label(r0, "trace_native_call");
    bl("call_on_primordial_stack");
    mov(lr, reg(saved_lr));
    mov(callee, reg(cpool));
  bind(skip);
  }
#endif

#if ENABLE_PROFILER
  // This is used to inform the Profiler that the
  // interpreter is calling into a native method.

  if (UseProfiler) {
    comment("Inform Profiler we're in native methods");

    mov_imm(tmp1, 1);
    ldr_label(tmp2, "_jvm_profiler_in_native_method");
    str(tmp1, imm_index(tmp2));
  }
#endif /* #if ENABLE_PROFILER*/

  comment("get parameter size in words");
  get_method_parameter_size(tmp0, callee);

  comment("get access flags");
  ldrh(tmp2, imm_index3(callee, Method::access_flags_offset()));

  if (GenerateDebugAssembly) {
    comment("QuickNative method must not be synchronized.");
    tst(tmp2, imm(JVM_ACC_SYNCHRONIZED));
    breakpoint(ne);
  }

  comment("get the address of quick native function");
  ldr(tmp1, imm_index(callee, Method::quick_native_code_offset()));

  comment("Calculate bottom of my parameters");
  if (JavaStackDirection > 0) {
    sub(new_jsp, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  } else {
    add(new_jsp, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  }
  comment("Test method staticness and set KNI parameters properly");
  tst(tmp2, imm(JVM_ACC_STATIC));
  eol_comment("Non-static method");
  add_imm(tmp2, new_jsp, JavaFrame::arg_offset_from_sp(-1), no_CC, eq);
  eol_comment("Static method, KNI-style, so first parameter at index 1");
  add_imm(tmp2, new_jsp, JavaFrame::arg_offset_from_sp(0), no_CC, ne);

  set_kni_parameter_base(tmp2);

  eol_comment("save return address of caller");
  mov(saved_lr, reg(lr));

  if (sp != jsp) {
    eol_comment("save my return address");
    mov(lr, reg(pc));

    eol_comment("jump to native function");
    jmpx(tmp1);
  } else {
    mov(r0, reg(tmp1));
    bl("call_on_primordial_stack");
    mov(jsp, reg(new_jsp));
  }

#if ENABLE_PROFILER
  if (UseProfiler) {

    comment("Inform Profiler we're out of native method");

    mov_imm(tmp1, 0);
    ldr_label(tmp2, "_jvm_profiler_in_native_method");
    str(tmp1, imm_index(tmp2));
  }
#endif /* #if ENABLE_PROFILER*/

  Label throwit;
  comment("Tell VM we're out of quick native methods");
  get_jvm_quick_native_exception(tmp1);
  mov_imm(tmp2, 0);
  set_jvm_in_quick_native_method(tmp2);
  cmp(tmp1, zero);
  set_return_type(return_type, eq);
  eol_comment("return back to caller if no exception");
  jmpx(saved_lr, eq);

bind(throwit);
  comment("Let the exception be thrown by System.quickNativeThrow()");
  mov(lr, reg(saved_lr));
  ldr_label_offset(r0, "persistent_handles",
                   Universe::quick_native_throw_method_index * BytesPerWord);
  ldr(r0, imm_index(r0));
  b("interpreter_method_entry");
}

void InterpreterGenerator::generate_interpreter_bytecode_templates_for(Bytecodes::Code bc) {
  Segment seg(this, code_segment);
  GUARANTEE(Bytecodes::is_defined(bc), "Cannot generate template for undefined bytecode");
  bool has_wide_variant = Bytecodes::wide_is_defined(bc);
  Template* t = TemplateTable::template_for(bc);
  GUARANTEE(t != NULL, "Cannot generate code for bytecode without a template");

  char buffer[256];
  if (has_wide_variant) {
    jvm_sprintf(buffer, "bc_impl_wide_%s", Bytecodes::name(bc));
    Template* wt = TemplateTable::template_for_wide(bc);
    GUARANTEE(wt != NULL,
              "Cannot generate code for wide bytecode without a template");
    if (wt->must_align()) {
      align(16);
    }

    {
      FunctionDefinition wide_one(this, buffer, FunctionDefinition::Local);
      if (!TaggedJavaStack) {
        for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
          Bytecodes::Code bcx = (Bytecodes::Code) i;
          if (Bytecodes::is_defined(bcx)
                 && TemplateTable::is_duplicate(bcx)
                 && TemplateTable::get_duplicate(bcx) == bc) {
             jvm_sprintf(buffer, "bc_impl_wide_%s", Bytecodes::name(bcx));
             bind_global(buffer);
          }
        }
      }
      generate_interpreter_bytecode_template(wt, true);
    }
    comment("Data containing entry for wide bytecode");
    define_long(buffer);
  }

  // Generate the template for the non-wide variant of the bytecode.
  jvm_sprintf(buffer, "bc_impl_%s", Bytecodes::name(bc));
  if (t->must_align() && !has_wide_variant) {
    align(16);
  }
  {
    FunctionDefinition def(this, buffer, FunctionDefinition::Local);

    if (!TaggedJavaStack) {
      for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
        Bytecodes::Code bcx = (Bytecodes::Code) i;
        if (Bytecodes::is_defined(bcx)
               && TemplateTable::is_duplicate(bcx)
               && TemplateTable::get_duplicate(bcx) == bc) {
           jvm_sprintf(buffer, "bc_impl_%s", Bytecodes::name(bcx));
           bind_global(buffer);
        }
      }
    }
    generate_interpreter_bytecode_template(t, false);
  }
}

void InterpreterGenerator::generate_interpreter_bytecode_template(Template* t, bool is_wide) {
  int step = is_wide ? Bytecodes::wide_length_for(t->bytecode())
                     : Bytecodes::length_for(t->bytecode());
  if (TraceBytecodes && GenerateDebugAssembly) {
    Label skip;
    ldr_label(tmp2, "TraceBytecodes");
    ldr(tmp2, imm_index(tmp2));
    cmp(tmp2, zero);
    b(skip, eq);

    // Our stack state is tos_interpreter_basic
    set_stack_state_to(tos_on_stack);
    interpreter_call_vm("trace_bytecode", T_VOID);
    restore_stack_state_from(tos_on_stack);

  bind(skip);
  }

  if (Deterministic) {
    comment("Decrement deterministic bytecode counter");
    get_bytecode_counter(tmp1);
    sub(tmp1, tmp1, one, set_CC);
    set_bytecode_counter(tmp1);

#if ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    // wcmpeqb(wR0, wR0, wR0, le);
    define_long(0xDE000060);  
#elif !ENABLE_PAGE_PROTECTION
    get_rt_timer_ticks(tmp2, le);
    add(tmp2, tmp2, one, le);
    set_rt_timer_ticks(tmp2, le);
#endif

    mov_imm(tmp1, RESCHEDULE_COUNT/8, le);
  }

  if (PrintBytecodeHistogram) {
    comment("Increment bytecode counters");
    int bc = (int)t->bytecode();
    ldr_label(tmp2, "interpreter_bytecode_counters", false);
    ldr(tmp0, imm_index(tmp2, 8 * bc, pre_indexed));
    ldr(tmp1, imm_index(tmp2, 4));
    add(tmp0, tmp0, one, set_CC);
    adc(tmp1, tmp1, zero);
    stmia(tmp2, set(tmp0, tmp1));
  }
  if (PrintPairHistogram) {
    comment("Increment pair counters");
    ldr_label_offset(tmp2, "interpreter_pair_counters",
                     Bytecodes::number_of_java_codes *8 * (int)t->bytecode(),
                     false);
    ldrb(tmp0, imm_index(bcp, Bytecodes::length_for(t->bytecode())));
    ldr(tmp0, add_index(tmp2, tmp0, lsl, 3, pre_indexed));
    ldr(tmp1, imm_index(tmp2, 4));
    add(tmp0, tmp0, one, set_CC);
    adc(tmp1, tmp1, zero);
    stmia(tmp2, set(tmp0, tmp1));
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
    get_debugger_active(tmp0);
    tst(tmp0, imm(DEBUGGER_STEPPING));
    b(not_stepping, eq);
    set_stack_state_to(tos_on_stack);
    interpreter_call_vm("handle_single_step", T_BOOLEAN);
    restore_stack_state_from(tos_on_stack);
 bind(not_stepping);
  }
#endif

  if (GenerateDebugAssembly) {
    // When debugging, a useful place to put a breakpoint to see
    // each instruction's execution.
    mov(tmp0, reg(pc));
    b("interpreter_bkpt");
  }

  // Generate the code for the template.
  // Note: This grossness of copying state back and forth is unfortunately
  //       necessary with the current design which doesn't allow a Macro-
  //       assembler or subclass of it to carry state. This is a bad design
  //       and eventually needs to be changed.
  char buffer[256];
  jvm_sprintf(buffer, "bc_impl%s_%s_internal",
          is_wide ? "_wide" : "", Bytecodes::name(t->bytecode()));
  bind_global(buffer);

  if (!TaggedJavaStack) {
    for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
      Bytecodes::Code bc = (Bytecodes::Code) i;
      if (Bytecodes::is_defined(bc)) {
        if (TemplateTable::is_duplicate(bc)
            && TemplateTable::get_duplicate(bc) == t->bytecode()) {
          jvm_sprintf(buffer, "bc_impl%s_%s_internal",
                  is_wide ? "_wide" : "", Bytecodes::name(bc));
          bind_global(buffer);
        }
      }
    }
  }

  t->_literals = _literals; // see note above
  t->generate();
  _literals = t->_literals; // see note above
}

void InterpreterGenerator::generate_interpreter_dispatch_table() {
  // On ARM, the interpreter dispatch table is generated as part of the
  // GP table. Nothing to be done here.
}

void InterpreterGenerator::generate_interpreter_bytecode_counters_table() {
  Segment seg(this, bss_segment, "Interpreter bytecode counters table");
  bind("interpreter_bytecode_counters");

  if (PrintBytecodeHistogram) {
    define_zeros(Bytecodes::number_of_java_codes * 2 * BytesPerWord);
  } else {
    comment("The bytecode counters table is intentionally left empty.");
    comment("To use bytecode counters, turn on +PrintBytecodeHistogram");
    comment("during interpreter loop generation.");
  }
}

void InterpreterGenerator::generate_interpreter_pair_counters_table() {
  Segment seg(this, bss_segment, "Interpreter pair counters table");
  bind("interpreter_pair_counters");
  if (PrintPairHistogram) {
    define_zeros(Bytecodes::number_of_java_codes *
                 Bytecodes::number_of_java_codes * 2 * BytesPerWord);
  } else {
    comment("The pair counters table is intentionally left empty.");
    comment("To use pair counters, turn on +PrintPairHistogram");
    comment("during interpreter loop generation.");
  }
}

// IMPL_NOTE: adding new fast routine don't forget to update -fno-builtin-*
//      flag in jvm.make, othrwise GCC will inline calls to mem routines,
//      although docs say that it not gonna affect C++ code, only C
void InterpreterGenerator::generate_fast_memroutines() {
  Segment seg(this, code_segment, "Fast memory routines");

  {
// Labels for memcmp
    Label
      small, small_loop, small_loop2, small_nonzero,
      align_loop, check_2byte_aligned, done, neq_2, neq_4,
      four_aligned_loop, four_aligned,
      two_aligned_loop, two_aligned;

// Labels for memcpy
    Label
      memcpy8Bit_copyForward, memcpy8Bit_alignedWords ,
      memcpy8Bit_alignedWordsLoop, memcpy8Bit_alignedWordsTrailingHalfword,
      memcpy8Bit_alignedTrailingByte, memcpy8Bit_alignedWordsDone,
      memcpy8Bit_alignedLeadingByte, memcpy8Bit_alignedLeadingHalfWord,
      memcpy8Bit_unaligned, memcpy8Bit_unalignedLoop,
      memcpy8Bit_unalignedLoopDone , memcpy8Bit_inverse,
      memcpy8Bit_copyBackward, memcpy8Bit_copyBackwardLoop ,
      memcpy8Bit_copyBackwardLoopDone;

  bind_global("jvm_memcmp");
    comment("for short areas use byte-by-byte comparision");
    cmp(r2, imm(4));
    b(small, le);
    comment("check, if strings both aligned or misaligned on the same offset");
    eor(r3, r0, reg(r1));
    tst(r3, imm(3));
    comment("if no, go to the next check");
    b(check_2byte_aligned, ne);
   bind(align_loop);
    comment("align loop");
    tst(r0, imm(3));
    b(four_aligned, eq);
    ldrb(r3,  imm_index(r0, 1, post_indexed));
    ldrb(r12, imm_index(r1, 1, post_indexed));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm(1));
    b(align_loop);
   bind(check_2byte_aligned);
    comment("now test if we can be two bytes aligned");
    tst(r3, imm(1));
    comment("go to byte-by-byte, if no");
    b(small_nonzero, ne);
    tst(r0, imm(1));
    b(two_aligned, eq);
    comment("2-byte align");
    ldrb(r3,  imm_index(r0, 1, post_indexed));
    ldrb(r12, imm_index(r1, 1, post_indexed));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm(1));
   bind(two_aligned);
    comment("avoid additional cmp in loop");
    sub(r2, r2, imm(1));
   bind(two_aligned_loop);
    ldrh(r3,  imm_index3(r0, 2, post_indexed));
    ldrh(r12, imm_index3(r1, 2, post_indexed));
    cmp(r3, reg(r12));
    b(neq_2, ne);
    sub(r2, r2, imm(2), set_CC);
    b(two_aligned_loop, gt);
    comment("compensate");
    add(r2, r2, imm(1), set_CC);
    mov(r0, imm(0), eq);
    jmpx(lr, eq);
    b(small);
   bind(four_aligned);
    comment("avoid additional cmp in loop");
    cmp(r2, imm(4));
    b(small, lt);
    sub(r2, r2, imm(3));
   bind(four_aligned_loop);
    comment("loop to do 4-byte aligned comparision");
    ldr(r3,  imm_index(r0, 4, post_indexed));
    ldr(r12, imm_index(r1, 4, post_indexed));
    cmp(r3, reg(r12));
    b(neq_4, ne);
    sub(r2, r2, imm(4), set_CC);
    b(four_aligned_loop, gt);
    comment("compensate");
    add(r2, r2, imm(3));
   bind(small);
    comment("we're here when data is heavily misaligned, or few bytes left");
    cmp(r2, imm(0));
    mov(r0, imm(0), eq);
    jmpx(lr, eq);
   bind(small_nonzero);
    tst(r2, imm(1));
    add(r2, r2, imm(1), ne);
    b(small_loop2, ne);
   bind(small_loop);
    comment("unrolled x2 loop for byte at the time processing");
    ldrb(r3,  imm_index(r0, 1, post_indexed));
    ldrb(r12, imm_index(r1, 1, post_indexed));
    cmp(r3, reg(r12));
    b(done, ne);
   bind(small_loop2);
    ldrb(r3,  imm_index(r0, 1, post_indexed));
    ldrb(r12, imm_index(r1, 1, post_indexed));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm(2), set_CC);
    b(small_loop, ne);
   bind(done);
    sub(r0, r3, reg(r12));
    jmpx(lr);
  bind(neq_4);
   comment("we found words not equal, find offender");
#if HARDWARE_LITTLE_ENDIAN
  bind(neq_2);
   andr(r0, r3,  imm(0xff));
   andr(r1, r12, imm(0xff));
   sub(r0, r0, reg(r1), set_CC);
   jmpx(lr, ne);
   mov(r3,  imm_shift(r3, lsr, 8));
   mov(r12, imm_shift(r12, lsr, 8));
   b(neq_4);
#else
   // IMPL_NOTE: we can find right r0 with bit shuffling here as far
   sub(r0, r0, imm(4));
   sub(r1, r1, imm(4));
   comment("compensate");
   add(r2, r2, imm(3));
   b(small_nonzero);
  bind(neq_2);
   comment("we found half-words not equal, find offender");
   sub(r0, r0, imm(2));
   sub(r1, r1, imm(2));
   comment("compensate");
   add(r2, r2, imm(1));
   b(small_nonzero);
#endif

  bind_global("jvm_memcpy");
    comment("Check to see if the length is 0.  Also check to see");
    comment(" which direction we need to do the copying in: ");
    cmp(r2, imm(0) );
    cmp(r0,reg(r1),ne);
    comment("Nothing to copy.  Return to caller. ");
    jmpx(lr,eq);
    b(memcpy8Bit_inverse,hi);

   bind(memcpy8Bit_copyForward);
    comment(" Do forward copying:");
        /* If the length is short, it will be faster to just do byte copying.
           This also simplifies the aligned case so that it doesn't have to
           check for short arrays. */

    cmp(r2, imm(4) );
    b(memcpy8Bit_unaligned,lt);

    comment("Check if the r0 and r1 pointers are similarly aligned: ");
    comment("Equal bits will become 0. ");
    eor(r3, r1, reg(r0));
    comment(" Check if low 2 bits are equal ");
    tst(r3, imm(0x3)  );
    b(memcpy8Bit_unaligned , ne );
    comment(" Do aligned copying:  Check if already word aligned");
    tst(r1, imm(0x3) );
    b( memcpy8Bit_alignedLeadingByte, ne );

   bind(memcpy8Bit_alignedWords);
    andr(r12, r2, imm(0x3) );
    bic(r2, r2, imm(0x3) );
    sub(r2, r2, imm(4)  , set_CC );
    b(memcpy8Bit_alignedWordsTrailingHalfword,lt);

   bind(memcpy8Bit_alignedWordsLoop);
    ldr(r3, imm_index(r1,4, post_indexed) );
    sub(r2, r2, imm(4) , set_CC );
    str(r3,  imm_index(r0,4, post_indexed)  );
    b(memcpy8Bit_alignedWordsLoop, ge );

   bind(memcpy8Bit_alignedWordsTrailingHalfword);
    comment(" Copy the trailing half word if necessary:");
    tst(r12, imm(0x2) );
    b( memcpy8Bit_alignedTrailingByte,eq);
    ldrh(r3, imm_index3(r1,2 , post_indexed ) );
    strh(r3, imm_index3(r0,2 , post_indexed  ) );

   bind(memcpy8Bit_alignedTrailingByte);
    comment(" Copy the trailing byte if necessary:");
    tst(r12, imm(0x1) );
    b(memcpy8Bit_alignedWordsDone,eq );
    ldrb(r3, imm_index(r1,0 , post_indexed) );
    strb(r3, imm_index(r0,0 , post_indexed) );

   bind(memcpy8Bit_alignedWordsDone);
    comment(" Return to caller.");
    jmpx(lr);

   bind(memcpy8Bit_alignedLeadingByte);
    comment("  Copy leading byte if necessary:");
    tst (r1, imm(0x1) );
    b(memcpy8Bit_alignedLeadingHalfWord , eq );
    ldrb(r3, imm_index(r1,1 , post_indexed) );
    sub(r2, r2, imm(1) );
    strb(r3, imm_index(r0,1 , post_indexed)   );

   bind(memcpy8Bit_alignedLeadingHalfWord);
    comment("   Copy the leading halfword if necessary:");
    tst(r1, imm(0x2) );
    b(memcpy8Bit_alignedWords ,eq);
    ldrh(r3, imm_index3(r1,2 , post_indexed ) );
    sub(r2, r2, imm(2) );
    strh(r3, imm_index3(r0,2 , post_indexed)  );
    b( memcpy8Bit_alignedWords);

   bind(memcpy8Bit_unaligned);
    sub( r2, r2, imm(1) ,set_CC);
    b( memcpy8Bit_unalignedLoopDone , lt);

   bind(memcpy8Bit_unalignedLoop);
    ldrb( r3,  imm_index(r1,1 , post_indexed) );
    sub( r2, r2, imm(1) ,set_CC );
    strb( r3, imm_index(r0,1 , post_indexed)   );
    b (  memcpy8Bit_unalignedLoop,ge);

   bind(memcpy8Bit_unalignedLoopDone);
    comment(" Return to caller.");
    jmpx(lr);


   bind(memcpy8Bit_inverse);
    comment("If (dest >= src + length), then we can do forward copying: ");
    add(r12, r1, reg(r2) );
    cmp (r0, reg(r12) );
    b(memcpy8Bit_copyForward , hs );

   bind(memcpy8Bit_copyBackward);
    comment( "Do backward copying:");
    sub(r2, r2, imm(1) , set_CC );
    add(r1, r1, reg(r2) );
    add(r0, r0, reg(r2) );
    b ( memcpy8Bit_copyBackwardLoopDone, lt );

   bind(memcpy8Bit_copyBackwardLoop);
    ldrb( r3, imm_index(r1,-1 , post_indexed) );
    sub( r2, r2, imm(1) , set_CC );
    strb( r3, imm_index(r0,-1 , post_indexed) );
    b( memcpy8Bit_copyBackwardLoop , ge );

   bind(memcpy8Bit_copyBackwardLoopDone);
    comment(" Return to caller.");
    jmpx(lr);
// end of memcpy

  }
}


#define PRINT_REGISTER(x) \
    tty->print_cr(((x<10) ? "    r%d   %s" : "    r%d  %s"), x, STR(x))

void InterpreterGenerator::print_register_definitions() {
  PRINT_REGISTER(gp);
  PRINT_REGISTER(cpool);
  PRINT_REGISTER(locals);
  PRINT_REGISTER(fp);
  PRINT_REGISTER(sp);
  PRINT_REGISTER(jsp);
  PRINT_REGISTER(lr);
  PRINT_REGISTER(pc);

  PRINT_REGISTER(method_return_type);
  PRINT_REGISTER(return_register);
  PRINT_REGISTER(stack_lock_register);

  PRINT_REGISTER(sbz);
  PRINT_REGISTER(sbo);

  PRINT_REGISTER(tos_val);
  PRINT_REGISTER(tos_tag);

  PRINT_REGISTER(tmp0);
  PRINT_REGISTER(tmp1);
  PRINT_REGISTER(tmp2);
  PRINT_REGISTER(tmp3);
  PRINT_REGISTER(tmp4);
  PRINT_REGISTER(tmp5);

  PRINT_REGISTER(bcode);
  PRINT_REGISTER(bcp);

  PRINT_REGISTER(first_register);
  PRINT_REGISTER(last_register);
}

#if GENERATE_LIBC_GLUE

void InterpreterGenerator::generate_libc_glue() {
  comment("***********************************************");
  comment("those are stubs to non-interworked system libs");
#if !ENABLE_FAST_MEM_ROUTINES
  generate_interwork_stub("jvm_memcmp");
  generate_interwork_stub("jvm_memcpy");
#endif
  generate_interwork_stub("jvm_memmove");
  generate_interwork_stub("jvm_strcpy");
  generate_interwork_stub("jvm_strncpy");
  generate_interwork_stub("jvm_strcmp");
  generate_interwork_stub("jvm_strncmp");
  generate_interwork_stub("jvm_strlen");
  generate_interwork_stub("jvm_strcat");
  generate_interwork_stub("jvm_strchr");
  generate_interwork_stub("jvm_strrchr");
  generate_interwork_stub("jvm_printf");
  generate_interwork_stub("jvm_sprintf");
  generate_interwork_stub("jvm_sscanf");
  generate_interwork_stub("jvm_vsprintf");
  generate_interwork_stub("jvm_vsnprintf");
  generate_interwork_stub("jvm_fopen");
  generate_interwork_stub("jvm_fread");
  generate_interwork_stub("jvm_fwrite");
  generate_interwork_stub("jvm_fseek");
  generate_interwork_stub("jvm_fflush");
  generate_interwork_stub("jvm_fgets");
  generate_interwork_stub("jvm_fprintf");
  generate_interwork_stub("jvm_fclose");
  generate_interwork_stub("jvm_socket");
  generate_interwork_stub("jvm_connect");
  generate_interwork_stub("jvm_shutdown");
  generate_interwork_stub("jvm_fcntl");
  generate_interwork_stub("jvm_recv");
  generate_interwork_stub("jvm_send");
  generate_interwork_stub("jvm_gethostbyname");
  generate_interwork_stub("jvm_select");
  generate_interwork_stub("jvm_htons");
  generate_interwork_stub("jvm_close");
  generate_interwork_stub("jvm_remove");
  generate_interwork_stub("jvm_exit");
  generate_interwork_stub("jvm_gettimeofday");
  generate_interwork_stub("jvm_malloc");
  generate_interwork_stub("jvm_free");
  generate_interwork_stub("jvm_qsort");
  generate_interwork_stub("jvm_setitimer");
  generate_interwork_stub("jvm_sigaction");
  generate_interwork_stub("jvm_sigaltstack");
  generate_interwork_stub("jvm_signal");
  generate_interwork_stub("jvm_usleep");
  generate_interwork_stub("jvm_nanosleep");
  generate_interwork_stub("jvm_abort");
  generate_interwork_stub("jvm_rename");
  generate_interwork_stub("jvm_stat");
  generate_interwork_stub("jvm_getenv");
  generate_interwork_stub("jvm_ftell");
  generate_interwork_stub("jvm_ferror");
  generate_interwork_stub("jvm_feof");
  generate_interwork_stub("jvm_mmap");
  generate_interwork_stub("jvm_munmap");
  generate_interwork_stub("jvm_sysconf");
  generate_interwork_stub("jvm_open");
  generate_interwork_stub("jvm_lseek");
  comment("done with system libs stubs");
  comment("***********************************************");
}

void InterpreterGenerator::generate_interwork_stub(const char* name,
                                                   bool from_thumb)
{
  if (from_thumb) {
    // actually this code could be pretty misleading:
    // we generate stub for invocation from ARM code, not THUMB, as
    // extern int foo(); will generate THUMB->ARM stub anyway, and we'll be called
    // from that stub in ARM mode, and the only thing we need to do here
    // is to call not interworking aware code in Glibc in such a way that our
    // Thumb code isn't disturbed.
    GUARANTEE(jvm_memcmp(name, "jvm_", 4) == 0,
              "Stubbed function should be jvm_<name>");

    Label me(name);
    Label peer(name+4);

    import(peer);
    global(me);
   bind(me);
    str(lr, imm_index(sp, -BytesPerWord, pre_indexed));
    bl(peer);
    ldr(lr, imm_index(sp, BytesPerWord, post_indexed));
    jmpx(lr);
  } else {
    GUARANTEE(false, "NOT IMPLEMENTED YET");
  }
}

void InterpreterGenerator::generate_fast_routines() {
  comment("those are local and hopefully optimized versions of important "
          "memory operations");
  Label arm_memset;
  Label internal_memset("memset");
  global(internal_memset);
 bind("jvm_memset");
 bind(internal_memset);
  stmfd(sp, set(lr), writeback);
  mov(lr, reg(pc));
  b(arm_memset);
  ldmfd(sp, set(lr), writeback);
  jmpx(lr);

 bind(arm_memset);
  mov(r3, reg(r0));
  andr(r1, r1, imm(0xff));
  Label l2;
 bind(l2);
  sub(r2, r2, imm(1));
  cmn(r2, imm(1));
  strb(r1, imm_index(r0, 1, post_indexed), ne);
  b(l2, ne);
  mov(r0, reg(r3));
  jmpx(lr);

  Label sig_handler("handle_vtalrm_signal");
  import(sig_handler);
 bind("handle_vtalrm_signal_stub");
  comment("hopefully this code works if being called from both ");
  comment("ARM and THUMB");
  stream()->print_cr("\t.thumb\n\tbx pc\n\tnop\n\t.arm");
  stmfd(sp, set(lr), writeback);
  mov(lr, reg(pc));
  b(sig_handler);
  ldmfd(sp, set(lr), writeback);
  jmpx(lr);
}
#endif // GENERATE_LIBC_GLUE

#endif // ENABLE_INTERPRETER_GENERATOR
