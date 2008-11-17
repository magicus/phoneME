/*
 * @(#)jitriscemitter_cpu.h	1.26 06/10/10
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

#ifndef _INCLUDED_SPARC_JITRISCEMITTER_CPU_H
#define _INCLUDED_SPARC_JITRISCEMITTER_CPU_H

/*
 * This file #defines all of the emitter function prototypes and macros that
 * shared parts use in a platform indendent way in RISC ports of the jit back
 * end. The exported functions and macros are prefixed with CVMCPU_.  Other
 * functions and macros should be considered private to the SPARC 
 * specific parts of the jit, including those with the CVMSPARC_ prefix.
 */

/**********************************************************************
 * Some very SPARC-specific bits of code generation.
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
 * debug the resulting code! This would not be desirable
 */

/**************************************************************
 * CPU ALURhs and associated types - The following are function
 * prototypes and macros for the ALURhs abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

#define CVMCPUalurhsIsEncodableAsImmediate(opcode, constValue) \
    ((CVMInt32)(constValue) < (4*1024) && (CVMInt32)(constValue) >= -(4*1024))

#define CVMCPUalurhsIsConstant(aluRhs) \
    ((aluRhs)->type == CVMSPARC_ALURHS_CONSTANT)

#define CVMCPUalurhsGetConstantValue(aluRhs)    ((aluRhs)->constValue)

/* ALURhs token encoder APIs:============================================= */

/* Purpose: Gets the token for the ALURhs value for use in the instruction
            emitters. */
CVMCPUALURhsToken
CVMSPARCalurhsGetToken(const CVMCPUALURhs *aluRhs);

#define CVMCPUalurhsGetToken(con, aluRhs)   \
     ((void)(con), CVMSPARCalurhsGetToken(aluRhs))

/* Purpose: Encodes an CVMCPUALURhsToken for the specified arguments. */
/* NOTE: This function is private to the SPARC implementation and can only be
         called from the shared implementation via the CVMCPUxxx macros
         below. */
CVMCPUALURhsToken
CVMSPARCalurhsEncodeToken(CVMSPARCALURhsType type, int constValue,
                         int rRegID);

/* Purpose: Encodes a CVMCPUALURhsToken for the specified constant value. */
#define CVMSPARCalurhsEncodeConstantToken(constValue) \
     CVMSPARCalurhsEncodeToken(CVMSPARC_ALURHS_CONSTANT, (constValue), \
         CVMCPU_INVALID_REG) 

#define CVMSPARCalurhsEncodeRegisterToken(regID) \
     (CVMSPARC_ALURHS_REGISTER | (regID))

/**************************************************************
 * CPU MemSpec and associated types - The following are function
 * prototypes and macros for the MemSpec abstraction required by
 * the RISC emitter porting layer.
 **************************************************************/

/* MemSpec query APIs: ==================================================== */

#define CVMCPUmemspecIsEncodableAsImmediate(value) \
    ((-0xfff <= ((CVMInt32)(value))) && (((CVMInt32)(value)) <= 0xfff))

#define CVMCPUmemspecIsEncodableAsOpcodeSpecificImmediate(opcode, value) \
    CVMCPUmemspecIsEncodableAsImmediate(value) 

/* MemSpec token encoder APIs: ============================================ */

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMSPARCmemspecGetToken(const CVMCPUMemSpec *memSpec);

#define CVMCPUmemspecGetToken(con, memSpec) \
    ((void)(con), CVMSPARCmemspecGetToken(memSpec))

/* NOTE: The CVMCPUMemSpecToken is encoded as:
        bits 13 - 14:   memSpec type;
        bits 0 - 12:    immediate offset;
        bits 0 - 4:     register id;
*/
#define CVMSPARCmemspecEncodeToken(type, offset) \
    (((type) << 13) | ((offset) & 0x1FFF))

#define CVMCPUmemspecEncodeImmediateToken(con, offset) \
    ((void)(con), CVMSPARCmemspecEncodeToken(CVMCPU_MEMSPEC_IMMEDIATE_OFFSET, \
                      (offset)))

/* NOTE: SPARC does not support post-increment and pre-decrement
   memory access. This is to mimic ARM hehavior. Extra add/sub 
   instruction is emitted in order to achieve it. */
#define CVMCPUmemspecEncodePostIncrementImmediateToken(con, increment)        \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(increment)),  \
     CVMSPARCmemspecEncodeToken(CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET,\
                                (increment)))

#define CVMCPUmemspecEncodePreDecrementImmediateToken(con, decrement)         \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(decrement)),  \
     CVMSPARCmemspecEncodeToken(CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET, \
                                (decrement)))

/**************************************************************
 * CPU code emitters - The following are prototypes of code
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* ===== 32 Bit ALU Emitter APIs ========================================== */

#define CVMCPUemitBinaryALURegister(con, opcode, destRegID, lhsRegID, \
                                    rhsRegID, setcc) \
    CVMCPUemitBinaryALU((con), (opcode), (destRegID), (lhsRegID), \
        CVMSPARCalurhsEncodeRegisterToken(rhsRegID), (setcc))

#define CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitMove((con), (opcode), (destRegID), \
        CVMSPARCalurhsEncodeRegisterToken(srcRegID), (setcc))

#define CVMCPUemitCompareRegister(con, opcode, condCode, lhsRegID, rhsRegID) \
    CVMCPUemitCompare((con), (opcode), (condCode), (lhsRegID), \
        CVMSPARCalurhsEncodeRegisterToken(rhsRegID))

/* ===== Branch Emitter APIs ========================================== */
#define CVMCPUemitBranch(con, target, condCode) \
    CVMCPUemitBranchNeedFixup((con), (target), (condCode), NULL)

#define CVMCPUemitBranchLink(con, target) \
    CVMCPUemitBranchLinkNeedFixup((con), (target), NULL)

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
CVMCPUInstruction
CVMSPARCgetBranchInstruction(int opcode, CVMCPUCondCode condCode, int offset);

/*
 * Make a PC-relative branch or branch-and-link instruction
 * CVMSPARCemitBranch does NOT emit the nop, and is capable of
 * or-ing in an annul bit.
 * CVMCPUemitBranch is much more tame and less machine specific:
 * no annullment and no problems with delay slots as it will supply a nop.
 */
void
CVMSPARCemitBranch(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode, CVMBool annul);

/* ===== Misc Emitter APIs ================================================ */

/**************************************************************
 * Misc function prototypes and macros.
 **************************************************************/

#endif /* _INCLUDED_SPARC_JITRISCEMITTER_CPU_H */
