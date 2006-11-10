/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * CPU-specific synchronization definitions.
 */

#ifndef _LINUX_SYNC_sarm_H
#define _LINUX_SYNC_sarm_H

/* Use default */

#undef CVM_FASTLOCK_TYPE

#define CVM_MICROLOCK_UNLOCKED  0x00
#define CVM_MICROLOCK_LOCKED    0xff

struct CVMMicroLock
{
    volatile CVMAddr lockWord;
};

extern void CVMmicrolockLockImpl(CVMMicroLock *m);
extern void CVMmicrolockUnlockImpl(CVMMicroLock *m);

#define CVMmicrolockLock(m) {                                          \
    CVMAddr                                                          \
    oldWord = CVMatomicSwapImpl(CVM_MICROLOCK_LOCKED, &(m)->lockWord); \
    if (oldWord != CVM_MICROLOCK_UNLOCKED) {                           \
        CVMmicrolockLockImpl(m);                                       \
    }                                                                  \
}

/* NOTE: CVMmicrolockUnlock() could conceptually be implemented using a
   a normal assignment statement.  It is implemented using a call to
   CVMatomicSwapImpl() instead to enforce the 'volatile' status of the
   lockWord.  Calling a function to set it ensures that it is done at
   the location that the programmer intended and not shuffled around
   by the compiler. */
#define CVMmicrolockUnlock(m) \
    CVMstoreImpl(CVM_MICROLOCK_UNLOCKED, &(m)->lockWord)

static inline void
CVMstoreImpl(CVMAddr new_value, volatile CVMAddr *addr)
{
    CVMAddr scratch;
#ifndef __RVCT__
    asm volatile (
        "str %1, [%2];"
        "ldr %0, [%2]"
        : "=r" (scratch)
        : "r" (new_value), "r" (addr)
        /* clobber? */);
#else
    __asm
    {    
        str new_value, [addr];
        ldr scratch, [addr]
    }
#endif
}

#define CVMatomicSwap(a, n) \
    CVMatomicSwapImpl((n), (a))

/* Purpose: Performs an atomic swap operation. */
static inline CVMAddr
CVMatomicSwapImpl(CVMAddr new_value, volatile CVMAddr *addr)
{
    CVMAddr old_value;
    CVMAddr scratch;
#ifndef __RVCT__
    asm volatile (
        "swp %0, %2, [%3];"
        "ldr %1, [%3]"
        : "=&r" (old_value), "=&r" (scratch)
        : "r" (new_value), "r" (addr)
        /* clobber? */);
#else
    __asm 
    {
        swp old_value, new_value, [addr];
        ldr scratch, [addr]
    }
#endif
    return old_value;
}

#endif /* _LINUX_SYNC_sarm_H */
