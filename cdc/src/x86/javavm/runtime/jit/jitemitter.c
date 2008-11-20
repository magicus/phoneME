/*
 * @(#)jitemitter.c	1.5 06/10/23
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
 * This file implements part of the CISC jit emitter porting layer. It
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
#include "javavm/include/jit/jitirnode.h"
/* SVMC_JIT d022609 (ML) 2004-02-23.
   rename from `jitriscemitter.h'. */
#include "javavm/include/jit/jitarchemitter.h"
#include "javavm/include/jit/ccmcisc.h"

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
     * SVMC_JIT PJ 2004-04-22
     *            tiny addition to this comment ...
     * Instead of the TOS register, we are going to be using 
     * JSP - argSize << 2   (3 on 64 bit architectures)
     */
    boundsOffset = capacity - argsSize;

    CVMJITcbufPushFixup(con, rec->capacityStartPC);
    /* Emit adjustment of TOS by capacity: */
    CVMJITprintCodegenComment(("Capacity is %d word(s)", capacity));
    CVMCPUemitALUConstant16Scaled_fixed(con, CVMCPU_ADD_OPCODE, CVMCPU_ARG2_REG,
					CVMCPU_JSP_REG, boundsOffset, 2);    
    /* Make sure we are not exceeding the reserved space for TOS adjust */
    CVMassert(CVMJITcbufGetLogicalPC(con) <= rec->capacityEndPC);
    CVMJITcbufPop(con);

    /* 2. Spill adjustment. */
    opstackOffset = CVMoffsetof(CVMCompiledFrame, opstackX);
    spillAdjust = (con->maxTempWords << 2) + opstackOffset;

    /* change pc to the spill adjustment instruction */
    CVMJITcbufPushFixup(con, rec->spillStartPC);

    /* Emit spill adjustment: */
    CVMJITprintCodegenComment(("spillSize is %d word(s), add to JFP+%d",
                               con->maxTempWords, opstackOffset));
    CVMCPUemitALUConstant16Scaled_fixed(con, CVMCPU_ADD_OPCODE,
					CVMCPU_JSP_REG, CVMCPU_JFP_REG,
					spillAdjust, 0);
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

#ifdef CVM_DEBUG_ASSERTS
    /*
        mov  arg4, #CONSTANT_CVM_FRAMETYPE_NONE
        strb arg4, [JFP, #OFFSET_CVMFrame_type]
        mov  arg4, #-1
        strb arg4, [JFP, #OFFSET_CVMFrame_flags]
    */
    {
        CVMUint32 typeOffset = CVMoffsetof(CVMFrame, type);
        CVMUint32 flagsOffset = CVMoffsetof(CVMFrame, flags);
        CVMUint32 topOfStackOffset = CVMoffsetof(CVMFrame, topOfStack);
        CVMJITprintCodegenComment(("DEBUG-ONLY CODE"));

	/* SVMC_JIT PJ 2003-11-18
	              small constant values are no longer loaded into registers,
		      but rather used as immediate operands
	*/
        CVMJITaddCodegenComment((con, "Store NONE into frame.type"));
        CVMCPUemitMemoryReferenceImmediateConst(con, CVMCPU_STR8_OPCODE,
                                           CVM_FRAMETYPE_NONE, /* NONE */
                                           CVMCPU_JFP_REG,
                                           typeOffset);

        CVMJITaddCodegenComment((con, "Store -1 into frame.flags"));
        CVMCPUemitMemoryReferenceImmediateConst(con, CVMCPU_STR8_OPCODE,
						0xff /* -1 (must be a byte) */,
                                           CVMCPU_JFP_REG,
                                           flagsOffset);

        CVMJITaddCodegenComment((con, "Store NULL into frame.topOfStack"));
        CVMCPUemitMemoryReferenceImmediateConst(con, CVMCPU_STR32_OPCODE,
                                           ( int ) NULL, /* NONE */
                                           CVMCPU_JFP_REG,
                                           topOfStackOffset);

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

#if 0 /* TODO(rr): enable! */

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
        CVMCPUemitAbsoluteCall_GlueCode(con, (void*)CVMCCMtraceMethodCallGlue,
                               CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH);

        CVMJITprintCodegenComment(("END OF DEBUG-ONLY CODE FOR TRACING"));
    }
#endif

#endif /* 0 */
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

    /* Add jspOffset * 4 ( 8 on 64 bit architectures ) to JSP to get the new JFP */
    jspOffset = con->numberLocalWords - argsSize;
    prologueStart = CVMJITcbufGetLogicalPC(con);

    CVMJITaddCodegenComment((con, "Set ARG2 = JSP + (capacity - argsSize) * 4"));
    /* Record the start logicalPC for TOS adjustment by capacity */
    rec->capacityStartPC = CVMJITcbufGetLogicalPC(con);
    /* This is an estimate. Will be patched by method prologue patch 
     * (SW): The CVMCPU_ARG2_REG register set here is checked by the 
     * CVMCPUemitStackLimitCheckAndStoreReturnAddr function. */
    CVMCPUemitALUConstant16Scaled_fixed(con, CVMCPU_ADD_OPCODE, CVMCPU_ARG2_REG, 
					CVMCPU_JSP_REG,con->capacity + 
					con->maxTempWords - argsSize, 2);

    /* Record end logicalPC after estimate */
    rec->capacityEndPC = CVMJITcbufGetLogicalPC(con);

    /* Emit what used to be INVOKER_PROLOGUE */
    CVMCPUemitStackLimitCheckAndStoreReturnAddr(con);

    /* Now do appropriate set-up for this mb */
    if (CVMmbIs(mb, SYNCHRONIZED)) {
        /* SVMC_JIT PJ 2004-04-22
                      cast function pointers in order to avoid
                      compiler warnings
	*/
        if (CVMmbIs(mb, STATIC)) {
            invokerFunc = ( void * ) CVMCCMinvokeStaticSyncMethodHelper;
            invokerName = "CVMCCMinvokeStaticSyncMethodHelper";
        } else {
            invokerFunc = ( void * ) CVMCCMinvokeNonstaticSyncMethodHelper;
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
	    CVMCPUemitALUConstant16Scaled_fixed(con, CVMCPU_SUB_OPCODE,
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
    /* SVMC_JIT PJ 2003-12-04
                  no need to use fixed instruction length here
                  - will not be patched
    */
    CVMCPUemitALUConstant16Scaled(con, CVMCPU_ADD_OPCODE,
                                  jfpReg, CVMCPU_JSP_REG, jspOffset, 2);

    if (simpleInvoke) {
        CVMassert(invokerFunc == NULL);
        /* Simple invocations don't need to call out to glue code */
        emitRestOfSimpleInvocation(con, needFlushPREV);
    } else {
        CVMJITaddCodegenComment((con, "call %s", invokerName));
        CVMJITsetSymbolName((con, invokerName));
        CVMCPUemitAbsoluteCall_GlueCode(con, invokerFunc,
                               CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH);
    }

    /* And finally the spill instruction. This is also the interpreted
       to compiled entry point. */
    CVMJITprintCodegenComment(("Interpreted -> compiled entry point"));

    rec->intToCompOffset = CVMJITcbufGetLogicalPC(con) - prologueStart;

    CVMJITaddCodegenComment((con, "spill adjust goes here"));

    /* This is an estimate. Will be patched by method prologue patch. */
    spillAdjust = (con->maxTempWords << 2) + 
                  CVMoffsetof(CVMCompiledFrame, opstackX);
    spillPC = CVMJITcbufGetLogicalPC(con);
    /* Record start logicalPC for spill adjustment */
    rec->spillStartPC = spillPC;
    CVMCPUemitALUConstant16Scaled_fixed(con, CVMCPU_ADD_OPCODE,
                                  CVMCPU_JSP_REG, CVMCPU_JFP_REG,
                                  spillAdjust, 0);
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

#ifdef CVM_JIT_USE_FP_HARDWARE
      if (con->target.usesFPU) CVMCPUemitInitFPMode(con);
#endif
}

#ifdef CVMCPU_HAS_DELAY_SLOT
#define FIXUP_PC_OFFSET (2 * CVMCPU_INSTRUCTION_SIZE)
#else
#define FIXUP_PC_OFFSET (1 * CVMCPU_INSTRUCTION_SIZE)
#endif


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
	CVMCPUemitAbsoluteCall_GlueCode(con, target,
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
  int regID, CVMInt32 v, CVMCPUCondCode condCode)
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
                  CVMRMregset *outgoingRegs)
{
    int regno = CVMCPU_ARG1_REG + argWordIndex;
    /* FIXME(rt) would like to deal with fp args, too. */
    arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
    /* TODO (SW): We set CVMCPU_MAX_ARG_REGS to zero. Does this assertion
       make any sense for us? */
    CVMassert(argWordIndex + arg->size <= CVMCPU_MAX_ARG_REGS);
    *outgoingRegs |= arg->rmask;
    /* SVMC_JIT 15062004 d022185(HS)
     * Currently intrinsic fcts only work with int registers,
     * but fp args are delivered in fp registers on sparcv9.
     * In case of float/double arguments we have to copy them into the
     * appropriate fp register via CVMRMcloneResource.
     * To handle fp arguments correctly, I would have to rewrite
     * the IR, but this should/will handle sun itself, hopefully. 
     */
#if defined(CVM_64) && defined(CVM_JIT_USE_FP_HARDWARE)
    if(argType == CVM_TYPEID_FLOAT || argType == CVM_TYPEID_DOUBLE)
    { CVMRMResource* darg;
      darg = CVMRMcloneResource(CVMRM_INT_REGS(con), arg,
	                        CVMRM_FP_REGS(con), 1U << argWordIndex, CVMRM_EMPTY_SET);
      CVMRMunpinResource(CVMRM_FP_REGS(con), darg);
    }
#endif
    return arg;
}

#endif /* !CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION &&
          !CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS */
