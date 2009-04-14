/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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


#include "limethread.h"

#ifdef WIN32

#include <windows.h>

#else  /* !WIN32 */ 

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define DWORD int

#endif

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define MALLOC(s) malloc(s)
#define FREE(p) free(p)

static const int debug = 0;

static int thread_start(LimeThread *t);
static int thread_getError(LimeThread *t);
static void thread_clearError(LimeThread *t);
static void thread_setError(LimeThread *t, int errorCode);
static void thread_checkError(LimeThread *t);
static void error(const char *format, ... );

#ifdef WIN32
typedef struct InternalThreadData {
    HANDLE handle;
    DWORD threadID;
    LimeRunnable f;
    int errorCondition;
    void *parameter;
} InternalThreadData;

#else /* !WIN32 */ 

typedef struct InternalThreadData {
    pthread_t threadID;
    LimeRunnable f;
    int errorCondition;
    void *parameter;
} InternalThreadData;


#endif

static void error(const char *s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    va_end(args);
}

static int getID(LimeThread *t) {
    return (int) t->data->threadID;
}

static void thread_clearError(LimeThread *t) {
    t->data->errorCondition = 0;
}

static void thread_checkError(LimeThread *t) {
    int state = t->data->errorCondition;
    if (state) {
        error("LimeThread: Error condition %x on thread %x\n",
              state, getID(t));
        exit(1);
    }
}

static int thread_getError(LimeThread *t) {
    return t->data->errorCondition;
}

static void thread_setError(LimeThread *t, int state) {
    t->data->errorCondition = state;
}

LimeThread *NewLimeThread(LimeRunnable f, void *parameter) {
    LimeThread *t = (LimeThread *) MALLOC( sizeof(LimeThread) );
    InternalThreadData *d = (InternalThreadData *)
        MALLOC( sizeof(InternalThreadData) );
    if (!t || !d) {
        if (t) FREE(t);
        error("LimeThread: Cannot allocate memory for new thread object\n");
    }
    if (debug) {
        printf("LimeThread: NewLimeThread at %p\n", t);
    }
    memset(d, 0, sizeof(d));
    t->data = d;
    t->data->f = f;
    t->data->parameter = parameter;
    t->data->errorCondition = 0;
    t->start = thread_start;
    t->getError = thread_getError;
    t->clearError = thread_clearError;
    return t;
}

void DeleteLimeThread(LimeThread *t) {
    if (debug) {
        printf("LimeThread: DeleteLimeThread at %p, thread=%i\n",
               t, getID(t));
    }
    FREE(t->data);
    FREE(t);
}

#ifdef WIN32

static DWORD WINAPI thread_run(LPVOID lpParam) {

#else /* !WIN32 */ 

static void *thread_run(void* lpParam){

#endif

    InternalThreadData *d = (InternalThreadData *) lpParam;
    LimeRunnable f;
    void *parameter;
    assert(d != NULL);
    assert(d->f != NULL);
    f = d->f;
    parameter = d->parameter;
    FREE(d);
    f(parameter);
    return 0;
}


static int thread_start(LimeThread *t) {
#ifdef WIN32    

    DWORD ThreadParam = 1;
    HANDLE h;

#else

    pthread_t h; 
    int res; 

#endif

    InternalThreadData *copiedData;

    assert(t != NULL);
    if (debug) {
        printf("LimeThread: 1\n");
    }
    thread_checkError(t);

    if (debug) {
        printf("LimeThread: Starting thread at %p\n", t);
    }



    /* Copy the thread data */
    copiedData = (InternalThreadData *) MALLOC( sizeof(InternalThreadData) );
    memcpy(copiedData, t->data, sizeof(InternalThreadData));

#ifdef WIN32

    if (t->data->handle != NULL) {
     
#else /* !WIN32 */ 
     
     if (t->data->threadID > 0) {

#endif
        error("LimeThread: Cannot start a thread that is already running"
              " with ID %i\n", (int) t->data->threadID);
        return 1;
    }

#ifdef WIN32

    h = CreateThread(NULL, 0, thread_run, copiedData, 0, &t->data->threadID);
    if (h == NULL) { 
#else /* WIN32 */ 

    res = pthread_create(&h, NULL, thread_run , copiedData);
    if (res != 0) {

#endif

        error("LimeThread: Cannot create thread\n");
        thread_setError(t, 1);
        return 1;
    } else {
#ifdef WIN32
        t->data->handle = 0;
#endif
        return 0;
    }
}

int LimeThreadRun(LimeRunnable f, void *parameter) {
    LimeThread *t = NewLimeThread(f, parameter);
    if (t == NULL) {
        return 1;
    }

    if (t->start(t)) {
        return 1;
    }

    DeleteLimeThread(t);

    return 0;
}
