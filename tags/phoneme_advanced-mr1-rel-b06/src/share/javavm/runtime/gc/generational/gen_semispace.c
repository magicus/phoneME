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

#include "javavm/include/gc/generational/generational.h"

#include "javavm/include/gc/generational/gen_semispace.h"

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/* NOTE: The following symbol is not defined for this implementation because
   the semispace generation is only used as the youngGen in this generational
   GC.  However, the code is intentionally left here for reference but is
   #ifdef'ed out. */
#undef CVM_SEMISPACE_AS_OLDGEN

/*
 * Range checks
 */
static CVMBool
CVMgenSemispaceInSpace(CVMGenSpace* space, CVMObject* ref)
{
    CVMUint32* heapPtr = (CVMUint32*)ref;
    return ((heapPtr >= space->allocBase) &&
	    (heapPtr <  space->allocTop));
}

static CVMBool 
CVMgenSemispaceInOld(CVMGenSemispaceGeneration* thisGen, CVMObject* ref)
{
    return CVMgenSemispaceInSpace(thisGen->fromSpace, ref);
}

/*
 * Allocate a semispace for the current semispaces generation
 */
static CVMGenSpace*
CVMgenOneSemispaceAlloc(CVMUint32* heapSpace, CVMUint32 numBytes)
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
 * Make a semispace the current semispace of the generation.
 */
static void
CVMgenSemispaceMakeCurrent(CVMGenSemispaceGeneration* thisGen,
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
CVMgenSemispaceCollect(CVMGeneration* gen,
		       CVMExecEnv*    ee, 
		       CVMUint32      numBytes, /* collection goal */
		       CVMGCOptions*  gcOpts);

#ifdef CVM_SEMISPACE_AS_OLDGEN
/*
 * Scan objects in contiguous range, without doing any special handling.
 */
static void
scanObjectsInRangePartial(CVMExecEnv* ee, CVMGCOptions* gcOpts,
			  CVMUint32* base, CVMUint32* top,
			  CVMRefCallbackFunc callback, void* callbackData)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[SS,full]: "
		       "Scanning object range (partial) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    if (*refPtr != 0) {
		(*callback)(refPtr, callbackData);
	    }
	});
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

static void
CVMgenSemispaceUpdateTableForObjectSlot(CVMObject** refPtr,
					void* data)
{
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)data;
    CVMObject* ref = *refPtr;
    if ((ref != NULL) &&
	!CVMgenSemispaceInOld(thisGen, ref) &&
	!CVMobjectIsInROM(ref)) {
        CVMassert(!CVMgenInGeneration((CVMGeneration*)thisGen, ref));
	*CARD_TABLE_SLOT_ADDRESS_FOR(refPtr) = CARD_DIRTY_BYTE;
    }
}

static CVMBool
CVMgenSemispaceIsYoung(CVMGenSemispaceGeneration* thisGen)
{
    return thisGen->gen.generationNo == 0;
}
#endif

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

#ifdef CVM_SEMISPACE_AS_OLDGEN
/*
 * Re-build the old-to-young pointer tables after an old generation is
 * done. 
 */
static void
CVMgenSemispaceRebuildBarrierTable(CVMGenSemispaceGeneration* thisGen,
				   CVMExecEnv* ee,
				   CVMGCOptions* gcOpts,
				   CVMUint32* startRange,
				   CVMUint32* endRange)
{
    CVMassert(!CVMgenSemispaceIsYoung(thisGen)); /* Do this only for old */
    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Rebuilding barrier table for range [%x,%x)\n",
		       thisGen->gen.generationNo, startRange, endRange));
    /*
     * No need to look at classes here. They are scanned separately in
     * CVMgenScanAllRoots().
     */
    scanObjectsInRangePartial(ee, gcOpts, startRange, endRange,
			      CVMgenSemispaceUpdateTableForObjectSlot,
			      thisGen);
    CVMgenBarrierObjectHeadersUpdate((CVMGeneration*)thisGen,
				     ee, gcOpts, startRange, endRange);
}
#endif

static void       
CVMgenSemispaceScanYoungerToOlderPointers(CVMGeneration* gen,
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
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)gen;
    CVMGenSpace* currentSpace = thisGen->fromSpace;
    CVMUint32* startRange     = currentSpace->allocBase;
    CVMUint32* endRange       = currentSpace->allocPtr;
    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Scanning younger to older ptrs [%x,%x)\n",
		       thisGen->gen.generationNo, startRange, endRange));
    scanObjectsInRangeFull(ee, gcOpts, startRange, endRange,
			   callback, callbackData);
}

static void       
CVMgenSemispaceScanOlderToYoungerPointers(CVMGeneration* gen,
					  CVMExecEnv* ee,
					  CVMGCOptions* gcOpts,
					  CVMRefCallbackFunc callback,
					  void* callbackData)
{
#ifdef CVM_SEMISPACE_AS_OLDGEN
    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Scanning older to younger ptrs\n",
		       gen->generationNo));
    /*
     * To get all older to younger pointers, traverse the records in
     * the write barrier tables.
     */
    CVMgenBarrierPointersTraverse(gen, ee, gcOpts, callback, callbackData);
#else
    CVMassert(CVM_FALSE);
#endif
}


/*
 * Start and end GC for this generation. 
 */
static void
CVMgenSemispaceStartGC(CVMGeneration* gen,
		       CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[SS,%d,full]: Starting GC\n",
		       gen->generationNo));
}

static void
CVMgenSemispaceEndGC(CVMGeneration* gen,
		     CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[SS,%d,full]: Ending GC\n",
		       gen->generationNo));
}

/*
 * Return remaining amount of memory
 */
static CVMUint32
CVMgenSemispaceFreeMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    return (CVMUint32)((CVMUint8*)gen->allocTop - (CVMUint8*)gen->allocPtr);
}

/*
 * Return total amount of memory
 */
static CVMUint32
CVMgenSemispaceTotalMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    return (CVMUint32)((CVMUint8*)gen->allocTop - (CVMUint8*)gen->allocBase);
}

/* Iterate over promoted pointers in a generation. A young space
   collection accummulates promoted objects in an older
   generation. These objects need to be scanned for pointers into
   the young generation.
   
   At the start of a GC, the current allocation pointer of each
   generation is marked. Therefore the promoted objects to scan are those
   starting from 'allocMark' to the current 'allocPtr'.
  */
static void
CVMgenSemispaceScanPromotedPointers(CVMGeneration* gen,
				    CVMExecEnv* ee,
				    CVMGCOptions* gcOpts,
				    CVMRefCallbackFunc callback,
				    void* callbackData)
{
#ifdef CVM_SEMISPACE_AS_OLDGEN
    CVMUint32* startRange = gen->allocMark;
    CVMUint32* endRange   = gen->allocPtr;

    CVMtraceGcCollect(("GC[SS,full]: "
		       "Scanning promoted ptrs [%x,%x)\n",
		       startRange, endRange));
    /* loop, until promoted object scans result in no more promotions */
    /*
     * First clear class marks, since we want to be able to iterate over
     * classes scanned before promotion.
     */
    do {
        scanObjectsInRangeFull(ee, gcOpts, startRange, endRange,
                               callback, callbackData);
	startRange = endRange;
	/* gen->allocPtr may have been incremented */
	endRange = gen->allocPtr; 
    } while (startRange < endRange);

    CVMassert(endRange == gen->allocPtr);
    /*
     * Remember these pointers for further young space collections.
     * Don't forget to clear class marks again, since we need to be
     * able to traverse already scanned classes again, in order to
     * build barrier tables.
     */
    CVMgenSemispaceRebuildBarrierTable((CVMGenSemispaceGeneration*)gen,
				       ee, gcOpts,
				       gen->allocMark, gen->allocPtr);
    gen->allocMark = gen->allocPtr; /* Advance mark for the next batch
				       of promotions */
    /*
     * Make sure that gen->allocPtr and gen->fromSpace->allocPtr are
     * in agreement (i.e. in the same space).
     */
    CVMassert(CVMgenSemispaceInOld((CVMGenSemispaceGeneration*)gen,
				   (CVMObject*)gen->allocPtr) ||
	      (gen->allocPtr == gen->allocTop));
    /*
     * This is the observable fromSpace pointer. We can probably
     * get away with not committing this pointer.
     */
    ((CVMGenSemispaceGeneration*)gen)->fromSpace->allocPtr = gen->allocPtr;
#else
    CVMassert(CVM_FALSE); /* Should never get here. */
#endif
}

/*
 * Copy numBytes from 'source' to 'dest'. Assume word copy, and assume
 * that the copy regions are disjoint.  
 */
static void
CVMgenSemispaceCopyDisjointWords(CVMUint32* dest, CVMUint32* source, 
				 CVMUint32 numBytes)
{
    CVMUint8* sourceEnd = (CVMUint8*)source + numBytes;
    while (source < (CVMUint32*)sourceEnd) {
	*dest++ = *source++;
    }
}

/* Promote 'objectToPromote' into the current generation. Returns
   the new address of the object, or NULL if the promotion failed. */
static CVMObject*
CVMgenSemispacePromoteInto(CVMGeneration* gen,
			   CVMObject* objectToPromote,
			   CVMUint32  objectSize)
{
#ifdef CVM_SEMISPACE_AS_OLDGEN
    CVMObject* dest;
    CVMgenContiguousSpaceAllocate(gen, objectSize, dest);
    if (dest != NULL) {
	CVMgenSemispaceCopyDisjointWords((CVMUint32*)dest,
					 (CVMUint32*)objectToPromote,
					 objectSize);
    }
    CVMtraceGcScan(("GC[SS,%d]: "
		    "Promoted object %x (len %d, class %C), to %x\n",
		    gen->generationNo,
		    objectToPromote, objectSize,
		    CVMobjectGetClass(objectToPromote),
		    dest));
    return dest;
#else
    CVMassert(CVM_FALSE); /* Should never get here. */
    return NULL;
#endif
}

static CVMGenSpace*
CVMgenSemispaceGetExtraSpace(CVMGeneration* gen)
{
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)gen;
    /*
     * Make sure it is unused
     */
    CVMassert(gen->allocBase != thisGen->toSpace->allocBase);
    return thisGen->toSpace;
}

/*
 * Allocate a semispaces generation. This includes the actual semispaces
 * as well as all the supporting data structures.
 */
CVMGeneration*
CVMgenSemispaceAlloc(CVMUint32* space, CVMUint32 totalNumBytes)
{
    CVMGenSemispaceGeneration* thisGen  = (CVMGenSemispaceGeneration*)
	calloc(sizeof(CVMGenSemispaceGeneration), 1);
    CVMUint32 numBytes = totalNumBytes / 2; /* Two equal-sized semispaces */
    CVMGenSpace* fromSpace;
    CVMGenSpace* toSpace;

    if (thisGen == NULL) {
	return NULL;
    }
    fromSpace     = CVMgenOneSemispaceAlloc(space, numBytes);
    toSpace       = CVMgenOneSemispaceAlloc(space + numBytes / 4, numBytes);
    
    if ((fromSpace == NULL) || (toSpace == NULL)) {
	free(thisGen);
	if (fromSpace != NULL) {
	    free(fromSpace);
	}
	if (toSpace != NULL) {
	    free(toSpace);
	}
	return NULL;
    }

    /*
     * Initialize thisGen. 
     */
    thisGen->fromSpace = fromSpace;
    thisGen->toSpace   = toSpace;
    thisGen->gen.heapBase = space;
    thisGen->gen.heapTop = space + totalNumBytes / 4;

    /*
     * Start the world on fromSpace
     */
    CVMgenSemispaceMakeCurrent(thisGen, fromSpace);

    /* 
     * And finally, set the function pointers for this generation
     */
    thisGen->gen.collect = CVMgenSemispaceCollect;
    thisGen->gen.scanOlderToYoungerPointers =
	CVMgenSemispaceScanOlderToYoungerPointers;
    thisGen->gen.scanYoungerToOlderPointers =
	CVMgenSemispaceScanYoungerToOlderPointers;
    thisGen->gen.promoteInto = 
	CVMgenSemispacePromoteInto;
    thisGen->gen.scanPromotedPointers =
	CVMgenSemispaceScanPromotedPointers;
    thisGen->gen.getExtraSpace = 
	CVMgenSemispaceGetExtraSpace;
    thisGen->gen.startGC = CVMgenSemispaceStartGC;
    thisGen->gen.endGC = CVMgenSemispaceEndGC;
    thisGen->gen.freeMemory = CVMgenSemispaceFreeMemory;
    thisGen->gen.totalMemory = CVMgenSemispaceTotalMemory;
    
    CVMdebugPrintf(("GC[SS]: Initialized semi-space gen for generational GC\n"));
    CVMdebugPrintf(("\tSize of *each* semispace in bytes=%d\n"
		  "\tLimits of generation = [0x%x,0x%x)\n" 
		  "\tFirst semispace      = [0x%x,0x%x)\n" 
		  "\tSecond semispace     = [0x%x,0x%x)\n",
		  numBytes, fromSpace->allocBase, toSpace->allocTop,
		  fromSpace->allocBase, fromSpace->allocTop,
		  toSpace->allocBase, toSpace->allocTop));
    return (CVMGeneration*)thisGen;
}

/*
 * Free all the memory associated with the current semispaces generation
 */
void
CVMgenSemispaceFree(CVMGenSemispaceGeneration* thisGen)
{
    free(thisGen->fromSpace);
    free(thisGen->toSpace);

    thisGen->fromSpace = NULL;
    thisGen->toSpace   = NULL;

    free(thisGen);
}

static void
CVMgenSemispaceSetupCopying(CVMGenSemispaceGeneration* thisGen)
{
    CVMUint32 spaceSize = (CVMUint32)
	(thisGen->toSpace->allocTop - thisGen->toSpace->allocBase);
    /* Always copy from 'fromSpace' to 'toSpace' */
    thisGen->copyBase = thisGen->toSpace->allocBase;
    thisGen->copyTop  = thisGen->copyBase;
    thisGen->copyThreshold = thisGen->toSpace->allocTop - spaceSize / 4;
}

/*
 * GC done. Flip the semispaces.
 */
static void
CVMgenSemispaceFlip(CVMGenSemispaceGeneration* thisGen)
{
    CVMGenSpace* newCurrent = thisGen->toSpace;
    thisGen->toSpace = thisGen->fromSpace;
    thisGen->fromSpace = newCurrent;

    newCurrent->allocPtr = thisGen->copyTop;
    CVMgenSemispaceMakeCurrent(thisGen, newCurrent);
}

/*
 * Copy object, leave forwarding pointer behind
 */
static CVMObject*
CVMgenSemispaceForwardOrPromoteObject(CVMGenSemispaceGeneration* thisGen,
				      CVMObject* ref, CVMAddr classWord)
{
    CVMClassBlock*  objCb      = CVMobjectGetClassFromClassWord(classWord);
    CVMUint32       objSize    = CVMobjectSizeGivenClass(ref, objCb);
    CVMUint32*      copyTop    = thisGen->copyTop;

    CVMObject* ret;
    CVMBool copyIntoToSpace;

    /* 
     * If this is the old generation, copy into to-space unconditionally.
     *
     * If it is not the old generation, try promotions as well.  Copy
     * into to-space if object age below the desired threshold. If
     * above the threshold, try promotion. If promotion fails, copy
     * into to-space anyway.  
     */
#ifdef CVM_SEMISPACE_AS_OLDGEN
    if (!CVMgenSemispaceIsYoung(thisGen)) {
	ret = NULL;
	copyIntoToSpace = CVM_TRUE;
    } else
#endif
    if (CVMobjGcBitsPlusPlusCompare(ref, CVM_GEN_PROMOTION_THRESHOLD)) {
	ret = NULL;
	copyIntoToSpace = CVM_TRUE;
    } else {
	/* Try promotion */
	ret = thisGen->gen.nextGen->promoteInto(thisGen->gen.nextGen,
						ref, objSize);
	if (ret == NULL) {
            thisGen->hasFailedPromotion = CVM_TRUE;
	    copyIntoToSpace = CVM_TRUE;
	} else {
	    copyIntoToSpace = CVM_FALSE;
	}
    }
    if (copyIntoToSpace) {
	CVMUint32*      newCopyTop = copyTop + objSize / 4;
	CVMtraceGcCollect(("GC[SS,%d]: "
			   "Forwarding object %x (len %d, class %C), to %x\n",
			   thisGen->gen.generationNo,
			   ref, objSize, objCb, thisGen->copyTop));
	CVMgenSemispaceCopyDisjointWords(copyTop, (CVMUint32*)ref, objSize);
	thisGen->copyTop = newCopyTop;
	ret = (CVMObject*)copyTop;
    }
    CVMassert(ret != NULL);

#ifdef CVM_JVMPI
    if (CVMjvmpiEventObjectMoveIsEnabled()) {
        CVMjvmpiPostObjectMoveEvent(1, ref, 1, ret);
    }
#endif
#ifdef CVM_INSPECTOR
    if (CVMglobals.inspector.hasCapturedState) {
        CVMgcHeapStateObjectMoved(ref, ret);
    }
#endif

    /*
     * Set forwarding pointer
     */
    {
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
CVMgenSemispaceGrayObject(CVMGenSemispaceGeneration* thisGen,
			  CVMObject** refPtr, CVMObject *ref)
{
    CVMAddr  classWord = CVMobjectGetClassWord(ref);

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
    CVMassert(CVMgenSemispaceInOld(thisGen, ref));

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
	*refPtr = CVMgenSemispaceForwardOrPromoteObject(thisGen, ref,
							classWord);
    }
}

/*
 * Gray a slot if it points into this generation.
 */
static void
CVMgenSemispaceFilteredGrayObject(CVMGenSemispaceGeneration* thisGen,
				  CVMObject** refPtr,
				  CVMObject*  ref)
{
    /*
     * Gray only if the slot points to this generation's from space. 
     *
     * Make sure we don't scan roots twice!
     */
    CVMassert(CVMobjectIsInROM(ref) ||
	      !CVMgenInGeneration((CVMGeneration*)thisGen, ref) ||
	      CVMgenSemispaceInOld(thisGen, ref));
    if (CVMgenSemispaceInOld(thisGen, ref)) {
        CVMgenSemispaceGrayObject(thisGen, refPtr, ref);
    }
}

/* 
 * A version of the above where multiple scans of the same slot are allowed.
 * Used for CVMgenSemispaceScanTransitively() only.
 */
static void
CVMgenSemispaceFilteredGrayObjectWithMS(CVMGenSemispaceGeneration* thisGen,
					CVMObject** refPtr,
					CVMObject*  ref)
{
    /*
     * Gray only if the slot points to this generation's from space. 
     */
    if (CVMgenSemispaceInOld(thisGen, ref)) {
	CVMgenSemispaceGrayObject(thisGen, refPtr, ref);
    }
}

/*
 * Scan a non-NULL reference into this generation. Make sure to
 * handle the case that the slot has already been scanned -- the
 * root scan should allow for such a case.
 */
static void
CVMgenSemispaceFilteredHandleRoot(CVMObject** refPtr, void* data)
{
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)data;
    CVMObject* ref = *refPtr;
    CVMassert(refPtr != 0);
    CVMassert(ref != 0);

    CVMtraceGcCollect(("GC[SS,%d]: Handling external reference: [%x] = %x\n",
		       thisGen->gen.generationNo,
		       refPtr, ref));
    CVMgenSemispaceFilteredGrayObject(thisGen, refPtr, ref);
}

static void
CVMgenSemispaceBlackenObjectFull(CVMGenSemispaceGeneration* thisGen,
				 CVMExecEnv* ee, 
				 CVMGCOptions* gcOpts, CVMObject* ref,
				 CVMClassBlock* refCb)
{
    CVMassert(ref != 0);

    CVMtraceGcCollect(("GC[SS,%d,full]: Blacken object %x\n",
		       thisGen->gen.generationNo,
		       ref));
    /*
     * Queue up all non-null object references. Handle the class as well.
     */
    CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, ref, refCb, {
	if (*refPtr != 0) {
	    CVMgenSemispaceFilteredGrayObject(thisGen, refPtr, *refPtr);
	}
    }, CVMgenSemispaceFilteredHandleRoot, thisGen);
}

/*
 * A function to blacken an object. Makes distinction between
 * "full" and "partial" blackening easier.
 */
typedef void (*CVMGenBlackenFunction)(CVMGenSemispaceGeneration* thisGen,
				      CVMExecEnv* ee, 
				      CVMGCOptions* gcOpts, CVMObject* ref,
				      CVMClassBlock* refCb);

static void
CVMgenSemispaceFollowRootsWithBlackener(CVMGenSemispaceGeneration* thisGen,
					CVMExecEnv* ee, CVMGCOptions* gcOpts,
					CVMGenBlackenFunction blackener)
{
    CVMUint32* copyBase = thisGen->copyBase;
#ifdef CVM_SEMISPACE_AS_OLDGEN
    CVMBool    isYoung = CVMgenSemispaceIsYoung(thisGen);
#endif
    CVMGeneration* nextGen = thisGen->gen.nextGen;
    CVMtraceGcCollect(("GC[SS,%d,full]: Following roots, "
		       "copyBase=%x, copyTop=%x\n",
		       thisGen->gen.generationNo,
		       thisGen->copyBase, thisGen->copyTop));
    do {
	/*
	 * First traverse the gray objects and blacken them
	 */
	while (copyBase < thisGen->copyTop) {
	    CVMObject* obj = (CVMObject*)copyBase;
	    CVMClassBlock* objCb = CVMobjectGetClass(obj);
	    CVMUint32 objSize = CVMobjectSizeGivenClass(obj, objCb);
            CVMgenSemispaceBlackenObjectFull(thisGen, ee, gcOpts, obj, objCb);
	    copyBase += objSize / 4;
	}
#ifdef CVM_SEMISPACE_AS_OLDGEN
	if (isYoung) 
#endif
        {
	    if (nextGen->allocPtr > nextGen->allocMark) {
		/* For a young generation, the scan above may have
		   promoted objects. Ask the next generation to scan
		   those objects for pointers into the current */
		nextGen->scanPromotedPointers(nextGen,
		    ee, gcOpts, CVMgenSemispaceFilteredHandleRoot, thisGen);
		/* And of course, scanning the promoted pointers gives
		   us more gray objects to traverse and or more more
		   promoted pointers to scan. Do it. */
	    }
	}
    } while (copyBase < thisGen->copyTop);

    thisGen->copyBase = copyBase;
    CVMassert(thisGen->copyBase == thisGen->copyTop);
}

static void
CVMgenSemispaceFollowRootsFull(CVMGenSemispaceGeneration* thisGen,
			   CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMgenSemispaceFollowRootsWithBlackener(thisGen, ee, gcOpts,
        CVMgenSemispaceBlackenObjectFull);
}

typedef struct CVMGenSemispaceTransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
    CVMGenSemispaceGeneration* thisGen;
} CVMGenSemispaceTransitiveScanData;

static void
CVMgenSemispaceScanTransitively(CVMObject** refPtr, void* data)
{
    CVMGenSemispaceTransitiveScanData* tsd =
	(CVMGenSemispaceTransitiveScanData*)data;
    CVMGenSemispaceGeneration* thisGen = tsd->thisGen;
    CVMObject* ref;

    CVMassert(refPtr != 0);
    ref = *refPtr;
    CVMassert(ref != 0);

    CVMgenSemispaceFilteredGrayObjectWithMS(thisGen, refPtr, ref);

    /*
     * Now scan all its children as well
     */
    CVMgenSemispaceFollowRootsFull(thisGen, tsd->ee, tsd->gcOpts);
}

static void
CVMgenSemispaceScanInternedStringsTransitively(CVMObject** refPtr, void* data)
{
    CVMGenSemispaceTransitiveScanData* tsd =
	(CVMGenSemispaceTransitiveScanData*)data;
    CVMGenSemispaceGeneration* thisGen = tsd->thisGen;

    CVMgenSemispaceScanTransitively(refPtr, data);

    /* Since this is a scanner that is only used for the interned strings
       table, if we encounter a ref that is in the youngGen, then we must
       have encountered an interned string or its sub data structures in
       the youngGen heap.  Make a note of this for the next GC cycle. */
    if (CVMgenSemispaceInSpace(thisGen->toSpace, *refPtr)) {
        CVMglobals.gc.hasYoungGenInternedStrings = CVM_TRUE;
    }
}

/*
 * Test whether a given reference is live or dying. If the reference
 * does not point to the current generation, answer TRUE. The generation
 * that does contain the pointer will give the right answer.
 */
static CVMBool
CVMgenSemispaceRefIsLive(CVMObject** refPtr, void* data)
{
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)data;
    CVMObject* ref;

    CVMassert(refPtr != NULL);

    ref = *refPtr;
    CVMassert(ref != NULL);

#ifdef CVM_INSPECTOR
    /* If the VM inspector wants to force all objects to be alive, then we'll
       say that the object is alive regardless of whether it is reachable or
       not: */
    if (CVMglobals.inspector.keepAllObjectsAlive) {
        return CVM_TRUE;
    }
#endif

    /* Check to see if the object is alive by checking:

       // ROM objects are always live
       if (CVMobjectIsInROM(ref)) {
	   return CVM_TRUE;
       }

       // If this reference is not in this generation, just assume it's live.
       if (!CVMgenInGeneration((CVMGeneration*)thisGen, ref)) {
           return CVM_TRUE;
       }
       return !CVMgenSemispaceInOld(thisGen, ref) || CVMobjectMarked(ref);

    The above basically says that:
       If the object is ROMized, it is alive.
       If it's in the oldGen or outside this generation, it is alive.
       If it's in this generation, then we check which space it is in:
          If it's in the current space, then it must be alive because
             it got forwarded.
          If it's in the old space, then it might be alive only if it
             has already been marked.

    Summarizing the above logic, we see that the object is only known
    to be not alive if it's in the old space and is not marked.  We
    can simplify this logic for efficiency by expressing in the form
    below:
    */
    if (CVMgenSemispaceInOld(thisGen, ref)) {
        return CVMobjectMarked(ref);
    }
    return CVM_TRUE;
}

#ifdef CVM_CLASSLOADING
static CVMBool
CVMgenSemispaceClassRefIsLive(CVMObject** refPtr, void* data)
{
    CVMGeneration* gen = (CVMGeneration*)data;
    CVMBool isLive;
    isLive = CVMgenSemispaceRefIsLive(refPtr, data);
    if (isLive && CVMgenInGeneration(gen, *refPtr)) {
        CVMglobals.gc.hasYoungGenClassesOrLoaders = CVM_TRUE;
    }
    return isLive;
}
#endif

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
/* Purpose: Scan over freed objects. */
static void
CVMgenSemispaceScanFreedObjects(CVMGeneration *thisGen, CVMExecEnv *ee)
{
    CVMUint32 *base = thisGen->allocBase;
    CVMUint32 *top = thisGen->allocPtr;

#ifndef CVM_JVMPI
    /* If this not a JVMPI build, we don't need to scan freed objects
       if we're not tracking any captured heap state: */
    if (!CVMglobals.inspector.hasCapturedState) {
        return;
    }
#endif

    CVMtraceGcCollect(("GC: CVMgenSemispaceScanFreedObjects, "
                       "base=%x, top=%x\n", base, top));
    while (base < top) {
        CVMObject *obj = (CVMObject*)base;
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
#ifdef CVM_INSPECTOR
            if (CVMglobals.inspector.hasCapturedState) {
	        CVMgcHeapStateObjectFreed(obj);
            }
#endif
            CVMtraceGcCollect(("GC: Freed object=0x%x, size=%d, class=%C\n",
                               obj, objSize, objCb));
        }
        base += objSize / 4;
    }
    CVMassert(base == top);
}

#else
#define CVMgenSemispaceScanFreedObjects(thisGen, ee)
#endif /* CVM_INSPECTOR || CVM_JVMPI */

static void
CVMgenSemispaceProcessSpecialWithLivenessInfo(CVMExecEnv* ee,
    CVMGCOptions* gcOpts, CVMGenSemispaceGeneration* thisGen)
{
    CVMGenSemispaceTransitiveScanData tsd;
    tsd.ee = ee;
    tsd.gcOpts = gcOpts;
    tsd.thisGen = thisGen;

    CVMgcProcessWeakrefWithLivenessInfo(ee, gcOpts,
        CVMgenSemispaceRefIsLive, thisGen,
	CVMgenSemispaceScanTransitively, &tsd);

    if (CVMglobals.gc.needToScanInternedStrings) {
        CVMgcProcessInternedStringsWithLivenessInfo(ee, gcOpts,
            CVMgenSemispaceRefIsLive, thisGen,
            CVMgenSemispaceScanInternedStringsTransitively, &tsd);
    }

    CVMdeleteUnreferencedMonitors(ee, CVMgenSemispaceRefIsLive, thisGen,
	CVMgenSemispaceScanTransitively, &tsd);

#ifdef CVM_CLASSLOADING
    if (CVMglobals.gcCommon.doClassCleanup) {
        CVMclassDoClassUnloadingPass1(ee,
            CVMgenSemispaceClassRefIsLive, thisGen,
            CVMgenSemispaceScanTransitively, &tsd, gcOpts);
    }
#endif
}

static CVMBool
CVMgenSemispaceCollect(CVMGeneration* gen,
		       CVMExecEnv*    ee, 
		       CVMUint32      numBytes, /* collection goal */
		       CVMGCOptions*  gcOpts)
{
    CVMGenSemispaceGeneration* thisGen = (CVMGenSemispaceGeneration*)gen;
    CVMGenSpace* space;
    CVMBool success;

    gcOpts->discoverWeakReferences = CVM_TRUE;
    thisGen->hasFailedPromotion = CVM_FALSE;

    CVMtraceGcStartStop(("GC[SS,%d,full]: Collecting from 0x%x to 0x%x\n",
			 thisGen->gen.generationNo,
			 thisGen->fromSpace->allocBase, 
			 thisGen->toSpace->allocBase));

    CVMgenSemispaceSetupCopying(thisGen);

    /*
     * Scan all roots that point to this generation.
     */
    CVMgenScanAllRoots((CVMGeneration*)thisGen,
		       ee, gcOpts, CVMgenSemispaceFilteredHandleRoot, thisGen);

    /*
     * Now start from the roots, and copy or promote all live data 
     */
    CVMgenSemispaceFollowRootsFull(thisGen, ee, gcOpts);

    /*
     * Don't discover any more weak references.
     */
    gcOpts->discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    CVMgenSemispaceProcessSpecialWithLivenessInfo(ee, gcOpts, thisGen);

    CVMgenSemispaceScanFreedObjects((CVMGeneration*)thisGen, ee);

    /*
     * All relevant live data has been copied to toSpace. Flip the spaces.
     */
    CVMgenSemispaceFlip(thisGen);

    /*
     * Now if we are not the young generation, we have to rebuild the
     * barrier structures.
     */
    space = thisGen->fromSpace;
#ifdef CVM_SEMISPACE_AS_OLDGEN
    if (!CVMgenSemispaceIsYoung(thisGen)) {
	CVMgenClearBarrierTable();
	CVMgenSemispaceRebuildBarrierTable(thisGen, ee, gcOpts,
					   space->allocBase,
					   space->allocPtr);
    }
#endif
    success = (numBytes / 4 <= (CVMUint32)(space->allocTop - space->allocPtr))
              && !thisGen->hasFailedPromotion;

    CVMtraceGcStartStop(("GC[SS,%d,full]: Done, success for %d bytes %s\n",
			 thisGen->gen.generationNo,
			 numBytes,
			 success ? "TRUE" : "FALSE"));

    /*
     * Have we processed everything?
     */
    CVMassert((gen->nextGen == NULL) ||
	      (gen->nextGen->allocMark == gen->nextGen->allocPtr));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;
}
