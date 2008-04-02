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

/**
 * @file
 *
 * @ingroup AMS
 *
 * This is reference implementation of the internal MIDlet suite storage
 * functions.
 * <p>
 * <h3>Common String List Files</h3>
 * <p>
 * A suite has more state than what is in the JAD or JAR, to hold this state
 * there a multiple files that contain lists of string values, with a string
 * key. The format of the string list file is a common Unicode-string file
 * format, for any file that is a series of Unicode strings. The format of
 * the file is binary and is written without byte conversion. These decisions
 * save code. Byte conversion can be omitted because the file is never copied
 * between devices.
 * <p>
 * The structure of the file is one native integer (int) for the number of
 * strings, followed by the data. The data consists of, for each Unicode
 * string, a Java programming language int for the number characters,
 * followed by the Java programming language characters themselves.
 * <p>
 * <h3>Suite Storage Files</h3>
 * <p>
 * When a suite is stored the files or record in a file listed below are
 * created.
 * <p>
 * <table border=2>
 * <tr>
 *   <th>File Type</th>
 *   <th>Description</th>
 * </tr>
 * <tr>
 *   <td>JAR</td>
 *   <td>The JAR file downloaded for the suite. This file is created by the
 *       installer and renamed when it is stored.</td>
 * </tr>
 * <tr>
 *   <td>Application Properties</td>
 *   <td>JAD properties file followed by the manifest properties of the MIDlet
 *       suite. The file is a Unicode string file, with each property written
 *       as two strings: the property key followed by the property value.</td>
 * </tr>
 * <tr>
 *   <td>Suite Settings</td>
 *   <td>API permissions and push interrupt setting of the MIDlet suite,
 *       written as an array of bytes</td>
 * </tr>
 * <tr>
 *   <td>Install Information</td>
 *   <td>The JAD file URL, JAR file URL, Software Certificate Authority (CA),
 *       Content Provider, Permission Domain, and a trusted boolean</td>
 * </tr>
 * <tr>
 *   <td>Suite ID</td>
 *   <td>List of MIDlet-suite identifiers. There is only one copy of this
 *       file, which is shared by all MIDlet suites. It is a common Unicode
 *       string file.</td>
 * </tr>
 * <tr>
 *   <td>Install Notification URLs</td>
 *   <td>One record is added to a list of URLs to which to post install
 *       notifications when the next OTA installation is performed or a MIDlet
 *       suite is run. There is only one copy of this file, which is shared by
 *       all MIDlet suites. It is a common Unicode string file.</td>
 * </tr>
 *   <td>Delete Notification URLs</td>
 *   <td>One record is added to a list of URLs to which to post delete
 *       notifications when the next OTA installation is performed. There is
 *       only one copy of this file, which is shared by all MIDlet suites. It
 *       is a common Unicode string file.</td>
 * </tr>
 * <tr>
 *   <td>Verification Hash<td>
 *   <td>If the preverification option is built, a file with the hash of
 *       the JAR is written to its own file.</td>
 * </tr>
 * <tr>
 *   <td>Cached Images<td>
 *   <td>If the image cache option is is built, each image in the JAR, will
 *       be extracted, decode into a platform binary image, and stored in the
 *       image cache. See imageCache.h.</td>
 * </tr>
 * </table>
 */

#include <kni.h>
#include <string.h> /* for NULL */
#include <midpInit.h>
#include <midpStorage.h>
#include <pcsl_memory.h>

#include <suitestore_intern.h>
#include <suitestore_listeners.h>

/*
 * This header is required for midp_suite_exists() which is used
 * from build_suite_filename(). This introduces a cyclic dependency
 * between suitestore_intern and suitestore_query libraries.
 * It can be easily eliminated by introducing suite_exists_impl()
 * in this (suitestore_intern) library, but it seems to be unnecessary
 * because suitestore_intern contains only internal functions and
 * is not intended to be ported.
 */
#include <suitestore_task_manager.h>

/** Cache of the last suite the exists function found. */
SuiteIdType g_lastSuiteExistsID = UNUSED_SUITE_ID;

/** A flag indicating that the _suites.dat was already loaded. */
int g_isSuitesDataLoaded = 0;

/** Number of the installed midlet suites. */
int g_numberOfSuites = 0;

/** List of structures with the information about the installed suites. */
MidletSuiteData* g_pSuitesData = NULL;

/** Indicates if the suite storage is already initialized. */
static int g_suiteStorageInitDone = 0;

/** Indicates if a transaction was started. */
static int g_transactionStarted = 0;

/**
 * A string that will be added to a file name to
 * make the name of the temporary file.
 */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(TMP_FILE_EXTENSION)
    {'.', 't', 'm', 'p', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(TMP_FILE_EXTENSION);

/**
 * Initializes the subsystem. This wrapper is used to hide
 * global variables from the suitestore_common library.
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError
suite_storage_init_impl() {
    if (g_suiteStorageInitDone) {
        /* Already initialized */
        return ALL_OK;
    }

    g_suiteStorageInitDone = 1;
    g_pSuitesData        = NULL;
    g_numberOfSuites     = 0;
    g_isSuitesDataLoaded = 0;
    g_lastSuiteExistsID  = UNUSED_SUITE_ID;

    return init_listeners_impl();
}

/**
 * Frees the memory occupied by the given MidletSuiteData structure.
 *
 * @param pData MidletSuiteData entry to be freed
 */
void
free_suite_data_entry(MidletSuiteData* pData) {
    if (pData != NULL) {
        if ((pData->jarHashLen > 0) && pData->varSuiteData.pJarHash) {
            pcsl_mem_free(pData->varSuiteData.pJarHash);
        }

        pcsl_string_free(&pData->varSuiteData.midletClassName);
        pcsl_string_free(&pData->varSuiteData.displayName);
        pcsl_string_free(&pData->varSuiteData.iconName);
        pcsl_string_free(&pData->varSuiteData.suiteVendor);
        pcsl_string_free(&pData->varSuiteData.suiteName);
        pcsl_string_free(&pData->varSuiteData.pathToJar);
        pcsl_string_free(&pData->varSuiteData.pathToSettings);

        pcsl_mem_free(pData);
    }
}

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 * This wrapper is used to hide global variables from the suitestore_common
 * library.
 */
void
suite_storage_cleanup_impl() {
    if (!g_suiteStorageInitDone) {
        /* The subsystem was not initialized */
        return;
    }

    remove_all_storage_lock();

    suite_remove_all_listeners();
    
    if (g_isSuitesDataLoaded) {
        MidletSuiteData* pData = g_pSuitesData;

        /* free each midlet suite entry */
        while (pData != NULL) {
            MidletSuiteData* pTmp = pData->nextEntry;
            free_suite_data_entry(pData);
            pData = pTmp;
        }
    }

    g_pSuitesData        = NULL;
    g_numberOfSuites     = 0;
    g_isSuitesDataLoaded = 0;
    g_lastSuiteExistsID  = UNUSED_SUITE_ID;
    g_suiteStorageInitDone = 0;
}

/**
 * Gets the storage root for a MIDlet suite by ID.
 * Free the data of the string returned with pcsl_string_free().
 *
 * @param suiteId suite ID
 * @param sRoot receives storage root (gets set to NULL in the case of an error)
 *
 * @return status: ALL_OK if success,
 * OUT_OF_MEMORY if out-of-memory
 */
MIDPError
get_suite_storage_root(SuiteIdType suiteId, pcsl_string* sRoot) {
    StorageIdType storageId;
    const pcsl_string* root;
    MIDPError status;

    *sRoot = PCSL_STRING_EMPTY;

    /* get an id of the storage where the suite is located */
    status = midp_suite_get_suite_storage(suiteId, &storageId);
    if (status != ALL_OK) {
        return status;
    }

    root = storage_get_root(storageId);

    pcsl_string_predict_size(sRoot,
        pcsl_string_length(root) + GET_SUITE_ID_LEN(suiteId));

    if (PCSL_STRING_OK == pcsl_string_append(sRoot, root) &&
            PCSL_STRING_OK == pcsl_string_append(sRoot,
                midp_suiteid2pcsl_string(suiteId))) {
        return ALL_OK;
    }

    pcsl_string_free(sRoot);
    *sRoot = PCSL_STRING_NULL;

    return OUT_OF_MEMORY;
}

/**
 * Free the buffers allocated for structures containing information
 * about the installed midlet suites.
 */
static void
free_suites_data() {
    MidletSuiteData *pData = g_pSuitesData, *pNextData;

    while (pData != NULL) {
        pNextData = pData->nextEntry;
        free_suite_data_entry(pData);
        pData = pNextData;
    }

    g_pSuitesData        = NULL;
    g_numberOfSuites     = 0;
    g_isSuitesDataLoaded = 0;
}

/**
 * Search for a structure describing the suite by the suite's ID.
 *
 * @param suiteId unique ID of the midlet suite
 *
 * @return pointer to the MidletSuiteData structure containing
 * the suite's attributes or NULL if the suite was not found
 */
MidletSuiteData*
get_suite_data(SuiteIdType suiteId) {
    MidletSuiteData* pData;

    pData = g_pSuitesData;

    /* walk through the linked list */
    while (pData != NULL) {
        if (pData->suiteId == suiteId) {
            return pData;
        }
        pData = pData->nextEntry;
    }

    return NULL;
}

/**
 * Reads the given file into the given buffer.
 * File contents is read as one piece.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param pFileName file to read
 * @param outBuffer buffer where the file contents should be stored
 * @param outBufferLen length of the outBuffer
 *
 * @return status code (ALL_OK if there was no errors)
 */
MIDPError
read_file(char** ppszError, const pcsl_string* pFileName,
          char** outBuffer, long* outBufferLen) {
    int handle, status = ALL_OK;
    long fileSize, len;
    char* pszTemp;
    char* buffer = NULL;

    *ppszError  = NULL;
    *outBuffer  = NULL;
    *outBufferLen = 0;

    /* open the file */
    handle = storage_open(ppszError, pFileName, OPEN_READ);
    if (*ppszError != NULL) {
        if (!storage_file_exists(pFileName)) {
            return NOT_FOUND;
        }
        return IO_ERROR;
    }

    do {
        /* get the size of file */
        fileSize = storageSizeOf(ppszError, handle);
        if (*ppszError != NULL) {
            status = IO_ERROR;
            break;
        }

        if (fileSize > 0) {
            /* allocate a buffer to store the file contents */
            buffer = (char*)pcsl_mem_malloc(fileSize);
            if (buffer == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }

            /* read the whole file */
            len = storageRead(ppszError, handle, buffer, fileSize);
            if (*ppszError != NULL || len != fileSize) {
                pcsl_mem_free(buffer);
                status = IO_ERROR;
            }
        }
    } while (0);

    /* close the file */
    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (status == ALL_OK) {
        *outBuffer    = buffer;
        *outBufferLen = fileSize;
    }

    return status;
}

/**
 * Writes the contents of the given buffer into the given file.
 *
 * Note that if the length of the input buffer is zero or less,
 * the file will be truncated.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param pFileName file to write
 * @param inBuffer buffer with data that will be stored
 * @param inBufferLen length of the inBuffer
 *
 * @return status code (ALL_OK if there was no errors)
 */
MIDPError
write_file(char** ppszError, const pcsl_string* pFileName,
           char* inBuffer, long inBufferLen) {
    int handle, status = ALL_OK;
    char* pszTemp;
    pcsl_string tmpFileName;
    pcsl_string_status rc;

    *ppszError = NULL;

    /* get the name of the temporary file */
    rc = pcsl_string_cat(pFileName, &TMP_FILE_EXTENSION, &tmpFileName);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    /* open the file */
    handle = storage_open(ppszError, &tmpFileName, OPEN_READ_WRITE_TRUNCATE);
    if (*ppszError != NULL) {
        pcsl_string_free(&tmpFileName);
        return IO_ERROR;
    }

    /* write the whole buffer */
    if (inBufferLen > 0) {
        storageWrite(ppszError, handle, inBuffer, inBufferLen);
    }

    if (*ppszError != NULL) {
        status = IO_ERROR;
    }

    /* close the file */
    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (status == ALL_OK) {
        /* rename the temporary file */
        storage_rename_file(ppszError, &tmpFileName, pFileName);
        if (*ppszError != NULL) {
            status = IO_ERROR;
            storage_delete_file(&pszTemp, &tmpFileName);
            storageFreeError(pszTemp);
        }
    } else {
        storage_delete_file(&pszTemp, &tmpFileName);
        storageFreeError(pszTemp);
    }

    pcsl_string_free(&tmpFileName);

    return status;
}

#define ADJUST_POS_IN_BUF(pos, bufferLen, n) \
    pos += n; \
    bufferLen -= n;

/**
 * Reads the file with information about the installed suites.
 *
 * Note that if the value of the global variable g_numberOfSuites
 * is zero, this function does nothing.
 *
 * @param ppszError pointer to character string pointer to accept an error
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR,
 *         SUITE_CORRUPTED_ERROR if the suite database is corrupted
 */
MIDPError
read_suites_data(char** ppszError) {
    MIDPError status;
    int i;
    long bufferLen, pos;
    char* buffer = NULL;
    pcsl_string_status rc;
    pcsl_string suitesDataFile;
    MidletSuiteData *pSuitesData = g_pSuitesData;
    MidletSuiteData *pData, *pPrevData = NULL;
    int numOfSuites = 0;

    *ppszError = NULL;

    if (g_isSuitesDataLoaded) {
        return ALL_OK;
    }

    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    /* get a full path to the _suites.dat */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &SUITE_DATA_FILENAME, &suitesDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    /* read the file */
    status = read_file(ppszError, &suitesDataFile, &buffer, &bufferLen);
    pcsl_string_free(&suitesDataFile);

    if (status == NOT_FOUND || (status == ALL_OK && bufferLen == 0)) {
        /* _suites.dat is absent or empty, it's a normal situation */
        g_pSuitesData        = NULL;
        g_numberOfSuites     = 0;
        g_isSuitesDataLoaded = 1;
        return ALL_OK;
    }

    if (status == ALL_OK && bufferLen < (long) sizeof(int)) {
        pcsl_mem_free(buffer);
        status = SUITE_CORRUPTED_ERROR; /* _suites.dat is corrupted */
    }
    if (status != ALL_OK) {
        return status;
    }

    /* parse contents of the suite database */
    pos = 0;
    numOfSuites = *(int*)&buffer[pos];
    ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(int));

    for (i = 0; i < numOfSuites; i++) {
        pData = (MidletSuiteData*) pcsl_mem_malloc(sizeof(MidletSuiteData));
        if (!pData) {
            status = OUT_OF_MEMORY;
            break;
        }

        if (pPrevData) {
            pPrevData->nextEntry = pData;
        } else {
            pSuitesData = pData;
        }

        /* IMPL_NOTE: introduce pcsl_mem_copy() */
        if (bufferLen < (long)MIDLET_SUITE_DATA_SIZE) {
            status = IO_ERROR;
            break;
        }
        memcpy((char*)pData, (char*)&buffer[pos], MIDLET_SUITE_DATA_SIZE);
        ADJUST_POS_IN_BUF(pos, bufferLen, MIDLET_SUITE_DATA_SIZE);

        pData->nextEntry = NULL;

        /* this suite was not checked if it is corrupted */
        pData->isChecked = 0;

        /* setup pJarHash */
        if (pData->jarHashLen > 0) {
            pData->varSuiteData.pJarHash = pcsl_mem_malloc(pData->jarHashLen);
            if (pData->varSuiteData.pJarHash == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }
            memcpy(pData->varSuiteData.pJarHash, (char*)&buffer[pos],
                pData->jarHashLen);
            ADJUST_POS_IN_BUF(pos, bufferLen, pData->jarHashLen);
        } else {
            pData->varSuiteData.pJarHash = NULL;
        }

        /* setup string fields */
        {
            int i;
            jint strLen;
            pcsl_string* pStrings[] = {
                &pData->varSuiteData.midletClassName,
                &pData->varSuiteData.displayName,
                &pData->varSuiteData.iconName,
                &pData->varSuiteData.suiteVendor,
                &pData->varSuiteData.suiteName,
                &pData->varSuiteData.pathToJar,
                &pData->varSuiteData.pathToSettings
            };

            status = ALL_OK;

            for (i = 0; i < (int) (sizeof(pStrings) / sizeof(pStrings[0]));
                    i++) {
                if (bufferLen < (long)sizeof(jint)) {
                    status = IO_ERROR; /* _suites.dat is corrupted */
                    break;
                }

                /*
                 * We have to guarantee 4 - bytes alignment to use this:
                 *     strLen = *(jint*)&buffer[pos];
                 * on RISC CPUs.
                 */
                pos = SUITESTORE_ALIGN_4(pos);
                strLen = *(jint*)&buffer[pos];
                ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(jint));

                if (bufferLen < (long)strLen) {
                    status = IO_ERROR; /* _suites.dat is corrupted */
                    break;
                }

                if (strLen > 0) {
                    rc = pcsl_string_convert_from_utf16(
                        (jchar*)&buffer[pos], strLen, pStrings[i]);

                    if (rc != PCSL_STRING_OK) {
                        status = OUT_OF_MEMORY;
                        break;
                    }
                    ADJUST_POS_IN_BUF(pos, bufferLen, strLen * sizeof(jchar));
                } else {
                    *pStrings[i] = strLen ?
                        PCSL_STRING_NULL : PCSL_STRING_EMPTY;
                }
            }
        }

        if (status != ALL_OK) {
            break;
        }

        pData->nextEntry = NULL;
        pPrevData = pData;
    } /* end for (numOfSuites) */

    pcsl_mem_free(buffer);

    if (status == ALL_OK) {
        g_numberOfSuites = numOfSuites;
        g_pSuitesData = pSuitesData;
        g_isSuitesDataLoaded = 1;
    } else {
        free_suites_data();
    }

    return status;
}

/**
 * Writes the file with information about the installed suites.
 *
 * Note that if the value of the global variable g_numberOfSuites
 * is zero, the file will be truncated.
 *
 * @param ppszError pointer to character string pointer to accept an error
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR
 */
MIDPError
write_suites_data(char** ppszError) {
    MIDPError status = ALL_OK;
    long bufferLen, pos;
    char* buffer = NULL;
    pcsl_string_status rc;
    pcsl_string suitesDataFile;
    MidletSuiteData* pData;

    *ppszError = NULL;

    /* get a full path to the _suites.dat */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &SUITE_DATA_FILENAME, &suitesDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    if (!g_numberOfSuites) {
        /* truncate the file with the list of the installed suites */
        status = write_file(ppszError, &suitesDataFile, buffer, 0);
        pcsl_string_free(&suitesDataFile);
        return status;        
    }

    /* allocate a buffer where the information about all suites will be saved */
    bufferLen = g_numberOfSuites * (sizeof(MidletSuiteData) +
        MAX_VAR_SUITE_DATA_LEN);
    /* space to store the number of suites */
    bufferLen += sizeof(int);
    buffer = pcsl_mem_malloc(bufferLen);
    if (buffer == NULL) {
        pcsl_string_free(&suitesDataFile);
        return OUT_OF_MEMORY;
    }

    /* assemble the information about all suites into the allocated buffer */
    pos = 0;
    pData = g_pSuitesData;

    *(int*)&buffer[pos] = g_numberOfSuites;
    ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(int));

    while (pData != NULL) {
        memcpy((char*)&buffer[pos], (char*)pData, MIDLET_SUITE_DATA_SIZE);
        ADJUST_POS_IN_BUF(pos, bufferLen, MIDLET_SUITE_DATA_SIZE);

        /* setup pJarHash */
        if (pData->jarHashLen > 0) {
            memcpy((char*)&buffer[pos], pData->varSuiteData.pJarHash,
                pData->jarHashLen);
            ADJUST_POS_IN_BUF(pos, bufferLen, pData->jarHashLen);
        }

        /* setup string fields */
        {
            int i, convertedLen;
            jint strLen;
            pcsl_string* pStrings[] = {
                &pData->varSuiteData.midletClassName,
                &pData->varSuiteData.displayName,
                &pData->varSuiteData.iconName,
                &pData->varSuiteData.suiteVendor,
                &pData->varSuiteData.suiteName,
                &pData->varSuiteData.pathToJar,
                &pData->varSuiteData.pathToSettings
            };

            status = ALL_OK;

            for (i = 0; i < (int) (sizeof(pStrings) / sizeof(pStrings[0]));
                    i++) {
                strLen = pcsl_string_utf16_length(pStrings[i]);
                /*
                 * We have to guarantee 4 - bytes alignment to use this:
                 *     *(jint*)&buffer[pos] = strLen;
                 * on RISC CPUs.
                 */
                pos = SUITESTORE_ALIGN_4(pos);
                *(jint*)&buffer[pos] = strLen;
                ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(jint));

                /* assert(bufferLen > 0); */
                if (strLen > 0) {
                    rc = pcsl_string_convert_to_utf16(pStrings[i],
                        (jchar*)&buffer[pos], bufferLen / sizeof(jchar),
                            &convertedLen);

                    if (rc != PCSL_STRING_OK) {
                        status = OUT_OF_MEMORY;
                        break;
                    }

                    ADJUST_POS_IN_BUF(pos, bufferLen,
                        convertedLen * sizeof(jchar));
                }
            }
        }

        if (status != ALL_OK) {
            break;
        }

        pData = pData->nextEntry;
    }

    if (status == ALL_OK) {
        /* write the buffer into the file */
        status = write_file(ppszError, &suitesDataFile, buffer, pos);
    }

    /* cleanup */
    pcsl_mem_free(buffer);
    pcsl_string_free(&suitesDataFile);

    return status;
}
#undef ADJUST_POS_IN_BUF

/**
 * Builds a full file name using the storage root and MIDlet suite by ID.
 *
 * @param suiteId suite ID
 * @param filename filename without a root path
 * @param sRoot receives full name of the file
 *
 * @return the status: ALL_OK if ok, OUT_OF_MEMORY if out of memory
 */
static MIDPError
get_suite_filename(SuiteIdType suiteId, const pcsl_string* filename,
                   pcsl_string* sRoot) {
  int sRoot_len;
  const pcsl_string* root;

  root = storage_get_root(INTERNAL_STORAGE_ID);

  *sRoot = PCSL_STRING_EMPTY;

  sRoot_len = pcsl_string_length(root)      
            + GET_SUITE_ID_LEN(suiteId)
            + pcsl_string_length(filename);
  pcsl_string_predict_size(sRoot, sRoot_len);

  if (PCSL_STRING_OK == pcsl_string_append(sRoot, root) &&
      PCSL_STRING_OK == pcsl_string_append(sRoot,
          midp_suiteid2pcsl_string(suiteId)) &&
      PCSL_STRING_OK == pcsl_string_append(sRoot, filename)) {
      return ALL_OK;
  } else {
        pcsl_string_free(sRoot);
        *sRoot = PCSL_STRING_NULL;
        return OUT_OF_MEMORY;
  }
}

/**
 * Builds a full file name using the storage root and MIDlet suite by ID.
 * get_suite_filename is used to build a filename after validation checks.
 *
 * @param suiteId suite ID
 * @param filename filename without a root path
 * @param res receives full name of the file
 *
 * @return the status:
 * ALL_OK if ok,
 * NOT_FOUND mean the suite does not exist,
 * OUT_OF_MEMORY if out of memory,
 * IO_ERROR if an IO_ERROR
 */
MIDPError
build_suite_filename(SuiteIdType suiteId, const pcsl_string* filename,
                     pcsl_string* res) {
    MIDPError status;

    *res = PCSL_STRING_NULL;

    status = midp_suite_exists(suiteId);

    /* Ignore if suite is corrupted */
    if ((status != ALL_OK) && (status != SUITE_CORRUPTED_ERROR)) {
        return status;
    }

    return get_suite_filename(suiteId, filename, res);
}

/**
 * Gets location of the file associated with the named resource
 * of the suite with the specified suiteId.
 *
 * Note that in/out parameter filename MUST be allocated by callee with
 * pcsl_mem_malloc(), the caller is responsible for freeing it.
 *
 * @param suiteId The application suite ID
 * @param resourceName The name of suite resource whose location is requested
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param filename The in/out parameter that contains returned filename
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
static MIDPError
get_suite_resource_file(SuiteIdType suiteId,
                        const pcsl_string* resourceName,
                        jboolean checkSuiteExists,
                        pcsl_string *filename) {
  pcsl_string returnFileName = PCSL_STRING_NULL;
  int rc;
  *filename = PCSL_STRING_NULL;

  if (checkSuiteExists) {
      rc = build_suite_filename(suiteId, resourceName, &returnFileName);
  } else {
      rc = get_suite_filename(suiteId, resourceName, &returnFileName);
  }

  if (rc == ALL_OK) {
      *filename = returnFileName;
  }

  return ALL_OK;
}

/**
 * Gets location of the properties file
 * for the suite with the specified suiteId.
 *
 * Note that in/out parameter filename MUST be allocated by callee with
 * pcsl_mem_malloc(), the caller is responsible for freeing it.
 *
 * @param suiteId The application suite ID
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param pFilename The in/out parameter that contains returned filename
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND
 * </pre>
 */
MIDPError get_property_file(SuiteIdType suiteId,
                            jboolean checkSuiteExists,
                            pcsl_string *pFilename) {
    return get_suite_resource_file(suiteId, &PROPS_FILENAME,
            checkSuiteExists, pFilename);
}

/**
 * Gets filename of the secure suite resource by suiteId and resource name
 *
 * Note that memory for the in/out parameter filename
 * MUST be allocated using pcsl_mem_malloc().
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param pResourceName     The name of secure resource to read from storage
 * @param pFilename         The in/out parameter that will return filename
 *                          of the specified secure resource
 * @return one of the error codes:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError get_secure_resource_file(SuiteIdType suiteId,
        const pcsl_string* pResourceName, pcsl_string *pFilename) {
    pcsl_string resourceID = PCSL_STRING_NULL;
    MIDPError errorCode;

    do {
        if (pcsl_string_cat(pResourceName, &SECURE_EXTENSION, &resourceID) !=
                PCSL_STRING_OK) {
            errorCode = OUT_OF_MEMORY;
            break;
        }

        errorCode = get_suite_resource_file(suiteId, &resourceID,
            KNI_FALSE, pFilename);

    } while (0);

    pcsl_string_free(&resourceID);

    return errorCode;
}

/**
 * Gets the settings of a MIDlet suite from persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId ID of the suite
 * @param pEnabled pointer to an enabled setting
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
read_enabled_state(char** ppszError, SuiteIdType suiteId,
                   jboolean* pEnabled) {
    MidletSuiteData* pData;
    MIDPError status;

    /* load _suites.dat */
    status = read_suites_data(ppszError);
    if (status != ALL_OK) {
        return status;
    }

    pData = g_pSuitesData;
    *pEnabled = 0;

    /* try to find a suite */
    while (pData != NULL) {
        if (pData->suiteId == suiteId) {
            *pEnabled = pData->isEnabled;
            break;
        }

        pData = pData->nextEntry;
    }

    return ALL_OK;
}

/**
 * Gets the settings of a MIDlet suite from persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId  ID of the suite
 * @param pEnabled pointer to an enabled setting
 * @param pPushInterrupt pointer to a push interruptSetting
 * @param pPushOptions user options for push interrupts
 * @param ppPermissions pointer a pointer to accept a permissions array
 * @param pNumberOfPermissions pointer to an int
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
read_settings(char** ppszError, SuiteIdType suiteId, jboolean* pEnabled,
             jbyte* pPushInterrupt, jint* pPushOptions,
             jbyte** ppPermissions, int* pNumberOfPermissions) {
    pcsl_string filename;
    int handle;
    int bytesRead;
    char* pszTemp;
    MIDPError status;

    *ppszError = NULL;
    *ppPermissions = NULL;
    *pNumberOfPermissions = 0;

    status = build_suite_filename(suiteId, &SETTINGS_FILENAME, &filename);
    if (status != ALL_OK) {
        return status;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return IO_ERROR;
    }

    bytesRead = storageRead(ppszError, handle, (char*)pEnabled,
        sizeof (jboolean));
    do {
        if (*ppszError != NULL) {
            status = IO_ERROR;
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)pPushInterrupt,
                                sizeof (jbyte));
        if (*ppszError != NULL) {
            status = IO_ERROR;
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)pNumberOfPermissions,
                                sizeof (int));

        if (bytesRead != sizeof (int) || *pNumberOfPermissions == 0) {
            status = IO_ERROR;
            break;
        }

        *ppPermissions = (jbyte*)pcsl_mem_malloc(
                *pNumberOfPermissions * sizeof (jbyte));
        if (*ppPermissions == NULL) {
            status = OUT_OF_MEMORY;
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)(*ppPermissions),
                                *pNumberOfPermissions * sizeof (jbyte));

        if (bytesRead != *pNumberOfPermissions) {
            *pNumberOfPermissions = 0;
            pcsl_mem_free(*ppPermissions);
            status = SUITE_CORRUPTED_ERROR;
            break;
        }

        /* Old versions of the file may not have options. */
        status = ALL_OK;
        *pPushOptions = 0;
        bytesRead = storageRead(ppszError, handle, (char*)pPushOptions,
                                sizeof (jint));
        if (*ppszError != NULL) {
            storageFreeError(*ppszError);
            *ppszError = NULL;
            break;
        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    return status;
}

/**
 * Writes the settings of a MIDlet suite to persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId  ID of the suite
 * @param enabled enabled setting
 * @param pushInterrupt pointer to a push interruptSetting
 * @param pushOptions user options for push interrupts
 * @param pPermissions pointer a pointer to accept a permissions array
 * @param numberOfPermissions length of pPermissions
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
write_settings(char** ppszError, SuiteIdType suiteId, jboolean enabled,
               jbyte pushInterrupt, jint pushOptions,
               jbyte* pPermissions, int numberOfPermissions) {
    pcsl_string filename;
    int handle;
    char* pszTemp;
    MIDPError status;

    *ppszError = NULL;

    status = build_suite_filename(suiteId, &SETTINGS_FILENAME, &filename);
    if (status != ALL_OK) {
        return status;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return IO_ERROR;
    }

    storageWrite(ppszError, handle, (char*)&enabled, sizeof (jboolean));
    do {
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)&pushInterrupt, sizeof (jbyte));
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)&numberOfPermissions,
                                sizeof (int));
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)pPermissions,
                                numberOfPermissions * sizeof (jbyte));

        storageWrite(ppszError, handle, (char*)&pushOptions, sizeof (jint));
        if (*ppszError != NULL) {
            break;
        }
    } while (0);

    if (*ppszError != NULL) {
        status = IO_ERROR;
    }

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    return ALL_OK;
}

/**
 * Native method boolean removeFromSuiteList(String) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Removes the suite from the list of installed suites.
 * <p>
 * Used from suitestore_midletsuitestorage_kni.c so is non-static.
 *
 * @param suiteId ID of a suite
 *
 * @return 1 if the suite was in the list, 0 if not
 * -1 if out of memory
 */
int
remove_from_suite_list_and_save(SuiteIdType suiteId) {
    int existed = 0;
    MidletSuiteData *pData, *pPrevData = NULL;

    if (suiteId == UNUSED_SUITE_ID || suiteId == INTERNAL_SUITE_ID) {
        return 0; /* suite was not in the list */
    }

    /*
     * This function is called from midp_remove_suite(),
     * so read_suites_data() was already called.
     */
    pData = g_pSuitesData;

    /* try to find a suite */
    while (pData != NULL) {
        if (pData->suiteId == suiteId) {
            int status;
            char* pszError;

            /* remove the entry we have found from the list */
            if (pPrevData) {
                /* this entry is not the first */
                pPrevData->nextEntry = pData->nextEntry;
            } else {
                /* handle the first entry */
                g_pSuitesData = pData->nextEntry;
            }

            /* free the memory allocated for the entry */
            free_suite_data_entry(pData);

            /* decrease the number of the installed suites */
            g_numberOfSuites--;

            /* save the modified list into _suites.dat */
            status = write_suites_data(&pszError);
            existed = (status == ALL_OK) ? 1 : -1;
            storageFreeError(pszError);
            break;
        }

        pPrevData = pData;
        pData = pData->nextEntry;
    }

    /* Reset the static variable for MVM-safety */
    g_lastSuiteExistsID = UNUSED_SUITE_ID;

    return existed;
}

/**
 * Check if the suite is corrupted
 * @param suiteId ID of a suite
 *
 * @return ALL_OK if the suite is not corrupted,
 *         SUITE_CORRUPTED_ERROR is suite is corrupted,
 *         OUT_OF_MEMORY if out of memory,
 *         IO_ERROR if I/O error
 */
MIDPError
check_for_corrupted_suite(SuiteIdType suiteId) {
    pcsl_string filename[NUM_SUITE_FILES];
    int arc[NUM_SUITE_FILES];
    int i;
    StorageIdType storageId;
    MIDPError status = ALL_OK; /* Default to no error */
    MidletSuiteData *pData = get_suite_data(suiteId);

    if (!pData) {
        return SUITE_CORRUPTED_ERROR;
    }

    /* if this suite was already checked, just return "OK" status */
    if (pData->isChecked) {
        return status;
    }

    /* get an id of the storage where the suite is located */
    status = midp_suite_get_suite_storage(suiteId, &storageId);
    if (status != ALL_OK) {
        return status;
    }

    arc[0] = get_suite_filename(suiteId, &INSTALL_INFO_FILENAME, &filename[0]);
    arc[1] = get_suite_filename(suiteId, &SETTINGS_FILENAME, &filename[1]);
    arc[2] = midp_suite_get_class_path(suiteId, storageId,
                                       KNI_FALSE, &filename[2]);
    arc[3] = get_property_file(suiteId, KNI_FALSE, &filename[3]);

    for (i = 0; i < NUM_SUITE_FILES; i++) {
        if (arc[i] != ALL_OK) {
            status = arc[i];
            break;
        }

        if (!storage_file_exists(&filename[i])) {
            /* File does not exist; suite must be corrupted */
            status = SUITE_CORRUPTED_ERROR;
            break;
        }
    }

    if (status == ALL_OK) {
        /* if the suite is not currupted, mark it as "checked" */
        pData->isChecked = 1;
    }

    pcsl_string_free(&filename[0]);
    pcsl_string_free(&filename[1]);
    pcsl_string_free(&filename[2]);
    pcsl_string_free(&filename[3]);

    return status;
}

/**
 * Tries to repair the suite database.
 *
 * @return ALL_OK if succeeded, other value if failed
 */
MIDPError repair_suite_db() {
    /* IMPL_NOTE: should be replaced with more sophisticated routine. */
    MIDPError status = ALL_OK;
    pcsl_string_status rc;
    pcsl_string suitesDataFile;
    char* pszTemp = NULL;

    /* get a full path to the _suites.dat */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &SUITE_DATA_FILENAME, &suitesDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    storage_delete_file(&pszTemp, &suitesDataFile);
    pcsl_string_free(&suitesDataFile);
    if (pszTemp != NULL) {
        storageFreeError(pszTemp);
        status = IO_ERROR;
    }

    if (g_isSuitesDataLoaded) {
        free_suites_data();
        g_isSuitesDataLoaded = 0;
    }

    return status;
}

/**
 * Starts a new transaction of the given type.
 *
 * @param transactionType type of the new transaction
 * @param suiteId ID of the suite, may be UNUSED_SUITE_ID
 * @param pFilename name of the midlet suite's file, may be NULL
 *
 * @return ALL_OK if no errors,
 *         IO_ERROR if I/O error
 */
MIDPError begin_transaction(MIDPTransactionType transactionType,
                            SuiteIdType suiteId,
                            const pcsl_string *pFilename) {
    pcsl_string_status rc;
    pcsl_string transDataFile;
    char *pszError = NULL;
    MIDPError status = ALL_OK;
    char pBuffer[MAX_FILENAME_LENGTH + sizeof(int) /* file name length */ + 
                 sizeof(suiteId) + sizeof(transactionType)];
    char *p = pBuffer;
    int len = sizeof(suiteId) + sizeof(transactionType);

    *(MIDPTransactionType*)p = transactionType;
    p += sizeof(MIDPTransactionType);
    *(SuiteIdType*)p = suiteId;
    p += sizeof(SuiteIdType);

    if (pFilename != NULL) {
        int strLen;

        rc = pcsl_string_convert_to_utf16(pFilename,
                        (jchar*)(p + sizeof(int)),
                        MAX_FILENAME_LENGTH / sizeof(jchar),
                        &strLen);
        if (rc != PCSL_STRING_OK) {
            return OUT_OF_MEMORY;
        }

        *(int*)p = strLen;

        len += strLen;
    }

    /* get a full path to the transaction data file */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &TRANSACTION_DATA_FILENAME, &transDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    status = write_file(&pszError, &transDataFile, pBuffer, len);
    storageFreeError(pszError);
    pcsl_string_free(&transDataFile);

    g_transactionStarted = 1;

    return status;
}

/**
 * Rolls back the transaction being in progress.
 *
 * @return ALL_OK if the transaction was rolled back,
 *         NOT_FOUND if the transaction has not been started,
 *         IO_ERROR if I/O error
 */
MIDPError rollback_transaction() {
    MIDPError status = ALL_OK;
    pcsl_string_status rc;
    pcsl_string transDataFile;
    char *pszTemp = NULL;

    /* get a full path to the transaction data file */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &TRANSACTION_DATA_FILENAME, &transDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    storage_delete_file(&pszTemp, &transDataFile);
    pcsl_string_free(&transDataFile);
    if (pszTemp != NULL) {
        storageFreeError(pszTemp);
        status = IO_ERROR;
    } else {
        g_transactionStarted = 0;
    }

    return ALL_OK;
}

/**
 * Finishes the previously started transaction.
 *
 * @return ALL_OK if the transaction was successfully finished,
 *         NOT_FOUND if the transaction has not been started,
 *         IO_ERROR if I/O error
 */
MIDPError finish_transaction() {
    pcsl_string_status rc;
    pcsl_string transDataFile;
    char* pszTemp = NULL;
    MIDPError status = ALL_OK;

    if (!g_transactionStarted) {
        return NOT_FOUND;
    }

    /* transaction is finished, removed the transaction file */

    /* get a full path to the transaction data file */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &TRANSACTION_DATA_FILENAME, &transDataFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    storage_delete_file(&pszTemp, &transDataFile);
    pcsl_string_free(&transDataFile);
    if (pszTemp != NULL) {
        storageFreeError(pszTemp);
        status = IO_ERROR;
    }

    g_transactionStarted = 0;

    return status;
}

/**
 * Checks if there is an unfinished transaction.
 *
 * @return 0 there is no unfinished transaction, != 0 otherwise
 */
int unfinished_transaction_exists() {
    pcsl_string_status rc;
    pcsl_string transDataFile;
    int res = 0;

    if (g_transactionStarted) {
        return 1;
    }

    /* get a full path to the transaction data file */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &TRANSACTION_DATA_FILENAME, &transDataFile);
    if (rc != PCSL_STRING_OK) {
        return 0;
    }

    res = storage_file_exists(&transDataFile);

    pcsl_string_free(&transDataFile);

    return res;
}


#if ENABLE_CONTROL_ARGS_FROM_JAD
/**
 * Loads the properties of a MIDlet suite from persistent storage.
 *
 * @param suiteId ID of the suite
 * @param pJadProps [out] pointer to a structure containing an array of strings,
 * in a pair pattern of key and value; NULL may be passed if it is not required
 * to read JAD properties
 * @param pJarProps [out] pointer to a structure containing an array of strings,
 * in a pair pattern of key and value; NULL may be passed if it is not required
 * to read JAR properties
 *
 * @return error code (ALL_OK for success)
 */
MIDPError
load_install_properties(SuiteIdType suiteId, MidpProperties* pJadProps,
                        MidpProperties* pJarProps) {
    pcsl_string filename;
    char* pszError = NULL;
    int handle, i, n;
    int numberOfProps;
    MIDPError status;

    status = get_property_file(suiteId, KNI_TRUE, &filename);
    if (status != ALL_OK) {
        return status;
    }

    handle = storage_open(&pszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return IO_ERROR;
    }

    status = ALL_OK;

    /* Read JAD, then JAR properties. */
    for (n = 0; n < 2; n++) {
        MidpProperties *pProps = n ? pJarProps : pJadProps;
        if (!pProps) {
            continue;
        }

        storageRead(&pszError, handle, (char*)&numberOfProps,
            sizeof (numberOfProps));
        if (pszError != NULL) {
            break;
        }

        pProps->pStringArr = alloc_pcsl_string_list(numberOfProps << 1);

        for (i = 0; i < numberOfProps << 1; i++) {
            storage_read_utf16_string(&pszError, handle,
                                      &pProps->pStringArr[i]);
            if (pszError != NULL) {
                break;
            }
        }

        if (pszError != NULL) {
            break;
        }

        pProps->numberOfProperties = numberOfProps;
    }

    if (pszError != NULL) {
        status = IO_ERROR;
        storageFreeError(pszError);
    }

    storageClose(&pszError, handle);
    storageFreeError(pszError);

    if (status != ALL_OK) {
        if (pJadProps) {
            free_pcsl_string_list(pJadProps->pStringArr,
                                  pJadProps->numberOfProperties << 1);
        }
        if (pJarProps) {
            free_pcsl_string_list(pJarProps->pStringArr,
                                  pJarProps->numberOfProperties << 1);
        }
    }

    return status;
}

/**
 * Finds a value of argument with the given name in the array of control
 * arguments.
 *
 * @param pJadArgs buffer holding the name and value of each control argument
 * @param pArgName name of the argument to find
 * @param ppArgValue address where to save the pointer to the argument's value
 *
 * @return ALL_OK if the given argument name was found, NOT_FOUND otherwise
 */
MIDPError get_jad_control_arg_value(MIDP_JAD_CONTROL_ARGS* pJadArgs,
        const char* pArgName, const char** ppArgValue) {
    int i = 0;
    while (pJadArgs[i].pArgName) {
        if (!strncmp(pJadArgs[i].pArgName, pArgName,
                     pJadArgs[i].argNameLen)) {
            if (ppArgValue) {
                *ppArgValue = pJadArgs[i].pArgValue;
            }
            return ALL_OK;
        }
        i++;
    }

    if (ppArgValue) {
        *ppArgValue = NULL;
    }

    return NOT_FOUND;
}

/**
 * Parses a value of MIDP_CONTROL_ARGS property from the midlet suite's
 * JAD file. Syntax of this property is defined by the following grammar:
 * <pre>
 * JAD_CONTROL_PARAM = "MIDP_CONTROL_ARGS" EQUAL *( ";" param)
 * param = report_level_param / log_channels_param / permissions_param /
 * trace_param / assert_param
 * report_level_param = "report_level" EQUAL 1*DIGIT
 * log_channels_param = "log_channels" EQUAL 1*DIGIT *( "," 1*DIGIT)
 * permissions_param  = "allow_all_permissions"
 * trace_param  = "enable_trace" EQUAL bool_val
 * assert_param = "enable_trace" EQUAL bool_val
 * bool_val = 0 / 1
 * </pre>
 *
 * @param pJadProps properties of the midlet suite from its jad file
 * @param pJadArgs [out] buffer where to store pointers to the name and value of
 * each control argument
 * @param maxJadArgs maximal number of arguments that the output buffer can hold
 */
void parse_control_args_from_jad(const MidpProperties* pJadProps,
        MIDP_JAD_CONTROL_ARGS* pJadArgs, int maxJadArgs) {
    int i, currArgNum = 0;
    int numOfJadProps = pJadProps ? pJadProps->numberOfProperties : 0;
    /* MIDP_CONTROL_ARGS */
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(propertyName) {
        'M', 'I', 'D', 'P', '_', 'C', 'O', 'N', 'T', 'R', 'O', 'L', '_',
        'A', 'R', 'G', 'S', '\0'
    } PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(propertyName);
    static jbyte propValue[128]; /* buffer to hold MIDP_ARGS jad property */

    for (i = 0; i < (numOfJadProps << 1); i++, i++) {
        if (pcsl_string_equals(&pJadProps->pStringArr[i], &propertyName)) {
            const char *pCurr;
            int propLen = pcsl_string_utf8_length(&pJadProps->pStringArr[i+1]);
            int start_pos, pos = 0;

            if (pcsl_string_convert_to_utf8(&pJadProps->pStringArr[i+1],
                    propValue, sizeof(propValue), NULL) != PCSL_STRING_OK) {
                break;
            }

            /* Parse the value of MIDP_CONTROL_ARGS jad property. */
            pCurr = (const char*)propValue;

            while (pos < propLen && currArgNum < maxJadArgs) {
                /* skip spaces */
                if (*pCurr == ' ') {
                    pCurr++;
                    pos++;
                    continue;
                }

                pJadArgs[currArgNum].pArgName = pCurr;
                start_pos = pos;

                /* look for "=", ";" or end-of-line) */
                while (pos < propLen && *pCurr) {
                    if (*pCurr == '=') {
                        pos++;
                        pJadArgs[currArgNum].pArgValue = ++pCurr;
                        break;
                    } else if (*pCurr == ';') {
                        pJadArgs[currArgNum].pArgValue = NULL;
                        break;
                    }
                    pos++;
                    pCurr++;
                }

                pJadArgs[currArgNum].argNameLen = pos - start_pos - 1;

                /* look for the next delimeter (";" or end-of-line) */
                while (pos < propLen && *pCurr) {
                    pos++;
                    if (*pCurr++ == ';') {
                        break;
                    }
                }

                currArgNum++;
            } /* end while */

            break;
        } /* end if MIDP_ARGS property found in JAD */
    } /* end for each property */

    pJadArgs[currArgNum].pArgName = pJadArgs[currArgNum].pArgValue = NULL;
    pJadArgs[currArgNum].argNameLen = 0;
}

#endif
