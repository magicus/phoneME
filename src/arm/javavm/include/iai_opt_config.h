/*
 * Portions Copyright 2000-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License version 2 for more details (a copy is included at
 * /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public
 * License version 2 along with this work; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#ifndef __IAI_OPT_CONFIG__
#define __IAI_OPT_CONFIG__

#ifdef CVM_IAI_OPT_ALL

/* IAI-01 */
/*
 * Optimize memmove_8bit by using WMMX instructions
 */
#ifdef CVM_ARM_HAS_WMMX
#define IAI_MEMMOVE
#define IAI_MEMMOVE_PLD
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
 */
#if !defined(__ARM_ARCH_4__) && !defined(__ARM_ARCH_4T__)
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

#endif /* CVM_IAI_OPT_ALL */

#endif
