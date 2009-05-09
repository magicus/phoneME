/*
 * @(#)ccmglue_cpu.S	1.33 06/10/10
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

/*
 * The file includes glue code that aids in the calling of some the
 * CCM C helpers, and also do shortcuts to try to avoid having
 * to call the helpers.
 */
	
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/sync.h"

        /* Monitor enter/exit code may post an event which will
         * push a JNI local frame on the stack.  Need to have frame->TOS
         * consistent with JSP
         */
#ifdef CVM_JVMTI
#define FLUSH_TOS	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
#else
#define FLUSH_TOS
#endif

/*
 * Make a pc-relative branch instruction. Used for patching and instruction
 * to jump around generated code.
 */	
#define MAKE_BRANCH(offset)					\
    ((0x4 << 26) |			 /* BEQ */		\
     (((offset - 4) >> 2) & 0x00ffff))	/* branch offset */ 

/*
 * Macro for calling a helper. Sets up the ccee and ee as first two
 * arguments and flushes the pc to the frame and the frame to the 
 * interpreter stack.
 */
#define CALL_HELPER_AND_PASS_EE(HELPER)					\
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP) ;			\
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE) ; \
	addiu	a0, sp, OFFSET_CStack_CCEE ;				\
	move	a1, EE ;						\
	BRANCH_TO_VM_FUNCTION(HELPER)
/*
 * Macro for calling a helper. Sets up the ccee as the first
 * argument and flushes the pc to the frame and the frame to the 
 * interpreter stack. It also passes the address of the cache or
 * guess value in a3. In order to keep the stackmap happy, it also
 * uses this value as the pc flushed to the frame instead of the
 * actual return address, which is just after the cache address.
 */
#define CALL_HELPER_AND_PASS_CACHE_ADDR(HELPER)				\
	subu	a3,  ra, 4 ;						\
        sw	ra,  OFFSET_CVMCompiledFrame_PC(JFP) ;			\
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE) ; \
	addiu	a0, sp, OFFSET_CStack_CCEE ;				\
	BRANCH_TO_VM_FUNCTION(HELPER)

/* call CVMCCMruntimeSimpleSyncUnlock */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
ENTRY ( CVMCCMruntimeSimpleSyncUnlockGlue )
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	BRANCH_TO_VM_FUNCTION(CVMCCMruntimeSimpleSyncUnlock)
SET_SIZE ( CVMCCMruntimeSimpleSyncUnlockGlue )
#endif
	
/* do a gc rendezvous */
ENTRY ( CVMCCMruntimeGCRendezvousGlue )
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)
SET_SIZE ( CVMCCMruntimeGCRendezvousGlue )

/* Throw various well known exceptions */

ENTRY ( CVMCCMruntimeThrowNullPointerExceptionGlue )
ccmRuntimeThrowNullPointerExceptionGlue:	
	FIXUP_FRAMES_ra(JFP, a2)
	LA(a2, java_lang_NullPointerException_Classblock)
	li	a3, 0  # exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
SET_SIZE ( CVMCCMruntimeThrowNullPointerExceptionGlue )
ENTRY ( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )
	FIXUP_FRAMES_ra(JFP, a2)
	LA(a2, java_lang_ArrayIndexOutOfBoundsException_Classblock)
	li	a3, 0  # exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
SET_SIZE ( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )
ENTRY ( CVMCCMruntimeThrowDivideByZeroGlue )
	FIXUP_FRAMES_ra(JFP, a2)
	LA(a2, java_lang_ArithmeticException_Classblock)
	LA(a3, divideByZeroString)	# exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
SET_SIZE ( CVMCCMruntimeThrowDivideByZeroGlue )

/* Throw an object */
ENTRY ( CVMCCMruntimeThrowObjectGlue )
	FIXUP_FRAMES_a2ra(JFP, a1)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowObject)
SET_SIZE ( CVMCCMruntimeThrowObjectGlue )

/* do a checkcast */
ENTRY ( CVMCCMruntimeCheckCastGlue )
	/*
         * a0 = object to cast
         * a2 = castCb
	 * ra = guess cb address
	 * cc = eq if a0 is null
	 */
	addu	ra, ra, 4	# skip passed the guess addr to the return addr
	beq	a0, zero, 1f	# null
	lw	a1, (a0)	# a1 = object.cb
	lw	a0, -4(ra)	# load the guess cb
	li	a3, ~3
	and	a1, a1, a3	# mask off low bits of object cb
	/* see if guess is correct */
	bne	a0, a1, inline_superclass_checkcast
1:
	move	v0, a0
	jr	ra		# return, null or same as last successful cast

inline_superclass_checkcast:
/* IAI - 21 */
#if defined(IAI_FAST_ASSIGNABLE_CHECKING) && !defined(CVM_AOT)
	/* Before calling helper, do a quick inline check cast. 
	 * Compare the cast cb with current object's class, super class, 
         * super super class ...  until to the super class is NULL.
         */
 	move 	v0, a1   /* v0 = object.cb */

FASTCHECKING1:
	bne	a2, v0, 1f  /* cast cb != object.cb */
	sw	a1, -4(ra)  /* save the matched cb into cache */
	jr	ra          /* match, so return */
1:	lw	v0, OFFSET_CVMClassBlock_superClassCb(v0) /* load super class*/
	bne	v0, zero, FASTCHECKING1 /* if superCb not NULL then continue */
#endif /* IAI_FAST_ASSIGNABLE_CHECKING */

call_checkcast_helper:
	/* need to call the helper */
	/*
	 * a1 = objectCb
	 * a2 = instanceCb
	 * a3 = cache address (to be setup by CALL_HELPER_AND_PASS_CACHE_ADDR)
	 */
	FIXUP_FRAMES_a1a2ra(JFP, a0)
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeCheckCast)
SET_SIZE ( CVMCCMruntimeCheckCastGlue )

/* do an instanceof check */
ENTRY ( CVMCCMruntimeInstanceOfGlue )
	/*
         * a0 = object to cast
         * a2 = instanceofCb
	 * lr = guess cb address
	 * cc = eq if a0 is null
	 */
	addu	ra, ra, 4	# skip passed the guess addr to the return addr
	beq	a0, zero, 1f	# null
	lw	a1, (a0)	# a1 = object.cb
	lw	a0, -4(ra)	# load the guess cb
	li	a3, ~3
	and	a1, a1, a3	# mask off low bits of object cb
        /* see if guess is correct */
	bne	a0, a1, inline_superclass_instanceof
	li	v0, 1		# return TRUE if equal
	jr	ra
1:
	move	v0, a0
	jr	ra

inline_superclass_instanceof:
/* IAI - 21 */
#if defined(IAI_FAST_ASSIGNABLE_CHECKING) && !defined(CVM_AOT)
	/* Before calling helper, do a quick inline check cast. 
	 * Compare the cast cb with current object's class, super class, 
         * super'super class ...  until to the super class is NULL.
         */
 	move 	a0, a1   /* a0 = object.cb */

FASTCHECKING2:
	bne	a2, a0, 2f  /* cast cb != object.cb */
	li	v0, 1       /* return TRUE if a match is fine */
	sw	a1, -4(ra)  /* save the matched cb into cache */
	jr	ra          /* match, so return */
2:	lw	a0, OFFSET_CVMClassBlock_superClassCb(a0) /* load super class*/
	bne	a0, zero, FASTCHECKING2 /* if superCb not NULL then continue */
#endif /* IAI_FAST_ASSIGNABLE_CHECKING */


	/* need to call the helper */
	/*
	 * a1 = objectCb
	 * a2 = instanceCb
	 * a3 = cache address (to be setup by CALL_HELPER_AND_PASS_CACHE_ADDR)
	 */
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeInstanceOf)
SET_SIZE ( CVMCCMruntimeInstanceOfGlue )

/* check reference array assignment capatibility */
ENTRY ( CVMCCMruntimeCheckArrayAssignableGlue )
	/*
         * a2 = cb of array object (with low bits still set)
         * a3 = cb of rhs object (with low bits still set) 
	 */
	li	a1, ~3
	and	a2, a2, a1	# clear low bits of arraycb
	lw	a2, OFFSET_CVMClassBlock_arrayInfoX(a2) # arraycb->arrayInfo
	and	a3, a3, a1	# clear low bits of rhs cb
	lw	a2, OFFSET_CVMArrayInfo_elementCb(a2)   # arrayInfo->elemCb
	beq	a2, a3, 1f	# check if elemClass(arr) == class(rhs)
	LA(a1, java_lang_Object_Classblock)
	beq	a2, a1, 1f	# check if elemClass(arr) == Object
	/*
	 * a2 = element cb of array object (with low bits clear)
	 * a3 = cb of rhs object (with low bits clear)
	 */
	FIXUP_FRAMES_a2a3ra(JFP, a1)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeCheckArrayAssignable)
1:
	jr	ra
SET_SIZE ( CVMCCMruntimeCheckArrayAssignableGlue )

/* run the clinit of a class */
ENTRY ( CVMCCMruntimeRunClassInitializerGlue )
	/*
         * a2 = target cb
	 * ra = return address
	 */
	/* setup ccee and ee arguments */
	addiu	a0, sp, OFFSET_CStack_CCEE
	move	a1, EE
	/* flush state */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(a1)

	/* call the helper. If the clinit has to be run, it will return
	 * directly to the method that called us. */
	CALL_VM_FUNCTION(CVMCCMruntimeRunClassInitializer)

	/* reload the return address */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	bne	v0,  zero, 1f	# FALSE means current thread is running clinit
	jr	ra

1:
	/* At this point we know the class has been intialized. Patch the 
	 * the call to this helper to be a nop.
	 * WARNING: Don't attempt to patch over the lw of the cb with
	 * a branch around the call to the helper. The lw may be the
	 * first instruction of a block, which means we may also try to
	 * patch it when doing a gc-rendezvous. */
#ifdef CVM_AOT
	/* Don't patch AOT code */
	LA	(a1, CVMJITcodeCacheAOTEnd)
	lw	a1, 0(a1)
	lw	a1, 0(a1)
	sltu	a1, ra, a1
	beq	a1, zero, doPatch
	LA	(a1, CVMJITcodeCacheAOTStart)
	lw	a1, 0(a1)
	lw	a1, 0(a1)
	sltu	a1, ra, a1
	bne	a1, zero, doPatch
	jr	ra
#endif

doPatch:
	/* get the patch, a nop instruction */
	sw	zero, -8(ra)	/* apply the patch */
	/* flush the patched instruction */
	addiu	a0, ra, -8	/* begin */
	addiu	a1, a0, 4       /* end */

	CALL_VM_FUNCTION(CVMJITflushCache)

	/* return to the helper */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra
SET_SIZE ( CVMCCMruntimeRunClassInitializerGlue )
	
/*
 * Resolve a cp entry and run the clinit if necessary
 */
ENTRY ( CVMCCMruntimeResolveGlue )
ccmRuntimeResolveGlue:	
	/*
         * a2 = cpIndex
	 * ra = address of cachedConstant, 4 bytes before returnAddress
	 * jp = address of ccm helper to call
	 */
	/* setup remaining arguments and call the helper */
	/* a0 = ccee */
	addiu	a0, sp, OFFSET_CStack_CCEE
	/* a1 = ee */
	move	a1, EE
	move	a3, ra		# a3 = address of cachedConstant
	/* flush state */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	add	ra,  ra, 4	# lr = address we want to return to
        sw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(a1)
	/* call the helper */
	jalr	jp

	/* reload the return address */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	bne	v0,  zero, 1f	# FALSE means current thread is running clinit
	jr	ra
1:
	/* At this point we know that any needed class initialization has
	 * been done. Patch the first instruction of the generated code
	 * to branch around the call to this helper, and also around
	 * the cachedConstant word. */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define RESOLVE_PATCH_OFFSET 12
#else
#define RESOLVE_PATCH_OFFSET 12
#endif
	LA(a1,  MAKE_BRANCH(RESOLVE_PATCH_OFFSET)) /* get the patch */
	sw	a1, -RESOLVE_PATCH_OFFSET(ra)       /* apply the patch */
	/* flush the patched instruction */
	subu	a0,  ra, RESOLVE_PATCH_OFFSET	/* begin */
	addiu	a1,  a0, 4		        /* end */
	CALL_VM_FUNCTION(CVMJITflushCache)

	/* return to the helper */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra
SET_SIZE ( CVMCCMruntimeResolveGlue )
	
#define RES(x)				\
ENTRY(x##Glue);				\
	LA(jp, x);			\
	b	ccmRuntimeResolveGlue;	\
SET_SIZE(x##Glue)

RES(CVMCCMruntimeResolveNewClassBlockAndClinit )
RES(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)
RES(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)
RES(CVMCCMruntimeResolveStaticMethodBlockAndClinit)
RES(CVMCCMruntimeResolveClassBlock)
RES(CVMCCMruntimeResolveArrayClassBlock)
RES(CVMCCMruntimeResolveGetfieldFieldOffset)
RES(CVMCCMruntimeResolvePutfieldFieldOffset)
RES(CVMCCMruntimeResolveSpecialMethodBlock)
RES(CVMCCMruntimeResolveMethodBlock)


/*
 * Resolve a method table offset. The protocol here is slight
 * different because the result could possibly be an MB pointer
 * rather than just an offset. Calling code looks like this:
 *	ld	destreg,cache
 *	mov	ARG1, cpIndex
 *	call	CVMCCMruntimeResolveMethodTableOffsetGlue
 *	<delay>
 *	mov	mbDestreg,RETURN1 # return direct MB pointer here
 *	b	haveMB
 *	<delay>
 *  cache:
 *	.word	-1
 *  return1:
 *	ld	destreg, cache
 *
 * If the helper returns 0, then the cache will have been filled with
 * the method table offset and we go let the general resolver glue code
 * do the rewrite and return.
 * Otherwise, the helper returned the MB pointer and we return that
 * directly.
 */
ENTRY ( CVMCCMruntimeResolveMethodTableOffsetGlue )
	/*
         * a2 = cpIndex
	 * ra = address of extraordinary return point
	 * +12 = address of cache word
	 * +16 = usual return point
	 */
	/* setup remaining arguments and call the helper */
	/* a0 = ccee */
	addiu	a0, sp, OFFSET_CStack_CCEE
	/* a1 = ee */
	move	a1, EE
	add	a3, ra, 12	# a3 = address of cachedConstant
	/* flush state */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	add	ra,  ra, 16	# lr = address we want to return to
        sw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(a1)
	/* call the helper */
	CALL_VM_FUNCTION( CVMCCMruntimeResolveMethodTableOffset )

	/* reload the return address */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	bne	v0,  zero, 1f	# NULL means cache is patched with offset

	/* At this point we know that the method table offset has been
	 * written to the cache word.
	 * Patch the first instruction of the generated code
	 * to branch around the call to this helper, around the special
	 * case return code, and around the cachedConstant word.
	 */
#define RESOLVE_METHOD_TABLE_PATCH_OFFSET (RESOLVE_PATCH_OFFSET+12)
	/* get the patch */
	LA(a1,  MAKE_BRANCH(RESOLVE_METHOD_TABLE_PATCH_OFFSET))
	/* apply the patch */
	sw	a1, -RESOLVE_METHOD_TABLE_PATCH_OFFSET(ra)
	/* flush the patched instruction */
	subu	a0,  ra, RESOLVE_METHOD_TABLE_PATCH_OFFSET	/* begin */
	addiu	a1,  a0, 4		        /* end */
	CALL_VM_FUNCTION(CVMJITflushCache)

	/* return to the helper */
        lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra

1:	/* here, we got a non-NULL return from the helper.
	 * Return that value at the extraordinary, direct-MB
	 * return address
	 */
	subu	ra, ra, 16
	jr	ra
SET_SIZE ( CVMCCMruntimeResolveMethodTableOffsetGlue )


	/* lookup an interface mb */
ENTRY ( CVMCCMruntimeLookupInterfaceMBGlue )
	/*
         * a1 = object to invoke with
         * a2 = interface mb
	 * ra = address of guess from last interface mb lookup
	 */

	add	ra, ra, 4	# skip pass the guess addr to the return addr

#define OCB           a1
#define INTERFACE_MB  a2
#define OINTERFACES   a3

        /* ocb = CVMobjectGetClass(obj); */
        lw	OCB, OFFSET_CVMObjectHeader_clas(a1) # get ocb from obj
	li	a3, ~3
        and     OCB, OCB, a3		# ocb &= ~0x3; prepare ocb

        /* interfaces = ocb->interfacesX */
        lw	OINTERFACES, OFFSET_CVMClassBlock_interfacesX(OCB)
	/* Check if ocb->interfacesX == NULL. */
        beq     OINTERFACES, zero, call_lookup_helper
       
        /* ointerfaceCount = ocb->interfacesX->interfaceCountX; */ 
        lhu    a0, OFFSET_CVMInterfaces_interfaceCountX(OINTERFACES)

#define GUESS   v1
        lw	GUESS, -4(ra)		# load guess value
        sltu	t0, GUESS, a0		# cmp guess and ointerfaceCount
	/* if guess >= ointerfaceCount, invalid */
        beq	t0, zero, call_lookup_helper

        /* If we get here than the guess is within the valid range: */
	sll	GUESS, GUESS, CONSTANT_LOG2_CVMInterfaceTable_SIZE
	addu	GUESS, GUESS, OINTERFACES
	/* target ICB = ointerfaces.itable[guess].interfaceCb */
        lw	t0, OFFSET_CVMInterfaces_itable(GUESS)

#ifdef CVM_METHODBLOCK_HAS_CB
        lw     v0, OFFSET_CVMMethodBlock_cbX(INTERFACE_MB) # get icb
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        lbu    v0, OFFSET_CVMMethodBlock_methodIndexX(INTERFACE_MB)

        /* v0 = v0 * 32 - v0 * 4
         * which is v0 = v0 * 28
         */
	sll	t7, v0, 5
	sll	v0, v0, 2
	subu	v0, t7, v0
	/* So now v0 holds 28 * methodIndex */
        subu	v0, INTERFACE_MB, v0
        lw	v0, -OFFSET_CVMMethodRange_mb(v0)
#endif

        /* Check if the guess' interface CB is the one we want: */
	/*
         * test if target ICB == source ICB
	 * go call helper if guess failed.
	 */
        bne	t0, v0, call_lookup_helper

        /* If we get here, then the guess is correct. Go fetch the method
           block from the interface CB: */

#define SOURCE_MB_IDX   t0
        /* get source mb MethodSlotIndex */
        lw     SOURCE_MB_IDX, OFFSET_CVMMethodBlock_codeX(INTERFACE_MB)

#define TARGET_METHODTABLE_INDICES jp
        /* Target methodTableIndices = 
                ocb->interfacesX.itable[guess].methodTableIndicesX;
        */
	lw	TARGET_METHODTABLE_INDICES, OFFSET_CVMInterfaces_itable0_intfInfoX(GUESS)
#undef GUESS
#undef OINTERFACES

        /* Get the interface mb from the ocb's vtbl: */
        lw	v0, OFFSET_CVMClassBlock_methodTablePtrX(OCB)
        sll     t0, SOURCE_MB_IDX, CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE
	addu	t0, t0, TARGET_METHODTABLE_INDICES
        lhu	jp, (t0)

        /* v0 = ocb->methodTablePtrX[jp]: */
	sll	jp, jp, 2
	addu	v0, v0, jp
	lw	v0, (v0)
	jr	ra

call_lookup_helper:
	/*
         * a1 = ocb
         * a2 = imb
         * a3 = guess address (to be setup by
         *      CALL_HELPER_AND_PASS_CACHE_ADDR)
	 */
	FIXUP_FRAMES_a1a2a3ra(JFP, a0)
        CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeLookupInterfaceMB)
SET_SIZE ( CVMCCMruntimeLookupInterfaceMBGlue )

#undef OCB
#undef TARGET_METHODTABLE_INDICES

/*
 * Entry point for monitorenter.
 */
ENTRY ( CVMCCMruntimeMonitorEnterGlue )
        /*
         * Arguments:
         *       a2 = 'obj'
         */
#if 0
	/*
	 * If you just want to call the C helper and write very little
	 * assembler code, then just the following.
	 */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
       /* CAS version of monitorenter: */
       /* Do fastTryLock(): */


#define OBJ		a2
#define OBITS		a3
#define LOCKREC		t7
#define OBITS0		v0
#define EXPECTED_CNT	v1
#define NEW_COUNT	OBITS
#define OLD_COUNT	OBITS0

        /* Make sure that the object is not NULL: */
        beq	OBJ, zero, ccmRuntimeThrowNullPointerExceptionGlue

        /* Setup a lock record and assume the object has not been locked
	   yet:	 */
        lw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        beq	LOCKREC, zero, _monenterRecordNotAvailable

        /* lockrec->object = obj: */
        sw	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

        /* lockrec->count = 1: */
        li	jp, 1		/* Initial lock re-entry count */
        sw	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)

#ifdef CVM_DEBUG
        li	jp, CONSTANT_CVM_OWNEDMON_OWNED
        sw	jp, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	/*
	 * nbits = (LOCKREC) | CVM_LOCKSTATE_LOCKED:	
	 * Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
	 * that nbits is essentially lockrec.  Nothing need to be done to
	 * initialize nbits.
	 */
#define NBITS LOCKREC

	/* obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED*/
        lw	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
	li	jp, ~3
	and	OBITS, OBITS, jp /* clear rightmost 2 bits */
	ori	OBITS, OBITS, CONSTANT_CVM_LOCKSTATE_UNLOCKED

	/* lockrec->u.fast.bits = obits: */
        sw	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

        /* Do atomicCompareAndSwap: */
1:
	move	jp, NBITS
	ll	OBITS0, OFFSET_CVMObjectHeader_various32(OBJ)
	bne	OBITS0, OBITS, _monenterFastFailed
	sc	jp, OFFSET_CVMObjectHeader_various32(OBJ)
	beq	jp, zero, 1b /* retry */
#undef NBITS

        /* Remove lockrec from the ee's free list: */
        lw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	jp, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        /* Add the lockrec to the ee's owned list: */
        /* nextRec = ee->objLocksOwned: */
        lw	jp, OFFSET_CVMExecEnv_objLocksOwned(EE)
        sw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)

        jr	ra	/* Return to the caller */

_monenterFastFailed:
#ifdef CVM_DEBUG
        /* lockrec->state = CONSTANT_CVM_OWNEDMON_FREE: */
        sw	zero, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        li	jp, CONSTANT_CVM_OWNEDMON_FREE
        sw	jp, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	/* If object is not in LOCKED state, then fail: */
	andi	jp, OBITS0, 0x3	/* check for CVM_LOCKSTATE_LOCKED */
	bne	jp, zero, _monenterFastRetryFailed
	
	/* If not associated with a lock record, then fail: */
	li	jp, ~0x3
	and	LOCKREC, OBITS0, jp
	beq	LOCKREC, zero, _monenterFastReentryFailed

	/* If (lockrec->owner != ee), then fail: */
	lw	jp, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
	bne	jp, EE, _monenterFastReentryFailed

	/* If we get here, then we are re-entering the lock: */
	lw	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	li	jp, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	EXPECTED_CNT, jp, _monenterFastReentryFailed

	addi	NEW_COUNT, EXPECTED_CNT, 1
1:
	move	jp, NEW_COUNT
	ll	OLD_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	bne	OLD_COUNT, EXPECTED_CNT, _monenterFastReentryFailed
	sc	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	beq	jp, zero, 1b	/* retry */

	/* If the old count is as expected, then we are successful: */
        jr	ra	/* Return to the caller */

_monenterFastRetryFailed:
_monenterFastReentryFailed:
_monenterRecordNotAvailable:
        /* Let C helper do the hard stuff: */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)
	
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
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)
#else
        /* CVM_MICROLOCK_SWAP_SPINLOCK version of monitorenter: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

        /* Do fastTryLock(): */

        /*
           a0 = &microlock
           a1 = lockrec
           a2 = obj
           a3 = obits
	   v0 = scratch
	   v1 = scratch
	   jp = scratch

           NOTE: By design, MICROLOCK is assigned to a0 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monenterSlowAcquireMicrolock.
        */

#define MICROLOCK   a0
#define OBITS       a1
#define OBJ         a2
#define LOCKREC     a3

        /* ld      MICROLOCK, CVMobjGlobalMicroLockPtr */
	LA	(MICROLOCK, CVMobjGlobalMicroLockPtr)

        /* Make sure that the object is not NULL */
        beq	OBJ, zero, ccmRuntimeThrowNullPointerExceptionGlue

	/* Get address of object microlock */
	lw	MICROLOCK, 0(MICROLOCK)

        /* Acquire the microlock: */
0:	
        li	jp, CVM_MICROLOCK_LOCKED 
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	ll	v1, 0(MICROLOCK)
	sc	jp, 0(MICROLOCK)
	beq	jp, zero, 0b /* retry */

	/* Branch if the microlock was already acquired by another thread. */
        bne	v1, CVM_MICROLOCK_UNLOCKED, _monenterSlowAcquireMicrolock

        /* The microlock has been acquired: */
_monenterLockObj:
        lw	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi	v0, OBITS, 0x3
        bne	v0, CONSTANT_CVM_LOCKSTATE_UNLOCKED, _monenterObjAlreadyLocked

        /* If we get here, then the object has not been locked yet. */
        /* lockrec = ee->objLocksFreeOwned: */
        lw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        beq	LOCKREC, zero, _monenterRecordNotAvailable

        /* lockrec->u.fast.bits = obits: */
        sw	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

#ifdef CVM_DEBUG
        /* lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED: */
        li	v0, CONSTANT_CVM_OWNEDMON_OWNED
        sw	v0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif
        /* obj->hdr.various32 = lockrec: */
        sw	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ)

        /* lockrec->count = 1: */
        li	v0, 1		/*  Initial lock re-entry count */
        sw	v0, OFFSET_CVMOwnedMonitor_count(LOCKREC)

        /* lockrec->object = obj: */
        sw	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

        /* Release the microlock:	 */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)	/* microlock->lockWord = UNLOCKED */

        /* Remove lockrec from the ee's free list: */
        /* nextRec = lockrec->next: */
        lw	v0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        /* ee->objLocksFreeOwned = nextRec: */
        sw	v0, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        /* Add the lockrec to the ee's owned list: */
        /* nextRec = ee->objLocksOwned: */
        lw	v0, OFFSET_CVMExecEnv_objLocksOwned(EE)
        /* lockrec->next = nextRec: */
        sw	v0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        /* ee->objLocksOwned = lockrec: */
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)

        jr	ra	/* Return to the caller */

_monenterSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        sw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        sw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        sw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           a0.  By design, MICROLOCK is a0, so we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        lw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        lw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)
        b	_monenterLockObj

_monenterObjAlreadyLocked:
        bne	v0, CONSTANT_CVM_LOCKSTATE_LOCKED, _monenterFastReentryFailed

        /* Make sure the current thread owns this lock: */
        lw	v0, OFFSET_CVMOwnedMonitor_owner(OBITS)
        bne	v0, EE, _monenterFastReentryFailed

        lw	v1, OFFSET_CVMOwnedMonitor_count(OBITS)
        addi	v1, v1, 1
        sw	v1, OFFSET_CVMOwnedMonitor_count(OBITS)

        /* Release the microlock: */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)	/* microlock->lockWord = UNLOCKED */

        jr	ra	/* Return to the caller */

_monenterFastReentryFailed:
_monenterRecordNotAvailable:
        /* Release the microlock: */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)

        /* Load the ccee. The obj arg is already loaded. */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

#undef MICROLOCK
#undef OBJ
#undef OBITS
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

SET_SIZE ( CVMCCMruntimeMonitorEnterGlue )

/*
 * Entry point for monitorexit.
 */
ENTRY ( CVMCCMruntimeMonitorExitGlue )
        /*
         * Arguments:
         *       a2 = 'obj'
         */
#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following.
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)
#endif


#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        /* CAS version of monitorexit: */
        /* Do fastTryUnlock(): */

#define OBJ		a2
#define NBITS		a3
#define LOCKREC		t7
#define OBITS0		v0
#define EXPECTED_CNT	v1
#define NEW_COUNT	NBITS
#define OLD_COUNT	OBITS0

        /* Make sure that the object is not NULL: */
        beq	OBJ, zero, ccmRuntimeThrowNullPointerExceptionGlue

        /* Check to see if the object is locked with a fastlock: */
        lw	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi	jp, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne	jp, zero, _monexitFastTryUnlockFailed /* If not, we failed. */

	/* If the LOCKREC is NULL, it is currently being inflated */
	beq	LOCKREC, zero, _monexitFastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lw	jp, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        bne	jp, EE, _monexitFastTryUnlockFailed /* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lw	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	li	jp, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	EXPECTED_CNT, jp, _monexitFastTryUnlockFailed
        addi	NEW_COUNT, EXPECTED_CNT, -1
        beq	NEW_COUNT, zero, _monexitDoUnlock  /* If zero, then unlock */

	/* new monitor count > 0, so just update it and we are done */
1:
	move	jp, NEW_COUNT
	ll	OLD_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	bne	OLD_COUNT, EXPECTED_CNT, _monexitFastTryUnlockFailed
	sc	jp, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	beq	jp, zero, 1b	/* retry */

	/* we are done! monitor count was successfully decrement */
	jr	ra	/* return to caller */

_monexitDoUnlock:	
#define OBITS LOCKREC
        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lw	NBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
1:
	move	jp, NBITS
	ll	OBITS0, OFFSET_CVMObjectHeader_various32(OBJ)
	bne	OBITS0, OBITS, _monexitFastTryUnlockFailed
	sc	jp, OFFSET_CVMObjectHeader_various32(OBJ)
	beq	jp, zero, 1b		/* retry */
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
        bne	jp, LOCKREC, _monexitFastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee's owned list: */
        lw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	jp, OFFSET_CVMExecEnv_objLocksOwned(EE)

_monexitFastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lw	jp, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        sw	jp, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        jr	ra	/* Return to the caller. */

/* PREV_REC (jp) is the first one on the thread's owned list */
#define PREV_REC jp
/* It is safe to reuse NBITS at this point */
#define NEXT_REC NBITS
_monexitFastTryUnlockFindPrevLockRecord:
        lw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        beq	NEXT_REC, LOCKREC, _monexitFastTryUnlockFoundPrevLockRecord
        move	PREV_REC, NEXT_REC
        b	_monexitFastTryUnlockFindPrevLockRecord

_monexitFastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */
        lw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	NEXT_REC, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC
#undef NEXT_REC

_monexitFastTryUnlockFailed:
        /* Let C helper do the hard stuff: */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

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
	mflr	r7
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)
#else
        /* CVM_MICROLOCK_SWAP_SPINLOCK version of monitorenter: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

        /* Do fastTryUnlock(): */

        /*
           a0 = &microlock
           a2 = obj
           a3 = lockrec
	   v0 = scratch
	   v1 = scratch
	   jp = scratch

           NOTE: By design, MICROLOCK is assigned to a0 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monexitSlowAcquireMicrolock.
        */

#define MICROLOCK   a0
#define OBJ         a2
#define LOCKREC     a3

        /* ld      MICROLOCK, CVMobjGlobalMicroLockPtr */
	LA	(MICROLOCK, CVMobjGlobalMicroLockPtr)

        /* Make sure that the object is not NULL */
        beq	OBJ, zero, ccmRuntimeThrowNullPointerExceptionGlue

	/* Get address of object microlock */
	lw	MICROLOCK, 0(MICROLOCK)

        /* Acquire the microlock: */
0:	
        li	jp, CVM_MICROLOCK_LOCKED 
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	ll	v1, 0(MICROLOCK)
	sc	jp, 0(MICROLOCK)
	beq	jp, zero, 0b /* retry */

	/* Branch if the microlock was already acquired by another thread. */
        bne	v1, CVM_MICROLOCK_UNLOCKED, _monexitSlowAcquireMicrolock

        /* The microlock has been acquired: */
_monexitUnlockObj:
        /* Check to see if the object is locked with a fastlock: */
        lw	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi	v0, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne	v0, zero, _monexitFastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lw	v0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        bne	v0, EE, _monexitFastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lw	v1, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	addi	v1, v1, -1
	bne	v1, zero, _monexitFastTryUnlockSuccess/* we're done if not 0 */

        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lw	v0, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        sw	v0, OFFSET_CVMObjectHeader_various32(OBJ)

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
        li	v0, CONSTANT_CVM_OWNEDMON_FREE
        sw	v0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
        sw	zero, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
        sw	zero, OFFSET_CVMOwnedMonitor_object(LOCKREC)
	sw	zero, OFFSET_CVMOwnedMonitor_count(LOCKREC)
#endif

        /* Release the microlock: */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)  /* *microlock = CVM_MICROLOCK_UNLOCKED */

        /* Check if the lockrec is the first one on the thread owned list: */
        lw	v0, OFFSET_CVMExecEnv_objLocksOwned(EE)
        bne	v0, LOCKREC, _monexitFastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee owned list: */
        lw	v0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	v0, OFFSET_CVMExecEnv_objLocksOwned(EE)

_monexitFastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lw	v1, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        sw	v1, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        jr	ra	/* Return to the caller */

/* v0 is the first one on the thread's owned list */
#define PREV_REC v0
_monexitFastTryUnlockFindPrevLockRecord:
        lw	v1, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        beq	v1, LOCKREC, _monexitFastTryUnlockFoundPrevLockRecord
        move	PREV_REC, v1
        b	_monexitFastTryUnlockFindPrevLockRecord

_monexitFastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */
        lw	v1, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        sw	v1, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC


_monexitFastTryUnlockSuccess:
        /* Set the new re-entry count: */
        sw	v1, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        /* Release the microlock: */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)  /* *microlock = CVM_MICROLOCK_UNLOCKED */

        jr	ra	/* Return to the caller */

_monexitSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a non-volatile reg
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        sw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        sw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        sw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           r3.  By design, MICROLOCK is a0, so we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lw	ra, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        lw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        lw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)
        b       _monexitUnlockObj	/* Go unlock the object if possible */

_monexitFastTryUnlockFailed:
        /* Release the microlock: */
        li	v0, CVM_MICROLOCK_UNLOCKED
        sw	v0, 0(MICROLOCK)

        /* Let C helper do the hard stuff: */
        /* Load the ccee. The obj arg is already loaded. */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

#undef OBJ
#undef MICROLOCK
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

SET_SIZE ( CVMCCMruntimeMonitorExitGlue )

	.text
divideByZeroString:
	.asciiz "/ by zero"
	.align 4

#ifdef CVM_JIT_DEBUG

#define CCEE  sp
#define OFFSET_REGS \
    (OFFSET_CVMCCExecEnv_debugContext + \
     OFFSET_CVMJITDebugContext_cpuContext + \
     OFFSET_CVMJITTargetCPUContext_regs)
#define OFFSET_CPSR \
    (OFFSET_CVMCCExecEnv_debugContext + \
     OFFSET_CVMJITDebugContext_cpuContext + \
     OFFSET_CVMJITTargetCPUContext_cpsr)

/*
 * Entry point for breakpoint polling.
 */
ENTRY ( CVMJITdebugPollBreakpointGlue )
#ifdef TODO
        #
        # Arguments:
        #       lr = saved PC.

        # NOTE: LR has already been saved by the caller to
        #       [CCEE, OFFSET_REGS+(14*4)].  It now holds the compiled pc.
        #       r2 has already been saved by the caller to
        #       [CCEE, #OFFSET_REGS+(2*4)].  It now holds the nodeID.
        #       r3 has already been saved by the caller to
        #       [CCEE, #OFFSET_REGS+(3*4)].  It now holds the description.

        # NOTE: Even though the callee will save the SP, we want to save it
        #       to the storage anyway so that the user can inspect its value.

        sw     lr,  [CCEE, #OFFSET_REGS+(15*4)] # Save LR as PC.  LR

        sw     sp,  [CCEE, #OFFSET_REGS+(13*4)] # Save SP.
        sw     r12, [CCEE, #OFFSET_REGS+(12*4)]
        sw     r11, [CCEE, #OFFSET_REGS+(11*4)]
        sw     r10, [CCEE, #OFFSET_REGS+(10*4)]
        sw     r9,  [CCEE, #OFFSET_REGS+(9*4)]
        sw     r8,  [CCEE, #OFFSET_REGS+(8*4)]
        sw     r7,  [CCEE, #OFFSET_REGS+(7*4)]
        sw     r6,  [CCEE, #OFFSET_REGS+(6*4)]
        sw     r5,  [CCEE, #OFFSET_REGS+(5*4)]
        sw     r4,  [CCEE, #OFFSET_REGS+(4*4)]

        sw     r1,  [CCEE, #OFFSET_REGS+(1*4)]
        sw     r0,  [CCEE, #OFFSET_REGS+(0*4)]

        # Save the CPSR:
        mrs     r1, CPSR
        sw     r1,  [CCEE, #OFFSET_CPSR]

        # Flush the top frame to be consistent:
        sw     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
        sw     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        lw     r1,  [CCEE, #OFFSET_CVMCCExecEnv_ee]
        sw     JFP, [r1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]

        # Call the breakpoint handler.
        mov     r0, CCEE    # arg0 = ccee.
        mov     r1, JFP     # arg1 = frame.
                            # arg2 = nodeID (already set).
                            # arg3 = description (already set).
        CALL_VM_FUNCTION(CVMJITdebugPollBreakpoint)

        # Restore the CPSR:
        lw     r1,  [CCEE, #OFFSET_CPSR]
        msr     CPSR_f, r1

        # Restore the registers:
        lw     r0,  [CCEE, #OFFSET_REGS+(0*4)]
        lw     r1,  [CCEE, #OFFSET_REGS+(1*4)]
        lw     r2,  [CCEE, #OFFSET_REGS+(2*4)]
        lw     r3,  [CCEE, #OFFSET_REGS+(3*4)]
        lw     r4,  [CCEE, #OFFSET_REGS+(4*4)]
        lw     r5,  [CCEE, #OFFSET_REGS+(5*4)]
        lw     r6,  [CCEE, #OFFSET_REGS+(6*4)]
        lw     r7,  [CCEE, #OFFSET_REGS+(7*4)]
        lw     r8,  [CCEE, #OFFSET_REGS+(8*4)]
        lw     r9,  [CCEE, #OFFSET_REGS+(9*4)]
        lw     r10, [CCEE, #OFFSET_REGS+(10*4)]
        lw     r11, [CCEE, #OFFSET_REGS+(11*4)]
        lw     r12, [CCEE, #OFFSET_REGS+(12*4)]
        # No need to restore sp as it was already restored by the callee.
        lw     lr,  [CCEE, #OFFSET_REGS+(14*4)]

        # Return to the caller.
        lw     pc,  [CCEE, #OFFSET_REGS+(15*4)]

#endif
	SET_SIZE( CVMJITdebugPollBreakpointGlue ) 

#undef CCEE
#undef OFFSET_REGS
#undef OFFSET_CPSR

#endif /* CVM_JIT_DEBUG */
