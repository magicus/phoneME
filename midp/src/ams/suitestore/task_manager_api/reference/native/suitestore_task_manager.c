/*
 *
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

#include <suitestore_intern.h>
#include <suitestore_locks.h>
#include <suitestore_otanotifier_db.h>
#include <suitestore_listeners.h>

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
            *pNumOfSuites = g_numberOfSuites;
        } else {
            storageFreeError(pszError);
        }
    } while(0);

    return status;
}

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
        pSuiteIds[numberOfSuites] = pData->suiteId;
        pData = pData->nextEntry;
        numberOfSuites++;
    }

    if (numberOfSuites != g_numberOfSuites) {
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
    int status;
    void* fileIteratorHandle = NULL;
    MidpProperties properties;
    pcsl_string* pproperty;
    MidletSuiteData* pData = NULL;

    lockStorageList *node = NULL;
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
        suite_listeners_notify(SUITESTORE_LISTENER_TYPE_REMOVE,
            SUITESTORE_OPERATION_START, ALL_OK, pData);

        if (pData->isPreinstalled) {
            status = BAD_PARAMS;
            break;
        }

        /*
         * Remove the files
         * Call the native RMS method to remove the RMS data.
         * This function call is needed for portability
         */
        rc = rmsdb_remove_record_stores_for_suite(suiteId);
        if (rc == KNI_FALSE) {
            status = SUITE_LOCKED;
            break;
        }

        /*
        if (midpRemoveRecordStoresForSuite(suiteId) == KNI_FALSE) {
            return SUITE_LOCKED;
        }
        */

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

    /*
     * Notify the listeners the we finished removing the suite.
     * It should be done before remove_from_suite_list_and_save()
     * call because it frees pData structure.
     */
    suite_listeners_notify(SUITESTORE_LISTENER_TYPE_REMOVE,
        SUITESTORE_OPERATION_END, status, pData);

    (void)remove_from_suite_list_and_save(suiteId);
    remove_storage_lock(suiteId);

    return ALL_OK;
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
    int handle;
    char* pszError;
    char* pszTemp;
    StorageIdType storageId;
    MIDPError status;

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

    build_suite_filename(suiteId, &INSTALL_INFO_FILENAME, &filename[0]);
    build_suite_filename(suiteId, &SETTINGS_FILENAME, &filename[1]);
    midp_suite_get_class_path(suiteId, storageId, KNI_TRUE, &filename[2]);
    get_property_file(suiteId, KNI_TRUE, &filename[3]);

    for (i = 0; i < NUM_SUITE_FILES; i++) {
        if (pcsl_string_is_null(&filename[i])) {
            continue;
        }

        handle = storage_open(&pszError, &filename[i], OPEN_READ);
        pcsl_string_free(&filename[i]);
        if (pszError != NULL) {
            storageFreeError(pszError);
            continue;
        }

        used += storageSizeOf(&pszError, handle);
        storageFreeError(pszError);

        storageClose(&pszTemp, handle);
        storageFreeError(pszTemp);
    }

    rms = rmsdb_get_rms_storage_size(suiteId);
    if (rms == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    return used + rms;
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
        return status;
    }

    status = write_settings(&pszError, suiteId, enabled, pushInterrupt,
                            pushOptions, pPermissions, numberOfPermissions);
    pcsl_mem_free(pPermissions);
    if (status != ALL_OK) {
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
            return IO_ERROR;
        }
    }

    return ALL_OK;
}
