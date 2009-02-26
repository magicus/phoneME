/*
 * @(#)jitriscemitter_cpu.h	1.17 06/10/10
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

#ifndef _INCLUDED_JITRISCEMITTER_CPU_H
#define _INCLUDED_JITRISCEMITTER_CPU_H

#include "javavm/include/jit/jitasmconstants.h"

/*
 * This file #defines all of the emitter function prototypes and macros that
 * shared parts use in a platform indendent way in RISC ports of the jit back
 * end. The exported functions and macros are prefixed with CVMCPU_.  Other
 * functions and macros should be considered private to the PPC 
 * specific parts of the jit, including those with the CVMPPC_ prefix.
 */

/**********************************************************************
 * Some very PPC-specific bits of code generation.
 * Our stack layout and the names of the registers we're going to use
 * to access it:
 *
 *	+----------------------
 *	| Java local 0
 *	+----------------------
 *	...
 *	+----------------------
 *	| Java local n
 *	+----------------------
 * JFP->| control information here
 *	| of some size...
 *	+----------------------
 *	| Java temp 0	( code generation allocated )
 *	+----------------------
 *	...
 *	+----------------------
 *	| Java temp n	( code generation allocated )
 *	+----------------------
 * JSP->| Outgoing Java parameters.
 *	+----------------------
 *
 * Java locals and temps accessed via JFP,
 * JSP  used to access only the stack top.
 *
 * In truth, we really only need JSP, since the offsets of everything
 * else are known at compilation of each instruction. But we could never
 * debug the resulting code! This seems like a bad idea in the short run.
 */

/**************************************************************
 * CPU ALURhs and associated types - The following are function
 * prototypes and macros for the ALURhs abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

#define CVMCPUalurhsIsConstant(aluRhs) \
    ((aluRhs)->type == CVMPPC_ALURHS_CONSTANT)

#define CVMCPUalurhsGetConstantValue(aluRhs)    ((aluRhs)->constValue)

/* ALURhs token encoder APIs:============================================= */

/* Purpose: Gets the token for the ALURhs value for use in the instruction
            emitters. */
CVMCPUALURhsToken
CVMPPCalurhsGetToken(const CVMCPUALURhs *aluRhs);

#define CVMCPUalurhsGetToken(con, aluRhs)   \
     ((void)(con), CVMPPCalurhsGetToken(aluRhs))

/* NOTE: The CVMCPUALURhsToken is encoded as:
        bits 17 - 16:   CVMPPCALURhsType;
        bits 0 - 15:    immediate value or reg;
*/

/* Purpose: Encodes an CVMCPUALURhsToken for the specified arguments. */
/* NOTE: This function is private to the PPC implementation and can only be
         called from the shared implementation via the CVMCPUxxx macros
         below. */

/* Purpose: Encodes a CVMCPUALURhsToken for the specified constant value. */
#define CVMPPCalurhsEncodeConstantToken(constValue)			\
    (CVMPPC_ALURHS_CONSTANT << 16 | ((constValue) & 0xffff))

#define CVMPPCalurhsEncodeRegisterToken(regID) \
    (CVMPPC_ALURHS_REGISTER << 16 | (regID))

#define CVMPPCalurhsDecodeGetTokenType(token) \
    (token >> 16)

#define CVMPPCalurhsDecodeGetTokenConstant(token) \
    ((CVMInt32)(token << 16) >> 16)  /* sign extended */

#define CVMPPCalurhsDecodeGetTokenRegister(token) \
    (token & 0xffff)

/**************************************************************
 * CPU MemSpec and associated types - The following are function
 * prototypes and macros for the MemSpec abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* MemSpec query APIs: ==================================================== */

#define CVMCPUmemspecIsEncodableAsImmediate(offset) \
    ((((CVMUint32)(offset) & 0xffff8000) == 0) ||        /* 0..32767 */  \
     (((CVMUint32)(offset) & 0xffff8000) == 0xffff8000)) /* -1..-32768 */

#define CVMCPUmemspecIsEncodableAsOpcodeSpecificImmediate(opcode, value) \
    CVMCPUmemspecIsEncodableAsImmediate(value) 

/* MemSpec token encoder APIs: ============================================ */

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMPPCmemspecGetToken(const CVMCPUMemSpec *memSpec);

#define CVMCPUmemspecGetToken(con, memSpec) \
    ((void)(con), CVMPPCmemspecGetToken(memSpec))

/* NOTE: The CVMCPUMemSpecToken is encoded as:
        bits 17 - 16:   memSpec type;
        bits 0 - 15:    immediate offset or index reg;
*/
#define CVMPPCmemspecEncodeToken(type, value /* offset or reg */) \
    (((type) << 16) | ((CVMUint32)(value) & 0xffff))

#define CVMCPUmemspecEncodeRegisterToken(con, reg)			\
    ((void)(con),							\
     CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, reg))

#define CVMCPUmemspecEncodeImmediateToken(con, offset)			\
    ((void)(con),							\
     CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_IMMEDIATE_OFFSET, offset))

#define CVMCPUmemspecEncodePostIncrementImmediateToken(con, increment)	     \
    ((void)(con),							     \
     CVMassert(CVMCPUmemspecIsEncodableAsImmediate(increment)),		     \
     CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET, \
			      increment))

#define CVMCPUmemspecEncodePreDecrementImmediateToken(con, decrement)	    \
    ((void)(con),							    \
     CVMassert(CVMCPUmemspecIsEncodableAsImmediate(decrement)),		    \
     CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET, \
                              decrement))

#define CVMPPCmemspecGetTokenType(memSpecToken) \
    ((memSpecToken) >> 16)

#define CVMPPCmemspecGetTokenRegister(memSpecToken) \
     ((memSpecToken) & 0xffff)

#define CVMPPCmemspecGetTokenOffset(memSpecToken) \
    ((CVMInt32)(memSpecToken << 16) >> 16)  /* sign extended */

/**************************************************************
 * CPU code emitters - The following are implementations of some
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* ===== 32 Bit ALU Emitter APIs ========================================== */

#define CVMCPUemitBinaryALURegister(con, opcode, destRegID, lhsRegID, \
                                    rhsRegID, setcc) \
    CVMCPUemitBinaryALU((con), (opcode), (destRegID), (lhsRegID), \
        CVMPPCalurhsEncodeRegisterToken(rhsRegID), (setcc))

#define CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitMove((con), (opcode), (destRegID), \
        CVMPPCalurhsEncodeRegisterToken(srcRegID), (setcc))

#define CVMCPUemitCompareRegister(con, opcode, condCode, lhsRegID, rhsRegID) \
    CVMCPUemitCompare((con), (opcode), (condCode), (lhsRegID), \
        CVMPPCalurhsEncodeRegisterToken(rhsRegID))

/**************************************************************
 * Misc function prototypes and macros.
 **************************************************************/

/*
 * Encode a BIC immediate using rlwinm. Returns TRUE if encodable.
 * Also returns MB and ME values for rlwinm instruction.
 */
CVMBool
CVMPPCgetEncodableRLWINM(CVMUint8* maskBeginPtr, CVMUint8* maskEndPtr,
			 CVMUint32 constValue);

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
extern CVMCPUInstruction
CVMPPCgetBranchInstruction(CVMCPUCondCode condCode, int offset);

/*
 * Do a PC-relative branch or branch-and-link instruction
 */
extern void
CVMPPCemitBranch(CVMJITCompilationContext* con, int logicalPC,
		 CVMCPUCondCode condCode, CVMBool link, 
                 CVMBool useBLR, CVMJITFixupElement** fixupList);

#define CVMCPUemitBranchNeedFixup(con, logicalPC, condCode, fixupList) \
    CVMPPCemitBranch(con, logicalPC, condCode, \
        CVM_FALSE, /* don't link */            \
        CVM_FALSE, /* blr not needed */        \
        fixupList)

#define CVMCPUemitBranchLinkNeedFixup(con, logicalPC, fixupList) \
    CVMPPCemitBranch(con, logicalPC, CVMCPU_COND_AL, \
        CVM_TRUE,   /* link */                 \
        CVM_FALSE,  /* blr not needed */       \
        fixupList)

#define CVMCPUemitBranch(con, logicalPC, condCode)	\
    CVMCPUemitBranchNeedFixup(con, logicalPC, condCode, NULL)

#define CVMCPUemitBranchLink(con, logicalPC)		\
    CVMCPUemitBranchLinkNeedFixup(con, logicalPC, NULL)

#define CVMCPUemitAbsoluteCall(con, target, okToDumpCp,			\
			       okToBranchAroundCpDump)			\
    CVMCPUemitAbsoluteCallConditional(con, target, okToDumpCp,		\
				      okToBranchAroundCpDump, CVMCPU_COND_AL)

/*
 * Load from a pc-relative location.
 */
void
CVMPPCemitMemoryReferencePCRelative(CVMJITCompilationContext* con, int opcode,
				    int destRegID, int lhsRegID, int offset);

#define CVMCPUemitMemoryReferencePCRelative(con, opcode, destRegID, offset) \
     CVMPPCemitMemoryReferencePCRelative(con, opcode,			    \
					 destRegID, CVMPPC_r0, offset)


/* Purpose:  Loads the CCEE into the specified register. */
#define CVMCPUemitLoadCCEE(con, destRegID)				  \
    CVMJITaddCodegenComment((con, "load CCEE into register"));		  \
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,			  \
                                destRegID, CVMPPC_sp, OFFSET_CStack_CCEE, \
                                CVMJIT_NOSETCC)

/* Purpose:  Load or Store a field of the CCEE. On PowerPC the CCEE
 *           is at a fixed offset from the sp. */
#define CVMCPUemitCCEEReferenceImmediate(con, opcode, regID, offset)	\
    CVMCPUemitMemoryReferenceImmediate(con, opcode,			\
                                       regID, CVMPPC_sp,		\
				       offset + OFFSET_CStack_CCEE)

/*
 * WARNING: using fmadd results in a non-compliant vm. Some floating
 * point tck tests will fail. Therefore, this support if off by defualt.
 */
#ifdef CVM_JIT_USE_FMADD
/* Purpose:  Emit a ternary ALU instruction. */
void
CVMPPCemitTernaryFP(CVMJITCompilationContext* con,
		    int opcode, int regidD,
		    int regidA, int regidC, int regidB);
#endif

#ifdef CVM_PPC_E500V1
/*
 * Purpose: Emit the proper opcode for a floating point test of the
 * given condition.
 */
extern void
CVME500emitFCompare(
    CVMJITCompilationContext* con,
    CVMCPUCondCode condCode,
    int lhsRegID, int rhsRegID);
#endif

#include "javavm/include/jit/jitriscemitter_arch.h"

#endif /* _INCLUDED_JITRISCEMITTER_CPU_H */
