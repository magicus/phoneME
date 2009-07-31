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
 * This file includes the implementation of a generational collector.
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

#include "javavm/include/gc/generational/generational.h"

#include "javavm/include/gc/generational/gen_semispace.h"
#include "javavm/include/gc/generational/gen_markcompact.h"

#include "javavm/include/porting/memory.h"
#include "javavm/include/porting/threads.h"

#if !CVM_USE_MMAP_APIS

#define CVMmemInit()
#define CVMmemPageSize()    NUM_BYTES_PER_CARD

/* Purpose: Uses calloc() to simulate the reservation of memory. */
static
void *CVMgenMap(size_t requestedSize, size_t *mappedSize)
{
    void *mem;
    void *alignedMem;
    mem = calloc(1, requestedSize + CVMmemPageSize());
    if (mem == NULL) {
        return NULL;
    }
    *mappedSize = requestedSize;

    /* We want to reserve at least the size of a pointer to store the actual
       address of the start of the allocated area.  After that, we will
       realigned the pointer to the next alignment of NUM_BYTES_PER_CARD. 
       Note: the padding in the the allocated size above is to allow room for
       this.
    */
    alignedMem = (CVMUint8 *)mem + sizeof(void *);
    alignedMem = (void *)CVMpackSizeBy((CVMAddr)alignedMem, CVMmemPageSize());

    /* Store the actual calloc'ed pointer just before the start of the "mapped"
       memory: */
    ((void **)alignedMem)[-1] = mem;

    return alignedMem;
}

/* Purpose: Releases the memory that was calloc'ed to simulate the reservation
            of memory.  */
static
void *CVMgenUnmap(void *addr, size_t requestedSize, size_t *unmappedSize)
{
    /* Retrieve the actual calloc'ed pointer from just before the start of the
       "mapped" memory tobe used by free(): */
    void *mem = ((void **)addr)[-1];
    free(mem);
    *unmappedSize = requestedSize;
    return addr;
}


#define CVMmemMap CVMgenMap
#define CVMmemUnmap CVMgenUnmap

#define CVMmemCommit(addr, requestedSize, committedSize) \
    (CVMassert(CVM_FALSE /* Should not be used: CVMmemCommit() */), NULL)

#define CVMmemDecommit(addr, requestedSize, decommittedSize) \
    (CVMassert(CVM_FALSE /* Should not be used: CVMmemDecommit() */), NULL)

#endif /* !CVM_USE_MMAP_APIS */


/*
 * Define to GC on every n-th allocation try
 */
/*#define CVM_GEN_GC_ON_EVERY_ALLOC*/
#define CVM_GEN_NO_ALLOCS_UNTIL_GC 1

#ifdef CVM_JVMPI
/* The number of live objects at the end of the GC cycle: 
   NOTE: This is implemented in this GC by keeping an active count of the
         objects in the heap.  If an object is allocated, the count is
         incremented.  When the object is GC'ed, the count is decremented. */
CVMUint32 liveObjectCount;
#endif

#ifdef CVM_JIT
void** const CVMgcCardTableVirtualBasePtr = 
        (void**)&CVMglobals.gc.cardTableVirtualBase;
#endif


#define roundUpToPage(x, pageSize) CVMpackSizeBy((x), pageSize)
#define roundDownToPage(x, pageSize) \
    CVMpackSizeBy(((CVMAddr)(x) - (pageSize-1)), pageSize)
#define isPageAligned(x, pageSize) \
    ((CVMAddr)(x) == CVMpackSizeBy((CVMAddr)(x), pageSize))

#if CVM_USE_MMAP_APIS
static CVMUint32
CVMgcimplSetSizeAndWatermarks(CVMGCGlobalState *gc, CVMUint32 newSize,
			      CVMUint32 currentUsage);
#endif

/*
 * Initialize GC global state
 */
void
CVMgcimplInitGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("GC: Initializing global state for generational GC\n"));
}

/*
 * %comment: f010
 */
void
CVMgenClearBarrierTable()
{
    memset(CVMglobals.gc.cardTable, CARD_CLEAN_BYTE, CVMglobals.gc.cardTableSize);
    memset(CVMglobals.gc.objectHeaderTable, 0, CVMglobals.gc.cardTableSize);
}

#ifdef CVM_DEBUG_ASSERTS
void CVMgenAssertBarrierTableIsClear()
{
    CVMUint32 i;
    for (i = 0; i < CVMglobals.gc.cardTableSize; i++) {
	CVMassert(CVMglobals.gc.cardTable[i] == CARD_CLEAN_BYTE);
	CVMassert(CVMglobals.gc.objectHeaderTable[i] == 0);
    }
}
#endif /* CVM_DEBUG_ASSERTS */

/*
 * Update the object headers table for objects in range [startRange, endRange)
 */
void
CVMgenBarrierObjectHeadersUpdate(CVMGeneration* gen, CVMExecEnv* ee,
				 CVMGCOptions* gcOpts,
				 CVMUint32* startRange,
				 CVMUint32* endRange)
{
    CVMJavaVal32* curr = (CVMJavaVal32*)startRange;
    CVMtraceGcCollect(("GC[SS,full]: "
		       "object headers update [%x,%x)\n",
		       startRange, endRange));
    while (curr < (CVMJavaVal32*)endRange) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMObject* currObjEnd = (CVMObject*)(curr + objSize / sizeof(CVMJavaVal32));
	CVMUint8*  startCard  =
	    (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(currObj);
	CVMUint8*  endCard    = 
	    (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(currObjEnd);
	/*
	 * I don't need to do anything if object starts on a card boundary
	 * since the object headers table is initialized to 0's. Otherwise
	 * I'd have to make a special case of it here, and check
	 * whether cardBoundary == curr.
	 */

	/* If the object resides entirely within the same card i.e. the
	   startCard is the same as the endCard, then there is nothing to
	   do.  We only need to handle the case where the object spans a
	   card boundary. */
	if (startCard != endCard) {
	    CVMUint8*  card;
	    CVMInt8*   hdr;
	    CVMJavaVal32* cardBoundary;
	    CVMInt32   numCards;

	    /* Object crosses card boundaries. 
	       Find the first card for which the header table entry would
	       be >= 0. It's either startCard in case currObj is at
	       a card boundary, or startCard+1 if currObj is not at a
	       card boundary. */
	    cardBoundary = ROUND_UP_TO_CARD_SIZE(currObj);
	    card = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(cardBoundary);
	    CVMassert((card == startCard) || (card == startCard + 1));
	    if (card == CVMglobals.gc.cardTableEnd) {
		CVMassert(card == startCard + 1);
		return; /* nothing to do -- we are at the edge */
	    }
	    hdr = HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card);
	    CVMassert(hdr >= CVMglobals.gc.objectHeaderTable);
	    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);

	    /* Mark the number of words from this header */
	    CVMassert(cardBoundary >= curr);
	    CVMassert(cardBoundary - curr <= 127);
	    *hdr = (CVMUint8)(cardBoundary - curr);
	    /* Now go through all the rest of the cards that the
	       object spans, and inform them that the start of the
	       object is here */
	    card += 1;
	    hdr  += 1;
	    numCards = -1;
	    if (endCard == CVMglobals.gc.cardTableEnd) {
		endCard--;
	    }
	    if (endCard - card < 127) {
		/* Fast common case, <= 127 iterations. */
		while (card <= endCard) {
		    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);
		    *hdr = numCards; /* Plant back-pointers */
		    numCards--;
		    hdr++;
		    card++;
		}
		CVMassert(numCards >= -128);
	    } else {
		/* Slower rarer case for large objects */
		while (card <= endCard) {
		    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);
		    *hdr = numCards; /* Plant back-pointers */
		    numCards--;
		    if (numCards == -127) {
			numCards = -1; /* Chain reaction! */
		    }
		    hdr++;
		    card++;
		}
	    }
	    CVMassert(card == endCard + 1);
	    CVMassert(HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card) == hdr);
	}
	curr += objSize / sizeof(CVMJavaVal32);
    }
    CVMassert(curr == (CVMJavaVal32*)endRange); /* This had better be exact */
}

/*
 * Scan objects in contiguous range, without doing any special handling.
 *
 * Scan from head of objStart. Scan only within [regionStart,regionEnd).
 * Call out to 'callback' only if the reference found is
 * not in the range [oldGenLower, oldGenHigher) 
 * (i.e. an old to young pointer).
 *
 * Returns what flag the card should contain based on what its scan
 * turned up.
 */
static CVMUint32
scanAndSummarizeObjectsOnCard(CVMExecEnv* ee, CVMGCOptions* gcOpts,
			      CVMJavaVal32* objStart,
			      CVMGenSummaryTableEntry* summEntry,
			      CVMJavaVal32* regionStart, CVMJavaVal32* regionEnd,
			      CVMUint32* oldGenLower,
			      CVMRefCallbackFunc callback, void* callbackData)
{
    CVMUint32  scanStatus = CARD_CLEAN_BYTE;
    CVMUint32  numRefs = 0;
    CVMJavaVal32* curr    = (CVMJavaVal32*)objStart;
    CVMJavaVal32* top     = regionEnd;
    CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(regionStart);
    CVMGeneration *youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMJavaVal32* youngGenStart = (CVMJavaVal32 *)youngGen->heapBase;
    CVMJavaVal32* youngGenEnd = (CVMJavaVal32 *)youngGen->heapTop;

    /* sanity check */
    CVMassert(SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(
	          CARD_TABLE_SLOT_ADDRESS_FOR(regionStart)) == summEntry);
    CVMassert(summEntry >= CVMglobals.gc.summaryTable);
    CVMassert(summEntry <  &(CVMglobals.gc.summaryTable[CVMglobals.gc.cardTableSize]));
    CVMtraceGcCollect(("GC[SS,full]: "
		       "Scanning card range [%x,%x), objStart=0x%x\n",
		       regionStart, regionEnd, objStart));
    /*
     * Clear summary table entry.
     * The summary table is 0xff terminated.
     */
#if (CVM_GENGC_SUMMARY_COUNT == 4)
    summEntry->intVersion = 0xffffffff;
#else
    memset(summEntry->offsets, 0xff, CVM_GENGC_SUMMARY_COUNT);
#endif
    
    /*
     * And scan the objects on the card
     */
    while (curr < top) {
	CVMObject* currObj = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);

	/* Note: No need to mark the card for the class of the object.  All
	   classes are scanned for the youngGen, and the oldGen does a
	   comprehensive root scan which include classes of objects found
	   in the root tree.  Hence, the cardTable need not concern itself
	   with classes.
	 */

	/* Special-case large arrays of refs */
	if ((objSize >= NUM_BYTES_PER_CARD) && 
	    ((CVMcbGcMap(currCb).map & CVM_GCMAP_FLAGS_MASK) == 
	     CVM_GCMAP_ALLREFS_FLAG)) {
	    CVMArrayOfRef* arr = (CVMArrayOfRef*)currObj;
	    CVMObject** arrStart = (CVMObject**)arr->elems;
	    CVMObject** arrEnd   = arrStart + CVMD_arrayGetLength(arr);
	    CVMObject** scanStart;
	    CVMObject** scanEnd;
	    CVMObject** scanPtr;
	    
	    CVMassert(CVMisArrayClass(currCb));

	    /* Get the intersection of the array data and the card
	       region */
	    if (arrStart < (CVMObject**)regionStart) {
		scanStart = (CVMObject**)regionStart;
	    } else {
		scanStart = arrStart;
	    }
	    if (arrEnd > (CVMObject**)regionEnd) {
		scanEnd = (CVMObject**)regionEnd;
	    } else {
		scanEnd = arrEnd;
	    }
	    CVMassert((scanStart <= scanEnd) ||
		      (arrStart >= (CVMObject**)regionEnd));
	    CVMassert(scanEnd <= arrEnd);
	    CVMassert(scanEnd <= (CVMObject**)regionEnd);
	    CVMassert(scanStart >= arrStart);
	    CVMassert(scanStart >= (CVMObject**)cardBoundary);
	    CVMassert(scanStart >= (CVMObject**)regionStart);
	    
	    /* Scan segment [scanStart, scanEnd) 

	       In the rare case where arrStart >= regionEnd and
	       currObj < regionEnd, scanStart becomes larger than
	       scanEnd, at which point this loop does not get
	       executed. */
	    for (scanPtr = scanStart; scanPtr < scanEnd; scanPtr++) {
		CVMJavaVal32* ref = (CVMJavaVal32*) *scanPtr;
                /* Only scan youngGen pointers: */
		if ((ref >= youngGenStart) && (ref < youngGenEnd)) {
		    if (numRefs < CVM_GENGC_SUMMARY_COUNT) {
			summEntry->offsets[numRefs] = (CVMUint8)
			    ((CVMJavaVal32*)scanPtr - cardBoundary);
		    }
		    (*callback)(scanPtr, callbackData);
		    numRefs++;
		}
	    }
	} else CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    CVMJavaVal32* heapRef = *((CVMJavaVal32**)refPtr);
	    /*
	     * Handle the range check here. If the pointer we found
	     * points to the old generation, 
	     * don't do the callback. We are only interested in pointers
	     * to the young generation.
	     *
	     * This one filters out NULL pointers as well.
	     * 
	     * %comment: f011
	     */
            /* Only scan youngGen pointers: */
	    if ((heapRef >= youngGenStart) && (heapRef < youngGenEnd)) {
		if ((CVMJavaVal32*)refPtr >= regionEnd) {
		    /* We ran off the edge of the card */
		    goto returnStatus;
		} else if ((CVMJavaVal32*)refPtr >= regionStart) {
		    /* Call the callback if we are within the bounds */
		    if (numRefs < CVM_GENGC_SUMMARY_COUNT) {
			summEntry->offsets[numRefs] = (CVMUint8)
			    ((CVMJavaVal32*)refPtr - cardBoundary);
		    }
		    numRefs++;
		    (*callback)(refPtr, callbackData);
		}
	    }
	});
	/* iterate */
	curr += objSize / sizeof(CVMJavaVal32);
    }
 returnStatus:
    /* If already marked dirty, don't undo it. Otherwise figure out
       what flag card should contain */
    if (scanStatus != CARD_DIRTY_BYTE) {
	if (numRefs == 0) {
	    scanStatus = CARD_CLEAN_BYTE;
	} else if (numRefs <= CVM_GENGC_SUMMARY_COUNT) {
	    scanStatus = CARD_SUMMARIZED_BYTE;
	} else {
	    scanStatus = CARD_DIRTY_BYTE;
	}
    }
    return scanStatus;
}

/* Purpose: Given the starting address for the heap location that correspond
            to the specified card, find the starting address of the object
	    that occupies the specified heap address. */
static CVMJavaVal32* 
findObjectStart(CVMUint8* card, CVMJavaVal32* heapAddrForCard)
{
    CVMInt8* hdr = HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card);
    CVMInt8  n;

    /* The header table entry must be within the range of the header table: */
    CVMassert(hdr >= CVMglobals.gc.objectHeaderTable);
    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);

    /* The header info from the entry is encoded in the following way:
       1. If the header info is >= 0, then it represents the absolute value of
          the negative offset (in terms of number of words) from the specified
	  heap address to the start of the object that occupies that heap
	  address.  The start address of the object is less that the specified
	  heap address by definition.  Hence, the start address of the object
	  is computed by:
	      obj = heapAddrForCard - n;

       2. If the header info is < 0, then it represents the negative offset to
          the next piece of header info that may contain the offset to the
	  start of the object as in case 1 above.
	     Since the maximum negative offset is -128, if the actual offset
	  to the header info that contains the object header offset is greater
	  than -128, then the info will be expressed in a chain of -128 (or
	  greater) offsets until we come to a positive header info which will
	  be intepreted analogously as is done in case 1 above.
    */
    n = *hdr;
    CVMassert(HEAP_ADDRESS_FOR_CARD(card) == heapAddrForCard);
    if (n >= 0) {
	return heapAddrForCard - n;
    } else {
	CVMJavaVal32* heapAddr = heapAddrForCard;
	do {
	    hdr = hdr + n; /* Go back! */
	    heapAddr = heapAddr + n * NUM_WORDS_PER_CARD;
	    CVMassert(hdr >= CVMglobals.gc.objectHeaderTable);
	    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);
	    n = *hdr;
	} while (n < 0);
	CVMassert(heapAddr < heapAddrForCard);
	return heapAddr - n;
    }
}


/*#define CARD_STATS*/
#ifdef CARD_STATS
typedef struct cardStats {
    CVMUint32 cardsScanned;
    CVMUint32 cardsSummarized;
    CVMUint32 cardsClean;
    CVMUint32 cardsDirty;
} cardStats;

static cardStats cStats = {0, 0, 0, 0};
#define cardStatsOnly(x) x
#else
#define cardStatsOnly(x)
#endif

static void
callbackIfNeeded(CVMExecEnv* ee, CVMGCOptions* gcOpts,
		 CVMUint8* card, 
		 CVMJavaVal32* lowerLimit, CVMJavaVal32* higherLimit, 
		 CVMUint32* genLower, CVMUint32* genHigher,
		 CVMRefCallbackFunc callback,
		 void* callbackData)
{
    CVMassert(card >= CVMglobals.gc.cardTable);
    CVMassert(card < CVMglobals.gc.cardTable + CVMglobals.gc.cardTableSize);
    cardStatsOnly(cStats.cardsScanned++);
    switch (*card) {
    case CARD_SUMMARIZED_BYTE: {
        /*
         * This card has not been dirtied since it was summarized. Scan the
         * summary.
         */
        CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(lowerLimit);
        CVMGenSummaryTableEntry* summEntry =
            SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(card);
        CVMUint32 i;
        CVMBool hasNoCrossGenPointer = CVM_TRUE;
        CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
        CVMObject *youngGenStart = (CVMObject *)youngGen->heapBase;
        CVMObject *youngGenEnd = (CVMObject *)youngGen->heapTop;
        CVMUint8  offset;

        i = 0;
        /* If this card is summarized, it must have at least one entry */
        CVMassert(summEntry->offsets[0] != 0xff);
        while ((i < CVM_GENGC_SUMMARY_COUNT) &&
               ((offset = summEntry->offsets[i]) != 0xff)) {
            CVMObject** refPtr = (CVMObject**)(cardBoundary + offset);
            CVMObject *ref;
            /* We could not have summarized a NULL pointer */
            CVMassert(*refPtr != NULL);
            (*callback)(refPtr, callbackData);
            /* Check to see if we still have a cross generational pointer: */
            ref = *refPtr;
            if ((ref >= youngGenStart) && (ref < youngGenEnd)) {
                hasNoCrossGenPointer = CVM_FALSE;
            }
            i++;
        }
        /* If we didn't encounter any cross generational pointers, then all
           the pointers in this card must either have been nullified, or
           are now referring to objects which have been promoted to the
           oldGen.  Hence, we can declare the card as clean to keep us from
           having to keep scanning it.  If a cross-generational pointer gets
           added later, the card will be marked dirty by the write barrier.
        */
        if (hasNoCrossGenPointer) {
            *card = CARD_CLEAN_BYTE;
        }
        CVMassert(i <= CVM_GENGC_SUMMARY_COUNT);
        cardStatsOnly(cStats.cardsSummarized++);
    }
        break;
    case CARD_DIRTY_BYTE: {
	CVMJavaVal32* objStart;
	CVMGenSummaryTableEntry* summEntry;

	/* Make sure the heap pointer and the card we are scanning are in
           alignment */
	CVMassert(HEAP_ADDRESS_FOR_CARD(card) ==
		  CARD_BOUNDARY_FOR(lowerLimit));
	CVMassert(higherLimit - lowerLimit <= NUM_WORDS_PER_CARD);
	objStart = findObjectStart(card, CARD_BOUNDARY_FOR(lowerLimit));
	summEntry = SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(card);
	*card = scanAndSummarizeObjectsOnCard(ee, gcOpts, objStart,
			      summEntry, lowerLimit, higherLimit, genLower,
			      callback, callbackData);
	cardStatsOnly(cStats.cardsDirty++);
    }
        break;
    case CARD_SENTINEL_BYTE:
        break;
    default:
	CVMassert(*card == CARD_CLEAN_BYTE);
	cardStatsOnly(cStats.cardsClean++);
    }
}

#define SENTINEL_WORD_VALUE \
    ((CARD_SENTINEL_BYTE << 24ul) |    \
     (CARD_SENTINEL_BYTE << 16ul) |    \
     (CARD_SENTINEL_BYTE <<  8ul) |    \
     (CARD_SENTINEL_BYTE <<  0ul))

#define SCAN_CHUNK_SIZE 500000
#define FAST_SCAN_MINIMUM 8

/*
 * Traverse all recorded pointers, and call 'callback' on each.
 */
void
CVMgenBarrierPointersTraverse(CVMGeneration* gen, CVMExecEnv* ee,
			      CVMGCOptions* gcOpts,
			      CVMRefCallbackFunc callback,
			      void* callbackData)
{
    CVMUint8*  lowerCardLimit;    /* Card to begin scanning              */
    CVMUint8*  higherCardLimit;   /* Card to end scanning                */
    CVMUint32* cardPtrWord;       /* Used for batch card scanning        */
    CVMJavaVal32* heapPtr;        /* Track card boundaries in heap       */
    CVMAddr    remainder;         /* Batch scanning spillover            */
    CVMJavaVal32* genLower;       /* Lower limit of live objects in gen  */
    CVMJavaVal32* genHigher;      /* Higher limit of live objects in gen */

    /*
     * Scan cards between lowerCardLimit and higherCardLimit
     *
     * Iterate only over those pointers that belong to the old
     * generation. Stores may have been recorded for the young one
     * as well, but we can ignore those.
     */
    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Scanning barrier records for range [%x,%x)\n",
		       gen->generationNo,
		       gen->allocBase, gen->allocPtr));

    /* We have to cover all pointers in [genLower,genHigher) */
    genLower        = (CVMJavaVal32*)gen->allocBase;
    genHigher       = (CVMJavaVal32*)gen->allocPtr;

    /* ... and look at all the cards in [lowerCardLimit,higherCardLimit) */
    lowerCardLimit  = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(genLower);
    higherCardLimit = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(genHigher);

    if (gen->allocMark != gen->allocPtr) {
	/* If we get here, then we may have allocated some large objects
	   directly from the old generation.  While the card table is
	   up to date, the object header table is not.  So, we need to
	   update that before we proceed with scanning the barrier
	   for this region as well. */
	CVMgenBarrierObjectHeadersUpdate(gen,
            ee, gcOpts, gen->allocMark, gen->allocPtr);
    }

    /* This is the heap boundary corresponding to the next card */
    heapPtr         = NEXT_CARD_BOUNDARY_FOR(genLower);
    
    /*
     * Scan the first card in range, which may be partial.
     */

    /* Check whether the amount of data is really small or not */
    if (genHigher < heapPtr) {
	/* This is really rare. So we might as well check for the
	   condition that genLower == genHigher, i.e. an empty old
	   generation! */
	if (genLower == genHigher) {
	    return;
	}
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 genLower, genHigher,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	return; /* Done, only one partial card! */
    } else {
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 genLower, heapPtr,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	lowerCardLimit++;
    }
    
    /*
     * Have we already reached the end?
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == genHigher) {
	    return; /* nothing to do */
	}
	/*
	 * Make sure this is really the last card.
	 */
	CVMassert(genHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 heapPtr, genHigher,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	return; /* Done, only one partial card! */
    }

    /*
     * How many individual card bytes are we going to look at until
     * we get to an integer boundary?
     */
    remainder =	CVMalignWordUp(lowerCardLimit) - (CVMAddr)lowerCardLimit;
    CVMassert(CARD_BOUNDARY_FOR(genLower) == genLower);
    /*
     * Get lowerCardLimit to a word boundary
     */
    while ((remainder-- > 0) && (lowerCardLimit < higherCardLimit)) {
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	lowerCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Nothing else to do
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == genHigher) {
	    return; /* nothing to do */
	}
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit) == heapPtr);
	CVMassert(genHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, genHigher,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	return;
    }

    /*
     * lowerCardLimit had better be at a word boundary
     */
    CVMassert(CVMalignWordDown(lowerCardLimit) == (CVMAddr)lowerCardLimit);

    /*
     * Now adjust the higher card limit to a word boundary for batch
     * scanning.
     */
    remainder = (CVMAddr)higherCardLimit - CVMalignWordDown(higherCardLimit);
    higherCardLimit -= remainder;
    CVMassert(CVMalignWordDown(higherCardLimit) == (CVMAddr)higherCardLimit);

    /* Need one word for sentinel, but don't bother with chunks for small
       sizes */
    if (higherCardLimit - lowerCardLimit >= 4 + FAST_SCAN_MINIMUM) {
        remainder += 4;
        higherCardLimit -= 4;
    } else {
        remainder += higherCardLimit - lowerCardLimit;
        higherCardLimit = lowerCardLimit;
        goto scan_remainder;
    }


    /*
     * Now go through the card table in blocks of four for faster
     * zero checks.
     */
    for (cardPtrWord = (CVMUint32*)lowerCardLimit;
	 cardPtrWord < (CVMUint32*)higherCardLimit; )
    {
        CVMUint32 *hl = cardPtrWord + SCAN_CHUNK_SIZE;
        CVMUint32 savedWord; 

        if (hl > (CVMUint32*)higherCardLimit) {
            hl = (CVMUint32*)higherCardLimit;
        }

        savedWord = hl[0];
        hl[0] = SENTINEL_WORD_VALUE;

        while (cardPtrWord < hl) {

            CVMUint32 word = *cardPtrWord;

            if (word == FOUR_CLEAN_CARDS) {
                CVMUint32 *start = cardPtrWord;

                /* Since there will tend to be many consecutive clean cards, scan
                   through them quickly in a tight loop: */
                do {
                    cardPtrWord++;
                    word = *cardPtrWord; /* Pre-fetch the next word. */
                    cardStatsOnly(cStats.cardsScanned += 4);
                    cardStatsOnly(cStats.cardsClean += 4);
                } while (word == FOUR_CLEAN_CARDS);

                /* When we get here, we may already have scanned through many cards.
                   Adjust the heapPtr accordingly: */
                heapPtr += (cardPtrWord - start) * (NUM_WORDS_PER_CARD * 4);
            }

            /* If we get here because we encountered a word that has cards that
               need to be scanned, then scan those cards one at a time.  The
               other reason we may be here is because we have reached the end of
               the region we need to scan.  Hence, we need to do a bounds check
               before we scan that presumed cards in that word. */
            {
                CVMJavaVal32* hptr = heapPtr;
                CVMUint8*  cptr = (CVMUint8*)cardPtrWord;
                CVMUint8*  cptr_end = cptr + 4;
                for (; cptr < cptr_end; cptr++, hptr += NUM_WORDS_PER_CARD) {
                    callbackIfNeeded(ee, gcOpts, cptr,
                                     hptr, hptr + NUM_WORDS_PER_CARD,
                                     (CVMUint32*)genLower, (CVMUint32*)genHigher,
                                     callback, callbackData);
                }
                cardPtrWord++;
                heapPtr += NUM_WORDS_PER_CARD * 4;
            }
        }
        if (cardPtrWord > hl) {
            /* Scanned past end to sentinel */
            heapPtr -= NUM_WORDS_PER_CARD * 4;
            --cardPtrWord;
        }
        hl[0] = savedWord;

        CVMthreadSchedHook(CVMexecEnv2threadID(ee));
    }

scan_remainder:

    /*
     * And finally, the remaining few cards, if any, that "spilled" out
     * of the word boundary.
     */
    while (remainder-- > 0) {
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
	higherCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Now if there is a partial card left, handle it.
     */
    if (heapPtr < genHigher) {
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit) == heapPtr);
	CVMassert(genHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, genHigher,
			 (CVMUint32*)genLower, (CVMUint32*)genHigher,
			 callback, callbackData);
    }
}

typedef struct CVMClassScanOptions CVMClassScanOptions;
struct CVMClassScanOptions {
    CVMRefCallbackFunc callback;
    void* callbackData;
};

/* Callback function to scan a class' statics and instance obj. */
static void
CVMgenScanClassPointers(CVMExecEnv* ee, CVMClassBlock* cb, void* opts)
{
    CVMClassScanOptions* options = (CVMClassScanOptions*)opts;
    /* Scan the class state if the java.lang.Class instance: */
    CVMscanClassIfNeeded(ee, cb, options->callback, options->callbackData);
}

/* Find all class instances and scan them for pointers. */
static void
CVMgenScanAllClassPointers(CVMExecEnv* ee, CVMGCOptions* gcOpts,
			   CVMRefCallbackFunc callback,
			   void* callbackData)
{
    CVMClassScanOptions opts;

    opts.callback = callback;
    opts.callbackData = callbackData;
    CVMclassIterateDynamicallyLoadedClasses(
        ee, CVMgenScanClassPointers, &opts);
}

/*
 * Initialize a heap of 'heapSize' bytes. heapSize is guaranteed to be
 * divisible by 4.  */
/* Returns: CVM_TRUE if successful, else CVM_FALSE. */
CVMBool
CVMgcimplInitHeap(CVMGCGlobalState* gc,
		  CVMUint32 startSize,
		  CVMUint32 minSize,
		  CVMUint32 maxSize,
		  CVMBool startIsUnspecified,
		  CVMBool minIsUnspecified,
		  CVMBool maxIsUnspecified)
{
    CVMUint32  heapSize;
    CVMGeneration* youngGen;
    CVMGeneration* oldGen;
    CVMUint32  totBytes; /* Total memory space for all generations */
    CVMUint32  totYoungGenSize; /* Total for both semispaces */
    CVMUint32  roundedSemispaceSize;
    char* youngGenAttr;

    size_t cardTableSize;
    size_t objectHeaderTableSize;
    size_t summaryTableSize;

    /* Note: the packedHeapSize is stored in totBytes. */
    size_t packedCardTableSize;
    size_t packedObjectHeaderTableSize;
    size_t packedSummaryTableSize;

    size_t totalSize;
    size_t actualSize = 0;
    CVMUint8 *heapRegion;
    void *mem;

    size_t pageSize = CVMmemPageSize();

    /* The code below assumes that page size is always an increment of card
       sizes: */
    CVMassert(pageSize == (size_t)ROUND_UP_TO_CARD_SIZE(pageSize));

    /* Heap layout:
         .-----------------------.
	 | Heap area             |
	 | .-------------------. |
	 | | YoungGen          | |
	 | |-------------------| |
	 | | OldGen            | |
	 | |                   | |
	 | `-------------------' |
	 |-----------------------|
	 | CardTable             |
	 |-----------------------|
	 | ObjectHeaderTable     |
	 |-----------------------|
	 | SummaryTable          |
	 `-----------------------'
    */

    /*======================================================================
     * Compute sizes of memory regions:
     */

    /*
     * Let the minimum total heap size be at least 2 pages worth of memory:
     * one for the youngGen and oldGen each.
     */
#define MIN_TOTAL_HEAP (2 * pageSize)

#ifdef CVM_JVMPI
    liveObjectCount = 0;
#endif

    /* Handle the cases where only -Xms is specified: */
    if (!startIsUnspecified && maxIsUnspecified) {
        /* This is for compatibility with legacy -Xms behavior: */
        maxSize = startSize;
    }

    heapSize = maxSize;

    /*
     * Make a sane minimum check on the total size of the heap.
     */
    if (heapSize < MIN_TOTAL_HEAP) {
	CVMdebugPrintf(("GC[generational]: "
                        "Stated heap size %d bytes is smaller than "
			"minimum %d bytes\n",
			heapSize, MIN_TOTAL_HEAP));
	CVMdebugPrintf(("\tTotal heap size now %d bytes\n", MIN_TOTAL_HEAP));
	heapSize = MIN_TOTAL_HEAP;

	/* Since the total size was too small and had to be bumped up to the
	   MIN_TOTAL_HEAP, the min, start, and max must therefore be too small
	   as well and have to be bumped up too: */
	minSize = heapSize;
	maxSize = heapSize;
	startSize = heapSize;
    }

    /* Round up the heap bounds: */
    heapSize = roundUpToPage(heapSize, pageSize);
    gc->heapMaxSize = roundUpToPage(maxSize, pageSize);
#if CVM_USE_MMAP_APIS
    gc->heapMinSize = roundUpToPage(minSize, pageSize);
    gc->heapStartSize = roundUpToPage(startSize, pageSize);
#else
    gc->heapMinSize = gc->heapMaxSize;
    gc->heapStartSize = gc->heapMaxSize;
#endif
    gc->heapCurrentSize = gc->heapStartSize;

    /*
     * Parse relevant attributes
     */
    youngGenAttr = CVMgcGetGCAttributeVal("youngGen");

    if (youngGenAttr == NULL) {
	gc->genGCAttributes.youngGenSize = 1024 * 1024; /* The default size */
    } else {
	CVMInt32 size = CVMoptionToInt32(youngGenAttr);
	if (size < 0) {
	    /* ignore -- the empty string, or no value at all */
	    size = 1024 * 1024;
	}
	gc->genGCAttributes.youngGenSize = size;
    }

    /*
     * At this point, we might find that the total size of the heap is
     * smaller than the stated size of the young generation. Make the
     * heap all young in that case. */

    if (heapSize <= gc->genGCAttributes.youngGenSize) {
	CVMdebugPrintf(("GC[generational]: "
                        "Total heap size %d bytes <= "
			"young gen size of %d bytes\n",
			heapSize, gc->genGCAttributes.youngGenSize));

	/* Leave at least one page for the oldGen: */
	gc->genGCAttributes.youngGenSize = heapSize - pageSize;

	CVMdebugPrintf(("\tYoung generation size now %d bytes\n",
			gc->genGCAttributes.youngGenSize));
    }
    
    /*
     * Now that the user specified sizes of the young generation and
     * total heap have stabilized, do one final sanity check -- a
     * comparison between young gen size and old gen size. If the
     * young gen size is less than 1M, it should not be smaller than,
     * say, around 1/8 the total heap size. If it's too small with
     * regards to the mark-compact generation, the mark-compact
     * algorithm will not have enough scratch space to work with,
     * causing eventual crashes.  
     */
    if ((gc->genGCAttributes.youngGenSize < (1024 * 1024)) &&
	(gc->genGCAttributes.youngGenSize < heapSize / 8)) {
	CVMdebugPrintf(("GC[generational]: "
			"young gen size of %d bytes < 1M\n",
			gc->genGCAttributes.youngGenSize));
	CVMdebugPrintf(("\tYoung generation size now %d bytes\n",
			heapSize / 8));
	gc->genGCAttributes.youngGenSize = heapSize / 8;
    }

    /*
     * %comment f001 
     */
    roundedSemispaceSize = 
	(CVMUint32)ROUND_UP_TO_CARD_SIZE(gc->genGCAttributes.youngGenSize);
    roundedSemispaceSize = roundUpToPage(roundedSemispaceSize, pageSize);

    /* Currently, the youngGen does not resize.  Hence, the start, min, max,
       and current sizes are all the same: */
    if (roundedSemispaceSize >= gc->heapMinSize) {
	/* Leave at least one page for the oldGen: */
	gc->heapMinSize = roundedSemispaceSize + pageSize;
	/* Since we've moved up the min size, we may need to move the start
	   size as well: */
	if (gc->heapStartSize < gc->heapMinSize) {
	    gc->heapStartSize = gc->heapMinSize;
	}
    }
    CVMassert(isPageAligned(roundedSemispaceSize, pageSize));
    gc->youngGenMaxSize = roundedSemispaceSize;
#if CVM_USE_MMAP_APIS
    gc->youngGenMinSize = roundedSemispaceSize;
    gc->youngGenStartSize = roundedSemispaceSize;
#else
    gc->youngGenMinSize = gc->youngGenMaxSize;
    gc->youngGenStartSize = gc->youngGenMaxSize;
#endif
    gc->youngGenCurrentSize = gc->youngGenStartSize;

    totYoungGenSize = roundedSemispaceSize * 2;
    CVMassert(totYoungGenSize ==
	      (CVMUint32)ROUND_UP_TO_CARD_SIZE(totYoungGenSize));

    /* Add the size of one 'wasted' semispace. */
    totBytes = heapSize + roundedSemispaceSize;
    CVMassert(totBytes == (CVMUint32)ROUND_UP_TO_CARD_SIZE(totBytes));
    CVMassert(isPageAligned(totBytes, pageSize));

    /* Compute the oldGen sizes: */
    CVMassert(gc->heapMaxSize > gc->youngGenMaxSize);
    CVMassert(gc->heapMinSize > gc->youngGenMinSize);
    CVMassert(gc->heapStartSize > gc->youngGenStartSize);

    gc->oldGenMaxSize     = gc->heapMaxSize - gc->youngGenMaxSize;
    gc->oldGenMinSize     = gc->heapMinSize - gc->youngGenMinSize;
    gc->oldGenStartSize   = gc->heapStartSize - gc->youngGenStartSize;
    gc->oldGenCurrentSize = gc->oldGenStartSize;

    CVMassert((CVMInt32)gc->oldGenMaxSize > 0);
    CVMassert((CVMInt32)gc->oldGenMinSize > 0);
    CVMassert((CVMInt32)gc->oldGenStartSize > 0);

    CVMassert(isPageAligned(gc->oldGenMaxSize, pageSize));
    CVMassert(isPageAligned(gc->oldGenMinSize, pageSize));
    CVMassert(isPageAligned(gc->oldGenStartSize, pageSize));
    CVMassert(isPageAligned(gc->oldGenCurrentSize, pageSize));

    /* Memory reservation:

       |                              |
       |------------------------------| <-- oldGenCurrentSize
       | ^ ^                          |
       | | | oldGenGrowThreshold      |
       | | v                          |
       |-|----------------------------| <-- oldGenHighWatermark
       | |                            |
       | |   memoryReserve            |
       | v                            |
       |------------------------------| <-- current usage level
       |   ^                          |                          
       |   | oldGenShrinkThreshold    |
       |   v                          |
       |------------------------------| <-- oldGenLowWatermark
       |                              |


       oldGenGrowThreshold (in %) - for computing the high watermark.
       oldGenShrinkThreshold (in %) - for computing the low watermark.

       memoryReserve (in %) - the amount of free memory to make available based
           on a percentage of the current usage level when resizing the heap.
	   The actual new size of the heap will be subject to rounding up to
	   the next page boundary.

       oldGenLowWatermark (in bytes) - after resizing the oldGen, this is
           computed as current usage * ( 100 - oldGenShrinkThreshold) / 100.
	   If the heap usage is observed to drop below the oldGenLowWatermark
	   after an oldGen GC, then the heap will be shrunken.

       oldGenHighWatermark (in bytes) - after resizing the oldGen, this is
           computed as oldGenCurrentSize * (100 - oldGenGrowThreshold) / 100.
	   If the heap usage is observed to exceed the oldGenHighWatermark
	   after an oldGen GC, the the heap will be grown.
    */

    /* Reserve 25% free memory: */
    gc->memoryReserve = 25;

    /* Allow for 10% oscillation in size before resizing: */
    gc->oldGenGrowThreshold = 10;
    gc->oldGenShrinkThreshold = 10;

    /*======================================================================
     * Total up the memory needs for all memory regions:
     */
    totalSize = totBytes; /* for heap regions. */

    /* Compute the space needed for the cardTable:
       NOTE: The cardTable is read using a prefetch that looks ahead by a
       32-bit word.  By always allocating the objectHeader table from the
       contiguous region that follows the cardTable, we guarantee that there
       will be allocated memory to be read from there without triggering a
       fault.  If this were not the case, we would have to pad the size of
       the cardTable with another 32-bit word to avoid this memory fault.
    */
    cardTableSize = totBytes / NUM_BYTES_PER_CARD;
    packedCardTableSize = roundUpToPage(cardTableSize, pageSize);
    CVMassert(cardTableSize * NUM_BYTES_PER_CARD == totBytes);
    totalSize += packedCardTableSize;

    /* Compute the space needed for the objectHeaderTable: */
    objectHeaderTableSize = cardTableSize;
    packedObjectHeaderTableSize = packedCardTableSize;
    totalSize += packedObjectHeaderTableSize;

    /* Compute the space needed for the summaryTable: */
    summaryTableSize = cardTableSize * sizeof(CVMGenSummaryTableEntry);
    packedSummaryTableSize = roundUpToPage(summaryTableSize, pageSize);
    totalSize += packedSummaryTableSize;

    CVMassert(isPageAligned(totalSize, pageSize));

    /* Record the computed sizes of the barrier tables: 
       Note: Currently these values are unused because the barrier tables are
             fixed sized.
    */
    gc->cardTableCurrentSize = packedCardTableSize;
    gc->objectHeaderTableCurrentSize = packedObjectHeaderTableSize;
    gc->summaryTableCurrentSize = packedSummaryTableSize;

    /*======================================================================
     * Reserve/allocate the memory range for the regions:
     */

    /* Reserve the needed space: */
    heapRegion = CVMmemMap(totalSize, &actualSize);
    if (heapRegion == NULL) {
	CVMconsolePrintf("Cannot start VM "
			 "(unable to reserve GC heap memory from OS)\n");
	goto failed;
    }

    CVMassert(actualSize == totalSize);
    gc->mappedTotalSize = actualSize;

    /*======================================================================
     * Assign address of memory regions:
     */

    /* Assign the heap region for the heap generations: */
    CVMassert(heapRegion == (CVMUint8 *)ROUND_UP_TO_CARD_SIZE(heapRegion));
    gc->heapBaseMemoryArea = (CVMUint32 *)heapRegion;
    gc->heapBase = (CVMUint32 *)heapRegion;

    /* Assign the region for the youngGen: */
    gc->youngGenStart = gc->heapBase;
    CVMassert(gc->youngGenStart ==
	      (void *)roundUpToPage((CVMAddr)gc->youngGenStart, pageSize));
    CVMassert(gc->youngGenStart ==
	      (void *)ROUND_UP_TO_CARD_SIZE(gc->youngGenStart));

    /* Assign the region for the oldGen: */
    gc->oldGenStart = gc->youngGenStart + totYoungGenSize / sizeof(CVMUint32);
    CVMassert(gc->oldGenStart ==
	      (void *)roundUpToPage((CVMAddr)gc->oldGenStart, pageSize));
    CVMassert(gc->oldGenStart ==
	      (void *)ROUND_UP_TO_CARD_SIZE(gc->oldGenStart));

    /* Assign the region for the cardTable: */
    gc->cardTable = (CVMUint8 *)gc->heapBase + totBytes;
    gc->cardTableSize = cardTableSize;
    gc->cardTableEnd = gc->cardTable + gc->cardTableSize;
    CVMassert(gc->cardTableSize <= cardTableSize);

    /* Assign the region for the objectHeaderTable: */
    gc->objectHeaderTable = (CVMInt8 *)gc->cardTable + packedCardTableSize;

    /* Assign the region for the summaryTable: */
    gc->summaryTable =
	(CVMGenSummaryTableEntry*)((CVMUint8 *)gc->objectHeaderTable +
				   packedObjectHeaderTableSize);

    /* Computing this virtual origin for the card table makes marking
     * the dirty byte very cheap.
     */
#ifdef CVM_64
    gc->cardTableVirtualBase =
	&(gc->cardTable[-((CVMInt64)(((CVMAddr)gc->heapBase)/NUM_BYTES_PER_CARD))]);
#else
    gc->cardTableVirtualBase =
	&(gc->cardTable[-((CVMInt32)(((CVMAddr)gc->heapBase)/NUM_BYTES_PER_CARD))]);
#endif

#if CVM_USE_MMAP_APIS
    /*======================================================================
     * Commit memory regions:
     */

    /* Commit the youngGen region: */
    mem = CVMmemCommit(gc->youngGenStart,
		       gc->youngGenCurrentSize * 2, &actualSize);
    if (mem != (void *)gc->youngGenStart) {
	CVMassert(mem == NULL);
	CVMassert(actualSize != gc->youngGenCurrentSize * 2);
	CVMconsolePrintf("Cannot start VM "
	    "(unable to commit GC heap memory YG: %d)\n",
	    gc->youngGenCurrentSize * 2);
        goto failed_after_reservedMem;
    }

    /* Commit the oldGen region: */
    mem = CVMmemCommit(gc->oldGenStart, gc->oldGenCurrentSize, &actualSize);
    if (mem != (void *)gc->oldGenStart) {
	CVMassert(mem == NULL);
	CVMassert(actualSize != gc->oldGenCurrentSize);
	CVMconsolePrintf("Cannot start VM "
	    "(unable to commit GC heap memory OG: %d)\n",
	    gc->oldGenCurrentSize);
        goto failed_after_commitYG;
    }

    /* Commit the cardTable region: */
    mem = CVMmemCommit(gc->cardTable, packedCardTableSize, &actualSize);
    if (mem != (void *)gc->cardTable) {
	CVMassert(mem == NULL);
	CVMassert(actualSize != packedCardTableSize);
	CVMconsolePrintf("Cannot start VM "
	    "(unable to commit GC heap memory CT: %d)\n",
	    packedCardTableSize);
        goto failed_after_commitOG;
    }

    /* Commit the objectHeaderTable region: */
    mem = CVMmemCommit(gc->objectHeaderTable,
		       packedObjectHeaderTableSize, &actualSize);
    if (mem != (void *)gc->objectHeaderTable) {
	CVMassert(mem == NULL);
	CVMassert(actualSize != packedObjectHeaderTableSize);
	CVMconsolePrintf("Cannot start VM "
	    "(unable to commit GC heap memory OT: %d)\n",
	    packedObjectHeaderTableSize);
        goto failed_after_commitCT;
    }

    /* Commit the summaryTable region: */
    mem = CVMmemCommit(gc->summaryTable, packedSummaryTableSize, &actualSize);
    if (mem != (void *)gc->summaryTable) {
	CVMassert(mem == NULL);
	CVMassert(actualSize != packedSummaryTableSize);
	CVMconsolePrintf("Cannot start VM "
	    "(unable to commit GC heap memory ST: %d)\n",
	    packedSummaryTableSize);
        goto failed_after_commitOT;
    }
#endif /* CVM_USE_MMAP_APIS */

    /*======================================================================
     * Initialize collectors:
     */

    /* Allocate and initialize the youngGen collector: */
    youngGen = CVMgenSemispaceAlloc(gc->youngGenStart, totYoungGenSize);
    if (youngGen == NULL) {
        goto failed_after_commitST;
    }

    youngGen->generationNo = 0;
    gc->CVMgenGenerations[0] = youngGen;

    CVMglobals.allocPtrPtr = &youngGen->allocPtr;
    CVMglobals.allocTopPtr = &youngGen->allocTop;
    
    /* Allocate and initialize the oldGen collector: */
#if !CVM_USE_MMAP_APIS
    CVMassert(gc->oldGenCurrentSize == gc->oldGenMaxSize);
#endif
    oldGen = CVMgenMarkCompactAlloc(gc->oldGenStart, gc->oldGenCurrentSize);
    if (oldGen == NULL) {
        goto failed_after_youngGen;
    }

    oldGen->generationNo = 1;
    gc->CVMgenGenerations[1] = oldGen;

    /* Relate the generations: */
    youngGen->nextGen = oldGen;
    youngGen->prevGen = NULL;

    oldGen->nextGen = NULL;
    oldGen->prevGen = youngGen;

    /*
     * Initialize the barrier
     */
#ifdef CVM_DEBUG_ASSERTS
    CVMgenAssertBarrierTableIsClear();
#endif

#if CVM_USE_MMAP_APIS
    /* Compute and set the size, and low and high watermarks: 
       Note: the currentUsage of the oldGen heap is 0 because we've only just
       initialized it.  We assert here to make sure that that is true.
    */
    CVMassert(((oldGen->allocPtr - oldGen->allocBase)*sizeof(CVMUint32)) == 0);
    gc->oldGenCurrentSize =
	CVMgcimplSetSizeAndWatermarks(gc, gc->oldGenCurrentSize, 0);
#endif

    /*
     * Initialize GC times. Let heap initialization be the first
     * major GC.
     */
    gc->lastMajorGCTime = CVMtimeMillis();

#ifdef CVM_JVMPI
    /* Report the arena info: */
    CVMgcimplPostJVMPIArenaNewEvent();
#endif

#if defined(CVM_DEBUG)
    CVMgenDumpSysInfo(gc);
#endif /* CVM_DEBUG || CVM_INSPECTOR */

    CVMtraceMisc(("GC: Initialized heap for generational GC\n"));
    return CVM_TRUE;

failed_after_youngGen:
    CVMgenSemispaceFree((CVMGenSemispaceGeneration*)youngGen);

failed_after_commitST:
#if CVM_USE_MMAP_APIS
    mem = CVMmemDecommit(gc->summaryTable, packedSummaryTableSize, &actualSize);
    CVMassert(mem == (void *)gc->summaryTable);
    CVMassert(actualSize == packedSummaryTableSize);
failed_after_commitOT:
    mem = CVMmemDecommit(gc->objectHeaderTable,
			 packedObjectHeaderTableSize, &actualSize);
    CVMassert(mem == (void *)gc->objectHeaderTable);
    CVMassert(actualSize == packedObjectHeaderTableSize);
failed_after_commitCT:
    mem = CVMmemDecommit(gc->cardTable, packedCardTableSize, &actualSize);
    CVMassert(mem == (void *)gc->cardTable);
    CVMassert(actualSize == packedCardTableSize);
failed_after_commitOG:
    mem = CVMmemDecommit(gc->oldGenStart, gc->oldGenCurrentSize, &actualSize);
    CVMassert(mem == (void *)gc->oldGenStart);
    CVMassert(actualSize == gc->oldGenCurrentSize);
failed_after_commitYG:
    mem = CVMmemDecommit(gc->youngGenStart,
		         gc->youngGenCurrentSize * 2, &actualSize);
    CVMassert(mem == (void *)gc->youngGenStart);
    CVMassert(actualSize == gc->youngGenCurrentSize * 2);
failed_after_reservedMem:
#endif /* CVM_USE_MMAP_APIS */

    mem = CVMmemUnmap(heapRegion, totalSize, &actualSize);
    CVMassert(mem == heapRegion);
    CVMassert(actualSize == totalSize);
failed:
    /* The caller will signal heap initialization failure */
    return CVM_FALSE;
}

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the generational GC (in addition to
   the semispace and markcompact dumps). */
void CVMgenDumpSysInfo(CVMGCGlobalState* gc)
{
    CVMconsolePrintf("GC[generational]: Sizes\n"
		     "\tyoungGen = min %d start %d max %d\n"
		     "\toldGen   = min %d start %d max %d\n"
		     "\toverall  = min %d start %d max %d\n",
		     gc->youngGenMinSize, gc->youngGenStartSize,
		     gc->youngGenMaxSize,
		     gc->oldGenMinSize, gc->oldGenStartSize, gc->oldGenMaxSize,
		     gc->heapMinSize, gc->heapStartSize, gc->heapMaxSize);

    CVMconsolePrintf("GC[generational]: Auxiliary data structures\n");
    CVMconsolePrintf("\theapBaseMemoryArea=[0x%x,0x%x)\n",
		     gc->heapBaseMemoryArea, gc->cardTable);
    CVMconsolePrintf("\tcardTable=[0x%x,0x%x)\n",
		     gc->cardTable, gc->cardTable + gc->cardTableSize);
    CVMconsolePrintf("\tobjectHeaderTable=[0x%x,0x%x)\n",
		     gc->objectHeaderTable,
		     gc->objectHeaderTable + gc->cardTableSize);
    CVMconsolePrintf("\tsummaryTable=[0x%x,0x%x)\n",
		     gc->summaryTable, gc->summaryTable + gc->cardTableSize);
}
#endif /* CVM_DEBUG || CVM_INSPECTOR */


#ifdef CVM_JVMPI
/* Purpose: Posts the JVMPI_EVENT_ARENA_NEW events for the GC specific
            implementation. */
/* NOTE: This function is necessary to compensate for the fact that
         this event has already transpired by the time that JVMPI is
         initialized. */
void CVMgcimplPostJVMPIArenaNewEvent(void)
{
    if (CVMjvmpiEventArenaNewIsEnabled()) {
        CVMUint32 arenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaNewEvent(arenaID+1, "YoungGen");
        CVMjvmpiPostArenaNewEvent(arenaID+2, "OldGen");
    }
}

/* Purpose: Posts the JVMPI_EVENT_ARENA_DELETE events for the GC specific
            implementation. */
void CVMgcimplPostJVMPIArenaDeleteEvent(void)
{
    if (CVMjvmpiEventArenaDeleteIsEnabled()) {
        CVMUint32 arenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaDeleteEvent(arenaID+2);
        CVMjvmpiPostArenaDeleteEvent(arenaID+1);
    }
}

/* Purpose: Gets the arena ID for the specified object. */
/* Returns: The requested ArenaID if found, else returns
            CVM_GC_ARENA_UNKNOWN. */
CVMUint32 CVMgcimplGetArenaID(CVMObject *obj)
{
    CVMUint32 arenaID = CVMgcGetLastSharedArenaID() + 1;
    int i;
    for (i = 0; i < CVM_GEN_NUM_GENERATIONS; i++) {
        CVMGeneration *gen = CVMglobals.gc.CVMgenGenerations[i];
        if (CVMgenInGeneration(gen, obj)) {
            return arenaID + i;
        }
    }
    return CVM_GC_ARENA_UNKNOWN;
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
    CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[1];
    return youngGen->freeMemory(youngGen, ee) +
	oldGen->freeMemory(oldGen, ee);
}

/*
 * Return the amount of total memory in the heap, in bytes. Take into
 * account one semispace size only, since that is the useful amount.
 */
CVMUint32
CVMgcimplTotalMemory(CVMExecEnv* ee)
{
    CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[1];
    return youngGen->totalMemory(youngGen, ee) +
	oldGen->totalMemory(oldGen, ee);
}

/*
 * Some generational service routines
 */

/*
 * Scan the root set of collection, as well as all pointers from other
 * generations
 */
void
CVMgenScanAllRoots(CVMGeneration* thisGen,
		   CVMExecEnv *ee, CVMGCOptions* gcOpts,
		   CVMRefCallbackFunc callback, void* data)
{
    CVMtraceGcCollect(("GC[SS,%d,full]: "
		       "Scanning roots of generation\n",
		       thisGen->generationNo));
    /*
     * Clear the class marks if we are going to scan classes by
     * reachability.  
     */
    CVMgcClearClassMarks(ee, gcOpts);

    /*
     * First scan pointers to this generation from others, depending
     * on whether we are scanning roots for the young or the old
     * generation.
     */
    if (thisGen->generationNo == 0) {
	/* Collecting young generation */

	/* First scan older to younger pointers */
	CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[1];

	/*
	 * Scan older to younger pointers up until 'allocMark'.
	 * These are pointers for which the object header table
	 * has already been updated in the last old gen GC.
	 */
	oldGen->scanOlderToYoungerPointers(oldGen, ee,
					   gcOpts, callback, data);

#ifdef BREADTH_FIRST_SCAN
	/*
	 * Get any allocated pointers in the old generation for which
	 * object headers and card tables have not been updated yet.
	 *
	 * This is going to happen the first time CVMgenScanAllRoots()
	 * is called. The next time, scanOlderToYoungerPointers will
	 * pick all old->young pointers up.  
	 */
	if (oldGen->allocPtr > oldGen->allocMark) {
	    oldGen->scanPromotedPointers(oldGen, ee, gcOpts, callback, data);
	    CVMassert(oldGen->allocPtr == oldGen->allocMark);
	}
#endif
        /* And scan all classes.  In practice, even if we do a true
	   reachability scan to find classes, we will tend to reach all
	   classes anyway.  So, scanning all classes actually simplifies
	   things because we no longer need to track reachability of
	   classes in the youngGen root scan and the card tables.
	*/
        CVMgenScanAllClassPointers(ee, gcOpts, callback, data);

    } else {
	/* Collecting full heap: */

	/* For now, assume one or two generations */
	CVMassert(thisGen->generationNo == 1);
    }
    /*
     * And now go through "system" pointers to this generation.
     */
    CVMgcScanRoots(ee, gcOpts, callback, data);
}

#if CVM_USE_MMAP_APIS

/* Purpose: Sets the size of the region as well as the watermarks. */
static CVMUint32
CVMgcimplSetSizeAndWatermarks(CVMGCGlobalState *gc, CVMUint32 newSize,
			      CVMUint32 currentUsage)
{
    CVMGeneration *oldGen = gc->CVMgenGenerations[1];
    CVMUint8 *allocBase = (CVMUint8 *)oldGen->allocBase;
    CVMInt32 newLow = 0;
    CVMInt32 newHigh = 0;

    size_t pageSize = CVMmemPageSize();

    if (newSize <= gc->oldGenMinSize) {
	/* Cannot shrink below minimum: */
	newSize = gc->oldGenMinSize;
	/* Once size is at minimum, set the low watermark so low that we don't
           have to check if we need to shrink.  By definition, we will never
           need to shrink because we are already at the minimum even if the
	   usage level drops below the minimum: */
	newLow = 0;

    } else {
	CVMInt32 threshold;
	CVMJavaDouble dNewSize;
	CVMJavaDouble dThreshold;
	CVMJavaDouble d100;
	CVMJavaDouble dResult;

	CVMassert(gc->oldGenShrinkThreshold <= 100);

	/* Note: Need to do this calculation using floating point to prevent
	   integer overflow errors.  Since this calculation is rarely done,
	   there is no performance penalty for using floating point math:
	      threshold = (newSize * gc->oldGenShrinkThreshold) / 100; 
	*/
	dNewSize = CVMint2Double(newSize);
	dThreshold = CVMint2Double(gc->oldGenShrinkThreshold);
	d100 = CVMint2Double(100);
	dResult = CVMdoubleMul(dNewSize, dThreshold);
	dResult = CVMdoubleDiv(dResult, d100);
	threshold = CVMdouble2Int(dResult);

	if (threshold < pageSize) {
	    threshold = pageSize;
	}
	newLow = currentUsage - threshold;
	if (newLow < 0) {
	    newLow = 0;
	}
	newLow = roundDownToPage(newLow, pageSize);
	if (newLow < gc->oldGenMinSize) {
	    newLow = gc->oldGenMinSize;
	}
    }

    /* Compute the new high watermark: */
    if (newSize >= gc->oldGenMaxSize) {
	/* Cannot grow above maximum: */
	newSize = gc->oldGenMaxSize;
	/* Skip high threshold checks if we're already at max capacity: */
	newHigh = gc->oldGenMaxSize;

    } else {
	CVMInt32 threshold;
	CVMJavaDouble dNewSize;
	CVMJavaDouble dThreshold;
	CVMJavaDouble d100;
	CVMJavaDouble dResult;

	CVMassert(gc->oldGenGrowThreshold <= 100);

	/* Note: Need to do this calculation using floating point to prevent
	   integer overflow errors.  Since this calculation is rarely done,
	   there is no performance penalty for using floating point math:
	      threshold = (newSize * gc->oldGenGrowThreshold) / 100; 
	*/
	dNewSize = CVMint2Double(newSize);
	dThreshold = CVMint2Double(gc->oldGenGrowThreshold);
	d100 = CVMint2Double(100);
	dResult = CVMdoubleMul(dNewSize, dThreshold);
	dResult = CVMdoubleDiv(dResult, d100);
	threshold = CVMdouble2Int(dResult);

	if (threshold < pageSize) {
	    threshold = pageSize;
	}
	newHigh = newSize - threshold;
	if (newHigh < 0) {
	    newHigh = 0;
	}
	newHigh = roundUpToPage(newHigh, pageSize);
	CVMassert(newHigh <= gc->oldGenMaxSize);
    }

    /* Publish the new sizes: */
    gc->oldGenCurrentSize = newSize;
    gc->oldGenLowWatermark = newLow;
    gc->oldGenHighWatermark = newHigh;

    oldGen->heapTop = (CVMUint32 *)(allocBase + newSize);
    oldGen->allocTop = (CVMUint32 *)(allocBase + newSize);
    CVMassert(oldGen->allocPtr <= oldGen->allocTop);
    CVMassert(oldGen->allocMark <= oldGen->allocTop);

    return newSize;
}

/* Purpose: Resizes the heap. */
static CVMBool
CVMgcimplResize(CVMExecEnv *ee, CVMUint32 numBytes, CVMBool grow)
{
    CVMGCGlobalState *gc = &CVMglobals.gc;
    CVMGeneration *oldGen = gc->CVMgenGenerations[1];
    CVMUint8 *allocBase = (CVMUint8 *)oldGen->allocBase;
    CVMUint8 *allocPtr = (CVMUint8 *)oldGen->allocPtr;
    CVMUint32 currentUsage = allocPtr - allocBase;
    CVMUint32 oldSize = gc->oldGenCurrentSize;

    CVMUint32 newSize;
    size_t pageSize = CVMmemPageSize();

    CVMBool success = CVM_TRUE;

    CVMJavaDouble dNewSize;
    CVMJavaDouble dTemp;
    CVMJavaDouble d100;
    CVMJavaDouble dUsage;

    /* Compute the new Size: 

       currentUsage     100 - gc->memoryReserve
       ------------  =  -----------------------
         newSize                 100

       And in addition to the above ratio, we add numBytes which is expected to
       be consumed immediately after this for youngGen to oldGen promotion.
       Adding this estimate allows us to avoid having the need to grow again
       due to promotion filling up the oldGen immediately after a growth cycle.
    */
    CVMassert(100 >= gc->memoryReserve);

    /* Note: Need to do this calculation using floating point to prevent
       integer overflow errors.  Since this calculation is rarely done,
       there is no performance penalty for using floating point math:
          newSize = (currentUsage * 100) / (100 - gc->memoryReserve);
    */
    dUsage = CVMint2Double(currentUsage);
    d100 = CVMint2Double(100);
    dTemp = CVMint2Double(100 - gc->memoryReserve);
    dNewSize = CVMdoubleMul(dUsage, d100);
    dNewSize = CVMdoubleDiv(dNewSize, dTemp);
    newSize = CVMdouble2Int(dNewSize);

    newSize += numBytes;
    newSize = roundUpToPage(newSize, pageSize);
    if (newSize > gc->oldGenMaxSize) {
	newSize = gc->oldGenMaxSize;
    }
    if (newSize < gc->oldGenMinSize) {
	newSize = gc->oldGenMinSize;
    }

    /* Resize the new memory: */
    if (grow) {
	/* Commit the new memory: */
	CVMUint8 *commitStart;
	CVMInt32 commitSize;
	CVMUint8 *cardAreaStart;
	CVMUint32 cardAreaSize;

	commitStart = allocBase + oldSize;
	CVMassert(isPageAligned(commitStart, pageSize));
	commitSize = newSize - oldSize;

	if (commitSize > 0) {
	    CVMUint8 *mem;
	    size_t actualSize;
	    CVMassert(isPageAligned(commitSize, pageSize));
	    mem = CVMmemCommit(commitStart, commitSize, &actualSize);
	    if (mem == NULL) {
		/* It's possible to fail is the OS fails to provide physical
		   memory to make the commit.  This will just result in an
		   OutOfMemoryError being thrown. */
		success = CVM_FALSE;

		goto failed;
	    }

	    CVMassert(mem == commitStart);
	    CVMassert(actualSize == commitSize);

            /* set new size and watermarks */
            CVMgcimplSetSizeAndWatermarks(gc, newSize, currentUsage);

	    /* Initialize the corresponding card table region: */
	    cardAreaStart = (void *)CARD_TABLE_SLOT_ADDRESS_FOR(commitStart);
	    cardAreaSize = commitSize / NUM_BYTES_PER_CARD;
	    CVMassert((void *)HEAP_ADDRESS_FOR_CARD(cardAreaStart) == commitStart);
	    CVMassert(cardAreaSize * NUM_BYTES_PER_CARD == commitSize);
	    memset(cardAreaStart, CARD_CLEAN_BYTE, cardAreaSize);
	}

    } else {
	/* Decommit the new memory: */
	CVMUint8 *decommitStart;
	CVMInt32 decommitSize;

	decommitStart = allocBase + newSize;
	CVMassert(isPageAligned(decommitStart, pageSize));
	decommitSize = oldSize - newSize;

	if (decommitSize > 0) {
	    CVMUint8 *mem;
	    size_t actualSize;
	    CVMassert(isPageAligned(decommitSize, pageSize));
	    mem = CVMmemDecommit(decommitStart, decommitSize, &actualSize);
	    CVMassert(mem == decommitStart);
	    CVMassert(actualSize == decommitSize);
            /* set new size and watermarks */
            CVMgcimplSetSizeAndWatermarks(gc, newSize, currentUsage);
	}
    }

#if 0 /* For debugging the resizing operation only. */
    /* Dump the resizing data: */
#define SIZES(x) (x) /*bytes*/ , ((x)/1024.0) /*KB*/, ((x)/1024.0/1024.0)/*MB*/
#define SIZEDIFF (grow?((CVMInt32)newSize-(CVMInt32)oldSize):((CVMInt32)oldSize-(CVMInt32)newSize))
    CVMconsolePrintf("===========================================\n");
    CVMconsolePrintf("Resizing to %s oldGen (numBytes %d) by %d (%.1fk:%.1fM)\n",
		     grow? "GROW":"SHRINK", numBytes, SIZES(SIZEDIFF));
    CVMconsolePrintf("     min     = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenMinSize));
    CVMconsolePrintf("     start   = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenStartSize));
    CVMconsolePrintf("     max     = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenMaxSize));
    CVMconsolePrintf("     current = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenCurrentSize));
    CVMconsolePrintf("     LowWatermark  = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenLowWatermark));
    CVMconsolePrintf("     HighWatermark = %d (%.1fk:%.1fM)\n", SIZES(gc->oldGenHighWatermark));
    CVMconsolePrintf("     used          = %d (%.1fk:%.1fM)\n", SIZES(allocPtr-allocBase));
    CVMconsolePrintf("===========================================\n");
#undef SIZES
#undef SIZEDIFF
#endif

failed:
    return success;
}

#endif /* CVM_USE_MMAP_APIS */

/*
 * This routine is called by the common GC code after all locks are
 * obtained, and threads are stopped at GC-safe points. It's the
 * routine that needs a snapshot of the world while all threads are
 * stopped (typically at least a root scan).
 *
 * GC enough to satisfy allocation of 'numBytes' bytes.
 */

void
CVMgcimplDoGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMGCOptions gcOpts;
    CVMBool success;
    CVMGeneration* oldGen;
    CVMGeneration* youngGen;

#if CVM_USE_MMAP_APIS
    CVMBool haveAttemptedResize = CVM_FALSE;
#endif

/*#define SOLARIS_TIMING*/
#ifdef SOLARIS_TIMING
#include <sys/time.h>
    static CVMUint32 totGCTime = 0;
    static CVMUint32 totGCCount = 0;
    hrtime_t time;
    CVMUint32 ms;

    time = gethrtime();
#endif /* SOLARIS_TIMING */

    gcOpts.isUpdatingObjectPointers = CVM_TRUE;
    gcOpts.discoverWeakReferences = CVM_FALSE;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    gcOpts.isProfilingPass = CVM_FALSE;
#endif

    /*
     * Do generation-specific initialization
     */
    youngGen = CVMglobals.gc.CVMgenGenerations[0];
    oldGen = CVMglobals.gc.CVMgenGenerations[1];

#if CVM_USE_MMAP_APIS
retryGC:
#endif
    /* If we are interned strings detected in the youngGen heap during the
       last GC or there are newly created interned strings, then we need to
       scan the interned strings table because there may still be interned
       string objects in the youngGen heap that could get collected or
       moved. */
    CVMglobals.gc.needToScanInternedStrings =
        (CVMglobals.gc.hasYoungGenInternedStrings ||
         CVMglobals.gcCommon.stringInternedSinceLastGC);
    CVMglobals.gc.hasYoungGenInternedStrings = CVM_FALSE;

    CVMglobals.gcCommon.doClassCleanup =
        (CVMglobals.gc.hasYoungGenClassesOrLoaders ||
         CVMglobals.gcCommon.classCreatedSinceLastGC ||
         CVMglobals.gcCommon.loaderCreatedSinceLastGC);

    CVMglobals.gc.hasYoungGenClassesOrLoaders = CVM_FALSE;

    /* Do a youngGen collection and see if it is adequate to satisfy this
       GC request: */
    success = youngGen->collect(youngGen, ee, numBytes, &gcOpts);

    /* Try a full heap GC if we can't satisfy the request from the youngGen:
    */
    if (!success) {
        /* For a full GC, classes can be unloaded.  Hence, we definitely need
           to scan for class-unloading activity: */
        CVMglobals.gcCommon.doClassCleanup = CVM_TRUE;

        /* For a full GC, the oldGen may have interned strings that can get
           collected.  Hence, we definitely need to scan the interned strings
           table: */
        CVMglobals.gc.needToScanInternedStrings = CVM_TRUE;

	oldGen->collect(oldGen, ee, numBytes, &gcOpts);
	CVMglobals.gc.lastMajorGCTime = CVMtimeMillis();
    }

#if CVM_USE_MMAP_APIS
    /* Resize heap if necessary: */
    if (!haveAttemptedResize)
    {
	CVMBool gcFailed;
	CVMGCGlobalState *gc = &CVMglobals.gc;
	CVMUint32 oldGenUsedSize;

	haveAttemptedResize = CVM_TRUE;
	oldGenUsedSize =
	    oldGen->totalMemory(oldGen, ee) - oldGen->freeMemory(oldGen, ee);

	/* We consider the GC failed if a full GC wasn't requested but we still
	   failed to satisfy the requested memory: */
	gcFailed = !success && (numBytes != (CVMUint32)(-1));

	/* If we can't satisfy the request or don't have enough free memory in
	   reserve, then maybe we need to grow the oldGen: */
	if (gcFailed || oldGenUsedSize > gc->oldGenHighWatermark) {
	    /* Only attempt to grow the heap if there is still room to grow: */
	    if (gc->oldGenCurrentSize < gc->oldGenMaxSize) {
		CVMUint32 additionalBytesToGrow = 0;
		CVMBool resized;
		if (gcFailed) {
		    /* Conservatively estimate that the entire youngGen content
		       will be promoted: */
		    CVMUint8 *allocPtr = (CVMUint8 *)youngGen->allocPtr;
		    CVMUint8 *allocBase = (CVMUint8*)youngGen->allocBase;
		    additionalBytesToGrow = allocPtr - allocBase;
		    if (numBytes != (CVMUint32)-1) {
			additionalBytesToGrow += numBytes;
		    }
		}
	        resized = CVMgcimplResize(ee, additionalBytesToGrow, CVM_TRUE);
		/* Now that we've grown the heap, we should re-attempt the
		   youngGen GC if we failed to produce the requested memory: */
		if (resized && gcFailed) {
		    goto retryGC;
		}
	    }

	/* If have too much free memory in reserve, then maybe we need to
	   shrink the oldGen: */
	} else if (oldGenUsedSize < gc->oldGenLowWatermark) {
	    /* Only shrink the heap if there is room to shrink: */
	    if (gc->oldGenCurrentSize > gc->oldGenMinSize) {
		CVMgcimplResize(ee, 0, CVM_FALSE);
	    }
	}
    }

#endif /* CVM_USE_MMAP_APIS */

#ifdef SOLARIS_TIMING
    time = gethrtime() - time;
    ms = time / 1000000;
    totGCCount++;
    totGCTime += ms;
    CVMconsolePrintf("GC took %d ms (%d ms total, %d ms average, %d count)\n",
                     ms, totGCTime, (totGCTime/totGCCount), totGCCount);
#endif
    cardStatsOnly({
	CVMconsolePrintf("CARDS: scanned=%d clean=%d (%d%%) "
			 "dirty=%d (%d%%) summarized=%d (%d%%)\n",
			 cStats.cardsScanned,
			 cStats.cardsClean,
			 cStats.cardsClean * 100 / cStats.cardsScanned,
			 cStats.cardsDirty,
			 cStats.cardsDirty * 100 / cStats.cardsScanned,
			 cStats.cardsSummarized,
			 cStats.cardsSummarized * 100 /
			 cStats.cardsScanned);
	memset(&cStats, 0, sizeof(cStats));
    });
}

CVMObject*
CVMgcimplRetryAllocationAfterGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[1];
    CVMObject* allocatedObj;

    /*
     * Re-try in the young generation.
     */
    CVMgenContiguousSpaceAllocate(youngGen, numBytes, allocatedObj);

    if (allocatedObj == NULL) {
	/* Try hard if we've already GC'ed! */
	CVMgenContiguousSpaceAllocate(oldGen, numBytes, allocatedObj);
    }
    return allocatedObj;
}

/*
 * Allocate uninitialized heap object of size numBytes
 */
CVMObject*
CVMgcimplAllocObject(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMObject* allocatedObj;
    CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[1];
#define LARGE_OBJECTS_TREATMENT
/*#define LARGE_OBJECT_STATS*/
#ifdef LARGE_OBJECT_STATS
    static CVMUint32
	largeAllocations = 0,
	smallAllocations = 0,
	totalAllocations = 0;
#endif

#ifdef CVM_GEN_GC_ON_EVERY_ALLOC
    static CVMUint32 allocCount = 1;
    if ((allocCount % CVM_GEN_NO_ALLOCS_UNTIL_GC) == 0) {
	/* Do one for stress-test. If it was unsuccessful, try again in the
	   next allocation */
	if (CVMgcStopTheWorldAndGC(ee, numBytes)) {
	    allocCount = 1;
	}
    } else {
	allocCount++;
    }
#endif
#ifdef LARGE_OBJECTS_TREATMENT
#define LARGE_OBJECT_THRESHOLD 50000
    if (numBytes > LARGE_OBJECT_THRESHOLD) {
	CVMgenContiguousSpaceAllocate(oldGen, numBytes, allocatedObj);
    } else {
	CVMgenContiguousSpaceAllocate(youngGen, numBytes, allocatedObj);
    }
#ifdef LARGE_OBJECT_STATS
    if (numBytes > LARGE_OBJECT_THRESHOLD) {
	largeAllocations++;
    } else {
	smallAllocations++;
    }
    totalAllocations++;
    if (totalAllocations == 100000) {
	CVMconsolePrintf("ALLOCATIONS: "
			 "tot=%d, small=%d, large=%d, %% large=%d\n",
			 totalAllocations, smallAllocations, largeAllocations,
			 largeAllocations * 100 / totalAllocations);
	totalAllocations = largeAllocations = smallAllocations = 0;
    }
#endif
#else
    /*
     * No special treatment for large objects. Always try to allocate
     * from young space first.
     */
    CVMgenContiguousSpaceAllocate(youngGen, numBytes, allocatedObj);
#endif

#ifdef CVM_JVMPI
    if (allocatedObj != NULL) {
        liveObjectCount++;
    }
#endif
    return allocatedObj;
}

/*
 * Destroy GC global state
 */
void
CVMgcimplDestroyGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying global state for generational GC\n"));
}

/*
 * Destroy heap
 */
CVMBool
CVMgcimplDestroyHeap(CVMGCGlobalState* gc)
{
    size_t actualSize;
    void *mem;

    CVMtraceMisc(("Destroying heap for generational GC\n"));

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaDeleteEvent();
#endif

    mem = CVMmemUnmap(gc->heapBaseMemoryArea, gc->mappedTotalSize, &actualSize);
    CVMassert(mem == gc->heapBaseMemoryArea);
    CVMassert(actualSize == gc->mappedTotalSize);

    gc->heapBaseMemoryArea = NULL;
    gc->cardTable = NULL;
    gc->objectHeaderTable = NULL;
    gc->summaryTable = NULL;

    CVMgenSemispaceFree((CVMGenSemispaceGeneration*)gc->CVMgenGenerations[0]);
    CVMgenMarkCompactFree((CVMGenMarkCompactGeneration*)gc->CVMgenGenerations[1]);
	
    gc->CVMgenGenerations[0] = NULL;
    gc->CVMgenGenerations[1] = NULL;

    return CVM_TRUE;
}

/*
 * JVM_MaxObjectInspectionAge() support
 */
CVMInt64
CVMgcimplTimeOfLastMajorGC()
{
    return CVMglobals.gc.lastMajorGCTime;
}

typedef struct CallbackInfo CallbackInfo;
struct CallbackInfo
{
    CVMObjectCallbackFunc callback;
    void* callbackData;
};

/* Purpose: Call back to the callback function if the object is not
            synthesized.  Filter out synthesized objects. */
static CVMBool
CVMgcCallBackIfNotSynthesized(CVMObject *obj, CVMClassBlock *cb,
                              CVMUint32 size, void *data)
{
    CallbackInfo *info = (CallbackInfo *)data;
    if (!CVMGenObjectIsSynthesized(obj)) {
        return info->callback(obj, cb, size, info->callbackData);
    }
    return CVM_TRUE;
}

/*
 * Heap iteration. Call (*callback)() on each object in the heap.
 */
/* Returns: CVM_TRUE if the scan was done completely.  CVM_FALSE if aborted
            before scan is complete. */
CVMBool
CVMgcimplIterateHeap(CVMExecEnv* ee, 
		     CVMObjectCallbackFunc callback, void* callbackData)
{
    int i;
    CallbackInfo info;

    info.callback = callback;
    info.callbackData = callbackData;

    /*
     * Iterate over objects in all generations
     */
    for (i = 0; i < CVM_GEN_NUM_GENERATIONS; i++) {
	CVMGeneration* gen  = CVMglobals.gc.CVMgenGenerations[i];
	CVMUint32*     base = gen->allocBase;
	CVMUint32*     top  = gen->allocPtr;
        CVMBool completeScanDone =
            CVMgcScanObjectRange(ee, base, top,
				 CVMgcCallBackIfNotSynthesized, &info);
        if (!completeScanDone) {
            return CVM_FALSE;
        };
    }
    return CVM_TRUE;
}

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the overall GC. */
void CVMgcimplDumpSysInfo()
{
    CVMGeneration *youngGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration *oldGen = CVMglobals.gc.CVMgenGenerations[1];
    CVMgenSemispaceDumpSysInfo((CVMGenSemispaceGeneration*)youngGen);
    CVMgenMarkCompactDumpSysInfo((CVMGenMarkCompactGeneration*)oldGen);
    CVMgenDumpSysInfo(&CVMglobals.gc);
}
#endif  /* CVM_DEBUG || CVM_INSPECTOR */

#undef roundUpToPage
#undef roundDownToPage
#undef isPageAligned
