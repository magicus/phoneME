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
 * IMPL_NOTE: 
 * This locking mechanism is used for ports that doesn't have 
 * AbstractRecordStoreLock interface implemented (currently, 
 * MIDP on CDC). In this case, only exclusive record store 
 * usage is allowed (see comment below).
 */

/**
 * @file
 *
 * <h2>
 * <center> Sequential Access in RMS </center>
 * </h2>
 *
 * <p>
 * In case of MVM, there is a potential issue of accessing the same record
 * store file by multiple isolates at the same time. In the following,
 * a sequential access to the RMS record store is implemented to maintain
 * the data integrity in MVM. It is based on the assumption that two isolates
 * can share a single record store file but can not access it
 * simultaneously at the same time. If two isolates try to access
 * the same record store file at the same time, the later one will get a
 * RecordStoreException.
 * </p>
 *
 * <h3> Implementation Details </h3>
 * <p>
 * Native calls are serialized across isolates. In this
 * approach, a linked list is maintained for all open files in native
 * code. Before opening or deleting any file, the isolate must verify if
 * the file is already open or not. The calling isolate will receive a
 * RecordStoreException if the file is already open.
 * </p>
 *
 * <p>
 * A linked list is maintained to store the locks for every
 * filenameBase/recordStoreName as follows:
 * </p>
 *
 * <p>
 * typedef struct _lockFileList { <br>
 *     &nbsp;&nbsp;&nbsp;pcsl_string filenameBase; <br>
 *     &nbsp;&nbsp;&nbsp;pcsl_string recordStoreName; <br>
 *     &nbsp;&nbsp;&nbsp;int handle; <br>
 *     &nbsp;&nbsp;&nbsp;_lockFileList* next; <br>
 *} lockFileList;
 * </p>
 *
 * <p>
 * lockFileList* lockFileListPtr;
 * </p>
 *
 * <p>
 * lockFileListPtr will always point to the head of the linked list.
 * </p>
 *
 * <ol>
 * <h3>
 * <li>
 * openRecordStore
 * </li>
 * </h3>
 *
 * <p>
 * The peer implementation of openRecordStore() [RecordStoreImpl
 * constructor] calls a native function, viz. openRecordStoreFile() in
 * RecordStoreFile.c. This function calls rmsdb_record_store_open() in rms.c.
 * The access to the recordstore file can be verified at this point.
 * </p>
 * <ol>
 * <li>
 * If the lockFileListPtr is NULL, the linked list is initialised and
 * a node is added to the lockFileList.
 * </li>
 *
 * <li>
 * If (lockFileListPtr != NULL) , the linked list is searched for
 * the node based on filenameBase and recordStoreName. If the node is found, it means
 * that the file is already opened by another isolate and a RecordStoreException
 * is thrown to the calling isolate.
 * </li>
 *
 * <li>
 * If (lockFileListPtr != NULL) and the node is not found for the
 * corresponding filenameBase and recordStoreName, then a new node will be
 * added to the linked list.
 * </li>
 * </ol>
 *
 * <h3>
 * <li>
 * closeRecordStore
 * </li>
 * </h3>
 *
 * <p>
 * In this case, a node is searched in the linked list based on filenameBase
 * and recordStoreName. The node is just removed from the linked list.
 * </p>
 *
 * <h3>
 * <li>
 * deleteRecordStore
 * </li>
 * </h3>
 *
 * <p>
 * In MVM mode, the record store file should not be deleted if it is in
 * use by another isolate.  A node is searched in the linked list based
 * on filenameBase and recordStoreName. If (lockFileListPtr != NULL) and a node
 * is found, a RecordStoreException in thrown to the calling isolate.
 * </p>
 *
 * <h3>
 * <li>
 * Important Notes
 * </li>
 * </h3>
 *
 * <ol>
 * <li>
 * The individual record operations like addRecord(), getRecord(),
 * setRecord() will not be affected due to the new locking mechanism.
 * </li>
 *
 * <li>
 * If the record store file is already in use by any isolate, the calling
 * isolate will just receive an exception. There won't be any retries or
 * waiting mechanism till the record store file becomes available.
 * </li>
 *
 * <li>
 * There is very less probability of introducing memory leaks with this
 * mechanism. In normal scenario, every isolate closes the file after it
 * is done with its read/write operations which will automatically flush
 * the memory for the node. Note that native finalizer for RecordStoreFile
 * also calls recordStoreClose() which in turn removes the node from
 * the linked list.
 * </li>
 * </ol>
 *
 * </ol>
 *
 ***/

#ifndef RMS_FILE_LOCK_H
#define RMS_FILE_LOCK_H

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

#ifdef __cplusplus
}
#endif

/*
 *! \struct lockFileList
 *
 * Maintain a linked list of nodes representing open files. If a node is present
 * in list, then it means that the file is opened by some isolate and no
 * other isolate can access it.
 *
 */
typedef struct _lockFileList {
    pcsl_string filenameBase;
    pcsl_string recordStoreName;
    int handle;      /* 32-bit file handle, useful in close operation */
    struct _lockFileList* next;
} lockFileList;

typedef lockFileList olockFileList;

/**
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
int rmsdb_create_file_lock(pcsl_string* filenameBase,
        const pcsl_string* name_str, int handle);

/** 
 * Delete a node from the linked list
 *
 * @param handle : handle of the file
 *
 */
void rmsdb_delete_file_lock(int handle);

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
lockFileList* rmsdb_find_file_lock_by_id(pcsl_string* filenameBase, 
        const pcsl_string* name);

#ifdef __cplusplus
}
#endif

#endif
