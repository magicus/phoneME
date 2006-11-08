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
	SET_SECTION_EXEC(ccminvokers_cpu)
#else
	SET_SECTION_EXEC_WRITE
#endif

	ENTRY( CVMCCMinvokeNonstaticSyncMethodHelper )
ENTRY1( CVMCCMinvokeNonstaticSyncMethodHelper )
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

#define CVM_MICROLOCK_LOCKED    0xff
#define CVM_MICROLOCK_UNLOCKED  0x00
#define EE          r0
#define OBJ         r1
#define OBITS       r3
#define MICROLOCK   r8
#define LOCKREC     ip
#define TEMP        r9
#define TEMP2       r10
#define ORIG_LR     TEMP2
	
#define MB   r0
#define PREV CVMARM_PREVFRAME_REGNAME     /* v3 = r6  */

#ifdef CVM_JIT_CCM_USE_C_HELPER
	bl letInterpreterDoInvoke
#else
	
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw MICROLOCK, W_MICROLOCK, #0
	str     MB, [NEW_JFP, #OFFSET_CVMFrame_mb]
#else
        ldr     MICROLOCK, =CVMobjGlobalMicroLockPtr
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
        swp     TEMP, TEMP, [MICROLOCK]      /*    the lockWord. */

	/* Get obits. INVARIANT: All branches and fallthroughs to _lockObj */
	/* have to set up OBITS first */

        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] 
        cmp     TEMP, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _fastLockAcquireMicrolock   /* Branch if failed. */

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
	ldr     TEMP, =fastMlockimplCount
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
	CALL_VM_FUNCTION(CVMCCMtraceMethodCallGlue)
	mov lr, ORIG_LR
#endif

#endif /* CVM_JIT_CCM_USE_C_HELPER */

	ENTRY( CVMARMgcPatchPointAtInvoke )
ENTRY1( CVMARMgcPatchPointAtInvoke )
	/* GC check - gc will patch at this location when a rendezvous is 
	 * needed. See ccmGcPatchPoints in jitinit_cpu.c. The patch will 
	 * be a "b CVMARMhandleGCAtInvoke" 
	 */

        mov	pc, lr	/* Return to method after handling possible GC request */

	ENTRY( CVMARMhandleGCAtInvoke)
ENTRY1( CVMARMhandleGCAtInvoke)
#ifndef CVM_JIT_CCM_USE_C_HELPER

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
        ldrb r1, [r1, #OFFSET_CVMMethodBlock_accessFlagsX]
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
	ldr ip, =CVMglobals
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
        ldrb r2, [r1, #OFFSET_CVMMethodBlock_accessFlagsX]
        tst  r2, #CONSTANT_METHOD_ACC_SYNCHRONIZED
        beq  CVMARMgcPatchPointAtInvoke

	/* Restore receiverObjX in new frame */
	ldr r0, [r0, #OFFSET_CVMExecEnv_miscICell]	/* r0 = &miscICell */
	ldr ip, [r0]
	str ip, [JFP, #OFFSET_CVMCompiledFrame_receiverObjX]
	
	/* And clear miscICell for other uses */
	mov ip, #0
	str ip, [r0]

	/* reload the ee. The frame flush needs it at the return point */
	ldr  r0, [sp, #OFFSET_CVMCCExecEnv_ee]		/* r0 = ee */

	b CVMARMgcPatchPointAtInvoke

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

#undef CVM_MICROLOCK_LOCKED
#undef CVM_MICROLOCK_UNLOCKED
#undef EE
#undef OBJ
#undef OBITS
#undef LOCKREC

#endif /* CVM_JIT_CCM_USE_C_HELPER */
	SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )

	ENTRY( CVMCCMinvokeStaticSyncMethodHelper )
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
	b	CVMCCMinvokeNonstaticSyncMethodHelper

	SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

	ENTRY( CVMCCMinvokeCNIMethod )
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
	CALL_VM_FUNCTION(CCMtraceFramelessMethodCall)
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
	CALL_VM_FUNCTION(CCMtraceFramelessMethodReturn)
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

	ENTRY( CVMCCMinvokeJNIMethod )
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

	ENTRY( CVMCCMletInterpreterDoInvoke )
ENTRY1( CVMCCMletInterpreterDoInvoke )
LABEL(letInterpreterDoInvoke_store_lr)
        str lr,[JFP,#OFFSET_CVMCompiledFrame_PC]

	ENTRY( CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr )
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
	add	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) & ~7)
	/* r4-r10 = v1-v7 */
	ldmfd	sp!, {r4-r10, fp, ip, pc}

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
	ldr	lr, =CVMglobals
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
	ENTRY( CVMARMhandleGCForReturnFromMethod )
ENTRY1( CVMARMhandleGCForReturnFromMethod )
	ldr	r7, =CVMCCMreturnFromMethod
	b	handleGCForReturn	

	ENTRY( CVMARMhandleGCForReturnFromSyncMethod )
ENTRY1( CVMARMhandleGCForReturnFromSyncMethod )
	ldr	r7, =CVMCCMreturnFromSyncMethod
	b	handleGCForReturn	

/* 
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 *
 * CVMMethodBlock* CVMCCMreturnFromMethod(); 
 */ 
	ENTRY( CVMCCMreturnFromMethod )
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
	CALL_VM_FUNCTION(CCMtraceMethodReturn)
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

	ENTRY( CVMCCMreturnToInterpreter )
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
	add	sp, sp, #((CONSTANT_CVMCCExecEnv_size+7) & ~7)
	/* r4-r10 = v1-v7 */
	ldmfd	sp!, {r4-r10, fp, ip, pc}
        SET_SIZE( CVMCCMreturnFromMethod )

/* 
 * Native code doing a synchronized return comes here. 
 *
 * CVMMethodBlock* 
 * CVMCCMreturnFromSyncMethod(); 
 */ 
	ENTRY( CVMCCMreturnFromSyncMethod )
ENTRY1( CVMCCMreturnFromSyncMethod )
#ifdef CVM_JIT_CCM_USE_C_HELPER
	b returnToInterpreter
#else
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

#define CVM_MICROLOCK_LOCKED    0xff
#define CVM_MICROLOCK_UNLOCKED  0x00
#define EE          r0
#define OBJ         r1
#define MICROLOCK   r8
#define LOCKREC     ip

/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        textrmuw MICROLOCK, W_MICROLOCK, #0
        mov     r3, #CVM_MICROLOCK_LOCKED
#else
        ldr     MICROLOCK, =CVMobjGlobalMicroLockPtr
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
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */

	/* Get LOCKREC. INVARIANT: All branches and fallthroughs to _unlockObj */
	/* have to set up LOCKREC first */

        ldr     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits.*/
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _fastUnlockAcquireMicrolock /* Branch if failed. */

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
	ldr     TEMP, =fastMlockimplCount
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
        CALL_VM_FUNCTION(CCMtraceMethodReturn)
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

#undef CVM_MICROLOCK_LOCKED
#undef CVM_MICROLOCK_UNLOCKED
#undef EE
#undef OBJ
#undef MICROLOCK
#undef LOCKREC

#endif
        SET_SIZE( CVMCCMreturnFromSyncMethod )

#ifdef CVM_TRACE

	ENTRY(CVMCCMtraceMethodCallGlue)
ENTRY1(CVMCCMtraceMethodCallGlue)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, =CVMglobals
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
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, =CVMglobals
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
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, =CVMglobals
#endif
	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	moveq pc, lr
	mov r3, #0			/* isJump */
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodCall)
SET_SIZE(CCMtraceFramelessMethodCall)

	ENTRY(CCMtraceFramelessMethodReturn)
ENTRY1(CCMtraceFramelessMethodReturn)
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw ip, W_CVMGLOBALS, #0
#else
	ldr ip, =CVMglobals
#endif
	ldr ip, [ip, #OFFSET_CVMGlobalState_debugFlags]
	tst ip, #CONSTANT_TRACE_METHOD
	moveq pc, lr
	BRANCH_TO_VM_FUNCTION(CVMtraceFramelessMethodReturn)
SET_SIZE(CCMtraceFramelessMethodReturn)

#endif

	POOL
