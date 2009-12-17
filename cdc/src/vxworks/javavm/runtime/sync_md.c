/*
 * @(#)sync_md.c	1.22 06/10/10
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

#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#define VXWORKS_PRIVATE
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/int.h"
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif


CVMBool
CVMmutexInit(CVMMutex * m)
{
    m->m = semMCreate(SEM_INVERSION_SAFE | SEM_Q_PRIORITY);
    return m->m != NULL;
}

void
CVMmutexDestroy(CVMMutex * m)
{
    semDelete(m->m);
}

CVMBool
CVMmutexTryLock(CVMMutex * m)
{
    return semTake(m->m, NO_WAIT) == OK;
}

void
CVMmutexLock(CVMMutex * m)
{
    semTake(m->m, WAIT_FOREVER);
}

void
CVMmutexUnlock(CVMMutex * m)
{
    semGive(m->m);
}

CVMBool
CVMcondvarInit(CVMCondVar * c, CVMMutex * m)
{
    c->sem = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    return c->sem != NULL;
}

void
CVMcondvarDestroy(CVMCondVar * c)
{
    semDelete(c->sem);
}

CVMBool
CVMcondvarWait(CVMCondVar * c, CVMMutex * m, CVMJavaLong millis)
{
    volatile CVMBool result = CVM_TRUE;
    CVMThreadID *self = CVMthreadSelf();
    volatile int timeout = WAIT_FOREVER;

    if (!CVMlongEqz(millis)) {
	/* Round up to nearest tick */
	CVMJavaLong ltmp, ltmp1, clocks, one_thousand, nine99;
	CVMJavaInt itmp;
	one_thousand = CVMint2Long(1000);
	nine99 = CVMint2Long(999);
	clocks = CVMint2Long(CLOCKS_PER_SEC);
	ltmp = CVMlongMul(millis, clocks);
	if (!CVMlongLtz(ltmp)) {
	    ltmp = CVMlongAdd(ltmp, nine99);
	    if (!CVMlongLtz(ltmp)) {
		ltmp = CVMlongDiv(ltmp, one_thousand);
		itmp = CVMlong2Int(ltmp);
		ltmp1 = CVMint2Long(itmp);
		if (CVMlongEq(ltmp, ltmp1)) {
		    /* (CLOCKS_PER_SEC * millis + 999) / 1000 */
		    timeout = itmp;
		}
	    }
	}
    }

    taskLock();

    if (self->interrupted) {
        self->interrupted = CVM_FALSE;
        taskUnlock();
        return CVM_FALSE;
    }
    self->in_wait = CVM_TRUE;

    CVMmutexUnlock(m);

    if (setjmp(self->env) == 0) {
	semTake(c->sem, timeout);
    } else {
	assert(self->interrupted);
        self->interrupted = CVM_FALSE;
        result = CVM_FALSE;
    }

    self->in_wait = CVM_FALSE;

    taskUnlock();

    CVMmutexLock(m);

    return result;
}

void
CVMcondvarNotify(CVMCondVar * c)
{
    taskLock();
    semGive(c->sem);		/* pull one waiter off the queue */
    semTake(c->sem, NO_WAIT);	/* but don't leave it available */
    taskUnlock();
}

void
CVMcondvarNotifyAll(CVMCondVar * c)
{
    semFlush(c->sem);		/* remove all waiters */
}

#include "javavm/include/porting/threads.h"

void
CVMschedLock()
{
    taskLock();
}

void
CVMschedUnlock()
{
    taskUnlock();
}

#include <signal.h>
#include <assert.h>

static void sigfunc0(int sig)
{
    CVMThreadID *self = CVMthreadSelf();
    assert(self->taskID == taskIdSelf());

    if (self->targetMutex == NULL) {
	fprintf(stderr, "Spurious signal %d\n", sig);
    } else {
	CVMBool success = CVMmutexTryLock(self->targetMutex);
	assert(success), (void) success;

	self->targetMutex = NULL;

	semGive(self->setownerCV);

	/* reset priority */
	taskPrioritySet(0, self->lastPriority);

	taskDelay(0);	/* yield */
    }
}

void
CVMmutexSetOwner(CVMThreadID *self, CVMMutex *m, CVMThreadID *t)
{
    int myPriority;

    assert(t != self);

    CVMmutexLock(&t->setOwnerLock);

    t->targetMutex = m;

    taskPriorityGet(t->taskID, &t->lastPriority);
    taskPriorityGet(0, &myPriority);

    taskLock();

    if (myPriority < t->lastPriority) {
	taskPrioritySet(t->taskID, myPriority);
    }

    kill(t->taskID, SIGUSR1);

    taskUnlock();

    semTake(t->setownerCV, WAIT_FOREVER);

    assert(t->targetMutex == NULL);

    CVMmutexUnlock(&t->setOwnerLock);
}

static void sigfunc1(int sig)
{
    CVMThreadID *self = CVMthreadSelf();
    assert(self->taskID == taskIdSelf());
    if (self->in_wait && self->interrupted) {
	longjmp(self->env, 1);
    } else {
	fprintf(stderr, "Spurious signal %d\n", sig);
    }
}

void
vxworksSyncInterruptWait(CVMThreadID *thread)
{
    /* %comment: mam053 */
    kill(thread->taskID, SIGUSR2);
}

CVMBool
vxworksSyncInit(CVMThreadID *self)
{
    struct sigaction sa;
    sa.sa_handler = sigfunc0;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR2);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
	return CVM_FALSE;
    }
    sa.sa_handler = sigfunc1;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR1);
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
	return CVM_FALSE;
    }

#ifdef CVM_JIT
    /* JIT-specific initialization */
    if (!vxworksJITSyncInitArch()) {
	return CVM_FALSE;
    }
#endif

    self->setownerCV = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    if (self->setownerCV == NULL) {
	return CVM_FALSE;
    }
    CVMmutexInit(&self->setOwnerLock);

    return CVM_TRUE;
}

void
vxworksSyncDestroy(CVMThreadID *self)
{
    semDelete(self->setownerCV);
    CVMmutexDestroy(&self->setOwnerLock);
}
