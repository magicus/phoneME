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

#if ENABLE_INTERPRETER_GENERATOR

#ifndef PRODUCT
#include "incls/_SharedStubs_arm.cpp.incl"

void SharedStubs::generate() {
  Label shared_entry_return_point("shared_entry_return_point");
  make_local(shared_entry_return_point);
  generate_shared_entry(shared_entry_return_point);
  generate_shared_invoke_compiler();
#if ENABLE_JAVA_DEBUGGER
  generate_shared_invoke_debug();
#endif
  generate_shared_fast_accessors();

  generate_shared_call_vm(shared_entry_return_point, T_VOID);
  generate_shared_call_vm(shared_entry_return_point, T_OBJECT);
  generate_shared_call_vm(shared_entry_return_point, T_ILLEGAL);

  generate_shared_monitor_enter();
  generate_shared_monitor_exit();

  generate_invoke_pending_entries();
  generate_call_on_primordial_stack();

#if ENABLE_METHOD_TRAPS
  generate_cautious_invoke();
#endif

  generate_fast_memclear();
  generate_brute_force_icache_flush();
}

void SharedStubs::generate_shared_entry(Label& shared_entry_return_point) {
  Segment seg(this, code_segment, "Shared entry");
  bind_global("shared_entry");

  // r0:r1 = int return value (must be preserved)
  // r2    = obj return value (must be preserved)

  GUARANTEE(tos_val == r0 && tos_tag == r1 && tmp0 == r2, "Sanity");

  comment("Get information");
  get_thread(tmp1);
  get_current_pending_exception(tmp4);

  eol_comment("Thread::last_java_fp");
  ldr(tmp2, imm_index(tmp1, Thread::last_java_fp_offset()));

  eol_comment("Thread::last_java_sp");
  ldr(tmp3, imm_index(tmp1, Thread::last_java_sp_offset()));


  eol_comment("EntryFrame::FakeReturnAddress");
  mov_imm(tmp5, EntryFrame::FakeReturnAddress);

  eol_comment("make room on stack for EntryFrame");
  add_imm(jsp, jsp, JavaStackDirection * EntryFrame::frame_desc_size());

  eol_comment("set the fp of the EntryFrame");
  add_imm(fp, jsp, -EntryFrame::empty_stack_offset());

  // These are increasing order of address.  XScale seems to like that
  comment("Store information in the frame");
  eol_comment("current pending exception");
  str(tmp4, imm_index(fp, EntryFrame::pending_exception_offset()));
  eol_comment("stored obj value");
  str(r2,   imm_index(fp, EntryFrame::stored_obj_value_offset()));
  eol_comment("stored int value #1");
  str(r1,   imm_index(fp, EntryFrame::stored_int_value1_offset()));
  eol_comment("stored int value #2");
  str(r0,   imm_index(fp, EntryFrame::stored_int_value2_offset()));
  eol_comment("stored_last_sp");
  str(tmp3, imm_index(fp, EntryFrame::stored_last_sp_offset()));
  eol_comment("stored_last_fp");
  str(tmp2, imm_index(fp, EntryFrame::stored_last_fp_offset()));
  eol_comment("real_return_address");
  str(lr,   imm_index(fp, EntryFrame::real_return_address_offset()));
  eol_comment("fake_return_address");
  str(tmp5, imm_index(fp, EntryFrame::fake_return_address_offset()));

  comment("reset last java frame");
  mov(tos_val, zero);
  eol_comment("last_java_sp");
  str(tos_val, imm_index(tmp1, Thread::last_java_sp_offset()));
  eol_comment("last_java_fp");
  str(tos_val, imm_index(tmp1, Thread::last_java_fp_offset()));

  comment("get pending activation list");
  ldr(tmp2, imm_index(tmp1, Thread::pending_entries_offset()));

  comment("reset pending exception and activation");
  str(tos_val, imm_index(tmp1, Thread::pending_entries_offset()));
  set_current_pending_exception(tos_val);

  Label activations_loop;
bind(activations_loop);
  comment("Handle one activation");
  eol_comment("get next pending activation");
  ldr(tos_val, imm_index(tmp2, EntryActivation::next_offset()));

  Label push_parameter;
  eol_comment("argument length");
  ldr(tmp4, imm_index(tmp2, EntryActivation::length_offset()));
  eol_comment("pointer to first argument");
  add(tmp3, tmp2, imm(EntryActivationDesc::header_size()));
  cmp(tmp4, zero);

  eol_comment("save next pending activation");
  str(tos_val, imm_index(fp, EntryFrame::pending_activation_offset()));

  comment("push parameter from entry activation");
bind(push_parameter);
  if (TaggedJavaStack) {
    ldmia(tmp3, tos, writeback, gt);
    push(tos, gt);
  } else {
    ldr(tos_val, imm_index(tmp3, 2 * BytesPerWord, post_indexed), gt);
    push(tos_val, gt);
  }
  eol_comment("Decrement the length and check for more parameters");
  sub(tmp4, tmp4, one, set_CC);
  b(push_parameter, gt);

  eol_comment("method from entry activation");
  ldr(callee, imm_index(tmp2, EntryActivation::method_offset()));
  eol_comment("variable_part_offset");
  ldr(tmp0, imm_index(callee, Method::variable_part_offset()));
  eol_comment("Code to call");
  ldr(tmp0, imm_index(tmp0));

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  comment("call the method with variable return point");
  ldr(lr, imm_index(tmp2, EntryActivation::return_point_offset()));
  mov(pc, reg(tmp0));

bind_global("entry_return_object");
  define_call_info();
  str(r0, imm_index(fp, EntryFrame::stored_obj_value_offset()));
  b(shared_entry_return_point);

bind_global("entry_return_double");
bind_global("entry_return_long");
  define_call_info();
  str(r1, imm_index(fp, EntryFrame::stored_int_value1_offset()));
  str(r0, imm_index(fp, EntryFrame::stored_int_value2_offset()));
  b(shared_entry_return_point);

bind_global("entry_return_float");
bind_global("entry_return_word");
  define_call_info();
  str(r0, imm_index(fp, EntryFrame::stored_int_value2_offset()));
  b(shared_entry_return_point);

bind_global("entry_return_void");
  define_call_info();
#else
  call_from_interpreter(tmp0, 0);
#endif

bind(shared_entry_return_point);

  if (GenerateDebugAssembly) {
    add_imm(tmp0, fp, EntryFrame::empty_stack_offset());
    cmp(tmp0, reg(jsp));
    breakpoint(ne);
  }

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm("jprof_record_method_transition", T_VOID, false);
  }

  eol_comment("restore the next pending activation");
  ldr(tmp2, imm_index(fp, EntryFrame::pending_activation_offset()));

  eol_comment("speculatively restore pending exception");
  ldr(tmp1, imm_index(fp, EntryFrame::pending_exception_offset()));

  eol_comment("check for more pending activations");
  cmp(tmp2, zero);
  b(activations_loop, ne);

  eol_comment("restore return value(s)");
  ldr(r2,   imm_index(fp, EntryFrame::stored_obj_value_offset()));
  ldr(r1,   imm_index(fp, EntryFrame::stored_int_value1_offset()));
  ldr(r0,   imm_index(fp, EntryFrame::stored_int_value2_offset()));

  comment("restore pending exception");
  set_current_pending_exception(tmp1);

bind_local("shared_entry_return");
  comment("Remove information from the entry frame");
  get_thread(tmp1);
  eol_comment("stored_last_fp");
  ldr(tmp3, imm_index(fp, EntryFrame::stored_last_sp_offset()));
  eol_comment("stored_last_sp");
  ldr(tmp2, imm_index(fp, EntryFrame::stored_last_fp_offset()));
  eol_comment("real_return_address");
  ldr(lr,   imm_index(fp, EntryFrame::real_return_address_offset()));

  eol_comment("pop entry frame");
  add_imm(jsp, fp, EntryFrame::empty_stack_offset() -
        JavaStackDirection * (EntryFrame::frame_desc_size()));

  eol_comment("restore fp");
  mov(fp, reg(tmp2));
  eol_comment("restore Thread::last_java_sp");
  str(tmp3, imm_index(tmp1, Thread::last_java_sp_offset()));
  eol_comment("restore Thread::last_java_fp");
  str(tmp2, imm_index(tmp1, Thread::last_java_fp_offset()));
  jmpx(lr);
}

void SharedStubs::generate_shared_invoke_compiler() {
#if ENABLE_COMPILER
  Segment seg(this, code_segment, "Shared invoke compiler");
  bind_global("shared_invoke_compiler");
  comment("Fake a clock tick");
#if ENABLE_PAGE_PROTECTION
  Label page_protect("OsMisc_page_protect");
  import(page_protect);
  fast_c_call("OsMisc_page_protect", callee);
#elif ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
  // wcmpeqb(wR0, wR0, wR0);
  define_long(0xEE000060); 
#else
  get_rt_timer_ticks(tmp0);
  add_imm(tmp0, tmp0, 1);
  set_rt_timer_ticks(tmp0);
#endif
  b("interpreter_method_entry");
#endif // ENABLE_COMPILER
}

void SharedStubs::generate_shared_invoke_debug() {
  Segment seg(this, code_segment, "Shared invoke debug");
  bind_global("shared_invoke_debug");
  comment("tmp0 acts as a flag to interpreter_method_entry");
  mov(tmp0, imm(0));
  b("interpreter_method_entry");
}

void SharedStubs::generate_shared_fast_accessors() {
  Segment seg(this, code_segment, "Shared fast accessors");

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

  Label null_receiver;

  bool is_static = false;
  do {
    for (int i = 0; i < sizeof(desc)/sizeof(desc[0]); i++) {
      char tmp[128];
      sprintf(tmp, "shared_fast_get%s%s_accessor", desc[i].name,
              is_static ? "_static" : "");
      FunctionDefinition this_func(this, tmp, FunctionDefinition::ROM);
      if (!USE_FP_RESULT_IN_VFP_REGISTER && 
          ((desc[i].type == T_FLOAT) || (desc[i].type == T_DOUBLE))) {
        continue;
      }

      eol_comment("pop 'this'");
      pop(tmp2);
      eol_comment("get field offset (is 2-byte aligned!)");
      ldrh(tmp3, imm_index3(callee, Method::fast_accessor_offset_offset()));
      if (is_static) {
        cmp(tmp2, zero);
        b(null_receiver, eq);
      }

      set_return_type(stack_type_for(desc[i].type));

      eol_comment("get field from 'this' object");
      switch (desc[i].type) {
        case T_BYTE  :
          ldrsb(tos_val, add_index3(tmp2, tmp3));
          break;
        case T_CHAR  :
          ldrh (tos_val, add_index3(tmp2, tmp3));
          break;
        case T_SHORT :
          ldrsh(tos_val, add_index3(tmp2, tmp3));
          break;
#if USE_FP_RESULT_IN_VFP_REGISTER
        case T_FLOAT :
          add(tmp3, tmp2, imm_shift(tmp3, lsl, 0));
          flds(s0, imm_index5(tmp3, 0));
          break;
        case T_DOUBLE:
          add(tmp3, tmp2, imm_shift(tmp3, lsl, 0));
          fldd(d0, imm_index5(tmp3, 0));
          break;
#endif
        case T_INT   : // fall through
        case T_OBJECT:
          ldr(tos_val, add_index(tmp2, tmp3));
          break;
        case T_LONG  :
          ldr(tos_val, add_index(tmp2, tmp3, lsl, 0, pre_indexed));
          ldr(tos_tag, imm_index(tmp2, BytesPerWord));
          break;
        default      :
          SHOULD_NOT_REACH_HERE();
      }
      jmpx(lr);
    }
  } while (!is_static++);

  bind(null_receiver);
  comment("Restore stack state and continue with exception throwing");
  push(tmp2);
  // static fast accessors should not be called from compiler,
  // because they are inlined into compiled caller
  b("interpreter_throw_NullPointerException");
}

void SharedStubs::generate_shared_call_vm(Label& shared_entry_return_point,
                                          BasicType return_value_type) {
  Segment seg(this, code_segment, "Shared call VM");

  const char *iname, *sname;
  switch(return_value_type) {
    case T_OBJECT:     iname = "interpreter_call_vm_oop";
                       sname = "shared_call_vm_oop";       break;
    case T_VOID:       iname = "interpreter_call_vm";
                       sname = "shared_call_vm";           break;
    case T_ILLEGAL:    iname = "interpreter_call_vm_exception";
                       sname = "shared_call_vm_exception";  break;
    default:           iname = sname = ""; SHOULD_NOT_REACH_HERE(); break;
  }
  Label i_call_vm(iname);
  Label s_call_vm(sname);
  Label return_address;

  make_local(i_call_vm);
bind_global(i_call_vm);
#if ENABLE_EMBEDDED_CALLINFO
  add(lr, lr, imm(BytesPerWord));
#endif
bind_global(s_call_vm);
  // r1 = optional argument
  // r2 = optional argument
  // r3 = vm entry point
  // lr = pointer to call info (and possibly the return address)

  comment("Save value of java stack pointer");
  mov(tmp4, reg(jsp));
  push(lr);
  if (return_value_type == T_ILLEGAL) {
    // If the top-most frame catches the exception, we need to make sure that
    // there is sufficient space for us to expand its expression stack to
    // hold one element.
    eol_comment("Leave empty space on stack");
    add_imm(jsp, jsp, BytesPerWord * JavaStackDirection);
  }


  comment("Get the current thread");
  get_thread(tmp5);

  comment("LWT Switch stack to primordial thread");
  ldr_nearby_label(r0, return_address);
  push(r0);
  push(fp);

  comment("Save last java stack pointer and last java frame pointer in thread");
  str(jsp,  imm_index(tmp5, Thread::stack_pointer_offset()));
  str(fp,   imm_index(tmp5, Thread::last_java_fp_offset()));
  str(tmp4, imm_index(tmp5, Thread::last_java_sp_offset()));

  if (sp == jsp) {
    get_primordial_sp(sp);
  } else {
    if (GenerateDebugAssembly) {
      get_primordial_sp(r0);
      cmp(r0, reg(sp));
      breakpoint(ne);
    }
  }
  get_thread_handle(r0);    // <- mem fetch interleave
  // r1 = optional argument
  // r2 = optional argument
  // r3 = vm entry point
  // lr = ptr to call info, and possible the return address
  comment("call the native entry");
  // note: native routines preserve registers r4 - r10
  if (return_value_type != T_ILLEGAL) {
    mov(lr, reg(pc));
    jmpx(r3);
  } else {
    Label find("find_exception_frame");
    import(find);
    bl(find);
  }

  // r0 = result lo word, if any
  // r1 = result hi word, if any

  if ((sp != jsp) && GenerateDebugAssembly) {
    get_primordial_sp(r2);
    cmp(r2, reg(sp));
    breakpoint(ne);
  }
  comment("Save the return values in the thread");
  get_thread(r2);
  if (return_value_type == T_OBJECT) {
    str(r0, imm_index(r2, Thread::obj_value_offset(), pre_indexed));
    oop_write_barrier(r2, r0, r1, r3, false);
    get_thread(r2);
  } if (return_value_type == T_VOID) {
    str(r0, imm_index(r2, Thread::int1_value_offset()));
    str(r1, imm_index(r2, Thread::int2_value_offset()));
  } else {
    // Value is 1 (success) or 0 (not)
    str(r0, imm_index(r2, Thread::int1_value_offset()));
  }

  // before switching back to the Java stack, we need to see if there is enough
  // stack space available for the parameters in the pending activation.  We
  // check it here rather than in the activation code since we may need to
  // call into the VM again and we want to avoid a potential recursive call
  // into shared entry.

  Label check_stack, get_next, no_entries;
  comment("check for pending entries and if so, check for enough stack space");
  ldr(r0, imm_index(r2, Thread::pending_entries_offset()));
  cmp(r0, zero);
  b(no_entries, eq);

  ldr(r3, imm_index(r0, EntryActivation::length_offset()));
  b(get_next);

bind(check_stack);
  comment("check parameter size");
  ldr(r1, imm_index(r0, EntryActivation::length_offset()));
  eol_comment("r3 = max(r3, r1)");
  cmp(r3, reg(r1));
  mov(r3, reg(r1), lt);
bind(get_next);
  comment("get next activation");
  ldr(r0, imm_index(r0, EntryActivation::next_offset()));
  cmp(r0, zero);
  b(check_stack, ne);
  comment("convert count to bytes plus a fudge factor");
  mov(r3, imm_shift(r3, lsl, LogBytesPerStackElement + 1));
  comment("add in a stack lock for good measure");
  add(r3, r3, imm(StackLock::size() + 4));
  ldr(r0, imm_index(r2, Thread::stack_pointer_offset()));
  if (JavaStackDirection < 0) {
    sub(r3, r0, reg(r3));
  } else {
    add(r3, r0, reg(r3));
  }
  get_current_stack_limit(r0);
  cmp(r3, reg(r0));
  b(no_entries, JavaStackDirection < 0 ? gt : lt);
  comment("need more stack, call into the VM");
  get_thread_handle(r0);
  mov(r1, reg(r3));
  bl("stack_overflow");

bind(no_entries);

  comment("Call switch_thread(JVM_TRAPS)");
  get_thread_handle(r0);
  static Label L("switch_thread");
  import(L);
  bl(L);

  comment("Switch back to the java stack");
  get_thread(r2);
  ldr(jsp, imm_index(r2, Thread::stack_pointer_offset()));
  pop(fp);
  pop(lr);
  // 99% of the time, lr=label("return_address"), so we optimize for that,
  // since mov(pc, reg) is really bad on the XScale.
  cmp(lr, reg(pc));             // pc == label("return_address")?
  jmpx(lr, ne);         // On eq, we fall through, same as branching

bind(return_address);
  if (return_value_type == T_OBJECT) {
    Label oop_return("shared_call_vm_oop_return");
    bind_global(oop_return);
  }

  get_thread(r0);

  if (return_value_type == T_ILLEGAL) {
    ldr(fp, imm_index(r0, Thread::last_java_fp_offset()));
    ldr(r1, imm_index(r0, Thread::last_java_sp_offset()));
    sub_imm(jsp, r1, -JavaStackDirection * BytesPerWord);
  }

  comment("Load pending offset activations here and check below");
  ldr(r3, imm_index(r0, Thread::pending_entries_offset()));

  comment("restore the return values in the thread");
  if (return_value_type == T_OBJECT) {
    ldr(r2, imm_index(r0, Thread::obj_value_offset()));
    mov(r1, zero);
    str(r1, imm_index(r0, Thread::obj_value_offset()));
  } else {
    if (return_value_type == T_VOID) {
      ldr(r1, imm_index(r0, Thread::int2_value_offset()));
    }
    ldr(r0, imm_index(r0, Thread::int1_value_offset()));
    mov(r2, zero);
  }

  eol_comment("check for pending activations");
  cmp(r3, zero);

  eol_comment("invoke the pending activations");
  bl("shared_entry", ne);

  if (return_value_type == T_OBJECT) {
    eol_comment("move oop return value to r0");
    mov(r0, reg(r2));
  } else if (return_value_type == T_ILLEGAL) {
    cmp(r0, imm(0));
  }

  comment("reset last java frame in thread");
  get_thread(r2);
  mov(r3, zero);
  str(r3, imm_index(r2, Thread::last_java_sp_offset()));
  str(r3, imm_index(r2, Thread::last_java_fp_offset()));

  if (return_value_type == T_ILLEGAL) {
    comment("A return value of zero means the initial entry frame");
    Label done;
    b(done, ne);
    eol_comment("exception is stored in frame");
    ldr(r1,  imm_index(fp, EntryFrame::pending_exception_offset()));
    eol_comment("return address");
    ldr(lr,  imm_index(fp, EntryFrame::real_return_address_offset()));
    eol_comment("pop entry frame off stack");
    add_imm(jsp, fp, EntryFrame::empty_stack_offset() -
            JavaStackDirection * (EntryFrame::frame_desc_size()));
    set_current_pending_exception(r1);
    jmpx(lr);
    bind(done);
  }

  comment("Get pending exceptions");
  get_current_pending_exception(r3);
  pop(lr);

  // lr = previous return address - 8, ptr to embedded call info
  comment("check for pending exceptions");
  cmp(r3, zero);

  comment("return if no pending exceptions");
  jmpx(lr, eq);

  // r2 = thread
  // r3 = exception
  // lr = previous return address - 8, ptr to embedded call info
  comment("Call shared_call_vm_exception");
  eol_comment("argument goes into r1");
  mov(r1, reg(r3));

  eol_comment("clear pending exception");
  mov(r3, zero);
  set_current_pending_exception(r3);
  b("shared_call_vm_exception");
}


void SharedStubs::generate_invoke_pending_entries() {
  Segment seg(this, code_segment, "Invoke pending entries");
  bind_global("invoke_pending_entries");
  comment("Get current thread (may not have gp yet)");
  ldr_label(r0, "_current_thread", false);
  ldr(r0, imm_index(r0));

  comment("Check if there's pending entries at all");
  ldr(r1, imm_index(r0, Thread::pending_entries_offset()));
  cmp(r1, zero);
  jmpx(lr, eq);

  // Note that we need to push fp-jsp rather than fp itself.  The GC knows
  // how to relocate the stack pointer, but not the saved frame pointer
  comment("Save permanent registers");
  sub(fp, fp, reg(jsp));
  Address4 join_set = join(range(r4, r12), set(lr));
  if (jsp != sp) {
    join_set = (Address4)(join_set & (~(1 << jsp)));
  }
  push(join_set);
  comment("Invoke the pending entries");
  mov(r2, imm(uninitialized_tag));
  bl("shared_entry");

  comment("Restore permanent registers, frame pointer and link register");
  pop(join_set);
  add(fp, fp, reg(jsp));
  jmpx(lr);
}

void SharedStubs::generate_call_on_primordial_stack() {
  Segment seg(this, code_segment, "Call on primoridal stack");
  bind_global("call_on_primordial_stack");
  comment("Get target method");
  mov(r2, reg(r0));
  get_thread(r1);
  comment("Save return address and fp on java stack");
  // We must push lr and then fp, to be consistent with shared_call_vm
  push(lr);
  push(fp);
  str(jsp, imm_index(r1, Thread::stack_pointer_offset()));

  if (sp == jsp) {
    get_primordial_sp(sp);
  } else {
    if (GenerateDebugAssembly) {
      get_primordial_sp(r0);
      cmp(r0, reg(sp));
      breakpoint(ne);
    }
  }
  comment("Call r2(THREAD)");
  get_thread_handle(r0);
  mov(lr, reg(pc));
  jmpx(r2);

  if (sp != jsp) {
    if (GenerateDebugAssembly) {
      get_primordial_sp(r0);
      cmp(r0, reg(sp));
      breakpoint(ne);
    }
  }

  comment("When we return, we may be on a different thread");
  get_thread(r1);
  ldr(jsp, imm_index(r1, Thread::stack_pointer_offset()));
  ldr(lr, imm_index(jsp, -JavaStackDirection * BytesPerWord));
  ldr(fp, imm_index(jsp, -JavaStackDirection * 2 * BytesPerWord, post_indexed));
  jmpx(lr);
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
  Label trap_function("trap_function"), no_java_handler("no_java_handler");

  Segment seg(this, code_segment, "Trap entry");
  align(trap_entry_offset);
  bind_global("cautious_invoke");
  GUARANTEE(callee == r0, "Code assumption");

  comment("Generate a number of trap entries for different methods");
  for (int i = 0; i < max_traps; i++) {
    comment("Trap entry no.%d", i);
    mov(r2, imm(i));
    b(trap_function);
    align(trap_entry_offset);
  }

bind_local(trap_function);
  comment("r3 = &_method_trap[trap_no]");
  ldr_label(r3, "_method_trap");
  mov(r1, imm(sizeof(MethodTrapDesc)));
  mla(r3, r2, r1, r3);

  comment("Test if there is Java handler");
  ldr(r1, imm_index(r3, MethodTrapDesc::handler_method_offset()));
  cmp(r1, imm(0));
  b(no_java_handler, eq);

  comment("Check if we are calling directly from the same handler");
  ldr(r2, imm_index(fp, JavaFrame::method_offset()));
  cmp(r1, reg(r2));

  comment("Replace the method to be executed and proceed in interpreter");
  mov(callee, reg(r1), ne);
  b("interpreter_method_entry", ne);

  comment("Else branch to stored original method entry");
  ldr(r3, imm_index(r3, MethodTrapDesc::old_entry_offset()));
  jmpx(r3);

bind_local(no_java_handler);
  comment("Store current state");
  get_thread(r1);
  push(lr);
  push(r0);
  push(r3);
  str(jsp, imm_index(r1, Thread::stack_pointer_offset()));

  comment("Call interrupt_or_invoke on primordial stack");
  if (sp == jsp) {
    get_primordial_sp(sp);
  }
  ldr_label(r2, "interrupt_or_invoke");
  mov(r0, reg(r3));
  mov(lr, reg(pc));
  jmpx(r2);

  comment("Restore previous state");
  get_thread(r1);
  ldr(jsp, imm_index(r1, Thread::stack_pointer_offset()));
  pop(r3);
  pop(r0);
  pop(lr);

  comment("Branch to stored original method entry");
  ldr(r3, imm_index(r3, MethodTrapDesc::old_entry_offset()));
  jmpx(r3);
}
#endif

// For OSes that do not support a code flushing API, we use a brute-force
// way to flush the icache.
void SharedStubs::generate_brute_force_icache_flush() {
  Segment seg(this, code_segment, "Brute force icache flushing");

  if (GenerateBruteForceICacheFlush) {
    comment("Brute force icache flushing (%d bytes)",
            BruteForceICacheFlushSize);
    bind_global("brute_force_flush_icache");
    int num_instr = (BruteForceICacheFlushSize + 3) / 4;
    for (int i=0; i<num_instr; i++) {
      nop();
    }
    jmpx(lr);
  }

  // Alternative
  bind_global("arm_flush_icache");
    Label flush_loop;
    Label user_mode_flush("user_mode_flush");
    Label non_user_mode_flush("non_user_mode_flush");
    const int dcache_line_size = 64;
    const int dcache_line_mask = dcache_line_size - 1;

    mrs(r2, cpsr);                        // r2 = psr
    add(r1, r0, reg(r1));                 // r1 = end of code
    bic(r0, r0, imm(dcache_line_mask));   // r0 = start of cache line
    add(r1, r1, imm(dcache_line_mask));
    bic(r1, r1, imm(dcache_line_mask));   // r1 = end of code, rounded up

    comment("Are we in user mode?");
    tst(r2, imm(0xF));             // Are low four bits zero?  User or User_26
    b(non_user_mode_flush, ne);

  bind_local(user_mode_flush);
    // IMPL_NOTE: this is Linux-specific code and shouldn't 
    // work on any other OSes.
    // On Linux 2.6 and newer, r2 must be zet to zero. r2's value
    // used to be ignored on previous versions of Linux.
    mov(r2, imm(0));
    swi(0x9F0002);
    jmpx(lr);

  bind(flush_loop);
    // Some machines have cache lines of size 32, others of size 64
    // For safety, we round down to a multiple of 64, but clear every
    // 32 instructions.

    // Not implemented on XScale.  Must clean and invalidate separately
    // eol_comment("clean and invalidate data line cache");
    // mcr(p15, 0, r0, c7, c14, 1);

    eol_comment("clean data cache line");
    mcr(p15, 0, r0, c7, c10, 1);
    // This isn't necessary for XScale
    // eol_comment("invalidate data cache line");
    // mcr(p15, 0, r0, c7, c6, 1);

    eol_comment("invalidate instruction line cache");
    mcr(p15, 0, r0, c7, c5,  1);
    add(r0, r0, imm(dcache_line_size/2));

    eol_comment("clean and invalidate data line cache");
    mcr(p15, 0, r0, c7, c14, 1);
    eol_comment("invalidate instruction line cache");
    mcr(p15, 0, r0, c7, c5,  1);
    add(r0, r0, imm(dcache_line_size/2));
  bind_local(non_user_mode_flush);
    cmp(r0, reg(r1));
    b(flush_loop, lo);

    mov(r0, zero);
    eol_comment("drain write buffer");
    mcr(p15, 0, r0, c7, c10, 4);

#if ENABLE_ARM_V6
    eol_comment("flush branch target cache (ARMv6 or higher)");
    mcr(p15, 0, r0, c7, c5, 6);
#endif

    comment("CPUWAIT");
    eol_comment("wait until p15 completes operation");
    mrc(p15, 0, r0, c2, c0, 0);
    mov(r0, reg(r0));
    sub(pc, pc, imm(4));
    jmpx(lr);
}

void SharedStubs::generate_shared_monitor_enter() {
  Segment seg(this, code_segment, "Shared monitor enter");
#if ENABLE_COMPILER
  const bool generate_monitor_enter = !in_glue_code();
  const bool generate_lock_method   =  in_glue_code();
#else
  const bool generate_monitor_enter = true;
  const bool generate_lock_method   = true;
#endif

  Register object = r0;
  Register lock   = r1;

  Label allocate_monitor, have_monitor;

  if (generate_monitor_enter) {
    Label find_monitor_loop, exit_find_monitor_loop;
  bind_global("shared_monitor_enter");
    comment("Are there any monitors at all?");
    ldr(tmp0, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
    mov(lock, zero);
    add_imm(tmp1, fp, JavaFrame::empty_stack_offset());
    cmp(tmp1, reg(tmp0));
    b(allocate_monitor, eq);

    comment("Search for empty lock");
    if (JavaStackDirection < 0) {
      comment("point at object field of stack lock");
      add_imm(tmp0, tmp0, StackLock::size());
    }
    comment("object field just before first lock");
    add_imm(tmp1, fp,
            JavaFrame::pre_first_stack_lock_offset() + StackLock::size());

    // tmp0 points to the object of the stack lock
    // tmp1 points to where the "object" field of a nonexistent lock would be

  bind(find_monitor_loop);
    comment("Start the loop by checking if the current stack lock is empty");
    ldr(tmp2, imm_index(tmp0,
                        -JavaStackDirection*(BytesPerWord + StackLock::size()),
                        post_indexed));
    cmp(tmp2, zero);
    // Because of the post_index, we are pointing at the object of the
    // next stack lock, and we want to be pointing at the >>head<< of
    // the previous
    eol_comment("point at beginning of previous lock if empty");
    add_imm(lock, tmp0, (JavaStackDirection*(BytesPerWord + StackLock::size()))
                        - StackLock::size(),
            no_CC, eq);
    comment("Current stack lock object equal to the object from the stack?");
    cmp(tmp2, reg(object));
    b(exit_find_monitor_loop, eq);

    comment("Go to next stack lock in monitor block");
    cmp(tmp0, reg(tmp1));
    if (GenerateDebugAssembly) {
      breakpoint(JavaStackDirection < 0 ? hi : lo);
    }
    b(find_monitor_loop, ne);

  bind(exit_find_monitor_loop);
    comment("Have we found an entry?");
    cmp(lock, zero);
    b(have_monitor, ne);
    b(allocate_monitor, eq);
  }

  if (generate_lock_method) {
  bind_global("shared_lock_synchronized_method");
    // Allocate space for the monitor
    add_imm(jsp, jsp, JavaStackDirection * (StackLock::size() + BytesPerWord));
    str(jsp, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));

    // Set r1 to the actual monitor lock
    add_imm(r1, jsp, (JavaStackDirection < 0 ? 0 : -((int)StackLock::size())));
  }

bind(have_monitor);
  comment("r0 contains the object to lock");
  comment("r1 contains a stack lock in the monitor block");

  Label maybe_slow_case, slow_case;

  eol_comment("Get the near object");
  ldr(tmp1, imm_index(object));
  
  eol_comment("Store the object in the stack lock and lock it");
  str(object, imm_index(lock, StackLock::size()));

  if (generate_monitor_enter) {
    // in case of shared_lock_synchronized_method, we don't need to check for
    // interned strings, because java.lang.String doesn have any
    // synchronized methods.
    eol_comment("See if this is an interned string");
    get_interned_string_near_addr(tmp2);
    cmp(tmp2, reg(tmp1));
    mov(tmp2, zero, no_CC, eq);
    comment("If going the slow way, zero out real java near for GC ");
    str(tmp2, imm_index(lock, StackLock::real_java_near_offset()), eq);
    b(slow_case, eq);
  }
  
  eol_comment("object already locked?");
  ldr(tmp2, imm_index(tmp1, JavaNear::raw_value_offset()));
  tst(tmp2, one);
  b(maybe_slow_case, ne);
  
  comment("We have the fast case");
  get_thread(tmp0);
  orr(tmp2, tmp2, one);
  str(tmp2, imm_index(lock, StackLock::copied_near_offset() +
                            JavaNear::raw_value_offset()));
  str(tmp1, imm_index(lock, StackLock::real_java_near_offset()));
  str(tmp0, imm_index(lock, StackLock::thread_offset()));
  
  comment("Clear waiters");
  mov(tmp0, zero);
  str(tmp0, imm_index(lock, StackLock::waiters_offset()));
  
  comment("Update the near pointer in the object");
  add(tmp0, lock, imm(StackLock::copied_near_offset()));
  str(tmp0, imm_index(object));
  
  comment("Copy the locked near object to the stack and set lock bit");
  if (ENABLE_OOP_TAG) {
    GUARANTEE(sizeof(JavaNearDesc) == 4 * BytesPerWord, "Sanity");
    GUARANTEE(JavaNear::raw_value_offset() == 3 * BytesPerWord, "Sanity");
  } else {
    GUARANTEE(sizeof(JavaNearDesc) == 3 * BytesPerWord, "Sanity");
    GUARANTEE(JavaNear::raw_value_offset() == 2 * BytesPerWord, "Sanity");
  }
  comment("Copy the first two fields of the near");
  ldmia(tmp1, set(tmp0, tmp2));
  str(tmp0, imm_index(lock, StackLock::copied_near_offset()));
  str(tmp2, imm_index(lock, StackLock::copied_near_offset() + BytesPerWord));
  if (ENABLE_OOP_TAG) {
    ldr(tmp0, imm_index(tmp1, 2*BytesPerWord));
    str(tmp0, imm_index(lock, StackLock::copied_near_offset() +
                              2*BytesPerWord));
  }
  jmpx(lr);

bind(maybe_slow_case);
  comment("maybe_slow_case");
  // r0 contains the object to lock
  // r1 contains a stack lock in the monitor block, already set to "object"
  // tmp1 contains the near object
  get_thread(tmp0);
  eol_comment("thread of object's lock");
  ldr(tmp1, imm_index(tmp1, StackLock::thread_offset()
                              - StackLock::copied_near_offset()));
  eol_comment("set our own real java near field to nul");
  mov(tmp2, zero);
  str(tmp2, imm_index(lock, StackLock::real_java_near_offset()));

  eol_comment("is this a recursive lock?");
  cmp(tmp0, reg(tmp1));
  jmpx(lr, eq);

bind(slow_case);
  comment("slow_case");
  // Setup the argument to the VM call
  if (lock != r1) {
    mov(r1, reg(lock));
  }
  ldr_label(r3, "lock_stack_lock");
  goto_shared_call_vm(T_VOID);

bind(allocate_monitor);
  comment("Allocate a stack lock in the monitor block");
  Label enter_copy_loop, copy_stack_loop;

  comment("Set lock to old expression stack bottom");
  ldr(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));

  comment("Move expression stack top and bottom");
  add_imm(jsp, jsp, JavaStackDirection * (StackLock::size() + 4));
  add_imm(lock, lock, JavaStackDirection * (StackLock::size() + 4));
  str(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));

  comment("Move the expression stack contents");
  mov(tmp0, reg(jsp));
  b(enter_copy_loop);

bind(copy_stack_loop);
   // IMPL_NOTE: to use two word copies when !TaggedJavaStack
   ldr(tmp1, imm_index(tmp0, -JavaStackDirection * (StackLock::size() + 4)));
   if (TaggedJavaStack) {
     ldr(tmp2, imm_index(tmp0, -JavaStackDirection * (StackLock::size() + 8)));
   }
   str(tmp1, imm_index(tmp0, -JavaStackDirection * 4, post_indexed));
   if (TaggedJavaStack) {
     str(tmp2, imm_index(tmp0, -JavaStackDirection * 4, post_indexed));
   }

bind(enter_copy_loop);
   comment("Check if bottom is reached");
   cmp(tmp0, reg(lock));
   b(copy_stack_loop, ne);
   if (JavaStackDirection > 0) {
     // We point to the end of the lock rather than to the beginning
     sub(lock, lock, imm(StackLock::size()));
   }
   b(have_monitor);
}

void SharedStubs::generate_shared_monitor_exit() {
  Segment seg(this, code_segment, "Shared monitor exit");
  Label monitor_found, monitor_not_found;
  Register object = r0;
  Register lock   = r1;
#if ENABLE_COMPILER
  const bool generate_monitor_exit = !in_glue_code();
  const bool generate_unlock_method   =  in_glue_code();
#else
  const bool generate_monitor_exit  = true;
  const bool generate_unlock_method = true;
#endif

  if (generate_monitor_exit) {
  bind_global("shared_monitor_exit");
    Label not_string;

    eol_comment("Get the near object");
    ldr(tmp1, imm_index(object));

    eol_comment("See if this is an interned string");
    get_interned_string_near_addr(tmp2);
    cmp(tmp2, reg(tmp1));
    b(not_string, ne);
    mov(r1, reg(object));
    ldr_label(r3, "unlock_special_stack_lock");
    goto_shared_call_vm(T_VOID);

  bind(not_string);
    comment("Find matching slot in monitor block");
    Label find_monitor_loop;

    eol_comment("end of locks");
    ldr(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
    eol_comment("address of lock just beyond the first");
    add_imm(tmp1, fp, JavaFrame::pre_first_stack_lock_offset());
    if (JavaStackDirection < 0) {
    } else {
      eol_comment("point to first word of lock");
      add_imm(lock, lock, -((int)StackLock::size()));
    }
    // lock points to the beginning of the first lock
    // tmp1 points to just past the last lock
    cmp(lock, reg(tmp1));
    b(monitor_not_found, eq);

  bind(find_monitor_loop);
    comment("Is monitor pointed at by lock the right one?");
    eol_comment("get object field");
    ldr(tmp0, imm_index(lock, StackLock::size()));
    eol_comment("is this the right object?");
    cmp(object, reg(tmp0));
    b(monitor_found, eq);

    eol_comment("Go to next lock");
    add_imm(lock, lock, 
            -JavaStackDirection * (BytesPerWord + StackLock::size()));

    eol_comment("at the end of the monitor block?");
    cmp(lock, reg(tmp1));
    if (GenerateDebugAssembly) {
      breakpoint(JavaStackDirection < 0 ? hi : lo);
    }
    b(find_monitor_loop, ne);
  }

bind(monitor_not_found);
  comment("Throw an IllegalMonitorStateException");
  ldr_label(r3, "illegal_monitor_state_exception");
  goto_shared_call_vm(T_VOID);

  if (generate_unlock_method) {
  bind_global("shared_unlock_synchronized_method");
    comment("get object field of last lock");
    ldr(object, imm_index(fp,
                     JavaFrame::first_stack_lock_offset()+StackLock::size()));
    comment("get address of last lock");
    add_imm(lock, fp, JavaFrame::first_stack_lock_offset());
    comment("lock better be locked");
    cmp(object, zero);
    b(monitor_not_found, eq);
  }

bind(monitor_found);
  comment("r0 contains the object that is being unlocked");
  comment("r1 contains the stack lock");
  comment("");
  comment("Get the real java near pointer from the stack lock");
  ldr(tmp1, imm_index(lock, StackLock::real_java_near_offset()));
  comment("Zero out the object field so that lock is free");
  mov(tmp0, zero);
  str(tmp0, imm_index(lock, StackLock::size()));

  comment("Is this the reentrant case?");
  cmp(tmp1, zero);
  jmpx(lr, eq);

  comment("Set the object near pointer");
  // tmp1 contains the real java near
  str(tmp1, imm_index(object));

  // The last argument can be false if we separate out the case of
  // unlocking synchronized methods into a separate case. There are
  // no synchronized methods in String.
  //
  // Oop write barrier is needed in case new near object has been assigned
  // during locking.  But this happens so rarely that it is worth our while
  // to do a quick test to avoid it.
  Label skip_barrier;
  cmp(tmp1, reg(object));
  ldr(tmp0, imm_index(lock, StackLock::copied_near_offset()
                                   + JavaNear::raw_value_offset()), lo);
  b(skip_barrier, lo);
  oop_write_barrier(object, tmp0, tmp1, tmp2, true);
  ldr(tmp0, imm_index(lock, StackLock::copied_near_offset()
                                   + JavaNear::raw_value_offset()));
bind(skip_barrier);
  tst(tmp0, imm(2));
  jmpx(lr, eq);

  comment("Call the runtime system to signal the waiters");
  if (lock != r1) {
    mov(r1, reg(lock));
  }
  ldr_label(r3, "signal_waiters");
  goto_shared_call_vm(T_VOID);
}

void SharedStubs::generate_fast_memclear() {
  // This function is intended to set the region of Java Heap to 0 after GC,
  // so that the newly created Java objects will have zero-valued fields.
  // It can be safely called from C code when declared like
  //   void fast_memclear(void* start, int length);
  Segment seg(this, code_segment, "Clears a large range of memory");
  bind_global("fast_memclear");
  
  const Register ptr      = r0;
  const Register count    = r1;
  const Register zr       = r2;
  const Address4 regset32 = join(range(r2, r7), set(r12, lr));
  const Address4 regset16 = range(r4, r7);
  const Address4 regset8  = set(r2, r3);
  Label chunk1024, chunk512, chunk256, chunk128, chunk64;

  if (GenerateDebugAssembly) {
    comment("length must be non-negative and word-aligned");
    tst(count, imm(0x80000003));
    breakpoint(ne);
  }

#define ZERO_BLOCK(size)                    \
  {                                         \
    for (int i = 0; i < (size); i += 32) {  \
      stmia(ptr, regset32, writeback);      \
    }                                       \
  }

  stmfd(sp, join(range(r4, r7), set(lr)), writeback);
  sub(count, count, imm(1024), set_CC);
  mov(r2,  imm(0));
  mov(r3,  imm(0));
  mov(r4,  imm(0));
  mov(r5,  imm(0));
  mov(r6,  imm(0));
  mov(r7,  imm(0));
  mov(r12, imm(0));
  mov(lr,  imm(0));
  b(chunk512, lo);

  bind(chunk1024);
  sub(count, count, imm(1024), set_CC);
  ZERO_BLOCK(1024);
  b(chunk1024, hs);

  bind(chunk512);
  tst(count, imm(512));
  b(chunk256, eq);
  ZERO_BLOCK(512);

  bind(chunk256);
  tst(count, imm(256));
  b(chunk128, eq);
  ZERO_BLOCK(256);

  bind(chunk128);
  tst(count, imm(128));
  b(chunk64, eq);
  ZERO_BLOCK(128);

  bind(chunk64);
  tst(count, imm(64));
  stmia(ptr, regset32, writeback, ne);
  stmia(ptr, regset32, writeback, ne);
  tst(count, imm(32));
  stmia(ptr, regset32, writeback, ne);
  tst(count, imm(16));
  stmia(ptr, regset16, writeback, ne);
  mov(count, imm_shift(count, lsl, 29), set_CC);
  stmia(ptr, regset8, writeback, cs);
  str(zr, imm_index(ptr), ne);

#if ENABLE_THUMB_VM
  ldmfd(sp, join(range(r4, r7), set(lr)), writeback);
  jmpx(lr);
#else
  ldmfd(sp, join(range(r4, r7), set(pc)), writeback);
#endif
}

#endif // PRODUCT

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
