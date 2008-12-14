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
 * This header file is interface to the common MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_COMMON_H_
#define _SUITESTORE_COMMON_H_

#include <kni.h>
#include <pcsl_string.h>
#include <midp_global_status.h>
#include <midpStorage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type of the midlet suite ID.
 */
typedef jint SuiteIdType;

/**
 * Type of the midlet suite's component ID.
 */
typedef jint ComponentIdType;

/**
 * Type of the AMS folder ID.
 */
typedef jint FolderIdType;

/**
 * Predefined suite IDs.
 */
#define UNUSED_SUITE_ID     0
#define INTERNAL_SUITE_ID  -1
#define MAX_SUITE_ID        2147483647

/** ID that is never used for a dynamic component of a midlet suite */
#define UNUSED_COMPONENT_ID 0
#define MAX_COMPONENT_ID    2147483647

/**
 * Length of string representation of the suite ID.
 * For example, "0000000F" for suiteId = 15.
 */
#define GET_SUITE_ID_LEN(suiteId) 8

/**
 * Length of string representation of the suite ID.
 */
#define GET_COMPONENT_ID_LEN(componentId) 8

/** A list of properties that can be searched by a key. */
typedef struct _Properties {
    /**
     * Number of properties, there are 2 Strings (key/value)
     * for each property
     */
    int numberOfProperties;
    /**
     * Creation status:
     *        0 success,
     *        OUT_OF_MEM_LEN if out of memory,
     *        IO_ERROR_LEN if an IO error occurred
     */
    int status;
    pcsl_string* pStringArr;
} MidpProperties;

/** Possible types of an installed component. */
typedef enum _ComponentType {
    COMPONENT_REGULAR_SUITE,
    COMPONENT_PREINSTALLED_SUITE,
    COMPONENT_DYNAMIC,
    /** force enum to be 4 bytes */
    COMPONENT_DUMMY = 0x10000000
} ComponentType;

/*
 * Maximal lenght of the hash. Currently MD5 is used so this value is set to 16.
 */
#define MAX_HASH_SIZE 16

/*
 * NOTE: maximal path length must not exceed the corresponding number
 * (now it is set to 128 2-byte characters) in the following macro.
 * 7 is the number of pcsl_strings in _variableLenSuiteData structure.
 */
#define MAX_VAR_SUITE_DATA_LEN ((128 * 2 + sizeof(jint)) * 7 + MAX_HASH_SIZE)

/**
 * A structure containing variable-length information (such as
 * a suite name and vendor name)about the installed midlet suites.
 */
typedef struct _variableLenSuiteData {
    /** Hash value (currently MD5) of the suite's jar file. */
    unsigned char* pJarHash;

    /**
     * jint (length) + UTF16 string
     * Class name of the midlet in single-midlet suite.
     * Not used (length field is 0) if the suite contains several midlets.
     */
    pcsl_string midletClassName;

    /**
     * jint (length) + UTF16 string
     * A name that will be displayed in the Application Manager.
     */
    pcsl_string displayName;

    /**
     * jint (length) + UTF16 string
     * Icon's name for this suite.
     */
    pcsl_string iconName;

    /**
     * jint (length) + UTF16 string
     * Vendor of the midlet suite.
     */
    pcsl_string suiteVendor;

    /**
     * jint (length) + UTF16 string
     * Name of the midlet suite.
     */
    pcsl_string suiteName;

    /**
     * jint (length) + UTF16 string
     * Version of the midlet suite.
     */
    pcsl_string suiteVersion;

    /**
     * jint (length) + UTF16 string
     * Full path to suite's jar file.
     */
    pcsl_string pathToJar;

    /**
     * jint (length) + UTF16 string
     * Full path to the settings files.
     */
    pcsl_string pathToSettings;
} VariableLenSuiteData;

/**
 * A structure containing all of the information about the installed
 * midlet suites required at the strartup time.
 * The first field in _suites.dat is "int suitesNum" - number of the
 * installed suites, then there is a list of MidletSuiteData structures.
 */
typedef struct _midletSuiteData {
    /**
     * Unique ID of the midlet suite
     * (0 means that this entry was removed).
     */
    SuiteIdType suiteId;

    /**
     * ID of the dynamic component (has meaning only if
     * the value of "type" field is COMPONENT_DYNAMIC).
     */
    ComponentIdType componentId;

    /**
     * ID of the storage (INTERNAL_STORAGE_ID for the internal storage
     * or another value for external storages).
     */
    jint storageId;

    /**
     * ID of the folder (see src/ams/ams_folders library for more info).
     */
    jint folderId;

    /** True if the suite enabled, false otherwise. */
    jboolean isEnabled;

    /** True if the suite is trusted, false otherwise. */
    jboolean isTrusted;

    /** True if the suite is temporary, false otherwise. */
    jboolean isTemporary;

    /** Number of midlets in this suite. */
    jint numberOfMidlets;

    /** Installation time (timestamp). */
    long installTime;

    /** Size of the midlet suite's jad file. */
    jint jadSize;

    /** Size of the midlet suite's jar file. */
    jint jarSize;

    /** Total size of all files except RMS belonging to the suite. */
    jint suiteSize;

    /** Size of the jar file hash. If it is 0, pJarHash field is empty. */
    jint jarHashLen;

    /**
     * Type of the component described by this structure.
     * If it is COMPONENT_PREINSTALLED_SUITE, this is a preinstalled
     * midlet suite and thus it should be prevented from being removed.
     */
    ComponentType type;

    /** A structure with string-represented information about the suite. */
    VariableLenSuiteData varSuiteData;

    /** True if it was checked that this suite is not corrupted. */
    jboolean isChecked;

    /** Application ID assigned by the external application manager. */
    jint externalAppId;

    /** Pointer to the next entry in the linked list. */
    struct _midletSuiteData* nextEntry;
} MidletSuiteData;

#define MIDLET_SUITE_DATA_SIZE (sizeof(MidletSuiteData) - \
    sizeof(VariableLenSuiteData) - sizeof(jboolean) - \
        sizeof(jint) - sizeof(MidletSuiteData*))

/**
 * Initializes the SuiteStore subsystem.
 */
MIDPError midp_suite_storage_init();

/**
 * Finalizes the SuiteStore subsystem.
 */
void midp_suite_storage_cleanup();

/**
 * Converts the given SuiteID to pcsl_string.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * @param value suite id to convert
 * @return pcsl_string representation of the given suite id
 */
const pcsl_string* midp_suiteid2pcsl_string(SuiteIdType value);

/**
 * Converts the given suite ID to array of chars.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * @param value suite id to convert
 *
 * @return char[] representation of the given suite ID
 * or NULL in case of error
 */
const char* midp_suiteid2chars(SuiteIdType value);

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Converts the given component ID to pcsl_string.
 * NOTE: this function returns a pointer to the static buffer!
 *
 * @param value component id to convert
 * @return pcsl_string representation of the given component id
 */
const pcsl_string* midp_componentid2pcsl_string(ComponentIdType value);
#endif

/**
 * Tells if a suite exists.
 *
 * @param suiteId ID of a suite
 *
 * @return ALL_OK if a suite exists,
 *         NOT_FOUND if not,
 *         OUT_OF_MEMORY if out of memory or IO error,
 *         IO_ERROR if IO error,
 *         SUITE_CORRUPTED_ERROR if suite is found in the list,
 *         but it's corrupted
 */
MIDPError midp_suite_exists(SuiteIdType suiteId);

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
MIDPError midp_suite_get_class_path(SuiteIdType suiteId,
                                    StorageIdType storageId,
                                    jboolean checkSuiteExists,
                                    pcsl_string *classPath);

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
MIDPError midp_component_exists(ComponentIdType componentId);

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
                                    pcsl_string *pClassPath);
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
MIDPError midp_suite_get_bin_app_path(SuiteIdType suiteId,
                                      StorageIdType storageId,
                                      pcsl_string *classPath);
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
                                        pcsl_string * pFileName);

/**
 * Retrieves an ID of the storage where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId The application suite ID
 * @param pSuiteId [out] receives an ID of the storage where the suite is stored
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
midp_suite_get_suite_storage(SuiteIdType suiteId, StorageIdType* pStorageId);

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
midp_suite_get_suite_folder(SuiteIdType suiteId, FolderIdType* pFolderId);

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
                  SuiteIdType* pSuiteId);

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * If the dynamic component exists, this function returns a unique identifier
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
                      const pcsl_string* name, ComponentIdType* pComponentId);
#endif /* ENABLE_DYNAMIC_COMPONENTS */

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
MidpProperties midp_get_suite_properties(SuiteIdType suiteId);

/**
 * Find and return the property the matches the given key.
 * The returned value need not be freed because it resides
 * in an internal data structure.
 *
 * @param pProperties property list
 * @param key key of property to find
 *
 * @return a pointer to the property value,
 *         or to PCSL_STRING_NULL if not found.
 */
pcsl_string*
midp_find_property(MidpProperties* pProperties, const pcsl_string* key);

/**
 * Free a list of properties. Does nothing if passed NULL.
 *
 * @param pProperties property list
 */
void midp_free_properties(MidpProperties* pProperties);

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
                        pcsl_string* pValue);

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_COMMON_H_ */
