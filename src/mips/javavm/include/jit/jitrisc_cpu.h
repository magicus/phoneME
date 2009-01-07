/*
 * @(#)jitrisc_cpu.h	1.28 06/10/10
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
 *
 */

#ifndef _INCLUDED_MIPS_JITRISC_CPU_H
#define _INCLUDED_MIPS_JITRISC_CPU_H

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * This file #defines all of the macros that shared risc parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * mips specific parts of the jit, including those with the
 * CVMMIPS_ prefix.
 */

/************************************************
 * Register Definitions.
 ************************************************/

/* all of the general purpose registers (32 of 'em) */
#define CVMMIPS_r0  0
#define CVMMIPS_r1  1
#define CVMMIPS_r2  2
#define CVMMIPS_r3  3
#define CVMMIPS_r4  4
#define CVMMIPS_r5  5
#define CVMMIPS_r6  6
#define CVMMIPS_r7  7
#define CVMMIPS_r8  8
#define CVMMIPS_r9  9
#define CVMMIPS_r10 10
#define CVMMIPS_r11 11
#define CVMMIPS_r12 12
#define CVMMIPS_r13 13
#define CVMMIPS_r14 14
#define CVMMIPS_r15 15
#define CVMMIPS_r16 16
#define CVMMIPS_r17 17
#define CVMMIPS_r18 18
#define CVMMIPS_r19 19
#define CVMMIPS_r20 20
#define CVMMIPS_r21 21
#define CVMMIPS_r22 22
#define CVMMIPS_r23 23
#define CVMMIPS_r24 24
#define CVMMIPS_r25 25
#define CVMMIPS_r26 26
#define CVMMIPS_r27 27
#define CVMMIPS_r28 28
#define CVMMIPS_r29 29
#define CVMMIPS_r30 30
#define CVMMIPS_r31 31

#define CVMMIPS_zero	0
#define CVMMIPS_at	1
#define CVMMIPS_v0	2
#define CVMMIPS_v1	3
#define CVMMIPS_a0	4
#define CVMMIPS_a1	5
#define CVMMIPS_a2	6
#define CVMMIPS_a3	7
#if (CVMCPU_MAX_ARG_REGS == 4)
#define CVMMIPS_t0	8
#define CVMMIPS_t1	9
#define CVMMIPS_t2	10
#define CVMMIPS_t3	11
#elif (CVMCPU_MAX_ARG_REGS == 8)
#define CVMMIPS_a4	8
#define CVMMIPS_a5	9
#define CVMMIPS_a6	10
#define CVMMIPS_a7	11
#else
#error Illegal value for CVMCPU_MAX_ARG_REGS
#endif
#define CVMMIPS_t4	12
#define CVMMIPS_t5	13
#define CVMMIPS_t6	14
#define CVMMIPS_t7	15
#define CVMMIPS_s0	16
#define CVMMIPS_s1	17
#define CVMMIPS_s2	18
#define CVMMIPS_s3	19
#define CVMMIPS_s4	20
#define CVMMIPS_s5	21
#define CVMMIPS_s6	22
#define CVMMIPS_s7	23
#define CVMMIPS_t8	24
#define CVMMIPS_t9	25
#define CVMMIPS_kt0	26
#define CVMMIPS_kt1	27
#define CVMMIPS_gp	28
#define CVMMIPS_sp	29
#define CVMMIPS_s8	30
#define CVMMIPS_lr	31

/*
 * Macro to map a register name (like a0) to a register number (like 4).
 * Because of the strange way the preprocessor expands macros, we need
 * an extra indirection of macro invocations to get it expanded correctly.
 */
#define CVMCPU_MAP_REGNAME_0(regname) CVMMIPS_##regname
#define CVMCPU_MAP_REGNAME(regname)   CVMCPU_MAP_REGNAME_0(regname)

/* Some registers known to regman and codegen.jcs */
#define CVMCPU_SP_REG           CVMMIPS_sp
#define CVMCPU_JFP_REG     	CVMCPU_MAP_REGNAME(CVMMIPS_JFP_REGNAME)
#define CVMCPU_JSP_REG     	CVMCPU_MAP_REGNAME(CVMMIPS_JSP_REGNAME)
#define CVMCPU_PROLOGUE_PREVFRAME_REG \
    				CVMCPU_MAP_REGNAME(CVMMIPS_PREVFRAME_REGNAME)
#define CVMCPU_PROLOGUE_NEWJFP_REG \
    				CVMCPU_MAP_REGNAME(CVMMIPS_NEWJFP_REGNAME)
#define CVMCPU_ZERO_REG         CVMMIPS_zero
#define CVMCPU_INVALID_REG (-1)

/* 
 * More registers known to regman and codegen.jcs. These ones are
 * optional. If you have registers to spare then defining these macros
 * allows for more efficient code generation. The registers will
 * be setup by CVMJITgoNative() when compiled code is first entered.
 */
#undef  CVMCPU_CVMGLOBALS_REG
#define CVMCPU_CHUNKEND_REG    CVMCPU_MAP_REGNAME(CVMMIPS_CHUNKEND_REGNAME)
#define CVMCPU_EE_REG          CVMCPU_MAP_REGNAME(CVMMIPS_EE_REGNAME)
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_CP_REG          CVMCPU_MAP_REGNAME(CVMMIPS_CP_REGNAME)
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GC_REG          CVMCPU_MAP_REGNAME(CVMMIPS_GC_REGNAME)
#endif

/* registers for C helper arguments */
#define CVMCPU_ARG1_REG    CVMMIPS_a0
#define CVMCPU_ARG2_REG    CVMMIPS_a1
#define CVMCPU_ARG3_REG    CVMMIPS_a2
#define CVMCPU_ARG4_REG    CVMMIPS_a3

/* registers for C helper result */
#define CVMCPU_RESULT1_REG   CVMMIPS_v0
#ifndef CVMCPU_HAS_64BIT_REGISTERS
#define CVMCPU_RESULT2_REG   CVMMIPS_v1
#endif

/* 
 * The set of registers dedicated for PHI spills. Usually the
 * first PHI is at the bottom of the range of available non-volatile registers.
 */
#if defined(CVMCPU_HAS_CP_REG) || defined(CVMJIT_TRAP_BASED_GC_CHECKS)
#define MIPS_PHI_REG_s3			0
#else
#define MIPS_PHI_REG_s3			(1U << CVMMIPS_s3)
#endif

#define CVMCPU_PHI_REG_SET (			\
    MIPS_PHI_REG_s3  | 1U << CVMMIPS_s4 |	\
    1U << CVMMIPS_s5 | 1U << CVMMIPS_s6 |	\
    1U << CVMMIPS_s7	       			\
)

/* range of registers that regman should look at */
#define CVMCPU_MIN_INTERESTING_REG      CVMMIPS_r1
#define CVMCPU_MAX_INTERESTING_REG      CVMMIPS_lr

/*******************************************************
 * Register Sets
 *******************************************************/

/* The set of all registers */
#define CVMCPU_ALL_SET  0xffffffff

/*
 * Registers that regman should never allocate. Only list registers
 * that haven't already been exported to regman. For example, there's
 * no need to tell regman about CVMCPU_SP_REG because regman already
 * knows that it is busy.
 */
#define CVMCPU_BUSY_SET (						\
    1U<<CVMMIPS_r0 | /* r0 is always #0 */				\
    1U<<CVMMIPS_t7 | /* reserved as a scratch register */               \
    1U<<CVMMIPS_kt0 | 1U<<CVMMIPS_kt1) /* for kernel use only */

/*
 * The set of all non-volatile registers according to C calling conventions.
 * There's no need to put CVMCPU_BUSY_SET registers in this set.
 */
#define CVMCPU_NON_VOLATILE_SET (  /* s0-s8,gp */	       		\
    1U<<CVMMIPS_s0 | 1U<<CVMMIPS_s1 | 1U<<CVMMIPS_s2 | 1U<<CVMMIPS_s3 |	\
    1U<<CVMMIPS_s4 | 1U<<CVMMIPS_s5 | 1U<<CVMMIPS_s6 | 1U<<CVMMIPS_s7 |	\
    1U<<CVMMIPS_s8)

/* The set of all volatile registers according to C calling conventions */
#define CVMCPU_VOLATILE_SET \
    (CVMCPU_ALL_SET & ~CVMCPU_NON_VOLATILE_SET)

/*
 * The set of registers that regman should only use as a last resort.
 *
 * Usually this includes the set of phi registers, but if you have a limited
 * number of non-phi non-volatile registers available, then at most only
 * include the first couple of phi registers.
 *
 * It also should include the argument registers, since we would rather
 * that other non-volatile registers get allocated before these. Also
 * include the result registers if they don't overlap the argument registers.
 */
#define CVMCPU_AVOID_SET (						  \
    /* argument registers we make use of */				  \
    1U<<CVMMIPS_a0 | 1U<<CVMMIPS_a1 | 1U<<CVMMIPS_a2 | 1U<<CVMMIPS_a3 |	  \
    /* result registers we make use of */				  \
    1U<<CVMMIPS_v0 | 1U<<CVMMIPS_v1

/*
 * Set of registers that can be used for the jsr return address. If the LR
 * is a GPR, then just set it to the LR reg. Otherwise usually it is best
 * to allow any register to be used.
 */
#define CVMCPU_JSR_RETURN_ADDRESS_SET (1U<<CVMMIPS_lr)

/*
 * Alignment parameters of integer registers are trivial
 * All quantities in 32-bit words
 */
#define CVMCPU_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_DOUBLE_REG_ALIGNMENT	1

/*************************************************************
 * In case we opt for the FPU, we'll need these parameters too
 *************************************************************/

#ifdef CVM_JIT_USE_FP_HARDWARE

/*
 * More work is needed to support NV FPRs when we have 64-bit FPRs.
 * For now there are no NV FPRS when there are 64-bit FPRs.
 */
#ifndef CVMMIPS_HAS_64BIT_FP_REGISTERS
#define CVMMIPS_USE_NV_FP_REGISTERS
#endif
									  
/*
 * FP register sets
 */

#define CVMCPU_FP_MIN_INTERESTING_REG	0
#define CVMCPU_FP_BUSY_SET 0
/* f0-f19 are volatile */
#define CVMCPU_FP_VOLATILE_SET     0x000fffff

#ifdef CVMMIPS_USE_NV_FP_REGISTERS
/* we'll by using f0-f27 */
#define CVMCPU_FP_MAX_INTERESTING_REG	27
#define CVMCPU_FP_ALL_SET 0x0fffffff
/* f20-f31 are non-volatile. However, in order to save overhead
 * in CVMJITgoNative, we only ever use 8: f20-f27 */
#define CVMCPU_FP_NON_VOLATILE_SET 0x0ff00000
#else
/* we'll by using f0-f19 */
#define CVMCPU_FP_MAX_INTERESTING_REG	19
#define CVMCPU_FP_ALL_SET 0x000fffff
/* no non-volatile FPRs */
#define CVMCPU_FP_NON_VOLATILE_SET 0x00000000
#endif


#ifdef CVMMIPS_USE_NV_FP_REGISTERS
#define CVMCPU_FP_PHI_REG_SET (	\
    1U << 20 | 1U << 21 |	\
    1U << 22 | 1U << 23	|	\
    1U << 24 | 1U << 25	|	\
    1U << 26 | 1U << 27		\
)
#else
#define CVMCPU_FP_PHI_REG_SET (	\
    1U << 0 | 1U << 1 |		\
    1U << 2 | 1U << 3		\
)
#endif

/*
 * Alignment parameters of floating registers.
 * "2" means that the registers are allocated as pairs.
 * All quantities in 32-bit words
 */
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
#define CVMCPU_FP_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_FP_DOUBLE_REG_ALIGNMENT	1
#else
#define CVMCPU_FP_SINGLE_REG_ALIGNMENT	2
#define CVMCPU_FP_DOUBLE_REG_ALIGNMENT	2
#endif

#endif /* CVM_JIT_USE_FP_HARDWARE */

/************************************************************************
 * CPU features - These macros define various features of the processor.
 ************************************************************************/

/* no conditional instructions supported other than branches */
#undef  CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS
#undef  CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS
#undef  CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS

/* ALU instructions can't set condition codes (there are no condition codes) */
#undef CVMCPU_HAS_ALU_SETCC

/* mips doesn't have an integer mul by immediate (mulli) instruction */
#undef CVMCPU_HAS_IMUL_IMMEDIATE

/* MIPS does not have a postincrement store */
#undef CVMCPU_HAS_POSTINCREMENT_STORE

/* Maximum offset (+/-) for a load/store word instruction. */
#define CVMCPU_MAX_LOADSTORE_OFFSET  (32*1024-1)

/* Number of instructions reserved for setting up constant pool base
 * register in method prologue */
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 3
#endif

/* Number of nop's needed for GC patching. We need two because we need
 * to make sure the delay slot does not get executed when the instruction
 * is patched to make the gc rendezvous call. */
#define CVMCPU_NUM_NOPS_FOR_GC_PATCH 2

#endif /* _INCLUDED_MIPS_JITRISC_CPU_H */
