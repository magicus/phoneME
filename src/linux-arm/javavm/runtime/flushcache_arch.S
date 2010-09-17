/*
 * @(#)flushcache_arch.S	1.7 06/10/10
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

/*
 * the cache line size of the I and D cache on the StrongARM, which
 * is the only processor we need this to be accurate for.
 */
#define DCACHELINESIZE	32

	SET_SECTION_EXEC(flushcache)

/*
 * Flush an address range in the dcache and all of the icache.
 */
	ENTRY(CVMflushCache)
	/* First argument:	 beginning address in range */
	/* Second argument:	 end address in range */

	/* Work around linux 2.2 bug on strongARM. The dcache is not always
	 * properly flushed if the address is in a write buffer. We only
	 * worry about this for single word cache flushes.
	 */
	bic a1, a1, #3
	ldr a4, [a1]

	/* Work around another linux 2.2 bug on strongARM. It does not
	 * properly flush if the start address is not 32-byte alinged.
 	 */
	bic a1, a1, #DCACHELINESIZE - 1
	mov a3, #0		/* flush both caches */

#ifdef __ARM_EABI__
	/* Linux Kernels built to support EABI expect the trap number
	 * to be in r7.  The immediate value of the swi instruction
	 * is ignored.
	 */
	stmfd sp!, {r7} 	
        ldr r7, =0x000f0002
	swi 0x0
	ldmfd sp!, {r7}
#else
	swi 0x009f0002
#endif

	mov pc, lr
SET_SIZE(CVMflushCache)
