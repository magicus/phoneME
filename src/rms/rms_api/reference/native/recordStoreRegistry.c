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
 * Sends notification event about changing of record store
 * The processing of this event should cause invocation of all
 * registered listeners of the record store, listeners can be
 * registered in a different execution contexts and should be
 * invoked within their contexts accordingly.
 *
 * @param suiteId suite ID
 * @param storeName name of the record store has been changed
 * @param changeType type of record change (added, deleted, changed)
 * @param recordId ID of the changed record
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
 * Register caller as listener of changes in record store to
 * deliver later inter-task events on record store changes
 * @param suiteId suite ID
 * @param storeName name of the record store has been changed
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
 * Unregister caller from list of listeners of record store changes
 * @param suiteId suite ID
 * @param storeName name of the record store has been changed
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
