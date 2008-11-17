/*
 * @(#)sync_md.h	1.20 06/10/10
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
 * Machine-dependent synchronization definitions.
 */

#ifndef _LINUX_SYNC_MD_H
#define _LINUX_SYNC_MD_H

#include "javavm/include/porting/vm-defs.h"
#include "javavm/include/sync_arch.h"

#ifndef CVM_FASTLOCK_TYPE
/* Use microlocks for fastlocks by default */
#define CVM_FASTLOCK_TYPE CVM_FASTLOCK_MICROLOCK
#endif

#ifndef CVM_MICROLOCK_TYPE
#define CVM_MICROLOCK_TYPE CVM_MICROLOCK_DEFAULT
#endif

#ifndef _ASM

#include "portlibs/posix/sync.h"

/*
 * CVM_ADV_SPINLOCK support, if needed. Note that CVMvolatileStore
 * must be implemented in sync_arch.h because it is CPU dependent.
 */
#ifdef CVM_ADV_SPINLOCK

#include <time.h>   /* for nanosleep() */

/* Yield to another thread when microlock is contended */
#define CVMspinlockYield() \
    CVMspinlockYieldImpl()

static inline void CVMspinlockYieldImpl()
{
    struct timespec tm;
    tm.tv_sec = 0;
    tm.tv_nsec = 5000000; /* 5 milliseconds. */
    nanosleep(&tm, NULL);
}

#endif /* CVM_ADV_SPINLOCK */

/*
 * In LinuxThreads pthreads, it is not safe to use a signal handler
 * and longjmp to break out of a condvar wait.  The mutex will not
 * be reacquired, and the pthreads data structures will not be
 * cleaned up, leaving the thread on the wait queue.  So we use
 * a "semaphore" per thread and explicitly wakeup the thread we want
 * to, just like HotSpot and win32 JDK.
 */

#define CVMmutexInit(m)		POSIXmutexInit(&(m)->pmtx)
#define CVMmutexDestroy(m)	POSIXmutexDestroy(&(m)->pmtx)
#define CVMmutexTryLock(m)	POSIXmutexTryLock(&(m)->pmtx)
#define CVMmutexLock(m)		POSIXmutexLock(&(m)->pmtx)
#define CVMmutexUnlock(m)	POSIXmutexUnlock(&(m)->pmtx)

CVMBool linuxSyncInit(void);
void linuxSyncInterruptWait(CVMThreadID *thread);
void linuxSyncSuspend(CVMThreadID *thread);
void linuxSyncResume(CVMThreadID *thread);

struct CVMMutex {
    POSIXMutex pmtx;
};

struct CVMCondVar {
    POSIXCondVar pcv;
    CVMThreadID *waiters;
    CVMThreadID **last_p;
};

#endif /* !_ASM */
#endif /* _LINUX_SYNC_MD_H */
