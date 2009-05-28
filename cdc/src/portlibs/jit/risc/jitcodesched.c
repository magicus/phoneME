/*
 * @(#)jitcodesched.c	1.8 06/10/10
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

#include "javavm/include/iai_opt_config.h"
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/globals.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitstackmap.h"
#include "javavm/include/jit/jitfixup.h"
#include "javavm/include/jit/jitcodesched.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitmemory.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"
#include "portlibs/jit/risc/include/export/jitregman.h"
#include "portlibs/jit/risc/jitstackman.h"

#include "javavm/include/clib.h"

typedef struct {
    union {
	struct {
	    CVMUint32 offset_12:12;
	    CVMUint32 Rd:4;
	    CVMUint32 Rn:4;
	    CVMUint32 Lbit:1;
	    CVMUint32 Wbit:1;
	    CVMUint32 Bbit:1;
	    CVMUint32 Ubit:1;
	    CVMUint32 Pbit:1;
	    CVMUint32 bit25:1;
	    CVMUint32 bit2627:2;
	    CVMUint32 cond:4;
	} ldrInstructionBits;
	CVMUint32 instr;
    } u;
} CVMARMLdrStrInstruction;

struct CVMJITCSPatchInstruction{
    CVMUint32  flags;
    CVMUint32  targetAddress;
    CVMUint32  logicalPC;
};

#undef MAX
#define MAX(a, b) (a > b ? a : b)

#define CVMARMisNOP2Instruction(instr) \
    (instr == CVMCPU_NOP2_INSTRUCTION)

#define CVMARMgetLdrTargetOffsetAddress(instr)	\
    ((instr).u.ldrInstructionBits.Ubit		\
     ? (instr.u.ldrInstructionBits.offset_12)	\
     : -(instr.u.ldrInstructionBits.offset_12))

#define CVMARMisBranchInstruction(instr) \
    (((instr) & 0x0e000000) == 0x0a000000)

#define CVMARMisLdrInstruction(instr)		\
    ((((instr) & 0x0c100000) == 0x04100000) ||	\
     ((((instr) & 0x0e100090) == 0x00100090) && (((instr) & 0x00000060) != 0)))

#define CVMARMgetLdrInstructionBaseRegister(instr) \
    ((instr) & 0x000f0000)

#define CVMARMgetBranchTargetOffsetAddress(instr) \
    ((((instr) & 0x800000)			  \
      ? ((instr) | 0xff800000)			  \
      : ((instr) & 0x007fffff)) << 2)

#define CVM_XSCALE_LDR_LATENCY 3

#define MAX_TEMP_INSTRUCTION_BUFFER (4*1024)

void
CVMJITcsInitInstructionStack(CVMJITCompilationContext* con)
{
    CVMJITCSInstructionStack(con) = 
	CVMJITmemNew(con, JIT_ALLOC_CODE_SCHEDULING, 
		     MAX_TEMP_INSTRUCTION_BUFFER);
    CVMJITcsTopOfInstructions(con) = 0;
}

void
CVMJITcsInitNOPsArray(CVMJITCompilationContext* con)
{
    CVMJITgarrInit(con, CVMJITcsNopsArray(con), sizeof(CVMUint32));
}

void
CVMJITcsPushNOPInstruction(CVMJITCompilationContext* con, CVMUint32 logicalPC)
{
    CVMUint32* item;
    void* garrElem;
    CVMJITgarrNewElem(con, CVMJITcsNopsArray(con), garrElem);
    item = (CVMUint32*)garrElem;
    *item = logicalPC;
}

void
CVMJITcsPopNOPInstruction(CVMJITCompilationContext* con)
{
    CVMJITgarrPopElem(con, CVMJITcsNopsArray(con));
}

void
CVMJITCSInitConstantPoolEntry(CVMJITCompilationContext* con)
{
    CVMJITCSConstantPoolDumpInfo* item; 
    CVMJITgarrInit(con, CVMJITcsConstantPoolArray(con), 
		   sizeof(CVMJITCSConstantPoolDumpInfo));
    item = CVMJITcsAllocateConstantPoolDumpInfoEntry(con);
    item->startPC = (CVMUint32)CVMJITcbufGetLogicalPC(con);
    item->endPC = (CVMUint32)CVMJITcbufGetLogicalPC(con);
}

CVMJITCSConstantPoolDumpInfo*
CVMJITcsAllocateConstantPoolDumpInfoEntry(CVMJITCompilationContext* con)
{
    void* item;
    CVMJITgarrNewElem(con, CVMJITcsConstantPoolArray(con), item);
    return (CVMJITCSConstantPoolDumpInfo*)item;
}

void 
CVMJITcsPushInstruction(CVMJITCompilationContext* con, CVMUint32 flags, 
			CVMUint32 logicalPC, CVMUint32 targetAddress)
{
    CVMUint32* NOPs;
 
    if (CVMJITcsTopOfInstructions(con) ==
	(MAX_TEMP_INSTRUCTION_BUFFER / sizeof(CVMJITCSPatchInstruction)))
    {
	/*
	 *  Fixup the possible instruction
	 */
	int i;
	CVMUint32 end = CVMJITcsTopOfInstructions(con);
	CVMJITcsTopOfInstructions(con) = 0;
	CVMJITcsPushNOPInstruction(con,
				   (CVMUint32)CVMJITcbufGetLogicalPC(con));
	NOPs = CVMJITgarrGetElems(con, CVMJITcsNopsArray(con));
	for (i = 0; i < end; i++) {
	    if (CVMJITCSInstructionStack(con)[i].targetAddress < 
		(CVMUint32)CVMJITcbufGetLogicalPC(con))
	    {
		CVMUint32 numOfNopsFounded;

		for (numOfNopsFounded = 0;
		     CVMJITCSInstructionStack(con)[i].targetAddress > 
			 NOPs[numOfNopsFounded];
		     numOfNopsFounded++);

		CVMJITCSInstructionStack(con)[i].targetAddress -=
		    numOfNopsFounded * sizeof(CVMCPUInstruction);

		CVMtraceJITCodegen(("fix up instruction @ (%d) to %d\n", 
                    CVMJITCSInstructionStack(con)[i].logicalPC,
                    CVMJITCSInstructionStack(con)[i].targetAddress));

		CVMJITfixupAddress(con,
		    CVMJITCSInstructionStack(con)[i].logicalPC,
		    CVMJITCSInstructionStack(con)[i].targetAddress,
		    CVMJITCSInstructionStack(con)[i].flags);
	    } else {
		CVMJITCSInstructionStack(con)[CVMJITcsTopOfInstructions(con)] =
		    CVMJITCSInstructionStack(con)[i];
		CVMJITcsTopOfInstructions(con)++;
	    }
	}
	CVMJITcsPopNOPInstruction(con);
    }

    if (CVMJITcsTopOfInstructions(con) < 
	(MAX_TEMP_INSTRUCTION_BUFFER / sizeof(CVMJITCSPatchInstruction)))
    {
	CVMJITCSInstructionStack(con)
	    [CVMJITcsTopOfInstructions(con)].flags = flags;
          
	CVMJITCSInstructionStack(con)
	    [CVMJITcsTopOfInstructions(con)].logicalPC = logicalPC;
          
	CVMJITCSInstructionStack(con)
	    [CVMJITcsTopOfInstructions(con)].targetAddress = targetAddress;

	CVMJITcsTopOfInstructions(con)++;
    } else {
	/* This should be rare. */
	CVMJITerror(con, CANNOT_COMPILE,
	    "CANNOT handle too many patch-necessary instructions");
    }
}


void
CVMJITcsCommitMethod(CVMJITCompilationContext* con)
/*
 *  Constant Pool Table is required
 */
{
    int i;
    /*
     * first scan
     */
    CVMUint32 curLogicalPC;
    CVMUint32 startAddress;
    CVMUint32 endAddress;
    CVMUint32 curConstantPoolEntryIndex;
    CVMUint32 numberOfConstantPoolEntries;

    CVMUint32* NOPs;
    CVMUint32 numOfNopsCounted;

    CVMJITCSConstantPoolDumpInfo *constantPoolEntries;
    CVMJITStackmapItem* smap;
    CVMCPUInstruction* patchedInstructions;

#ifdef IAI_CACHEDCONSTANT
    CVMJITConstantEntry *thisEntry;
    CVMJITFixupElement *thisRef;
#endif
    
#ifdef CVM_TRACE_JIT
    CVMUint32 numOfNops;
#endif
    
    CVMJITCSConstantPoolDumpInfo* item;
    CVMJITcsInitInstructionStack(con);
    CVMJITcsInitNOPsArray(con);
    item = CVMJITcsAllocateConstantPoolDumpInfoEntry(con);


    endAddress = CVMJITcbufGetLogicalPC(con);
    item->startPC = (CVMUint32)CVMJITcbufGetLogicalPC(con);
    item->endPC = (CVMUint32)CVMJITcbufGetLogicalPC(con);

    constantPoolEntries = 
	CVMJITgarrGetElems(con, CVMJITcsConstantPoolArray(con));
    numberOfConstantPoolEntries = 
	CVMJITgarrGetNumElems(con, CVMJITcsConstantPoolArray(con));

    startAddress                = constantPoolEntries[0].startPC;
    CVMJITcbufGetLogicalPC(con) = constantPoolEntries[0].startPC;

    for (curConstantPoolEntryIndex = 0; 
	 curConstantPoolEntryIndex < numberOfConstantPoolEntries - 1; 
	 curConstantPoolEntryIndex++) {

	for (curLogicalPC =
		 constantPoolEntries[curConstantPoolEntryIndex].endPC;
	     curLogicalPC <
		 constantPoolEntries[curConstantPoolEntryIndex + 1].startPC;
	     curLogicalPC += sizeof(CVMCPUInstruction))
	{
            CVMCPUInstruction* curPhysicalPC = (CVMCPUInstruction*)
		CVMJITcbufLogicalToPhysical(con, curLogicalPC);
            CVMCPUInstruction* targetPhysicalPC = (CVMCPUInstruction*)
		CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));

	    if (CVMARMisNOP2Instruction(*curPhysicalPC)) {
		CVMJITcsPushNOPInstruction(con, curLogicalPC);
	    } else {
		*targetPhysicalPC = *curPhysicalPC;
		if (CVMARMisBranchInstruction(*curPhysicalPC))
		{
		    CVMUint32 targetAddress = 
			CVMARMgetBranchTargetOffsetAddress(*curPhysicalPC)
			+ curLogicalPC + 8;

		    if (targetAddress > startAddress &&
			targetAddress < endAddress)
		    {
			CVMJITcsPushInstruction(con,
                            CVMJIT_BRANCH_ADDRESS_MODE, 
                            CVMJITcbufGetLogicalPC(con), targetAddress);
		    } else {
			CVMtraceJITCodegen((
			    "fix up instruction @ (%d,%d) to %d\n", 
			    CVMJITcbufGetLogicalPC(con), curLogicalPC,
			    targetAddress));

			CVMJITfixupAddress(con, 
                            CVMJITcbufGetLogicalPC(con), 
			    targetAddress, CVMJIT_BRANCH_ADDRESS_MODE);
		    }
		} else if (CVMARMisLdrInstruction(*curPhysicalPC)) {
		    CVMARMLdrStrInstruction ldrInstruction;

		    ldrInstruction.u.instr = *curPhysicalPC;

		    if ((ldrInstruction.u.ldrInstructionBits.Rn == CVMARM_PC)&&
			(ldrInstruction.u.ldrInstructionBits.bit25 == 0))
		    {
			CVMUint32 targetAddress = 
			    CVMARMgetLdrTargetOffsetAddress(ldrInstruction) + 
			    curLogicalPC + 8;

			CVMJITcsPushInstruction(con, 
			    CVMJIT_MEMSPEC_ADDRESS_MODE, 
			    CVMJITcbufGetLogicalPC(con), targetAddress);
		    }
		}
		CVMJITcbufGetLogicalPC(con) += sizeof(CVMCPUInstruction);
	    }
	}

	for (curLogicalPC =
		 constantPoolEntries[curConstantPoolEntryIndex + 1].startPC;
	     curLogicalPC <
		 constantPoolEntries[curConstantPoolEntryIndex + 1].endPC;
	     curLogicalPC += sizeof(CVMCPUInstruction))
	{
            CVMCPUInstruction* targetPhysicalPC = (CVMCPUInstruction*)
		CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));
            CVMCPUInstruction* curPhysicalPC = (CVMCPUInstruction*)
		CVMJITcbufLogicalToPhysical(con, curLogicalPC);

	    *targetPhysicalPC = *curPhysicalPC;
	    CVMJITcbufGetLogicalPC(con) += sizeof(CVMCPUInstruction);
	}
    }

    CVMJITcsPushNOPInstruction(con, endAddress);

    /*
     * MAP_PC table 
     */
    NOPs = CVMJITgarrGetElems(con, CVMJITcsNopsArray(con));

#ifdef CVM_TRACE_JIT
    CVMtraceJITCodegen(("Begin dumping Nops\n"));
    numOfNops = CVMJITgarrGetNumElems(con, CVMJITcsNopsArray(con));
    for (i = 0; i < numOfNops; i++) {
        CVMtraceJITCodegen(("Nops: %d\n", NOPs[i]));
    }
    CVMtraceJITCodegen(("End dumping Nops\n"));
#endif

    numOfNopsCounted = 0;
    for (i = 0; i < con->pcMapTable->numEntries; i++) {
        for(; con->pcMapTable->maps[i].compiledPc > NOPs[numOfNopsCounted];
	    numOfNopsCounted++); 
        con->pcMapTable->maps[i].compiledPc -= 
            (numOfNopsCounted)*sizeof(CVMCPUInstruction);
    }

#ifdef CVM_TRACE_JIT
    for (i = 0; i < con->pcMapTable->numEntries; i++) {
        CVMtraceJITCodegen(("MAP_PC javaPc=%d compiledPc=%d\n", 
			    con->pcMapTable->maps[i].javaPc, 
			    con->pcMapTable->maps[i].compiledPc));
    }
#endif

/*
 * Stack MAP table
 */

#ifdef CVM_TRACE_JIT
    for (smap = con->stackmapList; smap != NULL; smap = smap->next) {
        CVMtraceJITCodegen(("StackMap pcOffset=%d\n", 
            smap->pcOffset - (con->codeEntry - con->codeBufAddr)));
    }
#endif

    numOfNopsCounted = 0;
    for (smap = con->stackmapList; smap != NULL; smap = smap->next) {
        for(; smap->pcOffset > NOPs[numOfNopsCounted]; numOfNopsCounted++); 
        smap->pcOffset -= (numOfNopsCounted) * sizeof(CVMCPUInstruction);
    }

#ifdef CVM_TRACE_JIT
    for (smap = con->stackmapList; smap != NULL; smap = smap->next) {
        CVMtraceJITCodegen(("Fixed stackMap pcOffset=%d\n", 
            smap->pcOffset - (con->codeEntry - con->codeBufAddr)));
    }
#endif

    /*
     * pactchInstruction
     */
#ifdef CVM_TRACE_JIT
    for (i = 0; i < con->gcCheckPcsSize; i++) {
        CVMtraceJITCodegen(("gc check pc:%d\n", 
			    con->gcCheckPcs->pcEntries[i] - 
			    (con->codeEntry - con->codeBufAddr)));
    }
#endif

    numOfNopsCounted = 0;
    patchedInstructions = (CVMCPUInstruction*)
        (((CVMUint32)&con->gcCheckPcs->pcEntries[con->gcCheckPcsSize]
          + sizeof(CVMCPUInstruction) - 1)
         & ~(sizeof(CVMCPUInstruction)-1));

    for (i = 0; i < con->gcCheckPcsSize; i++) {
        if (con->gcCheckPcs->pcEntries[i] != 0) {
            CVMUint32 numOfNopsCountedForTargetAddress;
            CVMUint32 targetAddress =
		CVMARMgetBranchTargetOffsetAddress(patchedInstructions[i]) + 
		con->gcCheckPcs->pcEntries[i] -
		(con->codeEntry - con->codeBufAddr) + 8;

            for(; (con->gcCheckPcs->pcEntries[i] - 
		   (con->codeEntry - con->codeBufAddr)) >
		    NOPs[numOfNopsCounted]; 
		numOfNopsCounted++); 

            con->gcCheckPcs->pcEntries[i] -=
		(numOfNopsCounted)*sizeof(CVMCPUInstruction);

            if (targetAddress > startAddress && targetAddress < endAddress) {
		for(numOfNopsCountedForTargetAddress= 0; 
		    targetAddress > NOPs[numOfNopsCountedForTargetAddress]; 
		    numOfNopsCountedForTargetAddress++); 

		targetAddress -=
		    numOfNopsCountedForTargetAddress*sizeof(CVMCPUInstruction);
            }

            patchedInstructions[i] =
		CVMCPUfixupInstructionAddress(con,
		    patchedInstructions[i], 
		    con->gcCheckPcs->pcEntries[i]
		        - (con->codeEntry - con->codeBufAddr), 
                    targetAddress, CVMJIT_BRANCH_ADDRESS_MODE);
        }
    }

    /*
     * Inlining Table
     */
    for (i = 0; i < con->numInliningInfoEntries; i++) {
        numOfNopsCounted = 0;
        for(; con->inliningInfo->entries[i].pcOffset1 > NOPs[numOfNopsCounted];
	    numOfNopsCounted++); 
        con->inliningInfo->entries[i].pcOffset1 -=
	    (numOfNopsCounted)*sizeof(CVMCPUInstruction);
        for(; con->inliningInfo->entries[i].pcOffset2 > NOPs[numOfNopsCounted];
	    numOfNopsCounted++); 
        con->inliningInfo->entries[i].pcOffset2 -=
	    (numOfNopsCounted)*sizeof(CVMCPUInstruction);
    }

    /*
     * branch and ldr instruction
     */
    for (i = 0; i < CVMJITcsTopOfInstructions(con); i++) {
        for (numOfNopsCounted = 0;
	     CVMJITCSInstructionStack(con)[i].targetAddress > 
		 NOPs[numOfNopsCounted];
	     numOfNopsCounted++);
        CVMJITCSInstructionStack(con)[i].targetAddress -=
	    numOfNopsCounted * sizeof(CVMCPUInstruction);
        CVMJITfixupAddress(con,
	    CVMJITCSInstructionStack(con)[i].logicalPC, 
            CVMJITCSInstructionStack(con)[i].targetAddress,
            CVMJITCSInstructionStack(con)[i].flags);
    }

    for (numOfNopsCounted = 0; 
         con->intToCompOffset > NOPs[numOfNopsCounted]; 
         numOfNopsCounted++);

    con->intToCompOffset -= numOfNopsCounted * sizeof(CVMCPUInstruction);

    /*
     * Fix the cached constant (checkcast and instanceOf) instructions
     */
#ifdef IAI_CACHEDCONSTANT
    thisEntry = GET_CONSTANT_POOL_LIST_HEAD(con);
    while(thisEntry != NULL) {
        if (thisEntry->isCachedConstant) {
            thisRef = thisEntry->references;

	    for (numOfNopsCounted = 0;
		 thisEntry->address > NOPs[numOfNopsCounted];
		 numOfNopsCounted++);
	    thisEntry->address -= numOfNopsCounted * sizeof(CVMCPUInstruction);

	    for (numOfNopsCounted = 0;
		 thisRef->logicalAddress > NOPs[numOfNopsCounted];
		 numOfNopsCounted++);
	    thisRef->logicalAddress -=
		numOfNopsCounted * sizeof(CVMCPUInstruction);

	    CVMJITcsSetEmitInPlace(con);
	    CVMtraceJITCodegen((
	        ":::::Fixed instruction at %d to reference %d\n",
	        thisRef->logicalAddress, thisEntry->address));
	    CVMJITcbufPushFixup(con, thisRef->logicalAddress);
	    CVMJITemitLoadConstantAddress(con, thisEntry->address);
	    CVMJITcbufPop(con);
	    CVMJITcsClearEmitInPlace(con);
        }
        thisEntry = thisEntry->next;
    }
#endif

    CVMJITcbufGetPhysicalPC(con) = 
	CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));
}

void 
CVMJITcsInit(CVMJITCompilationContext* con)
{
    int i;
    CVMJITcsGetFieldPC(con) = 0;
    CVMJITcsPutFieldPC(con) = 0;
    CVMJITcsGetArrayElementPC(con) = 0;
    CVMJITcsPutArrayElementPC(con) = 0;

#ifdef CVM_DEBUG
    if (CVMglobals.jit.codeSchedRemoveNOP) {
	CVMJITcsContext(con)->nop = CVMCPU_NOP2_INSTRUCTION;
    } else {
	CVMJITcsContext(con)->nop = CVMCPU_NOP3_INSTRUCTION;
    }
#else
    CVMJITcsContext(con)->nop = CVMCPU_NOP2_INSTRUCTION;
#endif

    CVMJITcsOutGoingRegisters(con, CVMCPU_JSP_REG) = CVM_TRUE;
    CVMJITcsOutGoingRegisters(con, CVMCPU_JFP_REG) = CVM_TRUE;
    CVMJITcsOutGoingRegisters(con, CVMCPU_SP_REG) = CVM_TRUE;

    {
	int arrayWords = CVMJITCS_TEMP_LOCAL_NUM * con->numberLocalWords;
	int arraySize = arrayWords * sizeof(CVMUint32);

	CVMJITcsUseJavaFrameBasedReferencePCArray(con) = 
	    CVMJITmemNew(con, JIT_ALLOC_CODE_SCHEDULING, arraySize);
	CVMJITcsDefJavaFrameBasedReferencePCArray(con) = 
	    CVMJITmemNew(con, JIT_ALLOC_CODE_SCHEDULING, arraySize);

	for (i = 0; i < arrayWords; i++) {
	    CVMJITcsDefJavaFrameBasedReferencePC(con, i) = 0;
	    CVMJITcsUseJavaFrameBasedReferencePC(con, i) = 0;
	}
    }

    CVMJITCSInitConstantPoolEntry(con); 
    CVMJITcsBeginBlock(con);
}

void
CVMJITcsBeginBlock(CVMJITCompilationContext* con)
{
    int i;
    CVMassert(!CVMglobals.jit.codeScheduling ||
	      (CVMJITcsContext(con)->emitInPlaceCount == 0));
    CVMJITcsClearInstructionFlags(con);
    CVMJITcsSetEmitInsertInstruction(con);
    CVMJITcsSourceRegsNum(con) = 0;
    CVMJITcsDestRegister(con) = CVMCPU_INVALID_REG;
    CVMJITcsDestRegister2(con) = CVMCPU_INVALID_REG;
    CVMJITcsStorePC(con) = CVMJITcbufGetLogicalPC(con);
    CVMJITcsStatusPC(con) = CVMJITcbufGetLogicalPC(con);
    CVMJITcsBranchPC(con) = CVMJITcbufGetLogicalPC(con);
    CVMJITcbufGetLogicalInstructionPC(con) = CVMJITcbufGetLogicalPC(con);
    CVMJITcbufGetPhysicalPC(con) =
	CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));
    CVMJITcsPutStaticFieldPC(con) = CVMJITcbufGetLogicalPC(con);
    CVMJITcsGetStaticFieldPC(con) = CVMJITcbufGetLogicalPC(con);

    for (i = 0; i < CVM_CPU_NUM_REGISTERS; i++) {
	CVMJITcsDefRegisterPC(con, i) = CVMJITcbufGetLogicalPC(con);
	CVMJITcsUseRegisterPC(con, i) = CVMJITcbufGetLogicalPC(con);
	CVMJITcsOutGoingRegisters(con, i) = CVM_FALSE;
    }

    CVMJITcsOutGoingRegisters(con, CVMCPU_JSP_REG) = CVM_TRUE;
    CVMJITcsOutGoingRegisters(con, CVMCPU_JFP_REG) = CVM_TRUE;
    CVMJITcsOutGoingRegisters(con, CVMCPU_SP_REG) = CVM_TRUE;

    CVMJITcsContext(con)->curBlockLogicalPC = CVMJITcbufGetLogicalPC(con);

    if (!CVMglobals.jit.codeScheduling) {
        CVMJITcsSetEmitInPlace(con);
    }
}

/*
 * Update the score board
 */
void
CVMJITcsPrepareForNextInstruction(CVMJITCompilationContext* con)
{
    CVMInt32 i;
    CVMUint32 instructionLogicalPC = CVMJITcbufGetLogicalInstructionPC(con);
    CVMInt32 registerPC = CVMJITcsIsLoadInstruction(con)
	? instructionLogicalPC
	  + CVM_XSCALE_LDR_LATENCY * sizeof(CVMCPUInstruction)
	: instructionLogicalPC + sizeof(CVMCPUInstruction);
    /*
     *  Once the instruction is to raise exception, the branch propeties should be removed
     */
/* IAI - 12 */
#ifdef IAI_CS_EXCEPTION_ENHANCEMENT
    if (CVMJITcsIsExceptionInstruction(con)) {
        (CVMJITcsContext(con)->instructionFlags &= ~CVMJITCS_BRANCH_INSTRUCTION);
    }
#endif 
/* IAI -12 */

    /*
     * The earliest place of the next instruction which has the dependency
     * of status bits should be next of this instruction.
     */
    if (CVMJITcsIsStatusInstruction(con)) {
        CVMJITcsStatusPC(con) = 
	    MAX(CVMJITcsStatusPC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The earliest place of the next instruction which has the dependency
     * of exception bits should be next of this instruction.
     */
    if (CVMJITcsIsExceptionInstruction(con)) {
        CVMJITcsBranchPC(con) =
	    MAX(CVMJITcsBranchPC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The earliest place of the next instruction which has the dependency of 
     * put static field bits should be next of this instruction.
     */
    if (CVMJITcsIsPutStaticFieldInstruction(con)) {
        CVMJITcsPutStaticFieldPC(con) = 
	    MAX(CVMJITcsPutStaticFieldPC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The earliest place of the next instruction which has the dependency of 
     * get static field instruction should be next of this instruction.
     */
    if (CVMJITcsIsGetStaticFieldInstruction(con)) {
        CVMJITcsGetStaticFieldPC(con) =
	    MAX(CVMJITcsGetStaticFieldPC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The earliest place of the next instruction which has the dependency of 
     * branch instruction should be next of this instruction.
     * Note: as some register will flow to other block. Any instruction which 
     * modifies these registers is not allowed to be scheduling before the
     * branch instruction. Updating the defining register score board can
     * prevent such case.
     * SP, JSP, JFP are global visible registers.
     */
    if (CVMJITcsIsBranchInstruction(con)) {
        CVMassert(CVMJITcsOutGoingRegisters(con, CVMCPU_JSP_REG) == CVM_TRUE);
        CVMassert(CVMJITcsOutGoingRegisters(con, CVMCPU_JFP_REG) == CVM_TRUE);
        CVMassert(CVMJITcsOutGoingRegisters(con, CVMCPU_SP_REG) == CVM_TRUE);
        CVMJITcsBranchPC(con) = 
	    MAX(CVMJITcsBranchPC(con), 
		instructionLogicalPC + sizeof(CVMCPUInstruction));
        for (i = 0; i < CVM_CPU_NUM_REGISTERS; i++) {
	    if (CVMJITcsOutGoingRegisters(con, i)) {
		CVMJITcsUseRegisterPC(con, i) =
                    instructionLogicalPC + sizeof(CVMCPUInstruction);
	    }
	    CVMJITcsOutGoingRegisters(con, i) = CVM_FALSE;
        }
        CVMJITcsOutGoingRegisters(con, CVMCPU_JSP_REG) = CVM_TRUE;
        CVMJITcsOutGoingRegisters(con, CVMCPU_JFP_REG) = CVM_TRUE;
        CVMJITcsOutGoingRegisters(con, CVMCPU_SP_REG) = CVM_TRUE;
    }

    /*
     * The earliest place of the next instruction which has the dependency of 
     * store instruction should be next of this instruction.
     */
    if (CVMJITcsIsStoreInstruction(con)) {
        CVMJITcsStorePC(con) = 
	    MAX(CVMJITcsStorePC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The following two if statements targets on updating the destination 
     * register score board.
     */
    if ((CVMJITcsDestRegister(con) != CVMCPU_INVALID_REG) &&
	!CVMJITcsIsEmitPatchInstruction(con))
    {
        CVMassert((CVMJITcsIsEmitInPlace(con)) ||
		  (registerPC >=
		   CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister(con))));
        CVMJITcsDefRegisterPC(con, CVMJITcsDestRegister(con)) = registerPC;
        CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister(con)) =
	    MAX(CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister(con)),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }
    
    if ((CVMJITcsDestRegister2(con) != CVMCPU_INVALID_REG) &&
         !CVMJITcsIsEmitPatchInstruction(con))
    {
        CVMassert((CVMJITcsIsEmitInPlace(con)) ||
		  (registerPC >= 
		   CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister2(con))));
        CVMJITcsDefRegisterPC(con, CVMJITcsDestRegister2(con)) = 
	    instructionLogicalPC + sizeof(CVMCPUInstruction);
        CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister2(con)) =
	    MAX(CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister2(con)),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The following if statements targets on updating the source 
     * register score board.
     */
    for (i = 0; i < CVMJITcsSourceRegsNum(con); i++) {
	CVMJITcsUseRegisterPC(con, CVMJITcsSourceRegister(con, i)) = 
	    MAX(CVMJITcsUseRegisterPC(con, CVMJITcsSourceRegister(con, i)),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The following two if statements target on frame based load/store score
     * board updating.
     */
    if (CVMJITcsIsStoreInstruction(con) &&
        (CVMJITcsBaseRegister(con) == CVMCPU_JFP_REG) &&
        (CVMJITcsJavaFrameIndex(con) >= 0) &&
        (CVMJITcsJavaFrameIndex(con) <
	 CVMJITCS_TEMP_LOCAL_NUM * con->numberLocalWords))
    {
        CVMJITcsDefJavaFrameBasedReferencePC(con, CVMJITcsJavaFrameIndex(con))=
            instructionLogicalPC + sizeof(CVMCPUInstruction);
    }

    if ((CVMJITcsIsLoadInstruction(con)) &&
        (CVMJITcsBaseRegister(con) == CVMCPU_JFP_REG) &&
        (CVMJITcsJavaFrameIndex(con) >= 0) &&
        (CVMJITcsJavaFrameIndex(con) <
	 CVMJITCS_TEMP_LOCAL_NUM * con->numberLocalWords))
    {
        CVMJITcsUseJavaFrameBasedReferencePC(con, CVMJITcsJavaFrameIndex(con))=
	    instructionLogicalPC + sizeof(CVMCPUInstruction);
    }

    /*
     * The earliest place of the next instruction which has the
     * dependency of compare instruction (set status bits) should be
     * next of this instruction.
     */
    if (CVMJITcsIsCompareInstruction(con)) {
	CVMJITcsComparePC(con) =
	    MAX(CVMJITcsComparePC(con),
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * The following four if statements targets on update get/put
     * field, get/store array score board.
     */
    if (CVMJITcsIsPutFieldInstruction(con)) {
	CVMJITcsPutFieldPC(con) = 
	    MAX(CVMJITcsPutFieldPC(con), 
		instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    if (CVMJITcsIsGetFieldInstruction(con)) {
          CVMJITcsGetFieldPC(con) = 
	      MAX(CVMJITcsGetFieldPC(con), 
		 instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    if (CVMJITcsIsPutArrayInstruction(con)) {
          CVMJITcsPutArrayElementPC(con) =
	      MAX(CVMJITcsPutArrayElementPC(con),
                 instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    if (CVMJITcsIsGetArrayInstruction(con)) {
          CVMJITcsGetArrayElementPC(con) = 
	      MAX(CVMJITcsGetArrayElementPC(con), 
		 instructionLogicalPC + sizeof(CVMCPUInstruction));
    }

    /*
     * reset all the transition fields
     */
    CVMJITcsClearInstructionFlags(con);
    CVMJITcsSetEmitInsertInstruction(con);
    CVMJITcsSourceRegsNum(con) = 0;
    CVMJITcsDestRegister(con) = CVMCPU_INVALID_REG;
    CVMJITcsDestRegister2(con) = CVMCPU_INVALID_REG;
    CVMJITcsJavaFrameIndex(con) = -1;
}

CVMInt32 
CVMJITcsCalcInstructionPos(CVMJITCompilationContext* con)
{
    CVMInt32 i;
    CVMInt32 instructionPC = 0;

    if (CVMJITcsIsEmitPatchInstruction(con)) {
	return CVMJITcbufGetLogicalInstructionPC(con);
    }

    if (CVMJITcsIsEmitInPlace(con)) {
	instructionPC = CVMJITcbufGetLogicalPC(con);
	CVMJITcbufGetLogicalPC(con) += sizeof(CVMCPUInstruction);
	CVMJITcbufGetPhysicalPC(con) = 
	    CVMJITcbufLogicalToPhysical(con, CVMJITcbufGetLogicalPC(con));
	CVMJITcbufGetLogicalInstructionPC(con) = instructionPC;
	return instructionPC;
    }

    CVMassert(CVMJITcsIsEmitInsertInstruction(con));
    if ((CVMJITcsIsBranchInstruction(con))) {
	instructionPC = MAX(instructionPC, CVMJITcsBranchPC(con));
	instructionPC = MAX(instructionPC, CVMJITcsStorePC(con));
    }

    if (CVMJITcsIsExceptionInstruction(con)) {
	instructionPC = MAX(instructionPC, CVMJITcsBranchPC(con));
	instructionPC = MAX(instructionPC, CVMJITcsStorePC(con));
    }

    if ((CVMJITcsIsStoreInstruction(con))) {
	instructionPC = MAX(instructionPC, CVMJITcsBranchPC(con));
    }

    if ((CVMJITcsBaseRegister(con) == CVMCPU_JFP_REG)) {
        if ((CVMJITcsIsStoreInstruction(con))) {
	    CVMassert(CVMJITcsJavaFrameIndex(con) >= 0);
	    CVMassert(CVMJITcsJavaFrameIndex(con) <
		      CVMJITCS_TEMP_LOCAL_NUM * con->numberLocalWords);
	    instructionPC = MAX(instructionPC,
				CVMJITcsUseJavaFrameBasedReferencePC(
		        	    con, CVMJITcsJavaFrameIndex(con)));
	    instructionPC = MAX(instructionPC,
				CVMJITcsDefJavaFrameBasedReferencePC(
	        		    con, CVMJITcsJavaFrameIndex(con)));
        } else if ((CVMJITcsIsLoadInstruction(con))) {
	    CVMassert(CVMJITcsJavaFrameIndex(con) >= 0);
	    CVMassert(CVMJITcsJavaFrameIndex(con) <
		      CVMJITCS_TEMP_LOCAL_NUM * con->numberLocalWords);
	    instructionPC = MAX(instructionPC,
				CVMJITcsDefJavaFrameBasedReferencePC(
	         		    con, CVMJITcsJavaFrameIndex(con)));
        } 
    }

    CVMassert(CVMJITcsSourceRegsNum(con) < 4);
    CVMassert(CVMJITcsSourceRegsNum(con) >= 0);

    for (i = 0; i < CVMJITcsSourceRegsNum(con); i++) {
	CVMassert(CVMJITcsSourceRegister(con, i) != CVMCPU_INVALID_REG);
	instructionPC =
	    MAX(instructionPC,
		CVMJITcsDefRegisterPC(con, CVMJITcsSourceRegister(con, i)));
    }

    if (CVMJITcsDestRegister(con) != CVMCPU_INVALID_REG) {
	instructionPC =
	    MAX(instructionPC,
		CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister(con)));
    }

    if (CVMJITcsDestRegister2(con) != CVMCPU_INVALID_REG) {
	instructionPC =
	    MAX(instructionPC,
		CVMJITcsUseRegisterPC(con, CVMJITcsDestRegister2(con)));
    }

    if (CVMJITcsIsCompareInstruction(con)) {
	instructionPC = MAX(instructionPC, CVMJITcsStatusPC(con));
    }

    if (CVMJITcsIsStatusInstruction(con) || con->inConditionalCode) {
	instructionPC = MAX(instructionPC, CVMJITcsComparePC(con));
    }

    if (CVMJITcsIsPutFieldInstruction(con)){
	instructionPC = MAX(instructionPC, CVMJITcsGetFieldPC(con));
	instructionPC = MAX(instructionPC, CVMJITcsPutFieldPC(con));;
    } else if (CVMJITcsIsGetFieldInstruction(con)){
	instructionPC = MAX(instructionPC, CVMJITcsPutFieldPC(con));
    } else if (CVMJITcsIsPutArrayInstruction(con)){
	instructionPC = MAX(instructionPC, CVMJITcsPutArrayElementPC(con));
	instructionPC = MAX(instructionPC, CVMJITcsGetArrayElementPC(con));
    } else if (CVMJITcsIsGetArrayInstruction(con)){
	instructionPC = MAX(instructionPC, CVMJITcsPutArrayElementPC(con));
    } else if ((CVMJITcsIsPutStaticFieldInstruction(con))) {
	instructionPC = MAX(instructionPC, CVMJITcsPutStaticFieldPC(con));
	instructionPC = MAX(instructionPC, CVMJITcsGetStaticFieldPC(con));
    } else if ((CVMJITcsIsGetStaticFieldInstruction(con))) {
	instructionPC = MAX(instructionPC, CVMJITcsPutStaticFieldPC(con));
    } 

    while(instructionPC < CVMJITcbufGetLogicalPC(con)) {
	if (*(CVMCPUInstruction*)
	    CVMJITcbufLogicalToPhysical(con, instructionPC) ==
	    CVMJITcsContext(con)->nop)
	{
	    break;
	}
	instructionPC += sizeof(CVMCPUInstruction);
    }

    if (instructionPC >= CVMJITcbufGetLogicalPC(con)) {
	for (i = CVMJITcbufGetLogicalPC(con); 
	     i <= instructionPC; 
	     i += sizeof(CVMCPUInstruction))
	{
	    CVMJITcbufEmitPC(con, i, CVMJITcsContext(con)->nop);
	}

	CVMJITcbufGetLogicalPC(con) =
	    instructionPC + sizeof(CVMCPUInstruction);
	CVMJITcbufGetPhysicalPC(con) =
	    CVMJITcbufLogicalToPhysical(
		con, instructionPC + sizeof(CVMCPUInstruction));
    }

    CVMJITcbufGetLogicalInstructionPC(con) = instructionPC;

    return instructionPC;
}
