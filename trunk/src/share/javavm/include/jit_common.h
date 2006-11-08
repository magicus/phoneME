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

/*
 * This file includes functions common to all JITs. It is the interface
 * between a particular JIT implementation and the rest of the VM.
 */

#ifndef _INCLUDED_JIT_COMMON_H
#define _INCLUDED_JIT_COMMON_H

#ifdef CVM_JIT

#include "javavm/include/defs.h"
#include "javavm/include/utils.h"
#include "javavm/include/stacks.h"	/* CVMFrameFlags */
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"

#ifdef CVM_USE_MEM_MGR
#include "javavm/include/mem_mgr.h"
#endif

CVMFrameGCScannerFunc CVMcompiledFrameScanner;

typedef struct {
    CVMFrame *frame;
    CVMMethodBlock *mb;
    CVMCompiledInliningInfoEntry *inliningEntries;
    CVMInt32 pcOffset;
    CVMInt32 index;
    CVMInt32 numEntries;
} CVMJITFrameIterator;

/*
 * Given a compiled frame, this function re-constructs the back-trace
 * for the PC in the frame. Due to inlining, a PC can correspond to 
 * multiple mb's.
 */
extern void
CVMJITframeIterate(CVMFrame* frame, CVMJITFrameIterator* iter);

extern CVMBool
CVMJITframeIterateSkip(CVMJITFrameIterator* iter, int skip,
    CVMBool skipArtificial, CVMBool popFrame);

#define CVMJITframeIterateNext(iter)	\
    CVMJITframeIterateSkip((iter), 0, CVM_TRUE, CVM_FALSE)

#define CVMJITframeIteratePop(iter)	\
    CVMJITframeIterateSkip((iter), 0, CVM_TRUE, CVM_TRUE)

extern CVMUint32
CVMJITframeIterateCount(CVMJITFrameIterator* iter, CVMBool skipArtificial);

extern CVMFrame *
CVMJITframeIterateGetFrame(CVMJITFrameIterator *iter);

extern CVMMethodBlock *
CVMJITframeIterateGetMb(CVMJITFrameIterator *iter);

extern CVMUint8 *
CVMJITframeIterateGetJavaPc(CVMJITFrameIterator *iter);

extern void
CVMJITframeIterateSetJavaPc(CVMJITFrameIterator *iter, CVMUint8 *pc);


extern CVMStackVal32 *
CVMJITframeIterateGetLocals(CVMJITFrameIterator *iter);

extern CVMObjectICell *
CVMJITframeIterateSyncObject(CVMJITFrameIterator *iter);

extern CVMFrameFlags
CVMJITframeIterateGetFlags(CVMJITFrameIterator *iter);

extern CVMBool
CVMJITframeIterateIsInlined(CVMJITFrameIterator *iter);

extern CVMBool
CVMJITframeIterateHandlesExceptions(CVMJITFrameIterator *iter);

extern void
CVMJITframeIterateSetFlags(CVMJITFrameIterator *iter, CVMFrameFlags flags);

extern CVMMethodBlock *
CVMJITframeGetMb(CVMFrame *frame);

extern CVMMethodBlock *
CVMJITeeGetCurrentFrameMb(CVMExecEnv *ee);

#define CVMinvokeCompiled(ee, jfp)			\
    CVMJITgoNative(NULL, ee, jfp, (jfp)->pcX)

#define CVMreturnToCompiled(ee, jfp, exceptionObject)	\
    CVMJITgoNative(exceptionObject, ee, jfp, (jfp)->pcX)

/* Result codes for compiled native methods. */

typedef enum {
    CVM_COMPILED_RETURN,
    CVM_COMPILED_NEW_MB,
    CVM_COMPILED_NEW_TRANSITION,
    CVM_COMPILED_EXCEPTION
} CVMCompiledResultCode;

extern CVMCompiledResultCode
CVMinvokeCompiledHelper(CVMExecEnv *ee, CVMFrame *frame, CVMMethodBlock **mb);

extern CVMCompiledResultCode
CVMreturnToCompiledHelper(CVMExecEnv *ee, CVMFrame *frame,
			  CVMMethodBlock **mb, CVMObject* exceptionObject);

/* Purpose: Do on-stack replacement of an interpreted stack frame with a
            compiled stack frame and continue executing the method using its
            compiled form at the location indicated by the specified bytecode
            PC. */
extern CVMCompiledResultCode
CVMinvokeOSRCompiledHelper(CVMExecEnv *ee, CVMFrame *frame,
                           CVMMethodBlock **mb, CVMUint8 *pc);

#define CVM_JIT_OPTIONS "[-Xjit:[<option>[,<option>]...]] "

/* Order must match jitWhenToCompileOptions table */
typedef enum {
    CVMJIT_COMPILE_NONE,
    CVMJIT_COMPILE_ALL,
    CVMJIT_COMPILE_POLICY,
    /* NOTE: CVMJIT_COMPILE_NUM_OPTIONS must be the last entry in this list: */
    CVMJIT_COMPILE_NUM_OPTIONS
} CVMJITWhenToCompileOption;

/* Order must match jitWhatToInlineOptions table */
typedef enum {
    CVMJIT_INLINE_VIRTUAL,
    CVMJIT_INLINE_NONVIRTUAL,
    CVMJIT_INLINE_VIRTUAL_SYNC,
    CVMJIT_INLINE_NONVIRTUAL_SYNC,
    CVMJIT_INLINE_DOPRIVILEGED,
    CVMJIT_INLINE_USE_VIRTUAL_HINTS,
    CVMJIT_INLINE_USE_INTERFACE_HINTS,

    /* NOTE: CVMJIT_INLINE_NUM_OPTIONS must be the last entry in this list: */
    CVMJIT_INLINE_NUM_OPTIONS
} CVMJITWhatToInlineOption;

#define CVMJITinlines(inlineType_) \
    ((CVMglobals.jit.whatToInline & (1<<CVMJIT_INLINE_##inlineType_)) != 0)

/*
 * The default values of our command-line options
 */
#define CVMJIT_DEFAULT_ICOST        20
#define CVMJIT_DEFAULT_MCOST        50
#define CVMJIT_DEFAULT_BCOST        4
#define CVMJIT_DEFAULT_CLIMIT       20000
#define CVMJIT_DEFAULT_POLICY       CVMJIT_COMPILE_POLICY
#define CVMJIT_DEFAULT_INLINING     \
        ((1 << CVMJIT_INLINE_VIRTUAL) | \
         (1 << CVMJIT_INLINE_NONVIRTUAL) | \
         (1 << CVMJIT_INLINE_USE_VIRTUAL_HINTS) | \
         (1 << CVMJIT_INLINE_USE_INTERFACE_HINTS))

#define CVMJIT_DEFAULT_MAX_INLINE_DEPTH     12
#define CVMJIT_DEFAULT_MAX_INLINE_CODELEN   68
#define CVMJIT_DEFAULT_MIN_INLINE_CODELEN   16
#define CVMJIT_DEFAULT_MAX_WORKING_MEM      550*1024
#define CVMJIT_DEFAULT_MAX_COMP_METH_SIZE   (64*1024 - 1)
#define CVMJIT_DEFAULT_CODE_CACHE_SIZE      512*1024
#define CVMJIT_DEFAULT_UPPER_CCACHE_THR     95
/* NOTE: the default of -1 for lower code cache threshold is
   significant. See CVMJITcodeCacheInit */
#define CVMJIT_DEFAULT_LOWER_CCACHE_THR     -1

#ifdef CVM_JIT_COLLECT_STATS

/* Order must match jitStatsToCollect table */
typedef enum {
    CVMJIT_STATS_HELP,
    CVMJIT_STATS_COLLECT_NONE,
    CVMJIT_STATS_COLLECT_MINIMAL,
    CVMJIT_STATS_COLLECT_MORE,
    CVMJIT_STATS_COLLECT_VERBOSE,
    CVMJIT_STATS_COLLECT_CONSTANTS,
    CVMJIT_STATS_COLLECT_MAXIMAL,
    /* NOTE: CVMJIT_STATS_NUM_OPTIONS must be the last entry in this list.
       The only exception is CVMJIT_STATS_ABORTED which is used for error
       tracking. */
    CVMJIT_STATS_NUM_OPTIONS,
    CVMJIT_STATS_ABORTED

} CVMJITStatsToCollectOption;

#endif /* CVM_JIT_COLLECT_STATS */

/*
 * Used to keep track of patch points in ccm code required for gc.
 */
typedef struct CVMCCMGCPatchPoint {
    CVMUint8* patchPoint; /* instruction to patch */
    CVMCPUInstruction  patchInstruction; /* instr that does gc rendezvous */
} CVMCCMGCPatchPoint;

/*********************************************************************
 * Support for copying ccm assembler code into the start of the code cache 
 * for faster access and less code generation on some platforms.
 *********************************************************************/

/*
 * We want the table of ccm function mappings (from code cache address to
 * function name) if we are copying ccm code to the code cache, and either
 * debug or profiling is enabled.
 */
#if (defined(CVM_DEBUG) || defined(CVM_JIT_PROFILE) || defined(CVM_JVMPI)) && \
    defined(CVM_JIT_COPY_CCMCODE_TO_CODECACHE)
#define CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES
/*
 * used to keep track of ccm entry points that we want to profile separately.
 */
typedef struct CVMCCMCodeCacheCopyEntry {
    CVMUint8* helper;
    const char* helperName;
} CVMCCMCodeCacheCopyEntry;
#endif

/*
 * Return the offset from start of code buffer of the ccm function,
 * which is located in the beginning of the code cache.
 */
#if defined(CVM_JIT_COPY_CCMCODE_TO_CODECACHE)
#define CVMCCMcodeCacheCopyHelperOffset(con, helper)   	\
    (CVMglobals.jit.ccmCodeCacheCopyAddress - con->codeEntry +	\
     (CVMUint8*)helper - (CVMUint8*)&CVMCCMcodeCacheCopyStart)

#endif /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */

/*
 * CVMJIT_CCMCODE_ADDR - returns the address of a ccm entry point.
 * This address will be in the code cache if we copied the ccm assembler
 * code to the code cache. Otherwise, it is just the linker symbol.
 */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define CVMJIT_CCMCODE_ADDR(ccmAddr)			       	\
    (CVMUint8*)(CVMglobals.jit.ccmCodeCacheCopyAddress + 		\
		 ((CVMUint8*)(ccmAddr) - (CVMUint8*)&CVMCCMcodeCacheCopyStart))
#else
#define CVMJIT_CCMCODE_ADDR(ccmAddr)  ((CVMUint8*)(ccmAddr))
#endif

/* The following are used to determine the size of the cache of hints for
   virtual and interface inlining.
   NOTE: CVM_MAX_INVOKE_VIRTUAL_HINTS and CVM_MAX_INVOKE_INTERFACE_HINTS must
         be a number equal to some power of 2.
*/
#define CVM_MAX_INVOKE_VIRTUAL_HINTS    128
#define CVM_MAX_INVOKE_INTERFACE_HINTS  64
#define CVM_VHINT_MASK                  (CVM_MAX_INVOKE_VIRTUAL_HINTS - 1)
#define CVM_IHINT_MASK                  (CVM_MAX_INVOKE_INTERFACE_HINTS - 1)

/*********************************************************************
 * CVMJITGlobalState - where all the jit globals go.
 *********************************************************************/

typedef struct {
    /* command line options */
    CVMParsedSubOptions parsedSubOptions;
    CVMInt32 interpreterTransitionCost;
    CVMInt32 mixedTransitionCost;
    CVMInt32 backwardsBranchCost;
    CVMInt32 compileThreshold;
    CVMJITWhenToCompileOption whenToCompile;
    CVMUint32 whatToInline;
    CVMBool  registerPhis;
#ifdef CVM_JIT_REGISTER_LOCALS
    CVMBool  registerLocals;
#endif
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
#ifdef CVM_DEBUG
    CVMBool  codeSchedRemoveNOP;
#endif
    CVMBool  codeScheduling;
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */
    CVMBool  policyTriggeredDecompilations;
    CVMBool  compilingCausesClassLoading;
    CVMUint32 maxInliningDepth;
    CVMUint32 maxInliningCodeLength;
    CVMUint32 minInliningCodeLength;
    CVMUint32 maxWorkingMemorySize;
    CVMUint32 maxCompiledMethodSize;
    CVMUint32 codeCacheSize;  /* Size in bytes of the code cache. */
    CVMUint32 upperCodeCacheThresholdPercent; /* start decompiling */
    CVMUint32 lowerCodeCacheThresholdPercent; /* stop decompiling */
    CVMUint32 upperCodeCacheThreshold; /* start decompiling */
    CVMInt32  lowerCodeCacheThreshold; /* stop decompiling */
#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    CVMBool   doCSpeedMeasurement;
    CVMBool   doCSpeedTest;
#endif

    CVMInt32  *inliningThresholds;

    /* state information */
    volatile CVMBool compiling; /* true if we are currently compiling */
    CVMBool     destroyed;      /* true during shut after jit is destroyed */
    CVMBool     csNeedDisable;  /* true if CVMcsResumeConsistentState needs
				   to disable gc checkpoints */

    /* the code cache */
    CVMUint8*      codeCacheStart;  /* start of allocated code cache */
    CVMUint8*      codeCacheEnd;    /* end of allocated code cache */
    CVMUint8*      codeCacheDecompileStart; /* first method we can decompile */
    CVMUint8*      codeCacheNextDecompileScanStart; /* Next location to start
                                                  scanning for decompilation.*/
    CVMUint8*      codeCacheGeneratedCodeStart;/* first method in code cache */
    CVMJITFreeBuf* codeCacheFirstFreeBuf;      /* list of free bufferes */
    CVMUint32      codeCacheBytesAllocated;    /* bytes currently allocated */
    CVMUint32      codeCacheLargestFreeBuffer; /* largest free buffer */

#ifdef CVM_USE_MEM_MGR
    CVMMemType codeCacheMemType;
#endif

#ifdef CVM_JIT_PROFILE
    CVMUint16* profileBuf;
    char*      profile_filename;
    CVMInt32   profile_fd;
    CVMInt64   profileStartTime;
    CVMBool    profileInstructions;
#endif

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    /* Address of code we copy into the start of the code cache */
    CVMUint8* ccmCodeCacheCopyAddress;
#if defined(CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES)
    /* Mapping of copied entries to function names for profiling and debug. */
    CVMCCMCodeCacheCopyEntry* ccmCodeCacheCopyEntries;
#endif    
#endif

#if 1
    /*
     * These are stats that are very cheap to track - we could omit them for
     * optimized builds, but it's probably not necessary.
     */
    CVMUint32 codeCacheLowestForcedUtilization;
    CVMUint32 codeCacheFailedAllocations;
    CVMUint32 compilationAttempts;
    CVMUint32 failedCompilationAttempts;
#endif

#ifdef CVMJIT_INTRINSICS
    /* The following is for managing JIT intrinsics: */
    CVMJITIntrinsic *intrinsics;
    CVMJITIntrinsic **sortedJavaIntrinsics;
    CVMJITIntrinsic **sortedNativeIntrinsics;
    CVMJITIntrinsic **sortedUnknownIntrinsics;
    CVMUint8 numberOfIntrinsics;
    CVMUint8 numberOfJavaIntrinsics;
    CVMUint8 numberOfNativeIntrinsics;
    CVMUint8 numberOfUnknownIntrinsics;
#endif /* CVMJIT_INTRINSICS */

    /* Cache of inlining hints: */
    CVMMethodBlock *invokevirtualMBTargets[CVM_MAX_INVOKE_VIRTUAL_HINTS];
    CVMMethodBlock *invokeinterfaceMBTargets[CVM_MAX_INVOKE_INTERFACE_HINTS];

#ifdef CVM_DEBUG_ASSERTS
    /* Memory Fence Blocks list for the JIT long lived memory: */
    void *longLivedMemoryBlocks;
#endif /* CVM_DEBUG_ASSERTS */

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    CVMUint32 totalCompilationTime;
    CVMUint32 numberOfByteCodeBytesCompiled;
    CVMUint32 numberOfByteCodeBytesCompiledWithoutInlinedMethods;
    CVMUint32 numberOfBytesOfGeneratedCode;
    CVMUint32 numberOfMethodsCompiled;
    CVMUint32 numberOfMethodsNotCompiled;
#endif

#ifdef CVM_JIT_COLLECT_STATS
    CVMJITStatsToCollectOption statsToCollect;
    CVMUint32 statsOptions;
    CVMJITGlobalStats *globalStats;
#endif

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
    /* Patch points in ccm code required for gc */
    CVMCCMGCPatchPoint  ccmGcPatchPoints[CVMCPU_NUM_CCM_PATCH_POINTS];
#endif

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    /* Max offset in words allowed when loading from gcTrapAddr */
#define CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET 32
    void**   gcTrapAddr;  /* address that will trap when gc is requested */
    CVMCPUInstruction gcTrapInstr; /* instruction that will cause the trap */
#endif
} CVMJITGlobalState;

#ifdef CVM_MTASK
/*
 * Re-initialize the JIT in the client, parsing any new JIT options
 * that have been passed in
 */
extern CVMBool
CVMjitReinitialize(CVMExecEnv* ee, const char *options);
#endif

extern CVMBool
CVMjitInit(CVMExecEnv* ee, CVMJITGlobalState* jgs, const char *options);

extern void
CVMjitDestroy(CVMJITGlobalState* jgs);

extern void
CVMjitPrintUsage();


/* Purpose: Records the mb that was actually invoked from the specified pc. */
extern void
CVMjitSetInvokeVirtualHint(CVMExecEnv *ee, const CVMUint8 *pc,
                           CVMMethodBlock *mb);
#define CVMjitSetInvokeVirtualHint(ee, pc, mb) \
    (CVMglobals.jit.invokevirtualMBTargets[(CVMUint32)(pc) & CVM_VHINT_MASK] \
         = (mb))

/* Purpose: Gets the mb that was actually invoked from the specified pc. */
extern CVMMethodBlock *
CVMjitGetInvokeVirtualHint(CVMExecEnv *ee, const CVMUint8 *pc);
#define CVMjitGetInvokeVirtualHint(ee, pc) \
    (CVMglobals.jit.invokevirtualMBTargets[(CVMUint32)(pc) & CVM_VHINT_MASK])

/* Purpose: Invalidates all mbs in the hint table. */
extern void
CVMjitInvalidateInvokeVirtualHints(CVMClassBlock *cb);
#define CVMjitInvalidateInvokeVirtualHints(cb) \
    memset(CVMglobals.jit.invokevirtualMBTargets, 0, \
           (sizeof(CVMMethodBlock *) * CVM_MAX_INVOKE_VIRTUAL_HINTS))

/* Purpose: Records the mb that was actually invoked from the specified pc. */
extern void
CVMjitSetInvokeInterfaceHint(CVMExecEnv *ee, const CVMUint8 *pc,
                             CVMMethodBlock *mb);
#define CVMjitSetInvokeInterfaceHint(ee, pc, mb) \
    (CVMglobals.jit.invokeinterfaceMBTargets[(CVMUint32)(pc) & CVM_IHINT_MASK]\
         = (mb))

/* Purpose: Gets the mb that was actually invoked from the specified pc. */
extern CVMMethodBlock *
CVMjitGetInvokeInterfaceHint(CVMExecEnv *ee, const CVMUint8 *pc);
#define CVMjitGetInvokeInterfaceHint(ee, pc) \
    (CVMglobals.jit.invokeinterfaceMBTargets[(CVMUint32)(pc) & CVM_IHINT_MASK])

/* Purpose: Invalidates all mbs in the hint table. */
extern void
CVMjitInvalidateInvokeInterfaceHints(CVMClassBlock *cb);
#define CVMjitInvalidateInvokeInterfaceHints(cb) \
    memset(CVMglobals.jit.invokeinterfaceMBTargets, 0, \
           (sizeof(CVMMethodBlock *) * CVM_MAX_INVOKE_INTERFACE_HINTS))


#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
/* The following are used to estimate the compilation speed of the dynamic
   compiler.  The estimation need to be made without the interference of any
   statistics gathering during the compilation cycle.  Hence, it needs its own
   numberOfByteCodeBytesCompiled tracking rather than relying on the ones
   collected by the JIT statistics collector.  This way, stats collection can
   be turned off for an accurate estimate of compilation speed.
*/
extern void
CVMjitEstimateCompilationSpeed(CVMExecEnv *ee);

#define CVMJIT_NUMBER_OF_COMPILATION_PASS_MEASUREMENT_REPETITIONS 10

extern void
CVMjitReportCompilationSpeed();

#else
#define CVMjitEstimateCompilationSpeed(ee)
#define CVMjitReportCompilationSpeed()
#endif

#define CVM_COMPILEFLAGS_LOCK(ee)   \
    CVMsysMicroLock(ee, CVM_COMPILEFLAGS_MICROLOCK)
#define CVM_COMPILEFLAGS_UNLOCK(ee) \
    CVMsysMicroUnlock(ee, CVM_COMPILEFLAGS_MICROLOCK)

#endif /* CVM_JIT */

#endif /* _INCLUDED_JIT_COMMON_H */
