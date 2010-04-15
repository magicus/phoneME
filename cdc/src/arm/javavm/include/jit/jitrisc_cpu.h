/*
 * @(#)jitrisc_cpu.h	1.44 06/10/10
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

#ifndef _INCLUDED_ARM_JITRISC_CPU_H
#define _INCLUDED_ARM_JITRISC_CPU_H

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * This file #defines all of the macros that shared risc parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * arm specific parts of the jit, including those with the
 * CVMARM_ prefix.
 */

/************************************************
 * Register Definitions.
 ************************************************/

/* all of the general purpose registers (16 of 'em) */
#define CVMARM_a1  0
#define CVMARM_a2  1
#define CVMARM_a3  2
#define CVMARM_a4  3
#define CVMARM_v1  4
#define CVMARM_v2  5
#define CVMARM_v3  6
#define CVMARM_v4  7
#define CVMARM_v5  8
#define CVMARM_v6  9
#define CVMARM_v7 10
#define CVMARM_v8 11
#define CVMARM_ip 12
#define CVMARM_sp 13
#define CVMARM_lr 14
#define CVMARM_pc 15

#define CVMARM_PC CVMARM_pc
#define CVMARM_LR CVMARM_lr

/*
 * Some platforms may reserve registers that are not normally reserved
 * on ARM platforms. These registers can be added to CVMARM_RESERVED_REGS.
 */
#ifndef CVMARM_RESERVED_REGS
#define CVMARM_RESERVED_REGS 0
#endif

/*
 * Macro to map a register name (like v2) to a register number (like 5).
 * Because of the strange way the preprocessor expands macros, we need
 * an extra indirection of macro invocations to get it expanded correctly.
 */
#define CVMCPU_MAP_REGNAME_0(regname) CVMARM_##regname
#define CVMCPU_MAP_REGNAME(regname)   CVMCPU_MAP_REGNAME_0(regname)

/* Some registers known to regman and codegen.jcs */
#define CVMCPU_SP_REG      CVMARM_sp
#define CVMCPU_JFP_REG     CVMCPU_MAP_REGNAME(CVMARM_JFP_REGNAME)
#define CVMCPU_JSP_REG     CVMCPU_MAP_REGNAME(CVMARM_JSP_REGNAME)
#define CVMCPU_PROLOGUE_PREVFRAME_REG \
    CVMCPU_MAP_REGNAME(CVMARM_PREVFRAME_REGNAME)
#define CVMCPU_PROLOGUE_NEWJFP_REG \
    CVMCPU_MAP_REGNAME(CVMARM_NEWJFP_REGNAME)
#define CVMCPU_INVALID_REG (-1)

/* 
 * More registers known to regman and codegen.jcs. These ones are
 * optional. If you have registers to spare then defining these macros
 * allows for more efficient code generation. The registers will
 * be setup by CVMJITgoNative() when compiled code is first entered.
 */
#undef CVMCPU_CHUNKEND_REG
#undef CVMCPU_CVMGLOBALS_REG
#undef CVMCPU_EE_REG
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_CP_REG     CVMCPU_MAP_REGNAME(CVMARM_CP_REGNAME)
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GC_REG     CVMCPU_MAP_REGNAME(CVMARM_GC_REGNAME)
#endif

/* registers for C helper arguments */
#define CVMCPU_ARG1_REG    CVMARM_a1
#define CVMCPU_ARG2_REG    CVMARM_a2
#define CVMCPU_ARG3_REG    CVMARM_a3
#define CVMCPU_ARG4_REG    CVMARM_a4

/* registers for C helper result */
#define CVMCPU_RESULT1_REG   CVMARM_a1
#define CVMCPU_RESULT2_REG   CVMARM_a2

/* 
 * The set of registers dedicated for PHI spills. Usually the
 * first PHI is at the bottom of the range of available non-volatile registers.
 */
#define ARM_PHI_REG_1	(1U << CVMARM_v3)
#define ARM_PHI_REG_2	(1U << CVMARM_v4)
#define ARM_PHI_REG_3	(1U << CVMARM_v5)
#define ARM_PHI_REG_4	(1U << CVMARM_v6)
#define ARM_PHI_REG_5	(1U << CVMARM_v7)

#if defined(CVMCPU_HAS_CP_REG) || \
    (defined(CVMJIT_TRAP_BASED_GC_CHECKS) && \
     !defined(CVMCPU_HAS_VOLATILE_GC_REG))
#define ARM_PHI_REG_6	0
#else
#define ARM_PHI_REG_6	(1U << CVMARM_v8)
#endif

#define CVMCPU_PHI_REG_SET (                                            \
        (ARM_PHI_REG_1 | ARM_PHI_REG_2 |                                \
	 ARM_PHI_REG_3 | ARM_PHI_REG_4 |                                \
	 ARM_PHI_REG_5 | ARM_PHI_REG_6)	& ~(CVMARM_RESERVED_REGS)       \
)

/* range of registers that regman should look at */
#define CVMCPU_MIN_INTERESTING_REG      CVMARM_a1
#define CVMCPU_MAX_INTERESTING_REG      CVMARM_lr

/*******************************************************
 * Register Sets
 *******************************************************/

/* The set of all registers */
#define CVMCPU_ALL_SET  0x0000ffff

/*
 * Registers that regman should never allocate. Only list registers
 * that haven't already been exported to regman. For example, there's
 * no need to tell regman about CVMCPU_SP_REG because regman already
 * knows that it is busy.
 */
#define CVMCPU_BUSY_SET (1U<<CVMARM_pc | (CVMARM_RESERVED_REGS))

/*
 * The set of all non-volatile registers according to C calling conventions
 * There's no need to put CVMCPU_BUSY_SET registers in this set.
 */
#define CVMCPU_NON_VOLATILE_SET   /* v1-v8 */				\
    (1U<<CVMARM_v1  | 1U<<CVMARM_v2  | 1U<<CVMARM_v3 | 1U<<CVMARM_v4  |	\
     1U<<CVMARM_v5  | 1U<<CVMARM_v6  | 1U<<CVMARM_v7 | 1U<<CVMARM_v8)

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
 * that other non-volatile registers get allocated before these.
 */
#define CVMCPU_AVOID_SET (			\
    /* argument registers we make use of */	\
    1U<<CVMCPU_ARG1_REG | 1U<<CVMCPU_ARG2_REG |	\
    1U<<CVMCPU_ARG3_REG | 1U<<CVMCPU_ARG4_REG |	\
    /* phi registers */				\
    ARM_PHI_REG_1 | ARM_PHI_REG_2)

/*
 * Set of registers that can be used for the jsr return address. If the LR
 * is a GPR, then just set it to the LR reg. Otherwise usually it is best
 * to allow any register to be used.
 */
#define CVMCPU_JSR_RETURN_ADDRESS_SET  (1U<<CVMARM_lr)

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

#define CVMCPU_FP_MIN_INTERESTING_REG	0
#define CVMCPU_FP_MAX_INTERESTING_REG	31
#define CVMCPU_FP_BUSY_SET		0
#define CVMCPU_FP_ALL_SET		0xffffffff
#define CVMCPU_FP_NON_VOLATILE_SET	0
#define CVMCPU_FP_VOLATILE_SET CVMCPU_FP_ALL_SET

#define CVMCPU_FP_PHI_REG_SET (		\
    1U<<0 | 1U<<1 | 1U<<2 | 1U<<3 |	\
    1U<<4 | 1U<<5 | 1U<<6 | 1U<<7	\
)

/*
 * Alignment parameters of floating registers
 * All quantities in 32-bit words
 */
#define CVMCPU_FP_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_FP_DOUBLE_REG_ALIGNMENT	2

/* Maximum offset (+/-) for a vfp load/store word instruction. */
#define CVMARM_FP_MAX_LOADSTORE_OFFSET  (4*256-1)

#endif

/************************************************************************
 * CPU features - These macros define various features of the processor.
 ************************************************************************/

/* the following conditional instructions are all supported */
#define CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS
#define CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS
#define CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS

/* ALU instructions can set condition codes */
#define CVMCPU_HAS_ALU_SETCC

/* Arm doesn't have an integer mul by immediate instruction */
#undef CVMCPU_HAS_IMUL_IMMEDIATE

/* Arm does have a postincrement store */
#define CVMCPU_HAS_POSTINCREMENT_STORE

/* Maximum offset (+/-) for a load/store word instruction. */
#define CVMCPU_MAX_LOADSTORE_OFFSET  (4*1024-1)

/* Number of instructions reserved for setting up constant pool base
 * register in method prologue. */
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 3
#endif

/* ARM normally doesn't need any nop's emitted at GC check patch points
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

#endif /* _INCLUDED_ARM_JITRISC_CPU_H */
