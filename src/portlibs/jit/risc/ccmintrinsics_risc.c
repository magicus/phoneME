/*
 * @(#)ccmintrinsics_risc.c	1.9 06/10/10
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

#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitirnode.h"

#ifdef CVMJIT_INTRINSICS

extern const CVMJITIntrinsicEmitterVtbl
    CVMJITRISCintrinsicThreadCurrentThreadEmitter;
extern const CVMJITIntrinsicEmitterVtbl CVMJITRISCintrinsicIAbsEmitter;
extern const CVMJITIntrinsicEmitterVtbl CVMJITRISCintrinsicIMaxEmitter;
extern const CVMJITIntrinsicEmitterVtbl CVMJITRISCintrinsicIMinEmitter;

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
extern const CVMJITIntrinsicEmitterVtbl
    CVMJITRISCintrinsicSimpleLockGrabEmitter;
extern const CVMJITIntrinsicEmitterVtbl
    CVMJITRISCintrinsicSimpleLockReleaseEmitter;
#endif

CVMJIT_INTRINSIC_CONFIG_BEGIN(CVMJITriscIntrinsicsList)
    {
        "java/lang/Math", "abs", "(I)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIAbsEmitter,
    },
    {
        "java/lang/Math", "max", "(II)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIMaxEmitter,
    },
    {
        "java/lang/Math", "min", "(II)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIMinEmitter,
    },
    {
        "java/lang/StrictMath", "abs", "(I)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIAbsEmitter,
    },
    {
        "java/lang/StrictMath", "max", "(II)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIMaxEmitter,
    },
    {
        "java/lang/StrictMath", "min", "(II)I",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicIMinEmitter,
    },
    {
        "java/lang/Thread", "currentThread", "()Ljava/lang/Thread;",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void *)&CVMJITRISCintrinsicThreadCurrentThreadEmitter,
    },
#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK &&	\
     CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK) || \
    CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
    {
        "sun/misc/CVM", "simpleLockGrab", "(Ljava/lang/Object;)Z",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicSimpleLockGrabEmitter,
    },
    {
        "sun/misc/CVM", "simpleLockRelease", "(Ljava/lang/Object;)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_OPERATOR_ARGS | CVMJITINTRINSIC_SPILLS_NOT_NEEDED |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_NULL_FLAGS,
        (void *)&CVMJITRISCintrinsicSimpleLockReleaseEmitter,
    },
#else
#error Unsupported locking type for CVMJIT_SIMPLE_SYNC_METHODS
#endif
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

CVMJIT_INTRINSIC_CONFIG_END(CVMJITriscIntrinsicsList,
                            &CVMJITriscParentIntrinsicsList)

#endif /* CVMJIT_INTRINSICS */
