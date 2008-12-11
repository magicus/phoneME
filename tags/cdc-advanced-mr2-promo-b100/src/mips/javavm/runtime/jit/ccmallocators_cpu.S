/*
 * @(#)ccmallocators_cpu.S	1.19 06/10/10
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
	sw	ra,  OFFSET_CVMCompiledFrame_PC(JFP) ;			\
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP) ;			\
	sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE) ; \
	addiu	a0, sp, OFFSET_CStack_CCEE ;				\
	BRANCH_TO_VM_FUNCTION(HELPER)

/* IAI - 19 */
/* preload the memory to be allocated for the next object 
 * in the next NewGlue call 
 */
#ifdef IAI_PRELOAD_NEW_OBJECT
#define PREFETCH(offset, basereg) pref 1,offset(basereg)
#else
#define PREFETCH(offset, basereg)
#endif

/*
 * Entry point for allocating an object. The cb is the only argument and
 * is in a2 rather than a1. This is so we don't need to move it to a1
 * when it gets passed to CVMgcAllocNewInstance.
 */
ENTRY ( CVMCCMruntimeNewGlue )
	/* Arguments:           */
	/*	a1 = 'cb'       */
#ifdef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
	/*	a2 = 'instance size'	 IAI-03 */
	/*	a3 = 'accessflag'	 IAI-03 */
#endif

	/*
	 * TODO:	 Do a flag on class that means "big instance"
	 *		 So that we can check finalizability and big instance
	 *		 together.
	 */

#define	CB		a1
#define CVMGLOBALS	t0
	LA(CVMGLOBALS, CVMglobals)
/* IAI-03 */
#ifndef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
	/* Get access flags from class */
	lhu	a3, OFFSET_CVMClassBlock_accessFlagsX(CB)  # flags
#endif
	andi	a3, a3, CONSTANT_CLASS_ACC_FINALIZABLE     # finalizable?
	li	a0, 1	       		# locked	
	bne	a3, zero, GOSLOW	# bail if yes. Otherwise v1 == 0

	/* try lock using atomic swap */
	ll	jp, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	sc	a0, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	li	a3, 1	       		# locked	
	bne	a0, zero, 1f		# atomic op succeeded
	ll	jp, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	/* try again */
	sc	a3, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	beq	a3, zero, GOSLOW	# atomic op failed again
1:
	bne	jp, zero, GOSLOW	# already locked. Bail.
	/*
	 * Allocate inline
	 */
	lw	a0, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lw	a3, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lw	v0, (a0)   # v0 <- allocPtr  
	lw	a3, (a3)   # a3 <- allocTop
/* IAI-03 */
#ifndef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
	/* get instance size from class */
	lhu	a2, OFFSET_CVMClassBlock_instanceSizeX(CB)  # instance size
#endif
	addu	a2, v0, a2     # a2 <- allocNext (allocPtr v0 + size a2)

        /* first of 6 prefetches of next object's cache lines */
	PREFETCH(0,a2)

	/* Check for overflow */
	sltu	jp, a2, v0
	PREFETCH(32,a2)
	PREFETCH(64,a2)
	bne	jp, zero, GOUNLOCKANDSLOW
	/* Is a2 <= a3 (within range?) */
	sltu	jp, a3, a2
	PREFETCH(96,a2)
	bne	jp, zero, GOUNLOCKANDSLOW
	sw  	a2, (a0)    # and the new allocPtr is committed
#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	LA(a0, fastLockCount)
	lw	a3, (a0)
	addiu	a3, a3, 1
	sw	a3, (a0)
#endif
	/*
	 * Allocation done here
	 */
	sw	CB, (v0)	# Initialize cb
	li	a0, 2		# CVM_LOCKSTATE_UNLOCKED
	sw	a0, 4(v0)	# And initialize variousWord

	/*
	 * Now a3 is at the start of the object data
         * And a2 was allocNext which is also the end of the object
	 */
	addiu	a3, v0, 4
	PREFETCH(128,a2)
	addiu	a2, a2, -4
	PREFETCH(160,a2)
	b	LOOPTEST
INITLOOP:
	sw	zero, (a3)	# Next object field
LOOPTEST:
	slt	jp, a3, a2	# Done?
	addiu	a3, a3, 4
	bne	jp, zero, INITLOOP

	/*
	 * Unlock fast lock
	 * Store 0 into fastHeapLock
	 */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	/*
	 * v0 contains the resulting object
	 * return
	 */
	jr	ra

GOUNLOCKANDSLOW:
	/* Unlock by stuffing a zero in the lock */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
GOSLOW:
	/* Flush our state. */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	/* save CB */
	sw	CB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)

	/* Adjust ra? */
	sw	ra, OFFSET_CVMCompiledFrame_PC(JFP)

	move	a0, EE          # First argument ee  
	move	a1, CB          # a1 -- class
#ifdef CVM_CCM_COLLECT_STATS
        CALL_VM_FUNCTION(CVMgcAllocNewInstanceSpecial)
#else
	FIXUP_FRAMES_a0a1(JFP, jp)
	CALL_VM_FUNCTION(CVMgcAllocNewInstance)
#endif
	bne	v0, zero, RETURNOBJ
	/* cb */
	lw	a2, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	LA(a1, cbString)	# "%C"
	move	a0, EE          # ee
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	addiu	a0, sp, OFFSET_CStack_CCEE
	CALL_VM_FUNCTION(CVMJITexitNative)
RETURNOBJ:
	/* v0 contains the resulting object */
	lw	ra,  OFFSET_CVMCompiledFrame_PC(JFP)
	jr	ra

	SET_SIZE( CVMCCMruntimeNewGlue ) 

/*
 * Entry point for allocating an array of a basic type.
 */
ENTRY ( CVMCCMruntimeNewArrayGlue )
	/*
	 * Arguments:	
	 *	a0 = elementSize
	 *	a1 = dimension
	 *       a2 = arrCB
	 *
	 * TODO:	 Do a flag on class that means "big instance"
	 *		 So that we can check finalizability and big instance
	 *		 together.
	 */
#if 0
	#
	# If you just want to call the C helper and write very little
	# assembler code, then just the following.
	#
	FIXUP_FRAMES_a0a1a2ra(JFP, t7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeNewArray)
#endif

#define OBJ     v0
#define LEN     a1
#define ARRCB   a2
#define OBJSIZE t7
#define CVMGLOBALS t0

	LA(CVMGLOBALS, CVMglobals)

	/*
	 * Check if length is negative or too big. If it is, bail out
	 */
	lui	jp, 0x1000
	sltu	jp, LEN		/* LEN must less than 0x10000000 */
	bne	jp, zero, ARR_BADINDEX

	/*
	 * Now compute instance size of the array
	 * o0 holds element size
	 * LEN holds length
	 *
	 * OBJSIZE = roundup(elemsize * length + 12)
	 *
	 * which is equal to
	 *	
	 * (elemsize * length + 15) & ~3
	 */
	li	jp, ~0x3
	multu	LEN, a0			/* elemsize * length */
	mflo	OBJSIZE
	addi	OBJSIZE, OBJSIZE, 15
	and	OBJSIZE, OBJSIZE, jp

	/* lock using swp */
1:
	li	jp, 1
	ll	v0, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	sc	jp, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	beq	jp, zero, 1b		/* retry */
	bne	v0, zero, ARR_GOSLOW	/* already locked. Bail. */
	
	/*
	 * Allocate inline
	 */
	lw	v1, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lw	a3, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lw	OBJ, 0(v1)	/* OBJ <- allocPtr == function result */
	lw	a3, 0(a3)	/* o3 <- allocTop */

	addu	a0, OBJ, OBJSIZE

	/* first of 6 prefetches of next object's cache lines */
	PREFETCH(0,a0)

	/* Check for overflow */
	sltu	jp, a0, OBJ
	PREFETCH(32,a0)
	PREFETCH(64,a0)
	bne	jp, zero, ARR_GOUNLOCKANDSLOW
	/* within range? */
	sltu	jp, a3, a0
	PREFETCH(96,a0)
	bne	jp, zero, ARR_GOUNLOCKANDSLOW

	sw  	a0, (v1)	/* commit the new allocPtr */


	/* From now on, v1 can be trashed. */
#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	LA(jp, fastLockCount)
	lw	v1, (jp)
	addiu	v1, v1, 1
	sw	v1, (jp)
#endif

	/* Initialize the object header. */
	li	jp, 2		/* CVM_LOCKSTATE_UNLOCKED */
	sw	ARRCB, (OBJ)	/* cb is first field of object */
	sw	jp, 4(OBJ)	/* initialize variousWord */
	sw	LEN, 8(OBJ)	/* initialize array length */

#define	ALLOCNEXT	v1
	addu	ALLOCNEXT, OBJ, OBJSIZE
	PREFETCH(128,a0)
	addiu	a3, OBJ, 12
	PREFETCH(160,a0)
	b	ARR_LOOPTEST

ARR_INITLOOP:
	sw	zero, (a3)
	addiu	a3, a3, 4	/* Next field */
ARR_LOOPTEST:
	slt	jp, a3, ALLOCNEXT
	bne	jp, zero, ARR_INITLOOP

#undef	ALLOCNEXT
ARR_ENDINIT:	
	/* Unlock fast lock */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	/* return to compiled code. The object is in v0. */
	jr	ra

ARR_GOUNLOCKANDSLOW:
	/* Unlock by stuffing a zero in the lock */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)

ARR_GOSLOW:
	/* Flush our state. */
	/* save cb */
	sw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */

	/*
	 * Call CVMgcAllocNewInstance
	 *   a0 = arg1 = ee
	 *   a1 = arg2 = instance size
	 *   a2 = arg3 = arrayCB
	 *   a3 = arg4 = array length
	 */
	move	a3, LEN		/* arg4 = array length */
	move	a1, OBJSIZE	/* arg2 = instance size */
	FIXUP_FRAMES_a1a2a3ra(JFP, jp)
	move	a0, EE
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)

	/* return if successful */
        lw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	beq	v0, zero, ARR_ALLOC_FAIL
	jr	ra

ARR_ALLOC_FAIL:
	/* arg3 = cb */
	lw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
ARR_OUT_OF_MEMORY:	
	/* Out of memory. Throw exception and return to interpreter. */
	LA(a1, cbString)	/* arg2 = "%C" */
	move	a0, EE
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)

ARR_EXIT_NATIVE:
	addiu	a0, sp, OFFSET_CStack_CCEE
	CALL_VM_FUNCTION(CVMJITexitNative)

ARR_BADINDEX:
	/* flush state first */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */
	FIXUP_FRAMES_a1a2a3ra(JFP, jp)

	slt	jp, LEN, zero	/* check if array length < 0 */
	beq	jp, zero, ARR_OUT_OF_MEMORY

	/* The index is negative. Throw NegativeArraySizeException  */
	move	a0, EE		/* arg1 = EE */
	move	a1, zero	/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	b	ARR_EXIT_NATIVE

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef CVMGLOBALS
	SET_SIZE( CVMCCMruntimeNewArrayGlue ) 

/*
 * Entry point for allocating an array of the specified arrayCb.
 */
ENTRY ( CVMCCMruntimeANewArrayGlue )
	/*
	 * Arguments:	
	 *	a1 = dimension
	 *       a2 = arrayCb
	 *
	 * TODO:	 Do a flag on class that means "big instance"
	 *		 So that we can check finalizability and big instance
	 *		 together.
	 */
#if 0
	# If you just want to call the C helper and write very little
        # assembler code, then just do the following.
	#
	FIXUP_FRAMES_a1a2ra(JFP, t7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeANewArray)
#endif

#define OBJ     v0
#define LEN     a1
#define ARRCB	a2
#define OBJSIZE t7
#define	CVMGLOBALS t0

	LA(CVMGLOBALS, CVMglobals)

	/*
	 * Check if length is negative or too big. If it is, bail out
	 */
	lui	jp, 0x1000
	sltu	jp, LEN	/* must be less than 0x10000000 */
	bne	jp, zero, OBJARR_BADINDEX /* bail if negative len or too big */

	/*
	 * Now compute instance size of the array
	 * LEN holds length
	 *
	 * OBJSIZE = (LEN << 2 + 12)
	 *
	 */
	sll	OBJSIZE, LEN, 2
	addi	OBJSIZE, OBJSIZE, 12

	/* lock using swp */
1:
	li	jp, 1
	ll	v0, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	sc	jp, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	beq	jp, zero, 1b		/* retry */
	bne	v0, zero, OBJARR_GOSLOW	/* already locked. Bail. */

	/*
	 * Allocate inline
	 */
	lw	v1, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lw	a3, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lw	OBJ, (v1)	/* OBJ <- allocPtr == function result */
	lw	a3, (a3)	/* a3 <- allocTop */

	addu	a0, OBJ, OBJSIZE

	/* first of 6 prefetches of next object's cache lines */
	PREFETCH(0,a0)

	/* Check for overflow */
	sltu	jp, a0, OBJ
	PREFETCH(32,a0)
	PREFETCH(64,a0)
	/* within range? */
	sltu	jp, a3, a0
	PREFETCH(96,a0)
	bne	jp, zero, OBJARR_GOUNLOCKANDSLOW

	sw  	a0, (v1)	/* commit the new allocPtr */
   

#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	LA(jp, fastLockCount)
	lw	v1, (jp)
	addiu	v1, v1, 1
	sw	v1, (jp)
#endif

	/* Initialize the object header. */
	li	jp, 2		/* CVM_LOCKSTATE_UNLOCKED */
	sw	ARRCB, (OBJ)	/* cb is first field of object */
	sw	jp, 4(OBJ)	/* initialize variousWord */
	sw	LEN, 8(OBJ)	/* initialize array length */

#define ALLOCNEXT	v1
	addu	ALLOCNEXT, OBJ, OBJSIZE
	PREFETCH(128,a0)
	addiu	a3, OBJ, 12
	PREFETCH(160,a0)
	b	OBJARR_LOOPTEST

OBJARR_INITLOOP:
	sw	zero, (a3)
	addiu	a3, a3, 4	/* Next object field */
OBJARR_LOOPTEST:
	slt	jp, a3, ALLOCNEXT
	bne	jp, zero, OBJARR_INITLOOP
#undef ALLOCNEXT

OBJARR_ENDINIT:	
	/* Unlock fast lock */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	/* return to compiled code. The object is in v0. */
	jr	ra

OBJARR_GOUNLOCKANDSLOW:
	/* Unlock by stuffing a zero in the lock */
	sw	zero, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)

OBJARR_GOSLOW:
	/* Flush our state. */
	/* save cb */
	sw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
        sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        sw	ra, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */

	/*
	 * Call CVMgcAllocNewInstance
	 *   r3 = arg1 = ee
	 *   r4 = arg2 = instance size
	 *   r5 = arg3 = arrayCb (already in r5)
	 *   r6 = arg4 = array length
	 */
	move	a3, LEN		/* arg4 = array length */
	move	a1, OBJSIZE	/* arg2 = instance size */
	FIXUP_FRAMES_a1a2a3ra(JFP, jp)
	move	a0, EE
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)

	/* return if successful */
	lw	ra, OFFSET_CVMCompiledFrame_PC(JFP)
	beq	v0, zero, OBJARR_ALLOC_FAIL
	jr	ra

OBJARR_ALLOC_FAIL:
	/* setup some arguments for CVMthrowOutOfMemoryError */
	LA(a1, cbString)     /* arg2 = "%C" */
	/* arg3 = cb */
	lw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
OBJARR_OUT_OF_MEMORY:	
	/* Out of memory. Throw exception and return to interpreter. */
	move	a0, EE
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)

OBJARR_EXIT_NATIVE:
	addiu	a0, sp, OFFSET_CStack_CCEE
	CALL_VM_FUNCTION(CVMJITexitNative)

OBJARR_BADINDEX:
	/* flush state first */
	sw	JSP, OFFSET_CVMFrame_topOfStack(JFP)
	sw	JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
	sw	ra, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */
	FIXUP_FRAMES_a1a2a3ra(JFP, jp)

	slt	jp, LEN, zero  /* check if array length < 0 */
	LA(a1, cbStringArr)       /* arg2 = "[%C" */
	beq	jp, zero, OBJARR_OUT_OF_MEMORY

	/* The index is negative. Throw NegativeArraySizeException  */
	move	a0, EE		/* arg1 = EE */
	move	a1, zero	/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	b	OBJARR_EXIT_NATIVE

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef CVMGLOBALS
	SET_SIZE( CVMCCMruntimeANewArrayGlue ) 

/*
 * Allocate a multidimensional array. This is rare, so we just call 
 * the C helper.
 */
ENTRY ( CVMCCMruntimeMultiANewArrayGlue )
	/*
	 * Arguments:	
	 *	a1 = nDimensions
	 *	a2 = arrCb
	 *	a3 = address of dimension array
	 */
	FIXUP_FRAMES_a1a2a3ra(JFP, t7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeMultiANewArray)
	SET_SIZE ( CVMCCMruntimeMultiANewArrayGlue )

	.text
cbString:
	.asciiz "%C"
cbStringArr:
	.asciiz "[%C"
	.align 4
