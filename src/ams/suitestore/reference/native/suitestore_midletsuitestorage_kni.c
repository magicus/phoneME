/*
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

#include <string.h>

#include <sni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <suitestore_export.h>
#include <suitestore_intern.h>
#include <suitestore_util.h>
#include <suitestore_kni_util.h>
#include <suitestore_verify_hash.h>
#include <suitestore_port.h>
#include <midpStorage.h>
#include <midpServices.h>
#include <midpUtilKni.h>
#include <midp_ams_status.h>

#if ENABLE_MONET
#if VERIFY_ONCE
#error Contradictory build settings: ENABLE_MONET=true and VERIFY_ONCE=true.
#endif /* VERIFY_ONCE */
#endif /* ENABLE_MONET */

/**
 * Get the application binary image path for a suite.
 *
 * @param id unique ID of the suite
 *
 * @return class path or null if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteAppImagePath() {
#if ENABLE_MONET
    pcsl_string class_path_str = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string* const class_path = &class_path_str;
    MIDP_ERROR merr;
    KNI_StartHandles(2);
    KNI_DeclareHandle(tempHandle);
    GET_PARAMETER_AS_PCSL_STRING(1,suite_id)
    do {
        merr = midp_suite_get_bin_app_path(&suite_id, class_path);

        if (merr == MIDP_ERROR_OUT_MEM) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } else if (merr == MIDP_ERROR_AMS_SUITE_CORRUPTED) {
            KNI_ThrowNew(midpIOException, NULL);
            break;
        }
        
        if (merr != MIDP_ERROR_NONE) {
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
    RELEASE_PCSL_STRING_PARAMETER

    pcsl_string_free(&class_path_str);
    KNI_EndHandlesAndReturnObject(tempHandle);

#else
    return NULL;
#endif /* ENABLE_MONET */
}

/**
 * Get the class path for a suite.
 *
 * @param id unique ID of the suite
 *
 * @return class path or null if the suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getMidletSuiteJarPath() {
    pcsl_string classPath = PCSL_STRING_NULL;
    jint errorCode;

    KNI_StartHandles(2);
    KNI_DeclareHandle(tempHandle);
    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)

    do {
        errorCode = midp_suite_get_class_path(&suiteID, KNI_TRUE, &classPath);

        if (errorCode == MIDP_ERROR_OUT_MEM) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } else if (errorCode == MIDP_ERROR_AMS_SUITE_CORRUPTED) {
            KNI_ThrowNew(midpIOException, NULL);
            break;
        }

        if (errorCode != MIDP_ERROR_NONE) {
            break;
        }

        midp_jstring_from_pcsl_string(&classPath, tempHandle);
    } while (0);

    RELEASE_PCSL_STRING_PARAMETER
    pcsl_string_free(&classPath);

    KNI_EndHandlesAndReturnObject(tempHandle);
}

/**
 * Native method String getSuiteID(String, String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Gets the unique identifier of MIDlet suite.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param name name of the suite, as given in a JAD file
 *
 * @return the platform-specific storage name of the application
 *          given by vendorName and appName, or null if suite does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getSuiteID() {
    MIDP_ERROR error;
 
    KNI_StartHandles(3);
    KNI_DeclareHandle(suiteIDStr);
    KNI_ReleaseHandle(suiteIDStr);

    GET_PARAMETER_AS_PCSL_STRING(1, vendor)
    GET_PARAMETER_AS_PCSL_STRING(2, name)
    do {
        jchar * suiteID_data = NULL;
        jint suiteID_len = NULL_LEN;
        const pcsl_string * const p_vendor = &vendor;
        const pcsl_string * const p_name = &name;
        GET_PCSL_STRING_DATA_AND_LENGTH(p_vendor)
        if (PCSL_STRING_PARAMETER_ERROR(p_vendor)) {
            error = MIDP_ERROR_OUT_MEM;
        } else {
            GET_PCSL_STRING_DATA_AND_LENGTH(p_name)
            if (PCSL_STRING_PARAMETER_ERROR(p_name)) {
                error = MIDP_ERROR_OUT_MEM;
            } else {
                error = midpport_suite_get_id((jchar *)p_vendor_data,
                                              p_vendor_len,
                                              (jchar *)p_name_data,
                                              p_name_len,
                                              &suiteID_data, &suiteID_len);
            }
            RELEASE_PCSL_STRING_DATA_AND_LENGTH
        }
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
        switch(error) {
        case MIDP_ERROR_OUT_MEM:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case MIDP_ERROR_AMS_SUITE_CORRUPTED:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case MIDP_ERROR_AMS_SUITE_NOT_FOUND:
        default:
            break;
        }

        /*
         * IMPL_NOTE: review behaviour of this code; what happens if some error
         * occures in midpport_suite_get_id?
         */
        KNI_NewString(suiteID_data, suiteID_len, suiteIDStr);
        midpFree(suiteID_data);
    } while (0);
    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandlesAndReturnObject(suiteIDStr);
}

/**
 * Native method boolean suiteExists(String) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Tells if a suite exists.
 *
 * @param id ID of a suite
 *
 * @return true if a suite of the given storage name
 *          already exists on the system
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_suiteExists() {
    jboolean exists = KNI_FALSE;
    int status;
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, suiteID) {
        status = suite_exists(&suiteID);
        if (status == SUITE_CORRUPTED_ERROR) {
            removeFromSuiteList(&suiteID);
            KNI_ThrowNew(midletsuiteCorrupted, NULL);
        } else if (status == OUT_OF_MEM_LEN) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status > 0) {
            exists = KNI_TRUE;
        }
    } RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnBoolean(exists);
}

/**
 * Fill in the authPath String array in an installInfo java object.
 *
 * @param src MidpInstallInfo struct with the auth path array
 * @param dst java InstallInfo object to fill
 * @param dstClass InstallInfo class object
 */
static void fillAuthPath(MidpInstallInfo src, jobject dst, jclass dstClass) {
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
            authPathFieldId = midp_get_field_id(dstClass, "authPath",
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
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_createSuiteID() {
    pcsl_string v_suiteid = PCSL_STRING_NULL;
    pcsl_string* const suiteid = &v_suiteid;
    KNI_StartHandles(3);
    KNI_DeclareHandle(suiteIDStr);

    GET_PARAMETER_AS_PCSL_STRING(1, vendor)
    GET_PARAMETER_AS_PCSL_STRING(2, name)
    do {
        MIDPError rc = OUT_OF_MEM_LEN;
        rc = midp_create_suite_id(&vendor, &name, suiteid);
        if (rc == OUT_OF_MEM_LEN) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        if (!pcsl_string_is_null(suiteid)) {
            GET_PCSL_STRING_DATA_AND_LENGTH(suiteid)
                if(PCSL_STRING_PARAMETER_ERROR(suiteid)) {
                    KNI_ThrowNew(midpOutOfMemoryError, NULL);
                } else {
                    KNI_NewString(suiteid_data, suiteid_len, suiteIDStr);
                }
            RELEASE_PCSL_STRING_DATA_AND_LENGTH
        }

    } while (0);
    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER

    pcsl_string_free(suiteid);

    KNI_EndHandlesAndReturnObject(suiteIDStr);
}

/**
 * Native method int getStorageUsed(String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Gets the amount of storage on the device that this suite is using.
 * This includes the JAD, JAR, management data, and RMS.
 *
 * @param suiteID  ID of the suite
 *
 * @return number of bytes of storage the suite is using
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getStorageUsed() {
    int used = 0;

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
    used = midp_get_suite_storage_size(&suiteID);
    if (used < 0) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnInt((jint)used);
}

/**
 * Native method void getSuiteList(String[]) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Get the list installed of MIDlet suite IDs.
 *
 * @param suites empty array of strings to fill with suite IDs
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getSuiteList() {
    int numberOfStrings;
    int numberOfSuites = 0;
    pcsl_string* pSuites = NULL;
    int i;
    KNI_StartHandles(2);
    KNI_DeclareHandle(suites);
    KNI_DeclareHandle(tempStringObj);

    KNI_GetParameterAsObject(1, suites);

    numberOfStrings = (int)KNI_GetArrayLength(suites);

    do {
        if (numberOfStrings <= 0) {
            break;
        }

        numberOfSuites = midpGetSuiteIDs(&pSuites);
        if (numberOfSuites < 0) {
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
            midp_jstring_from_pcsl_string(&pSuites[i], tempStringObj);
            KNI_SetObjectArrayElement(suites, (jint)i, tempStringObj);
        }
    } while (0);

    midpFreeSuiteIDs(pSuites, numberOfSuites);
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method int getNumberOfSuites() for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Get the number of installed of MIDlet suites.
 *
 * @return the number of installed suites
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_getNumberOfSuites() {
    pcsl_string filename;
    char* pszError;
    int numberOfSuites;
    int rc;

    rc = pcsl_string_cat(storage_get_root(), &SUITE_LIST_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
        KNI_ReturnInt(0);
    }

    numberOfSuites = (jint)getNumberOfStrings(&pszError, &filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        numberOfSuites = 0;
    }

    pcsl_string_free(&filename);
    KNI_ReturnInt(numberOfSuites);
}

#if VERIFY_ONCE
/**
 * Read suite hash value and fill the verifyHash field of
 * the com.sun.midp.midletsuite.InstallInfo object with it
 *
 * @param suiteID  ID of the suite
 * @param installInfoObj object to fill
 *
 * @throws OutOfMemoryError if not enough memory to read/fill information
 * @throws IOException if the information cannot be read
 */
static int fillVerifyHash(const pcsl_string * suiteID, jobject installInfoObj) {
    int len;
    jbyte * data = NULL;
    jfieldID fieldID;
    int status;

    KNI_StartHandles(2);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(field);

    status = readVerifyHash(suiteID, &data, &len);
    if (status == OUT_OF_MEM_LEN) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == IO_ERROR_LEN) {
        KNI_ThrowNew(midpIOException, NULL);
    } else {
        SNI_NewArray(SNI_BYTE_ARRAY, len, field);
        if (KNI_IsNullHandle(field)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_GetObjectClass(installInfoObj, clazz);
            fieldID = midp_get_field_id(clazz, "verifyHash", "[B");
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
Java_com_sun_midp_midletsuite_InstallInfo_load() {
    MidpInstallInfo installInfo;
    char* pszError;
    jfieldID idFid;

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(temp);
    KNI_DeclareHandle(suiteIDStr);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);
    idFid = midp_get_field_id(clazz, "id", "Ljava/lang/String;");
    KNI_GetObjectField(thisObj, idFid, suiteIDStr);

    GET_JSTRING_AS_PCSL_STRING(suiteIDStr, suiteID_str)
        installInfo = read_install_info(&pszError, &suiteID_str);
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

            fillAuthPath(installInfo, thisObj, clazz);

#if VERIFY_ONCE
            fillVerifyHash(&suiteID_str, thisObj);
#endif /* VERIFY_ONCE */

            midpFreeInstallInfo(installInfo);
        }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * native void load();
 *
 * Gets the suite settings suite from persistent store.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_SuiteSettings_load() {
    pcsl_string suiteID = PCSL_STRING_NULL;
    char* pszError;
    jboolean enabled;
    jbyte* pPermissions;
    int numberOfPermissions;
    int permissionsFieldLen;
    jbyte pushInterrupt;
    jint pushOptions;
    jfieldID permissionsFid;
    jfieldID suiteIdFid;

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(suiteIDStr);
    KNI_DeclareHandle(permissionsField);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);
    permissionsFid = midp_get_field_id(clazz, "permissions", "[B");
    suiteIdFid = midp_get_field_id(clazz, "suiteId", "Ljava/lang/String;");

    KNI_GetObjectField(thisObj, permissionsFid, permissionsField);
    permissionsFieldLen = (int)KNI_GetArrayLength(permissionsField);

    KNI_GetObjectField(thisObj, suiteIdFid, suiteIDStr);
    if(PCSL_STRING_OK != midp_jstring_to_pcsl_string(suiteIDStr, &suiteID)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else
    do {

        if (!readSettings(&pszError, &suiteID, &enabled, &pushInterrupt,
            &pushOptions, &pPermissions, &numberOfPermissions)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } 

        if (pszError != NULL) {
            storageFreeError(pszError);
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

    pcsl_string_free(&suiteID);

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * private native void save0(
 *     String suiteId,
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
Java_com_sun_midp_midletsuite_SuiteSettings_save0() {
    char* pszError;
    jbyte* pPermissions = NULL;
    int permissionsLen;
    jbyte pushInterrupt;
    jint pushOptions;
    jboolean enabled;

    KNI_StartHandles(2);
    KNI_DeclareHandle(permissionsObj);

    pushInterrupt = KNI_GetParameterAsByte(2);
    pushOptions = KNI_GetParameterAsInt(3);
    KNI_GetParameterAsObject(4, permissionsObj);

    permissionsLen = (int)KNI_GetArrayLength(permissionsObj);

    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)

    do {
        pPermissions = (jbyte*)midpMalloc(permissionsLen);
        if (pPermissions == NULL) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        KNI_GetRawArrayRegion(permissionsObj, 0, permissionsLen,
                              (jbyte*)pPermissions);


        if (!readEnabledState(&pszError, &suiteID, &enabled)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } 

        if (pszError != NULL) {
            KNI_ThrowNew(midpIOException, pszError);
            storageFreeError(pszError);
            break;
        }

        if (!writeSettings(&pszError, &suiteID, enabled, pushInterrupt,
                           pushOptions, pPermissions, permissionsLen)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        if (pszError != NULL) {
            KNI_ThrowNew(midpIOException, pszError);
            storageFreeError(pszError);
            break;
        }

    } while (0);

    midpFree(pPermissions);

    RELEASE_PCSL_STRING_PARAMETER

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
Java_com_sun_midp_midletsuite_SuiteProperties_load() {
    pcsl_string suiteID = PCSL_STRING_NULL;
    pcsl_string * const p_suiteID = &suiteID;
    jchar * key_data   = NULL;
    jint key_len       = NULL_LEN;
    jchar * value_data = NULL;
    jint value_len     = NULL_LEN;
    jint numProperties;

    int i;
    jint handle;
    int  openError = 0;
    MIDP_ERROR error = MIDP_ERROR_NONE;
    jfieldID suiteIdFid;

    KNI_StartHandles(5);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(properties);
    KNI_DeclareHandle(suiteIDStr);
    KNI_DeclareHandle(tempStringObj);

    KNI_GetThisPointer(thisObj);
    KNI_GetObjectClass(thisObj, clazz);

    suiteIdFid = midp_get_field_id(clazz, "suiteId", "Ljava/lang/String;");
    KNI_GetObjectField(thisObj, suiteIdFid, suiteIDStr);

    do {
        if (PCSL_STRING_OK !=
            midp_jstring_to_pcsl_string(suiteIDStr, &suiteID)) {
            error = MIDP_ERROR_OUT_MEM;
            break;
        }

        GET_PCSL_STRING_DATA_AND_LENGTH(p_suiteID)
        if (PCSL_STRING_PARAMETER_ERROR(p_suiteID)) {
            error = MIDP_ERROR_OUT_MEM;
        } else {
            error = midpport_suite_open_properties((jchar *)p_suiteID_data,
                                                   p_suiteID_len,
                                                   &numProperties, &handle);
        }
        RELEASE_PCSL_STRING_DATA_AND_LENGTH

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

    pcsl_string_free(&suiteID);

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

    if ((!openError) &&
	(midpport_suite_close_properties(handle) == 
	 MIDP_ERROR_AMS_SUITE_CORRUPTED) &&
	(error == MIDP_ERROR_NONE)) {
      KNI_ThrowNew(midpIOException, NULL);
    }

    KNI_EndHandlesAndReturnObject(properties);
}


/**
 * Native method void disable(String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * Disables a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param id storage name for the installed package
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_disable() {
    int status;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
        status = midpDisableSuite(&suiteID);
        if (status == MIDP_ERROR_OUT_MEM) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status == SUITE_LOCKED) {
            KNI_ThrowNew(midletsuiteLocked, NULL);
        } else if (status == IO_ERROR_LEN) {
            KNI_ThrowNew(midpIOException, NULL);
        } else if (status == MIDP_ERROR_AMS_SUITE_NOT_FOUND) {
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
        }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method void enable(String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * Enables a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param id storage name for the installed package
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_enable() {
    int status;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
        status = midpEnableSuite(&suiteID);
        if (status == MIDP_ERROR_OUT_MEM) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status == SUITE_LOCKED) {
            KNI_ThrowNew(midletsuiteLocked, NULL);
        } else if (status == IO_ERROR_LEN) {
            KNI_ThrowNew(midpIOException, NULL);
        } else if (status == MIDP_ERROR_AMS_SUITE_NOT_FOUND) {
            KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
        }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method int removeFromStorage(String) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Removes a software package given its storage name
 * <p>
 * If the component is in use it must continue to be available
 * to the other components that are using it.  The resources it
 * consumes must not be released until it is not in use.
 *
 * @param id storage name for the installed package
 *
 * @exception IllegalArgumentException if the suite cannot be found
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_remove0() {
    int status;

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
    status = midp_remove_suite(&suiteID);
    if (status == OUT_OF_MEM_LEN) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == SUITE_LOCKED) {
        KNI_ThrowNew(midletsuiteLocked, NULL);
    } else if (status == 0) {
        KNI_ThrowNew(midpIllegalArgumentException, "bad suite ID");
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#define GET_PROP_PARAM(NUM,PARAM,STRINGOBJ,PROPS,STATUS) { \
    int i; \
    int numberOfStrings; \
 \
    KNI_GetParameterAsObject(NUM, PARAM); \
 \
    numberOfStrings = (int)KNI_GetArrayLength(PARAM); \
 \
    (PROPS.pStringArr) = alloc_pcsl_string_list(numberOfStrings); \
    if ((PROPS.pStringArr) == NULL) { \
        (STATUS) = OUT_OF_MEM_LEN; \
        break; \
    } \
 \
    (PROPS.numberOfProperties) = numberOfStrings / 2; \
    for (i = 0; i < numberOfStrings; i++) { \
        KNI_GetObjectArrayElement(PARAM, (jint)i, STRINGOBJ); \
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string((STRINGOBJ), &(PROPS.pStringArr[i]))) { \
            int j; \
            for (j = 0; j < i; j++) { \
                pcsl_string_free(&(PROPS.pStringArr[j])); \
            } \
            midpFree((PROPS.pStringArr)); \
            (PROPS.numberOfProperties) = 0; \
            (STATUS) = OUT_OF_MEM_LEN; \
            break; \
        } \
    } \
 \
    if (status != 0) { \
        break; \
    } \
 \
    KNI_ReleaseHandle(STRINGOBJ); \
}

/**
 * Native method void nativeStoreSuite(...) of
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Stores or updates a suite.
 *
 * @param id unique ID of the suite
 * @param jadUrl where the JAD came from, can be null
 * @param jadProps properties the JAD
 * @param jarUrl where the JAR came from
 * @param jarFilename the downloaded JAR
 * @param manifestProps properties of the manifest
 * @param authPath if signed, the authorization path starting with the
 *        most trusted authority
 * @param domain security domain of the suite
 * @param trusted true if suite is trusted
 * @param permissions permissions for the suite
 * @param pushInterruptSetting push interrupt setting for the suite
 * @param pushOptions user options for push interrupts
 * @param verifyHash hash value of the suite with preverified classes
 *
 * @exception IOException is thrown, if an I/O error occurs during
 * storing the suite
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 * @exception IllegalArgumentException is thrown if any of input strings
 * (except jadUrl) is null or empty
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteStorage_nativeStoreSuite() {
    pcsl_string suiteID = PCSL_STRING_NULL;
    pcsl_string jadUrl = PCSL_STRING_NULL;
    MidpProperties jadProps = {0, ALL_OK, NULL};
    pcsl_string jarUrl = PCSL_STRING_NULL;
    pcsl_string jarName = PCSL_STRING_NULL;
    MidpProperties manifestProps = {0, ALL_OK, NULL};
    pcsl_string domain = PCSL_STRING_NULL;
    jboolean trusted;
    pcsl_string* pAuthPath = NULL;
    int authPathLen = 0;
    jbyte* pPermissions = NULL;
    int permissionsLen = 0;
    unsigned char pushInterruptSetting;
    long pushOptions;
    int verifyHashLen = 0;
    jbyte* pVerifyHash = NULL;
    int status = 0;

    jadProps.numberOfProperties = 0;
    manifestProps.numberOfProperties = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(tempHandle);
    KNI_DeclareHandle(tempHandle2);
    do {
        GET_NON_EMPTY_STRING_PARAM(1, tempHandle, &suiteID, status);
        GET_STRING_PARAM(2, tempHandle, &jadUrl, status);
        if (!pcsl_string_is_null(&jadUrl)) {
            GET_PROP_PARAM(3, tempHandle, tempHandle2, jadProps, status);
        }
        GET_NON_EMPTY_STRING_PARAM(4, tempHandle, &jarUrl, status);
        GET_NON_EMPTY_STRING_PARAM(5, tempHandle, &jarName, status);
        GET_PROP_PARAM(6, tempHandle, tempHandle2, manifestProps, status);
        GET_STRING_ARRAY_PARAM(7, tempHandle, pAuthPath, authPathLen,
            tempHandle2, status);
        GET_NON_EMPTY_STRING_PARAM(8, tempHandle, &domain, status);
        trusted = KNI_GetParameterAsBoolean(9);
        GET_NON_EMPTY_BYTE_ARRAY_PARAM(10, tempHandle, pPermissions,
            permissionsLen, status);
        pushInterruptSetting = KNI_GetParameterAsByte(11);
        pushOptions = KNI_GetParameterAsInt(12);

        GET_BYTE_ARRAY_PARAM(13, tempHandle, pVerifyHash,
            verifyHashLen, status);
        KNI_ReleaseHandle(tempHandle);

        status = midp_store_suite(&suiteID, &jadUrl, jadProps, &jarUrl,
                                  &jarName, manifestProps, &domain, trusted,
                                  &pAuthPath, authPathLen,
                                  (unsigned char *)pPermissions, permissionsLen,
                                  pushInterruptSetting, pushOptions,
                                  (unsigned char *)pVerifyHash, verifyHashLen);
    } while (0);

    pcsl_string_free(&suiteID);
    pcsl_string_free(&jadUrl);
    midpFreeProperties(jadProps);
    pcsl_string_free(&jarUrl);
    pcsl_string_free(&jarName);
    midpFreeProperties(manifestProps);
    free_pcsl_string_list(pAuthPath, authPathLen);
    pcsl_string_free(&domain);
    midpFree(pPermissions);
    midpFree(pVerifyHash);

    KNI_EndHandles();

    if (status == OUT_OF_MEM_LEN) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (status == IO_ERROR_LEN) {
        KNI_ThrowNew(midpIOException, NULL);
    } else if (status == SUITE_LOCKED) {
        KNI_ThrowNew(midletsuiteLocked, NULL);
    } else if (status != 0) { /* includes BAD_PARAMS */
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_ReturnVoid();
}
