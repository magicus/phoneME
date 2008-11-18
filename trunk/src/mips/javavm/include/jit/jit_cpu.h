/*
 * @(#)jit_cpu.h	1.19 06/10/10
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

#ifndef _INCLUDED_MIPS_JIT_CPU_H
#define _INCLUDED_MIPS_JIT_CPU_H

#ifdef USE_CDC_COM
#include "javavm/include/iai_opt_config.h"
#endif

/*
 * This file #defines all of the macros that shared parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * arm specific parts of the jit, including those with the
 * CVMMIPS_ prefix.
 */

/* Number of 32-bit words per register (must be 1 in current implementation) */
#define CVMCPU_MAX_REG_SIZE	1
					  
/* Number of 32-bit words per FP register */
#ifdef CVM_JIT_USE_FP_HARDWARE
#ifndef CVMMIPS_HAS_64BIT_FP_REGISTERS
#define CVMCPU_FP_MAX_REG_SIZE 		1
#else
#define CVMCPU_FP_MAX_REG_SIZE 		2
#endif
#endif

#ifdef CVMMIPS_HAS_64BIT_REGISTERS
#define CVMCPU_HAS_64BIT_REGISTERS
#else
#undef  CVMCPU_HAS_64BIT_REGISTERS
#endif

/*
 * #define CVMCPU_HAS_CP_REG if you want a constant pool for large
 * constants rather then generating them inline. We don't support this
 * when using CVMJIT_TRAP_BASED_GC_CHECKS, because it makes us
 * a bit short of registers to spare one for the CP reg.
 */
#ifndef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_HAS_CP_REG
#endif

/* If the platform has branch delay slot. */
#define	CVMCPU_HAS_DELAY_SLOT

/* #define CVMCPU_HAS_COMPARE if the platform supports compare
 * instructions and condition codes.
 */
#undef CVMCPU_HAS_COMPARE

/* 
 * If the platform #defined CVMCPU_ZERO_REG register that always contains
 * constant zero.
 */
#define CVMCPU_HAS_ZERO_REG

/* This platform will support intrinsics: */
#define CVMJIT_INTRINSICS

/* This platform supports custom intrinsics with register arguments
   in non-std registers. For o32, this means supporting args 5 to 8
   in t0 to t3 just like n32 does. */
#define CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS

/* Max number of args registers. */
#define CVMCPU_MAX_ARG_REGS CVMMIPS_MAX_ARG_REGS

/* o32 calling conventions only use 4 arg registers. n32 and n64 use 8
 * arg registers. If only 4 args are supported, then we support up to 8
 * total args by storing the extra args on the C stack per normal
 * o32 calling conventions.
 * See CVMMIPSCCALLgetRequired.
 */
#if CVMCPU_MAX_ARG_REGS != 8
#define CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS
#endif

/* the size of one instruction */
#define CVMCPU_INSTRUCTION_SIZE 4


/****************************************************************
 * Code Size Estimations - The following are all used to estimate
 * how big a compiled method will be.
 ****************************************************************/

/* GC write barrier size, not counting potential spills */
#define CVMCPU_GC_BARRIER_SIZE          16

/* Some misc opcodes which generate abnormally large code segments */
#define CVMCPU_WORD_ARRAY_LOAD_SIZE     8
#define CVMCPU_DWORD_ARRAY_LOAD_SIZE    16
#define CVMCPU_WORD_ARRAY_STORE_SIZE    0
#define CVMCPU_DWORD_ARRAY_STORE_SIZE   8
#define CVMCPU_INVOKEVIRTUAL_SIZE       20
#define CVMCPU_INVOKEINTERFACE_SIZE     28
#define CVMCPU_CHECKCAST_SIZE           60
#define CVMCPU_INSTANCEOF_SIZE          64
#define CVMCPU_RESOLVE_SIZE             64
#define CVMCPU_CHECKINIT_SIZE           52

/* To avoid buffer overflows for short methods */
#define CVMCPU_INITIAL_SIZE             60

/* Virtual inlining overhead size including phi handling */
#define CVMCPU_VIRTINLINE_SIZE	        96

/* Size of method entry and exit code */
#define CVMCPU_ENTRY_EXIT_SIZE          8

/*
 * A ROUGH estimate of how much code will expand from byte-code to
 * native code after taking into account adjustments for all of the
 * sizes defined above.
 */
#define CVMCPU_CODE_EXPANSION_FACTOR  6

/*
 * Estimate the number of instructions it takes to invoke compiled to 
 * compiled. Adjust the value to compensate for stalls and branch overhead.
 * This value is used by profiling code to estimate how much performance
 * would be improved if a method was inlined.
 */
#define CVMCPU_NUM_METHOD_INVOCATION_INSTRUCTIONS     50

/*
 * The number of patch points in ccm code
 */
#define CVMCPU_NUM_CCM_PATCH_POINTS   3

#ifdef CVMJIT_INTRINSICS
#ifndef CVM_JIT_CCM_USE_C_HELPER
#ifndef _ASM
#ifdef USE_CDC_COM
/*
 * CVMJITintrinsicsList: define this value to be the name of the CPU
 * specific intrinsics config list if one if available.  If the OS/platform
 * does not define it, then the CVMJITmipsIntrinsicsList will be used.
 */
#ifndef CVMJITintrinsicsList
#define CVMJITintrinsicsList CVMJITmipsIntrinsicsList
#endif

extern const CVMJITIntrinsicConfigList CVMJITmipsIntrinsicsList;

/*
 * CVMJITriscParentIntrinsicsList: Leave this value alone because the
 * OS/platform may wish to override it.
 */
/* #undef CVMJITriscParentIntrinsicsList */
#endif
#endif /* _ASM */
#endif /* CVM_JIT_CCM_USE_C_HELPER */
#endif /* CVMJIT_INTRINSICS */


#endif /* _INCLUDED_MIPS_JIT_CPU_H */
