/*
 * @(#)ccmglue_cpu.S	1.39 06/10/10
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
#define FLUSH_TOS	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
#else
#define FLUSH_TOS
#endif
	.section	".text",""

/*
 * Make a pc-relative branch instruction. Used for patching and instruction
 * to jump around generated code.
 */
#define PPC_OPCODE_SHIFT   26  /* opcode in upper 6 bits */
#define PPC_B_OPCODE      (18 << PPC_OPCODE_SHIFT)
#define MAKE_BRANCH(offset)					        \
     (PPC_B_OPCODE | (offset & 0x03fffffc))

/*
 * Macro for calling a helper. Sets up the ccee and ee as first two
 * arguments and flushes the pc to the frame and the frame to the 
 * interpreter stack.
 *
 * On entry, r7 should contain the address to return to.
 */
#define CALL_HELPER_AND_PASS_EE(HELPER)					\
	la	r3,  OFFSET_CStack_CCEE(sp) 				_SE_\
	mr	r4,  EE 						_SE_\
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP) 			_SE_\
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)  _SE_\
	BRANCH_TO_VM_FUNCTION(HELPER)

/*
 * Macro for calling a helper. Sets up the ccee as the 1st
 * argument and flushes the pc to the frame and the frame to the 
 * interpreter stack. It also passes the address of the cache or
 * guess value in the 4th argument, which is assumed to already
 * be setup.
 *
 * On entry, r7 should contain the address to return to.
 */
#define CALL_HELPER_AND_PASS_CACHE_ADDR(HELPER)				\
	la	r3,  OFFSET_CStack_CCEE(sp) 				_SE_\
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP) 			_SE_\
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)  _SE_\
	BRANCH_TO_VM_FUNCTION(HELPER)
	
/* call CVMCCMruntimeSimpleSyncUnlock */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
ENTRY ( CVMCCMruntimeSimpleSyncUnlockGlue )
	mflr	r7
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	BRANCH_TO_VM_FUNCTION(CVMCCMruntimeSimpleSyncUnlock)
SET_SIZE ( CVMCCMruntimeSimpleSyncUnlockGlue )
#endif

/* do a gc rendezvous */
ENTRY ( CVMCCMruntimeGCRendezvousGlue )
	mflr	r7
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)

#if defined(CVMJIT_PATCH_BASED_GC_CHECKS)
ENTRY ( CVMPPCruntimeGCRendezvousRetAddrAdjustGlue )
	/* If we are copying to the code cache, then only one instruction
	 * was needed to call us, so we want to return to it. Otherwise
	 * it may have taken multiple instructions, so the last one is
	 * patched with a nop so we can skip it.
	 */
	mflr	r7
	subi	r7, r7, 4
	mtlr	r7
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)
#endif

/* Throw various well known exceptions */

ENTRY ( CVMCCMruntimeThrowNullPointerExceptionGlue )
CVMCCMruntimeThrowNullPointerExceptionGlueLocal:
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	lis	r5, HA16(java_lang_NullPointerException_Classblock)
	la	r5, LO16(java_lang_NullPointerException_Classblock)(r5)
	li	r6, 0  /* exception message */
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
ENTRY ( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	lis	r5, HA16(java_lang_ArrayIndexOutOfBoundsException_Classblock)
	la	r5, LO16(java_lang_ArrayIndexOutOfBoundsException_Classblock)(r5)
	li	r6, 0  /* exception message */
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
ENTRY ( CVMCCMruntimeThrowDivideByZeroGlue )
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	lis	r5, HA16(java_lang_ArithmeticException_Classblock)
	la	r5, LO16(java_lang_ArithmeticException_Classblock)(r5)
	/* exception message */
	lis	r6, HA16(divideByZeroString)
	la	r6, LO16(divideByZeroString)(r6)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)

/* Throw an object */
ENTRY ( CVMCCMruntimeThrowObjectGlue )
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowObject)

/* do a checkcast */
ENTRY ( CVMCCMruntimeCheckCastGlue )
        # r3 = object to cast
	# r4 = objectCb #ifdef IAI_CACHEDCONSTANT_INLINING
        # r5 = castCb
	# r6 = guess cb address #ifdef IAI_CACHEDCONSTANT
	# lr = guess cb address #ifndef IAI_CACHEDCONSTANT
	# cc = eq if r3 is null #ifndef IAI_CACHEDCONSTANT_INLINING

#ifndef IAI_CACHEDCONSTANT_INLINING
#ifndef IAI_CACHEDCONSTANT
	mflr	r6		/* get guess address */
	addi	r7, r6, 4	/* skip past guess addr to the return addr */
	mtlr	r7
#endif /* IAI_CACHEDCONSTANT */
	beqlr-			/* return if object is NULL */
	lwz	r4, 0(r3)	/* r4 = object.cb */
	lwz	r3, 0(r6)	/* load the guess cb */
	clrrwi.	r4, r4, 2	/* mask off low 2 bits of object cb */
	cmpw	r3, r4		/* see if guess is correct */
	beqlr+   		/* return, same as last successful cast */
#endif /* IAI_CACHEDCONSTANT_INLINING */

#ifdef IAI_CACHEDCONSTANT
	mflr	r7	
#endif

	/* need to call the helper */
	# r4 = objectCb
	# r5 = instanceCb
	# r6 = guess cb address
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeCheckCast)

/* do an instanceof check */
ENTRY ( CVMCCMruntimeInstanceOfGlue )
        # r3 = object to cast
	# r4 = objectCb #ifdef IAI_CACHEDCONSTANT_INLINING
        # r5 = instanceCb
	# r6 = guess cb address #ifdef IAI_CACHEDCONSTANT
	# lr = guess cb address #ifndef IAI_CACHEDCONSTANT
	# cc = eq if r3 is null #ifndef IAI_CACHEDCONSTANT_INLINING
	
#ifndef IAI_CACHEDCONSTANT_INLINING
#ifndef IAI_CACHEDCONSTANT
	mflr	r6		/* get guess address */
	addi	r7, r6, 4	/* skip past guess addr to the return addr */
	mtlr	r7
#endif /* IAI_CACHEDCONSTANT */
	beqlr-			/* return FALSE if object is NULL */
	lwz	r4, 0(r3)	/* r4 = object.cb */
	lwz	r3, 0(r6)	/* load the guess cb */
	clrrwi.	r4, r4, 2	/* mask off low 2 bits of object cb */
	cmpw	r3, r4		/* see if guess is correct */
	li	r3, 1		/* return TRUE if equal */
	beqlr+   		/* return, same as last successful cast */
#endif /* IAI_CACHEDCONSTANT_INLINING */

#ifdef IAI_CACHEDCONSTANT
	mflr	r7	
#endif

	/* need to call the helper */
	# r4 = objectCb
	# r5 = instanceCb
	# r6 = guess cb address
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeInstanceOf)

/* check reference array assignment capatibility */
ENTRY ( CVMCCMruntimeCheckArrayAssignableGlue )
        # r5 = cb of array object (with low bits still set)
        # r6 = cb of rhs object (with low bits still set) 
	clrrwi	r5, r5, 2	/* clear low 2 bits of arraycb */
	lis	r3, HA16(java_lang_Object_Classblock)
	lwz	r5, OFFSET_CVMClassBlock_arrayInfoX(r5)/* arraycb->arrayInfo */
	clrrwi	r6, r6, 2	/* clear low 2 bits of rhs cb */
	lwz	r5, OFFSET_CVMArrayInfo_elementCb(r5)  /* arrayInfo->elemCb */
	mflr	r7
	cmpw	r5, r6		/* check if elemClass(arr) == class(rhs) */
	la	r3, LO16(java_lang_Object_Classblock)(r3)
	beqlr+
	cmpw	r5, r3		/* check if elemClass(arr) == Object */
	beqlr+
	# r5 = element cb of array object (with low bits clear)
	# r6 = cb of rhs object (with low bits clear)
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeCheckArrayAssignable)

/* run the clinit of a class */
ENTRY ( CVMCCMruntimeRunClassInitializerGlue )
        # r5 = target cb
	# lr = return address
	mflr	r7
	/* flush state */
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	/* setup ccee and ee arguments */
	la	r3,  OFFSET_CStack_CCEE(sp)
	mr	r4,  EE
	/* call the helper. If the clinit has to be run, it will return
	 * directly to the method that called us. */
	CALL_VM_FUNCTION(CVMCCMruntimeRunClassInitializer)
	/* reload the return address */
        lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmpwi	r3, 0	/* FALSE means current thread is running clinit */
	beqlr-
	/* At this point we know the class has been intialized. Patch the 
	 * the call to this helper to be a nop.
	 * WARNING: Don't attempt to patch over the lwz of the cb with
	 * a branch around the call to the helper. The lwz may be the
	 * first instruction of a block, which means we may also try to
	 * patch it when doing a gc-rendezvous. */
#ifdef CVM_AOT
	/* Don't patch AOT code */
	lis	r4, HA16(CVMJITcodeCacheAOTEnd)
	lwz	r4, LO16(CVMJITcodeCacheAOTEnd)(r4)
	lwz	r4, 0(r4)
	cmplw	r7, r4
	bge	doPatch
	lis	r4, HA16(CVMJITcodeCacheAOTStart)
	lwz	r4, LO16(CVMJITcodeCacheAOTStart)(r4)
	lwz	r4, 0(r4)
	cmplw	r7, r4
	bgelr
#endif

doPatch:
	lis	r4, 24<<10       /* get the patch, a nop instruction */
	stw	r4, -4(r7)       /* apply the patch */
	/* flush the patched instruction */
	subi	r3, r7, 4	/* begin */
	addi	r4, r3, 4       /* end */
	CALL_VM_FUNCTION(CVMJITflushCache)
	/* return to the helper */
        lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	blr
	
/*
 * Resolve a cp entry and run the clinit if necessary
 */
ENTRY ( CVMCCMruntimeResolveGlue )
        # r5 = cpIndex
	# lr = address of cachedConstant, 4 bytes before returnAddress
	# r7 = address of ccm helper to call
	mflr	r6	/* r6 = arg4 = address of cachedConstant */
	mtctr	r7
	/* flush state */
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
	addi	r7,  r6, 4	/* r7 = address we want to return to */
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	/* setup remaining arguments and call the helper */
	la	r3,  OFFSET_CStack_CCEE(sp)	/* arg1 = ccee */
	mr	r4,  EE				/* arg2 = ee */
	/* call the helper */
	bctrl
	/* reload the return address */
        lwz	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmpwi	r3, 0	/* FALSE means current thread is running clinit */
	beqlr-
	/* At this point we know that any needed class initialization has
	 * been done. Patch the first instruction of the generated code
	 * to branch around the call to this helper, and also around
	 * the cachedConstant word. */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define RESOLVE_PATCH_OFFSET 20
#else
#define RESOLVE_PATCH_OFFSET 32
#endif
	/* get the patch */
	lis	r4, HA16(resolvePatch)
	lwz	r4, LO16(resolvePatch)(r4)
	stw	r4,  -RESOLVE_PATCH_OFFSET+8(r7)   /* apply the patch */
	/* flush the patched instruction */
	la	r3,  -RESOLVE_PATCH_OFFSET+8(r7)  /* begin */
	addi	r4,  r3, 4		          /* end */
	CALL_VM_FUNCTION(CVMJITflushCache)
	/* return to the method */
        lwz	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	blr
SYM_NAME(resolvePatch):
	.long	MAKE_BRANCH(RESOLVE_PATCH_OFFSET)
	
ENTRY ( CVMCCMruntimeResolveNewClassBlockAndClinitGlue )
	lis	r7, HA16(CVMCCMruntimeResolveNewClassBlockAndClinit)
	la	r7, LO16(CVMCCMruntimeResolveNewClassBlockAndClinit)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue )
	lis	r7, HA16(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)
	la	r7, LO16(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue )
	lis	r7, HA16(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)
	la	r7, LO16(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue )
	lis	r7, HA16(CVMCCMruntimeResolveStaticMethodBlockAndClinit)
	la	r7, LO16(CVMCCMruntimeResolveStaticMethodBlockAndClinit)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveClassBlockGlue )
	lis	r7, HA16(CVMCCMruntimeResolveClassBlock)
	la	r7, LO16(CVMCCMruntimeResolveClassBlock)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveArrayClassBlockGlue )
	lis	r7, HA16(CVMCCMruntimeResolveArrayClassBlock)
	la	r7, LO16(CVMCCMruntimeResolveArrayClassBlock)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveGetfieldFieldOffsetGlue )
	lis	r7, HA16(CVMCCMruntimeResolveGetfieldFieldOffset)
	la	r7, LO16(CVMCCMruntimeResolveGetfieldFieldOffset)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolvePutfieldFieldOffsetGlue )
	lis	r7, HA16(CVMCCMruntimeResolvePutfieldFieldOffset)
	la	r7, LO16(CVMCCMruntimeResolvePutfieldFieldOffset)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveSpecialMethodBlockGlue )
	lis	r7, HA16(CVMCCMruntimeResolveSpecialMethodBlock)
	la	r7, LO16(CVMCCMruntimeResolveSpecialMethodBlock)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

ENTRY ( CVMCCMruntimeResolveMethodBlockGlue )
	lis	r7, HA16(CVMCCMruntimeResolveMethodBlock)
	la	r7, LO16(CVMCCMruntimeResolveMethodBlock)(r7)
	b	SYM_NAME(CVMCCMruntimeResolveGlue)

/*
 * Resolve a vtable offset. The protocol here is slight
 * different because the result could possibly be an MB pointer
 * rather than just an offset. Calling code looks like this:
 *	ld	destreg,cache
 *	mov	ARG1, cpIndex
 *	call	CVMCCMruntimeResolveMethodTableOffsetGlue
 *	mov	mbDestreg,RETURN1
 *	b	haveMB
 *  cache:
 *	.word	-1
 *  return1:
 *	ld	destreg, cache
 *
 * If the helper returns 0, then the cache will have been filled with
 * the vtable offset and we go let the general resolver glue code
 * do the rewrite and return.
 * Otherwise, the helper returned the MB pointer and we return that
 * directly.
 */
ENTRY ( CVMCCMruntimeResolveMethodTableOffsetGlue )
        # r5 = cpIndex
	# lr = address of extraordinary return code
	# +8   cached constant
	# +12  return address
	/* flush state */
	mflr	r6	
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
	addi	r6,  r6, 8	/* r6 = arg4 = address of cachedConstant */
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	addi	r7,  r6, 4	/* r7 = address we want to return to */
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	/* setup remaining arguments and call the helper */
	la	r3,  OFFSET_CStack_CCEE(sp)	/* arg1 = ccee */
	mr	r4,  EE				/* arg2 = ee */
	/* call the helper */
	CALL_VM_FUNCTION(CVMCCMruntimeResolveMethodTableOffset)
	/* reload the usual return address */
        lwz	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	cmpwi	r3, 0	/* NULL means value written to cache */
	bne-	returnMBdirect

	/* At this point we know that the method table offset has been
	 * written to the cache word.
	 * Patch the first instruction of the generated code
	 * to branch around the call to this helper, around the special
	 * case return code, and around the cachedConstant word.
	 */
#define RESOLVE_METHOD_TABLE_PATCH_OFFSET (RESOLVE_PATCH_OFFSET+8)
	/* get the patch */
	lis	r4, HA16(resolveOffsetPatch)
	lwz	r4, LO16(resolveOffsetPatch)(r4)
	/* apply the patch */
	stw	r4,  -RESOLVE_METHOD_TABLE_PATCH_OFFSET+8(r7) 
	/* flush the patched instruction */
	la	r3,  -RESOLVE_PATCH_OFFSET+8(r7)  /* begin */
	addi	r4,  r3, 4		          /* end */
	CALL_VM_FUNCTION(CVMJITflushCache)
	/* return to the method */
        lwz	r7,  OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	blr
SYM_NAME(resolveOffsetPatch):
	.long	MAKE_BRANCH(RESOLVE_METHOD_TABLE_PATCH_OFFSET)
	
	/*
	 * helper code returned MB pointer to us.
	 * Return it in the same register to our caller.
	 * Return point is directly after call, which is
	 * 12 bytes less than the current contents of r7
	 */
returnMBdirect:
	subi	r7, r7, 12
	mtlr	r7
	blr

	/* lookup an interface mb */
ENTRY ( CVMCCMruntimeLookupInterfaceMBGlue )
        # r4 = object to invoke with
        # r5 = interface mb
	# lr = address of guess from last interface mb lookup
	mflr	r6		/* get guess address */
	addi	r7, r6, 4	/* skip past guess addr to the return addr */
	mtlr	r7

#define OCB           r4
#define INTERFACE_MB  r5
#define OINTERFACES   r9
#define GUESS         r10

        /* ocb = CVMobjectGetClass(obj); */
        lwz     OCB, OFFSET_CVMObjectHeader_clas(r4) /* get ocb from obj */
        clrrwi  OCB, OCB, 2            /* ocb &= ~0x3; prepare ocb */

        /* interfaces = ocb->interfacesX */
        lwz     OINTERFACES, OFFSET_CVMClassBlock_interfacesX(OCB)
        cmpwi   OINTERFACES, 0        /* Check if ocb->interfacesX == NULL */
        beq     call_lookup_helper
       
        /* ointerfaceCount = ocb->interfacesX->interfaceCountX; */ 
        lhz     r3, OFFSET_CVMInterfaces_interfaceCountX(OINTERFACES)

        lwz     GUESS, -4(r7)      /* load guess value */
        cmplw   GUESS, r3          /* cmp guess and ointerfaceCount */
        bge     call_lookup_helper /* if guess >= ointerfaceCount, invalid */

        /* If we get here than the guess is within the valid range: */
        addi    r3, OINTERFACES, OFFSET_CVMInterfaces_itable
        slwi    r8, GUESS, CONSTANT_LOG2_CVMInterfaceTable_SIZE
        lwzx    r3, r8, r3     /* target ICB = &ointerfaces.itable[guess] */

#ifdef CVM_METHODBLOCK_HAS_CB
        lwz     r0, OFFSET_CVMMethodBlock_cbX(INTERFACE_MB) /* get icb */
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        lbz     r8, OFFSET_CVMMethodBlock_methodIndexX(INTERFACE_MB)
	
	mulli	r8, r8, CONSTANT_CVMMethodBlock_size	
	# So now r8 holds 28 * methodIndex
        sub     r8, INTERFACE_MB, r8
        lwz     r0, -OFFSET_CVMMethodRange_mb(r8)
#endif

        /* Check if the guess' interface CB is the one we want: */
        cmpw    r3, r0                /* test if target ICB == source ICB */
        bne     call_lookup_helper    /* go call helper if guess failed. */

        /* If we get here, then the guess is correct. Go fetch the method
           block from the interface CB: */

#define SOURCE_MB_IDX   r3
        /* get source mb MethodSlotIndex */
        lwz     SOURCE_MB_IDX, OFFSET_CVMMethodBlock_codeX(INTERFACE_MB)

#define TARGET_METHODTABLE_INDICES r8
        /* Target methodTableIndices = 
                ocb->interfacesX.itable[guess].methodTableIndicesX;
        */
        addi    r0, OINTERFACES, OFFSET_CVMInterfaces_itable0_intfInfoX
        slwi    GUESS, GUESS, CONSTANT_LOG2_CVMInterfaceTable_SIZE
        lwzx    TARGET_METHODTABLE_INDICES, GUESS, r0
#undef GUESS
#undef OINTERFACES

        /* Get the interface mb from the ocb's vtbl: */
        lwz     r0, OFFSET_CVMClassBlock_methodTablePtrX(OCB)
        slwi    r3, SOURCE_MB_IDX, \
                    CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE
        lhzx	r8, TARGET_METHODTABLE_INDICES, r3
#undef TARGET_METHODTABLE_INDICES

        /* r3 = ocb->methodTablePtrX[r8]: */
        slwi	r8, r8, 2
        lwzx    r3, r8, r0
        blr

call_lookup_helper:
        # r4 = ocb
        # r5 = imb
        # r6 = guess address
	FIXUP_FRAMES(JFP,r7)
        CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeLookupInterfaceMB)

#undef OCB
#undef TARGET_METHODTABLE_INDICES

/*
 * Entry point for monitorenter.
 */
ENTRY ( CVMCCMruntimeMonitorEnterGlue )
        #
        # Arguments:
        #       r5 = 'obj'
	#

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 3 lines are needed.
	#
	mflr	r7
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
       /* CAS version of monitorenter: */
       /* Do fastTryLock(): */

#define OBJ		r5
#define OBITS		r6
#define LOCKREC		r7
#define OBITS0		r8
#define EXPECTED_CNT	r9
#define NEW_COUNT	r10
#define OLD_COUNT	r11

        # Make sure that the object is not NULL:
        cmpwi	OBJ, 0
        beq-	CVMCCMruntimeThrowNullPointerExceptionGlueLocal

        /* Setup a lock record and assume the object has not been locked
	   yet:	 */
        # lockrec = ee->objLocksFreeOwned:
        lwz	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        cmpwi	LOCKREC, 0
        beq-	_monenterRecordNotAvailable

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
	 *     [OBJ, #OFFSET_CVMObjectHeader_various32] = LOCKREC 
	 * }
	*/
	la	r0, OFFSET_CVMObjectHeader_various32(OBJ)
0:
	lwarx	OBITS0, 0, r0		/* OBITS0 = old various32 */
	cmpw	OBITS0, OBITS		/* check if already locked. */
	bne-	_monenterFastFailed	/* already locked */
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

        blr	/* Return to the caller */

_monenterFastFailed:
#ifdef CVM_DEBUG
        # lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
        li	r0, 0
        stw	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        li	r0, CONSTANT_CVM_OWNEDMON_FREE
        stw	r0, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	# If object is not in LOCKED state, then fail:
	andi.	r0, OBITS0, 0x3	/* check for CVM_LOCKSTATE_LOCKED */
	bne-	_monenterFastRetryFailed
	
	# If not associated with a lock record, then fail:
	clrrwi.	LOCKREC, OBITS0, 2
	beq-	_monenterFastReentryFailed

	# If (lockrec->owner != ee), then fail:	
	lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
	cmpw	r0, EE
	bne-	_monenterFastReentryFailed

	# If we get here, then we are re-entering the lock:
	lwz	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	cmpwi	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq-	_monenterFastReentryFailed

	addi	NEW_COUNT, EXPECTED_CNT, 1
	la	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
0:
	lwarx	OLD_COUNT, 0, r0	/* OLD_COUNT = old count */
	cmpw	OLD_COUNT, EXPECTED_CNT	/* check if already locked. */
        bne-	_monenterFastReentryFailed
	stwcx.	NEW_COUNT, 0, r0	/* update with NEW_COUNT */
	bne-	0b    /* if reservation lost, try again */

	/* If the old count is as expected, then we're successful: */
        blr	/* Return to the caller */

_monenterFastRetryFailed:
_monenterFastReentryFailed:
_monenterRecordNotAvailable:
        /* Let C helper do the hard stuff: */
	mflr	r7
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
	mflr	r7
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
	   r0 = scratch
           r3 = &microlock
           r5 = obj
           r6 = obits
           r7 = lockrec
	   r9 = scratch

           NOTE: By design, MICROLOCK is assigned to r3 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monenterSlowAcquireMicrolock.
        */

#define MICROLOCK   r3
#define OBJ         r5
#define OBITS       r6
#define LOCKREC     r7

        /* ld      MICROLOCK, CVMobjGlobalMicroLockPtr */
	lis	MICROLOCK, HA16(CVMobjGlobalMicroLockPtr)
	lwz	MICROLOCK, LO16(CVMobjGlobalMicroLockPtr)(MICROLOCK)

        /* Make sure that the object is not NULL */
        cmpwi	OBJ, 0
        beq-	CVMCCMruntimeThrowNullPointerExceptionGlueLocal

        /* Acquire the microlock: */
        li	r0, CVM_MICROLOCK_LOCKED 
0:	
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	lwarx	r9, 0, MICROLOCK
	stwcx.	r0, 0, MICROLOCK
	bne-	0b

	cmpwi	r9, CVM_MICROLOCK_UNLOCKED	/* See if we succeeded. */
        bne-	_monenterSlowAcquireMicrolock	/*  Branch if failed */

        /* The microlock has been acquired: */
_monenterLockObj:
        lwz	OBITS, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, OBITS, 0x3
        cmpwi	r0, CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne-	_monenterObjAlreadyLocked

        /* If we get here, then the object has not been locked yet. */
        /* lockrec = ee->objLocksFreeOwned: */
        lwz	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        cmpwi	LOCKREC, 0
        beq-	_monenterRecordNotAvailable

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

        blr	/* Return to the caller */

_monenterSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
	mflr	r0
        stw	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        stw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        stw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           r3.  By design, MICROLOCK is r3, so we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lwz	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
	mtlr	r0
        lwz	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        lwz	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)
        b	_monenterLockObj

_monenterObjAlreadyLocked:
        cmpwi	r0, CONSTANT_CVM_LOCKSTATE_LOCKED
        bne-	_monenterFastReentryFailed

        /* Make sure the current thread owns this lock: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(OBITS)
        cmpw	r0, EE
        bne-	_monenterFastReentryFailed

        lwz	r9, OFFSET_CVMOwnedMonitor_count(OBITS)
        addi	r9, r9, 1
        stw	r9, OFFSET_CVMOwnedMonitor_count(OBITS)

        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)	/* microlock->lockWord = UNLOCKED */

        blr		/* Return to the caller */

_monenterFastReentryFailed:
_monenterRecordNotAvailable:
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)

        /* Load the ccee.  The obj arg is already loaded. */
	mflr	r7
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

#undef MICROLOCK
#undef OBJ
#undef OBITS
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */


/*
 * Entry point for monitorexit.
 */
ENTRY ( CVMCCMruntimeMonitorExitGlue )
        #
        # Arguments:
        #       r5 = 'obj'
#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 3 lines are needed.
	#
	mflr	r7
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        /* CAS version of monitorexit: */
        /* Do fastTryUnlock(): */

#define OBJ		r5
#define NBITS		r6
#define LOCKREC		r7
#define OBITS0		r8
#define EXPECTED_CNT	r9
#define NEW_COUNT	r10
#define OLD_COUNT	r11

        /* Make sure that the object is not NULL: */
        cmpwi	OBJ, 0
        beq-	CVMCCMruntimeThrowNullPointerExceptionGlueLocal

        /* Check to see if the object is locked with a fastlock: */
        lwz	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne-	_monexitFastTryUnlockFailed /* If not, we failed. */

	/* If the LOCKREC is NULL, is is currently being inflated */
	cmpwi	LOCKREC, 0
	beq-	_monexitFastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        cmpw	r0, EE
        bne-	_monexitFastTryUnlockFailed	/* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lwz	EXPECTED_CNT, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	cmpwi	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq-	_monexitFastTryUnlockFailed
        subic.	NEW_COUNT, EXPECTED_CNT, 1
        beq-	_monexitDoUnlock	/* If zero, then unlock */

	/* new monitor count > 0, so just update it and we're done */
	la	r0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
0:
	lwarx	OLD_COUNT, 0, r0	/* OLD_COUNT = old count */
	cmpw	OLD_COUNT, EXPECTED_CNT	/* make sure it didn't inflate. */
        bne-	_monexitFastTryUnlockFailed
	stwcx.	NEW_COUNT, 0, r0	/* update with NEW_COUNT */
	bne-	0b    /* if reservation lost, try again */

	/* we are done! monitor count was successfully decrement */
	blr	/* return to caller */

_monexitDoUnlock:	
#define OBITS LOCKREC
        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        lwz	NBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
	la	r0, OFFSET_CVMObjectHeader_various32(OBJ)
0:
	lwarx	OBITS0, 0, r0		/* OBITS0 = old various32 */
	cmpw	OBITS0, OBITS		/* check if inflated. */
	bne-	_monexitFastTryUnlockFailed /* let C helper do hard work */
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

        /* Check if the lockrec is the first one on the thread's owned list */
        lwz	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)
        cmpw	r0, LOCKREC
        bne-	_monexitFastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee's owned list: */
        lwz	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)

_monexitFastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lwz	r0, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        stw	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        blr	/* Return to the caller. */

/* r0 is the first one on the thread's owned list */
/* It is safe to reuse NBITS at this point */
#define PREV_REC NBITS
_monexitFastTryUnlockFindPrevLockRecord:
	mr	PREV_REC, r0
        lwz	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        cmpw	r12, LOCKREC
        beq-	_monexitFastTryUnlockFoundPrevLockRecord
        mr	PREV_REC, r12
        b	_monexitFastTryUnlockFindPrevLockRecord

_monexitFastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */
        lwz	r12, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC


_monexitFastTryUnlockFailed:
        /* Let C helper do the hard stuff: */
	mflr	r7
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
	   r0 = scratch
           r3 = &microlock
           r5 = obj
           r6 = obits
           r7 = lockrec
	   r9 = scratch

           NOTE: By design, MICROLOCK is assigned to r3 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monexitSlowAcquireMicrolock.
        */

#define MICROLOCK   r3
#define OBJ         r5
#define LOCKREC     r7

        /* ld      MICROLOCK, =CVMobjGlobalMicroLockPtr */
	lis	MICROLOCK, HA16(CVMobjGlobalMicroLockPtr)
	lwz	MICROLOCK, LO16(CVMobjGlobalMicroLockPtr)(MICROLOCK)

        /* Make sure that the object is not NULL */
        cmpwi	OBJ, 0
        beq-	CVMCCMruntimeThrowNullPointerExceptionGlueLocal

        /* Acquire the microlock: */
        li	r0, CVM_MICROLOCK_LOCKED 
0:	
	/* Atomically swap CVM_MICROLOCK_LOCKED into MICROLOCK */
	lwarx	r9, 0, MICROLOCK
	stwcx.	r0, 0, MICROLOCK
	bne-	0b

	cmpwi	r9, CVM_MICROLOCK_UNLOCKED	/* See if we succeeded. */
        bne-	_monexitSlowAcquireMicrolock	/*  Branch if failed */

        /* The microlock has been acquired: */
_monexitUnlockObj:
        /* Check to see if the object is locked with a fastlock: */
        lwz	LOCKREC, OFFSET_CVMObjectHeader_various32(OBJ) /* Get obits */
        andi.	r0, LOCKREC, 0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne-	 _monexitFastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        lwz	r0, OFFSET_CVMOwnedMonitor_owner(LOCKREC)
        cmpw	r0, EE
        bne-	_monexitFastTryUnlockFailed	/* If not, we failed */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        lwz	r9, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	subic.	r9, r9, 1
	bne-	_monexitFastTryUnlockSuccess	/* If not zero, we are done */

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
        bne-	 _monexitFastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee owned list: */
        lwz	r0, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r0, OFFSET_CVMExecEnv_objLocksOwned(EE)

_monexitFastTryUnlockAddLockRecordToFreeList:
        /* Add the lockrec to the ee's free list: */
        lwz	r9, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)
        stw	r9, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

        blr	/* Return to the caller. */

/* r9 is the first one on the thread's owned list */
#define PREV_REC r9
_monexitFastTryUnlockFindPrevLockRecord:
        lwz	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        cmpw	r12, LOCKREC
        beq-	_monexitFastTryUnlockFoundPrevLockRecord
        mr	PREV_REC, r12
        b	_monexitFastTryUnlockFindPrevLockRecord

_monexitFastTryUnlockFoundPrevLockRecord:
        /* Remove the lockrec from the ee's owned list: */
        lwz	r12, OFFSET_CVMOwnedMonitor_next(LOCKREC)
        stw	r12, OFFSET_CVMOwnedMonitor_next(PREV_REC)
        b	_monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC


_monexitFastTryUnlockSuccess:
        /* Set the new re-entry count: */
        stw	r9, OFFSET_CVMOwnedMonitor_count(LOCKREC)
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)  /* *microlock = CVM_MICROLOCK_UNLOCKED */

        blr	/* Return to the caller */

_monexitSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a non-volatile reg
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
	mflr	r0
        stw	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
        stw	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        stw	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           r3.  By design, MICROLOCK is r3, so we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        lwz	r0, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+0(sp)
	mtlr	r0
        lwz	MICROLOCK, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+4(sp)
        lwz	OBJ, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8(sp)
        b       _monexitUnlockObj	/* Go unlock the object if possible */

_monexitFastTryUnlockFailed:
        /* Release the microlock: */
        li	r0, CVM_MICROLOCK_UNLOCKED
        stw	r0, 0(MICROLOCK)

        /* Let C helper do the hard stuff: */
        /* Load the ccee.  The obj arg is already loaded. */
	mflr	r7
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

#undef OBJ
#undef MICROLOCK
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */



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

#if 0
        str     lr,  [CCEE, #OFFSET_REGS+(15*4)] # Save LR as PC.  LR

        str     sp,  [CCEE, #OFFSET_REGS+(13*4)] # Save SP.
        str     r12, [CCEE, #OFFSET_REGS+(12*4)]
        str     r11, [CCEE, #OFFSET_REGS+(11*4)]
        str     r10, [CCEE, #OFFSET_REGS+(10*4)]
        str     r9,  [CCEE, #OFFSET_REGS+(9*4)]
        str     r8,  [CCEE, #OFFSET_REGS+(8*4)]
        str     r7,  [CCEE, #OFFSET_REGS+(7*4)]
        str     r6,  [CCEE, #OFFSET_REGS+(6*4)]
        str     r5,  [CCEE, #OFFSET_REGS+(5*4)]
        str     r4,  [CCEE, #OFFSET_REGS+(4*4)]

        str     r1,  [CCEE, #OFFSET_REGS+(1*4)]
        str     r0,  [CCEE, #OFFSET_REGS+(0*4)]

        # Save the CPSR:
        mrs     r1, CPSR
        str     r1,  [CCEE, #OFFSET_CPSR]

        # Flush the top frame to be consistent:
        str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
        str     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        ldr     r1,  [CCEE, #OFFSET_CVMCCExecEnv_ee]
        str     JFP, [r1, #OFFSET_CVMExecEnv_interpreterStack+ \
                           OFFSET_CVMStack_currentFrame]

        # Call the breakpoint handler.
        mov     r0, CCEE    # arg0 = ccee.
        mov     r1, JFP     # arg1 = frame.
                            # arg2 = nodeID (already set).
                            # arg3 = description (already set).
        CALL_VM_FUNCTION(CVMJITdebugPollBreakpoint)

        # Restore the CPSR:
        ldr     r1,  [CCEE, #OFFSET_CPSR]
        msr     CPSR_f, r1

        # Restore the registers:
        ldr     r0,  [CCEE, #OFFSET_REGS+(0*4)]
        ldr     r1,  [CCEE, #OFFSET_REGS+(1*4)]
        ldr     r2,  [CCEE, #OFFSET_REGS+(2*4)]
        ldr     r3,  [CCEE, #OFFSET_REGS+(3*4)]
        ldr     r4,  [CCEE, #OFFSET_REGS+(4*4)]
        ldr     r5,  [CCEE, #OFFSET_REGS+(5*4)]
        ldr     r6,  [CCEE, #OFFSET_REGS+(6*4)]
        ldr     r7,  [CCEE, #OFFSET_REGS+(7*4)]
        ldr     r8,  [CCEE, #OFFSET_REGS+(8*4)]
        ldr     r9,  [CCEE, #OFFSET_REGS+(9*4)]
        ldr     r10, [CCEE, #OFFSET_REGS+(10*4)]
        ldr     r11, [CCEE, #OFFSET_REGS+(11*4)]
        ldr     r12, [CCEE, #OFFSET_REGS+(12*4)]
        # No need to restore sp as it was already restored by the callee.
        ldr     lr,  [CCEE, #OFFSET_REGS+(14*4)]

        # Return to the caller.
        ldr     pc,  [CCEE, #OFFSET_REGS+(15*4)]


#undef CCEE
#undef OFFSET_REGS
#undef OFFSET_CPSR

#endif

#endif /* CVM_JIT_DEBUG */

SYM_NAME(divideByZeroString):
	.asciz "/ by zero"
