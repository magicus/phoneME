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
 * CPU-specific synchronization definitions.
 */

#ifndef _SOLARIS_SYNC_i686_H
#define _SOLARIS_SYNC_i686_H

#if defined(__i386__)
#else
#error Need i386 compiler support
#endif

/* Use atomic operation for fast locking on x86 */
#define CVM_FASTLOCK_TYPE CVM_FASTLOCK_ATOMICOPS

#ifndef _ASM

#define CVMatomicCompareAndSwap(a, n, o)	\
	atomicCmpSwap((n), (a), (o))

/* Purpose: Performs an atomic compare and swap operation. */
static inline CVMAddr atomicCmpSwap(CVMAddr new_value, volatile CVMAddr *addr,
				    CVMAddr old_value)
{
    int x;
    asm volatile (
        "lock cmpxchgl %3, %1"
        : "=a" (x)
        : "m" (*addr), "a" (old_value), "q" (new_value)
        /* clobber? */);
    return (CVMAddr)x;
}

#define CVMatomicSwap(a, n)	\
        atomicSwap((n), (a))

/* Purpose: Performs an atomic swap operation. */
static inline CVMAddr atomicSwap(CVMAddr new_value, volatile CVMAddr *addr)
{
    int x;
    asm volatile (
        "xchgl %1, %2"
        : "=a" (x)
        : "m" (*addr), "a" (new_value)
        /* clobber? */);
    return (CVMAddr)x;
}

#include "javavm/include/sync_cpu.h"

#endif /* !_ASM */
#endif /* _SOLARIS_SYNC_i686_H */
