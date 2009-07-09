/*
 * @(#)jit_cpu.S	1.9 06/10/10
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
NESTED(CVMJITgoNative, CONSTANT_CVMCCExecEnv_size, ra)
#define off_31	-0
#define off_30	-4
#define off_28	-8
#define off_23	-12
#define off_22	-16
#define off_21	-20
#define off_20	-24
#define off_19	-28
#define off_18	-32
#define off_17	-36
#define off_16	-40
#define off_f20	-44
#define off_f21	-48
#define off_f22	-52
#define off_f23	-56
#define off_f24	-60
#define off_f25	-64
#define off_f26	-68
#define off_f27	-72
#define savesize (-off_f27+4)
#if savesize != (CONSTANT_CStack_NumGPRs*4 + CONSTANT_CStack_NumFPRs*4)
#error bad savesize
#endif
#define STACK_ALIGN 16

	framesize = CONSTANT_CStack_FrameSize
	frameadjust = ((framesize+STACK_ALIGN-1)&~(STACK_ALIGN-1))
	frameoffset = (-CONSTANT_CVMCCExecEnv_size-4)

	.mask 0x80ff0000, frameoffset
	/*
	 * a0 = exceptionObject - unused unless returning to exception handler
	 * a1 = EE
	 * a2 = JFP
	 * a3 = pc
	 */

	/* setup %gp */
	.set noreorder
	.cpload $25
	.set reorder

       	addiu	sp, sp, -frameadjust
	/* Save general registers */
	sw	ra, framesize+frameoffset+off_31(sp)
	sw	$30, framesize+frameoffset+off_30(sp)
	sw	$28, framesize+frameoffset+off_28(sp)
	sw	$23, framesize+frameoffset+off_23(sp)
	sw	$22, framesize+frameoffset+off_22(sp)
	sw	$21, framesize+frameoffset+off_21(sp)
	sw	$20, framesize+frameoffset+off_20(sp)
	sw	$19, framesize+frameoffset+off_19(sp)
	sw	$18, framesize+frameoffset+off_18(sp)
	sw	$17, framesize+frameoffset+off_17(sp)
	sw	$16, framesize+frameoffset+off_16(sp)
#if defined(CVM_JIT_USE_FP_HARDWARE) && (CVMCPU_FP_NON_VOLATILE_SET != 0)
	/* We only use 8 NV FPRs so we don't have to save all of them */
#if CVMCPU_FP_NON_VOLATILE_SET != 0x0ff00000
#error "Need to save different NV registers"
#endif
	swc1	$20, framesize+frameoffset+off_f20(sp)
	swc1	$21, framesize+frameoffset+off_f21(sp)
	swc1	$22, framesize+frameoffset+off_f22(sp)
	swc1	$23, framesize+frameoffset+off_f23(sp)
	swc1	$24, framesize+frameoffset+off_f24(sp)
	swc1	$25, framesize+frameoffset+off_f25(sp)
	swc1	$26, framesize+frameoffset+off_f26(sp)
	swc1	$27, framesize+frameoffset+off_f27(sp)
#endif
	move	JFP, a2
	move	EE, a1
	lw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	lw	CHUNKEND, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_stackChunkEnd(EE)
#ifdef CVMCPU_HAS_CP_REG
	lw	CP, OFFSET_CVMCompiledFrame_cpBaseRegX(JFP)
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	/*
	 * Load CVMMIPS_GC_REGNAME with the address that will cause a trap
	 * when a gc is requested.
	 */
	la	CVMMIPS_GC_REGNAME, CVMgcTrapAddrPtr
	lw	CVMMIPS_GC_REGNAME, 0(CVMMIPS_GC_REGNAME)
	lw	CVMMIPS_GC_REGNAME, 0(CVMMIPS_GC_REGNAME)
#endif
	sw	EE, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ee(sp)
	sw	CHUNKEND, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_stackChunkEnd(sp)
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	la	a2, CVMCCMruntimeGCRendezvousGlue
	sw	a2, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmGCRendezvousGlue(sp)
#endif
        /* Store pointer to gp */
	sw	gp, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_gp(sp)
	jr	a3
SET_SIZE( CVMJITgoNative ) 

/* 
 * Return from C helper function to interpreter. 
 * void
 * CVMextNative(CVMCCExecEnv*cc ee); 
 */ 
ENTRY( CVMJITexitNative ) 
	/* a0 = ccee */

	/*
	 * return NULL, meaning we do not want the interpreter 
	 * to take any further action on our behalf
	 */
        li	v0, 0

	addiu   sp, a0, -OFFSET_CStack_CCEE

	EXPORT(MIPSexitNative0)
MIPSexitNative0:

	/*
	 * Restore floating-point registers?
	 * Restore general registers
	 */
	lw	ra, framesize+frameoffset+off_31(sp)
	lw	$30, framesize+frameoffset+off_30(sp)
	lw	$28, framesize+frameoffset+off_28(sp)
	lw	$23, framesize+frameoffset+off_23(sp)
	lw	$22, framesize+frameoffset+off_22(sp)
	lw	$21, framesize+frameoffset+off_21(sp)
	lw	$20, framesize+frameoffset+off_20(sp)
	lw	$19, framesize+frameoffset+off_19(sp)
	lw	$18, framesize+frameoffset+off_18(sp)
	lw	$17, framesize+frameoffset+off_17(sp)
	lw	$16, framesize+frameoffset+off_16(sp)
#if defined(CVM_JIT_USE_FP_HARDWARE) && (CVMCPU_FP_NON_VOLATILE_SET != 0)
	/* We only use 8 NV FPRs so we don't have to save all of them */
#if CVMCPU_FP_NON_VOLATILE_SET != 0x0ff00000
#error "Need to save different NV registers"
#endif
	lwc1	$20, framesize+frameoffset+off_f20(sp)
	lwc1	$21, framesize+frameoffset+off_f21(sp)
	lwc1	$23, framesize+frameoffset+off_f22(sp)
	lwc1	$23, framesize+frameoffset+off_f23(sp)
	lwc1	$24, framesize+frameoffset+off_f24(sp)
	lwc1	$25, framesize+frameoffset+off_f25(sp)
	lwc1	$26, framesize+frameoffset+off_f26(sp)
	lwc1	$27, framesize+frameoffset+off_f27(sp)
#endif
	addiu	sp, sp, frameadjust
	jr	ra
SET_SIZE( CVMJITexitNative ) 

	
/*
 * Fixup up uninitialized fields in compiled frames
 * extern void
 * CVMJITfixupFrames(CVMFrame *);
 */
ENTRY ( CVMJITfixupFrames )
#define CFP a0
#define PREV a1
#define TMP a2
        lw PREV, OFFSET_CVMFrame_prevX(CFP)
3:
	li TMP, CONSTANT_CVM_FRAMETYPE_COMPILED
	sb TMP, OFFSET_CVMFrame_type(CFP)
	sb zero, OFFSET_CVMFrame_flags(CFP)
        ori TMP, PREV, CONSTANT_CVM_FRAME_MASK_SPECIAL
        sw TMP, OFFSET_CVMFrame_prevX(CFP)
	move CFP, PREV
        lw PREV, OFFSET_CVMFrame_prevX(PREV)
        andi TMP, PREV, CONSTANT_CVM_FRAME_MASK_ALL
	beq TMP, zero, 3b
	jr ra
#undef CFP
#undef PREV
#undef TMP
SET_SIZE( CVMJITfixupFrames ) 
