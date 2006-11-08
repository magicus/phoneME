/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/common_exceptions.h"

#include "javavm/include/jit_common.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitstats.h"

#include "generated/javavm/include/opcodes.h"

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

    /* No inlining in this method. So we can trust the frame */
    if (inliningInfo == NULL) {
	iter->numEntries = 0;
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
CVMJITframeIterateSkip(CVMJITFrameIterator *iter, int skip,
    CVMBool skipArtificial, CVMBool popFrame)
{
    CVMInt32 pcOffset = iter->pcOffset;
    CVMBool found;

    do {
	++iter->index;
	found = CVM_FALSE;
	while (iter->index < iter->numEntries) {
	    CVMCompiledInliningInfoEntry* iEntry =
		&iter->inliningEntries[iter->index];
	
	    if (iEntry->pcOffset1 <= pcOffset && pcOffset < iEntry->pcOffset2 &&
		!(skipArtificial &&
		    (iEntry->flags & CVM_FRAMEFLAG_ARTIFICIAL) != 0))
	    {
		    found = CVM_TRUE;
		    break;
	    }
	    ++iter->index;
	}
    } while (found && --skip > 0);

    if (iter->index == iter->numEntries && skipArtificial &&
	(iter->frame->flags & CVM_FRAMEFLAG_ARTIFICIAL) != 0)
    {
	++iter->index;
    }

    if (!found && skip > 0) {
	CVMassert(iter->index == iter->numEntries);
	--skip;
	++iter->index;
    }

    if (popFrame) {
	/* update frame TOS and PC */
    }

    if (iter->index <= iter->numEntries) {
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

CVMUint32
CVMJITframeIterateCount(CVMJITFrameIterator *iter, CVMBool skipArtificial)
{
    CVMUint32 count = 0;
    while (CVMJITframeIterateSkip(iter, 1, skipArtificial, CVM_FALSE)) {
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
    CVMFrame *frame;
    if (iter->index == iter->numEntries) {
	frame = iter->frame;
	return CVMpcmapCompiledPcToJavaPc(frame->mb, CVMcompiledFramePC(frame));
    } else {
	/* No support for inlined frames yet */
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
    CVMInt32			  mapSize;
    CVMInt32			  paramSize;
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

	    while (CVMJITframeIterateSkip(&iter, 0, CVM_FALSE, CVM_FALSE)) {
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

	while (CVMJITframeIterateSkip(&iter, 0, CVM_FALSE, CVM_FALSE)) {
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

	while (CVMJITframeIterateSkip(&iter, 0, CVM_FALSE, CVM_FALSE)) {
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
                    /* The method is sync, so lock the object. */
                    /* %comment l002 */
                    if (!CVMfastTryLock(ee,
			CVMID_icellDirect(ee, receiverObjICell)))
		    {
			if (!CVMobjectLock(ee, receiverObjICell)) {
                            CVMthrowOutOfMemoryError(ee, NULL);
			    CVMassert(frameSanity(frame, topOfStack));
 			    ee->invokeMb = NULL;
                           return CVM_COMPILED_EXCEPTION;
			}
                    }
		    CVMID_icellAssignDirect(ee,
			&CVMframeReceiverObj(frame, Compiled),
			receiverObjICell);
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
                if ((CVMUint8*)cmd >= CVMglobals.jit.codeCacheDecompileStart) {
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
    CVMcmdEntryCount(cmd)++;
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
    { "bctoir",         CVM_DEBUGFLAG(TRACE_JITBCTOIR) },
    { "codegen",        CVM_DEBUGFLAG(TRACE_JITCODEGEN) },
    { "stats",          CVM_DEBUGFLAG(TRACE_JITSTATS) },
    { "iropt",          CVM_DEBUGFLAG(TRACE_JITIROPT) },
    { "inlining",       CVM_DEBUGFLAG(TRACE_JITINLINING) },
    { "osr",            CVM_DEBUGFLAG(TRACE_JITOSR) },
    { "reglocals",      CVM_DEBUGFLAG(TRACE_JITREGLOCALS) },
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
     &CVMglobals.jit.maxInliningDepth},

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
     {{0, 32*1024*1024, CVMJIT_DEFAULT_CODE_CACHE_SIZE}},
     &CVMglobals.jit.codeCacheSize},

    {"upperCodeCacheThreshold", "Upper Code Cache Threshold", 
     CVM_PERCENT_OPTION, 
     {{0, 100, CVMJIT_DEFAULT_UPPER_CCACHE_THR}},
     &CVMglobals.jit.upperCodeCacheThresholdPercent},

    {"lowerCodeCacheThreshold", "Lower Code Cache Threshold", 
     CVM_PERCENT_OPTION, 
     {{0, 100, CVMJIT_DEFAULT_LOWER_CCACHE_THR}},
     &CVMglobals.jit.lowerCodeCacheThresholdPercent},

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
static CVMBool
CVMJITpolicyInit(CVMExecEnv* ee, CVMJITGlobalState* jgs)
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
    } /* Otherwise do nothing.
         The default [im]cost and climit will do the work */

    /* Set up the inlining threshold table: */
    {
        CVMInt32 depth = jgs->maxInliningDepth;
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
            costTable[i] = 0;
        }

        jgs->inliningThresholds = costTable;
#undef QUAD_COEFFICIENT
#undef EFFECTIVE_MAX_DEPTH_THRESHOLD
#undef MIN
    }

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

    /* Re-build cost table */
    if (!CVMJITpolicyInit(ee, jgs)) {
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
    CVMconsolePrintf("JIT Configuration:\n");
    CVMprintSubOptionValues(knownJitSubOptions);
#endif

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
    
    /* Do the following after parsing options: */
    if (!CVMJITpolicyInit(ee, jgs)) {
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
#ifdef CVMJIT_INTRINSICS
    CVMJITintrinsicDestroy(jgs);
#endif
    CVMJITcodeCacheDestroy(jgs);
    CVMJITdestroyCompilerBackEnd();
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
