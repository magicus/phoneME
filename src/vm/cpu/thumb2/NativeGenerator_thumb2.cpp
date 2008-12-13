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
#ifndef PRODUCT
#include "incls/_NativeGenerator_thumb2.cpp.incl"


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
  RegisterSet LowTarget  = set(r0);
  RegisterSet HighTarget = set(r1);

  // Where the first item popped off the stack goes
  RegisterSet A = JavaStackDirection < 0 ?  LowTarget  : HighTarget;
  // Where the second item popped off the stack goes
  RegisterSet B = JavaStackDirection < 0 ?  HighTarget : LowTarget;
  
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
  leavex();
  bl(label_sin);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);

  // Generate cos entry.
  bind_rom_linkable("native_math_cos_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  leavex();
  bl(label_cos);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);

  // Generate tan entry.
  bind_rom_linkable("native_math_tan_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  leavex();
  bl(label_tan);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);

  // Generate square root entry.
  bind_rom_linkable("native_math_sqrt_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  leavex();
  bl(label_sqrt);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);

  // Generate ceil entry.
  bind_rom_linkable("native_math_ceil_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  leavex();
  bl(label_ceil);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);

  // Generate floor entry.
  bind_rom_linkable("native_math_floor_entry");
  pop(A); pop(B);
  mov(return_reg, reg(lr));
  leavex();
  bl(label_floor);
  enterx();
  set_return_type(T_DOUBLE);
  bx(return_reg);
#endif // ENABLE_FLOAT
}

// Note: this routine can be more optimized
void NativeGenerator::generate_native_string_entries() {
  Segment seg(this, code_segment, "Native entry points for string functions");
  const Register return_reg = r7;

  //----------------------java.lang.String.charAt---------------------------
  {
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
    ldr(array, string, String::value_offset());

    comment("r%d = offset", offset);
    ldr(offset, imm_index(string, String::offset_offset()));

    comment("if ((unsigned int) index >= (unsigned int) count) goto bailout;");
    cmp(index, count);
    b(bailout, hs);


    comment("return: r%d = array[index + offset]", tos_val);
    add(index, index, reg(offset));
    add(array, array, imm_shift(index, lsl, 1));
    ldrh_imm12_w(tos_val, array, Array::base_offset());
    set_return_type(T_INT);

    comment("remove arguments from the stack");
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);

    mov(pc, lr);

  bind(bailout);
    comment("Bail out to the general startsWith implementation");
    ldr_using_gp(tmp2, "interpreter_method_entry");
    mov(pc, tmp2);
  }

  //----------------------java.lang.String(java.lang.StringBuffer)--------------
  {
  bind_rom_linkable("native_string_init_entry");
    wtk_profile_quick_call(/* param size */2);

    comment("Bail out to the general implementation");
    ldr_using_gp(tmp2, "interpreter_method_entry");
    mov(pc, tmp2);
  }

  //----------------------java.lang.String.equals(java.lang.Object)-------------
  {
  bind_rom_linkable("native_string_equals_entry");
    wtk_profile_quick_call(/* param size */2);

    comment("Bail out to the general implementation");
    ldr_using_gp(tmp2, "interpreter_method_entry");
    mov(pc, tmp2);
  }

  //----------------------java.lang.String.indexOf(java.lang.String)------------
  {
  bind_rom_linkable("native_string_indexof0_string_entry");
    wtk_profile_quick_call(/* param size */2);

    comment("Bail out to the general indexOf implementation");
    ldr_using_gp(tmp2, "interpreter_method_entry");
    mov(pc, tmp2);
  }

  //----------------------java.lang.String.indexOf(java.lang.String, int)-------
  {
  bind_rom_linkable("native_string_indexof_string_entry");
    wtk_profile_quick_call(/* param size */3);

    comment("Bail out to the general indexOf implementation");
    ldr_using_gp(tmp2, "interpreter_method_entry");
    mov(pc, tmp2);
  }

  //----------------------java.lang.String.indexOf--------------------------

  {
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
      //could we support negative to postindex???
      Macros::ldr(from_index, jsp, PostIndex(-SignedBytesPerStackElement));
    } else {
      SHOULD_NOT_REACH_HERE(); // untested
      ldr(from_index, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
      add_imm(jsp, jsp, - SignedBytesPerStackElement);
    }
    cmp(from_index, imm12(zero));
    it(lt);
    mov(from_index, imm12(zero));
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
    mov(from_index, imm12(zero));

  bind(native_string_indexof_continue);
    if (JavaFrame::arg_offset_from_sp(0) == 0) { 
      Macros::ldr(ch,     jsp, PostIndex(-SignedBytesPerStackElement));
      Macros::ldr(string, jsp, PostIndex(-SignedBytesPerStackElement));  
    } else {
      SHOULD_NOT_REACH_HERE(); // untested
      ldr(ch,     imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));
      ldr(string, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));
      add(jsp, jsp, -2 * SignedBytesPerStackElement);
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
    ldrh(from_index, array, 0);
    add(array, array, imm(2));
    cmp(from_index, reg(ch));
    b(success, eq);

  bind(test);
    cmp(array, reg(max));
    b(loop, lt);
  
  bind(failure);
    comment("return -1;");
    mov(tos_val, -1);
    set_return_type(T_INT);
    comment("continue in caller");
    mov(pc, lr);

  bind(success);
    comment("return i - offset;");
    sub(tos_val, array, reg(tmp4));    
    mov_w(tos_val, tos_val, lsr_shift, 1);
    sub(tos_val, tos_val, imm(1)); // array has been post-indexed
    set_return_type(T_INT);
    comment("continue in caller");
    mov(pc, lr);
  }
#if 0 // BEGIN_CONVERT_TO_T2
  //----------------------java.lang.String.compareTo--------------------------

  { Label loop, done, error;
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
    
    // Warning:  We cannot bash tos_val until after we know we don't have 
    // an errors, since it contains the method
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
    comment("We have some sort of error");
    push(tmp01);
    push(tmp23);
    b_w("interpreter_method_entry");
  }
#endif

  //----------------------java.lang.String.{startsWith,endsWith}---------------

  { 
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

    // Not yet implemented
    ldr_using_gp(pc, "interpreter_method_entry");
#if 0 // BEGIN_CONVERT_TO_T2
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
#endif

    // java.lang.String.startsWith
    // Method boolean startsWith(java.lang.String prefix)

    Label native_string_startsWith0_continue;
    bind_rom_linkable("native_string_startsWith0_entry");

    // Not yet implemented
    ldr_using_gp(pc, "interpreter_method_entry");
#if 0 // BEGIN_CONVERT_TO_T2
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
#endif

    // java.lang.String.startsWith
    // Method boolean startsWith(java.lang.String prefix, int toffset)
    bind_rom_linkable("native_string_startsWith_entry");

    // Not yet implemented
    ldr_using_gp(pc, "interpreter_method_entry");
#if 0 // BEGIN_CONVERT_TO_T2
  
    wtk_profile_quick_call(/* param size */ 3);

    const int SignedBytesPerStackElement =
                JavaStackDirection * BytesPerStackElement;

    GUARANTEE((JavaFrame::arg_offset_from_sp(0) == 0), 
              "Unimplemented for non-zero offset");

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

    //We've got the return address in tmp4, and need to save it before
    // calling memcmp...
    stmfd(sp, set(return_address), writeback);
    bl(label_memcmp);
    ldmfd(sp, set(return_address), writeback);
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
#endif // END_CONVERT_TO_T2
  }
}

void NativeGenerator::generate_native_system_entries() {
  Segment seg(this, code_segment, "Native entry points for system functions");
  Label bailout;
  Label done;
  const Register return_reg = r7;

  //  public static native void arraycopy(Object src, int src_position,
  //                                      Object dst, int dst_position,
  //                                      int length);

  bind_rom_linkable("native_jvm_unchecked_byte_arraycopy_entry");
  bind_rom_linkable("native_jvm_unchecked_char_arraycopy_entry");
  bind_rom_linkable("native_jvm_unchecked_int_arraycopy_entry");
  bind_rom_linkable("native_jvm_unchecked_long_arraycopy_entry");
  bind_rom_linkable("native_jvm_unchecked_obj_arraycopy_entry");

  bind_rom_linkable("native_system_arraycopy_entry");  
  wtk_profile_quick_call(/* param size */ 5);

  // register r0      holds the method to invoke
  // register lr=bcp  holds the return address
  // We cannot bash it until we know that we have no error.
  Assembler::Register length  = tmp0;
  Assembler::Register src_pos = tmp1;
  Assembler::Register dst_pos = tmp2;
  Assembler::Register t1      = tmp3;
  Assembler::Register t2      = tmp4;
  Assembler::Register t3      = tmp5; // bcode
  Assembler::Register src     = tos_tag;
  Assembler::Register dst     = locals;

  // Used only after we know we aren't calling the method the slow path
  Assembler::Register m1      = tos_val;

  comment("load arguments to registers");

  comment("r%d = length", length);
  ldr(length, imm_index(jsp, JavaFrame::arg_offset_from_sp(0)));

  comment("r%d = dst_pos", dst_pos);
  ldr(dst_pos, imm_index(jsp, JavaFrame::arg_offset_from_sp(1)));

  comment("r%d = dst", dst);
  ldr(dst, imm_index(jsp, JavaFrame::arg_offset_from_sp(2)));

  comment("r%d = src_pos", src_pos);
  ldr(src_pos, imm_index(jsp, JavaFrame::arg_offset_from_sp(3)));

  comment("r%d = src", src);
  ldr(src, imm_index(jsp, JavaFrame::arg_offset_from_sp(4)));

  comment("if (src == NULL || dst == NULL) goto bailout;");
  /*cmp_w(src, imm12(0)); there are info about IT behavior in the spec.
  it(ne, ELSE);
  cmp_w(dst, imm12(0));
  b_w(bailout); */
  cmp(src, imm12(0)); 
  b_w(bailout, eq); 
  cmp(dst, imm12(0));
  b_w(bailout, eq); 

  // Since we specifically copy from low index to high index without
  // using memcpy, we can handle src_pos >= dst_pos
  comment("if (src == dst) && (src_pos < dst_pos) goto bailout;");
  Label src_ne_dst;
  cmp(src, reg(dst));
  b_w(src_ne_dst, ne);
  cmp(src_pos, reg(dst_pos));
  b_w(bailout, lt);
bind(src_ne_dst);

  comment("Get src.klass() and dst.klass()");
  ldr(t1, imm_index(src, Oop::klass_offset()));
  ldr(t2, imm_index(dst, Oop::klass_offset()));
  ldr(t1, imm_index(t1, Oop::klass_offset()));
  ldr(t2, imm_index(t2, Oop::klass_offset()));

  comment("if (length < 0 || src_pos < 0 || dst_pos < 0) goto bailout;");
  orr_w(t3, length, src_pos);
  orr_w(t3, t3, dst_pos, lsl_shift, 0, set_CC); 
  b_w(bailout, mi);
  
  comment("if (src.klass() != dst.klass()) goto bailout;");
  cmp(t1, t2);
  b_w(bailout, ne);

  comment("Make sure we really have an array.");
  comment("Java objects have an instance size >= 0");
  ldrsh_imm12_w(t3, t1, FarClass::instance_size_offset());
  cmp(t3, imm12(zero));
  b_w(bailout, ge);

  comment("if ((unsigned int) src.length < (unsigned int) src_pos + (unsigned int) length) goto bailout;");
  ldr(t1, imm_index(src, Array::length_offset()));
  add(t2, src_pos, length);
  cmp_w(t1, t2);
  b_w(bailout, lo);

  comment("if ((unsigned int) dst.length < (unsigned int) dst_pos + (unsigned int) length) goto bailout;");
  ldr(t1, imm_index(dst, Array::length_offset()));
  add(t2, dst_pos, length);
  cmp_w(t1, t2);
  b_w(bailout, lo);

  // size_object_array       = -1
  // size_type_array_1       = -2,   // instance is boolean[], byte[]
  // size_type_array_2       = -3,   // instance is short[], char[]
  // size_type_array_4       = -4,   // instance is int[], float[]
  // size_type_array_8       = -5,   // instance is long[], double[]
  // Convert these into an appropriate shift amount

  comment("remove arguments from the stack");
  add_imm12_w(jsp, jsp, imm12(-JavaStackDirection * 5 * BytesPerStackElement));

  comment("Point at actual data");
  add_imm12_w(src, src, imm12(Array::base_offset()));
  add_imm12_w(dst, dst, imm12(Array::base_offset()));
  
  comment("Jump to appropriate copy routine if length > 0");
  sub_imm12_w(length, length, imm12(one), set_CC);  
  b_w(done, lt);

  comment("Length > 0.  Jump to appropriate routine;");
  // t3 in the range -5 .. -1
  // IMPL_NOTE: optimize to a table jump

  Label copy_type_array[4], copy_object_array, copy_int_array, copy_illegal;
  cmn(t3, imm12(1)); b_w(copy_object_array,  eq); // size_obj_array
  cmn(t3, imm12(2)); b_w(copy_type_array[0], eq); // size_type_array_1
  cmn(t3, imm12(3)); b_w(copy_type_array[1], eq); // size_type_array_2
  cmn(t3, imm12(4)); b_w(copy_type_array[2], eq); // size_type_array_4
  cmn(t3, imm12(5)); b_w(copy_type_array[3], eq); // size_type_array_8
  b_w(copy_illegal);                                // unused

  if (OptimizeArrayCopy && 0) {
#if ENABLE_FAST_MEM_ROUTINES
    Label label_memcpy("jvm_memcpy");
#else
    Label label_memcpy("memcpy");   import(label_memcpy);
#endif

    for (int i = 0; i <= 3; i++) { 
      if( i == 2 ) {
        bind(copy_int_array);
      }
     bind(copy_type_array[i]);
      comment("Each element of the array is %d byte(s)", 1 << i);
      add(src, src, src_pos, lsl_shift, i);
      add(dst, dst, dst_pos, lsl_shift, i);
         
      mov(r0, dst); mov(r1, src);
      add(length, length, imm12(one));
      mov_w(r2, length, lsl_shift, i);
   
      mov(return_reg, lr);
      bl(label_memcpy);
      comment("fill the top of stack cache and return");
      set_return_type(T_VOID);
      bx(return_reg);
    }
  } else {
    for (int i = 0; i <= 3; i++) { 
      Label again;
      bind(copy_type_array[i]);
        comment("Each element of the array is %d byte(s)", 1 << i);
        cmp(length, imm12(zero));
        add(src, src, imm_shift(src_pos, lsl, i));
        add(dst, dst, imm_shift(dst_pos, lsl, i));

      bind(again);
        switch(i) { 
        case 0:
                  Macros::ldrb(t1, src, PostIndex(1));
          it(gt); Macros::ldrb(t2, src, PostIndex(1));
                  Macros::strb(t1, dst, PostIndex(1));
          it(gt); Macros::strb(t2, dst, PostIndex(1));
          break;
        case 1:
                  Macros::ldrh(t1, src, PostIndex(2));
          it(gt); Macros::ldrh(t2, src, PostIndex(2));
                  Macros::strh(t1, dst, PostIndex(2));
          it(gt); Macros::strh(t2, dst, PostIndex(2));
          break;
        case 2:
                  Macros::ldr(t1, src, PostIndex(4));
          it(gt); Macros::ldr(t2, src, PostIndex(4));
                  Macros::str(t1, dst, PostIndex(4));
          it(gt); Macros::str(t2, dst, PostIndex(4));
          break;
        case 3:
                  Macros::ldr(t1,      src, PostIndex(4));
                  Macros::ldr(t2,      src, PostIndex(4));
          it(gt, THEN);
                  Macros::ldr(src_pos, src, PostIndex(4));
                  Macros::ldr(dst_pos, src, PostIndex(4));
                  Macros::str(t1,      dst, PostIndex(4));
                  Macros::str(t2,      dst, PostIndex(4));
          it(gt, THEN);
                  Macros::str(src_pos, dst, PostIndex(4));
                  Macros::str(dst_pos, dst, PostIndex(4));
          break;
        }
        sub_imm12_w(length, length, imm12(2), set_CC);
        b_w(again, ge);
        comment("fill the top of stack cache and return");
        set_return_type(T_VOID);
        bx(lr);
      }
  }
  
  Label copy_object_array_loop;
  Label aligned_copy_loop;
  Label not_yet_aligned_copy;
  Label small_length_copy;

bind(copy_object_array);  
  comment("Check whether dst is in old space");
  get_old_generation_end(t3);
  cmp_w(dst, t3);
  b_w(copy_type_array[2], ge);

  comment("Copy elements of an object array");
  add(src, src, imm_shift(src_pos, lsl, times_4));
  add(dst, dst, imm_shift(dst_pos, lsl, times_4));

bind(copy_object_array_loop);
  comment("%s is one less than the number of elements still to copy", 
          reg_name(length));
  comment("Are we copying at least eight elements?");
  cmp(length, imm12(7));
  b_w(small_length_copy, lt);
       
  comment("Is the destination aligned?");
  tst(dst, imm12(right_n_bits(LogBitsPerByte) << LogBytesPerWord));
  b_w(not_yet_aligned_copy, ne);

  comment("We have at least 8 words to copy, and the dst is aligned");
  get_bitvector_base(t3);
  mov(m1, -1);


bind(aligned_copy_loop);
  // set the byte at little-endian address t3 + (dst >> 5) to -1
#if !0
  //IMPL_NOTE: consider whether it should be fixed!
  if (HARDWARE_LITTLE_ENDIAN) {  
    //we can use t1 here cause it is loaded later
    add(t1, t3, dst, lsr_shift, LogBitsPerByte + LogBytesPerWord);
    strb_imm12_w(m1, t1, 0);
   } else { 
    // We need to convert little-endian byte address to big-endian
    add(t1, t3, imm_shift(dst, lsr, LogBitsPerByte + LogBytesPerWord));
    eor_imm12_w(t1, t1, imm12(3));
    strb(m1, imm_index(t1, 0));
  }
#else
  comment("not immplemented");
  breakpoint(); // NOT IMPLEMENTED
#endif
  // ARM: Address4 four_regs = set(src_pos, dst_pos, t1, t2);
  // ARM: ldmia(src, four_regs, writeback);
  // ARM: stmia(dst, four_regs, writeback);
  Macros::ldr(src_pos, src, PostIndex(4));
  Macros::ldr(dst_pos, src, PostIndex(4));
  Macros::ldr(t1,      src, PostIndex(4));
  Macros::ldr(t2,      src, PostIndex(4));
  Macros::str(src_pos, dst, PostIndex(4));
  Macros::str(dst_pos, dst, PostIndex(4));
  Macros::str(t1,      dst, PostIndex(4));
  Macros::str(t2,      dst, PostIndex(4));

  // ARM: ldmia(src, four_regs, writeback);
  // ARM: stmia(dst, four_regs, writeback);
  Macros::ldr(src_pos, src, PostIndex(4));
  Macros::ldr(dst_pos, src, PostIndex(4));
  Macros::ldr(t1,      src, PostIndex(4));
  Macros::ldr(t2,      src, PostIndex(4));
  Macros::str(src_pos, dst, PostIndex(4));
  Macros::str(dst_pos, dst, PostIndex(4));
  Macros::str(t1,      dst, PostIndex(4));
  Macros::str(t2,      dst, PostIndex(4));

  sub_imm12_w(length, length, imm12(8), set_CC);
  b(done, lt);
  cmp(length, imm12(7));
  b(aligned_copy_loop, ge);

bind(small_length_copy);
  comment("Copying less than 8 elements");
  Macros::ldr(t2, src, PostIndex(BytesPerWord));
  mov(t1, dst);
  Macros::str(t2, dst, PostIndex(BytesPerWord));
  oop_write_barrier(t1, t2, src_pos, dst_pos, false);
  sub_imm12_w(length, length, imm12(one), set_CC);
  b_w(small_length_copy, ge);
bind(done);
  set_return_type(T_VOID);
  bx(lr);

bind(not_yet_aligned_copy);
  comment("length >= 8, but dst not aligned");
  Macros::ldr(t2, src, PostIndex(BytesPerWord));
  mov(t1, dst);
  Macros::str(t2, dst, PostIndex(BytesPerWord));
  oop_write_barrier(t1, t2, src_pos, dst_pos, false);
  sub(length, length, imm12(one));
  b(copy_object_array_loop);

bind(copy_illegal);
  comment("Shouldn't call this");  
  breakpoint();

bind(bailout);
  comment("Bail out to the general arraycopy implementation");
  ldr_using_gp(pc, "interpreter_method_entry");//was b "interpreter_method_entry"
}

void NativeGenerator::generate_native_thread_entries() {
}

void NativeGenerator::generate_native_misc_entries() {
  Segment seg(this, code_segment, "Native entry points for miscellaneous functions");

  //----------------------java.lang.StringBuffer.append-----------------------

  {
    bind_rom_linkable("native_stringbuffer_append_entry");

    // not implemented yet
    ldr_using_gp(pc, "interpreter_method_entry");

#if 0 // BEGIN_CONVERT_TO_T2

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
    strh(thechar, imm_index3(buf, Array::base_offset()));
    str(count, imm_index(obj, Instance::header_size()+sizeof(jobject)));

    comment("Return the object.");
    mov(return_value, reg(obj));
    set_return_type(T_OBJECT);
    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    jmpx(lr);
#endif // END_CONVERT_TO_T2
  }

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
    cmp(index, obj_size);
    it(cs);
    ldr_using_gp(pc, "interpreter_method_entry");//b("interpreter_method_entry"); //unsigned check
  
    comment("Check the lock bit.");
    tst(lock, imm12(one));
    it(eq, ELSE);
    ldr(v_temp, imm_index(obj, Instance::header_size()));
    ldr_using_gp(pc, "interpreter_method_entry");//b("interpreter_method_entry");

    comment("Get the object and return.");
    ldr_indexed(return_value,v_temp,index,times_4, Array::base_offset());
    set_return_type(T_OBJECT);
    comment("continue in caller");

    add_imm(jsp, jsp, -JavaStackDirection * 2 * BytesPerStackElement);
    mov(pc, lr);
  }

  {
    bind_rom_linkable("native_string_substringI_entry");
    ldr_using_gp(pc, "interpreter_method_entry");
  }

  {
    bind_rom_linkable("native_string_substringII_entry");
    ldr_using_gp(pc, "interpreter_method_entry");
  }

  {
    bind_rom_linkable("native_vector_addElement_entry");
    ldr_using_gp(pc, "interpreter_method_entry");
  }

  {
    bind_rom_linkable("native_integer_toString_entry");
    ldr_using_gp(pc, "interpreter_method_entry");
  }
}

#endif

#endif /*#if !ENABLE_THUMB_COMPILER*/
