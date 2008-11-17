/*
 * @(#)jitset.h	1.10 06/10/10
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

#ifndef _INCLUDED_JITSET_H
#define _INCLUDED_JITSET_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"

/* CVMJITset... */

/*
 * A variable size, expandable bit set structure, optimized for 32 elements
 * but can grow to any size. 
 */
typedef struct {
    CVMUint16 numChunks; /* Number of 32-bit used chunks */
    CVMUint16 maxChunks; /* Number of 32-bit allocated chunks */
    CVMUint32  oneChunk[1];
    CVMUint32* chunks;
} CVMJITSet;

/*
 * Create a new, empty set 
 */
extern CVMJITSet*
CVMJITsetNew(CVMJITCompilationContext* con);

/*
 * Initialize a set to be a one-chunk, empty set.
 */
extern void
CVMJITsetInit(CVMJITCompilationContext* con, CVMJITSet* set);

#define CVMJITsetInvalidate(set)		\
	((set)->numChunks = 0)

#define CVMJITsetInitialized(set)		\
	((set)->numChunks != 0)
/*
 * Clear a set to be empty
 */
#define CVMJITsetClear(con, s)			\
{						\
    CVMassert((s)->numChunks > 0);		\
    (s)->chunks[0] = 0;				\
    (s)->numChunks = 1;				\
}

/*
 * Populate set with nElems consecutive items, [0 .. nElems - 1)
 */
#define CVMJITsetPopulate(con, s, nElemsIn)				\
{									\
    CVMUint32 nElems_ = (nElemsIn);					\
    CVMUint32 numChunks = ((nElems_) + 31) / 32;			\
    CVMassert(nElems_ > 0);						\
    if ((s)->maxChunks < numChunks) {					\
	CVMJITsetExpandCapacity((con), (s), numChunks);			\
    }									\
    {									\
	int i;								\
	int numFullChunks = nElems_ / 32;				\
	for (i = 0; i < numFullChunks; i++) {				\
	    (s)->chunks[i] = ~0;					\
	}								\
	{								\
	    int edge = (nElems_) % 32;					\
	    if (edge > 0) {						\
		CVMassert(numFullChunks < numChunks);			\
		(s)->chunks[numFullChunks] = (1 << edge) - 1;		\
	    }								\
	}								\
	(s)->numChunks = numChunks;					\
	CVMassert((s)->numChunks <= (s)->maxChunks);			\
    }									\
}

/*
 * Destroy a set
 */
extern void
CVMJITsetDestroy(CVMJITCompilationContext* con, CVMJITSet* set);

/*
 * Free set allocated by CVMJITmemDestroy
 */
extern void
CVMJITsetRelease(CVMJITCompilationContext* con, CVMJITSet* set);

/*
 * Set intersection: s1 <- (s1 intersect s2)
 */
#define CVMJITsetIntersection(con, s1, s2)			\
{								\
    if (((s1)->numChunks == 1) || 				\
        ((s2)->numChunks == 1)) {				\
	(s1)->chunks[0] &= (s2)->chunks[0];			\
	(s1)->numChunks = 1;					\
    } else {							\
	CVMJITsetIntersectionLong((con), (s1), (s2));		\
    }								\
}
    
/*
 * Set union: s1 <- (s1 union s2)
 */
#define CVMJITsetUnion(con, s1, s2)				\
{								\
    if (((s1)->numChunks == 1) && 				\
        ((s2)->numChunks == 1)) {				\
	(s1)->chunks[0] |= (s2)->chunks[0];			\
    } else {							\
	CVMJITsetUnionLong((con), (s1), (s2));			\
    }								\
}
    
/*
 * Set intersection for large sets. 
 * Don't call directly; call CVMJITsetIntersection instead
 */
extern void
CVMJITsetIntersectionLong(CVMJITCompilationContext* con,
			  CVMJITSet* s1, CVMJITSet* s2);

/*
 * Perform a regular intersection, but report whether the
 * destination actually changed.  This is used for local ref
 * merges.
 */
extern CVMBool
CVMJITsetIntersectionAndNoteChanges(CVMJITCompilationContext* con,
				    CVMJITSet* s1, CVMJITSet* s2);

/*
 * Set union for large sets
 * Don't call directly; call CVMJITsetUnion instead
 */
extern void
CVMJITsetUnionLong(CVMJITCompilationContext* con,
		   CVMJITSet* s1, CVMJITSet* s2);

/* Purpose: Ensures that there the capacity of the bit set is at least
            that of the specified capacity i.e. the set will be able to
            hold elements between 0 and (capacityIn) inclusive. */
#define CVMJITsetEnsureCapacity(con, s, capacityIn)             \
{                                                               \
    CVMUint32 capacity_ = capacityIn;                           \
    CVMUint32 neededChunks_ = (capacity_ / 32) + 1;             \
    if ((s)->maxChunks < neededChunks_) {                       \
        CVMJITsetExpandCapacity((con), (s), 2 * neededChunks_); \
    }                                                           \
}

/* Purpose: Expands the bit set to the specified number of chunks. */
extern void
CVMJITsetExpandCapacity(CVMJITCompilationContext *con,
                        CVMJITSet *s, CVMUint32 newNumChunks);

/*
 * Add element with id 'elemID' to bit set 's' i.e. sets the bit at elemID.
 */
#define CVMJITsetAdd(con, s, elemIDIn)			\
{							\
    CVMUint32 elemID_ = elemIDIn;		        \
    CVMassert((s)->numChunks > 0);			\
    if ((elemID_) <= 31) {				\
	(s)->chunks[0] |= (1 << (elemID_));		\
    } else {						\
	/* do it the hard way */			\
	CVMJITsetAddLong((con), (s), (elemID_));	\
    }							\
}							\

/* Purpose: Ensures adequate capacity in the bit set and sets the bit at the
            specified elemID. */
/* Don't call directly.  Call CVMJITsetAdd() instead. */
extern void
CVMJITsetAddLong(CVMJITCompilationContext* con,
		 CVMJITSet* s, CVMUint32 elemID);

/*
 * Remove element with id 'elemID' from bit set 's' i.e. clears the bit at
 * elemID.
 */
#define CVMJITsetRemove(con, set, elemIDIn)                     \
{								\
    CVMJITSet *set_ = (set);                                    \
    CVMUint32 elemID_ = elemIDIn;		        	\
    CVMassert(set_->numChunks > 0);				\
    {								\
        CVMUint32 chunkId;                                      \
                                                                \
        /* First find which chunk this element belongs to. */   \
        chunkId = elemID_ / 32;                                 \
                                                                \
        /* Clear the bit if there is the capacity for it is     \
           already allocated: */                                \
        if (chunkId < set_->numChunks) {			\
            elemID_ %= 32;                                      \
            set_->chunks[chunkId] &= ~(1 << elemID_);          	\
        }							\
    }								\
}								\

/*
 * Does set 's' contain element with id 'elemID'?
 * Boolean result goes in 'doesIt'.
 */
extern CVMBool
CVMJITsetContainsFunc(CVMJITSet* s, CVMUint32 elem);

#define CVMJITsetContains(s, elemIDIn, doesIt)				\
{									\
    CVMUint32 elemID = elemIDIn;		        		\
    CVMUint32 chunkID = elemID / 32;					\
									\
    CVMassert((s)->numChunks > 0);					\
    if ((s)->numChunks <= chunkID) {					\
        doesIt = CVM_FALSE;						\
    } else {								\
        CVMUint32 pos     = elemID % 32;				\
        CVMUint32 posBits = (1 << pos);					\
	doesIt = (((s)->chunks[chunkID] & posBits) != 0);		\
    }									\
}

/*
 * Test whether the empty set
 */
#define CVMJITsetIsEmpty(con, s, result)				\
{									\
    CVMassert((s)->numChunks > 0);					\
    {									\
        int i;								\
	result = CVM_TRUE;						\
	for (i = 0; i < (s)->numChunks; i++) {				\
	    result &= ((s)->chunks[i] == 0);				\
	}								\
    }									\
}

/*
 * Copy set s2 into set s1
 */
#define CVMJITsetCopy(con, s1, s2)				\
{								\
    if ((s1)->numChunks != (s2)->numChunks) {			\
        CVMJITsetCopyLong((con), (s1), (s2));			\
    } else {							\
	int i;							\
	for (i = 0; i < (s1)->numChunks; i++) {			\
	    (s1)->chunks[i] = (s2)->chunks[i];			\
	}							\
    }								\
}

/*
 * Set copy for problematic sets
 * Don't call directly; call CVMJITsetCopy instead
 */
extern void
CVMJITsetCopyLong(CVMJITCompilationContext* con,
		  CVMJITSet* s1, CVMJITSet* s2);

/*
 * Perform a regular set copy, but limit the size of the
 * destination set.  This is used during local ref merges
 * to "clip" locals that cannot reach the target block.
 */
extern void
CVMJITsetCopyAndClear(CVMJITCompilationContext* con,
		      CVMJITSet* s1, CVMJITSet* s2, int size);


/* Set element iteration. Each integer element causes a callback of
   type CVMJITSetIterator(). */
typedef void (*CVMJITSetIterator)(CVMJITCompilationContext* con,
				  CVMUint32, void*);

/*
 * Macro version of: 
 * extern void
 * CVMJITsetIterate(CVMJITCompilationContext* con,
 *                  CVMJITSet* s, CVMJITSetIterator func, void* data);
 */
#define CVMJITsetIterate(con, s, func, data)				\
{									\
    CVMUint32 val = 0;							\
    CVMUint32 chunk;							\
    									\
    {									\
	CVMUint32* chunkPtr    = (s)->chunks;				\
	CVMUint32* chunkPtrEnd = &((s)->chunks[(s)->numChunks]);	\
        CVMUint32  valSegment  = 0;					\
									\
	while (chunkPtr < chunkPtrEnd) {				\
	    chunk = *chunkPtr++;					\
	    while (chunk != 0) {					\
		if ((chunk & 1) != 0) {					\
		    func((con), (val), (data));				\
		}							\
		val++;							\
		chunk >>= 1;						\
	    }								\
            valSegment += 32;						\
	    val = valSegment; /* Next segment of 32 */			\
	}								\
    }									\
}

/*
 * Debugging aids
 */
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)

extern void
CVMJITsetDumpElements(CVMJITCompilationContext* con, CVMJITSet* s);

#endif

#endif /* _INCLUDED_JITSET_H */
