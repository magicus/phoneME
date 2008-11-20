/*
 * @(#)gen_markcompact.c	1.56 06/10/10
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

#include "javavm/include/gc/generational-seg/gen_markcompact.h"

#include "javavm/include/porting/system.h"

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

static CVMUint32* 
sweep(CVMGenMarkCompactGeneration* thisGen, 
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

#if 0   /* inSpace() is very expensive for multiple segments. See below. */
static CVMBool
inSpace(CVMGenSpace* space, CVMObject* ref)
{
    CVMUint32* heapPtr = (CVMUint32*)ref;
    CVMGenSegment* segBase = (CVMGenSegment*) space->allocBase ;
    CVMGenSegment* segCurr = segBase ;

    do {
        if (heapPtr >= segCurr->allocBase &&
            heapPtr <  segCurr->allocTop) {
            return CVM_TRUE;
        } 
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;

    return CVM_FALSE;
}
#endif

static CVMBool 
inGeneration(CVMGeneration* gen, CVMObject* ref)
{
    /* Using negation to determine ref is in this gen since the
     * inSpace doesn't scale as the number of segments increases */
    CVMGeneration* youngestGen = gen->prevGen->prevGen;
    CVMGeneration* youngerGen  = gen->prevGen;
    
    /* Assume for now that this generation can only be instantiated as
       generation #2. Otherwise it would have to iterate. */
   CVMassert(gen->generationNo == 2);

   return (!CVMobjectIsInROM(ref) &&
	   !youngestGen->inGeneration(youngestGen, ref) &&
	   !youngerGen->inGeneration(youngerGen, ref));
}

static CVMBool 
inThisGeneration(CVMGenMarkCompactGeneration* thisGen, CVMObject* ref)
{
    return inGeneration(&thisGen->gen, ref);
}

/*
 * Allocate a MarkCompact for the current semispaces generation
 */
static CVMGenSpace*
allocSpace(CVMUint32* heapSpace, CVMUint32 numBytes)
{
    CVMUint32 allocSize = sizeof(CVMGenSpace);
    CVMGenSpace* space = (CVMGenSpace*)calloc(1, allocSize);

    CVMGenSegment* allocBase = NULL;
    CVMGenSegment* allocTop  = NULL;
    CVMGenSegment* segCurr   = NULL;
    CVMUint32 numSegments    = numBytes/CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES;

    if (space == NULL) {
        return NULL;
    }
    CVMassert(heapSpace == NULL);

    CVMassert(numSegments != 0);
#if 0
    /* numSegments can be 0 if we allow for segment chunks and numBytes to
       be less than CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES.  Since this is
       currently not the case, this code is unused for the moment. */
    if (numSegments == 0) {
        /* The only time we have segments that aren't packed to
           CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES is when the numBytes is less
           than CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES to begin with.  Hence,
           we will only have one segment. */
        segCurr = CVMgenAllocSegment(&allocBase, &allocTop, NULL,
                                     numBytes, SEG_TYPE_NORMAL);
        if (segCurr == NULL) {
            allocBase = NULL;
        }
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
               for misc overhead in setting up the segment.
            */
            segCurr = CVMgenAllocSegment(&allocBase, &allocTop, NULL,
                                         CVM_GCIMPL_SEGMENT_USABLE_BYTES,
                                         SEG_TYPE_NORMAL);
            if (segCurr == NULL) {
                allocBase = NULL;
                break;
            }
        }
    }

    if (allocBase == NULL || allocTop == NULL) {
        free(space);
        return NULL;
    }

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
makeCurrent(CVMGenMarkCompactGeneration* thisGen, CVMGenSpace* thisSpace)
{
    CVMGeneration* super = &thisGen->gen;
    super->allocPtr  = thisSpace->allocPtr;
    super->allocBase = thisSpace->allocBase;
    super->allocTop  = thisSpace->allocTop;
    {
        CVMGenSegment* segBase = (CVMGenSegment*) thisSpace->allocBase ;
        CVMGenSegment* segCurr = segBase ;
        do {
            segCurr->allocMark = segCurr->allocPtr ;
            segCurr = segCurr->nextSeg ;
        } while (segCurr != segBase) ;
    }
}

/* Forward declaration */
static CVMBool
collectGeneration(CVMGeneration* gen,
		  CVMExecEnv*    ee, 
		  CVMUint32      numBytes, /* collection goal */
		  CVMGCOptions*  gcOpts);

static CVMBool
inYounger(CVMGeneration* gen, CVMObject* ref)
{
    /* The pointer is either in the previous generation, or is
       younger than that */
    return gen->prevGen->inGeneration(gen->prevGen, ref) ||
	gen->prevGen->inYounger(gen->prevGen, ref);
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
scanObjectsAndFixPointers(CVMGenMarkCompactGeneration* thisGen,
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
    CVMassert(CVM_FALSE); /* this generation can only be oldest */
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

    CVMtraceGcCollect(("GC[MC,%d,full]: Starting GC\n",
		       gen->generationNo));
}

static void
endGC(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: Ending GC\n",
		       gen->generationNo));
#if defined(CVM_DEBUG_ASSERTS) && defined(CVM_CVM_SEGMENTED_HEAP)
    {
	CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
	CVMGenSegment* segCurr = segBase ;
	do {
	    CVMassert(segCurr->nextHotSeg == NULL) ;
	    segCurr = segCurr->nextSeg ;
	} while (segCurr != segBase) ;
    }
#endif
    
}


/*
 * Return remaining amount of memory
 */
static CVMUint32
freeMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segBase = (CVMGenSegment*)thisGen->gen.allocBase ;
    CVMGenSegment* segCurr = segBase ;
    CVMUint32   freeMemorySum = 0 ;

    do {
	CVMUint32 available = (CVMUint32)((CVMUint8*)segCurr->allocTop -
					  (CVMUint8*)segCurr->allocPtr);
        freeMemorySum += available ;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;

    if (thisGen->cacheBase == NULL)
        return freeMemorySum ;

    segCurr = segBase = thisGen->cacheBase ;

    do {
	CVMUint32 available = (CVMUint32)((CVMUint8*)segCurr->allocTop -
					  (CVMUint8*)segCurr->allocPtr);
        freeMemorySum += available ;
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segBase) ;

    return freeMemorySum ;
}

/*
 * Return total amount of memory
 */
static CVMUint32
totalMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segBase = (CVMGenSegment*)thisGen->gen.allocBase ;
    CVMGenSegment* segCurr = segBase ;
    CVMUint32   totalMemorySum = 0 ;

    do {
	CVMUint32 size = (CVMUint32)((CVMUint8*)segCurr->allocTop -
				     (CVMUint8*)segCurr->allocBase);
        totalMemorySum += size ;
        segCurr = segCurr->nextSeg ;
    }while (segCurr != segBase) ;

    if (thisGen->cacheBase == NULL)
        return totalMemorySum ; 

    segCurr = segBase = thisGen->cacheBase ;
    do {
	CVMUint32 size = (CVMUint32)((CVMUint8*)segCurr->allocTop -
				     (CVMUint8*)segCurr->allocBase);
        totalMemorySum += size ;
        segCurr = segCurr->nextSeg ;
    }while (segCurr != segBase) ;

    return totalMemorySum ;
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
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segBase = (CVMGenSegment*) thisGen->hotBase;
    CVMGenSegment* segTop  = (CVMGenSegment*) thisGen->hotTop;
    CVMGenSegment* segCurr   = segBase ;
    CVMBool scanned          = CVM_FALSE ;

    if (thisGen->hotBase == NULL) {
       /* Nothing to do */
       return ;
    }

    /* Store the allocMark for all the segments before
     * scanning. The allocMarks themselves would be used 
     * for rebuilding the barrier table
     */
    do {
        segCurr->cookie = segCurr->allocMark ;
	/* Remember what this scan looked like, if work is to be done */
	segCurr->logMarkStart = segCurr->allocMark;
        segCurr = segCurr->nextHotSeg ;
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

        if (scanned) {
           /* Promotion could have resulted in the addition of segments */
            segBase = thisGen->hotBase;
            segTop  = thisGen->hotTop;
            segCurr = segBase ;
        } else if (segCurr == segTop) {
            segCurr = NULL ;
        } else {
            segCurr = segCurr->nextHotSeg ;
        }

    } while (segCurr != NULL) ;


    /*
     * Remember these pointers for further young space collections.
     * Don't forget to clear class marks again, since we need to be
     * able to traverse already scanned classes again, in order to
     * build barrier tables.
     */
    
    segCurr = segBase ;
#ifdef CVM_VERIFY_HEAP
    do {
        CVMassert(segCurr->cookie == segCurr->allocPtr) ;
	CVMassert(segCurr->logMarkStart != NULL);
        segCurr = segCurr->nextHotSeg ;
    } while (segCurr != segBase) ;
#endif
    
    /* Do an incremental rebuild */
    CVMgenBarrierTableRebuild(gen, ee, gcOpts, CVM_FALSE);
    
#ifdef CVM_VERIFY_HEAP
    /* Sanity check the card table for each of the hot segments */
    segBase = (CVMGenSegment*) thisGen->hotBase;
    segCurr   = segBase ;
 
    do { 
	CVMassert(segCurr->logMarkStart != NULL);
	CVMassert(segCurr->logMarkStart < segCurr->allocPtr);
	CVMgenVerifyObjectRangeForCardSanity(gen, ee, gcOpts,
					     segCurr->logMarkStart,
					     segCurr->allocPtr);
        segCurr = segCurr->nextHotSeg ;
    } while (segCurr != segBase) ;
#endif

    segCurr = segBase ;
    /* Get rid of the hot segments list */
    do {
        CVMGenSegment* temp = segCurr ;
        CHECK_SEGMENT(segCurr) ;
        segCurr = segCurr->nextHotSeg ;
        temp->nextHotSeg = NULL ;
    } while (segCurr != segBase) ;

    /*
     * This is the observable fromSpace pointer. We can probably
     * get away with not committing this pointer.
     */
    ((CVMGenMarkCompactGeneration*)gen)->theSpace->allocPtr = gen->allocPtr;
    
    /* Everything that was touched has been scanned, empty the hot segments */
    ((CVMGenMarkCompactGeneration*)gen)->hotBase = NULL ;
    ((CVMGenMarkCompactGeneration*)gen)->hotTop  = NULL ;
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
	segCurr->logMarkStart = segCurr->allocMark;
        /* 
         * Assume heap smaller than 4 GB for now.
         */
        segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4;
        segCurr->rollbackMark = NULL;
        segCurr->nextHotSeg = NULL ;
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);

    ((CVMGenMarkCompactGeneration*)gen)->hotBase = NULL ;
    ((CVMGenMarkCompactGeneration*)gen)->hotTop  = NULL ;

    /* Scan all pointers in old-gen objects for pointers into
       young generation as well as the promoted space which will be
       rolled back shortly 
    */
    CVMgenScanBarrierObjects(gen, ee, gcOpts, callback, callbackData);

#ifdef CVM_VERIFY_HEAP
    verifySegmentsWalk(segBase) ;
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


/* Forward declaration */
static 
CVMObject* allocObject(CVMGeneration* gen, CVMUint32 numBytes) ;

/* Promote 'objectToPromote' into the current generation. Returns
   the new address of the object, or NULL if the promotion failed. */
static CVMObject*
promoteInto(CVMGeneration* gen, CVMObject* objectToPromote,
	    CVMUint32 objectSize)
{
    CVMObject* dest;

    dest = allocObject(gen, objectSize) ;
    if (dest != NULL) {
	copyDisjointWords((CVMUint32*)dest, (CVMUint32*)objectToPromote,
			  objectSize);
    }
    CVMtraceGcScan(("GC[MC,%d]: "
		    "Promoted object %x (len %d, class %C), to %x\n",
		    gen->generationNo,
		    objectToPromote, objectSize,
		    CVMobjectGetClass(objectToPromote),
		    dest));
    return dest;
}

#ifdef CVM_VERIFY_HEAP

static void
verifyGeneration(CVMGeneration* gen, CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMUint32* base = gen->allocBase;

    CVMGenSegment* segBase = (CVMGenSegment*) base ;
    CVMGenSegment* segCurr = (CVMGenSegment*) base ;

    verifySegmentsWalk(segBase) ;

    do { 
        CVMUint32* curr = segCurr->allocBase;
        CVMUint32* top = segCurr->allocPtr ;
        CHECK_SEGMENT(segCurr) ;

	CVMgenVerifyObjectRangeForCardSanity(gen, ee, gcOpts,
					     curr, top);
	
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);

}
#endif

#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
static CVMBool iterateSegList(CVMExecEnv* ee,
			      CVMGenSegment* segBase,
			      CVMObjectCallbackFunc callback,
			      void* callbackData)
{
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

static CVMBool iterateGen(CVMGeneration* gen, CVMExecEnv* ee,
			  CVMObjectCallbackFunc callback,
			  void* callbackData)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMBool completeScanDone;

    completeScanDone = iterateSegList(ee, segBase, callback, callbackData);
    
    segBase = ((CVMGenMarkCompactGeneration*)gen)->cacheBase;
    if (!completeScanDone || segBase == NULL) {
	return completeScanDone;
    }
    
    return iterateSegList(ee, segBase, callback, callbackData);
}
#endif

/*
 * Allocate a MarkCompact generation.
 */
CVMGeneration*
CVMgenMarkCompactAlloc(CVMUint32* space, CVMUint32 numBytes, 
		       CVMUint32 maxSize)
{
    CVMGenMarkCompactGeneration* thisGen  = (CVMGenMarkCompactGeneration*)
	calloc(sizeof(CVMGenMarkCompactGeneration), 1);
    CVMGenSpace* theSpace;

    if (thisGen == NULL) {
	return NULL;
    }

    /*
     * Start the heap with an initial amount.
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
    /* In the case of segmented heap, generation
       bounds doesnt make sense since the segments
       are possibly located throughout the address space
    */
    thisGen->gen.genBase = NULL;
    thisGen->gen.genTop  = NULL;

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

    thisGen->gen.allocObject = allocObject;
    thisGen->initHeapSize = numBytes ;
    thisGen->maxSize = maxSize ;

    CVMdebugPrintf(("GC[MC]: Initialized mark-compact segmented gen "
		  "for generational GC\n"));
    CVMdebugPrintf(("\tSize of the space in bytes=%d\n"
		    "\tMax size of the space in bytes=%d\n"
		    "\tFirst segment = 0x%x\n",
		  numBytes, maxSize, theSpace->allocBase));
    return (CVMGeneration*)thisGen;
}

/* Forward declaration */
static CVMBool   
freeCacheSegments(CVMGeneration* gen, CVMUint32 numRelease) ;

/*
 * Free all the memory associated with the current mark-compact generation
 */
void
CVMgenMarkCompactFree(CVMGenMarkCompactGeneration* thisGen)
{
    CVMGenSegment* segCurr = (CVMGenSegment*)thisGen->gen.allocBase;
    segCurr->prevSeg->nextSeg = NULL; 

    while (segCurr != NULL) {
	CVMGenSegment* temp = segCurr->nextSeg;
        CVMgenFreeSegment(segCurr, NULL, NULL);
        segCurr = temp;
    }

    freeCacheSegments((CVMGeneration*)thisGen, ~0);
    free(thisGen->theSpace);
    thisGen->theSpace  = NULL;
    free(thisGen);
 }

static CVMObject*
getForwardingPtr(CVMGenMarkCompactGeneration* thisGen,
				  CVMObject* ref)
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
preserveHeaderWord(CVMGenMarkCompactGeneration* thisGen,
                   CVMObject* movedRef, CVMAddr originalWord);

static void
addToTodo(CVMGenMarkCompactGeneration* thisGen, CVMObject* ref)
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
restorePreservedHeaderWords(CVMGenMarkCompactGeneration* thisGen)
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
preserveHeaderWord(CVMGenMarkCompactGeneration* thisGen,
		   CVMObject* movedRef, CVMAddr originalWord)
{
    CVMGenPreservedItem* item = &thisGen->preservedItems[thisGen->preservedIdx];

    if (thisGen->preservedIdx >= thisGen->preservedSize) {
        CVMpanic("**** OVERFLOW OF PRESERVED HEADER WORDS" 
                 " in mark-compact GC!!! ****");
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

static CVMUint32*
sweep(CVMGenMarkCompactGeneration* thisGen,  
      CVMExecEnv* ee, 
      CVMGCOptions* gcOpts,
      CVMUint32* base, 
      CVMUint32* top)
{
    CVMGenSegment* segBase = (CVMGenSegment*) base ;
    CVMGenSegment* segCurr = segBase ;

    CVMGenSegment* forwardingSeg= segBase ;
    CVMUint32* forwardingAddress = forwardingSeg->allocBase ;

    do {
        CVMUint32* curr = segCurr->allocBase;
        CVMUint32* top = segCurr->allocPtr ;
        CHECK_SEGMENT(segCurr) ;

        /* Skip the large object segments while pretending
         * to move them
         */
        if (segCurr->flag == SEG_TYPE_LOS) {
	    CVMObject* currObj    = (CVMObject*)curr;
	    /* 
	     * member clas and various32 hold a native pointer
	     * therefore use type CVMAddr (instead of CVMUint32)
	     * which is 64 bit aware
	     */
	    CVMAddr  classWord  = CVMobjectGetClassWord(currObj);

	    if (CVMobjectMarkedOnClassWord(classWord)) {
		/* 
		 * member clas and various32 hold a native pointer
		 * therefore use type CVMAddr (instead of CVMUint32)
		 * which is 64 bit aware
		 */
	        volatile CVMAddr* headerAddr   = &CVMobjectVariousWord(currObj);
	        CVMAddr  originalWord = *headerAddr;
                if (!CVMobjectTrivialHeaderWord(originalWord)) {
		    preserveHeaderWord(thisGen, currObj, originalWord) ;
                }
		/* 
		 * member clas and various32 hold a native pointer
		 * therefore use type CVMAddr (instead of CVMUint32)
		 * which is 64 bit aware
		 */
	        *headerAddr = (CVMAddr)curr ;
            } else {
#ifdef CVM_JVMPI
                extern CVMUint32 liveObjectCount;
                if (CVMjvmpiEventObjectFreeIsEnabled()) {
                    CVMjvmpiPostObjectFreeEvent(currObj);
                }
                liveObjectCount--;
#endif
#ifdef CVM_JVMPI
                if (CVMjvmtiShouldPostObjectFree()) {
                    CVMjvmtiPostObjectFreeEvent(currObj);
                }
#endif
            }
            continue ;
        }

        CVMtraceGcCollect(("GC[MC,%d]: Sweeping object range [%x,%x)\n",
		           thisGen->gen.generationNo, base, top));
        while (curr < top) {
	    CVMObject* currObj    = (CVMObject*)curr;
	    /* 
	     * member clas and various32 hold a native pointer
	     * therefore use type CVMAddr (instead of CVMUint32)
	     * which is 64 bit aware
	     */
	    CVMAddr  classWord  = CVMobjectGetClassWord(currObj);
	    CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	    CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	    
	    CVMassert(objSize <= SEG_SMALL_OBJ_CEIL) ;
	    
	    if (CVMobjectMarkedOnClassWord(classWord)) {
		volatile CVMAddr* headerAddr = 
		    &CVMobjectVariousWord(currObj);
		CVMAddr  originalWord = *headerAddr;
		/* see if the object would fit within this segment and
		 * Update forwarding address if needed . 
		 * Note If the object doesnt fit within this segment we
		 * are leaving some wasted space in the segment. We need
		 * to figure out a way of minimizing this
		 */
		if ((forwardingAddress + objSize / 4) > 
		    forwardingSeg->allocTop) {
		    forwardingSeg->cookie = forwardingAddress ;
		    CHECK_SEGMENT(forwardingSeg) ;
		    /* Dont slide through LOS */
		    do {
			forwardingSeg = forwardingSeg->nextSeg ;
		    } while (forwardingSeg->flag == SEG_TYPE_LOS) ;
		    
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
                        (CVMObject *)forwardingAddress, originalWord);
		}
		/* Set forwardingAddress in header */
		*headerAddr = (CVMAddr)forwardingAddress;
		/* Pretend to have copied this live object! */
		forwardingAddress += objSize / 4;
	    } else {
#ifdef CVM_JVMPI
		extern CVMUint32 liveObjectCount;
		if (CVMjvmpiEventObjectFreeIsEnabled()) {
		    CVMjvmpiPostObjectFreeEvent(currObj);
		}
		liveObjectCount--;
#endif
#ifdef CVM_JVMTI
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

    } while((segCurr = segCurr->nextSeg) != segBase) ; /*do-while loop*/

    forwardingSeg->cookie = forwardingAddress ;
    CHECK_SEGMENT(forwardingSeg) ;
    return (CVMUint32*)forwardingSeg;
}

/* Finally we can move objects, reset marks, and restore original
   header words. */
static void 
compact(CVMGenMarkCompactGeneration* thisGen, CVMUint32* base, CVMUint32* top) 
{
    CVMGenSegment* segBase = (CVMGenSegment*) base;
    CVMGenSegment* segCurr = segBase;

    CVMtraceGcCollect(("GC[MC,%d]: Compacting object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));

    do {
        CVMUint32* curr = segCurr->allocBase;
        CVMUint32* top = segCurr->allocPtr ;
        CHECK_SEGMENT(segCurr) ;

        /* Release unused large object segments */
        if (segCurr->flag == SEG_TYPE_LOS) {
	    /* 
	     * member clas and various32 hold a native pointer
	     * therefore use type CVMAddr (instead of CVMUint32)
	     * which is 64 bit aware
	     */
	    CVMAddr      classWord = CVMobjectGetClassWord((CVMObject*)curr);

	    if (!CVMobjectMarkedOnClassWord(classWord)) {
                segCurr->flag = SEG_TYPE_LOS_FREE ;
            } else {
                /* Clear the mark on the Object */
                CVMobjectClearMarkedOnClassWord(classWord) ;
	        CVMobjectSetClassWord((CVMObject*)curr, classWord);
	        CVMobjectVariousWord((CVMObject*)curr) =
		    CVM_OBJECT_DEFAULT_VARIOUS_WORD;
            }
            continue ;
        }

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

	    CVMassert(objSize <= SEG_SMALL_OBJ_CEIL) ;
	    if (CVMobjectMarkedOnClassWord(classWord)) {
	        CVMUint32* destAddr = (CVMUint32*)
		    getForwardingPtr(thisGen, currObj);
	        CVMobjectClearMarkedOnClassWord(classWord);
#ifdef CVM_DEBUG
	        /* For debugging purposes, make sure the deleted mark is
	           reflected in the original copy of the object as well. */
	        CVMobjectSetClassWord(currObj, classWord);
#endif
	        copyDown(destAddr, curr, objSize);

#ifdef CVM_JVMPI
                if (CVMjvmpiEventObjectMoveIsEnabled()) {
                    CVMjvmpiPostObjectMoveEvent(1, curr, 1, destAddr);
                }
#endif

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
	    /* iterate */
	    curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */

    }while((segCurr = segCurr->nextSeg) != segBase) ; /*do-while loop*/
    /* Restore the "non-trivial" old header words
       with the new address of object */
    restorePreservedHeaderWords(thisGen);
}

/*
 * Gray an object known to be in the old MarkCompact generation.
 */
static void
grayObject(CVMGenMarkCompactGeneration* thisGen, CVMObject* ref)
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
filteredGrayObject(CVMGenMarkCompactGeneration* thisGen, CVMObject* ref)
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
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
    CVMObject* ref = *refPtr;
    filteredGrayObject(thisGen, ref);
    return;
}

static void
filteredUpdateRoot(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
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
    CVMGenMarkCompactGeneration* thisGen;
} TransitiveScanData;

static void
blackenObject(TransitiveScanData* tsd,
	      CVMObject* ref, CVMClassBlock* refCb)
{
    CVMGenMarkCompactGeneration* thisGen = tsd->thisGen;
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
           CVMassert(inGeneration(&thisGen->gen, *refPtr) ||
              thisGen->gen.prevGen->inGeneration(thisGen->gen.prevGen, *refPtr) ||
              thisGen->gen.prevGen->prevGen->inGeneration(thisGen->gen.prevGen->prevGen, *refPtr) ||
              CVMobjectIsInROM(*refPtr))  ;
	    filteredGrayObject(thisGen, *refPtr);
	}
    }, filteredGrayCallback, thisGen);
}

static void
followRoots(CVMGenMarkCompactGeneration* thisGen, TransitiveScanData* tsd)
{
    CVMtraceGcCollect(("GC[MC,%d]: Following roots\n",
		       thisGen->gen.generationNo));
    while (thisGen->todoList != NULL) {
        CVMObject* ref = thisGen->todoList;
        CVMClassBlock* refCB;
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
    CVMGenMarkCompactGeneration* thisGen = tsd->thisGen;
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
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
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
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSpace* space;
    CVMBool success;
    CVMGenSpace* extraSpace;
    CVMUint32* newAllocPtr;

    CVMtraceGcStartStop(("GC[MC,%d,full]: Collecting ...",
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
    /* And update all interior pointers */
    {
        CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
        CVMGenSegment* segCurr = segBase ;

        do {
            scanObjectsAndFixPointers(thisGen, ee, gcOpts,
                                      segCurr->allocBase, segCurr->allocPtr);
            segCurr = segCurr->nextSeg ;
        } while (segCurr != segBase) ;
    }
    
    /* Finally we can move objects, reset marks, and restore original
       header words. */
    compact(thisGen, gen->allocBase, gen->allocPtr);

    /*
     * Sweep will rearrange the segment bounds so 
     * update the segment headers with the new bounds
     */
    {
        CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase ;
        CVMGenSegment* segTop  = (CVMGenSegment*)gen->allocTop  ;
        CVMGenSegment* segCurr ;
        CVMGenSegment* segPrev ;
        CVMUint32 freeMemorySum = 0 ;
        CVMUint32 totalMemorySum = 0 ;

	for (segCurr = segTop ; segCurr != segBase; segCurr = segPrev) {
            CHECK_SEGMENT(segCurr) ;
            segPrev = segCurr->prevSeg ;
            totalMemorySum += segCurr->size ;
            if (segCurr->flag == SEG_TYPE_LOS_FREE) {
                segCurr->allocMark = segCurr->allocPtr = segCurr->allocBase ;
		segCurr->logMarkStart = segCurr->allocMark;
                /* 
                 * Assume heap smaller than 4 GB for now.
                 */
                segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4 ;
                segCurr->flag = SEG_TYPE_LOS ;
                CVMgenRemoveSegment(segCurr, &segBase, &segTop) ;
                /* NOTE: We could potentially improve this.
		   Instead of inserting this in the cache, we can go
                   ahead and free it.  This might be the more prudent thing to
                   do because freeCacheSegment() currently uses a very simple
                   algorithm where the segment with the highest address gets
                   free'd first rather then freeing LOS segments.  This could
                   lead to unnecessary retention of LOS segments. */
                CVMgenInsertSegmentAddressOrder(segCurr, &thisGen->cacheBase, &thisGen->cacheTop) ;
                freeMemorySum += segCurr->size ;
                continue ;
            } else if (segCurr->flag == SEG_TYPE_LOS) {
                continue ;
            }

            if (segCurr->cookie != NULL) {
                segCurr->allocMark = segCurr->allocPtr = segCurr->cookie ;
                /* 
                 * Assume heap smaller than 4 GB for now.
                 */
                segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4 ;
                segCurr->cookie = NULL ;
            } else {
		segCurr->allocMark = segCurr->allocPtr = segCurr->allocBase ;
                /* 
		 * Assume heap smaller than 4 GB for now.
		 */
                segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4 ;
		segCurr->logMarkStart = segCurr->allocMark;
		/* We don't need this segment. Put it in free list */
                CVMgenRemoveSegment(segCurr, &segBase, &segTop) ;
                CVMgenInsertSegmentAddressOrder(segCurr, 
						&thisGen->cacheBase, 
						&thisGen->cacheTop) ;
                freeMemorySum += segCurr->size ;
            }
        }

        if (segCurr->cookie != NULL) {
            segCurr->allocMark = segCurr->allocPtr = segCurr->cookie ;
	    segCurr->logMarkStart = segCurr->allocMark;
            /* 
             * Assume heap smaller than 4 GB for now.
             */
            segCurr->available = (CVMUint32)(segCurr->allocTop - segCurr->allocPtr) * 4 ;
            segCurr->cookie = NULL ;
        }
   
        CVMassert(segBase != NULL) ;
        CVMassert(segTop != NULL) ;

        /* Now is a good time to contract the heap based on a given policy
         * Current policy :
         * 1. Free unused segments till a given threshold of free segments is reached.
         *    The segments are chosen in address-order from high to low.
         * 2. Number of segments doesnt go below the initial heap size.
         */

        /* NOTE: We could have a mode where we will always free all unused
           segments for the purpose of minimizing memory usage. */
        if (totalMemorySum > thisGen->initHeapSize && freeMemorySum > (CVMUint32) SEG_FREE_LIMIT) {
            freeCacheSegments(gen, (freeMemorySum - (CVMUint32)SEG_FREE_LIMIT)) ;
        }

        thisGen->theSpace->allocBase = (CVMUint32*)segBase ;
        thisGen->theSpace->allocTop  = (CVMUint32*)segTop ;
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

    /* Do a full reconstruction of the barrier table */
    CVMgenBarrierTableRebuild(&thisGen->gen, ee, gcOpts, CVM_TRUE);
    
    success = (numBytes <= freeMemory(gen, NULL)) ;

    /* Ensure that we left the heap in good shape: */
    CVMgenVerifyHeapIntegrity(ee, gcOpts);
    CVMtraceGcStartStop(("... MARK-COMPACT %d done, success for %d bytes %s\n",
			 gen->generationNo,
			 numBytes,
			 success ? "TRUE" : "FALSE"));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;
}

/* Iterate over promoted pointers in a generation. A young space
   collection accummulates promoted objects in an older
   generation. These objects need to be scanned for pointers into
   the young generation.
   
   At the start of a GC, the current allocation pointer of each
   generation is marked. Therefore the promoted objects to scan are those
   starting from 'allocMark' to the current 'allocPtr'.
  */
static CVMGenSegment*
getCacheSegment(CVMGeneration* gen, 
		CVMUint32 flag, 
		CVMUint32 numBytes) 
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segSentry = thisGen->cacheBase ;
    CVMGenSegment* segCurr = segSentry ;

    if (thisGen->cacheBase == NULL) {
        return NULL ;
    }

    do {
        if ((segCurr->flag == flag) && 
            (segCurr->flag != SEG_TYPE_LOS || segCurr->size == numBytes)) {
            CVMgenRemoveSegment(segCurr, &thisGen->cacheBase, &thisGen->cacheTop) ;
            CVMgenInsertSegmentAddressOrder(segCurr, (CVMGenSegment**)&gen->allocBase, 
                                            (CVMGenSegment**)&gen->allocTop) ;
            memset(segCurr->cardTable, CARD_CLEAN_BYTE, segCurr->cardTableSize) ;
            memset(segCurr->objHeaderTable, 0, segCurr->cardTableSize);

            return segCurr ;
        }
        segCurr = segCurr->nextSeg ;
    } while (segCurr != segSentry) ;

    return NULL ; 
}


static CVMBool
freeCacheSegments(CVMGeneration* gen, 
                                  CVMUint32 numRelease) {
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segCurr = thisGen->cacheTop ;
    CVMGenSegment* segPrev ;
    CVMUint32 numFreed = 0 ;

    if (thisGen->cacheBase == NULL) {
        return CVM_FALSE ;
    }
    
    do {
        segPrev = segCurr->prevSeg ;
        numFreed += segCurr->size ;	
        CVMgenFreeSegment(segCurr, &thisGen->cacheBase, &thisGen->cacheTop) ;
        segCurr = segPrev ;
    } while (thisGen->cacheBase != NULL && numFreed < numRelease) ;   
 
    return (numFreed >= numRelease) ;
}

/*
 * Allocate the required amount of space in an available segment
 * Use a first-fit strategy while remembering the last place where
 * allocation succeeded
 *
 */

#define TOLERANCE_FACTOR 16

static CVMObject*
allocObject(CVMGeneration* gen, CVMUint32 numBytes) 
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMGenSegment* segCurr = NULL;
    CVMObject* object = NULL;
    CVMGenSegment* segSentry = NULL;
    CVMUint32 segSize;
    CVMUint8 segType;

    if (numBytes > CVM_GCIMPL_SEGMENT_USABLE_BYTES) {
        segSize = CVMalignDoubleWordUp(numBytes);
        segType = SEG_TYPE_LOS;
    } else {
        segSize = CVM_GCIMPL_SEGMENT_USABLE_BYTES;
        segType = SEG_TYPE_NORMAL;
    }

    segCurr = (CVMGenSegment*)thisGen->gen.allocPtr ;

ALLOC :

    /* TODO: Note that this code seems to assume that segCurr is always not
       NULL when we get here.  This won't be true if we end up with a
       quiescent heap where the oldGen objects all die and get swept up
       thereby causing all oldGen segments to be moved to the cache list
       i.e. no current segment on the allocated list.  While this is unlikely,
       it is still possible and need to be fixed. */
    CVMassert(segCurr != NULL);
    segSentry = segCurr;
    CHECK_SEGMENT(segCurr) ;

    /* NOTE: We might be able to improve this in the future.
       Note that the current allocation implementation only checks if
       the current segment can satisfy the allocation.  If not, then it will
       simply choose to use a brand new segment from the free list (fetched
       by getCacheSegment()).  This increases the amount of internal
       fragmentation because the available space may be available in other
       already partially filled segments that will now go unused.  This can
       be remedied with a more intelligent size matching algorithm.
    */
    if (segCurr->available < numBytes || segCurr->flag != segType ) {
        CVMGenSegment* segment = getCacheSegment(gen, segType, numBytes) ;
        if (segment != NULL) {
            segCurr = segment ;
            segCurr->rollbackMark = segCurr->allocPtr;
            /* TODO: Since we've set segCurr to another segment here,
               shouldn't we also set segEntry so that the loop below
               will function properly?  Need to evaluate this. */
        }
    }

    do {
       if ((segCurr->available >= numBytes) && (segCurr->flag == segType)) {
           if (segCurr->flag != SEG_TYPE_LOS || 
	       ((segCurr->available <= numBytes + TOLERANCE_FACTOR) &&
		(segCurr->available >= numBytes))) {

               /* Allocate the object from this segment: */
               object = (CVMObject*) segCurr->allocPtr;
               segCurr->allocPtr = segCurr->allocPtr + numBytes / 4 ;
               segCurr->available -= numBytes ;
               CVMassert(segCurr->available >= 0) ;
               break ;
           }
       }    
      
       segCurr = segCurr->nextSeg ;
       CHECK_SEGMENT(segCurr) ;

       CVMassert(segCurr >= (CVMGenSegment*)thisGen->gen.allocBase) ;
       CVMassert(segCurr <= (CVMGenSegment*)thisGen->gen.allocTop) ;
    }while (segCurr != segSentry) ;

    if (object == NULL) {
        CVMUint32 totalMemorySum = totalMemory(gen, NULL) ;

        /* NOTE: We may want to adjust the size calculation to account for
           the segment overhead as well.  Similarly, freeMemory() and
           totalMemory() may need to be adjusted accordingly. */
        if ((totalMemorySum + segSize) <= thisGen->maxSize) {
            goto EXPAND ;
        } else {
            goto MAKE_SPACE ;
        } 
    }

    /* Make sure the segment has been added to the hot list
     * if not then add it
     */
    if (segCurr->nextHotSeg == NULL) {
        segCurr->cookie = segCurr->allocMark;
	/* We are about to put a segment into the hot
	   list. Make sure to reset its logMarkStart.
	   It might be stale. */
	segCurr->logMarkStart = segCurr->allocMark;
        if (thisGen->hotTop != NULL) {
            thisGen->hotTop->nextHotSeg = segCurr ;
            thisGen->hotTop = segCurr ;
            segCurr->nextHotSeg = thisGen->hotBase ;
        } else {
            thisGen->hotBase = segCurr ;
            segCurr->nextHotSeg = segCurr ;
            thisGen->hotTop = segCurr ;
        }
     }

   /* If we were forced to leave a hole to satisfy a request
    * bump up the allocPtr to the next segment
    */  
    if (segCurr != (CVMGenSegment*)thisGen->gen.allocPtr &&
        segCurr->flag != SEG_TYPE_LOS) {
       thisGen->gen.allocPtr = (CVMUint32*)segCurr ;
    } 

    return object ;

EXPAND :
     /* Try to expand the heap and accomodate the request provided
        we havent hit the heap ceiling
     */  
    segCurr = CVMgenAllocSegment((CVMGenSegment**)&gen->allocBase,
                 (CVMGenSegment**)&gen->allocTop, NULL, segSize, segType);
    if (segCurr != NULL) {
        thisGen->theSpace->allocBase = thisGen->gen.allocBase;
        thisGen->theSpace->allocTop  = thisGen->gen.allocTop;
        segCurr->rollbackMark = segCurr->allocPtr;
        goto ALLOC;
    }


MAKE_SPACE:
    if (freeCacheSegments(gen, numBytes)) {
        goto EXPAND;
    }
        
    return NULL;
}
