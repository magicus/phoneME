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
#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <midp_logging.h>

#include <string.h>

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
    RecordStoreSharedDBHeaderList* thisNode;

    /** Safety check */
    if (node == NULL) {
        return;
    }

    /** Make sure that node to delete is in list */
    thisNode =  rmsdb_find_header_node_by_id(node->lookupId);
    if (node != thisNode) {
        /** Oops, node is not in list, don't try to remove it */
        return;
    }

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
 * @param srcBuffer header data to set from
 * @param srcSize size of the data to set, in jbytes (safety measure)
 *
 * @return actual header version
 */
int rmsdb_set_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* srcBuffer, jint srcSize) {
    
    int size;
    
    /** Safety checks */    

    if (node == NULL || srcBuffer == NULL) {
        return 0;
    }

    size = srcSize;
    if (size > node->headerDataSize) {
        size = node->headerDataSize;
    }

    if (node->headerData != NULL) {
        memcpy(node->headerData, srcBuffer, size * sizeof(jbyte));
        node->headerVersion++;
    }

    return node->headerVersion;
}

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
        jbyte* dstBuffer, jint dstSize, jint headerVersion) {

    int size;

    /** Safety checks */

    if (node == NULL || dstBuffer == NULL) {
        return headerVersion;
    }    

    size = node->headerDataSize;
    if (size > dstSize) {
        size = dstSize;
    }

    if (node->headerVersion > headerVersion && node->headerData != NULL) {
        memcpy(dstBuffer, node->headerData, size * sizeof(jbyte));
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
    /** Safety check */    
    if (node == NULL) {
        return;
    }

    node->refCount++;
}

/**
 * Gets header node ref count.
 *
 * @param node data node to increase ref count for
 * @return node ref count
 */
int rmsdb_get_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    /** Safety check */    
    if (node == NULL) {
        return 0;
    }    
    return node->refCount;
}

/**
 * Decreases header node ref count.
 *
 * @param node data node to decrease ref count for
 */
void rmsdb_dec_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    /** Safety check */    
    if (node == NULL) {
        return;
    }

    node->refCount--;

    if (node->refCount <= 0) {
        rmsdb_delete_header_node(node);
    }
}

