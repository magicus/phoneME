/*
 * @(#)ccmmath_cpu.S	1.112 06/10/10
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

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/porting/endianness.h"
#include "javavm/include/iai_opt_config.h"

#ifdef __RVCT__
#define OR2(x,y)	((x) :OR: (y))
#define OR3(x,y,z)	((x) :OR: (y) :OR: (z))
#define OR4(x,y,z,p)	((x) :OR: (y) :OR: (z) :OR: (p))
#else
#define OR2(x,y)	((x) | (y))
#define OR3(x,y,z)	((x) | (y) | (z))
#define OR4(x,y,z,p)	((x) | (y) | (z) | (p))
#endif

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s4_ccmmath_cpu)

/*************************************
 * Float helpers start here!
 *************************************/
 
/*
 * Entry point for comparing floats.
 * NOTE: The result is in the condition codes that have been set by various
 *       comparisons. Even though the C prototype for this helper indicates
 *       that the helper is to return an integer status in a register, this
 *       assembly version returns the result in the CPU condition code
 *       register (thus effectively making the return type of the function
 *       void).  This is OK because the ARM specific rules which emits calls
 *       to this helper function will be expecting the result to be in the
 *       condition code register.  This method of returning the result is
 *       done for optimization purposes.
 */
	ENTRY(CVMCCMruntimeFCmp)
ENTRY1 ( CVMCCMruntimeFCmp )
	ENTRY(CVMCCMruntimeFCmp_C)
ENTRY1 ( CVMCCMruntimeFCmp_C )
        /* r0 = a1 = float1 
         * r1 = a2 = float2 
         * r2 = a3 = return value if nan. 
         * v1 = jfp  
         * v2 = jsp 
         * sp = ccee
	 */

        /* uses r0, r1, r2, r3 */

#define FLOAT1      r0
#define FLOAT2      r1
#define NAN_RESULT  r2

        /* Extract exponent1 and check float1 for a NaN: */
        mov     r3, #0xff
        and     r3, r3, FLOAT1, LSR #23     /* Extract exponent1. */
        cmp     r3, #0xff                   /* Check for infinity or nan. */
        beq     _fcmpFloat1CheckForNan
LABEL(_fcmpFloat1IsNotNan)

        /* Extract exponent2 and check float2 for a NaN: */
        mov     r3, #0xff
        and     r3, r3, FLOAT2, LSR #23     /* Extract exponent2. */
        cmp     r3, #0xff                   /* Check for infinity or nan. */
        beq     _fcmpFloat2CheckForNan
LABEL(_fcmpFloat2IsNotNan)

        /* Check if the 2 floats have the same sign bit: */
        eor     r2, FLOAT1, FLOAT2      /* Check to see if the sign bit is the */
        tst     r2, #0x80000000         /*    same. */
        bne     _fcmpEvaluateResultBasedOnSigns

        /* If we get here, then the sign bits are the same.  Next, check if
           the sign bits are negative: */
        tst     FLOAT1, #0x80000000     /* Check for negative sign bit. */
        bic     FLOAT1, FLOAT1, #0x80000000     /* Remove float1's sign. */
        bic     FLOAT2, FLOAT2, #0x80000000     /* Remove float2's sign. */
        bne     _fcmpDoNegativeChecks

        /* Do positive version of comparing the floats: */
        cmp     FLOAT1, FLOAT2
        mov     pc, lr                  /* Return to the caller. */

LABEL(_fcmpDoNegativeChecks)
        /* Do negative version of comparing the floats: */
        cmp     FLOAT2, FLOAT1
        mov     pc, lr                  /* Return to the caller. */

LABEL(_fcmpEvaluateResultBasedOnSigns)
        /* Check for the special case where the 2 numbers are zeroes.  If so,
           the two are still equal even if their signs are not: */

        mov     r3, #0
        cmp     r3, FLOAT1, LSL #1      /* Check if float1 w/o sign is 0. */
        bne     _fcmpNotBothZeroes
        cmp     r3, FLOAT2, LSL #1      /* Check if float2 w/o sign is 0. */

        /* If it's a zero, then both numbers are 0s and equal.  The condition
           code is already set correctly.  Just return to the caller: */
        moveq   pc, lr                  /* Return the result. */

LABEL(_fcmpNotBothZeroes)
        cmp     FLOAT1, FLOAT2          /* Compare the signs. */
        mov     pc, lr                  /* Return the result. */

LABEL(_fcmpFloat1CheckForNan)
        /* Check if the mantissa is 0 (i.e. float1 is an infinity): */
        mov     r3, FLOAT1, LSL #9      /* mantissa1 = float1 << 9. */
        cmp     r3, #0                  /* Check if mantissa1 is 0. */
        beq     _fcmpFloat1IsNotNan     /* If is infinity, resume in main code.*/
        cmp     NAN_RESULT, #0          /* Else, is NaN.  Return the desired */
        mov     pc, lr                  /*   condition code result. */

LABEL(_fcmpFloat2CheckForNan)
        /* Check if the mantissa is 0 (i.e. float2 is an infinity): */
        mov     r3, FLOAT2, LSL #9      /* mantissa2 = float2 << 9. */
        cmp     r3, #0                  /* Check if mantissa2 is 0. */
        beq     _fcmpFloat2IsNotNan     /* If is infinity, resume in main code.*/
        cmp     NAN_RESULT, #0          /* Else, is NaN.  Return the desired */
        mov     pc, lr                  /*   condition code result. */

#undef FLOAT1
#undef FLOAT2
#undef NAN_RESULT

/*
 * NOTE: FAdd, FSub, and FMul all use the same register designations.
 */

#define FLOAT1  r0
#define FLOAT2  r1
#define EXP1    r2
#define EXP2    r3
#define MANT1   r6
#define MANT2   r7
#define EMASK   r12
#define MMASK   r12

#define FLOAT_SAVE_SET 	  {r6-r7, lr}
#define FLOAT_RESTORE_SET {r6-r7, pc}

#ifndef __RVCT__
#define	FLOAT_RETURN_TO_CALLER_IF(cond) \
	ldm/**/cond/**/fd	sp!, FLOAT_RESTORE_SET
#else
#define	FLOAT_RETURN_TO_CALLER_IF(cond) \
	ldm##cond##fd	sp!, FLOAT_RESTORE_SET
#endif
#define	FLOAT_RETURN_TO_CALLER \
    FLOAT_RETURN_TO_CALLER_IF(al)

/*
 * Entry point for subtracting floats.
 * NOTE: The result is in r0.
 */
	ENTRY(CVMCCMruntimeFSub)
ENTRY1 ( CVMCCMruntimeFSub )
	ENTRY(CVMCCMruntimeFSub_C)
ENTRY1 ( CVMCCMruntimeFSub_C )
        /* r0 = a1 = float1 */
        /* r1 = a2 = float2 */
        /* v1 = jfp */
        /* v2 = jsp */
        /* sp = ccee */

        eor     r1, r1, #0x80000000     /* Negate float2. */
        /* Fall through to CVMCCMruntimeFAdd: */

/*
 * Entry point for adding floats.
 * NOTE: The result is in r0.
 */
	ENTRY(CVMCCMruntimeFAdd)
ENTRY1 ( CVMCCMruntimeFAdd )
	ENTRY(CVMCCMruntimeFAdd_C)
ENTRY1 ( CVMCCMruntimeFAdd_C )
        /* r0 = a1 = float1 
         * r1 = a2 = float2 
         * v1 = jfp  
         * v2 = jsp 
         * sp = ccee
	 */
/* IAI - 16 */
#ifdef IAI_WMMX_FADD
#define FEXP1 r2
#define FEXP2 r3
#define FMANT1 ip
#define FMANT2 r1
        @ arrange for the larger summand to be in FLOAT1
        mov	ip, FLOAT1, LSL #1      
        cmp	ip, FLOAT2, LSL #1
        tmcrrlo	wR0, FLOAT1, FLOAT2
        tmrrclo	FLOAT2, FLOAT1, wR0		/* exchage FLOAT1 and FLOAT2 */

LABEL(_fadd_noxchng)
        mov     EMASK, #0xff			/* Setup the pre-shifted exponent mask. */
        ands    FEXP2, EMASK, FLOAT2, LSR #23	/* Extract exponent2. */
        beq	_fadd_fexp2_equal_0
        and	FEXP1, EMASK, FLOAT1, LSR #23	/* Extract exponent1. */
        
        cmp     FEXP1, EMASK
        cmpne   FEXP2, EMASK
        beq     _fadd_fexp_equal_emask
       
        sub	r3, FEXP1, FEXP2		/* exp1 = exp1 - exp2. */
        cmp	r3, #25				/* difference too large, return */
        movhi	pc, lr
        
        teq	FLOAT1, FLOAT2
        bic	FMANT2, FLOAT2, #0xff000000	/* Extract mantissa2.	*/
        orr	FMANT2, FMANT2, #0x00800000
       	bmi	_fadd_sub
		
LABEL(_fadd_add)       
        bic	FMANT1, FLOAT1, #0xff000000	/* Extract mantissa1.	*/
        orr	FMANT1, FMANT1, #0x00800000

        add	ip, FMANT1, FMANT2, LSR r3	/* {hi<ip>,lo<r1>} = FMANT1 + FMANT2. */
        rsb     r3, r3, #32
	mov	FMANT2, FMANT2, LSL r3
	
LABEL(_fadd_process_result)        
        tst	ip, #0x01000000 
	addne	FEXP1, FEXP1, #1
	movnes	ip, ip, lsr #1
	moveqs  r1, r1, lsl #1
	adc	ip, ip, #0
	bcs	_fadd_process_result_round	/* r1 = #0x8xxxxxxx */

LABEL(_fadd_process_result_done)	
	tst	ip, #0x01000000
	addne	FEXP1, FEXP1, #1

	cmp	FEXP1, #0xff
	and	r0, r0, #0x80000000
	orr	r0, r0, FEXP1, lsl #23
	bicne	ip, ip, #0x01800000
	orrne	r0, ip, r0
	mov 	pc, lr				/* normalized result returned. */

LABEL(_fadd_process_result_round)
	teq	r1, #0
	biceq	ip, ip, #1
	b	_fadd_process_result_done

LABEL(_fadd_fexp2_equal_0)
        ands	FEXP1, EMASK, FLOAT1, LSR #23	/* Extract exponent1. */
        beq	_fadd_fexp1_equal_0
        
        cmp     FEXP1, EMASK
        moveq	pc, lr
        
        teq	FLOAT1,FLOAT2
        bic	FMANT2, FLOAT2, #0x80000000
        sub	r3, FEXP1, #1
        bpl	_fadd_add
	
LABEL(_fadd_sub)
        bic	FMANT1, FLOAT1, #0x7f000000	/* Extract mantissa1.	*/
        orr	FMANT1, FMANT1, #0x00800000
	rsb	r0, r3, #32
	mov	r0, FMANT2, LSL r0
	rsbs	r0, r0, #0
	sbc	ip, FMANT1, FMANT2, LSR r3	/* {hi<ip>,lo<r0>} = FMANT1 - FMANT2. */

	tst	ip, #0x00800000
	beq	_fadd_sub_result_need_shift
	
	cmp	r0, #0x80000000
	addhs	ip, ip, #1
	biceq	ip, ip, #1
	bic	r0, ip, #0x00800000
	orr	r0, r0, FEXP1, lsl #23
	mov	pc, lr

LABEL(_fadd_sub_result_need_shift)
	bic	r3, ip, #0x80000000
	orrs	r1, r3, r0
	moveq	r0, #0
	moveq	pc, lr				/* FLOAT1 = - FLOAT2, return 0 */
	
	clz	r1, r3
	sub	r1, r1, #8
	
	cmp	FEXP1, r1
	suble	r1, FEXP1, #1
	movle	FEXP1, ip, LSR #23
	subgt	FEXP1, FEXP1, r1
	orrgt	FEXP1, FEXP1, ip, LSR #23
	
	mov	ip, r3, LSL r1			/* need shift left */
	rsb	r3, r1, #32
	orr	ip, ip, r0, LSR r3
	mov	r0, r0, LSL r1
	
	cmp	r0, #0x80000000
	addhs	ip, ip, #1
	biceq	ip, ip, #1

	tst	ip, #0x01000000
	addne	FEXP1, FEXP1, #1
	bic	ip, ip, #0x01800000
	orr	r0, ip, FEXP1, LSL #23
	mov	pc, lr
        
LABEL(_fadd_fexp1_equal_0)   
	eors	r3, FLOAT1, FLOAT2
	bic	r2, FLOAT2, #0x80000000
	addpl	r0, FLOAT1, r2
	movpl	pc, lr
	
	teq	r3, #0x80000000
	moveq	r0, #0
	subne	r0, FLOAT1, r2
	mov	pc, lr
	     

LABEL(_fadd_fexp_equal_emask)
	cmp	FEXP1, EMASK
	movne	r0, FLOAT2
        cmpeq   FEXP2, EMASK
        movne	pc, lr
        
	eors	r2, FLOAT1, FLOAT2	
        orr	r0, FLOAT1, FLOAT2	
        movpl	pc, lr

        mov	r0, #0x7f000000
        orr	r0, r0, #0x00c00000
        mov	pc, lr
        
#else /* IAI - 16 */
LABEL(_faddStart)
        /* The following saves registers that we're going to use and extracts
           exponents and mantissas from the floats.  */

        mov     EMASK, #0xff        /* Setup the pre-shifted exponent mask. */

	stmfd	sp!, FLOAT_SAVE_SET
        and     EXP1, EMASK, FLOAT1, LSR #23    /* Extract exponent1. */
        and     EXP2, EMASK, FLOAT2, LSR #23    /* Extract exponent2. */

        mvn     MMASK, #0xc0000000
        and     MANT1, MMASK, FLOAT1, LSL #7    /* Extract mantissa1. */
        and     MANT2, MMASK, FLOAT2, LSL #7    /* Extract mantissa2. */

        /* Check if float1 or float2 is NaN or infinity: */
        cmp     EXP1, #0xff         /* Check float1 for infinity or nan. */
        cmpne   EXP2, #0xff         /* Check float2 for infinity or nan. */
        beq     _faddFloatCheckForNanOrInfinity

        /* If we get here, then both float1 and float2 are not infinities nor
           NaNs.  From this point forth, we'll use r12 for scratch regs
           instead of EMASK and MMASK. */

        /* Check for zeroes or denormalized numbers: */
        cmp     EXP1, #0
        beq     _faddCheckFloat1ForZero
        orr     MANT1, MANT1, #0x40000000   /* Else, set implied high bit. */
LABEL(_faddFloat1IsNotZero)

        cmp     EXP2, #0
        beq     _faddCheckFloat2ForZero
        orr     MANT2, MANT2, #0x40000000   /* Else, set implied high bit. */
LABEL(_faddFloat2IsNotZero)

        /* Make sure the magnitude of float1 is larger than the magnitude of
           float2.  If not, then swap the two:*/
        cmp     EXP1, EXP2              /* Compare exponents. */
        cmpeq   MANT1, MANT2            /* Compare mantissas. */

        blt     _faddSwapFloat1AndFloat2

LABEL(_faddFloat1IsGreaterThanFloat2)

        /* At this point, the mantissas should be fixed point numbers where
           the binary point is between bit 30 and 29: */

        /* Prepare float1 and float2 for addition: */
        sub     r12, EXP1, EXP2        /* r12 = order of magniture difference. */
        cmp     r12, #7

        /* At this point, EXP2 i.e. r3 is free to be used as a scratch. */

        /* Adjust float2 to the same order of magnitude as float1 if OK to do
           so: */
        movle   MANT2, MANT2, LSR r12
        ble     _faddReadyToAddOrSubtract

        /* Save the excess bits shifted out from MANT2.  All we care about is
           whether they are 0 or not: */
        cmp     r12, #32
        rsblt   r3, r12, #32
        movlt   r3, MANT2, LSL r3
        movge   r3, MANT2

        /* Adjust float2 to the same order of magnitude as float1: */
        mov     MANT2, MANT2, LSR r12

        cmp     r3, #0
        orrne   MANT2, MANT2, #0x10     /* Adjust for rounding later. */

LABEL(_faddReadyToAddOrSubtract)

        /* Set the result sign in r0: */
        and     r0, FLOAT1, #0x80000000 /* Set r0 = result sign. */

        /* We now move the implied binary point to between bit 31 and 30 (i.e.
           left by one).  We need to compensate for this by incrementing the
           exponent: */
        add     EXP1, EXP1, #1

        /* Check if the signs are the same: */
        eors    r12, FLOAT1, FLOAT2     /* Check if the sign bit is the same. */
        bpl     _faddDoAdd              /* High bit will not be set if same. */

LABEL(_faddDoSub)
        subs    MANT1, MANT1, MANT2     /* Subtract and set condition codes. */
        beq     _faddReturnZero         /* If zero, then go return zero. */

LABEL(_faddNormalizeResult)

        /* If we get here, then we know MANT1 is not zero.  It's OK to keep
           shifting until we find a bit because there is bound to be 1 within
           the 32 bits of MANT1: */
LABEL(_faddNormalizing)
        cmp     EXP1, #1                /* Else, normalized if necessary. */
        beq     _floatRoundResult       /* Cannot normalize, go wrap-up. */
        adds    MANT1, MANT1, MANT1     /* Shift left by 1. */
        sub     EXP1, EXP1, #1
        bpl     _faddNormalizing        /* If implied high bit is not found, */
                                        /*   continue looking. */
        b       _floatRoundResult       /*   then go do rounding. */

LABEL(_faddDoAdd)
        /* If we get here, the signs are the same.  We can just add both
           numbers as if they are positive numbers, and then reapply the
           original when we're done. */

        adds    MANT1, MANT1, MANT2     /* Add and set condition codes. */
        bpl     _faddNormalizeResult  /* If high bit is not set, go normalize. */
        b       _floatRoundResult       /* Go round result. */

LABEL(_faddSwapFloat1AndFloat2)
        /* Swap the values in { float1, exponent1, mantissa1 } with the values
           in { float2, exponent2, mantissa2 }: */
        mov     r12, EXP1
        mov     EXP1, EXP2
        mov     EXP2, r12

        mov     r12, MANT1
        mov     MANT1, MANT2
        mov     MANT2, r12

        mov     r12, FLOAT1
        mov     FLOAT1, FLOAT2
        mov     FLOAT2, r12
        b       _faddFloat1IsGreaterThanFloat2

LABEL(_faddCheckFloat1ForZero)
        cmp     MANT1, #0               /* Check to see if float1 is a zero. */
        beq     _faddFloat1IsZero
        mov     EXP1, #1              /* Else, not zero.  Adjust EXP1 because */
        b       _faddFloat1IsNotZero    /*   it is implied to be 1. */

LABEL(_faddFloat1IsZero)
        /* Check if float2 is also zero: */
        cmp     EXP2, #0
        bne     _faddReturnFloat2       /* float2 not zero.  Return float2. */
        cmp     MANT2, #0
        bne     _faddReturnFloat2       /* float2 not zero.  Return float2. */

        /* Fall through to _faddReturnZero. */

LABEL(_faddReturnZero)
        /* If the signs are the same, return the zero of said sign.
           Else, return positive 0: */
        and     r0, FLOAT1, FLOAT2      /* Massage the signs. */
        and     r0, r0, #0x80000000     /* Else return 0 with sign of the */
        FLOAT_RETURN_TO_CALLER 	        /*    larger number (magnitude-wise). */

LABEL(_faddCheckFloat2ForZero)
        /* If we get here, then we know that float1 is not zero because
           _faddFloat1IsZero would have already taken care of it otherwise. */

        cmp     MANT2, #0               /* check to see if float2 is zero. */
	FLOAT_RETURN_TO_CALLER_IF(eq) 	/* If 0, return float1 in r0. */
        mov     EXP2, #1              /* Else, not zero.  Adjust EXP2 because */
        b       _faddFloat2IsNotZero    /*   it is implied to be 1. */

LABEL(_faddFloatCheckForNanOrInfinity)
        /* Check if float1 is NaN or infinity: */
        cmp     EXP1, #0xff             /* Check for infinity or nan. */
        beq     _faddFloat1CheckForNanOrInfinity

        /* Else, float2 is NaN or infinity: */
LABEL(_faddFloat2CheckForNanOrInfinity)
        /* If we get here, then we know that float1 is finite.  The result
           will be what ever is in float2 (i.e. infinity or NaN): */

LABEL(_faddReturnFloat2)
        mov     r0, FLOAT2              /* result = float2. */
        FLOAT_RETURN_TO_CALLER		/* Return the result. */

LABEL(_faddFloat1CheckForNanOrInfinity)
        cmp     MANT1, #0               /* Check if float1 is an infinity. */

        /* If not an infinity, then must be a NaN: */
        /* The return value in r0 is a NaN already because r0 contains the
           value of float1 which is a Nan. */
	FLOAT_RETURN_TO_CALLER_IF(ne) 	/* result = NaN + ? => NaN. */

        /* If we get here, than float1 is an infinity.  We must return check
           if float2 is a NaN or an infinity or finite: */
        cmp     EXP2, #0xff             /* Check if float2 is NaN or infinity. */

        /* If is finite (i.e. not NaN or infinity), then just return the
           infinity value in float1 which is already in r0: */
	FLOAT_RETURN_TO_CALLER_IF(ne) 	/* Return the result. */

        cmp     MANT2, #0               /* Check if float2 is an infinity. */

        /* If not an infinity, then must be a NaN: */
        movne   r0, FLOAT2              /* Return the NaN in float2. */
	FLOAT_RETURN_TO_CALLER_IF(ne) 	/* Return the result. */

        /* We have 2 infinities: */
        eor     r12, FLOAT1, FLOAT2   /* NOTE: MMASK<r12> is no longer needed. */
        tst     r12, #0x80000000        /* Check if the sign bit is the same. */

        /* If the sign bit is the same, then just return the infinity already
           in r0 (i.e. float1): */
	FLOAT_RETURN_TO_CALLER_IF(eq) 	/* Return the result. */

        /* Else, return a NaN: */
        ldr     r0, floatNaN            /* result = NaN (0x7f800000). */
	FLOAT_RETURN_TO_CALLER	 	/* Return the result. */
#endif
/* IAI - 16 */

/*
 * Entry point for multiplying floats.
 * NOTE: The result is in r0.
 */
	ENTRY(CVMCCMruntimeFMul)
ENTRY1 ( CVMCCMruntimeFMul )
	ENTRY(CVMCCMruntimeFMul_C)
ENTRY1 ( CVMCCMruntimeFMul_C )
        /* r0 = a1 = float1 
         * r1 = a2 = float2 
         * v1 = jfp 
         * v2 = jsp 
         * sp = ccee
	 */

/* IAI - 16 */
#ifdef IAI_WMMX_FMUL
#define FEXP1 r2
#define FEXP2 r3
#define FMANT1 ip
#define FMANT2 r1
#define SIGN  r3
        eor     SIGN, FLOAT1, FLOAT2            /* Positive if same, else negative. */
        tmcr    wCASF, SIGN                     /* move SIGN to WMMX register */

        mov     EMASK, #0xff000000              /* Setup the pre-shifted exponent mask. */
        ands    FEXP1, EMASK, FLOAT1, LSL #1    /* Extract exponent1. */
        beq     fmul_fexp1_equal_0
        ands    FEXP2, EMASK, FLOAT2, LSL #1    /* Extract exponent2. */
        beq     fmul_fexp1_not_equal_0_fexp2_equal_0

        cmp     FEXP1, EMASK
        cmpne   FEXP2, EMASK
        beq     fmul_fexp_equal_emask

        adds    FEXP1, FEXP1, FEXP2             /* exp1 = exp1 + exp2. */
        bcs     fmul_fexp_too_big
        bpl     fmul_result_denormal_pre1       /* result needs to be donormalized */

LABEL(fmul_result_normal)
        eor     FMANT2, FEXP2, FLOAT2, LSL #1   /* Extract mantissa1. */
        mov     FMANT1, FLOAT1, LSL #8          /* Extract mantissa2. */
        orr     FMANT2, FMANT2, #0x01000000
        orr     FMANT1, FMANT1, #0x80000000

        umull   r3, r0, FMANT1, FMANT2          /* {hi<r0>,lo<r3>} = FMANT1 * FMANT2. */

        sub     FEXP1, FEXP1, #0x7f000000       /* exp1 -= 127 (i.e. the bias). */
        cmp     FEXP1, #0xfe000000
        bhs     fmul_result_too_big

LABEL(fmul_process_result)
        tst     r0, #0x01000000
        addne   FEXP1, FEXP1, #0x01000000
        movnes  r0, r0, lsr #1
        moveqs  r3, r3, lsl #1
        adc     r0, r0, #0
        bcs     fmul_process_result_round       /* r3 = #0x8xxxxxxx */

LABEL(fmul_process_result_done)
        tst     r0, #0x01000000
        addne   FEXP1, FEXP1, #0x01000000

        textrcb R15, #7
        bic     r0, r0, #0x01800000
        orrmi   r0, r0, #0x80000000
        orr     r0, r0, FEXP1, lsr #1
        mov     pc, lr                          /* normalized result returned. */

LABEL(fmul_process_result_round)
        teq     r3, #0
        biceq   r0, r0, #1
        b       fmul_process_result_done

LABEL(fmul_fexp1_equal_0)
        movs    r3, FLOAT1, LSL #8
        beq     fmul_float1_equal_0

        clz     FEXP1, r3
        mov     FLOAT1, FLOAT1, LSL FEXP1
        rsb     FEXP1, FEXP1, #0
        mov     FEXP1, FEXP1, LSL #24           /* Normalize FLOAT1 and compute FEXP1 */

        ands    FEXP2, EMASK, FLOAT2, LSL #1    /* Extract exponent2. */
        beq     fmul_return_zero

        cmp     FEXP2, EMASK
        beq     fmul_float2_0_or_fexp2_emask

        adds    FEXP1, FEXP1, FEXP2             /* exp1 = exp1 + exp2. */
        bpl     fmul_result_denormal_pre2
        addcs   FEXP1, FEXP1, #0x01000000       /* exp1 = exp1 + 1 for normalized result */
        bcs     fmul_result_normal

LABEL(fmul_return_zero)
        textrcb R15, #7
        mov     r0, #0x0
        orrmi   r0, r0, #0x80000000
        mov     pc, lr

LABEL(fmul_float1_equal_0)
        and     FEXP2, EMASK, FLOAT2, LSL #1    /* Extract exponent2. */
        cmp     FEXP2, EMASK
        moveq   r0, #0x7f000000
        addeq   r0, r0, #0x00c00000             /* return NaN */
        andne   r1, r1, #0x80000000
        eorne   r0, FLOAT1, FLOAT2              @ return +/- 0
        mov     pc, lr

LABEL(fmul_fexp1_not_equal_0_fexp2_equal_0)
        cmp     FEXP1, EMASK
        beq     fmul_fexp1_emask_fexp2_0

        movs    r3, FLOAT2, LSL #8
        beq     fmul_float2_0_or_fexp2_emask

        clz     FEXP2, r3
        mov     FLOAT2, FLOAT2, LSL FEXP2

        subs    FEXP1, FEXP1, FEXP2, LSL #24    /* exp1 = exp1 + exp2. */
        bpl     fmul_result_denormal_pre3
        bcc     fmul_return_zero

LABEL(fmul_fexp1_not_equal_0_fexp2_equal_0_process)
        mov     FMANT1, FLOAT1, LSL #8          /* Extract mantissa2. */
        mov     FMANT2, FLOAT2, LSL #1
        orr     FMANT1, FMANT1, #0x80000000
        umull   r3, r0, FMANT1, FMANT2          /* {hi<r0>,lo<r3>} = FMANT1 * FMANT2. */
        sub     FEXP1, FEXP1, #0x7e000000       /* exp1 -= 126 (i.e. the bias). */
        b       fmul_process_result

LABEL(fmul_fexp1_emask_fexp2_0)
        movs    r3, FLOAT2, LSL #8
        moveq   r0, #0x7f000000
        addeq   r0, r0, #0x00c00000             /* return NaN */
        andne   r1, r1, #0x80000000             /* return +/- 0 */
        eorne   r0, r0, r1
        mov     pc, lr

LABEL(fmul_fexp_equal_emask)
        cmp     FEXP1, EMASK
        bne     fmul_float2_0_or_fexp2_emask
        cmp     FEXP2, EMASK
        beq     fmul_fexp1_and_fexp2_equal_emask

        and     r1, r1, #0x80000000
        eor     r0, r0, r1
        mov     pc, lr

LABEL(fmul_fexp1_and_fexp2_equal_emask)
        orr     FLOAT1, FLOAT1, FLOAT2
        textrcb R15, #7
        bicpl   r0, FLOAT1, #0x80000000         /* return +/- inf or NaN */
        mov     pc, lr


LABEL(fmul_float2_0_or_fexp2_emask)
        textrcb R15, #7
        bicpl   r0, FLOAT2, #0x80000000
        orrmi   r0, FLOAT2, #0x80000000         /* return +/- 0 or inf or NaN */
        mov     pc, lr

LABEL(fmul_result_denormal_pre3)
        cmp     FEXP1, #0x7f000000              /* special case */
        beq     fmul_fexp1_not_equal_0_fexp2_equal_0_process
        b       fmul_result_denormal

LABEL(fmul_result_denormal_pre2)
        cmp     FEXP1, #0x7f000000              /* special case */
        addeq   FEXP1, FEXP1, #0x01000000
        beq     fmul_result_normal
        b       fmul_result_denormal

LABEL(fmul_result_denormal_pre1)
        sub     FEXP1, FEXP1, #1

LABEL(fmul_result_denormal)
        subs    FEXP1, FEXP1,  #0x66000000      /* fexp1 < -25 */
        blt     fmul_return_zero

        mov     FMANT2, FLOAT2, LSL #8          /* Extract mantissa1. */
        mov     FMANT1, FLOAT1, LSL #8          /* Extract mantissa2. */
        mov     FMANT2, FMANT2, LSR #3
        mov     FMANT1, FMANT1, LSR #4
        orr     FMANT2, FMANT2, #0x10000000
        orr     FMANT1, FMANT1, #0x08000000

        mov     FEXP1, FEXP1, LSR #24
        rsb     FEXP1, FEXP1, #25
        tmcr    wCGR0, FEXP1
        textrcb R15, #7
        wzero   wR0
        tmia    wR0, FMANT1, FMANT2             /* {wR0} = FMANT1 * FMANT2. */
        wsrldg  wR0, wR0, wCGR0
        tmrrc   r3, r0, wR0
        movpl   r1, #0
        movmi   r1, #0x80000000
        cmp     r3, #0x80000000
        addhs   r0, r0, #1
        biceq   r0, r0, #1


        eor     r0, r0, r1
        mov     pc, lr

LABEL(fmul_result_too_big)
        tst     r0, #0x01000000                 /* if result has carry, return inf */
        bne     fmul_return_inf
        movs    r3, r3, lsl #1
        adc     r0, r0, #0
        bcs     fmul_result_too_big_round

LABEL(fmul_result_too_big_round_done)
        tst     r0, #0x01000000                 /* if result has carry, return inf */
        bne     fmul_return_inf

        textrcb R15, #7
        bic     r0, r0, #0x01800000
        orrmi   r0, r0, #0x80000000
        orr     r0, r0, FEXP1, lsr #1
        mov     pc, lr

LABEL(fmul_result_too_big_round)
        teq     r3, #0
        biceq   r0, r0, #1
        b       fmul_result_too_big_round_done

LABEL(fmul_fexp_too_big)
        cmp     FEXP1, #0x7e000000
        blo     fmul_result_normal

LABEL(fmul_return_inf)
        textrcb R15, #7
        mov     r0, #0x7f000000
        add     r0, r0, #0x00800000
        orrmi   r0, r0, #0x80000000
        mov     pc, lr
#undef FEXP1
#undef FEXP2
#undef FMANT1
#undef FMANT2
#undef SIGN

#else

        /* The following saves registers that we're going to use and extracts
           exponents and mantissa from the floats. */

        mov     EMASK, #0xff        /* Setup the pre-shifted exponent mask. */

	stmfd	sp!, FLOAT_SAVE_SET
        and     EXP1, EMASK, FLOAT1, LSR #23    /* Extract exponent1. */
        and     EXP2, EMASK, FLOAT2, LSR #23    /* Extract exponent2. */

        mvn     MMASK, #0x80000000
        and     MANT1, MMASK, FLOAT1, LSL #8    /* Extract mantissa1. */
        and     MANT2, MMASK, FLOAT2, LSL #8    /* Extract mantissa2. */

        /* NOTE: r12 is free now because MMASK is no longer needed. */
        /* Set the sign bit in r0 and free up r1: */
        eor     r0, r0, r1              /* Positive if same, else negative. */
        and     r0, r0, #0x80000000     /* Zero out the other bits. */

        /* Check if float1 or float2 is NaN or infinity: */
        cmp     EXP1, #0xff             /* Check for infinity or nan. */
        cmpne   EXP2, #0xff             /* Check for infinity or nan. */
        beq     _fmulFloatCheckForNaNOrInfinity

        /* Check float1 for 0 or denormalized number: */
        cmp    EXP1, #0
        beq     _fmulCheckFloat1ForZero
        orr     MANT1, MANT1, #0x80000000       /* Set the implied high bit. */
LABEL(_fmulFloat1IsNotZero)

        cmp     EXP2, #0
        beq     _fmulCheckFloat2ForZero
        orr     MANT2, MANT2, #0x80000000       /* Set the implied high bit. */
LABEL(_fmulFloat2IsNotZero)

        /* Now we're ready to do the multiplication.   First add the exponents
           together and store the result in EXP1.  EXP2 is free after that: */
        sub     EXP1, EXP1, #126        /* exp1 -= 127 (i.e. the bias) + 1. */
        add     EXP1, EXP1, EXP2        /* exp1 = exp1 + exp2. */

        /* After the multiplication, the resultant binary point will be 2
           digits from the high end of the 64 bit number as follows:

           result = [xx.xx xxxx xxxx xxxx xxxx xxxx 0000 0000]

           But we want the binary point to be 1 from the high end.  To do this
           we divide the number by 2 and add 1 to the exponent.  Well, we
           don't actually have to divide the number by 2.  We just know that
           the binary point is now 1 from the left.

           EXP1 is already incremented above.  See how the bias is being
           subtracted from it.
        */
        umull   r3, r1, MANT1, MANT2    /* {hi<r1>,lo<r3>} = MANT1 * MANT2. */

        /* NOTE: r6, r7, and r12 are now free again: */

        /* Normalize the result: */
        cmp     EXP1, #1
        blt     _fmulDoGradualUnderflow0

        /* Normalize the resultant mantissa if necessary: */
        movs    MANT1, r1               /* Put the mantissa in { r6, r3 } */
        bmi     _fmulCheckForStickyBit  /* If high bit is set (i.e. already */
                                        /*   normalized, then move on. */

LABEL(_fmulNormalizing)
        cmp     EXP1, #1                /* See if exponent is down to 1. */
        beq     _fmulCheckForStickyBit  /* Cannot normalize.  Go wrap-up. */

        /* Shift the 64bit number left by 1: */
        movs    r3, r3, LSL #1          /* Shift low-order word left by 1. */
        sub     EXP1, EXP1, #1          /* exponent--. */
        adcs    MANT1, MANT1, MANT1     /* Shift high-order word left by 1. */
        bpl     _fmulNormalizing        /* If high bit not set, continue. */
        /* Fall through to _fmulCheckForStickyBit. */
#endif
/* IAI - 16 */

LABEL(_fmulCheckForStickyBit)
        /* Is already normalized.  Check the sticky bit for rounding: */
        cmp     r3, #0
        orrne   MANT1, MANT1, #0x20     /* Set the sticky bit if necessary. */
        b       _floatRoundResult       /* Go round the result and exit. */

LABEL(_fmulDoGradualUnderflow0)
        mov     MANT1, r1               /* Put the mantissa in { r6, r3 } */
        /* Fall thru to _fmulDoGradualUnderflow: */
LABEL(_fmulDoGradualUnderflow)
        /* Compute the number of bits we have to shift right by in order to
           bring EXP1 up to 1: */
        rsb     r7, EXP1, #1
        cmp     r7, #31

        /* If we have to shift 31 or more bits, then the result must be an
           underflow to zero: */
	FLOAT_RETURN_TO_CALLER_IF(ge) 	/* result = sign | 0. */

        /* Else, shift right and let the rounding do gradual underflow: */
        mov     EXP1, #1
        orr     r3, r3, MANT1, LSL r7   /* Compute the sticky bit. */
        mov     MANT1, MANT1, LSR r7    /* Shift right be said bits. */
        b       _fmulCheckForStickyBit  /* Go do rounding for the result. */

/* IAI - 16 */
#ifndef IAI_WMMX_FMUL
LABEL(_fmulCheckFloat1ForZero)
        /* If we get here, then float1 and float2 are finite: */
        cmp     MANT1, #0               /* Check for 0. */
	FLOAT_RETURN_TO_CALLER_IF(eq) /* result = sign | 0. Return to caller. */
        mov     EXP1, #1                /* Adjust exponent for denormalized #. */
        b       _fmulFloat1IsNotZero

LABEL(_fmulCheckFloat2ForZero)
        /* If we get here, then float1 and float2 are finite: */
        cmp     MANT2, #0               /* Check for 0. */
	FLOAT_RETURN_TO_CALLER_IF(eq) /* result = sign | 0. Return to caller. */
        mov     EXP2, #1                /* Adjust exponent for denormalized #. */
        b       _fmulFloat2IsNotZero

LABEL(_fmulFloatCheckForNaNOrInfinity)
        /* Check if float1 is NaN or infinity: */
        cmp     EXP1, #0xff             /* Check for infinity or nan. */
        beq     _fmulFloat1CheckForNaNOrInfinity

        /* Else, float2 is NaN or infinity: */
LABEL(_fmulFloat2CheckForNaNOrInfinity)
        /* If we get here, then float1 is not nan nor infinity: */
        cmp     MANT2, #0               /* Check for infinity. */
        bne     _floatReturnNaN         /* finite * nan => nan. */

        /* Else, float2 is an infinity.  Check float1 for 0: */
        orrs    r12, EXP1, MANT1        /* Check for 0. */
        beq     _floatReturnNaN         /* 0 * inf => nan. */
        b       _floatReturnInfinity    /* finite * inf => inf. */

LABEL(_fmulFloat1CheckForNaNOrInfinity)
        cmp     MANT1, #0               /* Check for infinity. */
        bne     _floatReturnNaN         /* nan * ? => nan. */

        /* Else, float1 is an infinity.  Check float2 for nan or infinity: */
        cmp     EXP2, #0xff
        bne     _fmulFloat1CheckForZeroInFloat2
        cmp     MANT2, #0               /* Check for infinity. */
        bne     _floatReturnNaN         /* inf * nan => nan. */
        b       _floatReturnInfinity    /* inf * inf => inf. */

LABEL(_fmulFloat1CheckForZeroInFloat2)
        orrs    r12, EXP2, MANT2        /* Check for 0. */
        beq     _floatReturnNaN         /* inf * 0 => nan. */
        b       _floatReturnInfinity    /* inf * finite => inf. */
#endif
/* IAI - 16 */

/*
 * The following are common to FAdd, FSub, and FMul:
 */

LABEL(_floatRoundResult)
        /* NOTE: _floatRoundResult expects the sign in r0, the exponent in
                 EXP1 (i.e. r2), and the normalized mantissa in MANT1 (i.e.
                 r6) with the binary point between bit 31 and 30. */

        /* Round the mantissa using IEEE 754 round to nearest mode: */
        and     r12, MANT1, #0xff      /* Extract the low 8 bits for rounding. */
        cmp     r12, #0x80
        blt     _floatRoundingDone      /* Round down.  Nothing to do. */
        bgt     _floatRoundUp           /* Go round up. */

        /* Else round to the nearest 0 in the LSBit in the result mantissa: */
        tst     MANT1, #0x100           /* Check LSBit of result mantissa. */
        beq     _floatRoundingDone      /* If already 0, then done rounding. */
                                        /* Else, round up. */
LABEL(_floatRoundUp)
        adds    MANT1, MANT1, #0x80

        /* After rounding, we have to re-check if we're normalized, and re-
           normalize if we're not: */
        bcc     _floatRoundingDone

        /* If we get here, then the high bit was in the carry.  Move the carry
           back into the high bit and adjust the exponent accordingly: */
        mov     MANT1, MANT1, LSR #1
        orr     MANT1, MANT1, #0x80000000
        add     EXP1, EXP1, #1
        /* Fall through to _floatRoundingDone. */

LABEL(_floatRoundingDone)

        /* Now check for overflow to infinities: */
        cmp     EXP1, #255
        bge     _floatReturnInfinity

        /* Check to see if the result is a denormalized number: */
        tst     MANT1, #0x80000000

        /* If the number is denormalized, mark it as so: */
        moveq   EXP1, #0                    /* Indicate denormalized. */

        /* Only do the following 2 inst if result is normalized: */
        bicne   MANT1, MANT1, #0x80000000   /* Clear the top bit. */
        orrne   r0, r0, EXP1, LSL #23       /* Set the exponent. */

        mov     MANT1, MANT1, LSR #8        /* Set the mantissa. */
        orr     r0, r0, MANT1

        /* Fall through to _floatReturnToCaller. */

LABEL(_floatReturnToCaller)
        /* Restore the saved registers: */
	FLOAT_RETURN_TO_CALLER	 	/* Return to caller. */

LABEL(_floatReturnInfinity)
        mov     r1, #0xff
        orr     r0, r0, r1, LSL #23     /* result = sign | infinity. */
	FLOAT_RETURN_TO_CALLER	 	/* Return the result */

LABEL(_floatReturnNaN)
        ldr     r0, floatNaN            /* result = nan. */
	FLOAT_RETURN_TO_CALLER	 	/* Return the result */




/*
 * Entry point for dividing floats.
 * NOTE: The result is in r0.
 */
	ENTRY(CVMCCMruntimeFDiv)
ENTRY1 ( CVMCCMruntimeFDiv )
	ENTRY(CVMCCMruntimeFDiv_C)
ENTRY1 ( CVMCCMruntimeFDiv_C )
        /* r0 = a1 = float1 
         * r1 = a2 = float2 
         * v1 = jfp 
         * v2 = jsp 
         * sp = ccee
	 */

#define QUOT	r12
#define ITER	r3

        /* The following saves registers that we're going to use and extracts
           exponents and mantissa from the floats. */

	stmfd	sp!, FLOAT_SAVE_SET
        mov     EMASK, #0xff        /* Setup the pre-shifted exponent mask. */

        and     EXP1, EMASK, FLOAT1, LSR #23    /* Extract exponent1. */
        and     EXP2, EMASK, FLOAT2, LSR #23    /* Extract exponent2. */

	orr	EMASK, EMASK, #0x100
        bic     MANT1, FLOAT1, EMASK, LSL #23    /* Extract mantissa1. */
        bic     MANT2, FLOAT2, EMASK, LSL #23    /* Extract mantissa2. */

        /* Set the sign bit in r0 and free up r1: */
        eor     r0, r0, r1              /* Positive if same, else negative. */
        and     r0, r0, #0x80000000     /* Zero out the other bits. */

        /* Check if float1 or float2 is NaN or infinity: */
        cmp     EXP1, #0xff             /* Check for infinity or nan. */
        cmpne   EXP2, #0xff             /* Check for infinity or nan. */
        beq     _fdivFloatCheckForNaNOrInfinity

        /* Check float1 for 0 or denormalized number: */
        cmp    EXP1, #0
        beq     _fdivCheckFloat1ForZero
        orr     MANT1, MANT1, #0x800000       /* Set the implied high bit. */
LABEL(_fdivFloat1IsNotZero)
        cmp     EXP2, #0
        beq     _fdivCheckFloat2ForZero
        orr     MANT2, MANT2, #0x800000       /* Set the implied high bit. */
LABEL(_fdivFloat2IsNotZero)

	/*
         * Now we're ready to do the division.   First subtract the exponents
         * and store the result in EXP1.  EXP2 is free after that.
	 * Ensure that the first digit of the quotient will be a '1'
	 * by shifting MANT1 if necessary. Decrease exponent accordingly.
	 *
	 * Do the division as 25 successive divide steps
	 * of MANT1 by MANT2, developing the quotient in QUOT.
	 */
        sub     EXP1, EXP1, EXP2        /* exp1 = exp1 - exp2. */
        add     EXP1, EXP1, #127        /* exp1 += 127 (i.e. the bias) */
	cmp	MANT1, MANT2
	movlt	MANT1, MANT1, LSL #1
	sublt	EXP1,  EXP1, #1

	mov	ITER, #25
	mov	QUOT, #0
	/* loop unrolled by a factor of 5 */
LABEL(_fdivIterationLoopTop)
	cmp	MANT1, MANT2
	subhs	MANT1, MANT1, MANT2
	adc	QUOT, QUOT, QUOT
	mov	MANT1, MANT1, LSL #1
	cmp	MANT1, MANT2
	subhs	MANT1, MANT1, MANT2
	adc	QUOT, QUOT, QUOT
	mov	MANT1, MANT1, LSL #1
	cmp	MANT1, MANT2
	subhs	MANT1, MANT1, MANT2
	adc	QUOT, QUOT, QUOT
	mov	MANT1, MANT1, LSL #1
	cmp	MANT1, MANT2
	subhs	MANT1, MANT1, MANT2
	adc	QUOT, QUOT, QUOT
	mov	MANT1, MANT1, LSL #1
	cmp	MANT1, MANT2
	subhs	MANT1, MANT1, MANT2
	adc	QUOT, QUOT, QUOT
	mov	MANT1, MANT1, LSL #1
	subs	ITER, ITER, #5
	bgt	_fdivIterationLoopTop

	/* adjust remainder: only for non-restoring. */
	/* cmp	MANT1, #0 */
	/* addlt	MANT1, MANT1, MANT2 */

	/*
	 * In order to share rounding and packing code with fmul,
	 * move QUOT to MANT1, shifting left s.t. the high-order bit is set.
	 * Also insert the sticky bit.
	 */
	cmp	MANT1, #0
	mov	MANT1, QUOT, LSL #7
	orrne	MANT1, MANT1, #20

        cmp     EXP1, #1
        bge     _floatRoundResult       /* Go round the result and exit. */
        b       _fmulDoGradualUnderflow

LABEL(_fdivFloatCheckForNaNOrInfinity)
        /* Check if float1 is NaN or infinity: */
        cmp     EXP1, #0xff             /* Check for infinity or nan. */
        beq     _fdivFloat1CheckForNaNOrInfinity

        /* Else, float2 is NaN or infinity: */
LABEL(_fdivFloat2CheckForNaNOrInfinity)
        /* If we get here, then float1 is not nan nor infinity: */
        cmp     MANT2, #0               /* Check for infinity. */
        bne     _floatReturnNaN         /* finite / nan => nan. */
        /* Else, float2 is an infinity.  Result is 0 */
	FLOAT_RETURN_TO_CALLER	 	/* Return the result. */

LABEL(_fdivFloat1CheckForNaNOrInfinity)
        cmp     MANT1, #0               /* Check for infinity. */
        bne     _floatReturnNaN         /* nan / ? => nan. */

        /* Else, float1 is an infinity.  Check float2 for nan or infinity: */
        cmp     EXP2, #0xff
        bne     _floatReturnInfinity	/* inf / finite => inf */
        b       _floatReturnNaN         /* inf / {nan or inf} => nan. */

#ifndef __RVCT__
#define FLOAT_NORMALIZE_LOOP( _MANT, _EXP ) \
        mov     _EXP, #1;		\
1:					\
	mov	_MANT, _MANT, LSL #1;	\
	sub	_EXP, _EXP, #1;		\
	cmp	_MANT, #0x800000;	\
	blt	1b
#else
	MACRO
	FLOAT_NORMALIZE_LOOP0 $_MANT, $_EXP
	mov     $_EXP, #1
1
	mov	$_MANT, $_MANT, LSL #1
	sub	$_EXP, $_EXP, #1
	cmp	$_MANT, #0x800000
	blt	%b1
	MEND

#define FLOAT_NORMALIZE_LOOP( _MANT, _EXP ) \
	FLOAT_NORMALIZE_LOOP0 _MANT, _EXP
#endif

LABEL(_fdivCheckFloat1ForZero)
        /* If we get here, then float1 and float2 are finite: */
        cmp     MANT1, #0               /* Check for 0. */
        beq     _fdivFloat1IsZero
	FLOAT_NORMALIZE_LOOP( MANT1, EXP1 )
        b       _fdivFloat1IsNotZero

LABEL(_fdivFloat1IsZero)
	cmp	EXP2, #0
	cmpeq	MANT2, #0
	FLOAT_RETURN_TO_CALLER_IF(ne) /* result = sign | 0. Return to caller. */
	b	_floatReturnNaN		/* 0 / 0 => NaN */

LABEL(_fdivCheckFloat2ForZero)
        /* If we get here, then float1 is finite and non-zero: */
        cmp     MANT2, #0               /* Check for 0. */
        beq     _floatReturnInfinity
	FLOAT_NORMALIZE_LOOP( MANT2, EXP2 )
        b       _fdivFloat2IsNotZero

#undef FLOAT1
#undef FLOAT2
#undef EXP1
#undef EXP2
#undef MANT1
#undef MANT2
#undef EMASK
#undef MMASK
#undef ITER
#undef QUOT

/*
 * Entry point for converting floats to doubles.
 * NOTE: The result is in FLOAT (high word) and MANT (low word).
 */
	ENTRY(CVMCCMruntimeF2D)
ENTRY1 ( CVMCCMruntimeF2D )
	ENTRY(CVMCCMruntimeF2D_C)
ENTRY1 ( CVMCCMruntimeF2D_C )
        /* r0 = a1 = float1 
         * v1 = jfp  
         * v2 = jsp 
         * sp = ccee
	 */

#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define FLOAT   r0
#define MANT    r1
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define FLOAT   r1
#define MANT    r0
#endif
#define EXP     r2
#define EMASK   r3
#define MMASK   r3

        ldr     MMASK, floatMantissaMask
#if CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
        mov     FLOAT, r0
#endif
        and     MANT, FLOAT, MMASK          /* Extract the mantissa. */
        mov     EMASK, #0xff
        and     EXP, FLOAT, EMASK, LSL #23  /* Extract the exponent. */

        /* Check for NaNs or infinities: */
        cmp     EXP, EMASK, LSL #23
        beq     _f2dHandleNaNOrInfinity     /* Go handle the nan or infinity. */

        /* Prepare the mantissa for conversion: */
        mov     MANT, MANT, LSL #9          /* Remove the leading zeroes. */

        cmp     EXP, #0                    /* Check for a denormalized number. */
        beq     _f2dHandleDenormalized

        /* Set the new exponent: */
        mov     EXP, EXP, LSR #3            /* The new exponent is 11 bits. */
        add     EXP, EXP, #0x38000000       /* Add the difference in the bias. */

LABEL(_f2dSetMantissaAndSignAndReturn)
        /* Set the new mantissa: */
        orr     EXP, EXP, MANT, LSR #12     /* Mask in the mantissa high bits. */
        mov     MANT, MANT, LSL #20         /* Set the low bits. */

LABEL(_f2dSetSignAndReturn)
        /* Set the sign of the result: */
        and     FLOAT, FLOAT, #0x80000000   /* Copy the sign. */
        orr     FLOAT, FLOAT, EXP           /* Add the exponent and high mant. */
        mov     pc, lr                      /* Return to caller. */

LABEL(_f2dHandleNaNOrInfinity)
        ldr     EXP, doubleExponentMask     /* Set the special exponent. */
        /* Note: If the mantissa is 0, then we have an infinity which requires
           that we return 0 in MANT.  If the mantissa is not zero, then we
	   have a NaN which requires a non zero mantissa which we already
	   have.  Hence, there is nothing to do. */
        b       _f2dSetSignAndReturn        /* Return to caller. */

LABEL(_f2dHandleDenormalized)
        cmp     MANT, #0                    /* Check if float is 0. */

        /* If the mantissa is 0, then we have a 0 float which requires 0 in
           MANT. */
        beq     _f2dSetSignAndReturn        /* Return to caller. */

        /* If we get here, then there must be a non zero bit somewhere in the
           mantissa.  Just shift left until we get it normalized.  Not that
           we also don't have to check for underflow because every non-zero
           finite float number can be expressed as a normalized double. */

        /* Convert the exponent.
           NOTE: float exponent 1 = double exponent 1 - 127 + 1023 = 897
                                  = 0x381 = 0x380 + 1: */
        mov     EXP, #0x38000000
        add     EXP, EXP, #0x00100000

        /* Normalize the mantissa: */
LABEL(_f2dNormalizing)
        sub     EXP, EXP, #0x00100000
        movs    MANT, MANT, LSL #1  /* Shift left until high bit is seen. */
        bcc     _f2dNormalizing     /* High bit not seen yet, continue. */
        b       _f2dSetMantissaAndSignAndReturn

#undef FLOAT
#undef EXP
#undef EMASK
#undef MMASK
#undef MANT

/*
 * Entry point for comparing doubles.
 * NOTE: The result is in the condition codes that have been set by various
 *       comparisons. Even though the C prototype for this helper indicates
 *       that the helper is to return an integer status in a register, this
 *       assembly version returns the result in the CPU condition code
 *       register (thus effectively making the return type of the function
 *       void).  This is OK because the ARM specific rules which emits calls
 *       to this helper function will be expecting the result to be in the
 *       condition code register.  This method of returning the result is
 *       done for optimization purposes.
 */
	ENTRY(CVMCCMruntimeDCmpg)
ENTRY1 ( CVMCCMruntimeDCmpg )
	ENTRY(CVMCCMruntimeDCmpg_C)
ENTRY1 ( CVMCCMruntimeDCmpg_C )
        /* r0 = a1 = double1 high 
         * r1 = a2 = double1 low 
         * r2 = a3 = double2 high 
         * r3 = a4 = double2 low 
         * v1 = jfp  
         * v2 = jsp 
         * sp = ccee 
	 */

        orr     lr, lr, #0x2            /* NaN returns 'greater than' status.*/
        /* Fall through to _dcmpStart: */

	ENTRY(CVMCCMruntimeDCmpl)
ENTRY1 ( CVMCCMruntimeDCmpl )
	ENTRY(CVMCCMruntimeDCmpl_C)
ENTRY1 ( CVMCCMruntimeDCmpl_C )
        /* r0 = a1 = double1 high 
         * r1 = a2 = double1 low 
         * r2 = a3 = double2 high 
         * r3 = a4 = double2 low 
         * v1 = jfp  
         * v2 = jsp 
         * sp = ccee
	 */

        /* NaN returns 'less than' status. */

        /* uses r0, r1, r2, r3, r12 */

LABEL(_dcmpStart)

/* Registers definition for different endianness. */
#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define DBL1LO  r1
#define DBL1HI  r0
#define DBL2LO  r3
#define DBL2HI  r2
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define DBL1LO  r0
#define DBL1HI  r1
#define DBL2LO  r2
#define DBL2HI  r3
#endif

        /* Check for NaN in double1 and double2.  Note that the double value
           can only be a NaN if the exponent value is 0x7ff00000.  This means
           that adding 0x00100000 to it will result in a negative value i.e.
           0x80000000.  We can check for this without using yet another
           register: */

        /* Check double1 for a NaN: */
        ldr     r12, doubleExponentMask /* load the exponent mask. */
        and     r12, DBL1HI, r12        /* Extract exponent1. */
        adds    r12, r12, #0x00100000   /* Check for infinity or nan. */
        bmi     _dcmpDouble1CheckForNan
LABEL(_dcmpDouble1IsNotNan)

        /* Check double2 for a NaN: */
        ldr     r12, doubleExponentMask /* load the exponent mask. */
        and     r12, DBL2HI, r12        /* Extract exponent2. */
        adds    r12, r12, #0x00100000   /* Check for infinity or nan. */
        bmi     _dcmpDouble2CheckForNan
LABEL(_dcmpDouble2IsNotNan)

        /* Check if the 2 doubles have the same sign bit: */
        eor     r12, DBL1HI, DBL2HI     /* Check to see if the sign bit is the */
        tst     r12, #0x80000000        /*    same. */
        bne     _dcmpEvaluateResultBasedOnSigns

        /* If we get here, then the sign bits are the same.  Next, check if
           the sign bits are negative: */
        tst     DBL1HI, #0x80000000         /* Check for negative sign bit. */
        bic     DBL1HI, DBL1HI, #0x80000000 /* Clear the sign bit. */
        bic     DBL2HI, DBL2HI, #0x80000000 /* Clear the sign bit. */
        bne     _dcmpDoNegativeChecks

        /* Do positive version of checks: */

        /* Compare high word: */
        cmp     DBL1HI, DBL2HI
        bne     _dcmpReturnToCaller     /* If not equal, we have our result. */

        /* Compare the mantissas: */
        eors    r12, DBL1LO, DBL2LO
        bmi     _dcmpPositiveRevLowTest
        cmp     DBL1LO, DBL2LO
        b       _dcmpReturnToCaller
LABEL(_dcmpPositiveRevLowTest)
        cmp     DBL2LO, DBL1LO
        /* Fall thru to _dcmpReturnToCaller. */

LABEL(_dcmpReturnToCaller)
        bic     lr, lr, #0x3            /* Clear the low bits set above. */
        mov     pc, lr                  /* Return to the caller. */

LABEL(_dcmpDoNegativeChecks)
        /* Do negative version of checks: */

        /* Compare high word: */
        cmp     DBL2HI, DBL1HI
        bne     _dcmpReturnToCaller     /* If not equal, we have our result. */

        /* Compare the mantissa upper low bits: */
        mov     r12, DBL2LO, LSR #16
        cmp     r12, DBL1LO, LSR #16
        bne     _dcmpReturnToCaller     /* If not equal, we have our answer. */

        /* Compare the mantissa lower low bits: */
        mvn     r12, #0                         /* r12 = 0xffffffff. */
        and     DBL1LO, DBL1LO, r12, LSR #16    /* mask with 0xffff. */
        and     DBL2LO, DBL2LO, r12, LSR #16    /* mask with 0xffff. */
        cmp     DBL2LO, DBL1LO
        b       _dcmpReturnToCaller             /* Return the result. */

LABEL(_dcmpEvaluateResultBasedOnSigns)
        /* Check for the special case where the 2 numbers are zeroes.  If so,
           the two are still equal even if their signs are not: */

        orrs    r12, DBL1LO, DBL1HI, LSL #1     /* See if double1 is 0. */
        bne     _dcmpNotBothZeroes
        orrs    r12, DBL2LO, DBL2HI, LSL #1     /* See if double2 is 0. */
        beq     _dcmpReturnToCaller             /* Return the result. */
LABEL(_dcmpNotBothZeroes)
        cmp     DBL1HI, DBL2HI          /* Compare the signs. */
        b       _dcmpReturnToCaller     /* Return the result. */

LABEL(_dcmpDouble1CheckForNan)
        /* Check if the mantissa is 0 i.e. the double is an infinity: */
        orrs    r12, DBL1LO, DBL1HI, LSL #12
        beq     _dcmpDouble1IsNotNan   /* If is infinity, resume in main code. */
        b       _dcmpNanFound

LABEL(_dcmpDouble2CheckForNan)
        /* Check if the mantissa is 0 i.e. the double is an infinity: */
        orrs    r12, DBL2LO, DBL2HI, LSL #12
        beq     _dcmpDouble2IsNotNan   /* If is infinity, resume in main code. */

LABEL(_dcmpNanFound)
        /* The return value for NaN is encoded in the low 2 bits of the lr
           which is normally 0: */
        and     r12, lr, #0x3           /* Else, is NaN.  Return the desired */
        cmp     r12, #1                 /*   condition code result. */
        b       _dcmpReturnToCaller     

#undef DBL1LO
#undef DBL1HI
#undef DBL1LO
#undef DBL1HI

	ENTRY(CVMCCMruntimeF2I)
ENTRY1 ( CVMCCMruntimeF2I )
	ENTRY(CVMCCMruntimeF2I_C)
ENTRY1 ( CVMCCMruntimeF2I_C )
	/*
	 * Keep the original argument in r0 (it has the sign)
	 * Working registers are F (Fraction) and  EXP (exponent)
	 */
#define F	r1
#define EXP	r2
	bic	F, r0, #0x80000000 	/* Strip sign */
	subs	EXP, F, #0x3f800000	/* De-bias exponent */
	blt	_f2iTooLittle		/* If exponent small, number is < 1 */
	cmp	EXP, #(31<<23)		/* If debiased exponent is > 31 */
	bhs	_f2iTooBig		/* ...then the number is > max int. */
	mov	F, F, LSL #8		/* Shift fraction far to the left */
	orr	F, F, #0x80000000	/* ...& insert implicit high order bit */
	mov	EXP, EXP, LSR #23	/* Shift the exponent down. */
	/* now need to shift F right by 31 - exponent */
	/* we know that 31 > EXP >= 0 */
	rsb	EXP, EXP, #31
	mov	F, F, LSR EXP
	/* apply the sign and return */
	cmp	r0, #0
	rsblt	F, F, #0
	mov	r0, F
	mov	pc, lr
	
LABEL(_f2iTooBig)
	/* test for Nan, which returns a zero */
	cmp	EXP, #(0x7f800000-0x3f800000)	/* (comparing after de-biasing) */
	bhi	_f2iTooLittle
	ands	r0, r0, #0x80000000	/* check the original sign */
	mvnpl	r0, #0x80000000		/* deliver maxint or minint accordingly */
	mov	pc, lr
LABEL(_f2iTooLittle)
	mov	r0, #0
	mov	pc, lr
#undef EXP
#undef F

	ENTRY(CVMCCMruntimeI2F)
ENTRY1( CVMCCMruntimeI2F )
	ENTRY(CVMCCMruntimeI2F_C)
ENTRY1( CVMCCMruntimeI2F_C )
	/*
	 * Keep the original argument in r0 (it has the sign)
	 * Working registers are F (Fraction) and  EXP (exponent)
	 */
#define F	r1
#define EXP	r2
	adds	F, r0, #0	/* absolute value to F */
	moveq	pc, lr		/* if zero, return zero */
	rsblt	F, F, #0
	/*
	 * 31 is the maximum number of shifts required to get
	 * a '1' in the high-order bit of F.
	 * 0x7f is the exponent of 1.0 (i.e. the bias).
	 * -1 is to account for the implied high-order bit. Rather
	 * than masking it out, we just subtract it out here.
	 */
	mov	EXP, #(31+0x7f-1)

	/*
	 * use shifts to get the high-order bit set.
	 * diminish EXP by the amount shifted.
	 */
	/* get bits into high-order 16 bits */
	cmp	F, #0x10000
	movlo	F, F, LSL #16
	sublo	EXP, EXP, #16
	/* get bits into high-order 8 bits */
	cmp	F, #0x1000000
	movlo	F, F, LSL #8
	sublo	EXP, EXP, #8
	/* get bits into high-order 4 bits */
	cmp	F, #0x10000000
	movlo	F, F, LSL #4
	sublo	EXP, EXP, #4
	/* get bits into high-order 2 bits */
	cmp	F, #0x40000000
	movlo	F, F, LSL #2
	sublo	EXP, EXP, #2
	/* get bit into high-order bit */
	cmp	F, #0x80000000
	movlo	F, F, LSL #1
	sublo	EXP, EXP, #1
	
	/* insert sign and exponent */
	and	r0, r0, #0x80000000	/* get the original sign */
	add	r0, r0, EXP, LSL #23	/* insert the exponent */
	add	r0, r0, F, LSR #8	/* insert the fraction */
	/* rounding is necessary if any bits were lost */
	ands	F, F, #0xff
	moveq	pc, lr			/* no bits lost (most likely case) */
	cmp	F, #0x80
	addhs	r0, r0, #1	/* round up ... */
	biceq	r0, r0, #1	/* ... or to even in case of 1/2 LSB */
	mov	pc, lr		/* return */
#undef EXP
#undef F

	ENTRY(CVMCCMruntimeI2D)
ENTRY1( CVMCCMruntimeI2D )
	ENTRY(CVMCCMruntimeI2D_C)
ENTRY1( CVMCCMruntimeI2D_C )
	/*
	 * Keep the original argument in H (it has the sign)
	 * Working registers are F (Fraction) and  EXP (exponent)
	 */
#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define H	r0
#define F	r1
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define H	r1
#define F	r0
#endif
#define EXP	r2

#if CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
	mov     H, r0
#endif
	adds	F, H, #0	/* absolute value to F */
	moveq	F, #0		/* if zero, return zero */
	moveq	pc, lr		/* if zero, return zero */
	rsblt	F, F, #0
	/*
	 * 31 is the maximum number of shifts required to get
	 * a '1' in the high-order bit of F.
	 * 0x3ff is the exponent of 1.0 (i.e. the bias).
	 * -1 is to account for the implied high-order bit. Rather
	 * than masking it out, we just subtract it out here.
	 */
	ldr	EXP, L_EXP_CONSTANT

	/*
	 * use shifts to get the high-order bit set.
	 * diminish EXP by the amount shifted.
	 */
	/* get bits into high-order 16 bits */
	cmp	F, #0x10000
	movlo	F, F, LSL #16
	sublo	EXP, EXP, #16
	/* get bits into high-order 8 bits */
	cmp	F, #0x1000000
	movlo	F, F, LSL #8
	sublo	EXP, EXP, #8
	/* get bits into high-order 4 bits */
	cmp	F, #0x10000000
	movlo	F, F, LSL #4
	sublo	EXP, EXP, #4
	/* get bits into high-order 2 bits */
	cmp	F, #0x40000000
	movlo	F, F, LSL #2
	sublo	EXP, EXP, #2
	/* get bit into high-order bit */
	cmp	F, #0x80000000
	movlo	F, F, LSL #1
	sublo	EXP, EXP, #1
	
	/* insert sign and exponent */
	and	H, H, #0x80000000	/* get the original sign */
	add	H, H, EXP, LSL #20	/* insert exponent */
	add	H, H, F, LSR #11	/* insert high 21 bits of the fraction */
	mov	F, F, LSL #21		/* low bits of the fraction here */
	mov	pc, lr			/* no rounding, so just go */

#undef H
#undef F
#undef EXP

	ENTRY(CVMCCMruntimeD2I)
ENTRY1( CVMCCMruntimeD2I )
	ENTRY(CVMCCMruntimeD2I_C)
ENTRY1( CVMCCMruntimeD2I_C )
	/*
	 * Keep the original argument in H/L (it has the sign)
	 * Working registers are F (Fraction) and  EXP (exponent)
	 */
#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define H       r0
#define L       r1
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define H       r1
#define L       r0
#endif
#define F	r2
#define EXP	r3

	bic	F, H, #0x80000000	/* strip sign */
	subs	EXP, F,   #0x3f000000	/* de-bias exponent */
	subs	EXP, EXP, #0x00f00000
	blt	_d2iTooLittle
	cmp	EXP, #(31<<20)
	bhs	_d2iTooBig
	/* shift fraction up to put high-order explicit bit in bit 30 */
	mov	F, F, LSL #11
	orr	F, F, L, LSR #21
	orr	F, F, #0x80000000	/* insert implicit high order bit */
	mov	EXP, EXP, LSR #20
	/* now need to shift F right by 31 - exponent */
	rsb	EXP, EXP, #31
	mov	F, F, LSR EXP
	/* apply the sign and return */
	cmp	H, #0
	rsblt	F, F, #0
	mov	r0, F
	mov	pc, lr
	
LABEL(_d2iTooBig)
	/* test for Nan, which is returns a zero */
	cmp	EXP, #(0x7ff00000-0x3ff00000)	
	cmpeq	L,  #0		/* any non-0 fraction bits indicate NaN */
	bhi	_d2iTooLittle
	cmp	H, #0
	mov	r0, #0x80000000 /* largest negative */
	mvngt	r0, r0		/* is the complement of the largest positive */
	mov	pc, lr
LABEL(_d2iTooLittle)
	mov	r0, #0
	mov	pc, lr
#undef H
#undef L
#undef F
#undef EXP


/*
 * Definitions and conventions shared by double-precision multiply 
 * and add/subtract.
 * Operands arrive in A1/A2 and B1/B2,
 * Result is returned in A1/A2.
 * The exponent of A and B are unpacked into EXPA and EXPB, respectively.
 * The result exponent is developed in EXPA.
 * FLAGS store various state bits. The sign bit of FLAGS is the
 * sign of the result.
 * EXPMASK contains the value DOUBLE_EXPVAL, which is both a mask and 
 * the exponent for infinity.
 *
 * When the shared rounding and packing code is entered, at _double_check_guard,
 * the resulting fraction has been moved into position in A1/A2,
 * the resulting exponent is in EXPA, and the guard bit is the high order bit
 * of RESULTX. The other bits of that register are the
 * sticky bits which will help determine rounding direction.
 * The fraction and exponent have the following relation at this point: either
 *   - the implicit bit is explicitly present (in what is normally the 
 *	low-order bit of the exponent OR
 *   - the exponent is incremented by 1.
 */
#define DOUBLE_SAVE_SET 	{r4-r9, lr}
#define DOUBLE_RESTORE_SET	{r4-r9, pc}

/* Registers definition for different endianness. */
#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define A1	r0
#define A2	r1
#define B1	r2
#define B2	r3
#define EXPA	r4
#define EXPB	r5
#define RESULTX	r6
#define EXPMASK	r7
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define A1	r1
#define A2	r0
#define B1	r3
#define B2	r2
#define EXPA	r5
#define EXPB	r4
#define RESULTX	r7
#define EXPMASK	r6
#endif

#define FLAGS	lr
#define EXPSHIFT 20
#define DOUBLE_EXPVAL	0x7ff

/*
 * The macros used for unpacking the two operands
 * have three parts. This allows us to place the less usual
 * code for dealing with denormalized values and NaNs out of line,
 * rather than disrupting the flow of normal computation.
 */

#ifndef __RVCT__
#define DOUBLE_UNPACK( HiSrc, ExpReg, DenormalDest, ExceptionalDest )\
	ands	ExpReg, EXPMASK, HiSrc, LSR #EXPSHIFT; \
	bic	HiSrc, HiSrc, EXPMASK, LSL #EXPSHIFT; \
	beq	DenormalDest; \
	cmp	ExpReg, EXPMASK; \
	beq	ExceptionalDest; \
	orr	HiSrc, HiSrc, #(1<<EXPSHIFT)

#define DOUBLE_EXCEPTIONAL( HiSrc, LoSrc, Infflag, Lreturn )\
	cmp	HiSrc, #0; \
	cmpeq	LoSrc, #0; \
	bne	_double_deliver_NaN; \
	orr	FLAGS, FLAGS, Infflag; \
	b	Lreturn


#define DOUBLE_NORMALIZE( HiSrc, LoSrc, ExpReg, Lreturn, Zflag )\
	cmp	HiSrc, #0; \
	cmpeq	LoSrc, #0; \
	orreq	FLAGS, FLAGS, Zflag; \
	beq	Lreturn; \
    1: \
	adds	LoSrc, LoSrc, LoSrc; \
	adc	HiSrc, HiSrc, HiSrc; \
	cmp	HiSrc, #(1<<EXPSHIFT);\
	sublt	ExpReg, ExpReg, #1; \
	blt	1b; \
	b	Lreturn
#else
	MACRO
	DOUBLE_UNPACK0 $HiSrc, $ExpReg, $DenormalDest, $ExceptionalDest
	ands	$ExpReg, EXPMASK, $HiSrc, LSR #EXPSHIFT
	bic	$HiSrc, $HiSrc, EXPMASK, LSL #EXPSHIFT
	beq	$DenormalDest
	cmp	$ExpReg, EXPMASK
	beq	$ExceptionalDest
	orr	$HiSrc, $HiSrc, #(1<<EXPSHIFT)
	MEND

	MACRO
	DOUBLE_EXCEPTIONAL0 $HiSrc, $LoSrc, $Infflag, $Lreturn
	cmp	$HiSrc, #0
	cmpeq	$LoSrc, #0
	bne	_double_deliver_NaN
	orr	FLAGS, FLAGS, $Infflag
	b	$Lreturn
	MEND

	MACRO
	DOUBLE_NORMALIZE0 $HiSrc, $LoSrc, $ExpReg, $Lreturn, $Zflag
	cmp	$HiSrc, #0
	cmpeq	$LoSrc, #0
	orreq	FLAGS, FLAGS, $Zflag
	beq	$Lreturn
1
	adds	$LoSrc, $LoSrc, $LoSrc
	adc	$HiSrc, $HiSrc, $HiSrc
	cmp	$HiSrc, #(1<<EXPSHIFT)
	sublt	$ExpReg, $ExpReg, #1
	blt	%b1
	b	$Lreturn
	MEND

#define DOUBLE_UNPACK( HiSrc, ExpReg, DenormalDest, ExceptionalDest )\
	DOUBLE_UNPACK0 HiSrc, ExpReg, DenormalDest, ExceptionalDest

#define DOUBLE_EXCEPTIONAL( HiSrc, LoSrc, Infflag, Lreturn )\
	DOUBLE_EXCEPTIONAL0 HiSrc, LoSrc, Infflag, Lreturn

#define DOUBLE_NORMALIZE( HiSrc, LoSrc, ExpReg, Lreturn, Zflag )\
	DOUBLE_NORMALIZE0 HiSrc, LoSrc, ExpReg, Lreturn, Zflag
#endif

/*
 * Multiplication and addition/subtraction treat denormalized numbers
 * differently.  For multiply we want to normalize by shifting.
 * For addition, we just leave the exponent at zero and refrain from 
 * inserting an implicit bit, because there is none.
 */

/*
 * Entry point for double precision floating multiplication.
 * On entry, multiplicand (call it A) is in r0/r1,
 * and multiplier (call it B) is in r2/r3.
 * On exit, product will be in r0/r1.
 */

	ENTRY(CVMCCMruntimeDMul)
ENTRY1( CVMCCMruntimeDMul )
	ENTRY(CVMCCMruntimeDMul_C)
ENTRY1( CVMCCMruntimeDMul_C )

/* IAI-06 */
#ifdef IAI_DMUL

#define SIGN_FLAG   r8
#define PROD2       r8
#define PROD3       ip

LABEL(_dmul_judge_A_zero)
        orrs        ip,         A2,         A1,         LSL #1      
        bne         _dmul_judge_A_INF_NaN   

LABEL(_dmul_A_equal_zero)
        mov         ip,         B1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        beq         _dmul_A_equal_zero_B_equal_INF_or_NaN           

LABEL(_dmul_A_equal_zero_B_NOT_equal_INF_NaN)
LABEL(_dmul_A_equal_INF_B_NOT_equal_zero_INF_NaN)
LABEL(_dmul_A_equal_INF_B_equal_INF)
        and         B1,         B1,         #0x80000000 /* get sign of B */
        eor         A1,         A1,         B1          /* deliver value of A */
        mov         pc,         lr          /* deliver sign of (A eor B) */
        
LABEL(_dmul_judge_A_INF_NaN)
        mov         ip,         A1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        bne         _dmul_judge_B_zero      

LABEL(_dmul_A_equal_INF_NaN)
        orrs        ip,         A2,         A1,         LSL #12     
        bne         _dmul_A_equal_NaN       
        orrs        ip,         B2,         B1,         LSL #1      
        beq         _dmul_A_equal_INF_B_equal_zero      
        mov         ip,         B1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        bne         _dmul_A_equal_INF_B_NOT_equal_zero_INF_NaN      
        orrs        ip,         B2,         B1,         LSL #12     
        beq         _dmul_A_equal_INF_B_equal_INF       
        b           _dmul_A_equal_INF_B_equal_NaN       
        
LABEL(_dmul_A_NOT_equal_zeor_A_NOT_equal_INF_NaN)
LABEL(_dmul_judge_B_zero)
        orrs        ip,         B2,         B1,         LSL #1      
        bne         _dmul_judge_B_INF_NaN   

LABEL(_dmul_A_normal_B_equal_zero)
LABEL(_dmul_A_normal_B_equal_INF)
        and         A1,         A1,         #0x80000000 /* get sign of A */
        eor         A1,         A1,         B1          /* deliver value of B */
        mov         A2,         #0          /*deliver sign of (A eor B) */
        mov         pc,         lr          

LABEL(_dmul_judge_B_INF_NaN)
        mov         ip,         B1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        bne         _dmul_normal_process    

LABEL(_dmul_B_equal_INF_NaN)
        orrs        ip,         B2,         B1,         LSL #12     
        beq         _dmul_A_normal_B_equal_INF          

LABEL(_dmul_A_equal_NaN)
LABEL(_dmul_A_equal_INF_B_equal_zero)
LABEL(_dmul_A_equal_INF_B_equal_NaN)
LABEL(_dmul_A_normal_B_equal_NaN)
        mov         A2,         #0          
        
LABEL(_dmul_A_equal_zero_B_equal_INF_or_NaN)
        mov         A1,         #0x80000000 
        sub         A1,         A1,         #0x80000    /* deliver NaN */
        mov         pc,         lr          

LABEL(_dmul_normal_process)
        str         SIGN_FLAG,  [sp,        #-4]!       /* push FLAG into sp */
        eor         SIGN_FLAG,  A1,         B1          /* capture result sign */
        bic         A1,         A1,         #0x80000000 /* no thinking here */
        bic         B1,         B1,         #0x80000000 /* no thinking here */
        str         EXPMASK,    [sp,        #-4]!       /* push EXPMASK into sp */
        mov         EXPMASK,    #0x800      
        sub         EXPMASK,    EXPMASK,    #1          /* EXPMASK=0x7ff */
        str         EXPA,       [sp,        #-4]!       /* push EXPA into sp */
        str         EXPB,       [sp,        #-4]!       /* push EXPB into sp */
        ands        EXPA,       EXPMASK,    A1,         LSR $EXPSHIFT           
        bic         A1,         A1,         EXPMASK,    LSL $EXPSHIFT           
        orrne       A1,         A1,         $(1<<EXPSHIFT)          
        bne         _dmult_unpackB          
        
LABEL(_dmult_unpackA_1)                                       /* normalize A */
        cmp         A1,         #0          
        clzne       ip,         A1          
        clzeq       ip,         A2          
        addeq       ip,         ip,         #21         
        subne       ip,         ip,         #11         
        tmcr        wCGR0,      ip          
        sub         EXPA,       EXPA,       ip          
        tmcrr       wR1,        A2,         A1          
        add         EXPA,       EXPA,       #1          
        wslldg      wR1,        wR1,        wCGR0       
        tmrrc       A2,         A1,         wR1         

LABEL(_dmult_unpackB)
        ands        EXPB,       EXPMASK,    B1,         LSR $EXPSHIFT           
        bic         B1,         B1,         EXPMASK,    LSL $EXPSHIFT           
        orrne       B1,         B1,         $(1<<EXPSHIFT)          
        bne         _dmult_unpacked         

LABEL(_dmult_unpackB_1)                                       /* normalize B */
        cmp         B1,         #0          
        clzne       ip,         B1          
        clzeq       ip,         B2          
        addeq       ip,         ip,         #21         
        subne       ip,         ip,         #11         
        tmcr        wCGR0,      ip          
        sub         EXPB,       EXPB,       ip          
        tmcrr       wR1,        B2,         B1          
        add         EXPB,       EXPB,       #1          
        wslldg      wR1,        wR1,        wCGR0       
        tmrrc       B2,         B1,         wR1         

LABEL(_dmult_unpacked)

        /*
         * Here we have a couple of normalized operands and
         * will perform the multiply.
         * Too bad the carry out of umlal is not trustworthy,
         * because we really need it for carry propagation!
         *
         * Start low-end and work up to cut down on carries.
         */
        str         RESULTX,    [sp,        #-4]!      /* push RESULTX into sp */
        umull       RESULTX,    PROD3,      A2,         B2          
        tmcr        wCASF,      SIGN_FLAG   /* move SIGN_FLAG to WMMX register */
        mov         PROD2,      #0          
        umlal       PROD3,      PROD2,      B2,         A1          
        add         EXPA,       EXPA,       EXPB        
        umlal       PROD3,      PROD2,      A2,         B1          
        sub         EXPA,       EXPA,       EXPMASK,    LSR #1 
        mov         B2,         #0          
        umlal       PROD2,      B2,         B1,         A1          
        ldr         EXPB,       [sp,        #4]         /* pop EXPB */
        /* now have result in B2/PROD2/PROD3/RESULTX */

        /*
         * Now have a full 106 bits of result. The very highest order bit
         * of the result may or may not be set. At most one shift will be
         * necessary to set it (since we were normalized coming into this
         * calculation).
         */
        cmp         B2,         #(1<<9)     
        bge         _dmult_round            
        mov         A1,         B2,         LSL #12     
        orr         A1,         A1,         PROD2,      LSR #20     
        mov         A2,         PROD2,      LSL #12     
        orr         A2,         A2,         PROD3,      LSR #20     
        adds        RESULTX,    RESULTX,    RESULTX
        adc         PROD3,      PROD3,      PROD3
        orrne       PROD3,      PROD3,      #1          
        cmp         EXPA,       #0          
        mov         RESULTX,    PROD3,      LSL #11     
        bgt         _dmult_check_guard      
        b           _dmult_denorm_result    

LABEL(_dmult_round)
        /*
         * Arrange the bits of our result to conform to the
         * expected output.
         *
         * We have:
         * 10 bits in B2. The highest order is set and will
         *        be the implicit bit of a normalized result.
         * 32 bits in PROD2
         * 32 bits in PROD3
         * some more junk in RESULTX. We only care whether this is ==0
         * or not.
         */
        add         EXPA,       EXPA,       #1          
        mov         A1,         B2,         LSL #11     
        orr         A1,         A1,         PROD2,      LSR #21     
        mov         A2,         PROD2,      LSL #11     
        orr         A2,         A2,         PROD3,      LSR #21     
        cmp         RESULTX,    #0          
        orrne       PROD3,      PROD3,      #1          
        cmp         EXPA,       #0          
        mov         RESULTX,    PROD3,      LSL #11     
        ble         _dmult_denorm_result    

LABEL(_dmult_check_guard)
        /*
         * Look at the guard and sticky bits of our result.
         * Round accordingly.
         */
        /* the Guard bit is now the sign of RESULTX */
        cmp         RESULTX,    #0x80000000 
        blo         _dmult_done_rounding    
        bhi         _dmult_round_up         
        /*
         * Round to even.
         */
        adds        A2,         A2,         #1          
        bic         A2,         A2,         #1          
        b           _dmult_rounding         
LABEL(_dmult_round_up)
        adds        A2,         A2,         #1          
LABEL(_dmult_rounding)
        adc         A1,         A1,         #0          
        /*
         * Now, if the result is 'too big', we carried all the way up.
         * In theory we could shift back down, but we know that all the
         * lower order bits have to be zero, so just jam them in.
         */
        cmp         A1,         #(1<<(EXPSHIFT+1))      
        addge       EXPA,       EXPA,       #1          
LABEL(_dmult_done_rounding)
        /* Test for exponent too big. */
        cmp         EXPA,       EXPMASK     
        bge         _dmult_deliver_Infinity 
        /* Clear the implicit bit, if present */
        bic         A1,         A1,         #0x00700000 
        /* Assemble the result. */
        orr         A1,         A1,         EXPA,       LSL #EXPSHIFT           

LABEL(_dmult_deliver_signed_result)
        ldr         RESULTX,    [sp],       #8          
        ldr         EXPA,       [sp],       #4          
        ldr         EXPMASK,    [sp],       #4          
        ldr         SIGN_FLAG,  [sp],       #4          
        textrcb     R15,        #7          
        orrmi       A1,         A1,         #0x80000000 
        mov         pc,         lr          

        /*
         * If here, we have some combination of zeros and infinities.
         * Act accordingly.
         */
LABEL(_dmult_deliver_signed_zero)
        /* result is zero */
        mov         A1,         #0          
        mov         A2,         #0          
        b           _dmult_deliver_signed_result        

LABEL(_dmult_deliver_Infinity)
        mov         A1,         EXPMASK,    LSL #EXPSHIFT           
        mov         A2,         #0          
        b           _dmult_deliver_signed_result        

LABEL(_dmult_denorm_result)
        /*
         * The exponent is <= 0. This is really too bad.
         * If it is really small ( <-52 ) I  believe we're safe to
         * return a zero. Else we very carefully right shift until
         * the exponent becomes zero, then go round and pack the result.
         * Note that the fraction part is normalized, so we must shift it
         * at least once to denormalize.
         */
        cmp         EXPA,       #-52        
        blt         _dmult_deliver_signed_zero          

        /* we always have to shift once even for EXPA of 0 */
        /* to achieve the denormalized form, then shift until EXPA */
        /* turns to zero. */
        /* (this accounts for the funny mov at the end of this loop) */

LABEL(_dmult_denorm_looping)
        movs        A1,         A1,         LSR #1      
        movs        A2,         A2,         RRX         
        movs        RESULTX,    RESULTX,    RRX         
        orrcs       RESULTX,    RESULTX,    #1          /* this is sticky */
        adds        EXPA,       EXPA,       #1          
        ble         _dmult_denorm_looping   
        mov         EXPA,       #0          /* if EXPA was 0  */
                                            /* we just incremented it, */
                                /* so reset it back before continuing */
LABEL(_dmult_denorm_check_guard)
        /*
         * Look at the guard and sticky bits of our result.
         * Round accordingly.
         * See if we became normalized, and adjust exponent if so.
         * This is very similar to _dmult_check_guard above
         * except for the post-rounding tests.
         */
        /* the Guard bit is now the sign of RESULTX */
        cmp         RESULTX,    #0x80000000 
        blo         _dmult_denorm_done_rounding         
        bhi         _dmult_denorm_round_up  
        /*
         * Round to even.
         */
        adds        A2,         A2,         #1          
        bic         A2,         A2,         #1          
        b           _dmult_denorm_rounding  
LABEL(_dmult_denorm_round_up)
        adds        A2,         A2,         #1          
LABEL(_dmult_denorm_rounding)
        adc         A1,         A1,         #0          
        /*
         * If the result is now normalized, we carried all the way up.
         * But that's ok, because we just won't clear it out
         * of the assembled result
         * Put the sign on and go.
         */
LABEL(_dmult_denorm_done_rounding)
        ldr         RESULTX,    [sp],       #8          
        ldr         EXPA,       [sp],       #4          
        ldr         EXPMASK,    [sp],       #4          
        ldr         SIGN_FLAG,  [sp],       #4          
        textrcb     R15,        #7          
        orrmi       A1,         A1,         #0x80000000 
        mov         pc,         lr          
   
LABEL(_double_check_guard)
        /*
         * Look at the guard and sticky bits of our result.
         * Round accordingly.
         */
        /* the Guard bit is now the sign of RESULTX */
        cmp         RESULTX,    #0x80000000 
        blo         _double_done_rounding   
        bhi         _double_round_up        
        /*
         * Round to even.
         */
        adds        A2,         A2,         #1          
        bic         A2,         A2,         #1          
        b           _double_rounding        
LABEL(_double_round_up)
        adds        A2,         A2,         #1          
LABEL(_double_rounding)
        adc         A1,         A1,         #0          
        /*
         * Now, if the result is 'too big', we carried all the way up.
         * In theory we could shift back down, but we know that all the
         * lower order bits have to be zero, so just jam them in.
         */
        cmp         A1,         #(1<<(EXPSHIFT+1))      
        blt         _double_done_rounding   
        add         EXPA,       EXPA,       #1          
        mov         A1,         #(1<<EXPSHIFT)          
        mov         A2,         #0          
LABEL(_double_done_rounding)
        /* Test for exponent too big. */
        cmp         EXPA,       EXPMASK     
        bge         _double_deliver_Infinity            
        /* Clear the implicit bit, if present */
        bic         A1,         A1,         #0x00100000 
        /* Assemble the result. */
        orr         A1,         A1,         EXPA,       LSL #EXPSHIFT           
LABEL(_double_deliver_signed_result)
        cmp         FLAGS,      #0          
        orrmi       A1,         A1,         #0x80000000 
        ldmfd       sp!,        DOUBLE_RESTORE_SET      
LABEL(_double_deliver_NaN)
        ldr         A1,         CONSTANT(0x7ff80000) 
        mov         A2,         #0          
        ldmfd       sp!,        DOUBLE_RESTORE_SET      
LABEL(_double_deliver_signed_zero)
        /* result is zero */
        mov         A1,         #0          
        mov         A2,         #0          
        b           _double_deliver_signed_result       
LABEL(_double_deliver_positive_zero)
        /* result is +zero */
        mov         A1,         #0          
        mov         A2,         #0          
        ldmfd       sp!,        DOUBLE_RESTORE_SET      
LABEL(_double_deliver_Infinity)
        mov         A1,         EXPMASK,    LSL #EXPSHIFT           
        mov         A2,         #0          
        b           _double_deliver_signed_result       

#undef SIGN_FLAG

#else /* !IAI_DMUL */
	stmfd	sp!, DOUBLE_SAVE_SET
	/*
	 * In addition to the above conventions, working register are:
	 * PROD2 for second-product word
	 * PROD3 for third product word
	 */

#define PROD2	r8
#define PROD3	r9

#define INFFLAG  1
#define ZEROFLAG 2

	/*
	 * Here starts the processing...
	 */
	eors	FLAGS, A1, B1	  /* capture sign of result */
	bic	A1,A1,#0x80000000 /* no thinking here */
	bic	B1,B1,#0x80000000 /* no thinking here */
	and	FLAGS, FLAGS, #0x80000000 /* clear other flags */
	ldr	EXPMASK, CONSTANT(DOUBLE_EXPVAL)   /* exponent mask */
	DOUBLE_UNPACK( A1, EXPA, _dmult_normalizeA, _dmult_exceptionalA )
LABEL(_dmult_unpackB)
	DOUBLE_UNPACK( B1, EXPB, _dmult_normalizeB, _dmult_exceptionalB )
LABEL(_dmult_unpacked)
	tst	FLAGS, #OR2(ZEROFLAG,INFFLAG)
	bne	_dmult_special_operands
	/*
	 * Here we have a couple of normalized operands and
	 * will perform the multiply.
	 * Too bad the carry out of umlal is not trustworthy,
	 * because we really need it for carry propagation!
	 *
	 * Start low-end and work up to cut down on carries.
	 */
	add	EXPA, EXPA, EXPB
	sub	EXPA, EXPA, EXPMASK, LSR #1 /* subtract extra bias */
	umull	RESULTX, PROD3, A2, B2
	mov	PROD2, #0
	umlal	PROD3, PROD2, A1, B2
	/* now done with B2 so can use it for scratch. */
	mov	B2, #0
	umlal	PROD3, B2, A2, B1
	adds	PROD2, PROD2, B2
	mov	B2, #0
	adc	B2, B2, #0	/* B2 gets carry out of above addition */
	umlal	PROD2, B2, A1, B1
	/* now have result in B2/PROD2/PROD3/RESULTX */

	/*
	 * Now have a full 106 bits of result. The very highest order bit
	 * of the result may or may not be set. At most one shift will be
	 * necessary to set it (since we were normalized coming into this
	 * calculation).
	 */
	cmp	B2, #(1<<9)
	addge	EXPA, EXPA, #1
	bge	_dmult_round
	adds	RESULTX, RESULTX, RESULTX
	adcs	PROD3, PROD3, PROD3
	adcs	PROD2, PROD2, PROD2
	adc	B2, B2, B2
LABEL(_dmult_round)
	/*
	 * Arrange the bits of our result to conform to the
	 * expected output.
	 *
	 * We have:
	 * 10 bits in B2. The highest order is set and will
	 *	be the implicit bit of a normalized result.
	 * 32 bits in PROD2
	 * 32 bits in PROD3
	 * some more junk in RESULTX. We only care whether this is ==0
	 * or not.
	 */

	mov	A1, B2, LSL #11
	orr	A1, A1, PROD2, LSR #21
	mov	A2, PROD2, LSL #11
	orr	A2, A2, PROD3, LSR #21
	cmp	RESULTX, #0
	orrne	PROD3, PROD3, #1
	mov	RESULTX, PROD3, LSL #11
	cmp	EXPA, #0
	ble	_dmult_denorm_result

LABEL(_double_check_guard)
	/*
	 * Look at the guard and sticky bits of our result.
	 * Round accordingly.
	 */
	/* the Guard bit is now the sign of RESULTX */
	cmp	RESULTX, #0x80000000
	blo	_double_done_rounding
	bhi	_double_round_up
	/*
	 * Round to even.
	 */
	adds	A2, A2, #1
	bic	A2, A2, #1
	b	_double_rounding
LABEL(_double_round_up)
	adds	A2, A2, #1
LABEL(_double_rounding)
	adc	A1, A1, #0
	/*
	 * Now, if the result is 'too big', we carried all the way up.
	 * In theory we could shift back down, but we know that all the
	 * lower order bits have to be zero, so just jam them in.
	 */
	cmp	A1, #(1<<(EXPSHIFT+1))
	blt	_double_done_rounding
	add	EXPA, EXPA, #1
	mov	A1, #(1<<EXPSHIFT)
	mov	A2, #0
LABEL(_double_done_rounding)
	/* Test for exponent too big. */
	cmp	EXPA, EXPMASK
	bge	_double_deliver_Infinity
	/* Clear the implicit bit, if present */
	bic	A1, A1, #0x00100000
	/* Assemble the result. */
	orr	A1, A1, EXPA, LSL #EXPSHIFT
LABEL(_double_deliver_signed_result)
	cmp	FLAGS, #0
	orrmi	A1, A1, #0x80000000
	ldmfd	sp!, DOUBLE_RESTORE_SET

LABEL(_double_deliver_NaN)
	ldr	A1,  CONSTANT(0x7ff80000)
	mov	A2, #0
	ldmfd	sp!, DOUBLE_RESTORE_SET

LABEL(_dmult_normalizeA)
	DOUBLE_NORMALIZE( A1, A2, EXPA, _dmult_unpackB, #ZEROFLAG )
LABEL(_dmult_normalizeB)
	DOUBLE_NORMALIZE( B1, B2, EXPB, _dmult_unpacked, #ZEROFLAG )
LABEL(_dmult_exceptionalA)
        DOUBLE_EXCEPTIONAL( A1, A2, #INFFLAG, _dmult_unpackB )
LABEL(_dmult_exceptionalB)
        DOUBLE_EXCEPTIONAL( B1, B2, #INFFLAG, _dmult_unpacked )

	/*
	 * If here, we have some combination of zeros and infinities.
	 * Act accordingly.
	 */
LABEL(_dmult_special_operands)
	tst	FLAGS, #INFFLAG
	bne	_dmult_Infinite_operand
LABEL(_double_deliver_signed_zero)
	/* result is zero */
	mov	A1, #0
	mov	A2, #0
	b	_double_deliver_signed_result
LABEL(_double_deliver_positive_zero)
	/* result is +zero */
	mov	A1, #0
	mov	A2, #0
	ldmfd	sp!, DOUBLE_RESTORE_SET

LABEL(_dmult_Infinite_operand)
	tst	FLAGS, #ZEROFLAG
	bne	_double_deliver_NaN
	/* else fall thru and deliver infinity */
LABEL(_double_deliver_Infinity)
	mov	A1, EXPMASK, LSL #EXPSHIFT
	mov	A2, #0
	b	_double_deliver_signed_result

LABEL(_dmult_denorm_result)
	/*
	 * The exponent is <= 0. This is really too bad.
	 * If it is really small ( <-52 ) I  believe we're safe to
	 * return a zero. Else we very carefully right shift until
	 * the exponent becomes zero, then go round and pack the result.
	 * Note that the fraction part is normalized, so we must shift it
	 * at least once to denormalize.
	 */
	cmp	EXPA, #-52
	blt	_double_deliver_signed_zero

	/* we always have to shift once even for EXPA of 0 */
	/* to achieve the denormalized form, then shift until EXPA */
	/* turns to zero. */
	/* (this accounts for the funny mov at the end of this loop) */
LABEL(_dmult_denorm_looping)
	movs	A1, A1, LSR #1
	movs	A2, A2, RRX
	movs	RESULTX, RESULTX, RRX
	orrcs	RESULTX, RESULTX, #1	/* this is why we call it sticky */
	adds	EXPA, EXPA, #1
	ble	_dmult_denorm_looping
	mov	EXPA, #0	/* if EXPA was 0 we just incremented it, */
				/* so reset it back before continuing */
#endif /* !IAI_DMUL */

LABEL(_double_denorm_check_guard)
	/*
	 * Look at the guard and sticky bits of our result.
	 * Round accordingly.
	 * See if we became normalized, and adjust exponent if so.
	 * This is very similar to _double_check_guard above
	 * except for the post-rounding tests.
	 */
	/* the Guard bit is now the sign of RESULTX */
	cmp	RESULTX, #0x80000000
	blo	_double_denorm_done_rounding
	bhi	_double_denorm_round_up
	/*
	 * Round to even.
	 */
	adds	A2, A2, #1
	bic	A2, A2, #1
	b	_double_denorm_rounding
LABEL(_double_denorm_round_up)
	adds	A2, A2, #1
LABEL(_double_denorm_rounding)
	adc	A1, A1, #0
	/*
	 * If the result is now normalized, we carried all the way up.
	 * But that's ok, because we just won't clear it out
	 * of the assembled result
	 * Put the sign on and go.
	 */
LABEL(_double_denorm_done_rounding)
	cmp	FLAGS, #0
	orrmi	A1, A1, #0x80000000
	ldmfd	sp!, DOUBLE_RESTORE_SET

	ENTRY(CVMCCMruntimeDSub)
ENTRY1( CVMCCMruntimeDSub )
	ENTRY(CVMCCMruntimeDSub_C)
ENTRY1( CVMCCMruntimeDSub_C )
	eor	B1, B1, #0x80000000 /* flip sign bit */
	/* fall through */

	ENTRY(CVMCCMruntimeDAdd)
ENTRY1( CVMCCMruntimeDAdd )
	ENTRY(CVMCCMruntimeDAdd_C)
ENTRY1( CVMCCMruntimeDAdd_C )
/* IAI-06 */
#ifdef IAI_DADD
LABEL(_dadd_judge_A_zero)
        orrs        ip,         A2,         A1,         LSL #1      
        bne         _dadd_judge_A_INF_NaN   

LABEL(_dadd_A_equal_zero)
        orrs        ip,         B2,         B1,         LSL #1      
        bne         _dadd_A_equal_zero_B_NOT_equal_Zero 
        and         A1,         A1,         B1          
        mov         pc,         lr          

LABEL(_dadd_A_equal_zero_B_NOT_equal_Zero)
LABEL(_dadd_A_normal_B_equal_INF)
        mov         A1,         B1          
        mov         A2,         B2          
        mov         pc,         lr          

LABEL(_dadd_judge_A_INF_NaN)
        mov         ip,         A1,         LSL #1
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1
        bne         _dadd_judge_B_zero      

LABEL(_dadd_A_equal_INF_NaN)
        orrs        ip,         A2,         A1,         LSL #12     
        bne         _dadd_A_equal_NaN       
        mov         ip,         B1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        movne       pc,         lr          /* A==INF && B!=INF/NaN, return A */
        orrs        ip,         B2,         B1,         LSL #12     
        bne         _dadd_A_equal_INF_B_euqal_NaN       
        eors        ip,         B1,         A1          
        movpl       pc,         lr          /* A==B==INF, return A */

LABEL(_dadd_A_equal_NaN)
LABEL(_dadd_A_equal_INF_B_euqal_NaN)
LABEL(_dadd_A_equal_INF_B_equal_INF_Signs_NOT_equal)
        mov         A1,         #0x80000000 
        sub         A1,         A1,         #0x80000    
        mov         A2,         #0          
        mov         pc,         lr          

LABEL(_dadd_A_NOT_equal_zeor_A_NOT_equal_INF_NaN)

LABEL(_dadd_judge_B_zero)
        orrs        ip,         B2,         B1,         LSL #1      
        moveq       pc,         lr          /* A_normal_B_equal_zero, return A */

LABEL(_dadd_judge_B_INF_NaN)
        mov         ip,         B1,         LSL #1      
        mov         ip,         ip,         ASR $(EXPSHIFT+1)       
        adds        ip,         ip,         #1          
        bne         _dadd_normal_process    

LABEL(_dadd_B_equal_INF_NaN)
        orrs        ip,         B2,         B1,         LSL #12     
        beq         _dadd_A_normal_B_equal_INF          

LABEL(_dadd_B_equal_NaN)
        mov         A1,         #0x80000000 
        sub         A1,         A1,         #0x80000    
        mov         A2,         #0          
        mov         pc,         lr          
        
LABEL(_dadd_normal_process)
 	/*
	 * working register are:
	 * EXPA for exponent of A
	 * EXPB for exponent of B
	 * SIGN_FLAG for sign of operands
	 * A1 for high-order sum word
	 * A2 for second-order sum word
	 * RESULT for left over bits (guard rounding sticky)
	 * TMP: strictly local temp reg.
	 *
	 * The result exponent will be developed in EXPA, and
	 * the result sum in A1/A2
	 */
#define SIGN_FLAG r4
#define TMP	  r4
#define RESULT	  ip
#define AINFFLAG  2
#define BINFFLAG  4
#define AZFLAG    8
#define BZFLAG    0x10

	/*
	 * Here starts the processing...
	 */
        str         EXPB,       [sp,        #-4]!       /* push EXPB into sp */
        str         EXPA,       [sp,        #-4]!       /* push EXPA into sp */

	/* arrange for the larger summand to be in A */
        mov         ip,         A1,         LSL #1      
        cmp         ip,         B1,         LSL #1      
        cmpeq       A2,         B2          
        bhs         _dadd_noxchng           
        tmcrr       wR0,        A2,         A1          
        tmcrr       wR1,        B2,         B1          
        tmrrc       A2,         A1,         wR1         
        tmrrc       B2,         B1,         wR0         /* exchage A and B */

LABEL(_dadd_noxchng)
        mov         EXPA,       A1,         LSL #1      
        mov         EXPB,       B1,         LSL #1      
        mov         EXPA,       EXPA,       LSR #21                /* get EXPA */
        sub         EXPB,       EXPA,       EXPB,       LSR #21    /* get EXPB */
        cmp         EXPB,       #54         
        bgt         _dadd_deliver_a         

LABEL(_dadd_get_flags)
        eor         ip,         A1,         B1          
        tmcrr       wR9,        ip,         A1          /* put flags into wR9 */
 	                                    /* LO(wR9) refers to do add or sub */
 	                                    /* HI(wR9) presents sign of result */
LABEL(_dadd_unpackA_andB)
        mov         ip,         $(1<<EXPSHIFT)          
        mov         A1,         A1,         LSL #12     
        mov         B1,         B1,         LSL #12     
        add         A1,         ip,         A1,         LSR #12    /* unpack A */
        add         B1,         ip,         B1,         LSR #12    /* unpack B */
        cmp         EXPB,       EXPA        
        bne         _dadd_unpacked          

LABEL(_dadd_A_and_or_B_denormal)
        teq         EXPA,       #0          
        eoreq       A1,         A1,         #0x00100000 /* denormal A */
        moveq       EXPA,       #1          
        eor         B1,         B1,         #0x00100000 /* denormal B */
        mov         EXPB,       #1          
        sub         EXPB,       EXPA,       EXPB        
    
LABEL(_dadd_unpacked)
	/*
	 * Scale B by (EXPA-EXPB) in preparation of addition.
	 * Recall that EXPA >= EXPB.
	 * RESULT will hold shifted-out bits. Its high-order
	 * bit is the guard bit, and any bits shifted out have
	 * to be treated as sticky.
	 */
        cmp         EXPB,       #0          
        mov         RESULT,     #0          
        beq         _dadd_done_scaling      /* It can happen. */
        cmp         EXPB,       #32         
        bgt         _dadd_scaling_32_to_54  

LABEL(_dadd_scaling_1_to_32)
        tmcr        wCGR0,      EXPB        
        tmcrr       wR0,        B2,         B1          
        tmcrr       wR1,        RESULT,     B2          
        wsrldg      wR0,        wR0,        wCGR0       
        wsrldg      wR1,        wR1,        wCGR0       
        tmrrc       B2,         B1,         wR0         
        textrmuw    RESULT,     wR1,        #0          
        b           _dadd_done_scaling      
	
LABEL(_dadd_scaling_32_to_54)
        sub         EXPB,       EXPB,       #32         
        tmcr        wCGR0,      EXPB        
        tmcrr       wR0,        B2,         B1          
        tmcrr       wR1,        RESULT,     B2          
        wsrldg      wR0,        wR0,        wCGR0       
        wsrldg      wR1,        wR1,        wCGR0       
        textrmuw    TMP,        wR1,        #0          
        tmrrc       RESULT,     B2,         wR0         
        mov         B1,         #0          
        cmp         TMP,        #0          
        orrne       RESULT,     RESULT,     #1          
        
LABEL(_dadd_done_scaling)
	/*
	 * If signs the same, we do addition.
	 * If signs differ, we do subtraction.
	 */
        textrmuw    SIGN_FLAG,  wR9,        #0          
        teq         SIGN_FLAG,  #0          
        bmi         _dadd_subtract          
        adds        A2,         A2,         B2          
        adc         A1,         A1,         B1          
	/* shift, if necessary, so HOB is the implicit bit. */
        cmp         A1,         #(1<<(EXPSHIFT+1))      
        blt         _dadd_nocarry           
	/* there was a carry out and we downshift accordingly */
        movs        A1,         A1,         LSR #1      
        movs        A2,         A2,         RRX         
        movs        RESULT,     RESULT,     RRX         
        orrcs       RESULT,     RESULT,     #1          /* this is sticky */
        add         EXPA,       EXPA,       #1          
        b           _dadd_check_guard       

LABEL(_dadd_nocarry)
	/* the only way to have a denorm here is from the sum of */
	/* two denorms. */
        cmp         EXPA,       #1          
        bgt         _dadd_check_guard       

	/*
	 * The exponent is == 1. This is the result of the sum 
	 * or difference of two denormalized numbers.
	 * If the fraction is all zero, return +0
	 * Else we jam in the sign and return. No rounding necessary here.
	 */
LABEL(_dadd_denorm_result)
        textrmuw    SIGN_FLAG,  wR9,        #1          
        ldr         EXPA,       [sp],       #4          /* pop EXPA */
        orrs        ip,         A1,         A2          
        cmpne       SIGN_FLAG,  #0          
        ldr         EXPB,       [sp],       #4          /* pop EXPB */
        orrmi       A1,         A1,         #0x80000000 
        mov         pc,         lr          


	/*
	 * Here we are taking the sum of numbers with different signs.
	 * This could lead to cancellation so we're a little careful.
	 */
LABEL(_dadd_subtract)
	/* negate the guard word, with carry */
	/* then subtract */
        rsbs        RESULT,     RESULT,     #0          
        sbcs        A2,         A2,         B2          
        sbc         A1,         A1,         B1          
	/* find a-priori denorms immediately. */
        cmp         EXPA,       #1          
        ble         _dadd_denorm_result     
        cmp         A1,         #(1<<EXPSHIFT)          
        bge         _dadd_check_guard       /* could get lucky */
	/*
	 * The most significant bit of the result might be anywhere.
	 * The result may be denormalized.
	 * It might be zero.
	 * If it is zero, we must deliver +0 as a result.
	 *
	 * We're going to have to shift it. We want to shift LEFT
	 * until we can normalize. However, we might not be able to 
	 * deliver a normalized result.  We shift until either normalized
	 * or the EXPA decrements to 1, which we then treat as denormalized.
	 *
	 * If the EXPA is great enough, try to do 16- or 32-bit at a time
	 * shifts.
	 */
        cmp         A1,         #0          
        beq         _dadd_A1_eq_zero        /* there is a significant bit  */
						/* ... in A1 */
LABEL(_dadd_A1_ne_zero)
        clz         TMP,        A1          
        sub         TMP,        TMP,        #11         
        subs        EXPA,       EXPA,       TMP         
        ble         _dadd_EXPA_le_TMP       

LABEL(_dadd_A1_ne_zero_EXPA_gt_TMP)        		/* normalize A */
        tmcr        wCGR0,      TMP         
        tmcrr       wR0,        A2,         A1          
        tmcrr       wR1,        RESULT,     A2          
        wslldg      wR0,        wR0,        wCGR0       
        wslldg      wR1,        wR1,        wCGR0       
        tmrrc       RESULT,     A2,         wR1         
        tmrrc       TMP,        A1,         wR0         
        b           _dadd_check_guard       

LABEL(_dadd_EXPA_le_TMP)				/* denormalize A */
        add         EXPA,       EXPA,       TMP         
        sub         EXPA,       EXPA,       #1          
        tmcr        wCGR0,      EXPA        
        tmcrr       wR0,        A2,         A1          
        tmcrr       wR1,        RESULT,     A2          
        wslldg      wR0,        wR0,        wCGR0       
        wslldg      wR1,        wR1,        wCGR0       
        tmrrc       RESULT,     A2,         wR1         
        tmrrc       TMP,        A1,         wR0         
        b           _dadd_deliver_signed_result         

LABEL(_dadd_A1_eq_zero)
        orrs        TMP,        A2,         RESULT
        beq         _dadd_deliver_a         /* deliver zero */
        clz         TMP,        A2          
        add         TMP,        TMP,        #21         
        subs        EXPA,       EXPA,       TMP         
        ble         _dadd_EXPA_le_TMP       

LABEL(_dadd_A1_eq_zero_EXPA_gt_TMP)        		/* normalize A */
        cmp         A2,         #0x100000   
        rsbhi       TMP,        TMP,        #32         
        subls       TMP,        TMP,        #32         
        tmcr        wCGR0,      TMP         
        tmcrr       wR1,        RESULT,     A2          
        wsrldghi    wR1,        wR1,        wCGR0       
        wslldgls    wR1,        wR1,        wCGR0       
        tmrrc       A2,         A1,         wR1         
        b           _dadd_done_rounding     
        
LABEL(_dadd_check_guard)
	/*
	 * Look at the guard and sticky bits of our result.
	 * Round accordingly.
	 */
	/* the Guard bit is now the sign of RESULT */
        cmp         RESULT,     #0x80000000 
        blo         _dadd_done_rounding     
        bhi         _dadd_round_up          
	/*
	 * Round to even.
	 */
        adds        A2,         A2,         #1          
        bic         A2,         A2,         #1          
        b           _dadd_rounding          
LABEL(_dadd_round_up)
        adds        A2,         A2,         #1          
LABEL(_dadd_rounding)
        adc         A1,         A1,         #0          
	/*
	 * Now, if the result is 'too big', we carried all the way up.
	 * In theory we could shift back down, but we know that all the
	 * lower order bits have to be zero, so just jam them in.
	 */
        cmp         A1,         #(1<<(EXPSHIFT+1))      
        blt         _dadd_done_rounding     
        add         EXPA,       EXPA,       #1          
        mov         A1,         #(1<<EXPSHIFT)          
LABEL(_dadd_done_rounding)
	/* Test for exponent too big. */
        add         ip,         EXPA,       #1          
        cmp         ip,         #0x800      
        bge         _dadd_deliver_Infinity  
	/* Clear the implicit bit, if present */
        bic         A1,         A1,         #0x00100000 
	/* Assemble the result. */
        orr         A1,         A1,         EXPA,       LSL #EXPSHIFT           
LABEL(_dadd_deliver_signed_result)
        textrmuw    SIGN_FLAG,  wR9,        #1          
        ldr         EXPA,       [sp],       #4          /* pop EXPA */
        cmp         SIGN_FLAG,  #0          
        ldr         EXPB,       [sp],       #4          /* pop EXPB */
        orrmi       A1,         A1,         #0x80000000 
        mov         pc,         lr          

LABEL(_dadd_deliver_Infinity)
        mov         A1,         #0x7f000000 
        add         A1,         A1,         #0x00f00000 
        mov         A2,         #0          
        b           _dadd_deliver_signed_result     
        
LABEL(_dadd_deliver_a)
        ldr         EXPA,       [sp],       #4          /* pop EXPA */
        ldr         EXPB,       [sp],       #4          /* pop EXPB */
        mov         pc,         lr          
            
#undef SIGN_FLAG
#undef RESULT
#undef TMP


#else /* !IAI_DADD */

	stmfd	sp!, DOUBLE_SAVE_SET
	/*
	 * working register are:
	 * EXPA for exponent of A
	 * EXPB for exponent of B
	 * FLAGS for sign of operands and other facts we determine
	 *	durning unpacking.
	 * A1 for high-order sum word
	 * A2 for second-order sum word
	 * RESULTX for left over bits (guard rounding sticky)
	 * EXPMASK: the exponent mask, also the infinite exponent.
	 * LSHIFT: a shift count and temp reg
	 * TEMP: strictly local temp reg.
	 *
	 * The result exponent will be developed in EXPA, and
	 * the result sum in A1/A2
	 */
#define LSHIFT	 r8
#define TEMP	 r9
#define ANEGFLAG 0x80000000
#define BNEGFLAG 0x40000000
#define AINFFLAG 2
#define BINFFLAG 4
#define AZFLAG   8
#define BZFLAG   0x10

/*
 * Check for and flag zeros. Else give the number an exponent of '1'
 * so that, even in the absence of an implicit bit, these numbers will
 * scale right when combined with normalized values.
 */
#ifndef __RVCT__
#define DADD_DENORMAL( HiSrc, LoSrc, ExpSrc, Zeroflag, Lreturn )\
	cmp	HiSrc, #0; \
	cmpeq	LoSrc, #0; \
	orreq	FLAGS, FLAGS, Zeroflag; \
	beq	Lreturn; \
	mov	ExpSrc, #1; \
	b	Lreturn
#else
	MACRO
	DADD_DENORMAL0 $HiSrc, $LoSrc, $ExpSrc, $Zeroflag, $Lreturn
	cmp	$HiSrc, #0
	cmpeq	$LoSrc, #0
	orreq	FLAGS, FLAGS, $Zeroflag
	beq	$Lreturn
	mov	$ExpSrc, #1
	b	$Lreturn
	MEND

#define DADD_DENORMAL( HiSrc, LoSrc, ExpSrc, Zeroflag, Lreturn )\
	DADD_DENORMAL0 HiSrc, LoSrc, ExpSrc, Zeroflag, Lreturn
#endif

	/*
	 * Here starts the processing...
	 */
	/* capture signs of operands */
	and	FLAGS, A1, #ANEGFLAG /* because ANEGFLAG is SIGN */
	cmp	B1, #0
	orrmi	FLAGS, FLAGS, #BNEGFLAG
	bic	A1,A1,#0x80000000 /* no thinking here */
	bic	B1,B1,#0x80000000 /* no thinking here */
	/* arrange for the larger summand to be in A */
	cmp	A1,B1
	cmpeq	A2,B2
	bhs	_dadd_noxchng
	mov	RESULTX, A1
	mov	A1, B1
	mov	B1, RESULTX
	mov	RESULTX, A2
	mov	A2, B2
	mov	B2, RESULTX
	movs	FLAGS, FLAGS, LSL #1	/* exchange sign flags, too */
	orrcs	FLAGS, FLAGS, #BNEGFLAG	/* (may not matter) */

LABEL(_dadd_noxchng)
	ldr	EXPMASK, CONSTANT(DOUBLE_EXPVAL)	  /* exponent mask */
	DOUBLE_UNPACK( A1, EXPA, _dadd_subnormalA, _dadd_exceptionalA )
LABEL(_dadd_unpackB)
	DOUBLE_UNPACK( B1, EXPB, _dadd_subnormalB, _dadd_exceptionalB )
LABEL(_dadd_unpacked)
	tst	FLAGS, #OR4(AINFFLAG,BINFFLAG,AZFLAG,BZFLAG)
	bne	_dadd_unusual_operands

	/*
	 * Scale B by (EXPA-EXPB) in preparation of addition.
	 * Recall that EXPA >= EXPB.
	 * RESULTX will hold shifted-out bits. Its high-order
	 * bit is the guard bit, and any bits shifted out have
	 * to be treated as sticky.
	 */
	subs	EXPB, EXPA, EXPB
	mov	RESULTX, #0
	beq	_dadd_done_scaling	/* It can happen. */
	cmp	EXPB, #54
	bgt	_dadd_deliver_a		/* B would scale to nothingness */
	cmp	EXPB, #32
	blt	_dadd_continue_scaling
	/* scale a whole 32 bits worth */
	mov	RESULTX, B2
	mov	B2, B1
	mov	B1, #0
	sub	EXPB, EXPB, #32
LABEL(_dadd_continue_scaling)
	rsb	LSHIFT, EXPB, #32
	/* capture any sticky bits we are about to shift out */
	mov	TEMP, RESULTX, LSL LSHIFT
	/* shift bits down */
	mov	RESULTX, RESULTX, LSR EXPB
	orr	RESULTX, RESULTX, B2, LSL LSHIFT
	mov	B2, B2, LSR EXPB
	orr	B2, B2, B1, LSL LSHIFT
	mov	B1, B1, LSR EXPB
	/* reinsert sticky bits */
	cmp	TEMP, #0
	orrne	RESULTX, RESULTX, #1

LABEL(_dadd_done_scaling)
	/*
	 * If signs the same, we do addition.
	 * If signs differ, we do subtraction.
	 */
	teq	FLAGS, FLAGS, LSL #1
	bmi	_dadd_subtract
	adds	A2, A2, B2
	adc	A1, A1, B1
	/* shift, if necessary, so HOB is the implicit bit. */
	cmp	A1, #(1<<(EXPSHIFT+1))
	blt	_dadd_nocarry
	/* there was a carry out and we downshift accordingly */
	movs	A1, A1, LSR #1
	movs	A2, A2, RRX
	movs	RESULTX, RESULTX, RRX
	orrcs	RESULTX, RESULTX, #1	/* this is why we call it sticky */
	add	EXPA, EXPA, #1
	b	_double_check_guard
LABEL(_dadd_nocarry)
	/* the only way to have a denorm here is from the sum of */
	/* two denorms. */
	cmp	EXPA, #1
	bgt	_double_check_guard

	/*
	 * The exponent is == 1. This is the result of the sum 
	 * or difference of two denormalized numbers.
	 * If the fraction is all zero, return +0
	 * Else we jam in the sign and return. No rounding necessary here.
	 */
LABEL(_dadd_denorm_result)
	orrs	TEMP, A1, A2
	cmpne	FLAGS, #0
	orrmi	A1, A1, #0x80000000
	ldmfd	sp!, DOUBLE_RESTORE_SET


	/*
	 * Here we are taking the sum of numbers with different signs.
	 * This could lead to cancellation so we're a little careful.
	 */
LABEL(_dadd_subtract)
	/* negate the guard word, with carry */
	/* then subtract */
	rsbs	RESULTX, RESULTX, #0
	sbcs	A2, A2, B2
	sbc	A1, A1, B1
	/* find a-priori denorms immediately. */
	cmp	EXPA, #1
	ble	_dadd_denorm_result
	cmp	A1, #(1<<EXPSHIFT)
	bge	_double_check_guard	/* could get lucky */
	/*
	 * The most significant bit of the result might be anywhere.
	 * The result may be denormalized.
	 * It might be zero.
	 * If it is zero, we must deliver +0 as a result.
	 *
	 * We're going to have to shift it. We want to shift LEFT
	 * until we can normalize. However, we might not be able to 
	 * deliver a normalized result.  We shift until either normalized
	 * or the EXPA decrements to 1, which we then treat as denormalized.
	 *
	 * If the EXPA is great enough, try to do 16- or 32-bit at a time
	 * shifts.
	 */
	cmp	A1, #0
	bne	_dadd_normalizing_bitloop	/* there is a significant bit  */
						/* ... in A1 */
	cmp	A2, #0x8000
	bhi	_dadd_normalizing_halfwordshift /* there is a bit in  */
						  /* ... the high half of A2 */
	orrs	TEMP, A2, RESULTX, LSR #31
	beq	_double_deliver_positive_zero	/* no nonzero bits. Deliver +0 */
	/* 32-bit shift if exponent is large enough, */
	/* then go to bitloop */
	cmp	EXPA, #32
	ble	_dadd_normalizing_bitloop
	mov	A1, A2
	mov	A2, RESULTX
	mov	RESULTX, #0
	sub	EXPA, EXPA, #32
	b	_dadd_normalizing_bitloop
LABEL(_dadd_normalizing_halfwordshift)
	/* halfword shift if the exponent is large enough, */
	/* then go to bitloop */
	cmp	EXPA, #16
	ble	_dadd_normalizing_bitloop
	mov	A1, A2, LSR #16
	mov	A2, A2, LSL #16
	orr	A2, A2, RESULTX, LSR #16
	mov	RESULTX, RESULTX, LSL #16 /* is always zero! */
	sub	EXPA, EXPA, #16
	b	_dadd_normalizing_bitloop

	/*
	 * In this loop we try to normalize the result for
	 * the subtraction of normalized values.
	 * We know that the result is non-zero, but it may
	 * underflow to a signed zero result.
	 * A1 < (1<<EXPSHIFT) and A1|A2|RESULTX != 0 and EXPA > 1
	 */
LABEL(_dadd_normalizing_bitloop_top)
	adds	RESULTX, RESULTX, RESULTX
	adcs	A2, A2, A2
	adc	A1, A1, A1
	sub	EXPA, EXPA, #1
LABEL(_dadd_normalizing_bitloop)
	cmp	A1, #(1<<EXPSHIFT)
	bhs	_double_check_guard	/* if now normalized */
	cmp	EXPA, #1
	bgt	_dadd_normalizing_bitloop_top
	/* else exponent went to 1 while trying to normalize. */
	/* go round and return as a denorm. */
	b	_double_denorm_check_guard

LABEL(_dadd_subnormalA)
	DADD_DENORMAL( A1, A2, EXPA, #AZFLAG, _dadd_unpackB )
LABEL(_dadd_subnormalB)
	DADD_DENORMAL( B1, B2, EXPB, #BZFLAG, _dadd_unpacked )

LABEL(_dadd_exceptionalA)
	DOUBLE_EXCEPTIONAL( A1, A2, #AINFFLAG, _dadd_unpackB )
LABEL(_dadd_exceptionalB)
	DOUBLE_EXCEPTIONAL( B1, B2, #BINFFLAG, _dadd_unpacked )

LABEL(_dadd_unusual_operands)
	tst	FLAGS, #OR2(AINFFLAG,BINFFLAG)
	bne	_dadd_infinite_operands
	/*
	 * one or more zeros. No Infinities or NaNs here.
	 */
	tst	FLAGS, #AZFLAG
	beq	_dadd_deliver_a /* A != 0 so B == 0 so result is A */
	/*
	 * here A is certainly zero.
	 * but since we arranged for abs(A) >= abs(B), B must be zero, too.
	 * So the result has to be zero.
	 * sign of result is AND of signs of operands
	 */
	and	A1, FLAGS, FLAGS, LSL #1
	and	A1, A1, #0x80000000
	mov	A2, #0
	ldmfd	sp!, DOUBLE_RESTORE_SET

LABEL(_dadd_deliver_a)
	/* if EXPA is "1", then treat it as a denorm */
	cmp	EXPA, #1
	bne	_double_done_rounding
	b	_double_denorm_done_rounding

	/*
	 * One or more Infinities.
	 * No NaNs.
	 */
LABEL(_dadd_infinite_operands)
	tst	FLAGS, #BINFFLAG
	beq	_double_done_rounding /* A is, but B is not */
	/* have two infinities */
	teq	FLAGS, FLAGS, LSL #1
	bmi	_double_deliver_NaN 	/* signs differ, deliver NaN */
	b	_double_deliver_Infinity
#endif /* !IAI_DADD */


/*
 * Entry point for double precision floating division.
 * On entry, dividend (call it A) is in r0/r1,
 * and divisor (call it B) is in r2/r3.
 * On exit, quotient will be in r0/r1.
 */

	ENTRY(CVMCCMruntimeDDiv)
ENTRY1( CVMCCMruntimeDDiv )
	ENTRY(CVMCCMruntimeDDiv_C)
ENTRY1( CVMCCMruntimeDDiv_C )
	stmfd	sp!, DOUBLE_SAVE_SET
	/*
	 * In addition to the above conventions, working register are:
	 * QUOT1 for first quotient word (after done with EXPB)
	 * QUOT2 for second quotient word
	 * ITER  for the loop counter
	 */

#define QUOT1	EXPB
#define QUOT2	r8
#define ITER	r9

	/*
	 * Here starts the processing...
	 */
	eors	FLAGS, A1, B1	  /* capture sign of result */
	bic	A1,A1,#0x80000000 /* no thinking here */
	bic	B1,B1,#0x80000000 /* no thinking here */
	and	FLAGS, FLAGS, #0x80000000 /* clear other flags */
	ldr	EXPMASK, CONSTANT(DOUBLE_EXPVAL)	  /* exponent mask */
	DOUBLE_UNPACK( A1, EXPA, _ddivNormalizeA, _ddivExceptionalA )
LABEL(_ddivUnpackB)
	DOUBLE_UNPACK( B1, EXPB, _ddivNormalizeB, _ddivExceptionalB )
LABEL(_ddivUnpacked)
	tst	FLAGS, #OR4(AZFLAG,BZFLAG,AINFFLAG,BINFFLAG)
	bne	_ddivSpecialOperands
	/*
	 * Here we have a couple of normalized operands and
	 * will perform the division.
	 */
	sub	EXPA, EXPA, EXPB
	add	EXPA, EXPA, EXPMASK, LSR #1 /* add extra bias */
	/*
	 * Make sure the first digit is a '1' by increasing A if
	 * necessary.
	 */
	cmp	A1, B1
	cmpeq	A2, B2
	bhs	_ddiv1
	adds	A2, A2, A2
	adc	A1, A1, A1
	sub	EXPA, EXPA, #1
LABEL(_ddiv1)
	/*
	 * Develop the high-order 21 quotient bits in QUOT1.
	 * Develop the lower-order 32 quotient bits in QUOT2
	 * Develop the lowest-order 1 quotient bit in RESULTX
	 *
	 * Try non-restoring divide here. It appears easier, maybe.
	 */
#ifndef __RVCT__
#define DDIV_SHIFT_REMAINDER	\
	adds	A2, A2, A2;	\
	adcs	A1, A1, A1;	\
	blt	1f

#define DDIV_STEP		\
	subs	A2, A2, B2;	\
	sbcs	A1, A1, B1;	\
	b	2f;		\
1:				\
	adds	A2, A2, B2;	\
	adcs	A1, A1, B1;	\
2:
#else
	MACRO
	DDIV_SHIFT_REMAINDER
	adds	A2, A2, A2
	adcs	A1, A1, A1
	blt	%f1
	MEND

	MACRO
	DDIV_STEP
	subs	A2, A2, B2
	sbcs	A1, A1, B1
	b	%f2
1
	adds	A2, A2, B2
	adcs	A1, A1, B1
2
	MEND
#endif
	
	mov	QUOT2, #0
	mov	QUOT1, #0

	/* first 21 bits of quotient into QUOT1 */
	mov	ITER, #21
	b	_ddivFirstLoopEntry
LABEL(_ddivFirstLoopTop)
	DDIV_SHIFT_REMAINDER
LABEL(_ddivFirstLoopEntry)
	DDIV_STEP
	adc	QUOT1, QUOT1, QUOT1
	subs	ITER, ITER, #1
	bgt	_ddivFirstLoopTop

	/* second 32 bits of quotient into QUOT2 */
	mov	ITER, #32
LABEL(_ddivSecondLoopTop)
	DDIV_SHIFT_REMAINDER
	DDIV_STEP
	adc	QUOT2, QUOT2, QUOT2
	subs	ITER, ITER, #1
	bgt	_ddivSecondLoopTop

	/* compute guard bit into high-order bit of RESULTX */
	/* we know ITER is 0 at this point. */
	DDIV_SHIFT_REMAINDER
	DDIV_STEP
	mov	RESULTX, ITER, RRX

	/* adjust remainder */
	cmp	A1, #0
	bge	_ddivSkipAdjustRemainder
	adds	B2, A2, B2
	adc	B1, A1, B1
LABEL(_ddivSkipAdjustRemainder)
	/*
	 * Result is now in QUOT1 | QUOT2 | RESULTX and EXPA
	 * The remainder is in B1 | B2
	 * Put result in the form that is expected by
	 * the shared rounding and packing code.
	 *
	 */
	mov	A1, QUOT1
	mov	A2, QUOT2
	orrs	B1, B1, B2
	orrne	RESULTX, RESULTX, #0x40000000 /* sticky bit */
	cmp	EXPA, #0
	bgt	_double_check_guard

/* IAI-06 */
#ifdef IAI_DMUL
        cmp         EXPA,       #-52
        blt         _double_deliver_signed_zero

        /* we always have to shift once even for EXPA of 0 */
        /* to achieve the denormalized form, then shift until EXPA */
        /* turns to zero. */
        /* (this accounts for the funny mov at the end of this loop) */
LABEL(_ddiv_denorm_looping)
        movs        A1,         A1,         LSR #1
        movs        A2,         A2,         RRX
        movs        RESULTX,    RESULTX,    RRX
        orrcs       RESULTX,    RESULTX,    #1 /* this is why we call it sticky */
        adds        EXPA,       EXPA,       #1
        ble         _ddiv_denorm_looping
        mov         EXPA,       #0    /* if EXPA was 0 we just incremented it, */
                                      /* so reset it back before continuing */
        b           _double_denorm_check_guard
#else        
	b	_dmult_denorm_result
#endif


LABEL(_ddivNormalizeA)
	DOUBLE_NORMALIZE( A1, A2, EXPA, _ddivUnpackB, #AZFLAG )
LABEL(_ddivNormalizeB)
	DOUBLE_NORMALIZE( B1, B2, EXPB, _ddivUnpacked, #BZFLAG )
LABEL(_ddivExceptionalA)
        DOUBLE_EXCEPTIONAL( A1, A2, #AINFFLAG, _ddivUnpackB )
LABEL(_ddivExceptionalB)
        DOUBLE_EXCEPTIONAL( B1, B2, #BINFFLAG, _ddivUnpacked )

	/*
	 * If here, we have some combination of zeros and infinities.
	 * Act accordingly.
	 */
LABEL(_ddivSpecialOperands)
	bic	RESULTX, FLAGS, #0x80000000 /* disregard sign */
	teq	RESULTX, #OR2(AINFFLAG,BINFFLAG)
	beq	_double_deliver_NaN	/* Inf / Inf => NaN */
	teq	RESULTX, #OR2(AZFLAG,BZFLAG)
	beq	_double_deliver_NaN	/* 0 / 0 => NaN */
	tst	RESULTX, #OR2(AINFFLAG,BZFLAG)
	bne	_double_deliver_Infinity /* Inf / non-Inf == non-0 / 0 => Inf */
	/* resulting cases are: */
	/*	non-Inf / Inf => 0 */
	/*	0 / non-0     => 0 */
	b	_double_deliver_signed_zero
#undef A1
#undef A2
#undef B1
#undef B2
#undef RESULTX
#undef PROD2
#undef PROD3
#undef QUOT1
#undef QUOT2
#undef TEMP
#undef LSHIFT
#undef ITER
#undef EXPA
#undef EXPB
#undef FLAGS
#undef EXPMASK
#undef INFFLAG
#undef ZEROFLAG
#undef EXPSHIFT
#undef DOUBLE_UNPACK
#undef DOUBLE_EXCEPTIONAL
#undef DEMULT_NORMALIZE
#undef DADD_DENORM

#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
#define HIARG1	r0
#define LOARG1	r1
#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
#define HIARG1	r1
#define LOARG1	r0
#endif

/*
 * Entry point for doing an unsigned shift right on longs.
 * NOTE: The result is in r0 and r1.
 */
	ENTRY(CVMCCMruntimeLUshr)
ENTRY1 ( CVMCCMruntimeLUshr )
	ENTRY(CVMCCMruntimeLUshr_C)
ENTRY1 ( CVMCCMruntimeLUshr_C )
        /* r0, r1 = a1, a2 = longValue */
        /* r2 = a3 = shiftCount */

        and     r2, r2, #0x3f               /* So says the VM spec. */
        rsbs    r3, r2, #32                 /* r3 = 32 - r2; */
        ble     _lushrShiftGreaterThan32    /* if (r2 > 32) then branch. */

        /* Perform a logical right shift of less than 32: */
        mov     LOARG1, LOARG1, LSR r2  /* result.lo = (value1.lo >>> value2) | */
        orr     LOARG1, LOARG1, HIARG1, LSL r3 /* (value1.hi << (32-value2)); */
        mov     HIARG1, HIARG1, LSR r2    /* result.hi = value1.hi >>> value2; */
        mov     pc, lr

LABEL(_lushrShiftGreaterThan32)
        /* Perform a logical right shift of greater than or equal to 32: */
        sub     r2, r2, #32
        mov     LOARG1, HIARG1, LSR r2  /* result.lo = value1.hi >> (value2-32); */
        mov     HIARG1, #0              /* result.hi = 0; */
        mov     pc, lr

/*
 * Entry point for doing a signed shift right on longs.
 * NOTE: The result is in r0 and r1.
 */
	ENTRY(CVMCCMruntimeLShr)
ENTRY1 ( CVMCCMruntimeLShr )
	ENTRY(CVMCCMruntimeLShr_C)
ENTRY1 ( CVMCCMruntimeLShr_C )
        /* r0, r1 = a1, a2 = longValue */
        /* r2 = a3 = shiftCount */

        and     r2, r2, #0x3f               /* So says the VM spec. */
        rsbs    r3, r2, #32                 /* r3 = 32 - r2; */
        ble     _lshrShiftGreaterThan32     /* if (r2 > 32) then branch. */

        /* Perform a signed right shift of less than 32: */
        mov     LOARG1, LOARG1, LSR r2  /* result.lo = (value1.lo >> value2) | */
        orr     LOARG1, LOARG1, HIARG1, LSL r3  /* (value1.hi << (32-value2)); */
        mov     HIARG1, HIARG1, ASR r2     /* result.hi = value1.hi >> value2; */
        mov     pc, lr

LABEL(_lshrShiftGreaterThan32)
        /* Perform a signed right shift of greater than or equal to 32: */
        sub     r2, r2, #32
        movs    LOARG1, HIARG1, ASR r2      /* result.lo = value1.hi >> (value2-32); */
        mov     HIARG1, HIARG1, ASR #31      /* result.hi = 0 or -1; */
        mov     pc, lr

/*
 * Entry point for doing an unsigned shift left on longs.
 * NOTE: The result is in r0 and r1.
 */
	ENTRY(CVMCCMruntimeLShl)
ENTRY1 ( CVMCCMruntimeLShl )
	ENTRY(CVMCCMruntimeLShl_C)
ENTRY1 ( CVMCCMruntimeLShl_C )
        /* r0, r1 = a1, a2 = longValue */
        /* r2 = a3 = shiftCount */

        and     r2, r2, #0x3f               /* So says the VM spec. */
        rsbs    r3, r2, #32                 /* r3 = 32 - r2; */
        ble     _lshlShiftGreaterThan32     /* if (r2 > 32) then branch. */

        /* Perform a left shift of less than 32: */
        mov     HIARG1, HIARG1, LSL r2  /* result.hi = (value1.hi << value2) | */
        orr     HIARG1, HIARG1, LOARG1, LSR r3  /*    (value1.lo >> (32-value2)); */
        mov     LOARG1, LOARG1, LSL r2     /* result.lo = value1.lo << value2; */
        mov     pc, lr

LABEL(_lshlShiftGreaterThan32)
        /* Perform a left slift of greater than or equal to 32: */
        sub     r2, r2, #32
        mov     HIARG1, LOARG1, LSL r2  /* result.hi = value1.lo << (value2-32); */
        mov     LOARG1, #0              /* result.lo = 0; */
        mov     pc, lr


        /* The following must be aligned on a 4 byte boundary: */
        ALIGN(2);
LABEL(floatMantissaMask)
        WORD(0x007fffff)
LABEL(floatNaN)
        WORD(0x7fc00000)
LABEL(doubleExponentMask)
        WORD(0x7ff00000)
LABEL(doubleMantissaMask)
        WORD(0x000fffff)

#undef HIARG1
#undef LOARG1

/*
 * Entry point for 32-bit integer division and remainder.
 * On entry, dividend is in r0 and divisor is in r1.
 * On exit, result will be in r0.
 */

	ENTRY(CVMCCMruntimeIDiv)
ENTRY1( CVMCCMruntimeIDiv )
	ENTRY(CVMCCMruntimeIDiv_C)
ENTRY1 ( CVMCCMruntimeIDiv_C )

#define DIVIDE_STEP0(shiftcount, R, V, QUOT) \
        cmp         R,          V,          LSL shiftcount
#define DIVIDE_STEP1(shiftcount, R, V, QUOT) \
        subhs       R,          R,          V,          LSL shiftcount
#define DIVIDE_STEP2(shiftcount, R, V, QUOT) \
        adc         QUOT,       QUOT,       QUOT

#define DIVIDE_INTERLOOP_STEP0(shiftcount, label, R, V) \
        cmp         R,          V,          LSL shiftcount
#define DIVIDE_INTERLOOP_STEP1(shiftcount, label, R, V) \
        bls         label       

#ifdef __RVCT__
	MACRO
	M_DIVIDE_STEP $ShiftCount, $R, $V, $Q
	DIVIDE_STEP0($ShiftCount, $R, $V, $Q)
	DIVIDE_STEP1($ShiftCount, $R, $V, $Q)
	DIVIDE_STEP2($ShiftCount, $R, $V, $Q)
	MEND
#define DIVIDE_STEP(shiftcount) M_DIVIDE_STEP shiftcount, R, V, QUOT

	MACRO
	M_DIVIDE_INTERLOOP_STEP $ShiftCount, $Label, $R, $V
	DIVIDE_INTERLOOP_STEP0($ShiftCount, $Label, $R, $V)
	DIVIDE_INTERLOOP_STEP1($ShiftCount, $Label, $R, $V)
	MEND
#define DIVIDE_INTERLOOP_STEP($shiftcount, label) \
	M_DIVIDE_INTERLOOP_STEP shiftcount, label, R, V
#else

#define DIVIDE_STEP(shiftcount) \
	DIVIDE_STEP0(shiftcount, R, V, QUOT); \
	DIVIDE_STEP1(shiftcount, R, V, QUOT); \
	DIVIDE_STEP2(shiftcount, R, V, QUOT)

#define DIVIDE_INTERLOOP_STEP(shiftcount, label) \
	DIVIDE_INTERLOOP_STEP0(shiftcount, label, R, V); \
	DIVIDE_INTERLOOP_STEP1(shiftcount, label, R, V) \

#endif


/* IAI-06 */
#ifdef IAI_IDIV
	/*
	 * working register are R, V, and QUOT.
	 * the partial quotient is being developed in QUOT
	 * while the partial remainder is in R.
	 */
#define R	r0
#define V	r1
#define QUOT	r2
#define FLAGS	r3
#define ITER	r12
#define T1	r2
#define T2	r12
#define DIVFLAG	0
#define REMFLAG	4
#define NEGQUO	1
#define NEGREM	2
	/*
	 * A division step is a comparison and conditional subtraction.
	 * We shift the Carry bit from the compare into the quotient,
	 * and diminish the shifted divisor.
	 */

        mov         FLAGS,      #DIVFLAG    
	/* rem merges in here. */
LABEL(_idivOperation)
        orrs        ITER,       R,          V      /* ITER gets trash here. */
        bmi         _idivNegative           
LABEL(_idivPositive)
        cmp         V,          #1          
        bls         _idivByZeroOrOne        /* recognize special cases */

LABEL(_idiv_judge_pow_of_2)
        clz         T1,         V           
        mov         T2,         V,          LSL T1      
        cmp         T2,         #0x80000000 
        beq         _idiv_pow_of_2   
               
LABEL(_idiv_ne_pow_of_2)
	/*
	 * We estimate the size of the quotient by shifting
	 * the divisor, V, until it is >= R.
	 *
	 * n.b. All quotients will require fewer than 32 bits to
	 * represent. In signed 32-bit arithemtic, the only exception to this
	 * would be min_int (0x80000000) / 1. But since we already caught
	 * division by 1 as a special case, that cannot occur.
	 */
        mov         QUOT,       #0          /* initial QUOTIENT */
        mov         ITER,       #0          

	/*
	 * The division interloop is totally unrolled.
	 * Based on the iteration count, we branch into it.
	 */
        DIVIDE_INTERLOOP_STEP(#0,           label_0)    
        DIVIDE_INTERLOOP_STEP(#1,           label_1)    
        DIVIDE_INTERLOOP_STEP(#2,           label_2)    
        DIVIDE_INTERLOOP_STEP(#3,           label_3)    
        DIVIDE_INTERLOOP_STEP(#4,           label_4)    
        DIVIDE_INTERLOOP_STEP(#5,           label_5)    
        DIVIDE_INTERLOOP_STEP(#6,           label_6)    
        DIVIDE_INTERLOOP_STEP(#7,           label_7)    
        DIVIDE_INTERLOOP_STEP(#8,           label_8)    
        DIVIDE_INTERLOOP_STEP(#9,           label_9)    
        DIVIDE_INTERLOOP_STEP(#10,          label_10)   
        DIVIDE_INTERLOOP_STEP(#11,          label_11)   
        DIVIDE_INTERLOOP_STEP(#12,          label_12)   
        DIVIDE_INTERLOOP_STEP(#13,          label_13)   
        DIVIDE_INTERLOOP_STEP(#14,          label_14)   
        DIVIDE_INTERLOOP_STEP(#15,          label_15)   
        DIVIDE_INTERLOOP_STEP(#16,          label_16)   
        DIVIDE_INTERLOOP_STEP(#17,          label_17)   
        DIVIDE_INTERLOOP_STEP(#18,          label_18)   
        DIVIDE_INTERLOOP_STEP(#19,          label_19)   
        DIVIDE_INTERLOOP_STEP(#20,          label_20)   
        DIVIDE_INTERLOOP_STEP(#21,          label_21)   
        DIVIDE_INTERLOOP_STEP(#22,          label_22)   
        DIVIDE_INTERLOOP_STEP(#23,          label_23)   
        DIVIDE_INTERLOOP_STEP(#24,          label_24)   
        DIVIDE_INTERLOOP_STEP(#25,          label_25)   
        DIVIDE_INTERLOOP_STEP(#26,          label_26)   
        DIVIDE_INTERLOOP_STEP(#27,          label_27)   
        DIVIDE_INTERLOOP_STEP(#28,          label_28)   
        DIVIDE_INTERLOOP_STEP(#29,          label_29)   
        DIVIDE_INTERLOOP_STEP(#30,          label_30)   
        DIVIDE_INTERLOOP_STEP(#31,          _idivCorrectResult)     

	/*
	 * The division loop is totally unrolled.
	 * Based on the iteration count, we branch into it.
	 */
LABEL(label_30)
        DIVIDE_STEP(#30)        
LABEL(label_29)
        DIVIDE_STEP(#29)        
LABEL(label_28)
        DIVIDE_STEP(#28)        
LABEL(label_27)
        DIVIDE_STEP(#27)        
LABEL(label_26)
        DIVIDE_STEP(#26)        
LABEL(label_25)
        DIVIDE_STEP(#25)        
LABEL(label_24)
        DIVIDE_STEP(#24)        
LABEL(label_23)
        DIVIDE_STEP(#23)        
LABEL(label_22)
        DIVIDE_STEP(#22)        
LABEL(label_21)
        DIVIDE_STEP(#21)        
LABEL(label_20)
        DIVIDE_STEP(#20)        
LABEL(label_19)
        DIVIDE_STEP(#19)        
LABEL(label_18)
        DIVIDE_STEP(#18)        
LABEL(label_17)
        DIVIDE_STEP(#17)        
LABEL(label_16)
        DIVIDE_STEP(#16)        
LABEL(label_15)
        DIVIDE_STEP(#15)        
LABEL(label_14)
        DIVIDE_STEP(#14)        
LABEL(label_13)
        DIVIDE_STEP(#13)        
LABEL(label_12)
        DIVIDE_STEP(#12)        
LABEL(label_11)
        DIVIDE_STEP(#11)        
LABEL(label_10)
        DIVIDE_STEP(#10)        
LABEL(label_9)
        DIVIDE_STEP(#9)         
LABEL(label_8)
        DIVIDE_STEP(#8)         
LABEL(label_7)
        DIVIDE_STEP(#7)         
LABEL(label_6)
        DIVIDE_STEP(#6)         
LABEL(label_5)
        DIVIDE_STEP(#5)         
LABEL(label_4)
        DIVIDE_STEP(#4)         
LABEL(label_3)
        DIVIDE_STEP(#3)         
LABEL(label_2)
        DIVIDE_STEP(#2)         
LABEL(label_1)
        DIVIDE_STEP(#1)         
LABEL(label_0)
        DIVIDE_STEP(#0)         

LABEL(_idivCorrectResult)
	/* Deliver the correct result with the correct sign
	 * based on the flags we set above.
	 */
        tst         FLAGS,      #REMFLAG    
        bne         _idivDeliverRemainder   
        tst         FLAGS,      #NEGQUO     
        mov         r0,         QUOT        
        rsbne       r0,         r0,         #0          
        mov         pc,         lr          

LABEL(_idivDeliverRemainder)
        tst         FLAGS,      #NEGREM     
        rsbne       r0,         r0,         #0          
        mov         pc,         lr          
	
LABEL(_idivNegative)
	/* one or the other (or both) is negative. */
        cmp         R,          #0          
        rsblt       R,          R,          #0          
        orrlt       FLAGS,      FLAGS,      #(NEGQUO+NEGREM)        
        cmp         V,          #0          
        rsblt       V,          V,          #0          
        eorlt       FLAGS,      FLAGS,      #NEGQUO     
        b           _idivPositive           

LABEL(_idiv_pow_of_2)
        tst         FLAGS,      #REMFLAG    
        bne         _idiv_pow_of_2_DeliverRemainder     
        rsb         T1,         T1,         #31         
        mov         r0,         R,          LSR T1      
        tst         FLAGS,      #NEGQUO     
        rsbne       r0,         r0,         #0          
        mov         pc,         lr          

LABEL(_idiv_pow_of_2_DeliverRemainder)
        tst         FLAGS,      #NEGREM     
        sub         V,          V,          #1          
        and         r0,         R,          V           
        rsbne       r0,         r0,         #0          
        mov         pc,         lr          

	/*
	 * The divisor is 1 <= V <= 0
	 * 0 is a problem, 1 is just a special case.
	 */
LABEL(_idivByZeroOrOne)
	/* go throw an exception */
        blo         SYM_NAME(CVMCCMruntimeThrowDivideByZeroGlueLocal)
        mov         QUOT,       R           
        mov         R,          #0          
        b           _idivCorrectResult      
#else /* !IAI_IDIV */
	/*
	 * working register are R, V, and QUOT.
	 * the partial quotient is being developed in QUOT
	 * while the partial remainder is in R.
	 */
#define R	r0
#define V	r1
#define QUOT	r2
#define FLAGS	r3
#define ITER	r12
#define DIVFLAG	0
#define REMFLAG	4
#define NEGQUO	1
#define NEGREM	2
	/*
	 * A division step is a comparison and conditional subtraction.
	 * We shift the Carry bit from the compare into the quotient,
	 * and diminish the shifted divisor.
	 */
#define DIVIDE_STEP_INSTRUCTIONS 3
#define DIVIDE_STEP_SIZE (DIVIDE_STEP_INSTRUCTIONS*4)
#define MAX_ITER 31

	mov	FLAGS, #DIVFLAG
	/* rem merges in here. */
LABEL(_idivOperation)
	orrs	ITER,R,V	/* ITER gets trash here. */
	bmi	_idivNegative
LABEL(_idivPositive)
	cmp	V,#1
	bls	_idivByZeroOrOne	/* recognize special cases */
	/*
	 * We estimate the size of the quotient by shifting
	 * the divisor, V, until it is >= R.
	 *
	 * n.b. All quotients will require fewer than 32 bits to
	 * represent. In signed 32-bit arithemtic, the only exception to this
	 * would be min_int (0x80000000) / 1. But since we already caught
	 * division by 1 as a special case, that cannot occur.
	 */
	mov	QUOT, #0 	/* initial QUOTIENT */
	mov	ITER, #0
LABEL(_idivIterloop)
	cmp	R, V, LSL ITER
	addhs	ITER, ITER, #1
	bhi	_idivIterloop
LABEL(_idivEndIterloop)

	/*
	 * The division loop is totally unrolled.
	 * Based on the iteration count, we branch into it.
	 */
	rsb	ITER, ITER, #MAX_ITER	 /* offset is 31 - ITER */
	add	ITER, ITER, ITER, LSL #1 /* scaled by DIVIDE_STEP_INSTRUCTIONS */
	add	pc, pc, ITER, LSL #2     /* scaled by bytes per instruction */
	nop				 /* minus one instruction */
	DIVIDE_STEP(#30)
	DIVIDE_STEP(#29)
	DIVIDE_STEP(#28)
	DIVIDE_STEP(#27)
	DIVIDE_STEP(#26)
	DIVIDE_STEP(#25)
	DIVIDE_STEP(#24)
	DIVIDE_STEP(#23)
	DIVIDE_STEP(#22)
	DIVIDE_STEP(#21)
	DIVIDE_STEP(#20)
	DIVIDE_STEP(#19)
	DIVIDE_STEP(#18)
	DIVIDE_STEP(#17)
	DIVIDE_STEP(#16)
	DIVIDE_STEP(#15)
	DIVIDE_STEP(#14)
	DIVIDE_STEP(#13)
	DIVIDE_STEP(#12)
	DIVIDE_STEP(#11)
	DIVIDE_STEP(#10)
	DIVIDE_STEP(#9)
	DIVIDE_STEP(#8)
	DIVIDE_STEP(#7)
	DIVIDE_STEP(#6)
	DIVIDE_STEP(#5)
	DIVIDE_STEP(#4)
	DIVIDE_STEP(#3)
	DIVIDE_STEP(#2)
	DIVIDE_STEP(#1)
	DIVIDE_STEP(#0)

LABEL(_idivCorrectResult)
	/* Deliver the correct result with the correct sign
	 * based on the flags we set above.
	 */
	tst	FLAGS, #REMFLAG
	bne	_idivDeliverRemainder
	tst	FLAGS, #NEGQUO
	mov	r0, QUOT
	rsbne	r0, r0, #0
	mov	pc, lr

LABEL(_idivDeliverRemainder)
	tst	FLAGS, #NEGREM
	rsbne	r0, r0, #0
	mov	pc, lr
	
LABEL(_idivNegative)
	/* one or the other (or both) is negative. */
	cmp	R, #0
	rsblt	R,R,#0
	orrlt	FLAGS,FLAGS,#(NEGQUO+NEGREM)
	cmp	V, #0
	rsblt	V,V,#0
	eorlt	FLAGS,FLAGS,#NEGQUO
	b	_idivPositive

	/*
	 * The divisor is 1 <= V <= 0
	 * 0 is a problem, 1 is just a special case.
	 */
LABEL(_idivByZeroOrOne)
	/* go throw an exception */
	blo	SYM_NAME(CVMCCMruntimeThrowDivideByZeroGlueLocal)
	mov	QUOT, R
	mov	R, #0
	b	_idivCorrectResult
#endif /* !IAI_IDIV */
	

/*
 * Entry for integer remainder.
 * Set a flag and jump into the division routine.
 */
	ENTRY(CVMCCMruntimeIRem)
ENTRY1 ( CVMCCMruntimeIRem )
	ENTRY(CVMCCMruntimeIRem_C)
ENTRY1 ( CVMCCMruntimeIRem_C )
	mov	FLAGS, #REMFLAG
	b	_idivOperation

#undef R
#undef V
#undef QUOT
#undef FLAGS
#undef ITER
#undef DIVFLAG
#undef REMFLAG
#undef NEGQUO
#undef NEGREM
#undef DIVIDE_STEP 
#undef DIVIDE_STEP_INSTRUCTIONS
#undef DIVIDE_STEP_SIZE
#undef MAX_ITER

LABEL(L_EXP_CONSTANT)
        WORD(31+0x3ff-1)

	POOL
