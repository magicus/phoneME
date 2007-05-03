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
 * This header file contains Secure API of the MIDlet suite storage subsystem.
 */

#ifndef _SUITESTORE_SECURE_H_
#define _SUITESTORE_SECURE_H_

#include <kni.h>
#include <pcsl_string.h>
#include <midp_global_status.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter returnValue using pcsl_mem_malloc().
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
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError midp_suite_read_secure_resource(SuiteIdType suiteId,
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
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError midp_suite_write_secure_resource(SuiteIdType suiteId,
                           const pcsl_string* resourceName,
                           jbyte *value,
                           jint valueSize);

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_SECURE_H_ */
