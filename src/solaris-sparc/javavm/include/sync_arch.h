/*
 * @(#)sync_arch.h	1.8 06/10/10
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
 * OS-CPU specific synchronization definitions.
 */

#ifndef _SOLARIS_SYNC_sparc_H
#define _SOLARIS_SYNC_sparc_H

/*
 * Setup the locking type.
 */
#ifdef CVM_ADV_ATOMIC_CMPANDSWAP
#define CVM_FASTLOCK_TYPE CVM_FASTLOCK_ATOMICOPS
#else
/* Use default FASTLOCK type for OS */
#undef  CVM_FASTLOCK_TYPE
#endif

/* Use the swap spinlock for microlocks. */
#define CVM_MICROLOCK_TYPE CVM_MICROLOCK_SWAP_SPINLOCK

#ifndef _ASM

/* NOTE: CVMvolatileStore() could conceptually be implemented using a
   a normal assignment statement.  It is implemented using a call to
   CVMatomicSwap() instead to enforce the 'volatile' status of the
   lockWord.  Calling a function to set it ensures that it is done at
   the location that the programmer intended and not shuffled around
   by the compiler.

   NOTE: Making this inline assembler that just does a store like the
   ARM port does would help performance.
*/
#define CVMvolatileStore(a, n) \
    atomicSwap((n), (a))
/*
 * Setup atomic operations.
 */
#ifdef CVM_ADV_ATOMIC_CMPANDSWAP
#define CVMatomicCompareAndSwap(a, n, o)	\
	atomicCmpSwap((n), (a), (o))
extern
CVMAddr atomicCmpSwap(CVMAddr new_value, volatile CVMAddr *addr,
			CVMAddr old);
#endif

#define CVMatomicSwap(a, n)	\
        atomicSwap((n), (a))
extern
CVMAddr atomicSwap(CVMAddr new_value, volatile CVMAddr *addr);

#endif /* !_ASM */
#endif /* _SOLARIS_SYNC_sparc_H */
