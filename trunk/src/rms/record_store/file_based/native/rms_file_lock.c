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

#include "kni.h"
#include "midp_file_cache.h"
#include <midpStorage.h>
#include <midpMalloc.h>
#include <midpInit.h>
#include <midpRMS.h>
#include <midpUtilKni.h>
#include <suitestore_rms.h>
#include <limits.h> /* for LONG_MAX */
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include "rms_file_lock.h"

/** Locks list header */
static lockFileList* lockFileListPtr = NULL;

/*
 * Insert a new node to the front of the linked list
 *
 * @param filenameBase : filename base of the suite
 * @param name : Name of the record store
 * @param handle : Handle of the opened file
 *
 * @return 0 if node is successfully inserted
 *  return  OUT_OF_MEM_LEN if memory allocation fails
 *
 */
int
rmsdb_create_file_lock(pcsl_string* filenameBase, 
        const pcsl_string* name_str, int handle) {
    lockFileList* newNodePtr;

    newNodePtr = (lockFileList*)midpMalloc(sizeof(lockFileList));
    if (newNodePtr == NULL) {
        return OUT_OF_MEM_LEN;
    }

    newNodePtr->filenameBase = *filenameBase;

    /*IMPL_NOTE: check for error codes instead of null strings*/
    pcsl_string_dup(name_str, &newNodePtr->recordStoreName);
    if (pcsl_string_is_null(&newNodePtr->recordStoreName)) {
        midpFree(newNodePtr);
        return OUT_OF_MEM_LEN;
    }

    newNodePtr->handle = handle;
    newNodePtr->next = NULL;

    if (lockFileListPtr == NULL) {
        lockFileListPtr = newNodePtr;
    } else {
        newNodePtr->next = lockFileListPtr;
        lockFileListPtr = newNodePtr;
    }

    return 0;
}

/** 
 * Delete a node from the linked list
 *
 * @param handle : handle of the file
 *
 */
void
rmsdb_delete_file_lock(int handle) {
    lockFileList* previousNodePtr;
    lockFileList* currentNodePtr = NULL;

    /*
     * Very important to check that lockFileListPtr is NOT null as this function
     * is invoked two times : once during close of a db file and again for
     * deleting index file. The linked list node either does not exist or
     * lockFileListPtr pointer is NULL during second call.
     */
    if (lockFileListPtr == NULL) {
        return;
    }

    /* If it's first node, delete it and re-assign head pointer */
    if (lockFileListPtr->handle == handle) {
        currentNodePtr = lockFileListPtr;
        lockFileListPtr = currentNodePtr->next;
        pcsl_string_free(&currentNodePtr->recordStoreName);
        midpFree(currentNodePtr);
        return;
    }

    for (previousNodePtr = lockFileListPtr; previousNodePtr->next != NULL;
         previousNodePtr = previousNodePtr->next) {
        if (previousNodePtr->next->handle == handle) {
                currentNodePtr = previousNodePtr->next;
                break;
            }
    }

    /* Current node needs to be deleted */
    if (currentNodePtr != NULL) {
        previousNodePtr->next = currentNodePtr->next;
        pcsl_string_free(&currentNodePtr->recordStoreName);
        midpFree(currentNodePtr);
    }
}

/*
 * Search for the node to the linked list
 *
 * @param filenameBase : filename base of the suite
 * @param name : Name of the record store
 *
 * @return the searched node pointer if match exist for filenameBase and
 * recordStoreName
 *  return NULL if node does not exist
 *
 */
lockFileList*
rmsdb_find_file_lock_by_id(pcsl_string* filenameBase, const pcsl_string* name_str) {
    lockFileList* currentNodePtr;

    for (currentNodePtr = lockFileListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        if (pcsl_string_equals(&currentNodePtr->filenameBase, filenameBase) &&
            pcsl_string_equals(&currentNodePtr->recordStoreName, name_str)) {
            return currentNodePtr;
        }
    }

    return NULL;
}

