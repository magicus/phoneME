/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include "incls/_CompilerStubs_arm.cpp.incl"

void CompilerStubs::generate() {
#if ENABLE_COMPILER
  generate_compiler_new_object();
  generate_compiler_new_obj_array();
  generate_compiler_new_type_array();
  generate_compiler_throw_exceptions();
  generate_compiler_timer_tick();
  generate_compiler_checkcast();
  generate_compiler_instanceof();
#if USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  generate_indirect_execution_sensor_update();  
#endif
#else
 Label newobject("_newobject");
 import(newobject);
#endif
  generate_compiler_idiv_irem();
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

#if ENABLE_THUMB_REGISTER_MAPPING
  comment("caller is thumb mode, but does not set low bit of lr");
  add(lr, lr, imm(1));
#endif

  // Move argument out of the way.
  mov(klass, reg(r0));

  comment("Get size");
  ldrsh(size, imm_index3(klass, FarClass::instance_size_offset()));
  comment("Get allocation start and end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);
  comment("Get prototypical near");
  ldr(prototypical_near,
      imm_index(klass, FarClass::prototypical_near_offset()));

  comment("Compute new allocation start");
  add(result_end, result, reg(size));

  comment("Compare against _inline_allocation_end");
  cmp(result_end, reg(heap_top));
  b(slow_case, hi);
#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
  pld(imm_index(result_end));
  pld(imm_index(result_end, 32));
  pld(imm_index(result_end, 64));
#endif  
  comment("Allocation succeeded.");
  comment("Set _inline_allocation_top and prototypical near");
  set_inline_allocation_top(result_end);
  str(prototypical_near, imm_index(result));

  comment("Zero object fields");
  zero_region(result, size, heap_top, result_end, BytesPerWord, true, true);

  comment("The newly allocated object is in register r0");
  jmpx(lr);

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  ldr_label(r3, "_newobject");
  mov(r1, reg(klass));
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
  cmp(length, imm(maximum_safe_array_length));
  b(slow_case, hi);

  comment("Compute new top");
  add(result_end, result, imm(Array::base_offset()));
  add(result_end, result_end, imm_shift(length, lsl, times_4), set_CC);
  b(slow_case, cs); // if carry is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmp(result_end, reg(heap_top));
  b(slow_case, hi);

#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
  pld(imm_index(result_end));
  pld(imm_index(result_end, 32));
  pld(imm_index(result_end, 64));
#endif  

  comment("Allocation succeeded");
  comment("set _inline_allocation_top");
  set_inline_allocation_top(result_end);

  comment("Set the prototypical near and length");
  str(prototypical_near, imm_index(tos_val));
  str(length, imm_index(tos_val, Array::length_offset()));

  comment("Zero array elements");
  zero_region(r0, length, heap_top, result_end, Array::base_offset(),
              false, false);

  comment("The newly allocated array is in register r0");
  jmpx(lr);

  bind(slow_case);
  comment("Slow case - call the VM runtime system");
  comment("_anewarray(THREAD, raw_base_klass, length)");

  GUARANTEE(prototypical_near != r2, "Register bashing");

  mov(r2, reg(length));
  ldr(r1, imm_index(prototypical_near));     // klass object
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
  mov(prototypical_near, reg(r0));

  comment("Get _inline_allocation_top, _inline_allocation_end");
  get_inline_allocation_top(result);
  get_inline_allocation_end(heap_top);

  comment("Check for array lengths larger than maximum safe array length or below zero");
  cmp(length, imm(maximum_safe_array_length));
  b(slow_case, hi);

  comment("Compute new top and check for overflow");
  add(result_end, result, reg(size), set_CC);
  b(slow_case, cs); // if carry is set the addition has overflowed

#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
  pld(imm_index(result_end));
  pld(imm_index(result_end, 32));
  pld(imm_index(result_end, 64));
#endif  

  comment("Compare against _inline_allocation_end");
  cmp(result_end, reg(heap_top));
  b(slow_case, hi);

  set_inline_allocation_top(result_end);
  comment("Set prototypical near and length");
  str(prototypical_near, imm_index(tos_val));
  str(length, imm_index(tos_val, Array::length_offset()));

  comment("Zero typed array fields");
  zero_region(r0, size, heap_top, result_end, Array::base_offset(),
              true, true);
  jmpx(lr);

  comment("Slow case - call the VM runtime system");
bind(slow_case);
  GUARANTEE(prototypical_near != r2 && prototypical_near != r3,
            "Register class");
  ldr(prototypical_near, imm_index(prototypical_near));     // klass object
  mov(r2, reg(length));
  ldr(prototypical_near, imm_index(prototypical_near,
                                   TypeArrayClass::class_info_offset()));
  ldr_label(r3, "_newarray");
  ldr(r1, imm_index(prototypical_near, ClassInfo::type_offset()));
  goto_shared_call_vm(T_ARRAY);
}

void CompilerStubs::generate_compiler_idiv_irem() {
  Segment seg(this, code_segment, "Compiler idiv irem");
  Register Dividend     = r0;
  Register Divisor      = r12;
  Register Remainder    = r0;
  Register Quotient     = r1;

  int i;
  Label division_step[32], division_by_zero, division_by_zero_compiler,
        positive_results, negative_or_division_by_zero;

bind_global("interpreter_idiv_irem");
  simple_c_setup_arguments(T_INT, T_INT);
#if ENABLE_EMBEDDED_CALLINFO
  comment("Fix return address");
  add(lr, lr, imm(4));
#endif

  comment("Register r0 holds the dividend, r1 the divisor");
bind_global("compiler_idiv_irem");

  comment("Copy Divisor to another register and free r1");
  comment("This instruction also resets V flag, which is used below by BLE");
  add(Divisor, r1, zero, set_CC);

  comment("Check for special cases: Dividend < 0 or Divisor <= 0");
  // ASR #32 is encoded as ASR #0
  orr(tmp1, Divisor, imm_shift(Dividend, asr, 0), set_CC);
  // Branch occurs when Z == 1 || N != V
  b(negative_or_division_by_zero, le);

bind(positive_results);

#if ENABLE_ARM_V5TE
  // Here is a faster version of the algorithm which uses
  // ARMv5-specific instructions such as CLZ
  comment("Approximate the mamximum order of the Quotient");
  clz(tmp1, Dividend);
  clz(Quotient, Divisor);
  sub(tmp1, Quotient, reg(tmp1), set_CC);
  mov(Quotient, zero);

  comment("Jump to the appropriate place in the unrolled loop below");
  // It is more difficult to calculate the offset than just to load it
  // from the table. If you feel that memory access should be avoided
  // anyway, you can replace ldr with the following instruction sequence:
  //   rsb(tmp1, tmp1, imm(31));
  //   add(tmp1, tmp1, imm_shift(tmp1, lsl, times_2));
  //   add(pc, pc, imm_shift(tmp1, lsl, times_4));
  ldr(pc, add_index(pc, tmp1, lsl, times_4), pl);

  comment("If Divisor is greater than Dividend, return immediately");
  jmpx(lr);

  comment("Offset table");
  for (i = 0; i <= 31; i++) {
    define_long(division_step[i]);
  }
#else
  // Generic implementation which is suitable for ARMv4
  mov(Quotient, zero);
  comment("Count quotient bits");
  for (i = 0; i <= 30; i++) {
    cmp(Dividend, imm_shift(Divisor, lsl, i));
    b(division_step[i], ls);
  }
#endif // ENABLE_ARM_V5TE

  comment("Unrolled loop of 32 division steps");
  for (i = 31; i >= 0; i--) {
    bind(division_step[i]);
    cmp(Remainder, imm_shift(Divisor, lsl, i));
    sub(Remainder, Remainder, imm_shift(Divisor, lsl, i), hs);
    add(Quotient, Quotient, imm(1 << i), hs);
  }
  jmpx(lr);

bind(negative_or_division_by_zero);
  comment("set bit 0 of temp register if Divisor < 0");
  cmp(Divisor, zero);
  b(division_by_zero, eq);
  mov(tmp1, imm_shift(Divisor, lsr, 31));
  rsb(Divisor, Divisor, zero, lt);

  comment("set bit 1 of temp register if Dividend < 0");
  cmp(Dividend, zero);
  orr(tmp1, tmp1, imm(2), lt);
  rsb(Dividend, Dividend, zero, lt);

  comment("Save the original return address and adjust lr to point to");
  comment("the appropriate sign-correction routine");
  mov(tmp0, reg(lr));
  add(lr, pc, imm_shift(tmp1, lsl, 4));
  b(positive_results);

  comment("Should not reach here - for alignment purpose only");
  breakpoint();
  breakpoint();
  breakpoint();
  breakpoint();

  comment("Divisor < 0 return point");
  rsb(Quotient, Quotient, zero);      // this piece must be 16-byte aligned
  jmpx(tmp0);
  nop();
  nop();
  
  comment("Dividend < 0 return point");
  rsb(Remainder, Remainder, zero);    // this piece must be 16-byte aligned
  rsb(Quotient, Quotient, zero);
  jmpx(tmp0);
  nop();
  
  comment("Dividend < 0 && Divisor < 0 return point");
  rsb(Remainder, Remainder, zero);    // this piece must be 16-byte aligned
  jmpx(tmp0);
  nop();
  nop();

bind(division_by_zero);
  comment("We must check here if we were called from interpreter");
  ldr_label(tmp0, "called_from_bc_idiv", /*import=*/false);
  ldr_label(tmp1, "called_from_bc_irem", /*import=*/false);
  cmp(lr, reg(tmp0));
  cmp(lr, reg(tmp1), ne);
  b(division_by_zero_compiler, ne);

  comment("Put the arguments back on the stack");
  if (TaggedJavaStack) {
    set_tag(int_tag, tmp0);
    push_results(2, set(Dividend, tmp0), set(Divisor, tmp0));
  } else {
    push_results(2, set(Dividend), set(Divisor));
  }
  eol_comment("store bcp in frame");
  str(tmp2, imm_index(fp, JavaFrame::bcp_store_offset()));

  set_stack_state_to(tos_on_stack);
  comment("Fall through to compiler code");

bind(division_by_zero_compiler);
  comment("Division by zero from the compiler");
  ldr_label(r3, "division_by_zero_exception");
  goto_shared_call_vm(T_VOID);
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

    mov_imm(r1, JavaFrame::end_of_locals_offset());
    if (JavaStackDirection < 0) {
      add(r0, r1, imm_shift(r0, lsl, times_4));
    } else { 
      sub(r0, r1, imm_shift(r0, lsl, times_4));
    }

    // Restore caller fp and sp
    mov(jsp, reg(fp));
    GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
    ldr(fp, add_index(jsp, r0, lsl, 0, post_indexed));

    // Fake a call from caller to System.throwXXXException().
    GUARANTEE(Universe::throw_null_pointer_exception_method_index+1 ==
              Universe::throw_array_index_exception_method_index, "sanity");
    ldr_label_offset(r0, "persistent_handles",
        (Universe::throw_null_pointer_exception_method_index+i)*BytesPerWord);
    ldr(r0, imm_index(r0));
    b("interpreter_method_entry");

    for (int j=0; j<MAX_INLINE_THROWER_METHOD_LOCALS; j++) {
      char buff2[100];
      jvm_sprintf(buff2, "compiler_throw_%s_%d", exception_name, j);

      comment("method->max_locals() = %d", j);
      bind_global(buff2);
      mov(r0, imm(j));
      b(buff);
    }

    // Generate exception handler for methods with omitted stack frames.
    jvm_sprintf(buff, "compiler_throw_%s_10", exception_name);

  bind_global(buff);
    comment("r0 = parameter offset");

    comment("Restore caller return address");
    add(jsp, jsp, reg(r0));

    // Fake a call from caller to System.throwXXXException().
    GUARANTEE(Universe::throw_null_pointer_exception_method_index+1 ==
              Universe::throw_array_index_exception_method_index, "sanity");
    ldr_label_offset(r0, "persistent_handles",
        (Universe::throw_null_pointer_exception_method_index+i)*BytesPerWord);
    ldr(r0, imm_index(r0));
    b("interpreter_method_entry");

  }
}

void CompilerStubs::generate_compiler_timer_tick() {
  Segment seg(this, code_segment, "Compiler timer tick");

  bind_global("compiler_timer_tick");

#if ENABLE_THUMB_REGISTER_MAPPING
  comment("caller is thumb mode, but does not set low bit of lr");
  add(lr, lr, imm(1));
#endif

  ldr_label(r3, "timer_tick");
  goto_shared_call_vm(T_VOID);
}

void CompilerStubs::generate_compiler_instanceof() {
  Segment seg(this, code_segment, "Compiler instanceof");

bind_global("compiler_instanceof");
  comment("%s   = class_id", reg_name(r0));
  comment("[%s] = object", reg_name(jsp));
  comment("check if object is instance of Universe::class_from_id(class_id)");

  Address2 addr = imm_index(jsp, 0);

  eol_comment("%s = object", reg_name(r1));
  ldr(r1, addr);

  get_class_list_base(r2);

  eol_comment("%s = near", reg_name(r1));
  ldr(r1, imm_index(r1));

  eol_comment("%s = target_class", reg_name(r2));
  ldr_class_from_index_and_base(r2, /*index*/r0, /*base*/r2);

  eol_comment("%s = object_class", reg_name(r1));
  ldr(r1, imm_index(r1));

  // r0 = _instanceof(thread, object_class(r1), target_class(r2));
  ldr_label(r3, "_instanceof");
  goto_shared_call_vm(T_VOID);
}

void CompilerStubs::generate_compiler_checkcast() {
  Segment seg(this, code_segment, "Compiler checkcast");
  Label loop, found;

bind_global("compiler_checkcast");
  comment("%s   = class_id", reg_name(tmp0));
  comment("[%s] = object", reg_name(jsp));
  comment("check if object is castable to Universe::class_from_id(class_id)");

  Address2 addr = imm_index(jsp, 0);

  eol_comment("%s = object", reg_name(tmp1));
  ldr(tmp1, addr);

  get_class_list_base(tmp2);

  eol_comment("%s = near", reg_name(tmp1));
  ldr(tmp1, imm_index(tmp1));

  eol_comment("%s = target_class", reg_name(tmp0));
  ldr_class_from_index_and_base(tmp0, tmp0, tmp2);

  eol_comment("%s = class", reg_name(tmp1));
  ldr(tmp1, imm_index(tmp1));

  eol_comment("%s = class (copy)", reg_name(tmp2));
  mov(tmp2, reg(tmp1));

bind(loop);
  cmp(tmp1, reg(tmp0));
  b(found, eq);
  eol_comment("%s = class.super", reg_name(tmp1));
  ldr(tmp1, imm_index(tmp1, JavaClass::super_offset()));
  cmp(tmp1, imm(0));
  b(loop, ne);

  // Need to go into the VM to check for interfaces, or to throw
  // exception.
  ldr_label(r3, "checkcast");
  goto_shared_call_vm(T_VOID);

bind(found);
  eol_comment("%s = class.subtype_cache_1", reg_name(tmp1));
  ldr(tmp1, imm_index(tmp2, JavaClass::subtype_cache_1_offset()));

  eol_comment("class.subtype_cache_1 = target_class (%s)", reg_name(tmp0));
  str(tmp0, imm_index(tmp2, JavaClass::subtype_cache_1_offset()));

  eol_comment("class.subtype_cache_2 = old class.subtype_cache_1");
  str(tmp1, imm_index(tmp2, JavaClass::subtype_cache_2_offset()));

  comment("no need for write barrier if we're pointing to a lower address");
  cmp(tmp1, reg(tmp2));
  cmp(tmp0, reg(tmp2), lo);
  eol_comment("return if %s < %s && %s < %s", 
              reg_name(tmp1), reg_name(tmp2),
              reg_name(tmp0), reg_name(tmp2));
  jmpx(lr, lo);

  // We'd come to here only in rare cases. We must check the range since
  // the targetclass may be the romized int[] class, which is outside of heap.
  add(tmp5, tmp2, imm(JavaClass::subtype_cache_1_offset()));
  oop_write_barrier(tmp5, tmp0, tmp1, tmp3, /*rangecheck*/true);

  add(tmp5, tmp2, imm(JavaClass::subtype_cache_2_offset()));
  oop_write_barrier(tmp5, tmp0, tmp1, tmp3, /*rangecheck*/true);

  jmpx(lr);
}

void CompilerStubs::generate_indirect_execution_sensor_update() {
  Segment seg(this, code_segment, "Execution sensor update");

  comment("r1 = return address to actual start of CompiledMethod");
  comment("r0 = Method (must not be changed by this function)");

bind_global("indirect_execution_sensor_update");

#if !ENABLE_THUMB_REGISTER_MAPPING
  UNIMPLEMENTED();
#endif

  eol_comment("r2 = CompiledMethod");
  sub(r2, r1, imm(CompiledMethod::base_offset() + 6));
  ldr(r3, imm_index(r2, CompiledMethod::flags_and_size_offset()));

  eol_comment("r3 = cache index");
  mov(r3, imm_shift(r3, lsr, CompiledMethodDesc::NUMBER_OF_SIZE_BITS));
 
  get_method_execution_sensor_addr(r7);
  add(r3, r3, reg(r7));
  strb(gp, imm_index(r3));

  eol_comment("r2 = Method");
  ldr(r2, imm_index(r2, CompiledMethod::method_offset()));
  add(r1, r1, imm(1));
  ldr(r2, imm_index(r2, Method::variable_part_offset()));

  eol_comment("Method::execution_entry() now points to real entry");
  str(r1, imm_index(r2));
  jmpx(r1);
}

#endif // ENABLE_INTERPRETER_GENERATOR
