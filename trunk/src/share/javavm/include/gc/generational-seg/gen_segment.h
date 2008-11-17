/*
 * @(#)gen_segment.h	1.11 06/10/10
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



#ifndef _INCLUDE_GEN_SEGMENT_H
#define _INCLUDE_GEN_SEGMENT_H

#define MAKE_MALLOC_HAPPY     96
 /* 64KB  segments*/
#define SEGMENT_ALIGNMENT     (64*1024)
#define SEGMENT_SIZE          (SEGMENT_ALIGNMENT - MAKE_MALLOC_HAPPY)

/* Standard segment size is the following. Assertions make sure that
   all the segment overhead for this size does not make the total
   allocation more than SEGMENT_SIZE */

/* NOTE: CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT is a magic number that is
   pre-calculated based on the SEGMENT_ALIGNMENT value specified.  This
   value is basically computed as:
     cardCount + (sizeof(CVMGenSummaryTableEntry) * cardCount) + cardCount
   where
     usableSpace = SEGMENT_ALIGNMENT - CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT;
     cardCount0 = (usableSpace + NUM_BYTES_PER_CARD - 1) / NUM_BYTES_PER_CARD;
     cardCount = round(cardCount0, sizeof(CVMUint32));

   The resultant number is attained by iterating between values of usableSpace
   and CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT.

   For a SEGMENT_ALIGNMENT of 64k, cardCount turns out to be 128 and
   CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT turns out to be 764 bytes.
*/

#define CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT  764
#define SEGMENT_USABLE_SIZE	\
    (SEGMENT_ALIGNMENT - MAKE_MALLOC_HAPPY \
     - CARD_TABLE_BYTES_FOR_SEGMENT_ALIGNMENT - sizeof(CVMGenSegment))

union CVMGenSummaryTableEntry ;

typedef struct CVMGenSegment {
    /* barrier accesses need to be marked volatile */
    CVMUint8 volatile * cardTableVirtual; /* bottom of virtual card table */
    CVMUint8*  cardTable;        /* the bottom of the card table */
    CVMUint32  cardTableSize;  /* the bottom of the card table */
    CVMUint32  size ;          /* size of the segment including header space*/
    CVMUint32  available ;     /* free mem available for object allocation */
    CVMUint32* allocPtr;       /* Current contiguous allocation pointer */
    CVMUint32* allocBase;      /* The bottom of the allocation area */
    CVMUint32* allocTop;       /* The top of the allocation area */
    CVMUint32* allocMark;      /* An allocation pointer mark (for promotion) */
    CVMUint32* rollbackMark;   /* Mark to which promotions can be rolled back*/
    CVMUint32* cookie;         /* scratch space used while scanning promotions */
    CVMUint32* logMarkStart;   /* Logging purposes */
    CVMUint8   flag ;	       /* Indicates segment state */
    struct CVMGenSegment* nextSeg; /* The next segment after this */
    struct CVMGenSegment* prevSeg; /* The previous segment before this */
    struct CVMGenSegment* nextHotSeg; /* The next segment in the list of hot segments*/

    CVMInt8*   objHeaderTable; /* the bottom of the object header table */
    union CVMGenSummaryTableEntry* summTable; /* the bottom of the summary table */

    CVMUint32  segmentNo; /* The number of this segment */
} CVMGenSegment;

enum {
    SEG_TYPE_NORMAL = 0,
    SEG_TYPE_LOS,
    SEG_TYPE_NORMAL_FREE,
    SEG_TYPE_LOS_FREE
} ;

#ifdef CVM_VERIFY_HEAP
void CheckSegment(CVMGenSegment* segment) ;
#define CHECK_SEGMENT(segment) CheckSegment(segment)
#else
#define CHECK_SEGMENT(segment) ((void)0)
#endif


CVMGenSegment* 
CVMgenAllocSegment(CVMGenSegment** allocBase, 
                   CVMGenSegment** allocTop, 
                   CVMUint32* space,
                   CVMUint32 numBytes,
                   CVMUint8 segType);

void 
CVMgenInsertSegmentAddressOrder(CVMGenSegment* segment, 
                                CVMGenSegment** allocBase, 
                                CVMGenSegment** allocTop) ;
void 
CVMgenRemoveSegment(CVMGenSegment* segment, 
                    CVMGenSegment** allocBase, 
                    CVMGenSegment** allocTop) ;
void 
CVMgenFreeSegment(CVMGenSegment* segment, 
                  CVMGenSegment** allocBase, 
                  CVMGenSegment** allocTop) ;
void 
CVMgenDumpSegments(CVMGenSegment* allocBase) ;

#ifdef CVM_VERIFY_HEAP
void 
verifySegmentsWalk(CVMGenSegment* allocBase) ;
#endif

CVMUint32
CVMgenMaxContiguousSpace(CVMGenSegment* segBase) ;

#endif /*_INCLUDE_GEN_SEGMENT_H*/
