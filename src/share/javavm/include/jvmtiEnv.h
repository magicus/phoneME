/*
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
 *
 */


#ifndef _INCLUDED_JVMTIENV_H
#define _INCLUDED_JVMTIENV_H



/* The following are some data structures for debugger type functionality: */

struct bkpt {
    CVMUint8* pc;      /* key - must be first */
    CVMUint8 opcode;   /* opcode to restore */
    jobject classRef;  /* Prevents enclosing class from being gc'ed */
};

struct fpop {
    CVMFrame* frame;       /* key - must be first */
    /* CVMUint8* returnpc; */ /* Was used for PC mangling in JDK version.
				 Now just indicates set membership. */
};

struct fieldWatch {
    CVMFieldBlock* fb;  /* field to watch; key - must be first */
    jclass classRef;    /* Prevents enclosing class from being gc'ed */
};

/* END of debugger data structures. */


enum {
    JVMTI_INTERNAL_CAPABILITY_COUNT = 39
};

/* Tag stuff */

#define HASH_SLOT_COUNT 1531    /* Check size of RefNode.refSlot if you change this */
#define ALL_REFS -1

typedef struct JvmtiMethodNode {
    CVMUint32 mid;              /* method ID */
    CVMMethodBlock *mb;
    CVMBool isObsolete;
    CVMConstantPool *cp;
    struct JvmtiMethodNode *next;    /* next node* in bucket chain */
} JvmtiMethodNode;

typedef struct JvmtiTagNode {
    jlong tag;                  /* opaque tag */
    jobject      ref;           /* could be strong or weak */
    struct JvmtiTagNode *next;       /* next RefNode* in bucket chain */
} JvmtiTagNode;

enum {
    JVMTI_MAGIC = 0x71EE,
    BAD_MAGIC   = 0xDEAD
};

typedef struct {
    jlong enabledBits;
} JvmtiEventEnabled;

typedef struct visit_stack {
    CVMObject **stackBase;
    CVMObject **stackPtr;
    jvmtiEnv *jvmtiEnv;
    int stackSize;
} JvmtiVisitStack;

#define VISIT_STACK_INIT 4096
#define VISIT_STACK_INCR 1024

typedef struct JvmtidumpContext {
    jint heapFilter;
    jclass klass;
    const jvmtiHeapCallbacks *callbacks;
    const void *userData;
    CVMExecEnv *ee;
    JNIEnv *env;
    CVMFrame  *frame;
    jint frameCount;
    CVMObjectICell *icell;
} JvmtiDumpContext;

typedef struct _JvmtiEnvEventEnable{

    /* user set global event enablement indexed by jvmtiEvent   */
    JvmtiEventEnabled eventUserEnabled;

    /*
     * this flag indicates the presence (true) or absence (false) of event callbacks
     *   it is indexed by jvmtiEvent  
     */
    JvmtiEventEnabled eventCallbackEnabled;

    /*
     * indexed by jvmtiEvent true if enabled globally or on any thread.
     * True only if there is a callback for it.
     */
    JvmtiEventEnabled eventEnabled;

} JvmtiEnvEventEnable;


typedef struct _JvmtiEnv {

    jvmtiEnv jvmtiExternal;
    const void *envLocalStorage;     /* per env agent allocated data. */
    jvmtiEventCallbacks eventCallbacks;
    jboolean isValid;
    jboolean threadEventsEnabled;

    jint magic;
    jint index;

    JvmtiEnvEventEnable envEventEnable;

    jvmtiCapabilities currentCapabilities;
    jvmtiCapabilities prohibitedCapabilities;

    jboolean  classFileLoadHookEverEnabled;

    char** nativeMethodPrefixes;
    int    nativeMethodPrefixCount;
}JvmtiEnv;

jint CVMdestroyJvmti(JvmtiEnv *env);
jvmtiError CVMjvmtiVisitStackPush(CVMObject *obj);
CVMBool CVMjvmtiVisitStackEmpty();
void CVMjvmtiCleanupMarked();
void CVMjvmtiRecomputeEnabled(JvmtiEnvEventEnable *);
jlong CVMjvmtiRecomputeThreadEnabled(CVMExecEnv *ee, JvmtiEnvEventEnable *);
CVMClassBlock* CVMjvmtiClassObject2CB(CVMExecEnv *ee, jclass clazz);

#endif /* _INCLUDED_JVMTIENV_H */
