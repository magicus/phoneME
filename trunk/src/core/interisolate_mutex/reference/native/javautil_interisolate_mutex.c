/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <jvmconfig.h>
#include <kni.h>
#include <midpMalloc.h>
#include <midpError.h>
#include <midp_thread.h>
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midp_logging.h>

#include "javautil_interisolate_mutex.h"

/** Mutexes count. Used for assigning mutexes ID */
static jint gsMutexCount = 0;

/** Head of the list */
static InterIsolateMutexList* gsMutexListHead = NULL;


/**
 * Finds mutex data in list by ID.
 *
 * @param mutexID mutex ID
 * @return mutex data that has specified ID, NULL if not found
 */
InterIsolateMutexList* javautil_find_mutex_by_id(int mutexID) {
    InterIsolateMutexList* mutex = gsMutexListHead;
    for (; mutex != NULL; mutex = mutex->next) {
        if (mutex->mutexID == mutexID) {
            break;
        }
    }

    return mutex;
}

/**
 * Finds mutex data in list by name.
 *
 * @param mutexName mutex name
 * @return mutex data that has specified name, NULL if not found
 */
InterIsolateMutexList* javautil_find_mutex_by_name(
        const pcsl_string* mutexName) {

    InterIsolateMutexList* mutex = gsMutexListHead;
    for (; mutex != NULL; mutex = mutex->next) {
        if (pcsl_string_equals(&(mutex->mutexName), mutexName)) {
            break;
        }
    }

    return mutex;
}

/**
 * Creates mutex data associated with specified name and puts it into list.
 *
 * @param mutexName mutex name
 * @return pointer to created data, NULL if OOM 
 */
InterIsolateMutexList* javautil_create_mutex(
        const pcsl_string* mutexName) {

    InterIsolateMutexList* mutex = (InterIsolateMutexList*)midpMalloc(
            sizeof(InterIsolateMutexList));
    if (mutex == NULL) {
        return NULL;
    }

    mutex->locked = 0;
    mutex->mutexID = gsMutexCount++;
    mutex->refCount = 0;

    pcsl_string_dup(mutexName, &mutex->mutexName);
    if (pcsl_string_is_null(&(mutex->mutexName))) {
        midpFree(mutex);
        return NULL;
    }

    mutex->next = gsMutexListHead;
    gsMutexListHead = mutex;

    return mutex;
}

/**
 * Deletes mutex data and removes it from the list.
 *
 * @param mutex pointer to mutex data to delete
 */
void javautil_delete_mutex(InterIsolateMutexList* mutex) {
    InterIsolateMutexList* prevMutex;

    /* If it's first node, re-assign head pointer */
    if (mutex == gsMutexListHead) {
        gsMutexListHead = mutex->next;
    } else {
        prevMutex = gsMutexListHead;
        for (; prevMutex->next != NULL; prevMutex = prevMutex->next) {
            if (prevMutex->next == mutex) {
                break;
            }
        }
        
        prevMutex->next = mutex->next;
    }

    pcsl_string_free(&(mutex->mutexName));
    midpFree(mutex);
}

/**
 * Locks the mutex.
 *
 * @param mutex data of the mutex to lock
 * @param isolateID ID of Isolate in which locking happens
 */
int javautil_lock_mutex(InterIsolateMutexList* mutex, int isolateID) {
    if (mutex->locked == 0) {
        mutex->locked = 1;
        mutex->mutexHolderIsolateID = isolateID;
    } else {
        if (mutex->mutexHolderIsolateID == isolateID) {
            return JAVAUTIL_MUTEX_ALREADY_LOCKED;
        }
        midp_thread_wait(INTERISOLATE_MUTEX_SIGNAL, mutex->mutexID, 0);
    }

    return JAVAUTIL_MUTEX_OK;
}

/**
 * Unlocks the mutex.
 *
 * @param mutex data of the mutex to unlock
 * @param isolateID ID of Isolate in which unlocking happens
 */
int javautil_unlock_mutex(InterIsolateMutexList* mutex, int isolateID) {
    if (mutex->locked != 1) {
        return JAVAUTIL_MUTEX_NOT_LOCKED;
    }

    if (mutex->mutexHolderIsolateID != isolateID) {
        return JAVAUTIL_MUTEX_ALREADY_LOCKED;
    }

    mutex->locked = 0;
    midp_thread_signal(INTERISOLATE_MUTEX_SIGNAL, mutex->mutexID, 0);

    return JAVAUTIL_MUTEX_OK;
}


void javautil_inc_mutex_refcount(InterIsolateMutexList* mutex) {    
    mutex->refCount++;
}

void javautil_dec_mutex_refcount(InterIsolateMutexList* mutex) {
    mutex->refCount--;

    if (mutex->refCount <= 0) {
        javautil_delete_mutex(mutex);
    }
}
