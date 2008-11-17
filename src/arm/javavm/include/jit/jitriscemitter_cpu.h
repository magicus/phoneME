/*
 * @(#)jitriscemitter_cpu.h	1.97 06/10/10
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

#ifndef _INCLUDED_ARM_JITRISCEMITTER_CPU_H
#define _INCLUDED_ARM_JITRISCEMITTER_CPU_H

/*
 * This file #defines all of the emitter function prototypes and macros that
 * shared parts use in a platform indendent way in RISC ports of the jit back
 * end. The exported functions and macros are prefixed with CVMCPU_.  Other
 * functions and macros should be considered private to the ARM specific parts
 * of the jit, including those with the CVMARM_ prefix.
 */

/**********************************************************************
 * Some very ARM-specific bits of code generation.
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
 * debug the resulting code! That would not be desirable.
 */

/**************************************************************
 * CPU ALURhs and associated types - The following are function
 * prototypes and macros for the ALURhs abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

/* Purpose: Constructs an ALURhs to represent a register shifted by a
            constant. */
CVMCPUALURhs*
CVMARMalurhsNewShiftByConstant(CVMJITCompilationContext*, int shiftop,
                               CVMRMResource*, CVMInt32 shiftval);

/* Purpose: Constructs an ALURhs to represent a register shifted by another
            register. */
CVMCPUALURhs*
CVMARMalurhsNewShiftByReg(CVMJITCompilationContext*, int shiftop,
                          CVMRMResource*, CVMRMResource*);

#define CVMCPUalurhsNewRegister(con, rp) \
    CVMARMalurhsNewShiftByConstant(con, CVMARM_NOSHIFT_OPCODE, rp, 0)

#define CVMCPUalurhsIsEncodableAsImmediate(opcode, constValue) \
    (((CVMUint32)(constValue) <= 255) || \
     CVMARMmode1EncodeImmediate((CVMUint32)(constValue), NULL, NULL))

#define CVMCPUalurhsIsConstant(aluRhs) \
    ((aluRhs)->type == CVMARM_ALURHS_CONSTANT)

#define CVMCPUalurhsGetConstantValue(aluRhs)    ((aluRhs)->constValue)

/* ALURhs token encoder APIs: ============================================= */

/* Purpose: Gets the token for the ALURhs value for use in the instruction
            emitters. */
CVMCPUALURhsToken
CVMARMalurhsGetToken(CVMJITCompilationContext* con,
		     const CVMCPUALURhs *aluRhs);

#define CVMCPUalurhsGetToken(con, aluRhs)   \
     ((void)(con), CVMARMalurhsGetToken(con, aluRhs))

/* Purpose: Encodes an CVMCPUALURhsToken for the specified arguments. */
/* NOTE: This function is private to the ARM implementation and can only be
         called from the shared implementation via the CVMCPUxxx macros
         below. */
CVMCPUALURhsToken
CVMARMalurhsEncodeToken(CVMJITCompilationContext* con,
			CVMARMALURhsType type, int constValue,
                        int shiftOp, int r1RegID, int r2RegID);

/* Purpose: Encodes a CVMCPUALURhsToken for the specified constant value. */
#define CVMARMalurhsEncodeConstantToken(con, constValue) \
    (((CVMUint32)(constValue) <= 255) ? \
     (CVMARM_MODE1_CONSTANT | (constValue)) : \
      CVMARMalurhsEncodeToken(con, CVMARM_ALURHS_CONSTANT, (constValue), \
         CVMARM_NOSHIFT_OPCODE, CVMCPU_INVALID_REG, CVMCPU_INVALID_REG))

#ifndef IAI_CODE_SCHEDULER_SCORE_BOARD

/* Purpose: Encodes a CVMCPUALURhsToken for the specified register ID. */
#define CVMARMalurhsEncodeRegisterToken(con, regID) \
     (CVMARM_MODE1_SHIFT_CONSTANT | CVMARM_NOSHIFT_OPCODE | (regID))

/* Purpose: Encodes a ShiftByConstant CVMCPUALURhsToken. */
#define CVMARMalurhsEncodeShiftByConstantToken(con, \
					       regID, shiftOp, shiftAmount) \
    (((shiftAmount) == 0) ? \
     (CVMARM_MODE1_SHIFT_CONSTANT | (regID) | CVMCPU_SLL_OPCODE | 0) : \
     (CVMARM_MODE1_SHIFT_CONSTANT | (regID) | (shiftOp) | (shiftAmount)<<7))

#else  /* IAI_CODE_SCHEDULER_SCORE_BOARD */

/* Purpose: Encodes a CVMCPUALURhsToken for the specified register ID. */
#define CVMARMalurhsEncodeRegisterToken(con, regID) \
     (CVMJITcsPushSourceRegister(con, regID),\
      CVMARM_MODE1_SHIFT_CONSTANT | CVMARM_NOSHIFT_OPCODE | (regID))

/* Purpose: Encodes a ShiftByConstant CVMCPUALURhsToken. */
#define CVMARMalurhsEncodeShiftByConstantToken(con, \
					       regID, shiftOp, shiftAmount) \
    (CVMJITcsPushSourceRegister(con, regID),\
     ((shiftAmount) == 0) ? \
     (CVMARM_MODE1_SHIFT_CONSTANT | (regID) | CVMCPU_SLL_OPCODE | 0) : \
     (CVMARM_MODE1_SHIFT_CONSTANT | (regID) | (shiftOp) | (shiftAmount)<<7))

#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

/**************************************************************
 * CPU MemSpec and associated types - The following are function
 * prototypes and macros for the MemSpec abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* MemSpec query APIs: ==================================================== */

#define CVMARMisMode2Instruction(opcode) ((opcode >> 26) == 0x1)

#define CVMCPUmemspecIsEncodableAsImmediate(value) \
    ((-0xfff <= ((CVMInt32)(value))) && (((CVMInt32)(value)) <= 0xfff))

#define CVMCPUmemspecIsEncodableAsOpcodeSpecificImmediate(opcode, value) \
    (CVMARMisMode2Instruction(opcode) ? \
     CVMCPUmemspecIsEncodableAsImmediate(value) : ((value) <= 255))

#define CVMARMisMode5Instruction(opcode) ((opcode >> 26) == 0x3)

/* MemSpec token encoder APIs: ============================================ */

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMARMmemspecGetToken(CVMJITCompilationContext* con,
		      const CVMCPUMemSpec *memSpec);

#define CVMCPUmemspecGetToken(con, memSpec) \
    ((void)(con), CVMARMmemspecGetToken(con, memSpec))

/* NOTE: The CVMCPUMemSpecToken is encoded as:
        bits 31 - 16:   signed offset or regID;
        bits 15:        unused;
        bits 14 - 13:   shiftOpcode;
        bits 12 - 8:    shiftAmount;
        bits 7 - 0:     memSpec type;
*/

#define CVMARMmemspecEncodeToken(type, offset, shiftOp, shiftImm) \
    (CVMassert((CVMUint32)(shiftImm) <= 31), \
     ((((offset) & 0xffff) << 16) | (((shiftOp) | (shiftImm)) << 8) | (type)))

#define CVMCPUmemspecEncodeImmediateToken(con, offset) \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(offset)), \
      CVMARMmemspecEncodeToken(CVMCPU_MEMSPEC_IMMEDIATE_OFFSET, \
                        (offset), CVMARM_NOSHIFT_OPCODE, 0))

#define CVMCPUmemspecEncodePostIncrementImmediateToken(con, increment) \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(increment)), \
     CVMARMmemspecEncodeToken(CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET, \
                              (increment), CVMARM_NOSHIFT_OPCODE, 0))

#define CVMCPUmemspecEncodePreDecrementImmediateToken(con, decrement) \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(decrement)), \
     CVMARMmemspecEncodeToken(CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET, \
                              (decrement), CVMARM_NOSHIFT_OPCODE, 0))

/**************************************************************
 * CPU code emitters - The following are prototypes of code
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* ===== MemoryReference Emitter APIs ===================================== */

#define CVMCPUemitMemoryReferencePCRelative(con, opcode, destRegID, offset) \
    CVMCPUemitMemoryReferenceImmediate((con), (opcode), \
        (destRegID), CVMARM_PC, (offset - 8))

#define CVMCPUemitMemoryReferenceImmediate(con, opcode, destreg, basereg, \
                                           immOffset) \
    CVMCPUemitMemoryReferenceImmediateConditional((con), (opcode), \
        (destreg), (basereg), (immOffset), CVMCPU_COND_AL)

#define CVMCPUemitMemoryReference(con, opcode, destreg, basereg, memSpecToken)\
    CVMCPUemitMemoryReferenceConditional((con), (opcode), \
        (destreg), (basereg), (memSpecToken), CVMCPU_COND_AL)

/* ===== 32 Bit ALU Emitter APIs ========================================== */

#define CVMCPUemitUnaryALU(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitUnaryALUConditional((con), (opcode), (destRegID), \
        (srcRegID), (setcc), CVMCPU_COND_AL)

#define CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID, \
                            rhstoken, setcc) \
    CVMCPUemitBinaryALUConditional((con), (opcode), (destRegID), \
        (lhsRegID), (rhstoken), (setcc), CVMCPU_COND_AL)

#define CVMCPUemitBinaryALUConstant(con, opcode, destRegID, lhsRegID, \
                                    rhsConstValue, setcc) \
    CVMCPUemitBinaryALUConstantConditional((con), (opcode), (destRegID), \
        (lhsRegID), (rhsConstValue), (setcc), CVMCPU_COND_AL)

#define CVMCPUemitBinaryALURegister(con, opcode, destRegID, lhsRegID, \
                                    rhsRegID, setcc) \
    CVMCPUemitBinaryALU((con), (opcode), (destRegID), (lhsRegID), \
        CVMARMalurhsEncodeRegisterToken(con, rhsRegID), (setcc))

#define CVMCPUemitBinaryALURegisterConditional(con, \
                opcode, destRegID, lhsRegID, rhsRegID, setcc, condCode) \
    CVMCPUemitBinaryALUConditional((con), (opcode), (destRegID), (lhsRegID),\
        CVMARMalurhsEncodeRegisterToken(con, rhsRegID), (setcc), (condCode))

#define CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitMove((con), (opcode), (destRegID), \
        CVMARMalurhsEncodeRegisterToken(con, srcRegID), (setcc))

#define CVMCPUemitMoveRegisterConditional(con, \
                opcode, destRegID, srcRegID, setcc, condCode) \
    CVMCPUemitMoveConditional((con), (opcode), (destRegID), \
        CVMARMalurhsEncodeRegisterToken(con, srcRegID), (setcc), (condCode))

#define CVMCPUemitCompareRegister(con, opcode, condCode, lhsRegID, rhsRegID) \
    CVMCPUemitCompare((con), (opcode), (condCode), (lhsRegID), \
        CVMARMalurhsEncodeRegisterToken(con, rhsRegID))

#define CVMCPUemitMove(con, opcode, destRegID, srcToken, setcc) \
    CVMCPUemitMoveConditional((con), (opcode), (destRegID), \
        (srcToken), (setcc), CVMCPU_COND_AL)

/* ===== Branch Emitter APIs ========================================== */

extern void
CVMARMemitAbsoluteCallConditional(CVMJITCompilationContext* con,
                                  const void* target,
                                  CVMBool okToDumpCp,
                                  CVMBool okToBranchAroundCpDump,
                                  CVMCPUCondCode condCode,
                                  CVMBool flushReturnPCToFrame);

#define CVMCPUemitAbsoluteCall(con, target, okToDumpCp,			\
			       okToBranchAroundCpDump)			\
    CVMARMemitAbsoluteCallConditional((con), (target), (okToDumpCp),	\
				      (okToBranchAroundCpDump),		\
				      CVMCPU_COND_AL, CVM_FALSE)

#define CVMCPUemitAbsoluteCallConditional(con, target, okToDumpCp,      \
                                          okToBranchAroundCpDump, condCode) \
    CVMARMemitAbsoluteCallConditional((con), (target), (okToDumpCp),    \
                                      (okToBranchAroundCpDump),         \
                                      (condCode), CVM_FALSE)

extern void
CVMARMemitBranch(CVMJITCompilationContext* con,
	   int logicalPC, CVMCPUCondCode condCode, 
           CVMBool link, CVMJITFixupElement** fixupList);

#define CVMCPUemitBranchNeedFixup(con, target, condCode, fixupList) \
    CVMARMemitBranch((con), (target), (condCode),                   \
                     CVM_FALSE, (fixupList))

#define CVMCPUemitBranchLinkNeedFixup(con, target, fixupList)       \
    CVMARMemitBranch((con), (target), CVMCPU_COND_AL,               \
                     CVM_TRUE, (fixupList))

#define CVMCPUemitBranch(con, target, condCode)                     \
    CVMCPUemitBranchNeedFixup((con), (target), (condCode), NULL)

#define CVMCPUemitBranchLink(con, target)                           \
    CVMCPUemitBranchLinkNeedFixup((con), (target), NULL)

/* ===== Misc Emitter APIs ================================================ */

/* Purpose:  Loads the CCEE into the specified register. */
#define CVMCPUemitLoadCCEE(con, destRegID) \
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE, destRegID, CVMARM_sp, \
                           CVMJIT_NOSETCC)

/* Purpose:  Load or Store a field of the CCEE. On ARM the CCEE
 *           is the same as sp. */
#define CVMCPUemitCCEEReferenceImmediate(con, opcode, regID, offset)	\
    CVMCPUemitMemoryReferenceImmediate(con, opcode,			\
                                       regID, CVMARM_sp, offset)

/**************************************************************
 * Misc function prototypes and macros.
 **************************************************************/

/* Purpose: Checks to see if a large constant can be encoded using mode1
            rotate right immediates. */
CVMBool
CVMARMmode1EncodeImmediate(CVMUint32 value, CVMUint32 *baseValue,
                           CVMUint32 *rotateValue);

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
CVMCPUInstruction
CVMARMgetBranchInstruction(CVMCPUCondCode condCode, int offset, CVMBool link);

/**************************************************************
 * Memory barrier emitters
 **************************************************************/
#ifdef CVM_MP_SAFE
#define CVMCPUemitMemBarAcquire(con) \
    CVMCPUemitMemBar(con)

#define CVMCPUemitMemBarRelease(con) \
    CVMCPUemitMemBar(con)
#endif

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
extern CVMJITRegsRequiredType
CVMARMCCALLgetRequired(CVMJITCompilationContext *con,
                       CVMJITRegsRequiredType argsRequired,
		       CVMJITIRNode *intrinsicNode,
		       CVMJITIntrinsic *irec,
		       CVMBool useRegArgs);

/* Purpose: Dynamically instantiates an instance of the CVMCPUCallContext. */
#define CVMCPUCCallnewContext(con) \
    ((CVMCPUCallContext *)CVMJITmemNew(con, JIT_ALLOC_CGEN_OTHER, \
                                       sizeof(CVMCPUCallContext)))

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
#define CVMCPUCCALLgetRequired(con, argsRequired, node, irec, useRegArgs) \
    useRegArgs? /* IAI-22 */ \
        CVMARMCCALLgetRequired(con, argsRequired, node, irec, useRegArgs): \
        (CVMCPU_AVOID_C_CALL | argsRequired)

#define CVMARMCCALLargSize(argType) \
    (((argType == CVM_TYPEID_LONG) || (argType == CVM_TYPEID_DOUBLE)) ? 2 : 1)

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. */
#define CVMCPUCCALLinitArgs(con, callContext, irec, forTargetting,     \
                            useRegArgs) {                              \
    int numberOfArgsWords = (irec)->numberOfArgsWords;                 \
    (callContext)->reservedRes = NULL;                                 \
    /* NOTE: We assume that if the intrinsic is using RegArgs, then    \
       we are guaranteed that there are enough registers to use as     \
       args.  This is because CVMJITINTRINSIC_REG_ARGS will only be    \
       declared for target specific intrinsics.  Common code cannot    \
       use this calling convention because it has no knowledge of the  \
       convention implementation details, and hence can't use it. */   \
    if (!(forTargetting) && !(useRegArgs) &&                           \
        (numberOfArgsWords > CVMCPU_MAX_ARG_REGS)) {                   \
        int stackWords = numberOfArgsWords - CVMCPU_MAX_ARG_REGS;      \
	stackWords = (stackWords + 1) & ~1; /* align for AAPCS */      \
        stackWords *= 4;                                               \
        CVMCPUemitBinaryALUConstant((con), CVMCPU_SUB_OPCODE,          \
            CVMCPU_SP_REG, CVMCPU_SP_REG, stackWords, CVMJIT_NOSETCC); \
    }                                                                  \
}

/* Purpose: Gets the register targets for the specified argument. */
#define CVMCPUCCALLgetArgTarget(con, callContext, argType, argNo, \
                                argWordIndex, useRegArgs) \
    ((argWordIndex + CVMARMCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) ? \
      (1U << (CVMCPU_ARG1_REG + argWordIndex)) :                           \
      useRegArgs ? /* IAI-22 */                                            \
        (1U << (CVMARM_v3 + argWordIndex - CVMCPU_MAX_ARG_REGS)) :       \
        CVMRM_ANY_SET)

/* Purpose: Relinquish a previously pinned arguments. */
#define CVMCPUCCALLrelinquishArg(con, callContext, arg, argType, argNo, \
                                 argWordIndex, useRegArgs)              \
    if ((useRegArgs) /* IAI-22 */ ||                                    \
        argWordIndex + CVMARMCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) { \
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);              \
    } else if ((callContext)->reservedRes != NULL) {                    \
        CVMRMrelinquishResource(CVMRM_INT_REGS(con),			\
				(callContext)->reservedRes);       	\
        (callContext)->reservedRes = NULL;                              \
    }

/* Purpose: Releases any resources allocated in CVMCPUCCALLinitArgs(). */
#define CVMCPUCCALLdestroyArgs(con, callContext, irec, forTargetting,  \
                               useRegArgs) {                           \
    int numberOfArgsWords = (irec)->numberOfArgsWords;                 \
    ((void)callContext);                                               \
    if (!(forTargetting) && !(useRegArgs) &&                           \
        (numberOfArgsWords > CVMCPU_MAX_ARG_REGS)) {                   \
        int stackWords = numberOfArgsWords - CVMCPU_MAX_ARG_REGS;      \
	stackWords = (stackWords + 1) & ~1; /* align for AAPCS */      \
        stackWords *= 4;                                               \
        CVMCPUemitBinaryALUConstant((con), CVMCPU_ADD_OPCODE,          \
            CVMCPU_SP_REG, CVMCPU_SP_REG, stackWords, CVMJIT_NOSETCC); \
    }                                                                  \
}

#ifdef CVM_JIT_USE_FP_HARDWARE
/* Purpose: Emits instructions to move register contents 
 *          to and from fp registers */
void
CVMARMemitMoveFloatFP(CVMJITCompilationContext* con,
                 int opcode, int destRegID, int srcRegID);

void
CVMARMemitMoveDoubleFP(CVMJITCompilationContext* con,
                 int opcode, int fpRegID, int regID);

/* Purpose: Emits instructions to move system register contents 
 *          to and from ARM registers */
void
CVMARMemitStatusRegisterFP(CVMJITCompilationContext* con,
                 int opcode, int statusReg, int regID);
#endif /* CVM_JIT_USE_FP_HARDWARE */

#endif /* _INCLUDED_ARM_JITRISCEMITTER_CPU_H */
