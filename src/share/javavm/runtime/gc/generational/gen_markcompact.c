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
#include "javavm/include/porting/ansi/setjmp.h"
#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif


/*
   How the mark-compact GC works?
   ==============================
   The mark-compact collector will mark objects in the entire heap (not only
   those in the oldGen region) by traversing the tree of all live objects.
   Hence, there is no need to scan the card table or youngGen heap for
   youngerToOlder references.  All live objects will be discovered by the GC
   roots traversal in step 1.

   However, even though the youngGen objects are scanned and marked, the
   youngGen will not be compacted.  This is because the youngGen was already
   collected using semispace collection before the oldGen is invoke.  Hence,
   it is very likely that it contains only live objects.  Attempting to
   compact the youngGen also here will yield only wasted work except for
   some edge conditions.  In those cases, the youngGen can afford to wait
   till the next GC cycle to compact out its garbage.

   But, even though the youngGen is not compacted, the fact that we do a
   complete root scan that oes not mark unreachable objects in the youngGen
   means that dead objects in the youngGen will essentially go through all
   the phases of collection except for compaction.  For example, finalizable
   objects will be finalized; the native counterparts of unreachable classes
   and classloaders in the C heap will be freed; the classloader loadcache,
   and string intern table will be updated; unreachable monitors will be
   force-unlocked and released.  This has some ramifications (read more
   below).

   Also since the youngGen objects are marked during the mark phase, they
   will need to be unmarked before the collector is finished.

   The collection process goes as follows:
   1. Traverse all roots and mark all live objects in the entire heap.

      We traverse the roots to mark objects in the youngGen too because we
      don't want to assume that all objects in the youngGen are alive.  In
      the edge case where we have a network of dead objects spanning the
      young and old generations, such an assumption would keep dead objects
      in the oldGen from being collected even though they are now dead.

      Note that weakrefs are also discovered during this traversal.  Hence,
      dead objects with finalizers which are found in the youngGen will
      also be resurrected through the normal Reference processing mechanism.
      If such objects are resurrected to be finalized, the objects which
      they reference which could otherwise have been dead would also be
      kept alive accordingly.

      One side effect of this is that even though this collector does not
      compact the youngGen heap region, the collector will enqueue
      finalizable dead youngGen objects in the finalization queue.  Hence,
      it will not require another youngGen to discover these finalizable
      objects.  They will get finalized after this GC cycle due to this
      collector.  This side effect is still functionally correct as far as
      garbage collection goes and is allowed.

      Note: We cannot rely on the card table to reveal all pointers from the
      youngGen into the oldGen.  This is because at the end of the oldGen,
      we optimize the card table to only record pointers from the oldGen to
      the youngGen.  This speeds up youngGen collection.

   2. Compute forwarding addresses for live objects by sweeping the oldGen
      region.

      The sweep will use a mechanism to record forwarded addresses.
      Forwarding addresses are stored in object header words.  The header
      words of some objects may need to be preserved if it is not trivial.
      Trivial means that it can be reconstructed without additional info
      from the original header word.

   3. Update all interior object pointers (i.e. pointers in the fields of
      heap objects) with the forwarded address information.

      Previously, CVMgenScanAllRoots() would call scanYoungerToOlderPointers()
      which would iterate over all objects in the youngGen.  This is how
      the youngGen references to oldGen objects used to be updated.  

      But now, since we mark the live objects in the youngGen too,
      scanYoungerToOlderPointers() has been obsoleted and removed.  After the
      sweep, we call scanObjectsInRangeSkipUnmarked() on the youngGen region
      instead.  This will scan all live objects in the youngGen and update
      their pointers.  Dead objects in the youngGen are left untouched.

      Note that since finalizable objects in the youngGen are handled in the
      same manner as finalizable objects in the oldGen, dead youngGen objects
      that are finalizable will be resurrected and put on the finalization
      queue.  Hence, they will treated like live objects.

      Class unloading is triggered by CVMgcProcessSpecialWithLivenessInfo()
      which calls CVMclassDoClassUnloadingPass1().  Class unloading is done
      based on an isLive() query.  isLive() previously declares all youngGen
      objects to be alive.  But now, it checks if the object was previously
      marked by a root scan.  Hence, unreachable classes and classloaders
      in the youngGen will also be collected (though not compacted).
          Since collecting classes and classloaders involved freeing their
      native counterparts (e.g. the classblock), the unreachable objects
      in the yougGen heap can have invalid classblock pointers.  Hence, we
      must not scan these objects after CVMclassDoClassUnloadingPass2() is
      called.
          Fortunately, the youngGen scanning algorithm only scans the
      youngGen heap using a root scan.  It does not iterate the heap region
      and is therefore not dependent on the availability of correct
      classblock info in dead objects.
          The only exception to this is when CVM_INSPECTOR or CVM_JVMPI is
      enabled.  Hence, when those features are enabled, we need to replace
      the dead object with an equivalent object of the same size that is
      not dependent on the unloaded classblock.  For these objects, we can
      use instances of java.lang.Object or an int[].  Let's call these
      replacement objects place holders.
          For JVMPI, we also need to send object free events for the objects
      we are collecting (though not compacted).  We need to do this before
      the state of the object is destroyed due to collection i.e. also before
      replacing it with a place holder.  Also, the place holders need to be
      marked accordingly so that no JVMPI object free events are sent for
      them when the youngGen is collected subsequently.  The marking
      basically indicates that these place holder objects are synthesized by
      the GC and are not known to the JVMPI agent i.e. no object alloc
      event for it was sent previously.

   4. Unmark objects in the youngGen region.
      Compact the live objects in the oldGen region.

   5. Restore preserved words from the preserved words database.  These are
      the preserved words that were added during the sweep phase.

   6. Update the alloc pointers to reflect their newly compacted allocation
      top.

   7. Reconstruct the card table barrier to reflect the new state of
      old to young pointers.  We don't care about young to old pointers
      and would rather leave them out of the computation since that
      will improve the efficiency of youngGen GC cycles in the
      next GC cycle.

   About GC aborts:
   ================
   Currently, the only reason for an abort is when we run out of scratch
   memory to preserve header words while doing GC scans.  If this happens,
   we will need to undo any partially done work so far so as to not leave
   the system in an unstable state.

   NOTE: We only allocate scratch memory while in the marking or sweeping
   phase.  Hence, an abort can only happen in these 2 phases.  And we'll
   need to implement undo handling accordingly.

   The mark and sweep phases may replaces object header words with links
   or forwarding addresses.  The undo operation need to restore the original
   object header words.

   About Weakrefs handling:
   ========================
   Unlike the semispace GC, CVMweakrefFinalizeProcessing() (which is called
   from CVMgcProcessSpecialWithLivenessInfo()) is not expected to update the
   next and referent fields in the References that are kept alive.  Object
   motion happens after it in the GC sweep phase.  After sweeping,
   CVMgcScanSpecial() will call CVMweakrefUpdate() to update pointers in
   JNI weakrefs.  As for the next and referrent fields of References, they
   will be automaticly taken cared of by the general root scan
   CVMgenScanAllRoots() which does not filter out references this time
   around since gcOpts->discoverWeakReferences is now set to FALSE.

*/


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


/* GC phases of the mark-compact collector.  Used for identifying how much
   and type of clean-up to do should an error occur during GC. */
enum {
    GC_PHASE_RESET = 0,
    GC_PHASE_MARK,
    GC_PHASE_SWEEP,
};

static void
gcError(CVMGenMarkCompactGeneration* thisGen)
{
    longjmp(thisGen->errorContext, 1);
}

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
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    updateCTForSlot(refPtr, *refPtr, thisGen);
	});
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

#define CHUNK_SIZE 2*1024*1024

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

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)

void CVMgcReplaceWithPlaceHolderObject(CVMObject *currObj, CVMUint32 objSize)
{
    CVMClassBlock *objCb;
    if (objSize > sizeof(CVMObjectHeader)) {
        /* Replace with an instance of int[]: */
        CVMArrayOfAnyType *array = (CVMArrayOfAnyType *)currObj;
        CVMUint32 arrayLength;
        CVMUint32 sizeOfArrayHeader =
	    sizeof(CVMObjectHeader) + sizeof(CVMJavaInt);
#ifdef CVM_64
	sizeOfArrayHeader += sizeof(CVMUint32) /* pad */;
#endif
        arrayLength = (objSize - sizeOfArrayHeader) / sizeof(CVMJavaInt);
	CVMassert(objSize >= sizeOfArrayHeader);
	array->length = arrayLength;

        objCb = CVMsystemClass(manufacturedArrayOfInt);

    } else {
        /* Replace with an instance of java.lang.Object: */
        objCb = CVMsystemClass(java_lang_Object);
        CVMassert(objSize == sizeof(CVMObjectHeader));
	CVMassert(sizeof(CVMObjectHeader) == CVMcbInstanceSize(objCb));
    }

    CVMobjectSetClassWord(currObj, objCb);
    CVMobjectVariousWord(currObj) = CVM_OBJECT_DEFAULT_VARIOUS_WORD |
                                    CVM_GEN_SYNTHESIZED_OBJ_MARK;

#ifdef CVM_DEBUG_ASSERTS
    {
	CVMAddr classWord  = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32 size = CVMobjectSizeGivenClass(currObj, currCb);
        CVMassert(size == objSize);
    }
#endif
}

/*
 * Scan objects in contiguous range, and do all special handling as well.
 * Replace unmarked objects with equivalent sized place holder objects.
 */
static void
scanObjectsInYoungGenRange(CVMGenMarkCompactGeneration* thisGen,
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
#ifdef CVM_JVMTI
	    {
                if (CVMjvmtiShouldPostObjectFree()) {
                    CVMjvmtiPostObjectFreeEvent(currObj);
                }
	    }
#endif
#ifdef CVM_INSPECTOR
	    /* We only need to report this if there are captured states that
	       need to be updated: */
	    if (CVMglobals.inspector.hasCapturedState) {
    	        CVMgcHeapStateObjectFreed(currObj);
	    }
#endif
	    CVMgcReplaceWithPlaceHolderObject(currObj, objSize);
	}
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
}

#else
#define scanObjectsInYoungGenRange scanObjectsInRangeSkipUnmarked
#endif /* CVM_INSPECTOR || CVM_JVMPI || CVM_JVMTI */


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
        CVMUint32* nextChunk = curr + CHUNK_SIZE;
        if (nextChunk > top) {
            nextChunk = top;
        }

        while (curr < nextChunk) {
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
        CVMthreadSchedHook(CVMexecEnv2threadID(ee));
    }
    CVMassert(curr == top); /* This had better be exact */
}

/*
 * Re-build the old-to-young pointer. 
 */
void
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

#ifdef CVM_DEBUG_ASSERTS
static CVMGenSpace*
CVMgenMarkCompactGetExtraSpace(CVMGeneration* gen)
{
    CVMassert(CVM_FALSE);
    return NULL;
}
#endif

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
    CVMmemmoveForGC(s, d, n);
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
        CVMmemmoveForGC(s, d, n);
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
    thisGen->gen.promoteInto = 
	CVMgenMarkCompactPromoteInto;
    thisGen->gen.scanPromotedPointers =
	CVMgenMarkCompactScanPromotedPointers;
#ifdef CVM_DEBUG_ASSERTS
    thisGen->gen.getExtraSpace = CVMgenMarkCompactGetExtraSpace;
#endif
    thisGen->gen.freeMemory = CVMgenMarkcompactFreeMemory;
    thisGen->gen.totalMemory = CVMgenMarkcompactTotalMemory;

#if defined(CVM_DEBUG)
    CVMgenMarkCompactDumpSysInfo(thisGen);
#endif /* CVM_DEBUG */

    return (CVMGeneration*)thisGen;
}

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the markcompact generation. */
void CVMgenMarkCompactDumpSysInfo(CVMGenMarkCompactGeneration* thisGen)
{
    CVMUint32 numBytes;

    numBytes = (thisGen->gen.heapTop - thisGen->gen.heapBase) *
	       sizeof(CVMUint32);

    CVMconsolePrintf("GC[MC]: Initialized mark-compact gen "
		     "for generational GC\n");
    CVMconsolePrintf("\tSize of the space in bytes=%d\n"
		     "\tLimits of generation = [0x%x,0x%x)\n",
		     numBytes, thisGen->gen.allocBase, thisGen->gen.allocTop);
}
#endif /* CVM_DEBUG || CVM_INSPECTOR */

/*
 * Free all the memory associated with the current mark-compact generation
 */
void
CVMgenMarkCompactFree(CVMGenMarkCompactGeneration* thisGen)
{
    free(thisGen);
}

static CVMObject*
CVMgenMarkCompactGetForwardingPtr(CVMObject* ref)
{
    CVMAddr  forwardingAddress = CVMobjectVariousWord(ref);
    CVMassert(CVMobjectMarked(ref));
    return (CVMObject*)forwardingAddress;
}

#ifdef MAX_STACK_DEPTHS
static CVMUint32 preservedIdxMax = 0;
#endif

static void
preserveHeaderWord(CVMGenMarkCompactGeneration* thisGen,
                   CVMObject* movedRef, CVMAddr originalWord);

/* NOTE: The todoList behaves like a stack.  We add to it by pushing an item
   on to it.  We remove from it by popping an item off.  Because the add and
   remove is ordered in a stack fashion, we can get efficient memory usage
   of the preservedItems database without having to maintain a free list.
*/

/* Adds an item to the todoList. */
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

/* Pops an item and removes it from the todoList. */
static CVMObject *
popFromTodo(CVMGenMarkCompactGeneration* thisGen)
{
    CVMObject* ref = thisGen->todoList;
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
        CVMassert(!CVMobjectTrivialHeaderWord(item->originalWord));
        *headerAddr = item->originalWord;
        CVMtraceGcScan(("GC[MC,%d]: "
                        "Restoring header %x for obj 0x%x at i=%d\n",
                        thisGen->gen.generationNo, item->originalWord,
                        ref, idx));
    } else {
        /* NOTE: popFromTodo can be used to handle objects in the youngGen
	   as well.  For those, we want to ensure that their GC age is at
	   least 1.  This is because we must have at least gone through one
	   youngGen GC before getting here.  If we simply reset the header
	   word to CVM_OBJECT_DEFAULT_VARIOUS_WORD, then the age will be
	   reset to 0 thereby causing the object to stay longer in the
	   youngGen then it should.  Setting the age to 1 does not guarantee
	   that we have the proper age of the object, but it is at least
	   closer to the real age than an age of 0.
	*/
        *headerAddr = CVM_OBJECT_DEFAULT_VARIOUS_WORD | (1 << CVM_GC_SHIFT);
    }
    return ref;
}

/* Checks to see if the todoList is empty. */
/* static CVMBool
   todoListIsEmpty(CVMGenMarkCompactGeneration* thisGen); */
#define todoListIsEmpty(thisGen)    ((thisGen)->todoList == NULL)

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
static void
preserveHeaderWord(CVMGenMarkCompactGeneration* thisGen,
		   CVMObject* movedRef, CVMAddr originalWord)
{
    CVMMCPreservedItem* item = &thisGen->preservedItems[thisGen->preservedIdx];

    if (thisGen->preservedIdx >= thisGen->preservedSize) {
        thisGen->lastProcessedRef = movedRef;
        gcError(thisGen);
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
sweep(CVMExecEnv* ee, CVMGenMarkCompactGeneration* thisGen,
      CVMUint32* base, CVMUint32* top)
{
    CVMUint32* forwardingAddress = base;
    CVMUint32* curr = base;

    CVMtraceGcCollect(("GC[MC,%d]: Sweeping object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));
    while (curr < top) {
        CVMUint32* nextChunk = curr + CHUNK_SIZE;
        if (nextChunk > top) {
            nextChunk = top;
        }

        while (curr < nextChunk) {
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
	        {
		    extern CVMUint32 liveObjectCount;
                    if (CVMjvmpiEventObjectFreeIsEnabled()) {
                        CVMjvmpiPostObjectFreeEvent(currObj);
                    }
		    liveObjectCount--;
	        }
#endif
#ifdef CVM_JVMTI
	        {
                    if (CVMjvmtiShouldPostObjectFree()) {
                        CVMjvmtiPostObjectFreeEvent(currObj);
                    }
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
        CVMthreadSchedHook(CVMexecEnv2threadID(ee));
    }
    CVMassert(curr == top); /* This had better be exact */
    CVMtraceGcCollect(("GC[MC,%d]: Swept object range [%x,%x) -> 0x%x\n",
		       thisGen->gen.generationNo, base, top,
		       forwardingAddress));
    return forwardingAddress;
}

static void 
unmark(CVMGenMarkCompactGeneration* thisGen, CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,%d]: Unmarking object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));
    while (curr < top) {
	CVMObject*     currObj   = (CVMObject*)curr;
	CVMAddr        classWord = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb    = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32      objSize   = CVMobjectSizeGivenClass(currObj, currCb);

	if (CVMobjectMarkedOnClassWord(classWord)) {
	    CVMobjectClearMarkedOnClassWord(classWord);
	    CVMobjectSetClassWord(currObj, classWord);
	}
	/* iterate */
	curr += objSize / 4;
    }

    CVMassert(curr == top); /* This had better be exact */
}

/* Purpose: Get the header words that were saved away by preservedHeaderWord.
            This should only be called by unsweepAndUnmark() below.  Since
	    unsweepAndUnmark() scans the heap in exactly the same order as
	    sweep(), we should be getting the objects in exactly the same
	    order as was preserved by preservedHeaderWords.  This should allow
	    us to restore the original words to the objects.

  NOTE: We cannot use restorePreservedHeaderWords() to restore the header
        words because it assumes that the objects has been moved and does its
        restoration based on forwarded pointers.  We want to restore the
        original objects because we're aborting the GC before the object
	motion in the compact phase.
*/
static CVMAddr
getHeaderWord(CVMGenMarkCompactGeneration* thisGen, CVMObject* ref,
	      CVMObject* forwardedRef)
{
    CVMAddr result = CVM_OBJECT_DEFAULT_VARIOUS_WORD;

    /* We only do restorations when we encountered an error due to having
       exhausted the preservedSize.  So, we only have preserved words for
       the case when we haven't exhausted the preservedSize yet. */

    if (thisGen->lastProcessedRef != NULL) {
        if (thisGen->lastProcessedRef == forwardedRef) {
            /* We have reached the last of the list of objects that were
               swept: */
	    /* NOTE: The lastProcessedRef points to the next object that
	       would have been swept if we had not run out of scratch space.
	       Hence, it's header word is still intact.  Nothing needs to
	       be done to restore it.
	    */
            thisGen->lastProcessedRef = NULL; /* Indicate we're done. */
            goto noMoreSweptObjects;
        } else {
            CVMMCPreservedItem* item;

            CVMassert(thisGen->preservedIdx <= thisGen->preservedSize);
            item = &thisGen->preservedItems[thisGen->preservedIdx];

            /* If the forwarding address in the object's header matches the
	       current preserved item in the list, then the original word
	       must have been preserved in the preserved item.  Restore this
	       word and increment the preserved index.

	       Otherwise, the original word must have been the default
	       various word value (i.e. CVM_OBJECT_DEFAULT_VARIOUS_WORD)
	       or equivalent.  The only difference may be the GC age, but
	       that information is inconsequential for objects in the old
	       generation.  Hence, we simply restore it to the default word.
	    */
            if (item->movedRef == forwardedRef) {
                result = item->originalWord;
                thisGen->preservedIdx++;
	    }
	}
    } else {
noMoreSweptObjects:
        /* This object's header wasn't changed.  Just return the original word
	   in the object header: */
        result = CVMobjectVariousWord(ref);
    }
    return result;
}

/* Purpose: Restore the objects header words modified by the sweep phase. */
static void
unsweepAndUnmark(CVMGenMarkCompactGeneration* thisGen, CVMUint32* base,
		 CVMUint32* top)
{
    CVMUint32* forwardingAddress = base;
    CVMUint32* curr = base;
    while (curr < top) {
	CVMObject*     currObj   = (CVMObject*)curr;
	CVMAddr        classWord = CVMobjectGetClassWord(currObj);
	CVMClassBlock* currCb    = CVMobjectGetClassFromClassWord(classWord);
	CVMUint32      objSize   = CVMobjectSizeGivenClass(currObj, currCb);
	if (CVMobjectMarkedOnClassWord(classWord)) {
	    if (thisGen->lastProcessedRef != NULL) {
	        volatile CVMAddr* headerAddr = &CVMobjectVariousWord(currObj);
	        CVMAddr  originalWord;

	        originalWord = getHeaderWord(thisGen, currObj,
					     (CVMObject *)forwardingAddress);
	        *headerAddr = originalWord;
	        /* Pretend to have copied this live object! */
	        forwardingAddress += objSize / 4;
            }

            /* Deleted mark in the object. */
	    /* NOTE: The sweep phase would leave the mark on the object
	       intact.  It's the compact phase that unmarks it.  But in this
	       case, we are already iterating the heap to undo the sweep
	       effects in an abort operation.  Hence, we'll take this
	       opportunity to unmark the objects too. */
            CVMobjectClearMarkedOnClassWord(classWord);
            CVMobjectSetClassWord(currObj, classWord);
        }

	/* iterate */
	curr += objSize / 4;
    }

    CVMassert(curr == top); /* This had better be exact */
}

/* Finally we can move objects, reset marks, and restore original
   header words. */
static void 
compact(CVMExecEnv* ee, CVMGenMarkCompactGeneration* thisGen,
        CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;
    CVMtraceGcCollect(("GC[MC,%d]: Compacting object range [%x,%x)\n",
		       thisGen->gen.generationNo, base, top));
    while (curr < top) {
        CVMUint32* nextChunk = curr + CHUNK_SIZE;
        if (nextChunk > top) {
            nextChunk = top;
        }

        while (curr < nextChunk) {
	    CVMObject*     currObj   = (CVMObject*)curr;
	    CVMAddr        classWord = CVMobjectGetClassWord(currObj);
	    CVMClassBlock* currCb = CVMobjectGetClassFromClassWord(classWord);
	    CVMUint32      objSize = CVMobjectSizeGivenClass(currObj, currCb);

	    if (CVMobjectMarkedOnClassWord(classWord)) {
	        CVMUint32* destAddr = (CVMUint32*)
		    CVMgenMarkCompactGetForwardingPtr(currObj);
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
        CVMthreadSchedHook(CVMexecEnv2threadID(ee)); 
    }

    CVMassert(curr == top); /* This had better be exact */
}

/* Gray an object known to be alive. */
static void
CVMgenMarkCompactGrayObject(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactGeneration* thisGen = (CVMGenMarkCompactGeneration*)data;
    CVMObject* ref = *refPtr;

    if (CVMobjectIsInROM(ref)) {
        return;
    }

    /*
     * ROM objects should have been filtered out by the time we get here
     */
    CVMassert(!CVMobjectIsInROM(ref));

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
       been 'seen'.
    */
    CVMassert(CVMobjectMarked(ref));
    *refPtr = CVMgenMarkCompactGetForwardingPtr(ref);
}

typedef struct CVMGenMarkCompactTransitiveScanData {
    CVMExecEnv* ee;
    CVMGCOptions* gcOpts;
    CVMGenMarkCompactGeneration* thisGen;
} CVMGenMarkCompactTransitiveScanData;

/* Scan and mark the references within an object and add them to the todo
   list if necessary. */
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
	    CVMgenMarkCompactGrayObject(refPtr, thisGen);
	}
    }, CVMgenMarkCompactGrayObject, thisGen);
}

/* Scan all references in the objects that have been placed on the todo list
   so that all the children can be scanned and marked as well. */
static void
CVMgenMarkCompactFollowRoots(CVMGenMarkCompactGeneration* thisGen,
			     CVMGenMarkCompactTransitiveScanData* tsd)
{
    CVMtraceGcCollect(("GC[MC,%d]: Following roots\n",
		       thisGen->gen.generationNo));

    /* Go through the todo list and blacken each object in it.  To blacken
       means to scan the references it contains.  Note that this may cause
       more referred objects to be put on the todolist.  We'lll just keep
       processing the todo list until all objects in it has been blackened.
    */
    while (!todoListIsEmpty(thisGen)) {
        CVMObject* ref = popFromTodo(thisGen);
        CVMClassBlock* refCB = CVMobjectGetClass(ref);
	/* Blackening may queue up more objects: */
	CVMgenMarkCompactBlackenObject(tsd, ref, refCB);
    }
}

/* Mark the specified object as live and add it to a todo list
   so that its children can be scanned and marked as live too. */
static void
CVMgenMarkCompactScanTransitively(CVMObject** refPtr, void* data)
{
    CVMGenMarkCompactTransitiveScanData* tsd =
	(CVMGenMarkCompactTransitiveScanData*)data;
    CVMGenMarkCompactGeneration* thisGen = tsd->thisGen;

    CVMassert(refPtr != 0);
    CVMassert(*refPtr != 0);

    /* Mark the immediate object and put it on the todo list to have its
       children scanned as well if necessary: */
    CVMgenMarkCompactGrayObject(refPtr, thisGen);

    /* Now scan all its children as well by going through the todo list: */
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
    CVMGeneration* youngGen = gen->prevGen;

    CVMtraceGcCollect(("GC[MC,%d,full]: Starting GC\n", gen->generationNo));
    CVMtraceGcStartStop(("GC[MC,%d,full]: Collecting\n",
			 gen->generationNo));

    CVMassert(GC_PHASE_RESET == thisGen->gcPhase);
    extraSpace = youngGen->getExtraSpace(youngGen);

    if (setjmp(thisGen->errorContext) != 0) {
        goto handleError;
    }

    /* Initialize the todoList for use in the "mark" phase of the GC: */

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
    thisGen->gcPhase = GC_PHASE_MARK;
    gcOpts->discoverWeakReferences = CVM_TRUE;

    /*
     * Scan all roots that point to this generation. The root callback is
     * transitive, so 'children' are aggressively processed.
     */
    CVMgenScanAllRoots((CVMGeneration*)thisGen,
		       ee, gcOpts, CVMgenMarkCompactScanTransitively, &tsd);

    CVMthreadSchedHook(CVMexecEnv2threadID(ee));

    /*
     * Don't discover any more weak references.
     */
    gcOpts->discoverWeakReferences = CVM_FALSE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */

    /* NOTE: ProcessSpecial will call ProcessWeakrefWithLivenessInfo(). 
       However, all transitive scans of weakref related objects will be
       done before its FinalizeProcessing().  Hence, we'll be able to
       use CVMweakrefCleanUpForGCAbort() to clean up should we encounter
       an abort error here because we're guaranteed that we won't encounter
       it in FinalizeProcessing() even though it does a transitive scan.
       Its transitive scan will always turn up objects that has already
       been scanned previously thereby requiring no work that can cause
       an abort.
           The processing of interned strings can cause some String objects
       to be marked.  In an abort situation, this is all cleaned up properly
       by the global unmarks() done in the handleError code below.
           The processing of unreferencedMonitors will cause those monitors
       to be released back into the system.  This side-effect is harmless
       and need not be undone.  It's as if we did a partial collection
       where we only collected unreferenced monitors.  Since the collection
       is only done for objects that are no longer live anyway, the object
       cannot be synchronized on.  Hence, it's safe to collect the monitor.
       
    */
    gcOpts->isUpdatingObjectPointers = CVM_FALSE;
    CVMgcProcessSpecialWithLivenessInfo(ee, gcOpts,
					CVMgenMarkCompactRefIsLive, thisGen,
					CVMgenMarkCompactScanTransitively,
					&tsd);
    gcOpts->isUpdatingObjectPointers = CVM_TRUE;

    /* Reset the data structure that preserves original second header
       words. We are done with the TODO stack, so we can re-use extraSpace. */
    thisGen->preservedItems = (CVMMCPreservedItem*)extraSpace->allocBase;
    CVMassert(thisGen->preservedIdx == 0);
    CVMassert(thisGen->preservedSize == (CVMUint32)
	      ((CVMMCPreservedItem*)extraSpace->allocTop - 
	       (CVMMCPreservedItem*)extraSpace->allocBase));
	
    /* The sweep phase. This phase computes the new addresses of each
       object and writes it in the second header word. Evacuated
       original header words will be kept in the 'preservedItems'
       list. */

    /* NOTE: If an abort happens here within the sweep phase, no weakref
       processing is involved.  Weakref constructs are only updated after
       the sweeping action of computing forwarding pointers is completed
       at which point no GC abort can occur.  Hence, no cleanup action is
       required for weakrefs in the sweep phase. 
       The same is true for the processing of interned strings.
    */
    thisGen->gcPhase = GC_PHASE_SWEEP;

    newAllocPtr = sweep(ee, thisGen, gen->allocBase, gen->allocPtr);
    CVMassert(newAllocPtr <= gen->allocPtr);

    /* At this point, the new addresses of each object are written in
       the headers of the old. Update all pointers to refer to the new
       copies. */

    /* Update the roots */
    CVMgenScanAllRoots((CVMGeneration*)thisGen, ee, gcOpts,
		       CVMgenMarkCompactFilteredUpdateRoot, thisGen);
    CVMgcScanSpecial(ee, gcOpts,
		     CVMgenMarkCompactFilteredUpdateRoot, thisGen);

    /* NOTE: CVMgenScanAllRoots() only scan the tree of GC roots.  Unlike in
       the mark phase, the scan is not being done transitively.  Hence, the
       objects in the heap are not scanned as part of this traversal.  So,
       we must scan the heap objects explicitly here: */

    /* Update all interior pointers. */
    scanObjectsInYoungGenRange(thisGen, ee, gcOpts,
			       youngGen->allocBase, youngGen->allocPtr);
    scanObjectsInRangeSkipUnmarked(thisGen,
				   ee, gcOpts, gen->allocBase, gen->allocPtr);

    /* Unmark: Clear/reset marks on the objects in the youngGen: */
    unmark(thisGen, youngGen->allocBase, youngGen->allocPtr);

    /* Compact: Move objects and reset marks in the oldGen: */
    compact(ee, thisGen, gen->allocBase, gen->allocPtr);

    /* Restore the "non-trivial" old header words into the object header words
       which were used for storing forwarding addresses: */
    restorePreservedHeaderWords(thisGen);

    /* Update the allocation pointers: */
    gen->allocPtr = newAllocPtr;
    gen->allocMark = newAllocPtr;

    /*
     * Now since we are not in the young generation, we have to
     * rebuild the barrier structures.  
     */
    CVMgenClearBarrierTable();
    CVMgenMarkCompactRebuildBarrierTable(thisGen, ee, gcOpts,
					 gen->allocBase,
					 gen->allocPtr);

    thisGen->gcPhase = GC_PHASE_RESET;

     /* Determine if we succeed or not based on whether we were able to free up
        a contiguous chunk of memory of the requested size: */
    success = (numBytes/4 <= (CVMUint32)(gen->allocTop - gen->allocPtr));

    CVMtraceGcStartStop(("GC[MC,%d,full]: Done, success for %d bytes %s\n",
			 gen->generationNo, numBytes,
			 success ? "TRUE" : "FALSE"));

    CVMtraceGcCollect(("GC[MC,%d,full]: Ending GC\n", gen->generationNo));

    /*
     * Can we allocate in this space after this GC?
     */
    return success;

handleError:
    /* Undo side-effects of work done before error was detected: */
    if (thisGen->gcPhase == GC_PHASE_MARK) {

        /* Undo the effects of the mark phase: */
        /* Restore all the headerwords in the todoList: */
        while (!todoListIsEmpty(thisGen)) {
            popFromTodo(thisGen);
        }
        /* Removed all the marks from the objects in both generations: */
        unmark(thisGen, youngGen->allocBase, youngGen->allocPtr);
        unmark(thisGen, gen->allocBase, gen->allocPtr);

    } else if (thisGen->gcPhase == GC_PHASE_SWEEP) {

        /* Undo the effects of the sweep phase: */

        /* Reset the data structure that preserves original second header
           words. We are done with the TODO stack, so we can re-use extraSpace. */
        thisGen->preservedItems = (CVMMCPreservedItem*)extraSpace->allocBase;
        thisGen->preservedIdx   = 0;
        thisGen->preservedSize  = (CVMUint32)
            ((CVMMCPreservedItem*)extraSpace->allocTop - 
             (CVMMCPreservedItem*)extraSpace->allocBase);

        unmark(thisGen, youngGen->allocBase, youngGen->allocPtr);
        unsweepAndUnmark(thisGen, gen->allocBase, gen->allocPtr);

#ifdef CVM_DEBUG
    } else {
        CVMpanic("Unknown GC phase!");
#endif
    }

    CVMweakrefCleanUpForGCAbort(ee);

    thisGen->gcPhase = GC_PHASE_RESET;

    return CVM_FALSE;
}
