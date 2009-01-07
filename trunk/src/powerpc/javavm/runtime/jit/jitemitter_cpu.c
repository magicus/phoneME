/*
 * @(#)jitemitter_cpu.c	1.59 06/10/10
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
 * PowerPC-specific bits.
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

/* The maximum reach for a pc relative branch (+/- 25 bits) */
#define CVMPPC_MAX_BRANCH_OFFSET ( 32*1024*1024-4)
#define CVMPPC_MIN_BRANCH_OFFSET (-32*1024*1024)
/* The maximum reach for a conditional pc relative branch (+/- 15 bits) */
#define CVMPPC_MAX_COND_BRANCH_OFFSET ( 32*1024-4)
#define CVMPPC_MIN_COND_BRANCH_OFFSET (-32*1024)

/**************************************************************
 * Private #defines for opcode encodings.
 **************************************************************/

#define PPC_ME_SHIFT 1   /* mask end */
#define PPC_MB_SHIFT 6   /* mask begin */
#define PPC_SH_SHIFT 11  /* shift amount */
#define PPC_RC_SHIFT 6   /* arg C register */
#define PPC_RB_SHIFT 11  /* arg B register */
#define PPC_RA_SHIFT 16  /* arg A register */
#define PPC_RS_SHIFT 21  /* source register */
#define PPC_RD_SHIFT 21  /* dest register */
#define PPC_OPCODE_SHIFT 26u  /* opcode in upper 6 bits */
#define PPC_SPR_LO_SHIFT 16   /* for mtspr and mfspr (mtlr and mflr) */

#define PPC_BI_SHIFT 16  /* BI field specifies condition to check for */
#define PPC_BO_SHIFT 21  /* BO field specifies type of branch */
#define PPC_BO_BRANCH_ALWAYS_BITS (0x14)
#define PPC_BO_BRANCH_TRUE_BITS   (0x0c)
#define PPC_BO_BRANCH_FALSE_BITS  (0x04)
#define PPC_BRANCH_LINK_BIT  1    /* do a branch and link */
#define PPC_BRANCH_OFFSET_SHIFT

#define PPC_RECORD_BIT  1    /* do a "setcc" */

#define PPC_ADD_OPCODE    (31u << PPC_OPCODE_SHIFT | 266u << 1)
#define PPC_ADDC_OPCODE   (31u << PPC_OPCODE_SHIFT |  10u << 1)
#define PPC_ADDI_OPCODE   (14u << PPC_OPCODE_SHIFT)
#define PPC_ADDIS_OPCODE  (15u << PPC_OPCODE_SHIFT)
#define PPC_ADDIC_OPCODE  (13u << PPC_OPCODE_SHIFT)
#define PPC_ADDE_OPCODE   (31u << PPC_OPCODE_SHIFT | 138u << 1)
#define PPC_SUBF_OPCODE   (31u << PPC_OPCODE_SHIFT |  40u << 1)
#define PPC_SUBFC_OPCODE  (31u << PPC_OPCODE_SHIFT |   8u << 1)
#define PPC_SUBFE_OPCODE  (31u << PPC_OPCODE_SHIFT | 136u << 1)
#define PPC_SUBFIC_OPCODE (8u  << PPC_OPCODE_SHIFT)
#define PPC_SUBFZE_OPCODE (31u << PPC_OPCODE_SHIFT | 200u << 1)
#define PPC_DIVW_OPCODE   (31u << PPC_OPCODE_SHIFT | 491u << 1)
#define PPC_MULLW_OPCODE  (31u << PPC_OPCODE_SHIFT | 235u << 1)
#define PPC_MULHW_OPCODE  (31u << PPC_OPCODE_SHIFT |  75u << 1)
#define PPC_MULHWU_OPCODE (31u << PPC_OPCODE_SHIFT |  11u << 1)
#define PPC_MULLI_OPCODE  (7u  << PPC_OPCODE_SHIFT)
#define PPC_AND_OPCODE    (31u << PPC_OPCODE_SHIFT |  28u << 1)
#define PPC_ANDI_OPCODE   (28u << PPC_OPCODE_SHIFT)
#define PPC_ANDIS_OPCODE  (29u << PPC_OPCODE_SHIFT)
#define PPC_ANDC_OPCODE   (31u << PPC_OPCODE_SHIFT |  60u << 1)
#define PPC_OR_OPCODE     (31u << PPC_OPCODE_SHIFT | 444u << 1)
#define PPC_ORI_OPCODE    (24u << PPC_OPCODE_SHIFT)
#define PPC_ORIS_OPCODE   (25u << PPC_OPCODE_SHIFT)
#define PPC_XOR_OPCODE    (31u << PPC_OPCODE_SHIFT | 316u << 1)
#define PPC_XORI_OPCODE   (26u << PPC_OPCODE_SHIFT)
#define PPC_XORIS_OPCODE  (27u << PPC_OPCODE_SHIFT)

#define PPC_NEG_OPCODE    (31u << PPC_OPCODE_SHIFT | 104u << 1)

#define PPC_RLWINM_OPCODE (21u << PPC_OPCODE_SHIFT)
#define PPC_CLRRWI_OPCODE PPC_RLWINM_OPCODE
#define PPC_CLRLWI_OPCODE PPC_RLWINM_OPCODE

#define PPC_SLW_OPCODE    (31u << PPC_OPCODE_SHIFT |  24u << 1)
#define PPC_SLWI_OPCODE   PPC_RLWINM_OPCODE
#define PPC_SRW_OPCODE    (31u << PPC_OPCODE_SHIFT | 536u << 1)
#define PPC_SRWI_OPCODE   PPC_RLWINM_OPCODE
#define PPC_SRAW_OPCODE   (31u << PPC_OPCODE_SHIFT | 792u << 1)
#define PPC_SRAWI_OPCODE  (31u << PPC_OPCODE_SHIFT | 824u << 1)

#define PPC_CMP_OPCODE    (31u << PPC_OPCODE_SHIFT |   0u << 1)
#define PPC_CMPI_OPCODE   (11u << PPC_OPCODE_SHIFT)
#define PPC_CMPL_OPCODE   (31u << PPC_OPCODE_SHIFT |  32u << 1)
#define PPC_CMPLI_OPCODE  (10u << PPC_OPCODE_SHIFT)

#define PPC_MTLR_OPCODE   (31u << PPC_OPCODE_SHIFT | 467u << 1 | \
			   8 << PPC_SPR_LO_SHIFT)
#define PPC_MFLR_OPCODE   (31u << PPC_OPCODE_SHIFT | 339u << 1 | \
			   8 << PPC_SPR_LO_SHIFT)

#define PPC_BLR_OPCODE    (19u << PPC_OPCODE_SHIFT | 16u << 1)
#define PPC_B_OPCODE      (18u << PPC_OPCODE_SHIFT)
#define PPC_BC_OPCODE     (16u << PPC_OPCODE_SHIFT)

#define PPC_LWZ_OPCODE   (32u << PPC_OPCODE_SHIFT)
#define PPC_LWZX_OPCODE  (31u << PPC_OPCODE_SHIFT |  23u << 1)
#define PPC_LWZU_OPCODE  (33u << PPC_OPCODE_SHIFT)

#define PPC_LHA_OPCODE   (42u << PPC_OPCODE_SHIFT)
#define PPC_LHAX_OPCODE  (31u << PPC_OPCODE_SHIFT | 343u << 1)
#define PPC_LHAU_OPCODE  (43u << PPC_OPCODE_SHIFT)

#define PPC_LHZ_OPCODE   (40u << PPC_OPCODE_SHIFT)
#define PPC_LHZX_OPCODE  (31u << PPC_OPCODE_SHIFT | 279u << 1)
#define PPC_LHZU_OPCODE  (41u << PPC_OPCODE_SHIFT)

#define PPC_LBZ_OPCODE   (34u << PPC_OPCODE_SHIFT)
#define PPC_LBZX_OPCODE  (31u << PPC_OPCODE_SHIFT | 119u << 1)
#define PPC_LBZU_OPCODE  (35u << PPC_OPCODE_SHIFT)

#define PPC_STW_OPCODE   (36u << PPC_OPCODE_SHIFT)
#define PPC_STWX_OPCODE  (31u << PPC_OPCODE_SHIFT |  151u << 1)
#define PPC_STWU_OPCODE  (37u << PPC_OPCODE_SHIFT)

#define PPC_STH_OPCODE   (44u << PPC_OPCODE_SHIFT)
#define PPC_STHX_OPCODE  (31u << PPC_OPCODE_SHIFT | 407u << 1)
#define PPC_STHU_OPCODE  (45u << PPC_OPCODE_SHIFT)

#define PPC_STB_OPCODE   (38u << PPC_OPCODE_SHIFT)
#define PPC_STBX_OPCODE  (31u << PPC_OPCODE_SHIFT | 215u << 1)
#define PPC_STBU_OPCODE  (39u << PPC_OPCODE_SHIFT)

#define PPC_EXTSB_OPCODE (31u << PPC_OPCODE_SHIFT | 954u << 1)

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#define PPC_LWARX_OPCODE (31u << PPC_OPCODE_SHIFT | 20u << 1)
#define PPC_STWCX_OPCODE (31u << PPC_OPCODE_SHIFT | 150u << 1 | 1)
#endif

#ifdef CVM_JIT_USE_FP_HARDWARE
/* opcodes only used for floating arithmetic */

#define PPC_CROR_OPCODE    (19u << PPC_OPCODE_SHIFT | 449u << 1)

#define PPC_FADD_OPCODE    (63u << PPC_OPCODE_SHIFT | 21u << 1)
#define PPC_FADDS_OPCODE   (59u << PPC_OPCODE_SHIFT | 21u << 1)
#define PPC_FCMPU_OPCODE   (63u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_FDIV_OPCODE    (63u << PPC_OPCODE_SHIFT | 18u << 1)
#define PPC_FDIVS_OPCODE   (59u << PPC_OPCODE_SHIFT | 18u << 1)
#define PPC_FMR_OPCODE     (63u << PPC_OPCODE_SHIFT | 72u << 1)
#define PPC_FMUL_OPCODE    (63u << PPC_OPCODE_SHIFT | 25u << 1)
#define PPC_FMULS_OPCODE   (59u << PPC_OPCODE_SHIFT | 25u << 1)
#define PPC_FNEG_OPCODE    (63u << PPC_OPCODE_SHIFT | 40u << 1)
#define PPC_FRSP_OPCODE    (63u << PPC_OPCODE_SHIFT | 12u << 1)
#define PPC_FSUB_OPCODE    (63u << PPC_OPCODE_SHIFT | 20u << 1)
#define PPC_FSUBS_OPCODE   (59u << PPC_OPCODE_SHIFT | 20u << 1)

/* WARNING: using fmadd results in a non-compliant vm. Some floating
 * point tck tests will fail. Therefore, this support if off by defualt.
 */
#ifdef CVM_JIT_USE_FMADD
#define PPC_FMADD_OPCODE   (59u << PPC_OPCODE_SHIFT | 29u << 1)
#define PPC_DMADD_OPCODE   (63u << PPC_OPCODE_SHIFT | 29u << 1)
#define PPC_FMSUB_OPCODE   (59u << PPC_OPCODE_SHIFT | 28u << 1)
#define PPC_DMSUB_OPCODE   (63u << PPC_OPCODE_SHIFT | 28u << 1)
#define PPC_FNMADD_OPCODE  (59u << PPC_OPCODE_SHIFT | 31u << 1)
#define PPC_DNMADD_OPCODE  (63u << PPC_OPCODE_SHIFT | 31u << 1)
#define PPC_FNMSUB_OPCODE  (59u << PPC_OPCODE_SHIFT | 30u << 1)
#define PPC_DNMSUB_OPCODE  (63u << PPC_OPCODE_SHIFT | 30u << 1)
#endif

#define PPC_LFD_OPCODE     (50u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_LFDU_OPCODE    (51u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_LFDX_OPCODE    (31u << PPC_OPCODE_SHIFT | 599u << 1)
#define PPC_LFS_OPCODE     (48u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_LFSU_OPCODE    (49u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_LFSX_OPCODE    (31u << PPC_OPCODE_SHIFT | 535u << 1)

#define PPC_STFD_OPCODE    (54u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_STFDU_OPCODE   (55u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_STFDX_OPCODE   (31u << PPC_OPCODE_SHIFT | 727u << 1)
#define PPC_STFS_OPCODE    (52u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_STFSU_OPCODE   (53u << PPC_OPCODE_SHIFT | 00u << 1)
#define PPC_STFSX_OPCODE   (31u << PPC_OPCODE_SHIFT | 663u << 1)
#endif

#ifdef CVM_PPC_E500V1
/* e500 opcodes. */
#define PPC_E500_EFSADD_OPCODE    (4u << PPC_OPCODE_SHIFT | 0x2C0)
#define PPC_E500_EFSSUB_OPCODE    (4u << PPC_OPCODE_SHIFT | 0x2C1)
#define PPC_E500_EFSMUL_OPCODE    (4u << PPC_OPCODE_SHIFT | 0x2C8)
#define PPC_E500_EFSDIV_OPCODE    (4u << PPC_OPCODE_SHIFT | 0x2C9)
#define PPC_E500_EFSNEG_OPCODE    (4u << PPC_OPCODE_SHIFT | 0x2C6)
#define PPC_E500_EFSCFSI_OPCODE   (4u << PPC_OPCODE_SHIFT | 0x2D1)
#define PPC_E500_EFSCTSI_OPCODE   (4u << PPC_OPCODE_SHIFT | 0x2D5)
#define PPC_E500_EFSCMPEQ_OPCODE  (4u << PPC_OPCODE_SHIFT | 0x2CE)
#define PPC_E500_EFSCMPLT_OPCODE  (4u << PPC_OPCODE_SHIFT | 0x2CD)
#define PPC_E500_EFSCMPGT_OPCODE  (4u << PPC_OPCODE_SHIFT | 0x2CC)
#endif

/**************************************************************
 * PowerPC condition code related stuff. PowerPC doesn't have
 * simple 4-bit (N, Z, V, and C) condition codes like ARM and 
 * SPARC, so we can't simply mask CVMCPUCondCodes into the
 * instructions.
 **************************************************************/

/* CR field bits. Only one of the first 3 bits is ever set. */
enum CVMPPCRFieldBit {
    PPC_CRFIELD_LT = 0,  /* bit 31 set */
    PPC_CRFIELD_GT = 1,  /* bit 31 clear and result not 0 */
    PPC_CRFIELD_EQ = 2,  /* result is 0 */
    PPC_CRFIELD_SO = 3  /* summary overflow */
};
typedef enum CVMPPCRFieldBit CVMPPCRFieldBit;

/*
 * Mapping between CVMCPUCondCodes and instruction endcoding information.
 * Since conditional branches can't differentiate between unsigned and
 * signed results, we need to make sure that the cmp emitter knows which
 * kind of result is needed, thus the need for the "signedCompare" field.
 * testForTrue and crFieldBit are used for encoding the conditional
 * branch.
 *
 * WARNING: The PowerPC LT bit is like the ARM N bit. ARM checks for LT
 * by ckecking for (N & ~V) || (~N & V). You can't do this on PowerPC in
 * one instruction. However, currently we never check for LT in cases where
 * an overflow can happen. Same for GT, LE, and GE, which also have the same
 * problem. Therefore on PowerPC we can get away with just checking the LT
 * bit and ignoring the SO bit.
 */
typedef struct CVMPPCCondition CVMPPCCondition;
struct CVMPPCCondition {
    CVMUint8        signedCompare;  /* 1 if doing a signed compare */
    CVMUint8        branchBOBits;   /* bits for the branch BO field */
    CVMPPCRFieldBit crFieldBit;     /* CR Field bit to check */
};

static const CVMPPCCondition ppcConditions[] = {
    /* EQ */ {1, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_EQ},
    /* NE */ {1, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_EQ},
    /* MI */ {1, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_LT},
    /* PL */ {1, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_LT},
    /* OV */ {1, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_SO},
    /* NO */ {1, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_SO},
    /* LT */ {1, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_LT}, /* SO ignored. */
    /* GT */ {1, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_GT}, /* SO ignored. */
    /* LE */ {1, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_GT}, /* SO ignored. */
    /* GE */ {1, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_LT}, /* SO ignored. */
    /* LO */ {0, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_LT},
    /* HI */ {0, PPC_BO_BRANCH_TRUE_BITS,  PPC_CRFIELD_GT},
    /* LS */ {0, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_GT},
    /* HS */ {0, PPC_BO_BRANCH_FALSE_BITS, PPC_CRFIELD_LT},
    /* AL */ {0, PPC_BO_BRANCH_ALWAYS_BITS, 0},
    /* NV */ {0, 0, 0}, /* unsupported */
};

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
        case CVMPPC_ANDIS_OPCODE:
        case CVMPPC_ORIS_OPCODE:
        case CVMPPC_XORIS_OPCODE:
        case CVMPPC_ADDIS_OPCODE: {
	    CVMassert((constValue & 0xffff0000) == 0);
	    return CVM_TRUE;
	}

        case CVMCPU_AND_OPCODE:
        case CVMCPU_OR_OPCODE:
        case CVMCPU_XOR_OPCODE: {
	    return ((constValue & 0xffff0000) == 0);
	}
    
        case CVMCPU_SUB_OPCODE:
	    constValue = -constValue;
        case CVMPPC_SUBFIC_OPCODE:
        case CVMCPU_ADD_OPCODE:
        case CVMCPU_MOV_OPCODE:
        case CVMCPU_MULL_OPCODE:
        case CVMCPU_CMP_OPCODE:
        case CVMCPU_CMN_OPCODE: {
	    return (constValue >= -32768 && constValue < 32768);
	}
        case CVMPPC_RLWINM_OPCODE:
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
    ap->type = CVMPPC_ALURHS_CONSTANT;
    ap->constValue = v;
    return ap;
}

/* Purpose: Constructs a register type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewRegister(CVMJITCompilationContext* con, CVMRMResource* rp)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMPPC_ALURHS_REGISTER;
    ap->r = rp;
    return ap;
}

/* Purpose: Encodes the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */

CVMCPUALURhsToken
CVMPPCalurhsGetToken(const CVMCPUALURhs *aluRhs)
{
    CVMCPUALURhsToken result;
    if (aluRhs->type == CVMPPC_ALURHS_REGISTER) {
	int regNo = CVMRMgetRegisterNumber(aluRhs->r);
        result = CVMPPCalurhsEncodeRegisterToken(regNo);
    } else {
	CVMassert(aluRhs->type == CVMPPC_ALURHS_CONSTANT);
	result = CVMPPCalurhsEncodeConstantToken(aluRhs->constValue);
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
    case CVMPPC_ALURHS_CONSTANT:
        /* If we can't encode the constant as a immediate, then
           load it into a register and re-write the aluRhs as a 
           CVMPPC_ALURHS_REGISTER: 
         */
        if (!CVMCPUalurhsIsEncodableAsImmediate(opcode, ap->constValue)) {
            ap->type = CVMPPC_ALURHS_REGISTER;
            ap->r = CVMRMgetResourceForConstant32(con, target, avoid,
                                                  ap->constValue);
            ap->constValue = 0;
        }
        return;
    case CVMPPC_ALURHS_REGISTER:
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
    case CVMPPC_ALURHS_CONSTANT:
        return;
    case CVMPPC_ALURHS_REGISTER:
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

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMPPCmemspecGetToken(const CVMCPUMemSpec *memSpec)
{
    int type = memSpec->type;
    if (type == CVMCPU_MEMSPEC_REG_OFFSET) {
	int regno = CVMRMgetRegisterNumber(memSpec->offsetReg);
	return CVMPPCmemspecEncodeToken(type, regno);
    } else {
	return CVMPPCmemspecEncodeToken(type, memSpec->offsetValue);
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

/* Private PPC specific defintions: ===================================== */

/* mapped from CVMCPUCondCode defined in jitriscemitterdefs_cpu.h*/
const CVMCPUCondCode CVMCPUoppositeCondCode[] = {
    CVMCPU_COND_NE, CVMCPU_COND_EQ, CVMCPU_COND_PL, CVMCPU_COND_MI,
    CVMCPU_COND_NO, CVMCPU_COND_OV, CVMCPU_COND_GE, CVMCPU_COND_LE,
    CVMCPU_COND_GT, CVMCPU_COND_LT, CVMCPU_COND_HS, CVMCPU_COND_LS,
    CVMCPU_COND_HI, CVMCPU_COND_LO, CVMCPU_COND_NV, CVMCPU_COND_AL
};

#ifdef CVM_JIT_USE_FP_HARDWARE
/*
 * When doing floating compare, under what circumstances do we
 * need to merge the unordered result into another result?
 * -1 => impossible cases
 *  0 => no merging necessary
 *  1 => add cror 1,1,3 after compare
 *  2 => add cror 0,0,3 after compare
 * Index this table using CVMCPU_COND_UNORDERED_LT bit, if any,
 * in place.
 */
const CVMInt8 needFloatConditionBitMerge[] = {
/* part one: treat Unordered as GT */
/*  EQ,  NE,  MI,  PL,  OV,  NO,  LT,  GT */
     0,   0,  -1,  -1,  -1,  -1,   0,   1,
/*  LE,  GE,  LO,  HI,  LS,  HS,  AL,  NV */
     1,   0,  -1,  -1,  -1,  -1,   0,   0,
/* part two: treat Unordered as LT */
/*  EQ,  NE,  MI,  PL,  OV,  NO,  LT,  GT */
     0,   0,  -1,  -1,  -1,  -1,   2,   0,
/*  LE,  GE,  LO,  HI,  LS,  HS,  AL,  NV */
     0,   2,  -1,  -1,  -1,  -1,   0,   0,
};
#endif

#ifdef CVM_TRACE_JIT
static const char* const conditions[] = {
    "eq", "ne", "mi", "pl", "ov", "no", "lt", "gt", 
    "le", "ge", "lo", "hi", "ls", "hs", "", "nv"
};

/* Purpose: Returns the name of the specified opcode. */
static const char *getMemOpcodeName(CVMUint32 ppcOpcode)
{
    const char *name = NULL;
    switch (ppcOpcode) {
        case PPC_LWZ_OPCODE:   name = "lwz"; break;
        case PPC_LWZX_OPCODE:  name = "lwzx"; break;
        case PPC_LWZU_OPCODE:  name = "lwzu"; break;
	
        case PPC_LHA_OPCODE:   name = "lha"; break;
        case PPC_LHAX_OPCODE:  name = "lhax"; break;
        case PPC_LHAU_OPCODE:  name = "lhau"; break;
	
        case PPC_LHZ_OPCODE:   name = "lhz"; break;
        case PPC_LHZX_OPCODE:  name = "lhzx"; break;
        case PPC_LHZU_OPCODE:  name = "lhzu"; break;
	
        case PPC_LBZ_OPCODE:   name = "lbz"; break;
        case PPC_LBZX_OPCODE:  name = "lbzx"; break;
        case PPC_LBZU_OPCODE:  name = "lbzu"; break;
	
        case PPC_STW_OPCODE:   name = "stw"; break;
        case PPC_STWX_OPCODE:  name = "stwx"; break;
        case PPC_STWU_OPCODE:  name = "stwu"; break;
	
        case PPC_STH_OPCODE:   name = "sth"; break;
        case PPC_STHX_OPCODE:  name = "sthx"; break;
        case PPC_STHU_OPCODE:  name = "sthu"; break;
	
        case PPC_STB_OPCODE:   name = "stb"; break;
        case PPC_STBX_OPCODE:  name = "stbx"; break;
        case PPC_STBU_OPCODE:  name = "stbu"; break;

#ifdef CVM_JIT_USE_FP_HARDWARE
	case PPC_LFD_OPCODE:   name = "lfd"; break;
	case PPC_LFDU_OPCODE:  name = "lfdu"; break;
	case PPC_LFDX_OPCODE:  name = "lfdx"; break;

	case PPC_LFS_OPCODE:   name = "lfs"; break;
	case PPC_LFSU_OPCODE:  name = "lfsu"; break;
	case PPC_LFSX_OPCODE:  name = "lfsx"; break;

	case PPC_STFD_OPCODE:   name = "stfd"; break;
	case PPC_STFDU_OPCODE:  name = "stfdu"; break;
	case PPC_STFDX_OPCODE:  name = "stfdx"; break;

	case PPC_STFS_OPCODE:   name = "stfs"; break;
	case PPC_STFSU_OPCODE:  name = "stfsu"; break;
	case PPC_STFSX_OPCODE:  name = "stfsx"; break;
#endif
        default:
	    CVMconsolePrintf("Unknown opcode 0x%08x", ppcOpcode);
	    CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;  
}

static const char *getALUOpcodeName(CVMUint32 ppcOpcode)
{
    const char *name = NULL;
    switch (ppcOpcode) {
    case PPC_ADD_OPCODE:     name = "add"; break;
    case PPC_ADDC_OPCODE:    name = "addc"; break;
    case PPC_ADDI_OPCODE:    name = "addi"; break;
    case PPC_ADDIS_OPCODE:   name = "addis"; break;
    case PPC_ADDIC_OPCODE:   name = "addic"; break;
    case PPC_ADDE_OPCODE:    name = "adde"; break;
    case PPC_SUBF_OPCODE:    name = "subf"; break;
    case PPC_SUBFC_OPCODE:   name = "subfc"; break;
    case PPC_SUBFE_OPCODE:   name = "subfe"; break;
    case PPC_SUBFIC_OPCODE:  name = "subfic"; break;
    case PPC_AND_OPCODE:     name = "and"; break;
    case PPC_ANDI_OPCODE:    name = "andi"; break;
    case PPC_ANDIS_OPCODE:   name = "andis"; break;
    case PPC_ANDC_OPCODE:    name = "andc"; break;
    case PPC_OR_OPCODE:      name = "or"; break;
    case PPC_ORI_OPCODE:     name = "ori"; break;
    case PPC_ORIS_OPCODE:    name = "oris"; break;
    case PPC_XOR_OPCODE:     name = "xor"; break;
    case PPC_XORI_OPCODE:    name = "xori"; break;
    case PPC_XORIS_OPCODE:   name = "xoris"; break;
#ifdef CVM_PPC_E500V1
    case PPC_E500_EFSADD_OPCODE: name = "efsadd"; break;
    case PPC_E500_EFSSUB_OPCODE: name = "efssub"; break;
    case PPC_E500_EFSMUL_OPCODE: name = "efsmul"; break;
    case PPC_E500_EFSDIV_OPCODE: name = "efsdiv"; break;
#endif

    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", ppcOpcode);
        CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;
}

#ifdef CVM_JIT_USE_FP_HARDWARE

static const char *getFPOpcodeName(CVMUint32 ppcOpcode)
{
    const char *name = NULL;
    switch (ppcOpcode) {

    case PPC_CROR_OPCODE:    name = "cror"; break;
	
    case PPC_FADD_OPCODE:    name = "fadd"; break;
    case PPC_FADDS_OPCODE:   name = "fadds"; break;
    case PPC_FCMPU_OPCODE:   name = "fcmpu"; break;
    case PPC_FDIV_OPCODE:    name = "fdiv"; break;
    case PPC_FDIVS_OPCODE:   name = "fdivs"; break;
    case PPC_FMR_OPCODE:     name = "fmr"; break;
    case PPC_FMUL_OPCODE:    name = "fmul"; break;
    case PPC_FMULS_OPCODE:   name = "fmuls"; break;
    case PPC_FNEG_OPCODE:    name = "fneg"; break;
    case PPC_FRSP_OPCODE:    name = "frsp"; break;
    case PPC_FSUB_OPCODE:    name = "fsub"; break;
    case PPC_FSUBS_OPCODE:   name = "fsubs"; break;
    /* WARNING: using fmadd results in a non-compliant vm. Some floating
     * point tck tests will fail. Therefore, this support if off by defualt.
     */
#ifdef CVM_JIT_USE_FMADD
    case PPC_FMADD_OPCODE:   name = "fmadds"; break;
    case PPC_DMADD_OPCODE:   name = "fmadd"; break;
    case PPC_FMSUB_OPCODE:   name = "fmsubs"; break;
    case PPC_DMSUB_OPCODE:   name = "fmsub"; break;
    case PPC_FNMADD_OPCODE:  name = "fnmadds"; break;
    case PPC_DNMADD_OPCODE:  name = "fnmadd"; break;
    case PPC_FNMSUB_OPCODE:  name = "fnmsubs"; break;
    case PPC_DNMSUB_OPCODE:  name = "fnmsub"; break;
#endif
    default:
        CVMconsolePrintf("Unknown opcode 0x%08x", ppcOpcode);
        CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;
}
#endif

static const char* regName(int regID) {
    const char *name = NULL;
    switch (regID) {
    case CVMPPC_r0:             name = "0"; break;
    case CVMCPU_JFP_REG:        name = "JFP"; break;
    case CVMPPC_r2:             name = "2"; break;
    case CVMPPC_r3:             name = "3"; break;
    case CVMPPC_r4:             name = "4"; break;
    case CVMPPC_r5:             name = "5"; break;
    case CVMPPC_r6:             name = "6"; break;
    case CVMPPC_r7:             name = "7"; break;
    case CVMPPC_r8:             name = "8"; break;
    case CVMPPC_r9:             name = "9"; break;
    case CVMPPC_r10:            name = "10"; break;
    case CVMPPC_r11:            name = "11"; break;
    case CVMPPC_r12:            name = "12"; break;
    case CVMPPC_r13:            name = "13"; break;
    case CVMPPC_r14:            name = "14"; break;
    case CVMPPC_r15:            name = "15"; break;
    case CVMPPC_r16:            name = "16"; break;
    case CVMPPC_r17:            name = "17"; break;
    case CVMPPC_r18:            name = "18"; break;
    case CVMPPC_r19:            name = "19"; break;
    case CVMPPC_r20:            name = "20"; break;
    case CVMPPC_r21:            name = "21"; break;
    case CVMPPC_r22:            name = "22"; break;
    case CVMPPC_r23:            name = "23"; break;
    case CVMPPC_r24:            name = "24"; break;
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    case CVMPPC_r25:            name = "GC"; break;
#else
    case CVMPPC_r25:            name = "25"; break;
#endif
#ifdef CVMCPU_HAS_CP_REG
    case CVMCPU_CP_REG:         name = "CP"; break;
#else
    case CVMPPC_r26:            name = "26"; break;
#endif
    case CVMCPU_EE_REG:         name = "EE"; break;
    case CVMCPU_CVMGLOBALS_REG: name = "CVMGLOBALS"; break;
    case CVMCPU_CHUNKEND_REG:   name = "CHUNKEND"; break;
    case CVMCPU_JSP_REG:        name = "JSP"; break;
    case CVMCPU_SP_REG:         name = "SP"; break;
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

/*
 * Figure out if constValue is encodable by rlwinm. If so, return
 * the MB and ME values we will need for the rlwinm instruction.
 */
CVMBool
CVMPPCgetEncodableRLWINM(CVMUint8* maskBeginPtr, CVMUint8* maskEndPtr,
			  CVMUint32 constValue)
{
    /* See if there is a run of bits */
    CVMUint32 bits = constValue;
    int maskBegin = 32;  /* the MB field of the rlwinm instruction */
    int maskEnd = 31;   /* the ME field of the rlwinm instruction */
    int shiftAmount =0;

    /* make sure all zeros are leading */
    if (bits != 0xffffffff) {
	/* if there are leading 1's rotate left so they are all trailing */
	while ((bits & 0x80000000) != 0) {
	    bits = bits << 1;
	    bits = bits | 1;
	    maskBegin++;
	    maskEnd++;
	    shiftAmount--;
	}
	/* shift right so all zeros are leading */
	while ((bits & 1) == 0) {
	    bits = bits >> 1;
	    maskBegin--;
	    maskEnd--;
	    shiftAmount++;
	}
    }

    /* count the run of ones */
    while ((bits & 1) != 0) {
	maskBegin--;
	bits = bits >> 1;
	shiftAmount++;
    }

    *maskBeginPtr = (maskBegin & 0x1f);
    *maskEndPtr = (maskEnd & 0x1f);

    if (shiftAmount == 32) {
	return CVM_TRUE;
    } else {
#if 0
	if (bits != 0) {
	    CVMconsolePrintf("0x%x not encodable\n", constValue);
	}
#endif
	return (bits == 0);
    }
}


/*
 * Makes a 32 bit constant without using the constant pool.
 */
static void
CVMPPCemitMakeConstant32(CVMJITCompilationContext* con,
			 int destRegID, CVMInt32 constant)
{
    /* Implemented as:

           lis    destRegID, r0, hi16(constant)
           ori    destRegID, destRegID, lo16(constant)
     */
    emitInstruction(con,
		    PPC_ADDIS_OPCODE | destRegID << PPC_RD_SHIFT |
		    CVMPPC_r0 << PPC_RA_SHIFT | ((constant >> 16) & 0xffff));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	lis	r%s, hi16(0x%x)\n",
			 regName(destRegID), constant);
    });

    emitInstruction(con,
		    PPC_ORI_OPCODE | destRegID << PPC_RD_SHIFT |
		    destRegID << PPC_RA_SHIFT | ((constant & 0xffff)));
    CVMtraceJITCodegenExec({
	CVMJITaddCodegenComment((con, CVMJITgetSymbolName(con)));
	printPC(con);
	CVMconsolePrintf("	ori	r%s, r%s, lo16(0x%x)",
			 regName(destRegID), regName(destRegID), constant);
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

    /* make the constant without using the constant pool */
    /* Construct the CP base register in a PC relative way:
     * 
     *     bl nextpc
     * nextpc:
     *     mflr CVMCPU_CP_REG  # move LR into CVMCPU_CP_REG
     *     add  CVMCPU_CP_REG, CVMCPU_CP_REG, <offset>
     *
     * or
     *
     *     bl nextpc
     * nextpc:
     *     mflr CVMCPU_CP_REG  # move LR into CVMCPU_CP_REG
     *     lis  r0, hi16(offset)
     *     ori  r0, r0, lo16(offset)
     *     add  CVMCPU_CP_REG, CVMCPU_CP_REG, r0
     */
    CVMPPCemitBranch(con, 
                     CVMJITcbufGetLogicalPC(con) + CVMCPU_INSTRUCTION_SIZE,
                     CVMCPU_COND_AL,
                     CVM_TRUE /*link */, CVM_FALSE,
                     NULL /* no fixupList */);
    targetOffset = con->target.cpLogicalPC - CVMJITcbufGetLogicalPC(con);
    CVMCPUemitLoadReturnAddress(con, CVMCPU_CP_REG);
    if (CVMCPUalurhsIsEncodableAsImmediate(CVMCPU_ADD_OPCODE, targetOffset)) {
        CVMCPUALURhsToken rhsToken = 
                     CVMPPCalurhsEncodeConstantToken(targetOffset);
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
                     CVMCPU_CP_REG, CVMCPU_CP_REG,
                     rhsToken, CVMJIT_NOSETCC);
    } else {
        CVMPPCemitMakeConstant32(con, CVMPPC_r0, targetOffset);
        CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
                     CVMCPU_CP_REG, CVMCPU_CP_REG, CVMPPC_r0,
		     CVMJIT_NOSETCC);
    }
#else
    CVMUint32 targetAddr;

    targetAddr = (CVMUint32)
	CVMJITcbufLogicalToPhysical(con, con->target.cpLogicalPC);

    CVMJITprintCodegenComment(("setup cp base register"));

    /* make the constant without using the constant pool */
    CVMPPCemitMakeConstant32(con, CVMCPU_CP_REG, targetAddr);
#endif

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
			    CVMPPCalurhsEncodeConstantToken(value),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 8k
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
    CVMJITprintCodegenComment(("Stack limit check"));

    CVMJITaddCodegenComment((con, "get return address"));
    CVMCPUemitLoadReturnAddress(con, CVMPPC_r0);

    CVMJITaddCodegenComment((con, "check for chunk overflow"));
    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_LS,
                              CVMCPU_CHUNKEND_REG, CVMCPU_ARG2_REG);

    CVMJITaddCodegenComment((con, "Store LR into frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
				       CVMPPC_r0, CVMCPU_JFP_REG,
				       CVMoffsetof(CVMCompiledFrame, pcX));

    CVMJITsetSymbolName((con, "letInterpreterDoInvoke"));
    CVMJITaddCodegenComment((con, "call letInterpreterDoInvoke"));
    CVMCPUemitAbsoluteCallConditional(con,
        CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr,
	CVMJIT_CPDUMPOK, CVMJIT_NOCPBRANCH, CVMCPU_COND_LS);
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
			    CVMPPCalurhsEncodeConstantToken(offset),
			    CVMJIT_NOSETCC);
    } else {
        /* The only time this would happen is if there are about 8k
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
     *   lwz     PREV, OFFSET_CVMFrame_prevX(JFP)
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
	CVMPPCemitBranch(con, helperLogicalPC, CVMCPU_COND_AL,
			 CVM_FALSE /* don't link */, CVM_FALSE,
                         NULL /* no fixupList */);
    }
}

void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall(CVMJITCompilationContext* con,
                                             const void* target,
                                             CVMBool okToDumpCp,
                                             CVMBool okToBranchAroundCpDump)
{
    CVMInt32 targetLogicalPC;
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    CVMCodegenComment *comment;

    CVMJITpopCodegenComment(con, comment);

    /*
     *     lis   r0, hi16(returnPC)
     *     ori   r0, r0, lo16(returnPC)
     *     stw   JSP, topOfStack(JFP)
     *     stw   r0, pc(JFP)
     *     bl    target
     * returnPC:
     *
     *         or
     *
     *     lis   r0, hi16(returnPC)
     *     ori   r0, r0, lo16(returnPC)
     *     stw   JSP, topOfStack(JFP)
     *     stw   r0, pc(JFP)
     *     lis   r0, hi16(target)
     *     ori   r0, r0, lo16(target)
     *     mtlr  r0
     *     blr
     * returnPC:
     */

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        targetLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else 
#endif
    {
        targetLogicalPC = (CVMUint8*)target - con->codeEntry;
    }

    /* load the return PC */
    CVMJITsetSymbolName((con, "return address"));
    CVMPPCemitMakeConstant32(con, CVMPPC_r0,
			     (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
			     5 * CVMCPU_INSTRUCTION_SIZE);

    /* Store the JSP into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush JFP to stack"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
        CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetof(CVMFrame, topOfStack));

    /* Store the return PC into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush return address to frame"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
				       CVMPPC_r0, CVMCPU_JFP_REG,
				       offsetof(CVMCompiledFrame, pcX));

    /* call the function */
    CVMJITpushCodegenComment(con, comment);
    CVMPPCemitBranch(con, targetLogicalPC, CVMCPU_COND_AL,
		     CVM_TRUE /* link */, CVM_FALSE,
                     NULL /* no fixupList */);

    {
	/* Make sure we really only emitted 5 instructions. The bl may
	 * have taken more than 1 instruction, in which case we need to
	 * emit the loading of the returnPC again.
	 */
	CVMInt32 returnPcOffset = CVMJITcbufGetLogicalPC(con) - logicalPC;
	if (returnPcOffset != 5 * CVMCPU_INSTRUCTION_SIZE) {
	    CVMJITcbufPushFixup(con, logicalPC);
	    CVMJITsetSymbolName((con, "return address"));
	    CVMPPCemitMakeConstant32(con, CVMPPC_r0,
				     (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
				     returnPcOffset);
	    CVMJITcbufPop(con);
	}
    }
}

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. If okToDumpCp is TRUE, the constant pool will
 * be dumped if possible. Set okToBranchAroundCpDump to FALSE if you plan
 * on generating a stackmap after calling CVMCPUemitAbsoluteCall.
 * 
 * PowerPC ignores okToDumpCp and okToBranchAroundCpDump because either
 * there is no constant pool, or it is dumped once when compilation
 * is done.
 */
void
CVMCPUemitAbsoluteCallConditional(CVMJITCompilationContext* con,
				  const void* target,
				  CVMBool okToDumpCp,
				  CVMBool okToBranchAroundCpDump,
				  CVMCPUCondCode condCode)
{
    CVMInt32 helperLogicalPC;
    CVMBool useBLR = !okToDumpCp;

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    useBLR = CVM_FALSE;
#else
    if (!okToDumpCp) {
	/*
	 * There are a few uses of !okToDumpCp that do not require the
	 * use of blr, so we can reset useBLR to false for them.
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
	    useBLR = CVM_FALSE;
	} else {
	    useBLR = CVM_TRUE;
	}
    }
#endif

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperLogicalPC = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else 
#endif
    {
        helperLogicalPC = (CVMUint8*)target - con->codeEntry;
    }
    CVMPPCemitBranch(con, helperLogicalPC, condCode,
		     CVM_TRUE /* link */, useBLR,
                     NULL /* no fixupList */);

    /* SymbolName may have been set to the name of the target */
    CVMJITresetSymbolName(con);
}

/*
 * branch to the address in "srcReg".
 */
static void
CVMPPCemitBranchRegister(CVMJITCompilationContext* con,
			 int srcReg, CVMBool link)
{
    emitInstruction(con, PPC_MTLR_OPCODE | srcReg << PPC_RS_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("	mtlr	r%s\n", regName(srcReg));
    });
    emitInstruction(con, PPC_BLR_OPCODE |
		    PPC_BO_BRANCH_ALWAYS_BITS << PPC_BO_SHIFT |
		    (link ? PPC_BRANCH_LINK_BIT : 0));
    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("	%s", (link ? "blrl" : "blr"));
    });
    CVMJITdumpCodegenComments(con);
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);
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

    /* get the address of the method. */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
				       CVMPPC_r0, CVMCPU_ARG1_REG, off);
    /* call it */
    CVMPPCemitBranchRegister(con, CVMPPC_r0, CVM_TRUE /* link */);
}

/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the LR is a GPR.
 */
void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno)
{
    emitInstruction(con, PPC_MFLR_OPCODE | regno << PPC_RS_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("	mflr	r%s", regName(regno));
    });
    CVMJITdumpCodegenComments(con);
}

/*
 * Branch to the address in the specified register.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno)
{
    CVMPPCemitBranchRegister(con, regno, CVM_FALSE /* don't link */);
}

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The table entry for index 0 will be generated right after
 * any instructions that are generated here.
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int keyReg)
{
    CVMRMResource *tmpResource1 = CVMRMgetResource(CVMRM_INT_REGS(con),
				      CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    CVMUint32 tmpReg1 = CVMRMgetRegisterNumber(tmpResource1);

    /* This is implemented as (not PC relative):
     *
     *     slwi  reg1, key, 2
     *     addis reg1, reg1, ha16(start+20)
     *     addi  reg1, reg1, lo16(start+20)
     *     mtlr  reg1
     *     blr
     *   tablePC:
     *
     * or when CVM_AOT is enabled (PC relative):
     *
     *     blr   nextLabel
     *   nextLabel:
     *     mflr  r0
     *     slwi  reg1, key, 2
     *     add   reg1, reg1, r0
     *     addi  reg1, reg1, 24 ! distance from 'nextLabel' to table start
     *     mtlr  ret1
     *     blr
     *   tablePC:
     */

#ifndef CVM_AOT
    CVMUint32 tablePC = (CVMUint32)CVMJITcbufGetPhysicalPC(con) + 20;
    CVMInt16  lo16 = tablePC & 0xffff;
    CVMUint16 ha16 = (tablePC >> 16) & 0xffff;

    /* Add one to ha16 if lo16 is negative to account for borrow that
     * will be done during ori. */
    if (lo16 < 0) {
	ha16 += 1;
    }
#else
    /* emit branch so we can get the current address from lr */
    CVMPPCemitBranch(con,
                     CVMJITcbufGetLogicalPC(con) + CVMCPU_INSTRUCTION_SIZE,
                     CVMCPU_COND_AL,
                     CVM_TRUE /*link */, CVM_FALSE,
                     NULL /* no fixupList */);
    CVMCPUemitLoadReturnAddress(con, CVMPPC_r0);
#endif

    /* shift the key by 2: */
    CVMCPUemitShiftByConstant(con, CVMCPU_SLL_OPCODE, 
                              tmpReg1, keyReg, 2);

#ifndef CVM_AOT
    /*
     * load the address to branch to:
     * NOTE: We can't use CVMCPUemitBinaryALUConstant() because it
     * may not encode negative values as immediates.
     */
    CVMCPUemitBinaryALU(con, CVMPPC_ADDIS_OPCODE,
			tmpReg1, tmpReg1,
			CVMPPCalurhsEncodeConstantToken(ha16), CVM_FALSE);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
			tmpReg1, tmpReg1,
			CVMPPCalurhsEncodeConstantToken(lo16), CVM_FALSE);
#else
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
                        tmpReg1, tmpReg1, CVMPPC_r0,
                        CVMJIT_NOSETCC);
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
                                tmpReg1, tmpReg1,
                                6 * CVMCPU_INSTRUCTION_SIZE, CVMJIT_NOSETCC);
#endif

    /* emit the blr instruction: */
    CVMPPCemitBranchRegister(con, tmpReg1, CVM_FALSE /* don't link */);

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource1);
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
    CVMassert((*(CVMUint32*)instrAddr & ~0x03fffffc) ==
	      (PPC_B_OPCODE | PPC_BRANCH_LINK_BIT));
    branch = CVMPPCgetBranchInstruction(CVMCPU_COND_AL, offset) |
	PPC_BRANCH_LINK_BIT;
    *((CVMCPUInstruction*)instrAddr) = branch;
}

#endif

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
CVMCPUInstruction
CVMPPCgetBranchInstruction(CVMCPUCondCode condCode, int offset)
{
    if (condCode == CVMCPU_COND_AL) {
	CVMassert(offset <= CVMPPC_MAX_BRANCH_OFFSET &&
		  offset >= CVMPPC_MIN_BRANCH_OFFSET);
	return (PPC_B_OPCODE | ((CVMUint32)offset & 0x03fffffc));
    } else {
	CVMassert(offset <= CVMPPC_MAX_COND_BRANCH_OFFSET &&
		  offset >= CVMPPC_MIN_COND_BRANCH_OFFSET);
	return (PPC_BC_OPCODE | ((CVMUint32)offset & 0x0000fffc) |
		(ppcConditions[condCode].branchBOBits << PPC_BO_SHIFT) |
		(ppcConditions[condCode].crFieldBit << PPC_BI_SHIFT));
    }
}

/*
 * Helper for CVMPPCemitBranch() that assumes the branch target
 * is withing range.
 *
 * if defaultBranchPrediction, then the processor's default branch
 * prediction is used. On the PowerPC, the default is to take
 * backwards branches and not take conditional register branches.
 * However, we override this to also default all conditional
 * branch-and-link instructions to not take the branch.
 *
 * prediction is controlled by the "y" bit, which is the first bit
 * of the BO field. It is cleared for default prediction.
 */
static void
CVMPPCemitBranch0(CVMJITCompilationContext* con, int logicalPC,
		  CVMCPUCondCode condCode, CVMBool link,
		  CVMBool defaultBranchPrediction)
{
    int realoffset;
    CVMUint32 branchInstruction;

    /* default to not take conditional branch-and-link */
    if (condCode != CVMCPU_COND_AL && link) {
	defaultBranchPrediction = CVM_FALSE;
    }

    CVMassert(defaultBranchPrediction || condCode != CVMCPU_COND_AL);

    realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);
    branchInstruction = CVMPPCgetBranchInstruction(condCode, realoffset); 
    emitInstruction(con, branchInstruction | 
		    (!defaultBranchPrediction ? (1 << PPC_BO_SHIFT) : 0) |
		    (link ? PPC_BRANCH_LINK_BIT : 0));
    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	b%s%s%s	PC=(%d)",
			 conditions[condCode], (link ? "l" : 0),
			 condCode == CVMCPU_COND_AL
			 ? "" : ((defaultBranchPrediction == 
				  (realoffset < 0 && logicalPC != 0))
				 ? "+" : "-"),
			 logicalPC == CVMJITcbufGetLogicalPC(con) ? 0 : logicalPC);
    });

    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);
    CVMJITdumpCodegenComments(con);
}

/*
 * Emit a conditional pc-relative branch or branch-and-link
 *
 * link:    if true, then a branch-and-link is done.
 * useBLR:  if true, the branch is forced to go through LR, even
 *          if the target is within range of a pc-relative branch.
 *          This is done to ensure a known number of instructions
 *          are emitted, which is needed to make patching done
 *          by CVMCCMruntimeResolveGlue to work.
 */
void
CVMPPCemitBranch(CVMJITCompilationContext* con, int logicalPC,
		 CVMCPUCondCode condCode, CVMBool link, 
                 CVMBool useBLR, CVMJITFixupElement** fixupList)
{
    CVMInt32 realoffset;
    CVMBool reachable;
    CVMCodegenComment *comment;

    CVMJITpopCodegenComment(con, comment);

    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }
    
#ifdef CVM_JIT_USE_FP_HARDWARE
    /*
     * If this is floating math, ignore UNORDERED handling -- its been done
     */
    condCode &= ~CVMCPU_COND_UNORDERED_LT;
#endif
    /*
     * A logicalPC of 0 means a forward branch we will resolve later. Make
     * the realoffset 0 in this case so we know it is reachable. If it is
     * not reachable at fixup time, we will simply fail to compiled the method,
     * which is reasonable given the +/- 32k branch range.
     */
    if (logicalPC == 0) {
	logicalPC = CVMJITcbufGetLogicalPC(con);
    }
    realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);

    /*
     * It's reachable with a bl instruction. If it's conditional,
     * then see if it is reachable with a "bc". If not, then emit
     * a conditional branch around the unconditional "b".
     */
    if (condCode != CVMCPU_COND_AL) {
	reachable = (realoffset <= CVMPPC_MAX_COND_BRANCH_OFFSET &&
		     realoffset >= CVMPPC_MIN_COND_BRANCH_OFFSET);
	/*
	 * If the target is not reachable with a conditional branch, then
	 * do a conditional branch around an unconditional branch.
	 */
	if (!reachable) {
	    int skip;
	    CVMInt32 logicalPC;
	    reachable = (realoffset <= CVMPPC_MAX_BRANCH_OFFSET &&
			 realoffset >= CVMPPC_MIN_BRANCH_OFFSET);
	    skip = (reachable ? 2 : 5); /* instructions to skip */
	    logicalPC =
		CVMJITcbufGetLogicalPC(con) + skip * CVMCPU_INSTRUCTION_SIZE;
	    CVMJITaddCodegenComment((con,
				     "branch around long conditional branch"));
	    CVMPPCemitBranch0(con, logicalPC,
			      CVMCPUoppositeCondCode[condCode], CVM_FALSE,
			      CVM_FALSE /* assume branch is taken */);
	    /* make the conditional branch unconditional */
	    condCode = CVMCPU_COND_AL;
	    /* acount for branch instruction we just emitted */
	    realoffset -= CVMCPU_INSTRUCTION_SIZE;
	}
    }

    /*
     * At this point, if condCode is still conditional, then at least we know
     * the target address is in range with a conditional branch. If it is
     * unconditional, then it may be out of range of an unconditional
     * branch so check this first.
     */
    reachable = !useBLR && (realoffset <= CVMPPC_MAX_BRANCH_OFFSET &&
			    realoffset >= CVMPPC_MIN_BRANCH_OFFSET);
    if (reachable) {
	CVMJITpushCodegenComment(con, comment);
	CVMPPCemitBranch0(con, logicalPC, condCode, link, CVM_TRUE);
    } else {
	/* need to load address into a register and do a blrl */
	CVMassert(condCode == CVMCPU_COND_AL);
	CVMPPCemitMakeConstant32(con, CVMPPC_r0,
				 (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
				 realoffset);
	CVMJITpushCodegenComment(con, comment);
	CVMPPCemitBranchRegister(con, CVMPPC_r0, CVM_TRUE /* link */);
    }
}

/* Purpose: Emits instructions to do the specified 32 bit unary ALU
            operation. */
void
CVMCPUemitUnaryALU(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int srcRegID, CVMBool setcc)
{
    switch (opcode) {
    case CVMCPU_NEG_OPCODE: {
	emitInstruction(con, PPC_NEG_OPCODE | (setcc ? PPC_RECORD_BIT : 0) |
			destRegID << PPC_RD_SHIFT |
			srcRegID << PPC_RA_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	neg%s	r%s, r%s",
			     setcc ? "." : "",
			     regName(destRegID), regName(srcRegID));
	});
        break;
    }
    case CVMCPU_NOT_OPCODE: {
        /* reg32 = (reg32 == 0)?1:0. */
	emitInstruction(con, PPC_SUBFIC_OPCODE |
			CVMPPC_r0 << PPC_RD_SHIFT |
			srcRegID << PPC_RA_SHIFT | 0);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	subfic	r%s, r%s, 0\n",
			     regName(CVMPPC_r0), regName(srcRegID));
	});
	emitInstruction(con, PPC_ADDE_OPCODE | (setcc ? PPC_RECORD_BIT : 0) |
			destRegID << PPC_RD_SHIFT |
			CVMPPC_r0 << PPC_RA_SHIFT |
			srcRegID << PPC_RB_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	adde%s	r%s, r%s, r%s",
			     setcc ? "." : "",
			     regName(destRegID), regName(CVMPPC_r0), regName(srcRegID));
	});
        break;
    }
    case CVMCPU_INT2BIT_OPCODE: {
        /* reg32 = (reg32 != 0)?1:0. */
	emitInstruction(con, PPC_ADDIC_OPCODE |
			CVMPPC_r0 << PPC_RD_SHIFT |
			srcRegID << PPC_RA_SHIFT | 0xffff /* 16-bit -1 */);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	addic	r%s, r%s, -1\n",
			     regName(CVMPPC_r0), regName(srcRegID));
	});
	emitInstruction(con, PPC_SUBFE_OPCODE | (setcc ? PPC_RECORD_BIT : 0) |
			destRegID << PPC_RD_SHIFT |
			CVMPPC_r0 << PPC_RA_SHIFT |
			srcRegID << PPC_RB_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	subfe%s	r%s, r%s, r%s",
			     setcc ? "." : "",
			     regName(destRegID), regName(CVMPPC_r0), regName(srcRegID));
	});
        break;
    }
#ifdef CVM_PPC_E500V1
    case CVME500_FNEG_OPCODE: {
	CVMassert(!setcc);
	emitInstruction(con, PPC_E500_EFSNEG_OPCODE |
			destRegID << PPC_RD_SHIFT |
			srcRegID << PPC_RA_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	efsneg	r%s, r%s",
			     regName(destRegID), regName(srcRegID));
	});
        break;
    }
    case CVME500_I2F_OPCODE: {
	CVMassert(!setcc);
	emitInstruction(con, PPC_E500_EFSCFSI_OPCODE |
			destRegID << PPC_RD_SHIFT |
			srcRegID << PPC_RB_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	efscfsi	r%s, r%s",
			     regName(destRegID), regName(srcRegID));
	});
        break;
    }
    case CVME500_F2I_OPCODE: {
	CVMassert(!setcc);
	emitInstruction(con, PPC_E500_EFSCTSI_OPCODE |
			destRegID << PPC_RD_SHIFT |
			srcRegID << PPC_RB_SHIFT);
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	efsctsi	r%s, r%s",
			     regName(destRegID), regName(srcRegID));
	});
        break;
    }
#endif
    default:
        CVMassert(CVM_FALSE);
    }
    CVMJITdumpCodegenComments(con);
}

/* Purpose: Emits a div or rem. */
static void
CVMPPCemitDivOrRem(CVMJITCompilationContext* con,
		   int opcode, int destRegID, int lhsRegID,
		   CVMCPUALURhsToken rhsToken)
{
    int rhsRegID = CVMPPCalurhsDecodeGetTokenRegister(rhsToken);
    int doDivLogicalPC;

    CVMassert(CVMPPCalurhsDecodeGetTokenType(rhsToken) ==
	      CVMPPC_ALURHS_REGISTER);

    /* 
     * We could support the following by allocating a temp register, but
     * it doesn't appear to be necessary.
     */
    CVMassert(destRegID != rhsRegID);

    /*
     *      @ integer divide
     *      cmpi   rhsRegID, 0    @ check for / by 0
     *      beq-   CVMCCMruntimeThrowDivideByZeroGlue
     *      bgt+   .doDiv         @ shortcut around 0x80000000 / -1 check
     *      cmpi   rhsRegID, -1   @ 0x80000000 / -1 check
     *      bne+   .doDiv
     *      addis  destRegID, r0, 0x8000
     *      cmp    destRegID, lhsRegID
     *      beq-   .divDone
     * .doDiv:
     *      div    destRegID, lhsRegID, rhsRegID
     * .divDone:
     *
     *      @ integer remainder after doing divide
     *      mullw  destRegID, destRegID, rhsRegID
     *      subf   destRegID, destRegID, lhsRegID
     */

    CVMJITdumpCodegenComments(con);
    CVMJITprintCodegenComment(("Do integer %s:",
			       opcode == CVMPPC_DIV_OPCODE
			       ? "divide" : "remainder"));


    /* move lhsRegID into CVMPPC_r0 if lhsRegID == destRegID. Otherwise
     * it will be clobbered before used. */
    if (lhsRegID == destRegID) {
	CVMJITaddCodegenComment((con, "avoid clobber when lshReg == destReg"));
	CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			       CVMPPC_r0, lhsRegID, CVMJIT_NOSETCC);
	lhsRegID = CVMPPC_r0;
    }

    /* compare for rhsRegID % 0 */
    CVMJITaddCodegenComment((con, "check for / by 0"));
    CVMCPUemitCompareConstant(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_EQ,
			      rhsRegID, 0);
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMJITsetSymbolName((con, "CVMCCMruntimeThrowDivideByZeroGlue"));
    CVMCPUemitAbsoluteCallConditional(con, CVMCCMruntimeThrowDivideByZeroGlue,
				      CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK,
				      CVMCPU_COND_EQ);

    doDivLogicalPC = CVMJITcbufGetLogicalPC(con) + 
	CVMCPU_INSTRUCTION_SIZE * 6;

    /* shortcut around compare for 0x80000000 % -1 */
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMPPCemitBranch0(con, doDivLogicalPC, CVMCPU_COND_GT, CVM_FALSE,
		      CVM_FALSE /* assume branch is taken */);

    /* compare for 0x80000000 / -1 */
    CVMJITaddCodegenComment((con, "0x80000000 / -1 check:"));
    CVMCPUemitCompare(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
		      rhsRegID, CVMPPCalurhsEncodeConstantToken(-1));
    CVMJITaddCodegenComment((con, "goto doDiv"));
    CVMPPCemitBranch0(con, doDivLogicalPC, CVMCPU_COND_NE, CVM_FALSE,
		      CVM_FALSE /* assume branch is taken */);
    CVMCPUemitBinaryALU(con, CVMPPC_ADDIS_OPCODE,
			destRegID, CVMPPC_r0,
			CVMPPCalurhsEncodeConstantToken(0x8000),
			CVMJIT_NOSETCC);
    CVMJITaddCodegenComment((con, "goto divDone"));
    CVMCPUemitCompare(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_NE,
		      destRegID, CVMPPCalurhsEncodeRegisterToken(lhsRegID));
    CVMPPCemitBranch0(con, CVMJITcbufGetLogicalPC(con)+8,
		      CVMCPU_COND_EQ, CVM_FALSE,
		      CVM_FALSE /* assume branch is taken */);

    /* emit the divw instruction */
    CVMtraceJITCodegen(("\t\t.doDiv\n"));
    emitInstruction(con, PPC_DIVW_OPCODE | destRegID << PPC_RD_SHIFT |
		    lhsRegID << PPC_RA_SHIFT | rhsRegID << PPC_RB_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	divw	r%s, r%s, r%s\n",
			 regName(destRegID), regName(lhsRegID),
			 regName(rhsRegID));
    });

    CVMtraceJITCodegen(("\t\t.divDone\n"));
 
    /* do rem computation if requested */
    if (opcode == CVMPPC_REM_OPCODE) {
	CVMJITprintCodegenComment(("do integer remainder after doing divide"));
	CVMCPUemitMul(con, CVMCPU_MULL_OPCODE,
		      destRegID, destRegID, rhsRegID, CVMCPU_INVALID_REG);
	CVMCPUemitBinaryALURegister(con, CVMCPU_SUB_OPCODE,
				    destRegID, lhsRegID, destRegID,
				    CVMJIT_NOSETCC);
    }
}

static void
emitRLWINM(CVMJITCompilationContext* con, int maskBegin, int maskEnd,
	   int destRegID, int lhsRegID, CVMBool setcc)
{
    emitInstruction(con, PPC_RLWINM_OPCODE |
		    destRegID << PPC_RA_SHIFT | lhsRegID << PPC_RS_SHIFT |
		    maskBegin << PPC_MB_SHIFT | maskEnd << PPC_ME_SHIFT |
		    (setcc ? PPC_RECORD_BIT : 0));
    CVMtraceJITCodegenExec({
        printPC(con);
	CVMconsolePrintf("	rlwimi%s	r%s, r%s, %d, %d, %d",
			 setcc ? "." : "",
			 regName(destRegID), regName(lhsRegID),
			 0, maskBegin, maskEnd);
    });
    CVMJITdumpCodegenComments(con);
}

/*
 * Table that provides encoding information for register based binary
 * ALU instructions. ARM has very consistent encodings across ALU opcodes,
 * so the encoding can easily be done in the CVMPU_XXX_OPCODE value,
 * plus some uniform masking of things like register encodings and
 * "setcc" bits. PowerPC has very inconsistent encodings, so making
 * it table based is necessary.
 */
typedef struct CVMPPCBinaryALUOpcode CVMPPCBinaryALUOpcode;
struct CVMPPCBinaryALUOpcode {
    CVMUint32 opcodeReg;      /* encoding of opcode with reg rhs */
    CVMUint32 opcodeRegSetcc; /* encoding of opcode with reg rhs and setcc */
    CVMUint32 opcodeImm;      /* encoding of opcode with imm rhs */
    CVMUint32 opcodeImmSetcc; /* encoding of opcode with imm rhs and setcc */
    CVMUint32 destShift;      /* # of bits to shift dest register */
    CVMUint32 lhsShift;       /* # of bits to shift lhs register */
    CVMUint32 rhsShift;       /* # of bits to shift rhs register */
};

static const CVMPPCBinaryALUOpcode ppcBinaryALUOpcodes[] = {
    /* CVMCPU_ADD_OPCODE */  
    {PPC_ADD_OPCODE, PPC_ADDC_OPCODE, PPC_ADDI_OPCODE, PPC_ADDIC_OPCODE,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_ADDIS_OPCODE */
    {0 /*invalid*/, 0 /*invalid*/,  PPC_ADDIS_OPCODE, 0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_ADDE_OPCODE */
    {PPC_ADDE_OPCODE, 0 /*invalid*/, 0 /*invalid*/, 0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMCPU_SUB_OPCODE */
    {PPC_SUBF_OPCODE, PPC_SUBFC_OPCODE, PPC_ADDI_OPCODE, PPC_ADDIC_OPCODE,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_SUBE_OPCODE */
    {PPC_SUBFE_OPCODE, 0 /*invalid*/, 0 /*invalid*/, 0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_SUBFIC_OPCODE */
    {0 /*invalid*/, 0 /*invalid*/, PPC_SUBFIC_OPCODE, 0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVMCPU_AND_OPCODE */
    {PPC_AND_OPCODE, PPC_AND_OPCODE, PPC_ANDI_OPCODE, PPC_ANDI_OPCODE,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_ANDIS_OPCODE */
    { 0 /*invalid*/,  0 /*invalid*/, PPC_ANDIS_OPCODE, PPC_ANDIS_OPCODE,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
    /* CVMCPU_OR_OPCODE */
    {PPC_OR_OPCODE, PPC_OR_OPCODE, PPC_ORI_OPCODE, PPC_ORI_OPCODE,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_ORIS_OPCODE */
    { 0 /*invalid*/,  0 /*invalid*/, PPC_ORIS_OPCODE,  0 /*invalid*/,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
    /* CVMCPU_XOR_OPCODE */
    {PPC_XOR_OPCODE, PPC_XOR_OPCODE, PPC_XORI_OPCODE, PPC_XORI_OPCODE,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
    /* CVMPPC_XORIS_OPCODE */
    { 0 /*invalid*/,  0 /*invalid*/, PPC_XORIS_OPCODE,  0 /*invalid*/,
     PPC_RA_SHIFT, PPC_RS_SHIFT, PPC_RB_SHIFT},
#ifdef CVM_PPC_E500V1
    /* CVME500_FADD_OPCODE */  
    {PPC_E500_EFSADD_OPCODE, 0 /*invalid*/,  0 /*invalid*/,  0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVME500_FSUB_OPCODE */  
    {PPC_E500_EFSSUB_OPCODE, 0 /*invalid*/,  0 /*invalid*/,  0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVME500_FMUL_OPCODE */  
    {PPC_E500_EFSMUL_OPCODE, 0 /*invalid*/,  0 /*invalid*/,  0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
    /* CVME500_FDIV_OPCODE */  
    {PPC_E500_EFSDIV_OPCODE, 0 /*invalid*/,  0 /*invalid*/,  0 /*invalid*/,
     PPC_RD_SHIFT, PPC_RA_SHIFT, PPC_RB_SHIFT},
#endif
};

#ifdef CVM_JIT_USE_FP_HARDWARE
static const CVMUint32 ppcFPOpcodes[] = {
    /* CVMPPC_FMOV_OPCODE */ PPC_FMR_OPCODE,
    /* CVMCPU_FNEG_OPCODE */ PPC_FNEG_OPCODE,
    /* CVMCPU_FADD_OPCODE */ PPC_FADDS_OPCODE,
    /* CVMCPU_FSUB_OPCODE */ PPC_FSUBS_OPCODE,
    /* CVMCPU_FMUL_OPCODE */ PPC_FMULS_OPCODE,
    /* CVMCPU_FDIV_OPCODE */ PPC_FDIVS_OPCODE,
    /* CVMCPU_FCMP_OPCODE */ PPC_FCMPU_OPCODE,

    /* CVMCPU_DNEG_OPCODE */ PPC_FNEG_OPCODE,
    /* CVMCPU_DADD_OPCODE */ PPC_FADD_OPCODE,
    /* CVMCPU_DSUB_OPCODE */ PPC_FSUB_OPCODE,
    /* CVMCPU_DMUL_OPCODE */ PPC_FMUL_OPCODE,
    /* CVMCPU_DDIV_OPCODE */ PPC_FDIV_OPCODE,
    /* CVMCPU_DCMP_OPCODE */ PPC_FCMPU_OPCODE

    /* WARNING: using fmadd results in a non-compliant vm. Some floating
     * point tck tests will fail. Therefore, this support if off by defualt.
     */
#ifdef CVM_JIT_USE_FMADD
    ,
    /* CVMPPC_FMADD_OPCODE */  PPC_FMADD_OPCODE,
    /* CVMPPC_DMADD_OPCODE */  PPC_DMADD_OPCODE,
    /* CVMPPC_FMSUB_OPCODE */  PPC_FMSUB_OPCODE,
    /* CVMPPC_DMSUB_OPCODE */  PPC_DMSUB_OPCODE,
    /* CVMPPC_FNMADD_OPCODE */ PPC_FNMADD_OPCODE,
    /* CVMPPC_DNMADD_OPCODE */ PPC_DNMADD_OPCODE,
    /* CVMPPC_FNMSUB_OPCODE */ PPC_FNMSUB_OPCODE,
    /* CVMPPC_DNMSUB_OPCODE */ PPC_DNMSUB_OPCODE
#endif

};
#endif

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
void
CVMCPUemitBinaryALU(CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID,
		    CVMCPUALURhsToken rhsToken,
		    CVMBool setcc)
{
    int tokenType = CVMPPCalurhsDecodeGetTokenType(rhsToken);
    CVMUint32 destShift = ppcBinaryALUOpcodes[opcode].destShift;
    CVMUint32 lhsShift  = ppcBinaryALUOpcodes[opcode].lhsShift;
    CVMUint32 ppcOpcode;

    if (opcode == CVMPPC_DIV_OPCODE || opcode == CVMPPC_REM_OPCODE) {
	CVMassert(!setcc);
	CVMPPCemitDivOrRem(con, opcode, destRegID, lhsRegID, rhsToken);
	return;
    }

    if (opcode == CVMPPC_RLWINM_OPCODE) {
	/* the MB and ME values were encoded into the constant */
	CVMUint16 rhsConst = CVMPPCalurhsDecodeGetTokenConstant(rhsToken);
	CVMUint8 maskBegin = (rhsConst >> 8) & 0xff;
	CVMUint8 maskEnd = rhsConst & 0xff;
	CVMassert(tokenType == CVMPPC_ALURHS_CONSTANT);
	emitRLWINM(con, maskBegin, maskEnd, destRegID, lhsRegID, setcc);
	return;
    }

    CVMassert(opcode == CVMCPU_ADD_OPCODE  || opcode == CVMPPC_ADDIS_OPCODE ||
	      opcode == CVMCPU_SUB_OPCODE  || opcode == CVMPPC_SUBFIC_OPCODE ||
              opcode == CVMCPU_AND_OPCODE  || opcode == CVMPPC_ANDIS_OPCODE ||
	      opcode == CVMCPU_OR_OPCODE   || opcode == CVMPPC_ORIS_OPCODE ||
              opcode == CVMCPU_XOR_OPCODE  || opcode == CVMPPC_XORIS_OPCODE || 
              opcode == CVMPPC_ADDE_OPCODE || opcode == CVMPPC_SUBE_OPCODE ||
#ifdef CVM_PPC_E500V1
	      opcode == CVME500_FADD_OPCODE || opcode == CVME500_FSUB_OPCODE ||
	      opcode == CVME500_FMUL_OPCODE || opcode == CVME500_FDIV_OPCODE ||
#endif
	      0);

    if (setcc || tokenType == CVMPPC_ALURHS_CONSTANT) {
	/* ADD/SUB with carry can't also setcc or be imm alurhs */
	CVMassert(opcode != CVMPPC_ADDE_OPCODE &&
		  opcode != CVMPPC_SUBE_OPCODE);
	/* ADDIS can't also setcc */
	CVMassert(!(setcc && opcode == CVMPPC_ADDIS_OPCODE));
#ifdef CVM_PPC_E500V1
	/* FP instructions can't setcc or be imm alurhs */
	CVMassert(opcode != CVME500_FADD_OPCODE &&
		  opcode != CVME500_FSUB_OPCODE && 
		  opcode != CVME500_FMUL_OPCODE &&
		  opcode != CVME500_FDIV_OPCODE); 
#endif
    }

    if (tokenType == CVMPPC_ALURHS_REGISTER) {
	int rhsRegID = CVMPPCalurhsDecodeGetTokenRegister(rhsToken);
	CVMUint32 rhsShift = ppcBinaryALUOpcodes[opcode].rhsShift;
	if (setcc) {
	    ppcOpcode = ppcBinaryALUOpcodes[opcode].opcodeRegSetcc;
	} else {
	    ppcOpcode = ppcBinaryALUOpcodes[opcode].opcodeReg;
	}
	CVMassert(ppcOpcode != 0);
	/* SUB of rhs is converted to SUBF (sub from) rhs */
	if (opcode == CVMCPU_SUB_OPCODE || opcode == CVMPPC_SUBE_OPCODE) {
	    int       tmpRegID = rhsRegID;
	    rhsRegID = lhsRegID;
	    lhsRegID = tmpRegID;
	}
	emitInstruction(con,
			ppcOpcode | destRegID << destShift |
			lhsRegID << lhsShift | rhsRegID << rhsShift |
			(setcc ? PPC_RECORD_BIT : 0));
	CVMtraceJITCodegenExec({
	    printPC(con);
	    if (opcode == CVMCPU_OR_OPCODE && lhsRegID == rhsRegID) {
		/* this is really a "move register" instruction */
		CVMconsolePrintf("	mr%s	r%s, r%s",
				 setcc ? "." : "",
				 regName(destRegID), regName(lhsRegID));
	    } else {
		CVMconsolePrintf("	%s%s	r%s, r%s, r%s",
				 getALUOpcodeName(ppcOpcode), setcc ? "." : "",
				 regName(destRegID), regName(lhsRegID),
				 regName(rhsRegID));
	    }
	});
    } else { /* CVMPPC_ALURHS_CONSTANT */
	CVMInt16 rhsConst = CVMPPCalurhsDecodeGetTokenConstant(rhsToken);
	CVMassert(tokenType == CVMPPC_ALURHS_CONSTANT);
	if (setcc) {
	    ppcOpcode = ppcBinaryALUOpcodes[opcode].opcodeImmSetcc;
	} else {
	    ppcOpcode = ppcBinaryALUOpcodes[opcode].opcodeImm;
	}
	CVMassert(ppcOpcode != 0);
	switch (opcode) {
	    case CVMCPU_SUB_OPCODE:
	    case CVMPPC_SUBE_OPCODE: {
		/* SUB of imm is converted to ADD of -imm */
		rhsConst = -rhsConst;
		break;
	    }
	    case CVMCPU_AND_OPCODE:
	    case CVMPPC_ANDIS_OPCODE: {
		/* WARNING: ANDI and ANDIS always do a setcc. */
		break;
	    }
	    case CVMPPC_SUBFIC_OPCODE:
	    case CVMCPU_OR_OPCODE:
	    case CVMPPC_ORIS_OPCODE:
	    case CVMCPU_XOR_OPCODE:
	    case CVMPPC_XORIS_OPCODE:
	    case CVMPPC_ADDIS_OPCODE:  {
		/* WARNING: SUBFIC, ORI, ORIS, XORI, XORIS, ADDIS 
		   can't do a setcc */
		CVMassert(!setcc);
		break;
	    }
	    default: break;
	}
	emitInstruction(con, ppcOpcode |
			destRegID << destShift | lhsRegID << lhsShift |
			((CVMUint16)rhsConst & 0xffff));
	CVMtraceJITCodegenExec({
	    printPC(con);
	    if ((opcode == CVMCPU_ADD_OPCODE || opcode == CVMCPU_SUB_OPCODE)
		&& lhsRegID == CVMPPC_r0) {
		/* this is really a "load immediate" instruction */
		CVMconsolePrintf("	li%s	r%s, %d",
				 setcc ? "." : "",
				 regName(destRegID),
				 opcode == CVMCPU_ADD_OPCODE ?
				     rhsConst : -rhsConst);
	    } else if (opcode == CVMPPC_ADDIS_OPCODE && lhsRegID == CVMPPC_r0){
		/* this is really a "load immediate" instruction */
		CVMconsolePrintf("	lis	r%s, 0x%x",
				 regName(destRegID), (CVMUint16)rhsConst);
	    } else if (opcode == CVMCPU_ADD_OPCODE && rhsConst == 0) {
		/* this is really a "move register" instruction */
		CVMconsolePrintf("	mr%s	r%s, r%s",
				 setcc ? "." : "",
				 regName(destRegID), regName(lhsRegID));
	    } else {
		CVMconsolePrintf("	%s%s	r%s, r%s, %d",
				 getALUOpcodeName(ppcOpcode), setcc ? "." : "",
				 regName(destRegID), regName(lhsRegID),
				 rhsConst);
	    }
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
    if (opcode == CVMCPU_BIC_OPCODE) {
	CVMUint8 maskBegin;  /* the MB field of the rlwinm instruction */
	CVMUint8 maskEnd;    /* the ME field of the rlwinm instruction */
	if (!CVMPPCgetEncodableRLWINM(&maskBegin, &maskEnd, ~rhsConstValue)) {
	    /* we know BIC is only used for consecutive bit sequences */
	    CVMassert(CVM_FALSE);
	}
	CVMJITaddCodegenComment((con, "BIC 0x%x", rhsConstValue));
	emitRLWINM(con, maskBegin, maskEnd, destRegID, lhsRegID, setcc);
	return;
    }

    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsToken = CVMPPCalurhsEncodeConstantToken(rhsConstValue);
	CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID, rhsToken, setcc);
    } else {
        /* Load the constant into a register. */
        CVMRMResource *rhsRes = CVMRMgetResourceForConstant32(
	    CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET,
	    rhsConstValue);
        rhsToken =
            CVMPPCalurhsEncodeRegisterToken(CVMRMgetRegisterNumber(rhsRes));
	CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID, rhsToken, setcc);
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), rhsRes);
    }
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit operand.*/
void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, CVMUint32 shiftAmount)
{
    CVMUint32 shiftInstruction;

    CVMassert(shiftAmount >> 5 == 0);

    switch(opcode) {
        case CVMCPU_SLL_OPCODE:
	    shiftInstruction = PPC_SLWI_OPCODE | 
		destRegID << PPC_RA_SHIFT | srcRegID << PPC_RS_SHIFT |
		shiftAmount << PPC_SH_SHIFT |
		(31-shiftAmount) << PPC_ME_SHIFT;
	    break;
        case CVMCPU_SRL_OPCODE:
	    shiftInstruction = PPC_SRWI_OPCODE | 
		destRegID << PPC_RA_SHIFT | srcRegID << PPC_RS_SHIFT |
		(shiftAmount == 0 ? 0 : 32-shiftAmount) << PPC_SH_SHIFT |
		shiftAmount << PPC_MB_SHIFT |
		31 << PPC_ME_SHIFT;
	    break;
        case CVMCPU_SRA_OPCODE:
	    shiftInstruction = PPC_SRAWI_OPCODE | 
		destRegID << PPC_RA_SHIFT | srcRegID << PPC_RS_SHIFT |
		shiftAmount << PPC_SH_SHIFT;
	    break;
        default: 
	    CVMassert(CVM_FALSE);
	    shiftInstruction = 0;
	    break;
    }

    emitInstruction(con, shiftInstruction); 

    CVMtraceJITCodegenExec({
	const char* opname = NULL;
        printPC(con);
	switch(opcode) {
            case CVMCPU_SLL_OPCODE: opname = "slwi"; break;
            case CVMCPU_SRL_OPCODE: opname = "srwi"; break;
            case CVMCPU_SRA_OPCODE: opname = "srawi"; break;
	}
        CVMconsolePrintf("	%s	r%s, r%s, %d",
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
    int scratchRegID = CVMPPC_r0;
    CVMUint32 ppcOpcode;

    /* Mask off the shift amount with 0x1f before shifting per VM spec. */
    CVMJITaddCodegenComment((con,
			     "vm spec says use low 5 bits of shift amount"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_AND_OPCODE,
        scratchRegID, shiftAmountRegID, 0x1F, CVMJIT_NOSETCC);

    switch(opcode) {
        case CVMCPU_SLL_OPCODE: ppcOpcode = PPC_SLW_OPCODE; break;
        case CVMCPU_SRL_OPCODE: ppcOpcode = PPC_SRW_OPCODE; break;
        case CVMCPU_SRA_OPCODE: ppcOpcode = PPC_SRAW_OPCODE; break;
        default: CVMassert(CVM_FALSE); ppcOpcode = 0; break;
    }
    emitInstruction(con, ppcOpcode | destRegID << PPC_RA_SHIFT |
        srcRegID << PPC_RS_SHIFT | scratchRegID << PPC_RB_SHIFT);

    CVMtraceJITCodegenExec({
	const char* opname = NULL;
        printPC(con);
	switch(opcode) {
            case CVMCPU_SLL_OPCODE: opname = "slw"; break;
            case CVMCPU_SRL_OPCODE: opname = "srw"; break;
            case CVMCPU_SRA_OPCODE: opname = "sraw"; break;
	}
        CVMconsolePrintf("	%s	r%s, r%s, r%s",
			 opname, regName(destRegID), regName(scratchRegID),
			 regName(shiftAmountRegID));
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitMulConstant(CVMJITCompilationContext* con,
		      int destreg, int lhsreg, CVMInt32 value)
{
    emitInstruction(con, PPC_MULLI_OPCODE | destreg << PPC_RD_SHIFT |
		    lhsreg << PPC_RA_SHIFT | (value & 0xffff));

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	mulli	r%s, r%s, %d",
			 regName(destreg), regName(lhsreg), value);
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
    CVMUint32 ppcOpcode;
#ifdef CVM_TRACE_JIT
    const char* opname = NULL;
#endif
    CVMassert(extrareg == CVMCPU_INVALID_REG);

    switch (opcode) {
        case CVMCPU_MULL_OPCODE:
	    CVMtraceJITCodegenExec(opname = "mullw");
	    ppcOpcode = PPC_MULLW_OPCODE;
	    break;
        case CVMCPU_MULH_OPCODE:
	    CVMtraceJITCodegenExec(opname = "mulhw");
	    ppcOpcode = PPC_MULHW_OPCODE;
	    break;
        case CVMPPC_MULHU_OPCODE:
	    CVMtraceJITCodegenExec(opname = "mulhwu");
	    ppcOpcode = PPC_MULHWU_OPCODE;
	    break;
        default:
	    CVMtraceJITCodegenExec(opname = "unknown");
	    ppcOpcode = 0;
	    CVMassert(CVM_FALSE);
    }

    emitInstruction(con, ppcOpcode | destreg << PPC_RD_SHIFT |
		    lhsreg << PPC_RA_SHIFT | rhsreg << PPC_RB_SHIFT);
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	%s	r%s, r%s, r%s", opname,
			 regName(destreg), regName(lhsreg), regName(rhsreg));
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitMove(CVMJITCompilationContext* con, int opcode,
	       int destRegID, CVMCPUALURhsToken srcToken,
	       CVMBool setcc)
{
    int srcRegID = CVMPPCalurhsDecodeGetTokenRegister(srcToken);

    CVMassert(CVMPPCalurhsDecodeGetTokenType(srcToken) ==
	      CVMPPC_ALURHS_REGISTER);

    switch (opcode){
    case CVMCPU_MOV_OPCODE:
	/* "or" the source register with itself. */
	CVMCPUemitBinaryALURegister(con, CVMCPU_OR_OPCODE, destRegID,
				    srcRegID, srcRegID, setcc);
	break;
#ifdef CVM_JIT_USE_FP_HARDWARE
    case CVMCPU_FMOV_OPCODE:
	CVMassert(!setcc);
	CVMCPUemitUnaryFP(con, CVMPPC_FMOV_OPCODE, destRegID, srcRegID);
	break;
#endif
    default:
	CVMassert(CVM_FALSE);
    }
}

static void
CVMPPCemitCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken,
		  CVMBool signedCompare)
{
    int type = CVMPPCalurhsDecodeGetTokenType(rhsToken);
    CVMassert(opcode == CVMCPU_CMP_OPCODE || opcode == CVMCPU_CMN_OPCODE);

    if (opcode == CVMCPU_CMP_OPCODE) {
	if (type == CVMPPC_ALURHS_REGISTER) {
	    int ppcOpcode =
		(signedCompare ? PPC_CMP_OPCODE : PPC_CMPL_OPCODE);
	    int rhsRegID = CVMPPCalurhsDecodeGetTokenRegister(rhsToken);
	    emitInstruction(con, ppcOpcode | lhsRegID << PPC_RA_SHIFT |
			    rhsRegID << PPC_RB_SHIFT );
	    CVMtraceJITCodegenExec({
		printPC(con);
		CVMconsolePrintf("	cmp%s	r%s, r%s",
				 signedCompare ? "" : "l",
				 regName(lhsRegID), regName(rhsRegID));
	    });
	} else {
	    int ppcOpcode =
		(signedCompare ? PPC_CMPI_OPCODE : PPC_CMPLI_OPCODE);
	    CVMUint16 rhsConst = CVMPPCalurhsDecodeGetTokenConstant(rhsToken);
	    emitInstruction(con, ppcOpcode | lhsRegID << PPC_RA_SHIFT |
			    rhsConst);
	    CVMtraceJITCodegenExec({
		printPC(con);
		CVMconsolePrintf("	cmpi%s	r%s, %d",
				 signedCompare ? "" : "l",
				 regName(lhsRegID), rhsConst);
	    });
	}
	CVMJITdumpCodegenComments(con);
    } else { /* CVMCPU_CMN_OPCODE */
	/*
	 * There's no such thing as an "unsigned" cmn, which makes it easier.
	 * We can just add the two values to do the compare.
	 */
	CVMassert(signedCompare);
	CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
			    CVMPPC_r0, lhsRegID, rhsToken, CVMJIT_SETCC);
    }
}

void
CVMCPUemitCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken)
{
    CVMPPCemitCompare(con, opcode, condCode, lhsRegID, rhsToken,
		      ppcConditions[condCode].signedCompare);
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
              
                   subfic  destRegID+1, srcRegID+1, 0
                   subfze  destRegID, srcRegID
             */
	    emitInstruction(con, PPC_SUBFIC_OPCODE |
			    (destRegID+1) << PPC_RD_SHIFT |
			    (srcRegID+1) << PPC_RA_SHIFT);
	    CVMtraceJITCodegenExec({
		printPC(con);
		CVMconsolePrintf("	subfic	r%s, r%s, 0",
				 regName(destRegID+1),regName(srcRegID+1));
	    });
	    emitInstruction(con, PPC_SUBFZE_OPCODE |
			    destRegID << PPC_RD_SHIFT |
			    srcRegID << PPC_RA_SHIFT);
	    CVMtraceJITCodegenExec({
		printPC(con);
		CVMconsolePrintf("	subfze	r%s, r%s",
				 regName(destRegID), regName(srcRegID));
	    });
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
           lhs+1 * rhs+1         --> destreg+1 (low), destreg (high)
           lhs * rhs+1 + destreg --> destreg
           lhs+1 * rhs + destreg --> destreg

	   mulhwu dest,lhs+1,rhs+1
	   mullw  dest+1,lhs+1,rhs+1
	   mullw  scratch,lhs,rhs+1
	   add    dest,dest,scratch
	   mullw  scratch,lhs+1,rhs
	   add    dest,dest,scratch
         */
	int scratchRegID = CVMPPC_r0;
        CVMCPUemitMul(con, CVMPPC_MULHU_OPCODE,
		      destRegID, lhsRegID+1, rhsRegID+1, CVMCPU_INVALID_REG);
        CVMCPUemitMul(con, CVMCPU_MULL_OPCODE,
		      destRegID+1, lhsRegID+1, rhsRegID+1, CVMCPU_INVALID_REG);
        CVMCPUemitMul(con, CVMCPU_MULL_OPCODE,
		      scratchRegID, lhsRegID, rhsRegID+1, CVMCPU_INVALID_REG);
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, destRegID,
			    CVMPPCalurhsEncodeRegisterToken(scratchRegID),
			    CVMJIT_NOSETCC);
        CVMCPUemitMul(con, CVMCPU_MULL_OPCODE,
		      scratchRegID, lhsRegID+1, rhsRegID, CVMCPU_INVALID_REG);
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, destRegID,
			    CVMPPCalurhsEncodeRegisterToken(scratchRegID),
			    CVMJIT_NOSETCC);
    } else {
        int lowOpcode  = (opcode >> 16) & 0xff;
        int highOpcode = (opcode >> 8) & 0xff;
        CVMBool setcc  = (opcode & 0xff) != 0;
        CVMCPUemitBinaryALU(con, lowOpcode, destRegID+1, lhsRegID+1,
			    CVMPPCalurhsEncodeRegisterToken(rhsRegID+1),
			    setcc);
        CVMCPUemitBinaryALU(con, highOpcode, destRegID, lhsRegID,
			    CVMPPCalurhsEncodeRegisterToken(rhsRegID), 
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
    CVMCPUemitLoadConstant(con, regID, val.l>>32);
    CVMCPUemitLoadConstant(con, regID+1, (int)val.l);
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
    CVMInt32 logicalPC;
    CVMassert(opcode == CVMCPU_CMP64_OPCODE);

    /*
     * Implemented as:
     *
     *     cmp    lhsRegID, rhsRegID
     *     bne    .label1     !.label = pc + 2 * CVMCPU_INSTRUCTION_SIZE
     *     cmpl   lhsRegID+1, rhsRegID+1
     */
    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, condCode,
			      lhsRegID, rhsRegID);
    logicalPC = CVMJITcbufGetLogicalPC(con) + 2 * CVMCPU_INSTRUCTION_SIZE;
    CVMCPUemitBranch(con, logicalPC, CVMCPU_COND_NE);
    CVMPPCemitCompare(con, CVMCPU_CMP_OPCODE, condCode,
		      lhsRegID+1, CVMPPCalurhsEncodeRegisterToken(rhsRegID+1),
		      CVM_FALSE /* unsiged */);
    return condCode;
}

/* Purpose: Emits instructions to convert a 32 bit int into a 64 bit int. */
void
CVMCPUemitInt2Long(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID+1, srcRegID, CVMJIT_NOSETCC);
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE,
			      destRegID, srcRegID, 31);
}

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID, srcRegID+1, CVMJIT_NOSETCC);
}

extern void
CVMCPUemitCompareConstant(CVMJITCompilationContext* con,
			  int opcode, CVMCPUCondCode condCode,
			  int lhsRegID, CVMInt32 rhsConstValue)
{
    CVMCPUALURhsToken rhsToken;

    /* rhsConstValue may not be encodable as an immediate value */
    if (CVMCPUalurhsIsEncodableAsImmediate(opcode, rhsConstValue)) {
        rhsToken = CVMPPCalurhsEncodeConstantToken(rhsConstValue);
	CVMCPUemitCompare(con, opcode, condCode, lhsRegID, rhsToken);
    } else {
        /* Load rhsConstValue into a register. */
        CVMRMResource* rhsRes = CVMRMgetResourceForConstant32(
            CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET,
	    rhsConstValue);
        rhsToken =
            CVMPPCalurhsEncodeRegisterToken(CVMRMgetRegisterNumber(rhsRes));
	CVMCPUemitCompare(con, opcode, condCode, lhsRegID, rhsToken);
	CVMRMrelinquishResource(CVMRM_INT_REGS(con), rhsRes);
   }
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
        /* addi regno, r0, v */
        CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
            regno, CVMPPC_r0, CVMPPCalurhsEncodeConstantToken(v),
            CVMJIT_NOSETCC);
    } else if ((v & 0xffff) == 0) {
        /* addis regno, r0, v >> 16 */
        CVMCPUemitBinaryALU(con, CVMPPC_ADDIS_OPCODE,
            regno, CVMPPC_r0,
	    CVMPPCalurhsEncodeConstantToken((v >> 16) & 0xffff),
            CVMJIT_NOSETCC);
    } else {
#ifndef CVMCPU_HAS_CP_REG
	/* make the constant without using the constant pool */
	CVMPPCemitMakeConstant32(con, regno, v);
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
 * on PowerPC, because there is no support for dong a pc-relative
 * load. We have to load the current address as a constant, and
 * then load relative to it. Fortunately the need for this is
 * rare and is in code that is somewhat slow already, like
 * dealing with potentially unresolved constant pool entries.
 *
 * NOTE: This function must always emit the exact same number of
 *       instructions regardless of the offset passed to it.
 */
void
CVMPPCemitMemoryReferencePCRelative(CVMJITCompilationContext* con, int opcode,
				    int destRegID, int lhsRegID, int offset)
{
    CVMInt32 physicalPC = (CVMInt32)CVMJITcbufGetPhysicalPC(con);
    CVMInt32 addr = physicalPC + offset;
    CVMInt16  lo16 =  addr & 0xffff;
    CVMUint16 ha16 = (addr >> 16) & 0xffff;

    /*
     *   addis    destRegID, r0, ha16(physicalPC)
     *   lwz      destRegID, lo16(physicalPC)(destRegID)
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
    CVMCPUemitBinaryALUConstant(con, CVMPPC_ADDIS_OPCODE,
				destRegID, lhsRegID, ha16, CVM_FALSE);
    CVMCPUemitMemoryReference(con, opcode,
        destRegID, destRegID, CVMCPUmemspecEncodeImmediateToken(con, lo16));
}

/*
 * Table that provides encoding information for register based binary
 * ALU instructions. ARM has very consistent encodings across memory
 * access opcodes, so the encoding can easily be done in the
 * CVMCPU_XXX_OPCODE value, plus some uniform masking of things like
 * register encodings and "signed" bits. PowerPC has very inconsistent
 * encodings, so making it table based is necessary.
 */
typedef struct CVMPPCMemoryReferenceOpcode CVMPPCMemoryReferenceOpcode;
struct CVMPPCMemoryReferenceOpcode {
    CVMUint32 opcodeReg;       /* load/store with reg offset */
    CVMUint32 opcodeImm;       /* load/store with imm offset */
    CVMUint32 opcodeImmUpdate; /* load/store with imm offset and update */
};

static const CVMPPCMemoryReferenceOpcode ppcMemoryReferenceOpcode[] = {
    /* LDR8U */ {PPC_LBZX_OPCODE, PPC_LBZ_OPCODE, PPC_LBZU_OPCODE},
    /* LDR8  */ {PPC_LBZX_OPCODE, PPC_LBZ_OPCODE, PPC_LBZU_OPCODE},
    /* STR8  */ {PPC_STBX_OPCODE, PPC_STB_OPCODE, PPC_STBU_OPCODE},
    /* LDR16U*/ {PPC_LHZX_OPCODE, PPC_LHZ_OPCODE, PPC_LHZU_OPCODE},
    /* LDR16 */ {PPC_LHAX_OPCODE, PPC_LHA_OPCODE, PPC_LHAU_OPCODE},
    /* STR16 */ {PPC_STHX_OPCODE, PPC_STH_OPCODE, PPC_STHU_OPCODE},
    /* LDR32 */ {PPC_LWZX_OPCODE, PPC_LWZ_OPCODE, PPC_LWZU_OPCODE},
    /* STR32 */ {PPC_STWX_OPCODE, PPC_STW_OPCODE, PPC_STWU_OPCODE},
    /* LDR64 and STR64 have special handling */
#ifdef CVM_JIT_USE_FP_HARDWARE
		{-1,              -1,             -1             },
		{-1,              -1,             -1             },
    /* FLDR32*/ {PPC_LFSX_OPCODE,  PPC_LFS_OPCODE,  PPC_LFSU_OPCODE},
    /* FSTR32*/ {PPC_STFSX_OPCODE, PPC_STFS_OPCODE, PPC_STFSU_OPCODE},
    /* FLDR64*/ {PPC_LFDX_OPCODE,  PPC_LFD_OPCODE,  PPC_LFDU_OPCODE},
    /* FSTR64*/ {PPC_STFDX_OPCODE, PPC_STFD_OPCODE, PPC_STFDU_OPCODE}
#endif
};

/*
 * Emit a 32-bit memory reference, or a 64-bit
 * floating-point reference.
 * NOTE: since powerpc does not support post-increment, this function
 * will not do any adjustment to the basereg if post-increment is
 * requested. It is up to the caller to handle this. This is so when
 * the caller is doing a 64-bit memory reference, it can do the
 * adjustment in one instruction.
 */
static void
CVMPPCemitPrimitiveMemoryReference(CVMJITCompilationContext* con,
    int opcode,
    int destreg, /* or srcreg for loads */
    int basereg,
    CVMCPUMemSpecToken memSpecToken)
{
    CVMUint32 ppcOpcode;
    int type = CVMPPCmemspecGetTokenType(memSpecToken);

   /* emit the load or store instruction */
    if (type == CVMCPU_MEMSPEC_REG_OFFSET) {
        int indexReg = CVMPPCmemspecGetTokenRegister(memSpecToken);
	ppcOpcode = ppcMemoryReferenceOpcode[opcode].opcodeReg;
	emitInstruction(con, ppcOpcode | destreg << PPC_RD_SHIFT |
			basereg << PPC_RA_SHIFT | indexReg << PPC_RB_SHIFT);
    } else {
        int offsetImm;
	switch (type) {
	    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
		offsetImm = CVMPPCmemspecGetTokenOffset(memSpecToken);
		ppcOpcode = ppcMemoryReferenceOpcode[opcode].opcodeImm;
		break;
	    case CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET:
		offsetImm = 0; /* real offset is added in later by caller */
		ppcOpcode = ppcMemoryReferenceOpcode[opcode].opcodeImm;
		break;
	    case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
		offsetImm = -CVMPPCmemspecGetTokenOffset(memSpecToken);
		ppcOpcode = ppcMemoryReferenceOpcode[opcode].opcodeImmUpdate;
		break;
	    default:
		ppcOpcode = 0;
		offsetImm = 0;
		CVMassert(CVM_FALSE);
	}
	emitInstruction(con, ppcOpcode | destreg << PPC_RD_SHIFT |
			basereg << PPC_RA_SHIFT | (offsetImm & 0xffff));
    }

    CVMJITstatsExec({
        /* This is going to be a memory reference. Determine
           if read or write */
        switch(opcode) {
        case CVMCPU_LDR32_OPCODE:
        case CVMCPU_LDR16U_OPCODE:
        case CVMCPU_LDR16_OPCODE:
        case CVMCPU_LDR8_OPCODE:
        case CVMCPU_LDR8U_OPCODE:
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_FLDR64_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD); break;
        case CVMCPU_STR32_OPCODE:
        case CVMCPU_STR16_OPCODE:
        case CVMCPU_STR8_OPCODE:
        case CVMCPU_FSTR32_OPCODE:
        case CVMCPU_FSTR64_OPCODE:
            CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
        default:
            CVMassert(CVM_FALSE);
        }
    });

    CVMtraceJITCodegenExec({
	char destregbuf[30];
        printPC(con);
	switch(opcode){
        case CVMCPU_FLDR32_OPCODE:
        case CVMCPU_FLDR64_OPCODE:
        case CVMCPU_FSTR32_OPCODE:
        case CVMCPU_FSTR64_OPCODE:
	    CVMformatString(destregbuf, sizeof(destregbuf),
			    "f%d", destreg);
	    break;
	default:
	    CVMformatString(destregbuf, sizeof(destregbuf),
			    "r%s", regName(destreg));
	    break;
	}
	if (type == CVMCPU_MEMSPEC_REG_OFFSET) {
	    int indexReg = CVMPPCmemspecGetTokenRegister(memSpecToken);
	    CVMconsolePrintf("	%s	%s, r%s, r%s",
			     getMemOpcodeName(ppcOpcode),
			     destregbuf, regName(basereg), regName(indexReg));
	} else {
	    int offsetImm = CVMPPCmemspecGetTokenOffset(memSpecToken);
	    if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
		offsetImm = -offsetImm;
	    } else if (type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET) {
		offsetImm = 0; /* real offset is added in later by caller */
	    }
	    CVMconsolePrintf("	%s	%s, %d(r%s)",
			     getMemOpcodeName(ppcOpcode),
			     destregbuf, offsetImm, regName(basereg));
	}
    });
 
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitMemoryReference(CVMJITCompilationContext* con,
    int opcode,
    int destreg, /* or srcreg for loads */
    int basereg,
    CVMCPUMemSpecToken memSpecToken)
{
    int type = CVMPPCmemspecGetTokenType(memSpecToken);

    if (opcode != CVMCPU_LDR64_OPCODE && opcode != CVMCPU_STR64_OPCODE) {
	CVMPPCemitPrimitiveMemoryReference(con, opcode,
				    destreg, basereg, memSpecToken);
    } else {
	/* 64-bit load/store implemented as two LDR32 or STR32 */
	int opcode32 = (opcode == CVMCPU_LDR64_OPCODE ?
			CVMCPU_LDR32_OPCODE : CVMCPU_STR32_OPCODE);
	if (type == CVMCPU_MEMSPEC_REG_OFFSET) {
	    /*
	     * There is no support for an indexed-based ldm or stm, so we
	     * must add the indexReg to the basereg so we can use an
	     * immmediate offset. We could then use ldm or stm, but two
	     * loads or stores is usually about 2 cycles faster.
	     */
	    CVMRMResource* newBaseregRes =
		CVMRMgetResource(CVMRM_INT_REGS(con),
				 CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1); 
	    int newBasereg = CVMRMgetRegisterNumber(newBaseregRes);
	    int indexReg = CVMPPCmemspecGetTokenRegister(memSpecToken);
	    CVMCPUMemSpecToken tmpToken;
	    /* add basereg and indexReg */
	    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
				newBasereg, basereg, indexReg, CVMJIT_NOSETCC);
	    /* first word */
	    CVMPPCemitPrimitiveMemoryReference(con, opcode32,
					destreg, basereg, memSpecToken);
	    /* second word */
	    tmpToken = CVMCPUmemspecEncodeImmediateToken(con, 4);
	    CVMPPCemitPrimitiveMemoryReference(con, opcode32,
					destreg+1, newBasereg, tmpToken);
	    CVMRMrelinquishResource(CVMRM_INT_REGS(con), newBaseregRes);
	}
	else {
	    /* immediate memspec */
	    CVMCPUMemSpecToken tmpToken;
	    int offsetImm;
	    if (type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET) {
		offsetImm = 0; /* real offset is added in later */
	    } else {
		offsetImm = CVMPPCmemspecGetTokenOffset(memSpecToken);
	    }
	    /* first word */
	    if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
		tmpToken = CVMCPUmemspecEncodePreDecrementImmediateToken(
                    con, offsetImm);
	    } else {
		tmpToken = CVMCPUmemspecEncodeImmediateToken(con, offsetImm);
	    }
	    CVMPPCemitPrimitiveMemoryReference(con, opcode32,
					destreg, basereg, tmpToken);
	    if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
		/* we already did the predecrement */
		offsetImm = 0;
	    }
	    /* second word */
	    if (CVMCPUmemspecIsEncodableAsImmediate(offsetImm + 4)) {
		tmpToken = 
		    CVMCPUmemspecEncodeImmediateToken(con, offsetImm + 4);
		CVMPPCemitPrimitiveMemoryReference(con, opcode32,
					    destreg+1, basereg, tmpToken);
	    } else {
		/* Load immOffset+4 into a register. */
		CVMRMResource* offsetRes = CVMRMgetResourceForConstant32(
		    CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET,
		    offsetImm + 4);
		tmpToken = CVMPPCmemspecEncodeToken(
                    CVMCPU_MEMSPEC_REG_OFFSET,
		    CVMRMgetRegisterNumber(offsetRes));
		CVMCPUemitMemoryReference(con, opcode32,
					  destreg+1, basereg, tmpToken);
		CVMRMrelinquishResource(CVMRM_INT_REGS(con), offsetRes);
	    }
	}
    }

    /* There is no post-increment on ppc, so we must manually post-increment
     * the base register */
    if (type == CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET) {
	int offsetImm = CVMPPCmemspecGetTokenOffset(memSpecToken);
	CVMJITaddCodegenComment((con, "do manual post increment"));
	CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE, basereg, 
				    basereg, offsetImm, CVMJIT_NOSETCC);
    }

    /* There is no signed "ldr8" on ppc, so we must manually extend the sign */
    if (opcode == CVMCPU_LDR8_OPCODE) {
	emitInstruction(con, PPC_EXTSB_OPCODE | destreg << PPC_RA_SHIFT |
			destreg << PPC_RS_SHIFT);
	CVMJITaddCodegenComment((con, "sign extend loaded byte"));
	CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("	extsb	r%s, r%s",
			     regName(destreg), regName(destreg));
	});
	CVMJITdumpCodegenComments(con);
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
            CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET,
				     CVMRMgetRegisterNumber(offsetRes)));
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), offsetRes);
    }
}

/* Purpose: Emits instructions to do a load/store operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
    or
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
   
   On PPC, this is done in two instructions: shift instruction
   and load/store instruction.
*/
void
CVMCPUemitArrayElementReference(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID,
    int indexRegID, int shiftOpcode, int shiftAmount)
{
    /* emit shift instruction first. Use valueRegID to temporarily
     * store the shift value. */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMPPC_r0, 
                              indexRegID, shiftAmount);

    /* then, the load/store instruction: */
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMPPCmemspecEncodeToken(CVMCPU_MEMSPEC_REG_OFFSET, CVMPPC_r0));
}

/* Purpose: Emits code to computes the following expression and stores the
            result in the specified destReg:
                destReg = baseReg + shiftOpcode(indexReg, #shiftAmount)

   On PPC, this is done in two instructions: shift instruction
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
     
           slwi r0, shiftRegID, shiftAmount
           add  destRegID, addRegID, r0
     */
    /* Use destRegID as the scratch register. */
    CVMCPUemitShiftByConstant(con, shiftOpcode, CVMPPC_r0,
                              shiftRegID, shiftAmount);
    CVMCPUemitBinaryALURegister(con, CVMCPU_ADD_OPCODE,
				destRegID, addRegID, CVMPPC_r0,
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
    int retryLogicalPC;

    CVMtraceJITCodegen(("\t\t"));
    CVMJITdumpCodegenComments(con);

    /* top of retry loop in case CAS operation is interrupted. */
    CVMtraceJITCodegen(("\t\tretry:\n"));
    retryLogicalPC = CVMJITcbufGetLogicalPC(con);

    /* emit LWARX instruction */
    emitInstruction(con, PPC_LWARX_OPCODE | CVMPPC_r0 << PPC_RD_SHIFT |
		    CVMPPC_r0 << PPC_RA_SHIFT | addressReg << PPC_RB_SHIFT);
    CVMJITaddCodegenComment((con, "fetch word from address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	lwarx	r%s, r%s, r%s",
			 regName(CVMPPC_r0), regName(CVMPPC_r0),
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* emit STWCX. instruction */
    emitInstruction(con, PPC_STWCX_OPCODE | CVMPPC_r0 << PPC_RD_SHIFT |
		    CVMPPC_r0 << PPC_RA_SHIFT | addressReg << PPC_RB_SHIFT);
    CVMJITaddCodegenComment((con, "conditionally store new word in address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	stwcx.	r%s, r%s, r%s",
			 regName(destReg), regName(CVMPPC_r0),
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* Retry if reservation was lost and atomic operation failed */
    CVMJITaddCodegenComment((con, "br retry if reservation was lost"));
    CVMPPCemitBranch0(con, retryLogicalPC,
		      CVMCPU_COND_NE, CVM_FALSE,
		      CVM_FALSE /* assume branch is not taken */);

    /* Move the old value into destReg */
    CVMJITaddCodegenComment((con, "move old value into destReg"));
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destReg, CVMPPC_r0, CVMJIT_NOSETCC);

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
    CVMRMResource* offsetRes;
    int offsetReg;
    int branchLogicalPC;
    int retryLogicalPC;
#ifdef CVM_TRACE_JIT
    const char* branchLabel = CVMJITgetSymbolName(con);
#endif
    CVMJITresetSymbolName(con);

    /* move addressOffset into a register */
    CVMJITsetSymbolName((con, "CAS address offset"));
    offsetRes = CVMRMgetResourceForConstant32(
	CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET, addressOffset);
    offsetReg = CVMRMgetRegisterNumber(offsetRes);
    CVMJITresetSymbolName(con);

    /* top of retry loop in case CAS operation is interrupted. */
    CVMtraceJITCodegen(("\t\tretry:\n"));
    retryLogicalPC = CVMJITcbufGetLogicalPC(con);

    /* emit LWARX instruction */
    emitInstruction(con, PPC_LWARX_OPCODE | CVMPPC_r0 << PPC_RD_SHIFT |
		    offsetReg << PPC_RA_SHIFT | addressReg << PPC_RB_SHIFT);
    CVMJITaddCodegenComment((con, "fetch word from address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	lwarx	r%s, r%s, r%s",
			 regName(CVMPPC_r0), regName(offsetReg),
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);
    
    /* Check if LWARX loaded the expected word */
    CVMJITaddCodegenComment((con, "check if word is as expected "));
    CVMCPUemitCompareRegister(con, CVMCPU_CMP_OPCODE, CVMCPU_COND_EQ,
                              CVMPPC_r0, oldValReg);

    /* Branch to failure addres if expected word not loaded. The proper
     * branch address will be patched in by the caller. */
    branchLogicalPC = CVMJITcbufGetLogicalPC(con);
    CVMJITaddCodegenComment((con, "br %s if word not as expected",
			     branchLabel));
    CVMPPCemitBranch0(con, CVMJITcbufGetLogicalPC(con),
		      CVMCPU_COND_NE, CVM_FALSE,
		      CVM_TRUE /* assume branch is not taken */);

    /* emit STWCX. instruction */
    emitInstruction(con, PPC_STWCX_OPCODE | newValReg << PPC_RD_SHIFT |
		    offsetReg << PPC_RA_SHIFT | addressReg << PPC_RB_SHIFT);
    CVMJITaddCodegenComment((con, "conditionally store new word in address"));
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	stwcx.	r%s, r%s, r%s",
			 regName(newValReg), regName(offsetReg),
			 regName(addressReg));
    });
    CVMJITdumpCodegenComments(con);

    /* Retry if reservation was lost and atomic operation failed */
    CVMJITaddCodegenComment((con, "br retry if reservation was lost"));
    CVMPPCemitBranch0(con, retryLogicalPC,
		      CVMCPU_COND_NE, CVM_FALSE,
		      CVM_FALSE /* assume branch is not taken */);

    CVMJITprintCodegenComment(("End of CAS"));

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), offsetRes);
    return branchLogicalPC;
}

#endif /* CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS */
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/* glue routine that calls CVMCCMruntimeGCRendezvous and adjusts
   the return address. */
extern void
CVMPPCruntimeGCRendezvousRetAddrAdjustGlue();

/*
 * Emit code to do a gc rendezvous.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind)
{
    /* Emit the gc rendezvous instruction(s) */
    if (cbufRewind) {
        /* The code buffer will be rewinded after the bl instruction,
         * and the bl will be patched with the next instruction. In
         * this case, CVMPPCruntimeGCRendezvousRetAddrAdjustGlue is 
         * going to subtract the lr by 4, so we can return to the 
         * patch point after the gc is done.
         */
        CVMJITaddCodegenComment((con, "CVMPPCruntimeGCRendezvousRetAddrAdjustGlue"));
        CVMCPUemitAbsoluteCall(con, CVMPPCruntimeGCRendezvousRetAddrAdjustGlue,
			       CVMJIT_CPDUMPOK, CVMJIT_NOCPBRANCH);
    } else {
        /* There is no rewinding in this case after emitting the
         * gc rendezvous call. We want to continue with the next
         * instruction after gc is done, so call 
         * CVMCCMruntimeGCRendezvousGlue and skip the
         * return address adjustment.
         */
        CVMJITaddCodegenComment((con, 
                    "CVMCCMruntimeGCRendezvousGlue"));
        CVMCPUemitAbsoluteCall(con, 
                    CVMCCMruntimeGCRendezvousGlue,
                    CVMJIT_CPDUMPOK, CVMJIT_NOCPBRANCH);
    }
}
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

#ifdef CVM_JIT_USE_FP_HARDWARE
/* Purpose: Emits instructions to do the specified floating point operation. */

/*
 * WARNING: using fmadd results in a non-compliant vm. Some floating
 * point tck tests will fail. Therefore, this support if off by defualt.
 */
#ifdef CVM_JIT_USE_FMADD

static const struct {
    int   mulOpcode;
    int   addOpcode;
    int   negOpcode;
} CVMPPCstrictfpOpcodes[] = {
    /* CVMPPC_FMADD_OPCODE */
    {CVMCPU_FMUL_OPCODE, CVMCPU_FADD_OPCODE, -1},
    /* CVMPPC_DMADD_OPCODE */
    {CVMCPU_DMUL_OPCODE, CVMCPU_DADD_OPCODE, -1},
    /* CVMPPC_FMSUB_OPCODE */
    {CVMCPU_FMUL_OPCODE, CVMCPU_FSUB_OPCODE, -1},
    /* CVMPPC_DMSUB_OPCODE */
    {CVMCPU_DMUL_OPCODE, CVMCPU_DSUB_OPCODE, -1},
    /* CVMPPC_FNMADD_OPCODE */
    {CVMCPU_FMUL_OPCODE, CVMCPU_FADD_OPCODE, CVMCPU_FNEG_OPCODE}, 
    /* CVMPPC_DNMADD_OPCODE */
    {CVMCPU_DMUL_OPCODE, CVMCPU_DADD_OPCODE, CVMCPU_DNEG_OPCODE}, 
    /* CVMPPC_FNMSUB_OPCODE */
    {CVMCPU_FMUL_OPCODE, CVMCPU_FSUB_OPCODE, CVMCPU_FNEG_OPCODE}, 
    /* CVMPPC_DNMSUB_OPCODE */
    {CVMCPU_DMUL_OPCODE, CVMCPU_DSUB_OPCODE, CVMCPU_DNEG_OPCODE}  
};

void
CVMPPCemitTernaryFP(CVMJITCompilationContext* con,
		    int opcode, int regidD,
		    int regidA, int regidC, int regidB)
{
    CVMUint32 ppcOpcode;
    CVMassert(
        opcode == CVMPPC_FMADD_OPCODE  || opcode == CVMPPC_DMADD_OPCODE ||
	opcode == CVMPPC_FMSUB_OPCODE  || opcode == CVMPPC_DMSUB_OPCODE ||
	opcode == CVMPPC_FNMADD_OPCODE || opcode == CVMPPC_DNMADD_OPCODE ||
	opcode == CVMPPC_FNMSUB_OPCODE || opcode == CVMPPC_DNMSUB_OPCODE);
   
    ppcOpcode = ppcFPOpcodes[opcode];
    emitInstruction(con, ppcOpcode |
		    regidD << PPC_RD_SHIFT | regidA << PPC_RA_SHIFT |
		    regidB << PPC_RB_SHIFT | regidC << PPC_RC_SHIFT);
    
    CVMtraceJITCodegenExec({
	printPC(con);
	CVMconsolePrintf("	%s	f%d, f%d, f%d, f%d",
			 getFPOpcodeName(ppcOpcode),
			 regidD, regidA, regidC, regidB);
    });
    CVMJITdumpCodegenComments(con);
}
#endif /* CVM_JIT_USE_FMADD */

void
CVMCPUemitBinaryFP( CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID, int rhsRegID)
{
    CVMUint32 ppcOpcode;
    CVMassert(opcode == CVMCPU_FADD_OPCODE || opcode == CVMCPU_FSUB_OPCODE ||
              opcode == CVMCPU_FMUL_OPCODE || opcode == CVMCPU_FDIV_OPCODE ||
              opcode == CVMCPU_DADD_OPCODE || opcode == CVMCPU_DSUB_OPCODE ||
              opcode == CVMCPU_DMUL_OPCODE || opcode == CVMCPU_DDIV_OPCODE );

    ppcOpcode = ppcFPOpcodes[opcode];
    switch (opcode){
    case CVMCPU_FMUL_OPCODE:
    case CVMCPU_DMUL_OPCODE:
	emitInstruction(con, ppcOpcode | destRegID << PPC_RD_SHIFT |
		    lhsRegID << PPC_RA_SHIFT | rhsRegID << PPC_RC_SHIFT);
	break;
    default:
	emitInstruction(con, ppcOpcode | destRegID << PPC_RD_SHIFT |
		    lhsRegID << PPC_RA_SHIFT | rhsRegID << PPC_RB_SHIFT);
    }

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	f%d, f%d, f%d",
            getFPOpcodeName(ppcOpcode),
            destRegID, lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitUnaryFP( CVMJITCompilationContext* con,
		    int opcode, int destRegID, int srcRegID)
{
    CVMUint32 ppcOpcode;
    CVMassert(opcode == CVMCPU_FNEG_OPCODE || opcode == CVMCPU_DNEG_OPCODE || 
              opcode == CVMPPC_FMOV_OPCODE );

    ppcOpcode = ppcFPOpcodes[opcode];
    emitInstruction(con, ppcOpcode | destRegID << PPC_RD_SHIFT |
		    srcRegID << PPC_RB_SHIFT);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	%s	f%d, f%d",
            getFPOpcodeName(ppcOpcode),
            destRegID, srcRegID);
    });
    CVMJITdumpCodegenComments(con);
}

void
CVMCPUemitFCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, int rhsRegID)
{
    int destCCbit;
    CVMassert(opcode == CVMCPU_FCMP_OPCODE || opcode == CVMCPU_DCMP_OPCODE);

    emitInstruction(con, PPC_FCMPU_OPCODE | 
		    lhsRegID << PPC_RA_SHIFT | rhsRegID << PPC_RB_SHIFT);

    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	fcmpu	f%d, f%d",
            lhsRegID, rhsRegID);
    });
    CVMJITdumpCodegenComments(con);
    /*
     * Handling of unordered cases may require condition code
     * merging.
     * -1 => impossible cases
     *  0 => no merging necessary
     *  1 => add cror 1,1,3 after compare
     *  2 => add cror 0,0,3 after compare
     * Index this table using CVMCPU_COND_UNORDERED_LT bit, if any,
     * in place.
     */
    switch(needFloatConditionBitMerge[condCode]){
    default: CVMassert(CVM_FALSE);
    case  0: return;
    case  1: destCCbit = 1;
	     break;
    case  2: destCCbit = 0;
	     break;
    }
    emitInstruction(con, PPC_CROR_OPCODE | destCCbit << PPC_RD_SHIFT |
		    destCCbit << PPC_RA_SHIFT | 3 << PPC_RB_SHIFT);
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	cror	%d, %d, 3\n", destCCbit, destCCbit);
    });
}

extern void
CVMCPUemitLoadConstantFP(
    CVMJITCompilationContext *con,
    int regID,
    CVMInt32 v)
{
    /*
     * there is no way to construct a constant in a floating-point register,
     * so we must load a constant from the constant pool.
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
    CVMInt32 offset;
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    offset = CVMJITgetRuntimeConstantReference64(con, logicalPC, value);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR64_OPCODE, regID, CVMCPU_CP_REG, offset);
}

#endif

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
	    return (diff >= CVMPPC_MIN_BRANCH_OFFSET &&
		    diff <= CVMPPC_MAX_BRANCH_OFFSET);
        case CVMJIT_COND_BRANCH_ADDRESS_MODE:
	    return (diff >= CVMPPC_MIN_COND_BRANCH_OFFSET &&
		    diff <= CVMPPC_MAX_COND_BRANCH_OFFSET);
        case CVMJIT_MEMSPEC_ADDRESS_MODE: {
#ifdef CVMCPU_HAS_CP_REG
	    CVMassert(needMargin == CVM_FALSE);
	    /* relative to cp base register */
	    diff = absolute(toPC - con->target.cpLogicalPC);
	    return (diff <= CVMCPU_MAX_LOADSTORE_OFFSET);
#else
	    return CVM_TRUE;
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
    CVMUint32* instructionPtr = (CVMUint32*)
        CVMJITcbufLogicalToPhysical(con, instructionLogicalAddress);
    CVMUint32 instruction = *instructionPtr;
    CVMInt32 offsetBits = targetLogicalAddress - instructionLogicalAddress;

    CVMtraceJITCodegen((":::::Fixed instruction at %d to reference %d\n", 
            instructionLogicalAddress, targetLogicalAddress));

    if (!CVMJITcanReachAddress(con, instructionLogicalAddress,
                               targetLogicalAddress,
                               instructionAddressMode, CVM_FALSE)) {

#if 0
	/* This feature of doing long forward branchs to get around the 32k
	 * conditonal branch limit is disabled because it seems to cause
	 * problems with exception handlers, probably related to the extra
	 * instructions at the target block and MAP_PC nodes.
	 */
	if (instructionAddressMode != CVMJIT_COND_BRANCH_ADDRESS_MODE ||
	    CVMJITcbufGetLogicalPC(con) != targetLogicalAddress)
#endif
	{
	    /* Retry if inliningDepth != 0, otherwise fail. */
	    CVMJITlimitExceeded(con, "Can't reach address");
	}

#if 0
	/* See above comment for why this is disabled */
	/*
	 * We have an out of range forward conditional branch. The range
	 * is 32k. We can make use of an unconditional branch to handle this,
	 * since it has a much larger range:
	 *
	 * instructionLogicalAddress:    b targetLogicalAddress+4
	 * instructionLogicalAddress+4:
	 * ...
	 * targetLogicalAddress:         b targetLogicalAddress+12
	 * targetLogicalAddress+4:       b<cond> targetLogicalAddress+12
	 * targetLogicalAddress+8:       b instructionLogicalAddress+4
	 * targetLogicalAddress+12:      ; start of target block
	 *
	 * Note the branch instruction at targetLogicalAddress is there
	 * to handle fall through from the previous block, and also any
	 * other branches to this block.
	 */

	CVMassert(instructionAddressMode == CVMJIT_COND_BRANCH_ADDRESS_MODE);

	/*
	 * We only attempt this once for any given targetLogicalAddress, and
	 * only if targetLogicalAddress is the current logicalPC (which
	 * it seems to always if this is the first attempt. Doing otherwise
	 * is too hard to manage.
	 */
	CVMassert(CVMJITcbufGetLogicalPC(con) == targetLogicalAddress);

	CVMtraceJITCodegen((
	    ":::::Handling >32k forward conditional branch\n"));
	CVMconsolePrintf(
	    ":::::Handling >32k forward conditional branch\n");

	/* fixup the branch instruction: b targetLogicalAddress+4 */
	{
	    CVMJITcbufPushFixup(con, instructionLogicalAddress);
	    CVMPPCemitBranch(con, 
			     targetLogicalAddress + CVMCPU_INSTRUCTION_SIZE,
			     CVMCPU_COND_AL,
			     CVM_FALSE /*link */, CVM_FALSE,
			     NULL /* no fixupList */);
	    CVMJITcbufPop(con);
	}
	
	/* b targetLogicalAddress+12 */
	CVMPPCemitBranch(con, 
	    targetLogicalAddress + 3*CVMCPU_INSTRUCTION_SIZE,
	    CVMCPU_COND_AL,
	    CVM_FALSE /*link */, CVM_FALSE,
	    NULL /* no fixupList */);
	/* b<cond> targetLogicalAddress+12 */
	CVMPPCemitBranch(con, 
	    targetLogicalAddress + 3*CVMCPU_INSTRUCTION_SIZE,
	    CVMCPU_COND_EQ, /* dummy condition. fixup later.*/
	    CVM_FALSE /*link */, CVM_FALSE,
	    NULL /* no fixupList */);
	/* Fix the condition on the previous conditional branch */
	{
	    CVMUint32* prevInstPtr =
		(CVMUint32*)CVMJITcbufGetPhysicalPC(con)-1;
	    CVMUint16 condbits = (CVMUint16)(instruction >> 16);
	    *(CVMUint16*)prevInstPtr = condbits;
	}
	/* b instructionLogicalAddress+4 */
	CVMPPCemitBranch(con, 
	    instructionLogicalAddress + CVMCPU_INSTRUCTION_SIZE,
	    CVMCPU_COND_AL,
	    CVM_FALSE /*link */, CVM_FALSE,
	    NULL /* no fixupList */);
	return;
#endif /* disabled */
    }

    switch (instructionAddressMode) {
    case CVMJIT_BRANCH_ADDRESS_MODE:
	/* mask off lower 26 bits, but not last 2 */
        instruction &= ~0x03fffffc;
        break;
    case CVMJIT_COND_BRANCH_ADDRESS_MODE:
	/* mask off lower 16 bits, but not last 2 */
        instruction &= ~0x0000fffc;
        break;
    case CVMJIT_MEMSPEC_ADDRESS_MODE:
#ifdef CVMCPU_HAS_CP_REG
        /* offset is relative to cp base register */
        offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
#else
        CVMassert(CVM_FALSE); /* should never be called in this case */
#endif
	/* mask off lower 16 bits */
        instruction &= ~0x0000ffff; 
        break;
    default:
        CVMassert(CVM_FALSE);
        break;
    }
    instruction |= offsetBits;
    *instructionPtr = instruction;
}

#ifdef IAI_CACHEDCONSTANT  
/*
 * CVMJITemitLoadConstantAddress - Emit instruction(s) to load the address
 * of a constant pool constant into CVMCPU_ARG4_REG (r7).
 */
void
CVMJITemitLoadConstantAddress(CVMJITCompilationContext* con,
			      int targetLogicalAddress)
{
#ifdef CVMCPU_HAS_CP_REG
    CVMInt32 offsetBits;

    /* 
     * con->target.cpLogicalPC == 0 is a special case when we don't know
     * the target address yet, so just assume an offset of 0
     */
    if (con->target.cpLogicalPC == 0) {
	offsetBits = 0;
    } else {
	offsetBits = targetLogicalAddress - con->target.cpLogicalPC;
    }

    CVMassert(offsetBits >= 0);
    if (offsetBits > CVMCPU_MAX_LOADSTORE_OFFSET) {
        CVMJITerror(con, CANNOT_COMPILE, "Constant address out of range");
    }

    CVMJITaddCodegenComment((con, "load address of cached constant"));
    CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
				CVMCPU_ARG4_REG, CVMCPU_CP_REG,
				offsetBits, CVMJIT_NOSETCC);
#else
    CVMPPCemitMakeConstant32(con, CVMCPU_ARG4_REG,
	CVMJITcbufLogicalToPhysical(con, targetLogicalAddress));
#endif
}
#endif /* IAI_CACHEDCONSTANT */

#ifdef CVM_PPC_E500V1
#ifdef CVM_TRACE_JIT
/* Purpose: Returns the name of the specified opcode. */
static const char *getE500CompareOpcodeName(CVMUint32 e500opcode)
{
    const char *name = NULL;
    switch (e500opcode) {
      case PPC_E500_EFSCMPEQ_OPCODE: name = "efscmpeq"; break;
      case PPC_E500_EFSCMPLT_OPCODE: name = "efscmplt"; break;
      case PPC_E500_EFSCMPGT_OPCODE: name = "efscmpgt"; break;
      default:
	CVMconsolePrintf("Unknown opcode 0x%08x", e500opcode);
	CVMassert(CVM_FALSE); /* Unknown opcode name */
    }
    return name;  
}
#endif

/*
 * Purpose: Emit the proper opcode for a floating point test of the
 * given condition.
 */
void
CVME500emitFCompare(
    CVMJITCompilationContext* con,
    CVMCPUCondCode condCode,
    int lhsRegID, int rhsRegID)
{
    CVMUint32 e500opcode;

    switch (condCode) {
      case CVMCPU_COND_EQ:
	e500opcode = PPC_E500_EFSCMPEQ_OPCODE;
	break;
      case CVMCPU_COND_LT:
	e500opcode = PPC_E500_EFSCMPLT_OPCODE;
	break;
      case CVMCPU_COND_GT:
	e500opcode = PPC_E500_EFSCMPGT_OPCODE;
	break;
      default:
	CVMassert(CVM_FALSE);
	e500opcode = 0;
	break;
    }

    emitInstruction(con, e500opcode |
                    lhsRegID << PPC_RA_SHIFT | rhsRegID << PPC_RB_SHIFT);

    CVMtraceJITCodegenExec({
	    printPC(con);
	    CVMconsolePrintf("\t%s r%d, r%d",
			     getE500CompareOpcodeName(e500opcode),
			     lhsRegID, rhsRegID);
	});
    CVMJITdumpCodegenComments(con);
}
#endif
