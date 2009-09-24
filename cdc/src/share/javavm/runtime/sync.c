/*
 * @(#)sync.c	1.69 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/sync.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/include/common_exceptions.h"

#include "javavm/include/clib.h"

/*
 * Spinlock microlock support.
 */

#if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
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
/* NOTE: fastMlockimplCount must be maintained by any assembler code
 *       that grabs the microlock successfully. It is not supported
 *       when CVMmicrolockLock() is called from C, but these calls are
 *       relatively rare when using the JIT. It also is currently not
 *       supported when synchronizing for Simple Sync methods, so
 *       Simple Sync method support should be disabled when gathering
 *       contention stats.
 */
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

    oldWord = CVMatomicSwap(&m->lockWord, CVM_MICROLOCK_LOCKED);
    while (oldWord != CVM_MICROLOCK_UNLOCKED) {
	CVMspinlockYield();
        oldWord = CVMatomicSwap(&m->lockWord, CVM_MICROLOCK_LOCKED);
    }
}

#endif /* CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK */

/*
 * Reentrant mutexes
 */

CVMBool
CVMreentrantMutexInit(CVMReentrantMutex * rm, CVMExecEnv *owner,
    CVMUint32 count)
{
    rm->count = count;
    rm->owner = owner;
    if (CVMmutexInit(&rm->mutex)) {
	if (count > 0) {
	    CVMBool success = CVMmutexTryLock(&rm->mutex);
	    CVMassert(success); (void) success;
	}
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

void
CVMreentrantMutexDestroy(CVMReentrantMutex * rm)
{
    CVMmutexDestroy(&rm->mutex);
}

CVMBool
CVMreentrantMutexTryLock(CVMExecEnv * ee, CVMReentrantMutex * rm)
{
    if (rm->owner == ee) {
	++rm->count;
    } else {
	if (CVMmutexTryLock(&rm->mutex)) {
	    rm->owner = ee;
	    rm->count = 1;
	} else {
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

/* Purpose: Do the general process of locking a ReentrantMutex. */
/* NOTE: This code is factored out into a macro to maintain as much commonality
         between CVMreentrantMutexLock(), CVMprofiledMonitorEnterSafe(),
         and CVMprofiledMonitorEnterUnsafe() as possible. */
#define CVMreentrantMutexDoLock(rm_, ee_, lockAction_) { \
    if ((rm_)->owner == (ee_)) {                         \
        ++(rm_)->count;                                  \
    } else {                                             \
        lockAction_;                                     \
        (rm_)->owner = (ee_);                            \
        (rm_)->count = 1;                                \
    }                                                    \
}

/* Purpose: Locks the specified CVMReentrantMutex. */
void
CVMreentrantMutexLock(CVMExecEnv *ee, CVMReentrantMutex *rm)
{
    CVMassert(CVMD_isgcSafe(ee));
    CVMreentrantMutexDoLock(rm, ee, {
        CVMmutexLock(&rm->mutex);
    });
}

/* Purpose: Do the general process of unlocking a ReentrantMutex. */
/* NOTE: This code is factored out into a macro to maintain as much commonality
         between CVMreentrantMutexUnLock(), and CVMprofiledMonitorExit() as
         possible. */
#define CVMreentrantMutexDoUnlock(rm_, ee_, unlockAction_) { \
    CVMassert((rm_)->count > 0);                             \
    CVMassert((rm_)->owner == (ee_));                        \
    if (--(rm_)->count == 0) {                               \
        (rm_)->owner = NULL;                                 \
        unlockAction_;                                       \
    }                                                        \
}

/* Purpose: Unlocks the specified CVMReentrantMutex. */
void
CVMreentrantMutexUnlock(CVMExecEnv *ee, CVMReentrantMutex *rm)
{
    CVMreentrantMutexDoUnlock(rm, ee, {
        CVMmutexUnlock(&rm->mutex);
    });
}

CVMBool
CVMreentrantMutexIAmOwner(CVMExecEnv * ee, CVMReentrantMutex * rm)
{
    CVMassert(rm->owner != ee || rm->count > 0);
    return (rm->owner == ee);
}

/* Purpose: Do the general process of waiting on a condvar protected by a
            ReentrantMutex. */
/* NOTE: This code is factored out into a macro to maintain as much commonality
         between CVMreentrantMutexWait(), and CVMprofiledMonitorWait() as
         possible. */
#define CVMreentrantMutexDoWait(rm_, ee_, waitAction_) { \
    CVMUint32 count;                                     \
    CVMassert(rm->owner == ee);                          \
    count = (rm_)->count;                                \
    (rm_)->owner = NULL;                                 \
    (rm_)->count = 0;                                    \
    waitAction_;                                         \
    (rm_)->owner = (ee_);                                \
    (rm_)->count = count;                                \
}

/*
 * CVM_FALSE is returned to indicate that the wait was interrupted,
 * otherwise CVM_TRUE is returned.
 */
CVMBool
CVMreentrantMutexWait(CVMExecEnv *ee, CVMReentrantMutex *rm,
    CVMCondVar *c, CVMInt64 millis)
{
    CVMBool result;
    CVMassert(CVMD_isgcSafe(ee));
    CVMreentrantMutexDoWait(rm, ee, {
        result = CVMcondvarWait(c, &rm->mutex, millis);
    });
    return result;
}

/*
 * Raw Monitors
 */

CVMBool
CVMsysMonitorInit(CVMSysMonitor *m, CVMExecEnv *owner, CVMUint32 count)
{
    if (CVMreentrantMutexInit(&m->rmutex, owner, count)) {
	if (CVMcondvarInit(&m->condvar, &m->rmutex.mutex)) {
	    return CVM_TRUE;
	}
	CVMreentrantMutexDestroy(&m->rmutex);
    }
    return CVM_FALSE;
}

void
CVMsysMonitorDestroy(CVMSysMonitor *m)
{
    CVMcondvarDestroy(&m->condvar);
    CVMreentrantMutexDestroy(&m->rmutex);
}

/* Purpose: Notifies the next thread waiting on this monitor. */
/* Returns: CVM_FALSE if the current thread is not the owner of monitor. */
/* NOTE: The ee arg is expected to be same as CVMgetEE() i.e the ee for the
         current thread. */
CVMBool
CVMsysMonitorNotify(CVMExecEnv *ee, CVMSysMonitor* m)
{
    if (CVMsysMonitorOwner(m) != ee) {
        return CVM_FALSE;
    }
    CVMcondvarNotify(&m->condvar);
    return CVM_TRUE;
}

/* Purpose: Notifies all threads waiting on this monitor. */
/* Returns: CVM_FALSE if the current thread is not the owner of monitor. */
/* NOTE: The ee arg is expected to be same as CVMgetEE() i.e the ee for the
         current thread. */
CVMBool
CVMsysMonitorNotifyAll(CVMExecEnv *ee, CVMSysMonitor *m)
{
    if (CVMsysMonitorOwner(m) != ee) {
        return CVM_FALSE;
    }
    CVMcondvarNotifyAll(&m->condvar);
    return CVM_TRUE;
}

/* Purpose: Do the general process of waiting on a monitor. */
/* NOTE: This code is factored out into a macro to maintain as much commonality
         between CVMsysMonitorWait(), and CVMprofiledMonitorWait() as
         possible. */
#define CVMsysMonitorDoWait(mon_, ee_, result_, waitAction_) { \
    CVMBool waitActionResult;                                  \
    if (CVMsysMonitorOwner(mon_) != (ee_)) {                   \
        (result_) =  CVM_WAIT_NOT_OWNER;                       \
    } else {                                                   \
        waitAction_;                                           \
        if (waitActionResult != CVM_FALSE) {                   \
            (result_) = CVM_WAIT_OK;                           \
        } else {                                               \
            (result_) = CVM_WAIT_INTERRUPTED;                  \
        }                                                      \
    }                                                          \
}

/* Purpose: Wait for the specified time period (in milliseconds).  If the
            period is 0, then wait forever. */
/* Returns: CVM_WAIT_NOT_OWNER if current_ee is not the owner of the monitor,
            CVM_WAIT_OK if the thread was woken up by a timeout, and
            CVM_WAIT_INTERRUPTED is the thread was woken up by a notify. */
/* NOTE:    Thread must be in a GC safe state before calling this. */
CVMWaitStatus
CVMsysMonitorWait(CVMExecEnv *ee, CVMSysMonitor *mon, CVMInt64 millis)
{
    CVMWaitStatus result;
    CVMassert(CVMD_isgcSafe(ee));
    CVMsysMonitorDoWait(mon, ee, result, {
        waitActionResult =
            CVMreentrantMutexWait(ee, &mon->rmutex, &mon->condvar, millis);
    });
    return result;
}

/*
 * VM mutexes
 */

CVMBool
CVMsysMutexInit(CVMSysMutex * m, const char * name, CVMUint8 rank)
{
    m->name = name;
    m->rank = rank;
    if (CVMreentrantMutexInit(&m->rmutex, NULL, 0)) {
#ifdef CVM_DEBUG
	m->nextOwned = NULL;
#endif
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

void
CVMsysMutexDestroy(CVMSysMutex * m)
{
    CVMreentrantMutexDestroy(&m->rmutex);
#ifdef CVM_DEBUG
    CVMassert(m->nextOwned == NULL && m->rmutex.owner == NULL);
#endif
}

CVMBool
CVMsysMutexTryLock(CVMExecEnv * ee, CVMSysMutex * m)
{
    CVMBool success;
    CVMtraceSysMutex(("CVMsysMutexTryLock(%s)\n", m->name));
#ifdef CVM_DEBUG
    CVMassert(ee->microLock == 0); /* no locking while holding microlock */
    CVMassert(ee->sysLocks == NULL || m->rmutex.owner == ee ||
	m->rank > ee->sysLocks->rank);
#endif
    success = CVMreentrantMutexTryLock(ee, &m->rmutex);
#ifdef CVM_DEBUG
    if (success && CVMsysMutexCount(m) == 1) {
	m->nextOwned = ee->sysLocks;
	ee->sysLocks = m;
    }
#endif
    return success;
}

void
CVMsysMutexLock(CVMExecEnv * ee, CVMSysMutex * m)
{
    CVMtraceSysMutex(("CVMsysMutexLock(\"%s\")\n", m->name));
#ifdef CVM_DEBUG
    /* no locking while holding a microlock */
    CVMassert(ee->microLock == 0 || m->rmutex.owner == ee);
    CVMassert(ee->sysLocks == NULL || m->rmutex.owner == ee ||
	m->rank > ee->sysLocks->rank);
#endif
    CVMreentrantMutexLock(ee, &m->rmutex);
#ifdef CVM_DEBUG
    if (CVMsysMutexCount(m) == 1) {
	m->nextOwned = ee->sysLocks;
	ee->sysLocks = m;
    }
#endif
}

void
CVMsysMutexUnlock(CVMExecEnv * ee, CVMSysMutex * m)
{
    CVMtraceSysMutex(("CVMsysMutexUnlock(\"%s\")\n", m->name));
#ifdef CVM_DEBUG
    CVMassert(CVMsysMutexCount(m) > 1 || ee->sysLocks == m);
    if (CVMsysMutexCount(m) == 1) {
	ee->sysLocks = m->nextOwned;
	m->nextOwned = NULL;
    }
#endif
    CVMreentrantMutexUnlock(ee, &m->rmutex);
}

/*
 * CVM_FALSE is returned to indicate that the wait was interrupted,
 * otherwise CVM_TRUE is returned.
 */
CVMBool
CVMsysMutexWait(CVMExecEnv * ee, CVMSysMutex *m,
    CVMCondVar * c, CVMInt64 millis)
{
    CVMBool ret;
#ifdef CVM_DEBUG
    CVMassert(ee->sysLocks == m);
    ee->sysLocks = ee->sysLocks->nextOwned;
#endif
    ret = CVMreentrantMutexWait(ee, &m->rmutex, c, millis);
#ifdef CVM_DEBUG
    m->nextOwned = ee->sysLocks;
    ee->sysLocks = m;
#endif
    return ret;
}

#ifdef CVM_DEBUG
void
CVMsysMicroLock(CVMExecEnv *ee, CVMSysMicroLock l)
{
    CVMassert(++ee->microLock == 1);
    CVMsysMicroLockImpl(l);
}

void
CVMsysMicroUnlock(CVMExecEnv *ee, CVMSysMicroLock l)
{
    CVMassert(--ee->microLock == 0);
    CVMsysMicroUnlockImpl(l);
}

#endif /* CVM_DEBUG */

/*
 * Acquire all the microlocks. Don't do anything for SCHEDLOCK because
 * the fact that this thread is running implies that no other thread
 * has acquired any of the microlocks.
 */
void
CVMsysMicroLockAll(CVMExecEnv *ee)
{
#if CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SCHEDLOCK
    int i;
    for (i = 0; i < CVM_NUM_SYS_MICROLOCKS; i++) {
	CVMsysMicroLockImpl(i);
    }
#endif
}

void
CVMsysMicroUnlockAll(CVMExecEnv *ee)
{
#if CVM_MICROLOCK_TYPE != CVM_MICROLOCK_SCHEDLOCK
    int i;
    for (i = CVM_NUM_SYS_MICROLOCKS - 1; i >= 0; i--) {
	CVMsysMicroUnlockImpl(i);
    }
#endif
}

/*===========================================================================*/

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Constructor. */
CVMBool CVMprofiledMonitorInit(CVMProfiledMonitor *self, CVMExecEnv *owner,
                            CVMUint32 count, CVMUint8 lockType)
{
    if (CVMsysMonitorInit(&self->_super, owner, count)) {
	CVMprofiledMonitorSetType(self, lockType);
	self->contentionCount = 0;
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

/* gcc 3.2 complains about ifdefs inside a macro */

static void postContendedEnter(CVMProfiledMonitor *self, CVMExecEnv *ee) {
#ifdef CVM_JVMPI
    if (CVMjvmpiEventMonitorContendedEnterIsEnabled()) {
	CVMjvmpiPostMonitorContendedEnterEvent(ee, self);
    }
#endif
    ee->threadState = CVM_THREAD_BLOCKED_MONITOR_ENTER;
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostMonitorContendedEnter()) {
	CVMjvmtiPostMonitorContendedEnterEvent(ee, self);
    }
#endif
}
static void postContendedEntered(CVMProfiledMonitor *self, CVMExecEnv *ee) {
#ifdef CVM_JVMPI
    if (CVMjvmpiEventMonitorContendedEnteredIsEnabled()) {
	CVMjvmpiPostMonitorContendedEnteredEvent(ee, self);
    }
#endif
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostMonitorContendedEntered()) {
	CVMjvmtiPostMonitorContendedEnteredEvent(ee, self);
    }
#endif
}

/* Purpose: Enters the specified monitor. */
void CVMprofiledMonitorEnterSafe(CVMProfiledMonitor *self, CVMExecEnv *ee,
				 CVMBool doPost)
{
    CVMReentrantMutex *rm = &self->_super.rmutex;
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(self->type != CVM_LOCKTYPE_UNKNOWN);
    CVMreentrantMutexDoLock(rm, ee, {
        /* Locking action: */
        CVMBool hasContention = CVM_FALSE;
        CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
        if (!CVMmutexTryLock(&rm->mutex)) {
            self->contentionCount++;
            hasContention = CVM_TRUE;
        }
        CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);

        if (hasContention) {
            ee->blockingLockEntryMonitor = self;
	    if (doPost) {
		postContendedEnter(self, ee);
	    }
            CVMmutexLock(&rm->mutex);

            ee->blockingLockEntryMonitor = NULL;
	    if (doPost) {
		postContendedEntered(self, ee);
	    }
        }
    });
}

#ifdef CVM_THREAD_BOOST
/* This operation is done before thread boosting */
void
CVMprofiledMonitorPreContendedEnterUnsafe(CVMProfiledMonitor *self,
    CVMExecEnv *ee)
{
    CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
    self->contentionCount++;
    CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);

    CVMD_gcSafeExec((ee), {
	ee->blockingLockEntryMonitor = self;
	postContendedEnter(self, ee);
    });
}


/* This operation is done after thread boosting */
void
CVMprofiledMonitorContendedEnterUnsafe(CVMProfiledMonitor *self,
    CVMExecEnv *ee)
{
    CVMReentrantMutex *rm = &self->_super.rmutex;
    CVMassert(CVMD_isgcUnsafe(ee));
    CVMassert(self->type != CVM_LOCKTYPE_UNKNOWN);
    CVMreentrantMutexDoLock(rm, ee, {
        /* Locking action: */
	CVMD_gcSafeExec((ee), {

	    CVMmutexLock(&(rm)->mutex);

	    ee->blockingLockEntryMonitor = NULL;
	    postContendedEntered(self, ee);
	});
    });
}
#endif

/* Purpose: Enters the specified monitor.  Assumes that the current thread is
            in a GC safe state. */
void CVMprofiledMonitorEnterUnsafe(CVMProfiledMonitor *self, CVMExecEnv *ee,
				   CVMBool doPost)
{
    CVMReentrantMutex *rm = &self->_super.rmutex;
    CVMassert(CVMD_isgcUnsafe(ee));
    CVMassert(self->type != CVM_LOCKTYPE_UNKNOWN);
    CVMreentrantMutexDoLock(rm, ee, {
        /* Locking action: */
        CVMBool hasContention = CVM_FALSE;
        CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
        if (!CVMmutexTryLock(&rm->mutex)) {
            self->contentionCount++;
            hasContention = CVM_TRUE;
        }
        CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);

        if (hasContention) {
            CVMD_gcSafeExec((ee), {
                ee->blockingLockEntryMonitor = self;
		if (doPost) {
		    postContendedEnter(self, ee);
		}
                CVMmutexLock(&(rm)->mutex);

                ee->blockingLockEntryMonitor = NULL;
		if (doPost) {
		    postContendedEntered(self, ee);
		}
            });
        }
    });
}

static void profileMonitorExit(CVMProfiledMonitor *self,
                                 CVMExecEnv *ee, CVMBool hasContention) {
#ifdef CVM_JVMTI
        (void)hasContention;
#endif
	ee->threadState &= ~CVM_THREAD_BLOCKED_MONITOR_ENTER;
#ifdef CVM_JVMPI
        if (hasContention &&
            CVMjvmpiEventMonitorContendedExitIsEnabled()) {
            CVMjvmpiPostMonitorContendedExitEvent(ee, self);
        }
#endif
}

/* Purpose: Exits the specified monitor. */
void CVMprofiledMonitorExit(CVMProfiledMonitor *self, CVMExecEnv *ee)
{
    CVMReentrantMutex *rm = &self->_super.rmutex;
    CVMassert(self->type != CVM_LOCKTYPE_UNKNOWN);
    CVMreentrantMutexDoUnlock(rm, ee, {
        /* Unlock action: */
        CVMBool hasContention = CVM_FALSE;
        CVMmutexUnlock(&rm->mutex);
        CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
        if (self->contentionCount > 0) {
            self->contentionCount--;
            hasContention = CVM_TRUE;
        }
        CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);
	profileMonitorExit(self, ee, hasContention);
    });
}

/* Purpose: Wait for the specified time period (in milliseconds).  If the
            period is 0, then wait forever. */
/* Returns: CVM_WAIT_NOT_OWNER if current_ee is not the owner of the monitor,
            CVM_WAIT_OK if the thread was woken up by a timeout, and
            CVM_WAIT_INTERRUPTED is the thread was woken up by a notify. */
/* NOTE:    Thread must be in a GC safe state before calling this. */
CVMWaitStatus
CVMprofiledMonitorWait(CVMProfiledMonitor *self, CVMExecEnv *ee,
                       CVMInt64 millis)
{
    CVMSysMonitor *mon = &self->_super;
    CVMWaitStatus result;
    CVMassert(CVMD_isgcSafe(ee));
    CVMsysMonitorDoWait(mon, ee, result, {
        CVMReentrantMutex *rm = &mon->rmutex;
        CVMreentrantMutexDoWait(rm, ee, {
            ee->blockingWaitMonitor = self;
            waitActionResult =
                CVMcondvarWait(&mon->condvar, &rm->mutex, millis);
            ee->blockingWaitMonitor = NULL;
        });
    });
    return result;
}

#endif /* CVM_JVMPI || CVM_JVMTI */
