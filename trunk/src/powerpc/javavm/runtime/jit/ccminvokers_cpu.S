/*
 * @(#)ccminvokers_cpu.S	1.37 06/10/10
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
#include "javavm/include/sync.h"

/*
 * There are places where we assume that clearing the lower 2 bits is
 * the same as doing an "and" with ~CONSTANT_CVM_FRAME_MASK_ALL.
 */
#if CONSTANT_CVM_FRAME_MASK_ALL != 0x3
#error CONSTANT_CVM_FRAME_MASK_ALL has changed
#endif

/*
 * Some of the code in this file gets patched at runtime for handling
 * gc rendezvous. If we aren't copying this code to the code cache,
 * then we must make this a writable section.
 */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	.section	".text",""
#else
	.section	".rwcode","wxa"
#endif

#define NEW_JFP	CVMPPC_NEWJFP_REGNAME		/* r24 */
#define PREV	CVMPPC_PREVFRAME_REGNAME	/* r25 */

ENTRY( CVMCCMinvokeNonstaticSyncMethodHelper )
	# r3 = mb
	# r4 = CVMObjectICell* of object to sync on.

#define MB		r3

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just to branch to (and implement) letInterpreterDoInvoke.
	#
        b       letInterpreterDoInvoke
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        /* CAS version of CVMCCMinvokeNonstaticSyncMethodHelper: */

#define OBJ		r5
#define OBITS		r6
#define LOCKREC		r7
#define OBITS0		r8
#define EXPECTED_CNT	r9
#define NEW_COUNT	r10
#define OLD_COUNT	r11

	lwz	OBJ, 0(r4)	/* get object to sync on. */
	stw	MB, OFFSET_CVMFrame_mb(NEW_JFP)
	# optimistically store receiver object
	stw	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(NEW_JFP)

        /* Do fastTryLock(): */

        /* Setup a lock record and assume the object has not been locked
	   yet:	 */
        # lockrec = ee->objLocksFreeOwned:
        lwz	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        cmpwi	LOCKREC, 0
        beq     _lockRecordNotAvailable

        # lockrec->object = obj:
        stw	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

        # lockrec->count = 1:
        li	r0, 1		/* Initial lock re-entry count */
        stw	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)

#ifdef CVM_DEBUG
        # lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED:
        li	r0, CONSTANT_CVM_OWNEDMON_OWNED
        stw	r0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	# nbits = (LOCKREC) | CVM_LOCKSTATE_LOCKED:	
	# Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
	# that nbits is essentially lockrec.  Nothing need to be done to
	# initialize nbits.
#define NBITS LOCKREC

	# obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED:
        lwz	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
	clrrwi	OBITS, OBITS, 2	/* clear rightmost 2 bits */
	ori	OBITS, OBITS, CONSTANT_CVM_LOCKSTATE_UNLOCKED

	# lockrec->u.fast.bits = obits:
        stw	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

        /* Do atomicCompareAndSwap:
         * OBITS0 = [OBJ, #OFFSET_CVMObjectHeader_various32]
	 * if ([OBJ, #OFFSET_CVMObjectHeader_various32] == NBITS) {
	 *    [OBJ, #OFFSET_CVMObjectHeader_various32] = LOCKREC 
	 * }
	 */
	la	r0, OFFSET_CVMObjectHeader_various32(OBJ)
0:
	lwarx	OBITS0, 0, r0		/* OBITS0 = old various32 */
	cmpw	OBITS0, OBITS		/* check if already locked. */
	bne-	_objAlreadyLocked	/* already locked */
	stwcx.	NBITS, 0, r0		/* store NBITS in various32 */
	bne-	0b    /* if reservation lost, try again */
#undef NBITS

        /* Remove lockrec from the ee's free list: */
        # nextRec = lockrec->next:
        lwz	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        # ee->objLocksFreeOwned = nextRec:
        stw	r0, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        /* Add the lockrec to the ee's owned list: */
        # nextRec = ee->objLocksOwned:
        lwz	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)
        # lockrec->next = nextRec:
        stw	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        # ee->objLocksOwned = lockrec:
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)
	b	_fastlockSuccess	/* we are all done locking */

_objAlreadyLocked:
#ifdef CVM_DEBUG
        # lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
        li	r0, 0
        stw	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        li	r0, CONSTANT_CVM_OWNEDMON_FREE
        stw	r0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	# If object is not in LOCKED state, then fail:
	andi.	r0, OBITS0, 0x3	/* check for CVM_LOCKSTATE_LOCKED */
	bne-	_fastRetryFailed
	
	# If not associated with a lock record, then fail:
	clrrwi.	LOCKREC, OBITS0, 2
	beq-	_fastReentryFailed

	# If (lockrec->owner != ee), then fail:	
	lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
	cmpw	r0, EE
	bne-	_fastReentryFailed

	# If we get here, then we are re-entering the lock:
	lwz	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	cmpwi	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq-	_fastReentryFailed

	addi	NEW_COUNT, EXPECTED_CNT, 1
	la	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
0:
	lwarx	OLD_COUNT, 0, r0	/* OLD_COUNT = old count */
	cmpw	OLD_COUNT, EXPECTED_CNT	/* check if already locked. */
        bne-	_fastReentryFailed
	stwcx.	NEW_COUNT, 0, r0	/* update with NEW_COUNT */
	bne-	0b    /* if reservation lost, try again */

	# If the old count is as expected, then we are successful:	
        b	_fastlockSuccess

_fastRetryFailed:
_fastReentryFailed:
_lockRecordNotAvailable:
        /* Let interpreter do the hard stuff: */
        mr	PREV, JFP
        b	letInterpreterDoInvoke

#undef OBJ
#undef OBITS
#undef LOCKREC
#undef OBITS0
#undef EXPECTED_CNT
#undef NEW_COUNT
#undef OLD_COUNT

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
        b       letInterpreterDoInvoke
#else

        /* CVM_MICROLOCK_SWAP_SPINLOCK version of
	   CVMCCMinvokeNonstaticSyncMethodHelper: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

        /*
	   r0 = scratch
           r4 = &microlock
           r5 = obj
           r6 = obits
           r7 = lockrec
	   r9 = scratch
        */

#define MICROLOCK   r4
#define OBJ         r5
#define OBITS       r6
#define LOCKREC     r7

	lwz	OBJ, 0(r4)	/* get object to sync on. */
	stw	MB, OFFSET_CVMFrame_mb(NEW_JFP)
	/* optimistically store receiver object */
	stw	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(NEW_JFP)

        /* ld      MICROLOCK, CVMobjGlobalMicroLockPtr */
	lis	MICROLOCK, HA16(CVMobjGlobalMicroLockPtr)
	lwz	MICROLOCK, LO16(CVMobjGlobalMicroLockPtr)(MICROLOCK)

        /* Acquire the microlock: */
        li	r0, CVM_MICROLOCK_LOCKED 
0:	
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	lwarx	r9, 0, MICROLOCK
	stwcx.	r0, 0, MICROLOCK
	bne-	0b

	cmpwi	r9, CVM_MICROLOCK_UNLOCKED	/* See if we succeeded. */
        bne-	_fastLockAcquireMicrolock	/*  Branch if failed */

        /* The microlock has been acquired: */
_lockObj:
        lwz	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, OBITS, 0x3
        cmpwi	r0, CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne-	_objAlreadyLocked

        /* If we get here, then the object has not been locked yet. */
        /* lockrec = ee->objLocksFreeOwned: */
        lwz	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        cmpwi	LOCKREC, 0
        beq-	_lockRecordNotAvailable

        /* lockrec->u.fast.bits = obits: */
        stw	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

#ifdef CVM_DEBUG
        /* lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED: */
        li	r9, CONSTANT_CVM_OWNEDMON_OWNED
        stw	r9, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif
        /* obj->hdr.various32 = lockrec: */
        stw	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ)

        /* lockrec->count = 1: */
        li	r9, 1		/*  Initial lock re-entry count */
        stw	r9, OFFSET_CVMOwnedMonitor_count(LOCKREC)

        /* lockrec->object = obj: */
        stw	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

        /* Release the microlock:	 */
        li	r9, CVM_MICROLOCK_UNLOCKED
        stw	r9, 0(MICROLOCK)	/* microlock->lockWord = UNLOCKED */

        /* Remove lockrec from the ee's free list: */
        /* nextRec = lockrec->next: */
        lwz	r9, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        /* ee->objLocksFreeOwned = nextRec: */
        stw	r9, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        /* Add the lockrec to the ee's owned list: */
        /* nextRec = ee->objLocksOwned: */
        lwz	r9, OFFSET_CVMExecEnv_objLocksOwned(EE)
        /* lockrec->next = nextRec: */
        stw	r9, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        /* ee->objLocksOwned = lockrec: */
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)

        b	_fastlockSuccess

_fastLockAcquireMicrolock:
        /* Call a C function to acquire the microlock: */
	mflr	r0
        stw	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        stw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
	mr	r3, MICROLOCK
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lwz	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
	mtlr	r0
        lwz	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        lwz	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(NEW_JFP)
        lwz	MB, OFFSET_CVMFrame_mb(NEW_JFP) /* restore MB */
        b	_lockObj

_objAlreadyLocked:
        cmpwi	r0, CONSTANT_CVM_LOCKSTATE_LOCKED
        bne-	_fastReentryFailed

        /* Make sure the current thread owns this lock: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(OBITS)
        cmpw	r0, EE
        bne-	_fastReentryFailed

        lwz	r9, OFFSET_CVMOwnedMonitor_count(OBITS)
        addi	r9, r9, 1
        stw	r9, OFFSET_CVMOwnedMonitor_count(OBITS)

        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)	/* microlock->lockWord = UNLOCKED */

        b	_fastlockSuccess

_fastReentryFailed:
_lockRecordNotAvailable:
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)

_fastTryLockFailed:
        b       letInterpreterDoInvoke

#undef MICROLOCK
#undef OBJ
#undef OBITS
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

_fastlockSuccess:
        mr	PREV, JFP
        mr	JFP, NEW_JFP

	# compiled frame
#ifdef CVM_DEBUG_ASSERTS
	li	r0, CONSTANT_CVM_FRAMETYPE_NONE
	stb	r0, OFFSET_CVMFrame_type(JFP)
	li	r0, -1
	stb	r0, OFFSET_CVMFrame_flags(JFP)
#endif
	stw	PREV, OFFSET_CVMFrame_prevX(JFP)

        # set up registers 
        # see about stack frame requirements. 

#ifdef CVM_TRACE
	mflr	r0
	stw	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mr	r3, EE		/* arg1 = EE */
	mr	r4, JFP		/* arg2 = JFP */
	bl	SYM_NAME(CVMCCMtraceMethodCallGlue)
	lwz	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mtlr	r0
#endif

ENTRY( CVMPPCgcPatchPointAtInvoke ) 
CVMPPCgcPatchPointAtInvokeLocal:
	# GC check - gc will patch at this location when a rendezvous is
	# needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	# be a "b CVMPPChandleGCAtInvoke"
        blr	/* Return to method after handling possible GC request */
	
ENTRY( CVMPPChandleGCAtInvoke)	
	#
	# At this point a GC is requested.
	#

#define ORIG_LR		r14

	mflr	ORIG_LR
	
	FIXUP_FRAMES_0(JFP, r0)

	# We will be gc safe soon. Prevent this method from being decompiled
	lwz	r4, OFFSET_CVMFrame_mb(JFP)	/* r4 = mb */
	stw	r4, OFFSET_CVMExecEnv_invokeMb(EE)

	# Check if this is a synchronized invocation
	# If it is, we have to stash the receiver in the
	# newly pushed frame into a safe location. The new frame is not
	# yet "committed" to the stack, and as such, cannot be located
	# by GC.
        lhz	r0, OFFSET_CVMMethodBlock_invokerAndAccessFlagsX(r4)
        andi.	r0, r0, CONSTANT_METHOD_ACC_SYNCHRONIZED
	
	# Synchronized method if result of 'tst' is 'ne'. Stash
	# receiver in [ee->miscICell]
	beq-	notSync
	lwz	r4, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
	lwz	r5, OFFSET_CVMExecEnv_miscICell(EE)	/* r5 = &miscICell */
	stw	r4, 0(r5)	/* stash in miscICell */
notSync:
	
        stw	PREV, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	mr	r3, EE
	la	r4, OFFSET_CVMGlobalState_cstate_GCSAFE(CVMGLOBALS)
	la	r5, OFFSET_CVMExecEnv_tcstate_GCSAFE(EE)
	li	r6, 1   /* block */
	CALL_VM_FUNCTION(CVMcsRendezvous)

	# reload the ee and mb
	lwz	r4, OFFSET_CVMFrame_mb(JFP)	/* r4 = mb */

	# we no longer need to prevent the method from being decompiled
	li	r0, 0
	stw	r0, OFFSET_CVMExecEnv_invokeMb(EE)

	mtlr	ORIG_LR
	
	/*
	 * We've returned from the GC. Check for a sync method
	 * again to see if we should restore 'receiverObjX'
	 * from miscICell.
	 */
        lhz	r0, OFFSET_CVMMethodBlock_invokerAndAccessFlagsX(r4)
        andi.	r0, r0, CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq	CVMPPCgcPatchPointAtInvokeLocal

	# Restore receiverObjX in new frame
	lwz	r5, OFFSET_CVMExecEnv_miscICell(EE)	/* r5 = &miscICell */
	lwz	r4, 0(r5)
	stw	r4, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
	
	# And clear miscICell for other uses
	li	r4, 0
	stw	r4, 0(r5)

	b	CVMPPCgcPatchPointAtInvokeLocal


#undef MB
#undef ORIG_LR

SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

ENTRY( CVMCCMinvokeStaticSyncMethodHelper ) 
        # r3 = MB = target mb

#define MB r3
#define CB r12

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just to branch to (and implement) letInterpreterDoInvoke.
	#
        b       letInterpreterDoInvoke
#endif

#ifdef CVM_METHODBLOCK_HAS_CB
	lwz	CB, OFFSET_CVMMethodBlock_cbX(MB)	/* get the cb */
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        lbz	r5, OFFSET_CVMMethodBlock_methodIndexX(MB)

	#
	# r6 = 28*r5
	# r8 = MB-r6
	#
	mulli	r6, r5, CONSTANT_CVMMethodBlock_size
        sub	r8, MB, r6
        lwz	CB, -OFFSET_CVMMethodRange_mb(r8)	/* get the cb */
#endif

	#
	# r4 needs to be set to the icell of the object to lock
	#
	lwz	r4, OFFSET_CVMClassBlock_javaInstanceX(CB)
	b	SYM_NAME(CVMCCMinvokeNonstaticSyncMethodHelper)
#undef MB
#undef CB
SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

ENTRY( CVMCCMinvokeCNIMethod ) 
        # r3 = target mb
#define MB   r14
#define ARGS r15   /* pointer to args */

	mr	MB, r3		/* save MB */

	# setup ARGS and schedule some other instructions in between
	lbz	ARGS, OFFSET_CVMMethodBlock_argsSizeX(MB)
	mflr	r7
	slwi	ARGS, ARGS, 2
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP)
        sub	ARGS, JSP, ARGS

	FIXUP_FRAMES(JFP, r7)

        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)

#ifdef CVM_TRACE
	# trace call
	mr	r3, EE		/* arg1 */
	mr	r4, JFP		/* arg2 */
	mr	r5, MB		/* arg3 */ 
	CALL_VM_FUNCTION(CCMtraceFramelessMethodCall)
#endif

	# invoke the method
        lwz	r0, OFFSET_CVMMethodBlock_codeX(MB)
	mr	r3, EE		/* arg1 */
	mtlr	r0
	/* arg3 == mbPtr */
	la	r5, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mr	r4, ARGS        /* arg2 */
	stw	MB, 0(r5)	/* store MB into mbPtr */
	blrl

#ifdef CVM_TRACE
	# trace return
	mr	r16, r3		/* save result in non-volatile register */
	mr	r3, EE		/* arg1 */
	mr	r4, MB		/* arg2 */
	mr	r5, JFP		/* arg3 */
	CALL_VM_FUNCTION(CCMtraceFramelessMethodReturn)
	mr	r3, r16		/* restore result */
#endif

	/* if r3 >= 0, then r3 is the size in words of the method result */
	cmpwi	r3, 0
	blt-	check_for_new_mb
        lwz	r0, OFFSET_CVMCompiledFrame_PC(JFP)
	slwi	r3, r3, 2
	mtlr	r0
	add	JSP, ARGS, r3	/* pop args and adjust for result */
	blr

	# check if a new mb to execute has been returned
check_for_new_mb:
	cmpwi	r3, CONSTANT_CNI_NEW_MB
	bne	check_for_new_transition
	# load new MB
	lwz	r3, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	# adjust TOS. The new method may have fewer args than the CNI method
        lbz	r4, OFFSET_CVMMethodBlock_argsSizeX(r3)  /* r3 = argsSize */
	slwi	r4, r4, 2
        add	JSP, ARGS, r4	/* adjust TOS past args */
        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	b	returnToInterpreter1

	# check if a new transition frame to execute has been setup
check_for_new_transition:	
	cmpwi	r3, CONSTANT_CNI_NEW_TRANSITION_FRAME
	bne-	returnToInterpreter	/* an exception has occurred */
        stw	ARGS, OFFSET_CVMFrame_topOfStack(JFP)    /* pop args */
	b	returnToInterpreter0
#undef MB
#undef ARGS
        SET_SIZE( CVMCCMinvokeCNIMethod ) 

ENTRY( CVMCCMinvokeJNIMethod ) 
        # r3 = target mb
	
	mflr	r7
	mr	r4, r3		/* arg3 == mb */
	mr	r3, EE		/* arg1 == ee */
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) 

	FIXUP_FRAMES(JFP, r7)

	# flush state
        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)

	CALL_VM_FUNCTION(CVMinvokeJNIHelper)

	lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	cmpwi	r3, 0		/* check for exception */
	mtlr	r7
        lwz	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	bnelr+			/* return  if no exception */
	b	returnToInterpreter0	/* interpreter handles exception */

        SET_SIZE( CVMCCMinvokeJNIMethod ) 
/*
 * Invoke a virtual method
 * 
 * This function is used for doing a true virtual invocation in the patched 
 * method invocations implementation.  In particular, it is used if we
 * have emitted a direct branch to a method known not to be overridden during 
 * compilation of the method that invokes it, and later it becomes overridden.
 * When this happens the original direct method call is patched to instead
 * call here so a true virtual invocation can be done.
 */
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
	/* 
 	 *  r3 = invokeMb (not the targetMb) 
 	 */
	ENTRY( CVMCCMinvokeVirtual )
	/* Get "this" by looking back on the stack */
        lbz	r4, OFFSET_CVMMethodBlock_argsSizeX(r3)
        slwi	r4, r4, 2	/* r4 = argSize * 4 */
        sub	r4, JSP, r4	/* r4 = stack address of "this" */
	lwz	r4, 0(r4)	/* r4 = "this" */
	/* get the method table from "this" */
	lwz	r5, OFFSET_CVMObjectHeader_clas(r4)
	clrrwi.	r5, r5, 2	/* mask off low 2 bits of object cb */
	lwz	r5, OFFSET_CVMClassBlock_methodTablePtrX(r5)
	/* get method table index from invokeMb */
	lhz	r6, OFFSET_CVMMethodBlock_methodTableIndexX(r3)
	slwi	r6, r6, 2
	/* get targetMb from method table */
	lwzx	r3, r5, r6	/* mb = *(methodTablePtr + index * 4) */
	/* call the method */
	lwz	r4, OFFSET_CVMMethodBlock_jitInvokerX(r3)
	mtctr	r4	/* use CTR so we don't trash LR */
	bctr	
        SET_SIZE( CVMCCMinvokeVirtual ) 
#endif

ENTRY( CVMCCMletInterpreterDoInvoke )
letInterpreterDoInvoke_store_lr:
	mflr	r0
        stw	r0, OFFSET_CVMCompiledFrame_PC(JFP)

ENTRY( CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr )
letInterpreterDoInvoke:
        /* 
         * Trying to invoke something beyond our ability. 
         * Return the mb to the interpreter and let it do the 
         * dirty work. 
         * we have already set up the return PC in our own frame 
         * We need to set topOfStack then return the target MB* 
         * as a C return value, which is already in r3.
         */ 
	FIXUP_FRAMES(JFP, r0)
        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	b	returnToInterpreter1
/*
 * Do a GC check, and rendezvous if one is requested
 */
handleGCForReturn:
	#
	# At this point a GC is requested.
	#

	FIXUP_FRAMES(JFP, r0)

        # 
        # Special flag that signals we are handling gc for return. 
        # Used by CVMcompiledFrameScanner. 
        # 
	li	r0, CONSTANT_HANDLE_GC_FOR_RETURN
	stw	r0, OFFSET_CVMCompiledFrame_PC(JFP)
	
	# We will be gc safe soon. Prevent this method from being decompiled
	lwz	r0, OFFSET_CVMFrame_mb(JFP)	/* r0 = mb */
	stw	r0, OFFSET_CVMExecEnv_invokeMb(EE)

	# save state
	stw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	
	# Fill in the arguments to CVMcsRendezvous	
	mr	r3, EE
	la	r4, OFFSET_CVMGlobalState_cstate_GCSAFE(CVMGLOBALS)
	la	r5, OFFSET_CVMExecEnv_tcstate_GCSAFE(EE)
	li	r6, 1   /* block */
	CALL_VM_FUNCTION(CVMcsRendezvous)

	#
	# GC done.
	#

	# we no longer need to prevent the method from being decompiled
	li	r0, 0
	stw	r0, OFFSET_CVMExecEnv_invokeMb(EE)

	# This is expected at the beginning of returns
        lwz     PREV, OFFSET_CVMFrame_prevX(JFP)

	# Return to caller
	mtlr	r14
	blr

/*
 * The GC checks for the various return variants
 */
ENTRY( CVMPPChandleGCForReturnFromMethod )
	lis	r14, HA16(CVMCCMreturnFromMethod)
	la	r14, LO16(CVMCCMreturnFromMethod)(r14)
	b	handleGCForReturn

ENTRY( CVMPPChandleGCForReturnFromSyncMethod )
	lis	r14, HA16(CVMCCMreturnFromSyncMethod)
	la	r14, LO16(CVMCCMreturnFromSyncMethod)(r14)
	b	handleGCForReturn

/* 
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 *
 * CVMMethodBlock* CVMCCMreturnFromMethod();
 */ 
ENTRY( CVMCCMreturnFromMethod ) 
	# GC check - gc will patch at this location when a rendezvous is
	# needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	# be a "b CVMPPChandleGCForReturnFromMethod"

	# NOTE:	lots of instruction schduling stalls here, but I am not
	# sure if much can be done about them.

	#
	# see if previous frame is compiled or not
	# PREV is set up by all code that branches here
	#
        andi.	r0, PREV, CONSTANT_CVM_FRAME_MASK_SLOW
	bne-	returnToInterpreter

doReturnToCompiled:

#ifdef CVM_TRACE
	mr	r3, EE    /* arg1 */
	mr	r4, JFP   /* arg2 */
	CALL_VM_FUNCTION(CCMtraceMethodReturn)
#endif /* CVM_TRACE */

	# setup JFP
	clrrwi	JFP, PREV, 2 /* mask with ~CONSTANT_CVM_FRAME_MASK_ALL  */

        # returning from one native to another. 
        # do this ourselves. 
	# java sp already set
        lwz	r0, OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r0
#ifdef CVMCPU_HAS_CP_REG
	lwz	CVMPPC_CP_REGNAME, OFFSET_CVMCompiledFrame_cpBaseRegX(JFP)
#endif
	blr

ENTRY( CVMCCMreturnToInterpreter )
returnToInterpreter:
	FIXUP_FRAMES_0(JFP, r0)

	# JSP needs to point just past any return value
        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP)

        # set stack->currentFrame to current value of JFP, 
        # then return NULL, meaning we do not want the interpreter 
        # to take any further action on our behalf (except pop
	# the current frame)
        stw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
returnToInterpreter0:
        li	r3, 0
returnToInterpreter1:
	# The following code is pretty much the same as what we do
	# in CVMJITexitNative.
	lwz	r0, OFFSET_CStack_SavedLR(sp)      /* get return address */
	lwz	r4, CVMCCExecEnv_ccmStorage_CR(sp) /* restore CR */
	lmw	r14, OFFSET_CStack_SavedGPRs(sp)   /* restore NV registers */
	mtcr	r4
	mtlr	r0		   /* put return address in lr */
	addic	sp, sp, CStack_FrameSize  /* pop CVMJITgoNative frame */
	blr			   /* return */
        SET_SIZE( CVMCCMreturnFromMethod )

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
ENTRY( CVMCCMreturnFromSyncMethod )
	# GC check - gc will patch at this location when a rendezvous is
	# needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	# be a "b CVMPPChandleGCForReturnFromSyncMethod"
#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just to branch to (and implement) returnToInterpreter.
	#
        b       returnToInterpreter
#endif

	
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

        /* CAS version of CVMCCMreturnFromSyncMethod: */

#define OBJ		r5
#define NBITS		r6
#define LOCKREC		r7
#define OBITS0		r8
#define EXPECTED_CNT	r9
#define NEW_COUNT	r10
#define OLD_COUNT	r11

	#
        # see if previous frame is compiled or not 
	# PREV is set up by all code that branches here
	#
        andi.	r0, PREV, CONSTANT_CVM_FRAME_MASK_SLOW
	bne-	returnToInterpreter

        /* Do fastTryUnlock(): */

	/* load the sync object */
        lwz	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(JFP)

        /* Check to see if the object is locked with a fastlock: */
        lwz	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne-	_fastTryUnlockFailed /* If not, we failed. */

	/* If the LOCKREC is NULL, it is currently being inflated */
	cmpwi	LOCKREC, 0
	beq-	_fastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        cmpw	r0, EE
        bne-	_fastTryUnlockFailed	/* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lwz	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	cmpwi	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq-	_fastTryUnlockFailed
        subic.	NEW_COUNT, EXPECTED_CNT, 1
        beq+	_doUnlock	/* If zero, then unlock */

	/* new monitor count > 0, so just update it and we're done */
	la	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
0:
	lwarx	OLD_COUNT, 0, r0	/* OLD_COUNT = old count */
	cmpw	OLD_COUNT, EXPECTED_CNT	/* make sure it didn't inflate. */
        bne-	_fastTryUnlockFailed
	stwcx.	NEW_COUNT, 0, r0	/* update with NEW_COUNT */
	bne-	0b    /* if reservation lost, try again */

	/* we're done! monitor count was successfully decrement */
	b	doReturnToCompiled
        /* End. */

_doUnlock:	
#define OBITS LOCKREC
        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lwz	NBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
	la	r0, OFFSET_CVMObjectHeader_various32(OBJ)
0:
	lwarx	OBITS0, 0, r0		/* OBITS0 = old various32 */
	cmpw	OBITS0, OBITS		/* check if inflated. */
	bne-	_fastTryUnlockFailed /* let C helper do hard work */
	stwcx.	NBITS, 0, r0		/* store NBITS in various32 */
	bne-	0b    /* if reservation lost, try again */
#undef OBITS

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
        li	r0, CONSTANT_CVM_OWNEDMON_FREE
        stw	r0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
        li	r0, 0
        stw	r0, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        stw	r0, OFFSET_CVMOwnedMonitor_object(LOCKREC)
	stw	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
#endif

        /* Check if the lockrec is the first one on the thread's owned list: */
        lwz	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)
        cmpw	r0, LOCKREC
        bne-	_fastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee's owned list: */
        lwz	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)

_fastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lwz	r0, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        stw	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

	/* we're done! lock was successfully released */
	b	doReturnToCompiled

/* r0 is the first one on the thread's owned list */
/* It is safe to reuse NBITS at this point */
#define PREV_REC NBITS
_fastTryUnlockFindPrevLockRecord:
	mr	PREV_REC, r0
        lwz	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        cmpw	r12, LOCKREC
        beq-	_fastTryUnlockFoundPrevLockRecord
        mr	PREV_REC, r12
        b	_fastTryUnlockFindPrevLockRecord

_fastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list:  */
        lwz	r12, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_fastTryUnlockAddLockRecordToFreeList
#undef PREV_REC

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

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
	b	returnToInterpreter
#else

        /* CVM_MICROLOCK_SWAP_SPINLOCK microlock version: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

        /* Do fastTryUnlock(): */

        /*
	   r0 = scratch
           r3 = &microlock
           r5 = obj
           r7 = lockrec
	   r9 = scratch

           NOTE: By design, MICROLOCK is assigned to r3 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _fastUnlockAcquireMicrolock.
        */

#define MICROLOCK   r3
#define OBJ         r5
#define LOCKREC     r7

	/* See if previous frame is compiled or not. */
	/* PREV is set up by all code that branches here */
        andi.	r0, PREV, CONSTANT_CVM_FRAME_MASK_SLOW
	bne-	returnToInterpreter

	/* load the sync object */
        lwz	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(JFP)

        /* ld      MICROLOCK, =CVMobjGlobalMicroLockPtr */
	lis	MICROLOCK, HA16(CVMobjGlobalMicroLockPtr)
	lwz	MICROLOCK, LO16(CVMobjGlobalMicroLockPtr)(MICROLOCK)

        /* Acquire the microlock: */
        li	r0, CVM_MICROLOCK_LOCKED 
0:	
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	lwarx	r9, 0, MICROLOCK
	stwcx.	r0, 0, MICROLOCK
	bne-	0b

	cmpwi	r9, CVM_MICROLOCK_UNLOCKED	/* See if we succeeded. */
        bne-	_fastUnlockAcquireMicrolock	/*  Branch if failed */

        /* The microlock has been acquired: */
_unlockObj:
        /* Check to see if the object is locked with a fastlock: */
        lwz	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne-	 _fastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        cmpw	r0, EE
        bne-	_fastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lwz	r9, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	subic.	r9, r9, 1
	bne-	_fastTryUnlockSuccess	/* If not zero, we are done */

        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lwz	r0, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        stw	r0, OFFSET_CVMObjectHeader_various32(OBJ)

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
        li	r0, CONSTANT_CVM_OWNEDMON_FREE
        stw	r0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
        li	r0, 0
        stw	r0, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        stw	r0, OFFSET_CVMOwnedMonitor_object(LOCKREC)
	stw	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
#endif

        /* Release the microlock: */
        li	r0, 0
        stw	r0, 0(MICROLOCK)  /* *microlock = CVM_MICROLOCK_UNLOCKED */

        /* Check if the lockrec is the first one on the thread owned list: */
        lwz	r9, OFFSET_CVMExecEnv_objLocksOwned(EE)
        cmpw	r9, LOCKREC
        bne-	 _fastTryUnlockFindPrevLockRecordLoop

        /* Remove the lockrec from the ee owned list: */
        lwz	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)

_fastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lwz	r9, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        stw	r9, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

_fastTryUnlockDone:
        b       doReturnToCompiled


/* r9 is the first one on the thread's owned list */
#define PREV_REC r9
_fastTryUnlockFindPrevLockRecordLoop:
        lwz	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        cmpw	r12, LOCKREC
        beq-	_fastTryUnlockFoundPrevLockRecord
        mr	PREV_REC, r12
        b	_fastTryUnlockFindPrevLockRecordLoop

_fastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */
        lwz	r12, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_fastTryUnlockAddLockRecordToFreeList
#undef PREV_REC


_fastTryUnlockSuccess:
        /* Set the new re-entry count: */
        stw	r9, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)  /* *microlock = CVM_MICROLOCK_UNLOCKED */

        b       _fastTryUnlockDone

_fastUnlockAcquireMicrolock:
        /* Call a C function to acquire the microlock: */
        stw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        /* CVMmicrolockLockImpl() requires the address of the microlock in
           r3.  By design, MICROLOCK is r3.  so, we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lwz	OBJ, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
        lwz	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
	lwz	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ)
        b       _unlockObj	/* Go unlock the object if possible */

_fastTryUnlockFailed:
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)

        /* Let the interpreter handle the hard cases: */
        b       returnToInterpreter

#undef OBJ
#undef MICROLOCK
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

SET_SIZE( CVMCCMreturnFromSyncMethod )

#ifdef CVM_TRACE

ENTRY(CVMCCMtraceMethodCallGlue)
	lwz	r0, OFFSET_CVMGlobalState_debugFlags(CVMGLOBALS)
	andi.	r0, r0, CONSTANT_TRACE_METHOD
	beqlr
	mflr	r7
	stw	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	FIXUP_FRAMES(JFP, r7)
	li	r5, 0	/* isJump arg */
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodCall)
SET_SIZE(CVMCCMtraceMethodCallGlue)

ENTRY(CCMtraceMethodReturn)
	lwz   r0, OFFSET_CVMGlobalState_debugFlags(CVMGLOBALS)
	andi. r0, r0, CONSTANT_TRACE_METHOD
	beqlr
	mflr	r7
	FIXUP_FRAMES(JFP, r7)
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodReturn)
SET_SIZE(CCMtraceMethodReturn)

ENTRY(CCMtraceFramelessMethodCall)
	lwz	r0, OFFSET_CVMGlobalState_debugFlags(CVMGLOBALS)
	andi.	r0, r0, CONSTANT_TRACE_METHOD
	beqlr
	mflr	r7
	FIXUP_FRAMES(JFP, r7)
	li	r6, 0	/* isJump arg */
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodCall)
SET_SIZE(CCMtraceFramelessMethodCall)

ENTRY(CCMtraceFramelessMethodReturn)
	lwz   r0, OFFSET_CVMGlobalState_debugFlags(CVMGLOBALS)
	andi. r0, r0, CONSTANT_TRACE_METHOD
	beqlr
	mflr	r7
	FIXUP_FRAMES(JFP, r7)
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodReturn)
SET_SIZE(CCMtraceFramelessMethodReturn)

#endif
