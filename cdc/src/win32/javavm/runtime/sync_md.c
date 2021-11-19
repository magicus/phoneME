/*
 * @(#)sync_md.c	1.23 06/10/10
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

#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/ansi/assert.h"
#ifndef WINCE
#include <stdio.h>
#endif

CVMBool
CVMmutexInit(CVMMutex * m)
{
#if 1
    __try {
	InitializeCriticalSection(&m->crit);
	return CVM_TRUE;
    } __except (_exception_code() == STATUS_NO_MEMORY) {
	return CVM_FALSE;
    }
#else
    m->h = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (m->h == 0)
        printf("CVMmutexInit Error \n");
    return (m->h != 0);
#endif
}

void
CVMmutexDestroy(CVMMutex * m)
{
#if 1
    DeleteCriticalSection(&m->crit);
#else
    CloseHandle(m->h);
#endif
}

CVMBool
CVMmutexTryLock(CVMMutex * m)
{
#if 1
#ifndef WINCE
#if (_WIN32_WINNT >= 0x0400)
#else
#error No TryEnterCriticalSection for win32 version _WIN32_WINNT
#endif
#endif
    return TryEnterCriticalSection(&m->crit);
#else
    return (WaitForSingleObject(m->h, 0) == WAIT_OBJECT_0);
#endif
}

void
CVMmutexLock(CVMMutex * m)
{
#ifdef CVM_DEBUG
if (CVMthreadSelf() != NULL) {
    //    CVMthreadSelf()->where = 5;
}
#endif
#if 1
 {
#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
     CVMThreadID *self = CVMthreadSelf();
     if (self != NULL) {

	 self->is_mutex_blocked = CVM_TRUE;
	 if (self->suspended && !self->suspended_in_wait &&
	     !self->suspended_in_mutex_blocked) {
	     int count;
	     /*
	      * If we get there then the win32syncsuspend code didn't
	      * see the is_mutex_blocked flag and issued a SuspendThread call.
	      * It could have happened well before we entered this code.
	      * Since 'suspended' is set and 'is_mutex_blocked' is now set
	      * we can Resume this thread. If another attempt comes in from
	      * some other thread to suspend this thread it will set the
	      * suspended_in_mutex_blocked flag.  If a ResumeThread comes
	      * in from some other thread, no problem because suspend count
	      * in thread will not go below zero.
	      */
	     while ((count = ResumeThread(self->handle)) > 1 &&
		    count != 0xffffffff) 
		 ;
	     /* It's possible that some other thread called win32syncResume on
	      * this thread.
	      * That's ok, since is_mutex_blocked is set no other thread
	      * will call SuspendThread on our behalf.  We can now 
	      * safely grab the self->lock
	      */
	     EnterCriticalSection(&self->lock.crit);
	     self->suspended_in_mutex_blocked = 1;
	     /* If some other thread Resumed this thread then we clear
	      * some flags and continue
	      */
	     if (!self->suspended) {
		 self->suspended_in_mutex_blocked = 0;
	     }
		 
	     /*
	      * once we release the self->lock, if this thread is still
	      * suspended then a win32syncResume will result in a setEvent call
	      */
	     LeaveCriticalSection(&self->lock.crit);
	 }

	 while (CVM_TRUE) {
	     /*
	      * If 'suspended' is set and other flags not set then that is
	      * a race we don't know about!  
	      */
	     assert(!(self->suspended && !self->suspended_in_mutex_blocked &&
	       !self->suspended_in_wait));
	     EnterCriticalSection(&m->crit);

	     if (self->suspended_in_mutex_blocked) {
		 assert(self->suspended);
		 assert(!self->suspended_in_wait);
		 /*
		  * This thread has been given ownership of the mutex
		  * while it's supposed to be suspended.  Release the mutex
		  * and 'suspend' the thread.  We can acquire the mutex again
		  * later.  We don't grab the self->lock since we're
		  * only concerned with suspends that happen while this
		  * thread is actually in the EnterCrit. call.  That call is
		  * flagged by is_mutex_blocked.  Any attempt to suspend this
		  * thread after that flag is set will result in 
		  * suspended_in_mutex_blocked being set and a SetEvent
		  * happening.
		  */
		 LeaveCriticalSection(&m->crit);
		 WaitForSingleObject(self->suspend_cv, INFINITE);
		 assert(!(self->suspended &&
			  !self->suspended_in_mutex_blocked &&
			  !self->suspended_in_wait));
	     } else {
		 break;
	     }
	 }
	 self->is_mutex_blocked = CVM_FALSE;

     } else
#endif	 /* defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION) */
	 {
	     EnterCriticalSection(&m->crit);
	 }

 }
#else
    WaitForSingleObject(m->h, INFINITE);
#endif
#ifdef CVM_DEBUG
if (CVMthreadSelf() != NULL) {
    CVMthreadSelf()->where = 0;
}
#endif
}
void
CVMmutexUnlock(CVMMutex * m)
{
#if 1
    LeaveCriticalSection(&m->crit);
#else
    SetEvent(m->h);
#endif
    {
	CVMThreadID *self = CVMthreadSelf();
	if (self != NULL) {
	    if (self->suspended == 1) {
		int count = ResumeThread(self->handle);
		if (count >= 1) {
		    self->suspended = 1;
		}
	    }
	}
    }
		
}

/* emulate a semaphore post */
#define SEM_POST(t) \
do {		\
    CVMmutexLock(&t->wait_mutex);	\
    t->value = 1;				\
    SetEvent(t->wait_cv);		\
    CVMmutexUnlock(&t->wait_mutex);	 	\
} while (0)

static void
CVMDQueueInit(CVMDQueue *q)
{
    q->waiters = NULL;
    q->last_p = &q->waiters;
}

CVMBool
CVMcondvarInit(CVMCondVar * c, CVMMutex * m)
{
    CVMDQueueInit(&c->q);
    return(CVM_TRUE);
}

void
CVMcondvarDestroy(CVMCondVar * c)
{
/*    condvarDestroy(c); */
}

static void
enqueue(CVMDQueue *c, CVMThreadID *t)
{
    assert(t->prev_p == NULL);
    assert(t->next == NULL);
    *c->last_p = t;
    t->prev_p = c->last_p;
    c->last_p = &t->next;
    t->next = NULL;
}

static CVMThreadID *
dequeue(CVMDQueue *c)
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
dequeue_me(CVMDQueue *c, CVMThreadID *t)
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
CVMcondvarWait(CVMCondVar * c, CVMMutex * m, CVMJavaLong millis)
{
    CVMBool result = CVM_TRUE;
    CVMThreadID *self = CVMthreadSelf();
    DWORD timeout = INFINITE;

#ifdef CVM_DEBUG
    CVMthreadSelf()->where = 7;
#endif
    CVMmutexLock(&self->lock);
    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	CVMmutexUnlock(&self->lock);
	return CVM_FALSE;
    }
    self->in_wait = CVM_TRUE;
    self->notified = CVM_FALSE;

    /* add ourself to the wait queue that notify and notifyAll look at */
    enqueue(&c->q, self);

    CVMmutexUnlock(&self->lock);
    CVMmutexUnlock(m);

    /* emulate semaphore wait */

    ResetEvent(self->wait_cv);
    while (self->value == 0) {
	int status;

	if (millis == 0 || millis > 0xffffffffi64) {
	    timeout = INFINITE;
	} else {
	    timeout = (long)millis;
	}

#ifdef CVM_DEBUG
	CVMthreadSelf()->where = 8;

	if (timeout == INFINITE) {
	    status = WaitForSingleObject(self->wait_cv, 1000);
	} else {
	    status = WaitForSingleObject(self->wait_cv, timeout);
	}
	if (status == WAIT_TIMEOUT && timeout != INFINITE) {
#else
	status = WaitForSingleObject(self->wait_cv, timeout);
	if (status == WAIT_TIMEOUT) {
#endif
		break;
	}

        else if (status == WAIT_FAILED ) {
		printf("CVMcondvar wait failed %d \n", GetLastError());
	}
    }

    /* Make sure we're not supposed to be still suspended before getting on
       with reacquiring the mutex lock: */
    CVMmutexLock(&self->lock);

    while (self->suspended) {
#ifdef CVM_DEBUG
	self->where = 9;
#endif
	CVMmutexUnlock(&self->lock);
	WaitForSingleObject(self->suspend_cv, INFINITE);
	CVMmutexLock(&self->lock);
    }

    self->in_wait = CVM_FALSE;
    CVMmutexUnlock(&self->lock);

#ifdef CVM_DEBUG
    self->where = 11;
#endif
    /* reacquire condvar mutex */
    CVMmutexLock(m);

    /* no more notifications after this point */

    /* find out why we were woken up */
    if (self->notified) {
	/* normal notify or notifyAll */
	assert(self->value == 1);
	/* removed from wait queue */ 
	 assert(self->next == NULL && self->prev_p == NULL);
    } else {
	/*
	* We were woken up by linuxSyncInterruptWait() posting on
	* the semaphore, or the timed wait timed out.
	*/
	assert(self->interrupted || millis != 0);
	dequeue_me(&c->q, self);
	/* removed from wait queue */ 
	assert(self->next == NULL && self->prev_p == NULL);
    }

    self->value = 0;

#ifdef CVM_DEBUG
    self->where = 12;
#endif
    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	return CVM_FALSE;
    }

    return CVM_TRUE;
}

void
CVMcondvarNotify(CVMCondVar * c)
{
    CVMThreadID *t;
    if ((t = dequeue(&c->q)) != NULL) {
	t->notified = CVM_TRUE;
	SEM_POST(t);
    }
}

void
CVMcondvarNotifyAll(CVMCondVar * c)
{
    CVMThreadID *t;
    while ((t = dequeue(&c->q)) != NULL) {
	t->notified = CVM_TRUE;
	SEM_POST(t);
    }
}

void
win32SyncInterruptWait(CVMThreadID *thread)
{
    CVMmutexLock(&thread->lock);
    if (thread->in_wait) {
	/* thread->notified is not set */
	/* thread will dequeue itself from wait queue */
	SEM_POST(thread);
    }
    CVMmutexUnlock(&thread->lock);
}

void
win32SyncSuspend(CVMThreadID *t)
{
#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    if (t == CVMthreadSelf()) {
	t->suspended = 1;
	SuspendThread(GetCurrentThread());
    } else {
	CVMmutexLock(&t->lock);
	if (!t->suspended) {
	    t->suspended = 1;
	    /*
	    * If the thread is performing a wait, we do special handling,
	    * to avoid the use of signals.
	    */
	    if (t->in_wait) {
		t->suspended_in_wait = 1;
	    } else {
		SuspendThread(t->handle);
		if (t->is_mutex_blocked) {
		    /*
		     * Waiting on some mutex, don't suspend as we could
		     * hit deadlock.  Special handling for this as well.
		     */
		    t->suspended_in_mutex_blocked = 1;
		    ResumeThread(t->handle);
		}
	    }

	}
	CVMmutexUnlock(&t->lock);
    }
#endif	 /* defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION) */
}

void
win32SyncResume(CVMThreadID *t)
{
#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    CVMmutexLock(&t->lock);
    if (t->suspended) {
	t->suspended = 0;
	if (t->suspended_in_wait) {
	    t->suspended_in_wait = 0;
	    SetEvent(t->suspend_cv);
	} else if (t->suspended_in_mutex_blocked) {
	    t->suspended_in_mutex_blocked = 0;
	    SetEvent(t->suspend_cv);
	} else {
	    ResumeThread(t->handle);
	}
    }
    CVMmutexUnlock(&t->lock);
#endif	 /* defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION) */
}

#ifdef CVM_ADV_THREAD_BOOST
CVMBool
CVMthreadBoostInit(CVMThreadBoostRecord *bp)
{
    bp->currentRecord = NULL;
    return CVM_TRUE;
}

CVMBool
WIN32threadBoostInit(CVMThreadBoostQueue *b)
{
    HANDLE ev;
    if (!CVMmutexInit(&b->lock)) {
	return CVM_FALSE;
    }
    CVMDQueueInit(&b->boosters);
    b->nboosters = 0;
    ev = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ev == NULL) {
	CVMmutexDestroy(&b->lock);
	return CVM_FALSE;
    }
    b->handles[0] = ev;
    b->handles[1] = NULL;
    return CVM_TRUE;
}

void
CVMthreadBoostDestroy(CVMThreadBoostRecord *b)
{
}

void
WIN32threadBoostDestroy(CVMThreadBoostQueue *b)
{
    CVMmutexDestroy(&b->lock);
    CloseHandle(b->handles[0]);
    assert(b->nboosters == 0);
#ifdef CVM_DEBUG
    b->handles[0] = NULL;
    b->handles[1] = NULL;
#endif
}

/* Protected by syncLock */

CVMThreadBoostQueue *
CVMthreadAddBooster(CVMThreadID *self, CVMThreadBoostRecord *bp,
    CVMThreadID *target)
{
    CVMThreadBoostQueue *b = bp->currentRecord;
    if (b == NULL) {
	/* I am first */
	b = bp->currentRecord = self->boost;
	assert(self->boost != NULL);
	self->boost = NULL;
	b->handles[1] = target->threadMutex;
#ifdef CVM_DEBUG_ASSERTS
	b->boosting = CVM_TRUE;
#endif
    }
    ++b->nboosters;
    assert(self->next == NULL);
    assert(self->prev_p == NULL);
    enqueue(&b->boosters, self);
    assert(b->handles[1] == target->threadMutex);
    return b;
}

/* Not protected by syncLock */

void
CVMthreadBoostAndWait(CVMThreadID *self, CVMThreadBoostRecord *bp,
    CVMThreadBoostQueue *b)
{
    DWORD status;
    assert(b->nboosters > 0);
    status = WaitForMultipleObjects(2, b->handles, FALSE, INFINITE);
#ifdef CVM_DEBUG_ASSERTS
#if 0
    assert(status == WAIT_OBJECT_0 + 0);
#else
    if (status != WAIT_OBJECT_0 + 0) {
	CVMconsolePrintf("CVMthreadBoostAndWait: unexpected wait status %x\n",
	    status);
    }
#endif
    /* no spurious wakeups allowed */
    assert(!b->boosting);
#endif
    assert(bp->currentRecord != b);
    CVMmutexLock(&b->lock);
    assert(b->nboosters > 0);
    assert((b->nboosters == 0) == (b->boosters.waiters == NULL));
    --b->nboosters;
    dequeue_me(&b->boosters, self);
    assert((b->nboosters == 0) == (b->boosters.waiters == NULL));
    /* Last to wakeup should have a missing boost record */
    assert(b->boosters.waiters != NULL || self->boost == NULL);
    if (self->boost == NULL) {
	/* I need my record back. */
	CVMThreadID *first = b->boosters.waiters;
	if (first != NULL) {
	    /* Steal one */
	    self->boost = first->boost;
	    first->boost = NULL;
	} else {
	    assert(b->nboosters == 0);
	    ResetEvent(b->handles[0]);
	    self->boost = b;
	}
    }
    CVMmutexUnlock(&b->lock);
    assert(self->boost != NULL);
}

void
CVMthreadCancelBoost(CVMThreadID *self, CVMThreadBoostRecord *bp)
{
    CVMThreadBoostQueue *b = bp->currentRecord;
    if (b != NULL) {
	/* Tell boosters to wake up */
	BOOL ok;

	bp->currentRecord = NULL;
#ifdef CVM_DEBUG_ASSERTS
	b->boosting = CVM_FALSE;
#endif
	ok = SetEvent(b->handles[0]);
	assert(ok); (void)ok;
    }
}

#endif
