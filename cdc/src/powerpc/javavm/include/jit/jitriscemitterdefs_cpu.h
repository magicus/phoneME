/*
 * @(#)jitriscemitterdefs_cpu.h	1.24 06/10/10
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

#ifndef _INCLUDED_JITRISCEMITTERDEFS_CPU_H
#define _INCLUDED_JITRISCEMITTERDEFS_CPU_H

#include "javavm/include/jit/jitrisc_cpu.h"

/*
 * This file defines all of the emitter types and option definitions that
 * in a platform indendent way to be used by the common RISC jit back end.
 * The exported symbols are prefixed with CVMCPU_.  Other symbols should be
 * considered private to the PPC specific parts of the jit, including those
 * with the CVMPPC_ prefix.
 *
 * This file should be included at the top of jitriscemitter.h.
 */

/**************************************************************
 * CPU condition codes - The following are definition of the condition
 * codes that are required by the RISC emitter porting layer.
 **************************************************************/

enum CVMCPUCondCode {
    CVMCPU_COND_EQ = 0,     /* Do when equal */
    CVMCPU_COND_NE = 1,     /* Do when NOT equal */
    CVMCPU_COND_MI = 2,     /* Do when has minus / negative */
    CVMCPU_COND_PL = 3,     /* Do when has plus / positive or zero */
    CVMCPU_COND_OV = 4,     /* Do when overflowed */
    CVMCPU_COND_NO = 5,     /* Do when NOT overflowed */
    CVMCPU_COND_LT = 6,     /* Do when signed less than */
    CVMCPU_COND_GT = 7,     /* Do when signed greater than */
    CVMCPU_COND_LE = 8,     /* Do when signed less than or equal */
    CVMCPU_COND_GE = 9,     /* Do when signed greater than or equal */
    CVMCPU_COND_LO = 10,    /* Do when lower / unsigned less than */
    CVMCPU_COND_HI = 11,    /* Do when higher / unsigned greater than */
    CVMCPU_COND_LS = 12,    /* Do when lower or same / is unsigned <= */
    CVMCPU_COND_HS = 13,    /* Do when higher or same / is unsigned >= */
    CVMCPU_COND_AL = 14,    /* Do always */
    CVMCPU_COND_NV = 15     /* Do never */

#ifdef CVM_JIT_USE_FP_HARDWARE
    /*
     * Floating-point condition codes are necessary for using floating
     * point hardware. They are in the same set as int condition codes,
     * but easily distinguishable from them.
     *
     * Remember that by default unordered should be treated as
     * greater than. If the UNORDERED_LT flag is set, then unordered
     * is treated as less than.
     */
#define CVMCPU_COND_UNORDERED_LT	16
    ,
    CVMCPU_COND_FEQ = CVMCPU_COND_EQ, /* Do when equal */
    CVMCPU_COND_FNE = CVMCPU_COND_NE, /* Do when NOT equal */
    CVMCPU_COND_FLT = CVMCPU_COND_LT, /* Do when less than */
    CVMCPU_COND_FGT = CVMCPU_COND_GT, /* Do when U or greater than */
    CVMCPU_COND_FLE = CVMCPU_COND_LE, /* Do when less than or equal */
    CVMCPU_COND_FGE = CVMCPU_COND_GE /* Do when U or greater than or equal */
#endif /* CVM_JIT_USE_FP_HARDWARE */
};
typedef enum CVMCPUCondCode CVMCPUCondCode;

/**************************************************************
 * CPU Opcodes - The following are definition of opcode encodings
 * that are required by the RISC emitter porting layer.  Where
 * actual opcodes do not exists, pseudo opcodes are substituted.
 **************************************************************/

#define CVMCPU_NOP_INSTRUCTION (24u << 26) /* nop */

/* Memory Reference opcodes */
enum {
    CVMCPU_LDR8U_OPCODE = 0, /* Load unsigned 8 bit value. */
    CVMCPU_LDR8_OPCODE,      /* Load signed 8 bit value. */
    CVMCPU_STR8_OPCODE,      /* Store 8 bit value. */
    CVMCPU_LDR16U_OPCODE,    /* Load unsigned 16 bit value. */
    CVMCPU_LDR16_OPCODE,     /* Load signed 16 bit value. */
    CVMCPU_STR16_OPCODE,     /* Store 16 bit value. */
    CVMCPU_LDR32_OPCODE,     /* Load signed 32 bit value. */
    CVMCPU_STR32_OPCODE,     /* Store 32 bit value. */
    CVMCPU_LDR64_OPCODE,     /* Load signed 64 bit value. */
    CVMCPU_STR64_OPCODE,     /* Store 64 bit value. */

    /* loads and stores to floating-point registers */
    CVMCPU_FLDR32_OPCODE,     /* Load float 32 bit value. */
    CVMCPU_FSTR32_OPCODE,     /* Store float 32 bit value. */
    CVMCPU_FLDR64_OPCODE,     /* Load float 64 bit value. */
    CVMCPU_FSTR64_OPCODE      /* Store float 64 bit value. */
};

enum {
    /* 32 bit binary ALU opcodes */
    /* NOTE: the binary ALU opcodes are used to index ppcBinaryALUOpcodes[],
       so they must start at 0 and be contiguous. */
    CVMCPU_ADD_OPCODE = 0, /* reg32 = reg32 + aluRhs32 */
    CVMPPC_ADDIS_OPCODE,   /* reg32 = reg32 + uint16 << 16 */
    CVMPPC_ADDE_OPCODE,    /* reg32 = reg32 + aluRhs32 + C */
    CVMCPU_SUB_OPCODE,     /* reg32 = reg32 - aluRhs32 */
    CVMPPC_SUBE_OPCODE,    /* reg32 = reg32 - aluRhs32 + C */
    CVMPPC_SUBFIC_OPCODE,  /* reg32 = aluRhs32 - reg32 */
    CVMCPU_AND_OPCODE,     /* reg32 = reg32 AND aluRhs32 */
    CVMPPC_ANDIS_OPCODE,   /* reg32 = reg32 AND uint16 << 16 */
    CVMCPU_OR_OPCODE,      /* reg32 = reg32 OR aluRhs. */
    CVMPPC_ORIS_OPCODE,    /* reg32 = reg32 OR uint16 << 16 */
    CVMCPU_XOR_OPCODE,     /* reg32 = reg32 XOR aluRhs32 */
    CVMPPC_XORIS_OPCODE,   /* reg32 = reg32 XOR uint16 << 16 */
#ifdef CVM_PPC_E500V1
    /* e500 floating point opcodes.  These operate on GPRs and are
       handled as 32 bit binary ALU opcodes. */
    CVME500_FADD_OPCODE, /* reg32 = reg32 + reg32 */
    CVME500_FSUB_OPCODE, /* reg32 = reg32 - reg32 */
    CVME500_FMUL_OPCODE, /* reg32 = reg32 * reg32 */
    CVME500_FDIV_OPCODE, /* reg32 = reg32 / reg32 */
#endif
    CVMPPC_RLWINM_OPCODE,  /* used for BIC like operations */
    CVMCPU_BIC_OPCODE,     /* reg32 = reg32 AND ~aluRhs32 */
    CVMPPC_DIV_OPCODE,     /* reg32 = reg32 % reg32. */
    CVMPPC_REM_OPCODE,     /* reg32 = reg32 % reg32. */
    
    /* 32 bit mov opcode */
    CVMCPU_MOV_OPCODE,      /* reg32 = aluRhs32. */

    /* Floating-point register-register move opcode: */
    /* must be in same space as CVMCPU_MOV_OPCODE */
    CVMCPU_FMOV_OPCODE,

    /* 32 bit unary ALU opcodes */
    CVMCPU_NEG_OPCODE,    /* reg32 = -reg32. */
    CVMCPU_NOT_OPCODE,     /* reg32 = (reg32 == 0)?1:0. */
    CVMCPU_INT2BIT_OPCODE, /* reg32 = (reg32 != 0)?1:0. */

#ifdef CVM_PPC_E500V1
    CVME500_FNEG_OPCODE,  /* reg32 = -reg32. */
    CVME500_I2F_OPCODE,   /* reg32 = (float) reg32. */
    CVME500_F2I_OPCODE,   /* reg32 = (int) reg32. */
#endif
    
    /* 32 bit shift opcodes */
    CVMCPU_SLL_OPCODE, /* Shift Left Logical */
    CVMCPU_SRL_OPCODE, /* Shift Right Logical */
    CVMCPU_SRA_OPCODE, /* Shift Right Arithmetic */
    
    /* 32 bit multiply opcodes */
    CVMCPU_MULL_OPCODE,	 /* reg32 = LO32(reg32 * reg32) */
    CVMCPU_MULH_OPCODE,  /* reg32 = HI32(reg32 * reg32) */
    CVMPPC_MULHU_OPCODE, /* reg32 = HI32(reg32 * reg32) unsigned */
    
    /* 32 bit compare opcodes */
    CVMCPU_CMP_OPCODE, /* cmp reg32, aluRhs32 */
    CVMCPU_CMN_OPCODE, /* cmp reg32, -aluRhs32 */
    
    /* 64 bit unary ALU opcodes */
    CVMCPU_NEG64_OPCODE,
    
    /* 64 bit compare ALU opcodes */
    CVMCPU_CMP64_OPCODE,

    /* 64 bit binary ALU opcodes
     * NOTE: Some ALU64 opcodes are actually encoded in terms of 2 32 bit 
     *       opcodes and a "setcc" boolean:
     *           ((lowOpcode << 16) | (highOpcode << 8) | setCCForLowOpcode)
     */
    CVMCPU_MUL64_OPCODE,
    CVMCPU_ADD64_OPCODE = /* ADD,ADDE,setcc */
        CVMCPU_ADD_OPCODE << 16 | CVMPPC_ADDE_OPCODE << 8 | CVMJIT_SETCC,
    CVMCPU_SUB64_OPCODE = /* SUB,SUBE,setcc */
        CVMCPU_SUB_OPCODE << 16 | CVMPPC_SUBE_OPCODE << 8 | CVMJIT_SETCC,
    CVMCPU_AND64_OPCODE = /* AND,AND,~setcc */
        CVMCPU_AND_OPCODE << 16 | CVMCPU_AND_OPCODE  << 8 | CVMJIT_NOSETCC,
    CVMCPU_OR64_OPCODE  = /* OR,OR,~setcc */
        CVMCPU_OR_OPCODE  << 16 | CVMCPU_OR_OPCODE   << 8 | CVMJIT_NOSETCC,
    CVMCPU_XOR64_OPCODE = /* XOR,XOR,~setcc */
        CVMCPU_XOR_OPCODE << 16 | CVMCPU_XOR_OPCODE  << 8 | CVMJIT_NOSETCC
};

/* Floating-point ALU ops */
enum {
    /* translation of CVMCPU_FMOV_OPCODE to this space */
    CVMPPC_FMOV_OPCODE,

    /* Floating-point opcodes, single precision: */
    CVMCPU_FNEG_OPCODE,
    CVMCPU_FADD_OPCODE,
    CVMCPU_FSUB_OPCODE,
    CVMCPU_FMUL_OPCODE,
    CVMCPU_FDIV_OPCODE,
    CVMCPU_FCMP_OPCODE,

    /* Floating-point opcodes, double precision: */
    CVMCPU_DNEG_OPCODE,
    CVMCPU_DADD_OPCODE,
    CVMCPU_DSUB_OPCODE,
    CVMCPU_DMUL_OPCODE,
    CVMCPU_DDIV_OPCODE,
    CVMCPU_DCMP_OPCODE

    /*
     * WARNING: using fmadd results in a non-compliant vm. Some floating
     * point tck tests will fail. Therefore, this support if off by defualt.
     */
#ifdef CVM_JIT_USE_FMADD
    ,
    /* Multiply-Add opcodes: CVMPPC_FMADD_OPCODE must be first! */
    CVMPPC_FMADD_OPCODE,
    CVMPPC_DMADD_OPCODE,
    CVMPPC_FMSUB_OPCODE,
    CVMPPC_DMSUB_OPCODE,
    CVMPPC_FNMADD_OPCODE,
    CVMPPC_DNMADD_OPCODE,
    CVMPPC_FNMSUB_OPCODE,
    CVMPPC_DNMSUB_OPCODE
#endif
};

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the types for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* 
 * PPC addressing modes:
 *     - register + register
 *     - register + immediate
 */

enum CVMPPCALURhsType {
    CVMPPC_ALURHS_REGISTER,
    CVMPPC_ALURHS_CONSTANT      /* 16-bit constant */
};
typedef enum CVMPPCALURhsType CVMPPCALURhsType;

/*
 * We could use a union to make this more compact,
 * but at much loss of clarity.
 */
typedef struct CVMCPUALURhs CVMCPUALURhs;
struct CVMCPUALURhs {
    CVMPPCALURhsType    type;
    CVMInt32            constValue;
    CVMRMResource*      r;
};

typedef CVMUint32 CVMCPUALURhsToken;

#define CVMCPUALURhsTokenConstZero \
    CVMPPCalurhsEncodeConstantToken(0) /* rhs constant 0 */

/**************************************************************
 * CPU MemSpec and associated types - The following are definition
 * of the types for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

enum CVMCPUMemSpecType {
    CVMCPU_MEMSPEC_REG_OFFSET = 0,
    CVMCPU_MEMSPEC_IMMEDIATE_OFFSET,
    CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET,
    CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET
};
typedef enum CVMCPUMemSpecType CVMCPUMemSpecType;

/* Class: CVMCPUMemSpec
   Purpose: Encapsulates the parameters for encoding a memory access
            specification for the use of a memory reference instruction.
*/
typedef struct CVMCPUMemSpec CVMCPUMemSpec;
struct CVMCPUMemSpec {
    CVMCPUMemSpecType type;

    /* Address mode: CVMCPU_MEMSPEC_IMMEDIATE_OFFSET: */
    CVMInt32       offsetValue;

    /* Address mode: CVMCPU_MEMSPEC_REG_OFFSET: */
    CVMRMResource* offsetReg;
};

typedef CVMUint32 CVMCPUMemSpecToken;


/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#undef CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION

#include "javavm/include/jit/jitriscemitterdefs_arch.h"

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define PPC_OPCODE_SHIFT 26u  /* opcode in upper 6 bits */
#define PPC_LWZ_OPCODE   (32u << PPC_OPCODE_SHIFT)
#define CVMCPU_GCTRAP_INSTRUCTION \
    (PPC_LWZ_OPCODE | CVMCPU_GC_REG <<  21 | \
     CVMCPU_GC_REG << 16)
#define CVMCPU_GCTRAP_INSTRUCTION_MASK ~0xffff
#endif

#endif /* _INCLUDED_JITRISCEMITTERDEFS_CPU_H */
