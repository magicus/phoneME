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
};

/* Internal structure of a mutex */
struct _JUMPThreadMutex {
    CRITICAL_SECTION crit;
};

/* Internal structure of a condition variable */
struct _JUMPThreadCond {
    HANDLE event;
    struct WAITING_THREAD *waiting_list;
    struct _JUMPThreadMutex *mutex;         /* "bound" mutex */
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
    DeleteCriticalSection(&m->crit);
    free(m);
}

/* locks the mutex */
int jumpThreadMutexLock(struct _JUMPThreadMutex *m) {

    assert(m != NULL);
    EnterCriticalSection(&m->crit);
    return JUMP_SYNC_OK; /* always OK */
}

/* tries to lock the mutex */
int jumpThreadMutexTrylock(struct _JUMPThreadMutex *m) {
    int err = 0;

    assert(m != NULL);
    /* old versions of Win32 API don't support "TryEnterCriticalSection" */
#if _WIN32_WINNT >= 0x0400
    err = TryEnterCriticalSection(&m->crit);
    return err == 0 ? JUMP_SYNC_WOULD_BLOCK : JUMP_SYNC_OK;
#else
    PRINT_ERROR(TryEnterCriticalSection, "Not supported", 0);
    return JUMP_SYNC_FAILURE;
#endif
}

/* unlocks the mutex */
int jumpThreadMutexUnlock(struct _JUMPThreadMutex *m) {
    
    assert(m != NULL);
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
    id = GetCurrentThreadId();
    wt.threadId = id;
    
    /* add to the waiting list */
    wt.next = c->waiting_list;
    c->waiting_list = &wt;
    wt.signaled = 0;
    do {
        jumpThreadMutexUnlock(c->mutex);
        err = WaitForSingleObject(c->event, (millis == 0 ? INFINITE : millis));
        if (err != WAIT_TIMEOUT && !wt.signaled) {
            Sleep(1);
        }
        jumpThreadMutexLock(c->mutex);
    } while (err == WAIT_OBJECT_0 && !wt.signaled);
    
    /* remove from the waiting list */
    assert(c->waiting_list != NULL);
    if (c->waiting_list->threadId == id) {
        assert(c->waiting_list == &wt);
        c->waiting_list = c->waiting_list->next;
    } else {
        for (ppwt = c->waiting_list, pwt = c->waiting_list->next; 
                    pwt != NULL; ppwt = pwt, pwt = pwt->next) {
            if (pwt->threadId == id) {
                assert(pwt == &wt);
                ppwt->next = pwt->next;
                break;
            }
        }
    }
    
    /* if no signaled thread remain then reset the event */
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        if (pwt->signaled != 0) {
            break;
        }
    }
    if (pwt == NULL) {
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

/* wakes up a thread that is waiting for the condition */
int jumpThreadCondSignal(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    
    /* only last waiting thread gets signaled */
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        if (pwt->next == NULL) {
            pwt->signaled = 1;
        }
    }
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JUMP_SYNC_FAILURE;
    }
    return JUMP_SYNC_OK;
}

/* wakes up a thread that is waiting for the condition */
int jumpThreadCondBroadcast(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    
    /* all waiting threads become signaled */
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        pwt->signaled = 1;
    }
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

