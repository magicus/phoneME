/*
 * @(#)sync_md.c	1.39 06/10/10
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
#include "javavm/include/porting/float.h"	/* for setFPMode() */
#include "javavm/include/porting/globals.h"
#include <thread.h>
#include <synch.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef CVM_JVMTI
#include "javavm/include/globals.h"
#include "javavm/include/jvmtiExport.h"
#endif
#ifdef CVM_JVMPI
#include "javavm/include/globals.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/porting/jit/jit.h"
#endif

#ifdef CVM_ADV_MUTEX_SET_OWNER

#undef CVMmutexSetOwner /* In case it's overridden */
void
CVMmutexSetOwner(CVMThreadID *self, CVMMutex *m, CVMThreadID *target)
{
    /* This is dependent on the pthreads library implementation.
       A private API would be preferable */
    m->pmtx = target->locked;
    assert(!CVMmutexTryLock(m));
}
#endif

#ifdef CVM_ADV_THREAD_BOOST

CVMBool
SOLARISthreadBoostInit(CVMThreadID *self)
{
    self->boost = (CVMThreadBoostQueue *)calloc(1, sizeof *self->boost);
    if (self->boost == NULL) {
	goto failed1;
    }
#ifdef SOLARIS_BOOST_MUTEX
    self->thread_lock =
	(SOLARISBoostLock *)calloc(1, sizeof *self->thread_lock);
    if (self->thread_lock == NULL) {
	goto failed2;
    }
    self->next_thread_lock =
	(SOLARISBoostLock *)calloc(1, sizeof *self->next_thread_lock);
    if (self->next_thread_lock == NULL) {
	goto failed3;
    }
    mutex_init(&self->thread_lock->lock, USYNC_THREAD, NULL);
    {
        int err = mutex_trylock(&self->thread_lock->lock);
        assert(err == 0), (void)err;
    }
    mutex_init(&self->boost->lock, USYNC_THREAD, NULL);
    mutex_init(&self->next_thread_lock->lock, USYNC_THREAD, NULL);

    self->boost->next_thread_lock = self->next_thread_lock;
#else
    if (sem_init(&self->boost->sem, 0, 0) != 0) {
	goto failed1;
    }
#endif
    self->boost->nboosters = 0;
    self->boost->boosters.waiters = NULL;
    self->boost->boosters.last_p = &self->boost->boosters.waiters;
    self->boost->boosting = CVM_FALSE;
    return CVM_TRUE;

#ifdef SOLARIS_BOOST_MUTEX
failed3:
    free(self->thread_lock);
failed2:
    free(self->boost);
#else
#endif

failed1:
    return CVM_FALSE;
}

#ifdef SOLARIS_BOOST_MUTEX
static void
destroyBoostLock(SOLARISBoostLock *b)
{
    assert(b->nboosters == 0);
    mutex_destroy(&b->lock);
}
#endif

void
SOLARISthreadBoostDestroy(CVMThreadID *self)
{
#ifdef SOLARIS_BOOST_MUTEX
    SOLARISBoostLock *b = self->thread_lock;
    self->thread_lock = NULL;
    destroyBoostLock(b);
    free(b);
    mutex_destroy(&self->boost->lock);
#else
    sem_destroy(&self->boost->sem);
#endif
    free(self->boost);
}

#ifdef SOLARIS_BOOST_MUTEX
static void
boostLockRemoveReference(CVMThreadBoostQueue *b)
{
    CVMBool destroy = CVM_FALSE;
    mutex_lock(&b->target->lock);
    assert(b->thread_lock->nboosters > 0);
    --b->thread_lock->nboosters;
    if (b->thread_lock->nboosters == 0) {
	destroy = CVM_TRUE;
    }
    mutex_unlock(&b->target->lock);
    if (destroy) {
	destroyBoostLock(b->thread_lock);
	free(b->thread_lock);
    }
}
#endif

CVMBool
CVMthreadBoostInit(CVMThreadBoostRecord *b)
{
    b->currentRecord = NULL;
    return CVM_TRUE;
}

void
CVMthreadBoostDestroy(CVMThreadBoostRecord *b)
{
    /* Do nothing */
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
	assert(b->boosting == CVM_FALSE);
	assert(b->nboosters == 0);

#ifdef SOLARIS_BOOST_MUTEX
	b->target = target;
	b->boosting = CVM_TRUE;

	b->thread_lock = NULL;

	assert(b->next_thread_lock != NULL);
	mutex_lock(&b->target->lock);
	b->thread_lock = b->target->thread_lock;
	++b->thread_lock->nboosters;
	mutex_unlock(&b->target->lock);
#endif
    } else {
	assert(b->target == target);
    }

    ++b->nboosters;
    assert(b->boosting);
    enqueue(&b->boosters, self);
    return b;
}

void
CVMthreadBoostAndWait(CVMThreadID *self, CVMThreadBoostRecord *bp,
    CVMThreadBoostQueue *b)
{
    CVMBool destroy = CVM_FALSE;
    assert(b->nboosters > 0);
#ifdef SOLARIS_BOOST_MUTEX
{
    SOLARISBoostLock *l = b->thread_lock;
    do {
	mutex_lock(&l->lock);
	mutex_unlock(&l->lock);
    } while (b->boosting);
}
#else
    sem_wait(&b->sem);
#endif

    assert(!b->boosting);
    mutex_lock(&b->lock);
    assert(b->nboosters > 0);
    assert((b->nboosters == 0) == (b->boosters.waiters == NULL));
    --b->nboosters;
    dequeue_me(&b->boosters, self);
    assert((b->nboosters == 0) == (b->boosters.waiters == NULL));
    /* Last to wakeup should have a missing boost record */
    assert(b->boosters.waiters != NULL || self->boost == NULL);
    if (self->boost == NULL) {
	/* I need my record back */
	if (b->boosters.waiters != NULL) {
	    /* Steal one */
	    self->boost = b->boosters.waiters->boost;
	    b->boosters.waiters->boost = NULL;
	    assert(self->boost != NULL);
	} else {
	    self->boost = b;
	}
    }

    assert(!b->boosting);
    if (b->nboosters == 0) {
	destroy = CVM_TRUE;
    }
    mutex_unlock(&b->lock);

    if (destroy) {
	if (b->thread_lock != NULL) {
	    boostLockRemoveReference(b);
	}
    }
}

/* Protected by syncLock */

void
CVMthreadCancelBoost(CVMThreadID *self, CVMThreadBoostRecord *bp)
{
    CVMThreadBoostQueue *b = bp->currentRecord;
    if (b == NULL) {
	return;
    }
    bp->currentRecord = NULL;
#ifdef SOLARIS_BOOST_MUTEX
    assert(self == b->target);

    mutex_lock(&self->lock);
    if (self->thread_lock->nboosters > 0) {
	int err = mutex_trylock(&b->next_thread_lock->lock);
	assert(err == 0), (void)err;
	self->thread_lock = b->next_thread_lock;
	assert(self->thread_lock->nboosters == 0);
#ifdef CVM_DEBUG
	b->next_thread_lock = NULL;
#endif
    }
    mutex_unlock(&self->lock);

    if (b->nboosters > 0) {
	mutex_lock(&b->lock);
	b->boosting = CVM_FALSE;
	assert(b->thread_lock->nboosters > 0);
	mutex_unlock(&b->lock);

	mutex_unlock(&b->thread_lock->lock);
    } else {
	assert(b->nboosters == 0);
	if (b->thread_lock != NULL) {
	    boostLockRemoveReference(b);
	}
    }
#else
    int i;
    mutex_lock(&b->lock);
    for (i = 0; i < b->nboosters; ++i) {
	sem_post(&b->sem);
    }
    if (b->nboosters == 0) {
	if (b->thread_lock != NULL) {
	    boostLockRemoveReference(b);
	}
    }
    mutex_unlock(&b->lock);
#endif
    assert(!b->boosting);
}
#endif

static void sigfunc1(int sig)
{
    CVMThreadID *self = CVMthreadSelf();
    assert(SOLARIS_COOKIE(self) == thr_self());
    if (self->interrupted) {
#ifdef SOLARIS_BUG_4377403_FIXED
	if (self->in_wait) {
	    siglongjmp(self->env, 1);
	}
#else
	/*
	 * We know that the signal itself is enough to cause the
	 * wait to abort.
	 */
#endif
    } else {
	fprintf(stderr, "Spurious signal %d\n", sig);
    }
}

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
void sigquitHandler(int sig)
{
#ifdef CVM_JVMPI
    CVMjvmpiSetDataDumpRequested();
#endif
#ifdef CVM_JVMTI
    CVMjvmtiSetDataDumpRequested();
#endif
}
#endif

#undef CVMcondvarWait /* In case it's overridden */
CVMBool
CVMcondvarWait(CVMCondVar* c, CVMMutex* m, CVMJavaLong millis)
{
    CVMBool result = CVM_TRUE;
    CVMThreadID * volatile self = CVMthreadSelf();
    sigset_t osigset;

    mutex_lock(&self->lock);

    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	mutex_unlock(&self->lock);
	return CVM_FALSE;
    }

    /* temporarily block USR2 */
    thr_sigsetmask(SIG_BLOCK, &CVMtargetGlobals->sigusr2Mask, &osigset);
    /* make sure USR2 will be unblocked when we restore sigmask */
    assert(!sigismember(&osigset, SIGUSR2));

    self->in_wait = CVM_TRUE;

    mutex_unlock(&self->lock);

#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
    POSIXmutexLock(&c->m);
    POSIXmutexUnlock(&m->pmtx);
#endif

#ifdef SOLARIS_BUG_4377403_FIXED
    if (sigsetjmp(self->env, 1) == 0) {
#endif 
	/* restore sigmask, unblocking USR2 */
	thr_sigsetmask(SIG_SETMASK, &osigset, NULL);
#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
	(void) POSIXcondvarWait(&c->pcv, &c->m, millis);
#else
	(void) POSIXcondvarWait(&c->pcv, &m->pmtx, millis);
#endif
#ifdef SOLARIS_BUG_4377403_FIXED
    } else {
	/* signal handler jumped here using siglongjmp, restore signal mask */
	thr_sigsetmask(SIG_SETMASK, &osigset, NULL);

	/* NOTE: lock should be owned by us, but how can we check? */

	assert(self->interrupted);
    }
#endif

    self->in_wait = CVM_FALSE;

#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
    POSIXmutexUnlock(&c->m);
#endif

    mutex_lock(&self->lock);
    /*
     * Check and clear interrupted flag.  The in_wait flag
     * must already be cleared, otherwise we may get
     * sent another signal.
     */
    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	result = CVM_FALSE;
    }
    mutex_unlock(&self->lock);

#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
    POSIXmutexLock(&m->pmtx);
#endif
    setFPMode();
    return result;
}

void
solarisSyncInterruptWait(CVMThreadID *thread)
{
    int kill_status;
    /* NOTE: Maybe we should wake up all waiters using broadcast instead? */
    kill_status = thr_kill(SOLARIS_COOKIE(thread), SIGUSR2);
    assert(kill_status == 0);
}

#ifdef SOLARIS_ACQUIRES_LOCK_BEFORE_CALLING_HANDLER
#undef CVMcondvarInit /* In case it's overridden */
CVMBool
CVMcondvarInit(CVMCondVar * c, CVMMutex *m)
{
    if (POSIXmutexInit(&c->m)) {
	if (POSIXcondvarInit(&c->pcv, &m->pmtx)) {
	    return CVM_TRUE;
	} else {
	    POSIXmutexDestroy(&c->m);
	}
    }
    return CVM_FALSE;
}

#undef CVMcondvarDestroy /* In case it's overridden */
void
CVMcondvarDestroy(CVMCondVar * c)
{
    POSIXcondvarDestroy(&c->pcv);
    POSIXmutexDestroy(&c->m);
}

#undef CVMcondvarNotify /* In case it's overridden */
void
CVMcondvarNotify(CVMCondVar * c)
{
    POSIXmutexLock(&c->m);
    POSIXcondvarNotify(&c->pcv);
    POSIXmutexUnlock(&c->m);
}

#undef CVMcondvarNotifyAll /* In case it's overridden */
void
CVMcondvarNotifyAll(CVMCondVar * c)
{
    POSIXmutexLock(&c->m);
    POSIXcondvarNotifyAll(&c->pcv);
    POSIXmutexUnlock(&c->m);
}
#endif

CVMBool
solarisSyncInit(void)
{
    struct sigaction sa;
    sa.sa_handler = sigfunc1; /* interrupt wait */
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    /*
     * but we allow interrupt-wait to be interrupted by a
     * set-owner request
     */
    sigaction(SIGUSR2, &sa, NULL);

    sigemptyset(&CVMtargetGlobals->sigusr2Mask);
    sigaddset(&CVMtargetGlobals->sigusr2Mask, SIGUSR2);

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
    sa.sa_handler = sigquitHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    /*
     * mask SIGQUIT, so set-owner cannot be interrupted by an
     * interrupt-wait request
     */
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaction(SIGQUIT, &sa, NULL);
#endif

#ifdef CVM_ADV_MUTEX_SET_OWNER
    /* Sanity check to make sure raw copy of locked mutex is safe */
    {
	POSIXMutex test1, test2;
	memset(&test1, 0, sizeof test1);
	memset(&test2, 0, sizeof test2);
	if (!POSIXmutexInit(&test1)) {
	    return CVM_FALSE;
	} else {
	    CVMBool ok;
	    if (!POSIXmutexInit(&test2)) {
		POSIXmutexDestroy(&test1);
		return CVM_FALSE;
	    }
	    ok = POSIXmutexTryLock(&test2);
	    if (!ok) {
		fprintf(stderr, "trylock failed\n");
		POSIXmutexDestroy(&test1);
		POSIXmutexDestroy(&test2);
		return CVM_FALSE;
	    }
	    ok = POSIXmutexTryLock(&test1);
	    if (!ok) {
		fprintf(stderr, "trylock failed\n");
		POSIXmutexDestroy(&test1);
		POSIXmutexUnlock(&test2);
		POSIXmutexDestroy(&test2);
		return CVM_FALSE;
	    }
	    ok = (memcmp(&test1, &test2, sizeof test1) == 0);
	    POSIXmutexUnlock(&test1);
	    POSIXmutexDestroy(&test1);
	    POSIXmutexUnlock(&test2);
	    POSIXmutexDestroy(&test2);
	    if (!ok) {
		fprintf(stderr, "locked mutexes not identical\n");
		return CVM_FALSE;
	    }
	}
    }
#endif

#ifdef CVM_JIT
    return solarisJITSyncInitArch();
#else
    return CVM_TRUE;
#endif
}
