/*
 * @(#)jitinit_cpu.c	1.27 06/10/10
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
#include "javavm/include/assert.h"
#include "javavm/include/globals.h"
#include "javavm/include/jit_common.h"
#include "javavm/include/ccm_runtime.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/porting/jit/jit.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"
#include "portlibs/jit/risc/include/porting/ccmrisc.h"

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
/*
 * All the ccm locations that we need to patch for gc purposes.
 *
 * -ccmGcPatchPoints is the array of locations we need to patch.
 * -gcPatchPointsBranchTargets is the array of locations that we
 *  want to patch the ccmGcPatchPoints to branch to.
 */
extern void CVMSPARCgcPatchPointAtInvoke();
extern void CVMSPARChandleGCForReturnFromSyncMethod();
extern void CVMSPARChandleGCForReturnFromMethod();
extern void CVMSPARChandleGCAtInvoke();
static CVMUint8 * const ccmGcPatchPoints[CVMCPU_NUM_CCM_PATCH_POINTS] = {
    (CVMUint8*)CVMCCMreturnFromSyncMethod,
    (CVMUint8*)CVMCCMreturnFromMethod,
    (CVMUint8*)CVMSPARCgcPatchPointAtInvoke,
};
static const CVMUint8 * const
gcPatchPointsBranchTargets[CVMCPU_NUM_CCM_PATCH_POINTS] = {
#if 0
    /* TODO: Implement gc patch for SPARC. 
             See the ARM port for example. */
    (const CVMUint8*)CVMSPARChandleGCForReturnFromSyncMethod,
    (const CVMUint8*)CVMSPARChandleGCForReturnFromMethod,
    (const CVMUint8*)CVMSPARChandleGCAtInvoke,
#else
    NULL,
    NULL,
    NULL
#endif
};
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

/*
 * A table so profiling code can tell what part of ccmcodecachecopy_cpu.o
 * we are executing in. Also used by some debugging code.
 */
#if defined(CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES)
extern void CVMCCMreturnToInterpreter();
#define ENTRY(x) {(CVMUint8*)x, #x},
static const CVMCCMCodeCacheCopyEntry ccmCodeCacheCopyEntries[] = {
    /* ccm glue */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
    ENTRY(CVMCCMruntimeSimpleSyncUnlockGlue)
#endif
    ENTRY(CVMCCMruntimeGCRendezvousGlue)
    ENTRY(CVMCCMruntimeThrowNullPointerExceptionGlue)
    ENTRY(CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue)
    ENTRY(CVMCCMruntimeThrowDivideByZeroGlue)
    ENTRY(CVMCCMruntimeThrowObjectGlue)
    ENTRY(CVMCCMruntimeCheckCastGlue)
    ENTRY(CVMCCMruntimeInstanceOfGlue)
    ENTRY(CVMCCMruntimeCheckArrayAssignableGlue)
    ENTRY(CVMCCMruntimeRunClassInitializerGlue)
    ENTRY(CVMCCMruntimeResolveGlue)
    ENTRY(CVMCCMruntimeLookupInterfaceMBGlue)
    ENTRY(CVMCCMruntimeMonitorEnterGlue)
    ENTRY(CVMCCMruntimeMonitorExitGlue)

    /* ccm allocators */
    ENTRY(CVMCCMruntimeNewGlue)
    ENTRY(CVMCCMruntimeNewArrayGlue)
    ENTRY(CVMCCMruntimeANewArrayGlue)
    ENTRY(CVMCCMruntimeMultiANewArrayGlue)

    /* ccm invocation code */
    ENTRY(CVMCCMinvokeNonstaticSyncMethodHelper)
    ENTRY(CVMCCMinvokeStaticSyncMethodHelper)
    ENTRY(CVMCCMinvokeCNIMethod)
    ENTRY(CVMCCMinvokeJNIMethod)
    ENTRY(CVMCCMletInterpreterDoInvoke)
    ENTRY(CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr)
    ENTRY(CVMCCMreturnFromMethod)
    ENTRY(CVMCCMreturnToInterpreter)
    ENTRY(CVMCCMreturnFromSyncMethod)
#ifdef CVM_TRACE
    ENTRY(CVMCCMtraceMethodCallGlue)
#if 0
    ENTRY(CCMtraceMethodReturn)
    ENTRY(CCMtraceFramelessMethodCall)
    ENTRY(CCMtraceFramelessMethodReturn)
#endif
#endif

    /* ccm math helpers */
#if 0
    /* TODO: currently the floating point math helpers are still in C. It
     * would be useful to have assembler versions of them for Sparc
     * processors without an FPU. They give you big gains in the spec
     * mtrt and mpegaudio benchmarks (probably 2X). If there is an FPU,
     * then they are not needed.
     */
    ENTRY(CVMCCMruntimeFCmp)
    ENTRY(CVMCCMruntimeFSub)
    ENTRY(CVMCCMruntimeFAdd)
    ENTRY(CVMCCMruntimeFMul)
    ENTRY(CVMCCMruntimeF2D)
    ENTRY(CVMCCMruntimeDCmpg)
    ENTRY(CVMCCMruntimeDCmpl)
    ENTRY(CVMCCMruntimeF2I)
    ENTRY(CVMCCMruntimeI2F)
    ENTRY(CVMCCMruntimeI2D)
    ENTRY(CVMCCMruntimeD2I)
    ENTRY(CVMCCMruntimeDMul)
    ENTRY(CVMCCMruntimeDSub)
    ENTRY(CVMCCMruntimeDAdd)
#endif
    ENTRY(CVMCCMruntimeLUshr)
    ENTRY(CVMCCMruntimeLShr)
    ENTRY(CVMCCMruntimeLShl)
#if 0
    /* The following are not needed on sparc because they are done inline */
    ENTRY(CVMCCMruntimeIDiv)
    ENTRY(CVMCCMruntimeIRem)
#endif
    {(CVMUint8*)&CVMCCMcodeCacheCopyEnd, NULL},
};
#undef ENTRY
#endif

/* Purpose: Initialized the platform specific compiler back-end. */
CVMBool
CVMJITinitCompilerBackEnd(void)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    (void)jgs; /* get rid of compiler warning for some builds */

#ifdef CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES    
    /* Let the codecache know where the table of ccm entypoints is. */
    jgs->ccmCodeCacheCopyEntries = (CVMCCMCodeCacheCopyEntry*)
        calloc(1, sizeof(ccmCodeCacheCopyEntries));
    if (jgs->ccmCodeCacheCopyEntries == NULL) {
        return CVM_FALSE;
    }

    memcpy(jgs->ccmCodeCacheCopyEntries, ccmCodeCacheCopyEntries,
        sizeof ccmCodeCacheCopyEntries);
#endif

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
    CVMassert(sizeof(ccmGcPatchPoints)/sizeof(const CVMUint8*) == 
	      CVMCPU_NUM_CCM_PATCH_POINTS);

    /* fixup the patchInstruction fields in ccmGcPatchPoints. */
    {
	int i;
	for (i=0; i < CVMCPU_NUM_CCM_PATCH_POINTS; i++) {
	    int offset =
		gcPatchPointsBranchTargets[i] - ccmGcPatchPoints[i];
	    if (gcPatchPointsBranchTargets[i] == NULL) {
		continue;
	    }
	    jgs->ccmGcPatchPoints[i].patchPoint = ccmGcPatchPoints[i];
	    jgs->ccmGcPatchPoints[i].patchInstruction =
		CVMSPARCgetBranchInstruction(CVMSPARC_BRANCH_OPCODE,
					     CVMCPU_COND_AL, offset);
	}
    }
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

   return CVM_TRUE;
}

/* Purpose: Destroys the platform specific compiler back-end. */
void
CVMJITdestroyCompilerBackEnd(void)
{
#ifdef CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES
    CVMJITGlobalState* jgs = &CVMglobals.jit;

    if (jgs->ccmCodeCacheCopyEntries != NULL) {
        free(jgs->ccmCodeCacheCopyEntries);
        jgs->ccmCodeCacheCopyEntries = NULL;
    }
#endif
}
