/*
 * @(#)posix_threads_md.c	1.29 06/10/10
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

#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "portlibs/posix/threads.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

struct args {
    void (*func)(void *);
    void *arg;
};

static void *
start_func(void *a)
{
    struct args *args = (struct args *)a;
    void (*func)(void *) = args->func;
    void *arg = args->arg;
    free(a);
    (*func)(arg);
    return 0;
}

CVMBool
POSIXthreadCreate(CVMThreadID *tid, CVMSize stackSize, CVMInt32 priority,
    void (*func)(void *), void *arg)
{
    struct args *args;
    pthread_t pthreadID;
    struct sched_param param;
    pthread_attr_t attr;
    int result;

    args = (struct args *)malloc(sizeof *args);
    if (args == NULL) {
	return CVM_FALSE;
    }
    args->func = func;
    args->arg = arg;
    pthread_attr_init(&attr);
    if (stackSize != 0) {
	errno = pthread_attr_setstacksize(&attr, stackSize);
	if (errno != 0) {
	    perror("pthread_attr_setstacksize");
	}
    }
    pthread_attr_getschedparam(&attr, &param);
    /* %comment d001 */
    param.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &param);

    /* Don't leave thread/stack around after exit for join */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    result = pthread_create(&pthreadID, &attr, start_func, args);
    pthread_attr_destroy(&attr);

    if (result != 0) {
	errno = result;
	perror("pthread_create");
	free(args);
	return CVM_FALSE;
    } else {
	POSIX_COOKIE(tid) = pthreadID;
	return CVM_TRUE;
    }
}

static pthread_key_t key;
static CVMBool validKey = CVM_FALSE;

CVMBool
POSIXthreadInitStaticState()
{
    validKey = (pthread_key_create(&key, NULL) == 0);
    return validKey;
}

void
POSIXthreadDestroyStaticState()
{
    if (validKey) {
	pthread_key_delete(key);
    }
}

static CVMBool
POSIXthreadSetSelf(const CVMThreadID *self)
{
    return (pthread_setspecific(key, self) == 0);
}

CVMThreadID *
POSIXthreadGetSelf()
{
    return (CVMThreadID *)pthread_getspecific(key);
}

CVMBool
POSIXthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    if (orphan) {
	POSIX_COOKIE(self) = pthread_self();
    } else {
	assert(POSIX_COOKIE(self) == pthread_self());
    }
    return POSIXthreadSetSelf(self);
}

void
POSIXthreadDetach(CVMThreadID *self)
{
    POSIXthreadSetSelf(NULL);
}

void
POSIXthreadSetPriority(CVMThreadID *t, CVMInt32 priority)
{
    struct sched_param param;
    int policy;
    pthread_getschedparam(POSIX_COOKIE(t), &policy, &param);
    param.sched_priority = priority;
    pthread_setschedparam(POSIX_COOKIE(t), policy, &param);
}

void
POSIXthreadGetPriority(CVMThreadID *t, CVMInt32 *priority)
{
    struct sched_param param;
    int policy;
    pthread_getschedparam(POSIX_COOKIE(t), &policy, &param);
    *priority = param.sched_priority;
}
