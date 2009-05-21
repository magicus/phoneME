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
#include "javacall_memory.h"
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


/**
 * @brief Describes a thread waiting for a native event.
 */
typedef struct _ThreadInfo {
    int desc;
    AsyncEventType type;
    CVMCondVar cv;
    javacall_result *pStatus;
    void **pData;
    int waiting;
    struct _ThreadInfo *prev;
    struct _ThreadInfo *next;
} ThreadInfo;

static ThreadInfo *head = NULL;
static CVMMutex tMutex;
static init_done = 0;

/**
 * Adds a thread to the list, associating it with the given descriptor.
 *
 * @param desc player descriptor.
 * @param type type of event: MMAPI event or AMMS event
 * @return pointer to the thread with the same descriptor if it already exists
 *     in the list, or to a newly created thread.
 */
static ThreadInfo * new_thread(int desc, AsyncEventType type) {
    ThreadInfo *ti = head;
    ThreadInfo *prev;

    for (ti = head; NULL != ti; ti = ti->next) {
        prev = ti;
        if (ti->desc == desc) {
            return ti;
        }
    }
    ti = (ThreadInfo *)javacall_malloc(sizeof(ThreadInfo));
    if (NULL == ti) {
        return NULL;
    }
    ti->desc = desc;
    ti->type = type;
    CVMcondvarInit(&ti->cv, &tMutex);
    ti->pStatus = NULL;
    ti->pData = NULL;
    ti->next = NULL;
    if (NULL == head) {
        head = ti;
        ti->prev = NULL;
    } else {
        prev->next = ti;
        ti->prev = prev;
    }

    return ti;
}

/**
 * Looks for the next thread matching the descriptor (exact match if event part
 * is non-zero, otherwise the event part is ignored).
 *
 * @param desc player descriptor.
 * @param type type of event: MMAPI event or AMMS event
 * @param from pointer to the thread preceding the one to start the search from.
 *     If NULL, the search will be started from head, otherwise the search is
 *     started from the next thread in the list.
 * @return pointer to the thread if found, NULL otherwise.
 */
static ThreadInfo * match_next_thread(int desc, AsyncEventType type, ThreadInfo *prev) {
    ThreadInfo *ti;

    for (ti = (NULL != prev) ? prev->next : head; NULL != ti; ti = ti->next) {
        if (ti->type == type &&
            (ti->desc == desc ||
            (ti->desc & PLAYER_DESCRIPTOR_EVENT_MASK) == desc)) {
            return ti;
        }
    }
    return NULL;
}

/**
 * Suspends the current thread and associates it with the given player
 * descriptor.
 *
 * @param desc player descriptor.
 * @param type type of event: MMAPI event or AMMS event
 * @param pStatus pointer to store the operation completion status upon resume
 *            of the thread.
 * @param pData pointer to store the event data upon resume of the thread.
 * @return JAVACALL_OK if the thread was suspended and then resumed,
 *         JAVACALL_FAIL if a thread for this descriptor is already suspended,
 *         JAVACALL_OUT_OF_MEMORY if the thread description cannot be allocated
 */
javacall_result mmapi_thread_suspend(int desc, AsyncEventType type, javacall_result *pStatus, void **pData) {
    ThreadInfo *ti;

    if (!init_done) {
        CVMmutexInit(&tMutex);
        init_done = 1;
    }

    CVMmutexLock(&tMutex);
    ti = new_thread(desc, type);
    if (NULL == ti) {
        CVMmutexUnlock(&tMutex);
        return JAVACALL_OUT_OF_MEMORY;
    }
    if (NULL != ti->pStatus) {
        CVMmutexUnlock(&tMutex);
        return JAVACALL_FAIL;
    }

    ti->pStatus = pStatus;
    ti->pData = pData;
    ti->waiting = 1;
    while (ti->waiting) {
        CVMcondvarWait(&ti->cv, &tMutex, 0);
    }
    CVMcondvarDestroy(&ti->cv);
    if (NULL != ti->prev) {
        ti->prev->next = ti->next;
    } else {
        head = ti->next;
    }
    if (NULL != ti->next) {
        ti->next->prev = ti->prev;
    }
    javacall_free(ti);
    CVMmutexUnlock(&tMutex);
    return JAVACALL_OK;
}

/**
 * Resumes threads that were previously suspended and match the descriptor.
 *
 * @param desc player descriptor to match. If the event part is equal to zero,
 *            all threads with the current appId and playerId are resumed.
 * @param type type of event: MMAPI event or AMMS event
 * @param status operation completion status that will be passed to each resumed
 *            thread.
 * @param data arbitrary event data that will be passed to each resumed thread.
 * @return JAVACALL_OK if any thread was resumed,
 *         JAAVCALL_FAIL otherwise
 */
javacall_result mmapi_thread_resume(int desc, AsyncEventType type, javacall_result status, void *data) {
    ThreadInfo *ti = NULL;
    javacall_result res = JAVACALL_FAIL;

    if (!init_done) {
        /* No blocked thread with the given parameters exists yet */
        return JAVACALL_FAIL;
    }

    CVMmutexLock(&tMutex);
    /*
     * The search of the next matching thread is protected by the mutex, so
     * ThreadInfo data being checked is still valid.
     */
    while (NULL != (ti = match_next_thread(desc, type, ti))) {
        if (NULL != ti->pStatus) {
            *ti->pStatus = status;
            *ti->pData = data;
            ti->waiting = 0;
            CVMcondvarNotify(&ti->cv);
            res = JAVACALL_OK;
        }
    }
    CVMmutexUnlock(&tMutex);
    return res;
}
