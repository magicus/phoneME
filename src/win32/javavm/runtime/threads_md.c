/*
 * @(#)threads_md.c	1.24 06/10/10
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

#include "javavm/include/porting/float.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/ansi/assert.h"
#include <windows.h>
#include <excpt.h>

void win32SyncSuspend(CVMThreadID *t);
void win32SyncResume(CVMThreadID *t);
void win32SyncInterruptWait(CVMThreadID *thread);

/*
 * Thread local storage index used for looking up CVMThreadID struct
 * (tid) associated with the current thread.
 */
#define TLS_INVALID_INDEX 0xffffffffUL
static unsigned long tls_index = TLS_INVALID_INDEX;
static unsigned long win32_pagesize;

static DWORD
getpc(struct _EXCEPTION_POINTERS *ep, DWORD *addr)
{
    addr = ep->ExceptionRecord->ExceptionAddress;
#ifdef ARM
    return ep->ContextRecord->Pc;
#endif
#ifdef _X86_
    return ep->ContextRecord->Eip;
#endif
}

DWORD WINAPI
start_func( void *a)
{
    DWORD code = 0, pc = 0, addr = 0;
    __try {
        void **arglist = (void **)a;
        void (*func)(void *)= (void *)arglist[0];
        void *arg = arglist[1];

        free(arglist);
        (*func)(arg);
        return 0;
    } __except ((code = _exception_code()),
                (pc = getpc(_exception_info(), &addr)),
                EXCEPTION_EXECUTE_HANDLER) {
        CVMconsolePrintf("Thread died with exception %x at pc %x addr %x\n",
                         code, pc, addr);
        CVMabort();
        DebugBreak();
        return _exception_code();
    }
}

/*
 * Set priority of specified thread.
 */
static CVMBool
threadSetPriority(HANDLE tid, CVMInt32 p)
{
    int priority;

    switch (p) {
    case 0:
	priority = THREAD_PRIORITY_IDLE;
	break;
    case 1: case 2:
	priority = THREAD_PRIORITY_LOWEST;
	break;
    case 3: case 4:
	priority = THREAD_PRIORITY_BELOW_NORMAL;
	break;
    case 5:
	priority = THREAD_PRIORITY_NORMAL;
	break;
    case 6: case 7:
	priority = THREAD_PRIORITY_ABOVE_NORMAL;
	break;
    case 8: case 9:
	priority = THREAD_PRIORITY_HIGHEST;
	break;
    case 10:
	/* Runs to completion, not time-sliced? */
	priority = THREAD_PRIORITY_TIME_CRITICAL;
	break;
    default:
	return FALSE;
    }
    return SetThreadPriority(tid, priority) ;
}

/* 
 * Create a new Java thread.
 */
CVMBool
CVMthreadCreate(CVMThreadID *tid, CVMSize stackSize, CVMInt32 priority,
    void (*func)(void *), void *arg)
{
    HANDLE thrid;
    void **arglist = (void **)malloc(sizeof(void *) * 2);
    arglist[0] = func;
    arglist[1] = arg;
    /*
     * Start the new thread.
     */
    thrid = (HANDLE)CreateThread(NULL, stackSize, start_func, 
			(void *)arglist, CREATE_SUSPENDED, &tid->id);
    if (thrid == 0) {
#ifdef CVM_DEBUG
	{
	    DWORD err = GetLastError();
	    CVMconsolePrintf("CreateThread failed with error %d\n", err);
	}
#endif
	free(arglist);
	return CVM_FALSE;
    }
    threadSetPriority(thrid, priority);
    ResumeThread(thrid);
    tid->handle = thrid;
    return CVM_TRUE;
}

/* 
 * Yield control to another thread.
 */
void
CVMthreadYield(void)
{
    Sleep(0);
}

/* 
 * Suspend execution of the specified thread.
 */
void
CVMthreadSuspend(CVMThreadID *tid)
{
#ifdef CVM_DEBUG
    tid->where = 1;
#endif
    win32SyncSuspend(tid);
}

/* 
 * Continue execution of the specified thread.
 */
void
CVMthreadResume(CVMThreadID *tid)
{
#ifdef CVM_DEBUG
    tid->where = 0;
#endif
    win32SyncResume(tid);
}


void
CVMthreadSetPriority(CVMThreadID *t, CVMInt32 priority)
{
    threadSetPriority(t->handle, priority);
}

CVMThreadID *
CVMthreadSelf()
{
    CVMThreadID* mySelf = (CVMThreadID *)TlsGetValue(tls_index);
    return mySelf;
}

static CVMBool
WIN32computeStackTop(CVMThreadID *self);

CVMBool
CVMthreadAttach(CVMThreadID *self, CVMBool orphan)
{
    WIN32computeStackTop(self);
    if (!CVMthreadStackCheck(self, CVM_THREAD_MIN_C_STACK_SIZE)) {
	return CVM_FALSE;
    }
    if (orphan) {
#ifndef WINCE
	HANDLE hnd = GetCurrentProcess();
	BOOL r = DuplicateHandle(hnd, GetCurrentThread(), hnd,
	    &self->handle, 0, FALSE,
	    DUPLICATE_SAME_ACCESS);
	if (!r) {
	    return CVM_FALSE;
	}
#else
	self->handle = (HANDLE)GetCurrentThreadId();
#endif
	self->id = GetCurrentThreadId();
    } else {
	assert(self->id == GetCurrentThreadId());
    }
    assert(tls_index != TLS_INVALID_INDEX);

    TlsSetValue(tls_index, self);

    if (!CVMmutexInit(&self->lock)) {
	goto fail0;
    }
    self->suspended = FALSE;
    self->interrupt_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (self->interrupt_event == NULL) {
	goto fail1;
    }
    self->value = 0;
    self->wait_cv = CreateEvent(0, FALSE, FALSE, 0);
    if (self->wait_cv == NULL) {
	goto fail2;
    }
    if (!CVMmutexInit(&self->wait_mutex)) {
	goto fail3;
    }
    self->suspend_cv = CreateEvent(0, FALSE, FALSE, 0);
    if (self->suspend_cv == NULL) {
	goto fail4;
    }

#ifdef CVM_ADV_THREAD_BOOST
    self->threadMutex = CreateMutex(NULL, TRUE, NULL);
    if (self->threadMutex == NULL) {
	goto fail5;
    }
    self->boost = (CVMThreadBoostQueue *)calloc(1, sizeof *self->boost);
    if (self->boost == NULL) {
	goto fail6;
    }
    if (!WIN32threadBoostInit(self->boost)) {
	goto fail7;
    }
#endif

    return CVM_TRUE;

#ifdef CVM_ADV_THREAD_BOOST
fail7:
    free(self->boost);
fail6:
    CloseHandle(self->threadMutex);
fail5:
    CloseHandle(self->suspend_cv);
#endif
fail4:
    CVMmutexDestroy(&self->wait_mutex);
fail3:
    CloseHandle(self->wait_cv);
fail2:
    CloseHandle(self->interrupt_event);
fail1:
    CVMmutexDestroy(&self->lock);
fail0:
#ifndef WINCE
    CloseHandle(self->handle);
#endif
    return CVM_FALSE;
}

void
CVMthreadDetach(CVMThreadID *self)
{
    CVMmutexDestroy(&self->lock);
    CloseHandle(self->interrupt_event);
    CVMmutexDestroy(&self->wait_mutex);
    CloseHandle(self->wait_cv);
    CloseHandle(self->suspend_cv);
    TlsSetValue(tls_index, 0);
#ifdef CVM_ADV_THREAD_BOOST
    CloseHandle(self->threadMutex);
    WIN32threadBoostDestroy(self->boost);
    free(self->boost);
#endif
    CloseHandle(self->handle);
}

CVMBool
CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone)
{
    return (char *)self->stackTop + redZone < (char *)&self;
}

static CVMBool
WIN32computeStackTop(CVMThreadID *self)
{
    MEMORY_BASIC_INFORMATION info;
    VirtualQuery((char *)&info, &info, sizeof info);
    self->stackTop = (char *)info.AllocationBase + 0x2000;
{
    MEMORY_BASIC_INFORMATION info1;
    char *start = info.AllocationBase;
    char *end = (char *)info.BaseAddress + info.RegionSize;
    char *start0 = start;
    VirtualQuery(start, &info1, sizeof info1);
    start += win32_pagesize;
    while (start < end) {
	MEMORY_BASIC_INFORMATION info2;
	VirtualQuery(start, &info2, sizeof info2);
	if (!memcmp(&info1, &info2, sizeof info1)) {
	    info1 = info2;
	}
	start += win32_pagesize;
    }
}
    return CVM_TRUE;
}


void
CVMthreadInterruptWait(CVMThreadID *thread)
{
    thread->interrupted = CVM_TRUE;
    win32SyncInterruptWait(thread);
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

/*
 * Private to porting layer implementation
 */

#ifdef CVM_DEBUG
DWORD WINAPI
idle(void *a)
{
    while (1) {
	Sleep(1000);
    }
}
#endif

CVMBool
WIN32threadsInit()
{
    SYSTEM_INFO si;
    tls_index = TlsAlloc();
    GetSystemInfo(&si);
    win32_pagesize = si.dwPageSize;
#ifdef CVM_DEBUG
    {
	DWORD id;
	HANDLE h = CreateThread(NULL, 0, idle, (void *)NULL, 0, &id);
	SetThreadPriority(h, THREAD_PRIORITY_IDLE);
    }
#endif
    return CVM_TRUE;
}

void
WIN32threadsDestroy()
{
    TlsFree(tls_index);
}
