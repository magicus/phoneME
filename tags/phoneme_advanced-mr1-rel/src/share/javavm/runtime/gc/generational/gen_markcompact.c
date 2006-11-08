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
 * This file includes the implementation of a mark-compact generation.
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

#include "javavm/include/gc/generational/gen_markcompact.h"

#include "javavm/include/porting/system.h"


#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/*
 * Range checks
 */
#define CVMgenMarkCompactInGeneration(gen, ref) \
    CVMgenInGeneration(((CVMGeneration *)(gen)), (ref))

/* Forward declaration */
static CVMBool
CVMgenMarkCompactCollect(CVMGeneration* gen,
			 CVMExecEnv*    ee, 
			 CVMUint32      numBytes, /* collection goal */
			 CVMGCOptions*  gcOpts);

static void
CVMgenMarkCompactFilteredUpdateRoot(CVMObject** refPtr, void* data);

/*
 * Given the address of a slot, mark the card table entry
 */
static void
updateCTForSlot(CVMObject** refPtr, CVMObject* ref,
		CVMGenMarkCompactGeneration* thisGen)
{
    /* 
     * If ref is a cross generational pointer, mark refPtr
     * in the card table
     */
    if ((ref != NULL) &&
        !CVMgenMarkCompactInGeneration(thisGen, ref) &&
	!CVMobjectIsInROM(ref)) {
	*CARD_TABLE_SLOT_ADDRESS_FOR(refPtr) = CARD_DIRTY_BYTE;
    }
}

/*
 * Scan objects in contiguous range, and update corresponding card
 * table entries. 
 */
static void
updateCTForRange(CVMGenMarkCompactGeneration* thisGen,
		 CVMExecEnv* ee, CVMGCOptions* gcOpts,
		 CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,full]: "
		       "Scanning object range (partial) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMObject* currCbInstance = *((CVMObject**)CVMcbJavaInstance(currCb));
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    updateCTForSlot(refPtr, *refPtr, thisGen);
	});
	/*
	 * Mark card for this slot if necessary for class of object
	 */
	updateCTForSlot((CVMObject**)currObj, currCbInstance, thisGen);
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
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
}

/*
 * Scan objects in contiguous range, and do all special handling as well.
 * Ignore unmarked objects.
 */
static void
scanObjectsInRangeSkipUnmarked(CVMGenMarkCompactGeneration* thisGen,
			       CVMExecEnv* ee, CVMGCOptions* gcOpts,
			       CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,full]: "
		       "Scanning object range (skip unmarked) [%x,%x)\n",
		       base, top));
    while (curr < top) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMAddr    classWord  = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	if (CVMobjectMarkedOnClassWord(classWord)) {
	    CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, currObj,
						 classWord, {
		if (*refPtr != 0) {
		    CVMgenMarkCompactFilteredUpdateRoot(refPtr, thisGen);
		}
	    }, CVMgenMarkCompactFilteredUpdateRoot, thisGen);
	}
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

/*
 * Re-build the old-to-young pointer tables after an old generation is
 * done. 
 */
static void
CVMgenMarkCompactRebuildBarrierTable(CVMGenMarkCompactGeneration* thisGen,
				   CVMExecEnv* ee,
				   CVMGCOptions* gcOpts,
				   CVMUint32* startRange,
				   CVMUint32* endRange)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: "
		       "Rebuilding barrier table for range [%x,%x)\n",
		       thisGen->gen.generationNo,
		       startRange, endRange));
    /*
     * No need to look at classes here. They are scanned separately in
     * CVMgenScanAllRoots(), so no need to make a record of them in the
     * barrier records.
     */
    updateCTForRange(thisGen, ee, gcOpts, startRange, endRange);
    CVMgenBarrierObjectHeadersUpdate((CVMGeneration*)thisGen,
				     ee, gcOpts, startRange, endRange);
}

static void       
CVMgenMarkCompactScanYoungerToOlderPointers(CVMGeneration* gen,
					  CVMExecEnv* ee,
					  CVMGCOptions* gcOpts,
					  CVMRefCallbackFunc callback,
					  void* callbackData)
{
    CVMassert(CVM_FALSE); /* this generation can only be old */
}

static void       
CVMgenMarkCompactScanOlderToYoungerPointers(CVMGeneration* gen,
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
}


/*
 * Start and end GC for this generation. 
 */
static void
CVMgenMarkCompactStartGC(CVMGeneration* gen,
		       CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: Starting GC\n",
		       gen->generationNo));
}

static void
CVMgenMarkCompactEndGC(CVMGeneration* gen,
		     CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    CVMtraceGcCollect(("GC[MC,%d,full]: Ending GC\n",
		       gen->generationNo));
}

/*
 * Return remaining amount of memory
 */
static CVMUint32
CVMgenMarkcompactFreeMemory(CVMGeneration* gen, CVMExecEnv* ee)
{
    return (CVMUint32)((CVMUint8*)gen->allocTop - (CVMUint8*)gen->allocPtr);
}

/*
 * Return total amount of memory
 */
static CVMUint32
CVMgenMarkcompactTotalMemory(CVMGeneration* gen, CVMExecEnv* ee)
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
CVMgenMarkCompactScanPromotedPointers(CVMGeneration* gen,
				      CVMExecEnv* ee,
				      CVMGCOptions* gcOpts,
				      CVMRefCallbackFunc callback,
				      void* callbackData)
{
    CVMUint32* startRange = gen->allocMark;
    CVMUint32* endRange   = gen->allocPtr;

    CVMtraceGcCollect(("GC[MC,full]: "
		       "Scanning promoted ptrs [%x,%x)\n",
		       startRange, endRange));
    /* loop, until promoted object scans result in no more promotions */
    do {
	scanObjectsInRange(ee, gcOpts, startRange, endRange,
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
    CVMgenMarkCompactRebuildBarrierTable((CVMGenMarkCompactGeneration*)gen,
					 ee, gcOpts,
					 gen->allocMark, gen->allocPtr);
    gen->allocMark = gen->allocPtr; /* Advance mark for the next batch
				       of promotions */
    /*
     * Make sure that gen->allocPtr and gen->fromSpace->allocPtr are
     * in agreement (i.e. in the same space).
     */
    CVMassert(CVMgenInGeneration(gen, (CVMObject*)gen->allocPtr) ||
	      (gen->allocPtr == gen->allocTop));
}

/*
 * Copy numBytes from 'source' to 'dest'. Assume word copy, and assume
 * that the copy regions are disjoint.  
 */
static void
CVMgenMarkCompactCopyDisjointWords(CVMUint32* dest, CVMUint32* source, 
				   CVMUint32 numBytes)
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
CVMgenMarkCompactCopyDown(CVMUint32* dest, CVMUint32* source,
			  CVMUint32 numBytes)
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
CVMgenMarkCompactPromoteInto(CVMGeneration* gen,
			     CVMObject* objectToPromote,
			     CVMUint32  objectSize)
{
    CVMObject* dest;
    CVMgenContiguousSpaceAllocate(gen, objectSize, dest);
    if (dest != NULL) {
	CVMgenMarkCompactCopyDisjointWords((CVMUint32*)dest,
					 (CVMUint32*)objectToPromote,
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

/*
 * Allocate a MarkCompact generation.
 */
CVMGeneration*
CVMgenMarkCompactAlloc(CVMUint32* space, CVMUint32 totalNumBytes)
{
    CVMGenMarkCompactGeneration* thisGen  = (CVMGenMarkCompactGeneration*)
	calloc(sizeof(CVMGenMarkCompactGeneration), 1);
    CVMUint32 numBytes = totalNumBytes; /* Respect the no. of bytes passed */

    if (thisGen == NULL) {
	return NULL;
    }

    /*
     * Initialize thisGen. 
     */
    thisGen->gen.heapTop = &space[numBytes / 4];
    thisGen->gen.heapBase = &space[0];
    thisGen->gen.allocTop = thisGen->gen.heapTop;
    thisGen->gen.allocPtr = thisGen->gen.heapBase;
    thisGen->gen.allocBase = thisGen->gen.heapBase;
    thisGen->gen.allocMark = thisGen->gen.heapBase;

    /* 
     * And finally, set the function pointers for this generation
     */
    thisGen->gen.collect = CVMgenMarkCompactCollect;
    thisGen->gen.scanOlderToYoungerPointers =
	CVMgenMarkCompactScanOlderToYoungerPointers;
    thisGen->gen.scanYoungerToOlderPointers =
	CVMgenMarkCompactScanYoungerToOlderPointers;
    thisGen->gen.promoteInto = 
	CVMgenMarkCompactPromoteInto;
    thisGen->gen.scanPromotedPointers =
	CVMgenMarkCompactScanPromotedPointers;
    thisGen->gen.startGC = CVMgenMarkCompactStartGC;
    thisGen->gen.endGC = CVMgenMarkCompactEndGC;
    thisGen->gen.freeMemory = CVMgenMarkcompactFreeMemory;
    thisGen->gen.totalMemory = CVMgenMarkcompactTotalMemory;

    CVMdebugPrintf(("GC[MC]: Initialized mark-compact gen "
		  "for generational GC\n"));
    CVMdebugPrintf(("\tSize of the space in bytes=%d\n"
		  "\tLimits of generation = [0x%x,0x%x)\n",
		  numBytes, thisGen->gen.allocBase, thisGen->gen.allocTop));
    return (CVMGeneration*)thisGen;
}

/*
 * Free all the memory associated with the current mark-compact generation
 */
void
CVMgenMarkCompactFree(CVMGenMarkCompactGeneration* thisGen)
{
    free(thisGen);
 }

static CVMObject*
CVMgenMarkCompactGetForwardingPtr(CVMGenMarkCompactGeneration* thisGen,
				  CVMObject* ref)
{
    CVMAddr  forwardingAddress = CVMobjectVariousWord(ref);
    CVMassert(CVMgenMarkCompactInGeneration(thisGen, ref));
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

    if (!CVMobjectTrivialClassWord(originalWord)) {
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
	CVMMCPreservedItem* item = &thisGen->preservedItems[idx];
	CVMObject* movedRef = item->movedRef;
	/*
	 * Make sure that the object we are looking at now has the default
	 * various word set up for now
	 */
	CVMassert(CVMobjectTrivialClassWord(CVMobjectVariousWord(movedRef)));
	/*
	 * And also, that we did not evacuate this word for nothing
	 */
	CVMassert(!CVMobjectTrivialClassWord(item->originalWord));
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
static void
preserveHeaderWord(CVMGenMarkCompactGeneration* thisGen,
		   CVMObject* movedRef, CVMAddr originalWord)
{
    CVMMCPreservedItem* item = &thisGen->preservedItems[thisGen->preservedIdx];

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

/* Sweep the heap, compute the compacted addresses, write them into the
   original object headers, and return the new allocPtr of this space. */
static CVMUint32*
sweep(CVMGenMarkCompactGeneration* thisGen, CVMUint32* base, CVMUint32* top)
{
    CVMUint32* forwardingAddress = base;
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,%d]: Sweeping object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));
    while (curr < top) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMAddr    classWord  = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	if (CVMobjectMarkedOnClassWord(classWord)) {
	    volatile CVMAddr* headerAddr   = &CVMobjectVariousWord(currObj);
	    CVMAddr  originalWord = *headerAddr;
	    CVMtraceGcScan(("GC[MC,%d]: obj 0x%x -> 0x%x\n",
			    thisGen->gen.generationNo, curr,
			    forwardingAddress));
	    if (!CVMobjectTrivialClassWord(originalWord)) {
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
	    {
	        extern CVMUint32 liveObjectCount;
                if (CVMjvmpiEventObjectFreeIsEnabled()) {
                    CVMjvmpiPostObjectFreeEvent(currObj);
                }
                liveObjectCount--;
	    }
#endif
#ifdef CVM_INSPECTOR
	    /* We only need to report this if there are captured states that
	       need to be updated: */
	    if (CVMglobals.inspector.hasCapturedState) {
    	        CVMgcHeapStateObjectFreed(currObj);
	    }
#endif
	}
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
    CVMtraceGcCollect(("GC[MC,%d]: Swept object range [%x,%x) -> 0x%x\n",
		       thisGen->gen.generationNo, base, top,
		       forwardingAddress));
    return forwardingAddress;
}

/* Finally we can move objects, reset marks, and restore original
   header words. */
static void 
compact(CVMGenMarkCompactGeneration* thisGen, CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,%d]: Compacting object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));
    while (curr < top) {
	CVMObject*     currObj   = (CVMObject*)curr;
	CVMAddr        classWord = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb    = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32      objSize   = CVMobjectSizeGivenClass(currObj, currCb);

	if (CVMobjectMarkedOnClassWord(classWord)) {
	    CVMUint32* destAddr = (CVMUint32*)
		CVMgenMarkCompactGetForwardingPtr(thisGen, currObj);
	    CVMobjectClearMarkedOnClassWord(classWord);
#ifdef CVM_DEBUG
	    /* For debugging purposes, make sure the deleted mark is
	       reflected in the original copy of the object as well. */
	    CVMobjectSetClassWord(currObj, classWord);
#endif
	    CVMgenMarkCompactCopyDown(destAddr, curr, objSize);

#ifdef CVM_JVMPI
            if (CVMjvmpiEventObjectMoveIsEnabled()) {
                CVMjvmpiPostObjectMoveEvent(1, curr, 1, destAddr);
            }
#endif
#ifdef CVM_INSPECTOR
	    if (CVMglobals.inspector.hasCapturedState) {
	        CVMgcHeapStateObjectMoved(currObj, (CVMObject *)destAddr);
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
    /* Restore the "non-trivial" old header words
       with the new address of object */
    restorePreservedHeaderWords(thisGen);
    CVMassert(curr == top); /* This had better be exact */
}

/*
 * Gray an object known to be in the old MarkCompact generation.
 */
static void
CVMgenMarkCompactGrayObject(CVMGenMarkCompactGeneration* thisGen,
			  CVMObject* ref)
{
    /*
     * ROM objects should have been filtered out by the time we get here
     */
    CVMassert(!CVMobjectIsInROM(ref));

    /*
     * Also, all objects that don't belong to this generation need to have
     * been filtered out by now.
     */
    CVMassert(CVMgenMarkCompactInGeneration(thisGen, ref));

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
CVMgenMarkCompactFilteredGrayObject(CVMGenMarkCompactGeneration* thisGen,
				    CVMObject* ref)
{
    /*
     * Gray only if the slot points to this generation's from space. 
     *
     * Also handles the case of root slots that have been already
     * scanned and grayed, and therefore contain a pointer into
     * to-space -- those are ignored.
     */
    if (CVMgenMarkCompactInGeneration(thisGen, ref)) {
	CVMgenMarkCompactGrayObject(thisGen, ref);
    }
}

static void
CVMgenMarkCompactFilteredGrayCallback(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
    CVMObject* ref = *refPtr;
    CVMgenMarkCompactFilteredGrayObject(thisGen, ref);
}

static void
CVMgenMarkCompactFilteredUpdateRoot(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
    CVMObject* ref = *refPtr;
    if (!CVMgenMarkCompactInGeneration(thisGen, ref)) {
	return;
    }
    /* The forwarding address is valid only for already marked
       objects. If the object is not marked, then this slot has already
       been 'seen'. */
    CVMassert(CVMobjectMarked(ref));
    *refPtr = CVMgenMarkCompactGetForwardingPtr(thisGen, ref);
}

typedef struct CVMGenMarkCompactTransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
    CVMGenMarkCompactGeneration* thisGen;
} CVMGenMarkCompactTransitiveScanData;

static void
CVMgenMarkCompactBlackenObject(CVMGenMarkCompactTransitiveScanData* tsd,
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
	    CVMgenMarkCompactFilteredGrayObject(thisGen, *refPtr);
	}
    }, CVMgenMarkCompactFilteredGrayCallback, thisGen);
}

static void
CVMgenMarkCompactFollowRoots(CVMGenMarkCompactGeneration* thisGen,
			     CVMGenMarkCompactTransitiveScanData* tsd)
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
            CVMMCPreservedItem* item = &thisGen->preservedItems[idx];
            CVMassert(idx >= 0);
            CVMassert(item->movedRef == ref);
            CVMassert(!CVMobjectTrivialClassWord(item->originalWord));
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
	CVMgenMarkCompactBlackenObject(tsd, ref, refCB);
    }
}

static void
CVMgenMarkCompactScanTransitively(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactTransitiveScanData* tsd =
	(CVMGenMarkCompactTransitiveScanData*)data;
    CVMGenMarkCompactGeneration* thisGen = tsd->thisGen;
    CVMObject* ref;

    CVMassert(refPtr != 0);
    ref = *refPtr;
    CVMassert(ref != 0);

    CVMgenMarkCompactFilteredGrayObject(thisGen, ref);

    /*
     * Now scan all its children as well
     */
    CVMgenMarkCompactFollowRoots(thisGen, tsd);
}


/*
 * Test whether a given reference is live or dying. If the reference
 * does not point to the current generation, answer TRUE. The generation
 * that does contain the pointer will give the right answer.
 */
static CVMBool
CVMgenMarkCompactRefIsLive(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
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
    if (!CVMgenMarkCompactInGeneration(thisGen, ref)) {
	return CVM_TRUE;
    }
    /* Did somebody else already scan or forward this object? It's live then */
    return CVMobjectMarked(ref);
}

static CVMBool
CVMgenMarkCompactCollect(CVMGeneration* gen,
			 CVMExecEnv*    ee, 
			 CVMUint32      numBytes, /* collection goal */
			 CVMGCOptions*  gcOpts)
{
    CVMGenMarkCompactTransitiveScanData tsd;
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)gen;
    CVMBool success;
    CVMGenSpace* extraSpace;
    CVMUint32* newAllocPtr;

    CVMtraceGcStartStop(("GC[MC,%d,full]: Collecting\n",
			 gen->generationNo));

    extraSpace = gen->prevGen->getExtraSpace(gen->prevGen);

    /* Prepare the data structure that preserves original second header
       words for use in the TODO stack. */
    thisGen->preservedItems = (CVMMCPreservedItem*)extraSpace->allocBase;
    thisGen->preservedIdx   = 0;
    thisGen->preservedSize  = (CVMUint32)
	((CVMMCPreservedItem*)extraSpace->allocTop - 
	 (CVMMCPreservedItem*)extraSpace->allocBase);
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
		       ee, gcOpts, CVMgenMarkCompactScanTransitively, &tsd);
    /*
     * Don't discover any more weak references.
     */
    gcOpts->discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    CVMgcProcessSpecialWithLivenessInfo(ee, gcOpts,
					CVMgenMarkCompactRefIsLive, thisGen,
					CVMgenMarkCompactScanTransitively,
					&tsd);

    /* Reset the data structure that preserves original second header
       words. We are done with the TODO stack, so we can re-use extraSpace. */
    thisGen->preservedItems = (CVMMCPreservedItem*)extraSpace->allocBase;
    thisGen->preservedIdx   = 0;
    thisGen->preservedSize  = (CVMUint32)
	((CVMMCPreservedItem*)extraSpace->allocTop - 
	 (CVMMCPreservedItem*)extraSpace->allocBase);
	
    /* The sweep phase. This phase computes the new addresses of each
       object and writes it in the second header word. Evacuated
       original header words will be kept in the 'preservedItems'
       list. */
    newAllocPtr = sweep(thisGen, gen->allocBase, gen->allocPtr);
    CVMassert(newAllocPtr <= gen->allocPtr);

    /* At this point, the new addresses of each object are written in
       the headers of the old. Update all pointers to refer to the new
       copies. */

    /* Update the roots */
    CVMgenScanAllRoots((CVMGeneration*)thisGen, ee, gcOpts,
		       CVMgenMarkCompactFilteredUpdateRoot, thisGen);
    CVMgcScanSpecial(ee, gcOpts,
		     CVMgenMarkCompactFilteredUpdateRoot, thisGen);
    /* And update all interior pointers */
    scanObjectsInRangeSkipUnmarked(thisGen,
				   ee, gcOpts, gen->allocBase, gen->allocPtr);
    
    /* Finally we can move objects, reset marks, and restore original
       header words. */
    compact(thisGen, gen->allocBase, gen->allocPtr);

    gen->allocPtr = newAllocPtr;

    /*
     * Now since we are not in the young generation, we have to
     * rebuild the barrier structures.  
     */
    gen->allocMark = newAllocPtr;
    CVMgenClearBarrierTable();
    CVMgenMarkCompactRebuildBarrierTable(thisGen, ee, gcOpts,
					 gen->allocBase,
					 gen->allocPtr);
    success = (numBytes / 4 <= (CVMUint32)(gen->allocTop - gen->allocPtr));

    CVMtraceGcStartStop(("GC[MC,%d,full]: Done, success for %d bytes %s\n",
			 gen->generationNo, numBytes,
			 success ? "TRUE" : "FALSE"));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;
}
