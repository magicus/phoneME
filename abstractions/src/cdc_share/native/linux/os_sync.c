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
#include <string.h>
#include <stdlib.h> /* malloc()/free() */
#include <errno.h>  /* symbolic names of errors */
#include <time.h>   /* struct timespec */
#include <pthread.h>
#include <assert.h>
#include <thread_sync.h>

/* Internal structure of a mutex */
struct _ThreadMutex {
    pthread_mutex_t mutex;
};

/* Internal structure of a condition variable */
struct _ThreadCond {
    pthread_cond_t condvar;
    struct _ThreadMutex *mutex;
};

/* Debug stuff */
#ifndef NDEBUG
#define PRINT_ERROR(func_,text_,code_)    \
    fprintf(stderr, \
        "%s: %s: error=%s (#%d)\n", \
        __FUNCTION__, #func_, text_, code_)
    
#define REPORT_ERROR(func_)   do {\
    PRINT_ERROR(func_,err2str(err),err); \
} while (0)

static char *err2str(int i);

#else
#define PRINT_ERROR(func_,text_,code_)
#define REPORT_ERROR(func_)
#endif

/* creates a POSIX mutex */
ThreadMutex threadMutexCreate() {
    struct _ThreadMutex *m = malloc(sizeof *m);
    int err;
    
    if (m == NULL) {
        PRINT_ERROR(malloc, "No memory", 0);
        return NULL;
    }
    if ((err = pthread_mutex_init(&m->mutex, NULL)) != 0) {
        REPORT_ERROR(pthread_mutex_init);
        free(m);
        return NULL;
    }
    return m;
}

/* destroys the mutex */
void threadMutexDestroy(struct _ThreadMutex *m) {
    int err;

    assert(m != NULL);
    if ((err = pthread_mutex_destroy(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_destroy);
    } else {
        free(m);
    }
}

/* locks the mutex */
int threadMutexLock(struct _ThreadMutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_lock(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_lock);
        return THREAD_SYNC_FAILURE;
    }
    return THREAD_SYNC_OK;
}

/* tries to lock the mutex */
int threadMutexTrylock(struct _ThreadMutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_trylock(&m->mutex)) != 0 && err != EBUSY) {
        REPORT_ERROR(pthread_mutex_trylock);
        return THREAD_SYNC_FAILURE;
    }
    return err == EBUSY ? THREAD_SYNC_WOULD_BLOCK : THREAD_SYNC_OK;
}

/* unlocks the mutex */
int threadMutexUnlock(struct _ThreadMutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_unlock(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_lock);
    }
    return err;
}

/* creates a POSIX condvar */
ThreadCond threadCondCreate(struct _ThreadMutex *m) {
    struct _ThreadCond *c = malloc(sizeof *c);
    int err;
    
    if (c == NULL) {
        PRINT_ERROR(malloc, "No memory", 0);
        return NULL;
    }
    assert(m != NULL);
    if ((err = pthread_cond_init(&c->condvar, NULL)) != 0) {
        REPORT_ERROR(pthread_cond_init);
        free(c);
        return NULL;
    }
    c->mutex = m;
    return c;
}

/* just returns the saved mutex */
ThreadMutex threadCondGetMutex(struct _ThreadCond *c) {
    return c->mutex;
}

/* destroys the condvar */
void threadCondDestroy(struct _ThreadCond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_destroy(&c->condvar)) != 0) {
        REPORT_ERROR(pthread_cond_destroy);
    } else {
        free(c);
    }
}

/* waits for condition. */
int threadCondWait(struct _ThreadCond *c, long millis) {
    int err;
/* denominators */
#define milli_denom   ((long long)1000)
#define micro_denom   (milli_denom * milli_denom)
#define nano_denom    (milli_denom * milli_denom * milli_denom)
    
    assert(c != NULL);
    if (millis == 0) {
        err = pthread_cond_wait(&c->condvar, &c->mutex->mutex);
    } else {
        struct timespec ts;
        
        /* 
         * pthread_cond_timedwait() receives the absolute time, so
         * we should get current time and add our millis
         */
        err = clock_gettime(CLOCK_REALTIME, &ts);
        if (err != 0) {
            REPORT_ERROR(clock_gettime);
            return THREAD_SYNC_FAILURE;
        }
        assert(ts.tv_sec > 0);
        /* calculate the time of deadline */
        ts.tv_sec += millis / milli_denom;
        ts.tv_nsec += (millis % milli_denom) * nano_denom / milli_denom;
        if (ts.tv_nsec > nano_denom) {
            ts.tv_sec += (time_t) ts.tv_nsec / nano_denom;
            ts.tv_nsec %= nano_denom;
        }
        err = pthread_cond_timedwait(&c->condvar, &c->mutex->mutex, &ts);
    }
    if (err == ETIMEDOUT) {
        return THREAD_SYNC_TIMEOUT;
    }
    if (err != 0) {
        REPORT_ERROR(pthread_cond_XXXwait);
        return THREAD_SYNC_FAILURE;
    }
    return THREAD_SYNC_OK;
#undef nano_denom
#undef micro_denom
#undef milli_denom
}

/* wakes up a thread that is waiting for the condition */
int threadCondSignal(struct _ThreadCond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_signal(&c->condvar)) != 0) {
        REPORT_ERROR(pthread_cond_signal);
        return THREAD_SYNC_FAILURE;
    }
    return THREAD_SYNC_OK;
}

/* wakes up all threads that are waiting for the condition */
int threadCondBroadcast(struct _ThreadCond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_broadcast(&c->condvar)) != 0) {
        return THREAD_SYNC_FAILURE;
    }
    return THREAD_SYNC_OK;
}

#ifndef NDEBUG

/* gets error's description */
#define CODE2STR(code_) \
    case code_:\
        return #code_; 
        
static char *err2str(int i) {
    switch (i) {
        CODE2STR(EBUSY)
        CODE2STR(EINVAL)
        CODE2STR(EAGAIN)
        CODE2STR(EDEADLK)
        CODE2STR(EPERM)
        // CODE2STR(EOWNERDEAD)
        // CODE2STR(ENOTRECOVERABLE)
        CODE2STR(ENOMEM)
        CODE2STR(ETIMEDOUT)
        CODE2STR(EINTR)
    default:
        return "unknown";
    }
}

#endif
