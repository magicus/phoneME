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

#include <string.h>

#include <sni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpServices.h>
#include <midpUtilKni.h>
#include <midp_ams_status.h>

#include <suitestore_intern.h>
#include <suitestore_installer.h>
#include <suitestore_task_manager.h>
#include <suitestore_rms.h>
#include <suitestore_kni_util.h>

#if ENABLE_ICON_CACHE
#include <suitestore_icon_cache.h>
#endif

#if ENABLE_IMAGE_CACHE
#include <imageCache.h>
#endif

#if ENABLE_MONET
#if VERIFY_ONCE
#error Contradictory build settings: ENABLE_MONET=true and VERIFY_ONCE=true.
#endif /* VERIFY_ONCE */
#endif /* ENABLE_MONET */

/* ---------------------- IMPL_NOTE: remove midpport_... ---------------------- */

/**
 * Gets the handle for accessing MIDlet suite properties.
 * Also returns the number of name/value pairs read using that handle.
 *
 * @param suiteId   The suite id of the MIDlet suite
 * @param numProperties [out] The number of properties
 * @param propHadle [out] The property handle for accessing
 *                  MIDlet suite properties
  * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_open_properties(SuiteIdType suiteId,
                                          jint *numProperties,
                                          jint *propHandle) {
    pcsl_string filename = PCSL_STRING_NULL;
    jint numStrings = 0;
    MIDP_ERROR errorCode = MIDP_ERROR_NONE;

    char* pszError = NULL;
    *propHandle = -1;
    *numProperties = 0;
    do {
        errorCode = get_property_file(suiteId, KNI_TRUE, &filename);
        if (errorCode != MIDP_ERROR_NONE) {
            break;
        }

        *propHandle = storage_open(&pszError, &filename, OPEN_READ);
        if (pszError != NULL) {
          storageFreeError(pszError);
          errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
          break;
        }

        storageRead(&pszError, *propHandle, (char*)&numStrings, sizeof(jint));
        if (pszError != NULL) {
          storageFreeError(pszError);
          errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
          break;
        }

        *numProperties = numStrings/2;
    } while (0);

    pcsl_string_free(&filename);

    return errorCode;
}

/**
 * Retrieves the next MIDlet suite property associated with the passed in
 * property handle. Note that the memory for in/out parameters
 * key and property MUST be allocated  using midpMalloc().
 * The caller is responsible for freeing memory associated
 * with key and value. If NULL is returned for key and value then there are
 * no more properties to retrieve.
 *
 * @param propHandle    MIDlet suite property handle
 * @param key           An in/outparameter that will return
 *                      the key part of the property
 *                      (NULL is a valid return value)
 * @param keyLength     The length of the key string
 * @param value         An in/out parameter that will return
 *                      the value part of the property
 *                      (NULL is a valid return value).
 * @param valueLength   The length of the value string
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_CORRUPTED,
 * </pre>
 */
MIDP_ERROR midpport_suite_next_property(jint propHandle,
                                        jchar **key, jint *keyLength,
                                        jchar **value, jint *valueLength) {
  char* pszError = NULL;
  int bytesRead = 0;
  jchar *tempStr = NULL;
  jint tempLen = 0;

  *key = NULL;
  *value = NULL;
  *keyLength = 0;
  *valueLength = 0;

  /* load the key string */
  storageRead(&pszError, propHandle, (char*)&tempLen, sizeof (jint));
  if (pszError != NULL) {
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }

  tempStr = (jchar*)midpMalloc(tempLen * sizeof (jchar));
  if (tempStr == NULL) {
      return MIDP_ERROR_OUT_MEM;
  }

  bytesRead = storageRead(&pszError, propHandle,
                          (char*)tempStr, tempLen * sizeof (jchar));
  if (pszError != NULL ||
      (bytesRead != (signed)(tempLen * sizeof (jchar)))) {
    midpFree(tempStr);
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }

  *key = tempStr;
  *keyLength = tempLen;

  /* load the value string */
  storageRead(&pszError, propHandle, (char*)&tempLen, sizeof (jint));
  if (pszError != NULL) {
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }

  tempStr = (jchar*)midpMalloc(tempLen * sizeof (jchar));
  if (tempStr == NULL) {
    return MIDP_ERROR_OUT_MEM;
  }

  bytesRead = storageRead(&pszError, propHandle,
                          (char*)tempStr, tempLen * sizeof (jchar));
  if (pszError != NULL ||
      (bytesRead != (signed)(tempLen * sizeof (jchar)))) {
    midpFree(tempStr);
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }

  *value = tempStr;
  *valueLength = tempLen;

  return MIDP_ERROR_NONE;
}

/**
 * Closes the passed in MIDlet suite property handle.
 * It will be called with a valid propHandle returned
 * by midpport_suite_open_properties().
 *
 * @param propHandle   The MIDlet suite property handle
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_close_properties(jint propHandle) {
    char* pszError = NULL;
    storageClose(&pszError, propHandle);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }
    return MIDP_ERROR_NONE;
}


/* --------------------------- midpport_... end --------------------------- */


/**
 * Java interface for midp_suiteid2pcsl_string().
 *
 * @param suiteId unique ID of the suite
 *
 * @return string representation of the given suiteId
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_suiteIdToString) {
    pcsl_string nullStr = PCSL_STRING_NULL_INITIALIZER;
    const pcsl_string* pPcslStrSuiteId = &nullStr;
    int suiteId;

    KNI_StartHandles(1);
    KNI_DeclareHandle(hStrSuiteId);
    /* assert(sizeof(SuiteIdType) == sizeof(jint)); */
    suiteId = KNI_GetParameterAsInt(1);

    pPcslStrSuiteId = midp_suiteid2pcsl_string(suiteId);

    do {
        GET_PCSL_STRING_DATA_AND_LENGTH(pPcslStrSuiteId)
        if (PCSL_STRING_PARAMETER_ERROR(pPcslStrSuiteId)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_NewString(pPcslStrSuiteId_data, pPcslStrSuiteId_len,
                          hStrSuiteId);
        }
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
    } while (0);

    KNI_EndHandlesAndReturnObject(hStrSuiteId);
}

/**
 * Get the application binary image path for a suite.
 *
 * @param suiteId unique ID of the suite
 *
 * @return class path or null if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteAppImagePath) {
#if ENABLE_MONET
    pcsl_string class_path_str = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string* const class_path = &class_path_str;
    SuiteIdType suiteId;
    StorageIdType storageId;
    MIDPError merr;

    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    /* assert(sizeof(SuiteIdType) == sizeof(jint)); */
    suiteId = KNI_GetParameterAsInt(1);

    do {
        merr = midp_suite_get_suite_storage(suiteId, &storageId);
        if (merr != ALL_OK) {
            /* the suite was not found */
            break;
        }

        merr = midp_suite_get_bin_app_path(suiteId, storageId, class_path);

        if (merr == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } else if (merr == SUITE_CORRUPTED_ERROR) {
            KNI_ThrowNew(midpIOException, NULL);
            break;
        }

        if (merr != ALL_OK) {
            break;
        }

        GET_PCSL_STRING_DATA_AND_LENGTH(class_path)
        if (PCSL_STRING_PARAMETER_ERROR(class_path)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_NewString(class_path_data, class_path_len, tempHandle);
        }
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
    } while (0);

    pcsl_string_free(&class_path_str);
    KNI_EndHandlesAndReturnObject(tempHandle);

#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif /* ENABLE_MONET */
}

/**
 * Get the class path for a suite.
 *
 * @param suiteId unique ID of the suite
 *
 * @return class path or null if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteJarPath) {
    SuiteIdType suiteId;
    MIDPError status;
    pcsl_string classPath = PCSL_STRING_NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(resultHandle);

    suiteId = KNI_GetParameterAsInt(1);

    status = get_jar_path(COMPONENT_REGULAR_SUITE, (jint)suiteId,
                          &classPath);
    if (status != ALL_OK) {
        KNI_ThrowNew(midpRuntimeException, NULL);
        KNI_ReleaseHandle(resultHandle);
    } else {
        midp_jstring_from_pcsl_string(KNIPASSARGS &classPath, resultHandle);
        pcsl_string_free(&classPath);
    }

    KNI_EndHandlesAndReturnObject(resultHandle);
}

/**
 * Get the storage id for a suite.
 *
 * @param suiteId unique ID of the suite
 *
 * @return storage id or UNUSED_STORAGE_ID if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteStorageId) {
    SuiteIdType suiteId;
    StorageIdType storageId;
    MIDPError errorCode;

    suiteId = KNI_GetParameterAsInt(1);

    do {
        errorCode = midp_suite_get_suite_storage(suiteId, &storageId);
        if (errorCode != ALL_OK) {
            /* the suite was not found */
            storageId = UNUSED_STORAGE_ID;
            break;
        }
    } while (0);

    KNI_ReturnInt(storageId);
}

/**
 * Get the folder id for a suite.
 *
 * @param suiteId unique ID of the suite
 *
 * @return folder id or -1 if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteFolderId) {
    SuiteIdType suiteId;
    FolderIdType folderId;
    MIDPError errorCode;

    suiteId = KNI_GetParameterAsInt(1);

    do {
        errorCode = midp_suite_get_suite_folder(suiteId, &folderId);
        if (errorCode != ALL_OK) {
            /* the suite was not found */
            folderId = -1;
            break;
        }
    } while (0);

    KNI_ReturnInt(folderId);
}

/**
 * Native method String getSuiteID(String, String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Gets the unique identifier of the MIDlet suite defined by vendor and name.
 *
 * @param vendor name of the vendor that created the suite,
 *        as given in a JAD file
 * @param name name of the suite, as given in a JAD file
 *
 * @return ID of the midlet suite given by vendor and name or
 *         MIDletSuite.UNUSED_SUITE_ID if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getSuiteID) {
    MIDPError error;
    SuiteIdType suiteId = UNUSED_SUITE_ID;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(1, vendor)
    GET_PARAMETER_AS_PCSL_STRING(2, name)

    error = midp_get_suite_id(&vendor, &name, &suiteId);

    switch(error) {
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_CORRUPTED_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            suiteId = UNUSED_SUITE_ID;
            break;
        default:
            break;
    }

    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();

    KNI_ReturnInt(suiteId);
}

/**
 * Native method boolean suiteExists(int) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Tells if a suite exists.
 *
 * @param suiteId ID of a suite
 *
 * @return true if a suite of the given storage name
 *         already exists on the system
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_suiteExists) {
    jboolean exists = KNI_FALSE;
    int status;
    /* assert(sizeof(SuiteIdType) == sizeof(jint)); */
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    status = midp_suite_exists(suiteId);
    if (status == ALL_OK) {
        exists = KNI_TRUE;
    } else if (status == SUITE_CORRUPTED_ERROR) {
        remove_from_suite_list_and_save(suiteId);
        KNI_ThrowNew(midletsuiteCorrupted, NULL);
    } else if (status == OUT_OF_MEMORY) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnBoolean(exists);
}

/**
 * Fill in the authPath String array in an installInfo java object.
 *
 * @param src MidpInstallInfo struct with the auth path array
 * @param dst java InstallInfo object to fill
 * @param dstClass InstallInfo class object
 */
static void fillAuthPath(KNIDECLARGS MidpInstallInfo src, jobject dst, jclass dstClass) {
    int i;
    jfieldID authPathFieldId;
    int error = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(stringObj);
    KNI_DeclareHandle(authPathObj);

    do {
        if (src.authPathLen <= 0) {
            break;
        }

        SNI_NewArray(SNI_STRING_ARRAY, src.authPathLen, authPathObj);

        if (KNI_IsNullHandle(authPathObj)) {
            break;
        }

        for (i = 0; i < src.authPathLen; i++) {
            const pcsl_string* const apath = &src.authPath_as[i];
            GET_PCSL_STRING_DATA_AND_LENGTH(apath)
            if (PCSL_STRING_PARAMETER_ERROR(apath)) {
                error = 1;
            } else {
                KNI_NewString(apath_data, (jsize)apath_len, stringObj);
            }
            RELEASE_PCSL_STRING_DATA_AND_LENGTH
            if (error) {
                break;
            }
            KNI_SetObjectArrayElement(authPathObj, (jint)i, stringObj);
        }

        if (error) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            authPathFieldId = midp_get_field_id(KNIPASSARGS dstClass, "authPath",
                                                "[Ljava/lang/String;");
            KNI_SetObjectField(dst, authPathFieldId, authPathObj);
        }
    } while (0);

    KNI_EndHandles();
}

/**
 * Native method String createSuiteID(String, String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Returns a unique identifier of MIDlet suite.
 * Constructed from the combination
 * of the values of the <code>MIDlet-Name</code> and
 * <code>MIDlet-Vendor</code> attributes.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param name name of the suite, as given in a JAD file
 *
 * @return the platform-specific storage name of the application
 *          given by vendorName and appName
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_createSuiteID) {
    SuiteIdType suiteId = UNUSED_SUITE_ID;
    MIDPError rc;

    rc = midp_create_suite_id(&suiteId);
    if (rc != ALL_OK) {
        if (rc == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_ReturnInt(suiteId);
}

/**
 * Native method int getStorageUsed(String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Gets the amount of storage on the device that this suite is using.
 * This includes the JAD, JAR, management data, and RMS.
 *
 * @param suiteId  ID of the suite
 *
 * @return number of bytes of storage the suite is using
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getStorageUsed) {
    int used = 0;
    /* assert(sizeof(SuiteIdType) == sizeof(jint)); */
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    used = midp_get_suite_storage_size(suiteId);

    if (used < 0) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt((jint)used);
}

/**
 * Native method void getSuiteList(int[]) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Get the list installed of MIDlet suite IDs.
 *
 * @param suites empty array of integerss to fill with suite IDs
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getSuiteList) {
    int i;
    int numberOfStrings;
    int numberOfSuites = 0;
    MIDPError status;
    SuiteIdType* pSuites = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(suites);

    KNI_GetParameterAsObject(1, suites);

    numberOfStrings = (int)KNI_GetArrayLength(suites);

    do {
        if (numberOfStrings <= 0) {
            break;
        }

        status = midp_get_suite_ids(&pSuites, &numberOfSuites);
        if (status != ALL_OK) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        if (numberOfSuites == 0) {
            break;
        }

        if (numberOfStrings > numberOfSuites) {
            numberOfStrings = numberOfSuites;
        }

        for (i = 0; i < numberOfStrings; i++) {
            KNI_SetIntArrayElement(suites, (jint)i, pSuites[i]);
        }
    } while (0);

    midp_free_suite_ids(pSuites, numberOfSuites);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method int getNumberOfSuites() for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Get the number of installed of MIDlet suites.
 *
 * @return the number of installed suites or -1 in case of error
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getNumberOfSuites) {
    int numberOfSuites;
    MIDPError status = midp_get_number_of_suites(&numberOfSuites);
    if (status != ALL_OK) {
        numberOfSuites = status;
    }

    KNI_ReturnInt(numberOfSuites);
}

#if VERIFY_ONCE
/**
 * Read suite hash value and fill the verifyHash field of
 * the com.sun.midp.midletsuite.InstallInfo object with it
 *
 * @param suiteId  ID of the suite
 * @param installInfoObj object to fill
 * @return MIDP_ERROR_NONE if successful,
 *      OUT_OF_MEMORY if out of memory,
 *      IO_ERROR if an IO error occurred
 *
 * @throws OutOfMemoryError if not enough memory to read/fill information
 */
static MIDPError fillVerifyHash(SuiteIdType suiteId, jobject installInfoObj) {
    int len;
    jbyte* data = NULL;
    jfieldID fieldID;
    MIDPError status;

    KNI_StartHandles(2);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(field);

    status = readVerifyHash(suiteId, &data, &len);

    /**
     * Don't throw IOException on IO_ERROR status since a suite could
     * be successfully installed even with not passed classes verification.
     * In this case verify hash value can be unavailable, and suite must
     * be started each time with classes verification.
     */

    if (status == OUT_OF_MEMORY) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == ALL_OK) {
        SNI_NewArray(SNI_BYTE_ARRAY, len, field);
        if (KNI_IsNullHandle(field)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_GetObjectClass(installInfoObj, clazz);
            fieldID = midp_get_field_id(KNIPASSARGS clazz, "verifyHash", "[B");
            KNI_SetObjectField(installInfoObj, fieldID, field);
            KNI_SetRawArrayRegion(field, 0, len, data);
        }
        midpFree(data);
    }
    KNI_EndHandles();

    return status;
}
#endif /* VERIFY_ONCE */

/**
 * native void load() throws IOException;
 *
 * Populates this InstallInfo instance from persistent store.
 *
 * @throws IOException if the information cannot be read
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_InstallInfo_load) {
    MidpInstallInfo installInfo;
    char* pszError;
    jfieldID idFid;
    SuiteIdType suiteId;

    KNI_StartHandles(3);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(temp);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);
    idFid = midp_get_field_id(KNIPASSARGS clazz, "id", "I");
    suiteId = KNI_GetIntField(thisObj, idFid);

    installInfo = read_install_info(&pszError, suiteId);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    } else if (OUT_OF_MEM_INFO_STATUS(installInfo)) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        KNI_RESTORE_PCSL_STRING_FIELD(thisObj, clazz, "jadUrl",
                                 &installInfo.jadUrl_s, temp);
        KNI_RESTORE_PCSL_STRING_FIELD(thisObj, clazz, "jarUrl",
                                 &installInfo.jarUrl_s, temp);
        KNI_RESTORE_PCSL_STRING_FIELD(thisObj, clazz, "domain",
                                 &installInfo.domain_s, temp);
        KNI_RESTORE_BOOLEAN_FIELD(thisObj, clazz, "trusted",
                                  installInfo.trusted);

        fillAuthPath(KNIPASSARGS installInfo, thisObj, clazz);

#if VERIFY_ONCE
        (void)fillVerifyHash(suiteId, thisObj);
#endif /* VERIFY_ONCE */

        midp_free_install_info(&installInfo);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * native void load();
 *
 * Gets the suite settings suite from persistent store.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_SuiteSettings_load) {
    SuiteIdType suiteId = UNUSED_SUITE_ID;
    char* pszError;
    jboolean enabled;
    jbyte* pPermissions;
    int numberOfPermissions;
    int permissionsFieldLen;
    jbyte pushInterrupt;
    jint pushOptions;
    jfieldID permissionsFid;
    jfieldID suiteIdFid;

    KNI_StartHandles(3);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(permissionsField);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);
    permissionsFid = midp_get_field_id(KNIPASSARGS clazz, "permissions", "[B");
    suiteIdFid = midp_get_field_id(KNIPASSARGS clazz, "suiteId", "I");

    KNI_GetObjectField(thisObj, permissionsFid, permissionsField);
    permissionsFieldLen = (int)KNI_GetArrayLength(permissionsField);

    suiteId = KNI_GetIntField(thisObj, suiteIdFid);

    do {
        MIDPError status;
        status = read_settings(&pszError, suiteId, &enabled, &pushInterrupt,
            &pushOptions, &pPermissions, &numberOfPermissions);

        if (status != ALL_OK) {
            if (pszError != NULL) {
                KNI_ThrowNew(midpIOException, pszError);
                storageFreeError(pszError);
            } else {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            }
            break;
        }

        KNI_RESTORE_BOOLEAN_FIELD(thisObj, clazz, "enabled", enabled);
        KNI_RESTORE_BYTE_FIELD(thisObj, clazz, "pushInterruptSetting",
                               pushInterrupt);
        KNI_RESTORE_INT_FIELD(thisObj, clazz, "pushOptions",
                              pushOptions);

        if (numberOfPermissions > 0 && permissionsFieldLen > 0) {
            if (numberOfPermissions > permissionsFieldLen) {
                numberOfPermissions = permissionsFieldLen;
            }

            KNI_SetRawArrayRegion(permissionsField, 0, numberOfPermissions,
                                  (jbyte*)pPermissions);
        }

        midpFree(pPermissions);
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * private native void save0(
 *     jint suiteId,
 *     byte pushInterruptSetting,
 *     int pushOptions,
 *     byte[] permissions) throws IOException;
 *
 * Saves the suite settings to persistent store.
 *
 * @param suiteId ID of the suite
 * @param pushInterruptSetting push interrupt setting
 * @param pushOptions push options
 * @param permissions current permissions
 *
 * @throws IOException if an I/O error occurs
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_SuiteSettings_save0) {
    char* pszError;
    jbyte* pPermissions = NULL;
    int permissionsLen;
    jbyte pushInterrupt;
    jint pushOptions;
    jboolean enabled;
    SuiteIdType suiteId;
    MIDPError status;

    KNI_StartHandles(1);
    KNI_DeclareHandle(permissionsObj);

    suiteId = KNI_GetParameterAsInt(1);
    pushInterrupt = KNI_GetParameterAsByte(2);
    pushOptions = KNI_GetParameterAsInt(3);
    KNI_GetParameterAsObject(4, permissionsObj);

    permissionsLen = (int)KNI_GetArrayLength(permissionsObj);

    do {
        if (permissionsLen < 0) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        pPermissions = (jbyte*)midpMalloc(permissionsLen);
        if (pPermissions == NULL) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        KNI_GetRawArrayRegion(permissionsObj, 0, permissionsLen,
                              (jbyte*)pPermissions);

        status = read_enabled_state(&pszError, suiteId, &enabled);
        if (status != ALL_OK) {
            if (pszError != NULL) {
                KNI_ThrowNew(midpIOException, pszError);
                storageFreeError(pszError);
            } else {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            }
            break;
        }

        status = write_settings(&pszError, suiteId, enabled, pushInterrupt,
                                pushOptions, pPermissions, permissionsLen,
                                NULL);
        if (status != ALL_OK) {
            if (pszError != NULL) {
                KNI_ThrowNew(midpIOException, pszError);
                storageFreeError(pszError);
            } else {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            }
            break;
        }
    } while (0);

    if (pPermissions != NULL) {
        midpFree(pPermissions);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * native String[] load() throws IOException;
 *
 * Gets the suite properties from persistent store. Returns the
 * properties as an array of strings: key0, value0, key1, value1, etc.
 *
 * @return an array of property key-value pairs
 *
 * @throws IOException if an IO error occurs
 *
 * The format of the properties file is:
 * <pre>
 * <number of strings as int (2 strings per property)>
 *    {repeated for each property}
 *    <length of a property key as int>
 *    <property key as jchars>
 *    <length of property value as int>
 *    <property value as jchars>
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_SuiteProperties_load) {
    SuiteIdType suiteId = UNUSED_SUITE_ID;
    jchar* key_data     = NULL;
    jint   key_len      = NULL_LEN;
    jchar* value_data   = NULL;
    jint   value_len    = NULL_LEN;
    jint   numProperties;

    int i;
    jint handle;
    int  openError = 0;
    MIDP_ERROR error = MIDP_ERROR_NONE;
    jfieldID suiteIdFid;

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(properties);
    KNI_DeclareHandle(tempStringObj);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);

    suiteIdFid = midp_get_field_id(KNIPASSARGS clazz, "suiteId", "I");
    suiteId = KNI_GetIntField(thisObj, suiteIdFid);

    do {
        error = midpport_suite_open_properties(suiteId,
                                               &numProperties, &handle);

        if (error != MIDP_ERROR_NONE) {
            openError = 1;
            break;
        }

        SNI_NewArray(SNI_STRING_ARRAY,  numProperties*2, properties);
        if (KNI_IsNullHandle(properties)) {
            error = MIDP_ERROR_OUT_MEM;
            break;
        }

        for (i = 0; i < numProperties*2; i+=2) {
            error = midpport_suite_next_property(handle,
                                                 &key_data, &key_len,
                                                 &value_data, &value_len);
            if (error == MIDP_ERROR_NONE) {
                KNI_NewString(key_data, (jsize)key_len, tempStringObj);
                midpFree(key_data);
                KNI_SetObjectArrayElement(properties, (jint)i, tempStringObj);

                KNI_NewString(value_data, (jsize)value_len, tempStringObj);
                midpFree(value_data);
                KNI_SetObjectArrayElement(properties, (jint)i+1, tempStringObj);
            } else {
                /* error while read properties */
                midpFree(key_data);
                midpFree(value_data);
                break;
          }
        }
    } while(0);

    switch(error) {
        case MIDP_ERROR_OUT_MEM:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case MIDP_ERROR_AMS_SUITE_CORRUPTED:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        default:
            break;
    }

    if (!openError && (midpport_suite_close_properties(handle) ==
            MIDP_ERROR_AMS_SUITE_CORRUPTED) && (error == MIDP_ERROR_NONE)) {
        KNI_ThrowNew(midpIOException, NULL);
    }

    KNI_EndHandlesAndReturnObject(properties);
}


/**
 * Native method void disable(int) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * Disables a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param suiteId suite Id of the installed midlet suite
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_disable) {
    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    status = midp_disable_suite(suiteId);

    switch (status) {
        case ALL_OK:
            break;
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_LOCKED:
            KNI_ThrowNew(midletsuiteLocked, NULL);
            break;
        case IO_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
            break;
        default:
            KNI_ThrowNew(midpRuntimeException, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * Native method void enable(int) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * Enables a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param suiteId suite Id of the installed midlet suite
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_enable) {
    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    status = midp_enable_suite(suiteId);

    switch (status) {
        case ALL_OK:
            break;
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_LOCKED:
            KNI_ThrowNew(midletsuiteLocked, NULL);
            break;
        case IO_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
            break;
        default:
            KNI_ThrowNew(midpRuntimeException, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * Native method void remove0(int) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Removes a software package given its ID.
 * <p>
 * If the suite is in use it must continue to be available
 * to the other suites that are using it.  The resources it
 * consumes must not be released until it is not in use.
 *
 * @param id unique ID of the installed package
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_remove0) {
    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    status = midp_remove_suite(suiteId);

    if (status == OUT_OF_MEMORY) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == SUITE_LOCKED) {
        KNI_ThrowNew(midletsuiteLocked, NULL);
    } else if (status == BAD_PARAMS) {
        KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
    } else if (status != ALL_OK) {
        KNI_ThrowNew(midpRuntimeException, "Remove failed");
    }

    KNI_ReturnVoid();
}

#define GET_PROP_PARAM(NUM, PARAM, STRINGOBJ, PROPS, STATUS) { \
    int i; \
    int numberOfStrings; \
 \
    KNI_GetParameterAsObject(NUM, PARAM); \
 \
    numberOfStrings = (int)KNI_GetArrayLength(PARAM); \
 \
    (PROPS.pStringArr) = alloc_pcsl_string_list(numberOfStrings); \
    if ((PROPS.pStringArr) == NULL) { \
        (STATUS) = OUT_OF_MEMORY; \
        break; \
    } \
 \
    (PROPS.numberOfProperties) = numberOfStrings / 2; \
    for (i = 0; i < numberOfStrings; i++) { \
        KNI_GetObjectArrayElement(PARAM, (jint)i, STRINGOBJ); \
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string((STRINGOBJ), \
                &(PROPS.pStringArr[i]))) { \
            int j; \
            for (j = 0; j < i; j++) { \
                pcsl_string_free(&(PROPS.pStringArr[j])); \
            } \
            midpFree((PROPS.pStringArr)); \
            (PROPS.numberOfProperties) = 0; \
            (STATUS) = OUT_OF_MEMORY; \
            break; \
        } \
    } \
 \
    if ((STATUS) != ALL_OK) { \
        break; \
    } \
 \
    KNI_ReleaseHandle(STRINGOBJ); \
}

/**
 * Native method void storeSuiteVerifyHash(...) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Native method to store hash value of the suite with preverified classes
 *
 * @param suiteId unique ID of the suite
 * @param verifyHash hash value of the suite with preverified classes
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_storeSuiteVerifyHash) {

#if VERIFY_ONCE
    int verifyHashLen = 0;
    jbyte* pVerifyHash = NULL;
    MIDPError status = ALL_OK;
    SuiteIdType suiteId;

    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);

    do {
        GET_BYTE_ARRAY_PARAM(2, tempHandle, pVerifyHash, verifyHashLen, status);
        KNI_ReleaseHandle(tempHandle);

        suiteId = KNI_GetParameterAsInt(1);

        if (pVerifyHash != NULL) {
            status = writeVerifyHash(suiteId, pVerifyHash, verifyHashLen);
        }
    } while (0);

    midpFree(pVerifyHash);
    KNI_EndHandles();

    if (status == OUT_OF_MEMORY) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == IO_ERROR) {
        KNI_ThrowNew(midpIOException, NULL);
    } else if (status != ALL_OK) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

#endif /* VERIFY_ONCE */

    KNI_ReturnVoid();
}

/**
 * Native method void changeStorage(...) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Moves a software package with given suite ID to the specified storage.
 *
 * @param suiteId suite ID for the installed package
 * @param storageId new storage ID
 *
 * @exception IllegalArgumentException if the suite cannot be found or
 * invalid storage ID specified
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_changeStorage) {

    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    StorageIdType newStorageId = KNI_GetParameterAsInt(2);

    status = midp_change_suite_storage(suiteId, newStorageId);

    switch (status) {
        case ALL_OK:
            break;
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_LOCKED:
            KNI_ThrowNew(midletsuiteLocked, NULL);
            break;
        case IO_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
            break;
        case BAD_PARAMS:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite or storage ID");
            break;
        default:
            KNI_ThrowNew(midpRuntimeException, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * Native method void moveSuiteToFolder(...) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Moves a software package with given suite ID to the specified folder.
 *
 * @param suiteId suite ID for the installed package
 * @param newFolderId folder ID
 *
 * @exception IllegalArgumentException if the suite cannot be found or
 *                                     invalid folder ID specified
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 *                                       locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_moveSuiteToFolder) {

    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    FolderIdType newFolderId = KNI_GetParameterAsInt(2);

    status = midp_move_suite_to_folder(suiteId, newFolderId);

    switch (status) {
        case ALL_OK:
            break;
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_LOCKED:
            KNI_ThrowNew(midletsuiteLocked, NULL);
            break;
        case IO_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
            break;
        case BAD_PARAMS:
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite or folder ID");
            break;
        default:
            KNI_ThrowNew(midpRuntimeException, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * Native method void nativeStoreSuite(...) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Stores or updates a midlet suite.
 *
 * @param installInfo structure containing the following information:<br>
 * <pre>
 *     id - unique ID of the suite;
 *     jadUrl - where the JAD came from, can be null;
 *     jarUrl - where the JAR came from;
 *     jarFilename - name of the downloaded MIDlet suite jar file;
 *     suiteName - name of the suite;
 *     suiteVendor - vendor of the suite;
 *     suiteVersion - version of the suite;
 *     authPath - authPath if signed, the authorization path starting
 *                with the most trusted authority;
 *     domain - security domain of the suite;
 *     trusted - true if suite is trusted;
 *     verifyHash - may contain hash value of the suite with
 *                  preverified classes or may be NULL;
 * </pre>
 *
 * @param suiteSettings structure containing the following information:<br>
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
 * @param suiteData structure containing the following information:<br>
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
 * @param jadProps properties the JAD as an array of strings in
 *        key/value pair order, can be null if jadUrl is null
 *
 * @param jarProps properties of the manifest as an array of strings
 *        in key/value pair order
 *
 * @exception IOException is thrown, if an I/O error occurs during
 * storing the suite
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 * @exception IllegalArgumentException is thrown if any of input strings
 * (except jadUrl) is null or empty
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_nativeStoreSuite) {
    MIDPError status = ALL_OK;
    MidpInstallInfo installInfo;
    MidpSuiteSettings suiteSettings;
    MidletSuiteData suiteData;
    int exceptionThrown;

    /*
     * This is needed because KNI_SAVE_PCSL_STRING_FIELD() macro
     * calls pcsl_mem_free(destString) for some reason.
     */
    memset((char*)&suiteData, 0, sizeof(MidletSuiteData));
    memset((char*)&installInfo, 0, sizeof(MidpInstallInfo));
    memset((char*)&suiteSettings, 0, sizeof(MidpSuiteSettings));

    installInfo.jadProps.status = ALL_OK;
    installInfo.jarProps.status = ALL_OK;

    /* get parameters */
    KNI_StartHandles(10);
    KNI_DeclareHandle(javaInstallInfo);
    KNI_DeclareHandle(javaSuiteSettings);
    KNI_DeclareHandle(javaSuiteData);
    KNI_DeclareHandle(javaComponentData);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(tmpHandle);
    KNI_DeclareHandle(tmpHandle2);
    KNI_DeclareHandle(permissionsObj);
    KNI_DeclareHandle(authPathObj);
    KNI_DeclareHandle(verifyHashObj);

    KNI_GetParameterAsObject(1, javaInstallInfo);
    KNI_GetParameterAsObject(2, javaSuiteSettings);
    KNI_GetParameterAsObject(3, javaSuiteData);
    KNI_GetParameterAsObject(4, javaComponentData);

    /* get fields from the java objects passed as parameters */

    /*
     * KNI_SAVE_PCSL_STRING_FIELD macro may throw an OutOfMemoryException,
     * so this will be the default status if we leave the loop; exceptionThrown
     * flag indicates that the exception was already thrown thus there is
     * no need to throw it at the end of the function.
     */
    status = OUT_OF_MEMORY;
    exceptionThrown = 1;

    do {
        /* 1 - from javaInstallInfo object */
        KNI_GetObjectClass(javaInstallInfo, clazz);

        KNI_SAVE_INT_FIELD(javaInstallInfo, clazz, "id",
                           suiteData.suiteId);
        suiteSettings.suiteId = suiteData.suiteId;
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "jadUrl",
                                   &installInfo.jadUrl_s, tmpHandle);
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "jarUrl",
                                   &installInfo.jarUrl_s, tmpHandle);
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "domain",
                                   &installInfo.domain_s, tmpHandle);
        KNI_SAVE_BOOLEAN_FIELD(javaInstallInfo, clazz, "trusted",
                               installInfo.trusted);

        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "jarFilename",
            &suiteData.varSuiteData.pathToJar, tmpHandle);
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "suiteName",
            &suiteData.varSuiteData.suiteName, tmpHandle);
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "suiteVendor",
            &suiteData.varSuiteData.suiteVendor, tmpHandle);
        KNI_SAVE_PCSL_STRING_FIELD(javaInstallInfo, clazz, "suiteVersion",
            &suiteData.varSuiteData.suiteVersion, tmpHandle);
        KNI_SAVE_INT_FIELD(javaInstallInfo, clazz, "expectedJarSize",
                           suiteData.jarSize);
        KNI_GetObjectField(javaInstallInfo,
                           midp_get_field_id(KNIPASSARGS clazz,
                               "authPath", "[Ljava/lang/String;"), authPathObj);
        installInfo.authPathLen = KNI_GetArrayLength(authPathObj);

        KNI_GetObjectField(javaInstallInfo,
                           midp_get_field_id(KNIPASSARGS clazz,
                               "verifyHash", "[B"), verifyHashObj);
        suiteData.jarHashLen = KNI_GetArrayLength(verifyHashObj);
        
        KNI_SAVE_BOOLEAN_FIELD(javaInstallInfo, clazz, "temporary",
                               suiteData.isTemporary);

        /* 2 - from javaSuiteSettings object */
        KNI_GetObjectClass(javaSuiteSettings, clazz);

        KNI_SAVE_INT_FIELD(javaSuiteSettings, clazz, "pushOptions",
                           suiteSettings.pushOptions);
        KNI_SAVE_BYTE_FIELD(javaSuiteSettings, clazz, "pushInterruptSetting",
                            suiteSettings.pushInterruptSetting);
        KNI_SAVE_BOOLEAN_FIELD(javaSuiteSettings, clazz, "enabled",
                               suiteSettings.enabled);
        suiteData.isEnabled = suiteSettings.enabled;

        KNI_GetObjectField(javaSuiteSettings,
                           midp_get_field_id(KNIPASSARGS clazz,
                               "permissions", "[B"), permissionsObj);

        suiteSettings.permissionsLen = KNI_GetArrayLength(permissionsObj);

        /*
         * 3 - from javaSuiteData or javaComponentData object, depending on
         * what is being installed
         */
#if ENABLE_DYNAMIC_COMPONENTS
        if (KNI_IsNullHandle(javaSuiteData) == KNI_FALSE) {
            suiteData.componentId = UNUSED_COMPONENT_ID;
#endif /* ENABLE_DYNAMIC_COMPONENTS */

            /* this is a midlet suite */
            suiteData.type = COMPONENT_REGULAR_SUITE;

            KNI_GetObjectClass(javaSuiteData, clazz);

            KNI_SAVE_INT_FIELD(javaSuiteData, clazz, "storageId",
                               suiteData.storageId);
            KNI_SAVE_INT_FIELD(javaSuiteData, clazz, "folderId",
                               suiteData.folderId);
            KNI_SAVE_INT_FIELD(javaSuiteData, clazz, "numberOfMidlets",
                               suiteData.numberOfMidlets);
            KNI_SAVE_PCSL_STRING_FIELD(javaSuiteData, clazz, "displayName",
                &suiteData.varSuiteData.displayName, tmpHandle);
            KNI_SAVE_PCSL_STRING_FIELD(javaSuiteData, clazz, "iconName",
                &suiteData.varSuiteData.iconName, tmpHandle);
            KNI_SAVE_PCSL_STRING_FIELD(javaSuiteData, clazz, "midletToRun",
                &suiteData.varSuiteData.midletClassName, tmpHandle);
#if ENABLE_DYNAMIC_COMPONENTS
        } else {
            /* this is a dynamic component of a midlet suite */
            suiteData.type = COMPONENT_DYNAMIC;

            KNI_GetObjectClass(javaComponentData, clazz);

            suiteData.storageId = INTERNAL_STORAGE_ID;
            KNI_SAVE_INT_FIELD(javaComponentData, clazz, "componentId",
                               suiteData.componentId);
            KNI_SAVE_PCSL_STRING_FIELD(javaComponentData, clazz, "displayName",
                &suiteData.varSuiteData.displayName, tmpHandle);
        }
#endif /* ENABLE_DYNAMIC_COMPONENTS */

        /* fill in arrays */
        exceptionThrown = 0; /* no more KNI macro bellow */
        status = ALL_OK;

        /* fill in authPath */
        if (installInfo.authPathLen > 0) {
            jint i;
            installInfo.authPath_as = (pcsl_string*)midpMalloc(
                installInfo.authPathLen * sizeof(pcsl_string));
            if (installInfo.authPath_as == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }

            for (i = 0; i < installInfo.authPathLen; i++) {
                KNI_GetObjectArrayElement(authPathObj, i, tmpHandle);

                if (KNI_GetStringLength(tmpHandle) >= 0) {
                    if (midp_jstring_to_pcsl_string(tmpHandle,
                            &installInfo.authPath_as[i]) != PCSL_STRING_OK) {
                        status = OUT_OF_MEMORY;
                        break;
                    }
                } else {
                    installInfo.authPath_as[i] = PCSL_STRING_NULL;
                }
            }

            if (status != ALL_OK) {
                break;
            }
        } else {
            installInfo.authPath_as = NULL;
        }

        /* fill in permissions */
        if (suiteSettings.permissionsLen > 0) {
            suiteSettings.pPermissions =
                (jbyte*)midpMalloc(suiteSettings.permissionsLen);
            if (suiteSettings.pPermissions == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }
            KNI_GetRawArrayRegion(permissionsObj, 0,
                suiteSettings.permissionsLen, suiteSettings.pPermissions);
        } else {
            suiteSettings.pPermissions = NULL;
        }

        /* fill in verifyHash */
        if (suiteData.jarHashLen > 0) {
            suiteData.varSuiteData.pJarHash =
                (unsigned char*)midpMalloc(suiteData.jarHashLen);
            if (suiteData.varSuiteData.pJarHash == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }
            KNI_GetRawArrayRegion(verifyHashObj, 0, suiteData.jarHashLen,
                                  (jbyte*)suiteData.varSuiteData.pJarHash);
        } else {
            suiteData.varSuiteData.pJarHash = NULL;
        }

        /* get jad and jar properties */
        if (!pcsl_string_is_null(&installInfo.jadUrl_s)) {
            GET_PROP_PARAM(5, tmpHandle, tmpHandle2,
                           installInfo.jadProps, status);
        }

        GET_PROP_PARAM(6, tmpHandle, tmpHandle2, installInfo.jarProps, status);
    } while (0);

    if (status == ALL_OK) {
        /* store the suite */
        status = midp_store_suite(&installInfo, &suiteSettings, &suiteData);
    }

    /* cleanup */
    if (suiteSettings.permissionsLen > 0 &&
            suiteSettings.pPermissions != NULL) {
        midpFree((void*)suiteSettings.pPermissions);
    }

    if (installInfo.authPathLen > 0 && installInfo.authPath_as != NULL) {
        free_pcsl_string_list(installInfo.authPath_as, installInfo.authPathLen);
    }

    if (!pcsl_string_is_null(&installInfo.jadUrl_s)) {
        if (installInfo.jadProps.pStringArr != NULL) {
            free_pcsl_string_list(installInfo.jadProps.pStringArr,
                                  installInfo.jadProps.numberOfProperties * 2);
        }
        pcsl_string_free(&installInfo.jadUrl_s);
    }

    if (installInfo.jarProps.pStringArr != NULL) {
        free_pcsl_string_list(installInfo.jarProps.pStringArr,
                              installInfo.jarProps.numberOfProperties * 2);
    }

    pcsl_string_free(&installInfo.jarUrl_s);
    pcsl_string_free(&installInfo.domain_s);
    
    pcsl_string_free(&suiteData.varSuiteData.pathToJar);

    /* end of cleanup */

    /* throw an exception if an error occured */
    if (status != ALL_OK && !exceptionThrown) {
        if (status == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status == IO_ERROR) {
            KNI_ThrowNew(midpIOException, NULL);
        } else if (status == SUITE_LOCKED) {
            KNI_ThrowNew(midletsuiteLocked, NULL);
        } else { /* includes BAD_PARAMS */
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Reads the basic information about the midlet suite from the storage.
 *
 * @param suiteId unique ID of the suite
 * @param msi object to fill
 *
 * @exception IOException if an the information cannot be read
 * @exception IllegalArgumentException if suiteId is invalid
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMIDletSuiteInfoImpl0) {
    SuiteIdType suiteId;
    MIDPError status = ALL_OK;

    KNI_StartHandles(3);
    KNI_DeclareHandle(msi);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(tmpHandle);

    suiteId = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, msi);
    KNI_GetObjectClass(msi, clazz);

    do {
        char *pszError = NULL;
        MidletSuiteData *pData = NULL;

        /* Ensure that suite data are read */
        status = read_suites_data(&pszError);
        storageFreeError(pszError);
        if (status != ALL_OK) {
            break;
        }

        pData = get_suite_data(suiteId);
        if (!pData) {
            status = NOT_FOUND;
            break;
        }

        KNI_RESTORE_PCSL_STRING_FIELD(msi, clazz, "displayName",
            &(pData->varSuiteData.displayName), tmpHandle);

        KNI_RESTORE_PCSL_STRING_FIELD(msi, clazz, "iconName",
            &(pData->varSuiteData.iconName), tmpHandle);

        KNI_RESTORE_PCSL_STRING_FIELD(msi, clazz, "midletToRun",
            &(pData->varSuiteData.midletClassName), tmpHandle);

        KNI_RESTORE_INT_FIELD(msi, clazz, "storageId", pData->storageId);
        KNI_RESTORE_INT_FIELD(msi, clazz, "folderId",  pData->folderId);
        KNI_RESTORE_INT_FIELD(msi, clazz, "numberOfMidlets",
            pData->numberOfMidlets);
        KNI_RESTORE_BOOLEAN_FIELD(msi, clazz, "enabled", pData->isEnabled);
        KNI_RESTORE_BOOLEAN_FIELD(msi, clazz, "trusted", pData->isTrusted);
        KNI_RESTORE_BOOLEAN_FIELD(msi, clazz, "temporary", pData->isTemporary);
    } while (0);

    KNI_EndHandles();

    if (status != ALL_OK) {
        if (status == NOT_FOUND) {
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_ReturnVoid();
}

/**
 * Loads image data of a suite icon from the image cache.
 * <p>
 * Java declaration:
 * <pre>
 *     byte[] loadCachedIcon0(String suiteId, String iconName);
 * </pre>
 *
 * @param suiteId the suite Id
 * @param iconName the name of the image resource
 * @return Java byte[] object with icon data,
 *     or NULL if the data wasn't found in the cache
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_loadCachedIcon0) {
#if ENABLE_IMAGE_CACHE
    SuiteIdType suiteID;
    int length;
    unsigned char *buffer = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(iconBytesArray);
    suiteID = KNI_GetParameterAsInt(1);

    GET_PARAMETER_AS_PCSL_STRING(2, iconName)
    length = loadImageFromCache(suiteID, &iconName, &buffer);
    if (length != -1 && buffer != NULL) {

        /* Create byte array object to return as result */
        SNI_NewArray(SNI_BYTE_ARRAY, length, iconBytesArray);
        if (KNI_IsNullHandle(iconBytesArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_SetRawArrayRegion(iconBytesArray, 0,
                length, (jbyte *)buffer);
        }
        midpFree(buffer);
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandlesAndReturnObject(iconBytesArray);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_ReleaseHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif
}

/**
 * Loads the cached icons from the permanent storage into memory.
 *
 * @return status code (ALL_OK = 0 if no errors)
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_loadSuitesIcons0) {
#if ENABLE_ICON_CACHE
    MIDPError status = midp_load_suites_icons();
    KNI_ReturnInt(status);
#else
    KNI_ReturnInt(ALL_OK);
#endif
}

/**
 * Retrieves the cached icon from the icon cache.
 *
 * @param suiteId unique identifier of the suite
 * @param iconName the name of the icon to retrieve
 *
 * @return cached image data if available, otherwise null
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getMIDletSuiteIcon0) {
#if ENABLE_ICON_CACHE
    SuiteIdType suiteID;
    int length;
    unsigned char *buffer = NULL;
    MIDPError status;

    KNI_StartHandles(2);
    KNI_DeclareHandle(iconBytesArray);
    suiteID = KNI_GetParameterAsInt(1);

    GET_PARAMETER_AS_PCSL_STRING(2, iconName)

    status = midp_get_suite_icon(suiteID, &iconName, &buffer, &length);

    if (status == ALL_OK && length != -1 && buffer != NULL) {
        /* Create byte array object to return as result */
        SNI_NewArray(SNI_BYTE_ARRAY, length, iconBytesArray);
        if (KNI_IsNullHandle(iconBytesArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_SetRawArrayRegion(iconBytesArray, 0,
                length, (jbyte *)buffer);
        }
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandlesAndReturnObject(iconBytesArray);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(iconBytesArray);
    KNI_ReleaseHandle(iconBytesArray);
    KNI_EndHandlesAndReturnObject(iconBytesArray);
#endif
}

/**
 * Get the filename base for a suite.
 *
 * @param suiteId unique ID of the suite
 *
 * @return filename base for the suite id
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getSecureFilenameBase) {
    pcsl_string filenameBase = PCSL_STRING_NULL;
    SuiteIdType suiteId;
    MIDPError errorCode;

    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);

    suiteId = (SuiteIdType)KNI_GetParameterAsInt(1);
    do {

        errorCode = build_suite_filename(suiteId, &PCSL_STRING_EMPTY, 
                                         &filenameBase);

        if (errorCode == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } else if (errorCode == SUITE_CORRUPTED_ERROR) {
            KNI_ThrowNew(midpIOException, NULL);
            break;
        }

        if (errorCode != ALL_OK) {
            break;
        }

        midp_jstring_from_pcsl_string(KNIPASSARGS &filenameBase, tempHandle);
    } while (0);

    pcsl_string_free(&filenameBase);
    KNI_EndHandlesAndReturnObject(tempHandle);
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
 * @return 0 if no errors,
 *         1 if the suite database was corrupted but has been successfully
 *           repaired,
 *         a negative value if the database is corrupted and could not
 *         be repaired
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_checkSuitesIntegrity) {
    jboolean fullCheck, delCorruptedSuites, retCode;
    MIDPError status;

    fullCheck = KNI_GetParameterAsBoolean(1);
    delCorruptedSuites = KNI_GetParameterAsInt(2);

    status = midp_check_suites_integrity((fullCheck == KNI_TRUE),
                                         (delCorruptedSuites == KNI_TRUE));

    if (status == ALL_OK) {
        retCode = 0;
    } else if (status == SUITE_CORRUPTED_ERROR) {
        retCode = 1;
    } else {
        retCode = (jint)status;
        if (retCode > 0) {
            /*
             * to guarantee that this function will return a negative
             * value in case of error
             */
            retCode = -retCode;
        }
    }

    KNI_ReturnInt(retCode);
}


/**
 * Returns absolute path of cached resource file
 * <p>
 * Java declaration:
 * <pre>
 *     String getCachedFilePath0(int suiteId, String resName);
 * </pre>
 *
 * @param suiteId the suite Id
 * @param resName the name of the resource file
 * @return Java byte[] object with icon data,
 *     or NULL if the data wasn't found in the cache
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteStorage_getCachedFilePath0) {
#if ENABLE_FONT_CACHE
    pcsl_string pcsl_path = PCSL_STRING_NULL;
    SuiteIdType suiteID;
    StorageIdType storageId;
    int errorCode;

    KNI_StartHandles(2);
    KNI_DeclareHandle(pathHandle);
    suiteID = KNI_GetParameterAsInt(1);

    GET_PARAMETER_AS_PCSL_STRING(2, resName)
    
    errorCode = midp_suite_get_suite_storage(suiteID, &storageId);
    if (errorCode != MIDP_ERROR_NONE) {
        KNI_ThrowNew(midpIOException, NULL);
        KNI_ReleaseHandle(pathHandle);
    }
    
    errorCode = midp_suite_get_cached_resource_filename(suiteID, storageId,
                                                        &resName, &pcsl_path);
    if (errorCode == MIDP_ERROR_NONE) {
        midp_jstring_from_pcsl_string(KNIPASSARGS &pcsl_path, pathHandle);
    }
                                                       
    RELEASE_PCSL_STRING_PARAMETER
    pcsl_string_free(&pcsl_path);

    KNI_EndHandlesAndReturnObject(pathHandle);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_ReleaseHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif
}
