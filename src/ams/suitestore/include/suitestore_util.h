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
/**
 * @file
 * @ingroup cams_extern
 *
 * @brief External (to AMS) Interface for MIDlet suite storage.
 */

#ifndef _SUITESTORE_UTIL_H_
#define _SUITESTORE_UTIL_H_

#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @name Constants for the midpport_suite_rms_filename function
 * Constants for extension definition for rms resources
 * @see midpport_suite_rms_filename
 * @{
 */
/** specifies the extension .db to the function midpport_suite_rms_filename */
#define MIDP_RMS_DG_EXT       0
/** specifies the extension .idx to the function midpport_suite_rms_filename */
#define MIDP_RMS_IDX_EXT      1

/** @} */

/**
 * Constant for the internal suite ID.
 */
extern const pcsl_string INTERNAL_SUITE_ID;

/**
 * Gets location of the class path
 * for the suite with the specified suiteId.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter classPath using midpMalloc.
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
                                     pcsl_string *classPath);

/**
 * Only for MONET--Gets the class path to binary application image
 * for the suite with the specified MIDlet suite id.
 *
 * It is different from "usual" class path in that class path points to a
 * jar file, while this binary application image path points to a MONET bundle.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter classPath using midpMalloc.
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
MIDP_ERROR midp_suite_get_bin_app_path(const pcsl_string* suiteId,
				        pcsl_string *classPath);
				      
/**
 * Gets location of the resource with specified type and name
 * for the suite with the specified suiteId.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter filename using midpMalloc.
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId           The application suite ID string
 * @param extension         rms extension that can be MIDP_RMS_DG_EXT or
 *                            MIDP_RMS_IDX_EXT
 * @param resourceName       RMS name
 * @param fileName           The in/out parameter that contains returned 
 *                             filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND, MIDP_ERROR_AMS_SUITE_CORRUPTED, 
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_suite_get_rms_filename(const pcsl_string* suiteId,
                                       jint extension,
                                       const pcsl_string* resourceName,
                                       pcsl_string* fileName);

/**
 * Gets location of the cached resource with specified name
 * for the suite with the specified suiteId.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter filename using midpMalloc.
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId            The application suite ID string
 * @param resourceName       Name of the cached resource
 * @param fileName           The in/out parameter that contains returned 
 *                            filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND, MIDP_ERROR_AMS_SUITE_CORRUPTED, 
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR
midp_suite_get_cached_resource_filename(const pcsl_string * suiteId,
                                        const pcsl_string * resourceName,
                                        pcsl_string * fileName);

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter returnValue using midpMalloc.
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
                           jint *valueSize);

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
                           jint valueSize);

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_UTIL_H_ */
