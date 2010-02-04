/*
 * @(#)jitinit_cpu.c	1.30 06/10/10
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
#include "javavm/include/clib.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/iai_opt_config.h"
#include "javavm/include/directmem.h"
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
extern void CVMARMgcPatchPointAtInvoke();
extern void CVMARMhandleGCForReturnFromSyncMethod();
extern void CVMARMhandleGCForReturnFromMethod();
extern void CVMARMhandleGCAtInvoke();
static CVMUint8 * const ccmGcPatchPoints[CVMCPU_NUM_CCM_PATCH_POINTS] = {
    (CVMUint8*)CVMCCMreturnFromSyncMethod,
    (CVMUint8*)CVMCCMreturnFromMethod,
    (CVMUint8*)CVMARMgcPatchPointAtInvoke
};
static const CVMUint8 * const
gcPatchPointsBranchTargets[CVMCPU_NUM_CCM_PATCH_POINTS] = {
#ifdef CVM_JIT_CCM_USE_C_HELPER
    NULL,
    NULL,
    NULL
#else
    (const CVMUint8*)CVMARMhandleGCForReturnFromSyncMethod,
    (const CVMUint8*)CVMARMhandleGCForReturnFromMethod,
    (const CVMUint8*)CVMARMhandleGCAtInvoke
#endif
};
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

#ifdef CVMJIT_INTRINSICS
#ifndef CVM_JIT_CCM_USE_C_HELPER
#ifndef CVMCCM_DISABLE_ARM_CVM_SYSTEM_IDENTITYHASHCODE_INTRINSIC
extern void CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue(void);
#endif
extern void CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue(void);
extern void CVMCCMARMintrinsic_java_lang_String_hashCodeGlue(void);
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
extern CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_I(CVMObject* thisObj,
					   CVMJavaInt ch);
extern CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_II(CVMObject* thisObj,
                                            CVMJavaInt ch,
                                            CVMJavaInt fromIndex);
#endif /* IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY */
extern void CVMCCMintrinsic_sun_misc_CVM_copyCharArray(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyObjectArray(void);
extern void CVMCCMintrinsic_java_lang_System_arraycopyGlue(void);
#endif /* !CVM_JIT_CCM_USE_C_HELPER */
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

    /* intrinsics code */
#ifdef CVMJIT_INTRINSICS
#ifndef CVM_JIT_CCM_USE_C_HELPER
#ifndef CVMCCM_DISABLE_ARM_CVM_SYSTEM_IDENTITYHASHCODE_INTRINSIC
    ENTRY(CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue)
#endif
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
    ENTRY(CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue)
    ENTRY(CVMCCMARMintrinsic_java_lang_String_hashCodeGlue)
#endif
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
    ENTRY(CVMCCMintrinsic_java_lang_String_indexOf_II)
    ENTRY(CVMCCMintrinsic_java_lang_String_indexOf_I)
#endif
    ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyCharArray)
    ENTRY(CVMCCMintrinsic_sun_misc_CVM_copyObjectArray)
    ENTRY(CVMCCMintrinsic_java_lang_System_arraycopyGlue)
#endif
#endif

    /* ccm math helpers */
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
    ENTRY(CVMCCMruntimeLUshr)
    ENTRY(CVMCCMruntimeLShr)
    ENTRY(CVMCCMruntimeLShl)
    ENTRY(CVMCCMruntimeIDiv)
    ENTRY(CVMCCMruntimeIRem)

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

    /*
     * If the ARM indexOf() assembler intrinsics are enabled, then
     * we cannot have a char read barrier.
     */
#if defined(IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY) && \
    !defined(CVMGC_HAS_NO_CHAR_READ_BARRIER)
    CVMassert(CVM_FALSE);
#endif

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
	    jgs->ccmGcPatchPoints[i].patchPoint = ccmGcPatchPoints[i];
	    jgs->ccmGcPatchPoints[i].patchInstruction =
		CVMARMgetBranchInstruction(CVMCPU_COND_AL, offset, CVM_FALSE);
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

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
/* Purpose: back-end PMI initialization. */
CVMBool
CVMJITPMIinitBackEnd(void)
{
    return CVM_TRUE;
}
#endif
