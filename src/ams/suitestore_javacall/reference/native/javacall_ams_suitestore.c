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
#include <pcsl_memory.h>

#include <suitestore_common.h>
#include <suitestore_installer.h>
#include <suitestore_task_manager.h>
#include <suitestore_secure.h>
#include <suitestore_intern.h>

#include <javautil_unicode.h>
#include <javacall_memory.h>

#include <javacall_util.h>
#include <suitestore_javacall.h>

static MIDPError parse_midlet_attr(const pcsl_string* pMIDletAttrValue,
                                   javacall_utf16_string* pDisplayName,
                                   javacall_utf16_string* pIconName,
                                   javacall_utf16_string* pClassName);
static javacall_ams_permission midp_permission2javacall(int midpPermissionId);
static javacall_ams_permission_val
midp_permission_val2javacall(jbyte midpPermissionVal);

static int javacall_permission2midp(javacall_ams_permission jcPermission);
static jbyte
javacall_permission_val2midp(javacall_ams_permission_val jcPermissionVal);

/*----------------------- Suite Storage: Common API -------------------------*/

/**
 * Initializes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_suite_storage_init() {
    return midp_error2javacall(midp_suite_storage_init());
}

/**
 * Finalizes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_suite_storage_cleanup() {
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
javanotify_ams_suite_suiteid2javacall_string(javacall_suite_id value) {
    static javacall_utf16 jsSuiteId[GET_SUITE_ID_LEN(value) + 1];
    jsize convertedLen;
    pcsl_string_status pcslRes;
    const pcsl_string* pPcslStrVal =
        midp_suiteid2pcsl_string((SuiteIdType)value);

    pcslRes = pcsl_string_convert_to_utf16(pPcslStrVal, jsSuiteId,
        (jsize)(sizeof(jsSuiteId) / sizeof(javacall_utf16)), &convertedLen);
    if (pcslRes != PCSL_STRING_OK) {
        jsSuiteId[0] = 0;
    }

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
javanotify_ams_suite_suiteid2chars(javacall_suite_id value) {
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
javanotify_ams_suite_exists(javacall_suite_id suiteId) {
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
javanotify_ams_suite_get_class_path(javacall_suite_id suiteId,
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
javanotify_ams_suite_get_bin_app_path(javacall_suite_id suiteId,
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_storage(javacall_suite_id suiteId,
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_storage_get_root(javacall_storage_id storageId,
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_property(javacall_suite_id suiteId,
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_id(javacall_const_utf16_string vendor,
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_create_id(javacall_suite_id* pSuiteId) {
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_install_info(javacall_suite_id suiteId,
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

        /* convert the authorization path */
        pInstallInfo->authPathLen = (javacall_int32)midpInstallInfo.authPathLen;
        if (midpInstallInfo.authPathLen > 0) {
            status = pcsl_string_array2javacall(
                midpInstallInfo.authPath_as, midpInstallInfo.authPathLen,
                &pInstallInfo->pAuthPath, &pInstallInfo->authPathLen);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pInstallInfo->pAuthPath = NULL;
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

        /* converting the properties from JAD */
        if (pInstallInfo->jadUrl != NULL) {
            pInstallInfo->jadProps.numberOfProperties =
                (javacall_int32)midpInstallInfo.jadProps.numberOfProperties;

            if (midpInstallInfo.jadProps.numberOfProperties > 0) {
                status = pcsl_string_array2javacall(
                    midpInstallInfo.jadProps.pStringArr,
                    midpInstallInfo.jadProps.numberOfProperties,
                    &pInstallInfo->jadProps.pStringArr,
                    &pInstallInfo->jadProps.numberOfProperties);
                if (status != ALL_OK) {
                    break;
                }
            } else {
                pInstallInfo->jadProps.pStringArr = NULL;
            }
        } else {
            pInstallInfo->jadProps.numberOfProperties = 0;
            pInstallInfo->jadProps.pStringArr = NULL;
        }

        /* converting the properties from JAR */
        pInstallInfo->jarProps.numberOfProperties =
            (javacall_int32)midpInstallInfo.jarProps.numberOfProperties;

        if (midpInstallInfo.jarProps.numberOfProperties > 0) {
            status = pcsl_string_array2javacall(
                midpInstallInfo.jarProps.pStringArr,
                midpInstallInfo.jarProps.numberOfProperties,
                &pInstallInfo->jarProps.pStringArr,
                &pInstallInfo->jarProps.numberOfProperties);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pInstallInfo->jarProps.pStringArr = NULL;
        }

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
 *                     javanotify_ams_suite_get_install_info
 */
void javanotify_ams_suite_free_install_info(
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

        /* free the auth. path */
        if (pInstallInfo->pAuthPath != NULL) {
            for (i = 0; i < pInstallInfo->authPathLen; i++) {
                javacall_free(&pInstallInfo->pAuthPath[i]);
            }

            javacall_free(pInstallInfo->pAuthPath);
        }

        /* free the properties from JAD */
        if (pInstallInfo->jadProps.pStringArr != NULL) {
            for (i = 0; i < pInstallInfo->jadProps.numberOfProperties; i++) {
                if (pInstallInfo->jadProps.pStringArr[i] != NULL) {
                    javacall_free(pInstallInfo->jadProps.pStringArr[i]);
                }
            }

            javacall_free(pInstallInfo->jadProps.pStringArr);
        }

        /* free the properties from JAR */
        if (pInstallInfo->jarProps.pStringArr != NULL) {
            for (i = 0; i < pInstallInfo->jarProps.numberOfProperties; i++) {
                if (pInstallInfo->jarProps.pStringArr[i] != NULL) {
                    javacall_free(pInstallInfo->jarProps.pStringArr[i]);
                }
            }

            javacall_free(pInstallInfo->jarProps.pStringArr);
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_store_suite(
                           const javacall_ams_suite_install_info* pInstallInfo,
                           const javacall_ams_suite_settings* pSuiteSettings,
                           const javacall_ams_suite_info* pSuiteInfo) {
    /*
     * IMPL_NOTE: to be implemented! Currently it's not critical to have this
     *            function implemented, it is required only for the case when
     *            the installer is located on the Platform's side, but the suite
     *            strorage - on our side.
     */
    return JAVACALL_NOT_IMPLEMENTED;
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
javanotify_ams_suite_get_suites_number(int* pNumberOfSuites) {
    return midp_error2javacall(midp_get_number_of_suites(pNumberOfSuites));
}

/**
 * AppManager invokes this function to get the list of IDs
 * of the installed MIDlet suites.
 *
 * Note that memory for the suite IDs is allocated by the callee,
 * and the caller is responsible for freeing it using
 * javanotify_ams_suite_free_ids().
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
javanotify_ams_suite_get_suite_ids(javacall_suite_id** ppSuiteIds,
                                   int* pNumberOfSuites) {
    int numberOfSuites;
    SuiteIdType* pSuites;
    MIDPError status;
    int i;

    if (pNumberOfSuites == NULL) {
        return JAVACALL_FAIL;
    }

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
 * during the previous call of javanotify_ams_suite_get_suite_ids().
 *
 * @param pSuiteIds points to an array of suite IDs
 * @param numberOfSuites number of elements in pSuites
 */
void
javanotify_ams_suite_free_suite_ids(javacall_suite_id* pSuiteIds,
                                    int numberOfSuites) {
    if (pSuiteIds != NULL && numberOfSuites > 0) {
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_midlets_info(javacall_suite_id suiteId,
                                      javacall_ams_midlet_info** ppMidletsInfo,
                                      int* pNumberOfEntries) {
    javacall_ams_midlet_info* pTmpMidletInfo;
    MidletSuiteData* pMidpSuiteData;
    MIDPError status;
    jint n;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(midletStr)
    {'M', 'I', 'D', 'l', 'e', 't', '-', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(midletStr);

    if (ppMidletsInfo == NULL || pNumberOfEntries == NULL) {
        return JAVACALL_FAIL;
    }

    pMidpSuiteData = get_suite_data((SuiteIdType)suiteId);
    if (pMidpSuiteData == NULL || pMidpSuiteData->numberOfMidlets == 0) {
        return JAVACALL_FAIL;
    }

    *pNumberOfEntries = pMidpSuiteData->numberOfMidlets;
    *ppMidletsInfo = javacall_malloc((*pNumberOfEntries) *
                                         sizeof(javacall_ams_midlet_info));
    if (*ppMidletsInfo == NULL) {
        return JAVACALL_FAIL;
    }

    n = 0;
    while (1) {
        pcsl_string midletNAttrName, midletNAttrValue, midletNum;
        pcsl_string_status err;

        err = pcsl_string_dup(&midletStr, &midletNAttrName);
	    if (err != PCSL_STRING_OK) {
            javanotify_ams_suite_free_midlets_info(*ppMidletsInfo, n);
            return JAVACALL_FAIL;
        }

        err = pcsl_string_convert_from_jint(n + 1, &midletNum);
	    if (err != PCSL_STRING_OK) {
	        pcsl_string_free(&midletNAttrName);
            javanotify_ams_suite_free_midlets_info(*ppMidletsInfo, n);
            return JAVACALL_FAIL;
        }

        err = pcsl_string_append(&midletNAttrName, &midletNum);
	    if (err != PCSL_STRING_OK) {
	        pcsl_string_free(&midletNum);
            javanotify_ams_suite_free_midlets_info(*ppMidletsInfo, n);
            return JAVACALL_FAIL;
        }

        status = midp_get_suite_property(
            pMidpSuiteData->suiteId, &midletNAttrName, &midletNAttrValue);

        pcsl_string_free(&midletNAttrName);
        pcsl_string_free(&midletNum);

        if (status != ALL_OK) {
            /* NOT_FOUND means OK: the last MIDlet-<n> attribute was handled */
            if (status != NOT_FOUND) {
                pcsl_string_free(&midletNAttrValue);
                javanotify_ams_suite_free_midlets_info(*ppMidletsInfo, n);
                return midp_error2javacall(status);
            }
            break;
        }

        if (n == pMidpSuiteData->numberOfMidlets) {
            /* IMPL_NOTE: an error message should be logged */
            pcsl_string_free(&midletNAttrValue);
            break;
        }

        /* filling the structure with the information we've got */
        pTmpMidletInfo = &(*ppMidletsInfo)[n];
        pTmpMidletInfo->suiteId = pMidpSuiteData->suiteId;

        status = parse_midlet_attr(&midletNAttrValue,
                                   &pTmpMidletInfo->displayName,
                                   &pTmpMidletInfo->iconPath,
                                   &pTmpMidletInfo->className);

        pcsl_string_free(&midletNAttrValue);

        if (status != ALL_OK) {
            /* ignore the error, just exit from the cycle */
            break;
        }

        n++;
    }

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
javanotify_ams_suite_free_midlets_info(javacall_ams_midlet_info* pMidletsInfo,
                                       int numberOfEntries) {
    if (pMidletsInfo != NULL && numberOfEntries > 0) {
        int i;
        for (i = 0; i < numberOfEntries; i++) {
            javacall_ams_midlet_info* pNextEntry = &pMidletsInfo[i];
            if (pNextEntry != NULL) {
                JCUTIL_FREE_JC_STRING(pNextEntry->displayName);
                JCUTIL_FREE_JC_STRING(pNextEntry->iconPath);
                JCUTIL_FREE_JC_STRING(pNextEntry->className);
            }
        }
    }
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
 *                          using javanotify_ams_suite_free_info() when it is not
 *                          needed anymore
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_info(javacall_suite_id suiteId,
                              javacall_ams_suite_info** ppSuiteInfo) {
    MIDPError status;
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
        (pMidpSuiteData->type == COMPONENT_PREINSTALLED_SUITE) ?
            JAVACALL_TRUE : JAVACALL_FALSE;

    /*
     * Convert strings from pMidpSuiteData from pcsl_string to
     * javacall_utf16_string and copy them into pTmpSuiteInfo.
     */
    status = ALL_OK;

    do {
        if (!pcsl_string_is_null(
                &pMidpSuiteData->varSuiteData.midletClassName)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.midletClassName,
                &pTmpSuiteInfo->midletClassName);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->midletClassName = NULL;
        }

        if (!pcsl_string_is_null(&pMidpSuiteData->varSuiteData.displayName)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.displayName,
                &pTmpSuiteInfo->displayName);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->displayName = NULL;
        }

        if (!pcsl_string_is_null(&pMidpSuiteData->varSuiteData.iconName)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.iconName,
                &pTmpSuiteInfo->iconPath);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->iconPath = NULL;
        }

        if (!pcsl_string_is_null(&pMidpSuiteData->varSuiteData.suiteVendor)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.suiteVendor,
                &pTmpSuiteInfo->suiteVendor);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->suiteVendor = NULL;
        }

        if (!pcsl_string_is_null(&pMidpSuiteData->varSuiteData.suiteName)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.suiteName,
                &pTmpSuiteInfo->suiteName);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->suiteName = NULL;
        }

        if (!pcsl_string_is_null(&pMidpSuiteData->varSuiteData.suiteVersion)) {
            status = midp_pcsl_str2javacall_str(
                &pMidpSuiteData->varSuiteData.suiteVersion,
                &pTmpSuiteInfo->suiteVersion);
            if (status != ALL_OK) {
                break;
            }
        } else {
            pTmpSuiteInfo->suiteVersion = NULL;
        }
    } while (0);

    if (status != ALL_OK) {
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->midletClassName)
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->displayName)
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->iconPath)
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->suiteVendor)
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->suiteName)
        JCUTIL_FREE_JC_STRING(pTmpSuiteInfo->suiteVersion)
        javacall_free(pTmpSuiteInfo);
        return midp_error2javacall(status);
    }

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
javanotify_ams_suite_free_info(javacall_ams_suite_info* pSuiteInfo) {
    if (pSuiteInfo != NULL) {
        JCUTIL_FREE_JC_STRING(pSuiteInfo->midletClassName)
        JCUTIL_FREE_JC_STRING(pSuiteInfo->displayName)
        JCUTIL_FREE_JC_STRING(pSuiteInfo->iconPath)
        JCUTIL_FREE_JC_STRING(pSuiteInfo->suiteVendor)
        JCUTIL_FREE_JC_STRING(pSuiteInfo->suiteName)
        JCUTIL_FREE_JC_STRING(pSuiteInfo->suiteVersion)
        javacall_free(pSuiteInfo);
    }
}

/**
 * App Manager invokes this function to get the domain the suite belongs to.
 *
 * @param suiteId unique ID of the MIDlet suite
 * @param pDomainId [out] pointer to the location where the retrieved domain
 *                        information will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_domain(javacall_suite_id suiteId,
                                javacall_ams_domain* pDomainId) {
    javacall_result jcRes;
    javacall_ams_suite_info* pSuiteInfo;

    if (pDomainId == NULL) {
        return JAVACALL_FAIL;
    }

    if ((SuiteIdType)suiteId == INTERNAL_SUITE_ID) {
        /* internal suites belong to the manufacturer domain */
        *pDomainId = JAVACALL_AMS_DOMAIN_MANUFACTURER;
        return JAVACALL_OK;
    }

    jcRes = javanotify_ams_suite_get_info(suiteId, &pSuiteInfo);
    if (jcRes != JAVACALL_OK) {
        return jcRes;
    }

    /*
     * IMPL_NOTE: instead of revealing the actual domain, here we just check
     *            if the domain is trusted or not for simplicity.
     */
    *pDomainId = (pSuiteInfo->isTrusted == JAVACALL_TRUE ?
        JAVACALL_AMS_DOMAIN_THIRDPARTY : JAVACALL_AMS_DOMAIN_UNTRUSTED);

    javanotify_ams_suite_free_info(pSuiteInfo);

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
javanotify_ams_suite_disable(javacall_suite_id suiteId) {
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
javanotify_ams_suite_enable(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_enable_suite((SuiteIdType)suiteId));
}

/**
 * App Manager invokes this function to get permissions of the suite.
 *
 * Note: memory for pPermissions array is allocated and freed by the caller.
 *
 * @param suiteId       [in]     unique ID of the MIDlet suite
 * @param pPermissions  [in/out] array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                               elements that will be filled with the
 *                               current permissions' values on exit
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_permissions(javacall_suite_id suiteId,
                                     javacall_ams_permission_val* pPermissions) {
    MIDPError status;
    char* pszError = NULL;
    jbyte* pMidpPermissions;
    int iNumberOfPermissions;
    jbyte pushInterruptSetting;
    jint pushOptions;
    jboolean enabled;
    int i;

    if (pPermissions == NULL) {
        return JAVACALL_FAIL;
    }

    status = read_settings(&pszError, (SuiteIdType)suiteId,
                           &enabled,
                           &pushInterruptSetting,
                           &pushOptions,
                           &pMidpPermissions,
                           &iNumberOfPermissions);
    if (status != ALL_OK) {
        storageFreeError(pszError);
        return midp_error2javacall(status);
    }

    for (i = 0; i < JAVACALL_AMS_NUMBER_OF_PERMISSIONS; i++) {
        pPermissions[i] = JAVACALL_AMS_PERMISSION_VAL_NEVER;
    }

    for (i = 0; i < iNumberOfPermissions; i++) {
        javacall_ams_permission jcPermission = midp_permission2javacall(i);
        if (jcPermission != JAVACALL_AMS_PERMISSION_VAL_INVALID) {
            pPermissions[(int)jcPermission] =
                midp_permission_val2javacall(pMidpPermissions[i]);
        }
    }

    if (iNumberOfPermissions > 0 && pMidpPermissions != NULL) {
        pcsl_mem_free(pMidpPermissions);
    }

    return JAVACALL_OK;
}

/**
 * Implementation for javanotify_ams_suite_set_permission and
 * javanotify_ams_suite_set_permissions.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [in]  array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                            elements containing the permissions' values
 *                            to be set; may be NULL
 * @param permissionToSet [in] permission that must be set or
 *                             JAVACALL_AMS_PERMISSION_INVALID if it is required
 *                             to set values for all permissions
 * @param valueToSet      [in] value to which permissionToSet must be set or
 *                             JAVACALL_AMS_PERMISSION_VAL_INVALID if it is
 *                             required to set values for all permissions
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
static javacall_result
set_permissions_impl(javacall_suite_id suiteId,
                     javacall_ams_permission_val* pPermissions,
                     javacall_ams_permission permissionToSet,
                     javacall_ams_permission_val valueToSet) {
    MIDPError status;
    char* pszError = NULL;
    jbyte permissionsBuf[JAVACALL_AMS_NUMBER_OF_PERMISSIONS];
    jbyte* pTmpPermissions;
    jbyte* pNewMidpPermissions;
    int iNumberOfPermissions;
    jbyte pushInterruptSetting;
    jint pushOptions;
    jboolean enabled;
    int i;

    /* read other settings to preserve them when updating permissions */
    status = read_settings(&pszError, (SuiteIdType)suiteId,
                           &enabled,
                           &pushInterruptSetting,
                           &pushOptions,
                           &pTmpPermissions,
                           &iNumberOfPermissions);
    if (status != ALL_OK) {
        storageFreeError(pszError);
        return midp_error2javacall(status);
    }

    /* convert Javacall permission values to the midp values */
    if (pPermissions != NULL) {
        /* set values for all permissions */
        iNumberOfPermissions = JAVACALL_AMS_NUMBER_OF_PERMISSIONS;
        pNewMidpPermissions = permissionsBuf;
        
        for (i = 0; i < iNumberOfPermissions; i++) {
            int midpPermission =
                javacall_permission2midp((javacall_ams_permission)i);
            if (midpPermission >= 0 &&
                    midpPermission < JAVACALL_AMS_NUMBER_OF_PERMISSIONS) {
                pNewMidpPermissions[midpPermission] =
                    javacall_permission_val2midp(pPermissions[i]);
            }
        }
    } else {
        /* set value for one permission */
        int midpPermission;

        pNewMidpPermissions = pTmpPermissions;

        midpPermission = javacall_permission2midp(permissionToSet);
        if (midpPermission >= 0 &&
                midpPermission < JAVACALL_AMS_NUMBER_OF_PERMISSIONS) {
            pNewMidpPermissions[midpPermission] =
                    javacall_permission_val2midp(valueToSet);
        }
    }

    /* write the updated settings */
    status = write_settings(&pszError, (SuiteIdType)suiteId,
                            enabled,
                            pushInterruptSetting,
                            pushOptions,
                            pNewMidpPermissions,
                            iNumberOfPermissions,
                            NULL);

    if (iNumberOfPermissions > 0 && pTmpPermissions != NULL) {
        pcsl_mem_free(pTmpPermissions);
    }

    if (status != ALL_OK) {
        storageFreeError(pszError);
        return midp_error2javacall(status);
    }

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set a single permission of the suite
 * when the user changes it.
 *
 * @param suiteId     unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_set_permission(javacall_suite_id suiteId,
                                    javacall_ams_permission permission,
                                    javacall_ams_permission_val value) {
    return set_permissions_impl(suiteId, NULL, permission, value);
}

/**
 * App Manager invokes this function to set permissions of the suite.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [in]  array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                            elements containing the permissions' values
 *                            to be set
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_set_permissions(javacall_suite_id suiteId,
                                     javacall_ams_permission_val* pPermissions) {
    if (pPermissions == NULL) {
        return JAVACALL_FAIL;
    }

    return set_permissions_impl(suiteId, pPermissions,
        JAVACALL_AMS_PERMISSION_INVALID, JAVACALL_AMS_PERMISSION_VAL_INVALID);
}

/**
 * App Manager invokes this function to remove a suite with the given ID.
 * This call is synchronous.
 *
 * @param suiteId ID of the suite to remove
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_remove(javacall_suite_id suiteId) {
    return midp_error2javacall(midp_remove_suite((SuiteIdType)suiteId));
}

/**
 * App Manager invokes this function to move a software package with given
 * suite ID to the specified storage.
 *
 * @param suiteId suite ID for the installed package
 * @param newStorageId new storage ID
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_change_storage(javacall_suite_id suiteId,
                                    javacall_storage_id newStorageId) {
    return midp_error2javacall(midp_change_suite_storage( 
        (SuiteIdType)suiteId, (StorageIdType)newStorageId));
}

/**
 * App Manager invokes this function to check if the suite with the given ID
 * is trusted.
 *
 * This is just a helper method, javanotify_ams_suite_get_info()
 * also can be used for this purpose.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @return <tt>JAVACALL_TRUE</tt> if the suite is trusted,
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool
javanotify_ams_suite_is_preinstalled(javacall_suite_id suiteId) {
    MidletSuiteData* pMidpSuiteData = get_suite_data((SuiteIdType)suiteId);
    if (pMidpSuiteData == NULL) {
        return JAVACALL_FALSE;
    }
    return (pMidpSuiteData->type == COMPONENT_PREINSTALLED_SUITE) ?
                JAVACALL_TRUE : JAVACALL_FALSE;
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
long javanotify_ams_suite_get_storage_size(javacall_suite_id suiteId) {
    return midp_get_suite_storage_size((SuiteIdType)suiteId);
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_check_suites_integrity(javacall_bool fullCheck,
                                            javacall_bool delCorruptedSuites) {
    return midp_error2javacall(midp_check_suites_integrity(
        (fullCheck == JAVACALL_TRUE), (delCorruptedSuites == JAVACALL_TRUE)));
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_all_folders_info(
    javacall_ams_folder_info** ppFoldersInfo, int* pNumberOfEntries) {
#if ENABLE_AMS_FOLDERS
    MIDPError status;
    javacall_ams_folder_info* pTmpFoldersInfo;
    int i;

    /* IMPL_NOTE: temporary hardcoded, should be moved to an xml */
    int foldersNum = 2;

    PCSL_DEFINE_ASCII_STRING_LITERAL_START(pcslStrFolderName1)
        {'P','r','e','i','n','s','t','a','l','l','e','d',' ',
         'a','p','p','s','\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(pcslStrFolderName1);

    PCSL_DEFINE_ASCII_STRING_LITERAL_START(pcslStrFolderName2)
        {'O','t','h','e','r',' ', 'a','p','p','s','\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(pcslStrFolderName2);
#endif /* ENABLE_AMS_FOLDERS */

    if (ppFoldersInfo == NULL || pNumberOfEntries == NULL) {
        return JAVACALL_FAIL;
    }

    *pNumberOfEntries = 0;

#if ENABLE_AMS_FOLDERS
    /* IMPL_NOTE: temporary hardcoded, should be moved to an xml */

    pTmpFoldersInfo = (javacall_ams_folder_info*)javacall_malloc(
        foldersNum * sizeof(javacall_ams_folder_info));
    if (pTmpFoldersInfo == NULL) {
        return JAVACALL_FAIL;
    }

    for (i = 0; i < foldersNum; i++) {
        pTmpFoldersInfo[i].folderId = (javacall_folder_id)i;
        status = midp_pcsl_str2javacall_str((i == 0) ? &pcslStrFolderName1 :
                                                       &pcslStrFolderName2,
                                            &pTmpFoldersInfo[i].folderName);
        if (status != ALL_OK) {
            int j;
            for (j = 0; j < i; j++) {
                if (pTmpFoldersInfo[i].folderName != NULL) {
                    javacall_free(pTmpFoldersInfo[i].folderName);
                }
            }
            return midp_error2javacall(status);
        }
    }

    *pNumberOfEntries = foldersNum;
    *ppFoldersInfo = pTmpFoldersInfo;
#endif /* ENABLE_AMS_FOLDERS */

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
javanotify_ams_suite_free_all_folders_info(
        javacall_ams_folder_info* pFoldersInfo, int numberOfEntries) {
    if (pFoldersInfo != NULL && numberOfEntries > 0) {
        int i;
        for (i = 0; i < numberOfEntries; i++) {
            if (pFoldersInfo[i].folderName != NULL) {
                javacall_free(pFoldersInfo[i].folderName);
            }
        }
        javacall_free(pFoldersInfo);
    }
}

/**
 * App Manager invokes this function to get an information about
 * the given AMS folder.
 *
 * Note that memory for the out parameter pFolderInfo and its fields is
 * allocated by the callee. The caller is responsible for freeing it using
 * javanotify_ams_suite_free_folder_info().
 *
 * @param folderId     [in]  unique ID of the folder
 * @param ppFolderInfo [out] on exit will hold a pointer to a structure
 *                           describing the given folder
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_folder_info(javacall_folder_id folderId,
                                     javacall_ams_folder_info** ppFolderInfo) {
    /* IMPL_NOTE: the implementation should be optimized */
    javacall_result res;
    javacall_ams_folder_info* pAllFoldersInfo;
    int foldersNum, i;

    res = javanotify_ams_suite_get_all_folders_info(&pAllFoldersInfo,
                                                    &foldersNum);
    if (res != JAVACALL_OK) {
        return res;
    }

    res = JAVACALL_FAIL;

    for (i = 0; i < foldersNum; i++) {
        if (pAllFoldersInfo[i].folderId == folderId) {
            *ppFolderInfo = (javacall_ams_folder_info*)javacall_malloc(
                sizeof(javacall_ams_folder_info));
            if (*ppFolderInfo == NULL) {
                break;
            }

            memcpy(*ppFolderInfo, &pAllFoldersInfo[i],
                (size_t)sizeof(javacall_ams_folder_info));

            (*ppFolderInfo)->folderName = pAllFoldersInfo[i].folderName;
            pAllFoldersInfo[i].folderName = NULL;

            res = JAVACALL_OK;
            break;
        }
    }

    javanotify_ams_suite_free_all_folders_info(pAllFoldersInfo, foldersNum);

    return res;
}

/**
 * App Manager invokes this function to free the given structure holding
 * an information about an AMS folder.
 *
 * @param pFolderInfo [in] a pointer to the structure that must be freed
 */
void
javanotify_ams_suite_free_folder_info(javacall_ams_folder_info* pFolderInfo) {
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
 * javanotify_ams_suite_free_ids().
 *
 * @param folderId        [in]  unique ID of the folder
 * @param ppSuiteIds      [out] on exit will hold an address of the array
 *                              containing suite IDs
 * @param pNumberOfSuites [out] pointer to variable to accept the number
 *                              of suites in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if an IO error,
 *         <tt>JAVACALL_FAIL</tt> if other error
 */
javacall_result
javanotify_ams_suite_get_suites_in_folder(javacall_folder_id folderId,
                                          javacall_suite_id** ppSuiteIds,
                                          int* pNumberOfSuites) {
    if (ppSuiteIds == NULL || pNumberOfSuites == NULL) {
        return JAVACALL_FAIL; 
    }

    if (folderId == JAVACALL_ROOT_FOLDER_ID) {
        return javanotify_ams_suite_get_suite_ids(ppSuiteIds,
                                                  pNumberOfSuites);
    } else {
        MidletSuiteData *pMidpSuiteData, *pSaveSuiteData;
        int i, n;

#if ENABLE_AMS_FOLDERS
        pMidpSuiteData = g_pSuitesData;
        if (pMidpSuiteData == NULL || g_numberOfSuites == 0) {
            *pNumberOfSuites = 0;
            *ppSuiteIds = NULL;
            return JAVACALL_OK;
        }

        /**
         * There are two iterations. During the first one we count the number
         * of suites residing in the given folder, then we allocate memory
         * to hold the suite IDs. During the second iteration the suite IDs
         * are saved into this memory.
         */
        pSaveSuiteData = pMidpSuiteData;

        for (i = 0; i < 2; i++) {
            n = 0;

            /* walk through the list of suites */
            while (pMidpSuiteData != NULL) {
                if (pMidpSuiteData->folderId == folderId) {
                    if (i == 1) {
                        (*ppSuiteIds)[n] =
                            (javacall_suite_id)pMidpSuiteData->suiteId; 
                    }
                    n++;
                }
                pMidpSuiteData = pMidpSuiteData->nextEntry;
            }

            if (i == 0) {
                /* the first iteration is finished */
                if (n == 0) {
                    *ppSuiteIds = NULL;
                    break;
                }

                /* allocate memory to hold the suite IDs */
                *ppSuiteIds = (javacall_suite_id*)javacall_malloc(
                    n * sizeof(javacall_suite_id));
                if (*ppSuiteIds == NULL) {
                    return JAVACALL_OUT_OF_MEMORY;
                }

                pMidpSuiteData = pSaveSuiteData;
            }
        }

        *pNumberOfSuites = n;
#else
        (void)i;
        (void)n;
        *pNumberOfSuites = 0;
#endif /* ENABLE_AMS_FOLDERS */
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
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_get_folder(javacall_suite_id suiteId,
                                javacall_folder_id* pSuiteFolderId) {
    MidletSuiteData* pMidpSuiteData;

    if (pSuiteFolderId == NULL) {
        return JAVACALL_FAIL;
    }

#if ENABLE_AMS_FOLDERS
    pMidpSuiteData = get_suite_data((SuiteIdType)suiteId);
    if (pMidpSuiteData == NULL) {
        return JAVACALL_FAIL;
    }
    *pSuiteFolderId = (javacall_folder_id)pMidpSuiteData->folderId;
#else
    (void)pMidpSuiteData;
    *pSuiteFolderId = JAVACALL_ROOT_FOLDER_ID;
#endif /* ENABLE_AMS_FOLDERS */

    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to move the given midlet suite
 * to another folder.
 *
 * @param suiteId ID of the suite
 * @param newFolderId ID of the folder where the suite must be moved
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javanotify_ams_suite_move_to_folder(javacall_suite_id suiteId,
                                    javacall_folder_id newFolderId) {
    return midp_error2javacall(midp_move_suite_to_folder(
        (SuiteIdType)suiteId, (FolderIdType)newFolderId));

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
javanotify_ams_suite_read_secure_resource(
                                   javacall_suite_id suiteId,
                                   javacall_const_utf16_string resourceName,
                                   javacall_uint8** ppReturnValue,
                                   javacall_int32* pValueSize) {
    MIDPError status;
    pcsl_string pcslStrResName;
    jint jintValueSize;

    status = midp_javacall_str2pcsl_str(resourceName, &pcslStrResName);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_suite_read_secure_resource((SuiteIdType)suiteId,
                                             &pcslStrResName,
                                             (jbyte**)ppReturnValue,
                                             &jintValueSize);
    pcsl_string_free(&pcslStrResName);                                             
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    *pValueSize = (javacall_int32)jintValueSize;

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
javanotify_ams_suite_write_secure_resource(
                                   javacall_suite_id suiteId,
                                   javacall_const_utf16_string resourceName,
                                   javacall_uint8* pValue,
                                   javacall_int32 valueSize) {
    MIDPError status;
    pcsl_string pcslStrResName;

    status = midp_javacall_str2pcsl_str(resourceName, &pcslStrResName);
    if (status != ALL_OK) {
        return midp_error2javacall(status);
    }

    status = midp_suite_write_secure_resource((SuiteIdType)suiteId,
                                              &pcslStrResName,
                                              (jbyte*)pValue,
                                              (javacall_int32)valueSize);
    pcsl_string_free(&pcslStrResName);

    return midp_error2javacall(status);
}

/******************************************************************************/

/**
 * Parses a "MIDlet-<n>: <midlet_name>, <icon_name>, <class_name>"
 * jad attribute and returns its separate components.
 *
 * The caller is responsible for freeing the returned strings
 * when they are not needed.
 *
 * @param pMIDletAttrValue [in]  a pcsl string holding the attribute to parse
 * @param pDisplayName     [out] on exit will hold a midlet_name component
 * @param pIconName        [out] on exit will hold an icon_name component
 * @param pClassName       [out] on exit will hold a class_name component
 *
 * @return an error code, ALL_OK if no errors
 */
static MIDPError parse_midlet_attr(const pcsl_string* pMIDletAttrValue,
                                   javacall_utf16_string* pDisplayName,
                                   javacall_utf16_string* pIconName,
                                   javacall_utf16_string* pClassName) {
    MIDPError status; 
    pcsl_string_status err;
    pcsl_string fieldValue, tmpFieldValue;
    jchar comma = 0x002c; /* ',' */
    jint commaIdx, startIdx = 0;
    jint attrValueLen = pcsl_string_length(pMIDletAttrValue);
    int i;

    *pDisplayName = NULL;
    *pIconName = NULL;
    *pClassName = NULL;

    for (i = 0; i < 3; i++) {
        commaIdx = pcsl_string_index_of_from(pMIDletAttrValue, comma, startIdx);
        if (commaIdx < 0) {
            commaIdx = attrValueLen;
        }

        err = pcsl_string_substring(pMIDletAttrValue, startIdx, commaIdx,
                                    &tmpFieldValue);
        if (err != PCSL_STRING_OK) {
            /* ignore error: can't do anything meaningful */
            break;                                    
        }

        err = pcsl_string_trim(&tmpFieldValue, &fieldValue);
        pcsl_string_free(&tmpFieldValue);
        if (err != PCSL_STRING_OK) {
            /* ignore error: can't do anything meaningful */
            break;                                    
        }

        status = midp_pcsl_str2javacall_str(&fieldValue,
            ((i == 0) ? pDisplayName : ((i == 1) ? pIconName : pClassName)));
        pcsl_string_free(&fieldValue);
        if (status != ALL_OK) {
            /* ignore error: can't do anything meaningful */
            break;
        }

        if (commaIdx == attrValueLen) {
            break;
        }

        startIdx = commaIdx + 1;
    }

    return ALL_OK;
}

static javacall_ams_permission
midp_permission2javacall(int midpPermissionId) {
    javacall_ams_permission jcPermissionId;
    /*
     * IMPL_NOTE: here it is assumed that the numeric values of the same
     *            MIDP and Javacall permissions are equal. This may not
     *            be true.
     */
    if (midpPermissionId > 0 &&
            midpPermissionId < (int)JAVACALL_AMS_PERMISSION_LAST) {
        jcPermissionId = (javacall_ams_permission) midpPermissionId;
    } else {
        jcPermissionId = JAVACALL_AMS_PERMISSION_INVALID;
    }

    return jcPermissionId;
}

static javacall_ams_permission_val
midp_permission_val2javacall(jbyte midpPermissionVal) {
    javacall_ams_permission_val jcPermissionVal;

    /*
     * IMPL_NOTE: values in "case <N>" must be equal to the corresponding
     *            Permissions.<Value> constants in Java.
     */
    switch (midpPermissionVal) {
        case 0: /* Permissions.NEVER */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_NEVER;
            break;
        case 1:  /* Permissions.ALLOW */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_ALLOW;
            break;
        case 2:  /* Permissions.BLANKET_GRANTED */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_BLANKET_GRANTED;
            break;
        case 4: /* Permissions.BLANKET */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_BLANKET;
            break;
        case 8:  /* Permissions.SESSION */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_SESSION;
            break;
        case 16: /* Permissions.ONESHOT */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_ONE_SHOT;
            break;
        case -128: /* Permissions.BLANKET_DENIED */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_BLANKET_DENIED;
            break;

        default: /* Unknown */
            jcPermissionVal = JAVACALL_AMS_PERMISSION_VAL_INVALID;
    }

    return jcPermissionVal;
}

static int
javacall_permission2midp(javacall_ams_permission jcPermission) {
    /*
     * IMPL_NOTE: here it is assumed that the numeric values of the same
     *            MIDP and Javacall permissions are equal. This may not
     *            be true.
     */
    return (int)jcPermission;
}

static jbyte
javacall_permission_val2midp(javacall_ams_permission_val jcPermissionVal) {
    jbyte midpPermissionVal;

    /*
     * IMPL_NOTE: values in "case <N>" must be equal to the corresponding
     *            Permissions.<Value> constants in Java.
     */
    switch (jcPermissionVal) {
        case JAVACALL_AMS_PERMISSION_VAL_NEVER:
            midpPermissionVal = 0;  /* Permissions.NEVER */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_ALLOW:
            midpPermissionVal = 1;  /* Permissions.ALLOW */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_BLANKET_GRANTED:
            midpPermissionVal = 2;  /* Permissions.BLANKET_GRANTED */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_BLANKET:
            midpPermissionVal = 4;  /* Permissions.BLANKET */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_SESSION:
            midpPermissionVal = 8;  /* Permissions.SESSION */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_ONE_SHOT:
            midpPermissionVal = 16; /* Permissions.ONESHOT */
            break;
        case JAVACALL_AMS_PERMISSION_VAL_BLANKET_DENIED:
            midpPermissionVal = -128; /* Permissions.BLANKET_DENIED */
            break;

        default: /* Unknown */
            midpPermissionVal = 0; /* Permissions.NEVER */
    }

    return midpPermissionVal;
}
