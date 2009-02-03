/*
 * @(#)memory_asm_cpu.S	1.13 06/10/10
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

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s6_memory_asm_cpu)

	ENTRY (CVMARMmemmove8Bit)
ENTRY1(CVMARMmemmove8Bit)
LABEL(CVMARMmemmove8BitLocal)
        /*
          r0 = a1 = dest
          r1 = a2 = src
          r2 = a3 = length
        */

#define DEST    r0
#define SRC     r1
#define LENGTH  r2

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmp     LENGTH, #0
        cmpne   DEST, SRC
        BRCOND_REG(lr,eq)          /* Nothing to copy.  Return to caller. */
        /* If (dest >= src + length) or (dest < src), then we can do forward copying: */
        addhi   r12, SRC, LENGTH
        cmphi   r12, DEST
        bhi     memmove8Bit_inverse

LABEL(memmove8Bit_copyForward)
        /* Do forward copying: */

/* IAI-01 */
#ifdef IAI_MEMMOVE

#ifdef IAI_MEMMOVE_PLD
        pld     [SRC]
        pld     [SRC,	#32]
        pld     [SRC,	#64]
        pld     [DEST]
        pld     [DEST,	#32]
        pld     [DEST,	#64]
#endif
	cmp     LENGTH, #64
	blt     memmove8Bit_smallLength

        /* Check if the DEST and SRC pointers are similarly aligned: */
        eor     r3,	SRC,	 DEST      /* Equal bits will become 0. */
        tst     r3, 	#0x7               /* Check if low 3 bits are equal. */
        bne     memmove8Bit_Doubleunaligned

        /* Do aligned copying: */
        tst     SRC,	#0x7       /* Check if already double word aligned. */
        bne     memmove8Bit_alignedDoubleWordsLeadingByte
        
LABEL(memmove8Bit_alignedDoubleWords)
        wldrd	wR0,	[SRC],	#8
        and     r12,	LENGTH,	#7
        wldrd	wR1,	[SRC],	#8
        sub     LENGTH,	LENGTH,	#24
        wldrd	wR2,	[SRC],	#8
        
L10:        
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#64]
        pld     [DEST, #64]
#endif

        wstrd	wR0,	[DEST],	#8
        wldrd	wR0,	[SRC],	#8
        wstrd	wR1,	[DEST],	#8
        wldrd	wR1,	[SRC],	#8
        wstrd	wR2,	[DEST],	#8
        wldrd	wR2,	[SRC],	#8
        subs	LENGTH,	LENGTH,	#24
        bge     L10
        
LABEL(memmove8Bit_alignedDoubleWordsTrailingTripleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_alignedDoubleWordsTrailingDoubleword
        wstrd	wR0,	[DEST], #8
        wmov	wR0,	wR1        
        wmov	wR1,	wR2        

LABEL(memmove8Bit_alignedDoubleWordsTrailingDoubleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_alignedDoubleWordsTrailingword
        wstrd   wR0,	[DEST], #8
        wmov	wR0,	wR1        

LABEL(memmove8Bit_alignedDoubleWordsTrailingword)
        /* Copy the trailing word if necessary: */
        tst     r12,	#0x4
        beq     memmove8Bit_alignedDoubleWordsTrailingHalfword
        wstrw	wR0,	[DEST],	#4
        waligni	wR0,	wR0,	wR0,	#4
       
LABEL(memmove8Bit_alignedDoubleWordsTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12,	#0x2
        beq     memmove8Bit_alignedDoubleWordsTrailingByte
        wstrh	wR0,	[DEST],	#2
        waligni	wR0,	wR0,	wR0,	#2

LABEL(memmove8Bit_alignedDoubleWordsTrailingByte)
        /* Copy the trailing byte if necessary: */
        tst     r12,	#0x1
        wstrbne	wR0,	[DEST]

LABEL(memmove8Bit_alignedDoubleWordsDone)
	BR_REG(lr)		/* Return to caller. */

LABEL(memmove8Bit_alignedDoubleWordsLeadingByte)
        /* Copy leading byte if necessary: */
        tst     SRC,	#0x1
        beq     memmove8Bit_alignedDoubleWordsLeadingHalfWord
        ldrb    r3,	[SRC], #1
        sub     LENGTH, LENGTH, #1
        strb    r3,	[DEST], #1

LABEL(memmove8Bit_alignedDoubleWordsLeadingHalfWord)
        /* Copy the leading halfword if necessary: */
        tst     SRC, 	#0x2
        beq     memmove8Bit_alignedDoubleWordsLeadingWord
        ldrh    r3, 	[SRC], #2
        sub     LENGTH, LENGTH, #2
        strh    r3, 	[DEST], #2
        
LABEL(memmove8Bit_alignedDoubleWordsLeadingWord)
        /* Copy the leading word if necessary: */
        tst     SRC, 	#0x4
        beq     memmove8Bit_alignedDoubleWords
        ldr     r3, 	[SRC], #4
        sub     LENGTH, LENGTH, #4
        str     r3, 	[DEST], #4
        b       memmove8Bit_alignedDoubleWords

LABEL(memmove8Bit_alignedWordCheck)
        tst     SRC, 	#0x7        /* Check if already double word aligned. */
        bne     memmove8Bit_alignedWordLeadingByte
        
LABEL(memmove8Bit_alignedWord)
        wldrd	wR0,	[SRC],	#8
        sub     LENGTH,	LENGTH,	#28
        wldrd	wR1,	[SRC],	#8
        and     r12,	LENGTH,	#7
        wldrd	wR2,	[SRC],	#8
        wstrw	wR0,	[DEST],	#4
        
LABEL(L11)
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#64]
        pld     [DEST, #64]
#endif

        waligni	wR3,	wR0,	wR1,	#4
        wldrd	wR0,	[SRC],	#8
        wstrd	wR3,	[DEST],	#8
        waligni	wR4,	wR1,	wR2,	#4
        wldrd	wR1,	[SRC],	#8
        wstrd	wR4,	[DEST],	#8
        waligni	wR5,	wR2,	wR0,	#4
        wldrd	wR2,	[SRC],	#8
        wstrd	wR5,	[DEST],	#8
        subs	LENGTH,	LENGTH,	#24
        bge     L11	
        
LABEL(memmove8Bit_alignedWordTrailingTripleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_alignedWordTrailingDoubleword
        waligni	wR3,	wR0,	wR1,	#4
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        wldrd	wR2,	[SRC],	#8
        wstrd	wR3,	[DEST],	#8

LABEL(memmove8Bit_alignedWordTrailingDoubleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_alignedWordTrailingword
        waligni	wR3,	wR0,	wR1,	#4
        wstrd	wR3,	[DEST],	#8
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        
LABEL(memmove8Bit_alignedWordTrailingword)
        /* Copy the trailing word if necessary: */
        waligni	wR2,	wR0,	wR1,	#4
        tst     r12,	#0x4
        beq     memmove8Bit_alignedWordTrailingHalfword
        wstrw	wR2,	[DEST],	#4
        waligni	wR2,	wR2,	wR2,	#4
       
LABEL(memmove8Bit_alignedWordTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12, 	#0x2
        beq     memmove8Bit_alignedWordTrailingByte
        wstrh	wR2,	[DEST],	#2
        waligni	wR2,	wR2,	wR2,	#2

LABEL(memmove8Bit_alignedWordTrailingByte)
        /* Copy the trailing byte if necessary: */
        tst     r12,	#0x1
        wstrbne	wR2,	[DEST]

LABEL(memmove8Bit_alignedWordDone)
	BR_REG(lr)		/* Return to caller. */

LABEL(memmove8Bit_alignedWordLeadingByte)
        /* Copy leading byte if necessary: */
        tst     SRC, 	#0x1
        beq     memmove8Bit_alignedWordLeadingHalfWord
        ldrb    r3, 	[SRC], #1
        sub     LENGTH, LENGTH, #1
        strb    r3, 	[DEST], #1

LABEL(memmove8Bit_alignedWordLeadingHalfWord)
        /* Copy the leading halfword if necessary: */
        tst     SRC, 	#0x2
        beq     memmove8Bit_alignedWordLeadingWord
        ldrh    r3, 	[SRC], #2
        sub     LENGTH, LENGTH, #2
        strh    r3, 	[DEST], #2
        
LABEL(memmove8Bit_alignedWordLeadingWord)
        /* Copy the leading word if necessary: */
        tst     SRC, 	#0x4
        beq     memmove8Bit_alignedWord
        bic     SRC,	SRC,	#7
        wldrd	wR0,	[SRC],	#8
        sub     LENGTH,	LENGTH,	#24
        wldrd	wR1,	[SRC],	#8
        and     r12,	LENGTH,	#7
        wldrd	wR2,	[SRC],	#8
        b       L11
        
LABEL(memmove8Bit_Doubleunaligned)
        tst     r3, 	#0x3          /* Check if low 2 bits are equal. */
        beq     memmove8Bit_alignedWordCheck

        stmfd	sp!,	{r4-r7,lr}
        
        and     r3,	SRC,	#7	/*r3 = number of src unaligned */
        and     r5,	DEST,	#7	/*r5 = number of DEST unaligned */
        subs	r7,	r3,	r5	
        blt     LM2
        
        /* Do alignment for the starting part.
           
           If the unaligned byte number of src is bigger than that of dest,
                                                                |<-src
           wR1:wR0 -> s12 s11 s10 s9 s8 s7 s6 s5 : s4 s3 s2 s1 s0 x2 x1 x0
           
           wR3     ->                              y7 y6 y5 y4 y3 y2 y1 y0
                                                                      |<-dest
	                                                              
           Result  ->                              s6 s5 s4 s3 s2 s1 s0 y0

           Step 1  ->                              s6 s5 s4 s3 s2 s1 s0 x2
           Step 2  ->                              y0 s6 s5 s4 s3 s2 s1 s0
           Step 3  ->                              s6 s5 s4 s3 s2 s1 s0 y0
        */   
	        
        bic     r6,	DEST,	#7		/*r6 = address 8-byte aligned */
        wldrd	wR3,	[r6]
        bic     r4,	SRC,	#7		/*r4 = address 8-byte aligned */
        wldrd	wR0,	[r4],	#8
        tmcr	wCGR0,	r7			/*wCGR0 = number of bytes to be aligned */
        wldrd	wR1,	[r4],	#8
        tmcr	wCGR1,	r5			/*wCGR1 = number of bytes to be aligned */
        rsb     r12,	r5,		#8
        sub     LENGTH,	LENGTH,	r12
        mov     r12,	r12,	LSL	#3
        tmcr	wCGR2,	r12			/*wCGR2 = number of bits to be rotated */
        walignr0	wR2,	wR0,	wR1	/* Step 1 */
        wmov	wR0,	wR1
        wldrd	wR1,	[r4],	#8
        sub     LENGTH,	LENGTH,	#24	
        walignr1	wR4,	wR2,	wR3	/* Step 2 */
        wldrd	wR2,	[r4],	#8
        wrordg	wR4,	wR4,	wCGR2		/* Step 3 */
        wstrd	wR4,	[r6],	#8
	
        /* The first unaligned 8-byte has been done. Begin mainloop
 	 * of 8-byte memmove.
	 */
LABEL(LM1)     
#ifdef IAI_MEMMOVE_PLD2
        pld     [r4,	#64]
        pld     [r6, #64]
#endif

        walignr0	wR3,	wR0,	wR1
        wldrd	wR0,	[r4],	#8
        wstrd	wR3,	[r6],	#8
        walignr0	wR4,	wR1,	wR2
        wldrd	wR1,	[r4],	#8
        wstrd	wR4,	[r6],	#8
        walignr0	wR5,	wR2,	wR0
        wldrd	wR2,	[r4],	#8
        wstrd	wR5,	[r6],	#8
        subs	LENGTH,	LENGTH,	#24
        bge     LM1	
         
        /*
         * Do the end unaligned part
         */
LABEL(L101)
        adds	LENGTH, LENGTH,	#8
        blt     L102
        walignr0	wR3,	wR0,	wR1
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        wldrd	wR2,	[r4],	#8
        wstrd	wR3,	[r6],	#8

LABEL(L102)
        adds	LENGTH, LENGTH,	#8
        blt     L103
        walignr0	wR3,	wR0,	wR1
        wstrd	wR3,	[r6],	#8
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        
LABEL(L103)
        walignr0	wR3,	wR0,	wR1
        tst     LENGTH,	#4
        beq     L104
        wstrw	wR3,	[r6],	#4
        waligni	wR3,	wR3,	wR3,	#4
          
LABEL(L104)
        tst     LENGTH,	#2
        beq     L105
        wstrh	wR3,	[r6],	#2
        waligni	wR3,	wR3,	wR3,	#2

LABEL(L105)
        tst     LENGTH,	#1
        wstrbne	wR3,	[r6]

        ldmfd	sp!,	{r4-r7,lr}     
        BR_REG(lr)	          /* Return to caller. */
        
LABEL(LM2)
        
        /* Do alignment for the starting part.
           
           If the unaligned byte number of src is smaller than that of dest,
                                         |<-src
           wR0     -> s6 s5 s4 s3 s2 s1 s0 x0
           
           wR3     -> y7 y6 y5 y4 y3 y2 y1 y0
	                               |<-dest
	                                                              
           Result  -> s4 s3 s2 s1 s0 y2 y1 y0.
	   
           Step 1  -> y2 y1 y0 z4 z3 z2 z1 z0
           Step 2  -> z0 s6 s5 s4 s3 s2 s1 s0
           Step 3  -> s4 s3 s2 s1 s0 y2 y1 y0
        */   

        add     r7,	r7,	#8
        bic     r6,	DEST,	#7		/*r6 = address 8-byte aligned */
        wldrd	wR3,	[r6]
        bic     r4,	SRC,	#7		/*r4 = address 8-byte aligned */
        wldrd	wR0,	[r4],	#8
        tmcr	wCGR2,	r5			/*wCGR0 = number of bytes to be aligned */
        tmcr	wCGR1,	r3			/*wCGR1 = number of bytes to be aligned */
        tmcr	wCGR0,	r7			/*wCGR2 = number of bytes to be aligned */
        rsb     r12,	r5,		#8
        sub     LENGTH,	LENGTH,	r12
        tmcr	wCGR3,	r12			/*wCGR3 = number of bytes to be aligned */
        walignr2	wR2,	wR1,	wR3	/* Step 1 */
        walignr1	wR4,	wR0,	wR1	/* Step 2 */
        wldrd	wR1,	[r4],	#8
        sub     LENGTH,	LENGTH,	#24
        walignr3	wR4,	wR2,	wR4	/* Step 3 */
        wldrd	wR2,	[r4],	#8
        wstrd	wR4,	[r6],	#8

        /*
        The first unaligned 8-byte has been done. Begin mainloop of 8-byte memmove.
        */

        b       LM1
	
LABEL(memmove8Bit_smallLength)
#endif /* IAI_MEMMOVE */

        tst     SRC, #0x3               /* Check if already word aligned. */
        bne     memmove8Bit_alignedLeadingByte

LABEL(memmove8Bit_alignedWords)
        and     r12, LENGTH, #0x3
        bic     LENGTH, LENGTH, #0x3
        subs    LENGTH, LENGTH, #4
        blt     memmove8Bit_alignedWordsTrailingHalfword

	/* Check if the DEST is word aligned. The SRC
	 * is already word aligned when we get here.
         */
	tst	DEST, #0x3		/* Check if low 2 bits are 0. */
	bne     memmove8Bit_unalignedLoop

LABEL(memmove8Bit_alignedWordsLoop)
        ldr     r3, [SRC], #4
        subs    LENGTH, LENGTH, #4
        str     r3, [DEST], #4
        bge     memmove8Bit_alignedWordsLoop

LABEL(memmove8Bit_alignedWordsTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12, #0x2
        beq     memmove8Bit_alignedTrailingByte
        ldrh    r3, [SRC], #2

	/* Can't do strh because the DEST pointer
	 * is not guaranteed to be half-word aligned.
	 */
#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
	mov	r3, r3, ror #8
	strb	r3, [DEST], #1
	mov	r3, r3, ror #24
	strb	r3, [DEST], #1

#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
	strb	r3, [DEST], #1
	mov	r3, r3, lsr #8
	strb	r3, [DEST], #1
#endif

LABEL(memmove8Bit_alignedTrailingByte)
        /* Copy the trailing byte if necessary: */
        tst     r12, #0x1
	BRCOND_REG(lr,eq)	/* Done. Return to caller. */
        ldrb    r3, [SRC]
        strb    r3, [DEST]

LABEL(memmove8Bit_alignedWordsDone)
        BR_REG(lr)	          /* Return to caller. */

LABEL(memmove8Bit_alignedLeadingByte)
        /* Copy leading byte if necessary: */
        tst     SRC, #0x1
        beq     memmove8Bit_alignedLeadingHalfWord
        ldrb    r3, [SRC], #1
        subs    LENGTH, LENGTH, #1
        strb    r3, [DEST], #1
	BRCOND_REG(lr,ls)	/* Done. LENGTH = 1 */	

LABEL(memmove8Bit_alignedLeadingHalfWord)
        /* Copy the leading halfword if necessary: */
        tst     SRC, #0x2
        beq     memmove8Bit_alignedWords
        ldrh    r3, [SRC], #2
        subs     LENGTH, LENGTH, #2
	/* Can't do strh because the DEST pointer
	 * is not guaranteed to be half-word aligned.
	 */
#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
	mov	r3, r3, ror #8
	strb	r3, [DEST], #1
	mov	r3, r3, ror #24
	strgeb	r3, [DEST], #1  /* LENGTH >= 2 */

#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
	strb	r3, [DEST], #1
	mov	r3, r3, lsr #8
	strgeb	r3, [DEST], #1	/* LENGTH >= 2 */
#endif
        bgt       memmove8Bit_alignedWords
	BR_REG(lr)	/* Done. */

LABEL(memmove8Bit_unalignedLoop)
	/* The SRC pointer is word aligned when we get here. The
	 * DEST pointer however is not.
         */
        ldr     r3, [SRC], #4
        subs    LENGTH, LENGTH, #4
#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
	mov	r3, r3, ror #24
	strb	r3, [DEST], #1
	mov	r3, r3, ror #24
	strb	r3, [DEST], #1
	mov	r3, r3, ror #24
	strb	r3, [DEST], #1
	mov	r3, r3, ror #24
	strb	r3, [DEST], #1

#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
	strb	r3, [DEST], #1
	mov	r3, r3, lsr #8
	strb	r3, [DEST], #1
	mov	r3, r3, lsr #8
	strb	r3, [DEST], #1
	mov	r3, r3, lsr #8
	strb	r3, [DEST], #1
#endif
        bge     memmove8Bit_unalignedLoop
	b	memmove8Bit_alignedWordsTrailingHalfword /* Loop done */

        /* ===============================================================*/
LABEL(memmove8Bit_inverse)

/* IAI-01 */
#ifdef IAI_MEMMOVE
        cmp     LENGTH,	#64
        blt     memmove8Bit_inverse_smallLength


#ifdef IAI_MEMMOVE_PLD
        pld     [SRC]
        pld     [SRC,	#32]
        pld     [SRC,	#64]
        pld     [DEST]
        pld     [DEST,	#32]
        pld     [DEST,	#64]
#endif
        add     SRC,	SRC,	LENGTH
        add 	DEST,	DEST,	LENGTH

       /* Check if the DEST and SRC pointers are similarly aligned: */
        eor     r3, 	SRC, 	DEST        /* Equal bits will become 0. */
        tst     r3, 	#0x7                /* Check if low 3 bits are equal. */
        bne     memmove8Bit_inverse_unaligned

        /* Do aligned copying: */
        tst     SRC, 	#0x7               /* Check if already double word aligned. */
        bne     memmove8Bit_inverse_alignedDoubleWordsLeadingByte
        
LABEL(memmove8Bit_inverse_alignedDoubleWords)
        wldrd	wR0,	[SRC,	#-8]!
        and     r12,	LENGTH,	#7
        wldrd	wR1,	[SRC,	#-8]!
        sub     LENGTH,	LENGTH,	#24
        wldrd	wR2,	[SRC,	#-8]!
        
LABEL(IL10)
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#-64]
        pld     [DEST, #-64]
#endif

        wstrd	wR0,	[DEST,	#-8]!
        wldrd	wR0,	[SRC,	#-8]!
        wstrd	wR1,	[DEST,	#-8]!
        wldrd	wR1,	[SRC,	#-8]!
        wstrd	wR2,	[DEST,	#-8]!
        wldrd	wR2,	[SRC,	#-8]!
        subs	LENGTH,	LENGTH,	#24
        bge     IL10
        
LABEL(memmove8Bit_inverse_alignedDoubleWordsTrailingTripleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_inverse_alignedDoubleWordsTrailingDoubleword
        wstrd	wR0,	[DEST,	#-8]!
        wmov	wR0,	wR1        
        wmov	wR1,	wR2        

LABEL(memmove8Bit_inverse_alignedDoubleWordsTrailingDoubleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_inverse_alignedDoubleWordsTrailingword
        wstrd	wR0,	[DEST,	#-8]!
        wmov	wR0,	wR1        

LABEL(memmove8Bit_inverse_alignedDoubleWordsTrailingword)
        /* Copy the trailing word if necessary: */
        tst     r12,	#0x4
        beq     memmove8Bit_inverse_alignedDoubleWordsTrailingHalfword
        waligni	wR0,	wR0,	wR0,	#4
        wstrw	wR0,	[DEST,	#-4]!
	       
LABEL(memmove8Bit_inverse_alignedDoubleWordsTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12,	#0x2
        beq     memmove8Bit_inverse_alignedDoubleWordsTrailingByte
        waligni	wR0,	wR0,	wR0,	#6
        wstrh	wR0,	[DEST,	#-2]!

LABEL(memmove8Bit_inverse_alignedDoubleWordsTrailingByte)
        /* Copy the trailing byte if necessary: */
        tst     r12,	#0x1
        walignine	wR0,	wR0,	wR0,	#7
        wstrbne	wR0,	[DEST,	#-1]

LABEL(memmove8Bit_inverse_alignedDoubleWordsDone)
        BR_REG(lr)	          /* Return to caller. */

LABEL(memmove8Bit_inverse_alignedDoubleWordsLeadingByte)
        /* Copy leading byte if necessary: */
        tst     SRC, 	#0x1
        beq     memmove8Bit_inverse_alignedDoubleWordsLeadingHalfWord
        ldrb    r3, 	[SRC, #-1]!
        sub     LENGTH, LENGTH, #1
        strb    r3, 	[DEST, #-1]!

LABEL(memmove8Bit_inverse_alignedDoubleWordsLeadingHalfWord)
        /* Copy the leading halfword if necessary: */
        tst     SRC, 	#0x2
        beq     memmove8Bit_inverse_alignedDoubleWordsLeadingWord
        ldrh    r3, 	[SRC, #-2]!
        sub     LENGTH, LENGTH, #2
        strh    r3, 	[DEST, #-2]!
        
LABEL(memmove8Bit_inverse_alignedDoubleWordsLeadingWord)
        /* Copy the leading word if necessary: */
        tst     SRC, 	#0x4
        beq     memmove8Bit_inverse_alignedDoubleWords
        ldr     r3, 	[SRC, #-4]!
        sub     LENGTH, LENGTH, #4
        str     r3, 	[DEST, #-4]!
        b       memmove8Bit_inverse_alignedDoubleWords

LABEL(memmove8Bit_inverse_alignedWordCheck)
        tst     SRC, 	#0x7               /* Check if already double word aligned. */
        bne     memmove8Bit_inverse_alignedWordLeadingByte
        
LABEL(memmove8Bit_inverse_alignedWord)
        wldrd	wR0,	[SRC,	#-8]!
        ldr     r12,	[SRC,	#4]
        wldrd	wR1,	[SRC,	#-8]!
        sub     LENGTH,	LENGTH,	#28
        str     r12,	[DEST,	#-4]!
        wldrd	wR2,	[SRC,	#-8]!
        and     r12,	LENGTH,	#7
        
LABEL(IL11)
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#-64]
        pld     [DEST, #-64]
#endif

        waligni	wR3,	wR1,	wR0,	#4
        wldrd	wR0,	[SRC,	#-8]!
        wstrd	wR3,	[DEST,	#-8]!
        waligni	wR4,	wR2,	wR1,	#4
        wldrd	wR1,	[SRC,	#-8]!
        wstrd	wR4,	[DEST,	#-8]!
        waligni	wR5,	wR0,	wR2,	#4
        wldrd	wR2,	[SRC,	#-8]!
        wstrd	wR5,	[DEST,	#-8]!
        subs	LENGTH,	LENGTH,	#24
        bge     IL11	
        
LABEL(memmove8Bit_inverse_alignedWordTrailingTripleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_inverse_alignedWordTrailingDoubleword
        waligni	wR3,	wR1,	wR0,	#4
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        wldrd	wR2,	[SRC,	#-8]!
        wstrd	wR3,	[DEST,	#-8]!

LABEL(memmove8Bit_inverse_alignedWordTrailingDoubleword)
        adds	LENGTH, LENGTH,	#8
        blt     memmove8Bit_inverse_alignedWordTrailingword
        waligni	wR3,	wR1,	wR0,	#4
        wstrd	wR3,	[DEST,	#-8]!
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        
LABEL(memmove8Bit_inverse_alignedWordTrailingword)
        /* Copy the trailing word if necessary: */
        tst     r12,	#0x4
        beq     memmove8Bit_inverse_alignedWordTrailingHalfword
        wstrw	wR0,	[DEST,	#-4]!
        waligni	wR0,	wR1,	wR1,	#4
       
LABEL(memmove8Bit_inverse_alignedWordTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12, 	#0x2
        beq     memmove8Bit_inverse_alignedWordTrailingByte
        waligni	wR1,	wR0,	wR0,	#2
        wstrh	wR1,	[DEST,	#-2]!
        waligni	wR0,	wR0,	wR0,	#6

LABEL(memmove8Bit_inverse_alignedWordTrailingByte)
        /* Copy the trailing byte if necessary: */
        tst     r12, 	#0x1
        walignine	wR0,	wR0,	wR0,	#3
        wstrbne	wR0,	[DEST,	#-1]

LABEL(memmove8Bit_inverse_alignedWordDone)
        BR_REG(lr)	          /* Return to caller. */

LABEL(memmove8Bit_inverse_alignedWordLeadingByte)
        /* Copy leading byte if necessary: */
        tst     SRC, 	#0x1
        beq     memmove8Bit_inverse_alignedWordLeadingHalfWord
        ldrb    r3, 	[SRC, #-1]!
        sub     LENGTH, LENGTH, #1
        strb    r3, 	[DEST, #-1]!

LABEL(memmove8Bit_inverse_alignedWordLeadingHalfWord)
        /* Copy the leading halfword if necessary: */
        tst     SRC, 	#0x2
        beq     memmove8Bit_inverse_alignedWordLeadingWord
        ldrh    r3, 	[SRC, #-2]!
        sub     LENGTH, LENGTH, #2
        strh    r3, 	[DEST, #-2]!
        
LABEL(memmove8Bit_inverse_alignedWordLeadingWord)
        /* Copy the leading word if necessary: */
        tst     SRC, 	#0x4
        beq     memmove8Bit_inverse_alignedWord
        wldrw	wR0,	[SRC,	#-4]!
        sub     LENGTH,	LENGTH,	#24
        wldrd	wR1,	[SRC,	#-8]!
        and     r12,	LENGTH,	#7
        wldrd	wR2,	[SRC,	#-8]!
        b       IL11
        
LABEL(memmove8Bit_inverse_unaligned)
        tst     r3, 	#0x3                /* Check if low 2 bits are equal. */
        beq     memmove8Bit_inverse_alignedWordCheck        
        
        stmfd	sp!,	{r4-r7,lr}
        
        and     r3,	SRC,	#7	/*r3 = number of SRC end unaligned */
        and     r5,	DEST,	#7	/*r5 = number of DEST end unaligned */
        subs	r7,	r3,	r5	/*r7 = difference of unaligned */
        blt     IL2

        /* Do alignment for the end part.
           
           If the unaligned byte number of src is bigger than that of dest,
                                   |<-src end  
           wR0	   -> x4 x3 x2 x1 x0 s2 s1 s0

           wR3     -> y5 y4 y3 y2 y1 y0 d1 d0
                                      |<-dest end
	                                                              
	       Result  -> y5 y4 y3 y2 y1 y0 s2 s1
	   
           Step 1  -> xx xx x3 x2 x1 x0 s2 s1 (wR2)
           Step 2  -> s2 s1 y5 y4 y3 y2 y1 y0 (WR4)
           Step 3  -> y5 y4 y3 y2 y1 y0 s2 s1
        */   

        bic     r6,	DEST,	#7		/*r6 = address 8-byte aligned */
        wldrd	wR3,	[r6]
        bic     r4,	SRC,	#7		/*r4 = address 8-byte aligned */
        wldrd	wR0,	[r4]
        tmcr	wCGR0,	r7			/*wCGR0 = number of bytes to be aligned */
        wldrd	wR1,	[r4,	#-8]!
        tmcr	wCGR1,	r5			/*wCGR1 = number of bytes to be aligned */
        rsb     r12,	r5,		#8
        sub     LENGTH,	LENGTH,	r5
        mov     r12,	r12,	LSL	#3
        tmcr	wCGR2,	r12			/*wCGR2 = number of bits to be rotated */
        walignr0	wR2,	wR0,	wR0	/* Step 1 */
        sub     LENGTH,	LENGTH,	#24	
        walignr1	wR4,	wR3,	wR2	/* Step 2 */
        wldrd   wR2,	[r4,	#-8]!
        wrordg	wR4,	wR4,	wCGR2		/* Step 3 */
        wstrd	wR4,	[r6]
	
        /*
        The first unaligned 8-byte has been done. Begin mainloop of 8-byte memmove.
        */
LABEL(IL1)
#ifdef IAI_MEMMOVE_PLD2
        pld     [r4,	#-64]
        pld     [r6,     #-64]
#endif

        walignr0	wR3,	wR1,	wR0
        wldrd	wR0,	[r4,	#-8]!
        wstrd	wR3,	[r6,	#-8]!
        walignr0	wR4,	wR2,	wR1
        wldrd	wR1,	[r4,	#-8]!
        wstrd	wR4,	[r6,	#-8]!
        walignr0	wR5,	wR0,	wR2
        wldrd	wR2,	[r4,	#-8]!
        wstrd	wR5,	[r6,	#-8]!
        subs	LENGTH,	LENGTH,	#24
        bge     IL1	

        /*
        Do the end unaligned part
        */
LABEL(IL101)
        adds	LENGTH, LENGTH,	#8
        blt     IL102
        walignr0	wR3,	wR1,	wR0
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        wldrd	wR2,	[r4,	#-8]!
        wstrd	wR3,	[r6,	#-8]!

LABEL(IL102)
        adds	LENGTH, LENGTH,	#8
        blt     IL103
        walignr0	wR3,	wR1,	wR0
        wstrd	wR3,	[r6,	#-8]!
        wmov	wR0,	wR1
        wmov	wR1,	wR2
        
LABEL(IL103)
        walignr0	wR3,	wR1,	wR0
        tst     LENGTH,	#4
        beq     IL104
        waligni	wR3,	wR3,	wR3,	#4
        wstrw	wR3,	[r6,	#-4]!
          
LABEL(IL104)
        tst     LENGTH,	#2
        beq     IL105
        waligni	wR3,	wR3,	wR3,	#6
        wstrh	wR3,	[r6,	#-2]!

LABEL(IL105)
        tst     LENGTH,	#1
        walignine	wR3,	wR3,	wR3,	#7
        wstrbne	wR3,	[r6,	#-1]

        ldmfd	sp!,	{r4-r7,lr}     
        BR_REG(lr)	          /* Return to caller. */
                

        /*
        unaligned number of SRC end is less than the one of DEST end 
        */
LABEL(IL2)
        /* Do alignment for the end part.
           
           If the unaligned byte number of src is bigger than that of dest,
                                         |<-src end  
           wR0:wR1 -> x5 x4 x3 x2 x1 x0 s9 s8 : s7 s6 s5 s4 s3 s2 s1 s0
           
           wR3     ->                           y4 y3 y2 y1 y0 d2 d1 d0
	                                                         |<-dest end
	                                                              
           Result  ->                           y4 y3 y2 y1 y0 s9 s8 s7
	   
           Step 1  ->                           s9 s8 s7 s6 s5 s4 s3 s2 (wR2)
           Step 2  ->                           yy yy yy y4 y3 y2 y1 y0 (wR4)
           Step 3  ->                           y4 y3 y2 y1 y0 s9 s8 s7
        */   

	        
        add     r7,	r7,	#8
        bic     r6,	DEST,	#7		/*r6 = address 8-byte aligned */
        wldrd	wR3,	[r6]
        bic     r4,	SRC,	#7		/*r4 = address 8-byte aligned */
        wldrd	wR0,	[r4]
        wldrd	wR1,	[r4,	#-8]!
        tmcr	wCGR2,	r3			/*wCGR0 = number of bytes to be aligned */
        tmcr	wCGR1,	r5			/*wCGR1 = number of bytes to be aligned */
        tmcr	wCGR0,	r7			/*wCGR2 = number of bytes to be aligned */
        rsb     r12,	r5,	#8
        sub     LENGTH,	LENGTH,	r5
        tmcr	wCGR3,	r12			/*wCGR3 = number of bytes to be aligned */
        walignr2	wR2,	wR1,	wR0	/* Step 1 */
        walignr1	wR4,	wR3,	wR3	/* Step 2 */
        wmov	wR0,	wR1
        wldrd	wR1,	[r4,	#-8]!
        sub     LENGTH,	LENGTH,	#24
        walignr3	wR4,	wR2,	wR4	/* Step 3 */
        wldrd	wR2,	[r4,	#-8]!
        wstrd	wR4,	[r6]

        /*
        The first unaligned 8-byte has been done. Begin mainloop of 8-byte memmove.
        */

        b       IL1
        
	
LABEL(memmove8Bit_inverse_smallLength)
#endif /* IAI_MEMMOVE */

LABEL(memmove8Bit_copyBackward)
        /* Do backward copying: */
        subs    LENGTH, LENGTH, #1
        add     SRC, SRC, LENGTH
        add     DEST, DEST, LENGTH
        blt     memmove8Bit_copyBackwardLoopDone

LABEL(memmove8Bit_copyBackwardLoop)
        ldrb    r3, [SRC], #-1
        subs    LENGTH, LENGTH, #1
        strb    r3, [DEST], #-1
        bge     memmove8Bit_copyBackwardLoop

LABEL(memmove8Bit_copyBackwardLoopDone)
        BR_REG(lr)	          /* Return to caller. */

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMARMmemmove8Bit )

	ENTRY (CVMARMmemmove16Bit)
ENTRY1(CVMARMmemmove16Bit)
LABEL(CVMARMmemmove16BitLocal)
        /*
          r0 = a1 = dest
          r1 = a2 = src
          r2 = a3 = length
        */

#define DEST    r0
#define SRC     r1
#define LENGTH  r2

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmp     LENGTH, #0
        cmpne   DEST, SRC
        BRCOND_REG(lr,eq)          /* Nothing to copy.  Return to caller. */
	/* If (dest >= src + length) or (dest < src), then we can do forward copying: */
        addhi     r12, SRC, LENGTH
        cmphi     r12, DEST
	bhi	  memmove16Bit_copyBackward

LABEL(memmove16Bit_copyForward)
        tst     SRC, #0x3       /* Check low 2 bits for word alignment. */
        beq     memmove16Bit_alignedWords

        /* Copy the leading halfword if necessary: */
        subs    LENGTH, LENGTH, #2              /* If there's nothing to */
        blt     memmove16Bit_alignedWordsDone   /* copy,then branch to done. */
        ldrh    r3, [SRC], #2
        strh    r3, [DEST], #2

LABEL(memmove16Bit_alignedWords)
        and     r12, LENGTH, #0x3
        bic     LENGTH, LENGTH, #0x3
        subs    LENGTH, LENGTH, #4
        blt     memmove16Bit_alignedWordsTrailingHalfword

	/* Check if the DEST is word aligned. The SRC
	 * is already word algined when we get here.
         */
	tst	DEST, #0x3	/* Check if the low 2 bits are 0. */
	bne     memmove16Bit_unalignedLoop

LABEL(memmove16Bit_alignedWordsLoop)
        ldr     r3, [SRC], #4
        subs    LENGTH, LENGTH, #4
        str     r3, [DEST], #4
        bge     memmove16Bit_alignedWordsLoop

LABEL(memmove16Bit_alignedWordsTrailingHalfword)
        /* Copy the trailing half word if necessary: */
        tst     r12, #0x2
	BRCOND_REG(lr,eq)	/* Done. Return to caller. */
        ldrh    r3, [SRC], #2
        strh    r3, [DEST], #2

LABEL(memmove16Bit_alignedWordsDone)
        BR_REG(lr)	          /* Return to caller. */

LABEL(memmove16Bit_unalignedLoop)
	/* The SRC pointer is word aligned when we get here. The
	 * DEST pointer however is not. */
	ldr     r3, [SRC], #4
        subs    LENGTH, LENGTH, #4

#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
	mov	r3, r3, ror #16
	strh	r3, [DEST], #2
	mov	r3, r3, ror #16
	strh	r3, [DEST], #2

#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
	strh	r3, [DEST], #2
	mov	r3, r3, lsr #16
	strh	r3, [DEST], #2
#endif
	bge     memmove16Bit_unalignedLoop
	b	memmove16Bit_alignedWordsTrailingHalfword	

        /* ===============================================================*/

LABEL(memmove16Bit_copyBackward)
        /* Do backward copying: */
        subs    LENGTH, LENGTH, #2
        add     SRC, SRC, LENGTH
        add     DEST, DEST, LENGTH
        blt     memmove16Bit_copyBackwardLoopDone

LABEL(memmove16Bit_copyBackwardLoop)
        ldrh    r3, [SRC], #-2
        subs    LENGTH, LENGTH, #2
        strh    r3, [DEST], #-2
        bge     memmove16Bit_copyBackwardLoop

LABEL(memmove16Bit_copyBackwardLoopDone)
        BR_REG(lr)	          /* Return to caller. */

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMARMmemmove16Bit )

	ENTRY (CVMARMmemmove32Bit)
ENTRY1(CVMARMmemmove32Bit)
LABEL(CVMARMmemmove32BitLocal)
        /*
          r0 = a1 = dest
          r1 = a2 = src
          r2 = a3 = length
        */

#define DEST    r0
#define SRC     r1
#define LENGTH  r2

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmp     LENGTH, #0
        cmpne   DEST, SRC
        BRCOND_REG(lr,eq)          /* Nothing to copy.  Return to caller. */
        bhi     memmove32Bit_inverse

LABEL(memmove32Bit_copyForward)
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#32]
        pld     [SRC,	#64]
        pld     [SRC,	#96]
        pld     [DEST, #32]
        pld     [DEST, #64]
        pld     [DEST, #96]
        
#endif

        /* Do forward copying: */
        subs    LENGTH, LENGTH, #4
        blt     memmove32Bit_alignedWordsDone

LABEL(memmove32Bit_alignedWordsLoop)
        ldr     r3, [SRC], #4
        subs    LENGTH, LENGTH, #4
        str     r3, [DEST], #4
        bge     memmove32Bit_alignedWordsLoop

LABEL(memmove32Bit_alignedWordsDone)
        BR_REG(lr)	          /* Return to caller. */

LABEL(memmove32Bit_inverse)
        /* Do backward copying: */
#ifdef IAI_MEMMOVE_PLD2
        pld     [SRC,	#-32]
        pld     [SRC,	#-64]
        pld     [SRC,	#-96]

        pld     [DEST, #-32]
        pld     [DEST, #-64]
        pld     [DEST, #-96]
#endif

        subs    LENGTH, LENGTH, #4
        add     SRC, SRC, LENGTH
        add     DEST, DEST, LENGTH
        blt     memmove32Bit_copyBackwardLoopDone

LABEL(memmove32Bit_copyBackwardLoop)
        ldr     r3, [SRC], #-4
        subs    LENGTH, LENGTH, #4
        str     r3, [DEST], #-4
        bge     memmove32Bit_copyBackwardLoop

LABEL(memmove32Bit_copyBackwardLoopDone)
        BR_REG(lr)	          /* Return to caller. */

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMARMmemmove32Bit )

