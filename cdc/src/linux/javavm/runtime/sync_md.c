/*
 * @(#)sync_md.c	1.32 06/10/10
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
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef CVM_JVMPI
#include "javavm/include/globals.h"
#endif
#ifdef CVM_JVMTI
#include "javavm/include/globals.h"
#include "javavm/include/jvmtiExport.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif

/*
 * signal handler for CVMthreadSuspend() and CVMthreadResume() request.
 */
/* NOTE: handleSuspendResume() is needed regardless of whether we enable
   thread suspension or not.  This is because the IO system uses SIGUSR1 to
   break a blocked thread out of its wait if the corresponding file descriptor
   is closed.  All the signal handler has to do in those cases is return.
   In the event that thread suspension is enabled, a SIGUSR1 from an IO
   function will cause the handler to be called, but since the handler always
   check the current suspension state self->isSuspended, receiving these
   SIGUSR1s won't change the behavior of suspension code.  This is as we
   expect it to be.
*/
static void handleSuspendResume(int sig)
{
#ifdef CVM_THREAD_SUSPENSION
    CVMThreadID *self = CVMthreadSelf();
    assert(POSIX_COOKIE(self) == pthread_self());

    /* NOTE: Normally, the SIGUSR1 signal would be masked out while we're in
       this handler thereby preventing this handler function from being called
       recursively.  However, the sigsuspend() call below is designed to
       suspend this thread while re-enabling SIGUSR1 so that we can wake up
       if someone sends us that signal again (usually to resume the thread).
       Hence, it is possible to enter this handler recursively when we're
       waiting for sigsuspend() to return.  The isInSuspendHandler flag is
       used to ensure that we don't do anything when the SIGUSR1 signal
       triggers again.  We simply return.

       When SIGUSR1 is received again, sigsuspend() will return to its caller,
       thereby allowing the first invocation of this handler function to
       continue running again.  Because there can be racing suspend and resume
       requests, we need to always check the isSuspended flag before
       returning.  If the isSuspended flag is set, then we need to call
       sigsuspend() again to suspend the thread.  The suspend/resume semantics
       is that the last most request will be honored.
    */
    if (self->isInSuspendHandler) {
        /* The handler is already on the stack.  Just return and let the first
           invocation of the handler deal with any changes to the suspension
           state which may need to be done. */
        return;
    }
    if (self->isMutexBlocked || self->isWaitBlocked) {
        /* We're currently in the process of acquiring a mutex or is waiting
           on a condvar.  There's nothing to do but return.  When we're done
           acquiring the mutex or waiting on the condvar, the thread will
           suspend itself automatically by raising this signal. */
        return;
    }
    self->isInSuspendHandler = CVM_TRUE;
    if (self->isSuspended) {
	sigset_t sig_set;

	pthread_sigmask(SIG_SETMASK, NULL, &sig_set);
	sigdelset(&sig_set, SIGUSR1);

        /* We'll keep suspending the thread as long as the its isSuspended
           flag indicates that we're supposed to be suspended. */
	do {
	    /* wait for thread resume or set-owner request */
            /* Mask out other signals but allow SIGUSR1 to trigger.
               sigsuspend() will wait until we receive SIGUSR1 again. See NOTE
               above for more details. */
	    sigsuspend(&sig_set);
	} while (self->isSuspended);
    }
    self->isInSuspendHandler = CVM_FALSE;
#endif /* CVM_THREAD_SUSPENSION */
}

#ifdef CVM_THREAD_SUSPENSION
/* Purpose: Suspend the self thread. */
static void suspendSelf(void)
{
    sigset_t sig_set;
    /* Create the signal set for SIGUSR1: */
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGUSR1);

    /* NOTE: Prevent SIGUSR1 from triggering the signal handler while we're
       calling it.  We need to block SIGUSR1 while we're calling the signal
       handler explicitly and cannot simply rely on the isInSuspendHandler
       flag instead because there can be a race condition that can result in
       resume failing.

       If we don't block the signal before calling handleSuspendResume(), then
       we could be in a suspended state (i.e. self->isSuspended is true) and
       just get past the check for self->isSuspended in handleSuspendResume()
       when a SIGUSR1 signal fires to resume the thread.  The signal causes
       handleSuspendResume() to be called recursedly, but it does nothing
       because self->isSuspended is now false.  The signal handler returns to
       mainline code which is still executing in handleSuspendResume() which
       proceeds to call sigsuspend() even though the thread should now be in
       a resumed state.  This causes the thread to block even though it should
       not be.

       If we block the signal before calling handleSuspendResume(), then the
       signal would not trigger the recursion into handleSuspendResume().
       Instead, the signal will be pending until SIGUSR1 gets unblocked by
       the call to sigsuspend() in handleSuspendResume().  At which point,
       handleSuspendResume() will be recursed into, and does nothing as
       expected because self->isSuspended is now false.  But upon returning
       from the signal handler, in mainline code, we'll also return from the
       call to sigsuspend() and the self->isSuspended flag will be checked
       once more and found to be false causing handleSuspendResume() to
       return.  Hence, the thread won't be blocking when it should have been
       resumed as expected.
    */
    pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
    /* Call the signal handler to suspend self if necessary: */
    handleSuspendResume(SIGUSR1);
    /* Allow SIGUSR1 to trigger the signal handler again: */
    pthread_sigmask(SIG_UNBLOCK, &sig_set, NULL);
}

/* Purpose: Locks the mutex.  This version is only needed to work around
   issues that pthread library semantics that poses a problem for mutex
   locking in the face of thread suspension.  Without thread suspension,
   we can call the POSIX mutex lock directly. */
void CVMmutexLock(CVMMutex *m)
{
    CVMThreadID * self = CVMthreadSelf();

    /* The CVMThreadID may be NULL if the current thread hasn't been attached
       to a Java thread yet.  If that is the case, we don't have to worry
       about suspension, and just call through to the POSIX mutex lock.  This
       is because we will need a non-NULL CVMThreadID in order to call the
       suspension API. */
    if (self != NULL) {
        /* NOTE: We're about to attempt to acquire the mutex.  This can cause
           this thread to block on that mutex.  This is OK, except that we
           need to let the suspend handler know so that this thread doesn't
           actually get suspended while it is in that state.  Usually, the
           suspension requester thread would first acquire the mutex to make
           sure that the target thread doesn't get suspended while holding
           that mutex.  But if the target thread is blocked on the mutex and
           gets suspended, when the owner of the mutex releases it, ownership
           will inadvertantly be assigned to this thread even though it is
           suspended (according to pthread library semantics).  So the
           following code is put in place to ensure that the mutex is not
           acquired while the thread is supposed to be suspended.
        */
        while(1) {
            self->isMutexBlocked = CVM_TRUE;
            POSIXmutexLock(&(m)->pmtx);
            self->isMutexBlocked = CVM_FALSE;
            if (self->isSuspended) {
                /* Oops, this thread has been given ownership of the mutex
                   while it's supposed to be suspended.  Release the mutex
                   and suspend the thread.  We can acquire the mutex again
                   later. */
                POSIXmutexUnlock(&(m)->pmtx);
                suspendSelf();
            } else {
                break;
            }
        }
    } else {
        POSIXmutexLock(&(m)->pmtx);
    }
}
#endif /* CVM_THREAD_SUSPENSION */

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

    /* Determine the duration to wait: */
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
    self->value = 0;

    /* add ourself to the wait queue that notify and notifyAll look at */
    enqueue(c, self);

    CVMmutexUnlock(m);

    /* notifies can happen now */

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
    if (thread->isWaitBlocked) {
	/* thread->notified is not set */
	/* thread will dequeue itself from wait queue */
	SEM_POST(thread);
    }
}

#ifdef CVM_THREAD_SUSPENSION
void
linuxSyncSuspend(CVMThreadID *t)
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
    /* Tell the target thread to service the suspension: */
    pthread_kill(POSIX_COOKIE(t), SIGUSR1);
}

void
linuxSyncResume(CVMThreadID *t)
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
    /* Tell the target thread to service the suspension: */
    pthread_kill(POSIX_COOKIE(t), SIGUSR1);
}
#endif /* CVM_THREAD_SUSPENSION */


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

CVMBool
linuxSyncInit(void)
{
    int result = 0;

    /* NOTE: handleSuspendResume() is needed regardless of whether we enable
       thread suspension or not.  See notes on handleSuspendResume() above for
       details.
    */
    {
	/* Signal initialization. The signals we recover from set
	   the SA_RESTART flag. */
	struct {
	    int sig;
	    void *handler;
	    int flags;
	} const signals[] = {
#if !defined(CVM_JIT) && !defined(CVM_USE_MEM_MGR)
	    {SIGSEGV, crash, 0},
#endif
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
            {SIGQUIT, sigquitHandler, SA_RESTART},
#endif
	    {SIGUSR1, handleSuspendResume, SA_RESTART},
	    {SIGBUS, crash, 0},
	    {SIGILL, crash, 0},
	    {SIGFPE, crash, 0}
	};

	int i;
	int numsignals = sizeof signals / sizeof signals[0];

	for (i = 0; i < numsignals; ++i) {
	    struct sigaction sa;
	    sa.sa_handler = (void (*)(int))signals[i].handler;
	    sa.sa_flags = signals[i].flags;
	    sigemptyset(&sa.sa_mask);
	    if ((result = sigaction(signals[i].sig, &sa, NULL)) == -1)
		break;
	}
	
    }

    return (result != -1);
}
