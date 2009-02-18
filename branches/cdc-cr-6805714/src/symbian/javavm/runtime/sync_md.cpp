/*
 * @(#)sync_md.cpp	1.6 06/10/10
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

extern "C" {
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
}
#include "threads_impl.h"

#include <assert.h>
#include <e32std.h>

#include "javavm/include/porting/ansi/stdlib.h"
#include "memory_impl.h"

CVMBool
CVMmutexInit(CVMMutex* m)
{
    m->count = 0;
    RSemaphore *sem = (RSemaphore *)malloc(sizeof *sem);
    if (sem == NULL) {
	return CVM_FALSE;
    }
    new ((TAny *)sem) RSemaphore();
    if (sem->CreateLocal(0) == KErrNone) {
	m->impl = (void *)sem;
	return CVM_TRUE;
    }
    free(sem);
    return CVM_FALSE;
}

void
CVMmutexDestroy(CVMMutex* m)
{
    RSemaphore *sem = (RSemaphore *)m->impl;
    sem->Close();
    free(sem);
#ifdef CVM_DEBUG
    m->impl = NULL;
#endif
}

CVMBool
CVMmutexTryLock(CVMMutex* m)
{
    if (User::LockedInc(m->count) > 0) {
        User::LockedDec(m->count);
        return CVM_FALSE;
    }
    return CVM_TRUE;
}

void
CVMmutexLock(CVMMutex* m)
{
    if (User::LockedInc(m->count) > 0) {
        RSemaphore *sem = (RSemaphore *)m->impl;
        sem->Wait();
    }
}

void
CVMmutexUnlock(CVMMutex* m)
{
    if (User::LockedDec(m->count)  > 1) {
        RSemaphore *sem = (RSemaphore *)m->impl;
        sem->Signal();
    }
}

void
CVMmutexSetOwner(CVMThreadID *self, CVMMutex *m, CVMThreadID *target)
{
    CVMmutexLock(m);
}

#include "sync_impl.h"

static void
enqueue(CVMCondVar *c, CVMThreadID *t)
{
    *c->last_p = t;
    t->prev_p = c->last_p;
    c->last_p = &t->next;
    t->next = NULL;
}

static CVMThreadID *
dequeue(CVMCondVar *c)
{
    CVMThreadID *t = c->waiters;
    if (t != NULL) {
	c->waiters = t->next;

	if (t->next != NULL) {
	    t->next->prev_p = t->prev_p;
	}

	if (c->last_p == &t->next) {
	    assert(t->prev_p == &c->waiters);
	    c->last_p = &c->waiters;
	}
	t->next = NULL;
	t->prev_p = NULL;
    }
    return t;
}

static void
dequeue_me(CVMCondVar *c, CVMThreadID *t)
{
    *t->prev_p = t->next;

    if (t->next != NULL) {
        t->next->prev_p = t->prev_p;
    }

    if (c->last_p == &t->next) {
        c->last_p = t->prev_p;
    }

    t->next = NULL;
    t->prev_p = NULL;
}

CVMBool
CVMcondvarWait(CVMCondVar* c, CVMMutex* m, CVMJavaLong millis)
{
    CVMBool waitForever;
    CVMThreadID * self = CVMthreadSelf();

    assert(millis >= (CVMJavaLong)0);

    /* Determine the duration to wait: */
    waitForever = (millis == (CVMJavaLong)0);

    /* NOTE: The thing about Thread.interrupt() is that it is asynchronous
       with respect to Object.wait().  Hence, there is a race between when
       the interrupt request and when we actually enter into wait().  If
       the interrupt comes too early (i.e. we're not waiting yet), then we'll
       still end up waiting below.  There's nothing wrong with this. */
    if (self->interrupted) {
        self->interrupted = CVM_FALSE;
        return CVM_FALSE;
    }

    /* NOTE: For notify() and notifyAll(), the following variables and the
       enqueuing of this thread on the condvar's wait list are protected by
       synchronization on the mutex m.  Thread.interrupt() will modify the
       value field asynchronously, but that does not have any negative side
       effects. */
    self->notified = CVM_FALSE;
    self->isWaitBlocked = CVM_TRUE;
#ifdef CVM_DEBUG_ASSERTS
    self->value = 0;
#endif

    /* add ourself to the wait queue that notify and notifyAll look at */
    enqueue(c, self);

    CVMThreadImpl *ti = (CVMThreadImpl *)self->info;
#ifdef CVM_DEBUG
    CVMJavaLong millis0 = millis;
again:
#endif
    ti->waitStatus = KRequestPending;

#ifndef CVM_DEBUG
    if (waitForever) {
	ti->timerRequestPending = CVM_FALSE;
    } else
#endif
    {
	TTime time;
	time.HomeTime();
#ifdef CVM_DEBUG
	if (waitForever) {
	    millis = 1000;
	} else {
	    if (millis0 > 1000) {
		millis = 1000;
	    } else {
		millis = millis0;
	    }
	    millis0 -= millis;
	}
#endif
	time += TTimeIntervalSeconds(millis / 1000);
	time += TTimeIntervalMicroSeconds32((millis % 1000) * 1000);
	ti->waitTimer.At(ti->waitStatus, time);
	ti->timerRequestPending = CVM_TRUE;
    }

    CVMmutexUnlock(m);

    /* notifies can happen now */

    /* emulate semaphore wait */

    User::WaitForRequest(ti->waitStatus);
    ti->waitTimer.Cancel();

    self->isWaitBlocked = CVM_FALSE;
#ifdef CVM_THREAD_SUSPENSION
    /* If we're supposed to be suspended, then suspend self: */
    while (self->isSuspended) {
        suspendSelf();
    }
#endif /* CVM_THREAD_SUSPENSION */

    /* reacquire condvar mutex */
    CVMmutexLock(m);

    /* no more notifications after this point */

    {
        /* find out why we were woken up */
        if (self->notified) {
            /* normal notify or notifyAll */
            assert(self->value == 1);
            /* removed from wait queue */
            assert(self->next == NULL && self->prev_p == NULL);
        } else {
#ifdef CVM_DEBUG
	    if (!self->interrupted) {
		if (waitForever || millis0 > 0) {
		    goto again;
		}
	    }
#endif
            /*
             * We were woken up by Thread.interrupt() posting on
             * the semaphore, or the timed wait timed out.
             */
            assert(self->interrupted || !waitForever);
            dequeue_me(c, self);
            /* removed from wait queue */
            assert(self->next == NULL && self->prev_p == NULL);
        }
    }

#ifdef CVM_DEBUG_ASSERTS
    self->value = 0;
#endif

    if (self->interrupted) {
        self->interrupted = CVM_FALSE;
        return CVM_FALSE;
    }

    return CVM_TRUE;

}

static void
semPost(CVMThreadID *ti)
{
    CVMThreadImpl *i = (CVMThreadImpl *)ti->info;
#ifdef CVM_DEBUG_ASSERTS
    ti->value = 1;
#endif
    RThread t;
    t.Open(i->id);
    TRequestStatus *st = &i->waitStatus;
    t.RequestComplete(st, 1);
    t.Close();
}

void
CVMcondvarNotify(CVMCondVar* c)
{
    CVMThreadID *t;
    if ((t = dequeue(c)) != NULL) {
	t->notified = CVM_TRUE;
	semPost(t);
    }
}

void
CVMcondvarNotifyAll(CVMCondVar* c)
{
    CVMThreadID *t;
    while ((t = dequeue(c)) != NULL) {
	t->notified = CVM_TRUE;
	semPost(t);
    }
}

void
SYMBIANsyncInterruptWait(CVMThreadID *thread)
{
    if (thread->isWaitBlocked) {
	/* thread->notified is not set */
	/* thread will dequeue itself from wait queue */
	semPost(thread);
    }
}

#ifdef CVM_THREAD_SUSPENSION
void
SYMBIANsyncSuspend(CVMThreadID *t)
{
    /* The suspended flag can be modified asynchronously (last one to set its
       value get to say what it is.  This is no different than racing suspend
       and resume requests.  Last request (suspend or resume) determines if
       the target thread is suspended or not.
       NOTE: All the suspension magic is actually done by the target thread.
             The requestor doesn't get to check nor manipulate the state of
             the target thread.  It only gets to make a request to suspend or
             resume the target thread.  This allows us to avoid having to do
             some fancy synchronization between the requestor and the target
             thread when manipulating the target thread's state.
    */
    t->isSuspended = CVM_TRUE;
    CVMThreadImpl *ti = (CVMThreadImpl *)t->info;
    RThread target;
    target.Open(ti->id);
    target.Suspend();
    target.Close();
}

void
SYMBIANsyncResume(CVMThreadID *t)
{
    /* The suspended flag can be modified asynchronously (last one to set its
       value get to say what it is.  This is no different than racing suspend
       and resume requests.  Last request (suspend or resume) determines if
       the target thread is suspended or not.
       NOTE: All the suspension magic is actually done by the target thread.
             The requestor doesn't get to check nor manipulate the state of
             the target thread.  It only gets to make a request to suspend or
             resume the target thread.  This allows us to avoid having to do
             some fancy synchronization between the requestor and the target
             thread when manipulating the target thread's state.
    */
    t->isSuspended = CVM_FALSE;
    CVMThreadImpl *ti = (CVMThreadImpl *)t->info;
    RThread target;
    target.Open(ti->id);
    target.Resume();
    target.Close();
}
#endif /* CVM_THREAD_SUSPENSION */

CVMBool
CVMcondvarInit(CVMCondVar *c, CVMMutex* m)
{
    CVMCondVarImpl *i = new (GLOBAL_HEAP) CVMCondVarImpl;
    if (i == NULL) {
	return CVM_FALSE;
    }
    c->waiters = NULL;
    c->last_p = &c->waiters;
    c->impl = (void *)i;
    return CVM_TRUE;
}

void
CVMcondvarDestroy(CVMCondVar *c)
{
    CVMCondVarImpl *i = (CVMCondVarImpl *)c->impl;
    i->~CVMCondVarImpl();
    CVMfree(i);
#ifdef CVM_DEBUG
    c->impl = NULL;
#endif
}
