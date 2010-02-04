/*
 * @(#)jit.h	1.30 06/10/10
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

#ifndef _INCLUDED_JIT_H
#define _INCLUDED_JIT_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/porting/jit/jit.h"

/*
 * Public interface to JIT from the rest of the VM.
 *
 * CVMJITReturnValue indicates how the compiler fared.
 */
typedef enum CVMJITReturnValue {
    CVMJIT_SUCCESS		= 1,
    CVMJIT_CANNOT_COMPILE,	     /* Refusal to compile, don't try again */
    CVMJIT_OUT_OF_MEMORY,	     /* out of memory, try again later */
    CVMJIT_CANNOT_COMPILE_NOW,	     /* can't compile now, try again later */
    CVMJIT_RETRYABLE,
    CVMJIT_CODEBUFFER_TOO_SMALL =
        CVMJIT_RETRYABLE,            /* try again with a larger size */
#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    CVMJIT_RETRY,
#endif
    CVMJIT_INLINING_TOO_DEEP,	     /* try again with less inlining */
    CVMJIT_RETRY_LATER,              /* the method is not compilable now,
                                        try again later */
    CVMJIT_DEFINE_USED_NODE_MISMATCH /* try again until inline depth is 0. */
} CVMJITReturnValue;

/*
 * Entry point 
 */
extern CVMJITReturnValue
CVMJITcompileMethod(CVMExecEnv *ee, CVMMethodBlock* mb);

extern void
CVMJITdecompileMethod(CVMExecEnv* ee, CVMMethodBlock* mb);

extern CVMBool
CVMJITneverCompileMethod(CVMExecEnv *ee, CVMMethodBlock* mb);

extern CVMBool
CVMJITinitializeContext(CVMJITCompilationContext* con, CVMMethodBlock* mb,
			CVMExecEnv* ee, CVMInt32 maxAllowedInliningDepth);

extern void
CVMJITinitializeCompilation(CVMJITCompilationContext* con);

extern void
CVMJITdestroyContext(CVMJITCompilationContext* con);

#ifdef CVM_DEBUG_ASSERTS
/* Purpose: Assert some assumptions made in the implementation of the JIT. */
extern void
CVMJITassertMiscJITAssumptions(void);
#endif

extern void
CVMJITmarkCodeBuffer();

#endif /* _INCLUDED_JIT_H */
