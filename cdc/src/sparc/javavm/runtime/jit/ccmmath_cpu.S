/*
 * @(#)ccmmath_cpu.S	1.8 06/10/10
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

#define HIARG1	%o0
#define LOARG1	%o1
#define HIRES	%o0
#define LORES	%o1

/*
 * Entry point for doing an unsigned shift right on longs.
 */
ENTRY ( CVMCCMruntimeLUshr )
	! %o2 = shiftCount

	cmp	%o2, 0
	be	_lushrRet

	and	%o2, 0x3f, %o2			/* So says the VM spec. */
	cmp	%o2, 32
	bge	_lushrShiftGreaterThan32	/* if (%o2 >= 32) then branch. */
	sub	%o2, 32, %g1

	! Perform a logical right shift of less than 32:
	sub	%g0, %g1, %g1
	srl	LOARG1, %o2, LORES	/* result.lo = (value1.hi >>> value2) */
	sll	HIARG1, %g1, %g1
	srl	HIARG1, %o2, HIRES	/* result.hi = value1.hi >>> value2; */
	retl
	or	LORES, %g1, LORES	/* result.lo |= (value1.hi << (32-value2)); */

_lushrShiftGreaterThan32:
	! Perform a logical right shift of greater than or equal to 32:
	srl	HIARG1, %g1, LORES	/* result.lo = value1.hi >> (value2-32); */
	retl
	mov	%g0, HIRES		/* result.hi = 0; */

_lushrRet:
	retl
	nop
	SET_SIZE( CVMCCMruntimeLUshr )


/*
 * Entry point for doing a signed shift right on longs.
 */
ENTRY ( CVMCCMruntimeLShr )
	! %o2 = shiftCount

	cmp	%o2, 0
	be	_lshrRet

	and	%o2, 0x3f, %o2		/* So says the VM spec. */
	cmp	%o2, 32
	bge	_lshrShiftGreaterThan32	/* if (%o2 >= 32) then branch. */
	sub	%o2, 32, %g1

	! Perform a signed right shift of less than 32:
	sub	%g0, %g1, %g1
	srl	LOARG1, %o2, LORES	/* result.lo = (value1.hi >>> value2) */
	sll	HIARG1, %g1, %g1
	sra	HIARG1, %o2, HIRES	/* result.hi = value1.hi >> value2; */
	retl
	or	LORES, %g1, LORES	/* result.lo |= (value1.hi << (32-value2)); */

_lshrShiftGreaterThan32:
	! Perform a signed right shift of greater than or equal to 32:
	sra	HIARG1, %g1, LORES	/* result.lo = value1.hi >> (value2-32); */
	retl
	sra	HIARG1, 31, HIRES	/* result.hi = 0 or -1 */

_lshrRet:
	retl
	nop
	SET_SIZE( CVMCCMruntimeLShr )

/*
 * Entry point for doing an unsigned shift left on longs.
 */
ENTRY ( CVMCCMruntimeLShl )
	! %o2 = shiftCount

	cmp	%o2, 0
	be	_lshlRet

	and	%o2, 0x3f, %o2		/* So says the VM spec. */
	cmp	%o2, 32
	bge	_lshlShiftGreaterThan32	/* if (%o2 >= 32) then branch. */
	sub	%o2, 32, %g1

	! Perform a left shift of less than 32:
	sub	%g0, %g1, %g1
	sll	HIARG1, %o2, HIRES	/* result.hi = (value1.hi << value2) | */
	srl	LOARG1, %g1, %g1
	sll	LOARG1, %o2, LORES	/* result.lo = value1.lo << value2; */
	retl
	or	HIRES, %g1, HIRES	/* result.hi |= (value1.lo >> (32-value2)); */

_lshlShiftGreaterThan32:
	! Perform a left slift of greater than or equal to 32:
	sll	LOARG1, %g1, HIRES	/* result.hi = value1.lo << (value2-32); */
	retl
	mov	%g0, LORES		/* result.lo = 0; */

_lshlRet:
	retl
	nop
	SET_SIZE( CVMCCMruntimeLShl )	
