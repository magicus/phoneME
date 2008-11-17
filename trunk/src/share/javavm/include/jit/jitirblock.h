/*
 * @(#)jitirblock.h	1.52 06/10/10
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

#ifndef _INCLUDED_JITIRBLOCK_H
#define _INCLUDED_JITIRBLOCK_H

#include "javavm/include/jit/jitirrange.h"
#include "javavm/include/jit/jitirlist.h"
#include "javavm/include/jit/jitset.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/porting/jit/jit.h"

/*
 * Flag values for IR block
 */
#define CVMJITIRBLOCK_IS_TRANSLATED          0x1
#define CVMJITIRBLOCK_IS_EXC_HANDLER         0x2
#define CVMJITIRBLOCK_IS_JSR_HANDLER         0x4
#define CVMJITIRBLOCK_ADDRESS_FIXED          0x8
#define CVMJITIRBLOCK_NO_JAVA_MAPPING        0x10 /* An "artificial" block */
#define CVMJITIRBLOCK_IS_BACK_BRANCH_TARGET  0x20
#define CVMJITIRBLOCK_FALLS_THRU             0x40
#define CVMJITIRBLOCK_LOCALREFS_INFO_CHANGED 0x80
#define CVMJITIRBLOCK_PENDING_TRANSLATION    0x100
#define CVMJITIRBLOCK_IS_BRANCH_TARGET	     0x200
#define CVMJITIRBLOCK_IS_JSR_RETURN_TARGET   0x400
#define CVMJITIRBLOCK_IS_OOL_INVOKEINTERFACE 0x800

/*
 * And now, a block.
 */

struct CVMJITIRBlock {
    CVMJITIRBlock* next;	/* see CVMJITIRListItem */
    CVMJITIRBlock* prev;	/* see CVMJITIRListItem */

    CVMJITIRBlock* qnext;

    CVMUint16      blockPCIndex; /* range original Java bytecode PC */
    CVMUint16      blockID; /* block compact id */ 

    CVMUint16	   cseID;
    CVMUint16      refCount;

    /* Assuming 64K blocks is more than enough!
       Assume unique ID for each block.
       Our CVMJITSet works on this blockID, which probably implies
       a blockID -> CVMJITIRBlock mapping somewhere.
    */
    CVMUint32      flags;   /* IS_TRANSLATED|LOCALREFS_INFO_CHANGED|
			       IS_EXC_HANDLER|IS_JSR_TARGET|ADDRESS_FIXED|
			       NO_JAVA_MAPPING|
			       IS_BACK_BRANCH_TARGET|FALLS_THRU */

    /* Flow graph related sets */
    CVMJITSet      predecessors;
    CVMJITSet      successors;
    CVMJITSet      dominators;

    /* The set of incoming local vars containing refs */
    CVMJITSet	   localRefs;
    
    /*
     * Incoming stack values for the block.
     */
    CVMJITIRNode** phiArray; /* array of USED nodes */
    CVMInt16       phiCount; /* number of phiArray items */
    CVMUint16	   phiSize;  /* size in words of all phi items */

    CVMInt32       logicalAddress; /* normal block entry point */
    CVMInt32       gcLogicalAddress; /* entry point when gc check needed */

#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * Similar to logicalAddress, but points to the starting point if we
     * need to load incoming locals. Used if backward branch target to
     * setup the proper MAP_PC address so OSR works. Also used for
     * forward branches when if we would need to load locals at the
     * source of the branch.
     */
    CVMInt32       loadLocalsLogicalAddress;
#endif /* CVM_JIT_REGISTER_LOCALS */

    /*
     * Fixup list to instructions which branch to this block. They must
     * be patched when the code's address is determined.
     */
    CVMJITFixupElement* branchFixupList;
    CVMJITFixupElement* condBranchFixupList;

    /*
     * Incoming branches must be in the method context of the first
     * range.  Likewise, fall-thru is in the context of the last
     * "open" (unfinished) range.
     */
    CVMJITIRRange *firstRange;
    CVMJITIRRange *lastOpenRange;

    CVMJITMethodContext *inMc;
    CVMUint32	   inDepth;
    CVMUint32	   outDepth;

    CVMJITIRList rootList; /* information of all connected root nodes */

    /* Fields for artificial blocks */
    CVMMethodBlock *targetMb; /* out-of-line invoke */
    CVMBool	   doNullCheck; /* inline prologue */

    CVMJITIRNode **initialLocals;
    CVMUint32 numInitialLocals;

    /* A set of registers that the block is only allowed to
     * allocate from when register sandboxing is in effect.
     * A sandboxRegSet value of 0 means that there are no sandbox
     * registers specified i.e. register sandboxing is not in effect,
     * and there will be no restrictions on register allocation other
     * than normal register allocation rules.
     * Otherwise, only registers within the specified sandboxRegSet
     * may be allocated to new resources that aren't already
     * previously occupying registers.
     */ 
    CVMRMregset sandboxRegSet;

#ifdef CVM_JIT_REGISTER_LOCALS
#define CVMJIT_MAX_INCOMING_LOCALS 6
#define CVMJIT_MAX_SUCCESSOR_BLOCKS 4  /* best if multiple of 4 */
#define CVMJIT_MAX_ASSIGN_NODES 12     /* max per block */
    /* Array of assign nodes in the block. If the same local is assigned
     * to more than once, only the last one remains in the array. Also
     * see CVMJITCompilationContext.assignNodes. */
    CVMJITIRNode** assignNodes;
    /* locals we want to flow into block */
    CVMJITIRNode*  incomingLocals[CVMJIT_MAX_INCOMING_LOCALS];
    /* copy of incomingLocals[] made before merging with successors */
    CVMJITIRNode** originalIncomingLocals;
    /* bitmap of locals w/ fixed order (can't be moved to new slot) */
    CVMUint16      orderedIncomingLocals;
    /* stop adding to incomingLocals when do something that trashes NV regs. */
    CVMUint8       noMoreIncomingLocals;
    /* stop adding refs to incomingLocals when do something that GCs. */
    CVMUint8       noMoreIncomingRefLocals; /* stop adding refs */

    /* Successor blocks. Not necessary room for all. Once noMoreIncomingLocals
     * is true, we stop adding blocks. Fallthrough block always included
     * unless noMoreIncomingLocals. */
    CVMJITIRBlock* successorBlocks[CVMJIT_MAX_SUCCESSOR_BLOCKS];
    /* The block's incomingLocalsCount for each successorBlock added. This
     * allows us to track the order incomingLocals are added relative to
     * successorBlocks[]. */
    CVMUint8 successorBlocksLocalOrder[CVMJIT_MAX_SUCCESSOR_BLOCKS];
    /* Like successorBlocksLocalOrder[], but for assignNodes[]. */
    CVMUint8 successorBlocksAssignOrder[CVMJIT_MAX_SUCCESSOR_BLOCKS];
    /* Boolean indicates if it is ok to merge from successor block.
     * Set to !noMoreIncomingLocals when block is added. */
    CVMUint8  okToMergeSuccessorBlock[CVMJIT_MAX_SUCCESSOR_BLOCKS];
    /* Boolean indicates if it is ok to merge refs from successor block.
     * Set to !noMoreIncomingLocalsRefs when block is added. */
    CVMUint8  okToMergeSuccessorBlockRefs[CVMJIT_MAX_SUCCESSOR_BLOCKS];

    CVMUint8       incomingLocalsCount; /* index into incomingLocals[] */
    CVMUint8       originalIncomingLocalsCount;
    CVMUint8       successorBlocksCount; /* index into successorBlocks[] */
    CVMUint8       assignNodesCount; /* index into assignNodes[] */
#endif /* CVM_JIT_REGISTER_LOCALS */
   
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    CVMInt32	   mtIndex;
    CVMInt32	   oolReturnAddress;
    /* default value for mtIndex and outOfLine */
#define CVMJIT_IAI_VIRTUAL_INLINE_CB_TEST_DEFAULT  -1
    CVMMethodBlock* candidateMb;
#endif
/* IAI - 20 */
};

/*
 * The queue is threaded through the block's qnext field, so
 * a block can only be on one queue, and it cannot be on the
 * queue more than once.
 */
typedef struct {
    CVMJITIRBlock *head;
    CVMJITIRBlock *tail;
} CVMJITIRBlockQueue;

typedef struct CVMJITJsrRetEntry CVMJITJsrRetEntry;
struct CVMJITJsrRetEntry {
    CVMJITJsrRetEntry* next;     /* see CVMJITIRListItem */
    CVMJITJsrRetEntry* prev;     /* see CVMJITIRListItem */

    CVMUint32       jsrTargetPC; /* Jsr target PC */
    CVMJITIRBlock*  jsrRetBk;    /* Jsr return target */
};

#define CVMJITjsrRetEntryGetNext(entry) \
    ((CVMJITJsrRetEntry *)(entry)->next)
#define CVMJITjsrRetEntryGetPrev(entry) \
    ((CVMJITJsrRetEntry *)(entry)->prev)
#define CVMJITjsrRetEntrySetNext(entry, value) \
    (*(CVMJITJsrRetEntry **)&(entry)->next = (value))
#define CVMJITjsrRetEntrySetPrev(entry, value) \
    (*(CVMJITJsrRetEntry **)&(entry)->prev = (value))

/*
 * CVMJITIRBlock macro defines and interface APIs
 */

/* Set members */
#define CVMJITirblockSetDominators(bk, dominator) \
    (CVMJITirblockGetDominators(bk) = dominator)

#define CVMJITirblockSetBlockPC(bk, pc) \
    (CVMJITirblockGetBlockPC(bk) = (pc))
#define CVMJITirblockSetBlockID(bk, id) \
    (CVMJITirblockGetBlockID(bk) = (id))
#define CVMJITirblockSetCseID(bk, cseID) \
    (CVMJITirblockGetCseID(bk) = (cseID))
#define CVMJITirblockSetRefCount(bk, rcnt) \
    (CVMJITirblockGetRefCount(bk) = (rcnt))
#define CVMJITirblockSetFlags(bk, flags) \
    (CVMJITirblockGetflags(bk) = (flags))
#define CVMJITirblockAddFlags(bk, flags) \
    (CVMJITirblockGetflags(bk) |= (flags))
#define CVMJITirblockClearFlags(bk, flags) \
    (CVMJITirblockGetflags(bk) &= ~(flags))
#define CVMJITirblockSetNext(bk, new_bk) \
    (*(CVMJITIRBlock **)&(bk)->next = (new_bk))
#define CVMJITirblockSetPrev(bk, new_bk) \
    (*(CVMJITIRBlock **)&(bk)->prev = (new_bk))

#define CVMJITirblockInitRootList(con, bk) \
    (CVMJITirlistInit(con, &(bk)->rootList))

/* Get members */
#define CVMJITirblockGetBlockPC(bk)	     ((bk)->blockPCIndex)
#define CVMJITirblockGetBlockID(bk)	     ((bk)->blockID)
#define CVMJITirblockGetCseID(bk)	     ((bk)->cseID)
#define CVMJITirblockGetRefCount(bk)	     ((bk)->refCount)
#define CVMJITirblockGetflags(bk)	     ((bk)->flags)
#define CVMJITirblockGetPredecessors(bk)     ((bk)->predecessors)
#define CVMJITirblockGetSuccessors(bk)	     ((bk)->successors)
#define CVMJITirblockGetDominators(bk)	     ((bk)->dominators)
#define CVMJITirblockGetNext(bk) \
    ((CVMJITIRBlock *)(bk)->next)
#define CVMJITirblockGetPrev(bk) \
    ((CVMJITIRBlock *)(bk)->prev)

#define CVMJITirblockGetRootList(bk)	     (&(bk)->rootList)

#define CVMJITirblockIncCseID(bk)            ((bk)->cseID++)
#define CVMJITirblockIncRefCount(bk)         ((bk)->refCount++)
#define CVMJITirblockDecRefCount(bk)         ((bk)->refCount--)

#define CVMJITirblockIsExcHandler(bk) \
    ((CVMJITIRBLOCK_IS_EXC_HANDLER & (bk)->flags) != 0)
#define CVMJITirblockFallsThru(bk) \
    ((CVMJITIRBLOCK_FALLS_THRU & (bk)->flags) != 0)
#define CVMJITirblockIsTranslated(bk) \
    ((CVMJITIRBLOCK_IS_TRANSLATED & (bk)->flags) != 0)
#define CVMJITirblockIsJSRTarget(bk) \
    ((CVMJITIRBLOCK_IS_JSR_HANDLER & (bk)->flags) != 0)
#define CVMJITirblockIsBranchTarget(bk) \
    ((CVMJITIRBLOCK_IS_BRANCH_TARGET & (bk)->flags) != 0)
#define CVMJITirblockIsBackwardBranchTarget(bk) \
    ((CVMJITIRBLOCK_IS_BACK_BRANCH_TARGET & (bk)->flags) != 0)
#define CVMJITirblockLocalRefsInfoChanged(bk) \
    ((CVMJITIRBLOCK_LOCALREFS_INFO_CHANGED & (bk)->flags) != 0)
#define CVMJITirblockIsJSRReturnTarget(bk) \
    ((CVMJITIRBLOCK_IS_JSR_RETURN_TARGET & (bk)->flags) != 0)

/*
 * This block has been created with no Java mapping
 */
#define CVMJITirblockIsArtificial(bk) \
    ((CVMJITIRBLOCK_NO_JAVA_MAPPING & bk->flags) != 0)

#define CVMJITirblockIsOOLInvokeInterface(bk) \
    ((CVMJITIRBLOCK_IS_OOL_INVOKEINTERFACE & bk->flags) != 0)

#define CVMJITirblockIsLastJavaBlock(bk)			\
    (CVMJITirblockGetNext(bk) == NULL ||			\
     con->mc != CVMJITirblockGetNext(bk)->inMc)

#define CVMJITirblockMaxPC(bk, codeLength)			\
    (CVMJITirblockIsLastJavaBlock(bk) ?  			\
     codeLength :						\
     CVMJITirblockGetBlockPC(CVMJITirblockGetNext(bk)))		\

#define CVMJITirblockIsPCInBlockRange(bk, codeLength, pc) \
    (pc <= CVMJITirblockMaxPC(bk, codeLength))

extern CVMJITIRBlock*
CVMJITirblockCreate(CVMJITCompilationContext* con, CVMUint16 pc);

extern CVMJITIRBlock*
CVMJITirblockSplit(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
		   CVMUint16 headPc);

/* Create a label, but not necessarily a branch target */
extern CVMJITIRBlock*
CVMJITirblockNewLabelForPC0(CVMJITCompilationContext* con, CVMUint16 pc);

/* Create a label and mark as branch target */
extern CVMJITIRBlock*
CVMJITirblockNewLabelForPC(CVMJITCompilationContext* con, CVMUint16 pc);

extern void
CVMJITirblockConnectBlocksInOrder(CVMJITCompilationContext* con);

extern void
CVMJITirblockMarkAllPCsThatNeedMapping(CVMJITCompilationContext* con);

extern void
CVMJITirblockFindAllExceptionLabels(CVMJITCompilationContext* con);

extern void
CVMJITirblockFindAllNormalLabels(CVMJITCompilationContext* con);

extern void
CVMJITirblockAtHandlerEntry(CVMJITCompilationContext* con, 
    CVMJITIRBlock* bk);

extern void 
CVMJITirblockAtLabelEntry(CVMJITCompilationContext* con, CVMJITIRBlock* bk);

extern CVMBool
CVMJITirblockMergeLocalRefs(CVMJITCompilationContext* con,
			    CVMJITIRBlock* targetbk);

extern CVMBool
CVMJITirblockConnectFlow(CVMJITCompilationContext* con,
			 CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk);

extern void
CVMJITirblockAtBranch(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk,
		      CVMJITIRNode* expression1ToEvaluate,
		      CVMJITIRNode* expression2ToEvaluate,
		      CVMJITIRNode* branchNode,
		      CVMBool unconditionalBranch);

void
CVMJITirblockNoIncomingLocalsAllowed(CVMJITIRBlock* bk);

#ifdef CVM_JIT_REGISTER_LOCALS
CVMInt32
CVMJITirblockFindIncomingLocal(CVMJITIRBlock* bk, CVMUint16 localNo);
#endif

#define CVMJITlocalrefsInit(con, refs)		\
    CVMJITsetInvalidate((refs))

#define CVMJITlocalrefsCopy(con, dest, src)	\
    CVMJITsetCopy((con), (dest), (src))

#ifdef CVM_DEBUG_ASSERTS
#define CVMJITlocalrefsIsRef(refs, num)		\
    CVMJITsetContainsFunc((refs), (num))
#endif

#define CVMJITlocalrefsSetRef(con, refs, num)	\
    CVMJITsetAdd((con), (refs), (num));

#define CVMJITlocalrefsSetValue(con, refs, num, isDoubleWord)	\
{								\
    CVMJITsetRemove((con), (refs), (num));			\
    if (isDoubleWord) {						\
	CVMJITsetRemove((con), (refs), (num) + 1);		\
    }								\
}

#define CVMJITlocalrefsDumpRefs(con, refs)	\
    CVMJITsetDumpElements((con), (refs))

/* 
 * CVMJITIRBlock block stack operations
 */

#define CVMJITirblockQueueInit(q) { 	\
    (q)->head = NULL;			\
    (q)->tail = NULL;			\
}

#define CVMJITirblockQueueEmpty(q) 	\
    ((q)->head == NULL)

#define CVMJITirblockEnqueue(q, bk) {			\
    if ((q)->head == NULL) {				\
	(q)->head = (bk);				\
    } else {						\
	(q)->tail->qnext = (bk);			\
    }							\
    (q)->tail = (bk);					\
    (bk)->qnext = NULL;					\
}

#define CVMJITirblockDequeue(q, bk) {			\
    (bk) = (q)->head;					\
    if ((bk) != NULL) {					\
	(q)->head = (bk)->qnext;			\
    }							\
}

#endif /* _INCLUDED_JITIRBLOCK_H */
