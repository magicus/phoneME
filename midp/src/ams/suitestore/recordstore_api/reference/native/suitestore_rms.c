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
 * This is reference implementation of the RMS API of the MIDlet suite
 * storage subsystem.
 */

#include <kni.h>
#include <string.h>
#include <midpInit.h>
#include <midpStorage.h>
#include <midpRMS.h>
#include <midpUtilKni.h>
#include <pcsl_memory.h>
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <suitestore_intern.h>
#include <suitestore_rms.h>

/* ------------------------------------------------------------ */
/*                           Public API                         */
/* ------------------------------------------------------------ */

/**
 * Gets location of the resource with specified type and name
 * for the suite with the specified suiteId.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter filename using pcsl_mem_malloc().
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID where the RMS will be located
 * NOTE: currently this parameter is ignored due to limitation of our
 * implementation: RMS is always located at the same storage as the suite.
 * @param extension rms extension that can be MIDP_RMS_DB_EXT or
 * MIDP_RMS_IDX_EXT
 * @param pResourceName RMS name
 * @param pFileName The in/out parameter that contains returned filename
 *
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError
midp_suite_get_rms_filename(SuiteIdType suiteId,
                            StorageIdType storageId,
                            jint extension,
                            const pcsl_string* pResourceName,
                            pcsl_string* pFileName) {
    const pcsl_string* root;
    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string rmsFileName = PCSL_STRING_NULL;
    jint suiteIdLen = GET_SUITE_ID_LEN(suiteId);
    jsize resourceNameLen = pcsl_string_length(pResourceName);

    /*
     * IMPL_NOTE: currently we have a limitation that the suite's RMS
     * must be located at the same storage as the midlet suite.
     * See rms.c, rmsdb_get_record_store_space_available() for more details.
     */
    root = storage_get_root(storageId);

    *pFileName = PCSL_STRING_NULL;

    if (resourceNameLen > 0) {
        const pcsl_string* ext;
        jsize extLen;
        int fileNameLen;

        if (MIDP_RMS_IDX_EXT == extension) {
            ext = &IDX_EXTENSION;
            extLen = pcsl_string_length(&IDX_EXTENSION);
        } else if (MIDP_RMS_DB_EXT == extension) {
            ext = &DB_EXTENSION;
            extLen = pcsl_string_length(&DB_EXTENSION);
        } else {
            return BAD_PARAMS;
        }

        /* performance hint: predict buffer capacity */
        fileNameLen = PCSL_STRING_ESCAPED_BUFFER_SIZE(resourceNameLen + extLen);
        pcsl_string_predict_size(&rmsFileName, fileNameLen);

        if (pcsl_esc_attach_string(pResourceName, &rmsFileName) !=
                PCSL_STRING_OK ||
                    pcsl_string_append(&rmsFileName, ext) != PCSL_STRING_OK) {
            pcsl_string_free(&rmsFileName);
            return OUT_OF_MEMORY;
        }
    }

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&returnPath, pcsl_string_length(root) +
        suiteIdLen + pcsl_string_length(&rmsFileName));

    if (PCSL_STRING_OK != pcsl_string_append(&returnPath, root) ||
            PCSL_STRING_OK != pcsl_string_append(&returnPath,
                midp_suiteid2pcsl_string(suiteId)) ||
            PCSL_STRING_OK != pcsl_string_append(&returnPath, &rmsFileName)) {
        pcsl_string_free(&rmsFileName);
        pcsl_string_free(&returnPath);
        return OUT_OF_MEMORY;
    }

    pcsl_string_free(&rmsFileName);

    *pFileName = returnPath;

    return ALL_OK;
}
