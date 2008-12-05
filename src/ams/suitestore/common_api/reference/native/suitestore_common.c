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
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midpInit.h>
#include <suitestore_common.h>

#include <suitestore_intern.h>
#include <suitestore_locks.h>

#if ENABLE_MONET
/**
 * Filename to save the application image file of suite.
 */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(APP_IMAGE_EXTENSION)
#ifdef PRODUCT
    {'.', 'b', 'u', 'n', '\0'}
#elif defined(AZZERT)
    {'_', 'g', '.', 'b', 'u', 'n', '\0'}
#else
    {'_', 'r', '.', 'b', 'u', 'n', '\0'}
#endif
PCSL_DEFINE_ASCII_STRING_LITERAL_END(APP_IMAGE_EXTENSION);
#endif

/* Description of these internal functions can be found bellow in the code. */
static MIDPError suite_in_list(ComponentType type, SuiteIdType suiteId,
                               ComponentIdType componentId);
static MIDPError get_string_list(char** ppszError, const pcsl_string* pFilename,
                                 pcsl_string** paList, int* pStringNum);
static MIDPError get_class_path_impl(ComponentType type,
                                     SuiteIdType suiteId,
                                     ComponentIdType componentId,
                                     jint storageId,
                                     pcsl_string* classPath,
                                     const pcsl_string* extension);
static MIDPError get_suite_or_component_id(ComponentType type,
                                           SuiteIdType suiteId,
                                           const pcsl_string* vendor,
                                           const pcsl_string* name,
                                           jint* pId);

/**
 * Initializes the subsystem.
 */
MIDPError
midp_suite_storage_init() {
    return suite_storage_init_impl();
}

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 */
void
midp_suite_storage_cleanup() {
    suite_storage_cleanup_impl();
}

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

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Converts the given component ID to pcsl_string.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * The current implementation uses the internal knowledge that
 * ComponentIdType and SuiteIdType are compatible integer types.
 *
 * @param value component id to convert
 * @return pcsl_string representation of the given component id
 */
const pcsl_string* midp_componentid2pcsl_string(ComponentIdType value) {
    return midp_suiteid2pcsl_string((SuiteIdType)value);
}
#endif /* ENABLE_DYNAMIC_COMPONENTS */

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
    MIDPError status;

    if (UNUSED_SUITE_ID == suiteId) {
        return NOT_FOUND;
    }

    if (suiteId == g_lastSuiteExistsID) {
        /* suite exists - we have already checked */
        return ALL_OK;
    }

    g_lastSuiteExistsID = UNUSED_SUITE_ID;

    /* The internal romized suite will not be found in appdb. */
    if (suiteId == INTERNAL_SUITE_ID) {
        g_lastSuiteExistsID = suiteId;
        return ALL_OK;
    }

    status = suite_in_list(COMPONENT_REGULAR_SUITE, suiteId,
                           UNUSED_COMPONENT_ID);

    if (status == ALL_OK) {
        g_lastSuiteExistsID = suiteId;
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
    MIDPError status = ALL_OK;
    int suiteExistsOrNotChecked;

    if (checkSuiteExists) {
        status = midp_suite_exists(suiteId);
        suiteExistsOrNotChecked = (status == ALL_OK ||
                                   status == SUITE_CORRUPTED_ERROR);
    } else {
        /*
         * Don't need to check is the suite exist,
         * just construct the classpath for the given suite ID.
         */
        suiteExistsOrNotChecked = 1;
    }

    if (suiteExistsOrNotChecked) {
        status = get_class_path_impl(COMPONENT_REGULAR_SUITE, suiteId,
                                     UNUSED_COMPONENT_ID, storageId, classPath,
                                     &JAR_EXTENSION);
    } else {
        *classPath = PCSL_STRING_NULL;
    }

    return status;
}

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Tells if a component exists.
 *
 * @param componentId ID of a component
 *
 * @return ALL_OK if a component exists,
 *         NOT_FOUND if not,
 *         OUT_OF_MEMORY if out of memory or IO error,
 *         IO_ERROR if IO error,
 *         SUITE_CORRUPTED_ERROR if component is found in the list,
 *         but it's corrupted
 */
MIDPError
midp_component_exists(ComponentIdType componentId) {
    MIDPError status;

    if (UNUSED_COMPONENT_ID == componentId) {
        return NOT_FOUND;
    }

    if (componentId == g_lastComponentExistsID) {
        /* suite exists - we have already checked */
        return ALL_OK;
    }

    g_lastComponentExistsID = UNUSED_COMPONENT_ID;

    status = suite_in_list(COMPONENT_DYNAMIC, UNUSED_SUITE_ID, componentId);

    if (status == ALL_OK) {
        g_lastComponentExistsID = componentId;
    }

    return status;
}

/**
 * Gets location of the class path for the component
 * with the specified componentId.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using pcsl_mem_free().
 *
 * @param componentId The ID of the component
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkComponentExists true if the component should be checked for
                               existence or not
 * @param pClassPath The in/out parameter that contains returned class path
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError
midp_suite_get_component_class_path(ComponentIdType componentId,
                                    SuiteIdType suiteId,
                                    StorageIdType storageId,
                                    jboolean checkComponentExists,
                                    pcsl_string *pClassPath) {
    MIDPError status = ALL_OK;
    int componentExistsOrNotChecked;

    if (checkComponentExists) {
        status = midp_component_exists(componentId);
        componentExistsOrNotChecked = (status == ALL_OK ||
                                       status == SUITE_CORRUPTED_ERROR);
    } else {
        /*
         * Don't need to check is the component exist,
         * just construct the classpath for the given component ID.
         */
        componentExistsOrNotChecked = 1;
    }

    if (componentExistsOrNotChecked) {
        status = get_class_path_impl(COMPONENT_DYNAMIC, suiteId, componentId,
                                     storageId, pClassPath, &JAR_EXTENSION);
    } else {
        *pClassPath = PCSL_STRING_NULL;
    }

    return status;
}
#endif /* ENABLE_DYNAMIC_COMPONENTS */

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
    return get_class_path_impl(COMPONENT_REGULAR_SUITE, suiteId,
                               UNUSED_COMPONENT_ID, storageId, classPath,
                               &APP_IMAGE_EXTENSION);
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
    const pcsl_string* root = storage_get_root(storageId);
    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string resourceFileName = PCSL_STRING_NULL;
    jint suiteIdLen = GET_SUITE_ID_LEN(suiteId);
    jsize resourceNameLen = pcsl_string_length(pResourceName);

    *pFileName = PCSL_STRING_NULL;

    if (resourceNameLen > 0) {
        /* performance hint: predict buffer capacity */
        int fileNameLen = PCSL_STRING_ESCAPED_BUFFER_SIZE(
            resourceNameLen + pcsl_string_length(&TMP_EXT));
        pcsl_string_predict_size(&resourceFileName, fileNameLen);

        if ( /* Convert any slashes */
            pcsl_esc_attach_string(pResourceName,
                &resourceFileName) != PCSL_STRING_OK ||
            /* Add the extension */
            pcsl_string_append(&resourceFileName, &TMP_EXT) !=
                PCSL_STRING_OK) {
            pcsl_string_free(&resourceFileName);
            return OUT_OF_MEMORY;
        }
    }

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&returnPath, pcsl_string_length(root) +
        suiteIdLen + pcsl_string_length(&resourceFileName));

    if (PCSL_STRING_OK != pcsl_string_append(&returnPath, root) ||
            PCSL_STRING_OK != pcsl_string_append(&returnPath,
                midp_suiteid2pcsl_string(suiteId)) ||
            PCSL_STRING_OK != pcsl_string_append(&returnPath,
                &resourceFileName)) {
        pcsl_string_free(&resourceFileName);
        pcsl_string_free(&returnPath);
        return OUT_OF_MEMORY;
    }

    pcsl_string_free(&resourceFileName);

    *pFileName = returnPath;

    return ALL_OK;
}

/**
 * Retrieves an ID of the requested type for the given suite.
 *
 * @param suiteId The application suite ID
 * @param resultType 0 to return suite storage ID, 1 - suite folder ID
 * @param pResult [out] receives the requested ID
 *
 * @return error code (ALL_OK if successful)
 */
static MIDPError
get_suite_int_impl(SuiteIdType suiteId, int resultType, void* pResult) {
    MIDPError status;
    MidletSuiteData* pData;
    char* pszError;

    if (pResult == NULL) {
        return BAD_PARAMS;
    }

    if (suiteId == INTERNAL_SUITE_ID) {
        /* handle a special case: predefined suite ID is given */
        if (resultType) {
            *(FolderIdType*)pResult  = 0;
        } else {
            *(StorageIdType*)pResult = INTERNAL_STORAGE_ID;
        }
        return ALL_OK;
    }

    /* load _suites.dat */
    status = read_suites_data(&pszError);
    storageFreeError(pszError);

    if (status == ALL_OK) {
        pData = get_suite_data(suiteId);
        if (pData) {
            if (resultType) {
                *(FolderIdType*)pResult  = pData->folderId;
            } else {
                *(StorageIdType*)pResult = pData->storageId;
            }
        } else {
            if (resultType) {
                *(FolderIdType*)pResult  = -1;
            } else {
                *(StorageIdType*)pResult = UNUSED_STORAGE_ID;
            }
            status = NOT_FOUND;
        }
    }

    return status;
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
    return get_suite_int_impl(suiteId, 0, (void*)pStorageId);
}

/**
 * Retrieves an ID of the folder where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId The application suite ID
 * @param pFolderId [out] receives an ID of the folder where the suite is stored
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
midp_suite_get_suite_folder(SuiteIdType suiteId, FolderIdType* pFolderId) {
    return get_suite_int_impl(suiteId, 1, (void*)pFolderId);
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
 *          application given by vendor and name, or UNUSED_SUITE_ID
 *          if suite does not exist, or out of memory error occured,
 *          or suite is corrupted.
 *
 * @return  ALL_OK if suite found,
 *          NOT_FOUND if suite does not exist (so a new ID was created),
 *          other error code in case of error
 */
MIDPError
midp_get_suite_id(const pcsl_string* vendor, const pcsl_string* name,
                  SuiteIdType* pSuiteId) {
    return get_suite_or_component_id(COMPONENT_REGULAR_SUITE, UNUSED_SUITE_ID,
                                     vendor, name,
                                     (jint*)pSuiteId);
}

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * If the dynamic component exists, this function returns an unique identifier
 * of the component. Note that the component may be corrupted even if it exists.
 * If the component doesn't exist, a new component ID is created.
 *
 * @param suiteId ID of the suite the component belongs to 
 * @param vendor name of the vendor that created the component, as
 *        given in a JAD file
 * @param name name of the component, as given in a JAD file
 * @param pComponentId [out] receives the platform-specific component ID of
 *        the component given by vendor and name, or UNUSED_COMPONENT_ID
 *        if component does not exist, or out of memory error occured,
 *        or component is corrupted.
 *
 * @return  ALL_OK if suite found,
 *          NOT_FOUND if component does not exist (so a new ID was created),
 *          other error code in case of error
 */
MIDPError
midp_get_component_id(SuiteIdType suiteId, const pcsl_string* vendor,
                      const pcsl_string* name, ComponentIdType* pComponentId) {
    return get_suite_or_component_id(COMPONENT_DYNAMIC, suiteId, vendor, name,
                                     (jint*)pComponentId);
}
#endif /* ENABLE_DYNAMIC_COMPONENTS */

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
 * Gets the properties of a MIDlet suite to persistent storage.
 * <pre>
 * The format of the properties file will be:
 * <number of strings as int (2 strings per property)>
 *    {repeated for each property}
 *    <length of a property key as int>
 *    <property key as jchars>
 *    <length of property value as int>
 *    <property value as jchars>
 * </pre>
 *
 *
 * Note that memory for the strings inside the returned MidpProperties
 * structure is allocated by the callee, and the caller is
 * responsible for freeing it using midp_free_properties().
 *
 * @param suiteId ID of the suite
 *
 * @return properties in a pair pattern of key and value,
 * use the status macros to check the result. A SUITE_CORRUPTED_ERROR
 * is returned as a status of MidpProperties when suite is corrupted
 */
MidpProperties
midp_get_suite_properties(SuiteIdType suiteId) {
    pcsl_string filename;
    MidpProperties result = { 0, ALL_OK, NULL };
    int len;
    char* pszError;
    MIDPError status;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        result.numberOfProperties = 0;
        result.status =  OUT_OF_MEMORY;
        return result;
    }

    /*
    if (check_for_corrupted_suite(suiteId) == SUITE_CORRUPTED_ERROR) {
        result.numberOfProperties = 0;
        result.status = SUITE_CORRUPTED_ERROR;
        return result;
    }
    */

    if (get_property_file(suiteId, KNI_TRUE, &filename) != ALL_OK) {
        result.numberOfProperties = 0;
        result.status = NOT_FOUND;
        return result;
    }

    status = get_string_list(&pszError, &filename, &result.pStringArr, &len);
    pcsl_string_free(&filename);
    if (status != ALL_OK) {
        result.numberOfProperties = 0;
        result.status = status;
        storageFreeError(pszError);
        return result;
    }

    if (len < 0) {
        /* error */
        result.numberOfProperties = 0;
        result.status = GENERAL_ERROR;
    } else {
        /* each property is 2 strings (key and value) */
        result.numberOfProperties = len / 2;
    }

    return result;
}

/**
 * Retrieves the specified property value of the suite.
 *
 * IMPL_NOTE: this functions is introduced instead of 3 functions above.
 *
 * @param suiteId [in]  unique ID of the MIDlet suite
 * @param pKey    [in]  property name
 * @param pValue  [out] buffer to conatain returned property value
 *
 * @return ALL_OK if no errors,
 *         BAD_PARAMS if some parameter is invalid,
 *         NOT_FOUND if suite was not found,
 *         SUITE_CORRUPTED_ERROR if the suite is corrupted
 */
MIDPError
midp_get_suite_property(SuiteIdType suiteId,
                        const pcsl_string* pKey,
                        pcsl_string* pValue) {
    MidpProperties prop;
    pcsl_string* pPropFound;

    if (pKey == NULL || pValue == NULL) {
        return BAD_PARAMS;
    }

    (void)suiteId;
    (void)pKey;

    *pValue = PCSL_STRING_NULL;

    /* IMPL_NOTE: the following implementation should be optimized! */
    prop = midp_get_suite_properties(suiteId);
    if (prop.status != ALL_OK) {
        return prop.status;
    }

    pPropFound = midp_find_property(&prop, pKey);
// TODO !!!
//    midp_free_properties(&prop);

    if (pPropFound == NULL) {
        return NOT_FOUND;
    }

    *pValue = *pPropFound;

    return ALL_OK;                        
}


/* ------------------------------------------------------------ */
/*                          Implementation                      */
/* ------------------------------------------------------------ */

/**
 * Checks if a midlet suite or dynamic component with the given name
 * created by the given vendor exists.
 * If it does, this function returns a unique identifier of the suite
 * or component. Note that the suite or component may be corrupted even
 * if it exists. If it doesn't, a new suite ID or component ID is created.
 *
 * @param type type of the component
 * @param suiteId if type == COMPONENT_DYNAMIC, contains ID of the suite this
 *                components belongs to; unused otherwise
 * @param vendor name of the vendor that created the application or component,
 *        as given in a JAD file
 * @param name name of the suite or component, as given in a JAD file
 * @param pId [out] receives the platform-specific suite ID or component ID
 *        of the application given by vendor and name, or UNUSED_SUITE_ID /
 *        UNUSED_COMPONENT_ID if suite does not exist, or out of memory
 *        error occured, or the suite / component is corrupted.
 *
 * @return  ALL_OK if suite or component found,
 *          NOT_FOUND if suite or component does not exist (so a new ID
 *          was created), other error code in case of error
 */
static MIDPError
get_suite_or_component_id(ComponentType type, SuiteIdType suiteId,
                           const pcsl_string* vendor, const pcsl_string* name,
                           jint* pId) {
    MIDPError status;
    char *pszError;
    MidletSuiteData* pData;

#if ENABLE_DYNAMIC_COMPONENTS
    if (type != COMPONENT_DYNAMIC) {
        *pId = (jint)UNUSED_SUITE_ID;
    } else {
        *pId = (jint)UNUSED_COMPONENT_ID;
    }
#else
    (void)type;
    (void)suiteId;
    *pId = (jint)UNUSED_SUITE_ID;
#endif /* ENABLE_DYNAMIC_COMPONENTS */

    /* load _suites.dat */
    status = read_suites_data(&pszError);
    storageFreeError(pszError);
    if (status != ALL_OK) {
        return status;
    }

    pData = g_pSuitesData;

    /* try to find a suite */
    while (pData != NULL) {
        if (pcsl_string_equals(&pData->varSuiteData.suiteName, name) &&
                pcsl_string_equals(&pData->varSuiteData.suiteVendor, vendor)
#if ENABLE_DYNAMIC_COMPONENTS
                    && (type == pData->type) && (type != COMPONENT_DYNAMIC ||
                        (type == COMPONENT_DYNAMIC && suiteId == pData->suiteId))
#endif
        ) {
#if ENABLE_DYNAMIC_COMPONENTS
            if (type != COMPONENT_DYNAMIC) {
                *pId = (jint)pData->suiteId;
            } else {
                *pId = (jint)pData->componentId;
            }
#else
            *pId = (jint)pData->suiteId;
#endif /* ENABLE_DYNAMIC_COMPONENTS */
            return ALL_OK; /* IMPL_NOTE: consider SUITE_CORRUPTED_ERROR */
        }

        pData = pData->nextEntry;
    }

    /* suite or component was not found - create a new suite or component ID */
#if ENABLE_DYNAMIC_COMPONENTS
    if (type != COMPONENT_DYNAMIC) {
        status = midp_create_suite_id((SuiteIdType*)pId);
    } else {
        status = midp_create_component_id((ComponentIdType*)pId);
    }
#else
    status = midp_create_suite_id((SuiteIdType*)pId);
#endif /* ENABLE_DYNAMIC_COMPONENTS */

    return (status == ALL_OK) ? NOT_FOUND : status;
}

/**
 * Tells if a given suite is in a list of the installed suites.
 *
 * @param suiteId unique ID of the midlet suite
 *
 * @return ALL_OK if the suite is in the list of the installed suites,
 *         NOT_FOUND if not,
 *         IO_ERROR if an i/o error occured when reading the information
 *         about the installed suites,
 *         OUT_OF_MEMORY if out of memory or IO error,
 *         SUITE_CORRUPTED_ERROR is suite is found in the list, but it's
 *         corrupted.
 */
static MIDPError
suite_in_list(ComponentType type, SuiteIdType suiteId,
              ComponentIdType componentId) {
    MIDPError status;
    char* pszError;
    MidletSuiteData* pData;

#if !ENABLE_DYNAMIC_COMPONENTS
    /** to supress compilation warnings */
    (void)type;
    (void)componentId;
#endif

    /* load _suites.dat */
    status = read_suites_data(&pszError);
    storageFreeError(pszError);
    if (status != ALL_OK) {
        return status;
    }

#if ENABLE_DYNAMIC_COMPONENTS
    if (type == COMPONENT_DYNAMIC) {
        pData = get_component_data(componentId);
        if (pData == NULL) {
            status = NOT_FOUND;
        }
    } else {
#endif /* ENABLE_DYNAMIC_COMPONENTS */
        pData = get_suite_data(suiteId);

        if (pData != NULL) {
            /*
             * Make sure that suite is not corrupted. Return
             * SUITE_CORRUPTED_ERROR if the suite is corrupted.
             * Remove the suite before returning the status.
             */
            status = check_for_corrupted_suite(suiteId);
        } else {
            status = NOT_FOUND;
        }
#if ENABLE_DYNAMIC_COMPONENTS
    }
#endif

    return status;
}

/**
 * Gets the classpath for the specified MIDlet suite id.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using pcsl_mem_free().
 *
 * @param suiteId   The suite id used to identify the MIDlet suite
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param classPath The in/out parameter that contains returned class path
 * @param extension Extension of the file (
 *
 * @return one of the error codes:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY
 * </pre>
 */
static MIDPError
get_class_path_impl(ComponentType type,
                    SuiteIdType suiteId,
                    ComponentIdType componentId,
                    jint storageId,
                    pcsl_string * pClassPath,
                    const pcsl_string * pExtension) {
    const pcsl_string* root = storage_get_root(storageId);
    pcsl_string path = PCSL_STRING_NULL;
    jint suiteIdLen = GET_SUITE_ID_LEN(suiteId);
    jint componentIdLen = 0;

#if ENABLE_DYNAMIC_COMPONENTS
    if (type == COMPONENT_DYNAMIC) {
        componentIdLen = GET_COMPONENT_ID_LEN(componentId);
    }
#else
    (void)type;
    (void)componentId;
#endif

    *pClassPath = PCSL_STRING_NULL;

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&path, pcsl_string_length(root)
                                    + suiteIdLen + componentIdLen +
                                    + pcsl_string_length(pExtension));
    if (PCSL_STRING_OK != pcsl_string_append(&path, root) ||
            PCSL_STRING_OK != pcsl_string_append(&path,
                midp_suiteid2pcsl_string(suiteId))) {
        pcsl_string_free(&path);
        return OUT_OF_MEMORY;
    }

#if ENABLE_DYNAMIC_COMPONENTS
    if (type == COMPONENT_DYNAMIC) {
        if (PCSL_STRING_OK != pcsl_string_append(&path,
                midp_componentid2pcsl_string(componentId))) {
            pcsl_string_free(&path);
            return OUT_OF_MEMORY;
        }
    }
#endif

    if (PCSL_STRING_OK != pcsl_string_append(&path, pExtension)) {
        pcsl_string_free(&path);
        return OUT_OF_MEMORY;
    }

    *pClassPath = path;

    return ALL_OK;
}

/**
 * Retrieves the list of strings in a file.
 * The file has the number of strings at the front, each string
 * is a length and the jchars.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param pFilename name of the file of strings
 * @param paList pointer to an array of pcsl_strings, free with
 *               free_pcsl_string_list
 * @param pStringNum number of strings if successful (can be 0)
 *
 * @return error code (ALL_OK if successful)
 */
static MIDPError
get_string_list(char** ppszError, const pcsl_string* pFilename,
                pcsl_string** paList, int* pStringNum) {
    char* pszTemp;
    int i = 0;
    int handle;
    int numberOfStrings = 0;
    pcsl_string* pStrings = NULL;
    MIDPError status = ALL_OK;

    *ppszError = NULL;
    *paList = NULL;
    *pStringNum = 0;

    handle = storage_open(ppszError, pFilename, OPEN_READ);
    if (*ppszError != NULL) {
        return IO_ERROR;
    }

    do {
        storageRead(ppszError, handle, (char*)&numberOfStrings,
            sizeof (numberOfStrings));
        if (*ppszError != NULL) {
            status = IO_ERROR;
            break;
        }
        if (numberOfStrings == 0) {
            break;
        }

        pStrings = alloc_pcsl_string_list(numberOfStrings);
        if (pStrings == NULL) {
            status = OUT_OF_MEMORY;
            break;
        }

        for (i = 0; i < numberOfStrings; i++) {
            pStrings[i] = PCSL_STRING_NULL;
        }

        for (i = 0; i < numberOfStrings; i++) {
            storage_read_utf16_string(ppszError, handle, &pStrings[i]);
            if (*ppszError != NULL) {
                status = IO_ERROR;
                break;
            }

        }

        if (i != numberOfStrings) {
            status = SUITE_CORRUPTED_ERROR;
            break;
        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (status == ALL_OK) {
        *paList = pStrings;
        *pStringNum = numberOfStrings;
    } else if (pStrings != NULL) {
        free_pcsl_string_list(pStrings, i);
    }

    return status;
}
