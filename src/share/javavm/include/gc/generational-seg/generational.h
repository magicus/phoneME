/*
 * @(#)generational.h	1.44 06/10/10
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
 * This file is specific and only visible to generational GC
 * implementation.  
 */

#ifndef _INCLUDED_GENERATIONAL_H
#define _INCLUDED_GENERATIONAL_H

#include "javavm/include/gc/generational-seg/gen_segment.h"

/*
 * These are the definitions of generations and spaces. Each
 * generation, when collected, gives you a pointer to the start of a
 * contiguous allocation area.  
 */
typedef struct CVMGenSpace {
    CVMUint32* allocPtr;   /* Current contiguous allocation pointer */
    CVMUint32* allocBase;  /* The bottom of the allocation area */
    CVMUint32* allocTop;   /* The top of the allocation area */
} CVMGenSpace;


typedef struct CVMGeneration {
    CVMUint32* allocPtr;   /* Current contiguous allocation pointer */
    CVMUint32* allocBase;  /* The bottom of the current allocation area */
    CVMUint32* allocTop;   /* The top of the current allocation area */
    CVMUint32* allocMark;  /* An allocation pointer mark (for promotion) */
    CVMUint32* genBase;    /* Lowest address in the generation*/
    CVMUint32* genTop;     /* Highest address in the generation*/
    CVMUint32  generationNo; /* The number of this generation */
    struct CVMGeneration* nextGen; /* The next generation after this */
    struct CVMGeneration* prevGen; /* The previous generation before this */

    /* Collect this generation. TRUE if successful, FALSE otherwise */
    CVMBool    (*collect)(struct CVMGeneration* gen,
			  CVMExecEnv* ee,
			  CVMUint32 numBytes,       /* Goal */
			  CVMGCOptions* gcOpts);  

    /* Iterate over pointers to a younger generation. Frequently used
       during young-space collections, and so should be fast (using
       write barrier information, for example) */
    void       (*scanOlderToYoungerPointers)(struct CVMGeneration* gen,
					     CVMExecEnv* ee,
					     CVMGCOptions* gcOpts,
					     CVMRefCallbackFunc callback,
					     void* callbackData);

    /* Iterate over pointers to an older generation. Used only
       during a full collection, and so does not really need to be fast. */
    void       (*scanYoungerToOlderPointers)(struct CVMGeneration* gen,
					     CVMExecEnv* ee,
					     CVMGCOptions* gcOpts,
					     CVMRefCallbackFunc callback,
					     void* callbackData);

    /* Promote 'objectToPromote' into the current generation. Returns
       the new address of the object, or NULL if the promotion failed. */
    CVMObject* (*promoteInto)(struct CVMGeneration* gen,
			      CVMObject* objectToPromote,
			      CVMUint32  objectSize);

    /* Iterate over promoted pointers in a generation. A young space
       collection accummulates promoted objects in an older
       generation. These objects need to be scanned for pointers into
       the young generation */
    void       (*scanPromotedPointers)(struct CVMGeneration* gen,
				       CVMExecEnv* ee,
				       CVMGCOptions* gcOpts,
				       CVMRefCallbackFunc callback,
				       void* callbackData);

    /* Indicate whether a pointer is from this generation */
    CVMBool    (*inGeneration)(struct CVMGeneration* gen,
			       CVMObject* ref);

    /* Indicate whether a pointer is from younger generation */
    CVMBool    (*inYounger)(struct CVMGeneration* gen,
			    CVMObject* ref);

    /*
     * Get extra, unused space for use by another generation.
     */
    CVMGenSpace* (*getExtraSpace)(struct CVMGeneration* gen);
				  
    /* Indicate to the generation that a GC is about to start, or has
       ended, so it can initialize and destroy its data structures,
       especially those used for inter-generational
       "communication". Information about the kind of GC is encoded in
       the CVMGCOptions passed in. */
    void       (*startGC)(struct CVMGeneration* gen,
			  CVMExecEnv* ee, CVMGCOptions* gcOpts);
    void       (*endGC)(struct CVMGeneration* gen,
			CVMExecEnv* ee, CVMGCOptions* gcOpts);

    /* Get total amount of usable space in this generation */
    CVMUint32  (*totalMemory)(struct CVMGeneration* gen, CVMExecEnv* ee);

    /* Get free amount of usable space in this generation */
    CVMUint32  (*freeMemory)(struct CVMGeneration* gen, CVMExecEnv* ee);

    /* Rollback any object promotions made by a generation into this
       generation */
    void       (*scanAndRollbackPromotions)(struct CVMGeneration* gen, 
				            CVMExecEnv* ee,
				            CVMGCOptions* gcOpts,
				            CVMRefCallbackFunc callback,
				            void* callbackData);
#ifdef CVM_VERIFY_HEAP
    void       (*verifyGen)(struct CVMGeneration* gen, 
                            CVMExecEnv* ee, 
                            CVMGCOptions* gcOpts);
#endif

#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    /* Iterate over objects in generation. Return CVM_FALSE
       if scan not completed. Return CVM_TRUE on success. */
    CVMBool       (*iterateGen)(struct CVMGeneration* gen, 
				CVMExecEnv* ee, 
				CVMObjectCallbackFunc callback,
				void* callbackData);
#endif

    /* Allocates an object of the given size from this generation */
    CVMObject* (*allocObject)(struct CVMGeneration* gen, CVMUint32 numBytes) ;
} CVMGeneration;

/*
 * Evacuated header words in mark-compact based algorithms
 */
/*
 * Header "words" can contain pointers so the evacuation
 * space must be capable to hold pointer size values.
 */
typedef struct CVMGenPreservedItem {
    CVMObject* movedRef;
    CVMAddr    originalWord;
} CVMGenPreservedItem;

/*
 * 0xff terminated list of object offsets
 */
typedef union CVMGenSummaryTableEntry {
    CVMUint8  offsets[4];
    CVMUint32 intVersion; /* For zeroing quickly */
} CVMGenSummaryTableEntry;

#define SEG_FREE_LIMIT          0
#define SEG_SMALL_OBJ_CEIL      SEGMENT_USABLE_SIZE
#define CARD_COUNT              (SEGMENT_SIZE / NUM_BYTES_PER_CARD)

#define SEG_HEAP_OLDGEN_INIT_SIZE  1048576 /* 1MB - default */

/*
 * Some useful generational GC service routines. Particular generations
 * use these either as convenience, or for "communicating" with other
 * generations, say for inter-generational pointer scanning, or object
 * promotion.
 */

/*
 * Scan the root set of collection, as well as all pointers from other
 * generations
 */
void
CVMgenScanAllRoots(CVMGeneration* thisGen,
		   CVMExecEnv *ee, CVMGCOptions* gcOpts,
		   CVMRefCallbackFunc callback, void* data);

/*
 * Update the object headers table for objects in range [startRange, endRange)
 */
void
CVMgenBarrierObjectHeadersUpdate(CVMGeneration* gen, CVMExecEnv* ee,
				 CVMGCOptions* gcOpts,
				 CVMUint32* startRange,
				 CVMUint32* endRange,
                                 CVMGenSegment* segCurr);

/*
 * A contiguous space allocator.
 * Parameters: CVMGeneration* gen, CVMUint32 numBytes
 */
#define CVMgenContiguousSpaceAllocate(gen, numBytes,    \
				      ret)              \
{							\
    CVMUint32* allocPtr = gen->allocPtr;		\
    CVMUint32* allocTop = gen->allocTop;		\
    CVMUint32* allocNext = allocPtr + numBytes / 4;     \
    if ((allocNext <= allocTop) &&                      \
        (allocNext >  allocPtr)) {                      \
	gen->allocPtr = allocNext;			\
	ret = (CVMObject*)allocPtr;			\
    } else {						\
	ret = NULL;					\
    }							\
}

/*
 * Constants.
 *
 * An object is to be promoted if it survives CVM_GEN_PROMOTION_THRESHOLD
 * no of GC's.
 */
#define CVM_GEN_PROMOTION_THRESHOLD 2

/*
 * Write barrier
 */

/*
 * Card table definitions
 */
/*
 * The beginning of the heap space, which the card table maps to.
 */

/* 512 bytes card*/
/* 
 * For 64 bit architectures, the card is too small to hold the expected number 
 * of slots (assertion in gen_segment.c: allocSize < SEGMENT_SIZE), so we
 * have to double the card size here (corresponding to the doubled slot size).
 */
#ifdef CVM_64
#define CVM_GENGC_CARD_SHIFT         10
#else
#define CVM_GENGC_CARD_SHIFT         9
#endif
#define NUM_BYTES_PER_CARD (1 << CVM_GENGC_CARD_SHIFT)

#define NUM_WORDS_PER_CARD (NUM_BYTES_PER_CARD / sizeof(CVMJavaVal32))

/* Round up to card size multiples. */
/* 
 * obj is of type CVMObject*
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 *
 * Cast result to CVMJavaVal32* instead of CVMUint32*,
 * because CVMJavaVal32 is the size of an object slot
 * on 32 and 64 bit machines.
 */
#define ROUND_UP_TO_CARD_SIZE(obj) \
    ((CVMJavaVal32*)((((CVMAddr)(obj)) + (NUM_BYTES_PER_CARD-1)) & ~(NUM_BYTES_PER_CARD-1)))

/* Find the card boundary for object 'obj' */
/* 
 * obj is of type CVMObject*
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 *
 * Cast result to CVMJavaVal32* instead of CVMUint32*,
 * because CVMJavaVal32 is the size of an object slot
 * on 32 and 64 bit machines.
 */
#define CARD_BOUNDARY_FOR(obj) \
    ((CVMJavaVal32*)(((CVMAddr)(obj)) & ~(NUM_BYTES_PER_CARD-1)))

/* Find the next card boundary for object 'obj' */
#define NEXT_CARD_BOUNDARY_FOR(obj) \
    (CARD_BOUNDARY_FOR(obj) + NUM_WORDS_PER_CARD)

#define ROUND_UP_TO_SEGMENT_SIZE(val) \
    (((val) + (SEGMENT_ALIGNMENT-1)) & ~(SEGMENT_ALIGNMENT-1))

#define SEG_HEADER_FOR(obj)  \
    ((CVMGenSegment*)((CVMUint32*)(((CVMAddr)(obj)) & ~(SEGMENT_ALIGNMENT-1))))

#define OBJ_IN_CARD_MARKING_RANGE(obj)					     \
    ((((CVMUint32*)(obj)) < CVMglobals.gc.CVMgenGenerations[0]->genBase) ||  \
     (((CVMUint32*)(obj)) >= CVMglobals.gc.CVMgenGenerations[0]->genTop))

/* Find the card corresponding to heap location 'ptr' */
#define SET_CARD_TABLE_SLOT_FOR_ADDRESS(obj, slot, cardVal)     	\
    if (OBJ_IN_CARD_MARKING_RANGE(obj)) {				\
        CVMGenSegment* segment = SEG_HEADER_FOR(obj);			\
        segment->cardTableVirtual[((CVMAddr)(slot)) >> CVM_GENGC_CARD_SHIFT] = cardVal; 							\
    }

/* Find the card corresponding to heap location 'obj' given the segment header
   much low cost than the above */
#define CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(obj, segment) \
    (&((segment->cardTableVirtual)[((CVMAddr)(obj)) >> CVM_GENGC_CARD_SHIFT]))

/*
 * Mark card table for a range of addresses [slot1, slot2]
 */
#define SET_CARD_TABLE_SLOTS_FOR_ADDRESS_RANGE(obj, slot1, slot2, cardVal) \
    if (OBJ_IN_CARD_MARKING_RANGE(obj)) {				   \
	CVMGenSegment* segment = SEG_HEADER_FOR(obj);			   \
	CVMUint8 volatile * cardPtr;					   \
	CVMUint8 volatile * cardStart =					   \
            CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(slot1, segment);		   \
	CVMUint8 volatile * cardEnd =					   \
            CARD_TABLE_SLOT_ADDRESS_IN_SEG_FOR(slot2, segment);		   \
        CVMassert(cardStart >= segment->cardTable);			   \
        CVMassert(cardEnd < &segment->cardTable[segment->cardTableSize]);  \
	for (cardPtr = cardStart; cardPtr <= cardEnd; cardPtr++) {	   \
	    *cardPtr = cardVal;						   \
	}								   \
    }

/* Find the object header table entry corresponding to a card */
#define HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(crd, segment) \
    ((crd) - ((segment)->cardTable) + (CVMInt8*)segment->objHeaderTable)

/* Find the summary table entry corresponding to a card */
#define SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(crd, segment) \
    (&(((segment)->summTable)[(CVMAddr)((crd) - segment->cardTable)]))

/* Find the heap address corresponding to a card */
/* 
 * Cast result to CVMJavaVal32* instead of CVMUint32*,
 * because CVMJavaVal32 is the size of an object slot
 * on 32 and 64 bit machines.
 */
#define HEAP_ADDRESS_FOR_CARD(crd, segment) \
    (CVMJavaVal32*)(((crd) - ((segment)->cardTable)) * NUM_WORDS_PER_CARD + ((CVMJavaVal32*)(segment)))

/* A card is dirty */
#define CARD_DIRTY_BYTE 1

/* A card is clean */
#define CARD_CLEAN_BYTE 0

/* A card is summarized */
#define CARD_SUMMARIZED_BYTE 2

/* A card is being decayed to clean from summarized */
#define CARD_DECAY_SUMMARIZED_BYTE 3

/* For faster checks, an integer that makes up four clean cards */
#define FOUR_CLEAN_CARDS \
    (((CVMUint32)CARD_CLEAN_BYTE << 24) |    \
     ((CVMUint32)CARD_CLEAN_BYTE << 16) |    \
     ((CVMUint32)CARD_CLEAN_BYTE <<  8) |    \
     ((CVMUint32)CARD_CLEAN_BYTE <<  0))


extern void
CVMgenClearBarrierTable(struct CVMGenSegment* allocBase);

/*
 * Rebuild the barrier table. This can be done either incrementally,
 * from allocMark to allocPtr, or for the entire generation, from
 * allocBase to allocPtr. The 'full' flag controls this.
 */
void
CVMgenBarrierTableRebuild(CVMGeneration* gen, 
			  CVMExecEnv* ee, CVMGCOptions* gcOpts,
			  CVMBool full);

/*
 * Traverse recorded pointers
 */
extern void
CVMgenBarrierPointersTraverse(CVMGeneration* gen, CVMExecEnv* ee,
			      CVMGCOptions* gcOpts,
			      CVMRefCallbackFunc callback,
			      void* callbackData);
/*
 * Traverse objects/pointers in barrier table
 */
void
CVMgenScanBarrierObjects(CVMGeneration* gen, CVMExecEnv* ee,
                         CVMGCOptions* gcOpts,
                         CVMRefCallbackFunc callback,
                         void* callbackData);

#ifdef CVM_VERIFY_HEAP
/*
 * Check whether the card table indicates object 'obj' and slot
 * 'refPtr' is marked correctly.
 */
extern CVMBool CVMgenCardIsLive(CVMGeneration* gen, CVMObject* obj, 
				CVMObject** refPtr);

/*
 * Traverse an object range, and make sure the objects agree with
 * the card table that covers them.
 */
extern void
CVMgenVerifyObjectRangeForCardSanity(CVMGeneration* thisGen,
				     CVMExecEnv* ee, 
				     CVMGCOptions* gcOpts,
				     CVMUint32* start,
				     CVMUint32* top);

/* Verify the integrity of the heap in all generations. */
extern void
CVMgenVerifyHeapIntegrity(CVMExecEnv *ee, CVMGCOptions* gcOpts);

#else

#define CVMgenVerifyHeapIntegrity(ee, gcOpts)

#endif /* CVM_VERIFY_HEAP */

#endif /* _INCLUDED_GENERATIONAL_H */

