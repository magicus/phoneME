/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#include <assert.h>
#include <porting/JUMPTypes.h>
#include <porting/JUMPThreadSync.h>

/* An element of the waiting list of threads. */
struct WAITING_THREAD {
    DWORD threadId;
    int signaled;
    struct WAITING_THREAD *next;
    struct WAITING_THREAD *prev;
};

/* Internal structure of a mutex */
struct _JUMPThreadMutex {
    CRITICAL_SECTION crit;
#ifndef NDEBUG
    int locked;
#endif
};

/* Internal structure of a condition variable */
struct _JUMPThreadCond {
    HANDLE event;
    struct WAITING_THREAD *waiting_list;
    int num_waiting;
    struct WAITING_THREAD *last_thread;
    struct _JUMPThreadMutex *mutex;         /* "bound" mutex */
#ifndef NDEBUG
    int signaled;
#endif
};

/* Debug stuff */
#ifndef NDEBUG
#define PRINT_ERROR(func_,text_,code_)    \
    fprintf(stderr, \
        "%s: %s: error=%s (#%d)\n", \
        __FUNCTION__, #func_, text_, code_)
    
#define REPORT_ERROR(func_)   do {\
    DWORD errCode_ = GetLastError(); \
    PRINT_ERROR(func_,err2str(errCode_),errCode_); \
} while (0)

static void remove_from_queue(struct _JUMPThreadCond *c, 
                              struct WAITING_THREAD *wt);
static char *err2str(int i);

#else
#define PRINT_ERROR(func_,text_,code_)
#define REPORT_ERROR(func_)
#endif

/* creates a mutex. We will use "CriticalSection" as a mutex */
JUMPThreadMutex jumpThreadMutexCreate() {
    struct _JUMPThreadMutex *m = malloc(sizeof *m);
    
    if (m == NULL) {
        PRINT_ERROR(malloc, "No memory", 0);
        return NULL;
    }
    __try {
        InitializeCriticalSection(&m->crit);
#ifndef NDEBUG
        m->locked = 0;
#endif
        return m;
    } __except (_exception_code() == STATUS_NO_MEMORY) {
        assert(m != NULL);
        free(m);
        return NULL;
    }
}

/* destroys the mutex */
void jumpThreadMutexDestroy(struct _JUMPThreadMutex *m) {
    
    assert(m != NULL);
    assert(!m->locked);
    DeleteCriticalSection(&m->crit);
    free(m);
}

/* locks the mutex */
int jumpThreadMutexLock(struct _JUMPThreadMutex *m) {

    assert(m != NULL);
    EnterCriticalSection(&m->crit);
    assert(!m->locked);
#ifndef NDEBUG
    m->locked = 1;
#endif
    return JUMP_SYNC_OK; /* always OK */
}

/* tries to lock the mutex */
int jumpThreadMutexTrylock(struct _JUMPThreadMutex *m) {
    int err = 0;

    assert(m != NULL);
    assert(!m->locked);
    
    /* old versions of Win32 API don't support "TryEnterCriticalSection" */
#if _WIN32_WINNT >= 0x0400
    err = TryEnterCriticalSection(&m->crit);
    if (err == 0) {
        return JUMP_SYNC_WOULD_BLOCK;
    }
#ifndef NDEBUG
    m->locked = 1;
#endif
    return JUMP_SYNC_OK;
#else
    PRINT_ERROR(TryEnterCriticalSection, "Not supported", 0);
    return JUMP_SYNC_FAILURE;
#endif /* _WIN32_WINNT >= 0x0400 */
}

/* unlocks the mutex */
int jumpThreadMutexUnlock(struct _JUMPThreadMutex *m) {
    
    assert(m != NULL);
    assert(m->locked);
#ifndef NDEBUG
    m->locked = 0;
#endif
    LeaveCriticalSection(&m->crit);
    return JUMP_SYNC_OK; /* always OK */
}

/* creates a condvar. We will use Win32/WinCE Event as a condvar */
JUMPThreadCond jumpThreadCondCreate(struct _JUMPThreadMutex *m) {
    struct _JUMPThreadCond *c = malloc(sizeof *c);
    HANDLE event;
    
    assert(m != NULL);
    if (c == NULL) {
        PRINT_ERROR(malloc, "No memory", 0);
        return NULL;
    }
    event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (event == NULL) {
        REPORT_ERROR(CreateEvent);
        free(c);
        return NULL;
    }
    c->event = event;
    c->mutex = m;
    c->waiting_list = NULL;
    c->last_thread = NULL;
    c->num_waiting = 0;
#ifndef NDEBUG
    c->signaled = 0;
#endif
    return c;
}

/* just returns the saved mutex */
JUMPThreadMutex jumpThreadCondGetMutex(struct _JUMPThreadCond *c) {
    assert(c != NULL);
    assert(c->mutex != NULL);
    return c->mutex;
}

/* destroys the condvar */
void jumpThreadCondDestroy(struct _JUMPThreadCond *c) {
    assert(c != NULL);
    assert(c->waiting_list == NULL);
    if (!CloseHandle(c->event)) {
        REPORT_ERROR(CloseHandle);
    }
    free(c);
}

/* 
 * Waits for condition.
 * Threads that are waiting for the condition has been linked in a waiting list.
 * jumpThreadCondSignal/jumpThreadCondBroadcast set the "signaled" field
 * and raise the Event. Woken thread checks the "signaled" field.
 */
int jumpThreadCondWait(struct _JUMPThreadCond *c, long millis) {
    int err;
    /* A waiting list element is allocated in the thread stack */
    struct WAITING_THREAD wt;
    struct WAITING_THREAD *pwt, *ppwt;
    DWORD id;

    assert(c != NULL);
    assert(c->mutex != NULL);
    assert(c->mutex->locked);
    
    wt.signaled = 0;
    /* add to the waiting list */
    wt.next = c->waiting_list;
    wt.prev = NULL;
    if (c->waiting_list != NULL) {
        assert(c->waiting_list->prev == NULL);
        c->waiting_list->prev = &wt;
    }
    c->waiting_list = &wt;
    if (c->last_thread == NULL) {
        c->last_thread = &wt;
    }
    do {
        c->num_waiting++;
        jumpThreadMutexUnlock(c->mutex);
        err = WaitForSingleObject(c->event, (millis == 0 ? INFINITE : millis));
        //if (err != WAIT_TIMEOUT && !wt.signaled) {
        //    Sleep(1);
        //}
        jumpThreadMutexLock(c->mutex);
        c->num_waiting--;
    } while (err == WAIT_OBJECT_0 && !wt.signaled);
    
    /* remove from the waiting list if a timeout or an error */
    if (!wt.signaled) {
        remove_from_queue(c, &wt);
    }
    /* if no signaled thread remain then reset the event */
    if (c->num_waiting == 0) {
#ifndef NDEBUG
        c->signaled = 0;
#endif
        if (!ResetEvent(c->event)) {
            REPORT_ERROR(ResetEvent);
            return JUMP_SYNC_FAILURE;
        }
    }
    if (err == WAIT_FAILED) {
        REPORT_ERROR(WaitForSingleObject);
        return JUMP_SYNC_FAILURE;
    }
    if (err == WAIT_TIMEOUT) {
        return JUMP_SYNC_TIMEOUT;
    }
    return JUMP_SYNC_OK;
}

static void remove_from_queue(struct _JUMPThreadCond *c, 
                              struct WAITING_THREAD *wt) {
    assert(c->waiting_list != NULL);
    if (c->waiting_list == wt) {
        c->waiting_list = wt->next;
    } else {
        if (wt->prev != NULL) {
            wt->prev->next = wt->next;
        }
    }
    if (wt->next != NULL) {
        wt->next->prev = wt->prev;
    }
    if (c->last_thread == wt) {
        c->last_thread = wt->prev;
    }
    
}


/* wakes up a thread that is waiting for the condition */
int jumpThreadCondSignal(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    
    assert(c != NULL);
    assert(c->mutex != NULL);
    assert(c->mutex->locked);
    
    /* if no waiting threads */
    if (c->waiting_list == NULL) {
        return JUMP_SYNC_OK;
    }
    /* only last waiting thread gets signaled */
    assert(c->last_thread != NULL);
    assert(!c->last_thread->signaled);
    c->last_thread->signaled = 1;
    remove_from_queue(c, c->last_thread);
#ifndef NDEBUG
    c->signaled = 1;
#endif
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JUMP_SYNC_FAILURE;
    }
    return JUMP_SYNC_OK;
}

/* wakes up all threads that are waiting for the condition */
int jumpThreadCondBroadcast(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    
    assert(c != NULL);
    assert(c->mutex != NULL);
    assert(c->mutex->locked);
    
    /* if no waiting threads */
    if (c->waiting_list == NULL) {
        return JUMP_SYNC_OK;
    }
    /* all waiting threads become signaled */
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        assert(!pwt->signaled);
        pwt->signaled = 1;
    }
    c->waiting_list = NULL;
    c->last_thread = NULL;
#ifndef NDEBUG
    c->signaled = 1;
#endif
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JUMP_SYNC_FAILURE;
    }
    return JUMP_SYNC_OK;
}
#ifndef NDEBUG

/* gets error's description */
static char *err2str(int i) {
    char *msg;
    int rez;
    LPTSTR lpMsgBuf;
    int len;
    
    rez = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        i,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    if (rez == 0) {
        return "Can't retrieve error message";
    }
#ifdef UNICODE
    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpMsgBuf, -1, 
                              NULL, 0, NULL, NULL);
    if (len > 0 && (msg = malloc(len)) != NULL) {
        len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpMsgBuf, -1, 
                                  msg, len, NULL, NULL);
    }
    if (len <= 0) {
        LocalFree(lpMsgBuf);
        return "Can't convert error message into readable string";
    }
#else
    msg = strdup((char*)lpMsgBuf);
#endif
    LocalFree(lpMsgBuf);
    if (msg == NULL) {
        return "No memory for error message";
    }
    for (len = strlen(msg) - 1; len >= 0; len--) {
        if (msg[len] != '\n' && msg[len] != '\r' && msg[len] != ' ') {
            msg[len + 1] = '\0';
            break;
        }
    }
    return msg;
}
#endif

