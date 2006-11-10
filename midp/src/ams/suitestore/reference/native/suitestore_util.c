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

#include <suitestore_util.h>
#include <suitestore_port.h>
#include <midpUtilKni.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>

/**
 * Gets location of the class path
 * for the suite with the specified suiteId.
 *
 * Note that when porting memory for the in/out parameter 
 * classPath MUST be allocated  using midpMalloc(). 
 * The caller is responsible for freeing the memory associated
 * with classPath.
 *
 * @param suiteId    - The application suite ID string
 * @param checkSuiteExists - true if suite should be checked for existence or not
 * @param classPath - The in/out parameter that contains returned class path
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND, MIDP_ERROR_AMS_SUITE_CORRUPTED, 
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_suite_get_class_path(const pcsl_string* suiteId,
                                     jboolean checkSuiteExists,
                                     pcsl_string *classPath) {
    MIDP_ERROR rc = MIDP_ERROR_OUT_MEM;
    pcsl_string_status perr;
    jchar* classPath_data = NULL;
    jint classPath_len = 0;

    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId) {
        rc = MIDP_ERROR_NONE;
        if (checkSuiteExists) {
            rc = midpport_suite_exists((jchar*)suiteId_data, suiteId_len);
        }
        if (rc == MIDP_ERROR_NONE) {
            rc = midpport_suite_class_path((jchar*)suiteId_data,
                                            suiteId_len,
                                            &classPath_data,
                                            &classPath_len);
        }
    } RELEASE_PCSL_STRING_DATA_AND_LENGTH

    if(0 == rc && NULL != classPath_data) {
        perr = pcsl_string_convert_from_utf16(classPath_data, classPath_len,
                                              classPath);
        if(PCSL_STRING_OK != perr) {
            rc = MIDP_ERROR_OUT_MEM;
        }
    } else {
        *classPath = PCSL_STRING_NULL;
    }
    midpFree(classPath_data);
    return rc;
}

#if ENABLE_MONET
MIDP_ERROR midp_suite_get_bin_app_path(const pcsl_string* suiteId,
				     pcsl_string *classPath) {
    MIDP_ERROR rc = MIDP_ERROR_OUT_MEM;
    pcsl_string_status perr;
    jchar* classPath_data = NULL;
    jint classPath_len = 0;

    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId) {
        rc = midpport_suite_app_image_path((jchar*)suiteId_data, suiteId_len,
                                           &classPath_data, &classPath_len);
    } RELEASE_PCSL_STRING_DATA_AND_LENGTH

    if(0 == rc && NULL != classPath_data) {
        perr = pcsl_string_convert_from_utf16(classPath_data, classPath_len,
                                              classPath);
        if(PCSL_STRING_OK != perr) {
            rc = MIDP_ERROR_OUT_MEM;
        }
    } else {
        *classPath = PCSL_STRING_NULL;
    }
    return rc;
}
#endif

/**
 * Gets location of the resource with specified type and name
 * for the suite with the specified suiteId.
 *
 * Note that when porting memory for the in/out parameter 
 * filename MUST be allocated  using midpMalloc(). 
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId    - The application suite ID string
 * @param extension - rms extension that can be IDX_EXTENSION_INDEX or
 *                    DB_EXTENSION_INDEX 
 * @param resourceName - The name of the resource file
 * @param fileName - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND,MIDP_ERROR_AMS_SUITE_CORRUPTED, 
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_suite_get_rms_filename(const pcsl_string* suiteId,
                                       jint extension,
                                       const pcsl_string* resourceName,
                                       pcsl_string* const fileName) {
    MIDP_ERROR rc = MIDP_ERROR_ILLEGAL_ARGUMENT;
    jint fileName__len = 0;
    jchar * fileName__data;
    *fileName = PCSL_STRING_NULL;
    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId)
    GET_PCSL_STRING_DATA_AND_LENGTH(resourceName) {
        rc =
            midpport_suite_rms_filename((jchar*)suiteId_data, suiteId_len,
                         extension,
                         (jchar*)resourceName_data, resourceName_len,
                         &fileName__data, &fileName__len);
         if(rc==0) {
            pcsl_string_status rc2 =
                pcsl_string_convert_from_utf16(fileName__data,
                                               fileName__len,
                                               fileName);
            midpFree(fileName__data);
            if(PCSL_STRING_OK != rc2) {
                rc = MIDP_ERROR_OUT_MEM;
            }
         }
    } RELEASE_PCSL_STRING_DATA_AND_LENGTH
    RELEASE_PCSL_STRING_DATA_AND_LENGTH;
    return rc;
}

/**
 * Gets location of the cached resource with specified name
 * for the suite with the specified suiteId.
 *
 * Note that when porting memory for the in/out parameter 
 * filename MUST be allocated  using midpMalloc(). 
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId    - The application suite ID string
 * @param resourceName - The name of the resource file
 * @param filename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND,MIDP_ERROR_AMS_SUITE_CORRUPTED, 
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR
midp_suite_get_cached_resource_filename(const pcsl_string * suiteId,
                                        const pcsl_string * resourceName,
                                        pcsl_string * fileName) {

    MIDP_ERROR res;
    jint fileName_len;
    jchar * fileName_data;

    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId)
    GET_PCSL_STRING_DATA_AND_LENGTH(resourceName)
    res = midpport_suite_cached_resource_filename((jchar *)suiteId_data,
                                                  suiteId_len,
                                                  (jchar *)resourceName_data,
                                                  resourceName_len,
                                                  &fileName_data,
                                                  &fileName_len);
    if (PCSL_STRING_OK !=
        pcsl_string_convert_from_utf16(fileName_data, fileName_len, fileName)) {
        if (MIDP_ERROR_NONE == res) {
            res = MIDP_ERROR_OUT_MEM;
        }
    }
    midpFree(fileName_data);
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    return res;
}


/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that when porting memory for the in/out parameter
 * returnValue MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param returnValue       The in/out parameter that will return the
 *                          value part of the requested secure resource
 *                          (NULL is a valid return value)
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_suite_read_secure_resource(const pcsl_string* suiteId,
                           const pcsl_string* resourceName,
                           jbyte **returnValue,
                           jint *valueSize) {
    MIDP_ERROR res;
    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId)
    GET_PCSL_STRING_DATA_AND_LENGTH(resourceName)
        if ((NULL == suiteId_data && 0 != suiteId_len)
         || (NULL == resourceName_data && 0 != resourceName_len)) {
            res = MIDP_ERROR_OUT_MEM;
        } else {
            res = midpport_suite_read_secure_resource(
                            (jchar*)suiteId_data,
                            suiteId_len,
                            (jchar*)resourceName_data,
                            resourceName_len,
                            returnValue,
                            valueSize);
        }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    return res;
}

/**
 * Writes named secure resource of the suite with specified suiteId
 * to secure persistent storage.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param value             The value part of the secure resource to be stored
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_suite_write_secure_resource(const pcsl_string* suiteId,
                           const pcsl_string* resourceName,
                           jbyte *value,
                           jint valueSize) {
    MIDP_ERROR res;
    GET_PCSL_STRING_DATA_AND_LENGTH(suiteId)
    GET_PCSL_STRING_DATA_AND_LENGTH(resourceName)
        if ((NULL == suiteId_data && 0 != suiteId_len)
         || (NULL == resourceName_data && 0 != resourceName_len)) {
            res = MIDP_ERROR_OUT_MEM;
        } else {
            res = midpport_suite_write_secure_resource(
                        (jchar*)suiteId_data,
                        suiteId_len,
                        (jchar*)resourceName_data,
                        resourceName_len,
                        value,
                        valueSize);
        }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    return res;
}
