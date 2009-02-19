/*
 * @(#)ccmallocators_cpu.S	1.14 06/10/10
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

/***********************************
 * Java heap allocators
 ***********************************/

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

#define CALL_HELPER_AND_PASS_CCEE(HELPER)				\
	add	%sp, MINFRAME, %o0;					\
	add	%o7, 8, %o7;						\
	st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC];		\
	sub	%o7, 8, %o7;						\
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack];		\
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]; \
	BRANCH_TO_VM_FUNCTION(HELPER);					\
	nop

/*
 * Entry point for allocating an object.
 */
ENTRY ( CVMCCMruntimeNewGlue )
	!
	! Arguments:
	!	Arg2 = 'cb'
	!
	! Also incoming:
	!	JFP
	!	JSP
	!	sp
	!
#if 0
	!
	! If you just want to call the C helper and write very little
	! assemble code:
	!
	FIXUP_FRAMES_2(JFP, %g1, %o1, %o7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeNew)
#endif

#define OBJ    %o0
#define CB     %o1
#define OBJSIZE %o2
#define ALLOCNEXT  OBJSIZE
#define CVMGLOBALS %o3

	sethi	%hi(SYM_NAME(CVMglobals)), CVMGLOBALS
	or	CVMGLOBALS, %lo(SYM_NAME(CVMglobals)), CVMGLOBALS

	lduh	[CB + OFFSET_CVMClassBlock_accessFlagsX], %g1
	andcc	%g1, CONSTANT_CLASS_ACC_FINALIZABLE, %g0  /* finalizable? */
	bne	GOSLOW         /* go slow route if finalizable */
	mov	1, %g1		/* 1 == locked flag for fastHeapLock */

	! lock using cas
	add     CVMGLOBALS, OFFSET_CVMGlobalState_fastHeapLock, %o4
	swap	[%o4], %g1
	cmp	%g1, %g0	/* check if already locked. */
	bne	GOSLOW		/* already locked. Bail. */
	nop

	!
	! Allocate inline
	!
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocPtrPtr], %o4
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocTopPtr], %o5
	ld	[%o4], OBJ	/* OBJ <- allocPtr == function result */
	lduh	[CB + OFFSET_CVMClassBlock_instanceSizeX], OBJSIZE
	ld	[%o5], %o5	/* o5 <- allocTop */
	addcc	OBJ, OBJSIZE, ALLOCNEXT /* g1 <- allocNext (allocPtr + size) */
	! Check for overflow
	bvs	GOUNLOCKANDSLOW	/* branch if we went passed top of memory */
	nop
	cmp	ALLOCNEXT, %o5	/* Is g1 <= allocTop */
	bgu	GOUNLOCKANDSLOW
	nop

	st  	ALLOCNEXT, [%o4]	/* commit the new allocPtr */

#ifdef CVM_FASTALLOC_STATS
	! Count fast locks
	sethi	%hi(fastLockCount), %g1
	or	%g1, %lo(fastLockCount), %g1 /* r0 <- fastLockCount */
	ld	[%g1], %o4
	add	%o4, 1, %o4
	st	%o4, [%g1]
#endif

	! Initialize the object header.
	mov	2, %g1		/* CVM_LOCKSTATE_UNLOCKED */
	st	CB, [OBJ]	/* cb is first field of object */
	st	%g1, [OBJ + 4]	/* initialize variousWord */

	b	LOOPTEST
	add	OBJ, 8, %o5

INITLOOP:
	st	%g0, [%o5]
	add	%o5, 4, %o5	/* Next object field */
LOOPTEST:
	cmp	%o5, ALLOCNEXT
	bcs	INITLOOP	/* decrement ctr and branch if ctr != 0 */
	nop
INITDONE:	
	! Unlock fast lock
	! r0 is already 0 here. Store it into fastHeapLock
	/*sync*/		/* be MP safe */
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock]	/* store 0 into fastHeapLock */
	! return to compiled code. The object is in r3.
	retl
	nop

GOUNLOCKANDSLOW:
	! Unlock by stuffing a zero in the lock
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock] /* store 0 into fastHeapLock */

GOSLOW:
	! Flush our state.
	/* save cb */
	st	CB, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage]
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
	st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	add	%o7, 8, %o7
	st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] /* save return PC */
	sub	%o7, 8, %o7

	! Call CVMgcAllocNewInstance
	! o0 = arg1 = ee
	! o1 = arg2 = cb
#ifdef CVM_CCM_COLLECT_STATS
	CALL_VM_FUNCTION(CVMgcAllocNewInstanceSpecial)
#else
	FIXUP_FRAMES_2(JFP, %g1, %o1, %o7)
	CALL_VM_FUNCTION(CVMgcAllocNewInstance)
#endif
	mov    EE, %o0          /* delay slot: arg1 = EE */

	! return if successful
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	cmp	%o0, 0
	beq	ALLOCNEWINSTANCEFAIL
	nop

	jmpl	%o7, %g0	/* return if successful */
	nop

ALLOCNEWINSTANCEFAIL:
	! Out of memory. Throw exception and return to interpreter.
	mov	EE, %o0			/* arg1 = EE */
	sethi	%hi(cbString), %o1
	or	%o1, %lo(cbString), %o1	/* arg2 = "%C" */
	/* arg3 = cb */
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], %o2
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	nop
	/* arg1 = CCEE */
	add	%sp, MINFRAME, %o0
	sethi	%hi(SYM_NAME(CVMJITexitNative)), %o7
	or	%o7, %lo(SYM_NAME(CVMJITexitNative)), %o7
	jmpl	%o7, %g0	/* call CVMJITexitNative */
	nop
#undef OBJ
#undef CB
#undef OBJSIZE
#undef ALLOCNEXT
#undef CVMGLOBALS

	SET_SIZE( CVMCCMruntimeNewGlue )

/*
 * Entry point for allocating an array of a basic type.
 */
ENTRY ( CVMCCMruntimeNewArrayGlue )
	!
	! Arguments:
	!	ARG1 = elementSize
	!	ARG2 = dimension
	!	ARG3 = arrCB
	!
	! Also incoming:
	!	JFP
	!	JSP
	!	sp
	!
#if 0
	!
	! If you just want to call the C helper and write very little assembler
	! code, then just the following 2 lines are needed.
	!
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeNewArray)
#endif

#define OBJ     %o0
#define LEN     %o1
#define ARRCB   %o2
#define OBJSIZE %o5
#define CVMGLOBALS %l0

	st	%l0, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4]
	sethi   %hi(SYM_NAME(CVMglobals)), CVMGLOBALS
        or      CVMGLOBALS, %lo(SYM_NAME(CVMglobals)), CVMGLOBALS

	!
	! Check if length is negative or too big. If it is, bail out
	!
	mov	1, %g1
	sll	%g1, 28, %g1
	cmp	LEN, %g1 /* must be less than 0x10000000 */
	bgu	ARR_BADINDEX	/* bail if negative length or too big */
	nop

	! Now compute instance size of the array
	! o0 holds element size
	! LEN holds length
	!
	! OBJSIZE = roundup(elemsize * length + 12)
	!
	! which is equal to
	!	
	! (elemsize * length + 15) & ~3
	!
	umul	%o0, LEN, OBJSIZE	/* elemsize * length */
	add	OBJSIZE, 15, OBJSIZE
	andn	OBJSIZE, 0x3, OBJSIZE	/* clear rightmost 2 bits */

	! lock using cas
	mov     1, %g1          /* 1 == locked flag for fastHeapLock */
	add     CVMGLOBALS, OFFSET_CVMGlobalState_fastHeapLock, %o4
	swap	[%o4], %g1
	cmp	%g1, %g0	/* check if already locked. */
	bne	ARR_GOSLOW	/* already locked. Bail. */
	nop

	!
	! Allocate inline
	!
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocPtrPtr], %o4
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocTopPtr], %o3
	ld	[%o4], OBJ	/* OBJ <- allocPtr == function result */
	ld	[%o3], %o3	/* o3 <- allocTop */
	addcc	OBJ, OBJSIZE, %g1 /* g1 <- allocNext (allocPtr + size) */
	! Check for overflow
	bvs	ARR_GOUNLOCKANDSLOW /* branch if we passed top of memory */
	nop
	cmp	%g1, %o3 	/* Is g1 <= allocTop */
	bgu	ARR_GOUNLOCKANDSLOW
	nop

	st  	%g1, [%o4]	/* commit the new allocPtr */

	/* From now on, o4 can be trashed. */
#ifdef CVM_FASTALLOC_STATS
	! Count fast locks
	sethi	%hi(fastLockCount), %g1
	or	%g1, %lo(fastLockCount), %g1 /* r0 <- fastLockCount */
	ld	[%g1], %o4
	add	%o4, 1, %o4
	st	%o4, [%g1]
#endif

	! Initialize the object header.
	mov	2, %g1		/* CVM_LOCKSTATE_UNLOCKED */
	st	ARRCB, [OBJ]	/* cb is first field of object */
	st	%g1, [OBJ + 4]	/* initialize variousWord */
	st	LEN, [OBJ + 8]	/* initialize array length */

#define	ALLOCNEXT	%o4
	add	OBJ, OBJSIZE, ALLOCNEXT
	b	ARR_LOOPTEST
	add     OBJ, 12, %o3

ARR_INITLOOP:
	st	%g0, [%o3]
	add	%o3, 4, %o3	/* Next object field */
ARR_LOOPTEST:
	cmp	%o3, ALLOCNEXT
	bcs	ARR_INITLOOP
	nop
#undef	ALLOCNEXT
ARR_ENDINIT:	
	! Unlock fast lock
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock]	/* store 0 into fastHeapLock */
	! return to compiled code. The object is in o0.
	ld     [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4], %l0
	retl
	nop

ARR_GOUNLOCKANDSLOW:
	! Unlock by stuffing a zero in the lock
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock]

ARR_GOSLOW:
	! Flush our state.
	/* save cb */
	st	ARRCB, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage]
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	add	%o7, 8, %o7
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] /* save return PC */
	sub	%o7, 8, %o7

	! Call CVMgcAllocNewInstance
	!   o0 = arg1 = ee
	!   o1 = arg2 = instance size
	!   o2 = arg3 = arrayCB
	!   o3 = arg4 = array length
	mov     LEN, %o3        /* arg4 = array length */
	mov	OBJSIZE, %o1	/* arg2 = instance size */
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o3, %o7)
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], ARRCB
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)
	mov     EE, %o0          /* arg1 = EE */

	! return if successful
        ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	cmp	%o0, 0
	beq	ARR_ALLOC_FAIL
	nop
	ld      [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4], %l0
	jmpl	%o7, %g0	/* return if successful */
	nop

ARR_ALLOC_FAIL:
	/* arg3 = cb */
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], %o2
ARR_OUT_OF_MEMORY:	
	! Out of memory. Throw exception and return to interpreter.
	sethi	%hi(cbString), %o1
	or	%o1, %lo(cbString), %o1	/* arg2 = "%C" */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	mov     EE, %o0                 /* arg1 = EE */

ARR_EXIT_NATIVE:
	add     %sp, MINFRAME, %o0      /* arg1 = CCEE */
	sethi	%hi(SYM_NAME(CVMJITexitNative)), %o7
	or	%o7, %lo(SYM_NAME(CVMJITexitNative)), %o7
	jmpl	%o7, %g0	/* call CVMJITexitNative */
	nop

ARR_BADINDEX:
	! flush state first
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	add	%o7, 8, %o7
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] /* save return PC */
	sub	%o7, 8, %o7
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)

	cmp	LEN, 0	/* check if array length < 0 */
	bge	ARR_OUT_OF_MEMORY /* array too big */
	nop
	! The index is negative. Throw NegativeArraySizeException 
	mov	EE, %o0		/* arg1 = EE */
	mov	0, %o1		/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	nop
	b	ARR_EXIT_NATIVE
	nop

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef CVMGLOBALS

	SET_SIZE( CVMCCMruntimeNewArrayGlue )

/*
 * Entry point for allocating an array of a basic type.
 */
ENTRY ( CVMCCMruntimeANewArrayGlue )
	!
	! Arguments
	!	ARG2 = dimension
	!	ARG3 = arrayCB
	!
	! Also incoming:
	!	JFP
	!	JSP
	!	sp
	!
#if 0
	! If you just want to call the C helper and write very little assembler
        ! code, then just the following 2 lines are needed.
	!
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeANewArray)
#endif

#define OBJ     %o0
#define LEN     %o1
#define ARRCB	%o2
#define OBJSIZE %o5
#define	CVMGLOBALS %l0

	st      %l0, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4]
        sethi   %hi(SYM_NAME(CVMglobals)), CVMGLOBALS
        or      CVMGLOBALS, %lo(SYM_NAME(CVMglobals)), CVMGLOBALS

	!
	! Check if length is negative or too big. If it is, bail out
	!
	mov	1, %g1
	sll	%g1, 28, %g1
	cmp	LEN, %g1	/* must be less than 0x10000000 */
	bgu	OBJARR_BADINDEX	/* bail if negative length or too big */
	nop

	! Now compute instance size of the array
	! LEN holds length
	!
	! OBJSIZE = (LEN << 2 + 12)
	!
	!
	sll	LEN, 2, OBJSIZE
	add	OBJSIZE, 12, OBJSIZE

	! lock using cas
	add	CVMGLOBALS, OFFSET_CVMGlobalState_fastHeapLock, %o4
	mov	1, %g1		/* 1 == locked flag for fastHeapLock */
	swap	[%o4], %g1
	cmp	%g1, %g0	/* check if already locked. */
	bne	OBJARR_GOSLOW	 /* already locked. Bail. */
	nop

	!
	! Allocate inline
	!
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocPtrPtr], %o4
	ld	[CVMGLOBALS + OFFSET_CVMGlobalState_allocTopPtr], %o3
	ld	[%o4], OBJ	/* OBJ <- allocPtr == function result */
	ld	[%o3], %o3	/* o3 <- allocTop */
	addcc	OBJ, OBJSIZE, %g1 /* g1 <- allocNext (allocPtr + size) */
	! Check for overflow
	bvs	OBJARR_GOUNLOCKANDSLOW /* branch if we passed top of memory */
	nop
	cmp	%g1, %o3	/* Is g1 <= allocTop */
	bgu	OBJARR_GOUNLOCKANDSLOW
	nop

	st  	%g1, [%o4]	/* commit the new allocPtr */

#ifdef CVM_FASTALLOC_STATS
	! Count fast locks
	sethi	%hi(fastLockCount), %g1
	or	%g1, %lo(fastLockCount), %g1 /* r0 <- fastLockCount */
	ld	[%g1], %o4
	add	%o4, 1, %o4
	st	%o4, [%g1]
#endif

	! Initialize the object header.
	mov	2, %g1		/* CVM_LOCKSTATE_UNLOCKED */
	st	ARRCB, [OBJ]	/* cb is first field of object */
	st	%g1, [OBJ + 4]	/* initialize variousWord */
	st	LEN, [OBJ + 8]	/* initialize array length */

#define ALLOCNEXT	%o4
	add	OBJ, OBJSIZE, ALLOCNEXT
	b	OBJARR_LOOPTEST
	add     OBJ, 12, %o3

OBJARR_INITLOOP:
	st	%g0, [%o3]
	add	%o3, 4, %o3	/* Next object field */
OBJARR_LOOPTEST:
	cmp	%o3, ALLOCNEXT
	bcs	OBJARR_INITLOOP
	nop
#undef ALLOCNEXT
OBJARR_ENDINIT:	
	! Unlock fast lock
	! r0 is already 0 here. Store it into fastHeapLock
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock]	/* store 0 into fastHeapLock */
	! return to compiled code. The object is in o0.
	ld     [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4], %l0
	retl		/* return */
	nop

OBJARR_GOUNLOCKANDSLOW:
	! Unlock by stuffing a zero in the lock
	st	%g0, [CVMGLOBALS + OFFSET_CVMGlobalState_fastHeapLock] /* store 0 into fastHeapLock */

OBJARR_GOSLOW:
	! Flush our state.
	/* save cb */
	st	ARRCB, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage]
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
        st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	add	%o7, 8, %o7
        st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] /* save return PC */
	sub	%o7, 8, %o7

	! Call CVMgcAllocNewInstance
	!   r3 = arg1 = ee
	!   r4 = arg2 = instance size
	!   r5 = arg3 = arrayCb (already in r5)
	!   r6 = arg4 = array length
	mov	LEN, %o3	/* arg4 = array length */
	mov	OBJSIZE, %o1	/* arg2 = instance size */
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o3, %o7)
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], ARRCB /* reload ARRCB */
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)
	mov     EE, %o0         /* arg1 = EE */

	! return if successful
	ld	[JFP + OFFSET_CVMCompiledFrame_PC], %o7
	cmp	%o0, 0
	beq	OBJARR_ALLOC_FAIL
	nop
	ld     [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage + 4], %l0
	jmpl	%o7, %g0		/* return if successful */
	nop

OBJARR_ALLOC_FAIL:
	! setup some arguments for CVMthrowOutOfMemoryError
	/* arg3 = cb */
	ld	[%sp + MINFRAME + OFFSET_CVMCCExecEnv_ccmStorage], %o2

OBJARR_OUT_OF_MEMORY:	
	! Out of memory. Throw exception and return to interpreter.
	sethi   %hi(cbStringArr), %o1
	or      %o1, %lo(cbStringArr), %o1      /* arg2 = "[%C" */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	mov     EE, %o0                 /* arg1 = EE */

OBJARR_EXIT_NATIVE:
	add	%sp, MINFRAME, %o0      /* arg1 = CCEE */
	sethi	%hi(SYM_NAME(CVMJITexitNative)), %o7
	or	%o7, %lo(SYM_NAME(CVMJITexitNative)), %o7
	jmpl	%o7, %g0		/* call CVMJITexitNative */
	nop

OBJARR_BADINDEX:
	! flush state first
	st	JSP, [JFP + OFFSET_CVMFrame_topOfStack]
	st	JFP, [EE + OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	add	%o7, 8, %o7
	st	%o7, [JFP + OFFSET_CVMCompiledFrame_PC] /* save return PC */
	sub	%o7, 8, %o7
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)

	cmp     LEN, 0  /* check if array length < 0 */
	bge	OBJARR_OUT_OF_MEMORY /* array too big */
	nop
	! The index is negative. Throw NegativeArraySizeException 
	mov	EE, %o0		/* arg1 = EE */
	mov	0, %o1		/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	nop
	b	OBJARR_EXIT_NATIVE
	nop

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef CVMGLOBALS

	SET_SIZE( CVMCCMruntimeANewArrayGlue )

/*
 * Allocate a multidimensional array.
 */
ENTRY ( CVMCCMruntimeMultiANewArrayGlue )
	!
	! Arguments:
	!	ARG2 = nDimensions
	!	ARG3 = arrCb
	!	ARG4 = address of dimension array
	!
	! Flush our state.
	!
	FIXUP_FRAMES_3(JFP, %g1, %o1, %o2, %o7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeMultiANewArray)
	SET_SIZE ( CVMCCMruntimeMultiANewArrayGlue )

cbString:
	.asciz "%C"
cbStringArr:
	.asciz "[%C"
