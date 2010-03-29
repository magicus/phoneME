/*
 * @(#)jitcodebuffer.c	1.102 06/10/10
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
 * Code buffer management.
 * The main code buffer has positive relative addresses.
 * Every "push" operation allocates a new buffer segment at a negative
 * address. "Pop" returns you to the previous buffer stream.
 *
 * Currently, push is limited to depth 4, simply so I can allocate a
 * fixed-size stack, and this seems quite adequate for allowing
 * exceptions within late bindings. Depth 2 should be adequate.
 * Currently all we really do is manipulate the logical PC, which is
 * used by the emitters and the fixup mechanism.  Obviously this
 * should be part of the compiler context, thus the "con"
 * parameters. But for now we don't even bother.  
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitmemory.h"

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/porting/io.h"

#ifdef CVM_JIT_PROFILE
#include "javavm/include/porting/time.h"
#endif

/*
 * Data structure used to maintain lists of freed buffers.
 */
struct CVMJITFreeBuf{
    CVMUint32      size;
    CVMJITFreeBuf* next;
    CVMJITFreeBuf* prev;
};

typedef struct {
    CVMUint32      size;
    CVMUint32      cmdOffset;
} CVMJITAllocedBuf;

static void
CVMJITcodeCacheAddBufToFreeList(CVMUint8* cbuf);

static void
CVMJITcodeCacheRemoveBufFromFreeList(CVMUint8* cbuf);

static CVMUint8*
CVMJITcodeCacheFindFreeBuffer(CVMJITCompilationContext* con,
			      CVMUint32 bufSizeEstimate,
			      CVMBool remove);

static void
CVMJITcodeCacheMakeRoomForMethod(CVMJITCompilationContext* con,
				 CVMUint32 bufSize);

#define CVMJITcbufAlignAddress(addr) \
    (CVMUint8*)(((CVMUint32)(addr) + 3) & ~3)

/*
 * Called before we start to compile the method.
 */
void
CVMJITcbufInitialize(CVMJITCompilationContext* con)
{
    con->curDepth       = 0;    
#ifdef CVM_DEBUG_ASSERTS
    con->oolCurDepth    = -1;
#endif
    
    /* estimate */
    con->numMainLineInstructionBytes  = 0;
    con->numBarrierInstructionBytes = 0;
    con->numVirtinlineBytes = 0;
    con->numLargeOpcodeInstructionBytes = 0;
    
    con->codeBufAddr  = NULL;
    
    /*
     * Try to make sure that there is enough memory in the code cache for
     * the method. This will help prevent wasting time repeatedly trying
     * to compile a method that won't fit in the code cache. Note that 
     * there is no guarantee that the size guess is big enough.
     */
    {
	CVMUint32 bufSizeEstimate = CVMmbCodeLength(con->mb) * 8;
	CVMUint8* cbuf = 
	    CVMJITcodeCacheFindFreeBuffer(con, bufSizeEstimate, CVM_FALSE);
	if (cbuf == NULL) {
	    CVMJITcodeCacheMakeRoomForMethod(con, bufSizeEstimate);
	}
    }
}

#define CVMJITcbufSetCmd0(cbuf, _cmd)	\
    ((CVMJITAllocedBuf *)(cbuf))->cmdOffset = (CVMUint32)(_cmd) - (CVMUint32)cbuf;

void 
CVMJITcbufSetCmd(CVMUint8 *cbuf, CVMCompiledMethodDescriptor *cmd)
{
    CVMJITcbufSetCmd0(cbuf, cmd);
}

/*
 * Allocate buffer and set pointers. Called just before we generate code.
 */
void
CVMJITcbufAllocate(CVMJITCompilationContext* con, CVMSize extraSpace)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint32 bufSizeEstimate;
    CVMUint8* cbuf;

    int extraExpansion = con->extraCodeExpansion;

    con->numMainLineInstructionBytes = CVMCPU_ENTRY_EXIT_SIZE +
       (con->codeLength * (CVMCPU_CODE_EXPANSION_FACTOR + extraExpansion));

    bufSizeEstimate = CVMCPU_INITIAL_SIZE + 
	con->numMainLineInstructionBytes +
	con->numBarrierInstructionBytes +
	con->numLargeOpcodeInstructionBytes +
	con->numVirtinlineBytes;

    /* Make room for CVMCompiledMethodDescriptor */
    bufSizeEstimate += sizeof(CVMCompiledMethodDescriptor);

    /* Make room for inliningInfo, gcCheckPcs, and pcMapTable */
    if (extraSpace > 0) {
	bufSizeEstimate += extraSpace;
	bufSizeEstimate += sizeof (CVMUint32);
    }

    /* Make room for stackmaps */
    bufSizeEstimate += con->extraStackmapSpace;

    bufSizeEstimate = CVMpackSizeBy(bufSizeEstimate, 4);
    
    if (bufSizeEstimate > 0xffff ||  
	bufSizeEstimate > jgs->maxCompiledMethodSize)
    {
	const char formatStr[] =
	    "Method is too big: estimated size=%d allowed=%d";

	/* Max int is only 10 digits.  Allow 2 10-digit numbers for the sizes
	   plus 1 char for '\0': */
	int length = sizeof(formatStr) + 10 + 10 + 1;
	char *msg = CVMJITmemNew(con, JIT_ALLOC_DEBUGGING, length);
	CVMUint32 limit = 0xffff < jgs->maxCompiledMethodSize ?
	                  0xffff : jgs->maxCompiledMethodSize;

	sprintf(msg, formatStr, bufSizeEstimate, limit);
	CVMJITlimitExceeded(con, msg);
    }

    /* Start with the last possible instruction in this buffer (sort of
       like maxLogicalPC) */
    con->earliestConstantRefPC = MAX_LOGICAL_PC;

#ifdef CVM_JIT_USE_FP_HARDWARE    
    con->earliestFPConstantRefPC = MAX_LOGICAL_PC;
#endif    

    /* find a free buffer to generate code into */
    cbuf = CVMJITcodeCacheFindFreeBuffer(con, bufSizeEstimate, CVM_TRUE);
#ifdef CVM_AOT
    if (CVMglobals.jit.isPrecompiling) {
        /* If we are doing AOT compilation, make sure the cbuf is
         * allocated within the AOT code cache */
        if (cbuf == NULL ||
            (cbuf + bufSizeEstimate) > CVMglobals.jit.codeCacheAOTEnd) {
            CVMconsolePrintf("WARNING: Code cache is full during AOT"
                             "compilation. Please use a larger AOT "
                             "code cache using -Xjit:aotCodeCacheSize=<size>.\n");
            CVMabort();
        }
    }
#endif
    if (cbuf == NULL) {
	/* Make sure there is enough memory in the code cache */
	CVMJITcodeCacheMakeRoomForMethod(con, bufSizeEstimate);
	/* Try again. This time it's guaranteed to succeed. */
	cbuf = CVMJITcodeCacheFindFreeBuffer(con, bufSizeEstimate, CVM_TRUE);
	CVMassert(cbuf != NULL);
    }

    con->codeBufAddr = cbuf;

    /* Make sure codeBufEnd doesn't allow for a method that is too big. */
    {
	CVMUint32 cbufSize;
	if (CVMJITcbufSize(cbuf) > jgs->maxCompiledMethodSize) {
	    cbufSize = jgs->maxCompiledMethodSize;
	} else {
	    cbufSize = CVMJITcbufSize(cbuf);
	}
	/* Leave room for the size tag at the end of the buffer. */
	con->codeBufEnd = cbuf + cbufSize - sizeof(CVMUint32);
    }

    /* Skip size header */
    con->curPhysicalPC = (CVMUint8 *)&((CVMJITAllocedBuf *)cbuf)->cmdOffset;

    /* CMD pointer */
    CVMJITcbufSetCmd0(cbuf, 0);
    con->curPhysicalPC += sizeof ((CVMJITAllocedBuf *)cbuf)->cmdOffset;

    CVMtraceJITCodegenExec({
        CVMconsolePrintf("NUM BARRIER BYTES = %d\n",
                         con->numBarrierInstructionBytes);
        CVMconsolePrintf("NUM VIRTUAL INLINE BYTES = %d\n",
                         con->numVirtinlineBytes);
        CVMconsolePrintf("NUM LARGE OPCODE BYTES = %d\n",
                         con->numLargeOpcodeInstructionBytes);
        CVMconsolePrintf("NUM MAIN LINE INSTRUCTION BYTES ESTIMATE = %d\n",
                         con->numMainLineInstructionBytes);
        CVMconsolePrintf("ESTIMATED BUFFER SIZE = %d\n",
                         bufSizeEstimate);
        CVMconsolePrintf("CODE BUFFER ADDRESS = 0x%x\n",
                         con->codeBufAddr);
    });
}

/*
 * Called when done compiling and we want to commit the code buffer.
 * Anything free space at the end of the code buffer is freed up.
 */
void
CVMJITcbufCommit(CVMJITCompilationContext* con)
{
    CVMUint8* cbuf = con->codeBufAddr;
    CVMUint32 bufSize;
    CVMUint32 origBufSize = CVMJITcbufSize(cbuf);
    CVMUint8* cbufEnd;

    con->curPhysicalPC += sizeof(CVMUint32); /* adjust for size field at end */
    con->curLogicalPC  += sizeof(CVMUint32); /* adjust for size field at end */

    cbufEnd = CVMJITcbufAlignAddress(con->curPhysicalPC);
    bufSize = cbufEnd - cbuf;    

    /*
     * Assert that the code did not overrun the actual end of the code buffer
     */
    CVMassert(((CVMInt32)(origBufSize - bufSize)) >= 0);
    CVMassert(CVMsysMutexIAmOwner(con->ee, &CVMglobals.jitLock));
    
    /* If there is enough extra at the end of this buffer, then free it */
    if (origBufSize - bufSize >= sizeof(CVMJITFreeBuf) + sizeof(CVMUint32)) {
	CVMUint8* freeBuf = cbuf + bufSize;
	CVMJITcbufSetCommittedBufSize(cbuf, bufSize); /* commit the buffer */
	CVMJITcbufSetUncommitedBufSize(freeBuf, origBufSize - bufSize);
	CVMJITcbufFree(freeBuf, CVM_FALSE);
    } else {
	bufSize = origBufSize;
	CVMJITcbufSetCommittedBufSize(cbuf, bufSize); /* commit the buffer */
    }

#ifdef CVM_AOT
    if (CVMglobals.jit.isPrecompiling) {
        /* If we are doing AOT compilation, make sure the cbuf is
         * allocated within the AOT code cache */
        if ((cbuf + CVMJITcbufSize(cbuf)) > CVMglobals.jit.codeCacheAOTEnd) {
            CVMconsolePrintf("WARNING: Code cache is full during AOT"
                             "compilation. Please use a larger AOT "
                             "code cache using-Xjit:aotCodeCacheSize=<size>.\n");
            CVMabort();
        }
    }
#endif

    /* Update the total bytes of the cache that are allocated */
    CVMglobals.jit.codeCacheBytesAllocated += CVMJITcbufSize(cbuf);
#if 0
    CVMtraceJITStatus(("JS: Allocated(%d) - code cache now %d%% full\n",
		       CVMJITcbufSize(cbuf),
		       (CVMglobals.jit.codeCacheBytesAllocated * 100 /
			CVMglobals.jit.codeCacheSize)));
#endif

#ifdef CVM_JIT_COLLECT_STATS
    /* Keeps stats gathering from thinking we wasted a bunch */
    con->codeBufEnd = cbuf + bufSize;
#endif
}

static void
CVMJITsetCodeCacheDecompileStart(CVMUint8 *p)
{
    CVMglobals.jit.codeCacheDecompileStart = p;
    /* If the start of the allowable decompilation region gets moved, then
       we must make sure that the next decompilation scan does not start
       before this allowed region: */
    if (CVMglobals.jit.codeCacheNextDecompileScanStart < p) {
        CVMglobals.jit.codeCacheNextDecompileScanStart = p;
   }

#if 0
    TRACE_RECORD_REGION("Decompile Space",
	p,
	CVMglobals.jit.codeCacheEnd);
#endif
}

void
CVMJITmarkCodeBuffer()
{
    /* Locking? */
#ifdef CVM_AOT
    if (!CVMglobals.jit.codeCacheAOTCodeExist)
#endif
    {
        if (CVMglobals.jit.codeCacheFirstFreeBuf != NULL) {
            CVMJITsetCodeCacheDecompileStart(
                (CVMUint8*)CVMglobals.jit.codeCacheFirstFreeBuf);
        } else {
            CVMJITsetCodeCacheDecompileStart(
                CVMglobals.jit.codeCacheEnd);
        }
    }

#ifdef CVM_USE_MEM_MGR
    {
	/* shared codecache */
	CVMmemRegisterCodeCache();
    }
#endif /* CVM_USE_MEM_MGR */

#ifdef CVM_AOT
    /* We are done AOT compilation. All methods compiled after this
     * point will not saved as AOT code.
     */
    CVMJITcodeCachePersist(&CVMglobals.jit);
#ifndef CVM_MTASK
    /* If CVM_MTASK is enabled, isPrecompiling is set to
     * false in the child VM.
     */
    CVMglobals.jit.isPrecompiling = CVM_FALSE;
#endif
#endif
}

/*
 * Called to free a buffer. "committed" is a flag that indicates if
 * CVMJITcbufCommit() has been called on the buffer or not.
 */
void
CVMJITcbufFree(CVMUint8* cbuf, CVMBool committed) {
    CVMUint32 cbufSize = CVMJITcbufSize(cbuf);
    CVMJITGlobalState* jgs = &CVMglobals.jit;

    /*
     * Only adjust bytes allocated if CVMJITcbufCommit() was called
     * on this buffer.
     */
    if (committed) {
	jgs->codeCacheBytesAllocated -= cbufSize;
    }

#if 0
    CVMtraceJITStatus(("JS: Free(%d) - code cache now %d%% full\n",
		       cbufSize,
		       (jgs->codeCacheBytesAllocated * 100 /
			jgs->codeCacheSize)));
#endif

    /* Merge with next buffer if it is free */
    {
	CVMUint8* nextBuf = CVMJITcbufNextBuf(cbuf);
	if (nextBuf != jgs->codeCacheEnd) {
	    if (CVMJITcbufIsFree(nextBuf)) {
		CVMJITcodeCacheRemoveBufFromFreeList(nextBuf);
		cbufSize += CVMJITcbufSize(nextBuf);
	    }
	}
    }

    /* Merge with previous buffer if it is free */
    if (cbuf != jgs->codeCacheStart) {
	CVMUint8* prevBuf = CVMJITcbufPrevBuf(cbuf);
	if (CVMJITcbufIsFree(prevBuf)) {
	    /* Fixup codeCacheDecompileStart if necessary. This can only
	     * happen because of class unloading or shutdown. */
	    if (cbuf == jgs->codeCacheDecompileStart) {
		CVMdebugPrintf(("WARNING: codeCacheDecompileStart lowered\n"));
		CVMJITsetCodeCacheDecompileStart(prevBuf);
	    }
	    CVMJITcodeCacheRemoveBufFromFreeList(prevBuf);
	    cbufSize += CVMJITcbufSize(prevBuf);
	    cbuf = prevBuf;
	}
    }

    /* Add the buffer to the free list. */
    CVMJITcbufSetFreeBufSize(cbuf, cbufSize);
    CVMJITcodeCacheAddBufToFreeList(cbuf);

    /* Start the next decompilation scan after this freed buffer so as to
       avoid immediately decompiling the method which may immediately
       be compiled into this freed buffer. */
    jgs->codeCacheNextDecompileScanStart = cbuf + CVMJITcbufSize(cbuf);

    /* Make sure we don't start at the end of the codeCache next time.  If
       we're pointing to the end of the codeCache, then change it to point
       to the start: */
    if (jgs->codeCacheNextDecompileScanStart == jgs->codeCacheEnd) {
	jgs->codeCacheNextDecompileScanStart =
	    jgs->codeCacheDecompileStart;
    }
}

/*
 * Allow re-emit fixups.
 */
void
CVMJITcbufPushFixup(CVMJITCompilationContext* con, CVMInt32 fixupPC)
{
    CVMassert(con->curDepth<4);

    /* Push context and re-map to out-of-line space */
    con->logicalPCstack[con->curDepth++] = con->curLogicalPC;
    con->curLogicalPC = fixupPC;
    con->curPhysicalPC = CVMJITcbufLogicalToPhysical(con, fixupPC);

#ifdef CVM_DEBUG_ASSERTS
    /* This is not for OOL code, so the bounds are the same as before: */
    {
        CVMUint8* endBound;
        if (con->oolCurDepth >= 0) {
            endBound = con->oolPhysicalEndPCstack[con->oolCurDepth];
        } else {
            endBound = con->codeBufEnd;
        }
        con->oolCurDepth++;
        CVMassert(con->oolCurDepth<4);
        con->oolPhysicalStartPCstack[con->oolCurDepth] = con->curPhysicalPC;
        con->oolPhysicalEndPCstack[con->oolCurDepth] = endBound;
    }
#endif

    /* Make sure we don't push out of bounds */
    CVMassert(con->curPhysicalPC >= con->codeBufAddr);

    CVMtraceJITCodegen((">>>>>>>>>Push Code Buffer to PC = %d (0x%x) "
                        ">>>>>>>>\n", con->curLogicalPC, con->curPhysicalPC));
}

extern void
CVMJITcbufPop(CVMJITCompilationContext* con)
{
#ifdef CVM_DEBUG_ASSERTS
    /* Make sure we have not overflowed the computed OOL code buffer size: */
    {
        CVMInt32 depth = con->oolCurDepth;
        CVMUint8* curPhysicalPC = (con)->curPhysicalPC;
        CVMassert(curPhysicalPC >= con->oolPhysicalStartPCstack[depth]);
        CVMassert(curPhysicalPC <= con->oolPhysicalEndPCstack[depth]);
        CVMassert(con->oolCurDepth > -1);
        con->oolCurDepth--;
    }
#endif

    CVMassert(con->curDepth>0);
    con->curLogicalPC  = con->logicalPCstack[--con->curDepth];
    con->curPhysicalPC = CVMJITcbufLogicalToPhysical(con, con->curLogicalPC);
    CVMtraceJITCodegen(("<<<<<<<<<Pop Code Buffer to PC = %d (0x%x) "
                        "<<<<<<<<<\n", con->curLogicalPC, con->curPhysicalPC));
}

/**************************
 * Code Cache Management
 **************************/

/*
 * If requested, copy the ccm assembler code to the start of the code cache.
 */
static CVMBool
CVMJITcodeCacheCopyCCMCode(CVMExecEnv* ee)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    jgs->codeCacheGeneratedCodeStart = jgs->codeCacheStart;
    return CVM_TRUE;
#else
    volatile CVMBool retVal;
    CVMJITCompilationContext con;
    CVMJITinitializeContext(&con, NULL, ee, -1);

    /* Code cache access. Keep from asserting. */
    CVMsysMutexLock(ee, &CVMglobals.jitLock);

    if (setjmp(con.errorHandlerContext) == 0) {
	/* Allocate a buffer for the codecache copy of ccm code. */
	CVMUint8* cbuf = 
	    CVMJITcodeCacheFindFreeBuffer(&con, jgs->codeCacheSize, CVM_TRUE);
	int ccmCodeCacheCopySize =
	    &CVMCCMcodeCacheCopyEnd - &CVMCCMcodeCacheCopyStart;

	CVMJITinitializeCompilation(&con);

	/*
	 * Copy code into start of code cache. The sizeof(CVMUint32)
	 * adjustments are for leaving room for buffer size fields.
	 */
	con.codeBufAddr   = cbuf;
	con.curPhysicalPC = cbuf + sizeof(CVMUint32);
	con.codeEntry     = cbuf + sizeof(CVMUint32);
	con.codeBufEnd    = jgs->codeCacheEnd - sizeof(CVMUint32);
	
	if (con.codeEntry + ccmCodeCacheCopySize > con.codeBufEnd) {
	    CVMJITerror(&con, OUT_OF_MEMORY,
			"Insufficient Code Cache space to copy CCM glue");
	}
	memcpy(con.curPhysicalPC, &CVMCCMcodeCacheCopyStart,
	       ccmCodeCacheCopySize);
	CVMJITflushCache(con.curPhysicalPC,
			 con.curPhysicalPC + ccmCodeCacheCopySize);
	jgs->ccmCodeCacheCopyAddress = con.curPhysicalPC;
	
	/* account for everything added to the start of the code cache */
	con.curPhysicalPC += ccmCodeCacheCopySize;
	con.curLogicalPC  += ccmCodeCacheCopySize;
	/* Mark where the copied ccm code ends and generated code starts. */
	jgs->codeCacheGeneratedCodeStart =
	    jgs->codeCacheStart + ccmCodeCacheCopySize;
	
#if defined(CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES)
	/*
	 * Massage the ccmCodeCacheCopyEntries table so it points to the
	 * code in the code cache rather than in ccmcodecachecopy_cpu.o.
	 */
	{
	    int i = 0;
	    CVMUint8* prevHelper = NULL;
	    CVMCCMCodeCacheCopyEntry* entries = jgs->ccmCodeCacheCopyEntries;
	    do {
		CVMUint8* currHelper = entries[i].helper;
		/* make sure table entries are in proper order */
		CVMassert(currHelper >= prevHelper);
		prevHelper = currHelper;
		entries[i].helper = CVMJIT_CCMCODE_ADDR(currHelper);
	    } while (entries[i++].helperName != NULL);
	}
#endif
#if defined(CVMJIT_PATCH_BASED_GC_CHECKS) && CVMCPU_NUM_CCM_PATCH_POINTS > 0
	/*
	 * Massage the ccmGcPatchPoints table so it points to the
	 * code in the code cache rather than in ccmcodecachecopy_cpu.o.
	 */
	{
	    int i;
	    for (i=0; i < CVMCPU_NUM_CCM_PATCH_POINTS; i++) {
		CVMUint8* gcPatchPoint = jgs->ccmGcPatchPoints[i].patchPoint;
		if (gcPatchPoint == NULL) {
		    continue;
		}
		jgs->ccmGcPatchPoints[i].patchPoint = 
		    CVMJIT_CCMCODE_ADDR(gcPatchPoint);
	    }
	}
#endif

	/*
	 * Commit the buffer, which will also free the unused part.
	 */
	CVMJITcbufCommit(&con);
	CVMJITcbufSetCCMCopyBufSize(cbuf, CVMJITcbufSize(cbuf));
	CVMJITsetCodeCacheDecompileStart((CVMUint8*)jgs->codeCacheFirstFreeBuf);

	CVMJITdestroyContext(&con);
	retVal = CVM_TRUE;
    } else {
	CVMdebugPrintf(("Jit code cache too small to initialize.\n"));
	CVMJITdestroyContext(&con);
	retVal = CVM_FALSE;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
    return retVal;
#endif /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */
}

CVMBool
CVMJITcodeCacheInitOptions(CVMJITGlobalState *jgs)
{
    /*
     * Initialize options:
     *
     * If the lowerCodeCacheThresholdPercent is -1, then the user didn't
     * set it and we should use 95% of the upperCodeCacheThresholdPercent.
     */
    if (jgs->lowerCodeCacheThresholdPercent == -1) {
	jgs->lowerCodeCacheThresholdPercent =
	    (CVMUint32)(0.95 * jgs->upperCodeCacheThresholdPercent);
    }
    jgs->lowerCodeCacheThreshold = 
	jgs->lowerCodeCacheThresholdPercent * jgs->codeCacheSize / 100;
    jgs->upperCodeCacheThreshold =
	jgs->upperCodeCacheThresholdPercent * jgs->codeCacheSize / 100;

#ifdef CVM_JIT_PROFILE
    /*
     * Open the profile output file, allocate the profile buffer,
     * and enable profiling.
     */
    if (jgs->profile_fd != -1) {
	close(jgs->profile_fd);
	free(jgs->profileBuf);
	jgs->profile_fd = -1;
    }
    
    CVMassert(jgs->profile_fd == -1);
    if (jgs->profile_filename != NULL) {
	jgs->profile_fd = CVMioOpen(jgs->profile_filename,
				    O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (jgs->profile_fd == -1) {
	    CVMconsolePrintf("Could not open profile output file: \"%s\"\n",
			     jgs->profile_filename);
	    return CVM_FALSE;
	}
	jgs->profileBuf = malloc(CVMJITprofileBufferSize());
	if (jgs->profileBuf == NULL) {
	    CVMconsolePrintf("Could not allocate profile buffer.\n");
	    return CVM_FALSE;
	}
	CVMJITprofileEnable();
	jgs->profileStartTime = CVMtimeMillis();
    }
#endif

    return CVM_TRUE;
}


CVMBool
CVMJITcodeCacheInit(CVMExecEnv* ee, CVMJITGlobalState *jgs)
{
#ifdef CVM_USE_MEM_MGR
    /* Add the code cache as a custom memory type. */
    jgs->codeCacheMemType = CVMmemAddCustomMemoryType(
        "Code Cache", NULL, NULL, CVMmemCodeCacheWriteNotify, NULL);
#endif
    
    /*
     * Assume we're utilizing everything and adjust lower later when we
     * start to force some decompilations to make more room.
     */
    jgs->codeCacheLowestForcedUtilization = jgs->codeCacheSize;

#ifdef CVM_AOT
    /* Find existing AOT code. If it doesn't exist, 
       CVMfindAOTCode allocates a big consecutive 
       code cache for both AOT and dynamic
       compilation. */
    if (jgs->aotEnabled) {
        if (!CVMfindAOTCode(jgs)) {
	    /* No existing AOT code found. The code cache
             * for AOT and JIT compilation has been allocated
               by CVMfindAOTCode. */ 
	    goto skipAllocateDynamicPart;
        }
    }
#endif
    {
        /* Allocate the dynamic part */
        /*
         *  Allocate the code cache and add it to the free list.
         */
#ifdef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
         jgs->codeCacheStart = CVMJITallocCodeCache(&jgs->codeCacheSize);
#elif defined(CVM_USE_MEM_MGR)
        /* Align the code cache start at page boundary. */
        jgs->codeCacheStart = CVMmemalignAlloc(CVMgetPagesize(),
                                           jgs->codeCacheSize);
        if ((int)jgs->codeCacheStart % CVMgetPagesize() != 0) {
            CVMpanic("CVMmemalignAlloc() returns unaligned storage");
        }
#else
        jgs->codeCacheStart = malloc(jgs->codeCacheSize);
#endif
    }
#ifdef CVM_AOT
skipAllocateDynamicPart:
#endif

    if (jgs->codeCacheStart == NULL) {
	    return CVM_FALSE;
    }

    jgs->codeCacheEnd = &jgs->codeCacheStart[jgs->codeCacheSize];

#if 0
    TRACE_RECORD_REGION("Code Cache", jgs->codeCacheStart,
	jgs->codeCacheEnd);
#endif

    jgs->codeCacheNextDecompileScanStart = jgs->codeCacheStart;
    CVMJITsetCodeCacheDecompileStart(jgs->codeCacheStart);

    jgs->codeCacheFirstFreeBuf = NULL;
    CVMJITcbufSetUncommitedBufSize(jgs->codeCacheStart, jgs->codeCacheSize);
    CVMJITcbufFree(jgs->codeCacheStart, CVM_FALSE);

#ifdef CVM_AOT
    /* If no AOT code exist, we will compile a list of methods and
     * save the compiled code as AOT code. */
    jgs->isPrecompiling = !(jgs->codeCacheAOTCodeExist);
#endif
#ifdef CVM_MTASK
    /* Server does the warmup compilation. */
    jgs->isPrecompiling = (CVMglobals.isServer && CVMglobals.clientId <= 0);
#endif
#if defined(CVM_AOT) || defined(CVM_MTASK)
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /* Disable PMI during pre-compiling. CVMglobals.jit.pmiEnabled
       will be reset properly later by re-processing the -Xjit 
       options when we are done pre-compilation or finihing
       initializing AOT code.
     */
    jgs->pmiEnabled = !(jgs->isPrecompiling);
#endif
#endif


#ifdef CVM_JIT_PROFILE
    jgs->profile_fd = -1;
#endif

    if (!CVMJITcodeCacheInitOptions(jgs)) {
	return CVM_FALSE;
    }

    /* If requested, copy the ccm assembler code to the code cache. */
    return CVMJITcodeCacheCopyCCMCode(ee);
}

static CVMCompiledMethodDescriptor *
cbuf2cmd(CVMUint8 *cbuf) {
    CVMCompiledMethodDescriptor *cmd =
	(CVMCompiledMethodDescriptor *)(((CVMJITAllocedBuf *)cbuf)->cmdOffset + cbuf);
    CVMassert(CVMJITcbufIsCommitted(cbuf));
    CVMassert(cmd->magic == CVMJIT_CMD_MAGIC);
    return cmd;
}

CVMCompiledMethodDescriptor *
CVMJITcbufCmd(CVMUint8 *cbuf) {
    return cbuf2cmd(cbuf);
}

#ifdef CVM_AOT
/*
 * Iterate through codecache and find all precompiled methods. 
 * Also set up various codecache values
 */
CVMBool
CVMJITinitializeAOTCode()
{
    CVMJITGlobalState *jgs = &CVMglobals.jit;
    if (jgs->codeCacheAOTCodeExist == CVM_TRUE) {
        CVMUint8* cbuf = jgs->codeCacheAOTStart;

        while (cbuf < jgs->codeCacheAOTEnd) {
            CVMUint32 bufSize = CVMJITcbufSize(cbuf);
            if (CVMJITcbufIsCommitted(cbuf)) {
                CVMCompiledMethodDescriptor *cmd = cbuf2cmd(cbuf);
                CVMMethodBlock* mb = CVMcmdMb(cmd);

                /* 
                 * So we found a pre-compiled method. Now fix the mb's
                 * jitInvokerX and startPCX, so they point to the start
                 * of the compiled code. Setting the mb's startPCX also
                 * make the method being marked as compiled.
                 */
	        CVMmbJitInvoker(mb) = (void*)(cmd + 1);
                CVMmbStartPC(mb) = (CVMUint8*)(cmd + 1);
                if (jgs->codeCacheAOTGeneratedCodeStart == 0) {
		    jgs->codeCacheAOTGeneratedCodeStart = cbuf;
                }
                CVMtraceJITStatus(
		    ("JS: Found pre-compiled method %C.%M, startPC=0x%x\n",
                    CVMmbClassBlock(mb), mb, CVMmbStartPC(mb)));
            }
            cbuf += bufSize;
        }
#ifndef CVM_MTASK
        /* Need to initialize the [im]cost, climit and pmiEnabled 
         * by re-processing the JIT options.
         * Dynamic compilation and PMI were disabled until this point. If
         * CVM_MTASK is enabled, CVMjitPolicyInit() is eventually called
         * by sun.misc.Warmup.runit(). */
        CVMjitProcessOptionsAndPolicyInit(CVMgetEE(), jgs);
#endif
    }
    return jgs->codeCacheAOTCodeExist;
}
#endif

void
CVMJITcodeCacheDestroy(CVMJITGlobalState *jgs)
{
   CVMUint8* cbuf = jgs->codeCacheStart;

#ifdef CVM_DEBUG
   CVMJITcodeCacheVerify(CVM_FALSE);
#endif

    /*
     * Decompile everything first.
     */
    while (cbuf < jgs->codeCacheEnd) {
	if (CVMJITcbufIsCommitted(cbuf)) {
	    CVMCompiledMethodDescriptor* cmd = cbuf2cmd(cbuf);
	    CVMMethodBlock* mb = CVMcmdMb(cmd);
	    CVMJITdecompileMethod(NULL, mb);
	}
	cbuf += CVMJITcbufSize(cbuf);
    }

    if (jgs->codeCacheStart != NULL) {
#ifdef CVM_AOT
        if (CVMJITAOTcodeCacheDestroy(jgs))
#endif
        {
#ifdef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
 	    CVMJITfreeCodeCache(jgs->codeCacheStart);
#else
 	    free(jgs->codeCacheStart);
#endif
        }
    }

#ifdef CVM_JIT_PROFILE
    /* free up profiling resources */
    if (jgs->profile_filename != NULL) {
	free(jgs->profile_filename);
    }
    if (jgs->profile_fd != -1) {
	CVMioClose(jgs->profile_fd);
    }
    if (jgs->profileBuf != NULL) {
	free(jgs->profileBuf);
    }
#endif
}

/***********************************************************************
 * Aging and Decompiling
 *
 * Age the entryCount of all compiled methods and decompile those that
 * reach 0 if we need to make more space. All threads must be gcSafe
 * when we decompile, but not just to do aging.
 ***********************************************************************/

static void
preventMethodFromDecompiling(CVMMethodBlock* mb)
{
    CVMCompiledMethodDescriptor* cmd = CVMmbCmd(mb);
    /* make sure method survives for 2 aging processes after this one */
    if (CVMcmdEntryCount(cmd) < 8 
        && (CVMUint8*)cmd >= CVMglobals.jit.codeCacheDecompileStart
        && (CVMUint8*)cmd < CVMglobals.jit.codeCacheEnd) {
	CVMcmdEntryCount(cmd) = 8;
    }
}

static void
preventExecutingMethodsFromDecompiling(CVMExecEnv* ee)
{
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	CVMStack* iStack = &currentEE->interpreterStack;
	/*
	 * We could grab the COMPILEFLAGS microlock while walking all threads,
	 * just while walking any one stack, or just while looking at a frame.
	 * It's a tradeoff between locking out other threads for too long,
	 * and wasting a lot of time locking and unlocking.
	 */
	CVM_COMPILEFLAGS_LOCK(ee);
	CVMstackWalkAllFrames(iStack, {
	    if (CVMframeIsCompiled(frame)) {
		preventMethodFromDecompiling(frame->mb);
	    }
	});
	if (currentEE->invokeMb != NULL) {
	    preventMethodFromDecompiling(currentEE->invokeMb);
	}
	CVM_COMPILEFLAGS_UNLOCK(ee);
    });
}

static CVMUint8 *
decompileAndFreeCbuf(CVMExecEnv *ee, CVMJITGlobalState* jgs,
                     CVMUint8* cbuf, CVMCompiledMethodDescriptor* cmd)
{
    CVMUint8* prevBuf = NULL;
    CVMMethodBlock* mb = CVMcmdMb(cmd);

    if (cbuf != jgs->codeCacheStart) {
        prevBuf = CVMJITcbufPrevBuf(cbuf);
    }

    CVMJITdecompileMethod(ee, mb);

    /* If the previous buffer was free, then this decompilation must coalesced
       with the previous. */
    if ((prevBuf != NULL) && CVMJITcbufIsFree(prevBuf)) {
        cbuf = prevBuf;
    }

    return cbuf;
}

void
CVMJITcodeCacheAgeEntryCountsAndDecompile(CVMExecEnv* ee,
					  CVMUint32 bytesRequested)
{
    CVMBool needDecompile;
    CVMJITGlobalState* jgs = &CVMglobals.jit;

    if (jgs->destroyed) {
	return;
    }
    if (!jgs->policyTriggeredDecompilations) {
	return;
    }

    CVMsysMutexLock(ee, &CVMglobals.jitLock);

    /* 
     * Only decompile if we are above the upper threshold or need more memory
     * for the curent compilation.
     */
    needDecompile = bytesRequested != 0 ||
	jgs->codeCacheBytesAllocated > jgs->upperCodeCacheThreshold;

    /*
     * If we need to decompile any methods, then we must prevent
     * any method that is executing from decompiling. In order to do this
     * we set the entryCount of any executing method to at least 2, and
     * also make all threads become gcSafe so no new methods are invoked
     * while we are scanning the code cache.
     */
    if (needDecompile) {
	/* 
	 * All threads must be gcSafe when we do this and stay gcSafe
	 * until after we are done decompiling.
	 */
	CVMsysMutexLock(ee, &CVMglobals.threadLock);
	CVMD_gcBecomeSafeAll(ee);
	preventExecutingMethodsFromDecompiling(ee);
    }

    /*
     * Age all compiled methods and give some a chance to decompile.
     * Decompile if the following are all true:
     * 1. decompilation was initially required because we were above 
     *    upperCodeCacheThreshold or there is not a free buffer large enough
     *    to meet the current request.
     * 2. the entryCount has reached 0
     * 3. we are still above the lowerCodeCacheThreshold or we have not
     *    yet found a large enough free block to meet the reqeust.
     *
     * In order to avoid decompiling a lot of methods in order to hopefully
     * find a large enough buffer to meet the request, we only decompile
     * methods if they are guaranteed to become part of a larger free
     * buffer that will meet the request. To do this, we don't decompile a
     * method just because its entryCount has reached 0. Instead we keep
     * track of the first buffer in the sequence of buffers that have
     * an entryCount of 0 or are already free. When this sequence becomes
     * big enough to meet the request, then all buffers in the sequence
     * are decompiled. If any non-freeable buffer interrupts the sequence,
     * then we start over and none of the buffers in the sequence end up
     * getting decompiled.
     */
    {
	CVMUint8* firstBufferInSequence = NULL;
	CVMUint32 sequenceSize = 0;
	CVMUint8* cbuf = jgs->codeCacheNextDecompileScanStart;
	CVMUint8* startCbuf = cbuf;

        /* NOTE: 

           CodeCache start: ,-----------------------,
                            |  Second Half Scanned  |
                            |-----------------------|
                            |  First Half Scanned   |
                            `-----------------------'
           CodeCache end:

           We could start scanning in the middle of the codeCache.  Hence,
           there are 2 halfs to the scan.  The first half goes from the
           middle to the end of the code cache.  If we reach the end of the
           code cache and still can't satisfy our space requirements, then
           we'll switch state to the second half scan and scan from the top
           of the code cache.

           Note that the second half scan is allowed to overlap the top part
           of the first half.  This is because the needed memory may be in a
           sequence that spans from the middle of the second half into the
           middle of the first half.

           Note that if codeCacheNextDecompileScanStart is already pointing
           to codeCacheDecompileStart, then we can skip the first half scan
           because it is empty.
	*/           
        enum {
            STATE_FIRST_HALF = 0,
            STATE_SECOND_HALF,
            STATE_DONE,
        };
        int state = STATE_FIRST_HALF;

        if (cbuf >= jgs->codeCacheEnd) {
	    CVMassert(cbuf == jgs->codeCacheEnd);
	    CVMassert(cbuf == jgs->codeCacheDecompileStart);
            /* No decompilable space at all: */
            state = STATE_DONE;
	} else if (cbuf == jgs->codeCacheDecompileStart) {
            /* First half is empty.  Just move on to the second half. */
            state = STATE_SECOND_HALF;
            /* Adjust the startCbuf to point to the end because that's
               effectively where the first half started and ended: */
	    startCbuf = jgs->codeCacheEnd;
	}

        while (state < STATE_DONE) {
	    CVMUint32 bufSize = CVMJITcbufSize(cbuf);

	    /* 
	     * Age the method and decompile it if only if we still need to
	     * get below the lowerCodeCacheThreshold.  For the purpose of
             * getting below the lowerCodeCacheThreshold, we won't be only
             * looking for large contiguous blocks to free.  Any method that
             * has an entryCount of 0 is fair game at this point.
	     */
	    if (CVMJITcbufIsCommitted(cbuf)) {
		CVMCompiledMethodDescriptor* cmd = cbuf2cmd(cbuf);
		CVMcmdEntryCount(cmd) = CVMcmdEntryCount(cmd) >> 1;
		if (needDecompile && (CVMcmdEntryCount(cmd) == 0) && 
		    (jgs->codeCacheBytesAllocated >
		     jgs->lowerCodeCacheThreshold))
                {
		    /* we are above the lower threshold so decompile */
                    cbuf = decompileAndFreeCbuf(ee, jgs, cbuf, cmd);
                    bufSize = CVMJITcbufSize(cbuf);
		}
	    }
	    
	    /*
	     * If we don't have a big enough buffer yet to meet the request,
	     * then see if we can use this buffer to help build a free one 
	     * that is big enough.
	     */
	    if (jgs->codeCacheLargestFreeBuffer < bytesRequested) {
		/* the buffer must be free or freeable */
		if (!CVMJITcbufIsFree(cbuf) &&
		    !(CVMJITcbufIsCommitted(cbuf) &&
		      CVMcmdEntryCount(cbuf2cmd(cbuf)) == 0))
		{
		    /* the sequence has been broken. restart */
		    firstBufferInSequence = NULL;
		    sequenceSize = 0;

                    /* If we're doing the second half scan now, and we've
                       already scanned past the end of the second half (i.e.
                       the start of the first half), then there is no
                       point in continuing to scan.  Advance the state to
                       DONE and abort. */
		    if ((state > STATE_FIRST_HALF) && (cbuf >= startCbuf)) {
                        state = STATE_DONE; /* Advance to DONE. */
                        break;
		    }

		} else {
		    if (firstBufferInSequence == NULL) {
			/* keep track of sequence of available buffers */
			firstBufferInSequence = cbuf;
		    }
		    /* Track of size of sequence of available buffers:
                       NOTE: If the current buffer is still the first buffer
                       in the sequence, then reset the sequence size because
                       the cbuf may have coalesced with adjacent buffers and
                       have grown in size.
                           The case where the current buffer is not the same
                       as the first buffer is if the current is freeable but
                       not freed above because the lowerCodeCacheThreshold
                       requirement has been met.  This means that there won't
                       be anymore coalescing due to freeing because only the
                       sequence driven decompilation below will free code
                       buffers now.
                           The only other case where the current buffer will
                       not be same as the first buffer is when the current
                       buffer is not freeable nor freed.  In that case, the
                       sequence would have been reset and started over.
                           Hence, the only case where we have to handle
                       coalescing of freed buffers is when the current buffer
                       equals the first buffer.  In the other case, we can
                       assume no coalescing and simply add the current buffer
                       size to the sequence size.
                    */
                    if (firstBufferInSequence == cbuf) {
		        sequenceSize = bufSize;
                    } else {
                        sequenceSize += bufSize;
		    }
		    /* See if the current sequence is large enough */
		    if (sequenceSize >= bytesRequested) {
			/* free the sequence */
			cbuf = firstBufferInSequence;
			while (jgs->codeCacheLargestFreeBuffer < 
			       bytesRequested)
                        {
			    CVMUint32 bufSize = CVMJITcbufSize(cbuf);
			    if (!CVMJITcbufIsFree(cbuf)) {
				CVMCompiledMethodDescriptor* cmd =
				    cbuf2cmd(cbuf);
                                cbuf =
                                    decompileAndFreeCbuf(ee, jgs, cbuf, cmd);
                                bufSize = CVMJITcbufSize(cbuf);
			    }
			    cbuf += bufSize;
			}
#if 1
			/*
			 * Keep track of the lowest code cache usage after
			 * we had to make room for a method. The hope is that
			 * this remains close to lowerCodeCacheThreshold.
			 *
			 * NOTE: This is simply a measurement and could be
			 * removed from optimized builds, but takes very
			 * little space and time.
			 */
			{
			    if (jgs->codeCacheLowestForcedUtilization >
				jgs->codeCacheBytesAllocated + bytesRequested)
			    {
				jgs->codeCacheLowestForcedUtilization =
				    jgs->codeCacheBytesAllocated +
				    bytesRequested;
			    }
			}
#endif
			/* If we're at the end of the code cache, then
			   resume at the start of the code cache. */
			if (cbuf >= jgs->codeCacheEnd) {
			    CVMassert(cbuf == jgs->codeCacheEnd);
			    cbuf = jgs->codeCacheDecompileStart;
			    state++; /* Advance to second half or done. */
			}
			continue; /* continue with current buffer */
		    }
		}
	    }
	    cbuf += bufSize;

            /* If we're at the end of the code cache, continue at the start of
               the decompilable region: */
            if (cbuf >= jgs->codeCacheEnd) {
                CVMassert(cbuf == jgs->codeCacheEnd);
                cbuf = jgs->codeCacheDecompileStart;
                /* the sequence has been broken. restart */
                firstBufferInSequence = NULL;
                sequenceSize = 0;
                state++; /* Advance to second half or done. */
	    }
	}  /* while (state < STATE_DONE) */
    }

    /*
     * Allow threads to execute unsafe again.
     */
    if (needDecompile) {
	CVMD_gcAllowUnsafeAll(ee);
	CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    }

    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
}

/**************************************
 * Functions for managing free buffers.
 **************************************/

/*
 * Add a buffer to the free list.
 */
static void
CVMJITcodeCacheAddBufToFreeList(CVMUint8* cbuf)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMJITFreeBuf* freebuf = (CVMJITFreeBuf*)cbuf;
    CVMassert(CVMJITcbufIsFree(cbuf));
    if (jgs->codeCacheFirstFreeBuf != NULL) {
	jgs->codeCacheFirstFreeBuf->prev = freebuf;
    }
    freebuf->next = jgs->codeCacheFirstFreeBuf;
    freebuf->prev = NULL;
    jgs->codeCacheFirstFreeBuf = freebuf;
    if (CVMJITcbufSize(cbuf) > jgs->codeCacheLargestFreeBuffer) {
	jgs->codeCacheLargestFreeBuffer = CVMJITcbufSize(cbuf);
    }
}

/*
 * Remove a buffer from the free list.
 */
static void
CVMJITcodeCacheRemoveBufFromFreeList(CVMUint8* cbuf)
{
    CVMJITFreeBuf* freebuf = (CVMJITFreeBuf*)cbuf;
    CVMassert(CVMJITcbufIsFree(cbuf));
    if (freebuf->next != NULL) {
	freebuf->next->prev = freebuf->prev;
    }
    if (freebuf->prev != NULL) {
	freebuf->prev->next = freebuf->next;
    } else {
	CVMglobals.jit.codeCacheFirstFreeBuf = freebuf->next;
    }
}

/*
 * Return a free buffer that is of at least bufSizeEstimate bytes in size.
 * If remove is true, then the buffer is removed from the free list.
 */
static CVMUint8*
CVMJITcodeCacheFindFreeBuffer(CVMJITCompilationContext* con,
			      CVMUint32 bufSizeEstimate,
			      CVMBool remove)
{
    CVMJITFreeBuf* freebuf;
    CVMUint8* cbuf;

    /* Need to guard against decompiling doing updates */
    CVMassert(CVMsysMutexIAmOwner(con->ee, &CVMglobals.jitLock));
    freebuf = CVMglobals.jit.codeCacheFirstFreeBuf;
    cbuf = (CVMUint8*)freebuf;
    while (freebuf != NULL) {
	if (CVMJITcbufSize(cbuf) >= bufSizeEstimate) {
	    if (remove) {
		/* remove this buffer from this free list */
		CVMJITcodeCacheRemoveBufFromFreeList(cbuf);
		/* clear the free flag from the size words */
		CVMJITcbufSetUncommitedBufSize(cbuf, CVMJITcbufSize(cbuf));
	    }
	    break;
	}
	freebuf = freebuf->next;
	cbuf = (CVMUint8*)freebuf;
    }

    return cbuf;
}

/*
 * Try to make sure there is enough memory for the method.
 */
static void
CVMJITcodeCacheMakeRoomForMethod(CVMJITCompilationContext* con,
				 CVMUint32 bufSize)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMBool success = CVM_FALSE;

    if (!jgs->policyTriggeredDecompilations) {
	CVMtraceJITStatus(("JS: Code cache full and decompilation "
			   "is disabled.\n"));
	goto done;
    }
    
    CVMtraceJITStatus(("JS: Code cache full. Aging...\n"));
    
    jgs->codeCacheLargestFreeBuffer = 0;
    
    /* 
     * Age all compiled methods and give some a chance to decompile
     */
    /*CVMconsolePrintf("Aging to make room...%d\n", bufSize);*/
    CVMJITcodeCacheAgeEntryCountsAndDecompile(con->ee, bufSize);
    /*CVMconsolePrintf("Done aging to make room...%d\n", bufSize);*/

    if (jgs->codeCacheLargestFreeBuffer < bufSize) {
        /* The purpose of a GC is so that unused classes can be freed, and its
           corresponding compile methods can be decompiled.  However, doing a
           GC seldom yields free classes.  Hence, this action consumes a lot
           of execution time while rarely yielding results.  Disable it for
           now. */
#if 0
	/*
	 * If we still don't have room in the cache, then force a full
	 * gc. This may cause some methods to decompile because of
	 * class unloading, and will also age all methods a 2nd time.
	 */
	CVMtraceJITStatus(("JS: Code cache full. Forcing gc.\n"));
	
	/*CVMconsolePrintf("Doing gc to make room...%d\n", bufSize);*/
	CVMgcRunGC(con->ee);
	/*CVMconsolePrintf("Done doing gc to make room...%d\n", bufSize);*/
	
	if (jgs->codeCacheLargestFreeBuffer < bufSize) {
	    CVMtraceJITStatus(("JS: Code cache full. Giving up.\n"));
	    goto done; 	/* give up */
	}
#else
	goto done; /* give up */
#endif
    }
    success = CVM_TRUE;

 done:
    jgs->codeCacheLargestFreeBuffer = 0;
    if (!success) {
	jgs->codeCacheFailedAllocations++;
	CVMJITerror(con, OUT_OF_MEMORY, "Code Cache is full");
    }
    return;
}

#ifdef CVM_JIT_PROFILE

static void
dumpEntry(void* data, int methodSampleCount, CVMBool isccm,
	  CVMUint32 totalSampleCount, float percentOfTimeInCodeCache)
{
    char buf[256];
    float samplePercentOfCodeCacheTime =
	100 * ((float)methodSampleCount / (float)totalSampleCount);
    float samplePercentOfTotalTime =
	samplePercentOfCodeCacheTime * percentOfTimeInCodeCache;
    
    if (isccm) {
	/* samples were in ccm copy part of the code cache */
	CVMformatString(buf, sizeof(buf),
			"        %5.2f%% %5.2f%% %s\n",
			samplePercentOfTotalTime,
			samplePercentOfCodeCacheTime,
			(char*)data);
    } else {
	if (data == NULL) {
	    /* samples were in a now free part of the code cache */
	    CVMformatString(buf, sizeof(buf),
			    "        %5.2f%% %5.2f%% free_buffer\n",
			    samplePercentOfTotalTime,
			    samplePercentOfCodeCacheTime);
	} else {
	    /* samples were in a compiled method */
	    CVMCompiledMethodDescriptor* cmd =
		(CVMCompiledMethodDescriptor*)data;
	    CVMMethodBlock* mb = CVMcmdMb(cmd);
	    /* size of the compiled code */
	    float codeSize = CVMcmdCodeSize(cmd);
	    /* estimate of % of time saved if method was inlined */
	    float inlineSavingsEstimate = 
		samplePercentOfTotalTime *
		(CVMCPU_NUM_METHOD_INVOCATION_INSTRUCTIONS /
		 (codeSize / sizeof(CVMUint32)));

	    if ((CVMmbCompileFlags(mb) & CVMJIT_HAS_BACKBRANCH) != 0) {
		inlineSavingsEstimate = 0; /* ignore methods with loops */
	    }
	    
	    CVMformatString(buf, sizeof(buf),
			    "%c%5.2f%% %5.2f%% %5.2f%% %C.%M\n",
			    CVMmbIs(mb, SYNCHRONIZED) ? '*' : ' ',
			    inlineSavingsEstimate, 
			    samplePercentOfTotalTime,
			    samplePercentOfCodeCacheTime,
			    CVMmbClassBlock(mb), mb);
	    /* ignore savings less than .25% if not synchronized */
	    if (!CVMmbIs(mb, SYNCHRONIZED) && inlineSavingsEstimate < 0.25) {
		strncpy(buf, "              ", 8);
	    }
	    buf[sizeof(buf)-2] = '\n';
	    buf[sizeof(buf)-1] = '\0';
	}
    }
    CVMioWrite(CVMglobals.jit.profile_fd, buf, strlen(buf));
}

void
CVMJITcodeCacheDumpProfileData()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint8*  curBuf = jgs->codeCacheStart;
    CVMUint8*  nextBuf = CVMJITcbufNextBuf(curBuf);
    CVMUint8*  codePtr = curBuf;
    CVMUint32  methodSampleCount = 0;
    CVMUint32  totalSampleCount = 0;
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    CVMUint32  currHelper = 0;
#endif
    CVMInt32   totalTime; /* total program time while profiling */
    float      percentOfTimeInCodeCache; /* % of time in code cache */
    char buf[256];

    /* for keeping track of top 10 hottest methods */
    struct {
	void*      data;
	CVMUint32  samples;
	CVMBool    isccm;
    } top10[10];
    void*      data;
    CVMBool    isccm;

    if (jgs->profileBuf == NULL) {
	return;
    }

    memset(top10, 0, sizeof(top10));

    /* precount the number of samples taken */
    while (codePtr < jgs->codeCacheEnd) {
	totalSampleCount += CVMJITprofileGetSampleCount(codePtr);
	codePtr = CVMJITprofileGetNextProfiledRegion(codePtr);
    }
    if (totalSampleCount == 0) {
	return;
    }


    totalTime =
	CVMlong2Int(CVMlongSub(CVMtimeMillis(), jgs->profileStartTime));
    percentOfTimeInCodeCache =
	((float)totalSampleCount * (1000/CVMJIT_PROFILE_SAMPLES_PER_SECOND)) /
	(float)totalTime;
    CVMformatString(buf, sizeof(buf),
		    "totalSampleCount = %d = %5.2f%% "
		    "of program execution time\n\n",
		    totalSampleCount, percentOfTimeInCodeCache * 100);
    CVMioWrite(jgs->profile_fd, buf, strlen(buf));
    CVMformatString(buf, sizeof(buf),
		    "'*' indicates method is synchronized.\n"
		    "column #1: estimated savings if method is inlined "
		    "(empty if < .25%% and not synchronized)\n"
		    "column #2: %% of program time spent in method\n"
		    "column #3: %% of code cache time spent in method\n\n");
    CVMioWrite(jgs->profile_fd, buf, strlen(buf));
    CVMformatString(buf, sizeof(buf),
		    "*  <1>    <2>    <3>\n"
		    "------------------------\n");
    CVMioWrite(jgs->profile_fd, buf, strlen(buf));

    /* find out how many samples taken in each part of the code cache */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (jgs->profileInstructions) {
	CVMformatString(buf, sizeof(buf), "%s\n",
			jgs->ccmCodeCacheCopyEntries[0].helperName);
	CVMioWrite(CVMglobals.jit.profile_fd, buf, strlen(buf));
    }
#endif
    codePtr = curBuf;
    while (codePtr < jgs->codeCacheEnd) {
	int sampleCount = CVMJITprofileGetSampleCount(codePtr);

	/* look at the current sample */
	methodSampleCount += sampleCount;
	if (sampleCount > 0 && jgs->profileInstructions) {
	    CVMUint32 offset;
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	    if (curBuf == jgs->codeCacheStart) {
		offset =
		    codePtr - jgs->ccmCodeCacheCopyEntries[currHelper].helper;
	    } else
#endif
	    {
		CVMCompiledMethodDescriptor* cmd = cbuf2cmd(curBuf);
		offset = codePtr - CVMmbStartPC(CVMcmdMb(cmd));
	    }
	    CVMformatString(buf, sizeof(buf),
			    "(pc: 0x%x)  (offset: %5d)  (samples: %d)\n",
			    codePtr, offset, sampleCount);
	    CVMioWrite(CVMglobals.jit.profile_fd, buf, strlen(buf));
	}

	/* move on to the next sample */
	codePtr = CVMJITprofileGetNextProfiledRegion(codePtr);

	/*
	 * If we reached the end of a method or ccm function,
	 * then dump sample total.
	 */
	if (codePtr >= nextBuf
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	    || (curBuf == jgs->codeCacheStart &&
		codePtr >= jgs->ccmCodeCacheCopyEntries[currHelper+1].helper)
#endif
	    )
	{
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	    if (curBuf == jgs->codeCacheStart) {
		/* samples were in helper part of the code cache */
		isccm = CVM_TRUE;
		data = (void*)
		    jgs->ccmCodeCacheCopyEntries[currHelper].helperName;
		currHelper++;
	    } else 
#endif
	    {
		isccm = CVM_FALSE;
		if (CVMJITcbufIsFree(curBuf)) {
		    /* samples were in a now free part of the code cache */
		    data = NULL;
		} else {
		    /* samples were in a compiled method */
		    data = cbuf2cmd(curBuf);
		}
	    }

	    if (methodSampleCount != 0) {
		/* print the sample information */
		dumpEntry(data, methodSampleCount, isccm,
			  totalSampleCount, percentOfTimeInCodeCache);

		/* update the top 10 list */
		{
		    int i;
		    for (i = 0; i < 10; i++) {
			if (methodSampleCount > top10[i].samples) {
			    int j;
			    for (j = 9; j > i; j--) {
				top10[j] = top10[j-1];
			    }
			    top10[i].data =  data;
			    top10[i].isccm = isccm;
			    top10[i].samples = methodSampleCount;
			    break;
			}
		    }
		}

		methodSampleCount = 0;
	    }
	    
	    /* move on to the next code buffer if necessary */
	    if (codePtr >= nextBuf) {
		curBuf = nextBuf;
		nextBuf = CVMJITcbufNextBuf(curBuf);
	    }

	    /* print name of next method or copied ccm function */
	    if (nextBuf < jgs->codeCacheEnd && jgs->profileInstructions) {
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
		if (curBuf == jgs->codeCacheStart) {
		    const char* helperName = 
			jgs->ccmCodeCacheCopyEntries[currHelper].helperName;
		    if (helperName == NULL) {
			buf[0] = '\0';  /* CVMCCMcodeCacheCopyEnd entry */
		    } else {
			CVMformatString(buf, sizeof(buf), "%s\n", helperName);
		    }
		} else 
#endif
		if (CVMJITcbufIsFree(curBuf)) {
		    CVMformatString(buf, sizeof(buf), "free_buffer\n");
		} else {
		    CVMCompiledMethodDescriptor* cmd = cbuf2cmd(curBuf);
		    CVMMethodBlock* mb = CVMcmdMb(cmd);
		    CVMUint32 codeSize = CVMcmdCodeSize(cmd);
		    CVMformatString(buf, sizeof(buf),
				    "%5d bytes: %C.%M\n",
				    codeSize, CVMmbClassBlock(mb), mb);
		    buf[sizeof(buf)-2] = '\n';
		    buf[sizeof(buf)-1] = '\0';
		}
		CVMioWrite(CVMglobals.jit.profile_fd, buf, strlen(buf));
	    }
	}
    }

    /* dump the top 10 */
    CVMformatString(buf, sizeof(buf), 
		    "\nTop 10 List\n===========\n");
    CVMioWrite(CVMglobals.jit.profile_fd, buf, strlen(buf));
    {
	int i;
	for (i = 0; i < 10; i++) {
	    if (top10[i].samples > 0) {
		dumpEntry(top10[i].data, top10[i].samples, top10[i].isccm,
			  totalSampleCount, percentOfTimeInCodeCache);
	    }
	}
    }
}

#endif

/*
 * Find the method that the specified pc is in.
 */
CVMBool
CVMJITcodeCacheInCompiledMethod(CVMUint8* pc)
{
    return ((pc >= CVMglobals.jit.codeCacheGeneratedCodeStart) && 
	    (pc < CVMglobals.jit.codeCacheEnd))
#ifdef CVM_AOT
           || ((pc >= CVMglobals.jit.codeCacheAOTGeneratedCodeStart) && 
               (pc < CVMglobals.jit.codeCacheAOTEnd))
#endif
            ;
}

CVMBool
CVMJITcodeCacheInCCM(CVMUint8* pc)
{
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    return ((pc >= CVMglobals.jit.codeCacheStart) &&
	    (pc < CVMglobals.jit.codeCacheGeneratedCodeStart))
#ifdef CVM_AOT
           || ((pc >= CVMglobals.jit.codeCacheAOTStart) && 
               (pc < CVMglobals.jit.codeCacheAOTGeneratedCodeStart))
#endif
           ;
#else
    return ((pc >= (CVMUint8*)&CVMCCMcodeCacheCopyStart) &&
	    (pc < (CVMUint8*)&CVMCCMcodeCacheCopyEnd));
#endif
}

#ifdef CVM_USE_MEM_MGR
void
CVMmemRegisterCodeCache()
{
    CVMMemHandle *h;
    CVMAddr end;

    /* To simplify, align the end. */
    end = (CVMAddr)((CVMUint32)CVMglobals.jit.codeCacheDecompileStart & 
                               ~(CVMgetPagesize() - 1));
    /* Register shared codecache */
    CVMconsolePrintf(
        "Register Shared CodeCache(0x%x ~ 0x%x) With Memory Manager ...\n",
        (CVMAddr)CVMglobals.jit.codeCacheStart, end);
    h = CVMmemRegisterWithMetaData(
        (CVMAddr)CVMglobals.jit.codeCacheStart, end,
        CVMglobals.jit.codeCacheMemType, 0);
    CVMmemSetMonitorMode(h, CVM_MEM_MON_ALL_WRITES);
}

/* Notify writes to registered shared code cache. */
void
CVMmemCodeCacheWriteNotify(int pid, void *addr, void *pc, CVMMemHandle *h)
{
    CVMMethodBlock *mb;
#if CVM_DEBUG_CLASSINFO
    CVMUint8* javapc;
#endif
    int lineno;

    mb = CVMJITcodeCacheFindCompiledMethod(addr, CVM_FALSE);

    if (mb != NULL) {
#if CVM_DEBUG_CLASSINFO
        javapc = CVMpcmapCompiledPcToJavaPc(mb, (CVMUint8*)addr);
        if (javapc != NULL) {
            lineno = CVMpc2lineno(mb, javapc - CVMmbJavaCode(mb));
        } else 
#endif
        {
            lineno = -1;
        }

        CVMconsolePrintf(
            "Process #%d (at 0x%x) is writing into 0x%x in compiled %C.%M:%d\n",
            pid, pc, addr, CVMmbClassBlock(mb), mb, lineno);
    }
}
#endif

/**********************
 * Debugging functions
 **********************/

#if defined(CVM_DEBUG) || defined(CVM_USE_MEM_MGR) || defined(CVM_TRACE_JIT)

/*
 * Find the method that the specified pc is in.
 */
CVMMethodBlock*
CVMJITcodeCacheFindCompiledMethod(CVMUint8* pc, CVMBool doPrint)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint8* cbuf     = jgs->codeCacheStart;
    CVMMethodBlock* mb;
    CVMExecEnv* ee     = CVMgetEE();

    /* make sure the pc is in the code cache */
    if (pc < jgs->codeCacheStart || pc >= jgs->codeCacheEnd) {
	if (doPrint) {
	    CVMconsolePrintf("PC 0x%x is not in code cache\n", pc);
	}
	return NULL;
    }

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    /* see if the pc is in the ccm copy part of the code cache */
    if (pc < jgs->codeCacheStart + CVMJITcbufSize(jgs->codeCacheStart)) {
	if (doPrint) {
#ifdef CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES
	    CVMCCMCodeCacheCopyEntry* entries = jgs->ccmCodeCacheCopyEntries;
	    int i = 0;
	    do {
		i++;
		if (entries[i].helper > pc) {
		    CVMconsolePrintf("ccm_copy: %s+%d\n",
				     entries[i-1].helperName,
				     pc - entries[i-1].helper);
		    break;
		}
	    } while (entries[i].helperName != NULL);
#else
	    CVMconsolePrintf("ccm_copy: *** no mapping available\n");
#endif
	}
	return NULL;
    }
#endif

    /* Need to guard against decompiling doing updates */
    if (CVMD_isgcSafe(ee)) {
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
    } else {
	CVMD_gcSafeExec(ee, {
	    CVMsysMutexLock(ee, &CVMglobals.jitLock);
	});
    }

    /* iterate over the code cache */
    while (cbuf < jgs->codeCacheEnd) {
	CVMUint32 bufSize = CVMJITcbufSize(cbuf);
	if (CVMJITcbufIsCommitted(cbuf)) {
	    CVMUint8* startpc = cbuf;
	    CVMUint8* endpc   = startpc + bufSize;
	    mb = CVMcmdMb(cbuf2cmd(cbuf));
	    if (pc >= startpc && pc < endpc) {
		char buf[256];
		CVMpc2string(pc, mb, CVM_FALSE, CVM_TRUE, 
			     buf, buf + sizeof(buf));
		if (doPrint) {
		    CVMconsolePrintf("offset(%d) %s\n", 
				     pc - CVMmbStartPC(mb), buf);
		}
		goto done_searching;
	    }
	}
	cbuf += bufSize;
    }

    mb = NULL;  /* not found */

 done_searching:
    /* release the jit lock */
    if (CVMD_isgcSafe(ee)) {
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
    } else {
	CVMD_gcSafeExec(ee, {
	    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
	});
    }

    if (mb == NULL && doPrint) {
	CVMconsolePrintf("Method Not Found\n");
    }
    return mb;
}
#endif

#ifdef CVM_DEBUG
/*
 * Dump all free and allocated blocks in the code cache and also do a bit
 * of sanity checking.
 */
CVMBool
CVMJITcodeCacheVerify(CVMBool doDump)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMUint8* cbuf;
    CVMUint32 freeTotal = 0;
    CVMUint32 allocatedTotal = 0;
    CVMUint32 uncommittedTotal = 0;
    CVMUint32 largestFreeBlock = 0;
    CVMBool   previousWasFree = CVM_FALSE;
    CVMBool   result = CVM_TRUE;
    CVMExecEnv* ee     = CVMgetEE();

    /* Need to guard against decompiling doing updates */
    if (CVMD_isgcSafe(ee)) {
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
    } else {
	CVMD_gcSafeExec(ee, {
	    CVMsysMutexLock(ee, &CVMglobals.jitLock);
	});
    }

    /*
     * Dump the code cache and total the number of free and allocated buffers.
     */
    cbuf = jgs->codeCacheStart;
    while (cbuf < jgs->codeCacheEnd) {
	CVMUint32 bufSize = CVMJITcbufSize(cbuf);
	char status;
	CVMMethodBlock* mb = NULL;
	if (CVMJITcbufIsFree(cbuf)) {
	    status = 'F';
	    freeTotal += bufSize;
	    if (previousWasFree) {
		CVMconsolePrintf("WARNING: two consecutive free buffers\n");
		result = CVM_FALSE;
	    }
	    previousWasFree = CVM_TRUE;
	    if (bufSize > largestFreeBlock) {
		largestFreeBlock = bufSize;
	    }
	} else if (CVMJITcbufIsCommitted(cbuf)) {
	    CVMCompiledMethodDescriptor* cmd = cbuf2cmd(cbuf);
	    status = 'A';
	    allocatedTotal += bufSize;
	    mb = CVMcmdMb(cmd);
	    previousWasFree = CVM_FALSE;
	} else if (CVMJITcbufIsCCMCopy(cbuf)) {
	    status = 'G';
	    allocatedTotal += bufSize;
	    previousWasFree = CVM_FALSE;
	} else {
	    status = 'U';
	    allocatedTotal += bufSize;
	    uncommittedTotal += bufSize;
	    previousWasFree = CVM_FALSE;
	}
	if (doDump) {
	    CVMconsolePrintf("%c 0x%x size=%-9d", status, cbuf, bufSize);
	    if (mb != NULL) {
		CVMconsolePrintf(" %C.%M\n", CVMmbClassBlock(mb), mb);
	    } else {
		CVMconsolePrintf("\n");
	    }
	}
	cbuf += bufSize;
    }

    if (doDump) {
	CVMconsolePrintf("Allocated(0x%x) Free(0x%x) Largest(0x%x) "
			 "Total(0x%x)\n",
			 allocatedTotal , freeTotal, largestFreeBlock,
			 allocatedTotal + freeTotal);
    }
    if (allocatedTotal + freeTotal != jgs->codeCacheEnd - jgs->codeCacheStart){
	CVMconsolePrintf("WARNING: allocatedTotal + freeTotal != "
			 "codeCacheEnd - codeCacheStart\n");
	result = CVM_FALSE;
    }
    if (allocatedTotal - uncommittedTotal !=
	jgs->codeCacheBytesAllocated) {
	CVMconsolePrintf("WARNING: allocatedTotal != "
			 "codeCacheBytesAllocated\n");
	result = CVM_FALSE;
    }

    /*
     * Walk the free list and see how many bytes are free.
     */
    {
	CVMJITFreeBuf* freebuf = jgs->codeCacheFirstFreeBuf;
	CVMUint32 freeListBytes = 0;
	
	while (freebuf != NULL) {
	    freeListBytes += CVMJITcbufSize((CVMUint8*)freebuf);
	    freebuf = freebuf->next;
	}
	if (freeTotal != freeListBytes) {
	    CVMconsolePrintf("WARNING: freeTotal != freeListBytes\n");
	    result = CVM_FALSE;
	}
	if (freeTotal + uncommittedTotal != 
	    jgs->codeCacheSize -
	    jgs->codeCacheBytesAllocated)
	{
	    CVMconsolePrintf("WARNING: freeTotal not in sync\n");
	    result = CVM_FALSE;
	}
    }

    /* release the jitLock */
    if (CVMD_isgcSafe(ee)) {
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
    } else {
	CVMD_gcSafeExec(ee, {
	    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
	});
    }

    return result;
}
#endif /* CVM_DEBUG */
