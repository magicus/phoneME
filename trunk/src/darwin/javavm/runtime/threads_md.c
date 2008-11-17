/*
 * @(#)threads_md.c	1.23 06/10/10
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
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>

void
CVMthreadYield(void)
{
    thr_yield();
}

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

#include <sys/resource.h>
#include <unistd.h>

static void
LINUXcomputeStackTop(CVMThreadID *self)
{
    size_t stackSize = pthread_get_stacksize_np(POSIX_COOKIE(self));
    char*  stackAddr = pthread_get_stackaddr_np(POSIX_COOKIE(self));
    self->stackTop = stackAddr - stackSize;
}

#ifdef CVM_JIT_PROFILE
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>

extern CVMBool __profiling_enabled;
extern struct itimerval prof_timer;

#endif

CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    if (pthread_mutex_init(&self->lock, NULL) != 0) {
	goto bad5;
    }
    if (pthread_mutex_init(&self->wait_mutex, NULL) != 0) {
	goto bad4;
    }
    if (pthread_cond_init(&self->wait_cv, NULL) != 0) {
	goto bad3;
    }
    if (pthread_cond_init(&self->suspend_cv, NULL) != 0) {
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
    LINUXcomputeStackTop(self);
    return CVM_TRUE;

bad0:
    POSIXmutexUnlock(&self->locked);
bad1:
    pthread_cond_destroy(&self->suspend_cv);
bad2:
    pthread_cond_destroy(&self->wait_cv);
bad3:
    pthread_mutex_destroy(&self->wait_mutex);
bad4:
    pthread_mutex_destroy(&self->lock);
bad5:
    return CVM_FALSE;
}

void
CVMthreadDetach(CVMThreadID *self)
{
    POSIXthreadDetach(self);
    pthread_cond_destroy(&self->suspend_cv);
    pthread_cond_destroy(&self->wait_cv);
    pthread_mutex_destroy(&self->wait_mutex);
    pthread_mutex_destroy(&self->lock);
    POSIXmutexDestroy(&self->locked);
}

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
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
