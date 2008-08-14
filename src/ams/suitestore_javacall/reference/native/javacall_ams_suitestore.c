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
#include <midpEvents.h>
#include <pcsl_string.h>

#include <suitestore_common.h>
#include <suitestore_installer.h>
#include <suitestore_task_manager.h>
#include <suitestore_intern.h>

#include <javautil_unicode.h>
#include <javacall_memory.h>

#include <suitestore_javacall.h>

static javacall_result midp_error2javacall(MIDPError midpErr);
static MIDPError midp_javacall_str2pcsl_str(
    javacall_const_utf16_string pSrcStr, pcsl_string* pDstStr);
static MIDPError midp_pcsl_str2javacall_str(const pcsl_string* pSrcStr,
                                            javacall_utf16_string* pDstStr);

/*----------------------- Suite Storage: Common API -------------------------*/

/**
 * Initializes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_storage_init() {
    return midp_error2javacall(midp_suite_storage_init());
}

/**
 * Finalizes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_storage_cleanup() {
    midp_suite_storage_cleanup();
    return JAVACALL_OK;
}

/**
 * Converts the given suite ID to javacall_string.
 *
 * NOTE: this function returns a pointer to a static buffer!
 *
 * @param value suite id to convert
 *
 * @return javacall_utf16_string representation of the given suite ID
 */
const javacall_utf16_string
java_ams_suite_suiteid2javacall_string(javacall_suite_id value) {
    static javacall_utf16_string jsSuiteId = {0};
    //const pcsl_string* pPcslStrVal =
    //    midp_suiteid2pcsl_string((SuiteIdType)value);
    return jsSuiteId;
}

/**
 * Converts the given suite ID to array of chars.
 *
 * NOTE: this function returns a pointer to a static buffer!
 *
 * @param value suite id to convert
 *
 * @return char[] representation of the given suite ID
 *         or NULL in case of error
 */
const char*
java_ams_suite_suiteid2chars(javacall_suite_id value) {
    return midp_suiteid2chars((SuiteIdType)value);
}

/**
 * Tells if a suite with the given ID exists.
 *
 * @param suiteId ID of a suite
 *
 * @return <tt>JAVACALL_OK</tt> if a suite exists,
 *         <tt>JAVACALL_FILE_NOT_FOUND</tt> if not,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if IO error,
 *         <tt>JAVACALL_FAIL</tt> if the suite is found in the list,
 *         but it's corrupted
 */
javacall_result
java_ams_suite_exists(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_suite_exists((SuiteIdType)suiteId));
}

/**
 * Gets location of the class path for the suite with the specified suiteId.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param pClassPath The in/out parameter that contains returned class path
 *
 * @return error code that should be one of the following:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_FILE_NOT_FOUND,
 *     JAVACALL_INVALID_ARGUMENT, JAVACALL_FAIL (for SUITE_CORRUPTED_ERROR)
 * </pre>
 */
javacall_result
java_ams_suite_get_class_path(javacall_suite_id suiteId,
                              javacall_storage_id storageId,
                              javacall_bool checkSuiteExists,
                              javacall_utf16_string* pClassPath) {
    pcsl_string pcslStrClassPath;
    MIDPError status = midp_suite_get_class_path((SuiteIdType)suiteId,
                          (StorageIdType)storageId,
                          (checkSuiteExists == JAVACALL_TRUE) ?
                              KNI_TRUE : KNI_FALSE,
                          &pcslStrClassPath);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_pcsl_str2javacall_str(&pcslStrClassPath, pClassPath);
    pcsl_string_free(&pcslStrClassPath);

    return midp_error2javacall(status);
}

#if ENABLE_MONET

/**
 * Only for MONET--Gets the class path to binary application image
 * for the suite with the specified MIDlet suite id.
 *
 * It is different from "usual" class path in that class path points to a
 * jar file, while this binary application image path points to a MONET bundle.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param pClassPath The in/out parameter that contains returned class path
 *
 * @return error code that should be one of the following:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_FILE_NOT_FOUND,
 *     JAVACALL_INVALID_ARGUMENT, JAVACALL_FAIL (for SUITE_CORRUPTED_ERROR)
 * </pre>
*/
javacall_result
java_ams_suite_get_bin_app_path(javacall_suite_id suiteId,
                                javacall_storage_id storageId,
                                javacall_utf16_string* pClassPath) {
    pcsl_string pcslStrClassPath;
    MIDPError status = midp_suite_get_bin_app_path((SuiteIdType)suiteId,
                          (StorageIdType)storageId,
                          &pcslStrClassPath);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_pcsl_str2javacall_str(&pcslStrClassPath, pClassPath);
    pcsl_string_free(&pcslStrClassPath);

    return midp_error2javacall(status);
}
#endif /* ENABLE_MONET */

/**
 * Retrieves an ID of the storage where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId    [in]  unique ID of the MIDlet suite
 * @param pStorageId [out] receives an ID of the storage where
 *                         the suite is stored
 *
 * @return <tt>JAVACALL_OK</tt> on success, error code otherwise
 */
javacall_result
java_ams_suite_get_storage(javacall_suite_id suiteId,
                           javacall_storage_id* pStorageId) {
    StorageIdType midpStorageId;
    MIDPError status = midp_suite_get_suite_storage((SuiteIdType)suiteId,
                                                    &midpStorageId);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    *pStorageId = (javacall_storage_id)midpStorageId;

    return JAVACALL_OK;
}

/**
 * Retrieves a path to the root of the given storage.
 *
 * Note that memory for the in/out parameter pStorageRootPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param storageId        [in]  unique ID of the storage where
 *                               the midlet suite resides
 * @param pStorageRootPath [out] receives a path to the given storage's root
 *
 * @return <tt>JAVACALL_OK</tt> on success, error code otherwise
 */
javacall_result
java_ams_storage_get_root(javacall_storage_id storageId,
                          javacall_utf16_string* pStorageRootPath) {
    MIDPError status;                          
    const pcsl_string* pRoot = storage_get_root(storageId);
    if (pRoot == NULL || pRoot == &PCSL_STRING_EMPTY) {
        return JAVACALL_FAIL;
    }

    status = midp_pcsl_str2javacall_str(pRoot, pStorageRootPath);

    return midp_error2javacall(status);
}

/**
 * Retrieves the specified property value of the suite.
 *
 * @param suiteId     [in]  unique ID of the MIDlet suite
 * @param key         [in]  property name
 * @param value       [out] buffer to conatain returned property value
 * @param maxValueLen [in]  buffer length of value
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_get_property(javacall_suite_id suiteId,
                            javacall_const_utf16_string key,
                            javacall_utf16_string value,
                            int maxValueLen) {
    pcsl_string pcslStrKey, pcslStrValue;
    MIDPError status;

    status = midp_javacall_str2pcsl_str(key, &pcslStrKey);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_get_suite_property((SuiteIdType)suiteId,
                        &pcslStrKey, &pcslStrValue);

    return JAVACALL_OK;
}

/*----------------- Suite Storage: interface to Installer -------------------*/

/**
 * Installer invokes this function. If the suite exists, this function
 * returns an unique identifier of MIDlet suite. Note that suite may be
 * corrupted even if it exists. If the suite doesn't exist, a new suite
 * ID is created.
 *
 * @param name     [in]  name of suite
 * @param vendor   [in]  vendor of suite
 * @param pSuiteId [out] suite ID of the existing suite or a new suite ID
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_id(javacall_const_utf16_string vendor,
                      javacall_const_utf16_string name,
                      javacall_suite_id* pSuiteId) {
    SuiteIdType midpSuiteId;
    MIDPError status;
    pcsl_string pcslStrVendor, pcslStrName;

    status = midp_javacall_str2pcsl_str(vendor, &pcslStrVendor);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_javacall_str2pcsl_str(name, &pcslStrName);
    if (status != ALL_OK) {
        pcsl_string_free(&pcslStrVendor);
        return midp_error2javacall(status);
    }

    status = midp_get_suite_id(&pcslStrVendor, &pcslStrName, &midpSuiteId);

    pcsl_string_free(&pcslStrVendor);
    pcsl_string_free(&pcslStrName);

    if (status != ALL_OK && status != NOT_FOUND) {
        return midp_error2javacall(status);
    }

    *pSuiteId = (javacall_suite_id)midpSuiteId;

    return JAVACALL_OK;
}

/**
 * Returns a new unique identifier of MIDlet suite.
 *
 * @param pSuiteId [out] receives a new unique MIDlet suite identifier
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_create_id(javacall_suite_id* pSuiteId) {
    SuiteIdType midpSuiteId;
    MIDPError status;

    status = midp_create_suite_id(&midpSuiteId);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    *pSuiteId = (javacall_suite_id)midpSuiteId; 

    return JAVACALL_OK;
}

/**
 * Installer invokes this function to get the install information
 * of a MIDlet suite.
 *
 * Note that memory for the strings inside the returned
 * javacall_ams_suite_install_info structure is allocated by the callee,
 * and the caller is responsible for freeing it using
 * javacall_free_install_info().
 *
 * @param suiteId      [in]     unique ID of the suite
 * @param pInstallInfo [in/out] pointer to a place where the installation
 *                              information will be saved.
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_get_install_info(javacall_suite_id suiteId,
                                javacall_ams_suite_install_info*
                                    pInstallInfo) {
    MidpInstallInfo midpInstallInfo;
    MIDPError status;

    if (pInstallInfo == NULL) {
        return JAVACALL_FAIL;
    }

    do {
        midpInstallInfo = midp_get_suite_install_info((SuiteIdType)suiteId);
        status = midpInstallInfo.status;
        if (status != ALL_OK) {
            break;
        }

        status = midp_pcsl_str2javacall_str(&midpInstallInfo.jadUrl_s,
                                            &pInstallInfo->jadUrl);
        if (status != ALL_OK) {
            break;
        }

        status = midp_pcsl_str2javacall_str(&midpInstallInfo.jarUrl_s,
                                            &pInstallInfo->jarUrl);
        if (status != ALL_OK) {
            javacall_free(pInstallInfo->jadUrl);
            break;
        }

        status = midp_pcsl_str2javacall_str(&midpInstallInfo.domain_s,
                                            &pInstallInfo->domain);
        if (status != ALL_OK) {
            javacall_free(pInstallInfo->jadUrl);
            javacall_free(pInstallInfo->jarUrl);
            break;
        }

        /** true if suite is trusted, false if not */
        pInstallInfo->isTrusted = (midpInstallInfo.trusted == KNI_TRUE) ?
            JAVACALL_TRUE : JAVACALL_FALSE;

        pInstallInfo->authPathLen = (javacall_int32)midpInstallInfo.authPathLen;
        if (midpInstallInfo.authPathLen > 0) {
            // TODO !!!
        } else {
            pInstallInfo->authPath = NULL;
        }

        pInstallInfo->verifyHashLen = midpInstallInfo.verifyHashLen;

        if (midpInstallInfo.verifyHashLen > 0) {
            pInstallInfo->pVerifyHash = (javacall_uint8*)
                javacall_malloc(midpInstallInfo.verifyHashLen);
            if (pInstallInfo->pVerifyHash == NULL) {
                javacall_free(pInstallInfo->jadUrl);
                javacall_free(pInstallInfo->jarUrl);
                javacall_free(pInstallInfo->domain);
                status = OUT_OF_MEMORY;
                break;
            }

            memcpy((unsigned char*)pInstallInfo->pVerifyHash,
                (unsigned char*)midpInstallInfo.pVerifyHash,
                    midpInstallInfo.verifyHashLen);
        } else {
            pInstallInfo->pVerifyHash = NULL;
        }

        // TODO !!!
        pInstallInfo->jadProps.numberOfProperties = 0;
//            midpInstallInfo.jadProps.numberOfProperties;

        pInstallInfo->jarProps.numberOfProperties = 0;
//            midpInstallInfo.jarProps.numberOfProperties;
    } while (0);

    midp_free_install_info(&midpInstallInfo);

    return midp_error2javacall(status);
}

/**
 * Installer invokes this function to frees an
 * javacall_ams_suite_install_info struct.
 * Does nothing if passed NULL.
 *
 * @param pInstallInfo installation information returned from
 *                     java_ams_suite_get_install_info
 */
void java_ams_suite_free_install_info(
        javacall_ams_suite_install_info* pInstallInfo) {
    if (pInstallInfo != NULL) {
        int i;

        if (pInstallInfo->jadUrl != NULL) {
            javacall_free(pInstallInfo->jadUrl);
        }

        if (pInstallInfo->jarUrl != NULL) {
            javacall_free(pInstallInfo->jarUrl);
        }

        if (pInstallInfo->domain != NULL) {
            javacall_free(pInstallInfo->domain);
        }

        if (pInstallInfo->verifyHashLen > 0 &&
                pInstallInfo->pVerifyHash != NULL) {
            javacall_free(pInstallInfo->pVerifyHash);
        }

        for (i = 0; i < pInstallInfo->authPathLen; i++) {
            javacall_free(&pInstallInfo->authPath[i]);
        }

        javacall_free(pInstallInfo);
    }
}

/**
 * Installer calls this function to store or update a midlet suite.
 *
 * @param pInstallInfo structure containing the following information:<br>
 * <pre>
 *     id - unique ID of the suite;
 *     jadUrl - where the JAD came from, can be null;
 *     jarUrl - where the JAR came from;
 *     jarFilename - name of the downloaded MIDlet suite jar file;
 *     suiteName - name of the suite;
 *     suiteVendor - vendor of the suite;
 *     authPath - authPath if signed, the authorization path starting
 *                with the most trusted authority;
 *     domain - security domain of the suite;
 *     isTrusted - true if suite is trusted;
 *     verifyHash - may contain hash value of the suite with
 *                  preverified classes or may be NULL;
 *     jadProps - properties given in the JAD as an array of strings in
 *                key/value pair order, can be null if jadUrl is null
 *     jarProps - properties of the manifest as an array of strings
 *                in key/value pair order
 * </pre>
 *
 * @param pSuiteSettings structure containing the following information:<br>
 * <pre>
 *     permissions - permissions for the suite;
 *     pushInterruptSetting - defines if this MIDlet suite interrupt
 *                            other suites;
 *     pushOptions - user options for push interrupts;
 *     suiteId - unique ID of the suite, must be equal to the one given
 *               in installInfo;
 *     boolean enabled - if true, MIDlet from this suite can be run;
 * </pre>
 *
 * @param pSuiteInfo structure containing the following information:<br>
 * <pre>
 *     suiteId - unique ID of the suite, must be equal to the value given
 *               in installInfo and suiteSettings parameters;
 *     storageId - ID of the storage where the MIDlet should be installed;
 *     numberOfMidlets - number of midlets in the suite;
 *     displayName - the suite's name to display to the user;
 *     midletToRunClassName - the midlet's class name if the suite contains
 *                            only one midlet, ignored otherwise;
 *     iconName - name of the icon for this suite.
 * </pre>
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_store_suite(const javacall_ams_suite_install_info* pInstallInfo,
                           const javacall_ams_suite_settings* pSuiteSettings,
                           const javacall_ams_suite_info* pSuiteInfo) {
    return JAVACALL_OK;
}

/*------------- Suite Storage: interface to Application Manager -------------*/

/**
 * App Manager invokes this function to get the number of MIDlet suites
 * currently installed.
 *
 * @param pNumbefOfSuites [out] pointer to location where the number
 *                              of the installed suites will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_suites_number(int* pNumberOfSuites) {
    return midp_error2javacall(midp_get_number_of_suites(pNumberOfSuites));
}

/**
 * AppManager invokes this function to get the list of IDs
 * of the installed MIDlet suites.
 *
 * Note that memory for the suite IDs is allocated by the callee,
 * and the caller is responsible for freeing it using
 * java_ams_suite_free_ids().
 *
 * @param ppSuiteIds      [out] on exit will hold an address of the array
 *                              containing suite IDs
 * @param pNumberOfSuites [out] pointer to variable to accept the number
 *                              of suites in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if an IO error
 */
javacall_result
java_ams_suite_get_suite_ids(javacall_suite_id** ppSuiteIds,
                             int* pNumberOfSuites) {
    int numberOfSuites;
    SuiteIdType* pSuites;
    MIDPError status;
    int i;

    status = midp_get_suite_ids(&pSuites, &numberOfSuites);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    if (numberOfSuites == 0) {
        *pNumberOfSuites = 0;
        return JAVACALL_OK;
    }

    *ppSuiteIds = (javacall_suite_id*)
        javacall_malloc(numberOfSuites * sizeof(javacall_suite_id));
    if (*ppSuiteIds == NULL) {
        midp_free_suite_ids(pSuites, numberOfSuites);
        return JAVACALL_OUT_OF_MEMORY;
    }

    for (i = 0; i < numberOfSuites; i++) {
        (*ppSuiteIds)[i] = (javacall_suite_id) pSuites[i];    
    }

    *pNumberOfSuites = numberOfSuites;
    midp_free_suite_ids(pSuites, numberOfSuites);

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free a list of suite IDs allocated
 * during the previous call of java_ams_suite_get_suite_ids().
 *
 * @param pSuiteIds points to an array of suite IDs
 * @param numberOfSuites number of elements in pSuites
 */
void
java_ams_suite_free_suite_ids(javacall_suite_id* pSuiteIds,
                              int numberOfSuites) {
    (void) numberOfSuites;
    if (pSuiteIds != NULL) {
        javacall_free(pSuiteIds);                              
    }
}

/**
 * App Manager invokes this function to get a information about the midlets
 * contained in the given suite.
 *
 * @param suiteId          [in]  unique ID of the MIDlet suite
 * @param ppMidletsInfo    [out] on exit will hold an address of the array
 *                               containing the midlets info
 * @param pNumberOfEntries [out] number of entries in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_midlets_info(javacall_suite_id suiteId,
                                javacall_ams_midlet_info** ppMidletsInfo,
                                int* pNumberOfEntries) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free an array of structures describing
 * midlets from the given suite.
 *
 * @param pMidletsInfo points to an array with midlets info
 * @param numberOfEntries number of elements in pMidletsInfo
 */
void
java_ams_suite_free_midlets_info(javacall_ams_midlet_info* pMidletsInfo,
                                 int numberOfEntries) {
}

/**
 * App Manager invokes this function to get information about the midlet suite
 * having the given ID. This call is synchronous.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @param ppSuiteInfo [out] on exit will hold a pointer to a structure where the
 *                          information about the given midlet suite is stored;
 *                          the caller is responsible for freeing this structure
 *                          using java_ams_suite_free_info() when it is not
 *                          needed anymore
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successful,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_info(javacall_suite_id suiteId,
                        javacall_ams_suite_info** ppSuiteInfo) {
    MidletSuiteData* pMidpSuiteData;
    javacall_ams_suite_info* pTmpSuiteInfo;

    pTmpSuiteInfo = javacall_malloc(sizeof(javacall_ams_suite_info));
    if (pTmpSuiteInfo == NULL) {
        return JAVACALL_FAIL;
    }

    /* IMPL_NOTE: it should be moved from suitestore_intern and renamed */
    pMidpSuiteData = get_suite_data((SuiteIdType)suiteId);
    if (pMidpSuiteData == NULL) {
        javacall_free(pTmpSuiteInfo);
        return JAVACALL_FAIL;
    }

    /* copy data from the midp structure to the javacall one */
    pTmpSuiteInfo->suiteId   = (javacall_suite_id) pMidpSuiteData->suiteId;
    pTmpSuiteInfo->storageId = (javacall_int32) pMidpSuiteData->storageId;
    pTmpSuiteInfo->folderId = (javacall_int32) pMidpSuiteData->folderId;
    pTmpSuiteInfo->isEnabled = (javacall_bool) pMidpSuiteData->isEnabled;
    pTmpSuiteInfo->isTrusted = (javacall_bool) pMidpSuiteData->isTrusted;
    pTmpSuiteInfo->numberOfMidlets =
        (javacall_int32) pMidpSuiteData->numberOfMidlets;
    pTmpSuiteInfo->installTime = (long) pMidpSuiteData->installTime;
    pTmpSuiteInfo->jadSize = (javacall_int32) pMidpSuiteData->jadSize;
    pTmpSuiteInfo->jarSize = (javacall_int32) pMidpSuiteData->jarSize;
    pTmpSuiteInfo->isPreinstalled =
        (javacall_bool) pMidpSuiteData->isPreinstalled;

    /*
     * IMPL_NOTE: the strings from pMidpSuiteData should be converted from
     *            pcsl_string and copied into the bellowing strings.
     */
    pTmpSuiteInfo->midletClassName = NULL;
    pTmpSuiteInfo->displayName = NULL;
    pTmpSuiteInfo->iconPath = NULL;
    pTmpSuiteInfo->suiteVendor = NULL;
    pTmpSuiteInfo->suiteName = NULL;
    pTmpSuiteInfo->suiteVersion = NULL;
//    pTmpSuiteInfo->pathToJar = NULL;

    *ppSuiteInfo = pTmpSuiteInfo;

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free a structure describing
 * a midlet suite and all fields of this structure.
 *
 * @param pSuiteInfo points to a structure holding midlet suite info
 */
void
java_ams_suite_free_info(javacall_ams_suite_info* pSuiteInfo) {
}

/**
 * App Manager invokes this function to get the domain the suite belongs to.
 *
 * @param suiteId unique ID of the MIDlet suite
 * @param pDomainId [out] pointer to the location where the retrieved domain
 *                        information will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_domain(javacall_suite_id suiteId,
                          javacall_ams_domain* pDomainId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to disable a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param suiteId ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
javacall_result
java_ams_suite_disable(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_disable_suite((SuiteIdType)suiteId));
}

/**
 * App Manager invokes this function to enable a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param suiteId ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
javacall_result
java_ams_suite_enable(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_enable_suite((SuiteIdType)suiteId));
}

/**
 * Java invokes this function to get permissions of the suite.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [out] pointer to a javacall_ams_permission_set structure
 *                            to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_get_permissions(javacall_suite_id suiteId,
                               javacall_ams_permission_set* pPermissions) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set a single permission of the suite
 * when user changes it.
 *
 * @param suiteId     unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_set_permission(javacall_suite_id suiteId,
                              javacall_ams_permission permission,
                              javacall_ams_permission_val value) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set permissions of the suite.
 *
 * @param suiteId       [in]  Unique ID of the MIDlet suite
 * @param pPermissions  [out] Pointer to a javacall_ams_permission_set structure
 *                            to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_set_permissions(javacall_suite_id suiteId,
                               javacall_ams_permission_set* pPermissions) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to remove a suite with the given ID.
 * This call is synchronous.
 *
 * @param suiteId ID of the suite to remove
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successfull,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_remove(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_remove_suite((SuiteIdType)suiteId));
}

/**
 * App Manager invokes this function to move a software package with given
 * suite ID to the specified storage.
 *
 * @param suiteId suite ID for the installed package
 * @param newStorageId new storage ID
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_change_storage(javacall_suite_id suiteId,
                              javacall_storage_id newStorageId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to check if the suite with the given ID
 * is trusted.
 *
 * This is just a helper method, java_ams_suite_get_info()
 * also can be used for this purpose.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @return <tt>JAVACALL_TRUE</tt> if the suite is trusted,
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool
java_ams_suite_is_preinstalled(javacall_suite_id suiteId) {
    return JAVACALL_FALSE;
}

/**
 * App Manager invokes this function to get the amount of storage
 * on the device that this suite is using.
 * This includes the JAD, JAR, management data and RMS.
 *
 * @param suiteId ID of the suite
 *
 * @return number of bytes of storage the suite is using or less than
 *         0 if out of memory
 */
long java_ams_suite_get_storage_size(javacall_suite_id suiteId) {
    return 0;
}

/**
 * App Manager invokes this function to check the integrity of the suite
 * storage database and of the installed suites.
 *
 * @param fullCheck 0 to check just an integrity of the database,
 *                    other value for full check
 * @param delCorruptedSuites != 0 to delete the corrupted suites,
 *                           0 - to keep them (for re-installation).
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_check_suites_integrity(javacall_bool fullCheck,
                                      javacall_bool delCorruptedSuites) {
    return JAVACALL_OK;
}

/*------------- Getting Information About AMS Folders ---------------*/

/**
 * App Manager invokes this function to get an information about
 * the AMS folders currently defined.
 *
 * @param ppFoldersInfo    [out]  on exit will hold an address of the array
 *                                containing the folders info
 * @param pNumberOfEntries [out] number of entries in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_all_folders_info(javacall_ams_folder_info** ppFoldersInfo,
                                    int* pNumberOfEntries) {
    if (ppFoldersInfo == NULL || pNumberOfEntries == NULL) {
        return JAVACALL_FAIL;
    }

    *pNumberOfEntries = 0;

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free an array of structures describing
 * the AMS folders.
 *
 * @param pFoldersInfo points to an array with midlets info
 * @param numberOfEntries number of elements in pFoldersInfo
 */
void
java_ams_suite_free_all_folders_info(javacall_ams_folder_info* pFoldersInfo,
                                     int numberOfEntries) {
    if (pFoldersInfo != NULL) {
        int i;
        for (i = 0; i < numberOfEntries; i++) {
            java_ams_suite_free_folder_info(&pFoldersInfo[i]);
        }
    }
}

/**
 * App Manager invokes this function to get an information about
 * the given AMS folder.
 *
 * Note that memory for the out parameter pFolderInfo and its fields is
 * allocated by the callee. The caller is responsible for freeing it using
 * java_ams_suite_free_folder_info().
 *
 * @param folderId    [in]  unique ID of the MIDlet suite
 * @param pFolderInfo [out] on exit will hold a pointer to a structure
 *                          describing the given folder
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_folder_info(javacall_folder_id folderId,
                               javacall_ams_folder_info* pFolderInfo) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free the given structure holding
 * an information about an AMS folder.
 *
 * @param pFolderInfo [in] a pointer to the structure that must be freed
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
void
java_ams_suite_free_folder_info(javacall_ams_folder_info* pFolderInfo) {
    if (pFolderInfo != NULL) {
        if (pFolderInfo->folderName != NULL) {
            javacall_free(pFolderInfo->folderName);
        }
        javacall_free(pFolderInfo);
    }
}

/**
 * AppManager invokes this function to get the list of IDs
 * of the installed MIDlet suites.
 *
 * Note that memory for the suite IDs is allocated by the callee,
 * and the caller is responsible for freeing it using
 * java_ams_suite_free_ids().
 *
 * @param folderId        [in]  unique ID of the folder
 * @param ppSuiteIds      [out] on exit will hold an address of the array
 *                              containing suite IDs
 * @param pNumberOfSuites [out] pointer to variable to accept the number
 *                              of suites in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if an IO error
 */
javacall_result
java_ams_suite_get_suites_in_folder(javacall_folder_id folderId,
                                    javacall_suite_id** ppSuiteIds,
                                    int* pNumberOfSuites) {
    if (folderId == JAVACALL_ROOT_FOLDER_ID) {
        return java_ams_suite_get_suite_ids(ppSuiteIds,
                                            pNumberOfSuites);
    } else {
        *pNumberOfSuites = 0;
    }

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to get an ID of the AMS folder where
 * the given suite resides.
 *
 * @param suiteId         [in]  unique ID of the MIDlet suite
 * @param pSuiteFolderId  [out] pointer to a place where the folder ID
 *                              will be stored
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_folder(javacall_suite_id suiteId,
                          javacall_folder_id* pSuiteFolderId) {
    *pSuiteFolderId = JAVACALL_ROOT_FOLDER_ID;                          
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to move the given midlet suite
 * to another folder.
 *
 * @param suiteId ID of the suite
 * @param newFolderId ID of the folder where the suite must be moved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_move_to_folder(javacall_suite_id suiteId,
                              javacall_folder_id newFolderId) {
    return JAVACALL_OK;
}

/*-------------------- API to read/write secure resources -------------------*/

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter ppReturnValue using javacall_malloc().
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param ppReturnValue     The in/out parameter that will return the
 *                          value part of the requested secure resource
 *                          (NULL is a valid return value)
 * @param pValueSize        The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_INVALID_ARGUMENT,
       JAVACALL_FAIL (for NOT_FOUND and SUITE_CORRUPTED_ERROR MIDP errors).
 * </pre>
 */
javacall_result
java_ams_suite_read_secure_resource(javacall_suite_id suiteId,
                                    javacall_const_utf16_string resourceName,
                                    javacall_uint8** ppReturnValue,
                                    javacall_int32* pValueSize) {
    return JAVACALL_OK;
}

/**
 * Writes named secure resource of the suite with specified suiteId
 * to secure persistent storage.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param pCalue            The value part of the secure resource to be stored
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_INVALID_ARGUMENT,
 *     JAVACALL_FAIL (for NOT_FOUND and SUITE_CORRUPTED_ERROR MIDP errors).
 * </pre>
 */
javacall_result
java_ams_suite_write_secure_resource(javacall_suite_id suiteId,
                           javacall_const_utf16_string resourceName,
                           javacall_uint8* pValue,
                           javacall_int32 valueSize) {
    return JAVACALL_OK;
}

/******************************************************************************/

/**
 * Converts the given MIDP error code to the Javacall error code.
 *
 * @param midpErr MIDP error code
 *
 * @return Javacall error code corresponding to the given MIDP one
 */
static javacall_result midp_error2javacall(MIDPError midpErr) {
    javacall_result jcRes;

    /*
     * We can't assume that the MIDP and Javacall constants are identical,
     * so this switch is used.
     */
    switch (midpErr) {
        case ALL_OK: {
            jcRes = JAVACALL_OK;
            break;
        }
        case NOT_FOUND: {
            jcRes = JAVACALL_FILE_NOT_FOUND;
            break;
        }
        case OUT_OF_MEMORY: {
            jcRes = JAVACALL_OUT_OF_MEMORY;
            break;
        }
        case IO_ERROR: {
            jcRes = JAVACALL_IO_ERROR;
            break;
        }
        case SUITE_CORRUPTED_ERROR: {
            jcRes = JAVACALL_FAIL;
            break;
        }
        default: {
            jcRes = JAVACALL_FAIL;
        }
    }

    return jcRes;
}

MIDPError midp_javacall_str2pcsl_str(javacall_const_utf16_string pSrcStr,
                                     pcsl_string* pDstStr) {
    return ALL_OK;
}

MIDPError midp_pcsl_str2javacall_str(const pcsl_string* pSrcStr,
                                     javacall_utf16_string* pDstStr) {
    jsize len = pcsl_string_utf16_length(pSrcStr);
    const jchar* pBuf = pcsl_string_get_utf16_data(pSrcStr);

    if (len > 0) {
        len = (len + 1) << 1; /* length in bytes, + 1 for terminating \0\0 */
        *pDstStr = (javacall_utf16_string)javacall_malloc(len);

        if (*pDstStr == NULL) {
            pcsl_string_release_utf16_data(pBuf, pSrcStr);
            return OUT_OF_MEMORY;
        }

        memcpy((unsigned char*)*pDstStr, (const unsigned char*)pBuf, len);
    } else {
        *pDstStr = NULL;
    }

    pcsl_string_release_utf16_data(pBuf, pSrcStr);

    return ALL_OK;
}
