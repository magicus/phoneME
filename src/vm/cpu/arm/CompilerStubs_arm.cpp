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
#include "incls/_CompilerStubs_arm.cpp.incl"

void CompilerStubs::generate() {

#if ENABLE_COMPILER
  generate_compiler_new_obj_array();
  generate_compiler_new_type_array();
  generate_compiler_throw_exceptions();
  generate_compiler_timer_tick();
  generate_compiler_checkcast();
  generate_compiler_instanceof();
  generate_compiler_callvm_stubs();

#if USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  generate_indirect_execution_sensor_update();
#endif // USE_INDIRECT_EXECUTION_SENSOR_UPDATE

#else  // !ENABLE_COMPILER
 Label newobject("_newobject");
 import(newobject);
#endif // ENABLE_COMPILER

  generate_compiler_idiv_irem();
}

void CompilerStubs::generate_glue_code() {
#if ENABLE_COMPILER
  set_in_glue_code(true);
  {
    Segment seg(this, code_segment, "Compiler glue code");
    bind_global("compiler_glue_code_start");
  }
  generate_compiler_new_object();

#if ENABLE_ARM_VFP
  generate_vfp_redo();
  generate_vfp_double_redo();
#endif  // ENABLE_ARM_VFP

  SharedStubs shared_stubs(stream());
  shared_stubs.generate_shared_monitor_enter();
  shared_stubs.generate_shared_monitor_exit();

  {
    Segment seg(this, code_segment, "Compiler glue code");
    bind_global("compiler_glue_code_end");
  }
  set_in_glue_code(false);
#endif
}

void CompilerStubs::generate_compiler_callvm_stubs() {
#if ENABLE_COMPRESSED_VSF
  // These routines are called from compiled method
  // when it needs to call VM without inlined frame-flushing code.
  // It should be a slow (unusual) case only, becase
  // the routines here are tough enough.
  Segment seg(this, code_segment, "Compiler call_vm stubs");
  bind_global("compiler_callvm_stubs_start");

#if ENABLE_PAGE_PROTECTION
  {
    bind_global("compiler_timer_tick");

    // The frame is already flushed in timer signal handler,
    // skip flushing and go directly to VM code
    ldr_label(r3, "timer_tick");
    bl("shared_call_vm");

    // Restore register mapping and return to compiled code
    sub(sp, sp, imm(16 * BytesPerWord));
    stmia(sp, range(r0, r12));
    mov(r0, reg(sp));
    bl("vsf_restore");
    ldmia(sp, range(r0, r12), writeback);
    add(sp, sp, imm(BytesPerWord));
    ldmia(sp, range(lr, pc), writeback);
  }
#endif // ENABLE_PAGE_PROTECTION

#if NOT_CURRENTLY_USED && ENABLE_INLINE_COMPILER_STUBS
  // This implementation is currently disabled because
  // inlined NewObjectStub and NewTypeArrayStub allow to achieve
  // better performance. However, compressed VSF tables should be
  // considered when it is necessary to reduce compiled code size.
  {
    bind_global("compiler_new_object");
    Register tmp = tmp2;
    GUARANTEE(is_c_saved_register(tmp), "Code assumption");

    // Actually we need to preserve only GP and JSP registers,
    // but let's make it simplier by storing just the whole range
    sub(sp, sp, imm(16 * BytesPerWord));
    stmia(sp, range(r0, r12));
    mov(r0, reg(sp));
    // Get the last STR instruction from inlined compiled_new_object code
    ldr(tmp, imm_index(lr, -BytesPerWord, pre_indexed));
    // Pretend as if PC holds the return address to caller compiled method
    str(lr, imm_index(sp, (int)pc * BytesPerWord));
    bl("vsf_flush");
    // JSP can be adjusted in frame-flushing procedure - needs to be reloaded
    ldr(jsp, imm_index(sp, (int)jsp * BytesPerWord));

    // Decode Rd field of STR instruction -
    // it's the same register where JavaNear sits
    andr(tmp, tmp, imm(0xf000));
    ldr(r1, add_index(sp, tmp, lsr, 10));
    add(sp, sp, imm(16 * BytesPerWord));
    ldr_label(r3, "_newobject");
    // Get the class from JavaNear and instantiate it using general mechanism
    ldr(r1, imm_index(r1, JavaNear::klass_offset()));
    bl("shared_call_vm_oop");

    sub(sp, sp, imm(16 * BytesPerWord));
    stmia(sp, range(r0, r12));
    // tmp holds the newly created object - store it in C-safe register
    mov(tmp, reg(r0));
    mov(r0, reg(sp));
    bl("vsf_restore");
    // At this point the original values of registers are prepared on C stack.
    // After the last ldmia no registers may be trashed.

    // Repeat the way of decoding the STR instruction at the return point
    ldr(lr, imm_index(sp, (int)pc * BytesPerWord));
    ldr(r0, imm_index(lr, -BytesPerWord));
    // This time we need Rn field of the instruction,
    // because Rn is mapped to result object
    andr(r0, r0, imm(0xf0000), set_CC);
    // Put result in Rn slot. It will be loaded by the next ldmia instruction
    // as well as all other mapped registers of VSF.
    str(tmp, add_index(sp, r0, lsr, 14), ne);
    ldmia(sp, range(r0, r12));
    add(sp, sp, imm(16 * BytesPerWord));
    // Go back to compiled code
    mov(pc, reg(lr));
  }
#endif // ENABLE_INLINE_COMPILER_STUBS

  bind_global("compiler_callvm_stubs_end");
#endif // ENABLE_COMPRESSED_VSF
}

void CompilerStubs::generate_compiler_new_object() {
  Segment seg(this, code_segment, "Compiler new object");
  bind_global("compiler_new_object");

#if ENABLE_INLINE_COMPILER_STUBS
  ldr_label(r3, "_newobject");
  goto_shared_call_vm(T_OBJECT);
#else // ENABLE_INLINE_COMPILER_STUBS
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

  if (!ENABLE_ZERO_YOUNG_GENERATION) {
    comment("Zero object fields");
    zero_region(result, size, heap_top, result_end, BytesPerWord, true, true);
  }

  comment("The newly allocated object is in register r0");
  jmpx(lr);

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  ldr_label(r3, "_newobject");
  mov(r1, reg(klass));
  goto_shared_call_vm(T_OBJECT);
#endif // ENABLE_INLINE_COMPILER_STUBS
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

  if (!ENABLE_ZERO_YOUNG_GENERATION) {
    comment("Zero array elements");
    zero_region(r0, length, heap_top, result_end, Array::base_offset(),
                false, false);
  }

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

#if ENABLE_INLINE_COMPILER_STUBS
  comment("r1 = klass, r2 = length");
  ldr(r1, imm_index(r1, TypeArrayClass::class_info_offset()));
  ldr_label(r3, "_newarray");
  ldr(r1, imm_index(r1, ClassInfo::type_offset()));
  goto_shared_call_vm(T_ARRAY);
#else // ENABLE_INLINE_COMPILER_STUBS
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

  if (!ENABLE_ZERO_YOUNG_GENERATION) {
    comment("Zero typed array fields");
    zero_region(r0, size, heap_top, result_end, Array::base_offset(),
                true, true);
  }

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
#endif // ENABLE_INLINE_COMPILER_STUBS
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
#if !ENABLE_PAGE_PROTECTION
  Segment seg(this, code_segment, "Compiler timer tick");
  bind_global("compiler_timer_tick");
#if ENABLE_THUMB_REGISTER_MAPPING
  comment("caller is thumb mode, but does not set low bit of lr");
  add(lr, lr, imm(1));
#endif
  ldr_label(r3, "timer_tick");
  goto_shared_call_vm(T_VOID);
#endif // !ENABLE_PAGE_PROTECTION
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

#if ENABLE_ARM_VFP
void CompilerStubs::generate_vfp_redo(void) {
  Segment seg(this, code_segment, "VFP redo");
  typedef Assembler::Register Register;

  #define load_index  Register(r4)
  #define load_value  Register(r0)
  #define store_index Register(r4)
  #define store_value Register(r0)
  #define temp        Register(r12)

  {
    comment("Load contents of VFP register in r2 to r12");
    bind_global("vfp_load");
    andr(temp, load_index, imm(0x1F));
    add(pc, pc, imm_shift(temp, lsl, times_8));
    nop();


// VALUE OF r1 is over written in Unary

    comment("Instruction table");
    for (int i = 0; i < number_of_float_registers; i++) {
      fmrs(load_value, Register(s0 + i));
      mov_reg(pc, lr);
    }
  }

  {
    comment("Store contents of r0 to VFP register in r1");
    bind_global("vfp_store");
    add(pc, pc, imm_shift(store_index, lsl, times_8));
    nop();

    comment("Instruction table");
    for (int i = 0; i < number_of_float_registers; i++) {
      fmsr(Register(s0 + i), store_value);
      mov_reg(pc, lr);
    }
  }

  {
    comment("VFP redo stub");
    comment("To be called in the context:");
    comment("  vfp instruction");
    comment("  fmrx reg, fpscr");
    comment("  tst  reg, 0x8f");
    comment("  blne vfp_redo");

    bind_global("vfp_redo");

    #define op1   Register(r0)
    #define op2   Register(r1)
    #define inst  Register(r8)

    GUARANTEE(op1 != op2 && op1 != temp && op1 != inst && op2 != temp, "sanity");

    const Address4 save_set = join(range(r0, r4), set(r8, r12, lr));
    stmfd(sp, save_set, writeback);
    comment("save all vfp registers");
    fstmdbs(sp, s0, number_of_float_registers, writeback);

    comment("Fetch the instruction");
    ldr(inst, imm_index(lr, -16));

    comment("Fm");
    andr(load_index, inst, imm(0xF));
    mov(temp, imm_shift(inst, lsr, 6), set_CC);
    adc(load_index, load_index, reg(load_index));
    bl("vfp_load");
    if( op2 != load_value ) {
      mov_reg(op2, load_value);
    }

    comment("Fn");
    mov(load_index, imm_shift(inst, lsr, 16));
    andr(load_index, load_index, imm(0xF));
    mov(temp, imm_shift(inst, lsr, 8), set_CC);
    adc(load_index, load_index, reg(load_index));
    bl("vfp_load");
    if( op1 != load_value ) {
      mov_reg(op1, load_value);
    }

    comment("p, q, r, s");
    // Get q r
    mov(lr, imm_shift(inst, lsr, 20));
    andr(lr, lr, imm(0x3));

    // Get p
    mov(temp, imm_shift(inst, lsr, 23));
    andr(temp, temp, imm(0x1));
    orr(lr, lr, imm_shift(temp, lsl, 2));

    // Get s
    mov(temp, imm_shift(inst, lsr, 7), set_CC);
    adc(temp, lr, reg(lr));

    enum {
      skipped_primary_opcodes = 4,
      defined_primary_opcodes = 5,
      extension_opcodes       = 32
    };

    cmp(temp, imm(0xF));
    sub(temp, temp, imm(skipped_primary_opcodes+1), ne);

    mov_reg(op1, op2, no_CC, eq);
    add(temp, load_index, imm(defined_primary_opcodes-1), eq);

    add(lr, pc, imm((defined_primary_opcodes+extension_opcodes) * 4 + 2));
    // Special case for fcvtds aka f2d.
    // The result needs to be returned in a double register
    cmp(temp, imm(0x13));
    add(lr, lr, imm(16), ne);

    // There must be exactly one instruction
    // between loading lr and the table start
    ldr(pc, add_index(pc, temp, lsl, times_4));

    // *** Binary opcodes ***
    // Skipped primary opcodes:
      //define_long(0);         // fmacs
      //define_long(0);         // fnmacs
      //define_long(0);         // fmscs
      //define_long(0);         // fnmscs
    // Defined primary opcodes:
      define_long("jvm_fmul");  // fmuls
      define_long(0);           // fnmuls
      define_long("jvm_fadd");  // fadds
      define_long("jvm_fsub");  // fsubs
      define_long("jvm_fdiv");  // fdivs
    // *** Unary opcodes ***
      define_long(0);           // fcpys
      define_long(0);           // fabss
      define_long(0);           // fnegs
      define_long(0);           // fsqrts
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // fcmps
      define_long(0);           // fcmpes
      define_long(0);           // fcmpzs
      define_long(0);           // fcmpezs
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long("jvm_f2d");   // fcvtds
      define_long(0);           // fuitos
      define_long("jvm_i2f");   // fsitos
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // ftouis
      define_long(0);           // ftouizs
      define_long(0);           // ftosis
      define_long("jvm_f2i");   // ftosizs
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined

    // For handling f2d
    b("vfp_double_epilog");

    comment("restore all vfp registers");
    fldmias(sp, s0, number_of_float_registers, writeback);

    comment("Fd");
    mov(store_index, imm_shift(inst, lsr, 12));
    andr(store_index, store_index, imm(0xF));
    mov(temp, imm_shift(inst, lsr, 23), set_CC);
    adc(store_index, store_index, reg(store_index) );
    bl("vfp_store");

    fmrx(temp, fpscr);
    mov(temp, imm_shift(temp, lsr, 8));
    mov(temp, imm_shift(temp, lsl, 8));
    fmxr(fpscr, temp);

    ldmia(sp, save_set, writeback);
    mov_reg(pc, lr);

    #undef op1
    #undef op2
    #undef inst
  }


  {
    comment("VFP fcmp redo stub");
    comment("To be called in the context:");
    comment("  vfp fcmp instruction");
    comment("  fmstat");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  fmrx reg, fpscr");
    comment("  tst  reg, 0x8f");
    comment("  blne vfp_redo");

    bind_global("vfp_fcmp_redo");

    #define op1   Register(r0)
    #define op2   Register(r1)
    #define inst  Register(r8)
    #define op    Register(r2)

    stmfd(sp, set(lr), writeback);

    const Address4 save_set = join(range(r0, r4), set(r8, r12));
    stmfd(sp, save_set, writeback);

    comment("Fetch the instruction with operands");
    ldr(inst, imm_index(lr, -44));

    comment("Fetch the instruction to identify the operation");
    ldr(op, imm_index(lr, -32));

    comment("Fm");
    andr(load_index, inst, imm(0xF));
    mov(temp, imm_shift(inst, lsr, 6), set_CC);
    adc(load_index, load_index, reg(load_index));
    bl("vfp_load");
    if( op2 != load_value ) {
      mov_reg(op2, load_value);
    }

    comment("Fd");
    mov(load_index, imm_shift(inst, lsr, 12));
    andr(load_index, load_index, imm(0xF));
    mov(temp, imm_shift(inst, lsr, 23), set_CC);
    adc(load_index, load_index, reg(load_index));
    bl("vfp_load");
    if( op1 != load_value ) {
      mov_reg(op1, load_value);
    }

    comment("Identify the operation");

    mov(op, imm_shift(op, lsl, 1), set_CC);
    sbc(op, op, reg(op));
    comment("Now op is equal -1(fcmpg) or 0(fcmpl)");

    add(lr, pc, imm(2 * 4));
    // There must be exactly one instruction
    // between loading lr and the table start
    ldr(pc, add_index(pc, op, lsl, times_4));
    define_long("jvm_fcmpg");
    define_long("jvm_fcmpl");

    mov_reg(lr, r0);

    comment("Clear the FPSCR");
    fmrx(temp, fpscr);
    mov(temp, imm_shift(temp, lsr, 8));
    mov(temp, imm_shift(temp, lsl, 8));
    fmxr(fpscr, temp);

    comment("Restore saved set");
    ldmia(sp, save_set, writeback);

    comment("Push result to stack");
    stmfd(sp, set(lr), writeback);

    comment("Load lr from stack");
    ldr(lr, imm_index(sp, 4));

    comment("Identify result register");
    ldr(lr, imm_index(lr, -16));

    comment("Intentionally no branch to arm_reg_pop_and_ret");

    #undef op1
    #undef op2
    #undef inst
    #undef op
  }

  {
    comment("Pop stack top to register in lr and return");
    bind_global("arm_reg_pop_and_ret");

    mov(lr, imm_shift(lr, lsr, 12));
    andr(lr, lr, imm(0xF));
    add(pc, pc, imm_shift(lr, lsl, times_8));
    nop();

    comment("Instruction table");
    for (Register r = r0; r < pc; r = Register(r+1) ) {
      if (r != sp) {
        ldmia(sp, set(r),  writeback);
        ldmia(sp, set(pc), writeback);
      } else {
         nop();
         nop();
      }
    }
  }

  #undef load_index
  #undef load_value
  #undef store_index
  #undef store_value
  #undef temp
}

void CompilerStubs::generate_vfp_double_redo(void) {
  Segment seg(this, code_segment, "VFP double redo");
  typedef Assembler::Register Register;

  #define load_index     Register(r4)
  #define load_value_lo  Register(r0)
  #define load_value_hi  Register(r1)
  #define store_index    Register(r4)
  #define store_value_lo Register(r0)
  #define store_value_hi Register(r1)
  #define temp           Register(r12)

  {
    comment("Load contents of VFP register in r4 to r2 and r3");
    bind_global("vfp_double_load");
    add(pc, pc, imm_shift(load_index, lsl, times_8));
    nop();

    comment("Instruction table");
    for (int i = 0; i < number_of_float_registers; i+=2) {
      fmrrd(load_value_lo, load_value_hi, Register(d0 + i));
      mov_reg(pc, lr);
    }
  }

  {
    comment("Store contents of r10 and r12 to VFP register in r4");
    bind_global("vfp_double_store");
    add(pc, pc, imm_shift(store_index, lsl, times_8));
    nop();

    comment("Instruction table");
    for (int i = 0; i < number_of_float_registers; i+=2) {
      fmdrr(Register(d0 + i), store_value_lo, store_value_hi);
      mov_reg(pc, lr);
    }
  }

  {
    comment("VFP redo stub");
    comment("To be called in the context:");
    comment("  vfp instruction");
    comment("  fmrx reg, fpscr");
    comment("  tst  reg, 0x8f");
    comment("  blne vfp_double_redo");

    bind_global("vfp_double_redo");

    #define op1_lo load_value_lo
    #define op1_hi load_value_hi
    #define op2_lo Register(r2)
    #define op2_hi Register(r3)

    #define inst   Register(r8)

    GUARANTEE(temp >= Register(r4) && inst >= Register(r4), "sanity");

    const Address4 save_set = join(range(r0, r4), set(r8, r12, lr));
    stmfd(sp, save_set, writeback);
    comment("save all vfp registers");
    fstmdbs(sp, s0, number_of_float_registers, writeback);


    comment("Fetch the instruction");
    ldr(inst, imm_index(lr, -16));

    comment("Fm");
    andr(load_index, inst, imm(0xF));
    bl("vfp_double_load");
    mov_reg(op2_lo, load_value_lo);
    mov_reg(op2_hi, load_value_hi);

    comment("Fn");
    mov(load_index, imm_shift(inst, lsr, 16));
    andr(load_index, load_index, imm(0xF));
    bl("vfp_double_load");

    comment("Get instruction encoding");
    mov(temp, imm_shift(inst, lsr, 8), set_CC);
    adc(load_index, load_index, reg(load_index));

    comment("p, q, r, s");
    // Get q r
    mov(lr, imm_shift(inst, lsr, 20));
    andr(lr, lr, imm(0x3));

    // Get p
    mov(temp, imm_shift(inst, lsr, 23));
    andr(temp, temp, imm(0x1));
    orr(lr, lr, imm_shift(temp, lsl, 2));

    // Get s
    mov(temp, imm_shift(inst, lsr, 7), set_CC);
    adc(temp, lr, reg(lr));

    enum {
      skipped_primary_opcodes = 4,
      defined_primary_opcodes = 5,
      extension_opcodes       = 32
    };

    cmp(temp, imm(0xF));
    sub(temp, temp, imm(skipped_primary_opcodes+1), ne);

    mov_reg(op1_lo, op2_lo, no_CC, eq);
    mov_reg(op1_hi, op2_hi, no_CC, eq);
    add(temp, load_index, imm(defined_primary_opcodes-1), eq);

    add(lr, pc, imm((defined_primary_opcodes+extension_opcodes) * 4));

    // There must be exactly one instruction
    // between loading lr and the table start
    ldr(pc, add_index(pc, temp, lsl, times_4));

    // *** Binary opcodes ***
    // Skipped primary opcodes:
      //define_long(0);         // fmacd
      //define_long(0);         // fnmacd
      //define_long(0);         // fmscd
      //define_long(0);         // fnmscd
    // Defined primary opcodes:
      define_long("jvm_dmul");  // fmuld
      define_long(0);           // fnmuld
      define_long("jvm_dadd");  // faddd
      define_long("jvm_dsub");  // fsubd
      define_long("jvm_ddiv");  // fdivd
    // *** Unary opcodes ***
      define_long(0);           // fcpyd
      define_long(0);           // fabsd
      define_long(0);           // fnegd
      define_long(0);           // fsqrtd
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // fcmpd
      define_long(0);           // fcmped
      define_long(0);           // fcmpzd
      define_long(0);           // fcmpezd
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long("jvm_d2f");   // fcvtsd
      define_long(0);           // fuitod
      define_long("jvm_i2d");   // fsitod
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // ftouid
      define_long(0);           // ftouizd
      define_long(0);           // ftosid
      define_long("jvm_d2i");   // ftosizd
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined
      define_long(0);           // undefined

    bind_global("vfp_double_epilog");

    comment("restore all vfp registers");
    fldmias(sp, s0, number_of_float_registers, writeback);

    comment("Fd");
    mov(store_index, imm_shift(inst, lsr, 12));
    andr(store_index, store_index, imm(0xF));
    bl("vfp_double_store");

    fmrx(temp, fpscr);
    mov(temp, imm_shift(temp, lsr, 8));
    mov(temp, imm_shift(temp, lsl, 8));
    fmxr(fpscr, temp);

    ldmia(sp, save_set, writeback);
    mov_reg(pc, lr);

    #undef op1_lo
    #undef op1_hi
    #undef op2_lo
    #undef op2_hi
    #undef inst
  }

  {
    comment("VFP dcmp redo stub");
    comment("To be called in the context:");
    comment("  vfp fcmp instruction");
    comment("  fmstat");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  ...");
    comment("  fmrx reg, fpscr");
    comment("  tst  reg, 0x8f");
    comment("  blne vfp_dcmp_redo");

    bind_global("vfp_dcmp_redo");

    #define op1_lo   Register(r0)
    #define op1_hi   Register(r1)
    #define op2_lo   Register(r2)
    #define op2_hi   Register(r3)

    #define inst  Register(r8)
    #define op    Register(r12)

    stmfd(sp, set(lr), writeback);

    const Address4 save_set = join(range(r0, r4), set(r8, r12));
    stmfd(sp, save_set, writeback);

    comment("Fetch the instruction with operands");
    ldr(inst, imm_index(lr, -44));

    comment("Fetch the instruction to identify the operation");
    ldr(op, imm_index(lr, -32));

    comment("Fm");
    andr(load_index, inst, imm(0xF));
    bl("vfp_double_load");
    mov_reg(op2_lo, load_value_lo);
    mov_reg(op2_hi, load_value_hi);

    comment("Fd");
    mov(load_index, imm_shift(inst, lsr, 12));
    andr(load_index, load_index, imm(0xF));
    bl("vfp_double_load");

    comment("Identify the operation");

    mov(op, imm_shift(op, lsl, 1), set_CC);
    sbc(op, op, reg(op));
    comment("Now op is equal -1(dcmpg) or 0(dcmpl)");

    add(lr, pc, imm(2 * 4));
    // There must be exactly one instruction
    // between loading lr and the table start
    ldr(pc, add_index(pc, op, lsl, times_4));
    define_long("jvm_dcmpg");
    define_long("jvm_dcmpl");

    mov_reg(lr, r0);

    comment("Clear the FPSCR");
    fmrx(temp, fpscr);
    mov(temp, imm_shift(temp, lsr, 8));
    mov(temp, imm_shift(temp, lsl, 8));
    fmxr(fpscr, temp);

    comment("Restore saved set");
    ldmia(sp, save_set, writeback);

    comment("Push result to stack");
    stmfd(sp, set(lr), writeback);

    comment("Load lr from stack");
    ldr(lr, imm_index(sp, 4));

    comment("Identify result register");
    ldr(lr, imm_index(lr, -16));

    b("arm_reg_pop_and_ret");

    #undef op1_lo
    #undef op1_hi
    #undef op2_lo
    #undef op2_hi
    #undef inst
    #undef op
  }
  #undef load_index
  #undef load_value_lo
  #undef load_value_hi
  #undef store_index
  #undef store_value_lo
  #undef store_value_hi
  #undef temp
}
#endif // ENABLE_ARM_VFP

#endif // ENABLE_INTERPRETER_GENERATOR
