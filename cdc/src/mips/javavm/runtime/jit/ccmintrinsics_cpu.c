/*
 * @(#)ccmintrinsics_cpu.c	1.8 06/10/10
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

#include "javavm/include/objects.h"
#include "javavm/include/ccee.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitirnode.h"

#ifdef CVMJIT_INTRINSICS

extern void CVMCCMMIPSintrinsic_java_lang_Object_hashCodeGlue(void);
extern void CVMCCMMIPSintrinsic_java_lang_String_hashCodeGlue(void);
extern void CVMCCMMIPSintrinsic_java_lang_System_identityHashCodeGlue(void);
extern void CVMCCMintrinsic_java_lang_String_indexOf0_STRING_I(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyCharArray(void);
extern void CVMCCMintrinsic_sun_misc_CVM_copyObjectArray(void);
extern void CVMCCMintrinsic_java_lang_System_arraycopyGlue(void);

#if !defined(CVMCCM_DISABLE_MIPS_CVM_OBJECTHASHCODE_INTRINSIC) || \
    !defined(CVMCCM_DISABLE_MIPS_CVM_STRINGHASHCODE_INTRINSIC)

CVMInt32
CVMCCMMIPSintrinsic_java_lang_Object_hashCode(CVMCCExecEnv *ccee, jobject obj)
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

CVMJIT_INTRINSIC_CONFIG_BEGIN(CVMJITmipsIntrinsicsList)

#ifndef CVMCCM_DISABLE_MIPS_CVM_OBJECTHASHCODE_INTRINSIC
    {
        "java/lang/Object", "hashCode", "()I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_JAVA_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        CVMCCMMIPSintrinsic_java_lang_Object_hashCodeGlue,
    },
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_STRINGHASHCODE_INTRINSIC
    {
        "java/lang/String", "hashCode", "()I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        CVMCCMMIPSintrinsic_java_lang_String_hashCodeGlue,
    },
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_INDEXOF_INTRINSIC
    {
        "java/lang/String", "indexOf0", "(Ljava/lang/String;I)I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf0_STRING_I,
    },
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_COPYCHARARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyCharArray", "([CI[CII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_REG_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyCharArray,
    },
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_COPYOBJECTARRAY_INTRINSIC
#if (CVM_GCCHOICE != CVM_GC_GENERATIONAL) || defined(CVM_SEGMENTED_HEAP)
#error "#undef CVMCCM_DISABLE_MIPS_CVM_COPYCHARARRAY_INTRINSIC not supported"
#endif
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
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_ARRAYCOPY_INTRINSIC
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
#endif
#ifndef CVMCCM_DISABLE_MIPS_CVM_INDENTITYHASHCODE_INTRINSIC
    {
        "java/lang/System", "identityHashCode", "(Ljava/lang/Object;)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_JAVA_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        CVMCCMMIPSintrinsic_java_lang_System_identityHashCodeGlue,
    },
#endif

CVMJIT_INTRINSIC_CONFIG_END(CVMJITmipsIntrinsicsList,
                            &CVMJITriscIntrinsicsList)
#endif /* CVMJIT_INTRINSICS */
