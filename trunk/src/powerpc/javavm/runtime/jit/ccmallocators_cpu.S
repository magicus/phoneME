/*
 * @(#)ccmallocators_cpu.S	1.22 06/10/10
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

	.section	".text",""

/*
 * Macro for calling a helper. Sets up the ccee as the 1st
 * argument and flushes the pc to the frame and the frame to the 
 * interpreter stack.
 *
 * On entry, r7 should contain the address to return to.
 */
#define CALL_HELPER_AND_PASS_CCEE(HELPER)				\
	la	r3,  OFFSET_CStack_CCEE(sp) 				_SE_\
        stw	r7,  OFFSET_CVMCompiledFrame_PC(JFP) 			_SE_\
        stw	JSP, OFFSET_CVMFrame_topOfStack(JFP) 			_SE_\
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)  _SE_\
	BRANCH_TO_VM_FUNCTION(HELPER)

/*
 * Entry point for allocating an object. The cb is the only argument and
 * is in r4 rather than a1. This is so we don't need to move it to a1
 * when it gets passed to CVMgcAllocNewInstance.
 */
ENTRY ( CVMCCMruntimeNewGlue )
	#
	# Arguments:	
	#	r4 = 'cb'
	#
	# TODO:		 Do a flag on class that means "big instance"
	#		 So that we can check finalizability and big instance
	#		 together.
	#
	# Register usage:	
	#	r3  = OBJ - the object we allocate and return.
	#	r4  = CB - the cb passed in.
	#	r10 = OBJSIZE - size of an instance of CB
	#	r11 = FHL - reserved for the fastHeapLock
	#

#define OBJ    r3
#define CB     r4
#define OBJSIZE r10
#define FHL    r11

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 3 lines are needed.
	#
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeNew)
#endif

	lhz	r0, OFFSET_CVMClassBlock_accessFlagsX(CB)
	la	FHL, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	andi.	r0, r0, CONSTANT_CLASS_ACC_FINALIZABLE  /* finalizable? */
	li      r0, 1	       /* 1 == locked flag for fastHeapLock */
	bne-	GOSLOW         /* go slow route if finalizable */

	# lock using swp
0:		
	lwarx	r8, 0, FHL	/* r8 = old value */
	stwcx.	r0, 0, FHL	/* store locked flag if reservation held */
	bne-	0b		/* if reservation lost, try again */
	/*isync*/		/* be MP safe */
	cmpi	cr1, r8, 1      /* check if already locked. */
	beq-	cr1, GOSLOW	/* already locked. Bail. */

	#
	# Allocate inline
	#
	lwz	r6, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lwz	r7, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lwz	OBJ, 0(r6)	/* OBJ <- allocPtr == function result */
	lhz	OBJSIZE, OFFSET_CVMClassBlock_instanceSizeX(CB)
	lwz	r7, 0(r7)	/* r7 <- allocTop */
	addco.	r9, OBJ, OBJSIZE /* r9 <- allocNext (allocPtr + size) */
	# Check for overflow
	bso-	GOUNLOCKANDSLOW	/* branch if we went passed top of memory */
	cmpl	cr1, r9, r7	/* Is r9 <= allocTop */
	bgt-	cr1, GOUNLOCKANDSLOW
	stw  	r9, 0(r6)	/* commit the new allocPtr */

#ifdef CVM_FASTALLOC_STATS
	# Count fast locks
	lis	r0, HA16(fastLockCount)
	la	r0, LO16(fastLockCount)(r0) /* r0 <- fastLockCount */
	lwz	r12, 0(r0)
	addi	r12, r12, 1
	stw	r12, 0(r0)
#endif

	# Initialize the object header.
	li	r8, 2		/* CVM_LOCKSTATE_UNLOCKED */
	stw	CB, 0(OBJ)	/* cb is first field of object */
	stw	r8, 4(OBJ)	/* initialize variousWord */

	# Setup loop to zero all fields. r7 starts 4 bytes before the first
	# field because powerpc only supports pre-increment
	subi	OBJSIZE, OBJSIZE, 8 /* remove object header size */
	li      r0, 0		/* used to clear field and fastHeapLock */
	srwi.	OBJSIZE, OBJSIZE, 2 /* convert size to # of words */
	addi	r7, OBJ, 4	/* r7 <- first fieldPtr - 4 */
	mtctr	OBJSIZE		/* ctr <- # of words */

	beq-	INITDONE	/* no fields to clear */

INITLOOP:
	stwu	r0, 4(r7)	/* Next object field */
	bdnz+	INITLOOP	/* decrement ctr and branch if ctr != 0 */
INITDONE:	
	# Unlock fast lock
	# r0 is already 0 here. Store it into fastHeapLock
	/*sync*/		/* be MP safe */
	stw	r0, 0(FHL)	/* store 0 into fastHeapLock */
	# return to compiled code. The object is in r3.
	blr

GOUNLOCKANDSLOW:
	# Unlock by stuffing a zero in the lock
	li      r0, 0	   /* 0 == unlocked flag */
	stw	r0, 0(FHL) /* store 0 into fastHeapLock */

GOSLOW:
	# Flush our state.
	/* save cb */
	stw	CB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mflr	r7
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */

	# Call CVMgcAllocNewInstance
	mr	r3, EE          /* arg1 = EE */
	# r3 = arg1 = ee
	# r4 = arg2 = cb
#ifdef CVM_CCM_COLLECT_STATS
        CALL_VM_FUNCTION(CVMgcAllocNewInstanceSpecial)
#else
	FIXUP_FRAMES(JFP, r7)
	CALL_VM_FUNCTION(CVMgcAllocNewInstance)
#endif

	# return if successful
        lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	mtlr	r7
	cmpwi	r3, 0
	bnelr+		/* return if successful */

	# Out of memory. Throw exception and return to interpreter.
	mr	r3, EE			/* arg1 = EE */
	lis	r4, HA16(cbString)
	la	r4, LO16(cbString)(r4)	/* arg2 = "%C" */
	/* arg3 = cb */
	lwz	r5, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	/* arg1 = CCEE */
	la	r3, OFFSET_CStack_CCEE(sp)
	lis	r7, HA16(CVMJITexitNative)
	la	r7, LO16(CVMJITexitNative)(r7)
	mtlr	r7
	blr		/* call CVMJITexitNative */
#undef OBJ
#undef CB
#undef OBJSIZE
#undef FHL

SET_SIZE( CVMCCMruntimeNewGlue ) 

/*
 * Entry point for allocating an array of a basic type.
 */
ENTRY ( CVMCCMruntimeNewArrayGlue )
	#
	# Arguments:	
	#	r3 = elementSize
	#	r4 = dimension (length)
	#       r5 = arrCB
	#
	# Register usage:	
	#	r3  = OBJ - the array we allocate and return.
	#	r4  = LEN - the length passed in.
	#	r5  = ARRCB - the array cb passed in.
	#	r10 = OBJSIZE - size of the allocated array
	#	r11 = FHL - reserved for the fastHeapLock
	#

#define OBJ     r3
#define LEN     r4
#define ARRCB   r5
#define OBJSIZE r10
#define FHL     r11

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 3 lines are needed.
	#
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeNewArray)
#endif

	la	FHL, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	li      r0, 1	       /* 1 == locked flag for fastHeapLock */

	#
	# Check if length is negative or too big. If it is, bail out
	#
	andis.	r12, LEN, 0xf000 /* must be less than 0x10000000 */
	bne-	ARR_BADINDEX	 /* bail if negative length or too big */

	# Now compute instance size of the array
	# r3 holds element size
	# LEN holds length
	#
	# OBJSIZE = roundup(elemsize * length + 12)
	#
	# which is equal to
	#
	# (elemsize * length + 15) & ~3
	#
	mullw	OBJSIZE, r3, LEN
	addi	OBJSIZE, OBJSIZE, 15
	clrrwi	OBJSIZE, OBJSIZE, 2	/* clear rightmost 2 bits */

	# lock using swp
0:		
	lwarx	r8, 0, FHL	/* r8 = old value */
	stwcx.	r0, 0, FHL	/* store locked flag if reservation held */
	bne-	0b		/* if reservation lost, try again */
	/*isync*/		/* be MP safe */
	cmpi	cr1, r8, 1      /* check if already locked. */
	beq-	cr1, ARR_GOSLOW	/* already locked. Bail. */

	#
	# Allocate inline
	#
	lwz	r6, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lwz	r7, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lwz	OBJ, 0(r6)	/* OBJ <- allocPtr == function result */
	lwz	r7, 0(r7)	/* r7 <- allocTop */
	addco.	r9, OBJ, OBJSIZE /* r9 <- allocNext (allocPtr + size) */
	# Check for overflow
	bso-	ARR_GOUNLOCKANDSLOW /* branch if we passed top of memory */
	cmpl	cr1, r9, r7	/* Is r9 <= allocTop */
	bgt-	cr1, ARR_GOUNLOCKANDSLOW
	stw  	r9, 0(r6)	/* commit the new allocPtr */

#ifdef CVM_FASTALLOC_STATS
	# Count fast locks
	lis	r0, HA16(fastLockCount)
	la	r0, LO16(fastLockCount)(r0) /* r0 <- fastLockCount */
	lwz	r12, 0(r0)
	addi	r12, r12, 1
	stw	r12, 0(r0)
#endif

	# Initialize the object header.
	li	r8, 2		/* CVM_LOCKSTATE_UNLOCKED */
	stw	ARRCB, 0(OBJ)	/* cb is first field of object */
	stw	r8, 4(OBJ)	/* initialize variousWord */
	stw	LEN, 8(OBJ)	/* initialize array length */

	# Setup loop to zero all fields. r7 starts 4 bytes before the first
	# field because powerpc only supports pre-increment
	subi	OBJSIZE, OBJSIZE, 12 /* remove object header size */
	li      r0, 0		/* used to clear field and fastHeapLock */
	srwi.	OBJSIZE, OBJSIZE, 2 /* convert size to # of words */
	addi	r7, OBJ, 8	/* r7 <- first elementPtr - 4 */
	mtctr	OBJSIZE		/* ctr <- # of words */

	beq-	ARR_ENDINIT	/* no fields to clear */

ARR_INITLOOP:
	stwu	r0, 4(r7)	/* Next object field */
	bdnz+	ARR_INITLOOP	/* decrement ctr and branch if ctr != 0 */
ARR_ENDINIT:	
	# Unlock fast lock
	# r0 is already 0 here. Store it into fastHeapLock
	/*sync*/		/* be MP safe */
	stw	r0, 0(FHL)	/* store 0 into fastHeapLock */
	# return to compiled code. The object is in r3.
	blr

ARR_GOUNLOCKANDSLOW:
	# Unlock by stuffing a zero in the lock
	li      r0, 0	   /* 0 == unlocked flag */
	stw	r0, 0(FHL) /* store 0 into fastHeapLock */

ARR_GOSLOW:
	# Flush our state.
	/* save cb */
	stw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mflr	r7
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */

	# Call CVMgcAllocNewInstance
	#   r3 = arg1 = ee
	#   r4 = arg2 = instance size
	#   r5 = arg3 = arrayCB
	#   r6 = arg4 = array length
	mr	r3, EE          /* arg1 = EE */
	mr	r6, LEN		/* arg4 = array length (currently in r4) */
	mr	r4, OBJSIZE	/* arg2 = instance size */
	FIXUP_FRAMES(JFP, r7)
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)

	# return if successful
        lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	cmpwi	r3, 0
	mtlr	r7
	bnelr+		/* return if successful */

	/* arg3 = cb */
	lwz	r5, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
ARR_OUT_OF_MEMORY:	
	# Out of memory. Throw exception and return to interpreter.
	mr	r3, EE			/* arg1 = EE */
	lis	r4, HA16(cbString)
	la	r4, LO16(cbString)(r4)	/* arg2 = "%C" */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)

ARR_EXIT_NATIVE:	
	la	r3, OFFSET_CStack_CCEE(sp)	/* arg1 = CCEE */
	lis	r7, HA16(CVMJITexitNative)
	la	r7, LO16(CVMJITexitNative)(r7)
	mtlr	r7
	blr		/* call CVMJITexitNative */

ARR_BADINDEX:
	# flush state first
	mflr	r7
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */
	FIXUP_FRAMES(JFP, r7)

	cmpwi	LEN, 0	/* check if array length < 0 */
	bge	ARR_OUT_OF_MEMORY /* array too big */
	# The index is negative. Throw NegativeArraySizeException 
	mr	r3, EE		/* arg1 = EE */
	li	r4, 0		/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	b	ARR_EXIT_NATIVE

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef FHL

SET_SIZE( CVMCCMruntimeNewArrayGlue ) 

/*
 * Entry point for allocating an array of a basic type.
 */
ENTRY ( CVMCCMruntimeANewArrayGlue )
	#
	# Arguments:	
	#	r4 = dimension
	#       r5 = arrayCB
	#
	# Register usage:	
	#	r3  = OBJ - the array we allocate and return.
	#	r4  = LEN - the length in.
	#	r5  = ARRCB - the cb for an array of elementCb.
	#	r10 = OBJSIZE - size of the allocated array
	#	r11 = FHL - reserved for the fastHeapLock
	#

#define OBJ     r3
#define LEN     r4
#define ARRCB	r5
#define OBJSIZE r10
#define FHL     r11

#if 0
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just the following 3 lines are needed.
	#
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeANewArray)
#endif

	#
	# Check if length is negative or too big. If it is, bail out
	#
	andis.	r0, LEN, 0xf000	/* must be less than 0x10000000 */
	bne-	OBJARR_BADINDEX	/* bail if negative length or too big */

	# Now compute instance size of the array
	# LEN holds length
	#
	# OBJSIZE = LEN << 2 + 12)
	#
	#
	slwi	OBJSIZE, LEN, 2
	addi	OBJSIZE, OBJSIZE, 12

	# lock using swp
	la	FHL, OFFSET_CVMGlobalState_fastHeapLock(CVMGLOBALS)
	li      r0, 1		/* 1 == locked flag for fastHeapLock */
0:		
	lwarx	r8, 0, FHL	/* r8 = old value */
	stwcx.	r0, 0, FHL	/* store locked flag if reservation held */
	bne-	0b		/* if reservation lost, try again */
	/*isync*/		/* be MP safe */
	cmpi	cr1, r8, 1      /* check if already locked. */
	beq-	cr1, OBJARR_GOSLOW	/* already locked. Bail. */

	#
	# Allocate inline
	#
	lwz	r6, OFFSET_CVMGlobalState_allocPtrPtr(CVMGLOBALS)
	lwz	r7, OFFSET_CVMGlobalState_allocTopPtr(CVMGLOBALS)
	lwz	OBJ, 0(r6)	/* OBJ <- allocPtr == function result */
	lwz	r7, 0(r7)	/* r7 <- allocTop */
	addco.	r9, OBJ, OBJSIZE /* r9 <- allocNext (allocPtr + size) */
	# Check for overflow
	bso-	OBJARR_GOUNLOCKANDSLOW /* branch if we passed top of memory */
	cmpl	cr1, r9, r7	/* Is r9 <= allocTop */
	bgt-	cr1, OBJARR_GOUNLOCKANDSLOW
	stw  	r9, 0(r6)	/* commit the new allocPtr */

#ifdef CVM_FASTALLOC_STATS
	# Count fast locks
	lis	r0, HA16(fastLockCount)
	la	r0, LO16(fastLockCount)(r0) /* r0 <- fastLockCount */
	lwz	r12, 0(r0)
	addi	r12, r12, 1
	stw	r12, 0(r0)
#endif

	# Initialize the object header.
	li	r8, 2		/* CVM_LOCKSTATE_UNLOCKED */
	stw	ARRCB, 0(OBJ)	/* cb is first field of object */
	stw	r8, 4(OBJ)	/* initialize variousWord */
	stw	LEN, 8(OBJ)	/* initialize array length */

	# Setup loop to zero all fields. r7 starts 4 bytes before the first
	# field because powerpc only supports pre-increment
	subi	OBJSIZE, OBJSIZE, 12 /* remove object header size */
	li      r0, 0		/* used to clear field and fastHeapLock */
	srwi.	OBJSIZE, OBJSIZE, 2 /* convert size to # of words */
	addi	r7, OBJ, 8	/* r7 <- first elementPtr - 4 */
	mtctr	OBJSIZE		/* ctr <- # of words */

	beq-	OBJARR_ENDINIT	/* no fields to clear */

OBJARR_INITLOOP:
	stwu	r0, 4(r7)	/* Next object field */
	bdnz+	OBJARR_INITLOOP	/* decrement ctr and branch if ctr != 0 */
OBJARR_ENDINIT:	
	# Unlock fast lock
	# r0 is already 0 here. Store it into fastHeapLock
	/*sync*/		/* be MP safe */
	stw	r0, 0(FHL)	/* store 0 into fastHeapLock */
	# return to compiled code. The object is in r3.
	blr		/* return */

OBJARR_GOUNLOCKANDSLOW:
	# Unlock by stuffing a zero in the lock
	li      r0, 0	   /* 0 == unlocked flag */
	stw	r0, 0(FHL) /* store 0 into fastHeapLock */

OBJARR_GOSLOW:
	# Flush our state.
	/* save cb - FIXUP_FRAMES uses the first 3 slots. Be safe and use
	 * the 6th */
	stw	ARRCB, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)
	mflr	r7
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */

	# Call CVMgcAllocNewInstance
	#   r3 = arg1 = ee
	#   r4 = arg2 = instance size
	#   r5 = arg3 = arrayCb (already in r5)
	#   r6 = arg4 = array length
	mr	r3, EE          /* arg1 = EE */
	mr	r6, LEN		/* arg4 = array length (currently in r4) */
	mr	r4, OBJSIZE	/* arg2 = instance size */
	FIXUP_FRAMES(JFP, r7)
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)

	# return if successful
        lwz	r7, OFFSET_CVMCompiledFrame_PC(JFP)
	cmpwi	r3, 0
	mtlr	r7
	bnelr+		/* return if successful */

	# setup some arguments for CVMthrowOutOfMemoryError
	lis	r4, HA16(cbString)
	la	r4, LO16(cbString)(r4)	/* arg2 = "%C" */
	/* arg3 = cb */
	lwz	r5, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage(sp)

OBJARR_OUT_OF_MEMORY:	
	# Out of memory. Throw exception and return to interpreter.
	mr	r3, EE			/* arg1 = EE */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)

OBJARR_EXIT_NATIVE:	
	la	r3, OFFSET_CStack_CCEE(sp)	/* arg1 = CCEE */
	lis	r7, HA16(CVMJITexitNative)
	la	r7, LO16(CVMJITexitNative)(r7)
	mtlr	r7
	blr		/* call CVMJITexitNative */

OBJARR_BADINDEX:
	# flush state first
	mflr	r7
	stw     JSP, OFFSET_CVMFrame_topOfStack(JFP)
        stw     JFP, OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame(EE)
        stw	r7, OFFSET_CVMCompiledFrame_PC(JFP) /* save return PC */
	FIXUP_FRAMES(JFP, r7)

	cmpwi	r4, 0	/* check if array length < 0 */
	lis	r4, HA16(cbStringArr)
	la	r4, LO16(cbStringArr)(r4)	/* arg2 = "[%C" */
	bge	OBJARR_OUT_OF_MEMORY /* array too big */
	# The index is negative. Throw NegativeArraySizeException 
	mr	r3, EE		/* arg1 = EE */
	li	r4, 0		/* arg2 = NULL */
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	b	OBJARR_EXIT_NATIVE

#undef OBJ
#undef LEN
#undef ARRCB
#undef OBJSIZE
#undef FHL

SET_SIZE( CVMCCMruntimeANewArrayGlue ) 

/*
 * Allocate a multidimensional array.
 */
ENTRY ( CVMCCMruntimeMultiANewArrayGlue )
	#
	# Arguments:	
	#	r4 = nDimensions
	#	r5 = arrCb
	#	r6 = address of dimension array
	#
	# Flush our state.
	#
	mflr	r7
	FIXUP_FRAMES(JFP,r7)
	CALL_HELPER_AND_PASS_CCEE(CVMCCMruntimeMultiANewArray)

SET_SIZE( CVMCCMruntimeMultiANewArrayGlue )


SYM_NAME(cbString):
	.asciz "%C"
SYM_NAME(cbStringArr):
	.asciz "[%C"
