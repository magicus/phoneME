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

#if ENABLE_RECORDSTORE_FILE_LOCK
#include "rms_file_lock.h"
#endif


/** Easily recognize record store files in the file system */
static const int DB_EXTENSION_INDEX = 0;
static const int IDX_EXTENSION_INDEX = 1;

/*
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START( DB_EXTENSION )
    {'.', 'd', 'b', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END( DB_EXTENSION );

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START( IDX_EXTENSION )
    {'.', 'i', 'd', 'x', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END( IDX_EXTENSION );
*/

static const char* const FILE_LOCK_ERROR = "File is locked, can not open";


/* Forward declarations for local functions */

static MIDPError buildSuiteFilename(pcsl_string * filenameBase,
                                    const pcsl_string* name, jint extension, 
                                    pcsl_string * pFileName);

/**
 * Returns a storage system unique string for this record store file
 * based on the current vendor and suite of the running MIDlet.
 * <ul>
 *  <li>The native storage path for the desired MIDlet suite
 *  is acquired from the Scheduler.
 *
 *  <li>The <code>filename</code> arg is converted into an ascii
 *  equivalent safe to use directly in the underlying
 *  file system and appended to the native storage path.  See the
 *  com.sun.midp.io.j2me.storage.File.unicodeToAsciiFilename()
 *  method for conversion details.
 *
 *  <li>Finally the extension number given by the extension parameter
 *  is appended to the file name.
 * <ul>
 * @param filenameBase filename base of the MIDlet suite that owns the record 
 * store
 * @param storageId ID of the storage where the RMS will be located
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return a unique identifier for this record store file
 */
static MIDP_ERROR
rmsdb_get_unique_id_path(pcsl_string* filenameBase, StorageIdType storageId,
                         const pcsl_string* name,
                         int extension, pcsl_string * res_path) {
    pcsl_string temp = PCSL_STRING_NULL;
    MIDP_ERROR midpErr;
    pcsl_string_status pcslErr;

    *res_path = PCSL_STRING_NULL; /* null in case of any error */

    if (pcsl_string_is_null(name)) {
        return MIDP_ERROR_ILLEGAL_ARGUMENT;
    }

    midpErr = buildSuiteFilename(filenameBase, name, extension == IDX_EXTENSION_INDEX
                                          ? MIDP_RMS_IDX_EXT : MIDP_RMS_DB_EXT,
                                          res_path);

    if (midpErr != MIDP_ERROR_NONE) {
        return midpErr;
    }

    if (pcsl_string_is_null(res_path)) {
        /* Assume this is special case where the suite was not installed
           and create a filename from the ID. */
        pcslErr = pcsl_string_cat(storage_get_root(storageId),
            filenameBase, &temp);
        if (pcslErr != PCSL_STRING_OK || pcsl_string_is_null(&temp) ) {
            return MIDP_ERROR_FOREIGN;
        }
        pcslErr = pcsl_string_cat(&temp, name, res_path);
        pcsl_string_free(&temp);
        if (PCSL_STRING_OK != pcslErr)
        {
            return MIDP_ERROR_FOREIGN;
        }
    }
    return MIDP_ERROR_NONE;
}

/**
 * Looks to see if the storage file for record store
 * identified by <code>uidPath</code> exists
 *
 * @param filenameBase filenameBase of the MIDlet suite that owns the record 
 * store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return true if the file exists, false if it does not.
 */
int rmsdb_record_store_exists(pcsl_string* filenameBase,
                              const pcsl_string* name,
                              int extension) {
    pcsl_string filename;
    int intStatus;

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(filenameBase,
            INTERNAL_STORAGE_ID, name, extension, &filename)) {
        return 0;
    }
    if (pcsl_string_is_null(&filename)) {
        return 0;
    }

    intStatus = storage_file_exists(&filename);
    pcsl_string_free(&filename);

    return 0 != intStatus;
}

/**
 * Removes the storage file for record store <code>filename</code>
 * if it exists.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param filenameBase filenameBase of the MIDlet suite that owns the record 
 * store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return 1 if successful
 *         0 if an IOException occurred
 *        -1 if file is locked by another isolate
 *        -2 if out of memory error occurs
 *
 */
int
rmsdb_record_store_delete(char** ppszError,
                          pcsl_string* filenameBase,
                          const pcsl_string* name_str,
                          int extension) {
    pcsl_string filename_str;
#if ENABLE_RECORDSTORE_FILE_LOCK
    lockFileList* searchedNodePtr = NULL;
#endif

    *ppszError = NULL;

#if ENABLE_RECORDSTORE_FILE_LOCK
    if (extension == DB_EXTENSION_INDEX) {
        searchedNodePtr = rmsdb_find_file_lock_by_id(filenameBase, name_str);
        if (searchedNodePtr != NULL) {
            /* File is in use by another isolate */
            *ppszError = (char *)FILE_LOCK_ERROR;
            return -1;
        }
    }
#endif

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(filenameBase, 
                INTERNAL_STORAGE_ID, name_str, extension, &filename_str)) {
        return -2;
    }
    storage_delete_file(ppszError, &filename_str);

    pcsl_string_free(&filename_str);

    if (*ppszError != NULL) {
        return 0;
    }

    return 1;
}

/**
 * Returns the number of record stores owned by the
 * MIDlet suite.
 *
 * @param root storage root a MIDlet suite
 *
 * @return number of record stores or OUT_OF_MEM_LEN
 */
static int
rmsdb_get_number_of_record_stores_int(const pcsl_string* root) {
    pcsl_string filename;
    int numberOfStores = 0;
    void* handle = NULL;
    int errc = 0; /* 0 for ok, -1 for error -- see pcsl docs */

    handle = storage_open_file_iterator(root);
    if (!handle) {
        return OUT_OF_MEM_LEN;
    }

    for(;;) {
        errc = storage_get_next_file_in_iterator(root, handle, &filename);
        if ( 0 != errc ) {
            break;
        }
        if (pcsl_string_ends_with(&filename, &DB_EXTENSION)) {
            numberOfStores++;
        }
        pcsl_string_free(&filename);
    }

    storageCloseFileIterator(handle);

    return numberOfStores;
}

/**
 * Returns the number of record stores owned by the
 * MIDlet suite.
 *
 * @param filenameBase filenameBase of the MIDlet suite that owns the record 
 * store
 *
 * @return number of record stores or OUT_OF_MEM_LEN
 */
int
rmsdb_get_number_of_record_stores(pcsl_string* filenameBase) {
    int numberOfStores;

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (filenameBase->data == NULL) {
        return 0;
    }

    numberOfStores = rmsdb_get_number_of_record_stores_int(filenameBase);

    return numberOfStores;
}

/**
 * Returns an array of the names of record stores owned by the
 * MIDlet suite.
 *
 * @param filenameBase filenameBase of the suite
 * @param ppNames pointer to pointer that will be filled in with names
 *
 * @return number of record store names or OUT_OF_MEM_LEN
 */
int
rmsdb_get_record_store_list(pcsl_string* filenameBase, pcsl_string* *const ppNames) {
    int numberOfStores;
    pcsl_string root;
    pcsl_string* pStores;
    pcsl_string filename;
    pcsl_string ascii_name = PCSL_STRING_NULL_INITIALIZER;
    int i;
    void* handle = NULL;
    MIDPError status;
    int f_errc;
    pcsl_string_status s_errc;
    /* IMPL_NOTE: how can we get it statically? */
    const int dbext_len = pcsl_string_length(&DB_EXTENSION);

    *ppNames = NULL;

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    status = buildSuiteFilename(filenameBase, &PCSL_STRING_EMPTY, -1, 
                                 &root);

    if (status != MIDP_ERROR_NONE) {
        return status;
    }

    if (pcsl_string_is_null(&root)) {
        return 0;
    }

    numberOfStores = rmsdb_get_number_of_record_stores_int(&root);
    if (numberOfStores <= 0) {
        pcsl_string_free(&root);
        return numberOfStores;
    }
    pStores = alloc_pcsl_string_list(numberOfStores);
    if (pStores == NULL) {
        pcsl_string_free(&root);
        return OUT_OF_MEM_LEN;
    }

    handle = storage_open_file_iterator(&root);
    if (!handle) {
        pcsl_string_free(&root);
        return OUT_OF_MEM_LEN;
    }

    /* the main loop */
    for (i=0,f_errc=0,s_errc=0;;) {
        f_errc = storage_get_next_file_in_iterator(&root, handle, &filename);
        if (0 != f_errc) {
            f_errc = 0;
            break;
        }
        if (pcsl_string_ends_with(&filename, &DB_EXTENSION)) {
            s_errc =
              pcsl_string_substring(&filename,
                                    pcsl_string_length(&root),
                                    pcsl_string_length(&filename)
                                        - dbext_len,
                                    &ascii_name);
            pcsl_string_free(&filename);

            if (PCSL_STRING_OK != s_errc ) {
                break;
            }

            s_errc = pcsl_esc_extract_attached(0, &ascii_name, &pStores[i]);
            pcsl_string_free(&ascii_name);
            if (PCSL_STRING_OK != s_errc ) {
                break;
            }
            i++;
        }

        pcsl_string_free(&filename);
        /* IMPL_NOTE: do we need this one? isn't it useless? */
        if (i == numberOfStores) {
            break;
        }
    }

    pcsl_string_free(&root);
    storageCloseFileIterator(handle);

    if (f_errc || s_errc) {
        /* The loop stopped because we ran out of memory. */
        free_pcsl_string_list(pStores, i);
        return OUT_OF_MEM_LEN;
    }
    *ppNames = pStores;
    return numberOfStores;
}

/**
 * Remove all the Record Stores for a suite.
 *
 * @param filenameBase filenameBase of the suite
 * @param id ID of the suite
 * Only one of the parameters will be used by a given implementation.  In the
 * case where the implementation might store data outside of the MIDlet storage,
 * the filenameBase will be ignored and only the suite id will be pertinent.
 *
 * @return false if out of memory else true
 */
int
rmsdb_remove_record_stores_for_suite(pcsl_string* filenameBase, SuiteIdType id) {
    int numberOfNames;
    pcsl_string* pNames;
    int i;
    int result = 1;
    char* pszError;
    (void)id; /* avoid a compiler warning */

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return 0;
    }

    numberOfNames = rmsdb_get_record_store_list(filenameBase, &pNames);
    if (numberOfNames == OUT_OF_MEM_LEN) {
        return 0;
    }

    if (numberOfNames <= 0) {
        return 1;
    }

    for (i = 0; i < numberOfNames; i++) {
        if (rmsdb_record_store_delete(&pszError, filenameBase, &pNames[i], 
            DB_EXTENSION_INDEX) <= 0) {
            result = 0;
            break;
        }
        if (rmsdb_record_store_delete(&pszError, filenameBase, &pNames[i], 
            IDX_EXTENSION_INDEX) <= 0) {
            /*
         * Since index file is optional, ignore error here.
         *
        result = 0;
            break;
        */
        }
    }

    recordStoreFreeError(pszError);
    free_pcsl_string_list(pNames, numberOfNames);

    return result;
}

/**
 * Returns true if the suite has created at least one record store.
 *
 * @param filenameBase filenameBase of the suite
 *
 * @return true if the suite has at least one record store
 */
int
rmsdb_suite_has_rms_data(pcsl_string* filenameBase) {
    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return 0;
    }

    return (rmsdb_get_number_of_record_stores(filenameBase) > 0);
}

/**
 * Approximation of remaining RMS space in storage for a suite.
 *
 * Usage Warning:  This may be a slow operation if
 * the platform has to look at the size of each file
 * stored in the MIDP memory space and include its size
 * in the total.
 *
 * @param storageId storage id of the suite
 *
 * @return the approximate space available to grow the
 *         record store in bytes.
 */
long
rmsdb_get_new_record_store_space_available(int storageId) {
    return rmsdb_get_record_store_space_available(-1, storageId);
}


/**
 * Open a native record store file.
 *
 * @param ppszError where to put an I/O error
 * @param filenameBase filenameBase of the MIDlet suite that owns the record 
 * store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return storage handle on success
 *  return -1 if there is an error getting the filename
 *  return -2 if the file is locked
 */

int
rmsdb_record_store_open(char** ppszError, pcsl_string* filenameBase,
                        const pcsl_string * name_str, int extension) {
    pcsl_string filename_str;
    int handle;
#if ENABLE_RECORDSTORE_FILE_LOCK
    lockFileList* searchedNodePtr = NULL;
#endif

    *ppszError = NULL;

#if ENABLE_RECORDSTORE_FILE_LOCK
    if (extension == DB_EXTENSION_INDEX) {
        searchedNodePtr = rmsdb_find_file_lock_by_id(filenameBase, name_str);
        if (searchedNodePtr != NULL) {
            /* File is already opened by another isolate, return an error */
            *ppszError = (char *)FILE_LOCK_ERROR;
            return -2;
        }
    }
#endif

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(filenameBase, 
                INTERNAL_STORAGE_ID, name_str, extension, &filename_str)) {
        return -1;
    }
    handle = midp_file_cache_open(ppszError, INTERNAL_STORAGE_ID,
                                  &filename_str, OPEN_READ_WRITE);

    pcsl_string_free(&filename_str);
    if (*ppszError != NULL) {
        return -1;
    }

#if ENABLE_RECORDSTORE_FILE_LOCK
    if (extension == DB_EXTENSION_INDEX) {
        if (rmsdb_create_file_lock(filenameBase, name_str, handle) != 0) {
            return -1;
        }
    }
#endif    

    return handle;
}

/**
 * Approximation of remaining RMS space in storage for a suite.
 *
 * Usage Warning:  This may be a slow operation if
 * the platform has to look at the size of each file
 * stored in the MIDP memory space and include its size
 * in the total.
 *
 * @param handle handle to record store storage
 * @param pStorageId storage id of the suite
 *
 * @return the approximate space available to grow the
 *         record store in bytes.
 */
long
rmsdb_get_record_store_space_available(int handle, int pStorageId) {
    /* Storage may have more then 2Gb space available so use 64-bit type */
    jlong availSpace;
    long availSpaceUpTo2Gb;
    char* pszError;
    StorageIdType storageId;

    /*
     * IMPL_NOTE: for security reasons we introduce a limitation that the
     * suite's RMS must be located at the internal storage.
     * Note that the public RecordStore API doesn't support
     * a storageId parameter.
     * There is a plan to get rid of such limitation by introducing a
     * function that will return a storage ID by the suite ID and RMS name.
     */
    storageId = pStorageId;

    availSpace = midp_file_cache_available_space(&pszError, handle, storageId);

    /*
     * Public RecordStore API uses Java int type for the available space
     * so here we trim the real space to 2Gb limit.
     */
    availSpaceUpTo2Gb = (availSpace <= LONG_MAX) ? availSpace : LONG_MAX;

    return availSpaceUpTo2Gb;
}

/**
 * Change the read/write position of an open file in storage.
 * The position is a number of bytes from beginning of the file.
 * Does not block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 * @param pos position within the file to move the current_pos
 *        pointer to.
 */
void recordStoreSetPosition(char** ppszError, int handle, int pos) {
    midp_file_cache_seek(ppszError, handle, pos);
}

/**
 * Write to an open file in storage. Will write all of the bytes in the
 * buffer or pass back an error. Does not block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 * @param buffer buffer to read out of.
 * @param length the number of bytes to write.
 */
void
recordStoreWrite(char** ppszError, int handle, char* buffer, long length) {
    midp_file_cache_write(ppszError, handle, buffer, length);
}

/**
 * Commit pending writes
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 */
void
recordStoreCommitWrite(char** ppszError, int handle) {
    midp_file_cache_flush(ppszError, handle);
}

/**
 * Read from an open file in storage, returning the number of bytes read or
 * -1 for the end of the file. May read less than the length of the buffer.
 * Does not block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 * @param buffer buffer to read in to.
 * @param length the number of bytes to read.
 *
 * @return the number of bytes read.
 */
long
recordStoreRead(char** ppszError, int handle, char* buffer, long length) {
    return midp_file_cache_read(ppszError, handle, buffer, length);
}

/**
 * Close a storage object opened by rmsdb_record_store_open. Does no block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 *
 */
void
recordStoreClose(char** ppszError, int handle) {

    midp_file_cache_close(ppszError, handle);
    /*
     * Verify that there is no error in closing the file. In case of any errors
     * there is no need to remove the node from the linked list
     */
     if (*ppszError != NULL) {
         return;
    }

#if ENABLE_RECORDSTORE_FILE_LOCK
    /*
     * Search the node based on handle. Delete the node upon file close
     */

     rmsdb_delete_file_lock(handle);
#endif     
}

/*
 * Truncate the size of an open file in storage.  Does not block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @param ppszError where to put an I/O error
 * @param handle handle to record store storage
 * @param size new size of the file
 */
void
recordStoreTruncate(char** ppszError, int handle, long size) {
    midp_file_cache_truncate(ppszError, handle, size);
}

/*
 * Free the error string returned from a record store function.
 * Does nothing if a NULL is passed in.
 * This allows for systems that provide error messages that are allocated
 * dynamically.
 *
 * @param pszError an I/O error
 */
void
recordStoreFreeError(char* pszError) {
    if (pszError != FILE_LOCK_ERROR) {
        storageFreeError(pszError);
    }
}

/*
 * Return the size of an open record store. Does not block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
long
recordStoreSizeOf(char** ppszError, int handle) {
    return midp_file_cache_sizeof(ppszError, handle);
}

/**
 * Gets the amount of RMS storage on the device that this suite is using.
 *
 * @param filenameBase filenameBase of the suite
 * @param id ID of the suite
 * Only one of the parameters will be used by a given implementation.  In the
 * case where the implementation might store data outside of the MIDlet storage,
 * the filenameBase will be ignored and only the suite id will be pertinent.
 *
 * @return number of bytes of storage the suite is using or OUT_OF_MEM_LEN
 */
long
rmsdb_get_rms_storage_size(pcsl_string* filenameBase, SuiteIdType id) {
    int numberOfNames;
    pcsl_string* pNames;
    int i;
    int used = 0;
    int handle;
    char* pszError;
    char* pszTemp;
    (void)id; /* avoid a compiler warning */

    numberOfNames = rmsdb_get_record_store_list(filenameBase, &pNames);
    if (numberOfNames == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    if (numberOfNames == 0) {
        return 0;
    }

    for (i = 0; i < numberOfNames; i++) {
        handle = rmsdb_record_store_open(&pszError, filenameBase, &pNames[i],
                                         DB_EXTENSION_INDEX);
        if (pszError != NULL) {
            recordStoreFreeError(pszError);
            break;
        }

        if (handle == -1) {
            break;
        }

        used += recordStoreSizeOf(&pszError, handle);
        recordStoreFreeError(pszError);

        recordStoreClose(&pszTemp, handle);
        recordStoreFreeError(pszTemp);

        if (pszError != NULL) {
            break;
        }
    }

    free_pcsl_string_list(pNames, numberOfNames);

    return used;
}


/* Utility function to build filename from filename base, name and extension
 * 
 * @param filenameBase base for the filename
 * @param name name of record store
 * @param extension rms extension that can be MIDP_RMS_DB_EXT or
 * MIDP_RMS_IDX_EXT
 *
 * @return the filename
 */

static MIDPError buildSuiteFilename(pcsl_string* filenameBase, 
                                    const pcsl_string* name, jint extension, 
                                    pcsl_string* pFileName) {

    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string rmsFileName = PCSL_STRING_NULL;

    jsize filenameBaseLen = pcsl_string_length(filenameBase);
    jsize nameLen = pcsl_string_length(name);

    *pFileName = PCSL_STRING_NULL;

    if (nameLen > 0) {
        const pcsl_string* ext;
        jsize extLen;
        int fileNameLen;

        if (MIDP_RMS_IDX_EXT == extension) {
            ext = &IDX_EXTENSION;
            extLen = pcsl_string_length(&IDX_EXTENSION);
        } else if (MIDP_RMS_DB_EXT == extension) {
            ext = &DB_EXTENSION;
            extLen = pcsl_string_length(&DB_EXTENSION);
        } else {
            return BAD_PARAMS;
        }

        /* performance hint: predict buffer capacity */
        fileNameLen = PCSL_STRING_ESCAPED_BUFFER_SIZE(nameLen + extLen);
        pcsl_string_predict_size(&rmsFileName, fileNameLen);

        if (pcsl_esc_attach_string(name, &rmsFileName) !=
                PCSL_STRING_OK ||
                    pcsl_string_append(&rmsFileName, ext) != PCSL_STRING_OK) {
            pcsl_string_free(&rmsFileName);
            return OUT_OF_MEMORY;
        }
    }

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&returnPath, filenameBaseLen + 
                             pcsl_string_length(&rmsFileName));

    if (PCSL_STRING_OK != pcsl_string_append(&returnPath, filenameBase) ||
            PCSL_STRING_OK != pcsl_string_append(&returnPath, &rmsFileName)) {
        pcsl_string_free(&rmsFileName);
        pcsl_string_free(&returnPath);
        return OUT_OF_MEMORY;
    }

    pcsl_string_free(&rmsFileName);
   *pFileName = returnPath;

    return ALL_OK;
}

