/*
 * @(#)threads_md.c	1.19 06/10/10
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

#include "javavm/include/porting/float.h"	/* for setFPMode() */
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/sync.h"
#include <thread.h>
#include <synch.h>

void
CVMthreadYield(void)
{
    thr_yield();
}

void
CVMthreadSuspend(CVMThreadID *t)
{
    if (SOLARIS_COOKIE(t) == thr_self()) {
	t->suspended = 1;
	/* loop in case we are explicitly resumed to handle a signal */
	do {
	    thr_suspend(SOLARIS_COOKIE(t));
	} while (t->suspended);
    } else {
	mutex_lock(&t->lock);
	t->suspended = 1;
	thr_suspend(SOLARIS_COOKIE(t));
	mutex_unlock(&t->lock);
    }
}

void
CVMthreadResume(CVMThreadID *t)
{
    mutex_lock(&t->lock);
    t->suspended = 0;
    thr_continue(SOLARIS_COOKIE(t));
    mutex_unlock(&t->lock);
}

#include <sys/resource.h>


CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
#ifdef CVM_ADV_MUTEX_SET_OWNER
    if (!POSIXmutexInit(&self->locked)) {
        return CVM_FALSE;
    }
    if (!POSIXmutexTryLock(&self->locked)) {
	POSIXmutexDestroy(&self->locked);
	return CVM_FALSE;
    }
#endif
#ifdef CVM_ADV_THREAD_BOOST
    if (!SOLARISthreadBoostInit(self)) {
	return CVM_FALSE;
    }
#endif
    mutex_init(&self->lock, NULL, NULL);
    {
	stack_t stack;
	CVMUint32 stackSize;
	thr_stksegment(&stack);
	if (thr_main()) {
	   struct rlimit r;
	   getrlimit(RLIMIT_STACK, &r);
	   stackSize = r.rlim_cur;
	} else {
	   stackSize = stack.ss_size;
	}
	self->stackTop = (char *)stack.ss_sp - stackSize;
    }
#ifdef CVM_JVMTI
    self->lwp_id = _lwp_self();
#endif
    setFPMode();
    return POSIXthreadAttach(self, orphan);
}

void
CVMthreadDetach(CVMThreadID *self)
{
    POSIXthreadDetach(self);
    mutex_destroy(&self->lock);
#ifdef CVM_ADV_THREAD_BOOST
    SOLARISthreadBoostDestroy(self);
#endif
#ifdef CVM_ADV_MUTEX_SET_OWNER
    POSIXmutexUnlock(&self->locked);
    POSIXmutexDestroy(&self->locked);
#endif
}

void
CVMthreadSetPriority(CVMThreadID *t, CVMInt32 priority)
{
    POSIXthreadSetPriority(t, priority);
}

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
    return (char *)self->stackTop + redZone < (char *)&self;
}

void
CVMthreadInterruptWait(CVMThreadID *thread)
{
    mutex_lock(&thread->lock);
    if (!thread->interrupted) {
	thread->interrupted = CVM_TRUE;
	if (thread->in_wait) {
	    solarisSyncInterruptWait(thread);
	}
    }
    mutex_unlock(&thread->lock);
}

CVMBool
CVMthreadIsInterrupted(CVMThreadID *thread, CVMBool clearInterrupted)
{
    CVMBool wasInterrupted;
    mutex_lock(&thread->lock);
    wasInterrupted = thread->interrupted;
    if (clearInterrupted) {
	thread->interrupted = CVM_FALSE;
    }
    mutex_unlock(&thread->lock);
    return wasInterrupted;
}
