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

/*
 * %comment: f010
 */
void
CVMgenClearBarrierTable()
{
    memset(CVMglobals.gc.cardTable, CARD_CLEAN_BYTE, CVMglobals.gc.cardTableSize);
    memset(CVMglobals.gc.objectHeaderTable, 0, CVMglobals.gc.cardTableSize);
}

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
    summEntry->intVersion = 0xffffffff;
    
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
	if ((CVMUint32*)currCbInstance < oldGenLower) {
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
		CVMJavaVal32* ref = (CVMJavaVal32*) *scanPtr;
                /* Only scan youngGen pointers: */
		if ((ref >= youngGenStart) && (ref < youngGenEnd)) {
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
            /* Only scan youngGen pointers: */
	    if ((heapRef >= youngGenStart) && (heapRef < youngGenEnd)) {
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
findObjectStart(CVMUint8* card, CVMJavaVal32* heapAddrForCard)
{
    CVMInt8* hdr = HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(card);
    CVMInt8  n;
    CVMassert(hdr >= CVMglobals.gc.objectHeaderTable);
    CVMassert(hdr <  CVMglobals.gc.objectHeaderTable + CVMglobals.gc.cardTableSize);
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
    if (*card == CARD_SUMMARIZED_BYTE) {
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
	while ((i < 4) && ((offset = summEntry->offsets[i]) != 0xff)) {
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
	CVMassert(i <= 4);
	cardStatsOnly(cStats.cardsSummarized++);
    } else if (*card == CARD_DIRTY_BYTE) {
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
    } else {
	CVMassert(*card == CARD_CLEAN_BYTE);
	cardStatsOnly(cStats.cardsClean++);
    }
		      
}

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
    genHigher       = (CVMJavaVal32*)gen->allocMark;

    /* ... and look at all the cards in [lowerCardLimit,higherCardLimit) */
    lowerCardLimit  = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(genLower);
    higherCardLimit = (CVMUint8*)CARD_TABLE_SLOT_ADDRESS_FOR(genHigher);

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

    /*
     * Now go through the card table in blocks of four for faster
     * zero checks.
     */
    for (cardPtrWord = (CVMUint32*)lowerCardLimit;
	 cardPtrWord < (CVMUint32*)higherCardLimit; ) {

        CVMUint32 word = *cardPtrWord;
        CVMUint32 *start = cardPtrWord;

        /* Since there will tend to be many consecutive clean cards, scan
           through them quickly in a tight loop: */
        while ((cardPtrWord < (CVMUint32*)higherCardLimit) &&
               (word == FOUR_CLEAN_CARDS)) {
            cardPtrWord++;
            word = *cardPtrWord; /* Pre-fetch the next word. */
            cardStatsOnly(cStats.cardsScanned += 4);
            cardStatsOnly(cStats.cardsClean += 4);
	}

        /* When we get here, we may already have scanned through many cards.
           Adjust the heapPtr accordingly: */
        heapPtr += (cardPtrWord - start) * (NUM_WORDS_PER_CARD * 4);

        /* If we get here because we encountered a word that has cards that
           need to be scanned, then scan those cards one at a time.  The
           other reason we may be here is because we have reached the end of
           the region we need to scan.  Hence, we need to do a bounds check
           before we scan that presumed cards in that word. */
 	if ((cardPtrWord < (CVMUint32*)higherCardLimit) && 
            (*cardPtrWord != FOUR_CLEAN_CARDS)) {

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
    CVMscanClassIfNeededConditional(ee, cb, CVMgenInGeneration(gen, instance),
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
		  CVMUint32 minHeapSize,
		  CVMUint32 maxHeapSize)
{
    CVMUint32  heapSize;
    CVMGeneration* youngGen;
    CVMGeneration* oldGen;
    CVMUint32  totBytes; /* Total memory space for all generations */
    CVMUint32  totYoungGenSize; /* Total for both semispaces */
    CVMUint32  roundedSemispaceSize;
    char* youngGenAttr;
    /*
     * Let the minimum total heap size be 1K.
     */
#define MIN_TOTAL_HEAP (1 << 10)

#ifdef CVM_JVMPI
    liveObjectCount = 0;
#endif

    heapSize = maxHeapSize;  /* This implementation cannot expand heap */

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
    }

    /*
     * Parse relevant attributes
     */
    youngGenAttr = CVMgcGetGCAttributeVal("youngGen");

    if (youngGenAttr == NULL) {
	globalState->genGCAttributes.youngGenSize = 1 << 20; /* The default size */
    } else {
	CVMInt32 size = CVMoptionToInt32(youngGenAttr);
	if (size < 0) {
	    /* ignore -- the empty string, or no value at all */
	    size = 1 << 20;
	}
	globalState->genGCAttributes.youngGenSize = size;
    }

    /*
     * At this point, we might find that the total size of the heap is
     * smaller than the stated size of the young generation. Make the
     * heap all young in that case. */

    if (heapSize < globalState->genGCAttributes.youngGenSize) {
	CVMdebugPrintf(("GC[generational]: "
                        "Total heap size %d bytes < "
			"young gen size of %d bytes\n",
			heapSize, globalState->genGCAttributes.youngGenSize));
	CVMdebugPrintf(("\tYoung generation size now %d bytes\n",
			heapSize));
	globalState->genGCAttributes.youngGenSize = heapSize;
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
    if ((globalState->genGCAttributes.youngGenSize < (1 << 20)) &&
	(globalState->genGCAttributes.youngGenSize < heapSize / 8)) {
	CVMdebugPrintf(("GC[generational]: "
			"young gen size of %d bytes < 1M\n",
			globalState->genGCAttributes.youngGenSize));
	CVMdebugPrintf(("\tYoung generation size now %d bytes\n",
			heapSize / 8));
	globalState->genGCAttributes.youngGenSize = heapSize / 8;
    }
    /*
     * %comment 001 
     */
    roundedSemispaceSize = 
	(CVMUint32)(CVMAddr)ROUND_UP_TO_CARD_SIZE(globalState->genGCAttributes.youngGenSize);
    totYoungGenSize = roundedSemispaceSize * 2;

    /* Add the size of one 'wasted' semispace. */
    totBytes = heapSize + roundedSemispaceSize; 

    /*
     * Round up to card size and add a card size for good measure.
     * We're going to need the slack, because we are going to round
     * the heap up to a card boundary.
     */
    totBytes = (CVMUint32)(CVMAddr)ROUND_UP_TO_CARD_SIZE(totBytes) + NUM_BYTES_PER_CARD;

    /*
     * A heap space allocator might be required here, where a porter
     * may have the option of allocating the heap with something other
     * than a malloc.
     */
    globalState->heapBaseMemoryArea = (CVMUint32*)calloc(1, totBytes);
    if (globalState->heapBaseMemoryArea == NULL) {
        goto failed;
    }
    /*
     * Remember the beginning of this space. This is how we are going to
     * map a store into a card table entry.
     */
    globalState->heapBase = (CVMUint32*)
	ROUND_UP_TO_CARD_SIZE(globalState->heapBaseMemoryArea);

    /*
     * Now totBytes is an integral number of cards. Subtract the slack,
     * and you get a safe region that is [heapBase, heapBase+totBytes)
     */
    totBytes -= NUM_BYTES_PER_CARD;

    /*
     * Allocate the extra data structures necessary
     */
    /* The pre-fetch padding is allocated to ensure that the pre-fetching done
       in CVMgenBarrierPointersTraverse() doesn't read an inaccessible region
       of memory. */
#undef PREFETCH_PADDING
#define PREFETCH_PADDING sizeof(CVMUint32)
    globalState->cardTableSize = totBytes / NUM_BYTES_PER_CARD;
    CVMassert(globalState->cardTableSize * NUM_BYTES_PER_CARD == totBytes);
    globalState->cardTable = (CVMUint8*)calloc(1, globalState->cardTableSize +
                                               PREFETCH_PADDING);
    if (globalState->cardTable == NULL) {
        goto failed_after_heapBaseMemoryArea;
    }
    globalState->cardTableEnd = globalState->cardTable + globalState->cardTableSize;
    /*
     * The objectHeaderTable records locations of the nearest object headers
     * for a given card. Its size is the same as the card table.
     */
    globalState->objectHeaderTable = (CVMInt8*)calloc(1, globalState->cardTableSize);
    if (globalState->objectHeaderTable == NULL) {
        goto failed_after_cardTable;
    }
    /*
     * The summaryTable records up to 4 locations of pointers
     * per card. If the card is not dirtied after it is summarized,
     * we can use the summary instead of scanning the card again.
     */
    globalState->summaryTable = 
	(CVMGenSummaryTableEntry*)calloc(globalState->cardTableSize,
					 sizeof(CVMGenSummaryTableEntry));
    if (globalState->summaryTable == NULL) {
	/* The caller will signal heap initialization failure */
        goto failed_after_objectHeaderTable;
    }
    /*
     * Computing this virtual origin for the card table makes marking
     * the dirty byte very cheap.
     */
#ifdef CVM_64
    globalState->cardTableVirtualBase =
	&(globalState->cardTable[-((CVMInt64)(((CVMAddr)globalState->heapBase)/NUM_BYTES_PER_CARD))]);
#else
    globalState->cardTableVirtualBase =
	&(globalState->cardTable[-((CVMInt32)(((CVMAddr)globalState->heapBase)/NUM_BYTES_PER_CARD))]);
#endif
    
    CVMassert((totYoungGenSize & (NUM_BYTES_PER_CARD - 1)) == 0);

    youngGen = CVMgenSemispaceAlloc(globalState->heapBase, totYoungGenSize); 

    if (youngGen == NULL) {
        goto failed_after_summaryTable;
    }

    youngGen->generationNo = 0;
    globalState->CVMgenGenerations[0] = youngGen;

    CVMglobals.allocPtrPtr = &youngGen->allocPtr;
    CVMglobals.allocTopPtr = &youngGen->allocTop;
    
    /*
     * And allocate the desired amount of heap space
     */
    oldGen = CVMgenMarkCompactAlloc(globalState->heapBase + totYoungGenSize / 4,
				    totBytes - totYoungGenSize);

    /*
     * Make sure old space is on a card boundary!
     */
    CVMassert((((CVMAddr)(globalState->heapBase + totYoungGenSize / 4)) & 
	      (NUM_BYTES_PER_CARD-1)) == 0);
    if (oldGen == NULL) {
        goto failed_after_youngGen;
    }

    oldGen->generationNo = 1;
    globalState->CVMgenGenerations[1] = oldGen;

    youngGen->nextGen = oldGen;
    youngGen->prevGen = NULL;

    oldGen->nextGen = NULL;
    oldGen->prevGen = youngGen;

    /*
     * Initialize the barrier
     */
    CVMgenClearBarrierTable();
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
    CVMdebugPrintf(("\theapBaseMemoryArea=[0x%x,0x%x)\n",
		    globalState->heapBaseMemoryArea,
		    globalState->heapBaseMemoryArea + (totBytes + NUM_BYTES_PER_CARD) / 4));
    CVMdebugPrintf(("\tcardTable=[0x%x,0x%x)\n",
		    globalState->cardTable, globalState->cardTable + globalState->cardTableSize));
    CVMdebugPrintf(("\tobjectHeaderTable=[0x%x,0x%x)\n",
		    globalState->objectHeaderTable, globalState->objectHeaderTable + globalState->cardTableSize));
    CVMdebugPrintf(("\tsummaryTable=[0x%x,0x%x)\n",
		    globalState->summaryTable, globalState->summaryTable + globalState->cardTableSize));
    CVMtraceMisc(("GC: Initialized heap for generational GC\n"));
    return CVM_TRUE;

failed_after_youngGen:
    CVMgenSemispaceFree((CVMGenSemispaceGeneration*)youngGen);
failed_after_summaryTable:
    free(globalState->summaryTable);
failed_after_objectHeaderTable:
    free(globalState->objectHeaderTable);
failed_after_cardTable:
    free(globalState->cardTable);
failed_after_heapBaseMemoryArea:
    free(globalState->heapBaseMemoryArea);
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
        /* And scan classes in old generation that are pointing to young */
        CVMgenScanClassPointersInGen(oldGen, ee, gcOpts, callback, data);

    } else {
	/* Collecting old generation */
	CVMGeneration* youngGen = CVMglobals.gc.CVMgenGenerations[0];
	/*
	 * For now, assume one or two generations
	 */
	CVMassert(thisGen->generationNo == 1);

	/* Scan younger to older pointers */
	youngGen->scanYoungerToOlderPointers(youngGen, ee,
					     gcOpts, callback, data);
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
/*#define SOLARIS_TIMING*/
#ifdef SOLARIS_TIMING
#include <sys/time.h>
    static CVMUint32 totGCTime = 0;
    static CVMUint32 totGCCount = 0;
    hrtime_t time;
    CVMUint32 ms;

    time = gethrtime();
#endif /* SOLARIS_TIMING */
    gcOpts.discoverWeakReferences = CVM_FALSE;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
    gcOpts.isProfilingPass = CVM_FALSE;
#endif

    /*
     * Do generation-specific initialization
     */
    youngGen = CVMglobals.gc.CVMgenGenerations[0];
    oldGen = CVMglobals.gc.CVMgenGenerations[1];
    youngGen->startGC(youngGen, ee, &gcOpts);
    oldGen->startGC(oldGen, ee, &gcOpts);

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
    
    /*
     * Try an old-space GC if no success
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

    youngGen->endGC(youngGen, ee, &gcOpts);
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
CVMgcimplDestroyHeap(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying heap for generational GC\n"));

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaDeleteEvent();
#endif

    free(globalState->heapBaseMemoryArea);
    free(globalState->cardTable);
    free(globalState->objectHeaderTable);
    free(globalState->summaryTable);

    globalState->heapBaseMemoryArea = NULL;
    globalState->cardTable = NULL;
    globalState->objectHeaderTable = NULL;
    globalState->summaryTable = NULL;

    CVMgenSemispaceFree((CVMGenSemispaceGeneration*)globalState->CVMgenGenerations[0]);
    CVMgenMarkCompactFree((CVMGenMarkCompactGeneration*)globalState->CVMgenGenerations[1]);
	
    globalState->CVMgenGenerations[0] = NULL;
    globalState->CVMgenGenerations[1] = NULL;

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

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)

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
	CVMUint32*     base = gen->allocBase;
	CVMUint32*     top  = gen->allocPtr;
        CVMBool completeScanDone =
                CVMgcScanObjectRange(ee, base, top, callback, callbackData);
        if (!completeScanDone) {
            return CVM_FALSE;
        };
    }
    return CVM_TRUE;
}

#endif /* defined(CVM_INSPECTOR) || defined(CVM_JVMPI) */
