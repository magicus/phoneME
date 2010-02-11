/*
 * @(#)gen_eden.c	1.70 06/10/10
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
 * This file includes the implementation of a copying semispace generation.
 * This generation can act as a young or an old generation.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/generational-seg/generational.h"

#include "javavm/include/gc/generational-seg/gen_eden.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

#include "javavm/include/weakrefs.h"
#include "javavm/include/string.h"

/*
 * Range checks
 */
static CVMBool
CVMgenEdenInSpace(CVMGenSpace* space, CVMObject* ref)
{
    CVMUint32* heapPtr = (CVMUint32*)ref;
    return ((heapPtr >= space->allocBase) &&
	    (heapPtr <  space->allocTop));
}

static CVMBool 
CVMgenEdenInGeneration(CVMGeneration* gen,
		       CVMObject* ref)
{
    CVMUint32* heapPtr = (CVMUint32*)ref;
    return (heapPtr >= gen->genBase && heapPtr < gen->genTop) ;
}

static CVMBool 
CVMgenEdenInEden(CVMGenEdenGeneration* thisGen, CVMObject* ref)
{
    return CVMgenEdenInSpace(thisGen->eden, ref);
}

/*
 * Allocate  eden for the current generation
 */
static CVMGenSpace*
CVMgenAllocEdenSpace(CVMUint32* heapSpace, CVMUint32 numBytes)
{
    CVMUint32 allocSize = sizeof(CVMGenSpace);
    CVMGenSpace* space = (CVMGenSpace*)calloc(1, allocSize);
    CVMUint32 numWords = numBytes / 4;
    if (space != NULL) {
	space->allocTop  = &heapSpace[numWords];
	space->allocPtr  = &heapSpace[0];
	space->allocBase = &heapSpace[0];
    }
    return space;
}

/*
 * Make a space the current space of the generation.
 */
static void
CVMgenEdenMakeCurrent(CVMGenEdenGeneration* thisGen,
                      CVMGenSpace* thisSpace)
{
    CVMGeneration* super = &thisGen->gen;
    super->allocPtr  = thisSpace->allocPtr;
    super->allocBase = thisSpace->allocBase;
    super->allocTop  = thisSpace->allocTop;
    super->allocMark = thisSpace->allocPtr;
}

/* Forward declaration */
static CVMBool
CVMgenEdenCollect(CVMGeneration* gen,
		  CVMExecEnv*    ee, 
		  CVMUint32      numBytes, /* collection goal */
		  CVMGCOptions*  gcOpts);

/*
 * Scan objects in contiguous range, and do all special handling as well.
 */
static void
scanObjectsInRangeFull(CVMExecEnv* ee, CVMGCOptions* gcOpts,
		       CVMUint32* base, CVMUint32* top,
		       CVMRefCallbackFunc callback, void* callbackData)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[SS,full]: "
		       "Scanning object range (full) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts,
					     currObj,
					     currCb, {
	    if (*refPtr != 0) {
		(*callback)(refPtr, callbackData);
	    }
	}, callback, callbackData);
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

static void       
CVMgenEdenScanYoungerToOlderPointers(CVMGeneration* gen,
					  CVMExecEnv* ee,
					  CVMGCOptions* gcOpts,
					  CVMRefCallbackFunc callback,
					  void* callbackData)
{

    /*
     * Iterate over the current space objects, and call 'callback' on
     * each reference slot found in there. 'callback' is passed in by
     * an older generation to get young-to-old pointers.  
     *
     * This generation has just been GC'ed, so the space we need to iterate
     * on is 'fromSpace', which has all remaining live pointers to older
     * generations.
     */
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)gen;
    CVMUint32* startRange     = thisGen->eden->allocBase;
    CVMUint32* endRange       = thisGen->eden->allocPtr;

    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Scanning Eden younger to older ptrs [%x,%x)\n",
		       thisGen->gen.generationNo,
		       startRange, endRange));
    
    /* Here Eden is not empty because of a mark-compact collection
       of the young generation */
    scanObjectsInRangeFull(ee, gcOpts, startRange, endRange,
			   callback, callbackData);
}

/*
 * Start and end GC for this generation. 
 */
static void
CVMgenEdenStartGC(CVMGeneration* gen,
		  CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[SS,%d,full]: Starting GC\n",
		       gen->generationNo));
}

static void
CVMgenEdenEndGC(CVMGeneration* gen,
		CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[SS,%d,full]: Ending GC\n",
		       gen->generationNo));
}

/*
 * Return remaining amount of memory
 */
static CVMUint32
CVMgenEdenFreeMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    return (CVMUint32)((CVMUint8*)gen->allocTop - (CVMUint8*)gen->allocPtr);
}

/*
 * Return total amount of memory
 */
static CVMUint32
CVMgenEdenTotalMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    return (CVMUint32)((CVMUint8*)gen->allocTop - (CVMUint8*)gen->allocBase);
}

static CVMGenSpace*
CVMgenEdenGetExtraSpace(CVMGeneration* gen)
{
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)gen;
    /*
     * Make sure it is unused
     */
    if (gen->allocPtr == gen->allocBase) {
        return thisGen->eden;
    } else {
        thisGen->extraSpace->allocPtr = thisGen->extraSpace->allocBase;
        return thisGen->extraSpace;
    }
}

#ifdef CVM_VERIFY_HEAP

static CVMBool
VerifyRefInSegGeneration(CVMGeneration* gen, CVMObject* ref) 
{ 
    CVMGenSegment* segBase = (CVMGenSegment*) gen->allocBase;
    CVMGenSegment* segCurr = (CVMGenSegment*) segBase;
 
    do {
        if ((CVMUint32*)ref >= segCurr->allocBase && 
            (CVMUint32*)ref <  segCurr->allocPtr) {
            return CVM_TRUE; 
        }
        segCurr = segCurr->nextSeg; 
    } while (segCurr != segBase); 
 
    return CVM_FALSE; 
} 

static void
CVMgenEdenVerifyRoot(CVMObject** refPtr, void* data)
{
    CVMGeneration* gen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration* spillGen = gen->nextGen;
    CVMGeneration* oldGen = spillGen->nextGen;
    CVMUint32* ref = (CVMUint32*)*refPtr ;

    CVMassert((ref >= gen->allocBase && ref < gen->allocPtr) ||
              VerifyRefInSegGeneration(spillGen, (CVMObject*)ref) ||
              VerifyRefInSegGeneration(oldGen, (CVMObject*)ref) ||
              CVMobjectIsInROM((CVMObject*)ref))  ;

    if((ref >= gen->allocBase && ref < gen->allocPtr) ||
       VerifyRefInSegGeneration(spillGen,(CVMObject*) ref) ||
       VerifyRefInSegGeneration(oldGen, (CVMObject*)ref)) {
        CVMObject*     currObj = (CVMObject*)ref;
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
        CVMAddr        classWord = CVMobjectGetClassWord(currObj);
        CVMClassBlock* currCb    = CVMobjectGetClassFromClassWord(classWord);
        CVMUint32      objSize   = CVMobjectSizeGivenClass(currObj, currCb);
        CVMassert(objSize > 0);
    }

    return;
}

static void
CVMgenEdenVerifyGeneration(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMUint32* base = gen->allocBase;
    CVMUint32* top = gen->allocPtr;
    CVMUint32* curr = base ;
    CVMGeneration* spillGen = gen->nextGen;
    CVMGeneration* oldGen = spillGen->nextGen;

    CVMgenScanAllRoots(gen, ee, gcOpts, CVMgenEdenVerifyRoot, gen);
    CVMgenScanAllRoots(gen, ee, gcOpts, CVMgenEdenVerifyRoot, spillGen);
    CVMgenScanAllRoots(gen, ee, gcOpts, CVMgenEdenVerifyRoot, oldGen);

    while (curr < top) {
	CVMObject*     currObj   = (CVMObject*)curr;
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
	CVMAddr        classWord = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb    = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32      objSize   = CVMobjectSizeGivenClass(currObj, currCb);

	/* CVMassert(CVMclassIsValidClassBlock(ee, currCb)) ; */
	CVMassert(!CVMobjectIsInROM(currObj));

	/* Make sure the object doesn't have any spurious references */
	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    if (*refPtr != 0) {
		CVMObject* ref = *refPtr ;
		CVMassert(((CVMUint32*)ref >= gen->allocBase && 
			   (CVMUint32*)ref < gen->allocPtr) ||
			  VerifyRefInSegGeneration(spillGen, ref) ||
			  VerifyRefInSegGeneration(oldGen, ref) ||
			  CVMobjectIsInROM(ref))  ;
	    }
	});
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
    
}
#endif

#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
static CVMBool iterateGen(CVMGeneration* gen, CVMExecEnv* ee,
			  CVMObjectCallbackFunc callback,
			  void* callbackData)
{
    CVMUint32*     base = gen->allocBase;
    CVMUint32*     top  = gen->allocPtr;
    return CVMgcScanObjectRange(ee, base, top, callback, callbackData);
}
#endif

/*
 * Allocate a semispaces generation. This includes the actual semispaces
 * as well as all the supporting data structures.
 */
CVMGeneration*
CVMgenEdenAlloc(CVMUint32* space, CVMUint32 totalNumBytes)
{
    CVMGenEdenGeneration* thisGen  = (CVMGenEdenGeneration*)
	                    calloc(sizeof(CVMGenEdenGeneration), 1);
    CVMGenSpace* eden;
    CVMGenSpace* extraSpace;
    CVMUint32 extraSpaceBytes = CVM_GCIMPL_SCRATCH_BUFFER_SIZE_BYTES;

    if (thisGen == NULL) {
	return NULL;
    }

    thisGen->edenSize = totalNumBytes;
    eden = CVMgenAllocEdenSpace(space, thisGen->edenSize) ;
    extraSpace = CVMgenAllocEdenSpace(eden->allocTop, extraSpaceBytes) ;

    if (eden == NULL || extraSpace == NULL) {
	free(thisGen);
	free(eden);
	free(extraSpace);
	return NULL;
    }

    /*
     * Initialize thisGen. 
     */
    thisGen->eden        = eden ;
    thisGen->extraSpace  = extraSpace ;

    thisGen->evacuationFailed = CVM_FALSE;
    /*
     * Start the world on Eden
     */
    CVMgenEdenMakeCurrent(thisGen, eden);

    /* Assuming eden and semispaces are contiguous */
    thisGen->gen.genBase = thisGen->eden->allocBase; 
    thisGen->gen.genTop  = thisGen->eden->allocTop; 

    /* 
     * And finally, set the function pointers for this generation
     */
    thisGen->gen.collect = CVMgenEdenCollect;
    thisGen->gen.scanOlderToYoungerPointers = NULL ;
    thisGen->gen.scanYoungerToOlderPointers =
	CVMgenEdenScanYoungerToOlderPointers;
    thisGen->gen.promoteInto = NULL ;
    thisGen->gen.scanPromotedPointers = NULL;
    thisGen->gen.inGeneration = CVMgenEdenInGeneration;
    thisGen->gen.inYounger = NULL; /* Never called on youngest gen */
    thisGen->gen.getExtraSpace = CVMgenEdenGetExtraSpace;
    thisGen->gen.startGC = CVMgenEdenStartGC;
    thisGen->gen.endGC = CVMgenEdenEndGC;
    thisGen->gen.freeMemory = CVMgenEdenFreeMemory;
    thisGen->gen.totalMemory = CVMgenEdenTotalMemory;
#ifdef CVM_VERIFY_HEAP
    thisGen->gen.verifyGen = CVMgenEdenVerifyGeneration;
#endif

#if defined(CVM_DEBUG) || defined(CVM_JVMPI)
    thisGen->gen.iterateGen = iterateGen;
#endif

    CVMdebugPrintf(("GC[SS]: Initialized semi-space gen for generational GC\n"));
    CVMdebugPrintf(("\tSize of Eden in bytes=%d\n"
		  "\tLimits of generation = [0x%x,0x%x)\n" 
		  "\tEden                 = [0x%x,0x%x)\n", 
		  thisGen->edenSize, 
                  eden->allocBase, eden->allocTop,
		  eden->allocBase, eden->allocTop));
    return (CVMGeneration*)thisGen;
}

/*
 * Free all the memory associated with the current semispaces generation
 */
void
CVMgenEdenFree(CVMGenEdenGeneration* thisGen)
{
    free(thisGen->eden);
    free(thisGen->extraSpace);
    thisGen->eden            = NULL;
    thisGen->extraSpace      = NULL;
    free(thisGen);
}

/*
 * GC done. Empty Eden.
 */
static void
CVMgenEdenEmptyEden(CVMGenEdenGeneration* thisGen)
{
    CVMGenSpace* eden = thisGen->eden;
    eden->allocPtr = eden->allocBase;
    CVMgenEdenMakeCurrent(thisGen, eden);
}

/*
 * Copy object, leave forwarding pointer behind
 */
/* 
 * member clas and various32 hold a native pointer
 * therefore use type CVMAddr (instead of CVMUint32)
 * which is 64 bit aware
 */
static CVMObject*
CVMgenEdenForwardOrPromoteObject(CVMGenEdenGeneration* thisGen,
				 CVMObject* ref, CVMAddr classWord)
{
    CVMClassBlock*  objCb      = CVMobjectGetClassFromClassWord(classWord);
    CVMUint32       objSize    = CVMobjectSizeGivenClass(ref, objCb);
    CVMObject*      ret        = NULL;

    /* 
     * If the object is present in Eden, copy into spill-space
     */

    CVMassert(!thisGen->evacuationFailed);

    /* Unconditionally promote into next generation */
    ret = thisGen->gen.nextGen->promoteInto(thisGen->gen.nextGen, 
					    ref, objSize);

    if (ret == NULL) {
        thisGen->evacuationFailed = CVM_TRUE ;
        /* CVMpanic("****Out of Space during Eden collection****"); */
        return NULL ;
    }

#ifdef CVM_JVMPI
    if (CVMjvmpiEventObjectMoveIsEnabled()) {
        CVMjvmpiPostObjectMoveEvent(1, ref, 1, ret);
    }
#endif

    /*
     * Set forwarding pointer
     */
    {
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
	CVMAddr newClassWord = (CVMAddr)ret;
	/* Mark as forwarded */
	CVMobjectSetMarkedOnClassWord(newClassWord);
	/* ... and forward to the new copy */
	CVMobjectSetClassWord(ref, newClassWord);
    }
    
    return ret;
}

/*
 * Gray an object known to be in the old semispace
 */
static void
CVMgenEdenGrayObject(CVMGenEdenGeneration* thisGen, CVMObject** refPtr)
{
    CVMObject* ref = *refPtr;
    /* 
     * member clas and various32 hold a native pointer
     * therefore use type CVMAddr (instead of CVMUint32)
     * which is 64 bit aware
     */
    CVMAddr  classWord = CVMobjectGetClassWord(ref);

    CVMassert(!thisGen->evacuationFailed);

    CVMtraceGcCollect(("GC[SS,%d]: Graying object %x\n",
		       thisGen->gen.generationNo,
		       ref));
    /*
     * ROM objects should have been filtered out by the time we get here
     */
    CVMassert(!CVMobjectIsInROM(ref));

    /*
     * Also, all objects that don't belong to this generation need to have
     * been filtered out by now.
     */
    CVMassert(CVMgenEdenInEden(thisGen, ref));

    /*
     * If the object is in old space it might already be
     * forwarded. Check that. If it has been forwarded, set the slot
     * to refer to the new copy. If it has not been forwarded, copy
     * the object, and set the forwarding pointer.
     */
    if (CVMobjectMarkedOnClassWord(classWord)) {
	/* Get the forwarding pointer */
	*refPtr = (CVMObject*)CVMobjectClearMarkedOnClassWord(classWord);
	CVMtraceGcCollect(("GC[SS,%d]: Object already forwarded: %x -> %x\n",
			   thisGen->gen.generationNo,
			   ref, *refPtr));
    } else {
        CVMObject* newRefPtr = CVMgenEdenForwardOrPromoteObject(thisGen, ref,
								classWord);
        if (newRefPtr != NULL) {
	    *refPtr = newRefPtr ;
        }
    }
}

/*
 * Gray a slot if it points into this generation.
 */
static void
CVMgenEdenFilteredGrayObject(CVMGenEdenGeneration* thisGen,
				  CVMObject** refPtr,
				  CVMObject*  ref)
{
    /*
     * Gray only if the slot points to this generation's eden/from space. 
     *
     * Make sure we don't scan roots twice!
     */
    CVMassert(!thisGen->evacuationFailed);

    if (CVMgenEdenInEden(thisGen, ref)) {
	CVMgenEdenGrayObject(thisGen, refPtr);
    }
}

/* 
 * A version of the above where multiple scans of the same slot are allowed.
 * Used for CVMgenEdenScanTransitively() only.
 */
static void
CVMgenEdenFilteredGrayObjectWithMS(CVMGenEdenGeneration* thisGen,
					CVMObject** refPtr,
					CVMObject*  ref)
{
    CVMassert(!thisGen->evacuationFailed);

    /*
     * Gray only if the slot points to this generation's from space or
     * into the Eden
     */
    if (CVMgenEdenInEden(thisGen, ref)) {
	CVMgenEdenGrayObject(thisGen, refPtr);
    }
}

/*
 * Scan a non-NULL reference into this generation. Make sure to
 * handle the case that the slot has already been scanned -- the
 * root scan should allow for such a case.
 */
static void
CVMgenEdenFilteredHandleRoot(CVMObject** refPtr, void* data)
{
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)data;
    CVMObject* ref = *refPtr;

    if (thisGen->evacuationFailed) {
        return;
    }

    CVMassert(refPtr != 0);
    CVMassert(ref != 0);

    CVMtraceGcCollect(("GC[SS,%d]: Handling external reference: [%x] = %x\n",
		       thisGen->gen.generationNo,
		       refPtr, ref));

    CVMgenEdenFilteredGrayObject(thisGen, refPtr, ref);
    return;
}

static void
CVMgenEdenFollowRootsFull(CVMGenEdenGeneration* thisGen,
			   CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMGeneration* nextGen = thisGen->gen.nextGen;


    /* For a young generation, the scan above may have
       promoted objects. Ask the next generation to scan
       those objects for pointers into the current */

    nextGen->scanPromotedPointers(nextGen, ee, gcOpts, 
                                  CVMgenEdenFilteredHandleRoot, thisGen);
}

typedef struct CVMGenSemispaceTransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
    CVMGenEdenGeneration* thisGen;
} CVMGenSemispaceTransitiveScanData;

static void
transitiveScanner(CVMObject** refPtr, void* data, CVMBool strict)
{
    CVMGenSemispaceTransitiveScanData* tsd =
	(CVMGenSemispaceTransitiveScanData*)data;
    CVMGenEdenGeneration* thisGen = tsd->thisGen;
    CVMObject* ref;

    CVMassert(refPtr != 0);
    ref = *refPtr;
    CVMassert(ref != 0);

    if (strict) {
	CVMassert(!thisGen->evacuationFailed);
    } else {
	if (thisGen->evacuationFailed) {
	    return;
	}
    }

    CVMgenEdenFilteredGrayObjectWithMS(thisGen, refPtr, ref);

    /*
     * Now scan all its children as well
     */
    CVMgenEdenFollowRootsFull(thisGen, tsd->ee, tsd->gcOpts);

    return;
}

static void
CVMgenEdenScanTransitively(CVMObject** refPtr, void* data)
{
    transitiveScanner(refPtr, data, CVM_FALSE);
}

/*
 * Just like the above, except that failure to move objects is not
 * allowed.
 */
static void
CVMgenEdenScanTransitivelyNoFailure(CVMObject** refPtr, void* data)
{
    transitiveScanner(refPtr, data, CVM_TRUE);
}


/*
 * Test whether a given reference is live or dying. If the reference
 * does not point to the current generation, answer TRUE. The generation
 * that does contain the pointer will give the right answer.
 */
static CVMBool
CVMgenEdenRefIsLive(CVMObject** refPtr, void* data)
{
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)data;
    CVMObject* ref;

    CVMassert(refPtr != NULL);

    ref = *refPtr;
    CVMassert(ref != NULL);

    /*
     * ROM objects are always live
     */
    if (CVMobjectIsInROM(ref)) {
	return CVM_TRUE;
    }
    /*
     * If this reference is not in this generation, just assume
     * it's live. 
     */
    if (!CVMgenEdenInGeneration(&thisGen->gen, ref)) {
	return CVM_TRUE;
    }
    return !CVMgenEdenInEden(thisGen, ref) || CVMobjectMarked(ref);
}

#if defined(CVM_JVMPI) || defined (CVM_JVMTI)
/* Purpose: Scan over freed objects. */
static void
CVMgenEdenScanFreedObjects(CVMGeneration *thisGen, CVMExecEnv *ee)
{
    CVMUint32 *base = thisGen->allocBase;
    CVMUint32 *top = thisGen->allocPtr;

    CVMtraceGcCollect(("GC: CVMgenEdenScanFreedObjects, "
                       "base=%x, top=%x\n", base, top));
    while (base < top) {
        CVMObject *obj = (CVMObject*)base;
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
        CVMAddr  classWord = CVMobjectGetClassWord(obj);
        CVMClassBlock *objCb;
        CVMUint32 objSize;
        CVMBool collected = CVM_TRUE;

        if (CVMobjectMarkedOnClassWord(classWord)) {
            /* Get the forwarding pointer */
            obj = (CVMObject*)CVMobjectClearMarkedOnClassWord(classWord);
            collected = CVM_FALSE;
        }

        objCb = CVMobjectGetClass(obj);
        objSize = CVMobjectSizeGivenClass(obj, objCb);

        /* Notify the profiler of an object which is about to be GC'ed: */
        if (collected) {
#ifdef CVM_JVMPI
            extern CVMUint32 liveObjectCount;
            if (CVMjvmpiEventObjectFreeIsEnabled()) {
                CVMjvmpiPostObjectFreeEvent(obj);
            }
            liveObjectCount--;
#endif
#ifdef CVM_JVMTI
            if (CVMjvmtiShouldPostObjectFree()) {
                CVMjvmtiPostObjectFreeEvent(obj);
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


static CVMBool
CVMgenEdenCollectFull(CVMGeneration* gen,
			   CVMExecEnv*    ee, 
			   CVMUint32      numBytes, /* collection goal */
			   CVMGCOptions*  gcOpts
			   )
{
    CVMGenSemispaceTransitiveScanData tsd;
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)gen;
    CVMBool success;

    gcOpts->discoverWeakReferences = CVM_TRUE;

    CVMtraceGcStartStop(("GC[EDEN,%d,full]: Collecting ... ",
			 thisGen->gen.generationNo));

    /*
     * Scan all roots that point to this generation.
     */
    CVMgenScanAllRoots((CVMGeneration*)thisGen,
		       ee, gcOpts, CVMgenEdenFilteredHandleRoot, thisGen);
    if (thisGen->evacuationFailed) {
        goto EVACUATION_FAILED;
    }

    /*
     * Now start from the roots, and copy or promote all live data 
     */
    CVMgenEdenFollowRootsFull(thisGen, ee, gcOpts);

    if (thisGen->evacuationFailed) {
        goto EVACUATION_FAILED;
    }

    /*
     * Don't discover any more weak references.
     */
    gcOpts->discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    tsd.ee = ee;
    tsd.gcOpts = gcOpts;
    tsd.thisGen = thisGen;

    /* First handle weakrefs processing. This is one case where there
       can be additional evacuations, and a possible "undo". */
    CVMweakrefProcessNonStrong(ee, CVMgenEdenRefIsLive, thisGen,
			       CVMgenEdenScanTransitively, &tsd, gcOpts);

    if (thisGen->evacuationFailed) {
	goto EVACUATION_FAILED;
    }

    /* Now handle interned string processing. This is another case
       where there can be additional evacuations, and a possible
       "undo". */
    CVMinternProcessNonStrong(CVMgenEdenRefIsLive, thisGen,
			      CVMgenEdenScanTransitively, &tsd);

    if (thisGen->evacuationFailed) {
	goto EVACUATION_FAILED;
    }

    /* Now we are done with all possible activities that could result
       in a failed evacuation. From here on out, transitiveScanner will
       not be called on anything that it has not already been called on. */

    /* Finish up the work of the weakrefs */
    CVMweakrefFinalizeProcessing(ee, CVMgenEdenRefIsLive, thisGen,
				 CVMgenEdenScanTransitivelyNoFailure, 
				 &tsd, gcOpts);
    
    /* Finish up the work of scanning interned strings */
    CVMinternFinalizeProcessing(CVMgenEdenRefIsLive, thisGen,
				CVMgenEdenScanTransitivelyNoFailure, &tsd);
    
    CVMgcProcessSpecialWithLivenessInfoWithoutWeakRefs(ee, gcOpts,
					CVMgenEdenRefIsLive, thisGen,
					CVMgenEdenScanTransitivelyNoFailure, 
                                        &tsd);

    CVMassert(!thisGen->evacuationFailed);
    
#ifdef CVM_JVMPI
    CVMgenEdenScanFreedObjects((CVMGeneration*)thisGen, ee);
#endif
    /*
     * All relevant live data has been copied to Spill gen.
     */
    CVMgenEdenEmptyEden(thisGen);

    success = (numBytes <= thisGen->edenSize) ;
    CVMtraceGcStartStop(("... EDEN %d done, success for %d bytes %s\n",
			 thisGen->gen.generationNo,
			 numBytes,
			 success ? "TRUE" : "FALSE"));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;

EVACUATION_FAILED:

    /*
      During the collection, an evacuation failed letting
      the heap fall into an unstable state. Need to 
      get out of this mess by unwinding any changes and
      do a mark-compact collection of the young generation
      so that life can go on
    */
    CVMdebugPrintf(("\n***Unwinding copy collection\n"));
    gcOpts->discoverWeakReferences = CVM_FALSE;
    return CVM_FALSE ;
}

static void
CVMgenEdenFixObjectHeaders(CVMGenEdenGeneration* thisGen, 
                           CVMUint32* base, 
                           CVMUint32* top)
{
    CVMUint32* curr = base;
 
    CVMtraceGcCollect(("GC[EDEN-MC,%d]: Sweeping object range [%x,%x)\n",
                        thisGen->gen.generationNo, base, top));
    while (curr < top) {
         CVMObject* currObj    = (CVMObject*)curr;
	 /* 
	  * member clas and various32 hold a native pointer
	  * therefore use type CVMAddr (instead of CVMUint32)
	  * which is 64 bit aware
	  */
         CVMAddr  classWord  = CVMobjectGetClassWord(currObj);
         CVMClassBlock* currCb; 
         CVMUint32  objSize;
 
         if (CVMobjectMarkedOnClassWord(classWord)) {
             CVMObject* copyObj = 
		 (CVMObject*)CVMobjectClearMarkedOnClassWord(classWord);
	     /* 
	      * member clas and various32 hold a native pointer
	      * therefore use type CVMAddr (instead of CVMUint32)
	      * which is 64 bit aware
	      */
             CVMAddr  newClassWord;
             
             /* Get the valid class word from the to-space or promoted copy*/
             classWord = CVMobjectGetClassWord(copyObj);

              CVMobjectSetMarkedOnClassWord(newClassWord);
              CVMobjectSetClassWord(currObj, classWord);

              /* Mark the to-space object */
	      /* 
	       * member clas and various32 hold a native pointer
	       * therefore use type CVMAddr (instead of CVMUint32)
	       * which is 64 bit aware
	       */
              newClassWord = (CVMAddr)currObj;
              CVMobjectSetMarkedOnClassWord(newClassWord);
              CVMobjectSetClassWord(copyObj, newClassWord);
         }

         currCb = CVMobjectGetClassFromClassWord(classWord);
         objSize = CVMobjectSizeGivenClass(currObj, currCb);

        /* iterate */
        curr += objSize / 4;
    } /* end of while */
    CVMassert(curr == top); /* This had better be exact */
}


static void   
CVMgenEdenFixRoot(CVMObject** refPtr, void* data)
{
    CVMObject* ref = *refPtr;
    /* 
     * member clas and various32 hold a native pointer
     * therefore use type CVMAddr (instead of CVMUint32)
     * which is 64 bit aware
     */
    CVMAddr  classWord  = CVMobjectGetClassWord(ref);

    CVMassert(refPtr != 0);
    CVMassert(ref != 0);
 
    CVMtraceGcCollect(("GC[SS,%d]: Handling external reference: [%x] = %x\n",
                       ((CVMGeneration*)data)->generationNo,
                       refPtr, ref));

    /* If the pointer is pointing to an object in the discarded
       to-space then fix it 
    */
   
    if (CVMobjectMarkedOnClassWord(classWord)) {
        /* The correct object is now being pointed to by the
           header in the to-space object */

        *refPtr = (CVMObject*)CVMobjectClearMarkedOnClassWord(classWord);
        CVMassert(CVMgenEdenInGeneration((CVMGeneration*)data, *refPtr));
        CVMassert(!CVMobjectIsInROM(ref)) ;
    }

    return;
}

static void
CVMgenEdenFixClassPointers(CVMGenEdenGeneration* thisGen,
                           CVMExecEnv* ee, 
                           CVMGCOptions* gcOpts,
                           CVMUint32* base,
                           CVMUint32* top)
{
    CVMUint32* curr = base;

    CVMtraceGcCollect(("GC[SS,%s]: "
                       "Fixing class pointers in object range (full) [%x,%x)\n",
                       curr, top));

    while (curr < top) {
        CVMObject* currObj    = (CVMObject*)curr;
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
        CVMAddr    classWord  = CVMobjectGetClassWord(currObj);
        CVMClassBlock* currCb;
        CVMUint32  objSize;

        CVMobjectClearMarkedOnClassWord(classWord);
        currCb     = CVMobjectGetClassFromClassWord(classWord);
        objSize    = CVMobjectSizeGivenClass(currObj, currCb);

        CVMscanClassIfNeeded(ee, currCb, CVMgenEdenFixRoot, thisGen);

        /* Clear mark on the object */
        CVMobjectSetClassWord(currObj, classWord);

        /* iterate */
        curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}


static void
CVMgenEdenRollbackCollect(CVMGenEdenGeneration* thisGen,
                          CVMExecEnv*    ee,
                          CVMGCOptions*  gcOpts)
{
    CVMGenSpace* eden;
/*#define SOL_EDEN_TIMING*/
#ifdef SOL_EDEN_TIMING
#include <sys/time.h>
    hrtime_t time;
    CVMUint32 ms;
#endif /* SOL_EDEN_TIMING */

    eden = thisGen->eden ;
    /* Current allocation ptr for Eden is kept in CVMGeneration
       by CVMgenContiguousAllocate */
    eden->allocPtr = thisGen->gen.allocPtr ;
    
#ifdef SOL_EDEN_TIMING
    time = gethrtime();
#endif

    CVMgenEdenFixObjectHeaders(thisGen, eden->allocBase, eden->allocPtr);
    CVMgcClearClassMarks(ee, gcOpts);
    CVMgenEdenFixClassPointers(thisGen, ee, gcOpts, 
                               eden->allocBase, eden->allocPtr);

#ifdef SOL_EDEN_TIMING
    time = gethrtime() - time;
    ms = time / 1000000;
    CVMconsolePrintf("GC:Fixes to young gen took %d ms\n", ms);
    time = gethrtime();
#endif
    thisGen->gen.nextGen->scanAndRollbackPromotions(thisGen->gen.nextGen, ee, gcOpts,
                                            CVMgenEdenFixRoot, thisGen);
#ifdef SOL_EDEN_TIMING
    time = gethrtime() - time;
    ms = time / 1000000;
    CVMconsolePrintf("GC:Rollback promotions to young gen took %d ms\n", ms);
    time = gethrtime();
#endif

    /* Rollback any work for interned strings. */
    CVMinternRollbackHandling(ee, gcOpts, CVMgenEdenFixRoot, thisGen);
    
    /* Rollback any work for weakrefs. */
    CVMweakrefRollbackHandling(ee, gcOpts, CVMgenEdenFixRoot, thisGen);
    
    /* And finally, update the rest of the roots to roll back */
    CVMgenScanAllRoots((CVMGeneration*)thisGen, ee, gcOpts, 
                       CVMgenEdenFixRoot, thisGen);
}

extern CVMInt64 CVMtimeMillis();

static CVMBool
CVMgenEdenCollect(CVMGeneration* gen,
                  CVMExecEnv*    ee, 
                  CVMUint32      numBytes, /* collection goal */
                  CVMGCOptions*  gcOpts)
{
    CVMGenEdenGeneration* thisGen = (CVMGenEdenGeneration*)gen;
    CVMGeneration* spillGen = gen->nextGen;
    CVMGeneration* oldGen   = spillGen->nextGen;
    CVMBool result ;
    /*
     * Assume heap smaller than 4 GB for now.
     */
    CVMUint32  estSpillSurvivors = (CVMUint32)(spillGen->allocPtr - spillGen->allocBase) * 4;

    /*
#define SOL_EDEN_TIMING
#define PLAT_EDEN_TIMING
#define EDEN_GC_PATH
    */
#ifdef SOL_EDEN_TIMING
#include <sys/time.h>
    hrtime_t time;
    CVMUint32 ms;
 
    time = gethrtime();
#endif /* SOL_EDEN_TIMING */
#ifdef PLAT_EDEN_TIMING
    CVMInt64 time;
    time = CVMtimeMillis();
#endif /* SOL_EDEN_TIMING */

#ifdef EDEN_GC_PATH
    CVMconsolePrintf("Eden Collect\n");
#endif

    /* Make sure that heap integrity is good before we start: */
    CVMgenVerifyHeapIntegrity(ee, gcOpts);
    result = CVMgenEdenCollectFull(gen, ee, numBytes, gcOpts);

#ifdef SOL_EDEN_TIMING
    time = gethrtime() - time;
    ms = time / 1000000;
    CVMconsolePrintf("GC:EDEN took %d ms\n", ms);
#endif
#ifdef PLAT_EDEN_TIMING
    time = CVMtimeMillis() - time;
    CVMconsolePrintf("GC:EDEN took %d ms\n", time);
#endif


    if (thisGen->evacuationFailed) {

#ifdef EDEN_GC_PATH
        CVMconsolePrintf("Rollback Eden Collect\n");
#endif
#ifdef SOL_EDEN_TIMING
        time = gethrtime();
#endif
#ifdef PLAT_EDEN_TIMING
        time = CVMtimeMillis();
#endif

        CVMgenEdenRollbackCollect(thisGen, ee, gcOpts);
        thisGen->evacuationFailed = CVM_FALSE;

        /* Ensure the heap integrity is good after the eden rollback: */
        CVMgenVerifyHeapIntegrity(ee, gcOpts);

        spillGen->collect(spillGen, ee, ~0, gcOpts);

        /* NOTE: No need to verify the heap explicitly after the spillGen
           collection because it is already done by the spillGen itself at the
           end of its collection cycle. */

        oldGen->collect(oldGen, ee, ~0, gcOpts);

        /* NOTE: No need to verify the heap explicitly after the oldGen
           collection because it is already done by the oldGen itself at the
           end of its collection cycle. */

	/* Do a checkpoint of all generations again, in case we would
	   want to roll back again. */
	gen->startGC(gen, ee, gcOpts);
	spillGen->startGC(spillGen, ee, gcOpts);
	oldGen->startGC(oldGen, ee, gcOpts);

	result = CVMgenEdenCollectFull(gen, ee, numBytes, gcOpts);
	if (thisGen->evacuationFailed) {
	    CVMgenEdenRollbackCollect(thisGen, ee, gcOpts);
	    thisGen->evacuationFailed = CVM_FALSE;
	    /* And prepare to die... */
	}

        /* Ensure the heap integrity has been restored before moving on: */
        CVMgenVerifyHeapIntegrity(ee, gcOpts);

#ifdef SOL_EDEN_TIMING
        time = gethrtime() - time;
        ms = time / 1000000; 
        CVMconsolePrintf("GC:ROLLBACK EDEN COLLECT took %d ms\n", ms); 
#endif 
#ifdef PLAT_EDEN_TIMING
        time = CVMtimeMillis() - time;
        CVMconsolePrintf("GC:ROLLBACK EDEN COLLECT took %d ms\n", time); 
#endif 

    } else {
#if 0
        if (spillGen->freeMemory(spillGen, ee) < (thisGen->edenSize >> 3)) {
        if (spillGen->allocPtr == spillGenAllocPtr) {
#endif
        if ((spillGen->allocPtr >= (spillGen->allocTop - 5)) &&
            (estSpillSurvivors > (thisGen->edenSize >> 3))) {
 
#ifdef EDEN_GC_PATH
            CVMconsolePrintf("Eden-Spill Collect\n");
#endif
#ifdef SOL_EDEN_TIMING
            time = gethrtime();
#endif
#ifdef PLAT_EDEN_TIMING
            time = CVMtimeMillis();
#endif

            /* Ensure the heap integrity is good after the eden collection
               before the spillGen collection: */
            CVMgenVerifyHeapIntegrity(ee, gcOpts);
            spillGen->collect(spillGen, ee, numBytes, gcOpts) ;
 
#ifdef EDEN_GC_PATH
            CVMconsolePrintf("Eden-Spill Collect Free = %d\n", spillGen->freeMemory(spillGen,ee));
#endif
#ifdef SOL_EDEN_TIMING
            time = gethrtime() - time;
            ms = time / 1000000;
            CVMconsolePrintf("GC:EDEN-SPILL took %d ms\n", ms);
#endif
#ifdef PLAT_EDEN_TIMING
            time = CVMtimeMillis() - time;
            CVMconsolePrintf("GC:EDEN-SPILL took %d ms\n", time);
#endif
        } else {
            /* Ensure the heap integrity is good after the eden collection: */
            CVMgenVerifyHeapIntegrity(ee, gcOpts);
        }
    }

    return result;
}


