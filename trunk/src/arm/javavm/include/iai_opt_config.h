/*
 * @(#)iai_opt_config.h	1.12 06/10/10
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

#ifndef __IAI_OPT_CONFIG__
#define __IAI_OPT_CONFIG__

#ifdef CVM_IAI_OPT_ALL

#ifdef __IWMMXT__
#define CVM_ARM_HAS_WMMX
#endif

/* IAI-01 */
/*
 * Optimize memmove_8bit by using WMMX instructions
 */
#ifdef CVM_ARM_HAS_WMMX
#define IAI_MEMMOVE
#define IAI_MEMMOVE_PLD

/* preload the array items when memove  */
#define IAI_MEMMOVE_PLD2
#endif

/* IAI-02 */
/*
 * Improve constant folding for iadd32 and isub32. 
 */
#define IAI_IMPROVED_CONSTANT_ENCODING

/* IAI-02 */
/*
 * Optimize idiv/irem by constant. Note that this option is enabled
 * regardless of the setting of IAI_COMBINED_SHIFT.
 */
#define IAI_COMBINED_SHIFT

/* IAI-03 */
/*
 * Pass class instanceSize and accessFlags to New Glue
 */
#define IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT

/* IAI-04 */
/*
 * Keep &CVMglobals and &CVMglobals.objGlobalMicroLock
 * in WMMX registers
 */
#ifdef CVM_ARM_HAS_WMMX
#define IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
#endif

/* IAI-05 */
/*
 * Reimplement String.indexOf intrinsic function in assembly code
 *
 * NOTE: These assembler intrinsics need to be disabled if the GC
 * has a char read barrier. At the moment none of the CDC-HI GC's have
 * a char read barrier. There is an assert in CVMJITinitCompilerBackEnd
 * in case one is ever added.
 */
#define IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY

/* IAI-06 */
/*
 * Faster version of CVMCCMruntimeIDiv. Uses CLZ. Needs ARM5 or later.
 * You can add more #defines to this list as newer ones become available.
 */
#if defined(__ARM_ARCH_5__)  || defined(__ARM_ARCH_5T__) || \
    defined(__ARM_ARCH_5E__) || defined(__ARM_ARCH_5TE__) || \
    defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6ZK__) || \
    defined(__ARM_ARCH_6K__)
#define IAI_IDIV
#endif

/* IAI-06 */
/*
 * Optimize CVMCCMruntimeDMul and CVMCCMruntimeDADD by using WMMX instructions.
 */
#ifdef CVM_ARM_HAS_WMMX
#define IAI_DMUL
#define IAI_DADD
#endif

/* IAI-08 */
#ifdef CVM_JIT_CODE_SCHED
#define IAI_CODE_SCHEDULER_SCORE_BOARD
#endif

/* IAI-09 */
#ifdef CVM_JIT_CODE_SCHED
#define IAI_ROUNDROBIN_REGISTER_ALLOCATION
#endif

/* IAI-10 */
#if 0  /* NOTE: currently disabled by default */
#define IAI_ARRAY_INIT_BOUNDS_CHECK_ELIMINATION
#endif

/* IAI-11 */
/*
 * IAI_CACHEDCONSTANT: Move the guessCB for checkcast and instanceof into
 *     the runtime constant pool.
 * IAI_CACHEDCONSTANT_INLINING: Inline fast path part of
 *     CVMCCMruntimeCheckCastGlue and CVMCCMruntimeInstanceOfGlue, thus
 *     avoiding call to glue with guessCB is correct. 
 * NOTE: currently IAI_CACHEDCONSTANT_INLINING requires IAI_CACHEDCONSTANT.
 */
#if 0  /* NOTE: currently disabled by default */
#define IAI_CACHEDCONSTANT
#define IAI_CACHEDCONSTANT_INLINING
#endif

/* IAI - 12 */
/*
 * Release the dependency between branch instruction to vm
 * exception handler and the following instructions. 
 * That means any instruction that won't raise exception can 
 * be scheduled before the instruction that may raise exception, 
 * if the other dependencies are satisfied.
 * This modification is an enhancement for code scheduling.
 */
#ifdef CVM_JIT_CODE_SCHED
#define IAI_CS_EXCEPTION_ENHANCEMENT
#define IAI_CS_EXCEPTION_ENHANCEMENT2
#endif

/* IAI - 13 */
/* Enabled WMMX optimization for GC copiers. */
#ifdef IAI_MEMMOVE
#define IAI_GC_WMMX_MEMCOPY 
#endif

/* IAI - 14: #define IAI_FIXUP_FRAME_VMFUNC_INLINING
 * Inline CVMJITfixupFrames into FIXUP_FRAMES_N. By default this is
 * disabled for ARM. IAI_FIXUP_FRAME_VMFUNC_INLINING is only enabled 
 * for bulverde and monahans ports.
 */

/* IAI - 15 */
/*
 * The modification is in the java library.
 * Replace System.arraycopy with CVM.copy(*)Array.
 * As the CVM.copy(*)Array doesn't check array properties, some code 
 * are added in the jave library to check specific exception.
 */

/* IAI-16 */
/*
 * Optimized CVMCCMruntimeFMul & CVMCCMruntimeFAdd
 */
#ifdef CVM_ARM_HAS_WMMX
#define IAI_WMMX_FMUL
#define IAI_WMMX_FADD
#endif

/* IAI - 17 */
/*
 * Avoid calling arraycopy for 0 length array in StringBuffer and Vector.
 */

/* IAI - 18 */
/* Optimize the InputStreamReader.read([C, I, I) method. Convert the 
 * default ISO8859_1 encoding directly. 
 */


/* IAI - 19*/
/* 
 * Preload the memory to be allocated for the next objects 
 * into the cache in CCM allocators.
 *
 * PLD is only supported is only supported on ARM v5TE and later.
 */
#if defined(__ARM_ARCH_5TE__)  || \
    defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6ZK__) || \
    defined(__ARM_ARCH_6K__)
#define IAI_PRELOAD_NEW_OBJECT
#endif

/* IAI-20 */
/*
 * For inlined virtual method invocation, use class guard test to 
 * replace the original method guard test; move method guard test
 * into out-of-line block to guard those failed in class guard test. 
 * By default this is enabled for all platform
 * (see src/share/javavm/include/porting/jit/jit.h).
 *
 * #define IAI_VIRTUAL_INLINE_CB_TEST
 */


/* IAI - 21 */
/*
 * Do quick CB check against the current object CB and super CBs in
 * CheckCast and InstanceOf.
 */
#define IAI_FAST_ASSIGNABLE_CHECKING

/* IAI - 22 */
/*
 * Implemented passing of intrinsic arguments using non-volatile registers
 * when volatile registers are exhausted.
 */

/* IAI - 23 */
/*
 * In the ASM intrinsic for System.arraycopy(), implemented fast verification
 * that srcIdx, dstIdx, and length arguments are >= 0.
 */


#endif /* CVM_IAI_OPT_ALL */

#endif
