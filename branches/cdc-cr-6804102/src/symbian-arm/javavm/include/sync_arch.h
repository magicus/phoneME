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

#ifndef _SYMBIAN_SYNC_SARM_H
#define _SYMBIAN_SYNC_SARM_H

/* Use Symbian default fastlock type */
#undef CVM_FASTLOCK_TYPE

/* Use the swap spinlock for microlocks. */
/*#define CVM_MICROLOCK_TYPE CVM_MICROLOCK_SWAP_SPINLOCK */

#ifndef _ASM

/* Store new_value into addr in a volatile way, with membar if needed. */

#define CVMvolatileStore(a, n) \
    CVMvolatileStoreImpl((n), (a))

static inline void
CVMvolatileStoreImpl(CVMAddr new_value, volatile CVMAddr *addr)
{
    CVMAddr scratch;
#ifndef __RVCT__
    asm volatile (
#ifdef CVM_MP_SAFE
	"mov %0, #0;"
        "mcr p15, 0, %0, c7, c10, 5;"
#endif
        "str %1, [%2];"
        "ldr %0, [%2]"
        : "=&r" (scratch)
        : "r" (new_value), "r" (addr)
        /* clobber? */);
#else
    __asm
    {    
#ifdef CVM_MP_SAFE
#error CVM_MP_SAFE currently is not supported for RVCT.
#endif
        str new_value, [addr];
        ldr scratch, [addr]
    }
#endif
}

/* Purpose: Performs an atomic swap operation. */

#define CVMatomicSwap(a, n) \
    CVMatomicSwapImpl((n), (a))

static inline CVMAddr
CVMatomicSwapImpl(CVMAddr new_value, volatile CVMAddr *addr)
{
    CVMAddr old_value;
    CVMAddr scratch;
#ifndef __RVCT__
    asm volatile (
#ifndef CVM_MP_SAFE
        "swp %0, %2, [%3];"
        "ldr %1, [%3]"
#else
	"1:;"
        "ldrex %0, [%3];"
	"strex %1, %2, [%3];"
	"cmp %1, #0;"
	"bne 1b;"
        "mcr p15, 0, %1, c7, c10, 5"
#endif
        : "=&r" (old_value), "=&r" (scratch)
        : "r" (new_value), "r" (addr)
        /* clobber? */);
#else
    __asm 
    {
#ifdef CVM_MP_SAFE
#error CVM_MP_SAFE currently is not supported for RVCT.
#endif
        swp old_value, new_value, [addr];
        ldr scratch, [addr]
    }
#endif
    return old_value;
}

#endif /* !_ASM */
#endif /* _SYMBIAN_SYNC_SARM_H */
