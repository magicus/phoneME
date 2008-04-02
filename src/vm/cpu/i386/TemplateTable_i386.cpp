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
#include "incls/_TemplateTable_i386.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

// The following macro used to be the following function, but it
// didn't work with gcc 2.96 in RELEASE mode.
//
// SourceMacros::Address Template::bcp_address(int offset) {
//  return Address(esi, Constant(offset));
// }

#define bcp_address(offset) \
    Address(esi, Constant(offset))

void Template::null_check(Register object, bool tos_cached/*unused*/) {
  comment("Null check");
  testl(object, object);
  jcc(zero, Constant("interpreter_throw_NullPointerException"));
}

void Template::index_check(Register array, Register index, Register tmp) {
  // tmp is required for pure load/store architectures (ARM) - not used here
  null_check(array);
  comment("Check array index out of bounds");
  cmpl(index, Address(array, Constant(Array::length_offset())));
#if ENABLE_ISOLATES
  // FOR DEBUGGING ONLY
  Label out_of_here;
  jcc(below, Constant(out_of_here));
  //  int3();
  jcc(above_equal,
      Constant("interpreter_throw_ArrayIndexOutOfBoundsException"));
  bind(out_of_here);
#else
  jcc(above_equal,
      Constant("interpreter_throw_ArrayIndexOutOfBoundsException"));
#endif
}

void Template::type_check(Register array, Register object, Register index) {
  Label done, cache_hit;
  GUARANTEE(array != edx && object != edx && index != edx,
            "Register edx is used");
  comment("Array store type check");

  // Get the class of the object.
  testl(object, object);
  jcc(zero, Constant(done));
  movl(edx, Address(object));
  movl(edx, Address(edx));

  // Get the element class of the array.
  movl(edi, Address(array));
  movl(edi, Address(edi));
  movl(edi, Address(edi, Constant(ObjArrayClass::element_class_offset())));

  // Fast check against the subtype check caches.
  cmpl(edi, Address(edx, Constant(JavaClass::subtype_cache_1_offset())));
  jcc(equal, Constant(cache_hit));
  cmpl(edi, Address(edx, Constant(JavaClass::subtype_cache_2_offset())));
  jcc(equal, Constant(cache_hit));

  // Do the expensive type check.
  push_obj(array);
  push_int(index);
  push_obj(object);
  interpreter_call_vm(Constant("array_store_type_check"), T_VOID);
  pop_obj(object, object);
  pop_int(index, index);
  pop_obj(array, array);

  // Cache hit.
  bind(cache_hit);
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  bind(done);
}

void Template::branch(bool is_jsr, bool is_wide) {
  comment("Load jump offset in edx and update esi to point to destination");
  if (is_wide) {
    movl(edx, bcp_address(1));
    comment("Begin - i386 has no bswap instruction");
    xchg(dh, dl);
    ror(edx, Constant(16));
    xchg(dh, dl);
    comment("End - i386 has no bswap instruction");
  } else {
    load_unsigned_word(edx, bcp_address(1));
    xchg(dh, dl); // convert to little-endian
    load_signed_word(edx, edx);
  }
  GUARANTEE(!is_jsr, "jsr not supported by CLDC");
#if NOT_CURRENTLY_USED
  if (is_jsr) {
    comment("Get current method and put it in ecx");
    movl(ecx, Address(ebp, Constant(JavaFrame::method_offset())));
    comment("Compute return byte code index and put it on stack");
    leal(eax, bcp_address((is_wide ? 5 : 3) - Method::base_offset()));
    subl(eax, ecx);
    push_ret(eax);
  }
#endif
  addl(esi, edx);

#if ENABLE_COMPILER && ENABLE_INTERPRETATION_LOG
#if 0
  testl( edx, edx );

  Label skip;
  jcc( less_equal, Constant( skip ) );
  comment("Get current method and put it in ebx");
  movl( ebx, Address(ebp, Constant(JavaFrame::method_offset())));
  update_interpretation_log();
  bind( skip );
#endif
#endif
  dispatch_next(0);
}


void Template::fast_access_field(bool is_static, bool is_put, BasicType type,
                                 int param_bytes, bool extended, int offset) {
  if (is_static) {
    GUARANTEE(param_bytes == 2, "1-byte param not supported");
    Label restart;
    bind(restart);

    comment("Get constant pool of current method");
    movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

    comment("Get constant pool index");
    load_unsigned_word(eax, bcp_address(1));
    xchg(ah,al); // convert to little-endian

    comment("Get class_id from constant pool");
    load_unsigned_word(ebx, Address(ecx, eax, times_4));
        // moved loading of offset here to free up register ecx early [Laurent]
    comment("Get the field offset from constant pool entry");
    load_unsigned_word(eax, Address(ecx, eax, times_4, Constant(2)));

#if ENABLE_ISOLATES
    Label class_is_initialized;
    comment("Get TaskMirror from class_id");
    load_task_mirror_from_list(edx, ebx, edx); // preserve class id in ebx
    cib_with_marker(edx, class_is_initialized);
    // Note: edx holds the task mirror
    // Need to load here instance class in edx if barrier failed 
    // -- ebx still holds class id
    load_class_from_list(edx, ebx, edx);
    interpreter_call_vm(Constant("task_barrier"), T_OBJECT);
    comment("Task mirror should be available now");
    movl(edx, eax);
    comment("restore state");
    movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

    comment("Get constant pool index");
    load_unsigned_word(eax, bcp_address(1));
    xchg(ah,al); // convert to little-endian
    comment("Get the field offset from constant pool entry");
    load_unsigned_word(eax, Address(ecx, eax, times_4, Constant(2)));
    comment("Need more freakin' registers!");
    bind(class_is_initialized);
#else
    comment("Get JavaClass from class_id");
    load_class_from_list(edx, ebx, edx);
    if (extended) {   // i.e. with class initialization check
      initialize_class_when_needed(edx, ebx, ecx, restart);
    }
#endif
  } else {
    if (extended) {   // i.e. with aload_0
      switch(type) { 
        case T_OBJECT:   push_local_obj(0); break;
        case T_INT:      push_local_int(0); break;
        default:         SHOULD_NOT_REACH_HERE();
      }
    }
    if (param_bytes == 0) {
      GUARANTEE(type == T_INT || type == T_OBJECT, "assumptions");
      comment("Implicit offset = %d", offset);
      movl(eax, Constant(offset/BytesPerWord)); // Is multipled x4 again later
    } else {
      comment("Get the field offset");
      if (param_bytes == 1) {
        load_unsigned_byte(eax, bcp_address(1));
      } else if (param_bytes == 2) {
        load_unsigned_word(eax, bcp_address(1));
        if (!ENABLE_NATIVE_ORDER_REWRITING) {
          xchg(ah, al);
        }
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    }
  }

  if (is_put) {
    comment("Get value from the stack");
    switch (type) {
    case T_BYTE    : // Fall through
    case T_SHORT   : // Fall through
    case T_INT     : pop_int(ebx, ebx);         break;
    case T_LONG    : pop_long(ebx, ecx);        break;
    case T_FLOAT   : pop_float(ebx, ebx);       break;
    case T_DOUBLE  : pop_double(ebx, ecx);      break;
    case T_OBJECT  : pop_obj(ebx, ebx);         break;
    default        : SHOULD_NOT_REACH_HERE();
    }
  }

  if (!is_static) {
    comment("Get object from stack");
    pop_obj(edx, edx);
    null_check(edx);
  }
  // Only for objects word size or bigger
  ScaleFactor scale = is_static ? times_1 : times_4;

  if (is_put) {
    comment("Put value into field");
    comment("Value is in ebx:ecx, object stored into is in edx, field offset is in eax");
    switch (type) {
    case T_BYTE    : GUARANTEE(!is_static, "only non-static fields are packed");
                     movb(Address(edx, eax, times_1), ebx);    break;
    case T_SHORT   : GUARANTEE(!is_static, "only non-static fields are packed");
                     movw(Address(edx, eax, times_1), ebx);    break;
    case T_OBJECT  : leal(eax, Address(edx, eax, scale));
                     movl(Address(eax), ebx);
                     oop_write_barrier(eax, ecx);
                     break;
    case T_INT     : // Fall through
    case T_FLOAT   : movl(Address(edx, eax, scale), ebx);    break;
    case T_LONG    : // Fall through
    case T_DOUBLE  : movl(Address(edx, eax, scale), ebx);
                     movl(Address(edx, eax, scale, Constant(4)), ecx); break;
    default        : SHOULD_NOT_REACH_HERE();
    }
  } else {
    comment("Get the value from the field");
    comment("Object read from is in edx, field offset is in eax");
    switch (type) {
    case T_BYTE    : GUARANTEE(!is_static, "only non-static fields are packed");
                     movsxb(eax, Address(edx, eax, times_1));
                     push_int(eax);
                     break;
    case T_SHORT   : GUARANTEE(!is_static, "only non-static fields are packed");
                     movsxw(eax, Address(edx, eax, times_1));
                     push_int(eax);
                     break;
    case T_CHAR    : GUARANTEE(!is_static, "only non-static fields are packed");
                     movzxw(eax, Address(edx, eax, times_1));
                     push_int(eax);
                     break;
    case T_INT     : push_int(Address(edx, eax, scale));
                     break;
    case T_LONG    : push_long(Address(edx, eax, scale),
                               Address(edx, eax, scale, Constant(4)));
                     break;
    case T_FLOAT   : push_float(Address(edx, eax, scale));
                     break;
    case T_DOUBLE  : push_double(Address(edx, eax, scale),
                                 Address(edx, eax, scale, Constant(4)));
                     break;
    case T_OBJECT  : push_obj(Address(edx, eax, scale));
                     break;
    default        : SHOULD_NOT_REACH_HERE();
    }
  }
}

// Condition conversion
SourceAssembler::Condition Template::j_not(j_condition cc) {
  switch (cc) {
    case j_equal         : return not_equal;
    case j_not_equal     : return equal;
    case j_less          : return greater_equal;
    case j_less_equal    : return greater;
    case j_greater       : return less_equal;
    case j_greater_equal : return less;
  }
  SHOULD_NOT_REACH_HERE();
  return zero;
}

void bc_unimplemented::generate() {
  comment("Unimplemented bytecode");
  int3();
}

#if ENABLE_JAVA_DEBUGGER
void bc_breakpoint::generate() {
  comment("breakpoint processing");
  comment("call JavaDebugger code to notify debugger");
  interpreter_call_vm(Constant("handle_breakpoint"), T_INT);
  comment("bytecode to interpret is in eax");
  comment("Dispatch to the opcode that would have been here");
  andl(eax, Constant(0xFF));
  movl(ebx, eax);
  jmp(Address(no_reg, ebx, times_4, Constant("interpreter_dispatch_table")));
}

#endif

void bc_nop::generate() {
  comment("Do nothing");
}

void bc_aconst_null::generate() {
  push_null_obj();
}

void bc_iconst::generate(jint arg) {
  push_int(arg);
}

void bc_lconst::generate(jlong arg) {
  push_long(arg);
}

void bc_fconst::generate(jfloat arg) {
  push_float(arg);
}

void bc_dconst::generate(jdouble arg) {
  push_double(arg);
}

void bc_bipush::generate() {
  load_signed_byte(eax, bcp_address(1));
  push_int(eax);
}

void bc_sipush::generate() {
  load_unsigned_word(eax, bcp_address(1));
  bswap(eax);
  sarl(eax, Constant(16));
  push_int(eax);
}

void bc_call_vm_redo::generate(const char*name) {
  interpreter_call_vm_redo(Constant(name));
}

void bc_call_vm_dispatch::generate(const char* name) {
  comment("Setup the pointer to the vm routine");
  movl(eax, Constant(name));

  comment("Notice the use of jump here");
  jmp(Constant("interpreter_call_vm_dispatch"));
}

void bc_load_n::generate(BasicType type, int index) {
  switch(type) {
    case T_OBJECT:   push_local_obj(index); break;
    case T_INT:      push_local_int(index); break;
    case T_LONG:     push_local_long(index); break;
    case T_FLOAT:    push_local_float(index); break;
    case T_DOUBLE:   push_local_double(index); break;
    default:         SHOULD_NOT_REACH_HERE();
  }
}


void bc_store_n::generate(BasicType type, int index) {
  switch(type) {
    case T_OBJECT:   pop_local_obj_ret(index, eax); break;
    case T_INT:      pop_local_int(index, eax); break;
    case T_LONG:     pop_local_long(index, eax); break;
    case T_FLOAT:    pop_local_float(index, eax); break;
    case T_DOUBLE:   pop_local_double(index, eax); break;
    default:         SHOULD_NOT_REACH_HERE();
  }
}


void bc_load::generate(BasicType type, bool is_wide) {
  if (is_wide) {
    load_unsigned_word(eax, bcp_address(2));
    xchg(ah, al); // convert to little-endian
  } else {
    load_unsigned_byte(eax, bcp_address(1));
  }
  switch (type) {
    case T_INT     : push_local_int(eax);    break;
    case T_LONG    : push_local_long(eax);   break;
    case T_FLOAT   : push_local_float(eax);  break;
    case T_DOUBLE  : push_local_double(eax); break;
    case T_OBJECT  : push_local_obj(eax);    break;
    default        : SHOULD_NOT_REACH_HERE();
  }
}

void bc_store::generate(BasicType type, bool is_wide) {
  if (is_wide) {
    load_unsigned_word(eax, bcp_address(2));
    xchg(ah, al); // convert to little-endian
  } else {
    load_unsigned_byte(eax, bcp_address(1));
  }
  switch (type) {
  case T_INT     : pop_local_int(eax, ecx);     break;
  case T_LONG    : pop_local_long(eax, ecx);    break;
  case T_FLOAT   : pop_local_float(eax, ecx);   break;
  case T_DOUBLE  : pop_local_double(eax, ecx);  break;
  case T_OBJECT  : pop_local_obj_ret(eax, ecx); break;
  default        : SHOULD_NOT_REACH_HERE();
  }
}

void bc_array_load::generate(BasicType type) {
  comment("Load the array from the stack");
  movl(ecx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));

  comment("Get array index");
  pop_int(edx, edx);

  index_check(ecx, edx, no_reg);

  comment("Get the next bytecode");
  movzxb(ebx, Address(esi, Constant(1)));

  comment("Load array element");
  switch (type) {
  case T_BYTE    :
    load_signed_byte(eax, Address(ecx, edx, times_1,
                                  Constant(Array::base_offset())));
    break;
  case T_CHAR    :
    load_unsigned_word(eax, Address(ecx, edx, times_2,
                                    Constant(Array::base_offset())));
    break;
  case T_SHORT   :
    load_signed_word(eax, Address(ecx, edx, times_2,
                                  Constant(Array::base_offset())));
    break;
  case T_INT     :
    movl(eax, Address(ecx, edx, times_4, Constant(Array::base_offset())));
    break;
  case T_LONG    :
    addl(esp, Constant(BytesPerStackElement));
    break;
  case T_FLOAT   :
    movl(eax, Address(ecx, edx, times_4, Constant(Array::base_offset())));
    break;
  case T_DOUBLE  :
    addl(esp, Constant(BytesPerStackElement));
    break;
  case T_OBJECT  :
    movl(eax, Address(ecx, edx, times_4, Constant(Array::base_offset())));
    break;
  default        :
    SHOULD_NOT_REACH_HERE();
  }

  comment("Update the bytecode pointer");
  incl(esi);

  comment("Load the array element");
  switch (type) {
  case T_BYTE    :
  case T_CHAR    :
  case T_SHORT   :
  case T_INT     :
    movl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))), eax);
    if (TaggedJavaStack) {
      movl(Address(esp), Constant(int_tag));
    }
    break;

  case T_LONG    :
    push_long(Address(ecx, edx, times_8, Constant(Array::base_offset())),
              Address(ecx, edx, times_8, Constant(4 + Array::base_offset())));
    break;

  case T_FLOAT   :
    movl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))), eax);
    if (TaggedJavaStack) {
      movl(Address(esp), Constant(float_tag));
    }
    break;

  case T_DOUBLE  :
    push_double(Address(ecx, edx, times_8, Constant(Array::base_offset())),
               Address(ecx, edx, times_8, Constant(4 + Array::base_offset())));
    break;

  case T_OBJECT  :
    movl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))), eax);
    // Keep tag from entry
    break;

  default        :
    SHOULD_NOT_REACH_HERE();
  }

  comment("Dispatch to the next bytecode");
  jmp(Address(no_reg, ebx, times_4, Constant("interpreter_dispatch_table")));
}

void bc_array_store::generate(BasicType type) {
  comment("Get new element value");
  switch (type) {
  case T_BYTE    : // Fall through.
  case T_CHAR    : // Fall through.
  case T_SHORT   : // Fall through.
  case T_INT     : pop_int(eax, eax);         break;
  case T_LONG    : pop_long(eax, edx);        break;
  case T_FLOAT   : pop_float(eax, eax);       break;
  case T_DOUBLE  : pop_double(eax, edx);      break;
  case T_OBJECT  : pop_obj(eax, eax);         break;
  default        : SHOULD_NOT_REACH_HERE();
  }

  comment("Get array index");
  pop_int(ecx, ecx);
  comment("Get array pointer");
  pop_obj(ebx, ebx);

  index_check(ebx, ecx, no_reg);

  if (type == T_OBJECT) {
    type_check(ebx, eax, ecx);
  }

  comment("Store array element");
  switch (type) {
  case T_BYTE    :
    movb(Address(ebx, ecx, times_1, Constant(Array::base_offset())), eax);
    break;

  case T_CHAR    : // Fall through.
  case T_SHORT   :
    movw(Address(ebx, ecx, times_2, Constant(Array::base_offset())), eax);
    break;

  case T_OBJECT  :
    leal(ebx, Address(ebx, ecx, times_4, Constant(Array::base_offset())));
    movl(Address(ebx), eax);
    oop_write_barrier(ebx, ecx);
    break;

  case T_FLOAT   : // Fall through.
  case T_INT     :
    movl(Address(ebx, ecx, times_4, Constant(Array::base_offset())), eax);
    break;

  case T_DOUBLE  : // Fall through.
  case T_LONG    :
    movl(Address(ebx, ecx, times_8, Constant(Array::base_offset())), eax);
    movl(Address(ebx, ecx, times_8, Constant(4 + Array::base_offset())), edx);
    break;

  default        :
    SHOULD_NOT_REACH_HERE();
  }
}

void bc_pop::generate() {
  increment(esp, BytesPerStackElement);
}

void bc_pop2::generate() {
  increment(esp, 2 * BytesPerStackElement);
}

void bc_dup::generate() {
  for (int i = 0; i < WordsPerStackElement; i++) {
    pushl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  }
}

void bc_dup_x1::generate() {
  // Generate code for a swap.
  if (TaggedJavaStack) {
    TemplateTable::template_for(Bytecodes::_swap)->generate();
    // IMPL_NOTE: We might have stalls due to the implicit writing of esp during
    // the push operation, and the reading of esp during address calculation.
    pushl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
    pushl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  } else {
    popl(eax); popl(ebx);
    pushl(eax); pushl(ebx); pushl(eax);
  }
}

void bc_dup_x2::generate() {
  if (TaggedJavaStack) {
    comment("Make room for the new value");
    decrement(esp, BytesPerStackElement);
    comment("Move the top 3 stack elements up");
    for (int i = 0; i < 3; i++) {
      movl(edx, Address(esp, Constant(12 + i * 8)));
      movl(eax, Address(esp, Constant(8 + i * 8)));
      movl(Address(esp, Constant(4 + i * 8)), edx);
      movl(Address(esp, Constant(i * 8)), eax);
    }
    comment("Copy top stack element into stack");
    movl(edx, Address(esp, Constant(4)));
    movl(eax, Address(esp));
    movl(Address(esp, Constant(4 + 3 * 8)), edx);
    movl(Address(esp, Constant(3 * 8)), eax);
  } else {
    popl(eax); popl(ebx); popl(ecx);
    pushl(eax); pushl(ecx); pushl(ebx); pushl(eax);
  }
}

void bc_dup2::generate() {
  comment("Duplicate long/double stack top (including tags)");
  for (int i = 0; i < 2 * WordsPerStackElement; i++) {
    pushl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  }
}

void bc_dup2_x1::generate() {
  if (TaggedJavaStack) {
    comment("Make room for the new value");
    decrement(esp, 16);
    comment("Move the top 2 (1 long/double) stack elements up");
    for (int i = 0; i < 3; i++) {
      movl(edx, Address(esp, Constant(20 + i * 8)));
      movl(eax, Address(esp, Constant(16 + i * 8)));
      movl(Address(esp, Constant(4 + i * 8)), edx);
      movl(Address(esp, Constant(i * 8)), eax);
    }
    comment("Copy top stack (long/double) element into stack");
    movl(eax, Address(esp));
    movl(edx, Address(esp, Constant(4)));
    movl(ebx, Address(esp, Constant(8)));
    movl(ecx, Address(esp, Constant(12)));
    movl(Address(esp, Constant(3 * 8)), eax);
    movl(Address(esp, Constant(4 + 3 * 8)), edx);
    movl(Address(esp, Constant(8 + 3 * 8)), ebx);
    movl(Address(esp, Constant(12 + 3 * 8)), ecx);
  } else {
    popl(eax); popl(ebx); popl(ecx);
    pushl(ebx); pushl(eax); pushl(ecx); pushl(ebx); pushl(eax);
  }
}

void bc_dup2_x2::generate() {
  if (TaggedJavaStack) {
    comment("Make room for the new value");
    decrement(esp, 16);
    comment("Move the top 3 (1 long/double) stack elements up");
    for (int i = 0; i < 4; i++) {
      movl(edx, Address(esp, Constant(20 + i * 8)));
      movl(eax, Address(esp, Constant(16 + i * 8)));
      movl(Address(esp, Constant(4 + i * 8)), edx);
      movl(Address(esp, Constant(i * 8)), eax);
    }
    comment("Copy top stack (long/double) element into stack");
    movl(eax, Address(esp));
    movl(edx, Address(esp, Constant(4)));
    movl(ebx, Address(esp, Constant(8)));
    movl(ecx, Address(esp, Constant(12)));
    movl(Address(esp, Constant(4 * 8)), eax);
    movl(Address(esp, Constant(4 + 4 * 8)), edx);
    movl(Address(esp, Constant(8 + 4 * 8)), ebx);
    movl(Address(esp, Constant(12 + 4 * 8)), ecx);
  } else {
    popl(eax); popl(ebx); popl(ecx); popl(edx);
    pushl(ebx); pushl(eax);
    pushl(edx); pushl(ecx); pushl(ebx); pushl(eax);
  }
}

void bc_swap::generate() {
  if (TaggedJavaStack) {
    popl(eax);
    popl(edx);

    popl(ebx);
    popl(ecx);

    pushl(edx);
    pushl(eax);

    pushl(ecx);
    pushl(ebx);

  } else {
    popl(eax); popl(ebx);
    pushl(eax); pushl(ebx);
  }
}

void bc_iop2::generate(j_operation op) {
  switch (op) {
    case j_add  :
    case j_sub  :
    case j_mul  :
    case j_and  :
    case j_or   :
    case j_xor  : pop_int(eax, eax); break;
    case j_shl  :
    case j_shr  :
    case j_ushr : pop_int(ecx, ecx); break;
    default     : SHOULD_NOT_REACH_HERE();
  }
  int offset = JavaFrame::arg_offset_from_sp(0);
  switch (op) {
    case j_add  :
      addl (Address(esp, Constant(offset)), eax); break;
    case j_sub  :
      subl (Address(esp, Constant(offset)), eax); break;
    case j_mul  :
      imull(eax, Address(esp, Constant(offset)));
      movl(Address(esp, Constant(offset)), eax); break;
    case j_and  :
      andl (Address(esp, Constant(offset)), eax); break;
    case j_or   :
      orl  (Address(esp, Constant(offset)), eax); break;
    case j_xor  :
      xorl (Address(esp, Constant(offset)), eax); break;
    case j_shl  :
      // Implicit masking of lower 5 bits by Intel shift instr.
      shll (Address(esp, Constant(offset)));      break;
    case j_shr  :
      // Implicit masking of lower 5 bits by Intel shift instr.
      sarl (Address(esp, Constant(offset)));      break;
    case j_ushr :
      // Implicit masking of lower 5 bits by Intel shift instr.
      shrl (Address(esp, Constant(offset)));      break;
    default     :
      SHOULD_NOT_REACH_HERE();
  }
}

void bc_lop2::generate(j_operation op) {
  pop_long(eax, edx);
  pop_long(ebx, ecx);
  switch (op) {
  case j_add  : addl(eax, ebx); adcl(edx, ecx); break;
  case j_sub  : subl(ebx, eax); sbbl(ecx, edx);
                movl(eax, ebx); movl(edx, ecx); break;
  case j_and  : andl(eax, ebx); andl(edx, ecx); break;
  case j_or   : orl (eax, ebx); orl (edx, ecx); break;
  case j_xor  : xorl(eax, ebx); xorl(edx, ecx); break;
  default     : SHOULD_NOT_REACH_HERE();
  }
  push_long(eax, edx);
}

void bc_iinc::generate(bool is_wide) {
  if (is_wide) {
    comment("Get local index");
    load_unsigned_word(ebx, bcp_address(2));
    xchg(bh, bl); // convert to little-endian

    comment("Get constant");
    load_unsigned_word(edx, bcp_address(4));
    xchg(dh, dl); // convert to little-endian
    load_signed_word(edx, edx);
    comment("Do the addition");
  } else {
    comment("Get local index");
    load_unsigned_byte(ebx, bcp_address(1));

    comment("Get constant");
    load_signed_byte(edx, bcp_address(2));

    comment("Do the addition");
  }
  increment_local_int(ebx, edx);
}

void bc_neg::generate(BasicType type) {
  switch (type) {
  case T_INT    :
    pop_int(eax, eax);
    negl(eax);
    push_int(eax);
    break;

  case T_LONG   :
    pop_long(eax, edx);
    negl(eax);
    adcl(edx, Constant(0));
    negl(edx);
    push_long(eax, edx);
    break;

  case T_FLOAT  :
    xorl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))),
         Constant(0x80000000));
    break;

  case T_DOUBLE :
    xorl(Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))),
         Constant(0x80000000));
    break;

  default       : SHOULD_NOT_REACH_HERE();
  }
}

void bc_lcmp::generate() {
  // Long compare for Java (semantics as described in JVM spec.)
  Label L1, L2;

  pop_long(eax, edx);
  pop_long(ebx, ecx);

  subl(ebx, eax);
  sbbl(ecx, edx);
  jcc(no_overflow, Constant(L1));
  comment("If underflow happened then invert the result");
  notl(ecx);

  bind(L1);
  orl(ebx, ecx);
  jcc(zero, Constant(L2));
  sarl(ecx, Constant(31 - 1));
  andl(ecx, Constant(-2));
  incl(ecx);

  bind(L2);
  push_int(ecx);
}

void bc_irem::generate() {
  pop_int(ecx, ecx);
  pop_int(eax, eax);

  // Division by zero check
  testl(ecx, ecx);
  jcc(zero, Constant("interpreter_throw_ArithmeticException"));

  xorl(edx, edx);

  comment("Sign extend 32-bit value in eax to 64-bit value in eax:edx");
  cdql();

  comment("Do the division and push the remainder on the stack");
  corrected_idivl(ecx);
  push_int(edx);
}

void bc_lrem::generate() {
  pop_long(ebx, ecx);    // low, high   divisor
  movl(eax, ebx);
  orl(eax, ecx);
  jcc(zero, Constant("interpreter_throw_ArithmeticException"));
  pop_long(eax, edx);    // low, high   dividend
  pushl(ecx); pushl(ebx);
  pushl(edx); pushl(eax);
  call(Constant("jvm_lrem"));
  addl(esp, Constant(4 * BytesPerWord));
  push_long(eax, edx);
}

void bc_idiv::generate() {
  pop_int(ecx, ecx);
  pop_int(eax, eax);

  // Division by zero check
  testl(ecx, ecx);
  jcc(zero, Constant("interpreter_throw_ArithmeticException"));

  comment("Sign extend 32-bit value in eax to 64-bit value in eax:edx");
  cdql();

  comment("Do the division and push the quotient on the stack");
  corrected_idivl(ecx);
  push_int(eax);
}

void bc_ldiv::generate() {
  pop_long(ebx, ecx);    // low, high   divisor
  movl(eax, ebx);
  orl(eax, ecx);
  jcc(zero, Constant("interpreter_throw_ArithmeticException"));
  pop_long(eax, edx);    // low, high   dividend

  pushl(ecx); pushl(ebx);
  pushl(edx); pushl(eax);
  call(Constant("jvm_ldiv"));
  addl(esp, Constant(4 * BytesPerWord));
  push_long(eax, edx);
}

void bc_lmul::generate() {
  pop_long(ebx, ecx);    // low, high   second argument
  pop_long(eax, edx);    // low, high   first  argument
  pushl(ecx); pushl(ebx);
  pushl(edx); pushl(eax);
  call(Constant("jvm_lmul"));
  addl(esp, Constant(4 * BytesPerWord));
  push_long(eax, edx);
}

void bc_lshl::generate() {
  // Java shift left long support (semantics as described in JVM spec., p.305)
  pop_int(ecx, ecx);
  pop_long(eax, edx);

  Label L;
  andl(ecx, Constant(0x3f));
  cmpl(ecx, Constant(32));
  jcc(less, Constant(L));
  movl(edx, eax);
  xorl(eax, eax);

  bind(L);
  shldl(edx, eax);
  shll(eax);

  push_long(eax, edx);
}

void bc_lshr::generate(bool arithmetic) {
  // Java shift right long support (semantics as described in JVM spec.,
  // p.306 & p.310)
  pop_int(ecx, ecx);
  pop_long(eax, edx);

  Label L;
  andl(ecx, Constant(0x3f));
  cmpl(ecx, Constant(32));
  jcc(less, Constant(L));
  movl(eax, edx);
  if (arithmetic) {
    sarl(edx, Constant(31));
  } else {
    xorl(edx, edx);
  }

  bind(L);
  shrdl(eax, edx);
  if (arithmetic) {
    sarl(edx);
  } else  {
    shrl(edx);
  }

  push_long(eax, edx);
}

void bc_if_0cmp::generate(j_condition condition) {
  // Insert stack check for backwards branch (use for recompilation)
  check_timer_tick();

  // Assume branch is more often taken than not (loops use backward branches).
  Label not_taken;
  pop_int(eax, eax);
  testl(eax, eax);
  jcc(j_not(condition),Constant(not_taken));
  branch(false, false);

  bind(not_taken);
}

void bc_if_icmp::generate(j_condition condition) {
  // Insert stack check for backwards branch (use for recompilation)
  check_timer_tick();

  // Assume branch is more often taken than not (loops use backward branches)
  Label not_taken;
  pop_int(eax, eax);
  pop_int(edx, edx);
  cmpl(edx, eax);
  jcc(j_not(condition), Constant(not_taken));
  branch(false, false);

  bind(not_taken);
}

void bc_if_acmp::generate(j_condition condition) {
  // Insert stack check for backwards branch (use for recompilation)
  check_timer_tick();

  // Assume branch is more often taken than not (loops use backward branches)
  Label not_taken;
  pop_obj(eax, eax);
  pop_obj(edx, edx);
  cmpl(edx, eax);
  jcc(j_not(condition), Constant(not_taken));
  branch(false, false);

  bind(not_taken);
}

void bc_if_nullcmp::generate(j_condition condition) {
  // Insert stack check for backwards branch (use for recompilation)
  check_timer_tick();

  // Assume branch is more often taken than not (loops use backward branches)
  Label not_taken;
  pop_obj(eax, eax);
  testl(eax, eax);
  jcc(j_not(condition), Constant(not_taken));
  branch(false, false);

  bind(not_taken);
}

// Branch routines.
void bc_goto::generate(bool is_wide) {
  // Insert stack check for backwards branch (use for recompilation)
  check_timer_tick();
  branch(false, is_wide);
}

void bc_return::generate(BasicType type) {
  unlock_activation(false);

  if (type != T_VOID) {
    comment("Get the return value");
  }
  switch(type) {
    case T_FLOAT  :
    case T_OBJECT :
    case T_INT    :
      movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
      break;
    case T_LONG   :
    case T_DOUBLE :
      movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
      movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
      break;
    case T_VOID   :
      break;
    default:
      SHOULD_NOT_REACH_HERE();
  }

  comment("Remove activation and store return address in esi");
  remove_activation(esi);

  if (type != T_VOID) {
    comment("Push the return value onto the stack");
  }
  switch(type) {
  case T_INT    : push_int(eax);         break;
  case T_LONG   : push_long(eax, edx);   break;
  case T_FLOAT  : push_float(eax);       break;
  case T_DOUBLE : push_double(eax, edx); break;
  case T_OBJECT : push_obj(eax);         break;
  case T_VOID   : break;
  }

  comment("Jump to return address");
  jmp(esi);

}

void bc_tableswitch::generate() {
  Label default_case, continue_execution;

  comment("Check for timer tick as we can have negative offsets in the table");
  check_timer_tick();

  comment("Get the lookup key");
  pop_int(eax, eax);

  comment("Get an aligned version of the bytecode pointer");
  leal(ebx, bcp_address(wordSize));
  andl(ebx, Constant(-wordSize));

  comment("Load the low and high limits");
  movl(ecx, Address(ebx, Constant(1 * wordSize)));
  movl(edx, Address(ebx, Constant(2 * wordSize)));
  if (!ENABLE_NATIVE_ORDER_REWRITING) {
    bswap(ecx);
    bswap(edx);
  }

  comment("Check against the low and high limits");
  cmpl(eax, ecx);
  jcc(less, Constant(default_case));
  cmpl(eax, edx);
  jcc(greater, Constant(default_case));

  comment("Lookup dispatch offset");
  subl(eax, ecx);
  movl(edx, Address(ebx, eax, times_4, Constant(3 * wordSize)));

  comment("Continue execution");
  bind(continue_execution);
  if (!ENABLE_NATIVE_ORDER_REWRITING) {
    bswap(edx);
  }
  addl(esi, edx);
  dispatch_next(0);

  comment("Handle default case");
  bind(default_case);
  movl(edx, Address(ebx));
  jmp(Constant(continue_execution));
}

void bc_lookupswitch::generate() {
  // This is the linear search variant.
  // Consider binary search if the number of elements is greater than 5.

  Label loop_entry, loop, found, continue_execution;

  comment("Check for timer tick as we can have negative offsets in the table");
  check_timer_tick();

  comment("Get the lookup key");
  pop_int(eax, eax);

  if (!ENABLE_NATIVE_ORDER_REWRITING) {
    comment("Byte-swap eax so we can avoid byte-swapping the table entries");
    bswap(eax);
  }

  comment("Get an aligned version of the bytecode pointer");
  // We should be able to get rid of this instruction (change offsets below)
  leal(ebx, bcp_address(wordSize));
  andl(ebx, Constant(-wordSize));

  comment("Get the number of elements in the table");
  movl(ecx, Address(ebx, Constant(wordSize)));
  if (!ENABLE_NATIVE_ORDER_REWRITING) {
    bswap(ecx);
  }
  jmp(Constant(loop_entry));

  comment("Search the table");
  bind(loop);
  cmpl(eax, Address(ebx, ecx, times_8, Constant(2 * wordSize)));
  jcc(equal, Constant(found));
  bind(loop_entry);
  decl(ecx);
  jcc(greater_equal, Constant(loop));

  comment("Default case");
  movl(edx, Address(ebx));
  jmp(Constant(continue_execution));

  comment("Entry found - get the offset");
  bind(found);
  movl(edx, Address(ebx, ecx, times_8, Constant(3 * wordSize)));

  comment("Continue the execution");
  bind(continue_execution);
  if (!ENABLE_NATIVE_ORDER_REWRITING) {
    bswap(edx);
  }
  addl(esi, edx);
  dispatch_next(0);
}

void bc_i2l::generate() {
  pop_int(eax, eax);
  xorl(edx, edx);
  cdql();
  push_long(eax, edx);
}

void bc_i2d::generate() {
  int offset = 0;
  pop_to_fpu_stack(int_tag, offset);
  push_from_fpu_stack(double_tag, offset);
}

void bc_i2b::generate() {
  pop_int(ecx, ecx);
  movsxb(eax, ecx);
  push_int(eax);
}

void bc_i2c::generate() {
  pop_int(ecx, ecx);
  movzxw(eax, ecx);
  push_int(eax);
}

void bc_i2s::generate() {
  pop_int(ecx, ecx);
  movsxw(eax, ecx);
  push_int(eax);
}

void bc_l2i::generate() {
  pop_long(eax, edx);
  push_int(eax);
}

void bc_athrow::generate() {
  pop_obj(edx, edx);
  null_check(edx);
  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);
  // Like shared_call_vm, etc, this expects the argument in edx
  call_from_interpreter(Constant("shared_call_vm_exception"));
}

void bc_new::generate() {
  comment("Call runtime routine");
  interpreter_call_vm(Constant("newobject"), T_OBJECT);
  push_obj(eax);
}

void bc_anewarray::generate() {
  interpreter_call_vm(Constant("anewarray"), T_ARRAY);
  pop_int(ebx, ebx);
  comment("Push the result on the stack");
  push_obj(eax);
}

void bc_multianewarray::generate() {
  interpreter_call_vm(Constant("multianewarray"), T_ARRAY);
  comment("Remove arguments from the stack");
  load_unsigned_byte(ecx, bcp_address(3));
  leal(esp, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4));

  comment("Push array on the stack");
  push_obj(eax);
}

void bc_arraylength::generate() {
  pop_obj(eax, eax);
  null_check(eax);
  movl(eax, Address(eax, Constant(Array::length_offset())));
  push_int(eax);
}

void bc_monitorenter::generate() {
  comment("Get the object to lock from the stack");
  pop_obj(eax, eax);
  null_check(eax);

  comment("Increment the bytecode pointer before locking to make asynchronous exceptions work");
  increment(esi, 1);
  save_interpreter_state();
  call_from_interpreter(Constant("shared_monitor_enter"));
  restore_interpreter_state();
  dispatch_next();
}

void bc_monitorexit::generate() {
  comment("Get the object to lock from the stack");
  pop_obj(eax, eax);
  null_check(eax);

  save_interpreter_state();
  call_from_interpreter(Constant("shared_monitor_exit"));
  restore_interpreter_state();
}

void bc_instanceof::generate() {
  interpreter_call_vm(Constant("instanceof"), T_INT);
  pop_obj(edx, edx);
  push_int(eax);
}

void bc_checkcast::generate() {
  interpreter_call_vm(Constant("checkcast"), T_VOID);
}

void bc_wide::generate() {
  load_unsigned_byte(ebx, bcp_address(1)); // non-wide bytecode
  if (!GenerateInlineAsm) {
    // address of the wide bytecode implementation is right before the
    // entry point of the normal one
    movl(ebx, Address(no_reg, ebx, times_4,
                      Constant("interpreter_dispatch_table")));
    jmp(Address(ebx, Constant(-4)));
  } else {
    // for inline asm we cannot apply such a solution, so dispatch through
    // a centralized table that maps normal bytecodes to their wide
    // variants.
    movl(ebx, Address(no_reg, ebx, times_4,
                      Constant("interpreter_wide_dispatch_table")));
    jmp(ebx);
  }
}

void bc_fast_ldc::generate(BasicType type, bool is_wide) {
  comment("Get constant pool index");
  if (is_wide) {
    load_unsigned_word(eax, bcp_address(1));
    xchg(ah, al); // convert to little-endian
  } else {
    load_unsigned_byte(eax, bcp_address(1));
  }

  comment("Get constant pool of current method");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Push value on the stack");
  switch (type) {
  case T_INT    :
    push_int(Address(ecx, eax, times_4));
    break;
  case T_FLOAT  :
    push_float(Address(ecx, eax, times_4));
    break;
  case T_OBJECT :
    push_obj(Address(ecx, eax, times_4));
    break;
  case T_LONG   :
    push_long(Address(ecx, eax, times_4),
              Address(ecx, eax, times_4, Constant(4)));
    break;
  case T_DOUBLE :
    push_double(Address(ecx, eax, times_4),
                Address(ecx, eax, times_4, Constant(4)));
    break;
  default       :
    SHOULD_NOT_REACH_HERE();
  }
}


void bc_fast_putstatic::generate(BasicType type, bool with_init) {
  fast_access_field(true, true, type, 2, with_init);
}

void bc_fast_getstatic::generate(BasicType type, bool with_init) {
  fast_access_field(true, false, type, 2, with_init);
}

void bc_fast_putfield::generate(BasicType type) {
  fast_access_field(false, true, type, 2);
}

void bc_fast_getfield::generate(BasicType type, int param_bytes) {
  fast_access_field(false, false, type, param_bytes);
}

void bc_aload_0_fast_getfield_1::generate(BasicType type) {
  fast_access_field(false, false, type, 1, true);
}
void bc_aload_0_fast_getfield_n::generate(BasicType type, int offset) {
  fast_access_field(false, false, type, 0, true, offset);
}

void bc_pop_and_npe_if_null::generate() {
  pop_obj(eax, eax);
  null_check(eax);
}

void bc_init_static_array::generate() {
  movl(eax, Address(esp)); //load array address  
  movzxb(ecx, bcp_address(1)); //load array element size
  movzxw(ebx, bcp_address(2));
  subl(ebx, Constant(1));
  index_check(eax, ebx, edx);
  addl(ebx, Constant(1));
  shldl(ebx, ecx);
  
  addl(esi, Constant(4));
  addl(eax, Constant(Array::base_offset()));
  movl(edx, Constant(0));

  Label copy_dword, copy_done;
  //we copy by 4 bytes cause the target is aligned!
  bind(copy_dword);
  movl(ecx, Address(esi, edx, times_1));
  movl(Address(eax, edx, times_1), ecx);
  addl(edx, Constant(4));
  cmpl(edx, ebx);
  jcc(less, Constant(copy_dword));  

  bind(copy_done);
  addl(esi, ebx);  
}


void bc_fast_invoke::generate(bool has_fixed_target_method) {
  Bytecodes::Code bc = bytecode();
  bool must_do_null_check = (bc != Bytecodes::_fast_invokestatic &&
                             bc != Bytecodes::_fast_init_invokestatic);

  Label restart;
  bind(restart);

  comment("Get current method");
  movl(edx, Address(ebp, Constant(JavaFrame::method_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get constant pool of current method");
  movl(edx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  if (has_fixed_target_method) {
    comment("Get method from resolved constant pool entry");
    movl(ebx, Address(edx, eax, times_4));
    if (must_do_null_check) {
      comment("Get the number of parameters from the method");
      load_unsigned_word(ecx, Address(ebx, 
          Constant(Method::method_attributes_offset())));
      andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));

      comment("Get receiver object");
      movl(edx, Address(esp, ecx,
                        TaggedJavaStack ? times_8 : times_4,
                        Constant(-4)));
      comment("Get prototype of receiver object");
      null_check(edx);
    } else {
      // A class initialization barrier is required only on static method invocation.
      // Incidentally, those have no receivers and therefore do not need null ptr
      // ptr check. Hence the "else" here!
#if ENABLE_ISOLATES
      Label class_is_initialized;
      comment("Class initialization barrier (static method invocation)");
      comment("load method holder's index");
      movzxw(edx, Address(ebx, Constant(Method::holder_id_offset())));
      comment("load task mirror from class list in eax");
      load_task_mirror_from_list(eax, edx, ecx);
      cib_with_marker(eax, class_is_initialized);
      comment("load JavaClass object to pass in parameter to the barrier handler");
      comment("edx still holds class_id");
      load_class_from_list(edx, ecx);
      interpreter_call_vm(Constant("task_barrier"), T_OBJECT);
      comment("Restore state");
      movl(edx, Address(ebp, Constant(JavaFrame::method_offset())));

      comment("Get constant pool index");
      load_unsigned_word(eax, bcp_address(1));
      xchg(ah, al); // convert to little-endian

      comment("Get constant pool of current method");
      movl(edx, Address(ebp, Constant(JavaFrame::cpool_offset())));
      comment("Get method from resolved constant pool entry");
      movl(ebx, Address(edx, eax, times_4));
      bind(class_is_initialized);
#else
      if (bc == Bytecodes::_fast_init_invokestatic) {
        movzxw(eax, Address(ebx, Constant(Method::holder_id_offset())));
        load_class_from_list(edx, eax, edx);
        initialize_class_when_needed(edx, eax, ecx, restart);
      }
#endif
    }
  } else {
    comment("Get vtable index from constant pool entry");
    load_unsigned_word(ebx, Address(edx, eax, times_4));

    comment("Get the class from constant pool entry");
    load_unsigned_word(edx, Address(edx, eax, times_4, Constant(sizeof(jushort))));
    load_class_from_list(edx, eax);

    comment("Get the ClassInfo");
    movl(edx, Address(edx, Constant(JavaClass::class_info_offset())));

    comment("Get the method");
    movl(edx, Address(edx, ebx, times_4, Constant(ClassInfoDesc::header_size())));

    comment("Get the number of parameters from method");
    load_unsigned_word(ecx, Address(edx,
        Constant(Method::method_attributes_offset())));
    andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));

    comment("Get receiver object");
    movl(edx, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4, Constant(-4)));

    null_check(edx);

    comment("Get prototype of receiver object");
    movl(edx, Address(edx));    // java near
    comment("Get method from vtable of the class");
    movl(edx, Address(edx, Constant(JavaNear::class_info_offset())));
    movl(ebx, Address(edx, ebx, times_4, Constant(ClassInfoDesc::header_size())));
  }

  comment("Call method");
  movl(esi, Address(ebx, Constant(Method::variable_part_offset())));
  movl(esi, Address(esi));
  call_from_interpreter(esi);

  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
  }
}

void bc_fast_invokespecial::generate() {
  comment("Get current method");
  movl(edx, Address(ebp, Constant(JavaFrame::method_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get constant pool of current method");
  movl(edx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Save bytecode pointer");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  comment("Get vtable index from constant pool entry");
  load_unsigned_word(ebx, Address(edx, eax, times_4));

  comment("Get the class from constant pool entry");
  load_unsigned_word(edx, Address(edx, eax, times_4, Constant(sizeof(jushort))));
  load_class_from_list(edx, eax);

  comment("Get the ClassInfo");
  movl(edx, Address(edx, Constant(JavaClass::class_info_offset())));

  comment("Get method from vtable of the ClassInfo");
  movl(ebx, Address(edx, ebx, times_4,Constant(ClassInfoDesc::header_size())));

  comment("Get the number of parameters from method");
  load_unsigned_word(ecx, Address(ebx, 
      Constant(Method::method_attributes_offset())));
  andl(ecx, Constant(Method::SIZE_OF_PARAMETERS_MASK));

  comment("Get receiver object and perform the null check");
  movl(ecx, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4, Constant(-4)));
  null_check(ecx);

  comment("Call method");
  movl(esi, Address(ebx, Constant(Method::variable_part_offset())));
  movl(esi, Address(esi));
  call(esi);

  define_call_info();

  if (GenerateInlineAsm) {
    jmp(Constant("invoke3_deoptimization_entry"));
  } else {
    comment("FALLTHROUGH");
  }
  entry_end(); // bc_fast_invokespecial_internal

  if (GenerateInlineAsm) {
    entry("invoke3_deoptimization_entry_0");
    jmp(Constant("invoke3_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke3_deoptimization_entry_1");
    jmp(Constant("invoke3_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke3_deoptimization_entry_2");
    jmp(Constant("invoke3_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke3_deoptimization_entry_3");
    jmp(Constant("invoke3_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke3_deoptimization_entry_4");
    jmp(Constant("invoke3_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke3_deoptimization_entry");
  } else {
    entry("invoke3_deoptimization_entry", 1);
    entry("invoke3_deoptimization_entry_0", 1);
    entry("invoke3_deoptimization_entry_1", 1);
    entry("invoke3_deoptimization_entry_2", 1);
    entry("invoke3_deoptimization_entry_3", 1);
    entry("invoke3_deoptimization_entry_4", 1);
  }

  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
  }
}

void bc_fast_invokeinterface::generate() {
  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get number of parameters");
  load_unsigned_byte(ecx, bcp_address(3));

  // eax = constant pool index for class id and itable index.
  // ecx = numbers of parameters.

  comment("Get receiver object");
  movl(edx, Address(esp, ecx, TaggedJavaStack ? times_8 : times_4, Constant(-4)));
  null_check(edx); // we must do null check before we change
                   // bytecode pointer register (esi)

  // eax = constant pool index for class id and itable index.
  // ecx = numbers of parameters.
  // edx = receiver object.

  comment("Save bytecode pointer to allow usage of esi in implementation of fast_invokeinterface");
  movl(Address(ebp, Constant(JavaFrame::bcp_store_offset())), esi);

  comment("Get constant pool of current method");
  movl(esi, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool entry for class id and itable index");
  load_unsigned_word(ebx, Address(esi, eax, times_4, Constant(2)));
  load_unsigned_word(esi, Address(esi, eax, times_4));

  // ebx = class_id for interface.
  // ecx = numbers of parameters.
  // edx = receiver object.
  // esi = itable index

  comment("Get class of receiver object");
  movl(edx, Address(edx));
  movl(edx, Address(edx));

  // ebx = class_id of interface
  // ecx = numbers of parameters.
  // edx = receiver class.
  // esi = itable index.

  comment("Get the itable from the class of the receiver object");
  movl(ecx, Address(edx, Constant(JavaClass::class_info_offset())));
  movzxw(edi, Address(ecx, Constant(ClassInfo::vtable_length_offset())));
  movzxw(eax, Address(ecx, Constant(ClassInfo::itable_length_offset())));
  leal(edi, Address(ecx, edi, times_4, Constant(ClassInfoDesc::header_size())));

  // eax = length of interface table.
  // ebx = class_id of interface.
  // ecx = class_info of receiver class.
  // edx = receiver class.
  // esi = itable index.
  // edi = start of itable.

  Label lookup, found, okay;
  comment("Lookup interface method table by linear search");
bind(lookup);
  subl(eax, Constant(1));
  jcc(greater_equal, Constant(okay));
  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  jmp(Constant("interpreter_throw_IncompatibleClassChangeError"));
bind(okay);  
  cmpl(ebx, Address(edi));
  jcc(equal, Constant(found));
  increment(edi, 8);
  jmp(Constant(lookup));

  // ebx = class_id of interface
  // ecx = class_info of receiver class.
  // edx = receiver class.
  // esi = itable index.
  // edi = found itable entry.

  bind(found);
  comment("Found the itable entry - now get the method table offset from there");
  movl(ebx, Address(edi, Constant(4)));
  leal(ebx, Address(ecx, ebx, times_1));

  // ebx = method table in receiver class.
  // ecx = class_info of receiver class.
  // edx = receiver class.
  // esi = itable index.

  comment("Get the method from the method table");
  movl(ebx, Address(ebx, esi, times_4));

  // ebx = method to call.
  // edx = receiver class.

  comment("Call method");
  movl(esi, Address(ebx, Constant(Method::variable_part_offset())));
  movl(esi, Address(esi));
  call(esi);

  define_call_info();

  if (GenerateInlineAsm) {
    jmp(Constant("invoke5_deoptimization_entry"));
  } else {
    comment("FALLTHROUGH");
  }

  entry_end(); // bc_fast_invokeinterface_internal

  if (GenerateInlineAsm) {
    entry("invoke5_deoptimization_entry_0");
    jmp(Constant("invoke5_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke5_deoptimization_entry_1");
    jmp(Constant("invoke5_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke5_deoptimization_entry_2");
    jmp(Constant("invoke5_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke5_deoptimization_entry_3");
    jmp(Constant("invoke5_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke5_deoptimization_entry_4");
    jmp(Constant("invoke5_deoptimization_entry"));
    entry_end(); // bc_fast_invokespecial_internal

    entry("invoke5_deoptimization_entry");
  } else {
    entry("invoke5_deoptimization_entry", 1);
    entry("invoke5_deoptimization_entry_0", 1);
    entry("invoke5_deoptimization_entry_1", 1);
    entry("invoke5_deoptimization_entry_2", 1);
    entry("invoke5_deoptimization_entry_3", 1);
    entry("invoke5_deoptimization_entry_4", 1);
  }

  comment("Restore bytecode pointer");
  movl(esi, Address(ebp, Constant(JavaFrame::bcp_store_offset())));
  movl(edi, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  if (ENABLE_WTK_PROFILER) {
    interpreter_call_vm(Constant("jprof_record_method_transition"), T_VOID);
  }
}

void bc_fast_invokenative::generate() {
  // A 'return' bytecode always follows this bytecode.
  // Therefore it's okay that we do not pop the arguments.

  Label set_param, redo, next;

  bind(redo);

  comment("Get address of first paramater");
  movl(eax, Address(ebp, Constant(JavaFrame::method_offset())));
  movl(ebx, edi);
  //movl(ebx, Address(ebp, Constant(JavaFrame::locals_pointer_offset())));

  comment("Set space for fake parameter for static method (KNI-ism)");
  load_unsigned_word(eax, Address(eax,
                                  Constant(Method::access_flags_offset())));
  andl(eax, Constant(JVM_ACC_STATIC));
  cmpl(eax, Constant(JVM_ACC_STATIC));
  jcc(not_zero, Constant(set_param));
  addl(ebx, Constant(BytesPerStackElement));

  bind(set_param);
  comment("Point _kni_parameter_base to the first parameter");
  movl(Address(Constant("_kni_parameter_base")), ebx);

  // The redo is necessary for implementing async native methods for
  // LWT. If a native method (e.g., socket.read()) must block, the
  // method calls Scheduler::set_thread_blocked(), which sets
  // Thread.async_redo to true. The native method then returns
  // immediately.
  //
  // We'll come to the check of Thread.async_redo below when the
  // native methods becomes ready for execution (by code inside
  // Scheduler::check_async_threads()). We call the native method
  // again and then return normally.
  //
  // Note that _kni_parameter_base also needs to be refreshed when
  // redoing, because another native method may have been executed and
  // overwritten _kni_parameter_base.
  //
  // If a method is redone, any previous return values are ignored.

  comment("Get the pointer from the bytecode");
  load_unsigned_byte(ebx, bcp_address(1));
  movl(eax, bcp_address(Method::native_code_offset_from_bcp()));

  Label invoke_non_object, invoke_join;
  cmpl(ebx, Constant(T_OBJECT));
  comment("Call the native code");
  jcc(not_equal, Constant(invoke_non_object));
  interpreter_call_native(eax, T_OBJECT);
  jmp(Constant(invoke_join));
bind(invoke_non_object);
  interpreter_call_native(eax, T_INT);
bind(invoke_join);

  get_thread(ebx);
  movl(ecx, Address(ebx, Constant(Thread::async_redo_offset())));
  testl(ecx, ecx);

  jcc(zero, Constant(next));
  comment("Need to redo the native method invocation");
  movl(ecx, Constant(0));

  // clear Thread.async_redo so that we won't loop indefinitely.
  movl(Address(ebx, Constant(Thread::async_redo_offset())), ecx);
  jmp(Constant(redo));

  bind(next);

  // Clear Thread.async_object, if necessary
  // We're doing this even if the method was never redone, but the
  // gain of optimizing it is probably miniscule.
  movl(Address(ebx, Constant(Thread::async_info_offset())), ecx);

  load_unsigned_byte(ecx, bcp_address(1));
  jmp(Address(no_reg, ecx, times_4,
              Constant("_bc_fast_invokenative_returns", -6 * BytesPerWord)));

  entry_end(); // bc_fast_invokenative_internal

  entry("_bc_fast_invokenative_return_int");
    push_int(eax);
    jmp(Constant("bc_ireturn_internal"));
  entry_end();

  entry("_bc_fast_invokenative_return_long");
    push_long(eax, edx);
    jmp(Constant("bc_lreturn_internal"));
  entry_end();

  entry("_bc_fast_invokenative_return_object");
    push_obj(eax);
    jmp(Constant("bc_areturn_internal"));
  entry_end();

  entry("_bc_fast_invokenative_return_void");
    jmp(Constant("bc_return_internal"));
  entry_end();

#if ENABLE_FLOAT
  entry("_bc_fast_invokenative_return_float");
    // IMPL_NOTE:  Check.  Is this really how floats are returned by natives?
    push_from_fpu_stack(float_tag, 0);
    jmp(Constant("bc_freturn_internal"));
  entry_end();

  entry("_bc_fast_invokenative_return_double");
    push_from_fpu_stack(double_tag, 0);
    jmp(Constant("bc_dreturn_internal"));
  entry_end();

#endif
  entry("_bc_fast_invokenative_return_illegal");
    int3();
  entry_end();

  define_array_begin("void * const", "_bc_fast_invokenative_returns",
                     14 - 6 + 1);
  for (int i = 6; i <= 14; i++) {
#define RETURN_PATH(x) \
        define_long_element(Constant("_bc_fast_invokenative_return_" #x))
    switch(i) {
      case T_INT:     RETURN_PATH(int);     break;
      case T_LONG:    RETURN_PATH(long);    break;
      case T_VOID:    RETURN_PATH(void);    break;
      case T_OBJECT:  RETURN_PATH(object);  break;
#if ENABLE_FLOAT
      case T_FLOAT:   RETURN_PATH(float);   break;
      case T_DOUBLE:  RETURN_PATH(double);  break;
#endif
      default:        RETURN_PATH(illegal); break;
    }
#undef RETURN_PATH
  }
  define_array_end();

  // IMPL_NOTE: caller will call entry_end on us, but we have alread closed
  // all functions to emit the array definition, so provide a fake entry.
  if (GenerateInlineAsm)
    entry("_bc_fast_invokenative_XXX_fake");
}

void bc_fast_new::generate() {
  Label restart;
  bind(restart);

  comment("Get constant pool of current method");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get class from resolved constant pool entry");
  movl(eax, Address(ecx, eax, times_4));
  load_class_from_list(edx, eax, ebx);

#if ENABLE_ISOLATES
  Label class_is_initialized;
  comment("load task mirror from class list in eax");
  load_task_mirror_from_list(ebx, eax, ebx);
  cib_with_marker(ebx, class_is_initialized);

  interpreter_call_vm(Constant("task_barrier"), T_OBJECT);
  comment("Restore state");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get class from resolved constant pool entry");
  movl(edx, Address(ecx, eax, times_4));
  load_class_from_list(edx, ebx);
  bind(class_is_initialized);
#else
  if (bytecode() == Bytecodes::_fast_init_new) {
    initialize_class_when_needed(edx, ebx, ecx, restart);
  }
#endif

  comment("Get instance size from class");
  movsxw(ebx, Address(edx, Constant(FarClass::instance_size_offset())));

  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  comment("Compute new top");
  leal(ecx, Address(eax, ebx, times_1));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant("bc_new"));
  }

  comment("Compare against _inline_allocation_end");
  cmpl(ecx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant("bc_new"));

  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), ecx);

  comment("Get prototypical near from far class");
  movl(edx, Address(edx, Constant(FarClass::prototypical_near_offset())));

  comment("Set prototypical near in object");
  movl(Address(eax), edx);

  comment("Compute remaining size");
  decrement(ebx, oopSize);

  comment("One-word object?");
  Label init_done;
  jcc(zero, Constant(init_done));

  comment("Zero object fields");
  movl(edx, eax);
  xorl(ecx, ecx);
  Label init_loop;
  bind(init_loop);
  increment(edx, oopSize);
  movl(Address(edx), ecx);
  decrement(ebx, oopSize);
  jcc(not_zero, Constant(init_loop));

  bind(init_done);
  comment("Push result onto stack");
  push_obj(eax);
}

void bc_fast_checkcast::generate() {
  Label done;

  comment("Get the object to check cast for - ignore tag");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));

  comment("Check for NULL object");
  testl(eax, eax);
  jcc(zero, Constant(done));

  comment("Get the class for the object");
  movl(edx, Address(eax));
  movl(edx, Address(edx));

  comment("Get constant pool of current method");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get class from resolved constant pool entry");
  movl(ecx, Address(ecx, eax, times_4));
  load_class_from_list(ecx, ebx);

  // This is likely to slow it down
  // comment("Compare the classes");
  // cmpl(edx, ecx);
  // jcc(equal, done);

  comment("Check the subtype caches");
  cmpl(ecx, Address(edx, Constant(JavaClass::subtype_cache_1_offset())));
  jcc(equal, Constant(done));
  cmpl(ecx, Address(edx, Constant(JavaClass::subtype_cache_2_offset())));
  jcc(equal, Constant(done));
  jmp(Constant("bc_checkcast"));

  bind(done);
}

void bc_fast_instanceof::generate() {
  Label done;

  comment("Get the object to check cast for - ignore tag");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));

  comment("Check for NULL object");
  testl(eax, eax);
  jcc(zero, Constant(done));

  comment("Get the class for the object");
  movl(edx, Address(eax));
  movl(edx, Address(edx));

  comment("Set the (possible) result to 1");
  xorl(eax, eax);
  increment(eax, 1);

  comment("Get constant pool of current method");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool index");
  load_unsigned_word(ebx, bcp_address(1));
  xchg(bh, bl); // convert to little-endian

  comment("Get class from resolved constant pool entry");
  movl(ecx, Address(ecx, ebx, times_4));
  load_class_from_list(ecx, ebx);

  // This check is likely to make it slower
  // comment("Compare the classes");
  // cmpl(edx, ecx);
  // jcc(equal, done);

  comment("Check the subtype caches");
  cmpl(ecx, Address(edx, Constant(JavaClass::subtype_cache_1_offset())));
  jcc(equal, Constant(done));
  cmpl(ecx, Address(edx, Constant(JavaClass::subtype_cache_2_offset())));
  jcc(equal, Constant(done));
  jmp(Constant("bc_instanceof"));

  bind(done);

    pop_obj(edx, edx);
    push_int(eax);

}

void bc_newarray::generate() {
  Label slow, done;
  comment("Get the array type");
  load_unsigned_byte(eax, bcp_address(1));

  // eax = array type
  comment("Compute size of array");
  movl(ebx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));

  // Call slow case if the length is too large or less than zero
  cmpl(ebx, Constant(maximum_safe_array_length));
  jcc(above, Constant(slow));

  // Calculate length into ebx
  movl(ecx, eax);               // Get the tag
  andl(ecx, Constant(3));       // Low three bits of tag
  shll(ebx);
  addl(ebx, Constant(Array::base_offset() + BytesPerWord - 1));
  shrl(ebx, Constant(2));

  // eax = array type
  // ebx = array size (in words)
  comment("Get the array class");
  // do the same thing as Universe::as_TypeArrayClass()
  movl(ecx,
       Address(no_reg, eax, times_4,
               Constant("persistent_handles",
             (Universe::bool_array_class_index - T_BOOLEAN) * BytesPerWord)));

  // eax = array type
  // ebx = array size (in words)
  // ecx = array class
  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  // eax = inline contig allocation top
  // ebx = array size (in words)
  // ecx = array class
  comment("Compute new top");
  leal(edx, Address(eax, ebx, times_4));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant(slow));
  }

  comment("Check for overflow");
  cmpl(edx, eax);
  jcc(below, Constant(slow));

  // eax = inline contig allocation top
  // ebx = array size (in words)
  // ecx = array class
  // edx = new top
  comment("Compare against _inline_allocation_end");
  cmpl(edx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant(slow));

  // eax = inline contig allocation top
  // ebx = array size (in words)
  // ecx = array class
  // edx = new top
  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), edx);

  // eax = new array
  // ebx = array size (in words)
  // ecx = array class
  // edx = new top
  comment("Get prototypical near from far class");
  movl(ecx, Address(ecx, Constant(FarClass::prototypical_near_offset())));

  // eax = new array
  // ebx = array size (in words)
  // ecx = prototypical near of array class
  // edx = new top
  comment("Clear fields");
  Label clear_done, clear_loop;
  xorl(edx, edx);
  cmpl(ebx, Constant(0));
  jcc(equal, Constant(clear_done));

  bind(clear_loop);
  movl(Address(eax, ebx, times_4, Constant(-4)), edx);
  decrement(ebx, 1);
  jcc(not_zero, Constant(clear_loop));

  comment("Set prototypical near in array");
  bind(clear_done);
  movl(Address(eax, Constant(Oop::klass_offset())), ecx);

  comment("Set length in array");
  pop_int(ebx, ebx);
  movl(Address(eax, Constant(Array::length_offset())), ebx);
  jmp(Constant(done));

bind(slow);
  interpreter_call_vm(Constant("newarray"), T_ARRAY);
  pop_int(edx, edx);

bind(done);
  comment("Push result onto stack");
  push_obj(eax);
}


void bc_fast_anewarray::generate() {
  comment("Get constant pool of current method");
  movl(ecx, Address(ebp, Constant(JavaFrame::cpool_offset())));

  comment("Get constant pool index");
  load_unsigned_word(eax, bcp_address(1));
  xchg(ah, al); // convert to little-endian

  comment("Get class from resolved constant pool entry");
  movl(ecx, Address(ecx, eax, times_4));
#if ENABLE_ISOLATES
  Label done;

  comment("Save class index");
  movl(edx, ecx);
  load_class_from_list(ecx, ebx);
  movl(ecx, Address(ecx, Constant(JavaClass::array_class_offset())));
  orl(ecx, ecx);
  jcc(not_zero, Constant(done));

  movl(ecx, edx);
  comment("Get array class from class");
  load_task_mirror_from_list(ecx, ecx, ebx); //class id is in ecx
  movl(ecx, Address(ecx,Constant(TaskMirror::array_class_offset())));
  comment("May have _task_class_init_marker here, check for null array class");
  testl(ecx, ecx);
  jcc(zero, Constant("bc_anewarray"));
bind(done);
#else
  load_class_from_list(ecx, ebx);
  movl(ecx, Address(ecx, Constant(JavaClass::array_class_offset())));
#endif
  comment("Compute size of array");
  movl(ebx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));

  // Call slow case if the length is too large or less than zero
  cmpl(ebx, Constant(maximum_safe_array_length));
  jcc(above, Constant("bc_anewarray"));

  shll(ebx, Constant(2));
  addl(ebx, Constant(Array::base_offset()));

  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  comment("Compute new top");
  leal(edx, Address(eax, ebx, times_1));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant("bc_anewarray"));
  }

  comment("Check for overflow");
  cmpl(edx, eax);
  jcc(below, Constant("bc_anewarray"));

  comment("Compare against _inline_allocation_end");
  cmpl(edx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant("bc_anewarray"));

  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), edx);

  comment("Get prototypical near from far class");
  movl(ecx, Address(ecx, Constant(FarClass::prototypical_near_offset())));

  comment("Set prototypical near in array");
  movl(Address(eax, Constant(Oop::klass_offset())), ecx);

  comment("Set length in array");
  pop_int(ebx, ebx);
  movl(Address(eax, Constant(Array::length_offset())), ebx);

  comment("Compute remaining size");
  cmpl(ebx, Constant(0));

  comment("Empty array?");
  Label init_done;
  jcc(zero, Constant(init_done));

  comment("Zero object fields");
  xorl(ecx, ecx);
  Label init_loop;
  bind(init_loop);
  movl(Address(eax, ebx, times_4, Constant(Array::base_offset() - oopSize)), ecx);

  decrement(ebx, 1);
  jcc(not_zero, Constant(init_loop));
  bind(init_done);

  comment("Push result onto stack");
  push_obj(eax);
}

#endif

/*************************************
 * Temporarily disabled robocop stuff.
 *************************************
 */
#if NOT_CURRENTLY_USED

void TemplateTable::robo_iinc() {
  // Allocate a robocop template with room for 7 instructions.
  RobocopTemplate rt(7);

  // Add the instructions to the robocop template.
  RobocopInstruction* load_index         = rt.add(new_movzxb (eax, Address(esi, 1)));
  RobocopInstruction* load_constant      = rt.add(new_movsxb (edx, Address(esi, 2)));
  RobocopInstruction* negate_index       = rt.add(new_negl   (eax));
  RobocopInstruction* increment_local    = rt.add(new_addl   (Address(edi, eax, times_8), edx));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 3)));
  RobocopInstruction* increment_bcp      = rt.add(new_addl   (esi, 3));
  RobocopInstruction* dispatch           = rt.add(new_jmp    (Address(no_reg, ebx, times_4, "interpreter_dispatch_table")));

  // We cannot negate the index before the index has been loaded.
  negate_index->must_be_after(load_index);

  // We cannot increment the local before the index has been negated, and the constant has been loaded.
  increment_local->must_be_after(negate_index);
  increment_local->must_be_after(load_constant);

  // The dispatch instruction must be after all other instructions.
  dispatch->must_be_after(load_index);
  dispatch->must_be_after(load_constant);
  dispatch->must_be_after(negate_index);
  dispatch->must_be_after(increment_local);
  dispatch->must_be_after(load_next_bytecode);
  dispatch->must_be_after(increment_bcp);

  // Dump the robocop template.
  rt.dump(83); // 39
}

void TemplateTable::robo_iload(int argument) {
  RobocopTemplate rt(5);

  // Add the instructions to the robocop template.
  RobocopInstruction* push_local         = rt.add(new_pushl  (Address(edi, -argument * 8)));
  RobocopInstruction* push_tag           = rt.add(new_pushl  (int_tag));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));
  RobocopInstruction* dispatch           = rt.add(new_jmp    (Address(no_reg, ebx, times_4, "interpreter_dispatch_table")));

  // We must preserve the right stack ordering.
  push_tag->must_be_after(push_local);

  // The dispatch instruction must be after all other instructions.
  dispatch->must_be_after(push_local);
  dispatch->must_be_after(push_tag);
  dispatch->must_be_after(load_next_bytecode);
  dispatch->must_be_after(increment_bcp);

  // Dump the robocop template.
  rt.dump(11);
}

void TemplateTable::robo_istore(int argument) {
  RobocopTemplate rt(6);

  // Add the instructions to the robocop template.
  RobocopInstruction* pop_tag            = rt.add(new_popl   (eax));
  RobocopInstruction* pop_local          = rt.add(new_popl   (Address(edi, -argument * 8)));
  RobocopInstruction* store_tag          = rt.add(new_movl   (Address(edi, -argument * 8 - 4), int_tag));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));
  RobocopInstruction* dispatch           = rt.add(new_jmp    (Address(no_reg, ebx, times_4, "interpreter_dispatch_table")));

  // We must preserve the right stack ordering.
  pop_local->must_be_after(pop_tag);

  // The dispatch instruction must be after all other instructions.
  dispatch->must_be_after(pop_tag);
  dispatch->must_be_after(pop_local);
  dispatch->must_be_after(store_tag);
  dispatch->must_be_after(load_next_bytecode);
  dispatch->must_be_after(increment_bcp);

  // Dump the robocop template.
  rt.dump(33);
}

void TemplateTable::robo_iadd() {
  RobocopTemplate rt(6);

  // Add the instructions to the robocop template.
  RobocopInstruction* pop_tag            = rt.add(new_popl   (eax));
  RobocopInstruction* pop_value          = rt.add(new_popl   (eax));
  RobocopInstruction* do_addition        = rt.add(new_addl   (Address(esp, 4), eax));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));
  RobocopInstruction* dispatch           = rt.add(new_jmp    (Address(no_reg, ebx, times_4, "interpreter_dispatch_table")));

  // We must preserve the right stack ordering.
  pop_value->must_be_after(pop_tag);

  // We cannot add before we've loaded the value.
  do_addition->must_be_after(pop_value);

  // The dispatch instruction must be after all other instructions.
  dispatch->must_be_after(pop_tag);
  dispatch->must_be_after(pop_value);
  dispatch->must_be_after(do_addition);
  dispatch->must_be_after(load_next_bytecode);
  dispatch->must_be_after(increment_bcp);

  // Dump the robocop template.
  rt.dump(3);
}

void TemplateTable::robo_iconst(int argument) {
  RobocopTemplate rt(5);

  // Add the instructions to the robocop template.
  RobocopInstruction* push_value         = rt.add(new_pushl  (argument));
  RobocopInstruction* push_tag           = rt.add(new_pushl  (int_tag));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));
  RobocopInstruction* dispatch           = rt.add(new_jmp    (Address(no_reg, ebx, times_4, "interpreter_dispatch_table")));

  // We must preserve the right stack ordering.
  push_tag->must_be_after(push_value);

  // The dispatch instruction must be after all other instructions.
  dispatch->must_be_after(push_value);
  dispatch->must_be_after(push_tag);
  dispatch->must_be_after(load_next_bytecode);
  dispatch->must_be_after(increment_bcp);

  // Dump the robocop template.
  rt.dump(8);
}

void TemplateTable::robo_load(bool is_wide, BasicType type) {
  RobocopTemplate rt(8);

  RobocopInstruction* load_index;
  if (is_wide) {
    load_index = rt.add(new_movzxw(eax, bcp_address(2)));
  } else {
    load_index = rt.add(new_movzxb(eax, bcp_address(1)));
  }
  RobocopInstruction* negate_index = rt.add(new_negl(eax));

  // Make sure the index isn't negated before it's loaded.
  negate_index->must_be_after(load_index);

  switch (type) {
  case T_OBJECT  : // Fall through.
  case T_FLOAT   : // Fall through.
  case T_INT     : { RobocopInstruction* push_val = rt.add(new_pushl(Address(edi, eax, times_8)));
                     RobocopInstruction* push_tag;
                     switch (type) {
                     case T_INT    : push_tag = rt.add(new_pushl(int_tag));   break;
                     case T_OBJECT : push_tag = rt.add(new_pushl(obj_tag));   break;
                     case T_FLOAT  : push_tag = rt.add(new_pushl(float_tag)); break;
                     default       : SHOULD_NOT_REACH_HERE();                       break;
                     }
                     push_val->must_be_after(negate_index);
                     push_tag->must_be_after(push_val);
                     break;
                   }
  case T_LONG    : // Fall through.
  case T_DOUBLE  : { RobocopInstruction* push_pad0          = rt.add(new_pushl  (uninitialized_tag));
                     RobocopInstruction* push_high          = rt.add(new_pushl  (Address(edi, eax, times_8, -4)));
                     RobocopInstruction* push_low           = rt.add(new_pushl  (Address(edi, eax, times_8, -8)));
                     RobocopInstruction* push_tag;
                     switch (type) {
                     case T_LONG   : push_tag = rt.add(new_pushl  (long_tag));   break;
                     case T_DOUBLE : push_tag = rt.add(new_pushl  (double_tag)); break;
                     default       : SHOULD_NOT_REACH_HERE();                          break;
                     }
                     push_high->must_be_after(negate_index);
                     push_low->must_be_after(negate_index);
                     push_tag->must_be_after(push_low);
                     push_low->must_be_after(push_high);
                     push_high->must_be_after(push_pad0);
                     break;
                   }
  default        : SHOULD_NOT_REACH_HERE();
  }

  RobocopInstruction* load_next_bytecode;
  RobocopInstruction* increment_bcp;
  if (is_wide) {
    load_next_bytecode = rt.add(new_movzxb(ebx, Address(esi, 5)));
    increment_bcp = rt.add(new_addl(esi, 5));
  } else {
    load_next_bytecode = rt.add(new_movzxb(ebx, Address(esi, 2)));
    increment_bcp = rt.add(new_addl(esi, 2));
  }

  switch (type) {
  case T_INT     : rt.dump(14);          break;
  case T_OBJECT  : rt.dump(14);          break;
  case T_FLOAT   : rt.dump(14);          break;
  case T_LONG    : rt.dump(61);          break;
  case T_DOUBLE  : rt.dump(52);          break;
  default        : SHOULD_NOT_REACH_HERE(); break;
  }
  jmp(Address(no_reg, ebx, times_4, "interpreter_dispatch_table"));
}

void TemplateTable::robo_load(BasicType type) {
  robo_load(false, type);
}

void TemplateTable::robo_dup() {
  RobocopTemplate rt(4);

  RobocopInstruction* push_value         = rt.add(new_pushl(Address(esp, 4)));
  RobocopInstruction* push_tag           = rt.add(new_pushl(Address(esp, 4)));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));

  // We must preserve the right stack ordering.
  push_tag->must_be_after(push_value);

  // Dump the robocop template.
  rt.dump(1);

  // Dispatch to next byte code
  jmp(Address(no_reg, ebx, times_4, "interpreter_dispatch_table"));
}

void TemplateTable::robo_isub() {
  RobocopTemplate rt(5);

  RobocopInstruction* pop_tag            = rt.add(new_popl   (eax));
  RobocopInstruction* pop_value          = rt.add(new_popl   (eax));
  RobocopInstruction* do_subtraction     = rt.add(new_subl   (Address(esp, 4), eax));
  RobocopInstruction* load_next_bytecode = rt.add(new_movzxb (ebx, Address(esi, 1)));
  RobocopInstruction* increment_bcp      = rt.add(new_incl   (esi));

  // We must preserve the right stack ordering.
  pop_value->must_be_after(pop_tag);

  // We cannot add before we've loaded the value.
  do_subtraction->must_be_after(pop_value);

  // Dump the robocop template.
  rt.dump(3);

  // Dispatch to next byte code
  jmp(Address(no_reg, ebx, times_4, "interpreter_dispatch_table"));
}

#endif // ENABLE_INTERPRETER_GENERATOR
