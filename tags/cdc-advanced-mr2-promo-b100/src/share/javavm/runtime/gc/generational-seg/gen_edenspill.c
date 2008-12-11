/*
 * @(#)gen_edenspill.c	1.33 06/10/10
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

#include "javavm/include/gc/generational-seg/gen_edenspill.h"

#include "javavm/include/porting/system.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

static CVMUint32* 
sweep(CVMGenEdenSpillGeneration* thisGen, 
      CVMExecEnv* ee, 
      CVMGCOptions* gcOpts,
      CVMUint32* base, 
      CVMUint32* top) ;

static void 
scanPromotedPointers(CVMGeneration* gen,
		     CVMExecEnv* ee, 
		     CVMGCOptions* gcOpts,
		     CVMRefCallbackFunc callback,
		     void* callbackData) ;

/*
 * Range checks
 */

static CVMBool
inSpace(CVMGenSpace* space, CVMObject* ref)
{
    /* Note : Eden-spill gen is faking segmentation so
       bounds checking doesnt account for displaced segments
    */
    CVMUint32* heapPtr = (CVMUint32*)ref;
    CVMGenSegment* segBase = (CVMGenSegment*) space->allocBase ;
    CVMGenSegment* segTop  = segBase->prevSeg ;

    return ((heapPtr >= segBase->allocBase) &&
            (heapPtr <  segTop->allocTop));
}

static CVMBool 
inGeneration(CVMGeneration* gen, CVMObject* ref)
{
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)gen;
    return inSpace(thisGen->theSpace, ref);
}

static CVMBool 
inThisGeneration(CVMGenEdenSpillGeneration* thisGen, CVMObject* ref)
{
    return inSpace(thisGen->theSpace, ref);
}

/*
 * Allocate an eden-spill space
 */
static CVMGenSpace*
allocSpace(CVMUint32* heapSpace, CVMUint32 numBytes)
{
    CVMUint32 allocSize = sizeof(CVMGenSpace);
    CVMGenSpace* space = (CVMGenSpace*)calloc(1, allocSize);

    CVMGenSegment* allocBase = NULL;
    CVMGenSegment* allocTop  = NULL;
    CVMGenSegment* segCurr   = NULL;
    CVMUint32 numSegments = numBytes/CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES;

    if (space == NULL) {
        return NULL;
    }
    CVMassert(heapSpace != NULL);

    CVMassert(numBytes != 0); /* spillGen only */
#if 0
    /* numSegments can be 0 if we allow for segment chunks and numBytes to
       be less than CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES.  Since this is
       currently not the case, this code is unused for the moment. */
    if (numSegments == 0) {
        /* The only time we have segments that aren't packed to
           CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES is when the numBytes is less
           than CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES to begin with.  Hence,
           we will only have one segment. */
        segCurr = CVMgenAllocSegment(&allocBase, &allocTop, heapSpace,
                                     numBytes, SEG_TYPE_NORMAL);
        CVMassert(segCurr != NULL && allocBase != NULL);
    } else 
#endif
    {
        /* Otherwise, we should have enough bytes to fill out one or more
           properly packed segments. */
        CVMassert(numBytes ==
                  CVMpackSizeBy(numBytes, CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

        while (numSegments-- > 0) {
	    /* NOTE: We don't actually go ahead and ask for
               CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES because we have to account
               for misc overhead in setting up the segment.  In this case, we
               ask for CVM_GCIMPL_SEGMENT_USABLE_BYTES +
               CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD because we know that the
               block of memory is already preallocated as a consecutive block
               by the caller.  Hence, we will not need to reserve any space
               for the malloc overhead between each segment. Hence, we can
               add CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD back into the usable
               space.
            */
            segCurr = CVMgenAllocSegment(&allocBase, &allocTop, heapSpace,
                                         CVM_GCIMPL_SEGMENT_USABLE_BYTES +
					 CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD,
                                         SEG_TYPE_NORMAL);
            CVMassert(segCurr != NULL && allocBase != NULL);
            heapSpace += CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES / 4;
        }
    }
 
    CVMassert(allocBase != NULL && allocTop != NULL) ;

    space->allocTop  = (CVMUint32*)allocTop;
    space->allocPtr  = (CVMUint32*)allocBase;
    space->allocBase = (CVMUint32*)allocBase;

    return space;
}

/*
 * Make sure the allocation pointers of the mark-compact generation
 * are reflected at the "superclass" level for the shared GC code.
 */
static void
makeCurrent(CVMGenEdenSpillGeneration* thisGen, CVMGenSpace* thisSpace)
{
    CVMGeneration* super = &thisGen->gen;
    super->allocPtr  = thisSpace->allocPtr;
    super->allocBase = thisSpace->allocBase;
    super->allocTop  = thisSpace->allocTop;
    super->allocMark = thisSpace->allocPtr;
}

static CVMGenSpace*
getExtraSpace(CVMGeneration* gen)
{
    return gen->prevGen->getExtraSpace(gen->prevGen);
}

/* Forward declaration */
static CVMBool
collectGeneration(CVMGeneration* gen,
		  CVMExecEnv*    ee, 
		  CVMUint32      numBytes, /* collection goal */
		  CVMGCOptions*  gcOpts);

/*
 * Given the address of a slot, mark the card table entry
 */
static CVMBool
inYounger(CVMGeneration* gen, CVMObject* ref)
{
    CVMGeneration* prevGen = gen->prevGen;
    return prevGen->inGeneration(prevGen, ref);
}

/*
 * Scan objects in contiguous range, and do all special handling as well.
 */
static void
scanObjectsInRange(CVMExecEnv* ee, CVMGCOptions* gcOpts,
		   CVMUint32* base, CVMUint32* top,
		   CVMRefCallbackFunc callback, void* callbackData)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,full]: "
		       "Scanning object range (full) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, currObj, currCb, {
	    if (*refPtr != 0) {
		(*callback)(refPtr, callbackData);
	    }
	}, callback, callbackData);
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
    return;
}

/* Forward declartion*/
static void
filteredUpdateRoot(CVMObject** refPtr, void* data);

/*
 * Scan objects in contiguous range, and do all special handling as well.
 * Ignore unmarked objects.
 */
static void
scanObjectsAndFixPointers(CVMGenEdenSpillGeneration* thisGen,
                          CVMExecEnv* ee, CVMGCOptions* gcOpts,
                          CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,full]: "
		       "Scanning object range (skip unmarked) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj    = (CVMObject*)curr;
	/* 
	 * member clas and various32 hold a native pointer
	 * therefore use type CVMAddr (instead of CVMUint32)
	 * which is 64 bit aware
	 */
	CVMAddr    classWord  = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	if (CVMobjectMarkedOnClassWord(classWord)) {
	    CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, currObj,
						 classWord, {
		if (*refPtr != 0) {
		    filteredUpdateRoot(refPtr, thisGen);
		}
	    }, filteredUpdateRoot, thisGen);
	}
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

static void       
scanYoungerToOlderPointers(CVMGeneration* gen,
			   CVMExecEnv* ee,
			   CVMGCOptions* gcOpts,
			   CVMRefCallbackFunc callback,
			   void* callbackData)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase;

    do {
        scanObjectsInRange(ee, gcOpts, segCurr->allocBase, segCurr->allocPtr,
                           callback, callbackData);
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);
}

static void       
scanOlderToYoungerPointers(CVMGeneration* gen,
			   CVMExecEnv* ee,
			   CVMGCOptions* gcOpts,
			   CVMRefCallbackFunc callback,
			   void* callbackData)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: "
		       "Scanning older to younger ptrs\n",
		       gen->generationNo));
    /*
     * To get all older to younger pointers, traverse the records in
     * the write barrier tables.
     */
    CVMgenBarrierPointersTraverse(gen, ee, gcOpts, callback, callbackData);
    return;

}

/*
 * Start and end GC for this generation. 
 */

static void
startGC(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
    CVMGenSegment* segCurr = segBase ;

    do {
        CHECK_SEGMENT(segCurr) ;
        segCurr->rollbackMark = segCurr->allocPtr;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;

    ((CVMGenEdenSpillGeneration*)gen)->promotions = CVM_FALSE;

    CVMtraceGcCollect(("GC[MC,%d,full]: Starting GC\n",
		       gen->generationNo));
}

static void
endGC(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: Ending GC\n",
                       gen->generationNo));
}

/*
 * Return remaining amount of memory
 */
static CVMUint32
freeMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase ;
    CVMUint32   freeMemorySum = 0 ;

    do {
	CVMUint32 available = (CVMUint32)((CVMUint8*)segCurr->allocTop -
					  (CVMUint8*)segCurr->allocPtr);
        freeMemorySum += available ;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase);

    return freeMemorySum;
}

static CVMUint32
totalMemory(CVMGeneration* gen, CVMExecEnv* ee)
{ 
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase ;
    CVMUint32   totalMemorySum = 0 ; 
 
    do {  
	CVMUint32 size = (CVMUint32)((CVMUint8*)segCurr->allocTop -
				     (CVMUint8*)segCurr->allocBase);
        totalMemorySum += size ; 
        segCurr = segCurr->nextSeg ;
    }while (segCurr != segBase) ;
 
    return totalMemorySum; 
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
scanPromotedPointers(CVMGeneration* gen,
		     CVMExecEnv* ee,
		     CVMGCOptions* gcOpts,
		     CVMRefCallbackFunc callback,
		     void* callbackData)
{
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)gen;
    CVMGeneration* oldGen = gen->nextGen;
    CVMGenSegment* segBase = (CVMGenSegment*) gen->allocBase;
    CVMGenSegment* segTop  = (CVMGenSegment*) gen->allocTop;
    CVMGenSegment* segCurr   = segBase ;
    CVMBool scanned          = CVM_FALSE ;
 
    /* Store the allocMark for all the segments before
     * scanning. The allocMarks themselves would be used 
     * for rebuilding the barrier table
     */  
    do { 
        segCurr->cookie = segCurr->allocMark;
	/* Remember what this scan looked like, if work is to be done */
	segCurr->logMarkStart = segCurr->allocMark;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;

    do {
        CVMUint32* endRange   = segCurr->allocPtr;
        scanned = CVM_FALSE ;
        CHECK_SEGMENT(segCurr) ;

        CVMtraceGcCollect(("GC[MC,full]: "
		           "Scanning promoted ptrs [%x,%x)\n",
		           segCurr->cookie, segCurr->allocPtr));
        
        /* loop, until promoted object scans result in no more promotions */
        while (segCurr->cookie < endRange) {
	    scanObjectsInRange(ee, gcOpts, segCurr->cookie,  
			       endRange, callback, callbackData);
            segCurr->cookie = endRange ; 
	    /* segCurr->allocPtr may have been incremented */
            endRange = segCurr->allocPtr ;
            scanned = CVM_TRUE ;
        } 
        CVMassert(segCurr->cookie == segCurr->allocPtr);

        oldGen->scanPromotedPointers(oldGen, ee, gcOpts, 
				     callback, callbackData);

        if (scanned || thisGen->promotions) {
            thisGen->promotions = CVM_FALSE;
            segCurr = segBase ;
        } else if (segCurr == segTop) {
            segCurr = NULL ;
        } else { 
            segCurr = segCurr->nextSeg ;
        }      
 
    } while (segCurr != NULL) ;
 
#ifdef CVM_DEBUG_ASSERTS
    {
	CVMGenSegment* oldSegBase = (CVMGenSegment*)oldGen->allocBase;
	CVMGenSegment* oldSegCurr = oldSegBase;
	do { 
	    CVMassert(oldSegCurr->allocMark  == oldSegCurr->allocPtr);
	    oldSegCurr = oldSegCurr->nextSeg ;
	} while (oldSegCurr != oldSegBase) ;
    }
#endif

 
    segCurr = segBase ;

#ifdef CVM_DEBUG_ASSERTS
    do { 
        CVMassert(segCurr->cookie == segCurr->allocPtr) ;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;
#endif   
     
    /* Incremental rebuild of the barrier table */
    CVMgenBarrierTableRebuild(gen, ee, gcOpts, CVM_FALSE);
    
#ifdef CVM_VERIFY_HEAP
    segBase = (CVMGenSegment*) gen->allocBase;
    segCurr = segBase ;
 
    do { 
	if (segCurr->logMarkStart != NULL) {
	    CVMgenVerifyObjectRangeForCardSanity(gen, ee, gcOpts,
						 segCurr->logMarkStart,
						 segCurr->allocPtr);
	}
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;
#endif

    /*
     * This is the observable fromSpace pointer. We can probably
     * get away with not committing this pointer.
     */
    ((CVMGenEdenSpillGeneration*)gen)->theSpace->allocPtr = gen->allocPtr;

    return;
}

static void
scanAndRollbackPromotions(CVMGeneration* gen,
			  CVMExecEnv* ee,
			  CVMGCOptions* gcOpts,
			  CVMRefCallbackFunc callback,
			  void* callbackData) 
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase;

    do {
        CHECK_SEGMENT(segCurr);
        CVMassert(segCurr->rollbackMark >= segCurr->allocBase);
        CVMassert(segCurr->rollbackMark <= segCurr->allocPtr);
        segCurr->allocPtr = segCurr->allocMark = segCurr->rollbackMark;
        /* 
         * Assume heap smaller than 4 GB for now.
         */
        segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4;
        segCurr->rollbackMark = NULL;
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);

    /* Scan all pointers in old-gen objects for pointers into
       young generation as well as the promoted space which will be
       rolled back shortly 
    */
    CVMgenScanBarrierObjects(gen, ee, gcOpts, callback, callbackData);
     
    /* Force the old generation also to rollback any promotions */
    gen->nextGen->scanAndRollbackPromotions(gen->nextGen, ee, gcOpts, callback, callbackData);

#ifdef CVM_VERIFY_HEAP
    verifySegmentsWalk((CVMGenSegment*)gen->allocBase) ;
#endif

    return;
}

/*
 * Copy numBytes from 'source' to 'dest'. Assume word copy, and assume
 * that the copy regions are disjoint.  
 */
static void
copyDisjointWords(CVMUint32* dest, CVMUint32* source, CVMUint32 numBytes)
{
    CVMUint32*       d = dest;
    const CVMUint32* s = source;
    CVMUint32        n = numBytes / 4; /* numWords to copy */

    do {
	*d++ = *s++;
    } while (--n != 0);
}

/*
 * Copy numBytes _down_ from 'source' to 'dest'. Assume word copy.
 */
static void
copyDown(CVMUint32* dest, CVMUint32* source, CVMUint32 numBytes)
{
    if (dest == source) {
	return;
    } else {
	CVMUint32*       d = dest;
	const CVMUint32* s = source;
	CVMUint32        n = numBytes / 4; /* numWords to copy */
	
	CVMassert(d < s); /* Make sure we are copying _down_ */
	do {
	    *d++ = *s++;
	} while (--n != 0);
    }
}


/* Promote 'objectToPromote' into the current generation. Returns
   the new address of the object, or NULL if the promotion failed. */
static CVMObject*
promoteInto(CVMGeneration* gen, CVMObject* objectToPromote,
	    CVMUint32 objectSize)
{
    CVMObject* dest = NULL;

    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = (CVMGenSegment*)gen->allocPtr;
    do {
        if (segCurr->available >= objectSize) {
            dest = (CVMObject*) segCurr->allocPtr;
            segCurr->allocPtr = segCurr->allocPtr + objectSize / 4;
            segCurr->available -= objectSize;
            CVMassert(segCurr->available >= 0);
            break;
        }
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);

    if (dest != NULL) {
        copyDisjointWords((CVMUint32*)dest,
                                         (CVMUint32*)objectToPromote,
                                         objectSize);
        gen->allocPtr = (CVMUint32*)segCurr;

        CVMtraceGcScan(("GC[MC,%d]: "
                    "Promoted object %x (len %d, class %C), to %x\n",
                    gen->generationNo,
                    objectToPromote, objectSize,
                    CVMobjectGetClass(objectToPromote),
                    dest));
    } else {
        CVMGeneration *nextGen = gen->nextGen;
        dest = nextGen->promoteInto(nextGen, objectToPromote, objectSize);
    }

    ((CVMGenEdenSpillGeneration*)gen)->promotions = (dest != NULL);
    return dest;
}

#ifdef CVM_VERIFY_HEAP

#if 0 /* Currently unused. */
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
#endif

static void 
verifyGeneration(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase;

    do {  
        CVMUint32* top  = segCurr->allocPtr;
        CVMUint32* curr = segCurr->allocBase;
	CVMgenVerifyObjectRangeForCardSanity(gen, ee, gcOpts,
					     curr, top);
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);
}
#endif

#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
static CVMBool iterateGen(CVMGeneration* gen, CVMExecEnv* ee,
			  CVMObjectCallbackFunc callback,
			  void* callbackData)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr = segBase;
    CVMBool completeScanDone;

    do {
        completeScanDone = 
	    CVMgcScanObjectRange(ee, segCurr->allocBase, segCurr->allocPtr,
				 callback, callbackData);
        segCurr = segCurr->nextSeg;
    } while (completeScanDone && (segCurr != segBase));
    return completeScanDone;
}
#endif

/*
 * Allocate a Eden-spill generation.
 */
CVMGeneration*
CVMgenEdenSpillAlloc(CVMUint32* space, CVMUint32 totalNumBytes)
{
    CVMGenEdenSpillGeneration* thisGen  = (CVMGenEdenSpillGeneration*)
        calloc(sizeof(CVMGenEdenSpillGeneration), 1);
    CVMUint32 numBytes = totalNumBytes; /* Respect the no. of bytes passed */
    CVMGenSpace* theSpace;

    if (thisGen == NULL) {
	return NULL;
    }

    /*
     * Start the heap with a pre-fixed heap
     */
    theSpace = allocSpace(space, numBytes);
    
    if (theSpace == NULL) {
	free(thisGen);
	return NULL;
    }

    /*
     * Initialize thisGen. 
     */
    thisGen->theSpace = theSpace;

    /*
     * Setup the bounds of the generation
     */
    thisGen->gen.genBase = space;
    thisGen->gen.genTop  = space + numBytes / 4;

    /*
     * Start the world on theSpace
     */
    makeCurrent(thisGen, theSpace);

    /* 
     * And finally, set the function pointers for this generation
     */
    thisGen->gen.collect = collectGeneration;
    thisGen->gen.scanOlderToYoungerPointers = scanOlderToYoungerPointers;
    thisGen->gen.scanYoungerToOlderPointers = scanYoungerToOlderPointers;
    thisGen->gen.promoteInto = promoteInto;
    thisGen->gen.scanPromotedPointers =	scanPromotedPointers;
    thisGen->gen.inGeneration = inGeneration;
    thisGen->gen.inYounger = inYounger;
    thisGen->gen.getExtraSpace = getExtraSpace;
    thisGen->gen.startGC = startGC;
    thisGen->gen.endGC = endGC;
    thisGen->gen.freeMemory = freeMemory;
    thisGen->gen.totalMemory = totalMemory;
    thisGen->gen.scanAndRollbackPromotions = scanAndRollbackPromotions;

#ifdef CVM_VERIFY_HEAP
    thisGen->gen.verifyGen = verifyGeneration;
#endif
    
#if defined(CVM_DEBUG) || defined(CVM_JVMPI)
    thisGen->gen.iterateGen = iterateGen;
#endif

    CVMdebugPrintf(("GC[ES]: Initialized Eden Spill mark-compact gen "
                  "for generational GC\n"));
    CVMdebugPrintf(("\tSize of the space in bytes=%d\n"
		    "\tFirst segment = 0x%x\n",
		  numBytes, theSpace->allocBase));
    return (CVMGeneration*)thisGen;
}

/*
 * Free all the memory associated with the current mark-compact generation
 */
void
CVMgenEdenSpillFree(CVMGenEdenSpillGeneration* thisGen)
{
    free(thisGen->theSpace);
    thisGen->theSpace  = NULL;
    free(thisGen);
}

static CVMObject*
getForwardingPtr(CVMGenEdenSpillGeneration* thisGen, CVMObject* ref)
{
    /*
     * member clas and various32 hold a native pointer
     * therefore use type CVMAddr (instead of CVMUint32)
     * which is 64 bit aware
     */
    CVMAddr  forwardingAddress = CVMobjectVariousWord(ref);
    CVMassert(inThisGeneration(thisGen, ref));
    CVMassert(CVMobjectMarked(ref));
    return (CVMObject*)forwardingAddress;
}

#ifdef MAX_STACK_DEPTHS
static CVMUint32 preservedIdxMax = 0;
#endif

static void
preserveHeaderWord(CVMGenEdenSpillGeneration* thisGen,
                   CVMObject* movedRef, CVMAddr originalWord);

static void
addToTodo(CVMGenEdenSpillGeneration* thisGen, CVMObject* ref)
{
    volatile CVMAddr *headerAddr = &CVMobjectVariousWord(ref);
    CVMAddr originalWord = *headerAddr;
    CVMAddr next = (CVMAddr)thisGen->todoList;

    if (!CVMobjectTrivialHeaderWord(originalWord)) {
        /* Preserve the old header word with the new address
           of object, but only if it is non-trivial. */
        preserveHeaderWord(thisGen, ref, originalWord);
        next |= 0x1;
    }

    /* Add the object to the todoList: */
    *headerAddr = next;
    thisGen->todoList = ref;
}

/*
 * Restore all preserved header words.
 */
static void
restorePreservedHeaderWords(CVMGenEdenSpillGeneration* thisGen)
{
    CVMInt32 idx;
    CVMtraceGcScan(("GC[MC,%d]: Restoring %d preserved headers\n",
		    thisGen->gen.generationNo,
		    thisGen->preservedIdx));
    while ((idx = --thisGen->preservedIdx) >= 0) {
	CVMGenPreservedItem* item = &thisGen->preservedItems[idx];
	CVMObject* movedRef = item->movedRef;
	/*
	 * Make sure that the object we are looking at now has the default
	 * various word set up for now
	 */
	CVMassert(CVMobjectTrivialHeaderWord(CVMobjectVariousWord(movedRef)));
	/*
	 * And also, that we did not evacuate this word for nothing
	 */
	CVMassert(!CVMobjectTrivialHeaderWord(item->originalWord));
	CVMobjectVariousWord(movedRef) = item->originalWord;
	CVMtraceGcScan(("GC[MC,%d]: "
			"Restoring header %x for obj 0x%x at i=%d\n",
			thisGen->gen.generationNo, item->originalWord,
			movedRef, idx));
    }
}

/*
 * Preserve a header word for an object in its new address.
 */
/*
 * member clas and various32 hold a native pointer
 * therefore use type CVMAddr (instead of CVMUint32)
 * which is 64 bit aware
 */
static void
preserveHeaderWord(CVMGenEdenSpillGeneration* thisGen,
                   CVMObject* movedRef, CVMAddr originalWord)
{
    CVMGenPreservedItem* item = &thisGen->preservedItems[thisGen->preservedIdx];

    /* Make sure we're not overflowing before writing to the item: */
    if (thisGen->preservedIdx >= thisGen->preservedSize) {
        CVMpanic("**** OVERFLOW OF PRESERVED HEADER WORDS" 
                 " in eden-spill GC!!! ****");
    }

    /* Write out the new item: */
    item->movedRef     = movedRef;
    item->originalWord = originalWord;
    CVMtraceGcScan(("GC[MC,%d]: Preserving header %x for obj 0x%x at i=%d\n",
		    thisGen->gen.generationNo, originalWord, movedRef,
		    thisGen->preservedIdx));
    thisGen->preservedIdx++;
#ifdef MAX_STACK_DEPTHS
    if (thisGen->preservedIdx >= preservedIdxMax) {
	preservedIdxMax = thisGen->preservedIdx;
	CVMconsolePrintf("**** preserved: max depth=%d\n", preservedIdxMax);
    }
#endif /* MAX_STACK_DEPTHS */
}

/* Sweep the heap, compute the compacted addresses, write them into the
   original object headers, and return the new allocPtr of this space. */

static CVMUint32*
sweep(CVMGenEdenSpillGeneration* thisGen,  
      CVMExecEnv* ee, 
      CVMGCOptions* gcOpts,
      CVMUint32* base, 
      CVMUint32* top)
{
    CVMGeneration* nextGen = thisGen->gen.nextGen;

    CVMGenSegment* segBase = (CVMGenSegment*) base ;
    CVMGenSegment* segCurr = segBase ;

    /* Keep objects in the same segment for now */
    CVMGenSegment* forwardingSeg = segBase ;
    CVMUint32* forwardingAddress = forwardingSeg->allocBase ;

    do {
        CVMUint32* curr = segCurr->allocBase;
        CVMUint32* top = segCurr->allocPtr ;
        CHECK_SEGMENT(segCurr) ;

        CVMtraceGcCollect(("GC[MC,%d]: Sweeping object range [%x,%x)\n",
		           thisGen->gen.generationNo, base, top));
        while (curr < top) {
	    CVMObject* currObj    = (CVMObject*)curr;
	    /* 
	     * member clas and various32 hold a native pointer
	     * therefore use type CVMAddr (instead of CVMUint32)
	     * which is 64 bit aware
	     */
	    CVMAddr    classWord  = CVMobjectGetClassWord(currObj);
	    CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	    CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	    
	    if (CVMobjectMarkedOnClassWord(classWord)) {
		/* 
		 * member clas and various32 hold a native pointer
		 * therefore use type CVMAddr (instead of CVMUint32)
		 * which is 64 bit aware
		 */
		volatile CVMAddr* headerAddr   = &CVMobjectVariousWord(currObj);
		CVMObject* copy = nextGen->promoteInto(nextGen,
						       currObj, objSize);
		if (copy != NULL) {
		    /* Copied the object to the next generation.
		       So no need to preserve the various word. Just
		       stuff the forwarding address in */
		    CVMobjectClearMarkedOnClassWord(classWord);
		    CVMobjectSetClassWord(copy, classWord);
		    /* 
		     * member clas and various32 hold a native pointer
		     * therefore use type CVMAddr (instead of CVMUint32)
		     * which is 64 bit aware
		     */
		    *headerAddr = (CVMAddr)copy;
		} else {
		    /* Could not promote. Keep in spill-gen. */
		    /* 
		     * member clas and various32 hold a native pointer
		     * therefore use type CVMAddr (instead of CVMUint32)
		     * which is 64 bit aware
		     */
		    CVMAddr  originalWord = *headerAddr;
		    
                    if ((forwardingAddress + objSize / 4) > 
			forwardingSeg->allocTop) {
			/* No more room in this segment. Find another. */
                        forwardingSeg->cookie = forwardingAddress ;
                        CHECK_SEGMENT(forwardingSeg) ;
                        forwardingSeg = forwardingSeg->nextSeg ;
                        forwardingAddress = forwardingSeg->allocBase ;
                        CHECK_SEGMENT(forwardingSeg) ;

			/* It would be very bad if we cycled
			   to our starting point, overwriting
			   the first copied objects! */
			CVMassert(forwardingSeg != segBase);
                    }
		    CVMtraceGcScan(("GC[MC,%d]: obj 0x%x -> 0x%x\n",
                                    thisGen->gen.generationNo, curr,
                                    forwardingAddress));
		    if (!CVMobjectTrivialHeaderWord(originalWord)) {
			/* Preserve the old header word with the new address
			   of object, but only if it is non-trivial. */
			preserveHeaderWord(thisGen, 
					   (CVMObject*)forwardingAddress, 
					   originalWord);
		    }
		    
		    /* Set forwardingAddress in header */
		    *headerAddr = (CVMAddr)forwardingAddress;
		    /* Pretend to have copied this live object! */
		    forwardingAddress += objSize / 4;
		    
		}
	    } else {
#ifdef CVM_JVMPI
		/* Object not marked. Count as dead. */
		extern CVMUint32 liveObjectCount;
		if (CVMjvmpiEventObjectFreeIsEnabled()) {
		    CVMjvmpiPostObjectFreeEvent(currObj);
		}
		liveObjectCount--;
#endif
#ifdef CVM_JVMTI
		/* Object not marked. Count as dead. */
		if (CVMjvmtiShouldPostObjectFree()) {
		    CVMjvmtiPostObjectFreeEvent(currObj);
		}
#endif
	    }
	    /* iterate */
	    curr += objSize / 4;
        } /* end of while */
        CVMassert(curr == top); /* This had better be exact */

        CVMtraceGcCollect(("GC[MC,%d]: Swept object range [%x,%x) -> 0x%x\n",
			   thisGen->gen.generationNo, base, top,
			   forwardingAddress));
	
    } while ((segCurr = segCurr->nextSeg) != segBase) ; /*do-while loop*/
    
    forwardingSeg->cookie = forwardingAddress ;
    CHECK_SEGMENT(forwardingSeg) ;
    return (CVMUint32*)forwardingSeg;
}

/* Finally we can move objects, reset marks, and restore original
   header words. */
static void 
compact(CVMGenEdenSpillGeneration* thisGen, CVMUint32* base, CVMUint32* top) 
{
    CVMGenSegment* segBase = (CVMGenSegment*) base;
    CVMGenSegment* segCurr = segBase;
    CVMtraceGcCollect(("GC[MC,%d]: Compacting object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));

    do {
        CVMUint32* curr = segCurr->allocBase;
        CVMUint32* top = segCurr->allocPtr ;
        CHECK_SEGMENT(segCurr) ;

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

            if (CVMobjectMarkedOnClassWord(classWord)) {
                CVMUint32* destAddr = (CVMUint32*)
                    getForwardingPtr(thisGen, currObj);

                /* The object could have been promoted to the old gen.
		   Copy down only if object is retained in spill-space */
                if (inSpace(thisGen->theSpace, (CVMObject*)destAddr)) {
                    CVMobjectClearMarkedOnClassWord(classWord);
                    copyDown(destAddr, curr, objSize);

                    /*
                     * First assume that all objects had the default various
                     * word. We'll patch the rest shortly.
                     * 
                     * Note that this clears the GC bits portion. No worries,
                     * we won't need that anymore in this generation.
                     */
                    CVMobjectVariousWord((CVMObject*)destAddr) =
                        CVM_OBJECT_DEFAULT_VARIOUS_WORD;
                    /* Write the class word whose mark has just been cleared */
                    CVMobjectSetClassWord((CVMObject*)destAddr, classWord);
                }                 
#ifdef CVM_JVMPI
                if (CVMjvmpiEventObjectMoveIsEnabled()) {
                    CVMjvmpiPostObjectMoveEvent(1, curr, 1, destAddr);
                }
#endif
            }
            /* iterate */
            curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */

    } while((segCurr = segCurr->nextSeg) != segBase) ; /*do-while loop*/
    /* Restore the "non-trivial" old header words
       with the new address of object */
    restorePreservedHeaderWords(thisGen);
}

/*
 * Gray an object known to be in the old MarkCompact generation.
 */
static void
grayObject(CVMGenEdenSpillGeneration* thisGen, CVMObject* ref)
{
    /*
     * ROM objects should have been filtered out by the time we get here
     */
    CVMassert(!CVMobjectIsInROM(ref));

    /*
     * Also, all objects that don't belong to this generation need to have
     * been filtered out by now.
     */
    CVMassert(inThisGeneration(thisGen, ref));

    /*
     * If this object has not been encountered yet, queue it up.
     */
    if (!CVMobjectMarked(ref)) {
        CVMClassBlock *cb = CVMobjectGetClass(ref);
	CVMobjectSetMarked(ref);
	/* queue it up */
	CVMtraceGcCollect(("GC[MC,%d]: Graying object %x\n",
			   thisGen->gen.generationNo, ref));

        /* If the object is an array of a basic type or if it is an instance
           of java.lang.Object, then it contains no references.  Hence, no
           need to queue it for further root traversal: */
        if (!CVMisArrayOfAnyBasicType(cb) &&
            (cb != CVMsystemClass(java_lang_Object))) {
            /* If we get here, then the object might contain some refs.  Need
               to queue it to check for refs later: */
            addToTodo(thisGen, ref);
        }
    }
}

/*
 * Gray a slot if it points into this generation.
 */
static void
filteredGrayObject(CVMGenEdenSpillGeneration* thisGen, CVMObject* ref)
{
    /*
     * Gray only if the slot points to this generation's from space. 
     *
     * Also handles the case of root slots that have been already
     * scanned and grayed, and therefore contain a pointer into
     * to-space -- those are ignored.
     */
    if (inThisGeneration(thisGen, ref)) {
	grayObject(thisGen, ref);
    }
}

static void
filteredGrayCallback(CVMObject** refPtr, void* data)
{
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)data;
    CVMObject* ref = *refPtr;
    filteredGrayObject(thisGen, ref);
    return;
}

static void
filteredUpdateRoot(CVMObject** refPtr, void* data)
{
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)data;
    CVMObject* ref = *refPtr;
    if (!inThisGeneration(thisGen, ref)) {
	return;
    }
    /* The forwarding address is valid only for already marked
       objects. If the object is not marked, then this slot has already
       been 'seen'. */
    CVMassert(CVMobjectMarked(ref));
    CVMassert(!CVMobjectIsInROM(ref));
    *refPtr = getForwardingPtr(thisGen, ref);
    return;
}

typedef struct TransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
    CVMGenEdenSpillGeneration* thisGen;
} TransitiveScanData;

static void
blackenObject(TransitiveScanData* tsd,
	      CVMObject* ref, CVMClassBlock* refCb)
{
    CVMGenEdenSpillGeneration* thisGen = tsd->thisGen;
    CVMExecEnv* ee = tsd->ee;
    CVMGCOptions* gcOpts = tsd->gcOpts;
    CVMassert(ref != 0);

    CVMtraceGcCollect(("GC[MC,%d,full]: Blacken object %x\n",
		       thisGen->gen.generationNo,
		       ref));
    /*
     * We'd better have grayed this object already.
     */
    CVMassert(CVMobjectMarked(ref));

    /*
     * Queue up all non-null object references. Handle the class as well.
     */
    CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, ref, refCb, {
        if (*refPtr != 0) {
           /*CVMassert(inGeneration(&thisGen->gen, *refPtr) ||
                     thisGen->gen.prevGen->inGeneration(thisGen->gen.prevGen, *refPtr) ||
                     thisGen->gen.nextGen->inGeneration(thisGen->gen.nextGen, *refPtr) ||
                    CVMobjectIsInROM(*refPtr))  ;*/
            filteredGrayObject(thisGen, *refPtr);
        }
    }, filteredGrayCallback, thisGen);
}

static void
followRoots(CVMGenEdenSpillGeneration* thisGen, TransitiveScanData* tsd)
{
    CVMtraceGcCollect(("GC[MC,%d]: Following roots\n",
		       thisGen->gen.generationNo));
    while (thisGen->todoList != NULL) {
        CVMObject *ref = thisGen->todoList;
        CVMClassBlock *refCB;
        volatile CVMAddr *headerAddr = &CVMobjectVariousWord(ref);
        CVMAddr next = *headerAddr;
        thisGen->todoList = (CVMObject *)(next & ~0x1);

        /* Restore the header various word that we used to store the next
           pointer of the todoList: */
        if ((next & 0x1) != 0) {
            CVMInt32 idx = --thisGen->preservedIdx;
            CVMGenPreservedItem* item = &thisGen->preservedItems[idx];
            CVMassert(idx >= 0);
            CVMassert(item->movedRef == ref);
            CVMassert(!CVMobjectTrivialHeaderWord(item->originalWord));
            *headerAddr = item->originalWord;
            CVMtraceGcScan(("GC[MC,%d]: "
                            "Restoring header %x for obj 0x%x at i=%d\n",
                            thisGen->gen.generationNo, item->originalWord,
                            ref, idx));
        } else {
            *headerAddr = CVM_OBJECT_DEFAULT_VARIOUS_WORD;
        }
	refCB = CVMobjectGetClass(ref);
	/*
	 * Blackening will queue up more objects
	 */
	blackenObject(tsd, ref, refCB);
    }
}

static void
scanTransitively(CVMObject** refPtr, void* data)
{
    TransitiveScanData* tsd = (TransitiveScanData*)data;
    CVMGenEdenSpillGeneration* thisGen = tsd->thisGen;
    CVMObject* ref;

    CVMassert(refPtr != 0);
    ref = *refPtr;
    CVMassert(ref != 0);

    filteredGrayObject(thisGen, ref);

    /*
     * Now scan all its children as well
     */
    followRoots(thisGen, tsd);
    return;
}


/*
 * Test whether a given reference is live or dying. If the reference
 * does not point to the current generation, answer TRUE. The generation
 * that does contain the pointer will give the right answer.
 */
static CVMBool
refIsLive(CVMObject** refPtr, void* data)
{
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)data;
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
    if (!inThisGeneration(thisGen, ref)) {
	return CVM_TRUE;
    }
    /* Did somebody else already scan or forward this object? It's live then */
    return CVMobjectMarked(ref);
}


static CVMBool
collectGeneration(CVMGeneration* gen,
		  CVMExecEnv*    ee, 
		  CVMUint32      numBytes, /* collection goal */
		  CVMGCOptions*  gcOpts)
{
    TransitiveScanData tsd;
    CVMGenEdenSpillGeneration* thisGen = (CVMGenEdenSpillGeneration*)gen;
    CVMGenSpace* space;
    CVMBool success;
    CVMGenSpace* extraSpace;
    CVMUint32* newAllocPtr;

    CVMtraceGcStartStop(("GC[EDENSPILL,%d,full]: Collecting ...",
			 gen->generationNo));

    /* NOTE: We assume that heap integrity is good at the start.  Hence, the
       caller is responsible for calling CVMgenVerifyHeapIntegrity() if a
       check is necessary.  We don't call it here ourselves because this
       allows us to avoid redundant calls to CVMgenVerifyHeapIntegrity()
       which waste time.
    */

    extraSpace = gen->prevGen->getExtraSpace(gen->prevGen);

    /* Prepare the data structure that preserves original second header
       words for use in the TODO stack. */
    thisGen->preservedItems = (CVMGenPreservedItem*)extraSpace->allocBase;
    thisGen->preservedIdx   = 0;
    /* 
     * Assume heap smaller than 4 GB for now.
     */
    thisGen->preservedSize  = (CVMUint32)((CVMGenPreservedItem*)extraSpace->allocTop - 
	(CVMGenPreservedItem*)extraSpace->allocBase);
    thisGen->todoList = NULL;
	
    /*
     * Prepare to do transitive scans 
     */
    tsd.ee = ee;
    tsd.gcOpts = gcOpts;
    tsd.thisGen = thisGen;

    /* The mark phase */
    gcOpts->discoverWeakReferences = CVM_TRUE;

    /*
     * Scan all roots that point to this generation. The root callback is
     * transitive, so 'children' are aggressively processed.
     */
    CVMgenScanAllRoots((CVMGeneration*)thisGen,
		       ee, gcOpts, scanTransitively, &tsd);

    /*
     * Don't discover any more weak references.
     */
    gcOpts->discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    CVMgcProcessSpecialWithLivenessInfo(ee, gcOpts,
					refIsLive, thisGen,
					scanTransitively,
					&tsd);

    /* Reset the data structure that preserves original second header
       words. We are done with the TODO stack, so we can re-use extraSpace. */
    thisGen->preservedItems = (CVMGenPreservedItem*)extraSpace->allocBase;
    thisGen->preservedIdx   = 0;
    /* 
     * Assume heap smaller than 4 GB for now.
     */
    thisGen->preservedSize  = (CVMUint32)((CVMGenPreservedItem*)extraSpace->allocTop - 
	(CVMGenPreservedItem*)extraSpace->allocBase);
	
    /*
     * Sweep will rearrange the segment bounds so make
     * sure we clear cookies to save this value
     */
    {
        CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
        CVMGenSegment* segCurr = segBase ;

        do {
            CHECK_SEGMENT(segCurr) ;
            segCurr->cookie = NULL ;
            segCurr = segCurr->nextSeg ;
        } while (segCurr != segBase) ;
    }

    /* The sweep phase. This phase computes the new addresses of each
       object and writes it in the second header word. Evacuated
       original header words will be kept in the 'preservedItems'
       list. */
    newAllocPtr = sweep(thisGen, ee, gcOpts, gen->allocBase, gen->allocPtr);

    /* At this point, the new addresses of each object are written in
       the headers of the old. Update all pointers to refer to the new
       copies. */

    /* Update the roots */
    CVMgenScanAllRoots((CVMGeneration*)thisGen, ee, gcOpts,
		       filteredUpdateRoot, thisGen);
    CVMgcScanSpecial(ee, gcOpts, filteredUpdateRoot, thisGen);

    {
        CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
        CVMGenSegment* segCurr = segBase ;

        /* And update all interior pointers */
        do {
            scanObjectsAndFixPointers(thisGen, ee, gcOpts,
                                      segCurr->allocBase, segCurr->allocPtr);
            segCurr = segCurr->nextSeg;
        } while (segCurr != segBase);
    
        /* Finally we can move objects, reset marks, and restore original
           header words. */
        compact(thisGen, gen->allocBase, gen->allocPtr);

	/*
	 * Sweep will rearrange the segment bounds so 
	 * update the segment headers with the new bounds
	 */
	do {
	    if (segCurr->cookie != NULL) {
		segCurr->allocPtr = segCurr->cookie;
		segCurr->cookie = NULL;
	    } else {
		segCurr->allocPtr = segCurr->allocBase;
	    }
	    segCurr->allocMark = segCurr->allocPtr;
	    /*
	     * Assume heap smaller than 4 GB for now.
	     */
	    segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4;
	    segCurr = segCurr->nextSeg;
	} while (segCurr != segBase);
    }
    
    thisGen->gen.allocPtr = newAllocPtr;

    /*
     * Now since we are not in the young generation, we have to
     * rebuild the barrier structures.  
     */
    space = thisGen->theSpace;
    space->allocPtr = newAllocPtr;
    CVMassert(space->allocPtr == gen->allocPtr);
    makeCurrent(thisGen, space);

    /* 
     * Make sure the barriers are up to date in the heap
     * in the appropriate [allocBase, allocPtr) ranges
     */
    CVMgenBarrierTableRebuild(gen, ee, gcOpts, CVM_TRUE);
    
    success = (numBytes <= freeMemory(gen, ee)) ;

    /* Ensure that we left the heap in good shape: */
    CVMgenVerifyHeapIntegrity(ee, gcOpts);
    CVMtraceGcStartStop(("... EDENSPILL %d done, success for %d bytes %s\n",
                         gen->generationNo,
                         numBytes,
                         success ? "TRUE" : "FALSE"));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;
}
