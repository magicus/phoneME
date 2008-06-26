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
#include <pcsl_memory.h>
#include <suitestore_rms.h>
#include <javacall_nams.h>

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
 * NOTE: currently this parameter is ignored because it is up to 
 * external manager to know where RMS storage is. 
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
    javacall_utf16_string root = NULL;
    int root_len = JAVACALL_MAX_ROOT_PATH_LENGTH;
    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string rmsFileName = PCSL_STRING_NULL;
    jint suiteIdLen = GET_SUITE_ID_LEN(suiteId);
    jsize resourceNameLen = pcsl_string_length(pResourceName);
    MIDPError error = ALL_OK;

    *pFileName = PCSL_STRING_NULL;
    do {
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
                error = BAD_PARAMS;
                break;
            }
    
            /* performance hint: predict buffer capacity */
            fileNameLen = PCSL_STRING_ESCAPED_BUFFER_SIZE(resourceNameLen + extLen);
            pcsl_string_predict_size(&rmsFileName, fileNameLen);
    
            if (pcsl_string_append_escaped_ascii(&rmsFileName, pResourceName) !=
                    PCSL_STRING_OK ||
                        pcsl_string_append(&rmsFileName, ext) != PCSL_STRING_OK) {
                error = OUT_OF_MEMORY;
                break;
            }
        }

        root = (javacall_utf16_string)pcsl_mem_malloc(JAVACALL_MAX_ROOT_PATH_LENGTH);
        if (NULL == root) {
            error = OUT_OF_MEMORY;
            break;
        }
        
        if (JAVACALL_OK == javacall_ams_get_rms_path((javacall_suite_id)(suiteId),
                                                     root, &root_len)) {
            pcsl_string suite_id_str;
            /* performance hint: predict buffer capacity */
            pcsl_string_predict_size(&returnPath, root_len +
                suiteIdLen + pcsl_string_length(&rmsFileName));
            
            if (PCSL_STRING_OK != pcsl_string_convert_from_jint((jint)suiteId, &suite_id_str) ||
                PCSL_STRING_OK != pcsl_string_append_buf(&returnPath, root, root_len) ||
                    PCSL_STRING_OK != pcsl_string_append(&returnPath,&suite_id_str) ||
                    PCSL_STRING_OK != pcsl_string_append(&returnPath, &rmsFileName)) {
                error = OUT_OF_MEMORY;
                pcsl_string_free(&suite_id_str);
                pcsl_string_free(&returnPath);
                break;
            }
            *pFileName = returnPath;            
        } else {
            error = NOT_FOUND;
        }
    } while (0);

    pcsl_mem_free(root);
    pcsl_string_free(&rmsFileName);

    return error;
}
