/*
 * @(#)jitirblock.c	1.97 06/10/25
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
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/utils.h"
#include "javavm/include/typeid.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/globals.h"
#include "javavm/include/clib.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/system.h"

#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitopt.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitirrange.h"
#include "javavm/include/jit/jitirlist.h"
#include "javavm/include/clib.h"


static void
CVMJITirblockPhiMerge(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk,
		      CVMJITIRNode* expression1ToEvaluate,
		      CVMJITIRNode* expression2ToEvaluate,
		      CVMJITIRNode* branchNode);

static void
CVMJITirblockInit(CVMJITCompilationContext* con, CVMJITIRBlock* bk, 
    CVMUint16 pc)
{
    CVMJITirblockSetBlockPC(bk, pc);
    CVMJITirblockSetBlockID(bk, 0);
    CVMJITirblockSetCseID(bk, 0);
    CVMJITirblockSetRefCount(bk, 0);
    CVMJITirblockSetFlags(bk, 0);
    CVMJITirblockSetNext(bk, NULL);
    CVMJITirblockSetPrev(bk, NULL);
    CVMJITirblockInitRootList(con, bk);
    bk->phiCount = -1;
    bk->phiSize = 0;

    bk->lastOpenRange = NULL;
    bk->inDepth = con->mc->compilationDepth;
    bk->outDepth = bk->inDepth;
    bk->inMc = con->mc;

/* IAI - 20 */
#ifdef IAI_VIRTUAL_INLINE_CB_TEST
    bk->mtIndex = CVMJIT_IAI_VIRTUAL_INLINE_CB_TEST_DEFAULT;
    bk->oolReturnAddress = CVMJIT_IAI_VIRTUAL_INLINE_CB_TEST_DEFAULT;
    bk->candidateMb = NULL;
#endif
/* IAI - 20 */

    CVMassert(bk->sandboxRegSet == 0);

#ifdef CVM_JIT_REGISTER_LOCALS
    if (!CVMglobals.jit.registerLocals) {
	CVMJITirblockNoIncomingLocalsAllowed(bk);
    }
#endif
}

/*
 * Create a new block and set it to the block map, or
 * get an existing block from the block map. 
 * bitset for all branch labels and exception labels.
 */
CVMJITIRBlock*
CVMJITirblockNewLabelForPC0(CVMJITCompilationContext* con, CVMUint16 pc)
{
    CVMJITMethodContext* mc = con->mc;
    CVMJITIRBlock* bk = mc->pcToBlock[pc]; 

    if (bk != NULL) {
        return bk;
    }

    bk = CVMJITirblockCreate(con, pc); 

    /* enter java bytecode pc to the pcToBlock map table 
       bitset for all branch labels and exception labels */ 
    mc->pcToBlock[pc] = bk; 
    CVMJITsetAdd(con, &mc->labelsSet, pc);
    /* Also add the label to the mapPcSet because it may be used as the
       on-ramp location of an OSR operation: */
    CVMJITsetAdd(con, &mc->mapPcSet, pc);
    return bk;
}

CVMJITIRBlock*
CVMJITirblockNewLabelForPC(CVMJITCompilationContext* con, CVMUint16 pc)
{
    CVMJITIRBlock *bk = CVMJITirblockNewLabelForPC0(con, pc);
    CVMJITirblockAddFlags(bk, CVMJITIRBLOCK_IS_BRANCH_TARGET);
    return bk;
}

/*
 * Create a new block and initialize fields in the block.
 */
CVMJITIRBlock*
CVMJITirblockCreate(CVMJITCompilationContext* con, CVMUint16 pc)
{
    CVMJITIRBlock* bk = 
	(CVMJITIRBlock*)CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
				     sizeof(CVMJITIRBlock)); 

    /* Initialize root list and phi list to keep track of root nodes
       phi nodes */
    CVMJITirblockInitRootList(con, bk);

    /* Initialize fields in block */
    CVMJITirblockInit(con, bk, pc);
    CVMJITsetInit(con, &bk->predecessors);
    CVMJITsetInit(con, &bk->successors);
    CVMJITsetInit(con, &bk->dominators);
    CVMJITlocalrefsInit(con, &bk->localRefs);

    CVMJITirblockSetBlockID(bk, con->blockIDCounter++);

    CVMassert(CVMJITirblockGetNext(bk) == NULL);
    CVMassert(CVMJITirblockGetPrev(bk) == NULL);

    return bk;
}

/*
 * Connect flow between the tail block in block list and the new block
 */
static void
CVMJITirblockAddArc(CVMJITCompilationContext* con, CVMJITIRBlock* curbk, 
    CVMJITIRBlock* targetbk)
{
    CVMJITsetAdd(con, &curbk->successors, CVMJITirblockGetBlockID(targetbk));
    CVMJITsetAdd(con, &targetbk->predecessors, CVMJITirblockGetBlockID(curbk));
    CVMJITirblockIncRefCount(targetbk);  
}

#ifdef NOT_USED
/*
 * Disconnect flow between the tail block in block list and the new block
 */
static void
CVMJITirblockRemoveArc(CVMJITCompilationContext* con, 
    CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk)
{
    CVMJITsetRemove(con, &curbk->successors, CVMJITirblockGetBlockID(targetbk));
    CVMJITsetRemove(con, &targetbk->predecessors,
                    CVMJITirblockGetBlockID(curbk));
    CVMJITirblockDecRefCount(targetbk);  
}
#endif

/*
 * Split the current block at 'headPc' by creating a new block
 * that is a successor of the current one 
 */
CVMJITIRBlock*
CVMJITirblockSplit(CVMJITCompilationContext* con,
		   CVMJITIRBlock* curbk,
		   CVMUint16 headPc)
{
    CVMJITIRBlock* newbk = con->mc->pcToBlock[headPc];
    
    CVMassert(curbk == con->mc->currBlock);
    CVMassert(headPc >= curbk->lastOpenRange->startPC);

    /* We don't want to insert the split block if it
       already exists. So first check. */
    if (newbk == NULL) {
	newbk = CVMJITirblockNewLabelForPC0(con, headPc);
	CVMJITirlistInsert(con, &con->bkList, curbk, newbk);
	CVMJITirblockAddArc(con, curbk, newbk);
    }
    return newbk;
}

/*
 * Connect blocks in PC straight-line order into the block list
 */

static void
connectBlocks(CVMJITCompilationContext* con, CVMUint32 elem, void* data)
{
    CVMJITIRBlock* bk = con->mc->pcToBlock[elem];
    CVMJITirlistAppend(con, &con->mc->bkList, (void*)bk);
}

void
CVMJITirblockConnectBlocksInOrder(CVMJITCompilationContext* con)
{
    CVMJITSet* labelsSet = &con->mc->labelsSet;
    CVMJITsetIterate(con, labelsSet, connectBlocks, NULL); 
}

/*
 * Discover all exception handler blocks
 */
void
CVMJITirblockFindAllExceptionLabels(CVMJITCompilationContext* con)
{
    CVMJITMethodContext* mc = con->mc;
    CVMJavaMethodDescriptor* jmd = mc->jmd;
    CVMExceptionHandler* handler = CVMjmdExceptionTable(jmd);
    int i; 

    for (i = CVMjmdExceptionTableLength(jmd); i > 0  ; --i, handler++) {
	CVMUint16 handlerPC = handler->handlerpc;
	CVMJITIRBlock* handlebk;

	/* The first instruction of an exception handler is a
	   GC point */
	CVMJITsetAdd(con, &mc->gcPointsSet, handlerPC);

	handlebk = CVMJITirblockNewLabelForPC(con, handlerPC); 
	CVMJITirblockAddFlags(handlebk, CVMJITIRBLOCK_IS_EXC_HANDLER);
    }
    return;
}

/*
 * Walk through the exceptionTable and and lineNumberTable and mark
 * every PC mentioned in these tables as a PC that we need to produce
 * a javaPC -> compiledPC mapping for.
 */
void
CVMJITirblockMarkAllPCsThatNeedMapping(CVMJITCompilationContext* con)
{
    CVMJITMethodContext* mc = con->mc;
    CVMJavaMethodDescriptor* jmd = mc->jmd;
    CVMJITSet* mapPcSet = &mc->mapPcSet;
    int i; 

    /* Mark the various exception handling pc's in the mapPcSet */
    {
	CVMExceptionHandler* handler = CVMjmdExceptionTable(jmd);
	for (i = CVMjmdExceptionTableLength(jmd); i > 0  ; --i, handler++) {
	    CVMJITsetAdd(con, mapPcSet, handler->handlerpc); 
	    CVMJITsetAdd(con, mapPcSet, handler->startpc); 
	    CVMJITsetAdd(con, mapPcSet, handler->endpc);
	}
    }

#ifdef CVM_DEBUG_CLASSINFO
    /* Mark the line number table pc's in the mapPcSet */
    if (con->mc->compilationDepth > 0) {
	
	CVMLineNumberEntry* entry = CVMjmdLineNumberTable(jmd);
	for (i = CVMjmdLineNumberTableLength(jmd); i > 0  ; --i, entry++) {
	    CVMJITsetAdd(con, mapPcSet, entry->startpc); 
	}
    }
#endif

    return;
}

static void
markJsrTargets(CVMJITCompilationContext* con,
               CVMUint32 jsrTargetPC,
               CVMJITIRBlock* jsrTargetBk,
               CVMJITIRBlock* jsrRetBk)
{
    CVMJavaMethodDescriptor* jmd = con->mc->jmd;
    CVMUint8* codeBegin = CVMjmdCode(jmd);
    CVMUint8* targetBlockStartPC = codeBegin +
	CVMJITirblockGetBlockPC(jsrTargetBk);
    CVMJITJsrRetEntry* entry;

    /* Now check that the first instruction of the jsr target
       is really a store into a local. We don't know how
       to handle methods that do otherwise (which are very rare, and are
       never produced by javac) */
    switch(*targetBlockStartPC) {
    case opc_astore:   break;
    case opc_astore_0: break;
    case opc_astore_1: break;
    case opc_astore_2: break;
    case opc_astore_3: break;
    case opc_wide:
	if (targetBlockStartPC[1] == opc_astore) {
            break;
	}
        /* fall through */
    default:
	/* The first instruction is not an astore. Can't compile this */
        CVMJITerror(con, CANNOT_COMPILE,
	  	    "jsr target does not start with PC store");
        return;
    }

    CVMJITirblockAddFlags(jsrTargetBk, CVMJITIRBLOCK_IS_JSR_HANDLER);
    CVMJITirblockAddFlags(jsrRetBk, CVMJITIRBLOCK_IS_JSR_RETURN_TARGET);
	
    /* Don't let JSR targets be tagged at backward branch targets since 
       letting them be gc checkpoints will cause problems. */
    jsrTargetBk->flags &= ~CVMJITIRBLOCK_IS_BACK_BRANCH_TARGET;

    /* Create a new entry for jsr return target */
    entry = (CVMJITJsrRetEntry*)CVMJITmemNew(con, 
                     JIT_ALLOC_IRGEN_OTHER, sizeof(CVMJITJsrRetEntry));

    CVMJITjsrRetEntrySetNext(entry, NULL);
    CVMJITjsrRetEntrySetPrev(entry, NULL);

    entry->jsrTargetPC = jsrTargetPC;
    entry->jsrRetBk = jsrRetBk;

    /* And append the entry onto the list */
    CVMJITirlistAppend(con, &con->jsrRetTargetList, (void*)entry);
}

static CVMJITIRBlock*
newLabel(CVMJITCompilationContext* con, CVMUint16 currPC, CVMUint16 targetPC)
{
    CVMJITIRBlock *targetbk = CVMJITirblockNewLabelForPC(con, targetPC);
    /* note backwards branches */
    if (targetPC <= currPC) {
	CVMJITirblockAddFlags(targetbk, CVMJITIRBLOCK_IS_BACK_BRANCH_TARGET);
    }
    return targetbk;
}

static void
CVMJITsplitNot(CVMJITCompilationContext *con, CVMUint32 i, void *data);

/*
 * This piece of code finds all block headers.
 * It also marks GC points
 */
void
CVMJITirblockFindAllNormalLabels(CVMJITCompilationContext* con)
{
    CVMJITMethodContext* mc = con->mc;
    CVMMethodBlock* mb = mc->mb;
    CVMJavaMethodDescriptor* jmd = mc->jmd;
    CVMUint8*  codeBegin = CVMjmdCode(jmd);
    CVMUint16  codeLength = CVMmbCodeLength(mb);
    CVMUint8*  codeEnd = &codeBegin[codeLength];
    CVMUint8*  pc = codeBegin;

    CVMJITsetClear(con, &mc->notSeq);

    /* Mark the first instruction as a GC point */
    CVMJITsetAdd(con, &mc->gcPointsSet, 0);
    
    /* The first instruction is also the first block header */
    CVMJITirblockNewLabelForPC0(con, 0);

    while (pc < codeEnd) { 
	CVMOpcode instr = *pc;
        CVMUint32 instrLen = CVMopcodeGetLength(pc);

#ifdef CVM_JVMTI
        if (instr == opc_breakpoint) {
	    CVMJITerror(con, CANNOT_COMPILE_NOW,
			"opc_breakpoint not supported");
	}
#endif

        if (CVMbcAttr(instr, RETURN)) {
	    /*
	     * Inline methods return by branches to a special end block,
	     * unless the return is the last instruction.
	     */
	    if (pc + instrLen < codeEnd && con->mc->compilationDepth > 0) {
		CVMJITirblockNewLabelForPC(con, codeEnd - codeBegin);
	    }
	} else if (CVMbcAttr(instr, BRANCH)) {
	    /* Branches are unconditional GC points. If there is
	       any refinement to be done on this selection, let
	       the back-end handle it */
	    CVMJITsetAdd(con, &mc->gcPointsSet, pc - codeBegin);
            /*
             * We either have a goto or jsr, a tableswitch, a lookupswitch,
             * or one of the if's.
             */
            switch(instr) {
                case opc_goto:
                case opc_jsr: {
                    /* An unconditional goto, 2-byte offset */
                    CVMInt16 offset = CVMgetInt16(pc+1);
                    CVMUint16 codeOffset = pc + offset - codeBegin;
		    CVMJITIRBlock* targetBlock =
			newLabel(con, pc - codeBegin, codeOffset);

		    if (instr == opc_jsr) {
                        CVMJITIRBlock* jsrRetBk = CVMJITirblockNewLabelForPC(con, 
                                  pc + instrLen - codeBegin);
                        markJsrTargets(con, codeOffset, targetBlock, jsrRetBk);
                    }
                    break;
                }
                case opc_goto_w:
                case opc_jsr_w: {
                    /* An unconditional goto, 4-byte offset */
                    CVMInt32 offset = CVMgetInt32(pc+1);
                    CVMUint32 codeOffset = pc + offset - codeBegin;
		    CVMJITIRBlock* targetBlock =
			newLabel(con, pc - codeBegin, codeOffset);

                    /* We don't know how to deal with code size > 64k */
                    CVMassert(codeOffset <= 65535);
		    if (instr == opc_jsr_w) {
                        CVMJITIRBlock* jsrRetBk = CVMJITirblockNewLabelForPC(con, 
                                  pc + instrLen - codeBegin);

                        markJsrTargets(con, codeOffset, targetBlock, jsrRetBk);
		    }
                    break;
                }
                case opc_lookupswitch: {
	    	    int cnt;
	    	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
            	    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);

		    /* default target block */
                    newLabel(con, pc - codeBegin,
                        CVMgetAlignedInt32(lpc) + pc - codeBegin); 

	    	    /* Identify match-offset target blocks */ 
	    	    for (cnt = 0; cnt < npairs; cnt++) {
                	lpc += 2;
                        newLabel(con, pc - codeBegin,
			    CVMgetAlignedInt32(&lpc[1]) + pc - codeBegin); 
            	    }
		    break; 
	    	}
                case opc_tableswitch: { 
            	    int cnt;
            	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
            	    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
            	    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
                    newLabel(con, pc - codeBegin,
			CVMgetAlignedInt32(&lpc[0]) + pc - codeBegin);

            	    /* Fill up jump offset table list */
            	    for (cnt = 0; cnt < high - low + 1; cnt++) {
                        newLabel(con, pc - codeBegin,
			    CVMgetAlignedInt32(&lpc[3+cnt]) + pc - codeBegin);
            	    }
            	    break;
		}

                case opc_ifeq:
                    if (CVMJIToptPatternIsNotSequence(con, pc)) {
                        CVMInt32 pcIndex = pc - codeBegin;
                        /* We need to check later to see if there are branches
                           into any of the internal labels in this sequence.
                           If so, we should abandon this NOT sequence and treat
                           them as blocks.  So, we record the sequence locations
                           (in a linked list) for a second pass examination
                           later to check for this: */

			CVMJITsetAdd(con, &mc->notSeq, pcIndex);

                        /* Skip the sequence for now: */
                        instrLen = CVMJITOPT_SIZE_OF_NOT_SEQUENCE;
                        break;
                    }
                    /* Else, fault thru to the default below. */

                default: {
                    CVMInt16 skip;
                    CVMUint16 codeOffset;
                    /* This had better be one of the 'if' guys */
                    CVMassert(((instr >= opc_ifeq) &&
                               (instr <= opc_if_acmpne)) ||
                              (instr == opc_ifnull) ||
                              (instr == opc_ifnonnull));
                    CVMassert(instrLen == 3);
                    skip = CVMgetInt16(pc+1);
                    /* Mark the target of the 'if' */
                    codeOffset = pc + skip - codeBegin;
                    newLabel(con, pc - codeBegin, codeOffset);
		    break;
                }
            }
        } else if ((instr == opc_astore_0) ||
		   (instr == opc_astore && pc[1] == 0) ||
		   (instr == opc_wide && pc[1] == opc_astore && 
		    CVMgetUint16(pc+2) == 0)) {
	    /* looks like we can't trust locals[0] to be "this" */
	    mc->removeNullChecksOfLocal_0 = CVM_FALSE;
	}

	if (instr == opc_wide) {
	    instr = pc[1];
	}

	if (CVMbcAttr(instr, GCPOINT)) {
	    CVMJITsetAdd(con, &mc->gcPointsSet, pc - codeBegin);
	}
        pc += instrLen;
	/* Instructions that don't let control flow through terminate
	   a block */
	if (CVMbcAttr(instr, NOCONTROLFLOW)) {
	    if (pc < codeEnd) {
		CVMJITirblockNewLabelForPC(con, pc - codeBegin);
	    }
	}
    } /* end of while */

    /* Now scan all the NOT sequences and see if we need to break them
       into blocks: */
    CVMJITsetIterate(con, &mc->notSeq, CVMJITsplitNot, NULL);

    return;
}

static void
CVMJITsplitNot(CVMJITCompilationContext *con, CVMUint32 i, void *data)
{
    CVMJITMethodContext* mc = con->mc;
    CVMUint16 pcIndex = i;

    /* The NOT sequence looks like this:

       0 ifeq absPC+7
       3 iconst_0
       4 goto absPC+4
       7 iconst_1
    */

#ifdef CVM_DEBUG_ASSERTS
{
    CVMJavaMethodDescriptor* jmd = mc->jmd;
    CVMUint8*  codeBegin = CVMjmdCode(jmd);
    CVMassert(codeBegin[pcIndex] == opc_ifeq);
}
#endif

    /* Check if anyone branches to the iconst_0 bytecode: */
    /* Check if anyone branches to the goto bytecode: */
    /* Check if anyone branches to the iconst_1 bytecode: */
    if (mc->pcToBlock[pcIndex + 3] == NULL &&
	mc->pcToBlock[pcIndex + 4] == NULL &&
	mc->pcToBlock[pcIndex + 7] == NULL)
    {
	return;
    }

    /* If we get here, then we now that someone will branch into the middle
       of this sequence.  We don't need to care about those incoming
       branches here.  All we need to take care of is to make sure that the
       branch targets of the ifeq and goto is setup as blocks now. 

       The ifge's target block is the iconst_1 bytecode.
       The goto's target block is the bytecode after the iconst_1.

       Note: we can just call newLabel() directly.  It will check if a
       block has been created already, and not create a redundant
       block if one already exists for the target.
    */

    /* Make sure the ifeq target is a block if not already: */
    newLabel(con, /* ifeq */ pcIndex, /* iconst_1 */ pcIndex + 7);
	      
    /* Make sure the goto target is a block if not already: */
    newLabel(con, /* goto */ pcIndex + 4, /* end */ pcIndex + 8);

    /* Bad candidate. Remove from set so that translateRange will
       not try to replace it. */ 
    CVMJITsetRemove(con, &mc->notSeq, pcIndex);
}

/*
 * Merge (set) local refs information 'localRefs' into that of 'targetbk'.
 *
 * Given that we are evaluating blocks in flow-order and stackmaps
 * disambiguation must already have taken place, there are two possibilities:
 * 
 *     1) The locals of targetbk have not yet been set
 *
 *            or
 *
 *     2) intersect(targetbk->locals, locals) is either locals or
 *     targetbk->locals.
 *
 *  Returns CVM_TRUE if the merge causes a change in the target's
 *  locals information.
 */
static CVMBool
mergeState(CVMJITCompilationContext* con,
	   CVMJITSet* dest, CVMJITSet* src, int maxLocals)
{
    CVMBool change = CVM_FALSE;

    CVMassert(CVMJITsetInitialized(src));

    if (!CVMJITsetInitialized(dest)) {
	CVMJITsetInit(con, dest);
	CVMJITsetCopyAndClear(con, dest, src, maxLocals);
	change = CVM_TRUE;
    } else {
	change = CVMJITsetIntersectionAndNoteChanges(con, dest, src);
    }

    return change;
}

CVMBool
CVMJITirblockMergeLocalRefs(CVMJITCompilationContext* con,
			    CVMJITIRBlock* targetbk)
{
    CVMJITSet* targetRefs = &targetbk->localRefs;
    int maxLocals = targetbk->inMc->currLocalWords;
    CVMJITSet* localRefs = &con->curRefLocals;
    CVMBool changed;

    changed = mergeState(con, targetRefs, localRefs, maxLocals);

    /* Now, if this changed the targetRefs, we'd better not have
       translated this method yet. Otherwise we can't trust the
       local refs information */
    if (changed && ((CVMJITirblockGetflags(targetbk) &
	(CVMJITIRBLOCK_IS_TRANSLATED|CVMJITIRBLOCK_LOCALREFS_INFO_CHANGED)) ==
	CVMJITIRBLOCK_IS_TRANSLATED))
    {
	CVMJITirblockAddFlags(targetbk, CVMJITIRBLOCK_LOCALREFS_INFO_CHANGED);
	CVMJITirblockEnqueue(&con->refQ, targetbk);
/*#define LOCALREF_INFO_REFINEMENT_DEBUGGING*/
#ifdef LOCALREF_INFO_REFINEMENT_DEBUGGING
	CVMconsolePrintf("\n\n\nWARNING: Will refine locals information for "
			 "computed for %C.%M, block PC=%d\n\n\n",
			 CVMmbClassBlock(con->mb),
			 con->mb,
			 CVMJITirblockGetBlockPC(targetbk));
#endif
    }
    return changed; /* changed if !equal */
}

/* Perform flow from one block to another, which also involves
   merging local refs into the target block. */
CVMBool
CVMJITirblockConnectFlow(CVMJITCompilationContext* con,
			 CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk)
{
    CVMassert(curbk == con->mc->currBlock);
    CVMassert(targetbk->inDepth <= con->mc->compilationDepth);
    CVMassert(con->mc == targetbk->inMc ||
	targetbk->inDepth < con->mc->compilationDepth);
    CVMassert(CVMJITirblockIsArtificial(targetbk) ||
	con->mc != targetbk->inMc ||
	con->mc->pcToBlock[CVMJITirblockGetBlockPC(targetbk)] == targetbk);

    if ((CVMJITirblockGetflags(targetbk) &
	(CVMJITIRBLOCK_IS_TRANSLATED|CVMJITIRBLOCK_PENDING_TRANSLATION)) == 0)
    {
	CVMJITirblockAddFlags(targetbk, CVMJITIRBLOCK_PENDING_TRANSLATION);
	CVMJITirblockEnqueue(&con->transQ, targetbk);
    }

    CVMJITirblockAddArc(con, curbk, targetbk);

    /* Mark as branch target */
    CVMJITirblockAddFlags(targetbk, CVMJITIRBLOCK_IS_BRANCH_TARGET);

    /* Merge local refs information into target block */
    return CVMJITirblockMergeLocalRefs(con, targetbk);    
}

/* 
 * At the exit of a block through goto branch, conditional branch,
 * or finishing up a block, connect arc between two blocks, and
 * take care of the phi merge.
 *
 * unconditionalBranch is set to CVM_TRUE when the branch in question is
 * unconditional. In this case, everything on the stack will get
 * removed. If it is not true, then there will be other branch targets
 * that will call CVMJITirblockPhiMerge() for the same stack items, so
 * we need to leave them.
 */
void
CVMJITirblockAtBranch(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk,
		      CVMJITIRNode* expression1ToEvaluate,
		      CVMJITIRNode* expression2ToEvaluate,
		      CVMJITIRNode* branchNode,
		      CVMBool unconditionalBranch)
{
    /* 
     * We don't support branching into exception handler blocks.  This will
     * never happen with javac code and we choose not to compile the method
     * if this ever does happen.
     */
    if (CVMJITirblockIsExcHandler(targetbk)) {
        CVMJITerror(con, CANNOT_COMPILE,
                    "branching into exception handler not supported");
    }

    /* Make sure all expressions with side effects get evaluated before we
       evaluate the non-side effect expressions when we flush out bound locals
       below.  CVMJITirFlushOutBoundLocals() would also call
       sideEffectOperator() to ensure this.  However, in this case, that
       proves to be inadequate because the branch opcode that got us here has
       already popped its arguments off the top of the stack.  Hence, we must
       call the sideEffectOperator(), followed by forced evaluation of the
       lhs, and rhs nodes (if they have side effects) ourself.

       However, we shouldn't force evaluation of the branchNode ahead of
       flushing the locals.  This is because the branchNode is supposed to
       get evaluated last.

       NOTE: We must flush the locals before doing the phi merge.  This is
       because the phi merge include emitting nodes to load phi values into
       registers.  Flushing the locals may entail evaluating expressions which
       may disrupt the phis in registers.  Flushing locals before that
       guarantees that the phis in regs are undisturbed before the branch.
    */
    CVMJITirDoSideEffectOperator(con, curbk);
    CVMJITirFlushOutBoundLocals(con, curbk, targetbk->inMc, CVM_FALSE);
    if ((expression1ToEvaluate != NULL) &&
        CVMJITirnodeHasSideEffects(expression1ToEvaluate)) {
        CVMJITirForceEvaluation(con, curbk, expression1ToEvaluate);
    }
    if ((expression2ToEvaluate != NULL) &&
        CVMJITirnodeHasSideEffects(expression2ToEvaluate)) {
        CVMJITirForceEvaluation(con, curbk, expression2ToEvaluate);
    }

    CVMJITirFlushOutBoundLocals(con, curbk, targetbk->inMc, CVM_TRUE);

    CVMJITirblockConnectFlow(con, curbk, targetbk);

    CVMJITirblockPhiMerge(con, curbk, targetbk,
			  expression1ToEvaluate, expression2ToEvaluate,
			  branchNode);

    if (unconditionalBranch) {
        CVMJITstackResetCnt(con, con->operandStack); /* set stack to empty */ 
    }
}

/*
 * At the beginning of a block: 
 */
void
CVMJITirblockAtLabelEntry(CVMJITCompilationContext* con, CVMJITIRBlock* bk)
{
    /* The operand stack must be empty after all items were
     * evaluated into DEFINE node at block exit or branch */
    CVMassert(CVMJITstackIsEmpty(con, con->operandStack));

    /* Push a JSR_RETURN_ADDRESS node on the stack if this a finally block */
    if (CVMJITirblockIsJSRTarget(bk)) {
	CVMJITIRNode* node;

	/*
	 * We don't support finally blocks that require any items to be
	 * on the stack. This will never happen with javac code and we
	 * choose not to compile the method if this ever does happen.
         * This method should have already been rejected from compilation in
         * translateRange() at the JSR site.  Just assert here:
	 */
        CVMassert(bk->phiCount == 0);

	/* Build a jsr return address node and place an entry on the stack
	   representing the return address we know is there. 
	   Stuff it with the pc of the current block, which is the
	   entry point of the jsr/ret subroutine. This may be used
	   in blockCodeChangesLocalInfo to do local ref-info refinement.
         */
	node = CVMJITirnodeNewNull(con, CVMJIT_ENCODE_JSR_RETURN_ADDRESS);
	CVMJITirnodeGetNull(node)->data = CVMJITirblockGetBlockPC(bk);

        CVMJITirnodeSetHasUndefinedSideEffect(node);
	CVMJITirnodeStackPush(con, node);
    }
    /*
     * Push all USED node in the phiArray onto the operandStack. They must
     * be pushed in increasing order of spillLocation (deepest node first).
     */
    if (bk->phiCount > 0) {
	int cnt;
	for (cnt = 0; cnt < bk->phiCount; cnt++) {
	    CVMJITirnodeStackPush(con, bk->phiArray[cnt]);
	}
    }     
}

/*
 * At the beginning of a handler block
 */
void
CVMJITirblockAtHandlerEntry(CVMJITCompilationContext* con, CVMJITIRBlock* bk)
{
    /* Build an exception object node and place an entry on the stack
       representing the exception object we know is there. */
    CVMJITIRNode* node =
	CVMJITirnodeNewNull(con, CVMJIT_ENCODE_EXCEPTION_OBJECT);

    CVMassert(CVMJITstackIsEmpty(con, con->operandStack));
    CVMJITirnodeSetHasUndefinedSideEffect(node);
    CVMJITirnodeStackPush(con, node);
}

#define localAlreadyOrdered(bk, idx) \
    ((bk->orderedIncomingLocals & (1 << idx)) != 0)

/*
 * When leaving a block through a conditional jump, uncoditional jump, or
 * completing each block translation, we need to force evaluation of 
 * things left on the stack into DEFINE nodes. We also need to make sure
 * that the target block has its phiArray of USED nodes setup, with
 * one USED node per item left on the stack.
 */

static void
CVMJITirblockPhiMerge(CVMJITCompilationContext* con,
		      CVMJITIRBlock* curbk, CVMJITIRBlock* targetbk,
		      CVMJITIRNode* expression1ToEvaluate,
		      CVMJITIRNode* expression2ToEvaluate,
		      CVMJITIRNode* branchNode)
{
    CVMJITStack*   operandStack = con->operandStack;
    CVMInt32       cnt;
    CVMUint32      stackCnt = CVMJITstackCnt(con, operandStack);
    CVMBool        needsPhiArray = (targetbk->phiCount == -1);
    CVMUint16      spillLocation;
    CVMJITIRNode*  defineNode;
    CVMJITIRNode** defineList = NULL;

#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * Blocks with incoming PHIs are not allowed to have incoming locals.
     */
    if (stackCnt != 0) {
	CVMJITirblockNoIncomingLocalsAllowed(targetbk);
	/* Although the code below works, it seems to actually hurt
	 * performance so it has been disabled.
	 */
#if 0
	/* Avoid Phi registers as much as possible be moving existing
	 * incoming locals to just after the outgoing phis. This isn't
	 * perfect because we don't take into account the size of the
	 * outgoing Phis, and if we already have too many incoming locals
	 * then there will still be conflicts that may result in locals
	 * being spilled by outgoing Phis.
	 */
	int idx;
	int shiftAmount = stackCnt;
	for (idx = stackCnt - 1; idx >= 0; idx--) {
	    if (localAlreadyOrdered(curbk, shiftAmount)) {
		shiftAmount--;
	    }
	}

	if (shiftAmount >
	    CVMJIT_MAX_INCOMING_LOCALS - curbk->incomingLocalsCount)
        {
	    shiftAmount =
		CVMJIT_MAX_INCOMING_LOCALS - curbk->incomingLocalsCount;
	}
	    
	curbk->orderedIncomingLocals <<= shiftAmount;
	for (idx = curbk->incomingLocalsCount - 1; idx >= 0; idx--)
	{
	    CVMassert(idx + shiftAmount < CVMJIT_MAX_INCOMING_LOCALS);
	    curbk->incomingLocals[idx + shiftAmount] =
		curbk->incomingLocals[idx];
	}
	for (idx = 0; idx < shiftAmount; idx++)
	{
	    curbk->incomingLocals[idx] = NULL;
	    curbk->orderedIncomingLocals |= (1 << idx);
	}
	curbk->incomingLocalsCount += shiftAmount;
	CVMassert(curbk->incomingLocalsCount <= CVMJIT_MAX_INCOMING_LOCALS);
	for (idx = 0; idx < curbk->successorBlocksCount; idx++) {
	    curbk->successorBlocksLocalOrder[idx] += shiftAmount;
	}
#endif
    }
#endif

   /*
     * If the target block does not have a phiArray yet, then create one.
     * The phiArray will already exist if a merge has already been done
     * into the target block. We need one entry (a USED node) in the array 
     * per item on the ir stack.
     */
    if (!needsPhiArray) {
	CVMassert(targetbk->phiCount == stackCnt);
    } else {
	if (stackCnt != 0) {
	    /* Allocate the phiArray for the target block. */
	    targetbk->phiArray = (CVMJITIRNode**)
		CVMJITmemNew(con, JIT_ALLOC_IRGEN_NODE,
			     stackCnt * sizeof(CVMJITIRNode*));
	}
	targetbk->phiCount = stackCnt;
    }

    if (stackCnt != 0) {
	defineList = (CVMJITIRNode**)
	    CVMJITmemNew(con, JIT_ALLOC_IRGEN_NODE,
			 stackCnt * sizeof(CVMJITIRNode*));
    }

    /* 
     * Create a DEFINE node for each item on the stack.
     */ 
    CVMwithAssertsOnly({ con->operandStackIsBeingEvaluated = CVM_TRUE; });
    spillLocation = 0;
    for (cnt = 0; cnt < stackCnt; cnt++) {
	CVMJITIRNode* stkElmt =
	    CVMJITstackGetElementAtIdx(con, operandStack, cnt);
	CVMUint8  typeTag = CVMJITgetTypeTag(stkElmt);
	CVMBool   is64 = CVMJITirnodeIsDoubleWordType(stkElmt);
	CVMBool   registerSpillOk;

	/*
	 * Decide if this phi value can be passed in a register.
	 */
	registerSpillOk = CVMglobals.jit.registerPhis;

	/* create the used node if it hasn't been created already. */
	if (needsPhiArray) {
	    targetbk->phiArray[cnt] =
		CVMJITirnodeNewUsedOp(con, CVMJIT_ENCODE_USED(typeTag),
				      spillLocation, registerSpillOk);
	} else {
	    /*
	     * Try to be type compatable with the target. This
	     * will make it possible to do type-dependent
	     * phi passing in a consistent manner.
	     */
	    typeTag = CVMJITgetTypeTag(targetbk->phiArray[cnt]);
	}
	
	/* The stkElmt type must be the compatible with the USED node type. */
	{
	    CVMwithAssertsOnly(
		CVMUint8 phiTypeTag =
		    CVMJITgetTypeTag(targetbk->phiArray[cnt]);
	    );
	    CVMassert((typeTag ==  phiTypeTag) ||
		      (typeTag == CVMJIT_TYPEID_32BITS &&
		       CVMJITirIsSingleWordTypeTag(phiTypeTag)) ||
		      (typeTag == CVMJIT_TYPEID_64BITS &&
		       CVMJITirIsDoubleWordTypeTag(phiTypeTag)) ||
		      (phiTypeTag == CVMJIT_TYPEID_32BITS &&
		       CVMJITirIsSingleWordTypeTag(typeTag)) ||
		      (phiTypeTag == CVMJIT_TYPEID_64BITS &&
		       CVMJITirIsDoubleWordTypeTag(typeTag)));
	}

	/*
	 * Step 1 for Phis in registers: Creation of a define node for each
	 * stack item will force the evaluation of the expressions. This
	 * must be done for all Phi values before we attempt to load any
	 * of them into registers.
	 */
        defineNode =
	    CVMJITirnodeNewDefineOp(con, CVMJIT_ENCODE_DEFINE(typeTag),
				    stkElmt,
				    targetbk->phiArray[cnt]);

	defineList[cnt] = defineNode;
	CVMJITirnodeNewRoot(con, curbk, defineList[cnt]);

	/* by definition, this node has been evaluated: */
	CVMJITirnodeSetHasBeenEvaluated(stkElmt);

	if (is64) {
	    spillLocation += 2;
	} else {
	    spillLocation += 1;
	}

    }

    targetbk->phiSize = spillLocation;

    /* update con->maxPhiSize if necessary. */
    if (spillLocation > con->maxPhiSize) {
	con->maxPhiSize = spillLocation;
    }

    if (stackCnt != 0) {
	/*
	 * Step 2 for Phis in registers: Before we force any phi value
	 * into its proper register, we need to force evaluation of any
	 * expression used to determine the branch target. Otherwise
	 * evaluation of these expressions could cause the DEFINE values
	 * to be bumped from their registers.
	 */
        /* No need to call CVMJITirDoSideEffectOperator() here because
         * it has been done by the caller, CVMJITirblockAtBranch() */
	if (expression1ToEvaluate != NULL) {
	    CVMJITirForceEvaluation(con, curbk, expression1ToEvaluate);
	}
	if (expression2ToEvaluate != NULL) {
	    CVMJITirForceEvaluation(con, curbk, expression2ToEvaluate);
	}

	/*
	 * Step 3 for Phis in regsiters: Now that all evaluations needed
	 * before block exit have been done, we can force phi values into
	 * the appropriate registers. We are guaranteed that all of the
	 * phis have been evaulated at this point, and that no further
	 * expressions will be evaluated before we do the branch.
	 * However, we are not guaranteed that the expressions from Step 2
	 * are in registers so LOAD_PHIS must keep the registers pinned.
	 */
	CVMJITirnodeNewRoot(con, curbk,
			    CVMJITirnodeNewPhisListOp(con,
				CVMJIT_ENCODE_LOAD_PHIS,
				targetbk,
				defineList));
    }

    /*
     * Step 4 for Phis in regsiters: Create the root node for the
     * branch node. When this node is evaluated, it will force any
     * expressions used in determining the branch target into
     * registers. Note that these expressions were already forced to be
     * evaluated in step 2.
     */
    if (branchNode != NULL) {
	CVMJITirnodeNewRoot(con, curbk, branchNode);
    }

    if (stackCnt != 0) {
	/*
	 * Step 5: Create a RELEASE_PHIS node. Now that we know that
	 * nothing can cause the phi values to be bumped from the registers
	 * they have been placed from, we can release the registers.
	 */
	CVMJITirnodeNewRoot(con, curbk,
			    CVMJITirnodeNewPhisListOp(con, 
				CVMJIT_ENCODE_RELEASE_PHIS,
				targetbk,
				defineList));
    }
    CVMwithAssertsOnly({ con->operandStackIsBeingEvaluated = CVM_FALSE; });
}

#ifdef CVM_JIT_REGISTER_LOCALS
void
CVMJITirblockNoIncomingLocalsAllowed(CVMJITIRBlock* bk)
{
    int i;
    /* First make sure we zap any existing incoming locals to avoid
     * possible confusion during the merging stage, which often looks
     * an incomingLocals[] regardless of incomingLocalsCount.
     */
    for (i = 0; i < bk->incomingLocalsCount; i++) {
	bk->incomingLocals[i] = NULL;
    }
    bk->noMoreIncomingLocals = CVM_TRUE;
    bk->noMoreIncomingRefLocals = CVM_TRUE;
    bk->incomingLocalsCount = 0;

    /*
     * Successor blocks are still useful for targetting LOCAL nodes
     * because we want to know which register to target the local to
     * when it flows out of the block. Thus we don't remove all the
     * successor blocks anymore. However, we don't want to merge the
     * locals from the successor blocks, to we need to set
     * okToMergeSuccessorBlock to FALSE for each.
     */
#if 0
    bk->successorBlocksCount = 0;
#else
    {
	int bkIdx;
	for (bkIdx = 0; bkIdx < bk->successorBlocksCount; bkIdx++) {
	    bk->okToMergeSuccessorBlock[bkIdx] = CVM_FALSE;
	}
    }
#endif
}
#endif
