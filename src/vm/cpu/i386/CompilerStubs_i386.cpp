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

#if ENABLE_INTERPRETER_GENERATOR
#include "incls/_CompilerStubs_i386.cpp.incl"

void CompilerStubs::generate() {
#if ENABLE_COMPILER
  generate_compiler_new_object();
  generate_compiler_new_obj_array();
  generate_compiler_new_type_array();
  generate_compiler_idiv_irem();
#endif
}

void CompilerStubs::generate_compiler_new_object() {
  comment_section("Compiler new object (any size)");
  comment("Register edx holds the instance size, register ebx holds the prototypical near of the instance class");
  Label slow_case;
  entry("compiler_new_object");

  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  comment("Compute new top");
  leal(ecx, Address(eax, edx, times_1));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant(slow_case));
  }

  comment("Compare against _inline_allocation_end");
  cmpl(ecx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant(slow_case));

  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), ecx);

  comment("Set prototypical near in object; no need for write barrier");
  movl(Address(eax), ebx);

  comment("Compute remaining size");
  decrement(edx, oopSize);

  comment("One-word object?");
  Label init_done;
  jcc(zero, Constant(init_done));

  comment("Zero object fields");
  xorl(ecx, ecx);
  Label init_loop;
  bind(init_loop);
  movl(Address(eax, edx, times_1), ecx);
  decrement(edx, oopSize);
  jcc(not_zero, Constant(init_loop));
  bind(init_done);

  comment("The newly allocated object is in register eax");
  ret();

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  leal(eax, Address(Constant("newobject")));
  goto_shared_call_vm(T_OBJECT);

  entry_end(); // compiler_new_object
}

void CompilerStubs::generate_compiler_new_obj_array() {
  comment_section("Compiler stub: new object array");
  comment("- ebx holds the prototypical near of the array class");

  entry("compiler_new_obj_array");
  comment("Get array length");
  // add BytesPerWord for return address
  movl(edx, Address(esp, Constant(BytesPerWord + JavaFrame::arg_offset_from_sp(0))));

  Label slow_case;
  comment("Check if the array length is too large or negative");
  cmpl(edx, Constant(maximum_safe_array_length));
  jcc(above, Constant(slow_case));

  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant(slow_case));
  }

  comment("Compute new top");
  leal(ecx, Address(eax, edx, times_4, Constant(Array::base_offset())));

  comment("Check for overflow");
  cmpl(ecx, eax);
  jcc(below, Constant(slow_case));

  comment("Compare against _inline_allocation_end");
  cmpl(ecx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant(slow_case));

  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), ecx);

  comment("Set prototypical near in object; no need for write barrier");
  movl(Address(eax), ebx);

  comment("Set the length");
  movl(Address(eax, Constant(Array::length_offset())), edx);

  comment("Compute remaining size");
  testl(edx, edx);

  comment("Empty array?");
  Label init_done;
  jcc(equal, Constant(init_done));

  comment("Zero array elements");
  xorl(ecx, ecx);
  Label init_loop;
  bind(init_loop);
  movl(Address(eax, edx, times_4, Constant(Array::base_offset() - oopSize)), ecx);
  decrement(edx, 1);
  jcc(not_zero, Constant(init_loop));
  bind(init_done);

  comment("The newly allocated array is in register eax");
  ret();

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  leal(eax, Address(Constant("anewarray")));
  goto_shared_call_vm(T_ARRAY);

  entry_end(); // compiler_new_obj_array
}

void CompilerStubs::generate_compiler_new_type_array() {
  comment_section("Compiler stub: new type array");
  comment("- ecx holds the array size");
  comment("- ebx holds the prototypical near of the array class");

  entry("compiler_new_type_array");
  comment("Get array length");
  // BytesPerWord added because of the return address
  movl(edx, Address(esp, Constant(BytesPerWord + JavaFrame::arg_offset_from_sp(0))));

  Label slow_case;
  comment("Check if the array length is too large or negative");
  cmpl(edx, Constant(maximum_safe_array_length));
  jcc(above, Constant(slow_case));

  comment("Get _inline_allocation_top");
  movl(eax, Address(Constant("_inline_allocation_top")));

  if (GenerateDebugAssembly) {
    comment("Check ExcessiveGC");
    testl(Address(Constant("ExcessiveGC")), Constant(0));
    jcc(not_zero, Constant(slow_case));
  }

  comment("Compute new top and check for overflow");
  addl(ecx, eax);
  jcc(carry_set, Constant(slow_case)); // if the carry bit is set the addition has overflowed

  comment("Compare against _inline_allocation_end");
  cmpl(ecx, Address(Constant("_inline_allocation_end")));
  jcc(above, Constant(slow_case));

  comment("Allocation succeeded, set _inline_allocation_top");
  movl(Address(Constant("_inline_allocation_top")), ecx);

  comment("Set prototypical near in object; no need for write barrier");
  movl(Address(eax), ebx);

  comment("Set the length");
  movl(Address(eax, Constant(Array::length_offset())), edx);

  comment("Compute remaining size");
  subl(ecx, eax);
  subl(ecx, Constant(Array::base_offset()));

  comment("One-word object?");
  Label init_done;
  jcc(zero, Constant(init_done));

  comment("Zero object fields");
  xorl(edx, edx);
  Label init_loop;
  bind(init_loop);
  movl(Address(eax, ecx, times_1, Constant(Array::base_offset() - oopSize)), edx);
  decrement(ecx, oopSize);
  jcc(not_zero, Constant(init_loop));
  bind(init_done);
  comment("The newly allocated array is in register eax");
  ret();

  comment("Slow case - call the VM runtime system");
  bind(slow_case);
  leal(eax, Address(Constant("newarray")));
  goto_shared_call_vm(T_ARRAY);

  entry_end(); // compiler_new_type_array
}

void CompilerStubs::generate_compiler_idiv_irem() {
  comment_section("Compiler integer divide and remainder");
  comment("Register eax holds the dividend, register ebx holds the divisor");

  entry("compiler_idiv_irem");
  const int min_int = 0x80000000;
  Label normal_case, special_case;

  Label throw_exception;

  testl(ebx,ebx);
  jcc(equal, Constant(throw_exception));

  // Check for special case
  cmpl(eax, Constant(min_int));
  jcc(not_equal, Constant(normal_case));
  xorl(edx, edx); // Prepare edx for possible special case (where remainder = 0)
  cmpl(ebx, Constant(-1));
  jcc(equal, Constant(special_case));

  // Handle normal case
  bind(normal_case);
  cdql();
  idivl(ebx);

  // Normal and special case exit
  bind(special_case);
  ret();

  bind(throw_exception);
  comment("Throw a DivisionByZeroException");
  leal(eax, Address(Constant("division_by_zero_exception")));
  goto_shared_call_vm(T_VOID);

  entry_end(); // compiler_idiv_irem
}

#endif // ENABLE_INTERPRETER_GENERATOR
