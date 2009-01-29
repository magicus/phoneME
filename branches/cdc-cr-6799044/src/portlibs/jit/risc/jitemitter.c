/*
 * @(#)jitemitter.c	1.35 06/10/10
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

/*
 * This file implements part of the risc jit emitter porting layer. It
 * implements all of the conditonal instructions, and is meant to be
 * used by platforms that don't support conditional instructions other
 * than branches. This probably means every platform other than ARM
 * will use the versions of the conditional emitters in this file.
 */
#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#include "javavm/include/ccee.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitirnode.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"
#include "portlibs/jit/risc/include/porting/ccmrisc.h"

/* Purpose: Patches prologue for the method */
void
CVMCPUemitMethodProloguePatch(CVMJITCompilationContext *con,
                              CVMCPUPrologueRec* rec)
{
    CVMUint32 capacity;
    CVMUint32 opstackOffset;
    CVMUint32 boundsOffset;
    CVMUint32 argsSize;
    CVMUint32 spillAdjust;
   
    /* 1. TOS adjustment. */
    argsSize = CVMmbArgsSize(con->mb);
    capacity = con->capacity + con->maxTempWords;
    /*
     * Instead of the TOS register, we are going to be using 
     * JSP - argSize << 2
     */
    boundsOffset = capacity - argsSize;

    CVMJITcbufPushFixup(con, rec->capacityStartPC);

    CVMJITcsSetEmitInPlace(con);
    /* Emit adjustment of TOS by capacity: */
    CVMJITprintCodegenComment(("Capacity is %d word(s)", capacity));
    CVMCPUemitALUConstant16Scaled(con, CVMCPU_ADD_OPCODE, CVMCPU_ARG2_REG,
                                  CVMCPU_JSP_REG, boundsOffset, 2);    
    CVMJITcsClearEmitInPlace(con);
    /* Make sure we are not exceeding the reserved space for TOS adjust */
    CVMassert (CVMJITcbufGetLogicalPC(con) <= rec->capacityEndPC);
    CVMJITcbufPop(con);

    /* 2. Spill adjustment. */
    opstackOffset = CVMoffsetof(CVMCompiledFrame, opstackX);
    spillAdjust = (con->maxTempWords << 2) + opstackOffset;

    /* change pc to the spill adjustment instruction */
    CVMJITcbufPushFixup(con, rec->spillStartPC);

    /* Emit spill adjustment: */
    CVMJITprintCodegenComment(("spillSize is %d word(s), add to JFP+%d",
                               con->maxTempWords, opstackOffset));
    CVMJITcsSetEmitInPlace(con);
    CVMCPUemitALUConstant16Scaled(con, CVMCPU_ADD_OPCODE,
                                  CVMCPU_JSP_REG, CVMCPU_JFP_REG,
                                  spillAdjust, 0);
    CVMJITcsClearEmitInPlace(con);
    /* Make sure we are not exceeding the reserved space for spill adjust */
    CVMassert(CVMJITcbufGetLogicalPC(con) <= rec->spillEndPC);

#ifdef CVMCPU_HAS_CP_REG
    {
#ifdef CVM_DEBUG_ASSERTS
        CVMInt32 cpLoadStartPC = CVMJITcbufGetLogicalPC(con);
#endif
        /* 3. Load Constant pool base register. */
        CVMCPUemitLoadConstantPoolBaseRegister(con);
#ifdef CVM_DEBUG_ASSERTS
        /* 
         * We better have not over-written the bounds of the reserved space
         * for the method prologue.
         */
        CVMassert(CVMJITcbufGetLogicalPC(con) - cpLoadStartPC <=
            CVMCPU_RESERVED_CP_REG_INSTRUCTIONS * CVMCPU_INSTRUCTION_SIZE);
#endif
    }
#endif

    CVMJITcbufPop(con);
}

/*
 * For non-synchronized invocations, we can finish the work
 * in the prologue:
 *
 *    str MB, [JFP, #OFFSET_CVMFrame_mb]
 *
 * As well as emit all the tracing related stuff.
 */
static void
emitRestOfSimpleInvocation(CVMJITCompilationContext* con, 
                           CVMBool needFlushPREV)
{
    CVMUint32 prevOffset;
    CVMUint32 mbOffset;

    prevOffset = CVMoffsetof(CVMCompiledFrame, frameX.prevX);
    mbOffset   = CVMoffsetof(CVMCompiledFrame, frameX.mb);

    CVMJITaddCodegenComment((con, "Store MB into frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                                       CVMCPU_ARG1_REG, /* mb */
                                       CVMCPU_JFP_REG,
                                       mbOffset);

#ifdef CVM_DEBUG
    /*
        mov  arg4, #CONSTANT_CVM_FRAMETYPE_NONE
        strb arg4, [JFP, #OFFSET_CVMFrame_type]
        mov  arg4, #-1
        strb arg4, [JFP, #OFFSET_CVMFrame_flags]
    */
    {
        CVMUint32 flagsOffset = CVMoffsetof(CVMFrame, flags);
        CVMJITprintCodegenComment(("DEBUG-ONLY CODE"));

#ifdef CVM_DEBUG_ASSERTS
	{
	    CVMUint32 typeOffset = CVMoffsetof(CVMFrame, type);
	    CVMCPUemitLoadConstant(con, CVMCPU_ARG4_REG, CVM_FRAMETYPE_NONE);
	    CVMJITaddCodegenComment((con, "Store NONE into frame.type"));
	    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR8_OPCODE,
					       CVMCPU_ARG4_REG, /* NONE */
					       CVMCPU_JFP_REG,
					       typeOffset);
	}
#endif

        CVMCPUemitLoadConstant(con, CVMCPU_ARG4_REG, -1);
        CVMJITaddCodegenComment((con, "Store -1 into frame.flags"));
        CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR8_OPCODE,
                                           CVMCPU_ARG4_REG, /* -1 */
                                           CVMCPU_JFP_REG,
                                           flagsOffset);

        CVMJITprintCodegenComment(("END OF DEBUG-ONLY CODE"));
    }

#endif
    /*
     * If we couldn't go the short route of  eliminating the use of PREV,
     * we need to flush PREV to the new frame.
     */
    if (needFlushPREV) {
        CVMJITaddCodegenComment((con, "Store curr JFP into new frame"));
        CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                                           CVMCPU_PROLOGUE_PREVFRAME_REG,
                                           CVMCPU_JFP_REG,
                                           prevOffset);
    }
#ifdef CVM_TRACE
    {
        CVMJITprintCodegenComment(("DEBUG-ONLY CODE FOR TRACING METHOD CALLS"));
#ifdef CVMCPU_EE_REG
	CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			       CVMCPU_ARG1_REG, CVMCPU_EE_REG, CVMJIT_NOSETCC);
#else
	{
	    CVMJITaddCodegenComment((con, "arg1 = ccee->ee"));
	    CVMCPUemitCCEEReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
		CVMCPU_ARG1_REG, CVMoffsetof(CVMCCExecEnv, eeX));

	}
#endif

        /* MOV arg2, JFP */
        CVMJITaddCodegenComment((con, "arg2 = JFP"));
        CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
                               CVMCPU_ARG2_REG, CVMCPU_JFP_REG,
                               CVMJIT_NOSETCC);
        CVMJITaddCodegenComment((con, "CVMCCMtraceMethodCallGlue()"));
        CVMCPUemitAbsoluteCall(con, (void*)CVMCCMtraceMethodCallGlue,
                               CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH);

        CVMJITprintCodegenComment(("END OF DEBUG-ONLY CODE FOR TRACING"));
    }
#endif

}

/* Purpose: Emits the prologue code for the compiled method. */
void
CVMCPUemitMethodPrologue(CVMJITCompilationContext *con,
                         CVMCPUPrologueRec* rec)
{
    CVMInt32 spillPC, prologueStart;
    CVMUint32 argsSize, spillAdjust;
    CVMUint8* startPC;
    CVMMethodBlock* mb;
    void*  invokerFunc;
    const char *invokerName;
    const char *jfpStr;
    int jfpReg;
    CVMBool simpleInvoke;
    CVMBool needFlushPREV = CVM_FALSE;
    CVMUint32 prevOffset;
    CVMUint32 jspOffset; /* The offset off of JSP to compute the new JFP */

    prevOffset = CVMoffsetof(CVMCompiledFrame, frameX.prevX);

    mb = con->mb;
    argsSize = CVMmbArgsSize(mb);
    startPC  = con->codeEntry;

    /* Add jspOffset * 4 to JSP to get the new JFP */
    jspOffset = con->numberLocalWords - argsSize;
    prologueStart = CVMJITcbufGetLogicalPC(con);

#ifndef CVMCPU_CHUNKEND_REG
    /*
     * Preload ccee->stackChunkEndX into CVMCPU_ARG4_REG for
     * better code sceduling purposes. It will be referenced by code
     * emited by CVMCPUemitStackLimitCheckAndStoreReturnAddr.
     */
    {
	CVMUint32 off = CVMoffsetof(CVMCCExecEnv, stackChunkEndX);
	CVMJITprintCodegenComment(("Stack limit check"));
	CVMJITaddCodegenComment((con, "ccee->stackChunkEnd"));
	CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
					   CVMCPU_ARG4_REG,
					   CVMCPU_SP_REG, off);
    }
#endif

    CVMJITaddCodegenComment((con, "Set R1 = JSP + (capacity - argsSize) * 4"));
    /* Record the start logicalPC for TOS adjustment by capacity */
    rec->capacityStartPC = CVMJITcbufGetLogicalPC(con);
    CVMJITcsSetEmitInPlace(con);
    /* This is an estimate. Will be patched by method prologue patch */
    CVMCPUemitALUConstant16Scaled(con,
        CVMCPU_ADD_OPCODE, CVMCPU_ARG2_REG, CVMCPU_JSP_REG,
        con->capacity + con->maxTempWords - argsSize, 2);
    CVMJITcsClearEmitInPlace(con);
    /* Record end logicalPC after estimate */
    rec->capacityEndPC = CVMJITcbufGetLogicalPC(con);

    /* Emit what used to be INVOKER_PROLOGUE */
    CVMCPUemitStackLimitCheckAndStoreReturnAddr(con);

    /* Now do appropriate set-up for this mb */
    if (CVMmbIs(mb, SYNCHRONIZED)) {
        if (CVMmbIs(mb, STATIC)) {
            invokerFunc = (void*)CVMCCMinvokeStaticSyncMethodHelper;
            invokerName = "CVMCCMinvokeStaticSyncMethodHelper";
        } else {
            invokerFunc = (void*)CVMCCMinvokeNonstaticSyncMethodHelper;
            invokerName = "CVMCCMinvokeNonstaticSyncMethodHelper";
        }
        /* Emit synchronized prologue: */
        jfpReg = CVMCPU_PROLOGUE_NEWJFP_REG;
        jfpStr = "NEW_JFP";
        CVMJITprintCodegenComment(("Set up frame for synchronized method"));
        simpleInvoke = CVM_FALSE;
	if (!CVMmbIs(mb, STATIC)) {
	    /* For nonstatic methods, we must set ARG2 to the stack address
	     * of "this". */
	    CVMJITaddCodegenComment((con, "ARG2 = JSP - argsSize * 4"));
	    CVMCPUemitALUConstant16Scaled(con, CVMCPU_SUB_OPCODE,
					  CVMCPU_ARG2_REG, CVMCPU_JSP_REG,
					  argsSize, 2);
	}

    } else {
        CVMUint32 offset = jspOffset * 4 + prevOffset;
        invokerFunc = NULL;
        invokerName = NULL;
        /* Emit non-synchronized prologue: */
        CVMJITprintCodegenComment(("Set up frame for method"));
        if (CVMCPUmemspecIsEncodableAsImmediate(offset)) {
            CVMJITaddCodegenComment((con, "Store curr JFP into new frame"));
            CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                                               CVMCPU_JFP_REG,
                                               CVMCPU_JSP_REG,
                                               offset);

        } else {
            /* Not encodable. Stash JFP in PREV. We'll flush it explicitly
             */
            /* MOV PREV, JFP */
            needFlushPREV = CVM_TRUE;
            CVMJITaddCodegenComment((con, "Set up PREV to be curr JFP"));
            CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
                                   CVMCPU_PROLOGUE_PREVFRAME_REG,
                                   CVMCPU_JFP_REG, CVMJIT_NOSETCC);
        }

        jfpReg = CVMCPU_JFP_REG;
        jfpStr = "JFP";
        simpleInvoke = CVM_TRUE;
    }
    /* Common tail of above two prologues is */
    /* add   JFP_VARIABLE, TOS, maxLocalsX * 4 */
    CVMJITaddCodegenComment((con, "%s = JSP + (maxLocals - argsSize) * 4",
                             jfpStr));
    CVMCPUemitALUConstant16Scaled(con, CVMCPU_ADD_OPCODE,
                                  jfpReg, CVMCPU_JSP_REG, 
                                  jspOffset, 2);

    if (simpleInvoke) {
        CVMassert(invokerFunc == NULL);
        /* Simple invocations don't need to call out to glue code */
        emitRestOfSimpleInvocation(con, needFlushPREV);
    } else {
        /* Call helper */
        CVMJITaddCodegenComment((con, "call %s", invokerName));
        CVMJITsetSymbolName((con, invokerName));
        CVMCPUemitAbsoluteCall(con, invokerFunc,
                               CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH);
    }

    /* And finally the spill instruction. This is also the interpreted
       to compiled entry point. */
    CVMJITprintCodegenComment(("Interpreted -> compiled entry point"));
    CVMJITaddCodegenComment((con, "spill adjust goes here"));

    /* This is an estimate. Will be patched by method prologue patch. */
    spillAdjust = (con->maxTempWords << 2) + 
        CVMoffsetof(CVMCompiledFrame, opstackX);
    spillPC = CVMJITcbufGetLogicalPC(con);
    CVMJITcsSetEmitInPlace(con);
    /* Record start logicalPC for spill adjustment */
    rec->spillStartPC = spillPC;
    CVMCPUemitALUConstant16Scaled(con, CVMCPU_ADD_OPCODE,
                                  CVMCPU_JSP_REG, CVMCPU_JFP_REG,
                                  spillAdjust, 0);
    CVMJITcsClearEmitInPlace(con);

    /* Record end logicalPC for spill adjustment */
    rec->spillEndPC = CVMJITcbufGetLogicalPC(con);

#ifdef CVMCPU_HAS_CP_REG
    {
        int i;
        /* reserve space for setting up the constant pool base register */
        CVMJITprintCodegenComment(("%d words for setting up cp base register",
                                  CVMCPU_RESERVED_CP_REG_INSTRUCTIONS));
        for (i = 0; i < CVMCPU_RESERVED_CP_REG_INSTRUCTIONS; i++) {
            CVMCPUemitNop(con);
        }
    }
#endif
    rec->intToCompOffset = spillPC - prologueStart;
}

#ifdef CVMCPU_HAS_DELAY_SLOT
#define FIXUP_PC_OFFSET (2 * CVMCPU_INSTRUCTION_SIZE)
#else
#define FIXUP_PC_OFFSET (1 * CVMCPU_INSTRUCTION_SIZE)
#endif

#define emitConditionalInstructions(con, condCode, conditionalInstructions) \
{									    \
    int fixupPC;    				                            \
									    \
    /* emit branch around conditional instruction(s) */			    \
    if (condCode != CVMCPU_COND_AL) {					    \
	CVMCodegenComment *comment;					    \
	CVMJITpopCodegenComment(con, comment);				    \
	CVMJITaddCodegenComment((con, "br .skip"));			    \
	CVMCPUemitBranch(con, 0, CVMCPUoppositeCondCode[condCode]);	    \
	CVMJITpushCodegenComment(con, comment);				    \
    }									    \
    fixupPC = CVMJITcbufGetLogicalPC(con) - FIXUP_PC_OFFSET;                \
									    \
    /* emit conditional instruction(s) */				    \
    con->inConditionalCode = CVM_TRUE;				    	    \
    conditionalInstructions						    \
    con->inConditionalCode = CVM_FALSE;				    	    \
									    \
    /* fixup target address of branch */				    \
    if (condCode != CVMCPU_COND_AL) {					    \
	CVMtraceJITCodegen(("\t\t.skip\n"));				    \
	CVMJITfixupAddress(con, fixupPC, CVMJITcbufGetLogicalPC(con),	    \
			   CVMJIT_COND_BRANCH_ADDRESS_MODE);		    \
    }									    \
}

#ifndef CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS

void
CVMCPUemitMemoryReferenceConditional(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg,
    CVMCPUMemSpecToken memSpecToken,
    CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitMemoryReference(con, opcode, destreg, basereg, memSpecToken);
    });
}

extern void
CVMCPUemitMemoryReferenceImmediateConditional(CVMJITCompilationContext* con,
    int opcode, int destreg, int basereg, CVMInt32 immOffset,
    CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitMemoryReferenceImmediate(con, opcode,
					   destreg, basereg, immOffset);
    });
}

#endif /* !CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS */

#ifndef CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS

extern void
CVMCPUemitAbsoluteCallConditional(CVMJITCompilationContext* con,
    const void* target,
    CVMBool okToDumpCp, CVMBool okToBranchAroundCpDump,
    CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitAbsoluteCall(con, target,
			       okToDumpCp, okToBranchAroundCpDump);
    });
}

#endif /* !CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS */

#ifndef CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS

void
CVMCPUemitUnaryALUConditional(CVMJITCompilationContext *con,
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitUnaryALU(con, opcode, destRegID, srcRegID, setcc);
    });
}

void
CVMCPUemitBinaryALUConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int lhsRegID, CVMCPUALURhsToken rhsToken,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitBinaryALU(con, opcode,
			    destRegID, lhsRegID, rhsToken, setcc);
    });
}

void
CVMCPUemitBinaryALUConstantConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int lhsRegID, CVMInt32 rhsConstValue,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitBinaryALUConstant(con, opcode, destRegID, lhsRegID,
				    rhsConstValue, setcc);
    });
}

void
CVMCPUemitBinaryALURegisterConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int lhsRegID, int rhsRegID,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitBinaryALURegister(con, opcode,
				    destRegID, lhsRegID, rhsRegID, setcc);
    });
}


void
CVMCPUemitLoadConstantConditional(CVMJITCompilationContext *con,
    int regID, CVMInt32 v,
    CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitLoadConstant(con, regID, v);
    });
}

void
CVMCPUemitMoveConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, CVMCPUALURhsToken srcToken,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitMove(con, opcode, destRegID, srcToken, setcc);
    });
}

void
CVMCPUemitMoveRegisterConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int srcRegID,
    CVMBool setcc, CVMCPUCondCode condCode)
{
    emitConditionalInstructions(con, condCode, {
	CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc);
    });
}

#endif /* !CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS */

#if !defined(CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION) && \
    !defined(CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS)

/* Purpose: Pins an arguments to the appropriate register or store it into the
            appropriate stack location. */
CVMRMResource *
CVMCPUCCALLpinArg(CVMJITCompilationContext *con,
                  CVMCPUCallContext *callContext, CVMRMResource *arg,
                  int argType, int argNo, int argWordIndex,
                  CVMRMregset *outgoingRegs, CVMBool useRegArgs)
{
    int regno = CVMCPU_ARG1_REG + argWordIndex;
    /*
     * In future, we would like to deal with arguments in fp regs, too. 
     * But for now, just passing arguments in int registers will have
     * to do.
     */
    arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
    CVMassert(argWordIndex + arg->size <= CVMCPU_MAX_ARG_REGS);
    *outgoingRegs |= arg->rmask;
    return arg;
}

#endif /* !CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION &&
          !CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS */

/* Purpose: Emits a constantpool dump with a branch around. */
void 
CVMRISCemitConstantPoolDumpWithBranchAround(
    CVMJITCompilationContext* con)
{
    if (CVMJITcpoolNeedDump(con)) {
        CVMInt32 startPC = CVMJITcbufGetLogicalPC(con);
        CVMInt32 endPC;

	CVMJITaddCodegenComment((con, "branch over constant pool dump"));
        CVMCPUemitBranch(con, startPC, CVMCPU_COND_AL);
        CVMJITdumpRuntimeConstantPool(con, CVM_TRUE);
        endPC = CVMJITcbufGetLogicalPC(con);

        /* Emit branch around the constant pool dump: */
        CVMJITcbufPushFixup(con, startPC);
	CVMJITaddCodegenComment((con, "branch over constant pool dump"));
        CVMCPUemitBranch(con, endPC, CVMCPU_COND_AL);
        CVMJITcbufPop(con);
    }
}

/* Purpose: Emits a constantpool dump with a branch around it if needed. */
void
CVMRISCemitConstantPoolDumpWithBranchAroundIfNeeded(
    CVMJITCompilationContext* con)
{
    if (CVMJITcpoolNeedDump(con)) {
	CVMRISCemitConstantPoolDumpWithBranchAround(con);
    }
}
