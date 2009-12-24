/*
 * @(#)sync_md.h	1.23 06/10/10
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
 */

/*
 * Machine-dependent synchronization definitions.
 */

#ifndef _SOLARIS_SYNC_MD_H
#define _SOLARIS_SYNC_MD_H

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
#ifdef CVM_ADV_THREAD_BOOST
#include <synch.h>
#ifndef SOLARIS_BOOST_MUTEX
#include <semaphore.h>
#endif
#endif

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

#ifdef CVM_ADV_THREAD_BOOST

typedef struct {
    CVMThreadID *waiters;
    CVMThreadID **last_p;
} CVMDQueue;

typedef struct {
    int nboosters;
    mutex_t lock;
} SOLARISBoostLock;

struct CVMThreadBoostQueue {
    int nboosters;
    CVMDQueue boosters;
    mutex_t lock;
    CVMBool boosting;
#ifdef SOLARIS_BOOST_MUTEX
    CVMThreadID *target;
    SOLARISBoostLock *thread_lock;
    SOLARISBoostLock *next_thread_lock;
#else
    sem_t sem;
#endif
};

struct CVMThreadBoostRecord {
    CVMThreadBoostQueue *currentRecord;
};

extern CVMBool SOLARISthreadBoostInit(CVMThreadID *self);
extern void SOLARISthreadBoostDestroy(CVMThreadID *self);

#endif

/*
 * When a Solaris thread is waiting on a pthread condition variable,
 * and the thread is sent a signal, the signal handler is not called
 * until the associated mutex is locked!  So we can't associate the
 * condition variable mutex directly with the Java object monitor,
 * otherwise the signal handler will not execute until the Java
 * monitor is unlocked!
 */
#define SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER

struct CVMMutex {
    POSIXMutex pmtx;
};

struct CVMCondVar {
#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
    POSIXMutex m;
#endif
    POSIXCondVar pcv;
};

#define CVMmutexInit(m)		POSIXmutexInit(&(m)->pmtx)
#define CVMmutexDestroy(m)	POSIXmutexDestroy(&(m)->pmtx)
#define CVMmutexTryLock(m)	POSIXmutexTryLock(&(m)->pmtx)
#define CVMmutexLock(m)		POSIXmutexLock(&(m)->pmtx)
#define CVMmutexUnlock(m)	POSIXmutexUnlock(&(m)->pmtx)

#ifndef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER

#define CVMcondvarInit(c, m)	POSIXcondvarInit(&(c)->pcv, &(m)->pmtx)
#define CVMcondvarDestroy(c)	POSIXcondvarDestroy(&(c)->pcv)
#define CVMcondvarNotify(c)	POSIXcondvarNotify(&(c)->pcv)
#define CVMcondvarNotifyAll(c)	POSIXcondvarNotifyAll(&(c)->pcv)

#endif

CVMBool solarisSyncInit(void);
void solarisSyncInterruptWait(CVMThreadID *thread);

#endif /* !_ASM */
#endif /* _SOLARIS_SYNC_MD_H */
