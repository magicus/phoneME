/*
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
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* malloc()/free() */
#include <errno.h>  /* symbolic names of errors */
#include <time.h>   /* struct timespec */
#include <pthread.h>
#include <assert.h>
#include "javacall_os.h"

/* Internal structure of a mutex */
struct _javacall_mutex {
    pthread_mutex_t mutex;
};

/* Internal structure of a condition variable */
struct _javacall_cond {
    pthread_cond_t condvar;
    struct _javacall_mutex *mutex;
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
javacall_mutex javacall_os_mutex_create() {
    struct _javacall_mutex *m = malloc(sizeof *m);
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
void javacall_os_mutex_destroy(struct _javacall_mutex *m) {
    int err;

    assert(m != NULL);
    if ((err = pthread_mutex_destroy(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_destroy);
    } else {
        free(m);
    }
}

/* locks the mutex */
javacall_result javacall_os_mutex_lock(struct _javacall_mutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_lock(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_lock);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/* tries to lock the mutex */
javacall_result javacall_os_mutex_try_lock(struct _javacall_mutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_trylock(&m->mutex)) != 0 && err != EBUSY) {
        REPORT_ERROR(pthread_mutex_trylock);
        return JAVACALL_FAIL;
    }
    return err == EBUSY ? JAVACALL_WOULD_BLOCK : JAVACALL_OK;
}

/* unlocks the mutex */
javacall_result javacall_os_mutex_unlock(struct _javacall_mutex *m) {
    int err;
    
    assert(m != NULL);
    if ((err = pthread_mutex_unlock(&m->mutex)) != 0) {
        REPORT_ERROR(pthread_mutex_lock);
    }
    return err;
}

/* creates a POSIX condvar */
javacall_cond javacall_os_cond_create(struct _javacall_mutex *m) {
    struct _javacall_cond *c = malloc(sizeof *c);
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
javacall_mutex javacall_os_cond_get_mutex(struct _javacall_cond *c) {
    return c->mutex;
}

/* destroys the condvar */
void javacall_os_cond_destroy(struct _javacall_cond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_destroy(&c->condvar)) != 0) {
        REPORT_ERROR(pthread_cond_destroy);
    } else {
        free(c);
    }
}

/* waits for condition. */
javacall_result javacall_os_cond_wait(struct _javacall_cond *c, long millis) {
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
            return JAVACALL_FAIL;
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
        return JAVACALL_TIMEOUT;
    }
    if (err != 0) {
        REPORT_ERROR(pthread_cond_XXXwait);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
#undef nano_denom
#undef micro_denom
#undef milli_denom
}

/* wakes up a thread that is waiting for the condition */
javacall_result javacall_os_cond_signal(struct _javacall_cond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_signal(&c->condvar)) != 0) {
        REPORT_ERROR(pthread_cond_signal);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/* wakes up all threads that are waiting for the condition */
javacall_result javacall_os_cond_broadcast(struct _javacall_cond *c) {
    int err;
    
    assert(c != NULL);
    if ((err = pthread_cond_broadcast(&c->condvar)) != 0) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
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
