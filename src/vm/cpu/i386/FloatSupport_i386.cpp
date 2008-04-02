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
#include "incls/_FloatSupport_i386.cpp.incl"

#if ENABLE_FLOAT

extern "C" {

jfloat  jvm_fadd(jfloat x, jfloat y)     { return x + y; }
jfloat  jvm_fsub(jfloat x, jfloat y)     { return x - y; }
jfloat  jvm_fmul(jfloat x, jfloat y)     { return x * y; }
jfloat  jvm_fdiv(jfloat x, jfloat y)     { return x / y; }

jdouble jvm_dadd(jdouble x, jdouble y)   { return (x + y); }
jdouble jvm_dsub(jdouble x, jdouble y)   { return (x - y); }
jdouble jvm_dmul(jdouble x, jdouble y)   { return
 (x * y); }
jdouble jvm_ddiv(jdouble x, jdouble y)   { return (x / y); }
jdouble jvm_i2d(jint x)                  { return (jdouble)x; }

jint    jvm_d2i(jdouble x)               { return (jint)x; }
jdouble jvm_f2d(jfloat x)                { return (jdouble)x; }
jlong   jvm_d2l(jdouble x)               { return (jlong)x; }
jdouble jvm_l2d(jlong x)                 { return (jdouble)x; }
jfloat  jvm_d2f(jdouble x)               { return (jfloat)x; }
jfloat  jvm_i2f(jint x)                  { return (jfloat)x; }
jint    jvm_f2i(jfloat x)                { return (jint)x; }
jfloat  jvm_l2f(jlong x)                 { return (jfloat)x; }
jlong   jvm_f2l(jfloat x)                { return (jlong)x; }

jint    jvm_fcmpl(jfloat x, jfloat y)    { return ((x > y) ? 1 : ( x == y) ? 0 : -1); }
jint    jvm_fcmpg(jfloat x, jfloat y)    { return ((x > y)   ?  1   : 
                                                   (x == y)  ?  0 : 
                                                   (x < y)   ? -1 : 1); }
jint    jvm_dcmpl(jdouble x, jdouble y)  { return  ((x > y) ? 1 : ( x == y) ? 0 : -1); }
jint    jvm_dcmpg(jdouble x, jdouble y)  { return ((x > y)   ?  1   : 
                                                   (x == y)  ?  0 : 
                                                   (x < y)   ? -1 : 1); }

jdouble jvm_dneg(jdouble x)              { return (-x); }

}  // extern C

#endif // ENABLE_FLOAT

#if ENABLE_INTERPRETER_GENERATOR

void FloatSupport::generate() {}

// this auxiliary class is used to change and restore rounding mode
// as FPU CW is stored in global, we couldn't use it recursively,
// but we also shouldn't, as it's only for purposes of bytecode
// generator
class FPUControl : public SourceMacros {
 public:
  FPUControl(const SourceMacros & other, int mode) : SourceMacros(other) {
    set_fpu_control_word(mode);
  }

  FPUControl(const SourceMacros & other) : SourceMacros(other) {
    // Default rounding mode, round to the nearest with double
    // precision
    set_fpu_control_word(0x027F);
  }

  void restore() {
    fldcw(Address(Constant("local_saved_cw"))); // Restore rounding mode
  }

 private:
  void set_fpu_control_word(int mode) {
    fstcw(Address(Constant("local_saved_cw")));
    fstcw(Address(Constant("local_masked_cw")));
    // we'd better use explicit setting
    movl(Address(Constant("local_masked_cw")), Constant(mode));
    fldcw(Address(Constant("local_masked_cw"))); // Change rounding mode
  }
};

void bc_i2f::generate() {
  int offset = 0;
  FPUControl mode(*this);
  pop_to_fpu_stack(int_tag, offset);
  push_from_fpu_stack(float_tag, offset);
  mode.restore();
}

void bc_l2d::generate() {
  int offset = 0;
  FPUControl mode(*this);
  pop_to_fpu_stack(long_tag, offset);
  push_from_fpu_stack(double_tag, offset);
  mode.restore();
}

void bc_l2f::generate() {
  int offset = 0;
  FPUControl mode(*this);
  pop_to_fpu_stack(long_tag, offset);
  push_from_fpu_stack(float_tag, offset);
  mode.restore();
}

void bc_f2d::generate() {
  int offset = 0;
  FPUControl mode(*this);
  pop_to_fpu_stack(float_tag, offset);
  push_from_fpu_stack(double_tag, offset);
  mode.restore();
}

void bc_f2l::generate() {
  Label nan,
        neg_nan,
        pos_inf,
        neg_inf,
        nan_inf_done,
        convert,
        done;

  comment("F2L Checking for NaNs and Infinity");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  andl(eax, Constant(0x7FFFFFFF));
  cmpl(eax, Constant(0x5F000000));
  jcc(less, Constant(convert));

  comment("Check for NaNs");
  pop_float(eax, eax);
  cmpl(eax, Constant(0x7F800001));
  jcc(less, Constant(neg_nan));
  cmpl(eax, Constant(0x7FFFFFFF));
  jcc(less_equal, Constant(nan));
  bind(neg_nan);
  cmpl(eax, Constant(0xFF800001));
  jcc(below, Constant(pos_inf));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));

  comment("NaN");
  bind(nan);
  movl(edx, Constant(0));
  movl(eax, Constant(0));
  jmp(Constant(nan_inf_done));

  comment("Positive Infinity");
  bind(pos_inf);
  cmpl(eax, Constant(0));
  jcc(less_equal, Constant(neg_inf));
  movl(edx, Constant(0x7FFFFFFF));
  movl(eax, Constant(0xFFFFFFFF));
  jmp(Constant(nan_inf_done));

  comment("Negative Infinity");
  bind(neg_inf);
  cmpl(eax, Constant(0));
  jcc(greater_equal, Constant(nan_inf_done));
  movl(edx, Constant(0x80000000));
  movl(eax, Constant(0x00000000));

  bind(nan_inf_done);
  push_long(eax, edx);
  jmp(Constant(done));

  comment("F2L conversion");
  bind(convert);
  int offset = 0;
  FPUControl mode(*this, 0x0E7F);

  pop_to_fpu_stack(float_tag, offset);
  push_from_fpu_stack(long_tag, offset);
  mode.restore();

  bind(done);
}

void bc_f2i::generate() {
  Label nan,
        neg_nan,
        pos_inf,
        neg_inf,
        nan_inf_done,
        convert,
        done;

  comment("F2I Checking for NaNs and Infinity");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  andl(eax, Constant(0x7FFFFFFF));
  cmpl(eax, Constant(0x4F000000));
  jcc(less, Constant(convert));

  comment("Check for NaNs");
  pop_float(eax, eax);
  cmpl(eax, Constant(0x7F800001));
  jcc(less, Constant(neg_nan));
  cmpl(eax, Constant(0x7FFFFFFF));
  jcc(less_equal, Constant(nan));
  bind(neg_nan);
  cmpl(eax, Constant(0xFF800001));
  jcc(below, Constant(pos_inf));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));

  comment("NaN");
  bind(nan);
  movl(eax, Constant(0));
  jmp(Constant(nan_inf_done));

  comment("Positive Infinity");
  bind(pos_inf);
  cmpl(eax, Constant(0));
  jcc(less_equal, Constant(neg_inf));
  movl(eax, Constant(0x7FFFFFFF));
  jmp(Constant(nan_inf_done));

  comment("Negative Infinity");
  bind(neg_inf);
  cmpl(eax, Constant(0));
  jcc(greater_equal, Constant(nan_inf_done));
  movl(eax, Constant(0x80000000));

  bind(nan_inf_done);
  push_int(eax);
  jmp(Constant(done));

  comment("F2I conversion");
  bind(convert);
  int offset = 0;
  FPUControl mode(*this, 0x0E7F);

  pop_to_fpu_stack(float_tag, offset);
  push_from_fpu_stack(int_tag, offset);
  mode.restore();

  bind(done);
}

void bc_d2f::generate() {
  int offset = 0;
  FPUControl mode(*this);
  pop_to_fpu_stack(double_tag, offset);
  push_from_fpu_stack(float_tag, offset);
  mode.restore();
}

void bc_d2l::generate() {
  Label check_nan_inf,
        nan,
        pos_nan,
        neg_nan,
        neg_nan2,
        pos_inf,
        pos_inf2,
        neg_inf,
        neg_inf2,
        nan_inf_done,
        convert,
        done;

  comment("D2L Checking for NaNs and Infinity");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  andl(edx, Constant(0x7FFFFFFF));
  cmpl(edx, Constant(0x43E00000));
  jcc(less, Constant(convert));
  jcc(greater, Constant(check_nan_inf));
  cmpl(eax, Constant(0x00000000));
  jcc(below, Constant(convert));

  comment("Check for NaNs");
  bind(check_nan_inf);
  pop_double(eax, edx);
  cmpl(edx, Constant(0x7FF00000));
  jcc(less, Constant(neg_nan));
  jcc(greater, Constant(pos_nan));
  cmpl(eax, Constant(0x00000001));
  jcc(below, Constant(neg_nan));

  bind(pos_nan);
  cmpl(edx, Constant(0x7FFFFFFF));
  jcc(less, Constant(nan));
  jcc(greater, Constant(neg_nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(neg_nan);
  cmpl(edx, Constant(0xFFF00000));
  jcc(below, Constant(pos_inf));
  jcc(above, Constant(neg_nan2));
  cmpl(eax,  Constant(0x00000001));
  jcc(below, Constant(pos_inf));

  bind(neg_nan2);
  cmpl(edx, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));
  jcc(below, Constant(nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));

  comment("NaN");
  bind(nan);
  movl(edx, Constant(0));
  movl(eax, Constant(0));
  jmp(Constant(nan_inf_done));

  comment("Positive Infinity");
  bind(pos_inf);
  cmpl(edx, Constant(0));
  jcc(less, Constant(neg_inf));
  jcc(greater, Constant(pos_inf2));
  cmpl(eax, Constant(0));
  jcc(below_equal, Constant(neg_inf));

  bind(pos_inf2);
  movl(edx, Constant(0x7FFFFFFF));
  movl(eax, Constant(0xFFFFFFFF));
  jmp(Constant(nan_inf_done));

  comment("Negative Infinity");
  bind(neg_inf);
  cmpl(edx, Constant(0));
  jcc(greater, Constant(nan_inf_done));
  jcc(less, Constant(neg_inf2));
  cmpl(eax, Constant(0));
  jcc(above_equal, Constant(nan_inf_done));

  bind(neg_inf2);
  movl(edx, Constant(0x80000000));
  movl(eax, Constant(0x00000000));

  bind(nan_inf_done);
  push_long(eax, edx);
  jmp(Constant(done));

  comment("D2L conversion");
  bind(convert);
  int offset = 0;
  FPUControl mode(*this, 0x0E7F);

  pop_to_fpu_stack(double_tag, offset);
  push_from_fpu_stack(long_tag, offset);
  mode.restore();

  bind(done);
}

void bc_d2i::generate() {
  Label check_nan_inf,
        nan,
        pos_nan,
        neg_nan,
        neg_nan2,
        pos_inf,
        pos_inf2,
        neg_inf,
        neg_inf2,
        nan_inf_done,
        convert,
        done;

  comment("D2I Checking for NaNs and Infinity");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  andl(edx, Constant(0x7FFFFFFF));
  cmpl(edx, Constant(0x41E00000));
  jcc(less, Constant(convert));
  jcc(greater, Constant(check_nan_inf));
  cmpl(eax, Constant(0x00000000));
  jcc(below, Constant(convert));

  comment("Check for NaNs");
  bind(check_nan_inf);
  pop_double(eax, edx);
  cmpl(edx, Constant(0x7FF00000));
  jcc(less, Constant(neg_nan));
  jcc(greater, Constant(pos_nan));
  cmpl(eax, Constant(0x00000001));
  jcc(below, Constant(neg_nan));

  bind(pos_nan);
  cmpl(edx, Constant(0x7FFFFFFF));
  jcc(less, Constant(nan));
  jcc(greater, Constant(neg_nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(neg_nan);
  cmpl(edx, Constant(0xFFF00000));
  jcc(below, Constant(pos_inf));
  jcc(above, Constant(neg_nan2));
  cmpl(eax,  Constant(0x00000001));
  jcc(below, Constant(pos_inf));

  bind(neg_nan2);
  cmpl(edx, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));
  jcc(below, Constant(nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(above, Constant(pos_inf));

  comment("NaN");
  bind(nan);
  movl(eax, Constant(0));
  jmp(Constant(nan_inf_done));

  comment("Positive Infinity");
  bind(pos_inf);
  cmpl(edx, Constant(0));
  jcc(less, Constant(neg_inf));
  jcc(greater, Constant(pos_inf2));
  cmpl(eax, Constant(0));
  jcc(below_equal, Constant(neg_inf));

  bind(pos_inf2);
  movl(eax, Constant(0x7FFFFFFF));
  jmp(Constant(nan_inf_done));

  comment("Negative Infinity");
  bind(neg_inf);
  cmpl(edx, Constant(0));
  jcc(greater, Constant(nan_inf_done));
  jcc(less, Constant(neg_inf2));
  cmpl(eax, Constant(0));
  jcc(above_equal, Constant(nan_inf_done));

  bind(neg_inf2);
  movl(eax, Constant(0x80000000));

  bind(nan_inf_done);
  push_int(eax);
  jmp(Constant(done));

  comment("D2I conversion");
  bind(convert);
  int offset = 0;
  FPUControl mode(*this, 0x0E7F);

  pop_to_fpu_stack(double_tag, offset);
  push_from_fpu_stack(int_tag, offset);
  mode.restore();

  bind(done);
}

#define FPU_FLOAT_BC(bytecode, insn)                   \
  void bytecode() {                                    \
  int offset = BytesPerStackElement;                   \
  FPUControl mode(*this);                              \
  pop_to_fpu_stack(float_tag, offset);                 \
  offset -= 2 * BytesPerStackElement;                  \
  pop_to_fpu_stack(float_tag, offset);                 \
  offset += BytesPerStackElement;                      \
  insn();                                              \
  push_from_fpu_stack(float_tag, offset, false);       \
  clear_one_from_fpu_stack(float_tag, offset);         \
  mode.restore();                                      \
}

#define FPU_DOUBLE_BC(bytecode, insn)                   \
  void bytecode() {                                     \
  int offset = 2 * BytesPerStackElement;                \
  FPUControl mode(*this);                               \
  pop_to_fpu_stack(double_tag, offset);                 \
  offset -= 4 * BytesPerStackElement;                   \
  pop_to_fpu_stack(double_tag, offset);                 \
  offset += 2 * BytesPerStackElement;                   \
  insn();                                               \
  push_from_fpu_stack(double_tag, offset, false);       \
  clear_one_from_fpu_stack(double_tag, offset);         \
  mode.restore();                                       \
}

FPU_FLOAT_BC(bc_fadd::generate, faddp);
FPU_FLOAT_BC(bc_fsub::generate, fsubp);
FPU_FLOAT_BC(bc_fmul::generate, fmulp);
FPU_FLOAT_BC(bc_fdiv::generate, fdivp);

FPU_DOUBLE_BC(bc_dadd::generate, faddp);
FPU_DOUBLE_BC(bc_dsub::generate, fsubp);

#undef FPU_FLOAT_BC
#undef FPU_DOUBLE_BC

void bc_frem::generate() {
  Label inf,
        frem,
        done;
  comment("FREM checking for Infinity");

  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));

  cmpl(eax, Constant(0x7F800000));
  jcc(equal, Constant(inf));
  cmpl(eax, Constant(0xFF800000));
  jcc(not_equal, Constant(frem));

  bind(inf);
  movl(ecx, edx);  // Copy edx to ecx because andl overwrites
  andl(ecx, Constant(0x7FFFFFFF));
  cmpl(ecx, Constant(0x7F800000));
  jcc(greater_equal, Constant(frem));

  pop_float(eax, eax);
  pop_float(edx, edx);
  push_float(edx);
  jmp(Constant(done));

  comment("FREM calculation");
  bind(frem);
  int offset = BytesPerStackElement;
  FPUControl mode(*this);

  pop_to_fpu_stack(float_tag, offset);
  offset -= 2 * BytesPerStackElement;
  pop_to_fpu_stack(float_tag, offset);
  offset += BytesPerStackElement;;
  fremp();
  push_from_fpu_stack(float_tag, offset, false);
  clear_one_from_fpu_stack(float_tag, offset);

  comment("Results equals the sign of the dividend");
  comment("Retrieve the sign bit");
  andl(edx, Constant(0x80000000));
  cmpl(edx, Constant(0x80000000));
  jcc(not_equal, Constant(done));

  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  andl(edx, Constant(0x80000000));
  cmpl(edx, Constant(0x80000000));
  jcc(equal, Constant(done));

  offset = 0;
  comment("Change the sign bit");
  pop_to_fpu_stack(float_tag, offset);
  fchs();
  push_from_fpu_stack(float_tag, offset, false);
  mode.restore();

  bind(done);

}

void bc_dmul::generate() {
  Label pos,
        neg,
        done;
  int offset = 0;
  FPUControl mode(*this);

  comment("DMUL");
  comment("poping multiplicand\n");
  pop_to_fpu_stack(double_tag, offset);

  comment("Loading 2^-15360");
  fld_t(Constant(neg));

  comment("scale: multiplier * 2^-15360\n");
  fmulp();

  comment("poping multiplier\n");
  pop_to_fpu_stack(double_tag, offset);

  comment("multiply: product * multiplicand\n");
  fmulp();

  comment("Loading 2^15360");
  fld_t(Constant(pos));

  comment("rescale: product and 2^15360\n");
  fmulp();

  comment("pushing results");
  push_from_fpu_stack(double_tag, offset, false);
  clear_one_from_fpu_stack(double_tag, offset);
  jmp(Constant(done));

  bind(neg);  // 2^-15360 = 0x03FF8000000000000000
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x80));
  emit_byte(Constant(0xFF));
  emit_byte(Constant(0x03));

  bind(pos);  // 2^15360 = 0x7BFF8000000000000000
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x80));
  emit_byte(Constant(0xFF));
  emit_byte(Constant(0x7B));

  bind(done);
  mode.restore();
}

void bc_ddiv::generate() {
  Label pos,
        neg,
        done;
  int offset = 0;
  FPUControl mode(*this);

  comment("DDIV");
  comment("poping divisor\n");
  pop_to_fpu_stack(double_tag, offset);

  comment("poping dividend\n");
  pop_to_fpu_stack(double_tag, offset);

  comment("Loading 2^-15360");
  fld_t(Constant(neg));

  comment("scale: dividend * 2^-15360\n");
  fmulp();

  comment("Swap dividend and divisor on the stack");
  fxch();

  comment("divide: dividend / divisor\n");
  fdivp();

  comment("Loading 2^15360");
  fld_t(Constant(pos));

  comment("rescale: quotient and 2^15360\n");
  fmulp();

  comment("pushing results");
  push_from_fpu_stack(double_tag, offset, false);
  clear_one_from_fpu_stack(double_tag, offset);
  jmp(Constant(done));

  bind(neg);  // 2^-15360 = 0x03FF8000000000000000
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x80));
  emit_byte(Constant(0xFF));
  emit_byte(Constant(0x03));

  bind(pos);  // 2^15360 = 0x7BFF8000000000000000
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x00));
  emit_byte(Constant(0x80));
  emit_byte(Constant(0xFF));
  emit_byte(Constant(0x7B));

  bind(done);
  mode.restore();
}

void bc_drem::generate() {
  Label inf,
        inf2,
        lvalue,
        drem,
        sign,
        done;
  comment("DREM checking for Infinity");

  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  movl(ebx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(2))));
  movl(ecx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(3))));

  cmpl(eax, Constant(0x00000000));
  jcc(not_equal, Constant(inf));
  cmpl(edx, Constant(0x7FF00000));
  jcc(equal, Constant(inf2));

  bind(inf);
  cmpl(eax, Constant(0x00000000));
  jcc(not_equal, Constant(drem));
  cmpl(edx, Constant(0xFF000000));
  jcc(not_equal, Constant(drem));

  bind(inf2);
  movl(edx, ecx);  // Copy ecx to edx because andl overwrites
  andl(edx, Constant(0x7FFFFFFF));
  cmpl(edx, Constant(0x7FF00000));
  jcc(greater, Constant(drem));
  jcc(less, Constant(lvalue));
  cmpl(ebx, Constant(0x00000000));
  jcc(above_equal, Constant(drem));

  bind(lvalue);
  pop_double(eax, edx);
  pop_double(ebx, ecx);
  push_double(ebx, ecx);
  jmp(Constant(done));

  comment("DREM calculation");
  bind(drem);
  int offset = 2 * BytesPerStackElement;
  FPUControl mode(*this);

  pop_to_fpu_stack(double_tag, offset);
  offset -= 4 * BytesPerStackElement;
  pop_to_fpu_stack(double_tag, offset);
  offset += 2 * BytesPerStackElement;  
  fremp();
  push_from_fpu_stack(double_tag, offset, false);
  clear_one_from_fpu_stack(double_tag, offset);

  comment("Results equals the sign of the dividend");
  comment("Retrieve the sign bit");
  andl(ebx, Constant(0x00000000));
  andl(ecx, Constant(0x80000000));
  cmpl(ebx, Constant(0x00000000));
  jcc(not_equal, Constant(done));
  cmpl(ecx, Constant(0x80000000));
  jcc(not_equal, Constant(done));

  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  andl(eax, Constant(0x00000000));
  andl(edx, Constant(0x80000000));
  cmpl(eax, Constant(0x00000000));
  jcc(not_equal, Constant(sign));
  cmpl(edx, Constant(0x80000000));
  jcc(equal, Constant(done));

  bind(sign);
  offset = 0;
  comment("Change the sign bit");
  pop_to_fpu_stack(double_tag, offset);
  fchs();
  push_from_fpu_stack(double_tag, offset, false);
  mode.restore();

  bind(done);
}

void bc_fcmp::generate(int nan_value) {
// If this passes TCK NaN checking may not be needed
// It is here because of differences between a P3 and P4
#if 0
  Label r_nan,
        neg_nan,
        neg_nan2,
        nan,
        fcmpop,
        done;

  comment("FCMP checking for NaNs");

  movl(ebx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(ecx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));

  cmpl(ebx, Constant(0x7F800001));
  jcc(less, Constant(neg_nan));
  cmpl(ebx, Constant(0x7FFFFFFF));
  jcc(less_equal, Constant(nan));

  bind(neg_nan);
  cmpl(ebx, Constant(0xFF800001));
  jcc(below, Constant(r_nan));
  cmpl(ebx, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(r_nan);
  cmpl(ecx, Constant(0x7F800001));
  jcc(less, Constant(neg_nan2));
  cmpl(ecx, Constant(0x7FFFFFFF));
  jcc(less_equal, Constant(nan));

  bind(neg_nan2);
  cmpl(ecx, Constant(0xFF800001));
  jcc(below, Constant(fcmpop));
  cmpl(ecx, Constant(0xFFFFFFFF));
  jcc(above, Constant(fcmpop));

  comment("NaN");
  bind(nan);
  pop_float(ebx, ecx);
  movl(ebx, Constant(nan_value));
  push_int(ebx);
  jmp(Constant(done));

  comment("FCMP compare");
  bind(fcmpop);
#endif

  int offset = 0;
  pop_to_fpu_stack(float_tag, offset);
  pop_to_fpu_stack(float_tag, offset);
  addl(esp, Constant(offset));
  fcmp2int(ecx, nan_value < 0);
  push_int(ecx);

#if 0
  bind(done);
#endif
}

void bc_dcmp::generate(int nan_value) {
// If this passes TCK NaN checking may not be needed
// It is here because of differences between a P3 and P4
#if 0
  Label nan,
        pos_nan,
        neg_nan,
        neg_nan2,
        r_nan,
        r_pos_nan,
        r_neg_nan,
        r_neg_nan2,
        dcmpop,
        done;

  comment("DCMP checking for NaNs");
  movl(eax, Address(esp, Constant(JavaFrame::arg_offset_from_sp(0))));
  movl(edx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(1))));
  movl(ebx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(2))));
  movl(ecx, Address(esp, Constant(JavaFrame::arg_offset_from_sp(3))));

  cmpl(edx, Constant(0x7FF00000));
  jcc(less, Constant(neg_nan));
  jcc(greater, Constant(pos_nan));
  cmpl(eax, Constant(0x00000001));
  jcc(below, Constant(neg_nan));

  bind(pos_nan);
  cmpl(edx, Constant(0x7FFFFFFF));
  jcc(less, Constant(nan));
  jcc(greater, Constant(neg_nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(neg_nan);
  cmpl(edx, Constant(0xFFF00000));
  jcc(below, Constant(r_nan));
  jcc(above, Constant(neg_nan2));
  cmpl(eax, Constant(0x00000001));
  jcc(below, Constant(r_nan));

  bind(neg_nan2);
  cmpl(edx, Constant(0xFFFFFFFF));
  jcc(below, Constant(nan));
  jcc(above, Constant(r_nan));
  cmpl(eax, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(r_nan);
  cmpl(ecx, Constant(0x7FF00000));
  jcc(less, Constant(r_neg_nan));
  jcc(greater, Constant(r_pos_nan));
  cmpl(ebx, Constant(0x00000001));
  jcc(below, Constant(r_neg_nan));

  bind(r_pos_nan);
  cmpl(ecx, Constant(0x7FFFFFFF));
  jcc(less, Constant(nan));
  jcc(greater, Constant(r_pos_nan));
  cmpl(ebx, Constant(0xFFFFFFFF));
  jcc(below_equal, Constant(nan));

  bind(r_neg_nan);
  cmpl(ecx, Constant(0xFFF00000));
  jcc(below, Constant(dcmpop));
  jcc(above, Constant(r_neg_nan2));
  cmpl(ebx, Constant(0x00000001));
  jcc(below, Constant(dcmpop));

  bind(r_neg_nan2);
  cmpl(ecx, Constant(0xFFFFFFFF));
  jcc(above, Constant(dcmpop));
  jcc(below, Constant(nan));
  cmpl(ebx, Constant(0xFFFFFFFF));
  jcc(above, Constant(dcmpop));

  comment("NaN");
  bind(nan);
  pop_double(eax, edx);
  pop_double(ebx, ecx);
  movl(eax, Constant(nan_value));
  push_int(eax);
  jmp(Constant(done));

  comment("DCMP compare");
  bind(dcmpop);
#endif

  int offset = 0;
  pop_to_fpu_stack(double_tag, offset);
  pop_to_fpu_stack(double_tag, offset);
  addl(esp, Constant(offset));
  fcmp2int(ecx, nan_value < 0);
  push_int(ecx);

#if 0
  bind(done);
#endif
}

#endif // ENABLE_INTERPRETER_GENERATOR
