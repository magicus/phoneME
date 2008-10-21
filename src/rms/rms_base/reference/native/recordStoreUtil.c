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

#include <kni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midpMalloc.h>
#include <midpRMS.h>
#include <midpUtilKni.h>
#include <midpError.h>

/**
 * @file
 *
 * Implementation for RMS Util native methods.
 */

/**
 * Looks to see if the storage file for record store
 * identified by <code>uidPath</code> exists
 *
 * @param suiteId ID of the MIDlet suite that owns the record store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return true if the file exists, false if it does not.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_rms_RecordStoreUtil_exists) {
    int extension = KNI_GetParameterAsInt(3);
    jboolean exists = KNI_FALSE;
    int status;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(1, filenameBase)
    GET_PARAMETER_AS_PCSL_STRING(2, name_str) {
        status = rmsdb_record_store_exists(&filenameBase, &name_str, extension);
        if (status == OUT_OF_MEM_LEN) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status > 0) {
            exists = KNI_TRUE;
        }
    } RELEASE_PCSL_STRING_PARAMETER;
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();

    KNI_ReturnBoolean(exists);
}

/**
 * Removes the storage file for record store <code>filename</code>
 * if it exists.
 *
 * @param suiteId ID of the MIDlet suite that owns the record store
 * @param name name of the record store
 * @param extension extension number to add to the end of the file name
 *
 * @return true if successful or false if an IOException occurred
 *         internally.
 */

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_rms_RecordStoreUtil_deleteFile) {
    int extension = KNI_GetParameterAsInt(3);
    jboolean existed = KNI_FALSE;
    int status;
    char* pszError;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(1, filenameBase)
    GET_PARAMETER_AS_PCSL_STRING(2, name_str) {
        status = rmsdb_record_store_delete(&pszError, &filenameBase, 
                                           &name_str, extension);

        switch (status) {
            case  0 : // Identifies IOException which is not allowed in
                      // contexts this utility method is called from.
                      // Fall through to RecordStoreException.

            case -1 : KNI_ThrowNew(midpRecordStoreException, pszError);
                      recordStoreFreeError(pszError);
                      break;

            case -2 : KNI_ThrowNew(midpOutOfMemoryError, NULL);
                      break;

            default : existed = KNI_TRUE;
        }
    } RELEASE_PCSL_STRING_PARAMETER;
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();

    KNI_ReturnBoolean(existed);
}

