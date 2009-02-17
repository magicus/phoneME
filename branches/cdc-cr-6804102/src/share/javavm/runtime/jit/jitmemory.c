/*
 * @(#)jitmemory.c	1.11 06/10/10
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
#include "javavm/include/globals.h"

#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitmemory.h"

/*
 * This file implements the CVMJITmemXXX() apis.
 */


/*
 * Large chunks are preallocated to reduce fragmentation, speed up 
 * allocation, and speed up freeing all memory. Each chunk holds
 * CVMJIT_MEMCHUNK_MAXSIZE many bytes of data by default
 */
#define CVMJIT_MEMCHUNK_MAXSIZE 8192

struct CVMJITMemChunk {
    CVMJITMemChunk* next;
    CVMUint8*       curPtr;
    CVMUint8*       dataEnd;
#ifdef CVM_JIT_COLLECT_STATS
    CVMUint32       alignmentWaste; /* Total per-chunk */
#endif
    CVMUint8        data[1];
};

/* Note: #define CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION if you want to
   validate the memory fence blocks on every allocation. */
#undef CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION


#ifdef CVM_DEBUG_ASSERTS

/* NOTES: About the memory fence debugging tool:
   ============================================
   The Memory Fence tool works by padding all allocation requests with some
   header and footer records.

   The header contains information about the caller of the allocation as well
   as some checksums to ensure the header itself hasn't been corrupted.  The
   header ends in an array of CVM_MEMORY_FENCE_SIZE fences which are filled
   with the CVM_MEMORY_FENCE_MAGIC_WORD value.

   The footer starts with an array of CVM_MEMORY_FENCE_SIZE fences which are
   also filled with the CVM_MEMORY_FENCE_MAGIC_WORD value.  After the fences,
   some extra memory is allocated for extra info that the particular instance
   of the fence may require.

   The integrity of blocks in a block list can be validated by invoking
   CVMmemoryFenceValidateBlocks() on a block list.  The validation process will
   validate the checksum of the header first.  This ensures that the info in
   the header (including the checksum of the footer) is reliable.
       Next, the checksum of the footer is validated which ensures that the
   extra info in the footer is valid.  Subsequently, the fences in the header
   and footer are validated.

   When a corruption is detected, CVMmemoryFenceValidateBlocks() will call on
   CVMmemoryFenceReportError() to dump the info in the block.
   CVMmemoryFenceReportError() is conscious of possible corrupted data and will
   not make use of any data that is corrupted in an unreliable way.

   The memory fence implementation can be customized by the user during
   initialization of each block by passing in a vtbl.  This vtbl provides the
   callbacks necessary to initialize and dump the the extra info fields.

   NOTE: The memory fence list is built with a singly linked list.  This is
   deliberate as it simplifies the checksum computations.  a doubly linked
   list would add complication in the form of having to update checksums and
   may open the window to corruptions sneaking by if not done carefully.

*/


/* Data structures for the memory fence debugging util: */
typedef struct CVMMemoryFenceVtbl CVMMemoryFenceVtbl;
typedef struct CVMMemoryFenceBlock CVMMemoryFenceBlock;

#define CVM_MEMORY_FENCE_SIZE          5
#define CVM_MEMORY_FENCE_MAGIC_WORD    0xcafebabe
#define CVM_MEMORY_FENCE_CHECKSUM_SEED 0xa5a5a5a5

struct CVMMemoryFenceVtbl
{
    void (*initExtraInfo)(void *blockExtraInfo, void *extraInfo);
    void (*dumpExtraInfo)(void *blockExtraInfo);
};

struct CVMMemoryFenceBlock
{
    CVMUint32 headerChecksum;
    CVMUint32 footerChecksum;
    const CVMMemoryFenceVtbl *vtbl;
    CVMMemoryFenceBlock *prev;
    const char *filename;
    CVMUint32 lineNumber;
    CVMUint32 size;
    CVMUint32 extraInfoSize;
    /* Header fence goes here ... */
    /* User data is next ... */
    /* Footer fence comes after ... */
    /* ExtraInfo is last ... */
};

/*===========================================================================
// CVMMemoryFence prototypes:
*/
static void
CVMmemoryFenceInit(void **blocksList, const CVMMemoryFenceVtbl *vtbl,
                   CVMMemoryFenceBlock *block, const char *filename,
                   CVMUint32 lineNumber, CVMUint32 size,
                   void *extraInfo, CVMUint32 extraInfoSize);

static void
CVMmemoryFenceDestroy(void **blocksList, CVMMemoryFenceBlock *block);

static CVMUint32
CVMmemoryFenceComputeHeaderChecksum(CVMMemoryFenceBlock *block);

static CVMUint32
CVMmemoryFenceComputeFooterChecksum(CVMMemoryFenceBlock *block);

static void
CVMmemoryFenceValidateBlocks(void **blocksList,
                             const char *validatingFilename,
                             CVMUint32 validatingLineNumber,
                             const char *validatingMessage);

static void
CVMmemoryFenceReportError(CVMMemoryFenceBlock *block,
                          const char *errMessage,
                          const char *validatingFilename,
                          CVMUint32 validatingLineNumber,
                          const char *validatingMessage);

/*===========================================================================
// CVMMemoryFence macros:
*/

/* Purpose: Increases the size by the amount of memory needed by the memory
            fence. */
#define CVMmemoryFenceComputeAdjustedSize(requestedSize, extraInfoSize) \
    (CVMpackSizeBy(sizeof(CVMMemoryFenceBlock), 4) + \
     (sizeof(CVMUint32) * CVM_MEMORY_FENCE_SIZE) + \
     CVMpackSizeBy((requestedSize), 4) + \
     (sizeof(CVMUint32) * CVM_MEMORY_FENCE_SIZE) + \
     CVMpackSizeBy((extraInfoSize), 4))

/* Purpose: Gets the location of the header fence in the block. */
#define CVMmemoryFenceBlock2HeaderFence(block) \
    ((CVMUint32 *)((CVMUint8 *)(block) + \
                   CVMpackSizeBy(sizeof(CVMMemoryFenceBlock), 4)))

/* Purpose: Gets the location of the data area in the block. */
#define CVMmemoryFenceBlock2Data(block) \
    ((CVMUint8 *)CVMmemoryFenceBlock2HeaderFence(block) + \
     (sizeof(CVMUint32) * CVM_MEMORY_FENCE_SIZE))

/* Purpose: Gets the location of the footer fence in the block. */
#define CVMmemoryFenceBlock2FooterFence(block) \
    ((CVMUint32 *)(CVMmemoryFenceBlock2Data(block) + \
                   CVMpackSizeBy((block)->size, 4)))

/* Purpose: Gets the location of the extraInfo in the block. */
#define CVMmemoryFenceBlock2ExtraInfo(block) \
    ((void *)(CVMmemoryFenceBlock2FooterFence(block) + CVM_MEMORY_FENCE_SIZE))

/* Purpose: Gets the location of the block given the data area. */
#define CVMmemoryFenceData2Block(data) \
    ((CVMMemoryFenceBlock *)(((CVMUint8 *)data) - \
                             CVMpackSizeBy(sizeof(CVMMemoryFenceBlock), 4) - \
                             (sizeof(CVMUint32) * CVM_MEMORY_FENCE_SIZE)))

/*===========================================================================
// CVMMemoryFence functions:
*/

/* Purpose: Initializes the memory fence. */
static void
CVMmemoryFenceInit(void **blocksList, const CVMMemoryFenceVtbl *vtbl,
                   CVMMemoryFenceBlock *block, const char *filename,
                   CVMUint32 lineNumber, CVMUint32 size,
                   void *extraInfo, CVMUint32 extraInfoSize)
{
    CVMUint32 *headerFence;
    CVMUint32 *footerFence;
    void *blockExtraInfo;
    int i;

    /* NOTE: The initialization sequence is deliberately done in the following
       order because some steps may depend on the previous ones having being
       initialized.  Be careful if you wish to rearrange the order of things.
    */

    /* Initialize block info: */
    block->vtbl = vtbl;
    block->prev = NULL;
    block->filename = filename;
    block->lineNumber = lineNumber;
    block->size = size;
    block->extraInfoSize = extraInfoSize;

    /* Initialize header & footer fence: */
    headerFence = CVMmemoryFenceBlock2HeaderFence(block);
    footerFence = CVMmemoryFenceBlock2FooterFence(block);
    for (i = 0; i < CVM_MEMORY_FENCE_SIZE; i++) {
        headerFence[i] = CVM_MEMORY_FENCE_MAGIC_WORD;
        footerFence[i] = CVM_MEMORY_FENCE_MAGIC_WORD;
    }

    /* Initialize the extraInfo: */
    blockExtraInfo = CVMmemoryFenceBlock2ExtraInfo(block);
    if (vtbl != NULL) {
        vtbl->initExtraInfo(blockExtraInfo, extraInfo);
    }

    /* Add this block to the memory fence blocks list: */
    if (*blocksList != NULL) {
        CVMMemoryFenceBlock *prev = (CVMMemoryFenceBlock *)*blocksList;
        block->prev = prev;
    }
    *blocksList = (void *)block;

    /* Initialize footer checksum: */
    block->footerChecksum = CVMmemoryFenceComputeFooterChecksum(block);

    /* Lastly, initialize header checksum: */
    block->headerChecksum = CVMmemoryFenceComputeHeaderChecksum(block);
}

/* Purpose: Removes the block from the block list and do house-cleaning. */
/* NOTE: This function assumes that the list has already been validated using
         CVMmemoryFenceValidateBlocks(). */
static void
CVMmemoryFenceDestroy(void **blocksList, CVMMemoryFenceBlock *block)
{
    CVMMemoryFenceBlock *b = (CVMMemoryFenceBlock *)*blocksList;
    CVMMemoryFenceBlock *next = 0;

    while (b && b != block) {
        next = b;
        b = b->prev;
    }

    /* Make sure that we actually found the block: */
    CVMassert(b != NULL);
    if (next != 0) {
        next->prev = b->prev;
        /* Need to recompute the header checksum in the next block because the
           prev pointer has changed: */
        next->headerChecksum = CVMmemoryFenceComputeHeaderChecksum(next);
    } else {
        *blocksList = b->prev;
    }
}

/* Purpose: Computes the checksum for the block header fields. */
static CVMUint32
CVMmemoryFenceComputeHeaderChecksum(CVMMemoryFenceBlock *block)
{
    return CVM_MEMORY_FENCE_CHECKSUM_SEED +
           (CVMUint32)block->footerChecksum +
           (CVMUint32)block->vtbl +
           (CVMUint32)block->prev +
           (CVMUint32)block->filename +
           (CVMUint32)block->lineNumber +
           (CVMUint32)block->size +
           (CVMUint32)block->extraInfoSize;
}

/* Purpose: Computes the checksum for the block footer fields. */
static CVMUint32
CVMmemoryFenceComputeFooterChecksum(CVMMemoryFenceBlock *block)
{
    CVMUint32 checksum = CVM_MEMORY_FENCE_CHECKSUM_SEED;
    CVMUint32 scaledExtraInfoSize;
    CVMUint32 *extraInfo = (CVMUint32 *)CVMmemoryFenceBlock2ExtraInfo(block);
    int i;

    scaledExtraInfoSize = CVMpackSizeBy(block->extraInfoSize, 4) / 4;
    for (i = 0; i < scaledExtraInfoSize; i++) {
        checksum += extraInfo[i];
    }
    return checksum;
}

/* Purpose: Validates the contents of the memory fence to see if any corruption
            has been detected. */
static void
CVMmemoryFenceValidateBlocks(void **blocksList,
                             const char *validatingFilename,
                             CVMUint32 validatingLineNumber,
                             const char *validatingMessage)
{
    CVMMemoryFenceBlock *block = (CVMMemoryFenceBlock *)*blocksList;
    while (block) {
        CVMUint32 checksum;
        CVMUint32 *fence;
        int i;

        /* First we validate the header checksum to make sure that none of the
           data in the header (including the footer checksum, but not including
           the fence itself) has been corrupted:
        */
        checksum = CVMmemoryFenceComputeHeaderChecksum(block);
        if (checksum != block->headerChecksum) {
            CVMmemoryFenceReportError(block, "header checksum failure",
                validatingFilename, validatingLineNumber, validatingMessage);
        }

        /* Next we validate the footer checksum to make sure that the extraInfo
           has been corrupted:
        */
        checksum = CVMmemoryFenceComputeFooterChecksum(block);
        if (checksum != block->footerChecksum) {
            CVMmemoryFenceReportError(block, "footer checksum failure",
                validatingFilename, validatingLineNumber, validatingMessage);
        }

        /* Next, inspect the header fence: */
        fence = CVMmemoryFenceBlock2HeaderFence(block);
        for (i = 0; i < CVM_MEMORY_FENCE_SIZE; i++) {
            if (fence[i] != CVM_MEMORY_FENCE_MAGIC_WORD) {
                CVMmemoryFenceReportError(block,
                    "header fence corrupted", validatingFilename,
                    validatingLineNumber, validatingMessage);
            }
        }

        /* Next, inspect the footer fence: */
        fence = CVMmemoryFenceBlock2FooterFence(block);
        for (i = 0; i < CVM_MEMORY_FENCE_SIZE; i++) {
            if (fence[i] != CVM_MEMORY_FENCE_MAGIC_WORD) {
                CVMmemoryFenceReportError(block,
                    "footer fence corrupted", validatingFilename,
                    validatingLineNumber, validatingMessage);
            }
        }

        /* Move onto next fence: */
        block = block->prev;
    }
}

/* Purpose: Reports an error detected during memory fence validation. */
static void
CVMmemoryFenceReportError(CVMMemoryFenceBlock *block,
                          const char *errMessage,
                          const char *validatingFilename,
                          CVMUint32 validatingLineNumber,
                          const char *validatingMessage)
{
    CVMUint32 *fence;
    CVMUint32 headerChecksum;
    CVMBool headerIsGood;
    int i;

    /* Print error message: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("CVMMemoryFenceError: %s\n", errMessage);
    CVMconsolePrintf("  Detected while doing %s\n", validatingMessage);
    CVMconsolePrintf("      at line %d in %s\n",
                     validatingLineNumber, validatingFilename);

    /* Print some block info: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("  Found at block:      0x%x\n", block);

    headerChecksum = CVMmemoryFenceComputeHeaderChecksum(block);
    headerIsGood = (headerChecksum == block->headerChecksum);
    CVMconsolePrintf("    actual header checksum:   0x%x\n", headerChecksum);
    CVMconsolePrintf("    expected header checksum: 0x%x\n",
                     block->headerChecksum);

    if (headerIsGood) {
        CVMconsolePrintf("    actual footer checksum:   0x%x\n",
                         CVMmemoryFenceComputeFooterChecksum(block));
        CVMconsolePrintf("    expected footer checksum: 0x%x\n",
                         block->footerChecksum);
    } else {
        CVMconsolePrintf("    actual footer checksum:   "
                         "Unavailable due to corrupted header\n");
        CVMconsolePrintf("    expected footer checksum: "
                         "Unavailable due to corrupted header\n");
    }
    CVMconsolePrintf("    vtbl:         0x%x\n", block->vtbl);
    CVMconsolePrintf("    prev:         0x%x\n", block->prev);

    /* Print block allocation info: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("  Allocated by:\n");
    if (headerIsGood) {
        CVMconsolePrintf("    filename:            %s\n", block->filename);
    } else {
        CVMconsolePrintf("    filename:            0x%x\n", block->filename);
    }
    CVMconsolePrintf("    lineNumber:          %d\n", block->lineNumber);
    CVMconsolePrintf("    alloc size:          %d\n", block->size);
    CVMconsolePrintf("    alloc extraInfoSize: %d\n", block->extraInfoSize);
    CVMconsolePrintf("    data address:        0x%x\n",
                     CVMmemoryFenceBlock2Data(block));

    /* Print header fence values: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("  Header fence values:\n");
    fence = CVMmemoryFenceBlock2HeaderFence(block);
    for (i = 0; i < CVM_MEMORY_FENCE_SIZE; i++) {
        CVMconsolePrintf("    fence[%d]:     0x%x\n", i, fence[i]);
    }

    /* Print footer fence values: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("  Footer fence values:\n");
    fence = CVMmemoryFenceBlock2FooterFence(block);
    for (i = 0; i < CVM_MEMORY_FENCE_SIZE; i++) {
        CVMconsolePrintf("    fence[%d]:     0x%x\n", i, fence[i]);
    }

    /* Print extra Info: */
    CVMconsolePrintf("\n");
    CVMconsolePrintf("  Extra info:\n");
    if (headerIsGood) {
        /* If we get here, then the header is probably fine, and we can trust
           the vtbl: */
        if (block->vtbl != NULL) {
            block->vtbl->dumpExtraInfo(CVMmemoryFenceBlock2ExtraInfo(block));
            CVMconsolePrintf("\n");
        } else {
            CVMconsolePrintf("    No extra info\n");
        }
    } else {
        CVMconsolePrintf("    Unavailable due to corrupted header\n");
    }

    CVMassert(CVM_FALSE);
}

/*============================================================================
// Memory Fence for CVMJITmemNew():
*/

static void
memNewInitExtraInfo(void *blockExtraInfo, void *extraInfo)
{
    CVMJITAllocationTag *tag = (CVMJITAllocationTag *)extraInfo;
    CVMJITAllocationTag *info = (CVMJITAllocationTag *)blockExtraInfo;
    *info = *tag;
}


/* Purpose: Returns the name of the allocator based on the specified tag. */
static const char*
CVMJITallocationTag2Name(CVMJITAllocationTag i)
{
    switch(i) {
    case JIT_ALLOC_IRGEN_NODE:   return "JIT_ALLOC_IRGEN_NODE";
    case JIT_ALLOC_IRGEN_OTHER:  return "JIT_ALLOC_IRGEN_OTHER";
    case JIT_ALLOC_OPTIMIZER:    return "JIT_ALLOC_OPTIMIZER";
    case JIT_ALLOC_CGEN_REGMAN:  return "JIT_ALLOC_CGEN_REGMAN";
    case JIT_ALLOC_CGEN_ALURHS:  return "JIT_ALLOC_CGEN_ALURHS";
    case JIT_ALLOC_CGEN_MEMSPEC: return "JIT_ALLOC_CGEN_MEMSPEC";
    case JIT_ALLOC_CGEN_FIXUP:   return "JIT_ALLOC_CGEN_FIXUP";
    case JIT_ALLOC_CGEN_OTHER:   return "JIT_ALLOC_CGEN_OTHER";
    case JIT_ALLOC_COLLECTIONS:  return "JIT_ALLOC_COLLECTIONS";
    case JIT_ALLOC_DEBUGGING:    return "JIT_ALLOC_DEBUGGING";
/* IAI - 08 */
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    case JIT_ALLOC_CODE_SCHEDULING:    return "JIT_ALLOC_CODE_SCHEDULING";
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */
    default: return "<UNKNOWN ALLOCATION>";
    }
}

static void
memNewDumpExtraInfo(void *blockExtraInfo)
{
    CVMJITAllocationTag tag = *(CVMJITAllocationTag *)blockExtraInfo;
    const char *typeStr = CVMJITallocationTag2Name(tag);
    CVMconsolePrintf("    tag:  %s\n", typeStr);
}

const CVMMemoryFenceVtbl memNewMemoryFenceVtbl =
{
    memNewInitExtraInfo,
    memNewDumpExtraInfo,
};

#endif


/* Purpose: Allocates a buffer to be used as working memory by the dynamic
            compiler.*/
static CVMJITMemChunk*
allocateAndLinkNewChunk(CVMJITCompilationContext* con,
			CVMUint32 dataSize)
{
    CVMJITMemChunk* allocChunk;
    CVMJITMemChunk* topChunk = con->memChunks;
    
    CVMUint32 chunkSize =
	sizeof(CVMJITMemChunk) + (dataSize - 1) * sizeof(CVMUint8);
    
    allocChunk = calloc(1, chunkSize);
    if (allocChunk == NULL) {
        CVMJITerror(con, OUT_OF_MEMORY, "Failed to allocate new chunk");
	CVMassert(CVM_FALSE); /* CVMJITerror must not return */
    }
    allocChunk->curPtr = allocChunk->data;
    allocChunk->dataEnd = &allocChunk->data[dataSize];
    allocChunk->next = topChunk;
#ifdef CVM_JIT_COLLECT_STATS
    allocChunk->alignmentWaste = 0;
#endif
    con->memChunks = allocChunk;
    return allocChunk;
}

void
CVMJITmemInitialize(CVMJITCompilationContext* con)
{
    /* Have one chunk ready */
    allocateAndLinkNewChunk(con, CVMJIT_MEMCHUNK_MAXSIZE);
}

/* Purpose: Allocates some working memory from the allocator specified by the
            allocation tag. */
void*
CVMJITmemNewInternal(CVMJITCompilationContext* con, CVMJITAllocationTag tag,
                     CVMUint32 sizeIn
                     DECLARE_CALLER_INFO(callerFilename, callerLineNumber))
{
    CVMJITMemChunk* allocChunk = con->memChunks;
    CVMUint32 size;
    CVMUint8 *dataEnd = allocChunk->dataEnd;
    CVMUint8 *result = allocChunk->curPtr;
    CVMUint8 *resultEnd;
#ifdef CVM_DEBUG_ASSERTS
    CVMUint32 origSize;
#endif

    size = CVMpackSizeBy(sizeIn, 4);  /* Round it up. */
#ifdef CVM_DEBUG_ASSERTS
    origSize =  size;
    size = CVMmemoryFenceComputeAdjustedSize(origSize, sizeof(tag));
#define STATS_SIZE origSize
#else
#define STATS_SIZE size
#endif

    if (tag != JIT_ALLOC_DEBUGGING) {
	con->workingMemory += STATS_SIZE;
	if (con->workingMemory > CVMglobals.jit.maxWorkingMemorySize) {
	    CVMJITlimitExceeded(con, "out of working memory");
	}
    }

    resultEnd = result + size;

    if (resultEnd < dataEnd) {
	allocChunk->curPtr = resultEnd;
    } else {
	/* Time to expand. You'd better fit in this next chunk */
	if (size < CVMJIT_MEMCHUNK_MAXSIZE) {
	    allocChunk = allocateAndLinkNewChunk(con, CVMJIT_MEMCHUNK_MAXSIZE);
	} else {
	    /* Make it just big enough for this big big allocation */
	    allocChunk = allocateAndLinkNewChunk(con, size);
	}
	
	result = allocChunk->data;
	allocChunk->curPtr = result + size;
	CVMassert((size < CVMJIT_MEMCHUNK_MAXSIZE) || 
		  (allocChunk->curPtr == allocChunk->dataEnd));
    }
    
#ifdef CVM_JIT_COLLECT_STATS
    CVMJITstatsRecordAdd(con, (CVMJITStatsTag)tag, STATS_SIZE);
    CVMJITstatsRecordAdd(con, CVMJIT_STATS_TOTAL_WORKING_MEMORY, STATS_SIZE);
    allocChunk->alignmentWaste += STATS_SIZE - sizeIn;
#endif /* CVM_JIT_COLLECT_STATS */

#ifdef CVM_DEBUG_ASSERTS

#ifdef CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION
    /* Make sure there is no corruption before allocating a new block: */
    CVMmemoryFenceValidateBlocks(&con->memoryFenceBlocks, callerFilename,
                                 callerLineNumber, "CVMJITmemNew()");
#endif /* CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION */

    /* Initialize the fence: */
    CVMmemoryFenceInit(&con->memoryFenceBlocks, &memNewMemoryFenceVtbl,
                       (CVMMemoryFenceBlock *)result, callerFilename,
                       callerLineNumber, sizeIn, &tag, sizeof(tag));

    /* Compute the address of the data buffer requested by the caller: */
    result = CVMmemoryFenceBlock2Data(result);

#endif /* CVM_DEBUG_ASSERTS */
    return result;
}

/* 
 * Flush all memory allocated with CVMJITmemNew()
 */
void
CVMJITmemFlush(CVMJITCompilationContext* con)
{
    CVMJITMemChunk* chunk;
    CVMJITMemChunk* nextChunk;

#ifdef CVM_DEBUG_ASSERTS
    /* Make sure there is no corruption before releasing all the blocks: */
    CVMmemoryFenceValidateBlocks(&con->memoryFenceBlocks, __FILE__, __LINE__,
                                 "CVMJITmemFlush()");
#endif /* CVM_DEBUG_ASSERTS */

    for (chunk = con->memChunks; chunk != NULL; chunk = nextChunk) {
	nextChunk = chunk->next;
#ifdef CVM_DEBUG
	/* Make sure you delete all data before blowing it off */
	memset(chunk->data, 0, chunk->dataEnd - chunk->data);
#endif
#ifdef CVMJIT_MEM_STATS
#ifdef CVM_JIT_COLLECT_STATS
        {
	    CVMUint32 chunkSize = chunk->dataEnd - chunk->data;
	    
	    CVMconsolePrintf("\tCVMJITmemAlloc(): FREEING CHUNK 0x%x\n", chunk);
	    CVMconsolePrintf("\t\tsize=%d, alignment waste=%d%% unused=%d%%\n",
			     chunkSize, 
			     chunk->alignmentWaste * 100 / chunkSize,
			     (chunk->dataEnd-chunk->curPtr) * 100 / chunkSize);
        };
#endif /* CVM_JIT_COLLECT_STATS */
#endif /* CVMJIT_MEM_STATS */
	free(chunk);
    }
    con->memChunks = NULL; /* Just in case */
}

/*
 * Separate "permanent" memory allocations from temporary allocations
 */
void*
CVMJITmemNewLongLivedMemoryInternal(CVMJITCompilationContext* con,
    CVMUint32 size DECLARE_CALLER_INFO(callerFilename, callerLineNumber))
{
    void *ret;
#ifdef CVM_DEBUG_ASSERTS
    CVMUint32 origSize = size;

    size = CVMpackSizeBy(size, 4);  /* Round it up. */
    size = CVMmemoryFenceComputeAdjustedSize(size, 0);
#endif
    ret = calloc(1, size);
    if (ret == NULL) {
        CVMJITerror(con, OUT_OF_MEMORY, "Failed to new long lived memory");
    }

#ifdef CVM_DEBUG_ASSERTS
#ifdef CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION
    /* Make sure there is no corruption before allocating a new block: */
    CVMmemoryFenceValidateBlocks(&CVMglobals.jit.longLivedMemoryBlocks,
        callerFilename, callerLineNumber, "CVMJITmemNewLongLivedMemory()");
#endif /* CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION */
    CVMmemoryFenceInit(&CVMglobals.jit.longLivedMemoryBlocks, NULL,
        (CVMMemoryFenceBlock *)ret, callerFilename, callerLineNumber,
        origSize, NULL, 0);

    /* Compute the address of the data buffer requested by the caller: */
    ret = CVMmemoryFenceBlock2Data(ret);
#endif /* CVM_DEBUG_ASSERTS */

    return ret;
}

void*
CVMJITmemExpandLongLivedMemoryInternal(CVMJITCompilationContext* con,
    void * ptr, CVMUint32 size
    DECLARE_CALLER_INFO(callerFilename, callerLineNumber))
{
    void *ret;
#ifdef CVM_DEBUG_ASSERTS
    CVMUint32 origSize = size;

    size = CVMpackSizeBy(size, 4);  /* Round it up. */
    size = CVMmemoryFenceComputeAdjustedSize(size, 0);
    ptr = CVMmemoryFenceData2Block(ptr);

#ifdef CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION
    /* Make sure there is no corruption before re-allocating the block: */
    CVMmemoryFenceValidateBlocks(&CVMglobals.jit.longLivedMemoryBlocks,
        callerFilename, callerLineNumber, "CVMJITmemExpandLongLivedMemory()");
#endif /* CVM_DO_INTENSIVE_MEMORY_FENCE_VALIDATION */

    /* Remove from the list first because the realloc() may free it: */
    CVMmemoryFenceDestroy(&CVMglobals.jit.longLivedMemoryBlocks,
                          (CVMMemoryFenceBlock *)ptr);
#endif

    ret = realloc(ptr, size);
    if (ret == NULL) {
        CVMJITerror(con, OUT_OF_MEMORY, "Failed to expand long lived memory");
    }

#ifdef CVM_DEBUG_ASSERTS
    /* Re-initialize it: */
    CVMmemoryFenceInit(&CVMglobals.jit.longLivedMemoryBlocks, NULL,
        (CVMMemoryFenceBlock *)ret, callerFilename, callerLineNumber,
        origSize, NULL, 0);

    /* Compute the address of the data buffer requested by the caller: */
    ret = CVMmemoryFenceBlock2Data(ret);
#endif /* CVM_DEBUG_ASSERTS */

    return ret;
}

void
CVMJITmemFreeLongLivedMemoryInternal(void* ptr
     DECLARE_CALLER_INFO(callerFilename, callerLineNumber))
{
#ifdef CVM_DEBUG_ASSERTS
    /* Make sure there is no corruption before re-allocating the block: */
    CVMmemoryFenceValidateBlocks(&CVMglobals.jit.longLivedMemoryBlocks,
        callerFilename, callerLineNumber, "CVMJITmemFreeLongLivedMemory()");

    ptr = CVMmemoryFenceData2Block(ptr);
    CVMmemoryFenceDestroy(&CVMglobals.jit.longLivedMemoryBlocks, ptr);
#endif
    free(ptr);
}
