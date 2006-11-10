/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <kni.h>
#include <midpMalloc.h>
#include <string.h>
#include <suitestore_locks.h>
#include <pcsl_string.h>
#include <midpUtilKni.h>

static lockStorageList* lockStorageListPtr = NULL;

/**
 * Locks a MIDletSuite
 *
 * @param suiteID ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @return 0 if lock was acquired
 */
int lock_storage(const pcsl_string * suiteID, jboolean isUpdate) {
    lockStorageList *currentNodePtr;

    currentNodePtr = find_storage_lock(suiteID);
    if (currentNodePtr != NULL) {
        if (currentNodePtr->update || isUpdate) {
            /* Error: cannot update, the suite is running */
            return SUITE_LOCKED;
        }
        currentNodePtr->timesOpened++;
        return 0;
    } else {
        return insert_storage_lock(suiteID, isUpdate);
    }
}

/**
 * Unlocks a MIDletSuite
 *
 * @param suiteID ID of the suite
 */
void unlock_storage(const pcsl_string * suiteID) {
    lockStorageList *currentNodePtr;

    currentNodePtr = find_storage_lock(suiteID);
    if (currentNodePtr != NULL) {
        if (currentNodePtr->update) {
            currentNodePtr->update = KNI_FALSE;
        } else {
            currentNodePtr->timesOpened--;
        }
        if (currentNodePtr->timesOpened == 0) {
            remove_storage_lock(suiteID);
        }
    }
}

/**
 * Removes all the Locks
 *
 */
void removeAllStorageLock() {
    lockStorageList* currentNodePtr;

    for (currentNodePtr = lockStorageListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        remove_storage_lock(&currentNodePtr->suiteID);
    }
}

/**
 * Finds the suite that is locked in the LinkedList
 *
 * @param suiteID ID of the suite
 *
 * @return the locked MIDletSuite; NULL if not found
 */
lockStorageList* find_storage_lock(const pcsl_string * suiteID) {
    lockStorageList* currentNodePtr;

    for (currentNodePtr = lockStorageListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        if (pcsl_string_equals(&currentNodePtr->suiteID, suiteID)) {
            return currentNodePtr;
        }
    }
    return NULL;
}

/**
 * Insert the suite into the LinkedList
 *
 * @param suiteID ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @return 0 if lock was acquired
 */
int insert_storage_lock(const pcsl_string * suiteID, jboolean isUpdate) {
    lockStorageList* newNodePtr;

    newNodePtr = (lockStorageList *)midpMalloc(sizeof(lockStorageList));
    if (newNodePtr == NULL) {
        return OUT_OF_MEM_LEN;
    }
    memset(newNodePtr, 0, sizeof(lockStorageList));

    if (PCSL_STRING_OK != pcsl_string_dup(suiteID, &newNodePtr->suiteID)) {
        midpFree(newNodePtr);
        return OUT_OF_MEM_LEN;
    }

    if (isUpdate) {
        newNodePtr->update = isUpdate;
    } else {
        newNodePtr->timesOpened++;
    }
    newNodePtr->next = NULL;

    if (lockStorageListPtr == NULL) {
        lockStorageListPtr = newNodePtr;
    } else {
        newNodePtr->next = lockStorageListPtr;
        lockStorageListPtr = newNodePtr;
    }
    return 0;
}

/**
 * Remove the suite that is locked in the LinkedList
 *
 * @param suiteID ID of the suite
 *
 */
void remove_storage_lock(const pcsl_string * suiteID) {
    lockStorageList* previousNodePtr;
    lockStorageList* currentNodePtr = NULL;

    if (lockStorageListPtr == NULL) {
        return;
    }

    if (pcsl_string_equals(&lockStorageListPtr->suiteID, suiteID)) {
        currentNodePtr = lockStorageListPtr;
        lockStorageListPtr = currentNodePtr->next;
        pcsl_string_free(&currentNodePtr->suiteID);
        midpFree(currentNodePtr);
        return;
    }

    for (previousNodePtr = lockStorageListPtr; previousNodePtr->next != NULL;
         previousNodePtr = previousNodePtr->next) {
        if (pcsl_string_equals(&previousNodePtr->next->suiteID, suiteID)) {
            currentNodePtr = previousNodePtr->next;
            break;
        }
    }

    if (currentNodePtr != NULL) {
        previousNodePtr->next = currentNodePtr->next;
        pcsl_string_free(&currentNodePtr->suiteID);
        midpFree(currentNodePtr);
    }
}


