/*
 * @(#)commonRef.c	1.23 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include "jvmdi.h"
#include "commonRef.h"
#include "util.h"
#include "typedefs.h"

#define HASH_SLOT_COUNT 1531    /* Check size of RefNode.refSlot if you change this */
#define ALL_REFS -1

#define OBJECT_ID(node) (node ? (jlong)node->seqNum : NULL_OBJECT_ID)

static JVMDI_RawMonitor refLock;

/*
 * Each object sent to the front end is tracked with the RefNode struct below.
 * External to this module, objects are identified by a jlong id which is 
 * simply the sequence number. A weak reference is usually used so that
 * the presence of a debugger-tracked object will not prevent
 * its collection. Once an object is collected, its RefNode may be 
 * deleted and the weak ref inside may be reused (these may happen in
 * either order). Using the sequence number
 * as the object id prevents ambiguity in the object id when the weak ref 
 * is reused.
 *
 * The ref member is changed from weak to strong when
 * gc of the object is to be prevented.
 * Whether or not it is strong, it is never exported from this module.
 *
 * A reference count of each jobject is also maintained here. It tracks
 * the number times an object has been referenced through 
 * commonRef_refToID. A RefNode is freed once the reference
 * count is decremented to 0 (with commonRef_release*), even if the 
 * correspoding object has not been collected.
 */
typedef struct RefNode {
    jobject ref;                /* could be strong or weak */
    unsigned int isStrong : 1;
    unsigned int refSlot : 12;   /* used to cleanup after id-based deletion */
    UNSIGNED_JLONG seqNum;
    jint count;
    struct RefNode *nextByRef;  /* used in ref-based hash table */
    struct RefNode *nextByID;   /* used in id-based hash table */
} RefNode;

/*
 * Two hash tables are maintained. One allows access to a RefNode through
 * its object id. The other allows access to a RefNode through a 
 * strong JNI reference. The same ref node is placed in both hash
 * tables when it is created and removed from both when it is destroyed.
 */
static RefNode *objectsByRef[HASH_SLOT_COUNT];
static RefNode *objectsByID[HASH_SLOT_COUNT];

static UNSIGNED_JLONG nextSeqNum;

/*
static void printRefTables() {
    int count;
    int i;
    RefNode *node;

    fprintf(stderr, "\nID-based object table:\n");
    for (count=0, i = 0; i < HASH_SLOT_COUNT; i++) {
        node = objectsByID[i];
        while (node != NULL) {
            fprintf(stderr, "%d: slot=%d ref=" PTR_FORMAT " isStrong=%x count=%d refSlot=%d\n",
                    (int)node->seqNum, i, (intptr_t)node->ref, node->isStrong, 
                    node->count, node->refSlot);
            count++;
            node = node->nextByID;
        }
    }
    fprintf(stderr, "Total of %d objects\n", count);

    fprintf(stderr, "\nRef-based object table:\n");
    for (count=0, i = 0; i < HASH_SLOT_COUNT; i++) {
        node = objectsByRef[i];
        while (node != NULL) {
            fprintf(stderr, PTR_FORMAT ": slot=%d id=%d isStrong=%x count=%d\n",
                    (intptr_t)node->ref, i, (int)node->seqNum, node->isStrong, 
                    node->count);
            count++;
            node = node->nextByRef;
        }
    }
    fprintf(stderr, "Total of %d objects\n", count);
}
*/

/*
 * Note that this may not give accurate results if called with a weak
 * reference.
 */
static jint hashRef(jobject ref) 
{
    jint hashCode = objectHashCode(ref);
    return abs(hashCode) % HASH_SLOT_COUNT;
}

static jint hashID(jlong id) 
{
    return ((UNSIGNED_JINT)id) % HASH_SLOT_COUNT;
}

static UNSIGNED_JLONG newSeqNum()
{
    return nextSeqNum++;
}

static RefNode *
createNode(JNIEnv *env, jobject ref) 
{
    RefNode *node;
    jobject weakRef = (*env)->NewWeakGlobalRef(env, ref);
    if (weakRef == NULL) {
        return NULL;
    }
                                                       
    /*
     * %comment gordonh006
     */
    node = jdwpAlloc(sizeof(*node));     
    if (node == NULL) {
        (*env)->DeleteWeakGlobalRef(env, weakRef);
        return NULL;
    }
    
    node->ref = weakRef;
    node->isStrong = JNI_FALSE;
    node->count = 1;
    node->seqNum = newSeqNum();
    return node;
}

static void 
deleteNode(JNIEnv *env, RefNode *node)
{
    /*fprintf(stderr, "Freeing %d (%x)\n", (int)node->seqNum, node->weakRef);*/
    if (node->isStrong) {
        (*env)->DeleteGlobalRef(env, node->ref);
    } else {
        (*env)->DeleteWeakGlobalRef(env, node->ref);
    }
    jdwpFree(node);
}

static jobject
strengthenNode(JNIEnv *env, RefNode *node) 
{
    if (!node->isStrong) {
        jobject strongRef = (*env)->NewGlobalRef(env, node->ref);
        /*
         * NewGlobalRef on a weak ref will return NULL if the weak 
         * reference has been collected or if out of memory. 
         * We need to distinguish those two occurrences.
         */
        if ((strongRef == NULL) && !(*env)->IsSameObject(env, node->ref, NULL)) {
            ERROR_MESSAGE_EXIT("Unable to create global reference");
        } 
        if (strongRef != NULL) {
            (*env)->DeleteWeakGlobalRef(env, node->ref);
            node->ref = strongRef;
        }
        return strongRef;
    } else {
        return node->ref;
    }
}

static jweak
weakenNode(JNIEnv *env, RefNode *node) 
{
    if (node->isStrong) {
        jweak weakRef = (*env)->NewWeakGlobalRef(env, node->ref);
        if (weakRef != NULL) {
            (*env)->DeleteGlobalRef(env, node->ref);
            node->ref = weakRef;
        }
        return weakRef;
    } else {
        return node->ref;
    }
}

/*
 * Returns the node which contains the common reference for the 
 * given object. The passed reference should not be a weak reference
 * managed in the object hash table (i.e. returned by commonRef_idToRef)
 * because no sequence number checking is done.
 */
static RefNode *
findNodeByRef(JNIEnv *env, jobject ref) 
{
    RefNode *node;
    RefNode *prev;
    jint slot = hashRef(ref);

    node = objectsByRef[slot];
    prev = NULL;

    while (node != NULL) {
        if ((*env)->IsSameObject(env, ref, node->ref)) {
            break;
        }
        prev = node;
        node = node->nextByRef;
    }

    return node;
}

static void
detachIDNode(JNIEnv *env, jint slot, RefNode *prev, RefNode *node) 
{
    RefNode *detached;

    /* Detach from id hash table */
    if (prev == NULL) {
        objectsByID[slot] = node->nextByID;
    } else {
        prev->nextByID = node->nextByID;
    }

    /* Find in ref hash table */
    detached = node;
    prev = NULL;
    node = objectsByRef[detached->refSlot];
    while (node != NULL) {
        if (node == detached) {
            /* Detach from ref hash table */
            if (prev == NULL) {
                objectsByRef[detached->refSlot] = node->nextByRef;
            } else {
                prev->nextByRef = node->nextByRef;
            }
            break;
        }
        prev = node;
        node = node->nextByRef;
    }
}


static void 
deleteNodeByID(JNIEnv *env, jlong id, jint refCount) 
{
    jint slot = hashID(id);
    RefNode *node = objectsByID[slot];
    RefNode *prev = NULL;

    while (node != NULL) {
        if (id == OBJECT_ID(node)) {
            if (refCount != ALL_REFS) {
                node->count -= refCount;
            } else {
                node->count = 0;
            }
            if (node->count <= 0) {
                detachIDNode(env, slot, prev, node);
                deleteNode(env, node);
            }
            break;
        }
        prev = node;
        node = node->nextByID;
    }
}

/*
 * Returns the node stored in the object hash table for the given object
 * id. The id should be a value previously returned by 
 * commonRef_refToID.
 */
static RefNode *
findNodeByID(JNIEnv *env, jlong id) 
{
    jint slot = hashID(id);
    RefNode *node = objectsByID[slot];

    while (node != NULL) {
        /*
         * Use this opportunity to clean up any nodes for weak 
         * references that have been garbage collected.
         */
        if ((*env)->IsSameObject(env, node->ref, NULL)) {
            jlong collectedID = OBJECT_ID(node);
            node = node->nextByID;
            deleteNodeByID(env, collectedID, ALL_REFS);
        } else if (id == OBJECT_ID(node)) {
            break;  /* found it */
        } else {
            node = node->nextByID;
        }
    }
    return node;
}


static RefNode *
newCommonRef(JNIEnv *env, jobject ref) 
{
    RefNode *node;
    jint slot;

    node = createNode(env, ref);

    /*
     * Add to reference hashtable 
     */
    slot = hashRef(ref);
    node->nextByRef = objectsByRef[slot];
    node->refSlot = slot;
    objectsByRef[slot] = node;

    /*
     * Add to id hashtable 
     */
    slot = hashID((jlong)node->seqNum);
    node->nextByID = objectsByID[slot];
    objectsByID[slot] = node;

    return node;
}

void 
commonRef_initialize() 
{
    refLock = debugMonitorCreate("JDWP Reference Table Monitor");
    nextSeqNum = 1;              /* 0 used for error indication */
}

void 
commonRef_reset()
{
    JNIEnv *env = getEnv();
    int i;
    RefNode *node;

    debugMonitorEnter(refLock);

    for (i = 0; i < HASH_SLOT_COUNT; i++) {
        node = objectsByID[i];
        while (node != NULL) {
            RefNode *temp = node->nextByID;
            deleteNode(env, node);
            node = temp;
        }
        objectsByID[i] = NULL;
        objectsByRef[i] = NULL;
    }
    nextSeqNum = 1;

    debugMonitorExit(refLock);
}

/*
 * Given a reference obtained from JNI or JVMDI, return an object
 * id suitable for sending to the debugger front end. The original
 * reference is not deleted. 
 */
jlong 
commonRef_refToID(jobject ref) 
{
    JNIEnv *env = getEnv();
    RefNode *node;
    jlong id;

    if (ref == NULL) {
        return NULL_OBJECT_ID;
    }

    debugMonitorEnter(refLock);

    node = findNodeByRef(env, ref);
    if (node == NULL) {
        node = newCommonRef(env, ref);
    } else {
        node->count++;
    }

    id = OBJECT_ID(node);

    debugMonitorExit(refLock);

    return id;
}

/*
 * Given an object ID obtained from the debugger front end, return a
 * strong, global reference to that object (or NULL if the object
 * has been collected). The reference can then be used for JNI and 
 * JVMDI calls. Caller is resposible for deleting the returned reference.
 */
jobject 
commonRef_idToRef(jlong id) 
{
    JNIEnv *env = getEnv();
    jobject ref = NULL;
    RefNode *node;

    debugMonitorEnter(refLock);

    node = findNodeByID(env, id);
    if (node != NULL) {
        ref = (*env)->NewGlobalRef(env, node->ref);
    }

    debugMonitorExit(refLock);

    return ref;
}

/* Prevent garbage collection of an object */
jint 
commonRef_pin(jlong id) 
{
    JNIEnv *env = getEnv();
    jint rc = JVMDI_ERROR_NONE;
    RefNode *node;

    if (id == NULL_OBJECT_ID) {
        return JNI_FALSE;
    }

    debugMonitorEnter(refLock);

    node = findNodeByID(env, id);
    if (node == NULL) {
        rc = JVMDI_ERROR_INVALID_OBJECT;
    } else {
        jobject strongRef = strengthenNode(env, node);
        if (strongRef == NULL) {
            /*
             * Referent has been collected, clean up now.
             */
            rc = JVMDI_ERROR_INVALID_OBJECT;
            deleteNodeByID(env, id, ALL_REFS);
        } 
    }

    debugMonitorExit(refLock);

    return rc;
}

/* Permit garbage collection of an object */
jint 
commonRef_unpin(jlong id) 
{
    JNIEnv *env = getEnv();
    jint rc = JVMDI_ERROR_NONE;
    RefNode *node;

    debugMonitorEnter(refLock);

    node = findNodeByID(env, id);
    if (node != NULL) {
        jweak weakRef = weakenNode(env, node);
        if (weakRef == NULL) {
            rc = JVMDI_ERROR_OUT_OF_MEMORY;
        } 
    }

    debugMonitorExit(refLock);

    return rc;
}

void 
commonRef_release(jlong id) 
{
    JNIEnv *env = getEnv();

    debugMonitorEnter(refLock);

    deleteNodeByID(env, id, 1);

    debugMonitorExit(refLock);
}

void 
commonRef_releaseMultiple(jlong id, jint refCount) 
{
    JNIEnv *env = getEnv();

    debugMonitorEnter(refLock);

    deleteNodeByID(env, id, refCount);

    debugMonitorExit(refLock);
}

/*
 * Get rid of RefNodes for objects that no longer exist
 */
void 
commonRef_compact() 
{
    JNIEnv *env = getEnv();
    RefNode *node;
    RefNode *prev;
    int i;

    /* printRefTables(); */

    debugMonitorEnter(refLock);

    /* 
     * Part 1: Walk through the id-based hash table. Detach any nodes
     * for which the ref has been collected. Mark the node instead of 
     * deleting it so that the second part through the ref-based hash
     * table can find it.
     */
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
        node = objectsByID[i];
        prev = NULL;
        while (node != NULL) {
            /* Has the object been collected? */
            if ((*env)->IsSameObject(env, node->ref, NULL)) {
                /* Mark it to be freed by part 2 */
                node->count = 0;

                /* Detach from the ID list */
                if (prev == NULL) {
                    objectsByID[i] = node->nextByID;
                } else {
                    prev->nextByID = node->nextByID;
                }
            } else {
                prev = node;
            }
            node = node->nextByID;
        }
    }

    /*
     * Part 2: Walk through the ref-based hash table. Detach and 
     * delete all nodes that were marked in pass 1.
     */
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
        node = objectsByRef[i];
        prev = NULL;
        while (node != NULL) {
            /* Has the object been marked? */
            if (node->count == 0) {
                RefNode *freed = node;
                /* Detach from the ref list */
                if (prev == NULL) {
                    objectsByRef[i] = freed->nextByRef;
                } else {
                    prev->nextByRef = freed->nextByRef;
                }
                node = node->nextByRef;
                deleteNode(env, freed);
            } else {
                prev = node;
                node = node->nextByRef;
            }
        }
    }
    debugMonitorExit(refLock);

    /* printRefTables(); */
}

void 
commonRef_lock() 
{
    debugMonitorEnter(refLock);
}

void 
commonRef_unlock() 
{
    debugMonitorExit(refLock);
}


