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
#include "incls/_NativeGenerator_i386.cpp.incl"

void NativeGenerator::generate() {
  int oldGenerateDebugAssembly = GenerateDebugAssembly;
  GenerateDebugAssembly = false;
  generate_native_math_entries();
  generate_native_string_entries();
  generate_native_system_entries();
  generate_native_thread_entries();
  generate_native_misc_entries();
  GenerateDebugAssembly = oldGenerateDebugAssembly;
}

void NativeGenerator::generate_native_math_entries() {
  comment_section("Native entry points for math functions");
  int offset = 0;

#if ENABLE_FLOAT

  stop_code_segment();
  start_data_segment();

  stop_data_segment();
  start_code_segment();

  // Generate sinus entry.
  offset = 0;
  rom_linkable_entry("native_math_sin_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_sin"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_sin_entry

  // Generate cosinus entry.
  offset = 0;
  rom_linkable_entry("native_math_cos_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_cos"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_cos_entry

  // Generate tangent entry.
  offset = 0;
  rom_linkable_entry("native_math_tan_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_tan"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_tan_entry

  // Generate square root entry.
  offset = 0;
  rom_linkable_entry("native_math_sqrt_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_sqrt"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_sqrt_entry

  // Generate ceil entry.
  offset = 0;
  rom_linkable_entry("native_math_ceil_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_ceil"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_ceil_entry

  // Generate floor entry.
  offset = 0;
  rom_linkable_entry("native_math_floor_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_floor"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_floor_entry

#if ENABLE_CLDC_111

  // Generate asin entry.
  offset = 0;
  rom_linkable_entry("native_math_asin_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_asin"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_asin_entry

  // Generate acos entry.
  offset = 0;
  rom_linkable_entry("native_math_acos_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_acos"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_acos_entry

  // Generate atan entry.
  offset = 0;
  rom_linkable_entry("native_math_atan_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_atan"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_atan_entry

  // Generate atan2 entry.
  offset = 0;
  rom_linkable_entry("native_math_atan2_entry");
  comment("store return address");
  popl(edi);
  pop_double(eax, ecx);
  pop_double(ebx, edx);
  pushl(edx);
  pushl(ebx);
  pushl(ecx);
  pushl(eax);
  call(Constant("jvm_atan2"));
  addl(esp, Constant(8));
  push_from_fpu_stack(double_tag, offset, true);
  jmp(edi);
  rom_linkable_entry_end(); // native_math_atan2_entry

#endif /* ENABLE_CLDC_111 */

#endif /* ENABLE_FLOAT */
}

void NativeGenerator::generate_native_string_entries() {

  comment_section("Native entry points for string functions");
  {

    //--------------------java.lang.String.indexof0---------------------------
    rom_linkable_entry("native_string_indexof0_entry");

    wtk_profile_quick_call(/* param_size*/ 2);

    comment("Pop the return address");
    popl(edi);
    comment("Push zero for fromIndex");
    pushl(Constant(0));
    comment("Push back the return address");
    pushl(edi);

    jmp(Constant("native_string_indexof_entry"));
    rom_linkable_entry_end(); // native_string_indexof0_entry

    //--------------------java.lang.String.indexof---------------------------

    rom_linkable_entry("native_string_indexof_entry");
    Label cont, loop, test, failure, success;

    wtk_profile_quick_call(/* param_size*/ 3);

    comment("Pop the return address");
    popl(edi);

    comment("Pop the argument: fromIndex");
    pop_int(eax, eax);

    comment("Pop the argument: ch");
    pop_int(ebx, ebx);

    comment("Pop the receiver");
    pop_obj(ecx, ecx);

    cmpl(ebx, Constant(0xFFFF));
    jcc(greater, Constant(failure));

    cmpl(eax, Constant(0));
    jcc(greater_equal, Constant(cont));
    movl(eax, Constant(0));

    bind(cont);
    movl(esi, Address(ecx, Constant(String::count_offset())));

    comment("if (fromIndex >= count) { return -1; }");
    cmpl(eax, esi);
    jcc(greater_equal, Constant(failure));

    movl(edx, Address(ecx, Constant(String::offset_offset())));
    addl(eax, edx); // i = offset + fromIndex
    addl(edx, esi); // int max = offset + count;
    movl(esi, Address(ecx, Constant(String::value_offset())));    // v = value.
    jmp(Constant(test));

    bind(loop);
    cmpw(Address(esi, eax, times_2, Constant(Array::base_offset())),  ebx);
    jcc(equal, Constant(success));
    incl(eax);

    bind(test);
    cmpl(eax, edx);
    jcc(less, Constant(loop));

    comment("Return -1 by pushing the value and jumping to the return address");
    bind(failure);
    push_int(-1);
    jmp(edi);

    comment("Return i - offset by pushing the value and jumping to the return address");
    bind(success);
    movl(esi, Address(ecx, Constant(String::offset_offset())));   // i = offset + fromIndex
    subl(eax, esi);
    push_int(eax);
    jmp(edi);

    rom_linkable_entry_end(); // native_string_indexof_entry
  }

  //----------------------java.lang.String.charAt---------------------------

  {
    rom_linkable_entry("native_string_charAt_entry");
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }
    rom_linkable_entry_end();
  }

  //----------------------java.lang.String(java.lang.StringBuffer)-------------

  {
    rom_linkable_entry("native_string_init_entry");
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }
    rom_linkable_entry_end();
  }

  //----------------------java.lang.String.equals(java.lang.Object)------------

  {
    rom_linkable_entry("native_string_equals_entry");
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }
    rom_linkable_entry_end();
  }

  //----------------------java.lang.String.indexOf(java.lang.String)-----------

  {
    rom_linkable_entry("native_string_indexof0_string_entry");
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }
    rom_linkable_entry_end();
  }

  //----------------------java.lang.String.indexOf(java.lang.String)-----------

  {
    rom_linkable_entry("native_string_indexof_string_entry");
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }
    rom_linkable_entry_end();
  }

  //----------------------java.lang.String.compareTo---------------------------

  { // java.lang.String.compareTo
    // Method int compareTo(java.lang.String)

    rom_linkable_entry("native_string_compareTo_entry");

    wtk_profile_quick_call(/* param_size*/ 2);

    comment("preserve method");
    pushl(ebx);

    // 8 is return address plus pushed method
    int  str1_offset =  JavaFrame::arg_offset_from_sp(0) + 8,
         str0_offset =  JavaFrame::arg_offset_from_sp(1) + 8;

    comment("load arguments to registers");
    movl(ecx, Address(esp, Constant(str1_offset)));
    movl(eax, Address(esp, Constant(str0_offset)));

    // eax: str0: this String
    // ebx: str1: String to compare against

    Label bailout;

    comment("Null check");
    testl(ecx, ecx);
    jcc(zero, Constant(bailout));

    comment("get str0.value[]");
    movl(esi, Address(eax, Constant(String::value_offset())));
    comment("get str0.offset");
    movl(ebx, Address(eax, Constant(String::offset_offset())));
    comment("compute start of character data");
    leal(esi, Address(esi, ebx, times_2, Constant(Array::base_offset())));
    comment("get str0.count");
    movl(eax, Address(eax, Constant(String::count_offset())));

    comment("get str1.value[]");
    movl(edi, Address(ecx, Constant(String::value_offset())));
    comment("get str1.offset");
    movl(ebx, Address(ecx, Constant(String::offset_offset())));
    comment("compute start of character data");
    leal(edi, Address(edi, ebx, times_2, Constant(Array::base_offset())));
    comment("get str1.count");
    movl(ebx, Address(ecx, Constant(String::count_offset())));

    // esi = str0 start of character data
    // edi = str1 start of character data
    // eax = str0 length
    // ebx = str1 length

    Label str1_longest;
    subl(eax, ebx);
    jcc(greater_equal, Constant(str1_longest));
    // str1 is longer than str0
    addl(ebx, eax);
    bind(str1_longest);

    // esi = str0 start of character data
    // edi = str1 start of character data
    // eax = str0.count - str1.count
    // ebx = min(str0.count, str1.count)

    // save str0.count - str1.count, we might need it later
    pushl(eax);

    xorl(ecx, ecx);

    Label loop, check_lengths, done;
    bind(loop);
    cmpl(ecx, ebx);
    jcc(above_equal, Constant(check_lengths));
    movzxw(eax, Address(esi, ecx, times_2));
    movzxw(edx, Address(edi, ecx, times_2));
    subl(eax, edx);
    jcc(not_equal, Constant(done));
    incl(ecx);
    jmp(Constant(loop));

    bind(check_lengths);
    movl(eax, Address(esp));

    bind(done);
    popl(ebx); // remove saved length difference

    // Push result on stack and return to caller
    popl(ebx);     // remove method
    popl(edi);     // pop return address
    addl(esp, Constant(2 * BytesPerStackElement)); // remove arguments
    push_int(eax); // push result
    jmp(edi);      // return

    comment("Bail out to the general compareTo implementation");
    bind(bailout);
    comment("pop method");
    popl(ebx);
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }

    rom_linkable_entry_end(); // native_string_compareTo_entry
  }

  //----------------------java.lang.String.endsWith----------------

  {
    // java.lang.String.endsWith
    // Method boolean endsWith(java.lang.String)

    rom_linkable_entry("native_string_endsWith_entry");

    wtk_profile_quick_call(/* param_size*/ 2);

    Label bailout;

    // 4 is return address
    int suffix_offset =  JavaFrame::arg_offset_from_sp(0) + 4,
        this_offset =  JavaFrame::arg_offset_from_sp(1) + 4;

    comment("load arguments to registers");
    movl(eax, Address(esp, Constant(suffix_offset)));
    cmpl(eax, Constant(0));
    jcc(equal, Constant(bailout));

    movl(ecx, Address(esp, Constant(this_offset)));

    comment("Pop the return address");
    popl(edi);

    movl(edx, Address(ecx, Constant(String::count_offset())));
    subl(edx, Address(eax, Constant(String::count_offset())));

    comment("Push (this.count - suffix.count) for toffset");
    pushl(edx);

    comment("Push back the return address");
    pushl(edi);

    jmp(Constant("native_string_startsWith_entry"));

    comment("Bail out to the general startsWith implementation");
    bind(bailout);
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }

    rom_linkable_entry_end(); // native_string_endsWith_entry
  }

  //----------------------java.lang.String.startsWith----------------
  {
    // java.lang.String.startsWith
    // Method boolean startsWith(java.lang.String)
    rom_linkable_entry("native_string_startsWith0_entry");

    wtk_profile_quick_call(/* param_size*/ 2);
    Label bailout;

    // 4 is return address
    int prefix_offset = JavaFrame::arg_offset_from_sp(0) + 4;

    comment("Check if prefix is null");
    cmpl(Address(esp, Constant(prefix_offset)), Constant(0));
    jcc(equal, Constant(bailout));

    comment("Pop the return address");
    popl(edi);
    comment("Push zero for toffset");
    pushl(Constant(0));
    comment("Push back the return address");
    pushl(edi);

    jmp(Constant("native_string_startsWith_entry"));

    comment("Bail out to the general startsWith implementation");
    bind(bailout);
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }

    rom_linkable_entry_end(); // native_string_startsWith0_entry
  }

  {
    // ----------- java.lang.String.startsWith ------------------------------
    // Method boolean startsWith(java.lang.String,int)

    rom_linkable_entry("native_string_startsWith_entry");

    wtk_profile_quick_call(/* param_size*/ 3);

    Label bailout, return_false;

    // 4 is return address
    int  prefix_offset =  JavaFrame::arg_offset_from_sp(1) + 4;

    comment("Check if prefix is null");
    cmpl(Address(esp, Constant(prefix_offset)), Constant(0));
    jcc(equal, Constant(bailout));

    comment("Pop the return address");
    popl(edi);

    comment("Pop the argument: toffset");
    pop_int(edx, edx);

    comment("Pop the argument: prefix");
    pop_obj(eax, eax);

    comment("Pop the receiver");
    pop_obj(ecx, ecx);

    comment("Preserve the return address");
    pushl(edi);

    // ecx: this String
    // eax: prefix

    cmpl(edx, Constant(0));
    jcc(less, Constant(return_false));

    comment("if (toffset > this.count - prefix.count) return false;");
    movl(ebx, Address(ecx, Constant(String::count_offset())));
    subl(ebx, Address(eax, Constant(String::count_offset())));
    cmpl(edx, ebx);
    jcc(greater, Constant(return_false));

    comment("get this.value[]");
    movl(esi, Address(ecx, Constant(String::value_offset())));
    comment("get this.offset");
    movl(ebx, Address(ecx, Constant(String::offset_offset())));
    comment("add toffset");
    addl(ebx, edx);
    comment("compute start of character data");
    leal(esi, Address(esi, ebx, times_2, Constant(Array::base_offset())));

    comment("get prefix.value[]");
    movl(edi, Address(eax, Constant(String::value_offset())));
    comment("get prefix.offset");
    movl(ebx, Address(eax, Constant(String::offset_offset())));
    comment("compute start of character data");
    leal(edi, Address(edi, ebx, times_2, Constant(Array::base_offset())));

    comment("get prefix.count");
    movl(ecx, Address(eax, Constant(String::count_offset())));
    comment("get the number of bytes to compare");
    shll(ecx, Constant(1));

    comment("memcmp(edi, esi, ecx);");
    pushl(ecx);
    pushl(esi);
    pushl(edi);

    if (GenerateInlineAsm) {
      // VC++ treats memcmp() as an intrinsic function and would cause
      // reference to memcmp in Interpreter_i386.c to fail to compile.
      call(Constant("memcmp_from_interpreter"));
    } else {
      call(Constant("memcmp"));
    }
    addl(esp, Constant(12));
    cmpl(eax, Constant(0));
    jcc(not_equal, Constant(return_false));

    // Push 1 on stack and return to caller
    popl(edi);     // pop return address
    push_int(1);   // push result
    jmp(edi);      // return

    bind(return_false);
    // Push 0 on stack and return to caller
    popl(edi);     // pop return address
    push_int(0);   // push result
    jmp(edi);      // return

    comment("Bail out to the general startsWith implementation");
    bind(bailout);
    if (AddExternCUnderscore) {
      emit_instruction("jmp _interpreter_method_entry");
    } else {
      emit_instruction("jmp  interpreter_method_entry");
    }

    rom_linkable_entry_end(); // native_string_startsWith_entry
  }
}

void NativeGenerator::generate_native_system_entries() {
  comment_section("Native entry points for system functions");

  rom_linkable_entry("native_jvm_unchecked_byte_arraycopy_entry");
  jmp(Constant("native_system_arraycopy_entry"));
  rom_linkable_entry_end();

  rom_linkable_entry("native_jvm_unchecked_char_arraycopy_entry");
  jmp(Constant("native_system_arraycopy_entry"));
  rom_linkable_entry_end();

  rom_linkable_entry("native_jvm_unchecked_int_arraycopy_entry");
  jmp(Constant("native_system_arraycopy_entry"));
  rom_linkable_entry_end();

  rom_linkable_entry("native_jvm_unchecked_long_arraycopy_entry");
  jmp(Constant("native_system_arraycopy_entry"));
  rom_linkable_entry_end();

  rom_linkable_entry("native_jvm_unchecked_obj_arraycopy_entry");
  jmp(Constant("native_system_arraycopy_entry"));
  rom_linkable_entry_end();

  rom_linkable_entry("native_system_arraycopy_entry");

  wtk_profile_quick_call(/* param_size*/ 5);

  Label bailout, cont, try_2_byte, try_4_byte, try_8_byte, do_4_byte;

  //  public static native void arraycopy(Object src, int src_position,
  //                                      Object dst, int dst_position,
  //                                      int length);
  comment("preserve method");
  pushl(ebx);

  // 8 is for the preserved method and the return address
  int  length_offset  =  JavaFrame::arg_offset_from_sp(0) + 8,
       dst_pos_offset =  JavaFrame::arg_offset_from_sp(1) + 8,
       dst_offset     =  JavaFrame::arg_offset_from_sp(2) + 8,
       src_pos_offset =  JavaFrame::arg_offset_from_sp(3) + 8,
       src_offset     =  JavaFrame::arg_offset_from_sp(4) + 8;

  comment("load arguments to registers");
  movl(ecx, Address(esp, Constant(length_offset)));
  movl(edi, Address(esp, Constant(dst_pos_offset)));
  movl(edx, Address(esp, Constant(dst_offset)));
  movl(esi, Address(esp, Constant(src_pos_offset)));
  movl(eax, Address(esp, Constant(src_offset)));

  // eax = src
  // ebx = tmp register
  // edx = dst
  // ecx = length
  // esi = src_pos
  // edi = dst_pos

  comment("if (src == NULL) goto bailout;");
  testl( eax, eax );
  jcc(zero, Constant(bailout));

  comment("if (dst == NULL) goto bailout;");
  testl( edx, edx );
  jcc(zero, Constant(bailout));

  comment("if (length < 0 || src_pos < 0 || dst_pos < 0) goto bailout;");
  movl(ebx, ecx);
  orl(ebx, esi);
  orl(ebx, edi);
  jcc(negative, Constant(bailout));

  comment("if ((unsigned int) dst.length < (unsigned int) dst_pos + (unsigned int) length) goto bailout;");
  movl(ebx, ecx);
  addl(ebx, edi);
  cmpl(Address(edx, Constant(Array::length_offset())), ebx);
  jcc(below, Constant(bailout));

  comment("if ((unsigned int) src.length < (unsigned int) src_pos + (unsigned int) length) goto bailout;");
  movl(ebx, ecx);
  addl(ebx, esi);
  cmpl(Address(eax, Constant(Array::length_offset())), ebx);
  jcc(below, Constant(bailout));

  comment("Same near test");
  comment("if (src.near != dst.near) goto bailout;");
  movl(ebx, Address(eax, Constant(Oop::klass_offset())));
  cmpl(ebx, Address(edx, Constant(Oop::klass_offset())));
  jcc(not_equal, Constant(bailout));

  comment("load the instance_size");
  movl(ebx, Address(ebx, Constant(JavaNear::klass_offset())));
  movsxw(ebx, Address(ebx, Constant(FarClass::instance_size_offset())));

  comment("if (instance_size != size_type_array_1()) goto try_2_byte");
  cmpl(ebx, Constant(InstanceSize::size_type_array_1));
  jcc(not_equal, Constant(try_2_byte));
  leal(esi, Address(eax, esi, times_1, Constant(Array::base_offset())));
  leal(edi, Address(edx, edi, times_1, Constant(Array::base_offset())));
  jmp(Constant(cont));

  bind(try_2_byte);
  comment("if (instance_size != size_type_array_2()) goto try_4_byte");
  cmpl(ebx, Constant(InstanceSize::size_type_array_2));
  jcc(not_equal, Constant(try_4_byte));
  leal(esi, Address(eax, esi, times_2, Constant(Array::base_offset())));
  leal(edi, Address(edx, edi, times_2, Constant(Array::base_offset())));
  shll(ecx, Constant(1));
  jmp(Constant(cont));

  bind(try_4_byte);
  comment("if (instance_size == size_type_array_4()) goto do_4_byte");
  cmpl(ebx, Constant(InstanceSize::size_type_array_4));
  jcc(equal, Constant(do_4_byte) );

  comment("if (instance_size != size_obj_array()) goto bailout");
  cmpl(ebx, Constant(InstanceSize::size_obj_array));
  jcc(not_equal, Constant(bailout));

  comment("if (dst < old_generation_end) goto bailout");
  cmpl( edx, Address( Constant( "_old_generation_end" ) ) );
  jcc( below, Constant(bailout));

  bind(do_4_byte);
  leal(esi, Address(eax, esi, times_4, Constant(Array::base_offset())));
  leal(edi, Address(edx, edi, times_4, Constant(Array::base_offset())));
  shll(ecx, Constant(2));

  bind(cont);
  comment("memmove(edi, esi, ecx);");
  pushl(ecx);
  pushl(esi);
  pushl(edi);
  call(Constant("memmove"));
  addl(esp, Constant(16));

  ret(Constant(5 * BytesPerStackElement));

  comment("Bail out to the general arraycopy implementation");
  bind(bailout);
  comment("pop method");
  popl(ebx);

  if (AddExternCUnderscore) {
    emit_instruction("jmp _interpreter_method_entry");
  } else {
    emit_instruction("jmp  interpreter_method_entry");
  }

  rom_linkable_entry_end(); // native_system_arraycopy_entry
}

void NativeGenerator::generate_native_thread_entries() {
  comment_section("Native entry points for thread functions");
  // rom_linkable_entry("native_thread_enterLockObject_entry");
  // jmp(Constant("Java_com_sun_cldchi_jvm_Thread_enterLockObject"));
}

#define MAKE_DUMMY_NATIVE_ENTRY(x) \
  { \
    rom_linkable_entry(#x); \
    if (AddExternCUnderscore) { \
      emit_instruction("jmp _interpreter_method_entry"); \
    } else { \
      emit_instruction("jmp  interpreter_method_entry"); \
    } \
    rom_linkable_entry_end(); \
  }

void NativeGenerator::generate_native_misc_entries() {
  MAKE_DUMMY_NATIVE_ENTRY(native_vector_elementAt_entry)
  MAKE_DUMMY_NATIVE_ENTRY(native_vector_addElement_entry)
  MAKE_DUMMY_NATIVE_ENTRY(native_stringbuffer_append_entry)
  MAKE_DUMMY_NATIVE_ENTRY(native_string_substringI_entry)
  MAKE_DUMMY_NATIVE_ENTRY(native_string_substringII_entry)
  MAKE_DUMMY_NATIVE_ENTRY(native_integer_toString_entry)
}

#endif // ENABLE_INTERPRETER_GENERATOR
