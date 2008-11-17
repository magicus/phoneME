/*
 * @(#)jit_cpu.S	1.25 06/10/10
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

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

	SET_SECTION_EXEC(jit_cpu)

/* 
 * A note on C stack usage: 
 * There will be at most one native code frame for compiled methods on the
 * stack per interpreter invocation. The native code frame is shared by any
 * chain of compiled methods. It is created by CVMJITgoNative() and also
 * includes space for the CVMCCExecEnv struct.
 *
 * Recursion back into the interpreter only occurs if a compiled methods calls
 * a JNI method which invokes another Java method, or if the compiled method
 * calls a CCM helper which needs to execute Java bytecodes.  But if a
 * compiled method is going to call an interpreted method, we always return
 * back to the interpreter, even if there are still compiled frames on the
 * stack.  The interpreter and compiled code act like co-routines.
 */

#ifdef WINCE
#define CVMJITgoNative CVMgoNative0
#ifdef CVMJIT_HAVE_STATIC_CODECACHE
/* Currently this only applies to WinCE, because of the way
   it handles exceptions. */
	SET_SECTION_EXEC_WRITE
#endif
#endif
/* 
 * Entry point from interpreted code: 
 * CVMMethodBlock* 
 * CVMJITgoNative(CVMObject* exceptionObject, CVMExecEnv* ee, 
 *	          CVMCompiledFrame *jfp, CVMUint8 *pc); 
 */
#if (defined(WINCE) && defined(CVMJIT_HAVE_STATIC_CODECACHE))
	NESTED_ENTRY CVMJITgoNative
#else
	ENTRY(CVMJITgoNative)
#endif
ENTRY1( CVMJITgoNative )
	/* a1 = exceptionObject - unused unless returning to exception handler
	 * a2 = EE 
	 * a3 = JFP 
	 * a4 = pc 
	 */

	/* use gcc prologue */
	mov	ip, sp
	stmfd	sp!, {v1-v7, fp, ip, lr}
#ifdef CVMJIT_HAVE_STATIC_CODECACHE
	/* Keep sp 64-bit aligned for AAPCS */
	sub	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) BIT_AND ~7)
	PROLOG_END
#endif

/* IAI-04 */
/* Cache the two global variables in wmmx registers. */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
#define GLOBALS    fp
#define MICROLOCK  ip
	/* Cache the two global variables in wmmx registers. */
	IMPORT(CVMglobals)
	IMPORT(CVMobjGlobalMicroLockPtr)
	ldr	GLOBALS, =CVMglobals
	ldr     MICROLOCK, =CVMobjGlobalMicroLockPtr
	tinsrw  W_CVMGLOBALS, GLOBALS, #0
	wldrw   W_MICROLOCK, [MICROLOCK]
#endif

	mov	JFP, a3
	ldr	JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	ldr	a3, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_stackChunkEnd]
#ifndef CVMJIT_HAVE_STATIC_CODECACHE
	/* Keep sp 64-bit aligned for AAPCS */
	sub	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) BIT_AND ~7)
#endif
	str	a2, [sp, #OFFSET_CVMCCExecEnv_ee]
	str	a3, [sp, #OFFSET_CVMCCExecEnv_stackChunkEnd]
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	IMPORT(CVMCCMruntimeGCRendezvousGlue)
	ldr	a3, =CVMCCMruntimeGCRendezvousGlue
	str	a3, [sp, #OFFSET_CVMCCExecEnv_ccmGCRendezvousGlue]
#endif
#ifdef CVMCPU_HAS_CP_REG
	ldr	CVMARM_CP_REGNAME, [JFP, #OFFSET_CVMCompiledFrame_cpBaseRegX]
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	/*
	 * Load CVMARM_GC_REGNAME with the address that will cause a trap
	 * when a gc is requested.
	 */
	IMPORT(CVMgcTrapAddrPtr)
	ldr	CVMARM_GC_REGNAME, =CVMgcTrapAddrPtr
	ldr	CVMARM_GC_REGNAME, [CVMARM_GC_REGNAME]
	ldr	CVMARM_GC_REGNAME, [CVMARM_GC_REGNAME]
#if defined(CVMCPU_HAS_VOLATILE_GC_REG)
	/* save CVMARM_GC_REGNAME in the ccee for easier access later */
	str	CVMARM_GC_REGNAME, [sp, #OFFSET_CVMCCExecEnv_gcTrapAddr]
#endif
#endif
	/*
	 * Compiled code is always ARM code, so a bx is never needed here.
	 */
	mov	pc, a4
#ifndef WINCE
	SET_SIZE( CVMJITgoNative ) 
#endif
	POOL

#ifdef CVMJIT_HAVE_STATIC_CODECACHE

CVMcodeCacheStart
        export CVMcodeCacheStart
        %       (511 * 1024)
CVMcodeCacheEnd
        export CVMcodeCacheEnd
#endif

/* 
 * Return from C helper function to interpreter. 
 * void
 * CVMextNative(CVMCCExecEnv*cc ee); 
 */ 
#ifdef CVMJIT_HAVE_STATIC_CODECACHE
	ALTERNATE_ENTRY CVMJITexitNative
#else
	ENTRY(CVMJITexitNative)
#endif
ENTRY1( CVMJITexitNative )
	/* a1 = ccee 
	 *
         * return NULL, meaning we do not want the interpreter 
         * to take any further action on our behalf 
	 */
	/* Keep sp 64-bit aligned for AAPCS */
	add	sp, a1, #((CONSTANT_CVMCCExecEnv_size+7) BIT_AND ~7)
        mov	a1,#0
#ifdef WINCE
	/* WinCE epilogue */
	ldmfd	sp, {v1-v7, fp, sp, pc}
#else
	ldmfd	sp!, {v1-v7, fp, ip, lr}
	BR_REG(lr)
#endif
	SET_SIZE( CVMJITexitNative ) 

	SET_SECTION_EXEC(jit_cpu)
	
/*
 * Fixup up uninitialized fields in compiled frames
 * extern void
 * CVMJITfixupFrames(CVMFrame *);
 */
	ENTRY(CVMJITfixupFrames)
ENTRY1 ( CVMJITfixupFrames )
#define CFP a1
#define PREV a2
#define ZERO a3
#define TMP ip
        ldr PREV, [CFP,#OFFSET_CVMFrame_prevX]
	mov ZERO, #0
LABEL(fixupframeloop)
	mov TMP, #CONSTANT_CVM_FRAMETYPE_COMPILED
	strb TMP, [CFP, #OFFSET_CVMFrame_type]
	strb ZERO, [CFP, #OFFSET_CVMFrame_flags]
        orr TMP, PREV, #CONSTANT_CVM_FRAME_MASK_SPECIAL
        str TMP, [CFP,#OFFSET_CVMFrame_prevX]
	mov CFP, PREV
        ldr PREV, [CFP,#OFFSET_CVMFrame_prevX]
        tst PREV, #CONSTANT_CVM_FRAME_MASK_ALL
	beq fixupframeloop
	BR_REG(lr)
#undef CFP
#undef PREV
#undef TMP
	SET_SIZE( CVMJITfixupFrames ) 

#ifdef __RVCT__
	END
#endif
