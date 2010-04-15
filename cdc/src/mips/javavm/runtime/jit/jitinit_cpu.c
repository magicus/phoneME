/*
 * @(#)jitinit_cpu.c	1.18 06/10/10
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
extern void CVMMIPSgcPatchPointAtInvoke();
extern void CVMMIPShandleGCForReturnFromSyncMethod();
extern void CVMMIPShandleGCForReturnFromMethod();
extern void CVMMIPShandleGCAtInvoke();

static CVMUint8 * const ccmGcPatchPoints[CVMCPU_NUM_CCM_PATCH_POINTS] = {
    (CVMUint8*)CVMCCMreturnFromSyncMethod,
    (CVMUint8*)CVMCCMreturnFromMethod,
    (CVMUint8*)CVMMIPSgcPatchPointAtInvoke,
};
static const CVMUint8 * const
gcPatchPointsBranchTargets[CVMCPU_NUM_CCM_PATCH_POINTS] = {
#if 0
    /* TODO: Implement gc patch for MIPS. See the ARM port
             for example. */
    (const CVMUint8*)CVMMIPShandleGCForReturnFromSyncMethod,
    (const CVMUint8*)CVMMIPShandleGCForReturnFromMethod,
    (const CVMUint8*)CVMMIPShandleGCAtInvoke
#else
    NULL,
    NULL,
    NULL
#endif
};
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

#ifdef CVMJIT_INTRINSICS
extern void CVMCCMMIPSintrinsic_java_lang_System_identityHashCodeGlue(void);
extern void CVMCCMMIPSintrinsic_java_lang_Object_hashCodeGlue(void);
extern void CVMCCMMIPSintrinsic_java_lang_String_hashCodeGlue(void);
extern void CVMCCMintrinsic_java_lang_String_indexOf0_STRING_I(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyCharArray(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyObjectArray(void);
extern void CVMCCMintrinsic_java_lang_System_arraycopyGlue(void);
#endif /* CVMJIT_INTRINSICS */

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
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    ENTRY(CVMCCMinvokeVirtual)
#endif
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
     * would be useful to have assembler versions of them for Mips
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
    /* The following are not needed on mips because they are done inline */
    ENTRY(CVMCCMruntimeIDiv)
    ENTRY(CVMCCMruntimeIRem)
#endif
#ifdef CVMJIT_INTRINSICS
#ifndef CVMCCM_DISABLE_MIPS_CVM_INDENTITYHASHCODE_INTRINSIC
    ENTRY(CVMCCMMIPSintrinsic_java_lang_System_identityHashCodeGlue)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_OBJECTHASHCODE_INTRINSIC
    ENTRY(CVMCCMMIPSintrinsic_java_lang_Object_hashCodeGlue)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_STRINGHASHCODE_INTRINSIC
    ENTRY(CVMCCMMIPSintrinsic_java_lang_String_hashCodeGlue)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_INDEXOF_INTRINSIC
    ENTRY(CVMCCMintrinsic_java_lang_String_indexOf0_STRING_I)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_COPYCHARARRAY_INTRINSIC
    ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyCharArray)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_COPYOBJECTARRAY_INTRINSIC
    ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyObjectArray)
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_ARRAYCOPY_INTRINSIC
    ENTRY(CVMCCMintrinsic_java_lang_System_arraycopyGlue)
#endif
#endif /* CVMJIT_INTRINSICS */
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

#ifdef TODO /* TODO: Enable this when patch points are implemented. */
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
		CVMMIPSgetBranchInstruction(CVMMIPS_BRANCH_OPCODE,
					    CVMCPU_COND_AL, offset);
	}
    }
#endif
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

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
/* Purpose: back-end PMI initialization. */
CVMBool
CVMJITPMIinitBackEnd(void)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    const CVMUint32 regionMask = 0xf0000000;
    
    /* In order for PMI to work on MIPS, the entire code cache has to
       fit entirely in one 256mb region. Otherwise we will not be able
       to properly patch JAL instructions that end up jumping between
       JAL regions.
    */
    if (((CVMUint32)jgs->codeCacheStart & regionMask) !=
        ((CVMUint32)jgs->codeCacheEnd & regionMask))
    {
        CVMdebugPrintf(("WARNING: PMI disabled because code cache is not "
                        "entirely located within one 256mb JAL region\n"));
        CVMglobals.jit.pmiEnabled = CVM_FALSE;
    }
    return CVM_TRUE;
}
#endif
