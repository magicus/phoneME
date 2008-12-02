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
#include <midpMalloc.h>
#include <midpError.h>
#include <midp_thread.h>
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midp_logging.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rms_shared_db_header.h"

/** For assigning lookup ID. Incremented after new list element is created. */
static jint gsLookupId = 0;

/** Head of the list */
static RecordStoreSharedDBHeaderList* gsHeaderListHead = NULL;

/**
 * Finds header node in list by ID.
 *
 * @param lookupId lookup ID
 * @return header node that has specified ID, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_id(int lookupId) {
    RecordStoreSharedDBHeaderList* node = gsHeaderListHead;
    for (; node != NULL; node = node->next) {
        if (node->lookupId == lookupId) {
            break;
        }
    }

    return node;    
}

/**
 * Finds header node in list by suite ID and record store name.
 *
 * @param suiteId suite ID
 * @param storeName recordStore name
 * @return header node that has specified name, NULL if not found
 */
RecordStoreSharedDBHeaderList* rmsdb_find_header_node_by_name(int suiteId, 
        const pcsl_string* storeName) {

    RecordStoreSharedDBHeaderList* node = gsHeaderListHead;
    for (; node != NULL; node = node->next) {
        if (node->suiteId == suiteId && pcsl_string_equals(
                    &(node->storeName), storeName)) {
            break;
        }
    }

    return node;
}

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
        const pcsl_string* storeName, jint headerDataSize) {

    RecordStoreSharedDBHeaderList* node; 

    node = (RecordStoreSharedDBHeaderList*)midpMalloc(
            sizeof(RecordStoreSharedDBHeaderList));
    if (node == NULL) {
        return NULL;
    }

    node->lookupId = gsLookupId++;
    node->headerDataSize = headerDataSize;
    node->headerVersion = 0;
    node->suiteId = suiteId;    
    node->refCount = 0;

    node->headerData = midpMalloc(headerDataSize * sizeof(jbyte));
    if (node->headerData == NULL) {
        midpFree(node);
        return NULL;        
    }

    pcsl_string_dup(storeName, &node->storeName);
    if (pcsl_string_is_null(&(node->storeName))) {
        midpFree(node->headerData);
        midpFree(node);
        return NULL;
    }

    node->next = gsHeaderListHead;
    gsHeaderListHead = node;

    return node;    
}

/**
 * Deletes header node and removes it from the list.
 *
 * @param node pointer to header node to delete
 */
void rmsdb_delete_header_node(RecordStoreSharedDBHeaderList* node) {
    RecordStoreSharedDBHeaderList* prevNode;

    /* If it's first node, re-assign head pointer */
    if (node == gsHeaderListHead) {
        gsHeaderListHead = node->next;
    } else {
        prevNode = gsHeaderListHead;
        for (; prevNode->next != NULL; prevNode = prevNode->next) {
            if (prevNode->next == node) {
                break;
            }
        }
        
        prevNode->next = node->next;
    }

    midpFree(node->headerData);
    pcsl_string_free(&(node->storeName));

    midpFree(node);
}

/**
 * Sets header data for the specified header node.
 *
 * @param node pointer to header node 
 * @param srcHeaderData header data to set from
 * @param srcOffset offset int srcheaderData
 * @param srcSize size of the data to set, in jbytes
 *
 * @return actual header version
 */
int rmsdb_set_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* srcHeaderData, jint srcOffset, jint srcSize) {
    
    jbyte* dst;    
    jbyte* src;
    int size;

    size = (srcOffset + srcSize > node->headerDataSize)? 
        node->headerDataSize - srcOffset : srcSize;
    dst = node->headerData + srcOffset * sizeof(jbyte);
    src = srcHeaderData + srcOffset * sizeof(jbyte);

    if (size > 0) {
        memcpy(dst, src, size * sizeof(jbyte));
        node->headerVersion++;        
    }

    return node->headerVersion;
}

/**
 * Gets header data from the specified header node.
 * It only sets the pointer to the node's data if the version 
 * of data stored in node is greater than the specified version.
 *
 * @param node pointer to header node 
 * @param dstHeaderData where to store pointer to the header data
 * @param headerVersion version of the header to check against
 *
 * @return actual header version
 */
int rmsdb_get_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte** dstHeaderData, jint headerVersion) {

    if (node->headerVersion > headerVersion) {
        *dstHeaderData = node->headerData;
        return node->headerVersion;
    }

    return headerVersion;
}

/**
 * Increases header node ref count.
 *
 * @param node data node to increase ref count for
 */
void rmsdb_inc_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    node->refCount++;
}

/**
 * Decreases header node ref count.
 *
 * @param node data node to decrease ref count for
 */
void rmsdb_dec_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    node->refCount++;

    if (node->refCount <= 0) {
        rmsdb_delete_header_node(node);
    }
}

