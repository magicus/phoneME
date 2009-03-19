/*
 * @(#)ccmglue_cpu.S	1.7 06/10/23
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
 * The file includes glue code that aids in the calling of some the
 * CCM C helpers, and also do shortcuts to try to avoid having
 * to call the helpers.
 */
	
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitcisc_cpu.h"
#include "javavm/include/porting/sync.h"

#if (CVM_FASTLOCK_TYPE != CVM_FASTLOCK_ATOMICOPS)
#error "Only CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS supported"
#endif

#ifdef CVM_DEBUG_ASSERTS
.file "ccmglue_cpu.S"
ccmglue_cpu_filename:
.asciz "ccmglue_cpu.S"
CVMCCMruntimeThrowNullPointerExceptionGlue_assert_expression:
.asciz "%eax == (%esp)"
#endif

/* CVMCCMruntimeThrowDivideByZeroGlue */
ENTRY(CVMCCMruntimeThrowDivideByZeroGlue )
	FIXUP_FRAMES_0(JFP, A1 /* scratch */, CVMCCMruntimeThrowDivideByZeroGlue_L1)
CVMCCMruntimeThrowDivideByZeroGlue_L1:
	CALL_HELPER_4(A1 /* scratch */, CCEE_AS_ARG, EE_AS_ARG, AS_ARG($SYM_NAME(java_lang_ArithmeticException_Classblock )), AS_ARG($divideByZeroString), SYM_NAME(CVMCCMruntimeThrowClass))
SET_SIZE( CVMCCMruntimeThrowDivideByZeroGlue )

/* CVMCCMruntimeCheckArrayAssignableGlue */
ENTRY(CVMCCMruntimeCheckArrayAssignableGlue )
#define SCRATCH A1
	# A3 = cb of array object (with low bits still set)
	# A4 = cb or rhs object (with low bits still set)

	andl $~3, A3                                 # clear low bits of array cb
	movl OFFSET_CVMClassBlock_arrayInfoX(A3), A3 # arraycb->arrayInfo
	andl $~3, A4                                 # clear low bits of rhs cb
	movl OFFSET_CVMArrayInfo_elementCb(A3), A3   # arrayInfo->elemCb

	cmpl A3, A4 # check if elemClass(arr) == class(rhs)
	je CVMCCMruntimeCheckArrayAssignableGlue_L1

	movl SYM_NAME(java_lang_Object_Classblock), SCRATCH
	cmpl A3, SCRATCH # check if elemClass(arr) == Object
	jne CVMCCMruntimeCheckArrayAssignableGlue_L2

CVMCCMruntimeCheckArrayAssignableGlue_L1:
	ret

CVMCCMruntimeCheckArrayAssignableGlue_L2:			
	FIXUP_FRAMES_1(JFP, SCRATCH, A3, CVMCCMruntimeCheckArrayAssignableGlue_L3)
CVMCCMruntimeCheckArrayAssignableGlue_L3:
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeCheckArrayAssignable))
#undef SCRATCH
SET_SIZE( CVMCCMruntimeCheckArrayAssignableGlue )

/* do a checkcast check */
ENTRY(CVMCCMruntimeCheckCastGlue )
#define GUESS_CB_ADDR A4
        # A1      = object to cast
        # A3      = castCb
	# 0(%esp) = guess cb address
	# cc      = eq if A1 is null
	popl	GUESS_CB_ADDR		# return address (points to first byte after
					# call instruction == first byte of guess cb)
	je	CVMCCMruntimeCheckCastGlue_L1	# return FALSE if object is NULL
	movl	0(A1), A2		# A2 = object.cb
	movl	0(GUESS_CB_ADDR), A1	# load the guess cb
	andl	$~3, A2			# mask off low bits of object cb
	cmpl	A1, A2			# see if guess is correct
	jne	call_checkcast_helper
	mov	$1, A1			# return TRUE if equal
CVMCCMruntimeCheckCastGlue_L1:
	add	$4, GUESS_CB_ADDR
	jmp	*GUESS_CB_ADDR		# return, null or same as last successful cast
call_checkcast_helper:
	/* need to call the helper
	 * A2 = objectCb
	 * A3 = instanceCb
	 * A4 = cache address (setup by CALL_HELPER_AND_PASS_CACHE_ADDR) */
#define SCRATCH A1
	pushl	GUESS_CB_ADDR		# save return address
	add	$4, 0(%esp)		# == instruction after guess cb
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, call_checkcast_helper_L1)
call_checkcast_helper_L1:
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeCheckCast))
#undef SCRATCH
#undef GUESS_CB_ADDR
SET_SIZE( CVMCCMruntimeCheckCastGlue )

/* do an instanceof check */
ENTRY(CVMCCMruntimeInstanceOfGlue )
#define GUESS_CB_ADDR A4
        # A1      = object to cast
        # A3      = instanceofCb
	# 0(%esp) = guess cb address 
	# cc      = eq if A1 is null
	popl	GUESS_CB_ADDR		# return address (points to first byte after
					# call instruction == first byte of guess cb)
	je	CVMCCMruntimeInstanceOfGlue_L1		# return FALSE if object is NULL
	movl	0(A1), A2		# A2 = object.cb
	movl	0(GUESS_CB_ADDR), A1	# load the guess cb
	andl	$~3, A2			# mask off low bits of object cb
	cmpl	A1, A2			# see if guess is correct
	jne	call_instanceof_helper
	mov	$1, A1			# return TRUE if equal
CVMCCMruntimeInstanceOfGlue_L1:
	add	$4, GUESS_CB_ADDR
	jmp	*GUESS_CB_ADDR		# return, null or same as last successful cast
	
call_instanceof_helper:
	/* need to call the helper
	 * A2 = objectCb
	 * A3 = instanceCb
	 * A4 = cache address (setup by CALL_HELPER_AND_PASS_CACHE_ADDR) */
#define SCRATCH A1
	pushl	GUESS_CB_ADDR		# save return address
	add	$4, 0(%esp)		# == instruction after guess cb
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeInstanceOf))
#undef SCRATCH
#undef GUESS_CB_ADDR
SET_SIZE( CVMCCMruntimeInstanceOfGlue )

/*
 * Resolve a cp entry and run the clinit if necessary
 */
#define RTARGET A2
#define SCRATCH A1
ENTRY(CVMCCMruntimeResolveGlue )
        # %o2 = cpIndex
	# %o7 = call instr
	#  +4 = delay slot
	#  +8 = address of branch to try again
	# +12 = cachedConstant
	# +16 = helper return
	# %g1 = address of ccm helper to call
	
	movl	0(%esp), A4    /* return address here is address of cachedConstant */
	addl	$4, 0(%esp)    /* modify the return address to the insn after the cached Constant */
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), AS_ARG(A4), *RTARGET)

#define RESOLVE(f)	\
	ENTRY2(f,Glue)	\
	movl	$SYM_NAME(f), RTARGET	;               \
	jmp	SYM_NAME(CVMCCMruntimeResolveGlue) ;

RESOLVE(CVMCCMruntimeResolveNewClassBlockAndClinit)
RESOLVE(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)
RESOLVE(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)
RESOLVE(CVMCCMruntimeResolveStaticMethodBlockAndClinit)
RESOLVE(CVMCCMruntimeResolveClassBlock)
RESOLVE(CVMCCMruntimeResolveArrayClassBlock)
RESOLVE(CVMCCMruntimeResolveGetfieldFieldOffset)
RESOLVE(CVMCCMruntimeResolvePutfieldFieldOffset)
RESOLVE(CVMCCMruntimeResolveSpecialMethodBlock)
RESOLVE(CVMCCMruntimeResolveMethodBlock)
RESOLVE(CVMCCMruntimeResolveMethodTableOffset)

#undef RTARGET
#undef SCRATCH
		
/* CVMCCMruntimeRunClassInitializerGlue */
ENTRY(CVMCCMruntimeRunClassInitializerGlue )
	# A3 = target cb

#define SCRATCH A1

	CALL_HELPER_3_NO_RET(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), SYM_NAME(CVMCCMruntimeRunClassInitializer))

        /* At this point we know that either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmp $0, %eax
	jne CVMCCMruntimeRunClassInitializerGlue_L1
	ret
	
CVMCCMruntimeRunClassInitializerGlue_L1:
        /* At this point we know the class has been intialized. Patch the 
	 * the call to this helper to be a nop.
	 * WARNING: Don't attempt to patch over the ldr of the cb with
	 * a branch around the call to the helper. The ldr may be the
	 * first instruction of a block, which means we may also try to
	 * patch it when doing a gc-rendezvous. */
	pushl	%esi			# callee saved
	movl	4(%esp), %esi		# addr of call
	subl    $5, %esi		# call is 5 bytes long
	movl	0(%esi), %eax		# load call
	movl	4(%esi), %edx		# load call
	movl	$0x90909090, %ebx	# 4 nops
	movl	%edx, %ecx
	movb	$0x90, %cl		# 5th nop
	CVMJITX86_CMPXCHG8B(0(%esi))
	popl	%esi
	ret
	
#if 0 /* no necessary on x86 */
	CALL_HELPER_2(SCRATCH, AS_ARG(A1), AS_ARG(A2), CVMJITflushCache)
#endif
#undef SCRATCH	
SET_SIZE( CVMCCMruntimeRunClassInitializerGlue )

/* CVMCCMruntimeLookupInterfaceMBGlue */
ENTRY(CVMCCMruntimeLookupInterfaceMBGlue )
	# A2     = object to invoke with
	# A3     = interface mb
	# 0(%esp) = address of guess from last interface mb lookup
#if 0 /* SVMC_JIT d022609 2003-09-19T. temporarily enable runtime call until 
	problem with `CONSTANT_LOG2_CVMInterfaceTable_SIZE' is solved. */
	#
	# If you just want to call the C helper and write very little
	# assembler code:
	#
	movl OFFSET_CVMObjectHeader_clas(A2), A2
	andl $(~3), A2
	movl 0(%esp), A4
	addl $4, 0(%esp)		# == instruction after guess cb
#define SCRATCH A1
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeLookupInterfaceMBGlue_L1)
CVMCCMruntimeLookupInterfaceMBGlue_L1:
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeLookupInterfaceMB))
#undef SCRATCH

#else /* #if 1 */

	#
	# Following is the fast version of CVMCCMruntimeLookupInterfaceMBGlue,
	# which only calls to C helper when the guess is wrong.
	#

#define OCB           A2
#define INTERFACE_MB  A3
#define GUESS	      %esi
#define GUESSw	      %si
#define OINTERFACES   %edi

/*
 *     | 4. word (free)  <- 16 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 3. word (free)  <- 12 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 2. %edi (JSP)   <-  8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 1. %esi         <-  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     :	
 *     | 1. ret addr.    <-      0(%esp)
 *     +---------------
 */

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	movl	%esi, 4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
	movl	%edi, 8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)     # %edi is JSP

	/* ocb = CVMobjectGetClass(obj); */
	movl	OFFSET_CVMObjectHeader_clas(A2), OCB # get ocb from obj
	andl	$~3, OCB		# ocb &= ~0x3; prepare ocb

        /* interfaces = ocb->interfacesX */
	movl	OFFSET_CVMClassBlock_interfacesX(OCB), OINTERFACES
	cmpl    $0, OINTERFACES		# Check if ocb->interfacesX == NULL.
	je      call_lookup_helper
       
	movl    0(%esp), GUESS
	movl    0(GUESS), GUESS		# load guess value
	/* cmp guess and ocb->interfacesX->interfaceCountX */
	cmpw    OFFSET_CVMInterfaces_interfaceCountX(OINTERFACES), GUESSw
	jae     call_lookup_helper	# if guess >= ointerfaceCount, invalid	
	
	/* If we get here then the guess is within the valid range: */
	/* shll	$CONSTANT_LOG2_CVMInterfaceTable_SIZE, GUESS */
        shll	$3, GUESS
	/* target ICB = &ointerfaces.itable[guess] */
	movl	OFFSET_CVMInterfaces_itable(OINTERFACES, GUESS), A4

#ifdef CVM_METHODBLOCK_HAS_CB
	/* A1 = source ICB */
	movl	OFFSET_CVMMethodBlock_cbX(INTERFACE_MB), A1 # get icb
#else

#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif

	movzbl    OFFSET_CVMMethodBlock_methodIndexX(INTERFACE_MB), A1
#if IMPLEMENTED_STACKMAP_CACHE_AND_REDUCED_MB_SIZE_FURTHER

#error ported this from sparc and wonder how this should ever work (rr).

	/* FIXME: When method size is 28 fix appropriately */
	# Here's a way to do r0 = 20r0
	#    First set r0 = r0 << 4 (16r0)
	#    Now add a quarter of that value to itself:	 r0 = r0 + r0 / 4
	# r0 = 32 * r0
	shll	$4, A1
	/* FIXME: When method size is 28, just subtract 1/8 of 32*r0 to
	get 28*r0 */
	# r0 = r0 + r0 / 4
#define SCRATCH	%edi
	movl	A1, SCRATCH
	shrl	$2, SCRATCH
	addl	SCRATCH, A1
#undef scratch
#endif

#if (CONSTANT_CVMMethodBlock_size == 28)
	# A1 = CONSTANT_CVMMethodBlock_size * A1
        imull    $CONSTANT_CVMMethodBlock_size, A1
#else        
#error Wrong CVMMethodBlock size, not tested yet
#endif
	negl	A1
	addl	INTERFACE_MB, A1
	movl	-OFFSET_CVMMethodRange_mb(A1), A1
#endif

	/* Check if the guess' interface CB is the one we want: */
	cmpl	A4, A1		# test if target ICB == source ICB
	jne	call_lookup_helper	# go call helper if guess failed.

        /* If we get here, then the guess is correct. Go fetch the method
           block from the interface CB: */

#define TARGET_METHODTABLE_INDICES %edi
        /* Target methodTableIndices = 
                ocb->interfacesX.itable[guess].methodTableIndicesX;
        */
	movl	OFFSET_CVMInterfaces_itable0_intfInfoX(OINTERFACES, GUESS), TARGET_METHODTABLE_INDICES
#undef GUESS
#undef OINTERFACES

#define SOURCE_MB_IDX   A4
        /* get source mb MethodSlotIndex */
        movl      OFFSET_CVMMethodBlock_codeX(INTERFACE_MB), SOURCE_MB_IDX

	/* Get the interface mb from the ocb's vtbl: */
	movl	OFFSET_CVMClassBlock_methodTablePtrX(OCB), A1
	/* shll	$CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE, A4 */
        shll	$1, A4
	movzwl	0(TARGET_METHODTABLE_INDICES, A4), TARGET_METHODTABLE_INDICES
#undef SOURCE_MB_IDX

	/* A1 = ocb->methodTablePtrX[ip]: */
	movl	0(A1, TARGET_METHODTABLE_INDICES, 4), A1
	addl	$4, 0(%esp)		# == instruction after guess cb
	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	movl	8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %edi
	ret
	

call_lookup_helper:
	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	movl	8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %edi
	movl 0(%esp), A4
	addl $4, 0(%esp)		# == instruction after guess cb
#define SCRATCH A1
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeLookupInterfaceMBGlue_L1)
CVMCCMruntimeLookupInterfaceMBGlue_L1:
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeLookupInterfaceMB))
#undef SCRATCH

	
#undef OCB
#undef INTERFACE_MB
#undef COUNT

#endif /* 0 */
SET_SIZE( CVMCCMruntimeLookupInterfaceMBGlue )

/* CVMCCMruntimeThrowObjectGlue */
ENTRY(CVMCCMruntimeThrowObjectGlue )
	# A3 = CVMObject
#define SCRATCH A1
	FIXUP_FRAMES_1(JFP, SCRATCH, A3, CVMCCMruntimeThrowObjectGlue_L1)
CVMCCMruntimeThrowObjectGlue_L1:
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), SYM_NAME(CVMCCMruntimeThrowObject))
#undef SCRATCH	
SET_SIZE( CVMCCMruntimeThrowObjectGlue )

/*
 * Entry point for monitorenter.
 */
ENTRY(CVMCCMruntimeMonitorEnterGlue )
	#
	# Arguments:
	#	A3 = 'obj'
	#
	# Also incoming:
	#	JFP, JSP
	#	ccee = %esp
	#	ee   =  ccee->eeX
	#

#define SCRATCH A1
#ifdef CVM_JIT_CCM_USE_C_HELPER
        CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), SYM_NAME(CVMCCMruntimeMonitorEnter))

#else

	cmpl	$0, A3
#ifdef CVM_DEBUG_ASSERTS
	movl	(%esp), A1
#endif
	je	SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlue)

#define OBITS		A1
#define LOCKREC         A2
#define OBJ		A3
#define EE		A4
#define NBITS		%esi



	movl    4 + OFFSET_CVMCCExecEnv_ee(%esp), EE
	
	# lockrec = ee->objLocksFreeOwned:
	movl	OFFSET_CVMExecEnv_objLocksFreeOwned(EE), LOCKREC
	cmpl	$0, LOCKREC
	je	_monenterRecordNotAvailable

/*
 *     | 4. word (free)  <- 16 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 3. word (free)  <- 12 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 2. word (free)  <-  8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 1. %esi         <-  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     :	
 *     | 1. ret addr.    <-      0(%esp)
 *     +---------------
 */

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	movl	%esi, 4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)

	# lockrec->object = obj:
	movl	OBJ, OFFSET_CVMOwnedMonitor_object(LOCKREC)

	# lockrec->count = 1:
	/* Initial lock re-entry count */
	movl	$1, OFFSET_CVMOwnedMonitor_count(LOCKREC) 
#ifdef CVM_DEBUG
	# lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED:
	movl	$CONSTANT_CVM_OWNEDMON_OWNED, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	# nbits = (LOCKREC) | CVM_LOCKSTATE_LOCKED:	
	# Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
	# that nbits is essentially lockrec.  Nothing need to be done to
	# initialize nbits.
	movl	LOCKREC, NBITS	

	# obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED:
	movl	OFFSET_CVMObjectHeader_various32(OBJ), OBITS /* Get obits */
	andl	$~0x3, OBITS	/* clear rightmost 2 bits */
	orl	$CONSTANT_CVM_LOCKSTATE_UNLOCKED, OBITS
	
	# lockrec->u.fast.bits = obits:
	movl	OBITS, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)

	# Do atomicCompareAndSwap:
	/* if ([OBJ, #OFFSET_CVMObjectHeader_various32] == OBITS) {
	 *     [OBJ, #OFFSET_CVMObjectHeader_various32] = NBITS
	 * }
	 */
	CVMJITX86_CMPXCHG(NBITS, OFFSET_CVMObjectHeader_various32(OBJ))
	/* cmpxchg sets ZF := OFFSET_CVMObjectHeader_various32(OBJ) == OBITS */
	jne	_monenterFastFailed

#undef OBITS
	/* Remove lockrec from the ee's free list: */
	# nextRec = lockrec->next:
#define SCRATCH		A1
	movl	OFFSET_CVMOwnedMonitor_next(LOCKREC), SCRATCH
	# ee->objLocksFreeOwned = nextRec:
	movl	SCRATCH, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

	/* Add the lockrec to the ee's owned list: */
	# nextRec = ee->objLocksOwned:
	movl	OFFSET_CVMExecEnv_objLocksOwned(EE), SCRATCH
	# lockrec->next = nextRec:
	movl	SCRATCH, OFFSET_CVMOwnedMonitor_next(LOCKREC)
	# ee->objLocksOwned = lockrec:
	movl	LOCKREC, OFFSET_CVMExecEnv_objLocksOwned(EE)
#undef SCRATCH

	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	ret	/* Return to the caller */

#define OBITS		A1
_monenterFastFailed:
#ifdef CVM_DEBUG
	# lockrec->state = CONSTANT_CVM_OWNEDMON_FREE:
	movl	$0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	movl	$CONSTANT_CVM_OWNEDMON_FREE, OFFSET_CVMOwnedMonitor_state(LOCKREC)
#endif

	# If object is not in LOCKED state, then fail:
	testl	$0x3, OBITS	/* check for CVM_LOCKSTATE_LOCKED */
	jne	_monenterFastRetryFailed

	
	# If not associated with a lock record, then fail:
	movl	OBITS, LOCKREC
	andl	$~0x3, LOCKREC
	je	_monenterFastReentryFailed


	# If (lockrec->owner != ee), then fail:	
	cmpl	OFFSET_CVMOwnedMonitor_owner(LOCKREC), EE
	jne	_monenterFastReentryFailed

#undef OBITS
#undef NBITS
#define EXPECTED_CNT    A1
#define NEW_COUNT       A4

	# If we get here, then we are re-entering the lock:
	movl	OFFSET_CVMOwnedMonitor_count(LOCKREC), EXPECTED_CNT
	cmpl	$CONSTANT_CVM_INVALID_REENTRY_COUNT, EXPECTED_CNT
	je	_monenterFastReentryFailed
	movl	EXPECTED_CNT, NEW_COUNT
	incl	NEW_COUNT
	CVMJITX86_CMPXCHG(NEW_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC))
#undef EXPECTED_CNT
#undef NEW_COUNT
	/* Reload EE since it was clobbered by NEW_COUNT */
	movl    4 + OFFSET_CVMCCExecEnv_ee(%esp), EE
	/* cmpxchg sets ZF := OFFSET_CVMOwnedMonitor_count(LOCKREC) == EXPECTED_CNT */
	jne	_monenterFastReentryFailed

	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	ret

_monenterFastRetryFailed:
_monenterFastReentryFailed:
_monenterRecordNotAvailable:
	/* Let C helper do the hard stuff: */
	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
#define SCRATCH A1
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, AS_ARG(EE), AS_ARG(OBJ), SYM_NAME(CVMCCMruntimeMonitorEnter))
#undef  SCRATCH
#undef EE
#undef OBJ
#undef LOCKREC

#undef SCRATCH
#endif
SET_SIZE( CVMCCMruntimeMonitorEnterGlue )

	

/*
 * Entry point for monitorexit.
 */
ENTRY(CVMCCMruntimeMonitorExitGlue )
	#
	# Arguments:
	# 	A3 = 'obj'
	#
	# Also incoming:
	#	JFP, JSP
	#	ccee = %esp
	#	ee   =  ccee->eeX
	#
#ifdef CVM_JIT_CCM_USE_C_HELPER
#define SCRATCH	A4
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), SYM_NAME(CVMCCMruntimeMonitorExit))
#undef  SCRATCH

#else
	cmpl	$0, A3
#ifdef CVM_DEBUG_ASSERTS
	movl	(%esp), A1
#endif
	je	SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlue)
	
#define LOCKREC         A2
#define OBJ		A3
#define EE		%esi

/*
 *     | 4. word (free)  <- 16 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 3. word (free)  <- 12 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 2. %edi (JSP)   <-  8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 1. %esi         <-  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     :	
 *     | 1. ret addr.    <-      0(%esp)
 *     +---------------
 */

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	movl	%esi, 4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)

	/* Check to see if the object is locked with a fastlock: */
	movl	OFFSET_CVMObjectHeader_various32(OBJ), LOCKREC /* Get obits */
	testl	$0x3, LOCKREC /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
	jne	_monexitFastTryUnlockFailed /* If not, we failed. */

	cmpl	$0, LOCKREC
	je	_monexitFastTryUnlockFailed

	/* If we get here, then the object is locked with a fastlock: */

	movl    4 + OFFSET_CVMCCExecEnv_ee(%esp), EE

	/* Make sure that the current thread owns the monitor: */
	cmpl	OFFSET_CVMOwnedMonitor_owner(LOCKREC), EE
	jne	_monexitFastTryUnlockFailed	/* If not owner, we failed. */
	
#define EXPECTED_CNT    A1
#define NEW_COUNT       A4

	/* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
	movl	OFFSET_CVMOwnedMonitor_count(LOCKREC), EXPECTED_CNT
	cmpl	$CONSTANT_CVM_INVALID_REENTRY_COUNT, EXPECTED_CNT
	je	_monexitFastTryUnlockFailed
	movl	EXPECTED_CNT, NEW_COUNT
	decl	NEW_COUNT
	jz	_monexitDoUnlock	/* If zero, then unlock */

	/* new monitor count > 0, so just update it and we're done */
	CVMJITX86_CMPXCHG(NEW_COUNT, OFFSET_CVMOwnedMonitor_count(LOCKREC))
	jne	_monexitFastTryUnlockFailed
	/* we're done! monitor count was successfully decrement */
	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	ret	/* return to caller */

#undef EXPECTED_CNT
#undef NEW_COUNT

_monexitDoUnlock:	
#define OBITS		A1
#define NBITS		A4
	/* If we get here, then the re-entry count has reached 0. */
	# Restore the obits to the object header:
	movl	LOCKREC, OBITS	/* initialize OBITS */
	movl	OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC), NBITS
	CVMJITX86_CMPXCHG(NBITS, OFFSET_CVMObjectHeader_various32(OBJ))
	jne	_monexitFastTryUnlockFailed

#ifdef CVM_DEBUG
	# Make the lockrec play nice with the debug assertions:
	movl	$0, OFFSET_CVMOwnedMonitor_count(LOCKREC)
	movl	$CONSTANT_CVM_OWNEDMON_FREE, OFFSET_CVMOwnedMonitor_state(LOCKREC)
	
	movl	$0, OFFSET_CVMOwnedMonitor_u_fast_bits(LOCKREC)
	movl	$0, OFFSET_CVMOwnedMonitor_object(LOCKREC)
#endif

#undef NBITS
#undef OBITS

	# Check if the lockrec is the first one on the thread's owned list:
	cmpl	OFFSET_CVMExecEnv_objLocksOwned(EE), LOCKREC
	jne	_monexitFastTryUnlockFindPrevLockRecord

#define SCRATCH	A4
	# Remove the lockrec from the ee's owned list: 
	movl	OFFSET_CVMOwnedMonitor_next(LOCKREC), SCRATCH
	movl	SCRATCH, OFFSET_CVMExecEnv_objLocksOwned(EE)

_monexitFastTryUnlockAddLockRecordToFreeList:
	# Add the lockrec to the ee's free list:
	movl	OFFSET_CVMExecEnv_objLocksFreeOwned(EE), SCRATCH
	movl	SCRATCH, OFFSET_CVMOwnedMonitor_next(LOCKREC)
	movl	LOCKREC, OFFSET_CVMExecEnv_objLocksFreeOwned(EE)

	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	ret	/* Return to the caller. */

#define PREV_REC	A1
_monexitFastTryUnlockFindPrevLockRecord:
	movl	OFFSET_CVMExecEnv_objLocksOwned(EE), PREV_REC
_monexitFastTryUnlockFindPrevLockRecord_loop:
	cmpl	OFFSET_CVMOwnedMonitor_next(PREV_REC), LOCKREC
	je	_monexitFastTryUnlockFoundPrevLockRecord
	movl	OFFSET_CVMOwnedMonitor_next(PREV_REC), PREV_REC
	jmp	_monexitFastTryUnlockFindPrevLockRecord_loop

_monexitFastTryUnlockFoundPrevLockRecord:
	# Remove the lockrec from the ee's owned list: 
	movl	OFFSET_CVMOwnedMonitor_next(LOCKREC), SCRATCH
	movl	SCRATCH, OFFSET_CVMOwnedMonitor_next(PREV_REC)
	jmp	_monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC


_monexitFastTryUnlockFailed:
	movl	4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), %esi
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG(A3), SYM_NAME(CVMCCMruntimeMonitorExit))

#undef SCRATCH	
#undef EE
#undef OBJ
#undef LOCKREC
#endif
SET_SIZE( CVMCCMruntimeMonitorExitGlue )

/* CVMCCMruntimeThrowNullPointerExceptionGlue */
ENTRY(CVMCCMruntimeThrowNullPointerExceptionGlue )

#define SCRATCH A4
	ffree	%st(0)
	ffree	%st(1)
	ffree	%st(2)
	ffree	%st(3)
	ffree	%st(4)
	ffree	%st(5)
	ffree	%st(6)
	ffree	%st(7)
#if defined(CVM_DEBUG_ASSERTS) && defined(CVMJIT_TRAP_BASED_NULL_CHECKS)
	cmpl	(%esp), A1	/* see handleSegv */
	je	CVMCCMruntimeThrowNullPointerExceptionGlue_passed_assert
	pushl	$CVMCCMruntimeThrowNullPointerExceptionGlue_assert_expression
	pushl	$__LINE__
	pushl	$ccmglue_cpu_filename
	call	SYM_NAME(CVMassertHook)
CVMCCMruntimeThrowNullPointerExceptionGlue_passed_assert:
#endif
	FIXUP_FRAMES_0(JFP, SCRATCH, CVMCCMruntimeThrowNullPointerExceptionGlue_L1)
CVMCCMruntimeThrowNullPointerExceptionGlue_L1:
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG($SYM_NAME(java_lang_NullPointerException_Classblock)), AS_ARG($NullPointerExceptionString), SYM_NAME(CVMCCMruntimeThrowClass))

#undef SCRATCH

SET_SIZE( CVMCCMruntimeThrowNullPointerExceptionGlue )

/* CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue */
ENTRY(CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )

#define SCRATCH A4	

	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue_L1)
CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue_L1:	
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, EE_AS_ARG, AS_ARG($SYM_NAME(java_lang_ArrayIndexOutOfBoundsException_Classblock)), AS_ARG($ArrayIndexOutOfBoundsExceptionString), SYM_NAME(CVMCCMruntimeThrowClass))

#undef SCRATCH
	
SET_SIZE( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )

/* CVMCCMruntimeGCRendezvousGlue */
ENTRY(CVMCCMruntimeGCRendezvousGlue )
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	addl $(CVMCPU_NUM_NOPS_FOR_GC_PATCH - CVMCPU_GC_RENDEZVOUS_INSTRUCTION_SIZE),(%esp)
#endif
	ffree	%st(0)
	ffree	%st(1)
	ffree	%st(2)
	ffree	%st(3)
	ffree	%st(4)
	ffree	%st(5)
	ffree	%st(6)
	ffree	%st(7)
	CALL_HELPER_2(A1 /* scratch */, CCEE_AS_ARG, EE_AS_ARG, SYM_NAME(CVMCCMruntimeGCRendezvous))
SET_SIZE( CVMCCMruntimeGCRendezvousGlue )

/* call CVMCCMruntimeSimpleSyncUnlock */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
ENTRY(CVMCCMruntimeSimpleSyncUnlockGlue)
	CALL_HELPER_2(A3 /* scratch */, EE_AS_ARG, AS_ARG(A2), \
	    SYM_NAME(CVMCCMruntimeSimpleSyncUnlock))
SET_SIZE( CVMCCMruntimeSimpleSyncUnlockGlue )
#endif

NullPointerExceptionString:
	.asciz "Null pointer dereference"
	
ArrayIndexOutOfBoundsExceptionString:
	.asciz "Array index out of bounds"

divideByZeroString:
	.asciz "/ by zero"
