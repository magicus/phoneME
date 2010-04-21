/*
 * @(#)jitir.c	1.316 06/10/25
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
#include "javavm/include/typeid.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/clib.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/jit/jit.h"

#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirdump.h"
#include "javavm/include/jit/jitopt.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirlist.h"
#include "javavm/include/jit/jitirrange.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitdebug.h"
#include "javavm/include/jit/jitasmconstants.h"

#ifdef CVM_HW
#include "include/hw.h"
#endif

/*#define CVM_DEBUG_RUNTIME_CHECK_ELIMINATION*/
/*#define LOCALREF_INFO_REFINEMENT_DEBUGGING*/

/* NOTES on:

   The Compilation Context and Method Contexts
   ===========================================
   When a method is being compiled, we shall call this the root method.  There
   is only one Compilation Context per compilation pass i.e. per root method.
   However, during the compilation of that method, we may inline other methods
   that it calls.  For each of these inlined methods, a Method Context will be
   assigned.

   The Compilation Context contains a Method Contexts stack.  Each time a
   method is inlined, a Method Context is pushed onto that stack.  When we are
   done inlining that method, its Method Context will be popped.

   The root method also has a Method Context.  It is easy to think of Method
   Contexts as being equivalent to stack frames on the the runtime Java stack
   since it sort of keeps track of which method is being "translated".

   However, this is not entirely true.  Unlike stack frames, the Method Context
   is not popped off when we reach a return opcode.  Instead, it is popped of
   when we are done processing the inlined method.  This is because we need to
   translate the entire method, and the return opcode is not guaranteed to be
   the last opcode we will encounter for that method.

   All blocks of an inlined method or root method are translated using the
   same Method Context.  But each Method Context has a locals[] array, and it
   is re-initialized every time we start up the translation of another block.

   Root Nodes and the CVMJITirDoSideEffectOperator()
   =================================================
   Every time a root node is to be added, we must make sre that the
   CVMJITirDoSideEffectOperator() has been called at an appropriate prior time
   to ensure that evaluation order of operands on the stack are preserved.
   There is an assertion in the root node constructor CVMJITirnodeNewRoot()
   that will ensure that all items on the operand stack has been evaluated
   before the root node is inserted.
*/

typedef CVMJITInlinePreScanInfo Inlinability;

#define CVMJITlocalStateMarkPC(con, mc, localNo, pc) {        \
    CVMassert((localNo) < (mc)->localsSize);                  \
    CVMassert((mc)->localsState != NULL);                     \
    {                                                         \
	if ((mc)->localsState[(localNo)] == 0) {              \
            (mc)->localsState[(localNo)] = (pc);              \
        } else if ((mc)->localsState[(localNo)] != (pc)) {    \
            CVMJITerror(con, CANNOT_COMPILE,                  \
	        "This local is already used by anther JSR");  \
	}                                                     \
    }                                                         \
}
#define CVMJITlocalStateGetPC(mc, localNo)                \
    (CVMassert((mc)->localsState[(localNo)] != 0),        \
     ((mc)->localsState[(localNo)]))

#undef getLocalNo
#define  getLocalNo(node) \
    (CVMJITirnodeGetLocal(CVMJITirnodeValueOf(node))->localNo)

static CVMJITIRNode*
arrayLengthCacheGet(CVMJITCompilationContext* con, CVMJITIRNode* objRef);
static void
arrayLengthCachePut(CVMJITCompilationContext* con, CVMJITIRNode* arrayrefNode,
                    CVMJITIRNode* arrayLengthNode);
static void
arrayLengthCacheKillForLocal(CVMJITCompilationContext* con, CVMUint32 localNo);

static CVMJITMethodContext*
pushMethodContext(CVMJITCompilationContext* con, 
		  CVMMethodBlock* mb,
		  Inlinability inlineStatus,
		  CVMBool doNullCheck);

static void
spliceContextBlocks(CVMJITCompilationContext* con,
    CVMJITMethodContext *mc, CVMMethodBlock *targetMb,
    CVMBool doNullCheck);

static CVMBool
translateInlinePrologue(CVMJITCompilationContext* con,
    CVMJITIRBlock* currBlock,
    CVMJITIRBlock* collBlock);

static CVMBool 
translateOutOfLineInvoke(CVMJITCompilationContext* con,
    CVMJITIRBlock* invBlock);

static void
refineLocalsInfo(CVMJITCompilationContext* con);

#ifdef CVM_JIT_REGISTER_LOCALS
/* Returns true if the block has seen an ASSIGN node for the local */
static CVMBool
assignNodeExists(CVMJITIRNode** assignNodes, CVMUint16 assignNodesCount,
		 CVMUint16 localNo);

/* Add target block to list of blocks we flow incoming locals from */
static void
addIncomingLocalsSuccessorBlock(CVMJITCompilationContext* con,
				CVMJITIRBlock* curbk,
				CVMJITIRBlock* successorBk,
				CVMBool fallthrough);
#endif

static CVMJITIRBlock**
CVMJITirblockMapCreate(CVMJITCompilationContext *con, CVMMethodBlock *mb)
{
    /* Allocate an extra slot at the end for a "return" block if needed */
    return (CVMJITIRBlock**)CVMJITmemNew(con,
	JIT_ALLOC_IRGEN_OTHER,
	(CVMmbCodeLength(mb) + 1) * sizeof(CVMJITIRBlock*));
}


/*
 * Java treats byte, boolean, char, and shorts as int, so we fold all
 * of these types into the type int. Basically we fold into the known
 * set of "computational types" as the VM Spec likes to call them.
 */
static CVMUint8
CVMJITfoldType(CVMUint8 typeTag)
{
    switch (typeTag) {
    case CVM_TYPEID_BYTE:
    case CVM_TYPEID_BOOLEAN:
    case CVM_TYPEID_CHAR:
    case CVM_TYPEID_SHORT:
	return CVM_TYPEID_INT;
    default:
	return typeTag;
    }
}

static void
nullCheckMark(CVMJITCompilationContext* con, CVMJITIRNode* node);

static CVMBool
nullCheckEmitted(CVMJITCompilationContext* con, CVMJITIRNode* node);

static CVMBool
nullCheckNeeded(CVMJITCompilationContext* con, CVMJITIRNode* node);

/*
 * Kill all cached field and array reads, and any derived pointers
 */
static void
killAllCachedReferences(CVMJITCompilationContext* con);

static CVMJITIRNode *
monitorEnter(CVMJITCompilationContext* con,
	     CVMJITIRBlock* curbk,
	     CVMJITIRNode *node)
{

    /* Synchronization points invalidate cached field results */
    killAllCachedReferences(con);
    
    /* monitorenter inserts a root node.  So we must call the
       CVMJITirDoSideEffectOperator() to preserve eval order of
       operands on the stack: */
    CVMJITirDoSideEffectOperator(con, curbk);
    node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_MONITOR_ENTER,
				  node); 
    CVMJITirnodeSetThrowsExceptions(node);
    return node;
}

static CVMJITIRNode *
monitorExit(CVMJITCompilationContext* con,
	    CVMJITIRBlock* curbk,
	    CVMJITIRNode *node)
{

    /* Synchronization points invalidate cached field results */
    killAllCachedReferences(con);
    
    /* monitorexit inserts a root node.  So we must call the
       CVMJITirDoSideEffectOperator() to preserve eval order of
       operands on the stack: */
    CVMJITirDoSideEffectOperator(con, curbk);
    node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_MONITOR_EXIT,
				  node); 
    CVMJITirnodeSetThrowsExceptions(node);
    return node;
}

/*
 * Create a new root node that marks the beginning of an inlined method
 */
static void
beginInlining(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
	      CVMUint16 tag, CVMJITMethodContext* mc)
{
    CVMJITIRNode* mcConst = CVMJITirnodeNewConstantMC(con, mc);
    CVMJITIRNode* info;
    info = CVMJITirnodeNewUnaryOp(con, tag, mcConst);
    CVMJITirnodeSetHasUndefinedSideEffect(info);
    CVMJITirnodeNewRoot(con, curbk, info);
}

/* 
 * Create a node that marks the end of an inlined method. 
 */
static CVMJITIRNode*
endInlining(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
	    CVMJITIRNode* arg)
{
    /* Another inlining performed */
    con->numInliningInfoEntries++;

    if (arg == NULL) {
	CVMJITIRRoot *prevRoot = 
	    (CVMJITIRRoot*)CVMJITirlistGetTail(CVMJITirblockGetRootList(curbk));
	CVMJITIRNode* prevExpr = (prevRoot == NULL) ? NULL :
	    CVMJITirnodeGetRootOperand(prevRoot);
	if (prevExpr != NULL && CVMJITirnodeIsEndInlining(prevExpr)) {
	    ++CVMJITirnodeGetNull(prevExpr)->data;
	} else {
	    CVMJITIRNode *eiNode =
		CVMJITirnodeNewNull(con, CVMJIT_ENCODE_END_INLINING_LEAF);
	    CVMJITirnodeGetNull(eiNode)->data = 1;
	    CVMJITirnodeNewRoot(con, curbk, eiNode);
	}
	return NULL;
    } else {
	CVMUint16 tid = CVMJITgetTypeTag(arg);
	CVMJITIRNode *eiNode =
	    CVMJITirnodeNewNull(con, CVMJIT_ENCODE_END_INLINING_LEAF);
	CVMJITirnodeGetNull(eiNode)->data = 1;
	eiNode = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_SEQUENCE_L(tid),
	    arg, eiNode);
	CVMJITirnodeSetHasUndefinedSideEffect(eiNode);
	return eiNode;
    }
}

/* Purpose: Compute the number of operands on the operands stack that the
            specified argSize correlates to starting from the current top of
            the operand stack. */
static CVMUint32
argSize2argCount(CVMJITCompilationContext *con, CVMUint16 argSize)
{
    CVMJITStack *operandStack = con->operandStack;
    CVMUint32 currentIndex = CVMJITstackCnt(con, con->operandStack) - 1;
    CVMUint32 argCount;

    /* Compute the starting stack index (exclude args): */
    while (argSize > 0) {
        CVMJITIRNode *node;
        node = CVMJITstackGetElementAtIdx(con, operandStack, currentIndex);
        currentIndex--;
        if (CVMJITirnodeIsDoubleWordType(node)) {
            argSize -= 2;
        } else {
            argSize --;
        }
    }
    argCount = CVMJITstackCnt(con, con->operandStack) - (currentIndex + 1);
    return argCount;
}

/*
 * Evaluate a stack item and bump refcount.
 */
void
CVMJITirForceEvaluation(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
			CVMJITIRNode* node)
{
    /*
     * If the node has already been evaluated, there's no point in forcing
     * evaluation of it.  A node can only be evaluated once though its
     * result may be used many times.
     * CONSTANT nodes are automatically considered to be
     * already evaluated by definition.
     */
    if (!CVMJITirnodeHasBeenEvaluated(node)) {
	CVMJITirnodeNewRoot(con, curbk, node);
	/* From this point, this tree has been evaluated: */
        CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_TEMP_NODES);
    }
}

#ifdef CVM_JIT_REGISTER_LOCALS

/*
 * Add this local to our list of incoming locals if there is room
 * and we are still allowing more items to be added to the list. We
 * stop adding if we know the incoming local will be trashed
 * before referenced (like if a method call is made).
 */
static void
addIncomingLocal(CVMJITCompilationContext* con, CVMJITIRBlock* bk,
		 CVMJITIRNode* localNode)
{
    CVMUint16 typeTag = CVMJITgetTypeTag(localNode);
    /*
     * Check all the conditions for which we will not allow the
     * local to be added.
     */
    if (
	/* allow longs only if they fit in one register */
#if (CVMCPU_MAX_REG_SIZE == 1)
	!(typeTag == CVM_TYPEID_LONG) &&
#endif
	/* allow doubles only if they fit in one register */
#if !defined(CVM_JIT_USE_FP_HARDWARE) || (CVMCPU_FP_MAX_REG_SIZE == 1)
	!(typeTag == CVM_TYPEID_DOUBLE) &&
#endif
	!(typeTag == CVM_TYPEID_OBJ && bk->noMoreIncomingRefLocals) &&
	!bk->noMoreIncomingLocals && 
	bk->incomingLocalsCount < CVMJIT_MAX_INCOMING_LOCALS)
    {
	int i;
	CVMUint16 localNo = getLocalNo(localNode);
	/* don't add the same local twice */
	for (i = 0; i < bk->incomingLocalsCount; i++) {
	    if (bk->incomingLocals[i] != NULL) {
		if (getLocalNo(bk->incomingLocals[i]) == localNo) {
		    return;  /* local already been added */
		}
	    }
	}
	/* don't add locals for which there is an ASSIGN node already */
	if (assignNodeExists(con->assignNodes,
			     con->assignNodesCount, localNo)) {
	    return;
	}

	bk->incomingLocals[bk->incomingLocalsCount] = localNode;
	bk->incomingLocalsCount++;
    }
}
#endif /* CVM_JIT_REGISTER_LOCALS */

/*
 * Handle local access. If we are in the process of inlining, go through
 * the "method context". If not, create a new LOCAL node and push it on the
 * stack.
 */
static CVMJITIRNode*
getLocal(CVMJITCompilationContext* con,
	 CVMJITMethodContext* mc, CVMUint16 localNo, CVMUint16 typeTag) 
{
    CVMJITIRNode* loc;
    CVMassert(localNo < mc->localsSize);
    loc = mc->locals[localNo];
    if (loc == NULL) {
	CVMInt32 mappedLocalNo = mc->firstLocal + localNo;

	CVMassert(mappedLocalNo < mc->currLocalWords);
	CVMassert(mappedLocalNo >= mc->currLocalWords - mc->nOwnLocals);
#if 1
	/* These are the same, so one of them should probably be removed */
	CVMassert(mc->nOwnLocals == mc->localsSize);
#endif
	loc = CVMJITirnodeNewLocal(con, CVMJIT_ENCODE_LOCAL(typeTag),
	                           mappedLocalNo);

	CVMassert(mc->compilationDepth <= mc->currBlock->inDepth);
	CVMassert(mappedLocalNo < mc->currBlock->numInitialLocals);
	mc->currBlock->initialLocals[mappedLocalNo] = loc;
        CVMJITirnodeSetIsInitialLocal(loc);

	mc->locals[localNo] = loc;
        mc->physLocals[localNo] = loc;
        CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_LOCAL_NODES);
#ifdef CVM_JIT_REGISTER_LOCALS
	addIncomingLocal(con, mc->currBlock, loc);
	/* Add node decoration for better register targeting in backend.
	 * Note that successorBlocksIdx is set to the *next* block we will
	 * branch to, not the previous block. It is used by the backend
	 * so it can find out what register the local is expected to be in
	 * in the next block we may branch to.
	 */
	loc->decorationType = CVMJIT_LOCAL_DECORATION;
	loc->decorationData.successorBlocksIdx =
	    mc->currBlock->successorBlocksCount;
#endif
#ifdef CVM_DEBUG_ASSERTS
	if (typeTag == CVM_TYPEID_OBJ) {
	    CVMassert(CVMJITlocalrefsIsRef(&con->curRefLocals, mappedLocalNo));
	}
#endif
    }

    return loc;
}

static void
pushLocal(CVMJITCompilationContext* con,
	  CVMJITMethodContext* mc, CVMUint16 localNo, CVMUint16 typeTag) 
{
    CVMJITirnodeStackPush(con, getLocal(con, mc, localNo, typeTag));
}

/*
 * Create various flavors of CVMJITConstant32 and CVMJITConstant64 nodes
 * and push them on the stack.
 */
static void
pushConstantJavaFloat(CVMJITCompilationContext* con, CVMJavaFloat f)
{
    CVMJavaVal32 val32;
    CVMJITIRNode* node;
    val32.f = f;
    node = 
	CVMJITirnodeNewConstantJavaNumeric32(con, val32.i,
					     CVM_TYPEID_FLOAT); 
    CVMJITirnodeStackPush(con, node);
}

static void
pushConstantJavaInt(CVMJITCompilationContext* con, CVMJavaInt i)
{
    CVMJITIRNode* node = 
	CVMJITirnodeNewConstantJavaNumeric32(con, i, CVM_TYPEID_INT); 
    CVMJITirnodeStackPush(con, node);
}

static void
pushConstant32(CVMJITCompilationContext* con, CVMJavaInt i)
{
    CVMJITIRNode* node = 
	CVMJITirnodeNewConstantJavaNumeric32(con, i, CVMJIT_TYPEID_32BITS); 
    CVMJITirnodeStackPush(con, node);
}

static void
pushConstantStringICell(CVMJITCompilationContext* con, CVMStringICell* str)
{
    CVMJITIRNode* node = CVMJITirnodeNewConstantStringICell(con, str); 
    CVMJITirnodeStackPush(con, node);
}

static void
pushConstantStringObject(CVMJITCompilationContext* con, CVMStringObject* str)
{
    CVMJITIRNode* node = CVMJITirnodeNewConstantStringObject(con, str); 
    CVMJITirnodeStackPush(con, node);
}

static void
pushConstantJavaVal64(CVMJITCompilationContext* con, CVMJavaVal64* val64,
		      CVMUint8 typeTag)
{
    CVMJITIRNode* node = 
	CVMJITirnodeNewConstantJavaNumeric64(con, val64, typeTag); 
    CVMJITirnodeStackPush(con, node);
}

/* 
 * Force evaluation of expressions on the operand stack if they
 * have side effects, but leave them on the stack instead of
 * popping them.
 */
void
CVMJITirDoSideEffectOperator(CVMJITCompilationContext* con,
                             CVMJITIRBlock* curbk)
{
    CVMJITStack* operandStack = con->operandStack;
    CVMUint32 stkCnt;

    CVMwithAssertsOnly({ con->operandStackIsBeingEvaluated = CVM_TRUE; });
    for (stkCnt = 0; stkCnt < CVMJITstackCnt(con, operandStack); stkCnt++) {
        CVMJITIRNode* stkNode =
	    CVMJITstackGetElementAtIdx(con, operandStack, stkCnt);

        if (!CVMJITirnodeHasBeenEvaluated(stkNode) &&
            CVMJITirnodeHasSideEffects(stkNode)) {
            /* Make sure this node gets evaluated. */
            CVMJITirForceEvaluation(con, curbk, stkNode);
        }
    } /* end of for loop */
    CVMwithAssertsOnly({ con->operandStackIsBeingEvaluated = CVM_FALSE; });
}

/*
 * We are about to do something that might throw an exception,
 * so we "flow" into our exception handlers.
 *
 * Return CVM_TRUE if the merge causes any changes, CVM_FALSE
 * otherwise.
 */
static CVMBool
connectFlowToExcHandlers(CVMJITCompilationContext* con,
			 CVMJITMethodContext* mc,
			 CVMUint16 curPC, CVMBool flushLocals)
{
    CVMBool change = CVM_FALSE;
    CVMBool done = CVM_FALSE;
    CVMJITIRBlock *curbk = con->mc->currBlock;

    /* We begin at the current translation context and work our way
       back through our callers.  This is a general solution for the
       time when we allow inlined methods to have exception handlers.
     */
    do {
	CVMJavaMethodDescriptor* jmd = mc->jmd;
	CVMExceptionHandler *eh = CVMjmdExceptionTable(jmd);
	CVMExceptionHandler *ehEnd = eh + CVMjmdExceptionTableLength(jmd);

	/* Check each exception handler to see if the pc is in its range */
	for (; eh < ehEnd; eh++) { 
	    if (curPC >= eh->startpc && curPC < eh->endpc) {
		/* Merge into exception handlers in range */
		CVMJITIRBlock* handlerBlock = mc->pcToBlock[eh->handlerpc];
		CVMassert(handlerBlock != NULL);

		/* Flush locals during translation, but not during
		   refinement pass */
		if (flushLocals) {
		    CVMJITirFlushOutBoundLocals(con, curbk, mc, CVM_TRUE);
		    /* We only need to flush locals once, at the
		       "innermost" context.  The flush operation
		       deals intelligently with caller contexts. */
		    flushLocals = CVM_FALSE;
		}

		/* Connect flow, which also merges local refs.  Keep track
		   if the local refs have changed in any target blocks. */
		change |= CVMJITirblockConnectFlow(con, curbk, handlerBlock);

		/* If the handler catches everything, then we are done.  No
		   need to do flow for enclosing contexts. */
		if (eh->catchtype == 0) {
		    done = CVM_TRUE;
		}
	    }
	}
	/* For exception purposes, the PC for the caller context is where
	   the invocation opcode was located. */
	curPC = mc->invokePC;
	mc = mc->caller;
    } while (mc != NULL && !done);

    return change;
}

static CVMBool
gotoBranchToTargetBlock(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
			CVMJITIRBlock* targetBlock, CVMUint16 opcodeTag,
			CVMBool fallthroughOK);

/*
 * conditional branch
 */
static CVMBool
condBranchToTargetBlock(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
			CVMUint16 typeTag, CVMJITCondition condition,
			CVMJITIRNode* rhsNode, CVMJITIRNode* lhsNode,
			CVMJITIRBlock* targetbk, CVMUint8 flags)
{
    CVMJITIRNode *lhs = CVMJITirnodeValueOf(lhsNode);
    CVMJITIRNode *rhs = CVMJITirnodeValueOf(rhsNode);

    /* Check to see if the condition for this branch is in fact a NULL check.
       nullCheck will be TRUE if the condition is a NULL check: */
    CVMBool nullCheck =
	(condition == CVMJIT_EQ &&
	CVMJITirnodeIsReferenceType(lhs) &&
	CVMJITirnodeIsConstantAddrNode(rhs) &&
	CVMJITirnodeGetConstantAddr(rhs)->vAddr == (CVMAddr)NULL);

    /* If the condition is a NULL check and we know that the lhs node is never
       NULL, then the branch will never happen and we don't need to emit code
       for it.  However, we still need to make sure that the operands for the
       condition are fully evaluated if they can have noticeable side effects.
    */
    if (nullCheck && !nullCheckNeeded(con, lhs)) {
        if (CVMJITirnodeHasSideEffects(lhsNode) ||
            CVMJITirnodeHasSideEffects(rhsNode)) {
            CVMJITirDoSideEffectOperator(con, curbk);
	    if (CVMJITirnodeHasSideEffects(lhsNode)) {
                CVMJITirForceEvaluation(con, curbk, lhsNode);
	    }
	    if (CVMJITirnodeHasSideEffects(rhsNode)) {
                CVMJITirForceEvaluation(con, curbk, rhsNode);
	    }
        }
	/* No need to check again! */
	CVMJITstatsRecordInc(con, CVMJIT_STATS_NULL_CHECKS_ELIMINATED);
	return CVM_TRUE;
    }

    /* We only handle integer compares for now */
    if (typeTag == CVM_TYPEID_INT &&
	CVMJITirnodeIsConstant32Node(lhs) &&
	CVMJITirnodeIsConstant32Node(rhs))
    {
	CVMBool pass;

	CVMassert(!CVMJITirnodeHasSideEffects(lhsNode));
	CVMassert(!CVMJITirnodeHasSideEffects(rhsNode));

	switch (condition) {
	case CVMJIT_LT:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i <
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	case CVMJIT_LE:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i <=
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	case CVMJIT_EQ:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i ==
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	case CVMJIT_GE:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i >=
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	case CVMJIT_GT:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i >
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	case CVMJIT_NE:
	    pass = CVMJITirnodeGetConstant32(lhs)->j.i !=
		CVMJITirnodeGetConstant32(rhs)->j.i;
	    break;
	default:
	    CVMassert(CVM_FALSE);
            pass = CVM_FALSE; /* resolve compiler warning. */
	}
	if (pass) {
	    return gotoBranchToTargetBlock(con, curbk, targetbk, CVMJIT_GOTO,
					   CVM_TRUE);
	} else {
	    /* do nothing */
	}
	return CVM_TRUE;
    }
    {
	/* build BCOND node */ 
	CVMJITIRNode* bcondNode = 
	    CVMJITirnodeNewCondBranchOp(con, lhsNode, rhsNode,
					typeTag, condition, targetbk, flags); 

	/* Push target block onto block stack
	   connect control flow arc between curbk and target block
	   phiMerge is needed. The stack items are needed to finish up
	   translating the rest of opcodes in the extended basic block */ 
	CVMJITirblockAtBranch(con, curbk, targetbk,
			      lhsNode, rhsNode, bcondNode,
			      CVM_FALSE);

	/*
	 * Add target block to list of blocks we flow incoming locals from.
	 *
	 * We must do this after calling CVMJITirblockAtBranch so ASSIGN
	 * nodes have been flushed.
	 */
#ifdef CVM_JIT_REGISTER_LOCALS
	addIncomingLocalsSuccessorBlock(con, curbk, targetbk, CVM_FALSE);
#endif

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	if (CVMJITirblockIsBackwardBranchTarget(targetbk)) {
	    con->gcCheckPcsSize++;
	}
#endif

    }
    if (nullCheck) {
	nullCheckMark(con, lhsNode);
    }
    return CVM_TRUE;
}

/*
 * conditional branch
 */
static CVMBool
condBranch(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
	   CVMUint16 typeTag, CVMJITCondition condition,
	   CVMJITIRNode* rhsNode, CVMJITIRNode* lhsNode,
	   CVMUint16 curPC, CVMUint8 flags)
{
    /* Build target block */
    CVMJITMethodContext* mc = con->mc;
    CVMUint8* codeBegin = CVMjmdCode(mc->jmd);
    CVMUint16 targetPC = CVMgetInt16(codeBegin+curPC+1) + curPC;
    CVMJITIRBlock* targetbk = mc->pcToBlock[targetPC];

    return condBranchToTargetBlock(con, curbk, typeTag, condition,
				   rhsNode, lhsNode, targetbk, flags);
}

/*
 * goto and jsr handling
 */
static CVMBool
gotoBranchToTargetBlock(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
			CVMJITIRBlock* targetBlock, CVMUint16 opcodeTag,
			CVMBool fallthroughOK)
{
    con->mc->abortTranslation = CVM_TRUE;
    if (fallthroughOK && CVMJITirblockGetNext(curbk) == targetBlock) {
	/* If branching to next block, just fall through */

	/* If this is the only branch to the target block, we should
	   remove the IsBranchTarget flag so that merging
	   can happen. */
	return CVM_TRUE;
    } else {
	CVMJITIRNode* branchNode;
	CVMJITIRBlock* thisBlock = curbk;

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	if (CVMJITirblockIsBackwardBranchTarget(targetBlock)) {
	    con->gcCheckPcsSize++;
	}
#endif

	CVMJITirblockAtBranch(con, thisBlock, targetBlock,
			      NULL, NULL, NULL,
			      fallthroughOK);
	
	/* Append branch node to the current root list */
	branchNode = CVMJITirnodeNewBranchOp(con,
					     CVMJIT_ENCODE_BRANCH(opcodeTag),
					     targetBlock); 

	CVMJITirnodeNewRoot(con, thisBlock, branchNode);

	/* 
	 * Add target block to list of blocks we flow incoming locals from.
	 * A goto is treated like a fallthrough in that we always want to add
	 * the target block.
	 *
	 * We must do this after calling CVMJITirblockAtBranch so ASSIGN
	 * nodes have been flushed.
	 */
#ifdef CVM_JIT_REGISTER_LOCALS
	addIncomingLocalsSuccessorBlock(con, curbk, targetBlock, CVM_TRUE);
#endif

	return CVM_FALSE;
    }
}

/*
 * goto and jsr handling
 */
static CVMBool
gotoBranch(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
	   CVMUint16 targetPC, CVMUint16 opcodeTag)
{
    CVMJITIRBlock* targetBlock = con->mc->pcToBlock[targetPC];
    return gotoBranchToTargetBlock(con, curbk, targetBlock, opcodeTag,
				   CVM_TRUE);
}

/* 
 * Create CVMJITMapPcNode node for pc if it is a pc that the backend
 * will need to map to a compiled pc.
 */
static void 
createMapPcNodeIfNeeded(
    CVMJITCompilationContext* con, 
    CVMJITIRBlock* curbk,
    CVMUint16 pc,
    CVMBool alwaysNeeded)
{
    CVMJITMethodContext *mc = con->mc;
    CVMBool contains;

    /* We can only handle context 0 for now. */
    if (mc->compilationDepth == 0) {
	/* See if this is a pc we are supposed to map */
	if (alwaysNeeded) {
	    contains = CVM_TRUE;
	} else {
	    CVMJITsetContains(&mc->mapPcSet, pc, contains);
	}

	if (contains) {
	    /* create MAP_PC node that will help the backend generate a
	       javaPC <--> compiledPC mapping */
	    CVMJITIRNode* node =
		CVMJITirnodeNewMapPcNode(con, CVMJIT_ENCODE_MAP_PC, pc, curbk);
	    CVMJITirDoSideEffectOperator(con, curbk);
	    CVMJITirnodeNewRoot(con, curbk, node);
	    con->mapPcNodeCount++;
	    /* avoid creating the same map twice */
	    CVMJITsetRemove(con, &mc->mapPcSet, pc);
	}
    }
}

static void
nullCheckInitialize(CVMJITCompilationContext* con)
{
    con->nextNullCheckID = 0;
    con->maxNullCheckID = 0;
}

/*
 * Same constant, or same underlying node 
 */
static CVMBool
isSameSimple(CVMJITCompilationContext* con, CVMJITIRNode* n1,
	     CVMJITIRNode* n2)
{
    CVMassert(n1 != NULL);
    CVMassert(n2 != NULL);
    
    if (CVMJITirnodeValueOf(n1) == CVMJITirnodeValueOf(n2)) {
	return CVM_TRUE;
    }
    
    if (CVMJITirnodeIsConstant32Node(n1) &&
	CVMJITirnodeIsConstant32Node(n2)) {
	return CVMJITirnodeGetConstant32(n1)->v32 == 
	    CVMJITirnodeGetConstant32(n2)->v32;
    }
    return CVM_FALSE;
}

/*
 * Check whether n1 'equals' n2. This is pointer equality, or constant
 * equality, or simple equality between the operands of simple binary
 * and unary expressions.
 */
static CVMBool
isSame(CVMJITCompilationContext* con, CVMJITIRNode* n1, CVMJITIRNode* n2)
{
    n1 =  CVMJITirnodeValueOf(n1);
    n2 =  CVMJITirnodeValueOf(n2);

    if (isSameSimple(con, n1, n2)) {
	return CVM_TRUE;
    }
    /* Look one deeper for binary nodes */
    if (CVMJITirnodeIsBinaryNode(n1) && CVMJITirnodeIsBinaryNode(n2)) {
	CVMUint8 op1 = CVMJITgetOpcode(n1) >> CVMJIT_SHIFT_OPCODE;
	CVMUint8 op2 = CVMJITgetOpcode(n2) >> CVMJIT_SHIFT_OPCODE;
	if ((op1 == op2) && CVMJITirnodeBinaryNodeIs(n1, ARITHMETIC)) {
	    CVMJITBinaryOp* binOp1 = CVMJITirnodeGetBinaryOp(n1);
	    CVMJITBinaryOp* binOp2 = CVMJITirnodeGetBinaryOp(n2);
	    CVMJITIRNode* n1lhs = CVMJITirnodeValueOf(binOp1->lhs);
	    CVMJITIRNode* n1rhs = CVMJITirnodeValueOf(binOp1->rhs);
	    CVMJITIRNode* n2lhs = CVMJITirnodeValueOf(binOp2->lhs);
	    CVMJITIRNode* n2rhs = CVMJITirnodeValueOf(binOp2->rhs);
	    CVMassert(CVMJITirnodeBinaryNodeIs(n2, ARITHMETIC));
	    if (isSameSimple(con, n1lhs, n2lhs) && 
		isSameSimple(con, n1rhs, n2rhs)) {
		return CVM_TRUE;
	    }
	}
    }
    /* Look one deeper for unary nodes */
    if (CVMJITirnodeIsUnaryNode(n1) && CVMJITirnodeIsUnaryNode(n2)) {
	CVMUint8 op1 = CVMJITgetOpcode(n1) >> CVMJIT_SHIFT_OPCODE;
	CVMUint8 op2 = CVMJITgetOpcode(n2) >> CVMJIT_SHIFT_OPCODE;
	if ((op1 == op2) && CVMJITirnodeUnaryNodeIs(n1, ARITHMETIC)) {
	    CVMJITUnaryOp* unOp1 = CVMJITirnodeGetUnaryOp(n1);
	    CVMJITUnaryOp* unOp2 = CVMJITirnodeGetUnaryOp(n2);
	    CVMJITIRNode* n1operand = CVMJITirnodeValueOf(unOp1->operand);
	    CVMJITIRNode* n2operand = CVMJITirnodeValueOf(unOp2->operand);
	    CVMassert(CVMJITirnodeUnaryNodeIs(n2, ARITHMETIC));
	    if (isSameSimple(con, n1operand, n2operand)) {
		return CVM_TRUE;
	    }
	}
    }
    return CVM_FALSE;
}

static CVMBool
nullCheckEmitted(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    int i;
    for (i = 0; i < con->maxNullCheckID; i++) {
	CVMJITIRNode* nullCheckedNode = con->nullCheckNodes[i];
	if ((nullCheckedNode != NULL) && 
	    isSameSimple(con, node, nullCheckedNode)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** NULL CHECK ALREADY EMITTED\n");
            CVMJITirdumpIRNode(con, node, 0, "   ");
	    CVMconsolePrintf("\n");
#endif
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

static CVMBool
CVMJITirnodeIsLocal_0Ref(CVMJITCompilationContext *con, CVMJITIRNode *n)
{
    CVMBool zeroRef;
    CVMJITIRNode *local0 = con->mc->locals[0];
    zeroRef = (local0 != NULL && isSameSimple(con, n, local0));

    CVMassert(!zeroRef || CVMJITirnodeIsReferenceType(n));

    return zeroRef;
}

static CVMBool
nullCheckNeeded(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    if (!nullCheckEmitted(con, node)) {
	CVMJITIRNode* valueOfRef = CVMJITirnodeValueOf(node);
	if (CVMJITirnodeIsNewOperation(valueOfRef) ||
	    (con->mc->removeNullChecksOfLocal_0 && 
	     CVMJITirnodeIsLocal_0Ref(con, valueOfRef)))
	{
	    nullCheckMark(con, node);
	    return CVM_FALSE;
	}
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

static CVMBool isLocalN(CVMJITIRNode* node, CVMUint32 localNo)
{
    CVMassert(node != NULL);
    node = CVMJITirnodeValueOf(node);
    return CVMJITirnodeIsLocalNode(node) &&
	(CVMJITirnodeGetLocal(node)->localNo == localNo);
}

/*
 * Return TRUE if node is LOCAL(localNo) or is a binary node
 * containing localNo
 */
static CVMBool containsLocalN(CVMJITIRNode* node, CVMUint32 localNo)
{
    CVMassert(node != NULL);
    if (isLocalN(node, localNo)) {
	return CVM_TRUE;
    }
    if (CVMJITirnodeIsBinaryNode(node)) {
	CVMJITBinaryOp* binOp = CVMJITirnodeGetBinaryOp(node);
	if (isLocalN(binOp->lhs, localNo) || isLocalN(binOp->rhs, localNo)) {
	    return CVM_TRUE;
	}
    }
    if (CVMJITirnodeIsUnaryNode(node)) {
	CVMJITUnaryOp* unOp = CVMJITirnodeGetUnaryOp(node);
	if (isLocalN(unOp->operand, localNo)) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

static void
nullCheckKillForLocal(CVMJITCompilationContext* con, CVMUint32 localNo)
{
    int i;
    for (i = 0; i < con->maxNullCheckID; i++) {
	CVMJITIRNode* node = con->nullCheckNodes[i];
	if (node == NULL) {
	    continue;
	}
	if (isLocalN(node, localNo)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** KILLING NULL CHECK FOR LOCAL\n");
            CVMJITirdumpIRNode(con, node, 0, "   ");
	    CVMconsolePrintf("\n");
#endif
	    con->nullCheckNodes[i] = NULL;
	}
    }
}

/*
 * Make it a circular buffer. It's OK to "forget" expressions.
 */
static void
nullCheckMark(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
    if (con->nullCheckNodes[con->nextNullCheckID] != NULL) {
	CVMconsolePrintf("***** OVERRIDING ITEM %d IN NULL CHECKS\n",
			 con->nextNullCheckID);
	CVMconsolePrintf("***** OLD NODE:\n");
        CVMJITirdumpIRNode(con, con->nullCheckNodes[con->nextNullCheckID],
                           0, "   ");
	CVMconsolePrintf("\n");
	CVMconsolePrintf("***** NEW NODE:\n");
        CVMJITirdumpIRNode(con, node, 0, "   ");
	CVMconsolePrintf("\n");
    }
#endif
    con->nullCheckNodes[con->nextNullCheckID++] = node;
    con->nextNullCheckID %= JIT_MAX_CHECK_NODES;
    if (con->maxNullCheckID < JIT_MAX_CHECK_NODES) {
	con->maxNullCheckID++;
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	CVMconsolePrintf("***** NEW MAX NULL CHECK ID: %d\n",
			 con->maxNullCheckID);
#endif
    }
}

/*
 * Keep two arrays around -- one for array bases, and one for indices 
 */
static void
boundsCheckInitialize(CVMJITCompilationContext* con)
{
    con->nextBoundsCheckID = 0;
    con->maxBoundsCheckID = 0;
}

/*
 * Return slot id for array table given (arrayNode, indexNode) pair
 * -1 if not found.
 */
static CVMInt32
arrayTableId(CVMJITCompilationContext* con,
	     CVMJITIRNode* arrayNode, CVMJITIRNode* indexNode)
{
    int i;
    for (i = 0; i < con->maxBoundsCheckID; i++) {
	CVMJITIRNode* boundsCheckedArray = con->boundsCheckArrays[i];
	CVMJITIRNode* boundsCheckedIndex = con->boundsCheckIndices[i];
	
	if ((boundsCheckedArray == NULL) || (boundsCheckedIndex == NULL)) {
	    continue;
	}
	
	if (isSame(con, arrayNode, boundsCheckedArray) &&
	    isSame(con, indexNode, boundsCheckedIndex)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** BOUNDS CHECK ALREADY EMITTED\n");
	    CVMJITirdumpIRNode(con, arrayNode, 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, indexNode, 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, con->indexExprs[i], 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, con->arrayFetchExprs[i], 0, "   ");
	    CVMconsolePrintf("\n");
#endif
	    return i;
	}
    }
    return -1;
}

#if 0
/*
 * Kill any array entries corresponding to indexRefNode
 */
static void
boundsCheckKill(CVMJITCompilationContext* con, CVMJITIRNode* indexRefNode)
{
    int i;
    for (i = 0; i < con->maxBoundsCheckID; i++) {
	CVMJITIRNode* boundsCheckedArray = con->boundsCheckArrays[i];
	CVMJITIRNode* boundsCheckedIndex = con->boundsCheckIndices[i];
	
	if ((boundsCheckedArray == NULL) || (boundsCheckedIndex == NULL)) {
	    continue;
	}
	
	if (con->indexExprs[i] == indexRefNode) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** INDEX EXPRESSION KILLED\n");
	    CVMJITirdumpIRNode(con, indexRefNode, 0, "   ");
	    CVMconsolePrintf("\n");
#endif
	    con->boundsCheckArrays[i] = NULL;
	    con->boundsCheckIndices[i] = NULL;
	    con->indexExprs[i] = NULL;
	    con->arrayFetchExprs[i] = NULL;
	}
    }
}
#endif

/*
 * Kill all cached index expressions
 */
static void
boundsCheckKillIndexExprs(CVMJITCompilationContext* con)
{
    int i;
#ifdef CVM_TRACE_JIT
    CVMBool workDone = CVM_FALSE;
#endif    
    for (i = 0; i < con->maxBoundsCheckID; i++) {
#ifdef CVM_TRACE_JIT
	if (con->indexExprs[i] != NULL) {
	    workDone = CVM_TRUE;
	}
#endif
	con->indexExprs[i] = NULL;
    }
#ifdef CVM_TRACE_JIT
    if (workDone) {
	CVMtraceJITIROPT(("CSE: Forgetting all CSE'd "
			  "array index expressions\n"));
    }
#endif
#ifdef CVM_JIT_REGISTER_LOCALS
    /* stop adding ref types to incomingLocals[]. Non-ref types are
     * safe since they are in NV registers and also gc won't hurt them
     */
    con->mc->currBlock->noMoreIncomingRefLocals = CVM_TRUE;
#endif
}

/*
 * Kill all cached fetch expressions
 */
static void
boundsCheckKillFetchExprs(CVMJITCompilationContext* con)
{
    int i;
#ifdef CVM_TRACE_JIT
    CVMBool workDone = CVM_FALSE;
#endif    
    for (i = 0; i < con->maxBoundsCheckID; i++) {
#ifdef CVM_TRACE_JIT
	if (con->arrayFetchExprs[i] != NULL) {
	    workDone = CVM_TRUE;
	}
#endif
	con->arrayFetchExprs[i] = NULL;
    }
#ifdef CVM_TRACE_JIT
    if (workDone) {
	CVMtraceJITIROPT(("CSE: Forgetting all CSE'd "
			  "array read expressions\n"));
    }
#endif
}

static void
boundsCheckKillForLocal(CVMJITCompilationContext* con, CVMUint32 localNo)
{
    int i;
#ifdef CVM_TRACE_JIT
    CVMBool workDone = CVM_FALSE;
#endif    
    for (i = 0; i < con->maxBoundsCheckID; i++) {
	CVMJITIRNode* arrNode = con->boundsCheckArrays[i];
	CVMJITIRNode* idxNode = con->boundsCheckIndices[i];
	/* Invalid entry */
	if ((arrNode == NULL) || (idxNode == NULL)) {
	    continue;
	}
	
	if (containsLocalN(arrNode, localNo) ||
	    containsLocalN(idxNode, localNo)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** KILLING BOUNDS CHECK FOR LOCAL\n");
	    CVMJITirdumpIRNode(con, arrNode, 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, idxNode, 0, "   ");
	    CVMconsolePrintf("\n");
#endif
#ifdef CVM_TRACE_JIT
	    workDone = CVM_TRUE;
#endif
	    /* Kill both. One is useless without the other */
	    con->boundsCheckArrays[i] = NULL;
	    con->boundsCheckIndices[i] = NULL;
	    /* And just to make sure ... */
	    con->indexExprs[i] = NULL;
	    con->arrayFetchExprs[i] = NULL;
	}
    }
#ifdef CVM_TRACE_JIT
    if (workDone) {
	CVMtraceJITIROPT(("CSE: Local %d changed: Forgetting CSE'd array "
			  "expressions referring to it\n", localNo));
    }
#endif
}

/*
 * Put (arrNode, idxNode) in the array table. Add an associated
 * index node if available.
 *
 * Make the buffer a circular one. It's OK to "forget" expressions.
 *
 * Return the array table id indicating where this item is stored.
 */
static CVMInt32
boundsCheckMark(CVMJITCompilationContext* con,
		CVMJITIRNode* arrNode, CVMJITIRNode* idxNode,
		CVMJITIRNode* indexRefNode)
{
    CVMInt32 retID;
    
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
    if (con->boundsCheckArrays[con->nextBoundsCheckID] != NULL) {
	CVMconsolePrintf("***** OVERRIDING ITEM %d IN BOUNDS CHECKS\n",
			 con->nextBoundsCheckID);
    }
#endif
    con->boundsCheckArrays[con->nextBoundsCheckID] = arrNode;
    con->boundsCheckIndices[con->nextBoundsCheckID] = idxNode;
    con->indexExprs[con->nextBoundsCheckID] = indexRefNode;
    /* No fetch yet */
    con->arrayFetchExprs[con->nextBoundsCheckID] = NULL;
    retID = con->nextBoundsCheckID;
    
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** MARKING BOUNDS CHECK\n");
	    CVMJITirdumpIRNode(con, arrNode, 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, idxNode, 0, "   ");
	    CVMconsolePrintf("\n");
	    CVMJITirdumpIRNode(con, indexRefNode, 0, "   ");
	    CVMconsolePrintf("\n");
#endif
    con->nextBoundsCheckID++;
    con->nextBoundsCheckID %= JIT_MAX_CHECK_NODES;
    if (con->maxBoundsCheckID < JIT_MAX_CHECK_NODES) {
	con->maxBoundsCheckID++;
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	CVMconsolePrintf("***** NEW MAX BOUNDS CHECK ID: %d\n",
			 con->maxBoundsCheckID);
#endif
    }
    return retID;
}

static void
getfieldCacheKillForLocal(CVMJITCompilationContext* con,
			  CVMUint32 localNo);

/*
 * NULL check routine.
 * Return argument, or argument wrapped in a NULLCHECK, based on need.
 * If !emitNode, then always return original node, but do bookkeeping.
 */
static CVMJITIRNode*
nullCheck(CVMJITCompilationContext* con, 
	  CVMJITIRBlock* curbk,
	  CVMJITIRNode* refNode,
	  CVMBool emitNodeIfNotEmitted)
{
    /* Build exception class block and trapblock */
    CVMJITIRNode* node; 

    /* 
     * When compiling in a method which itself is non-static (i.e. is
     * a virtual method or an <init>) and which contains no instances
     * of astore_0 or its synonyms, then all null checks on local
     * variable #0 can be eliminated. The null check flag is set
     * globally in con by the pre-pass. No null check on the result of
     * a new-object, new-array, or any other object-allocation
     * operations.
     *
     * We can also eliminate null checks for 'refNode' if one has
     * already been emitted in this block.  
     */
    if (!nullCheckNeeded(con, refNode)) {
        CVMJITstatsRecordInc(con, CVMJIT_STATS_NULL_CHECKS_ELIMINATED);
	return refNode; /* No null check */
    }
    
    if (emitNodeIfNotEmitted) {
	/* Build NULLCHECK node */
	node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_NULL_CHECK, refNode);
        CVMJITirnodeSetThrowsExceptions(node);
        CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_NULL_CHECK_NODES);
    } else {
	/* Book-keeping only */
	node = refNode;
        CVMJITirnodeSetParentThrowsExceptions(node);
    }

    /* And remember that we emitted null check for 'refNode'. */
    nullCheckMark(con, refNode);
    return node;
}

/*
 * Forward decl.
 */
static CVMJITIRNode**
indexExpressionSlot(CVMJITCompilationContext* con, CVMInt32 arrid)
{
    CVMassert(arrid >= 0);
    return &con->indexExprs[arrid];
}

static CVMJITIRNode**
arrayFetchExpressionSlot(CVMJITCompilationContext* con, CVMInt32 arrid)
{
    CVMassert(arrid >= 0);
    return &con->arrayFetchExprs[arrid];
}

/* 
 * array[index] operation
 *
 * The resultant INDEX tree looks like the following, in order to preserve
 * correct evaluation order:
 *
 *        INDEX
 *       /     \
 *      /     BOUNDS_CHECK
 *  arrayRef    /     \
 *             /       \
 *        indexNode  ALENGTH
 *                      | 
 *                  NULLCHECK 
 *                      |
 *                   arrayRef
 *
 * So the evaluation order is:
 *
 * 1) arrayRef
 * 2) indexNode
 * 3) NULL check on arrayRef
 * 4) ALENGTH on arrayRef
 * 5) Bounds check
 * 6) INDEX
 *
 * Return array table ID where index expression can be found.
 *
 */
static CVMInt32
indexArrayOperation(CVMJITCompilationContext* con, CVMJITIRBlock* curbk, 
		    CVMBool evalOrderGuaranteed,
		    CVMUint16 typeTag)
{
    CVMInt32 arrTableID;
    CVMJITIRNode* indexNode = CVMJITirnodeStackPop(con);
    CVMJITIRNode* origArrayrefNode = CVMJITirnodeStackPop(con);
    CVMJITIRNode* origIndexNode;
    CVMJITIRNode* arrayrefNode;

    CVMJITIRNode* indexRefNode;
    CVMJITIRNode** indexRefPtr;
    CVMBool boundsCheckEmitted;
    CVMBool needBoundsCheck;

    /* Remember the index node before it has the bounds check stuff
       attached to it */
    origIndexNode = indexNode;
    arrTableID = arrayTableId(con, origArrayrefNode, origIndexNode);
    if (arrTableID != -1) {
	boundsCheckEmitted = CVM_TRUE;
	indexRefPtr = indexExpressionSlot(con, arrTableID);
	indexRefNode = *indexRefPtr;
	CVMtraceJITIROPT(("CSE: Re-using CSE'd array index expression\n"));
    } else {
	boundsCheckEmitted = CVM_FALSE;
	indexRefPtr = NULL;
	indexRefNode = NULL;
    }
    needBoundsCheck = !boundsCheckEmitted;

#ifdef IAI_ARRAY_INIT_BOUNDS_CHECK_ELIMINATION
    /*
     * The follow code try to match the case below.
     *
     * Index node is CONSTANT32 node
     * Array reference node is NEW ARRAY node
     *   
     *  arrayRef -->   IDENT
     *                   |
     *                 NEW_ARRAY(BOOLEN..LONG)
     *                   |
     *                 ICONST32 (arraylength)
     *
     *  indexNode -->  ICONST32 (index)
     *  
     *  If the index < arraylength, then 
     *  no additional boundscheck required
     *  Thus this function will return the 
     *  following optimized tree.
     *
     *        INDEX
     *       /     \
     *      /       \
     *  arrayRef ICONST32(indexNode)
     * 
     */
    if (needBoundsCheck) {
        CVMJITIRNode *arrayRefNode = CVMJITirnodeValueOf(origArrayrefNode);

        if (CVMJITnodeTagIs(arrayRefNode, UNARY) &&
            CVMJITirnodeIsConstant32Node(indexNode))
	{
	    CVMUint8 opArrayRefOpcode;
            opArrayRefOpcode =
		(CVMJITgetOpcode(arrayRefNode) >> CVMJIT_SHIFT_OPCODE);
         
            if ((opArrayRefOpcode >= CVMJIT_NEW_ARRAY_BOOLEAN) &&
                (opArrayRefOpcode <= CVMJIT_NEW_ARRAY_LONG))
	    {
                CVMJITIRNode *lengthNode =
		    CVMJITirnodeGetLeftSubtree(arrayRefNode);
                if ((CVMJITirnodeIsConstant32Node(lengthNode))) {
                   CVMUint32 arrayLength =
		       CVMJITirnodeGetConstant32(lengthNode)->v32;
                   CVMUint32 index =
		       CVMJITirnodeGetConstant32(indexNode)->v32;
                   if (arrayLength > index) {
                      needBoundsCheck = CVM_FALSE;
                   }
                }
            }
        }
    }
#endif /* IAI_ARRAY_INIT_BOUNDS_CHECK_ELIMINATION*/

    /* Arrayref null and bounds check if needed */
    if (!boundsCheckEmitted) {
	CVMJITIRNode* lengthNode;
        CVMJITIRNode* cachedArrayLengthNode;

        CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_BOUNDS_CHECK_NODES);

        cachedArrayLengthNode = arrayLengthCacheGet(con, origArrayrefNode);
        if (cachedArrayLengthNode == NULL) {
#ifndef CVMJIT_TRAP_BASED_NULL_CHECKS
            arrayrefNode = nullCheck(con, curbk, origArrayrefNode, CVM_TRUE);
#else
            /* Implicit null check during ARRAY_LENGTH in bounds checking */
            arrayrefNode = nullCheck(con, curbk, origArrayrefNode, CVM_FALSE);
#endif
            lengthNode =
		CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_ARRAY_LENGTH, 
				       arrayrefNode);

            /* Finally cache: Key (objrefNode), value lengthNode */
            arrayLengthCachePut(con, origArrayrefNode, lengthNode);

        } else {
            lengthNode = cachedArrayLengthNode;
        }

	/* Array bound check which also returns the indexNode */
        if (needBoundsCheck) {
	    indexNode =
		CVMJITirnodeNewBoundsCheckOp(con, indexNode, lengthNode);
        }

	/* Now, since we've emitted this bounds check with a potential
	   NULL check on the array ref, we don't need to make the
	   subsequent INDEX operation have a null check. Omit */
	arrayrefNode = origArrayrefNode;
    } else if (!evalOrderGuaranteed) {
	/* 
	 * The bounds check has already been eliminated. So the tree
	 * will look different now if we want the evaluation order to be:
	 * 1) arrayReference
	 * 2) indexNode
	 * 3) nullCheck
	 *
	 * If this index node is being built for an array store, this eval
	 * order has already been guaranteed by CVMJITirDoSideEffectOperator().
	 *
	 * Otherwise, first force the evaluation of 'origArrayrefNode' and
	 * 'indexNode'.
	 */
        CVMJITirDoSideEffectOperator(con, curbk);
	if (CVMJITirnodeHasSideEffects(origArrayrefNode) &&
	    !CVMJITirnodeHasBeenEvaluated(origArrayrefNode))
	{
	    CVMJITirForceEvaluation(con, curbk, origArrayrefNode);
	}
	if (CVMJITirnodeHasSideEffects(indexNode) &&
	    !CVMJITirnodeHasBeenEvaluated(indexNode))
	{
	    CVMJITirForceEvaluation(con, curbk, indexNode);
	}
	arrayrefNode = origArrayrefNode;
        CVMJITstatsRecordInc(con, CVMJIT_STATS_BOUNDS_CHECKS_ELIMINATED);
    } else {
	arrayrefNode = origArrayrefNode;
        CVMJITstatsRecordInc(con, CVMJIT_STATS_BOUNDS_CHECKS_ELIMINATED);
    }

    /* Array index operation and build INDEX node */
    if (indexRefNode == NULL) {
	/*
	 * This is the only place INDEX nodes are built.
	 *
	 * The following index node could be with or w/o a bounds check
	 */
	indexRefNode = 
	    CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_INDEX,
				    arrayrefNode, indexNode);
	/*
	 * Pass on the element type to the back end.
	 */
	CVMJITirnodeGetBinaryOp(indexRefNode)->data = typeTag;
	if (boundsCheckEmitted) {
	    /* Bounds check already emitted. Just put in index expr. */
	    *indexRefPtr = indexRefNode;
	} else {
	    /* No entry for this (array,index) pair. Create one
	       and put the index expression in */
	    arrTableID = boundsCheckMark(con, origArrayrefNode, origIndexNode,
					 indexRefNode);
	}
    }
    
    /*
     * Make sure the caller will be able to dig it out of this table
     */
    CVMassert(con->indexExprs[arrTableID] == indexRefNode);
    return arrTableID;
}

static void
mcStoreValueToLocal(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
                    CVMUint16 localNo,
                    CVMUint16 typeTag, CVMJITMethodContext* mc,
                    CVMBool mustWriteLocal);

/*
 * Assign value to Local
 */
static void
storeValueToLocal(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
		  CVMUint16 localNo, CVMUint16 typeTag)
{
    mcStoreValueToLocal(con, curbk, localNo, typeTag, con->mc,
                        CVM_FALSE);
}

static void
storePCValueToLocal(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
                    CVMUint16 localNo, CVMUint16 typeTag, CVMUint32 pc)
{
    storeValueToLocal(con, curbk, localNo, typeTag);
    CVMJITlocalStateMarkPC(con, con->mc, localNo, pc); 
}

static void
checkInitialLocal(CVMJITCompilationContext *con,
    CVMJITIRBlock *curbk, int mappedLocalNo)
{
    if (mappedLocalNo < curbk->numInitialLocals) {
	CVMJITIRNode *initialLocalNode =
	    curbk->initialLocals[mappedLocalNo];
	if (initialLocalNode != NULL) {
	    /* Maybe there is a more effective way to tell that this initial
	       local value is useless, but now we just go ahead and evaluate
	       it first to be conservative: */
	    CVMJITirForceEvaluation(con, curbk, initialLocalNode);

	    /* The initial local has been evaluated */
	    curbk->initialLocals[mappedLocalNo] = NULL;
	}
	/* 2nd word of double-word local? */
	if (mappedLocalNo > 0) {
	    CVMJITIRNode *initialLocalNode =
		curbk->initialLocals[mappedLocalNo - 1];
	    if (initialLocalNode != NULL &&
		CVMJITirnodeIsDoubleWordType(initialLocalNode))
	    {
		/* Maybe there is a more effective way to tell that this initial
		   local value is useless, but now we just go ahead and evaluate
		   it first to be conservative: */
		CVMJITirForceEvaluation(con, curbk, initialLocalNode);

		/* The initial local has been evaluated */
		curbk->initialLocals[mappedLocalNo - 1] = NULL;
	    }
	}
    }
}

static void
mcStoreValueToLocal(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
                    CVMUint16 localNo,
                    CVMUint16 typeTag, CVMJITMethodContext* mc,
                    CVMBool mustWriteLocal)
{
    CVMJITIRNode* node;
    CVMJITIRNode* localNode;
    CVMJITIRNode* valueNode;
    CVMUint16 mappedLocalNo;

    CVMassert(localNo < mc->localsSize);
    valueNode = CVMJITirnodeStackPop(con);

    /*
       NOTE: Because we're deferring all local writes until the very last
       moment, one side effect of this is refs in locals that would have been
       nullified may end up hanging on for a longer time thereby causing an
       object to survive more GCs.  For example, local 1 may contain a ref.
       Later on, an integer is supposed to be written to local 1.  But due
       to our deferring local writes, local 1 will retain the object ref
       much longer than if the int would have been written to it immediately.
       This can happen because of multiple inlinings in the same block.
    */

    mappedLocalNo = mc->firstLocal + localNo;

    if (mustWriteLocal) {
	CVMassert(mappedLocalNo < mc->currLocalWords);
	CVMassert(mappedLocalNo >= mc->currLocalWords - mc->nOwnLocals);

        /* Force evaluation of all nodes with side effects in the correct eval
           order.  The valueNode will get evaluated later as part of the
           ASSIGNment expression (so, don't worry about it): */
        CVMJITirDoSideEffectOperator(con, curbk);

        /* Since we need to write the local out, check if we have an initial
           local value that needs to be evaluate first before we trash it.
           If it isn't an unevaluated initial local, then we are not concerned
           because those are all computed.  Only the initial local value must
           be fetched from the local storage location first: */

	checkInitialLocal(con, curbk, mappedLocalNo);
	if (CVMJITirIsDoubleWordTypeTag(typeTag)) {
	    checkInitialLocal(con, curbk, mappedLocalNo + 1);
	}

        localNode = CVMJITirnodeNewLocal(con, CVMJIT_ENCODE_LOCAL(typeTag),
                                         mappedLocalNo);
        node = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_ASSIGN(typeTag),
                                       localNode, valueNode);  
        CVMJITirnodeNewRoot(con, curbk, node);

#ifdef CVM_JIT_REGISTER_LOCALS
	if (con->assignNodesCount < CVMJIT_MAX_ASSIGN_NODES) {
	    /* Add node decoration for better register targeting in backend. */
	    node->decorationType = CVMJIT_ASSIGN_DECORATION;
	    node->decorationData.assignIdx = con->assignNodesCount;
	    /* Add ASSIGN node to the array of this block's assigned locals. */
	    con->assignNodes[con->assignNodesCount++] = node;
	} else {
	    /* Stop adding locals to incomingLocals[], because we can't
	     * reliably determine if an assign node will trash them. */
	    curbk->noMoreIncomingLocals = CVM_TRUE;
	    curbk->noMoreIncomingRefLocals = CVM_TRUE;
	}
#endif

        /* The last value written to the memory location of the local is
           now the valueNode: */
        mc->physLocals[localNo] = valueNode;

        /*
         * And finally, map the state of the re-mapped locals
         */
        switch(typeTag) {
        case CVM_TYPEID_OBJ:
	    CVMJITlocalrefsSetRef(con, &con->curRefLocals, mappedLocalNo);
            break;
        case CVM_TYPEID_LONG: 
        case CVM_TYPEID_DOUBLE:
        case CVMJIT_TYPEID_64BITS:
	    CVMJITlocalrefsSetValue(con, &con->curRefLocals, mappedLocalNo,
		CVM_TRUE);
            break;
        default:
	    CVMJITlocalrefsSetValue(con, &con->curRefLocals, mappedLocalNo,
		CVM_FALSE);
        }
    } else {
	if (CVMJITirnodeHasSideEffects(valueNode) &&
	    !CVMJITirnodeHasBeenEvaluated(valueNode))
	{
	    CVMJITirnodeStackPush(con, valueNode);
	    CVMJITirDoSideEffectOperator(con, curbk);
	    CVMJITirnodeStackDiscardTop(con);
	}
    }


    /* Now cache the new local value: */
    CVMassert(mc->locals != NULL);
    if (CVMJITirIsDoubleWordTypeTag(typeTag)) {
	mc->locals[localNo + 1] = NULL;
    }
    mc->locals[localNo] = valueNode;

    /* Now invalidate the null check and other checks: */

    /* NOTE: It is more conservative to kill these on every local write
       even if we're not actually writing out to the local storage.
       We can re-evaluate if this is needed later when the local isn't
       written out, but we'll have to be careful because we'll have to
       take into consideration that no-side-effect local code can move
       around.  But note, if we treat all writes of objref local as
       having side effects, then we will have to kill these on every write.
    */
    nullCheckKillForLocal(con, mappedLocalNo);
    boundsCheckKillForLocal(con, mappedLocalNo);
    getfieldCacheKillForLocal(con, mappedLocalNo);
    arrayLengthCacheKillForLocal(con, mappedLocalNo);
}

/* Purpose: Flush all out bound locals because we're about to branch to
            another block (or have the possibly of branching to another
            block as in the case of opcodes that can throw exceptions). */
void
CVMJITirFlushOutBoundLocals(CVMJITCompilationContext* con,
                            CVMJITIRBlock* curbk,
			    CVMJITMethodContext* mc,
			    CVMBool flushAll)
{
    CVMInt32 localNo;
    int count = 0;
    int numLeftUnflushed = flushAll ? 0 : 2;
    CVMJITMethodContext* originalmc = mc;

    /* Make sure all expressions with side effects get evaluated before we
       evaluate the non-side effect expressions below.

       If the side effect expressions are dependent on the non-side effect
       expressions, then the dependencies will automatically get evaluated
       as part of the side effect expressions' evaluation.

       However, if a side effect expression could prevent the execution
       of a non-side effect expression (e.g. when an excpetion is thrown),
       there is no way to ensure this if we evaluate the non-side effect
       expressions first.  This is why the CVMJITirDoSideEffectOperator()
       must be called before we flush all the non-side effect expressions in
       the locals array.
    */
    CVMJITirDoSideEffectOperator(con, curbk);

    CVMassert(mc != NULL);

    /* NOTE: We have to traverse over all method contexts that this block has
       translated and flush their locals.  If we are "out bound" to a block
       in an outer context, then our start "mc" will reflect that.  In
       that case, locals "inside" the start context don't need to get flushed
       because they aren't visible across contexts except in the same block
       (and only as a return value attached to an END_INLINING node.)

       We start from the inner most (leaf) context because the operands for the
       inner most method context would tend to still be in registers.
       Starting from the inner most would help minimize any register
       shuffling.

       We can stop when we get to a context that this block hasn't
       translated for, because earlier, outer contexts would necessarily
       have had their locals flush already.
    */
    for (; mc != NULL && mc->translateBlock == curbk; mc = mc->caller) {
	/* Iterate over locals and count each one: */
	for (localNo = 0; localNo < mc->localsSize; localNo++) {
	    CVMJITIRNode *value = mc->locals[localNo];
	    /* Flush the local if not already flushed: */
	    if ((value != NULL) && (value != mc->physLocals[localNo])) {
		count++;
#ifdef CVM_DEBUG_ASSERTS
		if (CVMJITirnodeIsDoubleWordType(value)) { 
		    CVMassert(mc->locals[localNo + 1] == NULL);
		}
#endif
	    }
	}
    }

    mc = originalmc;
    for (; mc != NULL && mc->translateBlock == curbk; mc = mc->caller) {
	/* Iterate over locals and flush each one: */
	for (localNo = 0; localNo < mc->localsSize; localNo++) {
	    CVMJITIRNode *value = mc->locals[localNo];
	    /* Flush the local if not already flushed: */
	    if ((value != NULL) && (value != mc->physLocals[localNo])) {
		CVMassert(count > 0);
		if (--count >= numLeftUnflushed) {
		    CVMJITirnodeStackPush(con, value);
		    mcStoreValueToLocal(con, curbk, localNo,
					CVMJITgetTypeTag(value), mc, CVM_TRUE);
		} else {
#ifndef CVM_DEBUG_ASSERTS
		    /* leave a couple of assigns for later */
		    return;
#endif
		}
	    }
	}
    }
    CVMassert(count == 0);
}

/* 
 * Load value of various type from array.
 */
static void
doArrayLoad(CVMJITCompilationContext* con, 
	    CVMJITIRBlock* curbk,
	    CVMUint8 typeTag)
{
    CVMInt32 arrTableID = indexArrayOperation(con, curbk, CVM_FALSE, typeTag);
    CVMJITIRNode** indexNodePtr = NULL;
    CVMJITIRNode** fetchNodePtr;
    CVMJITIRNode* indexNode;
    CVMJITIRNode* fetchNode;

    fetchNodePtr = arrayFetchExpressionSlot(con, arrTableID);
    fetchNode = *fetchNodePtr;
    
    /* No fetch computed yet */
    if (fetchNode == NULL) {
	indexNodePtr = indexExpressionSlot(con, arrTableID);
	indexNode = *indexNodePtr;
	
	/*
	 * Although the INDEX operation can have a one or two byte
	 * typetag, the FETCH operation needs to be folded to an int.
	 */
	/* Index created for a read */
	CVMJITirnodeSetBinaryNodeFlag(CVMJITirnodeValueOf(indexNode), 
				      CVMJITBINOP_READ);
	typeTag = CVMJITfoldType(typeTag);
	fetchNode = CVMJITirnodeNewUnaryOp(con,
            CVMJIT_ENCODE_FETCH(typeTag), indexNode);
	CVMJITirnodeSetHasUndefinedSideEffect(fetchNode);
	/* And remember for next time */
	*fetchNodePtr = fetchNode;
    }
#if CVM_TRACE_JIT
    else {
	CVMtraceJITIROPT(("CSE: Re-using CSE'd array read expression\n"));
    }
#endif
    CVMJITirnodeStackPush(con, fetchNode);
}


/*
 * Build binary IR node.
 */
static void
unaryStackOp(CVMJITCompilationContext* con, CVMUint8 opcodeTag,
	     CVMUint8 typeTag, CVMUint8 flags)
{
    CVMJITIRNode* argNode = CVMJITirnodeStackPop(con);
    CVMUint16     nodeTag = CVMJIT_TYPE_ENCODE(opcodeTag, 
					       CVMJIT_UNARY_NODE, 
					       typeTag);
    CVMJITIRNode* unaryNode = CVMJITirnodeNewUnaryOp(con, nodeTag, argNode);

    CVMJITirnodeSetUnaryNodeFlag(unaryNode, flags);
    CVMJITirnodeStackPush(con, unaryNode);
}

/*
 * Build binary IR node.
 */
static void
binaryStackOp(CVMJITCompilationContext* con, CVMUint8 opcodeTag,
    CVMUint8 typeTag, CVMUint8 flags)
{
    CVMJITIRNode*  rhsNode = CVMJITirnodeStackPop(con);
    CVMJITIRNode*  lhsNode = CVMJITirnodeStackPop(con);
    CVMUint16 nodeTag =
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, typeTag);
    CVMJITIRNode*  binaryNode = 
	CVMJITirnodeNewBinaryOp(con, nodeTag, lhsNode, rhsNode);
    CVMJITirnodeSetBinaryNodeFlag(binaryNode, flags);
    CVMJITirnodeStackPush(con, binaryNode);
}

/*
 * Build binary IR nodes: parameter list in function signagure.
 *
 * Parameter node is built from arg_n to arg_1 from bottom to top.
 *
 * The most bottom right subtree node is NULL_PARAMETER node.
 *
 * For non-static invoke, the objectref node is the left most top subtree
 * in the parameter list tree.
 *
 * For an empty parameter list, NULL_PARAMETER node is returned for
 * static invoke.
 *
 * PARAMETER node pointing to objeref and NULL_PARAMETER is returned
 * for non-static.  
 */
static CVMJITIRNode*
collectParameters(CVMJITCompilationContext* con,
		  CVMJITIRBlock* curbk,
		  CVMUint16 argSize) 
{
    CVMJITIRNode* rhs = CVMJITirnodeNewNull(con, CVMJIT_ENCODE_NULL_PARAMETER);

    /* Pop arg_n until arg_1 from stack and build parameter node
       bottom-up. Build objectref node as the most lef subtree of paremater 
       node for non-static invoke */
    while (argSize > 0) {
        CVMJITIRNode* lhs = CVMJITirnodeStackPop(con);
	if (CVMJITirnodeIsDoubleWordType(lhs)) { 
	    argSize -= 2;
	} else {
	    argSize--;
	}
        lhs = CVMJITirnodeNewBinaryOp(
            con, CVMJIT_ENCODE_PARAMETER(CVMJITgetTypeTag(lhs)), lhs, rhs);
	rhs = lhs;
    }

    /* Don't do the CVMJITirDoSideEffectOperator() here because a method which
       returns a value may be treated as part of an expression tree, thereby
       negating the need for the CVMJITirDoSideEffectOperator().  Instead let
       invokeMethod() take care or calling the CVMJITirDoSideEffectOperator()
       for void methods, and pop & pop2 take care of calling the
       CVMJITirDoSideEffectOperator() when discarding unused results of
       invocations. */

    return rhs;
}

#ifdef CVMJIT_INTRINSICS

/*
 * Collect arguments for an intrinsic method that doesn't pass arguments and
 * return values in registers.
 */
static CVMJITIRNode*
collectIArgs(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
             CVMUint16 argSize, CVMJITIntrinsic *irec) 
{
    CVMJITIRNode* rhs;
    CVMJITIRNode* args;
    int argNo;
    CVMUint16 intrinsicID = irec->intrinsicID;
    CVMMethodTypeID methodTypeID = irec->methodTypeID;
    int argWordAdjust = 0;
    CVMUint32 argType;
    CVMterseSigIterator terseSig;
    CVMUint16 properties = irec->config->properties;

    if ((properties & CVMJITINTRINSIC_ADD_CCEE_ARG) != 0) {
        argWordAdjust = 1;
    }

    /* The NULL_IARG is deliberately made to look like an IARG for
       simplicity: */
    rhs = CVMJITirnodeNewNull(con, CVMJIT_ENCODE_NULL_IARG);
    CVMJITirnodeGetNull(rhs)->data = intrinsicID;

    /* Pop arg_n until arg_1 from stack and build parameter node
       bottom-up. Build objectref node as the most lef subtree of paremater 
       node for non-static invoke */
    while (argSize > 0) {
        CVMJITIRNode* lhs = CVMJITirnodeStackPop(con);
	if (CVMJITirnodeIsDoubleWordType(lhs)) { 
	    argSize -= 2;
	} else {
	    argSize--;
	}
        lhs = CVMJITirnodeNewBinaryOp(con,
                  CVMJIT_ENCODE_IARG(CVM_TYPEID_VOID), lhs, rhs);

        /* Give the backend compiler some hints about this argument: */
        CVMJITirnodeGetBinaryOp(lhs)->data2 = intrinsicID;
        CVMJITirnodeGetBinaryOp(lhs)->data =
            ((argSize + argWordAdjust) << CVMJIT_IARG_WORD_INDEX_SHIFT);

        /* Confirm that the argSize was able to fit: */
        CVMassert((argSize & CVMJIT_IARG_WORD_INDEX_MASK) == argSize);

	rhs = lhs;
    }

    /* Set the arg number: */
    args = rhs;
    argNo = 0;
    /* Take care of the self pointer first if necessary: */
    if ((properties & CVMJITINTRINSIC_IS_STATIC) == 0) {
        CVMJITirnodeSetTag(args, CVMJIT_ENCODE_IARG(CVM_TYPEID_OBJ));
        CVMJITirnodeGetBinaryOp(args)->data |=
            (argNo << CVMJIT_IARG_ARG_NUMBER_SHIFT);
        /* Confirm that the argNo was able to fit: */
        CVMassert((argNo & CVMJIT_IARG_ARG_NUMBER_MASK) == argNo);
        args = CVMJITirnodeGetRightSubtree(args);
        argNo++;
    }

    CVMtypeidGetTerseSignatureIterator(methodTypeID, &terseSig);
    argType = CVM_TERSE_ITER_NEXT(terseSig);
    for (; args->tag != CVMJIT_ENCODE_NULL_IARG; argNo++) {
        CVMassert(argType != CVM_TYPEID_ENDFUNC);
        CVMJITirnodeSetTag(args, CVMJIT_ENCODE_IARG(argType));
        CVMJITirnodeGetBinaryOp(args)->data |=
            (argNo << CVMJIT_IARG_ARG_NUMBER_SHIFT);
        /* Confirm that the argNo was able to fit: */
        CVMassert((argNo & CVMJIT_IARG_ARG_NUMBER_MASK) == argNo);
        args = CVMJITirnodeGetRightSubtree(args);
        argType = CVM_TERSE_ITER_NEXT(terseSig);
    }
    return rhs;
}

#endif /* CVMJIT_INTRINSICS */


/*
 * Collect arguments into a CVMJITMethodContext
 */
static void
mcCollectParameters(CVMJITCompilationContext* con,
		    CVMJITIRBlock* curbk,
		    CVMJITMethodContext* mc,
		    CVMUint16 argSize) 
{
    CVMInt32 i;
    CVMBool doPriv = CVM_FALSE;

    /*
     * We have to do this before popping the arguments, because some
     * of the arguments might be in same locals we are going to store
     * into.  This can happen if you have multiple inlined methods
     * in the same block.
     *
     * NOTE: We could improve this and figure out when the
     * CVMJITirDoSideEffectOperator() isn't necessary.  One idea is to
     * force evaluation only if the expression involves one of the
     * locals to be stored.  
     */

    CVMJITirDoSideEffectOperator(con, curbk);

    CVMassert(argSize <= mc->localsSize);

    i = argSize;
    /* Then, get the arguments off the stack and collect into 'locals':
       This is just a temporary holding place until we store them.
       (We can probably combine these two loops later.)
    */
    while (i > 0) {
        CVMJITIRNode* arg = CVMJITirnodeStackPop(con);
	if (CVMJITirnodeIsDoubleWordType(arg)) { 
	    i -= 2;
	    mc->locals[i] = arg;
	    mc->locals[i + 1] = NULL;
	} else {
	    i--;
	    mc->locals[i] = arg;
	}
    }

    if (mc->mb == CVMglobals.java_security_AccessController_doPrivilegedAction2
	|| mc->mb == CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2)
    {
	doPriv = CVM_TRUE;
    }

    /* Assign arguments of inlined method to new locals */
    for (i = 0; i < argSize; ++i) {
	if (mc->locals[i] != NULL) {
	    CVMJITirnodeStackPush(con, mc->locals[i]);
	    /* single-word, non-reference stores */
	    mcStoreValueToLocal(con, curbk, i,
		CVMJITgetTypeTag(mc->locals[i]),
		mc, doPriv && i == 1);
	}
    }

    /* these locals are initialized and valid only for this block */
    mc->translateBlock = curbk;
}

/*
 * Find "receiver object" on stack, without popping
 */
static CVMJITIRNode*
findThis(CVMJITCompilationContext* con, CVMUint16 argSize) 
{
    CVMInt32 i = argSize;
    CVMInt32 j = CVMJITstackCnt(con, con->operandStack) - 1;
    CVMJITIRNode* arg = NULL;
    CVMassert(argSize >= 1);
    /* First get the arguments off the stack and collect into 'locals' */
    while (i > 0) {
        arg = CVMJITstackGetElementAtIdx(con, con->operandStack, j);
	if (CVMJITirnodeIsDoubleWordType(arg)) { 
	    i -= 2;
	} else {
	    i--;
	}
	j--;
    }
    return arg;
}

/* For better code readability, the following symbols are defined to be used
   when initialize the isInvokeVirtual boolean: */
#define CVMJIT_VINVOKE  CVM_TRUE        /* Virtual invocation   */
#define CVMJIT_IINVOKE  CVM_FALSE       /* Interface invocation */

/*
 * Build the node to fetch an invocation target for a dynamic method
 * invocation (invokespecial, invokevirtual, invokeinterface and
 * quickened variants).  
 */
static CVMJITIRNode* 
objectInvocationTarget(CVMJITCompilationContext* con,
		       CVMJITIRBlock* curbk,
                       CVMBool isInvokeVirtual,
		       CVMJITIRNode* objRefNode,
		       CVMJITIRNode* indexNode)
{
    CVMJITIRNode* targetNode;
    CVMUint16     nodeTag;
    CVMJITIRNode* tableNode;
    CVMUint8 tableAccessTag;
    CVMUint8 tableIndexTag;
    CVMBool emitNullCheck;

    if (isInvokeVirtual) {
	tableAccessTag = CVMJIT_GET_VTBL;
	tableIndexTag  = CVMJIT_FETCH_MB_FROM_VTABLE;
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	emitNullCheck  = CVM_FALSE; /* Getting the v-table will do the check */
#else
	emitNullCheck  = CVM_TRUE; /* Must do explicit check */
#endif
    } else {
	tableAccessTag = CVMJIT_GET_ITBL;
	tableIndexTag  = CVMJIT_FETCH_MB_FROM_INTERFACETABLE;
	emitNullCheck  = CVM_TRUE; /* Must do explicit check */
    }

    /* The lookup of the method in a table */
    nodeTag = CVMJIT_TYPE_ENCODE(tableAccessTag,
				 CVMJIT_UNARY_NODE,
				 CVM_TYPEID_NONE);
    objRefNode = nullCheck(con, curbk, objRefNode, emitNullCheck);
    
    tableNode = CVMJITirnodeNewUnaryOp(con, nodeTag, objRefNode);

    /* And the fetching of the target mb */
    nodeTag = CVMJIT_TYPE_ENCODE(tableIndexTag,
                                 CVMJIT_BINARY_NODE,
                                 CVMJIT_TYPEID_ADDRESS);
    targetNode = CVMJITirnodeNewBinaryOp(con, nodeTag, tableNode, indexNode);
    
    return targetNode;
}

/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
/*
 * Build the node to fetch cb for an inlinable dynamic method invocation.
 */
static CVMJITIRNode* 
objectInvocationTargetInlinable(CVMJITCompilationContext* con,
		                CVMJITIRBlock* curbk,
 		                CVMJITIRNode* objRefNode)
{
    CVMJITIRNode* targetNode;
    CVMUint16     nodeTag;
    CVMUint8      fetchTag;
    CVMBool       emitNullCheck;

    fetchTag  = CVMJIT_FETCH_VCB;
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
    emitNullCheck  = CVM_FALSE; /* Getting the object cb will do the check */
#else
    emitNullCheck  = CVM_TRUE; /* Must do explicit check */
#endif
    
    nodeTag = CVMJIT_TYPE_ENCODE(fetchTag,
				 CVMJIT_UNARY_NODE,
				 CVMJIT_TYPEID_ADDRESS);
    objRefNode = nullCheck(con, curbk, objRefNode, emitNullCheck);
    
    targetNode = CVMJITirnodeNewUnaryOp(con, nodeTag, objRefNode);

    return targetNode;
}
#endif
/* IAI - 20 */

/*
 * Build method invocation IR tree.
 *
 * Invoke method on 'targetNode', with a parameter list 'plist'.
 */
static void
invokeMethod(
    CVMJITCompilationContext* con, 
    CVMJITIRBlock* curbk,
    CVMUint8 rtnType,           /* Return type */ 
    CVMJITIRNode* plist,        /* Parameter list */
    CVMInt32      argSize,	/* number of words of parameters */
    CVMBool       isVirtual,    /* true if virtual invoke treated as NV */
    CVMJITIRNode* targetNode)   /* The 'target' to invoke */
{
    CVMJITIRNode* invokeNode;
 
    killAllCachedReferences(con);
#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * Stop adding locals to incomingLocals[], because they'll just end
     * up getting trashed by the method call before we ever use them.
     */
    curbk->noMoreIncomingLocals = CVM_TRUE;
    curbk->noMoreIncomingRefLocals = CVM_TRUE;
#endif

    /* Build invoke node */
    rtnType = CVMJITfoldType(rtnType);
    invokeNode = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_INVOKE(rtnType),
					 plist, targetNode);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_INVOKE_NODES);
    CVMJITirnodeSetThrowsExceptions(invokeNode);
    CVMJITirnodeGetBinaryOp(invokeNode)->data = argSize;
    if (isVirtual) {
	/* The backend needs to know if this is a virtual invoke so the 
	   proper type of invoke patch record can be created.
	*/
	CVMJITirnodeSetBinaryNodeFlag(invokeNode, CVMJITBINOP_VIRTUAL_INVOKE);
    }

    /* If non-void return invoke, push result of invoke onto stack */
    if (rtnType != CVM_TYPEID_VOID) {
	CVMJITirnodeStackPush(con, invokeNode);
    } else { 
        /* Call CVMJITirDoSideEffectOperator() before inserting a root node:*/
        CVMJITirDoSideEffectOperator(con, curbk);

	/* void return invoke, treat it as unevaluated expression */
        CVMJITirnodeNewRoot(con, curbk, invokeNode); 
    }
}

/*
 * Pop the top N words of the stack.
 * Build Effect node that points to the IR node, and
 * append to the current block.
 */
static void
doPop(CVMJITCompilationContext *con, CVMJITIRBlock *curbk, int numberOfWords)
{
    CVMJITIRNode *tosNode;

    /* We have to evaluate the operand at the top of the stack, but we can't
       evaluate that until we have evaluated the items below it.  So we call
       the CVMJITirDoSideEffectOperator() to evaluate the whole thing before
       popping: */
    CVMJITirDoSideEffectOperator(con, curbk);
    tosNode = CVMJITirnodeStackPop(con);
    if ((numberOfWords == 2) && !CVMJITirnodeIsDoubleWordType(tosNode)) {
        tosNode = CVMJITirnodeStackPop(con);
        CVMassert(!CVMJITirnodeIsDoubleWordType(tosNode));
    }
}

/*
 * Build CVMJIT_NEW_OBJECT node for 
 * opc_new object. 
 */
static void 
doNewObject(CVMJITCompilationContext* con, CVMJITIRNode* constCBNode) 
{
    CVMJITIRNode* node =
	CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_NEW_OBJECT, constCBNode);
    CVMJITirnodeSetThrowsExceptions(node);
    CVMJITirnodeStackPush(con, node);
    CVMJITirnodeSetUnaryNodeFlag(node, CVMJITUNOP_ALLOCATION);
    /* Forget all cached index expressions.
       This operation might cause a GC */
    boundsCheckKillIndexExprs(con);
}

/*
 * memRead and memWrite are shared between static and dynamic field access
 */

/*
 * Given an effective address, perform read from this address
 */
static void
memRead(CVMJITCompilationContext* con, 
	CVMJITIRNode*  effectiveAddr,
	CVMUint8       typeTag)        /* 32-bit or 64-bit */
{
    /* Build FETCH node and push it to stack */
    CVMJITIRNode* theNode =
	CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_FETCH(typeTag),
			       effectiveAddr);
    CVMJITirnodeSetHasUndefinedSideEffect(theNode);
    CVMJITirnodeStackPush(con, theNode);
}

/*
 * Given an effective address and a rhs, perform write
 */
static void
memWrite(CVMJITCompilationContext* con, 
	 CVMJITIRBlock* currentBlock,
	 CVMJITIRNode*  effectiveAddr,
	 CVMUint8       typeTag,        /* 32-bit, 64-bit, or ref */
	 CVMJITIRNode*  rhs)
{
    /* Build ASSIGN node */
    CVMJITIRNode* theNode =
	CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_ASSIGN(typeTag), 
				effectiveAddr, rhs);
    CVMJITirnodeSetHasUndefinedSideEffect(theNode);
    CVMJITirnodeNewRoot(con, currentBlock, theNode);
    if (typeTag == CVM_TYPEID_OBJ) {
	/* Reference write. Keep track of write-barriers */
	con->numBarrierInstructionBytes += CVMCPU_GC_BARRIER_SIZE;
    }
    
}

/*
 * Keep two arrays around -- one for array bases, and one for indices 
 */
static void
getfieldCacheInitialize(CVMJITCompilationContext* con)
{
    con->nextGetfieldID = 0;
    con->maxGetfieldID = 0;
}

static void
getfieldCacheKillForLocal(CVMJITCompilationContext* con,
			  CVMUint32 localNo)
{
    int i;
    for (i = 0; i < con->maxGetfieldID; i++) {
	CVMJITIRNode* objNode = con->getfieldObjrefs[i];
	/* Invalid entry */
	if (objNode == NULL) {
	    continue;
	}
	
	if (isLocalN(objNode, localNo)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMconsolePrintf("***** KILLING GETFIELD FOR LOCAL\n");
	    CVMJITirdumpIRNode(con, objNode, 0, "   ");
	    CVMconsolePrintf("\n(OFFSET=%d)\n", con->getfieldOffsets[i]);
#endif
	    /* Kill both. One is useless without the other */
	    con->getfieldObjrefs[i] = NULL;
	    con->getfield[i] = NULL;
	}
    }
}

static CVMInt32
getfieldCacheGetIdx(CVMJITCompilationContext* con,
		    CVMJITIRNode* objRef,
		    CVMUint32 offset)
{
    CVMInt32 i;
    for (i = 0; i < con->maxGetfieldID; i++) {
	CVMJITIRNode* getfieldObjnode = con->getfieldObjrefs[i];
	
	if (getfieldObjnode == NULL) {
	    continue;
	}
	
	if ((con->getfieldOffsets[i] == offset) &&
	    isSameSimple(con, getfieldObjnode, objRef)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
	    CVMJITirdumpIRNode(con, getfieldObjnode, 0, "   ");
	    CVMconsolePrintf("\n(OFFSET=%d)\n", offset);
	    CVMJITirdumpIRNode(con, con->getfield[i], 0, "   ");
	    CVMconsolePrintf("\n");
#endif
	    return i;
	}
    }
    return -1;
}


static CVMJITIRNode*
getfieldCacheGet(CVMJITCompilationContext* con,
		 CVMJITIRNode* objRef,
		 CVMUint32 offset)
{
    CVMInt32 id = getfieldCacheGetIdx(con, objRef, offset);

    if (id != -1) {
	return con->getfield[id];
    } else {
	return NULL;
    }
}

#if 0
static void
getfieldCacheKill(CVMJITCompilationContext* con,
		 CVMJITIRNode* objRef,
		 CVMUint32 offset)
{
    CVMInt32 id = getfieldCacheGetIdx(con, objRef, offset);

    if (id != -1) {
	con->getfieldObjrefs[id] = NULL;
	con->getfield[id] = NULL;
    }
}
#endif

/*
 * Kill all getfield entries corresponding to 'offset'
 */
static void
getfieldCacheKillOffset(CVMJITCompilationContext* con,
			CVMUint32 offset)
{
    CVMInt32 i;
    for (i = 0; i < con->maxGetfieldID; i++) {
	CVMJITIRNode* getfieldObjnode = con->getfieldObjrefs[i];
	
	if (getfieldObjnode == NULL) {
	    continue;
	}
	
	/* Kill */
	if (con->getfieldOffsets[i] == offset) {
	    con->getfieldObjrefs[i] = NULL;
	    con->getfield[i] = NULL;
	}
    }
}

/*
 * Kill all cached field and array reads, and any derived pointers
 */
static void
killAllCachedReferences(CVMJITCompilationContext* con)
{
    getfieldCacheInitialize(con);
    /* Conservatively reset all index expressions computed */
    boundsCheckKillIndexExprs(con);
    /* Conservatively reset all array fetch expressions computed */
    boundsCheckKillFetchExprs(con);
}

/* Key (objrefNode, fieldOffset), value fetchNode */
static void
getfieldCachePut(CVMJITCompilationContext* con, 
		  CVMJITIRNode* objrefNode, 
		  CVMUint16 fieldOffset, 
		  CVMJITIRNode* fetchNode)
{
    con->getfieldObjrefs[con->nextGetfieldID] = objrefNode;
    con->getfieldOffsets[con->nextGetfieldID] = fieldOffset;
    con->getfield[con->nextGetfieldID] = fetchNode;
    
    con->nextGetfieldID++;
    con->nextGetfieldID %= JIT_MAX_CHECK_NODES;
    if (con->maxGetfieldID < JIT_MAX_CHECK_NODES) {
	con->maxGetfieldID++;
    }
}

/* Purpose: Initialize the arrayLengthNode cache. */
static void
arrayLengthCacheInitialize(CVMJITCompilationContext* con)
{
    con->nextArrayLengthID = 0;
    con->maxArrayLengthID = 0;
}

/* Purpose: Invalidates any cached arrayLengthNodes that are for on an
            arrayNode store in the specified local. */
static void
arrayLengthCacheKillForLocal(CVMJITCompilationContext* con, CVMUint32 localNo)
{
    int i;
    for (i = 0; i < con->maxArrayLengthID; i++) {
        CVMJITIRNode* objNode = con->arrayLengthObjrefs[i];
        /* Invalid entry */
        if (objNode == NULL) {
            continue;
        }

        if (isLocalN(objNode, localNo)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
            CVMconsolePrintf("***** KILLING ARRAYLENGTH FOR LOCAL\n");
            CVMJITirdumpIRNode(con, objNode, 0, "   ");
            CVMconsolePrintf("\n");
#endif
            /* Kill both. One is useless without the other */
            con->arrayLengthObjrefs[i] = NULL;
            con->arrayLength[i] = NULL;
        }
    }
}

/* Purpose: Gets the cache ID for the specified arrayNode if available. */
static CVMInt32
arrayLengthCacheGetIdx(CVMJITCompilationContext* con, CVMJITIRNode* objRef)
{
    CVMInt32 i;
    for (i = 0; i < con->maxArrayLengthID; i++) {
        CVMJITIRNode* arrayLengthObjnode = con->arrayLengthObjrefs[i];

        if (arrayLengthObjnode == NULL) {
            continue;
        }

        if (isSameSimple(con, arrayLengthObjnode, objRef)) {
#ifdef CVM_DEBUG_RUNTIME_CHECK_ELIMINATION
            CVMJITirdumpIRNode(con, arrayLengthObjnode, 0, "   ");
            CVMconsolePrintf("\n");
            CVMJITirdumpIRNode(con, con->arrayLength[i], 0, "   ");
            CVMconsolePrintf("\n");
#endif
            return i;
        }
    }
    return -1;
}


/* Purpose: Gets the arrayLength node for the specified arrayNode if
            available. */
static CVMJITIRNode*
arrayLengthCacheGet(CVMJITCompilationContext* con, CVMJITIRNode* objRef)
{
    CVMInt32 id = arrayLengthCacheGetIdx(con, objRef);
    if (id != -1) {
        return con->arrayLength[id];
    } else {
        return NULL;
    }
}

/* Purpose: Cache the arrayLength node for the specified arrayNode. */
static void
arrayLengthCachePut(CVMJITCompilationContext* con, CVMJITIRNode* arrayrefNode,
                    CVMJITIRNode* arrayLengthNode)
{
    con->arrayLengthObjrefs[con->nextArrayLengthID] = arrayrefNode;
    con->arrayLength[con->nextArrayLengthID] = arrayLengthNode;
    con->nextArrayLengthID++;
    con->nextArrayLengthID %= JIT_MAX_ARRAY_LENGTH_CSE_NODES;
    if (con->maxArrayLengthID < JIT_MAX_ARRAY_LENGTH_CSE_NODES) {
        con->maxArrayLengthID++;
    }
}

/*
 * Handle dynamic fields:
 *    references and non-references
 *    32- and 64-bit values
 *    reads and writes
 */
static void
doInstanceFieldRead(CVMJITCompilationContext* con, 
		    CVMJITIRBlock* currentBlock,
		    CVMUint16      relPC,
		    CVMJITIRNode*  fieldOffsetNode,/* can be NULL */
		    CVMUint16      fieldOffset,    /* Object offset */
		    CVMUint8       typeTag,        /* 32-bit, 64-bit, or ref */
		    CVMBool        isVolatile)
{
    CVMJITIRNode* objrefNode;
    CVMJITIRNode* fieldRefNode;
    CVMJITIRNode* fetchNode;
    CVMBool       cacheIt;
    
    /* The caller either passes in a fieldOffsetNode when the cp entry
     * is unresolved, or the actual fieldOffset when the entry is resovled.
     * In the later case, we need to create the Constant32 to hold the offset.
     */
    objrefNode   = CVMJITirnodeStackPop(con);

    if (fieldOffsetNode == NULL) {
	CVMJITIRNode* cachedNode;
	/* Constant offset. Let's look for the cached FETCH(FIELD_REF(..))
	   node, if one exists. */
	cachedNode = getfieldCacheGet(con, objrefNode, fieldOffset);
	if (cachedNode != NULL) {
	    CVMJITirnodeStackPush(con, cachedNode);
	    /* Done. */
	    return;
	}
	/* There were no cached nodes. We'll have to go and build one */
	fieldOffsetNode = CVMJITirnodeNewConstantFieldOffset(con, fieldOffset);
	cacheIt = !isVolatile;
    } else {
	cacheIt = CVM_FALSE;
    }

    /* null check on the objref node */
#ifndef CVMJIT_TRAP_BASED_NULL_CHECKS
    objrefNode = nullCheck(con, currentBlock, objrefNode, CVM_TRUE);
#else
    objrefNode = nullCheck(con, currentBlock, objrefNode, CVM_FALSE);
#endif

    /* Build effective addr of FIELD_REF node */
    typeTag = CVMJITfoldType(typeTag);
    fieldRefNode =
	CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_FIELD_REF(typeTag), 
				objrefNode, fieldOffsetNode);

    if (isVolatile) {
	CVMJITirnodeSetBinaryNodeFlag(fieldRefNode, CVMJITBINOP_VOLATILE_FIELD);
    }

    /* Build FETCH node and push it to stack */
    fetchNode =	CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_FETCH(typeTag),
				       fieldRefNode);
    CVMJITirnodeSetHasUndefinedSideEffect(fetchNode);
    if (cacheIt) {
	/* Force evaluation of everyone before this fetch */
	CVMJITirDoSideEffectOperator(con, currentBlock);
	/* And force evaluation of the fetch itself */
	CVMassert(CVMJITirnodeHasSideEffects(fetchNode));
	CVMJITirForceEvaluation(con, currentBlock, fetchNode);
	/* Finally cache: Key (objrefNode, fieldOffset), value fetchNode */
	getfieldCachePut(con, objrefNode, fieldOffset, fetchNode);
    }
    CVMJITirnodeStackPush(con, fetchNode);
}

/*
 * Handle dynamic fields:
 *    references and non-references
 *    32- and 64-bit values
 *    reads and writes
 */
static void
doInstanceFieldWrite(CVMJITCompilationContext* con, 
		     CVMJITIRBlock* currentBlock,
		     CVMUint16      relPC,
		     CVMJITIRNode*  fieldOffsetNode,/* can be NULL */
		     CVMUint16      fieldOffset,    /* Object offset */
		     CVMUint8       typeTag,        /* 32-bit, 64-bit, or ref */
		     CVMBool        isVolatile)
{
    CVMJITIRNode* valueNode;
    CVMJITIRNode* objrefNode;
    CVMJITIRNode* fieldRefNode;
    
    /* If we're in the process of emitting an ASSIGN node (which is a
       root node), we need to call the CVMJITirDoSideEffectOperator() to
       preserve correct evaluation order of everything that is still
       on the stack. */
    CVMJITirDoSideEffectOperator(con, currentBlock);
    valueNode = CVMJITirnodeStackPop(con);
    objrefNode   = CVMJITirnodeStackPop(con);

    /* The caller either passes in a fieldOffsetNode when the cp entry
     * is unresolved, or the actual fieldOffset when the entry is resovled.
     * In the later case, we need to create the Constant32 to hold the offset.
     */
    if (fieldOffsetNode == NULL) {
	fieldOffsetNode = CVMJITirnodeNewConstantFieldOffset(con, fieldOffset);
	/* Kill all remembered fields at 'fieldOffset' */
	getfieldCacheKillOffset(con, fieldOffset);
    } 

    /* null check on the objref node */
#ifndef CVMJIT_TRAP_BASED_NULL_CHECKS
    objrefNode = nullCheck(con, currentBlock, objrefNode, CVM_TRUE);
#else
    objrefNode = nullCheck(con, currentBlock, objrefNode, CVM_FALSE);
#endif

    /* Build effective addr of FIELD_REF node */
    typeTag = CVMJITfoldType(typeTag);
    fieldRefNode =
	CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_FIELD_REF(typeTag), 
				objrefNode, fieldOffsetNode);

    if (isVolatile) {
	CVMJITirnodeSetBinaryNodeFlag(fieldRefNode, CVMJITBINOP_VOLATILE_FIELD);
    }

    memWrite(con, currentBlock, fieldRefNode, typeTag, valueNode);
}

/* Purpose: Checks if a CHECKINIT node needs to be emitted. */
static CVMBool
checkInitNeeded(CVMJITCompilationContext *con, CVMClassBlock *cb)
{
#if defined(CVM_AOT) && !defined(CVM_MTASK)
    /*
     * Always generate CHECKINIT for AOT code because we can't
     * guarantee that the class has already initialized when
     * AOT code is executed in subsequent runs. No need to do
     * this for MTASK because we know clinit will be run
     * by the warmup process in subsequent runs.
     */
    if (CVMglobals.jit.isPrecompiling) {
        return CVM_TRUE;
    } else
#endif
    {
        CVMBool needCheckInit = CVMcbCheckInitNeeded(cb, con->ee);
  
        return needCheckInit;
    }
}

/*
 * Emit CHECKINIT node on uninitialized class
 */
static CVMJITIRNode *
doCheckInitOnCB(CVMJITCompilationContext *con, CVMClassBlock *cb,
                CVMJITIRBlock *curbk, CVMJITIRNode *valueNode)
{
    /* Build constant of CVMJIT_CONST_CB */
    CVMJITIRNode *cbNode = CVMJITirnodeNewConstantCB(con, cb); 
    CVMJITIRNode* checkinitNode;
    CVMUint8 flag;

    /* This node means arbitrary <clinit> code can be run here. So kill
       all cached pointers */
    killAllCachedReferences(con);
    if (CVMcbIsLVMSystemClass(cb)) {
	flag = CVMJITUNOP_CLASSINIT_LVM;
    } else {
	flag = CVMJITUNOP_CLASSINIT;
    }
    checkinitNode =
        CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_CLASS_CHECKINIT, cbNode,
                                valueNode);
    CVMJITirnodeSetThrowsExceptions(checkinitNode);

    /* Make room in the code buffer for the checkinit code: */
    con->numLargeOpcodeInstructionBytes += CVMCPU_CHECKINIT_SIZE;
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_CHECKINIT_NODES);

    CVMJITirnodeSetBinaryNodeFlag(checkinitNode, flag);
    return checkinitNode;
}

/*
 * Handle static fields:
 *    32- and 64-bit values
 *    reads and writes
 */

static void
doStaticFieldRef(CVMJITCompilationContext* con, 
		 CVMJITIRBlock*   curbk,
		 CVMConstantPool* cp,
		 CVMUint16        cpIndex,
		 CVMClassBlock*   currCb,
		 CVMBool          isRead)    /* read or write */
{
    CVMExecEnv *ee = con->ee;
    CVMJITIRNode*   staticRefNode;
    CVMJITIRNode*   fieldAddressNode;
    CVMUint8        typeTag;
    CVMFieldTypeID  fid = CVM_TYPEID_ERROR;
    CVMJITIRNode*   valueNode = NULL;
    CVMBool         isVolatile;

    /* If we're in the process of emitting an ASSIGN node (which is a root
       node), we need to call the CVMJITirDoSideEffectOperator() to preserve
       correct evaluation order of everything that is still on the stack. */
    if (!isRead) {
        CVMJITirDoSideEffectOperator(con, curbk);
        /* Get the right-hand side if this is a write */
        valueNode = CVMJITirnodeStackPop(con);
    }

    if (CVMcpCheckResolvedAndGetTID(ee, currCb, cp, cpIndex, &fid)) {
	CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);
        CVMClassBlock* cb;
        CVMJavaVal32*  staticArea;
	CVMBool initNeeded;

        /* If the field is 64-bit volatile type, refuse to compile 
         * the method. Since 64-bit volatile fields are rare,
         * failure to compile methods that access this type of
         * fields should not have noticeable impact on performance.
         */
	isVolatile = CVMfbIs(fb, VOLATILE);

	fid = CVMfbNameAndTypeID(fb);

#ifndef CVM_TRUSTED_CLASSLOADERS
        /* Make sure that both the opcode and the fb agree on whether or
         * not the field is static: */
        /*
         * Make sure we aren't trying to store into a final field.
         * The VM spec says that you can only store into a final field
         * when initializing it. The JDK seems to interpret this as meaning
         * that a class has write access to its final fields, so we do
         * the same here (see how isFinalField gets setup above).
         */
        if (!CVMfbIs(fb, STATIC) ||
	    (!isRead && !CVMfieldIsOKToWriteTo(ee, fb, currCb, CVM_FALSE)))
	{
            /* If not, we treat it as if it is unresolved and let the
               compiler runtime take care of throwing an exception: */
            goto createResolveStaticFBNode;
        }
#endif
        cb = CVMfbClassBlock(fb);
        staticArea = &CVMfbStaticField(ee, fb);
	typeTag = CVMtypeidGetPrimitiveType(fid);

	initNeeded = checkInitNeeded(con, cb);

	/* Treat static final fields as constants */
#if 0
	/* 5025530: we can't enable this code because it is actually legal
	 * to initialize static final fields from native code after the
	 * class has been initialized. Fortunately non-ref static final
	 * fields are treated as contants by javac, so this rarely helps
	 * performance anyway.
	 */
	if (isRead && CVMfbIs(fb, FINAL) && !initNeeded) {
	    CVMBool handled = CVM_FALSE;
	    switch (typeTag) {
	    case CVM_TYPEID_OBJ:
		if (CVMID_icellIsNull(&staticArea->r)) {
		    CVMJITIRNode* node = 
			CVMJITirnodeNewConstantStringObject(con, NULL);
		    CVMJITirnodeStackPush(con, node);
		    handled = CVM_TRUE;
		}
		break;
	    case CVM_TYPEID_BYTE:
	    case CVM_TYPEID_CHAR:
	    case CVM_TYPEID_SHORT:
	    case CVM_TYPEID_BOOLEAN:
	    case CVM_TYPEID_INT:
		pushConstantJavaInt(con, staticArea->i);
		handled = CVM_TRUE;
		break;
	    case CVM_TYPEID_FLOAT:
		pushConstantJavaFloat(con, staticArea->f);
		handled = CVM_TRUE;
		break;
	    case CVM_TYPEID_LONG:
	    case CVM_TYPEID_DOUBLE: {
		CVMJavaVal64 v64;
		CVMmemCopy64(v64.v, &staticArea->raw);
		pushConstantJavaVal64(con, &v64, typeTag);
		handled = CVM_TRUE;
		break;
	    }
	    default:
		CVMconsolePrintf("Bad type %x\n", typeTag);
	    }
	    if (handled) {
		return;
	    }
	}
#endif

	/* %comment c023 */
	fieldAddressNode =
	    CVMJITirnodeNewConstantStaticFieldAddress(con, staticArea);

	if (initNeeded) {
	    fieldAddressNode =
		doCheckInitOnCB(con, cb, curbk, fieldAddressNode);
	}
    } else { /* Unresolved cp entry */
#ifndef CVM_TRUSTED_CLASSLOADERS
    createResolveStaticFBNode:
#endif
        typeTag = CVMtypeidGetPrimitiveType(fid);

	/* Since the field is unresolved.  We assume it is volatile just to be
	   safe. */
	isVolatile = CVM_TRUE;

	/* Resolution could cause Java class loader code to run. Kill
	   cached pointers. */
	killAllCachedReferences(con);
	
        /* Build RESOLVE_REFERENCE node with opcode
           CVMJIT_CONST_GETSTATIC_FB_UNRESOLVED or
           CVMJIT_CONST_PUTSTATIC_FB_UNRESOLVED, and the CVMJITUNOP_CLASSINIT
           flag set.  Then, append it to IR root list. */
	fieldAddressNode = 
	    CVMJITirnodeNewResolveNode(con, cpIndex, curbk,
                CVMJITUNOP_CLASSINIT,
                isRead ? CVMJIT_CONST_GETSTATIC_FB_UNRESOLVED :
                         CVMJIT_CONST_PUTSTATIC_FB_UNRESOLVED);
    }
    
    /* Build effective addr of STATIC_FIELD_REF node */
    typeTag = CVMJITfoldType(typeTag);
    staticRefNode = 
	CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_STATIC_FIELD_REF(typeTag),
			       fieldAddressNode);

    if (isVolatile) {
	CVMJITirnodeSetUnaryNodeFlag(staticRefNode, CVMJITUNOP_VOLATILE_FIELD);
    }

    /* Now that we have built the "effective address" it is time to do
       the read or write */
    if (isRead) {
	memRead(con, staticRefNode, typeTag);
    } else {
	memWrite(con, curbk, staticRefNode, typeTag, valueNode);
    }
}

/*
 * Load the Class object from the CB.  Resolve the constant pool
 * entry first if necessary.
 */
static void
doJavaInstanceRead(CVMJITCompilationContext* con, 
		   CVMJITIRBlock*   curbk,
		   CVMConstantPool* cp,
		   CVMUint16        cpIndex,
		   CVMClassBlock*   currCb)
{
    CVMExecEnv *ee = con->ee;
    CVMJITIRNode *javaInstanceRefNode;

    if (CVMcpCheckResolvedAndGetTID(ee, currCb, cp, cpIndex, NULL)) {
	/*
	   CB is resolved.  Load the value of CVMcbJavaInstance(cb),
	   which is at a fixed address.
        */
	CVMClassBlock *cpCb = CVMcpGetCb(cp, cpIndex);
	/* FETCHADDR(STATICADDR(CVMcbJavaInstance(cb))) */
	javaInstanceRefNode =
	    CVMJITirnodeNewConstantStaticFieldAddress(con,
		(CVMAddr)CVMcbJavaInstance(cpCb));
    } else {
	/*
	   CB is not resolved.  Resolve it, then fetch
	   CVMcbJavaInstance(cb) using a FIELD_REF expression.
        */
	CVMJITIRNode *cbNode = CVMJITirnodeNewResolveNode(
	    con, cpIndex, curbk, 0, CVMJIT_CONST_CB_UNRESOLVED);
	CVMJITIRNode *fieldOffsetNode =
	    CVMJITirnodeNewConstantJavaNumeric32(con,
		OFFSET_CVMClassBlock_javaInstanceX,
		CVM_TYPEID_INT);
	/* FETCHADDR(FIELDADDR(cb, javaInstanceX)) */
	javaInstanceRefNode = CVMJITirnodeNewUnaryOp(con,
	    CVMJIT_ENCODE_FETCH(CVMJIT_TYPEID_ADDRESS),
	    CVMJITirnodeNewBinaryOp(con,
		CVMJIT_ENCODE_FIELD_REF(CVMJIT_TYPEID_ADDRESS), 
		cbNode, fieldOffsetNode));
    }
    {
	/*
	   We have the cb->javaInstanceX value now, which
	   points to an ICell.  Fetch the object from the
	   ICell.
	 */

	/* fetchNode: FETCHOBJ(STATICOBJ(javaInstanceRefNode)) */
	CVMJITIRNode *fetchNode = CVMJITirnodeNewUnaryOp(con,
	    CVMJIT_ENCODE_FETCH(CVM_TYPEID_OBJ),
	    CVMJITirnodeNewUnaryOp(con,
		CVMJIT_ENCODE_STATIC_FIELD_REF(CVM_TYPEID_OBJ),
		javaInstanceRefNode));

	/* We could even treat this like a getfield and cache it. */

	CVMJITirnodeStackPush(con, fetchNode);
    }
}

static CVMBool
crossesProtectionDomains(CVMJITCompilationContext* con,
                         CVMMethodBlock* callerMb,
                         CVMMethodBlock* targetMb)
{
    CVMObjectICell* callerPd = CVMcbProtectionDomain(CVMmbClassBlock(callerMb));
    CVMObjectICell* targetPd = CVMcbProtectionDomain(CVMmbClassBlock(targetMb));
    CVMBool isSameProtectionDomain;
   
    CVMID_icellSameObjectNullCheck(con->ee, callerPd, targetPd,
                                   isSameProtectionDomain);

    if (targetPd == NULL || CVMID_icellIsNull(targetPd) 
                         || isSameProtectionDomain) {
        return CVM_FALSE;
    } else {
        return CVM_TRUE;
    }
}

/*
 * Do a quick pass over a target method to figure out if it is inlinable
 * into its caller 
 */
static Inlinability
isInlinable(CVMJITCompilationContext* con, CVMMethodBlock* targetMb)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJavaMethodDescriptor* jmd;
    CVMClassBlock* cb;
    CVMConstantPool* cp;
    CVMUint8* codeBegin;
    CVMUint8* codeEnd;
    CVMUint8* pc;
    CVMUint32 numArgLocals;
    CVMUint32 numLocals;
#ifdef CVM_TRACE_JIT
    const char* reason = NULL;
#endif
    Inlinability status = 0;
    CVMBool   stores = CVM_FALSE;
    CVMBool   storesParameters = CVM_FALSE;
    CVMBool   hasBranches = CVM_FALSE;
    CVMBool   callsOut = CVM_FALSE;
    CVMBool   allowedInvokes;
    CVMBool   backwardsBranchesAllowed = CVM_TRUE;
    /* Are we doing aggressive compilation? */
    CVMBool   unquickOK =
	!((jgs->compileThreshold != 0) &&
	  ((CVMmbCompileFlags(con->mb) & CVMJIT_COMPILE_ONCALL) == 0));
    CVMBool   alwaysInline = 
	(CVMmbCompileFlags(targetMb) & CVMJIT_ALWAYS_INLINABLE) != 0;
    CVMUint32 localNo;
    int maxInliningCodeLength;
#ifdef CVM_MTASK
    CVMBool iamserver = (CVMglobals.isServer && CVMglobals.clientId <= 0);
#endif

#ifdef CVM_JVMTI
    /* reject obsolete methods to make life easier */
    if (CVMjvmtiMbIsObsolete(targetMb)) {
	return InlineNotInlinable; 
    }
#endif

    /*
     * If the caller of this method has the CVMJIT_NEEDS_TO_INLINE flag set,
     * then don't let any of the imposed limits, like
     * con->maxAllowedInliningDepth, prevent this method from being inlined.
     */
    if ((CVMmbCompileFlags(con->mc->mb) & CVMJIT_NEEDS_TO_INLINE) != 0) {
	alwaysInline = CVM_TRUE;
    }
    
    /* Setup maxInliningCodeLength based on the current compilation depth */
    {
	float inlineDepthRatio; /* % of max inline depth we are at */
	if (con->maxAllowedInliningDepth == 0) {
	    inlineDepthRatio = 0;
	} else {
	    inlineDepthRatio = (float)con->mc->compilationDepth / 
		(float)jgs->maxAllowedInliningDepth;
	}
	maxInliningCodeLength = jgs->maxInliningCodeLength -
	    (int)(jgs->maxInliningCodeLength * inlineDepthRatio);
	/* always allow at least jgs->minInliningCodeLength */
	if (maxInliningCodeLength < jgs->minInliningCodeLength) {
	    maxInliningCodeLength = jgs->minInliningCodeLength;
	}
    }

    /* allow 3 invokes plus 1 per 8 bytecodes. This is somewhat
     * arbitrary. The main reason allowedInvokes exists is so it can
     * be set to 1 a bit further below. */
    allowedInvokes = 3 + maxInliningCodeLength / 8;

    /* Only java methods can be inlined */
    if (!CVMmbIsJava(targetMb)) {
	CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
			     "is not a Java method\n",
			     CVMmbClassBlock(targetMb), targetMb));
	return InlineNotInlinable; 
    }

    jmd = CVMmbJmd(targetMb);

    /* unquick opcodes are ok in methods half the size of the max size */
    if (CVMjmdCodeLength(jmd) < maxInliningCodeLength / 2) {
        unquickOK = CVM_TRUE;
        goto always_inline_short_methods;
    }

    /*
     * Don't try to inline if the call site hasn't been quickened,
     * unless we are doing aggressive compilation.
     */
    if (!alwaysInline && !unquickOK && !CVMbcAttr(con->mc->startPC[0], QUICK)){
	CVMtraceJITInlining(("NOT INLINABLE: Call site for method %C.%M "
			     "is not quickened\n",
			     CVMmbClassBlock(targetMb), targetMb));
	return InlineNotInlinable; 
    }

always_inline_short_methods:
    if (alwaysInline &&
	con->mc->compilationDepth > jgs->maxAllowedInliningDepth) {
	/* Normally alwaysInline will allow us to ignore the
	 * compilationDepth (see check immediately below), but we
	 * don't want to allow it to exceed jgs->maxAllowedInliningDepth.
	 * In this case we want to retry with less inlining, which should
	 * prevent the caller of this method from being inlined.
	 */
	CVMJITlimitExceeded(con, "global maxAllowedInliningDepth exceeded.");
    }

    /* don't exceed max compilation depth */
    if (!alwaysInline &&
	con->mc->compilationDepth >= con->maxAllowedInliningDepth)
    {
	CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
			     "is too deep in the call chain (depth=%d)\n",
			     CVMmbClassBlock(targetMb), targetMb,
			     con->mc->compilationDepth + 1));
#if 0
	CVMtraceJITInlining(("CALLER: %C.%M\n",
			     CVMmbClassBlock(con->mc->mb),
			     con->mc->mb));
#endif
	return InlineNotInlinable; 
    }

    /* Reject if target method is not hot enough */
    if (!alwaysInline) {
        CVMInt32 cost;
        CVMInt32 threshold;

        /* The inlining threshold table is used to determine if it is OK to
           inline a certain target method at a certain inlining depth.
           Basically, we use the current inlining depth to index into the
           inlining threshold table and come up with a threshold value.  If
           the target method's invocation cost has decreased to or below the
           threshold value, then we'll allow that target method to be inlined.

           For inlining depths less or equal to 6, the threshold value are
           determine based on the quadratic curve:
               100 * x^2.
           For inlining depths greater than 6, the target method must have a
           cost of 0 in order to be inlined.

           The choice of a quadratic mapping function, the QUAD_COEFFICIENT of
           100, and EFFECTIVE_MAX_DEPTH_THRESHOLD of 6 were determined by
           testing.  These heuristics were found to produce an effective
           inilining policy.
        */
        cost = CVMmbInvokeCost(targetMb);
        threshold =
            jgs->inliningThresholds[con->mc->compilationDepth];
#ifdef CVM_MTASK
	/*
	 * During warmup, render threshold useless. Since very little
	 * has been executed, we don't want this to limit inlining.
	 */
	if (iamserver) {
	    threshold = jgs->compileThreshold;
	}
#endif

	/*
	 * Don't inline if not below the required cost threshold. However,
	 * don't allow the threshold to prevent inlining of methods that
	 * have been flagged to have the threshold ignored.
	 */
	if ((CVMmbCompileFlags(targetMb) & CVMJIT_IGNORE_THRESHOLD) == 0) {
	    if (cost > threshold) {
		CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
				     "is not hot enough (%d/%d)\n",
				     CVMmbClassBlock(targetMb), targetMb,
				     cost, threshold));
		return InlineNotInlinable; 
	    }
	}
    }

    /* Reject methods already marked as not inlinable */
    if ((CVMmbCompileFlags(targetMb) & CVMJIT_NOT_INLINABLE) != 0) {
	CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
			     "is already marked not inlinable\n",
			     CVMmbClassBlock(targetMb), targetMb));
	return InlineNotInlinable;
    }

    /* Reject long methods */
    if (!alwaysInline && CVMjmdCodeLength(jmd) > maxInliningCodeLength) {
	/* Allow methods up to 1.5 * maxInliningCodeLength as long as
	 * they contain no more than one invoke and no backward branches.
	 */
	if ((float)CVMjmdCodeLength(jmd) < (float)maxInliningCodeLength * 1.5){
	    allowedInvokes = 1;
	    backwardsBranchesAllowed = CVM_FALSE;
	} else {
	    CVMtraceJITInlining((
                "NOT INLINABLE: Target method %C.%M is too big (%d)\n",
		CVMmbClassBlock(targetMb), targetMb,
		CVMjmdCodeLength(jmd)));
	    return InlineNotInlinable;
	}
    }

    /* Reject if stack grows beyond recommmended capacity */
    if (con->mc->currCapacity + CVMjmdCapacity(jmd) > con->maxCapacity) {
	if (alwaysInline) {
	    /* We can't allow this method to not get inlined, so we must
	     * fail this compilation and retry with a max inlining depth
	     * one less. This should prevent the caller of this method
	     * from getting inlined.
	     */
	    CVMJITlimitExceeded(con, "Exceeded max stack capacity.");
	}
	/* Don't mark uninlinable, and return. This limit failure is due
	   to the call path leading up to here, and it might
	   not be repeated for another caller */
	CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
			     "exceeds maximum stack capacity\n",
			     CVMmbClassBlock(targetMb), targetMb));
	return InlineNotInlinable; 
    }

    cb = CVMmbClassBlock(targetMb);
#ifdef CVM_JVMTI
    if (CVMjvmtiMbIsObsolete(targetMb)) {
	cp = CVMjvmtiMbConstantPool(targetMb);
	if (cp == NULL) {
	    cp = CVMcbConstantPool(cb);
	}	
    } else {
#else
    {
#endif

	cp = CVMcbConstantPool(cb);
    }

    /* Reject exception methods. They are mostly a waste of time */
    if (CVMisSubclassOf(con->ee, cb, CVMsystemClass(java_lang_Throwable))) {
	CVMtraceJITInliningExec(reason = "in a subclass of Throwable";)
	goto not_inlinable;
    }
    
    /* Reject sync methods */
    if (CVMmbIs(targetMb, SYNCHRONIZED)) {
	if (CVMmbIs(targetMb, STATIC)) {
	    CVMtraceJITInliningExec(reason = "synchronized static";)
	    goto not_inlinable;
	}
    }
    /* Reject methods with exception handlers */
    if (CVMjmdExceptionTableLength(jmd) > 0) {
	CVMtraceJITInliningExec(reason = "able to catch exceptions";)
	goto not_inlinable;
    }
 
    /*
     * Only allow leaf methods at the deepest inline depth. This prevents
     * inlining a method that isn't capable of inlining simple getter
     * and setter methods.
     */
    if (con->mc->compilationDepth == con->maxAllowedInliningDepth - 1) {
	allowedInvokes = 0;
    }

    /* Reject cross ProtectionDomain inlining in order to preserve
     * security semantics. However, there are a some special cases
     * we want to make them inlinable:
     *
     * (1) target method is empty
     * (2) target PD == null
     * (3) target PD == the outer most caller PD (allow nested 
     *     inlining. For example PD X calls PD null calls PD X) 
     */
    if (CVMjmdCodeLength(jmd) != 1) {
        CVMMethodBlock* callerMb = con->mb;
        if (crossesProtectionDomains(con, callerMb, targetMb)) {
            /* Don't mark uninlinable, and return. This limit failure is due
               to the call path leading up to here, and it might
               not be repeated for another caller */
               CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M crosses"
                                    " protection domains from source %C.%M\n",
                                    CVMmbClassBlock(targetMb), targetMb,
                                    CVMmbClassBlock(callerMb), callerMb));
               return InlineNotInlinable;
        }
    }
 
    codeBegin = CVMjmdCode(jmd);
    codeEnd   = codeBegin + CVMjmdCodeLength(jmd);
    pc = codeBegin;
    numArgLocals = CVMmbArgsSize(targetMb);
    while (pc < codeEnd) {
	CVMOpcode opcode = *pc;
        CVMUint32 instrLength = CVMopcodeGetLength(pc);

	if (CVMbcAttr(opcode, BRANCH)) {
	    hasBranches = CVM_TRUE;
	}
	if (CVMbcAttr(opcode, INVOCATION) &&
	    opcode != opc_invokeignored_quick)
        {
	    callsOut = CVM_TRUE;
	    /* if invokes aren't allowed then don't inline */
	    if (!alwaysInline && allowedInvokes == 0) {
		CVMtraceJITInlining((
                    "NOT INLINABLE: Target method %C.%M - "
		    "invocations not allowed because of depth or size.\n",
		    CVMmbClassBlock(targetMb), targetMb));
		return InlineNotInlinable;
	    } else {
		allowedInvokes--;
	    }
	}

	switch(opcode) {
	case opc_goto:
	case opc_ifeq:
	case opc_ifne:
	case opc_iflt:
	case opc_ifle:
	case opc_ifgt:
	case opc_ifge:
        case opc_if_icmpeq:
        case opc_if_icmpne:
        case opc_if_icmplt:
        case opc_if_icmple:
        case opc_if_icmpgt:
        case opc_if_icmpge:
	case opc_if_acmpeq:
	case opc_if_acmpne:
	case opc_ifnull:
	case opc_ifnonnull:
	    /* Don't inline if this is a backwards branch and backwards
	     * branches are not allowed. */
	    if (!backwardsBranchesAllowed) {
		CVMInt16 pcOffset = CVMgetInt16(pc+1);
		if (pcOffset < 0) {
		    CVMtraceJITInlining((
                        "NOT INLINABLE: Target method %C.%M - "
			"backward branches not allowed because size.\n",
			CVMmbClassBlock(targetMb), targetMb));
		    return InlineNotInlinable;
		}
	    }
	    break;
	    
	case opc_astore: case opc_dstore: case opc_lstore: 
	case opc_fstore: case opc_istore:
	    localNo = pc[1];
	    stores = CVM_TRUE;
	    if (localNo < numArgLocals) {
		storesParameters = CVM_TRUE;
	    }
	    break;
	case opc_istore_0: case opc_istore_1: case opc_istore_2: 
	case opc_istore_3: case opc_lstore_0: case opc_lstore_1:
	case opc_lstore_2: case opc_lstore_3: case opc_fstore_0: 
	case opc_fstore_1: case opc_fstore_2: case opc_fstore_3:
	case opc_dstore_0: case opc_dstore_1: case opc_dstore_2: 
	case opc_dstore_3: case opc_astore_0: case opc_astore_1:
	case opc_astore_2: case opc_astore_3: case opc_iinc:
	    localNo = CVMJITOpcodeMap[opcode][JITMAP_CONST];
	    stores = CVM_TRUE;
	    if (localNo < numArgLocals) {
		storesParameters = CVM_TRUE;
	    }
	    break;
	
        case opc_wide:
            localNo = CVMgetUint16(pc+2);
            opcode = pc[1];
            switch(opcode) {
                case opc_astore:
                case opc_istore:
                case opc_fstore:
                case opc_lstore:
                case opc_dstore:
	        case opc_iinc: {
		    stores = CVM_TRUE;
		    if (localNo < numArgLocals) {
			storesParameters = CVM_TRUE;
		    }
		    break;
                }
	        case opc_ret: {
		    CVMtraceJITInliningExec(reason = "has ret opcode");
                    goto not_inlinable;
                }
	    default:;
            }/* end of switch */
	    break;

	case opc_ldc: case opc_ldc_w: case opc_ldc2_w: {
	    /* only consider methods with unquick opcodes when compile=all,
	       or if the outermost method has been marked for aggressive
	       compilation */
	    if (!unquickOK && !hasBranches) {
		/* Don't want any "quickenables". If they are still
		   "unquick", it's not worth it */
	    not_quickened:
		CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
				     "is not totally quickened\n",
				     CVMmbClassBlock(targetMb), targetMb));
		return InlineNotInlinable; 
	    }
	    break;
	}

#ifdef CVM_MTASK
	case opc_instanceof_quick: case opc_checkcast_quick:  {
	    /* If we are processing the warmup list then don't allow
	       checkcast or instanceof */
	    if (iamserver) {
		CVMtraceJITInliningExec(
		    reason = "checkcast or instance of during warmup";)
		goto not_inlinable;
	    }
	    break;
	}
#endif

        case opc_invokevirtual: case opc_invokespecial: case opc_putfield:
	case opc_getfield: case opc_invokestatic: case opc_getstatic:
        case opc_putstatic: case opc_new: case opc_invokeinterface:
        case opc_anewarray: case opc_multianewarray:
	case opc_instanceof: case opc_checkcast: {
	    CVMUint16 cpIndex;

#ifdef CVM_MTASK
	    /* If we are processing the warmup list then don't allow
	       checkcast or instanceof */
	    if (iamserver) {
		if (opcode == opc_checkcast || opcode == opc_instanceof) {
		    CVMtraceJITInliningExec(
		        reason = "checkcast or instance of during warmup";)
			goto not_inlinable;
		}
	    }
#endif

	    /* only consider methods with unquick opcodes when compile=all,
	       or if the outermost method has been marked for aggressive
	       compilation */
	    if (!unquickOK && !hasBranches) {
		/* Don't want any "quickenables". If they are still
		   "unquick", it's not worth it */
		goto not_quickened; 
	    }

	    if (opcode != opc_newarray) {
		/* If we are here, the only not inlinable path is if
		   we can resolve the argument cp entry but we can't
		   make an array without creating a new class
		   (i.e. potentially calling into Java). If the opcode
		   is not newarray, this is irrelevant, so break */
		break;
	    }
	    
	    CVMassert(opcode == opc_newarray);
	    /* get the cpIndex in a safe way */
	    CVM_CODE_LOCK(con->ee);
	    if (opcode != *pc) {
		/* it got quickened on us. */
		CVMassert(CVMbcAttr(*pc, QUICK));
		CVM_CODE_UNLOCK(con->ee);
		break;
	    }
	    cpIndex = CVMgetUint16(pc+1);
	    CVM_CODE_UNLOCK(con->ee);

	    if (CVMcpCheckResolvedAndGetTID(con->ee, cb, cp, cpIndex, NULL)) {
		CVMClassBlock* elmentCb = CVMcpGetCb(cp, cpIndex);
		CVMClassBlock* arrayCb =
		    CVMclassGetArrayOfWithNoClassCreation(con->ee, elmentCb);
		if (arrayCb == NULL) {
		    goto not_quickened;
		}
	    }
	    break;
	}

	case opc_athrow: {
	    if (!hasBranches) {
		/* Don't want any of these. */
		CVMtraceJITInliningExec(
		    reason = "unconditionally throwing an exception";)
		goto not_inlinable;
	    }
	    break;
	}

        case opc_jsr:
        case opc_jsr_w:
        case opc_ret: {
	    CVMtraceJITInliningExec(reason = "has jsr/ret opcode");
            goto not_inlinable;
        }

#ifdef CVM_HW
#include "include/hw/jitir1.i"
	    break;
#endif

	default: break; /* The rest are not an obstacle to inlining */
	}
	pc += instrLength;
    }

    /*
     * calculate number of locals we'll need to allocate. If too many,
     * reject inlining.
     */
    numLocals = CVMjmdMaxLocals(jmd);
    if (con->mc->currLocalWords + numLocals > con->maxLocalWords) {
	if (alwaysInline) {
	    /* We can't allow this method to not get inlined, so we must
	     * fail this compilation and retry with a max inlining depth
	     * one less.
	     */
	    CVMJITlimitExceeded(con, "Exceeded max allowed locals.");
	}
	CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M "
			     "exceeds maximum number of locals\n",
			     CVMmbClassBlock(targetMb), targetMb));
	return InlineNotInlinable; 
    }

    CVMtraceJITInliningExec({
	CVMconsolePrintf("INLINABLE CALL (%s parameters)\n\t\t",
			 storesParameters ? "writable" : "read-only");
	CVMconsolePrintf("%C.%M  -->  %C.%M\n",
			 CVMmbClassBlock(con->mc->mb), con->mc->mb,
			 CVMmbClassBlock(targetMb), targetMb);
	/*CVMbcListWithIndentation(targetMb, 1, CVM_TRUE);*/
    });

    if (stores) {
	status |= InlineStoresLocals;
	if (storesParameters) {
	    status |= InlineStoresParameters;
	}
    }
    if (hasBranches) {
	status |= InlineHasBranches;
    }
    if (callsOut) {
	status |= InlineCallsOut;
    }

    return status;

    /* ... Otherwise we would have bailed out by now */
 not_inlinable:

    CVM_COMPILEFLAGS_LOCK(con->ee);
    CVMmbCompileFlags(targetMb) |= CVMJIT_NOT_INLINABLE;
    CVM_COMPILEFLAGS_UNLOCK(con->ee);

    CVMtraceJITInlining(("NOT INLINABLE: Target method %C.%M is %s\n",
			 CVMmbClassBlock(targetMb), targetMb, reason));
#if 0
    /* Print the backtrace for the method that can't be inlined.
       CVMJITdebugMethodIsToBeCompiled is used to filter which
       non-inlinable method to trace, but other calls to it need
       to be disabled first, or almost nothing is compiled.
    */
    if (CVMJITdebugMethodIsToBeCompiled(con->ee, targetMb)) {
	CVMJITMethodContext* mc = con->mc;
	CVMconsolePrintf("%C.%M not Inlineable:\n",
			 CVMmbClassBlock(targetMb), targetMb);
	do {
	    CVMconsolePrintf("\t%C.%M\n", CVMmbClassBlock(mc->mb), mc->mb);
	    mc = mc->caller;
	} while (mc != NULL);
    }
#endif
    return InlineNotInlinable;
}

/*
 * "Extend" origExpr with a null check for objRef, if needed.
 */
static CVMJITIRNode*
sequenceWithNullCheck(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk,
		      CVMJITIRNode* objRef,
		      CVMJITIRNode* origExpr)
{
    CVMJITIRNode* nullCheckedNode;
    /* The lhs of the parameter list is the object to null check */
    nullCheckedNode = nullCheck(con, curbk, objRef, CVM_TRUE);
    if (nullCheckedNode != objRef) {
	/* We did emit a null check. Create a sequence with the
	   null check on the left hand side as an effect. */
	CVMJITIRNode* seq;
	/* SEQUENCE will be tagged with origExpr's tag */
	CVMUint8 origTypeTag = CVMJITgetTypeTag(origExpr);
	seq = nullCheckedNode;
        CVMJITirnodeSetHasUndefinedSideEffect(seq);
	seq = CVMJITirnodeNewBinaryOp(con, 
				      CVMJIT_ENCODE_SEQUENCE_R(origTypeTag),
				      seq, origExpr);
	return seq;
    } else {
	return origExpr;
    }
}

#ifdef CVMJIT_INTRINSICS
/*
 * Compare known mb to a short list of those whose semantics are
 * so well defined that they can be encoded as intrinsic methods.
 */
static CVMBool
doKnownMBAsIntrinsic(
    CVMJITCompilationContext* con,
    CVMJITIRBlock*	      curbk,
    CVMMethodBlock*	      targetMb,
    CVMBool                   doNullCheck,
    CVMBool                   doCheckInit)
{
    CVMJITIRNode *intrinsicNode;
    CVMJITIntrinsic *irec;
    const CVMJITIntrinsicConfig *config;
    CVMJITIRNode *mbNode;
    CVMUint8 rtnType;
    CVMUint16 argSize;
    CVMJITIRNode *plist;
    CVMJITMethodContext* mc;
    CVMUint32 oldIndex, newIndex, argsCount;

    irec = CVMJITintrinsicGetIntrinsicRecord(con->ee, targetMb);
    if (irec == NULL) {
        return CVM_FALSE;
    }

    CVMtraceJITIROPT(("JO: Intrinsic %C.%M inlined in %C.%M\n",
                      CVMmbClassBlock(targetMb), targetMb,
                      CVMmbClassBlock(con->mb), con->mb));
    mc = con->mc;
    config = irec->config;
    if (config->irnodeFlags & CVMJITIRNODE_THROWS_EXCEPTIONS) {
        /* Connect flow and flush all locals before possibly throwing an
           exception in the invocation code coming up below: */
        connectFlowToExcHandlers(con, mc, mc->startPC-mc->code, CVM_TRUE);
    }

    mbNode = CVMJITirnodeNewConstantMB(con, targetMb);
    rtnType = CVMtypeidGetReturnType(CVMmbNameAndTypeID(targetMb));
    argSize = CVMmbArgsSize(targetMb);

    /* Collect the method parameters: */
    oldIndex = CVMJITstackCnt(con, con->operandStack);
    if ((config->properties & CVMJITINTRINSIC_JAVA_ARGS) != 0) {
        plist = collectParameters(con, curbk, argSize);
    } else {
        plist = collectIArgs(con, curbk, argSize, irec);
    }
    newIndex = CVMJITstackCnt(con, con->operandStack);
    argsCount = oldIndex - newIndex;

    /* CheckInits only apply to static methods and NULL checks only apply
       to non-static methods.  The 2 should be mutually exclusive.  Hence,
       it does not matter which order they are evaluated with respect to
       each other because they never cross each others path:
    */
    if (doNullCheck) {
        CVMJITIRNode* thisObj = CVMJITirnodeGetLeftSubtree(plist);
        CVMassert(!doCheckInit);
        mbNode = sequenceWithNullCheck(con, curbk, thisObj, mbNode);

    /* %comment l023 */ 
    } else {
        CVMClassBlock *targetCb = CVMmbClassBlock(targetMb);
        if (doCheckInit && checkInitNeeded(con, targetCb)) {
            /* If we need to do a CheckInit to see if the class is
               uninitialized, then wrap the mb in a CLASS_CHECKINIT node: */
            CVMassert(!doNullCheck);
            mbNode = doCheckInitOnCB(con, targetCb, curbk, mbNode);
        }
    }

    /* Make sure to kill references if necessary: */
    if ((config->properties & CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS) != 0) {
        killAllCachedReferences(con);
    }

    /* Build the intrinsic node: */
    rtnType = CVMJITfoldType(rtnType);
    intrinsicNode = CVMJITirnodeNewBinaryOp(con,
                        CVMJIT_ENCODE_INTRINSIC(rtnType), plist, mbNode);
    CVMJITirnodeGetBinaryOp(intrinsicNode)->data = irec->intrinsicID;
    intrinsicNode->flags |= config->irnodeFlags;

    /* If non-void return invoke, push result of invoke onto stack */
    if (rtnType != CVM_TYPEID_VOID) {
        CVMJITirnodeStackPush(con, intrinsicNode);
    } else { 
        /* Call CVMJITirDoSideEffectOperator() before inserting a root node:*/
        CVMJITirDoSideEffectOperator(con, curbk);
        /* void return invoke, treat it as unevaluated expression */
        CVMJITirnodeNewRoot(con, curbk, intrinsicNode); 
    }

    return CVM_TRUE;
}
#endif /* CVMJIT_INTRINSICS */
#ifdef CVMJIT_SIMPLE_SYNC_METHODS

/*
 * See if the specified method shold be re-mapped to its Simple
 * sSync version. Returns Simple Sync version if there is one. Otherwise
 * the specified mb is returned.
 */
static CVMMethodBlock*
lookupSimpleSyncMB(CVMJITCompilationContext* con, CVMMethodBlock* mb)
{
    int i;
    CVMJITGlobalState* jgs = &CVMglobals.jit;
#if 0
    CVMconsolePrintf("****CHECK: %C.%M\n", CVMmbClassBlock(mb), mb);
#endif

    for (i = 0; i < jgs->numSimpleSyncMBs; i++) {
	/*
	 * If this is a sync method that we need to remap to the 
	 * "Simple Sync" version, then return the Simple Sync version.
	 * However, don't do this if we are currently compiling the
	 * Simple Sync version, because we want the real version
	 * in this case.
	 */
	if (mb == jgs->simpleSyncMBs[i].originalMB &&
	    con->mc->mb != jgs->simpleSyncMBs[i].simpleSyncMB)
	{
#if 0
	    CVMconsolePrintf("****MATCH: %C.%M\n", CVMmbClassBlock(mb), mb);
#endif
	    return jgs->simpleSyncMBs[i].simpleSyncMB;
	}
    }
    return mb;
}

#endif /* CVMJIT_SIMPLE_SYNC_METHODS */
/*
 * Collect parameters and invoke a method with a known target methodblock
 */
static CVMBool
doInvokeKnownMB(CVMJITCompilationContext* con,
		CVMJITIRBlock* curbk,
		CVMMethodBlock* targetMb,
		CVMBool isVirtual,  /* true if virtual invoke treated as NV */
		CVMBool doNullCheck,
                CVMBool doCheckInit)
{
    CVMUint16 argSize = CVMmbArgsSize(targetMb);
    CVMJITMethodContext* mc = NULL;
    Inlinability	canInline;
    CVMMethodBlock*	newTargetMb = targetMb;

    /* If the method is an abstract method, let the interpreter handle it
       because it is probably rare: */
    if (CVMmbIs(targetMb, ABSTRACT)) {
        CVMJITerror(con, CANNOT_COMPILE,
            "invoking a known abstract method not supported");
    }

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
    /* remap to simple sync version if appropriate */
    newTargetMb = lookupSimpleSyncMB(con, targetMb);
#endif

    if (CVMJITinlines(NONVIRTUAL) &&
        (!CVMmbIs(newTargetMb, SYNCHRONIZED) || CVMJITinlines(NONVIRTUAL_SYNC))
        && (canInline = isInlinable(con, newTargetMb)) != InlineNotInlinable)
    {
	{
	    CVMJITMethodContext *mc0 = con->mc;
	    CVMUint8* retPc = mc0->startPC + 3;
	    CVMassert(CVMopcodeGetLength(mc0->startPC) == 3);
	    /* The current block resumes at 'retPc' */
	    CVMJITirblockSplit(con, curbk, retPc - mc0->code);
	    /* Update current context to "end sooner". */
	    mc0->endPC = retPc;
	    CVMassert(mc0->endPC <= mc0->codeEnd);
	}
	mc = pushMethodContext(con, newTargetMb, canInline, doNullCheck);

	{
	    /*
	     * If this method is inlineable at the maxAllowedInliningDepth,
	     * then mark so the threshold will be ignored in the
	     * future, and reset the invokeCost so it won't likely be
	     * compiled. This reduces the amount of code compiled
	     * without hurting performance.
	     */
	    CVMJITGlobalState* jgs = &CVMglobals.jit;
	    if (jgs->inliningThresholds[jgs->maxAllowedInliningDepth-1] >
		CVMmbInvokeCost(newTargetMb))
	    {
		CVMmbInvokeCostSet(newTargetMb, jgs->inliningThresholds[0]);
		CVMmbCompileFlags(newTargetMb) |= CVMJIT_IGNORE_THRESHOLD;
	    }
	}

	return CVM_TRUE; /* Inlined */

#ifdef CVMJIT_INTRINSICS
    } else if (doKnownMBAsIntrinsic(con, curbk, targetMb, doNullCheck,
                                    doCheckInit)) {
	/* Great! Done! */
	return CVM_FALSE; /* not Inlined */
#endif

    } else {
        CVMJITIRNode *mbNode = CVMJITirnodeNewConstantMB(con, targetMb);
	CVMUint8 rtnType =
	    CVMtypeidGetReturnType(CVMmbNameAndTypeID(targetMb));
	CVMJITIRNode* plist;
	CVMJITMethodContext* mc = con->mc;

        /* Connect flow and flush all locals before possibly throwing an
	   exception in the invocation code coming up below: */
	connectFlowToExcHandlers(con, mc, mc->startPC - mc->code, CVM_TRUE);

	/* Do the invocation */
	plist = collectParameters(con, curbk, argSize);

        /* CheckInits only apply to static methods and NULL checks only apply
           to non-static methods.  The 2 should be mutually exclusive.  Hence,
           it does not matter which order they are evaluated with respect to
           each other because they never cross each others path:
        */
	if (doNullCheck) {
	    CVMJITIRNode* thisObj = CVMJITirnodeGetLeftSubtree(plist);
            CVMassert(!doCheckInit);
	    mbNode = sequenceWithNullCheck(con, curbk, thisObj, mbNode);

        /* %comment l023 */ 
        } else {
            CVMClassBlock *targetCb = CVMmbClassBlock(targetMb);
            if (doCheckInit && checkInitNeeded(con, targetCb)) {
                /* If we need to do a CheckInit to see if the class is
                   uninitialized, then wrap the mb in a CLASS_CHECKINIT
                   node: */
                CVMassert(!doNullCheck);
                mbNode = doCheckInitOnCB(con, targetCb, curbk, mbNode);
            }
        }

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
        if (CVMglobals.jit.pmiEnabled) {
            con->numCallees++;
            CVMtraceJITPatchedInvokes((
                "PMI: callee(%d) allocated 0x%x %C.%M\n",
                con->numCallees,
                targetMb, CVMmbClassBlock(targetMb), targetMb));
        }
#endif
	invokeMethod(con, curbk, rtnType, plist, argSize, isVirtual, mbNode);
	return CVM_FALSE; /* Invoked */
    }
}

/*
 * Handle a virtual invocation.
 * If candidateMb != NULL, try inlining this call to be the body of
 * candidateMb
 */
static CVMBool
doInvokeVirtualOrInterface(CVMJITCompilationContext* con,
                           CVMJITIRBlock* curbk,
                           CVMUint32 mtIndex,
                           CVMUint32 argSize,
                           CVMUint8  rtnType,
                           CVMMethodBlock *prototypeMb,
                           CVMMethodBlock *candidateMb,
                           CVMBool isInvokeVirtual)
{
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    CVMJITIRNode* cbNode;
#endif    
/* IAI - 20 */
    CVMMethodBlock* newCandidateMb = candidateMb;
    CVMJITIRNode* mbNode;
    CVMJITIRNode* methodSpecNode;

    CVMBool inlinable = CVM_FALSE;
    Inlinability  canInline = InlineNotInlinable; /* init to please compiler */
#ifdef CVMJIT_INTRINSICS
    CVMJITIntrinsic *irec = NULL;
#endif

    CVMassert(prototypeMb != NULL);
    if (candidateMb != NULL) {
        /* For invokevirtual, if the prototype is the same as the candidateMb
           it could be because we're doing an invokevirtual on a final or
           private method or a final class.  If so, then this method can never
           be overridden anyway.  So, treat it as a direct invocation of a
           known nonvirtual Mb. */
        if (prototypeMb == candidateMb) {
            CVMClassBlock *cb = CVMmbClassBlock(candidateMb);
            /* We should never get here if this was an invokeinterface: */
            CVMassert(isInvokeVirtual);
            if (CVMmbIs(candidateMb, PRIVATE) ||
                CVMmbIs(candidateMb, FINAL) || CVMcbIs(cb, FINAL))
            {
                return doInvokeKnownMB(con, curbk, candidateMb,
				       CVM_FALSE /* not virtual */,
				       CVM_TRUE /* null check */,
				       CVM_FALSE /* do check init */);
            }
        }

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
	/* remap to simple sync version if appropriate */
	newCandidateMb = lookupSimpleSyncMB(con, candidateMb);
#endif

	if (CVMJITinlines(VIRTUAL) &&
	    (!CVMmbIs(newCandidateMb, SYNCHRONIZED) ||
	     CVMJITinlines(VIRTUAL_SYNC)))
	{
	    canInline = isInlinable(con, newCandidateMb);
	    inlinable = canInline != InlineNotInlinable;
	}
#ifdef CVMJIT_INTRINSICS
        if (!inlinable) {
            irec = CVMJITintrinsicGetIntrinsicRecord(con->ee, candidateMb);
        }
#endif
	
#if 0
	/* Print the backtrace for the method call.
	   CVMJITdebugMethodIsToBeCompiled is used to filter which
	   method to trace, but other calls to it need
	   to be disabled first, or almost nothing is compiled.
	*/
	if (CVMJITdebugMethodIsToBeCompiled(con->ee, prototypeMb)) {
	    CVMJITMethodContext* mc = con->mc;
	    CVMconsolePrintf("%C.%M %sInlineable:\n",
			     CVMmbClassBlock(candidateMb), candidateMb,
			     inlinable ? "" : "not ");
	    CVMconsolePrintf("%C.%M\n",
			     CVMmbClassBlock(prototypeMb), prototypeMb);
	    do {
		CVMconsolePrintf("\t%C.%M\n", CVMmbClassBlock(mc->mb), mc->mb);
		mc = mc->caller;
	    } while (mc != NULL);
	}
#endif
    }

    if (isInvokeVirtual) {
        /* Get the mb from the method table. This has to be done whether
           we have inlined or not. */
        methodSpecNode = CVMJITirnodeNewConstantMethodTableIndex(con, mtIndex);
    } else {
        /* Use imb to build Constant32 node */
        methodSpecNode = CVMJITirnodeNewConstantMB(con, prototypeMb);
    }

    if (!inlinable
#ifdef CVMJIT_INTRINSICS
        && (irec == NULL)
#endif
        ) {
	/* Not inlinable */
	CVMJITIRNode* objRefNode;
	CVMJITIRNode* plist;
	CVMJITMethodContext* mc = con->mc;

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
        if (CVMglobals.jit.pmiEnabled) {
            /* if the method is virtual... */
            if (isInvokeVirtual &&
                /* ...and it has not been overridden */
                (CVMmbCompileFlags(prototypeMb) & CVMJIT_IS_OVERRIDDEN) == 0 &&
                /* ...and it is not abstract */
                !CVMmbIs(prototypeMb, ABSTRACT) &&
                /* ...and it is not inlinable */
                (!CVMJITinlines(NONVIRTUAL) ||
                 (CVMmbIs(prototypeMb, SYNCHRONIZED) &&
                  !CVMJITinlines(NONVIRTUAL_SYNC)) ||
                 isInlinable(con, prototypeMb) == InlineNotInlinable))
            {
                /* ...then this translates to a known MB invocation in the back
                   end. There we will add this method to the patch table in
                   case it later gets overridden and we need patch the invoke
                   to do a true virtual invocation. */
                CVMBool invokeResult = doInvokeKnownMB(
                     con, curbk, prototypeMb, CVM_TRUE, CVM_TRUE, CVM_FALSE); 
                CVMassert(!invokeResult); /* Shouldn't be inlined! */
                return invokeResult;
            }
        }
#endif

        /* Connect flow and flush all locals before possibly throwing an
	   exception in the invocation code coming up below: */
	connectFlowToExcHandlers(con, mc, mc->startPC - mc->code, CVM_TRUE);

	plist = collectParameters(con, curbk, argSize);
	objRefNode = CVMJITirnodeGetLeftSubtree(plist);
        mbNode = objectInvocationTarget(con, curbk, isInvokeVirtual,
                                        objRefNode, methodSpecNode);
	invokeMethod(con, curbk, rtnType, plist, argSize,
		     CVM_FALSE, mbNode);
	return CVM_FALSE;
    } else {
        /* Inlinable virtual/interface */
	CVMJITIRNode* candidateNode;
	CVMJITIRNode* objRefNode;
	CVMJITIRBlock* nextBlock; /* The merge block */
	CVMJITIRBlock* invBlock;  /* The out of line invocation block */
	CVMBool mbCheck;
	CVMJITMethodContext* mc = con->mc;
        int nextBytecodeOffset = (isInvokeVirtual ? 3:5);
        CVMUint8* retPc = mc->startPC + nextBytecodeOffset;

        CVMassert(CVMopcodeGetLength(mc->startPC) == nextBytecodeOffset);

	/* Step 1. Split the current block at the virtual invocation */

	{
	    /* The current block resumes at 'retPc' */
	    nextBlock = CVMJITirblockSplit(con, curbk, retPc - mc->code);
	    /* Update current context to "end sooner". The continuation
	       after the inlined virtual will be in 'nextBlock'. */
	    mc->endPC = retPc;
	    CVMassert(mc->endPC <= mc->codeEnd);
	}
	
	/* Some sanity checks for the block split */
	CVMassert(CVMJITirblockGetNext(curbk) == nextBlock);

	/* Don't let it get merged away */
	CVMJITirblockAddFlags(nextBlock, CVMJITIRBLOCK_IS_BRANCH_TARGET);

	/* Avoid mb check if we have already done it */
	objRefNode = findThis(con, argSize);
	{
	    mbCheck = !(mc->isVirtualInline &&
		(canInline & InlineStoresParameters) == 0 &&
		CVMJITirnodeIsLocal_0Ref(con, objRefNode) &&
		mc->mb == candidateMb)
#ifdef CVMJIT_INTRINSICS
		|| (irec != NULL)
#endif
		;
	}

	if (mbCheck) {
	    /* Step 2. Create a block for the out of line invoke. This
	       block has no mapping to any Java PC's */
	    invBlock = CVMJITirblockCreate(con, retPc - mc->code -
	                                   nextBytecodeOffset);
	    CVMJITirlistAppend(con, &con->artBkList, invBlock);
	    /* This is an artifical block */
            CVMJITirblockAddFlags(invBlock,
                (isInvokeVirtual ? CVMJITIRBLOCK_NO_JAVA_MAPPING :
                 (CVMJITIRBLOCK_NO_JAVA_MAPPING | 
                  CVMJITIRBLOCK_IS_OOL_INVOKEINTERFACE)));
            invBlock->targetMb = prototypeMb;

	    /* Step 3. Emit code to fetch and check mb for the correctness
	     * of the guess, and branch conditionally to the out of line code
	     */

	    /* Compute the target mb at runtime. We don't want to pop the
	     * arguments from the stack -- just find 'this'.  
	     *
	     * We would like the top values on the stack to flow into the
	     * invocation block. Then we can pop them for the inlined version.
	     */
	    /* The following takes care of the NULL check as well */
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
            if (!isInvokeVirtual)
#endif
/* IAI - 20 */
	    {
                mbNode = objectInvocationTarget(con, curbk, isInvokeVirtual,
                                                objRefNode, methodSpecNode);
	        /* And our guess */
                candidateNode = CVMJITirnodeNewConstantMB(con, candidateMb);

                /* Propagate the result of the vtable/itable access to the
                   invocation node: */
	        CVMJITirnodeStackPush(con, mbNode);

	        /* Check for equality of this guy with the candidate mb.
	         * On inequality, go to the invocation block. The parameters
	         * on the top of the stack will flow into the invocation block
	         * due to this conditional branch */
	        {
		    CVMBool fallThru =
		        condBranchToTargetBlock(con, curbk,
					        CVMJIT_TYPEID_ADDRESS,
					        CVMJIT_NE, 
					        mbNode, candidateNode, 
					        invBlock, 0);
		    CVMassert(fallThru), (void)fallThru;
	        }

	        /* Get rid of mbNode. At this point, we are not doing a call.*/
	        CVMJITirnodeStackDiscardTop(con);
            }
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
            else {
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
#if 0
		/* Experimental code for supporting branching around the cb
		 * check for in inlined virtual invoke if the invokeMb is
		 * not overridden, with the intent of patching the branch
		 * with a nop if method is ever overridden. I never got
		 * around to making this work, but it probably is the right
		 * starting point so I'm leaving it in. Still needs the
		 * invokeMb override check to make sure the branch should
		 * be done, and of course the patching when the method is
		 * overridden. Also there is a problem with asserts for phi
		 * values on the stack.
		 */
		CVMBool fallThru =
		    gotoBranchToTargetBlock(con, curbk, nextBlock,
					    CVMJIT_GOTO, CVM_FALSE);
		CVMassert(!fallThru), (void)fallThru;
#endif
#endif
		cbNode = objectInvocationTargetInlinable(con, curbk, objRefNode);
		/* And our guess cb */    
		candidateNode = CVMJITirnodeNewConstantCB(
                                con, CVMmbClassBlock(candidateMb));

		CVMJITirnodeStackPush(con, cbNode);
		/* Check for equality of this guy with the candidate cb.
		 * On inequality, go to the invocation block. The parameters
		 * on the top of the stack will flow into the invocation block
	         * due to this conditional branch */	   
		{		
		    CVMBool fallThru =
			condBranchToTargetBlock(con, curbk,
				CVMJIT_TYPEID_ADDRESS,
				CVMJIT_NE,
				cbNode, candidateNode,
				invBlock, 0);
                    CVMassert(fallThru), (void)fallThru;     
		}	    

		/* Get rid of cbNode. At this point, we are not doing a call.*/
		CVMJITirnodeStackDiscardTop(con);

      	        invBlock->mtIndex = mtIndex;
	        invBlock->candidateMb =  candidateMb;
	    }

#endif
/* IAI - 20 */
	}

#ifdef CVMJIT_COUNT_VIRTUAL_INLINE_STATS
	/*
	 * Mark inlining info for stats gathering
	 */
	{
	    CVMJITIRNode* mbConst = CVMJITirnodeNewConstantMB(con,
		(((CVMUint32)prototypeMb)|0x1));
	    CVMJITIRNode* info = CVMJITirnodeNewUnaryOp(con,
		CVMJIT_ENCODE_OUTOFLINE_INVOKE, mbConst);
	    CVMJITirnodeSetHasUndefinedSideEffect(info);
	    CVMJITirnodeNewRoot(con, curbk, info);
	}
#endif

#ifdef CVMJIT_INTRINSICS
        if (inlinable) {
#else
        {
            CVMassert(inlinable);
#endif
            con->numVirtinlineBytes += CVMCPU_VIRTINLINE_SIZE;

            /* Recurse. Setting the return PC to retPc along with the
             * fudging of endPC allows the current block's translation to
             * be terminated at the virtual invocation 
             */
            mc = pushMethodContext(con, newCandidateMb, canInline, CVM_FALSE);

	    {
		/*
		 * If this method is inlineable at the maxAllowedInliningDepth,
		 * then mark it so the threshold will be ignored in the future,
		 * and reset the invokeCost so it won't likely be compiled.
		 * This reduces the amount of code compiled without hurting
		 * performance. It also results in better invokevirtual
		 * guesses if the method has an invokevirtual since it
		 * will continue to be interpreted.
		 */
		CVMJITGlobalState* jgs = &CVMglobals.jit;
		if (jgs->inliningThresholds[jgs->maxAllowedInliningDepth-1] >
		    CVMmbInvokeCost(candidateMb))
		{
		    CVMmbInvokeCostSet(candidateMb,
				       jgs->inliningThresholds[0]);
		    CVMmbCompileFlags(candidateMb) |= CVMJIT_IGNORE_THRESHOLD;
		}
	    }

            mc->isVirtualInline = CVM_TRUE;
            return CVM_TRUE; /* Inlined */

#ifdef CVMJIT_INTRINSICS
        } else {
            CVMassert(irec != NULL);
            /* If we did the mbCheck already, then the this pointer must not
               be NULL.  Hence, we can skip the Null check. */
            doKnownMBAsIntrinsic(con, curbk, candidateMb,
                                 !mbCheck /* null check */,
                                 CVM_FALSE /* do check init */);

            /* Even though we didn't do inlining strictly speaking, we are now
               at the end of the range we want to translate because we have
               split the block.  Hence, returning TRUE here is the correct
               thing to do because it tells translateRange() that we're done
               with this range, and allows it to return to translateBlock()
               indicating that we have a fall thru to the next block (which
               is also what we want). */
            return CVM_TRUE;
#endif /* CVMJIT_INTRINSICS */
        }
    }
}

/*
 * Invoke superclass method
 */
static CVMBool
doInvokeSuper(CVMJITCompilationContext* con, 
	      CVMJITIRBlock* curbk, 
	      CVMUint16 mtIndex)
{
    CVMClassBlock* supercb = CVMcbSuperclass(CVMmbClassBlock(con->mb));
    CVMMethodBlock* targetMb = CVMcbMethodTableSlot(supercb, mtIndex);
    return doInvokeKnownMB(con, curbk, targetMb,
			   CVM_FALSE, CVM_TRUE, CVM_FALSE);
}

static void
doRet(CVMJITCompilationContext* con, CVMJITIRBlock* curbk, CVMUint16 localNo)
{
    CVMJITMethodContext *mc = con->mc;
    CVMUint32 targetPC;
    CVMJITIRNode* node =
	getLocal(con, mc, localNo, CVMJIT_TYPEID_ADDRESS);
    CVMJITJsrRetEntry* entry;

    CVMassert(localNo < mc->localsSize);
    targetPC = CVMJITlocalStateGetPC(mc, localNo);

    /*
     * We don't support ret leaving any items on the stack. This will
     * never happen with javac code and we choose not to compile the
     * method if this ever does happen.
     */
    if (CVMJITstackCnt(con, con->operandStack) != 0) {
	CVMJITerror(con, CANNOT_COMPILE,
		    "finally block requiring phi merge at exit not supported");
    }
    
    node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_RET, node);
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    /* Need to flush locals before doing the RET because unlike the RETURN
       opcodes, we will continue to execute in this method after this.
       RET behaves like a branch without looking like one. */
    CVMJITirFlushOutBoundLocals(con, curbk, mc, CVM_TRUE);
    /* We marked all Jsr return targets in the first pass. Now
       connect the flow and merge the local states into the successors.*/
    for (entry = (CVMJITJsrRetEntry*)con->jsrRetTargetList.head; 
         entry != NULL; entry = CVMJITjsrRetEntryGetNext(entry)) {
      if (entry->jsrTargetPC == targetPC) {
          CVMJITirblockConnectFlow(con, curbk, entry->jsrRetBk);
      }
    }
    CVMJITirnodeNewRoot(con, curbk, node);
}

static void
doIinc(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
       CVMUint16 localNo, CVMInt32 value)
{
    CVMJITIRNode* localNodeForAdd;
    CVMJITIRNode* constNode;
    CVMJITIRNode* addNode;
    int           binaryOp;
    CVMJITMethodContext* mc;

    mc = con->mc;

    /* convert to a SUB if necessary for better codegen (no negative
     * immediates on some platforms).
     */
    if (value < 0) {
    	value = -value;
    	binaryOp = CVMJIT_SUB;
    } else {
	binaryOp = CVMJIT_ADD;
    }
    
    /* Create LOCAL nodes for the local we will increment */
    localNodeForAdd = getLocal(con, mc, localNo, CVM_TYPEID_INT);

    /* Create Constant node for the value to add */
    constNode =
	CVMJITirnodeNewConstantJavaNumeric32(con, value, CVM_TYPEID_INT);
    
    /* Create ADD node */
    addNode = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_IBINARY(binaryOp),
				      localNodeForAdd, constNode);
    CVMJITirnodeSetBinaryNodeFlag(addNode, CVMJITBINOP_ARITHMETIC);
    addNode = CVMJIToptimizeBinaryIntExpression(con, addNode);

    /* Store the result of the add into the local: */
    CVMJITirnodeStackPush(con, addNode);
    storeValueToLocal(con, curbk, localNo, CVM_TYPEID_INT);
}

/*
 * Is the store at this PC a reference store or a jsr return value store?
 */
static CVMBool
pcStore(CVMJITCompilationContext* con, CVMJITIRBlock* curbk, CVMUint16 pc)
{
    /* The first astore of a JSR handler is always a return PC store */
    return (pc == CVMJITirblockGetBlockPC(curbk)) && CVMJITirblockIsJSRTarget(curbk);
}

/*
 * Translate a range of byte-codes encapsulated by the incoming context 'mc'
 *
 * Returns 'fallThru'
 */

static CVMBool 
translateRange(CVMJITCompilationContext* con,
	       CVMJITIRBlock* curbk,
	       CVMJITMethodContext* mc);

#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
static void
dumpLocalInfoForBlocks(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* firstBlock = 
        (CVMJITIRBlock*)CVMJITirlistGetHead(&con->bkList);
    CVMJITIRBlock* block;
	
    block = firstBlock;
    CVMconsolePrintf("Dumping local refs info for method %C.%M\n",
		     CVMmbClassBlock(con->mb), con->mb);
    while (block != NULL) {
	CVMconsolePrintf("LOCALS INFO AT HEAD OF BLOCK %d, pc=%d: ",
			 CVMJITirblockGetBlockID(block),
			 CVMJITirblockGetBlockPC(block));
	CVMJITlocalrefsDumpRefs(con, &block->localRefs);
	CVMconsolePrintf("\n");
        block = CVMJITirblockGetNext(block);
    }
}
#endif

static void
pushRange(CVMJITCompilationContext* con,
    CVMJITIRBlock* curbk, CVMJITIRRange* r)
{
    if (curbk->firstRange == NULL) {
	curbk->firstRange = r;
    } else {
	curbk->lastOpenRange->next = r;
    }
    r->prev = curbk->lastOpenRange;
    curbk->lastOpenRange = r;
}

static void
pushNewRange(CVMJITCompilationContext* con,
    CVMJITIRBlock* curbk, CVMUint16 pc)
{
    if (curbk->lastOpenRange == NULL ||
	curbk->lastOpenRange->mc != con->mc)
    {
	CVMJITIRRange *r = CVMJITirrangeCreate(con, pc);
	pushRange(con, curbk, r);
    }
}

/* Fall through to the next block. */
static void
doFallThru(CVMJITCompilationContext* con, CVMJITIRBlock* curbk)
{
    CVMJITIRBlock* nextBlock = CVMJITirblockGetNext(curbk);
    CVMJITMethodContext *mc = con->mc;
    CVMJITMethodContext *mc1 = nextBlock->inMc;

    CVMassert((mc->compilationDepth - nextBlock->inDepth) <= 1);
    CVMassert(mc == mc1 || mc->caller == mc1);

    mc = mc1;
    mc->currBlock = curbk;
    con->mc = mc;

    CVMJITirblockAtBranch(con, curbk, nextBlock,
			  NULL, NULL, NULL,
			  CVM_TRUE);

#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * Add target block to list of blocks we flow incoming locals from.
     *
     * We must do this after calling CVMJITirblockAtBranch so ASSIGN
     * nodes have been flushed.
     */
    addIncomingLocalsSuccessorBlock(con, curbk, nextBlock, CVM_TRUE);
#endif

    /* Mark the fall thru for subsequent passes over the blocks. */
    CVMJITirblockAddFlags(curbk, CVMJITIRBLOCK_FALLS_THRU);
}

/*
 * The main function to translate a block
 */
static void
translateBlock(CVMJITCompilationContext* con, CVMJITIRBlock* curbk)
{
    CVMJITMethodContext* mc = curbk->inMc;
    CVMBool fallThru = CVM_FALSE;
    CVMBool   done = CVM_FALSE;
    CVMJITIRBlock* mergeBlock = curbk;

    {
	int numIncomingLocals =
	    mc->firstLocal + mc->localsSize;

	curbk->numInitialLocals = numIncomingLocals;
	curbk->initialLocals =
	    CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
		numIncomingLocals * sizeof(CVMJITIRNode*));
    }

#ifdef CVM_JIT_REGISTER_LOCALS
    /* Don't allow incoming locals for exception handlers and JSR returns. */
    if (CVMJITirblockIsExcHandler(curbk) ||
	CVMJITirblockIsJSRReturnTarget(curbk))
    {
	CVMJITirblockNoIncomingLocalsAllowed(curbk);
    }
#endif

    if (!CVMJITirblockIsExcHandler(curbk)) {
	CVMJITirblockAtLabelEntry(con, curbk);
    }

#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
    CVMconsolePrintf("TRANSLATING BLOCK ID:%d @ PC=%d\n",
        CVMJITirblockGetBlockID(curbk), CVMJITirblockGetBlockPC(curbk));
    dumpLocalInfoForBlocks(con);
#endif

    if (CVMJITirblockIsBackwardBranchTarget(curbk) ||
	CVMJITirblockIsJSRReturnTarget(curbk))
    {
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	con->gcCheckPcsSize++;
#endif
	if (CVMJITirblockIsBackwardBranchTarget(curbk)) {
	    CVMmbCompileFlags(con->mb) |= CVMJIT_HAS_BACKBRANCH;
	}
    }

    /* Initialize local copy of ref-typed locals to the one in the block. */
    CVMassert(CVMJITsetInitialized(&curbk->localRefs));
    CVMJITlocalrefsCopy(con, &con->curRefLocals, &curbk->localRefs);

    /* Initialize remembered null checks */
    nullCheckInitialize(con);
    
    /* Initialize remembered bounds checks */
    boundsCheckInitialize(con);
    
    /* Initialized remembered FETCH(FIELD_REF(..)) nodes */
    getfieldCacheInitialize(con);

    /* Initialized remembered ARRAYLENGTH() nodes */
    arrayLengthCacheInitialize(con);

    /*
     * NOTE: We can only "recurse" while translating a block
     */

    /* reset fields */

    /* No translation done yet */
    CVMassert(curbk->inDepth == mc->compilationDepth);
    CVMassert(curbk->outDepth == curbk->inDepth); 
    CVMassert(curbk->lastOpenRange == NULL);

    CVMassert(mc->translateBlock != curbk);

    /*
     * Translate until there is nothing more to do
     */
    do {
	mc->currBlock = curbk;
	con->mc = mc;

	CVMJITirblockAddFlags(mergeBlock, CVMJITIRBLOCK_IS_TRANSLATED);

	/* Clear locals if not done already and mark the context for this
	   block */
	if (mc->translateBlock != curbk) {
	    memset(mc->locals, 0, 2 * mc->localsSize * (sizeof mc->locals[0]));
	    mc->translateBlock = curbk;
	}

	if (CVMJITirblockIsArtificial(mergeBlock)) {
	    if (curbk->targetMb == NULL) {
		/* Inline method prologue */
		pushNewRange(con, curbk, CVMJITirblockGetBlockPC(mergeBlock));
		CVMassert(mergeBlock != curbk);
		fallThru = translateInlinePrologue(con, curbk, mergeBlock);
		CVMassert(fallThru);
	    } else {
		/* Out-of-line invocation for inlined method */
		CVMassert(mergeBlock == curbk);
		fallThru = translateOutOfLineInvoke(con, mergeBlock);
		CVMassert(!fallThru);
	    }
	} else {

	    /* Block boundaries */

	    {
		CVMUint32 codeLength = CVMjmdCodeLength(mc->jmd);
		CVMUint32 blockLength = CVMJITirblockMaxPC(mergeBlock, codeLength);
		CVMassert(blockLength <= codeLength);

		/*
		 * Override the ones in the method context. 
		 */
		mc->startPC = mc->code + CVMJITirblockGetBlockPC(mergeBlock);
		mc->endPC = mc->code + blockLength;
	    }
	    CVMassert(mc->endPC <= mc->codeEnd);


	    curbk->outDepth = mc->compilationDepth;

	    fallThru = translateRange(con, curbk, mc);
	    curbk->lastOpenRange->endPC = mc->startPC - mc->code;
	}

	done = CVM_TRUE;

	/*
	 * If we are falling through to a block that is not a
	 * branch target, then merge it into the current block.
	 * This could happen because of aggressive splitting
	 * due to inlining.
	 */
	if (fallThru) {
	    CVMJITIRBlock* nextBlock = CVMJITirblockGetNext(curbk);
	    if (nextBlock != NULL && (!CVMJITirblockIsBranchTarget(nextBlock)))
	    {
		CVMJITMethodContext *mc1 = nextBlock->inMc;

		if (!CVMJITirblockIsArtificial(nextBlock)) {
		    CVMUint16 startPC  = CVMJITirblockGetBlockPC(nextBlock);
		    CVMassert(mc1->pcToBlock[startPC] == nextBlock);
		    mc1->pcToBlock[startPC] = NULL;
		}

		CVMassert((CVMJITirblockGetflags(nextBlock) &
		    CVMJITIRBLOCK_PENDING_TRANSLATION) == 0);
		CVMassert((CVMJITirblockGetflags(nextBlock) &
		    CVMJITIRBLOCK_IS_TRANSLATED) == 0);

		/* May need CVMJITirblockMerge() if we care about
		   other information in nextBlock */
		CVMJITirlistRemove(con, &con->bkList, nextBlock);

		mergeBlock = nextBlock;

		if (mc1 != mc) {
		    CVMassert(mc->compilationDepth != mc1->compilationDepth);
		    CVMassert(mc1->caller == mc || mc->caller == mc1);
		    mc = mc1;
		}

		done = CVM_FALSE;
	    }
	}

    } while (!done);

    CVMassert(mc->startPC == mc->endPC || mc->abortTranslation);

    /* Falling through to next block? */
    if (fallThru) {
	doFallThru(con, curbk);
    }
    
#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * We are done accumulating ASSIGN nodes in con->assignNodes.
     * Allocate bk->assignNodes[] and copy the ASSIGN nodes accumulated
     * in the context.
     */
    {
	size_t size = con->assignNodesCount * sizeof(CVMJITIRNode*);
	curbk->assignNodesCount = con->assignNodesCount;
	con->assignNodesCount = 0;
	curbk->assignNodes = CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER, size);
	memcpy(curbk->assignNodes, con->assignNodes, size);
#ifdef CVM_DEBUG
	memset(con->assignNodes, 0, size);
#endif
    }
#endif /* CVM_JIT_REGISTER_LOCALS */
}

/* Add END_INLINING node if we are the last block of an inlined method */
static void
checkEndInlining(CVMJITCompilationContext* con,
		 CVMJITIRBlock* curbk,
		 CVMJITMethodContext* mc)
{
    if (con->mc->compilationDepth > 0 && CVMJITirblockIsLastJavaBlock(curbk)) {
	CVMUint8 rtnType;

	/* We ended the inlining here. */

	rtnType = CVMtypeidGetReturnType(CVMmbNameAndTypeID(mc->mb));

	if (rtnType == CVM_TYPEID_VOID) {
	    endInlining(con, curbk, NULL);
	} else {
	    CVMJITIRNode* resultNode = CVMJITirnodeStackPop(con);
	    if (CVMJITirnodeHasSideEffects(resultNode)) {
		resultNode = endInlining(con, curbk, resultNode);
	    } else {
		endInlining(con, curbk, NULL);
	    }
	    CVMJITirnodeStackPush(con, resultNode);
	}

	CVMassert(mc->retBlock == NULL || mc->retBlock == curbk);

	if (CVMmbIs(mc->mb, SYNCHRONIZED)) {
	    CVMassert(!CVMmbIs(mc->mb, STATIC));
	    {
		CVMJITIRNode *node = monitorExit(con, curbk,
		    getLocal(con, mc, mc->syncObject,
			CVM_TYPEID_OBJ));
		if (rtnType == CVM_TYPEID_VOID) {
		    CVMJITirnodeNewRoot(con, curbk, node);
		} else {
		    CVMJITIRNode* eiNode = CVMJITirnodeStackPop(con);
		    eiNode = CVMJITirnodeNewBinaryOp(con,
			CVMJIT_ENCODE_SEQUENCE_L(CVMJITgetTypeTag(eiNode)),
			eiNode, node);
		    CVMJITirnodeStackPush(con, eiNode);
		}
	    }
	}
    }
}

/*
 * Must grab the cpIndex and compare the opcode atomically
 * in case it is has been quickened.
 */
#undef  FETCH_CPINDEX_ATOMIC
#ifndef CVM_CLASSLOADING
#define FETCH_CPINDEX_ATOMIC(cpIndex_)		\
    cpIndex_ = CVMgetUint16(absPc+1);
#else
#define FETCH_CPINDEX_ATOMIC(cpIndex_)		\
{						\
    CVM_CODE_LOCK(ee);				\
    cpIndex_ = CVMgetUint16(absPc+1);		\
    if (opcode != *absPc) {			\
	/* it got quickened on us. */		\
	CVM_CODE_UNLOCK(ee);			\
	opcode = *absPc;			\
	goto dispatch_opcode;			\
    }						\
    CVM_CODE_UNLOCK(ee);			\
}
#endif /* CVM_CLASSLOADING */

static CVMBool
methodMatchesVirtualMb(CVMExecEnv *ee, CVMMethodBlock *hintMb,
                       CVMMethodBlock *prototypeMb)
{
    CVMClassBlock *prototypeCb;
    CVMClassBlock *hintCb = CVMmbClassBlock(hintMb);
    CVMUint16 mtIndex = CVMmbMethodTableIndex(prototypeMb);
    prototypeCb = CVMmbClassBlock(prototypeMb);
    if (CVMextendsClass(ee, hintCb, prototypeCb) &&
        (hintMb == CVMcbMethodTableSlot(hintCb, mtIndex))) {
        return CVM_TRUE;
    }
    return CVM_FALSE;
}

static CVMBool
methodMatchesInterfaceMb(CVMExecEnv *ee, CVMMethodBlock *hintMb,
                         CVMMethodBlock *imb)
{
    CVMClassBlock *hintCb; /* cb for method we plan to call/inline. */
    CVMClassBlock *icb;    /* interface cb for specified interface method. */

    CVMUint32       interfaceCount;
    CVMInterfaces*  interfaces;
    CVMInterfaceTable* itablePtr;
    int guess;
    CVMUint16       methodTableIndex;/* index into hintCb's methodTable */

    hintCb = CVMmbClassBlock(hintMb);
    icb = CVMmbClassBlock(imb);
    interfaces = CVMcbInterfaces(hintCb);
    interfaceCount = CVMcbInterfaceCountGivenInterfaces(interfaces);
    itablePtr = CVMcbInterfaceItable(interfaces);

    guess = interfaceCount - 1;

    while ((guess >= 0) && 
           (CVMcbInterfacecbGivenItable(itablePtr,guess) != icb)) {
        guess--;
    }
    if (guess < 0) {
        /* Looks like this hint is incompatible with this interface call.
           So, we don't have a match: */
        return CVM_FALSE;
#ifdef CVM_DEBUG_ASSERTS
    } else {
        CVMassert(CVMcbInterfacecbGivenItable(itablePtr, guess) == icb);
#endif
    }

    /* We know CVMcbInterfacecb(hintCb, guess) is for the interface that
     * we are invoking.  Use it to convert the index of the method in 
     * the interface's methods array to the index of the method 
     * in the class' methodTable array.
     */
    methodTableIndex = CVMcbInterfaceMethodTableIndexGivenInterfaces(
                           interfaces, guess, CVMmbMethodSlotIndex(imb));
    if (hintMb != CVMcbMethodTableSlot(hintCb, methodTableIndex)) {
        return CVM_FALSE;
    }

    return CVM_TRUE;
}

static CVMBool
methodMatchesVirtualOrInterfaceMb(CVMExecEnv *ee, CVMMethodBlock *hintMb,
                                  CVMMethodBlock *prototypeMb,
                                  CVMBool isInvokeVirtual)
{
    if (isInvokeVirtual) {
        return methodMatchesVirtualMb(ee, hintMb, prototypeMb);
    }
    return methodMatchesInterfaceMb(ee, hintMb, prototypeMb);
}

/*
 * Supports building a TABLE_SWITCH node for both opc_tableswitch and
 * opc_lookupswitch when there is a run of consecutive entries.
 */
static void
initializeTableSwitch(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk,
		      CVMJITMethodContext* mc,
		      CVMUint16 pc,
		      CVMInt32 low, CVMInt32 high, CVMInt32* lpc,
		      CVMJITIRBlock* defaultbk,
		      CVMJITIRNode* keyNode, CVMBool isLookupSwitch)
{
    int cnt;
    CVMJITIRNode* node = 
	CVMJITirnodeNewTableSwitchOp(con, low, high,
				     keyNode, defaultbk);
    CVMJITTableSwitch* ts = CVMJITirnodeGetTableSwitchOp(node);
    
    /* Enqueue defaultbk block
       Force evaluation of items on the stack, but no pop */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
    if (CVMJITirblockIsBackwardBranchTarget(defaultbk)) {
	con->gcCheckPcsSize++;
    }
#endif
    CVMJITirblockAtBranch(con, curbk, defaultbk,
			  keyNode, NULL, node, CVM_FALSE);
#ifdef CVM_JIT_REGISTER_LOCALS
    /* No incoming locals allowed at tableswitch targets */
    CVMJITirblockNoIncomingLocalsAllowed(defaultbk);
#endif
    
    /* Fill up jump offset table list */
    for (cnt = 0; cnt < high - low + 1; cnt++) {
	int lpcIndex = (isLookupSwitch ? cnt*2 + 1 : cnt);
	CVMJITIRBlock* targetbk =
	    mc->pcToBlock[CVMgetAlignedInt32(&lpc[lpcIndex]) + pc];
	ts->tableList[cnt] = targetbk;
	
	/* Enqueue target block
	   Force evaluation on items on the stack, but no pop */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	if (CVMJITirblockIsBackwardBranchTarget(targetbk)) {
	    con->gcCheckPcsSize++;
	}
#endif
	CVMJITirblockAtBranch(con, curbk, targetbk,
			      NULL, NULL, NULL, CVM_FALSE);
#ifdef CVM_JIT_REGISTER_LOCALS
	/* No incoming locals allowed at tableswitch targets */
	CVMJITirblockNoIncomingLocalsAllowed(targetbk);
#endif
    }
}

/*
 * Translate a range of byte-codes encapsulated by the incoming context 'mc'
 */
static CVMBool
translateRange(CVMJITCompilationContext* con,
	       CVMJITIRBlock* curbk,
	       CVMJITMethodContext* mc)
{
    CVMExecEnv* ee = con->ee;

    /* Context variables below */
    CVMMethodBlock* mb = mc->mb;
    CVMClassBlock* cb = mc->cb;
    CVMConstantPool* cp = mc->cp;
    CVMBool hasExceptionHandlers = mc->hasExceptionHandlers;

    /* Keep track of both absolute and relative PC's */
    CVMUint8* absPc = mc->startPC;
    CVMUint16 pc    = absPc - mc->code;
    CVMUint8* endPc = mc->endPC;

    CVMOpcode      opcode = 0; /* Keep compiler happy */
    CVMUint16      instrLength;
    CVMBool	   fallThru;
    CVMBool        isInvokeVirtual;

    /* If we are compiling an inlining target, we'd better not have
       exception handlers */
    CVMassert(CVMjmdExceptionTableLength(mc->jmd) == 0 ||
	con->mc->compilationDepth == 0);

    /* By default, this block falls through to the next one, unless
       it is terminated by 'athrow', 'return' or something like that */
    fallThru = CVM_TRUE;

    mc->abortTranslation = CVM_FALSE;

    pushNewRange(con, curbk, mc->startPC - mc->code);

    if (con->mc->compilationDepth == 0) {
	if (CVMJITirblockIsExcHandler(curbk)) {
	    if (pc == CVMJITirblockGetBlockPC(curbk)) {
		/* Force a MAP_PC node before the EXCEPTION_OBJECT node that
		 * CVMJITirblockIsExcHandler will create. */
		createMapPcNodeIfNeeded(con, curbk,
					mc->startPC - mc->code, CVM_TRUE);
		CVMJITirblockAtHandlerEntry(con, curbk);
	    }
	}
    }

    while (absPc < endPc && !mc->abortTranslation) {
	CVMassert(fallThru);

	/* Create CVMJITMapPcNode for this pc if necessary */
	createMapPcNodeIfNeeded(con, curbk, pc, CVM_FALSE);
	
	opcode = *absPc;
        instrLength = CVMopcodeGetLength(absPc);

	if (hasExceptionHandlers) {
	    /* If this opcode can throw an exception, propagate our
	       local state to all exception handlers */
	    if (CVMbcAttr(opcode, THROWSEXCEPTION) &&
                !CVMbcAttr(opcode, INVOCATION))
	    {
		connectFlowToExcHandlers(con, mc, pc, CVM_TRUE);
            }
        }

#ifdef CVM_CLASSLOADING
    dispatch_opcode:
#endif
        switch (opcode) {

	case opc_nop:
	    break;

	/* Push const onto the operand stack */
 	case opc_aconst_null: {
            /* NOTE: A NULL aconst might as well be a NULL String Object.
                     Since there is no type info associated with a NULL
                     object, there is no difference.   Just reuse the
                     CVMJITirnodeNewConstantStringObject() encoder. */
            CVMJITIRNode* node = 
                CVMJITirnodeNewConstantStringObject(con, NULL);
       	    CVMJITirnodeStackPush(con, node);
	    break;
	}

  	case opc_iconst_m1:
	case opc_iconst_0:
  	case opc_iconst_1:
	case opc_iconst_2:
	case opc_iconst_3:
 	case opc_iconst_4:
	case opc_iconst_5: {
	    pushConstantJavaInt(con, CVMJITOpcodeMap[opcode][JITMAP_CONST]);
	    break;
	}
	case opc_fconst_0:
	    pushConstantJavaFloat(con, 0.0);
	    break;
	case opc_fconst_1:
	    pushConstantJavaFloat(con, 1.0);
	    break;
	case opc_fconst_2:
	    pushConstantJavaFloat(con, 2.0);
	    break;
	case opc_dconst_0: {
	    CVMJavaVal64 v64;
	    v64.d = CVMdoubleConstZero();
	    pushConstantJavaVal64(con, &v64, CVM_TYPEID_DOUBLE);
	    break;
	}
	case opc_dconst_1: {
	    CVMJavaVal64 v64;
	    v64.d = CVMdoubleConstOne();
	    pushConstantJavaVal64(con, &v64, CVM_TYPEID_DOUBLE);
	    break;
	}
	case opc_lconst_0: {
	    CVMJavaVal64 v64;
	    v64.l = CVMlongConstZero();
	    pushConstantJavaVal64(con, &v64, CVM_TYPEID_LONG);
	    break;
	}
	case opc_lconst_1: {
	    CVMJavaVal64 v64;
	    v64.l = CVMlongConstOne();
	    pushConstantJavaVal64(con, &v64, CVM_TYPEID_LONG);
	    break;
	}

        /* Pop stack item and store into local variable */
        case opc_istore_0:
        case opc_fstore_0:
        case opc_istore_1:
        case opc_fstore_1:
        case opc_istore_2:
        case opc_fstore_2:
        case opc_istore_3:
        case opc_fstore_3: {
	    CVMUint32 localNo = CVMJITOpcodeMap[opcode][JITMAP_CONST];
	    
	    /* single-word, non-reference stores */
	    storeValueToLocal(con, curbk, localNo,
			      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
	    break;
        }

        case opc_lstore_0:
        case opc_dstore_0:
        case opc_lstore_1:
        case opc_dstore_1:
        case opc_lstore_2:
        case opc_dstore_2:
        case opc_lstore_3:
        case opc_dstore_3: {
	    CVMUint32 localNo = CVMJITOpcodeMap[opcode][JITMAP_CONST];
	    
	    /* double-word, non-reference stores */
	    storeValueToLocal(con, curbk, localNo,
			      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
	    break;
        }

        case opc_astore_0:
        case opc_astore_1:
        case opc_astore_2:
        case opc_astore_3: {
	    CVMUint32 localNo = CVMJITOpcodeMap[opcode][JITMAP_CONST];
	    
	    if (!pcStore(con, curbk, pc)) {
		/* reference stores */
		storeValueToLocal(con, curbk, localNo, CVM_TYPEID_OBJ);
	    } else {
		/* PC stores */
		storePCValueToLocal(con, curbk, localNo, CVM_TYPEID_INT, pc);
	    }
	    
	    break;
        }

        case opc_astore: {
	    CVMUint32 localNo = absPc[1];
	    
	    if (!pcStore(con, curbk, pc)) {
		/* reference stores */
		storeValueToLocal(con, curbk, localNo, CVM_TYPEID_OBJ);
	    } else {
		/* PC stores */
		storePCValueToLocal(con, curbk, localNo, CVM_TYPEID_INT, pc);
	    }
	    break;
	}
	
        case opc_istore:
        case opc_fstore: {
	    CVMUint32 localNo = absPc[1];
	    
	    /* single-word, non-reference stores */
	    storeValueToLocal(con, curbk, localNo,
			      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]); 
	    break;
	}

        case opc_dstore:
        case opc_lstore: {
	    CVMUint32 localNo = absPc[1];
	    
	    /* double-word, non-reference stores */
	    storeValueToLocal(con, curbk, localNo,
			      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]); 
	    break;
	}

	/* Unconditional jump with 16-bit or 32-bit jump offset */
        case opc_goto_w:
        case opc_goto: {
    	    CVMUint16 targetPC = pc + (opcode == opc_goto_w 
				       ? CVMgetInt32(absPc+1)
				       : CVMgetInt16(absPc+1));

	    if (!gotoBranch(con, curbk, targetPC, CVMJIT_GOTO)) {
		fallThru = CVM_FALSE;
	    }
	    break;
 	}

	/* jump subroutine to subroutine */
	case opc_jsr_w:
	case opc_jsr: {
	    /* calculating the return address is handled by the backend */
   	    CVMUint16 targetPC = pc + (opcode == opc_jsr_w 
				       ? CVMgetInt32(absPc+1)
				       : CVMgetInt16(absPc+1));

            /*
             * We don't support finally blocks that require any items to be
             * on the stack. This will never happen with javac code and we
             * choose not to compile the method if this ever does happen.
             * We should have already rejected such methods during stackmap
             * disambiguation.  Just assert here:
             */
            CVMassert(CVMJITstackIsEmpty(con, con->operandStack));
	    
	    if (!gotoBranch(con, curbk, targetPC, CVMJIT_JSR)) {
		fallThru = CVM_FALSE;
	    } else {
		CVMJITerror(con, CANNOT_COMPILE,
                            "jsr target is next block");
	    }
	    break;
	}

	/* Load value from local variable */
        case opc_aload_0:
        case opc_iload_0:
        case opc_fload_0:
        case opc_dload_0:
        case opc_lload_0:
        case opc_aload_1:
        case opc_iload_1:
        case opc_fload_1:
        case opc_dload_1:
        case opc_lload_1:
        case opc_aload_2:
        case opc_iload_2:
        case opc_fload_2:
        case opc_dload_2:
        case opc_lload_2:
        case opc_aload_3:
        case opc_iload_3:
        case opc_fload_3:
        case opc_dload_3:
        case opc_lload_3:
	    pushLocal(con, mc,
		      CVMJITOpcodeMap[opcode][JITMAP_CONST], 
		      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
	    break;

	case opc_aload:
	case opc_iload:
        case opc_fload:
	case opc_dload:
	case opc_lload: {
   	    pushLocal(con, mc,
		      absPc[1], CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
	    break;
	}


	/* Load value from array */
	case opc_iaload:
	case opc_aaload:
	case opc_baload:
	case opc_caload:
	case opc_saload:
	case opc_faload:
            /* Make extra room for the array load code: */
            con->numLargeOpcodeInstructionBytes += CVMCPU_WORD_ARRAY_LOAD_SIZE;
            goto doArrayLoadOpcode;

	case opc_laload:
	case opc_daload:
            /* Make extra room for the array load code: */
            con->numLargeOpcodeInstructionBytes +=
		CVMCPU_DWORD_ARRAY_LOAD_SIZE;
        doArrayLoadOpcode:
	    doArrayLoad(con, curbk, CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
	    break;

	/* Store value into array */
        case opc_dastore:
        case opc_lastore:
            /* Make extra room for the array load code: */
            con->numLargeOpcodeInstructionBytes +=
                CVMCPU_DWORD_ARRAY_STORE_SIZE;
            goto doArrayStoreOpcode;

        case opc_iastore:
        case opc_aastore:
        case opc_fastore:
        case opc_bastore:
        case opc_castore:
        case opc_sastore:
            /* Make extra room for the array load code: */
            con->numLargeOpcodeInstructionBytes +=
                CVMCPU_DWORD_ARRAY_STORE_SIZE;

        doArrayStoreOpcode: {
	    CVMUint8 arrayType = CVMJITOpcodeMap[opcode][JITMAP_TYPEID];
    	    CVMJITIRNode* valueNode;
	    CVMInt32 arrTableID;
            CVMJITIRNode* indexOperationNode;
            CVMJITIRNode** indexOperationNodePtr;
            CVMJITIRNode* assignNode;

	    /*
	     * Reference write
	     */
	    if (opcode == opc_aastore) {
		/* Reference write. Keep track of write-barriers */
		con->numBarrierInstructionBytes += CVMCPU_GC_BARRIER_SIZE;
	    }

            /* The CVMJITirDoSideEffectOperator() needs to come first to
               preserve correct evaluation order */
            CVMJITirDoSideEffectOperator(con, curbk);

    	    valueNode = CVMJITirnodeStackPop(con);
	    arrTableID = indexArrayOperation(con, curbk, CVM_TRUE, arrayType);
	    indexOperationNodePtr = indexExpressionSlot(con, arrTableID);
	    indexOperationNode = *indexOperationNodePtr;
	    
	    /* Index created for a write */
	    CVMJITirnodeSetBinaryNodeFlag(
                CVMJITirnodeValueOf(indexOperationNode), 
		CVMJITBINOP_WRITE);
 	    assignNode = CVMJITirnodeNewBinaryOp(con, 
                             CVMJIT_ENCODE_ASSIGN(arrayType),
                             indexOperationNode, valueNode);

            CVMJITirnodeNewRoot(con, curbk, assignNode);

	    /* Conservatively cancel all cached array fetch
	     * expressions to guard against aliasing related problems.  
	     */
	    boundsCheckKillFetchExprs(con);
	    
 	    break;
 	}

	/* Binary integer operations */
        case opc_iadd:
        case opc_iand:
        case opc_ior:
        case opc_ixor:
        case opc_imul: {
                /* Make sure that if one the operands is a constant, then it
                   should appear in the rhs.  Swapping the lhs and rhs
                   operands does not break evaluation semantics if the
                   operator is commutative and at least one of the operands
                   is constant.
                */
                CVMJITStack *operandStack = con->operandStack;
                CVMUint32 idx = operandStack->todoIdx - 1;
                CVMJITIRNode *lhsNode =
                                (CVMJITIRNode *)operandStack->todo[idx - 1];
                CVMJITIRNode *rhsNode ;
                if (CVMJITnodeTagIs(lhsNode, CONSTANT)) {
                    /* Swap the top 2 operands on the operand stack: */
                    operandStack->todo[idx - 1] = operandStack->todo[idx];
                    operandStack->todo[idx] = lhsNode;
		    lhsNode = (CVMJITIRNode *)operandStack->todo[idx - 1];
                }

#if 1
		/* Convert:
		 *    byteArray[i] & 0xff
		 * to:
		 *    byteArray[i]
		 * but with an INDEX node with a type of
		 * UBYTE instead of BYTE. This requires cloning the
		 * FETCH and INDEX nodes, setting the data field of the
		 * cloned INDEX node to type UBYTE. The BOUNDS_CHECK node
		 * if present will be re-used.
		 */
		rhsNode = (CVMJITIRNode *)operandStack->todo[idx];
                if (opcode == opc_iand &&
		    CVMJITnodeTagIs(rhsNode, CONSTANT) &&
		    CVMJITirnodeGetConstant32(rhsNode)->j.i == 0x000000ff &&
		    CVMJITirnodeIsFetch(lhsNode) &&
		    CVMJITirnodeIsIndex(CVMJITirnodeGetLeftSubtree(lhsNode)) &&
		    CVMJITirnodeGetBinaryOp(
			CVMJITirnodeGetLeftSubtree(lhsNode))->data ==
		    CVM_TYPEID_BYTE)
		{
		    CVMJITIRNode* fetchNode;
		    CVMJITIRNode* indexNode =
			CVMJITirnodeGetLeftSubtree(lhsNode);

		    /* clone INDEX node with new type of UBYTE */
		    indexNode = CVMJITirnodeNewBinaryOp(con,
			CVMJIT_ENCODE_INDEX,
			CVMJITirnodeGetLeftSubtree(indexNode),
			CVMJITirnodeGetRightSubtree(indexNode));
		    CVMJITirnodeGetBinaryOp(indexNode)->data =
			CVMJIT_TYPEID_UBYTE;
		    /* create new FETCH node using new INDEX node */
		    fetchNode = CVMJITirnodeNewUnaryOp(con,
                        CVMJIT_ENCODE_FETCH(CVM_TYPEID_INT), indexNode);
		    /* dispose of the 0x000000ff node */
		    (void)CVMJITirnodeStackPop(con);
		    /* replace old FETCH node with the new UBYTE FETCH node. */
		    operandStack->todo[idx - 1] = fetchNode;
		    break;
		}
#endif

                binaryStackOp(con, 
                              CVMJITOpcodeMap[opcode][JITMAP_OPC], 
                              CVMJITOpcodeMap[opcode][JITMAP_TYPEID],
                              CVMJITBINOP_ARITHMETIC);
                {
                    CVMJITIRNode *node = CVMJITirnodeStackPop(con);
                    node = CVMJIToptimizeBinaryIntExpression(con, node);
                    CVMJITirnodeStackPush(con, node);
                }
            }
            break;

        case opc_idiv:
        case opc_irem:
        case opc_ldiv:
        case opc_lrem:
            /* These can throw a DivideByZeroException.  Hence, they have a
               side effect: */
            binaryStackOp(con,
                          CVMJITOpcodeMap[opcode][JITMAP_OPC],
                          CVMJITOpcodeMap[opcode][JITMAP_TYPEID],
                          CVMJITBINOP_ARITHMETIC);
            /* Mark the div/rem node as having a side effect: */
            CVMJITirnodeSetThrowsExceptions(CVMJITirnodeStackGetTop(con));
            break;

	case opc_isub:

	case opc_ladd:
	case opc_lsub:
	case opc_lmul:
	case opc_land:
	case opc_lor:
	case opc_lxor:

	/* Binary floating number operations */
	case opc_fadd:
	case opc_fsub:
	case opc_fdiv:
	case opc_frem:
	case opc_fmul:
	case opc_dadd:
	case opc_dsub:
	case opc_ddiv:
	case opc_drem:
	case opc_dmul:

	/* Shift operations */
	case opc_ishl:
	case opc_ishr:
	case opc_iushr:
	case opc_lshl:
	case opc_lshr:
	case opc_lushr:
	    binaryStackOp(con, 
			  CVMJITOpcodeMap[opcode][JITMAP_OPC], 
			  CVMJITOpcodeMap[opcode][JITMAP_TYPEID],
			  CVMJITBINOP_ARITHMETIC);
	    break;

       /* Increment local variable by constant
	  The whole expression is built in the execution-order list after
	  checking the operandStack for expressions that involves the
	  local variable. The involved local variable gets evaluated before
	  the iinc does. */
        case opc_iinc: {
	    CVMUint8 localNo = absPc[1]; 
	    CVMInt32 value = (CVMInt8)(absPc[2]);

	    doIinc(con, curbk, localNo, value);
	    break;
       }

        case opc_arraylength: {
	    /* get arrayref */ 
	    CVMJITIRNode *node = CVMJITirnodeStackPop(con);
            CVMJITIRNode *cachedArrayLengthNode;
            cachedArrayLengthNode = arrayLengthCacheGet(con, node);
            if (cachedArrayLengthNode == NULL) {
                CVMJITIRNode *arrayNode = node;
#ifndef CVMJIT_TRAP_BASED_NULL_CHECKS
                /* Generate the null check node only if trap-based
                   null checks are not present. Otherwise ARRAY_LENGTH
                   will do the job */
                node = nullCheck(con, curbk, node, CVM_TRUE);
#else
                node = nullCheck(con, curbk, node, CVM_FALSE);
#endif
                node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_ARRAY_LENGTH,
                                              node);
                /* Finally cache: Key (objrefNode), value arrayLengthNode */
                arrayLengthCachePut(con, arrayNode, node);

            } else {
                node = cachedArrayLengthNode;
            }

	    CVMJITirnodeStackPush(con, node);
	    break;
        }

	/* Branch if long, float, double comparison succeeds */
        case opc_fcmpl:
        case opc_fcmpg:
        case opc_dcmpl:
        case opc_dcmpg:
        case opc_lcmp: 
	{
	    /* If we have a cmp followed by a conditional branch, which 
	     * will normally always be the case, then we can merge the
	     * two opcodes into one.
	     */
	    CVMUint8 opcodeTypeTag = CVMJITOpcodeMap[opcode][JITMAP_TYPEID];
	    CVMOpcode nextOpcode = absPc[instrLength];
            CVMUint8 flags = 0;
            if ((opcode == opc_fcmpl) || (opcode == opc_dcmpl)) {
                flags = CVMJITCMPOP_UNORDERED_LT;
            }
	    switch (nextOpcode) {
	    case opc_iflt:
	    case opc_ifgt:
	    case opc_ifle:
	    case opc_ifge:
	    case opc_ifeq:
	    case opc_ifne: {
                CVMJITIRNode *rhs = CVMJITirnodeStackPop(con);
                CVMJITIRNode *lhs = CVMJITirnodeStackPop(con);
		/* Combine two opcodes and emit conditional branch node */
		CVMJITCondition condition =
		    CVMJITOpcodeMap[nextOpcode][JITMAP_OPC];

          	pc += instrLength;
		absPc += instrLength;
                instrLength = CVMopcodeGetLength(absPc);
                if (!condBranch(con, curbk, opcodeTypeTag, condition,
				rhs, lhs, pc, flags))
		{
		    CVMassert(mc->abortTranslation);
		    fallThru = CVM_FALSE;
		}
		break;
	    }
	    default: {
		/* The next opcode is not conditional branch so just do a
		   binary op for the cmp opcode. */
                binaryStackOp(con, CVMJITOpcodeMap[opcode][JITMAP_OPC], 
                              CVM_TYPEID_INT, flags);
		break;
	    }
	    }
            break;
	}

	/* Branch if integer comparison succeeds */
        case opc_if_icmpeq:
        case opc_if_icmpne:
        case opc_if_icmplt:
        case opc_if_icmple:
        case opc_if_icmpgt:
        case opc_if_icmpge:
	/* Branch if reference comparison succeeds */
	case opc_if_acmpeq:
	case opc_if_acmpne: {
            CVMJITIRNode *rhs = CVMJITirnodeStackPop(con);
            CVMJITIRNode *lhs = CVMJITirnodeStackPop(con);
	    CVMJITCondition condition =
		(CVMJITCondition)CVMJITOpcodeMap[opcode][JITMAP_OPC];
	    CVMUint16 typeTag = CVMJITOpcodeMap[opcode][JITMAP_TYPEID];
            if (!condBranch(con, curbk, typeTag, condition, rhs, lhs, pc, 0)) {
		CVMassert(mc->abortTranslation);
		fallThru = CVM_FALSE;
	    }
	    break;
	}
	

	/* Branch if reference is null or not null */
	case opc_ifnull:
	case opc_ifnonnull: {
	    CVMJITCondition condition =
		(CVMJITCondition)CVMJITOpcodeMap[opcode][JITMAP_OPC];
            CVMJITIRNode* node =
		CVMJITirnodeNewConstantStringObject(con, NULL);
            if (!condBranch(con, curbk, CVM_TYPEID_OBJ, condition, node,
			    CVMJITirnodeStackPop(con), pc, 0))
	    {
		CVMassert(mc->abortTranslation);
		fallThru = CVM_FALSE;
	    }
            break;
	}

	case opc_nonnull_quick: {  /* pop stack, and error if it is null */
	    CVMJITIRNode* node = CVMJITirnodeStackPop(con);
	    CVMJITIRNode* nullCheckedNode = nullCheck(con, curbk, node,
						      CVM_TRUE);

	    /* Null check on popped node */ 
	    if (nullCheckedNode != node) {
                CVMJITirDoSideEffectOperator(con, curbk);
		CVMassert(CVMJITirnodeHasSideEffects(nullCheckedNode));
		CVMJITirForceEvaluation(con, curbk, nullCheckedNode);
	    }
	    
	    break;
	}

	/* Branch if integer comparison with zero succeeds */
	case opc_ifeq: {
	    CVMBool contains;

	    CVMJITsetContains(&mc->notSeq, pc, contains);
            if (contains) {
                /* If we get here, then we have found a NOT sequence: */
                unaryStackOp(con, CVMJIT_NOT, CVM_TYPEID_INT,
                             CVMJITUNOP_ARITHMETIC);
                {
                    CVMJITIRNode *node = CVMJITirnodeStackPop(con);
                    node = CVMJIToptimizeUnaryIntExpression(con, node);
                    CVMJITirnodeStackPush(con, node);
                }

                /* Treat the whole sequence as one big opcode: */
                instrLength = CVMJITOPT_SIZE_OF_NOT_SEQUENCE;
                break;
            }
            /* Else, fall thru to the case below: */
	}

	case opc_ifne:
	case opc_iflt:
	case opc_ifle:
	case opc_ifgt:
	case opc_ifge: {
	    CVMJITIRNode* node = CVMJITirnodeStackPop(con); 
	    CVMJITCondition condition =
		(CVMJITCondition)CVMJITOpcodeMap[opcode][JITMAP_OPC];
	    CVMJITIRNode* zeroNode;

	    zeroNode = CVMJITirnodeNewConstantJavaNumeric32( 
		con, 0, CVMJIT_ENCODE_CONST_JAVA_NUMERIC32(CVM_TYPEID_INT));
            if (!condBranch(con, curbk, CVM_TYPEID_INT, condition,
			    zeroNode, node, pc, 0))
	    {
		CVMassert(mc->abortTranslation);
		fallThru = CVM_FALSE;
	    }
	    break;
   	}

	/* Return from subroutine */
	case opc_ret: {
	    CVMUint8 localNo = absPc[1];
            doRet(con, curbk, localNo);
	    fallThru = CVM_FALSE;
            con->hasRet = CVM_TRUE;
	    break;
	}

	/* Return void from method */
        case opc_return: {
            CVMJITStack *operandStack = con->operandStack;

            /* If there still are operands on the stack, then we need to call
               the CVMJITirDoSideEffectOperator() here to force evaluation of
               all those operands before we return because they may have side
               effects: */
            if (mc->startStackIndex != CVMJITstackCnt(con, operandStack)) {
                CVMJITirDoSideEffectOperator(con, curbk);
                /* Also adjust the stack accordingly: */
                CVMJITstackSetCount(con, operandStack, mc->startStackIndex);
            }

	    /* Ignore returns for inlined targets */
	    if (con->mc->compilationDepth == 0) {
                CVMJITIRNode *node;
                node = CVMJITirnodeNewNull(con, CVMJIT_ENCODE_RETURN);
		CVMJITirnodeNewRoot(con, curbk, node);
		fallThru = CVM_FALSE;
	    } else {
		/*
		 * Jump to special return block, unless it
		 * immediately follows.
		 */
		if (absPc + 1 != mc->codeEnd) {
		    if (!gotoBranch(con, curbk, mc->codeEnd - mc->code,
				    CVMJIT_GOTO))
		    {
			fallThru = CVM_FALSE;
		    } else {
			CVMassert(CVM_FALSE);
		    }
		}
	    }
	    CVMassert(absPc + 1 == endPc);
	    
	    break;
        }

	/* Return value from method */
        case opc_ireturn:
        case opc_areturn:
        case opc_freturn:
        case opc_lreturn:
        case opc_dreturn: {
            CVMJITStack *operandStack = con->operandStack;
            CVMJITIRNode *resultNode = CVMJITirnodeStackPop(con);

            /* NOTE: We can pop the resultNode off first because it is the
               last thing to be evaluated.  If the current method context is
               not for an inlined method, a RETURN node will be emitted.  The
               RETURN node will cause this resultNode to be evaluate.
               On the other hand, if the current method context is for an
               inlined method, than the resultNode can simply be pushed back
               onto the stack.  Either way, the resultNode will still be the
               last one evaluated.  Hence, we did not violate any evaluation
               order by not including the resultNode in the
               CVMJITirDoSideEffectOperator.
            */

            /* If there still are operands on the stack, then we need to call
               the CVMJITirDoSideEffectOperator() here to force evaluation of
               all those operands before we return because they may have side
               effects: */
            if (mc->startStackIndex != CVMJITstackCnt(con, operandStack)) {
                CVMJITirDoSideEffectOperator(con, curbk);
                /* Also adjust the stack accordingly: */
                CVMJITstackSetCount(con, operandStack, mc->startStackIndex);
            }

	    /* Ignore returns for inlined targets */
	    /* The return value must be on the stack anyway */
	    if (con->mc->compilationDepth == 0) {
                CVMUint16 tag;
                tag = CVMJIT_ENCODE_RETURN_VALUE(
                          CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
                resultNode = CVMJITirnodeNewUnaryOp(con, tag, resultNode);
                CVMJITirnodeNewRoot(con, curbk, resultNode);
		fallThru = CVM_FALSE;
            } else {
		CVMJITirnodeStackPush(con, resultNode);
		/*
		 * Jump to special return block, unless it
		 * immediately follows.
		 */
		if (absPc + 1 != mc->codeEnd) {
		    if (!gotoBranch(con, curbk, mc->codeEnd - mc->code,
			CVMJIT_GOTO))
		    {
			fallThru = CVM_FALSE;
		    } else {
			CVMassert(CVM_FALSE);
		    }
		}
            }
	    CVMassert(absPc + 1 == endPc);
	    break;
        }

	/* Push byte or short */
	case opc_bipush: {
            CVMInt32 byteValue = (CVMInt8)(absPc[1]);
            pushConstantJavaInt(con, byteValue);
	    break;
	}
	case opc_sipush: {
	    CVMInt16 shortValue = CVMgetInt16(absPc+1);
            pushConstantJavaInt(con, shortValue);
	    break;
	}

	/* Pop the top operand stack value */
	case opc_pop:
            doPop(con, curbk, 1);
	    break;

	/* Pop the top one or two operand stack values */
	case opc_pop2: {
            doPop(con, curbk, 2);
            break;
        }

 	/* Dup and swap family
	 * Some dup opcodes checks the type of stack items in
 	 * category-II (64-bit): long and double types, and
	 * category-I (32-bit): other types.
	 */

	/* Swap topmost two items. Both stack items are category-I type. */
	case opc_swap: {
            CVMJITIRNode *tosNode;
            CVMJITIRNode *nextNode;

            /* Need to preserve evaluation order of everything on the stack up
               till now before we change the order of operands on the stack: */
            CVMJITirDoSideEffectOperator(con, curbk);

            tosNode = CVMJITirnodeStackPop(con);
            nextNode = CVMJITirnodeStackPop(con);
	    CVMJITirnodeStackPush(con, tosNode);
	    CVMJITirnodeStackPush(con, nextNode);
	    break;
	}

	/* Duplicate the top operand stack value of category-I type */
	case opc_dup: {
	    CVMJITIRNode* tosNode = CVMJITirnodeStackGetTop(con);
            /* NOTE: The CVMJITirDoSideEffectOperator() is not needed here
               because the evaluation order of items on the stack hasn't
               changed. */
            CVMJITirnodeStackPush(con, tosNode);
	    break;
	}

	/* Duplicate the top operand stack value and insert two values 
	 * down. Both stack items are category-I type. */
	case opc_dup_x1: {
            CVMJITIRNode *tosNode;
            CVMJITIRNode *nextNode;

            /* Need to preserve evaluation order of everything on the stack up
               till now before we change the order of operands on the stack: */
            CVMJITirDoSideEffectOperator(con, curbk);

            tosNode = CVMJITirnodeStackPop(con);
            nextNode = CVMJITirnodeStackPop(con);
            CVMJITirnodeStackPush(con, tosNode);
            CVMJITirnodeStackPush(con, nextNode);
            CVMJITirnodeStackPush(con, tosNode);
	    break;
	}

	/* Duplicate the top operand stack value and insert two values
	   or three values down */
	case opc_dup_x2: {
            CVMJITIRNode *tosNode;
            CVMJITIRNode *nextNode;
            CVMBool is64;

            /* Need to preserve evaluation order of everything on the stack up
               till now before we change the order of operands on the stack: */
            CVMJITirDoSideEffectOperator(con, curbk);

            tosNode = CVMJITirnodeStackPop(con);
            nextNode = CVMJITirnodeStackPop(con);
            is64 = CVMJITirnodeIsDoubleWordType(nextNode);

    	    /* Form 2: II,I */ 
	    if (is64) { /* nextNode is 64-bit node */
	        CVMJITirnodeStackPush(con, tosNode);
	        CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
	    } else {
	    /* Form 1: I,I,I */ 
		CVMJITIRNode* nnNode = CVMJITirnodeStackPop(con);
	        CVMJITirnodeStackPush(con, tosNode);
	        CVMJITirnodeStackPush(con, nnNode);
	        CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
	    }
	    break;
	}

	/* Duplicate the top one or two operand stack values */
        case opc_dup2: {/* Duplicate the top 2 items on the stack */
            CVMJITIRNode* tosNode = CVMJITirnodeStackGetTop(con);
            CVMBool is64 = CVMJITirnodeIsDoubleWordType(tosNode);

            /* NOTE: The CVMJITirDoSideEffectOperator() is not needed here
               because the evaluation order of items on the stack hasn't
               changed. */
            if (is64) {
                /* Category-II type: nothing to do here */
            } else {
		/* Category-I type */
            	CVMJITIRNode* nextNode = CVMJITstackGetElementAtIdx(con,
		    con->operandStack, con->operandStack->todoIdx - 2);
                CVMJITirnodeStackPush(con, nextNode);
            }
	    CVMJITirnodeStackPush(con, tosNode);
            break;
	}

	/* Duplicate the top one or two operand stack values and 
	   insert two or three values down. */
        case opc_dup2_x1: {/* insert top double three down */
            CVMJITIRNode *tosNode;
            CVMJITIRNode *nextNode;
            CVMBool is64;

            /* Need to preserve evaluation order of everything on the stack up
               till now before we change the order of operands on the stack: */
            CVMJITirDoSideEffectOperator(con, curbk);

            tosNode = CVMJITirnodeStackPop(con);
            nextNode = CVMJITirnodeStackPop(con);
            is64 = CVMJITirnodeIsDoubleWordType(tosNode);

	    /* Form2: tosNode is a 64-bit and nextNode is 32-bit */
            if (is64) { 
                CVMJITirnodeStackPush(con, tosNode);
                CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
            } else {
	    /* Form1: tosNode, nextNode, and nnNode are 32-bit */
	 	CVMJITIRNode* nnNode = CVMJITirnodeStackPop(con);
                CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
                CVMJITirnodeStackPush(con, nnNode);
                CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
	    }
            break;
	}

	/* Duplicate the top one or two operand stack values and
	   insert two, three, or four values down. */
        case opc_dup2_x2: {/* insert top double four down */

            CVMJITIRNode *tosNode;
            CVMJITIRNode *nextNode;
            CVMBool isTos64;
            CVMBool isNext64;

            /* Need to preserve evaluation order of everything on the stack up
               till now before we change the order of operands on the stack: */
            CVMJITirDoSideEffectOperator(con, curbk);

            tosNode = CVMJITirnodeStackPop(con);
            nextNode = CVMJITirnodeStackPop(con);
            isTos64 = CVMJITirnodeIsDoubleWordType(tosNode);
            isNext64 = CVMJITirnodeIsDoubleWordType(nextNode);

	    /* topNode has 64-bit value */
	    if (isTos64) {
		if (isNext64) { /* Form 4: II,II */
                    CVMJITirnodeStackPush(con, tosNode);
		} else { /* Form 2: II,I,I */
		    CVMJITIRNode* nnNode = CVMJITirnodeStackPop(con);
                    CVMJITirnodeStackPush(con, tosNode);
                    CVMJITirnodeStackPush(con, nnNode);
		} 
		CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
	    } else { 
	    /* tosNode has 32-bit value */
		CVMJITIRNode* nnNode = CVMJITirnodeStackPop(con);
		/* Form 1: I,I,I,I */
		if (!CVMJITirnodeIsDoubleWordType(nnNode)) { /* Form 1 */
		    CVMJITIRNode* nnnNode = CVMJITirnodeStackPop(con);
                    CVMJITirnodeStackPush(con, nextNode);
                    CVMJITirnodeStackPush(con, tosNode);
                    CVMJITirnodeStackPush(con, nnnNode);
	 	} else { /* Form 3: I,I,II */
                    CVMJITirnodeStackPush(con, nextNode);
                    CVMJITirnodeStackPush(con, tosNode);
	 	}
		CVMJITirnodeStackPush(con, nnNode);
                CVMJITirnodeStackPush(con, nextNode);
                CVMJITirnodeStackPush(con, tosNode);
	    }
	    break;
	}

	/* Extend local variable index by additional bytes */
        case opc_wide: {
            CVMUint16 localIndex = CVMgetUint16(absPc+2);
            opcode = absPc[1];
            switch(opcode) {
                case opc_aload:
                case opc_iload:
                case opc_fload:
                case opc_lload:
                case opc_dload:
            	    pushLocal(con, mc, localIndex, 
			      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
            	    break;

                case opc_astore:
		    if (!pcStore(con, curbk, pc)) {
			/* reference stores */
			storeValueToLocal(con, curbk, localIndex,
					  CVM_TYPEID_OBJ);
		    } else {
			/* PC stores */
			storePCValueToLocal(con, curbk, localIndex,
					  CVM_TYPEID_INT, pc);
		    }
            	    break;

                case opc_istore:
                case opc_fstore:
            	    storeValueToLocal(con, curbk, localIndex,
		                      CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
            	    break;

                case opc_lstore:
                case opc_dstore:
            	    storeValueToLocal(con, curbk, localIndex,
			              CVMJITOpcodeMap[opcode][JITMAP_TYPEID]);
            	    break;

                case opc_iinc: {
		    CVMInt32 value = CVMgetInt16(absPc+4); 
            	    doIinc(con, curbk, localIndex, value);
            	    break;
	        }
                case opc_ret: {
                    doRet(con, curbk, localIndex);
		    fallThru = CVM_FALSE;
                    con->hasRet = CVM_TRUE;
		    break;
		}
                default:
                    CVMassert(CVM_FALSE);
            }/* end of switch */
	    break;
        }/* end of opc_wide */

#ifdef CVM_HW
#include "include/hw/jitir2.i"
	    break;
#endif

       /* negate the value on the top of the stack */
       /* Conversion operations */

        case opc_ineg: 
        case opc_fneg:
        case opc_lneg:
        case opc_dneg:
        case opc_i2f:   /* convert top of stack int to float */
        case opc_i2l:   /* convert top of stack int to long */
        case opc_i2d:   /* convert top of stack int to double */
        case opc_i2b:   /* convert top of stack int to byte */
        case opc_i2c:   /* convert top of stack int to char */
        case opc_i2s:   /* convert top of stack int to short */
        case opc_l2i:	/* convert top of stack long to int */
        case opc_l2f:   /* convert top of stack long to float */
        case opc_l2d:	/* convert top of stack long to double */
        case opc_f2i:   /* Convert top of stack float to int */
        case opc_f2l:   /* convert top of stack float to long */
        case opc_f2d:   /* convert top of stack float to double */
        case opc_d2i:   /* convert top of stack double to int */
        case opc_d2f:   /* convert top of stack double to float */
        case opc_d2l:   /* convert top of stack double to long */
	    unaryStackOp(con, 
			 CVMJITOpcodeMap[opcode][JITMAP_OPC], 
			 CVMJITOpcodeMap[opcode][JITMAP_TYPEID],
			 CVMJITUNOP_ARITHMETIC);
	    break;

	/* Goto pc at specified offset in switch table. */

	case opc_tableswitch: {
            CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(absPc+1);
            CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
            CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
	    CVMJITIRBlock* defaultbk =
		mc->pcToBlock[CVMgetAlignedInt32(&lpc[0]) + pc];
	    CVMJITIRNode* keyNode = CVMJITirnodeStackPop(con);
	    initializeTableSwitch(con, curbk, mc, pc,
				  low, high, &lpc[3],
				  defaultbk, keyNode, CVM_FALSE);

	    fallThru = CVM_FALSE;
	    break;
	}

        /* Goto pc whose table entry matches specified key */

	case opc_lookupswitch: {
	    CVMBool foundGap = CVM_FALSE;
	    CVMInt32 gapEntry = 0;
	    int cnt;
	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(absPc+1);
            CVMJITIRBlock* defaultbk =
		mc->pcToBlock[CVMgetAlignedInt32(lpc) + pc];
            CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
	    CVMJITSwitchList* sl; 
	    CVMJITIRNode* keyNode = CVMJITirnodeStackPop(con);

	    /*
	     * Check if this lookupswitch is basically a tableswitch, but
	     * with the first or last value not part being part of a sequence.
	     */
	    if (npairs > 4) {
		CVMInt32 prevMatchValue = CVMgetAlignedInt32(&lpc[2]);
		for (cnt = 1; cnt < npairs; cnt++) {
		    CVMInt32 matchValue = CVMgetAlignedInt32(&lpc[2+2*cnt]);
		    /* check if sequence is broken */
		    if (matchValue != prevMatchValue + 1) {
			if (foundGap) {
			    /* too many gaps */
			    foundGap = CVM_FALSE;
			    break;
			} else {
			    if (cnt == 1) {
				gapEntry = 0;
			    } else if (cnt == npairs - 1) {
				gapEntry = cnt;
			    } else {
				/* gap was is in middle of table */
				break;
			    }
			    foundGap = CVM_TRUE;
			}
		    }
		    prevMatchValue = matchValue;
		}
	    }

	    if (foundGap) {
		/* Do TableSwitch, but first do a bcond for the
		 * one case that will not be in the tableswitch.
		 */
		CVMInt32 lowEntry, highEntry;
		CVMInt32 low, high;
		CVMInt32 matchValue = CVMgetAlignedInt32(&lpc[2+2*gapEntry]);
		CVMInt32 pcOffset = CVMgetAlignedInt32(&lpc[3+2*gapEntry]);
		CVMJITIRBlock* targetbk = mc->pcToBlock[pc + pcOffset];
		/* Create Constant node for the value to add */
		CVMJITIRNode* constNode =
		    CVMJITirnodeNewConstantJavaNumeric32(con, matchValue,
							 CVM_TYPEID_INT);
		/* build BCOND node to compare key to matchvalue */ 
		CVMJITIRNode* bcondNode = 
		    CVMJITirnodeNewCondBranchOp(con, keyNode, constNode,
						CVM_TYPEID_INT, CVMJIT_EQ,
						targetbk, 0);
		/* Push target block onto block stack
		   connect control flow arc between curbk and target block
		   phiMerge is needed. The stack items are needed to finish up
		   translating the rest of opcodes in the extended basic
		   block */ 
		CVMJITirblockAtBranch(con, curbk, targetbk,
				      keyNode, constNode, bcondNode,
				      CVM_FALSE);
		/*
		 * Add target block to list of blocks we flow incoming locals
		 * from.
		 *
		 * We must do this after calling CVMJITirblockAtBranch so
		 * ASSIGN nodes have been flushed.
		 */
#ifdef CVM_JIT_REGISTER_LOCALS
		addIncomingLocalsSuccessorBlock(con, curbk, targetbk,
						CVM_FALSE);
#endif
		
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
		if (CVMJITirblockIsBackwardBranchTarget(targetbk)) {
		    con->gcCheckPcsSize++;
		}
#endif
		/* gapEntry should be set to first or last item */
		CVMassert(gapEntry == 0 || gapEntry == npairs - 1);
		if (gapEntry == 0) {
		    lowEntry = 1;
		    highEntry = npairs - 1;
		} else {
		    lowEntry = 0;
		    highEntry = gapEntry - 1;
		}
		low  = CVMgetAlignedInt32(&lpc[2+2*lowEntry]);
		high = CVMgetAlignedInt32(&lpc[2+2*highEntry]);
		initializeTableSwitch(con, curbk, mc, pc,
				      low, high, &lpc[2+2*lowEntry],
				      defaultbk, keyNode, CVM_TRUE);
	    } else {
		/* Do LookupSwitch */
		CVMJITIRNode* node = 
		    CVMJITirnodeNewLookupSwitchOp(con, defaultbk,
						  keyNode, npairs);

		/* Enqueue default block
		   Force evaluation of items on the stack, but don't pop */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
		if (CVMJITirblockIsBackwardBranchTarget(defaultbk)) {
		    con->gcCheckPcsSize++;
		}
#endif
		CVMJITirblockAtBranch(con, curbk, defaultbk,
				      keyNode, NULL, node, CVM_FALSE);
#ifdef CVM_JIT_REGISTER_LOCALS
		/* No incoming locals allowed at lookupswitch targets */
		CVMJITirblockNoIncomingLocalsAllowed(defaultbk);
#endif

		/* Fill up match-offset pairs table list */
		sl = CVMJITirnodeGetLookupSwitchOp(node)->lookupList;
		for (cnt = 0; cnt < npairs; cnt++) {
		    CVMJITIRBlock* targetbk;
		    lpc += 2;
		    targetbk = mc->pcToBlock[CVMgetAlignedInt32(&lpc[1]) + pc];
		    sl[cnt].matchValue = CVMgetAlignedInt32(&lpc[0]);
		    sl[cnt].dest = targetbk; 

		    /* Enqueue target block
		       Force evaluation on items on the stack, but no pop */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
		    if (CVMJITirblockIsBackwardBranchTarget(targetbk)) {
			con->gcCheckPcsSize++;
		    }
#endif
		    CVMJITirblockAtBranch(con, curbk, targetbk,
					  NULL, NULL, NULL, CVM_FALSE);
#ifdef CVM_JIT_REGISTER_LOCALS
		    /* No incoming locals allowed at lookupswitch targets */
		    CVMJITirblockNoIncomingLocalsAllowed(targetbk);
#endif
		}
	    }

	    fallThru = CVM_FALSE;
            break;
	}

	/* Throw exception */
	case opc_athrow: {
            CVMJITIRNode *node;
            /* Make sure everything on the stack is evaluated in the
               proper order first: */
            CVMJITirDoSideEffectOperator(con, curbk);
            node = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_ATHROW,
                                          CVMJITirnodeStackPop(con)); 
            CVMJITirnodeSetThrowsExceptions(node);
	    CVMJITirnodeNewRoot(con, curbk, node);
	    fallThru = CVM_FALSE;
	    break;
	} 

       /* monitorenter and monitorexit for locking/unlocking an object */
        case opc_monitorenter: {
            CVMJITIRNode *node =
		monitorEnter(con, curbk, CVMJITirnodeStackPop(con));
	    CVMJITirnodeNewRoot(con, curbk, node);
	    break;
	}   
	case opc_monitorexit: {
            CVMJITIRNode *node =
		monitorExit(con, curbk, CVMJITirnodeStackPop(con));
	    CVMJITirnodeNewRoot(con, curbk, node);
	    break;
	}

   	/*
 	 * Invoke a static method.
	 */

	/* Invoke a static method: Constant pool entry is resolved. */
	case opc_invokestatic_quick: 
        /* invoke a romized static method that has a static initializer */
        case opc_invokestatic_checkinit_quick:
	/* Invoke a static method: Need to check constant pool entry
	   is resolved or not */
	case opc_invokestatic: {
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
	    CVMMethodTypeID mid = CVM_TYPEID_ERROR;
	    CVMJITIRNode* plist;
	    CVMJITIRNode* targetNode;
	    CVMUint16     argSize;
	    CVMUint8      rtnType;

	    /* If cp index is resolved, do the same as 
	       opc_invokestatic_checkinit_quick. */
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, &mid)) {
                CVMMethodBlock* targetMb = CVMcpGetMb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
                /* Make sure that both the opcode and the mb agree on whether
                 * or not the method is static. */
                if (!CVMmbIs(targetMb, STATIC)) {
                    /* If not, we treat it as if it is unresolved and let the
                       compiler runtime take care of throwing an exception: */
		    mid = CVMmbNameAndTypeID(targetMb);
                    goto createResolveStaticMBNode;
                }
#endif
		CVMassert(fallThru);
		mc->startPC = absPc;
                if (doInvokeKnownMB(con, curbk, targetMb,
				    CVM_FALSE, CVM_FALSE, CVM_TRUE))
		{
		    /* Did inlining. Time to execute out of new context */
		    mc->startPC += instrLength;
		    return CVM_TRUE;
		}
	    } else {
#ifndef CVM_TRUSTED_CLASSLOADERS
            createResolveStaticMBNode:
#endif
                argSize = CVMtypeidGetArgsSize(mid);
                rtnType = CVMtypeidGetReturnType(mid);

		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

		/* Connect flow and flush all locals before possibly throwing
		   an exception in the invocation code coming up below: */
		connectFlowToExcHandlers(con, mc, pc, CVM_TRUE);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_STATIC_MB_UNRESOLVED and with the
                   CVMJITUNOP_CLASSINIT flag set.  Then, append it to the IR
                   root list. */
		targetNode = CVMJITirnodeNewResolveNode(
                    con, cpIndex, curbk,
		    CVMJITUNOP_CLASSINIT, CVMJIT_CONST_STATIC_MB_UNRESOLVED);
		plist = collectParameters(con, curbk, argSize);
		/* Build INVOKE node */ 
		invokeMethod(con, curbk, rtnType, plist, argSize,
			     CVM_FALSE, targetNode);
	    }
            break;
        }

        case opc_invokespecial: {
            CVMUint16 cpIndex;
	    CVMMethodTypeID mid = CVM_TYPEID_ERROR;
	    
	    FETCH_CPINDEX_ATOMIC(cpIndex);

            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, &mid)) {
	   	/* invokenonvirtual_quick or invokesuper_quick semantics */
                CVMMethodBlock* targetMb = CVMcpGetMb(cp, cpIndex);
                CVMMethodBlock* new_mb = targetMb;
                CVMClassBlock* currClass = CVMmbClassBlock(mb);
#ifndef CVM_TRUSTED_CLASSLOADERS
                /* Make sure that both the opcode and the mb agree on whether
                 * or not the method is static. */
                if (CVMmbIs(targetMb, STATIC)) {
                    /* If not, we treat it as if it is unresolved and let the
                       compiler runtime take care of throwing an exception: */
		    mid = CVMmbNameAndTypeID(targetMb);
                    goto createResolveSpecialMBNode;
                }
#endif
                if (CVMisSpecialSuperCall(currClass, targetMb)) {
		    CVMMethodTypeID methodID = CVMmbNameAndTypeID(targetMb);
		    /* Find matching declared method in a super class. */
		    new_mb = CVMlookupSpecialSuperMethod(ee, currClass, methodID);
                }
		CVMassert(fallThru);
		mc->startPC = absPc;
                if (targetMb == new_mb) {
		    if (doInvokeKnownMB(con, curbk, targetMb, 
					CVM_FALSE, CVM_TRUE, CVM_FALSE))
		    {
			/* Did inlining. Time to execute out of new context */
			mc->startPC += instrLength;
			return CVM_TRUE;
		    }
                } else {
                    if (doInvokeSuper(con, curbk,
                                      CVMmbMethodTableIndex(new_mb)))
		    {
			/* Did inlining. Time to execute out of new context */
			mc->startPC += instrLength;
			return CVM_TRUE;
		    }
                }
            } else {
                /* Compute args size, account for 'this' */
                CVMUint16 argSize;
                CVMUint8 rtnType;
                CVMJITIRNode* plist;
                CVMJITIRNode* targetNode;
                CVMJITIRNode* thisObj;

#ifndef CVM_TRUSTED_CLASSLOADERS
            createResolveSpecialMBNode:
#endif
                argSize = CVMtypeidGetArgsSize(mid) + 1;
                rtnType = CVMtypeidGetReturnType(mid);

		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

		/* Connect flow and flush all locals before possibly throwing
		   an exception in the invocation code coming up below: */
		connectFlowToExcHandlers(con, mc, pc, CVM_TRUE);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_SPECIAL_MB_UNRESOLVED and with the
                   CVMJITUNOP_CLASSINIT flag set.  Then, append it to the IR
                   root list. */
                targetNode = CVMJITirnodeNewResolveNode(con, cpIndex, curbk, 0,
                                CVMJIT_CONST_SPECIAL_MB_UNRESOLVED);
                plist = collectParameters(con, curbk, argSize);
                thisObj = CVMJITirnodeGetLeftSubtree(plist);
                targetNode = sequenceWithNullCheck(con, curbk, thisObj,
                                                   targetNode);
                /* Build INVOKE node */ 
                invokeMethod(con, curbk, rtnType, plist, argSize,
			     CVM_FALSE, targetNode);
            }
	    break;
	}

	/* entry pointed from opc_invokespecial */
   	case opc_invokenonvirtual_quick: { 
	    CVMUint16 cpIndex = CVMgetUint16(absPc+1);
            CVMMethodBlock* targetMb = CVMcpGetMb(cp, cpIndex);
	    CVMassert(fallThru);
	    mc->startPC = absPc;
            if (doInvokeKnownMB(con, curbk, targetMb,
				CVM_FALSE, CVM_TRUE, CVM_FALSE)) {
		/* Did inlining. Time to execute out of new context */
		mc->startPC += instrLength;
		return CVM_TRUE;
	    }
	    break;
        }

	case opc_invokesuper_quick: {
	    CVMUint16 mtIndex = CVMgetUint16(absPc+1);
	    CVMassert(fallThru);
	    mc->startPC = absPc;
	    if (doInvokeSuper(con, curbk, mtIndex)) {
		/* Did inlining. Time to execute out of new context */
		mc->startPC += instrLength;
		return CVM_TRUE;
	    }
            break;
	}


 	/* invoke a virtual method */
	/* Used to be #ifndef CVM_NO_LOSSY_OPCODES which means we have
	 * lossy opcodes.  However, quicken.c will not generate these if
	 * a JVMTI agent is connected (CVMjvmtiEnabled() == true)
	 * OR if CVM_JIT is defined
	 */
#ifndef CVM_JIT
        /* These quickened bytecodes are unused for JIT builds because they
           prevent virtual inlining.  This code is left here for reference
           purposes only: */

	/* Resolved cp entry
	   mb is lost in the lossy mode.
	   operand1 = CVMmbMethodTableIndex(mb): absPc[1] 
	   operand2 = CVMmbArgsSize(mb): absPc[2] */
	case opc_ainvokevirtual_quick:
	case opc_dinvokevirtual_quick:
	case opc_vinvokevirtual_quick:
	case opc_invokevirtual_quick:
	    CVMassert(fallThru);
	    mc->startPC = absPc;
            if (doInvokeVirtualOrInterface(con, curbk, absPc[1], absPc[2],
                        CVMJITOpcodeMap[opcode][JITMAP_TYPEID],
                        NULL, NULL, CVMJIT_VINVOKE))
	    {
		/* Did inlining. Time to execute out of new context */
		mc->startPC += instrLength;
		return CVM_TRUE;
	    }
            /* Make room for the CVMJIT_FETCH_MB_FROM_VTABLE code: */
            con->numLargeOpcodeInstructionBytes += CVMCPU_INVOKEVIRTUAL_SIZE;
	    break;

	case opc_invokevirtualobject_quick: {
	    const CVMClassBlock* objCB = CVMsystemClass(java_lang_Object);
	    CVMMethodBlock* targetMb = CVMcbMethodTableSlot(objCB, absPc[1]);
	    CVMUint8 rtnType;

	    rtnType = CVMtypeidGetReturnType(CVMmbNameAndTypeID(targetMb));
	    CVMassert(fallThru);
	    mc->startPC = absPc;
            if (doInvokeVirtualOrInterface(con, curbk, absPc[1], absPc[2],
                        rtnType, NULL, NULL, CVMJIT_VINVOKE))
	    {
		/* Did inlining. Time to execute out of new context */
		mc->startPC += instrLength;
		return CVM_TRUE;
	    }

            /* Make room for the CVMJIT_FETCH_MB_FROM_VTABLE code: */
            con->numLargeOpcodeInstructionBytes += CVMCPU_INVOKEVIRTUAL_SIZE;
            break;
	}
#endif /* CVM_JIT */

	/*
	 * Constant pool entry is resolved or resolvable at compile-time.
	 *
	 * cp entry index to interface mb imb) -> interface cb (icb) ->
	 * objref -> object cb (ocb) -> object's interface cb
	 * imb is used for method we are calling.
	 * icb is used for interface we are invoking.
	 * Search object's interface interface table until the entry whose
	 * interface cb matches the cb of the interface we are invoking.
	 * Get the methodTableIndex from ocb, imb, and guess
	 */
        case opc_invokeinterface_quick: 
        case opc_invokeinterface:
            isInvokeVirtual = CVM_FALSE;
            goto doInvokeVirtualOrInterfaceIR;

	/* Unresolvable cp entry */
	case opc_invokevirtual_quick_w:
	case opc_invokevirtual: 
            isInvokeVirtual = CVM_TRUE;

        doInvokeVirtualOrInterfaceIR:
	{
            CVMUint16 cpIndex;
	    CVMMethodTypeID mid = CVM_TYPEID_ERROR;
	    CVMUint16 argSize;
	    CVMUint8 rtnType;
	    CVMJITIRNode* targetNode;
	    CVMJITIRNode* plist;

            if (isInvokeVirtual) {
                FETCH_CPINDEX_ATOMIC(cpIndex);
            } else {
                cpIndex = CVMgetUint16(absPc+1);
            }

	    /* Connect flow and flush all locals before possibly throwing
	       an exception in the invocation code coming up below: */
	    connectFlowToExcHandlers(con, mc, pc, CVM_TRUE);

            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, &mid)) {
	  	/* Since cpIndex must be resolved for invokevirtual_quick_w,
		   we combine the case together.  Use mtIndex to 
		   build constant node of CVMJIT_METHODTABLE_INDEX */
                CVMMethodBlock *prototypeMb = CVMcpGetMb(cp, cpIndex);
                CVMMethodBlock *hintMb = NULL;
                CVMMethodBlock *candidateMb = NULL;
                CVMUint16 mtIndex;

#ifndef CVM_TRUSTED_CLASSLOADERS
                /* Make sure that both the opcode and the mb agree on whether
                 * or not the method is static. */
                if (CVMmbIs(prototypeMb, STATIC)) {
                    /* If not, we treat it as if it is unresolved and let the
                       compiler runtime take care of throwing an exception: */
		    mid = CVMmbNameAndTypeID(prototypeMb);
                    goto createResolveVirtualOrInterfaceMBNode;
                }
#endif
                mtIndex = CVMmbMethodTableIndex(prototypeMb);
                rtnType =
                    CVMtypeidGetReturnType(CVMmbNameAndTypeID(prototypeMb));
                argSize = CVMmbArgsSize(prototypeMb);
		CVMassert(fallThru);
		mc->startPC = absPc;

                if (isInvokeVirtual) {
                    if (CVMJITinlines(USE_VIRTUAL_HINTS)) {
                        hintMb = CVMjitGetInvokeVirtualHint(ee, absPc);
                    }
                } else {
                    if (CVMJITinlines(USE_INTERFACE_HINTS)) {
                        hintMb = CVMjitGetInvokeInterfaceHint(ee, absPc);
                    }
                }
                if (hintMb != NULL) {
                    CVMClassBlock *hintCb = CVMmbClassBlock(hintMb);
                    CVMClassLoaderICell *hintLoader = CVMcbClassLoader(hintCb);
                    CVMBool isSystemClassLoader = CVM_FALSE;

                    /* If the hintCb's class loader is the systemClassLoader,
                     * then that allows us to inline the hint method. This is
                     * because we know that the systemClassLoader always
                     * defers to the NULL class loader first.  This means that
                     * we can safely inline the hint method without having to
                     * worry about it being unloaded.
                     */
                    if (hintLoader != NULL) {
                        CVMID_icellSameObject(ee, hintLoader,
			    CVMsystemClassLoader(ee), isSystemClassLoader);
                    }

                    if (hintLoader == NULL || isSystemClassLoader) {
                        if (methodMatchesVirtualOrInterfaceMb(ee,
                                hintMb, prototypeMb, isInvokeVirtual)) {
                                candidateMb = hintMb;
                        }
                    }
                }
                if (isInvokeVirtual && (candidateMb == NULL)) {
                    candidateMb = prototypeMb;
                }

#if 0
		/* Print some debug info abou the invokevirtual guess */
		if (isInvokeVirtual) {
		    CVMconsolePrintf("ContextMB:   %C.%M\n",
				     CVMmbClassBlock(mc->mb), mc->mb);
		    CVMconsolePrintf("prototypeMb: %C.%M\n",
				     CVMmbClassBlock(prototypeMb),
				     prototypeMb);
		    if (hintMb != NULL)
			CVMconsolePrintf("hintmb:      %C.%M\n",
					 CVMmbClassBlock(hintMb), hintMb);
		    CVMconsolePrintf("candidateMb: %C.%M\n",
				     CVMmbClassBlock(candidateMb),
				     candidateMb);
		}
#endif
                if (doInvokeVirtualOrInterface(con, curbk, mtIndex, argSize,
                        rtnType, prototypeMb, candidateMb, isInvokeVirtual)) {
		    /* Inlined */
		    mc->startPC += instrLength;
		    return CVM_TRUE;
		}
	    } else {
		CVMJITIRNode* objRefNode;
                CVMJITIRNode* methodSpecNode;

#ifndef CVM_TRUSTED_CLASSLOADERS
            createResolveVirtualOrInterfaceMBNode:
#endif
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

		/* Compute args size, account for 'this' */
    	 	argSize = CVMtypeidGetArgsSize(mid) + 1;
    		rtnType = CVMtypeidGetReturnType(mid);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_METHOD_TABLE_INDEX_UNRESOLVED.  Then, append it
                   to the IR root list. */
                methodSpecNode = CVMJITirnodeNewResolveNode(con,
                    cpIndex, curbk, 0, (isInvokeVirtual ?
                        CVMJIT_CONST_METHOD_TABLE_INDEX_UNRESOLVED :
                        CVMJIT_CONST_INTERFACE_MB_UNRESOLVED));

                plist = collectParameters(con, curbk, argSize);
                objRefNode = CVMJITirnodeGetLeftSubtree(plist);
                targetNode = objectInvocationTarget(con, curbk,
                                isInvokeVirtual, objRefNode, methodSpecNode);
                invokeMethod(con, curbk, rtnType, plist, argSize,
			     CVM_FALSE, targetNode);
            }

            /* Make room for the CVMJIT_FETCH_MB_FROM_VTABLE code: */
            con->numLargeOpcodeInstructionBytes += isInvokeVirtual ?
                CVMCPU_INVOKEVIRTUAL_SIZE : CVMCPU_INVOKEINTERFACE_SIZE;
            break;
	}

	/* ignore invocation. This is a result of inlining a method
	 * that does nothing.
	 */
#define CVM_INLINING
#ifdef CVM_INLINING
        case opc_invokeignored_quick: {
	    CVMUint8 pc1Value = absPc[1];
            CVMUint32 endStackIndex = 0;

	    /* Make sure everything on the stack is evaluated in the
	       proper order first: */
	    if (pc1Value > 0) {
                CVMJITStack *operandStack = con->operandStack;
                CVMUint32 argCount;
                CVMJITirDoSideEffectOperator(con, curbk);
                argCount = argSize2argCount(con, pc1Value);
                endStackIndex = CVMJITstackCnt(con, operandStack) - argCount;
                if (absPc[2]) { 
                    CVMJITIRNode *node;
                    CVMJITIRNode *nullCheckedNode;

                    node = (CVMJITIRNode *)CVMJITstackGetElementAtIdx(con, 
                                             operandStack, endStackIndex);
                    nullCheckedNode = nullCheck(con, curbk, node, CVM_TRUE);
                    /* Null check on popped node */ 
                    if (nullCheckedNode != node) {
			/* Connect flow and flush all locals before possibly
			   throwing an exception in the invocation code
			   coming up below: */
			connectFlowToExcHandlers(con, mc, pc, CVM_TRUE);

                        /* No need to call the CVMJITirDoSideEffectOperator()
                           because it has already been called above. */
			CVMassert(CVMJITirnodeHasSideEffects(nullCheckedNode));
                        CVMJITirForceEvaluation(con, curbk, nullCheckedNode);
                    }
                }
                CVMJITstackSetCount(con, operandStack, endStackIndex);
            } else {
                CVMassert(absPc[2] == 0);
            }
	    break;
	}
#endif

	/* 
	 * Allocate memory for a new java object.
	 *
	 * -if the cp entry is resolveable and the class needs initializing,
	 *  then create a CHECKINIT node so code will be generated
	 *  to initialize the class.
	 * -if the cp entry is not resolvable, then create a
	 *  RESOLVE_REFERENCE node so code will be generated to resovle
	 *  the entry
	 */
    	case opc_new:
	case opc_new_quick:
	case opc_new_checkinit_quick: {
	    CVMUint16 cpIndex = CVMgetUint16(absPc+1);
            CVMJITIRNode *cbNode;
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, NULL)) {
		CVMClassBlock* newCb = CVMcpGetCb(cp, cpIndex);
#ifndef CVM_TRUSTED_CLASSLOADERS
                if (CVMcbIs(newCb, INTERFACE) || CVMcbIs(newCb, ABSTRACT)) {
		    /* If not, we treat it as if it is unresolved and let the
                       compiler runtime take care of throwing an exception: */
                    goto createResolveCBNodeForNew;
                }
#endif
		cbNode = CVMJITirnodeNewConstantCB(con, newCb);
		/* generate a CHECKINIT node if needed */
                if (checkInitNeeded(con, newCb)) { 
                    cbNode = doCheckInitOnCB(con, newCb, curbk, cbNode);
		}
	    } else { /* unresolvable */
#ifndef CVM_TRUSTED_CLASSLOADERS
            createResolveCBNodeForNew:
#endif
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_NEW_CB_UNRESOLVED with the CVMJITUNOP_CLASSINIT
                   flag set.  Then, append it to the IR root list. */
                cbNode =
		    CVMJITirnodeNewResolveNode(con, cpIndex, curbk,
                        CVMJITUNOP_CLASSINIT, CVMJIT_CONST_NEW_CB_UNRESOLVED);
	    }

	    /* Build NEW_OBJECT node and push it on stack */
            doNewObject(con, cbNode);
            break;
	}

	/* Create new array of basic type */

        case opc_newarray: {
	    /* Build NEW_ARRAY_BASIC and push it to stack */
            CVMUint16 nodeTag = CVMJIT_ENCODE_NEW_ARRAY_BASIC(absPc[1]);
	    CVMJITIRNode* node = 
		CVMJITirnodeNewUnaryOp(con, nodeTag, 
				       CVMJITirnodeStackPop(con)); 
            CVMJITirnodeSetThrowsExceptions(node);
	    CVMJITirnodeSetUnaryNodeFlag(node, CVMJITUNOP_ALLOCATION);
	    CVMJITirnodeStackPush(con, node);
	    /* Forget all cached index expressions.
	       This operation might cause a GC */
	    boundsCheckKillIndexExprs(con);
	    break;
	}

	/* Create new array of reference */

        case opc_anewarray_quick:
        case opc_anewarray: { /* CP index entry is unresolvable */
            CVMJITIRNode* constNode = NULL;
	    CVMJITIRNode* node;
	    CVMUint16 cpIndex = CVMgetUint16(absPc+1);

	    /*
	     * If the cp entry is resolved, then the CONST_CB node
	     * will contain the arrayCb. If it is unresolved, then the
	     * CONST_CB_UNRESOLVED node will contain the unresolved
	     * elementCb.
	     */
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, NULL)) {
		CVMClassBlock* elmentCb = CVMcpGetCb(cp, cpIndex);
		CVMClassBlock* arrayCb =
		    CVMclassGetArrayOfWithNoClassCreation(ee, elmentCb);
		if (arrayCb != NULL) {
		    /* We managed to get the array class without
		       potentially calling into Java.
		       Build constant of CVMJIT_CONST_CB */
		    constNode = CVMJITirnodeNewConstantCB(con, arrayCb);
		}
	    }

	    if (constNode == NULL) {
		/* Resolution could cause Java class loader code
		   to run. Kill cached pointers. */
		killAllCachedReferences(con);
		
		/* Build RESOLVE_REFERENCE node with opcode
		   CVMJIT_CONST_ARRAY_CB_UNRESOLVED.  Then, append it to
		   the IR root list. */
		constNode = CVMJITirnodeNewResolveNode(
		    con, cpIndex, curbk, 0, 
		    CVMJIT_CONST_ARRAY_CB_UNRESOLVED);
	    }

            /* Build NEW_ARRAY_REF node and push to stack */ 
            node = 
		CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_NEW_ARRAY_REF,
					constNode, CVMJITirnodeStackPop(con));
            CVMJITirnodeSetThrowsExceptions(node);
	    CVMJITirnodeSetBinaryNodeFlag(node, CVMJITBINOP_ALLOCATION);
            CVMJITirnodeStackPush(con, node);
	    /* Forget all cached index expressions.
	       This operation might cause a GC */
	    boundsCheckKillIndexExprs(con);
            break;
	}

	/* Create new multidimensional array */

        case opc_multianewarray_quick:
        case opc_multianewarray: { /* Unresolvable CP index entry */
            /* Build parameter list of counts popped from stack */
            CVMJITIRNode* node = collectParameters(con, curbk, absPc[3]);
            CVMJITIRNode* constNode;
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);

	    /* Build Constant node */
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, NULL)) {
                /* Build constant of CVMJIT_CONST_CB */
                constNode = 
		    CVMJITirnodeNewConstantCB(con, CVMcpGetCb(cp, cpIndex));;
            } else {
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);
                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_CB_UNRESOLVED.  Then, append it to the IR root
                   list. */
	    	constNode = CVMJITirnodeNewResolveNode(
		    con, cpIndex, curbk, 0, CVMJIT_CONST_CB_UNRESOLVED);
	    }

            /* Build MULTIANEW_ARRAY node and push it to stack */ 
            node = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_MULTIANEW_ARRAY,
					   constNode, node);
            CVMJITirnodeSetThrowsExceptions(node);

            /* Store the number of dimensions: */
            CVMJITirnodeGetBinaryOp(node)->data = absPc[3];
	    CVMJITirnodeSetBinaryNodeFlag(node, CVMJITBINOP_ALLOCATION);
            CVMJITirnodeStackPush(con, node);
	    /* Forget all cached index expressions.
	       This operation might cause a GC */
	    boundsCheckKillIndexExprs(con);
	    break;
	}

        case opc_ldc: {
            CVMUint8 cpIndex = absPc[1];
	    switch (CVMcpEntryType(cp, cpIndex)) {
	    case CVM_CONSTANT_StringICell:
		pushConstantStringICell(con, CVMcpGetStringICell(cp, cpIndex));
		break;
	    case CVM_CONSTANT_Integer:
	    case CVM_CONSTANT_Float:
		pushConstant32(con, CVMcpGetVal32(cp, cpIndex).i);
		break;
	    case CVM_CONSTANT_ClassTypeID:
	    case CVM_CONSTANT_ClassBlock:
		doJavaInstanceRead(con, curbk, cp, cpIndex, cb);
		break;
	    default:
		CVMJITerror(con, CANNOT_COMPILE, "unknown ldc operand");
	    }
	    break;
	}

        case opc_ldc_w: {
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
	    switch (CVMcpEntryType(cp, cpIndex)) {
	    case CVM_CONSTANT_StringICell:
		pushConstantStringICell(con, CVMcpGetStringICell(cp, cpIndex));
		break;
	    case CVM_CONSTANT_Integer:
	    case CVM_CONSTANT_Float:
		pushConstant32(con, CVMcpGetVal32(cp, cpIndex).i);
		break;
	    case CVM_CONSTANT_ClassTypeID:
	    case CVM_CONSTANT_ClassBlock:
		doJavaInstanceRead(con, curbk, cp, cpIndex, cb);
		break;
	    default:
		CVMJITerror(con, CANNOT_COMPILE, "unknown ldc operand");
	    }
	    break;
	}

	/* Push int, float, string, long, or double from constant pool */

	case opc_ldc_quick: { /* int or float */
            CVMUint8 cpIndex = absPc[1];
            pushConstant32(con, CVMcpGetVal32(cp, cpIndex).i);
	    break;
	}

	case opc_ldc_w_quick: { /* int or float */
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
            pushConstant32(con, CVMcpGetVal32(cp, cpIndex).i);
	    break;
	}

        case opc_aldc_quick: { /* direct String refs (ROM only) */
            CVMUint8 cpIndex = absPc[1];
            pushConstantStringObject(con, CVMcpGetStringObj(cp, cpIndex));
	    break;
	}

        case opc_aldc_w_quick: { /* direct String refs (ROM only) */
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
            pushConstantStringObject(con, CVMcpGetStringObj(cp, cpIndex));
	    break;
	}

	case opc_aldc_ind_quick: {/* Indirect String (loaded classes) */
            CVMUint8 cpIndex = absPc[1];
	    pushConstantStringICell(con, CVMcpGetStringICell(cp, cpIndex));
	    break;
	}

	case opc_aldc_ind_w_quick: {/* Indirect String (loaded classes) */
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
	    pushConstantStringICell(con, CVMcpGetStringICell(cp, cpIndex));
	    break;
	}

        case opc_ldc2_w: /* long or double */
	case opc_ldc2_w_quick: {
            CVMUint16 cpIndex = CVMgetUint16(absPc+1);
	    CVMJavaVal64 v64;
	    CVMcpGetVal64(cp, cpIndex, v64);
	    pushConstantJavaVal64(con, &v64, CVMJIT_TYPEID_64BITS);
	    break;
	}


	/* Quick object field access byte-codes. The non-wide version are
	 * all lossy and are not needed when in losslessmode.
	 */


	/* Used to be Lossy mode: We now rely on quicken.c to 
         * do the right thing based on runtime decisions.
         * Romized code has quickened opcode hence the check for 
         * !CVMcbIsInROM().
	 * Quickened opcodes from opc_getfield and opc_getfield.
	 * field index is embedded in the instruction.
	 */

	case opc_agetfield_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
            doInstanceFieldRead(con, curbk, pc, NULL, fieldOffset,
				CVM_TYPEID_OBJ, CVM_FALSE);
	    break;
        }

        case opc_getfield_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
            doInstanceFieldRead(con, curbk, pc, NULL, fieldOffset,
				CVMJIT_TYPEID_32BITS, CVM_FALSE);
	    break;
        }

	/* Quickened from getfield if CVMfbIsDoubleWord(fb) */ 
        case opc_getfield2_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
	    doInstanceFieldRead(con, curbk, pc, NULL, fieldOffset,
				CVMJIT_TYPEID_64BITS, CVM_FALSE);
	    break;
        }

	/* Quickened from putfield if CVMfbIsRef(fb) */ 
	case opc_aputfield_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
            doInstanceFieldWrite(con, curbk, pc, NULL, fieldOffset,
				 CVM_TYPEID_OBJ, CVM_FALSE);
	    break;
        }

	/* Quickened from get|putfield if !CVMfbIsDoubleWord(fb) && 
	   !CVMfbIsRef(fb) resolved 32-bit field reference */
        case opc_putfield_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
            doInstanceFieldWrite(con, curbk, pc, NULL, fieldOffset,
				 CVMJIT_TYPEID_32BITS, CVM_FALSE);
	    break;
        }

	/* Quickened from getfield if CVMfbIsDoubleWord(fb) */ 
        case opc_putfield2_quick: {
	    CVMUint32 fieldOffset = absPc[1];
#ifdef CVM_JVMTI
            if (CVMjvmtiIsEnabled() && !CVMcbIsInROM(cb)) {
                goto unimplemented_opcode;
            }
#endif
            doInstanceFieldWrite(con, curbk, pc, NULL, fieldOffset,
				 CVMJIT_TYPEID_64BITS, CVM_FALSE);
	    break;
        }


        case opc_putfield_quick_w:
        case opc_getfield_quick_w: {
    	    CVMFieldBlock* fb = CVMcpGetFb(cp, CVMgetUint16(absPc+1));
	    CVMUint8 typeTag =
		CVMtypeidGetPrimitiveType(CVMfbNameAndTypeID(fb));
	    CVMUint32 fieldOffset = CVMfbOffset(fb);
            CVMBool isVolatile  = CVMfbIs(fb, VOLATILE);

	    if (opcode == opc_getfield_quick_w) {
		doInstanceFieldRead(con, curbk, pc, NULL, 
				    fieldOffset, typeTag, isVolatile);
	    } else {
		doInstanceFieldWrite(con, curbk, pc, NULL, 
				     fieldOffset, typeTag, isVolatile);
	    }
	    break;
        }

	case opc_getfield: 
	case opc_putfield: {
	    CVMJITIRNode*  constNode;
	    CVMUint16      fieldOffset;
	    CVMUint16      cpIndex;
	    CVMFieldTypeID fid = CVM_TYPEID_ERROR;
	    CVMBool        isVolatile;
            CVMUint8       typeTag;

	    FETCH_CPINDEX_ATOMIC(cpIndex);
	    
	    /* The constant pool entry might be resolved. If it is, this
	       case looks very much like the {get,put}field_quick_w. If it
	       is not, then we have to generate the fb resolution code. */
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, &fid)) {
		CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);

		fid = CVMfbNameAndTypeID(fb);
		fieldOffset = CVMfbOffset(fb);
		constNode = NULL;
		isVolatile = CVMfbIs(fb, VOLATILE);
                typeTag = CVMtypeidGetPrimitiveType(fid);

#ifndef CVM_TRUSTED_CLASSLOADERS
                /* Make sure that both the opcode and the fb agree on whether
                 * or not the field is static. */
                if (CVMfbIs(fb, STATIC) || ((opcode == opc_putfield) &&
                    !CVMfieldIsOKToWriteTo(ee, fb, cb, CVM_FALSE)))
		{
                    /* If not, we treat it as if it is unresolved and let the
                       compiler runtime take care of throwing an exception: */
                    goto createResolveFBNode;
                }
#endif
	    } else {
#ifndef CVM_TRUSTED_CLASSLOADERS
            createResolveFBNode:
#endif
                typeTag = CVMtypeidGetPrimitiveType(fid);

		/* Since the field is unresolved.  We assume it is volatile
		   just to be safe. */
		isVolatile = CVM_TRUE;
		
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_GETFIELD_FB_UNRESOLVED or
                   CVMJIT_CONST_PUTFIELD_FB_UNRESOLVED.  Then, append it to the
                   IR root list. */
		constNode = CVMJITirnodeNewResolveNode(
                    con, cpIndex, curbk, 0,
                    (opcode == opc_getfield) ?
                        CVMJIT_CONST_GETFIELD_FB_UNRESOLVED :
                        CVMJIT_CONST_PUTFIELD_FB_UNRESOLVED);
		fieldOffset = 0; /* prevent compiler warning */
	    }

	    /* Now cover both cases above */
	    if (opcode == opc_getfield) {
		doInstanceFieldRead(con, curbk, pc, constNode, fieldOffset,
				    typeTag, isVolatile);
	    } else {
		doInstanceFieldWrite(con, curbk, pc, constNode, fieldOffset,
				     typeTag, isVolatile);
	    }
	    
	    break;
        }

	/* 
	 * Quickened opcodes from opc_getstatic
	 */
	case opc_getstatic:
	case opc_agetstatic_quick:
	case opc_getstatic_quick:
	case opc_getstatic2_quick:
	case opc_agetstatic_checkinit_quick:
	case opc_getstatic_checkinit_quick:
	case opc_getstatic2_checkinit_quick: {
	    CVMUint16 cpIdx = CVMgetUint16(absPc+1);
            doStaticFieldRef(con, curbk, cp, cpIdx, cb, CVM_TRUE);
	    break;
	}
	    

	/*
	 * Quickened opcodes from opc_putstatic 
	 */
	case opc_putstatic:
	case opc_aputstatic_quick:
	case opc_putstatic_quick:
	case opc_putstatic2_quick:
	case opc_aputstatic_checkinit_quick:
	case opc_putstatic_checkinit_quick:
	case opc_putstatic2_checkinit_quick: {
	    CVMUint16 cpIdx = CVMgetUint16(absPc+1);
            doStaticFieldRef(con, curbk, cp, cpIdx, cb, CVM_FALSE);
	    break;
	}

	/* Check if an object is an instance of given type. Throw an
	 * exception if not. */
	case opc_checkcast:
        case opc_checkcast_quick: {
            CVMJITIRNode* objrefNode = CVMJITirnodeStackPop(con);
	    CVMJITIRNode* constNode;
	    CVMJITIRNode* node;
	    CVMUint16 cpIndex = CVMgetUint16(absPc+1);

            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, NULL)) {
                /* Build constant of CVMJIT_CONST_CB */
                CVMClassBlock* checkcastCB = CVMcpGetCb(cp, cpIndex); 
                constNode = CVMJITirnodeNewConstantCB(con, checkcastCB);
	    } else {
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

	        /* Unresolved cp index entry */
                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_CB_UNRESOLVED.  Then, append it to the IR root
                   list. */
	        constNode = CVMJITirnodeNewResolveNode(
		    con, cpIndex, curbk, 0, CVMJIT_CONST_CB_UNRESOLVED);
	    }

	   /* Build CHECKCAST node and append it to the root list */ 
	    node = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_CHECKCAST,
					   objrefNode, constNode);
            CVMJITirnodeSetThrowsExceptions(node);

            CVMJITirnodeStackPush(con, node);
            con->numLargeOpcodeInstructionBytes += CVMCPU_CHECKCAST_SIZE;
	    break;
	}

	/* Check if an object is an instance of given type and store
	 * result on the stack. */
        case opc_instanceof:
        case opc_instanceof_quick: {
            CVMJITIRNode* objrefNode = CVMJITirnodeStackPop(con);
            CVMJITIRNode* constNode;
	    CVMJITIRNode* node;
	    CVMUint16 cpIndex = CVMgetUint16(absPc+1);
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, cpIndex, NULL)) {
                /* Build constant of CVMJIT_CONST_CB */
                constNode = 
		    CVMJITirnodeNewConstantCB(con, CVMcpGetCb(cp, cpIndex));
	    } else {
		/* Resolution could cause Java class loader code to run. Kill
		   cached pointers. */
		killAllCachedReferences(con);

                /* Build RESOLVE_REFERENCE node with opcode
                   CVMJIT_CONST_CB_UNRESOLVED.  Then, append it to the IR root
                   list. */
                constNode = CVMJITirnodeNewResolveNode(
                    con, cpIndex, curbk, 0, CVMJIT_CONST_CB_UNRESOLVED);
	   }

	   /* Build INSTANCEOF node */
           node = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_INSTANCEOF, 
					  objrefNode, constNode);
           CVMJITirnodeStackPush(con, node);
           con->numLargeOpcodeInstructionBytes += CVMCPU_INSTANCEOF_SIZE;
	   break;
 	}

#ifdef CVM_JVMTI
        case opc_breakpoint: {
	    /* This should have been discovered during block discovery */
	    CVMassert(CVM_FALSE);
	}
#endif

#ifndef CVM_INLINING
	case opc_invokeignored_quick:
#endif
        case opc_xxxunusedxxx:
	case opc_exittransition: 
#ifdef CVM_JVMTI
 unimplemented_opcode:
#endif
        default:
	    CVMdebugPrintf(("\t*** jitir: Unimplemented opcode: %d = %s\n",
		   opcode, CVMopnames[opcode]));
	    CVMassert(CVM_FALSE);
	    break;
        } /* end of switch */

        pc += instrLength; 
        absPc += instrLength; 
    } /* end of while */

    checkEndInlining(con, curbk, mc);

    if (!fallThru) {
	CVMassert(CVMbcAttr(opcode, NOCONTROLFLOW) ||
	    mc->abortTranslation);
	{
#ifdef CVM_DEBUG_ASSERTS
	    CVMUint32 stackCnt = CVMJITstackCnt(con, con->operandStack);
	    
	    /*
	     * athrow, returns, and switches may leave items on the stack.
	     * No other NOCONTROLFLOW opcodes can except for ret, which
	     * we explicity fail to compile if an item is left on the
	     * stack. We need to decrement the refCount of all these
	     * stack items because they are one higher than they need
	     * to be. Then we need to clear the stack.
	     */
	    if (stackCnt > 0) {
		CVMassert(opcode == opc_tableswitch ||
			  opcode == opc_lookupswitch ||
			  opcode == opc_athrow ||
			  CVMJITOpcodeMap[opcode][JITMAP_OPC]
			  == CVMJIT_RETURN);
	    }
#endif
	    CVMJITstackResetCnt(con, con->operandStack);
	    /* Make sure this really does terminate a block,
	       or there are no more blocks */
	    CVMassert(mc->abortTranslation ||
		      ((absPc == endPc) && 
		       CVMJITirblockIsLastJavaBlock(curbk)) || 
		      (mc->pcToBlock[pc] == CVMJITirblockGetNext(curbk)));
	}
    }

    mc->startPC = absPc;

    return fallThru;
}

/*
 * Look at the method signature and type the ref-based locals
 * on entry
 */
static void
setLocalsStateAtMethodEntry(CVMJITCompilationContext* con,
			    CVMJITIRBlock* firstBlock)
{
    CVMMethodTypeID  tid = CVMmbNameAndTypeID(con->mb);
    CVMterseSigIterator  terseSig;
    int	       typeSyllable;

    CVMJITSet* refLocals = &firstBlock->localRefs;
    CVMUint32 varNo = 0;

    CVMassert(!CVMJITsetInitialized(refLocals));
    CVMJITsetInit(con, refLocals);

    /* Every local is a non-ref by default */

    CVMtypeidGetTerseSignatureIterator(tid, &terseSig);

    if (!CVMmbIs(con->mb, STATIC)) {
	CVMJITlocalrefsSetRef(con, refLocals, varNo);
	varNo++;
    }	
    /*
     * Parse signature and type incoming locals
     */
    while((typeSyllable = CVM_TERSE_ITER_NEXT(terseSig)) != CVM_TYPEID_ENDFUNC){
	switch( typeSyllable ){
	case CVM_TYPEID_OBJ:
            CVMJITlocalrefsSetRef(con, refLocals, varNo);
	    varNo++; break;
	case CVM_TYPEID_LONG: 
	case CVM_TYPEID_DOUBLE:
            CVMassert(!CVMJITlocalrefsIsRef(refLocals, varNo));
            CVMassert(!CVMJITlocalrefsIsRef(refLocals, varNo+1));
	    varNo += 2; break;
	default:
            CVMassert(!CVMJITlocalrefsIsRef(refLocals, varNo));
	    varNo++;
	}
    }
    CVMassert(varNo <= con->maxLocalWords);
    CVMassert(varNo <= firstBlock->inMc->currLocalWords);
}

/* 
 * The first pass of Byte2IR translator traverses the bytecode instructions
 * in a method and do the following things: 
 * 1. Create first block with label 0.
 * 2. Discover all branch labels (PC) and exception handle labels (PC), 
 *    and add them to a bitset called labelSet. 
 * 3. All found branch and exception handler PC have a newly created block, and
 *    are mapped to pcToBlock map table.
 * 4. Uses the labelsSet to connect blocks in PC straight-line order 
 *    into the block list and set block id to each block in asending order.
 *
 * Both bitsets contains elements that are java bytecode PC corresponding
 * to the extended basic block.
 */

static void 
firstPass(CVMJITCompilationContext* con)
{
    /* 
     * Setup the bitmap that indicates which java pc's need to eventually
     * be mapped to a compiled pc.
     */
    CVMJITirblockMarkAllPCsThatNeedMapping(con);

    /* Discover exception handler blocks */
    CVMJITirblockFindAllExceptionLabels(con);

    /* Now find the rest of the blocks */
    CVMJITirblockFindAllNormalLabels(con);
    CVMJITirblockConnectBlocksInOrder(con);
}


/*
 * Process missed PC mappings in unreachable blocks
 * Is there a better way?
 */
static void
processUnreachedBlocks(CVMJITCompilationContext *con)
{
    CVMJITIRBlock *bk = (CVMJITIRBlock*)CVMJITirlistGetHead(&con->bkList);
    while (bk != NULL) {
	if (!CVMJITirblockIsTranslated(bk)) {
	    CVMJITMethodContext *mc = con->mc = bk->inMc;
	    CVMUint32 codeLength = CVMjmdCodeLength(mc->jmd);
	    CVMUint32 blockLength = CVMJITirblockMaxPC(bk, codeLength);
	    int startPC = CVMJITirblockGetBlockPC(bk);
	    int endPC = blockLength;
	    int pc;
	    int lastPC = -1;
	    CVMBool needEndInlining = 
		mc->compilationDepth > 0 && CVMJITirblockIsLastJavaBlock(bk);

	    CVMassert(blockLength <= codeLength);
	    for (pc = startPC; pc < endPC; ++pc) {
		CVMBool mapPc;
		CVMJITsetContains(&mc->mapPcSet, pc, mapPc);
		if (mapPc) {
		    lastPC = pc;
		}
	    }

	    if (lastPC != -1 || needEndInlining) {
		CVMJITirblockAddFlags(bk, CVMJITIRBLOCK_IS_TRANSLATED);
		CVMJITirblockAddFlags(bk, CVMJITIRBLOCK_NO_JAVA_MAPPING);
		/* We don't need a GC check for this empty block */
		CVMJITirblockClearFlags(bk,
		    CVMJITIRBLOCK_IS_JSR_RETURN_TARGET);
		CVMJITirblockClearFlags(bk,
		    CVMJITIRBLOCK_IS_BACK_BRANCH_TARGET);

		CVMJITsetInit(con, &bk->localRefs);
		CVMJITstackResetCnt(con, con->operandStack);
		pushNewRange(con, bk, startPC);

		if (lastPC != -1) {
		    createMapPcNodeIfNeeded(con, bk, lastPC, CVM_TRUE);
		}
		if (needEndInlining) {
		    endInlining(con, bk, NULL);
		}

		bk->lastOpenRange->endPC = endPC;
	    }
	}
	bk = CVMJITirblockGetNext(bk);
    }
}

#ifdef CVM_JIT_REGISTER_LOCALS

#define localAlreadyOrdered(bk, idx) \
    ((bk->orderedIncomingLocals & (1 << idx)) != 0)

/*
 * Dump all the incoming locals for the block.
 */
#ifdef CVM_TRACE_JIT
static void
dumpIncomingLocals(CVMJITIRBlock* bk, const char * prefix)
{
    if (bk->incomingLocalsCount > 0) {
	int i;
	CVMconsolePrintf("%s bkID(%d) Incoming Locals(%d 0x%x): ", prefix,
                         CVMJITirblockGetBlockID(bk),
			 bk->incomingLocalsCount,
			 bk->orderedIncomingLocals);
	for (i = 0; i < bk->incomingLocalsCount; i++) {
	    if (bk->incomingLocals[i] == NULL) {
		CVMconsolePrintf("<null> ");
	    } else {
		CVMconsolePrintf("%d%c ",
		    getLocalNo(bk->incomingLocals[i]),
		    localAlreadyOrdered(bk, i) ? '+' : '-');
	    }
	}
	CVMconsolePrintf("\n");
    }
}
#endif

static void
mergeIncomingLocal(CVMJITIRBlock* bk, CVMJITIRNode* localNode);

/*
 * Merge incoming locals state of each block with the state of its successor
 * blocks.
 */
static void
mergeIncomingLocalsFromSuccessorBlocks(CVMJITCompilationContext* con,
				       CVMJITIRBlock* lastBlock,
				       CVMBool firstPass)
{
    CVMJITIRBlock* bk = lastBlock;
    int bkIdx;

    /* do merge for each block, starting with the last block */
    while (bk != NULL) {
	int oldLocalOrder = 0;
	CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "orig"));

	if (firstPass) {
	    /* copy to originalIncomingLocals for safe keeping */
	    int i;
	    bk->originalIncomingLocals =
		CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
			     bk->incomingLocalsCount * sizeof(CVMJITIRNode*));
	    for (i = 0; i < bk->incomingLocalsCount; i++) {
		bk->originalIncomingLocals[i] = bk->incomingLocals[i];
	    }
	    bk->originalIncomingLocalsCount = bk->incomingLocalsCount;
	}

	/* If no successor blocks, then nothing to merge */
	if (bk->successorBlocksCount == 0) {
	    bk = CVMJITirblockGetPrev(bk);
	    continue;
	}

	/*
	 * Reset incomingLocalsCount. We'll be merging our own locals
	 * interspersed with successor block locals based on the order locals
	 * were reference in our block and the order of branches to 
	 * successor blocks.
	 */
	bk->incomingLocalsCount = 0;

	/* Don't merge anything if we have incoming phis. */
	if (bk->phiCount > 0) {
	    bk = CVMJITirblockGetPrev(bk);
	    continue;
	}

	/* Skip over slots that are locked out because of outgoing phis */
	while (localAlreadyOrdered(bk, bk->incomingLocalsCount)) {
	    CVMassert(bk->incomingLocals[bk->incomingLocalsCount] == NULL);
	    bk->incomingLocalsCount++;
	}

	/* merge all successor blocks incoming locals into bk */
	for (bkIdx = 0; bkIdx < bk->successorBlocksCount; bkIdx++) {
	    CVMJITIRBlock* sbk = bk->successorBlocks[bkIdx];
	    int localOrder = bk->successorBlocksLocalOrder[bkIdx];
	    int assignOrder = bk->successorBlocksAssignOrder[bkIdx];
	    int i;

	    /* don't merge from this block if not allowed to */
	    if (!bk->okToMergeSuccessorBlock[bkIdx]) {
		continue;
	    }

	    /*
	     * Copy our own incoming locals that were added before
	     * the successor block's incoming locals.
	     */
	    while (oldLocalOrder < localOrder) {
		mergeIncomingLocal(
                    bk, bk->originalIncomingLocals[oldLocalOrder++]);
	    }
	    CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "o1  "));

	    /*
	     * Copy succesor blocks incoming locals.
	     */
	    for (i = 0; i < sbk->incomingLocalsCount; i++) {
		CVMJITIRNode* localNode = sbk->incomingLocals[i];
		CVMUint16 localNo;
		if (localNode == NULL) {
		    continue;
		}
		localNo = getLocalNo(localNode);

		/* Don't copy locals for which there is an ASSIGN */
		if (assignNodeExists(bk->assignNodes, assignOrder, localNo)) {
		    continue;
		}

		/*
		 * Don't copy ref locals if either we know they will be
		 * spilled or they are not covered by a stackmap.
		 */
		if (CVMJITirnodeIsReferenceType(localNode)) {
		    CVMBool isLocalRef;
		    if (!bk->okToMergeSuccessorBlockRefs[bkIdx]) {
			/* we know it will be lost before used */
			isLocalRef = CVM_FALSE;
		    } else {
			/* no stack map to cover this local so not used */
#ifdef CVM_DEBUG_ASSERTS
			CVMJITsetContains(&bk->localRefs, localNo, isLocalRef);
			CVMassert(isLocalRef);
#endif
			isLocalRef = CVM_TRUE;
		    }
		    if (!isLocalRef) {
			continue;
		    }
		} else {
#ifdef CVM_DEBUG_ASSERTS
		    CVMBool isLocalRef;
		    CVMJITsetContains(&bk->localRefs, localNo, isLocalRef);
		    CVMassert(!isLocalRef);
#endif
		}

		/* merge the local if there's room and it's not a dup */
		mergeIncomingLocal(bk, localNode);
		CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "s1  "));
	    }
	}
	/*
	 * Copy our own incoming locals that were added after
	 * the last successor.
	 */
	while (oldLocalOrder < bk->originalIncomingLocalsCount) {
	    mergeIncomingLocal(
		bk, bk->originalIncomingLocals[oldLocalOrder++]);
	}

	/*
	 * It's possible that we have fewer incoming locals now. This can
	 * happen when on the second merge pass, the successor block
	 * replaces a non-ref with a ref, and we can't merge in the reg.
	 * Make sure all unused slots are NULL to prevent problems later.
	 */
	{
	    int i = bk->incomingLocalsCount;
	    while (i < CVMJIT_MAX_INCOMING_LOCALS) {
		bk->incomingLocals[i++] = NULL;
	    }
	}

	CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "o1  "));
	bk = CVMJITirblockGetPrev(bk);
    }

    return;
}

/*
 * Merge the specified local into the block if possible.
 */
static void
mergeIncomingLocal(CVMJITIRBlock* bk, CVMJITIRNode* localNode)
{    
    if (bk->incomingLocalsCount == CVMJIT_MAX_INCOMING_LOCALS) {
	return;
    }
    if (localNode == NULL) {
	return;
    }

    /* No need to copy local if it already exists */
    {
	int j;
	int localNo = getLocalNo(localNode);
	for (j = 0; j < bk->incomingLocalsCount; j++) {
	    if (bk->incomingLocals[j] == NULL) {
		continue;
	    }
	    if (getLocalNo(bk->incomingLocals[j]) == localNo)  {
		CVMassert(CVMJITgetTypeTag(localNode) ==
			  CVMJITgetTypeTag(bk->incomingLocals[j]));
		return;
	    }
	}
    }
    
    /* copy the local from the successor block */
    bk->incomingLocals[bk->incomingLocalsCount++] = localNode;
}

/*
 * If possible, force the local whose localNo maches the incomingLocal at
 * srcIdx in srcBk to be located at srcIdx in dstBk.
 */
static void
orderLocal(CVMJITIRBlock* srcBk, CVMJITIRBlock* dstBk, int srcIdx)
{
    CVMJITIRNode* localNode = srcBk->incomingLocals[srcIdx];
    int dstIdx = CVMJITirblockFindIncomingLocal(dstBk, getLocalNo(localNode));
    if (dstIdx == -1) {
	return; /* not found */
    }
    CVMtraceJITRegLocals(("dstBk(%d) local(%d %d) found in slot(%d)\n",
                          CVMJITirblockGetBlockID(dstBk), srcIdx,
			  getLocalNo(localNode), dstIdx));

    /* if local is in same slot in both blocks, then we are done */
    if (dstIdx == srcIdx) {
	CVMtraceJITRegLocals(("local(%d %d) same slot\n",
			      srcIdx, getLocalNo(localNode)));
	/* make sure both order bits are set and return */
 	dstBk->orderedIncomingLocals |= (1 << srcIdx);
 	srcBk->orderedIncomingLocals |= (1 << srcIdx);
	return;
    }

    /* if dstBk has the local at dstIdx ordered, then we can't move it */
    if (localAlreadyOrdered(dstBk, dstIdx)) {
	CVMtraceJITRegLocals(("dstIdx local(%d %d) already ordered in dst\n",
			      dstIdx, getLocalNo(localNode)));
	return; /* can't reorder since current slot already ordered */
    }

    /* if dstBk has the local at srcIdx ordered, then we can't move it */
    if (localAlreadyOrdered(dstBk, srcIdx)) {
	CVMtraceJITRegLocals(("srcIdx local(%d %d) already ordered in dst\n",
			      srcIdx, getLocalNo(localNode)));
	return; /* can't reorder since target slot already ordered */
    }

    /* In dstBk, swap dstIdx with srcIdx */
    {
	CVMJITIRNode* dstNode = dstBk->incomingLocals[dstIdx];
	CVMJITIRNode* srcNode = dstBk->incomingLocals[srcIdx];
	CVMtraceJITRegLocals(("local(#%d) swapped with local(#%d)\n",
			      srcIdx, dstIdx));
	dstBk->incomingLocals[dstIdx] = srcNode;
	dstBk->incomingLocals[srcIdx] = dstNode;
 	dstBk->orderedIncomingLocals |= (1 << srcIdx);
 	srcBk->orderedIncomingLocals |= (1 << srcIdx);
   }

    /* we may have swapped with NULL node at end of array */
    if (srcIdx >= dstBk->incomingLocalsCount) {
	dstBk->incomingLocalsCount = srcIdx + 1;
    }
}

/*
 * Order the locals between bk and the goal blocks.
 */
static void
orderIncomingLocalsForBlocks(CVMJITCompilationContext* con,
			     CVMJITIRBlock* bk,
			     CVMJITIRBlock* goalBks[],
			     int goalBksCount)
{
    int goalBksIdx;

    /*
     * For each goal block, order locals between bk and the goal block.
     */
    for (goalBksIdx = goalBksCount - 1; goalBksIdx >= 0; goalBksIdx--) {
	CVMJITIRBlock* goalBk = goalBks[goalBksIdx];
	int goalBkLocalIdx;
	int bkLocalIdx;

	/*
	 * Order each local in bk. Note that we iterate based on the local's
	 * index (slot #), and order the localNo found in that slot. It
	 * could end up in some other slot, in which case we need to
	 * repeat the interation on the same local index since it will
	 * now have a new localNo in it.
	 */
	for (bkLocalIdx = 0; bkLocalIdx < bk->incomingLocalsCount;
	     bkLocalIdx++) {
	    int j;
	    int bestLocal = -1;
	    int bestLocalScore = 0;
	    int bkLocalNo;

	    if (bk->incomingLocals[bkLocalIdx] == NULL) {
		continue;
	    }
	    bkLocalNo = getLocalNo(bk->incomingLocals[bkLocalIdx]);
	    CVMtraceJITRegLocals(("bkID(%d) local(%d #%d) ",
                CVMJITirblockGetBlockID(bk), bkLocalIdx, bkLocalNo));

	    /* if local not in current goalBk, then skip */
	    goalBkLocalIdx = CVMJITirblockFindIncomingLocal(goalBk, bkLocalNo);
	    if (goalBkLocalIdx == -1) {
		CVMtraceJITRegLocals((" not found in goalBk\n"));
		continue; /* not found in our goalBk so can't order */
	    }

	    if (localAlreadyOrdered(goalBk, goalBkLocalIdx)) {
		/* Already ordered in goalBk, so force that order on bk. */
		CVMtraceJITRegLocals((" ordered in goalBk(%d)\n",
                                      CVMJITirblockGetBlockID(goalBk)));
		CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "-bk "));
		orderLocal(goalBk, bk, goalBkLocalIdx);
		CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "+bk "));
	    }

	    if (localAlreadyOrdered(bk, bkLocalIdx)) {
		/* Already ordered in bk, so force that order on goalBks */
		int i;
		CVMtraceJITRegLocals((" ordered in bk\n"));
		for (i = 0; i <= goalBksIdx; i++) {
		    CVMtraceJITRegLocalsExec(
                        dumpIncomingLocals(goalBks[i], "-gbk"));
		    orderLocal(bk, goalBks[i], bkLocalIdx);
		    CVMtraceJITRegLocalsExec(
			dumpIncomingLocals(goalBks[i], "+gbk"));
		}
		continue;
	    }

	    /* if it's now ordered in the goalBk, then we are done */
	    if (localAlreadyOrdered(goalBk, goalBkLocalIdx)) {
		continue;
	    }
	    CVMtraceJITRegLocals((" not ordered\n"));

	    /*
	     * If we get here we know the local is ordered in neither of bk
	     * and goalBk. Choose the order (slot #) that is best not
	     * just for bk and goalBk, but for all the other goalBks,
	     * giving most importance to goalBk.
	     *
	     * We look at all all slots in bk and goalBk. The first
	     * requirement is that it must be available in both. If
	     * so, we score the slot based on how well it works for
	     * other goalBks involved.
	     */
	    for (j = 0; j < CVMJIT_MAX_INCOMING_LOCALS; j++) {
		/* see if slot available in bk and goalBk */
		if (!localAlreadyOrdered(bk, j) && 
		    !localAlreadyOrdered(goalBk, j))
		{
		    int i;
		    int score = 0;

		    /* score the slot, based on how other goalBks like it */
		    for (i = goalBksIdx ; i >= 0; i--) {
			if (!localAlreadyOrdered(goalBks[i], j)) {
			    /* this slot is free */
			    score += ((i + 1) << 3);
			} else if (CVMJITirblockFindIncomingLocal(
                                       goalBks[i],  bkLocalNo) == j) {
			    /* this slot contains the local we are testing */
			    score += ((i + 1) << 3);
			} else {
			    /* this slot in use in goalBks[i] already */
			    if (i != goalBksIdx && 
				goalBks[i]->incomingLocals[j] != NULL)
			    {
				int localNo = 
				    getLocalNo(goalBks[i]->incomingLocals[j]);
				if (CVMJITirblockFindIncomingLocal(
                                        goalBks[i+1], localNo) != -1) {
				    /* worse yet, it has another local we
				     * need to setup for previous goalBk */
				    score -= (i + 1);
				}
			    }
			    /* don't add to score once bad goalBk is found */
			    break;
			}
		    }
		    if (score > bestLocalScore) {
			bestLocal = j;
			bestLocalScore = score;
		    }
		}
	    }
	    if (bestLocal == -1) {
		continue; /* no luck */
	    }
	    CVMtraceJITRegLocals(("bestLocal(%d) bestLocalScore(%d)\n",
				  bestLocal, bestLocalScore));

	    /*
	     * We've found the best slot for the local, so now force it
	     * into that slot for every block that will accept it, which
	     * means every block that doesn't already have that slot ordered,
	     * and doesn't have the local we want to put in the slot ordered.
	     */
	    {
		int i;
		CVMJITIRNode* temp;

		/* impose order for local on bk */
		temp = bk->incomingLocals[bkLocalIdx];
		CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "-bk "));
		bk->incomingLocals[bkLocalIdx] = bk->incomingLocals[bestLocal];
		bk->incomingLocals[bestLocal] = temp;
		bk->orderedIncomingLocals |= (1 << bestLocal);
		/* we may have swapped with NULL node at end of array */
		if (bestLocal >= bk->incomingLocalsCount) {
		    bk->incomingLocalsCount = bestLocal + 1 ;
		}
		CVMtraceJITRegLocalsExec(dumpIncomingLocals(bk, "+bk "));

		/* impose order for local on each goalBk */
		for (i = 0 ; i < goalBksCount; i++) {
		    CVMJITIRBlock* goalBk = goalBks[i];
		    goalBkLocalIdx =
			CVMJITirblockFindIncomingLocal(goalBk, bkLocalNo);
		    if (goalBkLocalIdx == -1) {
			continue; /* local not in goal blocok */
		    }
		    if (localAlreadyOrdered(goalBk, goalBkLocalIdx) ||
			localAlreadyOrdered(goalBk, bestLocal)) {
			continue; /* src or dst slot already ordered */
		    }
		    temp = goalBk->incomingLocals[goalBkLocalIdx];
		    CVMtraceJITRegLocalsExec(
                        dumpIncomingLocals(goalBk, "-gbk"));
		    goalBk->incomingLocals[goalBkLocalIdx] =
			goalBk->incomingLocals[bestLocal];
		    goalBk->incomingLocals[bestLocal] = temp;
		    goalBk->orderedIncomingLocals |= (1 << bestLocal);
		    /* we may have swapped with NULL node at end of array */
		    if (bestLocal >= goalBk->incomingLocalsCount) {
			goalBk->incomingLocalsCount = bestLocal + 1 ;
		    }
		    CVMtraceJITRegLocalsExec(
                        dumpIncomingLocals(goalBk, "+gbk"));
		}
	    }

	    /* recheck current slot because we swapped into it */
	    bkLocalIdx--;
	}
    }
}

/*
 * To the extent possible, make sure incoming locals flow from block
 * to block the same slot so we don't get a lot of register movement.
 */
static void
orderIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* lastBlock)
{
    int bkIdx;
#define CVMJIT_MAX_BB_BLOCKS_IN_GOAL_STACK 12
    CVMJITIRBlock* goalBks[CVMJIT_MAX_BB_BLOCKS_IN_GOAL_STACK +
			   CVMJIT_MAX_SUCCESSOR_BLOCKS];
    int goalBksCount = 0;

    CVMJITIRBlock* bk = lastBlock;
    while (bk != NULL) {
	int i;
	int oldGoalBksCount = goalBksCount;
	CVMJITIRBlock* targetBk = bk;

	/* Add all our successor blocks to the goalBks stack */
	for (bkIdx = bk->successorBlocksCount - 1; bkIdx >= 0; bkIdx--) {
	    if (bk->okToMergeSuccessorBlock[bkIdx]) {
		goalBks[goalBksCount++] = bk->successorBlocks[bkIdx];
	    }
	}

	/* Order locals between bk and everthing in goalBks */
	for (i = 0; i < goalBksCount; i++) {
	    orderIncomingLocalsForBlocks(con, targetBk, &goalBks[i],
					 goalBksCount - i);
	    targetBk = goalBks[i];
	}

	/* remove all our successor blocks from the goalBks stack */
	goalBksCount = oldGoalBksCount;

	/*
	 * Add backward branches targets to goal stack. The idea is that
	 * if we are in a loop, we also want to include the backward
	 * branch targets in our ordering goals. That way locals that
	 * don't have an order imposed on by successor blocks can
	 * smartly order the locals that are also referenced by the
	 * loop dominator, which has already had some order imposed
	 * on it by the block that did the backward branch to it.
	 */

	/* Pop current block if backward branch target in goalBks */
	if (goalBksCount > 0 && goalBks[goalBksCount-1] == bk) {
	    goalBksCount--;
            CVMtraceJITRegLocals(("Popping blockID(%d)\n",
                                  CVMJITirblockGetBlockID(bk)));
	}

	/* Add any successor blocks that are backward branch targets */
	for (bkIdx = bk->successorBlocksCount - 1; bkIdx >= 0; bkIdx--) {
	    CVMJITIRBlock* sbk = bk->successorBlocks[bkIdx];
	    if (CVMJITirblockIsBackwardBranchTarget(sbk)) {
		if (goalBksCount == CVMJIT_MAX_BB_BLOCKS_IN_GOAL_STACK) {
		    /* at least handle the inner most backwards branch */
		    goalBksCount--;
		}
		goalBks[goalBksCount++] = sbk;
                CVMtraceJITRegLocals(("Pushing blockID(%d)\n",
                                      CVMJITirblockGetBlockID(sbk)));
	    }
	}
	
	bk = CVMJITirblockGetPrev(bk);
    }

    /*
     * Now make one more pass over all the blocks. This time in the
     * forward direction. The goal in this pass is to move unordered
     * locals to a slot that doesn't conflict with a local in the same
     * slot in the target block.  This happens, for example, when two
     * consecutive blocks both have one incoming local, and the first
     * block's local doesn't flow to the 2nd block. They'll each
     * end up in the same slot in this case.  When code is generated,
     * this can cause the first block's local to be spilled in order to
     * branch to the 2nd block.
     *
     * The way to avoid this is to iterate over each block in the forward
     * direction. For each incoming local, check if it is in a slot
     * that conflicts with a local in the same slot in any of the successor
     * blocks. If it does, and the local in either the source block
     * or successor is not ordered, then relocate one of them if possible.
     */
    bk = (CVMJITIRBlock*)CVMJITirlistGetHead(&con->bkList);
    while (bk != NULL) {
	int i, j, k;
	
	/* Look for an unordered local in the block */
	for (i = 0; i < CVMJIT_MAX_INCOMING_LOCALS; i++) {
	    int maxScore = 0;
	    int bestSlot = -1;

	    /* if ordered or empty, then there's nothing to do */
	    if (localAlreadyOrdered(bk, i) || bk->incomingLocals[i] == NULL) {
		continue;
	    }
	    
	    /* Look at every slot in every successor block for a slot
	     * that does not contain an ordered local.
	     */
	    for (j = 0; j < CVMJIT_MAX_INCOMING_LOCALS; j++) {
		int score = 0;
		/* If the slot is ordered in the current block, then we
		 * can't possibly swap with it. */
		if (localAlreadyOrdered(bk, j)) {
		    continue;
		}
		/* Count the number of consecutive successor blocks for
		 * which the slot does not contain an ordered local */
		for (k = 0; k < bk->successorBlocksCount; k++) {
		    CVMJITIRBlock* succBk = bk->successorBlocks[k];
		    if (localAlreadyOrdered(succBk, j)) {
			break;
		    }
		    score++;
		}
		/* Keep track of which slot will work the best */
		if (score > maxScore) {
		    maxScore = score;
		    bestSlot = j;
		}
	    }
	    
	    if (bestSlot == -1) {
		continue; /* no good candidates */
	    }
	    
	    /* Swap local "i" in block "bk" to the best slot we found. */
	    CVMtraceJITRegLocalsExec({dumpIncomingLocals(bk, "pre ");});
	    {
		CVMJITIRNode* tmp = bk->incomingLocals[bestSlot];
		bk->incomingLocals[bestSlot] = bk->incomingLocals[i];
		bk->incomingLocals[i] = tmp;
		CVMtraceJITRegLocals((
				      "local(#%d) swapped with local(#%d)\n",
				      i, bestSlot));
	    }
	    /* this slot is now ordered. */
	    bk->orderedIncomingLocals |= (1 << bestSlot);
	    /* we may have swapped with NULL node at end of the array */
	    if (bestSlot >= bk->incomingLocalsCount) {
		bk->incomingLocalsCount = bestSlot + 1 ;
	    }
	    CVMtraceJITRegLocalsExec({dumpIncomingLocals(bk, "post");});
	    
	    /* For each successor block, find an empty slot that we
	     * can swap with so unordered local "i" in block "bk" no
	     * longer conflicts with a local in the successor block. */
	    for (k = 0; k < bk->successorBlocksCount; k++) {
		CVMJITIRBlock* succBk = bk->successorBlocks[k];
		/* don't mess with successor blocks with phis */
		if (succBk->phiCount > 0) {
		    continue;
		}
		/* no need or doing any ordering if there are no locals */
		if (succBk->incomingLocalsCount == 0) {
		    continue;
		}
		/* if bestSlot already empty, then no swapping is needed */
		if (succBk->incomingLocals[bestSlot] == NULL) {
		    /* mark slot ordered so we never move anything into it. */
		    succBk->orderedIncomingLocals |= (1 << bestSlot);
		    /* bestSlot might be after last local */
		    if (bestSlot >= succBk->incomingLocalsCount) {
			succBk->incomingLocalsCount = bestSlot + 1 ;
		    }
		    continue;
		}
		for (j = 0; j < CVMJIT_MAX_INCOMING_LOCALS; j++) {
		    if (localAlreadyOrdered(succBk, j) ||
			succBk->incomingLocals[j] != NULL) {
			continue; /* can't use this slot */
		    }
		    CVMtraceJITRegLocalsExec({
			dumpIncomingLocals(succBk, "pre2 ");});
		    /* move to empty slot */
		    succBk->incomingLocals[j] =
			succBk->incomingLocals[bestSlot];
		    succBk->incomingLocals[bestSlot] = NULL;
		    /* mark slot ordered so we never move anything into it. */
		    succBk->orderedIncomingLocals |= (1 << bestSlot);
		    /* we may have swapped with NULL at end of the array */
		    if (j >= succBk->incomingLocalsCount) {
			succBk->incomingLocalsCount = j + 1 ;
		    }
		    CVMtraceJITRegLocals((
			"local(#%d) swapped with local(#%d)\n",
			j, bestSlot));
		    CVMtraceJITRegLocalsExec({
			dumpIncomingLocals(succBk, "post2");});
		    break;
		}
	    }
	    
	    /* Retry the current index since we just swapped it */
	    i--;
	}
	bk = CVMJITirblockGetNext(bk);
    }
}

/*
 * Find localNo in the incomingLocals[] of bk. Returns the incomingLocals[]
 * index if found, -1 if not found.  */
CVMInt32
CVMJITirblockFindIncomingLocal(CVMJITIRBlock* bk, CVMUint16 localNo)
{
    int i;
    for (i = 0; i < bk->incomingLocalsCount; i++) {
	CVMJITIRNode* localNode = bk->incomingLocals[i];
	if (localNode == NULL) {
	    continue;
	}
	if (getLocalNo(localNode) == localNo) {
	    return i;
	}
    }
    return -1;
}

/* Returns true if the block has seen an ASSIGN node for the local */
static CVMBool
assignNodeExists(CVMJITIRNode** assignNodes, CVMUint16 assignNodesCount,
		 CVMUint16 localNo)
{
    int i;
    for (i = 0; i < assignNodesCount; i++) {
	CVMJITIRNode* localNode = CVMJITirnodeGetLeftSubtree(assignNodes[i]);
	if (localNo == getLocalNo(localNode)) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

/* Add target block to list of blocks we flow incoming locals from */
static void
addIncomingLocalsSuccessorBlock(CVMJITCompilationContext* con,
				CVMJITIRBlock* curbk,
				CVMJITIRBlock* successorBk,
				CVMBool fallthrough)
{
    int count = curbk->successorBlocksCount;

    /* 
     * Don't add any more blocks if we aren't allowing anymore incoming
     * locals, which usually means we've seen a method call or we are
     * a special block of some sort that does not allow incomingLocals.
     */
    if (curbk->noMoreIncomingLocals) {
	/* NOTE: The successor block is still useful for targeting
	 * outgoing locals, so add it anyway. */
#if 0
       	return;
#endif
    }

    if (curbk == successorBk) {
	/* NOTE: The successor block is still useful for targeting
	 * outgoing locals, so add it anyway. */
#if 0
	return; /* no need to add yourself */
#endif
    }

    /* Make sure we haven't added this block already */
    {
	int i;
	for (i = 0; i < count; i++) {
	    if (curbk->successorBlocks[i] == successorBk) {
		return;
	    }
	}
    }

    if (count == CVMJIT_MAX_SUCCESSOR_BLOCKS) {
	if (!fallthrough) {
	    return;  /* we've filled up the successorBlocks array */
	}
	/* always allow fallthrough block to be added */
	count--;
    }

    /* Add the successor block */
    curbk->successorBlocks[count] = successorBk;
    curbk->successorBlocksLocalOrder[count] = curbk->incomingLocalsCount;
    curbk->successorBlocksAssignOrder[count] = con->assignNodesCount;
    curbk->okToMergeSuccessorBlock[count] = !curbk->noMoreIncomingLocals;
    curbk->okToMergeSuccessorBlockRefs[count] =
	!curbk->noMoreIncomingRefLocals;
    curbk->successorBlocksCount = count + 1;
}

#endif /* CVM_JIT_REGISTER_LOCALS */

/*
 * Do IR generation: 
 *
 * We start translating with the block at pc 0.
 *
 * During translation the current block, we enqueue all untranslated
 * target blocks (successors) for translation.
 *
 * We translate blocks until the translation queue is empty.
 *
 * Any unreachable block *including* exception handler blocks will be
 * eliminated.
 *
 */

static void
secondPass(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* currBlock;
    CVMJITMethodContext* mc = con->mc;

    {
	/* Enqueue first block */
	CVMJITIRBlock *firstBlock = mc->pcToBlock[0];
	CVMassert(firstBlock != NULL);
	CVMJITirblockEnqueue(&con->transQ, firstBlock);

	/* and set the state of its ref-based locals */
	setLocalsStateAtMethodEntry(con, firstBlock);
    }

    /* Traverse block stack constructed in first pass */
    do {
	while (!CVMJITirblockQueueEmpty(&con->transQ)) {
	    /* Dequeue block */
	    CVMJITirblockDequeue(&con->transQ, currBlock);

	    CVMassert(!CVMJITirblockIsTranslated(currBlock));

	    /* Translate bytecode instruction to IR */
	    translateBlock(con, currBlock);

	    mc = con->mc;
	}
    } while (CVM_FALSE);

    /* Process missed PC mappings in unreachable blocks */
    processUnreachedBlocks(con);

    CVMJITirlistAppendList(con, &con->bkList, &con->artBkList);

#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
    dumpLocalInfoForBlocks(con);
#endif

    if (!CVMJITirblockQueueEmpty(&con->refQ)) {
	refineLocalsInfo(con);
#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
	CVMconsolePrintf("REFINED!\n");
	dumpLocalInfoForBlocks(con);
#endif
    }

#ifdef CVM_JIT_REGISTER_LOCALS
    if (CVMglobals.jit.registerLocals) {
	CVMJITIRBlock *lastBlock =
	    (CVMJITIRBlock*)CVMJITirlistGetHead(&con->bkList);
	while (CVMJITirblockGetNext(lastBlock) != NULL) {
	    lastBlock = CVMJITirblockGetNext(lastBlock);
	}
	/*
	 * Merge incoming locals from each block's successor block. Doing this
	 * twice should fully propogate all incoming locals as long as
	 * backwards branches aren't chained, which is probably always the
	 * case with javac generated code. If they are chained, then the only
	 * harm is possibly suboptimal code.
	 */
	mergeIncomingLocalsFromSuccessorBlocks(con, lastBlock, CVM_TRUE);
	mergeIncomingLocalsFromSuccessorBlocks(con, lastBlock, CVM_FALSE);
	/*
	 * Now order the incoming locals for each block in a way that
	 * minimizes register movement as locals flow from one blocok
	 * to the next.
	 */
	orderIncomingLocals(con, lastBlock);
    }
#endif /* CVM_JIT_REGISTER_LOCALS */
}

/* 
 * Iterate through the IR, follow flow of locals information to possible
 * branch targets, and see if all this changes the local state of any 
 * of the targets.
 *
 * In the process, update LOCALREFS node with refined locals info.
 */
static CVMBool
blockCodeChangesLocalInfo(CVMJITCompilationContext* con,
			  CVMJITIRBlock* block)
{
    CVMBool change = CVM_FALSE; /* until further notice */
    CVMUint16 javaPC; /* Last known corresponding java pc */
    CVMJITIRRoot* root = 
        (CVMJITIRRoot*)CVMJITirlistGetHead(CVMJITirblockGetRootList(block));
    CVMJITIRRange *r = NULL;
    CVMJITIRRange *n = block->firstRange;
    CVMJITMethodContext *mc = n->mc;
    CVMJavaMethodDescriptor* jmd = mc->jmd;
    CVMUint8* code     = CVMjmdCode(jmd);
    CVMJITSet* curLocalRefs = &con->curRefLocals;

    /* This might be set back to TRUE below. */
    CVMJITirblockClearFlags(block, CVMJITIRBLOCK_LOCALREFS_INFO_CHANGED);
    CVMJITlocalrefsCopy(con, curLocalRefs, &block->localRefs);

    CVMassert(mc == block->inMc || block->targetMb != NULL);

    mc->currBlock = block;
    con->mc = mc;

    javaPC = n->startPC;

    while (root != NULL) {
	CVMJITIRNode* expr = CVMJITirnodeGetRootOperand(root);
	
	/*
	 * Ugly.  We really should store the roots in the CVMJITIRRange,
	 * then use an iterator to get the next range and root node.
	 */
	while (n != NULL && n->firstRoot == NULL) {
	    n = n->next;
	}
	if (n != NULL && root == n->firstRoot) {
	    mc = n->mc;
	    code = CVMjmdCode(mc->jmd);
	    r = n;
	    n = r->next;
	    con->mc = mc;
	}
	CVMassert(r != NULL);

        /* If this expression can throw an exception, then we need to
           propagate the local refs info from the corresponding mapPC to the
           excpetion handler blocks: */
        if (CVMJITirnodeThrowsExceptions(expr)) {
            if (connectFlowToExcHandlers(con, mc, javaPC, CVM_FALSE)) {
                change = CVM_TRUE;
            }
        }

	if (CVMJITirnodeIsMapPcNode(expr)) {
	    javaPC = expr->type_node.mapPcNode.javaPcToMap;
	    CVMassert(javaPC >= r->startPC);
	    CVMassert(javaPC <= r->endPC);
	} else if (CVMJITirnodeIsAssign(expr)) {
	    CVMJITIRNode* lhs;
	    /* Check if it is an assignment into a local */
	    CVMassert(CVMJITirnodeIsBinaryNode(expr));
	    lhs = CVMJITirnodeGetLeftSubtree(expr);
	    if (CVMJITirnodeIsLocalNode(lhs)) {
		CVMUint32 localNo = CVMJITirnodeGetLocal(lhs)->localNo;
		if (CVMJITirnodeIsReferenceType(expr)) {
		    CVMJITlocalrefsSetRef(con, curLocalRefs, localNo);
		} else {
		    CVMJITlocalrefsSetValue(con, curLocalRefs, localNo, 
			CVMJITirnodeIsDoubleWordType(lhs));
		}
	    }
	} else if (CVMJITirnodeIsBranchNode(expr)) {
	    CVMJITIRBlock* target = CVMJITirnodeGetBranchOp(expr)->branchLabel;
	    if (CVMJITirblockMergeLocalRefs(con, target)) {
		change = CVM_TRUE;
	    }
	} else if (CVMJITirnodeIsCondBranchNode(expr)) {
	    CVMJITIRBlock* target = CVMJITirnodeGetCondBranchOp(expr)->target;
	    if (CVMJITirblockMergeLocalRefs(con, target)) {
		change = CVM_TRUE;
	    }
	} else if (CVMJITirnodeIsLookupSwitchNode(expr)) {
	    int i;
	    CVMJITLookupSwitch* ls = CVMJITirnodeGetLookupSwitchOp(expr);
	    if (CVMJITirblockMergeLocalRefs(con, ls->defaultTarget)) {
		change = CVM_TRUE;
	    }
	    for (i = 0; i < ls->nPairs; i++) {
		if (CVMJITirblockMergeLocalRefs(con, ls->lookupList[i].dest)) {
		    change = CVM_TRUE;
		}
	    }
	} else if (CVMJITirnodeIsTableSwitchNode(expr)) {
	    int i;
	    CVMJITTableSwitch* ts = CVMJITirnodeGetTableSwitchOp(expr);
	    if (CVMJITirblockMergeLocalRefs(con, ts->defaultTarget)) {
		change = CVM_TRUE;
	    }
	    for (i = 0; i < ts->nElements; i++) {
		if (CVMJITirblockMergeLocalRefs(con, ts->tableList[i])) {
		    change = CVM_TRUE;
		}
	    }
	} else if (CVMJITirnodeIsRetNode(expr)){
	    CVMUint32 targetPC;
	    CVMJITIRNode* lhs;
	    CVMUint32 localNo;
	    CVMJITJsrRetEntry* entry;
	    int nFound;

	    lhs = CVMJITirnodeGetLeftSubtree(expr);
	    lhs = CVMJITirnodeValueOf(lhs);
	    /*
	     * Subexpression is either a local or a JSR_RETURN_ADDRESS
	     * One way or the other find the target PC of the subroutine
	     * from which this returns.
	     */
	    if (CVMJITirnodeIsLocalNode(lhs)){
		localNo = getLocalNo(lhs);
		targetPC = CVMJITlocalStateGetPC(mc, localNo);
	    } else {
		CVMassert(CVMJITirnodeIsJsrReturnAddressNode(lhs));
		targetPC = CVMJITirnodeGetNull(lhs)->data;
	    }
	    /*
	     * Find all return points corresponding to jsr's corresponding to
	     * this ret and propagate state.
	     */
	    nFound = 0;
	    for (entry = (CVMJITJsrRetEntry*)con->jsrRetTargetList.head; 
		 entry != NULL; entry = CVMJITjsrRetEntryGetNext(entry)) {
		if (entry->jsrTargetPC == targetPC) {
		    nFound += 1;
		    if (CVMJITirblockMergeLocalRefs(con, entry->jsrRetBk)) {
			change = CVM_TRUE;
		    }
		}
	    }
	    CVMassert(nFound > 0);
	}
        root = CVMJITirnodeGetRootNext(root);
    }
    /* This block might fall through. Propagate to successor if so */
    if (CVMJITirblockFallsThru(block)) {
	if (CVMJITirblockMergeLocalRefs(con, CVMJITirblockGetNext(block))) {
	    change = CVM_TRUE;
	}
    }
    return change;
}

/*
 * The locals information we obtained is not entirely accurate. Start again,
 * and do an iterative analysis to refine the choices. Stop when there are no
 * more changes, and there is convergence.
 */
static void
refineLocalsInfo(CVMJITCompilationContext* con)
{
    CVMJITIRBlock* block;

    while (!CVMJITirblockQueueEmpty(&con->refQ)) {
	/* Dequeue block */
	CVMJITirblockDequeue(&con->refQ, block);
    
	CVMassert(CVMJITirblockLocalRefsInfoChanged(block));

#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
	CVMconsolePrintf("REFINING BLOCK ID:%d\n",
	    CVMJITirblockGetBlockID(block));
#endif
	if (blockCodeChangesLocalInfo(con, block)) {
#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
	    dumpLocalInfoForBlocks(con);
#endif
	}
    }
}

static void
translateMethod(CVMJITCompilationContext* con)
{
    /* Initialize transQ, the flow order queue of compilation */
    CVMJITirblockQueueInit(&con->transQ);

    /* Initialize refQ, the pending refinement queue */
    CVMJITirblockQueueInit(&con->refQ);

    /*
     * The first context looks a lot like the "top" method being compiled
     * No return address there
     */
    pushMethodContext(con, con->mb, InlineStoresParameters, CVM_FALSE);

    /* Second pass to traverse the block stack and translate IR in
       each block */
    secondPass(con);
}

CVMBool
CVMJITcompileBytecodeToIR(CVMJITCompilationContext* con)
{
    translateMethod(con);
    
    CVMtraceJITBCTOIRExec(CVMJITirdumpIRListPrint(con, &(con->bkList)));

    if (con->hasRet) {
        CVM_COMPILEFLAGS_LOCK(con->ee);
        CVMmbCompileFlags(con->mb) |= CVMJIT_HAS_RET;
        CVM_COMPILEFLAGS_UNLOCK(con->ee);
    }
    if (con->maxInliningDepth > 0) {
	con->inliningStack = CVMJITmemNew(con, JIT_ALLOC_CGEN_OTHER,
	     sizeof con->inliningStack[0] * con->maxInliningDepth);
    }

    return CVM_TRUE;
}

/*
 * Push a new method translation context
 */
static CVMJITMethodContext*
pushMethodContext(CVMJITCompilationContext* con, 
		  CVMMethodBlock* mb,
		  Inlinability inlineStatus,
		  CVMBool doNullCheck)
{
    CVMJITMethodContext *mc0 = con->mc;
    CVMJITMethodContext *mc;
    CVMInt32 declaredLocals;
    
    CVMassert(inlineStatus != InlineNotInlinable);

    mc = CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER, sizeof *mc);

    memset(mc, 0, sizeof(CVMJITMethodContext)); /* Making sure */
    mc->mb   = mb;
    mc->jmd  = CVMmbJmd(mb);
    mc->cb   = CVMmbClassBlock(mb);
#ifdef CVM_JVMTI
    if (CVMjvmtiMbIsObsolete(mb)) {
	mc->cp = CVMjvmtiMbConstantPool(mb);
	if (mc->cp == NULL) {
	    mc->cp = CVMcbConstantPool(mc->cb);
	}	
    } else
#endif
    {
	mc->cp   = CVMcbConstantPool(mc->cb);
    }
    mc->code = CVMjmdCode(mc->jmd);
    mc->codeEnd = mc->code + CVMjmdCodeLength(mc->jmd);
    mc->retBlock = NULL;

    mc->hasExceptionHandlers = CVMjmdExceptionTableLength(mc->jmd);

    /*
       Inlined method
     */
    if (mc0 != NULL) {
        CVMJITStack *operandStack = con->operandStack;
        CVMUint16 argSize = CVMmbArgsSize(mb);
        CVMUint32 argCount;
	CVMUint32 ownLocals;

	mc->compilationDepth = mc0->compilationDepth + 1;

        /* Compute the starting stack index (exclude args): */
        argCount = argSize2argCount(con, argSize);
        mc->startStackIndex = CVMJITstackCnt(con, operandStack) - argCount;

	/* Code gets bigger */
	con->codeLength += CVMjmdCodeLength(mc->jmd);

	/* And we add some more locals */

	mc->firstLocal = mc0->currLocalWords;
	ownLocals = CVMmbMaxLocals(mb);

	if (!CVMmbIs(mb, SYNCHRONIZED) || CVMmbIs(mb, STATIC) ||
	    ((inlineStatus & InlineStoresParameters) == 0))
	{
	    mc->syncObject = 0;
	} else {
	    /*
	       We really should check for local 0, but for now just
	       assume that if any parameter is written, so is "this".
	     */
	    ++ownLocals;
	    mc->syncObject = ownLocals - 1;
	}
	
	/* And stack capacity gets deeper */
	mc->currCapacity = mc0->currCapacity +
	    CVMjmdMaxStack(mc->jmd) + ownLocals;

	mc->nOwnLocals = ownLocals;
	mc->currLocalWords = mc0->currLocalWords + ownLocals;

	CVMtraceJITInlining((
	    ":: Pushing method context with %d own local(s) and depth %d\n",
	    ownLocals, mc->compilationDepth));

	/* Limits */
	/* The inlinability check already handles depth. So if we got
	   here, we'd better be under maximum allowed stack */
	CVMassert(mc->currCapacity <= con->maxCapacity);
	CVMassert(con->numberLocalWords <= con->maxLocalWords);
	if (mc->compilationDepth > con->maxInliningDepth) {
	    con->maxInliningDepth = mc->compilationDepth;
	}

	mc->invokePC = mc0->startPC - mc0->code;

	mc->hasExceptionHandlers = mc0->hasExceptionHandlers ||
	    mc->hasExceptionHandlers;

    } else {	/* root context */

	con->rootMc = mc;

	mc->compilationDepth = 0;

	mc->firstLocal = 0;

	mc->currCapacity = CVMjmdCapacity(mc->jmd);
	mc->currLocalWords = mc->nOwnLocals = CVMjmdMaxLocals(con->jmd);
    }

    /* Allocate enough space for locals */
    {
	declaredLocals = mc->nOwnLocals;
	mc->locals = CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
				  2 * declaredLocals * sizeof(CVMJITIRNode*));
        mc->physLocals = &mc->locals[declaredLocals];
	mc->localsSize = declaredLocals;
    }

    if (mc->currCapacity > con->capacity) {
	con->capacity = mc->currCapacity;
    }
    if (mc->currLocalWords > con->numberLocalWords) {
	con->numberLocalWords = mc->currLocalWords;
    }

    mc->caller = mc0;

    CVMJITirlistInit(con, &mc->bkList);
    mc->pcToBlock = CVMJITirblockMapCreate(con, mb); /* create blockMap */
    CVMJITsetInit(con, &mc->labelsSet); /* create label set */
    CVMJITsetInit(con, &mc->gcPointsSet); /* create GC points set */
    CVMJITsetInit(con, &mc->mapPcSet); /* create map pc set */
    CVMJITsetInit(con, &mc->notSeq);
    mc->removeNullChecksOfLocal_0 = !CVMmbIs(mb, STATIC);

    /* First pass to traverse the bytecodes instructions in method block */
    con->mc = mc;

    firstPass(con);

    /* Now we know if there are JSRs in the method. Allocate enough space 
       for localsState used to track JSR information. */       
    {
        if (CVMJITirlistGetCnt(&(con->jsrRetTargetList)) > 0) {
            mc->localsState = CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
                                       2 * declaredLocals * sizeof(CVMUint32));
        }
    }

    mc->preScanInfo = inlineStatus;

    spliceContextBlocks(con, mc, mb, doNullCheck);

    mc->startPC = mc->code;
    mc->endPC = mc->startPC;

    return mc;
}

/* Splice a block list from a method context onto the global
   compilation context */
static void
spliceContextBlocks(CVMJITCompilationContext* con,
    CVMJITMethodContext *mc, CVMMethodBlock *targetMb,
    CVMBool doNullCheck)
{
    if (mc->caller == NULL) {
	CVMJITirlistAppendList(con, &con->bkList, &mc->bkList);
    } else {
	CVMJITIRBlock *curbk = mc->caller->currBlock;

	/* Add the prologue block for an inlined method, where
	   we collect the incoming arguments by assigning
	   them to locals */
	CVMJITIRBlock *collBlock = CVMJITirblockCreate(con, 0);
	CVMJITirlistPrepend(con, &mc->bkList, collBlock);
	/* This is an artifical block */
	CVMJITirblockAddFlags(collBlock, CVMJITIRBLOCK_NO_JAVA_MAPPING);
	collBlock->doNullCheck = doNullCheck;

	/* And remember our return block, which can be NULL */
	mc->retBlock = mc->pcToBlock[mc->codeEnd - mc->code];

	CVMJITirlistAppendListAfter(con, &con->bkList, curbk, &mc->bkList);
    }
}

/* Prologue for an inlined method.
   This is where a monitor enter would go if the method was
   synchronized.
*/
static CVMBool
translateInlinePrologue(CVMJITCompilationContext* con,
    CVMJITIRBlock* currBlock, CVMJITIRBlock* collBlock)
{
    CVMJITMethodContext *mc = con->mc;
    CVMUint16 argSize = CVMmbArgsSize(mc->mb);

    mcCollectParameters(con, currBlock, mc, argSize);
    
    if (collBlock->doNullCheck) {
	CVMBool removeNullChecksOfLocal_0 = mc->removeNullChecksOfLocal_0;
	/* Get 'this' from the context and perform null check */
	CVMJITIRNode* thisNode =
	    getLocal(con, mc, 0, CVM_TYPEID_OBJ);
	CVMJITIRNode* nullCheckNode;

	/* Disable in prologue */
	mc->removeNullChecksOfLocal_0 = CVM_FALSE;

	nullCheckNode =
	    nullCheck(con, currBlock, thisNode, CVM_TRUE);

	if (nullCheckNode != thisNode) {
	    /* Connect flow and flush all locals before possibly
	       throwing an exception in the invocation code
	       coming up below: */
	    connectFlowToExcHandlers(con, mc,
		mc->startPC - mc->code, CVM_TRUE);
	    CVMassert(CVMJITirnodeHasSideEffects(nullCheckNode));
	    CVMJITirForceEvaluation(con, currBlock, nullCheckNode);
	}
	/* Restore to previous value */
	mc->removeNullChecksOfLocal_0 = removeNullChecksOfLocal_0;
    }

    if (CVMmbIs(mc->mb, SYNCHRONIZED)) {

	CVMassert(!CVMmbIs(mc->mb, STATIC));

	{
	    CVMJITIRNode *node = monitorEnter(con, currBlock,
		getLocal(con, mc, 0, CVM_TYPEID_OBJ));
	    CVMJITirnodeNewRoot(con, currBlock, node);
	}

	{
	    CVMJITirnodeStackPush(con, mc->locals[0]);
	    mcStoreValueToLocal(con, currBlock,
		mc->syncObject,
		CVM_TYPEID_OBJ, mc, CVM_TRUE);

	}
    }

    beginInlining(con, currBlock, CVMJIT_ENCODE_BEGIN_INLINING, mc);

    return CVM_TRUE;
}

/* Perform an out-of-line invocation for an inlined method.  We have
   to add the BEGIN/END_INLINING nodes, because we are at the end
   of the method.  It would be nice if we could place the out-of-line
   code closer to the code for the calling context, so we can use
   less inline info entries.
*/
static CVMBool
translateOutOfLineInvoke(CVMJITCompilationContext* con,
    CVMJITIRBlock* currBlock)
{
    CVMMethodBlock *mb = currBlock->targetMb;
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    CVMJITIRNode *cbNode, *methodSpecNode;	
    CVMJITIRNode *candidateMbNode, *cmpMbNode;
#endif
/* IAI - 20 */
    CVMJITIRNode *mbNode, *plist;
    CVMUint8 rtnType = CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb));
    CVMUint16 argSize = CVMmbArgsSize(mb);
    CVMUint16 pc = CVMJITirblockGetBlockPC(currBlock);
    CVMBool isOOLInterface = CVMJITirblockIsOOLInvokeInterface(currBlock);

    {
	CVMJITMethodContext *rootMc = con->rootMc;
	CVMJITMethodContext *mc = currBlock->inMc;
	int depth = mc->compilationDepth;
	int i;
	CVMJITMethodContext **mcs;

	if (depth == 0) {
	    CVMassert(con->mc == rootMc);
	    pushNewRange(con, currBlock, pc);
	    createMapPcNodeIfNeeded(con, currBlock, pc, CVM_TRUE);
	} else {
	    mcs = (CVMJITMethodContext **)CVMJITmemNew(con,
		JIT_ALLOC_IRGEN_OTHER,
		depth * sizeof mcs[0]);

	    i = depth;
	    while (mc != rootMc) {
		mcs[--i] = mc;
		mc = mc->caller;
	    }
	    CVMassert(i == 0);

	    CVMassert(mcs[0]->caller == rootMc);

	    for (i = 0; i < depth; ++i) {
		/* need to map this out-of-line code */
		mc = mcs[i];

		con->mc = mc->caller;
		pushNewRange(con, currBlock, mc->invokePC);
		createMapPcNodeIfNeeded(con, currBlock,
		    mc->invokePC, CVM_TRUE);

		beginInlining(con, currBlock,
		    CVMJIT_ENCODE_BEGIN_INLINING, mc);

	    }
	    con->mc = mc;
	    pushNewRange(con, currBlock, pc);
	    createMapPcNodeIfNeeded(con, currBlock, pc, CVM_TRUE);
	}
    }

#if defined(CVM_DEBUG) || defined(CVMJIT_COUNT_VIRTUAL_INLINE_STATS)
    /*
     * Mark inlining info for stats gathering
     */
    {
	CVMJITIRNode* mbConst = CVMJITirnodeNewConstantMB(con, mb);
	CVMJITIRNode* info = CVMJITirnodeNewUnaryOp(con,
	    CVMJIT_ENCODE_OUTOFLINE_INVOKE, mbConst);
	CVMJITirnodeSetHasUndefinedSideEffect(info);
	CVMJITirnodeNewRoot(con, currBlock, info);
    }
#endif
    /* The topmost item must be the vtable access result */
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    if (currBlock->mtIndex == CVMJIT_IAI_VIRTUAL_INLINE_CB_TEST_DEFAULT)
#endif
/* IAI - 20 */
    {
        mbNode = CVMJITirnodeStackPop(con);
        plist = collectParameters(con, currBlock, argSize);
    }
/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    else {
	CVMUint16 nodeTag;
	cbNode = CVMJITirnodeStackPop(con);
	plist = collectParameters(con, currBlock, argSize);
	methodSpecNode = CVMJITirnodeNewConstantMethodTableIndex(
                               con, currBlock->mtIndex);

        nodeTag = CVMJIT_TYPE_ENCODE(CVMJIT_FETCH_MB_FROM_VTABLE_OUTOFLINE,
                                     CVMJIT_BINARY_NODE,
                                     CVMJIT_TYPEID_ADDRESS);
        mbNode = CVMJITirnodeNewBinaryOp(con, nodeTag, cbNode,
                                         methodSpecNode);
	/* And our guess mb*/
        candidateMbNode = CVMJITirnodeNewConstantMB(
                                con, currBlock->candidateMb);
        nodeTag = CVMJIT_TYPE_ENCODE(CVMJIT_MB_TEST_OUTOFLINE,
                                     CVMJIT_BINARY_NODE,
                                     CVMJIT_TYPEID_ADDRESS);;
	cmpMbNode = CVMJITirnodeNewBinaryOp(con, nodeTag, mbNode,
                                            candidateMbNode);
	CVMJITirnodeNewRoot(con, currBlock, cmpMbNode);
    }
#endif
/* IAI - 20 */
    /* No need to do null check. That's already been done */
    invokeMethod(con, currBlock, rtnType, plist, argSize, CVM_FALSE, mbNode);

    {
	int i;
	int depth = currBlock->inMc->compilationDepth;

	for (i = 0; i < depth; ++i) {
	    endInlining(con, currBlock, NULL);
	}
    }

    /* Goto merge point after the invocation */
    if (gotoBranch(con, currBlock, pc + (isOOLInterface? 5:3), CVMJIT_GOTO)) {
	CVMassert(CVM_FALSE);
    }

    return CVM_FALSE;
}
