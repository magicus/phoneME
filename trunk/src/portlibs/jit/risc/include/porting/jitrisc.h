/*
 * @(#)jitrisc.h	1.13 06/10/10
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

#ifndef _INCLUDED_JITRISC_H
#define _INCLUDED_JITRISC_H

/*
 * This is part of the porting layer that cpu specific code must 
 * implement for the shared RISC code.
 *
 */

/*
 * This file #defines all of the symbols that shared RISC parts of the JIT
 * use in a platform-indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * platform-specific parts of the JIT, including those with a prefix such
 * as CVMARM_ or in general CVM<platform>_.
 */

/************************************************
 * Register Definitions.
 ************************************************/

/* Some register numbers known to regman and codegen.jcs
 * #define CVMCPU_SP_REG      the C top-of-stack point
 * #define CVMCPU_JFP_REG     the Java frame pointer
 * #define CVMCPU_JSP_REG     the Java top-of-stack pointer
 * #define CVMCPU_PROLOGUE_PREVFRAME_REG 
 *		      non-volatile register to hold old JFP during prologue
 * #define CVMCPU_PROLOGUE_NEWJFP_REG 
 *                    non-volatile register to hold new JFP during prologue
 * #define CVMCPU_INVALID_REG (-1)
 */

/* 
 * More register numbers known to regman and codegen.jcs. These are
 * optional. If you have registers to spare then defining these macros
 * allows for more efficient code generation. The registers must
 * be setup by CVMJITgoNative() when compiled code is first entered.
 *
 * #define CVMCPU_CHUNKEND_REG pointer to end of current Java stack chunk
 * #define CVMCPU_CVMGLOBALS_REG pointer to CVMGlobals data
 * #define CVMCPU_EE_REG	pointer to current CVMExecEnv
 * #ifdef CVMCPU_HAS_CP_REG
 * #define CVMCPU_CP_REG     pointer to the constant pool
 *			(especially valuable if you cannot do PC-relative loads)
 * #endif
 * #ifdef CVMCPU_HAS_ZERO_REG
 * #define CVMCPU_ZERO_REG        some RISCs have a dedicated zero register
 * #endif
 */

/* registers for C helper arguments */
/* It is assumed that, on a RISC platform, at least the
 * first 4 words of arguments to helper C functions can
 * be passed by way of processor (integer) registers.
 * These are the register designators used:
 * #define CVMCPU_ARG1_REG
 * #define CVMCPU_ARG2_REG
 * #define CVMCPU_ARG3_REG
 * #define CVMCPU_ARG4_REG
 */

/* registers for C helper result */
/* It is assumed that a 1- or2-word result from a C helper
 * will be returned in registers, as designated here:
 * #define CVMCPU_RESULT1_REG
 * #define CVMCPU_RESULT2_REG
 */

/* 
 * The compiler attempts to place temporary values flowing between blocks
 * in registers. These are called PHI registers.
 * These are the numbers of the first and max number of registers 
 * dedicated for PHI spills. Usually the
 * first PHI is at the bottom of the range of available non-volatile registers.
 */
/* #define CVMCPU_FIRST_PHI_SPILL_REG	 first phi register number
 * #define CVMCPU_MAX_PHI_SPILLS_IN_REGS max number of phi registers
 */

/* range of registers that regman should look at */
/* no point in wasting time on registers that will never be available
 * for allocation.
 * #define CVMCPU_MIN_INTERESTING_REG
 * #define CVMCPU_MAX_INTERESTING_REG
 */

/*******************************************************
 * Register Sets
 *******************************************************/

/* The set of all registers */
/* #define CVMCPU_ALL_SET  a bit set */

/*
 * Registers that regman should never allocate. Only list registers
 * that haven't already been exported to regman. For example, there's
 * no need to tell regman about CVMCPU_SP_REG because regman already
 * knows that it is busy.
 *
 * You may also need a teporary scratch register during for use 
 * during code emission, but which is never allocated by regman. 
 * This would also be part of the busy set.
 *
 * #define CVMCPU_BUSY_SET a bit set
 */

/*
 * The set of all non-volatile registers according to C calling conventions
 * There's no need to put CVMCPU_BUSY_SET registers in this set.
 *
 * #define CVMCPU_NON_VOLATILE_SET   a bit set
 */

/* The set of all volatile registers according to C calling conventions
 * #define CVMCPU_VOLATILE_SET a bit set
 */

/*
 * The set of registers that regman should only use as a last resort.
 *
 * Usually this includes the set of phi registers, but if you have a limited
 * number of non-phi non-volatile registers available, then at most only
 * include the first couple of phi registers.
 *
 * It also should include the argument registers, since we would rather
 * that other non-volatile registers get allocated before these.
 * For example:
 * #define CVMCPU_AVOID_SET (			\
 *     * argument registers we make use of *	\
 *     1U<<CVMCPU_ARG1_REG | 1U<<CVMCPU_ARG2_REG |	\
 *     1U<<CVMCPU_ARG3_REG | 1U<<CVMCPU_ARG4_REG |	\
 *     * phi registers *				\
 *     1U<<(CVMCPU_FIRST_PHI_SPILL_REG) |		\
 *     1U<<(CVMCPU_FIRST_PHI_SPILL_REG+1))
 */

/*
 * Set of registers that can be used for the jsr return address. If the LR
 * (link register or return-address register) is a general purpose register,
 * then just set it to the LR reg. Otherwise usually it is best
 * to allow any register to be used.
 *
 * #define CVMCPU_JSR_RETURN_ADDRESS_SET
 */

/*
 * Alignment parameters of integer registers.
 * All quantities in 32-bit words.
 * 1 = any alignment, 2 = even-register aligned
 *
 * #define CVMCPU_SINGLE_REG_ALIGNMENT
 * #define CVMCPU_DOUBLE_REG_ALIGNMENT	
 */

/*
 * In case we opt for the FPU, we'll need these parameters too
 */
#ifdef CVM_JIT_USE_FP_HARDWARE

/*
 * These regman parameters are the floating register
 * analogies to all the above integer register parameters
 *
 * #define CVMCPU_FP_MIN_INTERESTING_REG  the range of registers...
 * #define CVMCPU_FP_MAX_INTERESTING_REG  ... interesting to regman
 * #define CVMCPU_FP_BUSY_SET 		  the set of busy register
 * #define CVMCPU_FP_ALL_SET 0xfffffff	  the set of all register
 * #define CVMCPU_FP_NON_VOLATILE_SET the set of registers safe across a call
 * #define CVMCPU_FP_VOLATILE_SET     the set of registers unsafe across a call
 * #define CVMCPU_FP_FIRST_PHI_SPILL_REG    the first FP-PHI register
 * #define CVMCPU_FP_MAX_PHI_SPILLS_IN_REGS the number of FP-PHI register
 */

/*
 * Alignment parameters of floating registers
 * All quantities in 32-bit words
 * 1 = any alignment, 2 = even-register aligned
 *
 * #define CVMCPU_FP_SINGLE_REG_ALIGNMENT
 * #define CVMCPU_FP_DOUBLE_REG_ALIGNMENT
 */

#endif

/************************************************************************
 * CPU features - These macros define various features of the processor.
 ************************************************************************/

/*
 * All platforms have conditional branches.
 * Some have more. The ARM is the best example of a platform
 * that can support all of the following:
 *
 * #define CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS
 * #define CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS
 * #define CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS
 */

/* Define this if ALU instructions can set condition codes
 * #define CVMCPU_HAS_ALU_SETCC
 */

/* Define this if the platform has an integer mul by immediate instruction
 * #define CVMCPU_HAS_IMUL_IMMEDIATE
 */

/* Define this if the platform has a postincrement addressing mode for stores
 *
 * #define CVMCPU_HAS_POSTINCREMENT_STORE
 */

/* Maximum offset (+/-) for a load/store word instruction.
 * #define CVMCPU_MAX_LOADSTORE_OFFSET  (4*1024-1)
 */

/* Number of instructions reserved for setting up constant pool base
 * register in method prologue.
 * #ifdef CVMCPU_HAS_CP_REG
 * #define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 3
 * #endif
 */

/* If defined, this gives the number of nop's needed at GC check patch points,
 * which is normally the case on processors with a delay slot after a 
 * branch-and-link instruction. Leave it #undef if no nop's are needed.
 *
 * #ifdef CVMJIT_PATCH_BASED_GC_CHECKS
 * #define CVMCPU_NUM_OF_INSTRUCTIONS_FOR_GC_PATCH 1
 * #endif
 */

#include "javavm/include/jit/jitrisc_cpu.h"

/* The platform should define CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM
 * to be the number of scratch registers it needs to implement the out of line checks
 * for an inlined virtual invoke.  If the platform does not explicitly
 * specify the number of scratch registers it needs for this, the RISC layer
 * will assign a default number below.
 */
#ifndef CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM
#define CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM   2 /* default */
#endif

#endif /* _INCLUDED_JITRISC_H */
