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
#include "incls/_InterpreterGenerator_thumb2.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

void InterpreterGenerator::generate() {
#if ENABLE_ARM_V7
  GUARANTEE(!ENABLE_INCREASING_JAVA_STACK, "ascending Java stack cannot be"
                                           "implemented efficiently");
  GUARANTEE(ENABLE_FULL_STACK, "full descending stack is required");
#endif

  generate_interpreter_signature();
  // generate_test_code();
  generate_quick_native_method_entry(T_VOID);  
  generate_quick_native_method_entry(T_INT);
  generate_quick_native_method_entry(T_OBJECT);

  for (int i = 0; i < 5; i++) {
    generate_interpreter_fast_method_entry(i);
  }  

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
    
  generate_interpreter_bytecode_templates();
  generate_interpreter_dispatch_table();
  generate_interpreter_grow_stack();
  generate_interpreter_bytecode_counters_table();
  generate_interpreter_pair_counters_table();
  generate_interpreter_method_entry();
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

  Label assembler_loop_type("assembler_loop_type", /* data_label = */ true);
  bind(assembler_loop_type);
  define_long(AssemblerLoopFlags::get_flags());
}

void InterpreterGenerator::generate_interpreter_method_entry() {
  Segment seg(this, code_segment, "Interpreter method entry");

  comment_section("standard interpreter method entry");
  bind_rom_linkable("interpreter_method_entry", true);
  nop();
#if ENABLE_JAVA_DEBUGGER
  comment("if tmp0==0 then this is a call into a romized method with a breakpoint");
  Label normal_call;

  get_debugger_active(tmp1);
  tst(tmp1, imm12(DEBUGGER_ACTIVE));
  b(normal_call, eq);
  comment("Get execution entry point");
  ldr(tmp0, r0, Method::variable_part_offset());
  ldr(tmp0, tmp0);
  comment("See if this method is a ROM method being debugged");
  ldr_label(tmp1, "shared_invoke_debug", false);
  cmp(tmp0, tmp1);
  comment("If eq, we were called from shared_invoke_debug.");
  b(normal_call, ne);
  push(lr);
  mov(r1, reg(r0));
  interpreter_call_vm("get_rom_debug_method", T_INT, false);
  comment("r0 returns with correct method pointer");
  pop(lr);
bind(normal_call);
#endif

// record this method so that it gets a higher chance of being compiled
  update_interpretation_log();
  get_method_parameter_size(tmp0, r0);

  verify_gp_register(tmp1);

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
  comment("get maximum stack execution size");
  ldrh(tmp1, r0, Method::max_execution_stack_count_offset());
  get_current_stack_limit(r1);
  if (JavaStackDirection < 0) {
    sub(tmp1, jsp, imm_shift(tmp1, lsl,LogBytesPerStackElement));
    sub_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(r1));    
    ldr_nearby_label(r1, stack_grown, ls);
    it(ls); ldr_using_gp(pc, "interpreter_grow_stack");
  } else {
    add(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    add_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(r1));
    it(hs);
    ldr_nearby_label(r1, stack_grown);
    it(hs); ldr_using_gp(pc, "interpreter_grow_stack");
  }

bind(stack_grown);

  comment("get number of locals");
  ldrh(tmp1, r0, Method::max_locals_offset());

  comment("pushing cleared locals");
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
    mov_imm(tmp2, 0);
    Label loop;
    sub_w(tmp1, tmp1, tmp0, lsl_shift, 0, set_CC);
    bind(loop);
    it(gt);    
    push(tmp2);
    it(gt);
    sub(tmp1, tmp1, imm12(1), set_CC);
    b(loop, gt);
#else
  if (GenerateDebugAssembly) {
    //mov_imm(tmp2, GenerateDebugAssembly ? 0xF000000F : 0);
    eor_w(tmp2, tmp2, tmp2);
    Label loop;
    sub(tmp1, tmp1, tmp0, lsl_shift, 0, set_CC);
    bind(loop);    
    it(gt);
    push(tmp2);
    it(gt);
    sub_imm12_w(tmp1, tmp1, imm12(1), set_CC);
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
  get_cpool(cpool, true /* method in r0 */);

  comment("Reserve space on stack for frame");
  //add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size()); 

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();
  add_imm(cpool, cpool, ConstantPool::base_offset());
  // These are increasing address order.  JScale seems to like a stream of
  // writes one after the other
  str(jsp,   imm_index(jsp, offset + JavaFrame::stack_bottom_pointer_offset()));
  str(locals,imm_index(jsp, offset + JavaFrame::locals_pointer_offset()));
  str(cpool, imm_index(jsp, offset + JavaFrame::cpool_offset()));
  str(r0,    imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));

  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  comment("set bcp");
  //add(bcp, r0, imm(Method::base_offset()));
  add_imm(bcp, r0, Method::base_offset());

  comment("check if the method is synchronized");
  Label synchronize, synchronization_done;

  ldrh(tmp0, r0, Method::access_flags_offset());
  tst(tmp0, imm12(JVM_ACC_SYNCHRONIZED));
  b(synchronize, ne);

bind(synchronization_done);
  /*
  if (ENABLE_PERFORMANCE_COUNTERS) {
    Label not_main;
    ldr(r0, imm_index(fp, JavaFrame::method_offset()));
    ldrh(tmp0, imm_index3(r0, Method::access_flags_offset()));
    mov(tmp1, reg(tmp0));
    and(tmp1, tmp1, imm(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    cmp(tmp1, imm(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE));
    b(not_main, ne);
    eor_imm(tmp0, tmp0, imm(JVM_ACC_PRIVATE));
    strh(tmp0, imm_index3(r0, Method::access_flags_offset()));
    interpreter_call_vm("jvm_set_main_hrtick_count", T_VOID);
bind(not_main);
  }
  */
  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm("jprof_record_method_transition", T_VOID);
  }

  check_timer_tick(tos_on_stack);
  prefetch(0);
  dispatch(tos_on_stack);

  // Synchronization code
bind_local("synchronize_interpreted_method");
bind(synchronize);
  comment("synchronize method, register tmp0(r2) holds the access flags");
  tst(tmp0, imm12(JVM_ACC_STATIC));

  comment("CC = ne: Synchronize on current class");
  comment("CC = eq: Synchronize on local 0");
  it(ne, THEN);
  ldrh_imm12_w(tmp3, r0, Method::holder_id_offset());
  get_class_list_base(tmp2);
  it(eq, ELSE);
  ldr(tos_val, locals);
  ldr_class_from_index_and_base(tmp1, tmp3, tmp2);
  save_interpreter_state();
#if ENABLE_ISOLATES
  Label  mirror_loaded;
  Label class_is_initialized, need_init;
  comment("if 'eq' then synchronizing on a local so skip all this mirror stuff");
  b(class_is_initialized, eq);
  get_mirror_list_base(tmp2);
#if ENABLE_ARM_V7
  Assembler::ldr(tmp2, tmp2, reg(tmp3));
#else
  ldr_w(tmp2, tmp2, tmp3, times_4);
#endif
  get_task_class_init_marker(tmp3);
  cmp(tmp2, reg(tmp3));
  b(class_is_initialized, ne);
bind(need_init);
  comment("Class must be being initialized by the current thread then");
  comment("Get the class to search the clinit list of current thread");
  comment("get_mirror_from_clinit_list(mirror, klass, temp reg");
  get_mirror_from_clinit_list(tmp2, tmp1, tmp3);
  ldr(tos_val, tmp2, TaskMirror::real_java_mirror_offset());
  b(mirror_loaded);
bind(class_is_initialized);
  it(ne);
  ldr(tos_val, tmp2, TaskMirror::real_java_mirror_offset());
bind(mirror_loaded);
#else
  it(ne);
  ldr(tos_val, tmp1, JavaClass::java_mirror_offset());
#endif

  comment("object to lock is in %s", reg_name(tos_val));
  call_from_interpreter("shared_lock_synchronized_method");
  restore_interpreter_state();
  b(synchronization_done);
}

// Note: fast entries are not synchronized
void
InterpreterGenerator::generate_interpreter_fast_method_entry(int extra_locals)
{
  GUARANTEE(extra_locals >= 0, "sanity check");

  Segment seg(this, code_segment);
  comment_section("Fast interpreter method entry (%d extra locals)",
                  extra_locals);
  char name[64];
  jvm_sprintf(name, "interpreter_fast_method_entry_%d", extra_locals);
  bind_rom_linkable(name, true);

  // record this method so that it gets a higher chance of being compiled
  update_interpretation_log();
  get_method_parameter_size(tmp0, r0);

  verify_gp_register(tmp1);

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
  comment("get maximum stack execution size");
  ldrh(tmp1, r0, Method::max_execution_stack_count_offset());
  get_current_stack_limit(r1);
  if (JavaStackDirection < 0) {
    sub(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    sub_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(r1));
    it(ls);
    ldr_nearby_label(r1, stack_grown);
    it(ls); ldr_using_gp(pc, "interpreter_grow_stack");
  } else {
    add(tmp1, jsp, imm_shift(tmp1, lsl, LogBytesPerStackElement));
    add_imm(tmp1, tmp1, JavaFrame::frame_desc_size());
    cmp(tmp1, reg(r1));
    it(hs);
    ldr_nearby_label(r1, stack_grown);
    it(hs); ldr_using_gp(pc, "interpreter_grow_stack");
  }
bind(stack_grown);

  if (extra_locals >= 1) {
    comment("push cleared locals");
    add_imm(jsp, jsp, BytesPerStackElement * extra_locals * JavaStackDirection);
//IMPL_NOTE: temporary workaround. uncomment these lines, when xenon build will 
//be stable
#if ENABLE_JAVA_DEBUGGER_OLD_JAVAC
    // clear locals so test suites compiled on old compilers with
    // bogus local variable tables have a snowball's chance in heck of working
//    mov(tos_tag, imm(0));
/*    for (int i = 0; i < extra_locals; i++) {
      str(tos_tag, imm_index(jsp, JavaFrame::arg_offset_from_sp(i)));
    }*/
#else
    if (GenerateDebugAssembly) {
      mov(tos_tag, try_modified_imm12(0xadadadad));
/*      for (int i = 0; i < extra_locals; i++) {
        str(tos_tag, imm_index(jsp, JavaFrame::arg_offset_from_sp(i)));
      }*/
    }
#endif
  }
  
  // Get constant pool
  get_cpool(cpool, true /* method in r0 */);

  comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();
  add_imm(cpool, cpool, ConstantPool::base_offset());
  // These are increasing address order. XScale seems to like a stream of
  // writes one after the other
  str_imm12_w(jsp,   jsp, offset + JavaFrame::stack_bottom_pointer_offset());
  str_imm12_w(locals,jsp, offset + JavaFrame::locals_pointer_offset());
  str_imm12_w(cpool, jsp, offset + JavaFrame::cpool_offset());
  str_imm12_w(r0,    jsp, offset + JavaFrame::method_offset());
  str_imm12_w(fp,    jsp, offset + JavaFrame::caller_fp_offset());
  str_imm12_w(lr,    jsp, offset + JavaFrame::return_address_offset());

  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  comment("set bcp");
  add_imm(bcp, r0, Method::base_offset());

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm("jprof_record_method_transition", T_VOID);
  }

  check_timer_tick(tos_on_stack);
  prefetch(0);
  dispatch(tos_on_stack);
}

void InterpreterGenerator::generate_interpreter_grow_stack() {
  Segment seg(this, code_segment, "Interpreter grow stack");

bind_global("interpreter_grow_stack");
  comment("r0   contains the method");
  comment("r1   contains the return address of the interpreter_xxx");
  comment("lr   contains whoever called us");
  comment("tmp1 contains the amount of stack we need") ;
  comment("locals contains the current locals pointer");


  // Push method, and leave space for bcp
  get_cpool(cpool, true /* method in r0 */);

  comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  comment("Fill in fields of the frame");
  int offset = -  JavaFrame::empty_stack_offset();

  add_imm(cpool, cpool, ConstantPool::base_offset());
  // These are increasing address order.  JScale seems to like a stream of
  // writes one after the other
  str(jsp,   imm_index(jsp, offset + JavaFrame::stack_bottom_pointer_offset()));
  str(locals,imm_index(jsp, offset + JavaFrame::locals_pointer_offset()));
  str(cpool, imm_index(jsp, offset + JavaFrame::cpool_offset()));
  str(r0,    imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));

  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  // Create a fake bcp with a new flag set.
  // Note that bcp is the same as lr, so we can't do this until we've
  // done a "Save last frame"

  add_imm(bcp, r0, Method::base_offset());
  add(bcp, bcp, JavaFrame::overflow_frame_flag);
  save_interpreter_state();

  push(r1);
  comment("stack_overflow(limit)");
  mov(r1, tmp1);
  interpreter_call_vm("stack_overflow", T_VOID, false);

  comment("Get real return address");
  ldr(tmp2, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

  ldr(r0,     imm_index(fp, JavaFrame::method_offset()));
  ldr(locals, imm_index(fp, JavaFrame::locals_pointer_offset()));


  ldr(lr, imm_index(fp, JavaFrame::return_address_offset()));
  // We would like to do
  //    add_imm(jsp, fp, JavaFrame::end_of_locals_offset());
  //    ldr(fp, imm_index(fp, JavaFrame::caller_fp_offset()));
  // but that would leave the fp beyond the end of stack, and on some
  // systems the contents might get corrupted
  mov(jsp, reg(fp));
  GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
  Macros::ldr(fp, jsp, PostIndex(JavaFrame::end_of_locals_offset()));

  get_method_parameter_size(tmp0, r0);
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
      if (TemplateTable::is_duplicate(bc)) {
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

  wtk_profile_quick_call();

  if (GenerateDebugAssembly) {
    comment("Tell VM we're in quick native methods");
    mov(tmp1, 1);
    ldr_label(tmp0, "_jvm_in_quick_native_method", false);
    str(tmp1, tmp0);
  }

#if ENABLE_TTY_TRACE
  if (GenerateDebugAssembly) {
    Label skip;

    comment("Trace native calls");
    ldr_label(tmp1, "TraceNativeCalls");
    ldr(tmp1, imm_index(tmp1, 0));
    cmp(tmp1, imm12(zero));
    b(skip, eq);

    ldr(tmp1, imm_index(r0, Method::quick_native_code_offset()));
    ldr_label(tmp0, "_current_native_function");
    str(tmp1, imm_index(tmp0, 0));

    mov(cpool, reg(r0));
    mov(saved_lr, reg(lr));
    ldr_label(r0, "trace_native_call");
    bl("call_on_primordial_stack");   
    mov(lr, reg(saved_lr));
    mov(r0, reg(cpool));
  bind(skip);
  }
#endif

#if ENABLE_PROFILER
  // This is used to inform the Profiler that the
  // interpreter is calling into a native method.
  
  if (UseProfiler) {
    comment("Inform Profiler we're in native methods");
    mov_imm(tmp1, 1);
    ldr_label(tmp0, "_jvm_profiler_in_native_method");
    str(tmp1, imm_index(tmp0, 0));
  }
#endif /* #if ENABLE_PROFILER*/

  comment("get parameter size in words");
  get_method_parameter_size(tmp0, r0);  

  comment("get access flags");
  ldrh_imm12_w(tmp2, r0, Method::access_flags_offset());

  if (GenerateDebugAssembly) {
    comment("QuickNative method must not be synchronized.");
    tst(tmp2, imm12(JVM_ACC_SYNCHRONIZED));
    breakpoint(ne);
  }

  comment("get the address of quick native function");
  ldr(tmp1, imm_index(r0, Method::quick_native_code_offset()));

  comment("Calculate bottom of my parameters");
  if (JavaStackDirection > 0) {
    sub(new_jsp, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  } else {
    add(new_jsp, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
  }
  comment("Test method staticness and set KNI parameters properly");
  tst(tmp2, imm12(JVM_ACC_STATIC));
  eol_comment("Non-static method");
  it(eq, ELSE);
  add_imm(tmp2, new_jsp, JavaFrame::arg_offset_from_sp(-1), no_CC);
  eol_comment("Static method, KNI-style, so first parameter at index 1");
  add_imm(tmp2, new_jsp, JavaFrame::arg_offset_from_sp(0), no_CC);

  set_kni_parameter_base(tmp2);

  eol_comment("save return address of caller");
  mov(saved_lr, reg(lr));

  leavex();
  if (sp != jsp) {
    leavex();
    orr_imm12_w(saved_lr, saved_lr, imm12(1));
    blx(tmp1);
    enterx();
  } else {
    mov(r0, tmp1);
    bl("call_on_primordial_stack");
    mov(jsp, new_jsp);
  }

  if (GenerateDebugAssembly) {
    comment("Tell VM we're out of quick native methods");
    mov(tmp1, 0);
    ldr_label(tmp0, "_jvm_in_quick_native_method", false);
    str(tmp1, imm_index(tmp0, 0));
  }

#if ENABLE_PROFILER
  if (UseProfiler) {
    comment("Inform Profiler we're out of native method");
    mov_imm(tmp1, 0);
    ldr_label(tmp0, "_jvm_profiler_in_native_method");
    str(tmp1, imm_index(tmp0, 0));
  }
#endif

  set_return_type(return_type);
  eol_comment("return back to caller");
  mov(pc, saved_lr);
}

void InterpreterGenerator::generate_interpreter_bytecode_templates_for(
    Bytecodes::Code bc)
{
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
      for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
        Bytecodes::Code bcx = (Bytecodes::Code) i;
        if (Bytecodes::is_defined(bcx)
               && TemplateTable::is_duplicate(bcx)
               && TemplateTable::get_duplicate(bcx) == bc) {
           jvm_sprintf(buffer, "bc_impl_wide_%s", Bytecodes::name(bcx));
           bind_global(buffer);
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

    for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
      Bytecodes::Code bcx = (Bytecodes::Code) i;
      if (Bytecodes::is_defined(bcx)
             && TemplateTable::is_duplicate(bcx)
             && TemplateTable::get_duplicate(bcx) == bc) {
         jvm_sprintf(buffer, "bc_impl_%s", Bytecodes::name(bcx));
         bind_global(buffer);
      }
    }
    generate_interpreter_bytecode_template(t, false);
  }
}

void InterpreterGenerator::generate_interpreter_bytecode_template(Template* t,
                                                                  bool is_wide)
{
  //WAS_NOT_TESTED
  int step = is_wide ? Bytecodes::wide_length_for(t->bytecode())
                     : Bytecodes::length_for(t->bytecode());
  if (TraceBytecodes && GenerateDebugAssembly) {
    Label skip;
    ldr_label(tmp0, "TraceBytecodes");
    ldr(tmp0, imm_index(tmp0, 0));
    cmp(tmp0, zero);
    b(skip, eq);

    // Our stack state is tos_interpreter_basic
    set_stack_state_to(tos_on_stack);
    interpreter_call_vm("trace_bytecode", T_VOID);
    restore_stack_state_from(tos_on_stack);

  bind(skip);
  }

  if (PrintBytecodeHistogram) {
    comment("Increment bytecode counters");
    int bc = (int)t->bytecode();
    ldr_label(tmp2, "interpreter_bytecode_counters", false);
    Macros::ldr(tmp0, tmp2, PreIndex(8 * bc));
    ldr(tmp1, tmp2, imm12(4));
    add(tmp0, tmp0, imm12(one), set_CC);
    adc_imm12_w(tmp1, tmp1, imm12(zero));
    stmia_w(tmp2, set(tmp0, tmp1));
  }
  if (PrintPairHistogram) {
    comment("Increment pair counters");
    ldr_label_offset(tmp2, "interpreter_pair_counters",
                     Bytecodes::number_of_java_codes *8 * (int)t->bytecode(),
                     false);
    ldrb(tmp0, imm_index(bcp, Bytecodes::length_for(t->bytecode())));
    add(tmp2, tmp2, tmp0, lsl_shift, 3);
    ldr(tmp0, tmp2);
    ldr(tmp1, imm_index(tmp2, 4));
    add(tmp0, tmp0, imm12(one), set_CC);
    adc_imm12_w(tmp1, tmp1, imm12(zero));
    stmia_w(tmp2, set(tmp0, tmp1));
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

#if 0 // BEGIN_CONVERT_TO_T2
  if (GenerateDebugAssembly) {
    // When debugging, a useful place to put a breakpoint to see
    // each instruction's execution.
    mov(tmp0, reg(pc));
    b("interpreter_bkpt");
  }
#endif // END_CONVERT_TO_T2

  // Generate the code for the template.
  // Note: This grossness of copying state back and forth is unfortunately
  //       necessary with the current design which doesn't allow a Macro-
  //       assembler or subclass of it to carry state. This is a bad design
  //       and eventually needs to be changed.
  char buffer[256];
  jvm_sprintf(buffer, "bc_impl%s_%s_internal",
          is_wide ? "_wide" : "", Bytecodes::name(t->bytecode()));
  bind_global(buffer);

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
  //WAS_NOT_TESTED
  Segment seg(this, code_segment, "Fast memory routines");

  {
    Label 
      small, small_loop, small_loop2, small_nonzero,
      align_loop, check_2byte_aligned, done, neq_2, neq_4,
      four_aligned_loop, four_aligned,
      two_aligned_loop, two_aligned;
    
  bind_global("memcmp");
    comment("for short areas use byte-by-byte comparision");
    cmp(r2, imm12(4));
    b(small, le);
    comment("check, if strings both aligned or misaligned on the same offset");
    eor_w(r3, r0, reg(r1));
    tst(r3, imm12(3));
    comment("if no, go to the next check");
    b(check_2byte_aligned, ne);
   bind(align_loop);
    comment("align loop");
    tst(r0, imm12(3));
    b(four_aligned, eq);
    Macros::ldrb(r3,  r0, PostIndex(1));
    Macros::ldrb(r12, r1, PostIndex(1));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm12(1));
    b(align_loop);
   bind(check_2byte_aligned);
    comment("now test if we can be two bytes aligned");
    tst(r3, imm12(1));
    comment("go to byte-by-byte, if no");
    b(small_nonzero, ne);
    tst(r0, imm12(1));
    b(two_aligned, eq);
    comment("2-byte align");
    Macros::ldrb(r3,  r0, PostIndex(1));
    Macros::ldrb(r12, r1, PostIndex(1));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm12(1));
   bind(two_aligned);
    comment("avoid additional cmp in loop");
    sub(r2, r2, imm12(1));
   bind(two_aligned_loop);
    Macros::ldrh(r3,  r0, PostIndex(2));
    Macros::ldrh(r12, r1, PostIndex(2));
    cmp(r3, reg(r12));
    b(neq_2, ne);
    sub(r2, r2, imm12(2), set_CC);
    b(two_aligned_loop, gt);
    comment("compensate");
    add(r2, r2, imm12(1), set_CC);
    it(eq, THEN);
    mov(r0, imm12(0));
    mov(pc, lr);    
    b(small);
   bind(four_aligned);
    comment("avoid additional cmp in loop");
    cmp(r2, imm12(4));
    b(small, lt);
    sub(r2, r2, imm12(3));
   bind(four_aligned_loop);
    comment("loop to do 4-byte aligned comparision");
    Macros::ldr(r3,  r0, PostIndex(4));
    Macros::ldr(r12, r1, PostIndex(4));
    cmp(r3, reg(r12));
    b(neq_4, ne);
    sub(r2, r2, imm12(4), set_CC);
    b(four_aligned_loop, gt);
    comment("compensate");
    add(r2, r2, imm12(3));
   bind(small);
    comment("we're here when data is heavily misaligned, or few bytes left");
    cmp(r2, imm12(0));
    it(eq, THEN);
    mov(r0, imm12(0));
    mov(pc, lr);//jmpx(lr, eq);
   bind(small_nonzero);
    tst(r2, imm12(1));
    it(ne);
    add(r2, r2, imm12(1));
    b(small_loop2, ne);
   bind(small_loop);
    comment("unrolled x2 loop for byte at the time processing");
    Macros::ldrb(r3,  r0, PostIndex(1));
    Macros::ldrb(r12, r1, PostIndex(1));
    cmp(r3, reg(r12));
    b(done, ne);
   bind(small_loop2);
    Macros::ldrb(r3,  r0, PostIndex(1));
    Macros::ldrb(r12, r1, PostIndex(1));
    cmp(r3, reg(r12));
    b(done, ne);
    sub(r2, r2, imm12(2), set_CC);
    b(small_loop, ne);
   bind(done);
    sub(r0, r3, reg(r12));
    mov(pc, lr);
  bind(neq_4);
   comment("we found words not equal, find offender");   
#if HARDWARE_LITTLE_ENDIAN
  bind(neq_2);
   and_imm12_w(r0, r3,  imm12(0xff));
   and_imm12_w(r1, r12, imm12(0xff));
   sub(r0, r0, reg(r1), lsl_shift, 0, set_CC);
   it(ne);
   mov(pc, lr);
   mov_w(r3,  r3, lsr_shift, 8);
   mov_w(r12, r12, lsr_shift, 8);
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

#endif // ENABLE_INTERPRETER_GENERATOR
