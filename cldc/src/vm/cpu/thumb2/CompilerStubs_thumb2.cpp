/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include "incls/_CompilerStubs_thumb2.cpp.incl"

void CompilerStubs::generate() {
#if ENABLE_COMPILER
  generate_compiler_new_object();
  generate_compiler_new_obj_array();
  generate_compiler_new_type_array();
  generate_compiler_throw_exceptions();
  generate_compiler_timer_tick();
  generate_compiler_checkcast();
  generate_compiler_instanceof();
#if ENABLE_ARM_V7
  generate_handlers();
#endif
#endif
//IMPL_NOTE: temporary workaround. uncomment these lines, when xenon build will 
//be stable - compiler_idiv_irem doesn't needed for xenon build
//#if !ENABLE_ARM_V7
  generate_compiler_idiv_irem();
//#endif
}

void CompilerStubs::generate_glue_code() {
#if ENABLE_COMPILER
  set_in_glue_code(true);
  {
    Segment seg(this, code_segment, "Compiler glue code");
    bind_global("compiler_glue_code_start");
  }
  // IMPL_NOTE: generate glue code here
  {
    Segment seg(this, code_segment, "Compiler glue code");
    bind_global("compiler_glue_code_end");
  }
  set_in_glue_code(false);
#endif
}

void CompilerStubs::generate_compiler_instanceof() {
  Segment seg(this, code_segment, "Compiler instanceof");

bind_global("compiler_instanceof");
  comment("%s   = class_id", reg_name(r0));
  comment("[%s] = object", reg_name(jsp));
  comment("check if object is instance of Universe::class_from_id(class_id)");

  eol_comment("%s = object", reg_name(r1));
  ldr(r1, imm_index(jsp, 0));

  get_class_list_base(r2);

  eol_comment("%s = near", reg_name(r1));
  ldr(r1, imm_index(r1, 0));

  eol_comment("%s = target_class", reg_name(r2));
  ldr_class_from_index_and_base(r2, /*index*/r0, /*base*/r2);

  eol_comment("%s = object_class", reg_name(r1));
  ldr(r1, imm_index(r1, 0));

  // r0 = _instanceof(thread, object_class(r1), target_class(r2));
  ldr_label(r3, "_instanceof");
  goto_shared_call_vm(T_VOID);
}

void CompilerStubs::generate_compiler_new_object() {
  Segment seg(this, code_segment, "Compiler new object");

  bind_global("compiler_new_object");
  comment("r0 contains the klass of the object");

  GUARANTEE(r0 == tos_val, "Register Sanity");
  Register result             = r0;
  Register size               = tmp0;
  Register heap_top           = tmp1;
  Register result_end         = tmp2;
  Register prototypical_near  = tmp3;
  Register klass              = tmp4;

  Label slow_case;

  // Move argument out of the way.
  mov(klass, r0);

  comment("Get size");
  ldrsh_imm12_w(size, klass, FarClass::instance_size_offset());
  comment("Get allocation start and end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);
  comment("Get prototypical near");
  ldr(prototypical_near,
      imm_index(klass, FarClass::prototypical_near_offset()));

  comment("Compute new allocation start");
  add(result_end, result, size);

  comment("Compare against _inline_allocation_end");
  cmp(result_end, heap_top);
  b(slow_case, hi);

  comment("Allocation succeeded.");
  comment("Set _inline_allocation_top and prototypical near");
  set_inline_allocation_top(result_end);
  str(prototypical_near, imm_index(result, 0));

  comment("Zero object fields");
  zero_region(result, size, heap_top, result_end, BytesPerWord, true, true);

  comment("The newly allocated object is in register r0");
  comment("Don't use BX here. We always return to thumb code.");
  comment("bit-0 of lr is not set");
  mov(pc, lr);

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  ldr_label(r3, "_newobject");
  mov(r1, klass);
  goto_shared_call_vm(T_OBJECT);
}

void CompilerStubs::generate_compiler_new_obj_array() {
  Segment seg(this, code_segment, "Compiler new obj array");
  bind_global("compiler_new_obj_array");
  comment("r0 contains the prototypical near of the object");
  comment("r1 contains the array length");

  GUARANTEE(r0 == tos_val, "Register Sanity");
  GUARANTEE(r1 == tos_tag, "Register Sanity");

  Register result             = r0;
  Register length             = r1;
  Register heap_top           = tmp0;
  Register result_end         = tmp1;
  Register prototypical_near  = tmp2;

  Label slow_case;

  comment("Get array length");
  mov(prototypical_near, reg(r0));

  comment("Get _inline_allocation_top, _inline_allocation_end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);

  comment("Check for array lengths larger than maximum safe array length or below zero");
  movw_imm12_w(tmp3, maximum_safe_array_length & 0xFFFF);
  movt_imm12_w(tmp3, maximum_safe_array_length >> 16);
  cmp(length, tmp3);
  b(slow_case, hi);

  comment("Compute new top");
  add_imm(result_end, result, Array::base_offset());
  add(result_end, result_end, length, lsl_shift, LogBytesPerWord, set_CC);
  b(slow_case, cs); // if carry is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmp(result_end, heap_top);
  b(slow_case, hi);

  comment("Allocation succeeded");
  comment("set _inline_allocation_top");
  set_inline_allocation_top(result_end);

  comment("Set the prototypical near and length");
  str(prototypical_near, tos_val, 0);
  str(length, tos_val, Array::length_offset());

  comment("Zero array elements");
  zero_region(r0, length, heap_top, result_end, Array::base_offset(),
              false, false);

  comment("The newly allocated array is in register r0");
  mov(pc, lr);  

  bind(slow_case);
  comment("Slow case - call the VM runtime system");
  comment("_anewarray(THREAD, raw_base_klass, length)");

  GUARANTEE(prototypical_near != r2, "Register bashing");

  mov(r2, reg(length));
  ldr(r1, prototypical_near);     // klass object
  ldr_label(r3, "_anewarray");
  ldr(r1, imm_index(r1, ObjArrayClass::element_class_offset()));
  goto_shared_call_vm(T_ARRAY);
}

void CompilerStubs::generate_compiler_new_type_array() {
  Segment seg(this, code_segment, "Compiler new type array");
  bind_global("compiler_new_type_array");

  GUARANTEE(r0 == tos_val, "Register Sanity");
  GUARANTEE(r1 == tos_tag, "Register Sanity");
  GUARANTEE(r2 == tmp0,    "Register Sanity");

  Label slow_case;

  Register result      = r0;
  Register length      = r1;
  Register size        = tmp0;
  Register heap_top    = tmp1;
  Register result_end  = tmp2;
  Register prototypical_near = tmp3;

  comment("Move argument out of the way");
  mov(prototypical_near, r0);

  comment("Get _inline_allocation_top, _inline_allocation_end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);

  comment("Check for array lengths larger than maximum safe array length or below zero");
  cmp(length, maximum_safe_array_length);
  b(slow_case, hi);

  comment("Compute new top and check for overflow");
  add(result_end, result, size, lsl_shift, 0, set_CC);
  b(slow_case, cs); // if carry is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmp(result_end, reg(heap_top));
  b(slow_case, hi);

  set_inline_allocation_top(result_end);
  comment("Set prototypical near and length");
  str(prototypical_near, imm_index(tos_val, 0));
  str(length, imm_index(tos_val, Array::length_offset()));

  comment("Zero typed array fields");
  zero_region(r0, size, heap_top, result_end, Array::base_offset(),
              true, true);
  mov(pc, lr);

  comment("Slow case - call the VM runtime system");
bind(slow_case);
  GUARANTEE(prototypical_near != r2 && prototypical_near != r3,
            "Register class");
  ldr(prototypical_near, imm_index(prototypical_near, 0));    // klass object
  mov(r2, reg(length));
  ldr(prototypical_near, imm_index(prototypical_near,
                                   TypeArrayClass::class_info_offset()));
  ldr_label(r3, "_newarray");
  ldr(r1, imm_index(prototypical_near, ClassInfo::type_offset()));
  goto_shared_call_vm(T_ARRAY);
}

void CompilerStubs::generate_compiler_idiv_irem() { 
#if ENABLE_ARM_V7 && 0
  Segment seg(this, code_segment, "Compiler idiv irem");
  bind_global("compiler_idiv_irem");  
#else
  Segment seg(this, code_segment, "Compiler idiv irem");
  Register Dividend     = r0;
  Register Divisor      = tmp0;
  Register Remainder    = r0;
  Register Quotient     = r1;
  Register saved_lr     = tmp4;

  int i;
  Label division_step[32], division_by_zero, division_by_zero_compiler,
        positive_results, negative_or_division_by_zero,
        div_negative_by_positive, div_negative_by_negative;

bind_global("interpreter_idiv_irem");
  simple_c_setup_arguments(T_INT, T_INT);
#if ENABLE_EMBEDDED_CALLINFO
  comment("Fix return address");
  add(lr, lr, 4);
#endif

  comment("Register r0 holds the dividend, r1 the divisor");
bind_global("compiler_idiv_irem");

  comment("Copy Divisor to another register and free r1");
  comment("This instruction also resets V flag, which is used below by BLE");
  add(Divisor, r1, zero, set_CC);

  comment("Check for special cases: Dividend < 0 or Divisor <= 0");
  // ASR #32 is encoded as ASR #0
  orr_w(tmp1, Divisor, Dividend, asr_shift, 0, set_CC);
  // Branch occurs when Z == 1 || N != V
  b(negative_or_division_by_zero, le);

bind(positive_results);
  comment("Approximate the mamximum order of the Quotient");
  clz_w(tmp1, Dividend);
  clz_w(Quotient, Divisor);
  Assembler::sub(tmp1, Quotient, reg(tmp1)); // set_CC
  mov(Quotient, zero, al, no_CC);

  comment("If Divisor is greater than Dividend, return immediately");
  it(mi);
  bx(lr);

  comment("Jump to the appropriate place in the unrolled loop below");
  tbh_w(pc, tmp1);

  comment("Offset table");
  for (i = 31; i >= 0; i--) {
    define_short(i * 7 + 32);
  }

  comment("Unrolled loop of 32 division steps");
  for (i = 31; i >= 0; i--) {
    bind(division_step[i]);
    cmp_w(Remainder, Divisor, lsl_shift, i);
    it(hs, THEN);
    sub_w(Remainder, Remainder, Divisor, lsl_shift, i);
    add(Quotient, Quotient, 1 << i, no_CC); // IMPL_NOTE: can be short also
  }
  bx(lr);

bind(negative_or_division_by_zero);
  cmp(Divisor, zero);
  b(division_by_zero, eq);
  mov(saved_lr, reg(lr));
  b(div_negative_by_positive, gt);

  neg(Divisor, Divisor);
  cmp(Dividend, zero);
  b(div_negative_by_negative, lt);
  bl(positive_results);

  comment("Divisor < 0 return point");
  neg(Quotient, Quotient);
  bx(saved_lr);

bind(div_negative_by_positive);
  neg(Dividend, Dividend);
  bl(positive_results);
  comment("Dividend < 0 return point");
  neg(Remainder, Remainder);
  neg(Quotient, Quotient);
  bx(saved_lr);

bind(div_negative_by_negative);
  neg(Dividend, Dividend);
  bl(positive_results);
  comment("Divisor < 0 && Dividend < 0 return point");
  neg(Remainder, Remainder);
  bx(saved_lr);

bind(division_by_zero);
  comment("We must check here if we were called from interpreter");
  ldr_label(tmp0, "called_from_bc_idiv", /*import=*/false);
  ldr_label(tmp1, "called_from_bc_irem", /*import=*/false);
  cmp(lr, reg(tmp0));
  it(ne);
  cmp(lr, reg(tmp1));
  b(division_by_zero_compiler, ne);

  comment("Put the arguments back on the stack");
  push_results(2, set(Dividend), set(Divisor));

  eol_comment("store bcp in frame");
  str(tmp2, imm_index(fp, JavaFrame::bcp_store_offset()));

  set_stack_state_to(tos_on_stack);
  comment("Fall through to compiler code");

bind(division_by_zero_compiler);
  comment("Division by zero from the compiler");
  ldr_label(r3, "division_by_zero_exception");
  goto_shared_call_vm(T_VOID);
#endif
} 

void CompilerStubs::generate_compiler_throw_exceptions() {
  Segment seg(this, code_segment, "Compiler exceptions");

  static char* names[] = {
    "NullPointerException",
    "ArrayIndexOutOfBoundsException",
  };

  for (int i=0; i<2; i++) {
    char buff[100];
    char *exception_name = names[i];
    jvm_sprintf(buff, "compiler_throw_%s", exception_name);

  bind_global(buff);
    comment("r0 = method->max_locals()");

    // jint offset = JavaFrame::end_of_locals_offset()
    //            -  locals * JavaStackDirection * BytesPerStackElement;

    comment("Restore caller return address");
    ldr(lr, imm_index(fp, JavaFrame::return_address_offset()));

    mov(r1, JavaFrame::end_of_locals_offset());
    if (JavaStackDirection < 0) {
      add(r0, r1, imm_shift(r0, lsl, times_4));
    } else { 
      sub(r0, r1, imm_shift(r0, lsl, times_4));
    }

    // Restore caller fp and sp
    mov(jsp, reg(fp));
    GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
    //ldr(fp, add_index(jsp, r0, lsl, 0, post_indexed));
    ldr(fp, jsp, r0);
    add(jsp, jsp, r0);

    // Fake a call from caller to System.throwXXXException().
    GUARANTEE(Universe::throw_null_pointer_exception_method_index+1 ==
              Universe::throw_array_index_exception_method_index, "sanity");
    ldr_label_offset(r0, "persistent_handles",
        (Universe::throw_null_pointer_exception_method_index+i)*BytesPerWord);
    ldr(r0, r0);
    ldr_using_gp(pc, "interpreter_method_entry");

    for (int j=0; j<MAX_INLINE_THROWER_METHOD_LOCALS; j++) {
      char buff2[100];
      jvm_sprintf(buff2, "compiler_throw_%s_%d", exception_name, j);

      comment("method->max_locals() = %d", j);
      bind_global(buff2);
      mov(r0, imm12(j));
      ldr_using_gp(pc, buff); // IMPL_NOTE: why cannot use B here?
    }
  }
}

void CompilerStubs::generate_compiler_timer_tick() {
//WAS_NOT_TESTED!!!
  Segment seg(this, code_segment, "Compiler timer tick");

  bind_global("compiler_timer_tick");

#if ENABLE_THUMB_REGISTER_MAPPING
  comment("caller is thumb mode, but does not set low bit of lr");
  add(lr, lr, imm(1));
#endif

  ldr_label(r3, "timer_tick");
  goto_shared_call_vm(T_VOID);
}

void CompilerStubs::generate_compiler_checkcast() {
  Segment seg(this, code_segment, "Compiler checkcast");
  Label loop, found;

bind_global("compiler_checkcast");
  comment("%s   = class_id", reg_name(tmp0));
  comment("[%s] = object", reg_name(jsp));
  comment("check if object is castable to Universe::class_from_id(class_id)");

  eol_comment("%s = object", reg_name(tmp1));
  ldr(tmp1, jsp, 0);
  get_class_list_base(tmp2);

  eol_comment("%s = near", reg_name(tmp1));
  ldr(tmp1, tmp1, 0);

  eol_comment("%s = target_class", reg_name(tmp0));
  ldr_class_from_index_and_base(tmp0, tmp0, tmp2);

  eol_comment("%s = class", reg_name(tmp1));
  ldr(tmp1, tmp1, 0);

  eol_comment("%s = class (copy)", reg_name(tmp2));
  mov(tmp2, tmp1);

bind(loop);
  cmp(tmp1, tmp0);
  b(found, eq);
  eol_comment("%s = class.super", reg_name(tmp1));
  ldr(tmp1, tmp1, JavaClass::super_offset());
  cmp(tmp1, 0);
  b(loop, ne);

  // Need to go into the VM to check for interfaces, or to throw
  // exception.
  ldr_label(r3, "checkcast");
  goto_shared_call_vm(T_VOID);

bind(found);
  eol_comment("%s = class.subtype_cache_1", reg_name(tmp1));
  ldr(tmp1, tmp2, JavaClass::subtype_cache_1_offset());

  eol_comment("class.subtype_cache_1 = target_class (%s)", reg_name(tmp0));
  str(tmp0, tmp2, JavaClass::subtype_cache_1_offset());

  eol_comment("class.subtype_cache_2 = old class.subtype_cache_1");
  str(tmp1, tmp2, JavaClass::subtype_cache_2_offset());

  comment("no need for write barrier if we're pointing to a lower address");
  cmp(tmp1, tmp2);
  it(lo);
  cmp(tmp0, tmp2);
  eol_comment("return if %s < %s && %s < %s",
              reg_name(tmp1), reg_name(tmp2),
              reg_name(tmp0), reg_name(tmp2));
  it(lo);
  mov(pc, lr);

  // We'd come to here only in rare cases. We must check the range since
  // the targetclass may be the romized int[] class, which is outside of heap.
  add(tmp5, tmp2, JavaClass::subtype_cache_1_offset());
  oop_write_barrier(tmp5, tmp0, tmp1, tmp3, /*rangecheck*/true);

  add(tmp5, tmp2, JavaClass::subtype_cache_2_offset());
  oop_write_barrier(tmp5, tmp0, tmp1, tmp3, /*rangecheck*/true);

  mov(pc, lr);
}

void CompilerStubs::generate_handlers() {
#if ENABLE_ARM_V7
  Segment seg(this, code_segment, "ARM V7 handlers");
  Label index_exception_handler;
  Label null_exception_handler;
  
bind_global("index_exception_handler");
bind(index_exception_handler);
  comment("r0 = get_method(THREAD, lr)");
  mov_hi(r1, lr);
  //interpreter_call_vm("get_method", T_OBJECT);
  ldr_using_gp(r3, "get_method");
  save_interpreter_state();
  interpreter_call_shared_call_vm(T_OBJECT);
  restore_interpreter_state();

  ldrh(r0, r0, Method::max_locals_offset());
  ldr_using_gp(pc, "compiler_throw_ArrayIndexOutOfBoundsException");

bind_global("null_exception_handler");
bind(null_exception_handler);
  comment("r0 = get_method(THREAD, lr)");
  mov_hi(r1, lr);
  //interpreter_call_vm("get_method", T_OBJECT);
  ldr_using_gp(r3, "get_method");
  save_interpreter_state();
  interpreter_call_shared_call_vm(T_OBJECT);
  restore_interpreter_state();

  ldrh(r0, r0, Method::max_locals_offset());
  ldr_using_gp(pc, "compiler_throw_NullPointerException");

  align(32);
  nop();
  align(16);
  nop();
  align(8);
  b(index_exception_handler);  
  align(4);
  b(null_exception_handler);
  align(4);
bind_global("handler_base", ARM_CODE);
bind_global("check_stack_overflow_handler");
  comment("r8 = max_execution_stack_count for the current method");
  comment("r2 = current_stack_limit");
  comment("r1 = jsp + JavaStackDirection * (JavaFrame::frame_desc_size() +");
  comment("     (max_execution_stack_count * BytesPerStackElement))");

  get_current_stack_limit(r2);
  if (JavaStackDirection < 0) {
    sub_w(r1, r9, r8, lsl_shift, LogBytesPerStackElement);
    sub_imm(r1, JavaFrame::frame_desc_size());
    cmp(r1, reg(r2));
  } else {
    add_w(r1, r9, r8, lsl_shift, LogBytesPerStackElement);
    add_imm(r1, JavaFrame::frame_desc_size());
    cmp(r2, reg(r1));
  }
  
  it(hi);
  bx(lr);
  ldr_using_gp(pc, "interpreter_method_entry");  

  align(32);
bind_global("frame_setup_handler");

  comment("r8 = ((method->max_locals() - method->size_of_parameters()) * ");
  comment("  * BytesPerStackElement + JavaFrame::frame_desc_size()) >> 2");
  comment("r7 = return address from the current compiled method");
 
  if (JavaStackDirection > 0) {
    add_w(jsp, jsp, r8, lsl_shift, 2);
  } else {
    sub_w(jsp, jsp, r8, lsl_shift, 2);
  }

  // The new fp will be at jsp - JavaFrame::empty_stack_offset().  We need to
  // save the old value of fp before setting the new one
  str_jsp(fp, JavaFrame::caller_fp_offset() - JavaFrame::empty_stack_offset());
  str_jsp(r7, JavaFrame::return_address_offset() 
                                   - JavaFrame::empty_stack_offset());

  const int empty_stack_offset = JavaFrame::empty_stack_offset();
  if (empty_stack_offset >= 0) {
    sub(fp, jsp, imm12(empty_stack_offset));
  } else {
    add(fp, jsp, imm12(-empty_stack_offset));
  }
  bx(lr);

  align(32);
bind_global("return_handler");

  comment("r0/r1 = return value"); 
  comment("r8    = method->max_locals()"); 
  ldr(r3, fp, JavaFrame::return_address_offset());
  mov(jsp, reg(fp));
  GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
  ldr_jsp(fp, 0);
  add(jsp, jsp, imm12(JavaFrame::end_of_locals_offset()));
  if (JavaStackDirection < 0) {
    add_w(jsp, jsp, r8, lsl_shift, LogBytesPerStackElement);
  } else {
    sub_w(jsp, jsp, r8, lsl_shift, LogBytesPerStackElement);
  }
  bx(r3);

  align(32);
bind_global("return_error_handler");

  comment("r1 = exception object "); 
  comment("r8 = method->max_locals()"); 
  ldr(r0, fp, JavaFrame::return_address_offset());
  mov_hi(lr, r0);
  mov(jsp, reg(fp));
  GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
  ldr_jsp(fp, 0);
  add(jsp, jsp, imm12(JavaFrame::end_of_locals_offset()));
  if (JavaStackDirection < 0) {
    add_w(jsp, jsp, r8, lsl_shift, LogBytesPerStackElement);
  } else {
    sub_w(jsp, jsp, r8, lsl_shift, LogBytesPerStackElement);
  }
  ldr_using_gp(pc, "shared_call_vm_exception");

  align(32);
bind_global("init_class_handler");    
#if ENABLE_ISOLATES
  // should never be called
  breakpoint();
  nop();
#else
  ldr_using_gp(r3, "initialize_class");
  ldr_using_gp(pc, "shared_call_vm");
#endif

  align(32);
bind_global("invoke_heap_handler");
  comment("r0   = method");
  ldr(pc, r0, Method::heap_execution_entry_offset());

  align(32);
bind_global("invoke_rom_handler");
  comment("r0   = method");
  ldr(r7, r0, Method::variable_part_offset());
  ldr(pc, r7);

  align(32);
bind_global("invoke_virtual_heap_handler");
  comment("r0 = class handle");
  comment("r8 = vtable_index");
  ldr(r0, r0, JavaNear::class_info_offset());
  add(r7, r8, imm12(ClassInfoDesc::header_size() >> LogBytesPerWord));
  ldr_w(r0, r0, r7, 2); // r7 automatically multiplied by 4
  ldr(pc, r0, Method::heap_execution_entry_offset());

  align(32);
bind_global("invoke_virtual_rom_handler");
  comment("r0 = class handle");
  comment("r8 = vtable_index");
  ldr(r0, r0, JavaNear::class_info_offset());
  add(r7, r8, imm12(ClassInfoDesc::header_size() >> LogBytesPerWord));
  ldr_w(r0, r0, r7, 2); // r7 automatically multiplied by 4
  ldr(r7, r0, Method::variable_part_offset());
  ldr(pc, r7);

  align(32);
bind_global("invoke_interface_handler");
  comment("tmp0    = class handle");
  comment("tos_tag = class id");
  comment("r8      = itable_index");
  ldr(tmp0, tmp0, 0);
  ldr(tmp0, tmp0, JavaClass::class_info_offset());
  ldrh(tmp1, tmp0, ClassInfo::vtable_length_offset());
  add(tmp2, tmp0, imm12(ClassInfoDesc::header_size() - 2 * BytesPerWord));
  add(tmp1, tmp2, tmp1, lsl_shift, 2);
  ldrh(tmp5, tmp0, ClassInfo::itable_length_offset());

  comment("Lookup interface method table by linear search");
  {
    Label lookup;
    bind(lookup);

    sub_imm(tmp5, one); 
    breakpoint(lt); // IMPL_NOTE: throw incompatible_class_change error

    ldr_pre_imm12_w(tos_val, tmp1, PreIndex(2 * BytesPerWord));
    cmp(tos_val, reg(tos_tag));
    b(lookup, ne);
  }

  comment("Found the itable entry - get the method table offset from there");
  ldr(tmp1, tmp1, BytesPerWord);
  add_w(tos_val, tmp0, r8, lsl_shift, LogBytesPerWord);
  ldr_w(tos_val, tos_val, tmp1, 0);

  ldr(tmp5, tos_val, Method::variable_part_offset());
  ldr(pc, tmp5);
  align(32);
bind_global("method_prolog_handler_rom");    
  generate_common_method_prolog();

  //return to appropriate place
  ldr(r3, r0, Method::variable_part_offset());
  ldr(r3, r3);
  add(r3, r3, imm12(2));//we need to skip first instruction!
  bx(r3);    
  align(32);

bind_global("method_prolog_handler_heap_ex_sens_update");
  /********execution sensor update***************/

  ldr(r3, r0, Method::heap_execution_entry_offset());  
  sub(r3, r3, imm12(CompiledMethod::base_offset() + 1/*entry has low bit set to 1!*/));
  ldr(r3, r3, CompiledMethod::flags_and_size_offset());
  eol_comment("r3 = cache index");
  mov_w(r3, r3, lsr_shift, CompiledMethodDesc::NUMBER_OF_SIZE_BITS);
 
  get_method_execution_sensor_addr(r4);
  
  strb_w(gp, r4, r3, 0);
  generate_common_method_prolog();

  //return to appropriate place
  ldr(tmp1, r0, Method::heap_execution_entry_offset());
  add(tmp1, tmp1, imm12(2));//we need to skip first instruction!
  bx(tmp1);
  align(32);

bind_global("method_prolog_handler_rom_ex_sens_update");
  /********execution sensor update***************/

  ldr(r3, r0, Method::variable_part_offset());
  ldr(r3, r3);
  sub(r3, r3, imm12(CompiledMethod::base_offset() + 1/*entry has low bit set to 1!*/));
  ldr(r3, r3, CompiledMethod::flags_and_size_offset());

  eol_comment("tmp1 = cache index");
  mov_w(r3, r3, lsr_shift, CompiledMethodDesc::NUMBER_OF_SIZE_BITS);
 
  get_method_execution_sensor_addr(r4);
  strb_w(gp, r4, r3, 0);

  generate_common_method_prolog();

  //return to appropriate place
  ldr(r3, r0, Method::variable_part_offset());
  ldr(r3, r3);
  add(r3, r3, imm12(2));//we need to skip first instruction!
  bx(r3);    

  /******** oop_write_barrier ***************/
  GUARANTEE(HARDWARE_LITTLE_ENDIAN, "Not implemented yet for big-endian");
  for (int i = 0; i <= 7; i++) {
    align(32);
    char buf[32];
    sprintf(buf, "write_barrier_handler_r%d", i);
    bind_global(buf);

    Register dst = (Register)i;
    Register base = r8;
    Register scratch = r12;

    get_bitvector_base(base);
    comment("calculate bit_offset as dst[4:2]");
    ubfx_w(scratch, dst, 2, 3);
    comment("calculate absolute byte address in bitvector");
    add_w(base, base, dst, lsr_shift, LogBytesPerWord + LogBitsPerByte);
    comment("calculate bit mask as 2^bit_offset");
    mov(dst, 1);
    lsl_w(dst, dst, scratch);
    comment("get the byte from bitvector, set the bit and write the byte back");
    ldrb(scratch, base);
    orr_w(scratch, scratch, dst);
    strb(scratch, base);

    comment("return from handler");
    bx(lr);
  }
  
  align(32);
bind_global("compiler_new_object_handler");
  comment("r0 contains the klass of the object");

  Register result             = r0;
  Register size               = tmp0;
  Register heap_top           = tmp1;
  Register result_end         = r5;
  Register prototypical_near  = tmp3;
  Register klass              = tmp4;

  Label slow_case_noh;

  // Move argument out of the way.
  mov(klass, r0);

  comment("Get size");
  ldrsh_imm12_w(size, klass, FarClass::instance_size_offset());
  comment("Get allocation start and end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);
  comment("Get prototypical near");
  ldr(prototypical_near,
      imm_index(klass, FarClass::prototypical_near_offset()));

  comment("Compute new allocation start");
  add_w(result_end, result, size);

  comment("Compare against _inline_allocation_end");
  cmp(result_end, heap_top);
  b(slow_case_noh, hi);

  comment("Allocation succeeded.");
  comment("Set _inline_allocation_top and prototypical near");
  set_inline_allocation_top(result_end);
  str(prototypical_near, imm_index(result, 0));

  comment("Zero object fields");
  zero_region(result, size, heap_top, result_end, BytesPerWord, true, true);

  comment("The newly allocated object is in register r0");
  comment("Don't use BX here. We always return to thumb code.");
  comment("bit-0 of lr is not set");
  mov(pc, lr);

  comment("Slow case - call the VM runtime system");
  bind(slow_case_noh);
  ldr_label(r3, "_newobject");
  mov(r1, klass);
  goto_shared_call_vm(T_OBJECT);
  align(32);

bind_global("compiler_new_type_array_handler");
  Label slow_case_new_type_array;

  result      = r0;
  Register length      = r1;
  size        = tmp0;
  heap_top    = tmp1;
  result_end  = tmp2;
  prototypical_near = tmp3;

  comment("Move argument out of the way");
  mov(prototypical_near, r0);

  comment("Get _inline_allocation_top, _inline_allocation_end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);

  comment("Check for array lengths larger than maximum safe array length or below zero");
  cmp(length, maximum_safe_array_length);
  b(slow_case_new_type_array, hi);

  comment("Compute new top and check for overflow");
  add(result_end, result, size, lsl_shift, 0, set_CC);
  b(slow_case_new_type_array, cs); // if carry is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmp(result_end, reg(heap_top));
  b(slow_case_new_type_array, hi);

  set_inline_allocation_top(result_end);
  comment("Set prototypical near and length");
  str(prototypical_near, imm_index(tos_val, 0));
  str(length, imm_index(tos_val, Array::length_offset()));

  comment("Zero typed array fields");
  zero_region(r0, size, heap_top, result_end, Array::base_offset(),
              true, true);
  mov(pc, lr);

  comment("Slow case - call the VM runtime system");
bind(slow_case_new_type_array);
  GUARANTEE(prototypical_near != r2 && prototypical_near != r3,
            "Register class");
  ldr(prototypical_near, imm_index(prototypical_near, 0));    // klass object
  mov(r2, reg(length));
  ldr(prototypical_near, imm_index(prototypical_near,
                                   TypeArrayClass::class_info_offset()));
  ldr_label(r3, "_newarray");
  ldr(r1, imm_index(prototypical_near, ClassInfo::type_offset()));
  goto_shared_call_vm(T_ARRAY);
  align(32);

bind_global("compiler_new_obj_array_handler");
  result             = r0;
  length             = r1;
  heap_top           = tmp0;
  result_end         = tmp1;
  prototypical_near  = tmp2;

  Label slow_case_new_obj_array;

  comment("Get array length");
  mov(prototypical_near, reg(r0));

  comment("Get _inline_allocation_top, _inline_allocation_end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);

  comment("Check for array lengths larger than maximum safe array length or below zero");
  movw_imm12_w(tmp3, maximum_safe_array_length & 0xFFFF);
  movt_imm12_w(tmp3, maximum_safe_array_length >> 16);
  cmp(length, tmp3);
  b(slow_case_new_obj_array, hi);

  comment("Compute new top");
  add_imm(result_end, result, Array::base_offset());
  add(result_end, result_end, length, lsl_shift, LogBytesPerWord, set_CC);
  b(slow_case_new_obj_array, cs); // if carry is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmp(result_end, heap_top);
  b(slow_case_new_obj_array, hi);

  comment("Allocation succeeded");
  comment("set _inline_allocation_top");
  set_inline_allocation_top(result_end);

  comment("Set the prototypical near and length");
  str(prototypical_near, tos_val, 0);
  str(length, tos_val, Array::length_offset());

  comment("Zero array elements");
  zero_region(r0, length, heap_top, result_end, Array::base_offset(),
              false, false);

  comment("The newly allocated array is in register r0");
  mov(pc, lr);  

  bind(slow_case_new_obj_array);
  comment("Slow case - call the VM runtime system");
  comment("_anewarray(THREAD, raw_base_klass, length)");

  GUARANTEE(prototypical_near != r2, "Register bashing");

  mov(r2, reg(length));
  ldr(r1, prototypical_near);     // klass object
  ldr_label(r3, "_anewarray");
  ldr(r1, imm_index(r1, ObjArrayClass::element_class_offset()));
  goto_shared_call_vm(T_ARRAY);
  align(32);
#endif
}

#if ENABLE_ARM_V7
void CompilerStubs::generate_common_method_prolog() {
   //check stack overflow  
  Register tmp1 = r3;
  ldrh(tmp1, r0, Method::max_execution_stack_count_offset());  
  
  get_current_stack_limit(r2);

  comment("r1 = jsp + JavaStackDirection * (JavaFrame::frame_desc_size() +");
  comment("     (max_execution_stack_count * BytesPerStackElement))");
  mov(r1, JavaFrame::frame_desc_size());
  add_w(r1, r1, tmp1, lsl_shift, LogBytesPerStackElement);
  if (JavaStackDirection < 0) {
    sub_w(r1, jsp, r1);
  } else {
    add_w(r1, jsp, r1);
  }  
  cmp(r1, reg(r2));
  it(ls);
  ldr_using_gp(pc, "interpreter_method_entry");  

/*****************************/
  //init handler
  comment("tmp1 shall be = ((method->max_locals() - method->size_of_parameters()) * ");
  comment("  * BytesPerStackElement + JavaFrame::frame_desc_size())");
  ldrh(tmp1, r0, Method::max_locals_offset());
  get_method_parameter_size(r4, r0);
  sub(tmp1, tmp1, r4);
  add(tmp1, tmp1, imm12(JavaFrame::frame_desc_size() >> 2));
  comment("lr = return address from the current compiled method");
 
  if (JavaStackDirection > 0) {
    add(jsp, jsp, tmp1, lsl_shift, times_4);
  } else {
    sub(jsp, jsp, tmp1, lsl_shift, times_4);
  }

  // The new fp will be at jsp - JavaFrame::empty_stack_offset().  We need to
  // save the old value of fp before setting the new one
  str_jsp(fp, JavaFrame::caller_fp_offset() - JavaFrame::empty_stack_offset());
  str_jsp(lr, JavaFrame::return_address_offset() 
                                   - JavaFrame::empty_stack_offset());

  int empty_stack_offset = JavaFrame::empty_stack_offset();
  if (empty_stack_offset >= 0) {
    sub(fp, jsp, imm12(empty_stack_offset));
  } else {
    add(fp, jsp, imm12(-empty_stack_offset));
  }   
}
#endif

#endif // ENABLE_INTERPRETER_GENERATOR
