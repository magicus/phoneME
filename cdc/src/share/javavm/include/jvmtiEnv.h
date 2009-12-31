/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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

#define HASH_SLOT_COUNT 1531    /* Check size of RefNode.refSlot if you
				   change this */
#define ALL_REFS -1

typedef struct CVMJvmtiMethodNode CVMJvmtiMethodNode;
struct CVMJvmtiMethodNode {
    CVMUint32 mid;              /* method ID */
    CVMMethodBlock *mb;
    CVMBool isObsolete;
    CVMConstantPool *cp;
    CVMJvmtiMethodNode *next;   /* next node* in bucket chain */
};

typedef struct CVMJvmtiTagNode CVMJvmtiTagNode;
struct CVMJvmtiTagNode {
    jlong tag;                  /* opaque tag */
    jobject      ref;           /* could be strong or weak */
    CVMJvmtiTagNode *next;      /* next RefNode* in bucket chain */
};

enum {
    JVMTI_MAGIC = 0x71EE,
    BAD_MAGIC   = 0xDEAD
};

typedef struct CVMJvmtiEventEnabled CVMJvmtiEventEnabled;
struct CVMJvmtiEventEnabled{
    jlong enabledBits;
};

typedef struct CVMJvmtiVisitStack CVMJvmtiVisitStack;
struct CVMJvmtiVisitStack {
    CVMObject **stackBase;
    CVMObject **stackPtr;
    jvmtiEnv *env;
    int stackSize;
};

#define VISIT_STACK_INIT 4096
#define VISIT_STACK_INCR 1024

typedef struct CVMJvmtiDumpContext CVMJvmtiDumpContext;
struct CVMJvmtiDumpContext {
    jint heapFilter;
    jclass klass;
    const jvmtiHeapCallbacks *callbacks;
    const void *userData;
    CVMExecEnv *ee;
    JNIEnv *env;
    CVMFrame  *frame;
    jint frameCount;
    CVMObjectICell *icell;
};

typedef struct CVMJvmtiEnvEventEnable CVMJvmtiEnvEventEnable;
struct CVMJvmtiEnvEventEnable {

    /* user set global event enablement indexed by jvmtiEvent   */
    CVMJvmtiEventEnabled eventUserEnabled;

    /*
     * this flag indicates the presence (true) or absence (false) of event
     * callbacks.  it is indexed by jvmtiEvent  
     */
    CVMJvmtiEventEnabled eventCallbackEnabled;

    /*
     * indexed by jvmtiEvent true if enabled globally or on any thread.
     * True only if there is a callback for it.
     */
    CVMJvmtiEventEnabled eventEnabled;
};

/* Note: CVMJvmtiContext is based on JDK HotSpot's JvmtiEnv class.  It is
   renamed here to adhere to CVM coding conventions as well as to avoid
   confusion with the jvmtiEnv struct/class which is defined in jvmti.h.

   One CVMJvmtiContext object is created per jvmti/jvmdi agentLib attachment.
   This is done via JNI's GetEnv() call.  Multiple attachments are allowed in
   jvmti, and in the case of jvmdi, only one attachment works, so only one
   object is created for jvmdi.

   NOTE: Currently, the CVM JVMTI implementation only supports the use of one
   single agentLib per VM launch.

   Note: there is a one to one correspondence between the CVMJvmtiContext and
   the jvmtiEnv.  Hence, we can always get to one from the other.
*/
typedef struct CVMJvmtiContext CVMJvmtiContext;
struct CVMJvmtiContext {

    jvmtiEnv jvmtiExternal;
    const void *envLocalStorage;     /* per env agent allocated data. */
    jvmtiEventCallbacks eventCallbacks;
    jboolean isValid;
    jboolean threadEventsEnabled;

    jint magic;
    jint index;

    CVMJvmtiEnvEventEnable envEventEnable;

    jvmtiCapabilities currentCapabilities;
    jvmtiCapabilities prohibitedCapabilities;

    jboolean  classFileLoadHookEverEnabled;

    char** nativeMethodPrefixes;
    int    nativeMethodPrefixCount;
};

#define CVMjvmtiEnv2Context(jvmtienv_) \
    ((CVMJvmtiContext*)((CVMUint8 *)(jvmtienv_) - \
			CVMoffsetof(CVMJvmtiContext, jvmtiExternal)))

/* Purpose: Checks to see if it is safe to access object pointers directly. */
#define CVMjvmtiIsSafeToAccessDirectObject(ee_)         \
    (CVMD_isgcUnsafe(ee_) || CVMgcIsGCThread(ee_) ||    \
     CVMjvmtiIsGCOwner() ||                             \
     CVMsysMutexIAmOwner((ee_), &CVMglobals.heapLock))

/* Purpose: Gets the direct object pointer for the specified ICell. */
/* NOTE: It is only safe to use this when we are in a GC unsafe state, or we're
         are at a GC safe all state, or we're holding the thread lock. */
#define CVMjvmtiGetICellDirect(ee_, icellPtr_) \
    CVMID_icellGetDirectWithAssertion(CVMjvmtiIsSafeToAccessDirectObject(ee_), \
                                      icellPtr_)

/* Purpose: Sets the direct object pointer in the specified ICell. */
/* NOTE: It is only safe to use this when we are in a GC unsafe state, or we're
         are at a GC safe all state, or we're holding the thread lock. */
#define CVMjvmtiSetICellDirect(ee_, icellPtr_, directObj_) \
    CVMID_icellSetDirectWithAssertion(CVMjvmtiIsSafeToAccessDirectObject(ee_), \
                                      icellPtr_, directObj_)

/* Purpose: Assigns the value of one ICell to another. */
/* NOTE: It is only safe to use this when we are in a GC unsafe state, or we're
         are at a GC safe all state, or we're holding the thread lock. */
#define CVMjvmtiAssignICellDirect(ee_, dstICellPtr_, srcICellPtr_) \
    CVMjvmtiSetICellDirect((ee_), (dstICellPtr_),		   \
	CVMjvmtiGetICellDirect((ee_), (srcICellPtr_)))

jvmtiError CVMjvmtiVisitStackPush(CVMObject *obj);
CVMBool CVMjvmtiVisitStackEmpty();
void CVMjvmtiCleanupMarked();
void CVMjvmtiRecomputeEnabled(CVMJvmtiEnvEventEnable *);
jlong CVMjvmtiRecomputeThreadEnabled(CVMExecEnv *ee, CVMJvmtiEnvEventEnable *);
int CVMjvmtiDestroyContext(CVMJvmtiContext *context);
/* See also CVMjvmtiClassObject2ClassBlock() in jvmtiExport.h.
   CVMjvmtiClassObject2ClassBlock() takes a direct class object as input while
   CVMjvmtiClassRef2ClassBlock() takes a class ref i.e.  jclass.
*/
CVMClassBlock *CVMjvmtiClassRef2ClassBlock(CVMExecEnv *ee, jclass clazz);
CVMBool CVMjvmtiIsGCOwner();
void CVMjvmtiSetGCOwner(CVMBool);

#endif /* _INCLUDED_JVMTIENV_H */
