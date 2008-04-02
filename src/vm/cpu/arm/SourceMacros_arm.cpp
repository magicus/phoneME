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
#include "incls/_SourceMacros_arm.cpp.incl"

#ifndef PRODUCT

extern "C" void print_warning(const char* msg) {
  warning(msg == NULL ? "<no msg>" : msg);
}

#if ENABLE_INTERPRETER_GENERATOR && !ENABLE_THUMB_COMPILER

void SourceMacros::report_warning(const char* msg, Condition cond) {
  static Label L("print_warning");
  import(L);
  // do it conditionally
  Label exit;
  if (cond != al) {
    b(exit, not_cond(cond));
  }
  breakpoint(); // stop here in debug mode
  push(join(range(r0, r12), set(lr)));
  ldr_string(r0, msg);
  bl(L);
  pop(join(range(r0, r12), set(lr)));
  bind(exit);
}

void SourceMacros::report_fatal(const char* msg, Condition cond) {
  static Label L("print_fatal");
  import(L);
  breakpoint(cond);  // stop here in debug mode
  ldr_string(r0, msg, cond);
  bl(L, cond);
}

void SourceMacros::verify_gp_register(Register tmp) {
  if (GenerateDebugAssembly) {
    // make sure gp holds the right value
    ldr_gp_base(tmp);
    cmp(tmp, reg(gp));
    breakpoint(ne);
  }
}

void SourceMacros::verify_cpool_register(Register tmp) {
  if (GenerateDebugAssembly) {
    get_cpool(tmp);
    add(tmp, tmp, imm(ConstantPool::base_offset()));
    cmp(tmp, reg(cpool));
    ldr(tmp,  imm_index(fp, JavaFrame::cpool_offset()), eq);
    cmp(tmp, reg(cpool),                                eq);
    breakpoint(ne);
  }
}

void SourceMacros::debug_on_variable(const char *var, Condition cond, Register tmp) {
  if (GenerateDebugAssembly) {
    comment("Break if %s %s 0", var, Disassembler::cond_name(cond));
    Label skip;
    ldr_label(tmp, var);
    ldr(tmp, imm_index(tmp));
    cmp(tmp, zero);
    b(skip, not_cond(cond));
    breakpoint();
    bind(skip);
  }
}

void SourceMacros::zero_region(Register item, Register size,
                               Register tmpA, Register tmpB,
                               int header_size,
                               bool size_includes_header,
                               bool size_is_in_bytes) {
  // Note that "item" is preserved, but "size" is bashed
  Label loop;
  Register ptr = tmpA;
  Register zero = tmpB;

  int divisor = size_is_in_bytes ? 1 : BytesPerWord;
  int offset = BytesPerWord + (size_includes_header ? header_size : 0);
  GUARANTEE((offset % divisor) == 0, "Bad header size");

  add(ptr, item, imm(header_size));
  mov(zero, Assembler::zero);
  sub(size, size, imm(offset/divisor), set_CC);
bind(loop);
  comment("r%d = the next word to zero", ptr);
  comment("r%d = 0", zero);
  comment("r%d = the_number_of_%ss_still_to_zero - %d", size,
          (size_is_in_bytes ? "byte" : "word"),
          BytesPerWord/divisor);
  comment("condition codes reflect the (signed) value in r%d", size);
  str(zero, imm_index(ptr, BytesPerWord, post_indexed), ge);
  str(zero, imm_index(ptr, BytesPerWord, post_indexed), gt);
  sub(size, size, imm(2 * BytesPerWord/divisor), set_CC);
  b(loop, ge);
}

void SourceMacros::verify_tag(Register reg, Tag tag) {
  if (GenerateDebugAssembly && TaggedJavaStack) {
    comment("verify tag");
    cmp(reg, imm(MAKE_IMM(tag)));
    report_fatal("illegal tag", ne);
  }
}


Assembler::Address2 SourceMacros::local_addr_at(int index, int word_offset) {
  return imm_index(locals, word_offset * BytesPerWord
                        + JavaStackDirection * index * BytesPerStackElement);
}

Assembler::Address2 SourceMacros::local_addr_at(Register index) {
  if (JavaStackDirection < 0) {
    return sub_index(locals, index, lsl, LogBytesPerStackElement);
  } else {
    return add_index(locals, index, lsl, LogBytesPerStackElement);
  }
}

void SourceMacros::get_local_addr(Register result, int index) {
  eol_comment("address of local #%d", index);
  add_imm(result, locals, JavaStackDirection * index * BytesPerStackElement);
}

void SourceMacros::get_local_addr(Register result, Register index) {
  eol_comment("address of local #r%d", index);
  if (JavaStackDirection < 0) {
    sub(result, locals, imm_shift(index, lsl, LogBytesPerStackElement));
  } else {
    add(result, locals, imm_shift(index, lsl, LogBytesPerStackElement));
  }
}


static const char* tag_name(Tag tag) {
  switch (tag) {
    case obj_tag   : return "obj";
    case int_tag   : return "int";
    case long_tag  : return "long";
    case float_tag : return "float";
    case double_tag: return "double";
    case ret_tag   : return "ret";
  }
  SHOULD_NOT_REACH_HERE();
  return NULL;
}

void SourceMacros::ldrb_at_bcp(Register reg, int offset) {
  comment("load unsigned byte at bcp + %d", offset);
  ldrb(reg, imm_index(bcp, offset));
}

void SourceMacros::ldrh_at_bcp_A(Register reg, Register tmp, int offset) {
  comment("load unsigned 16bit word at bcp + %d", offset);
  ldrb(reg, imm_index(bcp, offset));
  ldrb(tmp, imm_index(bcp, offset + 1));
}

void SourceMacros::ldrh_at_bcp_B(Register reg, Register tmp, int offset) {
  orr(reg, tmp, imm_shift(reg, lsl, BitsPerByte));
}


void SourceMacros::ldr_receiver(Register dst, Register count) {
  eol_comment("address of stack pointer just past receiver");
  if (JavaStackDirection < 0) {
    add(dst, jsp, imm_shift(count, lsl, LogBytesPerStackElement));
  } else {
    sub(dst, jsp, imm_shift(count, lsl, LogBytesPerStackElement));
  }
  eol_comment("receiver");
  ldr(dst, imm_index(dst, JavaFrame::arg_offset_from_sp(-1)));
}

void SourceMacros::ldr_indexed(Register dst, Register base, Register index,
                               ScaleFactor scale_shift, int offset_12) {
  if (offset_12 == 0) {
    ldr(dst, add_index(base, index, lsl, scale_shift));
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldr(dst, imm_index(dst, offset_12));
  }
}

void SourceMacros::ldrb_indexed(Register dst, Register base, Register index,
                                ScaleFactor scale_shift, int offset_12) {
  if (offset_12 == 0) {
    ldrb(dst, add_index(base, index, lsl, scale_shift));
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldrb(dst, imm_index(dst, offset_12));
  }
}

void SourceMacros::ldrh_indexed(Register dst, Register base, Register index,
                                ScaleFactor scale_shift, int offset_8) {
  if (offset_8 == 0 && scale_shift == times_1) {
    ldrh(dst, add_index3(base, index));
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldrh(dst, imm_index3(dst, offset_8));
  }
}

void SourceMacros::ldr_cp_entry_A(Register dst, Register addr,
                                int addr_result, int size, int offset) {
  GUARANTEE(size == 1 || size == 2, "Bad parameter");
  if (size == 2) {
    comment("Get two bytes of constant pool entry");
    ldrb(addr, imm_index(bcp, offset));
    ldrb(dst,  imm_index(bcp, offset+1));
  } else {
    comment("Get byte of constant pool entry");
    ldrb(dst, imm_index(bcp, offset));
  }
}

void SourceMacros::ldr_cp_entry_B(Register dst, Register addr,
                                int addr_result, int size, int offset) {
  if (size == 1) {
    if (addr_result == addr_cpool_index) {
      eol_comment("get address of cpool entry");
      add(addr, cpool, imm_shift(dst, lsl, times_4));
    }
    eol_comment("load constant pool entry");
    ldr(dst, add_index(cpool, dst, lsl, times_4));
  } else {
    add(addr, cpool, imm_shift(addr, lsl, 2 + 8));
    ldr(dst, add_index(addr, dst, lsl, times_4, pre_indexed));
  }
}

// dst = _class_index_top[index]
void SourceMacros::ldr_class_from_index_and_base(Register dst, Register index,
                                                 Register base,
                                                 Condition cond) {
  ldr(dst, add_index(base, index, lsl, 2), cond);
}

void SourceMacros::prefetch(int step) {
  GUARANTEE(step >= 0, "stepping backwards in bytecodes - are you sure?");
  eol_comment("prefetch next bytecode");
  // We could use pre_indexed for both, but it looks silly.
  ldrb(bcode, imm_index(bcp, step, step == 0 ? offset : pre_indexed));
}

void SourceMacros::prefetch(Register step) {
  eol_comment("prefetch next bytecode");
  ldrb(bcode, add_index(bcp, step, lsl, times_1, pre_indexed));
}

void SourceMacros::set_stack_state_to(State state, Condition cond) {
  switch(state) {
    case tos_in_regs:
      comment("Change state to tos_in_regs");
      // Note:  We perform the pop before the adjusting of the stack to
      // help eliminate register stalls.
      if (!ENABLE_TOS_CACHING) {
        StackTypeMode mode = ENABLE_FULL_STACK ? full : empty;
        eol_comment("cache topmost element of stack");
        pop(tos, cond, mode);
      }
      if (!ENABLE_FULL_STACK) {
        eol_comment("adjust stack pointer");
        sub_imm(jsp, jsp, JavaStackDirection * BytesPerWord, no_CC, cond);
      }
      break;

    case tos_on_stack:
      comment("Change state to tos_on_stack");
      if (!ENABLE_FULL_STACK) {
        eol_comment("adjust stack pointer");
        sub_imm(jsp, jsp, JavaStackDirection * BytesPerWord, no_CC, cond);
      }
      if (ENABLE_TOS_CACHING) {
        eol_comment("uncache topmost element of stack");
        push(tos, cond);
      }
      break;

    case tos_interpreter_basic:
      break;

    default:
      SHOULD_NOT_REACH_HERE();
  }
}

void SourceMacros::restore_stack_state_from(State state, Condition cond) {
  switch(state) {
    case tos_in_regs:
      // We adjust the stack pointer before pushing the result in order
      // to minimize pipeline stalls
      // comment("Restore stack stack from tos_in_regs");
      if (!ENABLE_FULL_STACK) {
        eol_comment("Adjust the stack");
        add_imm(jsp, jsp, JavaStackDirection * BytesPerWord, no_CC, cond);
      }
      if (!ENABLE_TOS_CACHING) {
        StackTypeMode mode = ENABLE_FULL_STACK ? full : empty;
        eol_comment("uncache topmost element of stack");
        push(tos, cond, mode);
      }
      break;

    case tos_on_stack:
      comment("Restore stack state from tos_on_stack");
      if (ENABLE_TOS_CACHING) {
        eol_comment("cache topmost element of stack");
        pop(tos, cond);
      }
      if (!ENABLE_FULL_STACK) {
        add_imm(jsp, jsp, JavaStackDirection * BytesPerWord, no_CC, cond);
      }
      break;

    case tos_interpreter_basic:
      break;

    default:
      SHOULD_NOT_REACH_HERE();
  }
}

void SourceMacros::pop_arguments(int count) {
  StackTypeMode mode = ENABLE_FULL_STACK ? full : empty;
  comment("Entry: Count = %d", count);
  if (count < 0) {
    int pop = -count;
    int real_pop = ENABLE_TOS_CACHING ? pop - 1 : pop;
    if (real_pop != 0) {
      sub_imm(jsp, jsp, real_pop * JavaStackDirection *
              BytesPerStackElement);
    }
  } else if (count == 0) {
    if (ENABLE_TOS_CACHING) {
      push(tos, al, mode);
    }
  } else {
    GUARANTEE(count <= 4, "Cannot handle large counts");
    if (!ENABLE_TOS_CACHING) {
      pop(tos, al, mode);
    }
    if (count >= 2) {
      pop(tmp01, al, mode);
    }
    if (count >= 3) {
      pop(tmp23, al, mode);
    }
    if (count >= 4) {
      pop(tmp45, al, mode);
    }
  }
}

void SourceMacros::simple_c_bytecode(const char* name,
                              BasicType result, BasicType arg1, BasicType arg2,
                              bool commutative)
{
  simple_c_setup_arguments(arg1, arg2, commutative);
  fast_c_call(name);
  prefetch(1);
  simple_c_store_result(result);
  dispatch(tos_interpreter_basic);
}

#if USE_FP_RESULT_IN_VFP_REGISTER
void SourceMacros::pop_fp_result(BasicType type) {

  const Register A = JavaStackDirection < 0 ? s0 : s1;
  const Register B = JavaStackDirection < 0 ? s1 : s0;

  switch (type) {
    case T_FLOAT:
      comment("load float");
      // return type float, size = 1
      if (ENABLE_TOS_CACHING) {
        fmsr(s0, r0);
      } else {
        pop(s0);
      }
      break;
    case T_DOUBLE:
      comment("load double");
      // return type double, size = 2
      if (ENABLE_TOS_CACHING) {
        fmsr(A, r0);
      } else {
        pop(A);
      }
      pop(B);
      break;
    default:
      simple_c_setup_arguments(type);
  }
}
#endif // USE_FP_RESULT_IN_VFP_REGISTER

void SourceMacros::simple_c_setup_arguments(BasicType arg1, BasicType arg2,
                                            bool commutative) {
  StackTypeMode mode = ENABLE_FULL_STACK ? full : empty;

  if (arg2 == T_VOID) {
    // The logic below is simplified if we assume unary functions have a null
    // first argument.
    arg2 = arg1; arg1 = T_VOID;
  }
  int count_1 = arg1 == T_VOID ? 0 : word_size_for(arg1);
  int count_2 = arg2 == T_VOID ? 0 : word_size_for(arg2);
  // We get slightly better code if we reverse the arguments, which we can
  // only do if it is commutative.
  int start_1 = commutative ? count_1 : 0;
  int start_2 = commutative ? 0 : count_1;
  int i;

  comment("load one argument");
  for (i = 0; i < count_2; i++) {
    Register target = JavaStackDirection > 0
                            ? as_register(start_2 + count_2 - i - 1)
                            : as_register(start_2 + i);
    if (ENABLE_TOS_CACHING && i == 0) {
      mov_reg(target, r0);
    } else {
      pop(target, al, mode);
    }
  }
  if (count_1 != 0) {
    comment("load other argument");
  }
  for (i = 0; i < count_1; i++) {
    Register target = JavaStackDirection > 0
                            ? as_register(start_1 + count_1 - i - 1)
                            : as_register(start_1 + i);
    pop(target, al, mode);
  }
}

void SourceMacros::simple_c_store_result(BasicType kind) {
  StackTypeMode mode = ENABLE_FULL_STACK ? full : empty;
  int count_r = word_size_for(kind);

  if (count_r == 0) {
    if (ENABLE_TOS_CACHING) {
      pop(tos, al, mode);
    }
  } else {
    comment("store result");
    for (int i = count_r; --i >= 0; ) {
      Register target = JavaStackDirection > 0
                              ? as_register(count_r - i - 1)
                              : as_register(i);
      if (ENABLE_TOS_CACHING && i == 0) {
        mov_reg(r0, target);
      } else {
        push(target, al, mode);
      }
    }
  }
}

void SourceMacros::dispatch(State state) {
   int cur_pos;
   comment("dispatch");
   eol_comment("get implementation of bytecode");
   _stream->mark();
   ldr(bcode, bytecode_impl(bcode));
   cur_pos = _stream->current_position();
   restore_stack_state_from(state);
#if ENABLE_CPU_VARIANT
   CPUVariantSupport cpu_variant_support(stream());
   cpu_variant_support.jmp(bcode);
#else
   if (cur_pos == _stream->current_position()) {
     _stream->reset();
     ldr(pc, bytecode_impl(bcode));
   } else {
     eol_comment("go to next bytecode");
     mov(pc, reg(bcode));
   }
#endif
}

void SourceMacros::dispatch(int count,
                            Address4 result1, Address4 result2,
                            Address4 result3, Address4 result4,
                            Address4 result5, Address4 result6) {
   int cur_pos;
   comment("dispatch");
   eol_comment("get implementation of bytecode");
   _stream->mark();
   ldr(bcode, bytecode_impl(bcode));
   cur_pos = _stream->current_position();
   push_results(count, result1, result2, result3, result4, result5, result6);
#if ENABLE_CPU_VARIANT
   CPUVariantSupport cpu_variant_support(stream());
   cpu_variant_support.jmp(bcode);
#else
   if (cur_pos == _stream->current_position()) {
     _stream->reset();
     ldr(pc, bytecode_impl(bcode));
   } else {
     eol_comment("go to next bytecode");
     mov(pc, reg(bcode));
   }
#endif
}

void SourceMacros::push_results(int count,
                                Address4 result1, Address4 result2,
                                Address4 result3, Address4 result4,
                                Address4 result5, Address4 result6) {
  StackTypeMode mode = ENABLE_TOS_CACHING ? full : empty;
//  comment("Exit: Count = %d", count);
  if (count == 0) {

    if (ENABLE_TOS_CACHING) {
      pop(tos, al, mode);
    }
  } else {
    if (count >= 6) {
      GUARANTEE(result6 != 0, "Must pass set");
      push(result6, al, mode);
    }
    if (count >= 5) {
      GUARANTEE(result5 != 0, "Must pass set");
      push(result5, al, mode);
    }
    if (count >= 4) {
      GUARANTEE(result4 != 0, "Must pass set");
      push(result4, al, mode);
    }
    if (count >= 3) {
      GUARANTEE(result3 != 0, "Must pass set");
      push(result3, al, mode);
    }
    if (count >= 2) {
      GUARANTEE(result2 != 0, "Must pass set");
      push(result2, al, mode);
    }
    GUARANTEE(result1 != 0, "Must pass set");
    if (!ENABLE_TOS_CACHING) {
      push(result1, al, mode);
    } else {
      int low_bit = result1 & -result1;
      Register low_reg = as_register(exact_log2(low_bit));
      mov_reg(tos_val, low_reg);
      if (TaggedJavaStack) {
        Register high_reg = as_register(exact_log2(result1 - low_bit));
        mov_reg(tos_tag, high_reg);
      }
    }
  }
}



void SourceMacros::get_thread(Register thread) {
  get_current_thread(thread);
}

void SourceMacros::get_thread_handle(Register thread) {
  get_current_thread_addr(thread);
}

void SourceMacros::call_from_interpreter(Register routine, int offset) {
  GUARANTEE(routine != lr, "Bad register");
  if (GenerateDebugAssembly && offset != 0) {
    // Some debuggers seem unable to single step through an "add pc, ..."
    add(routine, routine, imm(offset));
    offset = 0;
  }
#if ENABLE_EMBEDDED_CALLINFO
  add(lr, pc, imm(4));
#else
  mov(lr, reg(pc));
#endif // ENABLE_EMBEDDED_CALLINFO
  if (offset == 0) {
    jmpx(routine);
  } else {
    add(pc, routine, imm(offset));
  }
  define_call_info();
}

void SourceMacros::call_from_interpreter(const char* routine,
                                         bool routine_fixes_lr) {
#if ENABLE_EMBEDDED_CALLINFO
  if (routine_fixes_lr) {
    bl(routine);
  } else {
    add(lr, pc, imm(4));
    b(routine);
  }
  define_call_info();
#else
  bl(routine);
#endif // ENABLE_EMBEDDED_CALLINFO
}

void SourceMacros::get_bottom(Register reg, Condition cond) {
  eol_comment("stack bottom pointer");
  ldr(reg, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()), cond);
}

void SourceMacros::get_method(Register reg, Condition cond) {
  eol_comment("current method");
  ldr(reg, imm_index(fp, JavaFrame::method_offset()), cond);
}

void SourceMacros::get_method_parameter_size(Register result, Register method)
{
  ldrh(result, imm_index3(method, 
       Method::method_attributes_offset()));
  int num_bits = 32 - Method::RESULT_STORAGE_TYPE_SHIFT;
  mov(result, imm_shift(result, lsl, num_bits));

  eol_comment("%s = parameter size in words", reg_name(result));
  mov(result, imm_shift(result, lsr, num_bits));
}

void SourceMacros::get_cpool(Register reg, bool method_in_callee_reg) {
  comment("get constants pool");

  if (method_in_callee_reg) {
    eol_comment("get access flags");
    ldrh(reg, imm_index3(callee, Method::access_flags_offset()));
    eol_comment("compressed header?");
    tst(reg, imm(JVM_ACC_HAS_COMPRESSED_HEADER));
    get_rom_constant_pool_fast(reg,                     ne);
    eol_comment("method constant pool");
    ldr(reg, imm_index(callee, Method::constants_offset()), eq);
  } else {
    // This is only used in debugging, so we don't care that this code
    // could be improved by having a temp register.
    get_method(reg);
    eol_comment("get access flags");
    ldrh(reg, imm_index3(reg, Method::access_flags_offset()));
    eol_comment("compressed header?");
    tst(reg, imm(JVM_ACC_HAS_COMPRESSED_HEADER));
    get_method(reg,                                      eq);
    get_rom_constant_pool_fast(reg,                      ne);
    eol_comment("method constant pool");
    ldr(reg, imm_index(reg, Method::constants_offset()), eq);
  }
}

void SourceMacros::save_interpreter_state() {
  comment("save interpreter state");
  str(bcp, imm_index(fp, JavaFrame::bcp_store_offset()));
}

void SourceMacros::restore_interpreter_state(bool include_bcp) {
  comment("restore interpreter state");
  eol_comment("restore locals");
  ldr(locals, imm_index(fp, JavaFrame::locals_pointer_offset()));
  eol_comment("restore cpool");
  ldr(cpool,  imm_index(fp, JavaFrame::cpool_offset()));
  if (include_bcp) {
    eol_comment("restore bcp");
    ldr(bcp,    imm_index(fp, JavaFrame::bcp_store_offset()));
  }
}

void SourceMacros::interpreter_call_vm(const char *name,
                                       BasicType return_value_type,
                                       bool save_state) {
  ldr_label(r3, name);
  if (save_state) {
    save_interpreter_state();
  }
  interpreter_call_shared_call_vm(return_value_type);
  if (return_value_type == T_ILLEGAL) {
    // should never return
  } else if (save_state) {
    restore_interpreter_state();
  }
}

void SourceMacros::wtk_profile_quick_call(int param_size) { 
  if (!ENABLE_WTK_PROFILER) { 
    return;
  }
  comment("Create temporary frame for the profiler");
  if (param_size == -1) {
    get_method_parameter_size(tmp0, callee);
    if (JavaStackDirection < 0) {
      add(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
    } else {
      sub(locals, jsp, imm_shift(tmp0, lsl, LogBytesPerStackElement));
    }
    add_imm(locals, locals, JavaFrame::arg_offset_from_sp(-1));
  } else {
    add_imm(locals, jsp, JavaFrame::arg_offset_from_sp(param_size - 1));
  }

  eol_comment("Reserve space on stack for frame");
  add_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());

  mov_imm(cpool, 0);

  int offset = -  JavaFrame::empty_stack_offset();
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

  interpreter_call_vm("jprof_record_method_transition", T_VOID);

  ldr(callee,imm_index(fp, JavaFrame::method_offset()));
  ldr(lr,    imm_index(fp, JavaFrame::return_address_offset()));
  ldr(fp,    imm_index(fp, JavaFrame::caller_fp_offset()));
  sub_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());
}      
      

void SourceMacros::check_timer_tick() {
  eol_comment("Timer tick?");
#if ENABLE_PAGE_PROTECTION
  strb(gp, imm_index(gp, -4));
#elif ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
  define_long(0xEE13F170); // textrcb(0);
  b("interpreter_timer_tick", ne);
#else
  get_rt_timer_ticks(tmp0);
  cmp(tmp0, imm(0x0));
  b("interpreter_timer_tick", ne);
#endif
}

void SourceMacros::set_tag(Tag tag, Register reg) {
  if (TaggedJavaStack) {
    eol_comment("Set tag to %s", tag_name(tag));
    mov_imm(reg, tag);
  }
}

void SourceMacros::set_tags(Tag tag, Register reg1, Register reg2) {
  if (TaggedJavaStack) {
    eol_comment("Set tag to %s", tag_name(tag));
    mov_imm(reg1, 2*int(tag));
    mov_imm(reg2,    int(tag));
  }
}

void SourceMacros::set_return_type(BasicType type, Condition cond) {
  GUARANTEE(type == stack_type_for(type), "Must be stack type");
  if (type == T_VOID) {
    mov(method_return_type, zero, cond);
  } else {
    mov(method_return_type, imm(word_size_for(type)), cond);
  }
}

void SourceMacros::push(Register reg, Condition cond,
                        StackTypeMode type) {
  const int offset =  JavaStackDirection * BytesPerWord;

#if USE_FP_RESULT_IN_VFP_REGISTER
  GUARANTEE(is_arm_register(reg) ||
            is_vfp_register(reg), "Wrong register value");
  if (is_vfp_register(reg)) {
    if (type == full) {
      if (JavaStackDirection < 0) {
        fstmfds(jsp, reg, 1, writeback, cond);
      } else {
        add_imm(jsp, jsp, offset, no_CC, cond);
        fsts(reg, imm_index5(jsp, 0));
      }
    } else {
      if (JavaStackDirection < 0) {
        fsts(reg, imm_index5(jsp, 0));
        add_imm(jsp, jsp, offset, no_CC, cond);
      } else {
        fstmeas(jsp, reg, 1, writeback, cond);
      }
    }
  } else
#endif // USE_FP_RESULT_IN_VFP_REGISTER
  {
  Address2 addr = (type == full) ? imm_index(jsp, offset, pre_indexed)
                                 : imm_index(jsp, offset, post_indexed);
  str(reg, addr, cond);
  }
}

void SourceMacros::push(Address4 set, Condition cond,
                       StackTypeMode type) {
  if (is_power_of_2((int)set)) {
    Register reg = as_register(exact_log2(set));
    push(reg, cond, type);
  } else {
    if (type == full) {
      if (JavaStackDirection < 0) {
        stmfd(jsp, set, writeback, cond);
      } else {
        stmfa(jsp, set, writeback, cond);
      }
    } else {
      if (JavaStackDirection < 0) {
        stmed(jsp, set, writeback, cond);
      } else {
        stmea(jsp, set, writeback, cond);
      }
    }
  }
}

void SourceMacros::pop(Register reg, Condition cond,
                        StackTypeMode type) {
  int offset = -JavaStackDirection * BytesPerWord;
#if USE_FP_RESULT_IN_VFP_REGISTER
  GUARANTEE(((reg >= s0) && (reg <= s31)) ||
            ((reg >= r0) && (reg <= r15)), "Wrong register value");

  if (is_vfp_register(reg)) {
    if (type == full) {
      if (JavaStackDirection < 0) {
        fldmfds(jsp, reg, 1, writeback, cond);
      } else {
        add_imm(jsp, jsp, offset, no_CC, cond);
        flds(reg, imm_index5(jsp, 0));
      }
    } else {
      if (JavaStackDirection < 0) {
        flds(reg, imm_index5(jsp, 0));
        add_imm(jsp, jsp, offset, no_CC, cond);
      } else {
        fldmeas(jsp, reg, 1, writeback, cond);
      }
    }
  } else
#endif // USE_FP_RESULT_IN_VFP_REGISTER
  {
  Address2 addr = (type == full) ? imm_index(jsp, offset, post_indexed)
                                 : imm_index(jsp, offset, pre_indexed);
  ldr(reg, addr, cond);
  }
}

void SourceMacros::pop(Address4 set, Condition cond,
                       StackTypeMode type) {
  if (is_power_of_2((int)set)) {
    Register reg = as_register(exact_log2(set));
    pop(reg, cond, type);
  } else {
    if (type == full) {
      if (JavaStackDirection < 0) {
        ldmfd(jsp, set, writeback, cond);
      } else {
        ldmfa(jsp, set, writeback, cond);
      }
    } else {
      if (JavaStackDirection < 0) {
        ldmed(jsp, set, writeback, cond);
      } else {
        ldmea(jsp, set, writeback, cond);
      }
    }
  }
}

void SourceMacros::fast_c_call(Label name, Register preserved) {
  GUARANTEE(sp != jsp, "This configuration no more supported");
  GUARANTEE(is_c_saved_register(tmp2) && is_c_saved_register(tmp3),
            "Register sanity");

  mov(tmp3, reg(bcp));
  if (preserved != no_reg && !is_c_saved_register(preserved)) {
    mov(tmp2, reg(preserved));
  }
  bl(name);
  if (preserved != no_reg && !is_c_saved_register(preserved)) {
    mov(preserved, reg(tmp2));
  }
  mov(bcp, reg(tmp3));
}

void SourceMacros::unlock_activation() {
  Register lock = tmp0;
  Register object = tmp1;
  Register flags = tmp2;

  Label no_locks, locked_monitor;
  Label check_other_monitors;

  comment("unlock_activation");
  comment("See if there are any locks on this frame");
  eol_comment("address of stack bottom pointer");
  ldr(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
  get_method(flags);
  eol_comment("address of stack bottom pointer if no locks");
  add_imm(object, fp, JavaFrame::empty_stack_offset());
  cmp(lock, reg(object));
  if (GenerateDebugAssembly) {
    breakpoint(JavaStackDirection < 0 ? hi : lo);
  }
  b(no_locks, eq);

  set_stack_state_to(tos_on_stack);
  save_interpreter_state();

  comment("Is this a synchronized method?");
  eol_comment("access flags");
  ldrh(flags, imm_index3(flags, Method::access_flags_offset()));
  eol_comment("ACC_SYNCHRONIZED");
  tst(flags, imm(JVM_ACC_SYNCHRONIZED));
  b(check_other_monitors, eq);

  call_from_interpreter("shared_unlock_synchronized_method");
  ldr(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
  b(check_other_monitors);

bind(locked_monitor);
  ldr_label(r3, "illegal_monitor_state_exception");
  interpreter_call_shared_call_vm(T_ILLEGAL);

bind(check_other_monitors);
  comment("Check that all stack locks have been unlocked.");
  // lock is pointing at first word of stack lock
  ldr(object, imm_index(lock,
                        JavaStackDirection < 0 ? StackLock::size() : 0,
                        pre_indexed));
  // Point at the object field of the oldest stack lock
  add_imm(flags, fp, JavaFrame::first_stack_lock_offset() + StackLock::size());

  Label unlocking_loop;
bind(unlocking_loop);
  cmp(object, zero);
  b(locked_monitor, ne);
  cmp(lock, reg(flags));
  if (GenerateDebugAssembly) {
    breakpoint(JavaStackDirection < 0 ? hi : lo);
  }
  // Grab object, point at next object
  ldr(object, imm_index(lock,
                        -JavaStackDirection* (StackLock::size() + BytesPerWord),
                        pre_indexed),
                ne);
  b(unlocking_loop, ne);

  restore_stack_state_from(tos_on_stack);
  ldr(locals, imm_index(fp, JavaFrame::locals_pointer_offset()));
bind(no_locks);
}

void SourceMacros::interpreter_call_shared_call_vm(BasicType return_value_type){
  // The routines are identical to the shared_call_vm_xxx, but they first
  // increment lr by 4.
  if (stack_type_for(return_value_type) == T_OBJECT) {
    call_from_interpreter("interpreter_call_vm_oop", /* target fixes lr */true);
  } else {
    call_from_interpreter("interpreter_call_vm", /* target fixes lr */true);
  }
  if (return_value_type == T_ILLEGAL) {
    if (GenerateDebugAssembly) {
      eol_comment("Should never return");
      breakpoint();
    }
  }
}

void SourceMacros::goto_shared_call_vm(BasicType return_value_type) {
  if (stack_type_for(return_value_type) == T_OBJECT) {
    b("shared_call_vm_oop");
  } else {
    b("shared_call_vm");
  }
  if (GenerateDebugAssembly) {
    eol_comment("Should never return");
    breakpoint();
  }
}

#if ENABLE_INTERPRETATION_LOG
void SourceMacros::update_interpretation_log() {
  // _interpretation_log[] is a circular buffer that stores
  // the most recently executed interpreted methods.
  comment("Save this interpreted method into interpretation log");
  get_interpretation_log_idx(tmp1);
  get_interpretation_log_addr(tmp0);
  str(callee, add_index(tmp0, tmp1));
  add(tmp1, tmp1, imm(sizeof(int)));
  andr(tmp1, tmp1, imm(INTERP_LOG_MASK));
  set_interpretation_log_idx(tmp1);
}
#endif

void SourceMacros::return_from_invoker(int prefetch_size, 
                                       int result_type) {
  if (!ENABLE_WTK_PROFILER) {
     prefetch(prefetch_size);
  }

  Register A = JavaStackDirection < 0 ? r0 : r1;
  Register B = JavaStackDirection < 0 ? r1 : r0;

#if USE_FP_RESULT_IN_VFP_REGISTER
  Register A_vfp = JavaStackDirection < 0 ? s0 : s1;
  Register B_vfp = JavaStackDirection < 0 ? s1 : s0;
#endif

  switch (result_type) {
    case Method::NO_RESULT:
      comment("return type is void");
      pop(tos_val);
      break;
    case Method::SINGLE:
      comment("result is in r0: do nothing");
      break;
    case Method::DOUBLE:
      comment("result is in r0/r1");      
      push(B);
      mov_reg(r0, A, no_CC);
      break;
#if USE_FP_RESULT_IN_VFP_REGISTER
    case Method::FP_SINGLE:
      comment("result is in s0");
      fmrs(r0, s0);
      break;
    case Method::FP_DOUBLE:
      comment("result is in s0/s1");
      fmrs(r0, A_vfp); 
      push(B_vfp);
      break;
#endif
    default:
      SHOULD_NOT_REACH_HERE();        
  }

  if (ENABLE_WTK_PROFILER) { 
    push(tos_val);
    interpreter_call_vm("jprof_record_method_transition", T_VOID);
    prefetch(prefetch_size);
    dispatch(tos_on_stack);
  } else { 
    dispatch(tos_in_regs);
  }
}

void SourceMacros::generate_call(Register entry, 
                                 Label& label,
                                 int result_type,
                                 int prefetch_size,
                                 char* deoptimization_entry_name) {
  bind(label);

  if (!USE_FP_RESULT_IN_VFP_REGISTER &&
      ((result_type == Method::FP_SINGLE) ||
       (result_type == Method::FP_DOUBLE))) {
    return;
  }

  GUARANTEE(USE_FP_RESULT_IN_VFP_REGISTER || 
            ((result_type != Method::FP_SINGLE) && 
             (result_type != Method::FP_DOUBLE)), "Wrong result type");
     
  comment("invoke method with result type == %d", result_type);
  call_from_interpreter(entry, 0);

  if (deoptimization_entry_name != NULL) {
    char buff[40];
#if !USE_FP_RESULT_IN_VFP_REGISTER
    if (result_type > 0) {
      jvm_sprintf(buff, "%s_%d", deoptimization_entry_name, result_type - 1);
      bind(buff);
    }
#endif
    jvm_sprintf(buff, "%s_%d", deoptimization_entry_name, result_type);
    bind(buff);
  }

  restore_interpreter_state();
  return_from_invoker(prefetch_size, result_type);
}

void SourceMacros::invoke_method(Register method,  Register entry,
                                 Register tmp, int prefetch_size,
                                 char *deoptimization_entry_name) {
  // callee must contain the method
  GUARANTEE(method == callee, "sanity");    
  ldrh(tmp, imm_index3(method, 
                       Method::method_attributes_offset()));  
  eol_comment("%s = result type", reg_name(tmp));
  mov(tmp, imm_shift(tmp, lsr, Method::RESULT_STORAGE_TYPE_SHIFT));    

  ldr(pc, add_index(pc, tmp, lsl, 2));

  Label labels[5];  
  nop();  
  define_long(labels[0]);
  define_long(labels[1]);
  define_long(labels[2]);
  define_long(labels[3]);
  define_long(labels[4]);

  for (int i = 0; i < Method::NUMBER_OF_RESULT_STORAGE_TYPES; i++) {
    generate_call(entry, labels[i], i, prefetch_size, 
      deoptimization_entry_name);
  }
}

void SourceMacros::swap_mask(Register msk) {
  // From: ARM Architecture Reference Manual, 2nd Ed.
  //       by David Seal, Addison-Wesley, Sect. 9.1.4
  eol_comment("compute swap mask");
  mov_imm(msk, 0xff00ff);
}

void SourceMacros::swap_bytes(Register res, Register tmp, Register msk) {
  // From: ARM Architecture Reference Manual, 2nd Ed.
  //       by David Seal, Addison-Wesley, Sect. 9.1.4
  GUARANTEE(res != tmp && res != msk && tmp != msk, "registers must be different");
  comment("swap bytes");                     // res = a  , b  , c  , d
  andr(tmp, res, reg(msk));                  // tmp = 0  , b  , 0  , d
  andr(res, msk, imm_shift(res, ror, 24));   // res = 0  , c  , 0  , a
  orr(res, res, imm_shift(tmp, ror,  8));    // res = d  , c  , b  , a
}

#if ENABLE_ISOLATES
void SourceMacros::get_mirror_from_clinit_list(Register tm, Register klass,
                                               Register temp) {
  Label find_task, found_class;

  get_current_task(temp);
  comment("clinit_list (task mirror) into tmp2");
  ldr(tm, imm_index(temp, Task::clinit_list_offset()));
bind(find_task);
  ldr(temp, imm_index(tm, TaskMirror::containing_class_offset()));
  cmp(klass, reg(temp));
  b(found_class, eq);
  ldr(tm, imm_index(tm, TaskMirror::next_in_clinit_list_offset()));
  if (GenerateDebugAssembly) {
    cmp(tm, zero);
    breakpoint(eq);
  }
  b(find_task);
bind(found_class);
}
#else
void SourceMacros::initialize_class_when_needed(Register dst,
                                                Register tmp1, Register tmp2,
                                                Label& restart,
                                                int args_from_stack) {
  Register mirror = tmp1;
  Register status = tmp2;
  Register thread = tmp2;
  Label class_is_initialized, do_initialize;
  
  comment("Get Java mirror for the class");
  ldr(mirror, imm_index(dst, JavaClass::java_mirror_offset()));
  comment("Check if the class is already initialized");
  ldr(status, imm_index(mirror, JavaClassObj::status_offset()));
  tst(status, imm(JavaClassObj::INITIALIZED));
  b(class_is_initialized, ne);
  comment("Check if the class is being initialized by the current thread");
  tst(status, imm(JavaClassObj::IN_PROGRESS));
  b(do_initialize, eq);
  get_thread(thread);
  ldr(mirror, imm_index(mirror, JavaClassObj::thread_offset()));
  ldr(thread, imm_index(thread, Thread::thread_obj_offset()));
  cmp(mirror, reg(thread));
  b(class_is_initialized, eq);

  bind(do_initialize);
  comment("Call VM runtime to initialize class");
  if (args_from_stack != 0) {
    push_results(args_from_stack);
    set_stack_state_to(tos_on_stack);
  }
  mov_reg(r1, dst);
  interpreter_call_vm("initialize_class", T_VOID);
  if (args_from_stack != 0) {
    restore_stack_state_from(tos_on_stack);
    pop_arguments(args_from_stack);
  }
  b(restart);
  bind(class_is_initialized);
}
#endif

#endif // PRODUCT

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
