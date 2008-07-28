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
 * This is reference implementation of the common MIDlet suite storage
 * functions.
 */

#include <string.h>
#include <kni.h>
#include <pcsl_memory.h>
#include <pcsl_string.h>
#include <midpInit.h>
#include <midpUtilKni.h>
#include <midpMalloc.h>
#include <midp_logging.h>
#include <suitestore_common.h>
#include <javacall_nams.h>

#define CONVERT_SUITE_ID(x) x
#define CONVERT_STORAGE_ID(y) y
#define CONVERT_STRING_STATUS(z) z

#define SUITESTORE_COUNTOF(x) (sizeof(x) / sizeof(x[0]))
/**
 * Converts the given suite ID to pcsl_string.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * @param value suite id to convert
 *
 * @return pcsl_string representation of the given suite ID
 */
const pcsl_string* midp_suiteid2pcsl_string(SuiteIdType value) {
    int i;
    jchar digits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    unsigned long unsignedValue = (unsigned long) value;
    static jchar resChars[GET_SUITE_ID_LEN(value) + 1]; /* +1 for last zero */
    static pcsl_string resString = {
        resChars, SUITESTORE_COUNTOF(resChars), 0
    };

    for (i = (int)SUITESTORE_COUNTOF(resChars) - 2; i >= 0; i--) {
        resChars[i] = digits[unsignedValue & 15];
        unsignedValue >>= 4;
    }

    resChars[SUITESTORE_COUNTOF(resChars) - 1] = (jchar)0;
    return &resString;
}

/**
 * Converts the given suite ID to array of chars.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * @param value suite id to convert
 *
 * @return char[] representation of the given suite ID
 * or NULL in case of error.
 */
const char* midp_suiteid2chars(SuiteIdType value) {
    jsize resLen;
    static jbyte resChars[GET_SUITE_ID_LEN(value) + 1]; /* +1 for last zero */
    const pcsl_string* pResString = midp_suiteid2pcsl_string(value);
    pcsl_string_status rc;

    rc = pcsl_string_convert_to_utf8(pResString, resChars,
        (jsize)SUITESTORE_COUNTOF(resChars), &resLen);

    return (rc == PCSL_STRING_OK) ? (char*)&resChars : NULL;
}
#undef SUITESTORE_COUNTOF


/**
 * Initializes the subsystem.
 */
MIDPError
midp_suite_storage_init() {
        return ALL_OK;
}

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 */
void
midp_suite_storage_cleanup() {
}



/**
 * Tells if a suite exists.
 *
 * @param suiteId ID of a suite
 *
 * @return ALL_OK if a suite exists,
 *         NOT_FOUND if not,
 *         OUT_OF_MEMORY if out of memory or IO error,
 *         IO_ERROR if IO error,
 *         SUITE_CORRUPTED_ERROR is suite is found in the list,
 *         but it's corrupted
 */
MIDPError
midp_suite_exists(SuiteIdType suiteId) {

        return ALL_OK;
}

/*
 * Gets location of either the class path or binary image.
*/
static MIDPError
_get_class_path(SuiteIdType suiteId,
                StorageIdType storageId,
                jboolean is_binary_image,
                pcsl_string *classPath) {
    MIDPError status = NOT_FOUND;
    int result;
    javacall_utf8_string utfClassPath[JAVACALL_MAX_FILE_NAME_LENGTH];
    memset((void*)utfClassPath, 0, JAVACALL_MAX_FILE_NAME_LENGTH);
    result = javacall_ams_get_classpath(CONVERT_SUITE_ID(suiteId),
                                         utfClassPath,
                                         JAVACALL_MAX_FILE_NAME_LENGTH);

    if (JAVACALL_OK == result) {
        result = pcsl_string_convert_from_utf8(utfClassPath, JAVACALL_MAX_FILE_NAME_LENGTH, 
                                               classPath);
        status = CONVERT_STRING_STATUS(result);
    } else if (JAVACALL_OUT_OF_MEMORY == result) {
        status = OUT_OF_MEMORY;
    }

    return status;
}

/**
 * Gets location of the class path for the suite with the specified suiteId.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using pcsl_mem_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param classPath The in/out parameter that contains returned class path
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError
midp_suite_get_class_path(SuiteIdType suiteId,
                          StorageIdType storageId,
                          jboolean checkSuiteExists,
                          pcsl_string *classPath) {
    return _get_class_path(suiteId, storageId, KNI_FALSE, classPath);

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
 * freeing it using pcsl_mem_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param classPath The in/out parameter that contains returned class path
 * @return  error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
*/
MIDPError
midp_suite_get_bin_app_path(SuiteIdType suiteId,
                            StorageIdType storageId,
                            pcsl_string *classPath) {
    return _get_class_path(suiteId, storageId, KNT_TRUE, classPath);
}
#endif

/**
 * Gets location of the cached resource with specified name
 * for the suite with the specified suiteId.
 *
 * Note that when porting memory for the in/out parameter
 * filename MUST be allocated  using pcsl_mem_malloc().
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId       The application suite ID
 * @param storageId     storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param pResourceName Name of the cached resource
 * @param pFileName     The in/out parameter that contains returned filename
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError
midp_suite_get_cached_resource_filename(SuiteIdType suiteId,
                                        StorageIdType storageId,
                                        const pcsl_string * pResourceName,
                                        pcsl_string * pFileName) {

    return NOT_FOUND;
}

/**
 * Retrieves an ID of the storage where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId The application suite ID
 * @param pStorageId [out] receives an ID of the storage where
 *                         the suite is stored
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
midp_suite_get_suite_storage(SuiteIdType suiteId, StorageIdType* pStorageId) {

    *pStorageId = 0;
    return ALL_OK;;

}

/**
 * Retrieves an ID of the folder where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId The application suite ID
 * @param pFolderId [out] receives an ID of the folder where the suite is stored
 * 
 * @note    used by installer and task manager
 * @return error code (ALL_OK if successful)
 */
MIDPError
midp_suite_get_suite_folder(SuiteIdType suiteId, FolderIdType* pFolderId) {
    *pFolderId = 0;
    return ALL_OK;
}

/**
 * If the suite exists, this function returns a unique identifier of
 * MIDlet suite. Note that suite may be corrupted even if it exists.
 * If the suite doesn't exist, a new suite ID is created.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param name name of the suite, as given in a JAD file
 * @param pSuiteId [out] receives the platform-specific suite ID of the
 *          application given by vendorName and appName, or string with
 *          a null data if suite does not exist, or
 *          out of memory error occured, or suite is corrupted.
 *
 * @return  ALL_OK if suite found,
 *          NOT_FOUND if suite does not exist (so a new ID was created),
 *          other error code in case of error
 */
MIDPError
midp_get_suite_id(const pcsl_string* vendor, const pcsl_string* name,
                  SuiteIdType* pSuiteId) {
    javacall_suite_id id;
    MIDPError status = NOT_FOUND;
    GET_PCSL_STRING_DATA_AND_LENGTH(vendor)
    GET_PCSL_STRING_DATA_AND_LENGTH(name)
    if (JAVACALL_OK == javacall_ams_get_suite_id(vendor_data, name_data, &id)) {
        *pSuiteId = (SuiteIdType)id;
        status = ALL_OK;
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    return status;
}

/**
 * Find and return the property the matches the given key.
 * The returned value need not be freed because it resides
 * in an internal data structure.
 *
 * @param pProperties property list
 * @param key key of property to find
 *
 * @return a pointer to the property value,
 *        or to PCSL_STRING_NULL if not found.
 */
pcsl_string*
midp_find_property(MidpProperties* pProperties, const pcsl_string* key) {
    int i;

    /* Properties are stored as key, value pairs. */
    for (i = 0; i < pProperties->numberOfProperties; i++) {
        if (pcsl_string_equals(&pProperties->pStringArr[i * 2], key)) {
            return &pProperties->pStringArr[(i * 2) + 1];
        }
    }

    if (NULL == pProperties->pStringArr 
        && 0 == pProperties->numberOfProperties) {
    }

    return (pcsl_string*)&PCSL_STRING_NULL;
}

/**
 * Free a list of properties. Does nothing if passed NULL.
 *
 * @param pProperties property list
 */
void
midp_free_properties(MidpProperties* pProperties) {
    /* Properties are stored as key, value pairs. */
    if (!pProperties || pProperties->numberOfProperties <= 0) {
        return;
    }

    free_pcsl_string_list(pProperties->pStringArr,
        pProperties->numberOfProperties * 2);
    pProperties->pStringArr = NULL;
}


/**
 * Gets the property of a MIDlet suite to persistent storage.
 *
 * @param suiteId   ID of the suite
 * @param key       key of property to find    
 *
 * @return property that that belongs to given key
 */
pcsl_string 
midp_get_suite_property(SuiteIdType suiteId, const pcsl_string* key)  {
    pcsl_string result = PCSL_STRING_NULL;
    int value_len = 256*sizeof(javacall_utf16);
    javacall_utf16_string value_data = midpMalloc(value_len);

    if (NULL == value_data) {
        REPORT_ERROR(LC_AMS, "Out of memory inside midp_get_suite_property\n");
        return result;
    }
    // allow pcsl_string_convert_from_utf16 strips trailing zero characters later
    memset((void*)value_data, 0, value_len);

    GET_PCSL_STRING_DATA_AND_LENGTH(key)
    if (JAVACALL_OK == javacall_ams_get_suite_property((javacall_suite_id)suiteId,
                                                       key_data,
                                                       value_data, value_len)) {
        // the function calculates real string length automatically 
        pcsl_string_convert_from_utf16(value_data, value_len, &result);

    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    midpFree((void*)value_data);

    return result;
}
