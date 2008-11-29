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
#include <kni_globals.h>
#include <jvmspi.h>
#include <sni.h>
#include <midpMalloc.h>
#include <midpError.h>
#include <midp_thread.h>
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midpUtilKni.h>
#include <commonKNIMacros.h>

#include "javautil_interisolate_mutex.h"

/** Cached field ID to be accessible in finalizer. */
static jfieldID gsMutexIDField = 0;


/**
 * Gets mutex ID for specified mutex name. If there is no native data in 
 * the list associated with specified name, it will be created and added 
 * to the list.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_util_isolate_InterIsolateMutex_getID0) {
    InterIsolateMutexList* mutex = NULL;
    jint mutexID = -1;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, mutexName)

    mutex = javautil_find_mutex_by_name(&mutexName);
    if (mutex == NULL) {
        mutex = javautil_create_mutex(&mutexName);
    }
    
    if (mutex == NULL) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        javautil_inc_mutex_refcount(mutex);
        mutexID = mutex->mutexID;
    }

    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnInt(mutexID);
}

/**
 * Locks the mutex.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_util_isolate_InterIsolateMutex_lock0) {
    jint mutexID;
    InterIsolateMutexList* mutex = NULL;
    jint isolateID;
    int status;

    mutexID = KNI_GetParameterAsInt(1);

    mutex = javautil_find_mutex_by_id(mutexID);
    if (mutex == NULL) {
        KNI_ThrowNew(midpIllegalStateException, "Invalid mutex ID");
    } else {
        isolateID = getCurrentIsolateId();        
        status = javautil_lock_mutex(mutex, isolateID);
        if (status != JAVAUTIL_MUTEX_OK) {
            if (status == JAVAUTIL_MUTEX_ALREADY_LOCKED) {
                KNI_ThrowNew(midpRuntimeException, 
                    "Attempting to lock mutex twice within the same Isolate");
            } else {
                KNI_ThrowNew(midpRuntimeException, 
                    "Unknown error while attempting to lock mutex");
            }
        }
    }

    KNI_ReturnVoid();
}

/**
 * Unlocks the mutex.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_util_isolate_InterIsolateMutex_unlock0) {
    jint mutexID;
    InterIsolateMutexList* mutex = NULL;
    jint isolateID;
    int status;

    mutexID = KNI_GetParameterAsInt(1);

    mutex = javautil_find_mutex_by_id(mutexID);
    if (mutex == NULL) {
        KNI_ThrowNew(midpIllegalStateException, "Invalid mutex ID");
    } else {
        isolateID = getCurrentIsolateId();
        status = javautil_unlock_mutex(mutex, isolateID);
        if (status != JAVAUTIL_MUTEX_OK) {
            if (status == JAVAUTIL_MUTEX_ALREADY_LOCKED) {
                KNI_ThrowNew(midpRuntimeException,
                        "Mutex is locked by different Isolate");
            } else if (status == JAVAUTIL_MUTEX_NOT_LOCKED) {
                KNI_ThrowNew(midpRuntimeException, "Mutex is not locked");
            } else {
                KNI_ThrowNew(midpRuntimeException, 
                    "Unknown error while attempting to unlock mutex");
            }
        }
    }

    KNI_ReturnVoid();
}

/**
 * Native finalizer for unlocking mutex when Isolate that locked it 
 * goes down. Also removes mutex data from the list when there are
 * no more InterIsolateMutex instances referencing it.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_util_isolate_InterIsolateMutex_finalize) {
    jint mutexID;
    InterIsolateMutexList* mutex = NULL;
    int isolateID;

    KNI_StartHandles(2);
    KNI_DeclareHandle(classHandle);
    KNI_DeclareHandle(thisHandle);

    KNI_GetThisPointer(thisHandle);

    if (gsMutexIDField == 0) {
        KNI_GetObjectClass(thisHandle, classHandle);
        gsMutexIDField = KNI_GetFieldID(classHandle, "mutexID", "I");
    }
    mutexID = KNI_GetIntField(thisHandle, gsMutexIDField);

    mutex = javautil_find_mutex_by_id(mutexID);
    if (mutex == NULL) {
        KNI_ThrowNew(midpIllegalStateException, "Invalid mutex ID");
    } else {
        isolateID = getCurrentIsolateId();
        if (mutex->locked && mutex->mutexHolderIsolateID == isolateID) {
            javautil_unlock_mutex(mutex, isolateID);
        }

        javautil_dec_mutex_refcount(mutex); 
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

