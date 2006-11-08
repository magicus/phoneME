/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

/*
 * Some of the code in this file gets patched at runtime for handling
 * gc rendezvous. If we aren't copying this code to the code cache,
 * then we must make this a writable section.
 */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	.text
#else
	.section	.rwcode,"wxa"
#endif

/* CVMMIPS_NEWJFP_REGNAME and CVMMIPS_PREVFRAME_REGNAME are defined
 * jitasmconstants_cpu.h. CVMMIPS_NEWJFP_REGNAME is a2, and
 * CVMMIPS_PREVFRAME_REGNAME is t8.
 */
#define NEW_JFP CVMMIPS_NEWJFP_REGNAME
#define PREV	CVMMIPS_PREVFRAME_REGNAME

ENTRY( CVMCCMinvokeNonstaticSyncMethodHelper ) 
ccmInvokeNonstaticSyncMethodHelper:	
	/*
	 * a0 = mb
	 * a1 = CVMObjectICell* of object to sync on.
	 */

#if 0
	/*
	 * If you just want to call the C helper and write very little
	 * assembler code, then just to branch to (and implement)
	 * letInterpreterDoInvoke.
	 */
        b       letInterpreterDoInvoke
#endif

#define MB		a0
#define OBJ		a1
#define OBITS		a3
#define LOCKREC		t7
#define OBITS0		v0
#define NBITS		v1

	lw	OBJ, 0(a1)	/* get object to sync on. */
	sw	MB, OFFSET_CVMFrame_mb(NEW_JFP)
	/* optimistically store receiver object */
	sw	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(NEW_JFP)

        /* Do fastTryLock(): */

        /* Setup a lock record and assume the object has not been locked
	   yet:	 */
        lw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
	beq	LOCKREC, zero, _lockRecordNotAvailable

        sw	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

        li	jp, 1
        sw	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)

#ifdef CVM_DEBUG
        li	jp, CONSTANT_CVM_OWNEDMON_OWNED
        sw	jp, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	/* obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED*/
        lw	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
	li	jp, ~3
	and	OBITS, OBITS, jp	/* clear rightmost 2 bits */
	ori	OBITS, OBITS, CONSTANT_CVM_LOCKSTATE_UNLOCKED

        sw	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

        /* Do atomicCompareAndSwap: */
1:
	move	NBITS, LOCKREC
	ll	OBITS0, OFFSET_CVMObjectHeader_various32(OBJ)
	bne	OBITS0, OBITS, _objAlreadyLocked
	sc	NBITS, OFFSET_CVMObjectHeader_various32(OBJ)
	beq	NBITS, zero, 1b	/* retry if failed */

        /* Remove lockrec from the ee's free list: */
        lw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	jp, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        /* Add the lockrec to the ee's owned list: */
        lw	jp, OFFSET_CVMExecEnv_objLocksOwned(EE)
        sw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)
	b	_fastlockSuccess	/* we are all done locking */

_objAlreadyLocked:
#ifdef CVM_DEBUG
        sw	zero, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        li	jp, CONSTANT_CVM_OWNEDMON_FREE
        sw	jp, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	/* If object is not in LOCKED state, then fail: */
	andi	jp, OBITS0, 0x3
	bne	jp, zero, _fastRetryFailed
	
	/* If not associated with a lock record, then fail: */
	li	jp, ~0x3
	and	LOCKREC, OBITS0, jp
	beq	LOCKREC, zero, _fastReentryFailed

	/* If (lockrec->owner != ee), then fail: */
	lw	jp, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
	bne	jp, EE, _fastReentryFailed 

#define EXPECTED_CNT OBITS
#define NEW_COUNT NBITS
#define OLD_COUNT OBITS0
	/* If we get here, then we are re-entering the lock: */
	lw	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	li	jp, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	EXPECTED_CNT, jp, _fastReentryFailed

	addi	NEW_COUNT, EXPECTED_CNT, 1
1:
	move	jp, NEW_COUNT
	ll	OLD_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	bne	OLD_COUNT, EXPECTED_CNT, _fastReentryFailed
	sc	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	beq	jp, zero, 1b /* retry */
#undef EXPECTED_CNT
#undef NEW_COUNT
#undef OLD_COUNT

	/* If the old count is as expected, then we are successful: */
        b	_fastlockSuccess

_fastRetryFailed:
_fastReentryFailed:
_lockRecordNotAvailable:
        /* Let interpreter do the hard stuff: */
	lw	MB, OFFSET_CVMFrame_mb(NEW_JFP)
        move	PREV, JFP
        b	letInterpreterDoInvoke

_fastlockSuccess:
        move	PREV, JFP
        move	JFP, NEW_JFP

	/* compiled frame */
#ifdef CVM_DEBUG_ASSERTS
	li	jp, CONSTANT_CVM_FRAMETYPE_NONE
	sb	jp, OFFSET_CVMFrame_type(JFP)
	li	jp, -1
	sb	jp, OFFSET_CVMFrame_flags(JFP)
#endif
	sw	PREV, OFFSET_CVMFrame_prevX(JFP) 

#ifdef CVM_TRACE
	sw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	move	a0, EE		/* arg1 = EE */
	move	a1, JFP		/* arg2 = JFP */
	CALL_VM_FUNCTION(CVMCCMtraceMethodCallGlue)
	lw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	jr	ra
#endif

#undef MB
#undef OBJ
#undef OBITS
#undef LOCKREC
#undef OBITS0
#undef NBITS

/* TODO: Implement gc patch. See the ARM port for example. */
EXPORT( CVMARMgcPatchPointAtInvoke ) 
CVMARMgcPatchPointAtInvoke:
armGcPatchPointAtInvoke:	
	/*
	 * GC check - gc will patch at this location when a rendezvous is
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	 * be a "b CVMARMhandleGCAtInvoke"
	 */

        jr	ra	# Return to method after handling possible GC request

/* TODO: Implement gc patch. See the ARM port for example. */
EXPORT( CVMARMhandleGCAtInvoke)	
CVMARMhandleGCAtInvoke:	
	/*
	 * At this point a GC is requested.
	 */

#define ORIG_LR s6
#define r0 a0
#define r1 a1
#define r2 a2
#define r3 a3

	move	ORIG_LR, ra
	
	FIXUP_FRAMES_0(JFP, a0)

	lw	r1, OFFSET_CVMFrame_mb(JFP)		# r1 = mb

	/* We will be gcsafe soon. Prevent this method from being decompiled */
	sw	r1, OFFSET_CVMExecEnv_invokeMb(EE)

	/*
	 * Check if this is a synchronized invocation
	 * If it is, we have to stash the receiver in the
	 * newly pushed frame into a safe location. The new frame is not
	 * yet "committed" to the stack, and as such, cannot be located
	 * by GC.
	 */
        lbu	r1, OFFSET_CVMMethodBlock_accessFlagsX(r1)
	andi	r1, r1, CONSTANT_METHOD_ACC_SYNCHRONIZED
	beq	r1, zero, 1f
	
	/*
	 * Synchronized method if result of 'tst' is 'ne'. Stash
	 * receiver in [ee->miscICell]
	 */
	lw	r1, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
	lw	r2, OFFSET_CVMExecEnv_miscICell(EE)
	sw	r1, (r2)				# stash in miscICell

1:
	LA(jp, CVMglobals)
        sw	PREV, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	li	a3, 1
	addiu	a1, jp, OFFSET_CVMGlobalState_cstate_GCSAFE
	addiu	a2, r0, OFFSET_CVMExecEnv_tcstate_GCSAFE
	move	a0, EE
	CALL_VM_FUNCTION(CVMcsRendezvous)

	/* reload the mb */
	lw	r1, OFFSET_CVMFrame_mb(JFP)		# r1 = mb

	/* we no longer need to prevent the method from being decompiled */
	sw	zero, OFFSET_CVMExecEnv_invokeMb(EE)

	move	ra, ORIG_LR
	
	/*
	  We've returned from the GC. Check for a sync method
	  again to see if we should restore 'receiverObjX'
	  from miscICell.
	*/
        lbu	r2, OFFSET_CVMMethodBlock_accessFlagsX(r1)
	andi	r2, r2, CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq	r2, zero, armGcPatchPointAtInvoke

	/* Restore receiverObjX in new frame */
	lw	r0, OFFSET_CVMExecEnv_miscICell(EE)	# r0 = &miscICell
	lw	jp, (r0)
	sw	jp, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
	
	/* And clear miscICell for other uses */
	sw	zero, (r0)

	b armGcPatchPointAtInvoke

#undef ORIG_LR
#undef r0
#undef r1
#undef r2
#undef r3

	SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

/* 
 * Set up arg2(a1) as the ICell of the object to lock, then call
 * CVMCCMinvokeNonstaticSyncMethodHelper.
 */
ENTRY( CVMCCMinvokeStaticSyncMethodHelper ) 
        /* a0 = target mb  */

#define MB	a0

#ifdef CVM_METHODBLOCK_HAS_CB
	lw	a1, OFFSET_CVMMethodBlock_cbX(MB)
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        lbu	a1, OFFSET_CVMMethodBlock_methodIndexX(MB)

	/* a1 = a1 * 32 - a1 * 4
         * which is a1 = a1 * 28
         */
	sll	t7, a1, 5
	sll	a1, a1, 2
	subu	a1, t7, a1
	/* a1 = MB - a1*28 */
	subu	a1, MB, a1
        lw	a1, -OFFSET_CVMMethodRange_mb(a1) /* get the CB */
#endif
	/*
	 * a1 needs to be set to the icell of the object to lock
	 */
	lw	a1, OFFSET_CVMClassBlock_javaInstanceX(a1)
	b	ccmInvokeNonstaticSyncMethodHelper
#undef MB
	SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

ENTRY( CVMCCMinvokeCNIMethod ) 
        /* a0 = target mb */

#define MB	a0

        lbu	a1, OFFSET_CVMMethodBlock_argsSizeX(MB)
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	sll	a1, a1, 2
        subu	a1, JSP, a1	# TOS

#undef MB
#define MB	s4
#define ARGS	s5

	move	MB, a0		# save MB
	move	ARGS, a1	# save args ptr

	/* although a1 is now in ARGS, we still want to preserve it */
	FIXUP_FRAMES_a1(JFP, jp)

        sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)

#ifdef CVM_TRACE
	/* trace call */
	move	a1, JFP
	move	a2, MB
	move	a0, EE
	bal ccmTraceFramelessMethodCall
	move	a1, ARGS		# restore args ptr
#endif

	/*
	 * invoke the method  - a1 is still the args pointer
	 * a2 = mbPtr
	 */
	addiu	a2, sp, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage
        lw	jp, OFFSET_CVMMethodBlock_codeX(MB)
	sw	MB, (a2)	# save the mb in the ccmStorage area
	move	a0, EE
	jalr	jp

#ifdef CVM_TRACE
	/*
	 * trace return
	 * save result
	 */
	sw	v0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
	move	a1, MB
	move	a2, JFP
	move	a0, EE
	bal ccmTraceFramelessMethodReturn
	/* restore result */
	lw	v0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
#endif

	/* if v0 >= 0, then v0 is the size in words of the method result */
	slt	a1, v0, zero
	bne	a1, zero, 1f

	/* pop args and adjust for result */
	sll	a1, v0, 2
	addu	JSP, ARGS, a1
        lw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra

1:

	/* check if a new mb to execute has been returned */
	li	a1, CONSTANT_CNI_NEW_MB
	bne	v0, a1, new_transition
	/* v0 = newMb */
	lw	v0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	/* adjust TOS. The new method may have fewer args than the CNI method*/
        lbu	a1, OFFSET_CVMMethodBlock_argsSizeX(v0)    # a1 = argsSize
	sll	a1, a1, 2
        addu	JSP, ARGS, a1	# adjust TOS past args
        sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	b	returnToInterpreter1

	/* check if a new transition frame to execute has been setup */
new_transition:	
	li	a1, CONSTANT_CNI_NEW_TRANSITION_FRAME
	bne	v0, a1, returnToInterpreter	# an exception has occurred

        sw	ARGS, OFFSET_CVMFrame_topOfStack(JFP)       # pop args
	b	returnToInterpreter0

#undef MB
#undef ARGS
        SET_SIZE( CVMCCMinvokeCNIMethod ) 

ENTRY( CVMCCMinvokeJNIMethod ) 
        /* a0 = target mb  */
#define MB  a0

        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)

	FIXUP_FRAMES_a0(JFP, t7)

	move	a1, MB

        sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE) 

	move	a0, EE
	CALL_VM_FUNCTION(CVMinvokeJNIHelper)

        lw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	/* check for exception */
	beq	v0, zero, returnToInterpreter0

        lw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra

        SET_SIZE( CVMCCMinvokeJNIMethod ) 

ENTRY( CVMCCMletInterpreterDoInvoke )

#define MB	a0

letInterpreterDoInvoke_store_lr:
        sw ra, OFFSET_CVMCompiledFrame_PC(JFP)

EXPORT( CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr )
CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr:
letInterpreterDoInvoke: 
        /* 
         * Trying to invoke something beyond our ability. 
         * Return the mb to the interpreter and let it do the 
         * dirty work. 
         * we have already set up the return PC in our own frame 
         * We need to set topOfStack then return the target MB* 
         * as a C return value. 
         */ 
	FIXUP_FRAMES_a0(JFP, a1)
        sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	move	v0, MB
	b returnToInterpreter1

#undef MB

SET_SIZE( CVMCCMletInterpreterDoInvoke )

/*
 * Do a GC check, and rendezvous if one is requested
 */
ENTRY( CVMARMhandleGCForReturn )
handleGCForReturn:	
	/*
	 * At this point a GC is requested.
	 */

	FIXUP_FRAMES_0(JFP, a0)

	sw	a1, OFFSET_CVMFrame_mb(JFP)		# a1 = mb

        /* 
         * Special flag that signals we are handling gc for return. 
         * Used by CVMcompiledFrameScanner. 
         */ 
	li	ra, CONSTANT_HANDLE_GC_FOR_RETURN
	sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	LA(ra, CVMglobals)
	
	/* We will be gcsafe soon. Prevent this method from being decompiled */
	sw	a1, OFFSET_CVMExecEnv_invokeMb(EE)

	sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	/* Fill in the arguments to CVMcsRendezvous	 */
	addu	a1, ra, OFFSET_CVMGlobalState_cstate_GCSAFE
	addu	a2, EE, OFFSET_CVMExecEnv_tcstate_GCSAFE
	li	a3, 1
	move	a0, EE
	CALL_VM_FUNCTION(CVMcsRendezvous)

	/*
	 * GC done.
	 */

	/* we no longer need to prevent the method from being decompiled */
	sw	zero, OFFSET_CVMExecEnv_invokeMb(EE)

	/* This is expected at the beginning of returns */
        lw	PREV, OFFSET_CVMFrame_prevX(PREV)
	/* Return to caller */
	jr	s6
	nop

/*
 * The GC checks for the various return variants
 */
EXPORT( CVMARMhandleGCForReturnFromMethod ) 
	LA(s6, CVMCCMreturnFromMethod)
	b	handleGCForReturn	
	nop

EXPORT( CVMARMhandleGCForReturnFromSyncMethod ) 
	LA(s6, CVMCCMreturnFromSyncMethod)
	b	handleGCForReturn	
	nop
SET_SIZE( CVMARMhandleGCForReturn )

/* 
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 *
 * CVMMethodBlock* CVMCCMreturnFromMethod(); 
 */ 
ENTRY( CVMCCMreturnFromMethod )
	/*
	 * GC check - gc will patch at this location when a rendezvous is
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	 * be a "b CVMMIPShandleGCForReturnFromMethod"
	 */

	/*
	 * see if previous frame is compiled or not
	 * PREV is set up by all code that branches here
	 */
	andi	jp, PREV, CONSTANT_CVM_FRAME_MASK_SLOW
	bne	jp, zero, returnToInterpreter

doReturnToCompiled:

#ifdef CVM_TRACE
	move	a0, EE    /* arg1 */
	move	a1, JFP   /* arg2 */
	sw	PREV, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	CALL_VM_FUNCTION(CCMtraceMethodReturn)
	lw	PREV, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
#endif /* CVM_TRACE */

	/* setup JFP */
	li	jp, ~CONSTANT_CVM_FRAME_MASK_ALL
	and	JFP, PREV, jp

	/*
         * returning from one native to another. 
         * do this ourselves. 
	 * java sp already set
	 */
        lw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
#ifdef CVMCPU_HAS_CP_REG
	lw	CVMMIPS_CP_REGNAME, OFFSET_CVMCompiledFrame_cpBaseRegX(JFP)
#endif
	jr	ra

EXPORT( CVMCCMreturnToInterpreter )
CVMCCMreturnToInterpreter:
returnToInterpreter:
	FIXUP_FRAMES_0(JFP, a1)

	/* JSP needs to point just past any return value */
        sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)

	/*
          set stack->currentFrame to current value of JFP, 
          then return NULL, meaning we don't want the interpreter 
          to take any further action on our behalf (except pop
	  the current frame)
	*/

        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)

returnToInterpreter0:
	move	v0, zero
returnToInterpreter1:
	LA(jp,MIPSexitNative0)
	jr	jp
        SET_SIZE( CVMCCMreturnFromMethod )

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
ENTRY( CVMCCMreturnFromSyncMethod )
#if 0
	/*
	 * If you just want to call the C helper and write very little
	 * assembler code, then just to branch to (and implement)
	 * letInterpreterDoInvoke.
	 */
        b       returnToInterpreter
#endif

#define OBJ		a0
#define NBITS		a1
#define LOCKREC		a2
#define OBITS0		a3
#define EXPECTED_CNT	v1
#define NEW_COUNT	OBITS0
#define OLD_COUNT	NBITS

	/*
         * see if previous frame is compiled or not 
	 * PREV is set up by all code that branches here
	 */
	andi	jp, PREV, CONSTANT_CVM_FRAME_MASK_SLOW
	bne	jp, zero, returnToInterpreter

        /* Do fastTryUnlock(): */

        lw	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(JFP)

        /* Check to see if the object is locked with a fastlock: */
        lw	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi	jp, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne	jp, zero, _fastTryUnlockFailed /* If not, we failed. */

	/* If the LOCKREC is NULL, it is currently being inflated */
	beq	LOCKREC, zero, _fastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lw	jp, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        bne	jp, EE, _fastTryUnlockFailed	/* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lw	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	li	jp, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	EXPECTED_CNT, jp, _fastTryUnlockFailed
        addi	NEW_COUNT, EXPECTED_CNT, -1
        beq	NEW_COUNT, zero, _doUnlock	/* If zero, then unlock */

	/* new monitor count > 0, so just update it and we are done */
1:
	move	jp, NEW_COUNT
	ll	OLD_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	bne	OLD_COUNT, EXPECTED_CNT, _fastTryUnlockFailed
	sc	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	beq	jp, zero, 1b /* retry */

	/* we are done! monitor count was successfully decrement */
	b	doReturnToCompiled
        /* End. */

_doUnlock:	
#define OBITS LOCKREC
        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lw	NBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
1:
	move	jp, NBITS
	ll	OBITS0, OFFSET_CVMObjectHeader_various32(OBJ)
	bne	OBITS0, OBITS, _fastTryUnlockFailed
	sc	jp, OFFSET_CVMObjectHeader_various32(OBJ)
	beq	jp, zero, 1b /* retry */
#undef OBITS

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
        li	jp, CONSTANT_CVM_OWNEDMON_FREE
        sw	jp, OFFSET_CVMOwnedMonitor_state(LOCKREC)
        sw	zero, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        sw	zero, OFFSET_CVMOwnedMonitor_object(LOCKREC)
	sw	zero, OFFSET_CVMOwnedMonitor_count(LOCKREC)
#endif

        /* Check if the lockrec is the first one on the thread's owned list: */
        lw	jp, OFFSET_CVMExecEnv_objLocksOwned(EE)
        bne	jp, LOCKREC, _fastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee's owned list: */ 
        lw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	jp, OFFSET_CVMExecEnv_objLocksOwned(EE)

_fastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lw	jp, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        sw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

	/* we are done! lock was successfully released */
	b	doReturnToCompiled

/* PREV_REC (jp) is the first one on the thread's owned list */
#define PREV_REC jp
/* It is safe to reuse NBITS at this point */
#define NEXT_REC NBITS
_fastTryUnlockFindPrevLockRecord:
        lw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        beq	NEXT_REC, LOCKREC, _fastTryUnlockFoundPrevLockRecord
        move	PREV_REC, NEXT_REC
        b	_fastTryUnlockFindPrevLockRecord

_fastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */ 
        lw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_fastTryUnlockAddLockRecordToFreeList
#undef PREV_REC
#undef NEXT_REC

_fastTryUnlockFailed:
	/* Let the interpreter handle the hard cases: */
        b	returnToInterpreter

#undef OBJ
#undef NBITS
#undef LOCKREC
#undef OBITS0
#undef EXPECTED_CNT
#undef NEW_COUNT
#undef OLD_COUNT
        SET_SIZE( CVMCCMreturnFromSyncMethod )

#ifdef CVM_TRACE

ENTRY(CVMCCMtraceMethodCallGlue)
	LA(jp, CVMglobals)
	lw	jp, OFFSET_CVMGlobalState_debugFlags(jp)
	andi	jp, jp, CONSTANT_TRACE_METHOD
	beq	jp, zero, 1f
	sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	FIXUP_FRAMES_a0a1ra(JFP, jp)
	move a2, zero		# isJump
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodCall)
1:
	jr	ra
SET_SIZE(CVMCCMtraceMethodCallGlue)

ENTRY(CCMtraceMethodReturn)
	LA(jp, CVMglobals)
	lw	jp, OFFSET_CVMGlobalState_debugFlags(jp)
	andi	jp, jp, CONSTANT_TRACE_METHOD
	beq	jp, zero, 1f
	FIXUP_FRAMES_a0a1ra(JFP, jp)
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodReturn)
1:
	jr	ra
SET_SIZE(CCMtraceMethodReturn)

ENTRY(CCMtraceFramelessMethodCall)
ccmTraceFramelessMethodCall:	
	LA(jp, CVMglobals)
	lw	jp, OFFSET_CVMGlobalState_debugFlags(jp)
	andi	jp, jp, CONSTANT_TRACE_METHOD
	beq	jp, zero, 1f
	move	a3, zero			# isJump
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodCall)
1:
	jr	ra
SET_SIZE(CCMtraceFramelessMethodCall)

ENTRY(CCMtraceFramelessMethodReturn)
ccmTraceFramelessMethodReturn:	
	LA(jp, CVMglobals)
	lw	jp, OFFSET_CVMGlobalState_debugFlags(jp)
	andi	jp, jp, CONSTANT_TRACE_METHOD
	beq	jp, zero, 1f
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodReturn)
1:
	jr	ra
SET_SIZE(CCMtraceFramelessMethodReturn)

#endif

	.text
