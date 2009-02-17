/*
 * @(#)atomic_mips.S	1.9 06/10/10
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

	.text

/*
 * CVMUint32 atomicSwap(CVMUint32 new, CVMUint32 *addr);
 */
ENTRY(atomicSwap)
1:
	move	t2,a0		/* we don't want to trash a0 */
	ll	v0,(a1)		/* load prev content in v0  */
	sc	t2,(a1)		/* store conditional new value */
	beq	t2,0,1b		/* retry if changed inbetween */
	jr	ra
SET_SIZE(atomicSwap)

/*
 * CVMUint32 atomicCmpSwap(CVMUint32 new, CVMUint32 *addr, CVMUint32 old);
 */
ENTRY(atomicCmpSwap)
1:
	move	t2,a0		/* we don't want to trash a0 */
	ll	v0,(a1)		/* load prev content in v0  */
	bne	v0,a2,2f
	sc	t2,(a1)		/* store conditional new value */
	beq	t2,0,1b		/* retry if changed inbetween */
2:
	jr	ra
SET_SIZE(atomicCmpSwap)
