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
#include <rms_registry.h>

/**
 * Sends asynchronous notification about change of record store done
 * in the current execution context of method caller
 *
 * @param token security token to restrict usage of the method
 * @param suiteId suite ID of changed record store
 * @param storeName name of changed record store
 * @param changeType type of record change: ADDED, DELETED or CHANGED
 * @param recordId ID of changed record
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_notifyRecordStoreChange) {

    int suiteId = KNI_GetParameterAsInt(1);
    int changeType = KNI_GetParameterAsInt(3);
    int recordId = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {

        rms_registry_notify_record_store_change(
            suiteId, &storeName, changeType, recordId);
    }
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Starts listening of asynchronous changes of record store
 *
 * @param token security token to restrict usage of the method
 * @param suiteId suite ID of record store to start listen for
 * @param storeName name of record store to start listen for
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_startRecordStoreListening) {

    int suiteId = KNI_GetParameterAsInt(1);
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {
        rms_registry_start_record_store_listening(
            suiteId, &storeName);
    }
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Stops listening of asynchronous changes of record store
 *
 * @param token security token to restrict usage of the method
 * @param suiteId suite ID of record store to stop listen for
 * @param storeName name of record store to stop listen for
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_rms_RecordStoreRegistry_stopRecordStoreListening) {

    int suiteId = KNI_GetParameterAsInt(1);
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName) {
        rms_registry_start_record_store_listening(
            suiteId, &storeName);
    }
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnVoid();
}
