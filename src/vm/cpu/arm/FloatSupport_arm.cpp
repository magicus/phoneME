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
#include "incls/_FloatSupport_arm.cpp.incl"

#if ENABLE_FLOAT

// Defines to set which operations will or will not use SUN JVM FP operations.
#if ENABLE_SOFT_FLOAT

#define USE_SUN_JVM_FADD   1
#define USE_SUN_JVM_FSUB   1
#define USE_SUN_JVM_FMUL   1
#define USE_SUN_JVM_FDIV   1
#define USE_SUN_JVM_FREM   1
#define USE_SUN_JVM_DCMP   1
#define USE_SUN_JVM_FCMP   1
#define USE_SUN_JVM_DADD   1
#define USE_SUN_JVM_DSUB   1
#define USE_SUN_JVM_DMUL   1
#define USE_SUN_JVM_DDIV   1
#define USE_SUN_JVM_DREM   1
#define USE_SUN_JVM_I2D    1
#define USE_SUN_JVM_D2I    1
#define USE_SUN_JVM_I2F    1
#define USE_SUN_JVM_F2I    1
#define USE_SUN_JVM_L2D    1
#define USE_SUN_JVM_D2L    1
#define USE_SUN_JVM_F2D    1
#define USE_SUN_JVM_D2F    1
#define USE_SUN_JVM_L2F    1
#define USE_SUN_JVM_F2L    1
#define USE_SUN_JVM_DNEG   1

#else

#define USE_SUN_JVM_FADD   0
#define USE_SUN_JVM_FSUB   0
#define USE_SUN_JVM_FMUL   0
#define USE_SUN_JVM_FDIV   0
#define USE_SUN_JVM_FREM   0
#define USE_SUN_JVM_DCMP   0
#define USE_SUN_JVM_FCMP   0
#define USE_SUN_JVM_DADD   0
#define USE_SUN_JVM_DSUB   0
#define USE_SUN_JVM_DMUL   0
#define USE_SUN_JVM_DDIV   0
#define USE_SUN_JVM_DREM   0
#define USE_SUN_JVM_I2D    0
#define USE_SUN_JVM_D2I    0
#define USE_SUN_JVM_I2F    0
#define USE_SUN_JVM_F2I    0
#define USE_SUN_JVM_L2D    0
#define USE_SUN_JVM_D2L    0
#define USE_SUN_JVM_F2D    0
#define USE_SUN_JVM_D2F    0
#define USE_SUN_JVM_L2F    0
#define USE_SUN_JVM_F2L    0
#define USE_SUN_JVM_DNEG   0

#endif //   ENABLE_SOFT_FLOAT


#if ENABLE_ARM_VFP
// GCC's implementation of F2L, F2I, D2L, D2I are not Java-compilant. Let's
// use SUN's own software emulation. N.B. These operations do not have
// corresponding VFP instructions.

#undef USE_SUN_JVM_F2I
#define USE_SUN_JVM_F2I 1

#undef USE_SUN_JVM_F2L
#define USE_SUN_JVM_F2L 1

#undef USE_SUN_JVM_D2L
#define USE_SUN_JVM_D2L 1

#undef USE_SUN_JVM_D2I
#define USE_SUN_JVM_D2I 1

#endif


#define floatNaN 0x7fc00000

#if ENABLE_INTERPRETER_GENERATOR

#define STUB(name)                             \
     {                                         \
       Label _name(#name);                     \
       import(_name);                          \
     }

#define GENERATE(condition, name)              \
  if (condition) {                             \
    generate_##name();                         \
  } else {                                     \
    STUB(name);                                \
  }

void FloatSupport::generate() {
  Segment seg(this, code_segment, "Floating point routines");

  GENERATE(USE_SUN_JVM_FADD, jvm_fadd);
  GENERATE(USE_SUN_JVM_FSUB, jvm_fsub);
  GENERATE(USE_SUN_JVM_FMUL, jvm_fmul);
  GENERATE(USE_SUN_JVM_FDIV, jvm_fdiv);
  GENERATE(USE_SUN_JVM_FREM, jvm_frem);

#if USE_SUN_JVM_DCMP
   generate_jvm_dcmp();
#else
   STUB(jvm_dcmpl);
   STUB(jvm_dcmpg);
#endif

#if USE_SUN_JVM_FCMP
   generate_jvm_fcmp();
#else
   STUB(jvm_fcmpl);
   STUB(jvm_fcmpg);
#endif

  GENERATE(USE_SUN_JVM_DADD, jvm_dadd);
  GENERATE(USE_SUN_JVM_DSUB, jvm_dsub);
  GENERATE(USE_SUN_JVM_DMUL, jvm_dmul);
  GENERATE(USE_SUN_JVM_DDIV, jvm_ddiv);
  GENERATE(USE_SUN_JVM_DREM, jvm_drem);
  GENERATE(USE_SUN_JVM_I2D,  jvm_i2d);
  GENERATE(USE_SUN_JVM_I2F,  jvm_i2f);
  GENERATE(USE_SUN_JVM_L2F,  jvm_l2f);
  GENERATE(USE_SUN_JVM_L2D,  jvm_l2d);
  GENERATE(USE_SUN_JVM_F2I,  jvm_f2i);
  GENERATE(USE_SUN_JVM_F2L,  jvm_f2l);
  GENERATE(USE_SUN_JVM_F2D,  jvm_f2d);
  GENERATE(USE_SUN_JVM_D2I,  jvm_d2i);
  GENERATE(USE_SUN_JVM_D2L,  jvm_d2l);
  GENERATE(USE_SUN_JVM_D2F,  jvm_d2f);

#if ENABLE_ARM_VFP
  generate_set_vfp_fast_mode();
#endif
}

#define FLOAT1  r0
#define FLOAT2  r1
#define EXP1    r4
#define EXP2    r5
#define MANT1   r6
#define MANT2   r7
#define EMASK   r12
#define MMASK   r12
#define QUOT    r12
#define ITER    r3
#define FLOAT_SAVE_SET    set(r4, r5, r6, r7)

#define FLOAT_RETURN_TO_CALLER_IF(cond) \
    ldmfd(sp, FLOAT_SAVE_SET, writeback, cond); jmpx(lr, cond)

#define FLOAT_RETURN_TO_CALLER \
   ldmfd(sp, FLOAT_SAVE_SET, writeback); jmpx(lr)

#if ENABLE_ARM_VFP
#define VFP_EMIT_RAW(x)    stream()->print_cr("\t%s", x)
#define VFP_COMMENT(x)     comment(x)
#else
#define VFP_EMIT_RAW(x)
#define VFP_COMMENT(x)
#endif

// Macros used to check if exception occurred in arthmetic operations
// a+a, a*a...
#define VFP_RETURN_IF_NO_EXCEPTION_r2 \
   VFP_COMMENT("return to caller if no FP exception"); \
   VFP_EMIT_RAW("fmrx    r2, FPSCR"); \
   VFP_COMMENT("Check cumulative exceptions IDC|UFC|OFC|DZC|IOC"); \
   VFP_EMIT_RAW("tst     r2, #0x8f"); \
   VFP_EMIT_RAW("bxeq    lr"); \
   VFP_COMMENT("clear cumulative exceptions lazily"); \
   VFP_EMIT_RAW("mov     r2, r2, lsr #8"); \
   VFP_EMIT_RAW("mov     r2, r2, lsl #8"); \
   VFP_EMIT_RAW("fmxr    FPSCR, r2")

// Macros used to check if exception occurred in conversions operations.
// i.e. f2i, d2i...
#define VFP_RETURN_IF_NO_EXCEPTION_r2_conversions \
   VFP_COMMENT("return to caller if no FP exception"); \
   VFP_EMIT_RAW("fmrx    r2, FPSCR"); \
   VFP_COMMENT("Check cumulative exceptions IDC|UFC|OFC|DZC"); \
   VFP_EMIT_RAW("tst     r2, #0x8E"); \
   VFP_EMIT_RAW("bxeq    lr"); \
   VFP_COMMENT("clear cumulative exceptions lazily"); \
   VFP_EMIT_RAW("mov     r2, r2, lsr #8"); \
   VFP_EMIT_RAW("mov     r2, r2, lsl #8"); \
   VFP_EMIT_RAW("fmxr    FPSCR, r2")

// This is an operation for floats in the form
//       r0 := r0 <opcode> r1
#define VFP_FLOAT_2_TO_1(opcode) \
  VFP_COMMENT("try to execute in VFP hardware"); \
  VFP_EMIT_RAW("fmsr    s14, r0"); \
  VFP_EMIT_RAW("fmsr    s15, r1"); \
  VFP_EMIT_RAW(opcode "   s13, s14, s15"); \
  VFP_EMIT_RAW("fmrs    r0, s13"); \
  VFP_RETURN_IF_NO_EXCEPTION_r2; \
  VFP_COMMENT("restore parameters in int registers, and retry in software"); \
  VFP_EMIT_RAW("fmrs    r0, s14")

// This is an operation for doubles in the form
//       r0,r1 := r0,r1 <opcode> r2,r3
#define VFP_DOUBLE_2_TO_1(opcode) \
  VFP_COMMENT("try to execute in VFP hardware"); \
  VFP_EMIT_RAW("fmdrr   d6, r0, r1"); \
  VFP_EMIT_RAW("fmdrr   d7, r2, r3"); \
  VFP_EMIT_RAW(opcode "   d5, d6, d7"); \
  VFP_EMIT_RAW("fmrrd   r0, r1, d5"); \
  VFP_RETURN_IF_NO_EXCEPTION_r2; \
  VFP_COMMENT("restore parameters in int registers, and retry in software"); \
  VFP_EMIT_RAW("fmrrd   r0, r1, d6"); \
  VFP_EMIT_RAW("fmrrd   r2, r3, d7")

// This is an operation for floats in the form
//     r0 := r0
// Used in f2i, i2f
#define VFP_FLOAT_1_TO_1(opcode) \
  VFP_COMMENT("try to execute in VFP hardware"); \
  VFP_EMIT_RAW("fmsr    s15, r0"); \
  VFP_EMIT_RAW(opcode " s14, s15"); \
  VFP_EMIT_RAW("fmrs    r0, s14");  \
  VFP_EMIT_RAW("bx lr");  \
  VFP_RETURN_IF_NO_EXCEPTION_r2_conversions; \
  VFP_COMMENT("restore parameters in int registers, and retry in software"); \
  VFP_EMIT_RAW("fmrs    r0, s15")

// This is an operation for doubles in the form
//     r0 := r0
// Used in d2i, d2f
#define VFP_DOUBLE_1_TO_1(opcode) \
  VFP_COMMENT("try to execute in VFP hardware"); \
  VFP_EMIT_RAW("fmdrr   d1, r0, r1"); \
  VFP_EMIT_RAW(opcode "     s15, d1"); \
  VFP_EMIT_RAW("fmrs    r0, s15"); \
  VFP_RETURN_IF_NO_EXCEPTION_r2_conversions; \
  VFP_COMMENT("restore parameters in int registers, and retry in software"); \
  VFP_EMIT_RAW("fmrrd   r0, r1, d1");

// This is an operation for ints in the form
//     r0 := r0
// Used in i2d, f2d
#define VFP_INT_1_TO_1(opcode) \
  VFP_COMMENT("try to execute in VFP hardware"); \
  VFP_EMIT_RAW("fmsr   s15, r0"); \
  VFP_EMIT_RAW(opcode "    d1, s15"); \
  VFP_EMIT_RAW("fmrrd    r0, r1, d1"); \
  VFP_RETURN_IF_NO_EXCEPTION_r2_conversions; \
  VFP_COMMENT("restore parameters in int registers, and retry in software"); \
  VFP_EMIT_RAW("fmrs   r0, s15");

#if ENABLE_ARM_VFP
void FloatSupport::generate_set_vfp_fast_mode() {
  bind_global("jvm_set_vfp_fast_mode");
    mov(r0, imm(0x03 << 24));
    fmxr(fpscr, r0);
    jmpx(lr);
}
#endif

void FloatSupport::generate_jvm_fadd() {
  Label fadd("jvm_fadd");
  Label floatRoundResult("_floatRoundResult");
  Label done("_floatDone");
  Label floatReturnToCaller("_floatReturnToCaller");
  Label floatReturnInfinity("_floatReturnInfinity");
  Label floatReturnNaN("_floatReturnNaN");
  Label checkForNanOrInfinity;
  Label float1CheckForNanOrInfinity;
  Label float2CheckForNanOrInfinity;
  Label checkFloat1ForZero, checkFloat2ForZero;
  Label float1IsZero;
  Label float1IsNotZero, float2IsNotZero;
  Label float1IsGreaterThanFloat2;
  Label swapFloat1AndFloat2;
  Label readyToAddOrSubtract;
  Label doAdd, doSub;
  Label returnZero, returnFloat2;
  Label normalizeResult, normalizing;
  Label floatRoundingDone, floatRoundUp;

bind_global(fadd);
//  Handled by the JIT
//  VFP_FLOAT_2_TO_1("fadds");

  stmfd(sp, FLOAT_SAVE_SET, writeback);
  mov(EMASK, imm(0xFF));
  comment("Extract exponent1");
  andr(EXP1, EMASK, imm_shift(FLOAT1, lsr, 23));
  comment("Extract exponent2");
  andr(EXP2, EMASK, imm_shift(FLOAT2, lsr, 23));
  mvn(MMASK, imm(0xc0000000));
  comment("Extract mantissa1");
  andr(MANT1, MMASK, imm_shift(FLOAT1, lsl, 7));
  comment("Extract mantissa2");
  andr(MANT2, MMASK, imm_shift(FLOAT2, lsl, 7));
  comment("Check if float1 or float2 is NaN or infinity:");
  cmp(EXP1, imm(0xff));
  cmp(EXP2, imm(0xff), ne);
  b(checkForNanOrInfinity, eq);
  comment("Check for zeroes or denormalized numbers");
  cmp(EXP1, imm(0));
  b(checkFloat1ForZero, eq);
  comment("Else, set implied high bit");
  orr(MANT1, MANT1, imm(0x40000000));

  bind(float1IsNotZero);
  cmp(EXP2, imm(0));
  b(checkFloat2ForZero, eq);
  comment("Else, set implied high bit");
  orr(MANT2, MANT2, imm(0x40000000));

  bind(float2IsNotZero);
  comment("Make sure the magnitude of float1 is larger than the magnitude of ");
  comment("float2.  If not, then swap the two");
  comment("Compare exponents");
  cmp(EXP1, reg(EXP2));
  comment("Compare mantissas");
  cmp(MANT1, reg(MANT2), eq);
  b(swapFloat1AndFloat2, lt);

  bind(float1IsGreaterThanFloat2);
  comment("r12 = order of magnitude difference");
  sub(r12, EXP1, reg(EXP2));
  cmp(r12, imm(7));
  comment("At this point, EXP2 is free to be used as a scratch");
  comment("Adjust float2 to the same order of magnitude as float1 if OK ");
  comment("to do so");
  mov(MANT2, reg_shift(MANT2, lsr, r12), le);
  b(readyToAddOrSubtract, le);
  comment("Save the excess bits shifted out from MANT2. ");
  comment("All we care about is whether they are 0 or not");
  cmp(r12, imm(32));
  rsb(r3, r12, imm(32), lt);
  mov(r3, reg_shift(MANT2, lsl, r3), lt);
  mov(r3, reg(MANT2), ge);
  comment("Adjust float2 to the same order of magnitude as float1");
  mov(MANT2, reg_shift(MANT2, lsr, r12));
  cmp(r3, imm(0));
  comment("Adjust for rounding later");
  orr(MANT2, MANT2, imm(0x10), ne);

  bind(readyToAddOrSubtract);
  comment("Set the result sign in r0");
  comment("Set r0 = result sign");
  andr(r0, FLOAT1, imm(0x80000000));
  comment("We now move the implied binary point to between bit 31 and 30 ");
  comment("(i.e. left by one).  We need to compensate for this by ");
  comment("incrementing the exponent");
  add(EXP1, EXP1, imm(1));
  comment("Check if the signs are the same");
  comment("@ Check if the sign bit is the same");
  eor(r12, FLOAT1, reg(FLOAT2), set_CC);
  comment("High bit will not be set if same");
  b(doAdd, pl);

  bind(doSub);
  comment("Subtract and set condition codes");
  sub(MANT1, MANT1, reg(MANT2), set_CC);
  comment("If zero, then go return zero");
  b(returnZero, eq);

  bind(normalizeResult);
  comment("If we get here, then we know MANT1 is not zero.  It's OK to keep ");
  comment("shifting until we find a bit because there is bound to be 1 within ");
  comment("the 32 bits of MANT1");

  bind(normalizing);
  comment("Else, normalized if necessary");
  cmp(EXP1, imm(1));
  comment("Cannot normalize, go wrap-up");
  b(floatRoundResult, eq);
  comment("Shift left by 1");
  add(MANT1, MANT1, reg(MANT1), set_CC);
  sub(EXP1, EXP1, imm(1));
  comment("If implied high bit is not found");
  b(normalizing, pl);
  comment("continue looking");
  comment("then go do rounding");
  b(floatRoundResult);

  bind(doAdd);
  comment("If we get here, the signs are the same.  We can just add both ");
  comment("numbers as if they are positive numbers, and then reapply the ");
  comment("original when we're done ");
  comment("Add and set condition codes");
  add(MANT1, MANT1, reg(MANT2), set_CC);
  comment("If high bit is not set, go normalize");
  b(normalizeResult, pl);
  comment("Go round result");
  b(floatRoundResult);

  bind(swapFloat1AndFloat2);
  comment("Swap the values in { float1, exponent1, mantissa1 } with the values ");
  comment("in { float2, exponent2, mantissa2 }");
  mov(r12,   reg(EXP1));
  mov(EXP1,  reg(EXP2));
  mov(EXP2,  reg(r12));
  mov(r12,   reg(MANT1));
  mov(MANT1, reg(MANT2));
  mov(MANT2, reg(r12));
  mov(r12,   reg(FLOAT1));
  mov(FLOAT1,reg(FLOAT2));
  mov(FLOAT2,reg(r12));
  b(float1IsGreaterThanFloat2);

  bind(checkFloat1ForZero);
  comment("Check if float1 is zero");
  cmp(MANT1, imm(0));
  comment("float1 is zero");
  b(float1IsZero, eq);
  mov(EXP1, imm(1));
  comment("float1 is not zero");
  b(float1IsNotZero);

  bind(float1IsZero);
  comment("Check if float2 is also zero");
  cmp(EXP2, imm(0));
  comment("float2 not zero.  Return float2");
  b(returnFloat2, ne);
  cmp(MANT2, imm(0));
  comment("float2 not zero.  Return float2");
  b(returnFloat2, ne);
  comment("Fall through to returnZero");

  bind(returnZero);
  comment("Massage the signs");
  andr(r0, FLOAT1, reg(FLOAT2));
  comment("Else return 0 with sign of the larger number (magnitude-wise)");
  andr(r0, r0, imm(0x80000000));
  FLOAT_RETURN_TO_CALLER;

  bind(checkFloat2ForZero);
  comment("check to see if float2 is zero");
  cmp(MANT2, imm(0));
  comment("If 0, return float1 in r0");
  FLOAT_RETURN_TO_CALLER_IF(eq);
  comment("Else, not zero.  Adjust EXP2 because it is implied to be 1");
  mov(EXP2, imm(1));
  b(float2IsNotZero);

  bind(checkForNanOrInfinity);
  comment("Check if float1 is NaN or infinity");
  comment("Check for infinity or nan");
  cmp(EXP1, imm(0xff));
  b(float1CheckForNanOrInfinity, eq);

  comment("Else, float2 is NaN or infinity");
  bind(float2CheckForNanOrInfinity);
  comment("If we get here, then we know that float1 is finite.  The result ");
  comment("will be what ever is in float2 (i.e. infinity or NaN)");

  bind(returnFloat2);
  comment("result = float2");
  mov(r0, reg(FLOAT2));
  comment("Return the result");
  FLOAT_RETURN_TO_CALLER;

  bind(float1CheckForNanOrInfinity);
  comment("Check if float1 is an infinity");
  cmp(MANT1, imm(0));
  comment("If not an infinity, then must be a NaN: ");
  comment("The return value in r0 is a NaN already because r0 contains the ");
  comment("value of float1 which is a Nan\n");
  comment("result = NaN + ? => NaN");
  FLOAT_RETURN_TO_CALLER_IF(ne);

  comment("If we get here, than float1 is an infinity.  We must return check ");
  comment("if float2 is a NaN or an infinity or finite");
  cmp(EXP2, imm(0xff));
  comment("If is finite (i.e. not NaN or infinity), then just return the ");
  comment("infinity value in float1 which is already in r0");
  FLOAT_RETURN_TO_CALLER_IF(ne);
  comment("Check if float2 is an infinity");
  cmp(MANT2, imm(0));
  comment("If not an infinity, then must be a NaN");
  comment("Return the NaN in float2");
  mov(r0, reg(FLOAT2), ne);
  FLOAT_RETURN_TO_CALLER_IF(ne);

  comment("We have 2 infinities");
  comment("NOTE: MMASK<r12> is no longer needed.");

  comment("Check if the sign bit is the same.");
  eor(r12, FLOAT1, reg(FLOAT2), set_CC);

  comment("If the sign bit is the same, both are +Inf or both are -Inf.");
  comment("Just return float1. I.e., Inf+Inf = Inf; -Inf-Inf = -Inf.");
  comment("Otherwise, return NaN");
  mov_imm(r0, floatNaN, mi);
  FLOAT_RETURN_TO_CALLER;

  bind(floatRoundResult);
  comment("NOTE: floatRoundResult expects the sign in r0, the exponent in ");
  comment("EXP1 (i.e. r2), and the normalized mantissa in MANT1 ");
  comment("with the binary point between bit 31 and 30");

  comment("round the mantissa using IEEE 754 round to nearest mode");
  comment("Extract the low 8 bits for rounding");
  andr(r12, MANT1, imm(0xff));
  cmp(r12, imm(0x80));
  comment("Round down.  Nothing to do");
  b(floatRoundingDone, lt);
  comment("Go round up");
  b(floatRoundUp, gt);
  comment("Else round to the nearest 0 in the LSBit in the result mantissa");
  comment("Check LSBit of result mantissa");
  tst(MANT1, imm(0x100));
  comment("If already 0, then done rounding, else, round up");
  b(floatRoundingDone, eq);

  bind(floatRoundUp);
  add(MANT1, MANT1, imm(0x80), set_CC);
  comment("After rounding, we have to re-check if we're normalized, and ");
  comment("re-normalize if we're not");
  b(floatRoundingDone, cc);
  comment("If we get here, then the high bit was in the carry.  Move the ");
  comment("carry back into the high bit and adjust the exponent accordingly");
  mov(MANT1, imm_shift(MANT1, lsr, 1));
  orr(MANT1, MANT1, imm(0x80000000));
  add(EXP1, EXP1, imm(1));
  comment("Fall through to floatRoundingDone");

  bind(floatRoundingDone);
  comment("Now check for overflow to infinities");
  cmp(EXP1, imm(255));
  b(floatReturnInfinity, ge);
  comment("Check to see if the result is a denormalized number");
  tst(MANT1, imm(0x80000000));
  comment("If the number is denormalized, mark it as so");
  comment("Indicate denormalized");
  mov(EXP1, imm(0), eq);
  comment("Only do the following 2 inst if result is normalized");
  comment("Clear the top bit");
  bic(MANT1, MANT1, imm(0x80000000), ne);
  comment("Set the exponent");
  orr(r0, r0, imm_shift(EXP1, lsl, 23), ne);
  comment("Set the mantissa");
  mov(MANT1, imm_shift(MANT1, lsr, 8));
  orr(r0, r0, reg(MANT1));
  comment("Fall through to _floatReturnToCaller");

  bind(floatReturnToCaller);
  comment("Restore the saved registers");
  FLOAT_RETURN_TO_CALLER;

  bind(floatReturnInfinity);
  mov(r1, imm(0xff));
  comment("result = sign | infinity");
  orr(r0, r0, imm_shift(r1, lsl, 23));
  FLOAT_RETURN_TO_CALLER;

  bind(floatReturnNaN);
  comment("result = nan");
  mov_imm(r0, floatNaN);
  bind(done);
  FLOAT_RETURN_TO_CALLER;
}

void FloatSupport::generate_jvm_fsub() {
  Label fsub("jvm_fsub");
  Label fadd("jvm_fadd");

bind_global(fsub);
//  Handled by the JIT
//  VFP_FLOAT_2_TO_1("fsubs");

  comment("invert sign of arg 2");
  eor(r1, r1, imm(0x80000000));
  comment("and call fadd");
  b(fadd);

}

void FloatSupport::generate_jvm_fmul() {
  Label fmul("jvm_fmul");

   // those labels are common
  Label  floatRoundResult("_floatRoundResult");
  Label  done("_floatDone");
  Label  floatReturnToCaller("_floatReturnToCaller");
  Label  floatReturnInfinity("_floatReturnInfinity");
  Label  floatReturnNaN("_floatReturnNaN");
  Label  doGradualUnderflow("_doGradualUnderflow");
  Label  checkForNaNOrInfinity;
  Label  checkForStickyBit;
  Label  checkFloat1ForZero, checkFloat2ForZero;
  Label  float1IsNotZero, float2IsNotZero;
  Label  normalizing;
  Label  float1CheckForNaNOrInfinity, float2CheckForNaNOrInfinity;
  Label  float1CheckForZeroInFloat2;

bind_global(fmul);
//  Handled by the JIT
//  VFP_FLOAT_2_TO_1("fmuls");

  stmfd(sp, FLOAT_SAVE_SET, writeback);
  mov(EMASK, imm(0xFF));
  comment("Extract exponent1");
  andr(EXP1, EMASK, imm_shift(FLOAT1, lsr, 23));
  comment("Extract exponent2");
  andr(EXP2, EMASK, imm_shift(FLOAT2, lsr, 23));
  mvn(MMASK, imm(0x80000000));
  comment("Extract mantissa1");
  andr(MANT1, MMASK, imm_shift(FLOAT1, lsl, 8));
  comment("Extract mantissa2");
  andr(MANT2, MMASK, imm_shift(FLOAT2, lsl, 8));

  comment("NOTE: r12 is free now because MMASK is no longer needed. ");
  comment("Set the sign bit in r0 and free up r1: ");
  comment("Positive if same, else negative.");
  eor(r0, r0, reg(r1));
  comment("Zero out the other bits.");
  andr(r0, r0, imm(0x80000000));
  comment("Check if float1 or float2 is NaN or infinity:");
  cmp(EXP1, imm(0xff));
  cmp(EXP2, imm(0xff), ne);
  b(checkForNaNOrInfinity, eq);
  comment("Check for zeroes or denormalized numbers");
  cmp(EXP1, imm(0));
  b(checkFloat1ForZero, eq);
  comment("Else, set implied high bit");
  orr(MANT1, MANT1, imm(0x80000000));

  bind(float1IsNotZero);
  cmp(EXP2, imm(0));
  b(checkFloat2ForZero, eq);
  comment("Else, set implied high bit");
  orr(MANT2, MANT2, imm(0x80000000));

  bind(float2IsNotZero);
  comment("Now we're ready to do the multiplication.");
  comment("First add the exponents together and store");
  comment("the result in EXP1. EXP2 is free after that.");

  comment("exp1 -= 127 (i.e. the bias) + 1");
  sub(EXP1, EXP1, imm(126));

  comment("exp1 = exp1 + exp2");
  add(EXP1, EXP1, reg(EXP2));

  comment("After the multiplication, the resultant binary point will be 2");
  comment("digits from the high end of the 64 bit number as follows:");
  comment("result = [xx.xx xxxx xxxx xxxx xxxx xxxx 0000 0000]");
  comment("But we want the binary point to be 1 from the high end. ");
  comment("To do this we divide the number by 2 and add 1 to the exponent.");
  comment("Well, we don't actually have to divide the number by 2.  We just");
  comment("know that the binary point is now 1 from the left.");
  comment("EXP1 is already incremented above.  See how the bias is being ");
  comment("subtracted from it.");

  comment("{hi<r1>,lo<r3>} = MANT1 * MANT2");
  umull(r3, r1, MANT1, MANT2);
  comment("NOTE: r6, r7, and r12 are now free again: ");
  comment("Normalize the resultant mantissa if necessary: ");
  comment("Put the mantissa in { r6, r3 }");
  mov(MANT1, reg(r1));

  comment("Normalize the result, if < 1");
  cmp(EXP1,  imm(1));
  b(doGradualUnderflow, lt);

  cmp(MANT1, imm(0));
  comment("If high bit is set and exponent is OK "
    "(i.e. already normalized), then move on.");
  b(checkForStickyBit, lt);

  bind(normalizing);
  comment("See if exponent is down to 1");
  cmp(EXP1, imm(1));
  comment("Cannot normalize.  Go wrap-up.");
  b(checkForStickyBit, eq);
  comment("Shift the 64bit number left by 1:");
  comment("Shift low-order word left by 1.");
  mov(r3, imm_shift(r3, lsl, 1), set_CC);
  comment("exponent--");
  sub(EXP1, EXP1, imm(1));
  comment("Shift high-order word left by 1.");
  adc(MANT1, MANT1, reg(MANT1), set_CC);
  comment("If high bit not set, continue.");
  b(normalizing, pl);
  comment("Fall through to checkForStickyBit.");

  bind(checkForStickyBit);
  comment("Is already normalized.  Check the sticky bit for rounding:");
  cmp(r3, imm(0));
  comment("Set the sticky bit if necessary.");
  orr(MANT1, MANT1, imm(0x20), ne);
  comment("Go round the result and exit.");
  b(floatRoundResult);

  bind(doGradualUnderflow);
  comment("Compute the number of bits we have to shift right by in order to");
  comment("bring EXP1 up to 1:");
  rsb(r7, EXP1, imm(1));
  cmp(r7, imm(31));
  comment("If we have to shift 31 or more bits, then the result must be an");
  comment("underflow to zero:");
  comment("result = sign | 0");
  FLOAT_RETURN_TO_CALLER_IF(ge);

  /* Else, shift right and let the rounding do gradual underflow: */
  mov(EXP1, imm(1));
  comment("Compute the sticky bit.");
  orr(r3, r3, reg_shift(MANT1, lsl, r7));
  comment("Shift right be said bits.");
  mov(MANT1, reg_shift(MANT1, lsr, r7));
  b(checkForStickyBit);

  bind(checkFloat1ForZero);
  comment("If we get here, then float1 and float2 are finite:");
  comment("Check for 0");
  cmp(MANT1, imm(0));
  comment("result = sign | 0. Return to caller.");
  FLOAT_RETURN_TO_CALLER_IF(eq);
  comment("Adjust exponent for denormalized #");
  mov(EXP1, imm(1));
  b(float1IsNotZero);

  bind(checkFloat2ForZero);
  comment("If we get here, then float1 and float2 are finite:");
  comment("Check for 0");
  cmp(MANT2, imm(0));
  comment("result = sign | 0. Return to caller.");
  FLOAT_RETURN_TO_CALLER_IF(eq);
  comment("Adjust exponent for denormalized #");
  mov(EXP2, imm(1));
  b(float2IsNotZero);

  bind(checkForNaNOrInfinity);
  comment("Check if float1 is NaN or infinity:");
  cmp(EXP1, imm(0xff));
  b(float1CheckForNaNOrInfinity, eq);
  comment("Else, float2 is NaN or infinity:");

  bind(float2CheckForNaNOrInfinity);
  comment("If we get here, then float1 is not nan nor infinity:");
  comment("Check for infinity");
  cmp(MANT2, imm(0));
  comment("finite * nan => nan");
  b(floatReturnNaN, ne);
  comment("Else, float2 is an infinity.  Check float1 for 0:");
  comment("Check for 0.");
  orr(r12, EXP1, reg(MANT1), set_CC);
  comment("0 * inf => nan.");
  b(floatReturnNaN, eq);
  comment("finite * inf => inf");
  b(floatReturnInfinity);

  bind(float1CheckForNaNOrInfinity);
  comment("Check for infinity");
  cmp(MANT1, imm(0));
  comment("nan * ? => nan.");
  b(floatReturnNaN, ne);
  comment("Else, float1 is an infinity.  Check float2 for nan or infinity:");
  cmp(EXP2, imm(0xff));
  b(float1CheckForZeroInFloat2, ne);
  comment("Check for infinity.");
  cmp(MANT2, imm(0));
  comment("inf * nan => nan.");
  b(floatReturnNaN, ne);
  comment("inf * inf => inf.");
  b(floatReturnInfinity);

  bind(float1CheckForZeroInFloat2);
  comment("Check for 0.");
  orr(r12, EXP2, reg(MANT2), set_CC);
  comment("inf * 0 => nan.");
  b(floatReturnNaN, eq);
  comment("inf * finite => inf.");
  b(floatReturnInfinity);
}

void FloatSupport::generate_jvm_fdiv() {
  Label fdiv("jvm_fdiv");

  // those labels are common
  Label  floatRoundResult("_floatRoundResult");
  Label  done("_floatDone");
  Label  floatReturnToCaller("_floatReturnToCaller");
  Label  floatReturnInfinity("_floatReturnInfinity");
  Label  floatReturnNaN("_floatReturnNaN");
  Label  doGradualUnderflow("_doGradualUnderflow");
  Label  checkForNaNOrInfinity;
  Label  float1IsNotZero, float2IsNotZero;
  Label  checkFloat1ForZero, checkFloat2ForZero;
  Label  iterationLoopTop;
  Label  norm1Loop, norm2Loop;
  Label  float1CheckForNaNOrInfinity, float2CheckForNaNOrInfinity;
  Label  float1IsZero;

bind_global(fdiv);
//  Handled by the JIT
//  VFP_FLOAT_2_TO_1("fdivs");

  stmfd(sp, FLOAT_SAVE_SET, writeback);
  mov(EMASK, imm(0xFF));
  comment("Extract exponent1");
  andr(EXP1, EMASK, imm_shift(FLOAT1, lsr, 23));
  comment("Extract exponent2");
  andr(EXP2, EMASK, imm_shift(FLOAT2, lsr, 23));
  orr(EMASK, EMASK, imm(0x100));
  comment("Extract mantissa1");
  bic(MANT1, FLOAT1, imm_shift(EMASK, lsl, 23));
  comment("Extract mantissa2");
  bic(MANT2, FLOAT2, imm_shift(EMASK, lsl, 23));
  comment("Set the sign bit in r0 and free up r1:");
  comment("Positive if same, else negative.");
  eor(r0, r0, reg(r1));
  comment("Zero out the other bits.");
  andr(r0, r0, imm(0x80000000));
  comment("Check if float1 or float2 is NaN or infinity:");
  cmp(EXP1, imm(0xff));
  cmp(EXP2, imm(0xff), ne);
  b(checkForNaNOrInfinity, eq);
  comment("Check float1 for 0 or denormalized number:");
  cmp(EXP1, imm(0));
  b(checkFloat1ForZero, eq);
  comment("Set the implied high bit.");
  orr(MANT1, MANT1, imm(0x800000));

  bind(float1IsNotZero);
  cmp(EXP2, imm(0));
  b(checkFloat2ForZero, eq);
  comment("Set the implied high bit.");
  orr(MANT2, MANT2, imm(0x800000));

  bind(float2IsNotZero);
  comment("Now we're ready to do the division. First subtract the exponents");
  comment("and store the result in EXP1.  EXP2 is free after that.");
  comment("Ensure that the first digit of the quotient will be a '1'");
  comment("by shifting MANT1 if necessary. Decrease exponent accordingly.");
  comment("Do the division as 25 successive divide steps");
  comment("of MANT1 by MANT2, developing the quotient in QUOT.");

  comment("exp1 = exp1 - exp2.");
  sub(EXP1, EXP1, reg(EXP2));
  comment("exp1 += 127 (i.e. the bias)");
  add(EXP1, EXP1, imm(127));
  cmp(MANT1, reg(MANT2));
  mov(MANT1, imm_shift(MANT1, lsl, 1), lt);
  sub(EXP1,  EXP1, imm(1), lt);
  mov(ITER, imm(25));
  mov(QUOT, imm(0));
  comment("loop unrolled by a factor of 5");

  bind(iterationLoopTop);
  cmp(MANT1, reg(MANT2));
  sub(MANT1, MANT1, reg(MANT2), hs);
  adc(QUOT, QUOT, reg(QUOT));
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  cmp(MANT1, reg(MANT2));
  sub(MANT1, MANT1, reg(MANT2), hs);
  adc(QUOT, QUOT, reg(QUOT));
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  cmp(MANT1, reg(MANT2));
  sub(MANT1, MANT1, reg(MANT2), hs);
  adc(QUOT, QUOT, reg(QUOT));
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  cmp(MANT1, reg(MANT2));
  sub(MANT1, MANT1, reg(MANT2), hs);
  adc(QUOT, QUOT, reg(QUOT));
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  cmp(MANT1, reg(MANT2));
  sub(MANT1, MANT1, reg(MANT2), hs);
  adc(QUOT, QUOT, reg(QUOT));
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  sub(ITER, ITER, imm(5), set_CC);
  b(iterationLoopTop, gt);

  comment("In order to share rounding and packing code with fmul, ");
  comment("move QUOT to MANT1, shifting left s.t. the high-order bit is set.");
  comment("Also insert the sticky bit.");
  cmp(MANT1, imm(0));
  mov(MANT1, imm_shift(QUOT, lsl, 7));
  orr(MANT1, MANT1, imm(20), ne);
  cmp(EXP1, imm(1));
  b(floatRoundResult, ge);
  b(doGradualUnderflow);

  bind(checkForNaNOrInfinity);
  comment("Check if float1 is NaN or infinity:");
  cmp(EXP1, imm(0xff));
  b(float1CheckForNaNOrInfinity, eq);

  comment("Else, float2 is NaN or infinity:");
  bind(float2CheckForNaNOrInfinity);
  comment("If we get here, then float1 is not nan nor infinity:");
  comment("Check for infinity.");
  cmp(MANT2, imm(0));
  b(floatReturnNaN, ne);
  comment("Else, float2 is an infinity.  Result is 0");
  FLOAT_RETURN_TO_CALLER;

  bind(float1CheckForNaNOrInfinity);
  comment("Check for infinity.");
  cmp(MANT1, imm(0));
  comment("nan / ? => nan.");
  b(floatReturnNaN, ne);

  comment("Else, float1 is an infinity.  Check float2 for nan or infinity: ");
  cmp(EXP2, imm(0xff));
  comment("inf / finite => inf");
  b(floatReturnInfinity, ne);
  comment("inf / {nan or inf} => nan.");
  b(floatReturnNaN);

  bind(checkFloat1ForZero);
  comment("If we get here, then float1 and float2 are finite:");
  cmp(MANT1, imm(0));
  b(float1IsZero, eq);
  mov(EXP1, imm(1));
  bind(norm1Loop);
  mov(MANT1, imm_shift(MANT1, lsl, 1));
  sub(EXP1, EXP1, imm(1));
  cmp(MANT1, imm(0x800000));
  b(norm1Loop, lt);
  b(float1IsNotZero);

  bind(float1IsZero);
  cmp(EXP2,  imm(0));
  cmp(MANT2, imm(0), eq);
  comment("result = sign | 0. Return to caller.");
  FLOAT_RETURN_TO_CALLER_IF(ne);
  comment("0 / 0 => NaN");
  b(floatReturnNaN);

  bind(checkFloat2ForZero);
  comment("If we get here, then float1 is finite and non-zero:");
  cmp(MANT2, imm(0));
  b(floatReturnInfinity, eq);
  mov(EXP2, imm(1));
  bind(norm2Loop);
  mov(MANT2, imm_shift(MANT2, lsl, 1));
  sub(EXP2, EXP2, imm(1));
  cmp(MANT2, imm(0x800000));
  b(norm2Loop, lt);
  b(float2IsNotZero);
}

void FloatSupport::generate_jvm_frem() {
  STUB(jvm_frem);
}

#undef FLOAT1
#undef FLOAT2
#undef EXP1
#undef EXP2
#undef MANT1
#undef MANT2
#undef EMASK
#undef MMASK
#undef ITER
#undef QUOT
#undef FLOAT_SAVE_SET
#undef FLOAT_RESTORE_SET
#undef FLOAT_RETURN_TO_CALLER_IF
#undef FLOAT_RETURN_TO_CALLER

void FloatSupport::generate_jvm_f2i() {

  Label f2i("jvm_f2i");

  Register ARG = r0;
  Register F = r1;
  Register EXP = r2;
  Label result_is_zero;
  Label large_exponent;

  // This is written as a separate function so that the
  // compiler can use it, too.
bind_global(f2i);

//  Handled by the JIT
//  VFP_FLOAT_1_TO_1("ftosizs");

  comment("Strip sign");
  bic(F, ARG, imm(0x80000000));
  comment("De-bias exponent");
  sub(EXP, F, imm(0x3f800000), set_CC);
  b(result_is_zero, lt);
  cmp(EXP, imm(31 << 23));
  b(large_exponent, hs);
  comment("Shift fraction so that high bit is bit 30");
  mov(F, imm_shift(F, lsl, 8));
  comment("Set implicit high order bit");
  orr(F, F, imm(0x80000000));
  comment("Get the exponent");
  mov(EXP, imm_shift(EXP, lsr, 23));
  comment("Shift F right by 31 - exponent");
  rsb(EXP, EXP, imm(31));
  mov(F, reg_shift(F, lsr, EXP));
  comment("Apply the sign and return");
  cmp(r0, zero);
  mov(r0, reg(F),  ge);
  rsb(r0, F, zero, lt);
  jmpx(lr);

  bind(large_exponent);
  comment("If nan, result is 0.  Otherwise big integer");
  cmp(EXP, imm(0x7f800000-0x3f800000));
  b(result_is_zero, hi);
  comment("Return appropriately signed largest integer");
  cmp(r0, zero);
  mvn(r0, imm(0x80000000), ge);
  mov(r0, imm(0x80000000), lt);
  jmpx(lr);

  bind(result_is_zero);
  mov(r0, zero);
  jmpx(lr);
}

void FloatSupport::generate_jvm_d2i() {

  Label d2i("jvm_d2i");

bind_global(d2i);

#if 0  // Doesn't work with VFP on H4
  VFP_DOUBLE_1_TO_1("ftosizd");
#endif

  Register ARG_HIGH = MSW_FIRST_FOR_DOUBLE ? r0 : r1; // exponent
  Register ARG_LOW  = MSW_FIRST_FOR_DOUBLE ? r1 : r0; // pure mantissa
  Register F = r2;
  Register EXP = r3;
  Register tmp = r12;
  Label result_is_zero;
  Label large_exponent;

  // This is written as a separate function so that the
  // compiler can use it, too.

  comment("Strip sign");
  bic(F, ARG_HIGH, imm(0x80000000));
  comment("De-bias exponent");
  sub(EXP, F,   imm(0x3f000000));
  sub(EXP, EXP, imm(0x00F00000), set_CC);
  b(result_is_zero, lt);
  cmp(EXP, imm(31 << 20));
  b(large_exponent, hs);

  comment("Shift fraction so that high bit is bit 30");
  mov(F, imm_shift(F, lsl, 11));
  orr(F, F, imm_shift(ARG_LOW, lsr, 21));
  comment("Set implicit high order bit");
  orr(F, F, imm(0x80000000));

  comment("Get the exponent");
  mov(EXP, imm_shift(EXP, lsr, 20));
  comment("Shift F right by 31 - exponent");
  rsb(EXP, EXP, imm(31));
  mov(F, reg_shift(F, lsr, EXP));
  comment("Apply the sign and return");
  cmp(ARG_HIGH, zero);
  mov(r0, reg(F),  ge);
  rsb(r0, F, zero, lt);
  jmpx(lr);

  bind(large_exponent);
  comment("If nan, result is 0.  Otherwise big integer");
  cmp(EXP, imm(0x7fF00000-0x3fF00000));
  cmp(ARG_LOW, zero, eq);
  b(result_is_zero, hi);
  comment("Return appropriately signed largest integer");
  cmp(ARG_HIGH, zero);
  mvn(r0, imm(0x80000000), ge);
  mov(r0, imm(0x80000000), lt);
  jmpx(lr);

  bind(result_is_zero);
  mov(r0, zero);
  jmpx(lr);
}

void FloatSupport::generate_jvm_i2d() {

  Label i2d("jvm_i2d");

  // These are written as separate functions so that
  // the compiler can use them, too.
bind_global(i2d);

//  VFP_INT_1_TO_1("fsitod")

  Register F = r2;
  Register EXP = r3;
  Register RESULT_HIGH = MSW_FIRST_FOR_DOUBLE ? r0 : r1; // exponent
  Register RESULT_LOW  = MSW_FIRST_FOR_DOUBLE ? r1 : r0; // mantissa

  comment("Get absolute value in %s.  Short circuit on zero", reg_name(F));
  mov(F, reg(r0), set_CC);      // doesn't set C or Z bit
  mov(r1, zero, eq);
  jmpx(lr, eq);
  rsb(F, r0,  zero, mi);

  //
  // 31 is the maximum number of shifts required to get
  // a '1' in the high-order bit of F.
  // 0x3ff is the exponent of 1.0 (i.e. the bias).
  // -1 is to account for the implied high-order bit. Rather
  //  than masking it out, we just subtract it out here.

  comment("Exponent of 0x80000000, minus one");
  mov_imm(EXP, 31+0x3ff-1);

  for (int i = 4; i >= 0; i--) {
    int shift = 1 << i;
    comment("Shift by %d if necessary", shift);
    cmp(F, imm(1 << (32 - shift)));
    mov(F, imm_shift(F, lsl, shift), lo);
    sub(EXP, EXP, imm(shift), lo);
  }
  comment("Integer shifted so top bit is in bit 31");
  comment("Combine the sign, exponent, and mantissa");
  andr(RESULT_HIGH, r0, imm(0x80000000));
  add(RESULT_HIGH, RESULT_HIGH, imm_shift(EXP, lsl, 20));
  add(RESULT_HIGH, RESULT_HIGH, imm_shift(F, lsr, 11));
  // And the low word gets remaining 11 low order bits
  mov(RESULT_LOW, imm_shift(F, lsl, 32 - 11));
  jmpx(lr);
}

void FloatSupport::generate_jvm_i2f() {

  Label i2f("jvm_i2f");

  // These are written as separate functions so that
  // the compiler can use them, too.
bind_global(i2f);
//  Handled by the JIT
//  VFP_FLOAT_1_TO_1("fsitos");

  Register F = r1;
  Register EXP = r2;

  comment("Get absolute value in %s.  Short circuit on zero", reg_name(F));
  mov(F, reg(r0), set_CC);      // doesn't set C or Z bit
  jmpx(lr, eq);
  rsb(F, r0,  zero, mi);

  //
  // 31 is the maximum number of shifts required to get
  // a '1' in the high-order bit of F.
  // 0x7f is the exponent of 1.0 (i.e. the bias).
  // -1 is to account for the implied high-order bit. Rather
  //  than masking it out, we just subtract it out here.

  comment("Exponent of 0x80000000, minus one");
  mov(EXP, imm(31+0x7f-1));

  for (int i = 4; i >= 0; i--) {
    int shift = 1 << i;
    comment("Shift by %d if necessary", shift);
    cmp(F, imm(1 << (32 - shift)));
    mov(F, imm_shift(F, lsl, shift), lo);
    sub(EXP, EXP, imm(shift), lo);
  }
  comment("Integer shifted so top bit is in bit 31");
  comment("Combine the sign, exponent, and mantissa");
  andr(r0, r0, imm(0x80000000));
  add(r0, r0, imm_shift(EXP, lsl, 23));
  add(r0, r0, imm_shift(F, lsr, 8));
  comment("Perform rounding, if necessary");
  andr(F, F, imm(0xFF), set_CC);
  jmpx(lr, eq);
  cmp(F, imm(0x80));
  add(r0, r0, one, hs);
  bic(r0, r0, one, eq);
  jmpx(lr);
}

void FloatSupport::generate_jvm_dcmp() {

  Label dcmpl("jvm_dcmpl");
  Label dcmpg("jvm_dcmpg");

// Disable don't pass TCK in RunFastMode
#if 0 // ENABLE_ARM_VFP

  bind_global(dcmpg);
  VFP_EMIT_RAW("  fmdrr d6, r0, r1");
  VFP_EMIT_RAW("  fmdrr d7, r2, r3");
  VFP_EMIT_RAW("  fcmped  d6, d7");
  VFP_EMIT_RAW("  fmstat");
  VFP_EMIT_RAW("  mov r0, #1");
  VFP_EMIT_RAW("  bxgt  lr");
  VFP_EMIT_RAW("  fcmpd d6, d7");
  VFP_EMIT_RAW("  fmstat");
  VFP_EMIT_RAW("  mov r0, #0");
  VFP_EMIT_RAW("  bxeq  lr");
  VFP_EMIT_RAW("  fcmped  d6, d7");
  VFP_EMIT_RAW("  fmstat");
  VFP_EMIT_RAW("  sub r0, r0, #1");
  VFP_EMIT_RAW("  bxmi  lr");
  VFP_EMIT_RAW("  mov r0, #1");
  VFP_EMIT_RAW("  bx  lr");

  bind_global(dcmpl);
  VFP_EMIT_RAW("  fmdrr d6, r0, r1");
  VFP_EMIT_RAW("  fmdrr d7, r2, r3");
  VFP_EMIT_RAW("  fcmped  d6, d7");
  VFP_EMIT_RAW("  fmstat");
  VFP_EMIT_RAW("  mov r0, #1");
  VFP_EMIT_RAW("  bxgt  lr");
  VFP_EMIT_RAW("  fcmpd d6, d7");
  VFP_EMIT_RAW("  fmstat");
  VFP_EMIT_RAW("  mvnne r0, #0");
  VFP_EMIT_RAW("  moveq r0, #0");
  VFP_EMIT_RAW("  bx  lr");

#else

  Register A_HIGH = MSW_FIRST_FOR_DOUBLE ? r0 : r1;
  Register A_LOW  = MSW_FIRST_FOR_DOUBLE ? r1 : r0;
  Register B_HIGH = MSW_FIRST_FOR_DOUBLE ? r2 : r3;
  Register B_LOW  = MSW_FIRST_FOR_DOUBLE ? r3 : r2;
  Register temp =  r12;
  Label oppositeSigns;
  Label bothPositive;
  Label bothNegative;
  Label dcmp_join;
  Label return_one, return_minus_one;

  bind_global(dcmpg);
  comment("Is first argument NaN?");
  mov(temp, imm_shift(A_HIGH, lsl, 1));
  cmn(temp, imm(0x200000));
  cmp(A_LOW, one, eq);
  b(return_one, cs);

  comment("Is second argument NaN?");
  mov(temp, imm_shift(B_HIGH, lsl, 1));
  cmn(temp, imm(0x200000));
  cmp(B_LOW, one, eq);
  b(dcmp_join,  cc);

  bind(return_one);
  mov_imm(r0, 1);
  jmpx(lr);
  bind(return_minus_one);
  mov_imm(r0, -1);
  jmpx(lr);

  bind(dcmpl);
  comment("Is first argument NaN?");
  mov(temp, imm_shift(A_HIGH, lsl, 1));
  cmn(temp, imm(0x200000));
  cmp(A_LOW, one, eq);
  b(return_minus_one, cs);

  comment("Is second argument NaN?");
  mov(temp, imm_shift(B_HIGH, lsl, 1));
  cmn(temp, imm(0x200000));
  cmp(B_LOW, one, eq);
  b(return_minus_one, cs);

  bind(dcmp_join);
  comment("Set N bit if arguments have opposite signs");
  comment("Set C bit to sign of first argument");
  teq(B_HIGH, imm_shift(A_HIGH, asr, 0)); // Note that 0 is really 32
  b(oppositeSigns, mi);

  comment("Arguments have the same sign.  C bit indicates their sign");
  b(bothNegative, cs);
  bind(bothPositive);
  // Since both A and B are non-negative at this point, comparing
  // them is simplified.
  cmp(A_HIGH, reg(B_HIGH));
  sub(r0, A_LOW, reg(B_LOW), set_CC, eq);
  mov_imm(r0, 1,  hi);
  mov_imm(r0, -1, lo);
  jmpx(lr);

  bind(bothNegative);
  cmp(A_HIGH, reg(B_HIGH));
  sub(r0, A_LOW, reg(B_LOW), set_CC, eq);
  mov_imm(r0, 1,  lo);
  mov_imm(r0, -1, hi);
  jmpx(lr);

  bind(oppositeSigns);
  comment("Arguments have opposite signs");
  comment("First, special check for comparing +0 against -0");
  orr(temp, A_LOW, reg(B_LOW), set_CC);
  orr(temp, A_HIGH, reg(B_HIGH), eq);
  cmp(temp, imm(0x80000000),            eq);
  mov(B_HIGH, reg(A_HIGH),       eq);

  // The return value is sign(B_HIGH) - sign(A_HIGH)
  mov(B_HIGH, imm_shift(B_HIGH, lsr, 31));
  sub(r0, B_HIGH, imm_shift(A_HIGH, lsr, 31));
  jmpx(lr);

#endif // ENABLE_ARM_VFP
}

void FloatSupport::generate_jvm_fcmp() {

  Label fcmpl("jvm_fcmpl");
  Label fcmpg("jvm_fcmpg");

  Label oppositeSigns;
  Label fcmp_join;

  Register temp = r2;

  bind_global(fcmpg);
  mov(temp, imm_shift(r0, lsl, 1));
  cmp(temp, imm(0xFF000000));
  mov(temp, imm_shift(r1, lsl, 1), ls);
  cmp(temp, imm(0xFF000000), ls);
  b(fcmp_join, ls);
  mov_imm(r0, 1);
  jmpx(lr);

  bind(fcmpl);
  mov(temp, imm_shift(r0, lsl, 1));
  cmp(temp, imm(0xFF000000));
  mov(temp, imm_shift(r1, lsl, 1), ls);
  cmp(temp, imm(0xFF000000), ls);
  b(fcmp_join, ls);
  mov_imm(r0, -1);
  jmpx(lr);

  bind(fcmp_join);
  comment("Set N bit if arguments have opposite signs");
  comment("Set C bit to sign of first argument");
  teq(r1, imm_shift(r0, asr, 0));   // Note that 0 is really 32
  b(oppositeSigns, mi);

  comment("Arguments have the same sign.  C bit indicates their sign");
  mvn(r0, reg(r0), cs);
  mvn(r1, reg(r1), cs);
  // r0 and r1 are both non-negative at this point.
  sub(r0, r0, reg(r1), set_CC);
  mov_imm(r0, 1,  gt);
  mov_imm(r0, -1, lt);
  jmpx(lr);

  bind(oppositeSigns);
  comment("Arguments have opposite signs");
  orr(temp, r0, reg(r1));
  cmp(temp, imm(0x80000000));
  mov(r1, reg(r0), eq);

  // The return value is sign(r1) - sign(r0)
  mov(r1, imm_shift(r1, lsr, 31));
  sub(r0, r1, imm_shift(r0, lsr, 31));
  jmpx(lr);
}

#if MSW_FIRST_FOR_DOUBLE
// high word, arg1
#define A1                      r0
#define A2                      r1
#define B1                      r2
#define B2                      r3
#else
#define A1                      r1
#define A2                      r0
#define B1                      r3
#define B2                      r2
#endif
#define DOUBLE_SAVE_SET         join(range(r4, r9), set(lr))
#define EXPA                    r4
#define EXPB                    r5
#define RESULTX                 r6
#define EXPMASK                 r7
#define FLAGS                   lr
#define EXPSHIFT                20
#define DOUBLE_EXPVAL           0x7ff

#define DOUBLE_RETURN_TO_CALLER             \
  ldmfd(sp, DOUBLE_SAVE_SET, writeback);    \
  jmpx(lr);                                 \

#define DOUBLE_UNPACK( HiSrc, ExpReg, DenormalDest, ExceptionalDest )  \
        andr(ExpReg, EXPMASK, imm_shift(HiSrc, lsr, EXPSHIFT), set_CC);\
        bic(HiSrc, HiSrc, imm_shift(EXPMASK, lsl, EXPSHIFT));          \
        b(DenormalDest, eq);                                           \
        cmp(ExpReg, reg(EXPMASK));                                     \
        b(ExceptionalDest, eq);                                        \
        orr(HiSrc, HiSrc, imm(1<<EXPSHIFT));

#define DOUBLE_EXCEPTIONAL( HiSrc, LoSrc, Infflag, Lreturn ) \
        cmp(HiSrc, imm(0));                                  \
        cmp(LoSrc, imm(0), eq);                              \
        b(_double_deliver_NaN, ne);                          \
        orr(FLAGS, FLAGS, Infflag);                     \
        b(Lreturn);

#define DOUBLE_NORMALIZE( HiSrc, LoSrc, ExpReg, Lreturn, Zflag )  \
        cmp(HiSrc, imm(0));                                       \
        cmp(LoSrc, imm(0), eq);                                   \
        orr(FLAGS, FLAGS, Zflag, eq);                             \
        b(Lreturn, eq);                                           \
        { /* to separate labels visibility */                     \
         Label label1;                                            \
         bind(label1);                                            \
         add(LoSrc, LoSrc, reg(LoSrc), set_CC);                   \
         adc(HiSrc, HiSrc, reg(HiSrc));                           \
         cmp(HiSrc, imm(1<<EXPSHIFT));                            \
         sub(ExpReg, ExpReg, imm(1), lt);                         \
         b(label1, lt);                                           \
    }                                                             \
         b(Lreturn);

void FloatSupport::generate_jvm_dadd() {

  Label dadd("jvm_dadd");

#define LSHIFT   r8
#define TEMP     r9
#define ANEGFLAG 0x80000000
#define BNEGFLAG 0x40000000
#define AINFFLAG 2
#define BINFFLAG 4
#define AZFLAG   8
#define BZFLAG   0x10
/*
 * Check for and flag zeros. Else give the number an exponent of '1'
 * so that, even in the absence of an implicit bit, these numbers will
 * scale right when combined with normalized values.
 */
#define DADD_DENORMAL( HiSrc, LoSrc, ExpSrc, Zeroflag, Lreturn ) \
        cmp(HiSrc, imm(0));                                      \
        cmp(LoSrc, imm(0), eq);                                  \
        orr(FLAGS, FLAGS, Zeroflag, eq);                         \
        b(Lreturn, eq);                                          \
        mov(ExpSrc, imm(1));                                     \
        b(Lreturn);

  Label
    _dadd_noxchng, _dadd_nocarry,
    _dadd_unpackB, _dadd_unpacked,
    _dadd_deliver_a,
    _dadd_done_scaling, _dadd_continue_scaling,
    _dadd_denorm_result, _dadd_subtract,
    _dadd_hard_rounding_halfwordshift, _dadd_small_result,
    _dadd_hard_bitloop, _dadd_hard_rounding_bitloop,
    _dadd_subnormalA, _dadd_subnormalB,
    _dadd_exceptionalA, _dadd_exceptionalB,
    _dadd_unusual_operands, _dadd_infinite_operands;

  Label
    _double_check_guard("_double_check_guard"),
    _double_deliver_positive_zero("_double_deliver_positive_zero"),
    _double_deliver_NaN("_double_deliver_NaN"),
    _double_deliver_Infinity("_double_deliver_Infinity"),
    _double_denorm_check_guard("_double_denorm_check_guard"),
    _double_done_rounding("_double_done_rounding"),
    _double_denorm_done_rounding("_double_denorm_done_rounding");

bind_global(dadd);
//  VFP_DOUBLE_2_TO_1("faddd");

bind_global("jvm_dadd_software");
  stmfd(sp, DOUBLE_SAVE_SET, writeback);
  comment("capture signs of operands");
  comment("because ANEGFLAG is SIGN");
  andr(FLAGS, A1, imm(ANEGFLAG));
  cmp(B1, imm(0));
  orr(FLAGS, FLAGS, imm(BNEGFLAG), mi);
  comment("no thinking here");
  bic(A1, A1, imm(0x80000000));
  comment("no thinking here");
  bic(B1, B1, imm(0x80000000));
  comment("arrange for the larger summand to be in A");
  cmp(A1, reg(B1));
  cmp(A2, reg(B2), eq);
  b(_dadd_noxchng, hs);
  mov(RESULTX, reg(A1));
  mov(A1, reg(B1));
  mov(B1, reg(RESULTX));
  mov(RESULTX, reg(A2));
  mov(A2, reg(B2));
  mov(B2, reg(RESULTX));
  comment("exchange sign flags, too");
  mov(FLAGS, imm_shift(FLAGS, lsl, 1), set_CC);
  comment("(may not matter)");
  orr(FLAGS, FLAGS, imm(BNEGFLAG), cs);

  bind(_dadd_noxchng);
  comment("exponent mask");
  mov_imm(EXPMASK, DOUBLE_EXPVAL);
  DOUBLE_UNPACK( A1, EXPA, _dadd_subnormalA, _dadd_exceptionalA);

  bind(_dadd_unpackB);
  DOUBLE_UNPACK( B1, EXPB, _dadd_subnormalB, _dadd_exceptionalB );

  bind(_dadd_unpacked);
  tst(FLAGS, imm(AINFFLAG|BINFFLAG|AZFLAG|BZFLAG));
  b(_dadd_unusual_operands, ne);

  comment("Scale B by (EXPA-EXPB) in preparation of addition.");
  comment("Recall that EXPA >= EXPB.");
  comment("RESULTX will hold shifted-out bits. Its high-order");
  comment("bit is the guard bit, and any bits shifted out have");
  comment("to be treated as sticky.");
  sub(EXPB, EXPA, reg(EXPB), set_CC);
  mov(RESULTX, imm(0));
  comment("It can happen.");
  b(_dadd_done_scaling, eq);
  cmp(EXPB, imm(54));
  comment("B would scale to nothingness");
  b(_dadd_deliver_a, gt);
  cmp(EXPB, imm(32));
  b(_dadd_continue_scaling, lt);
  comment("scale a whole 32 bits worth");
  mov(RESULTX, reg(B2));
  mov(B2, reg(B1));
  mov(B1, imm(0));
  sub(EXPB, EXPB, imm(32));

  bind(_dadd_continue_scaling);
  rsb(LSHIFT, EXPB, imm(32));
  comment("capture any sticky bits we are about to shift out");
  mov(TEMP, imm_shift(RESULTX, lsl, LSHIFT));
  comment("shift bits down");
  mov(RESULTX, reg_shift(RESULTX, lsr, EXPB));
  orr(RESULTX, RESULTX, reg_shift(B2, lsl, LSHIFT));
  mov(B2, reg_shift(B2, lsr, EXPB));
  orr(B2, B2, reg_shift(B1, lsl, LSHIFT));
  mov(B1, reg_shift(B1, lsr, EXPB));
  comment("reinsert sticky bits");
  cmp(TEMP, imm(0));
  orr(RESULTX, RESULTX, imm(1), ne);

  bind(_dadd_done_scaling);
  comment("If signs the same, we do addition.");
  comment("If signs differ, we do subtraction.");
  teq(FLAGS, imm_shift(FLAGS, lsl, 1));
  b(_dadd_subtract, mi);
  add(A2, A2, reg(B2), set_CC);
  adc(A1, A1, reg(B1));
  comment("shift, if necessary, so HOB is the implicit bit.");
  cmp(A1, imm(1<<(EXPSHIFT+1)));
  b(_dadd_nocarry, lt);
  comment("there was a carry out and we downshift accordingly");
  mov(A1, imm_shift(A1, lsr, 1), set_CC);
  mov(A2, imm_shift(A2, ror, 0), set_CC);
  mov(RESULTX, imm_shift(RESULTX, ror, 0), set_CC);
  comment("this is why we call it sticky");
  orr(RESULTX, RESULTX, imm(1), cs);
  add(EXPA, EXPA, imm(1));
  b(_double_check_guard);

  bind(_dadd_nocarry);
  comment("the only way to have a denorm here is from the sum of");
  comment("two denorms.");
  cmp(EXPA, imm(1));
  b(_double_check_guard, gt);

  comment("The exponent is == 1. This is the result of the sum");
  comment("or difference of two denormalized numbers.");
  comment("If the fraction is all zero, return +0");
  comment("Else we jam in the sign and return. No rounding necessary here.");

  bind(_dadd_denorm_result);
  orr(TEMP, A1, reg(A2), set_CC);
  cmp(FLAGS, imm(0), ne);
  orr(A1, A1, imm(0x80000000), mi);
  DOUBLE_RETURN_TO_CALLER

  comment("Here we are taking the sum of numbers with different signs.");
  comment("This could lead to cancellation so we're a little careful.");

  bind(_dadd_subtract);
  comment("negate the guard word, with carry then subtract");
  rsb(RESULTX, RESULTX, imm(0), set_CC);
  sbc(A2, A2, reg(B2), set_CC);
  sbc(A1, A1, reg(B1));
  comment("find a-priori denorms immediately.");
  cmp(EXPA, imm(1));
  b(_dadd_denorm_result, le);
  cmp(A1, imm(1<<EXPSHIFT));
  comment("could get lucky");
  b(_double_check_guard, ge);
  comment("The most significant bit of the result might be anywhere.");
  comment("The result may be denormalized.");
  comment("It might be zero.");
  comment("If it is zero, we must deliver +0 as a result.");
  comment("");
  comment("We're going to have to shift it. We want to shift LEFT");
  comment("until we can normalize. However, we might not be able to ");
  comment("deliver a normalized result.  We shift until either normalized");
  comment("or the EXPA decrements to 1, which we then treat as denormalized.");
  comment("");
  comment("If the EXPA is great enough, try to do 16- or 32-bit at a time");
  comment("shifts.");
  cmp(A1, imm(0));
  b(_dadd_hard_rounding_bitloop, ne);
  cmp(EXPA, imm(32));
  b(_dadd_small_result, le);
  cmp(A2, imm(0x8000));
  b(_dadd_hard_rounding_halfwordshift, hi);
  orr(TEMP, A2, imm_shift(RESULTX, lsr, 31), set_CC);
  b(_double_deliver_positive_zero, eq);
  mov(A1, reg(A2));
  mov(A2, reg(RESULTX));
  mov(RESULTX, imm(0));
  sub(EXPA, EXPA, imm(32));
  b(_dadd_hard_rounding_bitloop);

  bind(_dadd_hard_rounding_halfwordshift);
  mov(A1, imm_shift(A2, lsr, 16));
  mov(A2, imm_shift(A2, lsl, 16));
  orr(A2, A2, imm_shift(RESULTX, lsr, 16));
  mov(RESULTX, imm_shift(RESULTX, lsl, 16));
  sub(EXPA, EXPA, imm(16));
  b(_dadd_hard_rounding_bitloop);

  bind(_dadd_small_result);
  orr(TEMP, A1, reg(A2));
  orr(TEMP, TEMP, imm_shift(RESULTX, lsr, 31), set_CC);
  b(_dadd_hard_rounding_bitloop, ne);
  b(_double_deliver_positive_zero);

  comment("Here we try to normalize the result for");
  comment("the subtraction of normalized values.");
  comment("We know that the result is non-zero, but it may");
  comment("underflow to a signed zero result.");
  comment("A1 < (1<<EXPSHIFT) and A1|A2|RESULTX != 0 and EXPA > 1");

  bind(_dadd_hard_bitloop);
  add(RESULTX, RESULTX, reg(RESULTX), set_CC);
  adc(A2, A2, reg(A2), set_CC);
  adc(A1, A1, reg(A1));
  sub(EXPA, EXPA, imm(1));

  bind(_dadd_hard_rounding_bitloop);
  cmp(A1, imm(1<<EXPSHIFT));
  b(_double_check_guard, hs);
  cmp(EXPA, imm(1));
  b(_dadd_hard_bitloop, gt);
  b(_double_denorm_check_guard);

  bind(_dadd_subnormalA);
  DADD_DENORMAL( A1, A2, EXPA, imm(AZFLAG), _dadd_unpackB );

  bind(_dadd_subnormalB);
  DADD_DENORMAL( B1, B2, EXPB, imm(BZFLAG), _dadd_unpacked );

  bind(_dadd_exceptionalA);
  DOUBLE_EXCEPTIONAL( A1, A2, imm(AINFFLAG), _dadd_unpackB );

  bind(_dadd_exceptionalB);
  DOUBLE_EXCEPTIONAL( B1, B2, imm(BINFFLAG), _dadd_unpacked );

  bind(_dadd_unusual_operands);
  tst(FLAGS, imm(AINFFLAG|BINFFLAG));
  b(_dadd_infinite_operands, ne);
  comment("one or more zeros. No Infinities or NaNs here.");
  tst(FLAGS, imm(AZFLAG));
  comment("@ A != 0 so B == 0 so result is A");
  b(_dadd_deliver_a, eq);

  comment("here A is certainly zero.");
  comment("but since we arranged for abs(A) >= abs(B), B must be zero, too.");
  comment("So the result has to be zero.");
  comment("sign of result is AND of signs of operands");
  andr(A1, FLAGS, imm_shift(FLAGS, lsl, 1));
  andr(A1, A1, imm(0x80000000));
  mov(A2, imm(0));

  DOUBLE_RETURN_TO_CALLER;

  bind(_dadd_deliver_a);
  comment("if EXPA is \"1\", then treat it as a denorm");
  cmp(EXPA, imm(1));
  b(_double_done_rounding, ne);
  b(_double_denorm_done_rounding);

  comment("One or more Infinities. No NaNs.");

  bind(_dadd_infinite_operands);
  tst(FLAGS, imm(BINFFLAG));
  comment("A is, but B is not");
  b(_double_done_rounding, eq);
  comment("have two infinities:");
  teq(FLAGS, imm_shift(FLAGS, lsl, 1));
  comment("signs differ, deliver NaN");
  b(_double_deliver_NaN, mi);
  b(_double_deliver_Infinity);
}

void FloatSupport::generate_jvm_dsub() {
  Label dsub("jvm_dsub");
  Label dadd("jvm_dadd_software");

bind_global(dsub);
//  VFP_DOUBLE_2_TO_1("fsubd");

  comment("invert sign of arg 2");
  eor(B1, B1, imm(0x80000000));
  comment("and call dadd");
  b("jvm_dadd_software");

}

void FloatSupport::generate_jvm_dmul() {
  Label dmul("jvm_dmul");

#define PROD2   r8
#define PROD3   r9
#define INFFLAG  1
#define ZEROFLAG 2

#define DMULT_NORMALIZE(HiSrc, LoSrc, ExpReg, Lreturn )  \
        cmp(HiSrc, imm(0));                              \
        cmp(LoSrc, imm(0), eq);                          \
        orr(FLAGS, FLAGS, imm(ZEROFLAG), eq);            \
        b(Lreturn, eq);                                  \
        { /* we separate Label visibilities */           \
        Label l1;                                        \
            bind(l1);                                    \
            add(LoSrc, LoSrc, reg(LoSrc), set_CC);       \
            adc(HiSrc, HiSrc, reg(HiSrc));               \
            cmp(HiSrc, imm(1<<EXPSHIFT));                \
            sub(ExpReg, ExpReg, imm(1), lt);             \
            b(l1, lt);                                   \
        };                                               \
        b(Lreturn);

  Label
    _dmult_normalizeA, _dmult_normalizeB,
    _dmult_exceptionalA,  _dmult_exceptionalB,
    _dmult_unpackA, _dmult_unpackB, _dmult_unpacked,
    _dmult_special_operands,
    _dmult_round,
    _dmult_denorm_looping,
    _dmult_Infinite_operand;

  Label
    _double_check_guard("_double_check_guard"),
    _double_rounding("_double_rounding"),
    _double_round_up("_double_round_up"),
    _double_done_rounding("_double_done_rounding"),
    _double_deliver_Infinity("_double_deliver_Infinity"),
    _double_deliver_NaN("_double_deliver_NaN"),
    _double_deliver_signed_result("_double_deliver_signed_result"),
    _double_denorm_done_rounding("_double_denorm_done_rounding"),
    _double_denorm_round_up("_double_denorm_round_up"),
    _double_denorm_rounding("_double_denorm_rounding"),
    _double_deliver_signed_zero("_double_deliver_signed_zero"),
    _double_deliver_positive_zero("_double_deliver_positive_zero"),
    _double_denorm_check_guard("_double_denorm_check_guard"),
    _dmult_denorm_result("_dmult_denorm_result");

bind_global(dmul);
//  VFP_DOUBLE_2_TO_1("fmuld");

  stmfd(sp, DOUBLE_SAVE_SET, writeback);
  comment("capture sign of result");
  eor(FLAGS, A1, reg(B1), set_CC);
  comment("no thinking here");
  bic(A1, A1, imm(0x80000000));
  comment("no thinking here");
  bic(B1, B1, imm(0x80000000));
  comment("clear other flags");
  andr(FLAGS, FLAGS, imm(0x80000000));
  comment("exponent mask");
  // IMPL_NOTE
  mov_imm(EXPMASK, DOUBLE_EXPVAL);
  bind(_dmult_unpackA);
  DOUBLE_UNPACK( A1, EXPA, _dmult_normalizeA, _dmult_exceptionalA);

  bind(_dmult_unpackB);
  DOUBLE_UNPACK(B1, EXPB, _dmult_normalizeB, _dmult_exceptionalB);

  bind(_dmult_unpacked);
  tst(FLAGS, imm(ZEROFLAG|INFFLAG));
  b(_dmult_special_operands, ne);
  comment("Here we have a couple of normalized operands and");
  comment("will perform the multiply.");
  comment("Too bad the carry out of umlal is not trustworthy, ");
  comment("because we really need it for carry propagation! ");
  comment("Start low-end and work up to cut down on carries.");
  add(EXPA, EXPA, reg(EXPB));
  comment("@ subtract extra bias");
  sub(EXPA, EXPA, imm_shift(EXPMASK, lsr, 1));
  umull(RESULTX, PROD3, A2, B2);
  mov(PROD2, imm(0));
  umlal(PROD3, PROD2, A1, B2);
  comment("now done with B2 so can use it for scratch.");
  mov(B2, imm(0));
  umlal(PROD3, B2, A2, B1);
  add(PROD2, PROD2, reg(B2), set_CC);
  mov(B2, imm(0));
  comment("B2 gets carry out of above addition");
  adc(B2, B2, imm(0));
  umlal(PROD2, B2, A1, B1);
  comment("now have result in B2/PROD2/PROD3/RESULTX");
  comment("Now have a full 106 bits of result. The very highest order bit");
  comment("of the result may or may not be set. At most one shift will be");
  comment("necessary to set it (since we were normalized coming into this");
  comment("calculation");
  cmp(B2, imm(1<<9));
  add(EXPA, EXPA, imm(1), ge);
  b(_dmult_round, ge);
  add(RESULTX, RESULTX, reg(RESULTX), set_CC);
  adc(PROD3, PROD3, reg(PROD3), set_CC);
  adc(PROD2, PROD2, reg(PROD2), set_CC);
  adc(B2, B2, reg(B2));

  bind(_dmult_round);
  comment("Arrange the bits of our result to conform to the");
  comment("expected output.");
  comment("");
  comment("We have:");
  comment("10 bits in B2. The highest order is set and will");
  comment("     be the implicit bit of a normalized result.");
  comment("32 bits in PROD2");
  comment("32 bits in PROD3");
  comment("some more junk in RESULTX. We only care whether this is ==0");
  comment("or not.");
  mov(A1, imm_shift(B2, lsl, 11));
  orr(A1, A1, imm_shift(PROD2, lsr, 21));
  mov(A2, imm_shift(PROD2, lsl, 11));
  orr(A2, A2, imm_shift(PROD3, lsr, 21));
  cmp(RESULTX, imm(0));
  orr(PROD3, PROD3, imm(1), ne);
  mov(RESULTX, imm_shift(PROD3, lsl, 11));
  cmp(EXPA, imm(0));
  b(_dmult_denorm_result, le);

  bind(_double_check_guard);
  comment("Look at the guard and sticky bits of our result.");
  comment("Round accordingly.");
  comment("the Guard bit is now the sign of RESULTX");
  cmp(RESULTX, imm(0x80000000));
  b(_double_done_rounding, lo);
  b(_double_round_up, hi);
  comment("Round to even.");
  add(A2, A2, imm(1), set_CC);
  bic(A2, A2, imm(1));
  b(_double_rounding);

  bind(_double_round_up);
  add(A2, A2, imm(1), set_CC);

  bind(_double_rounding);
  adc(A1, A1, imm(0));
  comment("Now, if the result is 'too big', we carried all the way up.");
  comment("In theory we could shift back down, but we know that all the");
  comment("lower order bits have to be zero, so just jam them in.");
  cmp(A1, imm(1<<(EXPSHIFT+1)));
  b(_double_done_rounding, lt);
  add(EXPA, EXPA, imm(1));
  mov(A1, imm(1<<EXPSHIFT));
  mov(A2, imm(0));

  bind(_double_done_rounding);
  comment("Test for exponent too big.");
  cmp(EXPA, reg(EXPMASK));
  b(_double_deliver_Infinity, ge);
  comment("Clear the implicit bit, if present");
  bic(A1, A1, imm(0x00100000));
  comment("Assemble the result.");
  orr(A1, A1, imm_shift(EXPA, lsl, EXPSHIFT));

  bind(_double_deliver_signed_result);
  cmp(FLAGS, imm(0));
  orr(A1, A1, imm(0x80000000), mi);

  DOUBLE_RETURN_TO_CALLER

  bind(_double_deliver_NaN);
  mov_imm(A1,  0x7ff80000);
  mov(A2, imm(0));
  DOUBLE_RETURN_TO_CALLER

  bind(_dmult_normalizeA);
  DMULT_NORMALIZE( A1, A2, EXPA, _dmult_unpackB);

  bind(_dmult_normalizeB);
  DMULT_NORMALIZE( B1, B2, EXPB, _dmult_unpacked);

  bind(_dmult_exceptionalA);
  DOUBLE_EXCEPTIONAL( A1, A2, imm(INFFLAG), _dmult_unpackB);

  bind(_dmult_exceptionalB);
  DOUBLE_EXCEPTIONAL(B1, B2, imm(INFFLAG), _dmult_unpacked);

  comment("If here, we have some combination of zeros and infinities.");
  comment("Act accordingly.");
  bind(_dmult_special_operands);
  tst(FLAGS, imm(INFFLAG));
  b(_dmult_Infinite_operand, ne);

  bind(_double_deliver_signed_zero);
  comment("result is zero");
  mov(A1, imm(0));
  mov(A2, imm(0));
  b(_double_deliver_signed_result);

  bind(_double_deliver_positive_zero);
  comment("result is +zero");
  mov(A1, imm(0));
  mov(A2, imm(0));
  DOUBLE_RETURN_TO_CALLER

  bind(_dmult_Infinite_operand);
  tst(FLAGS, imm(ZEROFLAG));
  b(_double_deliver_NaN, ne);
  comment("else fall thru and deliver infinity");

  bind(_double_deliver_Infinity);
  mov(A1, imm_shift(EXPMASK, lsl, EXPSHIFT));
  mov(A2, imm(0));
  b(_double_deliver_signed_result);

  bind(_dmult_denorm_result);
  comment("The exponent is <= 0. This is really too bad.");
  comment("If it is really small ( <-52 ) I  believe we're safe to");
  comment("return a zero. Else we very carefully right shift until");
  comment("the exponent becomes zero, then go round and pack the result.");
  comment("Note that the fraction part is normalized, so we must shift it");
  comment("at least once to denormalize.");
  // cmp_imm doesn't work for some reasons
  cmn(EXPA, imm(52));
  b(_double_deliver_signed_zero, lt);

  comment("we always have to shift once even for EXPA of 0");
  comment("to achieve the denormalized form, then shift until EXPA");
  comment("turns to zero");
  comment("this accounts for the funny mov at the end of this loop");

  bind(_dmult_denorm_looping);
  mov(A1, imm_shift(A1, lsr, 1), set_CC);
  // IMPL_NOTE
  mov(A2, imm_shift(A2, ror, 0), set_CC);
  mov(RESULTX, imm_shift(RESULTX, ror, 0), set_CC);
  comment("this is why we call it sticky");
  orr(RESULTX, RESULTX, imm(1), cs);
  add(EXPA, EXPA, imm(1), set_CC);
  b(_dmult_denorm_looping, le);
  comment("if EXPA was 0 we just incremented it,");
  comment("so reset it back before continuing");
  mov(EXPA, imm(0));

  bind(_double_denorm_check_guard);
  comment("Look at the guard and sticky bits of our result.");
  comment("Round accordingly.");
  comment("See if we became normalized, and adjust exponent if so.");
  comment("This is very similar to _double_check_guard above");
  comment("except for the post-rounding tests.");
  comment("the Guard bit is now the sign of RESULTX");
  cmp(RESULTX, imm(0x80000000));
  b(_double_denorm_done_rounding, lo);
  b(_double_denorm_round_up, hi);
  comment("Round to even.");
  add(A2, A2, imm(1), set_CC);
  bic(A2, A2, imm(1));
  b(_double_denorm_rounding);

  bind(_double_denorm_round_up);
  add(A2, A2, imm(1), set_CC);

  bind(_double_denorm_rounding);
  adc(A1, A1, imm(0));
  comment("If the result is now normalized, we carried all the way up.");
  comment("But that's ok, because we just won't clear it out");
  comment("of the assembled result");
  comment("Put the sign on and go.");

  bind(_double_denorm_done_rounding);
  cmp(FLAGS, imm(0));
  orr(A1, A1, imm(0x80000000), mi);
  DOUBLE_RETURN_TO_CALLER
}

void FloatSupport::generate_jvm_ddiv() {
  Label ddiv("jvm_ddiv");

#define QUOT1   EXPB
#define QUOT2   r8
#define ITER    r9
  Label
    _ddivUnpackB, _ddivUnpacked,
    _ddivNormalizeA, _ddivNormalizeB,
    _ddivExceptionalA, _ddivExceptionalB,
    _ddivSpecialOperands,
    _ddivFirstLoopTop, _ddivFirstLoopEntry, _ddivSecondLoopTop;

 Label
    _double_deliver_signed_zero("_double_deliver_signed_zero"),
    _double_deliver_Infinity("_double_deliver_Infinity"),
    _double_deliver_NaN("_double_deliver_NaN"),
    _double_check_guard("_double_check_guard"),
    _dmult_denorm_result("_dmult_denorm_result");

bind_global(ddiv);
//  VFP_DOUBLE_2_TO_1("fdivd");

  comment("In addition to the above conventions, working register are:");
  comment("QUOT1 for first quotient word (after done with EXPB)");
  comment("QUOT2 for second quotient word");
  comment("ITER  for the loop counter");

  stmfd(sp, DOUBLE_SAVE_SET, writeback);
  comment("capture sign of result");
  eor(FLAGS, A1, reg(B1), set_CC);
  comment("no thinking here");
  bic(A1, A1, imm(0x80000000));
  comment("no thinking here");
  bic(B1, B1, imm(0x80000000));
  comment("clear other flags");
  andr(FLAGS, FLAGS, imm(0x80000000));
  comment("exponent mask");
  mov_imm(EXPMASK, DOUBLE_EXPVAL);
  DOUBLE_UNPACK( A1, EXPA, _ddivNormalizeA, _ddivExceptionalA );

  bind( _ddivUnpackB);
  DOUBLE_UNPACK( B1, EXPB, _ddivNormalizeB, _ddivExceptionalB );

  bind(_ddivUnpacked);
  tst(FLAGS, imm(AZFLAG|BZFLAG|AINFFLAG|BINFFLAG));
  b(_ddivSpecialOperands, ne);

  comment("Here we have a couple of normalized operands and");
  comment("will perform the division.");
  sub(EXPA, EXPA, reg(EXPB));
  comment("add extra bias");
  add(EXPA, EXPA, imm_shift(EXPMASK, lsr, 1));
  comment("Make sure the first digit is a '1' by increasing A if");
  comment("necessary");
  cmp(A1, reg(B1));
  cmp(A2, reg(B2), eq);
  Label t0;
  b(t0, hs);
  add(A2, A2, reg(A2), set_CC);
  adc(A1, A1, reg(A1));
  sub(EXPA, EXPA, imm(1));

  bind(t0);

  comment("Develop the high-order 21 quotient bits in QUOT1.");
  comment("Develop the lower-order 32 quotient bits in QUOT2");
  comment("Develop the lowest-order 1 quotient bit in RESULTX");
  comment("");
  comment("Try non-restoring divide here. It appears easier, maybe.");

#define DDIV_SHIFT_REMAINDER(label)   \
        add(A2, A2, reg(A2), set_CC); \
        adc(A1, A1, reg(A1), set_CC); \
        b(label, lt);

#define DDIV_STEP(label1, label2)     \
        sub(A2, A2, reg(B2), set_CC); \
        sbc(A1, A1, reg(B1), set_CC); \
        b(label2);                    \
        bind(label1);                 \
        add(A2, A2, reg(B2), set_CC); \
        adc(A1, A1, reg(B1), set_CC); \
        bind(label2);

  Label t1, t2;

  mov(QUOT2, imm(0));
  mov(QUOT1, imm(0));
  mov(ITER, imm(21));
  b(_ddivFirstLoopEntry);

  bind(_ddivFirstLoopTop);
  DDIV_SHIFT_REMAINDER(t1);

  bind(_ddivFirstLoopEntry);
  DDIV_STEP(t1, t2);
  adc(QUOT1, QUOT1, reg(QUOT1));
  sub(ITER, ITER, imm(1), set_CC);
  b(_ddivFirstLoopTop, gt);
  comment("second 32 bits of quotient into QUOT2");
  mov(ITER, imm(32));

  Label t3, t4;
  bind(_ddivSecondLoopTop);
  DDIV_SHIFT_REMAINDER(t3);
  DDIV_STEP(t3, t4);
  adc(QUOT2, QUOT2, reg(QUOT2));
  sub(ITER, ITER, imm(1), set_CC);
  b(_ddivSecondLoopTop, gt);

  comment("compute guard bit into high-order bit of RESULTX");
  comment("we know ITER is 0 at this point.");
  Label t5, t6, t7;
  DDIV_SHIFT_REMAINDER(t5);
  DDIV_STEP(t5, t6);
  mov(RESULTX, imm_shift(ITER, ror, 0));
  comment("adjust remainder");
  cmp(A1, imm(0));
  b(t7, ge);
  add(B2, A2, reg(B2), set_CC);
  adc(B1, A1, reg(B1));

  bind(t7);
  comment("Result is now in QUOT1 | QUOT2 | RESULTX and EXPA");
  comment("The remainder is in B1 | B2");
  comment("Put result in the form that is expected by");
  comment("the shared rounding and packing code.");
  mov(A1, reg(QUOT1));
  mov(A2, reg(QUOT2));
  orr(B1, B1, reg(B2), set_CC);
  comment("sticky bit");
  orr(RESULTX, RESULTX, imm(0x40000000), ne);
  cmp(EXPA, imm(0));
  b(_double_check_guard, gt);
  b(_dmult_denorm_result);

  bind(_ddivNormalizeA);
  DOUBLE_NORMALIZE( A1, A2, EXPA, _ddivUnpackB, imm(AZFLAG));

  bind(_ddivNormalizeB);
  DOUBLE_NORMALIZE( B1, B2, EXPB, _ddivUnpacked, imm(BZFLAG));

  bind(_ddivExceptionalA);
  DOUBLE_EXCEPTIONAL( A1, A2, imm(AINFFLAG), _ddivUnpackB );

  bind(_ddivExceptionalB);
  DOUBLE_EXCEPTIONAL( B1, B2, imm(BINFFLAG), _ddivUnpacked );

  comment("If here, we have some combination of zeros and infinities.");
  comment("Act accordingly.");

  bind(_ddivSpecialOperands);
  comment("disregard sign");
  bic(RESULTX, FLAGS, imm(0x80000000));
  teq(RESULTX, imm(AINFFLAG|BINFFLAG));
  comment("Inf / Inf => NaN");
  b(_double_deliver_NaN, eq);
  teq(RESULTX, imm(AZFLAG|BZFLAG));
  comment("0 / 0 => NaN");
  b(_double_deliver_NaN, eq);
  tst(RESULTX, imm(AINFFLAG|BZFLAG));
  comment("Inf / non-Inf == non-0 / 0 => Inf");
  b(_double_deliver_Infinity, ne);
  comment("resulting cases are:");
  comment("non-Inf / Inf => 0");
  comment("0 / non-0     => 0");
  b(_double_deliver_signed_zero);
}

void FloatSupport::generate_jvm_drem() {
  STUB(jvm_drem);
}

void FloatSupport::generate_jvm_l2f() {
  STUB(jvm_l2f);
}

void FloatSupport::generate_jvm_l2d() {
  STUB(jvm_l2d);
}

void FloatSupport::generate_jvm_f2l() {
  STUB(jvm_f2l);
}

void FloatSupport::generate_jvm_f2d() {
  Label f2d("jvm_f2d");
  Label _internal("jvm_f2d_internal");
  bind(f2d);

//  VFP_INT_1_TO_1("fcvtds");

  import(_internal);
  b(_internal);
}

void FloatSupport::generate_jvm_d2l() {
  STUB(jvm_d2l);
}

void FloatSupport::generate_jvm_d2f() {
  Label d2f("jvm_d2f");
  Label _internal("jvm_d2f_internal");
  bind(d2f);

//  VFP_DOUBLE_1_TO_1("fcvtsd");

  import(_internal);
  b(_internal);
}

#undef GENERATE
#undef STUB

#endif // ENABLE_INTERPRETER_GENERATOR

//////////////////////////////////////////////////////////
// Start C routines used to support FP operations on ARM
//////////////////////////////////////////////////////////

extern "C" {

#define float32 float
#define float64 double
#define int64   jlong
#define uint64  julong
#define flag    jboolean
#define bits16  jchar
#define bits32  juint
#define bits64  julong
#define sbits32 jint
#define int16   short
#define int8    jbyte
#define INLINE  inline

#if USE_SUN_JVM_L2F

  static inline int countLeadingZeros32(bits32 a) {
    int shiftCount = 0;
    // this code is 30% faster, but gives bigger footprint
#if NOT_CURRENTLY_USED
    static const int8 countLeadingZerosHigh[] = {
       8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
       3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
       2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
       2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
     };
    if (a < 0x10000) {
       shiftCount += 16;
       a <<= 16;
    }
    if (a < 0x1000000) {
       shiftCount += 8;
       a <<= 8;
    }
    shiftCount += countLeadingZerosHigh[a>>24];
#else
    if (a == 0) {
      return 32;
    }

    int delta = 16;
    while (delta > 0) {
      if (a < ((bits32)1 << (32 - delta - shiftCount))) {
        shiftCount += delta;
      }
      delta >>= 1;
    }
#endif
    return shiftCount;
  }

  static inline int countLeadingZeros64(bits64 a) {
    int shiftCount = 0;

    shiftCount = 0;
    if (a < ((bits64)1) << 32) {
      shiftCount += 32;
    } else {
      a >>= 32;
    }
    shiftCount += countLeadingZeros32((bits32)a);
    return shiftCount;
  }
#endif

  INLINE void shift32RightJamming(bits32 a, int16 count, bits32 *zPtr) {
    bits32 z;

    if (count == 0) {
      z = a;
    } else if (count < 32) {
      z = (a >> count) | ((a << ((-count) & 31)) != 0);
    } else {
      z = (a != 0);
    }
    *zPtr = z;
  }

  INLINE void shift64RightJamming(bits64 a, int16 count, bits64 *zPtr) {
    bits64 z;

    if (count == 0) {
      z = a;
    } else if (count < 64) {
      z = (a >> count) | ((a << ((-count) & 63)) != 0);
    } else {
#ifdef __SYMBIAN32__
      jlong_accessor _a;
      _a.long_value = a;
      z = (_a.words[0] != 0 || _a.words[1] != 0);
#else
      z = (a != 0);
#endif
    }
    *zPtr = z;
  }

  INLINE float32 packFloat32(flag zSign, int16 zExp, bits32 zSig) {
    int rv = (((bits32) zSign) << 31) + (((bits32) zExp) << 23) + zSig;
    return *(float32*)&rv;
  }

#if USE_SUN_JVM_L2F || USE_SUN_JVM_D2F
  static float32 roundAndPackFloat32(flag zSign, int16 zExp, bits32 zSig) {
    int8 roundIncrement, roundBits;

    roundIncrement = 0x40;
    roundBits = (int8)(zSig & 0x7F);
    if (0xFD <= (bits16) zExp) {
      if ((0xFD < zExp)
          || ((zExp == 0xFD)
          && ((sbits32)(zSig + roundIncrement) < 0))) {
        return packFloat32( zSign, 0xFF, 0 );
      }
      if (zExp < 0) {
        shift32RightJamming(zSig, -zExp, &zSig);
        zExp = 0;
        roundBits = (int8)(zSig & 0x7F);
      }
    }
    zSig = (zSig + roundIncrement) >> 7;
    zSig &= ~ (((roundBits ^ 0x40) == 0) & 1);
    if (zSig == 0) {
      zExp = 0;
    }
    return packFloat32(zSign, zExp, zSig);
  }
#endif

#if USE_SUN_JVM_L2F
  static float32 int64_to_float32(int64 a) {
    flag zSign;
    uint64 absA;
    int shiftCount;

    if (a == 0) {
      return 0;
    }
    zSign = (a < 0);
    absA = zSign ? - a : a;
    shiftCount = countLeadingZeros64(absA) - 40;

    if (0 <= shiftCount) {
      return packFloat32(zSign, 0x95 - shiftCount, (bits32)(absA << shiftCount));
    } else {
      shiftCount += 7;
      if (shiftCount < 0) {
        shift64RightJamming(absA, - shiftCount, &absA);
      } else {
        absA <<= shiftCount;
      }
      return roundAndPackFloat32(zSign, 0x9C - shiftCount, (bits32)absA);
    }
  }
#endif

#if USE_SUN_JVM_D2F
  static float32 float64_to_float32(float64 a) {
    flag aSign;
    int16 aExp;
    bits64 aSig;
    bits32 zSig;

    juint raw_hi = __JHI(a);
    juint raw_lo = __JLO(a);

    aSig = jlong_from_msw_lsw(raw_hi & 0x000FFFFF, raw_lo);
    aExp = (int16)((raw_hi >> 20) & 0x7FF);
    aSign = (flag)(raw_hi >> 31);
    if (aExp == 0x7FF) {
      if (aSig) {
        juint nan = floatNaN;
        // need revisit
        return *(jfloat*)&nan;
      }
      return packFloat32(aSign, 0xFF, 0);
    }
    shift64RightJamming(aSig, 22, &aSig);
    zSig = (bits32)aSig;
    if (aExp || zSig) {
      zSig |= 0x40000000;
      aExp -= 0x381;
    }
    return roundAndPackFloat32(aSign, aExp, zSig);
  }
#endif

#undef float32
#undef float64
#undef int64
#undef uint64
#undef flag
#undef bits16
#undef bits32
#undef bits64
#undef int16
#undef int8
#undef INLINE

#if !ENABLE_INTERPRETER_GENERATOR && !CROSS_GENERATOR

  JVM_SOFTFP_LINKAGE
  jfloat jvm_frem(jfloat x, jfloat y)    {
    return jvm_d2f(ieee754_fmod(jvm_f2d(x), jvm_f2d(y)));
  }

  JVM_SOFTFP_LINKAGE
  jdouble jvm_drem(jdouble x, jdouble y) {
    return ieee754_fmod(x, y);
  }

#if !USE_SUN_JVM_FADD
  JVM_SOFTFP_LINKAGE
  jfloat jvm_fadd(jfloat x, jfloat y)    { return x + y; }
#endif

#if !USE_SUN_JVM_FSUB
  JVM_SOFTFP_LINKAGE
  jfloat jvm_fsub(jfloat x, jfloat y)    { return x - y; }
#endif

#if !USE_SUN_JVM_FMUL
  JVM_SOFTFP_LINKAGE
  jfloat jvm_fmul(jfloat x, jfloat y)    { return x * y; }
#endif

#if !USE_SUN_JVM_FDIV
  JVM_SOFTFP_LINKAGE
  jfloat jvm_fdiv(jfloat x, jfloat y)    { return x / y; }
#endif

#if !USE_SUN_JVM_DADD
  JVM_SOFTFP_LINKAGE
  jdouble jvm_dadd(jdouble x, jdouble y) { return x + y; }
#endif

#if !USE_SUN_JVM_DSUB
  JVM_SOFTFP_LINKAGE
  jdouble jvm_dsub(jdouble x, jdouble y) { return x - y; }
#endif

#if !USE_SUN_JVM_DMUL
  JVM_SOFTFP_LINKAGE
  jdouble jvm_dmul(jdouble x, jdouble y) { return x * y; }
#endif

#if !USE_SUN_JVM_DDIV
  JVM_SOFTFP_LINKAGE
  jdouble jvm_ddiv(jdouble x, jdouble y) { return x / y; }
#endif

#if !USE_SUN_JVM_I2F
  JVM_SOFTFP_LINKAGE
  jfloat  jvm_i2f(jint x)                { return (jfloat)x;  }
#endif

#if !USE_SUN_JVM_I2D
  JVM_SOFTFP_LINKAGE
  jdouble jvm_i2d(jint x)                { return (jdouble)x; }
#endif

#if !USE_SUN_JVM_F2I
  JVM_SOFTFP_LINKAGE
  jint    jvm_f2i(jfloat x)              { return (jint)x;    }
#endif

  JVM_SOFTFP_LINKAGE
  jdouble jvm_l2d(jlong x)               {
#if USE_SUN_JVM_L2D
    jint lo = lsw(x);
    jint hi = msw(x);

    jdouble dlo = jvm_i2d(lo);
    jdouble dhi;

    if (lo < 0) {
      // lo looks signed, but it is really an unsigned 32-bit number
      // Compensate by incrementing hi
      hi++;
    }
    if (hi == 0) {
      // This was really just an integer.  We can handle it quickly
      return dlo;
    } else {
      if (hi == 0x80000000 && lo < 0) {
        // This was an overflow.  hi looks negative but is positive
        // Construct +1 << 63 by hand.
        __JHI(dhi) = (0x3ff + 63) << 20;
        __JLO(dhi) = 0;
      } else {
        dhi = jvm_i2d(hi);
        // We know that dhi isn't zero, and that its exponent is within a
        // reasonable range.  So we can multiply it by 2^32 rather trivially
        __JHI(dhi) += (32 << 20);
      }
      return jvm_dadd(dhi, dlo);
    }
#else
    return (jdouble)x;
#endif  // USE_SUN_JVM_L2D
  }

#if USE_SUN_JVM_F2D
  JVM_SOFTFP_LINKAGE
  jdouble jvm_f2d_internal(jfloat x)     {
    jdouble result;
    jint raw = *(int *)&x;
    jint exp = raw & (0xFF << 23); // The shifted exponent
    if (exp == 0) {
      // We have a denormal
      if ((raw & 0x7FFFFFFF) == 0) {
        // return appropriately signed zero
        __JHI(result) = raw;
        __JLO(result) = 0;
      } else {
        // Other denormals can be treated just like integers
        result = jvm_i2d(raw & right_n_bits(23));
        __JHI(result) -= ((0x7E + 23) << 20) - (raw & 0x80000000);
      }
    } else if (exp == (0xFF << 23)) {
      // infinite or NaN;
      __JHI(result) = (raw & 0x80000000) + (0x7ff << 20);
      __JLO(result) = raw & right_n_bits(23);
    } else {
      __JHI(result) = (raw & 0x80000000)          // sign
        + ((exp >> 3) + ((0x3FF - 0x7F) << 20))  // expt
        + ((raw & right_n_bits(23)) >> 3);       // high mantissa
      __JLO(result) = raw << 29;                  // low mantissa
    }
    return result;
  }
#else
  JVM_SOFTFP_LINKAGE
  jdouble jvm_f2d(jfloat x)              {
    return (jdouble)x;
  }
#endif // USE_SUN_JVM_F2D

  JVM_SOFTFP_LINKAGE
  jfloat  jvm_l2f(jlong x)               {
#if USE_SUN_JVM_L2F
    return int64_to_float32(x);
#else
    return (jfloat)x;
#endif //  USE_SUN_JVM_L2F
  }

#if USE_SUN_JVM_D2F
  JVM_SOFTFP_LINKAGE
  jfloat  jvm_d2f_internal(jdouble x)    {
    return float64_to_float32(x);
  }
#else
  JVM_SOFTFP_LINKAGE
  jfloat  jvm_d2f(jdouble x)             {
    return (jfloat)x;
  }
#endif // USE_SUN_JVM_D2F

  JVM_SOFTFP_LINKAGE
  jlong   jvm_f2l(jfloat x)              {
#if USE_SUN_JVM_F2L
    jint raw = *(int *)&x;

    // expp is the unbiased exponent and mantissa
    jint expp = (raw & 0x7FFFFFFF) - (0x7F << 23);
    if (expp < 0) {
      // value is less 1.0.  We can just return 0.
      return 0;
    } else if (expp >= (63 << 23)) {
      // Value is either too large to fit into a long, or else it is NaN
      if (expp > ((0xFF - 0x7F) << 23)) {
        // Nan.  By definition, we return 0.
        return 0;
      } else {
        return raw < 0 ? min_jlong : max_jlong;
      }
    } else {
      // The implicit mantissa
      juint  mantissa = (raw << 8) | 0x80000000;
      // The amount by which this has to be shifted
      int shift = 63 - (expp >> 23);
      GUARANTEE(shift >= 1 && shift <= 63, "Sanity");

      julong uresult = jlong_from_msw_lsw(mantissa, 0);
      uresult =  uresult >> shift;

      jlong result = (raw < 0) ?  -(jlong)uresult : (jlong)uresult;

      //printf("f2l: %f %lld raw=%d %lld\n", x, result, raw, uresult);

      return result;
    }
#else
    return (jlong)x;
#endif // USE_SUN_JVM_F2L
  }

  JVM_SOFTFP_LINKAGE
  jlong   jvm_d2l(jdouble x)             {
#if USE_SUN_JVM_D2L
    juint raw_hi = __JHI(x);
    juint raw_lo = __JLO(x);
    jint expp = (raw_hi & 0x7FFFFFFF) - (right_n_bits(11 - 1) << 20);
    if (expp < 0) {
      return 0;
    } else if (expp >= (63 << 20)) {
      if (expp >= ((0x7FF - right_n_bits(10)) << 20)) {
      // Infinite or NaN.  Very rare, I hope
        if (expp > ((0x7FF - right_n_bits(10)) << 20) || raw_lo != 0) {
          // NaN
          return 0;
        }
      }
      return ((jint)raw_hi) < 0 ? min_jlong : max_jlong;
    } else {
      juint mantissa_hi =  (((raw_hi & right_n_bits(20)) << 11) + (raw_lo >> 21))
        | 0x80000000;
      juint mantissa_lo = (raw_lo << 11);
      julong mantissa = jlong_from_msw_lsw(mantissa_hi, mantissa_lo);
      int shift = 63 - (expp >> 20);
      GUARANTEE(shift >= 1 && shift <= 63, "Sanity");
      julong uresult = mantissa >> shift;
      jlong result = ((jint)raw_hi < 0) ?  -(jlong)uresult : (jlong)uresult;
      return result;
    }
#else
    return (jlong)x;
#endif // USE_SUN_JVM_D2L
  }

#if !USE_SUN_JVM_D2I
  JVM_SOFTFP_LINKAGE
  jint    jvm_d2i(jdouble x)             { return (jint)x;    }
#endif

#if !USE_SUN_JVM_FCMP
  JVM_SOFTFP_LINKAGE
  jint    jvm_fcmpl(jfloat x, jfloat y)  {
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
  }

  JVM_SOFTFP_LINKAGE
  jint    jvm_fcmpg(jfloat x, jfloat y)   {
    return  ((x > y)   ?  1   :
       (x == y)  ?  0 :
       (x < y)   ? -1 : 1);
  }
#endif

#if !USE_SUN_JVM_DCMP
  JVM_SOFTFP_LINKAGE
  jint    jvm_dcmpl(jdouble x, jdouble y) {
    return  ((x > y) ? 1 : ( x == y) ? 0 : -1);
  }

  JVM_SOFTFP_LINKAGE
  jint    jvm_dcmpg(jdouble x, jdouble y)  {
    return  ((x > y)   ?  1   :
       (x == y)  ?  0 :
       (x < y)   ? -1 : 1);
  }
#endif

#endif // !ENABLE_INTERPRETER_GENERATOR

  JVM_SOFTFP_LINKAGE
  jdouble jvm_dneg(jdouble x)           {
#if USE_SUN_JVM_DNEG
    jdouble y;

    __JHI(y) = __JHI(x) ^ 0x80000000;
    __JLO(y) = __JLO(x);
    return y;
#else
    return -x;
#endif //  USE_SUN_JVM_DNEG
  }

} // extern "C"

#endif //  ENABLE_FLOAT
