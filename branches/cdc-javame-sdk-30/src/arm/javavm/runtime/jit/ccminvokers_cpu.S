/*
 * @(#)ccminvokers_cpu.S	1.53 06/10/29
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
#include "javavm/include/sync.h"

/*
 * Some of the code in this file gets patched at runtime for handling
 * gc rendezvous. If we aren't copying this code to the code cache,
 * then we must make this a writable section.
 */

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s3_ccminvokers_cpu)
#else
	SET_SECTION_EXEC_WRITE
#endif

/* Support fastlocking with C helper or swap/spinlock based microlocks */
#if defined(CVM_JIT_CCM_USE_C_HELPER) || \
    defined(CVM_JIT_CCM_USE_C_SYNC_HELPER) || \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK)

	ENTRY(CVMCCMinvokeNonstaticSyncMethodHelper)
ENTRY1( CVMCCMinvokeNonstaticSyncMethodHelper )
LABEL(CVMCCMinvokeNonstaticSyncMethodHelperLocal)
	/* r0 = mb 
	 * r1 = CVMObjectICell* of object to sync on. 
	 *
         * r4 = v1 = jfp  
         * r5 = v2 = jsp  
	 */

#define NEW_JFP  CVMARM_NEWJFP_REGNAME  /* r7  = v4 */

        /*
           r0 = ee
           r1 = obj
           r4 = v1 = JFP
           r5 = v2 = JSP
           r7 = v4 = NEW_JFP
        */

#define EE          r0
#define OBJ         r1
#define OBITS       r3
#define MICROLOCK   r8
#define LOCKREC     ip
#define TEMP        r11
#define TEMP2       r10
#define ORIG_LR     TEMP2
	
#define MB   r0
#define PREV CVMARM_PREVFRAME_REGNAME     /* v3 = r6  */

#if defined(CVM_JIT_CCM_USE_C_HELPER) || defined(CVM_JIT_CCM_USE_C_SYNC_HELPER)
/* Support fastlocking with C helper */
	bl letInterpreterDoInvoke
#else
/* Support fastlocking with swap/spinlock based microlocks */
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw MICROLOCK, W_MICROLOCK, #0
	str     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]
#else
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
	/* save r0 */
	str	MB, [NEW_JFP, #OFFSET_CVMFrame_mb]
	/* Schedule r8 = &microLock early */
        ldr     MICROLOCK, [MICROLOCK]
#endif
	ldr	OBJ, [r1]	/* get object to sync on. */
	ldr	TEMP, [MICROLOCK]
	/* now r0 = ee */
	ldr	EE, [sp, #OFFSET_CVMCCExecEnv_ee]
	/* optimistically store receiver object */
	str	OBJ, [NEW_JFP, #OFFSET_CVMCompiledFrame_receiverObjX]

        /* Do fastTryLock(): */

        /* Acquire the microlock: */
        mov     TEMP, #CVM_MICROLOCK_LOCKED  /* Swap CVM_MICROLOCK_LOCKED into */	
#ifndef CVM_MP_SAFE
        swp     TEMP, TEMP, [MICROLOCK]      /*    the lockWord. */

	/* Get obits. INVARIANT: All branches and fallthroughs to _lockObj */
	/* have to set up OBITS first */
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] 
        cmp     TEMP, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _fastLockAcquireMicrolock   /* Branch if failed. */
#else
LABEL(_syncInvokeMicrolockMPSafe)
	ldrex	TEMP2, [MICROLOCK]
	cmp	TEMP2, #CVM_MICROLOCK_UNLOCKED /* is the lock free? */
	bne	_fastLockAcquireMicrolock      /* already locked */
	strex	TEMP2, TEMP, [MICROLOCK]       /* try acquire the lock */
	cmp	TEMP2, #0		       /* strex succeed? */
	bne	_syncInvokeMicrolockMPSafe     /* No. Try again */
	mcr	p15, 0, TEMP2, c7, c10, 5      /* memory barrier */
	ldr	OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32]
#endif

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
	ldr     TEMP, SYMBOL(fastMlockimplCount)
	ldr	TEMP2, [TEMP]
	add	TEMP2, TEMP2, #1
	str	TEMP2, [TEMP]
#endif

        /* The microlock has been acquired: */
LABEL(_lockObj)
        and     ip, OBITS, #0x3
        cmp     ip, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne     _objAlreadyLocked

        /* If we get here, then the object has not been locked yet. */
        /* lockrec = ee->objLocksFreeOwned: */
        ldr     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        mov     TEMP, #1                  /* Initial lock re-entry count. */
        cmp     LOCKREC, #0
        beq     _lockRecordNotAvailable

#ifdef CVM_DEBUG
        /* lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED: */
        mov     r2, #CONSTANT_CVM_OWNEDMON_OWNED
        str     r2, [LOCKREC, #OFFSET_CVMOwnedMonitor_state]
#endif
        /* obj->hdr.various32 = lockrec: */
        str     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32]
        /* lockrec->count = 1: (TEMP initialized above) */
        str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
        /* lockrec->u.fast.bits = obits: */
        str     OBITS, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]

        /* lockrec->object = obj: */
        str     OBJ, [LOCKREC, #OFFSET_CVMOwnedMonitor_object]

#ifdef CVM_MP_SAFE
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5       /* memory barrier. */
#endif
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]         /* microlock->lockWord = UNLOCKED. */

        /* Remove lockrec from the ee's free list: */
        /* nextRec = lockrec->next: */
        ldr     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        /* Add the lockrec to the ee's owned list: */
        /* nextRec2 = ee->objLocksOwned: */
        ldr     r1, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
	
        /* ee->objLocksFreeOwned = nextRec: */
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* lockrec->next = nextRec2: */
        str     r1, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
	
        /* ee->objLocksOwned = lockrec: */
        str     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

LABEL(_fastlockSuccess)
        ldr     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]      /* Reload MB. */
        mov     PREV, JFP
        mov     JFP, NEW_JFP
LABEL(jfp_set)
	/* compiled frame */
#ifdef CVM_DEBUG_ASSERTS
	mov r3, #CONSTANT_CVM_FRAMETYPE_NONE
	strb r3, [JFP, #OFFSET_CVMFrame_type]
	mov r3, #-1
	strb r3, [JFP, #OFFSET_CVMFrame_flags]
#endif
	str PREV, [JFP, #OFFSET_CVMFrame_prevX]

        /* set up registers  */
        /* see about stack frame requirements.  */

#ifdef CVM_TRACE
	mov ORIG_LR, lr
	mov r1, JFP
	ldr r0, [sp, #OFFSET_CVMCCExecEnv_ee]
	bl SYM_NAME(CVMCCMtraceMethodCallGlueLocal)
	mov lr, ORIG_LR
#endif

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

	ENTRY(CVMARMgcPatchPointAtInvoke)
ENTRY1( CVMARMgcPatchPointAtInvoke )
LABEL(CVMARMgcPatchPointAtInvokeLocal)
	/* GC check - gc will patch at this location when a rendezvous is 
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will 
	 * be a "b CVMARMhandleGCAtInvoke" 
	 */

        mov	pc, lr	/* Return to method after handling possible GC request */

	ENTRY(CVMARMhandleGCAtInvoke)
ENTRY1( CVMARMhandleGCAtInvoke)
#if !defined(CVM_JIT_CCM_USE_C_HELPER) && !defined(CVM_JIT_CCM_USE_C_SYNC_HELPER)

	/* At this point a GC is requested. */

	mov	ORIG_LR, lr
	
	FIXUP_FRAMES_0(JFP)

	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		/* r0 = ee */
	ldr  r1, [JFP, #OFFSET_CVMFrame_mb]		/* r1 = mb */

	/* We will be gc safe soon. Prevent this method from being decompiled */
	str  r1, [r0, #OFFSET_CVMExecEnv_invokeMb]

	/* Check if this is a synchronized invocation 
	 * If it is, we have to stash the receiver in the 
	 * newly pushed frame into a safe location. The new frame is not 
	 * yet "committed" to the stack, and as such, cannot be located 
	 * by GC. 
	 */
        ldrh r1, [r1, #OFFSET_CVMMethodBlock_invokerAndAccessFlagsX]
        tst  r1, #CONSTANT_METHOD_ACC_SYNCHRONIZED
	
	/* Synchronized method if result of 'tst' is 'ne'. Stash
	 * receiver in [ee->miscICell] */
	ldrne r1, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX] 
	ldrne r2, [r0, #OFFSET_CVMExecEnv_miscICell]
	strne r1, [r2]					/* stash in miscICell */

/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, SYMBOL(CVMglobals)
#endif

	/* At this point r0 is guaranteed to contain the ee */
        str PREV,[r0,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	mov r3, #1
	add r1, ip, #OFFSET_CVMGlobalState_cstate_GCSAFE
	add r2, r0, #OFFSET_CVMExecEnv_tcstate_GCSAFE
	CALL_VM_FUNCTION(CVMcsRendezvous)

	/* reload the ee and mb */
	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		/* r0 = ee */
	ldr  r1, [JFP, #OFFSET_CVMFrame_mb]		/* r1 = mb */

	/* we no longer need to prevent the method from being decompiled */
	mov  ip, #0
	str  ip, [r0, #OFFSET_CVMExecEnv_invokeMb]

	mov  lr, ORIG_LR
	
	/*
	 * We've returned from the GC. Check for a sync method
	 * again to see if we should restore 'receiverObjX'
	 * from miscICell.
	 */
        ldrh r2, [r1, #OFFSET_CVMMethodBlock_invokerAndAccessFlagsX]
        tst  r2, #CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq  CVMARMgcPatchPointAtInvokeLocal

	/* Restore receiverObjX in new frame */
	ldr r0, [r0, #OFFSET_CVMExecEnv_miscICell]	/* r0 = &miscICell */
	ldr ip, [r0]
	str ip, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
	
	/* And clear miscICell for other uses */
	mov ip, #0
	str ip, [r0]

	/* reload the ee. The frame flush needs it at the return point */
	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		/* r0 = ee */

	b CVMARMgcPatchPointAtInvokeLocal

LABEL(_fastLockAcquireMicrolock)
	mov	ORIG_LR, lr
        /* Call a C function to acquire the microlock: */
        mov     r0, MICROLOCK
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
	mov	lr, ORIG_LR
        ldr     OBJ, [NEW_JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits. */
        b       _lockObj

LABEL(_objAlreadyLocked)
        cmp     ip, #CONSTANT_CVM_LOCKSTATE_LOCKED
        bne     _fastReentryFailed

        /* Make sure the current thread owns this lock: */
        ldr     TEMP, [OBITS, #OFFSET_CVMOwnedMonitor_owner]
	/* Optimistically load count */
        ldr     ip, [OBITS, #OFFSET_CVMOwnedMonitor_count]
	/* Are we owner? */
        cmp     TEMP, EE
        bne     _fastReentryFailed

        add     ip, ip, #1 /* count++ */
        str     ip, [OBITS, #OFFSET_CVMOwnedMonitor_count]

        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]            /* microlock->lockWord = UNLOCKED. */
        b       _fastlockSuccess

LABEL(_fastReentryFailed)
LABEL(_lockRecordNotAvailable)
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]
        /* Fall through to _fastTryLockFailed. */

LABEL(_fastTryLockFailed)
        ldr     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]
        b       letInterpreterDoInvoke

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK support */
	SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

#undef EE
#undef OBJ
#undef OBITS
#undef MICROLOCK
#undef LOCKREC

/* Support for fastlocking with atomic ops */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

#define MB   r0
#define PREV CVMARM_PREVFRAME_REGNAME     /* v3 = r6  */

#define ORIG_LR         r10

 ENTRY(CVMCCMinvokeNonstaticSyncMethodHelper) 
LABEL(CVMCCMinvokeNonstaticSyncMethodHelperLocal) 
	; r0 = mb
	; r1 = CVMObjectICell* of object to sync on.
	;
        ; r4 = v1 = jfp 
        ; r5 = v2 = jsp 

#define NEW_JFP  CVMARM_NEWJFP_REGNAME  /* r7  = v4 */

        /*
           r0 = ee
           r1 = obj
           r4 = v1 = JFP
           r5 = v2 = JSP
           r7 = v4 = NEW_JFP
        */

/* TODO: This CVMCCMinvokeNonstaticSyncMethodHelper code has not undergone
   thorough multi-threaded stress testing yet.  It needs to undergo stress
   testing before release. */

#if 0
        /* If you just want to call the C helper and write very little
           assembler code, then just branch to (and implement)
           letInterpreterDoInvoke.
        */
        b       letInterpreterDoInvoke
#endif

#define EE              ORIG_LR /* r10 */
#define OBJ             r1
#define EXPECTED_BITS   r8
#define OWNEDREC        r11
#define TEMP            ip

        ldr     OBJ, [r1]       /* get object to sync on. */
        str     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]      ; save r0
        ; optimistically store receiver object
        str     OBJ, [NEW_JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
        ; now r0 = ee
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]

        /* Do fastTryLock(): */

        /* Setup a lock record and assume the object has not been locked
           yet: */
        ; ownedRec = ee->objLocksFreeOwned:
        ldr     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        cmp     OWNEDREC, #0
        beq     _ownedRecordNotAvailable

        ; ownedRec->object = obj:
        str     OBJ, [OWNEDREC, #OFFSET_CVMOwnedMonitor_object]

        ; ownedRec->count = 1:
        mov     TEMP, #1        /* Initial lock re-entry count */
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]

#ifdef CVM_DEBUG
        ; ownedRec->state = CONSTANT_CVM_OWNEDMON_OWNED:
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_OWNED
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
#endif

        ; NEW_BITS = (OWNEDREC) | CVM_LOCKSTATE_LOCKED:
        ; Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
        ; that NEW_BITS is essentially ownedRec.  Nothing need to be done to
        ; initialize NEW_BITS.

#define ACTUAL_BITS     r0
#define NEW_BITS        OWNEDREC

        ; obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED:
        ldr     EXPECTED_BITS, [OBJ, #OFFSET_CVMObjectHeader_various32]
        bic     EXPECTED_BITS, EXPECTED_BITS, #3 /* clear rightmost 2 bits */
        orr     EXPECTED_BITS, EXPECTED_BITS, #CONSTANT_CVM_LOCKSTATE_UNLOCKED

        ; ownedRec->u.fast.bits = obits:
        str     EXPECTED_BITS, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]

        ; Do atomicCompareAndSwap:
        ; ACTUAL_BITS = [OBJ, #OFFSET_CVMObjectHeader_various32]
        ; if ([OBJ, #OFFSET_CVMObjectHeader_various32] == EXPECTED_BITS) {
        ;     [OBJ, #OFFSET_CVMObjectHeader_various32] = NEW_BITS
        ; }

        add     r0, OBJ, #OFFSET_CVMObjectHeader_various32
        mov     r1, EXPECTED_BITS
        mov     r2, NEW_BITS
        mov     ORIG_LR, lr /* NOTE: ORIG_LR and EE are the same registers. */
        CALL_VM_FUNCTION(InterlockedTestExchange)
        mov     lr, ORIG_LR
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee] /* Restore EE. */
        cmp     ACTUAL_BITS, EXPECTED_BITS      /* ACTUAL_BITS is in r0. */
        bne     _objAlreadyLocked               /* already locked */

#undef NEW_BITS

        /* Remove lockrec from the ee's free list: */
        ; nextRec = ownedRec->next:
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        ; ee->objLocksFreeOwned = nextRec:
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* Add the lockrec to the ee's owned list: */
        ; nextRec = ee->objLocksOwned:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        ; ownedRec->next = nextRec:
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        ; ee->objLocksOwned = ownedRec:
        str     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        b       _fastlockSuccess        /* we are all done locking */

_objAlreadyLocked
#ifdef CVM_DEBUG
        ; lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
        mov     TEMP, #0
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_FREE
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
#endif

        ; If object is not in LOCKED state, then fail:
        ands    TEMP, ACTUAL_BITS, #0x3 /* check for CVM_LOCKSTATE_LOCKED */
        bne     _fastRetryFailed

        ; If not associated with a lock record, then fail:
        bics    OWNEDREC, ACTUAL_BITS, #2
        beq     _fastReentryFailed      /* Make sure ownedRec is not NULL. */

#undef ACTUAL_BITS

        ; If (ownedRec->owner != ee), then fail:
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_owner]
        cmp     TEMP, EE
        bne     _fastReentryFailed

#define EXPECTED_CNT    EXPECTED_BITS
#define NEW_COUNT       r2
#define ACTUAL_CNT      r0

        ; If we get here, then we are re-entering the lock:
        ldr     EXPECTED_CNT, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        cmp     EXPECTED_CNT, #CONSTANT_CVM_INVALID_REENTRY_COUNT
        beq     _fastReentryFailed

        add     NEW_COUNT, EXPECTED_CNT, #1

        add     r0, OWNEDREC, #OFFSET_CVMOwnedMonitor_count
        mov     r1, EXPECTED_CNT
        ; r2 already contains the NEW_CNT.
        mov     ORIG_LR, lr /* NOTE: ORIG_LR and EE are the same registers. */
        CALL_VM_FUNCTION(InterlockedTestExchange)
        mov     lr, ORIG_LR
        /* NOTE: We don't need EE after this.  So, no need to restore it. */
        cmp     ACTUAL_CNT, EXPECTED_CNT        /* ACTUAL_CNT is in r0. */
        beq     _fastlockSuccess                /* Success if as expected. */

        /* Else, fall thru to _fastReentryFailed below. */

#undef EXPECTED_CNT
#undef NEW_COUNT
#undef ACTUAL_CNT

_fastRetryFailed
_fastReentryFailed
_ownedRecordNotAvailable
        /* Let interpreter do the hard stuff: */
        ldr     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]
        b       letInterpreterDoInvoke

_fastlockSuccess
        ldr     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]      ; Reload MB.
        mov     PREV, JFP
        mov     JFP, NEW_JFP

        ; compiled frame
#ifdef CVM_DEBUG_ASSERTS
        mov     TEMP, #CONSTANT_CVM_FRAMETYPE_NONE
        strb    TEMP, [JFP, #OFFSET_CVMFrame_type]
        mov     TEMP, #-1
        strb    TEMP, [JFP, #OFFSET_CVMFrame_flags]
#endif
        str     PREV, [JFP, #OFFSET_CVMFrame_prevX]

        ; set up registers
        ; see about stack frame requirements.

#ifdef CVM_TRACE
        mov ORIG_LR, lr
        mov r1, JFP
        ldr r0, [sp, #OFFSET_CVMCCExecEnv_ee]
        bl SYM_NAME(CVMCCMtraceMethodCallGlueLocal)
        mov lr, ORIG_LR
#endif

#undef EE
#undef OBJ
#undef EXPECTED_BITS
#undef OWNEDREC
#undef TEMP


 ENTRY(CVMARMgcPatchPointAtInvoke) 
LABEL(CVMARMgcPatchPointAtInvokeLocal)
	; GC check - gc will patch at this location when a rendezvous is
	; needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	; be a "b CVMARMhandleGCAtInvoke"

        mov	pc, lr	; Return to method after handling possible GC request

 ENTRY(CVMARMhandleGCAtInvoke)	
	;
	; At this point a GC is requested.
	;

	mov	ORIG_LR, lr
	
	FIXUP_FRAMES_0(JFP)

	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		; r0 = ee
	ldr  r1, [JFP, #OFFSET_CVMFrame_mb]		; r1 = mb

	; We will be gc safe soon. Prevent this method from being decompiled
	str  r1, [r0, #OFFSET_CVMExecEnv_invokeMb]

	; Check if this is a synchronized invocation
	; If it is, we have to stash the receiver in the
	; newly pushed frame into a safe location. The new frame is not
	; yet "committed" to the stack, and as such, cannot be located
	; by GC.
        ldrh r1, [r1, #OFFSET_CVMMethodBlock_invokerAndAccessFlagsX]
        tst  r1, #CONSTANT_METHOD_ACC_SYNCHRONIZED
	
	; Synchronized method if result of 'tst' is 'ne'. Stash
	; receiver in [ee->miscICell]
	ldrne r1, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX] 
	ldrne r2, [r0, #OFFSET_CVMExecEnv_miscICell]
	strne r1, [r2]					; stash in miscICell

	IMPORT CVMglobals
	ldr ip, SYMBOL(CVMglobals)
	; At this point r0 is guaranteed to contain the ee
        str PREV,[r0,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	mov r3, #1
	add r1, ip, #OFFSET_CVMGlobalState_cstate_GCSAFE
	add r2, r0, #OFFSET_CVMExecEnv_tcstate_GCSAFE
	CALL_VM_FUNCTION(CVMcsRendezvous)

	; reload the ee and mb
	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		; r0 = ee
	ldr  r1, [JFP, #OFFSET_CVMFrame_mb]		; r1 = mb

	; we no longer need to prevent the method from being decompiled
	mov  ip, #0
	str  ip, [r0, #OFFSET_CVMExecEnv_invokeMb]

	mov  lr, ORIG_LR
	
	;
	; We've returned from the GC. Check for a sync method
	; again to see if we should restore 'receiverObjX'
	; from miscICell.
	;
        ldrh r2, [r1, #OFFSET_CVMMethodBlock_invokerAndAccessFlagsX]
        tst  r2, #CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq  CVMARMgcPatchPointAtInvokeLocal

	; Restore receiverObjX in new frame
	ldr r0, [r0, #OFFSET_CVMExecEnv_miscICell]	; r0 = &miscICell
	ldr ip, [r0]
	str ip, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
	
	; And clear miscICell for other uses
	mov ip, #0
	str ip, [r0]

	; reload the ee. The frame flush needs it at the return point
	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		; r0 = ee

	b CVMARMgcPatchPointAtInvokeLocal

	SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

#undef ORIG_LR
#else
#error unsupported CVM_FASTLOCK_TYPE or CVM_MICROLOCK_TYPE
#endif 

	ENTRY(CVMCCMinvokeStaticSyncMethodHelper)
ENTRY1( CVMCCMinvokeStaticSyncMethodHelper )
        /* r0 = a1 = target mb  
         * r4 = v1 = jfp  
         * r5 = v2 = jsp  
	 */

#ifdef CVM_METHODBLOCK_HAS_CB
	ldr r1, [MB, #OFFSET_CVMMethodBlock_cbX]
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        ldrb r1, [MB, #OFFSET_CVMMethodBlock_methodIndexX]

	/* r1 = MB - r1*32 + r1*4 
	 *   which means 
	 * r1 = MB - r1*28 
	 */
        sub ip, MB, r1, asl #5
	add  r1, ip, r1, asl #2

        ldr  r1, [r1, #-OFFSET_CVMMethodRange_mb]
#endif

	/* r1 needs to be set to the icell of the object to lock */

	ldr	r1, [r1, #OFFSET_CVMClassBlock_javaInstanceX]
	b	CVMCCMinvokeNonstaticSyncMethodHelperLocal

	SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

	ENTRY(CVMCCMinvokeCNIMethod)
ENTRY1( CVMCCMinvokeCNIMethod )
        /* r4 = v1 = jfp  
         * r5 = v2 = jsp  
         * r0 = target mb 
	 */

        ldrb	r1, [MB,#OFFSET_CVMMethodBlock_argsSizeX]
        str	lr, [JFP, #OFFSET_CVMCompiledFrame_PC] 
        sub	r1, JSP, r1, LSL #2	/* TOS */

#undef MB
#define MB   r6 /* v3 */
#define ARGS r7 /* v4 */
	mov	MB, r0		/* save MB */
	mov	ARGS, r1	/* save args ptr */

	/* although r1 is now in ARGS, we still want to preserve it */
	FIXUP_FRAMES(JFP, {r1}, 1)

	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]
        str	JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 
        str	JFP,[r0 ,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 

#ifdef CVM_TRACE
	/* trace call */
	mov	r1, JFP
	mov	r2, MB		/* r2 = mb */
	bl	SYM_NAME(CCMtraceFramelessMethodCallLocal)
	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]	    /* r0 = ee */
	mov	r1, ARGS		/* restore args ptr */
#endif

	/* invoke the method  - r1 is still the args pointer */
	add	r2, sp, #OFFSET_CVMCCExecEnv_ccmStorage	    /* r2 = mbPtr */
	str	MB, [r2]	/* store MB into mbPtr */
	mov	lr, pc
        ldr	pc, [MB, #OFFSET_CVMMethodBlock_codeX]

#ifdef CVM_TRACE
	/* trace return */
	str	r0, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4] /* save result */
	mov	r1, MB		/* r1 = mb */
	mov	r2, JFP
	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]	    /* r0 = ee */
	bl	SYM_NAME(CCMtraceFramelessMethodReturnLocal)
	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4] /* restore result */
#endif

	/* if r0 >= 0, then r0 is the size in words of the method result */
	cmp	r0, #0
	addge	JSP, ARGS, r0, LSL #2	/* pop args and adjust for result */
        ldrge	pc, [JFP, #OFFSET_CVMCompiledFrame_PC] 

	/* check if a new mb to execute has been returned */
	cmp	r0, #CONSTANT_CNI_NEW_MB
	bne	new_transition
	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ccmStorage]    /* r0 = newMb */
	/* adjust TOS. The new method may have fewer args than the CNI method */
        ldrb	r1, [r0,#OFFSET_CVMMethodBlock_argsSizeX]    /* r1 = argsSize */
        add	JSP, ARGS, r1, LSL #2	/* adjust TOS past args */
        str	JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 
	b	returnToInterpreter1

	/* check if a new transition frame to execute has been setup */
LABEL(new_transition)
	cmp	r0, #CONSTANT_CNI_NEW_TRANSITION_FRAME
        streq	ARGS, [JFP, #OFFSET_CVMFrame_topOfStack]       /* pop args */
	beq	returnToInterpreter0

	/* an exception has occurred */
	b	returnToInterpreter
#undef MB
#undef ARGS
        SET_SIZE( CVMCCMinvokeCNIMethod ) 

	ENTRY(CVMCCMinvokeJNIMethod)
ENTRY1( CVMCCMinvokeJNIMethod ) 
        /* r4 = v1 = jfp  
         * r5 = v2 = jsp  
         * r0 = target mb 
	 */
#define MB  r0

        str	lr,[JFP,#OFFSET_CVMCompiledFrame_PC] 

	FIXUP_FRAMES(JFP, {r0}, 1)

	mov	r1, MB

	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]
        str	JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 
        str	JFP,[r0,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 

	CALL_VM_FUNCTION(CVMinvokeJNIHelper)
        ldr	JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 
	/* check for exception */
	cmp	r0, #0
        ldrne	pc,[JFP,#OFFSET_CVMCompiledFrame_PC] 
	b	returnToInterpreter0

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
 	 *  r0 = invokeMb (not the targetMb) 
 	 */
	ENTRY(CVMCCMinvokeVirtual)
ENTRY1( CVMCCMinvokeVirtual )
	/* Get "this" by looking back on the stack */
        ldrb	r1, [r0, #OFFSET_CVMMethodBlock_argsSizeX]
        sub	r1, JSP, r1, LSL #2	/* r1 = stack address of "this" */
	ldr	r1, [r1, #0]		/* r1 = "this" */
	/* get the method table from "this" */
	ldr	r2, [r1, #OFFSET_CVMObjectHeader_clas]
	bic	r2, r2, #3		/* mask off low 2 bits of object cb */
	ldr	r2, [r2, #OFFSET_CVMClassBlock_methodTablePtrX]
	/* get method table index from invokeMb */
	ldrh	r3, [r0, #OFFSET_CVMMethodBlock_methodTableIndexX]
        /* r0 = cb->methodTablePtrX[ip]: */
        ldr     r0, [r2, r3, asl #2]
	/* call the method */
	ldr	pc, [r0, #OFFSET_CVMMethodBlock_jitInvokerX]
        SET_SIZE( CVMCCMinvokeVirtual ) 
#endif

	ENTRY(CVMCCMletInterpreterDoInvoke)
ENTRY1( CVMCCMletInterpreterDoInvoke )
LABEL(letInterpreterDoInvoke_store_lr)
        str lr,[JFP,#OFFSET_CVMCompiledFrame_PC]

	ENTRY(CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr)
ENTRY1( CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr )
LABEL(letInterpreterDoInvoke)
        /* 
         * Trying to invoke something beyond our ability. 
         * Return the mb to the interpreter and let it do the 
         * dirty work. 
         * we have already set up the return PC in our own frame 
         * We need to set topOfStack then return the target MB* 
         * as a C return value. 
         */ 
	FIXUP_FRAMES(JFP, {r0}, 1)
	ldr r1, [sp, #OFFSET_CVMCCExecEnv_ee]
        str JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 
        str JFP,[r1,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	/* Keep sp 64-bit aligned for AAPCS */
	add	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) BIT_AND ~7)
#ifdef WINCE
	ldmfd	sp, {v1-v7, fp, sp, pc}
#else
	/* r4-r10 = v1-v7 */
	ldmfd	sp!, {r4-r10, fp, ip, pc}
#endif

/*
 * Do a GC check, and rendezvous if one is requested
 */
LABEL(handleGCForReturn)

	/* At this point a GC is requested. */

	FIXUP_FRAMES(JFP, {lr}, 1)

	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]	/* r0 = ee */
	ldr	r1, [JFP, #OFFSET_CVMFrame_mb]		/* r1 = mb */

        /* Special flag that signals we are handling gc for return.  */
        /* Used by CVMcompiledFrameScanner.  */

	mov	lr, #CONSTANT_HANDLE_GC_FOR_RETURN
	str	lr, [JFP, #OFFSET_CVMCompiledFrame_PC]

/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw lr, W_CVMGLOBALS, #0
#else
	ldr	lr, SYMBOL(CVMglobals)
#endif	
	
	/* We will be gc safe soon. Prevent this method from being decompiled */
	str	r1, [r0, #OFFSET_CVMExecEnv_invokeMb]

	str	JFP, [r0, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* r0 is ee already. Fill in the arguments to CVMcsRendezvous	 */
	add	r1, lr, #OFFSET_CVMGlobalState_cstate_GCSAFE
	add	r2, r0, #OFFSET_CVMExecEnv_tcstate_GCSAFE
	mov	r3, #1
	CALL_VM_FUNCTION(CVMcsRendezvous)

	/* GC done. */
	/* Reload the ee */

	ldr	r0, [sp, #OFFSET_CVMCCExecEnv_ee]	/* r0 = ee */

	/* we no longer need to prevent the method from being decompiled */
	mov	lr, #0
	str	lr, [r0, #OFFSET_CVMExecEnv_invokeMb]

	/* This is expected at the beginning of returns */
        ldr     PREV, [JFP,#OFFSET_CVMFrame_prevX]
	/* Return to caller */
	mov	pc, r7

/*
 * The GC checks for the various return variants
 */
	ENTRY(CVMARMhandleGCForReturnFromMethod)
ENTRY1( CVMARMhandleGCForReturnFromMethod )
	ldr	r7, SYMBOL(CVMCCMreturnFromMethod)
	b	handleGCForReturn

	ENTRY(CVMARMhandleGCForReturnFromSyncMethod)
ENTRY1( CVMARMhandleGCForReturnFromSyncMethod )
	ldr	r7, SYMBOL(CVMCCMreturnFromSyncMethod)
	b	handleGCForReturn	
	SET_SIZE( CVMARMhandleGCForReturnFromSyncMethod )

/* 
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 *
 * CVMMethodBlock* CVMCCMreturnFromMethod(); 
 */ 
	ENTRY(CVMCCMreturnFromMethod)
ENTRY1( CVMCCMreturnFromMethod )
	/* GC check - gc will patch at this location when a rendezvous is 
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will 
	 * be a "b CVMARMhandleGCForReturnFromMethod" 
	 *
	 * see if previous frame is compiled or not 
	 * PREV is set up by all code that branches here 
	 */

        tst PREV, #CONSTANT_CVM_FRAME_MASK_SLOW

#ifdef CVM_TRACE
	bne returnToInterpreter
	mov r1, JFP
	and JFP, PREV, #~CONSTANT_CVM_FRAME_MASK_ALL
	ldr r0, [sp, #OFFSET_CVMCCExecEnv_ee]
	bl SYM_NAME(CCMtraceMethodReturnLocal)
#ifdef CVMCPU_HAS_CP_REG
	ldr CVMARM_CP_REGNAME, [JFP, #OFFSET_CVMCompiledFrame_cpBaseRegX]
#endif
        ldr pc, [JFP,#OFFSET_CVMCompiledFrame_PC]
#else

        /* returning from one native to another.  
         * do this ourselves.  
	 * java sp already set 
	 */
	andeq JFP, PREV, #~CONSTANT_CVM_FRAME_MASK_ALL
#ifdef CVMCPU_HAS_CP_REG
	ldreq CVMARM_CP_REGNAME, [JFP, #OFFSET_CVMCompiledFrame_cpBaseRegX]
#endif
        ldreq pc, [JFP,#OFFSET_CVMCompiledFrame_PC]
	/* fall through to CVMCCMreturnToInterpreter */
#endif

	ENTRY(CVMCCMreturnToInterpreter)
ENTRY1( CVMCCMreturnToInterpreter )
LABEL(returnToInterpreter)
	FIXUP_FRAMES_0(JFP)

	ldr r1, [sp, #OFFSET_CVMCCExecEnv_ee]

	/* JSP needs to point just past any return value */
        str JSP,[JFP,#OFFSET_CVMFrame_topOfStack] 

        /* set stack->currentFrame to current value of JFP, 
         * then return NULL, meaning we don't want the interpreter 
         * to take any further action on our behalf (except pop
	 * the current frame) 
	 */

        str JFP,[r1,#OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
LABEL(returnToInterpreter0)
        mov r0,#0
LABEL(returnToInterpreter1)
	/* Align sp to 64-bit for AAPCS */
	add	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) BIT_AND ~7)
#ifdef WINCE
	ldmfd	sp, {v1-v7, fp, sp, pc}
#else
	/* r4-r10 = v1-v7 */
	ldmfd	sp!, {r4-r10, fp, ip, pc}
#endif
        SET_SIZE( CVMCCMreturnFromMethod )

/* Support fastlocking using C helpers */
#if defined(CVM_JIT_CCM_USE_C_HELPER) || defined(CVM_JIT_CCM_USE_C_SYNC_HELPER)

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
	ENTRY(CVMCCMreturnFromSyncMethod)
ENTRY1( CVMCCMreturnFromSyncMethod )
	b returnToInterpreter

	
/* We only support fastlocking with swap/spinlock based microlocks */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
	ENTRY(CVMCCMreturnFromSyncMethod)
ENTRY1( CVMCCMreturnFromSyncMethod )
	/* GC check - gc will patch at this location when a rendezvous is 
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will 
	 * be a "b CVMARMhandleGCForReturnFromSyncMethod" 
	 */

        /*
           r0 = a1 = ee
           r1 = a2 = obj
           r4 = v1 = JFP
           r5 = v2 = JSP
           r7 = v4 = PREV
        */

#define EE          r0
#define OBJ         r1
#define MICROLOCK   r8
#define LOCKREC     ip

/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        textrmuw MICROLOCK, W_MICROLOCK, #0
        mov     r3, #CVM_MICROLOCK_LOCKED
#else
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
	/* Set up r3 for swp below */
        mov     r3, #CVM_MICROLOCK_LOCKED   /* Swap CVM_MICROLOCK_LOCKED into */
	/* Get address of object microlock */
        ldr     MICROLOCK, [MICROLOCK]
#endif

        /* see if previous frame is compiled or not  */
	/* PREV is set up by all code that branches here */

        tst	PREV, #CONSTANT_CVM_FRAME_MASK_SLOW
	ldr	TEMP, [MICROLOCK]
	bne	returnToInterpreter

        /* Do fastTryUnlock(): */

        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        ldr     OBJ, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]

        /* Acquire the microlock: */
#ifndef CVM_MP_SAFE
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */

	/* Get LOCKREC. INVARIANT: All branches and fallthroughs to _unlockObj */
	/* have to set up LOCKREC first */
        ldr     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits.*/
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _fastUnlockAcquireMicrolock /* Branch if failed. */
#else
LABEL(_syncReturnMicrolockMPSafe)
	ldrex	TEMP2, [MICROLOCK]
	cmp	TEMP2, #CVM_MICROLOCK_UNLOCKED /* is the lock free? */
	bne	_fastUnlockAcquireMicrolock    /* already locked */
	strex	TEMP2, r3, [MICROLOCK]       /* acquire the lock */
	cmp	TEMP2, #0                      /* strex succeed? */
	bne	_syncReturnMicrolockMPSafe     /* No. Try again */
	mcr	p15, 0, TEMP2, c7, c10, 5      /* memory barrier */
	ldr     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits.*/
#endif

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
	ldr     TEMP, SYMBOL(fastMlockimplCount)
	ldr	TEMP2, [TEMP]
	add	TEMP2, TEMP2, #1
	str	TEMP2, [TEMP]
#endif
        /* The microlock has been acquired: */
LABEL(_unlockObj)
        /* Check to see if the object is locked with a fastlock: */
        tst     LOCKREC, #0x3        /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne     _fastTryUnlockFailed  /* If not, we failed. */

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_owner]
	/* Optimistically load count */
        ldr     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
	/* Are we the owner? */
        cmp     r3, EE
        bne     _fastTryUnlockFailed    /* If not, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        subs    TEMP, TEMP, #1
        bne     _fastTryUnlockSuccess   /* If not zero, we are done. */

        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        mov     TEMP2, #CVM_MICROLOCK_UNLOCKED
        str     r3, [OBJ, #OFFSET_CVMObjectHeader_various32]

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
	/* Now TEMP is not going to be the entry count anymore */
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_FREE
        str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_state]
        mov     TEMP, #0
        str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_object]
	str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
#endif

	/* r3 = ee->objLocksOwned  (advanced load for below) */
        ldr     r3, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

#ifdef CVM_MP_SAFE
	mov	TEMP, #0
	mcr	p15, 0, TEMP, c7, c10, 5       /* memory barrier. */
#endif
	/* Release the microlock: */
        str     TEMP2, [MICROLOCK]

        /* Check if the lockrec is the first one on the thread's owned list: */
        cmp     r3, LOCKREC
        bne     _fastTryUnlockFindPrevLockRecordLoop

        /* Remove the lockrec from the ee's owned list: */
        ldr     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]

	/* INVARIANT:	All branches and fallthroughs to  
	 * _fastTryUnlockAddLockRecordToFreeList must set up r3 to be 
	 * ee->objLocksFreeOwned first 
	 */

        ldr     r3, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

LABEL(_fastTryUnlockAddLockRecordToFreeList)
        /* Add the lockrec to the ee's free list: */
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        str     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
	/* Fall through to _fastTryUnlockDone */
LABEL(_fastTryUnlockDone)
#ifdef CVM_TRACE
        /* Do trace method return: */
        mov     r1, JFP
        ldr     r0, [sp, #OFFSET_CVMCCExecEnv_ee]
        bl	SYM_NAME(CCMtraceMethodReturnLocal)
#endif
        /* Restore the previous JFP: 
         * Returning from one compiled method to another. 
         * This, we can do ourselves. 
         * Note: The Java SP is already set. 
	 */
        and     JFP, PREV, #~CONSTANT_CVM_FRAME_MASK_ALL
#ifdef CVMCPU_HAS_CP_REG
	ldr     CVMARM_CP_REGNAME, [JFP, #OFFSET_CVMCompiledFrame_cpBaseRegX]
#endif
        ldr     pc, [JFP, #OFFSET_CVMCompiledFrame_PC]  /* Return to caller. */
        /* End. */

#define PREV_REC r3
LABEL(_fastTryUnlockFindPrevLockRecordLoop)
        ldr     r2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        cmp     r2, LOCKREC
        beq     _fastTryUnlockFoundPrevLockRecord
        mov     PREV_REC, r2
        b       _fastTryUnlockFindPrevLockRecordLoop

LABEL(_fastTryUnlockFoundPrevLockRecord)
        /* Remove the lockrec from the ee's owned list: */
        ldr     r2, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        str     r2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
#undef PREV_REC

	/* Satisfy invariant at _fastTryUnlockAddLockRecordToFreeList */

        ldr     r3, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        b       _fastTryUnlockAddLockRecordToFreeList

LABEL(_fastTryUnlockSuccess)
        /* Set the new re-entry count: */
	/* Decremented before we got here */
        str     TEMP, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
        /* Release the microlock: */
        mov     TEMP, #CVM_MICROLOCK_UNLOCKED
        str     TEMP, [MICROLOCK]
	b	_fastTryUnlockDone

LABEL(_fastUnlockAcquireMicrolock)
        /* Call a C function to acquire the microlock: */
        mov     r0, MICROLOCK
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)

        /* Restore the ee and obj pointer: */
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        ldr     OBJ, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
        ldr     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits.*/

        b       _unlockObj              /* Go unlock the object if possible. */

LABEL(_fastTryUnlockFailed)
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

        /* Let the interpreter handle the hard cases: */
        b       returnToInterpreter

#undef EE
#undef OBJ
#undef MICROLOCK
#undef LOCKREC

        SET_SIZE( CVMCCMreturnFromSyncMethod )

#undef TEMP
#undef TEMP2
#undef ORIG_LR

/* Support for fastlocking with atomic ops */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
 ENTRY(CVMCCMreturnFromSyncMethod)
	; GC check - gc will patch at this location when a rendezvous is
	; needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will
	; be a "b CVMARMhandleGCForReturnFromSyncMethod"

        /*
           r0 = a1 = ee
           r1 = a2 = obj
           r4 = v1 = JFP
           r5 = v2 = JSP
           r7 = v4 = PREV
        */

/* TODO: This CVMCCMreturnFromSyncMethod code has not undergone thorough
   multi-threaded stress testing yet.  It needs to undergo stress testing
   before release. */

#if 0
        ;
        ; If you just want to call the C helper and write very little assembler
        ; code, then just to branch to (and implement) returnToInterpreter.
        ;
        b       returnToInterpreter
#endif

#define EE              r8
#define OBJ             r1
#define OWNEDREC        r11
#define EXPECTED_CNT    r10
#define TEMP            r3
#define TEMP2           ip

        ;
        ; see if previous frame is compiled or not 
        ; PREV is set up by all code that branches here
        ;
        tst     PREV, #CONSTANT_CVM_FRAME_MASK_SLOW
        bne     returnToInterpreter

        /* Do fastTryUnlock(): */

        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        ldr     OBJ, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]

        /* Check to see if the object is locked with a fastlock: */
        ldr     OWNEDREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* obits */
        tst     OWNEDREC, #0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne     _fastTryUnlockFailed /* If not, we failed. */

        /* If the OWNEDREC is NULL, it is currently being inflated */
        cmp     OWNEDREC, #0
        beq     _fastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_owner]
        cmp     TEMP, EE
        bne     _fastTryUnlockFailed    /* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        ldr     EXPECTED_CNT, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        cmp     EXPECTED_CNT, #CONSTANT_CVM_INVALID_REENTRY_COUNT
        beq     _fastTryUnlockFailed

#define ACTUAL_CNT      r0
#define NEW_COUNT       r2
        subs    NEW_COUNT, EXPECTED_CNT, #1
        beq     _doUnlock       /* If zero, then unlock */

        ; new monitor count > 0, so just update it and we're done:
        add     r0, OWNEDREC, #OFFSET_CVMOwnedMonitor_count
        mov     r1, EXPECTED_CNT
        ; r2 has already has NEW_COUNT.
        /* NOTE: We don't need the value of the lr register.  Hence, we don't
                 need to save and restore it around this call: */
        CALL_VM_FUNCTION(InterlockedTestExchange)
        cmp     ACTUAL_CNT, EXPECTED_CNT        /* ACTUAL_CNT is in r0. */
        bne     _fastTryUnlockFailed            /* Go slow way. */

        ; we're done! monitor count was successfully decrement
        b       _fastTryUnlockDone
        /* End. */

#undef ACTUAL_CNT
#undef NEW_COUNT

_doUnlock

#define ACTUAL_BITS     r0
#define EXPECTED_BITS   OWNEDREC
#define NEW_BITS        r2

        /* If we get here, then the re-entry count has reached 0. */
        ; Restore the obits to the object header:
        ldr     NEW_BITS, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]

        add     r0, OBJ, #OFFSET_CVMObjectHeader_various32
        mov     r1, EXPECTED_BITS
        ; r2 already has NEW_BITS.
        /* NOTE: We don't need the value of the lr register.  Hence, we don't
                 need to save and restore it around this call: */
        CALL_VM_FUNCTION(InterlockedTestExchange)
        cmp     ACTUAL_BITS, EXPECTED_BITS      /* ACTUAL_BITS is in r0. */
        bne     _fastTryUnlockFailed            /* Go slow way. */

#undef ACTUAL_BITS
#undef EXPECTED_BITS
#undef NEW_BITS

#ifdef CVM_DEBUG
        ; Make the ownedRec play nice with the debug assertions:
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_FREE
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
        mov     TEMP, #0
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_object]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
#endif

        ; Check if the ownedRec is the first one on the thread's owned list:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        cmp     TEMP, OWNEDREC
        bne     _fastTryUnlockFindPrevLockRecord

        ; Remove the ownedRec from the ee's owned list: 
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

_fastTryUnlockAddLockRecordToFreeList
        ; Add the lockrec to the ee's free list:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* Fall thru to _fastTryUnlockDone below: */

_fastTryUnlockDone
        ; we're done! lock was successfully released

#ifdef CVM_TRACE
        ; Do trace method return
        mov     r1, JFP
        ldr     r0, [sp, #OFFSET_CVMCCExecEnv_ee]
        bl      CCMtraceMethodReturnLocal
#endif
        ; Restore the previous JFP
        ; Returning from one compiled method to another.
        ; This, we can do ourselves.
        ; Note: The Java SP is already set.
        and     JFP, PREV, #~CONSTANT_CVM_FRAME_MASK_ALL
#ifdef CVMCPU_HAS_CP_REG
	ldr     CVMARM_CP_REGNAME, [JFP, #OFFSET_CVMCompiledFrame_cpBaseRegX]
#endif
        ldr     pc, [JFP, #OFFSET_CVMCompiledFrame_PC]  ; Return to caller.

/* NOTE: PREV_REC must be TEMP because the following code depends on TEMP
   containing the initial value of PREV_REC before branching here: */
#define PREV_REC TEMP
_fastTryUnlockFindPrevLockRecord
        ldr     TEMP2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        cmp     TEMP2, OWNEDREC
        beq     _fastTryUnlockFoundPrevLockRecord
        mov     PREV_REC, TEMP2
        b       _fastTryUnlockFindPrevLockRecord

_fastTryUnlockFoundPrevLockRecord
        ; Remove the lockrec from the ee's owned list:
        ldr     TEMP2, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     TEMP2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        b       _fastTryUnlockAddLockRecordToFreeList
#undef PREV_REC

_fastTryUnlockFailed
        ; Let the interpreter handle the hard cases:
        b       returnToInterpreter

#undef EE
#undef OBJ
#undef OWNEDREC
#undef EXPECTED_CNT
#undef TEMP
#undef TEMP2

        SET_SIZE( CVMCCMreturnFromSyncMethod )

#else
#error unsupported CVM_FASTLOCK_TYPE or CVM_MICROLOCK_TYPE
#endif 

#ifdef CVM_TRACE

	ENTRY(CVMCCMtraceMethodCallGlue)
ENTRY1(CVMCCMtraceMethodCallGlue)
LABEL(CVMCCMtraceMethodCallGlueLocal)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, SYMBOL(CVMglobals)
#endif

	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	streq lr, [JFP, #OFFSET_CVMCompiledFrame_PC]
 	moveq pc, lr
#define SAVESET {r0, r1, lr}
	FIXUP_FRAMES(JFP, SAVESET, 3)
#undef SAVESET
	mov r2, #0		/* isJump */
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodCall)
	SET_SIZE(CVMCCMtraceMethodCallGlue)

	ENTRY(CCMtraceMethodReturn)
ENTRY1(CCMtraceMethodReturn)
LABEL(CCMtraceMethodReturnLocal)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, SYMBOL(CVMglobals)
#endif
	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	moveq pc, lr
#define SAVESET {r0, r1, lr}
	FIXUP_FRAMES(JFP, SAVESET, 3)
#undef SAVESET
	BRANCH_TO_VM_FUNCTION(CVMtraceMethodReturn)
	SET_SIZE(CCMtraceMethodReturn)

	ENTRY(CCMtraceFramelessMethodCall)
ENTRY1(CCMtraceFramelessMethodCall)
LABEL(CCMtraceFramelessMethodCallLocal)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, SYMBOL(CVMglobals)
#endif
	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	moveq pc, lr
	mov r3, #0			/* isJump */
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodCall)
	SET_SIZE(CCMtraceFramelessMethodCall)

	ENTRY(CCMtraceFramelessMethodReturn)
ENTRY1(CCMtraceFramelessMethodReturn)
LABEL(CCMtraceFramelessMethodReturnLocal)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, SYMBOL(CVMglobals)
#endif
	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	moveq pc, lr
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodReturn)
	SET_SIZE(CCMtraceFramelessMethodReturn)

#endif /* CVM_TRACE */

	POOL
