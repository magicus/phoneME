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
 * @ingroup AMS
 *
 * This is reference implementation of the Task Manager API of the MIDlet
 * suite storage subsystem.
 * The functions implemented in this file need not to be ported if using NAMS.
 */

#include <kni.h>
#include <string.h>

#include <midpInit.h>
#include <midpStorage.h>
#include <midpRMS.h>
#include <push_server_export.h>
#include <pcsl_memory.h>
#include <imageCache.h>
#include <fontCache.h>

#include <suitestore_intern.h>
#include <suitestore_locks.h>
#include <suitestore_otanotifier_db.h>
#include <suitestore_listeners.h>

#if ENABLE_ICON_CACHE
#include <suitestore_icon_cache.h>
#endif

static MIDPError change_enabled_state(SuiteIdType suiteId, jboolean enabled);

/* ------------------------------------------------------------ */
/*                           Public API                         */
/* ------------------------------------------------------------ */

/**
 * Retrieves the number of installed midlet suites.
 *
 * @param pNumOfSuites [out] pointer to variable to accept the number of suites
 *
 * @returns error code (ALL_OK if no errors)
 */
MIDPError
midp_get_number_of_suites(int* pNumOfSuites) {
    MIDPError status;
    char* pszError;

    do {
        /*
         * This is a public API which can be called without the VM running
         * so we need automatically init anything needed, to make the
         * caller's code less complex.
         *
         * Initialization is performed in steps so that we do use any
         * extra resources such as the VM for the operation being performed.
         */
        if (midpInit(LIST_LEVEL) != 0) {
            status = OUT_OF_MEMORY;
            break;
        }

        /* load _suites.dat */
        status = read_suites_data(&pszError);
        if (status == ALL_OK) {
#if ENABLE_DYNAMIC_COMPONENTS
            MidletSuiteData* pData = g_pSuitesData;
            int num = 0;

            /* walk through the linked list */
            while (pData != NULL) {
                if (pData->type == COMPONENT_REGULAR_SUITE) {
                    num++;
                }
                pData = pData->nextEntry;
            }

            *pNumOfSuites = num;
#else
            *pNumOfSuites = g_numberOfSuites;
#endif /* ENABLE_DYNAMIC_COMPONENTS */
        } else {
            storageFreeError(pszError);
        }
    } while(0);

    return status;
}

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Retrieves the number of the installed components belonging
 * to the given midlet suite.
 *
 * @param suiteId          [in]  ID of the MIDlet suite the information about
 *                               whose components must be retrieved
 * @param pNumOfComponents [out] pointer to variable to accept the number
 *                               of components
 *
 * @returns error code (ALL_OK if no errors)
 */
MIDPError
midp_get_number_of_components(SuiteIdType suiteId, int* pNumOfComponents) {
    MIDPError status;
    char* pszError;
    MidletSuiteData* pData;
    int n = 0;

    do {
        if (midpInit(LIST_LEVEL) != 0) {
            status = OUT_OF_MEMORY;
            break;
        }

        /* load _suites.dat */
        status = read_suites_data(&pszError);
        if (status != ALL_OK) {
            storageFreeError(pszError);
            break;
        }

        pData = g_pSuitesData;

        /* walk through the linked list */
        while (pData != NULL) {
            if (pData->suiteId == suiteId && pData->type == COMPONENT_DYNAMIC) {
                n++;
            }
            pData = pData->nextEntry;
        }

        *pNumOfComponents = n;
    } while(0);

    return status;
}
#endif /* ENABLE_DYNAMIC_COMPONENTS */

/**
 * Disables a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param suiteId  ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
MIDPError
midp_disable_suite(SuiteIdType suiteId) {
    return change_enabled_state(suiteId, KNI_FALSE);
}

/**
 * Enables a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param suiteId  ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
MIDPError
midp_enable_suite(SuiteIdType suiteId) {
    return change_enabled_state(suiteId, KNI_TRUE);
}

/**
 * Get the list installed of MIDlet suite IDs.
 *
 * Note that memory for the suite IDs is allocated by the callee,
 * and the caller is responsible for freeing it using midp_free_suite_ids().
 *
 * @param ppSuites empty array of jints to fill with suite IDs
 * @param pNumOfSuites [out] pointer to variable to accept the number
 * of suites in the returned array
 *
 * @returns error code: ALL_OK if no errors,
 *          OUT_OF_MEMORY if for out of memory,
 *          IO_ERROR if an IO error
 */
MIDPError
midp_get_suite_ids(SuiteIdType** ppSuites, int* pNumOfSuites) {
    MIDPError status;
    char* pszError;
    SuiteIdType* pSuiteIds;
    MidletSuiteData* pData;
    int numberOfSuites = 0;
#if ENABLE_DYNAMIC_COMPONENTS
    int numberOfEntries = 0;
#endif

    *ppSuites = NULL;
    *pNumOfSuites = 0;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    /* load _suites.dat */
    status = read_suites_data(&pszError);
    storageFreeError(pszError);

    if (status != ALL_OK) {
        return status;
    }

    if (!g_numberOfSuites) {
        /* there are no installed suites */
        return ALL_OK;
    }

    pData = g_pSuitesData;

    /* allocate a memory for the IDs */
    pSuiteIds = pcsl_mem_malloc(g_numberOfSuites * sizeof(SuiteIdType));
    if (pSuiteIds == NULL) {
        return OUT_OF_MEMORY;
    }

    /* walk through the linked list collecting suite IDs */
    while (pData != NULL) {
#if ENABLE_DYNAMIC_COMPONENTS
        if (pData->type == COMPONENT_REGULAR_SUITE) {
#endif
            pSuiteIds[numberOfSuites] = pData->suiteId;
            numberOfSuites++;
#if ENABLE_DYNAMIC_COMPONENTS
        }
        numberOfEntries++;
#endif
        pData = pData->nextEntry;
    }

#if ENABLE_DYNAMIC_COMPONENTS
    if (numberOfEntries != g_numberOfSuites) {
#else
    if (numberOfSuites != g_numberOfSuites) {
#endif
        /*
         * This should not happen: it means that something is wrong with
         * the list of structures containing the midlet suites information.
         */
        pcsl_mem_free(pSuiteIds);
        return IO_ERROR;
    }

    *ppSuites = pSuiteIds;
    *pNumOfSuites = numberOfSuites;

    return ALL_OK;
}

/**
 * Frees a list of suite IDs.
 *
 * @param pSuiteIds point to an array of suite IDs
 * @param numberOfSuites number of elements in pSuites
 */
void
midp_free_suite_ids(SuiteIdType* pSuiteIds, int numberOfSuites) {
    (void)numberOfSuites;
    pcsl_mem_free(pSuiteIds);
}

/**
 * Removes a software package given its suite ID
 * <p>
 * If the component is in use it must continue to be available
 * to the other components that are using it.  The resources it
 * consumes must not be released until it is not in use.
 *
 * @param suiteId ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         BAD_PARAMS this suite cannot be removed
 */
MIDPError
midp_remove_suite(SuiteIdType suiteId) {
    pcsl_string filename;
    char* pszError;
    pcsl_string suiteRoot;
    MIDPError status;
    int operationStarted = 0;
    void* fileIteratorHandle = NULL;
    MidpProperties properties;
    pcsl_string* pproperty;
    MidletSuiteData* pData = NULL;
    pcsl_string filenameBase;

    lockStorageList *node = NULL;

    /* get the filename base from the suite id */
    status = build_suite_filename(suiteId, &PCSL_STRING_EMPTY, 
                                          &filenameBase);
    if (status != ALL_OK) {
        return status;
    }
    node = find_storage_lock(suiteId);
    if (node != NULL) {
        if (node->update != KNI_TRUE) {
            return SUITE_LOCKED;
        }
    }

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(REMOVE_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    do {
        int rc; /* return code for rmsdb_... and storage_... */

        /* load _suites.dat */
        status = read_suites_data(&pszError);
        storageFreeError(pszError);
        if (status != ALL_OK) {
            break;
        }

        /* check that the suite exists and it is not a preloaded one */
        pData = get_suite_data(suiteId);

        if (pData == NULL) {
            status = NOT_FOUND;
            break;
        }

        /* notify the listeners that we starting to remove the suite */
        operationStarted = 1;
        suite_listeners_notify(SUITESTORE_LISTENER_TYPE_REMOVE,
            SUITESTORE_OPERATION_START, ALL_OK, pData);

        if (pData->type == COMPONENT_PREINSTALLED_SUITE) {
            status = BAD_PARAMS;
            break;
        }

        status = begin_transaction(TRANSACTION_REMOVE_SUITE, suiteId, NULL);
        if (status != ALL_OK) {
            return status;
        }

        /*
         * Remove the files
         * Call the native RMS method to remove the RMS data.
         * This function call is needed for portability
         */
        rc = rmsdb_remove_record_stores_for_suite(&filenameBase, suiteId);
        if (rc == KNI_FALSE) {
            status = SUITE_LOCKED;
            break;
        }

        pushdeletesuite(suiteId);

        /*
         * If there is a delete notify property, add the value to the delete
         * notify URL list.
         */
        properties = midp_get_suite_properties(suiteId);
        if (properties.numberOfProperties > 0) {
            pproperty = midp_find_property(&properties, &DELETE_NOTIFY_PROP);
            if (pcsl_string_length(pproperty) > 0) {
                midpAddDeleteNotification(suiteId, pproperty);
            }

            pproperty = midp_find_property(&properties, &INSTALL_NOTIFY_PROP);
            if (pcsl_string_length(pproperty) > 0) {
                /*
                 * Remove any pending install notifications since they are only
                 * retried when the suite is run.
                 */
                midpRemoveInstallNotification(suiteId);
            }

            midp_free_properties(&properties);
        }

        if ((status = get_suite_storage_root(suiteId, &suiteRoot)) != ALL_OK) {
            break;
        }

        fileIteratorHandle = storage_open_file_iterator(&suiteRoot);
        if (!fileIteratorHandle) {
            status = IO_ERROR;
            break;
        }

#if ENABLE_ICON_CACHE
        midp_remove_suite_icons(suiteId);
#endif        

        for (;;) {
            rc = storage_get_next_file_in_iterator(&suiteRoot,
                fileIteratorHandle, &filename);
            if (0 != rc) {
                break;
            }
            storage_delete_file(&pszError, &filename);
            pcsl_string_free(&filename);
            if (pszError != NULL) {
                storageFreeError(pszError);
                break;
            }
        }

    } while (0);

    pcsl_string_free(&suiteRoot);
    storageCloseFileIterator(fileIteratorHandle);

    (void)finish_transaction();

    /*
     * Notify the listeners the we've finished removing the suite.
     * It should be done before remove_from_suite_list_and_save()
     * call because it frees pData structure.
     */
    if (operationStarted) {
        suite_listeners_notify(SUITESTORE_LISTENER_TYPE_REMOVE,
            SUITESTORE_OPERATION_END, status, pData);
    }

    if (status == ALL_OK) {
        (void)remove_from_suite_list_and_save(suiteId);
    }

    remove_storage_lock(suiteId);

    return status;
}

/**
 * Moves a software package with given suite ID to the specified storage.
 *
 * @param suiteId suite ID for the installed package
 * @param storageId new storage ID
 *
 * @return SUITE_LOCKED if the
 * suite is locked, NOT_FOUND if the suite cannot be found or
 * invalid storage ID specified, BAD_PARAMS if attempt is made
 * to move suite to the external storage, GENERAL_ERROR if
 * VERIFY_ONCE is not enabled and if MONET is enabled
 */
MIDPError
midp_change_suite_storage(SuiteIdType suiteId, StorageIdType newStorageId) {

#ifndef VERIFY_ONCE
    (void)suiteId;
    (void)newStorageId;
    return GENERAL_ERROR;
#else

   /*
    * if VERIFY_ONCE is enabled then MONET is disabled
    * so we don't have to check ENABLE_MONET.
    */

    {
        pcsl_string suiteRoot;
        MidletSuiteData* pData = NULL;
        int status;
        void* fileIteratorHandle = NULL;
        lockStorageList *node = NULL;

        if ((UNUSED_STORAGE_ID == newStorageId) ||
            (newStorageId >= MAX_STORAGE_NUM)) {
            return BAD_PARAMS;
        }

        /*
         * IMPL_NOTE: for security reasons we allow to move suite
         * only to the internal storage.
         */
        if (newStorageId != INTERNAL_STORAGE_ID) {
            return BAD_PARAMS;
        }

        node = find_storage_lock(suiteId);
        if (node != NULL) {
            if (node->update != KNI_TRUE) {
                return SUITE_LOCKED;
            }
        }

        /*
         * This is a public API which can be called without the VM running
         * so we need automatically init anything needed, to make the
         * caller's code less complex.
         *
         * Initialization is performed in steps so that we do use any
         * extra resources such as the VM for the operation being performed.
         */
        if (midpInit(REMOVE_LEVEL) != 0) {
            remove_storage_lock(suiteId);
            return OUT_OF_MEMORY;
        }

        /* check that the suite exists and it is not a preloaded one */
        pData = get_suite_data(suiteId);

        if (pData == NULL) {
            remove_storage_lock(suiteId);
            return NOT_FOUND;
        }

        if (pData->storageId == newStorageId) {
            remove_storage_lock(suiteId);
            return BAD_PARAMS;
        }

        if (pData->type == COMPONENT_PREINSTALLED_SUITE) {
            remove_storage_lock(suiteId);
            return BAD_PARAMS;
        }

        do {
            jsize oldRootLength;
            jsize newRootLength;
            const pcsl_string* newRoot;
            const pcsl_string* oldRoot;
            char* pszError = NULL;
            pcsl_string filePath;
            pcsl_string newFilePath;


            status = begin_transaction(TRANSACTION_CHANGE_STORAGE, suiteId, NULL);
            if (status != ALL_OK) {
                break;
            }

            if ((status = get_suite_storage_root(suiteId, &suiteRoot)) != ALL_OK) {
                break;
            }

            fileIteratorHandle = storage_open_file_iterator(&suiteRoot);
            if (!fileIteratorHandle) {
                status = IO_ERROR;
                break;
            }

            newRoot = storage_get_root(newStorageId);
            oldRoot = storage_get_root(pData->storageId);
            newRootLength = pcsl_string_length(newRoot);
            oldRootLength = pcsl_string_length(oldRoot);

            status = ALL_OK;

            if ((status = midp_suite_get_class_path(suiteId,
                pData->storageId, KNI_FALSE, &filePath)) != ALL_OK) {
                break;
            }

            if ((status = midp_suite_get_class_path(suiteId,
                newStorageId, KNI_FALSE, &newFilePath)) != ALL_OK) {
                break;
            }

            storage_rename_file(&pszError, &filePath, &newFilePath);
            if (pszError != NULL) {
                status = IO_ERROR;
                storageFreeError(pszError);
                pcsl_string_free(&filePath);
                pcsl_string_free(&newFilePath);
                break;
            }

            pcsl_string_free(&filePath);
            pcsl_string_free(&newFilePath);

#if ENABLE_IMAGE_CACHE
            moveImageCache(suiteId, pData->storageId, newStorageId);
#endif

#if ENABLE_FONT_CACHE
            moveFontCache(suiteId, pData->storageId, newStorageId);
#endif

            pData->storageId = newStorageId;

            status = write_suites_data(&pszError);
            storageFreeError(pszError);

        } while (0);

        pcsl_string_free(&suiteRoot);
        storageCloseFileIterator(fileIteratorHandle);

        if (status != ALL_OK) {
            (void)rollback_transaction();
        } else {
            (void)finish_transaction();
        }

        remove_storage_lock(suiteId);

        return status;
    }

#endif /* VERIFY_ONCE */
}

/**
 * Moves the given midlet suite to another folder.
 *
 * @param suiteId ID of the suite
 * @param newFolderId ID of the folder where the suite must be moved
 *
 * @return ALL_OK if no errors or an error code
 */
MIDPError midp_move_suite_to_folder(SuiteIdType suiteId,
                                    FolderIdType newFolderId) {
    MIDPError status = ALL_OK;
    char* pszError = NULL;
    lockStorageList *node = NULL;
    MidletSuiteData* pSuiteData;
    FolderIdType oldFolderId;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(REMOVE_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    /* load _suites.dat */
    status = read_suites_data(&pszError);
    storageFreeError(pszError);

    if (status != ALL_OK) {
        return status;
    }

    node = find_storage_lock(suiteId);
    if (node != NULL) {
        if (node->update != KNI_TRUE) {
            return SUITE_LOCKED;
        }
    }

    pSuiteData = get_suite_data(suiteId);
    if (pSuiteData == NULL) {
        remove_storage_lock(suiteId);
        return NOT_FOUND;
    }

    status = begin_transaction(TRANSACTION_MOVE_TO_FOLDER, suiteId, NULL);
    if (status != ALL_OK) {
        remove_storage_lock(suiteId);
        return status;
    }

    oldFolderId = pSuiteData->folderId;
    pSuiteData->folderId = newFolderId;

    status = write_suites_data(&pszError);
    storageFreeError(pszError);

    if (status != ALL_OK) {
        pSuiteData->folderId = oldFolderId;
        (void)rollback_transaction();
    } else {
        (void)finish_transaction();
    }

    remove_storage_lock(suiteId);

    return status;
}

/**
 * Gets the amount of storage on the device that this suite is using.
 * This includes the JAD, JAR, management data, and RMS.
 *
 * @param suiteId  ID of the suite
 *
 * @return number of bytes of storage the suite is using or less than
 * OUT_OF_MEM_LEN if out of memory
 */
long
midp_get_suite_storage_size(SuiteIdType suiteId) {
    long used = 0;
    long rms = 0;
    pcsl_string filename[NUM_SUITE_FILES];
    int i;
    char* pszError;
    StorageIdType storageId;
    MIDPError status;
    MidletSuiteData* pData;
    pcsl_string filenameBase;

    // get the filename base from the suite id
    status = build_suite_filename(suiteId, &PCSL_STRING_EMPTY, 
                                            &filenameBase);
    
    if (status != ALL_OK) {
        return status;
    }
    pData = get_suite_data(suiteId);
    if (pData) {
        used = (jint)pData->suiteSize;
    }

    if (used <= 0) {
        /* Suite size is not cached (should not happen!), calculate it. */
        for (i = 0; i < NUM_SUITE_FILES; i++) {
            filename[i] = PCSL_STRING_NULL;
        }

        /*
         * This is a public API which can be called without the VM running
         * so we need automatically init anything needed, to make the
         * caller's code less complex.
         *
         * Initialization is performed in steps so that we do use any
         * extra resources such as the VM for the operation being performed.
         */
        if (midpInit(LIST_LEVEL) != 0) {
            return OUT_OF_MEM_LEN;
        }

        status = midp_suite_get_suite_storage(suiteId, &storageId);
        if (status != ALL_OK) {
            return OUT_OF_MEM_LEN;
        }

        status = build_suite_filename(suiteId, &INSTALL_INFO_FILENAME, &filename[0]);
        if (status != ALL_OK) {
            return status;
        }
        status = build_suite_filename(suiteId, &SETTINGS_FILENAME, &filename[1]);
        if (status != ALL_OK) {
            return status;
        }
        midp_suite_get_class_path(suiteId, storageId, KNI_TRUE, &filename[2]);
        get_property_file(suiteId, KNI_TRUE, &filename[3]);

        for (i = 0; i < NUM_SUITE_FILES; i++) {
            long tmp;

            if (pcsl_string_is_null(&filename[i])) {
                continue;
            }

            tmp = storage_size_of_file_by_name(&pszError, &filename[i]);
            pcsl_string_free(&filename[i]);

            if (pszError != NULL) {
                storageFreeError(pszError);
                continue;
            }

            used += tmp;
        }

        if (pData) {
            /* cache the calculated size */
            pData->suiteSize = (jint)used;
            status = write_suites_data(&pszError);
            if (status != ALL_OK) {
                storageFreeError(pszError);
                return OUT_OF_MEM_LEN;
            }
        }
    }

    rms = rmsdb_get_rms_storage_size(&filenameBase, suiteId);
    if (rms == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    return used + rms;
}

/**
 * Checks the integrity of the suite storage database and of the
 * installed suites.
 *
 * @param fullCheck 0 to check just an integrity of the database,
 *                    other value for full check
 * @param delCorruptedSuites != 0 to delete the corrupted suites,
 *                           0 - to keep them (for re-installation).
 *
 * @return ALL_OK if no errors,
 *         SUITE_CORRUPTED_ERROR if the suite database was corrupted
 *                               but has been successfully repaired,
 *         another error code if the database is corrupted and
 *         could not be repaired
 */
MIDPError
midp_check_suites_integrity(int fullCheck, int delCorruptedSuites) {
    MIDPError status;
    char *pszError = NULL;
    int dbWasCorrupted = 0;

    /* Check if there is a previously started transaction exists. */
    if (unfinished_transaction_exists()) {
        (void)rollback_transaction();
    }

    /* Check if the suite database is corrupted and repair it if needed. */
    status = read_suites_data(&pszError);
    if (status == SUITE_CORRUPTED_ERROR) {
        dbWasCorrupted = 1;
        status = repair_suite_db();
    }
    if (status != ALL_OK) {
        /* give up, user interaction is needed */
        return status;
    }

    /* if fullCheck is true, check all installed suites */
    if (fullCheck) {
        int i, numOfSuites;
        SuiteIdType suiteId, *pSuiteIds = NULL;

        status = midp_get_suite_ids(&pSuiteIds, &numOfSuites);

        if (status == ALL_OK) {
            for (i = 0; i < numOfSuites; i++) {
                suiteId = pSuiteIds[i];

                if (check_for_corrupted_suite(suiteId) ==
                        SUITE_CORRUPTED_ERROR) {
                    dbWasCorrupted = 1;
                    if (delCorruptedSuites) {
                        midp_remove_suite(suiteId);
                    }
                }
            }

            if (pSuiteIds != NULL) {
                midp_free_suite_ids(pSuiteIds, numOfSuites);
            }
        }
    }

    return dbWasCorrupted ? SUITE_CORRUPTED_ERROR : ALL_OK;
}

/* ------------------------------------------------------------ */
/*                          Implementation                      */
/* ------------------------------------------------------------ */

/**
 * Change the enabled state of a suite.
 *
 * @param suiteId ID of the suite
 * @param enabled true if the suite is be enabled
 *
 * @return an error code (ALL_OK if no errors)
 */
static MIDPError
change_enabled_state(SuiteIdType suiteId, jboolean enabled) {
    char* pszError;
    MIDPError status;
    jbyte* pPermissions;
    int numberOfPermissions;
    jbyte pushInterrupt;
    jint pushOptions;
    jboolean temp;
    lockStorageList* node;
    MidletSuiteData* pData;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    status = midp_suite_exists(suiteId);

    if ((status != ALL_OK) && (status != SUITE_CORRUPTED_ERROR)) {
        return status;
    }

    node = find_storage_lock(suiteId);
    if (node != NULL) {
        if (node->update == KNI_TRUE) {
            /* Suite is being updated currently. */
            return SUITE_LOCKED;
        }
    }

    status = read_settings(&pszError, suiteId, &temp, &pushInterrupt,
                           &pushOptions, &pPermissions, &numberOfPermissions);
    if (status != ALL_OK) {
        storageFreeError(pszError);
        return status;
    }

    status = begin_transaction(TRANSACTION_ENABLE_SUITE, suiteId, NULL);
    if (status != ALL_OK) {
        return status;
    }

    status = write_settings(&pszError, suiteId, enabled, pushInterrupt,
                            pushOptions, pPermissions, numberOfPermissions,
                            NULL);
    pcsl_mem_free(pPermissions);
    if (status != ALL_OK) {
        storageFreeError(pszError);
        /* nothing was written, so nothing to rollback, just finish */
        (void)finish_transaction();
        return status;
    }

    /* synchronize the settings in the list of MidletSuiteData structures */
    pData = get_suite_data(suiteId);

    /*
     * We can assert that pData is not NULL because midp_suite_exists()
     * was called above to ensure that the suite with the given ID exists.
     */
    if (pData != NULL) {
        int status;
        char* pszError;
        pData->isEnabled = enabled;

        /* IMPL_NOTE: these settings must be cached and saved on AMS exit. */
        status = write_suites_data(&pszError);
        storageFreeError(pszError);
        if (status != ALL_OK) {
            (void)rollback_transaction();
            return IO_ERROR;
        }
    }

    (void)finish_transaction();

    return ALL_OK;
}
