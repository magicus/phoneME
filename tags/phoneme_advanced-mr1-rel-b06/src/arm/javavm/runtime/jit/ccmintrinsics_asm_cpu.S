/*
 * Portions Copyright 2000-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License version 2 for more details (a copy is included at
 * /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public
 * License version 2 along with this work; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

/***********************************
 * Java intrinsic methods
 ***********************************/

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/asmmacros_cpu.h"

	SET_SECTION_EXEC(ccmintrinsics_asm_cpu)

#ifdef CVMJIT_INTRINSICS

#ifndef CVM_JIT_CCM_USE_C_HELPER
/*
 * Entry point for intrinsic implementation of Object.hashCode().
 */
	ENTRY ( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )
ENTRY1 ( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )
        /* r4 = v1 = jfp 
         * r5 = v2 = jsp 
         * sp = ccee 
	 */

#define CVM_MICROLOCK_LOCKED    0xff
#define CVM_MICROLOCK_UNLOCKED  0x00
#define OBJ                     r0
#define OBITS                   r1
#define MICROLOCK               r2

        /* NOTE: This function is based on the implementation of */
        /*       CVMobjectGetHashNoSet() in objsync.c. */

        /* Get the object from the argument: */
        ldr     OBJ, [JSP, #-4]

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
        ldr     MICROLOCK, =CVMobjGlobalMicroLockPtr
        ldr     MICROLOCK, [MICROLOCK]
#endif

        /* Acquire the microlock: */
        mov     r3, #CVM_MICROLOCK_LOCKED   /* Swap CVM_MICROLOCK_LOCKED into */
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _objectHashCodeSlowAcquireMicrolock    /* Branch if failed. */

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
        mov     r0, OBITS, LSR #CONSTANT_CVM_SYNC_BITS
        bics    r0, r0, #~(CONSTANT_CVM_HASH_MASK)
        beq     _objectHashCodeSlowRoute

LABEL(_objectHashCodeDone)
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

#undef CVM_MICROLOCK_LOCKED
#undef CVM_MICROLOCK_UNLOCKED
#undef OBJ
#undef OBITS
#undef MICROLOCK

        SET_SIZE( CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue )

/*
 * Entry point for intrinsic implementation of String.hashCode().
 */
	ENTRY ( CVMCCMARMintrinsic_java_lang_String_hashCodeGlue )
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

/*
 * Intrinsic version of String.indexOf(int ch, int fromIndex).
 */
/* IAI-05 */
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
	ENTRY ( CVMCCMintrinsic_java_lang_String_indexOf_II )
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
	ENTRY ( CVMCCMintrinsic_java_lang_String_indexOf_I )
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

#endif /* CVM_JIT_CCM_USE_C_HELPER */

#endif /* CVMJIT_INTRINSICS */
