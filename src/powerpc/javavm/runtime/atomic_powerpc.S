/*
 * @(#)atomic_powerpc.S	1.12 06/10/10
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

/*
 * CVMUint32 atomicCmpSwap(CVMUint32 new, CVMUint32 *addr, CVMUint32 old);
 */
ENTRY(atomicCmpSwap)
atomicCmpSwapLoop:
	lwarx	r9,0,r4		/* load prev content in r9 and reserve */
	cmpw	r9,r5
	bne-	CmpSwap_end
	stwcx.	r3,0,r4		/* store conditional new value */
	bne-	atomicCmpSwapLoop /* retry if changed inbetween */
CmpSwap_end:
	mr	r3, r9          /* return old value */
	blr
		
/*
 * CVMUint32 atomicSwap(CVMUint32 new, CVMUint32 *addr);
 */
ENTRY(atomicSwap)
atomicSwapLoop:
	/*sync*/		/* be MP safe */
	lwarx	r9,0,r4		/* load prev content in r9 and reserve */
	stwcx.	r3,0,r4		/* store conditional new value */
	bne-	atomicSwapLoop	/* retry if changed inbetween */
	/*isync*/		/* be MP safe */
	mr	r3, r9          /* return old value */
	blr
		
