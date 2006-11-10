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

#include "javavm/include/porting/sync.h"
#include "javavm/include/sync_arch.h"
#include <time.h>

/*
 * Initialize the given CVMMicroLock, allocating additional resources if
 * necessary.
 */
CVMBool CVMmicrolockInit(CVMMicroLock *m)
{
    m->lockWord = CVM_MICROLOCK_UNLOCKED;
    return CVM_TRUE;
}

/*
 * Destroy the given CVMMicroLock, releasing any resources that were
 * previously allocated by CVMmicrolockInit.
 */
void CVMmicrolockDestroy(CVMMicroLock *m)
{
    /* Nothing to do. */
}

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
CVMUint32 slowMlockimplCount = 0;
CVMUint32 fastMlockimplCount = 0;
#endif

/*
 * Acquire a lock for the given microlock.  This call may block.
 * A thread will never attempt to acquire the same microlock
 * more than once, so a counter to keep track of recursive
 * entry is not required.
 */
void CVMmicrolockLockImpl(CVMMicroLock *m)
{
    CVMUint32 oldWord;

#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
    slowMlockimplCount++;
#endif

    oldWord = CVMatomicSwapImpl(CVM_MICROLOCK_LOCKED, &m->lockWord);
    while (oldWord != CVM_MICROLOCK_UNLOCKED) {
        struct timespec tm;
        tm.tv_sec = 0;
        tm.tv_nsec = 5000000; /* 5 milliseconds. */
        nanosleep(&tm, NULL);
        oldWord = CVMatomicSwapImpl(CVM_MICROLOCK_LOCKED, &m->lockWord);
    }
}

/*
 * Release a lock previously acquired by CVMmicrolockLock.
 */
void CVMmicrolockUnlockImpl(CVMMicroLock *m)
{
    m->lockWord = CVM_MICROLOCK_UNLOCKED;
}
