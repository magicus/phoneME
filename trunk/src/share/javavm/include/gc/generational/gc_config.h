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
 * This file includes the configuration of the generational collector
 */

#ifndef _INCLUDED_GENERATIONAL_GC_CONFIG_H
#define _INCLUDED_GENERATIONAL_GC_CONFIG_H

#include "javavm/include/gc/gc_impl.h"
#include "javavm/include/gc/generational/generational.h"

/*
 * Barriers in this implementation
 */

#define CVMgcimplWriteBarrierRef(directObj, slotAddr, rhsValue)		    \
    /*									    \
     * Indexing off the virtual base makes this very efficient. Otherwise   \
     * we'd have to compute the offset of 'slotAddr' from 'heapBase',	    \
     * before adjusting for card size and indexing cardTable[].		    \
     */									    \
    CVMassert(CARD_TABLE_SLOT_ADDRESS_FOR(slotAddr) >= CVMglobals.gc.cardTable);    \
    CVMassert(CARD_TABLE_SLOT_ADDRESS_FOR(slotAddr) <			    \
	      CVMglobals.gc.cardTable + CVMglobals.gc.cardTableSize);	    \
    *CARD_TABLE_SLOT_ADDRESS_FOR(slotAddr) = CARD_DIRTY_BYTE;

/* Purpose: Override the Ref arraycopy to do more efficient barrier marking.*/
#define CVMgcimplArrayCopyRef(srcArr, srcIdx, dstArr, dstIdx, len) {     \
    CVMObject* volatile *slotAddr_;					 \
    CVMObject* volatile *lastAddr_;					 \
    CVMUint8 volatile *start_, *last_;                                   \
    /* Copy the elements: */                                             \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMObject*, Ref);          \
    /* Mark the write barriers:                                          \
       NOTE: The first array element may start in the middle of a card   \
       region.  Hence, we may not get every card we need if we just mark \
       the start addresses.  After looping over the start addresses,     \
       marking the lastAddress will make sure that we've got every bit   \
       of the written array elements covered.                            \
    */                                                                   \
    slotAddr_ = (CVMObject* volatile *)					 \
        CVMDprivate_arrayElemLoc((dstArr), (dstIdx));			 \
    lastAddr_ = slotAddr_ + (len) - 1;                                   \
    start_ = CARD_TABLE_SLOT_ADDRESS_FOR(slotAddr_);                     \
    last_ = CARD_TABLE_SLOT_ADDRESS_FOR(lastAddr_);                      \
    CVMassert(len == 0 || start_ >= CVMglobals.gc.cardTable);		 \
    CVMassert(len == 0 ||						 \
              start_ < CVMglobals.gc.cardTable + CVMglobals.gc.cardTableSize);\
    CVMassert(len == 0 || last_ >= CVMglobals.gc.cardTable);		 \
    CVMassert(len == 0 ||						 \
              last_ < CVMglobals.gc.cardTable + CVMglobals.gc.cardTableSize);\
    while (start_ <= last_) {						 \
        *start_++ = CARD_DIRTY_BYTE;                                     \
    }                                                                    \
}

/* 
 * Global state specific to GC 
 */

#define CVM_GEN_NUM_GENERATIONS 2

struct CVMGCGlobalState {
    CVMUint32  heapSize;
    CVMGeneration* CVMgenGenerations[CVM_GEN_NUM_GENERATIONS];
    CVMInt64 lastMajorGCTime;
    CVMUint8* cardTable;
    /* references done outside of gc must be volatile */
    CVMUint8 volatile* cardTableVirtualBase;
    CVMUint32 cardTableSize;
    CVMUint8* cardTableEnd;
    struct {
	CVMUint32 youngGenSize;
    } genGCAttributes;
    CVMInt8* objectHeaderTable;
    CVMGenSummaryTableEntry* summaryTable;
    CVMUint32* heapBase;
    CVMUint32* heapBaseMemoryArea;

    /* Runtime state information to optimize GC scans: */
    CVMBool hasYoungGenInternedStrings;
    CVMBool needToScanInternedStrings;
    CVMBool hasYoungGenClassesOrLoaders;
};

#define CVM_GCIMPL_GC_OPTIONS "[,youngGen=<youngSemispaceSize>]"

#define CVM_GC_GENERATIONAL 222
#define CVM_GCCHOICE CVM_GC_GENERATIONAL

#endif /* _INCLUDED_GENERATIONAL_GC_CONFIG_H */
