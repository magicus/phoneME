/*
 * @(#)jitir.h	1.44 06/10/10
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

#ifndef _INCLUDED_JITIR_H
#define _INCLUDED_JITIR_H

#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/typeid.h"

/*
 * Byte-code to IR translation
 */
extern CVMBool
CVMJITcompileBytecodeToIR(CVMJITCompilationContext* con);

/*
 * IR optimizations
 */
extern void
CVMJITcompileOptimizeIR(CVMJITCompilationContext* con);

/*
 * Some extra "java" data types we need in addition to those in typeid.h.
 * We use these when we can't determine the exact type of a java type,
 * such as with some of the lossy opcodes and with ldc.
 */
#define CVMJIT_TYPEID_32BITS          (CVM_TYPEID_LAST_PREDEFINED_TYPE + 1)
#define CVMJIT_TYPEID_64BITS          (CVM_TYPEID_LAST_PREDEFINED_TYPE + 2)
#define CVMJIT_TYPEID_ADDRESS         (CVM_TYPEID_LAST_PREDEFINED_TYPE + 3)
/* UBYTE is an unsigned byte */
#define CVMJIT_TYPEID_UBYTE           (CVM_TYPEID_LAST_PREDEFINED_TYPE + 4)
#define CVMJIT_TYPEID_LAST_PREDEFINED_TYPE      CVMJIT_TYPEID_UBYTE

#define CVMJITirIsSingleWordTypeTag(typeTag)	\
    (typeTag != CVMJIT_TYPEID_64BITS &&		\
     typeTag != CVM_TYPEID_DOUBLE &&		\
     typeTag != CVM_TYPEID_LONG &&		\
     typeTag != CVM_TYPEID_VOID)
#define CVMJITirIsDoubleWordTypeTag(typeTag)	\
    (typeTag == CVMJIT_TYPEID_64BITS ||		\
     typeTag == CVM_TYPEID_DOUBLE ||		\
     typeTag == CVM_TYPEID_LONG)

/*
 * Evaluate a stack item into a TEMP node and bump refcount.
 */
void
CVMJITirForceEvaluation(CVMJITCompilationContext* con, CVMJITIRBlock* curbk,
			CVMJITIRNode* stackNode);

/* Purpose: Force evaluation of all expressions on the operand stack that
            have side effects. */
void
CVMJITirDoSideEffectOperator(CVMJITCompilationContext* con,
                             CVMJITIRBlock* curbk);

/* Purpose: Flush all out bound locals because we're about to branch to
            another block (or have the possibly of branching to another
            block as in the case of opcodes that can throw exceptions). */
void
CVMJITirFlushOutBoundLocals(CVMJITCompilationContext* con,
                            CVMJITIRBlock* curbk,
			    CVMJITMethodContext* mc,
			    CVMBool flushAll);

#endif /* _INCLUDED_JITIR_H */
