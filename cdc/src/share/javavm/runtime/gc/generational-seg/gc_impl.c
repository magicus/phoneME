/*
 * @(#)gc_impl.c	1.107 06/10/10
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
 * This file includes the implementation of a generational collector.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/porting/float.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/time.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/generational-seg/generational.h"

#include "javavm/include/gc/generational-seg/gen_eden.h"
#include "javavm/include/gc/generational-seg/gen_edenspill.h"
#include "javavm/include/gc/generational-seg/gen_markcompact.h"

#include "javavm/include/porting/system.h"

#define CVM_GEN_NUM_GENERATIONS 3
#define LARGE_OBJECTS_TREATMENT
#define LARGE_OBJECT_THRESHOLD 50000

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

/*
 * Initialize GC global state
 */
void
CVMgcimplInitGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("GC: Initializing global state for generational GC\n"));
}

#ifdef CVM_DEBUG_EXTRA_ASSERTIVE_BARRIER
void
doBarrier(CVMObject* obj, CVMObject** slot)
{
     if (((CVMUint32*)obj < CVMglobals.gc.CVMgenGenerations[0]->genBase) || 
	 ((CVMUint32*)obj >= CVMglobals.gc.CVMgenGenerations[0]->genTop)) {
	 CVMGenSegment* thisSeg = SEG_HEADER_FOR(obj);
	 CVMUint8* cardSlot;
	 
	 cardSlot = &thisSeg->cardTableVirtual[(CVMUint32)slot >> CVM_GENGC_CARD_SHIFT];
	 CVMassert(cardSlot >= thisSeg->cardTable);
	 CVMassert(cardSlot < thisSeg->cardTable + thisSeg->cardTableSize);
	 *cardSlot = CARD_DIRTY_BYTE;
     }
}
#endif

#ifdef CVM_VERIFY_HEAP
/* 
 * Replaced CVMUint32* with CVMJavaVal32* in the functions body, because
 * pointer arithmetic would yield incorrect results on 64 bit machines
 * otherwise. Note that CVMUint32* pointers in the signature were not 
 * changed, in order keep the number of changes small.
 */
static CVMBool saneSummarized(CVMGenSegment* segment, 
			      CVMUint8* cardTableSlot,
			      CVMObject** refPtr)
{
    /* Make sure that the card offset for 'refPtr' is in summary table */
    CVMGenSummaryTableEntry* summEntry = 
	SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(cardTableSlot, segment);
    CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(refPtr);
    /*
     * 'offset' is used to store pointer differences.
     * So we make it a CVMAddr which is 4 byte on 32 bit
     * platforms and 8 byte on 64 bit platforms.
     */
    CVMAddr  offset;
    CVMUint32  i;
    
    /* First assert that no summarized entries refer to a null
       pointer.  This is illegal for any summarizedPtr <= refPtr, but
       not necessarily for summarizedPtr > refPTr */
    i = 0;
    while ((i < 4) && ((offset = summEntry->offsets[i]) != 0xff)) {
	CVMJavaVal32* summarizedPtr = cardBoundary + offset;
	if (((*summarizedPtr).raw == 0) &&
	    (summarizedPtr <= (CVMJavaVal32*)refPtr)) {
	    CVMdebugPrintf(("SUMMARIZED NULL: card=0x%x offset=%d "
			    "summPtr = 0x%x\n", cardBoundary, offset,
			    summarizedPtr));
	    return CVM_FALSE;
	}
	i++;
    }
    
    /*
     * If the ref was NULL, and it was not in the summary table, we are OK.
     */
    if (*refPtr == NULL) {
	return CVM_TRUE;
    }
    
    /*
     * Now check that this pointer is indeed summarized
     */
    offset = (CVMJavaVal32*)refPtr - cardBoundary;

    return ((summEntry->offsets[0] == offset) ||
	    (summEntry->offsets[1] == offset) ||
	    (summEntry->offsets[2] == offset) ||
	    (summEntry->offsets[3] == offset));
}

CVMBool
CVMgenCardIsLive(CVMGeneration* gen, CVMObject* obj, CVMObject** refPtr)
{
    CVMGenSegment* segment = NULL;
    CVMUint8* cardTableSlot = NULL;
    /* Make sure that the card associated with 'refPtr' in this generation
       is sane */
    CVMassert(OBJ_IN_CARD_MARKING_RANGE(refPtr));
    segment = SEG_HEADER_FOR(obj);
    cardTableSlot = CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(refPtr, segment);
    /* Non-null must mean the card is dirty or summarized */
    if ((*refPtr == NULL) && (*cardTableSlot == CARD_CLEAN_BYTE)) {
	return CVM_TRUE;
    } else {
	/* Either the card is dirty, or it's summarized at this pointer */
	return ((*cardTableSlot == CARD_DIRTY_BYTE) ||
		((*cardTableSlot == CARD_SUMMARIZED_BYTE) && 
		 saneSummarized(segment, cardTableSlot, refPtr)));
    }
}

void
CVMgenVerifyObjectRangeForCardSanity(CVMGeneration* thisGen,
				     CVMExecEnv* ee, 
				     CVMGCOptions* gcOpts,
				     CVMUint32* start,
				     CVMUint32* top)
{
    CVMUint32* curr = start;
    
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
	
	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    CVMObject* ref = *refPtr;
	    
	    if ((ref == NULL) || thisGen->inYounger(thisGen, ref)) {
		/* A younger pointer. Make sure that the
		   associated card is sane */
		CVMassert(CVMgenCardIsLive(thisGen, currObj, refPtr));
	    } 
	});
	
	/* iterate */
	curr += objSize / 4;
    }        
    CVMassert(curr == top); /* This had better be exact */
}

/* Verify the integrity of the heap in all generations. */
void CVMgenVerifyHeapIntegrity(CVMExecEnv *ee, CVMGCOptions* gcOpts)
{
    CVMGeneration *edenGen = CVMglobals.gc.CVMgenGenerations[0];
    CVMGeneration *spillGen = CVMglobals.gc.CVMgenGenerations[1];
    CVMGeneration *oldGen = CVMglobals.gc.CVMgenGenerations[2];

    edenGen->verifyGen(edenGen, ee, gcOpts);
    spillGen->verifyGen(spillGen, ee, gcOpts);
    oldGen->verifyGen(oldGen, ee, gcOpts);
}

#endif /* CVM_VERIFY_HEAP */

/*
 * %comment: f010
 */
void
CVMgenClearBarrierTable(CVMGenSegment* segBase) {
    CVMGenSegment* segCurr = segBase;

    CVMassert(segBase != NULL);
    do {
       memset(segCurr->cardTable, CARD_CLEAN_BYTE, segCurr->cardTableSize);
       memset(segCurr->objHeaderTable, 0, segCurr->cardTableSize);
       segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);
}

/*
 * Update the object headers table for objects in range [startRange, endRange)
 */
void
CVMgenBarrierObjectHeadersUpdate(CVMGeneration* gen, CVMExecEnv* ee,
				 CVMGCOptions* gcOpts,
				 CVMUint32* startRange,
				 CVMUint32* endRange ,
                                 CVMGenSegment* segCurr)
{
    CVMJavaVal32* curr = (CVMJavaVal32*)startRange;
    CVMtraceGcCollect(("GC[SS,full]: "
		       "object headers update [%x,%x)\n",
		       startRange, endRange));
    CVMassert(segCurr != NULL) ;

    while (curr < (CVMJavaVal32*)endRange) {
	CVMObject* currObj    = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMObject* currObjEnd = (CVMObject*)(curr + objSize / sizeof(CVMJavaVal32));
	CVMUint8*  startCard  =
	    (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(currObj, segCurr);
	CVMUint8*  endCard    =
	    (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(currObjEnd, segCurr);

	/*
	 * I don't need to do anything if object starts on a card boundary
	 * since the object headers table is initialized to 0's. Otherwise
	 * I'd have to make a special case of it here, and check
	 * whether cardBoundary == curr.
	 */
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
	    card = (CVMUint8*)
		CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(cardBoundary, segCurr);
	    CVMassert((card == startCard) || (card == startCard + 1));
	    if (card == (segCurr->cardTable + segCurr->cardTableSize)) {
		CVMassert(card == startCard + 1);
		return; /* nothing to do -- we are at the edge */
	    }
	    hdr = HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr);
	    CVMassert(hdr >= segCurr->objHeaderTable);
	    CVMassert(hdr <  segCurr->objHeaderTable + segCurr->cardTableSize);
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
	    if (endCard == (segCurr->cardTable + segCurr->cardTableSize)) {
		endCard--;
	    }
	    if (endCard - card < 127) {
		/* Fast common case, <= 127 iterations. */
		while (card <= endCard) {
	            CVMassert(hdr <  segCurr->objHeaderTable + segCurr->cardTableSize);
		    *hdr = numCards; /* Plant back-pointers */
		    numCards--;
		    hdr++;
		    card++;
		}
		CVMassert(numCards >= -128);
	    } else {
		/* Slower rarer case for large objects */
		while (card <= endCard) {
	            CVMassert(hdr <  segCurr->objHeaderTable + segCurr->cardTableSize);
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
	    CVMassert(HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr) == hdr);
	}
	curr += objSize / sizeof(CVMJavaVal32);
    }
    CVMassert(curr == (CVMJavaVal32*)endRange); /* This had better be exact */
}

/* 
 * Replaced CVMUint32* with CVMJavaVal32* in the functions body, because
 * pointer arithmetic would yield incorrect results on 64 bit machines
 * otherwise. Note that CVMUint32* pointers in the signature were not 
 * changed, in order keep the number of changes small.
 */
static CVMBool
validYoungerGenerationPtr(CVMGeneration* prevGen, CVMJavaVal32* ptr)
{
    CVMUint32* youngBase = prevGen->genBase;
    CVMUint32* youngTop  = CVMglobals.gc.CVMgenGenerations[0]->genTop;
    /* This ineqality filters out NULL pointers as well */
    return ((CVMUint32*)ptr < youngTop) && ((CVMUint32*)ptr >= youngBase);
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

/* 
 * Replaced CVMUint32* with CVMJavaVal32* in the functions body, because
 * pointer arithmetic would yield incorrect results on 64 bit machines
 * otherwise.
 */
static CVMUint32
scanAndSummarizeObjectsOnCard(CVMExecEnv* ee, CVMGCOptions* gcOpts,
			      CVMJavaVal32* objStart,
			      CVMGenSummaryTableEntry* summEntry,
			      CVMJavaVal32* regionStart, CVMJavaVal32* regionEnd,
			      CVMGeneration* prevGen,
			      CVMRefCallbackFunc callback, void* callbackData)
{
    CVMUint32  scanStatus = CARD_CLEAN_BYTE;
    CVMUint32  numRefs = 0;
    CVMJavaVal32* curr    = objStart;
    CVMJavaVal32* top     = regionEnd;
    CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(regionStart);
    
    CVMtraceGcCollect(("GC[SS,full]: "
		       "Scanning card range [%x,%x), objStart=0x%x\n",
		       regionStart, regionEnd, objStart));
    /*
     * Clear summary table entry.
     */
    summEntry->intVersion = 0xffffffff ;

    /*
     * And scan the objects on the card
     */
    while (curr < top) {
	CVMObject* currObj = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMObject* currCbInstance = *((CVMObject**)CVMcbJavaInstance(currCb));
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);

	/*
	 * First, handle the class. 
	 *
	 * If the class of this object is in a different generation than
	 * the instance, the caller cannot mark this card summarized.
	 */
	if (validYoungerGenerationPtr(prevGen, (CVMJavaVal32*)currCbInstance)) {
	    CVMscanClassWithGCOptsIfNeeded(ee, currCb, gcOpts,
					   callback, callbackData);
	    /* Force the card to stay dirty if the class was not ROMized */
	    if (!CVMcbIsInROM(currCb)) {
		scanStatus = CARD_DIRTY_BYTE;
	    }
	}
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
		if (validYoungerGenerationPtr(prevGen, 
					      (CVMJavaVal32*) *scanPtr)) {
		    if (numRefs < 4) {
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
	    if (validYoungerGenerationPtr(prevGen, heapRef)) {
		if ((CVMJavaVal32*)refPtr >= regionEnd) {
		    /* We ran off the edge of the card */
		    goto returnStatus;
		} else if ((CVMJavaVal32*)refPtr >= regionStart) {
		    /* Call the callback if we are within the bounds */
		    if (numRefs < 4) {
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
	} else if (numRefs <= 4) {
	    scanStatus = CARD_SUMMARIZED_BYTE;
	} else {
	    scanStatus = CARD_DIRTY_BYTE;
	}
    }
    return scanStatus;
}

static CVMJavaVal32* 
findObjectStart(CVMUint8* card, CVMJavaVal32* heapAddrForCard,
                CVMGenSegment* segCurr)
{
    /* If the segment header is at the start of the chunk, 
     * then the first object in the first card of the chunk 
     * would start at the segment->allocBase
     */ 
    if (card == &segCurr->cardTable[0]) {
        return (CVMJavaVal32*)segCurr->allocBase ;
    } else {
    CVMInt8* hdr = HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr);
    CVMInt8  n;

    CVMassert(hdr >= segCurr->objHeaderTable);
    CVMassert(hdr <  segCurr->objHeaderTable + segCurr->cardTableSize);
    CVMassert(HEAP_ADDRESS_FOR_CARD(card, segCurr) == heapAddrForCard);
    n = *hdr;
    if (n >= 0) {
	return heapAddrForCard - n;
    } else {
	CVMJavaVal32* heapAddr = heapAddrForCard;
	do {
	    hdr = hdr + n; /* Go back! */
	    heapAddr = heapAddr + n * NUM_WORDS_PER_CARD;
	    CVMassert(hdr >= segCurr->objHeaderTable);
	    CVMassert(hdr <  segCurr->objHeaderTable + segCurr->cardTableSize);
	    n = *hdr;
	} while (n < 0);
	CVMassert(heapAddr < heapAddrForCard);
	return heapAddr - n;
    }
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
		 CVMGeneration* prevGen,
		 CVMRefCallbackFunc callback,
		 void* callbackData, CVMGenSegment* segCurr)
{		
    CVMassert(card >= segCurr->cardTable);
    CVMassert(card < segCurr->cardTable + segCurr->cardTableSize);
    cardStatsOnly(cStats.cardsScanned++);
    if (*card == CARD_SUMMARIZED_BYTE) {
	/*
	 * This card has not been dirtied since it was summarized. Scan the
	 * summary.
	 */
	CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(lowerLimit);
	CVMGenSummaryTableEntry* summEntry =
	    SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr);
	CVMUint8  offset;
	CVMBool summaryUsed = CVM_FALSE;
	CVMBool markDirty   = CVM_FALSE;
	CVMUint32 i = 0;

	/* If this card is summarized, it must have at least one entry */
	CVMassert(summEntry->offsets[0] != 0xff);
	while ((i < 4) && ((offset = summEntry->offsets[i]) != 0xff)) {
	    CVMObject** refPtr = (CVMObject**)(cardBoundary + offset);
	    /* We only care about what happens within our bounds */
	    if (refPtr < (CVMObject**)higherLimit) {
		/* We could not have summarized a NULL pointer */
		CVMassert(*refPtr != NULL);
		if (validYoungerGenerationPtr(prevGen, (CVMJavaVal32*)*refPtr)) {
		    (*callback)(refPtr, callbackData);
		    summaryUsed = CVM_TRUE;
		}
	    } else {
		/* There are summarized pointers here out of our range. */
		/* Mark this card dirty for later, so these out of range
		   pointers can be flushed */
		markDirty = CVM_TRUE;
	    }
	    i++;
	}
	CVMassert(i <= 4);
	cardStatsOnly(cStats.cardsSummarized++);
	/* What to do for next time? */
	if (markDirty) {
	    *card = CARD_DIRTY_BYTE;
	} else if (!summaryUsed) {
	    *card = CARD_CLEAN_BYTE;
	}
    } else if (*card == CARD_DIRTY_BYTE) {
	CVMJavaVal32* objStart;
	CVMGenSummaryTableEntry* summEntry;

	/* Make sure the heap pointer and the card we are scanning are in
           alignment */
	CVMassert(HEAP_ADDRESS_FOR_CARD(card, segCurr) == CARD_BOUNDARY_FOR(lowerLimit));

	CVMassert(higherLimit - lowerLimit <= NUM_WORDS_PER_CARD);

	objStart = findObjectStart(card, CARD_BOUNDARY_FOR(lowerLimit), 
				   segCurr);
	summEntry = SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr);

        *card = scanAndSummarizeObjectsOnCard(ee, gcOpts, objStart,
			      summEntry, lowerLimit, higherLimit, prevGen,
			      callback, callbackData);
	cardStatsOnly(cStats.cardsDirty++);
    } else {
	CVMassert(*card == CARD_CLEAN_BYTE);
	cardStatsOnly(cStats.cardsClean++);
    }
		      
}

/*
 * Given the address of a slot, mark the card table entry
 */
static void
updateCardTableForSlot(CVMObject* directObj,
		       CVMObject** refPtr, CVMObject* ref,
		       CVMGeneration* gen)
{
    /* 
     * If ref is a cross generational pointer towards the younger
     * generation, mark (directObj, refPtr) in the card table 
     */
    if (gen->inYounger(gen, ref)) {
	SET_CARD_TABLE_SLOT_FOR_ADDRESS(directObj, refPtr, CARD_DIRTY_BYTE);
    }
}

/*
 * Scan objects in contiguous range, without doing any special handling.
 */
static void
scanObjectsAndUpdateBarrierInRange(CVMGeneration* gen,
                                   CVMExecEnv* ee, CVMGCOptions* gcOpts,
                                   CVMUint32* base, CVMUint32* top)
{
    CVMUint32* curr = base;

    while (curr < top) {
        CVMObject* currObj    = (CVMObject*)curr;
        CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMObject* currCbInstance = *((CVMObject**)CVMcbJavaInstance(currCb));
        CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);

	CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
	    updateCardTableForSlot(currObj, refPtr, *refPtr, gen);
	});

	/*
	 * Mark card for this slot if necessary for class of object
	 */
	updateCardTableForSlot(currObj, (CVMObject**)currObj, 
			       currCbInstance, gen);

        /* iterate */
        curr += objSize / 4;
    }
 
    CVMassert(curr == top); /* This had better be exact */
}

void
clearBarrierTableForSegment(CVMGeneration* gen,
			    CVMGenSegment* segCurr,
			    CVMUint32* base,
			    CVMUint32* top) 
{
    CVMUint8*  lowCard;
    CVMUint8*  highCard;
    CVMUint8*  cardRange;
    CVMAddr    count;
    CVMAddr    cardOffset;
    
    lowCard  = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(base, segCurr);
    highCard = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(top, segCurr);
    
    if ((((CVMUint32*)CARD_BOUNDARY_FOR(base) == base) || (segCurr->allocBase == base)) &&
	(lowCard != highCard)) {
	/* Object spans multiple cards and starts on card or segment
           boundary */
	cardRange = lowCard;
    } else {
	/* Partial object on card, or object wholly on card.
	   Assume dirty */
	*lowCard = CARD_DIRTY_BYTE; 
	cardRange = lowCard + 1;
    }
    if ((CVMUint32*)CARD_BOUNDARY_FOR(top) == top) {
	/* Nothing on this card, since top is on a card boundary */
	highCard--;
    }
    
    if (cardRange > highCard) {
	return; /* Nothing to do */
    }
    
    /* We'll cover [cardRange, highCard] */
    count = highCard - cardRange + 1;
    cardOffset = cardRange - segCurr->cardTable;
    
    CVMassert(cardRange + count <= 
	      &segCurr->cardTable[segCurr->cardTableSize]);
    CVMassert(count <= segCurr->cardTableSize);
    CVMassert(cardOffset + count <= segCurr->cardTableSize);
    
    memset(cardRange, CARD_CLEAN_BYTE, count);
    memset(segCurr->objHeaderTable + cardOffset, 0, count);
}

/*
 * Rebuild the barrier table. This can be done either incrementally,
 * from allocMark to allocPtr, or for the entire generation, from
 * allocBase to allocPtr. The 'full' flag controls this.
 */
void
CVMgenBarrierTableRebuild(CVMGeneration* gen, 
			  CVMExecEnv* ee, CVMGCOptions* gcOpts,
			  CVMBool full)
{
    CVMGenSegment* segBase = (CVMGenSegment*)gen->allocBase;
    CVMGenSegment* segCurr;
    
    segCurr = segBase;
    
    do {
	CVMUint32* startRange;
	CVMUint32* endRange;
	
	CHECK_SEGMENT(segCurr);
	if (full) {
	    startRange = segCurr->allocBase;
	} else {
	    startRange = segCurr->allocMark;
	}
	endRange = segCurr->allocPtr;
	
	if (endRange > startRange) {
	    /* Start with a clean slate, since
	       scanObjectsAndUpdateBarrierInRange will only mark dirty
	       cards. Make sure there is no stale info there */
	    clearBarrierTableForSegment(gen, segCurr, startRange, endRange);
	
	    /* Start with the object headers table */
	    CVMgenBarrierObjectHeadersUpdate(gen, ee, gcOpts,
					     startRange,
					     endRange, 
					     segCurr);
	    
	    /* And now mark dirty cards in range */
	    scanObjectsAndUpdateBarrierInRange(gen, ee, gcOpts, 
					       startRange,
					       endRange);
	    /* Advance mark for the next batch of promotions */
	    segCurr->allocMark = segCurr->allocPtr;  
	} else {
	    CVMassert(segCurr->allocMark == segCurr->allocPtr);
	}
	
        segCurr = segCurr->nextSeg;
    } while (segCurr != segBase);
}

/*
 * Traverse all recorded pointers, and call 'callback' on each.
 */
/*
 * Replaced CVMUint32* with CVMJavaVal32* in the functions body, because
 * pointer arithmetic would yield incorrect results on 64 bit machines
 * otherwise.
 */
void
CVMgenBarrierPointersTraverse(CVMGeneration* gen, CVMExecEnv* ee,
			      CVMGCOptions* gcOpts,
			      CVMRefCallbackFunc callback,
			      void* callbackData)
{
    CVMUint8*  lowerCardLimit;    /* Card to begin scanning                  */
    CVMUint8*  higherCardLimit;   /* Card to end scanning                    */
    /* This should not be of type CVMJavaVal32, because it's merely
     * used to scan 4 cards (each of 8 bit size) at once.  */
    CVMUint32* cardPtrWord;       /* Used for batch card scanning            */
    CVMJavaVal32* heapPtr;        /* Track card boundaries in heap           */
    /* 
     * 'remainder' is used to store pointer differences.
     * So we make it a CVMAddr which is 4 byte on 32 bit
     * platforms and 8 byte on 64 bit platforms.
     */
    CVMAddr    remainder;         /* Batch scanning spillover                */
    CVMJavaVal32* segLower;          /* Lower limit of live objects in Segment  */
    CVMJavaVal32* segHigher;         /* Higher limit of live objects in Segment */
    CVMGeneration* prevGen;       /* prev younger generation                 */

    CVMGenSegment* segSentry = (CVMGenSegment*)gen->allocBase ;
    CVMGenSegment* segCurr = segSentry ;

    prevGen = gen->prevGen ;

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

    do {

    segLower        = (CVMJavaVal32*)segCurr->allocBase;
    segHigher       = (CVMJavaVal32*)segCurr->allocMark;

    /* ... and look at all the cards in [lowerCardLimit,higherCardLimit) */
    lowerCardLimit  = (CVMUint8*)
	CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(segLower, segCurr);
    higherCardLimit = (CVMUint8*)
	CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(segHigher, segCurr);

    /* This is the heap boundary corresponding to the next card */
    heapPtr         = NEXT_CARD_BOUNDARY_FOR(segLower);
    
    /*
     * Scan the first card in range, which may be partial.
     */

    /* Check whether the amount of data is really small or not */
    if (segHigher < heapPtr) {
	/* This is really rare. So we might as well check for the
	   condition that genLower == genHigher, i.e. an empty old
	   generation! */
	if (segLower == segHigher) {
            continue ;
	}
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 segLower, segHigher,
			 prevGen, callback, callbackData, segCurr);
        continue ;/* Done, only one partial card! */
    } else {
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 segLower, heapPtr,
			 prevGen, callback, callbackData, segCurr);
	lowerCardLimit++;
    }
    
    /*
     * Have we already reached the end?
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == segHigher) {
            continue ; /* nothing to do  for this segment*/
	}
	/*
	 * Make sure this is really the last card.
	 */
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 heapPtr, segHigher,
			 prevGen, callback, callbackData, segCurr);
        continue ; /* Done, only one partial card! */
    }

    /*
     * How many individual card bytes are we going to look at until
     * we get to an integer boundary?
     */
    /* 
     * make CVM ready to run on 64 bit platforms
     * 
     * CVMalignWordUp() returns a value of type CVMAddr
     * therefore the cast has to be CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    remainder =	CVMalignWordUp(lowerCardLimit) - (CVMAddr)lowerCardLimit;
    CVMassert(CARD_BOUNDARY_FOR(segLower) == (CVMJavaVal32*)segCurr);
    /*
     * Get lowerCardLimit to a word boundary
     */
    while ((remainder-- > 0) && (lowerCardLimit < higherCardLimit)) {
	callbackIfNeeded(ee, gcOpts, lowerCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 prevGen, callback, callbackData, segCurr);
	lowerCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Nothing else to do
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == segHigher) {
            continue ; /* nothing to do */
	}
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit, segCurr) == heapPtr);
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, segHigher,
			 prevGen, callback, callbackData, segCurr);
        continue ;
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

    /*
     * Now go through the card table in blocks of four for faster
     * zero checks.
     */
    for (cardPtrWord = (CVMUint32*)lowerCardLimit;
	 cardPtrWord < (CVMUint32*)higherCardLimit;
	 cardPtrWord++, heapPtr += NUM_WORDS_PER_CARD * 4) {
	if (*cardPtrWord != FOUR_CLEAN_CARDS) {
	    CVMJavaVal32* hptr = heapPtr;
	    CVMUint8*  cptr = (CVMUint8*)cardPtrWord;
	    CVMUint8*  cptr_end = cptr + 4;
	    for (; cptr < cptr_end; cptr++, hptr += NUM_WORDS_PER_CARD) {
		callbackIfNeeded(ee, gcOpts, cptr,
				 hptr, hptr + NUM_WORDS_PER_CARD,
				 prevGen, callback, callbackData, segCurr);
	    }
	}
    }

    /*
     * And finally, the remaining few cards, if any, that "spilled" out
     * of the word boundary.
     */
    while (remainder-- > 0) {
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 prevGen, callback, callbackData, segCurr);
	higherCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Now if there is a partial card left, handle it.
     */
    if (heapPtr < segHigher) {
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit, segCurr) == heapPtr);
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	callbackIfNeeded(ee, gcOpts, higherCardLimit,
			 heapPtr, segHigher,
			 prevGen, callback, callbackData, segCurr);
    }
    } while ((segCurr = segCurr->nextSeg) != segSentry) ;
}

static void
scanObjectsOnCard(CVMExecEnv* ee, CVMGCOptions* gcOpts,
                  CVMGeneration* currentGen,
		  CVMUint8* card, 
		  CVMJavaVal32* lowerLimit, CVMJavaVal32* higherLimit, 
		  CVMRefCallbackFunc callback,
		  void* callbackData, 
                  CVMGenSegment* segCurr)
{		
    CVMassert(card >= segCurr->cardTable);
    CVMassert(card < segCurr->cardTable + segCurr->cardTableSize);

    if (*card == CARD_SUMMARIZED_BYTE) {
	/*
	 * This card has not been dirtied since it was summarized. Scan the
	 * summary.
	 */
	CVMJavaVal32* cardBoundary = CARD_BOUNDARY_FOR(lowerLimit);
	CVMGenSummaryTableEntry* summEntry =
            SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(card, segCurr);
	CVMUint32 i;
	CVMUint8  offset;
	CVMUint32  numOldYoungRefs;

	i = 0;
	numOldYoungRefs = 0;

	/* If this card is summarized, it must have at least one entry */
	CVMassert(summEntry->offsets[0] != 0xff);
	while ((i < 4) && ((offset = summEntry->offsets[i]) != 0xff)) {
	    CVMObject** refPtr = (CVMObject**)(cardBoundary + offset);
	    if (refPtr < (CVMObject**)higherLimit) {
		/* We could not have summarized a NULL pointer */
		CVMassert(*refPtr != NULL);
		(*callback)(refPtr, callbackData);
	    }
	    i++;
	}
	CVMassert(i <= 4);
    } else if (*card == CARD_DIRTY_BYTE) {
	CVMJavaVal32* objStart = 
	    findObjectStart(card, CARD_BOUNDARY_FOR(lowerLimit), segCurr);
        CVMJavaVal32* curr    = (CVMJavaVal32*)objStart;
        CVMJavaVal32* top     = higherLimit;
       
        /* Make sure the heap pointer and the card we are scanning are in
           alignment */
        CVMassert(HEAP_ADDRESS_FOR_CARD(card, segCurr) == CARD_BOUNDARY_FOR(lowerLimit));

        /*
         * And scan the objects on the card
         */
        while (curr < top) {
	    CVMObject* currObj = (CVMObject*)curr;
	    CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	    CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	    CVMobjectWalkRefs(ee, gcOpts, currObj, currCb, {
                if (*refPtr != 0 && !CVMobjectIsInROM(*refPtr)) {
                    (*callback)(refPtr, callbackData);
		}
            });
	    /* iterate */
	    curr += objSize / sizeof(CVMJavaVal32);
        } /*while*/
    } else {
	CVMassert(*card == CARD_CLEAN_BYTE);
    }
}

/* 
 * Replaced CVMUint32* with CVMJavaVal32* in the functions body, because
 * pointer arithmetic would yield incorrect results on 64 bit machines
 * otherwise.
 */
void
CVMgenScanBarrierObjects(CVMGeneration* gen, CVMExecEnv* ee,
                         CVMGCOptions* gcOpts,
			 CVMRefCallbackFunc callback,
			 void* callbackData)
{
    CVMUint8*  lowerCardLimit;    /* Card to begin scanning                  */
    CVMUint8*  higherCardLimit;   /* Card to end scanning                    */
    /* This should not be of type CVMJavaVal32, because it's merely
     * used to scan 4 cards (each of 8 bit size) at once.  */
    CVMUint32* cardPtrWord;       /* Used for batch card scanning            */
    CVMJavaVal32* heapPtr;           /* Track card boundaries in heap           */
    /* 
     * 'remainder' is used to store pointer differences.
     * So we make it a CVMAddr which is 4 byte on 32 bit
     * platforms and 8 byte on 64 bit platforms.
     */
    CVMAddr       remainder;         /* Batch scanning spillover                */
    CVMJavaVal32* segLower;          /* Lower limit of live objects in Segment  */
    CVMJavaVal32* segHigher;         /* Higher limit of live objects in Segment */

    CVMGenSegment* segSentry = (CVMGenSegment*)gen->allocBase ;
    CVMGenSegment* segCurr = segSentry ;

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

    do {

    segLower        = (CVMJavaVal32*)segCurr->allocBase;
    segHigher       = (CVMJavaVal32*)segCurr->allocMark;

    /* ... and look at all the cards in [lowerCardLimit,higherCardLimit) */
    lowerCardLimit  = (CVMUint8*)
	CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(segLower, segCurr);
    higherCardLimit = (CVMUint8*)
	CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(segHigher, segCurr);

    /* This is the heap boundary corresponding to the next card */
    heapPtr         = NEXT_CARD_BOUNDARY_FOR(segLower);
    
    /*
     * Scan the first card in range, which may be partial.
     */

    /* Check whether the amount of data is really small or not */
    if (segHigher < heapPtr) {
	/* This is really rare. So we might as well check for the
	   condition that genLower == genHigher, i.e. an empty old
	   generation! */
	if (segLower == segHigher) {
            continue ;
	}
	scanObjectsOnCard(ee, gcOpts, gen, lowerCardLimit,
			 segLower, segHigher,
			 callback, callbackData, segCurr);
        continue ;/* Done, only one partial card! */
    } else {
	scanObjectsOnCard(ee, gcOpts, gen, lowerCardLimit,
			 segLower, heapPtr,
			 callback, callbackData, segCurr);
	lowerCardLimit++;
    }
    
    /*
     * Have we already reached the end?
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == segHigher) {
            continue ; /* nothing to do  for this segment*/
	}
	/*
	 * Make sure this is really the last card.
	 */
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	scanObjectsOnCard(ee, gcOpts, gen, lowerCardLimit,
			 heapPtr, segHigher,
			 callback, callbackData, segCurr);
        continue ; /* Done, only one partial card! */
    }

    /*
     * How many individual card bytes are we going to look at until
     * we get to an integer boundary?
     */
    remainder =	CVMalignWordUp(lowerCardLimit) - (CVMAddr)lowerCardLimit;
    CVMassert(CARD_BOUNDARY_FOR(segLower) == (CVMJavaVal32*)segCurr);
    /*
     * Get lowerCardLimit to a word boundary
     */
    while ((remainder-- > 0) && (lowerCardLimit < higherCardLimit)) {
	scanObjectsOnCard(ee, gcOpts, gen, lowerCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 callback, callbackData, segCurr);
	lowerCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Nothing else to do
     */
    if (lowerCardLimit == higherCardLimit) {
	if (heapPtr == segHigher) {
            continue ; /* nothing to do */
	}
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit, segCurr) == heapPtr);
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	scanObjectsOnCard(ee, gcOpts, gen, higherCardLimit,
			 heapPtr, segHigher,
			 callback, callbackData, segCurr);
        continue ;
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

    /*
     * Now go through the card table in blocks of four for faster
     * zero checks.
     */
    for (cardPtrWord = (CVMUint32*)lowerCardLimit;
	 cardPtrWord < (CVMUint32*)higherCardLimit;
	 cardPtrWord++, heapPtr += NUM_WORDS_PER_CARD * 4) {
	if (*cardPtrWord != FOUR_CLEAN_CARDS) {
	    CVMJavaVal32* hptr = heapPtr;
	    CVMUint8*  cptr = (CVMUint8*)cardPtrWord;
	    CVMUint8*  cptr_end = cptr + 4;
	    for (; cptr < cptr_end; cptr++, hptr += NUM_WORDS_PER_CARD) {
		scanObjectsOnCard(ee, gcOpts, gen, cptr,
				 hptr, hptr + NUM_WORDS_PER_CARD,
				 callback, callbackData, segCurr);
	    }
	}
    }

    /*
     * And finally, the remaining few cards, if any, that "spilled" out
     * of the word boundary.
     */
    while (remainder-- > 0) {
	scanObjectsOnCard(ee, gcOpts, gen, higherCardLimit,
			 heapPtr, heapPtr + NUM_WORDS_PER_CARD,
			 callback, callbackData, segCurr);
	higherCardLimit++;
	heapPtr += NUM_WORDS_PER_CARD;
    }
    /*
     * Now if there is a partial card left, handle it.
     */
    if (heapPtr < segHigher) {
	/* Make sure that we are "on the same page", literally! */
	CVMassert(HEAP_ADDRESS_FOR_CARD(higherCardLimit, segCurr) == heapPtr);
	CVMassert(segHigher - heapPtr < NUM_WORDS_PER_CARD);
	scanObjectsOnCard(ee, gcOpts, gen, higherCardLimit,
			 heapPtr, segHigher,
			 callback, callbackData, segCurr);
    }
    } while ((segCurr = segCurr->nextSeg) != segSentry) ;
}

struct CVMClassScanOptions {
    CVMRefCallbackFunc callback;
    void* callbackData;
    CVMGeneration* generation; /* Scan only classes in this generation */
};

typedef struct CVMClassScanOptions CVMClassScanOptions;

/*
 * Callback function to scan a promoted class' statics
 */
static void
CVMgenScanClassPointersInGeneration(CVMExecEnv* ee, CVMClassBlock* cb,
				    void* opts)
{
    CVMObject* instance = *((CVMObject**)CVMcbJavaInstance(cb));
    CVMClassScanOptions* options = (CVMClassScanOptions*)opts;
    CVMGeneration* gen = options->generation;
    /*
     * Scan the class state if the java.lang.Class instance is in this
     * generation.
     */
    CVMscanClassIfNeededConditional(ee, cb, gen->inGeneration(gen, instance),
				    options->callback, options->callbackData);
}

/*
 * Find all class instances in generation 'gen' and scan them
 * for pointers. 
 */
static void
CVMgenScanClassPointersInGen(CVMGeneration* gen,
			     CVMExecEnv* ee, CVMGCOptions* gcOpts,
			     CVMRefCallbackFunc callback,
			     void* callbackData)
{
    CVMClassScanOptions opts;

    opts.callback = callback;
    opts.callbackData = callbackData;
    opts.generation = gen;
    CVMclassIterateDynamicallyLoadedClasses(
        ee, CVMgenScanClassPointersInGeneration, &opts);
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
    CVMGeneration* edenGen;
    CVMGeneration* spillGen;
    CVMGeneration* oldGen;
    CVMUint32  totYoungerBytes; /* Total space for younger generations */
    char*       edenGenAttr;
    char*       spillGenAttr;
    CVMUint32   edenGenSizeBytes;
    CVMUint32   spillGenSizeBytes;
    CVMUint32   oldGenSizeBytes;
    CVMUint32   maxOldGenSizeBytes;
    CVMBool     needToDumpGenSizes = CVM_FALSE;
    CVMBool     edenGenSizeWasSpecified = CVM_FALSE;

#ifdef CVM_JVMPI
    liveObjectCount = 0;
#endif

    /* NOTE: There are 2 parts to this function.  The first part is the heap
       size allotment / initialization policy that determines the sizes of
       each generation.  The second part is the allocation code that actually
       allocate memory for each of the generations and the various heap data
       structures.
    */

    /*======================================================================
    // Part 1: The heap size allotment / initialization policy.
    //
    // This part is responsible for computing the amount of memory that
    // should be allocated for each of the generations based on a policy and
    // user specified parameters.  This part is also responsible for
    // enforcing that all these computed sizes will meet any alignment/packing
    // requirements so that the subsequent allocation part won't have to
    // worry about all them.
    */

    /* Make sure that the min eden gen size is aligned on an 8 byte
       boundary: */
    CVMassert(CVM_GCIMPL_MIN_EDEN_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_MIN_EDEN_SIZE_BYTES,
                            sizeof(CVMJavaLong)));

    /* Make sure that the default eden gen size is aligned on an 8 byte
       boundary: */
    CVMassert(CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES,
                            sizeof(CVMJavaLong)));

    /* Make sure that the min spill gen size is aligned on a segment size
       boundary: */
    CVMassert(CVM_GCIMPL_MIN_SPILL_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_MIN_SPILL_SIZE_BYTES,
                            CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

    /* Make sure that the default spill gen size is aligned on a segment size
       boundary: */
    CVMassert(CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES,
                            CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

    /* Make sure that the min spill gen size is aligned on a segment size
       boundary: */
    CVMassert(CVM_GCIMPL_MIN_OLD_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_MIN_OLD_SIZE_BYTES,
                            CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

    /* Make sure that the default spill gen size is aligned on a segment size
       boundary: */
    CVMassert(CVM_GCIMPL_DEFAULT_OLD_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_DEFAULT_OLD_SIZE_BYTES,
                            CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

    /* Make sure that the min gen sizes add up to less than the min total min
       heap size: */
    CVMassert((CVM_GCIMPL_MIN_EDEN_SIZE_BYTES +
               CVM_GCIMPL_MIN_SPILL_SIZE_BYTES +
               CVM_GCIMPL_MIN_OLD_SIZE_BYTES)
	      <= CVM_GCIMPL_MIN_TOTAL_HEAP_SIZE_BYTES);

    /* Make sure that the scratch buffer size is aligned on an 8 byte
       boundary: */
    CVMassert(CVM_GCIMPL_SCRATCH_BUFFER_SIZE_BYTES ==
              CVMpackSizeBy(CVM_GCIMPL_SCRATCH_BUFFER_SIZE_BYTES,
                            sizeof(CVMJavaLong)));

    /* Just make sure that the specified min and max sizes are packed to the
       proper alignments.  This simplifies are generation size computations
       later: */
    minHeapSize = CVMpackSizeBy(minHeapSize, sizeof(CVMJavaLong));
    maxHeapSize = CVMpackSizeBy(maxHeapSize, sizeof(CVMJavaLong));

    /* Make sure that the minimum heap size if not too small: */
    if (minHeapSize < CVM_GCIMPL_MIN_TOTAL_HEAP_SIZE_BYTES) {
        CVMconsolePrintf("Warning: min heap size (%d bytes) is too small\n",
                         minHeapSize);
        CVMconsolePrintf("\tSetting min heap size to %d bytes\n",
                         CVM_GCIMPL_MIN_TOTAL_HEAP_SIZE_BYTES);
        minHeapSize = CVM_GCIMPL_MIN_TOTAL_HEAP_SIZE_BYTES;
    }

    /* Make sure that the max heap size is reasonable: */
    if (maxHeapSize < minHeapSize) {
        CVMconsolePrintf("Warning: max heap size (%d bytes) is less than min "
                         "heap size (%d bytes)\n", maxHeapSize, minHeapSize);
        CVMconsolePrintf("\tSetting max heap size to %d bytes\n",
                         minHeapSize);
        maxHeapSize = minHeapSize;
    }

    /*
     * Parse relevant attributes
     */

    /* NOTE: The value of UNSPECIFIED_SIZE is set to the encoding of -1 by
       design because this shows up both as a large number as well as a
       value that will be rejected if it is set in the youngGen and middleGen
       attributes.  The comparisons that follow depends on this property.
    */
#undef UNSPECIFIED_SIZE
#define UNSPECIFIED_SIZE   ((CVMUint32)-1)
    /* Get the specified edenGen size if any: */
    edenGenAttr = CVMgcGetGCAttributeVal("youngGen");
    edenGenSizeBytes = UNSPECIFIED_SIZE;
    if (edenGenAttr != NULL) {
        CVMInt32 size = CVMoptionToInt32(edenGenAttr);
        if (size >= 0) {
            edenGenSizeBytes = CVMpackSizeBy(size, sizeof(CVMJavaLong));
            edenGenSizeWasSpecified = CVM_TRUE;
        }
    }

    /* Get the specified spillGen size if any: */
    spillGenAttr = CVMgcGetGCAttributeVal("middleGen");
    spillGenSizeBytes = UNSPECIFIED_SIZE;
    if (spillGenAttr != NULL) {
        CVMInt32 size = CVMoptionToInt32(spillGenAttr);
        if (size >= 0) {
            spillGenSizeBytes = size;
        }
    }

    /* If the specified eden size is too small, then bump it up to the
       minimum.  If it is unspecified, it will show up as a rediculously
       large value and pass this test.  We'll set it to a reasonable value
       later. */
    if (edenGenSizeBytes < CVM_GCIMPL_MIN_EDEN_SIZE_BYTES) {
        CVMconsolePrintf("Warning: youngGen size (%d bytes) is too small\n",
                         edenGenSizeBytes);
        CVMconsolePrintf("\tSetting youngGen size to %d bytes\n",
                         CVM_GCIMPL_MIN_EDEN_SIZE_BYTES);
        edenGenSizeBytes = CVM_GCIMPL_MIN_EDEN_SIZE_BYTES;
    }

    /* If the specified spill size is too small, then bump it up to the
       minimum.  If it is unspecified, it will show up as a rediculously
       large value and pass this test.  We'll set it to a reasonable value
       later. */
    if (spillGenSizeBytes < CVM_GCIMPL_MIN_SPILL_SIZE_BYTES) {
        CVMconsolePrintf("Warning: middleGen size (%d bytes) is too small\n",
                         spillGenSizeBytes);
        CVMconsolePrintf("\tSetting middleGen size to %d bytes\n",
                         CVM_GCIMPL_MIN_SPILL_SIZE_BYTES);
        spillGenSizeBytes = CVM_GCIMPL_MIN_SPILL_SIZE_BYTES;

    } else if (spillGenSizeBytes != UNSPECIFIED_SIZE) {
        /* Make sure that spill gen size is aligned on a segment boundary: */
        CVMUint32 roundedSize = CVMpackSizeBy(spillGenSizeBytes,
                                    CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES);
        if (spillGenSizeBytes != roundedSize) {
            CVMconsolePrintf("Warning: middleGen size (%d bytes) need to be "
                             "in increments of %d bytes\n",
                             spillGenSizeBytes,
                             CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES);
            CVMconsolePrintf("\tSetting middleGen size to %d bytes\n",
                             roundedSize);
            spillGenSizeBytes = roundedSize;
        }
    }

    /* Set the old gen size to the minimum by default.  It may get increased
       if space is available: */
    oldGenSizeBytes = CVM_GCIMPL_MIN_OLD_SIZE_BYTES;

    if ((edenGenSizeBytes == UNSPECIFIED_SIZE) &&
        (spillGenSizeBytes == UNSPECIFIED_SIZE)) {

computeEdenAndSpillSizes:
        /* We got here because the eden and spill gen sizes are unspecified.
           So, we are going to assign the values of the eden and spill gen
           sizes. */
        if (minHeapSize < (CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES +
                           CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES +
                           CVM_GCIMPL_MIN_OLD_SIZE_BYTES)) {
            CVMJavaFloat size, ratio;
            CVMUint32 heapSize = minHeapSize - oldGenSizeBytes;

            /* Compute the fraction of the heap size which should be
               allocated to the spillGen using the default ratio as specified
               by the default eden and spill gen sizes: */
            size = CVMint2Float(heapSize);
            ratio = CVMfloatDiv(
                        CVMint2Float(CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES),
                        CVMint2Float(CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES +
                                     CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES));
            size = CVMfloatMul(size, ratio);
            spillGenSizeBytes = CVMfloat2Int(size);
            if (spillGenSizeBytes < CVM_GCIMPL_MIN_SPILL_SIZE_BYTES) {
                spillGenSizeBytes = CVM_GCIMPL_MIN_SPILL_SIZE_BYTES;
            }

            /* We need to round the computed spill gen size up to an increment
               of the segment alignment size.  Since we are quaranteed that
               minHeapSize >= CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES by the time
               we get here, we should not need to worry about this rounding
               causing spillGenSizeBytes to exceed minHeapSize: */
            spillGenSizeBytes = CVMpackSizeBy(spillGenSizeBytes,
                                    CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES);

            /* Whatever's left over can be used as the eden gen size: */
            edenGenSizeBytes = heapSize - spillGenSizeBytes;
            goto computeMaxOldGenSizeBytes;

        } else {
            /* Try to get eden and spill as close to the defaults as we can: */
            edenGenSizeBytes = CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES;
            spillGenSizeBytes = CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES;
        }

    } else if (edenGenSizeBytes == UNSPECIFIED_SIZE) {

        /* If we get here, then spillGenSizeBytes must have been specified. */
        edenGenSizeBytes = minHeapSize - spillGenSizeBytes -
                           oldGenSizeBytes;
        if (edenGenSizeBytes > CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES) {
            edenGenSizeBytes = CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES;
        }

    } else if (spillGenSizeBytes == UNSPECIFIED_SIZE) {

        /* If we get here, then edenGenSizeBytes must have been specified. */
        /* Assign the remainder of minHeapSize to spillGen: */
        spillGenSizeBytes = minHeapSize - edenGenSizeBytes -
                            oldGenSizeBytes;
        if (spillGenSizeBytes > CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES) {
            spillGenSizeBytes = CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES;
        } else {
            spillGenSizeBytes = CVMpackSizeBy(spillGenSizeBytes,
                                    CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES);
            /* If the total space now exceeds the maxHeapSize, take away from
               eden to pay for the packing of spill gen.  Otherwise, we're
               fine.*/
            if (edenGenSizeBytes + spillGenSizeBytes + oldGenSizeBytes >
                maxHeapSize) {
                /* Take the access away from eden: */
                edenGenSizeBytes = maxHeapSize - spillGenSizeBytes -
                                   oldGenSizeBytes;
                CVMconsolePrintf("Warning: youngGen size need to be resized "
                                 "to make room for middleGen alignment\n");
                needToDumpGenSizes = CVM_TRUE;
            }
        }

    } else {

        /* If we get here, then both edenGenSizeBytes and spillGenSizeBytes
           must have been specified. */

        /* Assign the remainder of minHeapSize to spillGen: */
        if (edenGenSizeBytes + spillGenSizeBytes + oldGenSizeBytes >
            maxHeapSize) {
            CVMconsolePrintf("Warning: specified youngGen (%d bytes) and "
                             "middleGen (%d bytes) sizes are too large to "
                             "fit\n", edenGenSizeBytes, spillGenSizeBytes);
            CVMconsolePrintf("\tWill assign generation sizes automatically\n");
            needToDumpGenSizes = CVM_TRUE;
            goto computeEdenAndSpillSizes;
        }
    }
#undef UNSPECIFIED_SIZE

    /* Assign the remainder of minHeapSize to oldGen: */
    oldGenSizeBytes = minHeapSize - edenGenSizeBytes - spillGenSizeBytes;
    CVMassert(edenGenSizeBytes + spillGenSizeBytes <= minHeapSize);
    /* But make sure that the oldGen is packed properly for alignment: */
    oldGenSizeBytes = CVMpackSizeBy(oldGenSizeBytes,
                          CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES);
    /* If the total space now exceeds the maxHeapSize, take away from eden to
       pay for the packing of old gen.  Otherwise, we're fine.*/
    if (edenGenSizeBytes + spillGenSizeBytes + oldGenSizeBytes > maxHeapSize) {
        /* Take the access away from eden: */
        edenGenSizeBytes = maxHeapSize - spillGenSizeBytes - oldGenSizeBytes;
        if (edenGenSizeWasSpecified) {
            CVMconsolePrintf("Warning: youngGen size need to be resized "
                             "to make room for oldGen alignment\n");
            needToDumpGenSizes = CVM_TRUE;
        }
    }

computeMaxOldGenSizeBytes:
    /* Compute the max old gen size: */
    maxOldGenSizeBytes = maxHeapSize - edenGenSizeBytes - spillGenSizeBytes;

    /* Once again, make sure that the values we ended up with make sense and
       satisfies all alignment requirements: */
    CVMassert(edenGenSizeBytes + spillGenSizeBytes <= minHeapSize);
    CVMassert(spillGenSizeBytes == CVMpackSizeBy(spillGenSizeBytes,
                                       CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));
    CVMassert(oldGenSizeBytes == CVMpackSizeBy(oldGenSizeBytes,
                                     CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES));

    /* Dump the gen sizes if needed: */
    if (needToDumpGenSizes || (CVMgcGetGCAttributeVal("verbose") != NULL)) {
        CVMconsolePrintf("Heap generation sizes:\n");
        CVMconsolePrintf("\tyoungGen size:   %d bytes\n", edenGenSizeBytes);
        CVMconsolePrintf("\tmiddleGen size:  %d bytes\n", spillGenSizeBytes);
        CVMconsolePrintf("\toldGen size:     %d bytes\n", oldGenSizeBytes);
        CVMconsolePrintf("\tmax oldGen size: %d bytes\n", maxOldGenSizeBytes);
        CVMconsolePrintf("\tinitial total:   %d bytes\n",
            edenGenSizeBytes + spillGenSizeBytes + oldGenSizeBytes);
        CVMconsolePrintf("\tmax total:       %d bytes\n",
            edenGenSizeBytes + spillGenSizeBytes + maxOldGenSizeBytes);
    }

    globalState->genGCAttributes.youngGenSize = edenGenSizeBytes;
    totYoungerBytes = edenGenSizeBytes + spillGenSizeBytes +
                      CVM_GCIMPL_SCRATCH_BUFFER_SIZE_BYTES;

    /*======================================================================
    // Part 2: Allocating the necessary memory.
    //
    // This part is responsible for allocating the amount of memory that
    // is computed in the above section.  The VM needs to fail gracefully if
    // we fail to allocate the require memory.
    */

    globalState->heapBase =
        CVMmemalignAlloc(CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES, totYoungerBytes);
    if (globalState->heapBase == NULL) {
        goto failed;
    }

    /* Sanity check memalign: */
    if ((CVMAddr)globalState->heapBase %
         CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES != 0) {
	CVMpanic("CVMmemalignAlloc() returns unaligned storage");
    }

    /* NOTE: Since the spillGen has to be aligned to increments of
       CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES (due to the card table marking
       algorithm requirements), we allocate the spillGen from the heapBase
       because this is the location which is guaranteed to be aligned to
       CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES.

       The edenGen will be allocated after the spillGen.  This is OK because
       edenGen does not have alignment requirements (except to be aligned to
       sizeof(CVMJavaLong)) since it does not have a card table.  And after
       edenGen, we can allocate the scratch buffer which like the edenGen
       only need to be aligned by sizeof(CVMJavaLong).
                  ________________
       heapBase: | spillGen       | <== All spillGen segments are in the
                 |                |     spillGen block.
                 |________________|
                 | edenGen        |
                 |                |
                 |________________|
                 | scratch buffer |
                 |                |
                 |________________|

                  ________________
                 | oldGen segment | <== oldGen segments are allocated
                 |________________|     individually and are not guaranteed
                 ...                    to be contiguous.
                  ________________
                 | oldGen segment |
                 |________________|
    */

    /* Instantiate the edenGen: */
    edenGen = CVMgenEdenAlloc(globalState->heapBase +
                  spillGenSizeBytes/sizeof(CVMUint32), edenGenSizeBytes);
    if (edenGen == NULL) {
        goto failed_after_edenGen;
    }
    edenGen->generationNo = 0;
    globalState->CVMgenGenerations[0] = edenGen;

    /* Instantiate the spillGen: */
    spillGen = CVMgenEdenSpillAlloc(globalState->heapBase, spillGenSizeBytes);
    if (spillGen == NULL) {
        goto failed_after_spillGen;
    }
    spillGen->generationNo = 1;
    globalState->CVMgenGenerations[1] = spillGen;

    /* Instantiate the oldGen: */
    oldGen = CVMgenMarkCompactAlloc(NULL, oldGenSizeBytes, maxOldGenSizeBytes);
    if (oldGen == NULL) {
        goto failed_after_oldGen;
    }
    oldGen->generationNo = 2;
    globalState->CVMgenGenerations[2] = oldGen;

    /* Link the generations together: */
    edenGen->nextGen = spillGen;
    edenGen->prevGen = NULL;

    spillGen->nextGen = oldGen;
    spillGen->prevGen = edenGen;

    oldGen->nextGen = NULL;
    oldGen->prevGen = spillGen;

    /* Setup the bounds of the young generation in globals: */
    CVMglobals.allocPtrPtr = &edenGen->allocPtr;
    CVMglobals.allocTopPtr = &edenGen->allocTop;

#ifdef CVM_JIT
    CVMglobals.youngGenLowerBound = edenGen->genBase;
    CVMglobals.youngGenUpperBound = edenGen->genTop;
#endif
     
    /*   
     * Initialize the barrier
     */  
    CVMgenClearBarrierTable((CVMGenSegment*)spillGen->allocBase);
    CVMgenClearBarrierTable((CVMGenSegment*)oldGen->allocBase);

    /* Make sure that the edenGen is allocated above the spillGen.  This is
       an assumption that needs to be true in order for
       validYoungerGenerationPtr() to work properly. */
    CVMassert(edenGen->genTop > spillGen->genBase);

    /*
     * Initialize GC times. Let heap initialization be the first
     * major GC.
     */
    globalState->lastMajorGCTime = CVMtimeMillis();

#ifdef CVM_JVMPI
    /* Report the arena info: */
    CVMgcimplPostJVMPIArenaNewEvent();
#endif

    CVMdebugPrintf(("GC[generational]: Auxiliary data structures\n"));
    CVMdebugPrintf(("\theapBase (younger generations)=[0x%x,0x%x)\n",
		    globalState->heapBase,
		    globalState->heapBase + totYoungerBytes / 4));
    CVMtraceMisc(("GC: Initialized heap for generational GC\n"));
    return CVM_TRUE;

failed_after_oldGen:
    CVMgenEdenSpillFree((CVMGenEdenSpillGeneration*)spillGen);
failed_after_spillGen:
    CVMgenEdenFree((CVMGenEdenGeneration*)edenGen);
failed_after_edenGen:
    CVMmemalignFree(globalState->heapBase);
    globalState->heapBase = NULL;
failed:
    /* The caller will signal heap initialization failure */
    return CVM_FALSE;
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
        if (gen->inGeneration(gen, obj)) {
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
    CVMGeneration* spillGen = CVMglobals.gc.CVMgenGenerations[1];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[2];
    return youngGen->freeMemory(youngGen, ee) +
	spillGen->freeMemory(spillGen, ee) +
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
    CVMGeneration* spillGen = CVMglobals.gc.CVMgenGenerations[1];
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[2];
    return youngGen->totalMemory(youngGen, ee) +
	spillGen->totalMemory(spillGen, ee) +
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
        while (oldGen != NULL) {
	    oldGen->scanOlderToYoungerPointers(oldGen, ee,
					   gcOpts, callback, data);

	    /*
	     * Get any allocated pointers in the old generation for which
	     * object headers and card tables have not been updated yet.
	     *
	     * This is going to happen the first time CVMgenScanAllRoots()
	     * is called. The next time, scanOlderToYoungerPointers will
	     * pick all old->young pointers up.  
	     */
	    oldGen->scanPromotedPointers(oldGen, ee, gcOpts, callback, data);

            /* And scan classes in old generation that are pointing to young */
            CVMgenScanClassPointersInGen(oldGen, ee, gcOpts, callback, data);
            oldGen = oldGen->nextGen;
        }
    } else {
	/* Collecting one of the older generations */
	CVMGeneration* youngerGen = thisGen->prevGen;
	CVMGeneration* olderGen = thisGen->nextGen;

        while (youngerGen != NULL) {
	    /* Scan younger to older pointers */
	    youngerGen->scanYoungerToOlderPointers(youngerGen, ee,
						   gcOpts, callback, data);
            youngerGen = youngerGen->prevGen;
        }

        while (olderGen != NULL) {
	    /* Scan older to younger pointers */
	    olderGen->scanOlderToYoungerPointers(olderGen, ee,
						 gcOpts, callback, data);
	    olderGen->scanPromotedPointers(olderGen, ee, gcOpts, callback, data);

            /* And scan classes in old generation that are pointing to young */
            CVMgenScanClassPointersInGen(olderGen, ee, gcOpts, callback, data);

            olderGen = olderGen->nextGen;
        }
    }
    /*
     * And now go through "system" pointers to this generation.
     */
    CVMgcScanRoots(ee, gcOpts, callback, data);
}

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
    CVMGeneration* spillGen;

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
#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    gcOpts.isProfilingPass = CVM_FALSE;
#endif

    /*
     * Do generation-specific initialization
     */
    youngGen = CVMglobals.gc.CVMgenGenerations[0];
    spillGen = CVMglobals.gc.CVMgenGenerations[1];
    oldGen = CVMglobals.gc.CVMgenGenerations[2];

    youngGen->startGC(youngGen, ee, &gcOpts);
    spillGen->startGC(spillGen, ee, &gcOpts);
    oldGen->startGC(oldGen, ee, &gcOpts);

    /* Full collection */
    success = youngGen->collect(youngGen, ee, numBytes, &gcOpts);
    
#ifdef LARGE_OBJECTS_TREATMENT
    /*
     * If we automatically allocate from the old generation
     * for large objects, prime the older generations for the retry
     * logic to follow 
     */
    if (numBytes > LARGE_OBJECT_THRESHOLD) {
        spillGen->collect(spillGen, ee, numBytes, &gcOpts);
	oldGen->collect(oldGen, ee, numBytes, &gcOpts);
	CVMglobals.gc.lastMajorGCTime = CVMtimeMillis();
    }
#endif

    youngGen->endGC(youngGen, ee, &gcOpts);
    spillGen->endGC(spillGen, ee, &gcOpts);
    oldGen->endGC(oldGen, ee, &gcOpts);

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
    CVMObject* allocatedObj;

    /*
     * Re-try in the young generation.
     */
    CVMgenContiguousSpaceAllocate(youngGen, numBytes, allocatedObj);

    if (allocatedObj == NULL) {
        CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[2];
	/* Try hard since we've already GC'ed! */
	allocatedObj = oldGen->allocObject(oldGen, numBytes);
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
    CVMGeneration* oldGen = CVMglobals.gc.CVMgenGenerations[2];
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
    if (numBytes > LARGE_OBJECT_THRESHOLD) {
	allocatedObj = oldGen->allocObject(oldGen, numBytes);
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
CVMgcimplDestroyHeap(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying heap for generational GC\n"));

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaDeleteEvent();
#endif

    CVMgenEdenFree((CVMGenEdenGeneration*)CVMglobals.gc.CVMgenGenerations[0]);
    CVMgenEdenSpillFree((CVMGenEdenSpillGeneration*)CVMglobals.gc.CVMgenGenerations[1]);
    CVMgenMarkCompactFree((CVMGenMarkCompactGeneration*)CVMglobals.gc.CVMgenGenerations[2]);
	
    CVMmemalignFree(globalState->heapBase);
    globalState->heapBase = NULL;

    CVMglobals.gc.CVMgenGenerations[0] = NULL;
    CVMglobals.gc.CVMgenGenerations[1] = NULL;
    CVMglobals.gc.CVMgenGenerations[2] = NULL;

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
    int i;
    /*
     * Iterate over objects in all generations
     */
    for (i = 0; i < CVM_GEN_NUM_GENERATIONS; i++) {
	CVMGeneration* gen  = CVMglobals.gc.CVMgenGenerations[i];
        CVMBool completeScanDone = 
	    gen->iterateGen(gen, ee, callback, callbackData);
        if (!completeScanDone) {
            return CVM_FALSE;
        };
    }
    return CVM_TRUE;
}

#endif /* defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI) */
