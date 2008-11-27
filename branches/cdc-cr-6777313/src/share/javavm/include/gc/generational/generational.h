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
 * This file is specific and only visible to generational GC
 * implementation.  
 */

#ifndef _INCLUDED_GENERATIONAL_H
#define _INCLUDED_GENERATIONAL_H

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
    CVMUint32* heapBase;   /* The bottom of the heap area */
    CVMUint32* heapTop;    /* The top of the heap area */
    CVMUint32* allocPtr;   /* Current contiguous allocation pointer */
    CVMUint32* allocBase;  /* The bottom of the allocation area */
    CVMUint32* allocTop;   /* The top of the allocation area */
    CVMUint32* allocMark;  /* An allocation pointer mark (for promotion) */
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

    /*
     * Get extra, unused space for use by another generation.
     */
    CVMGenSpace* (*getExtraSpace)(struct CVMGeneration* gen);

    /* Get total amount of usable space in this generation */
    CVMUint32  (*totalMemory)(struct CVMGeneration* gen, CVMExecEnv* ee);

    /* Get free amount of usable space in this generation */
    CVMUint32  (*freeMemory)(struct CVMGeneration* gen, CVMExecEnv* ee);
} CVMGeneration;

/*
 * Some useful generational GC service routines. Particular generations
 * use these either as convenience, or for "communicating" with other
 * generations, say for inter-generational pointer scanning, or object
 * promotion.
 */

/* Checks to see if the specified object is within the bounds of the specified
   generation. */
CVMBool
CVMgenInGeneration(CVMGeneration *thisGen, CVMObject *ref);
#define CVMgenInGeneration(gen, ref) \
    ((((CVMUint32 *)(ref)) >= ((CVMGeneration*)(gen))->heapBase) && \
     (((CVMUint32 *)(ref)) < ((CVMGeneration*)(gen))->heapTop))

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
				 CVMUint32* endRange);

/*
 * A contiguous space allocator.
 * Parameters: CVMGeneration* gen, CVMUint32 numBytes
 */
#define CVMgenContiguousSpaceAllocate(gen, numBytes,    \
				      ret)              \
{							\
    CVMUint32* allocPtr = gen->allocPtr;		\
    CVMUint32* allocTop = gen->allocTop;		\
    CVMUint32* allocNext = allocPtr + numBytes / 4;	\
    if ((allocNext <= allocTop) &&			\
        (allocNext >  allocPtr)) {			\
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

#define CVM_GEN_SYNTHESIZED_OBJ_MARK ((-1) << CVM_GC_SHIFT)
#define CVMGenObjectIsSynthesized(obj) \
    ((CVMobjectVariousWord(obj) & CVM_GEN_SYNTHESIZED_OBJ_MARK) == \
     CVM_GEN_SYNTHESIZED_OBJ_MARK)

/*
 * Write barrier
 */

/*
 * Card table definitions
 */

/*
 * 0xff terminated list of object offsets
 */
#define CVM_GENGC_SUMMARY_COUNT 4
typedef union CVMGenSummaryTableEntry {
    CVMUint8  offsets[CVM_GENGC_SUMMARY_COUNT];
#if (CVM_GENGC_SUMMARY_COUNT == 4)
    CVMUint32 intVersion; /* For zeroing quickly */
#endif
} CVMGenSummaryTableEntry;

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

/* Find the card corresponding to heap location 'ptr' */
#define CARD_TABLE_SLOT_ADDRESS_FOR(ptr) \
    (&(CVMglobals.gc.cardTableVirtualBase[(CVMAddr)(ptr) / NUM_BYTES_PER_CARD]))

/* Round up to card size multiples. */
#define ROUND_UP_TO_CARD_SIZE(obj) \
    ((CVMJavaVal32*)((((CVMAddr)(obj)) + (NUM_BYTES_PER_CARD-1)) & ~(NUM_BYTES_PER_CARD-1)))

/* Find the card boundary for object 'obj' */
#define CARD_BOUNDARY_FOR(obj) \
    ((CVMJavaVal32*)(((CVMAddr)(obj)) & ~(NUM_BYTES_PER_CARD-1)))

/* Find the next card boundary for object 'obj' */
#define NEXT_CARD_BOUNDARY_FOR(obj) \
    (CARD_BOUNDARY_FOR(obj) + NUM_WORDS_PER_CARD)

#define HEADER_TABLE_SLOT_ADDRESS_FOR_CARD(crd) \
    ((crd) - CVMglobals.gc.cardTable + (CVMInt8*)CVMglobals.gc.objectHeaderTable)

/* Find the summary table entry corresponding to a card */
#define SUMMARY_TABLE_SLOT_ADDRESS_FOR_CARD(crd) \
    (&(CVMglobals.gc.summaryTable[(CVMAddr)((crd) - CVMglobals.gc.cardTable)]))

/* Find the heap address corresponding to a card */
#define HEAP_ADDRESS_FOR_CARD(crd) \
    (CVMJavaVal32*)(((crd) - CVMglobals.gc.cardTable) * NUM_WORDS_PER_CARD + (CVMJavaVal32*)CVMglobals.gc.heapBase)

/* A card is dirty */
#define CARD_DIRTY_BYTE 1

/* A card is clean */
#define CARD_CLEAN_BYTE 0

/* A card is summarized */
#define CARD_SUMMARIZED_BYTE 2

/* A card is special */
#define CARD_SENTINEL_BYTE 3

/* For faster checks, an integer that makes up four clean cards */
#define FOUR_CLEAN_CARDS \
    (((CVMUint32)CARD_CLEAN_BYTE << 24) |    \
     ((CVMUint32)CARD_CLEAN_BYTE << 16) |    \
     ((CVMUint32)CARD_CLEAN_BYTE <<  8) |    \
     ((CVMUint32)CARD_CLEAN_BYTE <<  0))


extern void
CVMgenClearBarrierTable();

/*
 * Traverse recorded pointers
 */
extern void
CVMgenBarrierPointersTraverse(CVMGeneration* gen, CVMExecEnv* ee,
			      CVMGCOptions* gcOpts,
			      CVMRefCallbackFunc callback,
			      void* callbackData);

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the generational GC (in addition to
   the semispace and markcompact dumps). */
extern void CVMgenDumpSysInfo(CVMGCGlobalState* gc);
#endif /* CVM_DEBUG || CVM_INSPECTOR */

#endif /* _INCLUDED_GENERATIONAL_H */

