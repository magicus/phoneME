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
 * @param suiteId ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @return 0 if lock was acquired
 */
int lock_storage(SuiteIdType suiteId, jboolean isUpdate) {
    lockStorageList *currentNodePtr;

    currentNodePtr = find_storage_lock(suiteId);
    if (currentNodePtr != NULL) {
        if (currentNodePtr->update || isUpdate) {
            /* Error: cannot update, the suite is running */
            return SUITE_LOCKED;
        }
        currentNodePtr->timesOpened++;
        return 0;
    } else {
        return insert_storage_lock(suiteId, isUpdate);
    }
}

/**
 * Unlocks a MIDletSuite
 *
 * @param suiteId ID of the suite
 */
void unlock_storage(SuiteIdType suiteId) {
    lockStorageList *currentNodePtr;

    currentNodePtr = find_storage_lock(suiteId);
    if (currentNodePtr != NULL) {
        if (currentNodePtr->update) {
            currentNodePtr->update = KNI_FALSE;
        } else {
            currentNodePtr->timesOpened--;
        }
        if (currentNodePtr->timesOpened == 0) {
            remove_storage_lock(suiteId);
        }
    }
}

/**
 * Removes all the Locks
 *
 */
void remove_all_storage_lock() {
    lockStorageList* currentNodePtr;

    for (currentNodePtr = lockStorageListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        remove_storage_lock(currentNodePtr->suiteId);
    }
}

/**
 * Finds the suite that is locked in the LinkedList
 *
 * @param suiteId ID of the suite
 *
 * @return the locked MIDletSuite; NULL if not found
 */
lockStorageList* find_storage_lock(SuiteIdType suiteId) {
    lockStorageList* currentNodePtr;

    for (currentNodePtr = lockStorageListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        if (currentNodePtr->suiteId == suiteId) {
            return currentNodePtr;
        }
    }

    return NULL;
}

/**
 * Insert the suite into the LinkedList
 *
 * @param suiteId ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @return 0 if lock was acquired
 */
int insert_storage_lock(SuiteIdType suiteId, jboolean isUpdate) {
    lockStorageList* newNodePtr;

    newNodePtr = (lockStorageList *)midpMalloc(sizeof(lockStorageList));
    if (newNodePtr == NULL) {
        return OUT_OF_MEM_LEN;
    }
    memset(newNodePtr, 0, sizeof(lockStorageList));

    newNodePtr->suiteId = suiteId;

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
 * @param suiteId ID of the suite
 *
 */
void remove_storage_lock(SuiteIdType suiteId) {
    lockStorageList* previousNodePtr;
    lockStorageList* currentNodePtr = NULL;

    if (lockStorageListPtr == NULL) {
        return;
    }

    if (lockStorageListPtr->suiteId == suiteId) {
        currentNodePtr = lockStorageListPtr;
        lockStorageListPtr = currentNodePtr->next;
        midpFree(currentNodePtr);
        return;
    }

    for (previousNodePtr = lockStorageListPtr; previousNodePtr->next != NULL;
         previousNodePtr = previousNodePtr->next) {
        if (previousNodePtr->next->suiteId == suiteId) {
            currentNodePtr = previousNodePtr->next;
            break;
        }
    }

    if (currentNodePtr != NULL) {
        previousNodePtr->next = currentNodePtr->next;
        midpFree(currentNodePtr);
    }
}
