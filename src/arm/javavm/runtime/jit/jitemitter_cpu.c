/*
 * @(#)jitemitter_cpu.c	1.263 06/10/10
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
 * ARM-specific bits.
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
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/jit/jitfixup.h"
#include "javavm/include/porting/endianness.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/jit/jit.h"

#include "javavm/include/clib.h"

/*
 * Macros for 64-bit access.
 */
#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
#define HIREG(reg)  (reg)
#define LOREG(reg)  (reg)+1
#elif CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
#define HIREG(reg)  (reg)+1
#define LOREG(reg)  (reg)
#endif

static void
CVMARMemitCompareConditional(CVMJITCompilationContext* con, int opcode,
                             int lhsRegID, CVMCPUALURhsToken rhsToken,
                             CVMCPUCondCode condCode);

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
    ap->type = CVMARM_ALURHS_CONSTANT;
    ap->constValue = v;
    return ap;
}

/* Purpose: Constructs an ALURhs to represent a register shifted by a
            constant. */
CVMCPUALURhs*
CVMARMalurhsNewShiftByConstant(
    CVMJITCompilationContext* con,
    int shiftOp,
    CVMRMResource* reg,
    CVMInt32 shiftval)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMARM_ALURHS_SHIFT_BY_CONSTANT;
    ap->shiftOp = shiftOp;
    ap->constValue = shiftval;
    ap->r1 = reg;
    return ap;
}

/* Purpose: Constructs an ALURhs to represent a register shifted by another
            register. */
CVMCPUALURhs*
CVMARMalurhsNewShiftByReg(
    CVMJITCompilationContext* con,
    int shiftOp,
    CVMRMResource* reg,
    CVMRMResource* shiftval)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMARM_ALURHS_SHIFT_BY_REGISTER;
    ap->shiftOp = shiftOp;
    ap->r1 = reg;
    ap->r2 = shiftval;
    return ap;
}

/* ALURhs token encoder APIs: ============================================= */

#define CVMARMalurhsEncodeLargeConstantToken(base, rotate) \
    (CVMARM_MODE1_CONSTANT | (rotate << 8) | base)

/* Purpose: Encodes the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */
CVMCPUALURhsToken
CVMARMalurhsEncodeToken(CVMJITCompilationContext* con,
			CVMARMALURhsType type, int constValue,
                        int shiftOp, int r1RegID, int r2RegID)
{
   CVMJITcsPushSourceRegister(con, r1RegID);

    switch(type){
    case CVMARM_ALURHS_CONSTANT: {
        CVMUint32 base;
        CVMUint32 rotate;
        CVMBool success;
        success = CVMARMmode1EncodeImmediate(constValue, &base, &rotate);
        CVMassert(success);
        return CVMARM_MODE1_CONSTANT | (rotate << 8) | base;
    }
    case CVMARM_ALURHS_SHIFT_BY_CONSTANT: {
        /* Currently, CVMARM_ALURHS_SHIFT_BY_CONSTANT is only used for
           implementing the Java shift operators.  Hence, the shift offsets
           must be between 0 and 31 (and were masked with 0x1f before getting
           here).

           NOTE: For CVMCPU_SRL_OPCODE and CVMCPU_SRA_OPCODE, a shiftOffset of
           32 is encoded as 0.  Hence, if the shiftOffset is meant to be 0,
           then the shiftop will have to be set to CVMCPU_SLL_OPCODE which
           essentially does nothing as expected.
        */
        CVMassert((constValue >= 0) && (constValue <= 31));
        shiftOp = (constValue == 0) ? CVMCPU_SLL_OPCODE : shiftOp;
        return CVMARM_MODE1_SHIFT_CONSTANT | r1RegID | shiftOp |
               constValue << 7;
    }
    case CVMARM_ALURHS_SHIFT_BY_REGISTER:
        CVMJITcsPushSourceRegister(con, r2RegID);
        return CVMARM_MODE1_SHIFT_REGISTER | r1RegID | shiftOp | r2RegID << 8;
    default:
	CVMassert(CVM_FALSE);
	return 0;
    }
}

/* Purpose: Gets the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */
CVMCPUALURhsToken
CVMARMalurhsGetToken(CVMJITCompilationContext* con, const CVMCPUALURhs *aluRhs)
{
    CVMCPUALURhsToken result;
    int r1RegID = CVMCPU_INVALID_REG;
    int r2RegID = CVMCPU_INVALID_REG;
    if (aluRhs->type == CVMARM_ALURHS_SHIFT_BY_CONSTANT) {
        r1RegID = CVMRMgetRegisterNumber(aluRhs->r1);
    } else if (aluRhs->type == CVMARM_ALURHS_SHIFT_BY_REGISTER) {
        r1RegID = CVMRMgetRegisterNumber(aluRhs->r1);
        r2RegID = CVMRMgetRegisterNumber(aluRhs->r2);
    }
    result = CVMARMalurhsEncodeToken(con, aluRhs->type, aluRhs->constValue,
                                     aluRhs->shiftOp, r1RegID, r2RegID);
    return result;
}

/* ALURhs resource management APIs: ======================================= */

/* Purpose: Pins the resources that this CVMCPUALURhs may use. */
void
CVMCPUalurhsPinResource(CVMJITRMContext* con, int opcode, CVMCPUALURhs* ap,
                        CVMRMregset target, CVMRMregset avoid)
{
    switch (ap->type){
    case CVMARM_ALURHS_CONSTANT:
        /* If we can't encode the constant as an immediate, then load it into
           a register and re-write the aluRhs as a
           CVMARM_ALURHS_SHIFT_BY_CONSTANT: */
        if (!CVMCPUalurhsIsEncodableAsImmediate(opcode, ap->constValue)) {
            ap->type = CVMARM_ALURHS_SHIFT_BY_CONSTANT;
            ap->shiftOp = CVMARM_NOSHIFT_OPCODE;
            ap->r1 = CVMRMgetResourceForConstant32(con, target, avoid,
                                                   ap->constValue);
            ap->constValue = 0;
        }
	return;
    case CVMARM_ALURHS_SHIFT_BY_CONSTANT:
        CVMRMpinResource(con, ap->r1, target, avoid);
	return;
    case CVMARM_ALURHS_SHIFT_BY_REGISTER:
        CVMRMpinResource(con, ap->r1, target, avoid);
        CVMRMpinResource(con, ap->r2, target, avoid);
	return;
    }
}

/* Purpose: Relinquishes the resources that this CVMCPUALURhsToken may use. */
void
CVMCPUalurhsRelinquishResource(CVMJITRMContext* con,
                               CVMCPUALURhs* ap)
{
    switch (ap->type){
    case CVMARM_ALURHS_CONSTANT:
	break;
    case CVMARM_ALURHS_SHIFT_BY_CONSTANT:
        CVMRMrelinquishResource(con, ap->r1);
	break;
    case CVMARM_ALURHS_SHIFT_BY_REGISTER:
        CVMRMrelinquishResource(con, ap->r1);
        CVMRMrelinquishResource(con, ap->r2);
	break;
    }
    /* Will be freed whole-sale at the very end
       free(ap); */
}

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
    mp->shiftOpcode = CVMARM_NOSHIFT_OPCODE;
    mp->shiftAmount = 0;
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
    mp->shiftOpcode = CVMARM_NOSHIFT_OPCODE;
    mp->shiftAmount = 0;
    return mp;
}

/* MemSpec token encoder APIs: ============================================ */

#define CVMARMmemspecGetTokenType(memSpecToken) \
    ((memSpecToken) & 0xff)
#define CVMARMmemspecGetTokenOffset(memSpecToken) \
    ((CVMInt32)((CVMInt16)(((memSpecToken) >> 16) & 0xffff)))
#define CVMARMmemspecGetTokenShiftOpcode(memSpecToken) \
    (((memSpecToken) >> 8) & 0x60)
#define CVMARMmemspecGetTokenShiftAmount(memSpecToken) \
    (((memSpecToken) >> 8) & 0x1f)

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMARMmemspecGetToken(CVMJITCompilationContext *con,
		      const CVMCPUMemSpec *memSpec)
{
    CVMCPUMemSpecToken token;
    int type = memSpec->type;
    int offset;
    int shiftOp = memSpec->shiftOpcode;
    int shiftImm = memSpec->shiftAmount;

    offset = (type == CVMCPU_MEMSPEC_REG_OFFSET) ?
              CVMRMgetRegisterNumber(memSpec->offsetReg) :
              memSpec->offsetValue;
    token = CVMARMmemspecEncodeToken(type, offset, shiftOp, shiftImm);
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
            self->shiftOpcode = CVMARM_NOSHIFT_OPCODE;
            self->shiftAmount = 0;
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

/* Private ARM specific defintions: ======================================= */

#define ARM_MVN_OPCODE (0x1e << 20)
#define ARM_ADC_OPCODE (0x0a << 20) /* needed for ADD64 codegen comments */
#define ARM_SBC_OPCODE (0x0c << 20) /* needed for SUB64 codegen comments */
#define ARM_RSB_OPCODE CVMARM_RSB_OPCODE
#define ARM_RSC_OPCODE (0x0e << 20)
#define ARM_UMULL_OPCODE 0x00800090  /* reg32hi,reg32lo = reg32 * reg32.*/

/* Flow control opcodes: */
#define ARM_B_OPCODE                (5 << 25)   /* branch. */
#define ARM_BL_OPCODE               (ARM_B_OPCODE | (1 << 24)) /* b and link.*/

#ifdef CVM_MP_SAFE
#define ARM_LDREX_OPCODE (0x01900f9f)
#define ARM_STREX_OPCODE (0x01800f90)
#define ARM_MCR_OPCODE (0x0e000000)
#define C7 7
#define C10 10
#endif

/* condCodes */
#define ARM_MAKE_CONDCODE_BITS(cond) ((CVMUint32)((CVMUint32)(cond) << 28))

/* Support for emitting ARM mode 1 operands: */
#define ROTATE_RIGHT_32BIT(value, rotateValue) \
    ((((CVMUint32)(value)) >> (rotateValue)) | \
     (((CVMUint32)(value)) << (32 - (rotateValue))))

#define ROTATE_LEFT_32BIT(value, rotateValue) \
    ((((CVMUint32)(value)) << (rotateValue)) | \
     (((CVMUint32)(value)) >> (32 - (rotateValue))))

#ifdef CVM_TRACE_JIT
static const char* const conditions[] = {
    "eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "", "nv"
};

/* Purpose: Returns the name of the specified opcode. */
static const char *getOpcodeName(CVMInt32 opcode)
{
    const char *name = NULL;
    switch(opcode) {
    case CVMCPU_LDR32_OPCODE:   name = "ldr"; break;
    case CVMCPU_STR32_OPCODE:   name = "str"; break;
    case CVMCPU_LDR16U_OPCODE:  name = "ldrh"; break;
    case CVMCPU_LDR16_OPCODE:   name = "ldrsh"; break;
    case CVMCPU_STR16_OPCODE:   name = "strh"; break;
    case CVMCPU_LDR8U_OPCODE:   name = "ldrb"; break;
    case CVMCPU_LDR8_OPCODE:    name = "ldrsb"; break;
    case CVMCPU_STR8_OPCODE:    name = "strb"; break;

    case CVMCPU_ADD_OPCODE:     name = "add"; break;
    case ARM_ADC_OPCODE:        name = "adc"; break;
    case CVMCPU_SUB_OPCODE:     name = "sub"; break;
    case ARM_SBC_OPCODE:        name = "sbc"; break;
    case ARM_RSB_OPCODE:        name = "rsb"; break;
    case ARM_RSC_OPCODE:        name = "rsc"; break;
    case CVMCPU_AND_OPCODE:     name = "and"; break;
    case CVMCPU_OR_OPCODE:      name = "orr"; break;
    case CVMCPU_XOR_OPCODE:     name = "eor"; break;
    case CVMCPU_BIC_OPCODE:     name = "bic"; break;
    case CVMCPU_MOV_OPCODE:     name = "mov"; break;
    case ARM_MVN_OPCODE:        name = "mvn"; break;
    case CVMCPU_CMP_OPCODE:     name = "cmp"; break;
    case CVMCPU_CMN_OPCODE:     name = "cmn"; break;

    case CVMCPU_FADD_OPCODE:	name = "fadds"; break;
    case CVMCPU_FSUB_OPCODE:	name = "fsubs"; break;
    case CVMCPU_FMUL_OPCODE:	name = "fmuls"; break;
    case CVMCPU_FDIV_OPCODE:	name = "fdivs"; break;

    case CVMCPU_DADD_OPCODE:	name = "faddd"; break;
    case CVMCPU_DSUB_OPCODE:	name = "fsubd"; break;
    case CVMCPU_DMUL_OPCODE:	name = "fmuld"; break;
    case CVMCPU_DDIV_OPCODE:	name = "fdivd"; break;

    case CVMCPU_FLDR32_OPCODE:  name = "flds"; break;
    case CVMCPU_FLDR64_OPCODE:  name = "fldd"; break;
    case CVMCPU_FSTR32_OPCODE:  name = "fsts"; break;
    case CVMCPU_FSTR64_OPCODE:  name = "fstd"; break;

    case CVMARM_FLDRM32_OPCODE: name = "fldms"; break;
    case CVMARM_FLDRM64_OPCODE: name = "fldmd"; break;
    case CVMARM_FSTRM32_OPCODE: name = "fstms"; break;
    case CVMARM_FSTRM64_OPCODE: name = "fstmd"; break;

    case CVMCPU_FNEG_OPCODE:	name = "fnegs";   break;
    case CVMCPU_DNEG_OPCODE:	name = "fnegd";   break;

    case CVMARM_F2I_OPCODE:	name = "fsitos";  break;
    case CVMARM_I2F_OPCODE:	name = "ftosizs"; break;
    case CVMARM_D2I_OPCODE:	name = "ftosizd"; break;
    case CVMARM_I2D_OPCODE:	name = "fsitod";  break;
    case CVMARM_F2D_OPCODE:	name = "fcvtds";  break;
    case CVMARM_D2F_OPCODE:	name = "fcvtsd";  break;

    case CVMCPU_FMOV_OPCODE:	name = "fcpys";   break;
    case CVMCPU_DMOV_OPCODE:	name = "fcpyd";   break;
    case CVMARM_MOVFA_OPCODE:	name = "fmsr";    break;
    case CVMARM_MOVAF_OPCODE:	name = "fmrs";    break;
    case CVMARM_MOVDA_OPCODE:	name = "fmdrr";   break;
    case CVMARM_MOVAD_OPCODE:	name = "fmrrd";   break;

    case CVMARM_MOVSA_OPCODE:	name = "fmxr";   break;
    case CVMARM_MOVAS_OPCODE:	name = "fmrx";   break;

    case CVMCPU_FCMP_OPCODE:	name = "fcmps";   break;
    case CVMARM_FCMPES_OPCODE:	name = "fcmpes";  break;
    case CVMCPU_DCMP_OPCODE:	name = "fcmpd";   break;
    case CVMARM_DCMPED_OPCODE:	name = "fcmped";  break;
    case CVMARM_FMSTAT_OPCODE:	name = "fmstat";  break;

    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", opcode);
        CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;
}

#endif

/* Purpose: Encodes a large ARM mode1 constant using rotate right if possible.*/
/* Returns: CVM_TRUE is possible, else returns CVM_FALSE. */
CVMBool CVMARMmode1EncodeImmediate(CVMUint32 value, CVMUint32 *baseValue,
                                   CVMUint32 *rotateValue)
{
    CVMUint32 rotate;
    CVMUint32 base;
    CVMUint32 result;

    /* Check for the likely and easy case first: */
    if (value < 256) {
        if (baseValue != NULL) {
            CVMassert(rotateValue != NULL);
            *baseValue = value;
            *rotateValue = 0;
        }
        return CVM_TRUE;
    }

    /* Try to encode the constant with each of the 16 possible rotation
       values: */
    for (rotate = 2; rotate < 32; rotate += 2) {
        base = ROTATE_LEFT_32BIT(value, rotate) & 0xFF;
        result = ROTATE_RIGHT_32BIT(base, rotate);
        if (result == value) {
            if (baseValue != NULL) {
                CVMassert(rotateValue != NULL);
                *baseValue = base;
                *rotateValue = rotate / 2;
            }
            return CVM_TRUE; /* Successfully encoded it. */
        }
    }
    return CVM_FALSE; /* Not able to encode it. */
}

/*
 * Functions that assists in tracing generated code as assembler output.
 */

#ifdef CVM_TRACE_JIT

#ifdef CVMCPU_HAS_CP_REG
#define ARM_CP_REGNAME "rCP"
#else
#define ARM_CP_REGNAME "v7"
#endif

#if defined(CVMJIT_TRAP_BASED_GC_CHECKS) && \
    !defined(CVMCPU_HAS_VOLATILE_GC_REG)
#define ARM_GC_REGNAME "rGC"
#else
#define ARM_GC_REGNAME "v8"
#endif


static const char *const regNames[] = {
    "a1", "a2", "a3", "a4", "rJFP", "rJSP", "v3", "v4", "v5", "v6",
    ARM_CP_REGNAME, ARM_GC_REGNAME, "ip", "sp", "lr", "pc"
};

static const char*
shiftname(int shiftop)
{
    switch (shiftop){
    case CVMCPU_SLL_OPCODE:
	return "LSL";
    case CVMCPU_SRL_OPCODE:
	return "LSR";
    case CVMCPU_SRA_OPCODE:
	return "ASR";
    default:
	return "???";
    }
}

static void
formatMode1(char* buf, CVMCPUALURhsToken token)
{
    CVMARMALURhsType type = 0;
    int constValue = 0;
    int r1RegID = 0;
    int r2RegID = 0;
    int shiftOp = 0;

    if ((token & 0x0e000000) == CVMARM_MODE1_CONSTANT) {
        int base = token & 0xff;
        int rotate = (token >> 7) & 0x1e;
        type = CVMARM_ALURHS_CONSTANT;
        constValue = ROTATE_RIGHT_32BIT(base, rotate);
    } else {
        shiftOp = token & 0x60;
        r1RegID = token &0xf;
        if ((token & 0x0e000090) == CVMARM_MODE1_SHIFT_REGISTER) {
            type = CVMARM_ALURHS_SHIFT_BY_REGISTER;
            r2RegID = (token >> 8) & 0x1f;
        } else {
            type = CVMARM_ALURHS_SHIFT_BY_CONSTANT;
            constValue = (token >> 7) & 0xf;
        }
    }

    switch(type){
    case CVMARM_ALURHS_CONSTANT:
        sprintf(buf, "#%d", constValue);
	break;
    case CVMARM_ALURHS_SHIFT_BY_CONSTANT: {
        sprintf(buf, "%s %s #%d", regNames[r1RegID], shiftname(shiftOp),
                constValue);
	break;
    }
    case CVMARM_ALURHS_SHIFT_BY_REGISTER:
        sprintf(buf,"%s %s %s", regNames[r1RegID], shiftname(shiftOp),
                regNames[r2RegID]);
	break;
    }
}

/* Purpose: Dumps the content of an ARM mode 2 operand. */
void
dumpMode2Instruction(CVMJITCompilationContext *con,
		     CVMUint32 instruction, CVMCPUCondCode condCode)
{
    int opcode = instruction & 0x0c500000;
    int destreg = (instruction >> 12) & 0xF;
    int basereg = (instruction >> 16) & 0xF;
    CVMBool offsetIsToBeAdded = (instruction & ARM_LOADSTORE_ADDOFFSET);
    CVMBool isPreIndex = (instruction & ARM_LOADSTORE_PREINDEX);

    CVMassert(opcode == CVMCPU_LDR32_OPCODE ||
              opcode == CVMCPU_STR32_OPCODE ||
              opcode == CVMCPU_STR8_OPCODE);

    CVMconsolePrintf("	%s%s	%s, [%s", getOpcodeName(opcode),
        conditions[condCode], regNames[destreg], regNames[basereg]);

    if (isPreIndex) {
        CVMconsolePrintf(", ");
    } else {
        CVMconsolePrintf("], ");
    }

    if (instruction & ARM_LOADSTORE_REGISTER_OFFSET) {
        int offsetreg = instruction & 0xF;
        int shiftImmediate = (instruction >> 7) & 0x1F;
        int shiftOp = instruction & (0x3 << 5);
        CVMconsolePrintf("%c%s", (offsetIsToBeAdded?'+':'-'),
                         regNames[offsetreg]);
        if (shiftImmediate != 0) {
             CVMconsolePrintf(", %s #%d", shiftname(shiftOp), shiftImmediate);
        }
    } else {
        CVMUint32 immediateValue = instruction & 0xFFF;
        CVMconsolePrintf("#%c%d", (offsetIsToBeAdded?'+':'-'), immediateValue);
    }

    if (isPreIndex) {
        CVMconsolePrintf("]");
    }
    if (instruction & ARM_LOADSTORE_WRITEBACK) {
        CVMconsolePrintf("!");
    }
}

/* Purpose: Dumps the content of an ARM mode 5 operand. */
void
dumpMode5Instruction(CVMJITCompilationContext *con,
		     CVMUint32 instruction, CVMCPUCondCode condCode)
{
#ifdef CVM_JIT_USE_FP_HARDWARE
    int opcode, destreg, basereg;
    int P, U, W, L;
    int type, size, offset, puw;

    /* VFP load/store addressing modes */
    enum {
        unindexed = 2,
        increment,
        negativeOffset,
        decrement,
        positiveOffset
    };

    opcode  = instruction & 0x0FF00F00;
    destreg = (instruction >> 12) & 0xF;
    destreg = ((instruction >> 22) & 0x1) | destreg << 1;
    basereg = (instruction >> 16) & 0xF;
    P       = (instruction >> 24) & 0x1;
    U       = (instruction >> 23) & 0x1;
    W       = (instruction >> 21) & 0x1;
    L       = (instruction >> 20) & 0x1;
    type    = (instruction >> 8) & 0xF;
    size    = (instruction & 0xFF) + destreg;
    offset  = instruction & 0x000000FF;
    puw     = P << 2 | U << 1 | W;

    switch (puw) {
        case unindexed:
        case increment:
        case decrement: {
            CVMconsolePrintf("	f%sm", (L ? "ld" : "st"));

            if (basereg == CVMARM_sp) {
                CVMconsolePrintf("%c", L == P ? 'e' : 'f');
                CVMconsolePrintf("%c", L == U ? 'd' : 'a');
            } else {
                CVMconsolePrintf("%c", L == P ? 'i' : 'd');
                CVMconsolePrintf("%c", L == U ? 'b' : 'a');
            }
            CVMconsolePrintf("%c", type == 0xA ? 's' : 'd');
            CVMconsolePrintf("%s	%s", conditions[condCode],
                         regNames[basereg]);

            if (W) {
                CVMconsolePrintf("!");
            }

            CVMconsolePrintf(", {");
            CVMconsolePrintf("%%f%d", destreg);
            destreg += (type == 0xA ? 1:2);
            while (destreg < size) {
                CVMconsolePrintf(", ");
	        CVMconsolePrintf("%%f%d", destreg);
	        destreg += (type == 0xA ? 1:2);
            }
            CVMconsolePrintf("}");
            break;
        }
        case negativeOffset:
        case positiveOffset: {
            CVMconsolePrintf("	f%s%c%s	%%f%d, [%s, ",
                         (L ? "ld" : "st"),
                         (type == 0xA ? 's' : 'd'),
                          conditions[condCode], destreg, regNames[basereg]);

            CVMconsolePrintf("#%c%d", (U ? '+' : '-'), offset);
            CVMconsolePrintf("]");
	    break;
        }
        default: {
            CVMconsolePrintf("Unknown opcode 0x%08x", opcode);
            CVMassert(CVM_FALSE); /* Unknown opcode name */
        }
    }
#endif /* CVM_JIT_USE_FP_HARDWARE */
}

static void
printPC(CVMJITCompilationContext* con)
{
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMconsolePrintf("0x%8.8x\t%d \t%d:",
		     CVMJITcbufGetPhysicalPC(con) - 4,
		     CVMJITcbufGetLogicalPC(con) - 4,
                     CVMJITcbufGetLogicalInstructionPC(con));
#else
    CVMconsolePrintf("0x%8.8x\t%d:",
		     CVMJITcbufGetPhysicalPC(con) - 4,
		     CVMJITcbufGetLogicalPC(con) - 4);
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/
}
#endif /* CVM_TRACE_JIT */

static void
emitInstruction(CVMJITCompilationContext* con,
		CVMUint32 instruction)
{
#if 0
    CVMtraceJITCodegen(("0x%8.8x\t",  instruction));
#endif
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMJITcsCalcInstructionPos(con);
    CVMJITcbufEmitPC(con, CVMJITcbufGetLogicalInstructionPC(con), instruction);
    CVMJITcsPrepareForNextInstruction(con);
#else
    CVMJITcbufEmit(con, instruction);
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/
}

/*
 * Emit a 32 bit value (aka .word).
 */
void
CVMJITemitWord(CVMJITCompilationContext *con, CVMInt32 wordVal)
{
    CVMJITcsSetEmitInPlace(con);
    emitInstruction(con, wordVal);
    CVMJITcsClearEmitInPlace(con);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	.word	%d", wordVal);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitNop(CVMJITCompilationContext *con)
{
    CVMJITcsSetEmitInPlace(con);
    emitInstruction(con, CVMCPU_NOP_INSTRUCTION);
    CVMJITcsClearEmitInPlace(con);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	nop");
    });
    CVMJITdumpCodegenComments(con);
}

/*
 * Binary ALU operation w/ 16-bit constant, always using two instructions.
 */
static void
emitBinaryALUConstant16Conditional(CVMJITCompilationContext* con, int opcode,
				   int destReg, int sourceReg,
				   CVMUint32 constant,
				   CVMBool setcc, CVMCPUCondCode condCode)
{
    /* Scaled components of the 16-bit constant */
    CVMUint32 constantHi = constant & 0xff00;
    CVMUint32 constantLo = constant & 0x00ff;

    CVMassert(constant < 0x10000); /* Better fit in 16-bits */

    CVMassert(opcode == CVMCPU_BIC_OPCODE ||
	      opcode == CVMCPU_OR_OPCODE ||
	      opcode == CVMCPU_XOR_OPCODE ||
	      opcode == CVMCPU_ADD_OPCODE ||
	      opcode == CVMCPU_SUB_OPCODE);

    /* Encode as two instructions */
    CVMJITcsPushSourceRegister(con, sourceReg);
    CVMCPUemitBinaryALUConditional(con, opcode, destReg, sourceReg,
			CVMARMalurhsEncodeConstantToken(con, constantLo),
			setcc, condCode);
    CVMCPUemitBinaryALUConditional(con, opcode, destReg, destReg,
			CVMARMalurhsEncodeConstantToken(con, constantHi),
			setcc, condCode);
}

#ifdef CVMCPU_HAS_CP_REG
/* Purpose: Set up constant pool base register */
void
CVMCPUemitLoadConstantPoolBaseRegister(CVMJITCompilationContext *con)
{

    CVMInt32 offset;

    /*
     * Load the constant pool base register. It can be as far
     * as 64k-4 away, so it will take two adds to do this. Note
     * that even if it could be done in one, we already reserved
     * two instruction for it, so we might as well just use them
     * both.
     */
    offset = con->target.cpLogicalPC - CVMJITcbufGetLogicalPC(con) - 8;

    CVMJITprintCodegenComment(("setup cp base register"));
    aluConstant16ScaledConditional(con, CVMCPU_ADD_OPCODE,
				   CVMCPU_CP_REG, CVMARM_PC,
				   offset, 0, CVMCPU_COND_AL);

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
			    CVMARMalurhsEncodeConstantToken(con, value),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 240+
	 * words of locals, so it's ok to just not compile this method.
         */
	CVMJITerror(con, CANNOT_COMPILE, "too many locals");
    }
}

/*
 * Purpose: Stack limit check at the start of each method, corresponding to:
 *
 *     ldr      a4, [sp, #OFFSET_CVMCCExecEnv_stackChunkEnd];
 *     str      lr, [JFP, #OFFSET_CVMCCExecEnv_pcX];
 *     cmp      a4, a2;
 *     bls      letInterpreterDoInvoke;
 *
 * Schedule the lr flush to give 'a4' time to "settle".
 */
void
CVMCPUemitStackLimitCheckAndStoreReturnAddr(CVMJITCompilationContext* con)
{
    CVMJITaddCodegenComment((con, "Store LR into frame"));
    CVMassert(CVMCPUmemspecIsEncodableAsImmediate(
                    CVMoffsetof(CVMCompiledFrame, pcX)));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE, CVMARM_LR,
        CVMCPU_JFP_REG, CVMoffsetof(CVMCompiledFrame, pcX));

    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_LS,
                              CVMCPU_ARG4_REG, CVMCPU_ARG2_REG);
    CVMJITaddCodegenComment((con, "letInterpreterDoInvoke"));
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    CVMCPUemitBranch(con,
        CVMCCMcodeCacheCopyHelperOffset(con,
            CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr),
        CVMCPU_COND_LS);
#else
    /*
     * This accesses the CP, and such is incompatible with CP_REG code
     * (can't access constant pool in prologue)
     */
#ifdef CVMCPU_HAS_CP_REG
#error "Cannot support 'bls letInterpreterDoInvoke' when glue not in code cache and doing CP register setups"
#else
    /* load the new pc from memory */
    CVMCPUemitLoadConstantConditional(con, CVMARM_PC,
        (CVMInt32)CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr,
        CVMCPU_COND_LS);
#endif
#endif
}

/*
 *  Purpose: Emits code to invoke method through MB.
 *           MB is already in CVMCPU_ARG1_REG.
 */
void
CVMCPUemitInvokeMethod(CVMJITCompilationContext* con)
{
    CVMUint32 off;

    /* MOV LR, PC */
    CVMJITaddCodegenComment((con, "setup return address"));
    CVMJITcsSetEmitInPlace(con);
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   CVMARM_LR, CVMARM_PC, CVMJIT_NOSETCC);

    /* LDR PC, [MB + #OFFSET_JITInvoker]
     */
    CVMJITaddCodegenComment((con, "call method through mb"));
    off = CVMoffsetof(CVMMethodBlock, jitInvokerX);
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE, CVMARM_PC,
                                       CVMCPU_ARG1_REG, off);
    CVMJITcsClearEmitInPlace(con);

    CVMJITcsBeginBlock(con);
}

/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the CVMCPU_JSR_RETURN_ADDRESS_SET == LR.
 */
void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno)
{
    CVMassert(regno == CVMARM_LR);
}

/*
 * Branch to the address in the specified register.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno)
{
    CVMJITcsSetEmitInPlace(con);
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   CVMARM_PC, regno, CVMJIT_NOSETCC);
    CVMJITcsClearEmitInPlace(con);
}

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The table entry for index 0 will be generated right after
 * any instructions that are generated here..
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int indexRegNo)
{
    /*
     * Since on arm the pc is already offset by 8, we just need to add
     * key*4 to pc and then pad with a nop so things line up right.
     */
    CVMCPUALURhsToken token;
    token = CVMARMalurhsEncodeShiftByConstantToken(con,
	indexRegNo, CVMCPU_SLL_OPCODE, 2);
    CVMJITcsSetEmitInPlace(con);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
			CVMARM_PC, CVMARM_PC, token, CVMJIT_NOSETCC);
    CVMJITcsClearEmitInPlace(con);
    CVMCPUemitNop(con);
}

void
CVMCPUemitPopFrame(CVMJITCompilationContext* con, int resultSize)
{
    CVMUint32 offset; /* offset from JFP for new JSP */
    CVMUint32 rotate;
    CVMUint32 base;

    /* We want to set JSP to the address of the locals + the resultSize */
    offset = (con->numberLocalWords - resultSize) * sizeof(CVMJavaVal32);

    if (CVMARMmode1EncodeImmediate(offset, &base, &rotate)) {
        CVMJITcsSetDestRegister(con, CVMCPU_JSP_REG);
        CVMJITcsPushSourceRegister(con, CVMCPU_JFP_REG);
        /* If it's encodable, we quickly emit the instruction: */
        emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
            (CVMUint32)CVMCPU_SUB_OPCODE |
	    CVMCPU_JFP_REG << 16 | CVMCPU_JSP_REG << 12 |
            CVMARM_MODE1_CONSTANT | rotate << 8 | base);

        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	sub	JSP, JFP, #%d", offset);
        });
        CVMJITdumpCodegenComments(con);

    } else {
        /* Else, we make use of all the other emitter functions that are
           already available for emitting and tracking the pieces we need: */
        CVMRMResource *offsetRes;
        CVMUint32 offsetReg;

        /* Load the offset as a constant into a register: */
        offsetRes = CVMRMgetResource(CVMRM_INT_REGS(con),
				     CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
        offsetReg = CVMRMgetRegisterNumber(offsetRes);
        CVMCPUemitLoadConstant(con, offsetReg, offset);

        /* Pop the frame: */
        CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
				    CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetReg,
				    CVMJIT_NOSETCC);

        CVMRMunpinResource(CVMRM_INT_REGS(con), offsetRes);
    }
}

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS

/*
 * Patch branch instruction at location "instrAddr" to branch to offset
 * "offset" from "instrAddr".
 */
void
CVMCPUpatchBranchInstruction(int offset, CVMUint8* instrAddr)
{
    CVMCPUInstruction branch;
    /* There better already be an unconditional bl at this address */
    CVMassert((*(CVMUint32*)instrAddr & 0xff000000) ==
	      (ARM_BL_OPCODE | ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL)));
    branch = CVMARMgetBranchInstruction(CVMCPU_COND_AL, offset, CVM_TRUE);
    *((CVMCPUInstruction*)instrAddr) = branch;
}

#endif

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
CVMCPUInstruction
CVMARMgetBranchInstruction(CVMCPUCondCode condCode, int offset, CVMBool link)
{
    CVMUint32 opcode = (link ? ARM_BL_OPCODE : ARM_B_OPCODE);
    int realoffset = (offset - 8) >> 2; /* adjust by 8 on ARM */
    CVMassert((realoffset & 0xff800000) == 0 ||
	      (realoffset & 0xff800000) == 0xff800000);
    return (ARM_MAKE_CONDCODE_BITS(condCode) | opcode
	    | ((CVMUint32)realoffset & 0x00ffffff));
}

/* Purpose: Emits a branch or branch and link instruction. */

void
CVMARMemitBranch(CVMJITCompilationContext* con,
	         int logicalPC, CVMCPUCondCode condCode,
                 CVMBool link, CVMJITFixupElement** fixupList)
{
    CVMUint32 branchInstruction;

    CVMJITcsSetStatusInstruction(con, condCode);
    CVMJITcsSetBranchInstruction(con);
#ifdef IAI_CS_EXCEPTION_ENHANCEMENT2
    CVMJITcsSetEmitInPlaceWithBufSizeAdjust(con, \
        CVMJITcsIsArrayIndexOutofBoundsBranch(con), sizeof(CVMCPUInstruction));
#else
    CVMJITcsSetEmitInPlace(con);
#endif

    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }

    branchInstruction =
        CVMARMgetBranchInstruction(condCode,
				   logicalPC - CVMJITcbufGetLogicalPC(con),
				   link);
    emitInstruction(con, branchInstruction);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s%s	PC=(%d)",
                         link ? "bl" : "b",
                         conditions[condCode], logicalPC);
    });
    CVMJITcsClearEmitInPlace(con);
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified conditional 32 bit unary
            ALU operation. */
void
CVMCPUemitUnaryALUConditional(CVMJITCompilationContext *con, int opcode,
			      int destRegID, int srcRegID,
			      CVMBool setcc, CVMCPUCondCode condCode)
{
    switch (opcode) {
        case CVMCPU_NOT_OPCODE:
            /* reg32 = (reg32 == 0)?1:0. */
            CVMassert(condCode = CVMCPU_COND_AL);
            CVMCPUemitCompareConstant(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_AL,
                                      srcRegID, 0);
            CVMCPUemitLoadConstantConditional(con, destRegID, 0, CVMCPU_COND_NE);
            CVMCPUemitLoadConstantConditional(con, destRegID, 1, CVMCPU_COND_EQ);
            break;
        case CVMCPU_INT2BIT_OPCODE:
            /* reg32 = (reg32 != 0)?1:0. */
            CVMassert(condCode = CVMCPU_COND_AL);
            CVMCPUemitBinaryALUConstant(con, CVMCPU_SUB_OPCODE, destRegID,
                srcRegID, 0, CVMJIT_SETCC);
            CVMCPUemitLoadConstantConditional(con, destRegID, 1, CVMCPU_COND_NE);
            break;
        case CVMCPU_NEG_OPCODE:
            CVMCPUemitBinaryALUConditional(con, ARM_RSB_OPCODE, destRegID,
                srcRegID, CVMCPUALURhsTokenConstZero, setcc, condCode);
            break;
        default:
            CVMassert(CVM_FALSE);
    }
}

/* Purpose: Emits instructions to do the specified conditional 32 bit ALU
            operation. */
void
CVMCPUemitBinaryALUConditional(CVMJITCompilationContext* con,
			       int opcode, int destRegID, int lhsRegID,
			       CVMCPUALURhsToken rhsToken,
			       CVMBool setcc, CVMCPUCondCode condCode)
{
    CVMassert(opcode == CVMCPU_ADD_OPCODE || opcode == ARM_ADC_OPCODE    ||
              opcode == CVMCPU_SUB_OPCODE || opcode == ARM_SBC_OPCODE    ||
              opcode == ARM_RSB_OPCODE    || opcode == ARM_RSC_OPCODE    ||
              opcode == CVMCPU_AND_OPCODE || opcode == CVMCPU_OR_OPCODE  ||
              opcode == CVMCPU_XOR_OPCODE || opcode == CVMCPU_BIC_OPCODE);

    CVMJITcsSetDestRegister(con, destRegID);
    CVMJITcsPushSourceRegister(con, lhsRegID);
    CVMJITcsSetCompareInstruction(con, setcc);
    CVMJITcsSetStatusBinaryALUInstruction(con, opcode);
    CVMJITcsSetStatusInstruction(con, condCode);

    emitInstruction(con,
		    ARM_MAKE_CONDCODE_BITS(condCode) | (CVMUint32)opcode |
		    (setcc ? (1 << 20) : 0) |
                    lhsRegID << 16 | destRegID << 12 | rhsToken);

    CVMtraceJITCodegenExec({
        char mode1buf[48];
        printPC(con);
        formatMode1(mode1buf, rhsToken);
        CVMconsolePrintf("	%s%s%s	%s, %s, %s",
            getOpcodeName(opcode), conditions[condCode], setcc?"s":"",
            regNames[destRegID], regNames[lhsRegID],  mode1buf);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified conditional 32 bit ALU
            operation. */
void
CVMCPUemitBinaryALUConstantConditional(CVMJITCompilationContext* con,
                     int opcode, int destRegID, int lhsRegID,
                     CVMInt32 rhsConstValue,
		     CVMBool setcc, CVMCPUCondCode condCode)
{
    CVMCPUALURhsToken rhsToken;
    CVMRMResource *rhsRes;


    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsRes = NULL;
        rhsToken = CVMARMalurhsEncodeConstantToken(con, rhsConstValue);
    } else if ((opcode == CVMCPU_BIC_OPCODE ||
		opcode == CVMCPU_OR_OPCODE ||
		opcode == CVMCPU_XOR_OPCODE ||
		opcode == CVMCPU_ADD_OPCODE ||
		opcode == CVMCPU_SUB_OPCODE) &&
	       (rhsConstValue < 0x10000)) {
	/* If it is a 16-bit BIC, OR, XOR, ADD, or SUB, then emit as
	   two instructions rather than loading a constant. */
	emitBinaryALUConstant16Conditional(con, opcode, destRegID, lhsRegID,
					   rhsConstValue, setcc, condCode);
	return;
    } else  {
        rhsRes = CVMRMgetResourceForConstant32(CVMRM_INT_REGS(con),
			CVMRM_ANY_SET, CVMRM_EMPTY_SET, rhsConstValue);
        rhsToken = CVMARMalurhsEncodeRegisterToken(con,
			CVMRMgetRegisterNumber(rhsRes));
    }
    CVMCPUemitBinaryALUConditional(con, opcode,
				   destRegID, lhsRegID, rhsToken,
				   setcc, condCode);
    if (rhsRes != NULL) {
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), rhsRes);
    }
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.*/
void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, CVMUint32 shiftAmount)
{
#ifdef CVM_DEBUG_ASSERTS
    if (opcode == CVMCPU_SLL_OPCODE) {
        CVMassert(shiftAmount < 32);
    }
#endif
    /*  mov rDest, rSrc, SHIFTOPCODE #i */
    CVMCPUemitMove(con, CVMCPU_MOV_OPCODE, destRegID,
        CVMARMalurhsEncodeShiftByConstantToken(con,
					       srcRegID, opcode, shiftAmount),
	CVMJIT_NOSETCC);
}

/* Purpose: Emits instructions to do the specified 64 bit unary ALU
            operation. */
void
CVMCPUemitUnaryALU64(CVMJITCompilationContext *con, int opcode,
                     int destRegID, int srcRegID)
{
    switch (opcode) {
        case CVMCPU_NEG64_OPCODE:
            CVMCPUemitBinaryALU(con, ARM_RSB_OPCODE,
                LOREG(destRegID), LOREG(srcRegID), CVMCPUALURhsTokenConstZero,
                CVMJIT_SETCC);
            CVMCPUemitBinaryALU(con, ARM_RSC_OPCODE,
                HIREG(destRegID), HIREG(srcRegID), CVMCPUALURhsTokenConstZero,
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
        CVMCPUemitMul(con, ARM_UMULL_OPCODE, HIREG(destRegID),
                      LOREG(lhsRegID), LOREG(rhsRegID), LOREG(destRegID));
        CVMCPUemitMul(con, CVMARM_MLA_OPCODE, HIREG(destRegID),
                      HIREG(lhsRegID), LOREG(rhsRegID), HIREG(destRegID));
        CVMCPUemitMul(con, CVMARM_MLA_OPCODE, HIREG(destRegID),
                      LOREG(lhsRegID), HIREG(rhsRegID), HIREG(destRegID));
    } else {
        int lowOpcode = ((opcode >> 16) & 0xff) << 20;
        int highOpcode = ((opcode >> 8) & 0xff) << 20;
        CVMBool setcc = (opcode & 0xff);
        CVMCPUemitBinaryALURegister(con, lowOpcode, LOREG(destRegID),
            LOREG(lhsRegID), LOREG(rhsRegID), setcc);
        CVMCPUemitBinaryALURegister(con, highOpcode, HIREG(destRegID),
            HIREG(lhsRegID), HIREG(rhsRegID), CVMJIT_NOSETCC);
    }
}

/* Purpose: Loads a 64-bit integer constant into a register. */
void
CVMCPUemitLoadLongConstant(CVMJITCompilationContext *con, int regID,
                           CVMJavaVal64 *value)
{
    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstant(con, LOREG(regID), (int)val.l);
    CVMCPUemitLoadConstant(con, HIREG(regID), val.l>>32);
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
    CVMRMResource *extra;
    int t;

    CVMassert(opcode == CVMCPU_CMP64_OPCODE);

    /*
     * == and != are easy. All other cases need to be mapped into
     * < or >=, which are also "easy".
     */
    switch (condCode){
    case CVMCPU_COND_EQ:
    case CVMCPU_COND_NE:
        CVMJITcsSetStatusInstruction(con, condCode);
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
				  HIREG(lhsRegID), HIREG(rhsRegID));
        CVMARMemitCompareConditional(con,
	    CVMCPU_CMP_OPCODE, LOREG(lhsRegID),
	    CVMARMalurhsEncodeRegisterToken(con, LOREG(rhsRegID)),
	    CVMCPU_COND_EQ);
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
	/* extra is a dummy target for the sbc instruction */
	extra = CVMRMgetResource(CVMRM_INT_REGS(con),
				 CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
        CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
	    LOREG(lhsRegID), LOREG(rhsRegID));
        CVMJITcsSetDestRegister(con, CVMRMgetRegisterNumber(extra));
        CVMCPUemitBinaryALURegister(con, ARM_SBC_OPCODE,
            CVMRMgetRegisterNumber(extra), HIREG(lhsRegID), HIREG(rhsRegID),
	    CVMJIT_SETCC);
	CVMRMrelinquishResource(CVMRM_INT_REGS(con), extra);
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
			   LOREG(destRegID), srcRegID, CVMJIT_NOSETCC);
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE, HIREG(destRegID),
                              srcRegID, 31);
}

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID, LOREG(srcRegID), CVMJIT_NOSETCC);
}

void
CVMCPUemitMul(
    CVMJITCompilationContext* con,
    int opcode,
    int destreg,
    int lhsreg,
    int rhsreg,
    int extrareg )  /* MLA "accumulate from" reg and UMULL low reg */
{
    CVMRMResource* extraregRes = NULL;

    CVMassert(opcode == CVMCPU_MULL_OPCODE || opcode == CVMCPU_MULH_OPCODE ||
	      opcode == ARM_UMULL_OPCODE || opcode == CVMARM_MLA_OPCODE);


    if (opcode == CVMCPU_MULH_OPCODE) {
	CVMassert(extrareg == CVMCPU_INVALID_REG);
	extraregRes = CVMRMgetResource(CVMRM_INT_REGS(con),
				       CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
	/* extrareg needs to be the low word we want to ignore */
	extrareg = CVMRMgetRegisterNumber(extraregRes);

    }

    if (extrareg == CVMCPU_INVALID_REG) {
	CVMassert(opcode == CVMCPU_MULL_OPCODE);
	extrareg = 0;  /* extrareg must be 0 for mul */
    } else if (opcode == CVMARM_MLA_OPCODE) {
        CVMJITcsPushSourceRegister(con, extrareg);
    } else {
        CVMJITcsSetDestRegister2(con, extrareg);
    }
    CVMJITcsSetDestRegister(con, destreg);
    CVMJITcsPushSourceRegister(con, lhsreg);
    CVMJITcsPushSourceRegister(con, rhsreg);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
		    (CVMUint32)opcode | destreg << 16 | extrareg << 12 |
		    rhsreg << 8 | lhsreg);

    CVMtraceJITCodegenExec({
        char badcode[20];
        const char *opcodename;

        printPC(con);
        switch (opcode){
        case CVMCPU_MULL_OPCODE:
	    CVMconsolePrintf("	mul	%s, %s, %s",
                regNames[destreg], regNames[lhsreg], regNames[rhsreg]);
	    goto doneTracingMul;
        case CVMARM_MLA_OPCODE:
	    CVMconsolePrintf("	mla	%s, %s, %s, %s",
			 regNames[destreg], regNames[lhsreg],
			 regNames[rhsreg], regNames[extrareg]);
	    goto doneTracingMul;
        case CVMCPU_MULH_OPCODE:
	    opcodename = "smull";
	    break;
        case ARM_UMULL_OPCODE:
	    opcodename = "umull";
	    break;
        default:
            sprintf(badcode, "Unknown(%08x)", opcode);
            opcodename = badcode;
            break;
        }
	CVMconsolePrintf("	%s	%s, %s, %s, %s",
			 opcodename, regNames[extrareg], regNames[destreg],
			 regNames[lhsreg], regNames[rhsreg]);
doneTracingMul: (void)0;
    });

    CVMJITdumpCodegenComments(con);

    if (extraregRes != NULL) {
	CVMRMrelinquishResource(CVMRM_INT_REGS(con), extraregRes);
    }
}

void
CVMCPUemitMoveConditional(CVMJITCompilationContext* con, int opcode,
                          int destRegID, CVMCPUALURhsToken srcToken,
                          CVMBool setcc, CVMCPUCondCode condCode)
{

    CVMassert(opcode == CVMCPU_MOV_OPCODE || opcode == ARM_MVN_OPCODE ||
              opcode == CVMCPU_FMOV_OPCODE || opcode == CVMCPU_DMOV_OPCODE);

#ifdef CVM_JIT_USE_FP_HARDWARE
    if (opcode == CVMCPU_FMOV_OPCODE || opcode == CVMCPU_DMOV_OPCODE) {
        emitInstruction(con, ARM_MAKE_CONDCODE_BITS(condCode) |
                    (CVMUint32)opcode |
	            (destRegID & 0x01) << 22 | (destRegID >> 1) << 12 |
                    (srcToken  & 0x01) << 5  | (srcToken  >> 1));

        CVMtraceJITCodegenExec({
            char mode1buf[48];
            formatMode1(mode1buf, srcToken);
            printPC(con);
            CVMconsolePrintf("	%s%s	%%f%d, %%f%d",
                         getOpcodeName(opcode), conditions[condCode],
                         destRegID, srcToken);
        });
    } else
#endif
    {
        CVMJITcsSetCompareInstruction(con, setcc);
        CVMJITcsSetStatusInstruction(con, condCode);
        CVMJITcsSetDestRegister(con, destRegID);

        emitInstruction(con, ARM_MAKE_CONDCODE_BITS(condCode) |
                    (setcc ? (1 << 20) : 0) |
                    (CVMUint32)opcode | destRegID << 12 | srcToken);

        CVMtraceJITCodegenExec({
            char mode1buf[48];
            formatMode1(mode1buf, srcToken);
            printPC(con);
            CVMconsolePrintf("	%s%s%s	%s, %s",
                         getOpcodeName(opcode), conditions[condCode],
                         setcc?"s":"", regNames[destRegID], mode1buf);
        });
    }
    CVMJITdumpCodegenComments(con);
}

static void
CVMARMemitCompareConditional(CVMJITCompilationContext* con, int opcode,
                             int lhsRegID, CVMCPUALURhsToken rhsToken,
                             CVMCPUCondCode condCode)
{
    CVMassert(opcode == CVMCPU_CMP_OPCODE || opcode == CVMCPU_CMN_OPCODE);

    CVMJITcsPushSourceRegister(con, lhsRegID);
    CVMJITcsSetCompareInstruction(con, CVM_TRUE);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(condCode) |
                    (CVMUint32)opcode | lhsRegID << 16 | rhsToken);

    CVMtraceJITCodegenExec({
        char mode1buf[48];
        formatMode1(mode1buf, rhsToken);
        printPC(con);
        CVMconsolePrintf("	%s%s	%s, %s", getOpcodeName(opcode),
            conditions[condCode], regNames[lhsRegID], mode1buf);
    });
    CVMJITdumpCodegenComments(con);
}

extern void
CVMCPUemitCompare(CVMJITCompilationContext* con,
		  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken)
{
    CVMARMemitCompareConditional(con, opcode, lhsRegID, rhsToken,
				 CVMCPU_COND_AL);
}

extern void
CVMCPUemitCompareConstant(CVMJITCompilationContext* con,
			  int opcode, CVMCPUCondCode condCode,
			  int lhsRegID, CVMInt32 rhsConstValue)
{
    CVMCPUALURhsToken rhsToken;
    CVMRMResource* rhsRes;

    /* rhsConstValue may not be encodable as an immediate value */
    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsRes = NULL;
        rhsToken = CVMARMalurhsEncodeConstantToken(con, rhsConstValue);
    } else {
        rhsRes = CVMRMgetResourceForConstant32(CVMRM_INT_REGS(con),
			CVMRM_ANY_SET, CVMRM_EMPTY_SET, rhsConstValue);
        rhsToken =  CVMARMalurhsEncodeRegisterToken(con,
		        CVMRMgetRegisterNumber(rhsRes));
    }
    CVMARMemitCompareConditional(con, opcode, lhsRegID, rhsToken,
				 CVMCPU_COND_AL);
    if (rhsRes != NULL) {
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), rhsRes);
    }
}

/*
 * Putting a big, non-immediate value into a register
 * using the constant pool.
 */
void
CVMCPUemitLoadConstant(
    CVMJITCompilationContext* con,
    int regno,
    CVMInt32 v)
{
    CVMCPUemitLoadConstantConditional(con, regno, v, CVMCPU_COND_AL);
}

void
CVMCPUemitLoadConstantConditional(
    CVMJITCompilationContext* con,
    int regno,
    CVMInt32 v,
    CVMCPUCondCode condCode)
{
    CVMUint32 base;
    CVMUint32 rotate;

#ifdef CVM_JIT_USE_FP_HARDWARE
    CVMRMResource *res = CVMRMfindResourceConstant32InRegister(CVMRM_FP_REGS(con), v);
    if (res != NULL) {
        CVMARMemitMoveFloatFP(con, CVMARM_MOVAF_OPCODE,
            CVMRMgetRegisterNumberUnpinned(res),
            regno);
    } else
#endif
    /* Check to see if we can do this as a positive constant: */
    if (CVMARMmode1EncodeImmediate(v, &base, &rotate)) {
        CVMCPUemitMoveConditional(con, CVMCPU_MOV_OPCODE, regno,
            CVMARMalurhsEncodeLargeConstantToken(base, rotate),
            CVMJIT_NOSETCC, condCode);

    /* Check to see if we can do this as a negative constant: */
    } else if (CVMARMmode1EncodeImmediate(~v, &base, &rotate)) {
        CVMCPUemitMoveConditional(con, ARM_MVN_OPCODE, regno,
            CVMARMalurhsEncodeLargeConstantToken(base, rotate),
            CVMJIT_NOSETCC, condCode);

    /* Do a big constant stored at a PC relative location: */
    } else {
        CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
        CVMInt32 targetLiteralOffset = 0;
        CVMInt32 relativeOffset = 0;

        int baseReg;
	/*
	 * Normally we call CVMJITgetRuntimeConstantReference32 first and
	 * then emit the instruction to load the constant afterwards.
	 * When code scheduling is enabled, the instruction to load the
	 * constant might be emitted somewhere other than the current
	 * logicalPC, so when code scheduling is enabled we need to wait
	 * until after the load instruction is emitted before calling
	 * CVMJITgetRuntimeConstantReference32.
	 */
#ifndef IAI_CODE_SCHEDULER_SCORE_BOARD
        targetLiteralOffset =
	    CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

#ifdef CVMCPU_HAS_CP_REG
	baseReg = CVMCPU_CP_REG;
	/* offset of 0 will be patched after the constant pool is dumped */
	relativeOffset = 0;
#else
	baseReg = CVMARM_PC;
        logicalPC += 8; /* +8 ARM PC bias */
        relativeOffset = (targetLiteralOffset != 0) ?
                         targetLiteralOffset - logicalPC : 0;
#endif

	/* Emit the load relative to the constant pool base register. */
        CVMCPUemitMemoryReferenceConditional(con, CVMCPU_LDR32_OPCODE, regno,
            baseReg, CVMCPUmemspecEncodeImmediateToken(con, relativeOffset),
            condCode);

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
	/*
	 * Find out where the load instruction was emitted and pass this to
	 * CVMJITgetRuntimeConstantReference32. If the constant was
	 * already emitted, then we need to patch up the load
	 * instruction now.
	 */
        logicalPC = CVMJITcbufGetLogicalInstructionPC(con);
        targetLiteralOffset =
	    CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
        targetLiteralOffset = (targetLiteralOffset != 0) ?
	    targetLiteralOffset : logicalPC;
	/* Emit the load relative to the constant pool base register. */
        CVMJITfixupAddress(con, logicalPC, targetLiteralOffset,
			   CVMJIT_MEMSPEC_ADDRESS_MODE);
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */
    }
    CVMJITresetSymbolName(con);
}

static int absolute(int v){
    return (v<0)?-v:v;
}

#ifdef CVM_JIT_USE_FP_HARDWARE
/* This function can dump constant pool and so to update logical address
  in the context */
static CVMInt32
CVMARMgetRuntimeConstantReferenceFP(
    CVMJITCompilationContext* con,
    CVMInt32 v)
{

    if (con->constantPoolSize >= CVMARM_FP_MAX_LOADSTORE_OFFSET) {
    	CVMRISCemitConstantPoolDumpWithBranchAround(con);
    }
    return CVMJITgetRuntimeFPConstantReference32(con,
        CVMJITcbufGetLogicalPC(con), v);
}

void
CVMCPUemitLoadConstantConditionalFP(
    CVMJITCompilationContext* con,
    int regno,
    CVMInt32 v,
    CVMCPUCondCode condCode)
{
    CVMInt32 targetLiteralOffset;
    CVMInt32 logicalPC;
    int baseReg;
    CVMInt32 relativeOffset;
    CVMRMResource *res =
        CVMRMfindResourceConstant32InRegister(CVMRM_FP_REGS(con), v);
    if (res != NULL) {
	if (regno != CVMRMgetRegisterNumberUnpinned(res)) {
            CVMCPUemitMoveConditional(con, CVMCPU_FMOV_OPCODE, regno,
                CVMRMgetRegisterNumberUnpinned(res), CVMJIT_NOSETCC, condCode);
	}
        CVMJITresetSymbolName(con);
	return;
    }
    res = CVMRMfindResourceConstant32InRegister(CVMRM_INT_REGS(con), v);
    if (res != NULL) {
        CVMARMemitMoveFloatFP(con, CVMARM_MOVFA_OPCODE, regno,
            CVMRMgetRegisterNumberUnpinned(res));
        CVMJITresetSymbolName(con);
	return;
    }

    if (v == 0) {
	res = CVMRMfindResourceForNonNaNConstant(CVMRM_FP_REGS(con));
	if (res != NULL) {
	    CVMCPUemitBinaryFP(con, CVMCPU_FSUB_OPCODE, regno,
                CVMRMgetRegisterNumberUnpinned(res),
                CVMRMgetRegisterNumberUnpinned(res));
            CVMJITresetSymbolName(con);
	    return;
	}
	res = CVMRMfindResourceForNonNaNConstant(CVMRM_INT_REGS(con));
	if (res != NULL) {
            CVMARMemitMoveFloatFP(con, CVMARM_MOVFA_OPCODE, regno,
                CVMRMgetRegisterNumberUnpinned(res));
            CVMCPUemitBinaryFP(con, CVMCPU_FSUB_OPCODE, regno, regno, regno);
            CVMJITresetSymbolName(con);
	    return;
	}
        {
            /* Load 0 into ARM register */
            CVMRMResource *tempReg = CVMRMgetResource(CVMRM_INT_REGS(con),
                                       CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
            CVMInt32 tempRegID = CVMRMgetRegisterNumber(tempReg);
            CVMCPUemitLoadConstant(con, tempRegID, 0);
            CVMARMemitMoveFloatFP(con, CVMARM_MOVFA_OPCODE, regno, tempRegID);
            CVMRMrelinquishResource(CVMRM_INT_REGS(con), tempReg);
            CVMJITresetSymbolName(con);
            return;
        }
    }
    targetLiteralOffset =
        CVMARMgetRuntimeConstantReferenceFP(con, v);

    logicalPC = CVMJITcbufGetLogicalPC(con);

#ifdef CVMCPU_HAS_CP_REG
    baseReg = CVMCPU_CP_REG;
    /* offset of 0 will be patched after the constant pool is dumped */
    relativeOffset = 0;
#else
    baseReg = CVMARM_PC;
    relativeOffset =
        (targetLiteralOffset != 0) ? targetLiteralOffset - logicalPC - 8
                                   : 0;
#endif

    /* Emit the load relative to the constant pool base register. */
    CVMCPUemitMemoryReferenceConditional(con, CVMCPU_FLDR32_OPCODE, regno,
	baseReg, CVMCPUmemspecEncodeImmediateToken(con, relativeOffset),
	condCode);

    CVMJITresetSymbolName(con);
}
#endif

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. For the former, it will choose the quicker bl
 * instruction. For the later, it will use the 2 instruction <mov, ldr>
 * pair to setup the lr and load the target address into the pc, plus
 * generate a constant for the target address.
 *
 * condCode is the condition that must be met to do the call.
 *
 * The constant pool will be dumped if necessary and if allowed.
 * If okToBranchAroundCpDump is FALSE and the lr can't be setup
 * use mode1 math to skip over the constant pool, then it will not
 * be dumped.
 *
 * WARNING: pass FALSE for okToBranchAroundCpDump if you are going
 * to capture a stackmap right after calling CVMCPUemitAbsoluteCall().
 * Otherwise the pc flushed to the frame will not be the pc where the
 * stackmap was captured.
 */

static void
emitReturnAddressComputation(CVMJITCompilationContext* con,
			     CVMCPUCondCode condCode, int skip)
{
    CVMassert(skip >= 0);
    CVMJITcsSetEmitInPlace(con);
    CVMJITcsSetDestRegister(con, CVMARM_LR);
    CVMJITcsPushSourceRegister(con, CVMARM_PC);
    if (skip == 0) {
	emitInstruction(con, ARM_MAKE_CONDCODE_BITS(condCode) |
            (CVMUint32)CVMCPU_MOV_OPCODE | CVMARM_LR << 12 | CVMARM_PC);
    } else {
        CVMUint32 base, rotate;
        CVMBool success;
	CVMassert(skip > 0);
        /* Make sure skip is reachable via mode1 addressing: */
        success = CVMARMmode1EncodeImmediate(skip, &base, &rotate);
        CVMassert(success);
	emitInstruction(con, ARM_MAKE_CONDCODE_BITS(condCode) |
                    (CVMUint32)CVMCPU_ADD_OPCODE | CVMARM_PC << 16 |
                    CVMARM_LR << 12 | CVMARM_MODE1_CONSTANT |
                    rotate << 8 | base);
    }

    CVMtraceJITCodegenExec({
        printPC(con);
        if (skip == 0){
            CVMJITaddCodegenComment((con, "lr = pc"));
            CVMconsolePrintf("	mov%s	%s, %s", conditions[condCode],
                             regNames[CVMARM_LR], regNames[CVMARM_PC]);
        } else {
            CVMJITaddCodegenComment((con, "lr = pc + offset"));
            CVMconsolePrintf("	add%s	%s, %s, %d",
                             conditions[condCode],
                             regNames[CVMARM_LR], regNames[CVMARM_PC], skip);
        }
    });
    CVMJITcsClearEmitInPlace(con);
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall(CVMJITCompilationContext* con,
                                             const void* target,
                                             CVMBool okToDumpCp,
                                             CVMBool okToBranchAroundCpDump)
{
    CVMCodegenComment *comment;
    CVMJITpopCodegenComment(con, comment);
    /* Store the JSP into the compiled frame: */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
        CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetof(CVMFrame, topOfStack));
    CVMJITpushCodegenComment(con, comment);
    CVMARMemitAbsoluteCallConditional(con, target, okToDumpCp,
                                      okToBranchAroundCpDump, CVMCPU_COND_AL,
                                      CVM_TRUE);
    CVMJITcsBeginBlock(con);
}

void
CVMARMemitAbsoluteCallConditional(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump,
		       CVMCPUCondCode condCode,
                       CVMBool flushReturnPCToFrame)
{
    CVMCodegenComment *comment;
    CVMBool doCpDump = okToDumpCp && CVMJITcpoolNeedDump(con);
    int cpSkip = 0;
    CVMBool cpSkipIsMode1 = CVM_FALSE;
    int minSkip = 0;

    if (flushReturnPCToFrame) {
        minSkip = 4;
    }

    /*
     * Find out if we can use a mode1 instruction to adjust the lr to
     * skip over the constant pool.
     */
    if (doCpDump) {
        cpSkip = (con->numEntriesToEmit * 4) + minSkip;
        if (condCode != CVMCPU_COND_AL) {
	    cpSkip += 4; /* need to skip around absolute branch */
	}
	cpSkipIsMode1 = CVMARMmode1EncodeImmediate(cpSkip, NULL, NULL);

	/*
	 * If the call is conditional or the skip value is not mode1, then
	 * we will have to branch around the the constant pool. If the
	 * branch is not allowed, then we can't dump.
	 */
        if (condCode != CVMCPU_COND_AL || !cpSkipIsMode1) {
	    doCpDump = okToBranchAroundCpDump;
	    if (!doCpDump) {
                cpSkip = minSkip;
	    }
	}
    } else {
        cpSkip = minSkip;
    }

    /*
        The emitted code should look like one of the following cases:
        CASE 1: No constants to dump.

                add lr, pc, #4
                str lr, [JFP, #offsetof(CVMCompiledFrame, pcX)]
                ldr pc, #target  @ target not in code cache
            return_address:

            or
                mov lr, pc
                ldr pc, #target  @ target not in code cache
            return_address:

	    or

	        bl  target       @ target is in code cache
            return_address:

        CASE 2: Number of constants to dump is small, and the lr register can
                be adjusted using an ARM add instruction.

                add lr, pc + (offset to return_address)
                str lr, [JFP, #offsetof(CVMCompiledFrame, pcX)]
                ldr pc, #target  @ or bl if target is in code cache
                .word #constant1
                .word #constant2
                ...
            return_address:

            or

                add lr, pc + (offset to return_address)
                ldr pc, #target  @ or bl if target is in code cache
                .word #constant1
                .word #constant2
                ...
            return_address:

        CASE 3: Number of constants to dump is large.

                add lr, pc, #4
                str lr, [JFP, #offsetof(CVMCompiledFrame, pcX)]
                ldr pc, #target  @ or bl if target is in code cache
                b return_address
                .word #constant1
                .word #constant2
                ...
            return_address:

            or

                mov lr, pc
                ldr pc, #target  @ or bl if target is in code cache
                b return_address
                .word #constant1
                .word #constant2
                ...
            return_address:

	Case 3 is not allowed if okToBranchAroundCpDump is not TRUE.
        If the condCode is not CVMCPU_COND_AL, then all of the above
        instructions are conditional, and case 2 will include an unconditional
	branch around the constant pool. This is why we add 4 to
	cpSkip in this case.
    */

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    /*
     * If the target is one we've copied into the code cache, then use
     * the quicker branch to the codecache copy of the target, rather
     * than an ldr into the pc.
     */
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
	target < (void*)&CVMCCMcodeCacheCopyEnd) {
	int helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
	if ((doCpDump && cpSkipIsMode1) || flushReturnPCToFrame) {
	    /* If we have to dump the constant pool, then we need to setup
	     * the lr so it will skip over the constant pool.
	     */
	    CVMJITpopCodegenComment(con, comment);
	    emitReturnAddressComputation(con, condCode, cpSkip);
            if (flushReturnPCToFrame) {
                /* Store the return PC into the compiled frame: */
                int offset = offsetof(CVMCompiledFrame, pcX);
                CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                    CVMARM_lr, CVMCPU_JFP_REG, offset);
            }
	    CVMJITpushCodegenComment(con, comment);
            CVMCPUemitBranch(con, helperOffset, condCode);
	} else {
	    /* emit conditional branch link */
            CVMARMemitBranch(con, helperOffset, condCode, CVM_TRUE,
                             NULL /* no fixupList */);
	}
    } else
#endif
    {
	CVMInt32 fixupPC = CVMJITcbufGetLogicalPC(con);
	/* Compute return address */
	CVMJITpopCodegenComment(con, comment);
	CVMJITcsSetEmitInPlace(con);
	emitReturnAddressComputation(con, condCode,
                                     cpSkipIsMode1 ? cpSkip: minSkip);
	CVMJITcsClearEmitInPlace(con);
        if (flushReturnPCToFrame) {
            /* Store the return PC into the compiled frame: */
            CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                CVMARM_lr, CVMCPU_JFP_REG, offsetof(CVMCompiledFrame, pcX));
        }
	CVMJITpushCodegenComment(con, comment);
	/* load the new pc from memory */
	CVMJITcsSetEmitInPlace(con);
        CVMCPUemitLoadConstantConditional(con, CVMARM_PC, (CVMInt32)target,
                                          condCode);
	CVMJITcsClearEmitInPlace(con);
	/*
	 * loading the target constant may have added a new cp entry,
	 * so we better fix up the return address in case it has.
	 */
	if (doCpDump) {
	    int oldCpSkip = cpSkip;
            cpSkip = (con->numEntriesToEmit * 4) + minSkip;
            if (condCode != CVMCPU_COND_AL) {
		cpSkip += 4; /* need to skip around absolute branch */
	    }
	    if (cpSkip != oldCpSkip) {
		CVMJITcbufPushFixup(con, fixupPC);
		CVMJITcsSetEmitInPlace(con);
		cpSkipIsMode1 =
		    CVMARMmode1EncodeImmediate(cpSkip, NULL, NULL);
		if (cpSkipIsMode1) {
		    /* If the new cpSkip is still encodable as a mode 1
		       constant, then re-emit the return address
		       computation: */
		    emitReturnAddressComputation(con, condCode, cpSkip);
		} else {
		    /* Else, we need to do the branch around in order to dump
		       constants.  If we can't branch around, then don't do
		       the constant dump: */
                    emitReturnAddressComputation(con, condCode, minSkip);
		    if (!okToBranchAroundCpDump) {
		        doCpDump = CVM_FALSE;
		    }
		}
		CVMJITcsClearEmitInPlace(con);
		CVMJITcbufPop(con);
	    }
	}
    }
    CVMJITresetSymbolName(con);

    /* Now dump constant pool if necessary, and patch the return address */
    if (doCpDump) {
        CVMassert(condCode != CVMCPU_COND_AL ?
                  okToBranchAroundCpDump : CVM_TRUE);

	/*
	 * If the call is conditional or we can't encode the skip value
	 * using mode1, then we need to unconditionally jump over the
	 * constant pool. In the case of the conditional call, we do this
	 * in case the call isn't taken.
	 */
        if (!cpSkipIsMode1 || condCode != CVMCPU_COND_AL) {
	    CVMassert(okToBranchAroundCpDump);
	    /* Emit goto over the constant pool */
	    CVMJITaddCodegenComment((con, "Skip over dumped constant pool"));
            CVMCPUemitBranch(con,
			     CVMJITcbufGetLogicalPC(con) +
			     con->numEntriesToEmit * 4 + 4,
			     CVMCPU_COND_AL);
	}
	CVMJITdumpRuntimeConstantPool(con, CVM_TRUE);
    }
}

/*
 * Return using the correct CCM helper function.
 */
void
CVMCPUemitReturn(CVMJITCompilationContext* con, void* helper)
{
    CVMUint32 prevOffset;
    CVMCodegenComment *comment;

    /*
     * First set up
     *
     *   ldr     PREV, [JFP,#OFFSET_CVMFrame_prevX]
     *
     * This is expected at the return handler
     *
     */
    CVMJITpopCodegenComment(con, comment);
    CVMJITaddCodegenComment((con,
	"PREV(%s) = frame.prevX for return helper to use",
	regNames[CVMCPU_PROLOGUE_PREVFRAME_REG]));

    prevOffset = CVMoffsetof(CVMCompiledFrame, frameX.prevX);
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
				       CVMCPU_PROLOGUE_PREVFRAME_REG,
				       CVMCPU_JFP_REG,
				       prevOffset);
    CVMJITpushCodegenComment(con, comment);

    /* branch to the helper code to do the return */
    CVMCPUemitAbsoluteCall(con, helper, CVMJIT_CPDUMPOK, CVMJIT_NOCPBRANCH);
    CVMJITcsBeginBlock(con);
}

static void
CVMARMemitMemoryReferenceConditional(CVMJITCompilationContext* con,
    int opcode,
    int destreg,
    int basereg,
    CVMCPUMemSpecToken memSpecToken,
    CVMCPUCondCode condCode);

/* Purpose: Emits instructions to do a conditional load/store operation. */
void
CVMCPUemitMemoryReferenceConditional(CVMJITCompilationContext* con,
    int opcode,
    int destreg,
    int basereg,
    CVMCPUMemSpecToken memSpecToken,
    CVMCPUCondCode condCode)
{
    int offset = CVMARMmemspecGetTokenOffset(memSpecToken);

    /* Check for the 64 bit case: */
    if (opcode != CVMCPU_FSTR64_OPCODE &&
        opcode != CVMCPU_FLDR64_OPCODE &&
        opcode > CVMCPU_STR64_OPCODE) {
        /* Do 32-bit and less case: */
        CVMARMemitMemoryReferenceConditional(con, opcode, destreg, basereg,
            memSpecToken, condCode);
    } else {

        CVMJITcsSetEmitInPlace(con);
        /* Do 64-bit case: */
#ifdef CVM_JIT_USE_FP_HARDWARE
        if (CVMARMisMode5Instruction(opcode)) {
            opcode = (opcode == CVMCPU_FSTR64_OPCODE) ?
                CVMCPU_FSTR64_OPCODE : CVMCPU_FLDR64_OPCODE;
        } else
#endif
        {
        opcode = (opcode == CVMCPU_STR64_OPCODE) ?
                 CVMCPU_STR32_OPCODE : CVMCPU_LDR32_OPCODE;
        }

        switch (CVMARMmemspecGetTokenType(memSpecToken)) {
        case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
#ifdef CVM_JIT_USE_FP_HARDWARE
            if (CVMARMisMode5Instruction(opcode)) {
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg, CVMCPUmemspecEncodeImmediateToken(con, offset),
                    condCode);
	    } else
#endif
            {
                /* Load/store 1st word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg, CVMCPUmemspecEncodeImmediateToken(con, offset),
                    condCode);

                /* Load/store 2nd word: */
                if (offset > 0xFFF-4) {
                    goto useNewBaseReg;
                }
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg+1,
                    basereg, CVMCPUmemspecEncodeImmediateToken(con, offset+4),
                    condCode);
            }
            break;

        case CVMCPU_MEMSPEC_REG_OFFSET:
#ifdef CVM_JIT_USE_FP_HARDWARE
            if (CVMARMisMode5Instruction(opcode)) {
                /* Addressing mode 5 doesn't support register offset */
                CVMassert(CVM_FALSE);
	    } else
#endif
            {
                /* Load/store 1st word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                basereg, memSpecToken, condCode);

                /* There's no easy way to inc the offset by 4.
                   So instead, we make a new base register.
                   We can't just simply inc the original base
                   register because someone else may need it later: */
                useNewBaseReg:
                /* Load/store 2nd word: */
                {
                    CVMRMResource *newbase;
                    CVMInt32 newbasereg;

                    newbase = CVMRMgetResource(CVMRM_INT_REGS(con),
				CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
                    newbasereg = CVMRMgetRegisterNumber(newbase);
                    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                        newbasereg, basereg, 4, CVMJIT_NOSETCC);
                    CVMARMemitMemoryReferenceConditional(con, opcode,
                        destreg+1, newbasereg, memSpecToken, condCode);
                    CVMRMrelinquishResource(CVMRM_INT_REGS(con), newbase);
                }
	    }
            break;

        case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
#ifdef CVM_JIT_USE_FP_HARDWARE
            if (CVMARMisMode5Instruction(opcode)) {
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg,
                    CVMCPUmemspecEncodePostIncrementImmediateToken(con,
                        offset), condCode);
	    } else
#endif
            {
                /* Load/store 1st word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg,
                    CVMCPUmemspecEncodePostIncrementImmediateToken(con, 4),
                    condCode);

                /* Load/store 2nd word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg+1,
                    basereg,
                    CVMCPUmemspecEncodePostIncrementImmediateToken(con,
                    (offset-4)),
                    condCode);
            }
            break;

        case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
#ifdef CVM_JIT_USE_FP_HARDWARE
            if (CVMARMisMode5Instruction(opcode)) {
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg,
                    CVMCPUmemspecEncodePreDecrementImmediateToken(con, offset),
                    condCode);
	    } else
#endif
            {
                /* Load/store 2nd word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg+1,
                    basereg,
                    CVMCPUmemspecEncodePreDecrementImmediateToken(con,
                    (offset-4)),
                    condCode);

                /* Load/store 1st word: */
                CVMARMemitMemoryReferenceConditional(con, opcode, destreg,
                    basereg,
                    CVMCPUmemspecEncodePreDecrementImmediateToken(con, 4),
                    condCode);
            }
            break;

        default:
            CVMassert(CVM_FALSE);
        }
        CVMJITcsClearEmitInPlace(con);
    }
}

/* Purpose: Emits instructions to do a conditional load/store operation. */
void
CVMCPUemitMemoryReferenceImmediateConditional(CVMJITCompilationContext* con,
    int opcode, int destRegID, int baseRegID, CVMInt32 immOffset,
    CVMCPUCondCode condCode)
{
    int offsetLimit;
    CVMInt32 offset;


#ifdef CVM_JIT_USE_FP_HARDWARE
    if (CVMARMisMode5Instruction(opcode)) {
	/* Addressing Mode 5 offsets are multiplied by 4 */
	offsetLimit = 0x100 << 2;
    } else
#endif
    {
    offsetLimit =
        (opcode <= CVMCPU_STR64_OPCODE || CVMARMisMode2Instruction(opcode)) ?
            /* 64 bit and Mode 2 case: */ 0x1000 :
            /* Mode 3 case:            */ 0x100;
    }

    offset = (immOffset >= 0) ? immOffset : -immOffset;
    if (offset < offsetLimit) {
        CVMCPUemitMemoryReferenceConditional(con, opcode, destRegID, baseRegID,
            CVMCPUmemspecEncodeImmediateToken(con, immOffset), condCode);
    } else {
        CVMRMResource *offsetRes;
        offsetRes = CVMRMgetResourceForConstant32(CVMRM_INT_REGS(con),
				CVMRM_ANY_SET, CVMRM_EMPTY_SET, immOffset);
        CVMJITcsPushSourceRegister(con, CVMRMgetRegisterNumber(offsetRes));
        CVMCPUemitMemoryReferenceConditional(con,
            opcode, destRegID, baseRegID,
            CVMARMmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET,
            CVMRMgetRegisterNumber(offsetRes), CVMARM_NOSHIFT_OPCODE, 0),
            condCode);
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), offsetRes);
    }
}

static void
CVMARMemitMemoryReferenceConditional(CVMJITCompilationContext* con,
    int opcode,
    int destreg,
    int basereg,
    CVMCPUMemSpecToken memSpecToken,
    CVMCPUCondCode condCode)
{
    CVMUint32 instruction;

    static const int typeToAddressMode[] = {
            ARM_MEMSPEC_IMMEDIATE_OFFSET,
            ARM_MEMSPEC_REG_OFFSET,
            ARM_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET,
            ARM_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET
        };

    int type = CVMARMmemspecGetTokenType(memSpecToken);
    int offset = CVMARMmemspecGetTokenOffset(memSpecToken);
    int shiftOp = CVMARMmemspecGetTokenShiftOpcode(memSpecToken);
    int shifterImm = CVMARMmemspecGetTokenShiftAmount(memSpecToken);
    int loadStoreAddressingMode = typeToAddressMode[type];

    CVMJITcsSetBaseRegister(con, basereg);
    CVMJITcsPushSourceRegister(con, basereg);
    CVMJITcsSetDestRegister(con, destreg);
    CVMJITcsSetStatusInstruction(con, condCode);

    /* Make sure we are not emitting 64 bit load/stores here: */
    CVMassert(opcode > CVMCPU_STR64_OPCODE);

    if (offset < 0) {
        offset = -offset;
        /* turn off U bit */
        loadStoreAddressingMode &= ~ARM_LOADSTORE_ADDOFFSET;
    }

    /*
     * Allow either
     *   Rd, [Rn, #+/-<12-bit offset]
     * or
     *   Rd, [Rn, +/-Rm, <shift> #<shifterImm>]
     */
#ifdef CVM_DEBUG_ASSERTS
    if (loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET) {
	CVMassert(shifterImm <= 31);
	CVMassert(offset <= 15); /* offset is a base register in this case */
    } else {
	CVMassert((loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET)
		  == 0);
	CVMassert(shifterImm == 0);
	CVMassert(shiftOp == 0);
#ifdef CVM_JIT_USE_FP_HARDWARE
        if (CVMARMisMode5Instruction(opcode)) {
	    /* Do nothing we check for big offsets
	     * while generating the instruction.
	     */
        } else
#endif

        if (CVMARMisMode2Instruction(opcode)) {
	    CVMassert(offset <= 4095);
	} else {
	    CVMassert(offset <= 255);
	}
    }
#endif

#ifdef CVM_JIT_USE_FP_HARDWARE
    if (CVMARMisMode5Instruction(opcode)) {
	switch(type) {
	    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET: {
                int hiOffset = offset & ~0x03FF;
		if (hiOffset) {
                    CVMRMResource *tempReg =
                        CVMRMgetResource(CVMRM_INT_REGS(con),
                        CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
                    CVMInt32 tempRegID = CVMRMgetRegisterNumber(tempReg);

                    int op =
                        loadStoreAddressingMode & ARM_LOADSTORE_ADDOFFSET ?
                        CVMCPU_ADD_OPCODE : CVMCPU_SUB_OPCODE;
                    CVMCPUemitBinaryALUConstant(con, op, tempRegID,
                        basereg, hiOffset, CVMJIT_NOSETCC);
                    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tempReg);

		    offset -= hiOffset;
		    basereg = tempRegID;
		}
		break;
	    }
	    case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET: {
                /* If we are in post increment mode write back
                 * mode is not supported for fsts and fstd.
                 * To get around this we emit fstms or fstmd
                 * and set the writeback bit.
                 */
                CVMassert(offset == 4 || offset == 8);
                loadStoreAddressingMode =
                    ARM_LOADSTORE_WRITEBACK | ARM_LOADSTORE_ADDOFFSET;
                opcode &= ~ARM_LOADSTORE_PREINDEX;
                break;
	    }
	    case  CVMCPU_MEMSPEC_REG_OFFSET: {
	        /* Addressing Mode 5 doesn't support register offset.
	         * If we are here we need to convert the register offset
	         * to an address first.
	         */
                CVMRMResource *tempReg = CVMRMgetResource(CVMRM_INT_REGS(con),
                               CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
                CVMInt32 tempRegID = CVMRMgetRegisterNumber(tempReg);

                int op =
                    loadStoreAddressingMode & ARM_LOADSTORE_ADDOFFSET ?
                    CVMCPU_ADD_OPCODE : CVMCPU_SUB_OPCODE;

	        CVMCPUemitBinaryALURegister(con, op, tempRegID, basereg,
	            offset, CVMJIT_NOSETCC);

                offset = 0;
                basereg = tempRegID;
                loadStoreAddressingMode &= ~ARM_LOADSTORE_REGISTER_OFFSET;
                CVMRMrelinquishResource(CVMRM_INT_REGS(con), tempReg);
                break;
	    }
	}
        instruction = ARM_MAKE_CONDCODE_BITS(condCode) | (CVMUint32)opcode |
                      loadStoreAddressingMode | basereg << 16 |
                      (destreg & 0x01) << 22 | (destreg >> 1) << 12 |
                      shiftOp | shifterImm << 7;
    } else
#endif
    {
    instruction = ARM_MAKE_CONDCODE_BITS(condCode) | (CVMUint32)opcode |
                  loadStoreAddressingMode | basereg << 16 | destreg << 12 |
                  shiftOp | shifterImm << 7;
    }

    /* NOTE: It appears that we will never supposed to get an offset that
        can be out of range.

        The constantpool manager takes care of making sure that referenced
        constants are within range, otherwise a duplicate constant will be
        emitted to resolve the range problem.

        The stack manager uses this emitter for accessing arguments on the
        stack.  With a max of 256 arguments per method, that makes for a 2KB
        (256 * 8 bytes) range which is well within the 4K mode 2 limit.

        Branches within a method uses Branch and Link which has a 32M range
        and does not concern this emitter.

        Branches to far functions like CCM helpers uses this emitter but are
        done as loading of constants manager by the constantpool manager.

        Access of static fields do not use an immediate offset.

        Access of instance fields and method pointers in the vtbl are done
        through the use of offsets which are encapsulated in the CVMCPUMemSpec
        abstraction. This abstraction takes care of making sure that offsets
        are encodable as immediates.  If not, it will first load it into a
        register and use a register offset instead.  Hence, there is no issue
        with the offsets being out of range.
    */

    /* mode 3 and mode 5 stores the offset values in a different
     * order than mode 2
     */
    if (CVMARMisMode2Instruction(opcode)) {
	instruction |= (offset & 0xFFF);
    }
#ifdef CVM_JIT_USE_FP_HARDWARE
      else if (CVMARMisMode5Instruction(opcode)) {
	    /* mode 5 offset are automatically multiplied by 4
	     * we compenstate for this by dividing by 4 first
	     */
	    CVMassert((offset % 4) == 0);

	    offset >>= 2;

	    CVMassert(offset <= 255);

            instruction |= (offset & 0xff);
            /* multiply offset by 4 again so trace codegen
             * will print the correct offsets
             */
            offset = offset << 2;
    }
#endif
      else {
	instruction |= (offset & 0x0f);
	instruction |= (offset & 0xf0) << 4;

        /* Make sure we're not emitting a MODE3 register offset instruction.
           Currently, this is not done.  But if this becomes needed in the
           future, we will have to replace this assertion with the piece of
           code below: */
        #ifndef ARM_MODE3_REG_OFFSET_NEEDED
        CVMassert((loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET)
                  == 0);
        #else
        /* NOTE: The ARM_LOADSTORE_MODE3_IMMEDIATE_OFFSET is set by default in
                 the opcode. */
        if ((loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET) != 0) {
            /* Clear the MODE2 register offset bit, and clear the MODE3
               immediate offset bit clear: */
            instruction &= ~(ARM_LOADSTORE_REGISTER_OFFSET |
                             ARM_LOADSTORE_MODE3_IMMEDIATE_OFFSET);
        }
        #endif /* ARM_MODE3_REG_OFFSET_NEEDED */
    }

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    switch(opcode) {
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_LDR16U_OPCODE:
        case CVMCPU_LDR16_OPCODE:
        case CVMCPU_LDR8_OPCODE:
            CVMJITcsSetLoadInstruction(con);
            break;
        case CVMCPU_FSTR32_OPCODE:
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_STR16_OPCODE:
        case CVMCPU_STR8_OPCODE:
            CVMJITcsSetStoreInstruction(con);
            CVMJITcsPushSourceRegister(con, CVMJITcsDestRegister(con));
            CVMJITcsSetDestRegister(con, CVMCPU_INVALID_REG);
            break;
	default:
	    CVMassert(CVM_FALSE);
    }
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

    CVMJITstatsExec({
	/* This is going to be a memory reference. Determine
	   if read or write */
	switch(opcode) {
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_LDR16U_OPCODE:
        case CVMCPU_LDR16_OPCODE:
        case CVMCPU_LDR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD); break;
        case CVMCPU_FSTR32_OPCODE:
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_STR16_OPCODE:
        case CVMCPU_STR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
	default:
	    CVMassert(CVM_FALSE);
	}
    });

#ifndef IAI_CODE_SCHEDULER_SCORE_BOARD
    emitInstruction(con, instruction);
#else
    {
	CVMBool needClearInPlace = CVM_FALSE;
	if ((!(loadStoreAddressingMode & ARM_LOADSTORE_PREINDEX)) ||
	    (loadStoreAddressingMode & ARM_LOADSTORE_WRITEBACK))
	{
	    CVMJITcsSetDestRegister2(con, basereg);
	}

	if (loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET) {
	     /* offset is a base register in this case */
	    CVMJITcsPushSourceRegister(con, offset);
	} else if (CVMJITcsTestBaseRegister(con, CVMCPU_JFP_REG)) {
	    /* Before making CVMJITcsSetJavaFrameIndex call, we need to make
	     * sure we haven't exceeded the maximum index we can support.
	     */
	    if (offset < (con->numberLocalWords *
			  (CVMJITCS_TEMP_LOCAL_NUM - 1) * sizeof(CVMUint32)))
	    {
		int offsetIdx;
		if ((loadStoreAddressingMode & ARM_LOADSTORE_ADDOFFSET)) {
		    offsetIdx = offset / sizeof(CVMUint32);
		} else {
		    offsetIdx = -(offset / sizeof(CVMUint32));
		}
		CVMJITcsSetJavaFrameIndex(con,
		    offsetIdx + con->numberLocalWords);
	    } else {
		CVMJITcsSetEmitInPlace(con);
		needClearInPlace = CVM_TRUE;
	    }
	}

	emitInstruction(con, instruction);

	if (needClearInPlace) {
	    CVMJITcsClearEmitInPlace(con);
	}
    }
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

    CVMtraceJITCodegenExec({
        CVMBool isMode2 = CVM_FALSE;

        switch (opcode){
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_STR8_OPCODE:
	    isMode2 = CVM_TRUE;
	    break;
        }
        printPC(con);
        if (isMode2) {
            dumpMode2Instruction(con, instruction, condCode);
        } else {
            if (CVMARMisMode5Instruction(opcode)) {
		dumpMode5Instruction(con, instruction, condCode);
            } else {
                CVMconsolePrintf("	%s%s	%s, [%s",
                    getOpcodeName(opcode), conditions[condCode],
                    regNames[destreg], regNames[basereg]);
                /* not all are valid for all opcodes, but we're not
                   checking that here */
                if (loadStoreAddressingMode & ARM_LOADSTORE_PREINDEX){
                    CVMconsolePrintf(", ");
                } else {
                    CVMconsolePrintf("], ");
                }

                if (loadStoreAddressingMode & ARM_LOADSTORE_REGISTER_OFFSET) {
                    CVMconsolePrintf("%c%s",
                        (loadStoreAddressingMode & ARM_LOADSTORE_ADDOFFSET) ?
                        '+' : '-', regNames[offset]);
                    if ( shifterImm != 0 ){
                        CVMconsolePrintf(", %s #%d", shiftname(shiftOp),
                            shifterImm);
                    }
                } else {
                    CVMconsolePrintf("#%c%d",
                        (loadStoreAddressingMode & ARM_LOADSTORE_ADDOFFSET) ?
                        '+':'-', offset);
                }
                if (loadStoreAddressingMode & ARM_LOADSTORE_PREINDEX) {
                    CVMconsolePrintf("]");
                }
                if (loadStoreAddressingMode & ARM_LOADSTORE_WRITEBACK) {
                    CVMconsolePrintf("!");
                }
            }
	}
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do a load/store operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
    or
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
*/
void
CVMCPUemitArrayElementReference(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID, int indexRegID,
    int shiftOpcode, int shiftAmount)
{
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMARMmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, indexRegID,
                                 shiftOpcode, shiftAmount));
}

/* Purpose: Emits code to computes the following expression and stores the
            result in the specified destReg:
                destReg = baseReg + shiftOpcode(indexReg, #shiftAmount)
*/
void
CVMCPUemitComputeAddressOfArrayElement(CVMJITCompilationContext *con,
                                       int opcode, int destRegID,
                                       int baseRegID, int indexRegID,
                                       int shiftOpcode, int shiftAmount)
{
    CVMCPUALURhsToken rhs =
	CVMARMalurhsEncodeToken(con, CVMARM_ALURHS_SHIFT_BY_CONSTANT,
				shiftAmount, shiftOpcode,
				indexRegID, CVMCPU_INVALID_REG);
    CVMCPUemitBinaryALU(con, opcode, destRegID, baseRegID, rhs,
			CVMJIT_NOSETCC);
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
    CVMCPUALURhsToken rhs;

    /* add  rDest, rAdd, rShift, LSL #i */
    rhs = CVMARMalurhsEncodeShiftByConstantToken(con, shiftRegID, shiftOpcode,
                                                 shiftAmount);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, addRegID, rhs,
			CVMJIT_NOSETCC);
}

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

/*
 * Purpose: Emits an atomic swap operation. The value to swap in is in
 *          destReg,  which is also where the swapped out value will be placed.
 */
extern void
CVMCPUemitAtomicSwap(CVMJITCompilationContext* con,
		     int destReg, int addressReg)
{
#ifndef CVM_MP_SAFE
    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
		    (CVMUint32)CVMARM_SWP_OPCODE |
		    addressReg << 16 | destReg << 12 | destReg);

    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("\tswp\t%s, %s, [%s]",
			 regNames[destReg], regNames[destReg],
			 regNames[addressReg]);
	});
    CVMJITdumpCodegenComments(con);
#else
    int retryLogicalPC;
    CVMRMResource *scratchRes1, *scratchRes2;
    int scratchReg1, scratchReg2;

    /*
     * retry:
     *   ldrex scratch1, [addressReg]
     *   strex scratch2, destReg, [addressReg]
     *   cmp   scratch2, #0
     *   bne   retry
     *   mov   destReg, scratch1
     */
 
    /* top of retry loop in case ldrex/strex operation is interrupted. */
    CVMtraceJITCodegen(("\t\tretry:\n"));

    scratchRes1 = CVMRMgetResource(CVMRM_INT_REGS(con),
                                   CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    scratchRes2 = CVMRMgetResource(CVMRM_INT_REGS(con),
                                   CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    scratchReg1 = CVMRMgetRegisterNumber(scratchRes1);
    scratchReg2 = CVMRMgetRegisterNumber(scratchRes2);

    retryLogicalPC = CVMJITcbufGetLogicalPC(con);

    /* emit ldrex instruction */
    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    ARM_LDREX_OPCODE | addressReg << 16 |
                    scratchReg1 << 12);
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	ldrex	r%s, [r%s]",
			 regNames[scratchReg1],
			 regNames[addressReg]);
    });
    CVMJITdumpCodegenComments(con);

    /* emit strex instruction */
    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    ARM_STREX_OPCODE | addressReg << 16 |
                    scratchReg2 << 12 | destReg);
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	strex	r%s, r%s, [r%s]",
			 regNames[scratchReg1], regNames[destReg],
			 regNames[addressReg]);
    });
    CVMJITdumpCodegenComments(con);

    /* Check if strex succeed. Retry if failed. */
    CVMJITaddCodegenComment((con, "retry if strex failed"));
    CVMCPUemitCompareConstant(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
                              scratchReg2, 0);
    CVMCPUemitBranch(con, retryLogicalPC, CVMCPU_COND_NE);

    /* Move the old value into destReg */
    CVMJITaddCodegenComment((con, "move old value into destReg"));
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
                           destReg, scratchReg1, CVMJIT_NOSETCC);

    CVMJITprintCodegenComment(("End of Atomic Swap"));

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), scratchRes1);
    CVMRMrelinquishResource(CVMRM_INT_REGS(con), scratchRes2);
#endif
}
#endif
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

#if defined(CVM_JIT_COPY_CCMCODE_TO_CODECACHE) && defined(CVMJIT_PATCH_BASED_GC_CHECKS)
/* glue routine that calls CVMCCMruntimeGCRendezvous and adjustment lr. */
extern void
CVMARMruntimeGCRendezvousAdjustRetAddrGlue();
#endif

/*
 * Emit code to do a gc rendezvous.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind)
{
    CVMJITcsSetEmitInPlace(con);
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    {
	/* In this case, it will be a simple bl to the code cache copy */
	int helperOffset;

        if (cbufRewind) {
	    /* The code buffer will be rewinded after the bl instruction,
             * and the bl will be patched with the next instruction. In
             * this case, CVMARMruntimeGCRendezvousAdjustRetAddrGlue is 
             * going to subtract the lr by 4, so we can return to the 
             * patch point after the gc is done.
             */
            CVMJITaddCodegenComment((con,
                "call CVMARMruntimeGCRendezvousAdjustRetAddrGlue"));
            helperOffset = CVMCCMcodeCacheCopyHelperOffset(con,
		CVMARMruntimeGCRendezvousAdjustRetAddrGlue);
        } else {
	    /* There is no rewinding in this case. We don't want to return
             * to the bl instruction after the gc is done, so call
             * CVMCCMruntimeGCRendezvousGlue instead. The
             * lr is not adjusted in this case.
             */
            CVMJITaddCodegenComment((con,
                    "call CVMCCMruntimeGCRendezvousGlue"));
            helperOffset =  CVMCCMcodeCacheCopyHelperOffset(con,
                    CVMCCMruntimeGCRendezvousGlue);
        }
	CVMCPUemitBranchLink(con, helperOffset);
    }
#else
    /* Emit the gc rendezvous instruction(s) */
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeGCRendezvousGlue"));

    /*
     * This is ugly. CVMCPUemitAbsoluteCall() will generate a load
     * of the CVMCCMruntimeGCRendezvousGlue constant. At some later
     * point (when the constant pool is dumped) the load instruction
     * will be patched to reference the dumped contant. However,
     * by the time this happens it is too late. We've already
     * copied the load instruction to the patchedInstructions array,
     * and emitted a new instruction at the same location. Because of
     * this, we instead load the address from the ccee and avoid
     * the contant pool manager altogether.
     */
    {
	CVMCPUMemSpecToken offsetToken = CVMCPUmemspecEncodeImmediateToken(
	    con, CVMoffsetof(CVMCCExecEnv, ccmGCRendezvousGlue));
	emitReturnAddressComputation(con, CVMCPU_COND_AL, 0);
	CVMCPUemitMemoryReference(con, CVMCPU_LDR32_OPCODE,
				  CVMARM_PC, CVMCPU_SP_REG, offsetToken);
    }
#endif
   CVMJITcsClearEmitInPlace(con);
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

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode |
                    (destRegID & 0x01) << 22 | (destRegID >> 1) << 12 |
                    (lhsRegID  & 0x01) << 7  | (lhsRegID  >> 1) << 16 |
                    (rhsRegID  & 0x01) << 5  | (rhsRegID  >> 1));

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%%f%d, %%f%d, %%f%d",
            getOpcodeName(opcode),
            destRegID, lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitUnaryFP( CVMJITCompilationContext* con,
		    int opcode, int destRegID, int srcRegID)
{
    CVMassert(opcode == CVMCPU_FNEG_OPCODE || opcode == CVMCPU_DNEG_OPCODE ||
              opcode == CVMARM_I2F_OPCODE  || opcode == CVMARM_F2I_OPCODE  ||
              opcode == CVMARM_I2D_OPCODE  || opcode == CVMARM_D2I_OPCODE  ||
              opcode == CVMARM_F2D_OPCODE  || opcode == CVMARM_D2F_OPCODE);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode |
                    (destRegID & 0x01) << 22 | (destRegID >> 1) << 12 |
                    (srcRegID  & 0x01) << 5  | (srcRegID  >> 1));

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%%f%d, %%f%d",
            getOpcodeName(opcode),
            destRegID, srcRegID);
    });

    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitFCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, int rhsRegID)
{
    CVMassert(opcode == CVMCPU_FCMP_OPCODE   ||
              opcode == CVMARM_FCMPES_OPCODE ||
              opcode == CVMCPU_DCMP_OPCODE   ||
              opcode == CVMARM_DCMPED_OPCODE ||
              opcode == CVMARM_FMSTAT_OPCODE);

    if (opcode == CVMARM_FMSTAT_OPCODE) {
        lhsRegID = 0;
        rhsRegID = 0;
    }
    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode |
                    (lhsRegID  & 0x01) << 22 | (lhsRegID >> 1) << 12 |
                    (rhsRegID  & 0x01) << 5  | (rhsRegID >> 1));

    if (opcode == CVMARM_FMSTAT_OPCODE) {
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	fmstat");
        });
    } else {
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	%s	%%f%d, %%f%d",
	        getOpcodeName(opcode),
                lhsRegID, rhsRegID);
        });
    }
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Loads a 32-bit constant into an FP register. */
extern void
CVMCPUemitLoadConstantFP(
    CVMJITCompilationContext *con,
    int regID,
    CVMInt32 v)
{
    CVMCPUemitLoadConstantConditionalFP(con, regID, v, CVMCPU_COND_AL);
}

/* Purpose: Loads a 64-bit double constant into an FP register. */
void
CVMCPUemitLoadLongConstantFP(CVMJITCompilationContext *con, int regID,
                             CVMJavaVal64 *value)
{

    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstantFP(con, LOREG(regID), (int)val.l);
    CVMCPUemitLoadConstantFP(con, HIREG(regID), val.l>>32);
}

void
CVMARMemitMoveFloatFP(CVMJITCompilationContext* con,
                 int opcode, int fpRegID, int regID)
{
    CVMassert(opcode == CVMARM_MOVFA_OPCODE || opcode == CVMARM_MOVAF_OPCODE);

    CVMassert(((unsigned)fpRegID) < 32 && ((unsigned)regID) < 16);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode |
                    (regID << 12) |
                    (fpRegID  & 0x01) << 7  | (fpRegID  >> 1) << 16);

    CVMtraceJITCodegenExec({
        printPC(con);
        if (opcode == CVMARM_MOVAF_OPCODE) {
            CVMconsolePrintf("	%s	%s, %%f%d",
                getOpcodeName(opcode),
                regNames[regID], fpRegID);
        } else {
            CVMconsolePrintf("	%s	%%f%d, %s",
                getOpcodeName(opcode),
                fpRegID, regNames[regID]);
        }
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMARMemitMoveDoubleFP(CVMJITCompilationContext* con,
                 int opcode, int fpRegID, int regID)
{
    int lo;
    int hi;

    CVMassert(opcode == CVMARM_MOVDA_OPCODE || opcode == CVMARM_MOVAD_OPCODE);
    CVMassert((fpRegID & 1) == 0);

    lo = LOREG(regID);
    hi = HIREG(regID);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode | (fpRegID >> 1) |
                    (lo << 12) | (hi << 16));

    CVMtraceJITCodegenExec({
        printPC(con);
        if (opcode == CVMARM_MOVAD_OPCODE) {
            CVMconsolePrintf("	%s	%s, %s, %%f%d",
                getOpcodeName(opcode),
                regNames[lo], regNames[hi], fpRegID);
	} else {
            CVMconsolePrintf("	%s	%%f%d, %s, %s",
                getOpcodeName(opcode),
                fpRegID, regNames[lo], regNames[hi]);
	}
    });
    CVMJITdumpCodegenComments(con);
}

static const char *const fpStatusRegNames[] = {
    "fpsid", "fpscr","","","","","","","fpexc"
};

void
CVMARMemitStatusRegisterFP(CVMJITCompilationContext* con,
                 int opcode, int statusReg, int regID)
{
    CVMassert(opcode == CVMARM_MOVSA_OPCODE || opcode == CVMARM_MOVAS_OPCODE);

    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    (CVMUint32)opcode | (regID << 12) | (statusReg << 16));

    CVMtraceJITCodegenExec({
        printPC(con);
        if (opcode == CVMARM_MOVAS_OPCODE) {
            CVMconsolePrintf("	%s	%s, %s",
                getOpcodeName(opcode),
                regNames[regID], fpStatusRegNames[statusReg]);
        } else {
            CVMconsolePrintf("	%s	%s, %s",
                getOpcodeName(opcode),
                fpStatusRegNames[statusReg] , regNames[regID]);
        }
    });
    CVMJITdumpCodegenComments(con);
}
#endif /* CVM_JIT_USE_FP_HARDWARE */

/*
 * CVMJITcanReachAddress - Check if toPC can be reached by an
 * instruction at fromPC using the specified addressing mode. If
 * needMargin is ture, then a margin of safety is added (usually the
 * allowed offset range is cut in half).
 */
CVMBool
CVMJITcanReachAddress(CVMJITCompilationContext* con,
		      int fromPC, int toPC,
		      CVMJITAddressMode mode, CVMBool needMargin)
{
    switch (mode){
    case CVMJIT_BRANCH_ADDRESS_MODE:
    case CVMJIT_COND_BRANCH_ADDRESS_MODE:
	return CVM_TRUE; /* 32 Meg should reach in every case. */
    case CVMJIT_FPMEMSPEC_ADDRESS_MODE:
    case CVMJIT_MEMSPEC_ADDRESS_MODE: {
	int diff;
	int maxDiff = 
#ifdef CVM_JIT_USE_FP_HARDWARE	
	    mode == CVMJIT_FPMEMSPEC_ADDRESS_MODE ?
                CVMARM_FP_MAX_LOADSTORE_OFFSET:
#endif                
                CVMCPU_MAX_LOADSTORE_OFFSET;
#ifdef CVMCPU_HAS_CP_REG
	fromPC = con->target.cpLogicalPC; /* relative to cp base register */
	needMargin = CVM_FALSE;           /* we never worry about the margin */
#else
	toPC -= 8;                       /* relative to pc, offset by 8 */
#endif
	diff = absolute(toPC - fromPC);
	if (needMargin) {
	    maxDiff /= 2;  /* provide a margin of error */
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

    if (!CVMJITcanReachAddress(con,
			       instructionLogicalAddress,
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
	/* adjust for +8 PC bias */
	offsetBits = targetLogicalAddress - instructionLogicalAddress - 8;
	instruction &= 0xff000000; /* mask off lower 24 bits */
	offsetBits >>= 2;
	offsetBits &= 0x00ffffff;  /* offset in lower 24 bits */
	break;
    case CVMJIT_MEMSPEC_ADDRESS_MODE:
#ifdef CVMCPU_HAS_CP_REG
	/* offset is relative to cp base register */
	offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
#else
	/* offset is relative to pc, adjust for +8 PC bias */
	offsetBits = targetLogicalAddress - instructionLogicalAddress - 8;
#endif
	if (offsetBits < 0) {
	    offsetBits = -offsetBits;
	    instruction &= ~ARM_LOADSTORE_ADDOFFSET; /* turn off U bit */
	} else {
	    instruction |= ARM_LOADSTORE_ADDOFFSET; /* turn on U bit */
	}
	{
	    CVMUint32 offsetMask = 0x00000fff;
#ifdef CVM_JIT_USE_FP_HARDWARE
            /* Determines if this is a VFP instruction or not */
            if (((instruction >> 25) & 0x7) == 6) {
		offsetMask = 0x000000ff;
	        CVMassert((offsetBits & 0x3) == 0);
                offsetBits >>= 2;
	    }
#endif
	    CVMassert((offsetBits &~offsetMask) == 0);
	    instruction &=~offsetMask; /* mask off lower 12 bits */
        }
	break;
    default:
	offsetBits = 0; /* avoid compiler warning */
	CVMassert(CVM_FALSE);
	break;
    }
    instruction |= offsetBits;
    *instructionPtr = instruction;
}

#ifdef CVM_MP_SAFE
void
CVMCPUemitMemBar(CVMJITCompilationContext* con)
{
    CVMRMResource *rdRes;
    int rdReg;

#define CP_NUM     (0xf << 8)
#define OP_2       (5 << 5)

    rdRes = CVMRMgetResourceForConstant32(CVMRM_INT_REGS(con),
			     CVMRM_ANY_SET, CVMRM_EMPTY_SET, 0);
    rdReg = CVMRMgetRegisterNumber(rdRes);

    CVMJITprintCodegenComment(("Emit a memory barrier"));
    emitInstruction(con, ARM_MAKE_CONDCODE_BITS(CVMCPU_COND_AL) |
                    ARM_MCR_OPCODE | C7 << 16 | rdReg << 12 |
                    CP_NUM | OP_2 | (1 << 4) | C10);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	mcr	p15, 0, %s, c7, c10, 5", regNames[rdReg]);
    });
    CVMJITdumpCodegenComments(con);
    CVMRMrelinquishResource(CVMRM_INT_REGS(con), rdRes);
}
#endif


#ifdef IAI_CACHEDCONSTANT
/*
 * CVMJITemitLoadConstantAddress - Emit instruction(s) to load the address
 * of a constant pool constant into CVMCPU_ARG4_REG (a4).
 */
void
CVMJITemitLoadConstantAddress(CVMJITCompilationContext* con,
			      int targetLogicalAddress)
{
    CVMInt32 offsetBits = targetLogicalAddress - CVMJITcbufGetLogicalPC(con);
    CVMUint32 base;
    CVMUint32 rotate;

    /*
     * offsetBits == 0 is a special case when we don't know the target
     * address yet. Don't adjust for the PC in this case.
     */
    if (offsetBits != 0) {
	offsetBits -= 8;
    }
    CVMassert(offsetBits >= 0);

#ifdef CVMCPU_HAS_CP_REG
    CVMassert(CVM_FALSE);
#endif
    CVMJITaddCodegenComment((con, "load address of cached constant"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
				CVMCPU_ARG4_REG, CVMARM_PC,
				offsetBits, CVMJIT_NOSETCC);
    /*
     * If it was encodeable, then we only emitted one instruction but
     * we need to always emit two.
     */
    if (CVMARMmode1EncodeImmediate(offsetBits, &base, &rotate)) {
	CVMCPUemitNop(con);
    }
}
#endif /* IAI_CACHEDCONSTANT */

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#ifdef CVMJIT_INTRINSICS
/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
extern CVMJITRegsRequiredType
CVMARMCCALLgetRequired(CVMJITCompilationContext *con,
                       CVMJITRegsRequiredType argsRequired,
		       CVMJITIRNode *intrinsicNode,
		       CVMJITIntrinsic *irec,
		       CVMBool useRegArgs)
{
    CVMJITRegsRequiredType result = CVMCPU_AVOID_C_CALL | argsRequired;
    int numberOfArgs = irec->numberOfArgs;

    CVMassert(useRegArgs);
    if (numberOfArgs != 0) {
        int i;
        CVMJITIRNode *iargNode = CVMJITirnodeGetLeftSubtree(intrinsicNode);
        for (i = 0; i < numberOfArgs; i++) {
            int regno;
	    int	argType = CVMJITgetTypeTag(iargNode);
	    int argWordIndex = CVMJIT_IARG_WORD_INDEX(iargNode);
	    int argSize = CVMARMCCALLargSize(argType);

	    if (argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
	        regno = CVMCPU_ARG1_REG + argWordIndex;
	    } else {
	        /* IAI-22 */
	        regno = (CVMARM_v3 + argWordIndex - CVMCPU_MAX_ARG_REGS);
		/* Make sure that we are not allocating registers so high
		   that we overflow into the stack pointer and other non-
		   allocatable regs. */
		CVMassert(regno + argSize - 1 < CVMARM_sp);
	    }
	    result |= (1U << regno);
	    iargNode = CVMJITirnodeGetRightSubtree(iargNode);
	}
	CVMassert(iargNode->tag == CVMJIT_ENCODE_NULL_IARG);
    }
    return result;
}
#endif /* CVMJIT_INTRINSICS */

/* Purpose: Pins an arguments to the appropriate register or store it into the
            appropriate stack location. */
CVMRMResource *
CVMCPUCCALLpinArg(CVMJITCompilationContext *con,
                  CVMCPUCallContext *callContext, CVMRMResource *arg,
                  int argType, int argNo, int argWordIndex,
                  CVMRMregset *outgoingRegs, CVMBool useRegArgs)
{
    int argSize = CVMARMCCALLargSize(argType);

    if (useRegArgs || argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
        int regno;
        if (argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
            regno = CVMCPU_ARG1_REG + argWordIndex;
	} else {
	    /* IAI-22 */
	    CVMassert(useRegArgs == CVM_TRUE);
	    regno = (CVMARM_v3 + argWordIndex - CVMCPU_MAX_ARG_REGS);
	    /* Make sure that we are not allocating registers so high that we
	       overflow into the stack pointer and other non-allocatable
	       regs. */
	    CVMassert(regno + argSize - 1 < CVMARM_sp);
	}
        arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
        CVMassert(regno + arg->size <= CVMCPU_MAX_ARG_REGS || useRegArgs);
        *outgoingRegs |= arg->rmask;
    } else {
        int stackIndex = (argWordIndex - CVMCPU_MAX_ARG_REGS) * 4;
        CVMRMpinResource(CVMRM_INT_REGS(con), arg,
			 CVMRM_ANY_SET, CVMRM_EMPTY_SET);
        if (argSize == 1) {
            CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                CVMRMgetRegisterNumber(arg), CVMCPU_SP_REG, stackIndex);
        } else {
	    /* Assert argument is 64-bit aligned for AAPCS. If this ever fails
	     * we need actually do the alignment when using AAPCS. */
	    CVMassert(argWordIndex % 2 == 0);
            if (argWordIndex < CVMCPU_MAX_ARG_REGS) {
                /* Reserve the last arg because we're using it for the low
                   word of this argument.  Since we can't pin the arg which
                   is 64 bit to it, we have to create an artificial resource
                   just to keep that register protected until we're done with
                   it.  Tracking the reservedRes in the callContext will
                   allow us to relinquish it later when we're done with it. */
                callContext->reservedRes =
                    CVMRMgetResourceSpecific(CVMRM_INT_REGS(con),
					     CVMCPU_ARG4_REG, 1);

                /* We have a 64 bit arg and only one arg register left.
                   Hence, we put the high word on the stack: */
                CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                    CVMRMgetRegisterNumber(arg)+1, CVMCPU_SP_REG, 0);

                /* And the low word in the last arg register: */
                CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
                    CVMCPU_ARG4_REG, CVMRMgetRegisterNumber(arg),
                    CVMJIT_NOSETCC);

                *outgoingRegs |= callContext->reservedRes->rmask;
            } else {
                CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR64_OPCODE,
                    CVMRMgetRegisterNumber(arg), CVMCPU_SP_REG, stackIndex);
            }
        }
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);
    }
    return arg;
}

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
CVMCPUInstruction
CVMCPUfixupInstructionAddress(CVMJITCompilationContext* con,
			      CVMCPUInstruction instruction,
			      int instructionLogicalAddress,
			      int targetLogicalAddress,
			      CVMJITAddressMode instructionAddressMode)
{
    int offsetBits;
#ifdef CVM_DEBUG_ASSERTS
    if (!CVMJITcanReachAddress(con,
                               instructionLogicalAddress,
                               targetLogicalAddress,
                               instructionAddressMode, CVM_FALSE))
    {
	 /* This should never happen. */
        CVMassert(CVM_FALSE);
    }
#endif

    switch (instructionAddressMode) {
    case CVMJIT_BRANCH_ADDRESS_MODE:
    case CVMJIT_COND_BRANCH_ADDRESS_MODE:
	/* adjust for +8 PC bias */
	offsetBits = targetLogicalAddress - instructionLogicalAddress - 8;
	instruction &= 0xff000000; /* mask off lower 24 bits */
	offsetBits >>= 2;
	offsetBits &= 0x00ffffff;  /* offset in lower 24 bits */
	break;
    case CVMJIT_MEMSPEC_ADDRESS_MODE:
#ifdef CVMCPU_HAS_CP_REG
	/* offset is relative to cp base register */
	offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
#else
	/* offset is relative to pc, adjust for +8 PC bias */
	offsetBits = targetLogicalAddress - instructionLogicalAddress - 8;
#endif
	if (offsetBits < 0) {
	    offsetBits = -offsetBits;
	    instruction &= ~ARM_LOADSTORE_ADDOFFSET; /* turn off U bit */
	} else {
	    instruction |= ARM_LOADSTORE_ADDOFFSET; /* turn on U bit */
	}
	instruction &= 0xfffff000; /* mask off lower 12 bits */
	offsetBits &= 0x00000fff;  /* offset in lower 12 bits */
	break;
    default:
	offsetBits = 0; /* avoid compiler warning */
	CVMassert(CVM_FALSE);
	break;
    }
    instruction |= offsetBits;
    return instruction;
}
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */
