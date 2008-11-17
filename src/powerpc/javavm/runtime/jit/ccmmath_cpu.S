/*
 * @(#)ccmmath_cpu.S	1.11 06/10/10
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

#include "javavm/include/asmmacros_cpu.h"

	.section	".text",""

/*************************************
 * Msth helpers start here!
 *************************************/
 
/*
 * Entry point for doing an unsigned shift right on longs.
 * NOTE: The result is in r3 and r4.
 */
ENTRY ( CVMCCMruntimeLUshr )
	# r3 = long1high
	# r4 = long1low
	# r5 = shiftCount

	andi.	r5, r5, 0x3f			/* So says the VM spec. */
	cmpwi	r5, 32
	bge-	.lushrShiftGreaterThan32	/* if (r5 > 32) then branch. */

	# Perform a logical right shift of less than 32:
	subfic	r6, r5, 32	/* r6 = 32 - r5; */
	srw	r4, r4, r5	/* result.lo = (value1.hi >>> value2) */
	slw	r6, r3, r6
	srw	r3, r3, r5	/* result.hi = value1.hi >>> value2; */
	or	r4, r4, r6	/* result.lo |= (value1.hi << (32-value2)); */
	blr

.lushrShiftGreaterThan32:
	# Perform a logical right shift of greater than or equal to 32:
	subi	r5, r5, 32
	srw	r4, r3, r5	/* result.lo = value1.hi >> (value2-32); */
	li	r3, 0		/* result.hi = 0; */
	blr

/*
 * Entry point for doing a signed shift right on longs.
 * NOTE: The result is in r3 and r4.
 */
ENTRY ( CVMCCMruntimeLShr )
	# r3 = long1high
	# r4 = long1low
	# r5 = shiftCount

	andi.	r5, r5, 0x3f			/* So says the VM spec. */
	cmpwi	r5, 32
	bge-	.lshrShiftGreaterThan32		/* if (r5 > 32) then branch. */

	# Perform a signed right shift of less than 32:
	subfic	r6, r5, 32	/* r6 = 32 - r5; */
	srw	r4, r4, r5	/* result.lo = (value1.hi >>> value2) */
	slw	r6, r3, r6
	sraw	r3, r3, r5	/* result.hi = value1.hi >> value2; */
	or	r4, r4, r6	/* result.lo |= (value1.hi << (32-value2)); */
	blr

.lshrShiftGreaterThan32:
	# Perform a signed right shift of greater than or equal to 32:
	subi	r5, r5, 32
	sraw	r4, r3, r5	/* result.lo = value1.hi >> (value2-32); */
	srawi	r3, r3, 31	/* result.hi = 0 or -1 */
	blr

/*
 * Entry point for doing an unsigned shift left on longs.
 * NOTE: The result is in r3 and r4.
 */
ENTRY ( CVMCCMruntimeLShl )
	# r3 = long1high
	# r4 = long1low
	# r5 = shiftCount

	andi.	r5, r5, 0x3f			/* So says the VM spec. */
	cmpwi	r5, 32
	bge-	.lshlShiftGreaterThan32		/* if (r5 > 32) then branch. */

	# Perform a left shift of less than 32:
	subfic	r6, r5, 32	/* r6 = 32 - r5; */
	slw	r3, r3, r5	/* result.hi = (value1.hi << value2) | */
	srw	r6, r4, r6
	slw	r4, r4, r5	/* result.lo = value1.lo << value2; */
	or	r3, r3, r6	/* result.hi |= (value1.lo >> (32-value2)); */
	blr

.lshlShiftGreaterThan32:
	# Perform a left slift of greater than or equal to 32:
	subi	r5, r5, 32
	slw	r3, r4, r5	/* result.hi = value1.lo << (value2-32); */
	li	r4, 0		/* result.lo = 0; */
	blr
