/*
 * @(#)flushcache_cpu.S	1.12 06/10/10
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

	.file           "flushcache_cpu.S"

/*
 * Flush an address range in the dcache and the icache.
 *
 * NOTE: we're assuming a 32 byte cache line size. This is probably
 * too small on later generation PowerPC processors, which means this
 * takes a bit longer than it should. Hopefully 32 is never too big, in
 * which case we aren't flushing the cache properly.
 */
ENTRY(CVMJITflushCache)
	# First argument:	 beginning address in range
	# Second argument:	 non-inclusive end of address in range

	# round end of address range to start of cacheline after the last
	# cache line we need to flush.
	addi	r4, r4, 31
	clrrwi	r4, r4, 5       /* clear right-most 5 bits */
0:
	dcbst	0,r3		/* force store of dcache line */
	icbi	0,r3		/* force flush of icache line */
	
	# on to the next cache line
	addi	r3,r3,32
	cmpw	r3,r4
	blt	0b
	
	sync
	blr
	
SET_SIZE(CVMJITflushCache)
