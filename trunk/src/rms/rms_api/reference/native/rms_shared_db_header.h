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
#include <pcsl_esc.h>
#include <pcsl_string.h>

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

    /** 
     * DB header version, gets incremented by 1 every time 
     * someone sets the new data
     */
    jint headerVersion;

    /** DB header data */
    jbyte* headerData;

    /** DB header data size in jbytes */
    jint headerDataSize;

    /** 
     * How many RecordStoreSharedDBHeader instances reference this native data 
     */
    int refCount;

    /** Next node in list */ 
    struct RecordStoreSharedDBHeaderListStruct* next;
} RecordStoreSharedDBHeaderList;

/**
 * Finds header node in list by ID.
 *
 * @param lookupId lookup ID
 * @return header node that has specified ID, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_id(int lookupId);

/**
 * Finds header node in list by suite ID and record store name.
 *
 * @param suiteId suite ID
 * @param storeName recordStore name
 * @return header node that has specified name, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_name(int suiteId, 
        const pcsl_string* storeName);

/**
 * Creates header node associated with specified suite ID and 
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
 * Deletes header node and removes it from the list.
 *
 * @param node pointer to header node to delete
 */
void rmsdb_delete_header_node(RecordStoreSharedDBHeaderList* node);

/**
 * Sets header data for the specified header node and 
 * increments header version by 1.
 *
 * @param node pointer to header node 
 * @param srcBuffer header data to set 
 * @param srcSize src buffer size, in jbytes (safety measure)
 *
 * @return new header version
 */
int rmsdb_set_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* srcBuffer, jint srcSize);

/**
 * Gets header data from the specified header node.
 * It only copies the data into dst buffer if the version 
 * of data stored in node is greater than the specified version.
 *
 * @param node pointer to header node 
 * @param dstBuffer where to copy header data
 * @param dstSize dst buffer size, in jbytes (safety measure)
 * @param headerVersion version of the header to check against
 *
 * @return actual header version
 */
int rmsdb_get_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* dstBuffer, jint dstSize, jint headerVersion);

/**
 * Increases header node ref count.
 *
 * @param node data node to increase ref count for
 */
void rmsdb_inc_header_node_refcount(RecordStoreSharedDBHeaderList* node);

/**
 * Gets header node ref count.
 *
 * @param node data node to increase ref count for
 * @return node ref count
 */
int rmsdb_get_header_node_refcount(RecordStoreSharedDBHeaderList* node);

/**
 * Decreases header node ref count.
 *
 * @param node data node to decrease ref count for
 */
void rmsdb_dec_header_node_refcount(RecordStoreSharedDBHeaderList* node);

#ifdef __cplusplus
}
#endif

#endif /* RMS_SHARED_DB_HEADER */
