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

#include "rms_shared_db_header.h"

/** For assigning lookup ID. Incremented after new list element is created. */
static jint gsLookupId = 0;

/** Head of the list */
static RecordStoreSharedDBHeaderList* gsHeaderListHead = NULL;

/**
 * Finds header data struct in list by ID.
 *
 * @param lookupId lookup ID
 * @return header data struct that has specified ID, NULL if not found
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
 * Finds header data struct in list by suite ID and record store name.
 *
 * @param suiteId suite ID
 * @param storeName recordStore name
 * @return header data struct that has specified name, NULL if not found
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
 * Creates header data struct associated with specified suite ID and 
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
    node->isHeaderDataSet = 0;
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
 * Deletes header data struct and removes it from the list.
 *
 * @param node pointer to header data struct to delete
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
 * Sets header data for the specified header data struct.
 *
 * @param node pointer to header data struct 
 * @param headerData header data to set
 */
void rmsdb_set_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* headerData) {        

    memcpy(node->headerData, headerData, node->headerDataSize * sizeof(jbyte));
    node->isHeaderDataSet = 1;
}

/**
 * Gets header data from the specified header data struct.
 *
 * @param node pointer to header data struct 
 * @param headerData header data to set into
 */
void rmsdb_get_header_data(RecordStoreSharedDBHeaderList* node, 
        jbyte* headerData) {

    /**
     * No one has set header data yet, so nothing to get.
     */
    if (!node->isHeaderDataSet) {
        return;
    }

    memcpy(headerData, node->headerData, node->headerDataSize * sizeof(jbyte));
}

/**
 * Increases header data struct ref count.
 *
 * @param node data struct to increase ref count for
 */
void rmsdb_inc_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    node->refCount++;
}

/**
 * Decreases header data struct ref count.
 *
 * @param node data struct to decrease ref count for
 */
void rmsdb_dec_header_node_refcount(RecordStoreSharedDBHeaderList* node) {
    node->refCount++;

    if (node->refCount <= 0) {
        rmsdb_delete_header_node(node);
    }
}

