/*
 * @(#)gen_segment.c	1.18 06/10/10
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
 * This file includes the implementation of a segmented heap
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"

#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/generational-seg/generational.h"

#include "javavm/include/gc/generational-seg/gen_markcompact.h"

#include "javavm/include/porting/system.h"
#include "javavm/include/porting/memory.h"

void
CVMgenInsertSegmentAddressOrder(CVMGenSegment* segment,
                                CVMGenSegment** allocBase,
                                CVMGenSegment** allocTop) {
    if (*allocBase == NULL) {
        segment->prevSeg = segment->nextSeg = segment;
        *allocBase = *allocTop = segment;
    } else {
        /* Insert the segment at the right place */
 
        if (segment > *allocTop) {
            /* Insert segment at tail of list: */
            segment->nextSeg = (*allocTop)->nextSeg;
            segment->prevSeg = *allocTop;
            (*allocTop)->nextSeg->prevSeg = segment;
            (*allocTop)->nextSeg = segment;
            *allocTop = segment;
        } else if (segment < *allocBase) {
            /* Insert segment at head of list: */
            segment->prevSeg = (*allocBase)->prevSeg;
            segment->nextSeg = *allocBase;
            (*allocBase)->prevSeg->nextSeg = segment;
            (*allocBase)->prevSeg = segment;
            *allocBase = segment;
        } else {
            /* Insert segment in the middle of the list: */
            CVMGenSegment* segCurr = NULL;
            for (segCurr = (*allocTop)->prevSeg;
                 (segment < segCurr) && (segCurr != *allocBase);
                 segCurr = segCurr->prevSeg);
 
            segment->nextSeg = segCurr->nextSeg;
            segCurr->nextSeg = segment;
            segment->prevSeg = segCurr;
            segment->nextSeg->prevSeg = segment;
        }
    }

    return;
}

CVMGenSegment*
CVMgenAllocSegment(CVMGenSegment** allocBase, 
                   CVMGenSegment** allocTop,
                   CVMUint32*      space,
                   CVMUint32       numBytes,
                   CVMUint8        segType)
{
    CVMGenSegment* segment;
    CVMUint32* chunk;
    CVMUint32  cardCount, cardCountRounded;
    CVMUint32  cardSpaceBytes;
    CVMUint32  allocSize;

    /* NOTE: The caller is responsible for passing in a size (i.e. numBytes)
       and segType that is reasonable and correct.  This function will assert
       as many assumptions as possible but this is ultimately the
       responsibility of the caller. */
    CVMassert(numBytes == (CVMUint32)CVMalignDoubleWordUp(numBytes));

    /* For now, we only support segment sizes which are at least of usable
       segment size: */
    CVMassert(numBytes >= SEGMENT_USABLE_SIZE);

    /*
     * First compute how much real space we will require for 'numBytes'
     * bytes of allocatable space.
     *
     * The card table needs to cover the segment data as well. Take
     * that into account.
     */
    allocSize = numBytes + sizeof(CVMGenSegment);
    cardCount = (allocSize + NUM_BYTES_PER_CARD - 1) / NUM_BYTES_PER_CARD;
    /* 
     * CVMalignWordUp now returns a CVMAddr.
     * Just cast it to the type of 'cardCountRounded'.
     */
    cardCountRounded = (CVMUint32)CVMalignWordUp(cardCount);
    cardSpaceBytes = 
	cardCountRounded +   /* cardTable */
	(sizeof(CVMGenSummaryTableEntry)) * cardCount + /* summaryTable */
        cardCountRounded;    /* objHeaderTable */
    allocSize += cardSpaceBytes;

#ifdef CVM_DEBUG_ASSERTS
    if (segType == SEG_TYPE_NORMAL) {
	CVMassert(allocSize <= SEGMENT_ALIGNMENT);
    }
#endif

    if (space == NULL) {
        chunk = CVMmemalignAlloc(SEGMENT_ALIGNMENT, allocSize);
	/*
	 * Sanity check memalign
	 */
	if ((CVMAddr)chunk % SEGMENT_ALIGNMENT != 0) {
	    CVMpanic("CVMmemalignAlloc() returns unaligned storage");
	}
    } else {
	/* 
	 * space is a native pointer so use CVMAddr as cast
	 */
        CVMassert(((CVMAddr)space % SEGMENT_ALIGNMENT) == 0);
        chunk = space;
    }

    if (chunk == NULL) {
        return NULL;
    }

    /** NOTE : Segment header is at head of the chunk **/
    segment = (CVMGenSegment*) chunk;
    memset(segment, 0, sizeof(CVMGenSegment));

    segment->cardTableSize = cardCount;

    segment->allocBase = (CVMUint32*)(segment + 1); /* Skip segment */
    segment->size      = allocSize;  /* Total allocated size */
    segment->available = numBytes;   /* Total usable space */
    segment->allocTop  = segment->allocBase + (numBytes / 4);
    segment->allocPtr  = segment->allocBase; 
    segment->allocMark = segment->allocBase;
    segment->cookie    = segment->allocBase;
    segment->logMarkStart = segment->allocBase;

    /* The card table, summary table, and objHeaderTable follow
       the allocatable part */
    segment->cardTable = (CVMUint8*)segment->allocTop;
    /* The initial state of the card is clean */
    memset(segment->cardTable, CARD_CLEAN_BYTE, cardCountRounded);

    /* Make sure the summary table starts on a word boundary */
    segment->summTable = (CVMGenSummaryTableEntry*)(segment->cardTable + 
						    cardCountRounded);
    segment->objHeaderTable = (CVMInt8*) (segment->summTable + cardCount);
    /* Make sure there are no overruns */
    CVMassert((CVMUint8*)(segment->objHeaderTable + cardCount) <=
	      ((CVMUint8*)chunk + allocSize));

    /* 
     * make CVM ready to run on 64 bit platforms
     *
     * globalState->heapBase is a native pointer CVMUint32* 
     * (see struct CVMGCGlobalState in gc_config.h)
     * therefore the cast has to be CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     *
     * ...and the outer cast is obviously only there to convert
     * the unsigned expression into a signed expression so that
     * a unary minus can be applied. So we don't wand to lose
     * any address bits here as well.
     * Since we don't have a typedef for a signed type depending
     * on the platform address size I'll use an #ifdef.
     */
#ifdef CVM_64
    segment->cardTableVirtual = 
        &segment->cardTable[-((CVMInt64)
			      (((CVMAddr)segment) / NUM_BYTES_PER_CARD))];
#else
    segment->cardTableVirtual = 
        &segment->cardTable[-((CVMInt32)
			      (((CVMAddr)segment) / NUM_BYTES_PER_CARD))];
#endif
    segment->flag = segType;

    /* Number the segments for debugging */
    if (*allocBase == NULL) {
        segment->segmentNo = 1;
    } else {
        segment->segmentNo = (*allocTop)->segmentNo + 1;
    }

    CVMgenInsertSegmentAddressOrder(segment, allocBase, allocTop);
    CHECK_SEGMENT(segment);

#if 0
    CVMdebugPrintf(("GC[MC] : Created Segment No. %d of size=%d available=%d range [%x,%x)\n", segment->segmentNo, segment->size, segment->available, segment->allocBase, segment->allocTop));
#endif
    
    return segment;
}

void
CVMgenRemoveSegment(CVMGenSegment* segment, 
                    CVMGenSegment** allocBase, 
                    CVMGenSegment** allocTop) {

    CVMassert(allocTop != NULL && allocBase != NULL &&
              *allocTop != NULL && *allocTop != NULL);
 
    CHECK_SEGMENT(segment);
 
    segment->prevSeg->nextSeg = segment->nextSeg;
    segment->nextSeg->prevSeg = segment->prevSeg;
 
    segment->nextSeg = NULL;
    segment->prevSeg = NULL;
 
    if (*allocBase == *allocTop) {
        *allocBase = *allocTop = NULL;
    } else if (segment == *allocBase) {
        *allocBase = (*allocTop)->nextSeg;
    } else if (segment == *allocTop) {
        *allocTop = (*allocBase)->prevSeg;
    }
 
#ifdef CVM_DEBUG_ASSERTS
    if (*allocTop != NULL && *allocBase != NULL) {
        CVMassert((*allocTop)->prevSeg != NULL);
        CVMassert((*allocTop)->nextSeg == *allocBase);
        CVMassert((*allocBase)->nextSeg != NULL);
        CVMassert((*allocBase)->prevSeg == *allocTop);
    }
#endif

    return;
}

void
CVMgenFreeSegment(CVMGenSegment* segment, 
                  CVMGenSegment** allocBase, 
                  CVMGenSegment** allocTop) {

    if (allocBase != NULL && allocTop != NULL) {
        CVMgenRemoveSegment(segment, allocBase, allocTop);
    }

    CVMmemalignFree(segment);
    return;
}

#ifdef CVM_DEBUG
void
CVMgenDumpSegments(CVMGenSegment* segBase) {

    CVMGenSegment* segCurr = segBase;
    while (segCurr != NULL) {
        CHECK_SEGMENT(segCurr);
        CVMdebugPrintf(("GC[MC] : Segment %d of size=%d free=%d range [%x,%x)\n", segCurr->segmentNo, segCurr->size, segCurr->available, segCurr->allocBase, segCurr->allocTop));
        segCurr = segCurr->nextSeg;
        CVMassert(segCurr != NULL);
        if(segCurr == segBase) 
            break;
    }
}
void
CVMgenDumpHotSegments(CVMGenSegment* segBase) {

    CVMGenSegment* segCurr = segBase;
    while (segCurr != NULL) {
        CHECK_SEGMENT(segCurr);
        CVMdebugPrintf(("GC[MC] : Segment %d of size=%d free=%d range [%x,%x)\n", segCurr->segmentNo, segCurr->size, segCurr->available, segCurr->allocBase, segCurr->allocTop));
        segCurr = segCurr->nextHotSeg;
        CVMassert(segCurr != NULL);
        if(segCurr == segBase)
            break;
    }
}
#endif /* CVM_DEBUG */

#ifdef CVM_VERIFY_HEAP
void
verifySegmentsWalk(CVMGenSegment* segBase) {

    CVMGenSegment* segCurr = segBase;
    while (segCurr != NULL) {
        CHECK_SEGMENT(segCurr);
        segCurr = segCurr->nextSeg;
        CVMassert(segCurr != NULL);
        if(segCurr == segBase) 
            break;
    }
}
#endif /* CVM_VERIFY_HEAP */

#ifdef CVM_VERIFY_HEAP
void 
CheckSegment(CVMGenSegment* segment) {
    int cardCount = segment->cardTableSize;

    CVMassert(segment && segment->allocBase && segment->allocTop && segment->allocMark);
    CVMassert(segment->nextSeg && segment->prevSeg);
 
    if (segment->flag == SEG_TYPE_NORMAL) {
        CVMassert(segment->size == SEGMENT_SIZE);
        CVMassert(segment->cardTableSize == 
		  (CVMalignDoubleWordUp(SEGMENT_USABLE_SIZE)+
		   sizeof(CVMGenSegment)+
		   NUM_BYTES_PER_CARD-1)/
		  NUM_BYTES_PER_CARD);
    }

    CVMassert(segment->allocPtr >= segment->allocBase && segment->allocPtr <= segment->allocTop);
    CVMassert(segment->allocMark >= segment->allocBase && segment->allocMark <= segment->allocPtr);
    CVMassert(segment->available >= 0 && segment->available <= (segment->allocTop - segment->allocBase)*4);
    if (segment->prevSeg->flag == SEG_TYPE_NORMAL) {
        CVMassert(segment->prevSeg->size == SEGMENT_SIZE);
    }
    if (segment->nextSeg->flag == SEG_TYPE_NORMAL) {
        CVMassert(segment->nextSeg->size == SEGMENT_SIZE);
    }
 
    CVMassert(segment->cardTable == (CVMUint8*)segment->allocTop);
    CVMassert(segment->summTable == (CVMGenSummaryTableEntry*)(segment->cardTable +
                                      CVMalignWordUp(cardCount)));
    CVMassert(segment->objHeaderTable == (CVMInt8*)(segment->summTable + cardCount));

    CVMassert((CVMUint8*)segment->summTable - segment->cardTable == CVMalignWordUp(cardCount));
    CVMassert((CVMGenSummaryTableEntry*)segment->objHeaderTable - segment->summTable == cardCount);

    return;
}
#endif /* CVM_VERIFY_HEAP */
