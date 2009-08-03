/*
 * @(#)gc_impl.c	1.61 06/10/10
 *
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

/*
 * This file includes the implementation of a simple,
 * semispace copying collector.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/porting/time.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/semispace/semispace.h"

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/*
 * Time of the last major GC
 */
static CVMInt64 lastMajorGCTime;

/*
 * Define to GC on every n-th allocation try
 */
/*#define CVM_SS_GC_ON_EVERY_ALLOC*/
#define CVM_SS_NO_ALLOCS_UNTIL_GC 1

/*
 * GC when 1 / CVM_SS_GC_THRESHOLD remains of the current semispace
 */
#define CVM_SS_GC_THRESHOLD 8

/*
 * A quick way to get to the heap
 */
static CVMSsHeap* theHeap = 0;

#ifdef CVM_JVMPI
/* The number of live objects at the end of the GC cycle: */
static CVMUint32 liveObjectCount;
#endif

/*
 * The quick access to the allocation semispace
 */
static CVMUint32* allocPtr;
static CVMUint32* allocBase;
static CVMUint32* allocTop;

/*
 * The threshold at which to GC
 */
static CVMUint32* allocThreshold;

/*
 * The pointers used for the Cheney style copying. After the root
 * scan, [copyBase, copyTop) includes all root objects. We copy all
 * live objects starting from copyBase. We increment copyTop when a
 * new object is copied. Copying is all done when copyBase == copyTop.
 */
static CVMUint32* copyBase;
static CVMUint32* copyTop;

/*
 * Initialize GC global state
 */
void
CVMgcimplInitGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("GC: Initializing global state for semi-space copying GC\n"));
    theHeap = 0;
    allocPtr = 0;
    allocBase = 0;
    allocTop = 0;
    allocThreshold = 0;

    copyBase = 0;
    copyTop = 0;
}

/*
 * The 'forwarded' array maps each allocated word to a bit.
 *
 * For a given "byte index" byteIdx_ in the heap, CVM_SS_BITMAP_IDX
 * finds the corresponding bitmap word for byteIdx_, and
 * CVM_SS_BITMAP_MASK computes the appropriate bit mask.  
 */

#define CVM_SS_BITMAP_IDX(byteIdx_)  ((byteIdx_) >> 7)
#define CVM_SS_BITMAP_MASK(byteIdx_) (1 << (((byteIdx_) & 0x7f) / 4))

/*
 * Set bit corresponding to heap addr 'addr' in the bits array 
 * (the forwarded array)
 */
static void
CVMssSetBitFor(CVMUint32* heapPtr, CVMUint32* bitmap)
{
    CVMUint32 byteIdx;

    CVMassert((heapPtr >= allocBase) &&
	      (heapPtr <  allocTop));

    byteIdx = (CVMUint8*)heapPtr - (CVMUint8*)allocBase;
    bitmap[CVM_SS_BITMAP_IDX(byteIdx)] |= CVM_SS_BITMAP_MASK(byteIdx);
}

/*
 * Check if bit corresponding to heap addr 'addr' in a bits array is marked.
 */
static CVMBool
CVMssMarkedIn(CVMUint32* heapPtr, CVMUint32* bitmap)
{
    CVMUint32 byteIdx;

    CVMassert((heapPtr >= allocBase) &&
	      (heapPtr <  allocTop));

    byteIdx = (CVMUint8*)heapPtr - (CVMUint8*)allocBase;
    return ((bitmap[CVM_SS_BITMAP_IDX(byteIdx)] &
	     CVM_SS_BITMAP_MASK(byteIdx)) != 0);
}

static CVMSsSemispace*
CVMssAllocSemiSpace(CVMUint32 noBytes)
{
    CVMUint32 allocSize =
	sizeof(CVMSsSemispace) - sizeof(CVMUint32) + noBytes;
    CVMSsSemispace* space = (CVMSsSemispace*)malloc(allocSize);
    CVMUint32 noWords = noBytes / 4;
    if (space != 0) {
	space->topData   = &space->data[noWords];
	space->allocPtr  = &space->data[0];
	/* %comment:f008 */
	space->threshold = space->topData - noWords / CVM_SS_GC_THRESHOLD;
    }
    return space;
}

static void
CVMssMakeSpaceCurrent(CVMSsSemispace* space)
{
    allocPtr       = space->allocPtr;
    allocBase      = space->data;
    allocTop       = space->topData;
    allocThreshold = space->threshold;
}

/*
 * Initialize a heap of 'heapSize' bytes. heapSize is guaranteed to be
 * divisible by 4.  */
/* Returns: CVM_TRUE if successful, else CVM_FALSE. */
CVMBool
CVMgcimplInitHeap(CVMGCGlobalState* globalState, 
		  CVMUint32 startSize,
		  CVMUint32 minHeapSize,
		  CVMUint32 maxHeapSize,
		  CVMBool startIsUnspecified,
		  CVMBool minIsUnspecified,
		  CVMBool maxIsUnspecified)
{
    CVMSsHeap* heap  = (CVMSsHeap*)calloc(sizeof(CVMSsHeap), 1);
    CVMSsSemispace* fromSpace;
    CVMSsSemispace* toSpace;
    CVMUint32 heapSize = maxHeapSize;

    if (heap == 0) {
	return CVM_FALSE;
    }
    /*
     * The area for the objects. The initialization code must have
     * rounded heapSize up to the nearest double-word multiple.
     */
    CVMassert(heapSize % 8 == 0);
    heap->size    = heapSize;
    fromSpace     = CVMssAllocSemiSpace(heapSize);
    toSpace       = CVMssAllocSemiSpace(heapSize);
    
    if ((fromSpace == 0) || (toSpace == 0)) {
        free(heap);
	free(fromSpace);
	free(toSpace);
	return CVM_FALSE;
    }

    /*
     * Initialize heap. 
     */
    heap->from = fromSpace;
    heap->to   = toSpace;
    
    /* The following need a bit per word allocated */
    heap->sizeOfBitmaps    = (heapSize + 31) / 32;
    heap->forwarded        = (CVMUint32*)calloc(heap->sizeOfBitmaps, 1);

    if (heap->forwarded == 0) {
	free(heap);
	free(fromSpace);
	free(toSpace);
	return CVM_FALSE;
    }

#ifdef CVM_JVMPI
    liveObjectCount = 0;
#endif

    /*
     * Now make the heap visible to the outside world.
     */
    globalState->heap = heap;
    theHeap = heap;

    /*
     * Start the world on fromSpace
     */
    CVMssMakeSpaceCurrent(fromSpace);

    /*
     * Initialize GC times. Let heap initialization be the first
     * major GC.
     */
    lastMajorGCTime = CVMtimeMillis();

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaNewEvent();
#endif

    CVMtraceMisc(("GC: Initialized heap for semi-space copying GC\n"));
    return CVM_TRUE;
}

/*
 * The alloc* pointers are still pointing to the old semispace at
 * GC time.
 */
static CVMBool
CVMssInOldSemispace(CVMObject* ref)
{
    CVMUint32* ptr = (CVMUint32*)ref;
    return ((ptr >= allocBase) && (ptr < allocTop));
}

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Checks to see if the specified object pointer is in the range of
            the new semispace (i.e. the copy space). */
static CVMBool
CVMssInNewSemispace(CVMObject *ref)
{
    CVMUint32* ptr = (CVMUint32*)ref;
    return ((ptr >= copyBase) && (ptr < copyTop));
}
#endif

static CVMObject*
CVMssGetForwardingPtr(CVMObject* ref)
{
    CVMUint32* objData = (CVMUint32*)ref;   /* The old data for object */
    CVMassert(CVMssInOldSemispace(ref));
    CVMassert(CVMssMarkedIn(objData, theHeap->forwarded));
    return (CVMObject*)objData[0];
}

/*
 * Copy numBytes from 'source' to 'dest'. Assume word copy, and assume
 * that the copy regions are disjoint.  
 */
static void
CVMssCopyDisjointWords(CVMUint32* dest, CVMUint32* source, CVMUint32 numBytes)
{
    CVMUint32* sourceEnd = source + numBytes / 4;
    while (source < sourceEnd) {
	*dest++ = *source++;
    }
}

/*
 * Copy object, leave forwarding pointer behind
 */
static CVMObject*
CVMssForwardObject(CVMObject* ref)
{
    CVMUint32 objSize = CVMobjectSize(ref); 
    CVMObject* ret = (CVMObject*)copyTop;   /* The copy destination */
    CVMUint32* objData = (CVMUint32*)ref;   /* The old data for object */

    CVMtraceGcCollect(("GC: Forwarding object %x (len %d, class %C), to %x\n",
		       ref, objSize, CVMobjectGetClass(ref), copyTop));
    CVMssCopyDisjointWords(copyTop, objData, objSize);
    copyTop += objSize / 4;

#ifdef CVM_JVMPI
    if (CVMjvmpiEventObjectMoveIsEnabled()) {
        CVMjvmpiPostObjectMoveEvent(1, objData, 1, copyTop);
    }
#endif
    /*
     * Set forwarding pointer
     */
    CVMssSetBitFor(objData, theHeap->forwarded);
    objData[0] = (CVMUint32)ret;
    
    return ret;
}

/*
 * Gray an object known to be in the old semispace
 */
static void
CVMssGrayObject(CVMObject** refPtr)
{
    CVMObject* ref = *refPtr;

    CVMtraceGcCollect(("GC: Graying object %x\n", ref));
    /* 
     * Don't deal with ROM objects
     */
    if (CVMobjectIsInROM(ref)) {
	CVMtraceGcCollect(("GC: Ignoring ROM object %x\n", ref));
	return;
    }

    /* we should not be trying to gray things already pointing to the
       new semispace. */
    CVMassert(CVMssInOldSemispace(ref));

    /*
     * If the object is in old space it might already be
     * forwarded. Check that. If it has been forwarded, set the slot
     * to refer to the new copy. If it has not been forwarded, copy
     * the object, and set the forwarding pointer.
     */
    if (CVMssMarkedIn((CVMUint32*)ref, theHeap->forwarded)) {
	*refPtr = CVMssGetForwardingPtr(ref);
	CVMtraceGcCollect(("GC: Object already forwarded: %x -> %x\n",
			   ref, *refPtr));
    } else {
	*refPtr = CVMssForwardObject(ref);
    }

#if 0
    /*
     * Debugging code that will tell you each time a ClassLoader or
     * Class instance is reached. Useful for debugging class unloading.
     */
    {
	CVMClassBlock* cb = CVMobjectGetClass(*refPtr);
	while (cb != NULL) {
	    if (cb == CVMsystemClass(java_lang_ClassLoader)) {
		CVMconsolePrintf("grey: %O\n", *refPtr);
	    }
	    cb = CVMcbSuperclass(cb);
	}
    }

    if (CVMobjectGetClass(*refPtr) == CVMsystemClass(java_lang_Class)) {
	CVMClassBlock* cb =
            *((CVMClassBlock**)(*refPtr) +
	      CVMoffsetOfjava_lang_Class_classBlockPointer);
	if (CVMcbClassLoader(cb) != NULL) {
	    CVMconsolePrintf("grey: %C\n", cb);
	}
    }
#endif
}

#ifdef DEBUG
/*
 * Utility function to get an object reference, forwarded or not
 */
CVMObject*
CVMssGetObject(CVMObject* ref)
{
    if (ref == NULL) {
	return ref;
    }
    if (CVMobjectIsInROM(ref)) {
	return ref;
    }
    if (CVMssMarkedIn((CVMUint32*)ref, theHeap->forwarded)) {
	return CVMssGetForwardingPtr(ref);
    } else {
	return ref;
    }
}
#endif

/*
 * Copy one object to the next space. This is passed as a callback to
 * the class scanners. 
 */
static void
CVMssHandleNonNullReference(CVMObject** ref, void* data)
{
    CVMassert(ref != 0);
    CVMassert(*ref != 0);

    CVMtraceGcScan(("GC: Root: [0x%x] = 0x%x\n", ref, *ref));

    /*
     * Can't tolerate multiple scans of the same root
     */
    CVMassert(CVMobjectIsInROM(*ref) || CVMssInOldSemispace(*ref));
    CVMssGrayObject(ref);
}

/*
 * The generic reference handler. It is invoked for roots as well as
 * for object and array reference fields.
 */
static void
CVMssBlackenObject(CVMExecEnv* ee, CVMGCOptions* gcOpts, CVMObject* ref)
{
    CVMassert(ref != 0);

    CVMtraceGcCollect(("GC: Blacken object %x\n", ref));
    /*
     * Queue up all non-null object references. Handle the class as well.
     */
    CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts,
					 ref, CVMobjectGetClassWord(ref), {
	if (*refPtr != 0) {
	    CVMssGrayObject(refPtr);
	}
    }, CVMssHandleNonNullReference, NULL);
}

/*
 * Blacken objects until there is no more to do
 */
static void
CVMssScanLiveFromRoots(CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC: CVMssScanLiveFromRoots, copyBase=%x, copyTop=%x\n",
		       copyBase, copyTop));
    while (copyBase < copyTop) {
	CVMUint32 objSize = CVMobjectSize((CVMObject*)copyBase);
	/*
	CVMtraceGcCollect(("GC: Found object=0x%x, size=%d, class=%C\n",
			   copyBase, objSize,
			   CVMobjectGetClass((CVMObject*)copyBase)));
	*/
	CVMssBlackenObject(ee, gcOpts, (CVMObject*)copyBase);
	copyBase += objSize / 4;
    }
    CVMassert(copyBase == copyTop);
}

typedef struct CVMssTransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
} CVMssTransitiveScanData;

static void
CVMssRefScanTransitively(CVMObject** ref, void* data)
{
    CVMssTransitiveScanData* tsd = (CVMssTransitiveScanData*)data;
    CVMassert(ref != 0);
    CVMassert(*ref != 0);

    /*
     * Gray if not already grayed
     */
    if (CVMssInOldSemispace(*ref)) {
	CVMssGrayObject(ref);
    }
    /*
     * Now scan all its children as well
     */
    CVMssScanLiveFromRoots(tsd->ee, tsd->gcOpts);
}

/*
 * Make sure copyBase and copyTop point to the to-space before copying
 * any roots.
 */
static void
CVMssSetupCopying()
{
    copyBase = theHeap->to->data;
    copyTop  = copyBase;
}

static void
CVMssFlip()
{
    CVMSsSemispace* newCurrent = theHeap->to;
    theHeap->to = theHeap->from;
    theHeap->from = newCurrent;
    
    /* This is the boundary of the live data */
    newCurrent->allocPtr = copyTop;

    CVMssMakeSpaceCurrent(theHeap->from);
}

/*
 * Clear forwarded bits before starting GC
 */
static void
CVMssClearForwarded()
{
    memset(theHeap->forwarded, 0, theHeap->sizeOfBitmaps);
}

static CVMBool
CVMssRefIsLive(CVMObject** refPtr, void* data)
{
    CVMObject* ref;
    CVMassert(refPtr != NULL);

    ref = *refPtr;
    CVMassert(ref != NULL);

    /*
     * ROM objects are always live
     */
    if (CVMobjectIsInROM(ref)) {
	return CVM_TRUE;
    } else {
	/* Did somebody else already forward this object? */
	return !CVMssInOldSemispace(ref) ||
	    CVMssMarkedIn((CVMUint32*)ref, theHeap->forwarded);
    }
}

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Scan over freed objects. */
static void
CVMssScanFreedObjects(CVMExecEnv *ee)
{
    CVMUint32 *base = allocBase;
    CVMUint32 *top = allocPtr;

    CVMtraceGcCollect(("GC: CVMssScanFreedObjects, base=%x, top=%x\n",
                       base, top));
    while (base < top) {
        CVMObject *obj = (CVMObject*)base;
        CVMClassBlock *objCb;
        CVMUint32 objSize;
        CVMBool collected = CVM_TRUE;

        if (CVMssMarkedIn((CVMUint32*)obj, theHeap->forwarded)) {
            obj = CVMssGetForwardingPtr(obj);
            collected = CVM_FALSE;
        }

        objCb = CVMobjectGetClass(obj);
        objSize = CVMobjectSizeGivenClass(obj, objCb);

        if (collected) {
#ifdef CVM_JVMPI
            if (CVMjvmpiEventObjectFreeIsEnabled()) {
                CVMjvmpiPostObjectFreeEvent(obj);  /* Notify the profiler. */
            }
            liveObjectCount--;
#endif
#ifdef CVM_JVMTI
            if (CVMjvmtiShouldPostObjectFree()) {
                CVMjvmtiPostObjectFreeEvent(obj);  /* Notify the profiler. */
            }
#endif
            CVMtraceGcCollect(("GC: Freed object=0x%x, size=%d, class=%C\n",
                               obj, objSize, objCb));
        }
        base += objSize / 4;
    }
    CVMassert(base == top);
}
#endif

/*
 * This routine is called by the common GC code after all locks are
 * obtained, and threads are stopped at GC-safe points. It's the
 * routine that needs a snapshot of the world while all threads are
 * stopped (typically at least a root scan).
 */
void
CVMgcimplDoGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMGCOptions gcOpts = {
        /* isUpdatingObjectPointers */ CVM_TRUE,
        /*discoverWeakReferences*/ CVM_FALSE,
#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
        /*isProfilingPass*/ CVM_FALSE
#endif
    };
    CVMssTransitiveScanData tsd;
    CVMtraceGcStartStop(("GC: Collecting from 0x%x to 0x%x\n",
			 theHeap->from, theHeap->to));

    /*
     * Clear all class marks if necessary. This is so that we don't
     * try to scan the same class twice.
     */
    CVMgcClearClassMarks(ee, &gcOpts);

    /*
     * No objects copied yet
     */
    CVMssClearForwarded();

    /*
     * Set up the destination semispace for copying. We will need these
     * for CVMssHandleNonNullReference, as well as CVMssScanLiveFromRoots().
     */
    CVMssSetupCopying();

    gcOpts.discoverWeakReferences = CVM_TRUE;

    /*
     * CVMssHandleNonNullReference will copy all roots.
     */
    CVMgcScanRoots(ee, &gcOpts, CVMssHandleNonNullReference, NULL);
    /*
     * Now start from the roots, and copy all live data to 'toSpace'
     */
    CVMssScanLiveFromRoots(ee, &gcOpts);
    /*
     * Don't discover any more weak references.
     * %comment: f009
     */
    gcOpts.discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    tsd.ee = ee;
    tsd.gcOpts = &gcOpts;
    CVMgcProcessSpecialWithLivenessInfo(ee, &gcOpts, CVMssRefIsLive, NULL,
					CVMssRefScanTransitively, &tsd);
    
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVMssScanFreedObjects(ee); /* Report freed objects: */
#endif
    /*
     * All uncopied objects are garbage. Flip fromSpace and toSpace.
     */
    CVMssFlip();

    lastMajorGCTime = CVMtimeMillis();

    CVMtraceGcStartStop(("GC: Done.\n"));
}

/*
 * Try to allocate object of size 'numBytes' bytes
 */
static CVMObject*
CVMssTryAlloc(CVMUint32 numBytes, CVMUint32* threshold)
{
    CVMUint32* allocNext = allocPtr + numBytes / 4;
    if (allocNext > threshold) {
	CVMtraceGcAlloc(("GC: CVMssTryAlloc(%d, %x) -> %x\n",
			 numBytes, threshold, 0));
	return 0;
    } else {
	CVMUint32* ret = allocPtr;
	allocPtr = allocNext;
	CVMtraceGcAlloc(("GC: CVMssTryAlloc(%d, %x) -> %x\n",
			 numBytes, threshold, ret));
	return (CVMObject*)ret;
    }
}

CVMObject*
CVMgcimplRetryAllocationAfterGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMObject* allocatedObj;
    
    /* re-try if GC occurred, but this time relax the threshold */
    allocatedObj = CVMssTryAlloc(numBytes, allocTop);
    return allocatedObj;
}

/*
 * Allocate uninitialized heap object of size numBytes
 */
CVMObject*
CVMgcimplAllocObject(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMObject* allocatedObj;
#ifdef CVM_SS_GC_ON_EVERY_ALLOC
    static CVMUint32 allocCount = 1;
    /* Do one for stress-test. If it was unsuccessful, try again in the
       next allocation */
    if ((allocCount % CVM_SS_NO_ALLOCS_UNTIL_GC) == 0) {
	if (CVMssInitiateGC(ee, numBytes)) {
	    allocCount = 1;
	}
    } else {
	allocCount++;
    }
#endif
    allocatedObj = CVMssTryAlloc(numBytes, allocThreshold);
#ifdef CVM_JVMPI
    if (allocatedObj != NULL) {
        liveObjectCount++;
    }
#endif
    return allocatedObj;
}

#ifdef CVM_JVMPI
/* Purpose: Posts the JVMPI_EVENT_ARENA_NEW events for the GC specific
            implementation. */
/* NOTE: This function is necessary to compensate for the fact that
         this event has already transpired by the time that JVMPI is
         initialized. */
void CVMgcimplPostJVMPIArenaNewEvent(void)
{
    if (CVMjvmpiEventArenaNewIsEnabled()) {
        CVMUint32 lastSharedArenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaNewEvent(lastSharedArenaID + 1, "Java Heap");
    }
}

/* Purpose: Posts the JVMPI_EVENT_ARENA_DELETE events for the GC specific
            implementation. */
void CVMgcimplPostJVMPIArenaDeleteEvent(void)
{
    if (CVMjvmpiEventArenaDeleteIsEnabled()) {
        CVMUint32 lastSharedArenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaDeleteEvent(lastSharedArenaID + 1);
    }
}

/* Purpose: Gets the arena ID for the specified object. */
/* Returns: The requested ArenaID if found, else returns
            CVM_GC_ARENA_UNKNOWN. */
CVMUint32 CVMgcimplGetArenaID(CVMObject *obj)
{
    CVMUint32 arenaID = CVM_GC_ARENA_UNKNOWN;
    if (CVMssInOldSemispace(obj) || CVMssInNewSemispace(obj)) {
        arenaID = CVMgcGetLastSharedArenaID() + 1;
    }
    return arenaID;
}

/* Purpose: Gets the number of objects allocated in the heap. */
CVMUint32 CVMgcimplGetObjectCount(CVMExecEnv *ee)
{
    return liveObjectCount;
}
#endif

/*
 * Return the number of bytes free in the heap. 
 */
CVMUint32
CVMgcimplFreeMemory(CVMExecEnv* ee)
{
    /* The remaining space in the semispace */
    return ((CVMUint8*)allocTop - (CVMUint8*)allocPtr);
}

/*
 * Return the amount of total memory in the heap, in bytes. Take into
 * account one semispace size only, since that is the useful amount.
 */
CVMUint32
CVMgcimplTotalMemory(CVMExecEnv* ee)
{
    return theHeap->size;
}

/*
 * Destroy GC global state
 */
void
CVMgcimplDestroyGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying global state for semi-space copying GC\n"));
}

/*
 * Destroy heap
 */
CVMBool
CVMgcimplDestroyHeap(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying heap for semi-space copying GC\n"));

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaDeleteEvent();
#endif

    theHeap = 0;

    /*
     * All Java threads have stopped by the time we destroy the heap
     */
    free(globalState->heap->from);
    free(globalState->heap->to);
    free(globalState->heap);

    globalState->heap = 0;
    return CVM_TRUE;
}

/*
 * JVM_MaxObjectInspectionAge() support
 */
CVMInt64
CVMgcimplTimeOfLastMajorGC()
{
    return lastMajorGCTime;
}

#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)

/*
 * Heap iteration. Call (*callback)() on each object in the heap.
 */
/* Returns: CVM_TRUE if the scan was done completely.  CVM_FALSE if aborted
            before scan is complete. */
CVMBool
CVMgcimplIterateHeap(CVMExecEnv* ee, 
		     CVMObjectCallbackFunc callback, void* callbackData)
{
    CVMBool completeScanDone =
        CVMgcScanObjectRange(ee, allocBase, allocPtr, callback, callbackData);
    return completeScanDone;
}

#endif /* defined(CVM_DEBUG) || defined(CVM_JVMPI) */
