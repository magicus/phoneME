/*
 * @(#)jit_cpu.S	1.22 06/10/10
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
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"
#include "portlibs/jit/risc/include/porting/jitrisc.h"

	.section	".text",""
	.align	4

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

/* 
 * Entry point from interpreted code: 
 * CVMMethodBlock* 
 * CVMJITgoNative(CVMObject* exceptionObject, CVMExecEnv* ee, 
 *	          CVMCompiledFrame *jfp, CVMUint8 *pc); 
 */

ENTRY( CVMJITgoNative )
	# NOTE:	 Instruction scheduling can be improved.
	#
	# r3 = exceptionObject - unused unless returning to exception handler
	# r4 = EE
	# r5 = JFP
	# r6 = pc
	# use gcc prologue
	mflr	r0		    /* get return address */
	stwu	sp, -CStack_FrameSize(sp)  /* back chain and move SP */
	stw	r0, OFFSET_CStack_SavedLR(sp)     /* save the return address */
	mfcr	r0
	stmw	r14, OFFSET_CStack_SavedGPRs(sp)  /* save NV registers */
	stw	r0, CVMCCExecEnv_ccmStorage_CR(sp) /* save CR */
#ifdef CVM_JIT_USE_FP_HARDWARE
	/* We only use 8 NV FPRs so we don't have to save all of them */
#if CVMCPU_FP_NON_VOLATILE_SET != 0x003fc000
#error "Need to save different NV registers"
#endif
	stfd	f14, OFFSET_CStack_SavedFPRs(sp)
	stfd	f15, OFFSET_CStack_SavedFPRs+8(sp)
	stfd	f16, OFFSET_CStack_SavedFPRs+16(sp)
	stfd	f17, OFFSET_CStack_SavedFPRs+24(sp)
	stfd	f18, OFFSET_CStack_SavedFPRs+32(sp)
	stfd	f19, OFFSET_CStack_SavedFPRs+40(sp)
	stfd	f20, OFFSET_CStack_SavedFPRs+48(sp)
	stfd	f21, OFFSET_CStack_SavedFPRs+56(sp)
#endif
	mr	JFP, r5
	mr	EE, r4
	lwz	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	lwz	CHUNKEND, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_stackChunkEnd(EE)
	stw	EE, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ee(sp)
	stw	CHUNKEND, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_stackChunkEnd(sp)
#ifdef CVMCPU_HAS_CP_REG
	lwz	CP, OFFSET_CVMCompiledFrame_cpBaseRegX(JFP)
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	/*
	 * Load CVMPPC_GC_REGNAME with the address that will cause a trap
	 * when a gc is requested.
	 */
	lis	CVMPPC_GC_REGNAME, HA16(CVMgcTrapAddrPtr)
	lwz	CVMPPC_GC_REGNAME, LO16(CVMgcTrapAddrPtr)(CVMPPC_GC_REGNAME)
	lwz	CVMPPC_GC_REGNAME, 0(CVMPPC_GC_REGNAME)
#endif
	lis	CVMGLOBALS, HA16(CVMglobals) /* setup CVMGLOBALS register */
	mtlr	r6
	la	CVMGLOBALS, LO16(CVMglobals)(CVMGLOBALS) 
	blr
 SET_SIZE( CVMJITgoNative ) 

/* 
 * Return from C helper function to interpreter. 
 * void
 * CVMextNative(CVMCCExecEnv*cc ee);
 *
 * WARINING: keep this in sync with returnToInterpreter1 in ccminvokers_cpu.S
 */ 
ENTRY( CVMJITexitNative ) 
	# r3 = ccee
        # return NULL, meaning we do not want the interpreter 
        # to take any further action on our behalf
	subi	sp, r3, OFFSET_CStack_CCEE /* pop to CVMJITgoNative frame */
	lwz	r0, OFFSET_CStack_SavedLR(sp)      /* get return address */
	lwz	r4, CVMCCExecEnv_ccmStorage_CR(sp) /* restore CR */
	lmw	r14, OFFSET_CStack_SavedGPRs(sp)   /* restore NV registers */
#ifdef CVM_JIT_USE_FP_HARDWARE
	/* We only use 8 NV FPRs so we don't have to restore all of them */
#if CVMCPU_FP_NON_VOLATILE_SET != 0x003fc000
#error Need to restore different NV registers
#endif
	lfd	f14, OFFSET_CStack_SavedFPRs(sp)
	lfd	f15, OFFSET_CStack_SavedFPRs+8(sp)
	lfd	f16, OFFSET_CStack_SavedFPRs+16(sp)
	lfd	f17, OFFSET_CStack_SavedFPRs+24(sp)
	lfd	f18, OFFSET_CStack_SavedFPRs+32(sp)
	lfd	f19, OFFSET_CStack_SavedFPRs+40(sp)
	lfd	f20, OFFSET_CStack_SavedFPRs+48(sp)
	lfd	f21, OFFSET_CStack_SavedFPRs+56(sp)
#endif
	mtcr	r4
	mtlr	r0			   /* put return address in lr */
	addic	sp, sp, CStack_FrameSize   /* pop CVMJITgoNative frame */
        li	r3, 0			   /* return NULL */
	blr				   /* return */
SET_SIZE( CVMJITexitNative ) 
	
/*
 * Fixup up uninitialized fields in compiled frames
 * extern void
 * CVMJITfixupFrames(CVMFrame *);
 *
 * Trashes r0 and r3-r5. All other registers are preserved.
 */
ENTRY ( CVMJITfixupFrames )
#define CFP  r3
#define PREV r4
#define ZERO r5
#define TMP  r0
	lwz	PREV, OFFSET_CVMFrame_prevX(CFP)
	li	ZERO, 0

	# Fixup the flags, type, and prevX field of each frame
	# in the stack that needs fixing.
0:
	li	TMP, CONSTANT_CVM_FRAMETYPE_COMPILED
	stb	ZERO, OFFSET_CVMFrame_flags(CFP)
	stb	TMP, OFFSET_CVMFrame_type(CFP)
        ori	TMP, PREV, CONSTANT_CVM_FRAME_MASK_SPECIAL
        stw	TMP, OFFSET_CVMFrame_prevX(CFP)
	mr	CFP, PREV
        lwz	PREV, OFFSET_CVMFrame_prevX(CFP)
        andi.	TMP, PREV, CONSTANT_CVM_FRAME_MASK_ALL
	beq-	0b

	blr
#undef CFP
#undef PREV
#undef TMP

SET_SIZE( CVMJITfixupFrames ) 
