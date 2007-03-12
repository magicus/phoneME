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
#include <assert.h>
#include <JUMPThreadSync.h>

struct WAITING_THREAD {
    DWORD threadId;
    int signaled;
    struct WAITING_THREAD *next;
};

struct _JUMPThreadMutex {
    CRITICAL_SECTION crit;
};

struct _JUMPThreadCond {
    HANDLE event;
    struct WAITING_THREAD *waiting_list;
    struct _JUMPThreadMutex *mutex;
};

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

JUMPThreadMutex jumpThreadMutexCreate() {
    struct _JUMPThreadMutex *m = malloc(sizeof *m);
    HANDLE mutex;
    
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

void jumpThreadMutexDestroy(struct _JUMPThreadMutex *m) {
    
    assert(m != NULL);
    DeleteCriticalSection(&m->crit);
    free(m);
}

int jumpThreadMutexLock(struct _JUMPThreadMutex *m) {

    assert(m != NULL);
    EnterCriticalSection(&m->crit);
    return JUMP_SYNC_OK;
}

int jumpThreadMutexTrylock(struct _JUMPThreadMutex *m) {
    int err = 0;

    assert(m != NULL);
#if _WIN32_WINNT >= 0x0400
    err = TryEnterCriticalSection(&m->crit);
    return err == 0 ? JUMP_SYNC_WOULD_BLOCK : JUMP_SYNC_OK;
#else
    PRINT_ERROR(TryEnterCriticalSection, "Not supported", 0);
    return JUMP_SYNC_FAILURE;
#endif
}

int jumpThreadMutexUnlock(struct _JUMPThreadMutex *m) {
    
    assert(m != NULL);
    LeaveCriticalSection(&m->crit);
    return JUMP_SYNC_OK;
}

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

JUMPThreadMutex jumpThreadCondGetMutex(struct _JUMPThreadCond *c) {
    assert(c != NULL);
    assert(c->mutex != NULL);
    return c->mutex;
}

void jumpThreadCondDestroy(struct _JUMPThreadCond *c) {
    assert(c != NULL);
    if (!CloseHandle(c->event)) {
        REPORT_ERROR(CloseHandle);
    }
    free(c);
}

int jumpThreadCondWait(struct _JUMPThreadCond *c, long millis) {
    int err;
    struct WAITING_THREAD wt;
    struct WAITING_THREAD *pwt, *ppwt;
    DWORD id;

    assert(c != NULL);
    id = GetCurrentThreadId();
    wt.threadId = id;
    wt.next = c->waiting_list;
    c->waiting_list = &wt;
    wt.signaled = 0;
    do {
        jumpThreadMutexUnlock(c->mutex);
        err = WaitForSingleObject(c->event, (millis == 0 ? INFINITE : millis));
        if (!wt.signaled) {
            Sleep(1);
        }
        jumpThreadMutexLock(c->mutex);
    } while (err == WAIT_OBJECT_0 && !wt.signaled);
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

int jumpThreadCondSignal(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    //int cnt = 0;
    
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        if (pwt->next == NULL) {
            pwt->signaled = 1;
        }
        //cnt++;
    }
    //fprintf(stderr, "cnt=%d\n", cnt);
    //fflush(stderr);
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JUMP_SYNC_FAILURE;
    }
    return JUMP_SYNC_OK;
}

int jumpThreadCondBroadcast(struct _JUMPThreadCond *c) {
    struct WAITING_THREAD *pwt;
    //int cnt = 0;
    
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        pwt->signaled = 1;
        //cnt++;
    }
    //fprintf(stderr, "cnt=%d\n", cnt);
    //fflush(stderr);
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JUMP_SYNC_FAILURE;
    }
    return JUMP_SYNC_OK;
}
#ifndef NDEBUG

static char *err2str(int i) {
    char *msg, *pmsg;
    int rez;
    LPTSTR lpMsgBuf;
    
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
    msg = strdup(lpMsgBuf);
    if (msg == NULL) {
        msg = "No memory for error message\n";
    }
    LocalFree(lpMsgBuf);
    pmsg = msg + strlen(msg) - 1;
    while (pmsg > msg && 
            (*pmsg == ' ' || *pmsg == '\n' || *pmsg == '\r')) {
        pmsg--;
    }
    *pmsg = '\0';
    return msg;
}
#endif

