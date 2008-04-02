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

#include "kni.h"
#include "midp_file_cache.h"
#include <midpStorage.h>
#include <midpMalloc.h>
#include <midpInit.h>
#include <midpRMS.h>
#include <midpUtilKni.h>
#include <suitestore_rms.h>
#include <limits.h> /* for LONG_MAX */

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
 * suiteId/recordStoreName as follows:
 * </p>
 *
 * <p>
 * typedef struct _lockFileList { <br>
 *     &nbsp;&nbsp;&nbsp;pcsl_string suiteId; <br>
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
 * the node based on suiteId and recordStoreName. If the node is found, it means
 * that the file is already opened by another isolate and a RecordStoreException
 * is thrown to the calling isolate.
 * </li>
 *
 * <li>
 * If (lockFileListPtr != NULL) and the node is not found for the
 * corresponding suiteId and recordStoreName, then a new node will be
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
 * In this case, a node is searched in the linked list based on suiteId
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
 * on suiteId and recordStoreName. If (lockFileListPtr != NULL) and a node
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

/*
 *! \struct lockFileList
 *
 * Maintain a linked list of nodes representing open files. If a node is present
 * in list, then it means that the file is opened by some isolate and no
 * other isolate can access it.
 *
 */
typedef struct _lockFileList {
    SuiteIdType suiteId;
    pcsl_string recordStoreName;
    int handle;      //32-bit file handle, useful in close operation
    struct _lockFileList* next;
} lockFileList;
static lockFileList* lockFileListPtr = NULL;

typedef lockFileList olockFileList;

// Forward declarations for local functions

static int recordStoreCreateLock(SuiteIdType suiteId,
                                 const pcsl_string * name_str, int handle);
static void recordStoreDeleteLock(int handle);
static lockFileList * findLockById(SuiteIdType suiteId, const pcsl_string * name);


/**
 * A utility method that convert a hex character 0-9a-f to the
 * numerical value represented by this hex char.
 *
 * @param c the character to convert
 * @return the number represented by the character. E.g., '0' represents
 * the number 0x0, 'a' represents the number 0x0a, etc.
 */
static jchar
hexValue(jchar c) {
    if (c >= '0' && c <= '9') {
        return ((jchar)c) - '0';
    }

    return ((jchar)c) - 'a' + 10;
}

/**
 * Perform the reverse conversion of unicodeToEscapedAscii().
 *
 * @param str a string previously returned by escape()
 * @return the original string before the conversion by escape().
 */
static pcsl_string_status
escaped_ascii_to_unicode(const pcsl_string* str, pcsl_string* result) {
    int result_len=0;
    jchar* result_data = NULL;
    pcsl_string_status status = PCSL_STRING_OK;
    GET_PCSL_STRING_DATA_AND_LENGTH(str) do {
        int i;

        result_data = (jchar*)midpMalloc(str_len * sizeof (jchar));

        if (result_data == NULL) {
            status = PCSL_STRING_ENOMEM;
            break;
        }

        for (i = 0, result_len = 0; i < str_len; i++) {
            jchar c = str_data[i];

            if (c == '%') {
                jchar v = 0;

                v += hexValue(str_data[i+1]);
                v <<= 4;
                v += hexValue(str_data[i+2]);
                v <<= 4;
                v += hexValue(str_data[i+3]);
                v <<= 4;
                v += hexValue(str_data[i+4]);

                i += 4;

                result_data[result_len] = v;
                result_len++;
            } else if (c == '#') {
                /* drop c */
            } else {
                result_data[result_len] = c;
                result_len++;
            }
        }

    } while(0); RELEASE_PCSL_STRING_DATA_AND_LENGTH

    if (PCSL_STRING_OK == status) {
        if (PCSL_STRING_OK !=
                pcsl_string_convert_from_utf16(result_data, result_len, result)) {
            status = PCSL_STRING_ENOMEM;
        }
    }
    midpFree(result_data);
    return status;
}

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
 * @param suiteId ID of the MIDlet suite that owns the record store
 * @param storageId ID of the storage where the RMS will be located
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return a unique identifier for this record store file
 */
static MIDP_ERROR
rmsdb_get_unique_id_path(SuiteIdType suiteId, StorageIdType storageId,
                         const pcsl_string* name,
                         int extension, pcsl_string * res_path) {
    pcsl_string temp = PCSL_STRING_NULL;
    MIDP_ERROR midpErr;
    pcsl_string_status pcslErr;

    *res_path = PCSL_STRING_NULL; // null in case of any error

    if (pcsl_string_is_null(name)) {
        return MIDP_ERROR_ILLEGAL_ARGUMENT;
    }


    midpErr = midp_suite_get_rms_filename(suiteId, storageId,
                                          (extension == IDX_EXTENSION_INDEX
                                          ? MIDP_RMS_IDX_EXT : MIDP_RMS_DB_EXT),
                                          name, res_path);

    if (midpErr != MIDP_ERROR_NONE) {
        return midpErr;
    }

    if (pcsl_string_is_null(res_path)) {
        /* Assume this is special case where the suite was not installed
           and create a filename from the ID. */
        pcslErr = pcsl_string_cat(storage_get_root(storageId),
            midp_suiteid2pcsl_string(suiteId), &temp);
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
 * @param suiteId ID of the MIDlet suite that owns the record store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return true if the file exists, false if it does not.
 */
int rmsdb_record_store_exists(SuiteIdType suiteId,
                              const pcsl_string* name,
                              int extension) {
    pcsl_string filename;
    int intStatus;

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(suiteId,
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
 * @param suiteId ID of the MIDlet suite that owns the record store
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
                          SuiteIdType suiteId,
                          const pcsl_string* name_str,
                          int extension) {
    pcsl_string filename_str;
    lockFileList* searchedNodePtr = NULL;

    *ppszError = NULL;

    if ((extension == DB_EXTENSION_INDEX)&&(lockFileListPtr != NULL)) {
        /* linked list is already initialised for a db file */
        searchedNodePtr = findLockById(suiteId, name_str);
        if (searchedNodePtr != NULL) {
            /* File is in use by another isolate */
            *ppszError = (char *)FILE_LOCK_ERROR;
            return -1;
        }
    }

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(suiteId, INTERNAL_STORAGE_ID,
            name_str, extension, &filename_str)) {
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
 * @param suiteId ID of the MIDlet suite that owns the record store
 *
 * @return number of record stores or OUT_OF_MEM_LEN
 */
int
rmsdb_get_number_of_record_stores(SuiteIdType suiteId) {
    pcsl_string root = PCSL_STRING_NULL;
    int numberOfStores;
    MIDPError status;

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    status = midp_suite_get_rms_filename(suiteId, INTERNAL_STORAGE_ID, -1,
                                         &PCSL_STRING_EMPTY, &root);
    if (status != ALL_OK) {
      return OUT_OF_MEM_LEN;
    }

    if (root.data == NULL) {
        return 0;
    }

    numberOfStores = rmsdb_get_number_of_record_stores_int(&root);

    pcsl_string_free(&root);

    return numberOfStores;
}

/**
 * Returns an array of the names of record stores owned by the
 * MIDlet suite.
 *
 * @param suiteId
 * @param ppNames pointer to pointer that will be filled in with names
 *
 * @return number of record store names or OUT_OF_MEM_LEN
 */
int
rmsdb_get_record_store_list(SuiteIdType suiteId, pcsl_string* *const ppNames) {
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
    status = midp_suite_get_rms_filename(suiteId, INTERNAL_STORAGE_ID, -1,
                                         &PCSL_STRING_EMPTY, &root);
    if (status != ALL_OK) {
        return OUT_OF_MEM_LEN;
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
    for(i=0,f_errc=0,s_errc=0;;) {
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

            s_errc = escaped_ascii_to_unicode(&ascii_name, &pStores[i]);
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
 * @param id ID of the suite
 *
 * @return false if out of memory else true
 */
int
rmsdb_remove_record_stores_for_suite(SuiteIdType id) {
    int numberOfNames;
    pcsl_string* pNames;
    int i;
    int result = 1;
    char* pszError;

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

    numberOfNames = rmsdb_get_record_store_list(id, &pNames);
    if (numberOfNames == OUT_OF_MEM_LEN) {
        return 0;
    }

    if (numberOfNames <= 0) {
        return 1;
    }

    for (i = 0; i < numberOfNames; i++) {
        if (rmsdb_record_store_delete(&pszError, id, &pNames[i], DB_EXTENSION_INDEX) <= 0) {
            result = 0;
            break;
        }
        if (rmsdb_record_store_delete(&pszError, id, &pNames[i], IDX_EXTENSION_INDEX) <= 0) {
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
 * @param id ID of the suite
 *
 * @return true if the suite has at least one record store
 */
int
rmsdb_suite_has_rms_data(SuiteIdType id) {
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

    return (rmsdb_get_number_of_record_stores(id) > 0);
}

/**
 * Approximation of remaining RMS space in storage for a suite.
 *
 * Usage Warning:  This may be a slow operation if
 * the platform has to look at the size of each file
 * stored in the MIDP memory space and include its size
 * in the total.
 *
 * @param id ID of the suite
 *
 * @return the approximate space available to grow the
 *         record store in bytes.
 */
long
rmsdb_get_new_record_store_space_available(SuiteIdType id) {
    return rmsdb_get_record_store_space_available(-1, id);
}


/**
 * Open a native record store file.
 *
 * @param ppszError where to put an I/O error
 * @param suiteId ID of the MIDlet suite that owns the record store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return storage handle on success
 *  return -1 if there is an error getting the filename
 *  return -2 if the file is locked
 */

int
rmsdb_record_store_open(char** ppszError, SuiteIdType suiteId,
                        const pcsl_string * name_str, int extension) {
    pcsl_string filename_str;
    int handle;
    lockFileList* searchedNodePtr = NULL;
    int addflag = 0;

    *ppszError = NULL;

    if ((extension == DB_EXTENSION_INDEX) && (lockFileListPtr != NULL)) {
        /* linked list is already initialised for a db file */
        searchedNodePtr = findLockById(suiteId, name_str);
        if (searchedNodePtr != NULL) {
            /* File is already opened by another isolate, return an error */
            *ppszError = (char *)FILE_LOCK_ERROR;
            return -2;
        } else { /* remember to add a node */
            addflag = 1;
        }
    }

    /*
     * IMPL_NOTE: for security reasons the record store is always
     * located in the internal storage.
     */
    if (MIDP_ERROR_NONE != rmsdb_get_unique_id_path(suiteId, INTERNAL_STORAGE_ID,
            name_str, extension, &filename_str)) {
        return -1;
    }
    handle = midp_file_cache_open(ppszError, INTERNAL_STORAGE_ID,
                                  &filename_str, OPEN_READ_WRITE);

    pcsl_string_free(&filename_str);
    if (*ppszError != NULL) {
        return -1;
    }

    /*
     * Add the node only if it's a db file AND lockFileListPtr is NULL or
     * addflag is 1
     */
    if ((extension == DB_EXTENSION_INDEX)&&
        ( (lockFileListPtr == NULL) || (addflag == 1)) ) {
            if (recordStoreCreateLock(suiteId, name_str, handle) != 0) {
                return -1;
            }
    }

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
 * @param id ID of the suite
 *
 * @return the approximate space available to grow the
 *         record store in bytes.
 */
long
rmsdb_get_record_store_space_available(int handle, SuiteIdType id) {
    /* Storage may have more then 2Gb space available so use 64-bit type */
    jlong availSpace;
    long availSpaceUpTo2Gb;
    char* pszError;
    StorageIdType storageId;
    MIDPError status;

    (void)id; /* Avoid compiler warnings */

    /*
     * IMPL_NOTE: for security reasons we introduce a limitation that the
     * suite's RMS must be located at the internal storage.
     * Note that the public RecordStore API doesn't support
     * a storageId parameter.
     * There is a plan to get rid of such limitation by introducing a
     * function that will return a storage ID by the suite ID and RMS name.
     */
    status = midp_suite_get_suite_storage(id, &storageId);
    if (status != ALL_OK) {
        return 0; /* Error: report that no space is available */
    }

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

    /*
     * Search the node based on handle. Delete the node upon file close
     */

     recordStoreDeleteLock(handle);
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
 * @param suiteId  ID of the suite
 *
 * @return number of bytes of storage the suite is using or OUT_OF_MEM_LEN
 */
long
rmsdb_get_rms_storage_size(SuiteIdType suiteId) {
    int numberOfNames;
    pcsl_string* pNames;
    int i;
    int used = 0;
    int handle;
    char* pszError;
    char* pszTemp;

    numberOfNames = rmsdb_get_record_store_list(suiteId, &pNames);
    if (numberOfNames == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    if (numberOfNames == 0) {
        return 0;
    }

    for (i = 0; i < numberOfNames; i++) {
        handle = rmsdb_record_store_open(&pszError, suiteId, &pNames[i],
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

/*
 * Insert a new node to the front of the linked list
 *
 * @param suiteId : ID of the suite
 * @param name : Name of the record store
 * @param handle : Handle of the opened file
 *
 * @return 0 if node is successfully inserted
 *  return  OUT_OF_MEM_LEN if memory allocation fails
 *
 */
static int
recordStoreCreateLock(SuiteIdType suiteId, const pcsl_string * name_str,
                      int handle) {
    lockFileList* newNodePtr;

    newNodePtr = (lockFileList *)midpMalloc(sizeof(lockFileList));
    if (newNodePtr == NULL) {
        return OUT_OF_MEM_LEN;
    }

    newNodePtr->suiteId = suiteId;

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
static void
recordStoreDeleteLock(int handle) {
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
 * @param suiteId : ID of the suite
 * @param name : Name of the record store
 *
 * @return the searched node pointer if match exist for suiteId and
 * recordStoreName
 *  return NULL if node does not exist
 *
 */
static lockFileList *
findLockById(SuiteIdType suiteId, const pcsl_string * name_str) {
    lockFileList* currentNodePtr;

    for (currentNodePtr = lockFileListPtr; currentNodePtr != NULL;
         currentNodePtr = currentNodePtr->next) {
        if (currentNodePtr->suiteId == suiteId &&
            pcsl_string_equals(&currentNodePtr->recordStoreName, name_str)) {
            return currentNodePtr;
        }
    }

    return NULL;
}
