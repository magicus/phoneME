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

#include <string.h>

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpServices.h>
#include <midpStorage.h>
#include <midpJar.h>
#include <suitestore_task_manager.h>
#include <midp_constants_data.h>
#include <midpUtilKni.h>
#include <fileCache.h>

/**
 * @file
 *
 * Implements supporting methods for native image and font caches
 */


/**
 * Remove all the cached files associated with a suite.
 *
 * @param suiteId   Suite ID
 * @param storageId ID of the storage where to create the cache
*/
void deleteFileCache(SuiteIdType suiteId, StorageIdType storageId) {
    pcsl_string root;
    pcsl_string filename;
    char*  pszError;
    void*  handle = NULL;
    jint errorCode;

    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    errorCode = midp_suite_get_cached_resource_filename(suiteId, storageId,
                                                        &PCSL_STRING_EMPTY,
                                                        &root);
    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    handle = storage_open_file_iterator(&root);
    if (handle == NULL) {
        pcsl_string_free(&root);
        return;
    }

    /* Delete all files that start with suite Id and end with TMP_EXT */
    for (;;) {
        if (0 != storage_get_next_file_in_iterator(&root, handle, &filename)) {
            break;
        }
        if (pcsl_string_ends_with(&filename, &TMP_EXT)) {
            storage_delete_file(&pszError, &filename);
            if (pszError != NULL) {
                storageFreeError(pszError);
            }
        }

        pcsl_string_free(&filename);
    }

    storageCloseFileIterator(handle);
    pcsl_string_free(&root);
}


/**
 * Moves cached files from ome storage to another.
 * For security reasons we allow to move cache only to the
 * internal storage.
 *
 * @param suiteId The suite ID
 * @param storageIdFrom ID of the storage where files are cached
 * @param storageIdTo ID of the storage where to move the cache
 */
void moveFileCache(SuiteIdType suiteId, StorageIdType storageIdFrom,
                    StorageIdType storageIdTo) {
    pcsl_string root;
    pcsl_string filePath;
    char*  pszError;
    void*  handle = NULL;
    jint errorCode;
    jsize oldRootLength;
    jsize newRootLength;
    const pcsl_string* newRoot;


    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    /*
     * IMPL_NOTE: for security reasons we allow to move cache
     * only to the internal storage.
     */
    if (storageIdTo != INTERNAL_STORAGE_ID) {
        return;
    }

    errorCode = midp_suite_get_cached_resource_filename(suiteId, storageIdFrom,
                                                        &PCSL_STRING_EMPTY,
                                                        &root);
    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    handle = storage_open_file_iterator(&root);
    if (handle == NULL) {
        pcsl_string_free(&root);
        return;
    }

    newRoot = storage_get_root(storageIdTo);
    newRootLength = pcsl_string_length(newRoot);
    oldRootLength = pcsl_string_length(storage_get_root(storageIdFrom));

    /*
     * Move all files that start with suite Id and end with
     * TMP_EXT to new storage.
     */
    for (;;) {
        pcsl_string fileName;
        pcsl_string newFilePath = PCSL_STRING_NULL;
        jsize filePathLength;
        
        if (0 != storage_get_next_file_in_iterator(&root, handle, &filePath)) {
            break;
        }
        if (pcsl_string_ends_with(&filePath, &TMP_EXT)) {
            /* construct new file name. */
            filePathLength = pcsl_string_length(&filePath);
            pcsl_string_predict_size(&fileName, filePathLength - oldRootLength);
            if (PCSL_STRING_OK != pcsl_string_substring(&filePath,
                    oldRootLength, filePathLength, &fileName)) {
                break;
            }
            pcsl_string_predict_size(&newFilePath,
                newRootLength + pcsl_string_length(&fileName));
            if (PCSL_STRING_OK != pcsl_string_append(&newFilePath, newRoot)) {
                pcsl_string_free(&fileName);
                break;
            }
            if (PCSL_STRING_OK != pcsl_string_append(&newFilePath,
                    (const pcsl_string*)&fileName)) {
                pcsl_string_free(&fileName);
                pcsl_string_free(&newFilePath);
                break;
            }
            /* rename file. */
            storage_rename_file(&pszError, &filePath, &newFilePath);
            pcsl_string_free(&fileName);
            pcsl_string_free(&newFilePath);
            
            if (pszError != NULL) {
                storageFreeError(pszError);
            }
        }

        pcsl_string_free(&filePath);
    }

    storageCloseFileIterator(handle);
    pcsl_string_free(&root);
}


/**
 * Store a file to cache.
 *
 * @param suiteId    The suite id
 * @param storageId ID of the storage where to create the cache
 * @param resName    The file resource name
 * @param bufPtr     The buffer with the data
 * @param len        The length of the buffer
 * @return           1 if successful, 0 if not
 */
int storeFileToCache(SuiteIdType suiteId, StorageIdType storageId,
                             const pcsl_string * resName,
                             unsigned char *bufPtr, int len) {

    int            handle = -1;
    char*          errmsg = NULL;
    pcsl_string    path;
    int status     = 1;
    int errorCode;

    if (suiteId == UNUSED_SUITE_ID || pcsl_string_is_null(resName) ||
        (bufPtr == NULL)) {
        return 0;
    }

    /* Build the complete path */
    errorCode = midp_suite_get_cached_resource_filename(suiteId,
                                                        storageId,
                                                        resName,
                                                        &path);

    if (errorCode != MIDP_ERROR_NONE) {
        return 0;
    }

    /* Open the file */
    handle = storage_open(&errmsg, &path, OPEN_READ_WRITE_TRUNCATE);
    if (errmsg != NULL) {
        REPORT_WARN1(LC_LOWUI,"Warning: could not open file cache; %s\n",
		     errmsg);
        pcsl_string_free(&path);
	storageFreeError(errmsg);
	return 0;
    }

    /* Finally write the file */
    storageWrite(&errmsg, handle, (char*)bufPtr, len);
    if (errmsg != NULL) {
      REPORT_WARN1(LC_LOWUI,"Warning: could not save cached file; %s\n",
		     errmsg);
      status = 0;
      storageFreeError(errmsg);
    }

    storageClose(&errmsg, handle);
    if (errmsg != NULL) {
        storageFreeError(errmsg);    
    }
    
    if (status == 0) {
        /* Delete the file cach file to keep away from corrupted data */
        storage_delete_file(&errmsg, &path);
        if (errmsg != NULL) {
            storageFreeError(errmsg);
        }
    }

    pcsl_string_free(&path);

    return status;
}


/**
 * Loads file from cache, if present.
 *
 * @param suiteId    The suite id
 * @param resName    The file resource name
 * @param **bufPtr   Pointer where a buffer will be allocated and data stored
 * @return           -1 if failed, else length of buffer
 */
int loadFileFromCache(SuiteIdType suiteId, const pcsl_string * resName,
                       unsigned char **bufPtr) {

    int                len = -1;
    int                handle;
    char*              errmsg = NULL;
    int                bytesRead;
    pcsl_string        path;
    pcsl_string        resNameFix;
    MIDPError          errorCode;
    StorageIdType      storageId;
    pcsl_string_status res;

    if (suiteId == UNUSED_SUITE_ID || pcsl_string_is_null(resName)) {
        return len;
    }

    /* If resource starts with slash, remove it */
    if (pcsl_string_index_of(resName, (jint)'/') == 0) {
        jsize resNameLen = pcsl_string_length(resName);
        res = pcsl_string_substring(resName, 1, resNameLen, &resNameFix);
    } else {
        res = pcsl_string_dup(resName, &resNameFix);
    }
    if (PCSL_STRING_OK != res) {
        return len;
    }

    /*
     * IMPL_NOTE: here is assumed that the file cache is located in
     * the same storage as the midlet suite. This may not be true.
     */
    /* Build path */
    errorCode = midp_suite_get_suite_storage(suiteId, &storageId);
    if (errorCode != ALL_OK) {
        return len;
    }

    errorCode = midp_suite_get_cached_resource_filename(suiteId,
        storageId, &resNameFix, &path);
    pcsl_string_free(&resNameFix);
    if (errorCode != ALL_OK) {
        return len;
    }

    /* Open file */
    handle = storage_open(&errmsg, &path, OPEN_READ);
    pcsl_string_free(&path);
    if (errmsg != NULL) {
        REPORT_WARN1(LC_LOWUI,"Warning: could not load cached file; %s\n",
                     errmsg);

        storageFreeError(errmsg);
        return len;
    }

    do {
        /* Get size of file and allocate buffer */
        len = storageSizeOf(&errmsg, handle);
        *bufPtr = midpMalloc(len);
        if (*bufPtr == NULL) {
            len = -1;
            break;
        }

        /* Read data */
        bytesRead = storageRead(&errmsg, handle, (char*)*bufPtr, len);
        if (errmsg != NULL) {
            len = -1;
            midpFree(*bufPtr);
            storageFreeError(errmsg);
            break;
        }

        /* Sanity check */
        if (bytesRead != len) {
            len = -1;
            midpFree(*bufPtr);
            break;
        }

    } while (0);

    storageClose(&errmsg, handle);

    return len;
}
