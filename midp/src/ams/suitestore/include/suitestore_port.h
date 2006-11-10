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
 * @ingroup nams_port
 * @brief MIDlet suite storage porting interface.
 * This header file is the porting interface to the MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_PORT_H_
#define _SUITESTORE_PORT_H_

#include <kni.h>
#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Checks if the MIDlet suite is trusted or not.
 * 
 * @param suiteId The ID used to identify the MIDlet suite
 * @param suiteId   The length of the suiteId 
 * @return true if the application is trusted and false otherwise
 */
jboolean midpport_suite_is_trusted(jchar *suiteId, jint suiteIdLen);


/**
 * Gets the unique identifier of MIDlet suite.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param vendorLen length of the vendor string
 * @param suiteName name of the suite, as given in a JAD file
 * @param suiteNameLen length of the suiteName string
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_get_id(jchar *vendor, jint vendorLen,
				 jchar *suiteName, jint suiteNameLen,
				 jchar **suiteId, jint *suiteIdLen);

/**
 * Gets the handle for accessing MIDlet suite properties.
 *
 * @param suiteId   The suite id of the MIDlet suite 
 * @param suiteId   The length of the suiteId 
 * @param numProperties The number of properties 
 * @param propHadle The property handle for accessing 
 *                  MIDlet suite properties
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED, 
 * </pre>
 */
MIDP_ERROR midpport_suite_open_properties(jchar *suiteId, jint suiteIdLen,
					  jint *numProperties,
					  jint *propHandle);

/**
 * Retrieves the next MIDlet suite property associated with the passed in
 * property handle. Note that the implementation of this function
 * MUST allocate the memoryfor in/out parameters key and property
 * using midpMalloc().
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
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_SUITE_CORRUPTED, 
 * </pre>
 */
MIDP_ERROR midpport_suite_next_property(jint propHandle, jchar **key, 
					jint *keyLength,
					jchar **value, jint *valueLength);
  
/**
 * Closes the passed in MIDlet suite property handle.
 * 
 * @param propHandle   The MIDlet suite property handle
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_SUITE_CORRUPTED 
 * </pre>
 */
MIDP_ERROR midpport_suite_close_properties(jint propHandle);


/**
 * Determines if suite exists or not.
 * 
 * Note that the implementation of this function MUST allocate the memory for
 * the in/out parameter path using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId    The ID used to identify the MIDlet suite
 * @param suiteId    The length of the suiteId 
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_exists(jchar *suiteId, jint suiteLen);


/**
 * Gets the classpath for the specified MIDlet suite ID.
 * 
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter classPath using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId        The ID used to identify the MIDlet suite
 * @param suiteIdLen     The length of the suiteId 
 * @param classPath      The in/out parameter that contains returned class path
 * @param classPathLen   The in/out parameter that contains returned class path
 *                       length
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_class_path(jchar *suiteId, jint suiteIdLen,
				     jchar **classPath, jint *classPathLength);

#if ENABLE_MONET
/**
 * Only for MONET--Gets the class path to binary application image for the specified MIDlet suite id.
 * It is different from "usual" class path in that class path points to
 * a jar file, while this binary application image path points to MONET bundle.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter classPath using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId        The suite id used to identify the MIDlet suite
 * @param suiteIdLen     The length of the suiteId
 * @param classPath      The in/out parameter that contains returned class path
 * @param classPathLen   The in/out parameter that contains returned class path
 *                       length
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_app_image_path( jchar*  suiteId,
                                          jint    suiteIdLen,
                                          jchar** classPath, 
                                          jint*   classPathLength);
#endif //ENABLE_MONET

/**
 * Gets the path for the specified rms for the MIDlet suite by ID.
 * 
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter path using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId     The ID used to identify the MIDlet suite
 * @param suiteId     The length of the suiteId 
 * @param name        The name of the cached resource
 * @param nameLen     The length of the cached resource name
 * @param path        The in/out parameter for the path
 * @param pathLength  The in/out pathLength for the length of the path
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_cached_resource_filename(jchar *suiteId, 
						   jint suiteIdLen,
						   jchar *name, jint nameLen,
						   jchar **path, 
						   jint *pathLength);

/**
 * Gets the cached resource path for the MIDlet suite by ID.
 * 
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter path using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId    The ID used to identify the MIDlet suite
 * @param suiteId    The length of the suiteId 
 * @param extension  rms extension that can be MIDP_RMS_DG_EXT or
 *                            MIDP_RMS_IDX_EXT
 * @param name       The name of the cached resource
 * @param nameLen    The length of the cached resource name
 * @param path       The in/out parameter for the path
 * @param pathLength The in/out pathLength for the length of the path
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_rms_filename(jchar *suiteId, jint suiteIdLen,
				       jint extension,
				       jchar *name, jint nameLen,
				       jchar **path, jint *pathLength);

/**
 * Called to reset MIDlet suite storage state. 
 * This function will be called when the Java platform system is stopped. 
 * Implementation should clean up all its internal states and 
 * be ready to serve calls when Java platform system is restarted within 
 * the same OS process.
 */
void midpport_suite_storage_reset();

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter returnValue using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The ID used to identify the MIDlet suite
 * @param suiteIdLen        The length of the suiteId
 * @param resourceName      The name of secure resource to read from storage
 * @param resourceNameLen   The length of secure resource name
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
MIDP_ERROR midpport_suite_read_secure_resource(jchar *suiteId, jint suiteIdLen,
                       jchar *resourceName, jint resourceNameLen,
                       jbyte **returnValue, jint *valueSize);

/**
 * Writes named secure resource of the suite with specified suiteId
 * to secure persistent storage.
 *
 * @param suiteId           The ID used to identify the MIDlet suite
 * @param suiteIdLen        The length of the suiteId
 * @param resourceName      The name of secure resource to read from storage
 * @param resourceNameLen   The length of secure resource name
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
MIDP_ERROR midpport_suite_write_secure_resource(jchar *suiteId, jint suiteIdLen,
                       jchar *resourceName, jint resourceNameLen,
                       jbyte *value, jint valueSize);

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_PORT_H_ */
