/*
 * @(#)jitinit_cpu.c	1.5 06/10/24
 * 
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/assert.h"
#include "javavm/include/globals.h"
#include "javavm/include/jit_common.h"
#include "javavm/include/ccm_runtime.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/porting/jit/jit.h"
#ifdef CVM_JIT_USE_FP_HARDWARE
#include "javavm/include/float_cpu.h"
#endif /* CVM_JIT_USE_FP_HARDWARE */
#include "javavm/include/jit/ccmcisc.h"

/*
 * A table so profiling code can tell what part of ccmcodecachecopy_cpu.o
 * we are executing in. Also used by some debugging code.
 */
#if defined(CVM_JIT_HAVE_CCM_CODECACHE_COPY_ENTRIES)
extern void CVMCCMreturnToInterpreter();
#define ENTRY(x) {(CVMUint8*)x, #x},
static const CVMCCMCodeCacheCopyEntry ccmCodeCacheCopyEntries[] = {
    /* ccm glue */
    ENTRY(CVMCCMruntimeThrowDivideByZeroGlue)
    ENTRY(CVMCCMruntimeCheckArrayAssignableGlue)
    ENTRY(CVMCCMruntimeCheckCastGlue)
    ENTRY(CVMCCMruntimeInstanceOfGlue)
    ENTRY(CVMCCMruntimeResolveGlue)
    ENTRY(CVMCCMruntimeResolveNewClassBlockAndClinitGlue)
    ENTRY(CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue)
    ENTRY(CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue)
    ENTRY(CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue)
    ENTRY(CVMCCMruntimeResolveClassBlockGlue)
    ENTRY(CVMCCMruntimeResolveArrayClassBlockGlue)
    ENTRY(CVMCCMruntimeResolveGetfieldFieldOffsetGlue)
    ENTRY(CVMCCMruntimeResolvePutfieldFieldOffsetGlue)
    ENTRY(CVMCCMruntimeResolveSpecialMethodBlockGlue)
    ENTRY(CVMCCMruntimeResolveMethodBlockGlue)
    ENTRY(CVMCCMruntimeResolveMethodTableOffsetGlue)
    ENTRY(CVMCCMruntimeRunClassInitializerGlue)
    ENTRY(CVMCCMruntimeLookupInterfaceMBGlue)
    ENTRY(CVMCCMruntimeThrowObjectGlue)
    ENTRY(CVMCCMruntimeMonitorEnterGlue)
    ENTRY(CVMCCMruntimeMonitorExitGlue)
    ENTRY(CVMCCMruntimeThrowNullPointerExceptionGlue)
    ENTRY(CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue)
    ENTRY(CVMCCMruntimeGCRendezvousGlue)
    ENTRY(CVMCCMruntimeSimpleSyncUnlockGlue)

    /* ccm allocators */
    ENTRY(CVMCCMruntimeNewGlue)
    ENTRY(CVMCCMruntimeNewArrayGlue)
#ifdef CVM_JIT_INLINE_NEWARRAY
    ENTRY(CVMCCMruntimeNewArrayGounlockandslowGlue)
    ENTRY(CVMCCMruntimeNewArrayGoslowGlue)
    ENTRY(CVMCCMruntimeNewArrayBadindexGlue)
#endif
    ENTRY(CVMCCMruntimeANewArrayGlue)
    ENTRY(CVMCCMruntimeMultiANewArrayGlue)

    /* ccm invocation code */
    ENTRY(CVMCCMinvokeJNIMethod)
    ENTRY(CVMCCMletInterpreterDoInvoke)
    ENTRY(CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr)
    ENTRY(CVMCCMinvokeCNIMethod)
    ENTRY(CVMCCMreturnFromSyncMethod)
    ENTRY(CVMCCMreturnFromMethod)
    ENTRY(CVMCCMreturnToInterpreter)
    ENTRY(CVMCCMtraceMethodCallGlue)
    ENTRY(CVMCCMinvokeStaticSyncMethodHelper)
    ENTRY(CVMCCMinvokeNonstaticSyncMethodHelper)

    /* ccm math helpers */
#if 0
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
#endif
#if 0
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

#ifdef CVM_JIT_USE_FP_HARDWARE
   /* specific to FPU of x86 */
   CVMglobals.jit.cpu.FPModeNearSgl = CVMX86_FP_mode_near_sgl;
   CVMglobals.jit.cpu.FPModeNearDbl = CVMX86_FP_mode_near_dbl;
   CVMglobals.jit.cpu.FPModeTrncDbl = CVMX86_FP_mode_trnc_dbl;

   /* encoding of 2^{-15360} in double-extended-precision */
   CVMglobals.jit.cpu.FPFactorSmall[0] = 0x00000000;
   CVMglobals.jit.cpu.FPFactorSmall[1] = 0x80000000;
   CVMglobals.jit.cpu.FPFactorSmall[2] = 0x03ff;

   /* encoding of 2^{+15360} in double-extended-precision */
   CVMglobals.jit.cpu.FPFactorLarge[0] = 0x00000000;
   CVMglobals.jit.cpu.FPFactorLarge[1] = 0x80000000;
   CVMglobals.jit.cpu.FPFactorLarge[2] = 0x7bff;
#endif /* CVM_JIT_USE_FP_HARDWARE */

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
