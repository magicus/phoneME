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

#include <windows.h>
#include "javacall_defs.h"
#include "javacall_memory.h"
#include "javacall_eventqueue.h"


static int specialId;   /* thread safety */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Creates the event queue lock.
 *
 * @return <tt>JAVACALL_OK</tt> if successully created lock
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_create_event_queue_lock(void){
    /*
     * The Master mode needs a lock since all events put in asynchronously.
     * But slave mode may not if events can be put in the system GUI event
     * system.
     */
    HANDLE *mutex;

    specialId = TlsAlloc();
    if (TLS_OUT_OF_INDEXES == specialId) {
        return JAVACALL_FAIL;
    }

    mutex = javacall_malloc(sizeof(HANDLE));
    if (NULL == mutex) {
        return JAVACALL_FAIL;
    }

    if (0 == TlsSetValue(specialId, mutex)) {
        return JAVACALL_FAIL;
    }

    *mutex = CreateMutex(0, JAVACALL_FALSE, "eventQueueMutex");
    if (NULL == *mutex) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Destroys the event queue lock.
 *
 * @return <tt>JAVACALL_OK</tt> if successully destroyed lock
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_destroy_event_queue_lock(void){
    /* Clean up thread local data */
    void* ptr = (void*) TlsGetValue(specialId);

    if (ptr != NULL) {
        /* Must free TLS data before freeing the TLS (special) ID */
        javacall_free(ptr);
        TlsFree(specialId);
        return JAVACALL_OK;
    }
    else {
        return JAVACALL_FAIL;
    }
}

/**
 * Waits to get the event queue lock and then locks it.
 *
 * @return <tt>JAVACALL_OK</tt> if successully obtained lock
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_wait_and_lock_event_queue(void){
    HANDLE *mutex = (HANDLE*) TlsGetValue(specialId);

    if (0 == mutex || 0 == *mutex) {
        return JAVACALL_FAIL;
    }

    if (WAIT_FAILED == WaitForSingleObject(*mutex, INFINITE)) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Unlocks the event queue.
 *
 * @return <tt>JAVACALL_OK</tt> if successully released lock
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_unlock_event_queue(void){
    HANDLE *mutex = (HANDLE*) TlsGetValue(specialId);

    if (0 == mutex || 0 == *mutex) {
        return JAVACALL_FAIL;
    }
    if (0 == ReleaseMutex(*mutex)) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}


#ifdef __cplusplus
} //extern "C"
#endif


