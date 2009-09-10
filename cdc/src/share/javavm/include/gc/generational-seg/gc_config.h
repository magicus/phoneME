/*
 * @(#)gc_config.h	1.27 06/10/10
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
 * This file includes the configuration of the generational collector
 */

#ifndef _INCLUDED_GENERATIONAL_GC_CONFIG_H
#define _INCLUDED_GENERATIONAL_GC_CONFIG_H

#include "javavm/include/gc/gc_impl.h"
#include "javavm/include/gc/generational-seg/generational.h"

/*
 * Barriers in this implementation
 */

#define CVMgcimplWriteBarrierRef(directObj, slotAddr, rhsValue)           \
    SET_CARD_TABLE_SLOT_FOR_ADDRESS(directObj, slotAddr, CARD_DIRTY_BYTE);

/* Purpose: Override the Ref arraycopy to do more efficient barrier marking.*/
#define CVMgcimplArrayCopyRef(srcArr, srcIdx, dstArr, dstIdx, len) {	 \
    CVMObject* volatile *slotAddr_;					 \
    CVMObject* volatile *lastAddr_;					 \
									 \
    /* Copy the elements first */					 \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMObject*, Ref);		 \
    /* Batch the write barriers */					 \
    slotAddr_ = (CVMObject* volatile *)					 \
        CVMDprivate_arrayElemLoc((dstArr), (dstIdx));			 \
    lastAddr_ = slotAddr_ + (len) - 1;					 \
    SET_CARD_TABLE_SLOTS_FOR_ADDRESS_RANGE((dstArr), slotAddr_,		 \
                                           lastAddr_, CARD_DIRTY_BYTE);	 \
}

/* 
 * Global state specific to GC 
 */

#define CVM_GEN_NUM_GENERATIONS 3

struct CVMGCGlobalState {
    CVMUint32  heapSize;
    CVMGeneration* CVMgenGenerations[CVM_GEN_NUM_GENERATIONS];
    CVMInt64 lastMajorGCTime;
    CVMGenSummaryTableEntry* summaryTable;
    struct {
	CVMUint32 youngGenSize;
        CVMUint32 middleGenSize;
    } genGCAttributes;
    CVMUint32* heapBase;
};

#define CVM_GCIMPL_GC_OPTIONS \
    "[,youngGen=<youngGenerationSize>][,middleGen=<middleGenerationSize>]"\
    "[,verbose] "

#define CVM_GC_GENERATIONAL 222
#define CVM_GCCHOICE CVM_GC_GENERATIONAL

/* NOTE: The DEFAULT_EDEN_SIZE_BYTES and DEFAULT_SPILL_SIZE_BYTES values are
   also used to determine the ideal ratio of eden to spill generation sizes.

   NOTE: The scratch buffer size has CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD
   subtracted from it so that the sum of spillGenSize + edenGenSize +
   scratchBufferSize doesn't pack to an even increment of
   CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES.  This subtraction allows malloc
   some space for its own overhead without having to internally fragment
   another block of CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES.  This type of
   internal fragmentation will reduce the number of blocks of
   CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES available for use.  That's why we
   avoid the fragmentation.
*/
#define CVM_GCIMPL_DEFAULT_EDEN_SIZE_BYTES          (1024 * 1024) /* 1M */
#define CVM_GCIMPL_DEFAULT_SPILL_SIZE_BYTES         (512 * 1024)  /* 512k */
#define CVM_GCIMPL_DEFAULT_OLD_SIZE_BYTES           (512 * 1024)  /* 512k */
#define CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES          SEGMENT_ALIGNMENT
#define CVM_GCIMPL_SEGMENT_USABLE_BYTES             SEGMENT_USABLE_SIZE
#define CVM_GCIMPL_SEGMENT_OVERHEAD_BYTES           \
    (CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES - CVM_GCIMPL_SEGMENT_USABLE_BYTES)
#define CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD          MAKE_MALLOC_HAPPY
#define CVM_GCIMPL_MIN_EDEN_SIZE_BYTES              (16 * 1024)
#define CVM_GCIMPL_MIN_SPILL_SIZE_BYTES             \
    (1 * CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES)
#define CVM_GCIMPL_MIN_OLD_SIZE_BYTES               \
    (1 * CVM_GCIMPL_SEGMENT_ALIGNMENT_BYTES)
#define CVM_GCIMPL_MIN_TOTAL_HEAP_SIZE_BYTES        \
    (CVM_GCIMPL_MIN_EDEN_SIZE_BYTES + CVM_GCIMPL_MIN_SPILL_SIZE_BYTES + \
     CVM_GCIMPL_MIN_OLD_SIZE_BYTES)

#define CVM_GCIMPL_SCRATCH_BUFFER_SIZE_BYTES        \
    ((64 * 1024) - CVM_GCIMPL_SEGMENT_MALLOC_OVERHEAD)

#endif /* _INCLUDED_GENERATIONAL_GC_CONFIG_H */
