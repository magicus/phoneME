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

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/time.h"

#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/inspector.h"
#include "javavm/include/indirectmem.h"
#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#endif

#include "native/common/jni_util.h"

#include "generated/offsets/java_lang_String.h"
#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif

/*===========================================================================*/

#ifdef CVM_INSPECTOR

/* Purpose: Disables GC.  To be used for debugging purposes ONLY. */
/* Returns: CVM_TRUE if successful. */
CVMBool CVMgcDisableGC(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMGCLocker *inspectorGCLocker = &CVMglobals.inspectorGCLocker;
    CVMBool success = CVM_TRUE;

    if (CVMgcLockerGetLockCount(inspectorGCLocker) > 0) {
        CVMconsolePrintf("GC is already disabled!  No need to disable.\n");
    } else {
        CVMinspectorGCLockerLock(inspectorGCLocker, ee);
    }
    return success;
}

/* Purpose: Enables GC.  To be used for debugging purposes ONLY. */
/* Returns: CVM_TRUE if successful. */
CVMBool CVMgcEnableGC(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMBool success = CVM_TRUE;
    CVMGCLocker *inspectorGCLocker = &CVMglobals.inspectorGCLocker;

    if (CVMgcLockerGetLockCount(inspectorGCLocker) == 0) {
        CVMconsolePrintf("GC was not disabled!  No need to enable.\n");
    } else {
        CVMinspectorGCLockerUnlock(inspectorGCLocker, ee);
    }
    return success;
}

/* Purpose: Checks to see if GC is disabled.  To be used for debugging
            purposes ONLY. */
/* Returns: CVM_TRUE if GC is disabled. */
CVMBool CVMgcIsDisabled(void)
{
    CVMGCLocker *inspectorGCLocker = &CVMglobals.inspectorGCLocker;
    return CVMgcLockerIsActive(inspectorGCLocker);
}

/* Purpose: Forces the GC to keep all objects alive or not.  To be used for
            debugging purposes ONLY. */
void CVMgcKeepAllObjectsAlive(CVMBool keepAlive)
{
    CVMglobals.inspector.keepAllObjectsAlive = keepAlive;
}

/*================================================= Inspector GC Locker ==*/

/* Purpose: Deactivates the debug GC lock. */
/* NOTE: Calls to CVMinspectorGCLockerLock() & CVMinspectorGCLockerUnlock()
         can be nested.  Can be called while GC safe or unsafe.  Based on
         CVMgcLockerUnlock() with the addition of the cv notification. */
void CVMinspectorGCLockerUnlock(CVMGCLocker *self, CVMExecEnv *current_ee)
{
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexLock (current_ee, &CVMglobals.gcLockerLock);
    }
    CVMsysMicroLock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    if (self->lockCount > 0) {
        self->lockCount--;

        /* The following notification is for waking up those threads that were
           waiting using CVMgcLockerWait().  However, CVMgcLockerWait() is
           only called after all threads are GC safe.  If the current thread
           is still GC unsafe, then we know that there are no threads waiting
           using CVMgcLockerWait() and no notification is needed.
        */
        if (CVMD_isgcSafe(current_ee)) {
            if (self->lockCount == 0 && self->wasContended) {
                CVMcondvarNotify(&CVMglobals.gcLockerCV);
            }
        }
    }
    CVMsysMicroUnlock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexUnlock (current_ee, &CVMglobals.gcLockerLock);
    }
}

/* Purpose: Wait for the debug GC lock to deactivate. */
/* NOTE: This is only used by GC code to wait for the debug GCLocker to be
         deactivated.  This function assumes that the current thread
         already holds the gcLockerLock. */
/* NOTE: Called while all threads are GC safe. */
void CVMinspectorGCLockerWait(CVMGCLocker *self, CVMExecEnv *current_ee)
{
    CVMassert(CVMD_gcAllThreadsAreSafe());
    CVMassert(CVMsysMutexOwner(&CVMglobals.gcLockerLock) == current_ee);
    while (self->lockCount > 0) {
        self->wasContended = CVM_TRUE;
        CVMsysMutexWait(current_ee, &CVMglobals.gcLockerLock,
                        &CVMglobals.gcLockerCV, CVMlongConstZero());
    }
}

/*================================================= ObjectValidityVerifier ==*/

/* Purpose: Call back function for CVMgcIsValidHeapObject(). */
/* Returns: CVM_FALSE if the specified object is found (i.e. abort scan).
            Else returns CVM_TRUE. */
static CVMBool
CVMgcIsValidHeapObjectCallBack(CVMObject *obj, CVMClassBlock *cb,
                               CVMUint32 size, void *data)
{
    if (obj == (CVMObject *)data) {
        return CVM_FALSE; /* Found match.  Hence abort. */
    }
    return CVM_TRUE;
}

/* Purpose: Checks to see if the specified pointer corresponds to a valid
            object in the heap. */
/* Returns: CVM_TRUE if the specified object is valid.  Else returns
            CVM_FALSE. */
CVMBool CVMgcIsValidHeapObject(CVMExecEnv *ee, CVMObject *obj)
{
    CVMBool status =
        CVMgcimplIterateHeap(ee, CVMgcIsValidHeapObjectCallBack, (void *) obj);
    return (status == CVM_FALSE);
}

/* Purpose: Checks to see if the specified pointer corresponds to a valid
            object. */
/* Returns: CVM_TRUE if the specified object is valid.  Else returns
            CVM_FALSE. */
CVMBool CVMgcIsValidObject(CVMExecEnv *ee, CVMObject *obj)
{
    /* Check if pointer is a valid heap object: */
    if (CVMgcIsValidHeapObject(ee, obj)) {
        return CVM_TRUE;
    }
    /* If its not an object on the heap, check if pointer is a valid ROMized
       object: */
    if (CVMpreloaderIsPreloadedObject(obj)) {
        return CVM_TRUE;
    }
    return CVM_FALSE;
}

/*================================================== ObjectReferenceFinder ==*/

/*
 * Reference Finder routines
 */

/* Purpose: Call back function to process a non-null reference. */
static void
CVMgcDumpReferencesCallBack(CVMObject **ref, void *data)
{
    CVMGCProfilingInfo *info = (CVMGCProfilingInfo *) data;
    CVMObject *targetObj = (CVMObject *) info->data;
    if (*ref == targetObj) {
        const char *typestr = "INVALID";
        switch (info->type) {
            case CVMGCRefType_GLOBAL_ROOT: {
                typestr = "GLOBAL ROOT";
                break;
            }
            case CVMGCRefType_PRELOADER_STATICS: {
                typestr = "PRELOADER STATICS ROOT";
                break;
            }
            case CVMGCRefType_CLASS_STATICS: {
                /* Check for the case where we came across the ICell in the
                   classblock which points to the self class object.  This is
                   not a valid reference to the targetObj i.e. the class in
                   this case: */
                if (ref == (CVMObject **) CVMcbJavaInstance(info->u.clazz.cb)) {
                    /* This is kept here for future reference:
                    CVMconsolePrintf("   Ref 0x%x type: ICell inside classblock"
                                     " of %C pointing to itself\n",
                                     ref, info->u.clazz.cb);
                    */
                    return;
                }
                CVMconsolePrintf("   Ref 0x%x type: STATIC FIELD of class %C\n",
                                 ref, info->u.clazz.cb);
                return;
            }
            case CVMGCRefType_LOCAL_ROOTS: {
                typestr = "LOCAL ROOT";
                break;
            }
            case CVMGCRefType_UNKNOWN_STACK_FRAME: {
                typestr = "UNKNOWN STACK_FRAME ROOT";
                break;
            }
            case CVMGCRefType_JAVA_FRAME: {
                CVMconsolePrintf("   Ref 0x%x type: JAVA FRAME ROOT ee 0x%x frame %d\n",
                                 ref, info->u.frame.ee, info->u.frame.frameNumber);
                return;
            }
            case CVMGCRefType_JNI_FRAME: {
                CVMconsolePrintf("   Ref 0x%x type: JNI FRAME ROOT ee 0x%x frame %d\n",
                                 ref, info->u.frame.ee, info->u.frame.frameNumber);
                return;
            }
            case CVMGCRefType_TRANSITION_FRAME: {
                typestr = "TRANSITION FRAME ROOT";
                break;
            }
            default:
                CVMassert(CVM_FALSE);
        }
        CVMconsolePrintf("   Ref 0x%x type: %s\n", ref, typestr);
    }
}

/* Purpose: Dump references from the fields of an object. */
static CVMBool
CVMgcDumpReferencesObjectCallback(CVMObject *obj, CVMClassBlock *cb,
                                  CVMUint32 size, void *data)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMObject *targetObj = (CVMObject *) data;
    CVMGCOptions gcOpts = {
	/* isUpdatingObjectPointers */ CVM_FALSE,
        /* discoverWeakReferences   */ CVM_FALSE,
        /* isProfilingPass          */ CVM_TRUE
    };
    CVMGCOptions *gcOptsPtr = &gcOpts;

    CVMobjectWalkRefsWithSpecialHandling(ee, gcOptsPtr, obj,
                                         CVMobjectGetClassWord(obj), {
        if (*refPtr == targetObj) {
            CVMconsolePrintf("   Ref 0x%x type: OBJECT_FIELD Addr: 0x%x Size:"
                             " %d \tClass: %C\n", refPtr, obj, size, cb);
        }
    }, CVMgcDumpReferencesCallBack, data);
    return CVM_TRUE;
}

/* Purpose: Scans all roots and the heap for references to the specified
            object. */
/* NOTE: Should only be called from a GC safe state with the HeapLock already
         acquired by the current thread to prevent GC from running.  The
         current thread should already have called "stopped the world" (see
         CVMgcStopTheWorld() for details on how to stop the world). */
static void CVMgcDumpReferences(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMGCOptions gcOpts = {
	/* isUpdatingObjectPointers */ CVM_FALSE,
        /* discoverWeakReferences   */ CVM_FALSE,
        /* isProfilingPass          */ CVM_TRUE
    };

    CVMassert(CVMsysMutexOwner(&CVMglobals.heapLock) == ee);
    CVMassert(CVMD_isgcSafe(ee));

    CVMconsolePrintf("List of references to object 0x%x (%C):\n",
                     obj, CVMobjectGetClass(obj));

    CVMgcClearClassMarks(ee, NULL);

    CVMgcScanRoots(ee, &gcOpts, CVMgcDumpReferencesCallBack, obj);

    CVMgcimplIterateHeap(ee, CVMgcDumpReferencesObjectCallback, obj);
}

#ifdef CVM_ROOT_FINDER
static void CVMgcDumpGCRoots(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMGCOptions gcOpts = {
	/* isUpdatingObjectPointers */ CVM_FALSE,
        /* discoverWeakReferences   */ CVM_FALSE,
        /* isProfilingPass          */ CVM_TRUE
    };
    
    CVMassert(CVMsysMutexOwner(&CVMglobals.heapLock) == ee);
    CVMassert(CVMD_isgcSafe(ee));

    CVMconsolePrintf("List of references to object 0x%x (%C):\n",
                     obj, CVMobjectGetClass(obj));

    /* TODO:
       this is where we have to set up data structures to track the
       references so that we can traverse each reverse reference. */

    CVMgcClearClassMarks(ee, NULL);

    CVMgcScanRoots(ee, &gcOpts, CVMgcDumpReferencesCallBack, obj);

    CVMgcimplIterateHeap(ee, CVMgcDumpReferencesObjectCallback, obj);
}
#endif

/*
 * Heap dump related routines
 */

/*
 * Locks heap and dumps contents given a 'dumpFunc'
 */
static void
dumper(CVMExecEnv* ee, void (*dumpFunc)(CVMExecEnv* ee, void *data), void *data)
{
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
#ifdef CVM_JIT
    CVMSysMutex* jitLock = &CVMglobals.jitLock;
#endif

#ifdef CVM_JIT
    /* We need to become safe all to do the dump.  So, we need to enter the
     * jitLock first without blocking.  Refuse to dump heap if recursively
     * entering lock, or if another thread has already locked the lock.
     */
    if (!CVMsysMutexTryLock(ee, jitLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }
#endif

    /* Try to enter the heap lock without blocking. Refuse to dump heap
     * if recursively entering lock, or if another thread has already locked
     * the lock.
     */
    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        goto abort1;
    }

    /* We did obtain the lock without blocking. Now just make sure it's not a
       recursive entry.
     */
    if (CVMsysMutexCount(heapLock) > 1) {
        CVMconsolePrintf("Cannot dump heap -- "
                         "this thread already owns the heap lock!\n");
        goto abort2;
    }

    (*dumpFunc)(ee, data);

abort2:
    CVMsysMutexUnlock(ee, heapLock);
abort1:;
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, jitLock);
#endif
}

/*===========================================================================*/

/* Purpose: Worker function called indirectly by CVMgcDumpClassReferences()
            to dump all references to instances of the specified class. */
static CVMBool CVMgcObjectReferencesDumpAction(CVMExecEnv *ee, void *data)
{
    CVMObject *obj = (CVMObject *) data;

    if (!CVMgcIsValidObject(ee, obj)) {
        CVMconsolePrintf("Address 0x%x is not a valid object\n", obj);
        return CVM_FALSE;
    }

    CVMgcDumpReferences(obj);
    return CVM_TRUE;
}

/* Purpose: Worker function called indirectly by CVMgcDumpObjectReferences()
            to dump all references to the specified object. */
static void CVMgcObjectReferencesDump(CVMExecEnv* ee, void *data)
{
    CVMgcStopTheWorldAndDoAction(ee, data, NULL,
	CVMgcObjectReferencesDumpAction, NULL, NULL, NULL);
}

/*===========================================================================*/

#ifdef CVM_ROOT_FINDER
static CVMBool CVMgcObjectGCRootsDumpAction(CVMExecEnv *ee, void *data)
{
    CVMObject *obj = (CVMObject *) data;

    if (!CVMgcIsValidObject(ee, obj)) {
        CVMconsolePrintf("Address 0x%x is not a valid object\n", obj);
        return CVM_FALSE;
    }

    CVMgcDumpGCRoots(obj);
    return CVM_TRUE;
}

static void CVMgcObjectGCRootsDump(CVMExecEnv* ee, void *data)
{
    CVMgcStopTheWorldAndDoAction(ee, data, NULL,
        CVMgcObjectGCRootsDumpAction, NULL, NULL, NULL);
}
#endif

/*===========================================================================*/

/*
 * A dumper that dumps all objects of the specified type and the references to
 * them.
 */
static CVMBool
classReferenceDumpCallback(CVMObject* obj, CVMClassBlock* cb, CVMUint32 size,
                           void* data)
{
    CVMClassTypeID *clazznameID = (CVMClassTypeID *) data;
    if (CVMcbClassName(cb) == *clazznameID) {
        CVMconsolePrintf("Addr: 0x%x Size: %d  \tClass: %C\n", obj, size, cb);
        CVMgcDumpReferences(obj);
    }
    return CVM_TRUE;
}

/* Purpose: Worker function called indirectly by CVMgcDumpClassReferences()
            to dump all references to instances of the specified class. */
static CVMBool CVMgcClassReferencesDumpAction(CVMExecEnv *ee, void *data)
{
    CVMgcimplIterateHeap(ee, classReferenceDumpCallback, data);
    return CVM_TRUE;
}

/* Purpose: Worker function called indirectly by CVMgcDumpClassReferences()
            to dump all references to instances of the specified class. */
static void CVMgcClassReferencesDump(CVMExecEnv *ee, void *data)
{
    const char *clazzname = (const char *)data;
    char *newClazzname;
    size_t length;
    char *p;
    CVMClassTypeID clazznameID;

    /* Replace all '.' with '/' because the typeid system uses '/'s: */
    length = strlen(clazzname) + 1;
    newClazzname = malloc(length);
    strcpy(newClazzname, clazzname);
    p = newClazzname;
    VerifyFixClassname(newClazzname);

    /* Look up the classID for the specified class: */
    clazznameID = CVMtypeidLookupClassID(ee, newClazzname, (int)length - 1);
    free (newClazzname);

    if (clazznameID == CVM_TYPEID_ERROR) {
        CVMconsolePrintf("Class %s is NOT loaded\n", clazzname);
        return;
    }
    /* Go do the dump: */
    CVMgcStopTheWorldAndDoAction(ee, &clazznameID, NULL,
        CVMgcClassReferencesDumpAction, NULL, NULL, NULL);
}

/*===========================================================================*/

static void
classBlocksDumpCallback(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
{
    CVMClassTypeID *clazznameID = (CVMClassTypeID *) data;
    if (*clazznameID == CVMcbClassName(cb)) {
        CVMconsolePrintf("cb %p %C\n", cb, cb);
    }
}

/* Purpose: Worker function called indirectly by CVMgcDumpClassBlocks()
            to dump all classblocks of the specified class. */
static void CVMgcClassBlocksDump(CVMExecEnv *ee, void *data)
{
    const char *clazzname = (const char *)data;
    char *newClazzname;
    size_t length;
    char *p;
    CVMClassTypeID clazznameID;

    /* Replace all '.' with '/' because the typeid system uses '/'s: */
    length = strlen(clazzname) + 1;
    newClazzname = malloc(length);
    strcpy(newClazzname, clazzname);
    p = newClazzname;
    p = strchr(p, '.');
    while (p != NULL) {
        *p++ = '/';
        p = strchr(p, '.');
    }

    /* Look up the classID for the specified class: */
    clazznameID = CVMtypeidLookupClassID(ee, newClazzname, (int)length - 1);
    free (newClazzname);

    if (clazznameID == CVM_TYPEID_ERROR) {
        CVMconsolePrintf("Class %s is NOT loaded\n", clazzname);
        return;
    }
    CVMclassIterateAllClasses(ee, classBlocksDumpCallback, &clazznameID);
}

/*===========================================================================*/

/*
 * A dumper that counts objects on the heap
 */
static CVMBool
simpleDumpCallback(CVMObject* obj, CVMClassBlock* cb, CVMUint32 size,
		   void* data)
{
    CVMUint32* count = (CVMUint32*)data;
    *count += 1;
    return CVM_TRUE;
}

void
CVMgcSimpleHeapDump(CVMExecEnv* ee, void *data)
{
    CVMUint32 objCount;
    objCount = 0;
    CVMgcimplIterateHeap(ee, simpleDumpCallback, &objCount);
    CVMconsolePrintf("Counted %d objects\n", objCount);
}

/*
 * A dumper that dumps every object on the heap
 */
static CVMBool
verboseDumpCallback(CVMObject* obj, CVMClassBlock* cb, CVMUint32 size,
		   void* data)
{
    if (cb == CVMsystemClass(java_lang_Class)) {
        CVMAddr selfCBAddr;
        CVMClassBlock *selfCB;
	CVMD_fieldReadAddr(obj,
            CVMoffsetOfjava_lang_Class_classBlockPointer, selfCBAddr);
	selfCB = (CVMClassBlock *)selfCBAddr;
        CVMconsolePrintf("Addr: 0x%x Size: %d  \tClass: %C of %C\n",
			 obj, size, cb, selfCB);
    } else {
        CVMconsolePrintf("Addr: 0x%x Size: %d  \tClass: %C\n", obj, size, cb);
    }
    return CVM_TRUE;
}

void
CVMgcVerboseHeapDump(CVMExecEnv* ee, void *data)
{
    CVMgcimplIterateHeap(ee, verboseDumpCallback, NULL);
}

/*
 * A dumper that is used to output stats on heap objects
 */

/*
 * A per-object callback to determine the maximum TypeID of an object
 * on the heap.
 */
static CVMBool
findMaxTypeIDCallback(CVMObject* obj, CVMClassBlock* cb, CVMUint32 size,
                      void* data)
{
    CVMUint32* currMaxID = (CVMUint32*)data;
    if (CVMcbClassName(cb) > *currMaxID) {
	*currMaxID = CVMcbClassName(cb);
    }
    return CVM_TRUE;
}

/* 
 * The following table is used to hold instance stats. Currently it is
 * one large array, indexed by typeID: [0 .. maxTypeID). A first pass
 * optimization would remove the huge hole in the middle that is the
 * non-array -> array skip. Another pass would turn this into a
 * hashtable keyed by typeID. Left as an exercise to the reader, since
 * this is debugging-only code.  
 */
typedef struct {
    CVMUint32 totalSize; /* Total size consumed for this class */
    CVMUint32 numInstances; /* Total number of instances of this class */
    CVMClassTypeID typeID; /* We will sort this array, so record typeID here */
} TableEntry;

typedef struct {
    int  tableSize; /* Number of elements in the tables below */
    TableEntry* entries;
} Stats;

static CVMBool
statsCollectCallback(CVMObject* obj, CVMClassBlock* cb, CVMUint32 size,
		   void* data)
{
    CVMClassTypeID typeID = CVMcbClassName(cb);
    Stats*         stats  = (Stats*)data;
    TableEntry*    entry  = &stats->entries[typeID];

    entry->typeID        = typeID;
    entry->totalSize    += size;
    entry->numInstances += 1;
    return CVM_TRUE;
}

/*
 * Comparator for class stats table. This one sorts in descending total
 * memory usage
 */
static int 
tableEntrySizeCompare(const void *p1, const void *p2)
{
    TableEntry* e1 = (TableEntry*)p1;
    TableEntry* e2 = (TableEntry*)p2;
    return e2->totalSize - e1->totalSize;
}

/*
 * And now the heap stats dumper
 */
void
CVMgcStatsHeapDump(CVMExecEnv* ee, void *data)
{
    CVMUint32 maxTypeID = 0;
    int i;
    Stats stats;

    /* First look at all objects in the heap, and figure out the max TypeID
       value. We will use this to allocate stats tables */
    CVMgcimplIterateHeap(ee, findMaxTypeIDCallback, &maxTypeID);
    stats.tableSize = maxTypeID + 1;
    /*
     * OK, now allocate the stats table
     */
    stats.entries = (TableEntry *)calloc(sizeof(TableEntry), stats.tableSize);
    if (stats.entries == NULL) {
	CVMconsolePrintf("Out of memory allocating totalSize array\n");
	return;
    }
    CVMgcimplIterateHeap(ee, statsCollectCallback, &stats);
    /*
     * OK, now sort the table in decreasing order of memory consumption
     */
    qsort(stats.entries, stats.tableSize, sizeof(TableEntry), 
	  tableEntrySizeCompare);
    /*
     * And dump
     */
    for (i = 0; i < stats.tableSize; i++) {
	TableEntry* e = &stats.entries[i];
	char buf[256]; /* 256 chars max for the class name */
	if (e->numInstances == 0) {
	    continue;
	}
	CVMclassname2String(e->typeID, buf, 256);
	printf("TS=%-8d NI=%-8d CL=%s\n",
	       e->totalSize, e->numInstances, buf);
    }
    free(stats.entries);
}

/* Purpose: Dumps all references to the specified object. */
/* NOTE: The user should use CVMdumpObjectReferences() instead of calling this
         function directly.  CVMdumpObjectReferences() will set up the proper
         VM state and then calls this function to do the work. */
/* NOTE: Called while GC safe. */
void
CVMgcDumpObjectReferences(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));
    dumper(ee, CVMgcObjectReferencesDump, (void *) obj);
}

/* Purpose: Dumps all references to instances of the specified class. */
/* NOTE: The user should use CVMdumpClassReferences() instead of calling this
         function directly.  CVMdumpClassReferences() will set up the proper
         VM state and then calls this function to do the work. */
/* NOTE: Called while GC safe. */
void
CVMgcDumpClassReferences(const char *clazzname)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));
    dumper(ee, CVMgcClassReferencesDump, (void *) clazzname);
}

/* Purpose: Dumps all classblocks of the specified class. */
/* NOTE: The user should use CVMdumpClassBlocks() instead of calling this
         function directly.  CVMdumpClassBlocks() will set up the proper
         VM state and then calls this function to do the work. */
/* NOTE: Called while GC safe. */
void
CVMgcDumpClassBlocks(const char *clazzname)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));
    dumper(ee, CVMgcClassBlocksDump, (void *) clazzname);
}

/* Purpose: Dumps all GC roots that is keeping the specified object alive. */
/* NOTE: The user should use CVMdumpObjectGCRoots() instead of calling this
         function directly.  CVMdumpObjectGCRoots() will set up the proper
         VM state and then calls this function to do the work. */
/* NOTE: Called while GC safe. */
void
CVMgcDumpObjectGCRoots(CVMObject *obj)
{
#ifdef CVM_ROOT_FINDER
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));

    dumper(ee, CVMgcObjectGCRootsDump, (void *) obj);
#else
    CVMconsolePrintf("Currently not implemented yet.\n");
    return;
#endif
}


/*
 * Dumping heap contents. These routines are designed to be called
 * from the debugger, so they don't take an ee as argument.
 */
void
CVMgcDumpHeapSimple()
{
    dumper(CVMgetEE(), CVMgcSimpleHeapDump, NULL);
}

void
CVMgcDumpHeapVerbose()
{
    dumper(CVMgetEE(), CVMgcVerboseHeapDump, NULL);
}

void
CVMgcDumpHeapStats()
{
    dumper(CVMgetEE(), CVMgcStatsHeapDump, NULL);
}

/* ==========================================================================
 *  Heap State Capture analysis tool:
 */

static CVMHeapState *findHeapState(CVMUint32 id, CVMHeapState **prevPtr);
static void printHeapStateObject(CVMHeapStateObject *hso);
static int
heapStateObjectCompareObj(const void *p1, const void *p2);
static int
heapStateObjectCompareObjClass(const void *p1, const void *p2);
static CVMHeapState *
sortHeapState(CVMHeapState *chs, int (*compare)(const void *, const void *));

static CVMBool
captureHeapStateCountCallback(CVMObject* obj, CVMClassBlock* cb,
			      CVMUint32 size, void* data)
{
    CVMUint32* count = (CVMUint32*)data;
    *count += 1;
    return CVM_TRUE;
}

static CVMBool
captureHeapStateCallback(CVMObject* obj, CVMClassBlock* cb,
			 CVMUint32 size, void* data)
{
    CVMHeapState* chs = (CVMHeapState *)data;
    CVMUint32 index = chs->numberOfObjects++;
    CVMHeapStateObject *hso = &chs->objects[index];
    hso->obj = obj;
    hso->size = CVMobjectSize(obj);
    chs->totalSize += hso->size;
    return CVM_TRUE;
}

static void
captureHeapState(CVMExecEnv* ee, void *data)
{
    const char *name = (const char *)data;
    CVMUint32 objCount = 0;
    CVMHeapState *chs;

    /* Count the numer of objects: */
    CVMgcimplIterateHeap(ee, captureHeapStateCountCallback, &objCount);

    /* Allocate memory to capture the heap state: */
    chs = malloc(sizeof(CVMHeapState) +
		 ((objCount - 1) * sizeof(CVMHeapStateObject)));

    chs->id = ++CVMglobals.inspector.lastHeapStateID;
    chs->timeStamp = CVMlong2Int(CVMtimeMillis());
    chs->numberOfObjects = 0;
    chs->totalSize = 0;

    if (name != NULL) {
        chs->name = strdup(name);
    } else {
        /* 7 chars for format, 20 for integer value, and 1 for '\0': */
        chs->name = malloc(7 + 20 + 1);
        sprintf(chs->name, "*time %d*", chs->timeStamp);
    }

    CVMgcimplIterateHeap(ee, captureHeapStateCallback, chs);
    CVMassert(chs->numberOfObjects == objCount);

    /* Add the captured state to the global list: */
    chs->next = CVMglobals.inspector.heapStates;
    CVMglobals.inspector.heapStates = chs;
    CVMglobals.inspector.hasCapturedState = CVM_TRUE;
}

void  CVMgcCaptureHeapState(const char *name)
{
    dumper(CVMgetEE(), captureHeapState, (char *)name);
}

void CVMgcReleaseHeapState(CVMUint32 id)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
    CVMHeapState *chs;
    CVMHeapState *prev = NULL;

    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }

    /* Find and release the heapState: */
    chs = findHeapState(id, &prev);
    if (chs != NULL) {
	if (prev != NULL) {
	    prev->next = chs->next;
	} else {
	    CVMglobals.inspector.heapStates = chs->next;
	}
	CVMassert(chs->name != NULL);
	free(chs->name);
	free(chs);
	CVMconsolePrintf("Released heapState %d\n", id);
    } else {
        CVMconsolePrintf("Could not find heapState %d\n", id);
    }

    CVMsysMutexUnlock(ee, heapLock);
}

void CVMgcReleaseAllHeapState(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
    CVMHeapState *chs;

    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }

    /* Release all heapStates: */
    chs = CVMglobals.inspector.heapStates;
    while (chs != NULL) {
        CVMHeapState *next = chs->next;
	if (chs->name != NULL) {
	    free(chs->name);
	}
        free(chs);
	chs = next;
    }
    CVMglobals.inspector.heapStates = NULL;
    CVMconsolePrintf("Released all heapStates\n");

    CVMsysMutexUnlock(ee, heapLock);
}

void CVMgcListHeapStates(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
    CVMHeapState *chs;
    int i = 0;

    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }

    /* List all heapStates: */
    CVMconsolePrintf("List of captured heap states:\n");
    chs = CVMglobals.inspector.heapStates;
    if (chs == NULL) {
        CVMconsolePrintf("   none\n");
    }
    while (chs != NULL) {
	CVMassert(chs->name != NULL);
        CVMconsolePrintf("   hs %d: %s\n", chs->id, chs->name);
	i++;
	chs = chs->next;
    }

    CVMsysMutexUnlock(ee, heapLock);
}

void CVMgcDumpHeapState(CVMUint32 id, int sortKey)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
    CVMHeapState *chs;
    CVMUint32 i;

    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }

    /* Find the heapState: */
    chs = findHeapState(id, NULL);
    if (chs == NULL) {
        CVMconsolePrintf("Could not find heapState %d\n", id);
	goto abort;
    }

    /* Sort the heap state if necessary: */
    if (sortKey == CVM_HEAPSTATE_SORT_BY_OBJ) {
        chs = sortHeapState(chs, heapStateObjectCompareObj);
    } else if (sortKey == CVM_HEAPSTATE_SORT_BY_OBJCLASS) {
        chs = sortHeapState(chs, heapStateObjectCompareObjClass);
    }
    if (chs == NULL) {
        CVMconsolePrintf("Out of memory: could not sort heap state\n");
        goto abort;
    }

    /* Dump the heapState: */
    CVMconsolePrintf("Heap State %d: ", id);
    if (chs->name == NULL) {
        CVMconsolePrintf("*un-named* ");
    } else {
        CVMconsolePrintf("\"%s\"", chs->name);
    }
    if (sortKey == CVM_HEAPSTATE_SORT_BY_OBJ) {
	CVMconsolePrintf("sorted by obj");
    } else if (sortKey == CVM_HEAPSTATE_SORT_BY_OBJCLASS) {
	CVMconsolePrintf("sorted by obj class");
    } else {
	CVMconsolePrintf("not sorted");
    }
    CVMconsolePrintf("\n");
    CVMconsolePrintf("   timestamp: %d ms\n", chs->timeStamp);
    CVMconsolePrintf("   number of objects: %d\n", chs->numberOfObjects);
    CVMconsolePrintf("   total size: %d\n", chs->totalSize);
    for (i = 0; i < chs->numberOfObjects; i++) {
        CVMHeapStateObject *hso = &chs->objects[i];
        CVMconsolePrintf("   [%d] ", i);
	printHeapStateObject(hso);
        CVMconsolePrintf("\n");
    }

    /* Release the sorted heap state if appropriate: */
    if (sortKey != CVM_HEAPSTATE_SORT_NONE) {
        free(chs);
    }

abort:
    CVMsysMutexUnlock(ee, heapLock);
}

void CVMgcCompareHeapState(CVMUint32 id1, CVMUint32 id2)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMSysMutex* heapLock = &CVMglobals.heapLock;
    CVMHeapState *chs1;
    CVMHeapState *chs2;
    CVMUint32 i1, i2, diffs1, diffs2;
    CVMUint32 size1, size2;

    if (!CVMsysMutexTryLock(ee, heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the heap lock!\n");
        return;
    }

    /* Find and release the heapState: */
    chs1 = findHeapState(id1, NULL);
    if (chs1 == NULL) {
        CVMconsolePrintf("Could not find heapState %d\n", id1);
	goto abort;
    }
    chs2 = findHeapState(id2, NULL);
    if (chs2 == NULL) {
        CVMconsolePrintf("Could not find heapState %d\n", id2);
	goto abort;
    }

    /* Sort the heap states to simplify comparison: */
    chs1 = sortHeapState(chs1, heapStateObjectCompareObj);
    if (chs1 == NULL) {
        CVMconsolePrintf("Out of memory: could not sort heap state\n");
        goto abort;
    }
    chs2 = sortHeapState(chs2, heapStateObjectCompareObj);
    if (chs2 == NULL) {
        CVMconsolePrintf("Out of memory: could not sort heap state\n");
        goto abort1;
    }

    /* Compare the 2 heap states: */
    CVMconsolePrintf("Comparing heapStates %d and %d:\n", id1, id2);
    i1 = i2 = diffs1 = diffs2 = 0;
    size1 =  size2 = 0;
    while (i1 < chs1->numberOfObjects && i2 < chs2->numberOfObjects) {
        CVMHeapStateObject *hso1 = &chs1->objects[i1];
	CVMHeapStateObject *hso2 = &chs2->objects[i2];
	/* Skip all freed objects: */
	if (CVMheapStateObjectIsFreed(hso1)) {
	    i1++;
	    continue;
	}
	if (CVMheapStateObjectIsFreed(hso2)) {
	    i2++;
	    continue;
	}
	CVMassert(!CVMheapStateObjectIsFreed(hso1) &&
		  !CVMheapStateObjectIsFreed(hso2));
	if (hso2->obj < hso1->obj) {
	    /* The object in the 2nd list preceeds anything in the 1st.
	       It must be a mismatch.  Dump it and advance the 2nd list
	       until we catch up to the 1st list: */
	    CVMconsolePrintf("   hs %d: ", id2);
	    printHeapStateObject(hso2);
	    CVMconsolePrintf("\n");
	    size2 += hso2->size;
	    i2++;
	    diffs2++;
	} else if (hso1->obj < hso2->obj) {
	    /* The object in the 1st list preceeds anything in the 2nd.
	       It must be a mismatch.  Dump it and advance the 1st list
	       until we catch up to the 2nd list: */
	    CVMconsolePrintf("   hs %d: ", id1);
	    printHeapStateObject(hso1);
	    CVMconsolePrintf("\n");
	    size1 += hso1->size;
	    i1++;
	    diffs1++;
	} else {
	    /* We have a match.  Move on to next object in both lists: */
	    i1++;
	    i2++;
	}
    }

    /* Any remaining objects in the 1st list must be a mismatch: */
    while (i1 < chs1->numberOfObjects) {
        CVMHeapStateObject *hso1 = &chs1->objects[i1];
	/* Skip all freed objects: */
	if (CVMheapStateObjectIsFreed(hso1)) {
	    i1++;
	    continue;
	}
	CVMconsolePrintf("   hs %d: ", id1);
	printHeapStateObject(hso1);
	CVMconsolePrintf("\n");
	size1 += hso1->size;
	i1++;
	diffs1++;
    }

    /* Any remaining objects in the 2nd list must be a mismatch: */
    while (i2 < chs2->numberOfObjects) {
        CVMHeapStateObject *hso2 = &chs2->objects[i2];
	/* Skip all freed objects: */
	if (CVMheapStateObjectIsFreed(hso2)) {
	    i2++;
	    continue;
	}
	CVMconsolePrintf("   hs %d: ", id2);
	printHeapStateObject(hso2);
	CVMconsolePrintf("\n");
	size2 += hso2->size;
	i2++;
	diffs2++;
    }

    CVMconsolePrintf("Number of mismatches in heapState %d: %d (size %d)\n",
		     id1, diffs1, size1);
    CVMconsolePrintf("Number of mismatches in heapState %d: %d (size %d)\n",
		     id2, diffs2, size2);
    CVMconsolePrintf("Total number of mismatches: %d (size %d)\n",
		     diffs1 + diffs2, size1 + size2);
    CVMconsolePrintf("Size of heapState %d: %d\n", id1, chs1->totalSize);
    CVMconsolePrintf("Size of heapState %d: %d\n", id2, chs2->totalSize);
    CVMconsolePrintf("Size difference: %d\n",
		     chs2->totalSize - chs1->totalSize);

    /* Release the sorted heap states: */
    free(chs2);
abort1:
    free(chs1);
abort:
    CVMsysMutexUnlock(ee, heapLock);
}

/* NOTE: This is called from within a GC cycle.  So, no need to acquire the
   heapLock first. */
void CVMgcHeapStateObjectMoved(CVMObject *oldObj, CVMObject *newObj)
{
    CVMHeapState *chs;
    chs = CVMglobals.inspector.heapStates;
    while (chs != NULL) {
        CVMUint32 i;
	for (i = 0; i < chs->numberOfObjects; i++) {
	    CVMHeapStateObject *hso = &chs->objects[i];
	    if (hso->obj == oldObj) {
	        /* NOTE: An object can only exist once per captured heap
		   state.  Hence, once we've found it, we can move on to
		   the next heap state. */
	        hso->obj = newObj;
	        break;
	    }
	}
	chs = chs->next;
    }
}

/* NOTE: This is called from within a GC cycle.  So, no need to acquire the
   heapLock first. */
void CVMgcHeapStateObjectFreed(CVMObject *obj)
{
    CVMHeapState *chs;
    chs = CVMglobals.inspector.heapStates;
    while (chs != NULL) {
        CVMUint32 i;
	for (i = 0; i < chs->numberOfObjects; i++) {
	    CVMHeapStateObject *hso = &chs->objects[i];
	    if (hso->obj == obj) {
	        /* NOTE: An object can only exist once per captured heap
		   state.  Hence, once we've found it, we can move on to
		   the next heap state. */
	        CVMheapStateObjectMarkAsFreed(hso);
	        break;
	    }
	}
	chs = chs->next;
    }
}

/* Purpose: Find the specified heapState. */ 
static CVMHeapState *findHeapState(CVMUint32 id, CVMHeapState **prevPtr)
{
    CVMHeapState *chs = CVMglobals.inspector.heapStates;
    CVMHeapState *prev = NULL;
    while (chs != NULL) {
        if (chs->id == id) {
	    break;
	}
	prev = chs;
	chs = chs->next;
    }
    if (prevPtr != NULL) {
        *prevPtr = prev;
    }
    return chs;
}

static void printHeapStateObject(CVMHeapStateObject *hso)
{
    if (CVMheapStateObjectIsFreed(hso)) {
        CVMconsolePrintf("size %d: *GCed* 0x%x", hso->size, (int)hso->obj);
    } else {
        CVMClassBlock *cb = CVMobjectGetClass(hso->obj);
	CVMconsolePrintf("size %d: 0x%x ", hso->size, (int)hso->obj);
	if (cb == CVMsystemClass(java_lang_Class)) {
	    CVMAddr selfCBAddr;
	    CVMClassBlock *selfCB;
	    CVMD_fieldReadAddr(hso->obj,
		CVMoffsetOfjava_lang_Class_classBlockPointer, selfCBAddr);
	    selfCB = (CVMClassBlock *)selfCBAddr;
	    CVMconsolePrintf("%O of %C ********************************",
			     hso->obj, selfCB);
	} else {
	    CVMconsolePrintf("%O", hso->obj);
	}
    }
}

static int
heapStateObjectCompareObj(const void *p1, const void *p2)
{
    CVMHeapStateObject *hso1 = (CVMHeapStateObject *)p1;
    CVMHeapStateObject *hso2 = (CVMHeapStateObject *)p2;
    return ((CVMAddr)hso1->obj - (CVMAddr)hso2->obj);
}

static int
heapStateObjectCompareObjClass(const void *p1, const void *p2)
{
    CVMHeapStateObject *hso1 = (CVMHeapStateObject *)p1;
    CVMHeapStateObject *hso2 = (CVMHeapStateObject *)p2;
    CVMClassBlock *cb1 = NULL;
    CVMClassBlock *cb2 = NULL;

    if (!CVMheapStateObjectIsFreed(hso1)) {
        cb1 = CVMobjectGetClass(hso1->obj);
    }
    if (!CVMheapStateObjectIsFreed(hso2)) {
        cb2 = CVMobjectGetClass(hso2->obj);
    }
    /* First sort by class: */
    if (cb1 != cb2) {
        return ((CVMAddr)cb1 - (CVMAddr)cb2);
    }
    /* Next sort by object pointer: */
    return ((CVMAddr)hso1->obj - (CVMAddr)hso2->obj);
}

static CVMHeapState *
sortHeapState(CVMHeapState *chs, int (*compare)(const void *, const void *))
{
    CVMHeapState *newChs;
    size_t length = sizeof(CVMHeapState) +
                    ((chs->numberOfObjects - 1) * sizeof(CVMHeapStateObject));
    newChs = malloc(length);
    if (newChs == NULL) {
        return NULL;
    }
    memcpy(newChs, chs, length);
    chs = newChs;
    qsort(&chs->objects[0], chs->numberOfObjects, sizeof(chs->objects[0]),
	  compare);
    return chs;
}

/* ==========================================================================
 *  Dumpers:
 */

/* Purpose: Dumps the type and fields of the specified object. */
static void CVMdumpInstanceInternal(CVMObject *obj)
{
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    CVMClassBlock *cb0;
    CVMUint32 fieldsCount;

    CVMconsolePrintf("Object 0x%x: instance of class %C\n", obj, cb);
    CVMconsolePrintf("   classblock = 0x%x\n", cb);
    CVMconsolePrintf("   size = %d\n", CVMobjectSizeGivenClass(obj, cb));

    /* Dump the field values: */
    fieldsCount = 0;
    for (cb0 = cb; cb0 != NULL; cb0 = CVMcbSuperclass(cb0)) {
        CVMInt32 i;
        for (i = CVMcbFieldCount(cb0); --i >= 0; ) {
            CVMFieldBlock *fb = CVMcbFieldSlot(cb0, i);
            if (!CVMfbIs(fb, STATIC)) {
                fieldsCount++;
            }
        }
    }
    CVMconsolePrintf("   fields[%d] = {", fieldsCount);
    if (fieldsCount == 0) {
        CVMconsolePrintf(" NONE }\n");
    } else {
        CVMconsolePrintf("\n");
        for (cb0 = cb; cb0 != NULL; cb0 = CVMcbSuperclass(cb0)) {
            CVMInt32 i;
            /* We're iterating in reverse on purpose.  This results in dumping
               the last field first and the first field last (which makes it
               easy to travers the inheritance chain all the way up to the
               root superclass: */
            for (i = CVMcbFieldCount(cb0); --i >= 0; ) {
                CVMFieldBlock *fb = CVMcbFieldSlot(cb0, i);
                CVMUint32 fieldOffset; /* word offset for next field */
                CVMClassTypeID fieldType;
                if (CVMfbIs(fb, STATIC)) {
                    continue;
                }

                /* Get the field offset: */
                fieldOffset = CVMfbOffset(fb);

                fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
                if (CVMtypeidFieldIsRef(fieldType)) {
                    CVMObject *value;
                    CVMD_fieldReadRef(obj, CVMfbOffset(fb), value);
                    CVMconsolePrintf("      [%04x] 0x%x : %F\n",
                                     fieldOffset, value, fb);

                } else {
                    switch (fieldType) {
                        case CVM_TYPEID_LONG: {
                            CVMJavaLong value, highLong;
                            CVMUint32 high, low;
                            CVMD_fieldReadLong(obj, CVMfbOffset(fb), value);
                            highLong = CVMlongShr(value, 32);
                            high = CVMlong2Int(highLong);
                            low = CVMlong2Int(value);
                            CVMconsolePrintf("      [%04x] 0x%08x%08x : %F\n",
                                             fieldOffset, high, low, fb);
                            break;
                        }
                        case CVM_TYPEID_DOUBLE: {
                            CVMJavaDouble value;
                            CVMD_fieldReadDouble(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %g : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                        case CVM_TYPEID_INT: {
                            CVMJavaInt value;
                            CVMD_fieldReadInt(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %d : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                        case CVM_TYPEID_FLOAT: {
                            CVMJavaFloat value;
                            CVMD_fieldReadFloat(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %f : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                        case CVM_TYPEID_SHORT: {
                            CVMJavaShort value;
                            CVMD_fieldReadInt(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %d : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                        case CVM_TYPEID_CHAR: {
                            CVMJavaChar value;
                            CVMD_fieldReadInt(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %d(%c) : %F\n",
                                             fieldOffset, value, (char)value, fb);
                            break;
                        }
                        case CVM_TYPEID_BYTE: {
                            CVMJavaByte value;
                            CVMD_fieldReadInt(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %d : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                        case CVM_TYPEID_BOOLEAN: {
                            CVMJavaBoolean value;
                            CVMD_fieldReadInt(obj, CVMfbOffset(fb), value);
                            CVMconsolePrintf("      [%04x] %d : %F\n",
                                             fieldOffset, value, fb);
                            break;
                        }
                    }
                }
            }
        }
        CVMconsolePrintf("   }\n");
    }
}

#undef DUMP_ELEMENTS
#define DUMP_ELEMENTS(valueType_, type_, count_, reportAction_) {   \
        CVMArrayOf##type_ *array_ = (CVMArrayOf##type_ *) array;    \
        jint i, j;                                                  \
        for (i = 0; i < length; ) {                                 \
            if (i == 0) {                                           \
                CVMconsolePrintf(" { ");                            \
            } else {                                                \
                CVMconsolePrintf("   ");                            \
            }                                                       \
            for (j = 0; j < count_ && i < length; j++, i++) {       \
                valueType_ value;                                   \
                CVMD_arrayRead##type_(array_, i, value);            \
                reportAction_;                                      \
            }                                                       \
            if (i < length) {                                       \
                CVMconsolePrintf("\n");                             \
            } else {                                                \
                CVMconsolePrintf(" }\n");                           \
            }                                                       \
        }                                                           \
    }

#undef DUMP_PRIMITIVE_ELEMENTS
#define DUMP_PRIMITIVE_ELEMENTS(type_, count_, reportAction_) \
    DUMP_ELEMENTS(CVMJava##type_, type_, count_, reportAction_)

/* Purpose: Dump an object array. */
/* NOTE: Should be called while GC is disabled. */
static void CVMdumpObjArrayInternal(CVMObject *obj)
{
    CVMArrayOfAnyType *array = (CVMArrayOfAnyType *) obj;
    jint length = CVMD_arrayGetLength(array);
    CVMClassBlock *cb = CVMobjectGetClass(obj);

    CVMconsolePrintf("Object 0x%x: array of %C\n", obj, CVMarrayElementCb(cb));
    CVMconsolePrintf("   classblock = 0x%x\n", cb);
    CVMconsolePrintf("   size = %d\n", CVMobjectSizeGivenClass(obj, cb));
    CVMconsolePrintf("   arrayLength = %d\n", length);
    DUMP_ELEMENTS(CVMObject *, Ref, 4, {
        CVMconsolePrintf("0x%08x, ", value);
    });
}

/* Purpose: Dumps a primitive array. */
/* NOTE: Should be called while GC is disabled or the current thread is in a
         GC unsafe state. */
static void CVMdumpPrimitiveArrayInternal(CVMObject *obj)
{
    CVMArrayOfAnyType *array = (CVMArrayOfAnyType *) obj;
    jint length = CVMD_arrayGetLength(array);
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    const char *primitiveTypeStr = 0;

    switch(CVMarrayBaseType(cb)) {
        case CVM_T_BOOLEAN: primitiveTypeStr = "BOOLEAN";   break;
        case CVM_T_BYTE:    primitiveTypeStr = "BYTE";      break;
        case CVM_T_CHAR:    primitiveTypeStr = "CHAR";      break;
        case CVM_T_SHORT:   primitiveTypeStr = "SHORT";     break;
        case CVM_T_INT:     primitiveTypeStr = "INT";       break;
        case CVM_T_LONG:    primitiveTypeStr = "LONG";      break;
        case CVM_T_FLOAT:   primitiveTypeStr = "FLOAT";     break;
        case CVM_T_DOUBLE:  primitiveTypeStr = "DOUBLE";    break;
        default: CVMassert(CVM_FALSE);
    }

    CVMconsolePrintf("Object 0x%x: array of %s\n", obj, primitiveTypeStr);
    CVMconsolePrintf("   classblock = 0x%x\n", cb);
    CVMconsolePrintf("   size = %d\n", CVMobjectSizeGivenClass(obj, cb));
    CVMconsolePrintf("   arrayLength = %d\n", length);

    switch (CVMarrayBaseType(cb)) {
        case CVM_T_BOOLEAN:
            DUMP_PRIMITIVE_ELEMENTS(Boolean, 8, {
                CVMconsolePrintf("%s ",
                    (value == CVM_FALSE) ? "false," : "true, ");
            });
            break;
        case CVM_T_BYTE:
            DUMP_PRIMITIVE_ELEMENTS(Byte, 8, {
                CVMconsolePrintf("%02d, ", value);
            });
            break;
        case CVM_T_SHORT:
            DUMP_PRIMITIVE_ELEMENTS(Short, 8, {
                CVMconsolePrintf("%04d, ", value);
            });
            break;
        case CVM_T_CHAR:
            DUMP_PRIMITIVE_ELEMENTS(Char, 4, {
                CVMconsolePrintf("0x%04x(%c), ", value, (char)value);
            });
            break;
        case CVM_T_INT:
            DUMP_PRIMITIVE_ELEMENTS(Int, 4, {
                CVMconsolePrintf("0x%08x, ", value);
            });
            break;
        case CVM_T_FLOAT:
            DUMP_PRIMITIVE_ELEMENTS(Float, 4, {
                CVMconsolePrintf("%f, ", value);
            });
            break;
        case CVM_T_LONG:
            DUMP_PRIMITIVE_ELEMENTS(Long, 2, {
                CVMJavaLong highLong = CVMlongShr(value, 32);
                CVMUint32 high = CVMlong2Int(highLong);
                CVMUint32 low = CVMlong2Int(value);
                CVMconsolePrintf("0x%08x08x, ", high, low);
            });
            break;
        case CVM_T_DOUBLE:
            DUMP_PRIMITIVE_ELEMENTS(Double, 2, {
                CVMconsolePrintf("%g, ", value);
            });
            break;
        default:
            CVMassert(CVM_FALSE);
    }
}

#undef DUMP_PRIMITIVE_ELEMENTS
#undef DUMP_ELEMENTS

/* Purpose: Dumps all the type and fields of an object. */
void CVMdumpObject(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock *cb;

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (!CVMgcIsValidObject(ee, obj)) {
        CVMconsolePrintf("Address 0x%x is not a valid object\n", obj);
        return;
    }

    cb = CVMobjectGetClass(obj);
    if (CVMisArrayClass(cb)) {
        if (CVMarrayElemTypeCode(cb) == CVM_T_CLASS) {
            CVMdumpObjArrayInternal(obj);
        } else {
            CVMdumpPrimitiveArrayInternal(obj);
        }
    } else {
        CVMdumpInstanceInternal(obj);
    }
}

/* Purpose: Dump an instance of java.lang.Class. */
void CVMdumpClassBlock(CVMClassBlock *cb)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMInt32 staticFieldsCount;
    CVMObject *clazz;

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (!CVMclassIsValidClassBlock(ee, cb)) {
        CVMconsolePrintf("Address 0x%x is not a valid Classblock\n", cb);
        return;
    }

    clazz = *(CVMObject **) CVMcbJavaInstance(cb);
    CVMconsolePrintf("Classblock 0x%x: class %C\n", cb, cb);
    CVMconsolePrintf("   class object = 0x%x\n", clazz);
    if (!CVMisArrayClass(cb)) {
        CVMconsolePrintf("   instance size = %d\n", CVMcbInstanceSize(cb));
    }

    if (!CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED)) {
        CVMconsolePrintf("Class NOT fully initialized yet\n");
        return;
    }

    {
        CVMClassBlock *superCB = CVMcbSuperclass(cb);
        if (superCB != NULL) {
            CVMconsolePrintf("   superclass = 0x%x : %C\n", superCB, superCB);
        } else {
            CVMconsolePrintf("   superclass = 0x%x :\n", superCB);
        }
    }
#ifdef CVM_CLASSLOADING
    {
        CVMObjectICell *classloader;
        classloader = CVMcbClassLoader(cb);
        if (classloader != NULL) {
            CVMObject *classloaderObj;
	    /*
	     * Access member variable of type 'int'
	     * and cast it to a native pointer. The java type 'int' only garanties 
	     * 32 bit, but because one slot is used as storage space and
	     * a slot is 64 bit on 64 bit platforms, it is possible 
	     * to store a native pointer without modification of
	     * java source code. This assumes that all places in the C-code
	     * are catched which set/get this member.
	     */
            CVMAddr temp;
            CVMD_fieldReadAddr(*(CVMObject **)classloader,
			       CVMoffsetOfjava_lang_ClassLoader_loaderGlobalRoot,
			       temp);
            classloader = (CVMObjectICell *) temp;
            classloaderObj = (classloader != NULL) ?
                                    *(CVMObject **)classloader: NULL;
            CVMconsolePrintf("   classloader ref= 0x%x, obj= 0x%x : %O\n",
                             classloader, classloaderObj, classloaderObj);
        } else {
            CVMconsolePrintf("   classloader ref= 0x0, obj= 0x0 :\n");
        }
    }
#else
    CVMconsolePrintf("   classloader ref= 0x0, obj= 0x0 :\n");
#endif
    {
        CVMObjectICell *protectionDomain = CVMcbProtectionDomain(cb);
        CVMObject *pdobj = (protectionDomain != NULL) ?
                           *(CVMObject **)protectionDomain : NULL;
        if (pdobj != NULL) {
            CVMconsolePrintf("   protectionDomain = 0x%x : %O\n", pdobj, pdobj);
        } else {
            CVMconsolePrintf("   protectionDomain = 0x%x\n", pdobj);
        }
    }
    CVMconsolePrintf("   instanceSize = %d\n", CVMcbInstanceSize(cb));

    /* Dump static field values */
    staticFieldsCount = 0;
    if ((CVMcbFieldCount(cb) > 0)) {
        CVMInt32 i;
        CVMassert(CVMcbFields(cb) != NULL);
        for (i = CVMcbFieldCount(cb); --i >= 0; ) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            if (CVMfbIs(fb, STATIC)) {
                staticFieldsCount++;
            }
        }
    }

    CVMconsolePrintf("   static fields [%d] = {", staticFieldsCount);
    if (staticFieldsCount == 0) {
        CVMconsolePrintf(" NONE }\n");
    } else if (staticFieldsCount > 0) {
        CVMInt32 i;

        CVMconsolePrintf("\n");
        for (i = CVMcbFieldCount(cb); --i >= 0; ) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            CVMUint32 fieldOffset; /* word offset for next field */
            CVMClassTypeID fieldType;

            if (!CVMfbIs(fb, STATIC)) {
                continue;
            }

            fieldOffset = CVMfbOffset(fb);

            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (CVMtypeidFieldIsRef(fieldType)) {
                CVMObject *value = *(CVMObject **)&CVMfbStaticField(ee, fb).r;
                CVMconsolePrintf("      [0x%04x] 0x%x : %F\n",
                                 CVMfbOffset(fb), value, fb);

            } else {
                switch (fieldType) {
                    case CVM_TYPEID_LONG: {
                        CVMJavaLong value, highLong;
                        CVMUint32 high, low;
                        value = CVMjvm2Long(&CVMfbStaticField(ee, fb).raw);
                        highLong = CVMlongShr(value, 32);
                        high = CVMlong2Int(highLong);
                        low = CVMlong2Int(value);
                        CVMconsolePrintf("      [0x%04x] 0x%08x%08x : %F\n",
                                         fieldOffset, high, low, fb);
                        break;
                    }
                    case CVM_TYPEID_DOUBLE: {
                        CVMJavaDouble value;
                        value = CVMjvm2Double(&CVMfbStaticField(ee, fb).raw);
                        CVMconsolePrintf("      [0x%04x] %g : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                    case CVM_TYPEID_INT: {
                        CVMJavaInt value;
                        value = CVMfbStaticField(ee, fb).i;
                        CVMconsolePrintf("      [0x%04x] %d : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                    case CVM_TYPEID_FLOAT: {
                        CVMJavaFloat value;
                        value = CVMfbStaticField(ee, fb).f;
                        CVMconsolePrintf("      [0x%04x] %f : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                    case CVM_TYPEID_SHORT: {
                        CVMJavaShort value;
                        value = CVMfbStaticField(ee, fb).i;
                        CVMconsolePrintf("      [0x%04x] %d : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                    case CVM_TYPEID_CHAR: {
                        CVMJavaChar value;
                        value = CVMfbStaticField(ee, fb).i;
                        CVMconsolePrintf("      [0x%04x] %d(%c) : %F\n",
                                         fieldOffset, value, (char)value, fb);
                        break;
                    }
                    case CVM_TYPEID_BYTE: {
                        CVMJavaByte value;
                        value = CVMfbStaticField(ee, fb).i;
                        CVMconsolePrintf("      [0x%04x] %d : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                    case CVM_TYPEID_BOOLEAN: {
                        CVMJavaBoolean value;
                        value = CVMfbStaticField(ee, fb).i;
                        CVMconsolePrintf("      [0x%04x] %d : %F\n",
                                         fieldOffset, value, fb);
                        break;
                    }
                }
            }
        }
        CVMconsolePrintf("   }\n");
    }
}

/* Purpose: Dumps the content of the specified string. */
/* NOTE: No i18n support i.e. assumes that the string is an ASCII string. */
void CVMdumpString(CVMObject *string)
{
    CVMExecEnv *ee = CVMgetEE();
    char *str;
    CVMInt32 len, offset, i;
    CVMObject *value;
    CVMArrayOfChar *array;
    jsize length = 0;

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (!CVMgcIsValidObject(ee, string)) {
        CVMconsolePrintf("Address 0x%x is not a valid object\n", string);
        return;
    }

    if (CVMobjectGetClass(string) != CVMsystemClass(java_lang_String)) {
        CVMconsolePrintf("Error: Object 0x%x  is not an instance of java.lang.String\n");
        CVMconsolePrintf("       It is an instance of %C\n", CVMobjectGetClass(string));
        return;
    }

    CVMD_fieldReadRef(string, CVMoffsetOfjava_lang_String_value, value);
    CVMD_fieldReadInt(string, CVMoffsetOfjava_lang_String_count, len);
    CVMD_fieldReadInt(string, CVMoffsetOfjava_lang_String_offset, offset);

    array = (CVMArrayOfChar *) value;

    /* Calculate the length: */
    for (i = offset; i < len + offset; i++) {
        CVMJavaChar c;
        CVMD_arrayReadChar(array, i, c);
        length += CVMunicodeChar2UtfLength(c);
    }
    str = (char *)malloc(length + 1);

    /* Convert to UTF8 and print: */
    CVMutfCopyFromCharArray((const CVMJavaChar*)&array->elems[offset],
			    str, len);
    str[length] = '\0';
    CVMconsolePrintf("String 0x%x: length= %d\n"
                     "   value= \"%s\"\n", string, len, str);
    free(str);
}

/* Purpose: Dumps all references to the specified object. */
void CVMdumpObjectReferences(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    extern void CVMgcDumpObjectReferences(CVMObject *obj);

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (CVMD_isgcSafe(ee)) {
        CVMgcDumpObjectReferences(obj);
    } else {
        CVMD_gcSafeExec(ee, {
            CVMgcDumpObjectReferences(obj);
        });
    }
}

/* Purpose: Dumps all references to instances of the specified class. */
void CVMdumpClassReferences(const char *clazzname)
{
    CVMExecEnv *ee = CVMgetEE();

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (CVMD_isgcSafe(ee)) {
        CVMgcDumpClassReferences(clazzname);
    } else {
        CVMD_gcSafeExec(ee, {
            CVMgcDumpClassReferences(clazzname);
        });
    }
}

/* Purpose: Dumps all classblocks of the specified class. */
void CVMdumpClassBlocks(const char *clazzname)
{
    CVMExecEnv *ee = CVMgetEE();

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (CVMD_isgcSafe(ee)) {
        CVMgcDumpClassBlocks(clazzname);
    } else {
        CVMD_gcSafeExec(ee, {
            CVMgcDumpClassBlocks(clazzname);
        });
    }
}

/* Purpose: Dumps all GC Roots keeping this object alive. */
void CVMdumpObjectGCRoots(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();

    if (!CVMgcIsDisabled() && !CVMgcIsGCThread(ee)) {
        CVMconsolePrintf("You need to disable GC using CVMgcDisableGC()"
                         " before calling this function!\n");
        return;
    }

    if (!CVMgcIsValidObject(ee, obj)) {
        CVMconsolePrintf("Address 0x%x is not a valid object\n", obj);
        return;
    }

    if (CVMD_isgcSafe(ee)) {
        CVMgcDumpObjectGCRoots(obj);
    } else {
        CVMD_gcSafeExec(ee, {
            CVMgcDumpObjectGCRoots(obj);
        });
    }
}

/* Purpose: Dumps misc system informaion. */
void CVMdumpSysInfo()
{
    CVMconsolePrintf("CVM Configuration:\n");
    CVMdumpGlobalsSubOptionValues();
    CVMgcDumpSysInfo();
#ifdef CVM_JIT
    CVMjitDumpSysInfo();
#endif
}

#endif /* CVM_INSPECTOR */
