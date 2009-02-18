/*
 * @(#)jitriscemitterdefs_cpu.h	1.19 06/10/10
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
 * considered private to the MIPS specific parts of the jit, including those
 * with the CVMMIPS_ prefix.
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

#define CVMCPU_COND_UNORDERED_LT	32
#define CVMMIPS_COND_FFIRST 16
    ,
    CVMCPU_COND_FEQ = 16, /* Do when equal */
    CVMCPU_COND_FNE = 17, /* Do when NOT equal */
    CVMCPU_COND_FLT = 18, /* Do when less than */
    CVMCPU_COND_FGT = 19, /* Do when U or greater than */
    CVMCPU_COND_FLE = 20, /* Do when less than or equal */
    CVMCPU_COND_FGE = 21  /* Do when U or greater than or equal */
};
typedef enum CVMCPUCondCode CVMCPUCondCode;

/**************************************************************
 * CPU Opcodes - The following are definition of opcode encodings
 * that are required by the RISC emitter porting layer.  Where
 * actual opcodes do not exists, pseudo opcodes are substituted.
 **************************************************************/

#define CVMCPU_NOP_INSTRUCTION (0x00000000) /* nop */

/* Memory Reference opcodes: */
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
    CVMCPU_ADD_OPCODE = 0, /* reg32 = reg32 + aluRhs32 */
    CVMCPU_SUB_OPCODE,     /* reg32 = reg32 - aluRhs32 */
    CVMCPU_AND_OPCODE,     /* reg32 = reg32 AND aluRhs32 */
    CVMCPU_OR_OPCODE,      /* reg32 = reg32 OR aluRhs. */
    CVMCPU_XOR_OPCODE,     /* reg32 = reg32 XOR aluRhs32 */
    CVMCPU_BIC_OPCODE,     /* reg32 = reg32 AND ~aluRhs32 */
    CVMMIPS_DIV_OPCODE,    /* reg32 = reg32 / reg32. */
    CVMMIPS_REM_OPCODE,    /* reg32 = reg32 % reg32. */
    
    /* 32 bit mov opcode */
    CVMCPU_MOV_OPCODE,     /* reg32 = aluRhs32. */
#ifdef CVM_JIT_USE_FP_HARDWARE
    /* Floating-point register-register move opcode: */
    /* must be in same space as CVMCPU_MOV_OPCODE */
    CVMCPU_FMOV_OPCODE,
#endif

    /* 32 bit unary ALU opcodes */
    CVMCPU_NEG_OPCODE,     /* reg32 = -reg32. */
    CVMCPU_NOT_OPCODE,     /* reg32 = (reg32 == 0)?1:0. */
    CVMCPU_INT2BIT_OPCODE, /* reg32 = (reg32 != 0)?1:0. */
    
    /* 32 bit shift opcodes */
    CVMCPU_SLL_OPCODE, /* Shift Left Logical */
    CVMCPU_SRL_OPCODE, /* Shift Right Logical */
    CVMCPU_SRA_OPCODE, /* Shift Right Arithmetic */
    
    /* 32 bit multiply opcodes */
    CVMCPU_MULL_OPCODE,	  /* reg32 = LO32(reg32 * reg32), signed */
    CVMCPU_MULH_OPCODE,   /* reg32 = HI32(reg32 * reg32), signed */
    CVMMIPS_MULTU_OPCODE, /* reg32 = LO32(reg32 * reg32), unsigned */
    CVMMIPS_MUL_OPCODE,   /* reg32 = reg32 * reg32 */
    
    /* 32 bit compare opcodes */
    CVMCPU_CMP_OPCODE, /* cmp reg32, aluRhs32 */
    CVMCPU_CMN_OPCODE, /* cmp reg32, -aluRhs32 */
    
    /* 64 bit unary ALU opcodes */
    CVMCPU_NEG64_OPCODE,
    
    /* 64 bit compare ALU opcodes */
    CVMCPU_CMP64_OPCODE,

    /* 64 bit binary ALU opcodes */
    CVMCPU_MUL64_OPCODE,
    CVMCPU_ADD64_OPCODE,
    CVMCPU_SUB64_OPCODE,
    CVMCPU_AND64_OPCODE,
    CVMCPU_OR64_OPCODE,
    CVMCPU_XOR64_OPCODE,

#ifdef CVM_JIT_USE_FP_HARDWARE
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
    CVMCPU_DCMP_OPCODE,

    /* MIPS specific */
    CVMMIPS_I2F_OPCODE,
    CVMMIPS_F2I_OPCODE,
    CVMMIPS_I2D_OPCODE,
    CVMMIPS_D2I_OPCODE,
    CVMMIPS_F2D_OPCODE,
    CVMMIPS_D2F_OPCODE,

    CVMMIPS_MFC1_OPCODE,
    CVMMIPS_MTC1_OPCODE
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
    ,
    CVMMIPS_DMFC1_OPCODE,
    CVMMIPS_DMTC1_OPCODE
#endif
#endif /* CVM_JIT_USE_FP_HARDWARE */
};

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the types for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* 
 * MIPS addressing modes:
 *     - register + register
 *     - register + immediate
 */

enum CVMMIPSALURhsType {
    CVMMIPS_ALURHS_REGISTER,
    CVMMIPS_ALURHS_CONSTANT      /* 16-bit constant */
};
typedef enum CVMMIPSALURhsType CVMMIPSALURhsType;

/*
 * We could use a union to make this more compact,
 * but at much loss of clarity.
 */
typedef struct CVMCPUALURhs CVMCPUALURhs;
struct CVMCPUALURhs {
    CVMMIPSALURhsType   type;
    CVMInt32            constValue;
    CVMRMResource*      r;
};

typedef CVMUint32 CVMCPUALURhsToken;

#define CVMCPUALURhsTokenConstZero \
    CVMMIPSalurhsEncodeConstantToken(0) /* rhs constant 0 */

/**************************************************************
 * CPU MemSpec and associated types - The following are definition
 * of the types for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* MIPS has only one load/store addressing mode: reg+offset. */
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
 * MIPS specific comparison associated types
 **************************************************************/
typedef struct CVMMIPSCompareContext CVMMIPSCompareContext;
struct CVMMIPSCompareContext {
    int               opcode;
    CVMCPUCondCode    condCode;
    int               lhsRegID;
    CVMCPUALURhsToken rhsToken;
};


/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/
#if CVMCPU_MAX_ARG_REGS != 8
typedef struct CVMCPUCallContext CVMCPUCallContext;
struct CVMCPUCallContext
{
    CVMRMResource *reservedRes;
};
#define CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION
#endif

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define MIPS_LW    (0x23 << 26)
#define CVMCPU_GCTRAP_INSTRUCTION \
    (MIPS_LW | CVMCPU_GC_REG <<  21 | \
     CVMCPU_GC_REG << 16)
#define CVMCPU_GCTRAP_INSTRUCTION_MASK ~0xffff
#endif

#endif /* _INCLUDED_JITRISCEMITTERDEFS_CPU_H */
