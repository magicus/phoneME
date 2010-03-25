/*
 * @(#)ccmallocators_cpu.S	1.6 06/10/23
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


/***********************************
 * Java heap allocators
 ***********************************/

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

#ifdef CVM_DEBUG_ASSERTS
.file "ccmallocators_cpu.S"
ccmallocators_cpu_filename:
.asciz "ccmallocators_cpu.S"
CVMCCMruntimeNewGlue_assert_expression:
CVMCCMruntimeNewArrayGlue_assert_expression:
CVMCCMruntimeANewArrayGlue_assert_expression:
CVMCCMruntimeNewGlue_unlock1_assert_expression:
CVMCCMruntimeNewGlue_unlock2_assert_expression:
CVMCCMruntimeNewArrayGlue_unlock1_assert_expression:
CVMCCMruntimeNewArrayGlue_unlock2_assert_expression:
CVMCCMruntimeANewArrayGlue_unlock1_assert_expression:
CVMCCMruntimeANewArrayGlue_unlock2_assert_expression:
.asciz "heap is unlocked"
#endif /* CVM_DEBUG_ASSERTS */

/*
 * Entry point for allocating an object.
 */
ENTRY(CVMCCMruntimeNewGlue )
	#
	# Arguments:
	#	A2 = 'cb'
	#
	# Also incoming:
	#	JFP
	#	JSP
	#	sp
	#

#if 0
#define SCRATCH A4
	#
	# If you just want to call the C helper and write very little
	# assemble code:
	#
	FIXUP_FRAMES_1(JFP, SCRATCH, A2, CVMCCMruntimeNewGlue_L1)
CVMCCMruntimeNewGlue_L1:
	CALL_HELPER_2(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), SYM_NAME(CVMCCMruntimeNew))
#undef SCRATCH
#else /* 0 */

#define CB         A2
#define SCRATCH    A4

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	/* nothing to do */
	
	testw	$CONSTANT_CLASS_ACC_FINALIZABLE, OFFSET_CVMClassBlock_accessFlagsX(CB)
	jne	GOSLOW         /* go slow route if finalizable */

	# lock the heap
	movl	$1, SCRATCH		/* 1 == locked flag for fastHeapLock */
	xchgl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals), SCRATCH
	cmpl	$0, SCRATCH	/* check if already locked. */
	jne	GOSLOW		/* already locked. Bail. */
#undef SCRATCH

#define OBJ    A1	/* function result */
#define TOPPTR A3
#define ALLOCNEXT  A4

	
	#
	# Allocate inline
	#
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, OBJ
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocTopPtr, TOPPTR
	movl	0(OBJ), OBJ	/* OBJ <- allocPtr == function result */
	movl	0(TOPPTR), TOPPTR
	movzwl	OFFSET_CVMClassBlock_instanceSizeX(CB), ALLOCNEXT
	addl	OBJ, ALLOCNEXT /* allocNext (allocPtr + size) */
	# Check for overflow
	jo	GOUNLOCKANDSLOW
	cmpl	TOPPTR, ALLOCNEXT
#undef  TOPPTR
	ja	GOUNLOCKANDSLOW
#define ALLOCPTRPTR A3
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, ALLOCPTRPTR
	movl	ALLOCNEXT, 0(ALLOCPTRPTR) 	/* commit the new allocPtr */
#undef ALLOCPTRPTR
		
#ifdef CVM_FASTALLOC_STATS
	TODO
#endif

#define FIELD A3
	# Initialize the object header.
	movl	OBJ, FIELD
	movl	CB, 0(FIELD)	/* cb is first field of object */
	movl	$2, 4(FIELD)	/* CVM_LOCKSTATE_UNLOCKED: initialize variousWord */

	addl	$8, FIELD
	jmp	LOOPTEST
INITLOOP:
	movl	$0, 0(FIELD)
	addl	$4, FIELD		/* Next object field */
LOOPTEST:
	cmp	ALLOCNEXT, FIELD
	jne	INITLOOP
#undef FIELD
#undef ALLOCNEXT

INITDONE:	
        # unlock the heap
	lock ; decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals) 

	# return to compiled code. The object is in A1.
	ret
#undef OBJ
		
GOUNLOCKANDSLOW:
        # unlock the heap
	lock decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals)
GOSLOW:	
#define SCRATCH A4
	movl	CB,  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)   # +4 because ret. addr. on stack
#undef CB
	# Call CVMgcAllocNewInstance
	# arg1 = ee
	# arg2 = cb
#ifdef CVM_CCM_COLLECT_STATS
	TODO(rr)
	CALL_HELPER_2_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG(CB), CVMgcAllocNewInstanceSpecial)
#else
	FIXUP_FRAMES_0(JFP, SCRATCH, CVMCCMruntimeNewGlue_GOSLOW_L1)
CVMCCMruntimeNewGlue_GOSLOW_L1:	
	CALL_HELPER_2_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* CB */, SYM_NAME(CVMgcAllocNewInstance))
#endif

	# return if successful
	cmpl	$0, A1
	je	ALLOCNEWINSTANCEFAIL

	ret	/* return if successful */

ALLOCNEWINSTANCEFAIL:
	# Out of memory. Throw exception and return to interpreter.
	# arg1 = ee
	# arg2 = format string
	# arg3 = array cb
	CALL_HELPER_3_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG($cbString), AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* CB */, SYM_NAME(CVMthrowOutOfMemoryError))

	addl	$4, %esp	/* pop return address */
	pushl	%esp		/* arg1 = CCEE */
	call	SYM_NAME(CVMJITexitNative)
	
#undef SCRATCH
#endif /* 0 */
SET_SIZE( CVMCCMruntimeNewGlue )

/* CVMCCMruntimeNewArrayGlue */
ENTRY(CVMCCMruntimeNewArrayGlue )
	#
	# Arguments:
	#	A1 = elementSize
	#	A2 = dimension
	#	A3 = arrCB
	#
	# Also incoming:
	#	JFP
	#	JSP
	#	sp
	#
	#

#if 0
#define SCRATCH A4
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 2 lines are needed.
	#
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeNewArrayGlue_L1)
CVMCCMruntimeNewArrayGlue_L1:		
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), SYM_NAME(CVMCCMruntimeNewArray))
#undef SCRATCH
#else /* 0 */

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	# nothing to do
	
#define LEN     A2
#define ARRCB   A3
#define OBJSIZE A4

/*
 *     | 4. word (free)  <- 16 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 3. word (free)  <- 12 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 2. LEN          <-  8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 1. ARRCB        <-  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     :	
 *     | 1. ret addr.    <-      0(%esp)
 *     +---------------
 */

	#
	# Check if length is negative or too big. If it is, bail out
	#
	cmpl	$0x10000000, LEN /* must be less than 0x10000000 */
	ja	ARR_BADINDEX	 /* bail if negative length or too big */

	# Now compute instance size of the array
	# A1 holds element size
	# LEN holds length
	#
	# OBJSIZE = roundup(elemsize * length + 12)
	#
	# which is equal to
	#	
	# (elemsize * length + 15) & ~3
	#
	movl	LEN, 8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)  /* save length */
	mull	LEN	/* elemsize * length */
	movl	A1 /* product */, OBJSIZE
	
	addl	$15, OBJSIZE
	andl	$~0x3, OBJSIZE	/* clear rightmost 2 bits */
#undef LEN
#undef  OBJSIZE

#define SCRATCH A2
	# lock the heap
	movl	$1, SCRATCH		/* 1 == locked flag for fastHeapLock */
	xchgl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals), SCRATCH
	cmpl	$0, SCRATCH	/* check if already locked. */
	jne	ARR_GOSLOW		/* already locked. Bail. */
#undef SCRATCH

#define OBJ    A1	/* function result */
#define TOPPTR A2
#define ALLOCNEXT  A4   /* OBJSIZE */
	
	#
	# Allocate inline
	#
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, OBJ
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocTopPtr, TOPPTR
	movl	0(OBJ), OBJ	/* OBJ <- allocPtr == function result */
	movl	0(TOPPTR), TOPPTR
	/* ALLOCNEXT holds OJBSIZE*/
	addl	OBJ, ALLOCNEXT /* allocNext (allocPtr + size) */
	# Check for overflow
	jo	ARR_GOUNLOCKANDSLOW
	cmpl	TOPPTR, ALLOCNEXT
#undef  TOPPTR
	ja	ARR_GOUNLOCKANDSLOW
#define ALLOCPTRPTR A2
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, ALLOCPTRPTR
	movl	ALLOCNEXT, 0(ALLOCPTRPTR) 	/* commit the new allocPtr */
#undef ALLOCPTRPTR

#ifdef CVM_FASTALLOC_STATS
	TODO(rr)
	# Count fast locks
	sethi	%hi(fastLockCount), %g1
	or	%g1, %lo(fastLockCount), %g1 /* r0 <- fastLockCount */
	ld	[%g1], %o4
	add	%o4, 1, %o4
	st	%o4, [%g1]
#endif

#define LEN   A2
	# Initialize the object header.
	movl	ARRCB, 0(OBJ)	/* cb is first field of object */
	movl	$2, 4(OBJ)	/* CVM_LOCKSTATE_UNLOCKED: initialize variousWord */
	movl	(8 + OFFSET_CVMCCExecEnv_ccmStorage)(%esp), LEN
	movl	LEN, 8(OBJ)
#undef LEN
#define FIELD A2
	movl	OBJ, FIELD	

	addl	$12, FIELD
	jmp	ARR_LOOPTEST
ARR_INITLOOP:
	movl	$0, 0(FIELD)
	addl	$4, FIELD		/* Next object field */
ARR_LOOPTEST:
	cmp	ALLOCNEXT, FIELD
	jne	ARR_INITLOOP
#undef FIELD

ARR_ENDINIT:	
	# unlock the heap
	lock  ; decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals) ; 

	# return to compiled code. The object is in A1.
	ret

/* SVMC_JIT c5041613 (dk) 2004-02-12 */
#ifdef  CVM_JIT_INLINE_NEWARRAY
ENTRY(CVMCCMruntimeNewArrayGounlockandslowGlue )
#endif /*  CVM_JIT_INLINE_NEWARRAY */
ARR_GOUNLOCKANDSLOW:
	# unlock the heap
	lock decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals)

	/* A4 holds ALLOCNEXT  */
	subl	OBJ, ALLOCNEXT /* A4 */
#define OBJSIZE   A4 /* make sure that OBJSIZE and ALLOCNEXT are the same register */
#undef  ALLOCNEXT
#undef  OBJ
#ifdef  CVM_JIT_INLINE_NEWARRAY
ENTRY(CVMCCMruntimeNewArrayGoslowGlue )
#endif /*  CVM_JIT_INLINE_NEWARRAY */
ARR_GOSLOW:	
	movl	ARRCB,  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
#undef  ARRCB
#define SCRATCH A1
	# Call CVMgcAllocNewArrayWithInstanceSize
	#   arg1 = ee
	#   arg2 = instance size
	#   arg3 = arrayCB
	#   arg4 = array length
	FIXUP_FRAMES_0(JFP, SCRATCH, CVMCCMruntimeNewArrayGlue_ARR_GOSLOW_L1)
CVMCCMruntimeNewArrayGlue_ARR_GOSLOW_L1:

	CALL_HELPER_4_NO_RET(SCRATCH,				               \
		 EE_AS_ARG,						       \
		 AS_ARG(OBJSIZE),					       \
		 AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* ARRCB */, \
	         AS_ARG_FROM_STACK(8 + OFFSET_CVMCCExecEnv_ccmStorage) /* LEN */,   \
		 SYM_NAME(CVMgcAllocNewArrayWithInstanceSize))
#undef SCRATCH
#undef OBJSIZE


	# return if successful
	cmpl	$0, A1
	je	ARR_ALLOC_FAIL

	ret	/* return if successful */
#undef OBJ
	
ARR_ALLOC_FAIL:
#define SCRATCH A1
	# Out of memory. Throw exception and return to interpreter.
	# arg1 = ee
	# arg2 = format string
	# arg3 = array cb
	CALL_HELPER_3_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG($cbString), AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* ARRCB */, SYM_NAME(CVMthrowOutOfMemoryError))

ARR_EXIT_NATIVE:	
	addl	$4, %esp	/* pop return address */
	pushl	%esp		/* arg1 = CCEE */
	call	SYM_NAME(CVMJITexitNative)

#ifdef  CVM_JIT_INLINE_NEWARRAY
ENTRY(CVMCCMruntimeNewArrayBadindexGlue )
#endif /*  CVM_JIT_INLINE_NEWARRAY */
ARR_BADINDEX:
	# flush state first
	# CALL_HELPER does the job
	
	FIXUP_FRAMES_1(JFP, SCRATCH, A2, CVMCCMruntimeNewArrayGlue_ARR_BADINDEX_L1)
CVMCCMruntimeNewArrayGlue_ARR_BADINDEX_L1:	

#define LEN     A2
#define ARRCB   A3
	movl	ARRCB,  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp) /* where ARR_ALLOC_FAIL expects it */
	cmpl	$0, LEN	/* check if array length < 0 */
	jge	ARR_ALLOC_FAIL /* array too big */
	# The index is negative. Throw NegativeArraySizeException 
	CALL_HELPER_2_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG($0), SYM_NAME(CVMthrowNegativeArraySizeException))
	jmp	ARR_EXIT_NATIVE

#undef LEN
#undef ARRCB
#undef CVMGLOBALS
#undef SCRATCH	
#endif /* 0 */
SET_SIZE( CVMCCMruntimeNewArrayGlue )
#ifdef CVM_JIT_INLINE_NEWARRAY	
SET_SIZE( CVMCCMruntimeNewArrayGounlockandslowGlue )
SET_SIZE( CVMCCMruntimeNewArrayGoslowGlue )
SET_SIZE( CVMCCMruntimeNewArrayBadindexGlue )
#endif /* CVM_JIT_INLINE_NEWARRAY */

/* CVMCCMruntimeANewArrayGlue */
ENTRY(CVMCCMruntimeANewArrayGlue )
	#
	# Arguments
	#	A2 = dimension
	#	A3 = arrCB
	#
	# Also incoming:
	#	JFP
	#	JSP
	#	sp
	#
	# If you just want to call the C helper and write very little assembler
        # code, then just the following 2 lines are needed.
	#
#if 0
#define SCRATCH A4
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeANewArrayGlue_L1)
CVMCCMruntimeANewArrayGlue_L1:		
	CALL_HELPER_3(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), SYM_NAME(CVMCCMruntimeANewArray))
#undef SCRATCH	
#else /* 0 */

	# saving callee saved registers (see CVMCPU_NON_VOLATILE_SET)
	# nothing to do
	
#define LEN     A2
#define ARRCB   A3
#define OBJSIZE A4

/*
 *     | 4. word (free)  <- 16 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 3. word (free)  <- 12 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 2. LEN          <-  8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     | 1. ARRCB        <-  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
 *     +---------------
 *     :	
 *     | 1. ret addr.    <-      0(%esp)
 *     +---------------
 */

	#
	# Check if length is negative or too big. If it is, bail out
	#
	cmpl	$0x10000000, LEN /* must be less than 0x10000000 */
	ja	OBJARR_BADINDEX	 /* bail if negative length or too big */

	# Now compute instance size of the array
	# LEN holds length
	#
	# OBJSIZE = (LEN << 2 + 12)
	#
	movl	LEN, 8 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)  /* save length */
	movl	LEN, OBJSIZE
	sall	$2, OBJSIZE
	addl	$12, OBJSIZE
#undef LEN

#define SCRATCH A2
	# lock the heap
	movl	$1, SCRATCH		/* 1 == locked flag for fastHeapLock */
	xchgl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals), SCRATCH
	cmpl	$0, SCRATCH	/* check if already locked. */
	jne	OBJARR_GOSLOW		/* already locked. Bail. */
#undef SCRATCH
#undef  OBJSIZE

#define OBJ    A1	/* function result */
#define TOPPTR A2
#define ALLOCNEXT  A4   /* OBJSIZE */
	
	#
	# Allocate inline
	#
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, OBJ
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocTopPtr, TOPPTR
	movl	0(OBJ), OBJ	/* OBJ <- allocPtr == function result */
	movl	0(TOPPTR), TOPPTR
	/* ALLOCNEXT holds OJBSIZE*/
	addl	OBJ, ALLOCNEXT /* allocNext (allocPtr + size) */
	# Check for overflow
	jo	OBJARR_GOUNLOCKANDSLOW
	cmpl	TOPPTR, ALLOCNEXT
#undef  TOPPTR
	ja	OBJARR_GOUNLOCKANDSLOW
#define ALLOCPTRPTR A2
	movl	SYM_NAME(CVMglobals) + OFFSET_CVMGlobalState_allocPtrPtr, ALLOCPTRPTR
	movl	ALLOCNEXT, 0(ALLOCPTRPTR) 	/* commit the new allocPtr */
#undef ALLOCPTRPTR

#ifdef CVM_FASTALLOC_STATS
	TODO(rr)
	# Count fast locks
	sethi	%hi(fastLockCount), %g1
	or	%g1, %lo(fastLockCount), %g1 /* r0 <- fastLockCount */
	ld	[%g1], %o4
	add	%o4, 1, %o4
	st	%o4, [%g1]
#endif

#define LEN   A2
	# Initialize the object header.
	movl	ARRCB, 0(OBJ)	/* cb is first field of object */
	movl	$2, 4(OBJ)	/* CVM_LOCKSTATE_UNLOCKED: initialize variousWord */
	movl	(8 + OFFSET_CVMCCExecEnv_ccmStorage)(%esp), LEN
	movl	LEN, 8(OBJ)
#undef LEN
#define FIELD A2
	movl	OBJ, FIELD	

	addl	$12, FIELD
	jmp	OBJARR_LOOPTEST
OBJARR_INITLOOP:
	movl	$0, 0(FIELD)
	addl	$4, FIELD		/* Next object field */
OBJARR_LOOPTEST:
	cmp	ALLOCNEXT, FIELD
	jne	OBJARR_INITLOOP
#undef FIELD

OBJARR_ENDINIT:	
	# unlock the heap
	lock  ; decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals)
	# return to compiled code. The object is in A1.
	ret


OBJARR_GOUNLOCKANDSLOW:
	# unlock the heap
	lock decl	OFFSET_CVMGlobalState_fastHeapLock + SYM_NAME(CVMglobals) /* store 0 into fastHeapLock */

	/* A4 holds ALLOCNEXT  */
	subl	OBJ, ALLOCNEXT /* A4 */
#define OBJSIZE   A4 /* make sure that OBJSIZE and ALLOCNEXT are the same register */
#undef  ALLOCNEXT
#undef  OBJ
OBJARR_GOSLOW:	
	movl	ARRCB,  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
#undef  ARRCB
#define SCRATCH A1
	# Call CVMgcAllocNewArrayWithInstanceSize
	#   arg1 = ee
	#   arg2 = instance size
	#   arg3 = arrayCB
	#   arg4 = array length
	FIXUP_FRAMES_0(JFP, SCRATCH, CVMCCMruntimeNewArrayGlue_OBJARR_GOSLOW_L1)
CVMCCMruntimeNewArrayGlue_OBJARR_GOSLOW_L1:

	CALL_HELPER_4_NO_RET(SCRATCH,				               \
		 EE_AS_ARG,						       \
		 AS_ARG(OBJSIZE),					       \
		 AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* ARRCB */, \
	         AS_ARG_FROM_STACK(8 + OFFSET_CVMCCExecEnv_ccmStorage) /* LEN */,   \
		 SYM_NAME(CVMgcAllocNewArrayWithInstanceSize))
#undef SCRATCH
#undef OBJSIZE


	# return if successful
	cmpl	$0, A1
	je	OBJARR_ALLOC_FAIL

	ret	/* return if successful */
#undef OBJ
	
OBJARR_ALLOC_FAIL:
#define SCRATCH A1
	# Out of memory. Throw exception and return to interpreter.
	# arg1 = ee
	# arg2 = format string
	# arg3 = array cb
	CALL_HELPER_3_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG($cbStringArr), AS_ARG_FROM_STACK(4 + OFFSET_CVMCCExecEnv_ccmStorage) /* ARRCB */, SYM_NAME(CVMthrowOutOfMemoryError))

OBJARR_EXIT_NATIVE:	
	addl	$4, %esp	/* pop return address */
	pushl	%esp		/* arg1 = CCEE */
	call	SYM_NAME(CVMJITexitNative)


OBJARR_BADINDEX:
	# flush state first
	# CALL_HELPER does the job
	
	FIXUP_FRAMES_1(JFP, SCRATCH, A2, CVMCCMruntimeNewArrayGlue_OBJARR_BADINDEX_L1)
CVMCCMruntimeNewArrayGlue_OBJARR_BADINDEX_L1:	

#define LEN     A2
#define ARRCB   A3
	movl	ARRCB,  4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp) /* where OBJARR_ALLOC_FAIL expects it */
	cmpl	$0, LEN	/* check if array length < 0 */
	jge	OBJARR_ALLOC_FAIL /* array too big */
	# The index is negative. Throw NegativeArraySizeException 
	CALL_HELPER_2_NO_RET(SCRATCH, EE_AS_ARG, AS_ARG($0), SYM_NAME(CVMthrowNegativeArraySizeException))
	jmp	OBJARR_EXIT_NATIVE

#undef LEN
#undef ARRCB
#undef CVMGLOBALS
#undef SCRATCH	
#endif /* 0 */

SET_SIZE( CVMCCMruntimeANewArrayGlue )

/* CVMCCMruntimeMultiANewArrayGlue */
#define SCRATCH A1
ENTRY(CVMCCMruntimeMultiANewArrayGlue )
	#
	# Arguments:
	#	A2 = nDimensions
	#	A3 = arrCb
	#	A4 = address of dimension array
	#
	# Flush our state.
	#
	FIXUP_FRAMES_2(JFP, SCRATCH, A2, A3, CVMCCMruntimeMultiANewArrayGlue_L1)
CVMCCMruntimeMultiANewArrayGlue_L1:		
	CALL_HELPER_4(SCRATCH, CCEE_AS_ARG, AS_ARG(A2), AS_ARG(A3), AS_ARG(A4), SYM_NAME(CVMCCMruntimeMultiANewArray))
#undef SCRATCH	

SET_SIZE( CVMCCMruntimeMultiANewArrayGlue )

cbString:
	.asciz "%C"
cbStringArr:
	.asciz "[%C"
