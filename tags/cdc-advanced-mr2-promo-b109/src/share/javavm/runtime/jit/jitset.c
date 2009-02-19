/*
 * @(#)jitset.c	1.12 06/10/10
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

#include "javavm/include/utils.h"

#include "javavm/include/jit/jitset.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitutils.h"

/*
 * Allocate a new set
 *
 * By default, it only has 1 chunk to accommodate up to 32 elements
 */
CVMJITSet* 
CVMJITsetNew(CVMJITCompilationContext* con)
{
    CVMJITSet* newSet = (CVMJITSet*)CVMJITmemNew(con, JIT_ALLOC_COLLECTIONS, 
						 sizeof(CVMJITSet));
    CVMJITsetInit(con, newSet);
    return newSet;
}

/* Purpose: Initializes the bit set. */
void
CVMJITsetInit(CVMJITCompilationContext* con, CVMJITSet* set)
{
    memset(set, 0, sizeof(CVMJITSet));
    set->chunks = set->oneChunk;
    set->numChunks = 1;
    set->maxChunks = 1;
}
    
/* Purpose: Destructor for the bit set. */
void
CVMJITsetDestroy(CVMJITCompilationContext* con, CVMJITSet* set)
{
    /* Release the existing bit set: */
    CVMJITsetRelease(con, set);
}
    
/* Purpose: Release the contents of the existing bit set. */
void
CVMJITsetRelease(CVMJITCompilationContext* con, CVMJITSet* set)
{
    /* Nothing to do really.  The memory is allocated from the JIT's chunky
       allocator for working memory.  So, don't try to free it here.  However,
       we just reset the bit set in case: */
    CVMJITsetInit(con, set);
}

CVMBool
CVMJITsetIntersectionAndNoteChanges(CVMJITCompilationContext* con, 
				    CVMJITSet* d, CVMJITSet* s)
{
    int i;
    CVMBool changed = CVM_FALSE;

    CVMassert(s->numChunks > 0);

    if (d->numChunks > s->numChunks) {
	d->numChunks = s->numChunks;
    }
    for (i = 0; i < d->numChunks; i++) {
	CVMUint32 old = d->chunks[i];
	CVMUint32 new = old & s->chunks[i];
	if (old != new) {
	    d->chunks[i] = new;
	    changed = CVM_TRUE;
	}
    }
    return changed;
}

void
CVMJITsetIntersectionLong(CVMJITCompilationContext* con, 
			  CVMJITSet* d, CVMJITSet* s)
{
    int i;

    CVMassert(s->numChunks > 0);

    if (d->numChunks > s->numChunks) {
	d->numChunks = s->numChunks;
    }
    for (i = 0; i < d->numChunks; i++) {
	d->chunks[i] &= s->chunks[i];
    }
}

void
CVMJITsetUnionLong(CVMJITCompilationContext* con,
		   CVMJITSet* s1, CVMJITSet* s2)
{
    int i;
    int commonChunks = s1->numChunks < s2->numChunks ?
	s1->numChunks : s2->numChunks;

    if (s1->maxChunks < s2->numChunks) {
	CVMJITsetExpandCapacity(con, s1, s2->numChunks);
    }

    for (i = 0; i < commonChunks; i++) {
	s1->chunks[i] |= s2->chunks[i];
    }
    for (; i < s2->numChunks; i++) {
	s1->chunks[i] = s2->chunks[i];
    }
    s1->numChunks = s2->numChunks;
    CVMassert(s1->numChunks > 0);
}

void
CVMJITsetCopyAndClear(CVMJITCompilationContext* con,
		      CVMJITSet* s1, CVMJITSet* s2, int size)
{
    if (size > 0) {
	int i;
	int maxChunks = (size + 31) / 32;
	int lastChunk = maxChunks - 1;

	if (s2->numChunks < maxChunks) {
	    maxChunks = s2->numChunks;
	}

	if (s1->maxChunks < maxChunks) {
	    CVMJITsetExpandCapacity(con, s1, maxChunks);
	}
	
	for (i = 0; i < maxChunks; i++) {
	    s1->chunks[i] = s2->chunks[i];
	}

	{
	    int edge = size % 32;
	    if (edge > 0 && lastChunk < maxChunks) {
		s1->chunks[lastChunk] &= ((1 << edge) - 1);
	    }
	}

	CVMassert(maxChunks > 0);

	s1->numChunks = maxChunks;
    } else {
	CVMassert(size == 0);
	s1->chunks[0] = 0;
	s1->numChunks = 1;
    }
}

void
CVMJITsetCopyLong(CVMJITCompilationContext* con,
		  CVMJITSet* s1, CVMJITSet* s2)
{
    int i;
    
    if (s1->maxChunks < s2->numChunks) {
	CVMJITsetExpandCapacity(con, s1, s2->numChunks);
    }
    
    for (i = 0; i < s2->numChunks; i++) {
	s1->chunks[i] = s2->chunks[i];
    }
    s1->numChunks = s2->numChunks;
    CVMassert(s1->numChunks > 0);
}

/* Purpose: Expands the bit set to the specified number of chunks. */
void
CVMJITsetExpandCapacity(CVMJITCompilationContext *con,
                        CVMJITSet *s, CVMUint32 newNumChunks)
{
    CVMUint32* source;
    CVMUint32* dest;

    /* Check that we really are expanding */
    CVMassert(newNumChunks > s->maxChunks);
    
    /* We are short. Allocate enough new ones, and copy the old data */
    source = s->chunks;
    /* Leave some extra room */
    dest = CVMJITmemNew(con, JIT_ALLOC_COLLECTIONS,
			newNumChunks * sizeof(CVMUint32));
    /*
     * Copy old data
     */
    memmove((void*)dest, (void*)source, s->numChunks * sizeof(CVMUint32));
    if (s->numChunks > 1) {
	/* memory allocated from a chunky allocator, so don't try to free it */
    }
    s->chunks = dest;
    s->maxChunks = newNumChunks;
}

/* Purpose: Ensures adequate capacity in the bit set and sets the bit at the
            specified elemID. */
/* Don't call directly.  Call CVMJITsetAdd() instead. */
void
CVMJITsetAddLong(CVMJITCompilationContext* con,
		 CVMJITSet* s, CVMUint32 elemID)
{
    CVMUint32 chunkId;

    CVMassert(elemID > 31);
    /* First find which chunk this element belongs to. */
    chunkId = elemID / 32;

    /* Make sure we have adequate capacity: */
    CVMJITsetEnsureCapacity(con, s, elemID);

    if (chunkId >= s->numChunks) {
	int i;
	for (i = s->numChunks; i <= chunkId; ++i) {
	    s->chunks[i] = 0;
	}
	s->numChunks = chunkId + 1;
    }

    CVMassert(s->numChunks <= s->maxChunks);
    CVMassert(s->numChunks > 0);

    /* Now find relative position within chunk, and insert item */
    elemID %= 32;
    s->chunks[chunkId] |= (1 << elemID);
}

#ifdef CVM_DEBUG_ASSERTS
CVMBool
CVMJITsetContainsFunc(CVMJITSet* s, CVMUint32 elem)
{
    CVMBool result;
    CVMJITsetContains(s, elem, result);
    return result;
}
#endif

#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)

static void
iterateChunk(CVMJITCompilationContext* con, 
	     CVMUint32 chunkIn, CVMUint32 initVal,
	     CVMJITSetIterator func, void* data)
{
    CVMUint32 chunk = chunkIn;
    CVMUint32 val   = initVal;
    while(chunk != 0) {
	if ((chunk & 1) != 0) {
	    (*func)(con, val, data);
	}
	val++;
	chunk >>= 1;
    }
}

void
CVMJITsetIterateFunc(CVMJITCompilationContext* con,
		     CVMJITSet* s, CVMJITSetIterator func, void* data)
{
    int i;
    for (i = 0; i < s->numChunks; i++) {
	iterateChunk(con, s->chunks[i], i * 32, func, data);
    }
}

/*
 * Function versions for debugger use
 */
void
CVMJITsetAddFunc(CVMJITCompilationContext* con, CVMJITSet* s, CVMUint32 elem)
{
    CVMJITsetAdd(con, s, elem);
}

void
CVMJITsetRemoveFunc(CVMJITCompilationContext* con,
		    CVMJITSet* s, CVMUint32 elem)
{
    CVMJITsetRemove(con, s, elem);
}

void
CVMJITsetUnionFunc(CVMJITCompilationContext* con,
		   CVMJITSet* s1, CVMJITSet* s2)
{
    CVMJITsetUnion(con, s1, s2);
}

void
CVMJITsetIntersectionFunc(CVMJITCompilationContext* con,
			  CVMJITSet* s1, CVMJITSet* s2)
{
    CVMJITsetIntersection(con, s1, s2);
}

CVMBool
CVMJITsetIsEmptyFunc(CVMJITCompilationContext* con, CVMJITSet* s)
{
    CVMBool result;
    CVMJITsetIsEmpty(con, s, result);
    return result;
}

void
CVMJITsetClearFunc(CVMJITCompilationContext* con, CVMJITSet* s)
{
    CVMJITsetClear(con, s);
}

void
CVMJITsetPopulateFunc(CVMJITCompilationContext* con, 
		      CVMJITSet* s, CVMUint32 nElems)
{
    CVMJITsetPopulate(con, s, nElems);
}

void
CVMJITstackPushFunc(CVMJITCompilationContext* con, CVMJITIRBlock* block,
		    CVMJITStack* stack)
{
    CVMJITstackPush(con, block, stack);
}

CVMJITIRBlock*
CVMJITstackPopFunc(CVMJITCompilationContext* con, CVMJITIRBlock* block,
		    CVMJITStack* stack)
{
    return CVMJITstackPop(con, stack);
}

static void
printChunk(CVMUint32 chunkIn, CVMUint32 initVal, CVMBool verbose)
{
    /* 32-bits of the chunk */
    char binString[33];
    int i;
    CVMUint32 chunk = chunkIn;
    if (verbose) {
	for (i = 0; i < 32; i++) {
	    binString[i] = '0';
	}
	binString[32] = '\0';
    }
    i = 31;
    while(chunk != 0) {
	if ((chunk & 1) != 0) {
	    CVMconsolePrintf("%d ", initVal);
	    binString[i] = '1';
	}
	initVal++;
	i--;
	chunk >>= 1;
    }
    if (verbose) {
	CVMconsolePrintf("\n0x%x [%s]\n", chunkIn, binString);
    }
}

void
CVMJITsetDumpElementsVerbose(CVMJITSet* s, CVMBool verbose)
{
    CVMconsolePrintf("CVMJITSet 0x%x, %d chunks\n", s, s->numChunks);
    {
	int i;
	for (i = 0; i < s->numChunks; i++) {
	    printChunk(s->chunks[i], i * 32, verbose);
	}
    }
    CVMconsolePrintf("\n");
}

static void dumpOne(CVMJITCompilationContext* con,
		    CVMUint32 elem, void* data)
{
    CVMconsolePrintf("%d ", elem);
}

void
CVMJITsetDumpElements(CVMJITCompilationContext* con, CVMJITSet* s)
{
    CVMJITsetIterate(con, s, dumpOne, NULL);
}

/*
 * Test the operations we have defined. Visual inspection to spot problems.
 */
#if 0
void
CVMJITsetTest(CVMJITCompilationContext* con)
{
    int i;
    CVMJITSet* s[4];

    s[0] = CVMJITsetNew(con);
    s[1] = CVMJITsetNew(con);
    s[2] = CVMJITsetNew(con);
    s[3] = CVMJITsetNew(con);

    /* s[0] only short */
    CVMJITsetAdd(con, s[0], 3);
    CVMJITsetAdd(con, s[0], 7);
    CVMJITsetAdd(con, s[0], 13);
    CVMJITsetAdd(con, s[0], 11);

    /* s[1] only short */
    CVMJITsetAdd(con, s[1], 2);
    CVMJITsetAdd(con, s[1], 4);
    CVMJITsetAdd(con, s[1], 6);
    CVMJITsetAdd(con, s[1], 11);

    /* s[2] long */
    CVMJITsetAdd(con, s[2], 1);
    CVMJITsetAdd(con, s[2], 8);
    CVMJITsetAdd(con, s[2], 6);
    CVMJITsetAdd(con, s[2], 13);
    CVMJITsetAdd(con, s[2], 11);
    CVMJITsetAddFunc(con, s[2], 51); /* Use func to avoid compiler silliness */

    /* s[3] really long */
    CVMJITsetAdd(con, s[3], 2);
    CVMJITsetAdd(con, s[3], 9);
    CVMJITsetAddFunc(con, s[3], 53);
    CVMJITsetAddFunc(con, s[3], 145);

    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set 0x%x, numChunks %d:\t", s[i], s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetUnion(con, s[0], s[1]);

    CVMconsolePrintf("After s[0] <- s[0] U s[1]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set 0x%x, numChunks %d:\t", s[i], s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetIntersection(con, s[0], s[1]);

    CVMconsolePrintf("After s[0] <- s[0] intersect s[1]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set s[%d], numChunks %d:\t", i, s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetIntersection(con, s[1], s[2]);

    CVMconsolePrintf("After s[1] <- s[1] intersect s[2]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set s[%d], numChunks %d:\t", i, s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetUnion(con, s[0], s[2]);

    CVMconsolePrintf("After s[0] <- s[0] U s[2]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set s[%d], numChunks %d:\t", i, s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetUnion(con, s[2], s[3]);

    CVMconsolePrintf("After s[2] <- s[2] U s[3]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set s[%d], numChunks %d:\t", i, s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    CVMJITsetIntersection(con, s[2], s[3]);

    CVMconsolePrintf("After s[2] <- s[2] intersect s[3]\n");
    for (i = 0; i < 4; i++) {
	CVMconsolePrintf("Set s[%d], numChunks %d:\t", i, s[i]->numChunks);
	CVMJITsetDumpElements(con, s[i]);
	CVMconsolePrintf("\n");
    }

    for (i = 0; i < 4; i++) {
	CVMJITsetRelease(con, s[i]);
    }
}
#endif

#endif /* CVM_TRACE_JIT */
