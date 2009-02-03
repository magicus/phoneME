/*
 * @(#)atomic_arm.S	1.10 06/10/10
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

        SET_SECTION_EXEC(atomic_arm)

#if 0
/* NOTE: CVMatomicSwapImpl() is being implemented using an inlined _asm
         statement in an inlined function.  The following is here only for
         reference purposes.
*/

/*
 * CVMUint32 CVMatomicSwapImpl(CVMUint32 newValue, CVMUint32 *addr);
 */
ENTRY ( CVMatomicSwapImpl )
        /* r0 = new value */
        /* r1 = addr */

        swp     r0, r0, [r1]    /* Swap the contents of r1 with the *addr. */
        BR_REG(lr)              /* Return to the caller. */

SET_SIZE(CVMatomicSwapImpl)
#endif



#ifdef CVM_MP_SAFE
ENTRY(CVMmemoryBarrier)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4 /* data synchronization barrier */
	mov	pc, lr
#endif
