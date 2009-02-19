/*
 * @(#)jitrisc_cpu.h	1.40 06/10/10
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

#ifndef _INCLUDED_POWERPC_JITRISC_CPU_H
#define _INCLUDED_POWERPC_JITRISC_CPU_H

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * This file #defines all of the macros that shared risc parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * powerpc specific parts of the jit, including those with the
 * CVMPPC_ prefix.
 */

/************************************************
 * Register Definitions.
 ************************************************/

/* all of the general purpose registers (32 of 'em) */
#define CVMPPC_r0  0
#define CVMPPC_r1  1
#define CVMPPC_r2  2
#define CVMPPC_r3  3
#define CVMPPC_r4  4
#define CVMPPC_r5  5
#define CVMPPC_r6  6
#define CVMPPC_r7  7
#define CVMPPC_r8  8
#define CVMPPC_r9  9
#define CVMPPC_r10 10
#define CVMPPC_r11 11
#define CVMPPC_r12 12
#define CVMPPC_r13 13
#define CVMPPC_r14 14
#define CVMPPC_r15 15
#define CVMPPC_r16 16
#define CVMPPC_r17 17
#define CVMPPC_r18 18
#define CVMPPC_r19 19
#define CVMPPC_r20 20
#define CVMPPC_r21 21
#define CVMPPC_r22 22
#define CVMPPC_r23 23
#define CVMPPC_r24 24
#define CVMPPC_r25 25
#define CVMPPC_r26 26
#define CVMPPC_r27 27
#define CVMPPC_r28 28
#define CVMPPC_r29 29
#define CVMPPC_r30 30
#define CVMPPC_r31 31

#define CVMPPC_sp  CVMPPC_r1

/*
 * Macro to map a register name (like R31) to a register number (like 31).
 * Because of the strange way the preprocessor expands macros, we need
 * an extra indirection of macro invocations to get it expanded correctly.
 */
#define CVMCPU_MAP_REGNAME_0(regname) CVMPPC_##regname
#define CVMCPU_MAP_REGNAME(regname)   CVMCPU_MAP_REGNAME_0(regname)

/* Some registers known to regman and codegen.jcs */
#define CVMCPU_SP_REG           CVMPPC_sp
#define CVMCPU_JFP_REG     	CVMCPU_MAP_REGNAME(CVMPPC_JFP_REGNAME)
#define CVMCPU_JSP_REG     	CVMCPU_MAP_REGNAME(CVMPPC_JSP_REGNAME)
#define CVMCPU_PROLOGUE_PREVFRAME_REG \
    				CVMCPU_MAP_REGNAME(CVMPPC_PREVFRAME_REGNAME)
#define CVMCPU_PROLOGUE_NEWJFP_REG \
    				CVMCPU_MAP_REGNAME(CVMPPC_NEWJFP_REGNAME)
#define CVMCPU_INVALID_REG (-1)

/* 
 * More registers known to regman and codegen.jcs. These ones are
 * optional. If you have registers to spare then defining these macros
 * allows for more efficient code generation. The registers will
 * be setup by CVMJITgoNative() when compiled code is first entered.
 */
#define CVMCPU_CHUNKEND_REG    CVMCPU_MAP_REGNAME(CVMPPC_CHUNKEND_REGNAME)
#define CVMCPU_CVMGLOBALS_REG  CVMCPU_MAP_REGNAME(CVMPPC_CVMGLOBALS_REGNAME)
#define CVMCPU_EE_REG          CVMCPU_MAP_REGNAME(CVMPPC_EE_REGNAME)
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_CP_REG          CVMCPU_MAP_REGNAME(CVMPPC_CP_REGNAME)
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GC_REG          CVMCPU_MAP_REGNAME(CVMPPC_GC_REGNAME)
#endif

/* registers for C helper arguments */
#define CVMCPU_ARG1_REG    CVMPPC_r3
#define CVMCPU_ARG2_REG    CVMPPC_r4
#define CVMCPU_ARG3_REG    CVMPPC_r5
#define CVMCPU_ARG4_REG    CVMPPC_r6

/* registers for C helper result */
#define CVMCPU_RESULT1_REG   CVMPPC_r3
#define CVMCPU_RESULT2_REG   CVMPPC_r4

/* 
 * The set of registers dedicated for PHI spills. Usually the
 * first PHI is at the bottom of the range of available non-volatile registers.
 */
#define CVMCPU_PHI_REG_SET (			\
    1U << CVMPPC_r14 | 1U << CVMPPC_r15 |	\
    1U << CVMPPC_r16 | 1U << CVMPPC_r17 |	\
    1U << CVMPPC_r18 | 1U << CVMPPC_r19	|	\
    1U << CVMPPC_r20 | 1U << CVMPPC_r21 |	\
    1U << CVMPPC_r22 | 1U << CVMPPC_r23 |	\
    1U << CVMPPC_r24				\
)

/* range of registers that regman should look at */
#define CVMCPU_MIN_INTERESTING_REG      CVMPPC_r3
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_MAX_INTERESTING_REG      CVMPPC_r25
#else
#define CVMCPU_MAX_INTERESTING_REG      CVMPPC_r26
#endif

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
#define CVMCPU_BUSY_SET (						    \
    1U<<CVMPPC_r0 | /* avoid confusion with R0 sometimes meaning #0 */	    \
    1U<<CVMPPC_r2 | 1U<<CVMPPC_r13) /* don't clobber the globals pointers */

/*
 * The set of all non-volatile registers according to C calling conventions.
 * There's no need to put CVMCPU_BUSY_SET registers in this set.
 */
#define CVMCPU_NON_VOLATILE_SET (  /* R14-R31 */			 \
    1U<<CVMPPC_r14 | 1U<<CVMPPC_r15 | 1U<<CVMPPC_r16 |			 \
    1U<<CVMPPC_r17 | 1U<<CVMPPC_r18 | 1U<<CVMPPC_r19 | 1U<<CVMPPC_r20 | \
    1U<<CVMPPC_r21 | 1U<<CVMPPC_r22 | 1U<<CVMPPC_r23 | 1U<<CVMPPC_r24 | \
    1U<<CVMPPC_r25 | 1U<<CVMPPC_r26 | 1U<<CVMPPC_r27 | 1U<<CVMPPC_r28 | \
    1U<<CVMPPC_r29 | 1U<<CVMPPC_r30 | 1U<<CVMPPC_r31)

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
#define CVMCPU_AVOID_SET (			\
    /* argument registers we make use of */	\
    1U<<CVMCPU_ARG1_REG | 1U<<CVMCPU_ARG2_REG |	\
    1U<<CVMCPU_ARG3_REG | 1U<<CVMCPU_ARG4_REG |	\
    /* phi registers */				\
    1U << CVMPPC_r14 | 1U << CVMPPC_r15 |	\
    1U << CVMPPC_r16 | 1U << CVMPPC_r17 |	\
    1U << CVMPPC_r18 | 1U << CVMPPC_r19	|	\
    0/*CVMCPU_PHI_REG_SET*/)

/*
 * Set of registers that can be used for the jsr return address. If the LR
 * is a GPR, then just set it to the LR reg. Otherwise usually it is best
 * to allow any register to be used.
 */
#define CVMCPU_JSR_RETURN_ADDRESS_SET CVMCPU_ALL_SET

/*
 * Alignment parameters of integer registers are trivial
 * All quantities in 32-bit words
 */
#define CVMCPU_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_DOUBLE_REG_ALIGNMENT	1

/*
 * In case we opt for the FPU, we'll need these parameters too
 */
#ifdef CVM_JIT_USE_FP_HARDWARE

/* FP register sets */
#define CVMCPU_FP_MIN_INTERESTING_REG	0
#define CVMCPU_FP_MAX_INTERESTING_REG	21
#define CVMCPU_FP_BUSY_SET 0
/* we'll by using f0-f21 */
#define CVMCPU_FP_ALL_SET 0x003fffff
/* f14-f31 are non-volatile. However, in order to save overhead
 * in CVMJITgoNative, we only ever use 8: f14-f21 */
#define CVMCPU_FP_NON_VOLATILE_SET 0x003fc000
/* f0-f13 are volatile */
#define CVMCPU_FP_VOLATILE_SET 0x00003fff

#define CVMCPU_FP_PHI_REG_SET (			\
    1U << 14 | 1U << 15 |			\
    1U << 16 | 1U << 17 |			\
    1U << 18 | 1U << 19 |			\
    1U << 20 | 1U << 21				\
)

/*
 * Alignment parameters of floating registers
 * All quantities in 64-bit words
 */
#define CVMCPU_FP_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_FP_DOUBLE_REG_ALIGNMENT	1

#endif

/************************************************************************
 * CPU features - These macros define various features of the processor.
 ************************************************************************/

/* no conditional instructions supported other than branches and calls */
#undef  CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS
#undef  CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS
#define CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS

/* ALU instructions can set condition codes */
#define CVMCPU_HAS_ALU_SETCC

/* PowerPC has an integer mul by immediate (mulli) instruction */
#define CVMCPU_HAS_IMUL_IMMEDIATE

/* PowerPC does not have a postincrement store */
#undef CVMCPU_HAS_POSTINCREMENT_STORE

/* Maximum offset (+/-) for a load/store word instruction. */
#define CVMCPU_MAX_LOADSTORE_OFFSET  (32*1024-1)

/* Number of instructions reserved for setting up constant pool base
 * register in method prologue */
#ifdef CVMCPU_HAS_CP_REG
#ifdef CVM_AOT
#define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 6
#else
#define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 3
#endif
#endif

/* PowerPC normally doesn't need any nop's emitted at GC check patch points
 * because normally it can make the call to CVMCCMruntimeGCRendezvousGlue
 * in one instruction. However, it might not be able to if we aren't
 * copying to the code cache, so we emit an extra nop and allow
 * CVMCCMruntimeGCRendezvousGlue to return after the nop.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#undef CVMCPU_NUM_NOPS_FOR_GC_PATCH
#else
#define CVMCPU_NUM_NOPS_FOR_GC_PATCH 1
#endif
#endif

#endif /* _INCLUDED_POWERPC_JITRISC_CPU_H */
