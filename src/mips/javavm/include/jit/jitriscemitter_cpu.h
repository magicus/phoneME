/*
 * @(#)jitriscemitter_cpu.h	1.20 06/10/10
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
 * functions and macros should be considered private to the MIPS 
 * specific parts of the jit, including those with the CVMMIPS_ prefix.
 */

/**********************************************************************
 * Some very MIPS-specific bits of code generation.
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
 * debug the resulting code! This would not be desirable.
 */

/**************************************************************
 * CPU ALURhs and associated types - The following are function
 * prototypes and macros for the ALURhs abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

#define CVMCPUalurhsIsConstant(aluRhs) \
    ((aluRhs)->type == CVMMIPS_ALURHS_CONSTANT)

#define CVMCPUalurhsGetConstantValue(aluRhs)    ((aluRhs)->constValue)

/* ALURhs token encoder APIs:============================================= */

/* Purpose: Gets the token for the ALURhs value for use in the instruction
            emitters. */
CVMCPUALURhsToken
CVMMIPSalurhsGetToken(const CVMCPUALURhs *aluRhs);

#define CVMCPUalurhsGetToken(con, aluRhs)   \
     ((void)(con), CVMMIPSalurhsGetToken(aluRhs))

/* NOTE: The CVMCPUALURhsToken is encoded as:
        bits 17 - 16:   CVMMIPSALURhsType;
        bits 0 - 15:    immediate value or reg;
*/

/* Purpose: Encodes an CVMCPUALURhsToken for the specified arguments. */
/* NOTE: This function is private to the MIPS implementation and can only be
         called from the shared implementation via the CVMCPUxxx macros
         below. */

/* Purpose: Encodes a CVMCPUALURhsToken for the specified constant value. */
#define CVMMIPSalurhsEncodeConstantToken(constValue)			\
    (CVMMIPS_ALURHS_CONSTANT << 16 | ((constValue) & 0xffff))

#define CVMMIPSalurhsEncodeRegisterToken(regID) \
    (CVMMIPS_ALURHS_REGISTER << 16 | (regID))

#define CVMMIPSalurhsDecodeGetTokenType(token) \
    (token >> 16)

#define CVMMIPSalurhsDecodeGetTokenConstant(token) \
    ((CVMInt32)(token << 16) >> 16)  /* sign extended */

#define CVMMIPSalurhsDecodeGetTokenRegister(token) \
    (token & 0xffff)

/**************************************************************
 * CPU MemSpec and associated types - The following are function
 * prototypes and macros for the MemSpec abstraction required by
 * the RISC emitter porting layer.
 *
 * NOTE: MIPS has only one load/store addressing mode:
 *
 *           base_reg + sign_extend(offset_16)
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
CVMMIPSmemspecGetToken(const CVMCPUMemSpec *memSpec);

#define CVMCPUmemspecGetToken(con, memSpec) \
    ((void)(con), CVMMIPSmemspecGetToken(memSpec))

/* NOTE: The CVMCPUMemSpecToken is encoded as:
        bits 17 - 16:   memSpec type;
        bits 0 - 15:    immediate offset or index reg;
*/
#define CVMMIPSmemspecEncodeToken(type, value /* offset or reg */) \
    (((type) << 16) | ((CVMUint32)(value) & 0xffff))

#define CVMCPUmemspecEncodeRegisterToken(con, reg)			\
    ((void)(con),							\
     CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, reg))

#define CVMCPUmemspecEncodeImmediateToken(con, offset)			\
    ((void)(con),							\
     CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_IMMEDIATE_OFFSET, offset))

#define CVMCPUmemspecEncodePostIncrementImmediateToken(con, increment)	     \
    ((void)(con),							     \
     CVMassert(CVMCPUmemspecIsEncodableAsImmediate(increment)),		     \
     CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET, \
			      increment))

#define CVMCPUmemspecEncodePreDecrementImmediateToken(con, decrement)	    \
    ((void)(con),							    \
     CVMassert(CVMCPUmemspecIsEncodableAsImmediate(decrement)),		    \
     CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET, \
                              decrement))

#define CVMMIPSmemspecGetTokenType(memSpecToken) \
    ((memSpecToken) >> 16)

#define CVMMIPSmemspecGetTokenRegister(memSpecToken) \
     ((memSpecToken) & 0xffff)

#define CVMMIPSmemspecGetTokenOffset(memSpecToken) \
    ((CVMInt32)(memSpecToken << 16) >> 16)  /* sign extended */

/**************************************************************
 * CPU code emitters - The following are implementations of some
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* ===== 32 Bit ALU Emitter APIs ========================================== */

#define CVMCPUemitBinaryALURegister(con, opcode, destRegID, lhsRegID, \
                                    rhsRegID, setcc)                  \
    CVMCPUemitBinaryALU((con), (opcode), (destRegID), (lhsRegID),     \
        CVMMIPSalurhsEncodeRegisterToken(rhsRegID), (setcc))

#define CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitMove((con), (opcode), (destRegID),                        \
        CVMMIPSalurhsEncodeRegisterToken(srcRegID), (setcc))

extern void
CVMMIPSemitMul(CVMJITCompilationContext* con, int opcode,
              int destreg, int lhsreg, int rhsreg,
              int extrareg, CVMBool needNop);

#define CVMCPUemitMul(con, opcode, destreg, lhsreg, rhsreg, extrareg) \
    CVMMIPSemitMul((con), (opcode), (destreg), (lhsreg), (rhsreg),    \
                   (extrareg), CVM_TRUE /* need nop's */ )

extern void
CVMMIPSemitCompare(CVMJITCompilationContext* con,
                   int opcode, CVMCPUCondCode condCode, int lhsRegID,
		   int rhsRegID, CVMInt32 rhsConst, int cmpRhsType);

#define CVMCPUemitCompareRegister(con, opcode, condCode, lhsRegID, rhsRegID) \
    CVMMIPSemitCompare(con, opcode, condCode, lhsRegID,			     \
		       rhsRegID, 0, CVMMIPS_ALURHS_REGISTER);

#define CVMCPUemitCompareConstant(con, opcode, condCode, lhsRegID, rhsConst) \
    CVMMIPSemitCompare(con, opcode, condCode, lhsRegID,			     \
		       CVMCPU_INVALID_REG, rhsConst, CVMMIPS_ALURHS_CONSTANT);

/**************************************************************
 * Misc function prototypes and macros.
 **************************************************************/

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
extern CVMCPUInstruction
CVMMIPSgetBranchInstruction(CVMCPUCondCode condCode, int offset);

/*
 * Do a PC-relative branch or branch-and-link instruction
 */
extern void
CVMMIPSemitBranch(CVMJITCompilationContext* con, int logicalPC,
	 	  CVMCPUCondCode condCode, CVMBool link,
                  CVMJITFixupElement** fixupList);

#define CVMCPUemitBranchNeedFixup(con, logicalPC, condCode, fixupList) \
    CVMMIPSemitBranch(con, logicalPC, condCode,		\
		      CVM_FALSE /* don't link */,       \
                      fixupList)

#define CVMCPUemitBranchLinkNeedFixup(con, logicalPC, fixupList)       \
    CVMMIPSemitBranch(con, logicalPC, CVMCPU_COND_AL,	\
		      CVM_TRUE /* link */,              \
                      fixupList)

#define CVMCPUemitBranch(con, logicalPC, condCode)	\
    CVMCPUemitBranchNeedFixup(con, logicalPC, condCode, NULL)

#define CVMCPUemitBranchLink(con, logicalPC)		\
    CVMCPUemitBranchLinkNeedFixup(con, logicalPC, NULL)

/*
 * Load from a pc-relative location.
 */
void
CVMMIPSemitMemoryReferencePCRelative(CVMJITCompilationContext* con, int opcode,
		 		     int destRegID, int lhsRegID, int offset);

#define CVMCPUemitMemoryReferencePCRelative(con, opcode, destRegID, offset) \
     CVMMIPSemitMemoryReferencePCRelative(con, opcode,			    \
					 destRegID, CVMMIPS_r0, offset)


/* Purpose:  Loads the CCEE into the specified register. */
#define CVMCPUemitLoadCCEE(con, destRegID)				  \
    CVMJITaddCodegenComment((con, "load CCEE into register"));		  \
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,			  \
                                destRegID, CVMMIPS_sp, OFFSET_CStack_CCEE, \
                                CVMJIT_NOSETCC)

/* Purpose:  Load or Store a field of the CCEE. On MIPS the CCEE
 *           is at a fixed offset from the sp. */
#define CVMCPUemitCCEEReferenceImmediate(con, opcode, regID, offset)	\
    CVMCPUemitMemoryReferenceImmediate(con, opcode,			\
                                       regID, CVMMIPS_sp,		\
				       offset + OFFSET_CStack_CCEE)


/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#if CVMCPU_MAX_ARG_REGS != 8

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
extern CVMJITRegsRequiredType
CVMMIPSCCALLgetRequired(CVMJITCompilationContext *con,
                        CVMJITRegsRequiredType argsRequired,
                        CVMJITIRNode *intrinsicNode,
                        CVMJITIntrinsic *irec,
                        CVMBool useRegArgs);

/* Purpose: Dynamically instantiates an instance of the CVMCPUCallContext. */
#define CVMCPUCCallnewContext(con)                                \
    ((CVMCPUCallContext *)CVMJITmemNew(con, JIT_ALLOC_CGEN_OTHER,       \
                                       sizeof(CVMCPUCallContext)))

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
#define CVMCPUCCALLgetRequired(con, argsRequired, node, irec, useRegArgs) \
    useRegArgs? /* IAI-22 */ \
        CVMMIPSCCALLgetRequired(con, argsRequired, node, irec, useRegArgs): \
        (CVMCPU_AVOID_C_CALL | argsRequired)

#define CVMMIPSCCALLargSize(argType) \
    (((argType == CVM_TYPEID_LONG) || (argType == CVM_TYPEID_DOUBLE)) ? 2 : 1)

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. Unlike ARM, there
            is no need to emit a stack adjustment.
*/
#define CVMCPUCCALLinitArgs(con, callContext, irec, forTargetting,     \
                            useRegArgs) {                              \
    (callContext)->reservedRes = NULL;                                 \
}

/* Purpose: Gets the register targets for the specified argument. */
#define CVMCPUCCALLgetArgTarget(con, callContext, argType, argNo, \
                                argWordIndex, useRegArgs) \
    ((argWordIndex + CVMMIPSCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) ? \
      (1U << (CVMCPU_ARG1_REG + argWordIndex)) :                           \
      useRegArgs ? /* IAI-22 */                                            \
        (1U << (CVMMIPS_t0 + argWordIndex - CVMCPU_MAX_ARG_REGS)) :       \
        CVMRM_ANY_SET)

/* Purpose: Relinquish a previously pinned arguments. */
#define CVMCPUCCALLrelinquishArg(con, callContext, arg, argType, argNo, \
                                 argWordIndex, useRegArgs)              \
    if ((useRegArgs) /* IAI-22 */ ||                                    \
        argWordIndex + CVMMIPSCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) { \
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);              \
    } else if ((callContext)->reservedRes != NULL) {                    \
        CVMRMrelinquishResource(CVMRM_INT_REGS(con),                    \
                                (callContext)->reservedRes);            \
        (callContext)->reservedRes = NULL;                              \
    }

/* Purpose: Releases any resources allocated in CVMCPUCCALLinitArgs().
            Unlike ARM, there is nothing for us to do here since we did
            not need to emit any stack adjustment, so this function
            is a nop.
*/
#define CVMCPUCCALLdestroyArgs(con, callContext, irec, forTargetting,   \
                               useRegArgs) {                            \
    ((void)useRegArgs);                                                 \
    ((void)callContext);                                                \
}

#endif /* CVMCPU_MAX_ARG_REGS != 8 */

#endif /* _INCLUDED_JITRISCEMITTER_CPU_H */
