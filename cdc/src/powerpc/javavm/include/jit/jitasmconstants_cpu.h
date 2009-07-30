/*
 * @(#)jitasmconstants_cpu.h	1.17 06/10/10
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

/*
 * Special registers we use in assembler code that also need to be
 * exported to jitrisc_cpu.h and other C code. See the CVMCPU_MAP_REGNAME
 * macro to see how to map these register names to register numbers.
 */
#define CVMPPC_JSP_REGNAME		r31
#define CVMPPC_JFP_REGNAME		r30
#define CVMPPC_CHUNKEND_REGNAME		r29
#define CVMPPC_CVMGLOBALS_REGNAME	r28
#define CVMPPC_EE_REGNAME		r27
#ifdef CVMCPU_HAS_CP_REG
#define CVMPPC_CP_REGNAME		r26
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMPPC_GC_REGNAME		r25
#endif

/* The registers that need to be returned by prologue code */

#define CVMPPC_PREVFRAME_REGNAME   r24  /* Used for non-sync invocations */
#define CVMPPC_NEWJFP_REGNAME      r23  /* Used for sync invocations     */

/*
 * Stack Frame Layout:
 * We use a combination of the MacOS and Linux models in such a way
 * that is should work fine for both.
 */
#define OFFSET_CStack_SavedSP	     0 /* callers SP */
#define OFFSET_CStack_CalleeSavedCR  4 /* reserved for functions we *call* */
#define OFFSET_CStack_CalleeSavedLR  8 /* reserved for functions we *call* */
#define OFFSET_CStack_Reserved1	    12
#define OFFSET_CStack_Reserved2	    16
#define OFFSET_CStack_SavedLR	    20 /* address we need to return to */ 
#define OFFSET_CStack_Parameters    24 
#define OFFSET_CStack_CCEE	 \
    (OFFSET_CStack_Parameters + 8*4) /* room for 8 parameters */
#define OFFSET_CStack_SavedGPRs	 \
    (OFFSET_CStack_CCEE + CONSTANT_CVMCCExecEnv_size)
#define OFFSET_CStack_SavedFPRs			\
    /* room for r14-r31 */			\
    (OFFSET_CStack_SavedGPRs + 18*4)
#define CStack_FrameSize					\
    /* room for f14-f21 + pad out to 16 byte boundary */	\
    (((OFFSET_CStack_SavedFPRs + 8*8) + 15) & ~15)

#endif /* _INCLUDED_JITASMCONSTANTS_CPU_H */
