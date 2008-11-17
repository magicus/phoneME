/*
 * @(#)gc_impl.c	1.51 06/10/10
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
 * This file includes the implementation of a simple,
 * free list based mark-sweep allocator/GC.
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

#include "javavm/include/gc/marksweep/marksweep.h"

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/*
 * Define to get tracing output
 */
/*#define CVM_MS_TRACE*/

/*
 * Define to GC on every n-th allocation try
 */
/*#define CVM_MS_GC_ON_EVERY_ALLOC*/
#define CVM_MS_NO_ALLOCS_UNTIL_GC 1000

/*
 * Time of the last major GC
 */
static CVMInt64 lastMajorGCTime;

/*
 * Explicit object scan buffer. Let's start with 1000 objects buffer.
 * We can expand when needed.
 */
#define CVM_MS_INITIAL_SIZEOF_TODO_LIST 100000
static CVMUint32   CVMmsTodoSize;
static CVMUint32   CVMmsTodoIdx;
static CVMObject** CVMmsTodoList;

/*
 * A quick way to get to the heap
 */
static CVMMsHeap* theHeap = 0;

static CVMGCOptions* currGcOpts;

/*
 * CVM_MS_NFREELISTS linear size classes. List i covers sizes
 * [i, i+1) * CVM_MS_SIZEOFBIN_INBYTES (0 =< i < CVM_MS_NFREELISTS)
 *
 * Bin number CVM_MS_NFREELISTS covers all sizes 
 * that are >= CVM_MS_NFREELISTS * CVM_MS_SIZEOFBIN_INBYTES
 */
#define CVM_MS_NFREELISTS        8
#define CVM_MS_SIZEOFBIN_SHIFT   5
#define CVM_MS_SIZEOFBIN_INBYTES (1 << CVM_MS_SIZEOFBIN_SHIFT)

/*
 * The format of a free block in a free list
 */
typedef struct CVMMsFreeBlock_t {
    CVMUint32                blockSize;  /* Size of this block in bytes */
    struct CVMMsFreeBlock_t* nextFree;   /* Pointer to the next block   */
} CVMMsFreeBlock;

static CVMMsFreeBlock* CVMmsFreeLists[CVM_MS_NFREELISTS+1];

#ifdef CVM_JVMPI
/* The number of live objects at the end of the GC cycle: */
static CVMUint32 liveObjectCount;
#endif

static CVMUint32 freeBytes;

/*
 * Forward declaration
 */
static void
CVMmsRootMarkTransitively(CVMObject** root, void* data);

/*
 * Initialize GC global state
 */
void
CVMgcimplInitGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("GC: Initializing global state for mark-sweep GC\n"));
    theHeap = 0;
}

/*
 * Get the first list number that qualifies for objects of size
 * numBytes.  
 */
static CVMUint32
CVMmsGetFirstListFor(CVMUint32 numBytes)
{
    CVMUint32 binNo = numBytes >> CVM_MS_SIZEOFBIN_SHIFT;
    if (binNo >= CVM_MS_NFREELISTS) {
	binNo = CVM_MS_NFREELISTS;
    }
    return binNo;
}

/*
 * Add a memory block to an appropriate free list.
 * Bash the object's contents in the debug case
 *
 * Keep the lists address-ordered in order to reduce fragmentation.
 */
static void
CVMmsAddToFreeList(CVMUint32* block, CVMUint32 numBytes, CVMUint32 listNo)
{
    CVMMsFreeBlock* newBlock  = (CVMMsFreeBlock*)block;

    CVMMsFreeBlock* chunk     = CVMmsFreeLists[listNo];
    CVMMsFreeBlock* prevChunk = 0;
    /*
     * We are going to link the new block in an address-ordered
     * free list. Find insertion point.
     */
    while ((chunk != 0) && (chunk < newBlock)) {
	prevChunk = chunk;
	chunk = chunk->nextFree;
    }
    /*
     * Insert in the right position
     */
    if (prevChunk == 0) {
	CVMmsFreeLists[listNo] = newBlock;
    } else {
	prevChunk->nextFree = newBlock;
    }
    newBlock->blockSize    = numBytes;
    newBlock->nextFree     = chunk;

#ifdef CVM_DEBUG_XXX
    /*
     * Now let's paint this bright glowing red!
     */
    memset(newBlock+1, 0x55, numBytes - sizeof(CVMMsFreeBlock));
#endif
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
    CVMMsHeap* heap  = (CVMMsHeap*)calloc(sizeof(CVMMsHeap), 1);
    CVMUint32 heapSize = maxHeapSize;
    int i;

    if (heap == 0) {
	return CVM_FALSE;
    }
    /*
     * We want to be sweeping from the top of the heap to the bottom
     * So we don't want the bitmaps arrays to have any redundant bits.
     */
    heapSize = (heapSize + 127) & ~127;

    /* The area for the objects */
    heap->size             = heapSize;
    heap->data             = (CVMUint32*)calloc(heapSize, 1);
    
    if (heap->data == 0) {
	free(heap);
	return CVM_FALSE;
    }
    /* The following need a bit per word allocated */
    heap->sizeOfBitmaps    = (heapSize + 31) / 32;
    heap->boundaries       = (CVMUint32*)calloc(heap->sizeOfBitmaps, 1);
    heap->markbits         = (CVMUint32*)calloc(heap->sizeOfBitmaps, 1);

    if (heap->boundaries == 0 || heap->markbits == 0) {
	free(heap->data);
	free(heap->boundaries);
	free(heap->markbits);
	free(heap);
	return CVM_FALSE;
    }
    /*
     * Initialize heap. All the free space ends up in a list
     */
    for (i = 0; i <= CVM_MS_NFREELISTS; i++) {
	CVMmsFreeLists[i] = 0;
    }
    CVMmsAddToFreeList(heap->data, heapSize, CVMmsGetFirstListFor(heapSize));

    /*
     * Initialization for iterative scanning
     */
    CVMmsTodoSize = CVM_MS_INITIAL_SIZEOF_TODO_LIST;
    CVMmsTodoIdx  = 0; /* nothing queued up yet */
    CVMmsTodoList = calloc(CVMmsTodoSize, sizeof(CVMObject*));

    if (CVMmsTodoList == 0) {
	free(heap->data);
	free(heap->boundaries);
	free(heap->markbits);
	free(heap);
	return CVM_FALSE;
    }

#ifdef CVM_JVMPI
    liveObjectCount = 0;
#endif

    /*
     * Now make the heap visible to the outside world.
     */
    globalState->heap = heap;
    theHeap = heap;
    freeBytes = heapSize;

    /*
     * Initialize GC times. Let heap initialization be the first
     * major GC.
     */
    lastMajorGCTime = CVMtimeMillis();

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaNewEvent();
#endif

    CVMtraceMisc(("GC: Initialized heap for mark-sweep GC\n"));
    return CVM_TRUE;
}

/*
 * Clear mark bits before starting GC
 */
static void
CVMmsClearMarks()
{
    memset(theHeap->markbits, 0, theHeap->sizeOfBitmaps);
}

/*
 * The 'markbits' and 'boundaries' arrays map each allocated word
 * to a bit.
 *
 * For a given "byte index" byteIdx_ in the heap, CVM_MS_BITMAP_IDX
 * finds the corresponding bitmap word for byteIdx_, and
 * CVM_MS_BITMAP_MASK computes the appropriate bit mask.
 */

#define CVM_MS_BITMAP_IDX(byteIdx_)  ((byteIdx_) >> 7)
#define CVM_MS_BITMAP_MASK(byteIdx_) (1 << (((byteIdx_) & 0x7f) / 4))

static CVMBool
CVMmsObjectIsInROM(CVMObject* ref)
{
    return ((((CVMUint32)(CVMobjectGetClassWord(ref))) &
	     CVM_OBJECT_IN_ROM_MASK) != 0);
}

/*
 * Set bit corresponding to heap addr 'addr' in the bits array 
 * (which could be part of the boundaries array or markbits array)
 */
static void
CVMmsSetBitFor(CVMUint32* heapPtr, CVMUint32* bitmap)
{
    CVMUint32 byteIdx;

    CVMassert((heapPtr >= theHeap->data) &&
	      (heapPtr < &theHeap->data[theHeap->size / 4]));

    byteIdx = (CVMUint8*)heapPtr - (CVMUint8*)theHeap->data;
    bitmap[CVM_MS_BITMAP_IDX(byteIdx)] |= CVM_MS_BITMAP_MASK(byteIdx);
}

/*
 * Check if bit corresponding to heap addr 'addr' in a bits array is marked.
 */
static CVMBool
CVMmsMarkedIn(CVMUint32* heapPtr, CVMUint32* bitmap)
{
    CVMUint32 byteIdx;

    CVMassert((heapPtr >= theHeap->data) &&
	      (heapPtr < &theHeap->data[theHeap->size / 4]));

    byteIdx = (CVMUint8*)heapPtr - (CVMUint8*)theHeap->data;
    return ((bitmap[CVM_MS_BITMAP_IDX(byteIdx)] &
	     CVM_MS_BITMAP_MASK(byteIdx)) != 0);
}

static CVMObject*
CVMmsTryAlloc(CVMUint32 numBytes)
{
    /*
     * Pick the first candidate bin for allocation
     */
    CVMUint32       listNo = CVMmsGetFirstListFor(numBytes);
    CVMMsFreeBlock* chunk;
    CVMMsFreeBlock* prevChunk;
    CVMMsFreeBlock* bestFit;
    CVMMsFreeBlock* bestPrevChunk;
    CVMUint32       fitDiff;
#ifdef CVM_MS_TRACE
    /*
     * Some accounting
     */
    CVMUint32 noCoalesced; /* No of adjacent free blocks coalesced */
    CVMUint32 noIter;      /* No iterations for best fit */
#endif

#ifdef CVM_MS_TRACE
    CVMconsolePrintf("GC: tryAlloc(%d), size class=%d\n",
		     numBytes, listNo);
#endif
    /*
     * Try first fit allocation from this bin and onward for all size
     * classes. The last bin is special!
     */
    while (listNo < CVM_MS_NFREELISTS) {
	chunk = CVMmsFreeLists[listNo];
	prevChunk = 0;
	/*
	 * Find the first chunk that qualifies 
	 */
	while ((chunk != 0) && (chunk->blockSize < numBytes)) {
	    prevChunk = chunk;
	    chunk = chunk->nextFree;
	}
	/*
	 * Did we find an eligible free chunk?
	 */
	if (chunk != 0) {
	    char* allocatedArea = (char*)chunk;
	    CVMInt32 fitDiff;
	    /*
	     * Unlink the free chunk we are going to use
	     */
	    if (prevChunk == 0) {
		/* The first free block was the one */
		CVMmsFreeLists[listNo] = chunk->nextFree;
	    } else {
		/* Unlink the free block from the free list */
		prevChunk->nextFree = chunk->nextFree;
	    }
	    fitDiff = chunk->blockSize - numBytes;
	    /*
	     * If the remainder chunk is big enough, put it back into
	     * a free list.
	     *
	     * We might have to waste the remainder chunk if it is
	     * smaller than one free block header size.
	     *
	     * %comment f006
	     */
	    if (fitDiff >= sizeof(CVMMsFreeBlock)) {
		/* Take the remainder and put it in a free list */
		CVMmsAddToFreeList((CVMUint32*)(&allocatedArea[numBytes]),
				   fitDiff, CVMmsGetFirstListFor(fitDiff));
	    }
	    /* This area has now been allocated. Mark it so. */
	    CVMmsSetBitFor((CVMUint32*)allocatedArea, theHeap->boundaries);
#ifdef CVM_MS_TRACE
	    CVMconsolePrintf("GC: tryAlloc(%d) OK from list %d -> 0x%x\n",
			     numBytes, listNo, allocatedArea);
#endif
	    freeBytes -= numBytes;
	    return (CVMObject*)allocatedArea;
	}
	listNo++;
    }

#ifdef CVM_MS_TRACE
    CVMconsolePrintf("GC: tryAlloc(%d) free list search not OK, "
		     "trying best fit\n", numBytes);
#endif
    /*
     * Either we are trying to allocate a large object or we couldn't
     * allocate from any of the free lists. In that case, we will look
     * in the big free list.
     *
     * While searching in this one, we coalesce adjacent free blocks,
     * and look for a best fit instead of a first fit.  
     */
    chunk = CVMmsFreeLists[CVM_MS_NFREELISTS];
    prevChunk = 0;
    bestFit = 0;
    bestPrevChunk = 0;
    fitDiff = ~0; /* Initial distance from best fit */

    /*
     * Find a best fit chunk and coalesce adjacent ones while searching.
     */
#ifdef CVM_MS_TRACE
    noIter = 0;
#endif
    while (chunk != 0) {
	/*
	 * First coalesce all adjacent free blocks.
	 */
	CVMUint32 newSize = chunk->blockSize;
	CVMUint32 oldSize = newSize; /* to see if anything has changed */
	CVMMsFreeBlock* nextBlock = chunk->nextFree;
#ifdef CVM_MS_TRACE
	noCoalesced = 0; /* How many blocks did we coalesce */
	noIter++;        /* No of best-fit iterations */
#endif
	while ((char*)chunk + newSize == (char*)nextBlock) { /* adjacency */
	    newSize += nextBlock->blockSize;
	    nextBlock = nextBlock->nextFree;
#ifdef CVM_MS_TRACE
	    noCoalesced++;
#endif
	}
	if (oldSize != newSize) {
	    chunk->blockSize = newSize;
	    chunk->nextFree = nextBlock;
#ifdef CVM_DEBUG_XXX
	    /* Destroy all traces of the old blocks, the lazy and sure way.
	     * (the non-lazy way would be to wipe out the headers while
	     * coalescing).
	     */
	    memset(chunk+1, 0x55, newSize - sizeof(CVMMsFreeBlock));
#endif
#ifdef CVM_MS_TRACE
	    CVMconsolePrintf("GC: Coalesced %d free chunks 0x%x "
			     "from size %d to size %d\n",
			     noCoalesced, chunk, oldSize, newSize);
#endif
	}
	/*
	 * Now see if we have a better fit than the previous best fit.
	 *
	 * Don't stop at the exact fit -- we would like to coalesce
	 * as much as possible, and so we'll look at all the list.
	 */
	if ((newSize >= numBytes) && (newSize - numBytes < fitDiff)) {
	    bestFit = chunk;
	    bestPrevChunk = prevChunk;
	    fitDiff = newSize - numBytes;
	}
	prevChunk = chunk;
	chunk = nextBlock;
    }
    /*
     * Did we find an eligible free chunk?
     */
    if (bestFit != 0) { /* Covers the case of the empty list */
	char* allocatedArea = (char*)bestFit;
	/*
	 * Unlink the free chunk we are going to use
	 */
	if (bestPrevChunk == 0) {
	    /* The first free block was the one */
	    CVMmsFreeLists[listNo] = bestFit->nextFree;
	} else {
	    /* Unlink the free block from the free list */
	    bestPrevChunk->nextFree = bestFit->nextFree;
	}
	/*
	 * If the remainder chunk is big enough, put it back into
	 * a free list.
	 *
	 * We might have to waste the remainder chunk if it is
	 * smaller than one free block header size.
	 *
	 * %comment: f006
	 */
	if (fitDiff >= sizeof(CVMMsFreeBlock)) {
	    /* Take the remainder and put it in a free list */
	    CVMmsAddToFreeList((CVMUint32*)(&allocatedArea[numBytes]),
			       fitDiff, CVMmsGetFirstListFor(fitDiff));
	}
	/* This area has now been allocated. Mark it so. */
	CVMmsSetBitFor((CVMUint32*)allocatedArea, theHeap->boundaries);
#ifdef CVM_MS_TRACE
	CVMconsolePrintf("GC: tryAlloc(%d) (best fit) OK in %d iterations"
			 " -> 0x%x\n",
			 numBytes, noIter, allocatedArea);
#endif
	freeBytes -= numBytes;
	return (CVMObject*)allocatedArea;
    }

#ifdef CVM_MS_TRACE
    CVMconsolePrintf("GC: tryAlloc(%d) failed!\n", numBytes);
#endif
    return 0; /* No luck */
}

/*
 * The following will collapse all small linked lists and join them
 * with the big one. This operations is performed as a last resort
 * when allocation has failed, even after a GC.  
 */
static void
CVMmsCollapseFreeLists()
{
    CVMUint32       listNo = 0;
    CVMMsFreeBlock *chunk;
    
#ifdef CVM_MS_TRACE
    CVMconsolePrintf("GC: Collapsing free lists\n");
#endif
    /*
     * Take each of the free lists and put them in the big list.
     */
    for (listNo = 0; listNo < CVM_MS_NFREELISTS; listNo++) {
	chunk = CVMmsFreeLists[listNo];
	if (chunk != 0) {
	    CVMmsFreeLists[listNo] = 0;
	    while (chunk != 0) {
		/*
		 * Add to last bin. The next allocation will
		 * coalesce all adjacent free blocks anyway.
		 */
		CVMmsAddToFreeList((CVMUint32 *) chunk, chunk->blockSize,
				   CVM_MS_NFREELISTS);
		chunk = chunk->nextFree;
	    }
	}
    }
}

static void
CVMmsSweep()
{
    /*
     * Iterate over the boundaries and marks arrays, and figure out
     * who's live and who's dead. Reclaim the dead.
     */
    CVMUint32* marksBegin = theHeap->markbits;
    CVMUint32* marks = &theHeap->markbits[theHeap->sizeOfBitmaps/4] - 1;

    CVMUint32* boundaries = &theHeap->boundaries[theHeap->sizeOfBitmaps/4] - 1;
    CVMUint32* heapPtr = &theHeap->data[theHeap->size/4] - 1;

    CVMUint32* otherHeapPtr = heapPtr;

    while (marks >= marksBegin) {
	CVMUint32 currBoundaries = *boundaries;

	/*
	 * Don't bother looking unless there are allocated objects
	 * in this 32-word segment
	 */
	if (currBoundaries != 0) {
	    CVMUint32 currMarks = *marks;
	    CVMUint32 toClear = currBoundaries & ~currMarks;
	    
	    /*
	     * Take those objects that are not marked and put them in
	     * their respective linked lists.
	     */
	    while (toClear != 0) {
		if ((toClear & 0x80000000) != 0) {
		    CVMUint32 objSize;
		    /*
		     * Make sure we're looking at an object we have allocated
		     */
		    CVMassert(CVMmsMarkedIn(heapPtr, theHeap->boundaries));
		    /*
		     * Look in the object's header to figure out
		     * how large it is.
		     */
		    objSize = CVMobjectSize((CVMObject*)heapPtr);
#ifdef CVM_MS_TRACE
		    {
			CVMClassBlock* cb =
			    CVMobjectGetClass((CVMObject*)heapPtr);
			CVMconsolePrintf("GC: Deleting object %x "
					 "(class=%C size=%d)\n",
					 heapPtr, cb, objSize);
		    }
#endif
#ifdef CVM_JVMPI
                    if (CVMjvmpiEventObjectFreeIsEnabled()) {
                        CVMjvmpiPostObjectFreeEvent(heapPtr);
                    }
                    liveObjectCount--;
#endif
#ifdef CVM_JVMTI
                    if (CVMjvmtiShouldPostObjectFree()) {
                        CVMjvmtiPostObjectFreeEvent(heapPtr);
                    }
#endif
		    /* 
		     * Add it to the free list.
		     * We are going to clear the boundaries bits all
		     * together at the end.  
		     */
		    CVMmsAddToFreeList(heapPtr, objSize,
				       CVMmsGetFirstListFor(objSize));
		    freeBytes += objSize;
		}
		toClear <<= 1;
		heapPtr--;
	    }
	    /*
	     * Clear those objects which have not been marked
	     */
	    *boundaries = currBoundaries & currMarks;
	} else {
	    /*
	     * We tried very hard not to mark any non-references, so this
	     * had better be true.
	     */
	    CVMassert(*marks == 0);
	}
	/*
	 * We might have bailed out while incrementing heapPtr. Make
	 * sure we get to the next set of heap words.
	 */
	otherHeapPtr -= 32;
	heapPtr = otherHeapPtr;
	    
	marks--;
	boundaries--;
    }
}

/*
 * We have more work to do
 */
static void
CVMmsAddTodo(CVMObject* oneMoreRef) 
{
    CVMmsTodoList[CVMmsTodoIdx] = oneMoreRef;
    CVMmsTodoIdx++;

    /* This is just a proof-of-concept GC. Therefore, we don't have
       expansion support on it. */
    if (CVMmsTodoIdx >= CVMmsTodoSize) {
	CVMconsolePrintf("CVMmsAddTodo: todo list exceeds size.\n");
	exit(1);
    }
}

/*
 * The generic reference handler. It is invoked for roots as well as
 * for object and array reference fields.
 */
static void
CVMmsHandleReference(CVMObject* ref)
{
    CVMassert(ref != 0);

    /*
     * Conservative reference handler. 
     *
     * First check range. If 'ref' is not in the heap range,
     * it cannot be a real reference.
     *
     * Either the object is in ROM, or it is allocated within the heap.
     */
    CVMassert(CVMmsObjectIsInROM(ref) ||
	      (((CVMUint32*)ref >= theHeap->data) &&
	       ((CVMUint32*)ref < &theHeap->data[theHeap->size / 4])));

    /*
     * %comment: f007
     */
    if (CVMmsObjectIsInROM(ref)) {
	return;
    }

    /*
     * Now check whether the 'ref' corresponds to an allocated
     * object. 
     *
     */
    CVMassert(CVMmsMarkedIn((CVMUint32*)ref, theHeap->boundaries));
    {
#ifdef CVM_MS_TRACE
	CVMconsolePrintf("CVMmsHandleReference(%x)<%s>\n",
			 ref, CVMobjectGetClass(ref)->classname);
#endif

	/*
	 * OK, it looks like a 'legal' object reference
	 * Scan it and its children if we have not seen it before.
	 */
	if (!CVMmsMarkedIn((CVMUint32*)ref, theHeap->markbits)) {
	    CVMExecEnv* currEE = CVMgetEE();
	    CVMmsSetBitFor((CVMUint32*)ref, theHeap->markbits);
	    /*
	     * Queue up all non-null object references.
	     */
	    CVMobjectWalkRefsWithSpecialHandling(currEE, currGcOpts, ref,
						 CVMobjectGetClassWord(ref), {
		if (*refPtr != 0) {
		    CVMmsAddTodo(*refPtr);
		}
	    }, CVMmsRootMarkTransitively, NULL);
	}
    }
}

/*
 * Go through and run the "to-do" list.
 */
static void
CVMmsExhaustTodoList()
{
    while (CVMmsTodoIdx > 0) {
	CVMObject* anotherReference = CVMmsTodoList[--CVMmsTodoIdx];
	CVMmsHandleReference(anotherReference); /* This may queue up more */
    }
}

static void
CVMmsRootMarkTransitively(CVMObject** root, void* data)
{
    CVMassert(root != 0);
    CVMassert(*root != 0);
#if CVM_MS_TRACE
    CVMconsolePrintf("GC: Root: [0x%x] = 0x%x\n", root, *root);
#endif
    CVMmsHandleReference(*root);
    /*
     * For each root, aggressively mark children instead of first
     * queueing up the roots and then running through the children.
     */
    CVMmsExhaustTodoList();
}

static CVMBool
CVMmsRefIsLive(CVMObject** refPtr, void* data)
{
    CVMObject* ref;

    CVMassert(refPtr != NULL);

    ref = *refPtr;
    CVMassert(ref != NULL);

    /*
     * ROM objects are always live
     */
    if (CVMobjectIsInROM(ref)) {
	return CVM_TRUE;
    }
    return CVMmsMarkedIn((CVMUint32*)ref, theHeap->boundaries);
}

/*
 * This routine is called by the common GC code after all locks are
 * obtained, and threads are stopped at GC-safe points. It's the
 * routine that needs a snapshot of the world while all threads are
 * stopped (typically at least a root scan).
 */
void
CVMgcimplDoGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMGCOptions gcOpts;

    CVMtraceGcStartStop(("GC: Collecting...\n"));

    gcOpts.isUpdatingObjectPointers = CVM_TRUE;
    gcOpts.discoverWeakReferences = CVM_FALSE;
#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    gcOpts.isProfilingPass = CVM_FALSE;
#endif
    currGcOpts = &gcOpts;

    /*
     * Clear all class marks. This is so that we don't
     * try to scan the same class twice.
     */
    CVMgcClearClassMarks(ee, &gcOpts);

    /*
     * Nothing is marked yet.
     */
    CVMmsClearMarks();

    /*
     * The marking phase will start discovering weak reference objects
     */
    gcOpts.discoverWeakReferences = CVM_TRUE;

    /*
     * CVMmsRootMark will mark roots, and all their children. So at the end
     * of gcScanRoots(), we know all the live objects.
     */
    CVMgcScanRoots(ee, &gcOpts, CVMmsRootMarkTransitively, NULL);

    /*
     * Stop discovering weak references
     */
    gcOpts.discoverWeakReferences = CVM_TRUE;

    /*
     * At this point, we know which objects are live and which are not.
     * Do the special objects processing.
     */
    CVMgcProcessSpecialWithLivenessInfo(ee, &gcOpts,
					CVMmsRefIsLive, NULL,
					CVMmsRootMarkTransitively, NULL);
    /*
     * Now clean garbage
     */
    CVMmsSweep();

    /*
     * And mark when all this happened
     */
    lastMajorGCTime = CVMtimeMillis();

    CVMtraceGcStartStop(("GC: Done.\n"));
}

CVMObject*
CVMgcimplRetryAllocationAfterGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    CVMObject* allocatedObj;
    
    allocatedObj = CVMmsTryAlloc(numBytes);	/* re-try */
    if (allocatedObj == NULL) {
	/*
	 * Last resort. Undo all our fragmented free lists,
	 * add everything to the big list, and try allocation
	 * for the third time. Deferred coalescing.
	 */
	CVMmsCollapseFreeLists();
	allocatedObj = CVMmsTryAlloc(numBytes);
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
#ifdef CVM_MS_GC_ON_EVERY_ALLOC
    static CVMUint32 allocCount = 1;
    /* Do one for stress-test. If it was unsuccessful, try again in the
       next allocation */
    if ((allocCount % CVM_MS_NO_ALLOCS_UNTIL_GC) == 0) {
	if (CVMmsInitiateGC(ee, numBytes)) {
	    allocCount = 1;
	}
    } else {
	allocCount++;
    }
#endif
    allocatedObj = CVMmsTryAlloc(numBytes);
#ifdef CVM_JVMPI
    if (allocatedObj != NULL) {
        liveObjectCount++;
    }
#endif
    return allocatedObj;
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
        CVMUint32 lastSharedArenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaNewEvent(lastSharedArenaID + 1, "Java Heap");
    }
}

/* Purpose: Posts the JVMPI_EVENT_ARENA_DELETE events for the GC specific
            implementation. */
void CVMgcimplPostJVMPIArenaDeleteEvent(void)
{
    if (CVMjvmpiEventArenaDeleteIsEnabled()) {
        CVMUint32 lastSharedArenaID = CVMgcGetLastSharedArenaID();
        CVMjvmpiPostArenaDeleteEvent(lastSharedArenaID + 1);
    }
}

/* Purpose: Gets the arena ID for the specified object. */
/* Returns: The requested ArenaID if found, else returns
            CVM_GC_ARENA_UNKNOWN. */
CVMUint32 CVMgcimplGetArenaID(CVMObject *obj)
{
    CVMUint32 arenaID = CVM_GC_ARENA_UNKNOWN;
    CVMUint8 *objPtr = (CVMUint8 *)obj;
    CVMUint8 *heapStart = (CVMUint8 *)heap->data;
    CVMUint8 *heapEnd = heapStart + heap->size;
    if ((objPtr >= heapStart) && (objPtr < heapEnd)) {
        arenaID = (CVMgcGetLastSharedArenaID() + 1);
    }
    return arenaID;
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
    return freeBytes;
}

/*
 * Return the amount of total memory in the heap, in bytes.
 */
CVMUint32
CVMgcimplTotalMemory(CVMExecEnv* ee)
{
    return theHeap->size;
}

/*
 * Destroy GC global state
 */
void
CVMgcimplDestroyGlobalState(CVMGCGlobalState* globalState)
{
    CVMtraceMisc(("Destroying global state for mark-sweep GC\n"));
}

/*
 * Destroy heap
 */
CVMBool
CVMgcimplDestroyHeap(CVMGCGlobalState* globalState)
{
    int i;

    CVMtraceMisc(("Destroying heap for mark-sweep GC\n"));

#ifdef CVM_JVMPI
    CVMgcimplPostJVMPIArenaDeleteEvent();
#endif

    theHeap = 0; /* First make sure nobody tries to allocate anything */
    for (i = 0; i <= CVM_MS_NFREELISTS; i++) {
	CVMmsFreeLists[i] = 0;
    }

    /*
     * All Java threads have stopped by the time we destroy the heap
     */
    free(globalState->heap->data);
    free(globalState->heap->boundaries);
    free(globalState->heap->markbits);

    free(globalState->heap);
    free(CVMmsTodoList);
    globalState->heap = 0;
    return CVM_TRUE;
}

/*
 * JVM_MaxObjectInspectionAge() support
 */
CVMInt64
CVMgcimplTimeOfLastMajorGC()
{
    return lastMajorGCTime;
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
    /* This is just a proof-of-concept GC. Therefore, we don't support
       heap iteration on it. */
    CVMconsolePrintf("Heap iteration not supported on mark-sweep GC yet\n");
    return CVM_FALSE;
}

#endif /* defined(CVM_DEBUG) || defined(CVM_JVMPI) */
