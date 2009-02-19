/*
 * @(#)jitasmconstants_cpu.h	1.12 06/10/10
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

#ifndef _INCLUDED_JITASMCONSTANTS_CPU_H
#define _INCLUDED_JITASMCONSTANTS_CPU_H

/* jit.h is needed for the CVMCPU_HAS_CP_REG flag */
#include "javavm/include/porting/jit/jit.h"

/* Make sure both CVMCPU_HAS_CP_REG and CVMJIT_TRAP_BASED_GC_CHECKS
 * are not #defined, because we don't have the available registers
 * to support both at the same time.
 */
#if defined(CVMCPU_HAS_CP_REG) && defined(CVMJIT_TRAP_BASED_GC_CHECKS)
#error "MIPS port does not support both CVMCPU_HAS_CP_REG and CVMJIT_TRAP_BASED_GC_CHECKS"
#endif

/*
 * Special registers we use in assembler code that also need to be
 * exported to jitrisc_cpu.h and other C code. See the CVMCPU_MAP_REGNAME
 * macro to see how to map these register names to register numbers.
 */
#define CVMMIPS_JSP_REGNAME		s0
#define CVMMIPS_JFP_REGNAME		s1
#define CVMMIPS_CHUNKEND_REGNAME	s8
#define CVMMIPS_EE_REGNAME		s2
#ifdef CVMCPU_HAS_CP_REG
#define CVMMIPS_CP_REGNAME		s3
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMMIPS_GC_REGNAME		s3
#endif

/* The registers that need to be returned by prologue code */
#define CVMMIPS_PREVFRAME_REGNAME   t8  /* Used for non-sync invocations */
#define CVMMIPS_NEWJFP_REGNAME      a2  /* Used for sync invocations     */

/*
 * Stack Frame Layout:
 *
 * <high mem>
 * +-------------------
 * | CCEE             |
 * +-------------------
 * | Saved GPRs       |
 * +-------------------
 * | Saved FPRs       |
 * +-------------------
 * | Outgoing Params  |
 * +-------------------
 * <low mem>
 *
 * sp points to the first outgoing parameter.
 */
#define CONSTANT_CStack_NumParameters	8
#define CONSTANT_CStack_NumFPRs		8
#define CONSTANT_CStack_NumGPRs		11

#define OFFSET_CStack_Parameters	0
#define OFFSET_CStack_SavedFPRs		\
    (OFFSET_CStack_Parameters + (CONSTANT_CStack_NumParameters * 4))
#define OFFSET_CStack_SavedGPRs		\
    (OFFSET_CStack_SavedFPRs + (CONSTANT_CStack_NumFPRs * 4))
#define OFFSET_CStack_CCEE		\
    (OFFSET_CStack_SavedGPRs + (CONSTANT_CStack_NumGPRs * 4))
#define CONSTANT_CStack_FrameSize	\
    (OFFSET_CStack_CCEE + CONSTANT_CVMCCExecEnv_size)

#endif /* _INCLUDED_JITASMCONSTANTS_CPU_H */
