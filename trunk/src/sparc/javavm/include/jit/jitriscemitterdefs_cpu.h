/*
 * @(#)jitriscemitterdefs_cpu.h	1.30 06/10/13
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

#ifndef _INCLUDED_SPARC_JITRISCEMITTERDEFS_CPU_H
#define _INCLUDED_SPARC_JITRISCEMITTERDEFS_CPU_H

#include "javavm/include/jit/jitrisc_cpu.h"

/*
 * This file defines all of the emitter types and option definitions that
 * in a platform indendent way to be used by the common RISC jit back end.
 * The exported symbols are prefixed with CVMCPU_.  Other symbols should be
 * considered private to the SPARC specific parts of the jit, including those
 * with the CVMSPARC_ prefix.
 *
 * This file should be included at the top of jitriscemitter.h.
 */

/**************************************************************
 * CPU condition codes - The following are definition of the condition
 * codes that are required by the RISC emitter porting layer.
 **************************************************************/

enum CVMCPUCondCode {
    CVMCPU_COND_EQ = 1,     /* Do when equal */
    CVMCPU_COND_NE = 9,     /* Do when NOT equal */
    CVMCPU_COND_MI = 6,     /* Do when has minus / negative */
    CVMCPU_COND_PL = 14,    /* Do when has plus / positive or zero */
    CVMCPU_COND_OV = 7,     /* Do when overflowed */
    CVMCPU_COND_NO = 15,    /* Do when NOT overflowed */
    CVMCPU_COND_LT = 3,     /* Do when signed less than */
    CVMCPU_COND_GT = 10,    /* Do when signed greater than */
    CVMCPU_COND_LE = 2,     /* Do when signed less than or equal */
    CVMCPU_COND_GE = 11,    /* Do when signed greater than or equal */
    CVMCPU_COND_LO = 5,     /* Do when lower / unsigned less than */
    CVMCPU_COND_HI = 12,    /* Do when higher / unsigned greater than */
    CVMCPU_COND_LS = 4,     /* Do when lower or same / is unsigned <= */
    CVMCPU_COND_HS = 13,    /* Do when higher or same / is unsigned >= */
    CVMCPU_COND_AL = 8,     /* Do always */
    CVMCPU_COND_NV = 0      /* Do never */

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
#define CVMCPU_COND_UNORDERED_LT	32
#define CVMSPARC_COND_FFIRST 16
    ,
    CVMCPU_COND_FEQ = CVMSPARC_COND_FFIRST+0, /* Do when equal */
    CVMCPU_COND_FNE = CVMSPARC_COND_FFIRST+1, /* Do when NOT equal */
    CVMCPU_COND_FLT = CVMSPARC_COND_FFIRST+2, /* Do when less than */
    CVMCPU_COND_FGT = CVMSPARC_COND_FFIRST+3, /* Do when U or greater than */
    CVMCPU_COND_FLE = CVMSPARC_COND_FFIRST+4, /* Do when less than or equal */
    CVMCPU_COND_FGE = CVMSPARC_COND_FFIRST+5  /* Do when U or greater than or equal */
#endif /* CVM_JIT_USE_FP_HARDWARE */
};
typedef enum CVMCPUCondCode CVMCPUCondCode;

/**************************************************************
 * CPU Opcodes - The following are definition of opcode encodings
 * that are required by the RISC emitter porting layer.  Where
 * actual opcodes do not exists, pseudo opcodes are substituted.
 **************************************************************/

#define CVMCPU_NOP_INSTRUCTION (0x1000000) /* nop */

/* Memory Reference opcodes: */
#define CVMSPARC_LDD_OPCODE	(0x03 << 19) /* Load 64 bit value. */
#define CVMSPARC_STD_OPCODE	(0x07 << 19) /* Store 64 bit value. */

#define CVMCPU_LDR64_OPCODE     CVMSPARC_LDD_OPCODE /* Load 64 bit value. */
#define CVMCPU_STR64_OPCODE     CVMSPARC_STD_OPCODE /* Store 64 bit value. */
#define CVMCPU_LDR32_OPCODE     (0x00 << 19) /* Load 32 bit value. */
#define CVMCPU_STR32_OPCODE     (0x04 << 19) /* Store 32 bit value. */
#define CVMCPU_LDR16U_OPCODE    (0x02 << 19) /* Load unsigned 16 bit value. */
#define CVMCPU_LDR16_OPCODE     (0x0a << 19) /* Load signed 16 bit value. */
#define CVMCPU_STR16_OPCODE     (0x06 << 19) /* Store 16 bit value. */
#define CVMCPU_LDR8U_OPCODE     (0x01 << 19) /* Load unsigned 8 bit value. */
#define CVMCPU_LDR8_OPCODE      (0x09 << 19) /* Load signed 8 bit value. */
#define CVMCPU_STR8_OPCODE      (0x05 << 19) /* Store 8 bit value. */

/* Floating Point Mememory reference opcodes: */
#define CVMCPU_FLDR32_OPCODE	(0x20 << 19) /* Load float 32 bit value */
#define CVMCPU_FLDR64_OPCODE	(0x23 << 19) /* Load float 64 bit value */
#define CVMCPU_FSTR32_OPCODE	(0x24 << 19) /* Store float 32 bit value */
#define CVMCPU_FSTR64_OPCODE	(0x27 << 19) /* Store float 64 bit value */

/* 32 bit ALU opcodes: */
#define CVMSPARC_ANDN_OPCODE	(0x05 << 19) /* reg32 = reg32 AND ~aluRhs32. */

#define CVMCPU_ADD_OPCODE   (0 << 19) /* reg32 = reg32 + aluRhs32. */
#define CVMCPU_SUB_OPCODE   (0x04 << 19) /* reg32 = reg32 - aluRhs32. */
#define CVMCPU_AND_OPCODE   (0x01 << 19) /* reg32 = reg32 AND aluRhs32. */
#define CVMCPU_OR_OPCODE    (0x02 << 19) /* reg32 = reg32 OR aluRhs32. */
#define CVMCPU_XOR_OPCODE   (0x03 << 19) /* reg32 = reg32 XOR aluRhs32. */
#define CVMCPU_BIC_OPCODE   CVMSPARC_ANDN_OPCODE /*reg32 = reg32 AND ~aluRhs32*/
#define CVMCPU_MOV_OPCODE     (0x00000001) /* reg32 = aluRhs32. */
#define CVMCPU_NEG_OPCODE     (0x00000002) /* reg32 = -reg32. */
#define CVMCPU_NOT_OPCODE     (0x0000000b) /* reg32 = (reg32 == 0)?1:0. */
#define CVMCPU_INT2BIT_OPCODE (0x0000000c) /* reg32 = (reg32 != 0)?1:0. */

#define CVMCPU_SLL_OPCODE       (0x25 << 19) /* Shift Left Logical */
#define CVMCPU_SRL_OPCODE       (0x26 << 19) /* Shift Right Logical */
#define CVMCPU_SRA_OPCODE       (0x27 << 19) /* Shift Right Arithmetic */

#define CVMCPU_MULL_OPCODE  (0x00000003) /* reg32 = LO32(reg32 * reg32). */
#define CVMCPU_MULH_OPCODE  (0x00000004) /* reg32 = HI32(reg32 * reg32). */
/* These are used by overriding the IDIV32 and IREM32 rules */
#define CVMSPARC_DIV_OPCODE   (0x0f << 19) 
#define CVMSPARC_REM_OPCODE   (0x00000005) 
#define CVMCPU_CMP_OPCODE   CVMCPU_SUB_OPCODE /* cmp reg32, aluRhs32 => set cc*/
#define CVMCPU_CMN_OPCODE   CVMCPU_ADD_OPCODE /* cmp reg32, -aluRhs32 => set cc*/

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
/* Atomic swap opcode */
#define CVMSPARC_SWAP_OPCODE  ((0x3u << 30) | (0x0f << 19) | (1 << 13))
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
/* Atomic cas opcode */
#define CVMSPARC_CAS_OPCODE  ((0x3u << 30) | (0x3c << 19) | (0x80 << 5))
#endif
#endif

/* 32-bit floating point opcodes: */
#define CVMCPU_FADD_OPCODE    ((0x34 << 19) | (0x41 << 5))
#define CVMCPU_FSUB_OPCODE    ((0x34 << 19) | (0x45 << 5))
#define CVMCPU_FMUL_OPCODE    ((0x34 << 19) | (0x49 << 5))
#define CVMCPU_FDIV_OPCODE    ((0x34 << 19) | (0x4d << 5))
#define CVMCPU_FCMP_OPCODE    ((0x35 << 19) | (0x51 << 5))
#define CVMCPU_FNEG_OPCODE    ((0x34 << 19) | (0x05 << 5))
#define CVMCPU_FMOV_OPCODE    ((0x34 << 19) | (0x01 << 5))

/* 64-bit floating point opcodes: */
#define CVMCPU_DADD_OPCODE    ((0x34 << 19) | (0x42 << 5))
#define CVMCPU_DSUB_OPCODE    ((0x34 << 19) | (0x46 << 5))
#define CVMCPU_DMUL_OPCODE    ((0x34 << 19) | (0x4a << 5))
#define CVMCPU_DDIV_OPCODE    ((0x34 << 19) | (0x4e << 5))
#define CVMCPU_DCMP_OPCODE    ((0x35 << 19) | (0x52 << 5))
#define CVMCPU_DNEG_OPCODE    ((0x34 << 19) | (0x205<< 5))

/* Floating-point conversion opcodes: */
/* Used only within SPARC-specific code generation */
#define CVMSPARC_I2F_OPCODE     ((0x34 << 19) | (0xc4 << 5))
#define CVMSPARC_F2I_OPCODE     ((0x34 << 19) | (0xd1 << 5))
#define CVMSPARC_I2D_OPCODE     ((0x34 << 19) | (0xc8 << 5))
#define CVMSPARC_D2I_OPCODE     ((0x34 << 19) | (0xd2 << 5))
#define CVMSPARC_F2D_OPCODE     ((0x34 << 19) | (0xc9 << 5))
#define CVMSPARC_D2F_OPCODE     ((0x34 << 19) | (0xc6 << 5))

/* 64 bit ALU opcodes:
 * NOTE: The ALU64 opcodes are actually encoded in terms of 2 32 bit SPARC 
 *       opcodes and a boolean:
 *           ((lowOpcode << 16) | (highOpcode << 8) | setCCForLowOpcode)
 */
#define CVMCPU_NEG64_OPCODE (0x00000006)
#define CVMCPU_ADD64_OPCODE ((0 << 16)|(0x08 << 8)|1) /* ADD,ADX,true */
#define CVMCPU_SUB64_OPCODE ((0x04 << 16)|(0x0c << 8)|1) /* SUB,SUBX,true */
#define CVMCPU_AND64_OPCODE ((0x01 << 16)|(0x01 << 8)|0) /* AND,AND,false */
#define CVMCPU_OR64_OPCODE  ((0x02 << 16)|(0x02 << 8)|0) /* OR,OR,false */
#define CVMCPU_XOR64_OPCODE ((0x03 << 16)|(0x03 << 8)|0) /* XOR,XOR,false */

/* NOTE: DIV64 and REM64 opcodes are not used, but are kept here for
 * possible future use. */
#define CVMCPU_MUL64_OPCODE (0x00000007)
#define CVMCPU_DIV64_OPCODE (0x00000008)
#define CVMCPU_REM64_OPCODE (0x00000009)
#define CVMCPU_CMP64_OPCODE (0x0000000a)

/* Branch opcodes. Used only within SPARC layer */
#define CVMSPARC_BRANCH_OPCODE  (2<<22)
#define CVMSPARC_FBRANCH_OPCODE (6<<22)

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the types for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* 
 * SPARC addressing modes:
 *     - register + register
 *     - register + immediate
 */

enum CVMSPARCALURhsType {
    CVMSPARC_ALURHS_REGISTER,
    CVMSPARC_ALURHS_CONSTANT
};
typedef enum CVMSPARCALURhsType CVMSPARCALURhsType;

/*
 * We could use a union to make this more compact,
 * but at much loss of clarity.
 */
typedef struct CVMCPUALURhs CVMCPUALURhs;
struct CVMCPUALURhs {
    CVMSPARCALURhsType  type;
    CVMInt32            constValue;
    CVMRMResource*      r;
};

typedef CVMUint32 CVMCPUALURhsToken;

/* SPARC specific ALURhs encoding bits: */
#define CVMSPARC_MODE_CONSTANT (1 << 13)
#define CVMSPARC_MODE_REGISTER (0 << 13)

#define CVMCPUALURhsTokenConstZero  (1 << 13) /* rhs constant 0 */

/**************************************************************
 * CPU MemSpec and associated types - The following are definition
 * of the types for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

enum CVMCPUMemSpecType {
    CVMCPU_MEMSPEC_REG_OFFSET = 0,
    CVMCPU_MEMSPEC_IMMEDIATE_OFFSET = 1,
    /* NOTE: SPARC does not support post-increment and pre-decrement
       memory access. This is to mimic ARM hehavior. Extra add/sub
       instruction is emitted in order to achieve it. */ 
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
    CVMRMResource *offsetReg;
};

typedef CVMUint32 CVMCPUMemSpecToken;


/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#undef CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GCTRAP_INSTRUCTION \
    (3U << 30 | CVMCPU_LDR32_OPCODE | CVMCPU_GC_REG <<  25 | \
     CVMCPU_GC_REG << 14)
#define CVMCPU_GCTRAP_INSTRUCTION_MASK ~0x3fff
#endif

#endif /* _INCLUDED_SPARC_JITRISCEMITTERDEFS_CPU_H */
