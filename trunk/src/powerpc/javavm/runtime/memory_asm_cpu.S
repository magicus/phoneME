/*
 * %W% %E%
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

/*
 * These don't seem to help, so we keep them disabled.
 */
#if 0

#include "javavm/include/asmmacros_cpu.h"

        .file           "memory_asm_cpu.S"

ENTRY ( CVMPPCmemmove8Bit )
        /*
          r3 = dest
          r4 = src
          r5 = length
        */

#define DEST    r3
#define SRC     r4
#define LENGTH  r5

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmpwi	LENGTH, 0
	beqlr-
        cmplw	DEST, SRC
        beqlr-		/* Nothing to copy.  Return to caller. */
        bgt-	memmove8Bit_inverse

memmove8Bit_copyForward:
        /* Do forward copying: */

        /* Get at least SRC 32-bit aligned for better performance: */
memmove8Bit_alignWordLoop:	
        andi.	r0, SRC, 0x3	/* Check low 2 bits for 32-bit alignment. */
        beq+	memmove8Bit_alignedWordsLoop
        lbz	r0, 0(SRC)
	addi	SRC, SRC, 1
        subic.	LENGTH, LENGTH, 1
        stb	r0, 0(DEST)
	addi	DEST, DEST, 1
        beqlr-		/* Return to caller. */
	b	memmove8Bit_alignWordLoop

memmove8Bit_alignedWordsLoop:
        subic.	LENGTH, LENGTH, 4
	blt-	memmove8Bit_alignedWordsTrailingHalfword
        lwzu	r0, 0(SRC)
	addi	SRC, SRC, 4
        stwu	r0, 0(DEST)
        beqlr-		/* Return to caller. */
	addi	DEST, DEST, 4
        b	memmove8Bit_alignedWordsLoop

memmove8Bit_alignedWordsTrailingHalfword:
	/* LENGTH is -1, -2, or -3 at this point. Add 4 to get real LENGTH */
        /* Copy the trailing half word if necessary: */
        addic.	r0, LENGTH, 2
        blt-    memmove8Bit_alignedTrailingByte
        lhz	r0, 0(SRC)
	addi	SRC, SRC, 2
        sth	r0, 0(DEST)
	addi	DEST, DEST, 2
	beqlr+		/* Return to caller */

memmove8Bit_alignedTrailingByte:
        /* Copy the trailing byte if necessary: */
        lbz	r0, 0(SRC)
        stb	r0, 0(DEST)
        blr	/* Return to caller. */

        /* ===============================================================*/
memmove8Bit_inverse:
        /* If (dest >= src + length), then we can do forward copying: */
        add	r8, SRC, LENGTH
        cmplw	DEST, r8
        bge+	memmove8Bit_copyForward

        /* Do backward copying: */
memmove8Bit_copyBackwardLoop:
        subic.	LENGTH, LENGTH, 1
        bltlr-	/* return to caller */
        lbzx	r0, SRC, LENGTH
        stbx	r0, DEST, LENGTH
        b	memmove8Bit_copyBackwardLoop

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMPPCmemmove8Bit )

ENTRY ( CVMPPCmemmove16Bit )
        /*
          r3 = dest
          r4 = src
          r5 = length
        */

#define DEST    r3
#define SRC     r4
#define LENGTH  r5

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmpwi	LENGTH, 0
	beqlr-
        cmplw	DEST, SRC
        beqlr-		/* Nothing to copy.  Return to caller. */
        bgt-	memmove16Bit_inverse

memmove16Bit_copyForward:
        /* Do forward copying: */

        /* Get at least SRC 32-bit aligned for better performance: */
        andi.	r0, SRC, 0x3	/* Check low 2 bits for word alignment. */
        beq+	memmove16Bit_alignedWordsLoop

        /* Copy the leading halfword: */
        lhz	r0, 0(SRC)
	addi	SRC, SRC, 2
        subic.	LENGTH, LENGTH, 2
        sth	r0, 0(DEST)
	addi	DEST, DEST, 2
        beqlr-		/* Return to caller. */

memmove16Bit_alignedWordsLoop:
        subic.	LENGTH, LENGTH, 4
	blt-	memmove16Bit_alignedWordsTrailingHalfword
        lwzu	r0, 0(SRC)
	addi	SRC, SRC, 4
        stwu	r0, 0(DEST)
        beqlr-		/* Return to caller. */
	addi	DEST, DEST, 4
        b	memmove16Bit_alignedWordsLoop

memmove16Bit_alignedWordsTrailingHalfword:
        /* Copy the trailing half word if necessary: */
        lhz	r0, 0(SRC)
        sth	r0, 0(DEST)
        blr	/* Return to caller. */

        /* ===============================================================*/
memmove16Bit_inverse:
        /* If (dest >= src + length), then we can do forward copying: */
        add	r8, SRC, LENGTH
        cmplw	DEST, r8
        bge+	memmove16Bit_copyForward

        /* Do backward copying: */
memmove16Bit_copyBackwardLoop:
        subic.	LENGTH, LENGTH, 2
        bltlr-	/* return to caller */
        lhzx	r0, SRC, LENGTH
        sthx	r0, DEST, LENGTH
        b	memmove16Bit_copyBackwardLoop

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMPPCmemmove16Bit )

ENTRY ( CVMPPCmemmove32Bit )
        /*
          r3 = dest
          r4 = src
          r5 = length
        */

#define DEST    r3
#define SRC     r4
#define LENGTH  r5

        /* Check to see if the length is 0.  Also check to see if which
           direction we need to do the copying in: */
        cmpwi	LENGTH, 0
	beqlr-
        cmplw	DEST, SRC
        beqlr-		/* Nothing to copy.  Return to caller. */
        bgt-	memmove32Bit_copyBackwardLoop

memmove32Bit_copyForward:
        /* Do forward copying: */
        subic.	LENGTH, LENGTH, 4
        bltlr-	/* Return to caller. */
        lwz	r0, 0(SRC)
        stw	r0, 0(DEST)

memmove32Bit_alignedWordsLoop:
        subic.	LENGTH, LENGTH, 4
        bltlr-	/* Return to caller. */
        lwzu	r0, 4(SRC)
        stwu	r0, 4(DEST)
        b	memmove32Bit_alignedWordsLoop

        /* Do backward copying: */
memmove32Bit_copyBackwardLoop:
        subic.	LENGTH, LENGTH, 4
        bltlr-	/* Return to caller. */
        lwzx	r0, SRC, LENGTH
        stwx	r0, DEST, LENGTH
        b	memmove32Bit_copyBackwardLoop

#undef DEST
#undef SRC
#undef LENGTH

        SET_SIZE( CVMPPCmemmove32Bit )

#endif
