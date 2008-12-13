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

#ifndef PRODUCT
#include "incls/_SourceMacros_i386.cpp.incl"

void SourceMacros::load_unsigned_byte(const Register dst, const Register src) {
  movzxb(dst, src);
}

void SourceMacros::load_unsigned_byte(const Register dst, const Address& src) {
  movzxb(dst, src);
}

void SourceMacros::load_unsigned_word(const Register dst, const Register src) {
  movzxw(dst, src);
}

void SourceMacros::load_unsigned_word(const Register dst, const Address& src) {
  movzxw(dst, src);
}

void SourceMacros::load_signed_byte(const Register dst, const Register src) {
  movsxb(dst, src);
}

void SourceMacros::load_signed_byte(const Register dst, const Address& src) {
  movsxb(dst, src);
}

void SourceMacros::load_signed_word(const Register dst, const Register src) {
  movsxw(dst, src);
}

void SourceMacros::load_signed_word(const Register dst, const Address& src) {
  movsxw(dst, src);
}

void SourceMacros::increment(const Register reg, const int value) {
  if (value <  0) { decrement(reg, -value)     ; return; }
  if (value == 0) {                            ; return; }
  if (value == 1) { incl(reg)                  ; return; }
  /* else */      { addl(reg, Constant(value)) ; return; }
}

void SourceMacros::decrement(const Register reg, const int value) {
  if (value <  0) { increment(reg, -value); return; }
  if (value == 0) {                       ; return; }
  if (value == 1) { decl(reg)             ; return; }
  /* else */      { subl(reg, Constant(value)); return; }
}

void SourceMacros::corrected_idivl(const Register reg) {
  // Full implementation of Java idiv and irem; checks for
  // special case as described in JVM spec., p.243 & p.271.
  //
  //         normal case                           special case
  //
  // input : eax: dividend                         min_int
  //         reg: divisor   (may not be eax/edx)   -1
  //
  // output: eax: quotient  (= eax idiv reg)       min_int
  //         edx: remainder (= eax irem reg)       0
  //
  GUARANTEE(reg != eax && reg != edx, "reg cannot be eax or edx register");
  const int min_int = 0x80000000;
  Label normal_case, special_case;

  // check for special case
  cmpl(eax, Constant(min_int));
  jcc(not_equal, Constant(normal_case));
  xorl(edx, edx); // prepare edx for possible special case (where remainder = 0)
  cmpl(reg, Constant(-1));
  jcc(equal, Constant(special_case));

  // handle normal case
  bind(normal_case);
  cdql();
  idivl(reg);

  // normal and special case exit
  bind(special_case);
}

void SourceMacros::fremp() {
  Label L;

  fxch();

  bind(L);
  fprem();
  fwait();
  fnstsw(eax);
  sahf();
  jcc(parity, Constant(L));
}

void SourceMacros::fcmp() {
  fcompp();
  comment("Convert FPU condition into eflags condition via eax");
  fwait();
  fnstsw(eax);
  sahf();
}

void SourceMacros::fcmp2int(const Register dst, bool unordered_is_less) {
  GUARANTEE(dst != eax, "Cannot use eax as temporary register since fcmp() stores in eax");

  Label L;
  fcmp();
  if (unordered_is_less) {
    movl(dst, Constant(-1));
    jcc(parity, Constant(L));
    jcc(below,  Constant(L));
    movl(dst, Constant(0));
    jcc(equal,  Constant(L));
    incl(dst);
  } else {
    movl(dst, Constant(1));
    jcc(parity, Constant(L));
    jcc(above,  Constant(L));
    movl(dst, Constant(0));
    jcc(equal,  Constant(L));
    decl(dst);
  }
  bind(L);
}

void SourceMacros::verify_tag(const Register src, const int tag) {
  if (GenerateDebugAssembly && TaggedJavaStack) {
    Label foo;
    testl(src, Constant(tag));
    jcc(not_zero, Constant(foo));
    int3();
    jmp(Constant("interpreter_throw_InternalStackTagException"));
    bind(foo);
  }
}

void SourceMacros::verify_tag(const Address& src, const int tag) {
  if (GenerateDebugAssembly && TaggedJavaStack) {
    Label foo;
    testl(src, Constant(tag));
    jcc(not_zero, Constant(foo));
    int3();
    jmp(Constant("interpreter_throw_InternalStackTagException"));
    bind(foo);
  }
}

void SourceMacros::push_int(const Register src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(int_tag));
  }
}

void SourceMacros::push_int(const Address& src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(int_tag));
  }
}

void SourceMacros::push_int(int imm32) {
  pushl(Constant(imm32));
  if (TaggedJavaStack) {
    pushl(Constant(int_tag));
  }
}

void SourceMacros::push_long(const Register low, const Register high) {
  pushl(high);
  if (TaggedJavaStack) {
    pushl(Constant(long_tag));
  }
  pushl(low);
  if (TaggedJavaStack) {
    pushl(Constant(long2_tag));
  }
}

void SourceMacros::push_long(const Address& low, const Address& high) {
  pushl(high);
  if (TaggedJavaStack) {
    pushl(Constant(long_tag));
  }
  pushl(low);
  if (TaggedJavaStack) {
    pushl(Constant(long2_tag));
  }
}

void SourceMacros::push_long(const jlong imm64) {
  pushl(Constant(msw(imm64)));
  if (TaggedJavaStack) {
    pushl(Constant(long_tag));
  }
  pushl(Constant(lsw(imm64)));
  if (TaggedJavaStack) {
    pushl(Constant(long2_tag));
  }
}

void SourceMacros::push_float(const Register src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(float_tag));
  }
}

void SourceMacros::push_float(const Address& src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(float_tag));
  }
}

void SourceMacros::push_float(const jfloat imm32) {
  union {
    jfloat a;
    jint   b;
  } u;
  u.a = imm32;
  pushl(Constant(u.b));
  if (TaggedJavaStack) {
    pushl(Constant(float_tag));
  }
}

void SourceMacros::push_double(const Register low, const Register high) {
  pushl(high);
  if (TaggedJavaStack) {
    pushl(Constant(double_tag));
  }
  pushl(low);
  if (TaggedJavaStack) {
    pushl(Constant(double2_tag));
  }
}

void SourceMacros::push_double(const Address& low, const Address& high) {
  pushl(high);
  if (TaggedJavaStack) {
    pushl(Constant(double_tag));
  }
  pushl(low);
  if (TaggedJavaStack) {
    pushl(Constant(double2_tag));
  }
}

void SourceMacros::push_double(const jdouble imm64) {
  union {
    jdouble a;
    jlong   b;
  } u;
  u.a = imm64;
  pushl(Constant(msw(u.b)));
  if (TaggedJavaStack) {
    pushl(Constant(double_tag));
  }
  pushl(Constant(lsw(u.b)));
  if (TaggedJavaStack) {
    pushl(Constant(double2_tag));
  }
}

void SourceMacros::push_obj(const Register src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(obj_tag));
  }
}

void SourceMacros::push_obj(const Address& src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(obj_tag));
  }
}

void SourceMacros::push_ret(const Register src) {
  pushl(src);
  if (TaggedJavaStack) {
    pushl(Constant(ret_tag));
  }
}

void SourceMacros::push_null_obj() {
  pushl(Constant(0));
  if (TaggedJavaStack) {
    pushl(Constant(obj_tag));
  }
}

void SourceMacros::pop_int(const Register dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, int_tag);
  }
  popl(dst);
}

void SourceMacros::pop_int(const Address& dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, int_tag);
  }
  popl(dst);
}

void SourceMacros::pop_long(const Register low, const Register high) {
  if (TaggedJavaStack) {
    popl(low);
    verify_tag(low, long2_tag);
  }
  popl(low);
  if (TaggedJavaStack) {
    popl(high);
    verify_tag(high, long_tag);
  }
  popl(high);
}

void SourceMacros::pop_long(const Address& low, const Address& high, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, long2_tag);
  }
  popl(low);
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, long_tag);
  }
  popl(high);
}

void SourceMacros::pop_float(const Register dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, float_tag);
  }
  popl(dst);
}

void SourceMacros::pop_float(const Address& dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, float_tag);
  }
  popl(dst);
}

void SourceMacros::pop_double(const Register low, const Register high) {
  if (TaggedJavaStack) {
    popl(low);
    verify_tag(low, double2_tag);
  }
  popl(low);
  if (TaggedJavaStack) {
    popl(high);
    verify_tag(high, double_tag);
  }
  popl(high);
}

void SourceMacros::pop_double(const Address& low, const Address& high, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, double2_tag);
  }
  popl(low);
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, double_tag);
  }
  popl(high);
}

void SourceMacros::pop_obj(const Register dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, obj_tag);
  }
  popl(dst);
}

void SourceMacros::pop_obj(const Address& dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, obj_tag);
  }
  popl(dst);
}

void SourceMacros::pop_obj_ret(const Register dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, obj_tag | ret_tag);
  }
  popl(dst);
}

void SourceMacros::pop_obj_ret(const Address& dst, const Register tag) {
  if (TaggedJavaStack) {
    popl(tag);
    verify_tag(tag, obj_tag | ret_tag);
  }
  popl(dst);
}

void SourceMacros::push_local_int(const int index) {
  if (TaggedJavaStack) {
    verify_tag(local_tag_address (index), int_tag);
  }
  push_int(local_address(index));
}

void SourceMacros::push_local_int(const Register index) {
  negl(index);
  if (TaggedJavaStack) {
    verify_tag(local_tag_address (index), int_tag);
  }
  push_int(local_address (index));
}

void SourceMacros::push_local_long(const int index) {
  if (TaggedJavaStack) {
    verify_tag(local_tag_address_high(index), long_tag);
    verify_tag(local_tag_address_low(index), long2_tag);
  }
  push_long(local_address_low(index), local_address_high(index));
}

void SourceMacros::push_local_long(const Register index) {
  negl(index);
  if (TaggedJavaStack) {
    verify_tag(local_tag_address_high(index), long_tag);
    verify_tag(local_tag_address_low(index), long2_tag);
  }
  push_long(local_address_low(index), local_address_high(index));
}

void SourceMacros::push_local_float(const int index) {
  if (TaggedJavaStack) {
    verify_tag(local_tag_address(index), float_tag);
  }
  push_float(local_address (index));
}

void SourceMacros::push_local_float(const Register index) {
  negl(index);
  if (TaggedJavaStack) {
    verify_tag(local_tag_address(index), float_tag);
  }
  push_float(local_address (index));
}

void SourceMacros::push_local_double(const int index) {
  if (TaggedJavaStack) {
    verify_tag(local_tag_address_high(index), double_tag);
    verify_tag(local_tag_address_low(index), double2_tag);
  }
  push_double(local_address_low(index), local_address_high(index));
}

void SourceMacros::push_local_double(const Register index) {
  negl(index);
  if (TaggedJavaStack) {
    verify_tag(local_tag_address_high(index), double_tag);
    verify_tag(local_tag_address_low(index), double2_tag);
  }
  push_double(local_address_low(index), local_address_high(index));
}

void SourceMacros::push_local_obj(const int index){
  if (TaggedJavaStack) {
    verify_tag(local_tag_address(index), obj_tag);
  }
  push_obj(local_address(index));
}

void SourceMacros::push_local_obj(const Register index) {
  negl(index);
  if (TaggedJavaStack) {
    verify_tag(local_tag_address(index), obj_tag);
  }
  push_obj(local_address(index));
}

void SourceMacros::pop_local_int(const int index, const Register tag) {
  pop_int(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address (index), Constant(int_tag));
  }
}

void SourceMacros::pop_local_int(const Register index, const Register tag) {
  negl(index);
  pop_int(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address (index), Constant(int_tag));
  }
}

void SourceMacros::pop_local_long(const int index, const Register tag) {
  pop_long(local_address_low(index), local_address_high(index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address_high(index), Constant(long_tag));
    movl(local_tag_address_low(index), Constant(long2_tag));
  }
}

void SourceMacros::pop_local_long(const Register index, const Register tag) {
  negl(index);
  pop_long(local_address_low(index), local_address_high(index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address_high(index), Constant(long_tag));
    movl(local_tag_address_low(index), Constant(long2_tag));
  }
}

void SourceMacros::pop_local_float(const int index, const Register tag) {
  pop_float(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), Constant(float_tag));
  }
}

void SourceMacros::pop_local_float(const Register index, const Register tag) {
  negl(index);
  pop_float(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), Constant(float_tag));
  }
}

void SourceMacros::pop_local_double(const int index, const Register tag) {
  pop_double(local_address_low(index), local_address_high(index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address_high(index), Constant(double_tag));
    movl(local_tag_address_low(index), Constant(double2_tag));
  }
}

void SourceMacros::pop_local_double(const Register index, const Register tag) {
  negl(index);
  pop_double(local_address_low(index), local_address_high(index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address_high(index), Constant(double_tag));
    movl(local_tag_address_low(index), Constant(double2_tag));
  }
}

void SourceMacros::pop_local_obj(const int index, const Register tag) {
  pop_obj(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), Constant(obj_tag));
  }
}

void SourceMacros::pop_local_obj(const Register index, const Register tag) {
  negl(index);
  pop_obj(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), Constant(obj_tag));
  }
}

void SourceMacros::pop_local_obj_ret(const int index, const Register tag) {
  pop_obj_ret(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), tag);
  }
}

void SourceMacros::pop_local_obj_ret(const Register index, const Register tag) {
  negl(index);
  pop_obj_ret(local_address (index), tag);
  if (TaggedJavaStack) {
    movl(local_tag_address(index), tag);
  }
}

void SourceMacros::pop_to_fpu_stack(Tag tag, int& offset) {
  void (SourceAssembler::*inst)(const Address&);
  switch(tag) {
    case double_tag: case long_tag:
      inst = (tag == long_tag) ? &SourceAssembler::fild_d
                               : &SourceAssembler::fld_d;
      if (TaggedJavaStack) {
        verify_tag(stackvar_tag_address_high(offset), tag);
        verify_tag(stackvar_tag_address_low (offset), 2 * tag);
        pushl(stackvar_address_high(offset));
        offset += 4;
        pushl(stackvar_address_low (offset));
        offset += 4;
        (this->*inst)(Address(esp));
      } else {
        (this->*inst)(Address(esp, Constant(offset)));
      }

      offset += 2 * BytesPerStackElement;
      break;

    case float_tag: case int_tag:
      inst = (tag == int_tag) ? &SourceAssembler::fild_f
                              : &SourceAssembler::fld_f;
      if (TaggedJavaStack) {
        verify_tag(stackvar_tag_address(offset), tag);
      }
      (this->*inst)(stackvar_address(offset));
      offset += BytesPerStackElement;      // Argument on stack is now trash
      break;

    default:
      SHOULD_NOT_REACH_HERE();
  }
}

void SourceMacros::push_from_fpu_stack(Tag tag, int offset, bool set_tags) {
  void (SourceAssembler::*inst)(const Address&);
  switch(tag) {
  case float_tag: case int_tag:
    inst = (tag == int_tag) ? &SourceAssembler::fistp_f
                            : &SourceAssembler::fstp_f;
    // Set offset to be BytesPerStackElement, the exact amount of space for a
    // float/int
    decrement(esp, BytesPerStackElement - offset);
    (this->*inst)(stackvar_address());

    if (TaggedJavaStack) {
      if (set_tags) {
        movl(stackvar_tag_address(), Constant(tag));
      } else {
        verify_tag(stackvar_tag_address(), tag);
      }

    }
    break;
  case double_tag:
  case long_tag:
    inst = (tag == long_tag) ? &SourceAssembler::fistp_d
                             : &SourceAssembler::fstp_d;
    // Space for two words, plus a double/long
    if (TaggedJavaStack) {
      decrement(esp, 24 - offset);
      // Put result at the top of the stack
      (this->*inst)(Address(esp));
      // Move to the appropriate location.
      popl(stackvar_address_low(4));
      popl(stackvar_address_high(0));
      if (TaggedJavaStack) {
        if (set_tags) {
          movl(stackvar_tag_address_high(), Constant(tag));
          movl(stackvar_tag_address_low(),  Constant(2 * tag));
        } else {
          verify_tag(stackvar_tag_address_high(), tag);
          verify_tag(stackvar_tag_address_low(),  2 * tag);
        }
      }
    } else {
      decrement(esp, 2 * BytesPerStackElement - offset);
      (this->*inst)(Address(esp));
    }
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
}

// Pops off one item from the FPU stack, using space that's currently
// unused above the top of stack.
void SourceMacros::clear_one_from_fpu_stack(Tag tag, int offset) {
  void (SourceAssembler::*inst)(const Address&) = NULL;
  switch(tag) {
  case float_tag: case int_tag:
    inst = (tag == int_tag) ? &SourceAssembler::fistp_f
                            : &SourceAssembler::fstp_f;
    offset -= BytesPerStackElement;
    break;
  case double_tag: case long_tag:
    inst = (tag == long_tag) ? &SourceAssembler::fistp_d
                             : &SourceAssembler::fstp_d;
    if (TaggedJavaStack) {
      offset -= 24;
    } else {
      offset -= 2 * BytesPerStackElement;
    }
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
  decrement(esp, offset);
  (this->*inst)(Address(esp));
  increment(esp, offset);
}

void SourceMacros::increment_local_int(const Register index, const Register src) {
  negl(index);
  addl(local_address (index), src);
}

void SourceMacros::get_local_ret(const Register index, const Register dst) {
  negl(index);
  verify_tag(local_tag_address(index), ret_tag);
  movl(dst, local_address (index));
}

void SourceMacros::get_thread(const Register thread) {
  movl(thread, Address(Constant("_current_thread")));
}

void SourceMacros::get_thread_handle(const Register thread) {
  movl(thread, Constant("_current_thread"));
}
#if ENABLE_ISOLATES
void SourceMacros::get_current_task(const Register &m){
    movl(m, Address(Constant("_current_task")));
}

void SourceMacros::get_task_mirror_list(const Register dst){
  movl(dst, Address(Constant("_mirror_list_base")));
}

void SourceMacros::load_task_mirror_from_list(const Register dst,
                        const Register class_id,
                        const Register tmp) {
  get_task_mirror_list(tmp);
  movl(dst, Address(tmp, class_id, times_4));
}

void SourceMacros::cib_with_marker(const Register task_mirror,
                                   Label class_is_initialized){
  Label need_init;
  comment("class initialization barrier");
  cmpl(task_mirror, Address(Constant("_task_class_init_marker")));
  jcc(not_equal, Constant(class_is_initialized));
bind(need_init);
}

void SourceMacros::get_task_class_init_marker(const Register &m){
    movl(m, Address(Constant("_task_class_init_marker")));
}

void SourceMacros::get_clinit_list(const Register clinit_list){
  get_current_task(clinit_list);
  movl(clinit_list, Address(clinit_list, Constant(Task::clinit_list_offset())));
}
void SourceMacros::get_mirror_from_clinit_list(const Register &tm, const Register &klass){
  Label end, begin;
  // Scan the clinit list for the task mirror holding the class's isolate-private representation
  get_clinit_list(tm);
  bind(begin); 
  cmpl(klass, Address(tm, Constant(TaskMirror::containing_class_offset())));
  jcc(equal, Constant(end));
  movl(tm,Address(tm, Constant(TaskMirror::next_in_clinit_list_offset())));
  // IMPL_NOTE: HERE GUARD AGAINST END OF LIST ?
  jmp(Constant(begin));
  bind(end);
}

#else // ENABLE_ISOLATES

void SourceMacros::initialize_class_when_needed(const Register dst,
                                                const Register tmp1,
                                                const Register tmp2,
                                                Label restart) {
  GUARANTEE(!ENABLE_ISOLATES, "Not supposed to be used in MVM");
  GUARANTEE(dst != tmp1 && dst != tmp2 && tmp1 != tmp2, "Sanity");
  GUARANTEE(dst == edx, "edx is passed as an argument in shared_call_vm");

  const Register mirror = tmp1;
  const Register thread = tmp2;
  Label class_is_initialized, do_initialize;

  comment("Load Java mirror for the class");
  movl(mirror, Address(dst, Constant(JavaClass::java_mirror_offset())));

  comment("Check if the class is already initialized");
  testl(Address(mirror, Constant(JavaClassObj::status_offset())),
        Constant(JavaClassObj::INITIALIZED));
  jcc(not_zero, Constant(class_is_initialized));
  
  comment("Check if the class is being initialized by the current thread");
  testl(Address(mirror, Constant(JavaClassObj::status_offset())),
        Constant(JavaClassObj::IN_PROGRESS));
  jcc(zero, Constant(do_initialize));
  get_thread(thread);
  movl(mirror, Address(mirror, Constant(JavaClassObj::thread_offset())));
  cmpl(mirror, Address(thread, Constant(Thread::thread_obj_offset())));
  jcc(equal, Constant(class_is_initialized));

  bind(do_initialize);
  comment("Call VM runtime to initialize class");
  interpreter_call_vm(Constant("initialize_class"), T_VOID);
  jmp(Constant(restart));
  bind(class_is_initialized);
}

#endif // ENABLE_ISOLATES

void SourceMacros::call_from_interpreter(const Register routine, int offset) {
  addl(routine, Constant(offset));
  call(routine);
  define_call_info();
}

void SourceMacros::call_from_interpreter(const Constant& routine) {
  call(routine);
  define_call_info();
}

void SourceMacros::save_interpreter_state() {
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);
}

void SourceMacros::restore_interpreter_state() {
  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));

  comment("Restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));
}

void SourceMacros::increment_bytecode_counter(Bytecodes::Code bc) {
  comment("Increment bytecode counter");
  addl(Address(Constant("interpreter_bytecode_counters", 8 * (int)bc)),
       Constant(1));
  adcl(Address(Constant("interpreter_bytecode_counters", 4 + 8 * (int)bc)),
       Constant(0));
}

void SourceMacros::increment_pair_counter(Bytecodes::Code bc) {
  comment("Increment pair counter");
  int base = Bytecodes::number_of_java_codes * 8 * (int) bc;

  movzxb(ebx, Address(esi, Constant(Bytecodes::length_for(bc))));
  addl(Address(no_reg, ebx, times_8,
               Constant("interpreter_pair_counters", base)),
       Constant(1));
  adcl(Address(no_reg, ebx, times_8,
               Constant("interpreter_pair_counters", base + 4)),
       Constant(0));
}

void SourceMacros::trace_native_call(const Register tmp) {
#if ENABLE_TTY_TRACE
  GUARANTEE(tmp == eax, "tmp register must be eax");

  Label done_trace;
  pushl(tmp);
  movl(tmp, Address(Constant("TraceNativeCalls")));
  testl(tmp, tmp);
  jcc(equal, Constant(done_trace));
  popl(tmp);
  movl(Address(Constant("_current_native_function")), tmp);

  pushl(tmp);
  movl(tmp, Constant("trace_native_call"));
  pushl(tmp);
  call(Constant("call_on_primordial_stack"));
  popl(tmp);

  bind(done_trace);
  popl(tmp);
#endif
}

void SourceMacros::wtk_profile_quick_call(int param_size) {
  if (!ENABLE_WTK_PROFILER) { 
    return;
  }
  if (param_size == -1) { 
    comment("Get the size of the parameters");
    load_unsigned_word(ecx, Address(ebx, Constant(
        Method::method_attributes_offset())));
    andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));
    comment("Compute the start of the parameters - the locals pointer");
    leal(edi, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4));
  } else { 
    leal(edi, Address(esp, Constant(param_size * BytesPerStackElement)));
  }
  
  comment("Build a pseudo frame.  Return address still on stack");
  comment("");
  comment("Setup frame pointer");
  pushl(ebp);
  movl(ebp, esp);

  comment("Push the method onto the stack");
  pushl(ebx);

  comment("Fake value for constant pool");
  pushl(Constant(0));

  comment("Push the locals pointer onto the stack");
  pushl(edi);

  comment("Bytecode pointer");
  leal(esi, Address(ebx, Constant(Method::base_offset())));
  pushl(esi);

  comment("Setup the stack bottom pointer");
  pushl(ebp);
  movl(Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())), esp);

  interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
    
  comment("Remove the frame");
  movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
  movl(ebp, Address(ebp, Constant(JavaFrame::caller_fp_offset())));
  // leave return address on the stack
  addl(esp, Constant(JavaFrame::frame_desc_size() - 4));  
}

void SourceMacros::interpreter_call_native(const Register native, 
                                           BasicType return_value_type) {
  GUARANTEE(native == eax, "The interpreter call vm stub expects the "
                           "register to be eax");

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  if (TraceNativeCalls) {
    trace_native_call(native);
  }

  comment("Call the shared call vm");
  call_shared_call_vm(return_value_type);

  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));

  comment("Restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));
}

void SourceMacros::interpreter_call_vm(const Constant& routine, BasicType return_value_type) {
  comment("Setup the pointer to the vm routine");
  movl(eax, routine);

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  comment("Call the shared call vm");
  call_shared_call_vm(return_value_type);

  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));

  comment("Restore locals pointer");
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));
}

void SourceMacros::interpreter_call_vm_redo(const Constant& routine) {
  comment("Setup the pointer to the vm routine");
  movl(eax, routine);

  comment("Notice the use of jump here");
  jmp(Constant("interpreter_call_vm_redo"));
}

void SourceMacros::check_timer_tick() {
  comment("Check timer tick");
#if ENABLE_PAGE_PROTECTION
  movl(Address(Constant("_protected_page", INTERPRETER_TIMER_TICK_SLOT)), eax);
#else
  cmpl(Address(Constant("_rt_timer_ticks")), Constant(0));
  jcc(not_equal, Constant("interpreter_timer_tick"));
#endif
}

void SourceMacros::dispatch_prologue(int step) {
  // Do nothing.
}

void SourceMacros::dispatch_epilogue(int step) {
  dispatch_next(step);
}

void SourceMacros::dispatch_next(int step) {
  comment("Load next bytecode (load before advancing esi to prevent AGI)");
  load_unsigned_byte(ebx, Address(esi, Constant(step)));

  if (step > 0) {
    comment("Advance bytecode pointer");
    increment(esi, step);
  }

  comment("Dispatch to next byte code");
  jmp(Address(no_reg, ebx, times_4, Constant("interpreter_dispatch_table")));
}

void SourceMacros::unlock_activation(bool is_unwinding) {
  Label done, unlocked, unlock, exception, method_exception;
  Label no_locks;

  GUARANTEE(!is_unwinding, "We don't do that anymore");

  comment("See if there are any stack locks");
  leal(eax, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  cmpl(Address(eax), eax);
  jcc(equal, Constant(no_locks));

  save_interpreter_state();

  movl(ebx, Address(ebp, Constant(JavaFrame::method_offset())));
  load_unsigned_word(ecx, Address(ebx,
                                  Constant(Method::access_flags_offset())));
  testl(ecx, Constant(JVM_ACC_SYNCHRONIZED));
  jcc(zero, Constant(unlocked));

  call_from_interpreter(Constant("shared_unlock_synchronized_method"));
  jmp(Constant(unlocked));


bind(exception);
  Label post_exception;
  comment("Found locked monitor that shouldn't have been");
  // Note, we can't use interpreter_call_vm since it saves and restores state.
  movl(eax, Constant("illegal_monitor_state_exception"));
  call_shared_call_vm(T_VOID);

bind(method_exception);

  // Check that all stack locks have been unlocked.
bind(unlocked);
  Label loop;
  movl(ecx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  leal(ebx, Address(ebp, Constant(JavaFrame::stack_bottom_pointer_offset())));
  // We know that there is at least one stack lock, or we wouldn't be here in
  // the first place!

bind(loop);
  cmpl(Address(ecx, Constant(StackLock::size())), Constant(0));
  jcc(not_equal, Constant(exception));
  increment(ecx, (4 + StackLock::size()));
  cmpl(ecx, ebx);
  jcc(not_equal, Constant(loop));

bind(done);

  restore_interpreter_state();

  bind(no_locks);
}

void SourceMacros::remove_activation(const Register return_address) {
  movl(esp, ebp);
  popl(ebp);
  popl(return_address);
  leal(esp, Address(edi, Constant(4)));
}

void SourceMacros::oop_write_barrier(const Register dst, const Register tmp,
                                     bool preserve_dst, bool check_heap_top){
  Label skip;
  if (check_heap_top) {
    // Currently romized Strings are stored outside of the heap, so
    // we must not do write barriers on their _klass field.
    //
    // IMPL_NOTE: This restriction applies on the monitorenter/monitorexit
    // byte codes only, but we're doing it for synchronized method
    // enter/exit as well. It's probably not worth optimizing ...
    comment("Oop write barrier (with heap limit checks)");
    cmpl(dst, Address(Constant("_heap_start")) );
    jcc(below, Constant(skip));

    cmpl(dst, Address(Constant("_old_generation_end")) );
    jcc(above_equal, Constant(skip));
  } else {
    comment("Oop write barrier (destination must be in heap)");

    if (GenerateDebugAssembly) {
      Label next1, next2;

      movl(tmp, Address(Constant("_heap_start")));
      cmpl(dst, tmp);
      jcc(above_equal, Constant(next1));
      comment("object below heap!");
      int3();

      bind(next1);

      movl(tmp, Address(Constant("_heap_top")));
      cmpl(dst, tmp);
      jcc(below, Constant(next2));
      comment("object above heap!");
      int3();

      bind(next2);
    }
  }

  movl(tmp, Address(Constant("_bitvector_base")));
  shrl(dst, Constant(LogBytesPerWord));
  btsl(Address(tmp), dst);
  if (preserve_dst) {
    shll(dst, Constant(LogBytesPerWord));
  }
  bind(skip);
}

void SourceMacros::load_class_from_list(const Register dst,
                                        const Register class_id,
                                        const Register tmp) {
  movl(tmp, Address(Constant("_class_list_base")));
  movl(dst, Address(tmp, class_id, times_4));
}

void SourceMacros::call_shared_call_vm(BasicType return_value_type) {
  if (stack_type_for(return_value_type) == T_OBJECT) {
    call_from_interpreter(Constant("shared_call_vm_oop"));
  } else {
    call_from_interpreter(Constant("shared_call_vm"));
  }
}

void SourceMacros::goto_shared_call_vm(BasicType return_value_type) {
  if (stack_type_for(return_value_type) == T_OBJECT) {
    jmp(Constant("shared_call_vm_oop"));
  } else {
    jmp(Constant("shared_call_vm"));
  }
}

#if ENABLE_INTERPRETATION_LOG
void SourceMacros::update_interpretation_log() {
  // _interpretation_log[] is a circular buffer that stores
  // the most recently executed interpreted methods.
  comment("Save this interpreted method into interpretation log");
  movl(eax, Address(Constant("_interpretation_log_idx")));
  movl(Address(eax, Constant("_interpretation_log")), ebx);
  leal(eax, Address(eax, Constant(4)) );
  andl(eax, Constant(INTERP_LOG_MASK));
  movl(Address(Constant("_interpretation_log_idx")), eax);
}
#endif

#endif
