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

/** Cached number of maximal allowed VM tasks */
static int maxTasks = 0;

/**
  * The structure is designed to store total number of record store
  * notifications sent to VM task and not acknowledged by it yet
  */
typedef struct _NotificationCounter {
    int taskId;
    int count;
    struct _NotificationCounter *next;
} NotificationCounter;

/**
 * The structure is designed to store information about listeners
 * of a record store registered in different execution contexts
 */
typedef struct _RecordStoreListener {
    int count;                           /* number of VM tasks with listeners of the record store */
    int suiteId;                         /* suite ID of record store */
    pcsl_string recordStoreName;         /* name of record store */
    int *listenerId;                     /* IDs of VM tasks with listeneres of the record store */
    struct _RecordStoreListener *next;   /* reference to next entry in the list of structures */
} RecordStoreListener;

/** List of registered listeners for record store changes */
static RecordStoreListener *rootListenerPtr = NULL;

/** List of notification counters of VM tasks */
static NotificationCounter *rootCounterPtr = NULL;


/** Remove notification counter of VM task from the list */
static void removeNotificationCounter(int taskId) {
    NotificationCounter *counterPtr;
    NotificationCounter **counterRef;
    counterRef = &rootCounterPtr;
    while ((counterPtr = *counterRef) != NULL) {
        if (counterPtr->taskId == taskId) {
            *counterRef = counterPtr->next;
            midpFree(counterPtr);
            break;
        }
        counterRef = &(counterPtr->next);
    }
}

/** Search for notification counter of VM task */
static NotificationCounter *findNotificationCounter(int taskId) {
    NotificationCounter *counterPtr;
    for (counterPtr = rootCounterPtr; counterPtr != NULL;
            counterPtr = counterPtr->next) {
        if (counterPtr->taskId == taskId) {
            return counterPtr;
        }
    }
    return NULL;
}

/** Resets notification counter for given VM task */
static void resetNotificationCounter(int taskId) {
    NotificationCounter *counterPtr;
    if ((counterPtr = findNotificationCounter(taskId)) != NULL) {
        counterPtr->count = 0;
    }
}

/** Gets notifications count for given VM task */
static int getNotificationsCount(int taskId) {
    NotificationCounter *counterPtr;
    if ((counterPtr = findNotificationCounter(taskId)) != NULL) {
        return counterPtr->count;
    }
    return 0;
}

/**
 * Increments notification counter for given VM task
 * and returns its current value
 */
static int incNotificationCounter(int taskId) {
    NotificationCounter *counterPtr;
    /* Create new counter structure if needed */
    if ((counterPtr = findNotificationCounter(taskId)) == NULL) {
        counterPtr = (NotificationCounter *)midpMalloc(
            sizeof(NotificationCounter));
        if (counterPtr == NULL) {
            REPORT_CRIT(LC_RMS,
                "rms_registry: OUT OF MEMORY");
            return 0;
        }
        counterPtr->next = rootCounterPtr;
        counterPtr->taskId = taskId;
        counterPtr->count = 0;
        rootCounterPtr = counterPtr;
    }
    /* Return incremented counter value */
    return ++counterPtr->count;
}

/** Finds record store listener by suite ID and record store name */
static RecordStoreListener *findRecordStoreListener(
        int suiteId, pcsl_string* recordStoreName) {

    RecordStoreListener *currentPtr;
    for (currentPtr = rootListenerPtr; currentPtr != NULL;
                currentPtr = currentPtr->next) {
        if (currentPtr->suiteId == suiteId &&
            pcsl_string_equals(&currentPtr->recordStoreName, recordStoreName)) {
            return currentPtr;
        }
    }
    return NULL;
}

/** Detects whether given VM task listens for some record store changes */
static int hasRecordStoreListeners(int taskId) {
    RecordStoreListener *listenerPtr;
    for (listenerPtr = rootListenerPtr; listenerPtr != NULL;
                listenerPtr = listenerPtr->next) {
        int i;
        for (i = 0; i < listenerPtr->count; i++) {
            if (listenerPtr->listenerId[i] == taskId) {
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
    RecordStoreListener *newPtr;

    newPtr = (RecordStoreListener *)midpMalloc(
        sizeof(RecordStoreListener));
    if (newPtr == NULL) {
        REPORT_CRIT(LC_RMS,
            "rms_registry: OUT OF MEMORY");
        return;
    }
    newPtr->listenerId =
        (int *)midpMalloc(maxTasks * sizeof(int));
    if (newPtr->listenerId == NULL) {
        REPORT_CRIT(LC_RMS,
            "rms_registry: OUT OF MEMORY");
        return;
    }

    newPtr->count = 1;
    newPtr->suiteId = suiteId;
    newPtr->listenerId[0] = listenerId;
    rc = pcsl_string_dup(recordStoreName, &newPtr->recordStoreName);
    if (rc != PCSL_STRING_OK) {
        midpFree(newPtr);
        REPORT_CRIT(LC_RMS,
            "rms_registry: OUT OF MEMORY");
        return;
    }
    newPtr->next = NULL;

    if (rootListenerPtr== NULL) {
        rootListenerPtr= newPtr;
    } else {
        newPtr->next = rootListenerPtr;
        rootListenerPtr = newPtr;
    }
}

/** Unlinks listener node from the list and frees its resources */
static void deleteListenerNodeByRef(RecordStoreListener **listenerRef) {
    RecordStoreListener *listenerPtr;

    listenerPtr = *listenerRef;
    *listenerRef = listenerPtr->next;
    pcsl_string_free(&listenerPtr->recordStoreName);
    midpFree(listenerPtr->listenerId);
    midpFree(listenerPtr);
}

/** Deletes earlier found record store listener entry */
static void deleteListenerNode(RecordStoreListener *listenerPtr) {
    RecordStoreListener **listenerRef;

    listenerRef = &rootListenerPtr;
    while (*listenerRef != listenerPtr) {
        /* No check for NULL, the function is called for list nodes only */
        listenerRef = &((*listenerRef)->next);
    }
    deleteListenerNodeByRef(listenerRef);
}

/** Registeres current VM task to get notifications on record store changes */
void rms_registry_start_record_store_listening(int suiteId, pcsl_string *storeName) {

    RecordStoreListener *listenerPtr;
    int taskId = getCurrentIsolateId();

    /* Cache maximal VM tasks number on first demand */
    if (maxTasks == 0) {
        maxTasks = getMaxIsolates();
    }

    /* Search for existing listener entry to update listener ID */
    listenerPtr = findRecordStoreListener(suiteId, storeName);
    if (listenerPtr != NULL) {
        int i, count;
        count = listenerPtr->count;
        for (i = 0; i < count; i++) {
            if (listenerPtr->listenerId[i] == taskId) {
                return;
            }
        }
        if (count >= maxTasks) {
            REPORT_CRIT(LC_RMS,
                "rms_registry_start_record_store_listening: "
                "internal structure overflow");
            return;
        }
        listenerPtr->listenerId[count] = taskId;
        listenerPtr->count = count + 1;
        return;
    }
    /* Create new listener entry for record store */
    createListenerNode(suiteId, storeName, taskId);
}

/** Unregisters current VM task from the list of listeners of record store changes */
void rms_registry_stop_record_store_listening(int suiteId, pcsl_string *storeName) {
    RecordStoreListener *listenerPtr;
    listenerPtr = findRecordStoreListener(suiteId, storeName);
    if (listenerPtr != NULL) {
        int i, count, taskId;
        count = listenerPtr->count;
        taskId = getCurrentIsolateId();
        for (i = 0; i < count; i++) {
            if (listenerPtr->listenerId[i] == taskId) {
                if (--count == 0) {
                    deleteListenerNode(listenerPtr);
                } else {
                    listenerPtr->listenerId[i] =
                        listenerPtr->listenerId[count];
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

    RecordStoreListener *listenerPtr;
    listenerPtr = findRecordStoreListener(suiteId, storeName);
    if (listenerPtr != NULL) {
        int i;
        pcsl_string_status rc;
        int taskId = getCurrentIsolateId();

        for (i = 0; i < listenerPtr->count; i++) {
            int listenerId = listenerPtr->listenerId[i];
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

    RecordStoreListener *listenerPtr;
    *length = 0;
    listenerPtr = findRecordStoreListener(suiteId, storeName);
    if (listenerPtr != NULL) {
        int i;
        int taskId = getCurrentIsolateId();
        for (i = 0; i < listenerPtr->count; i++) {
            int listenerId = listenerPtr->listenerId[i];
            if (listenerId != taskId) {
                int index = *length;
                listeners[index] = listenerId;
                listeners[index + 1] = getNotificationsCount(listenerId);
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
    RecordStoreListener *listenerPtr;
    RecordStoreListener **listenerRef;

    /* Remove notification counter of the VM task */
    removeNotificationCounter(taskId);
    
    /* Remove task ID from all record store listener structures */ 
    listenerRef = &rootListenerPtr;
    while((listenerPtr = *listenerRef) != NULL) {
        int i, count;
        count = listenerPtr->count;
        for (i = 0; i < count; i++) {
            if (listenerPtr->listenerId[i] == taskId) {
                if (--count == 0) {
                    deleteListenerNodeByRef(listenerRef);
                } else {
                    listenerPtr->listenerId[i] =
                        listenerPtr->listenerId[count];
                }
                break;
            }
        }
        listenerRef = &(listenerPtr->next);
    }
}
