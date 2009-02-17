/*
 * @(#)jit_cpu.h	1.9 06/10/23
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

#ifndef _INCLUDED_X86_JIT_CPU_H
#define _INCLUDED_X86_JIT_CPU_H

/*
 * This file #defines all of the macros that shared parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * arm specific parts of the jit, including those with the
 * CVMSPARC_ prefix.
 */

#ifndef _ASM
struct CVMCPUGlobalState {
      int dummy;
#ifdef CVM_JIT_USE_FP_HARDWARE
/* SVMC_JIT d022609 (ML). x87 FPU settings. */
      CVMInt16 FPModeNearSgl /* FPU control word */;
      CVMInt16 FPModeNearDbl /* FPU control word */;
      CVMInt16 FPModeTrncDbl /* FPU control word */;
      CVMInt32 FPFactorSmall[3] /* force IEEE-754 single/double arithmetic */;
      CVMInt32 FPFactorLarge[3] /* force IEEE-754 single/double arithmetic */;
#endif
};
CVM_STRUCT_TYPEDEF(CVMCPUGlobalState);
#endif /* #ifndef _ASM */

/* Number of 32-bit words per register (must be 1 in current implementation) */
#define CVMCPU_MAX_REG_SIZE	1

/* 64 bit FP values (2 words) fit into a single FP register on IA-32. */
#ifdef CVM_JIT_USE_FP_HARDWARE
#define CVMCPU_FP_MAX_REG_SIZE 	2
#endif

/* SVMC_JIT d022609 (ML). no generic 64 bit regs on x86. */
#undef CVMCPU_HAS_64BIT_REGISTERS

/*
 * #define CVMCPU_HAS_CP_REG if you want a constant pool for large
 * constants rather than generating them inline.
 *
 * On X86 CP_REG is only used when loading floating point numbers.
 */
#ifdef	CVM_JIT_USE_FP_HARDWARE
#define CVMCPU_HAS_CP_REG
#else
#undef	CVMCPU_HAS_CP_REG
#endif

/*
 * #define CVMCPU_HAS_VOLATILE_GC_REG if you are using trap-based GC
 * checks (CVMJIT_TRAP_BASED_GC_CHECKS), and you don't want to use a
 * dedicated register for storing CVMglobals.jit.gcTrapAddr. This will
 * result in slightly slower loops because CVMglobals.jit.gcTrapAddr
 * will need to be loaded from memory each time. However, it also
 * frees up an extra register for general purpose use, so it should be
 * enabled on platforms with fewer registers.
 *
 */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_HAS_VOLATILE_GC_REG
#endif

/* 
 * If the platform #defined CVMCPU_ZERO_REG register that always contains
 * constant zero.
 */
#undef CVMCPU_HAS_ZERO_REG

/* If the platform has branch delay slot. */
#undef CVMCPU_HAS_DELAY_SLOT

/* #define CVMCPU_HAS_COMPARE if the platform supports compare
 * instructions and condition codes.
 */
#define CVMCPU_HAS_COMPARE

/* This platform will support intrinsics: */
/* FIXME: enable intrinsics support after adding 
 * CVMJITRegsRequiredType definition for x86.
 */
#define CVMJIT_INTRINSICS

/*
 * Max number of args registers: actually on x86 parameters are not
 * passed in registers, but there are a lot of locations in the shared
 * code section, where it is assumed, that argument registers
 * exist. As we don't want to change too much in shared code, we define
 * some registers for parameter passing and right before the call we
 * push them on the stack. See also CVMCPUemitAbsoluteCall_CCode.
 */
#define CVMCPU_MAX_ARG_REGS     4

/* 
 * We use virtual argument registers (see above) and can support
 * more arguments than the number of virtual argument registers
 */
#define CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS

/* the size of one instruction */
/* in general x86 instructions are variably sized. but we can
 * guarantee that instructions that need to be patched will not be
 * longer than 8 bytes and fit into datastructures like
 * CVMCompiledGCCheckPCs.
 */
#define CVMCPU_INSTRUCTION_SIZE 8

/* GC rendezvous is implemented by an unconditional call */
#define CVMCPU_GC_RENDEZVOUS_INSTRUCTION_SIZE 5 

/*
 * The size of call instructions. Used to compute the call address
 * from a given return address.
 */
#define CVMCPU_CALL_SIZE 2

/*
 * Used in jit_cisc.h for defining an assertion in
 * CVMJITmassageCompiledPC.
 */
#define CVMCPU_IS_CALL(addr) ((char)0xff == *(char*)(addr))

#undef CVM_JIT_HAVE_CALLEE_SAVED_REGISTER

/* maximal alignment the target architecture requires */
#define CVMCPU_MAX_MEMORY_ALIGNMENT 4

/****************************************************************
 * Code Size Estimations - The following are all used to estimate
 * how big a compiled method will be.
 ****************************************************************/

/* GC write barrier size, not counting potential spills */
#define CVMCPU_GC_BARRIER_SIZE          40

/* Some misc opcodes which generate abnormally large code segments */
#define CVMCPU_WORD_ARRAY_LOAD_SIZE     8
#define CVMCPU_DWORD_ARRAY_LOAD_SIZE    16
#define CVMCPU_WORD_ARRAY_STORE_SIZE    16
#define CVMCPU_DWORD_ARRAY_STORE_SIZE   16
#define CVMCPU_INVOKEVIRTUAL_SIZE       32
#define CVMCPU_INVOKEINTERFACE_SIZE     24
#define CVMCPU_CHECKCAST_SIZE           24
#define CVMCPU_INSTANCEOF_SIZE          16
#define CVMCPU_RESOLVE_SIZE             64
#define CVMCPU_CHECKINIT_SIZE           48

/* To avoid buffer overflows for short methods */
#define CVMCPU_INITIAL_SIZE             64

/* Virtual inlining overhead size including phi handling */
#define CVMCPU_VIRTINLINE_SIZE	        96

/* Size of method entry and exit code */
#define CVMCPU_ENTRY_EXIT_SIZE          24

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

/* Switches off the caching of field references. */
#define CVMCPUfieldref32CachingOff

/*
 * The number of patch points in ccm code
 */
#define CVMCPU_NUM_CCM_PATCH_POINTS   0

/* SVMC_JIT c5041613 (dk) 2003-11-07. Array length caching may cause
   performance drops in CISC architectures. Experimental evaluation
   not finished yet, so not activated right now. */
#define CVMJIT_NO_ARRAYLENGTH_CACHE


/* FIXME: #define in header file instead of changing the make file.
 * FIXME: to enable GC patching.
 */
#define CVM_JIT_GENERATE_GC_RENDEZVOUS

#endif /* _INCLUDED_X86_JIT_CPU_H */
