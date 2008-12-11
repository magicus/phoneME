/*
 * @(#)jit_cpu.h	1.23 06/10/10
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
 */

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#ifndef _INCLUDED_ARM_JIT_CPU_H
#define _INCLUDED_ARM_JIT_CPU_H

#include "javavm/include/iai_opt_config.h"

/*
 * This file #defines all of the macros that shared parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * arm specific parts of the jit, including those with the
 * CVMARM_ prefix.
 */

/* Number of 32-bit words per register (must be 1 in current implementation) */
#define CVMCPU_MAX_REG_SIZE	1

/* Number of 32-bit words per FP register */
#ifdef CVM_JIT_USE_FP_HARDWARE
#define CVMCPU_FP_MAX_REG_SIZE 	1
#endif

/* No 64-bit registers */
#undef CVMCPU_HAS_64BIT_REGISTERS

/*
 * #define CVMCPU_HAS_CP_REG if you want a constant pool for large
 * constants rather then generating them inline.
 *
 * NOTE: On arm we normally do pc-relative constant pool accesses, but
 * implemented support for a constant pool base register for 
 * demonstration purposes only. You can enable this define to see it
 * in action, but will get better performance without it.
 */
/*#define CVMCPU_HAS_CP_REG*/

/*
 * #define CVMCPU_HAS_VOLATILE_GC_REG if you are using trap-based GC
 * checks (CVMJIT_TRAP_BASED_GC_CHECKS), and you don't want to use a
 * dedicated register for storing CVMglobals.jit.gcTrapAddr. This will
 * result in slightly slower loops because CVMglobals.jit.gcTrapAddr
 * will need to be loaded from memory each time. However, it also
 * frees up an extra register for general purpose use, so it should be
 * enabled on platforms with fewer registers.
 */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_HAS_VOLATILE_GC_REG
#endif

/*
 * If the platform #defined CVMCPU_ZERO_REG register that always contains
 * constant zero.
 */
#undef	CVMCPU_HAS_ZERO_REG

/* If the platform has branch delay slot. */
#undef	CVMCPU_HAS_DELAY_SLOT

/* #define CVMCPU_HAS_COMPARE if the platform supports compare
 * instructions and condition codes.
 */
#define CVMCPU_HAS_COMPARE

/* This platform will support intrinsics: */
#define CVMJIT_INTRINSICS
#define CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS

/* Max number of args registers: */
#define CVMCPU_MAX_ARG_REGS     4

/* This platform has support for args beyond the max number of arg registers:*/
#define CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS

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
/*
 * CVMJITintrinsicsList: define this value to be the name of the CPU
 * specific intrinsics config list if one if available.  If the OS/platform
 * does not define it, then the CVMJITarmIntrinsicsList will be used.
 */
#ifndef CVMJITintrinsicsList
#define CVMJITintrinsicsList CVMJITarmIntrinsicsList
#endif

extern const CVMJITIntrinsicConfigList CVMJITarmIntrinsicsList;


/*
 * CVMJITriscParentIntrinsicsList: Leave this value alone because the
 * OS/platform may wish to override it.
 */
/* #undef CVMJITriscParentIntrinsicsList */
#endif /* _ASM */
#endif /* CVM_JIT_CCM_USE_C_HELPER */
#endif /* CVMJIT_INTRINSICS */

#endif /* _INCLUDED_ARM_JIT_CPU_H */
