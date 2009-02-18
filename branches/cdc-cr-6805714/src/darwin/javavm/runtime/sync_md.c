/*
 * @(#)sync_md.c	1.26 06/10/10
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

#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/float.h"	/* for setFPMode() */
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif

/*
 * signal handler for CVMthreadSuspend() request
 */

static void sigfunc0(int sig)
{
    CVMThreadID *self = CVMthreadSelf();
    assert(POSIX_COOKIE(self) == pthread_self());

    if (self->in_sigsuspend) {
	return;
    }
    if (self->suspended) {
	sigset_t sig_set;

	pthread_sigmask(SIG_SETMASK, NULL, &sig_set);
	sigdelset(&sig_set, SIGUSR1);

	do {
	    /* wait for thread resume or set-owner request */
	    self->in_sigsuspend = 1;
	    sigsuspend(&sig_set);
	    self->in_sigsuspend = 0;
	} while (self->suspended);
    }
}

void
CVMmutexSetOwner(CVMThreadID *self, CVMMutex *m, CVMThreadID *target)
{
    /* This is dependent on the pthreads library implementation.
       A private API would be preferable */
    m->pmtx = target->locked;
    assert(!CVMmutexTryLock(m));
}

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
    struct timespec timeout;

    if (millis == 0LL) {
	waitForever = CVM_TRUE;
    } else {
	struct timeval tv;
	long secs;
	long long tmp;
        gettimeofday(&tv, NULL);

	waitForever = CVM_FALSE;
	tmp = millis / 1000LL;
	secs = (long)tmp;
	if (secs != tmp) {
	    /* overflow */
	    waitForever = CVM_TRUE;
	} else {
	    timeout.tv_sec = tv.tv_sec + secs;
	    timeout.tv_nsec = tv.tv_usec * 1000;
	    timeout.tv_nsec += (millis % 1000) * 1000000;
            if (timeout.tv_nsec >= 1000000000) {
                ++timeout.tv_sec;
                timeout.tv_nsec -= 1000000000;
            }
            assert(timeout.tv_nsec < 1000000000);
            if (timeout.tv_sec < tv.tv_sec) {
                /* overflow */
		waitForever = CVM_TRUE;
            }
	}
    }

    pthread_mutex_lock(&self->lock);

    assert(self->value == 0);

    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	pthread_mutex_unlock(&self->lock);
	return CVM_FALSE;
    }

    self->in_wait = CVM_TRUE;
    self->notified = CVM_FALSE;

    /* add ourself to the wait queue that notify and notifyAll look at */
    enqueue(c, self);

    CVMmutexUnlock(m);

    /* notifies can happen now */

    pthread_mutex_unlock(&self->lock);

    /* thread can be interrupted now */

    /* emulate semaphore wait */
    pthread_mutex_lock(&self->wait_mutex);

    while (self->value == 0) {
	if (waitForever) {
	    pthread_cond_wait(&self->wait_cv, &self->wait_mutex);
	} else {
	    int status = pthread_cond_timedwait(&self->wait_cv,
		&self->wait_mutex, &timeout);
	    if (status == ETIMEDOUT) {
		break;
	    }
	}
    }

    pthread_mutex_unlock(&self->wait_mutex);

    /* Make sure we're not supposed to be still suspended before getting on
       with reacquiring the mutex lock: */
    pthread_mutex_lock(&self->lock);
    while (self->suspended) {
        pthread_cond_wait(&self->suspend_cv, &self->lock);
    }
    self->in_wait = CVM_FALSE;
    pthread_mutex_unlock(&self->lock);

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
	    /*
	     * We were woken up by linuxSyncInterruptWait() posting on
	     * the semaphore, or the timed wait timed out.
	     */
	    assert(self->interrupted || !waitForever);
	    dequeue_me(c, self);
	    /* removed from wait queue */ 
	    assert(self->next == NULL && self->prev_p == NULL);
	}
    }

    /*
     * Workaround for Linux kernel bug exposed by LinuxThreads.
     */
    setFPMode();

    self->value = 0;

    if (self->interrupted) {
	self->interrupted = CVM_FALSE;
	return CVM_FALSE;
    }

    return CVM_TRUE;

}

/* emulate a semaphore post */
#define SEM_POST(t) \
do {						\
    pthread_mutex_lock(&(t)->wait_mutex);	\
    (t)->value = 1;				\
    pthread_cond_signal(&(t)->wait_cv);		\
    pthread_mutex_unlock(&(t)->wait_mutex);	\
} while (0)

CVMBool
CVMcondvarInit(CVMCondVar * c, CVMMutex * m)
{
    c->waiters = NULL;
    c->last_p = &c->waiters;

    return POSIXcondvarInit(&(c)->pcv, &(m)->pmtx);
}

void
CVMcondvarDestroy(CVMCondVar * c)
{
    POSIXcondvarDestroy(&(c)->pcv);
}

void
CVMcondvarNotify(CVMCondVar * c)
{
    CVMThreadID *t;
    if ((t = dequeue(c)) != NULL) {
	t->notified = CVM_TRUE;
	SEM_POST(t);
    }
}

void
CVMcondvarNotifyAll(CVMCondVar * c)
{
    CVMThreadID *t;
    while ((t = dequeue(c)) != NULL) {
	t->notified = CVM_TRUE;
	SEM_POST(t);
    }
}

void
linuxSyncInterruptWait(CVMThreadID *thread)
{
    pthread_mutex_lock(&thread->lock);
    if (thread->in_wait) {
	/* thread->notified is not set */
	/* thread will dequeue itself from wait queue */
	SEM_POST(thread);
    }
    pthread_mutex_unlock(&thread->lock);
}

void
linuxSyncSuspend(CVMThreadID *t)
{
    if (POSIX_COOKIE(t) == thr_self()) {
	t->suspended = 1;
	pthread_kill(POSIX_COOKIE(t), SIGUSR1);
    } else {
	mutex_lock(&t->lock);
	if (!t->suspended) {
	    t->suspended = 1;
	    /*
	     * If the thread is performing a wait, we do special handling,
	     * to avoid the use of signals.
	     */
	    if (t->in_wait) {
		t->suspended_in_wait = 1;
	    } else {
		pthread_kill(POSIX_COOKIE(t), SIGUSR1);
	    }
	}
	mutex_unlock(&t->lock);
    }
}

void
linuxSyncResume(CVMThreadID *t)
{
    pthread_mutex_lock(&t->lock);
    if (t->suspended) {
	t->suspended = 0;
	if (t->suspended_in_wait) {
	    t->suspended_in_wait = 0;
	    pthread_cond_signal(&t->suspend_cv);
	} else {
	    pthread_kill(POSIX_COOKIE(t), SIGUSR1);
	}
    }
    pthread_mutex_unlock(&t->lock);
}

/*
 * Linux doesn't handle MT core dumps very well, so suspend here
 * for the debugger.
 */
void crash(int sig)
{
    int pid = getpid();
    
    fprintf(stderr, "Process #%d received signal %d, suspending\n", pid, sig);
    kill(pid, SIGSTOP);
}

CVMBool
linuxSyncInit(void)
{
    int result = 0;
    struct sigaction sa;
    sa.sa_handler = sigfunc0;	/* mutex set owner and suspend */
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    {
#ifdef CVM_JIT
        int signals[] = {SIGILL, SIGFPE};
#else
	int signals[] = {SIGBUS, SIGILL, SIGFPE, SIGSEGV};
#endif
	int i;

	for (i = 0;
	     result !=-1 && i < (sizeof signals / sizeof signals[0]);
	     ++i)
	{
	    struct sigaction sa;
	    sa.sa_handler = crash;
	    sa.sa_flags = 0;
	    sigemptyset(&sa.sa_mask);
	    result = sigaction(signals[i], &sa, NULL);
	}
    }

    if (result != -1) {
#ifdef CVM_JIT
	return linuxJITSyncInitArch(); /* JIT-specific initialization */
#else
	return CVM_TRUE;
#endif
    } else {
        return CVM_FALSE;
    }
}
