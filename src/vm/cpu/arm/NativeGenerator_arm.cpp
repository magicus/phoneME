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

#if ENABLE_INTERPRETER_GENERATOR && !ENABLE_THUMB_COMPILER
#ifndef PRODUCT
#include "incls/_NativeGenerator_arm.cpp.incl"


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
  Address4 LowTarget  = TaggedJavaStack ? set(r0, r2) : set(r0);
  Address4 HighTarget = TaggedJavaStack ? set(r1, r2) : set(r1);

  // Where the first item popped off the stack goes
  Address4 A = JavaStackDirection < 0 ?  LowTarget  : HighTarget;
  // Where the second item popped off the stack goes
  Address4 B = JavaStackDirection < 0 ?  HighTarget : LowTarget;
  
  Segment seg(this, code_segment, "Native entry points for math functions");

  const Register return_reg = r7;

#if ENABLE_FLOAT

  Label label_sin("jvm_sin");         import(label_sin);
  Label label_cos("jvm_cos");         import(label_cos);
  Label label_tan("jvm_tan");         import(label_tan);
  Label label_sqrt("jvm_sqrt");       import(label_sqrt);
  Label label_floor("jvm_floor");     import(label_floor);
  Label label_ceil("jvm_ceil");       import(label_ceil);

  // Generate sine entry.
  bind_rom_linkable("native_math_sin_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_sin);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate cos entry.
  bind_rom_linkable("native_math_cos_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_cos);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate tan entry.
  bind_rom_linkable("native_math_tan_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_tan);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate square root entry.
  bind_rom_linkable("native_math_sqrt_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_sqrt);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate ceil entry.
  bind_rom_linkable("native_math_ceil_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_ceil);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate floor entry.
  bind_rom_linkable("native_math_floor_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_floor);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

#if ENABLE_CLDC_111

  Label label_asin("jvm_asin");       import(label_asin);
  Label label_acos("jvm_acos");       import(label_acos);
  Label label_atan("jvm_atan");       import(label_atan);
  Label label_atan2("jvm_atan2");     import(label_atan2);

  // Generate asin entry.
  bind_rom_linkable("native_math_asin_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_asin);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate acos entry.
  bind_rom_linkable("native_math_acos_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_acos);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate atan entry.
  bind_rom_linkable("native_math_atan_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  bl(label_atan);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

  // Generate atan2 entry.
  bind_rom_linkable("native_math_atan2_entry");
  Address4 args = set(r0, r1, r2, r3);
  pop(args);
  mov(return_reg, reg(lr));
  bl(label_atan2);
  set_return_type(T_DOUBLE);
  jmpx(return_reg);

#endif // ENABLE_CLDC_111

#endif // ENABLE_FLOAT
}

// Note: this routine can be more optimized
void NativeGenerator::generate_native_string_entries() {
  const Register return_reg = r7;

  //----------------------java.lang.String.charAt---------------------------
  {
    Segment seg(this, code_segment, "java.lang.String.charAt");
    Register string     = tmp0;
    Register index      = tmp1;
    Register count      = tmp2;
    Register array      = tmp3;
    Register offset     = tmp4;
    Label bailout;

  bind_rom_linkable("native_string_charAt_entry");
    wtk_profile_quick_call(/* param size */2);

    comment("r%d = string", string);
    ldr(string, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

    comment("r%d = index", index);
    ldr(index, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

    comment("r%d = count", count);
    ldr(count, imm_index(string, String::count_offset()));

    comment("r%d = array", array);
    ldr(array, imm_index(string, String::value_offset()));

    comment("r%d = offset", offset);
    ldr(offset, imm_index(string, String::offset_offset()));

    comment("if ((unsigned int) index >= (unsigned int) count) goto bailout;");
    cmp(index, reg(count));
    b(bailout, hs);

    comment("return: r%d = array[index + offset]", tos_val);
    add(index, index, reg(offset));
    add(array, array, imm_shift(index, lsl, 1));
    ldrh(tos_val, imm_index3(array, Array::base_offset()));
    set_return_type(T_INT);

    comment("remove arguments from the stack");
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);

    jmpx(lr);

  bind(bailout);
    comment("Bail out to the general charAt() implementation");
    b("interpreter_method_entry");
  }

  //----------------------java.lang.String(StringBuffer)-----------------------
  {
    Segment seg(this, code_segment, "java.lang.String(java.lang.StringBuffer)");
    Register buffer = JavaStackDirection < 0 ? tos_tag : tmp0;
    Register string = JavaStackDirection < 0 ? tmp0 : tos_tag;
    Register buffer_near   = tmp1;
    Register buffer_value  = tmp2;
    Register buffer_lock   = tmp3;
    Register buffer_count  = tmp4;
    Register buffer_offset = buffer_lock;
    Register buffer_shared = tmp5;
    Address4 init_args = JavaStackDirection < 0 ? 
      set(buffer, string) : set(string, buffer);

    const int value_field_offset = Instance::header_size();
    const int count_field_offset = value_field_offset + sizeof(jobject);
    const int shared_field_offset = count_field_offset + sizeof(jint);

    Label bailout;

  bind_rom_linkable("native_string_init_entry");
    wtk_profile_quick_call(/* param size */2);

    comment("Get string, buffer from stack");
    pop(init_args);

    comment("Receiver or str are null?");
    tst(string, reg(buffer));
    b(bailout, eq);

    if (Oop::klass_offset() == 0 && 
        Instance::header_size() == BytesPerWord) {

      Address4 buffer_fields = set(buffer_near, buffer_value, buffer_count);

      ldmia(buffer, buffer_fields, no_writeback);

    } else {
      ldr(buffer_near,  imm_index(buffer, Oop::klass_offset()));
      ldr(buffer_value, imm_index(buffer, value_field_offset));
      ldr(buffer_count, imm_index(buffer, count_field_offset));
    }

    ldr(buffer_lock,  imm_index(buffer_near, JavaNear::raw_value_offset()));
    mov(buffer_shared, one); 

    comment("Check the buffer lock bit.");
    andr(buffer_lock, buffer_lock, one, set_CC);
    b(bailout, ne);

    comment("Note: r%d is zero at this point", buffer_offset);
    strb(buffer_shared, imm_index(buffer, shared_field_offset));

    if (String::value_offset()  == BytesPerWord &&
        String::offset_offset() == String::value_offset() + BytesPerWord &&
        String::count_offset()  == String::offset_offset() + BytesPerWord) {

      Address4 string_fields = set(buffer_value, buffer_offset, buffer_count);

      stmib(string, string_fields, no_writeback);
    } else {
      str(buffer_value,   imm_index(string, String::value_offset()));
      str(buffer_offset,  imm_index(string, String::offset_offset()));
      str(buffer_count,   imm_index(string, String::count_offset()));
    }

    set_return_type(T_VOID);
    jmpx(lr);

  bind(bailout);
    comment("Bail out to the general <init>() implementation");
    push(init_args);
    b("interpreter_method_entry");
  }

  //----------------------java.lang.String.equals(java.lang.Object)---------

  {
    Segment seg(this, code_segment, "java.lang.String.equals(java.lang.Object)");
    Label aligned_loop, unaligned_loop;
    Label aligned, unaligned;
    Label small_string, error;
    Label done_true, done_false;
    // java.lang.String.equals
    // Method boolean equals(java.lang.String)
    bind_rom_linkable("native_string_equals_entry");
  
    wtk_profile_quick_call(/* param size */ 2);

    comment("get strings to compare");
    pop(tmp2);   // get first parameter;
    pop(tmp0);   // get receiver

    Register str0       = tmp0;
    Register str0_count = tmp4;
    Register str0_charp = tmp1;
    
    Register str1       = tmp2;
    Register str1_count = tmp5;
    Register str1_charp = tmp3;

    Register offset0    = str0;
    Register offset1    = str1;

    Register value0     = str0;
    Register value1     = str1;

    Register str0_class = str0_count;
    Register str1_class = str1_count;

    Register junk       = tmp5;
    Register result     = tos_val;

    Register junk0      = tos_val;
    Register junk1      = tos_tag;

    // Warning:  We cannot bash tos_val until after we know we don't have 
    // an errors, since it contains the method
    comment("Null receiver?");
    cmp(str0, zero);
    b(error, eq);

    comment("Identity?");
    cmp(str0, reg(str1));
    b(done_true, eq);

    comment("Null argument?");
    cmp(str1, zero);

    comment("Get str0.class, str1.class");
    ldr(str0_class, imm_index(str0, Oop::klass_offset()), ne);
    ldr(str1_class, imm_index(str1, Oop::klass_offset()), ne);

    b(done_false, eq);

    cmp(str0_class, reg(str1_class));

    comment("Get str0.blueprint, str1.blueprint");
    ldr(str0_class, imm_index(str0_class, Oop::klass_offset()), ne);
    ldr(str1_class, imm_index(str1_class, Oop::klass_offset()), ne);

    comment("Different classes?");
    cmp(str0_class, reg(str1_class), ne);

    comment("Get str0.count, str1.count");
    ldr(str0_count, imm_index(str0, String::count_offset()), eq);
    ldr(str1_count, imm_index(str1, String::count_offset()), eq);

    comment("Different length?");
    cmp(str0_count, reg(str1_count), eq);

    b(done_false, ne);

    comment("Get str0.value[], str1.value[]");
    ldr(str0_charp, imm_index(str0, String::value_offset()));
    ldr(str1_charp, imm_index(str1, String::value_offset()));

    comment("Get str0.offset, str1.offset");
    ldr(offset0, imm_index(str0, String::offset_offset()));
    ldr(offset1, imm_index(str1, String::offset_offset()));
    
    comment("Compute start of character data");
    add(str0_charp, str0_charp, imm(Array::base_offset()));
    add(str1_charp, str1_charp, imm(Array::base_offset()));
    add(str0_charp, str0_charp, imm_shift(offset0, lsl, LogBytesPerShort));
    add(str1_charp, str1_charp, imm_shift(offset1, lsl, LogBytesPerShort));
    
    comment("Small string?");
    cmp(str0_count, imm(2));
    b(small_string, le);

    comment("Both strings are aligned?");
    orr(junk, str0_charp, reg(str1_charp));
    tst(junk, imm(3));
    comment("Unaligned strings are uncommon - go out of the fast path ASAP");
    b(unaligned, ne);

  bind(aligned);
    comment("Get number of words");
    mov(str0_count, imm_shift(str0_count, lsr, 
                              (LogBytesPerWord - LogBytesPerShort)), set_CC);

    comment("Count word-aligned?");
    ldr(value0, imm_index(str0_charp, BytesPerWord, post_indexed), cc);
    ldr(value1, imm_index(str1_charp, BytesPerWord, post_indexed), cc);
    b(aligned_loop, cc);

    comment("Get offset of the last char");
    mov(str1_count, imm_shift(str0_count, lsl, LogBytesPerWord));
    comment("Compare last char");
    ldrh(value0, add_index3(str0_charp, str1_count));
    ldrh(value1, add_index3(str1_charp, str1_count));
    cmp(value0, reg(value1));
    ldr(value0, imm_index(str0_charp, BytesPerWord, post_indexed), eq);
    ldr(value1, imm_index(str1_charp, BytesPerWord, post_indexed), eq);
    b(done_false, ne);

  bind(aligned_loop);
    cmp(value0, reg(value1));
    b(done_false, ne);
    sub(str0_count, str0_count, one, set_CC);
    ldr(value0, imm_index(str0_charp, BytesPerWord, post_indexed), ne);
    ldr(value1, imm_index(str1_charp, BytesPerWord, post_indexed), ne);
    b(done_true, eq);
    cmp(value0, reg(value1));
    b(done_false, ne);
    sub(str0_count, str0_count, one, set_CC);
    ldr(value0, imm_index(str0_charp, BytesPerWord, post_indexed), ne);
    ldr(value1, imm_index(str1_charp, BytesPerWord, post_indexed), ne);
    b(aligned_loop, ne);

  bind(done_true);
    comment("Return true");
    mov(result, one);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);

  bind(unaligned);
    comment("Check relative alignment");
    eor(junk, str0_charp, reg(str1_charp));
    tst(junk, imm(3));
    b(unaligned_loop, ne);

    comment("Strings are both unaligned");
    comment("Compare first char and become word-aligned");
    ldrh(value0, imm_index3(str0_charp, BytesPerShort, post_indexed));
    ldrh(value1, imm_index3(str1_charp, BytesPerShort, post_indexed));
    cmp(value0, reg(value1));
    sub(str0_count, str0_count, one, eq);    
    b(aligned, eq);

  bind(done_false);
    comment("Return false");
    mov(result, zero);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);

  bind(small_string);
    comment("Empty string?");
    cmp(str0_count, zero);
    b(done_true, eq);

  bind(unaligned_loop);
    comment("Char-aligned loop");
    ldrh(value0, imm_index3(str0_charp, BytesPerShort, post_indexed));
    ldrh(value1, imm_index3(str1_charp, BytesPerShort, post_indexed));
    cmp(value0, reg(value1));
    b(done_false, ne);
    sub(str0_count, str0_count, one, set_CC);
    b(unaligned_loop, ne);

    comment("Return true");
    mov(result, one);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);

  bind(error);
    comment("We have some sort of error");
    push(tmp0);
    push(tmp2);
    b("interpreter_method_entry");
  }

  //----------------------java.lang.String.indexOf(java.lang.String)-------------------

  { 
    Segment seg(this, code_segment, "java.lang.String.indexOf(java.lang.String)");

    Label native_string_indexof0_string_continue;
    Label bailout, large_from_index, return_from_index;
    Label search_for_first_char_entry, search_for_first_char_loop;
    Label found_first_char;
    Label check_substring_entry, check_substring_loop;

    Register result     = tos_val;
    Register from_index = JavaStackDirection < 0 ? tos_tag : tmp1;
    Register str1       = tmp0;
    Register str0       = JavaStackDirection < 0 ? tmp1 : tos_tag;
    Register str0_count = tmp2;
    Register str1_count = tmp3;
    Register offset0    = str0;
    Register offset1    = str1;
    Register str0_charp = tmp4;
    Register str1_charp = tmp5;
    Register str0_min_charp  = from_index;
    Register str0_max_charp  = tos_val;
    Register str1_end_charp = str1_count;
    Register first_char = str0_count;

    Address4 indexof0_args = JavaStackDirection < 0 ? 
      set(str1, str0) : set(str0, str1);
    Address4 indexof_args = JavaStackDirection < 0 ? 
      set(from_index, str1, str0) : set(str0, str1, from_index);

    GUARANTEE((str1 - str0) * JavaStackDirection > 0, 
              "Bad register assignment");
    GUARANTEE((from_index - str1) * JavaStackDirection > 0, 
              "Bad register assignment");

    // Method boolean java.lang.String.indexOf(java.lang.String)
  bind_rom_linkable("native_string_indexof0_string_entry");
  
    wtk_profile_quick_call(/* param size */ 2);

    comment("Get receiver, str from stack");
    pop(indexof0_args);

    comment("Set fromIndex to zero");
    mov(from_index, zero);
    
    comment("Neither receiver nor str are null?");
    tst(str0, reg(str1));
    b(native_string_indexof0_string_continue, ne);

    comment("Bailout to interpreter");
    push(indexof0_args);
    b("interpreter_method_entry");

    // Method boolean java.lang.String.indexOf(java.lang.String, fromIndex)
  bind_rom_linkable("native_string_indexof_string_entry");
  
    wtk_profile_quick_call(/* param size */ 3);

    comment("Get receiver, str, fromIndex from stack");
    pop(indexof_args);

    comment("if (fromIndex < 0) { fromIndex = 0; }");
    cmp(from_index, zero);
    mov(from_index, zero, lt);

    comment("Receiver or str are null?");
    tst(str0, reg(str1));
    b(bailout, eq);
    
  bind(native_string_indexof0_string_continue);
    
    comment("Get this.count, str.count");
    ldr(str0_count, imm_index(str0, String::count_offset()));
    ldr(str1_count, imm_index(str1, String::count_offset()));
      
    comment("fromIndex >= this.count ? - uncommon case");
    cmp(from_index, reg(str0_count));
    b(large_from_index, ge);

    comment("if (str.count <= 0) { return fromIndex; }");
    cmp(str1_count, zero);
    b(return_from_index, eq);

    comment("Get this.value[], str.value[]");
    ldr(str0_charp, imm_index(str0, String::value_offset()));
    ldr(str1_charp, imm_index(str1, String::value_offset()));

    comment("Get this.offset, str.offset");
    ldr(offset0, imm_index(str0, String::offset_offset()));
    ldr(offset1, imm_index(str1, String::offset_offset()));
    
    comment("Compute start of character data");
    add(str0_charp, str0_charp, imm(Array::base_offset()));
    add(str1_charp, str1_charp, imm(Array::base_offset()));
    add(str0_charp, str0_charp, imm_shift(offset0, lsl, LogBytesPerShort));
    add(str1_charp, str1_charp, imm_shift(offset1, lsl, LogBytesPerShort));

    {
      Register count_diff = str0_max_charp;
      comment("Compute max pointer to start of a substring = ");
      comment("    = this.value + this.offset + this.count - str.count");
      sub(count_diff, str0_count, reg(str1_count));
      add(str0_max_charp, str0_charp, imm_shift(count_diff, lsl, LogBytesPerShort));
    }

    comment("Compute start of a substring = ");
    comment("    = this.value + this.offset + fromIndex");
    add(str0_min_charp, str0_charp, imm_shift(from_index, lsl, LogBytesPerShort));

    comment("Compute pointer after the end of substring = ");
    comment("    = str.value + str.offset + str.count * 2");
    add(str1_end_charp, str1_charp, 
        imm_shift(str1_count, lsl, LogBytesPerShort));
    
    comment("Load first character of a substring"); 
    comment(" and increment pointer to substring start");
    ldrh(first_char, imm_index3(str1_charp, BytesPerShort, post_indexed));

    {
      Register str0_char  = gp;
      Register str1_char  = jsp;
      Register str0_cur_charp = locals;
      Register str1_cur_charp = lr;
      Address4 save_set = set(gp, jsp, locals, lr);
      // If register set for ldm contains pc it performs jump without mode 
      // switching.  We should avoid this and use dx to switch from ARM to 
      // Thumb mode for Thumb VM here.
#if ENABLE_THUMB_VM
      Address4 restore_set = set(gp, jsp, locals, tmp4);
#else
      Address4 restore_set = set(gp, jsp, locals, pc);
#endif

      stmfd(sp, save_set, writeback);

      b(search_for_first_char_entry);

    bind(large_from_index);
      comment("if (fromIndex >= count) {");
      comment("    if (count == 0 && fromIndex == 0 && str.count == 0) {");
      comment("        return 0;");
      comment("    }");
      comment("    return -1;");
      comment("}");
      orr(from_index, from_index, reg(str1_count), set_CC);
      mov_imm(from_index, -1, ne);

    bind(return_from_index);
      comment("Subtring found at fromIndex");
      mov(result, reg(from_index));
      set_return_type(T_INT);
      comment("continue in caller");
      jmpx(lr);

    bind(search_for_first_char_loop);
      ldrh(str0_char, imm_index3(str0_min_charp, BytesPerShort, post_indexed));
      cmp(str0_char, reg(first_char));
      b(found_first_char, eq);
    bind(search_for_first_char_entry);
      cmp(str0_min_charp, reg(str0_max_charp));
      b(search_for_first_char_loop, le);
  
      comment("Substring not found - return -1;");
      mov_imm(result, -1);
      set_return_type(T_INT);
      comment("continue in caller");
      ldmfd(sp, restore_set, writeback);
#if ENABLE_THUMB_VM
      jmpx(tmp4);
#endif

    bind(found_first_char);
      mov(str0_cur_charp, reg(str0_min_charp));
      mov(str1_cur_charp, reg(str1_charp));
      b(check_substring_entry);

    bind(check_substring_loop);
      ldrh(str0_char, imm_index3(str0_cur_charp, BytesPerShort, post_indexed));
      ldrh(str1_char, imm_index3(str1_cur_charp, BytesPerShort, post_indexed));
      cmp(str0_char, reg(str1_char));
      b(search_for_first_char_entry, ne);
    bind(check_substring_entry);
      cmp(str1_cur_charp, reg(str1_end_charp));
      b(check_substring_loop, lt);

      comment("Substring found - return index;");
      sub(result, str0_min_charp, reg(str0_charp));
      sub_imm(result, result, BytesPerShort);
      mov(result, imm_shift(result, lsr, LogBytesPerShort));
      set_return_type(T_INT);
      comment("continue in caller");

      ldmfd(sp, restore_set, writeback);
#if ENABLE_THUMB_VM
      jmpx(tmp4);
#endif

  }

  bind(bailout);
    comment("Bailout to interpreter");
    push(indexof_args);
    b("interpreter_method_entry");
  }

  //----------------------java.lang.String.indexOf--------------------------

  {
    Segment seg(this, code_segment, "java.lang.String.indexOf");
    Register from_index = tos_val;
    Register ch         = tmp0;
    Register string     = tmp1;
    Register max        = tmp2;
    Register array      = tos_tag;

    const int SignedBytesPerStackElement =
                JavaStackDirection * BytesPerStackElement;

    GUARANTEE((JavaFrame::arg_offset_from_sp(0) == 0), 
              "string_indexof0 is not tested with non-zero offset");

    Label native_string_indexof_continue;
    bind_rom_linkable("native_string_indexof_entry");

    comment("[[[Hand coded method from java.lang.String]]]");
    comment("    public int indexOf(int ch) {");
    comment("        return indexOf(ch, 0);");
    comment("    }");

    comment("return address is in lr");

    wtk_profile_quick_call(/* param size */3);

    if (JavaFrame::arg_offset_from_sp(0) == 0) { 
      ldr(from_index, imm_index(jsp, -SignedBytesPerStackElement, 
                                post_indexed));
    } else {
      SHOULD_NOT_REACH_HERE(); // untested
      ldr(from_index, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
      add_imm(jsp, jsp, - SignedBytesPerStackElement);
    }
    cmp(from_index, zero);
    mov(from_index, zero, lt);
    b(native_string_indexof_continue);

    Label loop, test, failure, success;
    bind_rom_linkable("native_string_indexof0_entry");

    comment("[[[Hand coded method from java.lang.String]]]");
    comment("    public int indexOf(int ch, int fromIndex) {");
    comment("        int max = offset + count;");
    comment("        char v[] = value;");
    comment("");
    comment("        if (fromIndex < 0) {");
    comment("            fromIndex = 0;");
    comment("        } else if (fromIndex >= count) {");
    comment("            // Note: fromIndex might be near -1>>>1.");
    comment("            return -1;");
    comment("        }");
    comment("        for (int i = offset + fromIndex ; i < max ; i++) {");
    comment("            if (v[i] == ch) {");
    comment("                return i - offset;");
    comment("            }");
    comment("        }");
    comment("        return -1;");
    comment("    }");

    comment("return address is in lr");

    wtk_profile_quick_call(/* param size */2);

    // Note: pre for lr, as it must be preserved (and is same as tmp3)

    comment("Get fromIndex, ch, string");
    mov(from_index, zero);

  bind(native_string_indexof_continue);
    if (JavaFrame::arg_offset_from_sp(0) == 0) { 
      ldr(ch,     imm_index(jsp, -SignedBytesPerStackElement, post_indexed));
      ldr(string, imm_index(jsp, -SignedBytesPerStackElement, post_indexed));
    } else {
      SHOULD_NOT_REACH_HERE(); // untested
      ldr(ch,     imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
      ldr(string, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));
      add_imm(jsp, jsp, -2 * SignedBytesPerStackElement);
    }

    // i = offset + fromIndex
    // v = value.
    ldr(tmp4,    imm_index(string, String::count_offset()));  // count
    ldr(array,   imm_index(string, String::value_offset()));  // value
    ldr(tmp2,    imm_index(string, String::offset_offset())); // offset

    cmp(from_index, reg(tmp4));   // if (fromIndex >= count) goto Failure
    b(failure, ge);

    add_imm(array, array, Array::base_offset());
    add(array, array, imm_shift(tmp2, lsl, 1));   // array -> s.charAt(0);
    add(max,   array, imm_shift(tmp4, lsl, 1));   // max -> s.charAt(count)
                                                  // i.e., one chary after end
                                                  // of string)
    mov(tmp4, reg(array));
    add(array, array, imm_shift(from_index, lsl, 1)); // array -> 
                                                      // s.charAt(fromIndex);

    // At this point we must be able to go throught the loop at least
    // once -- because we are guaranteed (from_index < count)

  bind(loop);
    // IMPL_NOTE: try to unroll the loop and compare 2 or 4 chars per iteration
    ldrh(from_index, imm_index3(array, 0));
    add(array, array, imm(2));
    cmp(from_index, reg(ch));
    b(success, eq);

  bind(test);
    cmp(array, reg(max));
    b(loop, lt);
  
  bind(failure);
    comment("return -1;");
    mov_imm(tos_val, -1);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);

  bind(success);
    comment("return i - offset;");
    sub(tos_val, array, reg(tmp4));
    mov(tos_val, imm_shift(tos_val, lsr, 1));
    sub(tos_val, tos_val, imm(1)); // array has been post-indexed
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);
  }

  //----------------------java.lang.String.compareTo--------------------------

  {
    Segment seg(this, code_segment, "java.lang.String.compareTo");
    Label loop, done, error;
    // java.lang.String.compareTo
    // Method int compareTo(java.lang.String)
    bind_rom_linkable("native_string_compareTo_entry");
  
    wtk_profile_quick_call(/* param size */ 2);

    comment("get strings to compare");
    pop(tmp23);   // get first parameter;
    pop(tmp01);   // get receiver

    Register str1       = tmp2;
    Register str1_count = tmp2;
    Register str1_charp = tmp3;

    Register str0       = tmp0;
    Register str0_count = tmp0;
    Register str0_charp = tmp1;
    
    Register junk0      = tmp4;
    Register junk1      = tmp5;

    Register result     = tos_val;
    Register limit      = tos_tag;
    
    // Warning: We cannot bash Assembler::callee until after we know
    // we don't have an errors, since it contains the method
    cmp(str1, zero);
    b(error, eq);

    comment("Get str0.value[], str1.value[]");
    ldr(str0_charp, imm_index(str0, String::value_offset()));
    ldr(str1_charp, imm_index(str1, String::value_offset()));

    comment("Get str0.offset[], str1.offset[]");
    ldr(junk0, imm_index(str0, String::offset_offset()));
    ldr(junk1, imm_index(str1, String::offset_offset()));
    
    comment("Get str0.count[], str1.count[]");
    ldr(str0_count, imm_index(str0, String::count_offset()));
    ldr(str1_count, imm_index(str1, String::count_offset()));

    comment("Compute start of character data");
    add(str0_charp, str0_charp, imm(Array::base_offset()));
    add(str1_charp, str1_charp, imm(Array::base_offset()));
    add(str0_charp, str0_charp, imm_shift(junk0, lsl, LogBytesPerShort));
    add(str1_charp, str1_charp, imm_shift(junk1, lsl, LogBytesPerShort));

    comment("Compute min(str0_count, str1_count)");
    sub(result, str0_count, reg(str1_count), set_CC);
    mov(limit, reg(str0_count), lt);
    mov(limit, reg(str1_count), ge);
    comment("Is at least one string empty?");
    cmp(limit, zero);         
    b(done, eq);
    
  bind(loop);
    ldrh(junk0, imm_index3(str0_charp, BytesPerShort, post_indexed));
    ldrh(junk1, imm_index3(str1_charp, BytesPerShort, post_indexed));
    sub(result, junk0, reg(junk1), set_CC);
    b(done, ne);
    sub(limit, limit, one, set_CC);
    b(loop, ne);
    sub(result, str0_count, reg(str1_count));

  bind(done);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(lr);

  bind(error);
    comment("We have some sort of error. Redo in Java version of compareTo()");
    push(tmp01);
    push(tmp23);
    b("interpreter_method_entry");
  }

  //----------------------java.lang.String.{startsWith,endsWith}---------------

  { 
    Segment seg(this, code_segment, "java.lang.String.{starts,ends}With");
    // Note: we choose r0, r1, r2 for this_charp, prefix_charp and prefix_count,
    // to pass them as arguments to memcmp.
    Register this_charp     = tos_val, return_value = tos_val;
    Register prefix_charp   = tos_tag;
    Register prefix_count   = tmp0,    suffix_count = tmp0;
    Register this_string    = tmp1;
    Register prefix         = tmp2,    suffix       = tmp2;
    Register this_count     = tmp3,    this_offset  = tmp3;
    // Note: we would have to save off and restore lr because of bl to memcmp,
    // instead we use a different register for return address.
    Register return_address = tmp4;
    Register toffset        = tmp5;
    Register prefix_offset  = lr;

    // java.lang.String.endsWith
    // Method boolean endsWith(java.lang.String prefix)

    Label native_string_endsWith_continue, endsWithBailout;
    bind_rom_linkable("native_string_endsWith_entry");

    comment("[[[Hand coded method from java.lang.String]]]");
    comment("    public boolean endsWith(String suffix) {");
    comment("        return startsWith(suffix, count - suffix.count);");
    comment("    }");

    comment("return address is in lr");

    wtk_profile_quick_call(/* param size */2);

    comment("get suffix and this String");
    pop(suffix);
    pop(this_string);

    cmp(suffix, zero);
    b(endsWithBailout, eq);

    comment("Get this.count[], suffix.count[]");
    ldr(this_count, imm_index(this_string, String::count_offset()));
    ldr(suffix_count, imm_index(suffix, String::count_offset()));

    comment("Preserve return address");
    mov(return_address, reg(lr));

    comment("Calculate (this.count - suffix.count) for toffset");
    sub(toffset, this_count, reg(suffix_count));
    b(native_string_endsWith_continue);

  bind(endsWithBailout);
    comment("Bail out to the general endsWith implementation");
    push(this_string);
    push(suffix);
    b("interpreter_method_entry");

    // java.lang.String.startsWith
    // Method boolean startsWith(java.lang.String prefix)

    Label native_string_startsWith0_continue;
    bind_rom_linkable("native_string_startsWith0_entry");

    comment("[[[Hand coded method from java.lang.String]]]");
    comment("    public boolean startsWith(String prefix) {");
    comment("        return startsWith(prefix, 0);");
    comment("    }");

    comment("return address is in lr");

    wtk_profile_quick_call(/* param size */2);

    comment("get prefix and this String");
    pop(prefix);
    pop(this_string);

    cmp(prefix, zero);
    comment("zero for toffset");
    mov(toffset, zero, ne);
    b(native_string_startsWith0_continue, ne);

    comment("Bail out to the general startsWith implementation");
    push(this_string);
    push(prefix);
    b("interpreter_method_entry");

#if ENABLE_FAST_MEM_ROUTINES
    Label label_memcmp("jvm_memcmp");
#else
    Label label_memcmp("memcmp");
    import(label_memcmp);
#endif
    Label bailout, return_false;


    // java.lang.String.startsWith
    // Method boolean startsWith(java.lang.String prefix, int toffset)
    bind_rom_linkable("native_string_startsWith_entry");
  
    wtk_profile_quick_call(/* param size */ 3);

    const int SignedBytesPerStackElement =
                JavaStackDirection * BytesPerStackElement;

    GUARANTEE((JavaFrame::arg_offset_from_sp(0) == 0), 
              "Unimplemented for non-zero offset");

    GUARANTEE(!TaggedJavaStack, "Tagged stack not supported");

    comment("get toffset, prefix, this String");
    pop(toffset);
    pop(prefix);
    pop(this_string);

    cmp(prefix, zero);
    b(bailout, eq);

  bind(native_string_startsWith0_continue);
    comment("Get this.count[], prefix.count[]");
    ldr(this_count, imm_index(this_string, String::count_offset()));
    ldr(prefix_count, imm_index(prefix, String::count_offset()));

    comment("Preserve return address");
    mov(return_address, reg(lr));

    comment("if (toffset > this.count - prefix.count) return false;");
    sub(this_count, this_count, reg(prefix_count));
    cmp(this_count, reg(toffset));
    b(return_false, lt);

  bind(native_string_endsWith_continue);
    cmp(toffset, zero);
    b(return_false, lt);

    comment("Get this.value[], prefix.value[]");
    ldr(this_charp, imm_index(this_string, String::value_offset()));
    ldr(prefix_charp, imm_index(prefix, String::value_offset()));

    comment("Get this.offset[], prefix.offset[]");
    ldr(this_offset, imm_index(this_string, String::offset_offset()));
    ldr(prefix_offset, imm_index(prefix, String::offset_offset()));
    
    comment("Compute start of character data");
    add(this_charp, this_charp, imm(Array::base_offset()));
    add(prefix_charp, prefix_charp, imm(Array::base_offset()));
    add(this_charp, this_charp, imm_shift(this_offset, lsl, LogBytesPerShort));
    add(prefix_charp, prefix_charp, imm_shift(prefix_offset, lsl, LogBytesPerShort));

    comment("Add toffset");
    add(this_charp, this_charp, imm_shift(toffset, lsl, LogBytesPerShort));

    comment("Compute the number of bytes to compare");
    mov(prefix_count, imm_shift(prefix_count, lsl, LogBytesPerShort));

     
    // IMPL_NOTE: it's possible, that smth like
    //   ldr_label(r12, label_memcmp);
    //   blx(r12);
    // should be used to invoke external memcmp, if it's written in Thumb
    // (this shouldn't occur for any sane environment)

    //Sometimes (on THUMB, in particular) return_address is 
    //saved in a scratch register.  Save it before calling
    //memcmp if needed.
    if(   return_address == r0  || return_address == r1 
       || return_address == r2  || return_address == r3 
       || return_address == r12) {
      stmfd(sp, set(return_address), writeback);
      bl(label_memcmp);
      ldmfd(sp, set(return_address), writeback);
    } else {
      bl(label_memcmp);
    }
    cmp(return_value, zero);
    mov(return_value, one, eq);
  bind(return_false);
    // NOTE: we use that return_false is always taken on 'lt' that implies 'ne'.
    mov(return_value, zero, ne);
    set_return_type(T_INT);
    comment("continue in caller");
    jmpx(return_address);

  bind(bailout);
    comment("Bail out to the general startsWith implementation");
    push(this_string);
    push(prefix);
    push(toffset);
    b("interpreter_method_entry");
  }

  Segment seg(this, code_segment, "other java.lang.String entries");

  //----------------------java.lang.String.substring(int)--------------------
  //----------------------java.lang.String.substring(int,int)----------------
  for (int args=1; args <=2; args ++) {
    Register string     = tmp0;
    Register beginIndex = tmp1;
    Register endIndex   = tmp2;
    Register count      = tmp3;
    Label bailout;
    Label return_this;

    if (args == 1) {
      bind_rom_linkable("native_string_substringI_entry");
    } else {
      bind_rom_linkable("native_string_substringII_entry");
    }
    wtk_profile_quick_call(args+1);

    if (args == 1) {
      eol_comment("r%d = string", string);
      ldr(string, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

      eol_comment("r%d = beginIndex", beginIndex);
      ldr(beginIndex, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

      eol_comment("r%d = endIndex", endIndex);
      ldr(endIndex, imm_index(string, String::count_offset()));

      eol_comment("r%d = count", count);
      ldr(count, imm_index(string, String::count_offset()));
    } else {
      eol_comment("r%d = string", string);
      ldr(string, imm_index(jsp, JavaFrame::arg_offset_from_sp(2)));

      eol_comment("r%d = beginIndex", beginIndex);
      ldr(beginIndex, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

      eol_comment("r%d = endIndex", endIndex);
      ldr(endIndex, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

      eol_comment("r%d = count", count);
      ldr(count, imm_index(string, String::count_offset()));
    }

    comment("if (beginIndex < 0)          goto bailout;");
    comment("if (endIndex   < beginIndex) goto bailout;");
    comment("if (count      < endIndex)   goto bailout;");

    cmp(beginIndex, imm(0));
    cmp(endIndex,   reg(beginIndex), ge);
    cmp(count,      reg(endIndex), ge);
    b(bailout, lt);

    /*
        return ((beginIndex == 0) && (endIndex == count)) ? this :
            new String(offset + beginIndex, endIndex - beginIndex, value);
    */

    Register result   = tmp4;
    Register heap_top = tmp5;  
    get_inline_allocation_top(result);
    get_inline_allocation_end(heap_top);

    cmp(beginIndex, imm(0));
    cmp(endIndex, reg(count), eq);
    b(return_this, eq);

    sub(heap_top, heap_top, imm(4 * BytesPerWord), set_CC);
    b(bailout, mi);
    cmp(heap_top, reg(result));

    Register p = locals; // endIndex no longer used
    Register offset = heap_top; // heap_top no longer used

    eol_comment("r%d = string.near()", p);
    ldr(p, imm_index(string, Oop::klass_offset()));
    eol_comment("r%d = string.offset", offset);
    ldr(offset, imm_index(string, String::offset_offset()));
    b(bailout, lo);

    eol_comment("r%d = string class", p);
    ldr(p, imm_index(p, Oop::klass_offset()));

    sub(endIndex, endIndex, reg(beginIndex));

    eol_comment("result.count = endIndex - beginIndex");
    str(endIndex, imm_index(result, String::count_offset()));

    eol_comment("r%d = string proto near", p);
    ldr(p, imm_index(p, FarClass::prototypical_near_offset()));

    Register value = endIndex; // endIndex no longer used
    eol_comment("r%d = string.value", value);
    ldr(value, imm_index(string, String::value_offset()));

    add(offset, offset, reg(beginIndex));

    eol_comment("result.offset = offset + beginIndex");
    str(offset, imm_index(result, String::offset_offset()));

    eol_comment("result.klass = String proto near");
    str(p,      imm_index(result, String::klass_offset()));

    eol_comment("result.value = string.value");
    str(value,  imm_index(result, String::value_offset()));

    // No need for write barrier because the new string is in young gen

    eol_comment("return new string in r%d", tos_val);
    mov(tos_val, reg(result));

    add(result, result, imm(4 * BytesPerWord));
    set_inline_allocation_top(result);

    set_return_type(T_OBJECT);
    comment("remove arguments from the stack");
    add_imm(jsp, jsp, -JavaStackDirection * (args+1) * BytesPerStackElement);
    jmpx(lr);

  bind(return_this);
    comment("return <this>");
    mov(tos_val, reg(string));
    set_return_type(T_OBJECT);
    comment("remove arguments from the stack");
    add_imm(jsp, jsp, -JavaStackDirection * (args+1) * BytesPerStackElement);
    jmpx(lr);

  bind(bailout);
    comment("Bail out to the general startsWith implementation");
    b("interpreter_method_entry");
  }

}

#define UNCHECKED_ARRAY_COPY_ENTRY(type) \
  do {                                                                     \
    comment("No fallthrough");                                             \
    breakpoint();                                                          \
                                                                           \
    bind_rom_linkable("native_jvm_unchecked_" #type "_arraycopy_entry");   \
    wtk_profile_quick_call(/* param size */ 5);                            \
                                                                           \
    /* IMPL_NOTE: consider using LDM, currently conflicts with existing */ \
    /* register assignment fixed by hardcoded WMMX instructions.        */ \
    comment("Pop arguments from stack");                                   \
    pop(length);                                                           \
    pop(dst_pos);                                                          \
    pop(dst);                                                              \
    pop(src_pos);                                                          \
    pop(src);                                                              \
                                                                           \
    sub(length, length, one, set_CC);                                      \
                                                                           \
    comment("Length == 0.  Just return");                                  \
    b(done, lt);                                                           \
                                                                           \
    comment("Point at actual data");                                       \
    add(dst, dst, imm(Array::base_offset()));                              \
    add(src, src, imm(Array::base_offset()));                              \
  } while (0)

void NativeGenerator::generate_native_system_entries() {
  Segment seg(this, code_segment, "Native entry points for system functions");
  Label bailout, bailout_2;
  Label done;
  const Register return_reg = r7;
#if ENABLE_XSCALE_WMMX_ARRAYCOPY
  OptimizeArrayCopy=false;
  Label bailout_length_zero, bailout_length_zero_2;
#endif

  //  public static native void arraycopy(Object src, int src_position,
  //                                      Object dst, int dst_position,
  //                                      int length);

  bind_rom_linkable("native_system_arraycopy_entry");

  wtk_profile_quick_call(/* param size */ 5);

  // register callee  holds the method to invoke
  // register lr=bcp  holds the return address
  // We cannot bash it until we know that we have no error.
  Assembler::Register length  = tmp0;
  Assembler::Register src_pos = tmp1;
  Assembler::Register dst_pos = tmp2;
  Assembler::Register t1      = tmp3;
  Assembler::Register t2      = tmp4;
  Assembler::Register t3      = tmp5; // bcode
  Assembler::Register src     = (callee == tos_val) ? tos_tag : tos_val;
  Assembler::Register dst     = locals;

  // Used only after we know we aren't calling the method the slow path
  Assembler::Register m1      = callee;

  comment("load arguments to registers");
  comment("r%d = src", src);
  ldr(src, imm_index(jsp, JavaFrame::arg_offset_from_sp(4)));

  comment("r%d = dst", dst);
  ldr(dst, imm_index(jsp, JavaFrame::arg_offset_from_sp(2)));

  comment("r%d = length", length);
  ldr(length, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

  comment("r%d = dst_pos", dst_pos);
  ldr(dst_pos, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

  comment("r%d = src_pos", src_pos);
  ldr(src_pos, imm_index(jsp, JavaFrame::arg_offset_from_sp(3)));

  comment("if (src == NULL || dst == NULL) goto bailout;");
  cmp(src, imm(0));
  cmp(dst, imm(0), ne);
  b(bailout, eq); 

  comment("Get src.klass() and dst.klass()");
  ldr(t1, imm_index(src, Oop::klass_offset()));
  ldr(t2, imm_index(dst, Oop::klass_offset()));
  ldr(t1, imm_index(t1, Oop::klass_offset()));
  ldr(t2, imm_index(t2, Oop::klass_offset()));

  comment("if (length < 0 || src_pos < 0 || dst_pos < 0) goto bailout;");
  orr(t3, length, reg(src_pos));
  orr(t3, t3, reg(dst_pos), set_CC); 
  b(bailout, mi);
  
  comment("if (src.klass() != dst.klass()) goto bailout;");
  cmp(t1, reg(t2));
  b(bailout, ne);

  comment("Make sure we really have an array.");
  comment("Java objects have an instance size >= 0");
  ldrsh(t3, imm_index3(t1, FarClass::instance_size_offset()));
  cmp(t3, zero);
  b(bailout, ge);

  comment("if ((unsigned int) src.length < (unsigned int) src_pos + (unsigned int) length) goto bailout;");
  ldr(t1, imm_index(src, Array::length_offset()));
  add(t2, src_pos, reg(length));
  cmp(t1, reg(t2));
  b(bailout, lo);

  comment("if ((unsigned int) dst.length < (unsigned int) dst_pos + (unsigned int) length) goto bailout;");
  ldr(t1, imm_index(dst, Array::length_offset()));
  add(t2, dst_pos, reg(length));
  cmp(t1, reg(t2));
  b(bailout, lo);

  // size_object_array       = -1
  // size_type_array_1       = -2,   // instance is boolean[], byte[]
  // size_type_array_2       = -3,   // instance is short[], char[]
  // size_type_array_4       = -4,   // instance is int[], float[]
  // size_type_array_8       = -5,   // instance is long[], double[]
  // Convert these into an appropriate shift amount

  comment("remove arguments from the stack");
  add_imm(jsp, jsp, -JavaStackDirection * 5 * BytesPerStackElement);

  comment("Point at actual data");
  add(src, src, imm(Array::base_offset()));
  add(dst, dst, imm(Array::base_offset()));
  
  comment("Jump to appropriate copy routine if length > 0");
  sub(length, length, one, set_CC);
#if ENABLE_STATIC_TRAMPOLINE
  comment("Length > 0.  Jump to appropriate trampoline;");
//  ldr(pc, sub_index(pc, t3, lsl, times_4), ge); // t3 in the range -5 .. -1
  sub(pc, pc, imm_shift(t3, lsl, times_4),ge);
  comment("Length == 0.  Just return");
  b(done, lt);

  // This table must be two instructions after the ldr. . .
  Label copy_type_array[4], copy_object_array, copy_int_array, copy_illegal;
  b(copy_illegal);             // unused
  b(copy_object_array);
  b(copy_type_array[0]);       // size_type_array_1
  b(copy_type_array[1]);       // size_type_array_2
  b(copy_type_array[2]);       // size_type_array_4
  b(copy_type_array[3]);       // size_type_array_8
#else
  comment("Length > 0.  Jump to appropriate routine;");
  ldr(pc, sub_index(pc, t3, lsl, times_4), ge); // t3 in the range -5 .. -1
  comment("Length == 0.  Just return");
  b(done, lt);

  // This table must be two instructions after the ldr. . .
  Label copy_type_array[4], copy_object_array, copy_int_array, copy_illegal;
  define_long(copy_illegal);             // unused
  define_long(copy_object_array);
  define_long(copy_type_array[0]);       // size_type_array_1
  define_long(copy_type_array[1]);       // size_type_array_2
  define_long(copy_type_array[2]);       // size_type_array_4
  define_long(copy_type_array[3]);       // size_type_array_8
#endif

#if ENABLE_XSCALE_WMMX_ARRAYCOPY 
  Label array_copy_align_dest_head("wmmx_memcpy");      // align dest address

  Label array_copy_align_dest_head_loop;
  Label array_copy_dest_head_aligned;
  Label array_copy_loop_32;              // copy 32 bytes in one loop
  Label array_copy_loop_8; 
  Label array_copy_loop_32_src_aligned;           // copy 8 bytes in one loop
  Label array_copy_loop_8_src_aligned;           // copy 8 bytes in one loop
  Label array_quick_leave;
  OptimizeArrayCopy=false;
#endif
  if (OptimizeArrayCopy) {
    for (int i = 0; i <= 3; i++) { 
      switch (i) {
      case 0: UNCHECKED_ARRAY_COPY_ENTRY(byte); break;
      case 1: UNCHECKED_ARRAY_COPY_ENTRY(char); break;
      case 2: UNCHECKED_ARRAY_COPY_ENTRY(int);  break;
      case 3: UNCHECKED_ARRAY_COPY_ENTRY(long);  break;
      }
      
      if( i == 2 ) {
        bind(copy_int_array);
      }
      bind(copy_type_array[i]);

      comment("Each element of the array is %d byte(s)", 1 << i);
      if (src != r0) {
        add(r0, dst, imm_shift(dst_pos, lsl, i));
        add(r1, src, imm_shift(src_pos, lsl, i));
      } else if (dst != r1) {
        add(r1, src, imm_shift(src_pos, lsl, i));
        add(r0, dst, imm_shift(dst_pos, lsl, i));
      } else {
        SHOULD_NOT_REACH_HERE(); // need additional register
      }

      add(r2, length, one);
      if (i != 0) {
        mov(r2, imm_shift(r2, lsl, i));
      }
   
      mov_reg(return_reg, lr);
#if ENABLE_FAST_MEM_ROUTINES
      // Our implementation of memcpy should handle overlapping arguments
      bl("jvm_memcpy");
#else
      bl("memmove");
#endif
      comment("fill the top of stack cache and return");
      set_return_type(T_VOID);
      jmpx(return_reg);
    }
  } else {

    for (int i = 0; i <= 3; i++) {
      switch (i) {
      case 0: UNCHECKED_ARRAY_COPY_ENTRY(byte); break;
      case 1: UNCHECKED_ARRAY_COPY_ENTRY(char); break;
      case 2: UNCHECKED_ARRAY_COPY_ENTRY(int);  break;
      case 3: UNCHECKED_ARRAY_COPY_ENTRY(long);  break;
      }

      Label again;
      if( i == 2 ) {
        bind(copy_int_array);
      }
      bind(copy_type_array[i]);

      // Since we specifically copy from low index to high index,
      // we can handle src_pos >= dst_pos
      comment("if (src == dst) && (src_pos < dst_pos) goto bailout_2;");
      Label src_ne_dst;
      cmp(src, reg(dst));
      b(src_ne_dst, ne);
      cmp(src_pos, reg(dst_pos));
#if XSCALE_ENABLE_WMMX_ARRAYCOPY
      b(bailout_length_zero_2,lt);
#else
      b(bailout_2, lt);
#endif
    bind(src_ne_dst);
        comment("Need to reset condition codes so copy below work correctly");
        cmp(length, zero);

        comment("Each element of the array is %d byte(s)", 1 << i);
        add(src, src, imm_shift(src_pos, lsl, i));
        add(dst, dst, imm_shift(dst_pos, lsl, i));
#if ENABLE_XSCALE_WMMX_ARRAYCOPY
        if (i == 0) {
          cmp(length, imm(40));
          add_imm(length, length, 1, no_CC, ge);
          b(array_copy_align_dest_head, ge);
          cmp(length, imm(0));
        }
#endif
      bind(again);
        switch(i) { 
        case 0:
            ldrb(t1, imm_index(src, 1, post_indexed));
            ldrb(t2, imm_index(src, 1, post_indexed), gt);
            strb(t1, imm_index(dst, 1, post_indexed));
            strb(t2, imm_index(dst, 1, post_indexed), gt);
            break;
        case 1:
            ldrh(t1, imm_index3(src, 2, post_indexed));
            ldrh(t2, imm_index3(src, 2, post_indexed), gt);
            strh(t1, imm_index3(dst, 2, post_indexed));
            strh(t2, imm_index3(dst, 2, post_indexed), gt);
            break;
        case 2:
            ldr(t1, imm_index(src, 4, post_indexed));
            ldr(t2, imm_index(src, 4, post_indexed), gt);
            str(t1, imm_index(dst, 4, post_indexed));
            str(t2, imm_index(dst, 4, post_indexed), gt);
            break;
        case 3:
            ldmia(src, set(t1, t2), writeback);
            ldmia(src, set(src_pos, dst_pos), writeback, gt);
            stmia(dst, set(t1, t2), writeback);
            stmia(dst, set(src_pos, dst_pos), writeback, gt);
            break;
        }
        sub(length, length, imm(2), set_CC);
        b(again, ge);
        comment("fill the top of stack cache and return");
        set_return_type(T_VOID);
        jmpx(lr);

#if ENABLE_XSCALE_WMMX_ARRAYCOPY
      if( i == 0 ){
        comment("array copy with iwmmxt instructions");
       bind(array_copy_align_dest_head);
      
        comment("check dest pos is aligned");
        andr_imm(t1, dst, 7, set_CC);
        b(array_copy_dest_head_aligned, eq);
        rsb_imm(t1, t1, 8);
        sub(length, length, reg(t1));
        pld(imm_index(src));
        pld(imm_index(src,32));
        pld(imm_index(src,64));
        comment("loop used to aligan dest head");
       bind(array_copy_align_dest_head_loop);
        ldrb(t2, imm_index(src, 1, post_indexed));
        //pld(imm_index(src,32));
        sub_imm(t1, t1, 1, set_CC);
        strb(t2, imm_index(dst, 1, post_indexed));
        b(array_copy_align_dest_head_loop, gt);
      
        comment("copy left src bytes before next aligned address");
       bind(array_copy_dest_head_aligned);
        andr_imm(t1, src, 7, set_CC);
        b(array_copy_loop_32_src_aligned, eq);
        bic_imm(src, src, 7);
        //wldrd(wR0, src, 8, post_indexed);
        define_long(0xecf10102);
        //tmcr(wCGR0, t1);
        define_long(0xee089110); 

        comment("copy 32 bytes at a time");
       bind(array_copy_loop_32);
        //wldrd(wR1, src, 8, post_indexed);
        define_long(0xecf11102);
        sub_imm(length, length, 32);
        //wldrd(wR2, src, 8, post_indexed);
        define_long(0xecf12102);

        cmp(length, imm(32));
        //wldrd(wR3, src, 8, post_indexed);
        define_long(0xecf13102);

        //walignr0(wR0, wR0, wR1);
        define_long(0xee800021); 

        //wstrd(wR0, dst, 8, post_indexed);
        define_long(0xece70102);

        //walignr0(wR1, wR1, wR2);
        define_long(0xee811022);

        //wldrd(wR0, src, 8, post_indexed);
        define_long(0xecf10102);  
        //wstrd(wR1, dst, 8, post_indexed);
        define_long(0xece71102); 

        //walignr0(wR2, wR2, wR3);
        define_long(0xee822023);

        //wstrd(wR2, dst, 8, post_indexed);
        define_long(0xece72102);

        //walignr0(wR3, wR3, wR0);
        define_long(0xee833020);

        //wstrd(wR3, dst, 8, post_indexed);
        define_long(0xece73102); 
        
        //Change from wor to waligni to prevent touching
        //wCASF
        b(array_copy_loop_32, ge);
        cmp(length, imm(0));
        b(array_quick_leave, eq);
   
        comment("copy 8 bytes at a time");
       bind(array_copy_loop_8);
        sub_imm(length, length, 8, set_CC);
        //wldrd(wR1, src, 8, post_indexed, ge);
        define_long(0xacf11102);
        //walignr0(wR2, wR0, wR1, ge);
        define_long(0xae802021);
        //wstrd(wR2, dst, 8, post_indexed, ge);
        define_long(0xace72102);     
      
        //Change from wor to waligni to prevent touching
        //wCASF
        //waligni(wR0, wR1, wR1, 0, ge);
        define_long(0xae010021);
        b(array_copy_loop_8, gt);
        b(array_quick_leave, eq);
        
        comment("compensate length decrease in loop");
        add_imm(length, length, 8, no_CC, ne);
        comment("compensate postindex of src address  in loop");
        sub_imm(src, src, 8, no_CC, ne);
        comment("compensate src bytes left in wR0");
        add(src, src, reg(t1), no_CC, ne);
        comment("compensate state for using sun code");
        sub_imm(length, length, 1, set_CC);
        
        b(again);
       bind(array_quick_leave);
        mov_imm(length, 0, eq);
        mov(pc, reg(lr), eq);
        
       bind(array_copy_loop_32_src_aligned);
        //wldrd(wR0, src, 8, post_indexed);
        define_long(0xecf10102);
        sub_imm(length, length, 32);
        //wldrd(wR1, src, 8, post_indexed);
        define_long(0xecf11102);
        cmp(length, imm(32));
        //wldrd(wR2, src, 8, post_indexed);
        define_long(0xecf12102);
        //wstrd(wR0, dst, 8, post_indexed);
        define_long(0xece70102);
        //wldrd(wR3, src, 8, post_indexed);
        define_long(0xecf13102);
        //wstrd(wR1, dst, 8, post_indexed);
        define_long(0xece71102);
        //wstrd(wR2, dst, 8, post_indexed);
        define_long(0xece72102);
        //wstrd(wR3, dst, 8, post_indexed);
        define_long(0xece73102);
        
        //Change from wor to waligni to prevent touching
        //wCASF
        b(array_copy_loop_32_src_aligned, ge);
        cmp(length, imm(0));
        b(array_quick_leave, eq);
   
        comment("copy 8 bytes at a time");
       bind(array_copy_loop_8_src_aligned);
        sub_imm(length, length, 8, set_CC);
        //wldrd(wR1, src, 8, post_indexed, ge);
        define_long(0xacf11102);

        pld(imm_index(src,0));
        //wstrd(wR1, dst, 8, post_indexed, ge);
        define_long(0xace71102);
      
        //Change from wor to waligni to prevent touching
        //wCASF
        //waligni(wR0, wR1, wR1, 0, ge);
        define_long(0xae010021);
        b(array_copy_loop_8_src_aligned, gt);
        b(array_quick_leave, eq);
                
        comment("compensate length decrease in loop");
        add_imm(length, length, 8, no_CC, ne);
        comment("compensate state for using sun code");
        sub_imm(length, length, 1, set_CC);
        
        b(again);
      }
#endif // ENABLE_XSCALE_WMMX_ARRAYCOPY  
      }
  }

  UNCHECKED_ARRAY_COPY_ENTRY(obj);

bind(copy_object_array);  
  comment("If dst is in new space, there's no need to set bitvector");
  get_old_generation_end(t3);
  cmp(dst, reg(t3));
  b(copy_int_array, ge);

  comment("if (src == dst) && (src_pos < dst_pos) goto copy_downwards;");
  Label copy_downwards, copy_upwards;
  cmp(src, reg(dst));
  b(copy_upwards, ne);
  cmp(src_pos, reg(dst_pos));
  b(copy_downwards, lt);

bind(copy_upwards);
  for (int upwards=1; upwards>=0; upwards--) {
    // upwards = 1: We are coping the array starting from a low address
    // upwards = 0: We are coping the array starting from a high address
    //              to handle overlapping regions.

    Label copy_object_array_loop;
    Label aligned_copy_loop;
    Label not_yet_aligned_copy;
    Label small_length_copy;

    Address4 four_regs = set(src_pos, dst_pos, t1, t2);

    if (upwards) {
      comment("Copy the elements starting from low address upwards");
    } else {
      comment("Copy the elements starting from high address downwards");
      bind(copy_downwards);
    }
    comment("Point to the (inclusive) low-end of elements to copy");
    add(src, src, imm_shift(src_pos, lsl, times_4));
    add(dst, dst, imm_shift(dst_pos, lsl, times_4));

    if (!upwards) {
      comment("Point to the (exclusive) high-end of elements");
      comment("%s is one less than the number of elements still to copy", 
              reg_name(length));
      add(src, src, imm_shift(length, lsl, LogBytesPerWord));
      add(dst, dst, imm_shift(length, lsl, LogBytesPerWord));
      add(src, src, imm(BytesPerWord));
      add(dst, dst, imm(BytesPerWord));
    }

  bind(copy_object_array_loop);
    comment("%s is one less than the number of elements still to copy", 
            reg_name(length));
    comment("Are we copying at least eight elements?");
    cmp(length, imm(7));
    b(small_length_copy, lt);

    comment("Is the destination aligned?");
    tst(dst, imm(right_n_bits(LogBitsPerByte) << LogBytesPerWord));
    b(not_yet_aligned_copy, ne);

    comment("We have at least 8 words to copy, and the dst is aligned");
    get_bitvector_base(t3);
    mov_imm(m1, -1);

    comment("The main copy loop copies 32 elements each iteration.");
    comment("If don't have (n*32 + {0..7}) elements, jump to the");
    comment("middle of the array");

    Label n_x_32_plus_24;
    Label n_x_32_plus_16;
    Label n_x_32_plus_8;

    add_imm(t2, length, 1);
    andr_imm(t2, t2, 8+16);

    cmp(t2, imm( 8));   b(n_x_32_plus_8,  eq);
    cmp(t2, imm(16));   b(n_x_32_plus_16, eq);
    cmp(t2, imm(24));   b(n_x_32_plus_24, eq);

    bind(aligned_copy_loop);
    for (int i=0; i<4; i++) {
      if (!upwards) {
        ldmdb(src, four_regs, writeback);
        stmdb(dst, four_regs, writeback);
        ldmdb(src, four_regs, writeback);
        stmdb(dst, four_regs, writeback);
      }
      // set the byte at little-endian address t3 + (dst >> 5) to -1
      if (HARDWARE_LITTLE_ENDIAN) {  
        strb(m1, add_index(t3, dst, lsr, LogBitsPerByte + LogBytesPerWord));
      } else { 
        // We need to convert little-endian byte address to big-endian
        add(t1, t3, imm_shift(dst, lsr, LogBitsPerByte + LogBytesPerWord));
        eor(t1, t1, imm(3));
        strb(m1, imm_index(t1));
      }
      if (upwards) {
        ldmia(src, four_regs, writeback);
        stmia(dst, four_regs, writeback);
        ldmia(src, four_regs, writeback);
        stmia(dst, four_regs, writeback);
      }
      if (i != 3) {
        sub(length, length, imm(8));
        if (i == 0) {
          bind(n_x_32_plus_24);
        } else if (i == 1) {
          bind(n_x_32_plus_16);
        } else {
          bind(n_x_32_plus_8);
        }
      } else {
        sub(length, length, imm(8), set_CC);
        b(done, lt);
        cmp(length, imm(7));
        b(aligned_copy_loop, ge);
      }
    }

  bind(small_length_copy);
    comment("Copying less than 8 elements");
    if (upwards) {
      ldr(t2, imm_index(src, BytesPerWord, post_indexed));
      mov(t1, reg(dst));
      str(t2, imm_index(dst, BytesPerWord, post_indexed));
      oop_write_barrier(t1, t2, src_pos, dst_pos, false);
    } else {
      ldr(t2, imm_index(src, -BytesPerWord, pre_indexed));
      str(t2, imm_index(dst, -BytesPerWord, pre_indexed));
      mov(t1, reg(dst));
      oop_write_barrier(t1, t2, src_pos, dst_pos, false);
    }
    sub(length, length, one, set_CC);
    b(small_length_copy, ge);
    set_return_type(T_VOID);
    jmpx(lr);

  bind(not_yet_aligned_copy);
    comment("length >= 8, but dst not aligned");
    if (upwards) {
      ldr(t2, imm_index(src, BytesPerWord, post_indexed));
      mov(t1, reg(dst));
      str(t2, imm_index(dst, BytesPerWord, post_indexed));
      oop_write_barrier(t1, t2, src_pos, dst_pos, false);
    } else {
      ldr(t2, imm_index(src, -BytesPerWord, pre_indexed));
      str(t2, imm_index(dst, -BytesPerWord, pre_indexed));
      mov(t1, reg(dst));
      oop_write_barrier(t1, t2, src_pos, dst_pos, false);
    }
    sub(length, length, one);
    b(copy_object_array_loop);
  }

bind(done);
  set_return_type(T_VOID);
  jmpx(lr);

bind(copy_illegal);
  comment("Shouldn't call this");
  breakpoint();

bind(bailout_2);
  comment("Recover original parameters");
  sub_imm(jsp, jsp, -JavaStackDirection * 5 * BytesPerStackElement);
  sub(src, src, imm(Array::base_offset()));
  sub(dst, dst, imm(Array::base_offset()));
  add(length, length, one);

bind(bailout);
  comment("Bail out to the general arraycopy implementation");
  b("interpreter_method_entry");
#if  ENABLE_XSCALE_WMMX_ARRAYCOPY

bind(bailout_length_zero_2);
  comment("Recover original parameters");
  sub_imm(jsp, jsp, -JavaStackDirection * 5 * BytesPerStackElement);
  sub(src, src, imm(Array::base_offset()));
  sub(dst, dst, imm(Array::base_offset()));
  add(length, length, one);

bind(bailout_length_zero);
  cmp( r2, imm(0));
  b(bailout, ne);
  ldr(t1, imm_index(src, Oop::klass_offset()));
  ldr(t2, imm_index(dst, Oop::klass_offset()));
  ldr(t1, imm_index(t1, Oop::klass_offset()));
  ldr(t2, imm_index(t2, Oop::klass_offset()));
  
  orr(t3, length, reg(src_pos));
  orr(t3, t3, reg(dst_pos), set_CC); 
  b(bailout, mi);  
  
  cmp(t1, reg(t2));
  b(bailout, ne);
  
  ldrsh(t3, imm_index3(t1, FarClass::instance_size_offset()));
  cmp(t3, zero);
  b(bailout, ge);
  
  set_return_type(T_VOID);
  jmpx(lr); 
#endif
}

#undef UNCHECKED_ARRAY_COPY_ENTRY

void NativeGenerator::generate_native_thread_entries() {
  // bind_rom_linkable("native_thread_enterLockObject_entry");
  // Label c_code("Java_com_sun_cldchi_jvm_Thread_enterLockObject");
  // import(c_code);
  // b(c_code);
}

void NativeGenerator::generate_native_misc_entries() {
  //----------------------java.lang.StringBuffer.append-----------------------

  {
    Segment seg(this, code_segment, "StringBuffer.append");
    bind_rom_linkable("native_stringbuffer_append_entry");

    Register obj          = tmp0;
    Register thechar      = tmp1;
    Register near_obj     = tmp1;
    Register count        = tmp2;
    Register lock         = tmp1;
    Register buf          = tmp3;
    Register bufsize      = tmp1;
    Register return_value = tos_val;

    comment("Get the object.");
    ldr(obj,      imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

    comment("Get the near.");
    ldr(near_obj, imm_index(obj, Oop::klass_offset()));
    ldr(buf,      imm_index(obj, Instance::header_size()));
    ldr(lock,     imm_index(near_obj, JavaNear::raw_value_offset()));
    ldr(count,    imm_index(obj, Instance::header_size()+sizeof(jobject)));

    comment("Check the lock bit.");
    tst(lock, one);
    b("interpreter_method_entry", ne);

    comment("Check for insufficient capacity of the stringbuffer.");
    ldr(bufsize,  imm_index(buf, Array::length_offset()));
    add(buf, buf, imm_shift(count, lsl, 1));
    cmp(count, reg(bufsize));
    b("interpreter_method_entry", ge);

    comment("Increment the count, append the char.");
    ldr(thechar,  imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
    add(count, count, imm(1));
    str(count, imm_index(obj, Instance::header_size()+sizeof(jobject)));
    strh(thechar, imm_index3(buf, Array::base_offset()));

    comment("Return the object.");
    mov(return_value, reg(obj));
    set_return_type(T_OBJECT);
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    jmpx(lr);
  }

  //---------------------- java.lang.Integer.toString(int) --------------------
  /*
      This procedure creates the following structure in Java Heap

      result -> [String JavaNear       ] <- old inline_allocation_top
                [String value          ] == obj
                [String offset         ] == max_len - count
                [String count          ] == (last_ptr - ptr) / sizeof(jchar)
         obj -> [char[] JavaNear       ]
                [char[] length         ] == max_len
                [char[] digit 0        ]
         ptr -> [char[] digit ...      ]
                [char[] digit max_len-1] 
    last_ptr ->                          <- new inline_allocation_top

  */
  {
    Segment seg(this, code_segment, "Integer.toString");
    bind_rom_linkable("native_integer_toString_entry");

    Register remainder = tmp0;
    Register quotient  = tmp1;
    Register obj       = tmp2;
    Register ptr       = tmp3;
    Register last_ptr  = tmp4;
    Register tmp       = tmp5;
    Register num       = tos_val;
    Register sign      = tos_tag;
    Register result    = tos_val;

    Label div_loop, div10_magic_constant;

    const int max_len = 12; // the longest number (-2147483648) is of 11 chars
    const int string_instance_size = String::header_size() + 12;
    const int charray_instance_size = TypeArray::base_offset() +
                                      sizeof(jchar) * max_len;
    const int needed_memory = string_instance_size + charray_instance_size;

    comment("Find the size of inline allocation area to see if it is large");
    comment("enough to create new String object with associated char[] array");
    get_inline_allocation_top(obj);
    get_inline_allocation_end(tmp);
    add(ptr, obj, imm(needed_memory));
    cmp(ptr, reg(tmp));
    b("interpreter_method_entry", hi);

    comment("Get JavaNear prototype for String and char[] from persistent handles");
    ldr_label(tmp, "persistent_handles");
    set_inline_allocation_top(ptr);
    ldr(tmp0, imm_index(tmp, Universe::string_class_index * BytesPerWord));
    ldr(tmp1, imm_index(tmp, Universe::char_array_class_index * BytesPerWord));
    ldr(tmp0, imm_index(tmp0, FarClass::prototypical_near_offset()));
    ldr(tmp1, imm_index(tmp1, FarClass::prototypical_near_offset()));

    comment("Fill in all the fields we know");
    eol_comment("set String JavaNear");
    str(tmp0, imm_index(obj, string_instance_size, post_indexed));
    eol_comment("set char[] JavaNear");
    str(tmp1, imm_index(obj));
    eol_comment("set String.value to be our char[]");
    str(obj, imm_index(obj, String::value_offset() - string_instance_size));
    eol_comment("set length field of char[] array");
    mov(tmp, imm(max_len));
    str(tmp, imm_index(obj, TypeArray::length_offset()));

    comment("Get the argument from Java stack");
    pop(num);
    comment("Remember the position of the last digit");
    mov(last_ptr, imm_shift(ptr, lsr, LogBytesPerShort));
    comment("Load the magic constant for integer divsion by 10");
    ldr_from(tmp, div10_magic_constant);

    comment("Make the argument non-negative");
    andr(sign, num, imm(0x80000000), set_CC);
    rsb(num, num, zero, mi);

    comment("Perform integer division by 10 in the loop");
    comment("and store digits in char[] array from right to left.");
    comment("Dividend is in r%d, remainder is in r%d", num, remainder);
  bind(div_loop);
    umull(remainder, quotient, num, tmp);
    add(remainder, num, imm('0'));
    mov(num, imm_shift(quotient, lsr, 3), set_CC);
    sub(remainder, remainder, imm_shift(num, lsl, 3));
    sub(remainder, remainder, imm_shift(num, lsl, 1));
    strh(remainder, imm_index3(ptr, -sizeof(jchar), pre_indexed));
    b(div_loop, ne);

    comment("Add '-' sign if the original number was negative");
    orr(sign, sign, imm('-'), set_CC);
    strh(sign, imm_index3(ptr, -sizeof(jchar), pre_indexed), mi);

    comment("Fill in remained fields");
    eol_comment("pointer to result string");
    sub(result, obj, imm(string_instance_size));
    eol_comment("set String.count");
    sub(ptr, last_ptr, imm_shift(ptr, lsr, LogBytesPerShort));
    str(ptr, imm_index(result, String::count_offset()));
    eol_comment("set String.offset");
    rsb(ptr, ptr, imm(max_len));
    str(ptr, imm_index(result, String::offset_offset()));
    
    comment("Return to caller");
    set_return_type(T_OBJECT);
    jmpx(lr);

  bind(div10_magic_constant);
    define_long(0xcccccccd);
  }

 Segment seg(this, code_segment, 
             "Native entry points for miscellaneous functions");

  //----------------------java.util.Vector.elementAt---------------------
  {
    bind_rom_linkable("native_vector_elementAt_entry");

    Register obj          = tmp0;
    Register index        = tmp1;
    Register near_obj     = tmp2;
    Register obj_size     = tmp3;
    Register v_temp       = obj_size;
    Register lock         = near_obj;
    Register return_value = tos_val;

    ldr(obj,       imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));
    ldr(index,     imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

    comment("Get the near object.");
    ldr(near_obj, imm_index(obj, Oop::klass_offset()));
    ldr(obj_size, imm_index(obj, Instance::header_size()+sizeof(jobject)));
    ldr(lock,     imm_index(near_obj, JavaNear::raw_value_offset()));

    comment("Check for out of range index.");
    cmp(index, reg(obj_size));
    b("interpreter_method_entry", cs); //unsigned check
  
    comment("Check the lock bit.");
    tst(lock, one);
    ldr(v_temp, imm_index(obj, Instance::header_size()), eq);
    b("interpreter_method_entry", ne);

    comment("Get the object and return.");
    ldr_indexed(return_value,v_temp,index,times_4, Array::base_offset());
    set_return_type(T_OBJECT);
    comment("continue in caller");

    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    jmpx(lr);
  }

  //----------------------java.util.Vector.addElement---------------------
  {
    bind_rom_linkable("native_vector_addElement_entry");
    Label label_expand_vector;
    Label label_memset("memset");
    import(label_memset);
#if ENABLE_FAST_MEM_ROUTINES
    Label label_memcpy("jvm_memcpy");
#else
    Label label_memcpy("memcpy");
    import(label_memcpy);
#endif

    Register self        = tmp0;
    Register near_obj    = tmp1;
    Register el_data     = tmp2;
    Register el_count    = tmp3;
    Register tmp         = tmp4;
    Register data_length = tmp5;

    comment("Get this Vector object");
    ldr(self, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

    comment("Load r%d = Near, r%d = elementData and r%d = elementCount",
            near_obj, el_data, el_count);
#if NOT_CURRENTLY_USED
    ldr(near_obj, imm_index(self, Oop::klass_offset()));
    ldr(el_data, imm_index(self, Instance::header_size()));
    ldr(el_count, imm_index(self, Instance::header_size() + sizeof(jobject)));
#else
    GUARANTEE(Instance::header_size() == Oop::klass_offset() + sizeof(jobject),
              "otherwise we cannot use ldmia");
    ldmia(self, set(near_obj, el_data, el_count));
#endif

    comment("Check if the object is locked");
    ldr(near_obj, imm_index(near_obj, JavaNear::raw_value_offset()));
    ldr(data_length, imm_index(el_data, ObjArray::length_offset()));
    tst(near_obj, one);
    b("interpreter_method_entry", ne);

    comment("Check if elementCount < elementData.length");
    cmp(el_count, reg(data_length));
    b(label_expand_vector, hs);

    comment("Usual case: put new element in existing elementData array");
    ldr(tmp, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
    add(el_data, el_data, imm_shift(el_count, lsl, times_4));
    str(tmp, imm_index(el_data, Array::base_offset(), pre_indexed));
    oop_write_barrier(el_data, tmp, near_obj, data_length, false);
    add(el_count, el_count, one);
    str(el_count, imm_index(self, Instance::header_size() + sizeof(jobject)));

    comment("Return to caller");
    set_return_type(T_VOID);
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    jmpx(lr);

bind(label_expand_vector);
    comment("Need to increase Vector capacity");
    Register new_capacity = near_obj;
    Register heap_top = tos_tag;
    Register heap_end = tmp;
    Register new_heap_top = data_length;
    ldr(new_capacity, imm_index(self, Instance::header_size() + sizeof(jobject) * 2));
    get_inline_allocation_top(heap_top);
    cmp(new_capacity, zero);
    add(new_capacity, new_capacity, reg(data_length), gt);
    add(new_capacity, data_length, reg(data_length), set_CC, le);
    add(new_capacity, el_count, one, eq);

    comment("Check if we have enough space in inline allocation area");
    get_inline_allocation_end(heap_end);
    add(new_heap_top, heap_top, imm(ObjArray::base_offset()));
    add(new_heap_top, new_heap_top, imm_shift(new_capacity, lsl, times_4));
    cmp(heap_end, reg(new_heap_top));
    b("interpreter_method_entry", lo);
    
    comment("Create a new ObjArray with increased length");
    set_inline_allocation_top(new_heap_top);
    ldr(tmp, imm_index(el_data, Oop::klass_offset()));
    str(new_capacity, imm_index(heap_top, ObjArray::length_offset()));
    str(tmp, imm_index(heap_top, Oop::klass_offset()));

    comment("Update fields of this Vector object");
    add(tmp, el_count, one);
    str(tmp, imm_index(self, Instance::header_size() + sizeof(jobject)));
    str(heap_top, imm_index(self, Instance::header_size(), pre_indexed));
    oop_write_barrier(self, r0, tmp, new_heap_top, false);

    comment("Copy old Vector data - prepare registers for jvm_memcpy");
    Register return_reg = el_data;
    Register fill_start = is_c_saved_register(tmp) ? tmp : new_heap_top;
    add(r0, heap_top, imm(ObjArray::base_offset()));
    add(r1, el_data, imm(ObjArray::base_offset()));
    mov(r2, imm_shift(el_count, lsl, times_4));
    add(return_reg, lr, zero, set_CC); // this will also clear CARRY flag
    sbc(el_count, new_capacity, reg(el_count));
    add(fill_start, r0, reg(r2));
    bl(label_memcpy);

    GUARANTEE(is_c_saved_register(el_count) &&
              is_c_saved_register(fill_start) &&
              is_c_saved_register(return_reg), "code assumption");

    comment("Put new element into the Vector");
    ldr(r1, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    str(r1, imm_index(fill_start));

    if (!ENABLE_ZERO_YOUNG_GENERATION) {
      comment("Fill other elements with zero using memset");
      mov(r2, imm_shift(el_count, lsl, times_4), set_CC);
      add(r0, fill_start, imm(sizeof(jobject)), ne);
      mov(r1, zero, ne);
      bl(label_memset, ne);
    }
    
    comment("Return to caller");
    set_return_type(T_VOID);
    jmpx(return_reg);
  }

#if ENABLE_VM_MIPS
  for (int kb = 4; kb <= 64; kb = kb*4) {
    // Generate loops that are 4KB, 16KB and 64KB in size
    char buff[100];
    sprintf(buff, "vm_mips_%d", kb);
    bind(buff);

    for (int i = kb*1024; i > 0; i -= 16) {
      // each loop produces 16 bytes of code
      add(r0, r0, imm(0));
      add(r0, r0, imm(0));

      sub(r0, r0, imm(1), set_CC);
      if (i != 16) {
        add(r0, r0, imm(1));
      } else {
        b(buff, ne);
      }
    }
    jmpx(lr);
    sprintf(buff, "vm_mips_%d_end", kb);
    bind(buff);
  }
#endif
}

#endif

#endif /*#if !ENABLE_THUMB_COMPILER*/
