/*
 * @(#)jitriscemitter.h	1.81 06/10/10
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

#ifndef _INCLUDED_JITRISCEMITTER_H
#define _INCLUDED_JITRISCEMITTER_H

#include "javavm/include/objects.h"
#include "javavm/include/classes.h"

#include "javavm/include/porting/jit/jit.h"
#include "portlibs/jit/risc/include/export/jitregman.h"
#include "javavm/include/jit/jitriscemitterdefs_cpu.h"

#ifdef CVM_HDR_JIT_JITRISCEMITTER_H
#include CVM_HDR_JIT_JITRISCEMITTER_H
#endif

/*
 * This file defines the RISC emitter porting layer which includes opaque
 * data structures and function APIs which the platform needs to provide
 * in 2 files: jitriscemitterdefs_cpu.h and jitriscemitter_cpu.h.
 * jitriscemitterdefs_cpu.h will be included at the top of this file and
 * jitriscemitter_cpu.h will be included at the bottom.
 *
 * jitriscemitterdefs_cpu.h is responsible for defining the opaque data
 * structures and option #defines required by this file, as well as
 * defining data structures and #define values for use in the platform
 * specific grammar file.
 *
 * jitriscemitter_cpu.h is responsible for overriding emitter functions
 * with macros as needed as well as declaring platform specific emitter
 * functions and macros for use in the platform specific grammar file.
 */

/**************************************************************
 * CPU condition codes - The following are definition of the condition
 * codes that are required by the RISC emitter porting layer.
 **************************************************************

    CVMCPUCondCode - opaque type encapsulating condition codes.  The enum
                     list below outlines the complete set of condition codes
                     that may need to be supported.  The common RISC code only
                     makes use of a subset of these condition codes.  However,
                     the complete set is listed here for completeness.

    typedef enum CVMCPUCondCode {
        CVMCPU_COND_EQ,     // Do when equal
        CVMCPU_COND_NE,     // Do when NOT equal
        CVMCPU_COND_MI,     // Do when has minus / negative
        CVMCPU_COND_PL,     // Do when has plus / positive or zero
        CVMCPU_COND_OV,     // Do when overflowed
        CVMCPU_COND_NO,     // Do when NOT overflowed
        CVMCPU_COND_LT,     // Do when signed less than
        CVMCPU_COND_GT,     // Do when signed greater than
        CVMCPU_COND_LE,     // Do when signed less than or equal
        CVMCPU_COND_GE,     // Do when signed greater than or equal
        CVMCPU_COND_LO,     // Do when lower / unsigned less than
        CVMCPU_COND_HI,     // Do when higher / unsigned greater than
        CVMCPU_COND_LS,     // Do when lower or same / is unsigned <=
        CVMCPU_COND_HS,     // Do when higher or same / is unsigned >=
        CVMCPU_COND_AL,     // Do always
        CVMCPU_COND_NV,     // Do never

#ifdef CVM_JIT_USE_FP_HARDWARE
        CVMCPU_COND_FEQ,    // Do when float equal
	CVMCPU_COND_FNE,    // Do when float NOT equal
	CVMCPU_COND_FLT,    // Do when float less than
	CVMCPU_COND_FGT,    // Do when float unordered or greater than
	CVMCPU_COND_FLE,    // Do when float less than or equal
	CVMCPU_COND_FGE    // Do when float unordered or greater than or equal

    // by default, unordered compares are treated at greater than. If the
    // UNORDERED_LT bit is set on the condition code, then unordered
    // is treated as less than.
    #define CVMCPU_COND_UNORDERED_LT	<bitmask>
#endif

    } CVMCPUCondCode;

    #ifdef CVM_JIT_USE_FP_HARDWARE
    #endif

    CVMCPUCondCode and its various values should be defined in
    jitriscemitterdefs_cpu.h.
*/

/**************************************************************
 * CPU Opcodes - The following are definition of opcode encodings
 * that are required by the RISC emitter porting layer.  Where
 * actual opcodes do not exists, pseudo opcodes are substituted.
 **************************************************************

    The platform need to provide definitions of the following opcode values.
    The operation that the opcode represents is also documented below.

    CVMCPU_NOP_INSTRUCTION     // do a no-op

    // Memory Reference opcodes:
    CVMCPU_LDR64_OPCODE     // Load signed 64 bit value.
    CVMCPU_STR64_OPCODE     // Store 64 bit value.
    CVMCPU_LDR32_OPCODE     // Load signed 32 bit value.
    CVMCPU_STR32_OPCODE     // Store 32 bit value.
    CVMCPU_LDR16U_OPCODE    // Load unsigned 16 bit value.
    CVMCPU_LDR16_OPCODE     // Load signed 16 bit value.
    CVMCPU_STR16_OPCODE     // Store 16 bit value.
    CVMCPU_LDR8_OPCODE      // Load signed 8 bit value.
    CVMCPU_STR8_OPCODE      // Store 8 bit value.

    // 32 bit ALU opcodes:
    CVMCPU_MOV_OPCODE       // reg32 = aluRhs32.
    CVMCPU_NEG_OPCODE       // reg32 = -reg32.
    CVMCPU_NOT_OPCODE       // reg32 = (reg32 == 0)?1:0.
    CVMCPU_INT2BIT_OPCODE   // reg32 = (reg32 != 0)?1:0.
    CVMCPU_ADD_OPCODE       // reg32 = reg32 + aluRhs32.
    CVMCPU_SUB_OPCODE       // reg32 = reg32 - aluRhs32.
    CVMCPU_AND_OPCODE       // reg32 = reg32 AND aluRhs32.
    CVMCPU_ORR_OPCODE       // reg32 = reg32 OR aluRhs32.
    CVMCPU_XOR_OPCODE       // reg32 = reg32 XOR aluRhs32.
    CVMCPU_BIC_OPCODE       // reg32 = reg32 AND ~aluRhs32.
    CVMCPU_SLL_OPCODE       // Do logical left shift on a reg32.
    CVMCPU_SRL_OPCODE       // Do logical right shift on a reg32.
    CVMCPU_SRA_OPCODE       // Do arithmetic right shift on a reg32.

    CVMCPU_MULL_OPCODE      // reg32 = LO32(reg32 * reg32).
    CVMCPU_MULH_OPCODE      // reg32 = HI32(reg32 * reg32).
    CVMCPU_CMP_OPCODE       // cmp reg32, aluRhs32 => set cc.
    CVMCPU_CMN_OPCODE       // cmp reg32, -aluRhs32 => set cc.

    // 64 bit ALU opcodes:
    CVMCPU_NEG64_OPCODE     // reg64 = -reg64.
    CVMCPU_ADD64_OPCODE     // reg64 = reg64 + reg64.
    CVMCPU_SUB64_OPCODE     // reg64 = reg64 - reg64.
    CVMCPU_AND64_OPCODE     // reg64 = reg64 AND reg64.
    CVMCPU_OR64_OPCODE      // reg64 = reg64 OR reg64.
    CVMCPU_XOR64_OPCODE     // reg64 = reg64 XOR reg64.
    CVMCPU_SLL64_OPCODE     // Do logical left shift on a reg64.
    CVMCPU_SRL64_OPCODE     // Do logical right shift on a reg64.
    CVMCPU_SRA64_OPCODE     // Do arithmetic right shift on a reg64.

    CVMCPU_MUL64_OPCODE     // reg64 = reg64 * reg64.
    CVMCPU_DIV64_OPCODE     // reg64 = reg64 / reg64.
    CVMCPU_REM64_OPCODE     // reg64 = reg64 % reg64.
    CVMCPU_CMP64_OPCODE     // cmp reg64, reg64 => set cc.

    If floating-point hardware use used by setting CVM_JIT_USE_FP_HARDWARE
    then the following must also be defined:

    CVMCPU_FLDR64_OPCODE     // Load 64 bit value to float register.
    CVMCPU_FSTR64_OPCODE     // Store 64 bit value from float register.
    CVMCPU_FLDR32_OPCODE     // Load 32 bit value to float register.
    CVMCPU_FSTR32_OPCODE     // Store 32 bit value from float register.
    // in shared code, these will only be used to move values of
    // types 'double' and 'float' respectively.

    CVMCPU_FADD_OPCODE	     // single float freg32 = freg32 + freg32
    CVMCPU_FSUB_OPCODE	     // single float freg32 = freg32 - freg32
    CVMCPU_FMUL_OPCODE	     // single float freg32 = freg32 * freg32
    CVMCPU_FDIV_OPCODE	     // single float freg32 = freg32 / freg32
    CVMCPU_FNEG_OPCODE	     // single float freg32 = - freg32
    CVMCPU_FMOV_OPCODE	     // single float freg32 = freg32
    CVMCPU_FCMP_OPCODE	     // single float compare freg32, freg32 => set cc.

    CVMCPU_DADD_OPCODE	     // double float freg64 = freg64 + freg64
    CVMCPU_DSUB_OPCODE	     // double float freg64 = freg64 - freg64
    CVMCPU_DMUL_OPCODE	     // double float freg64 = freg64 * freg64
    CVMCPU_DDIV_OPCODE	     // double float freg64 = freg64 / freg64
    CVMCPU_DNEG_OPCODE	     // double float freg64 = - freg64
    CVMCPU_DCMP_OPCODE	     // double float compare freg64, freg64 => set cc.

    The opcode encodings should be defined in jitriscemitterdefs_cpu.h.
*/

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the ALURhs abstraction required by the RISC emitter porting layer.
 **************************************************************

    The ALURhs is an abstraction of right hand side operands for ALU type
    instructions.  The platform need to provide definitions of the ALURhs
    abstraction in the form of the following types:

        CVMCPUALURhs        - The base class of all ALURhs for the CPU.
                              CVMCPUALURhs is a field in the code-gen time
                              semantic stack element.
        CVMCPUALURhsToken   - A token encoded from the content of an
                              ALUrhs.  This token is used as arguments for
                              ALU code emitters.

        CVMCPUALURhsTokenConstZero - An instance of CVMCPUALURhsToken
                                     representing the an ALURhs of a constant
                                     value 0.

    There are 2 standard types of ALURhs that is expected by the common risc
    code:
        1. Constant
        2. Register

    It is assumed that immediate values of 0 to 255 (inclusive) are always
    encodable as ALURhs constants.

    The above should be defined in jitriscemitterdefs_cpu.h.

    The following are prototypes of ALURhs functions that need to be
    implemented by the platform.  The platform can choose to redirect these
    functions with macros.  If so, the re-direction macros should be defined
    in jitriscemitter_cpu.h.
*/

/* ALURhs constructors and query APIs: ==================================== */

/* Purpose: Constructs a constant type CVMCPUALURhs. */
extern CVMCPUALURhs*
CVMCPUalurhsNewConstant(CVMJITCompilationContext*, CVMInt32);

/* Purpose: Constructs a register type CVMCPUALURhs. */
extern CVMCPUALURhs*
CVMCPUalurhsNewRegister(CVMJITCompilationContext*, CVMRMResource* regID);

/* Purpose: Checks to see if the specified value is encodable as an immediate
            value that can be used as an ALURhs in an instruction. */
extern CVMBool
CVMCPUalurhsIsEncodableAsImmediate(int opcode, CVMInt32 constValue);

/* Purpose: Checks if the CVMCPUALURhs operand is of a const value type. */
extern CVMBool
CVMCPUalurhsIsConstant(CVMCPUALURhs *aluRhs);

/* Purpose: Gets the const value that of a const value type CVMCPUALURhs.
            This function only applies if the CVMCPUALURhs is of a const value
            type. */
extern CVMInt32
CVMCPUalurhsGetConstantValue(CVMCPUALURhs *aluRhs);

/* ALURhs token encoder APIs: ============================================= */

/* Purpose: Gets the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */
extern CVMCPUALURhsToken
CVMCPUalurhsGetToken(CVMJITCompilationContext *, const CVMCPUALURhs *);

/* ALURhs resource management APIs: ======================================= */

/* Purpose: Pins the resources that this CVMCPUALURhs may use. */
extern void
CVMCPUalurhsPinResource(CVMJITRMContext*, int opcode, CVMCPUALURhs*,
                        CVMRMregset target, CVMRMregset avoid);

/* Purpose: Relinquishes the resources that this CVMCPUALURhsToken may use. */
extern void
CVMCPUalurhsRelinquishResource(CVMJITRMContext*, CVMCPUALURhs*);


/**************************************************************
 * CPU MemSpec and associated types - The following are definitions
 * of the MemSpec abstraction required by the RISC emitter porting layer.
 **************************************************************

     The MemSpec is an abstraction of memory specification operands for memory
     reference instructions.  The memory specification defines modifications
     on the base register before a memory reference is made.  The platform
     need to provide definitions of the MemSpec abstraction in the form of the
     following types:

        CVMCPUMemSpec       - The base class of all MemSpec for the CPU.
                              CVMCPUMemSpec is a field in the code-gen time
                              semantic stack element.
        CVMCPUMemSpecToken  - A token encoded from the content of a MemSpec.
                              This token is used as arguments for memory
                              reference (i.e. load/store) code emitters.


    The platform should define encodings for the following MemSpec types.
    The modification on the baseReg (for each of these types) as used during
    a memory reference is also documented below.

    CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
        targetAddress = baseReg + #immediateOffset.

    CVMCPU_MEMSPEC_REG_OFFSET:
        targetAddress = baseReg + offsetReg.

    CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
        targetAddress = baseReg.
        baseReg = baseReg + #immediateOffset.

    CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
        targetAddress = baseReg - #immediateOffset.
        baseReg = targetAddress.

    It is assumed that immediate values of 0 to 255 (inclusive) are always
    encodable as MemSpec immediate offsets.

    The above should be defined in jitriscemitterdefs_cpu.h.

    The following are prototypes of MemSpec functions that need to be
    implemented by the platform.  The platform can choose to redirect these
    functions with macros.  If so, the re-direction macros should be defined
    in jitriscemitter_cpu.h.
*/

/* MemSpec constructors and query APIs: =================================== */

/* Purpose: Constructs a CVMCPUMemSpec immediate operand. */
extern CVMCPUMemSpec *
CVMCPUmemspecNewImmediate(CVMJITCompilationContext *con, CVMInt32 value);

/* Purpose: Constructs a CVMCPUMemSpec register operand. */
extern CVMCPUMemSpec *
CVMCPUmemspecNewRegister(CVMJITCompilationContext *con,
                         CVMBool offsetIsToBeAdded, CVMRMResource *offsetReg);

/* Purpose: Checks to see if the specified value is encodable as an immediate
            value that can be used as a MemSpec in an instruction. */
extern CVMBool
CVMCPUmemspecIsEncodableAsImmediate(CVMInt32 constValue);

/* Purpose: Checks to see if the specified value is encodable as an immediate
            value that can be used as a MemSpec for the specified opcode. */
extern CVMBool
CVMCPUmemspecIsEncodableAsOpcodeSpecificImmediate(int opcode,
                                                  CVMInt32 constValue);

/* MemSpec token encoder APIs: ============================================ */

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified MemSpec. */
extern CVMCPUMemSpecToken
CVMCPUmemspecGetToken(CVMJITCompilationContext *, const CVMCPUMemSpec *);

/* Purpose: Encodes an immediate offset CVMCPUMemSpecToken. */
extern CVMCPUMemSpecToken
CVMCPUmemspecEncodeImmediateToken(CVMJITCompilationContext *, CVMInt32 offset);

/* Purpose: Encodes a post-increment immediate offset CVMCPUMemSpecToken. */
extern CVMCPUMemSpecToken
CVMCPUmemspecEncodePostIncrementImmediateToken(CVMJITCompilationContext *con,
                                               CVMInt32 increment);

/* Purpose: Encodes a pre-decrement immediate offset CVMCPUMemSpecToken. */
extern CVMCPUMemSpecToken
CVMCPUmemspecEncodePreDecrementImmediateToken(CVMJITCompilationContext *con,
                                              CVMInt32 decrement);

/* MemSpec resource management APIs: ====================================== */

/* Purpose: Pins the resources that this CVMCPUMemSpec may use. */
extern void
CVMCPUmemspecPinResource(CVMJITRMContext *con, CVMCPUMemSpec *self,
                         CVMRMregset target, CVMRMregset avoid);

/* Purpose: Relinquishes the resources that this CVMCPUMemSpec may use. */
extern void
CVMCPUmemspecRelinquishResource(CVMJITRMContext *con,
                                CVMCPUMemSpec *self);



/**************************************************************
 * CPU code emitters - The following are prototypes of code
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* ===== MemoryReference Emitter APIs ===================================== */

/* Purpose: Emits instructions to do a PC-relative load. 
   NOTE: This function must always emit the exact same number of
         instructions regardless of the offset passed to it.
*/
extern void
CVMCPUemitMemoryReferencePCRelative(CVMJITCompilationContext* con,
				    int opcode, int destRegID, int offset);

/* Purpose: Emits instructions to do a load/store operation. */
extern void
CVMCPUemitMemoryReference(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg, CVMCPUMemSpecToken memSpecToken);

/* Purpose: Emits instructions to do a load/store operation. */
extern void
CVMCPUemitMemoryReferenceImmediate(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg, CVMInt32 immOffset);

/* Purpose: Emits instructions to do a conditional load/store operation. */
extern void
CVMCPUemitMemoryReferenceConditional(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg, CVMCPUMemSpecToken memSpecToken,
    CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do a conditional load/store operation. */
extern void
CVMCPUemitMemoryReferenceImmediateConditional(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg, CVMInt32 immOffset,
    CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do a load/store operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
    or
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
*/
extern void
CVMCPUemitArrayElementReference(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID, int indexRegID,
    int shiftOpcode, int shiftAmount);

/* ===== 32 Bit ALU Emitter APIs ========================================== */

/* Purpose: Emits instructions to do the specified 32 bit unary ALU
            operation. */
extern void
CVMCPUemitUnaryALU(CVMJITCompilationContext *con, 
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc);

/* Purpose: Emits instructions to do the specified conditional 32 bit unary
            ALU operation. */
extern void
CVMCPUemitUnaryALUConditional(CVMJITCompilationContext *con,
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc, CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
extern void
CVMCPUemitBinaryALU(CVMJITCompilationContext *con,
    int opcode, int destRegID, int srcRegID, CVMCPUALURhsToken aluRhsToken,
    CVMBool setcc);

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
extern void
CVMCPUemitBinaryALUConstant(CVMJITCompilationContext *con,
    int opcode, int destRegID, int lhsRegID, CVMInt32 rhsConstValue,
    CVMBool setcc);

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
extern void
CVMCPUemitBinaryALURegister(CVMJITCompilationContext *con,
    int opcode, int destRegID, int lhsRegID, int rhsRegID,
    CVMBool setcc);

/* Purpose: Emits instructions to do the specified conditional 32 bit ALU
            operation. */
extern void
CVMCPUemitBinaryALUConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int lhsRegID, CVMCPUALURhsToken rhsToken,
    CVMBool setcc, CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do the specified conditional 32 bit ALU
            operation. */
extern void
CVMCPUemitBinaryALUConstantConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int lhsRegID, CVMInt32 rhsConstValue,
    CVMBool setcc, CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do the specified conditional 32 bit ALU
            operation. */
extern void
CVMCPUemitBinaryALURegisterConditional(CVMJITCompilationContext* con,
   int opcode, int destRegID, int lhsRegID, int rhsRegID,
    CVMBool setcc, CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.*/
extern void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con,
    int opcode, int destRegID, int srcRegID, CVMUint32 shiftAmount);

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.
            The shift amount is specified in a register.  The platform
            implementation is responsible for making sure that the shift
            semantics is done in such a way that the effect is the same as if
            the shiftAmount is masked with 0x1f before the shift operation is
            done.  This is needed in order to be compliant with the VM spec.
*/
extern void
CVMCPUemitShiftByRegister(CVMJITCompilationContext *con,
    int opcode, int destRegID, int srcRegID, int shiftAmountRegID);

/*
 * Purpose: multiply by an immedate value if the platform supports it.
 */
#ifdef CVMCPU_HAS_IMUL_IMMEDIATE
extern void
CVMCPUemitMulConstant(CVMJITCompilationContext* con,
    int destreg, int lhsreg, CVMInt32 value);
#endif

/* For MULL instruction:
       destreg = (lhsreg * rhsreg)[31:0]

   For MULH instruction:
       destreg = (lhsreg * rhsreg)[63:32]

   extraReg will always be CVMCPU_INVALID_REG unless processor specific
   code decides to make use of it with a processor specific MUL opcode.
*/
extern void
CVMCPUemitMul(CVMJITCompilationContext* con,
    int opcode, int destreg, int lhsreg, int rhsreg, int extrareg);

/* ===== Emitters to load a constant =================================== */

/* Purpose: Loads a 32-bit constant into a register. */
extern void
CVMCPUemitLoadConstant(CVMJITCompilationContext *con,
    int regID, CVMInt32 v);

/* Purpose: Conditionally loads a 32-bit constant into a register. */
extern void
CVMCPUemitLoadConstantConditional(CVMJITCompilationContext *con,
    int regID, CVMInt32 v,
    CVMCPUCondCode condCode);

/* Purpose: Does a 32-bit mov into a register.. */

extern void
CVMCPUemitMove(CVMJITCompilationContext* con,
    int opcode, int destRegID, CVMCPUALURhsToken srcToken,
    CVMBool setcc);

extern void
CVMCPUemitMoveRegister(CVMJITCompilationContext* con,
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc);

extern void
CVMCPUemitMoveConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, CVMCPUALURhsToken srcToken,
    CVMBool setcc, CVMCPUCondCode condCode);

extern void
CVMCPUemitMoveRegisterConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc, CVMCPUCondCode condCode);

/* Purpose: 32-bit compare. */

extern void
CVMCPUemitCompare(CVMJITCompilationContext* con,
    int opcode, CVMCPUCondCode condCode,
    int lhsreg, CVMCPUALURhsToken rhsToken);

extern void
CVMCPUemitCompareConstant(CVMJITCompilationContext* con,
    int opcode, CVMCPUCondCode condCode,
    int lhsRegID, CVMInt32 rhsConstValue);

extern void
CVMCPUemitCompareRegister(CVMJITCompilationContext* con,
    int opcode, CVMCPUCondCode condCode,
    int lhsRegID, int rhsRegID);

/* Purpose: emit a nop. */

extern void
CVMCPUemitNop(CVMJITCompilationContext* con);

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
/*
 * Purpose: Emits an atomic swap operation. The value to swap in is in
 *          destReg, which is also where the swapped out value will be placed.
 */
extern void
CVMCPUemitAtomicSwap(CVMJITCompilationContext* con,
		     int destReg, int addressReg);
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
/*
 * Purpose: Does an atomic compare-and-swap operation of newValReg into
 *          the address addressReg+addressOffset if the current value in
 *          the address is oldValReg. oldValReg and newValReg may
 *          be clobbered.
 * Return:  The int returned is the logical PC of the instruction that
 *          branches if the atomic compare-and-swap fails. It will be
 *          patched by the caller to branch to the proper failure address.
 */
extern int
CVMCPUemitAtomicCompareAndSwap(CVMJITCompilationContext* con,
			       int addressReg, int addressOffset,
			       int oldValReg, int newValReg);
#else
#error Unsupported locking type for CVMJIT_SIMPLE_SYNC_METHODS
#endif
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/* Purpose: Emits code to computes the following expression and stores the
            result in the specified destReg:

        dest = baseReg opcode (indexReg shiftOpcode #shiftAmount)

            where opcode is a binary ALU operation and shiftOpcode is a
            shift operator that shifts the indexReg by the shiftAmount.
*/
extern void
CVMCPUemitComputeAddressOfArrayElement(CVMJITCompilationContext *con,
                                       int opcode, int destRegID,
                                       int baseRegID, int indexRegID,
                                       int shiftOpcode, int shiftAmount);

/* ===== Floating Point Emitter APIs ====================================== */
#ifdef CVM_JIT_USE_FP_HARDWARE
extern void
CVMCPUemitBinaryFP(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int lhsRegID, int rhsRegID);

extern void
CVMCPUemitUnaryFP(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int srcRegID);

extern void
CVMCPUemitFCompare(CVMJITCompilationContext *con, int opcode,
                   CVMCPUCondCode condCode, int lhsRegID, int rhsRegID);

/* Purpose: Loads a 32-bit constant into a floating-point register. */
/* the value is just bits to us. If it started as a floating-point number,
 * you'd better cast it carefully.
 */
extern void
CVMCPUemitLoadConstantFP(CVMJITCompilationContext *con,
    int regID, CVMInt32 v);

extern void
CVMCPUemitLoadLongConstantFP(CVMJITCompilationContext *con,
    int regID, CVMJavaVal64 *value);

#endif

/* ===== 64 Bit Emitter APIs ============================================== */

/* Purpose: Emits instructions to do the specified 64 bit unary ALU
            operation. */
extern void
CVMCPUemitUnaryALU64(CVMJITCompilationContext *con, int opcode,
                     int destRegID, int srcRegID);

/* Purpose: Emits instructions to do the specified 64 bit ALU operation. */
extern void
CVMCPUemitBinaryALU64(CVMJITCompilationContext *con,
		      int opcode, int destRegID, int lhsRegID, int rhsRegID);

/* Purpose: Loads a 64-bit integer constant into a register. */
extern void
CVMCPUemitLoadLongConstant(CVMJITCompilationContext *con, int regID,
                           CVMJavaVal64 *value);

/* Purpose: Emits instructions to compares 2 64 bit integers for the specified
            condition. 
   Return: The returned condition code may have been transformed by the
           comparison instructions because the emitter may choose to implement
           the comparison in a different form.  For example, a less than
           comparison can be implemented as a greater than comparison when
           the 2 arguments are swapped.  The returned condition code indicates
           the actual comparison operation that was emitted.
*/
CVMCPUCondCode
CVMCPUemitCompare64(CVMJITCompilationContext *con, int opcode,
                    CVMCPUCondCode condCode, int lhsRegID, int rhsRegID);

/* Purpose: Emits instructions to convert a 32 bit int into a 64 bit int. */
extern void
CVMCPUemitInt2Long(CVMJITCompilationContext *con, int destRegID, int srcRegID);

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
extern void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID);

/* ===== Flow Control Emitter APIs ======================================== */

extern void
CVMCPUemitPopFrame(CVMJITCompilationContext* con, int resultSize);

/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the CVMCPU_JSR_RETURN_ADDRESS_SET == LR.
 */
extern void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno);

/*
 * Branch to the address in the specified register.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno);

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The target address for index 0 is located at
 * pc + 2 * CVMCPU_INSTRUCTION_SIZE, assuming that branches are always
 * done as a single instruction.
 * The branch should only be done if the compare succeeded. Otherwise
 * we just need to fall through to the default case.
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int indexRegNo);

/*
 * Emit code to do a gc rendezvous. The second argument 'cbufRewind'
 * tells the emitter if there is a code buffer rewinding, so whichever
 * instructions are emitted next just overwrite the instructions emitted
 * by CVMCPUemitGcCheck. This is done to support patching of gc checkpoints
 * so they have zero cost until a gc is needed.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind);
#endif

/*
 * Make a PC-relative branch or branch-and-link instruction
 */

extern void
CVMCPUemitBranch(CVMJITCompilationContext* con, 
		 int logicalPC, CVMCPUCondCode condCode);

extern void
CVMCPUemitBranchLink(CVMJITCompilationContext* con, int logicalPC );

/*
 * Make a PC-relative branch or branch-and-link instruction, and
 * add the branch instruction(s) to the target fixup list.
 */
extern void
CVMCPUemitBranchNeedFixup(CVMJITCompilationContext* con, 
		          int logicalPC, CVMCPUCondCode condCode,
                          CVMJITFixupElement** fixupList);

extern void
CVMCPUemitBranchLinkNeedFixup(CVMJITCompilationContext* con, int logicalPC,
                              CVMJITFixupElement** fixupList);


enum CVMCPUFrameReferenceType { 
    CVMCPU_FRAME_LOCAL, CVMCPU_FRAME_TEMP, CVMCPU_FRAME_CSTACK
};

/*
 * This ultimately calls CVMCPUemitMemoryReference,
 * but knows the stack layout, so can convert cell numbers to 
 * basereg +/- offset.
 */
extern void
CVMCPUemitFrameReference(CVMJITCompilationContext* con,
    int opcode, int destreg, enum CVMCPUFrameReferenceType addressing,
    int cellNumber);

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. If okToDumpCp is TRUE, the constant pool will
 * be dumped if possible. Set okToBranchAroundCpDump to FALSE if you plan
 * on generating a stackmap after calling CVMCPUemitAbsoluteCall.
 */
extern void
CVMCPUemitAbsoluteCall(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
                       CVMBool okToBranchAroundCpDump);
extern void
CVMCPUemitAbsoluteCallConditional(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump,
		       CVMCPUCondCode condCode);
extern void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall(CVMJITCompilationContext* con,
                                             const void* target,
                                             CVMBool okToDumpCp,
                                             CVMBool okToBranchAroundCpDump);

/*
 * Return using the correct CCM helper function.
 */
extern void
CVMCPUemitReturn(CVMJITCompilationContext* con, void* helper);


#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
/*
 * CVMCPUfixupInstructionAddress - change the instruction to reference
 * the specified targetLogicalAddress, and return the fixed instruction.
 */

extern CVMCPUInstruction
CVMCPUfixupInstructionAddress(CVMJITCompilationContext* con,
                   CVMCPUInstruction instruction,
                   int instructionLogicalAddress,
                   int targetLogicalAddress,
                   CVMJITAddressMode instructionAddressMode);
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

/* ===== Misc Emitter APIs ================================================ */

typedef struct CVMCPUPrologueRec CVMCPUPrologueRec;
struct CVMCPUPrologueRec {
    /* The logical PC's of the capacity and spill update instructions
       in a compiled method prologue. */
    CVMInt32 capacityStartPC;
    CVMInt32 capacityEndPC;
    CVMInt32 spillStartPC;
    CVMInt32 spillEndPC;
    /* The offset of the int->comp entry point from the comp->comp entry
       point */
    CVMInt32 intToCompOffset;
};

#ifdef CVMCPU_HAS_CP_REG
/* Purpose: Set up constant pool base register */
void
CVMCPUemitLoadConstantPoolBaseRegister(CVMJITCompilationContext *con);
#endif

/* Purpose:  Loads the CCEE into the specified register. */
extern void
CVMCPUemitLoadCCEE(CVMJITCompilationContext *con, int destRegID);

/* Purpose:  Load or Store a field of the CCEE. */
extern void
CVMCPUemitCCEEReferenceImmediate(CVMJITCompilationContext *con,
				 int opcode, int regID, int offset);

/* Purpose:  Add/sub a 16-bits constant scaled by 2^scale. Called by 
 *           method prologue and patch emission routines. 
 * NOTE:     CVMCPUemitAddConstant16Scaled should not rely on regman   
 *           states because the regman context used to emit the method
 *           prologue is gone at the patching time.
 * NOTE:     CVMCPUemitALUConstant16Scaled must always emit the same
 *           number of instructions, no matter what constant or scale
 *           is passed to it.
 * NOTE:     A platform can choose to fail compilation if (constant<<scale)
 *           is on the large size. The largest number passed will be for
 *           the java stack usage, so it is normally relatively small
 *           (less than 256), unless there are a large number of locals.
 */
extern void
CVMCPUemitALUConstant16Scaled(CVMJITCompilationContext *con, int opcode,
                              int destRegID, int srcRegID,
                              CVMUint32 constant, int scale);

/*
 * Purpose: Emit stack limit check and store return address at the 
 *          start of each method.
 */
extern void
CVMCPUemitStackLimitCheckAndStoreReturnAddr(CVMJITCompilationContext* con);

/* Purpose: Emits the prologue code for the compiled method.  */
extern void
CVMCPUemitMethodPrologue(CVMJITCompilationContext *con, 
			 CVMCPUPrologueRec* r);

/* Purpose: Patches prologue for the method */
extern void
CVMCPUemitMethodProloguePatch(CVMJITCompilationContext *con, 
			      CVMCPUPrologueRec* r);

/* 
 *  Purpose: Emits code to invoke method through MB.
 *           MB is already in CVMCPU_ARG1_REG. 
 */
extern void
CVMCPUemitInvokeMethod(CVMJITCompilationContext *con);

#ifndef CVMemitThreadSchedHook
#define CVMemitThreadSchedHook(con)   /* */
#endif

/* ===== Optional APIs to support 64-bit registers ======================== */
#ifdef CVMCPU_HAS_64BIT_REGISTERS

/* Purpose: Move the 64-bit content of HI and LO registers into one
 *          64-bit register.
 */
extern void
CVMCPUemitMoveTo64BitRegister(CVMJITCompilationContext* con,
                              int destRegID, int srcRegID);

/* Purpose: Move the content of the 64-bit register into HI and LO
 *          registers. 
 */
extern void
CVMCPUemitMoveFrom64BitRegister(CVMJITCompilationContext* con,
                                int destRegID, int srcRegID);
#endif

/* Purpose: Emits instructions for implementing the following arithmetic
            operation: destRegID = (shiftRegID << i) + addRegID
            where i is an immediate value. Any shift opcode is allowed. */
extern void
CVMCPUemitShiftAndAdd(CVMJITCompilationContext *con,
		      int shiftOpcode,
		      int destRegID, int shiftRegID, int addRegID,
		      CVMInt32 shiftAmount);

/* Purpose: On platforms that don't have full support for conditional
            instructions, the CVMCPUoppositeCondCode must be defined.
	    It is used by shared code that implements conditional
	    emitters by emitting a conditional branch around unconditional
	    instructions. The array maps condition codes to their
	    opposites. For example, it maps EQ to NE and LT to GE.
*/
#if !defined(CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS) || \
    !defined(CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS) || \
    !defined(CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS)
extern const CVMCPUCondCode CVMCPUoppositeCondCode[];
#endif

/* ===== Memory barrier emitters ======================== */
#ifdef CVM_MP_SAFE
extern void
CVMCPUemitMemBar(CVMJITCompilationContext *con);

/*
 * Guarantees that all load/store subsequent to
 * CVMCPUemitMemBarAcquire() becomes visible after it
 * completes.
 */
extern void
CVMCPUemitMemBarAcquire(CVMJITCompilationContext *con);

/*
 * Guarantees that all load/store occurring previous to
 * CVMCPUemitMemBarAcquire() complete before it completes.
 */
extern void
CVMCPUemitMemBarRelease(CVMJITCompilationContext *con);

#else
#define CVMCPUemitMemBar(con)
#define CVMCPUemitMemBarAcquire(con)
#define CVMCPUemitMemBarRelease(con)
#endif


/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************

    The following symbols are specified by javavm/include/porting/jit/jit.h:
        CVMCPU_MAX_ARG_REGS
        CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS

    NOTE: If the platform #defines CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS, it
    must provide its own implementation of the calling convention abstraction
    APIs.  The reason for this is because overflow of arguments that do not
    fit in the argument registers are usually handled differently on different
    platforms.

    If the platform wishes to define its own C calling convention which is
    different from the default implementation provided below, then the
    platform must #define the symbol in jitriscemitterdefs_cpu.h:
        CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION

    CVMCPUCallContext - opaque type encapsulating the record for tracking the
                        resources utilized for setting up C arguments.
                        This should be defined in jitriscemitterdefs_cpu.h
                        if CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION is
                        #define'd.

    The behavior of the default calling convention implementation provided
    below is as follows:
        1. Arguments to C functions are passed from left to right.
        2. The 1st (i.e. left most) argument is passed in CVMCPU_ARG1_REG
           if it is 32 bit in size, and in CVMCPU_ARG1_REG and CVMCPU_ARG2_REG
           if it is 64 bit in size.
        3. The next argument is assigned the next available argument register.
           For example, if the arg1 only took up CVMCPU_ARG1_REG, then arg2
           will start in CVMCPU_ARG2_REG.  If arg1 took up both
           CVMCPU_ARG1_REG and CVMCPU_ARG2_REG, the arg2 will start in
           CVMCPU_ARG3_REG.
        4. It is assumed that the register numbers for the arg registers are
           consecutive starting with CVMCPU_ARG1_REG having the lowest reg
           number and each subsequent arg register has a reg number of 1
           greater than the preceeding arg reg.
        5. All arguments are aligned on word boundaries.  This means that
           there is no special action to align 64 bit arguments on argument
           registers with even (or odd) register numbers.  Arguments are
           assigned the next available register in the set of argument
           registers.
        6. An attempt to use more than the max number of argument registers
           available as defined by CVMCPU_MAX_ARG_REGS will result in
           assertion failures if CVM is built with CVM_DEBUG_ASSERTS=true.
           If CVM is built with CVM_DEBUG_ASSERTS=false, then using more than
           the max number of argument registers may result in unpredicatble
           failures.
        7. Passing structs by value is not implemented.

    If the default implementation is not appropriate, the platform must
    provide its own implementation of the functions/macros below:

    The platform specific implemention of these functions/macros can be placed
    in jitriscemitter_cpu.h and jitemitter_cpu.c as appropriate.
*/

#if !defined(CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION) && \
    !defined(CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS)
typedef CVMUint32 CVMCPUCallContext;
#endif

/* Purpose: Dynamically instantiates an instance of the CVMCPUCallContext. */
/* NOTE: There is no corresponding delete or free function/macro.  If the
         platform wishes to allocate an instance of CVMCPUCallContext, it
         should use the CVMJITmemNew() allocator. */
extern CVMCPUCallContext *
CVMCPUCCallnewContext(CVMJITCompilationContext *con);

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
extern CVMJITRegsRequiredType
CVMCPUCCALLgetRequired(CVMJITCompilationContext *con,
                       CVMJITRegsRequiredType argsRequired,
		       CVMJITIRNode *intrinsicNode,
		       CVMJITIntrinsic *irec,
		       CVMBool useRegArgs);

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. */
extern void
CVMCPUCCALLinitArgs(CVMJITCompilationContext *con,
                    CVMCPUCallContext *callContext,
                    CVMJITIntrinsic *irec, CVMBool forTargetting,
		    CVMBool useRegArgs);

/* Purpose: Gets the register targets for the specified argument. */
extern CVMRMregset
CVMCPUCCALLgetArgTarget(CVMJITCompilationContext *con,
                        CVMCPUCallContext *callContext,
                        int argType, int argNo, int argWordIndex,
			CVMBool useRegArgs);

/* Purpose: Pins an arguments to the appropriate register or store it into the
            appropriate stack location. */
extern CVMRMResource *
CVMCPUCCALLpinArg(CVMJITCompilationContext *con,
                  CVMCPUCallContext *callContext, CVMRMResource *arg,
                  int argType, int argNo, int argWordIndex,
                  CVMRMregset *outgoingRegs, CVMBool useRegArgs);

/* Purpose: Relinquish a previously pinned arguments. */
extern void
CVMCPUCCALLrelinquishArg(CVMJITCompilationContext *con,
                         CVMCPUCallContext *callContext, CVMRMResource *arg,
                         int argType, int argNo, int argWordIndex,
			 CVMBool useRegArgs);

/* Purpose: Releases any resources allocated in CVMCPUCCALLinitArgs(). */
extern void
CVMCPUCCALLdestroyArgs(CVMJITCompilationContext *con,
                       CVMCPUCallContext *callContext,
                       CVMJITIntrinsic *irec, CVMBool forTargetting,
		       CVMBool useRegArgs);

#if !defined(CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION) && \
    !defined(CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS)

/* Purpose: Dynamically instantiates an instance of the CVMCPUCallContext. */
#define CVMCPUCCallnewContext(con)  ((CVMCPUCallContext *)NULL)

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
#define CVMCPUCCALLgetRequired(con, argsRequired, \
			       intrinsicNode, irec, useRegArgs) \
    ((void)&(useRegArgs), (CVMCPU_AVOID_C_CALL | argsRequired))

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. */
#define CVMCPUCCALLinitArgs(con, callContext, irec, forTargetting, \
			    useRegArgs) \
    ((void)(callContext), (void)&(useRegArgs))

/* Purpose: Gets the register targets for the specified argument. */
#define CVMCPUCCALLgetArgTarget(con, callContext, \
                                argType, argNo, argWordIndex, useRegArgs) \
    ((void)&(useRegArgs), (1U << (CVMCPU_ARG1_REG + argWordIndex)))

/* Purpose: Relinquish a previously pinned arguments. */
#define CVMCPUCCALLrelinquishArg(con, callContext, arg, argType, argNo, \
				 argWordIndex, useRegArgs) \
    ((void)&(useRegArgs), CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg))

/* Purpose: Releases any resources allocated in CVMCPUCCALLinitArgs(). */
#define CVMCPUCCALLdestroyArgs(con, callContext, irec, forTargetting, \
			       useRegArgs) \
    ((void)(callContext), (void)&(useRegArgs))

#endif /* !CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION &&
          !CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS */

/* Purpose: Emits a constantpool dump with a branch around. */
extern void
CVMRISCemitConstantPoolDumpWithBranchAround(
    CVMJITCompilationContext* con);

/* Purpose: Emits a constantpool dump with a branch around it if needed. */
extern void
CVMRISCemitConstantPoolDumpWithBranchAroundIfNeeded(
    CVMJITCompilationContext* con);

#include "javavm/include/jit/jitriscemitter_cpu.h"

#endif /* _INCLUDED_JITRISCEMITTER_H */

