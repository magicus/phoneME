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
#include "incls/_SourceMacros_thumb2.cpp.incl"

#ifndef PRODUCT

extern "C" void print_warning(const char* msg) {
  warning(msg == NULL ? "<no msg>" : msg);
}

#if ENABLE_INTERPRETER_GENERATOR

void SourceMacros::report_warning(const char* msg, Condition cond) {
  breakpoint();
#if 0 // BEGIN_CONVERT_TO_T2
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
#endif // END_CONVERT_TO_T2
}

void SourceMacros::report_fatal(const char* msg, Condition cond) {
  breakpoint(cond);  // stop here in debug mode
#if 0 // BEGIN_CONVERT_TO_T2
  static Label L("print_fatal");
  import(L);
  breakpoint(cond);  // stop here in debug mode
  ldr_string(r0, msg, cond);
  bl(L, cond);
#endif // END_CONVERT_TO_T2
}

void SourceMacros::verify_gp_register(Register tmp) {
  if (GenerateDebugAssembly) {
    // make sure gp holds the right value
    ldr_gp_base(tmp, tmp);
    cmp(tmp, (gp));
    breakpoint(ne);
  }
}

void SourceMacros::verify_cpool_register(Register tmp) {
  breakpoint();
#if 0 // BEGIN_CONVERT_TO_T2
  if (GenerateDebugAssembly) {
    get_cpool(tmp);
    add(tmp, tmp, imm(ConstantPool::base_offset()));
    cmp(tmp, reg(cpool));
    ldr(tmp,  imm_index(fp, JavaFrame::cpool_offset()), eq);
    cmp(tmp, reg(cpool),                                eq);
    breakpoint(ne);
  }
#endif // END_CONVERT_TO_T2
}

void SourceMacros::debug_on_variable(const char *var, Condition cond, 
                                     Register tmp) {
  breakpoint();
#if 0 // BEGIN_CONVERT_TO_T2
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
#endif // END_CONVERT_TO_T2
}

void SourceMacros::zero_region(Register item, Register size,
                               Register tmpA, Register tmpB,
                               int header_size,
                               bool size_includes_header,
                               bool size_is_in_bytes) {
  // Note that "item" is preserved, but "size" is bashed
  Label loop;
  Register ptr = tmpA;
  Register zero_reg = tmpB;

  int divisor = size_is_in_bytes ? 1 : BytesPerWord;
  int offset = BytesPerWord + (size_includes_header ? header_size : 0);
  GUARANTEE((offset % divisor) == 0, "Bad header size");

  add_imm(ptr, item, header_size);
  sub(zero_reg, zero_reg, zero_reg);
  sub_imm(size, size, offset/divisor, set_CC);
bind(loop);
  comment("r%d = the next word to zero", ptr);
  comment("r%d = 0", zero_reg);
  comment("r%d = the_number_of_%ss_still_to_zero - %d", size,
          (size_is_in_bytes ? "byte" : "word"),
          BytesPerWord/divisor);
  comment("condition codes reflect the (signed) value in r%d", size);
  it(ge); 
  Macros::str(zero_reg, ptr, PostIndex(BytesPerWord));
  it(gt); 
  Macros::str(zero_reg, ptr, PostIndex(BytesPerWord));
  sub_imm(size, size, 2 * BytesPerWord/divisor, set_CC);
  b(loop, ge);
}

void SourceMacros::verify_tag(Register reg, Tag tag) {
}


static const char* tag_name(Tag tag) {
  SHOULD_NOT_REACH_HERE();
#if 0 // BEGIN_CONVERT_TO_T2
  switch (tag) {
    case obj_tag   : return "obj";
    case int_tag   : return "int";
    case long_tag  : return "long";
    case float_tag : return "float";
    case double_tag: return "double";
    case ret_tag   : return "ret";
  }
  SHOULD_NOT_REACH_HERE();
#endif // END_CONVERT_TO_T2
  return NULL;
}

void SourceMacros::ldrb_at_bcp(Register reg, int offset) {
  comment("load unsigned byte at bcp + %d", offset);
  ldrb_imm12_w(reg, imm_index(bcp, offset));
}

void SourceMacros::ldrh_at_bcp_A(Register reg, Register tmp, int offset) {
  comment("load unsigned 16bit word at bcp + %d", offset);
  ldrb_imm12_w(reg, bcp, offset);
  ldrb_imm12_w(tmp, bcp, offset + 1);
}

void SourceMacros::ldrh_at_bcp_B(Register reg, Register tmp, int offset) {
  orr_w(reg, tmp, reg, lsl_shift, BitsPerByte);
}


void SourceMacros::get_local_addr(Register result, Register index) {
  eol_comment("address of local #r%d", index);
  if (JavaStackDirection < 0) {
    sub(result, locals, imm_shift(index, lsl, LogBytesPerStackElement));
  } else {
    add(result, locals, imm_shift(index, lsl, LogBytesPerStackElement));
  }
}

void SourceMacros::str_local_at(Register value, Register index, Register tmp) {
  get_local_addr(tmp, index);
  str_imm12_w(value, tmp, imm12(0));
}

void SourceMacros::ldr_local_at(Register value, Register index, Register tmp) {
  get_local_addr(tmp, index);
  ldr_imm12_w(value, tmp, imm12(0));
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
    ldr_w(dst, base, index, scale_shift);
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldr(dst, imm_index(dst, offset_12));
  }
}

void SourceMacros::ldrb_indexed(Register dst, Register base, Register index,
                                ScaleFactor scale_shift, int offset_12) {
  if (offset_12 == 0) {
    ldrb_w(dst, base, index, scale_shift);
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldrb(dst, imm_index(dst, offset_12));
  }
}

void SourceMacros::ldrh_indexed(Register dst, Register base, Register index,
                                ScaleFactor scale_shift, int offset_8) {
  if (offset_8 == 0 && scale_shift == times_1) {
    ldrh(dst, base, index);
  } else {
    add(dst, base, imm_shift(index, lsl, scale_shift));
    ldrh(dst, dst, offset_8);
  }
}

// dst = _class_index_top[index]
void SourceMacros::ldr_class_from_index_and_base(Register dst, Register index,
                                                 Register base,
                                                 Condition cond) {
  it(cond);
  ldr_w(dst, base, index, 2);
}

void SourceMacros::prefetch(int step) {
  GUARANTEE(step >= 0, "stepping backwards in bytecodes - are you sure?");
  eol_comment("prefetch next bytecode");
  // We could use pre_indexed for both, but it looks silly.
  ldrb(bcode, bcp, PreIndex(step));
}

void SourceMacros::prefetch(Register step) {
  eol_comment("prefetch next bytecode");
  add(bcp, bcp, step);
  ldrb_imm12_w(bcode, bcp, 0);
}

void SourceMacros::set_stack_state_to(State state, Condition cond) {
  switch (state) {
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
        it(cond);
        if (JavaStackDirection > 0) {
          sub_imm12_w(jsp, jsp, imm12(BytesPerWord), no_CC);
        } else {
          add_imm12_w(jsp, jsp, imm12(BytesPerWord), no_CC);
        }
      }
      break;

    case tos_on_stack:
      comment("Change state to tos_on_stack");
      if (!ENABLE_FULL_STACK) {
        eol_comment("adjust stack pointer");
        it(cond);
        if (JavaStackDirection > 0) {
          sub_imm12_w(jsp, jsp, imm12(BytesPerWord), no_CC);
        } else {
          add_imm12_w(jsp, jsp, imm12(BytesPerWord), no_CC);
        }
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
      comment("Restore stack stack from tos_in_regs");
      if (!ENABLE_FULL_STACK) {
        eol_comment("Adjust the stack");
        it(cond);
        add_imm(jsp, jsp, JavaStackDirection * BytesPerWord);
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
        it(cond);
        add_imm(jsp, jsp, JavaStackDirection * BytesPerWord);
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
      mov(target, r0);
    } else {
      pop(target);
    }
  }
  if (count_1 != 0) {
    comment("load other argument");
  }
  for (i = 0; i < count_1; i++) {
    Register target = JavaStackDirection > 0
                            ? as_register(start_1 + count_1 - i - 1)
                            : as_register(start_1 + i);
    pop(target);
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
      int this_tag =  basic_type2tag(kind) << (count_r - i - 1);
      if (ENABLE_TOS_CACHING && i == 0) {
        mov(r0, target);
      } else {
        push(target, al, mode);
      }
    }
  }
}

void SourceMacros::dispatch(State state) {
   comment("dispatch");
   eol_comment("get implementation of bytecode");
   sub(bcode, gp, bcode, lsl_shift, 2);
   ldr(bcode, bcode);
   restore_stack_state_from(state);
#if ENABLE_CPU_VARIANT
   CPUVariantSupport cpu_variant_support(stream());
   cpu_variant_support.jmp(bcode);
#else
   eol_comment("go to next bytecode");
   mov(pc, bcode);
#endif
}

void SourceMacros::dispatch(int count,
                            RegisterSet result1, RegisterSet result2,
                            RegisterSet result3, RegisterSet result4,
                            RegisterSet result5, RegisterSet result6) {
   comment("dispatch");
   eol_comment("get implementation of bytecode");
   sub(bcode, gp, bcode, lsl_shift, 2);
   ldr(bcode, bcode);
   push_results(count, result1, result2, result3, result4, result5, result6);
#if ENABLE_CPU_VARIANT
   CPUVariantSupport cpu_variant_support(stream());
   cpu_variant_support.jmp(bcode);
#else
   eol_comment("go to next bytecode");
   mov(pc, reg(bcode));
#endif
}

void SourceMacros::push_results(int count,
                                RegisterSet result1, RegisterSet result2,
                                RegisterSet result3, RegisterSet result4,
                                RegisterSet result5, RegisterSet result6) {
  StackTypeMode mode = ENABLE_TOS_CACHING ? full : empty;
  comment("Exit: Count = %d", count);
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
      if (tos_val != low_reg) {
        mov(tos_val, low_reg);
      }
    }
  }
}

void SourceMacros::get_method_parameter_size(Register result, Register method)
{
  comment("get parameter size in words");
  ldrh(result, method, Method::method_attributes_offset());
  int num_bits = 32 - Method::RESULT_STORAGE_TYPE_SHIFT;
  eol_comment("mask off the return type bits");
  mov_w(result, result, lsl_shift, num_bits);

  eol_comment("%s = parameter size in words", reg_name(result));
  mov_w(result, result, lsr_shift, num_bits);
}

void SourceMacros::get_thread(Register thread) {
  get_current_thread(thread);
}

void SourceMacros::get_thread_handle(Register thread) {
  get_current_thread_addr(thread);
}

void SourceMacros::call_from_interpreter(Register routine, int offset) {
  GUARANTEE(routine != lr, "Bad register");
  if (offset != 0) {
    add_imm(routine, routine, offset);
  }

#if ENABLE_EMBEDDED_CALLINFO
  align(4);
  eol_comment("r7 points to 12 bytes after the start of this instruction");
  add_pc(r7, 8);
  add_imm(r7, 1);
  mov_hi(lr, r7);

  bx(routine);
  define_call_info();
#else
  blx(routine);
#endif // ENABLE_EMBEDDED_CALLINFO
}

void SourceMacros::call_from_interpreter(const char* routine,
                                         bool routine_fixes_lr,
                                         Register tmp) {
#if ENABLE_EMBEDDED_CALLINFO
  align(4); // only 4-bytes aligned callinfo can be read

  if (routine_fixes_lr) {
    bl(routine);
  } else {
    if (tmp != no_reg && find_gp_offset(routine) >= 0) {
      // IMPL_NOTE: this works around "branch out of range" problems for
      // assemblers that don't understand Thumb-2
      ldr_using_gp(tmp, routine);
      blx(tmp);
    } else {
      add_pc(r7, 4);
      mov_hi(lr, r7);
      b(routine);
    }
  }

  define_call_info();
#else
  if (tmp != no_reg && find_gp_offset(routine) >= 0) {
    // IMPL_NOTE: this works around "branch out of range" problems for
    // assemblers that don't understand Thumb-2
    ldr_using_gp(tmp, routine);
    blx(tmp);
  } else {
    bl(routine);
  }
#endif // ENABLE_EMBEDDED_CALLINFO
}

void SourceMacros::get_bottom(Register reg, Condition cond) {
  eol_comment("stack bottom pointer");
  it(cond);
  ldr(reg, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
}

void SourceMacros::get_method(Register reg, Condition cond) {
  eol_comment("current method");
  it(cond);
  ldr(reg, imm_index(fp, JavaFrame::method_offset()));
}

void SourceMacros::get_cpool(Register reg, bool method_in_r0) {
  comment("get constants pool");

  if (method_in_r0) {
    eol_comment("get access flags");
    ldrh(reg, r0, Method::access_flags_offset());
    eol_comment("compressed header?");
    tst(reg, JVM_ACC_HAS_COMPRESSED_HEADER);
    //IMPL_NOTE: 
    it(ne);       
    get_rom_constant_pool_fast(reg);
    eol_comment("method constant pool");
    it(eq);
    ldr(reg, r0, Method::constants_offset());
  } else {
    // This is only used in debugging, so we don't care that this code
    // could be improved by having a temp register.    
    get_method(reg);
    eol_comment("get access flags");
    ldrh(reg, reg, Method::access_flags_offset());
    eol_comment("compressed header?");
    tst(reg, JVM_ACC_HAS_COMPRESSED_HEADER);
    get_method(reg,                                      eq);
    //IMPL_NOTE:
    get_rom_constant_pool_fast(reg,                      ne);
    eol_comment("method constant pool");
    it(eq);
    ldr(reg, imm_index(reg, Method::constants_offset()));
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
    ldr(bcp,  imm_index(fp, JavaFrame::bcp_store_offset()));
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
  breakpoint();
#if 0 // BEGIN_CONVERT_TO_T2
  comment("Create temporary frame for wtk profiler");
  if (param_size == -1) {
    comment("get parameter size in words");
    ldrh(tmp0, imm_index3(r0, Method::size_of_parameters_offset()));
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
  str(r0,    imm_index(jsp, offset + JavaFrame::method_offset()));
  str(fp,    imm_index(jsp, offset + JavaFrame::caller_fp_offset()));
  str(lr,    imm_index(jsp, offset + JavaFrame::return_address_offset()));


  comment("Set the Frame pointer");
  add_imm(fp,  jsp, offset);

  comment("set bcp");
  add(bcp, r0, imm(Method::base_offset()));

  interpreter_call_vm("jprof_record_method_transition", T_VOID);

  ldr(r0,    imm_index(fp, JavaFrame::method_offset()));
  ldr(lr,    imm_index(fp, JavaFrame::return_address_offset()));
  ldr(fp,    imm_index(fp, JavaFrame::caller_fp_offset()));
  sub_imm(jsp, jsp, JavaStackDirection * JavaFrame::frame_desc_size());
#endif // END_CONVERT_TO_T2
}      
      

void SourceMacros::check_timer_tick(State state) {
  Label done;
  eol_comment("Timer tick?");

  get_rt_timer_ticks(tmp5);
  cmp(tmp5, imm12(0));
  b(done, eq);

  if (state == tos_on_stack) {
    interpreter_call_vm("timer_tick", T_VOID);
  } else {
    restore_stack_state_from(state);
    set_stack_state_to(tos_on_stack);
    interpreter_call_vm("timer_tick", T_VOID);
    restore_stack_state_from(tos_on_stack);
    set_stack_state_to(state);
  }

  bind(done);
}

void SourceMacros::set_tag(Tag tag, Register reg) {
}

void SourceMacros::set_tags(Tag tag, Register reg1, Register reg2) {
}


void SourceMacros::set_return_type(BasicType type) {
}

// Push to Java stack
void SourceMacros::push(Register reg, Condition cond,
                        StackTypeMode type, WritebackMode mode) {
  it(cond);
  const int offset = JavaStackDirection * BytesPerWord;
  if (mode == writeback) {
    if (type == full) {
      Macros::str(reg, jsp, PreIndex(offset));
    } else {
      UNIMPLEMENTED();
    }
  } else {
    UNIMPLEMENTED();
  }

#if 0 // ARM code
  Address2 addr = (mode == writeback)  ?
                    ((type == full) ? imm_index(jsp, offset, pre_indexed)
                                    : imm_index(jsp, offset, post_indexed))
                  : ((type == full) ? imm_index(jsp, offset)
                                    : imm_index(jsp, 0));
  str(reg, addr, cond);
#endif
}

// Pop from Java stack
void SourceMacros::pop(Register reg, Condition cond,
                       StackTypeMode type, WritebackMode mode) {
  it(cond);
  const int offset = -JavaStackDirection * BytesPerWord;
  if (mode == writeback) {
    if (type == full) {
      Macros::ldr(reg, jsp, PostIndex(offset));
    } else {
      UNIMPLEMENTED();
    }
  } else {
    if (type == full) {
      ldr_imm12_w(reg, jsp);
    } else {
      UNIMPLEMENTED();
    }
  }
}

void SourceMacros::push(RegisterSet set, Condition cond,
                        StackTypeMode type, WritebackMode mode) {
  if (is_power_of_2((int)set)) {
    Register reg = as_register(exact_log2(set));
    push(reg, cond, type, mode);
  } else {
    UNIMPLEMENTED();
#if 0
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
#endif
  }
}

void SourceMacros::pop(RegisterSet set, Condition cond,
                       StackTypeMode type, WritebackMode mode) {
  if (is_power_of_2((int)set)) {
    Register reg = as_register(exact_log2(set));
    pop(reg, cond, type, mode);
  } else {
    UNIMPLEMENTED();
#if 0
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
#endif
  }
}

void SourceMacros::ldr_cp_entry_A(Register dst, Register addr,
                                  int addr_result, int size, int offset) {
  GUARANTEE(size == 1 || size == 2, "Bad parameter");
  if (size == 2) {
    comment("Get two bytes of constant pool entry");
    ldrb_imm12_w(addr, imm_index(bcp, offset));
    ldrb_imm12_w(dst,  imm_index(bcp, offset+1));
  } else {
    comment("Get byte of constant pool entry");
    ldrb_imm12_w(dst, imm_index(bcp, offset));
  }
}

void SourceMacros::ldr_cp_entry_B(Register dst, Register addr,
                                  int addr_result, int size, int offset) {
  if (size == 1) {
    if (addr_result == addr_cpool_index) {
      eol_comment("get address of cpool entry");
      add(addr, cpool, dst, lsl_shift, times_4);
    }
    eol_comment("load constant pool entry");
    //ldr(dst, add_index(cpool, dst, lsl, times_4));
    add(dst, cpool, dst, lsl_shift, times_4);
    ldr_imm12_w(dst, dst);
  } else {
    add(addr, cpool, imm_shift(addr, lsl, 2 + 8));
    //ldr(dst, add_index(addr, dst, lsl, times_4, pre_indexed));
    add(addr, addr, dst, lsl_shift, times_4);
    ldr_imm12_w(dst, addr);
  }
}

void SourceMacros::fast_c_call(Label name, Register tmp) {
  import(name);
  GUARANTEE(is_c_saved_register(tmp2), "Register sanity");
  mov(tmp2, reg(bcp));
#if ENABLE_ARM_V7
  // r12 is a scratch register, but we use it as cpool
  GUARANTEE(is_c_saved_register(tmp4), "Register sanity");
  mov(tmp4, reg(cpool));
  leavex();
#endif
  if (JavaStackDirection < 0 || sp != jsp) {
    // We can call this on the Java stack.  It doesn't matter
    bl(name);
  } else {
    mov(tmp, reg(jsp));
    get_primordial_sp(jsp);
    bl(name);
    mov(jsp, reg(tmp));
  }
  mov(bcp, reg(tmp2));
#if ENABLE_ARM_V7
  enterx();
  mov(cpool, reg(tmp4));
#endif
}

void SourceMacros::unlock_activation() {
  Register lock = tmp0;
  Register object = tmp1;
  Register flags = tmp2;
  Register link = tmp4;

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
  ldrh_imm12_w(flags, flags, Method::access_flags_offset());
  eol_comment("ACC_SYNCHRONIZED");
  tst(flags, JVM_ACC_SYNCHRONIZED);
  b(check_other_monitors, eq);

  call_from_interpreter("shared_unlock_synchronized_method", false, link);
  ldr(lock, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
  b(check_other_monitors);

bind(locked_monitor);
  ldr_label(r3, "illegal_monitor_state_exception");
  interpreter_call_shared_call_vm(T_ILLEGAL);

bind(check_other_monitors);
  comment("Check that all stack locks have been unlocked.");
  // lock is pointing at first word of stack lock
  Macros::ldr(object, lock, PreIndex(
                        JavaStackDirection < 0 ? StackLock::size() : 0));
  // Point at the object field of the oldest stack lock
  add_imm(flags, fp, JavaFrame::first_stack_lock_offset() + StackLock::size());

  Label unlocking_loop;
bind(unlocking_loop);
  cmp(object, imm12(zero));
  b(locked_monitor, ne);
  cmp(lock, reg(flags));
  if (GenerateDebugAssembly) {
    breakpoint(JavaStackDirection < 0 ? hi : lo);
  }
  // Grab object, point at next object
  it(ne, THEN);
  Macros::ldr(object, lock, PreIndex(
                    -JavaStackDirection* (StackLock::size() + BytesPerWord)));
  b(unlocking_loop);

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
    ldr_using_gp(pc, "shared_call_vm_oop");    
  } else {
    ldr_using_gp(pc, "shared_call_vm");    
  }
  if (GenerateDebugAssembly) {
    eol_comment("Should never return");
    breakpoint();
  }
}

#if ENABLE_INTERPRETATION_LOG
void SourceMacros::update_interpretation_log() {
#if 0 // BEGIN_CONVERT_TO_T2
  // _interpretation_log[] is a circular buffer that stores
  // the most recently executed interpreted methods.
  comment("Save this interpreted method into interpretation log");
  get_interpretation_log_idx(tmp1);
  get_interpretation_log_addr(tmp0);
  str(r0, add_index(tmp0, tmp1));
  add(tmp1, tmp1, imm(sizeof(int)));
  andr(tmp1, tmp1, imm(INTERP_LOG_MASK));
  set_interpretation_log_idx(tmp1);
#endif // END_CONVERT_TO_T2
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
      mov(r0, A);
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
    push(tos);
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

void SourceMacros::invoke_method(Register method, Register entry,
                                 Register tmp, int prefetch_size,
                                 char *deoptimization_entry_name) {
  char buff[40];

  // tos_val must contain the method
  GUARANTEE(method == tos_val, "sanity");

  Label labels[5];

  ldrh_imm12_w(tmp, method, Method::method_attributes_offset());
  eol_comment("%s = size of return type in words", reg_name(tmp));
  mov_w(tmp, tmp, lsr_shift, Method::RESULT_STORAGE_TYPE_SHIFT);

  // IMPL_NOTE: change this to a TBB instruction
  cmp(tmp, imm12(0)); b(labels[0], eq);
  cmp(tmp, imm12(1)); b(labels[1], eq);
  cmp(tmp, imm12(2)); b(labels[2], eq);
  cmp(tmp, imm12(3)); b(labels[3], eq);
  b(labels[4]);

  for (int i=0; i<Method::NUMBER_OF_RESULT_STORAGE_TYPES; i++) {
    generate_call(entry, labels[i], i, prefetch_size, 
      deoptimization_entry_name);
  }                
}

void SourceMacros::swap_mask(Register msk) {
  UNIMPLEMENTED();
#if 0 // BEGIN_CONVERT_TO_T2
  // From: ARM Architecture Reference Manual, 2nd Ed.
  //       by David Seal, Addison-Wesley, Sect. 9.1.4
  eol_comment("compute swap mask");
  mov_imm(msk, 0xff00ff);
#endif // END_CONVERT_TO_T2
}

void SourceMacros::swap_bytes(Register res, Register tmp, Register msk) {
  UNIMPLEMENTED();
#if 0 // BEGIN_CONVERT_TO_T2
  // From: ARM Architecture Reference Manual, 2nd Ed.
  //       by David Seal, Addison-Wesley, Sect. 9.1.4
  GUARANTEE(res != tmp && res != msk && tmp != msk, "registers must be different");
  comment("swap bytes");                     // res = a  , b  , c  , d
  andr(tmp, res, reg(msk));                  // tmp = 0  , b  , 0  , d
  andr(res, msk, imm_shift(res, ror, 24));   // res = 0  , c  , 0  , a
  orr(res, res, imm_shift(tmp, ror,  8));    // res = d  , c  , b  , a
#endif // END_CONVERT_TO_T2
}

#if ENABLE_ISOLATES
void SourceMacros::get_mirror_from_clinit_list(Register tm, Register klass,
                                               Register temp) {
  breakpoint();
#if 0 // BEGIN_CONVERT_TO_T2
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
#endif // END_CONVERT_TO_T2
}
#endif

void SourceMacros::emit_raw(juint instr) {
  stream()->print("\t");
  jushort hw2 = (jushort) (instr >> 16);
  stream()->print(".short\t0x%04x\n\t.short\t0x%04x\n",
                  instr & 0xffff, hw2);
}

#endif // PRODUCT

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
