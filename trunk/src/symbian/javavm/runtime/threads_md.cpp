/*
 * @(#)threads_md.cpp	1.15 06/10/10
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
#include "javavm/include/porting/threads.h"
}

#include <e32std.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <E32SVR.H>

/* MT-safe malloc w/ global heap */
#include "javavm/include/porting/ansi/stdlib.h"

static void		SYMBIANthreadSetSelf(CVMThreadID *self);
static CVMThreadID *	SYMBIANthreadGetSelf();

EXPORT_C void		SYMBIANDLLthreadSetSelf(CVMThreadID *self);
EXPORT_C CVMThreadID *	SYMBIANDLLthreadGetSelf();

#ifndef CVM_HPI_DLL_IMPL

typedef struct {
    void (*f)(void *);
    void *arg;
} args;

static TTrapHandler *trap;

static int
start_func(void *arg)
{
    args *a = (args *)arg;
    if (trap != NULL) {
        TTrapHandler *oTrap = User::SetTrapHandler(trap);
    }
    a->f(a->arg);
    free(a);
    return 0;
}

#include "threads_impl.h"

typedef CVMThreadImpl info;
static RCriticalSection mtx;
static unsigned int id_hi = 0;
static unsigned int id_lo = 0;

#if 1
static int
disruptor(void *arg)
{
    while (1) {
	User::After(TTimeIntervalMicroSeconds32(500*1000));
    }
    return 0;
}
#endif

int
SYMBIANthreadsInit()
{
    trap = User::TrapHandler();
    /*if (trap == NULL) {
        fprintf(stderr, "No trap handler!\n");
	return 0;
    }*/
    if (mtx.CreateLocal() != KErrNone) {
	return 0;
    }
#if 1
    // Should not be necessary if time-slicing is working...
    RThread child;
    _LIT(name, "disruptor");
    TInt err = child.Create(name, (TThreadFunction)disruptor,
	CVM_THREAD_MIN_C_STACK_SIZE,
	KMinHeapSize, KMinHeapSize,
	(TAny *)NULL, EOwnerProcess);
    if (err != KErrNone) {
	mtx.Close();
	return 0;
    }
    child.SetPriority(EPriorityMore);
    child.Resume();
#endif
    return 1;
}

void
SYMBIANthreadsDestroy()
{
    mtx.Close();
}

RTimer &
SYMBIANthreadGetTimer(CVMThreadID *self)
{
    info *ti = (info *)self->info;
    return ti->waitTimer;
}

CVMBool
CVMthreadCreate(CVMThreadID *thread,
    CVMSize stackSize, CVMInt32 priority,
    void (*func)(void *), void *arg)
{
    _LIT(jstr, "Java");
    TBuf<32> name(jstr);
    RThread child;
    args *a;
    info *i;

    mtx.Wait();
    int tid_lo = ++id_lo;
    int tid_hi = (id_lo == 0) ? ++id_hi : id_hi;
    mtx.Signal();

    TBuf<8> tstr_lo;
    TBuf<8> tstr_hi;
    tstr_lo.NumFixedWidth(tid_lo, EHex, 8);
    tstr_hi.NumFixedWidth(tid_hi, EHex, 8);
    name.Append(tstr_hi);
    name.Append(tstr_lo);

    a = (args *)malloc(sizeof *a);
    if (a == NULL) {
	return CVM_FALSE;
    }
    if (stackSize < CVM_THREAD_MIN_C_STACK_SIZE) {
	stackSize = CVM_THREAD_MIN_C_STACK_SIZE;
    }
    a->f = func;
    a->arg = arg;
#if 0
    /* NOTE: Don't use shared heap without locking!!! */
    if (child.Create(name, (TThreadFunction)start_func, stackSize,
	NULL, /* user parent's heap */
	(TAny *)a, EOwnerProcess) == KErrNone)
#endif
#if 1
    TInt err = child.Create(name, (TThreadFunction)start_func, stackSize,
	KMinHeapSize, KMinHeapSize,
	(TAny *)a, EOwnerProcess);
    if (err == KErrNone)
#endif
    {
#if 0
	child.SetPriority(abs2symbian(priority));
#endif
	/* NOTE: Should allocate "a" on child heap */
	child.Resume();
	return CVM_TRUE;
    }
    free(a);
#if 1
fprintf(stderr, "RThread.Created failed (%x)\n", err);
#endif
    return CVM_FALSE;
}

void
CVMthreadYield(void)
{
#if 1
    // Should not be necessary if time-slicing is working...
    User::After(TTimeIntervalMicroSeconds32(1));
#endif
}

void
CVMthreadSetPriority(CVMThreadID *thread, CVMInt32 prio)
{
}

void
CVMthreadSuspend(CVMThreadID *thread)
{
}

void
CVMthreadResume(CVMThreadID *thread)
{
}

CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    if (self->info == NULL) {
	info *i = new info; // default per-thread heap
	if (i == NULL) {
	    return CVM_FALSE;
	}
	i->id = RThread().Id();
	if (i->waitTimer.CreateLocal() != KErrNone) {
	    delete i;
	    return CVM_FALSE;
	}
	self->info = i;
    }
    SYMBIANthreadSetSelf(self);
    return CVM_TRUE;
}

void
CVMthreadDetach(CVMThreadID *self)
{
    info *ti = (info *)self->info;
    ti->waitTimer.Close();
    delete ti;	// default per-thread heap
#ifdef CVM_DEBUG
    self->info = NULL;
#endif
    SYMBIANthreadSetSelf(NULL);
}

CVMThreadID *
CVMthreadSelf(void)
{
    return SYMBIANthreadGetSelf();
}

void
CVMthreadInterruptWait(CVMThreadID *thread)
{
    thread->interrupted = CVM_TRUE;
    SYMBIANsyncInterruptWait(thread);
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

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
    return CVM_TRUE;
}

static void
SYMBIANthreadSetSelf(CVMThreadID *self)
{
    SYMBIANDLLthreadSetSelf(self);
}

static CVMThreadID *
SYMBIANthreadGetSelf()
{
    return SYMBIANDLLthreadGetSelf();
}
#endif /* !CVM_HPI_DLL_IMPL */


EXPORT_C void
SYMBIANDLLthreadSetSelf(CVMThreadID *self)
{
#ifdef CVM_DLL
    Dll::SetTls((TAny *)self);
#else
    UserSvr::DllSetTls(67338721, (TAny *)self); 
#endif
}

EXPORT_C CVMThreadID *
SYMBIANDLLthreadGetSelf()
{
#ifdef CVM_DLL
    return (CVMThreadID *)Dll::Tls();
#else
    return (CVMThreadID *)UserSvr::DllTls(67338721);;
#endif
}

#ifdef CVM_DLL
GLDEF_C TInt
E32Dll(TDllReason r)
{
    return KErrNone;
}
#endif

