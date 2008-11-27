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

#include <pcsl_string.h>
#include <midpServices.h>
#include <midpMalloc.h>
#include <midpEvents.h>
#include <midpInit.h>

#include "rms_registry.h"

/**
 * List of pairs <ID, counter> corresponding to number of
 * record store notifications sent to VM task with given ID
 * and not acknowledged by this VM task yet
 */
static int notificationCounters[MAX_ISOLATES * 2];
static int notificationCountersNumber = 0;

/**
 * The structure is designed to store information about listeners
 * of a record store registered in different execution contexts
 */
typedef struct _RecordStoreListener {
    int count;                           /* number of VM tasks with listeners of the record store */
    int suiteId;                         /* suite ID of record store */
    pcsl_string recordStoreName;         /* name of record store */
    int listenerId[MAX_ISOLATES];        /* IDs of VM tasks with listeneres of the record store */
    struct _RecordStoreListener *next;   /* reference to next entry in the list of structures */
} RecordStoreListener;

/** List of registered listeners for record store changes */
static RecordStoreListener *rootListenerPtr = NULL;

/** Remove notification counter of VM task from the list */
static void removeNotificationCounter(int taskId) {
    int i, index;
    for (i = 0; i < notificationCountersNumber; i++) {
        index = i * 2;
        if (notificationCounters[index] == taskId) {
            int last = (--notificationCountersNumber) * 2;
            if (index != last) {
                notificationCounters[index] = notificationCounters[last];
                notificationCounters[index + 1] = notificationCounters[last + 1];
            }
            return;
        }
    }
}

/** Resets notification counter for given VM task */
static void resetNotificationCounter(int taskId) {
    int i, index;
    for (i = 0; i < notificationCountersNumber; i++) {
        index = i * 2;
        if (notificationCounters[index] == taskId) {
            notificationCounters[index + 1] = 0;
            return;
        }
    }
}

/** Gets notification counter for given VM task */
static int getNotificationCounter(int taskId) {
    int i, index;
    for (i = 0; i < notificationCountersNumber; i++) {
        index = i * 2;
        if (notificationCounters[index] == taskId) {
            return notificationCounters[index + 1];
        }
    }
    return 0;
}

/**
 * Increments notification counter for given VM task
 * and returns its current value
 */
static int incNotificationCounter(int taskId) {
    int i, index, counter;
    for (i = 0; i < notificationCountersNumber; i++) {
        index = i * 2;
        if (notificationCounters[index] == taskId) {
            counter = ++notificationCounters[index + 1];
            return counter;
        }
    }

    if (notificationCountersNumber < MAX_ISOLATES) {
        index = notificationCountersNumber * 2;
        notificationCountersNumber++;
    } else {
        index = 0;
        REPORT_CRIT(LC_RMS,
            "rms_registry: notification counters list overflow");

    }
    counter = 1;
    notificationCounters[index] = taskId;
    notificationCounters[index + 1] = counter;
    return counter;
}

/** Finds record store listener by suite ID and record store name */
static RecordStoreListener *findRecordStoreListener(
        int suiteId, pcsl_string* recordStoreName) {

    RecordStoreListener *currentNodePtr;
    for (currentNodePtr = rootListenerPtr; currentNodePtr != NULL;
                currentNodePtr = currentNodePtr->next) {
        if (currentNodePtr->suiteId == suiteId &&
            pcsl_string_equals(&currentNodePtr->recordStoreName, recordStoreName)) {
            return currentNodePtr;
        }
    }
    return NULL;
}

/** Detects whether given VM task listens for some record store changes */
static int hasRecordStoreListeners(int taskId) {
    RecordStoreListener *listenerNodePtr;
    for (listenerNodePtr = rootListenerPtr; listenerNodePtr != NULL;
                listenerNodePtr = listenerNodePtr->next) {
        int i;
        for (i = 0; i < listenerNodePtr->count; i++) {
            if (listenerNodePtr->listenerId[i] == taskId) {
                return 1;
            }
        }
    }
    return 0;
}

/** Creates new record store listener and adds it to the list of know listeners */
static void createListenerNode(
        int suiteId, pcsl_string *recordStoreName, int listenerId) {

    pcsl_string_status rc;
    RecordStoreListener *newNodePtr;

    newNodePtr = (RecordStoreListener *)midpMalloc(
        sizeof(RecordStoreListener));
    if (newNodePtr == NULL) {
        REPORT_CRIT(LC_RMS,
            "rms_registry: OUT OF MEMORY");
        return;
    }

    newNodePtr->count = 1;
    newNodePtr->suiteId = suiteId;
    newNodePtr->listenerId[0] = listenerId;
    rc = pcsl_string_dup(recordStoreName, &newNodePtr->recordStoreName);
    if (rc != PCSL_STRING_OK) {
        midpFree(newNodePtr);
        REPORT_CRIT(LC_RMS,
            "rms_registry: OUT OF MEMORY");
        return;
    }
    newNodePtr->next = NULL;

    if (rootListenerPtr== NULL) {
        rootListenerPtr= newNodePtr;
    } else {
        newNodePtr->next = rootListenerPtr;
        rootListenerPtr = newNodePtr;
    }
}

/** Unlinks listener node from the list and frees its resources */
static void deleteListenerNodeByRef(RecordStoreListener **listenerNodeRef) {
    RecordStoreListener *listenerNodePtr;

    listenerNodePtr = *listenerNodeRef;
    *listenerNodeRef = listenerNodePtr->next;
    pcsl_string_free(&listenerNodePtr->recordStoreName);
    midpFree(listenerNodePtr);
}

/** Deletes earlier found record store listener entry */
static void deleteListenerNode(RecordStoreListener *listenerNodePtr) {
    RecordStoreListener **listenerNodeRef;

    listenerNodeRef = &rootListenerPtr;
    while (*listenerNodeRef != listenerNodePtr) {
        /* No check for NULL, the function is called for list nodes only */
        listenerNodeRef = &((*listenerNodeRef)->next);
    }
    deleteListenerNodeByRef(listenerNodeRef);
}

/** Registeres current VM task to get notifications on record store changes */
void rms_registry_start_record_store_listening(int suiteId, pcsl_string *storeName) {

    RecordStoreListener *listenerNodePtr;
    int taskId = getCurrentIsolateId();

    /* Search for existing listener entry to update listener ID */
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i, count;
        count = listenerNodePtr->count;
        for (i = 0; i < count; i++) {
            if (listenerNodePtr->listenerId[i] == taskId) {
                return;
            }
        }
        listenerNodePtr->listenerId[count] = taskId;
        listenerNodePtr->count = count + 1;
        return;
    }
    /* Create new listener entry for record store */
    createListenerNode(suiteId, storeName, taskId);
}

/** Unregisters current VM task from the list of listeners of record store changes */
void rms_registry_stop_record_store_listening(int suiteId, pcsl_string *storeName) {
    RecordStoreListener *listenerNodePtr;
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i, count, taskId;
        count = listenerNodePtr->count;
        taskId = getCurrentIsolateId();
        for (i = 0; i < count; i++) {
            if (listenerNodePtr->listenerId[i] == taskId) {
                if (--count == 0) {
                    deleteListenerNode(listenerNodePtr);
                } else {
                    listenerNodePtr->listenerId[i] =
                        listenerNodePtr->listenerId[count];
                }
                /* Remove notification counter of the task
                 * not listening for any record store changes */
                if (!hasRecordStoreListeners(taskId)) {
                    removeNotificationCounter(taskId);
                }

                return;
            }
        }
    }
}

/** Notifies registered record store listeneres about record store change */                                
void rms_registry_send_record_store_change_event(
        int suiteId, pcsl_string *storeName, int changeType, int recordId) {

    RecordStoreListener *listenerNodePtr;
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i;
        pcsl_string_status rc;
        int taskId = getCurrentIsolateId();

        for (i = 0; i < listenerNodePtr->count; i++) {
            int listenerId = listenerNodePtr->listenerId[i];
            if (listenerId != taskId) {
                int counter;
                MidpEvent evt;
                int requiresAcknowledgment = 0;

                /* Request acknowledgment from receiver after sending a series
                 * of notifications to protect receiver from queue overflow. */
                counter = incNotificationCounter(listenerId);
                if (counter == RECORD_STORE_NOTIFICATION_QUEUE_SIZE / 2) {
                    requiresAcknowledgment = 1;
                }

                MIDP_EVENT_INITIALIZE(evt);
                evt.type = RECORD_STORE_CHANGE_EVENT;
                evt.intParam1 = suiteId;
                evt.intParam2 = changeType;
                evt.intParam3 = recordId;
                evt.intParam4 = requiresAcknowledgment;
                rc = pcsl_string_dup(storeName, &evt.stringParam1);
                if (rc != PCSL_STRING_OK) {
                    REPORT_CRIT(LC_RMS,
                        "rms_registry_notify_record_store_change(): OUT OF MEMORY");
                    return;
                }

                StoreMIDPEventInVmThread(evt, listenerId);
                REPORT_INFO1(LC_RMS, "rms_registry_notify_record_store_change(): "
                    "notify VM task %d of RMS changes", listenerId);

            }
        }
    }
}

/** Fills list of listeners with pairs <listener ID, notification counter> */
void rms_registry_get_record_store_listeners(
        int suiteId, pcsl_string *storeName, int *listeners, int *length) {

    RecordStoreListener *listenerNodePtr;
    *length = 0;
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i;
        int taskId = getCurrentIsolateId();
        for (i = 0; i < listenerNodePtr->count; i++) {
            int listenerId = listenerNodePtr->listenerId[i];
            if (listenerId != taskId) {
                int index = *length;
                listeners[index] = listenerId;
                listeners[index + 1] = getNotificationCounter(listenerId);
                *length += 2;
            }
        }
    }
}

/** Resets notification counter of VM task */
void rms_registry_reset_record_store_notification_counter(int taskId) {
    resetNotificationCounter(taskId);
}

/** Acknowledges delivery of record store notifications */
void rms_registry_acknowledge_record_store_notifications(int taskId) {
    resetNotificationCounter(taskId);
}

/** Stops listening for any record store changes in the VM task */
void rms_regisrty_stop_task_listeners(int taskId) {
    RecordStoreListener *listenerNodePtr;
    RecordStoreListener **listenerNodeRef;

    /* Remove notification counter of the VM task */
    removeNotificationCounter(taskId);
    
    /* Remove task ID from all record store listener structures */ 
    listenerNodeRef = &rootListenerPtr;
    while((listenerNodePtr = *listenerNodeRef) != NULL) {
        int i, count;
        count = listenerNodePtr->count;
        for (i = 0; i < count; i++) {
            if (listenerNodePtr->listenerId[i] == taskId) {
                if (--count == 0) {
                    deleteListenerNodeByRef(listenerNodeRef);
                } else {
                    listenerNodePtr->listenerId[i] =
                        listenerNodePtr->listenerId[count];
                }
                break;
            }
        }
        listenerNodeRef = &(listenerNodePtr->next);
    }
}
