/*
 * @(#)ccmintrinsics_cpu.c	1.15 06/10/29
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

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#include "javavm/include/objects.h"
#include "javavm/include/ccee.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitirnode.h"

#ifdef CVMJIT_INTRINSICS

#ifndef CVM_JIT_CCM_USE_C_HELPER
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
extern void CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue(void);
extern void CVMCCMARMintrinsic_java_lang_String_hashCodeGlue(void);
#endif
extern void CVMCCMintrinsic_sun_misc_CVM_copyCharArray(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyObjectArray(void);
extern void CVMCCMintrinsic_java_lang_System_arraycopyGlue(void);
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
extern void CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue(void);
#endif

/* IAI-05 */
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
extern CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_I(CVMObject* thisObj,
                                           CVMJavaInt ch);
extern CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_II(CVMObject* thisObj,
                                            CVMJavaInt ch,
                                            CVMJavaInt fromIndex);
#endif

#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
CVMInt32
CVMCCMARMintrinsic_java_lang_Object_hashCode(CVMCCExecEnv *ccee, jobject obj)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMInt32 hashCode = 0;
    CVMassert(CVMID_icellDirect(ee, obj) != NULL);
    CVMCCMruntimeLazyFixups(ee);
    CVMD_gcSafeExec(ee, {
        hashCode = CVMobjectGetHashSafe(ee, obj);
    });
    return hashCode;
}
#endif

CVMJIT_INTRINSIC_CONFIG_BEGIN(CVMJITarmIntrinsicsList)
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
    {
        "java/lang/Object", "hashCode", "()I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_JAVA_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        CVMCCMARMintrinsic_java_lang_Object_hashCodeGlue,
    },
    {
        "java/lang/String", "hashCode", "()I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        CVMCCMARMintrinsic_java_lang_String_hashCodeGlue,
    },
#endif
/* IAI-05 */
#ifdef IAI_IMPLEMENT_INDEXOF_IN_ASSEMBLY
    {
        "java/lang/String", "indexOf", "(I)I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_I,
    },
    {
        "java/lang/String", "indexOf", "(II)I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_II,
    },
#endif

    {
        "sun/misc/CVM", "copyCharArray", "([CI[CII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_REG_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyCharArray,
    },
#if (CVM_GCCHOICE == CVM_GC_GENERATIONAL) && !defined(CVM_SEGMENTED_HEAP)
    {
        "sun/misc/CVM", "copyObjectArray",
	"([Ljava/lang/Object;I[Ljava/lang/Object;II)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_REG_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyObjectArray,
    },
#endif /* CVM_GC_GENERATIONAL && !CVM_SEGMENTED_HEAP */
    {
        "java/lang/System",
        "arraycopy", "(Ljava/lang/Object;ILjava/lang/Object;II)V",
        CVMJITINTRINSIC_IS_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_REG_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
	CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_NO_CP_DUMP |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_System_arraycopyGlue,
    },
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
#ifndef CVMCCM_DISABLE_ARM_CVM_SYSTEM_IDENTITYHASHCODE_INTRINSIC
    {
        "java/lang/System", "identityHashCode", "(Ljava/lang/Object;)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_JAVA_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        CVMCCMARMintrinsic_java_lang_System_identityHashCodeGlue,
    },
#endif
#endif

CVMJIT_INTRINSIC_CONFIG_END(CVMJITarmIntrinsicsList,
                            &CVMJITriscIntrinsicsList)

#endif /* CVM_JIT_CCM_USE_C_HELPER */

#endif /* CVMJIT_INTRINSICS */
