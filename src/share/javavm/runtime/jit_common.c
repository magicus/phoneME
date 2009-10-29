/*
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

#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/preloader.h"

#include "javavm/include/jit_common.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitstats.h"

#include "javavm/include/opcodes.h"

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
#include "javavm/include/preloader_impl.h"
#include "javavm/include/porting/time.h"
#endif

static CVMUint16*
lookupStackMap(
    CVMCompiledStackMaps* maps,
    CVMUint32 off,
    CVMInt32* msize,
    CVMInt32* psize)
{
    int					nentries;
    int 				i;
    CVMUint16*				largeMaps;
    CVMCompiledStackMapEntry*		e;
    CVMCompiledStackMapLargeEntry*	l;

    /* do straight search for now. When maps get sorted properly, can do 
       better
    */
    if (maps == NULL) return NULL;
    nentries = maps->noGCPoints;
    for (i = 0; i < nentries; i++){
	if (maps->smEntries[i].pc == off)
	    break;
    }
    if (i >= nentries)
	return NULL; /* iterated off the end! */
    e = & maps->smEntries[i];
    if (e->totalSize < (unsigned)0xff){
	*msize = e->totalSize;
	*psize = e->paramSize;
	return &(e->state);
    }else{
	largeMaps = (CVMUint16*)((char*)maps + sizeof (maps->noGCPoints) + 
			nentries * sizeof (CVMCompiledStackMapEntry));
	l = (CVMCompiledStackMapLargeEntry*) &largeMaps[e->state];
	*msize = l->totalSize;
	*psize = l->paramSize;
	return &(l->state[0]);
    }
}

static CVMUint16*
findStackMap(
    CVMExecEnv *                 targetEE,
    CVMCompiledMethodDescriptor* cmd,
    CVMUint32*			 offPtr,
    CVMInt32*			 msize,
    CVMInt32*			 psize,
    CVMFrame*			 frame)
{
    CVMCompiledStackMaps* maps = CVMcmdStackMaps(cmd);
    CVMUint16*		  mapData;
    CVMUint8*		  compiledPc;
    CVMUint8*		  javaPc;
    CVMUint32		  off = *offPtr;
    CVMMethodBlock*	  mb = frame->mb;

    mapData = lookupStackMap(maps, off, msize, psize);
    if (mapData != NULL){
	/*
	   If we are throwing an exception, we can prune the
	   locals to scan, as we do below when a stackmap isn't
	   found.  But it's probably cheaper to scan a few
	   extra locals than to perform another lookup.
	*/
	return mapData;
    }

    /*
     * There is no map here. We'd better be throwing an exception.
     * In this case, only the locals matter since the stack and
     * temps get discarded, and those locals are described at any
     * convenient handler to which we could transfer.
     */
    if (!CVMexceptionIsBeingThrownInFrame(targetEE, frame)) {
#ifdef CVM_DEBUG
        extern void CVMbcList(CVMMethodBlock* mb);
        CVMconsolePrintf("Unable to find stackmap for method:\n"
                         "    %C.%M\n"
                         "    at PC 0x%x, offset 0x%x\n"
                         "    method startPC is 0x%x\n",
                         CVMmbClassBlock(mb), mb,
                         off + CVMcmdStartPC(cmd), off,
                         CVMcmdStartPC(cmd));
        CVMconsolePrintf("\nThe method's bytecode:\n");
        CVMbcList(mb);
        CVMconsolePrintf("\n");
#endif
#ifdef CVM_DEBUG_DUMPSTACK
        CVMconsolePrintf("\nThe thread's backtrace:\n");
        CVMdumpStack(&targetEE->interpreterStack,0,0,0);
        CVMconsolePrintf("\n");
#endif

#ifdef CVM_DEBUG
        CVMpanic("Unable to find stackmap");
#endif
	CVMassert(CVM_FALSE);
    }

    /* Check for the handler here: */
    compiledPc = off + CVMcmdStartPC(cmd);
    javaPc = CVMpcmapCompiledPcToJavaPc(mb, compiledPc);
    javaPc = CVMfindInnermostHandlerFor(CVMmbJmd(mb), javaPc);
    if (javaPc == 0){
	/* this stack frame is getting blown away anyway. */
	*msize = 0;
	*psize = 0;
	return NULL;
    }
    compiledPc = CVMpcmapJavaPcToCompiledPc(mb, javaPc);
    *offPtr = off = compiledPc - CVMcmdStartPC(cmd);
    mapData = lookupStackMap(maps, off, msize, psize);
    CVMassert(mapData != NULL);
    return mapData;
}

/*
 * Given a compiled frame and PC, this function sets up an iterator for all
 * matching mb's.  Due to inlining, a PC can correspond to multiple mb's.
 *
 * The return value is the number of mb's in the trace...
 *
 * The ordering is reverse from what you would expect. So to get the
 * real call order, you should traverse the backtrace items
 * "backwards".  
 */
void
CVMJITframeIterate(CVMFrame* frame, CVMJITFrameIterator *iter)
{
    CVMMethodBlock*		  mb = frame->mb;
    CVMCompiledMethodDescriptor*  cmd = CVMmbCmd(mb);
    CVMUint8*			  pc0 = CVMcompiledFramePC(frame);
    CVMUint8*			  startPC = CVMcmdStartPC(cmd);
    CVMUint8*			  pc1 = CVMJITmassageCompiledPC(pc0, startPC);
    CVMInt32			  pcOffset = pc1 - startPC;
    CVMCompiledInliningInfo*      inliningInfo = CVMcmdInliningInfo(cmd);
    
    CVMassert(CVMframeIsCompiled(frame));

    iter->frame = frame;
    iter->mb = mb;
    iter->pcOffset = pcOffset;
    iter->index = -1;
    iter->invokePC = -1;

    /* No inlining in this method. So we can trust the frame */
    if (inliningInfo == NULL) {
	iter->numEntries = 0;
	iter->inliningEntries = NULL;  /* avoid compiler warning */
	return;
    }

    iter->inliningEntries = inliningInfo->entries;
    iter->numEntries = inliningInfo->numEntries;

    /*
    CVMconsolePrintf("FRAME 0x%x (pc=0x%x, offset=%d), %C.%M\n",
		     frame, pc, pcOffset, CVMmbClassBlock(mb), mb);
    */
}

/*
 * Given a compiled frame and PC, this iterates over all matching mb's.
 * Due to inlining, a PC can correspond to multiple mb's.
 *
 * The ordering is reverse from what you would expect. So to get the
 * real call order, you should traverse the backtrace items
 * "backwards".  
 */
CVMBool
CVMJITframeIterateSkip(CVMJITFrameIterator *iter,
    CVMBool skipArtificial, CVMBool popFrame)
{
    CVMInt32 pcOffset = iter->pcOffset;
    CVMInt32 lastIndex = iter->index;

    ++iter->index;
    while (iter->index < iter->numEntries) {
	CVMCompiledInliningInfoEntry* iEntry =
	    &iter->inliningEntries[iter->index];
    
	if (iEntry->pcOffset1 <= pcOffset && pcOffset < iEntry->pcOffset2) {
	    if (skipArtificial &&
		(iEntry->flags & CVM_FRAMEFLAG_ARTIFICIAL) != 0)
	    {
		lastIndex = iter->index;
	    } else {
		break;
	    }
	}
	++iter->index;
    }

    if (iter->index == iter->numEntries && skipArtificial &&
	(iter->frame->flags & CVM_FRAMEFLAG_ARTIFICIAL) != 0)
    {
	++iter->index;
    }

    if (popFrame) {
	/* perhaps when we support inlined exception handlers... */
    }

    if (iter->index <= iter->numEntries) {
	CVMassert(lastIndex != iter->index);
	if (lastIndex != -1) {
	    iter->invokePC = iter->inliningEntries[lastIndex].invokePC;
	} else {
	    iter->invokePC = -1;
	}
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

CVMUint32
CVMJITframeIterateCount(CVMJITFrameIterator *iter, CVMBool skipArtificial)
{
    CVMUint32 count = 0;
    while (CVMJITframeIterateSkip(iter, skipArtificial, CVM_FALSE)) {
	++count;
    }
    return count;
}

CVMFrame *
CVMJITframeIterateGetFrame(CVMJITFrameIterator *iter)
{
    CVMassert(iter->index == iter->numEntries);
    return iter->frame;
}

static CVMBool
CVMJITframeIterateContainsPc(CVMJITFrameIterator *iter, CVMUint32 off)
{
    if (iter->index == iter->numEntries) {
#ifdef CVM_DEBUG_ASSERTS
	CVMFrame *frame = iter->frame;
	CVMCompiledMethodDescriptor *cmd = CVMmbCmd(frame->mb);
	CVMUint32 codeSize = CVMcmdCodeBufSize(cmd) - sizeof(CVMUint32);
	CVMassert(off < codeSize);
#endif
	return CVM_TRUE;
    } else {
	CVMCompiledInliningInfoEntry* iEntry =
	    &iter->inliningEntries[iter->index];
    
	return (iEntry->pcOffset1 <= off && off < iEntry->pcOffset2);
    }
}

CVMUint8 *
CVMJITframeIterateGetJavaPc(CVMJITFrameIterator *iter)
{
   /*
    * Get the java pc of the frame. This is a bit tricky for compiled
    * frames since we need to map from the compiled pc to the java pc.
    */
    if (iter->invokePC != -1) {
	/* The invokePC will be set if we have iterated through
	   an inlined frame already. */
	CVMUint16 invokePC = iter->invokePC;
	CVMMethodBlock *mb = CVMJITframeIterateGetMb(iter);
	CVMUint8 *pc = CVMmbJavaCode(mb) + invokePC;
	CVMassert(CVMbcAttr(*pc, INVOCATION));
	return pc;
    } else if (iter->index == iter->numEntries) {
	/* We have reached the initial Java frame, without
	   seeing an inlined frame.  Rely on the pc map
	   table. */
	CVMFrame *frame = iter->frame;
	return CVMpcmapCompiledPcToJavaPc(frame->mb,
	    CVMcompiledFramePC(frame));
    } else {
	/* We must be in the top-most inlined method, so
	   we have no PC information. */
	return NULL;
    }
}

void
CVMJITframeIterateSetJavaPc(CVMJITFrameIterator *iter, CVMUint8 *pc)
{
   /*
    * Set the pc of the frame.
    */
    CVMFrame *frame;
    /* No support for inlined frames yet */
    CVMassert(iter->index == iter->numEntries);
    frame = iter->frame;
    /* need to convert java pc to compiled pc */
    CVMcompiledFramePC(frame) = CVMpcmapJavaPcToCompiledPc(frame->mb, pc);
}

CVMStackVal32 *
CVMJITframeIterateGetLocals(CVMJITFrameIterator *iter)
{
    CVMUint16 firstLocal = 0;
    if (iter->index < iter->numEntries) {
	firstLocal = iter->inliningEntries[iter->index].firstLocal;
    }
    {
	CVMFrame *frame = iter->frame;
	CVMCompiledMethodDescriptor *cmd = CVMmbCmd(frame->mb);
	CVMStackVal32* locals = (CVMStackVal32 *)frame -
			      CVMcmdMaxLocals(cmd);
	return &locals[firstLocal];
    }
}

CVMObjectICell *
CVMJITframeIterateSyncObject(CVMJITFrameIterator *iter)
{
    if (iter->index == iter->numEntries) {
	return &CVMframeReceiverObj(iter->frame, Compiled);
    } else {
	CVMCompiledInliningInfoEntry *entry =
	    &iter->inliningEntries[iter->index];
	CVMUint16 localNo = entry->firstLocal + entry->syncObject;
	CVMFrame *frame = iter->frame;
	CVMCompiledMethodDescriptor *cmd = CVMmbCmd(frame->mb);
	CVMStackVal32* locals = (CVMStackVal32 *)frame -
			      CVMcmdMaxLocals(cmd);
	return &locals[localNo].ref;
    }
}

CVMMethodBlock *
CVMJITframeIterateGetMb(CVMJITFrameIterator *iter)
{
    if (iter->index == iter->numEntries) {
	return iter->mb;
    } else {
	CVMCompiledInliningInfoEntry* iEntry =
	    &iter->inliningEntries[iter->index];
	CVMassert(iEntry->pcOffset1 <= iter->pcOffset &&
	    iter->pcOffset < iEntry->pcOffset2);
	return iEntry->mb;
    }
}

CVMFrameFlags
CVMJITframeIterateGetFlags(CVMJITFrameIterator *iter)
{
    if (iter->index == iter->numEntries) {
	return iter->frame->flags;
    } else {
	CVMCompiledInliningInfoEntry* iEntry =
	    &iter->inliningEntries[iter->index];
	CVMassert(iEntry->pcOffset1 <= iter->pcOffset &&
	    iter->pcOffset < iEntry->pcOffset2);
	return iEntry->flags;
    }
}

void
CVMJITframeIterateSetFlags(CVMJITFrameIterator *iter, CVMFrameFlags flags)
{
    if (iter->index == iter->numEntries) {
	iter->frame->flags = flags;
    } else {
	CVMCompiledInliningInfoEntry* iEntry =
	    &iter->inliningEntries[iter->index];
	CVMassert(iEntry->pcOffset1 <= iter->pcOffset &&
	    iter->pcOffset < iEntry->pcOffset2);
	iEntry->flags = flags;
    }
}

CVMBool
CVMJITframeIterateIsInlined(CVMJITFrameIterator *iter)
{
    return (iter->index < iter->numEntries);
}

CVMBool
CVMJITframeIterateHandlesExceptions(CVMJITFrameIterator *iter)
{
    return (iter->index == iter->numEntries);
}

CVMMethodBlock *
CVMJITframeGetMb(CVMFrame *frame)
{

    CVMassert(CVMframeMaskBitsAreCorrect(frame));

    if (CVMframeIsCompiled(frame)) {
	CVMMethodBlock*		      mb = frame->mb;
	CVMCompiledMethodDescriptor*  cmd = CVMmbCmd(mb);
	CVMUint8*		      pc0 = CVMcompiledFramePC(frame);
	CVMUint8*		      startPC = CVMcmdStartPC(cmd);
	CVMUint8*		      pc1 = CVMJITmassageCompiledPC(pc0,
						startPC);
	CVMInt32		      pcOffset = pc1 - startPC;
	CVMCompiledInliningInfo*      inliningInfo;
	CVMCompiledInliningInfoEntry* iEntry;
	int i;
	inliningInfo = CVMcmdInliningInfo(cmd);
	
	if (inliningInfo != NULL) {
	    for (i = 0; i < inliningInfo->numEntries; i++) {
		iEntry = &inliningInfo->entries[i];
		if (iEntry->pcOffset1 <= pcOffset &&
		    pcOffset < iEntry->pcOffset2)
		{
		    return iEntry->mb;
		}
	    }
	}
	return mb;
    } else {
	return frame->mb;
    }
}

CVMMethodBlock *
CVMJITeeGetCurrentFrameMb(CVMExecEnv *ee)
{
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);

    if (!CVMframeMaskBitsAreCorrect(frame)) {
	CVMJITfixupFrames(frame);
    }

    return CVMJITframeGetMb(frame);
}

void
CVMcompiledFrameScanner(CVMExecEnv* ee,
			CVMFrame* frame, CVMStackChunk* chunk,
			CVMRefCallbackFunc callback, void* scannerData,
			CVMGCOptions* opts)
{
    CVMMethodBlock*		  mb = frame->mb;
    CVMCompiledMethodDescriptor*  cmd = CVMmbCmd(mb);
    CVMUint8*			  pc = CVMcompiledFramePC(frame);
    CVMInt32			  pcOffset = pc - CVMcmdStartPC(cmd);
    CVMBool			  isAtReturn = CVM_FALSE;
    CVMUint16*			  mapData;
    CVMInt32			  mapSize = 0;
    CVMInt32			  paramSize = 0;
    CVMUint16			  bitmap;
    int				  bitNo;
    int				  noSlotsToScan;
    int				  i;
    CVMStackVal32*		  scanPtr;
    CVMObject**			  slot;
    CVMInterpreterStackData*	  interpreterStackData =
	(CVMInterpreterStackData *)scannerData;
    void*			  data = interpreterStackData->callbackData;
    CVMExecEnv*			  targetEE = interpreterStackData->targetEE;

    if (pc == (CVMUint8*)CONSTANT_HANDLE_GC_FOR_RETURN) {
        CVMassert(frame == targetEE->interpreterStack.currentFrame);
        isAtReturn = CVM_TRUE;        
        pcOffset = -1;
    } else {
	CVMassert((CVMUint32)pcOffset <= 0xffff);
    }
    /* CVMassert(pcOffset < CVMjmdCodeLength(jmd)); would be nice... */

    CVMtraceGcScan(("Scanning compiled frame for %C.%M (frame =0x%x maxLocals=%d, stack=0x%x, pc=%d[0x%x] full pc = 0x%x )\n",
		    CVMmbClassBlock(mb), mb,
		    frame,
		    CVMcmdMaxLocals(cmd),
		    CVMframeOpstack(frame, Compiled),
		    pcOffset,
		    CVMcmdStartPC(cmd),
		    pc));

    if (isAtReturn) {
        goto returnValueScanAndSync;
    }

    {
	CVMInt32 off = pcOffset;

	/* NOTE: The range of PC offsets set by findStackMap() can only be
	   between 0 and 0xffff. */
	mapData = findStackMap(targetEE, cmd, (CVMUint32 *)&off,
			       &mapSize, &paramSize, frame);
	if (mapData == NULL) {
	    /*
	     * nothing to do here, as the stack frame is getting blown away
	     * by an exception.
	     */
	    CVMassert(CVMexceptionIsBeingThrownInFrame(targetEE, frame));
	    /* CR6287566: blow away the locals so we don't run into any
	       scanning bugs with the caller's args. */
#if 1
		noSlotsToScan = CVMcmdMaxLocals(cmd);
		scanPtr = (CVMStackVal32*)frame - noSlotsToScan;
		for (i = 0; i < noSlotsToScan; i++, scanPtr++) {
		    CVMID_icellSetNull(&scanPtr->j.r);
		}
#endif
	    goto check_inlined_sync;
	}

	/* Did findStackMap() skip to an exception handler? */

	if (off != pcOffset) {
	    CVMJITFrameIterator iter;
	    CVMBool foundHandler = CVM_FALSE;

	    CVMJITframeIterate(frame, &iter);

	    /* Scan sync objects until we get to the handler frame */

	    while (CVMJITframeIterateSkip(&iter, CVM_FALSE, CVM_FALSE)) {
		if (CVMJITframeIterateContainsPc(&iter, off)) {
		    foundHandler = CVM_TRUE;
		    break;
		} else {
		    CVMMethodBlock *mb  = CVMJITframeIterateGetMb(&iter);
		    if (CVMmbIs(mb, SYNCHRONIZED)) {
			CVMObjectICell* objICell =
			    CVMJITframeIterateSyncObject(&iter);
			if (objICell != NULL) {
			    slot = (CVMObject**)objICell;
			    callback(slot, data);
			}
		    }
		}
	    }
	    CVMassert(foundHandler);
	}
    }

    bitmap = *mapData++;
    bitNo  = 0;

    /*
     * Scan locals
     */
    noSlotsToScan = CVMcmdMaxLocals(cmd);
    scanPtr = (CVMStackVal32*)frame - noSlotsToScan;

    for (i = 0; i < noSlotsToScan; i++) {
	if ((bitmap & 1) != 0) {
	    slot = (CVMObject**)scanPtr;
	    if (*slot != 0) {
		callback(slot, data);
	    }
	}
	scanPtr++;
	bitmap >>= 1;
	bitNo++;
	if (bitNo == 16) {
	    bitNo = 0;
	    bitmap = *mapData++;
	}
    }

#if 0
    /*
      This is currently disabled because it does not deal
      correctly with a CNI method (no frame of its own)
      throwing an exception.
    */
    /*
      This frame is throwing an exception, so don't bother
      scanning any further.
    */
    if (CVMexceptionIsBeingThrownInFrame(targetEE, frame)) {
	/*
	   If we aren't the top frame, then the callee frame
	   had better be due to classloading during exception
	   handling.
	*/
	CVMassert(interpreterStackData->prevFrame == NULL ||
	    (CVMframeIsFreelist(interpreterStackData->prevFrame) &&
		interpreterStackData->prevFrame->mb == NULL));
	goto skip_opstack;
    }
#endif

    scanPtr = (CVMStackVal32*)CVMframeOpstack(frame, Compiled);
    mapSize -= noSlotsToScan;

    CVMassert((frame == targetEE->interpreterStack.currentFrame)
               == (interpreterStackData->prevFrame == NULL));

    /*
     * if we are the top frame, or calling through a JNI or
     * transition frame, then we scan the outgoing parameters, too.
     * Else not.
     */

    if (interpreterStackData->prevFrame == NULL
	|| CVMframeIsFreelist(interpreterStackData->prevFrame)
        || CVMframeIsTransition(interpreterStackData->prevFrame)) {
        /* count the parameters */
        noSlotsToScan = mapSize;
    } else {
	/* don't count the parameters */
	noSlotsToScan = mapSize - paramSize;
    }

    /*
     * The stackmaps for the variables and the operand stack are
     * consecutive. Keep bitmap, bitNo, mapData as is.
     */
    for (i = 0; i < noSlotsToScan; i++) {
	if ((bitmap & 1) != 0) {
	    slot = (CVMObject**)scanPtr;
	    if (*slot != 0) {
		callback(slot, data);
	    }
	}
	scanPtr++;
	bitmap >>= 1;
	bitNo++;
	if (bitNo == 16) {
	    bitNo = 0;
	    bitmap = *mapData++;
	}
    }

#if 0
skip_opstack:
#endif

    /*
     * The classes of the methods executing. Do this only if not all classes
     * are roots.
     */
    {
	CVMJITFrameIterator iter;

	CVMJITframeIterate(frame, &iter);

	while (CVMJITframeIterateSkip(&iter, CVM_FALSE, CVM_FALSE)) {
	    CVMMethodBlock *mb  = CVMJITframeIterateGetMb(&iter);
	    CVMClassBlock* cb = CVMmbClassBlock(mb);
	    CVMscanClassIfNeeded(ee, cb, callback, data);
	}
    }
    
check_outer_sync:

    /*
     * And last but not least, javaframe->receiverObj
     */
    if (CVMmbIs(mb, SYNCHRONIZED)){
	slot = (CVMObject**)&CVMframeReceiverObj(frame, Compiled);
	callback(slot, data);
    }

    return;

check_inlined_sync:

    /* Scan sync objects of all inlined methods, plus outer method */
    {
	CVMJITFrameIterator iter;

	CVMJITframeIterate(frame, &iter);

	while (CVMJITframeIterateSkip(&iter, CVM_FALSE, CVM_FALSE)) {
	    CVMMethodBlock *mb  = CVMJITframeIterateGetMb(&iter);
	    if (CVMmbIs(mb, SYNCHRONIZED)) {
		CVMObjectICell* objICell = CVMJITframeIterateSyncObject(&iter);
		if (objICell != NULL) {
		    slot = (CVMObject**)objICell;
		    callback(slot, data);
		}
	    }
	}
    }
    return;

returnValueScanAndSync:
    /* Scan return value */
    /* Check ref-based return value */
    if (CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb)) == CVM_TYPEID_OBJ) {
        slot = (CVMObject**)((CVMStackVal32*)frame - CVMcmdMaxLocals(cmd));
        if (*slot != 0) {
            callback(slot, data);
        }
    }

    goto check_outer_sync;

}

#undef TRACE
#define TRACE(a) CVMtraceOpcode(a)

#define DECACHE_TOS()	 frame->topOfStack = topOfStack;
#define CACHE_TOS()	 topOfStack = frame->topOfStack;
#define CACHE_PREV_TOS() topOfStack = CVMframePrev(frame)->topOfStack;
#define CACHE_FRAME()	 ee->interpreterStack.currentFrame
#define DECACHE_FRAME()	 ee->interpreterStack.currentFrame = frame;

#ifdef CVM_DEBUG_ASSERTS
static CVMBool
inFrame(CVMFrame* frame, CVMStackVal32* tos)
{
    if (CVMframeIsCompiled(frame)) {
	CVMCompiledMethodDescriptor *cmd = CVMmbCmd(frame->mb);
	CVMStackVal32* base = (CVMStackVal32 *)frame - CVMcmdMaxLocals(cmd);
	CVMStackVal32* top  = base + CVMcmdCapacity(cmd);
	return ((tos >= base) && (tos <= top));
    } else if (CVMframeIsJava(frame)) {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(frame->mb);
	CVMStackVal32* base = CVMframeOpstack(frame, Java);
	CVMStackVal32* top  = base + CVMjmdMaxStack(jmd);
	return ((tos >= base) && (tos <= top));
    } else {
	return CVM_TRUE;
    }
}
#endif


#define frameSanity(f, tos)	inFrame((f), (tos))

CVMCompiledResultCode
CVMinvokeCompiledHelper(CVMExecEnv *ee, CVMFrame *frame,
			CVMMethodBlock **mb_p)
{
    CVMMethodBlock *mb = *mb_p;
    CVMStackVal32 *topOfStack;

    CACHE_TOS();

check_mb:

    if (mb == NULL) {

        /* Make sure there isn't an exception thrown first: */
        if (CVMexceptionOccurred(ee)) {
            return CVM_COMPILED_EXCEPTION;
        }

	/*
	 * Do this check after we check the exception, because
	 * topOfStack might be stale if an exception was
	 * thrown.
	 */
	CVMassert(frameSanity(frame,topOfStack));

	/* Support for CNI calls */
	if (CVMframeIsTransition(frame)) {
	    *mb_p = frame->mb;
	    return CVM_COMPILED_NEW_TRANSITION;
	}

        /* If we're not invoking a transition method, then we must be returning
           from this compiled method: */
        TRACE_METHOD_RETURN(frame);

	CVMassert(CVMframeIsCompiled(frame));

#if 0
	CVMD_gcSafeCheckPoint(ee, {}, {});
#endif

	mb = frame->mb;

	if (CVMmbIs(mb, SYNCHRONIZED)) {
	    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
	    CVMStackVal32* locals = (CVMStackVal32 *)frame -
		CVMcmdMaxLocals(cmd);
	    CVMObjectICell* retObjICell = &locals[0].j.r;
	    CVMObjectICell* receiverObjICell =
		&CVMframeReceiverObj(frame, Compiled);
	    if (!CVMfastTryUnlock(ee,
		CVMID_icellDirect(ee, receiverObjICell)))
	    {
		CVMBool areturn =
		    CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb)) ==
			CVM_TYPEID_OBJ;
		CVMBool success;

		CVMcompiledFramePC(frame) =
		    (CVMUint8*)CONSTANT_HANDLE_GC_FOR_RETURN;
		success = CVMsyncReturnHelper(ee, frame, retObjICell, areturn);
		if (!success) {
		    CVMassert(frameSanity(frame,topOfStack));
		    return CVM_COMPILED_EXCEPTION;
		}
	    }
	}

	CVMpopFrameSpecial(&ee->interpreterStack, frame, {
	    /* changing stack chunk */
	    CVMFrame *prev = prev_;
	    int	retType = CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb));
	    if (retType == CVM_TYPEID_VOID) {
		topOfStack = prev->topOfStack;
	    } else if (retType == CVM_TYPEID_LONG ||
		retType == CVM_TYPEID_DOUBLE)
	    {
		CVMmemCopy64(&prev->topOfStack[0].j.raw,
		    &topOfStack[-2].j.raw);
		topOfStack = prev->topOfStack + 2;
	    } else {
		prev->topOfStack[0] = topOfStack[-1];
		topOfStack = prev->topOfStack + 1;;
	    }
	});

	CVMassert(frameSanity(frame,topOfStack));
	DECACHE_TOS();

	if (CVMframeIsCompiled(frame)) {
	    goto returnToCompiled;
	} else {
	    DECACHE_FRAME();
	    return CVM_COMPILED_RETURN;
	}
    } else {
	int invokerId;
	topOfStack -= CVMmbArgsSize(mb);
new_mb:
	invokerId = CVMmbInvokerIdx(mb);

	CVMassert(frameSanity(frame,topOfStack));

	if (invokerId < CVM_INVOKE_CNI_METHOD) {
            /* This means that invokerId == CVM_INVOKE_JAVA_METHOD or
               CVM_INVOKE_JAVA_SYNC_METHOD: */

	    CVMInt32 cost;
	    CVMInt32 oldCost;
	    /* Java method */

	    if (CVMmbIsCompiled(mb)) {
		CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
		CVMObjectICell*   receiverObjICell;
		CVMFrame *prev = frame;
		CVMBool needExpand = CVM_FALSE;

		CVMassert(frameSanity(frame,topOfStack));

                /* If we get here, then we're about to call a compiled method
                   from an interpreted, or a transition method.  JNI methods
                   would have to go through a transition method to call any
                   method.  Hence the caller cannot be a JNI method.

                   If the caller is an interpreted method, then we would want
                   to increment its invokeCost because this transition makes
                   the caller more desirable for compilation.

                   If the caller is a transition method, then the mb might
		   be abstract, in which case we can't make any adjustment
		   to invokeCost.
                */
		if (!CVMframeIsTransition(frame)) {
		    CVMMethodBlock* callerMb = frame->mb;
		    oldCost = CVMmbInvokeCost(callerMb);
		    cost = oldCost - CVMglobals.jit.mixedTransitionCost;
		    if (cost < 0) {
			cost = 0;
		    }
		    if (cost != oldCost){
			CVMmbInvokeCostSet(callerMb, cost);
		    }
		}

		if (CVMmbIs(mb, STATIC)) {
		    CVMClassBlock* cb = CVMmbClassBlock(mb);
		    receiverObjICell = CVMcbJavaInstance(cb);
		} else {
		    receiverObjICell = &topOfStack[0].ref;
		}

		CVMassert(frameSanity(frame,topOfStack));

		/*
		 * Make sure we don't decompile this method if we become
		 * gcSafe during the call to CVMpushFrame().
		 */
                CVMassert(ee->invokeMb == NULL);
		ee->invokeMb = mb;
		CVMpushFrame(ee, &ee->interpreterStack, frame, topOfStack,
			     CVMcmdCapacity(cmd), CVMcmdMaxLocals(cmd),
			     CVM_FRAMETYPE_COMPILED, mb, CVM_FALSE,
		/* action if pushFrame fails */
		{
                    /* CVMpushFrame() set 'frame' to NULL.  So, do the sanity
                       check on the re-cache value of the currentFrame on the
                       stack instead: */
		    CVMassert(frameSanity(CACHE_FRAME(),topOfStack));
		    ee->invokeMb = NULL;
		    return CVM_COMPILED_EXCEPTION;
		},
		/* action to take if stack expansion occurred */
		{
		    TRACE(("pushing JavaFrame caused stack expansion\n"));
		    needExpand = CVM_TRUE;
		});

		CVMassert(frameSanity(prev, topOfStack));

		/* Use the interpreter -> compiled entry point */
		CVMcompiledFramePC(frame) = CVMcmdStartPCFromInterpreted(cmd);
		CVM_RESET_COMPILED_TOS(frame->topOfStack, frame);

		TRACE_METHOD_CALL(frame, CVM_FALSE);

#if 0
		/* Force GC for testing purposes. */
		CVMD_gcSafeExec(ee, {
		    CVMgcRunGC(ee);
		});
#endif
		if (CVMmbIs(mb, SYNCHRONIZED)) {
#ifdef CVM_JVMTI
		    /* No events during this delicate phase of creating
		     * a frame */
		    CVMBool jvmtiEvents = CVMjvmtiDebugEventsEnabled(ee);
		    CVMjvmtiDebugEventsEnabled(ee) = CVM_FALSE;
#endif
                    /* The method is sync, so lock the object. */
                    /* %comment l002 */
                    if (!CVMfastTryLock(ee,
			CVMID_icellDirect(ee, receiverObjICell)))
		    {
			if (!CVMobjectLock(ee, receiverObjICell)) {
                            CVMthrowOutOfMemoryError(ee, NULL);
			    CVMassert(frameSanity(frame, topOfStack));
 			    ee->invokeMb = NULL;
#ifdef CVM_JVMTI
			    CVMjvmtiDebugEventsEnabled(ee) = jvmtiEvents;
#endif
                           return CVM_COMPILED_EXCEPTION;
			}
                    }
		    CVMID_icellAssignDirect(ee,
			&CVMframeReceiverObj(frame, Compiled),
			receiverObjICell);
#ifdef CVM_JVMTI
		    CVMjvmtiDebugEventsEnabled(ee) = jvmtiEvents;
#endif
		}

		DECACHE_FRAME();
		if (needExpand) {
#ifdef CVM_DEBUG_ASSERTS
		    CVMStackVal32* space =
#endif
			CVMexpandStack(ee, &ee->interpreterStack,
			    CVMcmdCapacity(cmd), CVM_TRUE, CVM_FALSE);
		    CVMassert((CVMFrame*)&space[CVMcmdMaxLocals(cmd)] ==
			      frame);

		    /* Since the stack expanded, we need to set locals
		     * to the new frame minus the number of locals.
		     * Also, we need to copy the arguments, which
		     * topOfStack points to, to the new locals area.
		     */
		    memcpy((CVMSlotVal32*)frame - CVMcmdMaxLocals(cmd),
			   topOfStack,
			   CVMmbArgsSize(mb) * sizeof(CVMSlotVal32));
		}

		/*
		 * GC-safety stage 2: Update caller frame's topOfStack to
		 * exclude the arguments. Once we update our new frame
		 * state, the callee frame's stackmap will cover the arguments
		 * as incoming locals.
		 */
		prev->topOfStack = topOfStack;

		CVMassert(frameSanity(prev, topOfStack));
		CVMassert(frame->mb == mb);

		ee->invokeMb = NULL;

		/* Invoke the compiled method */
                if ((CVMUint8*)cmd >= CVMglobals.jit.codeCacheDecompileStart
#ifdef CVM_AOT
	            && !((CVMUint8*)cmd >= CVMglobals.jit.codeCacheAOTStart &&
                         (CVMUint8*)cmd < CVMglobals.jit.codeCacheAOTEnd)
#endif
                   )
                {
		    CVMcmdEntryCount(cmd)++;
                }
		mb = CVMinvokeCompiled(ee, CVMgetCompiledFrame(frame));
		frame = CACHE_FRAME();
		CACHE_TOS();
		goto check_mb;
	    }

            /*
	     * If we get here, then we're about to call an interpreted method
	     * from a compiled method.  Hence, we increment the invokeCost on
	     * the callee because this transition makes the callee more
	     * desirable for compilation.
	     *
	     * NOTE: we add in interpreterTransitionCost because this
	     * value will also be subtracted when the interpreter loop
	     * does this invocation for us and we only want to count
	     * this invocation is a mixed one, not both a mixed one
	     * and an interpreted-to-interpreted one.
	     */
	    oldCost = CVMmbInvokeCost(mb);
	    cost = oldCost - CVMglobals.jit.mixedTransitionCost;
	    cost += CVMglobals.jit.interpreterTransitionCost;
	    if (cost < 0) {
		cost = 0;
	    }
	    if (cost != oldCost){
		CVMmbInvokeCostSet(mb, cost);
	    }

	    *mb_p = mb;
	    CVMassert(frameSanity(frame,topOfStack));
	    return CVM_COMPILED_NEW_MB;
	} else if (invokerId < CVM_INVOKE_JNI_METHOD) {
            /* This means that invokerId == CVM_INVOKE_CNI_METHOD: */
	    /* CNI */
	    CVMStack *stack = &ee->interpreterStack;
	    CNINativeMethod *f = (CNINativeMethod *)CVMmbNativeCode(mb);
	    CNIResultCode ret;

	    TRACE_FRAMELESS_METHOD_CALL(frame, mb, CVM_FALSE);

	    ret = (*f)(ee, topOfStack, &mb);

	    TRACE_FRAMELESS_METHOD_RETURN(mb, frame);

	    if ((int)ret >= 0) {
		topOfStack += (int)ret;
                DECACHE_TOS();
		CVMassert(frame == stack->currentFrame); (void)stack;
		goto returnToCompiled;
	    } else if (ret == CNI_NEW_TRANSITION_FRAME) {
		CVMassert(frameSanity(frame,topOfStack));
		/* pop invoker's arguments. Must be done
		 * before CACHE_FRAME() */
		DECACHE_TOS();
		/* get the transition frame. */
		frame = CACHE_FRAME();
		CVMassert(frameSanity(frame, frame->topOfStack));
		return CVM_COMPILED_NEW_TRANSITION;
	    } else if (ret == CNI_NEW_MB) {
		CVMassert(frame == stack->currentFrame); (void)stack;
		CVMassert(frameSanity(frame,topOfStack));

		DECACHE_TOS();
		frame->topOfStack += CVMmbArgsSize(mb);

		CVMassert(frameSanity(frame, frame->topOfStack));

		goto new_mb;
	    } else if (ret == CNI_EXCEPTION) {
		CVMassert(frame == stack->currentFrame); (void)stack;
		CVMassert(frameSanity(frame, frame->topOfStack));

		return CVM_COMPILED_EXCEPTION;
	    } else {
		CVMdebugPrintf(("Bad CNI result code"));
		CVMassert(CVM_FALSE);
	    }
	} else if (invokerId < CVM_INVOKE_ABSTRACT_METHOD) {
            /* This means that invokerId == CVM_INVOKE_JNI_METHOD or
               CVM_INVOKE_JNI_SYNC_METHOD: */
	    /* JNI */
	    CVMBool ok;

	    TRACE_METHOD_CALL(frame, CVM_FALSE);

	    ok = CVMinvokeJNIHelper(ee, mb);

	    TRACE_METHOD_RETURN(frame);

	    if (ok) {
		goto returnToCompiled;
	    } else {
		CVMassert(frameSanity(frame, frame->topOfStack));
		return CVM_COMPILED_EXCEPTION;
	    }

        } else if (invokerId == CVM_INVOKE_ABSTRACT_METHOD) {
            CVMthrowAbstractMethodError(ee, "%C.%M", CVMmbClassBlock(mb), mb);
            return CVM_COMPILED_EXCEPTION;

#ifdef CVM_CLASSLOADING
        } else if (invokerId == CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD) {
            /* It's a miranda method created to deal with a non-public method
               with the same name as an interface method: */
            CVMthrowIllegalAccessError(ee,
                "access non-public method %C.%M through an interface",
                CVMmbClassBlock(mb), CVMmbMirandaInterfaceMb(mb));
            return CVM_COMPILED_EXCEPTION;

        } else if (invokerId == CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD) {
            /* It's a miranda method created to deal with a missing interface
               method: */
            CVMthrowAbstractMethodError(ee, "%C.%M", CVMmbClassBlock(mb), mb);
            return CVM_COMPILED_EXCEPTION;

        } else if (invokerId == CVM_INVOKE_LAZY_JNI_METHOD) {
            /*
             * It's a native method of a dynamically loaded class.
             * We still need to lookup the native code.
             */
            CVMBool result;
            *mb_p = mb;
            CVMD_gcSafeExec(ee, {
                result = CVMlookupNativeMethodCode(ee, mb);
            });
            if (!result) {
                return CVM_COMPILED_EXCEPTION;
            } else {
                /*
                 * CVMlookupNativeMethod() stored the pointer to the
                 * native method and also changed the invoker index, so
                 * just branch to new_mb and the correct native
                 * invoker will be used.
                 */
                goto new_mb;
            }
#endif
	} else {
            CVMdebugPrintf(("ERROR: Method %C.%M(), invokerID = %d\n",
			    CVMmbClassBlock(mb), mb, invokerId));
	    CVMassert(CVM_FALSE);
	    return CVM_COMPILED_EXCEPTION;
	}
    }
    CVMassert(CVM_FALSE); /* unreachable */

returnToCompiled:

    CVMassert(frameSanity(frame, topOfStack));

    /* Return to the compiled method */
    mb = CVMreturnToCompiled(ee, CVMgetCompiledFrame(frame), NULL);
    frame = CACHE_FRAME();
    CACHE_TOS();
    goto check_mb;
}

CVMCompiledResultCode
CVMreturnToCompiledHelper(CVMExecEnv *ee, CVMFrame *frame,
			  CVMMethodBlock **mb_p, CVMObject* exceptionObject)
{
    CVMCompiledResultCode resCode;
    CVMCompiledFrame *cframe = CVMgetCompiledFrame(frame);

    *mb_p = CVMreturnToCompiled(ee, cframe, exceptionObject);

    frame = CACHE_FRAME();

    resCode = CVMinvokeCompiledHelper(ee, frame, mb_p);

    frame = CACHE_FRAME();
    CVMassert(resCode == CVM_COMPILED_EXCEPTION ||
	      frameSanity(frame, frame->topOfStack));

    return resCode;
}

/* Purpose: Do On-Stack replacement of an interpreted stack frame with a
            compiled stack frame and continue executing the method using its
            compiled form at the location indicated by the specified bytecode
            PC. */
CVMCompiledResultCode
CVMinvokeOSRCompiledHelper(CVMExecEnv *ee, CVMFrame *frame,
                           CVMMethodBlock **mb_p, CVMUint8 *pc)
{
    CVMStack *stack = &ee->interpreterStack;
    CVMCompiledResultCode resCode;
    CVMMethodBlock *mb = *mb_p;
    CVMObjectICell *receiverObjICell;
    CVMJavaMethodDescriptor *jmd;
    CVMCompiledMethodDescriptor *cmd;
    CVMFrame *oldFrame = frame;
    CVMBool needExpand = CVM_FALSE;

    CVMassert(CVMmbIsCompiled(mb));
    jmd = CVMmbJmd(mb);
    cmd = CVMmbCmd(mb);

    /*
     * Make sure we don't decompile this method if we become
     * gcSafe during the call to CVMreplaceFrame().
     */
    CVMassert(ee->invokeMb == NULL);
    ee->invokeMb = mb;

    /* Save off the receiverObject: */
    receiverObjICell = CVMsyncICell(ee);
    CVMassert(CVMID_icellIsNull(receiverObjICell));
    CVMID_icellAssignDirect(ee, receiverObjICell,
                            &CVMframeReceiverObj(frame, Java));

    /* Convert the Java frame into a Compiled frame: */
    CVMreplaceFrame(ee, stack, frame,
                    sizeof(CVMJavaFrame) - sizeof(CVMSlotVal32),
                    CVMjmdMaxLocals(jmd),
                    CVMcmdCapacity(cmd), CVMcmdMaxLocals(cmd),
                    CVM_FRAMETYPE_COMPILED, mb, CVM_FALSE,
    /* action if replaceFrame fails */
    {
        /* We should never fail because we have already verified
           that the replacement frame will fit before we actually
           do the replacement. */
        CVMassert(CVM_FALSE);
    },
    /* action to take if stack expansion occurred */
    {
        TRACE(("replacing JavaFrame caused stack expansion\n"));
        needExpand = CVM_TRUE;
    });

    /* NOTE: We cannot become GC safe after CVMreplaceFrame() until we finish
       replacing the frame below: */

    /* If the stack was expanded, then we need to copy the locals to new frame
       in the new stack chunk: */
    if (needExpand) {
        CVMUint16 oldNumberOfLocals = CVMjmdMaxLocals(jmd);
        CVMSlotVal32 *oldFrameStart;
        CVMStackChunk *prevChunk;

        oldFrameStart = (CVMSlotVal32*)oldFrame - oldNumberOfLocals;
        memcpy((CVMSlotVal32*)frame - CVMcmdMaxLocals(cmd), oldFrameStart,
               oldNumberOfLocals * sizeof(CVMSlotVal32));

        /* We're here because we have expanded the stack in order to install
           the replacement frame.  However, the oldFrame may be the only frame
           in the previous chunk.  In that case, since the oldFrame is no
           longer in use, we need to delete that chunk.
        */
        prevChunk = CVMstackGetCurrentChunk(stack)->prev;
        if (CVMaddressIsAtStartOfChunk(oldFrameStart, prevChunk)) {
            /* If we get here, then there is nothing else in the prevChunk
               except for the oldFrame.  Hence, we should delete that chunk:
            */
            CVMstackDeleteChunk(stack, prevChunk);
        }

#ifdef CVM_TRACE_JIT
        CVMtraceJITOSR(("OSR: Intr2Comp across stack chunk: %C.%M\n",
                        CVMmbClassBlock(mb), mb));
    } else {
        CVMtraceJITOSR(("OSR: Intr2Comp: %C.%M\n", CVMmbClassBlock(mb), mb));
#endif
    }

    /* Re-set the receiverObj: */
    CVMID_icellAssignDirect(ee, &CVMframeReceiverObj(frame, Compiled),
                            receiverObjICell);
    CVMID_icellSetNull(receiverObjICell);

    /* Get the interpreter -> compiled entry point: */
    CVMcompiledFramePC(frame) =
        CVMpcmapJavaPcToCompiledPcStrict(frame->mb, pc);

#ifdef CVMCPU_HAS_CP_REG
    /* Set the constantpool base reg: */
    CVMcompiledFrameCpBaseReg(frame) = CVMcmdCPBaseReg(cmd);
#endif

    CVMassert(frame->mb == mb);
    DECACHE_FRAME();

    /* NOTE: The frame has been replaced completedly.  Now, we can become GC
             safe again. */
#if 0
    /* Force GC for testing purposes. */
    CVMD_gcSafeExec(ee, {
        CVMgcRunGC(ee);
    });
#endif

    /* When are not supposed to have any arguments on the stack when we
       do OSR.  Normally, the method prologue will adjust the stack by
       the spill adjust size.  We'll have to do it ourselves in this case
       since we're not going through the method prologue. */
    CVM_RESET_COMPILED_TOS(frame->topOfStack, frame);
    frame->topOfStack += CVMcmdSpillSize(CVMmbCmd(frame->mb));

    ee->invokeMb = NULL;

    /* Continue executing in the compiled method: */
#ifdef CVM_AOT
    /* Don't adjust entry count for AOT methods. */
    if (!((CVMUint8*)cmd >= CVMglobals.jit.codeCacheAOTStart &&
        (CVMUint8*)cmd < CVMglobals.jit.codeCacheAOTEnd))
#endif
    {
        CVMcmdEntryCount(cmd)++;
    }
    resCode = CVMreturnToCompiledHelper(ee, frame, mb_p, NULL);

    return resCode;
}

/* Order must match CVMJITWhenToCompileOption enum */
static const char* const jitWhenToCompileOptions[] = {
    "none", "all", "policy"
};

const CVMSubOptionEnumData jitInlineOptions[] = {
    { "none",       0 },
    { "default",    CVMJIT_DEFAULT_INLINING },
    { "all",        ((1 << CVMJIT_INLINE_VIRTUAL) |
                     (1 << CVMJIT_INLINE_NONVIRTUAL) |
                     (1 << CVMJIT_INLINE_USE_VIRTUAL_HINTS) |
                     (1 << CVMJIT_INLINE_USE_INTERFACE_HINTS)) },
    { "virtual",    (1 << CVMJIT_INLINE_VIRTUAL) },
    { "nonvirtual", (1 << CVMJIT_INLINE_NONVIRTUAL) },
    { "vhints",     (1 << CVMJIT_INLINE_USE_VIRTUAL_HINTS) },
    { "ihints",     (1 << CVMJIT_INLINE_USE_INTERFACE_HINTS) },
    { "Xvsync",     (1 << CVMJIT_INLINE_VIRTUAL_SYNC) },
    { "Xnvsync",    (1 << CVMJIT_INLINE_NONVIRTUAL_SYNC) },
    { "Xdopriv",    (1 << CVMJIT_INLINE_DOPRIVILEGED) },
};

#ifdef CVM_JIT_COLLECT_STATS
/* Order must match CVMJITStatsToCollectOption enum */
static const char* const jitStatsToCollectOptions[] = {
    "help", "none", "minimal", "more", "verbose", "constant", "maximal"
};
#endif

#ifdef CVM_TRACE_JIT
#define CVMJIT_DEFAULT_TRACE_OPTIONS	0
const CVMSubOptionEnumData jitTraceOptions[] = {
    { "none",           0 },
    { "default",        CVMJIT_DEFAULT_TRACE_OPTIONS },
    { "all",            0xffffffff },
    { "status",         CVM_DEBUGFLAG(TRACE_JITSTATUS) },
    { "error",          CVM_DEBUGFLAG(TRACE_JITERROR) },
    { "bctoir",         CVM_DEBUGFLAG(TRACE_JITBCTOIR) },
    { "codegen",        CVM_DEBUGFLAG(TRACE_JITCODEGEN) },
    { "stats",          CVM_DEBUGFLAG(TRACE_JITSTATS) },
    { "iropt",          CVM_DEBUGFLAG(TRACE_JITIROPT) },
    { "inlining",       CVM_DEBUGFLAG(TRACE_JITINLINING) },
    { "osr",            CVM_DEBUGFLAG(TRACE_JITOSR) },
    { "reglocals",      CVM_DEBUGFLAG(TRACE_JITREGLOCALS) },
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    { "pmi",            CVM_DEBUGFLAG(TRACE_JITPATCHEDINVOKES) },
#endif
};
#endif

static const CVMSubOptionData knownJitSubOptions[] = {

    {"icost", "Interpreter transition cost", 
     CVM_INTEGER_OPTION, 
     {{0, CVMJIT_MAX_INVOKE_COST / 2, CVMJIT_DEFAULT_ICOST}},
     &CVMglobals.jit.interpreterTransitionCost},

    {"mcost", "Mixed transition cost", 
     CVM_INTEGER_OPTION, 
     {{0, CVMJIT_MAX_INVOKE_COST / 2, CVMJIT_DEFAULT_MCOST}},
     &CVMglobals.jit.mixedTransitionCost},

    {"bcost", "Backwards branch cost", 
     CVM_INTEGER_OPTION, 
     {{0, CVMJIT_MAX_INVOKE_COST / 2, CVMJIT_DEFAULT_BCOST}},
     &CVMglobals.jit.backwardsBranchCost},

    {"climit", "Compilation threshold", 
     CVM_INTEGER_OPTION, 
     {{0, CVMJIT_MAX_INVOKE_COST, CVMJIT_DEFAULT_CLIMIT}},
     &CVMglobals.jit.compileThreshold},

    {"compile", "When to compile", 
     CVM_MULTI_STRING_OPTION, 
     {{CVMJIT_COMPILE_NUM_OPTIONS, 
       (CVMAddr)jitWhenToCompileOptions, CVMJIT_DEFAULT_POLICY}},
     &CVMglobals.jit.whenToCompile},

    {"inline", "What to inline",
    CVM_ENUM_OPTION,
    {{sizeof(jitInlineOptions)/sizeof(jitInlineOptions[0]),
      (CVMAddr)jitInlineOptions, CVMJIT_DEFAULT_INLINING}},
    &CVMglobals.jit.whatToInline},

    {"maxInliningDepth", "Max Inlining Depth", 
     CVM_INTEGER_OPTION, 
     {{0, 1000, CVMJIT_DEFAULT_MAX_INLINE_DEPTH}},
     &CVMglobals.jit.maxAllowedInliningDepth},

    {"maxInliningCodeLength", "Max Inlining Code Length", 
     CVM_INTEGER_OPTION, 
     {{0, 1000, CVMJIT_DEFAULT_MAX_INLINE_CODELEN}},
     &CVMglobals.jit.maxInliningCodeLength},

    {"minInliningCodeLength", "Min Inlining Code Length", 
     CVM_INTEGER_OPTION, 
     {{0, 1000, CVMJIT_DEFAULT_MIN_INLINE_CODELEN}},
     &CVMglobals.jit.minInliningCodeLength},

    {"policyTriggeredDecompilations", "Policy Triggered Decompilations", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.policyTriggeredDecompilations},

    {"maxWorkingMemorySize", "Max Working Memory Size", 
     CVM_INTEGER_OPTION, 
     {{0, 64*1024*1024, CVMJIT_DEFAULT_MAX_WORKING_MEM}},
     &CVMglobals.jit.maxWorkingMemorySize},

    {"maxCompiledMethodSize", "Max Compiled Method Size", 
     CVM_INTEGER_OPTION, 
     {{0, 64*1024 - 1, CVMJIT_DEFAULT_MAX_COMP_METH_SIZE}},
     &CVMglobals.jit.maxCompiledMethodSize},

    {"codeCacheSize", "Code Cache Size", 
     CVM_INTEGER_OPTION, 
     {{0, CVMJIT_MAX_CODE_CACHE_SIZE, CVMJIT_DEFAULT_CODE_CACHE_SIZE}},
     &CVMglobals.jit.codeCacheSize},

    {"upperCodeCacheThreshold", "Upper Code Cache Threshold", 
     CVM_PERCENT_OPTION, 
     {{0, 100, CVMJIT_DEFAULT_UPPER_CCACHE_THR}},
     &CVMglobals.jit.upperCodeCacheThresholdPercent},

    {"lowerCodeCacheThreshold", "Lower Code Cache Threshold", 
     CVM_PERCENT_OPTION, 
     {{0, 100, (CVMAddr)CVMJIT_DEFAULT_LOWER_CCACHE_THR}},
     &CVMglobals.jit.lowerCodeCacheThresholdPercent},

#ifdef CVM_AOT
    {"aot", "Enable AOT",
     CVM_BOOLEAN_OPTION,
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.aotEnabled},

    {"aotFile", "AOT File Path",
     CVM_STRING_OPTION,
     {{0, (CVMAddr)"<AOT file>", 0}},
     &CVMglobals.jit.aotFile},

    {"recompileAOT", "Recompile AOT code",
     CVM_BOOLEAN_OPTION,
     {{CVM_FALSE, CVM_TRUE, CVM_FALSE}},
     &CVMglobals.jit.recompileAOT},

    {"aotCodeCacheSize", "AOT Code Cache Size",
     CVM_INTEGER_OPTION,
     {{0, CVMJIT_MAX_CODE_CACHE_SIZE, CVMJIT_DEFAULT_AOT_CODE_CACHE_SIZE}},
     &CVMglobals.jit.aotCodeCacheSize},

    {"aotMethodList", "List of Method to be compiled ahead of time", 
     CVM_STRING_OPTION, 
     {{0, (CVMAddr)"<filename>", 0}},
     &CVMglobals.jit.aotMethodList},
#endif

#define CVM_JIT_EXPERIMENTAL_OPTIONS
#ifdef  CVM_JIT_EXPERIMENTAL_OPTIONS
    {"XregisterPhis", "Pass Phi values in registers", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.registerPhis},

#ifdef CVM_JIT_REGISTER_LOCALS
    {"XregisterLocals", "Pass locals in registers between blocks", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.registerLocals},
#endif
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
#ifdef CVM_DEBUG
    {"XremoveNOP", "Remove NOPs from generated code", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.codeSchedRemoveNOP},
#endif
    {"XcodeScheduling", "Enable code scheduling",
     CVM_BOOLEAN_OPTION,
    {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
    &CVMglobals.jit.codeScheduling},
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/
    {"XcompilingCausesClassLoading", "Compiling Causes Class Loading", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_FALSE}},
     &CVMglobals.jit.compilingCausesClassLoading},
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    {"Xpmi", "Patched Method Invocations", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_TRUE}},
     &CVMglobals.jit.pmiEnabled},
#endif
#endif

#ifdef CVM_JIT_COLLECT_STATS
    {"stats", "Collect statistics about JIT activity", 
     CVM_MULTI_STRING_OPTION, 
     {{CVMJIT_STATS_NUM_OPTIONS,
       (CVMAddr)jitStatsToCollectOptions, CVMJIT_STATS_COLLECT_NONE}},
     &CVMglobals.jit.statsToCollect},
#endif

#ifdef CVM_JIT_PROFILE
    {"profile", "Enable profiling of jit compiled code", 
     CVM_STRING_OPTION, 
     {{0, (CVMAddr)"<filename>", 0}},
     &CVMglobals.jit.profile_filename},

    {"profileInstructions", "profile at the instruction level", 
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_FALSE}},
     &CVMglobals.jit.profileInstructions},
#endif

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    {"measureCSpeed", "Enable measurement of compilation speed",
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_FALSE}},
     &CVMglobals.jit.doCSpeedMeasurement},

    {"testCSpeed", "Run compilation speed test",
     CVM_BOOLEAN_OPTION, 
     {{CVM_FALSE, CVM_TRUE, CVM_FALSE}},
     &CVMglobals.jit.doCSpeedTest},
#endif

#ifdef CVM_TRACE_JIT
     {"trace", "Trace",
     CVM_ENUM_OPTION,
     {{sizeof(jitTraceOptions)/sizeof(jitTraceOptions[0]),
       (CVMAddr)jitTraceOptions, CVMJIT_DEFAULT_TRACE_OPTIONS}},
     &CVMglobals.debugJITFlags},
#endif

    {NULL, NULL, 0, {{0, 0, 0}}, NULL}
};

/* Purpose: Initializes the compilation policy data. */
CVMBool
CVMjitPolicyInit(CVMExecEnv* ee, CVMJITGlobalState* jgs)
{
    /* Some extra logic to reconcile the -Xjit:compile option with the rest */
    if (jgs->whenToCompile == CVMJIT_COMPILE_NONE) {
        /* Prevent stuff from getting compiled */
        jgs->interpreterTransitionCost = 0;
        jgs->mixedTransitionCost = 0;
        jgs->backwardsBranchCost = 0;
        jgs->compileThreshold = 1000;
    } else if (jgs->whenToCompile == CVMJIT_COMPILE_ALL) {
        jgs->compileThreshold = 0;
#if defined(CVM_AOT) || defined(CVM_MTASK)
        /* The [imb]cost were set to 0 earlier in CVMjitInit() to 
           prevent unwanted methods being compiled as AOT methods.
           Need to reset [im]cost here.
	 */
        jgs->interpreterTransitionCost = CVMJIT_DEFAULT_ICOST;
        jgs->mixedTransitionCost = CVMJIT_DEFAULT_MCOST;
        jgs->backwardsBranchCost = CVMJIT_DEFAULT_BCOST;
#endif
    } /* Otherwise do nothing.
         The default [im]cost and climit will do the work normally */

    return CVM_TRUE;
}

#if defined(CVM_AOT) || defined(CVM_MTASK)
/* During AOT compilation and MTASK warmup, dynamic compilation policy
 * is disabled. Patched method invocation (PMI) is also disabled during
 * that. This is used to re-initialize JIT options and policy after 
 * pre-compilation. For AOT, if there is existing AOT code, this is
 * called after initializing the AOT code.
 */
CVMBool
CVMjitProcessOptionsAndPolicyInit(
        CVMExecEnv* ee, CVMJITGlobalState* jgs)
{
    if (!CVMprocessSubOptions(knownJitSubOptions, "-Xjit",
			   &jgs->parsedSubOptions)) {
        return CVM_FALSE;
    }
    return CVMjitPolicyInit(ee, jgs);
}
#endif

/* Purpose: Set up the inlining threshold table. */
CVMBool
CVMjitSetInliningThresholds(CVMExecEnv* ee, CVMJITGlobalState* jgs)
{
    /* Set up the inlining threshold table: */
    CVMInt32 depth = jgs->maxAllowedInliningDepth;
    CVMInt32 thresholdLimit;
    CVMInt32 *costTable;
    CVMInt32 i;
    CVMInt32 effectiveMaxDepth;

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
#define QUAD_COEFFICIENT                100
#define EFFECTIVE_MAX_DEPTH_THRESHOLD   6
#undef MIN
#define MIN(x, y)                       (((x) < (y)) ? (x) : (y))

    if (depth == 0) {
        depth = 1;
    }
    costTable = malloc(depth * sizeof(CVMInt32));
    if (costTable == NULL) {
        return CVM_FALSE;
    }

    thresholdLimit = QUAD_COEFFICIENT *
                     EFFECTIVE_MAX_DEPTH_THRESHOLD *
                     EFFECTIVE_MAX_DEPTH_THRESHOLD;

    effectiveMaxDepth = MIN(depth, EFFECTIVE_MAX_DEPTH_THRESHOLD);
    for (i = 0; i < effectiveMaxDepth; i++) {
        CVMInt32 cost = (jgs->compileThreshold *
                         (QUAD_COEFFICIENT *i*i)) / thresholdLimit;
        costTable[i] = jgs->compileThreshold - cost;
        if (costTable[i] < 0) {
            costTable[i] = 0;
         }
    }
    for (; i < depth; i++) {
	costTable[i] = costTable[effectiveMaxDepth-1];
    }

    jgs->inliningThresholds = costTable;
#undef QUAD_COEFFICIENT
#undef EFFECTIVE_MAX_DEPTH_THRESHOLD
#undef MIN

    return CVM_TRUE;
}

static void
handleDoPrivileged()
{
    if (!CVMJITinlines(DOPRIVILEGED)) {
	CVMMethodBlock *mb0 =
	    CVMglobals.java_security_AccessController_doPrivilegedAction2;
	CVMMethodBlock *mb1 =
	    CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2;

	CVMmbCompileFlags(mb0) |= CVMJIT_NOT_INLINABLE;
	CVMmbCompileFlags(mb1) |= CVMJIT_NOT_INLINABLE;
    }
}

#ifdef CVM_MTASK
/*
 * Re-initialize the JIT state after a client JVM instance
 * has been created.
 */
CVMBool
CVMjitReinitialize(CVMExecEnv* ee, const char* subOptionsString)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    /* Remember those values we don't want overridden */
    CVMUint32 currentCodeCacheSize = jgs->codeCacheSize;
    
    CVMassert(!jgs->compiling);
    
    if (!CVMinitParsedSubOptions(&jgs->parsedSubOptions, subOptionsString)) {
	return CVM_FALSE;
    }
    if (!CVMprocessSubOptions(knownJitSubOptions, "-Xjit",
			      &jgs->parsedSubOptions)) {
	CVMjitPrintUsage();
	return CVM_FALSE;
    }
    /* Re-set codeCacheSize to what it was. That way, any overrides
       are ignored */
    jgs->codeCacheSize = currentCodeCacheSize;

    handleDoPrivileged();

    free(jgs->inliningThresholds);

    if (!CVMjitPolicyInit(ee, jgs)) {
        return CVM_FALSE;
    }

    if (!CVMjitSetInliningThresholds(ee, jgs)) {
        return CVM_FALSE;
    }

    /* Forget any collected stats if any */
    CVMJITstatsDestroyGlobalStats(&jgs->globalStats);
    /* And re-initialize if necessary */
    if (!CVMJITstatsInitGlobalStats(&jgs->globalStats)) {
        return CVM_FALSE;
    }    

    if (!CVMJITcodeCacheInitOptions(jgs)) {
        return CVM_FALSE;
    }    

#ifdef CVM_DEBUG
    CVMjitDumpSysInfo();
#endif

    return CVM_TRUE;
}
#endif

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the JIT. */
void CVMjitDumpSysInfo()
{
    CVMconsolePrintf("JIT Configuration:\n");
    CVMprintSubOptionValues(knownJitSubOptions);
}
#endif /* CVM_DEBUG || CVM_INSPECTOR */


#if defined(CVM_AOT) && !defined(CVM_MTASK)
/*
 * CVMjitCompileAOTCode() is called during VM startup. If there is
 * no existing AOT code, compile all the methods on 
 * CVMglobals.jit.aotMethodList. The compiled methods will be save 
 * as the AOT code.
 */
CVMBool
CVMjitCompileAOTCode(CVMExecEnv* ee)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    jstring jmlist;

    CVMassert(!jgs->aotCompileFailed);

    if (!jgs->aotEnabled) {
        return CVM_TRUE;
    }

    if (jgs->codeCacheAOTCodeExist) {
        return CVMJITinitializeAOTCode();
    } else {  
        CVMassert(jgs->isPrecompiling == CVM_TRUE);
        CVMassert(jgs->aotMethodList != NULL);

        jmlist = (*env)->NewStringUTF(env, jgs->aotMethodList);
        if ((*env)->ExceptionOccurred(env)) {
            return CVM_FALSE;
        }

        CVMjniCallStaticVoidMethod(env,
            CVMcbJavaInstance(CVMsystemClass(sun_misc_Warmup)),
            CVMglobals.sun_misc_Warmup_runit, NULL, jmlist);

        if (jgs->aotCompileFailed) {
	    return CVM_FALSE;
	}

        return CVM_TRUE;
    }
}
#endif

#ifdef CVMJIT_SIMPLE_SYNC_METHODS

/*
 * Simple Sync Method Names
 *
 * Array of method names that have simple synchronization requirements
 * and should be remapped to "Simple Sync" versions. This array provides
 * the symbolic mappings, and is processed to produce the mb->mb mappings
 * stored in jgs->simpleSyncMBs[].
 */
typedef struct {
    CVMClassBlock* cb;
    const char* methodName;
    const char* methodSig;
    const char* simpleSyncMethodName; /* uses methodSig as the signature */
    const CVMUint16 jitFlags; /* extra mb->jitFlagsx flags to set */
} CVMJITSimpleSyncMethodName;

static const CVMJITSimpleSyncMethodName CVMJITsimpleSyncMethodNames[] = {
/* FIXME - for now, don't include any simple sync methods */
#ifndef JAVASE
    {
	/* java.util.Random.next(I)I */
	CVMsystemClass(java_util_Random),
	"next", "(I)I",
	"nextSimpleSync",
        0
    },

    {
	/* java.util.Hashtable.size()I */
	CVMsystemClass(java_util_Hashtable),
	"size", "()I",
	"sizeSimpleSync",
        0
    },
    {
	/* java.util.Hashtable.isEmpty()Z */
	CVMsystemClass(java_util_Hashtable),
	"isEmpty", "()Z",
	"isEmptySimpleSync",
        0
    },
    {
	/* java.lang.String.<init>(Ljava.lang.StringBuffer;) */
	CVMsystemClass(java_lang_String),
	"<init>", "(Ljava/lang/StringBuffer;)V",
	"initSimpleSync",
	/* calls other simple methods, which must be inlined */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.lang.StringBuffer.length()I */
	CVMsystemClass(java_lang_StringBuffer),
	"length", "()I",
	"lengthSimpleSync",
        0
    },
    {
	/* java.lang.StringBuffer.capacity()I */
	CVMsystemClass(java_lang_StringBuffer),
	"capacity", "()I",
	"capacitySimpleSync",
        0
    },
    {
	/* java.lang.StringBuffer.charAt(I)C */
	CVMsystemClass(java_lang_StringBuffer),
	"charAt", "(I)C",
	"charAtSimpleSync",
        0
    },
    {
	/* java.lang.StringBuffer.append(C)Ljava/lang/StringBuffer; */
	CVMsystemClass(java_lang_StringBuffer),
	"append", "(C)Ljava/lang/StringBuffer;",
	"appendSimpleSync",
        0
    },
    {
	/* java.util.Vector.capacity()I */
	CVMsystemClass(java_util_Vector),
	"capacity", "()I",
	"capacitySimpleSync",
        0
    },
    {
	/* java.util.Vector.size()I */
	CVMsystemClass(java_util_Vector),
	"size", "()I",
	"sizeSimpleSync",
        0
    },
    {
	/* java.util.Vector.isEmpty()Z */
	CVMsystemClass(java_util_Vector),
	"isEmpty", "()Z",
	"isEmptySimpleSync",
        0
    },
    {
	/* java.util.Vector.elementAt(I)Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector),
	"elementAt", "(I)Ljava/lang/Object;",
	"elementAtSimpleSync",
	/* calls get0, which really does the simple sync work */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.util.Vector.firstElement()Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector),
	"firstElement", "()Ljava/lang/Object;",
	"firstElementSimpleSync",
	/* calls get0, which really does the simple sync work */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.util.Vector.lastElement()Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector),
	"lastElement", "()Ljava/lang/Object;",
	"lastElementSimpleSync",
        0
    },
    {
	/* java.util.Vector.setElementAt(Ljava/lang/Object;I)V */
	CVMsystemClass(java_util_Vector),
	"setElementAt", "(Ljava/lang/Object;I)V",
	"setElementAtSimpleSync",
	/* calls set0, which really does the simple sync work */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.util.Vector.addElement(Ljava/lang/Object;)V */
	CVMsystemClass(java_util_Vector),
	"addElement", "(Ljava/lang/Object;)V",
	"addElementSimpleSync",
        0
    },
    {
	/* java.util.Vector.get(I)Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector),
	"get", "(I)Ljava/lang/Object;",
	"getSimpleSync",
	/* calls get0, which really does the simple sync work */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.util.Vector.set(ILjava/lang/Object;)Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector),
	"set", "(ILjava/lang/Object;)Ljava/lang/Object;",
	"setSimpleSync",
	/* calls set0, which really does the simple sync work */
	CVMJIT_NEEDS_TO_INLINE
    },
    {
	/* java.util.Vector$1.nextElement()Ljava/lang/Object; */
	CVMsystemClass(java_util_Vector_1),
	"nextElement", "()Ljava/lang/Object;",
	"nextElementSimpleSync",
        0
    },
#endif /* !JAVASE */
};

#define CVM_JIT_NUM_SIMPLESYNC_METHODS  \
    (sizeof(CVMJITsimpleSyncMethodNames) / sizeof(CVMJITSimpleSyncMethodName))

static CVMBool
CVMJITsimpleSyncInit(CVMExecEnv* ee, CVMJITGlobalState* jgs)
{
    int i;
    CVMMethodTypeID methodTypeID; 
    CVMMethodBlock* mb;

    /* These method are always inlinable. They are called by the
     * simple sync methods, but are actually the ones that really
     * do the simple sync work. For example:
     *
     * private Object setSimpleSync(int index, Object element) {
     *	   return set0(index, element);
     * }
     *
     * We want to be agressive about inlining them.
     */
    methodTypeID = CVMtypeidLookupMethodIDFromNameAndSig(
	ee, "get0", "(IZ)Ljava/lang/Object;");
    mb = CVMclassGetMethodBlock(
       	CVMsystemClass(java_util_Vector), methodTypeID, CVM_FALSE);
    CVMassert(mb != NULL);
    CVMmbCompileFlags(mb) |= CVMJIT_ALWAYS_INLINABLE;

    methodTypeID = CVMtypeidLookupMethodIDFromNameAndSig(
	ee, "set0", "(ILjava/lang/Object;)Ljava/lang/Object;");
    mb = CVMclassGetMethodBlock(
       	CVMsystemClass(java_util_Vector), methodTypeID, CVM_FALSE);
    CVMassert(mb != NULL);
    CVMmbCompileFlags(mb) |= CVMJIT_ALWAYS_INLINABLE;

    /*
     * allocate jgs->simpleSyncMBs[]
     */
    jgs->numSimpleSyncMBs = CVM_JIT_NUM_SIMPLESYNC_METHODS;
    jgs->simpleSyncMBs = (void*)
	malloc(sizeof(jgs->simpleSyncMBs[0]) * CVM_JIT_NUM_SIMPLESYNC_METHODS);
    if (jgs->simpleSyncMBs == NULL) {
	return CVM_FALSE;
    }

    /*
     * initialize jgs->simpleSyncMBs based on the info in
     * CVMJITsimpleSyncMethodNames[]
     */
    for (i = 0; i < CVM_JIT_NUM_SIMPLESYNC_METHODS; i++) {
	/* lookup originalMB */
	methodTypeID = CVMtypeidLookupMethodIDFromNameAndSig(
	    ee, 
	    CVMJITsimpleSyncMethodNames[i].methodName, 
	    CVMJITsimpleSyncMethodNames[i].methodSig);
	mb = CVMclassGetMethodBlock(
            CVMJITsimpleSyncMethodNames[i].cb, methodTypeID, CVM_FALSE);
#if 0
	CVMconsolePrintf("%C.%!M\n",
			 CVMJITsimpleSyncMethodNames[i].cb,
			 methodTypeID);
#endif
	CVMassert(mb != NULL);
	jgs->simpleSyncMBs[i].originalMB = mb;

	/* lookup simpleSyncMB */
	methodTypeID = CVMtypeidLookupMethodIDFromNameAndSig(
	    ee, 
	    CVMJITsimpleSyncMethodNames[i].simpleSyncMethodName, 
	    CVMJITsimpleSyncMethodNames[i].methodSig);
	mb = CVMclassGetMethodBlock(
            CVMJITsimpleSyncMethodNames[i].cb, methodTypeID,  CVM_FALSE);
	CVMassert(mb != NULL);
	jgs->simpleSyncMBs[i].simpleSyncMB = mb;
	/* these methods are always inlinable */
	CVMmbCompileFlags(mb) |= CVMJIT_ALWAYS_INLINABLE;
	/* Possibly set CVMJIT_NEEDS_TO_INLINE flag */
	CVMmbCompileFlags(mb) |= CVMJITsimpleSyncMethodNames[i].jitFlags;
    }
    return CVM_TRUE;
}

static void
CVMJITsimpleSyncDestroy(CVMJITGlobalState* jgs)
{
    free(jgs->simpleSyncMBs);
    jgs->simpleSyncMBs = NULL;
}

#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS

/********************************************************************
  PMI - Patched Method Invocations support.
 
  Support for the patch table used to support direct method invocations.
  The patch table is used to track which code cache locations have direct
  calls so they can be patched as methods get compiled/decompiled or
  overridden for the first time. It has two main components:

   -A hash table used to lookup mb's that have direct method calls to them.
    This is referred to as the "Callee Table".

   -An array that contains codecache offsets of instructions that make
    direct method calls. This is referred to as the "Caller Table".

  The Caller Table is indexed by the entries in the Callee Table. Each
  Caller Table entry forms a linked list of entries. When an mb changes
  state, it looks itself up in the Callee Table. If it finds an entry
  for itself, then it knows there are direct method calls to it that
  need to be patched. The location of these calls are discovered by
  following the link (actually an array index) in the Callee Table
  entry to the first entry for the method in the Caller Table. From
  there each Caller Table method has a link (index) to the next entry
  for the method. This is how a method finds all code cache locations
  that need to be patched.

  Details for the Callee Table and Caller Table are below:
*/

/*
  CALLEE TABLE - Hash table for tracking callee records.

  Callee records (CVMJITPMICalleeRecords) are used to find all direct
  method calls to a given callee. For tracking CVMJITPMICalleeRecords,
  we use a hash table with open addressing and a double hashing
  (secondary hash used when there is a collision). The secondary hash
  value is a small prime number indexed out of the prime number table
  below. We use a table size that is relatively prime to the secondary
  hash (no factors common to any of the secondary hash primes). This
  guarantees that all entries will be traversed and we will get a good
  dispersal of entries in the table.

  The reason for using this type of hash table is that it strikes a
  good balance between minimal space and minimal lookup times. A
  simple array would use the least amount of space, but would result
  in either very slow lookups, or require extra space in each mb to
  map each mb to an entry (even ones with no patched calls to
  them). Any other type of data structure would waste space with link
  pointers.

  Since entries can be deleted, they need to be specially tagged so the
  search does not end when a deleted entry is found. If callerMb == NULL,
  this indicates an empty entry. If callerMb == 1, this indicates a
  deleted entry.

  The hash table grows if it gets too full, including counting
  entries marked as deleted. CVMJITPMI_CALLEETABLE_MAX_PERCENT_FULL
  determines when the table grows. Deleted entries are purged during
  this process.

  Each entry in the callee hash table is a CVMJITPMICalleeRecord,
  which contains indices for the first and last caller entries in the
  Caller Table (see Caller Table details below).  A "caller" is a
  patchable invoke site that calls the calleeMb.
*/
struct CVMJITPMICalleeRecord {
    CVMMethodBlock* calleeMb;       /* the method directly called */
    CVMUint16 firstCallerRecordIdx; /* first caller record in Caller Table */
    CVMUint16 lastCallerRecordIdx;  /* last caller record in Caller Table */
};

/* Primes used for secondary hash. We avoid 2, 3, and 5 so hash chains
   are not too condensed. */
static int CVMJITPMIsecondaryHashPrimes[] =
    {7, 11, 13, 17, 19, 23, 29, 31, 37};

#define CVMJITPMI_NUM_SECONDARY_HASH_PRIMES \
    (sizeof(CVMJITPMIsecondaryHashPrimes) / sizeof(int))

/* When the callee table is this % full, it must grow before being added to. */
#define CVMJITPMI_CALLEETABLE_MAX_PERCENT_FULL 80

/*
  CALLER TABLE: Table for tracking caller records.

  A caller record (CVMJITPMICallerRecord) is simply a mapping to a
  code cache address that contains a patchable direct method
  call. Since you get to a CVMJITPMICallerRecord via the
  CVMJITPMICalleeRecord, the method being called is always the one in
  the CVMJITPMICalleeRecord.

  A CVMJITPMICallerRecord contains a pair of patchable direct method
  calls. The reason is that for any given record we also need to find
  the next record in the sequence. Since there can't be more than 64k
  of entries, we need 16-bits to index to the next entry. However, we
  need more than 16-bits to index into the code cache. Thus every
  CVMJITPMICallerRecord contains two 23-bit code cache references and
  a 16-bit index to the next CVMJITPMICallerRecord. It also has a two
  1-bit "isVirtual" flags to indicate if the patches are for a virtual
  invokes of non-overridden methods. This makes a total of
  64-bits. The 16-bit index is split into two 8-bit values so the
  compiler can generated better code when accessing the two code cache
  indices, and also to guarantee that no alignment of the bits is
  done, which would result in more than 64-bits per record.
 
  The isVirtual flag is needed for two reasons. The first is to make
  sure that after a method is overridden and calls to it are patched
  to do a true virtual invoke, we don't then patch the calls later
  when the state of the original callee method changes between the
  compiled and decompiled states. Otherwise we'll start calling the
  original callee again rather than doing a true virtual invoke. The
  other reason is to properly handle non-virtual calls to virtual
  methods. We don't want to patch them to do a true virtual invoke
  when the method is overridden. These checks are handled in the
  CVMJITPMIpatchCall() funtion.

  It's possible for first_ccIndex to be non-zero and second_ccIndex to
  be 0, in which case any new patchable direct method call for the
  specified calleeMb will be added to second_ccIndex in this record.

  All freed up records are maintained on a link list, using nextIndex as
  the index to the next free record. This is is also true of records
  linked together for the same calleeMb.
*/

struct CVMJITPMICallerRecord {
    unsigned first_ccIndex    : 23;
    unsigned first_isVirtual  :  1;
    unsigned nextIndex_upper8 :  8;
    unsigned second_ccIndex   : 23;
    unsigned second_isVirtual :  1;
    unsigned nextIndex_lower8 :  8;
};


/* Estimate of average number of instructions per patchable
   instruction. Used to compute how many patch records we'll need in
   pmiCalleeRecords. A more accurate ratio is probably closer to 70
   to 1, but we are conservative on the high side. If the patchable
   instructions are more densily packed than the estimate, the table
   can always grow.
*/
#define CVMJITPMI_NUM_INSTRUCTIONS_PER_PATCH 200

/* Estimate of average number of caller records we'll have for each
   callee record. In other words, the average number of times a
   patchable call is made to a given method. Used to compute the
   number of mb entries that will be needed in pmiCallerRecords. A
   more accurate ratio is probably closer to 3 to 1. We are
   conservative on the high side. If we end up with a higher ratio of
   patch sites (callers) to callees, we will just end up expanding the
   hash table, which is fine.
*/
#define CVMJITPMI_NUM_CALLER_RECORDS_PER_CALLEE_RECORD 6


/* This value must be less than 0xffff since 0xffff is the index used
   to mark the end of the caller list. */
#define CVMJITPMI_MAX_CALLER_RECORDS 0xfffe

/* A number big enough to make sure that growing the table by a few
   percent will always result in some progress being made. */
#define CVMJITPMI_MIN_CALLER_RECORDS 100
#define CVMJITPMI_MIN_CALLEE_RECORDS 20

/************************
 * PMI Debugging APIs
 ************************/

#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
extern void
CVMJITPMIcallerTableDumpEntry(const char* prefix,
			      int callerRecIdx, CVMBool isFirst_ccIndex);
extern void
CVMJITPMIcalleeTableDumpIdx(CVMUint32 calleeRecIdx, CVMBool printCallerInfo);
extern void
CVMJITPMIcalleeTableDumpMB(CVMMethodBlock* calleeMb, CVMBool printCallerInfo);
extern void
CVMJITPMIcalleeTableDump(CVMBool printCallerInfo);
#endif

/************************
 * PMI Caller Table APIs
 ************************/

static CVMBool
CVMJITPMIcallerTableAllocateTable()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    jgs->pmiCallerRecords =
	malloc(jgs->pmiNumCallerRecords * sizeof(CVMJITPMICallerRecord));
    if (jgs->pmiCallerRecords == NULL) {
	return CVM_FALSE;
    }
    jgs->pmiFirstFreeCallerRecordIdx = -1; /* list is empty */
    return CVM_TRUE;
}

static void
CVMJITPMIcallerTableFreeTable()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    if (jgs->pmiCallerRecords != NULL) {
	free(jgs->pmiCallerRecords);
	jgs->pmiCallerRecords = NULL;
    }
}
static CVMUint32
CVMJITPMIcallerTableGetNextInList(CVMJITPMICallerRecord* callerRec)
{
    return (callerRec->nextIndex_upper8 << 8) + callerRec->nextIndex_lower8;
}

static void
CVMJITPMIcallerTableSetNextInList(CVMJITPMICallerRecord* callerRec,
				  CVMUint32 nextCallerRecIdx)
{
    callerRec->nextIndex_upper8 = nextCallerRecIdx >> 8;
    callerRec->nextIndex_lower8 = nextCallerRecIdx;
    CVMassert(nextCallerRecIdx == 
	      CVMJITPMIcallerTableGetNextInList(callerRec));
}

static void
CVMJITPMIcallerTableMarkEndOfList(CVMJITPMICallerRecord* callerRec)
{
    callerRec->nextIndex_upper8 = 0xff;
    callerRec->nextIndex_lower8 = 0xff;
}

static CVMBool
CVMJITPMIcallerTableIsEndOfList(CVMJITPMICallerRecord* callerRec)
{
    return (callerRec->nextIndex_upper8 == 0xff &&
	    callerRec->nextIndex_lower8 == 0xff);
}

static void
CVMJITPMIcallerTableDeleteRecord(CVMUint32 callerRecIdx)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJITPMICallerRecord* callerRec = &jgs->pmiCallerRecords[callerRecIdx];
    CVMassert(jgs->pmiEnabled);
    if (jgs->pmiFirstFreeCallerRecordIdx == -1) {
	/* Free record list is empty, so this new record will be last */
	CVMJITPMIcallerTableMarkEndOfList(callerRec);
    } else {
	/* Make this newly freed record point to the first free record. */
	CVMJITPMIcallerTableSetNextInList(callerRec,
					  jgs->pmiFirstFreeCallerRecordIdx);
    }
    /* Make this newly free record the first in the free list. */
    jgs->pmiFirstFreeCallerRecordIdx = callerRecIdx;
    jgs->pmiNumUsedCallerRecords--;
}

static CVMBool
CVMJITPMIcallerTableAllocateRecord(CVMUint32* callerRecIdxPtr)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMassert(jgs->pmiEnabled);

    if (jgs->pmiFirstFreeCallerRecordIdx != -1) {
	CVMJITPMICallerRecord* callerRec;
	/* found free caller record on the free list */
	*callerRecIdxPtr = jgs->pmiFirstFreeCallerRecordIdx;
	callerRec = &jgs->pmiCallerRecords[*callerRecIdxPtr];
	if (CVMJITPMIcallerTableIsEndOfList(callerRec)) {
	    jgs->pmiFirstFreeCallerRecordIdx = -1;
	} else {
	    jgs->pmiFirstFreeCallerRecordIdx =
		CVMJITPMIcallerTableGetNextInList(callerRec);
	}
    } else {
	/* allocate from the unallocated part of pmiCallerRecords */
	if (jgs->pmiNextAvailCallerRecordIdx == jgs->pmiNumCallerRecords) {
	    /* No room in pmiCallerRecords. Try to grow by 15%. */
	    CVMUint32 newNumCallerRecords;
	    CVMJITPMICallerRecord* newCallerRecords;
	    if (jgs->pmiNumCallerRecords == CVMJITPMI_MAX_CALLER_RECORDS) {
		/* we've already max'd out */
		return CVM_FALSE;
	    }
	    newNumCallerRecords = jgs->pmiNumCallerRecords * 1.15;
	    if (newNumCallerRecords > CVMJITPMI_MAX_CALLER_RECORDS) {
		newNumCallerRecords = CVMJITPMI_MAX_CALLER_RECORDS;
	    }
	    newCallerRecords = (CVMJITPMICallerRecord*)
		realloc(jgs->pmiCallerRecords,
			newNumCallerRecords * sizeof(CVMJITPMICallerRecord));
	    if (newCallerRecords == NULL) {
		return CVM_FALSE;
	    }
	    CVMtraceJITPatchedInvokes((
                "PMI RESIZE CALLER TABLE: oldSize(%d) newSize(%d)\n",
	        jgs->pmiNumCallerRecords, newNumCallerRecords));
	    jgs->pmiCallerRecords = newCallerRecords;
	    jgs->pmiNumCallerRecords = newNumCallerRecords;
	}
	/* allocate record */
	*callerRecIdxPtr = jgs->pmiNextAvailCallerRecordIdx;
	jgs->pmiNextAvailCallerRecordIdx++;
	CVMassert(jgs->pmiNextAvailCallerRecordIdx <=
		  jgs->pmiNumCallerRecords);
    }
    jgs->pmiNumUsedCallerRecords++;
    return CVM_TRUE;
}


#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)

void
CVMJITPMIcallerTableDumpEntry(const char* prefix,
			      int callerRecIdx, CVMBool isFirst_ccIndex)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint8* instrToPatch;
    CVMJITPMICallerRecord* callerRec = &jgs->pmiCallerRecords[callerRecIdx];

    CVMconsolePrintf("%s callerIdx(%d) nextIdx(%d) ", prefix, callerRecIdx,
		     CVMJITPMIcallerTableGetNextInList(callerRec));
    if (isFirst_ccIndex) {
	CVMconsolePrintf("virtual(%d) first_ccIndex(%d) ",
			 callerRec->first_isVirtual, callerRec->first_ccIndex);
	instrToPatch = jgs->codeCacheStart + callerRec->first_ccIndex;
    } else {
	CVMconsolePrintf("virtual(%d) second_ccIndex(%d)",
			 callerRec->second_isVirtual,
			 callerRec->second_ccIndex);
	instrToPatch = jgs->codeCacheStart + callerRec->second_ccIndex;
    }
    CVMconsolePrintf(" instrToPatch(0x%x) ", instrToPatch);
    /* This will print out the method name */
    CVMJITcodeCacheFindCompiledMethod(instrToPatch, CVM_TRUE);
}

#endif /* CVM_DEBUG || CVM_TRACE_JIT */

/************************
 * PMI Callee Table APIs
 ************************/

/*
 * Allocate a new callee hash table, using the size in jgs->pmiCalleeRecords.
 */
static CVMBool
CVMJITPMIcalleeTableAllocateTable()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    int searchCount = 0; /* number of secondary primes we compare against */
    if (jgs->pmiNumCalleeRecords < CVMJITPMI_MIN_CALLEE_RECORDS) {
	jgs->pmiNumCalleeRecords = CVMJITPMI_MIN_CALLEE_RECORDS;
    }
    jgs->pmiNumCalleeRecords |= 1; /* make it odd */

    /*
     * Loop until we find a size for the callee table that is relatively
     * prime with all the primes used for the secondary hash.
     */
    while (CVM_TRUE) {
	int i = 0;
	CVMBool relativelyPrime = CVM_TRUE;
	for (i = 0; i < CVMJITPMI_NUM_SECONDARY_HASH_PRIMES; i++) {
	    searchCount++;
	    if (jgs->pmiNumCalleeRecords % CVMJITPMIsecondaryHashPrimes[i]
		== 0)
	    {
		/* secondary prime is a factor of jgs->pmiNumCalleeRecords */
		relativelyPrime = CVM_FALSE;
		break;
	    }
	}
	if (relativelyPrime) {
	    break; /* we found it */
	}
	jgs->pmiNumCalleeRecords += 2; /* try next candidate */
    }
    CVMassert(searchCount < 200); /* we should never need this many tries */

    if (jgs->pmiNumCalleeRecords > 65535) {
	/* A relative prime that is less than 65535 */
	jgs->pmiNumCalleeRecords = 101 * 631;
    }

    /* Allocate callee table */
    jgs->pmiCalleeRecords =
	calloc(jgs->pmiNumCalleeRecords * sizeof(CVMJITPMICalleeRecord), 1);
    if (jgs->pmiCalleeRecords == NULL) {
	return CVM_FALSE;
    }

    /* Don't allow callee table to get too full */
    jgs->pmiMaxUsedCalleeRecordsAllowed = jgs->pmiNumCalleeRecords *
	CVMJITPMI_CALLEETABLE_MAX_PERCENT_FULL / 100;

#if 0 /* some debug tracing that we normally want off */
    CVMconsolePrintf("CVMJITPMIcalleeTableAllocate: searchCount(%d) "
		     "size(%d) callerSize(%d/%d==%d%%)\n",
		     searchCount, jgs->pmiNumCalleeRecords,
		     jgs->pmiNumUsedCallerRecords, jgs->pmiNumCallerRecords,
		     (jgs->pmiNumUsedCallerRecords * 100 /
		      jgs->pmiNumCallerRecords));
#endif
    return CVM_TRUE;
}

static void
CVMJITPMIcalleeTableFreeTable() {
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    if (jgs->pmiCalleeRecords != NULL) {
	free(jgs->pmiCalleeRecords);
	jgs->pmiCalleeRecords = NULL;
    }
}

static CVMBool
CVMJITPMIcalleeTableIsEmptyRecord(CVMJITPMICalleeRecord* calleeRec)
{
    return calleeRec->calleeMb == NULL;
}

static CVMBool
CVMJITPMIcalleeTableIsDeletedRecord(CVMJITPMICalleeRecord* calleeRec)
{
    return calleeRec->calleeMb == (CVMMethodBlock*)1;
}

static void
CVMJITPMIcalleeTableDeleteRecord(CVMJITPMICalleeRecord* calleeRec)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMassert(jgs->pmiEnabled);
    jgs->pmiNumUsedCalleeRecords--;
    jgs->pmiNumDeletedCalleeRecords++;
    calleeRec->calleeMb = (CVMMethodBlock*)1;
}

/*
 * Finds the index in jgs->pmiCalleeRecords of the callee record
 * that contains calleeMb. Returns CVM_TRUE if found, with the
 * index in *indexPtr. Returns CVM_FALSE if not found, with the
 * index to use for adding a new record in *indexPtr.
 */
static CVMBool
CVMJITPMIcalleeTableFindRecord(CVMMethodBlock* calleeMb, CVMUint32* indexPtr)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJITPMICalleeRecord* calleeRec;
    CVMBool result = CVM_FALSE; /* assume not found */
    int chainLen = 0; /* used to track how long a search takes */
    CVMUint32 hashIdx =
	(CVMmbNameAndTypeID(calleeMb) + (int)calleeMb) %
	jgs->pmiNumCalleeRecords;
    CVMassert(jgs->pmiEnabled);

    /* Make sure we own the jitLock */
    CVMassert(CVMsysMutexIAmOwner(CVMgetEE(), &CVMglobals.jitLock));

    /* See if the primary hash slot is the record we are looking for */
    calleeRec = &jgs->pmiCalleeRecords[hashIdx];
    if (calleeRec->calleeMb == calleeMb) {
	result = CVM_TRUE; /* calleeMb found */
	goto done;
    }

    /* If the primary hash slot is empty, then the record is not found */
    if (CVMJITPMIcalleeTableIsEmptyRecord(calleeRec)) {
	goto done; /* calleeRec not found */
    }
    
    /*
     * Search the secondary hash slots until we find the calleeMb or find
     * an empty record. Keep track of the first deleted slot we find
     * so the caller can add a new record there if no match is found.
     */
    {
	int firstDeletedHashIdx;
	int secondaryHashPrimesIdx = 
	    (CVMmbNameAndTypeID(calleeMb) + (int)calleeMb) %
	    CVMJITPMI_NUM_SECONDARY_HASH_PRIMES;
	int secondaryHashPrime =
	    CVMJITPMIsecondaryHashPrimes[secondaryHashPrimesIdx];

	if (CVMJITPMIcalleeTableIsDeletedRecord(calleeRec)) {
	    firstDeletedHashIdx = hashIdx;
	} else {
	    firstDeletedHashIdx = -1;
	}

	do {
	    chainLen++;
#ifdef CVM_DEBUG_ASSERTS
	    if (chainLen > jgs->pmiNumCalleeRecords) {
		CVMdebugPrintf(("chainLen(%d) > pmiNumCalleeRecords(%d)\n",
				chainLen, jgs->pmiNumCalleeRecords));
		CVMdebugPrintf(("hashIdx(%d) secondaryHashPrime(%d)\n",
				hashIdx, secondaryHashPrime));
		CVMassert(CVM_FALSE);
	    }
#endif
	    hashIdx =
		(hashIdx + secondaryHashPrime) % jgs->pmiNumCalleeRecords;

	    /* See if this hash slot has the record we are looking for */
	    calleeRec = &jgs->pmiCalleeRecords[hashIdx];
	    if (calleeRec->calleeMb == calleeMb) {
		result = CVM_TRUE;
		goto done; /* calleeRec found */
	    }

	    /* If the hash slot is empty, then the record is not found */
	    if (CVMJITPMIcalleeTableIsEmptyRecord(calleeRec)) {
		if (firstDeletedHashIdx != -1) {
		    hashIdx = firstDeletedHashIdx;
		}
		goto done; /* calleeRec not found */
	    }

	    if (firstDeletedHashIdx == -1 &&
		CVMJITPMIcalleeTableIsDeletedRecord(calleeRec))
	    {
		firstDeletedHashIdx = hashIdx;
	    }
	} while(CVM_TRUE);
    }
    CVMassert(CVM_FALSE);

 done:
    CVMtraceJITPatchedInvokes((
	"PMI FIND: chainLen(%d) %s(%d) %C.%M\n", chainLen,
	result ? "Found idx" : "Not Found newIdx", hashIdx,
	CVMmbClassBlock(calleeMb), calleeMb));
    *indexPtr = hashIdx; /* index of found or available slot */
    return result; 
}

/*
 * Resize the callee hash table. This is done when too large of a % of
 * the entries are either allocated or deleted (too few are truly
 * empty).
 */
static CVMBool
CVMJITPMIcalleeTableResize()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint32 oldCalleeRecIdx;
    CVMUint32 oldNumCalleeRecords = jgs->pmiNumCalleeRecords;
    CVMJITPMICalleeRecord* oldCalleeRecords = jgs->pmiCalleeRecords;
    CVMBool success;

    /* Make new size big enough so the table can add 20% more entries before
     * reaching the "max % full" threshold */
    jgs->pmiNumCalleeRecords = jgs->pmiNumUsedCalleeRecords * 
	100 / CVMJITPMI_CALLEETABLE_MAX_PERCENT_FULL * 1.20;

    success = CVMJITPMIcalleeTableAllocateTable();

    CVMtraceJITPatchedInvokes(("PMI RESIZE CALLEE TABLE: "
			       "oldSize(%d) newSize(%d)\n",
			       oldNumCalleeRecords, jgs->pmiNumCalleeRecords));

    /* Allocate callee hash table */
    if (!success) {
	jgs->pmiNumCalleeRecords = oldNumCalleeRecords;
	jgs->pmiCalleeRecords = oldCalleeRecords;
	CVMtraceJITPatchedInvokes(("PMI RESIZE CALLEE TABLE: FAILED\n"));
	return CVM_FALSE;
    }

    jgs->pmiNumDeletedCalleeRecords = 0;

    /* iteratate over the entire old callee hash table and add each
     * entry to the new hash table. */
    for (oldCalleeRecIdx = 0;
	 oldCalleeRecIdx < oldNumCalleeRecords;
	 oldCalleeRecIdx++)
    {
	CVMUint32 calleeRecIdx;
	CVMBool calleeRecordExists;
	CVMJITPMICalleeRecord* calleeRec;
	CVMJITPMICalleeRecord* oldCalleeRec =
	    &oldCalleeRecords[oldCalleeRecIdx];

	if (CVMJITPMIcalleeTableIsDeletedRecord(oldCalleeRec) ||
	    CVMJITPMIcalleeTableIsEmptyRecord(oldCalleeRec))
	{
	    continue;
	}

	calleeRecordExists =
	    CVMJITPMIcalleeTableFindRecord(oldCalleeRec->calleeMb,
					   &calleeRecIdx);
	CVMassert(!calleeRecordExists);
	calleeRec = &jgs->pmiCalleeRecords[calleeRecIdx];
	/* copy the callee record */
	*calleeRec = *oldCalleeRec;
    }
    
    /* Free the old table */
    free(oldCalleeRecords);

    CVMtraceJITPatchedInvokesExec({
        CVMJITPMIcalleeTableDump(CVM_FALSE);
    });
    CVMtraceJITPatchedInvokes(("PMI RESIZE CALLEE TABLE: DONE\n"));
    return CVM_TRUE;
}

/*
 * Adds calleeMb to the callee table, using the table entry indicated
 * by *calleeRecIdxPtr. If this makes the table too full, it will be
 * grown first and *calleeRecIdxPtr will be updated to reference a new
 * slot in the expanded table. Returns CVM_TRUE if added
 * successfully. Returns CVM_FALSE if unsucessful, which can happen if
 * the hash table cannot be grown.
 */
static CVMBool
CVMJITPMIcalleeTableAddRecord(CVMMethodBlock* calleeMb,
			      CVMUint32* calleeRecIdxPtr)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJITPMICalleeRecord* calleeRec =
	&jgs->pmiCalleeRecords[*calleeRecIdxPtr];
    CVMassert(jgs->pmiEnabled);

    if (CVMJITPMIcalleeTableIsDeletedRecord(calleeRec)) {
	/* If it is a previously deleted record, then just use it without
	   checking if we exceeded the hash table usage threshold (which
	   we can't possibly be exceeding. */
	jgs->pmiNumDeletedCalleeRecords--;
	goto done;
    }

    /* Make sure there is room in callee hash table for a new entry */
    if(jgs->pmiNumUsedCalleeRecords + jgs->pmiNumDeletedCalleeRecords ==
       jgs->pmiMaxUsedCalleeRecordsAllowed)
    {
	/* Table too full. Try to grow it first. */
	CVMassert(!CVMJITPMIcalleeTableIsDeletedRecord(calleeRec));
	if (!CVMJITPMIcalleeTableResize()) {
	    CVMtraceJITPatchedInvokes((
                "PMI ADD: Failed. Callee Table Full(%d/%d) %C.%M\n",
		jgs->pmiNumUsedCalleeRecords, jgs->pmiNumCallerRecords,
		CVMmbClassBlock(calleeMb), calleeMb));
	    return CVM_FALSE; /* no more room and we can't grow */
	}
	/* find a new empty slot in the expanded hash table */
	CVMJITPMIcalleeTableFindRecord(calleeMb, calleeRecIdxPtr);
	calleeRec = &jgs->pmiCalleeRecords[*calleeRecIdxPtr];
    }

 done:
    jgs->pmiNumUsedCalleeRecords++;
    calleeRec->calleeMb = calleeMb;    
    return CVM_TRUE;
}

#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)

void
CVMJITPMIcalleeTableDumpIdx(CVMUint32 calleeRecIdx,
			    CVMBool printCallerInfo)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJITPMICalleeRecord* calleeRec = &jgs->pmiCalleeRecords[calleeRecIdx];

    /* Print callee record info */
    CVMconsolePrintf("%3d ", calleeRecIdx);
    if (CVMJITPMIcalleeTableIsEmptyRecord(calleeRec)) {
	CVMconsolePrintf("<empty>\n");
    } else if (CVMJITPMIcalleeTableIsDeletedRecord(calleeRec)) {
	CVMconsolePrintf("<deleted>\n");
    } else {
	CVMMethodBlock* calleeMb = calleeRec->calleeMb;
	CVMconsolePrintf("firstCaller(%d) lastCaller(%d) "
			 "calleeMb(0x%x) %C.%M\n",
			 calleeRec->firstCallerRecordIdx,
			 calleeRec->lastCallerRecordIdx,
			 calleeMb, CVMmbClassBlock(calleeMb), calleeMb);
	if (printCallerInfo) {
	    /* Print each caller record */
	    CVMUint32 callerRecIdx = calleeRec->firstCallerRecordIdx;
	    CVMJITPMICallerRecord* callerRec;
	    do {
		callerRec = &jgs->pmiCallerRecords[callerRecIdx];
		/* print first caller in record */
		CVMJITPMIcallerTableDumpEntry(" -->", callerRecIdx, CVM_TRUE);
		if (callerRec->second_ccIndex != 0) {
		    /* print second caller in record */
		    CVMJITPMIcallerTableDumpEntry(" -->",
						  callerRecIdx, CVM_FALSE);
		}
		callerRecIdx = CVMJITPMIcallerTableGetNextInList(callerRec);
	    } while (!CVMJITPMIcallerTableIsEndOfList(callerRec));
	}
    }
}

void
CVMJITPMIcalleeTableDumpMB(CVMMethodBlock* calleeMb, CVMBool printCallerInfo)
{
    CVMUint32 calleeRecIdx;
    if (!CVMJITPMIcalleeTableFindRecord(calleeMb, &calleeRecIdx)) {
	CVMconsolePrintf(" <not found>. 0x%x %C.%M\n",
			 calleeMb, CVMmbClassBlock(calleeMb), calleeMb);
	return;
    }
    CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, printCallerInfo);
}

void
CVMJITPMIcalleeTableDump(CVMBool printCallerInfo)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint32 calleeRecIdx;
    CVMUint32 numEmpty = 0;
    CVMUint32 numDeleted = 0;
    CVMUint32 numUsed = 0;

    CVMconsolePrintf("========== CALLEE TABLE DUMP ==========\n");

    /* iteratate over the entire callee hash table */
    for (calleeRecIdx = 0;
	 calleeRecIdx < jgs->pmiNumCalleeRecords;
	 calleeRecIdx++)
    {
	CVMJITPMICalleeRecord* calleeRec =
	    &jgs->pmiCalleeRecords[calleeRecIdx];
	if (calleeRec->calleeMb == NULL) {
	    numEmpty++;
	} else if ((int)calleeRec->calleeMb == 1) {
	    numDeleted++;
	} else {
	    numUsed++;
	}
	/* Dump info for this callee table entry */
	CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, printCallerInfo);
    }

    CVMconsolePrintf(
	"empty(%d==%d%%) deleted(%d==%d%%) "
	"callees(%d/%d==%d%%) callers(%d/%d==%d%%)\n",
	numEmpty, numEmpty * 100 / jgs->pmiNumCalleeRecords,
	numDeleted, numDeleted * 100 / jgs->pmiNumCalleeRecords,
	numUsed, jgs->pmiNumCalleeRecords, 
	numUsed * 100 / jgs->pmiNumCalleeRecords,
	jgs->pmiNumUsedCallerRecords, jgs->pmiNumCallerRecords,
	jgs->pmiNumUsedCallerRecords * 100 / jgs->pmiNumCallerRecords);

    CVMassert(numUsed == jgs->pmiNumUsedCalleeRecords);
    CVMassert(numDeleted == jgs->pmiNumDeletedCalleeRecords);
    CVMconsolePrintf("========== END CALLEE TABLE DUMP ==========\n");
}

#endif /* CVM_DEBUG || CVM_TRACE_JIT */

/****************************
 * PMI Public APIs
 ****************************/

static CVMBool
CVMJITPMIinit(CVMJITGlobalState* jgs)
{
    /* Set size of callee and caller tables. The sizes of two are related, with
     * the ratio determined by CVMJITPMI_NUM_CALLER_RECORDS_PER_CALLEE_RECORD
     */
    jgs->pmiNumCallerRecords = (jgs->codeCacheSize / CVMCPU_INSTRUCTION_SIZE) /
	CVMJITPMI_NUM_INSTRUCTIONS_PER_PATCH / 2 /* 2 callers per record */;
    if (jgs->pmiNumCallerRecords > CVMJITPMI_MAX_CALLER_RECORDS) {
	jgs->pmiNumCallerRecords = CVMJITPMI_MAX_CALLER_RECORDS;
    } else if (jgs->pmiNumCallerRecords < CVMJITPMI_MIN_CALLER_RECORDS) {
	jgs->pmiNumCallerRecords = CVMJITPMI_MIN_CALLER_RECORDS;
    }
    jgs->pmiNumCalleeRecords = jgs->pmiNumCallerRecords /
	CVMJITPMI_NUM_CALLER_RECORDS_PER_CALLEE_RECORD;

    /* Allocate callee hash table */
    if (!CVMJITPMIcalleeTableAllocateTable()) {
	return CVM_FALSE;
    }

    /* Allocate caller table */
    if (!CVMJITPMIcallerTableAllocateTable()) {
	CVMJITPMIcalleeTableFreeTable();
	return CVM_FALSE;
    }

    /* init the backend */
    if (!CVMJITPMIinitBackEnd()) {
        return CVM_FALSE;
    }

    return CVM_TRUE;
}

static void
CVMJITPMIdestroy(CVMJITGlobalState* jgs)
{
    CVMJITPMIcalleeTableFreeTable();
    CVMJITPMIcallerTableFreeTable();
}

/*
 * Add a patch record. callerMb is making a direct call to calleeMb at
 * address instrAddr. If isVirtual is true, then this is a virtual
 * invoke of a method that has not been overridden. Returns CVM_FALSE
 * if this fails for any reason, in which case a direct method call
 * should not be used.
 */
extern CVMBool
CVMJITPMIaddPatchRecord(CVMMethodBlock* callerMb,
			CVMMethodBlock* calleeMb,
			CVMUint8* instrAddr,
			CVMBool isVirtual)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMBool calleeRecordExists;
    CVMJITPMICalleeRecord* calleeRec;
    CVMJITPMICallerRecord* callerRec;
    CVMUint32 calleeRecIdx;
    CVMUint32 callerRecIdx;

    /* Make sure we own the jitLock */
    CVMassert(CVMsysMutexIAmOwner(CVMgetEE(), &CVMglobals.jitLock));
    CVMassert(jgs->pmiEnabled);
    
    CVMtraceJITPatchedInvokes((
        "PMI ADD: callerMb=0x%x instrAddr=0x%x %C.%M\n",
	callerMb, instrAddr, CVMmbClassBlock(callerMb), callerMb));
    CVMtraceJITPatchedInvokes((
        "PMI ADD: calleeMb=0x%x %C.%M\n",
	calleeMb, CVMmbClassBlock(calleeMb), calleeMb));

    /* See if there is already a callee record. If not, get an index
     * for a new record. */
    calleeRecordExists = 
	CVMJITPMIcalleeTableFindRecord(calleeMb, &calleeRecIdx);

    if (!calleeRecordExists) {
	/* Add the new record. If the table is too full, it will be grown.
	 * If the table can't grow, then it will fail. */
	if (!CVMJITPMIcalleeTableAddRecord(calleeMb, &calleeRecIdx)) {
	    return CVM_FALSE;
	}
    }
    calleeRec = &jgs->pmiCalleeRecords[calleeRecIdx];
    
    /* 
     * If the callee record already exists and second_ccIndex of the last
     * caller record is not yet in use, then just use it and we are done.
     */
    if (calleeRecordExists) {
	callerRecIdx = calleeRec->lastCallerRecordIdx;
	callerRec = &jgs->pmiCallerRecords[callerRecIdx];
	if (callerRec->second_ccIndex == 0) {
	    callerRec->second_ccIndex = instrAddr - jgs->codeCacheStart;
	    callerRec->second_isVirtual = isVirtual;
	    CVMtraceJITPatchedInvokesExec({
	        CVMJITPMIcallerTableDumpEntry("PMI ADD CALLER:",
					      callerRecIdx, CVM_FALSE);
	    });
	    goto success;
	}
    }

    /*
     * Find a free caller record.
     */
    if (!CVMJITPMIcallerTableAllocateRecord(&callerRecIdx)) {
	/* Failed to allocate caller record. */
	CVMtraceJITPatchedInvokes((
            "PMI ADD: Failed. Caller Table Full. %C.%M\n",
	    CVMmbClassBlock(calleeMb), calleeMb));
	/* Delete callee record if it is a newly allocated one and then fail */
	if (!calleeRecordExists) {
	    CVMJITPMIcalleeTableDeleteRecord(calleeRec);
	}
	return CVM_FALSE;
    }
    callerRec = &jgs->pmiCallerRecords[callerRecIdx];

    /* Update existing callee record or initialize new callee record */
    if (calleeRecordExists) {
	/* we need to link the last caller record to the newly allocated one */
	CVMJITPMIcallerTableSetNextInList(
            &jgs->pmiCallerRecords[calleeRec->lastCallerRecordIdx],
	    callerRecIdx);
    } else {
	/* this is the first caller record for this callee */
	calleeRec->firstCallerRecordIdx = callerRecIdx;
    }
    calleeRec->lastCallerRecordIdx = callerRecIdx;

    /* initialized the caller record */
    callerRec->first_ccIndex = instrAddr - jgs->codeCacheStart;
    callerRec->first_isVirtual = isVirtual;
    callerRec->second_ccIndex = 0;
    CVMJITPMIcallerTableMarkEndOfList(callerRec);

    CVMtraceJITPatchedInvokesExec({
	CVMJITPMIcallerTableDumpEntry("PMI ADD CALLER:",
				      callerRecIdx, CVM_TRUE);
    });

 success:

    CVMtraceJITPatchedInvokesExec({
	CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, CVM_TRUE);
	/*CVMJITPMIcalleeTableDump(CVM_FALSE);*/
    });
    CVMtraceJITPatchedInvokes(("PMI ADD: DONE\n"));

    return CVM_TRUE;
}

extern void 
CVMJITPMIremovePatchRecords(CVMMethodBlock* callerMb, CVMMethodBlock* calleeMb,
			    CVMUint8* callerStartPC, CVMUint8* callerEndPC)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMBool calleeRecordExists;
    CVMJITPMICalleeRecord* calleeRec;
    CVMJITPMICallerRecord* callerRec;
    CVMJITPMICallerRecord* prevCallerRec = NULL;
    CVMJITPMICallerRecord* lastCallerRec;
    CVMUint32 calleeRecIdx;
    CVMUint32 callerRecIdx;
    CVMUint32 prevCallerRecIdx = 0;    

    /* Make sure we own the jitLock */
    CVMassert(CVMsysMutexIAmOwner(CVMgetEE(), &CVMglobals.jitLock));
    CVMassert(jgs->pmiEnabled);
    
    CVMtraceJITPatchedInvokes(("PMI REMOVE: calleeMb=0x%x %C.%M\n",
			       calleeMb, CVMmbClassBlock(calleeMb), calleeMb));
    CVMtraceJITPatchedInvokes(("PMI REMOVE: callerMb=0x%x %C.%M\n",
			       callerMb, CVMmbClassBlock(callerMb), callerMb));

    /* find the callee record for calleeMb */
    calleeRecordExists =
	CVMJITPMIcalleeTableFindRecord(calleeMb, &calleeRecIdx);
    CVMassert(calleeRecordExists);
    calleeRec = &jgs->pmiCalleeRecords[calleeRecIdx];
    lastCallerRec = &jgs->pmiCallerRecords[calleeRec->lastCallerRecordIdx];

    CVMtraceJITPatchedInvokesExec({
	CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, CVM_TRUE);
    });

    /* 
     * Iterate over all the callers referenced by the callee record.
     * If the caller is in the caller method, then remove it.
     */
    callerRecIdx = calleeRec->firstCallerRecordIdx;

    do {
	CVMUint8* instrToPatch;
	CVMBool removeFirst, removeSecond;
	CVMUint32 nextCallerRecIdx;

	callerRec = &jgs->pmiCallerRecords[callerRecIdx];
	nextCallerRecIdx = CVMJITPMIcallerTableGetNextInList(callerRec);

	/* check first call in caller record */
	instrToPatch = jgs->codeCacheStart + callerRec->first_ccIndex;
	if (instrToPatch >= callerStartPC && instrToPatch <= callerEndPC) {
	    removeFirst = CVM_TRUE;
	} else {
	    removeFirst = CVM_FALSE;
	}
	/* check second call in caller record */
	if (callerRec->second_ccIndex == 0) {
	    removeSecond = CVM_FALSE; /* already empty */
	    CVMassert(callerRecIdx == calleeRec->lastCallerRecordIdx);
	} else {
	    instrToPatch = jgs->codeCacheStart + callerRec->second_ccIndex;
	    if (instrToPatch >= callerStartPC && instrToPatch <= callerEndPC) {
		removeSecond = CVM_TRUE;
	    } else {
		removeSecond = CVM_FALSE;
	    }
	}

	/* If no callers to remove, then go on to next record */
	if (!removeFirst && !removeSecond) {
	    prevCallerRec = callerRec;
	    prevCallerRecIdx = callerRecIdx;
	    goto next_iter;
	}

	/*
	 * If required, deal with having just one of the two callers
	 * remaining after a removal.
	 */
	if (removeFirst != removeSecond && callerRec->second_ccIndex != 0) {
	    /* one caller remains in this caller record */
	    if (callerRecIdx == calleeRec->lastCallerRecordIdx) {
		/* It's the last record, so just leave whichever entry we are
		   keeping in the 1st caller and zero out the 2nd caller. */
		if (removeFirst) {
		    callerRec->first_ccIndex = callerRec->second_ccIndex;
		    callerRec->first_isVirtual = callerRec->second_isVirtual;
		}
		callerRec->second_ccIndex = 0;
		CVMtraceJITPatchedInvokes((
		    "PMI REMOVE: removed %s from lastCallerRecord(%d)\n",
		    removeFirst ? "first_ccIndex" : "second_ccIndex",
		    callerRecIdx));
		break; /* we are done */
	    }

	    /* If second_ccIndex in the lastCallerRec is open, then move the
	       caller we are keeping into it and then remove this record. */
	    if (lastCallerRec->second_ccIndex == 0) {
		if (removeFirst) {
		    lastCallerRec->second_ccIndex = callerRec->second_ccIndex;
		    lastCallerRec->second_isVirtual =
			callerRec->second_isVirtual;
		} else {
		    lastCallerRec->second_ccIndex = callerRec->first_ccIndex;
		    lastCallerRec->second_isVirtual =
			callerRec->first_isVirtual;
		}
		CVMtraceJITPatchedInvokes((
		    "PMI REMOVE: moved %s(%d) to lastCallerRec(%d)\n",
		    removeFirst ? "first_ccIndex" : "second_ccIndex",
		    callerRecIdx, calleeRec->lastCallerRecordIdx));
		goto delete_record;
	    }

	    /* If we get here, we know we need to remove just one of the
	       two callers, and that we can't move it into second_ccIndex
	       in the last caller. We also know this is not the last record.
	       So, make this record the last caller record, and also make
	       sure the caller we keep is put in first_ccIndex. (NOTE: only
	       the last record is allowed to have second_ccIndex == 0).
	    */
	    if (removeFirst) {
		callerRec->first_ccIndex = callerRec->second_ccIndex;
		callerRec->first_isVirtual = callerRec->second_isVirtual;
	    }
	    callerRec->second_ccIndex = 0;
	    calleeRec->lastCallerRecordIdx = callerRecIdx;
	    if (callerRecIdx == calleeRec->firstCallerRecordIdx) {
		/* callerRec moved from the front of the list to the end,
		   so update calleeRec->firstCallerRecordIdx. */
		calleeRec->firstCallerRecordIdx = nextCallerRecIdx;
	    } else {
		/* calleeRec moved from the middle of the list to the end,
		   so update the next pointer of the previous record. */
		CVMJITPMIcallerTableSetNextInList(prevCallerRec,
						  nextCallerRecIdx);
	    }
	    /* calleeRec is now at the end of the list. */
	    CVMJITPMIcallerTableMarkEndOfList(callerRec);
	    /* The old end of the list now points to calleeRec. */
	    CVMJITPMIcallerTableSetNextInList(lastCallerRec, callerRecIdx);
	    lastCallerRec = callerRec; /* keep lastCallerRec up to date */
	    callerRec = prevCallerRec; /* needed to keep loop going */
	    CVMtraceJITPatchedInvokes((
	        "PMI REMOVE: removed %s(%d) and moved to last\n",
		    removeFirst ? "first" : "second", callerRecIdx));
	    goto next_iter;
	}
		
    delete_record:
	/*
	 * If we get here, the whole record is going away, either
	 * because both callers were removed, the first caller was removed
	 * and there is no second caller, or the one that was not
	 * removed was moved to the last caller record.
	 */

	/* we are done with the caller record, so delete it */
	CVMJITPMIcallerTableDeleteRecord(callerRecIdx);
	CVMtraceJITPatchedInvokes((
	    "PMI REMOVE: deleted callerRec(%d)\n", callerRecIdx));
	if (callerRecIdx == calleeRec->firstCallerRecordIdx) {
	    if (callerRecIdx == calleeRec->lastCallerRecordIdx) {
		/* This is the last remaining caller record for calleeMb,
		   so remove calleeMb from the callee table. */
		CVMJITPMIcalleeTableDeleteRecord(calleeRec);
		CVMtraceJITPatchedInvokes((
	            "PMI REMOVE: deleted calleeRec(%d)\n", calleeRecIdx));
		CVMtraceJITPatchedInvokesExec({
			/*CVMJITPMIcalleeTableDump(CVM_FALSE);*/
		});
		break; /* we are done */
	    }
	    /* First callerRec removed, so update firstCallerRecordIdx */
	    calleeRec->firstCallerRecordIdx = nextCallerRecIdx;
	} else {
	    /* calleeRec removed from the middle of the list, so update the
	       next pointer of the previous record. */
	    if (callerRecIdx == calleeRec->lastCallerRecordIdx) {
		CVMJITPMIcallerTableMarkEndOfList(prevCallerRec);
		calleeRec->lastCallerRecordIdx = prevCallerRecIdx;
	    } else {
		CVMJITPMIcallerTableSetNextInList(prevCallerRec,
						  nextCallerRecIdx);
	    }
	}
	callerRec = prevCallerRec; /* needed to keep loop going */

    next_iter:
	/* Check out the next caller record. */
	callerRecIdx = nextCallerRecIdx;
    } while (callerRec == NULL || !CVMJITPMIcallerTableIsEndOfList(callerRec));

    CVMtraceJITPatchedInvokesExec({
	CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, CVM_TRUE);
    });
}

static void
CVMJITPMIpatchCall(CVMMethodBlock* calleeMb,
		   CVMUint8* instrAddr,
		   CVMJITPMI_PATCHSTATE newPatchState,
		   CVMBool isVirtual)
{
    int branchTargetOffset;
    CVMassert(CVMglobals.jit.pmiEnabled);
    
    /*
      These are the possible states a patch point can be in, and the
      transitions that can be made while in each state.

              COMPILED <-------------------------> DECOMPILED
         (direct method call)              (CVMCCMletInterpreterDoInvoke)
                 |                                     |
                 |                                     |
                 +----------> OVERRIDDEN <-------------+
                         (CVMCCMinvokeVirtual)
    */

    /*
     * It's possible to have a non-virtual method call to a virtual method,
     * such as when super.<method>() is used to invoke a method. In this
     * case we don't want to apply the patch that will force an invoke
     * virtual to be done. We just keep the call site as is.
     */
    if (!isVirtual && newPatchState == CVMJITPMI_PATCHSTATE_OVERRIDDEN) {
	CVMtraceJITPatchedInvokes((
	    "PMI PATCH: not patched. Method call is not virtual."));
	return;
    }

    /*
     * If this is a patch for a virtual method call and the target
     * method has been overridden, then don't apply the patch unless
     * it is for handling the override. This is done to make sure we
     * don't ever move a patch from the OVERRIDDEN state to the
     * COMPILED or DECOMPILED states, which can result in the wrong
     * method being called.
     */
    if (isVirtual &&
	newPatchState != CVMJITPMI_PATCHSTATE_OVERRIDDEN &&
	(CVMmbCompileFlags(calleeMb) & CVMJIT_IS_OVERRIDDEN) != 0)
    {
	CVMtraceJITPatchedInvokes((
	    "PMI PATCH: not patched. Method OVERRIDDEN."));
	return;
    }

    switch (newPatchState) {
        case CVMJITPMI_PATCHSTATE_COMPILED:
	    branchTargetOffset = CVMmbStartPC(calleeMb) - instrAddr;
	    break;
        case CVMJITPMI_PATCHSTATE_DECOMPILED:
	    branchTargetOffset =
		CVMJIT_CCMCODE_ADDR(CVMCCMletInterpreterDoInvoke) - instrAddr;
	    break;
        case CVMJITPMI_PATCHSTATE_OVERRIDDEN:
	    branchTargetOffset =
		CVMJIT_CCMCODE_ADDR(CVMCCMinvokeVirtual) - instrAddr;
	    break;
        default:
	    branchTargetOffset = 0;
	    CVMassert(CVM_FALSE);
    }

    /* Modify the branch bits to call new target. */
    CVMCPUpatchBranchInstruction(branchTargetOffset, instrAddr);
    
    /* Flush the cache after patching. */
    CVMJITflushCache(instrAddr, instrAddr + CVMCPU_INSTRUCTION_SIZE);
}

extern void
CVMJITPMIpatchCallsToMethod(CVMMethodBlock* calleeMb,
			    CVMJITPMI_PATCHSTATE newPatchState)
{
    CVMJITGlobalState*     jgs = &CVMglobals.jit;
    CVMBool                calleeRecordExists;
    CVMUint32              calleeRecIdx;
    CVMJITPMICalleeRecord* calleeRec;
    CVMUint32              callerRecIdx;
    CVMJITPMICallerRecord* callerRec;

    if (!jgs->pmiEnabled) {
        return;
    }

    /* Make sure we own the jitLock */
    CVMassert(CVMsysMutexIAmOwner(CVMgetEE(), &CVMglobals.jitLock));
    /* get the callee record */
    calleeRecordExists =
	CVMJITPMIcalleeTableFindRecord(calleeMb, &calleeRecIdx);
    
    CVMtraceJITPatchedInvokesExec({
	switch (newPatchState) {
	    case CVMJITPMI_PATCHSTATE_COMPILED:
		CVMconsolePrintf("PMI PATCH: newstate(COMPILED) ");
		break;
	    case CVMJITPMI_PATCHSTATE_DECOMPILED:
		CVMconsolePrintf("PMI PATCH: newstate(DECOMPILED) ");
		break;
	    case CVMJITPMI_PATCHSTATE_OVERRIDDEN:
		CVMconsolePrintf("PMI PATCH: newstate(OVERRIDDEN) ");
		break;
	}
	if (calleeRecordExists) {
	    CVMJITPMIcalleeTableDumpIdx(calleeRecIdx, CVM_TRUE);
	} else {
	    CVMconsolePrintf("<not found> calleeMb=0x%x %C.%M\n",
			     calleeMb, CVMmbClassBlock(calleeMb), calleeMb);
	}
    });

    if (!calleeRecordExists) {
	return; /* no one calls this method so no patching needed */
    }
    calleeRec = &jgs->pmiCalleeRecords[calleeRecIdx];

    /* 
     * Iterate over all the callers referenced by the callee record
     * and patch each one.
     */
    callerRecIdx = calleeRec->firstCallerRecordIdx;
    do {
	CVMUint8* instrToPatch;
	callerRec = &jgs->pmiCallerRecords[callerRecIdx];
	/* patch first call in caller record */
	instrToPatch = jgs->codeCacheStart + callerRec->first_ccIndex;
	CVMJITPMIpatchCall(calleeMb, instrToPatch, newPatchState,
			   callerRec->first_isVirtual);
	if (callerRec->second_ccIndex != 0) {
	    /* patch second call in caller record */
	    instrToPatch = jgs->codeCacheStart + callerRec->second_ccIndex;
	    CVMJITPMIpatchCall(calleeMb, instrToPatch, newPatchState,
			       callerRec->second_isVirtual);
	}
	callerRecIdx = CVMJITPMIcallerTableGetNextInList(callerRec);
    } while (!CVMJITPMIcallerTableIsEndOfList(callerRec));
}

#if CVM_DEBUG || CVM_TRACE_JIT
void
CVMJITPMIdumpMethodCalleeInfo(CVMMethodBlock* callerMb,
			      CVMBool printCallerInfo)
{
    CVMMethodBlock** callees = CVMcmdCallees(CVMmbCmd(callerMb));
    if (callees == NULL) {
	CVMconsolePrintf("PMI CALLEES: No callees. callerMb(0x%x) %C.%M\n",
			 callerMb, CVMmbClassBlock(callerMb), callerMb);
    } else {
	CVMUint32 numCallees = (CVMUint32)callees[0];
	CVMUint32 i;
	CVMconsolePrintf("PMI CALLEES: numCallees(%d) callerMb(0x%x) %C.%M\n",
			 numCallees, callerMb,
			 CVMmbClassBlock(callerMb), callerMb);
	for (i = 1; i <= numCallees; i++) {
	    CVMMethodBlock* calleeMb = callees[i];
	    if (printCallerInfo) {
		CVMconsolePrintf("PMI CALLEES: calleeIdx(%d)", i);
		CVMJITPMIcalleeTableDumpMB(calleeMb, printCallerInfo);
	    }
	}
    }
    
}
#endif

#endif /* CVM_JIT_PATCHED_METHOD_INVOCATIONS */


#ifdef CVM_AOT
static CVMBool
CVMjitInitializeAOTGlobals(CVMJITGlobalState* jgs)
{
    const CVMProperties *sprops = CVMgetProperties();
    const char *libpath = sprops->library_path;

    jgs->aotCompileFailed = CVM_FALSE;

    if (jgs->aotFile == NULL) {
        jgs->aotFile = (char*)malloc(strlen(libpath) +
                                     strlen("/cvm.aot") + 1);
        if (jgs->aotFile == NULL) {
            return CVM_FALSE;
        }
        *jgs->aotFile = '\0';
        strcat(jgs->aotFile, libpath);
        strcat(jgs->aotFile, "/cvm.aot");
    }

    if (jgs->aotMethodList == NULL) {
        jgs->aotMethodList = (char*)malloc(strlen(libpath) +
            strlen("/methodsList.txt") + 1);
        if (jgs->aotMethodList == NULL) {
            return CVM_FALSE;
        }
        *jgs->aotMethodList = '\0';
        strcat(jgs->aotMethodList, libpath);
        strcat(jgs->aotMethodList, "/methodsList.txt");
    }

    return CVM_TRUE;
}
#endif

CVMBool
CVMjitInit(CVMExecEnv* ee, CVMJITGlobalState* jgs,
	   const char* subOptionsString)
{
    jgs->compiling = CVM_FALSE;
    jgs->destroyed = CVM_FALSE;

    /*
     * Initialize any experimental options that we may not end up
     * supporting on the command line in some builds.
     */
    jgs->registerPhis = CVM_TRUE;
#ifdef CVM_JIT_REGISTER_LOCALS
    jgs->registerLocals = CVM_TRUE;
#endif
    jgs->compilingCausesClassLoading = CVM_FALSE;

    if (!CVMinitParsedSubOptions(&jgs->parsedSubOptions, subOptionsString)) {
	return CVM_FALSE;
    }
    if (!CVMprocessSubOptions(knownJitSubOptions, "-Xjit",
			      &jgs->parsedSubOptions)) {
	CVMjitPrintUsage();
	return CVM_FALSE;
    }


    handleDoPrivileged();

#ifdef CVM_AOT
    CVMjitInitializeAOTGlobals(jgs);
#endif
    
    /* Do the following after parsing options: */
#if defined(CVM_MTASK) || defined(CVM_AOT)
    /*
     * Prevent methods being compiled too early if AOT is enabled or it's
     * in MTASK server mode.
     * The JIT policy is initialized after MTASK or AOT compile the methods
     * in the method list.
     */
#ifdef CVM_AOT
    if (jgs->aotEnabled) 
#endif
    {
#ifdef CVM_MTASK
        if (!CVMglobals.isServer) {
	    goto initializeJITPolicy;
        }
#endif
        jgs->interpreterTransitionCost = 0;
        jgs->mixedTransitionCost = 0;
        jgs->backwardsBranchCost = 0;
        jgs->compileThreshold = CVMJIT_DEFAULT_CLIMIT;
    }
#ifdef CVM_AOT
    else
#endif
#endif
    {
#ifdef CVM_MTASK
initializeJITPolicy:
#endif
        if (!CVMjitPolicyInit(ee, jgs)) {
            return CVM_FALSE;
        }
    }
    if (!CVMjitSetInliningThresholds(ee, jgs)) {
        return CVM_FALSE;
    }

    if (!CVMJITstatsInitGlobalStats(&jgs->globalStats)) {
        return CVM_FALSE;
    }

    if (!CVMJITinitCompilerBackEnd()) {
        return CVM_FALSE;
    }

    if (!CVMJITcodeCacheInit(ee, jgs)) {
	return CVM_FALSE;
    }

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    if (!CVMJITPMIinit(jgs)) {
	return CVM_FALSE;
    }
#endif /* CVM_JIT_PATCHED_METHOD_INVOCATIONS */

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
    if (!CVMJITsimpleSyncInit(ee, jgs)) {
	return CVM_FALSE;
    }
#endif

#ifdef CVMJIT_INTRINSICS
    if (!CVMJITintrinsicInit(ee, jgs)) {
        return CVM_FALSE;
    }
#endif /* CVMJIT_INTRINSICS */

#ifdef CVM_DEBUG
    CVMconsolePrintf("JIT Configuration:\n");
    CVMprintSubOptionValues(knownJitSubOptions);
#endif

#ifdef CVM_DEBUG_ASSERTS
    /* The following contains sanity checks for the JIT system in general that
       does not have anything to do with the current context of compilation.
       These assertions can be called from anywhere with the same result. */
    CVMJITassertMiscJITAssumptions();
#endif
    return CVM_TRUE;
}

void
CVMjitDestroy(CVMJITGlobalState *jgs)
{
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    CVMJITPMIdestroy(jgs);
#endif
#ifdef CVMJIT_SIMPLE_SYNC_METHODS
	CVMJITsimpleSyncDestroy(jgs);
#endif
#ifdef CVMJIT_INTRINSICS
    CVMJITintrinsicDestroy(jgs);
#endif
    CVMJITcodeCacheDestroy(jgs);
    CVMJITdestroyCompilerBackEnd();

    CVMassert(jgs->inliningThresholds != NULL);
    free(jgs->inliningThresholds);
#ifdef CVM_DEBUG_ASSERTS
    jgs->inliningThresholds = NULL;
#endif

    CVMdestroyParsedSubOptions(&jgs->parsedSubOptions);
    CVMJITstatsDestroyGlobalStats(&jgs->globalStats);
    jgs->destroyed = CVM_TRUE;
}

void
CVMjitPrintUsage()
{
    CVMconsolePrintf("Valid -Xjit options include:\n");
    CVMprintSubOptionsUsageString(knownJitSubOptions);
}

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
/* Purpose: Compute the totalCompilationTime for the estimate. */
extern void
CVMjitEstimateCompilationSpeed(CVMExecEnv *ee)
{
    int i;
    CVMBool policyTriggeredDecompilations;
    CVMBool oldDoCSpeedMeasurement;

    CVMassert(CVMD_isgcSafe(ee));

    if (!CVMglobals.jit.doCSpeedTest) {
        return;
    }

    oldDoCSpeedMeasurement = CVMglobals.jit.doCSpeedMeasurement;
    CVMglobals.jit.doCSpeedMeasurement = CVM_TRUE;

    /* Just to be safe, turn off all policy triggered compilations: */
    policyTriggeredDecompilations =
        CVMglobals.jit.policyTriggeredDecompilations;
    CVMglobals.jit.policyTriggeredDecompilations = CVM_FALSE;

    /* We can estimate the compilation speed by simply compiling every
       preloaded method: */
    for (i = 0; i < CVM_nROMClasses; ++i) {
        const CVMClassBlock *cb = CVM_ROMClasses[i].classBlockPointer;
        int nmethods = CVMcbMethodCount(cb);
        int i;
        for (i = 0; i < nmethods; i++) {
            CVMMethodBlock *mb = CVMcbMethodSlot(cb, i);
            /* Only compile if it's a Java method (i.e. has bytecodes): */
            if (CVMmbInvokerIdx(mb) < CVM_INVOKE_CNI_METHOD) {
                CVMJITcompileMethod(ee, mb);
            }
        }
    }

    /* Restore the original policy triggered compilations setting: */
    CVMglobals.jit.policyTriggeredDecompilations =
        policyTriggeredDecompilations;

    CVMglobals.jit.doCSpeedMeasurement = oldDoCSpeedMeasurement;
}

/* Purpose: Report the estimated compilation speed. */
extern void
CVMjitReportCompilationSpeed()
{
    double methodsPerSec;
    double rate;
    double rateNoInline;
    double rateGenCode;

    CVMUint32 totalCompilationTime = CVMglobals.jit.totalCompilationTime;

    if (!CVMglobals.jit.doCSpeedMeasurement && !CVMglobals.jit.doCSpeedTest) {
        return;
    }

    totalCompilationTime = totalCompilationTime /
        CVMJIT_NUMBER_OF_COMPILATION_PASS_MEASUREMENT_REPETITIONS;

    methodsPerSec = (CVMglobals.jit.numberOfMethodsCompiled * 1000.0) /
                    totalCompilationTime;
    rate = (CVMglobals.jit.numberOfByteCodeBytesCompiled * 1000.0) /
           totalCompilationTime;
    rateNoInline =
        (CVMglobals.jit.numberOfByteCodeBytesCompiledWithoutInlinedMethods *
         1000.0) / totalCompilationTime;
    rateGenCode = (CVMglobals.jit.numberOfBytesOfGeneratedCode * 1000.0) /
                  totalCompilationTime;

    /* Report the compilation speed: */
    CVMconsolePrintf("Estimated Compilation Speed Results:\n");

    CVMconsolePrintf("    Total compilation time:         %d ms\n",
                     totalCompilationTime);
    CVMconsolePrintf("    Number of methods not compiled: %d\n",
                     CVMglobals.jit.numberOfMethodsNotCompiled);
    CVMconsolePrintf("    Number of methods compiled:     %d\n",
                     CVMglobals.jit.numberOfMethodsCompiled);
    CVMconsolePrintf("    Methods compiled per second:    %1.2f methods/sec\n",
                     methodsPerSec);

    CVMconsolePrintf("  Including inlined bytes:\n");
    CVMconsolePrintf("    Total byte code bytes compiled: %d bytes\n",
                     CVMglobals.jit.numberOfByteCodeBytesCompiled);
    CVMconsolePrintf("    Effective compilation speed:    %d bytes/sec "
                     "(%1.2f Kbytes/sec)\n",
                     (CVMUint32)rate, rate / 1024.0);

    CVMconsolePrintf("  Excluding inlined bytes:\n");
    CVMconsolePrintf("    Total byte code bytes compiled: %d bytes\n",
        CVMglobals.jit.numberOfByteCodeBytesCompiledWithoutInlinedMethods);
    CVMconsolePrintf("    Effective compilation speed:    %d bytes/sec "
                     "(%1.2f Kbytes/sec)\n",
                     (CVMUint32)rateNoInline, rateNoInline / 1024.0);

    CVMconsolePrintf("  Generated code bytes:\n");
    CVMconsolePrintf("    Total code bytes generated:     %d bytes\n",
        CVMglobals.jit.numberOfBytesOfGeneratedCode);
    CVMconsolePrintf("    Effective compilation speed:    %d bytes/sec "
                     "(%1.2f Kbytes/sec)\n",
                     (CVMUint32)rateGenCode, rateGenCode / 1024.0);
    CVMconsolePrintf("\n");
}
#endif /* CVM_JIT_ESTIMATE_COMPILATION_SPEED */

