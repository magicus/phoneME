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
#include <rms_registry.h>

typedef struct _RecordStoreListener {
    int count;
    int suiteId;
    pcsl_string recordStoreName;
    int listenerId[MAX_ISOLATES];
    struct _RecordStoreListener *next;
} RecordStoreListener;

/** List of registered listeners for record store changes */
static RecordStoreListener *rootListenerPtr = NULL;

/** Find record store listener by suite ID and record store name */
static RecordStoreListener* findRecordStoreListener(
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

/** Creates new record store listener and adds it to the list of know listeners */
static int createRecordStoreListener(
        int suiteId, pcsl_string *recordStoreName) {

    pcsl_string_status rc;
    RecordStoreListener *newNodePtr;

    newNodePtr = (RecordStoreListener *)midpMalloc(
        sizeof(RecordStoreListener));
    if (newNodePtr == NULL) {
        return OUT_OF_MEM_LEN;
    }

    newNodePtr->count = 1;
    newNodePtr->suiteId = suiteId;
    newNodePtr->listenerId[0] = getCurrentIsolateId();
    rc = pcsl_string_dup(recordStoreName, &newNodePtr->recordStoreName);
    if (rc != PCSL_STRING_OK) {
        midpFree(newNodePtr);
        return OUT_OF_MEM_LEN;
    }
    newNodePtr->next = NULL;

    if (rootListenerPtr== NULL) {
        rootListenerPtr= newNodePtr;
    } else {
        newNodePtr->next = rootListenerPtr;
        rootListenerPtr = newNodePtr;
    }
    return 0;
}

/** Deletes earlier found record store listener entry */
static void deleteListenerNode(
        RecordStoreListener *listenerNodePtr) {

    if (rootListenerPtr == listenerNodePtr) {
        rootListenerPtr = listenerNodePtr->next;
    } else {
        RecordStoreListener *prevNodePtr = rootListenerPtr;
        while (prevNodePtr->next != listenerNodePtr) {
            prevNodePtr = prevNodePtr->next;
        }
        prevNodePtr->next = listenerNodePtr->next;
    }
    pcsl_string_free(&listenerNodePtr->recordStoreName);
    midpFree(listenerNodePtr);
}

/** Deletes record store listener */
static void deleteRecordStoreListener(
        int suiteId, pcsl_string *recordStoreName) {

    RecordStoreListener* searchedNodePtr;
    searchedNodePtr = findRecordStoreListener(suiteId, recordStoreName);
    if (searchedNodePtr != NULL) {
        deleteListenerNode(searchedNodePtr);
    }
}

/** Registeres current VM task to get notifications on record store changes */
void rms_registry_start_record_store_listening(int suiteId, pcsl_string *storeName) {
    RecordStoreListener *listenerNodePtr;

    // Search for existing listener entry to update listener ID
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i, count, currentIsolateId;
        count = listenerNodePtr->count;
        currentIsolateId = getCurrentIsolateId();
        for (i = 0; i < count; i++) {
            if (listenerNodePtr->listenerId[i] == currentIsolateId) {
                return;
            }
        }
        listenerNodePtr->listenerId[count] = currentIsolateId;
        listenerNodePtr->count = count + 1;
        return;
    }
    // Create new listener entry for record store
    createRecordStoreListener(suiteId, storeName);
}

/** Unregisters current VM task from the list of listeners of record store changes */
void rms_registry_stop_record_store_listening(int suiteId, pcsl_string *storeName) {
    RecordStoreListener *listenerNodePtr;
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i, count, currentIsolateId;
        count = listenerNodePtr->count;
        currentIsolateId = getCurrentIsolateId();
        for (i = 0; i < count; i++) {
            if (listenerNodePtr->listenerId[i] == currentIsolateId) {
                count--;
                if (count == 0) {
                    deleteListenerNode(listenerNodePtr);
                } else {
                    listenerNodePtr->listenerId[i] =
                        listenerNodePtr->listenerId[count];
                }
                return;
            }
        }
    }
}

/** Notifies registered record store listeneres about record store change */
void rms_registry_notify_record_store_change(
        int suiteId, pcsl_string *storeName, int changeType, int recordId) {

    RecordStoreListener *listenerNodePtr;
    listenerNodePtr = findRecordStoreListener(suiteId, storeName);
    if (listenerNodePtr != NULL) {
        int i;
        pcsl_string_status rc;
        int currentIsolateId = getCurrentIsolateId();

        for (i = 0; i < listenerNodePtr->count; i++) {
            int listenerId = listenerNodePtr->listenerId[i];
            if (listenerId != currentIsolateId) {
                MidpEvent evt;

                MIDP_EVENT_INITIALIZE(evt);
                evt.type = RECORD_STORE_CHANGE_EVENT;
                evt.intParam1 = suiteId;
                evt.intParam2 = changeType;
                evt.intParam3 = recordId;
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
