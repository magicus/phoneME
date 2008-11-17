/*
 * @(#)jit_cisc.h	1.7 06/10/24
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
 * This file implements part of the jit porting layer.
 */

#ifndef _INCLUDED_JIT_CISC_H
#define _INCLUDED_JIT_CISC_H

#include "javavm/include/sync.h"

/*
 * Support Simple Sync methods if we do fast locking with spinlock microlocks
 * or with atomic ops unless the platform has told us not to be #defining
 * CVMCPU_NO_SIMPLE_SYNC_METHODS.
 */
#if defined(CVMJIT_INTRINSICS) && !defined(CVMCPU_NO_SIMPLE_SYNC_METHODS)
#if (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK &&     \
     CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK) || \
    CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
#define CVMJIT_SIMPLE_SYNC_METHODS
#endif
#endif /* !CVMCPU_NO_SIMPLE_SYNC_METHODS */

#ifndef _ASM

#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitciscemitterdefs_cpu.h"

/* Purpose: Massage the compiled PC.  This is used in the mapping of a compiled
            PC to Java Bytecode PC.  The massaging is necessary because
            compiled PCs point to the return address for a method call as
            instead of the caller's PC as is the convention for Java PCs. */
#define CVMJITmassageCompiledPC(compiledPC, startPC)		            \
    (((compiledPC) == (startPC)) ?				            \
     (CVMUint8 *)(compiledPC) :					            \
     (CVMUint8 *)(compiledPC) - CVMCPU_CALL_SIZE)

/* Define CVM_DEBUG_JIT_TRACE_CODEGEN_RULE_EXECUTION to enable tracing of each
   rule that is executed by the code generator:
#define CVM_DEBUG_JIT_TRACE_CODEGEN_RULE_EXECUTION
*/

#if !defined(CVM_DEBUG_ASSERTS)
#define CVMJITdoEndOfCodegenRuleAction(con)
#endif

/* the datatype that represents an instruction */
#if CVMCPU_INSTRUCTION_SIZE == 2
typedef CVMUint16 CVMCPUInstruction;
#elif CVMCPU_INSTRUCTION_SIZE == 4
typedef CVMUint32 CVMCPUInstruction;
#elif CVMCPU_INSTRUCTION_SIZE == 8
typedef CVMInt64 CVMCPUInstruction;
#else
#error Unsupported CVMCPU_INSTRUCTION_SIZE
#endif

/*
 * Code offsets within the method that require GC check instructions
 */
struct CVMCompiledGCCheckPCs {
    CVMUint16		     noEntries;
    CVMUint16                pcEntries[1];
    /* This declaration is for documentation purposes only. pcEntries
     * is a variable length array and patchedInstructions[] occurs
     * after it.
     */
    CVMCPUInstruction        patchedInstructions[1];
};

#define CVMCompiledGCCheckPCs_patchedInstructions(compiledgcpcs, size)				\
  ( (CVMCPUInstruction*)									\
	    ((((CVMUint32)&((compiledgcpcs)->pcEntries[size]))					\
	      + MIN( sizeof(CVMCPUInstruction), CVMCPU_MAX_MEMORY_ALIGNMENT ) - 1 )		\
	     & ~( MIN( sizeof(CVMCPUInstruction), CVMCPU_MAX_MEMORY_ALIGNMENT ) - 1 ) ) )

struct CVMJITTargetCompilationContext {
    /*
     * If the platform has a dedicated base register for the constant pool,
     * then this is where we save away the start of the constant pool dump.
     */
#ifdef CVMCPU_HAS_CP_REG
    CVMInt32    cpLogicalPC;
#else
    int  dummy;   /* need something to avoid compiler error */
#endif

#ifndef CVMCPU_HAS_COMPARE
    /*
     * If the platform has no compare instruction. The arguments of
     * compare are saved in CompilationContext, so the comparison
     * can be deferred until the result is used in a conditional
     * branch.
     */
    int cmpOpcode;
    int cmpLhsRegID;
    int cmpRhsType;
    int cmpRhsRegID;
    CVMInt32 cmpRhsConst;
    CVMCPUCondCode cmpCondCode;
#endif

#ifdef CVM_JIT_USE_FP_HARDWARE
    /* x87 specific parameters */
      CVMBool                   usesFPU /* flag is set if currently compiled method uses FPU instructions */;
#endif
};

#ifdef CVMJIT_INTRINSICS
/*
 * CVMJITintrinsicsList: Override this value to be the RISC version of the
 * intrinsics config list if the target platform does not define one.
 */
#ifndef CVMJITintrinsicsList
#define CVMJITintrinsicsList CVMJITriscIntrinsicsList
#endif

extern const CVMJITIntrinsicConfigList CVMJITriscIntrinsicsList;

/*
 * CVMJITriscParentIntrinsicsList: Set this value to have the RISC version
 * inherit from the default intrinsics config list if the target platform does
 * not specify otherwise.
 */
#ifndef CVMJITriscParentIntrinsicsList
#define CVMJITriscParentIntrinsicsList CVMJITdefaultIntrinsicsList
#endif

#endif /* CVMJIT_INTRINSICS */

/*
 * Values assigned to the irnode's regsRequired field.
 *
 * WARNING, the CVMCPUavoidSets and CVMCPUfloatAvoidSets are dependent
 * on the values given below.
 */
typedef enum {
    CVMCPU_AVOID_NONE 		= 0,
    CVMCPU_AVOID_C_CALL 	= 1U<<0,
    CVMCPU_AVOID_METHOD_CALL 	= 1U<<1
#ifdef CVM_DEBUG
} CVMJITRegsRequiredType;
#else
} CVMJITRegsRequiredTypeValues;
typedef CVMUint8 CVMJITRegsRequiredType;
#endif

#endif /* !_ASM */
#endif /* _INCLUDED_JIT_CISC_H */
