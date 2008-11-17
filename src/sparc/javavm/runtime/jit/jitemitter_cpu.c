/*
 * @(#)jitemitter_cpu.c	1.73 06/10/10
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
 * SPARC-specific bits.
 * Does not use compiler context, yet.
 */
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/utils.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/ccm_runtime.h"
#include "javavm/include/ccee.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitirnode.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"
#include "portlibs/jit/risc/include/porting/ccmrisc.h"
#include "portlibs/jit/risc/include/export/jitregman.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/jit/jitfixup.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/jit/jit.h"

#include "javavm/include/clib.h"

/**************************************************************
 * CPU ALURhs and associated types - The following are implementations
 * of the functions for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

/* Purpose: Constructs a constant type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewConstant(CVMJITCompilationContext* con, CVMInt32 v)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMSPARC_ALURHS_CONSTANT;
    ap->constValue = v;
    return ap;
}

/* Purpose: Constructs a register type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewRegister(CVMJITCompilationContext* con, CVMRMResource* rp)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMSPARC_ALURHS_REGISTER;
    ap->r = rp;
    return ap;
}

/* Purpose: Encodes the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */
CVMCPUALURhsToken
CVMSPARCalurhsEncodeToken(CVMSPARCALURhsType type, int constValue,
                          int rRegID)
{
    switch(type) {
    case CVMSPARC_ALURHS_CONSTANT: {
        CVMassert(CVMCPUalurhsIsEncodableAsImmediate(
                  CVMCPU_ADD_OPCODE, constValue));
        return CVMSPARC_MODE_CONSTANT | (constValue & 0x1fff);
    }
    case CVMSPARC_ALURHS_REGISTER: {
        return CVMSPARC_MODE_REGISTER | rRegID;
    }
    default:
        CVMassert(CVM_FALSE);
        return 0;
    }
}

CVMCPUALURhsToken
CVMSPARCalurhsGetToken(const CVMCPUALURhs *aluRhs)
{
    CVMCPUALURhsToken result;
    int regID = CVMCPU_INVALID_REG;
    if (aluRhs->type == CVMSPARC_ALURHS_REGISTER) {
        regID = CVMRMgetRegisterNumber(aluRhs->r);
    }
    result = CVMSPARCalurhsEncodeToken(aluRhs->type,
                                       aluRhs->constValue,
                                       regID);
    return result;
}

/* ALURhs resource management APIs: ======================================= */

/* Purpose: Pins the resources that this CVMCPUALURhs may use. */
void
CVMCPUalurhsPinResource(CVMJITRMContext* con, int opcode, CVMCPUALURhs* ap,
                        CVMRMregset target, CVMRMregset avoid)
{
    switch (ap->type) {
    case CVMSPARC_ALURHS_CONSTANT:
        /* If we can't encode the constant as a immediate, then
           load it into a register and re-write the aluRhs as a 
           CVMSPARC_ALURHS_REGISTER: 
         */
        if (!CVMCPUalurhsIsEncodableAsImmediate(opcode, ap->constValue)) {
            ap->type = CVMSPARC_ALURHS_REGISTER;
            ap->r = CVMRMgetResourceForConstant32(con, target, avoid,
                                                  ap->constValue);
            ap->constValue = 0;
        }
        return;
    case CVMSPARC_ALURHS_REGISTER:
        CVMRMpinResource(con, ap->r, target, avoid);
        return;
    }
}

/* Purpose: Relinquishes the resources that this CVMCPUALURhsToken may use. */
void
CVMCPUalurhsRelinquishResource(CVMJITRMContext* con,
                               CVMCPUALURhs* ap)
{
    switch (ap->type) {
    case CVMSPARC_ALURHS_CONSTANT:
        return;
    case CVMSPARC_ALURHS_REGISTER:
        CVMRMrelinquishResource(con, ap->r);
        return;
    }
}

/**************************************************************
 * CPU MemSpec and associated types - The following are implementations
 * of the functions for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* MemSpec constructors and query APIs: =================================== */

/* Purpose: Constructs a CVMCPUMemSpec immediate operand. */
CVMCPUMemSpec *
CVMCPUmemspecNewImmediate(CVMJITCompilationContext *con, CVMInt32 value)
{
    CVMCPUMemSpec *mp;
    mp = CVMJITmemNew(con, JIT_ALLOC_CGEN_MEMSPEC, sizeof(CVMCPUMemSpec));
    mp->type = CVMCPU_MEMSPEC_IMMEDIATE_OFFSET;
    mp->offsetValue = value;
    mp->offsetReg = NULL;
    return mp;
}

/* Purpose: Constructs a MemSpec register operand. */
CVMCPUMemSpec *
CVMCPUmemspecNewRegister(CVMJITCompilationContext *con,
                         CVMBool offsetIsToBeAdded, CVMRMResource *offsetReg)
{
    CVMCPUMemSpec *mp;
    mp = CVMJITmemNew(con, JIT_ALLOC_CGEN_MEMSPEC, sizeof(CVMCPUMemSpec));
    mp->type = CVMCPU_MEMSPEC_REG_OFFSET;
    mp->offsetValue = 0;
    mp->offsetReg = offsetReg;
    return mp;
}

/* MemSpec token encoder APIs: ============================================ */

#define CVMSPARCmemspecGetTokenType(memSpecToken) \
    (((memSpecToken) >> 13) & 0x3)

#define CVMSPARCmemspecGetTokenOffset(memSpecToken) \
     (((CVMInt32)(memSpecToken)<<19)>>19)

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMSPARCmemspecGetToken(const CVMCPUMemSpec *memSpec)
{
    CVMCPUMemSpecToken token;
    int type = memSpec->type;
    int offset;

    offset = (type == CVMCPU_MEMSPEC_REG_OFFSET) ?
              CVMRMgetRegisterNumber(memSpec->offsetReg) :
              memSpec->offsetValue;
    token = CVMSPARCmemspecEncodeToken(type, offset);
    return token;
}

/* MemSpec resource management APIs: ====================================== */

/* Purpose: Pins the resources that this CVMCPUMemSpec may use. */
void
CVMCPUmemspecPinResource(CVMJITRMContext *con, CVMCPUMemSpec *self,
                         CVMRMregset target, CVMRMregset avoid)
{
    switch (self->type) {
    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
        /* If we can't encode the constant as an immediate, then load it into
           a register and re-write the memSpec as a
           CVMCPU_MEMSPEC_REG_OFFSET: */
        if (!CVMCPUmemspecIsEncodableAsImmediate(self->offsetValue)) {
            self->type = CVMCPU_MEMSPEC_REG_OFFSET;
            self->offsetReg = 
                CVMRMgetResourceForConstant32(con, target, avoid,
                                              self->offsetValue);
            self->offsetValue = 0;
        }
        return;
    case CVMCPU_MEMSPEC_REG_OFFSET:
        CVMRMpinResource(con, self->offsetReg, target, avoid);
        return;
    default:
        /* No pinning needed for the other memSpec types. */

        /* Since the post-increment and pre-decrement types are only used
           for pushing and popping arguments on to the Java stack, the
           immediate offset should always be small.  No need to check if
           it is encodable: */
        CVMassert(CVMCPUmemspecIsEncodableAsImmediate(self->offsetValue));
    }
}

/* Purpose: Relinquishes the resources that this CVMCPUMemSpec may use. */
void
CVMCPUmemspecRelinquishResource(CVMJITRMContext *con,
                                CVMCPUMemSpec *self)
{
    switch (self->type) {
    case CVMCPU_MEMSPEC_REG_OFFSET:
        CVMRMrelinquishResource(con, self->offsetReg);
        break;
    default:
        ; /* No unpinning needed for the other memSpec types. */
    }
}

/**************************************************************
 * CPU code emitters - The following are implementations of code
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* Private SPARC specific defintions: ===================================== */
#define SPARC_ENCODE_CALL   (1U << 30)
#define SPARC_ENCODE_BRANCH (0U << 30)
#define SPARC_ENCODE_MEMORY (3U << 30)
#define SPARC_ENCODE_ALU    (2U << 30)
#define SPARC_ENCODE_FPOP   (2U << 30)

#define SPARC_JMPL_OPCODE   (0x38 << 19)
#define SPARC_ADDX_OPCODE   (0x08 << 19) /* add with carry */
#define SPARC_SUBX_OPCODE   (0x0c << 19) /* Substract with carry */
#define SPARC_SMUL_OPCODE   (0x0b << 19) /* reg32hi,reg32lo = reg32 * reg32 */
#define SPARC_UMUL_OPCODE   (0x0a << 19) /* reg32hi,reg32lo = reg32 * reg32 */
#define SPARC_RDY_OPCODE    (0x28 << 19) /* read Y register */
#define SPARC_RDPC_OPCODE   (0x28 << 19 | 0x5 << 14) /* read Program Counter*/
#define SPARC_WDY_OPCODE    (0x30 << 19) /* write Y register */
#define SPARC_SETHI_OPCODE  (0x4 << 22)  /* sethi */

/* modify integer condition codes (icc) */
#define SPARC_ALU_CC   (1 << 23)

/* branch condCodes */
#define SPARC_MAKE_BRANCH_CONDCODE_BITS(cond) \
    ((CVMUint32)((CVMUint32)(cond) << 25))

/* mapped from CVMCPUCondCode defined in jitriscemitterdefs_cpu.h*/
const CVMCPUCondCode CVMCPUoppositeCondCode[] = {
    CVMCPU_COND_AL, CVMCPU_COND_NE, CVMCPU_COND_GT, CVMCPU_COND_GE,
    CVMCPU_COND_HI, CVMCPU_COND_HS, CVMCPU_COND_PL, CVMCPU_COND_NO,
    CVMCPU_COND_NV, CVMCPU_COND_EQ, CVMCPU_COND_LE, CVMCPU_COND_LT,
    CVMCPU_COND_LS, CVMCPU_COND_LO, CVMCPU_COND_MI, CVMCPU_COND_OV
};

#ifdef CVM_TRACE_JIT
static const char* const conditions[] = {
    "n", "e", "le", "l", "leu", "cs", "neg", "vs", 
    "a", "ne", "g", "ge", "gu", "cc", "pos", "vc"
};

/* Purpose: Returns the name of the specified opcode. */
static const char *getMemOpcodeName(CVMInt32 opcode)
{
    const char *name = NULL;
    switch (opcode) {
    case CVMCPU_LDR32_OPCODE:  name = "ld"; break;
    case CVMCPU_STR32_OPCODE:  name = "st"; break;
    case CVMCPU_LDR16U_OPCODE: name = "lduh"; break;
    case CVMCPU_LDR16_OPCODE:  name = "ldsh"; break;
    case CVMCPU_STR16_OPCODE:  name = "sth"; break;
    case CVMCPU_LDR8U_OPCODE:  name = "ldub"; break;
    case CVMCPU_LDR8_OPCODE:   name = "ldsb"; break;
    case CVMCPU_STR8_OPCODE:   name = "stb"; break;
    case CVMCPU_STR64_OPCODE:  name = "std"; break;
    case CVMCPU_LDR64_OPCODE:  name = "ldd"; break;

    case CVMCPU_FLDR32_OPCODE: name = "fld"; break;
    case CVMCPU_FSTR32_OPCODE: name = "fst"; break;
    case CVMCPU_FLDR64_OPCODE: name = "fldd"; break;
    case CVMCPU_FSTR64_OPCODE: name = "fstd"; break;
    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", opcode);
        CVMassert(CVM_FALSE); /* Unknow opcode name */
    }
    return name;  
}

static const char *getALUOpcodeName(CVMInt32 opcode)
{
    const char *name = NULL;
    switch (opcode) {
    case CVMCPU_SLL_OPCODE:     name = "sll"; break;
    case CVMCPU_SRL_OPCODE:     name = "srl"; break;
    case CVMCPU_SRA_OPCODE:     name = "sra"; break;
    case CVMCPU_ADD_OPCODE:     name = "add"; break;
    case CVMCPU_SUB_OPCODE:     name = "sub"; break;
    case SPARC_SMUL_OPCODE:     name = "smul"; break;
    case SPARC_UMUL_OPCODE:     name = "umul"; break;
    case CVMCPU_AND_OPCODE:     name = "and"; break;
    case CVMCPU_OR_OPCODE:      name = "or"; break;
    case CVMCPU_XOR_OPCODE:     name = "xor"; break;
    case CVMCPU_BIC_OPCODE:     name = "andn"; break;
    case SPARC_SUBX_OPCODE:     name = "subx"; break;
    case SPARC_ADDX_OPCODE:     name = "addx"; break;

    case SPARC_RDY_OPCODE:      name = "rdy"; break;
    case SPARC_RDPC_OPCODE:     name = "rdpc"; break;

    case CVMCPU_FADD_OPCODE:	name = "fadds"; break;	
    case CVMCPU_FSUB_OPCODE:	name = "fsubs"; break;	
    case CVMCPU_FMUL_OPCODE:	name = "fmuls"; break;	
    case CVMCPU_FDIV_OPCODE:	name = "fdivs"; break;	
    case CVMCPU_FNEG_OPCODE:	name = "fnegs"; break;
    case CVMSPARC_I2F_OPCODE:	name = "fitos"; break;
    case CVMSPARC_F2I_OPCODE:	name = "fstoi"; break;
    case CVMCPU_FMOV_OPCODE:	name = "fmovs"; break;

    case CVMCPU_DADD_OPCODE:	name = "faddd"; break;	
    case CVMCPU_DSUB_OPCODE:	name = "fsubd"; break;	
    case CVMCPU_DMUL_OPCODE:	name = "fmuld"; break;	
    case CVMCPU_DDIV_OPCODE:	name = "fdivd"; break;	
    case CVMSPARC_I2D_OPCODE:	name = "fitod"; break;
    case CVMSPARC_D2I_OPCODE:	name = "fdtoi"; break;
    case CVMSPARC_F2D_OPCODE:	name = "fstod"; break;
    case CVMSPARC_D2F_OPCODE:	name = "fdtos"; break;

    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", opcode);
        CVMassert(CVM_FALSE); /* Unknow opcode name */
    }
    return name;
}

static void
printPC(CVMJITCompilationContext* con)
{
    CVMconsolePrintf("0x%8.8x\t%d:",
        CVMJITcbufGetPhysicalPC(con) - CVMCPU_INSTRUCTION_SIZE,
        CVMJITcbufGetLogicalPC(con) - CVMCPU_INSTRUCTION_SIZE);
}

static void
getRegName(char *buf, int regID)
{
    char *name = NULL;
    char *extra = "";

    switch (regID / 8) {
        case 0: name = "g"; break;
        case 1: name = "o"; break;
        case 2: name = "l"; break;
        case 3: name = "i"; break;
        default:
            CVMconsolePrintf("Unknown register r%d\n", regID);
            CVMassert(CVM_FALSE);
    }

    /* add extra info if register has a special purpose */
    switch (regID) {
    case CVMCPU_SP_REG:         extra = "_SP"; break;
    case CVMSPARC_LR:           extra = "_LR"; break;
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    case CVMCPU_GC_REG:         extra = "_GC"; break;
#endif
    case CVMCPU_CHUNKEND_REG:   extra = "_CHUNKEND"; break;
#ifdef CVMCPU_HAS_CP_REG
    case CVMCPU_CP_REG:         extra = "_CP"; break;
#endif
    case CVMCPU_EE_REG:         extra = "_EE"; break;
    case CVMCPU_JSP_REG:        extra = "_JSP"; break;
    case CVMCPU_JFP_REG:        extra = "_JFP"; break;
    }

    sprintf(buf, "%%%s%d%s", name, (regID % 8), extra);
}

static void
formatALURhsToken(char* buf, CVMCPUALURhsToken token)
{
    int constValue = 0;
    int regID = 0;
    if ((token & 0x2000) == CVMSPARC_MODE_CONSTANT) {
        constValue = token & 0x1fff;
        sprintf(buf, "#%d", (CVMInt32)(constValue << 19) >> 19);
    } else {
        regID = token & 0x1f;
        getRegName(buf, regID);
    }
}
#endif

/*
 * Instruction emitters. Use printf for now.
 */

static void
emitInstruction(CVMJITCompilationContext* con,
                CVMUint32 instruction)
{
#if 0
    CVMtraceJITCodegen(("0x%8.8x\t", instruction));
#endif
    CVMJITcbufEmit(con, instruction);
}

void
CVMJITemitWord(CVMJITCompilationContext *con, CVMInt32 wordVal)
{
    emitInstruction(con, wordVal);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	.word	%d", wordVal);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitNop(CVMJITCompilationContext *con)
{
    emitInstruction(con, CVMCPU_NOP_INSTRUCTION);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	nop");
    });
    CVMJITdumpCodegenComments(con);
}

static void
emitSetHi(CVMJITCompilationContext *con, int destRegID,
          CVMUint32 value)
{
    value &= 0x3fffff; /* Better fit in 22 bits */

    emitInstruction(con, (destRegID << 25) | SPARC_SETHI_OPCODE | value);
    CVMtraceJITCodegenExec({
        char destRegNameBuf[30];
        printPC(con);
        getRegName(destRegNameBuf, destRegID);
        CVMconsolePrintf("	sethi	%d, %s", value, destRegNameBuf);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits rd instruction to read state register. */
static void
CVMSPARCemitRd(CVMJITCompilationContext *con, 
                int opcode, int destRegID)
{
    emitInstruction(con, SPARC_ENCODE_ALU | destRegID << 25 |
                    (CVMUint32)opcode);
    CVMtraceJITCodegenExec({
        char destregbuf[30];
        printPC(con);
        getRegName(destregbuf, destRegID);
        CVMconsolePrintf("	%s	%%y, %s",
                         getALUOpcodeName(opcode), destregbuf);
    });
    CVMJITdumpCodegenComments(con);
}

#ifdef CVMCPU_HAS_CP_REG
/* Purpose: Set up constant pool base register */
void
CVMCPUemitLoadConstantPoolBaseRegister(CVMJITCompilationContext *con)
{
#ifdef CVM_AOT
    CVMUint32 targetOffset;

    CVMJITprintCodegenComment(("setup cp base register"));

    /* Implemented in PC relative way:
     
           rd  %pc, %cpReg
           add %cpReg, offset, %cpReg
       
       or:

           rd  %pc, %cpReg
	   sethi offset22 %scratch
	   or  %scratch, offset10, %scratch
           add %cpReg, %scratch, %cpReg
     */
    targetOffset = con->target.cpLogicalPC - CVMJITcbufGetLogicalPC(con);
    CVMSPARCemitRd(con, SPARC_RDPC_OPCODE, CVMCPU_CP_REG);
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                                CVMCPU_CP_REG, CVMCPU_CP_REG, 
                                targetOffset, CVMJIT_NOSETCC);
#else
    CVMUint32 targetAddr;
    targetAddr = (CVMUint32)CVMJITcbufLogicalToPhysical(
                                con, con->target.cpLogicalPC);

    CVMJITprintCodegenComment(("setup cp base register"));

    /* Implemented as:

           sethi addr22, %cpReg
           or    %cpReg, addr10, %cpReg
     */
    emitSetHi(con, CVMCPU_CP_REG, targetAddr >> 10);
    CVMCPUemitBinaryALU(con, CVMCPU_OR_OPCODE, 
        CVMCPU_CP_REG, CVMCPU_CP_REG,
        CVMSPARCalurhsEncodeConstantToken(targetAddr & 0x3ff),
        CVMJIT_NOSETCC);
#endif

    /*
     * Save the constant pool base register into the frame so it can
     * be restored if we invoke another method.
     */
    CVMJITaddCodegenComment((con, "save to frame->cpBaseRegX"));
    CVMassert(CVMCPUmemspecIsEncodableAsImmediate(
                    CVMoffsetof(CVMCompiledFrame, cpBaseRegX)));
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_STR32_OPCODE, CVMCPU_CP_REG, CVMCPU_JFP_REG,
        CVMoffsetof(CVMCompiledFrame, cpBaseRegX));
}
#endif /* CVMCPU_HAS_CP_REG */

/* Purpose:  Loads the CCEE into the specified register. */
void
CVMCPUemitLoadCCEE(CVMJITCompilationContext *con, int destRegID)
{
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, destRegID,
                               CVMCPU_SP_REG, MINFRAME, CVM_FALSE);
}

/* Purpose:  Load or Store a field of the CCEE. */
void
CVMCPUemitCCEEReferenceImmediate(CVMJITCompilationContext *con,
                                 int opcode, int regID, int offset)
{
    int realOffset = offset + MINFRAME;
    CVMCPUemitMemoryReferenceImmediate(con, opcode, regID,
                                       CVMSPARC_SP, realOffset);
}

/* Purpose:  Add/sub a 16-bits constant scaled by 2^scale. Called by 
 *           method prologue and patch emission routines. 
 * NOTE:     CVMCPUemitAddConstant16Scaled should not rely on regman   
 *           states because the regman context used to emit the method
 *           prologue is gone at the patching time.
 * NOTE:     CVMCPUemitALUConstant16Scaled must always emit the same
 *           number of instructions, no matter what constant or scale
 *           is passed to it.
 */
void
CVMCPUemitALUConstant16Scaled(CVMJITCompilationContext *con, int opcode,
                              int destRegID, int srcRegID,
                              CVMUint32 constant, int scale)
{
    CVMUint32 value = constant << scale;

    CVMassert(opcode == CVMCPU_ADD_OPCODE ||
              opcode == CVMCPU_SUB_OPCODE);

    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, value)) {
        /* Encode in one instruction */
        CVMCPUemitBinaryALU(con, opcode, destRegID, srcRegID,
			    CVMSPARCalurhsEncodeConstantToken(value),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 2k
	 * words of locals, so it's ok to just not compile this method.
         */ 
	CVMJITerror(con, CANNOT_COMPILE, "too many locals");
    }
}

/*
 * Purpose: Emit Stack limit check at the start of each method.
 */
void
CVMCPUemitStackLimitCheckAndStoreReturnAddr(CVMJITCompilationContext* con)
{
    CVMJITprintCodegenComment(("Stack limit check"));

    CVMJITaddCodegenComment((con, "Store LR into frame"));
    CVMassert(CVMCPUmemspecIsEncodableAsImmediate(
                    CVMoffsetof(CVMCompiledFrame, pcX)));
    /* The link register has the address of last call instruction.
       Adjust it by 8 before store it into frame. */
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, CVMSPARC_LR,
                                CVMSPARC_LR, 8, CVMJIT_NOSETCC);
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE, CVMSPARC_LR,
        CVMCPU_JFP_REG, CVMoffsetof(CVMCompiledFrame, pcX));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_SUB_OPCODE, CVMSPARC_LR,
                                CVMSPARC_LR, 8, CVMJIT_NOSETCC);

    CVMJITaddCodegenComment((con, "check for chunk overflow"));
    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_LS,
                              CVMCPU_CHUNKEND_REG, CVMCPU_ARG2_REG);

    CVMJITaddCodegenComment((con, "letInterpreterDoInvoke"));
    CVMCPUemitAbsoluteCallConditional(con,
        (const void*)CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr,
        CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH, CVMCPU_COND_LS);
}

static void
CVMSPARCemitJumpLink(CVMJITCompilationContext* con, int destreg,
                     int lhsreg, CVMCPUALURhsToken rhsToken)
{
    CVMUint32 jmplInstruction;

    jmplInstruction = (SPARC_ENCODE_ALU | (destreg << 25) |
        SPARC_JMPL_OPCODE | (lhsreg << 14) | rhsToken);
    emitInstruction(con, jmplInstruction);
    CVMtraceJITCodegenExec({
        char rhsbuf[30];
        char lhsregBuf[30];
        char destregBuf[30];
        printPC(con);
        formatALURhsToken(rhsbuf, rhsToken);
        getRegName(lhsregBuf, lhsreg);
        getRegName(destregBuf, destreg);
        CVMconsolePrintf("	jmpl	%s, %s, %s",
            lhsregBuf, rhsbuf, destregBuf);
    });
    CVMJITdumpCodegenComments(con);

    /* Fill the delayed slot with a `nop' */
    CVMCPUemitNop(con);
}

static void
CVMSPARCemitMakeConstant32(CVMJITCompilationContext* con,
                           int destRegID, CVMInt32 constant)
{
    emitSetHi(con, destRegID, constant >> 10);
    CVMCPUemitBinaryALU(con, CVMCPU_OR_OPCODE, destRegID, destRegID,
        CVMSPARCalurhsEncodeConstantToken(constant & 0x3ff),
        CVMJIT_NOSETCC);
}

/* 
 *  Purpose: Emits code to invoke method through MB.
 *           MB is already in CVMCPU_ARG1_REG. 
 */
void
CVMCPUemitInvokeMethod(CVMJITCompilationContext* con)
{
    CVMUint32 off = CVMoffsetof(CVMMethodBlock, jitInvokerX);

    CVMJITaddCodegenComment((con, "call method through mb"));

    /* Implemented using jmpl instruction. */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
        CVMSPARC_g1, CVMCPU_ARG1_REG, off);
    CVMSPARCemitJumpLink(con, CVMSPARC_LR, CVMSPARC_g1,
        CVMSPARCalurhsEncodeConstantToken(0));
}

/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the LR is a GPR.
 */
void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno)
{
    CVMassert(regno == CVMSPARC_LR);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
                        regno, regno, CVMSPARCalurhsEncodeConstantToken(8),
                        CVMJIT_NOSETCC);
}

/*
 * Branch to the address in the specified register use jmpl instruction.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno)
{
    CVMSPARCemitJumpLink(con, CVMSPARC_g0, regno,
                         CVMSPARCalurhsEncodeConstantToken(0));
}

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The table entry for index 0 will be generated right after
 * any instructions that are generated here.
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int indexRegNo)
{
    /* This is implemented as (PC relative):
     *
     *     sll    key, 3, %reg1
     *     rd     %pc, %reg2
     *     add    %reg2, 16, %reg2 ! distance from 'rd' to table tart
     *     jmpl   %reg1, %reg2, %g0
     *     nop
     *   tablePC:
     */
    CVMRMResource *tmpResource1, *tmpResource2;
    CVMUint32 tmpReg1, tmpReg2;

    tmpResource1 = CVMRMgetResource(CVMRM_INT_REGS(con),
				    CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    tmpReg1 = CVMRMgetRegisterNumber(tmpResource1);
    tmpResource2 = CVMRMgetResource(CVMRM_INT_REGS(con),
				    CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    tmpReg2 = CVMRMgetRegisterNumber(tmpResource2);

    /* emit the shift instruction: */
    /* shift key by 3 because every table element branch has a nop for
       the branch delay slot */
    CVMCPUemitShiftByConstant(con, CVMCPU_SLL_OPCODE, 
                              tmpReg1, indexRegNo, 3);

    /* emit the rd instruction to get current pc */
    CVMSPARCemitRd(con, SPARC_RDPC_OPCODE, tmpReg2);
    /* tablePC = pc + 4 * CVMCPU_INSTRUCTION_SIZE */
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, tmpReg2,
                                tmpReg2, 4*CVMCPU_INSTRUCTION_SIZE,
                                CVMJIT_NOSETCC);

    /* emit the jmpl instruction: */
    CVMSPARCemitJumpLink(con, CVMSPARC_g0, tmpReg2, 
        CVMSPARCalurhsEncodeRegisterToken(tmpReg1));

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource1);
    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource2);
}

void
CVMCPUemitPopFrame(CVMJITCompilationContext* con, int resultSize)
{
    CVMUint32 offset; /* offset from JFP for new JSP */

    /* We want to set JSP to the address of the locals + the resultSize */
    offset = (con->numberLocalWords - resultSize) * sizeof(CVMJavaVal32);

    if (CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_SUB_OPCODE, offset)) {
        CVMCPUemitBinaryALU(con, CVMCPU_SUB_OPCODE,
			    CVMCPU_JSP_REG, CVMCPU_JFP_REG,
			    CVMSPARCalurhsEncodeConstantToken(offset),
			    CVMJIT_NOSETCC);
    } else {
        /* Load the offset as a constant into a register: */
        CVMCPUemitLoadConstant(con, CVMSPARC_g1, offset);

        /* Pop the frame: */
        CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
            CVMCPU_JSP_REG, CVMCPU_JFP_REG, CVMSPARC_g1, CVMJIT_NOSETCC);
    } 
}

CVMCPUInstruction
CVMSPARCgetBranchInstruction(int opcode, CVMCPUCondCode condCode, int offset)
{
    int realoffset = offset >> 2;

    CVMassert((realoffset & 0xffe00000) == 0 ||
              (realoffset & 0xffe00000) == 0xffe00000);
    return (SPARC_ENCODE_BRANCH |
        SPARC_MAKE_BRANCH_CONDCODE_BITS(condCode) | opcode |
        ((CVMUint32)realoffset & 0x003fffff));
}

#ifdef CVM_JIT_USE_FP_HARDWARE
/*
 * It is difficult to map the floating point condition codes,
 * with their variants, into the enum along with the integer codes,
 * so we use this set of tables to help us translate.
 */
struct fpccMap{
    int ccval;
    const char * const ccname;
};

/* this map used when unordered is treated as greater than */
static struct fpccMap fpccugt[] = {
    { 9,  "e"  },	/* equal */
    { 1,  "ne" },	/* not equal */
    { 4,  "l"  },	/* less than */
    { 5,  "ug" },	/* unordered or greater than */
    { 13, "le" },	/* less than or equal */
    { 12, "uge"},	/* unordered or greater than or equal */
};

/* this map used when unordered is treated as less than */
static struct fpccMap fpccult[] = {
    { 9,  "e"  },	/* equal */
    { 1,  "ne" },	/* not equal */
    { 3,  "ul"  },	/* unordered or less than */
    { 6,  "g" },	/* greater than */
    { 14, "ule" },	/* less than or equal */
    { 11, "ge"},	/* greater than or equal */
};
#endif

/*
 * Make a PC-relative branch or branch-and-link instruction
 * CVMSPARCemitBranch does NOT emit the nop, and is capable of
 * or-ing in an annul bit.
 * CVMCPUemitBranch is much more tame and less machine specific:
 * no annullment and no problems with delay slots as it will supply a nop.
 */
void
CVMSPARCemitBranch(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode, CVMBool annul)
{
    int realoffset;
    CVMUint32 branchInstruction;

    realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);
#ifdef CVM_JIT_USE_FP_HARDWARE
    if (condCode < CVMSPARC_COND_FFIRST){
#endif
	/* integer (conditional) branch */
	branchInstruction = CVMSPARCgetBranchInstruction(
			    CVMSPARC_BRANCH_OPCODE, condCode, realoffset); 
	if (annul)
	    branchInstruction |= 1 << 29;
	emitInstruction(con, branchInstruction);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	b%s%s	PC=(%d)",
		conditions[condCode], annul ? ",a" : "",  logicalPC);
	});
#ifdef CVM_JIT_USE_FP_HARDWARE
    } else {
	/* floating conditional branch */
	struct fpccMap* ccmap;
	struct fpccMap* ccentry;
	if (condCode & CVMCPU_COND_UNORDERED_LT){
	    ccmap = fpccult;
	    condCode &= ~CVMCPU_COND_UNORDERED_LT;
	} else {
	    ccmap =  fpccugt;
	}
	ccentry = &ccmap[condCode - CVMSPARC_COND_FFIRST];
	branchInstruction = CVMSPARCgetBranchInstruction(
			    CVMSPARC_FBRANCH_OPCODE, 
			    ccentry->ccval, realoffset); 
	if (annul)
	    branchInstruction |= 1 << 29;
	emitInstruction(con, branchInstruction);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	fb%s%s	PC=(%d)",
		ccentry->ccname, annul ? ",a" : "",  logicalPC);
	});

    }
#endif

    CVMJITdumpCodegenComments(con);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);
}

void
CVMCPUemitBranchNeedFixup(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode,
                 CVMJITFixupElement** fixupList)
{
    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }
    CVMSPARCemitBranch(con, logicalPC, condCode, CVM_FALSE);
    /* Fill the branch delay slot with a `nop'. */
    CVMCPUemitNop(con);
}

static void
CVMSPARCemitCall(CVMJITCompilationContext* con, int logicalPC)
{
    CVMInt32 realoffset;
    CVMUint32 callInstruction;

    realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);
    callInstruction = (SPARC_ENCODE_CALL | ((realoffset >> 2) & 0x3fffffff));
    emitInstruction(con, callInstruction);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	call	PC=(%d)", logicalPC);
    });
    CVMJITdumpCodegenComments(con);

    /* Fill the branch delay slot with a `nop'. */
    CVMCPUemitNop(con);
}

/* Encoded using call instruction. */
void
CVMCPUemitBranchLinkNeedFixup(CVMJITCompilationContext* con, int logicalPC,
                              CVMJITFixupElement** fixupList)
{
    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }
    CVMSPARCemitCall(con, logicalPC); 
}

/* Purpose: Emits instructions to do the specified 32 bit unary ALU
            operation. */
void
CVMCPUemitUnaryALU(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int srcRegID, CVMBool setcc)
{
    switch (opcode) {
    case CVMCPU_NEG_OPCODE: {
        CVMCPUALURhsToken rhsToken = 
            CVMSPARCalurhsEncodeRegisterToken(srcRegID);
        CVMCPUemitBinaryALU(con, CVMCPU_SUB_OPCODE,
			    destRegID, CVMSPARC_g0, rhsToken, setcc);
        break;
    }
    case CVMCPU_NOT_OPCODE: {
        /* reg32 = (reg32 == 0)?1:0. */
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_AL,
				  CVMSPARC_g0, srcRegID);
        CVMCPUemitBinaryALUConstant(con, SPARC_SUBX_OPCODE,
                                    destRegID, CVMSPARC_g0, -1, setcc);        
        break;
    }
    case CVMCPU_INT2BIT_OPCODE: {
        /* reg32 = (reg32 != 0)?1:0. */
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_AL,
				  CVMSPARC_g0, srcRegID);
        CVMCPUemitBinaryALUConstant(con, SPARC_ADDX_OPCODE,
                                    destRegID, CVMSPARC_g0, 0, setcc);        
        break;
    }
    default:
        CVMassert(CVM_FALSE);
    }
}

/* Purpose: Emits wdy instruction to write Y register */
static void
CVMSPARCemitWdy(CVMJITCompilationContext *con, int lhsRegID,
                CVMInt32 value)
{
    /* lhs xor value -> %y*/
    emitInstruction(con, SPARC_ENCODE_ALU | SPARC_WDY_OPCODE |
                    (lhsRegID << 14) | (1 << 13) | (value & 0x1ffff));
    CVMtraceJITCodegenExec({
        char lhsbuf[30];
        printPC(con);
        getRegName(lhsbuf, lhsRegID);
        CVMconsolePrintf("	wr	%s, %d, %%y",
                         lhsbuf, value);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits a div or rem. */
static void
CVMSPARCemitDivOrRem(CVMJITCompilationContext* con,
	 	     int opcode, int destRegID, int lhsRegID,
		     CVMCPUALURhsToken rhsToken)
{
    int rhsRegID = rhsToken & 0x1f;
    int doDivLogicalPC;
    int divDoneLogicalPC;

    CVMassert(
        (rhsToken & CVMSPARC_MODE_REGISTER) == CVMSPARC_MODE_REGISTER);

    CVMassert(destRegID != rhsRegID);

    /*
     *      ! Sign extend lhs to y register
     *      ! Per the SPARC Architecture Manual, there is a delay
     *      ! (three instructions) before the content of %y is available
     *      ! for read after a WRY. Instead of doing WRY at .doDiv and 
     *      ! insert three nop before sdiv, we do it here.
     *      sra    lhsRegID, 31, tmpReg
     *      wr     tmpReg, 0, y
     *
     *      ! Integer divide
     *      cmp    rhsRegID, 0    ! check for / by 0
     *      be     CVMCCMruntimeThrowDivideByZeroGlue
     *      nop
     *      bg     .doDiv         ! shortcut around 0x80000000 / -1 check
     *      nop
     *      cmp    rhsRegID, -1   ! 0x80000000 / -1 check
     *      bne    .doDiv
     *      nop
     *      sethi  %hi(0x80000000), destRegID
     *      cmp    destRegID, lhsRegID
     *      be     .divDone
     *      nop
     * .doDiv:
     *      ! sdiv performs 64-bit/32-bit, using the y register
     *      sdiv   lhsRegID, rhsRegID, destRegID
     * .divDone:
     *
     *      ! integer remainder after doing divide
     *      smull  destRegID, rhsRegID, destRegID
     *      sub    destRegID, lhsRegID, destRegID
     */

    CVMJITdumpCodegenComments(con);
    CVMJITprintCodegenComment(("Do integer %s:",
        opcode == CVMSPARC_DIV_OPCODE ? "divide" : "remainder"));

    /* sign extend lhs to y register */
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE,
                              CVMSPARC_g1, lhsRegID, 31);
    CVMSPARCemitWdy(con, CVMSPARC_g1, 0);

    /* move lhsRegID into CVMSPARC_g1 if lhsRegID == destRegID. Otherwise
     * it will be clobbered before used. */
    if (lhsRegID == destRegID) {
	CVMJITaddCodegenComment((con, "avoid clobber when lshReg == destReg"));
	CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			       CVMSPARC_g1, lhsRegID, CVMJIT_NOSETCC);
	lhsRegID = CVMSPARC_g1;
    }

    /* compare for rhsRegID % 0 */
    CVMJITaddCodegenComment((con, "check for / by 0"));
    CVMCPUemitCompareConstant(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_EQ,
			      rhsRegID, 0);
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMJITsetSymbolName((con, "CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMCPUemitAbsoluteCallConditional(con, 
        (const void*)CVMCCMruntimeThrowDivideByZeroGlue,
        CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH,
        CVMCPU_COND_EQ);

    doDivLogicalPC = CVMJITcbufGetLogicalPC(con) + 
	                 CVMCPU_INSTRUCTION_SIZE * 9;

    /* shortcut around compare for 0x80000000 % -1 */
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMCPUemitBranch(con, doDivLogicalPC, CVMCPU_COND_GT);

    /* compare for 0x80000000 / -1 */
    CVMJITaddCodegenComment((con, "0x80000000 / -1 check:"));
    CVMCPUemitCompare(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
		      rhsRegID, CVMSPARCalurhsEncodeConstantToken(-1));
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMCPUemitBranch(con, doDivLogicalPC, CVMCPU_COND_NE);
    emitSetHi(con, destRegID, (0x80000000 >> 10));
    CVMJITaddCodegenComment((con, "goto divDone"));
    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
		             destRegID, lhsRegID);

    divDoneLogicalPC = CVMJITcbufGetLogicalPC(con) +
                           CVMCPU_INSTRUCTION_SIZE * 3;
    CVMCPUemitBranch(con, divDoneLogicalPC, CVMCPU_COND_EQ);

    /* emit the sdiv instruction */
    CVMtraceJITCodegen(("\t\t.doDiv\n"));
    emitInstruction(con, SPARC_ENCODE_ALU | destRegID << 25 |
                    CVMSPARC_DIV_OPCODE | lhsRegID << 14 | rhsRegID);
    CVMtraceJITCodegenExec({
        char rhsbuf[30];
        char lhsbuf[30];
        char destbuf[30];
        printPC(con);
        getRegName(rhsbuf, rhsRegID);
        getRegName(lhsbuf, lhsRegID);
        getRegName(destbuf, destRegID);
        CVMconsolePrintf("	sdiv	%s, %s, %s\n",
			 lhsbuf, rhsbuf, destbuf);
    });

    CVMtraceJITCodegen(("\t\t.divDone\n"));
 
    /* do rem computation if requested */
    if (opcode == CVMSPARC_REM_OPCODE) {
	CVMJITprintCodegenComment(("do integer remainder after doing divide"));
	CVMCPUemitMul(con, CVMCPU_MULL_OPCODE,
		      destRegID, destRegID, rhsRegID, CVMCPU_INVALID_REG);
	CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
				    destRegID, lhsRegID, destRegID,
				    CVMJIT_NOSETCC);
    }
}

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
void
CVMCPUemitBinaryALU(CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID,
		    CVMCPUALURhsToken rhsToken,
		    CVMBool setcc)
{
    CVMassert(opcode == CVMCPU_ADD_OPCODE || opcode == CVMCPU_SUB_OPCODE ||
              opcode == CVMCPU_AND_OPCODE || opcode == CVMCPU_OR_OPCODE  ||
              opcode == CVMCPU_XOR_OPCODE || opcode == CVMCPU_BIC_OPCODE ||
              opcode == CVMSPARC_DIV_OPCODE || opcode == CVMSPARC_REM_OPCODE ||
              opcode == SPARC_ADDX_OPCODE || opcode == SPARC_SUBX_OPCODE);

    if (opcode == CVMSPARC_DIV_OPCODE || opcode == CVMSPARC_REM_OPCODE) {
	CVMassert(!setcc);
	CVMSPARCemitDivOrRem(con, opcode, destRegID, lhsRegID, rhsToken);
	return;
    }

    emitInstruction(con,
                    SPARC_ENCODE_ALU | destRegID << 25 | (CVMUint32)opcode |
                    (setcc ? SPARC_ALU_CC : 0) | lhsRegID << 14 | rhsToken);

    CVMtraceJITCodegenExec({
        char rhsRegBuf[30];
        char lhsRegBuf[30];
        char destRegBuf[30];
        printPC(con);
        formatALURhsToken(rhsRegBuf, rhsToken);
        getRegName(lhsRegBuf, lhsRegID);
        getRegName(destRegBuf, destRegID);
	if (destRegID == CVMSPARC_g0) {
	    /* This is really a compare */
	    CVMassert(opcode == CVMCPU_SUB_OPCODE && setcc);
	    CVMconsolePrintf("	cmp	%s, %s", lhsRegBuf, rhsRegBuf);
	} else if (opcode == CVMCPU_ADD_OPCODE && lhsRegID == CVMSPARC_g0) {
	    /* This is really a move register */
	    CVMconsolePrintf("	mov%s	%s, %s",
			     setcc ? "cc" : "", rhsRegBuf, destRegBuf);
	} else {
	    CVMconsolePrintf("	%s%s	%s, %s, %s",
			     getALUOpcodeName(opcode), setcc ? "cc" : "",
			     lhsRegBuf, rhsRegBuf, destRegBuf);
	}
    });
    CVMJITdumpCodegenComments(con);
}


/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
void
CVMCPUemitBinaryALUConstant(CVMJITCompilationContext* con,
			    int opcode, int destRegID, int lhsRegID,
			    CVMInt32 rhsConstValue, CVMBool setcc)
{
    CVMCPUALURhsToken rhsToken;
    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsToken = CVMSPARCalurhsEncodeConstantToken(rhsConstValue);
    } else {
        /* Load the constant into a register. Use g1 as
           the scratch register.
         */
        CVMCPUemitLoadConstant(con, CVMSPARC_g1, rhsConstValue);
        rhsToken = CVMSPARCalurhsEncodeRegisterToken(CVMSPARC_g1);
    }
    CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID, rhsToken, setcc);
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.*/
void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, CVMUint32 shiftAmount)
{
    CVMUint32 shiftInstruction;

    CVMassert(opcode == CVMCPU_SLL_OPCODE || opcode == CVMCPU_SRL_OPCODE ||
              opcode == CVMCPU_SRA_OPCODE);

    CVMassert(shiftAmount >> 5 == 0);

    shiftInstruction = (SPARC_ENCODE_ALU |
        (destRegID << 25) | opcode | (srcRegID << 14) |
        (0x1 << 13) | (shiftAmount & 0x1F));
    emitInstruction(con, shiftInstruction); 

    CVMtraceJITCodegenExec({
        char srcregbuf[30];
        char destregbuf[30];
        printPC(con);
        getRegName(srcregbuf, srcRegID);
        getRegName(destregbuf, destRegID);
        CVMconsolePrintf("	%s	%s, %d, %s",
            getALUOpcodeName(opcode), srcregbuf,
            shiftAmount, destregbuf);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.
            The shift amount is specified in a register.  The platform
            implementation is responsible for making sure that the shift
            semantics is done in such a way that the effect is the same as if
            the shiftAmount is masked with 0x1f before the shift operation is
            done.  This is needed in order to be compliant with the VM spec.
*/
void
CVMCPUemitShiftByRegister(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, int shiftAmountRegID)
{
    CVMassert(opcode == CVMCPU_SLL_OPCODE || opcode == CVMCPU_SRL_OPCODE ||
              opcode == CVMCPU_SRA_OPCODE);

    /* Mask off the offset with 0x1f before shifting per VM spec. */
    CVMCPUemitBinaryALUConstant(con, CVMCPU_AND_OPCODE,
        destRegID, shiftAmountRegID, 0x1F, CVMJIT_NOSETCC);

    emitInstruction(con, SPARC_ENCODE_ALU | (destRegID << 25) |
        (CVMUint32)opcode | (srcRegID << 14) | destRegID);

    CVMtraceJITCodegenExec({
        char srcregbuf[30];
        char lhsregbuf[30];
        char destregbuf[30];
        printPC(con);
        getRegName(srcregbuf, srcRegID);
        getRegName(lhsregbuf, destRegID);
        getRegName(destregbuf, destRegID);
        CVMconsolePrintf("	%s	%s, %s, %s",
            getALUOpcodeName(opcode), srcregbuf, lhsregbuf, destregbuf);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified 64 bit unary ALU
            operation. */
void
CVMCPUemitUnaryALU64(CVMJITCompilationContext *con, int opcode,
                     int destRegID, int srcRegID)
{
    switch (opcode) {
        case CVMCPU_NEG64_OPCODE:
            /* Implemented as:
              
                   sub  %g0, srcRegID+1, destRegID+1
                   subx %g0, srcRegID, destRegID
             */
            CVMCPUemitBinaryALU(con, CVMCPU_SUB_OPCODE, 
                destRegID+1, CVMSPARC_g0, 
                CVMSPARCalurhsEncodeRegisterToken(srcRegID+1),
                CVMJIT_SETCC);
            CVMCPUemitBinaryALU(con, SPARC_SUBX_OPCODE,
                destRegID, CVMSPARC_g0,
                CVMSPARCalurhsEncodeRegisterToken(srcRegID),
                CVMJIT_NOSETCC);
            break;
        default:
            CVMassert(CVM_FALSE);
    }
}

/* Purpose: Emits instructions to do the specified 64 bit ALU operation. */
void
CVMCPUemitBinaryALU64(CVMJITCompilationContext *con,
		      int opcode, int destRegID, int lhsRegID, int rhsRegID)
{
    if (opcode == CVMCPU_MUL64_OPCODE) {
        /* 
           Use CVMSPARC_g1 as the scratch register since it's not in the
           register sets that regman allocates from. 
         */
        CVMCPUemitMul(con, SPARC_UMUL_OPCODE, destRegID+1,
                      lhsRegID+1, rhsRegID+1, destRegID);
        CVMCPUemitMul(con, CVMCPU_MULL_OPCODE, CVMSPARC_g1,
                      lhsRegID, rhsRegID+1, CVMCPU_INVALID_REG);
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, destRegID,
			    CVMSPARCalurhsEncodeRegisterToken(CVMSPARC_g1),
			    CVMJIT_NOSETCC);
        CVMCPUemitMul(con, CVMCPU_MULL_OPCODE, CVMSPARC_g1,
                      lhsRegID+1, rhsRegID, CVMCPU_INVALID_REG);
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, destRegID,
			    CVMSPARCalurhsEncodeRegisterToken(CVMSPARC_g1),
			    CVMJIT_NOSETCC);
    } else {
        int lowOpcode = ((opcode >> 16) & 0xff) << 19;
        int highOpcode = ((opcode >> 8) & 0xff) << 19;
        CVMBool setcc = (opcode & 0xff);
        CVMCPUemitBinaryALU(con, lowOpcode, destRegID+1, lhsRegID+1,
			    CVMSPARCalurhsEncodeRegisterToken(rhsRegID+1),
			    setcc);
        CVMCPUemitBinaryALU(con, highOpcode, destRegID, lhsRegID,
			    CVMSPARCalurhsEncodeRegisterToken(rhsRegID), 
			    CVMJIT_NOSETCC);
    }
}

/* Purpose: Loads a 64-bit integer constant into a register. */
void
CVMCPUemitLoadLongConstant(CVMJITCompilationContext *con, int regID,
                           CVMJavaVal64 *value)
{
    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstant(con, regID+1, (int)val.l);
    CVMCPUemitLoadConstant(con, regID, val.l>>32);
}

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
CVMCPUemitCompare64(CVMJITCompilationContext *con,
		    int opcode, CVMCPUCondCode condCode,
                    int lhsRegID, int rhsRegID)
{
    int t;

    CVMassert(opcode == CVMCPU_CMP64_OPCODE);

    switch (condCode) {
    case CVMCPU_COND_EQ:
    case CVMCPU_COND_NE:
        /*
         * Implemented as:
         *
         *     subcc lhsreg, rhsreg, %g0
         *     bne .label1     !.label = pc+CVMCPU_INSTRUCTION_SIZE*3
         *     nop
         *     subcc lhsreg+1, rhsreg+1, %g0
         */
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
				  lhsRegID, rhsRegID);
        CVMCPUemitBranch(con, 
            3*CVMCPU_INSTRUCTION_SIZE+CVMJITcbufGetLogicalPC(con), 
            CVMCPU_COND_NE);
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
				  lhsRegID+1, rhsRegID+1);
        break;
    case CVMCPU_COND_GT:
        t = rhsRegID;
        rhsRegID = lhsRegID;
        lhsRegID = t;
        condCode = CVMCPU_COND_LT;
        goto easyCase;
    case CVMCPU_COND_LE:
        t = rhsRegID;
        rhsRegID = lhsRegID;
        lhsRegID = t;
        condCode = CVMCPU_COND_GE;
        goto easyCase; 
    case CVMCPU_COND_LT:
    case CVMCPU_COND_GE:
easyCase:
        /*
         * subcc lhsreg+1, rhsreg+1, %g0
         * subxcc lhsreg, rhsreg, %g0
         */
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
	    lhsRegID+1, rhsRegID+1);
        CVMCPUemitBinaryALURegister(con, SPARC_SUBX_OPCODE,
            CVMSPARC_g0, lhsRegID, rhsRegID,
            CVMJIT_SETCC); 
        break;
    default:
        CVMassert(CVM_FALSE); /* Unsupported condition code. */
    }
    return condCode;
}

/* Purpose: Emits instructions to convert a 32 bit int into a 64 bit int. */
void
CVMCPUemitInt2Long(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID+1, srcRegID, CVMJIT_NOSETCC);
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE, destRegID,
                              srcRegID, 31);
}

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID, srcRegID+1, CVMJIT_NOSETCC);
}

void
CVMCPUemitMulConstant(CVMJITCompilationContext* con,
		      int destreg, int lhsreg, CVMInt32 value)
{
    emitInstruction(con, SPARC_ENCODE_ALU | destreg << 25 |
                    SPARC_SMUL_OPCODE | lhsreg << 14 | (1<<13) |
                    (value & 0x1fff));

    CVMtraceJITCodegenExec({
        char destregBuf[30];
        char lhsregBuf[30];
        printPC(con);
        getRegName(destregBuf, destreg);
        getRegName(lhsregBuf, lhsreg);
        CVMconsolePrintf("	smul	%s, %d, %s",
			 lhsregBuf, value, destregBuf);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitMul(CVMJITCompilationContext* con,
              int opcode,
              int destreg,
              int lhsreg,
              int rhsreg,
              int extrareg)
{
    if (opcode == CVMCPU_MULH_OPCODE) {
	CVMassert(extrareg == CVMCPU_INVALID_REG);
	extrareg = destreg; /* need to return high word in destReg */
	opcode = SPARC_SMUL_OPCODE;
    } else if (opcode == CVMCPU_MULL_OPCODE) {
	CVMassert(extrareg == CVMCPU_INVALID_REG);
	opcode = SPARC_SMUL_OPCODE;
    } else {
	CVMassert(opcode == SPARC_UMUL_OPCODE);
	CVMassert(extrareg != CVMCPU_INVALID_REG);
    }

    emitInstruction(con, SPARC_ENCODE_ALU | destreg << 25 |
                    (CVMUint32)opcode | lhsreg << 14 | rhsreg);

    CVMtraceJITCodegenExec({
        char lhsregbuf[30];
        char rhsregbuf[30];
        char destregbuf[30];
        printPC(con);
        getRegName(lhsregbuf, lhsreg);
        getRegName(rhsregbuf, rhsreg);
        getRegName(destregbuf, destreg);
        CVMconsolePrintf("	%s	%s, %s, %s",
            getALUOpcodeName(opcode), lhsregbuf,
            rhsregbuf, destregbuf);
    });
    CVMJITdumpCodegenComments(con);

    if (extrareg != CVMCPU_INVALID_REG) {
        CVMSPARCemitRd(con, SPARC_RDY_OPCODE, extrareg);
    }
}

void
CVMCPUemitMove(CVMJITCompilationContext* con, int opcode,
	       int destRegID, CVMCPUALURhsToken srcToken,
	       CVMBool setcc)
{

    /*
     * Sparc doesn't have an integer mov instruction.
     * For integer register move, use add instruction
     * instead, and set lhs reg to r0.
     * Floating move instruction is a unary FP op.
     */
    switch (opcode){
    case CVMCPU_MOV_OPCODE:
	CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
			destRegID, CVMSPARC_g0, srcToken, setcc);
	break;
#ifdef CVM_JIT_USE_FP_HARDWARE
    case CVMCPU_FMOV_OPCODE:
	CVMassert((srcToken & CVMSPARC_MODE_CONSTANT) == 0);
	CVMassert(!setcc);
	CVMCPUemitUnaryFP(con, CVMCPU_FMOV_OPCODE, destRegID, srcToken & 0x1f);
	break;
#endif
    default:
	CVMassert(CVM_FALSE);
    }
}

void
CVMCPUemitCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken)
{
    CVMassert(opcode == CVMCPU_CMP_OPCODE || opcode == CVMCPU_CMN_OPCODE);

    /* CVMCPU_CMP_OPCODE is CVMCPU_SUB_OPCODE. 
       CVMCPU_CMN_OPCODE is CVMCPU_ADD_OPCODE. */
    CVMCPUemitBinaryALU(con, opcode,
			CVMSPARC_g0, lhsRegID, rhsToken, CVMJIT_SETCC);
}


extern void
CVMCPUemitCompareConstant(CVMJITCompilationContext* con,
			  int opcode, CVMCPUCondCode condCode,
			  int lhsRegID, CVMInt32 rhsConstValue)
{
    CVMCPUALURhsToken rhsToken;
    CVMRMResource* rhsRes = NULL;

    /* rhsConstValue may not be encodable as an immediate value */
    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsRes = NULL;
        rhsToken = CVMSPARCalurhsEncodeConstantToken(rhsConstValue);
    } else {
        /* Load rhsConstValue into a register. Use CVMSPARC_g1 as
           the scratch register.
         */
        CVMCPUemitLoadConstant(con, CVMSPARC_g1, rhsConstValue);
        rhsToken = CVMSPARCalurhsEncodeRegisterToken(CVMSPARC_g1);
    }
    CVMCPUemitCompare(con, opcode, condCode, lhsRegID, rhsToken);
    if (rhsRes != NULL) {
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), rhsRes);
    } 
}

/*
 * Putting a big, non-immediate value into a register.
 */
void
CVMCPUemitLoadConstant(
    CVMJITCompilationContext* con,
    int regno,
    CVMInt32 v)
{
    if (CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_ADD_OPCODE, v)) {
        /* add regno, %g0, v */
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
            regno, CVMSPARC_g0, CVMSPARCalurhsEncodeConstantToken(v),
            CVMJIT_NOSETCC);
    } else {
        /* Implemented as:
               sethi %hi(v), %reg
               or    %reg, %low(v), %reg
         */
        emitSetHi(con, regno, v >> 10);
	/* or in the low part if it's non-zero */
	if ((v & 0x3ff) != 0) {
	    CVMCPUemitBinaryALU(con, CVMCPU_OR_OPCODE, regno, regno,
				CVMSPARCalurhsEncodeConstantToken(v & 0x3ff),
				CVMJIT_NOSETCC);
	}
    }
    CVMJITresetSymbolName(con);
}

/* Purpose: Emits instructions to do a PC-relative load. 
   NOTE: This function must always emit the exact same number of
         instructions regardless of the offset passed to it.
*/
void
CVMCPUemitMemoryReferencePCRelative(CVMJITCompilationContext* con,
				    int opcode, int destRegID, int offset)
{
    CVMInt32 physicalPC = (CVMInt32)CVMJITcbufGetPhysicalPC(con);
    CVMInt32 addr = physicalPC + offset;

    /* Implemented as:
          sethi %hi(addr), %reg
          ld    [%reg+%low(addr)], %reg
    */
    emitSetHi(con, destRegID, addr >> 10);
    CVMCPUemitMemoryReference(con, opcode,
        destRegID, destRegID,
	CVMCPUmemspecEncodeImmediateToken(con, addr & 0x3ff));
}

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. If okToDumpCp is TRUE, the constant pool will
 * be dumped if possible. Set okToBranchAroundCpDump to FALSE if you plan
 * on generating a stackmap after calling CVMCPUemitAbsoluteCall.
 */
void
CVMCPUemitAbsoluteCall(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump)
{
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    int helperOffset;
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
        /* Use PC-relative call instruction

           The emitted code should look like the following case:

                 call target  ! return address is the address of call
                 nop
         */
        CVMSPARCemitCall(con, (int)helperOffset);
    } else 
#endif
    {
        /* Use non PC-relative jump and link instruction:
         
                 sethi %hi(target), %g1
                 jmpl  %low(target), %g1
         */
        emitSetHi(con, CVMSPARC_g1, (CVMUint32)target >> 10);
        CVMSPARCemitJumpLink(con, CVMSPARC_LR, CVMSPARC_g1,
            CVMSPARCalurhsEncodeConstantToken((CVMUint32)target & 0x3ff));
    }
    CVMJITresetSymbolName(con);
}

/*
 * Return using the correct CCM helper function.
 */
void
CVMCPUemitReturn(CVMJITCompilationContext* con, void* helper)
{
    CVMCPUemitAbsoluteCall(con, helper, CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH);
}

void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall(CVMJITCompilationContext* con,
                                             const void* target,
                                             CVMBool okToDumpCp,
                                             CVMBool okToBranchAroundCpDump)
{
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    CVMCodegenComment *comment;

    CVMJITpopCodegenComment(con, comment);
    /* load the return PC */
    CVMJITsetSymbolName((con, "return address"));
    CVMSPARCemitMakeConstant32(con, CVMSPARC_g1,
                               (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
                               6 * CVMCPU_INSTRUCTION_SIZE);

    /* Store the JSP into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush JFP to stack"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
        CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetof(CVMFrame, topOfStack));

    /* Store the return PC into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush return address to frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                                       CVMSPARC_g1, CVMCPU_JFP_REG,
                                       offsetof(CVMCompiledFrame, pcX));

    /* call the function */
    CVMJITpushCodegenComment(con, comment);
    CVMCPUemitAbsoluteCall(con, target, CVMJIT_NOCPDUMP,
                           CVMJIT_NOCPBRANCH);

    {
        /* Make sure we really only emitted 6 instructions. Otherwise,
           fix up the return address.
         */
        CVMInt32 returnPcOffset = CVMJITcbufGetLogicalPC(con) - logicalPC;
        if (returnPcOffset != 6 * CVMCPU_INSTRUCTION_SIZE) {
            CVMJITcbufPushFixup(con, logicalPC);
            CVMJITsetSymbolName((con, "return address"));
            CVMSPARCemitMakeConstant32(con, CVMSPARC_g1,
                                     (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
                                     returnPcOffset);
            CVMJITcbufPop(con);
        }
    }
}

#ifdef CVM_TRACE_JIT
static void
printAddress(int basereg, int offset, int type)
{
    CVMtraceJITCodegenExec({
        char baseregbuf[30];
        char offsetregbuf[30];
        getRegName(baseregbuf, basereg);
        CVMconsolePrintf("[%s", baseregbuf);
        switch (type) {
        case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
            CVMconsolePrintf(" + %d]", offset);
            break;
        case CVMCPU_MEMSPEC_REG_OFFSET: 
            getRegName(offsetregbuf, offset);
            CVMconsolePrintf(" + %s]", offsetregbuf);
            break;
        case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
        case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
            CVMconsolePrintf("]");
            break;
        default:
            CVMassert(CVM_FALSE);
        }
    });
}
#endif

/* NOTE: SPARC does not support post-increment and pre-decrement
   memory access. To mimic ARM hehavior extra add/sub instruction
   is emitted. */
static void
CVMSPARCemitMemoryReference(CVMJITCompilationContext* con,
                            int opcode, int destreg, int basereg,
                            CVMCPUMemSpecToken memSpecToken)
{
    CVMUint32 instruction;
    int type = CVMSPARCmemspecGetTokenType(memSpecToken);
    int offset = CVMSPARCmemspecGetTokenOffset(memSpecToken);
    CVMUint32 realToken;

    CVMassert(opcode == CVMCPU_LDR32_OPCODE || 
              opcode == CVMCPU_LDR16U_OPCODE ||
              opcode == CVMCPU_LDR16_OPCODE ||
              opcode == CVMCPU_LDR8U_OPCODE ||
              opcode == CVMCPU_LDR8_OPCODE ||
              opcode == CVMCPU_STR32_OPCODE ||
              opcode == CVMCPU_STR16_OPCODE ||
              opcode == CVMCPU_STR8_OPCODE  ||
              opcode == CVMCPU_FLDR32_OPCODE ||
              opcode == CVMCPU_FSTR32_OPCODE );

    if (type == CVMCPU_MEMSPEC_IMMEDIATE_OFFSET ||
        type == CVMCPU_MEMSPEC_REG_OFFSET) {
        realToken = memSpecToken & 0x3FFF;
    } else {
        CVMassert(type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET ||
                  type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET);
        realToken = 0x1 << 13;
    }

    if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
        /* pre-decrement the base register */
        CVMCPUemitBinaryALUConstant(con, CVMCPU_SUB_OPCODE, basereg, 
				    basereg, offset, CVMJIT_NOSETCC);
    }

    /* emit the load or store instruction */
    instruction = SPARC_ENCODE_MEMORY | (destreg << 25) |
        (CVMUint32)opcode | (basereg << 14) | realToken;
    emitInstruction(con, instruction);

    CVMJITstatsExec({
        /* This is going to be a memory reference. Determine
           if read or write */
        switch(opcode) {
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_LDR16U_OPCODE:
        case CVMCPU_LDR16_OPCODE:
        case CVMCPU_LDR8U_OPCODE:
        case CVMCPU_LDR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD); break;
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_FSTR32_OPCODE:
        case CVMCPU_STR16_OPCODE:
        case CVMCPU_STR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
        default:
            CVMassert(CVM_FALSE);
        }
    });

    CVMtraceJITCodegenExec({
        char destregbuf[30];
        printPC(con);
        getRegName(destregbuf, destreg);
        CVMconsolePrintf("	%s	", getMemOpcodeName(opcode));

	switch(opcode){
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_FSTR32_OPCODE:
	   CVMformatString(destregbuf, sizeof(destregbuf), "%%f%d", destreg);
	}
        switch(opcode) {
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_LDR16U_OPCODE:
        case CVMCPU_LDR16_OPCODE:
        case CVMCPU_LDR8U_OPCODE:
        case CVMCPU_LDR8_OPCODE:
        case CVMCPU_FLDR32_OPCODE:
            printAddress(basereg, offset, type);
            CVMconsolePrintf(", %s", destregbuf);
            break;
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_STR16_OPCODE:
        case CVMCPU_STR8_OPCODE:
        case CVMCPU_FSTR32_OPCODE:
            CVMconsolePrintf("%s, ", destregbuf);
            printAddress(basereg, offset, type);
            break;
        default:
            CVMassert(CVM_FALSE);
        }
    });
    CVMJITdumpCodegenComments(con);

    if (type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET) {
        /* post-increment the base register */
        CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, basereg, 
				    basereg, offset, CVMJIT_NOSETCC);
    }
}

void
CVMCPUemitMemoryReference(CVMJITCompilationContext* con,
                          int opcode, int destreg, int basereg,
                          CVMCPUMemSpecToken memSpecToken)
{
    int type = CVMSPARCmemspecGetTokenType(memSpecToken);
    int offset = CVMSPARCmemspecGetTokenOffset(memSpecToken);

    /* Unfortunately, we can't take advantage of SPARC double-word
       instructions, because they require double-word alignment but
       when we store/load arguments to/from java stack, it is only
       word aligned.
     */
    switch(opcode){
    case CVMCPU_LDR64_OPCODE:
	opcode = CVMCPU_LDR32_OPCODE;
	break;
    case CVMCPU_STR64_OPCODE:
	opcode = CVMCPU_STR32_OPCODE;
	break;
    case CVMCPU_FLDR64_OPCODE:
	opcode = CVMCPU_FLDR32_OPCODE;
	break;
    case CVMCPU_FSTR64_OPCODE:
	opcode = CVMCPU_FSTR32_OPCODE;
	break;
    default:
        CVMSPARCemitMemoryReference(con, opcode, destreg,
                                    basereg, memSpecToken);
        return;
    }

    /* Do the 64-bit cases */

    switch (type) {
    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
        CVMSPARCemitMemoryReference(con, opcode, destreg, basereg,
                                    memSpecToken);
        if (!CVMCPUmemspecIsEncodableAsImmediate(offset+4)) {
            goto useNewBaseReg;
        }
        CVMSPARCemitMemoryReference(con, opcode, destreg+1, basereg,
            CVMCPUmemspecEncodeImmediateToken(con, offset+4));
        break;
    case CVMCPU_MEMSPEC_REG_OFFSET:
        CVMSPARCemitMemoryReference(con, opcode, destreg, basereg,
                                    memSpecToken);
    useNewBaseReg:
        CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
            CVMSPARC_g1, basereg, 4, CVMJIT_NOSETCC);
        CVMSPARCemitMemoryReference(con, opcode, destreg+1,
            CVMSPARC_g1, memSpecToken); 
        break;
    case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
        CVMSPARCemitMemoryReference(con, opcode, destreg, basereg,
            CVMCPUmemspecEncodePostIncrementImmediateToken(con, 4));
        CVMSPARCemitMemoryReference(con, opcode, destreg+1, basereg,
            CVMCPUmemspecEncodePostIncrementImmediateToken(con, offset-4));
        break;
    case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
        /* Load/store 2nd word: */
        CVMSPARCemitMemoryReference(con, opcode, destreg+1, basereg,
            CVMCPUmemspecEncodePreDecrementImmediateToken(con, offset-4));
        /* Load/store 1st word: */
        CVMSPARCemitMemoryReference(con, opcode, destreg, basereg,
            CVMCPUmemspecEncodePreDecrementImmediateToken(con, 4));
        break;
    default :
        CVMassert(CVM_FALSE);
    }
}

/* Purpose: Emits instructions to do a load/store operation. */
void
CVMCPUemitMemoryReferenceImmediate(CVMJITCompilationContext* con,
    int opcode, int destRegID, int baseRegID, CVMInt32 immOffset)
{
    if (immOffset < CVMCPU_MAX_LOADSTORE_OFFSET ||
        immOffset > -CVMCPU_MAX_LOADSTORE_OFFSET) {
        CVMCPUemitMemoryReference(con, opcode,
	    destRegID, baseRegID,
            CVMCPUmemspecEncodeImmediateToken(con, immOffset));
    } else {
        /* Load immOffset into a register. Use g1 as the
           scratch register.
         */
        CVMCPUemitLoadConstant(con, CVMSPARC_g1, immOffset);
        CVMCPUemitMemoryReference(con, opcode,
            destRegID, baseRegID,
            CVMSPARCmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET,
				       CVMSPARC_g1));
    }
}

/* Purpose: Emits instructions to do a load/store operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
    or
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
   
   On SPARC, it's implemented in two instructions: shift instruction,
   and load/store instruction.
*/
void
CVMCPUemitArrayElementReference(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID, int indexRegID,
    int shiftOpcode, int shiftAmount)
{
    /* Use CVMSPARC_g1 as the scratch register. */

    /* emit shift instruction first: */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMSPARC_g1, 
                              indexRegID, shiftAmount);

    /* then, the load/store instruction: */
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMSPARCmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, CVMSPARC_g1));
}

/* Purpose: Emits code to computes the following expression and stores the
            result in the specified destReg:
                destReg = baseReg + shiftOpcode(indexReg, #shiftAmount)

   On SPARC, this is done in two instructions.
*/
void
CVMCPUemitComputeAddressOfArrayElement(CVMJITCompilationContext *con,
                                       int opcode, int destRegID,
                                       int baseRegID, int indexRegID,
                                       int shiftOpcode, int shiftAmount)
{
    /* Use g1 as the scratch register. */
    if (shiftAmount != 0) {
	CVMCPUemitShiftByConstant(con, shiftOpcode, destRegID,
				  indexRegID, shiftAmount);
	CVMCPUemitBinaryALURegister(con, opcode,
				    destRegID, baseRegID, destRegID,
				    CVMJIT_NOSETCC);
    } else {
	CVMCPUemitBinaryALURegister(con, opcode,
				    destRegID, baseRegID, indexRegID,
				    CVMJIT_NOSETCC);
    }
}

/* Purpose: Emits instructions for implementing the following arithmetic
            operation: destRegID = (shiftRegID << i) + addRegID
            where i is an immediate value. Any shift opcode is allowed. */
void
CVMCPUemitShiftAndAdd(CVMJITCompilationContext *con,
		      int shiftOpcode,
		      int destRegID, int shiftRegID, int addRegID,
		      CVMInt32 shiftAmount)
{
    /* This is implemented as:
     
           sll shiftRegID, shiftAmount, g1 
           add addRegID, g1, destRegID
     */
    /* Use g1 as the scratch register. */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMSPARC_g1,
                              shiftRegID, shiftAmount);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, addRegID, 
			CVMSPARCalurhsEncodeRegisterToken(CVMSPARC_g1),
			CVMJIT_NOSETCC);
}

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

/*
 * Purpose: Emits an atomic swap operation. The value to swap in is in
 *          destReg, which is also where the swapped out value will be placed.
 */
extern void
CVMCPUemitAtomicSwap(CVMJITCompilationContext* con,
		     int destReg, int addressReg)
{
    emitInstruction(con, 
		    (CVMUint32)CVMSPARC_SWAP_OPCODE |
		    addressReg << 14 | destReg << 25);

    CVMtraceJITCodegenExec({
	char addressRegBuf[30];
        char destRegBuf[30];
        printPC(con);
        getRegName(addressRegBuf, addressReg);
        getRegName(destRegBuf, destReg);
	CVMconsolePrintf("\tswap\t[%s], %s", addressRegBuf, destRegBuf);
	});
    CVMJITdumpCodegenComments(con);
}

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
int
CVMCPUemitAtomicCompareAndSwap(CVMJITCompilationContext* con,
			       int addressReg, int addressOffset,
			       int oldValReg, int newValReg)
{
    /* add addressOffset to addressReg */
    CVMJITaddCodegenComment((con, "compute CAS address"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
				CVMSPARC_g1, addressReg, addressOffset,
				CVMJIT_NOSETCC);
    addressReg = CVMSPARC_g1;

    /* emit atomic cas */
    emitInstruction(con, 
		    (CVMUint32)CVMSPARC_CAS_OPCODE |
		    CVMSPARC_g1 << 14 | oldValReg << 0 | newValReg << 25);
    CVMtraceJITCodegenExec({
	char addressRegBuf[30];
        char oldValRegBuf[30];
        char newValRegBuf[30];
        printPC(con);
        getRegName(addressRegBuf, addressReg);
        getRegName(oldValRegBuf, oldValReg);
        getRegName(newValRegBuf, newValReg);
	CVMconsolePrintf("\tcas\t[%s], %s, %s",
			 addressRegBuf, oldValRegBuf, newValRegBuf);
	});
    CVMJITdumpCodegenComments(con);

    {
	int branchLogicalPC;
	/* check if cas failed */
	CVMJITaddCodegenComment((con, "check if cas failed"));
	CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
				  oldValReg, newValReg);
	branchLogicalPC = CVMJITcbufGetLogicalPC(con);
	CVMtraceJITCodegenExec({
            const char* branchLabel = CVMJITgetSymbolName(con);
	    CVMJITresetSymbolName(con);
	    CVMJITaddCodegenComment((con, "br %s if cas failed", branchLabel));
	});
	/* Branch to failure addres if expected word not loaded. The proper
	 * branch address will be patched in by the caller. */
	CVMCPUemitBranch(con, CVMJITcbufGetLogicalPC(con), CVMCPU_COND_NE);
	return branchLogicalPC;
    }
}

#endif
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/*
 * Emit code to do a gc rendezvous.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind)
{
    const void* target = (const void*)CVMCCMruntimeGCRendezvousGlue;
    int helperOffset;
    CVMInt32 realoffset;
    CVMUint32 callInstruction;

    /* 
     * The second argument 'cbufRewind' is ignored. On SPARC, we
     * always continue the execution with the next instruction after
     * the gc rendezvous call.
     */

    /* Emit the gc rendezvous instruction(s) */
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeGCRendezvousGlue"));

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else
#endif
    {
        helperOffset = (CVMUint8*)target - con->codeEntry;
    }

    realoffset = helperOffset - CVMJITcbufGetLogicalPC(con);
    callInstruction = (SPARC_ENCODE_CALL | ((realoffset >> 2) & 0x3fffffff));

    /* Note: Do not emit nop in delay slot after 'call' here, because
     * checkGC() patches the last instruction emitted by CVMCPUemitGcCheck().
     * checkGC() is responsible for putting nop in delay slot.
     */
    emitInstruction(con, callInstruction);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("\tcall    PC=(%d)", helperOffset);
    });
    CVMJITdumpCodegenComments(con);
}
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */


#ifdef CVM_JIT_USE_FP_HARDWARE
/* Purpose: Emits instructions to do the specified floating point operation. */
void
CVMCPUemitBinaryFP( CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID, int rhsRegID)
{
    CVMassert(opcode == CVMCPU_FADD_OPCODE || opcode == CVMCPU_FSUB_OPCODE ||
              opcode == CVMCPU_FMUL_OPCODE || opcode == CVMCPU_FDIV_OPCODE ||
              opcode == CVMCPU_DADD_OPCODE || opcode == CVMCPU_DSUB_OPCODE ||
              opcode == CVMCPU_DMUL_OPCODE || opcode == CVMCPU_DDIV_OPCODE );

    emitInstruction(con,
                    SPARC_ENCODE_FPOP | destRegID << 25 | (CVMUint32)opcode |
                    lhsRegID << 14 | rhsRegID);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%%f%d, %%f%d, %%f%d",
            getALUOpcodeName(opcode),
            lhsRegID, rhsRegID, destRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitUnaryFP( CVMJITCompilationContext* con,
		    int opcode, int destRegID, int srcRegID)
{
    CVMassert(opcode == CVMCPU_FNEG_OPCODE || opcode == CVMSPARC_F2I_OPCODE ||
              opcode == CVMSPARC_I2F_OPCODE ||
	      opcode == CVMCPU_DNEG_OPCODE || opcode == CVMSPARC_D2I_OPCODE ||
              opcode == CVMSPARC_I2D_OPCODE || opcode == CVMCPU_FMOV_OPCODE ||
              opcode == CVMSPARC_F2D_OPCODE || opcode == CVMSPARC_D2F_OPCODE);

    if (opcode == CVMCPU_DNEG_OPCODE){
	/* DNEG is not a real instruction. It is an FNEG plus an FMOV */
	CVMCPUemitUnaryFP( con, CVMCPU_FNEG_OPCODE, destRegID, srcRegID);
	CVMCPUemitUnaryFP( con, CVMCPU_FMOV_OPCODE, destRegID+1, srcRegID+1);
	return;
    } else if (opcode == CVMSPARC_F2I_OPCODE || opcode == CVMSPARC_D2I_OPCODE){
	/* F2I requires special help to get Java semantics right */
	/* if the source is != itself, it must be NaN */
	CVMInt32 fixupPC;
	int cmpop = (opcode == CVMSPARC_F2I_OPCODE) ? CVMCPU_FCMP_OPCODE :
						    CVMCPU_DCMP_OPCODE;
	CVMCPUemitFCompare(con, cmpop, CVMCPU_COND_FEQ, srcRegID, srcRegID);
	fixupPC = CVMJITcbufGetLogicalPC(con);
	/* this branch is annulled, and has NO trailing nop */
	CVMSPARCemitBranch(con, 0, CVMCPU_COND_FEQ, CVM_TRUE);
	/* fstoi or fstod in delay slot of branch */
	emitInstruction(con,
                    SPARC_ENCODE_FPOP | destRegID << 25 | opcode |
                    srcRegID);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	%s	%%f%d, %%f%d\n",
		getALUOpcodeName(opcode),
		srcRegID, destRegID);
	});
	/* result is 0 in case src != src */
	CVMCPUemitLoadConstantFP(con, destRegID, 0);
	CVMJITfixupAddress(con, fixupPC, CVMJITcbufGetLogicalPC(con),
			   CVMJIT_COND_BRANCH_ADDRESS_MODE);
	return;
    }
    emitInstruction(con,
                    SPARC_ENCODE_FPOP | destRegID << 25 | (CVMUint32)opcode |
                    srcRegID);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%%f%d, %%f%d",
            getALUOpcodeName(opcode),
            srcRegID, destRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitFCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, int rhsRegID)
{
    CVMassert(opcode == CVMCPU_FCMP_OPCODE || opcode == CVMCPU_DCMP_OPCODE);

    emitInstruction(con,
                    SPARC_ENCODE_FPOP | (CVMUint32)opcode |
                    lhsRegID << 14 | rhsRegID);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%%f%d, %%f%d",
	    (opcode == CVMCPU_FCMP_OPCODE ) ? "fcmps" : "fcmpd",
            lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
    /*
     * The V8 SPARC architecture wants one instruction between
     * compare and use. The V9 architecture lifts this requirement.
     * This nop may not be necessary.
     */
    CVMCPUemitNop(con);
}

extern void
CVMCPUemitLoadConstantFP(
    CVMJITCompilationContext *con,
    int regID,
    CVMInt32 v)
{
    /*
     * there is no floating equivalent to sethi/or, so we will
     * load a constant from the constant pool.
     */
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    CVMInt32 offset;
    offset = CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR32_OPCODE, regID, CVMCPU_CP_REG, offset);
}

/* Purpose: Loads a 64-bit double constant into an FP register. */
void
CVMCPUemitLoadLongConstantFP(CVMJITCompilationContext *con, int regID,
                             CVMJavaVal64 *value)
{
    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstantFP(con, regID, val.v[0]);
    CVMCPUemitLoadConstantFP(con, regID+1, val.v[1]);
}

#endif

/*
 * CVMJITcanReachAddress - Check if toPC can be reached by an
 * instruction at fromPC using the specified addressing mode. If
 * needMargin is ture, then a margin of safety is added (usually the
 * allowed offset range is cut in half).
 */
static int absolute(int v){
    return (v<0)?-v:v;
}

CVMBool
CVMJITcanReachAddress(CVMJITCompilationContext* con,
                      int fromPC, int toPC,
                      CVMJITAddressMode mode, CVMBool needMargin)
{
    int diff = absolute(toPC - fromPC);
    switch (mode) {
    case CVMJIT_BRANCH_ADDRESS_MODE:
    case CVMJIT_COND_BRANCH_ADDRESS_MODE: {
        int maxOffset = 0x1FFFFF * 4;
        /* SPARC has +/-8 Meg branch range */
        return diff <= maxOffset;
    }
    case CVMJIT_MEMSPEC_ADDRESS_MODE: {
        int maxDiff = CVMCPU_MAX_LOADSTORE_OFFSET;
#ifdef CVMCPU_HAS_CP_REG
        fromPC = con->target.cpLogicalPC; /* relative to cp base register */
        needMargin = CVM_FALSE;
#endif
        diff = absolute(toPC - fromPC);
        if (needMargin) {
            maxDiff /= 2; /* provide a margin of error */
        }
        return (diff <= maxDiff);
    }
    default:
        return CVM_FALSE;
    }
}

/*
 * CVMJITfixupAddress - change the instruction to reference the specified
 * targetLogicalAddress.
 */
void
CVMJITfixupAddress(CVMJITCompilationContext* con,
                   int instructionLogicalAddress,
                   int targetLogicalAddress,
                   CVMJITAddressMode instructionAddressMode)
{
    CVMUint32* instructionPtr;
    CVMUint32 instruction;
    CVMInt32 offsetBits;

    CVMtraceJITCodegen((":::::Fixed instruction at %d to reference %d\n", 
            instructionLogicalAddress, targetLogicalAddress));

    if (!CVMJITcanReachAddress(con, instructionLogicalAddress,
                               targetLogicalAddress,
                               instructionAddressMode, CVM_FALSE)) {
        CVMJITerror(con, CANNOT_COMPILE, "Can't reach address");
    }

    instructionPtr = (CVMUint32*)
        CVMJITcbufLogicalToPhysical(con, instructionLogicalAddress);
    instruction = *instructionPtr;
    switch (instructionAddressMode) {
    case CVMJIT_BRANCH_ADDRESS_MODE:
    case CVMJIT_COND_BRANCH_ADDRESS_MODE:
        offsetBits = targetLogicalAddress - instructionLogicalAddress;
        offsetBits >>= 2;
        if ((instruction & SPARC_ENCODE_CALL) == SPARC_ENCODE_CALL) {
            /* It is a call instruction */
            instruction = SPARC_ENCODE_CALL;
            offsetBits &= 0x3fffffff;
        } else {
            /* It is a branch instruction */
            /*offsetBits = targetLogicalAddress - instructionLogicalAddress;*/
            instruction &= 0xffc00000; /* mask off lower 22 bits */
            /*offsetBits >>= 2;*/
            offsetBits &= 0x003fffff;  /* offset in lower 22 bits */
        }
        break;
    case CVMJIT_MEMSPEC_ADDRESS_MODE:
#ifdef CVMCPU_HAS_CP_REG
        /* offset is relative to cp base register */
        offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
#else
        offsetBits = targetLogicalAddress - instructionLogicalAddress;
#endif
        instruction &= 0xffffe000; /* mask off lower 13 bits */
        offsetBits &= 0x00001fff; /* offset in lower 12 bits */
        break;
    default:
        offsetBits = 0; /* avoid compiler warning */
        CVMassert(CVM_FALSE);
        break;
    }
    instruction |= offsetBits;
    *instructionPtr = instruction;
}
