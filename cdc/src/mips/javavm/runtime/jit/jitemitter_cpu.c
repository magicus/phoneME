/*
 * @(#)jitemitter_cpu.c	1.66 06/10/13
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
 * MIPS-specific bits.
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
 * #define CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED if you have 64-bit
 * GPRs and you want to try to use LDL/LDR/STL/STR to load/store
 * unaligned 64-bit values. This should normally work, but seems not
 * to on some hardware.
 */
#ifdef CVMCPU_HAS_64BIT_REGISTERS
#undef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
#endif

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

#if CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN
#define HIREGFP(reg)  (reg)
#define LOREGFP(reg)  (reg)+1
#elif CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define HIREGFP(reg)  (reg)+1
#define LOREGFP(reg)  (reg)
#endif

#define MIPS_RHSISREG CVM_TRUE
#define MIPS_RHSISIMM CVM_FALSE

/* The maximum reach for a pc relative branch (18 bits signed offset)
 * NOTE: The 18-bit signed offset is added to the address of the
 * instruction following the branch (not the branch itself), in the
 * branch delay slot, to form a PC-relative effective target address.
 */
#define CVMMIPS_MAX_BRANCH_OFFSET ( 128*1024)
#define CVMMIPS_MIN_BRANCH_OFFSET (-128*1024+4)

/**************************************************************
 * Private #defines for opcode encodings.
 **************************************************************/

#define MIPS_RS_SHIFT 21      /* source register */
#define MIPS_I_RD_SHIFT 16    /* I-Type dest register */
#define MIPS_R_RD_SHIFT 11    /* R-Type dest register */
#define MIPS_R_RT_SHIFT 16    /* R-Type rhs register */
#define MIPS_SA_SHIFT 6       /* shift amount */

#define MIPS_ADD_OPCODE	  (0x20) /* reg32 = reg32 + reg32. */
#define MIPS_ADDU_OPCODE  (0x21) /* reg32 = reg32 + reg32. No overflow. */
#define MIPS_ADDI_OPCODE  (0x08 << 26) /* reg32 = reg32 + imm32. */
#define MIPS_ADDIU_OPCODE (0x09 << 26) /* reg32 = reg32 + imm32. No overflow.*/
#define MIPS_SUB_OPCODE   (0x22) /* reg32 = reg32 - reg32. */
#define MIPS_SUBU_OPCODE  (0x23) /* reg32 = reg32 - reg32. Unsigned. */
#define MIPS_MUL_OPCODE   (0x70000002) /* reg32 = reg32 * reg32. */
#define MIPS_MULT_OPCODE  (0x18) /* HI, LO = reg32 * reg32. */
#define MIPS_MULTU_OPCODE (0x19) /* HI, LO = reg32U * reg32U. */
#define MIPS_MADD_OPCODE  (0x70000000) /* HI,LO = (HI,LO)+(reg32*reg32). */
#define MIPS_MADDU_OPCODE (0x70000001) /* HI,LO = (HI,LO)+(reg32*reg32), unsigned.*/
#define MIPS_DIV_OPCODE   (0x1a) /* Div32 */

#define MIPS_AND_OPCODE	  (0x24) /* reg32 = reg32 AND reg32. */
#define MIPS_ANDI_OPCODE  (0x0c << 26) /* reg32 = reg32 AND imm32.*/
#define MIPS_OR_OPCODE	  (0x25) /* reg32 = reg32 OR reg32. */
#define MIPS_ORI_OPCODE	  (0x0d << 26) /* reg32 = reg32 OR imm32.*/
#define MIPS_NOR_OPCODE   (0x27) /* reg32 = reg32 NOR reg32. */
#define MIPS_XOR_OPCODE	  (0x26) /* reg32 = reg32 XOR reg32. */
#define MIPS_XORI_OPCODE  (0x0e << 26) /* reg32 = reg32 XOR imm32.*/

#define MIPS_SLL_OPCODE	  (0x00) /* Shift Left Logical */
#define MIPS_SLLV_OPCODE  (0x04) /* Shift Left Logical. Shift amount in reg.*/
#define MIPS_SRL_OPCODE	  (0x02) /* Shift Right Logical */
#define MIPS_SRLV_OPCODE  (0x06) /* Shift Right Logical. Shift amount in reg*/
#define MIPS_SRA_OPCODE	  (0x03) /* Shift Right Arithmetic */
#define MIPS_SRAV_OPCODE  (0x07) /* Shift Right Arithmetic. Shift amount in reg*/

#define MIPS_MOVN_OPCODE  (0x0b) /* Move Conditional on Not Zero. */
#define MIPS_MOVZ_OPCODE  (0x0a) /* Move Conditional on Zero. */
#define MIPS_MFHI_OPCODE  (0x10) /* Move From HI Register. */
#define MIPS_MFLO_OPCODE  (0x12) /* Move Form LO Register. */
#define MIPS_MTHI_OPCODE  (0x11) /* Move To HI Register. */
#define MIPS_MTLO_OPCODE  (0x13) /* Move To LO Register. */

#define MIPS_SLT_OPCODE   (0x2a) /* Set on Less Than. */
#define MIPS_SLTU_OPCODE  (0x2b) /* Set on Less Than Unsigend. */
#define MIPS_SLTI_OPCODE  (0x0a << 26) /* Set on Less Than Immediate. */
#define MIPS_SLTIU_OPCODE (0x0b << 26) /* Set on Less Than Immediate Unsigned. */

#define MIPS_BEQ_OPCODE   (0x04 << 26) /* Branch on Equal. */
#define MIPS_BNE_OPCODE   (0x05 << 26) /* Branch on Not Equal. */
#define MIPS_BGEZ_OPCODE  (0x01 << 16) /* Branch on Greater Than or Equal to 0. */
#define MIPS_BGEZAL_OPCODE (0x11 << 16) /*Branch on Greater Than or Equal to 0 and Link*/
#define MIPS_BGTZ_OPCODE  (0x07 << 26) /* Branch on Greater Than 0. */
#define MIPS_BLEZ_OPCODE  (0x06 << 26) /* Branch on Less Than or Equal to 0. */
#define MIPS_BLTZ_OPCODE  (0x00 << 16) /* Branch on Less Than 0. */
#define MIPS_BLTZAL_OPCODE (0x10 << 16) /*Branch on Less Than 0 and Link. */
#define MIPS_J_OPCODE     (0x02 << 26) /* Jump. */
#define MIPS_JAL_OPCODE   (0x03 << 26) /* Jump and Link. */
#define MIPS_JALR_OPCODE  (0x09) /* Jump and Link Register. */
#define MIPS_JR_OPCODE    (0x08) /* Jump Register. */

#define MIPS_LD_OPCODE    (0x37 << 26) /* Load 64 bit value. */
#define MIPS_SD_OPCODE    (0x3f << 26) /* Store 64 bit value. */
#define MIPS_LW_OPCODE    (0x23 << 26) /* Load 32 bit value (signed). */
#define MIPS_SW_OPCODE    (0x2b << 26) /* Store 32 bit value (signed). */
#define MIPS_LHU_OPCODE   (0x25 << 26) /* Load unsigned 16 bit value. */
#define MIPS_LH_OPCODE    (0x21 << 26) /* Load signed 16 bit value. */
#define MIPS_SH_OPCODE    (0x29 << 26) /* Store 16 bit value. */
#define MIPS_LBU_OPCODE   (0x24 << 26) /* Load signed 8 bit value. */
#define MIPS_LB_OPCODE    (0x20 << 26) /* Load signed 8 bit value. */
#define MIPS_SB_OPCODE    (0x28 << 26) /* Store 8 bit value. */

#define MIPS_LL_OPCODE    (0x30 << 26) /* Load Linked 32 bit value */
#define MIPS_SC_OPCODE    (0x38 << 26) /* Store Conditional 32 bit value */

#define MIPS_LUI_OPCODE   (0x0f << 26) /* Load Upper Immediate. */

#define MIPS_BC1F_OPCODE (0x11<<26 | 0x8<<21) /* Branch on FP False */
#define MIPS_BC1T_OPCODE (0x11<<26 | 0x8<<21 | 0x1<<16) /* Branch on FP True */

/*
 * 64-bit GPR support.
 */
#ifdef CVMCPU_HAS_64BIT_REGISTERS
#define MIPS_DSLL32_OPCODE (0x3c) /* Doubleword Shift Left Logical Plus 32*/
#define MIPS_DSRL32_OPCODE (0x3e) /* Doubleword Shift Right Logical Plus 32 */
#define MIPS_DSRA32_OPCODE (0x3f) /* Doubleword Shift Right Arithmetic Plus 32 */
#endif

/*
 * Hardware FP support.
 */
#ifdef CVM_JIT_USE_FP_HARDWARE

#define MIPS_SWC1_OPCODE (0x39 << 26) /* Store Word from Floating Point */
#define MIPS_SDC1_OPCODE (0x3d << 26) /* Store Doubleword from Floating Point */
#define MIPS_LWC1_OPCODE (0x31 << 26) /* Load Word to Floating Point */
#define MIPS_LDC1_OPCODE (0x35 << 26) /* Load Doubleword to Floating Point */

#define MIPS_FT_SHIFT 16 /* ft register */
#define MIPS_FS_SHIFT 11 /* fs register */
#define MIPS_FD_SHIFT 6  /* fd register */

#define MIPS_FP_COP1      (0x11 << 26)
#define MIPS_FP_S_FMT     (0x10 << 21) /* Single */
#define MIPS_FP_D_FMT     (0x11 << 21) /* Double */
#define MIPS_FP_W_FMT     (0x14 << 21) /* Word */

#define MIPS_FP_NEG_OPCODE  (0x7) /* Floating Point Negate */
#define MIPS_FP_ADD_OPCODE  (0)   /* Floating Point Add */
#define MIPS_FP_SUB_OPCODE  (0x1) /* Floating Point Sub */
#define MIPS_FP_MUL_OPCODE  (0x2) /* Floating Point Mul */
#define MIPS_FP_DIV_OPCODE  (0x3) /* Floating Point Div */
#define MIPS_FP_CMP_OPCODE  (0x3 << 4) /* Floating Point Compare */
#define MIPS_FP_MOV_OPCODE  (0x6) /* Floating Point Move */

#define MIPS_FP_CVTD_OPCODE (0x21) /* Floating Point Convert to Double */
#define MIPS_FP_CVTS_OPCODE (0x20) /* Floating Point Convert to Single */

#define MIPS_FP_TRUNCW_OPCODE (0xd) /*Floating Point Truncate to Word Fixed Point*/

#define MIPS_MFC1_OPCODE    (0)         /* Move 32-bit GPR from 32-bit FPR */
#define MIPS_MTC1_OPCODE    (0x4 << 21) /* Move 32-bit GPR to 32-bit FPR */

#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
#define MIPS_LDC1_OPCODE  (0x35 << 26) /* load aligned double to 64-bit FPR */
#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
#define MIPS_LDL_OPCODE   (0x1A << 26) /* load unaligned 64-bit GPR left */
#define MIPS_LDR_OPCODE   (0x1B << 26) /* load unaligned 64-bit GPR right */
#define MIPS_SDL_OPCODE   (0x2C << 26) /* store unaligned 64-bit GPR left */
#define MIPS_SDR_OPCODE   (0x2D << 26) /* load unaligned 64-bit GPR right */
#endif
#define MIPS_DMFC1_OPCODE (0x1 << 21) /* Move 64-bit GPR from 64-bit FPR */
#define MIPS_DMTC1_OPCODE (0x5 << 21) /* Move 64-bit GPR to 64-bit FPR */
#endif /* CVMMIPS_HAS_64BIT_FP_REGISTERS */

#endif /* CVM_JIT_USE_FP_HARDWARE */

#ifdef CVMCPU_HAS_64BIT_REGISTERS
static void
CVMMIPSemitDoublewordShiftPlus32(CVMJITCompilationContext* con, 
                                 int opcode, int destRegID, int srcRegID,
                                 CVMUint32 shiftAmount);
#endif

/**************************************************************
 * CPU ALURhs and associated types - The following are implementations
 * of the functions for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* Purpose: Checks to see if the specified value is encodable as an immediate
            value that can be used as an ALURhs in an instruction. */
extern CVMBool
CVMCPUalurhsIsEncodableAsImmediate(int opcode, CVMInt32 constValue)
{
    switch (opcode) {
        case CVMCPU_AND_OPCODE:
        case CVMCPU_OR_OPCODE:
        case CVMCPU_XOR_OPCODE: {
	    return ((constValue & 0xffff0000) == 0);
	}
    
        case CVMCPU_SUB_OPCODE:
        case CVMCPU_CMN_OPCODE:
	    constValue = -constValue;
        case CVMCPU_ADD_OPCODE:
        case CVMCPU_CMP_OPCODE:
        case CVMCPU_MOV_OPCODE:
        case CVMCPU_MULL_OPCODE: {
            return (constValue >= -32768 && constValue < 32768);
        }
        case CVMCPU_BIC_OPCODE: {
            return CVM_TRUE;
	}
        default: {
	    CVMassert(CVM_FALSE);
	    return CVM_FALSE;
	}
    }
}

/* ALURhs constructors and query APIs: ==================================== */

/* Purpose: Constructs a constant type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewConstant(CVMJITCompilationContext* con, CVMInt32 v)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMMIPS_ALURHS_CONSTANT;
    ap->constValue = v;
    return ap;
}

/* Purpose: Constructs a register type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewRegister(CVMJITCompilationContext* con, CVMRMResource* rp)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMMIPS_ALURHS_REGISTER;
    ap->r = rp;
    return ap;
}

/* Purpose: Encodes the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */

CVMCPUALURhsToken
CVMMIPSalurhsGetToken(const CVMCPUALURhs *aluRhs)
{
    CVMCPUALURhsToken result;
    if (aluRhs->type == CVMMIPS_ALURHS_REGISTER) {
	int regNo = CVMRMgetRegisterNumber(aluRhs->r);
        result = CVMMIPSalurhsEncodeRegisterToken(regNo);
    } else {
	CVMassert(aluRhs->type == CVMMIPS_ALURHS_CONSTANT);
	result = CVMMIPSalurhsEncodeConstantToken(aluRhs->constValue);
    }
    return result;
}

/* ALURhs resource management APIs: ======================================= */

/* Purpose: Pins the resources that this CVMCPUALURhs may use. */
void
CVMCPUalurhsPinResource(CVMJITRMContext* con, int opcode, CVMCPUALURhs* ap,
                        CVMRMregset target, CVMRMregset avoid)
{
    switch (ap->type) {
    case CVMMIPS_ALURHS_CONSTANT:
        /* If we can't encode the constant as a immediate, then
           load it into a register and re-write the aluRhs as a 
           CVMMIPS_ALURHS_REGISTER: 
         */
        if (!CVMCPUalurhsIsEncodableAsImmediate(opcode, ap->constValue)) {
            ap->type = CVMMIPS_ALURHS_REGISTER;
            ap->r = CVMRMgetResourceForConstant32(con, target, avoid,
                                                  ap->constValue);
            ap->constValue = 0;
        }
        return;
    case CVMMIPS_ALURHS_REGISTER:
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
    case CVMMIPS_ALURHS_CONSTANT:
        return;
    case CVMMIPS_ALURHS_REGISTER:
        CVMRMrelinquishResource(con, ap->r);
        return;
    }
}

/**************************************************************
 * CPU MemSpec and associated types - The following are implementations
 * of the functions for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 *
 * NOTE: MIPS has only one load/store addressing mode:
 *
 *           base_reg + sign_extend(offset_16)
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

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMMIPSmemspecGetToken(const CVMCPUMemSpec *memSpec)
{
    int type = memSpec->type;
    if (type == CVMCPU_MEMSPEC_REG_OFFSET) {
	int regno = CVMRMgetRegisterNumber(memSpec->offsetReg);
	return CVMMIPSmemspecEncodeToken(type, regno);
    } else {
	return CVMMIPSmemspecEncodeToken(type, memSpec->offsetValue);
    }
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

static void
CVMMIPSemitBranchOnCompare32(CVMJITCompilationContext* con, 
    int logicalPC, CVMCPUCondCode cmpCondCode,
    int cmpLhsReg, int cmpRhsReg, CVMInt32 cmpRhsConst,
    CVMBool rhsIsConst, CVMBool link,
    CVMJITFixupElement** fixupList);

static void
CVMMIPSemitBranch0(CVMJITCompilationContext* con,
                  int logicalPC, int opcode,
                  int srcRegID, CVMInt32 rhsRegID,
                  CVMJITFixupElement** fixupList,
                  CVMBool nopInDelaySlot);

static void 
CVMMIPSemitBranchToTarget(CVMJITCompilationContext* con,
                          int logicalPC, int opcode,
                          int srcRegID, CVMInt32 rhsRegID,
                          CVMBool link, CVMJITFixupElement** fixupList);

static void
CVMMIPSemitJump(CVMJITCompilationContext* con,
                CVMInt32 logicalPC, CVMBool link,
                CVMBool delaySlot);

static void
CVMMIPSemitJumpRegister(CVMJITCompilationContext* con,
	         	int srcReg, CVMBool link, CVMBool delaySlot);

/* Private MIPS specific defintions: ===================================== */

/**************************************************************
 * MIPS condition code related stuff. MIPS doesn't have
 * simple 4-bit (N, Z, V, and C) condition codes like ARM and 
 * SPARC, so we can't simply mask CVMCPUCondCodes into the
 * instructions.
 **************************************************************/
/* mapped from CVMCPUCondCode defined in jitriscemitterdefs_cpu.h*/
const CVMCPUCondCode CVMCPUoppositeCondCode[] = {
    CVMCPU_COND_NE, CVMCPU_COND_EQ, CVMCPU_COND_PL, CVMCPU_COND_MI,
    CVMCPU_COND_NO, CVMCPU_COND_OV, CVMCPU_COND_GE, CVMCPU_COND_LE,
    CVMCPU_COND_GT, CVMCPU_COND_LT, CVMCPU_COND_HS, CVMCPU_COND_LS,
    CVMCPU_COND_HI, CVMCPU_COND_LO, CVMCPU_COND_NV, CVMCPU_COND_AL
};

static int getOppositeCondBranch(int branchOpcode)
{
    switch(branchOpcode) {
    case MIPS_BEQ_OPCODE:  return MIPS_BNE_OPCODE;
    case MIPS_BNE_OPCODE:  return MIPS_BEQ_OPCODE;
    case MIPS_BGEZ_OPCODE: return MIPS_BLTZ_OPCODE;
    case MIPS_BGTZ_OPCODE: return MIPS_BLEZ_OPCODE;
    case MIPS_BLEZ_OPCODE: return MIPS_BGTZ_OPCODE;
    case MIPS_BLTZ_OPCODE: return MIPS_BGEZ_OPCODE;
    case MIPS_BGEZAL_OPCODE: return MIPS_BLTZ_OPCODE;
    case MIPS_BLTZAL_OPCODE: return MIPS_BGEZ_OPCODE;
    case MIPS_BC1T_OPCODE: return MIPS_BC1F_OPCODE;
    case MIPS_BC1F_OPCODE: return MIPS_BC1T_OPCODE;
    default:
        CVMassert(CVM_FALSE);
        return -1;
    }
}

#ifdef CVM_TRACE_JIT
static const char* const conditions[] = {
    "eq", "ne", "mi", "pl", "ov", "no", "lt", "gt", 
    "le", "ge", "lo", "hi", "ls", "hs", "", "nv"
};

/* Purpose: Returns the name of the specified opcode. */
static const char *getMemOpcodeName(CVMUint32 mipsOpcode)
{
    const char *name = NULL;
    switch (mipsOpcode) {
        case MIPS_LD_OPCODE:   name = "ld"; break;
        case MIPS_SD_OPCODE:   name = "sd"; break;

        case MIPS_LW_OPCODE:   name = "lw"; break;	
        case MIPS_SW_OPCODE:   name = "sw"; break;

        case MIPS_LHU_OPCODE:  name = "lhu"; break;
        case MIPS_LH_OPCODE:   name = "lh"; break;	
        case MIPS_SH_OPCODE:   name = "sh"; break;

        case MIPS_LBU_OPCODE:  name = "lbu"; break;
        case MIPS_LB_OPCODE:   name = "lb"; break;
        case MIPS_SB_OPCODE:   name = "sb"; break;
#ifdef CVM_JIT_USE_FP_HARDWARE
        case MIPS_LWC1_OPCODE: name = "lwc1"; break;
        case MIPS_SWC1_OPCODE: name = "swc1"; break;

        case MIPS_LDC1_OPCODE: name = "ldc1"; break;
        case MIPS_SDC1_OPCODE: name = "sdc1"; break;
#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
        case MIPS_LDL_OPCODE: name = "ldl"; break;
        case MIPS_LDR_OPCODE: name = "ldr"; break;
        case MIPS_SDL_OPCODE: name = "sdl"; break;
        case MIPS_SDR_OPCODE: name = "sdr"; break;
#endif
#endif
        default:
	    CVMconsolePrintf("Unknown opcode 0x%08x", mipsOpcode);
	    CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;  
}

static const char *getALUOpcodeName(CVMUint32 mipsOpcode)
{
    const char *name = NULL;
    switch (mipsOpcode) {
    case MIPS_ADD_OPCODE:     name = "add"; break;
    case MIPS_ADDU_OPCODE:    name = "addu"; break;
    case MIPS_ADDI_OPCODE:    name = "addi"; break;
    case MIPS_ADDIU_OPCODE:   name = "addiu"; break;
    case MIPS_SUB_OPCODE:     name = "sub"; break;
    case MIPS_SUBU_OPCODE:    name = "subu"; break;
    case MIPS_AND_OPCODE:     name = "and"; break;
    case MIPS_ANDI_OPCODE:    name = "andi"; break;
    case MIPS_OR_OPCODE:      name = "or"; break;
    case MIPS_ORI_OPCODE:     name = "ori"; break;
    case MIPS_XOR_OPCODE:     name = "xor"; break;
    case MIPS_XORI_OPCODE:    name = "xori"; break;
    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", mipsOpcode);
        CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;
}

static const char *getBranchOpcodeName(CVMUint32 mipsOpcode)
{
    const char *name = NULL;
    switch (mipsOpcode) {
    case MIPS_BEQ_OPCODE:    name = "beq"; break;
    case MIPS_BNE_OPCODE:    name = "bne"; break;
    case MIPS_BGEZ_OPCODE:   name = "bgez"; break;
    case MIPS_BGEZAL_OPCODE: name = "bgezal"; break;
    case MIPS_BGTZ_OPCODE:   name = "bgtz"; break;
    case MIPS_BLEZ_OPCODE:   name = "blez"; break;
    case MIPS_BLTZ_OPCODE:   name = "bltz"; break;
    case MIPS_BLTZAL_OPCODE: name = "bltzl"; break;
    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", mipsOpcode);
        CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;
}

static const char* regName(int regID) {
    const char *name = NULL;
    switch (regID) {
    case CVMMIPS_r0:             name = "zero"; break; 
    case CVMMIPS_r1:             name = "at"; break;
    case CVMMIPS_r2:             name = "v0"; break;
    case CVMMIPS_r3:             name = "v1"; break;
    case CVMMIPS_r4:             name = "a0"; break;
    case CVMMIPS_r5:             name = "a1"; break;
    case CVMMIPS_r6:             name = "a2"; break;
    case CVMMIPS_r7:             name = "a3"; break;
    case CVMMIPS_r8:             name = "t0"; break;
    case CVMMIPS_r9:             name = "t1"; break;
    case CVMMIPS_r10:            name = "t2"; break;
    case CVMMIPS_r11:            name = "t3"; break;
    case CVMMIPS_r12:            name = "t4"; break;
    case CVMMIPS_r13:            name = "t5"; break;
    case CVMMIPS_r14:            name = "t6"; break;
    case CVMMIPS_r15:            name = "t7"; break;
    case CVMCPU_JSP_REG:         name = "JSP"; break; /* s0 */
    case CVMCPU_JFP_REG:         name = "JFP"; break; /* s1 */
    case CVMCPU_EE_REG:          name = "EE"; break;  /* s2 */
#ifdef CVMCPU_HAS_CP_REG
    case CVMCPU_CP_REG:          name = "CP"; break;  /* s3 */
#elif defined(CVMJIT_TRAP_BASED_GC_CHECKS)
    case CVMMIPS_s3:             name = "GC"; break;
#else
    case CVMMIPS_s3:             name = "s3"; break;
#endif
    case CVMMIPS_s4:             name = "s4"; break;
    case CVMMIPS_s5:             name = "s5"; break;
    case CVMMIPS_s6:             name = "s6"; break;
    case CVMMIPS_s7:             name = "s7"; break;    
    case CVMMIPS_t8:             name = "t8"; break;
    case CVMMIPS_t9:             name = "t9"; break;
    case CVMMIPS_r26:            name = "k0"; break;
    case CVMMIPS_r27:            name = "k1"; break;
    case CVMMIPS_r28:            name = "gp"; break;
    case CVMCPU_SP_REG:          name = "SP"; break;
    case CVMCPU_CHUNKEND_REG:    name = "CHUNKEND"; break;
    case CVMMIPS_r31:            name = "ra"; break;
    default:
        CVMconsolePrintf("Unknown register 0x%08x", regID);
        CVMassert(CVM_FALSE); /* Unknown register */
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

#endif

/*
 * emit one instruction to the code buffer.
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
CVMMIPSemitSLT(CVMJITCompilationContext *con, int opcode, 
               int destRegID, int lhsRegID, CVMInt32 rhs)
{
    /* rhs is either a register or an immediate, depending on
       the opcode */
    if (opcode == MIPS_SLT_OPCODE || opcode == MIPS_SLTU_OPCODE) {
        emitInstruction(con, 
            lhsRegID << MIPS_RS_SHIFT | rhs << MIPS_R_RT_SHIFT |
            destRegID << MIPS_R_RD_SHIFT | opcode);
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	%s	%s, %s, %s",
	        (opcode == MIPS_SLT_OPCODE) ? "slt" : "sltu",
                regName(destRegID), regName(lhsRegID), regName(rhs));
        });
    } else {
        CVMInt16 rhsConst = (CVMInt16)rhs;
        CVMassert(opcode == MIPS_SLTI_OPCODE || MIPS_SLTIU_OPCODE);
        emitInstruction(con,
            opcode | lhsRegID << MIPS_RS_SHIFT | 
            destRegID << MIPS_I_RD_SHIFT | 
            ((CVMUint16)rhsConst & 0xffff));
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	%s	%s, %s, %d",
                (opcode == MIPS_SLTU_OPCODE) ? "sltu" : "sltiu",
                regName(destRegID), regName(lhsRegID), rhsConst);
        });
    }
    CVMJITdumpCodegenComments(con);
}

static void
CVMMIPSemitLUI(CVMJITCompilationContext* con,
               int destRegID, CVMUint32 constant)
{
    emitInstruction(con,
		    MIPS_LUI_OPCODE | destRegID << MIPS_I_RD_SHIFT |
		    (constant & 0xffff));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	lui	%s, hi16(0x%x)\n",
			 regName(destRegID), constant);
    });
}

/*
 * Makes a 32 bit constant without using the constant pool.
 */
static void
CVMMIPSemitMakeConstant32(CVMJITCompilationContext* con,
			 int destRegID, CVMInt32 constant)
{
    /* Implemented as:

           lui    destRegID, hi16(constant)
           ori    destRegID, destRegID, lo16(constant)
     */
    CVMMIPSemitLUI(con, destRegID, constant >> 16);

    emitInstruction(con,
		    MIPS_ORI_OPCODE | destRegID << MIPS_RS_SHIFT |
		    destRegID << MIPS_I_RD_SHIFT | ((constant & 0xffff)));
    CVMtraceJITCodegenExec({
	CVMJITaddCodegenComment((con, CVMJITgetSymbolName(con)));
	printPC(con);
	CVMconsolePrintf("	ori	%s, %s, lo16(0x%x)",
			 regName(destRegID), regName(destRegID), constant);
    });
    CVMJITdumpCodegenComments(con);
}

#ifdef CVMCPU_HAS_CP_REG
/* Purpose: Set up constant pool base register */
void
CVMCPUemitLoadConstantPoolBaseRegister(CVMJITCompilationContext *con)
{
    CVMUint32 targetAddr;

    targetAddr = (CVMUint32)
	CVMJITcbufLogicalToPhysical(con, con->target.cpLogicalPC);

    CVMJITprintCodegenComment(("setup cp base register"));

    /* make the constant without using the constant pool */
    CVMMIPSemitMakeConstant32(con, CVMCPU_CP_REG, targetAddr);

    /*
     * Save the constant pool base register into the frame so it can
     * be restored if we invoke another method.
     */
    CVMJITaddCodegenComment((con, "save to frame->cpBaseRegX"));
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
			    CVMMIPSalurhsEncodeConstantToken(value),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 32k
	 * of locals, so it's ok to just not compile this method.
         */ 
	CVMJITerror(con, CANNOT_COMPILE, "too many locals");
    }
}

/*
 * Purpose: Emit Stack limit check at the start of each method and
 *          also flush the return address to the frame.
 */
void
CVMCPUemitStackLimitCheckAndStoreReturnAddr(CVMJITCompilationContext* con)
{
    const void* target = CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr;
    CVMInt32 helperLogicalPC;

    CVMJITprintCodegenComment(("Stack limit check"));

    CVMJITaddCodegenComment((con, "Store LR into frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
				       CVMMIPS_lr, CVMCPU_JFP_REG,
				       CVMoffsetof(CVMCompiledFrame, pcX));

    CVMJITaddCodegenComment((con, "check for chunk overflow"));
    CVMJITaddCodegenComment((con, "call letInterpreterDoInvoke if overflow"));

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else 
#endif
    {
        helperLogicalPC = (CVMUint8*)target - con->codeEntry;
    }

    CVMJITsetSymbolName((con, "letInterpreterDoInvoke"));
    CVMMIPSemitBranchOnCompare32(con, helperLogicalPC,
        CVMCPU_COND_LS, CVMCPU_CHUNKEND_REG, CVMCPU_ARG2_REG,
        0, CVM_FALSE, /* rhs is not a constant */
        CVM_TRUE /* link */, NULL);
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
			    CVMMIPSalurhsEncodeConstantToken(offset),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 32k
	 * of locals, so it's ok to just not compile this method.
         */ 
	CVMJITerror(con, CANNOT_COMPILE, "too many locals");
    } 
}

/*
 * Return using the correct CCM helper function.
 */
void
CVMCPUemitReturn(CVMJITCompilationContext* con, void* helper)
{
    /*
     * First set up
     * 
     *   lw     PREV, OFFSET_CVMFrame_prevX(JFP)
     *
     * This is expected at the return handler
     *
     */
    {
	CVMCodegenComment *comment;
	CVMUint32 prevOffset;
	CVMJITpopCodegenComment(con, comment);
	CVMJITaddCodegenComment((con,
	    "PREV(%s) = frame.prevX for return helper to use",
	    regName(CVMCPU_PROLOGUE_PREVFRAME_REG)));
	prevOffset = CVMoffsetof(CVMCompiledFrame, frameX.prevX);
	CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
					   CVMCPU_PROLOGUE_PREVFRAME_REG,
					   CVMCPU_JFP_REG,
					   prevOffset);
	CVMJITpushCodegenComment(con, comment);
    }

    /* branch to the helper code to do the return */
    {
	int helperLogicalPC;
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
	helperLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, helper);
#else
	helperLogicalPC = (CVMUint8*)helper - con->codeEntry;
#endif
	CVMMIPSemitBranchToTarget(con, helperLogicalPC,
            MIPS_BEQ_OPCODE, CVMMIPS_zero, CVMMIPS_zero,
	    CVM_FALSE /* don't link */, NULL);
    }
}

void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall(CVMJITCompilationContext* con,
                                             const void* target,
                                             CVMBool okToDumpCp,
                                             CVMBool okToBranchAroundCpDump)
{
    CVMInt32 returnPcOffset;
    CVMInt32 targetLogicalPC;
    CVMInt32 fixupPC;
    CVMCodegenComment *comment;
    CVMBool targetInCodeCache = CVM_FALSE;

    CVMJITpopCodegenComment(con, comment);

    /* Implemented as:
     *
     *  If the helper is not copied into codecache:
     *     lw    t9, =target
     *     lui   ra, hi16(returnPC)
     *     ori   ra, ra, lo16(returnPC)
     *     sw    JSP, topOfStack(JFP)
     *     jalr  t9
     *     sw    ra, pc(JFP)   # delay slot of jalr
     *   returnPC:
     *
     * Or:
     *
     *  If the helper is copied into codecache:
     *     lui	ra, hi16(returnPC)
     *     ori	ra, ra, lo16(returnPC)
     *     sw	JSP, topOfStack(JFP)
     *     b	target
     *     sw	ra, pc(JFP)    # delay slot of b
     */

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        targetInCodeCache = CVM_TRUE;
        targetLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else 
#endif
    {
        targetLogicalPC = (CVMUint8*)target - con->codeEntry;
    }

    if (!targetInCodeCache) {
        CVMInt32 physicalPC;
        physicalPC = (CVMInt32)CVMJITcbufLogicalToPhysical(con, 
                                   targetLogicalPC);
        /* load target address into t9 per MIPS calling convention */
        CVMJITsetSymbolName((con, "target address"));
        CVMCPUemitLoadConstant(con, CVMMIPS_t9, physicalPC);
    }

    /* load the return PC. Return PC will the fix later. */
    fixupPC = CVMJITcbufGetLogicalPC(con);
    CVMJITsetSymbolName((con, "return address"));
    CVMMIPSemitMakeConstant32(con, CVMMIPS_lr,
			      (CVMInt32)CVMJITcbufGetPhysicalPC(con));

    /* Store the JSP into the compiled frame: */
    CVMJITaddCodegenComment((con, "Store JSP into the compiled frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
        CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetof(CVMFrame, topOfStack));

    /* call the function */
    CVMJITpushCodegenComment(con, comment);
    if (targetInCodeCache) {
        CVMMIPSemitJump(con, targetLogicalPC, CVM_TRUE /* link */,
                    CVM_FALSE /* don't fill delay slot with nop */);
    } else {
        /* target address is in t9 */
        CVMMIPSemitJumpRegister(con, CVMMIPS_t9, CVM_TRUE, /* link */
                            CVM_FALSE /* don't fill delay slot with nop */);
    }

    /* In delay slot: store the return PC into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush return address to frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
				       CVMMIPS_lr, CVMCPU_JFP_REG,
				       offsetof(CVMCompiledFrame, pcX));    

    /* Fixup return address */
    returnPcOffset = CVMJITcbufGetLogicalPC(con) - fixupPC;
    CVMJITcbufPushFixup(con, fixupPC);
    CVMJITsetSymbolName((con, "fix return address"));
    CVMMIPSemitMakeConstant32(con, CVMMIPS_lr,
			      (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
			      returnPcOffset);
    CVMJITcbufPop(con);
}

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. If okToDumpCp is TRUE, the constant pool will
 * be dumped if possible. Set okToBranchAroundCpDump to FALSE if you plan
 * on generating a stackmap after calling CVMCPUemitAbsoluteCall.
 * 
 * MIPS ignores okToDumpCp and okToBranchAroundCpDump because either
 * there is no constant pool, or it is dumped once when compilation
 * is done.
 */
void
CVMCPUemitAbsoluteCall(CVMJITCompilationContext* con,
		       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump)
{
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    /*
     * Need to use fixed AOT codecache if 
     * CVM_JIT_COPY_CCMCODE_TO_CODECACHE is enabled,
     * because the 'jal' instruction is not PC-relative.
     */
#if defined(CVM_AOT) && !defined(CVMAOT_USE_FIXED_ADDRESS)
#error Need to use fixed AOT codecache if CVM_JIT_COPY_CCMCODE_TO_CODECACHE is enabled.
#endif
    CVMBool useJAL = CVM_TRUE;
    if (!okToDumpCp) {
	/*
	 * There are a few uses of !okToDumpCp that require we do not
	 * use JAL, but instead must use JALR.
	 * NOTE: This is really ugly. We should really find a better
	 * way to keep CVMCCMruntimeResolveGlue happy so it can patch.
	 */
	if (target == CVMCCMinvokeStaticSyncMethodHelper ||
	    target == CVMCCMinvokeNonstaticSyncMethodHelper ||
#ifdef CVM_TRACE_ENABLED
	    target == CVMCCMtraceMethodCallGlue ||
#endif
	    target == CVMCCMruntimeRunClassInitializerGlue)
	{
	    useJAL = CVM_FALSE;
	}
    }

    if (useJAL &&
	target >= (void*)&CVMCCMcodeCacheCopyStart &&
	target < (void*)&CVMCCMcodeCacheCopyEnd)
    {
	/* The code is in the code cache, so we can just branch to it. */
        CVMInt32 helperLogicalPC =
	    CVMCCMcodeCacheCopyHelperOffset(con, target);
        CVMMIPSemitJump(con, helperLogicalPC, CVM_TRUE /* link */,
			CVM_TRUE /* fill delay slot with nop */);
    } else 
#endif
    {
	/* Emit a function call. Use t9 as the jump register per
	 * MIPS calling convention. */
        CVMInt32 helperLogicalPC = (CVMUint8*)target - con->codeEntry;
	CVMInt32 physicalPC =
	    (CVMInt32)CVMJITcbufLogicalToPhysical(con, helperLogicalPC);
	CVMMIPSemitMakeConstant32(con, CVMMIPS_t9, physicalPC);
	CVMMIPSemitJumpRegister(con, CVMMIPS_t9, CVM_TRUE, /* link */
				CVM_TRUE /* fill delay slot */);
    }

    /* SymbolName may have been set to the name of the target */
    CVMJITresetSymbolName(con);
}

/* 
 * branch to target within the current 256 MB-aligned region
 */
static void
CVMMIPSemitJump(CVMJITCompilationContext* con,
                CVMInt32 logicalPC, CVMBool link,
                CVMBool delaySlot) 
{
    int opcode;
    const char *opcodeName;
    CVMInt32 physicalPC;
    CVMInt32 offset;
    CVMInt32 regionMask = 0xf0000000;
    CVMInt32 currRegion, targetRegion ;
    
    opcode = (link ? MIPS_JAL_OPCODE : MIPS_J_OPCODE);
    opcodeName = (link ? "jal" : "j");
   
    /* 
     * Target address =
     *      (((addr of jump delay slot) & 0xf0000000) | 
     *        (offset << 2) & 0xfffffff)
     *
     * MIPS J and JAL are a PC-region branches. The effective target
     * address is in the current 256 MB-aligned region.
     */
    currRegion = ((CVMInt32)CVMJITcbufGetPhysicalPC(con) + 
                  CVMCPU_INSTRUCTION_SIZE) & regionMask;
    physicalPC = (CVMInt32)CVMJITcbufLogicalToPhysical(con, logicalPC);
    targetRegion = physicalPC & regionMask;
    
    if (targetRegion == currRegion) {
        CVMassert((physicalPC & 0x3) == 0);

        /* target address is in the current 256 MB-aligned region */
        offset = (physicalPC & ~regionMask) >> 2;

        emitInstruction(con, opcode | 
                        ((CVMUint32)offset & 0x3ffffff));
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	%s	PC=(0x%8x)", 
                             opcodeName, physicalPC);
        });
        CVMJITdumpCodegenComments(con);

	/* Fill the delay slot with a 'nop'. */
        if (delaySlot) {
	    CVMCPUemitNop(con);
        }
    } else {
        /* Target is not within the current 256 region. */
        CVMMIPSemitMakeConstant32(con, CVMMIPS_t7, physicalPC);
        CVMMIPSemitJumpRegister(con, CVMMIPS_t7, link,
                                delaySlot /* fill delay slot */);
    }
}

/*
 * branch to the address in "srcReg".
 */
static void
CVMMIPSemitJumpRegister(CVMJITCompilationContext* con,
	         	int srcReg, CVMBool link, CVMBool delaySlot)
{
    emitInstruction(con, srcReg << MIPS_RS_SHIFT |
                    (link ? CVMMIPS_lr << MIPS_R_RD_SHIFT : 0) |
		    (link ? MIPS_JALR_OPCODE : MIPS_JR_OPCODE));
    CVMtraceJITCodegenExec({
            printPC(con);
	    CVMconsolePrintf("	%s	%s", 
                (link ? "jalr" : "jr"), regName(srcReg));
    });
    CVMJITdumpCodegenComments(con);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);

    /* Fill the delay slot with a 'nop'. */
    if (delaySlot) {
        CVMCPUemitNop(con);
    }
}

/* 
 *  Purpose: Emits code to invoke method through MB.
 *           MB is already in CVMCPU_ARG1_REG. 
 */
void
CVMCPUemitInvokeMethod(CVMJITCompilationContext* con)
{
    CVMUint32 off = CVMoffsetof(CVMMethodBlock, jitInvokerX);
    CVMRMResource *invokerXRes =
        CVMRMgetResource(CVMRM_INT_REGS(con),
                         CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    int invokerXReg = CVMRMgetRegisterNumber(invokerXRes);

    CVMJITaddCodegenComment((con, "call method through mb"));

    /* get the address of the method. */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
				       invokerXReg, CVMCPU_ARG1_REG, off);
    /* call it */
    /* Note: No need to set up t9, because the glue code will
     *       set up jp(t9) if the method is JNI/CNI/uncompiled.
     */
    CVMMIPSemitJumpRegister(con, invokerXReg, CVM_TRUE /* link */,
                            CVM_TRUE /* fill delay slot */);

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), invokerXRes);
}

/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the LR is a GPR.
 */
void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno)
{
    CVMassert(regno == CVMMIPS_lr);
}

/*
 * Branch to the address in the specified register.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno)
{
    /* Note: No need to set up t9 since this is only used in 'RET'.
     *       We know we are not calling out to a C function in this
     *       case.
     */
    CVMMIPSemitJumpRegister(con, regno, CVM_FALSE /* don't link */,
                            CVM_TRUE /* fill delay slot */);
}

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The table entry for index 0 will be generated right after
 * any instructions that are generated here.
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int keyReg)
{
    /* This is implemented as (not PC relative):
     *
     *     lw    reg2, =tablePC
     *     sll   reg1, key, 3
     *     add   reg1, reg1, reg2
     *     jr    reg1
     *     nop
     *   tablePC:
     *
     * or when CVM_AOT is enabled (PC relative):
     *
     *     # We need to do 'sll' first in case 'key' is in 'ra'. This
     *     # is based on the fact that 'key' will be discarded after
     *     # CVMCPUemitTableSwitchBranch() returns (in TABLESWITCH
     *     # rule). If that changes, the follow won't work because
     *     # 'bal' will trash the 'key' if it's in 'ra' register.
     *     sll   reg1, key, 3
     *     bal   nextLabel      # skip the delay slot. ra is 'nextLabel'
     *     add   reg1, reg1, 12 # distance from 'nextLabel' to tablePC
     *   nextLabel:
     *     add   reg1, reg1, ra # ra is 'nextLabel'
     *     jr    reg1
     *     nop
     *   tablePC:
     */
#undef NUM_TABLESWITCH_INSTRUCTIONS
#ifdef CVMCPU_HAS_CP_REG
#define NUM_TABLESWITCH_INSTRUCTIONS 5
#else
#define NUM_TABLESWITCH_INSTRUCTIONS 6
#endif
    /* Avoid CVMMIPS_lr for tmpResource1 because CVMMIPS_lr is used as
     * tmpReg2 if CVM_AOT is enabled.
     */
    CVMRMResource *tmpResource1 = CVMRMgetResourceStrict(CVMRM_INT_REGS(con),
                      (CVMRM_ANY_SET & ~(1U << CVMMIPS_lr)),
                      (1U << CVMMIPS_lr), 1);
    CVMUint32 tmpReg1 = CVMRMgetRegisterNumber(tmpResource1);
    CVMUint32 tmpReg2; /* tmpReg2 is 'ra' if CVM_AOT is enabled */

#ifndef CVM_AOT
    CVMUint32 tablePC = (CVMUint32)CVMJITcbufGetPhysicalPC(con) +
        NUM_TABLESWITCH_INSTRUCTIONS * CVMCPU_INSTRUCTION_SIZE;
    CVMRMResource *tmpResource2 = CVMRMgetResource(CVMRM_INT_REGS(con),
                                    CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    tmpReg2 = CVMRMgetRegisterNumber(tmpResource2);

    /* get the address of the table: */
#ifdef CVMCPU_HAS_CP_REG
    CVMCPUemitLoadConstant(con, tmpReg2, tablePC);
#else
    /* We can't use CVMCPUemitLoadConstant, because it might emit 1 or 2
     * instructions, and we need to know the number ahread of time. */
    CVMMIPSemitMakeConstant32(con, tmpReg2, tablePC);
#endif
#endif

    /* shift the key by 3, because every table element branch has a nop as
       the branch delay slot */
    CVMCPUemitShiftByConstant(con, CVMCPU_SLL_OPCODE, 
                              tmpReg1, keyReg, 3);

#ifdef CVM_AOT
    /* emit the bal instruction, 'ra' register will have pc+8 
     * after the branch. */
    CVMMIPSemitBranch0(con, 
        CVMJITcbufGetLogicalPC(con) + 2*CVMCPU_INSTRUCTION_SIZE, 
        MIPS_BGEZAL_OPCODE, CVMMIPS_zero, CVMCPU_INVALID_REG,
        NULL, CVM_FALSE);
    tmpReg2 = CVMMIPS_lr;
    /* branch target address += 3*CVMCPU_INSTRUCTION_SIZE */
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                                tmpReg1, tmpReg1,
                                3*CVMCPU_INSTRUCTION_SIZE,
                                CVMJIT_NOSETCC);
#endif

    /* compute the address to branch to: */
    CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                                tmpReg1, tmpReg1, tmpReg2,
                                CVMJIT_NOSETCC);

    /* emit the jr instruction: */
    CVMMIPSemitJumpRegister(con, tmpReg1, CVM_FALSE /* don't link */,
                            CVM_TRUE /* fill delay slot */);

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource1);
#ifndef CVM_AOT
    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource2);
#endif
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
    CVMUint32 sourcePC = (CVMUint32)instrAddr;
    CVMUint32 targetPC = sourcePC + offset;
    const CVMInt32 regionMask = 0xf0000000;

    CVMassert(CVMglobals.jit.pmiEnabled);

    /* We should never run into a case where instrAddr and instrAddr+offset
       are not located in the same 256mb region, since PMI is disabled
       if the code cache does not fit entirely in one 256mb region,
       or if we are not copying ccmglue to the start of the codecache.
    */
    CVMassert((sourcePC & regionMask) == (targetPC & regionMask));

    /* There better already be an unconditional jal or bal at this address */
    CVMassert((*(CVMUint32*)instrAddr & ~0x03ffffff) == MIPS_JAL_OPCODE ||
              (*(CVMUint32*)instrAddr & ~0x0000ffff) == 
              ((0x1 << 26) | MIPS_BGEZAL_OPCODE));

    /* Patch with a bal (bgezal r0,r0) if within range. Otherwise jal. */
    if  (offset <= CVMMIPS_MAX_BRANCH_OFFSET &&
         offset >= CVMMIPS_MIN_BRANCH_OFFSET)
    {
        /* The bal instruction branches based on the offset of PC+4, so we
           need to adjust it. Note the MIN and MAX values above assume
           that the adjustment has not been made yet. */
        offset -= CVMCPU_INSTRUCTION_SIZE;
        /* patch with bal */
        branch = (0x1 << 26) | MIPS_BGEZAL_OPCODE | ((offset >> 2) & 0xffff);
    } else {
        /* patch with jal */
        branch = MIPS_JAL_OPCODE | ((targetPC & ~regionMask) >> 2);
    }
    *((CVMCPUInstruction*)instrAddr) = branch;
}

#endif

/* Helper that emits branch to reachable target */
static void
CVMMIPSemitBranch0(CVMJITCompilationContext* con,
                  int logicalPC, int opcode,
                  int srcRegID, CVMInt32 rhsRegID,
                  CVMJITFixupElement** fixupList,
                  CVMBool nopInDelaySlot)
{
    CVMInt32 realoffset;
    
    CVMassert(opcode == MIPS_BEQ_OPCODE ||
              opcode == MIPS_BNE_OPCODE ||
              opcode == MIPS_BGEZ_OPCODE ||
              opcode == MIPS_BGEZAL_OPCODE ||
              opcode == MIPS_BGTZ_OPCODE ||
              opcode == MIPS_BLEZ_OPCODE ||
              opcode == MIPS_BLTZ_OPCODE ||
              opcode == MIPS_BLTZAL_OPCODE ||
              opcode == MIPS_BC1T_OPCODE ||
              opcode == MIPS_BC1F_OPCODE);

    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }

    realoffset = (logicalPC - CVMJITcbufGetLogicalPC(con) - 
                           CVMCPU_INSTRUCTION_SIZE) >> 2;
    CVMassert((realoffset & 0xffff8000) == 0 ||
              (realoffset & 0xffff8000) == 0xffff8000);

    switch (opcode) {
    case MIPS_BEQ_OPCODE:
    case MIPS_BNE_OPCODE:
        emitInstruction(con,
            opcode | srcRegID << MIPS_RS_SHIFT | 
            rhsRegID << MIPS_I_RD_SHIFT | 
            ((CVMUint32)realoffset & 0xffff));
        break;
    case MIPS_BGTZ_OPCODE:
    case MIPS_BLEZ_OPCODE:
        emitInstruction(con,
            opcode | srcRegID << MIPS_RS_SHIFT |
            ((CVMUint32)realoffset & 0xffff));
        break;
    case MIPS_BGEZ_OPCODE:
    case MIPS_BGEZAL_OPCODE:
    case MIPS_BLTZ_OPCODE:
    case MIPS_BLTZAL_OPCODE:
        emitInstruction(con,
            (0x1 << 26) | srcRegID << MIPS_RS_SHIFT | opcode |
            ((CVMUint32)realoffset & 0xffff));
        break;
    case MIPS_BC1T_OPCODE:
        emitInstruction(con,
            MIPS_BC1T_OPCODE | ((CVMUint32)realoffset & 0xffff));
        break;
    case MIPS_BC1F_OPCODE:
        emitInstruction(con,
            MIPS_BC1F_OPCODE | ((CVMUint32)realoffset & 0xffff));
        break;
    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", opcode);
        CVMassert(CVM_FALSE);
    }

    CVMtraceJITCodegenExec({
        printPC(con);
	if (srcRegID == CVMMIPS_zero && rhsRegID == CVMMIPS_zero &&
	    (opcode == MIPS_BEQ_OPCODE || opcode == MIPS_BGEZAL_OPCODE)) {
            CVMconsolePrintf("	%s	PC=0x%8x",
                             opcode == MIPS_BEQ_OPCODE ? "b" : "bal",
			     CVMJITcbufLogicalToPhysical(con, logicalPC));
	} else if (opcode == MIPS_BEQ_OPCODE || opcode == MIPS_BNE_OPCODE) {
            CVMconsolePrintf("	%s	%s, %s, PC=0x%8x",
                getBranchOpcodeName(opcode),
                regName(srcRegID), regName(rhsRegID),
                CVMJITcbufLogicalToPhysical(con, logicalPC));
        } else if (opcode == MIPS_BGTZ_OPCODE ||
                   opcode == MIPS_BLEZ_OPCODE ||
                   opcode == MIPS_BGEZ_OPCODE ||
                   opcode == MIPS_BGEZAL_OPCODE ||
                   opcode == MIPS_BLTZ_OPCODE ||
                   opcode == MIPS_BLTZAL_OPCODE) {
            CVMconsolePrintf("	%s	%s, PC=0x%8x",
                getBranchOpcodeName(opcode), regName(srcRegID),
                CVMJITcbufLogicalToPhysical(con, logicalPC));
        } else {
	    CVMconsolePrintf("	%s, PC=0x%8x",
                (opcode == MIPS_BC1T_OPCODE ? "bc1t" : "bc1f"),
                CVMJITcbufLogicalToPhysical(con, logicalPC));
        }
    });
    CVMJITdumpCodegenComments(con);

    /* nop in branch delay slot */
    if (nopInDelaySlot) {
        CVMCPUemitNop(con);
    }
}

/*
 * Emit branch to target. If the target is not reachable, then
 * emit a conditional branch around the unconditional 'j target'.
 */
static void
CVMMIPSemitBranchToTarget(CVMJITCompilationContext* con,
                          int logicalPC, int opcode,
                          int srcRegID, CVMInt32 rhsRegID,
                          CVMBool link, CVMJITFixupElement** fixupList)
{
    CVMInt32 realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);
    CVMBool reachable = (realoffset <= CVMMIPS_MAX_BRANCH_OFFSET &&
                         realoffset >= CVMMIPS_MIN_BRANCH_OFFSET);

    if (link) {
        /* MIPS has two conditional branch and link instructions:
         * BLTZAL and BGEZAL.
         */ 
        if (opcode == MIPS_BLTZ_OPCODE) {
            opcode = MIPS_BLTZAL_OPCODE; /* branch on < 0 and link */
        } else if (opcode == MIPS_BGEZ_OPCODE) {
            opcode = MIPS_BGEZAL_OPCODE; /* branch on >= 0 and link */
        }
  
        /* If link is required and the opcode is neither BLTZAL nor
         * BGEZAL, we use jal and branch around the jal with the opposite
         * conditional code.
         */
        if (opcode != MIPS_BGEZAL_OPCODE || opcode != MIPS_BLTZAL_OPCODE) {
            reachable = CVM_FALSE;
        }
    }

    if (reachable) {
        CVMMIPSemitBranch0(con, logicalPC, opcode, srcRegID,
                           rhsRegID, fixupList, CVM_TRUE);
    } else if (srcRegID == CVMMIPS_zero && rhsRegID == CVMMIPS_zero &&
	       (opcode == MIPS_BEQ_OPCODE || opcode == MIPS_BGEZAL_OPCODE)) {
	/* This is an unreachable unconditional banch, so do a jump */
        CVMMIPSemitJump(con, logicalPC, link, CVM_TRUE /* fill delay slot */);
    } else  {
	/* This is an unreachable conditional branch, so emit a short
	 * conditional branch around and unconditional jump, using the
	 * opposite condition for the conditional branch.
	 */
        int oppositeCondBranch = getOppositeCondBranch(opcode);
	CVMInt32 startPC = CVMJITcbufGetLogicalPC(con);
	CVMInt32 endPC;
	CVMCodegenComment *comment;
	
	CVMJITpopCodegenComment(con, comment);
	CVMJITaddCodegenComment((con, "goto .skip"));
        CVMMIPSemitBranch0(con, 0, oppositeCondBranch,
                           srcRegID, rhsRegID, NULL, CVM_TRUE);
	CVMJITpushCodegenComment(con, comment);
        CVMMIPSemitJump(con, logicalPC, link, CVM_TRUE /* fill delay slot */);

	endPC = CVMJITcbufGetLogicalPC(con);
	CVMJITcbufPushFixup(con, startPC);
 	CVMJITaddCodegenComment((con, "goto .skip"));
	CVMMIPSemitBranch0(con, endPC, oppositeCondBranch,
			   srcRegID, rhsRegID, NULL, CVM_TRUE);
	CVMJITcbufPop(con);
	CVMtraceJITCodegen(("\t\t.skip\n"));				    \
    }
}

/* 
 * Emit branch based on the result of 64-bit comparsion.
 *
 * link:    if true, then a branch-and-link is done.
 */
static void 
CVMMIPSemitBranchOnCompare64(CVMJITCompilationContext* con, 
    int logicalPC, CVMCPUCondCode cmpCondCode,
    int cmpLhsReg, int cmpRhsReg, CVMBool link,
    CVMJITFixupElement** fixupList)
{
    int tmp;
    int hi_lhs = HIREG(cmpLhsReg);
    int low_lhs = LOREG(cmpLhsReg);
    int hi_rhs = HIREG(cmpRhsReg);
    int low_rhs = LOREG(cmpRhsReg);

    CVMInt32 fixupPC;

    switch (cmpCondCode) {
    case CVMCPU_COND_EQ:
       /*   Reachable
        *     bne hi_lhs, hi_rhs, _skip
        *     nop
        *     beq low_lhs, low_rhs, target
        *     nop
        *  _skip:
        * 
        *   Not reachable    
        *     bne hi_lhs, hi_rhs, _skip
        *     nop
        *     bne low_lhs, low_rhs, _skip
        *     nop
        *     j target
        *     nop
        *  _skip:
        */
        fixupPC = CVMJITcbufGetLogicalPC(con);
        CVMMIPSemitBranch0(con, 0, MIPS_BNE_OPCODE,
                           hi_lhs, hi_rhs, NULL, CVM_TRUE);
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BEQ_OPCODE,
                           low_lhs, low_rhs, link, fixupList);
        CVMJITfixupAddress(con, fixupPC, CVMJITcbufGetLogicalPC(con),
                           CVMJIT_COND_BRANCH_ADDRESS_MODE);
        break;
    case CVMCPU_COND_NE:
        /* CVMCPU_COND_NE:
         *   Reachable
         *     bne hi_lhs, hi_rhs, target
         *     nop
         *     bne low_lhs, low_rhs, target
         *     nop
         *  _skip:
         *
         *   Not Reachable
         *     beq hi_lhs, hi_rhs, _skip1
         *     nop
         *     j target                 # hi_lhs != hi_rhs
         *     nop
         *  _skip1:
         *     beq low_lhs, low_rhs, _skip2
         *     nop
         *     j target
         *     nop
         *  _skip2:
         */
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                  hi_lhs, hi_rhs, link, fixupList);
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                  low_lhs, low_rhs, link, fixupList);
        break;
    case CVMCPU_COND_GT:
        /* switch lhs and rhs, fall through to CVMCPU_COND_LT case */
        tmp = hi_lhs;
        hi_lhs = hi_rhs;
        hi_rhs = tmp;
        tmp = low_lhs;
        low_lhs = low_rhs;
        low_rhs = tmp;
    case CVMCPU_COND_LT:
        /*   Reachable
         *     slt scratch, hi_lhs, hi_rhs
         *     bne scratch, zero, target
         *     nop
         *     bne hi_lhs, hi_rhs, _skip # must be >, skip
         *     nop
         *     sltu scratch, low_lhs, low_rhs # hi are =, compare low
         *     bne scratch, zero, target
         *     nop
         *  _skip:
         *
         *   Not reachable
         *     slt scratch, hi_lhs, hi_rhs
         *     beq scratch, zero, _skip1     
         *     nop
         *     j target
         *     nop
         *  _skip1:                      # hi_lhs >= hi_rhs
         *     bne hi_lhs, hi_rhs, _skip2
         *     sltu scratch, low_lhs, low_rhs
         *     beq scratch, zero, _skip2
         *     nop
         *     j target
         *     nop
         *  _skip2: 
         */
        CVMMIPSemitSLT(con, MIPS_SLT_OPCODE, CVMMIPS_t7, hi_lhs, hi_rhs);
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                  CVMMIPS_t7, CVMMIPS_zero, link,
                                  fixupList);
        fixupPC = CVMJITcbufGetLogicalPC(con);
        CVMMIPSemitBranch0(con, 0, MIPS_BNE_OPCODE, 
                           hi_lhs, hi_rhs, NULL, CVM_TRUE);
        CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE, CVMMIPS_t7, low_lhs, low_rhs);
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                  CVMMIPS_t7, CVMMIPS_zero, link,
                                  fixupList);
        CVMJITfixupAddress(con, fixupPC, CVMJITcbufGetLogicalPC(con),
                           CVMJIT_COND_BRANCH_ADDRESS_MODE);
        break;
    case CVMCPU_COND_LE:
        /* switch lhs and rhs, fall through to CVMCPU_COND_GE case */
        tmp = hi_lhs;
        hi_lhs = hi_rhs;
        hi_rhs = tmp;
        tmp = low_lhs;
        low_lhs = low_rhs;
        low_rhs = tmp;
    case CVMCPU_COND_GE:
        /*   Reachable
         *     slt scratch, hi_lhs, hi_rhs
         *     bne scratch, zero _skip
         *     nop
         *     # if we are here, hi_lhs >= hi_rhs
         *     bne hi_lhs, hi_rhs, target # must be >, branch to target
         *     nop
         *     # if we are here, hi_lhs = hi_rhs
         *     sltu scratch, low_lhs, low_rhs
         *     beq scratch, zero, target
         *     nop
         *  _skip:
         *
         *   Not reachable
         *     slt scratch, hi_lhs, hi_rhs
         *     bne scratch, zero, _skip         
         *     nop
         *     # if we are here, hi_lhs >= hi_rhs
         *     beq hi_lhs, hi_rhs, _skip2
         *     nop
         *     j target
         *     nop
         *  _skip2:
         *     # if we are here, hi_lhs = hi_rhs
         *     sltu scratch, low_lhs, low_rhs
         *     bne scratch, zero, _skip           # low_lsh < low_rhs
         *     nop
         *     j target
         *     nop
         *  _skip:
         */
         CVMMIPSemitSLT(con, MIPS_SLT_OPCODE, CVMMIPS_t7, hi_lhs, hi_rhs);
         fixupPC = CVMJITcbufGetLogicalPC(con);
         CVMMIPSemitBranch0(con, 0, MIPS_BNE_OPCODE, 
                            CVMMIPS_t7, CVMMIPS_zero, 
                            NULL, CVM_TRUE);
         CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                   hi_lhs, hi_rhs, link, fixupList);
         CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE, CVMMIPS_t7, low_lhs, low_rhs);
         CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BEQ_OPCODE,
                                   CVMMIPS_t7, CVMMIPS_zero, link,
                                   fixupList);
         CVMJITfixupAddress(con, fixupPC, CVMJITcbufGetLogicalPC(con),
                            CVMJIT_COND_BRANCH_ADDRESS_MODE);
         break;
    default:
        CVMassert(CVM_FALSE);
    }
}

/*
 * Emit branch based on the result of 32-bit comparsion.
 *
 * link:    if true, then a branch-and-link is done.
 */
static void
CVMMIPSemitBranchOnCompare32(CVMJITCompilationContext* con, 
    int logicalPC, CVMCPUCondCode cmpCondCode,
    int cmpLhsReg, int cmpRhsReg, CVMInt32 cmpRhsConst,
    CVMBool rhsIsConst, CVMBool link,
    CVMJITFixupElement** fixupList)
{
    int branchOpcode;
    CVMUint32 sltOpcode;
    int tmp;
    CVMCodegenComment* comment;

    /* We may need to load a constant or do a compare before the branch, so
     * save away any pushed comment.
     */
    CVMJITpopCodegenComment(con, comment);

    if (rhsIsConst && 
        !CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_CMP_OPCODE, cmpRhsConst)) {
        CVMCPUemitLoadConstant(con, CVMMIPS_t7, cmpRhsConst);
        cmpRhsReg = CVMMIPS_t7;
        rhsIsConst = CVM_FALSE;
    }

    /* From now on, the rhsConst is encodable */

    if (rhsIsConst && (cmpCondCode == CVMCPU_COND_EQ || 
                       cmpCondCode == CVMCPU_COND_NE ||
                       cmpCondCode == CVMCPU_COND_GT || 
                       cmpCondCode == CVMCPU_COND_HI ||
                       cmpCondCode == CVMCPU_COND_LE || 
                       cmpCondCode == CVMCPU_COND_LS)) {
        /* Can't handle rhs as a constant */
        if (cmpRhsConst == 0) {
            cmpRhsReg = CVMMIPS_zero;
        } else {
	    CVMassert(CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_CMP_OPCODE, 
                                                         cmpRhsConst));
            CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                CVMMIPS_t7, CVMMIPS_zero, cmpRhsConst, CVMJIT_NOSETCC);
            cmpRhsReg = CVMMIPS_t7;
        }
        rhsIsConst = CVM_FALSE;
    }

    CVMJITpushCodegenComment(con, comment);

    switch (cmpCondCode) {
    case CVMCPU_COND_EQ: /* equal */
    case CVMCPU_COND_NE: /* not equal */
        CVMassert(!rhsIsConst);
        branchOpcode = ((cmpCondCode ==  CVMCPU_COND_EQ) ? 
                        MIPS_BEQ_OPCODE : MIPS_BNE_OPCODE);
        CVMMIPSemitBranchToTarget(con, logicalPC, branchOpcode,
                                  cmpLhsReg, cmpRhsReg, link,
                                  fixupList);
        break;
    case CVMCPU_COND_GT: /* signed greater than */
    case CVMCPU_COND_HI: /* unsigned greater than */ 
        /* switch lhs and rhs, fall through to LT/LO case */
        CVMassert(!rhsIsConst);
        tmp = cmpLhsReg;
        cmpLhsReg = cmpRhsReg;
        cmpRhsReg = tmp;
    case CVMCPU_COND_LT: /* signed less than */
    case CVMCPU_COND_LO: /* unsigned less than */
        /* Reachable
         *    slt tmp, lhs, rhs
         *    bne tmp, zero, target
         *    nop
         *
         * Not reachable
         *    slt tmp, lhs, rhs
         *    beq tmp, zero, skip
         *    nop
         *    j target
         *    nop
         *  _skip:
         */
        if (cmpCondCode == CVMCPU_COND_LT || 
            cmpCondCode == CVMCPU_COND_GT) {
            sltOpcode = rhsIsConst ? MIPS_SLTI_OPCODE :  MIPS_SLT_OPCODE;
        } else {
            sltOpcode = rhsIsConst ? MIPS_SLTIU_OPCODE: MIPS_SLTU_OPCODE;
        }
	{
	    CVMCodegenComment* comment;
	    CVMJITpopCodegenComment(con, comment);
	    CVMMIPSemitSLT(con, sltOpcode, CVMMIPS_t7, cmpLhsReg,
			   (rhsIsConst ? cmpRhsConst : cmpRhsReg));
	    CVMJITpushCodegenComment(con, comment);
	}
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BNE_OPCODE,
                                  CVMMIPS_t7, CVMMIPS_zero, link,
                                  fixupList);
        break;
    case CVMCPU_COND_LE: /* signed less than or equal */
    case CVMCPU_COND_LS: /* unsgined less than or equal */
        /* switch lhs and rhs, fall through to GE/HS case */
        CVMassert(!rhsIsConst);
        tmp = cmpLhsReg;
        cmpLhsReg = cmpRhsReg;
        cmpRhsReg = tmp;
    case CVMCPU_COND_GE: /* signed greater than or equal */
    case CVMCPU_COND_HS: /* unsigned greater than or equal */
        /* Reachable
         *     slt tmp, lhs, rhs
         *     beq tmp, zero, target
         *     nop
         *
         * Not reachable
         *     slt tmp, lhs, rhs
         *     bne tmp, zero, _skip
         *     nop
         *     j target
         *     nop
         *  _skip:
         */
        if (cmpCondCode == CVMCPU_COND_GE ||
            cmpCondCode == CVMCPU_COND_LE) {
            sltOpcode = rhsIsConst ? MIPS_SLTI_OPCODE : MIPS_SLT_OPCODE;
        } else {
            sltOpcode = rhsIsConst ? MIPS_SLTIU_OPCODE : MIPS_SLTU_OPCODE;
        }
	{
	    CVMCodegenComment* comment;
	    CVMJITpopCodegenComment(con, comment);
	    CVMMIPSemitSLT(con, sltOpcode, CVMMIPS_t7, cmpLhsReg,
			   rhsIsConst ? cmpRhsConst : cmpRhsReg);
	    CVMJITpushCodegenComment(con, comment);
	}
        CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BEQ_OPCODE,
                                  CVMMIPS_t7, CVMMIPS_zero, link,
                                  fixupList);
        break;
    case CVMCPU_COND_MI: /* negative */        
    case CVMCPU_COND_PL: /* positive */
    case CVMCPU_COND_OV: /* overflowed */
    case CVMCPU_COND_NO: /* not overflowed */
	CVMassert(CVM_FALSE); /* Condition code not implemented */
        break;
    default:
        CVMassert(CVM_FALSE);
    }
}

void
CVMMIPSemitBranch(CVMJITCompilationContext* con, int logicalPC,
                  CVMCPUCondCode condCode, CVMBool link,
                  CVMJITFixupElement** fixupList)
{
    int cmpOpcode;
    int cmpRhsType;
    int cmpLhsReg, cmpRhsReg;
    CVMInt32 cmpRhsConst;
    CVMCPUCondCode cmpCondCode;
    CVMJITTargetCompilationContext *targetCon = &(con->target);

    /*
     * A logicalPC of 0 means a forward branch we will resolve later. Make
     * the realoffset 0 in this case so we know it is reachable. If it is
     * not reachable at fixup time, we will simply fail to compiled the 
     * method.
     */
    if (logicalPC == 0) {
	logicalPC = CVMJITcbufGetLogicalPC(con);
    }

#ifdef CVM_JIT_USE_FP_HARDWARE
    if (condCode >= CVMMIPS_COND_FFIRST) {
        if ((condCode & ~CVMCPU_COND_UNORDERED_LT) == CVMCPU_COND_FNE){
            CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BC1F_OPCODE,
                CVMCPU_INVALID_REG, CVMCPU_INVALID_REG, link, fixupList);
        } else {
            CVMMIPSemitBranchToTarget(con, logicalPC, MIPS_BC1T_OPCODE,
                CVMCPU_INVALID_REG, CVMCPU_INVALID_REG, link, fixupList);
        }
        return;
    }
#endif

    /* If it is an unconditional branch, simply emit the instruction. */
    if (condCode == CVMCPU_COND_AL) {
        CVMInt32 realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);
        CVMBool reachable = (realoffset <= CVMMIPS_MAX_BRANCH_OFFSET &&
                             realoffset >= CVMMIPS_MIN_BRANCH_OFFSET);
        if (reachable) {
            CVMMIPSemitBranch0(con, logicalPC,
                (link ? MIPS_BGEZAL_OPCODE : MIPS_BEQ_OPCODE),
                CVMMIPS_zero, CVMMIPS_zero, fixupList, CVM_TRUE);
        } else {
            CVMMIPSemitJump(con, logicalPC, link,
            CVM_TRUE /* fill delay slot */);
	}
        return;
    }

    /* If we are here, then it is a conditional branch.
     * On MIPS, comparsion is defered until we do conditional 
     * branch/jump, since MIPS has no compare instructions. The
     * arguments of comparsion were stored in CompilationContext
     * by the PrepareToBranch varieties.
     */
    cmpOpcode = targetCon->cmpOpcode;
    cmpLhsReg = targetCon->cmpLhsRegID;
    cmpCondCode = targetCon->cmpCondCode;
    cmpRhsType = targetCon->cmpRhsType;
    cmpRhsReg = targetCon->cmpRhsRegID;
    cmpRhsConst = targetCon->cmpRhsConst;

    CVMassert(cmpOpcode == CVMCPU_CMP_OPCODE || 
              cmpOpcode == CVMCPU_CMN_OPCODE ||
              cmpOpcode == CVMCPU_CMP64_OPCODE);

    /* 
     * To support conditional instructions on MIPS, such as conditional
     * ALU, conditional load/store, and conditional jump, a conditional 
     * branch is emitted to branch around the unconditional instruction.
     * See emitConditionalInstructions macro in risc jitemitter.c. So
     *     (condCode == cmpCondCode || 
     *      condCode == CVMCPUoppositeCondCode[condCode])
     */
    CVMassert(condCode == cmpCondCode || 
              condCode == CVMCPUoppositeCondCode[cmpCondCode]);

    if (cmpOpcode == CVMCPU_CMP64_OPCODE) {
        /* The rhs of a 64-bit compare is always a register */
        CVMassert(cmpRhsType == CVMMIPS_ALURHS_REGISTER);
        CVMMIPSemitBranchOnCompare64(con, logicalPC, condCode, 
                                     cmpLhsReg, cmpRhsReg, link,
                                     fixupList);
    } else {
        /* 
         * If the opcode is CMN (compare negative), take the
         * negative value of the rhs.
         */
        if (cmpOpcode == CVMCPU_CMN_OPCODE) {
            if (cmpRhsType == CVMMIPS_ALURHS_REGISTER) {
                CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                                        CVMMIPS_t7, CVMMIPS_zero, cmpRhsReg,
                                        CVMJIT_NOSETCC);

                cmpRhsReg = CVMMIPS_t7;
            } else {
                cmpRhsConst = -cmpRhsConst;
            }
        }
        CVMMIPSemitBranchOnCompare32(con, logicalPC, condCode,
            cmpLhsReg, cmpRhsReg, cmpRhsConst,
            (cmpRhsType == CVMMIPS_ALURHS_CONSTANT),
            link, fixupList);
    }
}

/* Purpose: Emits instructions to do the specified 32 bit unary ALU
 *          operation.
 * Note:    There is no setcc support on MIPS ALU instructions.
 */
void
CVMCPUemitUnaryALU(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int srcRegID, CVMBool setcc)
{
    switch (opcode) {
    case CVMCPU_NEG_OPCODE: {
        CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                                    destRegID, CVMMIPS_zero, srcRegID,
                                    setcc);
        break;
    }
    case CVMCPU_NOT_OPCODE: {
        /* Unsigned Compare: x < 1? 1 : 0. 
           Since only 0 is less 1, x == 0 yields a 1, and
           all other values of x yields a 0. */
        CVMMIPSemitSLT(con, MIPS_SLTIU_OPCODE, 
                       destRegID, srcRegID, 0x1);
        break;
    }
    case CVMCPU_INT2BIT_OPCODE: {
        /* Unsigned Compare: 0 < x? 1 : 0. 
           Since only 0 is less than all non-0 values, x == 0 yields a 0, and
           all other values of x yields a 1. */
        CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE, 
                       destRegID, CVMMIPS_zero, srcRegID);
        break;
    }
    default:
        CVMassert(CVM_FALSE);
    }
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits a div or rem. */
static void
CVMMIPSemitDivOrRem(CVMJITCompilationContext* con,
		   int opcode, int destRegID, int lhsRegID,
		   CVMCPUALURhsToken rhsToken)
{
    int rhsRegID = CVMMIPSalurhsEncodeRegisterToken(rhsToken);
    int doDivLogicalPC, divDoneLogicalPC;

    CVMassert(CVMMIPSalurhsDecodeGetTokenType(rhsToken) ==
	      CVMMIPS_ALURHS_REGISTER);

    /* 
     * We could support the following by allocating a temp register, but
     * it doesn't appear to be necessary.
     */
    CVMassert(destRegID != rhsRegID);

    /*
     *      @ integer divide
     *
     *      @ check for / by 0
     *      beq    rhsRegID, zero, CVMCCMruntimeThrowDivideByZeroGlue
     *      nop
     *      bgtz   rhsRegID, .doDiv @ shortcut around 0x80000000 / -1 check
     *      nop
     *
     *      @ 0x80000000 / -1 check
     *      addiu  destRegID, rhsRegID, -1
     *      bne    destRegID, zero, .doDiv
     *      nop
     *      lui    destRegID, 0x8000
     *      beq    destRegID, lhsRegID, .divDone
     *      nop
     * .doDiv:
     *      div    lhsRegID, rhsRegID
     *      mflo/mfhi   destRegID
     * .divDone:
     *
     */

    CVMJITdumpCodegenComments(con);
    CVMJITprintCodegenComment(("Do integer %s:",
        opcode == CVMMIPS_DIV_OPCODE ? "divide" : "remainder"));

    /* move lhsRegID into CVMMIPS_t7 if lhsRegID == destRegID. Otherwise
     * it will be clobbered before used. */
    if (lhsRegID == destRegID) {
	CVMJITaddCodegenComment((con, "avoid clobber when lshReg == destReg"));
	CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			       CVMMIPS_t7, lhsRegID, CVMJIT_NOSETCC);
	lhsRegID = CVMMIPS_t7;
    }

    /* compare for rhsRegID % 0 */
    CVMJITaddCodegenComment((con, "check for / by 0"));
    CVMCPUemitCompareConstant(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_EQ,
			      rhsRegID, 0);
    CVMJITaddCodegenComment((con, 
         "call CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMJITsetSymbolName((con, "CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMCPUemitAbsoluteCallConditional(con, CVMCCMruntimeThrowDivideByZeroGlue,
				      CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK,
				      CVMCPU_COND_EQ);

    doDivLogicalPC = CVMJITcbufGetLogicalPC(con) + 
	CVMCPU_INSTRUCTION_SIZE * 8;
    divDoneLogicalPC = doDivLogicalPC + CVMCPU_INSTRUCTION_SIZE * 3;

    /* shortcut around compare for 0x80000000 % -1 */
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMMIPSemitBranch0(con, doDivLogicalPC, MIPS_BGTZ_OPCODE,
                       rhsRegID, CVMCPU_INVALID_REG, NULL, CVM_TRUE);

    /* compare for 0x80000000 / -1 */
    CVMJITaddCodegenComment((con, "0x80000000 / -1 check:"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, destRegID, rhsRegID,
                                -1, CVMJIT_NOSETCC);
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMMIPSemitBranch0(con, doDivLogicalPC, MIPS_BNE_OPCODE, 
                      destRegID, CVMMIPS_zero, NULL, CVM_TRUE);
    CVMMIPSemitLUI(con, destRegID, 0x8000);
    CVMJITaddCodegenComment((con, "goto divDone"));
    CVMMIPSemitBranch0(con, divDoneLogicalPC,
		      MIPS_BEQ_OPCODE, destRegID, lhsRegID,
                      NULL, CVM_TRUE);

    /* emit the divw instruction */
    CVMtraceJITCodegen(("\t\t.doDiv\n"));
    emitInstruction(con, lhsRegID << MIPS_RS_SHIFT |
		    rhsRegID << MIPS_R_RT_SHIFT | MIPS_DIV_OPCODE);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	div	%s, %s\n",
			 regName(lhsRegID), regName(rhsRegID));
    });

    emitInstruction(con, destRegID << MIPS_R_RD_SHIFT | 
        (opcode == CVMMIPS_DIV_OPCODE ? MIPS_MFLO_OPCODE : MIPS_MFHI_OPCODE));
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%s\n",
			 (opcode == CVMMIPS_DIV_OPCODE ? "mflo" : "mfhi"),
                         regName(destRegID));
    });
    CVMCPUemitNop(con);

    CVMtraceJITCodegen(("\t\t.divDone\n"));
}

/*
 * Table that provides encoding information for register based binary
 * ALU instructions. ARM has very consistent encodings across ALU opcodes,
 * so the encoding can easily be done in the CVMPU_XXX_OPCODE value,
 * plus some uniform masking of things like register encodings and
 * "setcc" bits. MIPS has very inconsistent encodings, so making
 * it table based is necessary. Also Note, there is no "setcc" concept 
 * in MIPS ALU instructions.
 */
typedef struct CVMMIPSBinaryALUOpcode CVMMIPSBinaryALUOpcode;
struct CVMMIPSBinaryALUOpcode {
    CVMUint32 opcodeReg;      /* encoding of opcode with reg rhs */
    CVMUint32 opcodeImm;      /* encoding of opcode with imm rhs */
};

static const CVMMIPSBinaryALUOpcode mipsBinaryALUOpcodes[] = {
    /* CVMCPU_ADD_OPCODE */
    {MIPS_ADDU_OPCODE, MIPS_ADDIU_OPCODE},
    /* CVMCPU_SUB_OPCODE */
    {MIPS_SUBU_OPCODE, MIPS_ADDIU_OPCODE},
    /* CVMCPU_AND_OPCODE */
    {MIPS_AND_OPCODE, MIPS_ANDI_OPCODE},
    /* CVMCPU_OR_OPCODE */
    {MIPS_OR_OPCODE, MIPS_ORI_OPCODE},
    /* CVMCPU_XOR_OPCOE */
    {MIPS_XOR_OPCODE, MIPS_XORI_OPCODE},
    /* CVMCPU_BIC_OPCODE */
    {CVMCPU_BIC_OPCODE, MIPS_ANDI_OPCODE},
};

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
void
CVMCPUemitBinaryALU(CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID,
		    CVMCPUALURhsToken rhsToken,
		    CVMBool setcc)
{
    int tokenType = CVMMIPSalurhsDecodeGetTokenType(rhsToken);
    CVMUint32 mipsOpcode;

    /*
     * MIPS ALU instructions do not have the concept of 'setcc'.
     * Ignore 'setcc' argument.
     */

    if (opcode == CVMMIPS_DIV_OPCODE || opcode == CVMMIPS_REM_OPCODE) {
	CVMMIPSemitDivOrRem(con, opcode, destRegID, lhsRegID, rhsToken);
	return;
    }

    CVMassert(opcode == CVMCPU_ADD_OPCODE  || opcode == CVMCPU_SUB_OPCODE ||
              opcode == CVMCPU_AND_OPCODE  || opcode == CVMCPU_OR_OPCODE  ||
              opcode == CVMCPU_XOR_OPCODE  || opcode == CVMCPU_BIC_OPCODE);

    if (tokenType == CVMMIPS_ALURHS_REGISTER) {
	int rhsRegID = CVMMIPSalurhsDecodeGetTokenRegister(rhsToken);

        mipsOpcode = mipsBinaryALUOpcodes[opcode].opcodeReg;
	CVMassert(mipsOpcode != 0);

        if (opcode == CVMCPU_BIC_OPCODE) {
            /* BIC register is implemented as:
             * 
             *     nor	t7, rhs, zero
             *     and	dest, lhs, t7
             */
            emitInstruction(con,
                            rhsRegID << MIPS_RS_SHIFT |
                            CVMMIPS_zero << MIPS_R_RT_SHIFT |
                            CVMMIPS_t7 << MIPS_R_RD_SHIFT | MIPS_NOR_OPCODE);
            CVMtraceJITCodegenExec({
                printPC(con);
                CVMconsolePrintf("	nor	%s, %s, %s",
                                 regName(CVMMIPS_t7), regName(rhsRegID),
                                 regName(CVMMIPS_zero));
            });
            CVMJITdumpCodegenComments(con);

            emitInstruction(con,
                            lhsRegID << MIPS_RS_SHIFT |
                            CVMMIPS_t7 << MIPS_R_RT_SHIFT |
                            destRegID << MIPS_R_RD_SHIFT | MIPS_AND_OPCODE);
            CVMtraceJITCodegenExec({
                printPC(con);
                CVMconsolePrintf("	and	%s, %s, %s",
                                 regName(destRegID), regName(lhsRegID),
                                 regName(CVMMIPS_t7));
            });
        } else {
	    emitInstruction(con, 
                            lhsRegID << MIPS_RS_SHIFT | 
                            rhsRegID << MIPS_R_RT_SHIFT |
                            destRegID << MIPS_R_RD_SHIFT | mipsOpcode);
	    CVMtraceJITCodegenExec({
	        printPC(con);
		if (opcode == CVMCPU_ADD_OPCODE && lhsRegID == CVMMIPS_zero) {
		    /* this is really a "move register" instruction */
		    CVMconsolePrintf("	move	%s, %s",
				     regName(destRegID), regName(rhsRegID));
		} else {
		    CVMconsolePrintf("	%s	%s, %s, %s",
				     getALUOpcodeName(mipsOpcode),
				     regName(destRegID), regName(lhsRegID),
				     regName(rhsRegID));
		}
	    });
        }
    } else { /* CVMMIPS_ALURHS_CONSTANT */
        CVMInt16 rhsConst;

        CVMassert(opcode != CVMCPU_BIC_OPCODE);
	CVMassert(tokenType == CVMMIPS_ALURHS_CONSTANT);

        rhsConst = CVMMIPSalurhsDecodeGetTokenConstant(rhsToken);
        mipsOpcode = mipsBinaryALUOpcodes[opcode].opcodeImm;
	CVMassert(mipsOpcode != 0);

	switch (opcode) {
	    case CVMCPU_SUB_OPCODE: {
		/* SUB of imm is converted to ADD of -imm */
		rhsConst = -rhsConst;
		break;
	    }
	    case CVMCPU_ADD_OPCODE:
	    case CVMCPU_AND_OPCODE:
	    case CVMCPU_OR_OPCODE:
	    case CVMCPU_XOR_OPCODE:
		break;
	    default: CVMassert(CVM_FALSE);
	}
	emitInstruction(con, mipsOpcode |
                        lhsRegID << MIPS_RS_SHIFT |
                        destRegID << MIPS_I_RD_SHIFT |
			((CVMUint16)rhsConst & 0xffff));
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	%s	%s, %s, %d",
	    		 getALUOpcodeName(mipsOpcode),
	    		 regName(destRegID), regName(lhsRegID),
	    		 rhsConst);
	});
    }
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
void
CVMCPUemitBinaryALUConstant(CVMJITCompilationContext* con,
			    int opcode, int destRegID, int lhsRegID,
			    CVMInt32 rhsConstValue, CVMBool setcc)
{
    CVMCPUALURhsToken rhsToken;

    /*
     * MIPS ALU instructions do not have the concept of 'setcc'.
     * Ignore 'setcc' argument.
     */

    if (opcode == CVMCPU_BIC_OPCODE) {
        /* BIC constant is implemented as:
         *
         *     li	t7, ~rhsConst
         *     and	dest, lhs, t7
         *
         * or:
         *
         *     lui      t7, hi16(~rhsConst)
         *     and	dest, lhs, t7
         *
         * or:
         *
         *     lui	t7, hi16(~rhsConst)
         *     ori	t7, t7, lo16(~rhsConst)
         *     and	dest, lhs, t7
         */
        CVMCPUemitLoadConstant(con, CVMMIPS_t7, ~rhsConstValue);
        rhsToken = CVMMIPSalurhsEncodeRegisterToken(CVMMIPS_t7);
        CVMCPUemitBinaryALU(con, CVMCPU_AND_OPCODE, destRegID, lhsRegID,
                            rhsToken, CVM_FALSE /* no setcc */);
        return;
    }

    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsToken = CVMMIPSalurhsEncodeConstantToken(rhsConstValue);
    } else {
        /* Load the constant into a register. Use t7 as
           the scratch register.
         */
        CVMCPUemitLoadConstant(con, CVMMIPS_t7, rhsConstValue);
        rhsToken = CVMMIPSalurhsEncodeRegisterToken(CVMMIPS_t7);
    }
    CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID,
                        rhsToken, CVM_FALSE /* no setcc */);
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.*/
void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, CVMUint32 shiftAmount)
{
    CVMUint32 mipsOpcode;

    CVMassert((shiftAmount >> 5) == 0);

    switch(opcode) {
        case CVMCPU_SLL_OPCODE: mipsOpcode = MIPS_SLL_OPCODE; break;
        case CVMCPU_SRL_OPCODE: mipsOpcode = MIPS_SRL_OPCODE; break;
        case CVMCPU_SRA_OPCODE: mipsOpcode = MIPS_SRA_OPCODE; break;
        default: mipsOpcode = 0; CVMassert(CVM_FALSE); break;
    }

    emitInstruction(con,
        srcRegID <<  MIPS_R_RT_SHIFT | destRegID << MIPS_R_RD_SHIFT |
        shiftAmount << MIPS_SA_SHIFT | mipsOpcode); 

    CVMtraceJITCodegenExec({
	const char* opname = NULL;
        printPC(con);
	switch(opcode) {
            case CVMCPU_SLL_OPCODE: opname = "sll"; break;
            case CVMCPU_SRL_OPCODE: opname = "srl"; break;
            case CVMCPU_SRA_OPCODE: opname = "sra"; break;
	}
        CVMconsolePrintf("	%s	%s, %s, %d",
			 opname, regName(destRegID), regName(srcRegID),
			 shiftAmount);
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.
            The shift amount is specified in a register. The platform
            implementation is responsible for making sure that the shift
            semantics are done in such a way that the effect is the same as if
            the shiftAmount is masked with 0x1f before the shift operation is
            done. This is needed in order to be compliant with the VM spec.
*/
void
CVMCPUemitShiftByRegister(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, int shiftAmountRegID)
{
    CVMRMResource * scratchRes =
        CVMRMgetResource(CVMRM_INT_REGS(con),
                         CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    int scratchRegID = CVMRMgetRegisterNumber(scratchRes);
    CVMUint32 mipsOpcode;

    /* Mask off the shift amount with 0x1f before shifting per VM spec. */
    CVMJITaddCodegenComment((con,
			     "vm spec says use low 5 bits of shift amount"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_AND_OPCODE,
        scratchRegID, shiftAmountRegID, 0x1F, CVMJIT_NOSETCC);

    switch(opcode) {
        case CVMCPU_SLL_OPCODE: mipsOpcode = MIPS_SLLV_OPCODE; break;
        case CVMCPU_SRL_OPCODE: mipsOpcode = MIPS_SRLV_OPCODE; break;
        case CVMCPU_SRA_OPCODE: mipsOpcode = MIPS_SRAV_OPCODE; break;
        default: mipsOpcode = 0; CVMassert(CVM_FALSE); break;
    }
    emitInstruction(con,
        scratchRegID << MIPS_RS_SHIFT | srcRegID << MIPS_R_RT_SHIFT |
        destRegID << MIPS_R_RD_SHIFT | mipsOpcode);

    CVMtraceJITCodegenExec({
	const char* opname = NULL;
        printPC(con);
	switch(opcode) {
            case CVMCPU_SLL_OPCODE: opname = "sllv"; break;
            case CVMCPU_SRL_OPCODE: opname = "srlv"; break;
            case CVMCPU_SRA_OPCODE: opname = "srav"; break;
	}
        CVMconsolePrintf("	%s	%s, %s, %s",
			 opname, regName(destRegID), regName(srcRegID),
			 regName(scratchRegID));
    });
    CVMJITdumpCodegenComments(con);

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), scratchRes);
}

void
CVMMIPSemitMul(CVMJITCompilationContext* con, int opcode,
              int destreg, int lhsreg, int rhsreg,
              int extrareg, CVMBool needNop)
{
    CVMUint32 mipsOpcode;
    CVMBool moveFromHI;
#ifdef CVM_TRACE_JIT
    const char* opname = NULL;
#endif
    CVMassert(extrareg == CVMCPU_INVALID_REG);

#if 0
    /* On the MIPS III, IV machines that we have, mul is not 
     * available, although it is specified in MIPS32 ISA.
     */
    if (opcode == CVMMIPS_MUL_OPCODE) {
        emitInstruction(con, MIPS_MUL_OPCODE |
            lhsreg << MIPS_RS_SHIFT | rhsreg << MIPS_R_RT_SHIFT |
            destreg << MIPS_R_RD_SHIFT);
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	mul	%s, %s, %s",
                regName(destreg), regName(lhsreg), regName(rhsreg));
        });
    }
#endif

    switch (opcode) {
        case CVMCPU_MULL_OPCODE:
            CVMtraceJITCodegenExec(opname = "mult");
	    mipsOpcode = MIPS_MULT_OPCODE;
            moveFromHI = CVM_FALSE;
	    break;
        case CVMCPU_MULH_OPCODE:
	    CVMtraceJITCodegenExec(opname = "mult");
	    mipsOpcode = MIPS_MULT_OPCODE;
            moveFromHI = CVM_TRUE;
	    break;
	case CVMMIPS_MULTU_OPCODE:
            CVMtraceJITCodegenExec(opname = "multu");
            mipsOpcode = MIPS_MULTU_OPCODE;
            moveFromHI = CVM_FALSE;
            break;
        default:
	    CVMtraceJITCodegenExec(opname = "unknown");
	    mipsOpcode = 0;
            moveFromHI = CVM_FALSE;
	    CVMassert(CVM_FALSE);
    }

    emitInstruction(con, lhsreg << MIPS_RS_SHIFT |
		        rhsreg << MIPS_R_RT_SHIFT | mipsOpcode);
    CVMtraceJITCodegenExec({
	printPC(con);
        CVMconsolePrintf("	%s	%s, %s", opname,
			     regName(lhsreg), regName(rhsreg));
    });
    CVMJITdumpCodegenComments(con);

    emitInstruction(con, destreg << MIPS_R_RD_SHIFT | 
        (moveFromHI ? MIPS_MFHI_OPCODE : MIPS_MFLO_OPCODE));
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	%s",
			 (moveFromHI ? "mfhi" : "mflo"),
                         regName(destreg));
    });
    CVMJITdumpCodegenComments(con);

    if (needNop) {
        CVMCPUemitNop(con);
        CVMCPUemitNop(con);
    } 
}

void
CVMCPUemitMove(CVMJITCompilationContext* con, int opcode,
	       int destRegID, CVMCPUALURhsToken srcToken,
	       CVMBool setcc)
{
    int srcRegID = CVMMIPSalurhsDecodeGetTokenRegister(srcToken);

    CVMassert(!setcc);
    CVMassert(CVMMIPSalurhsDecodeGetTokenType(srcToken) !=
	      CVMMIPS_ALURHS_CONSTANT);

    switch (opcode){
    case CVMCPU_MOV_OPCODE:
#if 0
        emitInstruction(con,
            srcRegID << MIPS_RS_SHIFT | CVMMIPS_zero << MIPS_R_RT_SHIFT |
            destRegID << MIPS_R_RD_SHIFT | MIPS_MOVZ_OPCODE);
        CVMtraceJITCodegenExec({
            printPC(con);
            CVMconsolePrintf("	movz	%s, %s, %s",
                regName(destRegID), regName(srcRegID), regName(CVMMIPS_zero));
	});
#endif
        CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                                    destRegID, CVMMIPS_zero, srcRegID,
                                    CVMJIT_NOSETCC);
	break;
#ifdef CVM_JIT_USE_FP_HARDWARE
    case CVMCPU_FMOV_OPCODE:
	CVMassert(!setcc);
	CVMCPUemitUnaryFP(con, CVMCPU_FMOV_OPCODE, destRegID, srcRegID);
	break;
#endif
    default:
	CVMassert(CVM_FALSE);
    }
}

extern void
CVMMIPSemitCompare(CVMJITCompilationContext* con,
                   int opcode, CVMCPUCondCode condCode, int lhsRegID,
		   int rhsRegID, CVMInt32 rhsConst, int cmpRhsType)
{
    CVMJITTargetCompilationContext *targetCon = &(con->target);
    CVMassert(opcode == CVMCPU_CMP_OPCODE || 
              opcode == CVMCPU_CMN_OPCODE ||
              opcode == CVMCPU_CMP64_OPCODE);
    
    /* 
     * Store the arguments of emitCompare() in CompilationContext.
     * Defer comparisions until the result is used by conditional
     * branch.
     */
    targetCon->cmpOpcode = opcode;
    targetCon->cmpLhsRegID = lhsRegID;
    CVMassert(targetCon->cmpLhsRegID != CVMCPU_INVALID_REG);
    targetCon->cmpCondCode = condCode;
    targetCon->cmpRhsConst = rhsConst;
    targetCon->cmpRhsRegID = rhsRegID;
    targetCon->cmpRhsType = cmpRhsType;
    
    CVMtraceJITCodegenExec({
	CVMCodegenComment* comment;
	const char* commentStr = "";
	CVMJITpopCodegenComment(con, comment);
	if (comment != NULL) {
	    commentStr = comment->commentStr;
	}
	CVMJITprintCodegenComment(("deferred comparison: %s", commentStr));
    });
}

extern void
CVMCPUemitCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken)
{
    int type = CVMMIPSalurhsDecodeGetTokenType(rhsToken);
    
    if (type == CVMMIPS_ALURHS_REGISTER) {
	int rhsRegID = CVMMIPSalurhsDecodeGetTokenRegister(rhsToken);
	CVMassert(rhsRegID != CVMCPU_INVALID_REG);
	CVMMIPSemitCompare(con, opcode, condCode, lhsRegID,
			   rhsRegID, 0, type);
    } else {
	CVMInt32 rhsConst = CVMMIPSalurhsDecodeGetTokenConstant(rhsToken);
	CVMMIPSemitCompare(con, opcode, condCode, lhsRegID,
			   CVMCPU_INVALID_REG, rhsConst, type);
    }
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
    CVMMIPSemitCompare(con, opcode, condCode, lhsRegID,
		       rhsRegID, 0, CVMMIPS_ALURHS_REGISTER);
    return condCode;
}

/* Purpose: Emits instructions to do the specified 64 bit unary ALU
            operation. */
void
CVMCPUemitUnaryALU64(CVMJITCompilationContext *con, int opcode,
                     int destRegID, int srcRegID)
{
    switch (opcode) {
        case CVMCPU_NEG64_OPCODE: {
            /* Implemented as:

                   subu    dest_lo,zero,src_lo
                   subu    dest_hi,zero,src_hi
                   sltu    tmp,zero,dest_lo
                   subu    dest_hi,dest_hi,tmp
             */
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                LOREG(destRegID), CVMMIPS_zero, LOREG(srcRegID),
                CVMJIT_NOSETCC);
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                HIREG(destRegID), CVMMIPS_zero, HIREG(srcRegID),
                CVMJIT_NOSETCC);
            CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE, CVMMIPS_t7,
                CVMMIPS_zero, LOREG(destRegID));
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                HIREG(destRegID), HIREG(destRegID), CVMMIPS_t7,
                CVMJIT_NOSETCC);
            break;
        }
        default:
            CVMassert(CVM_FALSE);
    }
}

static void
CVMMIPSemitMFHI(CVMJITCompilationContext *con, int destRegID)
{
    emitInstruction(con, 
        destRegID << MIPS_R_RD_SHIFT | MIPS_MFHI_OPCODE);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	mfhi	%s", regName(destRegID));
    });
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits instructions to do the specified 64 bit ALU operation. */
void
CVMCPUemitBinaryALU64(CVMJITCompilationContext *con,
		      int opcode, int destRegID, int lhsRegID, int rhsRegID)
{
    CVMUint32 realOpcode;
    CVMRMResource* scratchRes =
        CVMRMgetResource(CVMRM_INT_REGS(con),
                         CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);           
    int scratchRegID = CVMRMgetRegisterNumber(scratchRes);
    int lhs_low = LOREG(lhsRegID);
    int lhs_hi = HIREG(lhsRegID);
    int rhs_low = LOREG(rhsRegID);
    int rhs_hi = HIREG(rhsRegID);
    int dest_low = LOREG(destRegID);
    int dest_hi = HIREG(destRegID);

    switch (opcode) {
        case CVMCPU_MUL64_OPCODE:
#if 0
	    /* It seems on MIPS III and MIPS IV machines, mul is
             * not a legal instruction, even it's specified in 
             * MIPS32 ISA. Also, MIPS I, II, and III architectures
             * require that the two instructions which follow the
             * MFHI must not modify the HI, LO registers. If this
             * restriction is violated, the result of the MFHI is
             * UNPREDICTABLE. This restriction was removed in MIPS
             * IV and MIPS32, and all subsequent levels of the
             * architecture.
             */
            /* 
               lhs_lo * rhs_lo --> des_low, dest_hi
               lhs_hi * rhs_lo + dest_hi --> dest_hi
               lhs_lo * rhs_hi + dest_hi --> dest_hi

    	       multu	$lhs_low, $rhs_low
               mflo	$dest_low
               mfhi	$dest_hi
    	       mul	$scratch,$lhs_hi,$rhs_low
               addu	$dest_hi, $dest_hi, $scratch
	       mul	$scratch,$lhs_low,$rhs_hi
	       addu	$dest_hi,$dest_hi,$scratch
             */	
            CVMMIPSemitMul(con, CVMMIPS_MULTU_OPCODE,
		          dest_low, lhs_low, rhs_low, 
                          CVMCPU_INVALID_REG, CVM_FALSE);
            CVMMIPSemitMFHI(con, dest_hi);
            CVMMIPSemitMul(con, CVMMIPS_MUL_OPCODE,
		          scratchRegID, lhs_hi, rhs_low, 
                          CVMCPU_INVALID_REG, CVM_FALSE);
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, dest_hi, dest_hi,
			        CVMMIPSalurhsEncodeRegisterToken(scratchRegID),
			        CVMJIT_NOSETCC);
            CVMMIPSemitMul(con, CVMMIPS_MUL_OPCODE,
		          scratchRegID, lhs_low, rhs_hi, 
                          CVMCPU_INVALID_REG, CVM_FALSE);
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, dest_hi, dest_hi,
			        CVMMIPSalurhsEncodeRegisterToken(scratchRegID),
			        CVMJIT_NOSETCC);
#endif
            /* 
               lhs_lo * rhs_lo --> des_low, dest_hi
               lhs_hi * rhs_lo + dest_hi --> dest_hi
               lhs_lo * rhs_hi + dest_hi --> dest_hi

	       nop
    	       multu	$lhs_low, $rhs_low
               mflo	$dest_low
               mfhi	$dest_hi
	       nop
	       nop
    	       mult	$lhs_hi,$rhs_low
	       mflo	$scratch
               addu	$dest_hi, $dest_hi, $scratch
	       nop
	       mult	$lhs_low,$rhs_hi
	       mflo	$scratch
	       addu	$dest_hi,$dest_hi,$scratch
             */
            CVMCPUemitNop(con);
            CVMMIPSemitMul(con, CVMMIPS_MULTU_OPCODE,
		          dest_low, lhs_low, rhs_low,
                          CVMCPU_INVALID_REG, CVM_FALSE);
            CVMMIPSemitMFHI(con, dest_hi);
            CVMCPUemitNop(con);
            CVMCPUemitNop(con);
            CVMMIPSemitMul(con, CVMCPU_MULL_OPCODE,
		           scratchRegID, lhs_hi, rhs_low, 
                           CVMCPU_INVALID_REG, CVM_FALSE);
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, dest_hi, dest_hi,
			        CVMMIPSalurhsEncodeRegisterToken(scratchRegID),
			        CVMJIT_NOSETCC);
            CVMCPUemitNop(con);
            CVMMIPSemitMul(con, CVMCPU_MULL_OPCODE,
		           scratchRegID, lhs_low, rhs_hi, 
                           CVMCPU_INVALID_REG, CVM_FALSE);
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, dest_hi, dest_hi,
			        CVMMIPSalurhsEncodeRegisterToken(scratchRegID),
			        CVMJIT_NOSETCC);
            break;
        case CVMCPU_ADD64_OPCODE:
	    /*  
	       addu	$dest_low, $lhs_low, $rhs_low
               sltu	$scratch, $dest_low, $rhs_low
               addu	$dest_hi, $lhs_hi, $rhs_hi
               addu	$dest_hi, $dest_hi, $scratch
             */
            CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                dest_low, lhs_low, rhs_low, CVMJIT_NOSETCC);
            CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE,
                scratchRegID, dest_low, rhs_low);
            CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                dest_hi, lhs_hi, rhs_hi, CVMJIT_NOSETCC);
            CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                dest_hi, dest_hi, scratchRegID, CVMJIT_NOSETCC);
            break;
        case CVMCPU_SUB64_OPCODE:
	    /* 
               sltu	$scratch, $lhs_low, $rhs_low
               subu	$dest_low, $lhs_low, $rhs_low
               subu	$dest_hi, $lhs_hi, $rhs_hi
               subu	$dest_hi, $dest_hi, $scratch
             */
            CVMMIPSemitSLT(con, MIPS_SLTU_OPCODE,
                scratchRegID, lhs_low, rhs_low);
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                dest_low, lhs_low, rhs_low, CVMJIT_NOSETCC);
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                dest_hi, lhs_hi, rhs_hi, CVMJIT_NOSETCC);
            CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
                dest_hi, dest_hi, scratchRegID, CVMJIT_NOSETCC);
            break;
        case CVMCPU_AND64_OPCODE:
            realOpcode = CVMCPU_AND_OPCODE;
            goto emitTwo32Instr;
        case CVMCPU_OR64_OPCODE:
            realOpcode = CVMCPU_OR_OPCODE;
            goto emitTwo32Instr;
        case CVMCPU_XOR64_OPCODE:
            realOpcode = CVMCPU_XOR_OPCODE;
        emitTwo32Instr:
            CVMCPUemitBinaryALURegister(con, realOpcode, dest_low,
                lhs_low, rhs_low, CVMJIT_NOSETCC);
            CVMCPUemitBinaryALURegister(con, realOpcode, dest_hi,
                lhs_hi, rhs_hi, CVMJIT_NOSETCC);
            break;
        default:
            CVMassert(CVM_FALSE);
    }

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), scratchRes);
}

/* Purpose: Loads a 64-bit integer constant into a register. */
void
CVMCPUemitLoadLongConstant(CVMJITCompilationContext *con, int regID,
                           CVMJavaVal64 *value)
{
    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstant(con, HIREG(regID), val.l>>32);
    CVMCPUemitLoadConstant(con, LOREG(regID), (int)val.l);
}

/* Purpose: Emits instructions to convert a 32 bit int into a 64 bit int. */
void
CVMCPUemitInt2Long(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   LOREG(destRegID), srcRegID, CVMJIT_NOSETCC);
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE,
			      HIREG(destRegID), srcRegID, 31);
}

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID, LOREG(srcRegID), CVMJIT_NOSETCC);
}

/*
 * Put a constant into a register.
 */
void
CVMCPUemitLoadConstant(
    CVMJITCompilationContext* con,
    int regno,
    CVMInt32 v)
{
    if (CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_ADD_OPCODE, v)) {
        /* addiu regno, zero, v */
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
            regno, CVMMIPS_zero, CVMMIPSalurhsEncodeConstantToken(v),
            CVMJIT_NOSETCC);
    } else if (CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_OR_OPCODE, v)) {
        /* ori regno, r0, v */
        CVMCPUemitBinaryALU(con, CVMCPU_OR_OPCODE,
            regno, CVMMIPS_zero, CVMMIPSalurhsEncodeConstantToken(v),
            CVMJIT_NOSETCC);
    } else if ((v & 0xffff) == 0) {
        /* lui regno, hi16(v) */
	CVMMIPSemitLUI(con, regno, v >> 16);
    } else {
#ifndef CVMCPU_HAS_CP_REG
	CVMMIPSemitMakeConstant32(con, regno, v);
#else
	/* Do a big constant stored in the constant pool: */
        CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
        CVMInt32 targetLiteralOffset =
	    CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
	CVMassert(targetLiteralOffset == 0);
	(void)targetLiteralOffset;

	/*
	 * Emit the load relative to the constant pool base register.
	 * The offset of 0 will be patched after the constant pool is dumped.
	 */
        CVMCPUemitMemoryReference(con, CVMCPU_LDR32_OPCODE,
	    regno, CVMCPU_CP_REG,
	    CVMCPUmemspecEncodeImmediateToken(con, 0) /* offset */);
#endif
    }
    CVMJITresetSymbolName(con);
}

/*
 * Load from a pc-relative location. Unfortunately this is slow
 * on MIPS, because there is no support for dong a pc-relative
 * load. We have to load the current address as a constant, and
 * then load relative to it. Fortunately the need for this is
 * rare and is in code that is somewhat slow already, like
 * dealing with potentially unresolved constant pool entries.
 *
 *
 * NOTE: This function must always emit the exact same number of
 *       instructions regardless of the offset passed to it.
 */
void
CVMMIPSemitMemoryReferencePCRelative(CVMJITCompilationContext* con, int opcode,
				    int destRegID, int lhsRegID, int offset)
{
    CVMInt32 physicalPC = (CVMInt32)CVMJITcbufGetPhysicalPC(con);
    CVMInt32 addr = physicalPC + offset;
    CVMInt16  lo16 =  addr & 0xffff;
    CVMUint16 ha16 = (addr >> 16) & 0xffff;

    /*
     *   lui      destRegID, ha16(physicalPC)
     *   lw       destRegID, lo16(physicalPC)(destRegID)
     */

    /* Add one to ha16 if lo16 is negative to account for borrow that
     * will be done during lwz. */
    if (lo16 < 0) {
	ha16 += 1;
    }

    /*
     * Load high bits into register, and then load at an offset
     * from the high bits.
     */
    CVMMIPSemitLUI(con, destRegID, ha16);
    CVMCPUemitMemoryReference(con, opcode,
        destRegID, destRegID, CVMCPUmemspecEncodeImmediateToken(con, lo16));
}

/*
 * If we have 64-bit FPR's, then handle unlaigned access using
 * LDL, LDR, SDL, and SDR.
 */
#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
static void
CVMMIPSemitMemoryReference64Unaligned(CVMJITCompilationContext* con,
				      CVMUint32 mipsOpcode,
				      int destRegID, int baseRegID, int offset)
{
    emitInstruction(con, mipsOpcode | baseRegID << MIPS_RS_SHIFT |
		    destRegID << MIPS_I_RD_SHIFT | (offset & 0xffff));
    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("	%s	%s, %d(%s)",
			 getMemOpcodeName(mipsOpcode),
			 regName(destRegID), offset, regName(baseRegID));
    });
    CVMJITdumpCodegenComments(con);
}
#endif

/*
 * Emit a 32-bit memory reference.
 *
 * NOTE: Mips has only one load/store addressing mode: baseReg+<imm16>.
 * Also since mips does not support post-increment/pre-decrement,
 * this function will not do any adjustment to the basereg if 
 * post-increment/pre-decrement is requested. It is up to the caller
 * to handle this. This is so when the caller is doing a 64-bit memory
 * reference, it can do the adjustment in one instruction.
 */
static void
CVMMIPSemitPrimitiveMemoryReference(CVMJITCompilationContext* con,
    int opcode,
    int destreg, /* or srcreg for loads */
    int basereg,
    CVMCPUMemSpecToken memSpecToken)
{
    CVMUint32 mipsOpcode;
    int offsetImm;
    int type = CVMMIPSmemspecGetTokenType(memSpecToken);
    int newbaseReg = CVMMIPS_t7;

    switch (opcode) {
        case CVMCPU_LDR8U_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LBU_OPCODE; break;
        case CVMCPU_LDR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LB_OPCODE; break;
        case CVMCPU_STR8_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE);
            mipsOpcode = MIPS_SB_OPCODE; break;
        case CVMCPU_LDR16U_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LHU_OPCODE; break;
        case CVMCPU_LDR16_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LH_OPCODE; break;
        case CVMCPU_STR16_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE);
            mipsOpcode = MIPS_SH_OPCODE; break;
        case CVMCPU_LDR32_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LW_OPCODE; break;
        case CVMCPU_STR32_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE);
            mipsOpcode = MIPS_SW_OPCODE; break;
#ifdef CVM_JIT_USE_FP_HARDWARE
        case CVMCPU_FLDR32_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = MIPS_LWC1_OPCODE; break;
        case CVMCPU_FSTR32_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE);
            mipsOpcode = MIPS_SWC1_OPCODE; break;
#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
        case CVMCPU_FLDR64_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
            mipsOpcode = 0; break;
        case CVMCPU_FSTR64_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE);
            mipsOpcode = 0; break;
#endif
#endif
        default:
            mipsOpcode = 0;
            CVMassert(CVM_FALSE);
    }

    switch (type) {
        case CVMCPU_MEMSPEC_REG_OFFSET: {
            int indexReg = CVMMIPSmemspecGetTokenRegister(memSpecToken);
            /* add indexReg and baseReg */
            CVMJITaddCodegenComment((con, "add indexReg and baseReg"));
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
                        newbaseReg, basereg, indexReg, CVMJIT_NOSETCC);
            offsetImm = 0;
            basereg = newbaseReg;
            break;
        }
	case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
	    offsetImm = CVMMIPSmemspecGetTokenOffset(memSpecToken);
	    break;
	case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
	    offsetImm = 0;
      	    break;
	case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
            CVMJITaddCodegenComment((con, "do manual pre decrement"));
            CVMCPUemitBinaryALUConstant(con, CVMCPU_SUB_OPCODE, basereg,
                basereg, CVMMIPSmemspecGetTokenOffset(memSpecToken),
                CVMJIT_NOSETCC);
	    offsetImm = 0;
            break;
	default:
	    offsetImm = 0;
	    CVMassert(CVM_FALSE);
    }

#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
    /*
     * If we have 64-bit FPR's, then handle unlaigned access using
     * LDL, LDR, SDL, and SDR.
     */
#undef UNALIGNED_OP1
#undef UNALIGNED_OP2
#if CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN
#define UNALIGNED_OP1(op1, op2) op1
#define UNALIGNED_OP2(op1, op2) op2
#else
#define UNALIGNED_OP1(op1, op2) op2
#define UNALIGNED_OP2(op1, op2) op1
#endif
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
    if (opcode == CVMCPU_FLDR64_OPCODE) {
        int tmpreg = CVMMIPS_t7;
	CVMMIPSemitMemoryReference64Unaligned(con,
            UNALIGNED_OP1(MIPS_LDL_OPCODE, MIPS_LDR_OPCODE),
	    CVMMIPS_t7, basereg, offsetImm+7);
	CVMMIPSemitMemoryReference64Unaligned(con,
            UNALIGNED_OP2(MIPS_LDL_OPCODE, MIPS_LDR_OPCODE),
	    CVMMIPS_t7, basereg, offsetImm);
        CVMCPUemitUnaryFP(con, CVMMIPS_DMTC1_OPCODE, destreg, tmpreg);
    } else if (opcode == CVMCPU_FSTR64_OPCODE) {
        int tmpreg = CVMMIPS_t7;
        CVMCPUemitUnaryFP(con, CVMMIPS_DMFC1_OPCODE, tmpreg, destreg);
	CVMMIPSemitMemoryReference64Unaligned(con,
            UNALIGNED_OP1(MIPS_SDL_OPCODE, MIPS_SDR_OPCODE),
	    CVMMIPS_t7, basereg, offsetImm+7);
	CVMMIPSemitMemoryReference64Unaligned(con,
            UNALIGNED_OP2(MIPS_SDR_OPCODE, MIPS_SDR_OPCODE),
	    CVMMIPS_t7, basereg, offsetImm);
    } else
#endif /* CVMMIPS_HAS_64BIT_FP_REGISTERS */
#endif /* CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED */
    {
	CVMassert(mipsOpcode != 0);
	emitInstruction(con, mipsOpcode | basereg << MIPS_RS_SHIFT |
			destreg << MIPS_I_RD_SHIFT | (offsetImm & 0xffff));
    }
    
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
    if (opcode == CVMCPU_FLDR64_OPCODE || opcode == CVMCPU_FSTR64_OPCODE) {
	    /* do nothing. instruction already printed earlier */
    } else
#endif
#endif
    {
	CVMtraceJITCodegenExec({
	    if (opcode == CVMCPU_LDR8U_OPCODE ||
		opcode == CVMCPU_LDR8_OPCODE ||
		opcode == CVMCPU_STR8_OPCODE ||
		opcode == CVMCPU_LDR16U_OPCODE ||
		opcode == CVMCPU_LDR16_OPCODE ||
		opcode == CVMCPU_STR16_OPCODE ||
		opcode == CVMCPU_LDR32_OPCODE ||
		opcode == CVMCPU_STR32_OPCODE) {
		printPC(con);
		CVMconsolePrintf("	%s	%s, %d(%s)",
				 getMemOpcodeName(mipsOpcode),
				 regName(destreg), offsetImm,
				 regName(basereg));
	    } else if (opcode == CVMCPU_FLDR32_OPCODE ||
		       opcode == CVMCPU_FSTR32_OPCODE) {
		printPC(con);
		CVMconsolePrintf("	%s	$f%d, %d(%s)",
				 getMemOpcodeName(mipsOpcode),
				 destreg, offsetImm, regName(basereg));
	    } else {
		CVMassert(CVM_FALSE);
	    }
	});
    }
    CVMJITdumpCodegenComments(con);

    /* If it's post-increment type, subtract the basereg */
    if (type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET) {
        CVMJITaddCodegenComment((con, "do manual post increment"));
        CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, basereg,
            basereg, CVMMIPSmemspecGetTokenOffset(memSpecToken),
            CVMJIT_NOSETCC);
    }
}

#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
#ifndef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
static void
CVMMIPSemitMemoryReference64FP(CVMJITCompilationContext* con,
    int opcode,
    int destreg, /* or srcreg for loads */
    int basereg,
    CVMCPUMemSpecToken memSpecToken) 
{   
    CVMRMResource* tmpRes = CVMRMgetResource(CVMRM_INT_REGS(con),
			    CVMRM_ANY_SET, CVMRM_EMPTY_SET, 2);
    int tmpReg1 = CVMRMgetRegisterNumber(tmpRes);

    /* TODO: these can probably be done by using FPR pairs
     * instead of GPR pairs. In this case, the FLDR64 and FSTR64
     * would take only 3 instructions instead of 7.
     */
    if (opcode == CVMCPU_FLDR64_OPCODE) {
        /* 1. Load the 64-bit content into a pair of integer registers. */
        CVMCPUemitMemoryReference(con, CVMCPU_LDR64_OPCODE, tmpReg1,
                                  basereg, memSpecToken);

        /* 2. Move the 64-bit content into one integer register (t7). */
        CVMCPUemitMoveTo64BitRegister(con, CVMMIPS_t7, tmpReg1);

        /* 3. Move the 64-bit content into the FP register. */
        CVMCPUemitUnaryFP(con, CVMMIPS_DMTC1_OPCODE, destreg, CVMMIPS_t7);

    } else {
        CVMassert(opcode == CVMCPU_FSTR64_OPCODE);

        /* 1. Move the 64-bit content to an integer register (t7) */
        CVMCPUemitUnaryFP(con, CVMMIPS_DMFC1_OPCODE, CVMMIPS_t7, destreg);
        
        /* 2. Move the 64-bit content to an integer register pair */
	/* TODO: no SLL32+SRA32 is needed here for the first word.
	 * Just move it to HIREG(tmpReg1). The high bits are truncated
	 * during the store. */
        CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSLL32_OPCODE,
                                     LOREGFP(tmpReg1), CVMMIPS_t7, 0);
        CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSRA32_OPCODE,
                                     HIREGFP(tmpReg1), CVMMIPS_t7, 0);
        CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSRL32_OPCODE,
                                     LOREGFP(tmpReg1), 
                                     LOREGFP(tmpReg1), 0);

        /* 3. Store it */
        CVMCPUemitMemoryReference(con, CVMCPU_STR64_OPCODE, tmpReg1,
                                  basereg, memSpecToken);
    }
    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpRes);
}
#endif /* !CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED */
#endif /* CVMMIPS_HAS_64BIT_FP_REGISTERS */

void
CVMCPUemitMemoryReference(CVMJITCompilationContext* con,
    int opcode,
    int destreg, /* or srcreg for loads */
    int basereg,
    CVMCPUMemSpecToken memSpecToken)
{
    int type = CVMMIPSmemspecGetTokenType(memSpecToken);
    int offset = CVMMIPSmemspecGetTokenOffset(memSpecToken);

#ifdef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
    if (opcode != CVMCPU_LDR64_OPCODE && opcode != CVMCPU_STR64_OPCODE ) {
#else
    if (opcode != CVMCPU_LDR64_OPCODE && opcode != CVMCPU_STR64_OPCODE &&
        opcode != CVMCPU_FLDR64_OPCODE && opcode != CVMCPU_FSTR64_OPCODE) {
#endif
	CVMMIPSemitPrimitiveMemoryReference(con, opcode,
	    destreg, basereg, memSpecToken);
    } else {
        /* Do the 64-bit cases */
        int newBasereg = CVMMIPS_t7;
        int destreg1 = destreg;
        int destreg2 = destreg + 1;

#ifdef CVMCPU_HAS_64BIT_REGISTERS
#ifndef CVMMIPS_USE_MEMORY_REFERERNCE64_UNALIGNED
        if (opcode == CVMCPU_FLDR64_OPCODE ||
            opcode == CVMCPU_FSTR64_OPCODE) {
            CVMMIPSemitMemoryReference64FP(con, opcode,
                destreg, basereg, memSpecToken);
            return;
        }
#endif
#endif

	/* 
	 * NOTE: 32-bit FPR pairs are always in big endian order, while
	 * 32-bit GPR pairs are always in the native endian order.
	 */
        switch (opcode) {
	    case CVMCPU_LDR64_OPCODE:
                opcode = CVMCPU_LDR32_OPCODE; break;
            case CVMCPU_STR64_OPCODE:
                opcode = CVMCPU_STR32_OPCODE; break;
#ifndef CVMMIPS_HAS_64BIT_FP_REGISTERS
            case CVMCPU_FLDR64_OPCODE:
                opcode = CVMCPU_FLDR32_OPCODE;
                destreg1 = LOREGFP(destreg);
		destreg2 = HIREGFP(destreg);
		break;
            case CVMCPU_FSTR64_OPCODE:
                opcode = CVMCPU_FSTR32_OPCODE;
		destreg1 = LOREGFP(destreg);
		destreg2 = HIREGFP(destreg);
		break;
#endif
            default: CVMassert(CVM_FALSE);
        }

        switch (type) {
        case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
	    /* first word */
            CVMMIPSemitPrimitiveMemoryReference(con, opcode, destreg1,
                                         basereg, memSpecToken);
            /* second word */
            if (!CVMCPUmemspecIsEncodableAsImmediate(offset+4)) {
                CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                    newBasereg, basereg, 4, CVMJIT_NOSETCC);
                CVMMIPSemitPrimitiveMemoryReference(con, opcode,
                    destreg2, newBasereg, memSpecToken);
            } else {
                CVMMIPSemitPrimitiveMemoryReference(con, opcode,
                    destreg2, basereg,
                    CVMCPUmemspecEncodeImmediateToken(con, offset+4));
            }
            break;
        case CVMCPU_MEMSPEC_REG_OFFSET: {
            CVMCPUMemSpecToken tmpToken =
                CVMCPUmemspecEncodeImmediateToken(con, 0);
            int indexReg = CVMMIPSmemspecGetTokenRegister(memSpecToken);
            /* first word */
            /* add basereg and indexReg */
            CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, 
                                newBasereg, basereg, indexReg,
                                CVMJIT_NOSETCC);
            CVMMIPSemitPrimitiveMemoryReference(con, opcode, destreg1,
						newBasereg, tmpToken);
            /* second word */
            CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                newBasereg, newBasereg, 4, CVMJIT_NOSETCC);
            CVMMIPSemitPrimitiveMemoryReference(con, opcode, destreg2,
                newBasereg, tmpToken); 
            break;
        }
        case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
	    /* TODO: this is inefficient since it leads to two adjustments
	     * of basreg. Emit the two loads first and then the adjustment
	     * like we do on powerpc. */
	    /* first word */
            CVMMIPSemitPrimitiveMemoryReference(
                con, opcode, destreg1, basereg,
                CVMCPUmemspecEncodePostIncrementImmediateToken(con, 4));
            /* second word */
            CVMMIPSemitPrimitiveMemoryReference(
                con, opcode, destreg2, basereg,
                CVMCPUmemspecEncodePostIncrementImmediateToken(con,offset-4));
            break;
        case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
	    /* TODO - this is inefficient since it leads to two adjustments
	     * of basreg. Emit the adjustment first and then emit two loads
	     * like we do on powerpc.*/
	    /* second word */
            CVMMIPSemitPrimitiveMemoryReference(
                con, opcode, destreg2, basereg,
                CVMCPUmemspecEncodePreDecrementImmediateToken(con, offset-4));
            /* first word */
            CVMMIPSemitPrimitiveMemoryReference(
                con, opcode, destreg1, basereg,
                CVMCPUmemspecEncodePreDecrementImmediateToken(con, 4));
            break;
        default :
            CVMassert(CVM_FALSE);
        }
    }
}

/* Purpose: Emits instructions to do a load/store operation. */
void
CVMCPUemitMemoryReferenceImmediate(CVMJITCompilationContext* con,
    int opcode, int destRegID, int baseRegID, CVMInt32 immOffset)
{
    /* The "immOffset+4" check is in case this is a STR64 or LDR64 */
    if (CVMCPUmemspecIsEncodableAsImmediate(immOffset) &&
	CVMCPUmemspecIsEncodableAsImmediate(immOffset+4)) {
        CVMCPUemitMemoryReference(con, opcode,
	    destRegID, baseRegID,
            CVMCPUmemspecEncodeImmediateToken(con, immOffset));
    } else {
        /* Load immOffset into a register. */
        CVMRMResource* offsetRes = 
	    CVMRMgetResourceForConstant32(CVMRM_INT_REGS(con),
		CVMRM_ANY_SET, CVMRM_EMPTY_SET, immOffset);
        CVMCPUemitMemoryReference(con, opcode,
            destRegID, baseRegID,
            CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET,
				     CVMRMgetRegisterNumber(offsetRes)));
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), offsetRes);
    }
}

/* Purpose: Emits instructions to do a load/store operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
    or
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
   
   On MIPS, this is done in two instructions: shift instruction
   and load/store instruction.
*/
void
CVMCPUemitArrayElementReference(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID,
    int indexRegID, int shiftOpcode, int shiftAmount)
{
    /* emit shift instruction first */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMMIPS_t7, 
                              indexRegID, shiftAmount);

    /* then, the load/store instruction: */
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMMIPSmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, CVMMIPS_t7));
}

/* Purpose: Emits code to computes the following expression and stores the
            result in the specified destReg:
                destReg = baseReg + shiftOpcode(indexReg, #shiftAmount)

   On MIPS, this is done in two instructions: shift instruction
   and add instruction.
*/
void
CVMCPUemitComputeAddressOfArrayElement(CVMJITCompilationContext *con,
    int opcode, int destRegID, int baseRegID,
    int indexRegID, int shiftOpcode, int shiftAmount)
{
    /* emit shift instruction first. Use destRegID to temporarily
     * store the shift value. */
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
     
           sll  scratch, shiftRegID, shiftAmount
           add  destRegID, addRegID, scratch
     */
    /* Use t7 as the scratch register. */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMMIPS_t7,
                              shiftRegID, shiftAmount);
    CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
				destRegID, addRegID, CVMMIPS_t7,
				CVMJIT_NOSETCC);
}

/* Purpose: Emits instructions for implementing the following ALU operation:
            destReg = srcReg & ((1 << bottomNBitsToKeep) - 1)
            where bottomNBitsToKeep is an immediate value.
            The user must ensure that bottomNBitsToKeep is less than 32.
*/
void
CVMCPUemitMaskOfTopNBits(CVMJITCompilationContext *con, int destRegID,
                         int srcRegID, CVMInt32 bottomNBitsToKeep)
{
    /* 
        This can be implemented as:

            and     rDest, rSrc, #((1 << bottomNBitsToKeep) - 1)
        or
            sll     rDest, rSrc, #(32 - bottomNBitsToKeep)
            slr     rDest, rDest, #(32 - bottomNBitsToKeep)
    */
    if (bottomNBitsToKeep < 17) {
        CVMCPUemitBinaryALU(con, CVMCPU_AND_OPCODE, destRegID, srcRegID,
            CVMMIPSalurhsEncodeConstantToken((1<<bottomNBitsToKeep)-1),
            CVMJIT_NOSETCC);
    } else {
        CVMCPUemitShiftByConstant(con, CVMCPU_SLL_OPCODE, destRegID,
                                  srcRegID, (32 - bottomNBitsToKeep));
        CVMCPUemitShiftByConstant(con, CVMCPU_SRL_OPCODE, destRegID,
                                  destRegID, (32 - bottomNBitsToKeep));
    }
    CVMJITdumpCodegenComments(con);
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
    int retryLogicalPC;

    CVMtraceJITCodegen(("\t\t"));
    CVMJITdumpCodegenComments(con);

    /* top of retry loop in case CAS operation is interrupted. */
    CVMtraceJITCodegen(("\t\tretry:\n"));
    retryLogicalPC = CVMJITcbufGetLogicalPC(con);

    /* SC will clobber destReg, but we still need it in case SC
     * fails and we have to retry. Use t7 as the scratch register to
     * save contents of newValReg.
     */
    CVMJITaddCodegenComment((con, "copy new word since sc will trash it"));
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   CVMMIPS_t7, destReg, CVMJIT_NOSETCC);


    /* emit LL instruction */
    emitInstruction(con, MIPS_LL_OPCODE | destReg << MIPS_R_RT_SHIFT |
		    addressReg << MIPS_RS_SHIFT | 0);
    CVMJITaddCodegenComment((con, "fetch word from address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	ll	%s, (%s)",
			 regName(destReg), regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* emit SC instruction */
    emitInstruction(con, MIPS_SC_OPCODE | CVMMIPS_t7 << MIPS_R_RT_SHIFT |
		    addressReg << MIPS_RS_SHIFT | 0);
    CVMJITaddCodegenComment((con, "conditionally store new word in address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	sc	%s, (%s)",
			 regName(CVMMIPS_t7), regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* Retry if reservation was lost and atomic operation failed */
    CVMJITaddCodegenComment((con, "br retry if reservation was lost"));
    CVMMIPSemitBranch0(con, retryLogicalPC,
		       MIPS_BEQ_OPCODE, CVMMIPS_t7, CVMMIPS_zero,
		       NULL, CVM_TRUE /* include nop */);

    CVMJITprintCodegenComment(("End of Atomic Swap"));
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
    int branchLogicalPC; /* pc of branch when CAS fails */
    int retryLogicalPC;  /* pc to retry of SC fails */
#ifdef CVM_TRACE_JIT
    const char* branchLabel = CVMJITgetSymbolName(con);
#endif

    /* Resource for loading value stored at address */
    CVMRMResource* actualVal = CVMRMgetResource(
        CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    int actualValReg = CVMRMgetRegisterNumber(actualVal);

    /* top of retry loop in case CAS operation is interrupted. */
    CVMtraceJITCodegen(("\t\tretry:\n"));
    retryLogicalPC = CVMJITcbufGetLogicalPC(con);

    /* emit LL instruction */
    emitInstruction(con, MIPS_LL_OPCODE | actualValReg << MIPS_R_RT_SHIFT |
		    addressReg << MIPS_RS_SHIFT | addressOffset);
    CVMJITaddCodegenComment((con, "fetch word from address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	ll	%s, %d(%s)",
			 regName(actualValReg), addressOffset, 
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);
   
    /* Branch to failure address if expected word not loaded. The proper
     * branch address will be patched in by the caller. */
    branchLogicalPC = CVMJITcbufGetLogicalPC(con);
    CVMJITaddCodegenComment((con, "br %s if word not as expected",
			     branchLabel));
    CVMMIPSemitBranch0(con, CVMJITcbufGetLogicalPC(con),
		       MIPS_BNE_OPCODE, actualValReg, oldValReg,
		       NULL, CVM_FALSE /* no nop */);

    /* SC will clobber newValReg, but we still need it in case SC
     * fails and we have to retry. Use t7 as the scratch register to
     * save contents of newValReg.  This will be done in the delay
     * slot of the previous branch.
     */
    CVMJITaddCodegenComment((con, "copy new word since sc will trash it"));
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   CVMMIPS_t7, newValReg, CVMJIT_NOSETCC);


    /* emit SC instruction */
    emitInstruction(con, MIPS_SC_OPCODE | CVMMIPS_t7 << MIPS_R_RT_SHIFT |
		    addressReg << MIPS_RS_SHIFT | addressOffset);
    CVMJITaddCodegenComment((con, "conditionally store new word in address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	sc	%s, %d(%s)",
			 regName(CVMMIPS_t7), addressOffset, 
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* Retry if reservation was lost and atomic operation failed */
    CVMJITaddCodegenComment((con, "br retry if reservation was lost"));
    CVMMIPSemitBranch0(con, retryLogicalPC,
		       MIPS_BEQ_OPCODE, CVMMIPS_t7, CVMMIPS_zero,
		       NULL, CVM_TRUE /* include nop */);

    CVMJITprintCodegenComment(("End of CAS"));

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), actualVal);
    return branchLogicalPC;
}

#endif /* CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS */
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/*
 * Emit code to do a gc rendezvous.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind)
{
    const void* target = CVMCCMruntimeGCRendezvousGlue;
    CVMInt32 helperLogicalPC;

    /* The second argument 'cbufRewind' is ignored. On MIPS, we
     * always continue the execution with the next instruction after
     * the gc rendezvous call.
     */

    /* Emit the gc rendezvous instruction(s) */
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeGCRendezvousGlue"));

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else 
#endif
    {
        helperLogicalPC = (CVMUint8*)target - con->codeEntry;
    }
    CVMMIPSemitJump(con, helperLogicalPC, 
                    CVM_TRUE /* link */,
                    CVM_FALSE /* don't fill delay slot */);
}
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

/*
 * CVMJITcanReachAddress - Check if toPC can be reached by an
 * instruction at fromPC using the specified addressing mode. If
 * needMargin is true, then a margin of safety is added (usually the
 * allowed offset range is cut in half).
 */
#ifdef CVMCPU_HAS_CP_REG
static int absolute(int v) {
    return (v<0)?-v:v;
}
#endif

CVMBool
CVMJITcanReachAddress(CVMJITCompilationContext* con,
                      int fromPC, int toPC,
                      CVMJITAddressMode mode, CVMBool needMargin)
{
    int diff = toPC - fromPC;
    switch (mode) {
        case CVMJIT_BRANCH_ADDRESS_MODE:
        case CVMJIT_COND_BRANCH_ADDRESS_MODE:
	    return (diff >= CVMMIPS_MIN_BRANCH_OFFSET &&
		    diff <= CVMMIPS_MAX_BRANCH_OFFSET);
        case CVMJIT_MEMSPEC_ADDRESS_MODE: {
#ifdef CVMCPU_HAS_CP_REG
	    CVMassert(needMargin == CVM_FALSE);
	    /* relative to cp base register */
	    diff = absolute(toPC - con->target.cpLogicalPC);
	    return (diff <= CVMCPU_MAX_LOADSTORE_OFFSET);
#else
	    CVMassert(CVM_FALSE); /* shouldn't be called if no cp */
#endif
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
    CVMInt32 offsetBits = targetLogicalAddress - instructionLogicalAddress;

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
        instruction &= 0xffff0000; /* mask off lower 16 bits */
        offsetBits = (targetLogicalAddress - instructionLogicalAddress - 
                      CVMCPU_INSTRUCTION_SIZE) >> 2;
        break;
    case CVMJIT_MEMSPEC_ADDRESS_MODE:
#ifdef CVMCPU_HAS_CP_REG
        /* offset is relative to cp base register */
        offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
#else
        CVMassert(CVM_FALSE); /* should never be called in this case */
#endif
	/* mask off lower 16 bits */
        instruction &= 0xffff0000;
        break;
    default:
        CVMassert(CVM_FALSE);
        break;
    }
    offsetBits &= 0x0000ffff;
    instruction |= offsetBits;
    *instructionPtr = instruction;
    CVMtraceJITCodegen(("--->Fixed instruction at %d(0x%x) to reference %d\n", 
			instructionLogicalAddress, instructionPtr,
			targetLogicalAddress));
}

#ifdef CVM_JIT_USE_FP_HARDWARE
static CVMUint32 getMipsFPOpcodes(int opcode)
{
    CVMUint32 mipsOpcode = 0;
    switch (opcode) {
    case CVMCPU_FNEG_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_NEG_OPCODE);
        break;
    case CVMCPU_FADD_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_ADD_OPCODE);
        break;
    case CVMCPU_FSUB_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_SUB_OPCODE);
        break;
    case CVMCPU_FMUL_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_MUL_OPCODE);
        break;
    case CVMCPU_FDIV_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_DIV_OPCODE);
        break;
    case CVMCPU_FCMP_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_CMP_OPCODE);
        break;
    case CVMCPU_FMOV_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_MOV_OPCODE);
        break;
    case CVMCPU_DNEG_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_NEG_OPCODE);
        break;
    case CVMCPU_DADD_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_ADD_OPCODE);
        break;
    case CVMCPU_DSUB_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_SUB_OPCODE);
        break;
    case CVMCPU_DMUL_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_MUL_OPCODE);
        break;
    case CVMCPU_DDIV_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_DIV_OPCODE);
        break;
    case CVMCPU_DCMP_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_CMP_OPCODE);
        break;
    case CVMMIPS_I2F_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_W_FMT | MIPS_FP_CVTS_OPCODE);
        break;
    case CVMMIPS_I2D_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_W_FMT | MIPS_FP_CVTD_OPCODE);
        break;
    case CVMMIPS_F2I_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_TRUNCW_OPCODE);
        break;
    case CVMMIPS_D2I_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_TRUNCW_OPCODE);
        break;
    case CVMMIPS_F2D_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_S_FMT | MIPS_FP_CVTD_OPCODE);
        break;
    case CVMMIPS_D2F_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_FP_D_FMT | MIPS_FP_CVTS_OPCODE);
        break;
    case CVMMIPS_MFC1_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_MFC1_OPCODE);
        break;
    case CVMMIPS_MTC1_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_MTC1_OPCODE);
        break;
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
    case CVMMIPS_DMFC1_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_DMFC1_OPCODE);
        break;
    case CVMMIPS_DMTC1_OPCODE:
        mipsOpcode = (MIPS_FP_COP1 | MIPS_DMTC1_OPCODE);
        break;	
#endif	
    default:
        CVMconsolePrintf("Unknown Floating Point Opcode");
        CVMassert(CVM_FALSE);
    }
    return mipsOpcode;
};

static CVMUint32 mipsFPCondCode(CVMCPUCondCode condCode)
{
    CVMUint32 mipsCondCode = 0;
    CVMBool unordedLT = CVM_FALSE;

    if (condCode & CVMCPU_COND_UNORDERED_LT) {
        /* unorded is treated as less than */
        unordedLT = CVM_TRUE;
        condCode &= ~CVMCPU_COND_UNORDERED_LT;
    }

    switch (condCode) {
        case CVMCPU_COND_FEQ:
        case CVMCPU_COND_FNE:
            /* condCode for FNE is the same as FEQ. In the conditional
             * branch after the compare, bc1f is used for FNE, and bc1t
             * is used for FEQ. */
            mipsCondCode = 0x2; break;
        case CVMCPU_COND_FGT: 
            if (unordedLT)  {
                mipsCondCode = 0x4; 
            } else {
                mipsCondCode = 0x5;
            }
            break;
        case CVMCPU_COND_FLT: {
	    if (unordedLT) {
                mipsCondCode = 0x5; /* unorded or less than */
            } else {
	        mipsCondCode = 0x4; /* less than */
            }
            break;
	}
        case CVMCPU_COND_FGE:
            if (unordedLT) {
	        mipsCondCode = 0x6;
            } else {
                mipsCondCode = 0x7;
            }
            break;
        case CVMCPU_COND_FLE: {
	    if (unordedLT) {
	        mipsCondCode = 0x7; /* unorded or less than or equal */
            } else {
                mipsCondCode = 0x6; /* less than or equal */
            }
            break;
        }
        default:
            CVMconsolePrintf("Unknown Floating Point condition code");
            CVMassert(CVM_FALSE);
    }
    return mipsCondCode;
}

#ifdef CVM_TRACE_JIT
static const char *getFPCondCodeName(CVMCPUCondCode condCode)
{
    const char *name = NULL;
    CVMBool unordedLT = CVM_FALSE;

    if (condCode & CVMCPU_COND_UNORDERED_LT) {
        unordedLT = CVM_TRUE;
        condCode &= ~CVMCPU_COND_UNORDERED_LT;
    }
    switch (condCode) {
    case CVMCPU_COND_FEQ:  name = "eq"; break;
    case CVMCPU_COND_FNE:  name = "ne"; break;
    case CVMCPU_COND_FLT:
    case CVMCPU_COND_FGT:
        if (unordedLT) {
            name = "ult";
        } else {
            name = "olt";
        }
        break;
    case CVMCPU_COND_FLE:
    case CVMCPU_COND_FGE:  
        if (unordedLT) {
            name = "ule";
        } else {
            name = "ole";
        }
        break;
    default:
        CVMconsolePrintf("Unknown Floating Point condition code");
        CVMassert(CVM_FALSE);
    }
    return name;
}

static const char *getFPOpcodeName(int opcode)
{
    const char *name = NULL;

    switch (opcode) {
    case CVMCPU_FNEG_OPCODE: name = "neg.s"; break;
    case CVMCPU_FADD_OPCODE: name = "add.s"; break;
    case CVMCPU_FSUB_OPCODE: name = "sub.s"; break;
    case CVMCPU_FMUL_OPCODE: name = "mul.s"; break;
    case CVMCPU_FDIV_OPCODE: name = "div.s"; break;
    case CVMCPU_FMOV_OPCODE: name = "mov.s"; break;

    case CVMCPU_DNEG_OPCODE: name = "neg.d"; break;
    case CVMCPU_DADD_OPCODE: name = "add.d"; break;
    case CVMCPU_DSUB_OPCODE: name = "sub.d"; break;
    case CVMCPU_DMUL_OPCODE: name = "mul.d"; break;
    case CVMCPU_DDIV_OPCODE: name = "div.d"; break;
    case CVMMIPS_I2F_OPCODE: name = "cvt.s.w"; break;
    case CVMMIPS_I2D_OPCODE: name = "cvt.d.w"; break;
    case CVMMIPS_F2D_OPCODE: name = "cvt.d.s"; break;
    case CVMMIPS_D2F_OPCODE: name = "cvt.s.d"; break;
    case CVMMIPS_F2I_OPCODE: name = "trunc.w.s"; break;
    case CVMMIPS_D2I_OPCODE: name = "trunc.w.d"; break;
    case CVMMIPS_MTC1_OPCODE: name = "mtc1"; break;
    case CVMMIPS_MFC1_OPCODE: name = "mfc1"; break;
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
    case CVMMIPS_DMTC1_OPCODE: name = "dmtc1"; break;
    case CVMMIPS_DMFC1_OPCODE: name = "dmfc1"; break;
#endif			      
    default:
        CVMconsolePrintf("Unknown FP opcode");
        CVMassert(CVM_FALSE);
    }
    return name;
}
#endif

/* Purpose: Emits instructions to do the specified floating point operation. */
void
CVMCPUemitBinaryFP(CVMJITCompilationContext* con,
		   int opcode, int destRegID, int lhsRegID, int rhsRegID)
{
    CVMUint32 mipsOpcode;
    CVMassert(opcode == CVMCPU_FADD_OPCODE || opcode == CVMCPU_FSUB_OPCODE ||
              opcode == CVMCPU_FMUL_OPCODE || opcode == CVMCPU_FDIV_OPCODE ||
              opcode == CVMCPU_DADD_OPCODE || opcode == CVMCPU_DSUB_OPCODE ||
              opcode == CVMCPU_DMUL_OPCODE || opcode == CVMCPU_DDIV_OPCODE );

    mipsOpcode = getMipsFPOpcodes(opcode);
    emitInstruction(con, mipsOpcode | rhsRegID << MIPS_FT_SHIFT |
                    lhsRegID << MIPS_FS_SHIFT | destRegID << MIPS_FD_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	$f%d, $f%d, $f%d",
                         getFPOpcodeName(opcode), destRegID,
                         lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
}

static void
CVMMIPSemitMFC1(CVMJITCompilationContext* con,
                int destRegID, int srcRegID)
{
    CVMUint32 mipsOpcode;
    mipsOpcode = getMipsFPOpcodes(CVMMIPS_MFC1_OPCODE);
    emitInstruction(con, mipsOpcode | destRegID << 16 | srcRegID << 11);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	mfc1	%s, $f%d",
                         regName(destRegID), srcRegID);
    });
    CVMJITdumpCodegenComments(con);
}

static void
CVMMIPSemitMTC1(CVMJITCompilationContext* con,
                int destRegID, int srcRegID)
{
    CVMUint32 mipsOpcode;
    mipsOpcode = getMipsFPOpcodes(CVMMIPS_MTC1_OPCODE);
    emitInstruction(con, mipsOpcode | srcRegID << 16 | destRegID << 11);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	mtc1	%s, $f%d",
                         regName(srcRegID), destRegID);
    });
    CVMJITdumpCodegenComments(con);
}

#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
static void
CVMMIPSemitDMFC1(CVMJITCompilationContext* con,
                int destRegID, int srcRegID)
{
    CVMUint32 mipsOpcode;
    mipsOpcode = getMipsFPOpcodes(CVMMIPS_DMFC1_OPCODE);
    emitInstruction(con, mipsOpcode | destRegID << 16 | srcRegID << 11);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	dmfc1	%s, $f%d",
                         regName(destRegID), srcRegID);
    });
    CVMJITdumpCodegenComments(con);
}

static void
CVMMIPSemitDMTC1(CVMJITCompilationContext* con,
                int destRegID, int srcRegID)
{
    CVMUint32 mipsOpcode;
    mipsOpcode = getMipsFPOpcodes(CVMMIPS_DMTC1_OPCODE);
    emitInstruction(con, mipsOpcode | srcRegID << 16 | destRegID << 11);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	dmtc1	%s, $f%d",
                         regName(srcRegID), destRegID);
    });
    CVMJITdumpCodegenComments(con);
}
#endif /* CVMMIPS_HAS_64BIT_FP_REGISTERS */

static void
CVMMIPSemitUnaryFP0(CVMJITCompilationContext* con,
                    int opcode, int destRegID, int srcRegID)
{
    CVMUint32 mipsOpcode = getMipsFPOpcodes(opcode);
    emitInstruction(con, mipsOpcode | srcRegID << MIPS_FS_SHIFT |
                    destRegID << MIPS_FD_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	$f%d, $f%d",
                     getFPOpcodeName(opcode), destRegID, srcRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitUnaryFP(CVMJITCompilationContext* con,
		  int opcode, int destRegID, int srcRegID)
{
    if (opcode != CVMCPU_FNEG_OPCODE && opcode != CVMCPU_DNEG_OPCODE && 
	opcode != CVMCPU_FMOV_OPCODE && opcode != CVMMIPS_I2F_OPCODE &&
	opcode != CVMMIPS_F2I_OPCODE && opcode != CVMMIPS_I2D_OPCODE &&
	opcode != CVMMIPS_D2I_OPCODE && opcode != CVMMIPS_F2D_OPCODE &&
	opcode != CVMMIPS_D2F_OPCODE &&
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
	opcode != CVMMIPS_DMFC1_OPCODE && opcode != CVMMIPS_DMTC1_OPCODE &&
#endif
	opcode != CVMMIPS_MFC1_OPCODE && opcode != CVMMIPS_MTC1_OPCODE) {
	CVMassert(CVM_FALSE);
    }
    
    switch (opcode) {
    case CVMCPU_FNEG_OPCODE:
    case CVMCPU_DNEG_OPCODE:
    case CVMCPU_FMOV_OPCODE: 
    case CVMMIPS_F2D_OPCODE:
    case CVMMIPS_D2F_OPCODE:
        CVMMIPSemitUnaryFP0(con, opcode, destRegID, srcRegID);
        break;
    case CVMMIPS_I2F_OPCODE:
    case CVMMIPS_I2D_OPCODE:
        CVMMIPSemitMTC1(con, destRegID, srcRegID);
        CVMMIPSemitUnaryFP0(con, opcode, destRegID, destRegID);
        break;
    case CVMMIPS_F2I_OPCODE:
    case CVMMIPS_D2I_OPCODE: {
        CVMRMResource* tmp = CVMRMgetResource(CVMRM_FP_REGS(con),
                             CVMRM_FP_ANY_SET, CVMRM_EMPTY_SET, 1);
        int tmpFPReg = CVMRMgetRegisterNumber(tmp);
        CVMMIPSemitUnaryFP0(con, opcode, tmpFPReg, srcRegID);
        CVMMIPSemitMFC1(con, destRegID, tmpFPReg);
        CVMRMrelinquishResource(CVMRM_FP_REGS(con), tmp);
        break;
    }
    case CVMMIPS_MFC1_OPCODE:
        CVMMIPSemitMFC1(con, destRegID, srcRegID);
        break;
    case CVMMIPS_MTC1_OPCODE:
        CVMMIPSemitMTC1(con, destRegID, srcRegID);
        break;
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
    case CVMMIPS_DMFC1_OPCODE:
        CVMMIPSemitDMFC1(con, destRegID, srcRegID);
        break;
    case CVMMIPS_DMTC1_OPCODE:
        CVMMIPSemitDMTC1(con, destRegID, srcRegID);
        break;	
#endif
    default:
        CVMconsolePrintf("Unknown FP opcode");
        CVMassert(CVM_FALSE);
    }
}

void
CVMCPUemitFCompare(CVMJITCompilationContext* con,
                   int opcode, CVMCPUCondCode condCode,
		   int lhsRegID, int rhsRegID)
{
    CVMUint32 mipsOpcode;
    CVMUint32 mipsCondCode;
    CVMassert(opcode == CVMCPU_FCMP_OPCODE || opcode == CVMCPU_DCMP_OPCODE);

    if ((condCode & ~CVMCPU_COND_UNORDERED_LT) == CVMCPU_COND_FGT || 
        (condCode & ~CVMCPU_COND_UNORDERED_LT) == CVMCPU_COND_FGE) {
        int tmpRegID;
        tmpRegID = lhsRegID;
        lhsRegID = rhsRegID;
        rhsRegID = tmpRegID;
    }

    mipsOpcode = getMipsFPOpcodes(opcode);
    mipsCondCode = mipsFPCondCode(condCode);
    emitInstruction(con, mipsOpcode | rhsRegID << MIPS_FT_SHIFT |
                    lhsRegID << MIPS_FS_SHIFT | mipsCondCode);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	c.%s.%s	$f%d, $f%d",
            getFPCondCodeName(condCode), 
            (opcode == CVMCPU_FCMP_OPCODE ? "s" : "d"),
            lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitLoadConstantFP(CVMJITCompilationContext *con,
                         int regID, CVMInt32 v)
{
    /*
     * there is no way to construct a constant in a floating-point register,
     * so we must load a constant from the constant pool or build it up
     * in GPR and then move it to an FPR
     */
#ifdef CVMCPU_HAS_CP_REG
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    CVMInt32 offset;
    offset = CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR32_OPCODE, regID, CVMCPU_CP_REG, offset);
#else
    CVMCPUemitLoadConstant(con, CVMMIPS_t7, v);
    CVMMIPSemitMTC1(con, regID, CVMMIPS_t7);
#endif
}

/* Purpose: Loads a 64-bit double constant into an FP register. */

void
CVMCPUemitLoadLongConstantFP(CVMJITCompilationContext *con, int regID,
                             CVMJavaVal64 *value)
{
#if defined(CVMMIPS_HAS_64BIT_FP_REGISTERS) && defined(CVMCPU_HAS_CP_REG)
    /*
     * With 64-bit fp support, we just use LDC1 since the constant pool
     * entry is guaranteed to be doubleword aligned.
     */
    CVMInt32 offset;
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    offset = CVMJITgetRuntimeConstantReference64(con, logicalPC, value);

    CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD);
    emitInstruction(con, MIPS_LDC1_OPCODE | CVMCPU_CP_REG << MIPS_RS_SHIFT |
		    regID << MIPS_I_RD_SHIFT | (offset & 0xffff));
    CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	%s	$f%d, %d(%s)",
			     getMemOpcodeName(MIPS_LDC1_OPCODE),
			     regID, offset, regName(CVMCPU_CP_REG));
    });
    CVMJITdumpCodegenComments(con);
#else
    /* load 64-bit value into two 32-bit fp registers */
    CVMJavaVal64 val;
    CVMmemCopy64(val.v, value->v);
    CVMCPUemitLoadConstantFP(con, LOREGFP(regID), val.v[0]);
    CVMCPUemitLoadConstantFP(con, HIREGFP(regID), val.v[1]);
#endif
}

#endif /* CVM_JIT_USE_FP_HARDWARE */

#ifdef CVMCPU_HAS_64BIT_REGISTERS

static void
CVMMIPSemitDoublewordShiftPlus32(CVMJITCompilationContext* con, 
                                 int opcode, int destRegID, int srcRegID,
                                 CVMUint32 shiftAmount)
{
    CVMassert((shiftAmount >> 5) == 0);
    emitInstruction(con, 
        srcRegID << MIPS_R_RT_SHIFT | destRegID << MIPS_R_RD_SHIFT |
        shiftAmount << MIPS_SA_SHIFT | (CVMUint32)opcode);
    CVMtraceJITCodegenExec({
        char *name = NULL;
        printPC(con);
        switch (opcode) {
            case MIPS_DSLL32_OPCODE: name = "dsll32"; break;
            case MIPS_DSRL32_OPCODE: name = "dsrl32"; break;
            case MIPS_DSRA32_OPCODE: name = "dsra32"; break;
            default: CVMassert(CVM_FALSE);
        }
        CVMconsolePrintf("	%s	%s, %s, %d", name,
                         regName(destRegID), regName(srcRegID),
                         shiftAmount);
    });
    CVMJITdumpCodegenComments(con);
}

/*
 * WARNING: the srcRegID register pair will get trashed. Use with caution.
 */
void
CVMCPUemitMoveTo64BitRegister(CVMJITCompilationContext* con,
                              int destRegID, int srcRegID)
{
    /* Implemented as:
     *
     *   dsll32	t7, HI(src), 0
     *   dsll32	LO(SRC), LO(SRC), 0
     *   dsrl32	LO(SRC), LO(SRC), 0     @ clear hi 32 bits
     *   or	dest, t7, LO(SRC)
     */

    /* 1. dsll32 t7, hi, 0 */ 
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSLL32_OPCODE,
                                     CVMMIPS_t7, HIREG(srcRegID), 0);
    /* 2. clear h32 of lo */
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSLL32_OPCODE,
                                     LOREG(srcRegID), LOREG(srcRegID), 0);
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSRL32_OPCODE,
                                     LOREG(srcRegID), LOREG(srcRegID), 0);
    /* 3. or dest, t7, lo */
    CVMCPUemitBinaryALURegister(con, CVMCPU_OR_OPCODE, destRegID, 
                                CVMMIPS_t7, LOREG(srcRegID),
                                CVMJIT_NOSETCC);
}

void
CVMCPUemitMoveFrom64BitRegister(CVMJITCompilationContext* con,
                                int destRegID, int srcRegID)
{
    /* Implemented as:
     *
     *   dsll32 t7, src, 0
     *   dsra32	hi, src, 0
     *   dsra32 lo, t7, 0
     */
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSLL32_OPCODE,
                                     CVMMIPS_t7, srcRegID, 0);
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSRA32_OPCODE,
                                     HIREG(destRegID), srcRegID, 0);
    CVMMIPSemitDoublewordShiftPlus32(con, MIPS_DSRA32_OPCODE,
                                     LOREG(destRegID), CVMMIPS_t7, 0);
}
#endif /* CVMCPU_HAS_64BIT_REGISTERS */

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#if CVMCPU_MAX_ARG_REGS != 8

#ifdef CVMJIT_INTRINSICS
/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. */
extern CVMJITRegsRequiredType
CVMMIPSCCALLgetRequired(CVMJITCompilationContext *con,
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
	    int argSize = CVMMIPSCCALLargSize(argType);

	    if (argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
	        regno = CVMCPU_ARG1_REG + argWordIndex;
	    } else {
	        /* IAI-22 */
	        regno = (CVMMIPS_t0 + argWordIndex - CVMCPU_MAX_ARG_REGS);
		/* Make sure that we are not allocating more than 8 words
                   of args. Although we could choose to support more
                   are registers if we wanted, it is not needed. */
		CVMassert(regno + argSize - 1 < CVMMIPS_t4);
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
    int argSize = CVMMIPSCCALLargSize(argType);

    if (useRegArgs || argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
        int regno;
        if (argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
            regno = CVMCPU_ARG1_REG + argWordIndex;
	} else {
	    /* IAI-22 */
	    CVMassert(useRegArgs == CVM_TRUE);
	    regno = (CVMMIPS_t0 + argWordIndex - CVMCPU_MAX_ARG_REGS);
            /* Make sure that we are not allocating more than 8 words
               of args. Although we could choose to support more
               registers if we wanted, it is not needed. */
	    CVMassert(regno + argSize - 1 < CVMMIPS_t4);
	}
        arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
        CVMassert(regno - CVMCPU_ARG1_REG + arg->size <= CVMCPU_MAX_ARG_REGS
                  || useRegArgs);
        *outgoingRegs |= arg->rmask;
    } else {
        /* Save arguments to the C stack */
        int stackIndex = argWordIndex * 4;
        /* no more than 8 words of args allowed */
        CVMassert(argWordIndex < 8);
        CVMRMpinResource(CVMRM_INT_REGS(con), arg,
			 CVMRM_ANY_SET, CVMRM_EMPTY_SET);
        if (argSize == 1) {
            CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
                CVMRMgetRegisterNumber(arg), CVMCPU_SP_REG, stackIndex);
        } else {
	    /* Assert argument is 64-bit aligned. If this ever fails then
	     * we need to actually do the alignment. */
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

#endif /* CVMCPU_MAX_ARG_REGS != 8 */
