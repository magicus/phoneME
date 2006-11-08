/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
/*
 * This module tracks classes that have been prepared, so as to
 * be able to compute which have been unloaded.  On VM start-up
 * all prepared classes are put in a table.  As class prepare
 * events come in they are added to the table.  After an unload
 * event or series of them, the VM can be asked for the list
 * of classes; this list is compared against the table keep by
 * this module, any classes no longer present are known to
 * have been unloaded.
 *
 * For efficient access, classes are keep in a hash table.  
 * Each slot in the hash table has a linked list of KlassNode.
 *
 * Comparing current set of classes is compared with previous
 * set by transferring all classes in the current set into
 * a new table, any that remain in the old table have been
 * unloaded.
 */
#include <stdlib.h>
#include "jvmdi.h"
#include "bag.h"
#include "classTrack.h"
#include "util.h"

#define HASH_SLOT_COUNT 263    /* Prime which eauals 4k+3 for some k */

typedef struct KlassNode {
    jclass klass;            /* weak global reference */
    char *signature;         /* class signature */
    struct KlassNode *next;  /* next node in this slot */
} KlassNode;

/*
 * Hash table of prepared classes.  Each entry is a pointer
 * to a linked list of KlassNode.
 */
static KlassNode **table;

/*
 * Return slot in hash table to use for this class.
 */
static jint 
hashKlass(jclass klass) 
{
    jint hashCode = objectHashCode(klass);
    return abs(hashCode) % HASH_SLOT_COUNT;
}

/*
 * Transfer a node (which represents klass) from the current 
 * table to the new table.
 */
static void 
transferClass(JNIEnv *env, jclass klass, KlassNode **newTable) {
    jint slot = hashKlass(klass);
    KlassNode **head = &table[slot];
    KlassNode **newHead = &newTable[slot];
    KlassNode **nodePtr;
    KlassNode *node;
    
    /* Search the node list of the current table for klass */
    for (nodePtr = head; node = *nodePtr, node != NULL; nodePtr = &(node->next)) {
        if ((*env)->IsSameObject(env, klass, node->klass)) {
            /* Match found transfer node */

            /* unlink from old list */
            *nodePtr = node->next;  

            /* insert in new list */
            node->next = *newHead;   
            *newHead = node;

            return;
        }
    }

    /* we haven't found the class, only unloads should have happenned, 
     * so the only reason a class should not have been found is
     * that it is not prepared yet, in which case we don't want it.
     * Asset that the above is true.
     */
/**** the HotSpot VM doesn't create prepare events for some internal classes ***
    JDI_ASSERT_MSG((classStatus(klass) & JVMDI_CLASS_STATUS_PREPARED) == 0 && 
                   !isArrayClass(klass),
                   classSignature(klass));
***/
}

/* 
 * Delete a hash table of classes. 
 * The signatures of classes in the table are returned.
 */
static struct bag *
deleteTable(JNIEnv *env, KlassNode *oldTable[])
{
    struct bag *signatures = bagCreateBag(sizeof(char*), 10);
    jint slot;

    if (signatures == NULL) {
        ALLOC_ERROR_EXIT();
    }

    for (slot = 0; slot < HASH_SLOT_COUNT; slot++) {
        KlassNode *node = oldTable[slot];
        
        while (node != NULL) {
            KlassNode *next;
            char **sigSpot;

            /* Add signature to the signature bag */
            sigSpot = bagAdd(signatures);   
            if (sigSpot == NULL) {
                ALLOC_ERROR_EXIT();
            }
            *sigSpot = node->signature;
            
            /* Free weak ref and the node itself */
            (*env)->DeleteWeakGlobalRef(env, node->klass);
            next = node->next;
            jdwpFree(node);

            node = next;
        }
    }
    jdwpFree(oldTable);

    return signatures;
}

/* 
 * Called after class unloads have occurred.  Creates a new hash table
 * of currently loaded prepared classes. 
 * The signatures of classes which were unloaded (not present in the
 * new table) are returned.
 */
struct bag *
classTrack_processUnloads(JNIEnv *env)
{
    KlassNode **newTable = jdwpClearedAlloc(HASH_SLOT_COUNT * sizeof(KlassNode *));
    jint classCount;    
    jclass *classes;
    jint i;
    struct bag *unloadedSignatures;

    if (newTable == NULL) {
        ALLOC_ERROR_EXIT();
    }
    if ((classes = allLoadedClasses(&classCount)) == NULL) {
        jdwpFree(newTable);
        ALLOC_ERROR_EXIT();
    }

    /* Transfer each current class into the new table */
    for (i=0; i<classCount; i++) {
        jclass klass = classes[i];
        transferClass(env, klass, newTable);
        (*env)->DeleteGlobalRef(env, klass);
    }
    jdwpFree(classes);

    /* Delete old table, install new one */
    unloadedSignatures = deleteTable(env, table);
    table = newTable;

    return unloadedSignatures;
}

/*
 * Add a class to the prepared class hash table.
 * Assumes no duplicates.
 */
void
classTrack_addPreparedClass(JNIEnv *env, jclass klass)
{
    jint slot = hashKlass(klass);
    KlassNode **head = &table[slot];
    KlassNode *node;

    if (assertOn) {
        /* Check this is not a duplicate */
        for (node = *head; node != NULL; node = node->next) {
            if ((*env)->IsSameObject(env, klass, node->klass)) {
                JDI_ASSERT_FAILED("Attempting to insert duplicate class");
                break;
            }
        }
    }
       
    node = jdwpAlloc(sizeof(KlassNode));
    if (node == NULL) {
        ALLOC_ERROR_EXIT();
    }
    if ((node->signature = classSignature(klass)) == NULL) {
        jdwpFree(node);
        ALLOC_ERROR_EXIT();
    }
    if ((node->klass = (*env)->NewWeakGlobalRef(env, klass)) == NULL) {
        jdwpFree(node->signature);
        jdwpFree(node);
        ALLOC_ERROR_EXIT();
    }

    /* Insert the new node */
    node->next = *head;
    *head = node;
}

/*
 * Called once to build the initial prepared class hash table.
 */
void
classTrack_initialize()
{
    JNIEnv *env = getEnv();
    jint classCount;    
    jclass *classes = allLoadedClasses(&classCount);
    jint i;

    if (classes == NULL) {
        ALLOC_ERROR_EXIT();
    }
    table = jdwpClearedAlloc(HASH_SLOT_COUNT * sizeof(KlassNode *));
    if (table == NULL) {
        jdwpFree(classes);
        ALLOC_ERROR_EXIT();
    }
    for (i=0; i<classCount; i++) {
        jclass klass = classes[i];

        /* Filter out unprepared classes (arrays may or
         * may not be marked as prepared) */
        jboolean preped = (classStatus(klass) & JVMDI_CLASS_STATUS_PREPARED) != 0;
        if (preped || isArrayClass(klass)) {
            classTrack_addPreparedClass(env, klass);
        }
        (*env)->DeleteGlobalRef(env, klass);
    }
    jdwpFree(classes);
}    
