/*
 * %W% %E%
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

/***********************************
 * Java heap allocators
 ***********************************/

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s2_ccmallocators_cpu)

/*
 * Entry point for allocating an object. The cb is the only argument and
 * is in a2 rather than a1. This is so we don't need to move it to a1
 * when it gets passed to CVMgcAllocNewInstance.
 */
	ENTRY(CVMCCMruntimeNewGlue)
ENTRY1 ( CVMCCMruntimeNewGlue )

	/* Arguments:	  */
	/*	a2 = 'cb' */
#ifdef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
        /*	a3 = 'instance size'	 IAI-03 */
        /*	a4 = 'accessflag'	 IAI-03 */
#endif
	/* Also incoming:
         *       v1 = jfp
         *       v2 = jsp
	 *       sp = ccee
	 */

	/* POSSIBLE OPTIMIZATION:
	 *		 Do a flag on class that means "big instance"
	 *		 So that we can check finalizability and big instance
	 *		 together. 
	 */

	/* Registers used and must be saved 
	 *   v1 - return value (already saved!) 
	 *   v2 - cb (already saved!) 
	 */

	/* Flush our state. JSP and JFP have to be saved anyway */

#define LOCK ip
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]     /* a1 -- ee */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack] /* v1 saved */
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] /* return PC saved */
        str     JFP, [a1,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]	/* v2 saved */
	mov	v2, a2	       /* cb */
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        textrmuw a2, W_CVMGLOBALS, #0
#else
	IMPORT(CVMglobals)
	ldr	a2, SYMBOL(CVMglobals)
#endif
        add	LOCK, a2, #OFFSET_CVMGlobalState_fastHeapLock
        mov     a1, #1	       /* locked	 */
        ldr	v1, [LOCK]
/* IAI-03 */
#ifndef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
	/* Get access flags from class */
	ldrh	a4, [v2, #OFFSET_CVMClassBlock_accessFlagsX]  /* flags */
#endif
	ands	v1, a4, #CONSTANT_CLASS_ACC_FINALIZABLE     /* finalizable? */
	bne	GOSLOW         /* bail if yes. Otherwise v1 == 0 */
	
#ifndef CVM_MP_SAFE
	/* lock using swp */
        swp	a1, a1, [LOCK]   /* try locking */
	cmp	a1, #1         /* already locked. Bail. */
	beq	GOSLOW
#else
LABEL(_newAcquireLockMPSafe)
	ldrex	a4, [LOCK]
	cmp	a4, #1     /* already locked? */
	beq	GOSLOW     /* go slow if already locked */
	strex	a4, a1, [LOCK] /* try acquire the lock */
	cmp	a4, #0	       /* strex succeed? */
	bne	_newAcquireLockMPSafe  /* No. Try again */
	mcr	p15, 0, a4, c7, c10, 5 /* memory barrier. a4 has 0. */
#endif

	/* Allocate inline
	 * a2 = &CVMglobals
	 * v2 = cb 
	 */

	ldr	a1, [a2, #OFFSET_CVMGlobalState_allocPtrPtr]
	ldr	a4, [a2, #OFFSET_CVMGlobalState_allocTopPtr]
	ldr	v1, [a1, #0]   /* v1 <- allocPtr   */
	ldr	a4, [a4, #0]   /* a4 <- allocTop */
/* IAI-03 */
#ifndef IAI_NEW_GLUE_CALLING_CONVENTION_IMPROVEMENT
	/* get instance size from class */
	ldrh	a3, [v2, #OFFSET_CVMClassBlock_instanceSizeX]  /* instance size*/
#endif
	adds a3, v1, a3     /* a3 <- allocNext (allocPtr v1 + size a3) */

/* IAI - 19*/
#ifdef IAI_PRELOAD_NEW_OBJECT	
       /* preload the memory to be allocated for the next object 
        * in the next NewGlue call 
        */
	pld  [a3]
	pld  [a3, #32]
	pld  [a3, #64]

	pld  [a3, #96]	
	pld  [a3, #128]	
	pld  [a3, #160]	

	pld  [v2]
#endif  /* IAI_PRELOAD_NEW_OBJECT */   

	/* Check for overflow */
	bvs	GOUNLOCKANDSLOW
	cmp	a3, a4         /* Is a3 <= a4 (within range?) */
	bhi	GOUNLOCKANDSLOW
	str  	a3, [a1]    /* and the new allocPtr is committed */
#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	ldr	a1, =fastLockCount
	ldr	a4, [a1]
	add	a4, a4, #1
	str	a4, [a1]
#endif

	/* Allocation done here */

	mov     a4, v1         /* a4 -- iteration variable */
	str	v2, [a4], #4   /* Initialize cb */
	mov	a1, #2         /* CVM_LOCKSTATE_UNLOCKED */
	str	a1, [a4], #4   /* And initialize variousWord */
	/*
	 * Now a4 is at the start of the object's data
         * And a3 was allocNext which is also the end of the object
	 * Also initialize a1 to be the heap lock again
	 */
	mov	a1, #0         /* Initialize to 0 */
	b	LOOPTEST
LABEL(INITLOOP)
	str	a1, [a4], #4   /* Next object field */
LABEL(LOOPTEST)
	cmp	a4, a3         /* Done? */
	bcc	INITLOOP
LABEL(ENDINIT)
	/* Unlock fast lock */
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. a1 has 0*/
#endif
	/* a1 is already 0 here. Store it into fastHeapLock */
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
	
	mov	a1, v1         /* Return object */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* Restore JSP/JFP and return to compiled code */
	ldr	a3,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	/* a1 contains the resulting object */
	/* a3 contains the return address */
	/* jump */
	mov     pc, a3
LABEL(GOUNLOCKANDSLOW)
	/* Unlock by stuffing a zero in the lock */
	mov	a1, #0
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. */
#endif	
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
LABEL(GOSLOW)
	/* Start taking stuff out of the ccee */
	ldr	v1, [sp, #OFFSET_CVMCCExecEnv_ee]	/* v1 -- ee */
	mov	a2, v2          /* a2 -- class */
#ifndef CVM_CCM_COLLECT_STATS
        ldr     lr, [v1,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
#endif
	mov	a1, v1          /* First argument ee   */
#ifdef CVM_CCM_COLLECT_STATS
        CALL_VM_FUNCTION(CVMgcAllocNewInstanceSpecial)
#else
#define SAVESET {a1, a2}
	FIXUP_FRAMES(lr, SAVESET, 2)
#undef SAVESET
	CALL_VM_FUNCTION(CVMgcAllocNewInstance)
#endif
	cmp	a1, #0
	bne	RETURNOBJ
	mov	a1, v1          /* ee */
	adr	a2, cbString    /* "%C" */
	mov	a3, v2          /* cb */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	mov	a1, sp
	IMPORT(CVMJITexitNative)
	ldr     pc, SYMBOL(CVMJITexitNative)
LABEL(RETURNOBJ)
	/* Restore JSP/JFP and return to compiled code */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	/* a1 contains the resulting object */
	ldr     pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]

#undef LOCK
	SET_SIZE( CVMCCMruntimeNewGlue ) 

/*
 * Entry point for allocating an array of a basic type.
 */
	ENTRY(CVMCCMruntimeNewArrayGlue)
ENTRY1 ( CVMCCMruntimeNewArrayGlue )

	/* Arguments:	 
 	 *	a1 = r0 = elementSize 
	 *	a2 = r1 = dimension 
	 *       a3 = r2 = arrCB 
	 *
	 * Also incoming: 
         *       v1 = jfp  
         *       v2 = jsp 
	 *       sp = ccee 
	 *
	 * POSSIBLE OPTIMIZATION:		  
	 *		 Do a flag on class that means "big instance" 
	 *		 So that we can check finalizability and big instance 
	 *		 together. 
	 *
	 * Registers used and must be saved 
	 *   v1 - return value (already saved!) 
	 *   v2 - cb (already saved!)
	 *   v3 - array length
	 */

	/* Flush our state. JSP and JFP have to be saved anyway */

	ldr	a4,  [sp,  #OFFSET_CVMCCExecEnv_ee]     /* a4 -- ee */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack] /* v2 saved */
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] /* return PC saved */
        str     JFP, [a4,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] /* v2 saved */
	str	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage] /* v3 saved */

	/* Keep cb in v2 for later */
	/* Also, keep array length in v3 for later */

	mov	v3, a2         /* array length */
	mov	v2, a3	       /* cb */

	/* Check if length is negative. If it is, bail out */

/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw a2, W_CVMGLOBALS, #0
#else
	ldr	a2, SYMBOL(CVMglobals)
#endif
	cmp	v3, #(1 << 28)
	add	a4, a2, #OFFSET_CVMGlobalState_fastHeapLock
	bhi	ARR_BADINDEX	/* bail if too big or negative */
	ldr	v1, [a4]

	/* Now compute instance size of the array */
	/* a1 holds element size */
	/* a2 holds length */

	/* len = roundup(elemsize * length + 12) */

	/* which is equal to */

	/* (elemsize * length + 15) & ~3 */

	mul	a3, a1, v3
	add	a3, a3, #15
	bic	a3, a3, #3
	/* a4 = instance size */
	/* lock using atomic operation */
	mov     a1, #1	       /* locked	 */
#ifndef CVM_MP_SAFE
	swp	a1, a1, [a4]   /* try locking */
	cmp	a1, #1         /* already locked. Bail. */
	beq	ARR_GOSLOW
#else
LABEL(_arrAcquireLockMPSafe)
	ldrex	v1, [a4]
	cmp	v1, #1       /* already locked? */
	beq	ARR_GOSLOW   /* already locked. Bail. */
	strex	v1, a1, [a4] /* try lock */
	cmp	v1, #0       /* strex succeed? */
	bne	_arrAcquireLockMPSafe
	mcr	p15, 0, v1, c7, c10, 5 /* memory barrier. v1 has 0 */
#endif

	/* Allocate inline */
	/* a2 = &CVMglobals */
	/* v2 = cb */

	ldr	a1, [a2, #OFFSET_CVMGlobalState_allocPtrPtr]
	ldr	a4, [a2, #OFFSET_CVMGlobalState_allocTopPtr]
	ldr	v1, [a1, #0]   /* v1 <- allocPtr   */
	ldr	a4, [a4, #0]   /* a1 <- allocTop */
	/* instance size is in a3 */
	adds	ip, v1, a3     /* ip <- allocNext (allocPtr v1 + size a3) */
	/* Check for overflow */
	bvs	ARR_GOUNLOCKANDSLOW
	cmp	ip, a4         /* Is ip <= a4 (within range?) */
	bhi	ARR_GOUNLOCKANDSLOW
	str  	ip, [a1]    /* and the new allocPtr is committed */

	/* v1 is the allocation point now */

/* IAI - 19*/
#ifdef IAI_PRELOAD_NEW_OBJECT	
       /* preload the memory to be allocated for the next object 
        * in the next NewGlue call
        */
	cmp a3, #64

	/* current allocated array is small size, 
	 * so the memory to be allocated is already in cache by
         * preload in last NewGlue call
	 */
	blt SMALL_ARRAY_PLD     

	/* large size array allocation, 
	 *  the memory beyond 64 bytes is not in the cache, so preload it
	 */
	pld [v1, #96]          
	pld [v1, #128]
LABEL(SMALL_ARRAY_PLD)       
	pld  [ip]	
	pld  [ip, #32]
	pld  [ip, #64]

 	pld  [ip, #96]	
	pld  [ip, #128]	
 	pld  [ip, #160]	

#if 0
	/* TODO: The code above appears to have the wrong logic. I believe
	 * the code below is correct, but I was not able to test it
	 * on a platform with pld. Note that the two pld's before the
	 * SMALL_ARRAY_PLD label should also be removed. This same change
	 * should also be applied in CVMCCMruntimeANewArrayGlue. What
         * would probably be better than the above logic would be to just
         * do a good job of scheduling all 6 pld instructions in delay
         * slots between other instructions.
	 */
	pld  [ip]	
	pld  [ip, #32]
	pld  [ip, #64]
	pld  [ip, #96]	
LABEL(SMALL_ARRAY_PLD)       
	pld  [ip, #128]	
	pld  [ip, #160]
#endif
#endif	/* IAI_PRELOAD_NEW_OBJECT */

#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	ldr	a1, SYMBOL(fastLockCount)
	ldr	a4, [a1]
	add	a4, a4, #1
	str	a4, [a1]
#endif

	/* Allocation done here */

	mov     a4, v1         /* a4 -- iteration variable */
	str	v2, [a4], #4   /* Initialize cb */
	mov	a1, #2         /* CVM_LOCKSTATE_UNLOCKED */
	str	a1, [a4], #4   /* Initialize variousWord */
	str	v3, [a4], #4   /* And finally, initialize array length */
	/*
	 * Now a4 is at the start of the object's data
         * And ip is allocNext which is also the end of the object
	 */
	mov	a1, #0         /* Initialize to 0 */
	b	ARR_LOOPTEST
LABEL(ARR_INITLOOP)
	str	a1, [a4], #4   /* Next object field */
LABEL(ARR_LOOPTEST)
	cmp	a4, ip         /* Done? */
	bcc	ARR_INITLOOP
LABEL(ARR_ENDINIT)
	/* Unlock fast lock */
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. a1 has 0 */
#endif
	/* a1 is already 0 here. Store it into fastHeapLock */
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
	mov	a1, v1         /* Return object */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* Restore JSP/JFP and return to compiled code */
	ldr	a3,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	ldr	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage]
	/* a1 contains the resulting object */
	/* a3 contains the return address */
	/* jump */
	mov     pc, a3
LABEL(ARR_GOUNLOCKANDSLOW)
	/* Unlock by stuffing a zero in the lock */
	mov	a1, #0
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. */
#endif
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
LABEL(ARR_GOSLOW)
	/* At this point, a3 holds the instance length */
	ldr	a1, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a1 -- ee */
	mov	a2, a3		/* a2 -- instance length */
        ldr     lr, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	mov	a3, v2          /* a3 -- class */
	mov	a4, v3          /* a4 -- array length */
#define SAVESET {a1, a2, a3}
	FIXUP_FRAMES(lr, SAVESET, 3)
#undef SAVESET
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)
	cmp	a1, #0
	bne	ARR_RETURNOBJ
LABEL(ARR_OUT_OF_MEMORY)
	ldr	a1, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a1 -- ee */
LABEL(ARR_OUT_OF_MEMORY1)
	adr	a2, cbString    /* "%C" */
	mov	a3, v2          /* cb */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	mov	a1, sp
	ldr     pc, SYMBOL(CVMJITexitNative)
LABEL(ARR_BADINDEX)
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a1 -- ee */
        ldr     lr, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
#define SAVESET {a1}
	FIXUP_FRAMES(lr, SAVESET, 1)
#undef SAVESET
	cmp	v3, #0
	blt	ARR_NEGATIVEINDEX
	/* If we are here, the array was too big to even try to allocate */
	/* Just throw out of memory error */
	b	ARR_OUT_OF_MEMORY1
LABEL(ARR_NEGATIVEINDEX)
	mov	a2, #0
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	mov	a1, sp
	ldr	pc, SYMBOL(CVMJITexitNative)
LABEL(ARR_RETURNOBJ)
	/* Restore JSP/JFP and return to compiled code */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	ldr	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage]
	/* a1 contains the resulting object */
	ldr     pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]

	SET_SIZE( CVMCCMruntimeNewArrayGlue ) 

/*
 * Entry point for allocating an array of the specified arrayCb.
 */
	ENTRY(CVMCCMruntimeANewArrayGlue)
ENTRY1 ( CVMCCMruntimeANewArrayGlue )

	/* Arguments:
	 *	a2 = r1 = dimension 
	 *       a3 = r2 = arrayCb 
	 *
 	 * Also incoming: 
         *       v1 = jfp  
         *       v2 = jsp 
	 *       sp = ccee 
	 *
	 * POSSIBLE OPTIMIZATION:		  
	 *		 Do a flag on class that means "big instance" 
	 *		 So that we can check finalizability and big instance 
	 *		 together. 
	 *
	 * Registers used and must be saved 
	 *   v1 - return value (already saved!) 
	 *   v2 - cb (already saved!) 
	 *   v3 - array length 
	 */

	/* Flush our state. JSP and JFP have to be saved anyway */

	ldr	a4,  [sp,  #OFFSET_CVMCCExecEnv_ee]     /* a4 -- ee */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack] /* v1 saved */
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] /* return PC saved */
        str     JFP, [a4,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]  /* v2 saved */
	str	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage] /* v3 saved */

	/* Keep array length in v3 for later */

	mov	v3, a2         /* array length */
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
	textrmuw a2, W_CVMGLOBALS, #0
#else
	ldr	a2, SYMBOL(CVMglobals)
#endif

	/* Check if length is negative or too big. If it is, bail out */

	cmp	v3, #(1 << 28)
	bhi	OBJARR_BADINDEX     /* bail if negative length or too big */

	/* Schedule this early */

	add	a4, a2, #OFFSET_CVMGlobalState_fastHeapLock
	ldr	v1, [a4]

	/* Keep cb in v2 for later */

	mov	v2, a3	       /* cb */

	/* Now compute instance size of the array */
	/* v3 holds length */

	/* len = (length << 2 + 12) */

	mov	a1, #12
	add	a3, a1, v3, LSL #2
	/* a3 = instance size */
	/* lock using atomic operation */
	mov     a1, #1	       /* locked	 */
#ifndef CVM_MP_SAFE	
	swp	a1, a1, [a4]   /* try locking */
	cmp	a1, #1         /* already locked. Bail. */
	beq	OBJARR_GOSLOW
#else
LABEL(_objarrAcquireLockMPSafe)
	ldrex	v1, [a4]
	cmp	v1, #1        /* already locked? */
	beq	OBJARR_GOSLOW /* already locked. Bail. */
	strex	v1, a1, [a4]  /* try lock */
	cmp	v1, #0        /* strex succeed? */
	bne	_objarrAcquireLockMPSafe
	mcr	p15, 0, v1, c7, c10, 5 /* memory barrier */
#endif

	/* Allocate inline */
	/* a2 = &CVMglobals */
	/* v2 = cb */

	ldr	a1, [a2, #OFFSET_CVMGlobalState_allocPtrPtr]
	ldr	a4, [a2, #OFFSET_CVMGlobalState_allocTopPtr]
	ldr	v1, [a1, #0]   /* v1 <- allocPtr   */
	ldr	a4, [a4, #0]   /* a4 <- allocTop */
	/* instance size is in a4 */
	adds	ip, v1, a3     /* ip <- allocNext (allocPtr v1 + size a3) */
	/* Check for overflow */
	bvs	OBJARR_GOUNLOCKANDSLOW
	cmp	ip, a4         /* Is ip <= a4 (within range?) */
	bhi	OBJARR_GOUNLOCKANDSLOW
	str  	ip, [a1]    /* and the new allocPtr is committed */

	/* v1 is the allocation point now */

/* IAI - 19*/
#ifdef IAI_PRELOAD_NEW_OBJECT	
       /* preload the memory to be allocated for the next object 
        * in the next NewGlue call */
	cmp a3, #64

	/* current allocated array is small size, 
	 * so the memory to be allocated is already in cache by 
         * preload in last NewGlue call
	 */
	blt SMALL_AARRAY_PLD

	/* large size array allocation, 
	 * the memory beyond 64 bytes is not in the cache, so preload it
	 */
	pld [v1, #96]
	pld [v1, #128]
	
LABEL(SMALL_AARRAY_PLD)       
	pld  [ip]
	pld  [ip, #32]
	pld  [ip, #64]


 	pld  [ip, #96]	
 	pld  [ip, #128]	
 	pld  [ip, #160]	
       
#endif	/* IAI_PRELOAD_NEW_OBJECT */

#ifdef CVM_FASTALLOC_STATS
	/* Count fast locks */
	ldr	a1, SYMBOL(fastLockCount)
	ldr	a2, [a1]
	add	a2, a2, #1
	str	a2, [a1]
#endif

	/* Allocation done here */

	mov     a4, v1         /* a4 -- iteration variable */
	str	v2, [a4], #4   /* Initialize cb */
	mov	a1, #2         /* CVM_LOCKSTATE_UNLOCKED */
	str	a1, [a4], #4   /* Initialize variousWord */
	str	v3, [a4], #4   /* And finally, initialize array length */
	/*
	 * Now a4 is at the start of the object's data
         * And ip is allocNext which is also the end of the object
	 */
	mov	a1, #0         /* Initialize to 0 */
	b	OBJARR_LOOPTEST
LABEL(OBJARR_INITLOOP)
	str	a1, [a4], #4   /* Next object field */
LABEL(OBJARR_LOOPTEST)
	cmp	a4, ip         /* Done? */
	bcc	OBJARR_INITLOOP
LABEL(OBJARR_ENDINIT)
	/* Unlock fast lock */
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. a1 has 0 */
#endif
	/* a1 is already 0 here. Store it into fastHeapLock */
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
	mov	a1, v1         /* Return object */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* Restore JSP/JFP and return to compiled code */
	ldr	a3,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	ldr	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage]
	/* a1 contains the resulting object */
	/* a3 contains the return address */
	/* jump */
	mov     pc, a3
LABEL(OBJARR_GOUNLOCKANDSLOW)
	/* Unlock by stuffing a zero in the lock */
	mov	a1, #0
#ifdef CVM_MP_SAFE
	mcr	p15, 0, a1, c7, c10, 5       /* memory barrier. */
#endif
	str	a1, [a2, #OFFSET_CVMGlobalState_fastHeapLock]
LABEL(OBJARR_GOSLOW)
	/* At this point, a3 holds the instance length */
	ldr	a1, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a1 -- ee */
	mov	a2, a3		/* a2 -- instance length */
        ldr     lr, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	mov	a3, v2          /* a3 -- class */
	mov	a4, v3          /* a4 -- array length */
#define SAVESET {a1, a2, a3}
	FIXUP_FRAMES(lr, SAVESET, 3)
#undef SAVESET
	CALL_VM_FUNCTION(CVMgcAllocNewArrayWithInstanceSize)
	cmp	a1, #0
	bne	OBJARR_RETURNOBJ
	adr	a2, cbString    /* "%C" */
	mov	a3, v2          /* cb */
LABEL(OBJARR_OUT_OF_MEMORY)
	ldr	a1, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a1 -- ee */
	CALL_VM_FUNCTION(CVMthrowOutOfMemoryError)
	mov	a1, sp
	ldr     pc, SYMBOL(CVMJITexitNative)
LABEL(OBJARR_BADINDEX)
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a1 -- ee */
        ldr     lr, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
#define SAVESET {a1}
	FIXUP_FRAMES(lr, SAVESET, 1)
#undef SAVESET
	cmp	v3, #0
	blt	OBJARR_NEGATIVEINDEX
	/* If we are here, the array was too big to even try to allocate */
	/* Throw out of memory error here */
	adr	a2, cbStringArr /* "[%C" */
	/* a3 already has the element cb here. */
	b	OBJARR_OUT_OF_MEMORY
LABEL(OBJARR_NEGATIVEINDEX)
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a1 -- ee */
	mov	a2, #0
	CALL_VM_FUNCTION(CVMthrowNegativeArraySizeException)
	mov	a1, sp
	ldr	pc, SYMBOL(CVMJITexitNative)
LABEL(OBJARR_RETURNOBJ)
	/* Restore JSP/JFP and return to compiled code */
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]   /* a2 -- ee */
        ldr     JFP, [a2,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] 
	ldr     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	ldr	v3, [sp, #OFFSET_CVMCCExecEnv_ccmStorage]
	/* a1 contains the resulting object */
	ldr     pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]

	SET_SIZE( CVMCCMruntimeANewArrayGlue ) 

/*
 * Allocate a multidimensional array. This is rare, so we just call 
 * the C helper.
 */
	ENTRY(CVMCCMruntimeMultiANewArrayGlue)
ENTRY1 ( CVMCCMruntimeMultiANewArrayGlue )

	/* Arguments:	 
	 *	a2 = nDimensions 
	 *	a3 = arrCb 
	 *	a4 = address of dimension array 
	 */

	/* Flush our state. */

	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]     /* a1 = ee */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] /* return PC saved */
#define SAVESET {a1, a2, a3, lr}
	FIXUP_FRAMES(JFP, SAVESET, 4)
#undef SAVESET
        str     JFP, [a1,  #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	mov	a1,  sp			/* setup ccee argument */
	BRANCH_TO_VM_FUNCTION(CVMCCMruntimeMultiANewArray)
	SET_SIZE ( CVMCCMruntimeMultiANewArrayGlue )

LABEL(cbString)
	STRING("%C")
LABEL(cbStringArr)
	STRING("[%C")
	POOL
