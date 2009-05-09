/*
 * @(#)ccmglue_cpu.S	1.44 06/10/10
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
#define FLUSH_TOS	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
#else
#define FLUSH_TOS
#endif

/*
 * We assume that the mb does not contain a cb field. If this is not the
 * case then change the following to #define.
 */
#undef CVM_METHODBLOCK_HAS_CB

/*
 * Make a pc-relative branch instruction. Used for patching an instruction
 * to jump around generated code.
 */
#define MAKE_BRANCH(offset)	\
    ((0x8 << 25) | (0x2 << 22) | ((offset >> 2) & 0x3ffff))

/*
 * Macro for calling a helper. Sets up the ccee and ee as first two
 * arguments and flushes the pc to the frame and the frame to the 
 * interpreter stack.
 */
#define CALL_HELPER_AND_PASS_EE(HELPER)					\
	add	%sp, MINFRAME, %o0;					\
	add	%o7, 8, %o1;						\
	st	%o1, [JFP + OFFSET_CVMCompiledFrame_PC];		\
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack +		\
			   OFFSET_CVMStack_currentFrame]; 		\
	BRANCH_TO_VM_FUNCTION(HELPER);					\
	mov	EE, %o1

/*
 * Macro for calling a helper. Sets up the ccee as the first
 * argument and flushes the pc to the frame and the frame to the 
 * interpreter stack. It also passes the address of the cache or
 * guess value in %o3. In order to keep the stackmap happy, it also
 * uses this value as the pc flushed to the frame instead of the
 * actual return address, which is just after the cache address.
 */
#define CALL_HELPER_AND_PASS_CACHE_ADDR(HELPER)				\
	add	%o7, 8, %o3;						\
	add	%o7, 12, %o7;						\
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC];		\
	sub	%o7, 8, %o7;						\
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack +		\
		           OFFSET_CVMStack_currentFrame]; 		\
	BRANCH_TO_VM_FUNCTION(HELPER);					\
	add	%sp, MINFRAME, %o0

/* call CVMCCMruntimeSimpleSyncUnlock */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
ENTRY ( CVMCCMruntimeSimpleSyncUnlockGlue )
	add	%o7, 8, %o3  /* fixup return address */
	st	%o3, [JFP + OFFSET_CVMCompiledFrame_PC]
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	BRANCH_TO_VM_FUNCTION(CVMCCMruntimeSimpleSyncUnlock)
	nop
SET_SIZE ( CVMCCMruntimeSimpleSyncUnlockGlue )
#endif

/* do a gc rendezvous */
ENTRY ( CVMCCMruntimeGCRendezvousGlue )
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)

/* Throw various well known exceptions */

ENTRY ( CVMCCMruntimeThrowNullPointerExceptionGlue )
	FIXUP_FRAMES_1(JFP, %g1, %o7)
	set	SYM_NAME(java_lang_NullPointerException_Classblock), %o2
	mov	0, %o3  ! exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
ENTRY ( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )
	FIXUP_FRAMES_1(JFP, %g1, %o7)
	set	SYM_NAME(java_lang_ArrayIndexOutOfBoundsException_Classblock), %o2
	mov	0, %o3  ! exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
ENTRY ( CVMCCMruntimeThrowDivideByZeroGlue )
	FIXUP_FRAMES_1(JFP, %g1, %o7)
	set	SYM_NAME(java_lang_ArithmeticException_Classblock), %o2
	set	divideByZeroString, %o3 !exception message
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)

/* Throw an object */
ENTRY ( CVMCCMruntimeThrowObjectGlue )
	FIXUP_FRAMES_2(JFP, %g1, %o2, %o7)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowObject)

/* do a checkcast */
ENTRY ( CVMCCMruntimeCheckCastGlue )
        ! %o0 = object to cast
        ! %o2 = castCb
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = guess cb address
	! cc = eq if %o0 is null
	beq	1f		! null
	nop
	ld	[%o0], %o1	! %o1 = object.cb
	ld	[%o7 + 8], %o0	! load the guess cb
	andn	%o1, 3, %o1	! mask off low bits of object cb
	cmp	%o0, %o1	! see if guess is correct
	bne	call_checkcast_helper
	nop
1:
	jmp	%o7 + 12	! return, null or same as last successful cast
	nop

call_checkcast_helper:
	/* need to call the helper
	 * %o1 = objectCb
	 * %o2 = instanceCb
	 * %o3 = cache address (setup by CALL_HELPER_AND_PASS_CACHE_ADDR) */
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeCheckCast)

/* do an instanceof check */
ENTRY ( CVMCCMruntimeInstanceOfGlue )
        ! %o0 = object to cast
        ! %o2 = instanceofCb
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = guess cb address
	! cc = eq if %o0 is null
	beq	1f		! return FALSE if object is NULL
	nop
	ld	[%o0], %o1	! %o1 = object.cb
	ld	[%o7 + 8], %o0	! load the guess cb
	andn	%o1, 3, %o1	! mask off low bits of object cb
	cmp	%o0, %o1	! see if guess is correct
	bne	call_instanceof_helper
	nop
	mov	1, %o0		! return TRUE if equal
1:
	jmp	%o7 + 12	! return, null or same as last successful cast
	nop
call_instanceof_helper:
	/* need to call the helper
	 * %o1 = objectCb
	 * %o2 = instanceCb
	 * %o3 = cache address (setup by CALL_HELPER_AND_PASS_CACHE_ADDR) */
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeInstanceOf)

/* check reference array assignment capatibility */
ENTRY ( CVMCCMruntimeCheckArrayAssignableGlue )
        ! %o2 = cb of array object (with low bits still set)
        ! %o3 = cb of rhs object (with low bits still set) 

	andn	%o2, 3, %o2	! clear low bits of arraycb
	ld	[%o2 + OFFSET_CVMClassBlock_arrayInfoX], %o2 ! arraycb->arrayInfo
	andn	%o3, 3, %o3	! clear low bits of rhs cb
	ld	[%o2 + OFFSET_CVMArrayInfo_elementCb], %o2   ! arrayInfo->elemCb
	cmp	%o2, %o3		! check if elemClass(arr) == class(rhs)
	beq	2f
	nop
	set	SYM_NAME(java_lang_Object_Classblock), %o0
	cmp	%o2, %o0		! check if elemClass(arr) == Object
	bne	3f
	nop
2:
	retl
	nop
3:
	! %o2 = element cb of array object (with low bits clear)
	! %o3 = cb of rhs object (with low bits clear)
	FIXUP_FRAMES_3(JFP, %g1, %o2, %o3, %o7)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeCheckArrayAssignable)

/* run the clinit of a class */
ENTRY ( CVMCCMruntimeRunClassInitializerGlue )
        ! %o2 = target cb
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = helper return
	/* setup ccee and ee arguments */
	add	%sp, MINFRAME, %o0
	! ldr	%o0 + OFFSET_CVMCCExecEnv_ee], %o1
	mov	EE, %o1
	/* flush state */
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
        add	%o7, 8, %o7	! Return address = call addr + 8
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC]
	sub	%o7, 8, %o7
        st	JFP, [%o1 + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
        /* call the helper. If the clinit has to be run, it will return
         * directly to the method that called us. */
	BRANCH_TO_VM_FUNCTION(CVMCCMruntimeRunClassInitializer)
	nop
        /* reload the return address */
        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
        /* At this point we know that either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
        cmp	%o0, 0		! FALSE means current thread is running clinit
        bne	0f
	nop
	jmpl	%o7, %g0
        nop

0:
#ifdef CVM_AOT
	/* Don't patch AOT code */
	sethi	%hi(SYM_NAME(CVMJITcodeCacheAOTEnd)), %o1
	ld	[%o1+%lo(SYM_NAME(CVMJITcodeCacheAOTEnd))], %o1
	ld	[%o1], %o1
	cmp	%o7, %o1
	bcc	doPatch
	sethi	%hi(SYM_NAME(CVMJITcodeCacheAOTStart)), %o1
	ld	[%o1+%lo(SYM_NAME(CVMJITcodeCacheAOTStart))], %o1
	ld	[%o1], %o1
	cmp	%o7, %o1
	bcs	doPatch
	nop
	jmpl	%o7, %g0
	nop
#endif

        /* At this point we know the class has been intialized. Patch the 
	 * the call to this helper to be a nop.
	 * WARNING: Don't attempt to patch over the ldr of the cb with
	 * a branch around the call to the helper. The ldr may be the
	 * first instruction of a block, which means we may also try to
	 * patch it when doing a gc-rendezvous. */

doPatch:
        set	0x01000000, %o1	/* get the patch, a nop instruction */
	st	%o1, [%o7 - 8]	/* apply the patch */
        /* flush the patched instruction */
        sub	%o7, 8, %o0	/* begin */
        add	%o7, 4, %o1	/* end */
        CALL_VM_FUNCTION(CVMJITflushCache)
        nop
        /* return to the helper */
        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
        jmpl	%o7, %g0
        nop

/*
 * Resolve a cp entry and run the clinit if necessary
 */
ENTRY ( CVMCCMruntimeResolveGlue )
        ! %o2 = cpIndex
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = cachedConstant
	! +12 = helper return
	! %g1 = address of ccm helper to call
	/* flush state */
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], %o1	! %o1 = ee
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
	add	%o7, 12, %o3		! return address, skip catched constant
        st	%o3, [JFP + OFFSET_CVMCompiledFrame_PC]
        st	JFP, [%o1 + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* setup remaining arguments and call the helper */
	add	%sp, MINFRAME, %o0	! %o0 = ccee
	add	%o7, 8, %o3		! %o3 = address of cachedConstant

	call	%g1			! call the helper
	nop

	/* reload the return address */
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmp	%o0, 0		! FALSE means current thread is running clinit
	bne	1f
	nop
	jmpl	%o7, %g0
	nop
	
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
	set	MAKE_BRANCH(RESOLVE_PATCH_OFFSET), %o1
	st	%o1, [%o7 - RESOLVE_PATCH_OFFSET]	! apply the patch
	/* flush the patched instruction */
	sub	%o7, RESOLVE_PATCH_OFFSET, %o0
	add	%o7, 4, %o1
	CALL_VM_FUNCTION(CVMJITflushCache)
	nop
	/* return to the helper */
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	jmpl	%o7, %g0
	nop


ENTRY ( CVMCCMruntimeResolveNewClassBlockAndClinitGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveNewClassBlockAndClinit)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveNewClassBlockAndClinit)), %g1

ENTRY ( CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)), %g1

ENTRY ( CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)), %g1

ENTRY ( CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveStaticMethodBlockAndClinit)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveStaticMethodBlockAndClinit)), %g1

ENTRY ( CVMCCMruntimeResolveClassBlockGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveClassBlock)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveClassBlock)), %g1

ENTRY ( CVMCCMruntimeResolveArrayClassBlockGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveArrayClassBlock)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveArrayClassBlock)), %g1

ENTRY ( CVMCCMruntimeResolveGetfieldFieldOffsetGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveGetfieldFieldOffset)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveGetfieldFieldOffset)), %g1

ENTRY ( CVMCCMruntimeResolvePutfieldFieldOffsetGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolvePutfieldFieldOffset)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolvePutfieldFieldOffset)), %g1

ENTRY ( CVMCCMruntimeResolveSpecialMethodBlockGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveSpecialMethodBlock)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveSpecialMethodBlock)), %g1

ENTRY ( CVMCCMruntimeResolveMethodBlockGlue )
	sethi	%hi(SYM_NAME(CVMCCMruntimeResolveMethodBlock)), %g1
	b	SYM_NAME(CVMCCMruntimeResolveGlue)
	or	%g1, %lo(SYM_NAME(CVMCCMruntimeResolveMethodBlock)), %g1


/*
 * The protocol for this one is a little difference,
 * since the resolution may result in an MB that doesn't
 * even have a vtable offset. The calling code looks like this:
 *	sethi	%hi(cache), rdest
 * 	ld	[%rdest+%lo(cache)], rdest
 *	call	CVMCCMruntimeResolveMethodTableOffset
 *	nop
 *	mov	mbDestReg,RETURN1
 *	b	haveMBdirect
 *	nop
 *	.word	-1	# cache word
 *	sethi	%hi(cache), rdest
 * 	ld	[%rdest+%lo(cache)], rdest
 */
ENTRY(CVMCCMruntimeResolveMethodTableOffsetGlue)
        ! %o2 = cpIndex
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = mov instr
	! +12 = branch instr 
	! +16 = delay slot
	! +20 = cachedConstant
	! +24 = helper return
	/* flush state */
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], %o1	! %o1 = ee
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
	add	%o7, 24, %o3		! normal return address, skip catched constant
        st	%o3, [JFP + OFFSET_CVMCompiledFrame_PC]
        st	JFP, [%o1 + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* setup remaining arguments and call the helper */
	add	%sp, MINFRAME, %o0	! %o0 = ccee
	add	%o7, 20, %o3		! %o3 = address of cachedConstant

	CALL_VM_FUNCTION(CVMCCMruntimeResolveMethodTableOffset)
	nop

	/* reload the normal return address */
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	/* See if we have an MB or a normal table entry. */
	cmp	%o0, 0		! NULL means table offset stored in cache
	beq	1f
	nop
	jmpl	%o7-16, %g0 ! unusual return
	nop

1:
	/* At this point we know that the method table offset was stored
	 * in the cache word. Patch the first instruction of the generated code
	 * to branch around the call to this helper, around the special return
	 * code, and around the cachedConstant word.
	 */
#define RESOLVE_METHOD_TABLE_PATCH_OFFSET (RESOLVE_PATCH_OFFSET+12)
	set	MAKE_BRANCH(RESOLVE_METHOD_TABLE_PATCH_OFFSET), %o1
	st	%o1, [%o7 - RESOLVE_METHOD_TABLE_PATCH_OFFSET]	! apply the patch
	/* flush the patched instruction */
	sub	%o7, RESOLVE_METHOD_TABLE_PATCH_OFFSET, %o0
	add	%o7, 4, %o1
	CALL_VM_FUNCTION(CVMJITflushCache)
	nop
	/* return to the helper */
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	jmpl	%o7, %g0
	nop
	

ENTRY ( CVMCCMruntimeLookupInterfaceMBGlue )
	! %o1 = object to invoke with
	! %o2 = interface mb
	! %o7 = call instr
	!  +4 = delay slot
	!  +8 = address of guess from last interface mb lookup
	! +12 = return address
#if 0
	!
	! If you just want to call the C helper and write very little
	! assembler code:
	!
#define OCB           %o1
	/* ocb = CVMobjectGetClass(obj); */
	ld	[%o1 + OFFSET_CVMObjectHeader_clas], OCB ! get ocb from obj
	andn	OCB, 3, OCB	! ocb &= ~0x3; prepare ocb

	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeLookupInterfaceMB)
#undef OCB
#endif	/* #if 0 */

	!
	! Following is the fast version of CVMCCMruntimeLookupInterfaceMBGlue,
	! which only calls to C helper when the guess is wrong.
	!

#define OCB           %o1
#define INTERFACE_MB  %o2
#define GUESS	      %o4
#define OINTERFACES   %o5 

	/* ocb = CVMobjectGetClass(obj); */
	ld	[%o1 + OFFSET_CVMObjectHeader_clas], OCB ! get ocb from obj
	andn	OCB, 3, OCB		! ocb &= ~0x3; prepare ocb

        /* interfaces = ocb->interfacesX */
	ld	[OCB + OFFSET_CVMClassBlock_interfacesX], OINTERFACES
	cmp     OINTERFACES, 0		! Check if ocb->interfacesX == NULL.
	beq     call_lookup_helper
	nop
       
	/* ointerfaceCount = ocb->interfacesX->interfaceCountX; */ 
	lduh	[OINTERFACES + OFFSET_CVMInterfaces_interfaceCountX], %o3

	ld      [%o7 + 8], GUESS        ! load guess value
	cmp     GUESS, %o3		! cmp guess and ointerfaceCount
	bcc     call_lookup_helper	! if guess >= ointerfaceCount, invalid

	/* If we get here then the guess is within the valid range: */
	add	OINTERFACES, OFFSET_CVMInterfaces_itable, %o3	! delay slot
	sll	GUESS, CONSTANT_LOG2_CVMInterfaceTable_SIZE, GUESS
	ld	[%o3 + GUESS], %o3	! target ICB = &ointerfaces.itable[guess]

#ifdef CVM_METHODBLOCK_HAS_CB
	ld	[INTERFACE_MB + OFFSET_CVMMethodBlock_cbX], %o0 ! get icb
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
	ldub    [INTERFACE_MB + OFFSET_CVMMethodBlock_methodIndexX], %o0

	! r0 = 28 * r0
        umul	%o0, CONSTANT_CVMMethodBlock_size, %o0
	! So now r0 holds 28 * methodIndex
	sub	INTERFACE_MB, %o0, %o0
	ld	[%o0 - OFFSET_CVMMethodRange_mb], %o0
#endif

	/* Check if the guess' interface CB is the one we want: */
	cmp	%o3, %o0		! test if target ICB == source ICB
	bne	call_lookup_helper	! go call helper if guess failed.

        /* If we get here, then the guess is correct. Go fetch the method
           block from the interface CB: */

#define TARGET_METHODTABLE_INDICES %o5
        /* Target methodTableIndices = 
                ocb->interfacesX.itable[guess].methodTableIndicesX;
        */
	add     OINTERFACES, OFFSET_CVMInterfaces_itable0_intfInfoX, TARGET_METHODTABLE_INDICES	! delay slot
	ld	[TARGET_METHODTABLE_INDICES + GUESS], TARGET_METHODTABLE_INDICES
#undef GUESS
#undef OINTERFACES

#define SOURCE_MB_IDX   %o3
        /* get source mb MethodSlotIndex */
        ld      [INTERFACE_MB + OFFSET_CVMMethodBlock_codeX], SOURCE_MB_IDX

	/* Get the interface mb from the ocb's vtbl: */
	ld	[OCB + OFFSET_CVMClassBlock_methodTablePtrX], %o0
	sll	SOURCE_MB_IDX, CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE, %o3
	lduh	[TARGET_METHODTABLE_INDICES + %o3], TARGET_METHODTABLE_INDICES

	/* r0 = ocb->methodTablePtrX[ip]: */
	sll	TARGET_METHODTABLE_INDICES, 2, TARGET_METHODTABLE_INDICES
	ld	[%o0 + TARGET_METHODTABLE_INDICES], %o0
	jmpl	%o7 + 12, %g0
	nop

call_lookup_helper:
	/* o1 = a2 = ocb
	 * o2 = a3 = imb
	 * o3 = a4 = guess address (to be setup by
	 *           CALL_HELPER_AND_PASS_CACHE_ADDR)
	 */
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeLookupInterfaceMB)

#undef OCB
#undef TARGET_METHODTABLE_INDICES

/*
 * Entry point for monitorenter.
 */
ENTRY ( CVMCCMruntimeMonitorEnterGlue )
	!
	! Arguments:
	!	%o2 = 'obj'
	!
	! Also incoming:
	!	JFP, JSP, EE
	!	ccee = %sp + MINFRAME
	!

#if 0
	!
	! If you just want to call the C helper and write very little
	! assembler code:
	!
	/* Let C helper do the hard stuff: */
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        /* CAS version of monitorenter: */
        /* Do fastTryLock(): */

#define OBJ		%o2
#define LOCKREC         %o4
#define OBITS		%o3
#define OBITS0		%o5
#define NBITS		%o1

	! Make sure that the object is not NULL:
	cmp	OBJ, 0
	beq	CVMCCMruntimeThrowNullPointerExceptionGlue
	nop

	/* Setup a lock record and assume the object has not been locked
	   yet:	 */
	! lockrec = ee->objLocksFreeOwned:
	ld	[EE + OFFSET_CVMExecEnv_objLocksFreeOwned], LOCKREC
	cmp	LOCKREC, 0
	beq	_monenterRecordNotAvailable
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
	bne	_monenterFastFailed
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

	retl	/* Return to the caller */
	nop

_monenterFastFailed:
#ifdef CVM_DEBUG
	! lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
	st	%g0, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
	mov	CONSTANT_CVM_OWNEDMON_FREE, %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
#endif

	! If object is not in LOCKED state, then fail:
	btst	0x3, OBITS0      /* check for CVM_LOCKSTATE_LOCKED */
	bne	_monenterFastRetryFailed
	nop
	
	! If not associated with a lock record, then fail:
	andncc	OBITS0, 0x3, LOCKREC
	beq	_monenterFastReentryFailed
	nop

	! If (lockrec->owner != ee), then fail:	
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_owner], %g1
	cmp	%g1, EE
	bne	_monenterFastReentryFailed
	nop

#define EXPECTED_CNT    OBITS
#define NEW_COUNT       NBITS

	! If we get here, then we are re-entering the lock:
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_count], EXPECTED_CNT
	cmp	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	_monenterFastReentryFailed
	nop

	add	EXPECTED_CNT, 1, NEW_COUNT
	add	LOCKREC, OFFSET_CVMOwnedMonitor_count, %g1 /* address of count */
	cas	[%g1], EXPECTED_CNT, NEW_COUNT
	cmp	EXPECTED_CNT, NEW_COUNT
	bne	_monenterFastReentryFailed
	nop
#undef EXPECTED_CNT
#undef NEW_COUNT

	! If the old count is as expected, then we are successful:	
	retl	/* Return to the caller */
	nop

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
#undef NBITS

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
        FLUSH_TOS
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
           %o0 = &microlock
           %o2 = obj, scratch
           %o3 = obits, scratch
           %o4 = lockrec

           NOTE: By design, MICROLOCK is assigned to %o0 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monenterSlowAcquireMicrolock.
        */

#define OBJ         %o2
#define OBITS       %o3
#define MICROLOCK   %o0
#define LOCKREC     %o4

        ! ld      MICROLOCK, =CVMobjGlobalMicroLockPtr
	set     CVMobjGlobalMicroLockPtr, MICROLOCK
        ! Make sure that the object is not NULL:
        cmp     OBJ, 0
	! Get address of object microlock
        ld      [MICROLOCK], MICROLOCK
        beq     CVMCCMruntimeThrowNullPointerExceptionGlue
        ld      [MICROLOCK], %g1

        /* Acquire the microlock: */
        mov     CVM_MICROLOCK_LOCKED, %g1   ! Swap CVM_MICROLOCK_LOCKED into
        swap    [MICROLOCK], %g1            !    the lockWord.
        cmp     %g1, CVM_MICROLOCK_UNLOCKED ! See if we succeeded.
        bne     _monenterSlowAcquireMicrolock    ! Branch if failed.
        nop

        /* The microlock has been acquired: */
_monenterLockObj:
        ld      [OBJ + OFFSET_CVMObjectHeader_various32], OBITS ! Get obits.
        and     OBITS, 0x3, %g1
        cmp     %g1, CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne     _monenterObjAlreadyLocked
        nop

        /* If we get here, then the object has not been locked yet. */
        ! lockrec = ee->objLocksFreeOwned:
        ld      [EE + OFFSET_CVMExecEnv_objLocksFreeOwned], LOCKREC
        cmp     LOCKREC, 0
        beq     _monenterRecordNotAvailable
        nop

        ! lockrec->u.fast.bits = obits:
        st      OBITS, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]

        ! Now, %o3 (i.e. OBITS) is free.

#ifdef CVM_DEBUG
        ! lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED:
        set     CONSTANT_CVM_OWNEDMON_OWNED, %o3
        st      %o3, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
#endif
        ! obj->hdr.various32 = lockrec:
        st      LOCKREC, [OBJ + OFFSET_CVMObjectHeader_various32]

        ! lockrec->count = 1:
        set     1, %o3                  ! Initial lock re-entry count.
        st      %o3, [LOCKREC + OFFSET_CVMOwnedMonitor_count]

        ! lockrec->object = obj:
        st      OBJ, [LOCKREC + OFFSET_CVMOwnedMonitor_object]

        ! Now, r2 (i.e OBJ) is free.

        ! Release the microlock:
        set     CVM_MICROLOCK_UNLOCKED, %o3
        st      %o3, [MICROLOCK]        ! microlock->lockWord = UNLOCKED.

        /* Remove lockrec from the ee's free list: */
        ! nextRec = lockrec->next:
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], %o2
        ! ee->objLocksFreeOwned = nextRec:
        st      %o2, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* Add the lockrec to the ee's owned list: */
        ! nextRec = ee->objLocksOwned:
        ld      [EE + OFFSET_CVMExecEnv_objLocksOwned], %o2
        ! lockrec->next = nextRec:
        st      %o2, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
        ! ee->objLocksOwned = lockrec:
        st      LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksOwned]

        retl                            ! Return to the caller.
        nop

_monenterSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        st      %o7, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0]
        st      MICROLOCK, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4]
        st      OBJ, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+8]

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           %o0.  By design, MICROLOCK is %o0.  so, we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        nop
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0], %o7
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4], MICROLOCK
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+8], OBJ
        b       _monenterLockObj

_monenterObjAlreadyLocked:
        cmp     %g1, CONSTANT_CVM_LOCKSTATE_LOCKED
        bne     _monenterFastReentryFailed
        nop

        /* Make sure the current thread owns this lock: */
        ld      [OBITS + OFFSET_CVMOwnedMonitor_owner], %g1
        cmp     %g1, EE
        bne     _monenterFastReentryFailed
        nop

        ld      [OBITS + OFFSET_CVMOwnedMonitor_count], %g1
        add     %g1, 1, %g1
        st      %g1, [OBITS + OFFSET_CVMOwnedMonitor_count]

        /* Release the microlock: */
        set     CVM_MICROLOCK_UNLOCKED, %g1
        st      %g1, [MICROLOCK]        ! microlock->lockWord = UNLOCKED.

        retl                            ! Return to the caller.
        nop

_monenterFastReentryFailed:
_monenterRecordNotAvailable:
        /* Release the microlock: */
        set     CVM_MICROLOCK_UNLOCKED, %o3
        st      %o3, [MICROLOCK]

        ! Load the ccee.  The obj arg is already loaded.
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

#undef OBJ
#undef OBITS
#undef MICROLOCK
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

/*
 * Entry point for monitorexit.
 */
ENTRY ( CVMCCMruntimeMonitorExitGlue )
	!
	! Arguments:
	! 	r2 = %o2 = 'obj'
	!
	! Also incoming:
	!	r4 = v1 = jfp
	!	r5 = v2 = jsp
	!	ccee = sp + MINFRAME
	!

#if 0
	! If you just want to call the C helper and write very little 
	! assembler code:
	/* Let C helper do the hard stuff: */
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)
#endif

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        /* CAS version of monitorexit: */
        /* Do fastTryUnlock(): */

#define OBJ		%o2
#define NBITS		%o3
#define LOCKREC		%o4
#define OBITS0		%o5
#define EXPECTED_CNT	%o0
#define NEW_COUNT	%o1
#define OBITS		NEW_COUNT

	! Make sure that the object is not NULL:
	cmp	OBJ, 0
	beq	CVMCCMruntimeThrowNullPointerExceptionGlue
	nop

	/* Check to see if the object is locked with a fastlock: */
	ld	[OBJ + OFFSET_CVMObjectHeader_various32], LOCKREC /* obits */
	btst	0x3, LOCKREC    /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
	bne	_monexitFastTryUnlockFailed /* If not, we failed. */
	nop

	cmp	LOCKREC, 0
	beq	_monexitFastTryUnlockFailed
	nop

	/* If we get here, then the object is locked with a fastlock: */

	/* Make sure that the current thread owns the monitor: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_owner], %g1
	cmp	%g1, EE
	bne	_monexitFastTryUnlockFailed	/* If not owner, we failed. */
	nop

	/* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_count], EXPECTED_CNT
	cmp	EXPECTED_CNT, CONSTANT_CVM_INVALID_REENTRY_COUNT
	beq	_monexitFastTryUnlockFailed
	nop
	sub	EXPECTED_CNT, 1, NEW_COUNT
	cmp	NEW_COUNT, 0
	beq	_monexitDoUnlock	/* If zero, then unlock */
	nop

	/* new monitor count > 0, so just update it and we're done */
	add	LOCKREC, OFFSET_CVMOwnedMonitor_count, %g1 /* add of count */
	cas	[%g1], EXPECTED_CNT, NEW_COUNT
	cmp	EXPECTED_CNT, NEW_COUNT
	bne	_monexitFastTryUnlockFailed
	nop

	/* we're done! monitor count was successfully decrement */
	retl	/* return to caller */
	nop

_monexitDoUnlock:	
	/* If we get here, then the re-entry count has reached 0. */
	/* Restore the obits to the object header: */
	mov	LOCKREC, OBITS	/* initialize OBITS */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits], NBITS
	add	OBJ, OFFSET_CVMObjectHeader_various32, %g1
	cas	[%g1], OBITS, NBITS
	cmp	OBITS, NBITS
	bne	_monexitFastTryUnlockFailed
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
	bne	_monexitFastTryUnlockFindPrevLockRecord
	nop

	/* Remove the lockrec from the ee's owned list: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_next], %g1
	st	%g1, [EE + OFFSET_CVMExecEnv_objLocksOwned]

_monexitFastTryUnlockAddLockRecordToFreeList:
	/* Add the lockrec to the ee's free list: */
	ld	[EE + OFFSET_CVMExecEnv_objLocksFreeOwned], %g1
	st	%g1, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
	st	LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

	retl	/* Return to the caller. */
	nop

/* PREV_REC (g1) is the first one on the thread's owned list */
#define PREV_REC %g1
/* It is safe to reuse NBITS at this point */
#define NEXT_REC NBITS
_monexitFastTryUnlockFindPrevLockRecord:
	ld	[PREV_REC + OFFSET_CVMOwnedMonitor_next], NEXT_REC
	cmp	NEXT_REC, LOCKREC
	beq	_monexitFastTryUnlockFoundPrevLockRecord
	nop
	b	_monexitFastTryUnlockFindPrevLockRecord
	mov	NEXT_REC, PREV_REC

_monexitFastTryUnlockFoundPrevLockRecord:
	/* Remove the lockrec from the ee's owned list: */
	ld	[LOCKREC + OFFSET_CVMOwnedMonitor_next], NEXT_REC
	b	_monexitFastTryUnlockAddLockRecordToFreeList
	st	NEXT_REC, [PREV_REC + OFFSET_CVMOwnedMonitor_next]
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
#undef OBITS

#elif (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_MICROLOCK) || \
      (CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SWAP_SPINLOCK)
	/* TODO: if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK, then
	 * use CVMschedLock for locking. For now, just defer to C */
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)
#else
        /* CVM_MICROLOCK_SWAP_SPINLOCK microlock version of monitorexit: */
        /* TODO: The following code can be optimized a little more by doing
           some more selective register usage and instruction scheduling (like
	   making use of delay slots).  Since the risk of doing this is high
	   and the gains are small, this is left as an advanced exercise for
	   later. */

        /* Do fastTryUnlock(): */

        /*
           %o0 = &microlock
           %o2 = obj, scratch
           %o3 = scratch
           %o4 = lockrec

           NOTE: By design, MICROLOCK is assigned to %o0 so that it will be in
	   the appropriate arg register if we need to call
	   CVMmicrolockLockImpl() in _monexitSlowAcquireMicrolock.
        */

#define OBJ         %o2
#define MICROLOCK   %o0
#define LOCKREC     %o4

        set     CVMobjGlobalMicroLockPtr, MICROLOCK
        ! Make sure that the object is not NULL:
        cmp     OBJ, 0
	! Get address of object microlock
        ld      [MICROLOCK], MICROLOCK
        beq     CVMCCMruntimeThrowNullPointerExceptionGlue
        ld      [MICROLOCK], %g1

_monexitAcquireMicrolock:

        set     CVM_MICROLOCK_LOCKED, %g1   ! Swap CVM_MICROLOCK_LOCKED into
        swap    [MICROLOCK], %g1            !    the lockWord.

        cmp     %g1, CVM_MICROLOCK_UNLOCKED ! See if we succeeded.
        bne     _monexitSlowAcquireMicrolock    ! Branch if failed.
        nop

        /* The microlock has been acquired: */
_monexitUnlockObj:
        /* Check to see if the object is locked with a fastlock: */
        ld      [OBJ + OFFSET_CVMObjectHeader_various32], LOCKREC ! Get obits.
        btst    0x3, LOCKREC    ! (obits & 0x3) == CVM_LOCKSTATE_LOCKED?
        bne     _monexitFastTryUnlockFailed ! If not, we failed.
        nop

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_owner], %o3
        cmp     %o3, EE
        bne     _monexitFastTryUnlockFailed ! If not, we failed.
        nop

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_count], %o3
        subcc   %o3, 1, %o3
        bne     _monexitFastTryUnlockSuccess    ! If not zero, we are done.
        Nop

        /* If we get here, then the re-entry count has reached 0. */
        ! Restore the obits to the object header:
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits], %o3
        st      %o3, [OBJ + OFFSET_CVMObjectHeader_various32]

#ifdef CVM_DEBUG
        ! Make the lockrec play nice with the debug assertions:
        set     CONSTANT_CVM_OWNEDMON_FREE, %o3
        st      %o3, [LOCKREC + OFFSET_CVMOwnedMonitor_state]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_u_fast_bits]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_object]
        st      %g0, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
#endif

        ! Release the microlock:
        st      %g0, [MICROLOCK]    ! *microlock = CVM_MICROLOCK_UNLOCKED;

        ! Now, %o2 (i.e. OBJ) is free.

        ! Check if the lockrec is the first one on the thread owned list:
        ld      [EE + OFFSET_CVMExecEnv_objLocksOwned], %o3
        cmp     %o3, LOCKREC
        bne     _monexitFastTryUnlockFindPrevLockRecord
        nop

        ! Remove the lockrec from the ee owned list: 
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], %o3
        st      %o3, [EE + OFFSET_CVMExecEnv_objLocksOwned]

_monexitFastTryUnlockAddLockRecordToFreeList:
        ! Add the lockrec to the ee free list:
        ld      [EE + OFFSET_CVMExecEnv_objLocksFreeOwned], %o3
        st      %o3, [LOCKREC + OFFSET_CVMOwnedMonitor_next]
        st      LOCKREC, [EE + OFFSET_CVMExecEnv_objLocksFreeOwned]

        retl                        ! Return to the caller.
        nop

#define PREV_REC %o3
_monexitFastTryUnlockFindPrevLockRecord:
        ld      [PREV_REC + OFFSET_CVMOwnedMonitor_next], %o2
        cmp     %o2, LOCKREC
        beq     _monexitFastTryUnlockFoundPrevLockRecord
        mov     %o2, PREV_REC
        b       _monexitFastTryUnlockFindPrevLockRecord
        nop

_monexitFastTryUnlockFoundPrevLockRecord:
        ! Remove the lockrec from the ee owned list: 
        ld      [LOCKREC + OFFSET_CVMOwnedMonitor_next], %o2
        st      %o2, [PREV_REC + OFFSET_CVMOwnedMonitor_next]
        b       _monexitFastTryUnlockAddLockRecordToFreeList
        nop
#undef PREV_REC

_monexitFastTryUnlockSuccess:
        ! Set the new re-entry count:
        st      %o3, [LOCKREC + OFFSET_CVMOwnedMonitor_count]
        ! Release the microlock:
        st      %g0, [MICROLOCK]    ! *microlock = CVM_MICROLOCK_UNLOCKED;

        retl                        ! Return to the caller.
        nop

_monexitSlowAcquireMicrolock:
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a non-volatile reg
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        st      %o7, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0]
        st      MICROLOCK, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4]
        st      OBJ, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+8]

        /* CVMmicrolockLockImpl() requires the address of the microlock in
           %o0.  By design, MICROLOCK is %o0.  so, we're cool here. */
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        nop
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+0], %o7
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+4], MICROLOCK
        ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage+8], OBJ
        b       _monexitUnlockObj       ! Go unlock the object if possible.

_monexitFastTryUnlockFailed:
        /* Release the microlock: */
        st      %g0, [MICROLOCK]    ! *microlock = CVM_MICROLOCK_UNLOCKED;

        /* Let C helper do the hard stuff: */
        ! Load the ccee.  The obj arg is already loaded.
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

#undef OBJ
#undef MICROLOCK
#undef LOCKREC

#endif /* CVM_MICROLOCK_SWAP_SPINLOCK */

divideByZeroString:
	.asciz "/ by zero"


