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

/**
 * @file
 *
 * This header file is interface to the internal MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_LOCKS_H_
#define _SUITESTORE_LOCKS_H_

#include <java_types.h>
#include <midpString.h>
#include <suitestore_common.h>

typedef struct _lockStorageList {
    SuiteIdType suiteId;
    int timesOpened;
    jboolean update;
    struct _lockStorageList* next;
} lockStorageList;

/**
 * Locks a MIDlet Suite to prevent updating a running suite
 *
 * @param suiteId The MIDlet Suite ID.
 * @param isUpdate Notifies the lock that it is updating
 *
 * @returns 0 if lock was made otherwise, OUT_OF_MEM_LEN or
 *          SUITE_LOCKED.
 */
int lock_storage(SuiteIdType suiteId, jboolean isUpdate);

/**
 * Unlocks a MIDlet suite.
 *
 * @param suiteId The MIDlet Suite ID.
 */

void unlock_storage(SuiteIdType suiteId);

/**
 * Finds the suite that is locked in the LinkedList
 *
 * @param suiteId ID of the suite
 *
 * @return the locked MIDletSuite; NULL if not found
 */
lockStorageList* find_storage_lock(SuiteIdType suiteId);

/**
 * Removes all the Locks
 *
 */
void remove_all_storage_lock();

/**
 * Insert the suite into the LinkedList
 *
 * @param suiteId ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @return 0 if lock was acquired
 */
int insert_storage_lock(SuiteIdType suiteId, jboolean isUpdate);

/**
 * Remove the suite that is locked in the LinkedList
 *
 * @param suiteId ID of the suite
 *
 */
void remove_storage_lock(SuiteIdType suiteId);


#endif /* _SUITESTORE_LOCKS_H_ */
