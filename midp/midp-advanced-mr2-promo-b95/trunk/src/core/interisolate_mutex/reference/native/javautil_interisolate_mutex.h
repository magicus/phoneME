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

#ifndef JAVAUTIL_INTERISOLATE_MUTEX_H
#define JAVAUTIL_INTERISOLATE_MUTEX_H

#include <jvmconfig.h>
#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Native data associated with InterIsolateMutex class. Also, the list.
 */
typedef struct InterIsolateMutexListStruct {
    /** Mutex ID */
    jint mutexID;

    /** Mutex name */
    pcsl_string mutexName;

    /** 1 if mutex is locked, 0 otherwise */
    int locked;

    /** ID of Isolate that locked the mutex */
    int mutexHolderIsolateID;

    /** How many InterIsolateMutex instances reference this native data */
    int refCount;

    /** Next node in list */ 
    struct InterIsolateMutexListStruct* next;
} InterIsolateMutexList;


/**
 * Status codes related to operations with mutex
 */
#define JAVAUTIL_MUTEX_OK 0     
#define JAVAUTIL_MUTEX_NOT_LOCKED -1
#define JAVAUTIL_MUTEX_ALREADY_LOCKED -2

/**
 * Finds mutex data in list by ID.
 *
 * @param mutexID mutex ID
 * @return mutex data that has specified ID, NULL if not found
 */
InterIsolateMutexList* javautil_find_mutex_by_id(int mutexID);

/**
 * Finds mutex data in list by name.
 *
 * @param mutexName mutex name
 * @return mutex data that has specified name, NULL if not found
 */
InterIsolateMutexList* javautil_find_mutex_by_name(const pcsl_string* mutexName);

/**
 * Creates mutex data associated with specified name and puts it into list.
 *
 * @param mutexName mutex name
 * @return pointer to created data, NULL if OOM 
 */
InterIsolateMutexList* javautil_create_mutex(const pcsl_string* mutexName);

/**
 * Deletes mutex data and removes it from the list.
 *
 * @param mutex pointer to mutex data to delete
 */
void javautil_delete_mutex(InterIsolateMutexList* mutex);

/**
 * Locks the mutex.
 *
 * @param mutex data of the mutex to lock
 * @param isolateID ID of Isolate in which locking happens
 * @return 0 if locking succeeded, error code otherwise
 */
int javautil_lock_mutex(InterIsolateMutexList* mutex, int isolateID);

/**
 * Unlocks the mutex.
 *
 * @param mutex data of the mutex to unlock
 * @param isolateID ID of Isolate in which unlocking happens
 * @return 0 if unlocking succeeded, error code otherwise
 */
int javautil_unlock_mutex(InterIsolateMutexList* mutex, int isolateID);

/**
 * Increases mutex ref count.
 *
 * @param mutex data of the mutex to increase ref count for
 */
void javautil_inc_mutex_refcount(InterIsolateMutexList* mutex);

/**
 * Decreases mutex ref count.
 *
 * @param mutex data of the mutex to decrease ref count for
 */
void javautil_dec_mutex_refcount(InterIsolateMutexList* mutex);

#ifdef __cplusplus
}
#endif

#endif /* JAVAUTIL_INTERISOLATE_MUTEX_H */
