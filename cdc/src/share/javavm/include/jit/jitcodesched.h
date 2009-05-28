/*
 * @(#)jitcodesched.h	1.7 06/10/10
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
 *
 */

/*
 * Copyright 2005 Intel Corporation. All rights reserved.
 */

#ifndef _INCLUDED_JITCODESCHEDMAN_H
#define _INCLUDED_JITCODESCHEDMAN_H
#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitset.h"

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD

#define CVMJITCS_EMIT_LOAD_CONSTANT_INSTRUCTION (1 << 0)
#define CVMJITCS_EMIT_BRANCH_INSTRUCTION        (1 << 1)
#define CVMJITCS_EMIT_PATCHED_INSTRUCTION       (1 << 2)
#define CVMJITCS_EMIT_CAPTURE_STACKMAP          (1 << 3)
#define CVMJITCS_EMIT_MAPPC                     (1 << 4)
#define CVMJITCS_EMIT_PATCH_INSTRUCTION         (1 << 5)
#define CVMJITCS_EMIT_BEGIN_INLINING            (1 << 6)
#define CVMJITCS_EMIT_END_INLINING              (1 << 7)


/* NOTE: this eventually needs to be moved to the porting layer
 * if we ever support code scheduling in a platform other than ARM.*/
#define CVM_CPU_NUM_REGISTERS 16

typedef struct CVMJITCSPatchInstruction CVMJITCSPatchInstruction;


typedef struct {
  CVMUint32 startPC;
  CVMUint32 endPC;
} CVMJITCSConstantPoolDumpInfo;

struct CVMJITCSContext{
    CVMUint32                 curBlockLogicalPC;
    CVMUint32                 instructionFlags;
    CVMCPUInstruction         nop;
    CVMInt32                  destRegister;
    CVMInt32                  destRegister2;
    CVMUint32                 statusPC;
    CVMUint32                 exceptionPC;
    CVMUint32                 strPC;
    CVMUint32                 branchPC;
    CVMUint32                 comparePC;
    CVMInt8                   sourceRegsNum;
    CVMInt8                   sourceRegisters[3];

    CVMUint32                 getFieldPC;
    CVMUint32                 putFieldPC;
    CVMUint32                 putStaticFieldPC;
    CVMUint32                 getStaticFieldPC;
    CVMUint32                 getArrayElementPC;
    CVMUint32                 putArrayElementPC;

    CVMUint32                 baseRegister;
    CVMUint32                 frameIndex;

    CVMInt8                   outGoingRegisters[CVM_CPU_NUM_REGISTERS];

    CVMUint32                 defRegistersPC[CVM_CPU_NUM_REGISTERS];
    CVMUint32                 useRegistersPC[CVM_CPU_NUM_REGISTERS];

    CVMUint32                 emitInPlaceCount;
    CVMUint32                 *defJavaFrameBasedReferencePC;
    CVMUint32                 *useJavaFrameBasedReferencePC;

    CVMUint32                 branchInstructionTargetOffset;
    CVMBool                   branchInstructionLink;
    CVMCPUCondCode            branchInstructionCondCode;

    CVMJITGrowableArray       nopsArray;
    CVMJITGrowableArray       constantPoolArray;
    CVMUint32                 topOfInstructions;
    CVMJITCSPatchInstruction* instructionsStack;
};

#define CVMJITcsContext(_con) (&((_con)->csContext))


#define CVMJITCS_EMIT_INPLACE          (1 << 0)
#define CVMJITCS_EMIT_INSERT           (1 << 1)
#define CVMJITCS_EMIT_PATCH            (1 << 2)
#define CVMJITCS_LOAD_INSTRUCTION      (1 << 3)
#define CVMJITCS_STORE_INSTRUCTION     (1 << 4)
#define CVMJITCS_BRANCH_INSTRUCTION    (1 << 5)
#define CVMJITCS_STATUS_INSTRUCTION    (1 << 6)

#define CVMJITCS_COMPARE_INSTRUCTION     (1 << 7)
#define CVMJITCS_NEEDFIXUP_INSTRUCTION   (1 << 8)
#define CVMJITCS_PUTFIELD_INSTRUCTION    (1 << 9)
#define CVMJITCS_GETFIELD_INSTRUCTION    (1 << 10)
#define CVMJITCS_PUTARRAY_INSTRUCTION    (1 << 11)
#define CVMJITCS_GETARRAY_INSTRUCTION    (1 << 12)
#define CVMJITCS_GETSTATIC_INSTRUCTION    (1 << 13)
#define CVMJITCS_PUTSTATIC_INSTRUCTION    (1 << 14)
#define CVMJITCS_EXCEPTION_INSTRUCTION    (1 << 15)
#define CVMJITCS_ARRAYINDEXOUTOF_INSTRUCTION (1 << 16)

#define CVMJITCS_TEMP_LOCAL_NUM 3

#define CVMJITcsClearInstructionFlags(con) \
           (CVMJITcsContext(con)->instructionFlags &= CVMJITCS_EMIT_INPLACE)

#define CVMJITcsIsArrayIndexOutofBoundsBranch(con)\
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_ARRAYINDEXOUTOF_INSTRUCTION)
#define CVMJITcsIsEmitInPlace(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_EMIT_INPLACE)
#define CVMJITcsIsEmitPatchInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_EMIT_PATCH)
#define CVMJITcsIsEmitInsertInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_EMIT_INSERT)
#define CVMJITcsIsLoadInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_LOAD_INSTRUCTION)
#define CVMJITcsIsStoreInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_STORE_INSTRUCTION)
#define CVMJITcsIsStatusInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_STATUS_INSTRUCTION)
#define CVMJITcsIsBranchInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_BRANCH_INSTRUCTION)
#define CVMJITcsIsCompareInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_COMPARE_INSTRUCTION)
#define CVMJITcsIsNeedFixupInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_NEEDFIXUP_INSTRUCTION)
#define CVMJITcsIsPutFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_PUTFIELD_INSTRUCTION)
#define CVMJITcsIsGetFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_GETFIELD_INSTRUCTION)
#define CVMJITcsIsPutArrayInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_PUTARRAY_INSTRUCTION)
#define CVMJITcsIsGetArrayInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_GETARRAY_INSTRUCTION)
#define CVMJITcsIsExceptionInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_EXCEPTION_INSTRUCTION)

#define CVMJITcsSetEmitPatchInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_EMIT_PATCH)
#define CVMJITcsSetEmitInsertInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_EMIT_INSERT)

#define CVMJITcsSetEmitInPlace(con)					\
    {									\
	CVMJITcsContext(con)->emitInPlaceCount ++;			\
	CVMassert(CVMJITcsContext(con)->emitInPlaceCount > 0);		\
	CVMJITcsContext(con)->instructionFlags |= CVMJITCS_EMIT_INPLACE; \
    }

#define CVMJITcsSetEmitInPlaceWithBufSizeAdjust(con, flag, size)	\
    {									\
	CVMJITcsContext(con)->emitInPlaceCount ++;			\
	CVMassert(CVMJITcsContext(con)->emitInPlaceCount > 0);		\
	CVMJITcsContext(con)->instructionFlags |= CVMJITCS_EMIT_INPLACE; \
        if (flag) {\
            {\
                CVMUint32 _i = 0;\
                for (_i = 0; _i < size; _i += sizeof(CVMCPUInstruction)) {\
                    CVMJITcbufEmitPC(con, ((CVMUint32)CVMJITcbufGetLogicalPC(con) + _i),\
                          CVMJITcsContext(con)->nop);\
                }\
            }\
            CVMJITcbufGetLogicalPC(con) += size;\
            CVMJITcbufGetPhysicalPC(con) =\
                CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));\
        }\
    }

#define CVMJITcsClearEmitPatchInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags &= ~CVMJITCS_EMIT_PATCH)
#define CVMJITcsClearEmitInsertInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags &= ~CVMJITCS_EMIT_INSERT)
#define CVMJITcsClearEmitInPlace(con)					\
    {									\
	CVMassert(CVMJITcsContext(con)->emitInPlaceCount > 0);		\
	CVMJITcsContext(con)->emitInPlaceCount --;			\
	if (CVMJITcsContext(con)->emitInPlaceCount == 0) {		\
	    CVMJITcsContext(con)->instructionFlags &= ~CVMJITCS_EMIT_INPLACE; \
	}								\
    }

#define CVMJITcsSetLoadInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_LOAD_INSTRUCTION)
#define CVMJITcsSetStoreInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_STORE_INSTRUCTION)
#define CVMJITcsSetStatusInstructionInternal(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_STATUS_INSTRUCTION)
#define CVMJITcsSetBranchInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_BRANCH_INSTRUCTION)
#define CVMJITcsSetCompareInstructionInternal(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_COMPARE_INSTRUCTION)
#define CVMJITcsSetNeedFixupInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_FIXUP_INSTRUCTION)
#define CVMJITcsSetPutFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_PUTFIELD_INSTRUCTION)
#define CVMJITcsSetGetFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_GETFIELD_INSTRUCTION)
#define CVMJITcsSetPutArrayInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_PUTARRAY_INSTRUCTION)
#define CVMJITcsSetGetArrayInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_GETARRAY_INSTRUCTION)
#define CVMJITcsSetExceptionInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_EXCEPTION_INSTRUCTION)
#define CVMJITcsSetArrayIndexOutofBoundsBranch(con)\
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_ARRAYINDEXOUTOF_INSTRUCTION)

#define CVMJITcsBranchPC(con) \
    (CVMJITcsContext(con)->branchPC)
#define CVMJITcsBaseRegister(con) \
    (CVMJITcsContext(con)->baseRegister)
#define CVMJITcsJavaFrameIndex(con) \
    (CVMJITcsContext(con)->frameIndex)
#define CVMJITcsUseJavaFrameBasedReferencePCArray(con) \
    (CVMJITcsContext(con)->useJavaFrameBasedReferencePC)
#define CVMJITcsDefJavaFrameBasedReferencePCArray(con) \
    (CVMJITcsContext(con)->defJavaFrameBasedReferencePC)
#define CVMJITcsUseJavaFrameBasedReferencePC(con, index) \
    (CVMJITcsContext(con)->useJavaFrameBasedReferencePC[index])
#define CVMJITcsDefJavaFrameBasedReferencePC(con, index) \
    (CVMJITcsContext(con)->defJavaFrameBasedReferencePC[index])
#define CVMJITcsSourceRegsNum(con) \
    (CVMJITcsContext(con)->sourceRegsNum)
#define CVMJITcsSourceRegister(con, index) \
    (CVMJITcsContext(con)->sourceRegisters[index])
#define CVMJITcsDestRegister(con) \
    (CVMJITcsContext(con)->destRegister)
#define CVMJITcsDestRegister2(con) \
    (CVMJITcsContext(con)->destRegister2)

#define CVMJITcsUseRegisterPC(con, index) \
    (CVMJITcsContext(con)->useRegistersPC[index])
#define CVMJITcsDefRegisterPC(con, index) \
    (CVMJITcsContext(con)->defRegistersPC[index])
#define CVMJITcsStatusPC(con) \
    (CVMJITcsContext(con)->statusPC)
#define CVMJITcsComparePC(con) \
    (CVMJITcsContext(con)->comparePC)
#define CVMJITcsGetFieldPC(con) \
    (CVMJITcsContext(con)->getFieldPC)
#define CVMJITcsPutFieldPC(con) \
    (CVMJITcsContext(con)->putFieldPC)
#define CVMJITcsGetArrayElementPC(con) \
    (CVMJITcsContext(con)->getArrayElementPC)
#define CVMJITcsPutArrayElementPC(con) \
    (CVMJITcsContext(con)->putArrayElementPC)
#define CVMJITcsOutGoingRegisters(con, index) \
    (CVMJITcsContext(con)->outGoingRegisters[index])
#define CVMJITcsOutGoingRegsNum(con) \
    (CVMJITcsContext(con)->outGoingRegsNum)
#define CVMJITcsStorePC(con) \
    (CVMJITcsContext(con)->strPC)
#define CVMJITcsExceptionPC(con) \
    (CVMJITcsContext(con)->exceptionPC)

#define CVMJITcsNopsArray(_con) \
    (&(CVMJITcsContext(_con)->nopsArray))
#define CVMJITcsConstantPoolArray(_con) \
    (&(CVMJITcsContext(_con)->constantPoolArray))

#define CVMJITcsSetOutGoingRegisters(con, index) \
    {CVMJITcsOutGoingRegisters(con, index) = CVM_TRUE;}

#define CVMJITcsPushSourceRegister(con, reg)				\
    (void)((reg != CVMCPU_INVALID_REG)					\
	   ? (CVMJITcsSourceRegister(con, CVMJITcsSourceRegsNum(con)) = reg, \
	      CVMJITcsSourceRegsNum(con)++)				\
	   : 0)

#define CVMJITcsSetDestRegister(con, reg) \
    {CVMJITcsDestRegister(con) = reg;}
#define CVMJITcsSetCompareInstruction(con, setcc) \
    {if(setcc) {CVMJITcsSetCompareInstructionInternal(con);}}
#define CVMJITcsSetDestRegister2(con, reg) \
    CVMJITcsDestRegister2(con) = reg;

#define CVMJITcsSetStatusBinaryALUInstruction(con, opcode) \
    switch (opcode) {					   \
        case ARM_ADC_OPCODE:				   \
        case ARM_SBC_OPCODE:				   \
        case ARM_RSC_OPCODE:				   \
	    CVMJITcsSetStatusInstructionInternal(con);	   \
	    break;					   \
        default:					   \
	    ;						   \
    }

#define CVMJITcsSetStatusInstruction(con, condCode)			    \
    if (!((condCode == CVMCPU_COND_AL) || (condCode == CVMCPU_COND_NV)))  { \
        CVMJITcsSetStatusInstructionInternal(con);			    \
    }

#define CVMJITcsSetBaseRegister(con, basereg) \
    CVMJITcsBaseRegister(con) = (basereg);

#define CVMJITcsSetJavaFrameIndex(con, javaFrameIndex) \
    CVMJITcsJavaFrameIndex(con) = (javaFrameIndex);

#define CVMJITcsTopOfInstructions(con) \
    (CVMJITcsContext(con)->topOfInstructions)
#define CVMJITCSInstructionStack(con) \
    (CVMJITcsContext(con)->instructionsStack)

#define CVMJITcsTestBaseRegister(con, reg) \
    (CVMJITcsBaseRegister(con) == reg)

#define CVMJITcsSetPutStaticFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_PUTSTATIC_INSTRUCTION)
        #define CVMJITcsSetGetStaticFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags |= CVMJITCS_GETSTATIC_INSTRUCTION)
       
#define CVMJITcsIsPutStaticFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_PUTSTATIC_INSTRUCTION)
#define CVMJITcsIsGetStaticFieldInstruction(con) \
    (CVMJITcsContext(con)->instructionFlags & CVMJITCS_GETSTATIC_INSTRUCTION)

#define CVMJITcsPutStaticFieldPC(con) \
    CVMJITcsContext(con)->putStaticFieldPC
#define CVMJITcsGetStaticFieldPC(con) \
    CVMJITcsContext(con)->getStaticFieldPC

extern void
CVMJITcsAlignInstruction(CVMJITCompilationContext* con);
extern void 
CVMJITcsInit(CVMJITCompilationContext* con);
extern void
CVMJITcsBeginBlock(CVMJITCompilationContext* con);
extern void
CVMJITcsEndBlock(CVMJITCompilationContext* con, CVMJITIRBlock* bk);
extern void
CVMJITcsPrepareForNextInstruction(CVMJITCompilationContext* con);
extern CVMInt32 
CVMJITcsCalcInstructionPos(CVMJITCompilationContext* con);
extern CVMJITCSConstantPoolDumpInfo*
CVMJITcsAllocateConstantPoolDumpInfoEntry(CVMJITCompilationContext* con);
extern void
CVMJITcsCommitMethod(CVMJITCompilationContext* con);

#else /* !IAI_CODE_SCHEDULER_SCORE_BOARD */

#define CVMJITcsPushSourceRegister(con, reg) do {}while(0);

#define CVMJITcsSetDestRegister(con, reg) do {}while(0);
#define CVMJITcsSetBranchOffset(con, offset) do {} while(0);
#define CVMJITcsSetCompareInstruction(con, setcc) do {} while(0);
#define CVMJITcsSetDestRegister2(con, reg) do {} while(0);
#define CVMJITcsSetOutGoingRegisters(con, index) do {} while(0);
#define CVMJITcsSetStatusBinaryALUInstruction(con, opcode) do {} while(0);

#define CVMJITcsSetJavaFrameIndex(con, javaFrameIndex) do {} while(0);

#define CVMJITcsSetLoadInstruction(con)  do {} while(0);
#define CVMJITcsSetStoreInstruction(con) do {} while(0);
#define CVMJITcsSetStatusInstructionInternal(con) do {} while(0);
#define CVMJITcsSetBranchInstruction(con) do {} while(0);
#define CVMJITcsSetCompareInstructionInternal(con) do {} while(0);
#define CVMJITcsSetNeedFixupInstruction(con) do {} while(0);
#define CVMJITcsSetPutFieldInstruction(con) do {} while(0);
#define CVMJITcsSetGetFieldInstruction(con) do {} while(0);
#define CVMJITcsSetPutArrayInstruction(con) do {} while(0);
#define CVMJITcsSetGetArrayInstruction(con) do {} while(0);
#define CVMJITcsSetStatusInstruction(con, condCode) do {} while(0);
#define CVMJITcsSetBranchLink(con, link) do {} while(0);
#define CVMJITcsSetCondCode(con, condCode) do {} while(0);
#define CVMJITcsSetPutStaticFieldInstruction(con) do {} while(0);
#define CVMJITcsSetGetStaticFieldInstruction(con) do {} while(0);
#define CVMJITcsSetExceptionInstruction(con) do{} while(0);
#define CVMJITcsSetArrayIndexOutofBoundsBranch(con) do{} while(0);

#define CVMJITcsSetEmitPatchInstruction(con) do {} while(0);
#define CVMJITcsSetEmitInsertInstruction(con) do {} while(0);
#define CVMJITcsSetEmitInPlace(con) do {} while(0);
#define CVMJITcsSetEmitInPlaceWithBufSizeAdjust(con) do {} while(0);

#define CVMJITcsTestBaseRegister(con, reg) CVM_FALSE

#define CVMJITcsClearEmitPatchInstruction(con) do {} while(0);
#define CVMJITcsClearEmitInsertInstruction(con) do {} while(0);
#define CVMJITcsClearEmitInPlace(con) do {} while(0);
#define CVMJITcsSetBaseRegister(con, reg) do {} while(0);
#define CVMJITcsBeginBlock(con) do {} while(0);
#define CVMJITcsInit(con) do {} while(0);
#define CVMJITcsPrepareForNextInstruction(con) do {} while(0);
#define CVMJITcsCommitMethod(con) do {} while(0);

#endif /* !IAI_CODE_SCHEDULER_SCORE_BOARD */
#endif
