/*
 *
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

#include <sni.h>
#include <kni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midpMalloc.h>
#include <midpRMS.h>
#include <midpUtilKni.h>
#include <midpError.h>
#include <midpServices.h>

#include "rms_registry.h"

/**
 * Sends asynchronous notification about change of record store done
 * in the current execution context of method caller
 *
 * @param token security token to restrict usage of the method
 * @param suiteId suite ID of changed record store
 * @param storeName name of changed record store
 * @param changeType type of record change: ADDED, DELETED or CHANGED
 * @param recordId ID of changed record
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_sendRecordStoreChangeEvent) {

    int suiteId = KNI_GetParameterAsInt(1);
    int changeType = KNI_GetParameterAsInt(3);
    int recordId = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {

        rms_registry_send_record_store_change_event(
            suiteId, &storeName, changeType, recordId);
    }
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Starts listening of asynchronous changes of record store
 *
 * @param token security token to restrict usage of the method
 * @param suiteId suite ID of record store to start listen for
 * @param storeName name of record store to start listen for
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_startRecordStoreListening) {

    int suiteId = KNI_GetParameterAsInt(1);
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {
        rms_registry_start_record_store_listening(
            suiteId, &storeName);
    }
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Stops listening of asynchronous changes of record store
 *
 * @param suiteId suite ID of record store to stop listen for
 * @param storeName name of record store to stop listen for
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_stopRecordStoreListening) {

    int suiteId = KNI_GetParameterAsInt(1);
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {
        rms_registry_stop_record_store_listening(
            suiteId, &storeName);
    }
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets list of pairs <task ID, counter> for each VM task listening for changes
 * of given record store. The pair consists of ID of VM task, and of number of
 * record store notifications been sent to this VM task and not acknowledged by
 * it yet. The number of not acknowledged record store notifications must be
 * regarded by notification sender to not overflow event queue of the reciever
 * VM task.
 *
 * @param suiteId suite ID of record store
 * @param storeName name of record store
 * @returns list of <task ID, notification counter> pairs,
 *   or null in the case there are no VM tasks listening for this record store
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_getRecordStoreListeners) {

    int suiteId = KNI_GetParameterAsInt(1);
    KNI_StartHandles(2);
    KNI_DeclareHandle(listenersArray);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {

        int length;
        int *listeners;
        int maxIsolates;

        maxIsolates = getMaxIsolates();
        listeners = (int *)midpMalloc(2 * maxIsolates * sizeof(int));
        if (listeners != NULL) {
            rms_registry_get_record_store_listeners(
                suiteId, &storeName, listeners, &length);

            if (length != 0) {
                SNI_NewArray(SNI_INT_ARRAY, length, listenersArray);
                if (KNI_IsNullHandle(listenersArray)) {
                    KNI_ThrowNew(midpOutOfMemoryError, NULL);
                } else KNI_SetRawArrayRegion(
                    listenersArray, 0, length * sizeof(jint), (jbyte *)listeners);
            } else {
                KNI_ReleaseHandle(listenersArray);
            }
            midpFree(listeners);
        } else {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    }
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandlesAndReturnObject(listenersArray);
}

/**
 * Resets record store notification counter for given VM task.
 * The notification counter is used to request notifications reciever to
 * acknowledge delivery of a series of notifications. Resetting of the
 * counter can be done either after the acknowledgment, or to not wait
 * for an acknowledgment in abnormal situations.
 *
 * @param taskId ID of the VM task ID
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_resetNotificationCounter) {
    int taskId = KNI_GetParameterAsInt(1);
    rms_registry_reset_record_store_notification_counter(taskId);
    KNI_ReturnVoid();
}

/**
 * Acknowledges delivery of record store notifications sent to VM task earlier.
 * The acknowledgment is required by notifications sender for each series of
 * notification events to be sure a reciever can process the notifications and
 * its queue won't be overflowen.
 *
 * @param taskId ID of VM task
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_acknowledgeNotificationsDelivery) {
    int taskId = KNI_GetParameterAsInt(1);
    rms_registry_acknowledge_record_store_notifications(taskId);
    KNI_ReturnVoid();
}

/**
 * Stops listening for any record store changes in VM task
 * @param taskId ID of VM task
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_stopAllRecordStoreListeners) {
    int taskId = KNI_GetParameterAsInt(1);
    rms_regisrty_stop_task_listeners(taskId);
    KNI_ReturnVoid();
}
