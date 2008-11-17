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

#define VXWORKS_PRIVATE
#include "javavm/include/porting/threads.h"
#include "javavm/include/utils.h"
#include <vxWorks.h>
#include <taskLib.h>
#include <taskVarLib.h>
#include <fppLib.h>
#include <stdio.h>
#include <assert.h>


/* %comment: mam052 */
#define JAVAPRIO2VXPRIO(p)	(100 - (p))
#define VXPRIO2JAVAPRIO(p)	(100 - (p))

static CVMThreadID *mySelf = NULL;

static void
fpuInit()
{
    FP_CONTEXT fpContext;
    fppSave(&fpContext);
    /* disable floating-point traps */
#if defined(__i386__)
    /* set the fp control word precision to 53-bit for both single and double precision */
    fpContext.fpcr = 0xffff027f; 
    fppRestore(&fpContext);
#elif defined(__sparc__)
    fpContext.fsr &= ~0x0f800000;
    fppRestore(&fpContext);
    fppSave(&fpContext);
#endif
#if 1
    {
	int options = -1;
	int status = taskOptionsGet(0, &options);
	assert(status == OK);
	assert(options & VX_FP_TASK);
    }
#endif
}

static int
start_func(void (*func)(void *), void *arg)
{
    (*func)(arg);
    return 0;
}

int javaThreadStackSize = 0x10000;

CVMBool
CVMthreadCreate(CVMThreadID *tid, CVMSize stackSize, CVMInt32 priority,
    void (*func)(void *), void *arg)
{
    int task_options = VX_FP_TASK;
    int taskID;
    int vxPrio;

    vxPrio = JAVAPRIO2VXPRIO(priority);
    if (stackSize == 0) {
	stackSize = javaThreadStackSize;
    }
    taskID = taskSpawn(NULL, vxPrio, task_options, stackSize,
                            (FUNCPTR)start_func, (int)func,
                            (int)arg,0,0,0,0,0,0,0,0);

    if (taskID == ERROR) {
#ifdef CVM_DEBUG
	perror("taskSpawn");
#endif
	return CVM_FALSE;
    } else {
	tid->taskID = taskID;
	return CVM_TRUE;
    }
}

void
CVMthreadYield(void)
{
    taskDelay(0);
}

void
CVMthreadSuspend(CVMThreadID *t)
{
    taskSuspend(t->taskID);
}

void
CVMthreadResume(CVMThreadID *t)
{
    taskResume(t->taskID);
}

void
CVMthreadSetPriority(CVMThreadID *t, CVMInt32 priority)
{
    taskPrioritySet(t->taskID, JAVAPRIO2VXPRIO(priority));
}

CVMThreadID *
CVMthreadSelf()
{
    return mySelf;
}

CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    if (orphan) {
	self->taskID = taskIdSelf();
    } else {
	assert(self->taskID == taskIdSelf());
    }

    fpuInit();
    if (!vxworksSyncInit(self)) {
	CVMdebugPrintf(("WARNING: CVMthreadAttach failed.\n"));
	return CVM_FALSE;
    }

    /* NOTE: taskVarAdd() creates a unique copy of the mySelf pointer for each
       task so that we can have a unique thread ID for each task: */
    taskVarAdd(0, (int *)&mySelf);
    mySelf = self;

    {
        TASK_DESC desc;
        taskInfoGet(0, &desc);
        self->stackTop = desc.td_pStackLimit;
    }

    return CVM_TRUE;
}

void
CVMthreadDetach(CVMThreadID *self)
{
    mySelf = NULL;
    vxworksSyncDestroy(self);
}

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
    return (char *)self->stackTop + redZone < (char *)&self;
}


void
CVMthreadInterruptWait(CVMThreadID *thread)
{
    taskLock();
    thread->interrupted = CVM_TRUE;
    if (thread->in_wait) {
        vxworksSyncInterruptWait(thread);
    }
    taskUnlock();
}

CVMBool
CVMthreadIsInterrupted(CVMThreadID *thread, CVMBool clearInterrupted)
{
    CVMBool wasInterrupted;
    taskLock();
    wasInterrupted = thread->interrupted;
    if (clearInterrupted) {
        thread->interrupted = CVM_FALSE;
    }
    taskUnlock();
    return wasInterrupted;
}

/*
 * Private to porting layer implementation
 */

void
threadInitStaticState()
{
}

void
threadDestroyStaticState()
{
}
