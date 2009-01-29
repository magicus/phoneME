/*
 * @(#)ccminvokers_cpu.S	1.41 06/10/10
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

/* 
 * Helpers for invoking from and returning from compiled methods. 
 */ 

/* 
 * The native code has computed all necessary Java parameters 
 * onto the Java stack, has computed the target mb into %o0, 
 * and calls us. We still have access to the JFP and JSP in the v registers. 
 */
#define MB  %o0

#define NEW_JFP	%CVMSPARC_NEWJFP_REGNAME
#define PREV	%CVMSPARC_PREVFRAME_REGNAME

ENTRY( CVMCCMinvokeNonstaticSyncMethodHelper )
	! %o0 = target mb
	! %o1 = CVMObjectICell* of object to sync on
#if 0
	! If you just want to call the C helper and write very little assemble
	! code, then just to branch to (and implement) letInterpreterDoInvoke.
	!
	call	letInterpreterDoInvoke
        nop
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

        /* CAS version of CVMCCMinvokeNonstaticSyncMethodHelper: */

#define OBJ		%o1
#define OBITS		%o2
#define LOCKREC		%o3
#define OBITS0		%o4
#define NBITS		%o5

	ld	[%o1], OBJ	/* get object to sync on. */
	st	MB, [NEW_JFP + OFFSET_CVMFrame_mb]
	! optimistically store receiver object
	st	OBJ, [NEW_JFP + OFFSET_CVMCompiledFrame_receiverObjX]

	/* Do fastTryLock(): */

	/* Setup a lock record and assume the object has not been locked
	   yet:	 */
	! lockrec = ee->objLocksFreeOwned:
	ld	[EE + OFFSET_CVMExecEnv_objLocksFreeOwned], LOCKREC
	cmp	LOCKREC, 0
	beq     _lockRecordNotAvailable
	nop

	! lockrec->object = obj:
	st	OBJ, [LOCKREC + OFFSET_CVMOwnedMonitor_object]

	! lockrec->count = 1:
	mov	1, %g1		/* Initial lock re-entry count */
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_count]

#ifdef CVM_DEBUG
	! lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED:
	mov	CONSTANT_CVM_OWNEDMON_OWNED, %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
#endif

	! nbits = (LOCKREC) | CVM_LOCKSTATE_LOCKED:	
	! Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
	! that nbits is essentially lockrec.  Nothing need to be done to
	! initialize nbits.
	mov	LOCKREC, NBITS

	! obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED:
	ld	[OBJ + OFFSET_CVMObjectHeader_various32], OBITS0 /* Get obits */
	andn	OBITS0, 0x3, OBITS	/* clear rightmost 2 bits */
	or	OBITS, CONSTANT_CVM_LOCKSTATE_UNLOCKED, OBITS

	! lockrec->u.fast.bits = obits:
	st	OBITS, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]

	! Do atomicCompareAndSwap:
	! OBITS0 = [OBJ, #OFFSET_CVMObjectHeader_various32]
	! if ([OBJ, #OFFSET_CVMObjectHeader_various32] == NBITS) {
	!     [OBJ, #OFFSET_CVMObjectHeader_various32] = LOCKREC 
	! }
	add	OBJ, OFFSET_CVMObjectHeader_various32, %g1 /* address of various32 */
	cas	[%g1], OBITS, NBITS
	cmp	OBITS, NBITS
	bne	_objAlreadyLocked
	nop

	/* Remove lockrec from the ee's free list: */
	! nextRec = lockrec->next:
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_next], %g1
	! ee->objLocksFreeOwned = nextRec:
	st	%g1, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

	/* Add the lockrec to the ee's owned list: */
	! nextRec = ee->objLocksOwned:
	ld	[EE + OFFSET_CVMExecEnv_objLocksOwned], %g1
	! lockrec->next = nextRec:
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
	! ee->objLocksOwned = lockrec:
	st	LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksOwned]
	b	_fastlockSuccess	/* we are all done locking */
	nop

_objAlreadyLocked:
#ifdef CVM_DEBUG
	! lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
	st	%g0, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
	mov	CONSTANT_CVM_OWNEDMON_FREE, %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
#endif

	! If object is not in LOCKED state, then fail:
	btst    0x3, OBITS0     /* check for CVM_LOCKSTATE_LOCKED */
	bne	_fastRetryFailed
	nop
	
	! If not associated with a lock record, then fail:
	andncc	OBITS0, 0x3, LOCKREC
	beq	_fastReentryFailed
	nop

	! If (lockrec->owner != ee), then fail:	
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_owner], %g1
	cmp	%g1, EE
	bne	_fastReentryFailed
	nop

#define EXPECTED_CNT    OBITS0
#define NEW_COUNT       NBITS

	! If we get here, then we are re-entering the lock:
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_count], EXPECTED_CNT
	cmp	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	_fastReentryFailed
	nop

	add	EXPECTED_CNT, 1, NEW_COUNT
	add	LOCKREC, OFFSET_CVMOwnedMonitor_count, %g1 /* addr of count */
	cas	[%g1], EXPECTED_CNT, NEW_COUNT
	cmp	EXPECTED_CNT, NEW_COUNT
	! If the old count is as expected, then we are successful:	
	beq	_fastlockSuccess
	nop

#undef EXPECTED_CNT
#undef NEW_COUNT

_fastRetryFailed:
_fastReentryFailed:
_lockRecordNotAvailable:
	/* Let interpreter do the hard stuff: */
	mov	JFP, PREV
	b	letInterpreterDoInvoke
	nop

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
	call	letInterpreterDoInvoke
        nop
#else

        /* CVM_MICROLOCK_SWAP_SPINLOCK version of
	   CVMCCMinvokeNonstaticSyncMethodHelper: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

#define OBJ         %o1
#define OBITS       %o3
#define MICROLOCK   %o4
#define LOCKREC     %g1
#define TEMP        %o5
#define TEMP2       %o6

        set     CVMobjGlobalMicroLockPtr, MICROLOCK
        st      MB, [NEW_JFP + OFFSET_CVMFrame_mb]

        ! Schedule r8 = &microLock early
        ld      [MICROLOCK], MICROLOCK
        ld      [OBJ], OBJ              ! get object to sync on.
        ld      [MICROLOCK], TEMP

        ! optimistically store receiver object:
        st      OBJ, [NEW_JFP + OFFSET_CVMCompiledFrame_receiverObjX]

        /* Do fastTryLock(): */

        /* Acquire the microlock: */
        set     CVM_MICROLOCK_LOCKED, TEMP  ! Swap CVM_MICROLOCK_LOCKED into
        swap    [MICROLOCK], TEMP           !    the lockWord.

        ! Get obits. INVARIANT: All branches and fallthroughs to _lockObj
        ! have to set up OBITS first

        ld      [OBJ + OFFSET_CVMObjectHeader_various32], OBITS
        cmp     TEMP, CVM_MICROLOCK_UNLOCKED    ! See if we succeeded.
        bne     _fastLockAcquireMicrolock       ! Branch if failed.
        nop

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
        set     fastMlockimplCount, TEMP
        ld      [TEMP], TEMP2
        add     TEMP2, 1, TEMP2
        st      TEMP2, [TEMP]
#endif

        /* The microlock has been acquired: */
_lockObj:
        and     OBITS, 0x3, %g1
        cmp     %g1, CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne     _objAlreadyLocked
        nop

        /* If we get here, then the object has not been locked yet. */
        ! lockrec = ee->objLocksFreeOwned:
        ld      [EE + OFFSET_CVMExecEnv_objLocksFreeOwned], LOCKREC
        set     1, TEMP                 ! Initial lock re-entry count.
        cmp     LOCKREC, 0
        beq     _lockRecordNotAvailable
        nop

#ifdef CVM_DEBUG
        ! lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED:
        set     CONSTANT_CVM_OWNEDMON_OWNED, %o2
        st      %o2, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
#endif
        ! obj->hdr.various32 = lockrec:
        st      LOCKREC, [OBJ + OFFSET_CVMObjectHeader_various32]
        ! lockrec->count = 1: (TEMP initialized above)
        st      TEMP, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
        ! lockrec->u.fast.bits = obits:
        st      OBITS, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]

        ! lockrec->object = obj:
        st      OBJ, [LOCKREC + OFFSET_CVMOwnedMonitor_object]

        ! Release the microlock:
        st      %g0, [MICROLOCK]        ! microlock->lockWord = UNLOCKED.

        /* Remove lockrec from the ee's free list: */
        ! nextRec = lockrec->next:
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], TEMP
        /* Add the lockrec to the ee's owned list: */
        ! nextRec2 = ee->objLocksOwned:
        ld      [EE + OFFSET_CVMExecEnv_objLocksOwned], %o1

        ! ee->objLocksFreeOwned = nextRec:
        st      TEMP, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

        ! lockrec->next = nextRec2:
        st      %o1, [LOCKREC + OFFSET_CVMOwnedMonitor_next]

        ! ee->objLocksOwned = lockrec:
        st      LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksOwned]

        b	_fastlockSuccess

_fastLockAcquireMicrolock:
        st      %o7, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0]
        st      MICROLOCK, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4]
        /* Call a C function to acquire the microlock: */
        mov     MICROLOCK, %o0
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        nop
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0], %o7
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4], MICROLOCK
        ld      [NEW_JFP + OFFSET_CVMCompiledFrame_receiverObjX], OBJ
        ld      [OBJ + OFFSET_CVMObjectHeader_various32], OBITS ! Get obits.
	ld      [NEW_JFP + OFFSET_CVMFrame_mb], MB
	b       _lockObj
        nop

_objAlreadyLocked:
        cmp     %g1, CONSTANT_CVM_LOCKSTATE_LOCKED
        bne     _fastReentryFailed
        nop

        /* Make sure the current thread owns this lock: */
        ld      [OBITS + OFFSET_CVMOwnedMonitor_owner], TEMP
        ! Optimistically load count
        ld      [OBITS + OFFSET_CVMOwnedMonitor_count], %g1
        ! Are we owner?
        cmp     TEMP, EE
        bne     _fastReentryFailed

        add     %g1, 1, %g1     ! count++
        st      %g1, [OBITS + OFFSET_CVMOwnedMonitor_count]

        /* Release the microlock: */
        st      %g0, [MICROLOCK]            ! microlock->lockWord = UNLOCKED.
        b       _fastlockSuccess
        nop

_fastReentryFailed:
_lockRecordNotAvailable:
        /* Release the microlock: */
        st      %g0, [MICROLOCK]        ! *microlock = CVM_MICROLOCK_UNLOCKED;
        /* Fall through to _fastTryLockFailed. */

_fastTryLockFailed:
        ld      [NEW_JFP + OFFSET_CVMFrame_mb], MB
        b       letInterpreterDoInvoke
        nop

#undef OBJ
#undef OBITS
#undef MICROLOCK
#undef LOCKREC
#undef TEMP
#undef TEMP2

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

_fastlockSuccess:
        mov     JFP, PREV
        mov     NEW_JFP, JFP

        ! compiled frame
#ifdef CVM_DEBUG_ASSERTS
        set     CONSTANT_CVM_FRAMETYPE_NONE, %o3
        stb     %o3, [JFP + OFFSET_CVMFrame_type]
        mov     -1, %o3
        stb     %o3, [JFP + OFFSET_CVMFrame_flags]
#endif
        st      PREV, [JFP + OFFSET_CVMFrame_prevX]

        ! set up registers 
        ! see about stack frame requirements. 

#ifdef CVM_TRACE
        st      %o7, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage]
        mov     EE, %o0		/* arg1 = EE */
        mov     JFP, %o1 	/* arg2 = JFP */
        CALL_VM_FUNCTION(CVMCCMtraceMethodCallGlue)
        nop
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], %o7
        nop
#endif

ENTRY( CVMSPARCgcPatchPointAtInvoke ) 
	! GC check - gc will patch at this location when a rendezvous is
	! needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	! be a "b CVMSPARChandleGCAtInvoke"
        retl	/* Return to method after handling possible GC request */
	nop
	
ENTRY( CVMSPARChandleGCAtInvoke)	
	!
	! At this point a GC is requested.
	!
#if 0
	mflr	ORIG_LR
	
	FIXUP_FRAMES_0(JFP, ORIG_LR)

	! We will be gc safe soon. Prevent this method from being decompiled
	lwz	r4, OFFSET_CVMFrame_mb(JFP)	/* r4 = mb */
	stw	r4, OFFSET_CVMExecEnv_invokeMb(EE)

	! Check if this is a synchronized invocation
	! If it is, we have to stash the receiver in the
	! newly pushed frame into a safe location. The new frame is not
	! yet "committed" to the stack, and as such, cannot be located
	! by GC.
        lduh	r0, OFFSET_CVMMethodBlock_invokerAndAccessFlagsX(r4)
        andi.	r0, r0, CONSTANT_METHOD_ACC_SYNCHRONIZED
	
	! Synchronized method if result of 'tst' is 'ne'. Stash
	! receiver in [ee->miscICell]
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

	! reload the ee and mb
	lwz	r4, OFFSET_CVMFrame_mb(JFP)	/* r4 = mb */

	! we no longer need to prevent the method from being decompiled
	li	r0, 0
	stw	r0, OFFSET_CVMExecEnv_invokeMb(EE)

	mtlr	ORIG_LR
	
	!
	! We have returned from the GC. Check for a sync method
	! again to see if we should restore 'receiverObjX'
	! from miscICell.
	!
        lduh	r0, OFFSET_CVMMethodBlock_invokerAndAccessFlagsX(r4)
        andi.	r0, r0, CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq	SYM_NAME(CVMSPARCgcPatchPointAtInvoke)

	! Restore receiverObjX in new frame
	lwz	r5, OFFSET_CVMExecEnv_miscICell(EE)	/* r5 = &miscICell */
	lwz	r4, 0(r5)
	stw	r4, OFFSET_CVMCompiledFrame_receiverObjX(JFP)
	
	! And clear miscICell for other uses
	li	r4, 0
	stw	r4, 0(r5)

	b	SYM_NAME(CVMSPARCgcPatchPointAtInvoke)
#endif
	SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

ENTRY( CVMCCMinvokeStaticSyncMethodHelper ) 
        ! %o0 = target mb 
#if 0
	! If you just want to call the C helper and write very little assembler
	! code, then just to branch to (and implement) letInterpreterDoInvoke.
	!
        call 	letInterpreterDoInvoke
	nop
#endif

#define CB %l0
#ifdef CVM_METHODBLOCK_HAS_CB
	ld	[MB + OFFSET_CVMMethodBlock_cbX], CB	/* get the cb */
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        ldub	[MB + OFFSET_CVMMethodBlock_methodIndexX], %o1

	!
	! o2 = 28*o1
	! o4 = MB-o2
	!
	umul	%o1, CONSTANT_CVMMethodBlock_size, %o2
        sub	MB, %o2, %o4
	
        ld	[%o4 - OFFSET_CVMMethodRange_mb], CB	/* get the cb */
#endif

	!
	! %o1 needs to be set to the icell of the object to lock
	!
	ld	[CB + OFFSET_CVMClassBlock_javaInstanceX], %o1
	b	SYM_NAME(CVMCCMinvokeNonstaticSyncMethodHelper)
	nop
#undef CB
	SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

#undef OBJ
#undef OBITS
#undef LOCKREC
#undef OBITS0
#undef NBITS

ENTRY( CVMCCMinvokeCNIMethod ) 
        ! %o0 = target mb

        add	%o7, 8, %o7	! Return address = call address + 8
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC]	! Save return address
        ldub	[MB + OFFSET_CVMMethodBlock_argsSizeX], %o1
        sll	%o1, 2, %o1
        sub	JSP, %o1, %o1	! TOS

#undef MB
#define MB   %l0
#define ARGS %l1
	mov	%o0, MB		! save MB
	mov	%o1, ARGS	! save args ptr

	/* although %o1 is now in ARGS, we still want to preserve it */
	FIXUP_FRAMES_1(JFP, %g1, %o1)

        st	JSP, [JFP + OFFSET_CVMFrame_topOfStack] 
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 

#ifdef CVM_TRACE
	! trace call
	mov	JFP, %o1
	mov	MB, %o2		! %o2 = mb
	CALL_VM_FUNCTION(CCMtraceFramelessMethodCall)
	mov	EE, %o0		! %o0 = ee

	mov	ARGS, %o1	! restore args ptr
#endif

	! invoke the method  - %o1 is still the args pointer
	! %o2 is the mbPtr
        ld	[MB + OFFSET_CVMMethodBlock_codeX], %o7
	add	%sp, MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage, %o2    ! %o2 = mbPtr
	mov	EE, %o0
	jmpl	%o7, %o7
	st	MB, [%o2]

#ifdef CVM_TRACE
	! trace return
	st	%o0, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4] ! save result
	mov	MB, %o1		! %o1 = mb
	mov	JFP, %o2
	!ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], %o0	    ! %o0 = ee
	mov	EE, %o0
	CALL_VM_FUNCTION(CCMtraceFramelessMethodReturn)
        nop
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4], %o0 ! restore result
#endif

	! if %o0 >= 0, then %o0 is the size in words of the method result
	cmp	%o0, 0
	bl	0f
	nop
	sll	%o0, 2, %o0
	add	ARGS, %o0, JSP	! pop args and adjust for result
        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	mov	%o0, %i0	! Set up return value
        ! Note: Use jmpl instead of retl. %o7 already has the call address + 8.
	jmpl	%o7, %g0
	nop

0:

	! check if a new mb to execute has been returned
	cmp	%o0, CONSTANT_CNI_NEW_MB
	bne	new_transition
	nop
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], %o0    ! %o0 = newMb
        ! adjust TOS. The new method may have fewer args than the CNI method
        ldub	[%o0 + OFFSET_CVMMethodBlock_argsSizeX], %o1	! %o1 = argsSize
	sll	%o1, 2, %o1
	add	ARGS, %o1, JSP	! adjust TOS past args
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
        mov	%o0, %i0	! Set up return value
	b	returnToInterpreter1
	nop

new_transition:

	! check if a new transition frame to execute has been setup
	cmp	%o0, CONSTANT_CNI_NEW_TRANSITION_FRAME
	! an exception has occurred
        mov	%o0, %i0	! Set up return value
	bne	returnToInterpreter
	nop
        st	ARGS, [JFP + OFFSET_CVMFrame_topOfStack]	! pop args
	mov	%o0, %i0	! Set up return value
	b	returnToInterpreter0
	nop

#undef MB
#undef ARGS
        SET_SIZE( CVMCCMinvokeCNIMethod )

ENTRY( CVMCCMinvokeJNIMethod ) 
        ! %o0 = target mb 
#define MB  %o0

	add	%o7, 8, %o7
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] 

	FIXUP_FRAMES_1(JFP, %g1, %o0)

	mov	MB, %o1

        st	JSP, [JFP + OFFSET_CVMFrame_topOfStack] 
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 

	CALL_VM_FUNCTION(CVMinvokeJNIHelper)
	mov	EE, %o0

        ld	[JFP + OFFSET_CVMFrame_topOfStack], JSP
	! check for exception
	cmp	%o0, 0
	beq	returnToInterpreter0
	nop

        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	! Note: Use jmpl instead of retl. %o7 already has the call address + 8.
	jmpl	%o7, %g0
	nop

        SET_SIZE( CVMCCMinvokeJNIMethod ) 

ENTRY( CVMCCMletInterpreterDoInvoke )
letInterpreterDoInvoke_store_lr:
        add	%o7, 8, %o7   ! return address
        st	%o7,[JFP + OFFSET_CVMCompiledFrame_PC]

ENTRY( CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr ) 
letInterpreterDoInvoke: 
        /* 
         * Trying to invoke something beyond our ability. 
         * Return the mb to the interpreter and let it do the 
         * dirty work. 
         * we have already set up the return PC in our own frame 
         * We need to set topOfStack then return the target MB* 
         * as a C return value. 
         */ 
	FIXUP_FRAMES_1(JFP, %g1, %o0)

        mov	%o0, %i0             ! Set the return MB
        ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], EE
        st	JSP, [JFP + OFFSET_CVMFrame_topOfStack] 
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	ret
	restore

	!
	! Do a GC check, and rendezvous if one is going on
	!
handleGCForReturn:
	!
	! At this point a GC is requested.
	!

	FIXUP_FRAMES_1(JFP, %g1, %o7)

	ld	[JFP + OFFSET_CVMFrame_mb], %o1		! %o1 = mb
        ! 
        ! Special flag that signals we are handling gc for return. 
        ! Used by CVMcompiledFrameScanner. 
        ! 
	mov	CONSTANT_HANDLE_GC_FOR_RETURN, %g1
	st	%g1, [JFP + OFFSET_CVMCompiledFrame_PC]
	
	! We will be gc safe soon. Prevent this method from being decompiled
	st	%o1, [EE + OFFSET_CVMExecEnv_invokeMb]

	st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	! Fill in the arguments to CVMcsRendezvous	
	set	SYM_NAME(CVMglobals), %g1	
	add	%g1, OFFSET_CVMGlobalState_cstate_GCSAFE, %o1
	add	%o0, OFFSET_CVMExecEnv_tcstate_GCSAFE, %o2
	mov	1, %o3
	CALL_VM_FUNCTION(CVMcsRendezvous)
	mov	EE, %o0					! %o0 = ee
	!
	! GC done.
	!

	! we no longer need to prevent the method from being decompiled
	st	%g0, [EE + OFFSET_CVMExecEnv_invokeMb]

	! Return to caller
	mov	%l7, %o7
	retl
	nop
					
/*
 * The GC checks for the various return variants
 */
handleGCForDoReturn:	
	sethi	%hi(returnToNativeDoReturn), %l7
	b	handleGCForReturn	
	or	%l7, %lo(returnToNativeDoReturn), %l7

handleGCForDoSyncReturn:	
	sethi	%hi(returnToNativeDoSyncReturn), %l7
	b	handleGCForReturn	
	or	%l7, %lo(returnToNativeDoSyncReturn), %l7

/* 
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 *
 * CVMMethodBlock* CVMCCMreturnFromMethod(); 
 */ 
ENTRY( CVMCCMreturnFromMethod )
	! TODO: For now just forget about GC check. For example,
	!       see the ARM port.
	!b	handleGCForDoReturn
	!nop
	/* The GC check for non-sync returns comes back here */
returnToNativeDoReturn:	
        ! see if previous frame is compiled or not 
        ld	[JFP + OFFSET_CVMFrame_prevX], PREV
        btst	CONSTANT_CVM_FRAME_MASK_SLOW, PREV
	bne 	returnToInterpreter
	nop
doReturnToCompiled:

#ifdef CVM_TRACE
	mov	JFP, %o1
	CALL_VM_FUNCTION(CCMtraceMethodReturn)
	mov	EE, %o0
#endif

        ! returning from one native to another. 
        ! do this ourselves. 
	! java sp already set
	and	PREV, ~CONSTANT_CVM_FRAME_MASK_ALL, JFP
        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
#ifdef CVMCPU_HAS_CP_REG
	ld	[JFP + OFFSET_CVMCompiledFrame_cpBaseRegX], CP
#endif
	jmpl	%o7, %g0
	nop

ENTRY( CVMCCMreturnToInterpreter )
returnToInterpreter:
	FIXUP_FRAMES_0(JFP, %g1)

	ld [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], EE     

	! JSP needs to point just past any return value
        st JSP, [JFP + OFFSET_CVMFrame_topOfStack] 

        ! set stack->currentFrame to current value of JFP, 
        ! then return NULL, meaning we do not want the interpreter 
        ! to take any further action on our behalf (except pop
	! the current frame)

        st JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
returnToInterpreter0:
        mov 0, %i0
returnToInterpreter1:
	ret
	restore
        SET_SIZE( CVMCCMreturnFromMethod )

ENTRY( CVMCCMreturnFromSyncMethod )
returnToNativeDoSyncReturn:
#if 0
	!
	! If you just want to call the C helper and write very little assembler
	! code, then just to branch to (and implement) returnToInterpreter.
	!
	b	returnToInterpreter
	nop
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

        /* CAS version of CVMCCMreturnFromSyncMethod: */

#define OBJ		%o0
#define	OBITS		%o1
#define NBITS		%o2
#define LOCKREC		%o3
#define OBITS0		%o4
#define EXPECTED_CNT	OBITS0
#define NEW_COUNT	NBITS

	!
	! see if previous frame is compiled or not 
	!
	ld      [JFP + OFFSET_CVMFrame_prevX], PREV
	btst	CONSTANT_CVM_FRAME_MASK_SLOW, PREV
	bne	returnToInterpreter
	nop

	/* Do fastTryUnlock(): */

	ld	[JFP + OFFSET_CVMCompiledFrame_receiverObjX], OBJ

	/* Check to see if the object is locked with a fastlock: */
	ld	[OBJ + OFFSET_CVMObjectHeader_various32], LOCKREC /* Get obits */
        btst    0x3, LOCKREC    /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
	bne	_fastTryUnlockFailed /* If not, we failed. */
	nop

	cmp	LOCKREC, 0
	beq	_fastTryUnlockFailed
	nop

	/* If we get here, then the object is locked with a fastlock: */

	/* Make sure that the current thread owns the monitor: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_owner], %g1
	cmp	%g1, EE
	bne	_fastTryUnlockFailed	/* If not owner, we failed. */
	nop

	/* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_count], EXPECTED_CNT
	cmp	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	_fastTryUnlockFailed
	nop
	sub	EXPECTED_CNT, 1, NEW_COUNT
	cmp	NEW_COUNT, %g0
	beq	_doUnlock	/* If zero, then unlock */
	nop

	/* new monitor count > 0, so just update it and we're done */
	add	LOCKREC, OFFSET_CVMOwnedMonitor_count, %g1 /* address of count */
	cas	[%g1], EXPECTED_CNT, NEW_COUNT
	cmp	EXPECTED_CNT, NEW_COUNT
	bne	_fastTryUnlockFailed
	nop

	/* we're done! monitor count was successfully decrement */
	b	doReturnToCompiled
	nop
	/* End. */

_doUnlock:	
	/* If we get here, then the re-entry count has reached 0. */
	/* Restore the obits to the object header: */
	mov	LOCKREC, OBITS		/* initialize OBITS */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits], NBITS
	add	OBJ, OFFSET_CVMObjectHeader_various32, %g1 /* address of various32 */
	cas	[%g1], OBITS, NBITS
	cmp	OBITS, NBITS
	bne	_fastTryUnlockFailed
	nop
	
#ifdef CVM_DEBUG
	/* Make the lockrec play nice with the debug assertions: */
	st	%g0, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
	mov	CONSTANT_CVM_OWNEDMON_FREE, %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
	st	%g0, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]
	st	%g0, [LOCKREC + OFFSET_CVMOwnedMonitor_object]
#endif

	/* Check if the lockrec is the first one on the thread's owned list: */
	ld	[EE + OFFSET_CVMExecEnv_objLocksOwned], %g1
	cmp	%g1, LOCKREC
	bne	_fastTryUnlockFindPrevLockRecord
	nop

	/* Remove the lockrec from the ee's owned list: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_next], %g1
	st	%g1, [EE + OFFSET_CVMExecEnv_objLocksOwned]

_fastTryUnlockAddLockRecordToFreeList:
	/* Add the lockrec to the ee's free list: */
	ld	[EE + OFFSET_CVMExecEnv_objLocksFreeOwned], %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
	st	LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

	/* we're done! lock was successfully released */
	b	doReturnToCompiled
	nop

/* PREV_REC (g1) is the first one on the thread's owned list */
#define PREV_REC %g1
/* It is safe to reuse NBITS at this point */
#define NEXT_REC NBITS
_fastTryUnlockFindPrevLockRecord:
	ld	[PREV_REC + OFFSET_CVMOwnedMonitor_next], NEXT_REC
	cmp	NEXT_REC, LOCKREC
	beq	_fastTryUnlockFoundPrevLockRecord
	nop
	b	_fastTryUnlockFindPrevLockRecord
	mov	NEXT_REC, PREV_REC

_fastTryUnlockFoundPrevLockRecord:
	/* Remove the lockrec from the ee's owned list: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_next], NEXT_REC
	b	_fastTryUnlockAddLockRecordToFreeList
	st	NEXT_REC, [PREV_REC + OFFSET_CVMOwnedMonitor_next]
#undef PREV_REC
#undef NEXT_REC

_fastTryUnlockFailed:
	/* Let the interpreter handle the hard cases: */
	b	returnToInterpreter
	nop

#undef OBJ
#undef NBITS
#undef LOCKREC
#undef OBITS0
#undef OBITS
#undef EXPECTED_CNT
#undef NEW_COUNT

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
	b	returnToInterpreter
        nop
#else

        /* CVM_MICROLOCK_SWAP_SPINLOCK microlock version: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

#define OBJ         %o0
#define MICROLOCK   %o1
#define LOCKREC     %o2
#define TEMP        %o3
#define TEMP2       %o4

        ld      [JFP + OFFSET_CVMFrame_prevX], PREV

        set     CVMobjGlobalMicroLockPtr, MICROLOCK
        ! Set up TEMP for swap below
        set     CVM_MICROLOCK_LOCKED, TEMP
        ! Get address of object microlock
        ld      [MICROLOCK], MICROLOCK

        ! see if previous frame is compiled or not 
        ! PREV is set up by all code that branches here

        btst    CONSTANT_CVM_FRAME_MASK_SLOW, PREV
        ld      [MICROLOCK], TEMP2
        bne     returnToInterpreter
        nop

        /* Do fastTryUnlock(): */

        ld      [JFP + OFFSET_CVMCompiledFrame_receiverObjX], OBJ

        /* Acquire the microlock: */
        swap    [MICROLOCK], TEMP ! Swap CVM_MICROLOCK_LOCKED into the lockWord.

        ! Get LOCKREC. INVARIANT: All branches and fallthroughs to _unlockObj
        ! have to set up LOCKREC first

        ld      [OBJ + OFFSET_CVMObjectHeader_various32], LOCKREC ! Get obits.
        cmp     TEMP, CVM_MICROLOCK_UNLOCKED     ! See if we succeeded.
        bne     _fastUnlockAcquireMicrolock     ! Branch if failed.
        nop

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
        set     fastMlockimplCount, %g1
        ld      [%g1], TEMP2
        add     TEMP2, 1, TEMP2
        st      TEMP2, [%g1]
#endif
        /* The microlock has been acquired: */
_unlockObj:
        /* Check to see if the object is locked with a fastlock: */
        btst    0x3, LOCKREC          ! (obits & 0x3) == CVM_LOCKSTATE_LOCKED?
        bne     _fastTryUnlockFailed  ! If not, we failed.
        nop

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_owner], %g1
        ! Optimistically load count
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_count], TEMP
        ! Are we the owner?
        cmp     %g1, EE
        bne     _fastTryUnlockFailed    ! If not, we failed.
        nop

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        subcc   TEMP, 1, TEMP
        bne     _fastTryUnlockSuccess   ! If not zero, we are done.
        nop

        /* If we get here, then the re-entry count has reached 0. */
        ! Restore the obits to the object header:
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits], %g1
        st      %g1, [OBJ + OFFSET_CVMObjectHeader_various32]

#ifdef CVM_DEBUG
        ! Make the lockrec play nice with the debug assertions:
        ! Now TEMP is not going to be the entry count anymore
        set     CONSTANT_CVM_OWNEDMON_FREE, TEMP
        st      TEMP, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_object]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
#endif

        ! %g1 = ee->objLocksOwned  (advanced load for below)
        ld      [EE + OFFSET_CVMExecEnv_objLocksOwned], %g1

        ! Release the microlock:
        st      %g0, [MICROLOCK] ! *microlock = CVM_MICROLOCK_UNLOCKED;

        ! Check if the lockrec is the first one on the thread owned list:
        cmp     %g1, LOCKREC
        bne     _fastTryUnlockFindPrevLockRecordLoop
        nop

        ! Remove the lockrec from the ee owned list: 
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], TEMP

        ! INVARIANT: All branches and fallthroughs to 
        ! _fastTryUnlockAddLockRecordToFreeList must set up %g1 to be
        ! ee->objLocksFreeOwned first

        ld      [EE + OFFSET_CVMExecEnv_objLocksFreeOwned], %g1
        st      TEMP, [EE + OFFSET_CVMExecEnv_objLocksOwned]

_fastTryUnlockAddLockRecordToFreeList:
        ! Add the lockrec to the ee free list:
        st      %g1, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
        st      LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]
        ! Fall through to _fastTryUnlockDone
_fastTryUnlockDone:
        b       doReturnToCompiled
        nop
        /* End. */

#define PREV_REC %g1
_fastTryUnlockFindPrevLockRecordLoop:
        ld      [PREV_REC + OFFSET_CVMOwnedMonitor_next], TEMP2
        cmp     TEMP2, LOCKREC
        beq     _fastTryUnlockFoundPrevLockRecord
        nop
        mov     TEMP2, PREV_REC
        b       _fastTryUnlockFindPrevLockRecordLoop
        nop

_fastTryUnlockFoundPrevLockRecord:
        ! Remove the lockrec from the ee owned list: 
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], TEMP2
        st      TEMP2, [PREV_REC + OFFSET_CVMOwnedMonitor_next]
#undef PREV_REC
        ! Satisfy invariant at _fastTryUnlockAddLockRecordToFreeList
        ld      [EE + OFFSET_CVMExecEnv_objLocksFreeOwned], %g1
        b       _fastTryUnlockAddLockRecordToFreeList
        nop

_fastTryUnlockSuccess:
        ! Set the new re-entry count:
        ! Decremented before we got here
        st      TEMP, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
        ! Release the microlock:
        st      %g0, [MICROLOCK]        ! *microlock = CVM_MICROLOCK_UNLOCKED;
        b       _fastTryUnlockDone
        nop

_fastUnlockAcquireMicrolock:
        /* Call a C function to acquire the microlock: */
        st      MICROLOCK, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4]
        mov     MICROLOCK, %o0
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        nop
        ld      [JFP + OFFSET_CVMCompiledFrame_receiverObjX], OBJ
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4], MICROLOCK
        ld      [OBJ + OFFSET_CVMObjectHeader_various32], LOCKREC ! Get obits.

        b       _unlockObj              ! Go unlock the object if possible.
        nop

_fastTryUnlockFailed:
        /* Release the microlock: */
        st      %g0, [MICROLOCK]        ! *microlock = CVM_MICROLOCK_UNLOCKED;

        ! Let the interpreter handle the hard cases:
        b       returnToInterpreter
        nop

#undef OBJ
#undef MICROLOCK
#undef LOCKREC
#undef TEMP
#undef TEMP2

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

	SET_SIZE( CVMCCMreturnFromSyncMethod )


#ifdef CVM_TRACE
ENTRY(CVMCCMtraceMethodCallGlue)
	set SYM_NAME(CVMglobals), %g1 
	ld [%g1 + OFFSET_CVMGlobalState_debugFlags], %g1
	andcc %g1, CONSTANT_TRACE_METHOD, %g0
	bne	0f
	nop

	retl
	nop

0:
	FIXUP_FRAMES_3(JFP, %g1, %o0, %o1, %o7)

        ! Flush the PC to the new frame that was just pushed, otherwise
        ! CVMtraceMethodCall() will print the wrong line number information.
	add	%o7, 8, %o7
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC]
	sub	%o7, 8, %o7
	mov 0, %o2		! isJump
	b SYM_NAME(CVMtraceMethodCall)	! tail call
	nop
SET_SIZE(CVMCCMtraceMethodCallGlue)

ENTRY(CCMtraceMethodReturn)
	set SYM_NAME(CVMglobals), %g1
	ld [%g1 + OFFSET_CVMGlobalState_debugFlags], %g1
	andcc %g1, CONSTANT_TRACE_METHOD, %g0
	bne 0f
	nop
	retl
	nop
0: 
	FIXUP_FRAMES_3(JFP, %g1, %o0, %o1, %o7)
	b SYM_NAME(CVMtraceMethodReturn)	! tail call
	nop
SET_SIZE(CCMtraceMethodReturn)

ENTRY(CCMtraceFramelessMethodCall)
	set SYM_NAME(CVMglobals), %g1
	ld [%g1 + OFFSET_CVMGlobalState_debugFlags], %g1
	andcc %g1, CONSTANT_TRACE_METHOD, %g0
	bne 0f
	nop
	retl
	nop
0:
	mov 0, %o3			! isJump
	b SYM_NAME(CVMtraceFramelessMethodCall)	! tail call
	nop
SET_SIZE(CCMtraceFramelessMethodCall)

ENTRY(CCMtraceFramelessMethodReturn)
	set SYM_NAME(CVMglobals), %g1
	ld [%g1 + OFFSET_CVMGlobalState_debugFlags], %g1
	andcc %g1, CONSTANT_TRACE_METHOD, %g0
	bne 0f
	nop
	retl
	nop
0:
	b SYM_NAME(CVMtraceFramelessMethodReturn)	! tail call
	nop
SET_SIZE(CCMtraceFramelessMethodReturn)
#endif
