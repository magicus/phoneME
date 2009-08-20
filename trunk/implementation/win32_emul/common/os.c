/*
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#include <assert.h>
#include "javacall_os.h"


/*
 * Initialize the OS structure.
 * This is where timers and threads get started for the first
 * real_time_tick event, and where signal handlers and other I/O
 * initialization should occur.
 *
*/
void javacall_os_initialize(void){
    return;
}


/*
 * Performs a clean-up of all threads and other OS related activity
 * to allow for a clean and complete restart.  This should undo
 * all the work that initialize does.
 */
void javacall_os_dispose(){
    return;
}

/** 
 * javacall_os_flush_icache is used, for example, to flush any caches used by a
 * code segment that is deoptimized or moved during a garbage collection.
 * flush at least [address, address + size] (may flush complete icache).
 *
 * @param address   Start address to flush
 * @param size      Size to flush
 */
void javacall_os_flush_icache(unsigned char* address, int size) {
}

/* An element of the waiting list of threads. */
struct WAITING_THREAD {
    DWORD threadId;
    int signaled;
    struct WAITING_THREAD *next;
    struct WAITING_THREAD *prev;
};

/* Internal structure of a mutex */
struct _javacall_mutex {
    CRITICAL_SECTION crit;
#ifndef NDEBUG
    int locked;
#endif
};

/* Internal structure of a condition variable */
struct _javacall_cond {
    HANDLE event;
    struct WAITING_THREAD *waiting_list;
    int num_signaled;
    struct WAITING_THREAD *last_thread;
    struct _javacall_mutex *mutex;         /* "bound" mutex */
#ifndef NDEBUG
    int signaled;
    long spurious_sum;
    long spurious_cnt;
    long qlen_sum;
    long qlen_cnt;
#endif
};

/* Debug stuff */
#ifndef NDEBUG

#if !defined(__GNUC__) && (!defined(_MSC_VER) || (_MSC_VER < 1300))

#define PRINT_ERROR(func_,text_,code_)    \
    fprintf(stderr, \
        "%s:%s %s: error=%s (#%d)\n", \
        __FILE__, __LINE__, #func_, text_, code_)

#else

#define PRINT_ERROR(func_,text_,code_)    \
    fprintf(stderr, \
        "%s: %s: error=%s (#%d)\n", \
        __FUNCTION__, #func_, text_, code_)

#endif
    
#define REPORT_ERROR(func_)   do {\
    DWORD errCode_ = GetLastError(); \
    PRINT_ERROR(func_,err2str(errCode_),errCode_); \
} while (0)

static char *err2str(int i);

#else
#define PRINT_ERROR(func_,text_,code_)
#define REPORT_ERROR(func_)
#endif
static void remove_from_queue(struct _javacall_cond *c, 
                              struct WAITING_THREAD *wt);

/* creates a mutex. We will use "CriticalSection" as a mutex */
javacall_mutex javacall_os_mutex_create() {
    struct _javacall_mutex *m = malloc(sizeof *m);
    
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
void javacall_os_mutex_destroy(struct _javacall_mutex *m) {
    
    assert(m != NULL);
#ifndef NDEBUG
    assert(!m->locked);
#endif
    DeleteCriticalSection(&m->crit);
    free(m);
}

/* locks the mutex */
javacall_result javacall_os_mutex_lock(struct _javacall_mutex *m) {

    assert(m != NULL);
    EnterCriticalSection(&m->crit);
#ifndef NDEBUG
    assert(!m->locked);
    m->locked = 1;
#endif
    return JAVACALL_OK; /* always OK */
}

/* tries to lock the mutex */
javacall_result javacall_os_mutex_try_lock(struct _javacall_mutex *m) {
    int err = 0;

    assert(m != NULL);
#ifndef NDEBUG
    assert(!m->locked);
#endif
    
    /* old versions of Win32 API don't support "TryEnterCriticalSection" */
#if _WIN32_WINNT >= 0x0400
    err = TryEnterCriticalSection(&m->crit);
    if (err == 0) {
        return JAVACALL_WOULD_BLOCK;
    }
#ifndef NDEBUG
    m->locked = 1;
#endif
    return JAVACALL_OK;
#else
    PRINT_ERROR(TryEnterCriticalSection, "Not supported", 0);
    return JAVACALL_FAIL;
#endif /* _WIN32_WINNT >= 0x0400 */
}

/* unlocks the mutex */
javacall_result javacall_os_mutex_unlock(struct _javacall_mutex *m) {
    
    assert(m != NULL);
#ifndef NDEBUG
    assert(m->locked);
    m->locked = 0;
#endif
    LeaveCriticalSection(&m->crit);
    return JAVACALL_OK; /* always OK */
}

/* creates a condvar. We will use Win32/WinCE Event as a condvar */
javacall_cond javacall_os_cond_create(struct _javacall_mutex *m) {
    struct _javacall_cond *c = malloc(sizeof *c);
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
    c->num_signaled = 0;
#ifndef NDEBUG
    c->signaled = 0;
    c->spurious_sum = 0;
    c->spurious_cnt = 0;
    c->qlen_sum = 0;
    c->qlen_cnt = 0;
#endif
    return c;
}

/* just returns the saved mutex */
javacall_mutex javacall_os_cond_get_mutex(struct _javacall_cond *c) {
    assert(c != NULL);
    assert(c->mutex != NULL);
    return c->mutex;
}

/* destroys the condvar */
void javacall_os_cond_destroy(struct _javacall_cond *c) {
    assert(c != NULL);
    assert(c->waiting_list == NULL);
    assert(c->num_signaled == 0);
    if (!CloseHandle(c->event)) {
        REPORT_ERROR(CloseHandle);
    }
#ifndef NDEBUG
    fprintf(stderr, "Wakeup stats: %ld spurious per %ld proper (%g avg)\n", 
        c->spurious_sum, c->spurious_cnt, 
        (c->spurious_cnt == 0 ? 
            (double)0 : (double)c->spurious_sum / c->spurious_cnt));
    fprintf(stderr, "Average length of queue: %g (%ld/%ld)\n", 
        (c->qlen_cnt == 0 ? 
            (double)0 : (double)c->qlen_sum / c->qlen_cnt),
        c->qlen_sum, c->qlen_cnt);
#endif
    free(c);
}

/* 
 * Waits for condition.
 * Threads that are waiting for the condition has been linked in a waiting list.
 * javacall_os_cond_signal/javacall_os_cond_broadcast set the "signaled" field
 * and raise the Event. Woken thread checks the "signaled" field.
 */
javacall_result javacall_os_cond_wait(struct _javacall_cond *c, long millis) {
    int err;
    /* A waiting list element is allocated in the thread stack */
    struct WAITING_THREAD wt;
    struct WAITING_THREAD *pwt, *ppwt;
    DWORD id;
#ifndef NDEBUG
    long num_spurious = 0;
#endif

    assert(c != NULL);
    assert(c->mutex != NULL);
#ifndef NDEBUG
    assert(c->mutex->locked);
#endif
    
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
        javacall_os_mutex_unlock(c->mutex);
        err = WaitForSingleObject(c->event, (millis == 0 ? INFINITE : millis));
        //if (err != WAIT_TIMEOUT && !wt.signaled) {
        //    Sleep(1);
        //}
        javacall_os_mutex_lock(c->mutex);
#ifndef NDEBUG
        if (err == WAIT_OBJECT_0 && !wt.signaled) {
            num_spurious ++;
        }
#endif
    } while (err == WAIT_OBJECT_0 && !wt.signaled);
    
#ifndef NDEBUG
    c->spurious_sum += num_spurious;
    c->spurious_cnt ++;
#endif
    /* remove from the waiting list if a timeout or an error */
    if (!wt.signaled) {
        remove_from_queue(c, &wt);
    } else {
        c->num_signaled--;
    }
    /* if no signaled thread remain then reset the event */
    if (c->num_signaled == 0) {
#ifndef NDEBUG
        c->signaled = 0;
#endif
        if (!ResetEvent(c->event)) {
            REPORT_ERROR(ResetEvent);
            return JAVACALL_FAIL;
        }
    }
    if (err == WAIT_FAILED) {
        REPORT_ERROR(WaitForSingleObject);
        return JAVACALL_FAIL;
    }
    if (err == WAIT_TIMEOUT) {
        return JAVACALL_TIMEOUT;
    }
    return JAVACALL_OK;
}

static void remove_from_queue(struct _javacall_cond *c, 
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
javacall_result javacall_os_cond_signal(struct _javacall_cond *c) {
    struct WAITING_THREAD *pwt;
    
    assert(c != NULL);
    assert(c->mutex != NULL);    
    
#ifndef NDEBUG
    assert(c->mutex->locked);
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        c->qlen_sum ++;
    }
    c->qlen_cnt ++;
#endif
    /* if no waiting threads */
    if (c->waiting_list == NULL) {
        return JAVACALL_OK;
    }
    /* only last waiting thread gets signaled */
    assert(c->last_thread != NULL);
    assert(!c->last_thread->signaled);
    c->last_thread->signaled = 1;
    c->num_signaled++;
    remove_from_queue(c, c->last_thread);
#ifndef NDEBUG
    c->signaled = 1;
#endif
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/* wakes up all threads that are waiting for the condition */
javacall_result javacall_os_cond_broadcast(struct _javacall_cond *c) {
    struct WAITING_THREAD *pwt;
    
    assert(c != NULL);
    assert(c->mutex != NULL);
    
#ifndef NDEBUG
    assert(c->mutex->locked);
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        c->qlen_sum ++;
    }
    c->qlen_cnt ++;
#endif

    /* if no waiting threads */
    if (c->waiting_list == NULL) {
        return JAVACALL_OK;
    }
    /* all waiting threads become signaled */
    for (pwt = c->waiting_list; pwt != NULL; pwt = pwt->next) {
        assert(!pwt->signaled);
        pwt->signaled = 1;
        c->num_signaled++;
    }
    c->waiting_list = NULL;
    c->last_thread = NULL;
#ifndef NDEBUG
    c->signaled = 1;
#endif
    if (!SetEvent(c->event)) {
        REPORT_ERROR(SetEvent);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
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


#ifdef __cplusplus
}
#endif


