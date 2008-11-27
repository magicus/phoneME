/*
 * @(#)jitirnode.c	1.102 06/10/10
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
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/globals.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirdump.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/porting/doubleword.h"

#include "javavm/include/porting/jit/jit.h"

#include "javavm/include/clib.h"

static void
CVMJITirnodeSetTreeHasBeenEmitted(CVMJITCompilationContext *con,
    CVMJITIRNode *node);

/*
 * CVMJITIRNode interface APIs
 */

#undef MAX
#define MAX(x, y) ((x > y) ? x : y)

#define CVMJITirnodeInheritSideEffects0(node, operand) { \
    (node)->flags |= ((operand)->flags & CVMJITIRNODE_SIDE_EFFECT_MASK); \
}

static void
CVMJITirnodeInheritSideEffects(CVMJITIRNode *node, CVMJITIRNode *operand) {
    if (!CVMJITirnodeHasBeenEvaluated(operand)) {
	CVMJITirnodeInheritSideEffects0(node, operand);
    }
}

static void
CVMJITirnodeBinaryInheritSideEffects(CVMJITIRNode *node,
    CVMJITIRNode *lhs, CVMJITIRNode *rhs)
{
    /* If we really wanted to get clever, we could do this all
       with bit operations and no conditionals. */
    CVMJITirnodeInheritSideEffects(node, lhs);
    CVMJITirnodeInheritSideEffects(node, rhs);
}

static CVMJITIRNode*
cloneExpressionNode(
    CVMJITCompilationContext* con,
    CVMJITIRNode* node)
{
    CVMJITIRNode *newNode;
    /*
    CVMconsolePrintf("cloneExpressionNode:\n");
    CVMJITirdumpIRNode(con, node, 0, "   ");
    CVMconsolePrintf("\n");
    */
    switch (CVMJITgetNodeTag(node)){
    case CVMJIT_NULL_NODE << CVMJIT_SHIFT_NODETAG:
	newNode = CVMJITirnodeNewNull(con, node->tag);
	newNode->type_node.nullOp.data = node->type_node.nullOp.data;
	break;
    case CVMJIT_LOCAL_NODE << CVMJIT_SHIFT_NODETAG:
	newNode = CVMJITirnodeNewLocal(con, node->tag,
					node->type_node.local.localNo);
	break;
    case CVMJIT_UNARY_NODE << CVMJIT_SHIFT_NODETAG:
	newNode = CVMJITirnodeNewUnaryOp(con, node->tag,
					 node->type_node.unOp.operand);
	newNode->type_node.unOp.flags = node->type_node.unOp.flags;
	break;

    case CVMJIT_BINARY_NODE << CVMJIT_SHIFT_NODETAG:
	newNode = CVMJITirnodeNewBinaryOp(con, node->tag,
			 node->type_node.binOp.lhs, node->type_node.binOp.rhs);
	newNode->type_node.binOp.flags = node->type_node.binOp.flags;
	newNode->type_node.binOp.data = node->type_node.binOp.data;
        newNode->type_node.binOp.data2 = node->type_node.binOp.data2;
	break;
    case CVMJIT_PHI0_NODE << CVMJIT_SHIFT_NODETAG:
	if (CVMJITgetOpcode(node) == (CVMJIT_USED << CVMJIT_SHIFT_OPCODE)){
	    newNode = CVMJITirnodeNewUsedOp(con, node->tag,
			  node->type_node.usedOp.spillLocation, 
			  node->type_node.usedOp.registerSpillOk);
	    break;
	}
    default:
#ifdef CVM_DEBUG
	CVMconsolePrintf(">>cloneExpressionNode: don't know about...\n");
	CVMtraceJITBCTOIRExec({
	    CVMJITirdumpIRNodeAlways(con, node, 0, "   ");
	    CVMconsolePrintf("\n");
	});
#endif
	newNode = NULL;  /* prevent compiler warning */
	CVMassert(CVM_FALSE);
    }
    return newNode;
}

/* Purpose: Produces a node which effectively gets the value of the specified
            node.  "Producing a node" in this case can mean several things:
            1. If node is an IDENTITY, increment the reference count 
		and return node.
            2. If node is CONSTANT, increment the reference count 
		and return node
	    4. If reference count on node is 0, increment the reference count
		and return the node
            3. Else, clone node, overpaint it with an IDENTITY
	       referencing the clone, increment reference count on node
	       (setting clone's ref count to 1), and return src_node
*/
static void
CVMJITidentity(
    CVMJITCompilationContext *con,
    CVMJITIRNode *node)
{
    CVMJITIRNode *cloneNode = NULL;

    CVMassert(CVMJITirnodeGetRefCount(node) == 2);
    CVMassert(!CVMJITirnodeIsIdentity(node));
    /* Next, check if the node is a constant node: */
    if (CVMJITnodeTagIs(node, CONSTANT)) {
	return;
    /* Else, create a new IDENTITY node in the place of the specified node: */
    } else {
	CVMJITUnaryOp* unOpNode = &node->type_node.unOp;
	cloneNode = cloneExpressionNode(con, node);
	/* paint over current node with IDENTITY node. It will be unary */
	node->tag = (node->tag & CVMJIT_TYPE_MASK) |
			CVMJIT_ENCODE_IDENTITY(0);
	unOpNode->operand = cloneNode;

	/* Just in case we turn a null-check node into an identity node */
	unOpNode->flags &= CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS;

	/* treat as a leaf */
	CVMJITirnodeSetcurRootCnt(node, 1);

	CVMassert(CVMJITirnodeGetRefCount(node) == 2);
	CVMJITirnodeSetRefCount(cloneNode, 1);
	return;
    }
}

/*
 * Decorate the current IDENTITY node with an IdentityDecoration
 */
void
CVMJITidentitySetDecoration(
    CVMJITCompilationContext* con,
    CVMJITIdentityDecoration* dec,
    CVMJITIRNode* expr)
{
    dec->refCount += CVMJITirnodeGetRefCount(expr) - 1;
    /*
     * if expr->tag is an IDENTITY operator, do the occupy, else
     * skip it. Ensure that the EVALUATED bit is set.
     */
    if (CVMJITirnodeIsIdentity(expr)) {
	CVMJITidentDecoration(expr) = (void *)dec;
    }
}

/*
 * Get decoration of the current IDENTITY node
 */
CVMJITIdentityDecoration*
CVMJITidentityGetDecoration(
    CVMJITCompilationContext* con,
    CVMJITIRNode* expr)
{
    if ((expr != NULL) && CVMJITirnodeIsIdentity(expr)) {
	return (CVMJITIdentityDecoration*)CVMJITidentDecoration(expr);
    } else {
	return NULL;
    }
}

static CVMJITIRNode *
CVMJITirnodeCreate(CVMJITCompilationContext* con, CVMUint32 size, CVMUint16 tag)
{
    CVMJITIRNode* node; 
    CVMUint32 nodeSize;
    /* make sure the allocation is large enough to repaint into
     * an identity operator if necessary.
     */
    if (size < sizeof(CVMJITIdentOp))
	size = sizeof(CVMJITIdentOp);
    nodeSize = CVMJIT_IRNODE_SIZE + size;
    node = 
	(CVMJITIRNode*) CVMJITmemNew(con, JIT_ALLOC_IRGEN_NODE, nodeSize);
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
    CVMJITirnodeSetDumpTag(node, CVM_TRUE);
    CVMJITirnodeSetID(con, node);
#endif
    CVMJITirnodeSetTag(node, tag);
    CVMJITirnodeSetRefCount(node, 0); /* Start out with no references */
    CVMJITirnodeSetcurRootCnt(node, 1);
    node->flags = 0;                  /* Initialize the flags. */
    node->regsRequired = 0;
    node->decorationType = CVMJIT_NO_DECORATION;
    CVMJITstatsRecordInc(con, CVMJIT_STATS_TOTAL_NUMBER_OF_NODES);
    return node;
}

/*
 * Assert on the allowed IR nodes before getting the first operand of IR node
 * in CVMJITirnodeGetLeftSubtree().
 */
#ifdef CVM_DEBUG_ASSERTS
void
CVMJITirnodeAssertIsGenericSubNode(CVMJITIRNode* node)
{ 
    CVMassert(CVMJITirnodeIsDefineNode(node) || 
	      CVMJITirnodeIsTableSwitchNode(node) ||
	      CVMJITirnodeIsLookupSwitchNode(node) || 
	      CVMJITirnodeIsCondBranchNode(node) || 
	      CVMJITirnodeIsBinaryNode(node) || 
	      CVMJITirnodeIsUnaryNode(node));
}
#endif

/*
 * Create CVMJITMapPcNode
 * Node used by backend to generate javaPC -> compiledPC  table
 */
CVMJITIRNode*
CVMJITirnodeNewMapPcNode(
    CVMJITCompilationContext* con, 
    CVMUint16 tag,
    CVMUint16 pc,
    CVMJITIRBlock* curbk)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITMapPcNode), tag);
    node->type_node.mapPcNode.javaPcToMap = pc; 
    CVMJITirnodeSetHasUndefinedSideEffect(node);
#ifdef CVM_DEBUG_ASSERTS
    if (!CVMJITirblockIsArtificial(curbk)) {
	CVMassert(pc >= curbk->lastOpenRange->startPC);
	CVMassert(pc >= con->mc->startPC - con->mc->code);
	CVMassert(pc < con->mc->endPC - con->mc->code);
    }
#endif
    return node;
}

#ifdef CVM_DEBUG_ASSERTS
/* Purpose: Checks if every operand with a side effect on the operand stack
            has been evaluated. */
static CVMBool
CVMJITirnodeOperandStackSideEffectsAreFullyEvaluated(
    CVMJITCompilationContext *con)
{
    CVMJITStack *operandStack = con->operandStack;
    CVMUint32 i;

    for (i = 0; i < CVMJITstackCnt(con, operandStack); i++) {
        CVMJITIRNode *stkNode =
            CVMJITstackGetElementAtIdx(con, operandStack, i);
        if (!CVMJITirnodeHasBeenEvaluated(stkNode) &&
            CVMJITirnodeHasSideEffects(stkNode)) {
            return CVM_FALSE;
        }
    } /* end of for loop */
    return CVM_TRUE;
}
#endif

/*
 * Create CVMJITIRRoot
 * Only root node is appended into the root list within a block,
 * we append it right after the root node is created.
 */
CVMJITIRRoot*
CVMJITirnodeNewRoot(CVMJITCompilationContext* con,
                    CVMJITIRBlock* curbk, CVMJITIRNode* expr)
{
    CVMJITIRRoot* root = 
        (CVMJITIRRoot*)CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
                                    sizeof(CVMJITIRRoot));
    root->next = NULL;
    root->prev = NULL;
    root->expr = expr;

    /* Make sure that operands on the operand stack have all been evaluated
       before inserting a root node: */
    /* TBD: Is it possible to resolve the following so that there is no
            exception to the rule? */
    CVMassert(
        (CVMJITgetOpcode(expr) ==
            (CVMJIT_END_INLINING << CVMJIT_SHIFT_OPCODE)) ||
        (CVMJITgetOpcode(expr) ==
            (CVMJIT_OUTOFLINE_INVOKE << CVMJIT_SHIFT_OPCODE)) ||
        con->operandStackIsBeingEvaluated ||
        CVMJITirnodeOperandStackSideEffectsAreFullyEvaluated(con));

    CVMJITirlistAppend(con, CVMJITirblockGetRootList(curbk), (void*)root);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_ROOT_NODES);

    if (curbk->lastOpenRange->firstRoot == NULL) {
	curbk->lastOpenRange->firstRoot = root;
    }

    /* Keep track of the maximum number of nodes per root node. */
    if (expr->curRootCnt > con->saveRootCnt) {
	con->saveRootCnt = expr->curRootCnt;
    }

    CVMassert(!CVMJITirnodeHasBeenEmitted(expr));
    CVMJITirnodeSetTreeHasBeenEmitted(con, expr);

    return root;
}

/*
 * Create CVMJITConstant
 */

CVMJITIRNode*
CVMJITirnodeNewConstant32(CVMJITCompilationContext* con, CVMUint16 tag, 
			  CVMUint32 v32)
{
    CVMJITIRNode* node =
	CVMJITirnodeCreate(con, sizeof(CVMJITConstant32), tag);
    node->type_node.constant32.v32 = v32;
    /* By definition, a constant node is already evaluated from the start: */
    CVMJITirnodeSetHasBeenEvaluated(node);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_CONST_NODES);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewConstantAddr(CVMJITCompilationContext* con, CVMUint16 tag, 
			  CVMAddr vAddr)
{
    CVMJITIRNode* node =
	CVMJITirnodeCreate(con, sizeof(CVMJITConstantAddr), tag);
    node->type_node.constantAddr.vAddr = vAddr;
    /* By definition, a constant node is already evaluated from the start: */
    CVMJITirnodeSetHasBeenEvaluated(node);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_CONST_NODES);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewConstant64(CVMJITCompilationContext* con, CVMUint16 tag, 
			  CVMJavaVal64* v64)
{
    CVMJITIRNode* node =
	CVMJITirnodeCreate(con, sizeof(CVMJITConstant64), tag);
    CVMmemCopy64(node->type_node.constant64.j.v, v64->v);
    /* By definition, a constant node is already evaluated from the start: */
    CVMJITirnodeSetHasBeenEvaluated(node);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_CONST_NODES);
    return node;
}

/*
 * Build Constant32 node for the unresolved cp index.
 * Build RESOLVE_REFERENCE node of one of the following types:
 *   CVMJIT_CONST_NEW_CB_UNRESOLVED
 *   CVMJIT_CONST_CB_UNRESOLVED
 *   CVMJIT_CONST_ARRAY_CB_UNRESOLVED
 *   CVMJIT_CONST_GETFIELD_FB_UNRESOLVED
 *   CVMJIT_CONST_PUTFIELD_FB_UNRESOLVED
 *   CVMJIT_CONST_GETSTATIC_FB_UNRESOLVED
 *   CVMJIT_CONST_PUTSTATIC_FB_UNRESOLVED
 *   CVMJIT_CONST_STATIC_MB_UNRESOLVED
 *   CVMJIT_CONST_SPECIAL_MB_UNRESOLVED
 *   CVMJIT_CONST_INTERFACE_MB_UNRESOLVED
 *   CVMJIT_CONST_METHOD_TABLE_INDEX_UNRESOLVED
 * Build Root node pointing to RESOLVE_REFERENCE node at the top.
 * Return the constant node containing the unresolved cp index:
 */
CVMJITIRNode*
CVMJITirnodeNewResolveNode(
    CVMJITCompilationContext* con,
    CVMUint16 cpIndex,
    CVMJITIRBlock* curbk,
    CVMUint8 checkInitFlag,
    CVMUint8 opcodeTag)
{
    CVMJITIRNode *constNode;

    /* build Constant32 node for the cpIndex*/
    constNode = CVMJITirnodeNewConstantCpIndex(con, cpIndex, opcodeTag);

    /* build RESOLVE_REFERENCE node that refers to the cpIndex */
    constNode = CVMJITirnodeNewUnaryOp(con, CVMJIT_ENCODE_RESOLVE_REFERENCE,
                                       constNode);

    /* if the caller passed us a checkinitFlag, then set it. */
    CVMJITirnodeSetUnaryNodeFlag(constNode, checkInitFlag);
    CVMJITirnodeSetThrowsExceptions(constNode);

    /* Make room in the code buffer for the lazy resolution code: */
    con->numLargeOpcodeInstructionBytes += CVMCPU_RESOLVE_SIZE;
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_RESOLVE_NODES);

    /* return the Constant32 node to the caller */
    return constNode;
}

CVMJITIRNode*
CVMJITirnodeNewLocal(CVMJITCompilationContext* con, CVMUint16 tag,
    CVMUint16 localNo) 
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITLocal), tag);
    CVMassert(localNo < con->numberLocalWords);
    node->type_node.local.localNo = localNo;
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewUnaryOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRNode* operand) 
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITUnaryOp), tag);
    CVMJITUnaryOp* unOpNode = &node->type_node.unOp;
    unOpNode->operand = operand;
    CVMJITirnodeSetcurRootCnt(node, operand->curRootCnt + node->curRootCnt);
    CVMJITirnodeInheritSideEffects(node, operand);
    CVMJITirnodeResolveParentThrowsExceptions(node, operand);
    return node;
}

/*
 * Create CVMJITBinaryOp
 */

CVMJITIRNode*
CVMJITirnodeNewBinaryOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRNode* lhs, CVMJITIRNode* rhs)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITBinaryOp), tag);
    CVMJITBinaryOp* binOpNode = &node->type_node.binOp;
    binOpNode->lhs = lhs;
    binOpNode->rhs = rhs;
    CVMJITirnodeSetcurRootCnt(node, MAX(lhs->curRootCnt, rhs->curRootCnt) +
                              node->curRootCnt); 
    CVMJITirnodeBinaryInheritSideEffects(node, lhs, rhs);
    CVMJITirnodeResolveParentThrowsExceptions(node, lhs);
    CVMJITirnodeResolveParentThrowsExceptions(node, rhs);
    return node;
}

/*
 * Create CVMJITBranchOp
 */
CVMJITIRNode*
CVMJITirnodeNewBranchOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRBlock* target_block)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITBranchOp), tag);
    node->type_node.branchOp.branchLabel = target_block;
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewCondBranchOp(CVMJITCompilationContext* con,
			    CVMJITIRNode* lhs, CVMJITIRNode* rhs,
			    CVMUint16 typeTag, CVMJITCondition condition,
			    CVMJITIRBlock* target, int flags)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, 
					    sizeof(CVMJITConditionalBranch),
					    CVMJIT_ENCODE_BCOND(typeTag));
    CVMJITConditionalBranch* condBranchOpNode = &node->type_node.condBranchOp;
    condBranchOpNode->lhs = lhs;
    condBranchOpNode->rhs = rhs;
    condBranchOpNode->target = target;
    condBranchOpNode->condition = condition;
    condBranchOpNode->flags = flags;
    CVMJITirnodeSetcurRootCnt(node, MAX(lhs->curRootCnt, rhs->curRootCnt) +
                              node->curRootCnt); 
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    CVMJITirnodeBinaryInheritSideEffects(node, lhs, rhs);
    CVMJITirnodeResolveParentThrowsExceptions(node, lhs);
    CVMJITirnodeResolveParentThrowsExceptions(node, rhs);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewBoundsCheckOp(CVMJITCompilationContext *con,
			     CVMJITIRNode *indexNode, CVMJITIRNode *lengthNode)
{
    CVMJITIRNode *node;

    /* The indexNode must be evaluated first to maintain evaluation order */
    node = CVMJITirnodeNewBinaryOp(con, CVMJIT_ENCODE_BOUNDS_CHECK,
				   indexNode, lengthNode);
    CVMJITirnodeSetThrowsExceptions(node);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewNull(CVMJITCompilationContext* con, CVMUint16 tag)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, sizeof(CVMJITNull), tag);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_NUMBER_OF_NULL_NODES);
    return node;

}

CVMJITIRNode*
CVMJITirnodeNewUsedOp(CVMJITCompilationContext* con, CVMUint16 tag,
		      CVMInt16 spillLocation,
		      CVMBool registerSpillOk)
{
    CVMJITIRNode* node = 
	CVMJITirnodeCreate(con, sizeof(CVMJITUsedOp), tag);
    CVMJITUsedOp* usedOp = &node->type_node.usedOp;
    usedOp->spillLocation = spillLocation;
    usedOp->registerSpillOk = registerSpillOk;
    usedOp->resource = NULL;
    CVMJITirnodeSetHasBeenEvaluated(node);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewDefineOp(CVMJITCompilationContext* con, CVMUint16 tag,
			CVMJITIRNode* operand, CVMJITIRNode* usedNode)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, 
					    sizeof(CVMJITDefineOp), 
					    tag);
    CVMJITDefineOp* defineOp = &node->type_node.defineOp;
    defineOp->operand = operand;
    defineOp->usedNode = usedNode;

    CVMJITirnodeSetcurRootCnt(node, node->curRootCnt + operand->curRootCnt);

    CVMJITirnodeSetHasUndefinedSideEffect(node);
    CVMJITirnodeResolveParentThrowsExceptions(node, operand);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewPhisListOp(CVMJITCompilationContext* con,
			  CVMUint16 tag,
			  CVMJITIRBlock* targetBlock,
			  CVMJITIRNode** defineNodeList)
{
    CVMJITIRNode* node = 
	CVMJITirnodeCreate(con, sizeof(CVMJITPhisListOp), tag);
    CVMJITPhisListOp* phisListOp = &node->type_node.phisListOp;
    phisListOp->targetBlock = targetBlock;
    phisListOp->defineNodeList = defineNodeList;
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    return node;
}

CVMJITIRNode*
CVMJITirnodeNewLookupSwitchOp(CVMJITCompilationContext* con,
			      CVMJITIRBlock* defaultTarget,
			      CVMJITIRNode* key, CVMUint32 nPairs)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, 
			     sizeof(CVMJITLookupSwitch) + 
			     ((nPairs-1) * sizeof(CVMJITSwitchList)), 
			     CVMJIT_ENCODE_LOOKUPSWITCH);
    CVMJITLookupSwitch* lswitchOpNode = &node->type_node.lswitchOp;
    lswitchOpNode->defaultTarget = defaultTarget;
    lswitchOpNode->key = key;
    lswitchOpNode->nPairs = nPairs;
    CVMJITirnodeSetcurRootCnt(node, key->curRootCnt + node->curRootCnt);
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    CVMJITirnodeResolveParentThrowsExceptions(node, key);
    return node; 
}

CVMJITIRNode*
CVMJITirnodeNewTableSwitchOp(CVMJITCompilationContext* con, 
			     CVMInt32 low, CVMInt32 high, CVMJITIRNode* key, 
			     CVMJITIRBlock* defaultTarget)
{
    CVMJITIRNode* node = CVMJITirnodeCreate(con, 
                             sizeof(CVMJITTableSwitch) +
			     ((high - low + 1) * sizeof(CVMJITIRBlock*)),
			     CVMJIT_ENCODE_TABLESWITCH);
    CVMJITTableSwitch* tswitchOpNode = &node->type_node.tswitchOp;
    tswitchOpNode->defaultTarget = defaultTarget;
    tswitchOpNode->low = low;
    tswitchOpNode->high = high;
    tswitchOpNode->nElements = high - low + 1;
    tswitchOpNode->key = key;
    CVMJITirnodeSetcurRootCnt(node, key->curRootCnt + node->curRootCnt);
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    CVMJITirnodeResolveParentThrowsExceptions(node, key);
    return node;
}

/* Purpose: Sets the lhs operand with the new operand. */
void
CVMJITirnodeSetLeftSubtree(CVMJITCompilationContext* con, 
                           CVMJITIRNode *node, CVMJITIRNode *operand)
{
    CVMJITUnaryOp *unOpNode;
    CVMJITIRNode *oldOp;
    CVMUint16 rootCntAdjust = 0;

    CVMJITirnodeAssertIsGenericSubNode(node);
    unOpNode = &node->type_node.unOp;

    /* Detach the old operand: */
    oldOp = unOpNode->operand;
    CVMassert(oldOp != NULL);
    if (CVMJITnodeTagIs(node, BINARY)) {
        CVMJITBinaryOp *binOpNode = &node->type_node.binOp;
        CVMJITIRNode *rhs = binOpNode->rhs;
        rootCntAdjust = MAX(oldOp->curRootCnt, rhs->curRootCnt);
    } else {
        rootCntAdjust = oldOp->curRootCnt;
    }
    CVMJITirnodeSetcurRootCnt(node, node->curRootCnt - rootCntAdjust);

    /* Attach the new operand: */
    unOpNode->operand = operand;
    if (CVMJITnodeTagIs(node, BINARY)) {
        CVMJITBinaryOp *binOpNode = &node->type_node.binOp;
        CVMJITIRNode *lhs = binOpNode->lhs;
        rootCntAdjust = MAX(lhs->curRootCnt, operand->curRootCnt);
    } else {
        rootCntAdjust = operand->curRootCnt;
    }
    CVMJITirnodeSetcurRootCnt(node, rootCntAdjust + node->curRootCnt);
    CVMJITirnodeInheritSideEffects(node, operand);
}

/* Purpose: Sets the rhs operand with the new operand. */
void
CVMJITirnodeSetRightSubtree(CVMJITCompilationContext* con, 
                            CVMJITIRNode *node, CVMJITIRNode *operand)
{
    CVMJITBinaryOp *binOpNode;
    CVMUint16 rootCntAdjust;
    CVMJITIRNode *lhs;
    CVMJITIRNode *rhs;

    CVMassert(CVMJITnodeTagIs(node, BINARY));
    binOpNode = &node->type_node.binOp;
    lhs = binOpNode->lhs;
    rhs = binOpNode->rhs;
    CVMassert(lhs != NULL);
    CVMassert(rhs != NULL);

    /* Detach the old operand: */
    rootCntAdjust = MAX(lhs->curRootCnt, rhs->curRootCnt);
    CVMJITirnodeSetcurRootCnt(node, node->curRootCnt - rootCntAdjust);

    /* Attach the new operand: */
    binOpNode->rhs = operand;
    CVMJITirnodeSetcurRootCnt(node,
        MAX(lhs->curRootCnt, operand->curRootCnt) + node->curRootCnt);
    CVMJITirnodeInheritSideEffects(node, operand);
}

/* Purpose: Delete the specified binary node and adjust all the refCount of
            its operands accordingly. */
void
CVMJITirnodeDeleteBinaryOp(CVMJITCompilationContext *con, CVMJITIRNode *node)
{
    CVMJITBinaryOp *binOpNode;
    CVMJITIRNode *lhs;
    CVMJITIRNode *rhs;

    CVMassert(CVMJITnodeTagIs(node, BINARY));
    binOpNode = &node->type_node.binOp;
    lhs = binOpNode->lhs;
    rhs = binOpNode->rhs;
    CVMassert(lhs != NULL);
    CVMassert(rhs != NULL);
}

/* Purpose: Delete the specified unary node and adjust all the refCount of
            its operands accordingly. */
void
CVMJITirnodeDeleteUnaryOp(CVMJITCompilationContext *con, CVMJITIRNode *node)
{
    CVMJITUnaryOp *unOpNode;
    CVMJITIRNode *operand;

    CVMassert(CVMJITnodeTagIs(node, UNARY));
    unOpNode = &node->type_node.unOp;
    operand = unOpNode->operand;
    CVMassert(operand != NULL);
}

/* This is an arbitrary limit. */
#define CVM_MAX_BINARY_TREE_DEPTH 64

static void
CVMJITirnodeSetTreeHasBeenEmitted(CVMJITCompilationContext *con,
    CVMJITIRNode *node)
{
    CVMJITIRNode *stk0[CVM_MAX_BINARY_TREE_DEPTH];
    int dstk0[CVM_MAX_BINARY_TREE_DEPTH];
    CVMJITIRNode **stk = stk0;
    int *dstk = dstk0;
    int idx = 0;
    int max_depth = CVM_MAX_BINARY_TREE_DEPTH;
    int depth = 1;
    int max = 0;
    CVMJITIRNode *n = node;
    CVMJITIRNode *p = NULL;

    /* We only care about binary nodes, while curRootCnt gives the tree
       depth, which can only be larger, but it's close enough. */
    if (node->curRootCnt > CVM_MAX_BINARY_TREE_DEPTH) {
	max_depth = node->curRootCnt;
	stk = (CVMJITIRNode **)CVMJITmemNew(con,
	    JIT_ALLOC_IRGEN_OTHER,
	    max_depth * sizeof(CVMJITIRNode *));
	dstk = (int *)CVMJITmemNew(con,
	    JIT_ALLOC_IRGEN_OTHER,
	    max_depth * sizeof(int));
    }

    CVMassert(CVMJITirnodeGetRefCount(node) == 0);

    do {
	CVMJITIRNodeTag tag;

#ifdef CVM_DEBUG_ASSERTS
	if (CVMJITirnodeHasBeenEmitted(n)) {
	    if (CVMJITirnodeIsIdentity(n)) {
		CVMassert(CVMJITirnodeGetRefCount(n) > 1);
	    } else if (!CVMJITnodeTagIs(n, CONSTANT)) {
		CVMassert(CVMJITirnodeGetRefCount(n) == 1);
	    }
	}
#endif

	CVMJITirnodeAddRefCount(n);
	if (CVMJITirnodeGetRefCount(n) == 2 && !CVMJITnodeTagIs(n, CONSTANT)) {
	    CVMJITidentity(con, n);
	}

	if (CVMJITirnodeIsIdentity(n)) {
	    CVMassert(CVMJITirnodeHasBeenEmitted(n));
	    goto leaf;
	}

        CVMJITirnodeSetHasBeenEmitted(n);
        CVMJITirnodeSetHasBeenEvaluated(n);

	tag = CVMJITgetNodeTag(n) >> CVMJIT_SHIFT_NODETAG;
	if (tag >= CVMJIT_BINARY_NODE) {
	    CVMassert(idx < max_depth);
	    dstk[idx] = depth;
	    stk[idx++] = n;
	    p = n;
	    n = CVMJITirnodeGetLeftSubtree(n);
	    ++depth;
	} else if (tag >= CVMJIT_UNARY_NODE) {
	    p = n;
	    n = CVMJITirnodeGetLeftSubtree(n);
	    ++depth;
	} else {
leaf:
	    if (depth > max) {
		max = depth;
	    }
	    if (idx > 0) {
		p = stk[--idx];
		n = CVMJITirnodeGetRightSubtree(p);
		depth = dstk[idx] + 1;
	    } else {
		break;
	    }
	}
    } while (n != NULL);
    node->curRootCnt = max;
}
