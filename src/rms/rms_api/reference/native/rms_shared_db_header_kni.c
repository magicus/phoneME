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

#include <jvmconfig.h>
#include <kni.h>
#include <kni_globals.h>
#include <jvmspi.h>
#include <sni.h>
#include <midpMalloc.h>
#include <midpError.h>
#include <midp_thread.h>
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midpUtilKni.h>
#include <commonKNIMacros.h>
#include <string.h>

#include "rms_shared_db_header.h"

/** For MIDP on CDC */
#ifndef SNI_BEGIN_RAW_POINTERS
#define SNI_BEGIN_RAW_POINTERS
#endif

#ifndef SNI_END_RAW_POINTERS
#define SNI_END_RAW_POINTERS
#endif

/** Cached field ID to be used in finalizer. */
// static jfieldID gsLookupIdField = 0;

/**
 * Gets lookup ID for specified suite ID and record store name. 
 * If there is no native data in the list associated with these 
 * ID and name, it will be created and added to the list.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_rms_RecordStoreSharedDBHeader_getLookupId0) {
    RecordStoreSharedDBHeaderList* node = NULL;
    jint lookupId = -1;
    jint suiteId = -1;
    jint dataHeaderSize = 0;

    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_PCSL_STRING(2, storeName)
    dataHeaderSize = KNI_GetParameterAsInt(3);        

    node = rmsdb_find_header_node_by_name(suiteId, &storeName);
    if (node == NULL) {
        node = rmsdb_create_header_node(suiteId, &storeName, dataHeaderSize);
    }
    
    if (node == NULL) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        rmsdb_inc_header_node_refcount(node);
        lookupId = node->lookupId;
    }

    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnInt(lookupId);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_rms_RecordStoreSharedDBHeader_headerUpdated0) {
    RecordStoreSharedDBHeaderList* node = NULL;
    jint lookupId = -1;
    jint offset = 0;
    jint size = 0;
    jint headerVersion = 0;
    jbyte* headerData = NULL;

    KNI_StartHandles(1); 
    KNI_DeclareHandle(headerDataJavaArray);

    lookupId = KNI_GetParameterAsInt(1);
    offset = KNI_GetParameterAsInt(3);
    size = KNI_GetParameterAsInt(4); 

    node = rmsdb_find_header_node_by_id(lookupId);

    if (node == NULL) {
        KNI_ThrowNew(midpIllegalStateException, 
                "Invalid header node lookup ID");
    } else {
        KNI_GetParameterAsObject(2, headerDataJavaArray);
        if (KNI_IsNullHandle(headerDataJavaArray)) {
            KNI_ThrowNew(midpIllegalArgumentException, 
                    "Header data array is null");
        } else {
            headerData = JavaByteArray(headerDataJavaArray);
        SNI_BEGIN_RAW_POINTERS            
            headerVersion = rmsdb_set_header_data(node, headerData, 
                    offset, size);
        SNI_END_RAW_POINTERS
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(headerVersion);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_rms_RecordStoreSharedDBHeader_getHeaderData0) {
    RecordStoreSharedDBHeaderList* node = NULL;
    jint lookupId = -1;
    jint headerVersion = 0;    
    jbyte* headerData = NULL;

    KNI_StartHandles(1); 
    KNI_DeclareHandle(headerDataJavaArray);

    lookupId = KNI_GetParameterAsInt(1);
    node = rmsdb_find_header_node_by_id(lookupId);

    if (node == NULL) {
        KNI_ThrowNew(midpIllegalStateException, 
                "Invalid header node lookup ID");
    } else {
        KNI_GetParameterAsObject(2, headerDataJavaArray);
        if (KNI_IsNullHandle(headerDataJavaArray)) {
            KNI_ThrowNew(midpIllegalArgumentException, 
                    "Header data array is null");
        } else {
            headerVersion = rmsdb_get_header_data(node, &headerData, 
                    headerVersion);
            if (headerData != NULL) {
            SNI_BEGIN_RAW_POINTERS                
                memcpy(JavaByteArray(headerDataJavaArray), headerData, 
                        node->headerDataSize * sizeof(jbyte));
            SNI_END_RAW_POINTERS
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(headerVersion);
}

