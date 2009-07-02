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
 * This is reference implementation of the Secure API of the MIDlet suite
 * storage subsystem.
 */

#include <string.h>
#include <kni.h>
#include <midpStorage.h>
#include <pcsl_memory.h>
#include <suitestore_intern.h>

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
MIDPError
midp_suite_write_secure_resource(SuiteIdType suiteId,
                                 const pcsl_string* resourceName,
                                 jbyte *value,
                                 jint valueSize) {
    pcsl_string filename = PCSL_STRING_NULL;
    char *pszError = NULL;
    MIDPError errorCode;
    int handle;

    errorCode = get_secure_resource_file(suiteId, resourceName,
                                         &filename);
    if (errorCode != ALL_OK) {
        pcsl_string_free(&filename);
        return errorCode;
    }

    handle = storage_open(&pszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return SUITE_CORRUPTED_ERROR;
    }

    do {
        storageWrite(&pszError, handle, (char*)&valueSize, sizeof (int));
        if (pszError != NULL) break;
        storageWrite(&pszError, handle, (char*)value,
            valueSize * sizeof (jbyte));
        if (pszError != NULL) break;
    } while (0);

    if (pszError != NULL) {
        errorCode = SUITE_CORRUPTED_ERROR;
        storageFreeError(pszError);
    }

    storageClose(&pszError, handle);
    storageFreeError(pszError);

    return errorCode;
}

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that when porting memory for the in/out parameter
 * returnValue MUST be allocated using pcsl_mem_malloc().
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
MIDPError
midp_suite_read_secure_resource(SuiteIdType suiteId,
                                const pcsl_string* resourceName,
                                jbyte **returnValue,
                                jint *valueSize) {
    pcsl_string filename = PCSL_STRING_NULL;
    char *pszError = NULL;
    MIDPError errorCode;
    int bytesRead;
    int handle;

    *returnValue = NULL;
    *valueSize = 0;

    errorCode = get_secure_resource_file(suiteId, resourceName, &filename);
    if (errorCode != ALL_OK) {
        pcsl_string_free(&filename);
        return errorCode;
    }

    handle = storage_open(&pszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return SUITE_CORRUPTED_ERROR;
    }

    do {
        bytesRead = storageRead(&pszError, handle,
            (char*)valueSize, sizeof (int));
        if (bytesRead != sizeof (int) || *valueSize == 0)
            break;

        *returnValue = (jbyte*)pcsl_mem_malloc(*valueSize * sizeof (jbyte));
        if (*returnValue == NULL) {
            errorCode = OUT_OF_MEMORY;
            break;
        }

        bytesRead = storageRead(&pszError, handle, (char*)(*returnValue),
            *valueSize * sizeof (jbyte));

        if (pszError != NULL || bytesRead != *valueSize) {
            errorCode = SUITE_CORRUPTED_ERROR;
            pcsl_mem_free(*returnValue);
            *returnValue = NULL;
            break;
        }
    } while (0);

    storageClose(&pszError, handle);
    storageFreeError(pszError);

    return errorCode;
}
