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

#ifndef _INCLUDED_INSPECTOR_H
#define _INCLUDED_INSPECTOR_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/gc_common.h"


enum {
    CVM_HEAPSTATEOBJ_INITIAL = 0,
    CVM_HEAPSTATEOBJ_FREED
};

typedef struct CVMHeapStateObject CVMHeapStateObject;
struct CVMHeapStateObject
{
    CVMObject *obj;
    CVMUint32 size;
};

/* If the lowest bit is set, then the object is marked as having been GCed: */
#define CVMheapStateObjectMarkAsFreed(hso) \
    (((hso)->obj) = (CVMObject *)((CVMAddr)((hso)->obj) | 0x1))
#define CVMheapStateObjectIsFreed(hso) \
    ((((CVMAddr)((hso)->obj)) & 0x1) != 0)

typedef struct CVMHeapState CVMHeapState;
struct CVMHeapState
{
    CVMHeapState *next;
    char *name;
    CVMUint32 id;
    CVMUint32 timeStamp;
    CVMUint32 numberOfObjects;
    CVMUint32 totalSize;
    CVMHeapStateObject objects[1];
};


typedef struct CVMInspector CVMInspector;
struct CVMInspector {
    /* Option to force GC to keep all objects alive: */
    CVMBool keepAllObjectsAlive;

    /* HeapState capture and analysis: */
    CVMBool hasCapturedState;
    CVMUint32 lastHeapStateID;
    CVMHeapState *heapStates;
};


/* ================================================================== */

#define CVMinspectorGCLockerLock CVMgcLockerLock

/* Purpose: Deactivates the inspector GC lock. */
/* NOTE: Calls to CVMinspectorGCLockerLock() & CVMinspectorGCLockerUnlock()
         can be nested.  Can be called while GC safe or unsafe.  Based on
         CVMgcLockerUnlock() with the addition of the cv notification. */
void CVMinspectorGCLockerUnlock(CVMGCLocker *self, CVMExecEnv *current_ee);

/* Purpose: Wait for the inspector GC lock to deactivate. */
/* NOTE: This is only used by GC code to wait for the inspector GCLocker to
         be deactivated.  This function assumes that the current thread
         already holds the gcLockerLock. */
/* NOTE: Called while all threads are GC safe. */
void CVMinspectorGCLockerWait(CVMGCLocker *self, CVMExecEnv *current_ee);

/* ================================================================== */


/* Purpose: Checks to see if the specified pointer is a valid object. */
extern CVMBool CVMgcIsValidObject(CVMExecEnv *ee, CVMObject *obj);

/* Purpose: Disables GC.  To be used for debugging purposes ONLY. */
/* Returns: CVM_TRUE if successful. */
CVMBool CVMgcDisableGC(void);

/* Purpose: Enables GC.  To be used for debugging purposes ONLY. */
/* Returns: CVM_TRUE if successful. */
CVMBool CVMgcEnableGC(void);

/* Purpose: Checks to see if GC is disabled.  To be used for debugging
            purposes ONLY. */
/* Returns: CVM_TRUE if GC is disabled. */
CVMBool CVMgcIsDisabled(void);

/* Purpose: Forces the GC to keep all objects alive or not.  To be used for
            debugging purposes ONLY. */
void CVMgcKeepAllObjectsAlive(CVMBool keepAlive);

/* Purpose: Dumps all references to the specified object. */
/* NOTE: The user should use CVMdumpObjectReferences() instead of calling this
         function directly.  CVMdumpObjectReferences() will set up the proper
         VM state and then calls this function to do the work. */
void CVMgcDumpObjectReferences(CVMObject *obj);

/* Purpose: Dumps all references to instances of the specified class. */
/* NOTE: The user should use CVMdumpClassReferences() instead of calling this
         function directly.  CVMdumpClassReferences() will set up the proper
         VM state and then calls this function to do the work. */
void CVMgcDumpClassReferences(const char *clazzname);

/* Purpose: Dumps all classblocks of the specified class. */
/* NOTE: The user should use CVMdumpClassBlocks() instead of calling this
         function directly.  CVMdumpClassBlocks() will set up the proper
         VM state and then calls this function to do the work. */
void CVMgcDumpClassBlocks(const char *clazzname);

/* Purpose: Dumps all GC roots keeping the specified object alive. */
/* NOTE: The user should use CVMdumpObjectGCRoots() instead of calling this
         function directly.  CVMdumpObjectGCRoots() will set up the proper
         VM state and then calls this function to do the work. */
void CVMgcDumpObjectGCRoots(CVMObject *obj);

/*
 * Heap dump support: Iterate over a contiguous-allocated range of objects.
 */
extern void
CVMgcDumpHeapSimple();

extern void
CVMgcDumpHeapVerbose();

extern void
CVMgcDumpHeapStats();

/* Heap State Capture analysis tool: */
enum {
    CVM_HEAPSTATE_SORT_NONE = 0,
    CVM_HEAPSTATE_SORT_BY_OBJ,
    CVM_HEAPSTATE_SORT_BY_OBJCLASS
};

extern void
CVMgcCaptureHeapState(const char *name);
extern void
CVMgcReleaseHeapState(CVMUint32 id);
extern void
CVMgcReleaseAllHeapState(void);
extern void
CVMgcListHeapStates(void);
extern void
CVMgcDumpHeapState(CVMUint32 id, int sortKey);
extern void
CVMgcCompareHeapState(CVMUint32 id1, CVMUint32 id2);
extern void
CVMgcHeapStateObjectMoved(CVMObject *oldObj, CVMObject *newObj);
extern void
CVMgcHeapStateObjectFreed(CVMObject *obj);

/* ================================================================== */

extern void CVMdumpObject(CVMObject* directObj);
extern void CVMdumpClassBlock(CVMClassBlock *cb);
extern void CVMdumpString(CVMObject *string);
extern void CVMdumpObjectReferences(CVMObject *obj);
extern void CVMdumpClassReferences(const char *clazzname);
extern void CVMdumpClassBlocks(const char *clazzname);
extern void CVMdumpObjectGCRoots(CVMObject *obj);

/* ================================================================== */

/* Purpose: Dumps misc system informaion. */
extern void CVMdumpSysInfo();

#endif /* _INCLUDED_INSPECTOR_H */
