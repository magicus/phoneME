/*
 * @(#)jitopt.c	1.24 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitutils.h"

#include "javavm/include/clib.h"

#if 0 /* Temporarily commented out. */

/* Loop descriptors */
/* Back edges go tail -> head. So pc(head) < pc(tail). */
struct CVMJITLoop {
    CVMJITIRBlock* headBlock;
    CVMJITIRBlock* tailBlock;
    CVMJITSet      loopBlocks;
    CVMUint32      numBlocks;
};

/*
 * Nested loops. Each CVMJITNestedLoop entry contains an array of loop
 * pointers, and the size of this array.
 */
struct CVMJITNestedLoop {
    CVMJITLoop**    loops;
    CVMUint32       numLoopsContained;
};

#ifdef CVM_DEBUG
/* Forward declarations for dumpers */
static void
dumpBlocks(CVMJITCompilationContext* con,
	   CVMJITIRBlock* rootblock, CVMUint32 numBlocks);

static void
dumpLoops(CVMJITCompilationContext* con);

static void
dumpNestedLoops(CVMJITCompilationContext* con);
#endif /* CVM_DEBUG */

/*
 * Iterator for DFS walk computation
 */
typedef struct DFSIter {
    CVMJITIRBlock** dfsWalk;
    CVMUint32       nextIdx;
    char*           visited;

    /* Explicit stack for iterative DFS walk */
    CVMJITStack     stack;
} DFSIter;

/*
 * Iterative DFS walker. This one is pre-order -- we visit each node
 * before we visit its successors.  
 */
static void
dfsOrderDoIt(CVMJITCompilationContext* con, DFSIter* iter);

static void
visitSucc(CVMJITCompilationContext* con, CVMUint32 succ, void* data)
{
    DFSIter* iter = (DFSIter*)data;
    
    if (!iter->visited[succ]) {
	CVMJITstackPush(con, (CVMJITIRBlock*)(con->idToBlock[succ]), 
	    &iter->stack);
    }
}

static void
dfsOrderDoIt(CVMJITCompilationContext* con, DFSIter* iter)
{
    while (!CVMJITstackIsEmpty(con, &iter->stack)) {
	CVMJITIRBlock* block;
	block = (CVMJITIRBlock*)CVMJITstackPop(con, &iter->stack);
	/* Pre-order traversal */
	iter->dfsWalk[iter->nextIdx++] = block;
	iter->visited[block->blockID]  = CVM_TRUE;
	CVMJITsetIterate(con, &block->successors, visitSucc, iter);
    }
}

static CVMJITIRBlock**
computeDfsWalkDoIt(CVMJITCompilationContext* con, 
		   CVMJITIRBlock* rootblock, CVMUint32 numBlocks)
{
    DFSIter iter;
    
    iter.visited = (char*)CVMJITmemNew(con, JIT_ALLOC_OPTIMIZER,
				       numBlocks * sizeof(char));
    iter.nextIdx = 0;
    /* Allocate one extra place to make the list NULL-terminated */
    iter.dfsWalk = (CVMJITIRBlock**)
	CVMJITmemNew(con, JIT_ALLOC_OPTIMIZER,
		     (numBlocks+1) * sizeof(CVMJITIRBlock*));
    CVMJITstackInit(con, &iter.stack, numBlocks);

    /* Start walk with root block */
    CVMJITstackPush(con, (CVMJITIRBlock*)rootblock, &iter.stack);

    /* And do the walk */
    dfsOrderDoIt(con, &iter);
    
    /* NULL-terminated */
    iter.dfsWalk[numBlocks] = NULL;

    return iter.dfsWalk;
}

/*
 * Compute DFS walk if it has not been computed already
 */
static void
computeDfsWalkIfNeeded(CVMJITCompilationContext* con,
		       CVMJITIRBlock* rootnode, CVMUint32 numBlocks)
{
    if (con->dfsWalk == NULL) {
	con->dfsWalk = computeDfsWalkDoIt(con, rootnode, numBlocks);
    }
}

/*
 * Finding dominators. Blocks are traversed in DFS order for faster
 * convergence.
 */
static void
addDominators(CVMJITCompilationContext* con, CVMUint32 elem, void* data)
{
    CVMJITSet*     tempSet   = (CVMJITSet*)data;
    CVMJITIRBlock* predBlock = con->idToBlock[elem];
    
    CVMJITsetIntersection(con, tempSet, &predBlock->dominators);
}

void
findDominators(CVMJITCompilationContext* con,
	       CVMJITIRBlock* rootblock, CVMUint32 numBlocks)
{
    CVMJITSet temp;
    CVMJITIRBlock* block;
    CVMJITIRBlock** dfsWalk;
    CVMBool change;
#ifdef CVM_DEBUG
    CVMUint32 numIter = 0;
#endif

    CVMJITsetInit(con, &temp);

    /*
     * Initially, the dominator set for each block but the root is
     * all the blocks.
     */
    for (block = rootblock->next; block != NULL; block = block->next) {
	CVMJITsetInit(con, &block->dominators);
	CVMJITsetPopulate(con, &block->dominators, numBlocks);
    }
    /*
     * And the root block dominates only itself
     */
    CVMJITsetInit(con, &rootblock->dominators);
    CVMJITsetAdd(con, &rootblock->dominators, rootblock->blockID);

    /* Compute pre-order DFS traversal of the blocks */
    computeDfsWalkIfNeeded(con, rootblock, numBlocks);
    CVMassert(*con->dfsWalk == rootblock);

    /*
     * Compute until there are no changes
     */
    do {
	dfsWalk = con->dfsWalk;

	change = CVM_FALSE;
	
	/* The pre-increment neatly skips over the root block */
	while ((block = *(++dfsWalk)) != NULL) {
	    CVMBool equals;
	    
	    /* Temp has all the blocks for a start */
	    CVMJITsetPopulate(con, &temp, numBlocks);
	    /*
	     * Find the intersection of the dominators of all
	     * predecessors of this block
	     */
	    CVMJITsetIterate(con, &block->predecessors, addDominators, &temp);
	    /* And add the current block as a dominator */
	    CVMJITsetAdd(con, &temp, block->blockID);
	    /* Had we already set the dominators of this block to the
	       accumulated value? If so, no change happened */
	    CVMJITsetEquals(con, &temp, &block->dominators, equals);
	    if (!equals) {
		change = CVM_TRUE;
		CVMJITsetCopy(con, &block->dominators, &temp);
	    }
	}

#ifdef CVM_DEBUG
	numIter++;
#endif

    } while (change);

    CVMtraceJITIROPT(("JO: Computed dominators in %d iterations\n", numIter));
}

/*
 * Loop finder help.
 */
typedef struct BackEdgeIter {
    CVMJITGrowableArray loops;

    CVMUint32     tail;     /* The blockID of the tail node */

    /* To be used in natural loop computation */
    CVMJITStack  stack;
    CVMJITLoop*  currentLoop;
} BackEdgeIter;

static CVMJITLoop*
newLoopStruct(CVMJITCompilationContext* con, BackEdgeIter* iter)
{
    void* elem;
    
    CVMJITgarrNewElem(con, &iter->loops, elem);
    return (CVMJITLoop*)elem;
}

/*
 * Visit predecessor for loop computation purposes
 */
static void
visitPred(CVMJITCompilationContext* con, CVMUint32 pred, void* data)
{
    CVMBool contains;
    BackEdgeIter* iter = (BackEdgeIter*)data;
    
    CVMJITLoop* loop = iter->currentLoop;
    CVMJITSet*  loopBlocks = &loop->loopBlocks;
    
    CVMJITsetContains(loopBlocks, pred, contains);
    if (!contains) {
	CVMJITsetAdd(con, loopBlocks, pred);
	loop->numBlocks++;
	CVMJITstackPush(con, (CVMJITIRBlock*)(con->idToBlock[pred]), 
	    &iter->stack);
    }
}

/*
 * Find natural loop of the current back edge
 */
static void
findNaturalLoop(CVMJITCompilationContext* con, CVMJITLoop* loop,
		BackEdgeIter* iter)
{
    CVMJITIRBlock* headBlock = loop->headBlock;
    CVMJITIRBlock* tailBlock = loop->tailBlock;

    iter->currentLoop = loop;
    
    loop->numBlocks = 1;
    CVMJITsetAdd(con, &loop->loopBlocks, headBlock->blockID);
    
    if (headBlock != tailBlock) {
	CVMJITsetAdd(con, &loop->loopBlocks, tailBlock->blockID);
	loop->numBlocks++;
	CVMJITstackPush(con, (CVMJITIRBlock*)tailBlock, &iter->stack);
    }
    while (!CVMJITstackIsEmpty(con, &iter->stack)) {
	CVMJITIRBlock* block;
	block = (CVMJITIRBlock*)CVMJITstackPop(con, &iter->stack);
	CVMJITsetIterate(con, &block->predecessors, visitPred, iter);
    }
}

/* Back edges go tail -> head. So pc(head) < pc(tail). 
 * During each set iteration, tail is constant, and head changes
 *
 * Add this back edge and find its natural loop
 */
static void
handleBackEdge(CVMJITCompilationContext* con, CVMUint32 head, void* data)
{
    BackEdgeIter* iter = (BackEdgeIter*)data;
    /*
     * Each back edge corresponds to a loop. Allocate space.
     */
    CVMJITLoop* loop = newLoopStruct(con, iter);

    CVMtraceJITIROPT(("JO: Found back edge: head=%d, tail=%d\n",
		      head, iter->tail));
    loop->tailBlock = con->idToBlock[iter->tail];
    loop->headBlock = con->idToBlock[head];
    CVMJITsetInit(con, &loop->loopBlocks);

    /* Find out which blocks are included in the natural loop for the
       current back edge */
    findNaturalLoop(con, loop, iter);
    /* Make sure the stack in iter is empty after each natural loop
       computation */
    CVMassert(iter->stack.todoIdx == 0);
}

/*
 * Given loop data, find nested loops and order them.
 */
typedef struct NestedLoopIter {
    CVMJITGrowableArray nestedLoops;
} NestedLoopIter;

static void
findNestedLoops(CVMJITCompilationContext* con, NestedLoopIter* iter);

#ifdef CVM_DEBUG

static void
findLoops(CVMJITCompilationContext* con, CVMJITIRBlock* rootblock,
	  CVMUint32 numBlocks)
{
    CVMJITIRBlock* block;
    CVMJITSet temp;
    BackEdgeIter iterBE;
    NestedLoopIter iterNL;

    findDominators(con, rootblock, numBlocks);
    CVMtraceJITIROPTExec(dumpBlocks(con, rootblock, numBlocks));
    
    /* no loops yet */
    CVMJITgarrInit(con, &iterBE.loops, sizeof(CVMJITLoop));

    /* This stack will be used in natural loop computation */
    CVMJITstackInit(con, &iterBE.stack, numBlocks);
    
    /* We have the dominators. Now look for back edges. */
    CVMJITsetInit(con, &temp);
    for (block = rootblock; block != NULL; block = block->next) {
	CVMBool isEmpty;
	/* The intersection of the successors and the dominators gives
	   you the loop closing branches, otherwise known 
	   as "back edges". */
	CVMJITsetClear(con, &temp);
	
	CVMJITsetCopy(con, &temp, &block->successors);
	CVMJITsetIntersection(con, &temp, &block->dominators);
	CVMJITsetIsEmpty(con, &temp, isEmpty);
	if (!isEmpty) {
	    /* We found some back edges. Record each, and compute
	       its natural loops */
	    iterBE.tail = block->blockID;
	    
	    CVMJITsetIterate(con, &temp, handleBackEdge, &iterBE);

	    CVMtraceJITIROPT(("JO: Loop closing branches from block #%d: ",
			      block->blockID));
	    CVMtraceJITIROPTExec(CVMJITsetDumpElements(con, &temp));
	    CVMtraceJITIROPT(("\n"));
	}
    }
    CVMJITsetDestroy(con, &temp);

    /*
     * We have collected the loops in iterBE.loops. Pass info to con
     */
    con->loops    = (CVMJITLoop*)CVMJITgarrGetElems(con, &iterBE.loops);
    con->numLoops = CVMJITgarrGetNumElems(con, &iterBE.loops);

    CVMtraceJITIROPTExec(dumpLoops(con));

    /*
     * We'll now be collecting nested loops
     */
    CVMJITgarrInit(con, &iterNL.nestedLoops, sizeof(CVMJITNestedLoop));
    
    findNestedLoops(con, &iterNL);
    
    /*
     * We have collected the loops in iterNL.nestedLoops. Pass info to con
     */
    con->nestedLoops    = (CVMJITNestedLoop*)
	CVMJITgarrGetElems(con, &iterNL.nestedLoops);
    con->numNestedLoops = CVMJITgarrGetNumElems(con, &iterNL.nestedLoops);

    CVMtraceJITIROPTExec(dumpNestedLoops(con));
}

#endif

/* Forward declarations */
static CVMJITNestedLoop*
findNestedLoopFor(CVMJITCompilationContext* con, CVMJITLoop* loop,
		  NestedLoopIter* iter);

static CVMJITNestedLoop*
newNestedLoop(CVMJITCompilationContext* con, NestedLoopIter* iter);

static void
addToNestedLoop(CVMJITCompilationContext* con, CVMJITNestedLoop* nestedLoop,
		CVMJITLoop* loop);

/*
 * Given loop data, find nested loops and order them.
 */
static void
findNestedLoops(CVMJITCompilationContext* con, NestedLoopIter* iter)
{
    /*
     * The loops we have collected are natural loops. So for each pair
     * of block sets L1 and L2 corresponding to two natural loops, L1
     * is either disjoint from L2, or L2 is wholly contained in L1.
     * (assuming size(L1) > size(L2))
     *
     * Based on that, we can separate out the nested loops from the rest.  */
    CVMUint32 i;
    
    /* For each loop find or create a nested loop structure that will
       hold it */
    for (i = 0; i < con->numLoops; i++) {
	CVMJITNestedLoop* nestedLoop;
	CVMJITLoop* loop = &con->loops[i];

	nestedLoop = findNestedLoopFor(con, loop, iter);
	if (nestedLoop == NULL) {
	    nestedLoop = newNestedLoop(con, iter);
	}
	addToNestedLoop(con, nestedLoop, loop);
    }
}

static CVMJITNestedLoop*
findNestedLoopFor(CVMJITCompilationContext* con, CVMJITLoop* loop,
		  NestedLoopIter* iter)
{
    CVMUint32 i;
    CVMJITSet temp;
    CVMJITNestedLoop* nestedLoops;
    CVMUint32 numNestedLoops;
    
    nestedLoops = (CVMJITNestedLoop*)
	CVMJITgarrGetElems(con, &iter->nestedLoops);
    numNestedLoops = CVMJITgarrGetNumElems(con, &iter->nestedLoops);
    CVMJITsetInit(con, &temp);

    for (i = 0; i < numNestedLoops; i++) {
	CVMJITNestedLoop* nloop      = &nestedLoops[i];
	CVMJITLoop*       firstLoop  = nloop->loops[0];
	CVMBool isEmpty;

	/*
	 * Look through the nested loops we have accumulated thus far.
	 * If this current loop has a non-empty intersection with any
	 * of them, insert it in order. If not, it starts a nested loop list
	 * of its own
	 */
	CVMJITsetClear(con, &temp);
	
	CVMJITsetCopy(con, &temp, &firstLoop->loopBlocks);
	CVMJITsetIntersection(con, &temp, &loop->loopBlocks);
	CVMJITsetIsEmpty(con, &temp, isEmpty);
	if (!isEmpty) {
	    /* This is our nested loop */
	    return nloop;
	}
    }
    return NULL;
}

static void
addToNestedLoop(CVMJITCompilationContext* con, CVMJITNestedLoop* nestedLoop,
		CVMJITLoop* loop)
{
    /* Keep list in ascending order of numBlocks */
    CVMUint32    numNested = nestedLoop->numLoopsContained;
    CVMJITLoop** insPos    = &nestedLoop->loops[0];
    CVMJITLoop** insPosEnd = &nestedLoop->loops[numNested];
    
    while (insPos < insPosEnd) {
	if ((*insPos)->numBlocks >= loop->numBlocks) {
	    /* Make space */
	    memmove(insPos + 1, insPos,
		    (insPosEnd - insPos) * sizeof(CVMJITLoop*));
	    break;
	}
	insPos++;
    }
    nestedLoop->numLoopsContained++;
    *insPos = loop;
}

static CVMJITNestedLoop*
newNestedLoop(CVMJITCompilationContext* con, NestedLoopIter* iter)
{
    void* elem;
    CVMJITNestedLoop* nloop;

    CVMJITgarrNewElem(con, &iter->nestedLoops, elem);
    nloop = (CVMJITNestedLoop*)elem;

    /*
     * Assume conservatively that all loops will be concentrated here,
     * and allocate enough space. We won't have too many of these
     * nestedloop structures anyway.
     */
    nloop->loops = CVMJITmemNew(con, JIT_ALLOC_OPTIMIZER,
				con->numLoops * sizeof(CVMJITLoop*));
    nloop->numLoopsContained = 0;
    return nloop;
}


#ifdef CVM_DEBUG
/*
 * Until other parts of the system fall into place, I'll create a few
 * sample flow graphs to work with.  */
#define MAKE_BLOCK_HACK

#ifdef MAKE_BLOCK_HACK

static CVMJITIRBlock*
createBlock(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* newNode = 
	CVMJITmemNew(con, JIT_ALLOC_DEBUGGING, sizeof(CVMJITIRBlock));
    return newNode;
}

/*
 * The following flow graph corresponds to the method Simple.simple(int[], int)
 */
static CVMJITIRBlock*
makeFlowGraph(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* rootblock;
    CVMJITIRBlock* block_Lpc0 = createBlock(con);
    CVMJITIRBlock* block_Lpc10 = createBlock(con);
    CVMJITIRBlock* block_Lpc28 = createBlock(con);
    CVMJITIRBlock* block_Lpc37 = createBlock(con);
    CVMJITIRBlock* block_Lpc41 = createBlock(con);
    CVMJITIRBlock* block_Lpc50 = createBlock(con);
    CVMJITIRBlock* block_Lpc51 = createBlock(con);
    rootblock = block_Lpc0;
    con->idToBlock = (CVMJITIRBlock**)calloc(8, sizeof(CVMJITIRBlock*));
 
    block_Lpc0->blockID = 2;
    con->idToBlock[block_Lpc0->blockID] = block_Lpc0;
    block_Lpc0->next = block_Lpc10;
    CVMJITsetInit(con, &block_Lpc0->successors);
    CVMJITsetAdd(con, &block_Lpc0->successors, 3); /* Lpc28 */
    CVMJITsetInit(con, &block_Lpc0->predecessors);
 
    block_Lpc10->blockID = 4;
    con->idToBlock[block_Lpc10->blockID] = block_Lpc10;
    block_Lpc10->next = block_Lpc28;
    CVMJITsetInit(con, &block_Lpc10->successors);
    CVMJITsetAdd(con, &block_Lpc10->successors, 0); /* Lpc37 */
    CVMJITsetAdd(con, &block_Lpc10->successors, 3); /* Lpc28 */
    CVMJITsetInit(con, &block_Lpc10->predecessors);
    CVMJITsetAdd(con, &block_Lpc10->predecessors, 3); /* Lpc28 */
 
    block_Lpc28->blockID = 3;
    con->idToBlock[block_Lpc28->blockID] = block_Lpc28;
    block_Lpc28->next = block_Lpc37;
    CVMJITsetInit(con, &block_Lpc28->successors);
    CVMJITsetAdd(con, &block_Lpc28->successors, 4); /* Lpc10 */
    CVMJITsetAdd(con, &block_Lpc28->successors, 5); /* Lpc41 */
    CVMJITsetInit(con, &block_Lpc28->predecessors);
    CVMJITsetAdd(con, &block_Lpc28->predecessors, 2); /* Lpc0 */
    CVMJITsetAdd(con, &block_Lpc28->predecessors, 4); /* Lpc10 */
 
    block_Lpc37->blockID = 0;
    con->idToBlock[block_Lpc37->blockID] = block_Lpc37;
    block_Lpc37->next = block_Lpc41;
    CVMJITsetInit(con, &block_Lpc37->successors);
    CVMJITsetAdd(con, &block_Lpc37->successors, 5); /* Lpc41 */
    CVMJITsetInit(con, &block_Lpc37->predecessors);
    CVMJITsetAdd(con, &block_Lpc37->predecessors, 4); /* Lpc10 */
 
    block_Lpc41->blockID = 5;
    con->idToBlock[block_Lpc41->blockID] = block_Lpc41;
    block_Lpc41->next = block_Lpc50;
    CVMJITsetInit(con, &block_Lpc41->successors);
    CVMJITsetAdd(con, &block_Lpc41->successors, 6); /* Lpc50 */
    CVMJITsetAdd(con, &block_Lpc41->successors, 7); /* Lpc51 */
    CVMJITsetInit(con, &block_Lpc41->predecessors);
    CVMJITsetAdd(con, &block_Lpc41->predecessors, 0); /* Lpc37 */
    CVMJITsetAdd(con, &block_Lpc41->predecessors, 3); /* Lpc28 */
 
    block_Lpc50->blockID = 6;
    con->idToBlock[block_Lpc50->blockID] = block_Lpc50;
    block_Lpc50->next = block_Lpc51;
    CVMJITsetInit(con, &block_Lpc50->successors);
    CVMJITsetAdd(con, &block_Lpc50->successors, 7); /* Lpc51 */
    CVMJITsetInit(con, &block_Lpc50->predecessors);
    CVMJITsetAdd(con, &block_Lpc50->predecessors, 5); /* Lpc41 */
 
    block_Lpc51->blockID = 7;
    con->idToBlock[block_Lpc51->blockID] = block_Lpc51;
    CVMJITsetInit(con, &block_Lpc51->successors);
    CVMJITsetInit(con, &block_Lpc51->predecessors);
    CVMJITsetAdd(con, &block_Lpc51->predecessors, 5); /* Lpc41 */
    CVMJITsetAdd(con, &block_Lpc51->predecessors, 6); /* Lpc50 */
    return rootblock;
}

/*
 * The following flow graph corresponds to figure 7.4 in the Muchnick book.
 */
static CVMJITIRBlock*
makeFlowGraph2(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* rootblock;
    CVMJITIRBlock* block0 = createBlock(con);
    CVMJITIRBlock* block1 = createBlock(con);
    CVMJITIRBlock* block2 = createBlock(con);
    CVMJITIRBlock* block3 = createBlock(con);
    CVMJITIRBlock* block4 = createBlock(con);
    CVMJITIRBlock* block5 = createBlock(con);
    CVMJITIRBlock* block6 = createBlock(con);
    CVMJITIRBlock* block7 = createBlock(con);
    rootblock = block0;
    con->idToBlock = (CVMJITIRBlock**)calloc(8, sizeof(CVMJITIRBlock*));
 
    block0->blockID = 0;
    con->idToBlock[block0->blockID] = block0;
    block0->next = block1;
    CVMJITsetInit(con, &block0->successors);
    CVMJITsetAdd(con, &block0->successors, 1);
    CVMJITsetInit(con, &block0->predecessors);
 
    block1->blockID = 1;
    con->idToBlock[block1->blockID] = block1;
    block1->next = block2;
    CVMJITsetInit(con, &block1->successors);
    CVMJITsetAdd(con, &block1->successors, 2); 
    CVMJITsetAdd(con, &block1->successors, 3); 
    CVMJITsetInit(con, &block1->predecessors);
    CVMJITsetAdd(con, &block1->predecessors, 0); 
 
    block2->blockID = 2;
    con->idToBlock[block2->blockID] = block2;
    block2->next = block3;
    CVMJITsetInit(con, &block2->successors);
    CVMJITsetAdd(con, &block2->successors, 7); 
    CVMJITsetInit(con, &block2->predecessors);
    CVMJITsetAdd(con, &block2->predecessors, 1);
 
    block3->blockID = 3;
    con->idToBlock[block3->blockID] = block3;
    block3->next = block4;
    CVMJITsetInit(con, &block3->successors);
    CVMJITsetAdd(con, &block3->successors, 4); 
    CVMJITsetInit(con, &block3->predecessors);
    CVMJITsetAdd(con, &block3->predecessors, 1); 
 
    block4->blockID = 4;
    con->idToBlock[block4->blockID] = block4;
    block4->next = block5;
    CVMJITsetInit(con, &block4->successors);
    CVMJITsetAdd(con, &block4->successors, 5);
    CVMJITsetAdd(con, &block4->successors, 6);
    CVMJITsetInit(con, &block4->predecessors);
    CVMJITsetAdd(con, &block4->predecessors, 3);
 
    block5->blockID = 5;
    con->idToBlock[block5->blockID] = block5;
    block5->next = block6;
    CVMJITsetInit(con, &block5->successors);
    CVMJITsetAdd(con, &block5->successors, 7);
    CVMJITsetInit(con, &block5->predecessors);
    CVMJITsetAdd(con, &block5->predecessors, 4);
 
    block6->blockID = 6;
    block6->next = block7;
    con->idToBlock[block6->blockID] = block6;
    CVMJITsetInit(con, &block6->successors);
    CVMJITsetAdd(con, &block6->successors, 4);
    CVMJITsetInit(con, &block6->predecessors);
    CVMJITsetAdd(con, &block6->predecessors, 4);

    block7->blockID = 7;
    con->idToBlock[block7->blockID] = block7;
    CVMJITsetInit(con, &block7->successors);
    CVMJITsetInit(con, &block7->predecessors);
    CVMJITsetAdd(con, &block7->predecessors, 2);
    CVMJITsetAdd(con, &block7->predecessors, 5);
    return rootblock;
}

/*
 * And this one is figure 10.13 in the dragon book
 */
static CVMJITIRBlock*
makeFlowGraph3(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* rootblock;
    CVMJITIRBlock* block1 = createBlock(con);
    CVMJITIRBlock* block2 = createBlock(con);
    CVMJITIRBlock* block3 = createBlock(con);
    CVMJITIRBlock* block4 = createBlock(con);
    CVMJITIRBlock* block5 = createBlock(con);
    CVMJITIRBlock* block6 = createBlock(con);
    CVMJITIRBlock* block7 = createBlock(con);
    CVMJITIRBlock* block8 = createBlock(con);
    CVMJITIRBlock* block9 = createBlock(con);
    CVMJITIRBlock* block10 = createBlock(con);
    CVMJITIRBlock* block11 = createBlock(con);
    CVMJITIRBlock* block12 = createBlock(con);
    rootblock = block1;
    con->idToBlock = (CVMJITIRBlock**)calloc(13, sizeof(CVMJITIRBlock*));
 
    block1->blockID = 1;
    con->idToBlock[block1->blockID] = block1;
    block1->next = block2;
    CVMJITsetInit(con, &block1->successors);
    CVMJITsetAdd(con, &block1->successors, 2); 
    CVMJITsetAdd(con, &block1->successors, 3); 
    CVMJITsetInit(con, &block1->predecessors);
    CVMJITsetAdd(con, &block1->predecessors, 9); 
 
    block2->blockID = 2;
    con->idToBlock[block2->blockID] = block2;
    block2->next = block3;
    CVMJITsetInit(con, &block2->successors);
    CVMJITsetAdd(con, &block2->successors, 3); 
    CVMJITsetInit(con, &block2->predecessors);
    CVMJITsetAdd(con, &block2->predecessors, 1);
 
    block3->blockID = 3;
    con->idToBlock[block3->blockID] = block3;
    block3->next = block4;
    CVMJITsetInit(con, &block3->successors);
    CVMJITsetAdd(con, &block3->successors, 4); 
    CVMJITsetInit(con, &block3->predecessors);
    CVMJITsetAdd(con, &block3->predecessors, 1); 
    CVMJITsetAdd(con, &block3->predecessors, 2); 
    CVMJITsetAdd(con, &block3->predecessors, 4); 
    CVMJITsetAdd(con, &block3->predecessors, 8); 
 
    block4->blockID = 4;
    con->idToBlock[block4->blockID] = block4;
    block4->next = block5;
    CVMJITsetInit(con, &block4->successors);
    CVMJITsetAdd(con, &block4->successors, 5);
    CVMJITsetAdd(con, &block4->successors, 6);
    CVMJITsetAdd(con, &block4->successors, 3);
    CVMJITsetInit(con, &block4->predecessors);
    CVMJITsetAdd(con, &block4->predecessors, 3);
    CVMJITsetAdd(con, &block4->predecessors, 7);
 
    block5->blockID = 5;
    con->idToBlock[block5->blockID] = block5;
    block5->next = block6;
    CVMJITsetInit(con, &block5->successors);
    CVMJITsetAdd(con, &block5->successors, 7);
    CVMJITsetInit(con, &block5->predecessors);
    CVMJITsetAdd(con, &block5->predecessors, 4);
 
    block6->blockID = 6;
    block6->next = block7;
    con->idToBlock[block6->blockID] = block6;
    CVMJITsetInit(con, &block6->successors);
    CVMJITsetAdd(con, &block6->successors, 7);
    CVMJITsetInit(con, &block6->predecessors);
    CVMJITsetAdd(con, &block6->predecessors, 4);

    block7->blockID = 7;
    block7->next = block8;
    con->idToBlock[block7->blockID] = block7;
    CVMJITsetInit(con, &block7->successors);
    CVMJITsetAdd(con, &block7->successors, 4);
    CVMJITsetAdd(con, &block7->successors, 8);
    CVMJITsetInit(con, &block7->predecessors);
    CVMJITsetAdd(con, &block7->predecessors, 6);
    CVMJITsetAdd(con, &block7->predecessors, 5);
    CVMJITsetAdd(con, &block7->predecessors, 10);

    block8->blockID = 8;
    block8->next = block9;
    con->idToBlock[block8->blockID] = block8;
    CVMJITsetInit(con, &block8->successors);
    CVMJITsetAdd(con, &block8->successors, 9);
    CVMJITsetAdd(con, &block8->successors, 10);
    CVMJITsetAdd(con, &block8->successors, 3);
    CVMJITsetInit(con, &block8->predecessors);
    CVMJITsetAdd(con, &block8->predecessors, 7);

    block9->blockID = 9;
    block9->next = block10;
    con->idToBlock[block9->blockID] = block9;
    CVMJITsetInit(con, &block9->successors);
    CVMJITsetAdd(con, &block9->successors, 1);
    CVMJITsetInit(con, &block9->predecessors);
    CVMJITsetAdd(con, &block9->predecessors, 8);

    block10->blockID = 10;
    block10->next = block11;
    con->idToBlock[block10->blockID] = block10;
    CVMJITsetInit(con, &block10->successors);
    CVMJITsetAdd(con, &block10->successors, 7);
    CVMJITsetAdd(con, &block10->successors, 11);
    CVMJITsetInit(con, &block10->predecessors);
    CVMJITsetAdd(con, &block10->predecessors, 8);

    block11->blockID = 11;
    block11->next = block12;
    con->idToBlock[block11->blockID] = block11;
    CVMJITsetInit(con, &block11->successors);
    CVMJITsetAdd(con, &block11->successors, 12);
    CVMJITsetInit(con, &block11->predecessors);
    CVMJITsetAdd(con, &block11->predecessors, 10);

    block12->blockID = 12;
    block12->next = NULL;
    con->idToBlock[block12->blockID] = block12;
    CVMJITsetInit(con, &block12->successors);
    CVMJITsetAdd(con, &block12->successors, 11);
    CVMJITsetInit(con, &block12->predecessors);
    CVMJITsetAdd(con, &block12->predecessors, 11);

    return rootblock;
}

static CVMJITIRBlock*
makeFlowGraph4(CVMJITCompilationContext* con)
{
    extern void CVMJITsetAddFunc(CVMJITCompilationContext*, CVMJITSet*, CVMUint32);
    CVMJITIRBlock* rootblock;
    CVMJITIRBlock* block1 = createBlock(con);
    CVMJITIRBlock* block2 = createBlock(con);
    CVMJITIRBlock* block3 = createBlock(con);
    CVMJITIRBlock* block4 = createBlock(con);
    CVMJITIRBlock* block5 = createBlock(con);
    CVMJITIRBlock* block6 = createBlock(con);
    CVMJITIRBlock* block7 = createBlock(con);
    CVMJITIRBlock* block8 = createBlock(con);
    CVMJITIRBlock* block9 = createBlock(con);
    CVMJITIRBlock* block10 = createBlock(con);
    CVMJITIRBlock* block11 = createBlock(con);
    CVMJITIRBlock* block12 = createBlock(con);
    rootblock = block1;
    con->idToBlock = (CVMJITIRBlock**)calloc(41, sizeof(CVMJITIRBlock*));
 
    block1->blockID = 29;
    con->idToBlock[block1->blockID] = block1;
    block1->next = block2;
    CVMJITsetInit(con, &block1->successors);
    CVMJITsetAddFunc(con, &block1->successors, 30); 
    CVMJITsetAddFunc(con, &block1->successors, 31); 
    CVMJITsetInit(con, &block1->predecessors);
    CVMJITsetAddFunc(con, &block1->predecessors, 37); 
 
    block2->blockID = 30;
    con->idToBlock[block2->blockID] = block2;
    block2->next = block3;
    CVMJITsetInit(con, &block2->successors);
    CVMJITsetAddFunc(con, &block2->successors, 31); 
    CVMJITsetInit(con, &block2->predecessors);
    CVMJITsetAddFunc(con, &block2->predecessors, 29);
 
    block3->blockID = 31;
    con->idToBlock[block3->blockID] = block3;
    block3->next = block4;
    CVMJITsetInit(con, &block3->successors);
    CVMJITsetAddFunc(con, &block3->successors, 32); 
    CVMJITsetInit(con, &block3->predecessors);
    CVMJITsetAddFunc(con, &block3->predecessors, 29); 
    CVMJITsetAddFunc(con, &block3->predecessors, 30); 
    CVMJITsetAddFunc(con, &block3->predecessors, 32); 
    CVMJITsetAddFunc(con, &block3->predecessors, 36); 
 
    block4->blockID = 32;
    con->idToBlock[block4->blockID] = block4;
    block4->next = block5;
    CVMJITsetInit(con, &block4->successors);
    CVMJITsetAddFunc(con, &block4->successors, 33);
    CVMJITsetAddFunc(con, &block4->successors, 34);
    CVMJITsetAddFunc(con, &block4->successors, 31);
    CVMJITsetInit(con, &block4->predecessors);
    CVMJITsetAddFunc(con, &block4->predecessors, 31);
    CVMJITsetAddFunc(con, &block4->predecessors, 35);
 
    block5->blockID = 33;
    con->idToBlock[block5->blockID] = block5;
    block5->next = block6;
    CVMJITsetInit(con, &block5->successors);
    CVMJITsetAddFunc(con, &block5->successors, 35);
    CVMJITsetInit(con, &block5->predecessors);
    CVMJITsetAddFunc(con, &block5->predecessors, 32);
 
    block6->blockID = 34;
    block6->next = block7;
    con->idToBlock[block6->blockID] = block6;
    CVMJITsetInit(con, &block6->successors);
    CVMJITsetAddFunc(con, &block6->successors, 35);
    CVMJITsetInit(con, &block6->predecessors);
    CVMJITsetAddFunc(con, &block6->predecessors, 32);

    block7->blockID = 35;
    block7->next = block8;
    con->idToBlock[block7->blockID] = block7;
    CVMJITsetInit(con, &block7->successors);
    CVMJITsetAddFunc(con, &block7->successors, 32);
    CVMJITsetAddFunc(con, &block7->successors, 36);
    CVMJITsetInit(con, &block7->predecessors);
    CVMJITsetAddFunc(con, &block7->predecessors, 34);
    CVMJITsetAddFunc(con, &block7->predecessors, 33);
    CVMJITsetAddFunc(con, &block7->predecessors, 38);

    block8->blockID = 36;
    block8->next = block9;
    con->idToBlock[block8->blockID] = block8;
    CVMJITsetInit(con, &block8->successors);
    CVMJITsetAddFunc(con, &block8->successors, 37);
    CVMJITsetAddFunc(con, &block8->successors, 38);
    CVMJITsetAddFunc(con, &block8->successors, 31);
    CVMJITsetInit(con, &block8->predecessors);
    CVMJITsetAddFunc(con, &block8->predecessors, 35);

    block9->blockID = 37;
    block9->next = block10;
    con->idToBlock[block9->blockID] = block9;
    CVMJITsetInit(con, &block9->successors);
    CVMJITsetAddFunc(con, &block9->successors, 29);
    CVMJITsetInit(con, &block9->predecessors);
    CVMJITsetAddFunc(con, &block9->predecessors, 36);

    block10->blockID = 38;
    block10->next = block11;
    con->idToBlock[block10->blockID] = block10;
    CVMJITsetInit(con, &block10->successors);
    CVMJITsetAddFunc(con, &block10->successors, 35);
    CVMJITsetAddFunc(con, &block10->successors, 39);
    CVMJITsetInit(con, &block10->predecessors);
    CVMJITsetAddFunc(con, &block10->predecessors, 36);

    block11->blockID = 39;
    block11->next = block12;
    con->idToBlock[block11->blockID] = block11;
    CVMJITsetInit(con, &block11->successors);
    CVMJITsetAddFunc(con, &block11->successors, 40);
    CVMJITsetInit(con, &block11->predecessors);
    CVMJITsetAddFunc(con, &block11->predecessors, 38);

    block12->blockID = 40;
    block12->next = NULL;
    con->idToBlock[block12->blockID] = block12;
    CVMJITsetInit(con, &block12->successors);
    CVMJITsetAddFunc(con, &block12->successors, 39);
    CVMJITsetInit(con, &block12->predecessors);
    CVMJITsetAddFunc(con, &block12->predecessors, 39);

    return rootblock;
}

#endif /* MAKE_BLOCK_HACK */

void
loopTest(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* graph1;
    CVMJITIRBlock* graph2;
    CVMJITIRBlock* graph3;
    CVMJITIRBlock* graph4;

    CVMtraceJITIROPT(("\nGRAPH 1\n\n"));
    
    graph1 = makeFlowGraph(con);
    findLoops(con, graph1, 8);
    
    /* Invalidate the DFS walk of the previous graph */
    con->dfsWalk = NULL;

    CVMtraceJITIROPT(("\nGRAPH 2\n\n"));
    
    graph2 = makeFlowGraph2(con);
    findLoops(con, graph2, 8);

    /* Invalidate the DFS walk of the previous graph */
    con->dfsWalk = NULL;

    CVMtraceJITIROPT(("\nGRAPH 3\n\n"));
    
    graph3 = makeFlowGraph3(con);
    findLoops(con, graph3, 13);
    
    /* Invalidate the DFS walk of the previous graph */
    con->dfsWalk = NULL;

    CVMtraceJITIROPT(("\nGRAPH 4\n\n"));
    
    graph4 = makeFlowGraph4(con);
    findLoops(con, graph4, 41);
    
    /* Invalidate the DFS walk of the previous graph */
    con->dfsWalk = NULL;

}

/*
 * Dump blocks, topology and all
 */
static void
dumpBlocks(CVMJITCompilationContext* con,
	   CVMJITIRBlock* rootblock, CVMUint32 numBlocks)
{
    CVMJITIRBlock* block;
    CVMJITIRBlock** dfsWalk;
    
    for (block = rootblock; block != NULL; block = block->next) {
	CVMconsolePrintf("Pred(%d): ", block->blockID);
	CVMJITsetDumpElements(con, &block->predecessors);
	CVMconsolePrintf("\t\tSucc(%d): ", block->blockID);
	CVMJITsetDumpElements(con, &block->successors);
	CVMconsolePrintf("\t\tDom(%d): ", block->blockID);
	CVMJITsetDumpElements(con, &block->dominators);
	CVMconsolePrintf("\n");
    }
    computeDfsWalkIfNeeded(con, rootblock, numBlocks);
    dfsWalk = con->dfsWalk;
    
    CVMconsolePrintf("DFS Walk: ");
    while ((block = *(dfsWalk++)) != NULL) {
	CVMconsolePrintf("%d ", block->blockID);
    }
    CVMconsolePrintf("\n");

    CVMconsolePrintf("Link-order Walk: ");
    for (block = rootblock; block != NULL; block = block->next) {
	CVMconsolePrintf("%d ", block->blockID);
    }
    CVMconsolePrintf("\n");
}

static void
dumpOneLoop(CVMJITCompilationContext* con, CVMJITLoop* loop)
{
    CVMconsolePrintf("Loop [head=%d, tail=%d] contains %d blocks: ",
		     loop->headBlock->blockID,
		     loop->tailBlock->blockID,
		     loop->numBlocks);
    CVMJITsetDumpElements(con, &loop->loopBlocks);
    CVMconsolePrintf("\n");
}

static void
dumpLoops(CVMJITCompilationContext* con)
{
    CVMUint32   i;
    for (i = 0; i < con->numLoops; i++) {
	dumpOneLoop(con, &con->loops[i]);
    }
}

static void
dumpNestedLoops(CVMJITCompilationContext* con)
{
    CVMUint32 i, j;
    for (i = 0; i < con->numNestedLoops; i++) {
	CVMJITNestedLoop* nloop = &con->nestedLoops[i];
	CVMconsolePrintf("Nested loop struct with %d loops\n",
			 nloop->numLoopsContained);
	for (j = 0; j < nloop->numLoopsContained; j++) {
	    CVMconsolePrintf("\t");
	    dumpOneLoop(con, nloop->loops[j]);
	}
	
    }
}

#endif /* CVM_DEBUG */

#endif /* Temporarily commented out. */


/*======================================================================
// Strength reduction and constant folding optimizers: 
*/

#undef getNodeOpcode
#define getNodeOpcode(node) (CVMJITgetOpcode(node) >> CVMJIT_SHIFT_OPCODE)

/* Purpose: Applies constant folding and strength reduction to ADD
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeAddExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        add v, #0               => v
        add c1, c2              => (c1+c2)
        add (add v, c1), c2     => add v, (c1+c2)
        add (add v1, c), v2     => add (add v1, v2), c
        add v1, (add v2, c)     => add (add v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs = CVMJITirnodeGetLeftSubtree(node);
    CVMJITIRNode *rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an XOR appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: add v, #0 => v */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = lhs;

        } else if (getNodeOpcode(lhs) == CVMJIT_ADD) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;

                /* Found: add (add v, c1), c2 */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1+rhsConst;

                if (newConst == 0) {
                    /* Transform: add (add v, c1), c2
                                  => add v, (c1+c2)
                                  => add v, #0 
                                  => v
                    */
                    CVMJITirnodeDeleteBinaryOp(con, node);
                    node = lhs_lhs;

                } else {
                    CVMJITIRNode *newConstNode =
                        CVMJITirnodeNewConstantJavaNumeric32(con,
                            newConst, CVM_TYPEID_INT);

                    /* Transform: add (add v, c1), c2 => add v, (c1+c2) */
                    CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                    CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                }
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: add c1, c2 => (c1+c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1+rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Applies constant folding and strength reduction to SUB
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeSubExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        sub v, #0               => v
        sub c1, c2              => (c1-c2)
        sub (sub v, c1), c2     => sub v, (c1+c2)
        sub (sub v1, c), v2     => sub (sub v1, v2), c
        sub v1, (sub v2, c)     => sub (sub v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs = CVMJITirnodeGetLeftSubtree(node);
    CVMJITIRNode *rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an XOR appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: sub v, #0 => v */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = lhs;

        } else if (getNodeOpcode(lhs) == CVMJIT_SUB) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;

                /* Found: sub (sub v, c1), c2 */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1+rhsConst;

                if (newConst == 0) {
                    /* Transform: sub (sub v, c1), c2
                                  => sub v, (c1+c2)
                                  => sub v, #0 
                                  => v
                    */
                    CVMJITirnodeDeleteBinaryOp(con, node);
                    node = lhs_lhs;

                } else {
                    CVMJITIRNode *newConstNode =
                        CVMJITirnodeNewConstantJavaNumeric32(con,
                            newConst, CVM_TYPEID_INT);

                    /* Transform: sub (sub v, c1), c2 => sub v, (c1+c2) */
                    CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                    CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                }
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: sub c1, c2 => (c1-c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1-rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Applies constant folding and strength reduction to MUL
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeMulExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        mul v, #0               => #0
        mul v, #1               => v
        mul c1, c2              => (c1*c2)
        mul (mul v, c1), c2     => add v, (c1*c2)
        mul (mul v1, c), v2     => add (add v1, v2), c
        mul v1, (mul v2, c)     => add (add v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs;
    CVMJITIRNode *rhs;

startCheck:
    lhs = CVMJITirnodeGetLeftSubtree(node);
    rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an XOR appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: mul v, #0 => #0 */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                            0, CVM_TYPEID_INT);

        } else if (rhsConst == 1) {
            /* Transform: mul v, #1 => v */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = lhs;

        } else if (getNodeOpcode(lhs) == CVMJIT_MUL) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;
                CVMJITIRNode *newConstNode;

                /* Transform: mul (mul v, c1), c2 => mul v, (c1*c2) */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1*rhsConst;
                newConstNode = CVMJITirnodeNewConstantJavaNumeric32(con,
                                    newConst, CVM_TYPEID_INT);
                CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                goto startCheck;
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: mul c1, c2 => (c1*c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1*rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Applies constant folding and strength reduction to AND
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeAndExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        and v, #0               => #0
        and c1, c2              => (c1&c2)
        and v, v                => v
        and (and v, c1), c2     => and v, (c1&c2)
        and (and v1, c), v2     => and (and v1, v2), c
        and v1, (and v2, c)     => and (and v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs = CVMJITirnodeGetLeftSubtree(node);
    CVMJITIRNode *rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an AND appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    /* TODO: use isSame() here */
    if (lhs == rhs) {
        /* Transform: and v, v => v */
        CVMJITirnodeDeleteBinaryOp(con, node);
        node = lhs;

    } else if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: and v, #0 => #0 */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con, 0,
                                                        CVM_TYPEID_INT);

        } else if (getNodeOpcode(lhs) == CVMJIT_AND) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;
                CVMJITIRNode *newConstNode;

                /* Found: and (and v, c1), c2 */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1&rhsConst;
                newConstNode = CVMJITirnodeNewConstantJavaNumeric32(con,
                                    newConst, CVM_TYPEID_INT);

                /* NOTE: We don't check for newConst == 0xffffffff because
                   it is unlikely that c1 would be 0xffffffff (both c1
                   and c2 will have to be 0xffffffff in order for newConst
                   to be 0xffffffff).
                */
                if (newConst == 0) {
                    /* Transform: and (and v, c1), c2
                                  => and v, (c1^c2)
                                  => and v, #0 
                                  => #0
                    */
                    CVMJITirnodeDeleteBinaryOp(con, node);
                    node = newConstNode;

                } else {
                    /* Transform: and (and v, c1), c2 => and v, (c1&c2) */
                    CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                    CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                }
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: and c1, c2 => (c1&c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1&rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Applies constant folding and strength reduction to OR
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeOrExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        or v, #0                => v
        or v, #0xffffffff       => #0xffffffff
        or c1, c2               => (c1|c2)
        or v, v                 => v
        or (or v, c1), c2       => or v, (c1|c2)
        or (or v1, c), v2       => or (or v1, v2), c
        or v1, (or v2, c)       => or (or v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs = CVMJITirnodeGetLeftSubtree(node);
    CVMJITIRNode *rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an OR appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    /* TODO: use isSame() here */
    if (lhs == rhs) {
        /* Transform: or v, v => v */
        CVMJITirnodeDeleteBinaryOp(con, node);
        node = lhs;

    } else if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: or v, #0 => v */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = lhs;

        } else if (getNodeOpcode(lhs) == CVMJIT_OR) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;
                CVMJITIRNode *newConstNode;

                /* Found: or (or v, c1), c2 */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1|rhsConst;
                newConstNode = CVMJITirnodeNewConstantJavaNumeric32(con,
                                    newConst, CVM_TYPEID_INT);

                /* NOTE: We don't check for newConst == 0 because it is
                   unlikely that c1 would be 0 (both c1 and c2 will have
                   to be 0 in order for newConst to be 0).  If c1 was 0,
                   it would have been transformed when the lhs expression
                   was formed in the first place:
                */
                if (newConst == 0xffffffff) {
                    /* Transform: or (or v, c1), c2
                                  => or v, (c1|c2)
                                  => or v, #0xffffffff
                                  => #0xffffffff
                    */
                    CVMJITirnodeDeleteBinaryOp(con, node);
                    node = newConstNode;

                } else {
                    /* Transform: or (or v, c1), c2 => or v, (c1|c2) */
                    CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                    CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                }
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: or c1, c2 => (c1|c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1|rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Applies constant folding and strength reduction to XOR
            expressions. */
static CVMJITIRNode *
CVMJIToptimizeXorExpression(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /* Transformation we can do:

        xor v, #0               => v
        xor c1, c2              => (c1^c2)
        xor v, v                => #0
        xor (xor v, c1), c2     => xor v, (c1^c2)
        xor (xor v1, c), v2     => xor (xor v1, v2), c
        xor v1, (xor v2, c)     => xor (xor v1, v2), c

       NOTE: We're not doing all possible transformations because in general
             this optimization seldom yields any gains.  We're only choosing
             to do the most simplest and likely cases.
    */
    CVMJITIRNode *lhs = CVMJITirnodeGetLeftSubtree(node);
    CVMJITIRNode *rhs = CVMJITirnodeGetRightSubtree(node);

    /* Most interesting cases are when the rhs is constant or an XOR appears
       in the lhs or rhs.  We want to detect failure quickly without doing too
       many checks because we don't want to waste our time doing the checks if
       it isn't going to pay:
    */
    /* TODO: use isSame() here */
    if (lhs == rhs) {
        /* Transform: xor v, v => #0 */
        CVMJITirnodeDeleteBinaryOp(con, node);
        node = CVMJITirnodeNewConstantJavaNumeric32(con, 0, CVM_TYPEID_INT);

    } else if (CVMJITnodeTagIs(rhs, CONSTANT)) {
        CVMInt32 rhsConst = (CVMInt32)CVMJITirnodeGetConstant32(rhs)->v32;
        if (rhsConst == 0) {
            /* Transform: xor v, #0 => v */
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = lhs;

        } else if (getNodeOpcode(lhs) == CVMJIT_XOR) {
            CVMJITIRNode *lhs_lhs = CVMJITirnodeGetLeftSubtree(lhs);
            CVMJITIRNode *lhs_rhs = CVMJITirnodeGetRightSubtree(lhs);
            if (CVMJITnodeTagIs(lhs_rhs, CONSTANT)) {
                CVMInt32 c1, newConst;

                /* Found: xor (xor v, c1), c2 */
                c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs_rhs)->v32;
                newConst = c1^rhsConst;

                if (newConst == 0) {
                    /* Transform: xor (xor v, c1), c2
                                  => xor v, (c1^c2)
                                  => xor v, #0 
                                  => v
                    */
                    CVMJITirnodeDeleteBinaryOp(con, node);
                    node = lhs_lhs;

                } else {
                    CVMJITIRNode *newConstNode =
                        CVMJITirnodeNewConstantJavaNumeric32(con,
                            newConst, CVM_TYPEID_INT);

                    /* Transform: xor (xor v, c1), c2 => xor v, (c1^c2) */
                    CVMJITirnodeSetLeftSubtree(con, node, lhs_lhs);
                    CVMJITirnodeSetRightSubtree(con, node, newConstNode);
                }
            }

        } else if (CVMJITnodeTagIs(lhs, CONSTANT)) {
            /* Transform: xor c1, c2 => (c1^c2) */
            CVMUint32 c1 = (CVMInt32)CVMJITirnodeGetConstant32(lhs)->v32;
            CVMJITirnodeDeleteBinaryOp(con, node);
            node = CVMJITirnodeNewConstantJavaNumeric32(con,
                               (c1^rhsConst), CVM_TYPEID_INT);
        }
    }

    return node;
}

/* Purpose: Optimizes the specified binary int expression. */
CVMJITIRNode *
CVMJIToptimizeBinaryIntExpression(CVMJITCompilationContext* con,
                                  CVMJITIRNode* node)
{
    /* Currently, we're assuming that the node that is passed in has just been
       created.  We're using this assumption to adjust all the ref counts
       accordingly.  We're also modifying the node rather than creating a new
       one.
    */
    CVMassert(node->refCount == 0);

    /* NOTE: In the following transformation code, constant operands will
       always be folded towards the top right hand corner of the expression
       tree.  We also take advantage of this to reduce the cases we check for
       below regarding what to expect at each branch. */

    /* If the node has a side effect, then don't do any optimizations on it:*/
    if (CVMJITirnodeHasSideEffects(node)) {
        return node;
    }

    /* If we get here, then we no that node and its sub-expressions do not
       have any side effects: */
    switch (getNodeOpcode(node)) {

        case CVMJIT_ADD:
            node = CVMJIToptimizeAddExpression(con, node);
            break;

        case CVMJIT_SUB:
            node = CVMJIToptimizeSubExpression(con, node);
            break;

        case CVMJIT_MUL:
            node = CVMJIToptimizeMulExpression(con, node);
            break;

        case CVMJIT_AND:
            node = CVMJIToptimizeAndExpression(con, node);
            break;

        case CVMJIT_OR:
            node = CVMJIToptimizeOrExpression(con, node);
            break;

        case CVMJIT_XOR:
            node = CVMJIToptimizeXorExpression(con, node);
            break;

        default: ;
    }

    return node;
}

/*======================================================================
// Code Sequence recognition utilities: 
*/
#ifndef USE_CDC_COM
/* Purpose: Checks if the bytecode steam at the specified location is a NOT
   expression. */
CVMBool
CVMJIToptPatternIsNotSequence(CVMJITCompilationContext* con, CVMUint8 *absPc)
{
    return CVM_FALSE;
}

/* Purpose: Optimizes the specified binary int expression. */
CVMJITIRNode *
CVMJIToptimizeUnaryIntExpression(CVMJITCompilationContext* con,
                                 CVMJITIRNode* node)
{
    return node;
}
#endif

#undef getNodeOpcode

