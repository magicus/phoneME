/*
 * @(#)ccmintrinsics_asm_cpu.S	1.16 06/10/29
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
 * Java intrinsic methods
 ***********************************/

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/sync.h"

#ifdef CVMJIT_INTRINSICS

#ifndef CVM_JIT_CCM_USE_C_HELPER
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s5_ccmintrinsics_asm_cpu)

/* We only support fastlocking with swap/spinlock based microlocks */
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

#define OBJ                     r0
#define OBITS                   r1
#define MICROLOCK               r2

#ifndef CVMCCM_DISABLE_ARM_CVM_SYSTEM_IDENTITYHASHCODE_INTRINSIC
/*
 * Entry point for intrinsic implementation of Object.hashCode().
 */
	ENTRY(CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue)
ENTRY1 ( CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue )
        /* Get the object from the argument: */
        ldr     OBJ, [JSP, #-4]
	cmp	OBJ, #0
        bne	_objectHashCodeStart
	/* If we get here, the object is NULL.  We need to return a 0 hash
	   code for a NULL reference.  But since the return slot on the stack
	   already contains a NULL/0 value, we can just return without any
	   additional work: */
        mov     pc, lr                  /* Return to caller. */
#endif
	
/*
 * Entry point for intrinsic implementation of Object.hashCode().
 */
	ENTRY(CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue)
ENTRY1 ( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )
        /* r4 = v1 = jfp 
         * r5 = v2 = jsp 
         * sp = ccee 
	 */

        /* NOTE: This function is based on the implementation of */
        /*       CVMobjectGetHashNoSet() in objsync.c. */

        /* Get the object from the argument: */
        ldr     OBJ, [JSP, #-4]

LABEL(_objectHashCodeStart)
        /* Check if the object is unlocked or locked with a fastlock: */
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits. */
        and     r12, OBITS, #0x3
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        beq     _objectHashCodeTryFastGetHashCode

        /* If we get here, then the object could be locked or has a heavy 
         * monitor.  This means that the state of the data structures can 
         * be in flux.  We will need to acquire the microlock to ensure that
         * the hashCode we get is valid. 
	 */

        /* Get the address of the object microlock: */
/* IAI-04 */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        /* Get &microlock from wmmx register */
        textrmuw MICROLOCK, W_MICROLOCK, #0
#else
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
        ldr     MICROLOCK, [MICROLOCK]
#endif

        /* Acquire the microlock: */
        mov     r3, #CVM_MICROLOCK_LOCKED   /* Swap CVM_MICROLOCK_LOCKED into */
#ifndef CVM_MP_SAFE
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _objectHashCodeSlowAcquireMicrolock    /* Branch if failed. */
#else
LABEL(_objectHashCodeAcquireMicrolockMPSafe)
	ldrex	r12, [MICROLOCK]
	cmp	r12, #CVM_MICROLOCK_UNLOCKED /* already locked? */
	bne	_objectHashCodeSlowAcquireMicrolock
	strex	r12, r3, [MICROLOCK]         /* try lock */
	cmp	r12, #0                      /* strex succeed? */
	bne	_objectHashCodeAcquireMicrolockMPSafe
	mcr	p15, 0, r12, c7, c10, 5 /* memory barrier. r12 has 0. */
#endif

        /* The microlock has been acquired: */
LABEL(_objectHashCodeHasAcquiredMicrolock)
        /* Check to see if the object is locked with a fastlock: */
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits. */
        ands    r12, OBITS, #0x3

        /* If (obits & 0x3) == CVM_LOCKSTATE_LOCKED, go handle LOCKED case: */
        beq     _objectHashCodeGetObitsFromLockRecord

        /* The object may have gotten unlocked after we acquired the microlock. */
        /* Check for it:  */
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        beq     _objectHashCodeUnlockMicrolockAndTryFast

        /* Else, the object is in an inflated MONITOR state: */
        bic     OBITS, OBITS, #0x3          /* Mask off sync bits. */
        ldr     OBITS, [OBITS, #OFFSET_CVMObjMonitor_bits]
        b       _objectHashCodeUnlockMicrolockAndTryFast

LABEL(_objectHashCodeGetObitsFromLockRecord)
        /* Object is in LOCKED state: */
        ldr     OBITS, [OBITS, #OFFSET_CVMOwnedMonitor_u_fast_bits]

LABEL(_objectHashCodeUnlockMicrolockAndTryFast)
#ifdef CVM_MP_SAFE
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5       /* memory barrier. */
#endif
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

LABEL(_objectHashCodeTryFastGetHashCode)
        /* If we get here, then we must have caught the object in an UNLOCKED 
         * or LOCKED (i.e. not inflated) state.  So, we go ahead and extract 
         * the hash code bits.  If the bits are 0, then it means that we  
         * have not set the hash code yet. 
         * NOTE: This is dependent on CVM_OBJECT_NO_HASH being 0.  The check 
         *       is implicit in the "bics" instruction below. 
	 */
	ldr     r3, [OBJ, #OFFSET_CVMObjectHeader_clas] /* Get CB. */
        mov     r0, OBITS, LSR #CONSTANT_CVM_SYNC_BITS
        bics    r0, r0, #~(CONSTANT_CVM_HASH_MASK)
        beq     _objectHashCodeSlowRoute

LABEL(_objectHashCodeDone)
	eor	r3, r0, r3, LSR #4
	orr	r0, r0, r3, LSL #CONSTANT_CVM_HASH_BITS	/* Add the classbits.*/
        str     r0, [JSP, #-4]          /* Set the return value. */
        mov     pc, lr                  /* Return to caller. */

LABEL(_objectHashCodeSlowAcquireMicrolock)
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and will not be becoming GC safe while acquiring the
                 microlock.
        */
        str     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        mov     r0, MICROLOCK
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        ldr     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        b       _objectHashCodeHasAcquiredMicrolock

LABEL(_objectHashCodeSlowRoute)
        /* Got to go the slow route: */
        mov     r0, sp                                  /* arg1 = ccee. */
        ldr     r12, [r0, #OFFSET_CVMCCExecEnv_ee]

        /* The JSP flushed to the frame should point after the arguments.
           This allows GC to scan those arguments.  But we need to decrement
           the elements from the JSP register in preparation for the return
           from CVMCCMruntime_java_lang_System_arraycopy().  Since the helper
           is returning to the compiled code directly, we need to pre-adjust
           it before calling the helper.  It is OK to pre-adjust the JSP
           because no one will see this pre-adjusted value until we get back
           into the compiled code that called us which is what we want.
        */
        str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack] /* Flush JSP. */
        str     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] /* Flush return PC. */
        sub     r1, JSP, #4                            /* arg2 = &argument[0]. */
        str     JFP, [r12, #OFFSET_CVMExecEnv_interpreterStack + \
                            OFFSET_CVMStack_currentFrame] /* Flush JFP. */
        CALL_VM_FUNCTION(CVMCCMARMintrinsic_java_lang_Object_hashCode)

        str     r0, [JSP, #-4]          /* Set the return value. */
        ldr     pc, [JFP, #OFFSET_CVMCompiledFrame_PC]  /* Return to caller. */

#undef OBJ
#undef OBITS
#undef MICROLOCK

        SET_SIZE( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )

/*
 * Entry point for intrinsic implementation of String.hashCode().
 */
	ENTRY(CVMCCMARMintrinsic_java_lang_String_hashCodeGlue)
ENTRY1 ( CVMCCMARMintrinsic_java_lang_String_hashCodeGlue )
        /* r0 = a1 = (java.lang.String)thisString 
         * r4 = v1 = jfp 
         * r5 = v2 = jsp 
         * sp = ccee 
	 */

#define THIS            r0
#define COUNT           r1
#define OFFSET          r2
#define VALUE           r3
#define HASH            r12
#define PTR             r3
#define CHAR            r2
#define HASH_FACTOR     r0 /* lr */
#define CCEE            sp

        /* NOTE: We let the caller do the explicit null check of the this
           pointer if needed.  This way, the null check can be eliminated
           if redundant.

           But if the null check is trap based, then we have to make sure
           we give an opportunity for the trap to occur by dereferencing
           the object in our instructions.
        */

        /* Get the string value, offset, and count and setup for char array */
        /* traversal: */
        ldr     VALUE, [THIS, #OFFSET_java_lang_String_value]
        mov     HASH, #0
        ldr     OFFSET, [THIS, #OFFSET_java_lang_String_offset]
        add     PTR, VALUE, #OFFSET_ARRAY_ELEMENTS
        ldr     COUNT, [THIS, #OFFSET_java_lang_String_count]
        add     PTR, PTR, OFFSET, LSL #1
        subs    COUNT, COUNT, #1
        blt     String_hashCode_done

        /* Hash the 1st char: */
        ldrh    HASH, [PTR], #2
        subs    COUNT, COUNT, #1
        blt     String_hashCode_done

        /* Set HASH_FACTOR = 31: */
        mov     HASH_FACTOR, #31
LABEL(String_hashCode_loop)
        ldrh    CHAR, [PTR], #2
        subs    COUNT, COUNT, #1
        mla     HASH, HASH_FACTOR, HASH, CHAR
        bge     String_hashCode_loop

LABEL(String_hashCode_done)
        mov     r0, HASH
        mov     pc, lr                  /* Return to caller. */

#undef THIS
#undef COUNT
#undef OFFSET
#undef VALUE
#undef HASH
#undef PTR
#undef CHAR
#undef HASH_FACTOR
#undef CCEE

        SET_SIZE( CVMCCMARMintrinsic_java_lang_String_hashCodeGlue )

/* We only support fastlocking with atomic ops */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

/*
 * Entry point for intrinsic implementation of Object.hashCode().
 */
 ENTRY(CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue)
        ; r4 = v1 = jfp
        ; r5 = v2 = jsp
        ; sp = ccee

#ifdef WINCE_FAST_LOCKING

#define OBJ                     r0
#define OBITS                   r1
#define MICROLOCK               r2

        ; NOTE: This function is based on the implementation of
        ;       CVMobjectGetHashNoSet() in objsync.c.

        ; Get the object from the argument:
        ldr     OBJ, [JSP, #-4]

        ; Check if the object is unlocked or locked with a fastlock:
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] ; Get obits.
        and     r12, OBITS, #0x3
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        beq     _objectHashCodeTryFastGetHashCode

        ; If we get here, then the object could be locked or has a heavy
        ; monitor.  This means that the state of the data structures can
        ; be in flux.  We'll need to acquire the microlock to ensure that
        ; the hashCode we get is valid.

        ; Get the address of the object microlock:
        IMPORT CVMobjGlobalMicroLockPtr
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
        ldr     MICROLOCK, [MICROLOCK]

        /* Acquire the microlock: */
        mov     r3, #CVM_MICROLOCK_LOCKED   ; Swap CVM_MICROLOCK_LOCKED into
        swp     r3, r3, [MICROLOCK]         ;    the lockWord.
        cmp     r3, #CVM_MICROLOCK_UNLOCKED ; See if we succeeded.
        bne     _objectHashCodeSlowAcquireMicrolock    ; Branch if failed.

        /* The microlock has been acquired: */
_objectHashCodeHasAcquiredMicrolock
        /* Check to see if the object is locked with a fastlock: */
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] ; Get obits.
        ands    r12, OBITS, #0x3

        ; If (obits & 0x3) == CVM_LOCKSTATE_LOCKED, go handle LOCKED case:
        beq     _objectHashCodeGetObitsFromLockRecord

        ; The object may have gotten unlocked after we acquired the microlock.
        ; Check for it: 
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        beq     _objectHashCodeUnlockMicrolockAndTryFast

        ; Else, the object is in an inflated MONITOR state:
        bic     OBITS, OBITS, #0x3          ; Mask off sync bits.
        ldr     OBITS, [OBITS, #OFFSET_CVMObjMonitor_bits]
        b       _objectHashCodeUnlockMicrolockAndTryFast

_objectHashCodeGetObitsFromLockRecord
        ; Object is in LOCKED state:
        ldr     OBITS, [OBITS, #OFFSET_CVMOwnedMonitor_u_fast_bits]

_objectHashCodeUnlockMicrolockAndTryFast
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

_objectHashCodeTryFastGetHashCode
        ; If we get here, then we must have caught the object in an UNLOCKED
        ; or LOCKED (i.e. not inflated) state.  So, we go ahead and extract
        ; the hash code bits.  If the bits are 0, then it means that we 
        ; haven't set the hash code yet.
        ; NOTE: This is dependent on CVM_OBJECT_NO_HASH being 0.  The check
        ;       is implicit in the "bics" instruction below.
	ldr     r3, [OBJ, #OFFSET_CVMObjectHeader_clas]  ; Get CB.
        mov     r0, OBITS, LSR #CONSTANT_CVM_SYNC_BITS
        bics    r0, r0, #~(CONSTANT_CVM_HASH_MASK)
        beq     _objectHashCodeSlowRoute

_objectHashCodeDone
	eor	r3, r0, r3, LSR #4
	orr	r0, r0, r3, LSL #CONSTANT_CVM_HASH_BITS	; Add the CB bits.
        str     r0, [JSP, #-4]          ; Set the return value.
        mov     pc, lr                  ; Return to caller.

_objectHashCodeSlowAcquireMicrolock
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        str     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        mov     r0, MICROLOCK
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        ldr     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        b       _objectHashCodeHasAcquiredMicrolock

#endif

_objectHashCodeSlowRoute
        ; Got to go the slow route:
        mov     r0, sp                                  ; arg1 = ccee.
        ldr     r12, [r0, #OFFSET_CVMCCExecEnv_ee]

        /* The JSP flushed to the frame should point after the arguments.
           This allows GC to scan those arguments.  But we need to decrement
           the elements from the JSP register in preparation for the return
           from CVMCCMruntime_java_lang_System_arraycopy().  Since the helper
           is returning to the compiled code directly, we need to pre-adjust
           it before calling the helper.  It is OK to pre-adjust the JSP
           because no one will see this pre-adjusted value until we get back
           into the compiled code that called us which is what we want.
        */
        str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack] ; Flush JSP.
        str     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] ; Flush return PC.
        sub     r1, JSP, #4                             ; arg2 = &argument[0].
        str     JFP, [r12, #OFFSET_CVMExecEnv_interpreterStack + \
                            OFFSET_CVMStack_currentFrame] ; Flush JFP.
        CALL_VM_FUNCTION(CVMCCMARMintrinsic_java_lang_Object_hashCode)

        str     r0, [JSP, #-4]          ; Set the return value.
        ldr     pc, [JFP, #OFFSET_CVMCompiledFrame_PC]  ; Return to caller.

#undef OBJ
#undef OBITS
#undef MICROLOCK

        SET_SIZE( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )

/*
 * Entry point for intrinsic implementation of String.hashCode().
 */
 ENTRY(CVMCCMARMintrinsic_java_lang_String_hashCodeGlue)
        ; r0 = a1 = (java.lang.String)thisString
        ; r4 = v1 = jfp
        ; r5 = v2 = jsp
        ; sp = ccee

#define THIS            r0
#define COUNT           r1
#define OFFSET          r2
#define VALUE           r3
#define HASH            r12
#define PTR             r3
#define CHAR            r2
#define HASH_FACTOR     r0 /* lr */
#define CCEE            sp

        /* NOTE: We let the caller do the explicit null check of the this
           pointer if needed.  This way, the null check can be eliminated
           if redundant.

           But if the null check is trap based, then we have to make sure
           we give an opportunity for the trap to occur by dereferencing
           the object in our instructions.
        */

        ; Get the string value, offset, and count and setup for char array
        ; traversal:
        ldr     VALUE, [THIS, #OFFSET_java_lang_String_value]
        mov     HASH, #0
        ldr     OFFSET, [THIS, #OFFSET_java_lang_String_offset]
        add     PTR, VALUE, #OFFSET_ARRAY_ELEMENTS
        ldr     COUNT, [THIS, #OFFSET_java_lang_String_count]
        add     PTR, PTR, OFFSET, LSL #1
        subs    COUNT, COUNT, #1
        blt     String_hashCode_done

        ; Hash the 1st char:
        ldrh    HASH, [PTR], #2
        subs    COUNT, COUNT, #1
        blt     String_hashCode_done

        ; Set HASH_FACTOR = 31:
        mov     HASH_FACTOR, #31
String_hashCode_loop
        ldrh    CHAR, [PTR], #2
        subs    COUNT, COUNT, #1
        mla     HASH, HASH_FACTOR, HASH, CHAR
        bge     String_hashCode_loop

String_hashCode_done
        mov     r0, HASH
        mov     pc, lr                  ; Return to caller.

#undef THIS
#undef COUNT
#undef OFFSET
#undef VALUE
#undef HASH
#undef PTR
#undef CHAR
#undef HASH_FACTOR
#undef CCEE

        SET_SIZE( CVMCCMARMintrinsic_java_lang_String_hashCodeGlue )

#else
#error unsupported CVM_FASTLOCK_TYPE or CVM_MICROLOCK_TYPE
#endif

#endif /* !CVM_JIT_CCM_USE_C_SYNC_HELPER */

/*
 * Intrinsic version of String.indexOf(int ch, int fromIndex).
 */
/* IAI-05 */
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
	ENTRY(CVMCCMintrinsic_java_lang_String_indexOf_II)
ENTRY1 ( CVMCCMintrinsic_java_lang_String_indexOf_II )
        /*Arguments: 
         *    r0 = this object
         *    r1 = a character 
         *    r2 = from index 
	 *
         *
         *Register: 
         *    r3 = string length 
	 */

#define OFFSET_OF_STRING_VALUE 8
#define OFFSET_OF_STRING_OFFSET 12
#define OFFSET_OF_STRING_COUNT 16

#define OFFSET_OF_ARRAY_CHAR 12

        ldr     r3, [r0, #OFFSET_OF_STRING_COUNT]
        cmp     r2, #0
        movlt   r2, #0
        cmp     r2, r3
        mvnge   r0, #0
        movge   pc, lr

        ldr     r12, [r0, #OFFSET_OF_STRING_OFFSET]
        ldr     r0, [r0, #OFFSET_OF_STRING_VALUE] 

        add     r12, r12, r2 

        add     r12, r0, r12, lsl #1
        mov     r0, r2
        ldrh    r2, [r12, #OFFSET_OF_ARRAY_CHAR]!
LABEL(L1)
        cmp     r2, r1
        moveq   pc, lr
        ldrh    r2, [r12, #2]!
        add     r0, r0, #1
        cmp     r0, r3
        blt     L1

        mvn     r0, #0
        mov     pc, lr

#undef OFFSET_OF_STRING_VALUE
#undef OFFSET_OF_STRING_OFFSET
#undef OFFSET_OF_STRING_COUNT
#undef OFFSET_OF_ARRAY_CHAR
	SET_SIZE( CVMCCMintrinsic_java_lang_String_indexOf_II)

/* 
 * Intrinsic version of String.indexOf(int ch)
 */
	ENTRY(CVMCCMintrinsic_java_lang_String_indexOf_I)
ENTRY1 ( CVMCCMintrinsic_java_lang_String_indexOf_I )
        /*Arguments: 
         *    r0 = this object 
         *    r1 = a character 
	 *
         *Register: 
         *    r3 = string length 
	 */
 
#define OFFSET_OF_STRING_VALUE 8
#define OFFSET_OF_STRING_OFFSET 12
#define OFFSET_OF_STRING_COUNT 16

#define OFFSET_OF_ARRAY_CHAR 12

        ldr     r3, [r0, #OFFSET_OF_STRING_COUNT]
        ldr     r12, [r0, #OFFSET_OF_STRING_OFFSET]
        ldr     r0, [r0, #OFFSET_OF_STRING_VALUE]
        cmp     r3, #0
        mvneq   r0, #0
        moveq   pc, lr

        add     r12, r0, r12, lsl #1
        ldrh    r2, [r12, #OFFSET_OF_ARRAY_CHAR]!
        mov     r0, #0
LABEL(L2)
        cmp     r2, r1
        moveq   pc, lr
        ldrh    r2, [r12, #2]!
        add     r0, r0, #1
        cmp     r0, r3
        blt     L2

        mvn     r0, #0
        mov     pc, lr

#undef OFFSET_OF_STRING_VALUE
#undef OFFSET_OF_STRING_OFFSET
#undef OFFSET_OF_STRING_COUNT
#undef OFFSET_OF_ARRAY_CHAR
	SET_SIZE( CVMCCMintrinsic_java_lang_String_indexOf_I)
#endif

/* 
 * Intrinsic version of sun.misc.CVM.copyCharArray(...)
 */
	ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyCharArray)
ENTRY1 ( CVMCCMintrinsic_sun_misc_CVM_copyCharArray )
        /*Arguments: 
         *    r0 = srcArr 
         *    r1 = srcIndex
	 *    r2 = dstArr
         *    r3 = dstIndex
	 *    r6 = length	
	 */
	add	r0, r0, r1, LSL #1  /* r0 = src + (sIdx * 2) */
	add	r2, r2, r3, LSL #1  /* r2 = dst + (dIdx * 2) */
	add	r1, r0, #OFFSET_ARRAY_ELEMENTS /* r1 = src+(2*sIdx) + ofst */
	add	r0, r2, #OFFSET_ARRAY_ELEMENTS /* r0 = dst+(2*dIdx) + ofst */
	mov	r2, r6, LSL #1	    /* r2 = (length * 2) */
	b	SYM_NAME(CVMARMmemmove16BitLocal)
	SET_SIZE( CVMCCMintrinsic_sun_misc_CVM_copyCharArray )


/* 
 * Intrinsic version of sun.misc.CVM.copyObjectArray(...)
 */
#if (CVM_GCCHOICE == CVM_GC_GENERATIONAL) && !defined(CVM_SEGMENTED_HEAP)

#ifdef __RVCT__
	IMPORT CVMgcCardTableVirtualBasePtr
#endif

	ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyObjectArray)
ENTRY1 ( CVMCCMintrinsic_sun_misc_CVM_copyObjectArray )
        /*Arguments: 
         *    r0 = srcArr 
         *    r1 = srcIndex
	 *    r2 = dstArr
         *    r3 = dstIndex
	 *    r6 = length
	 */

#define CARDSTART	ip
#define CARDEND		r3

	ldr	CARDSTART, SYMBOL(CVMgcCardTableVirtualBasePtr)

	/* Now do the copying.  NOTE: dstAddr is already in r3. */
	add	r0, r0, r1, LSL #2  /* r0 = src + (4 * sIdx) */
	add	r2, r2, r3, LSL #2  /* r2 = dst + (4 * dIdx) */
	ldr	CARDSTART, [CARDSTART]
	add	r1, r0, #OFFSET_ARRAY_ELEMENTS /* r1 = src+(4*sIdx) + ofst */
	add	r0, r2, #OFFSET_ARRAY_ELEMENTS /* r0 = dst+(4*dIdx) + ofst */
	mov	r2, r6, LSL #2	    /* r2 = (length * 4) */

	/* First, mark the card table. 
	   cardStart = CTVBptr + (dst / 512)
	   cardEnd = CTVBptr + ((dst + len - 4) / 512)
	*/
	ldr	CARDSTART, [CARDSTART]
	add	CARDEND, r0, r2
	sub	CARDEND, CARDEND, #4
	add	CARDEND, CARDSTART, CARDEND, LSR #CONSTANT_CVM_GENGC_CARD_SHIFT
	add	CARDSTART, CARDSTART, r0, LSR #CONSTANT_CVM_GENGC_CARD_SHIFT

	mov	r6, #CONSTANT_CARD_DIRTY_BYTE	/* CARD_DIRTY_BYTE is 1 */
LABEL(copyObjectArray_markCard)
	strb	r6, [CARDSTART], #1
	cmp	CARDSTART, CARDEND
	bls	copyObjectArray_markCard
	
#undef CARDSTART
#undef CARDEND

	b	SYM_NAME(CVMARMmemmove32BitLocal)

	SET_SIZE ( CVMCCMintrinsic_sun_misc_CVM_copyObjectArray )

#endif /* CVM_GCCHOICE && !CVM_SEGMENTED_HEAP */	

/* 
 * Intrinsic version of System.arraycopy(Object srcArr, int srcIndex,
 *	Object dstArr, int dstIndex, int length).
 */

#define CCEE	r0
#define SRC	r1
#define SRCIDX	r2
#define DST	r3
#define DSTIDX	r6
#define LENGTH	r7

#define SRCCB	ip
#define DSTCB	r0
#define	TYPE	r0
#undef  TEMP
#define TEMP	ip

	ENTRY(CVMCCMintrinsic_java_lang_System_arraycopyGlue)
ENTRY1 ( CVMCCMintrinsic_java_lang_System_arraycopyGlue )
        /*Arguments: 
	 *    r0 = ccee
         *    r1 = srcArr 
         *    r2 = srcIndex
	 *    r3 = dstArr
         *    r6 = dstIndex
	 *    r7 = length
	 */

#ifndef CVMJIT_TRAP_BASED_NULL_CHECKS
	cmp	SRC, #0
	beq	slowArraycopy
	cmp	DST, #0
	beq	slowArraycopy
#endif
	
	/* Check array parameters: */
	/* IAI-23 */
	/* Ensure: srcIndex, dstIndex, and length are >= 0 i.e. NOT < 0: */
	orrs	TEMP, SRCIDX, DSTIDX
	orrges	TEMP, TEMP, LENGTH
	ldr	TEMP, [SRC, #OFFSET_ARRAY_LENGTH]
	blt	slowArraycopy

	ldr	DSTCB, [DST]	/* dst classblock */

	/* Ensure: length + srcIndex <= srcLen: */
	sub	TEMP, TEMP, LENGTH
	cmp	SRCIDX, TEMP
	ldr	TEMP, [DST, #OFFSET_ARRAY_LENGTH]
	bgt	slowArraycopy

	/* Ensure: length + dstIndex <= dstLen: */
	sub	TEMP, TEMP, LENGTH
	cmp	DSTIDX, TEMP
	ldr	SRCCB, [SRC]	/* src classblock */
	bgt	slowArraycopy

	bic	DSTCB, DSTCB, #3   /* Clear the low bits of dst arrayCb */
	bic	SRCCB, SRCCB, #3   /* Clear the low bits of src arrayCb */
	cmp	SRCCB, DSTCB
	ldr	TYPE, [SRCCB, #OFFSET_CVMClassBlock_classNameX] /* typeid */
	bne	slowArraycopy

	/* src & dst are same types: */
	/* Check to see if they are arrays:
	   NOTE: The typeids for the primitive arrays are:
		int[]	  0x4003  // Dimension 1, Element 0x3
		short[]	  0x4004  // Dimension 1, Element 0x4
		char[]	  0x4005  // Dimension 1, Element 0x5
		long[]	  0x4006  // Dimention 1, Element 0x6
		byte[]	  0x4007  // Dimension 1, Element 0x7
		float[]	  0x4008  // Dimension 1, Element 0x8
		double[]  0x4009  // Dimension 1, Element 0x9
		boolean[] 0x400a  // Dimension 1, Element 0xa

           Object arrays will have dimension >= 1, and element type which is
	   not in the base types above.
	   A dimension of 0 means not an array type.
	*/

	/* First get the array dimension from the typeid: */
	movs	TEMP, TYPE, LSR #CONSTANT_CVMtypeidArrayShift
	beq	slowArraycopy	/* If 0, then it's not an array. */

	/* We now know that it's an array: */
	cmp	TEMP, #0x1	/* Check if dimension is greater than 1. */
	bne	arraycopyObject	/* typeid not between 0x4003 and 0x400a */

	/* Check the element/base type range: */
	bic	TYPE, TYPE, #CONSTANT_CVMtypeidArrayMask
	cmp	TYPE, #0x3
	blt	slowArraycopy	/* typeid < 0x4003.  Something's wrong. */
	cmp	TYPE, #0xa
	bgt	arraycopyObject	/* typeid not between 0x4003 and 0x400a */

	/* We now know that we have a primitive array.  Dispatch to the
	   specific array copier: */
	sub	TYPE, TYPE, #3
	add	pc, pc, TYPE, LSL #2
	nop

	b	arraycopy32bit	/* arraycopyInt */
	b	arraycopy16bit	/* arraycopyShort */
	b	arraycopy16bit	/* arraycopyChar */
	b	arraycopy64bit	/* arraycopyLong */
	b	arraycopy8bit	/* arraycopyByte */
	b	arraycopy32bit	/* arraycopyFloat */
	b	arraycopy64bit	/* arraycopyDouble */
	b	arraycopy8bit	/* arraycopyBoolean */

LABEL(arraycopy8bit)
	add	r1, SRC, SRCIDX
	add	r1, r1, #OFFSET_ARRAY_ELEMENTS	/* srcAddr */
	mov	r2, LENGTH			/* length */
	add	r3, DST, DSTIDX
	add	r0, r3, #OFFSET_ARRAY_ELEMENTS	/* dstAddr */
	b	SYM_NAME(CVMARMmemmove8BitLocal)

LABEL(arraycopy16bit)
	add	r1, SRC, SRCIDX, LSL #1
	add	r1, r1, #OFFSET_ARRAY_ELEMENTS	/* srcAddr */
	mov	r2, LENGTH, LSL #1		/* length */
	add	r3, DST, DSTIDX, LSL #1
	add	r0, r3, #OFFSET_ARRAY_ELEMENTS	/* dstAddr */
	b	SYM_NAME(CVMARMmemmove16BitLocal)

LABEL(arraycopy32bit)
	add	r1, SRC, SRCIDX, LSL #2
	add	r1, r1, #OFFSET_ARRAY_ELEMENTS	/* srcAddr */
	mov	r2, LENGTH, LSL #2		/* length */
	add	r3, DST, DSTIDX, LSL #2
	add	r0, r3, #OFFSET_ARRAY_ELEMENTS	/* dstAddr */
	b	SYM_NAME(CVMARMmemmove32BitLocal)

LABEL(arraycopy64bit)
	add	r1, SRC, SRCIDX, LSL #3
	add	r1, r1, #OFFSET_ARRAY_ELEMENTS	/* srcAddr */
	mov	r2, LENGTH, LSL #3		/* length */
	add	r3, DST, DSTIDX, LSL #3
	add	r0, r3, #OFFSET_ARRAY_ELEMENTS	/* dstAddr */
	b	SYM_NAME(CVMARMmemmove32BitLocal)

LABEL(arraycopyObject)
#if (CVM_GCCHOICE == CVM_GC_GENERATIONAL) && !defined(CVM_SEGMENTED_HEAP)

#define CARDSTART	ip
#define CARDEND		DSTIDX	/* r6 */

	/* Compute arguments for CVMARMmemmove32Bit:
	   dst(r0), src(r1), length(r2)
	*/
	ldr	CARDSTART, SYMBOL(CVMgcCardTableVirtualBasePtr)
	add	r0, SRC, SRCIDX, LSL #2	/* r0 = src + (4 * sIdx) */
	add	r2, DST, DSTIDX, LSL #2 /* r2 = dst + (4 * dIdx) */
	ldr	CARDSTART, [CARDSTART]
	add	r1, r0, #OFFSET_ARRAY_ELEMENTS /* r1 = src+(4*sIdx) + ofst */
	add	r0, r2, #OFFSET_ARRAY_ELEMENTS /* r0 = dst+(4*dIdx) + ofst */
	mov	r2, LENGTH, LSL #2             /* r2 = (length * 4) */

	/* First, mark the card table. 
	   cardStart = CTVBptr + (dst / 512)
	   cardEnd = CTVBptr + ((dst + len - 4) / 512)
	*/
	ldr	CARDSTART, [CARDSTART]
	add	CARDEND, r0, r2
	sub	CARDEND, CARDEND, #4
	add	CARDEND, CARDSTART, CARDEND, LSR #CONSTANT_CVM_GENGC_CARD_SHIFT
	add	CARDSTART, CARDSTART, r0, LSR #CONSTANT_CVM_GENGC_CARD_SHIFT

	mov	r7, #CONSTANT_CARD_DIRTY_BYTE	/* CARD_DIRTY_BYTE is 1 */
LABEL(arraycopyObject_markCard)
	strb	r7, [CARDSTART], #1
	cmp	CARDSTART, CARDEND
	bls	arraycopyObject_markCard

#undef CARDSTART
#undef CARDEND

	/* Now do the copying.  NOTE: dstAddr is already in r3. */
	b	SYM_NAME(CVMARMmemmove32BitLocal)

#else

	/* Fall thru to the slow case coz we don't know how to handle it
	   (i.e. an unknown GC type): */

#endif /* CVM_GCCHOICE && !CVM_SEGMENTED_HEAP */	

LABEL(slowArraycopy)
	/* NOTE: The C intrinsic function expects the 5th and 6th arguments
	   on the operand stack instead of in registers.  Hence, we need to
	   store these on the stack before calling the C intrinsic. */
	str	r6, [sp, #-8]
	mov	r0, sp	/* Reload the CCEE. */
	str	r7, [sp, #-4]
	str	lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
	sub	sp, sp, #8
	CALL_VM_FUNCTION(CVMCCMintrinsic_java_lang_System_arraycopy)
	add	sp, sp, #8
	ldr	pc, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
	/* Let the C intrinsic do the hard work: */

#undef CCEE
#undef SRC
#undef SIDX
#undef DST
#undef DIDX
#undef LENGTH

#undef SRCCB
#undef DSTCB
#undef TYPE
#undef TEMP

	SET_SIZE( CVMCCMintrinsic_java_lang_System_arraycopyGlue )

#endif /* CVM_JIT_CCM_USE_C_HELPER */

#endif /* CVMJIT_INTRINSICS */

        POOL
