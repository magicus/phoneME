/*
 * @(#)threads_md.c	1.33 06/10/10
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

#include "javavm/include/porting/float.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/globals.h"
#include "javavm/include/assert.h"
#include "javavm/include/utils.h"
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

void
CVMthreadYield(void)
{
    thr_yield();
}

#ifdef CVM_THREAD_SUSPENSION
void
CVMthreadSuspend(CVMThreadID *t)
{
    linuxSyncSuspend(t);
}

void
CVMthreadResume(CVMThreadID *t)
{
    linuxSyncResume(t);
}
#endif /* CVM_THREAD_SUSPENSION */

#include <sys/resource.h>
#include <unistd.h>
#include <dlfcn.h>

/* Some static variables of the initial thread stack. These
 * values are computed by looking at the /proc/<id>/maps of
 * the thread. */
static void *initial_stack_top;
#ifdef LINUX_WATCH_STACK_GROWTH
static void *initial_stack_bottom;
#endif
static pthread_t initial_thread_id;

/* pthread_getattr_np() is a new API provided by glibc (2.2.3
 * and newer versions). It can be used to get the thread 
 * attributes, which then can be used to get the thread stack
 * size information, etc. Some of the older linux kernel
 * does not support this API. In that case, we cannot compute
 * the thread stack top accurately.
 */
typedef int (*pthread_getattr_np_t) (pthread_t, pthread_attr_t *);
static pthread_getattr_np_t pthreadGetAttr = 0;

typedef int (*pthread_attr_getstack_t) (pthread_attr_t *, void **, size_t *);
static pthread_attr_getstack_t pthreadAttrGetStack = 0;

void
linuxCaptureInitialStack()
{
    void *dummy = NULL;
    struct rlimit rlim;
    void *sp = &dummy;

#ifdef LINUX_DLSYM_BUG
    void * handle = dlopen("libpthread.so.0", RTLD_LAZY);
    if (handle != NULL) {
        pthreadGetAttr = 
            (pthread_getattr_np_t)dlsym(handle, "pthread_getattr_np");
        pthreadAttrGetStack =
            (pthread_attr_getstack_t)dlsym(handle, 
                                           "pthread_attr_getstack");
    }
#else
    pthreadGetAttr = 
            (pthread_getattr_np_t)dlsym(RTLD_DEFAULT, "pthread_getattr_np");
    pthreadAttrGetStack =
            (pthread_attr_getstack_t)dlsym(RTLD_DEFAULT, 
                                           "pthread_attr_getstack");
#endif
    initial_thread_id = pthread_self();

    getrlimit(RLIMIT_STACK, &rlim);
    
    /* Need to check the rlimit value. Bug 4970832. */
    if (rlim.rlim_cur > (CVMUint32)(2 * 1024 * 1024) ||
        rlim.rlim_cur == RLIM_INFINITY) {
        rlim.rlim_cur = 2 * 1024 * 1024;
    }

    /* 3K is a rough estimate: from sp to the stack bottom. */
    initial_stack_top = (CVMUint8*)sp + (3 * 1024) - rlim.rlim_cur;
#ifdef LINUX_WATCH_STACK_GROWTH
    initial_stack_bottom = sp;
#endif
}

static CVMBool
LINUXcomputeStackTop(CVMThreadID *self)
{
    void *sp = &self;
    pthread_t myself = pthread_self();
    if (myself == initial_thread_id) {
        self->stackTop = initial_stack_top;
#ifdef LINUX_WATCH_STACK_GROWTH
	self->stackBottom = initial_stack_bottom;
	self->stackLimit = self->stackBottom;
#endif
        return CVM_TRUE;
    } else if (pthreadGetAttr != NULL) {
        pthread_attr_t attr;
        int result;
        void *base;
        size_t size;
 
        result = (*pthreadGetAttr)(myself, &attr);
	if (result != 0) {
	    return CVM_FALSE;
	}
        
        /* 'base' is the lowest addressable byte on the stack */
        /*pthread_attr_getstack(&attr, &base, &size);*/
        CVMassert(pthreadAttrGetStack != 0);
        (*pthreadAttrGetStack)(&attr, &base, &size);
        self->stackTop = base;
#ifdef LINUX_WATCH_STACK_GROWTH
	self->stackBottom = sp;
	self->stackLimit = self->stackBottom;
#endif
	pthread_attr_destroy(&attr);
        return CVM_TRUE;
    } else {
        struct rlimit rlim;
        unsigned int size;

        /* There is no reliable way to compute the thread 
         * stack top without the new pthread APIs.
         * So just give a rough estimate. It's better than '0'.
         */
        getrlimit(RLIMIT_STACK, &rlim);
        size = (CVMglobals.config.nativeStackSize != 0) ?
	       CVMglobals.config.nativeStackSize : rlim.rlim_cur;
        self->stackTop = (CVMUint8*)sp + (3 * 1024) - size;
#ifdef LINUX_WATCH_STACK_GROWTH
	self->stackBottom = sp;
	self->stackLimit = self->stackBottom;
#endif
        return CVM_TRUE;
    }
}

#if defined(CVM_JIT_PROFILE) || defined(CVM_GPROF)
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>

extern CVMBool __profiling_enabled;
extern struct itimerval prof_timer;

#endif

CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    if (!LINUXcomputeStackTop(self)) {
	return CVM_FALSE;
    }
    if (pthread_mutex_init(&self->wait_mutex, NULL) != 0) {
        goto bad3;
    }
    if (pthread_cond_init(&self->wait_cv, NULL) != 0) {
        goto bad2;
    }
    if (!POSIXmutexInit(&self->locked)) {
	goto bad1;
    }
    POSIXmutexLock(&self->locked);

    setFPMode();
    if (!POSIXthreadAttach(self, orphan)) {
	goto bad0;
    }
#if defined(CVM_JIT_PROFILE) || defined(CVM_GPROF)
#if !defined(CVM_GPROF)
    if (__profiling_enabled)
#endif
    {
        /* Need to enable the profiling timer for the new thread.
         * This is to workaround the known problem that profiler
         * (gprof) only profiles the main thread.
         */
	prof_timer.it_value = prof_timer.it_interval;
	setitimer(ITIMER_PROF, &prof_timer, NULL);
    }
#endif
    return CVM_TRUE;

bad0:
    POSIXmutexUnlock(&self->locked);
bad1:
    pthread_cond_destroy(&self->wait_cv);
bad2:
    pthread_mutex_destroy(&self->wait_mutex);
bad3:
    return CVM_FALSE;
}

void
CVMthreadDetach(CVMThreadID *self)
{
    POSIXthreadDetach(self);
    pthread_cond_destroy(&self->wait_cv);
    pthread_mutex_destroy(&self->wait_mutex);
    POSIXmutexDestroy(&self->locked);
}

#ifdef LINUX_WATCH_STACK_GROWTH
static pthread_mutex_t stk_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t max_stack = 0;
#endif

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
#ifdef LINUX_WATCH_STACK_GROWTH
    if ((void *)&self < self->stackLimit) {
	size_t size = (char *)self->stackBottom - (char *)&self;
	size_t dsize = (char *)self->stackLimit - (char *)&self;
	size_t m;
	pthread_mutex_lock(&stk_mutex);
	m = max_stack += dsize;
	pthread_mutex_unlock(&stk_mutex);
	fprintf(stderr, "New stack size %dKB reached for thread %ld "
	    "(%dKB all threads)\n",
	    size / 1024,
	    self->pthreadCookie,
	    m / 1024);
	self->stackLimit = (char *)&self;
    }
#endif
    return (char *)self->stackTop + redZone < (char *)&self;
}

void
CVMthreadInterruptWait(CVMThreadID *thread)
{
    thread->interrupted = CVM_TRUE;
    linuxSyncInterruptWait(thread);
}

CVMBool
CVMthreadIsInterrupted(CVMThreadID *thread, CVMBool clearInterrupted)
{
    if (clearInterrupted) {
	CVMBool wasInterrupted;
	assert(thread == CVMthreadSelf());
	wasInterrupted = thread->interrupted;
	thread->interrupted = CVM_FALSE;
	return wasInterrupted;
    } else {
	return thread->interrupted;
    }
}
