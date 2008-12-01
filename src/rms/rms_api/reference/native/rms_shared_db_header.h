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

#ifndef RMS_SHARED_DB_HEADER
#define RMS_SHARED_DB_HEADER

#include <jvmconfig.h>
#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Native data associated with RecordStoreSharedDBHeader class. 
 * Also, the list.
 */
typedef struct RecordStoreSharedDBHeaderListStruct {
    /** Lookup Id */
    jint lookupId;

    /** Suite Id */
    jint suiteId;

    /** Record store name */
    pcsl_string storeName;

    /** 1, if header has been set at least once, 0 otherwise */
    int isHeaderDataSet;

    /** DB header data */
    jbyte* headerData;

    /** DB header data size in jbytes */
    jint headerDataSize;

    /** 
     * How many RecordStoreSharedDBHeader instances reference
     * this native data 
     * */
    int refCount;

    /** Next node in list */ 
    struct RecordStoreSharedDBHeaderListStruct* next;
} RecordStoreSharedDBHeaderList;

/**
 * Finds header data struct in list by ID.
 *
 * @param lookupId lookup ID
 * @return header data struct that has specified ID, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_id(int lookupId);

/**
 * Finds header data struct in list by suite ID and record store name.
 *
 * @param suiteId suite ID
 * @param storeName recordStore name
 * @return header data struct that has specified name, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_name(int suiteId, 
        const pcsl_string* storeName);

/**
 * Creates header data struct associated with specified suite ID and 
 * record store name, and puts it into list.
 *
 * @param suiteId suite ID
 * @param storeName recordStore name
 * @param headerDataSize size of header data in jbytes
 * @return pointer to created data, NULL if OOM 
 */
RecordStoreSharedDBHeaderList* rmsdb_create_header_node(int suiteId, 
        const pcsl_string* storeName, jint headerDataSize);

/**
 * Deletes header data struct and removes it from the list.
 *
 * @param node pointer to header data struct to delete
 */
void rmsdb_delete_header_node(RecordStoreSharedDBHeaderList* node);

/**
 * Sets header data for the specified header data struct.
 *
 * @param node pointer to header data struct 
 * @param headerData header data to set from
 */
void rmsdb_set_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* headerData);

/**
 * Gets header data from the specified header data struct.
 *
 * @param node pointer to header data struct 
 * @param headerData where to copy header data 
 */
void rmsdb_get_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* headerData);

/**
 * Increases header data struct ref count.
 *
 * @param node data struct to increase ref count for
 */
void rmsdb_inc_header_node_refcount(RecordStoreSharedDBHeaderList* node);

/**
 * Decreases header data struct ref count.
 *
 * @param node data struct to decrease ref count for
 */
void rmsdb_dec_header_node_refcount(RecordStoreSharedDBHeaderList* node);

#ifdef __cplusplus
}
#endif

#endif /* RMS_SHARED_DB_HEADER */
