/*
 * @(#)ccmmath_cpu.S	1.10 06/10/10
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

#include "javavm/include/porting/endianness.h"
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/porting/jit/jit.h"

#ifndef CVMCPU_HAS_64BIT_REGISTERS

/*
 * In this case, the 64-bit integer type arg/return values are in HI
 * and LO register pair.
 */

#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
#define HIARG1	a0
#define LOARG1	a1
#define HIRES	v0
#define LORES	v1
#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
#define HIARG1	a1
#define LOARG1	a0
#define HIRES	v1
#define LORES	v0
#endif

/*
 * Entry point for doing an unsigned shift right on longs.
 */
ENTRY ( CVMCCMruntimeLUshr )
	# a0, a1 = longValue
	# a2 = shiftCount

	andi	a2, a2, 0x3f		/* So says the VM spec. */
	addiu	a3, a2, -32		
	bgez	a3, _lushrShiftGreaterThan32	/* if (a2 >= 32) then branch. */

	# Perform a logical right shift of less than 32:
	srlv	LORES, LOARG1, a2	/* result.lo = (value1.hi >>> value2) */
	beq	a2, zero, 1f		/* shift amount is 0 */
	negu	a3, a3
	sllv	a3, HIARG1, a3
	or	LORES, LORES, a3	/* result.lo |= (value1.hi << (32-value2)); */
1:
	srlv	HIRES, HIARG1, a2	/* result.hi = value1.hi >>> value2; */
	jr	ra

_lushrShiftGreaterThan32:
	# Perform a logical right shift of greater than or equal to 32:
	srlv	LORES, HIARG1, a3	/* result.lo = value1.hi >> (value2-32); */
	move	HIRES, zero		/* result.hi = 0; */
	jr	ra
	SET_SIZE( CVMCCMruntimeLUshr )


/*
 * Entry point for doing a signed shift right on longs.
 */
ENTRY ( CVMCCMruntimeLShr )
	# a0, a1 = longValue
	# a2 = shiftCount

	andi	a2, a2, 0x3f			/* So says the VM spec. */
	addiu	a3, a2, -32
	bgez	a3, _lshrShiftGreaterThan32	/* if (a2 >= 32) then branch. */

	# Perform a signed right shift of less than 32:
	srlv	LORES, LOARG1, a2	/* result.lo = (value1.hi >>> value2) */
	beq	a2, zero, 1f		/* shift amount is 0 */
	negu	a3, a3
	sllv	a3, HIARG1, a3
	or	LORES, LORES, a3	/* result.lo |= (value1.hi << (32-value2)); */
1:
	srav	HIRES, HIARG1, a2	/* result.hi = value1.hi >> value2; */
	jr	ra

_lshrShiftGreaterThan32:
	# Perform a signed right shift of greater than or equal to 32:
	srav	LORES, HIARG1, a3	/* result.lo = value1.hi >> (value2-32); */
	sra	HIRES, HIARG1, 31	/* result.hi = 0 or -1 */
	jr	ra
	SET_SIZE( CVMCCMruntimeLShr )

/*
 * Entry point for doing an unsigned shift left on longs.
 */
ENTRY ( CVMCCMruntimeLShl )
	# a0, a1 = longValue
	# a2 = shiftCount

	andi	a2, a2, 0x3f			/* So says the VM spec. */
	addiu	a3, a2, -32
	bgez	a3, _lshlShiftGreaterThan32	/* if (a2 >= 32) then branch. */

	# Perform a left shift of less than 32:
	sllv	HIRES, HIARG1, a2	/* result.hi = (value1.hi << value2) | */
	beq	a2, zero, 1f		/* shift amount is 0 */
	negu	a3, a3
	srlv	a3, LOARG1, a3
	or	HIRES, HIRES, a3	/* result.hi |= (value1.lo >> (32-value2)); */
1:
	sllv	LORES, LOARG1, a2	/* result.lo = value1.lo << value2; */
	jr	ra

_lshlShiftGreaterThan32:
	# Perform a left slift of greater than or equal to 32:
	sllv	HIRES, LOARG1, a3	/* result.hi = value1.lo << (value2-32); */
	move	LORES, zero		/* result.lo = 0; */
	jr	ra
	SET_SIZE( CVMCCMruntimeLShl )

#undef HIARG1
#undef LOARG1
#undef HIRES
#undef LORES
	
#else /* !CVMCPU_HAS_64BIT_REGISTERS */

/* 
 * In this case, we use one 64-bit register instead of HI and LO 
 * register pair.
 */

/* Warning: The following code are untested. */

/*
 * Entry point for doing an unsigned shift right on longs.
 */
ENTRY ( CVMCCMruntimeLUshr )
	/*
	 * a0 = longValue
	 * a1 = shiftCount
	 */

	andi	a1, a1, 0x3f		/* So says the VM spec. */
	dsrlv	v0, a0, a1
	jr	ra
	SET_SIZE( CVMCCMruntimeLUshr )


/*
 * Entry point for doing a signed shift right on longs.
 */
ENTRY ( CVMCCMruntimeLShr )
	/*
	 * a0 = longValue
	 * a1 = shiftCount
	 */

	andi	a1, a1, 0x3f			/* So says the VM spec. */
	dsrav	v0, a0, a1
	jr	ra
	SET_SIZE( CVMCCMruntimeLShr )

/*
 * Entry point for doing an unsigned shift left on longs.
 */
ENTRY ( CVMCCMruntimeLShl )
	/*
	 * a0 = longValue
	 * a1 = shiftCount
	 */

	andi	a1, a1, 0x3f			/* So says the VM spec. */
	dsllv	v0, a0, a1
	jr	ra
	SET_SIZE( CVMCCMruntimeLShl )

#endif /* !CVMCPU_HAS_64BIT_REGISTERS */
