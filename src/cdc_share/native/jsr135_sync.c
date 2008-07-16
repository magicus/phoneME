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


#include "jsr135_sync.h"
#include "javacall_defs.h"
#include "javavm/include/porting/sync.h"
#include "KNICommon.h"

static CVMMutex        nAudioMutex;
static javacall_bool   nAudioMutexCreated = JAVACALL_FALSE;

void LockAudioMutex() {
    if(!nAudioMutexCreated) {
        CVMmutexInit(&nAudioMutex);
        nAudioMutexCreated = JAVACALL_TRUE;
    }
    CVMmutexLock(&nAudioMutex);
}

void UnlockAudioMutex() {
    CVMmutexUnlock(&nAudioMutex);
}


#define MAX_THREADS 50

typedef struct {
    int desc;
    CVMCondVar cv;
    javacall_result *pStatus;
    void **pData;
} ThreadInfo;

static ThreadInfo ti[MAX_THREADS];
static CVMMutex tMutex;
static init_done = 0;

static void mmapi_threads_init(void) {
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        /*
         * This cannot be a valid descriptor because event part is always
         * non-zero - see MAKE_PLAYER_DESCRIPTOR macro.
         */
        ti[i].desc = 0;
    }
    CVMmutexInit(&tMutex);
    init_done = 1;
}

static int get_thread(int desc) {
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (ti[i].desc == desc) {
            return i;
        }
        if (ti[i].desc == 0) {
            ti[i].desc = desc;
            CVMcondvarInit(&ti[i].cv, &tMutex);
            ti[i].pStatus = NULL;
            ti[i].pData = NULL;
            return i;
        }
    }
    return -1;
}

static int get_next_thread(int desc, int from) {
    int i;
    for (i = from; i < MAX_THREADS; i++) {
        if (ti[i].desc == desc ||
            (desc & PLAYER_DESCRIPTOR_EVENT_MASK) == desc &&
            (ti[i].desc & PLAYER_DESCRIPTOR_EVENT_MASK) == desc &&
            0 != ti[i].desc) {
            return i;
        }
    }
    return -1;
}

javacall_result mmapi_thread_suspend(int desc, javacall_result *pStatus, void **pData) {
    int tnum;

    if (!init_done) {
        mmapi_threads_init();
    }

    CVMmutexLock(&tMutex);
    tnum = get_thread(desc);
    if (-1 == tnum || NULL != ti[tnum].pStatus) {
        CVMmutexUnlock(&tMutex);
        return JAVACALL_FAIL;
    }

    ti[tnum].pStatus = pStatus;
    ti[tnum].pData = pData;
    CVMcondvarWait(&ti[tnum].cv, &tMutex, 0);
    CVMmutexUnlock(&tMutex);
    return JAVACALL_OK;
}

javacall_result mmapi_thread_resume(int desc, javacall_result status, void *data) {
    int tnum = -1;

    if (!init_done) {
        mmapi_threads_init();
        /* No blocked thread with the given parameters exists yet */
        return JAVACALL_FAIL;
    }

    CVMmutexLock(&tMutex);
    while (-1 != (tnum = get_next_thread(desc, tnum + 1))) {
        if (NULL != ti[tnum].pStatus) {
            *ti[tnum].pStatus = status;
            *ti[tnum].pData = data;
            ti[tnum].pStatus = NULL;
            ti[tnum].pData = NULL;
            CVMcondvarNotify(&ti[tnum].cv);
        }
    }
    CVMmutexUnlock(&tMutex);
    return JAVACALL_OK;
}
