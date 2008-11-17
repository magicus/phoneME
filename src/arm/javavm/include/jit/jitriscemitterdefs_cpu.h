/*
 * @(#)jitriscemitterdefs_cpu.h	1.27 06/10/10
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

#ifndef _INCLUDED_ARM_JITRISCEMITTERDEFS_CPU_H
#define _INCLUDED_ARM_JITRISCEMITTERDEFS_CPU_H

#include "javavm/include/jit/jitrisc_cpu.h"

/*
 * This file defines all of the emitter types and option definitions that
 * in a platform indendent way to be used by the common RISC jit back end.
 * The exported symbols are prefixed with CVMCPU_.  Other symbols should be
 * considered private to the ARM specific parts of the jit, including those
 * with the CVMARM_ prefix.
 *
 * This file should be included at the top of jitriscemitter.h.
 */

/**************************************************************
 * CPU condition codes - The following are definition of the condition
 * codes that are required by the RISC emitter porting layer.
 **************************************************************/

typedef enum CVMCPUCondCode {
    CVMCPU_COND_EQ = 0,     /* Do when equal */
    CVMCPU_COND_NE = 1,     /* Do when NOT equal */
    CVMCPU_COND_MI = 4,     /* Do when has minus / negative */
    CVMCPU_COND_PL = 5,     /* Do when has plus / positive or zero */
    CVMCPU_COND_OV = 6,     /* Do when overflowed */
    CVMCPU_COND_NO = 7,     /* Do when NOT overflowed */
    CVMCPU_COND_LT = 11,    /* Do when signed less than */
    CVMCPU_COND_GT = 12,    /* Do when signed greater than */
    CVMCPU_COND_LE = 13,    /* Do when signed less than or equal */
    CVMCPU_COND_GE = 10,    /* Do when signed greater than or equal */
    CVMCPU_COND_LO = 3,     /* Do when lower / unsigned less than */
    CVMCPU_COND_HI = 8,     /* Do when higher / unsigned greater than */
    CVMCPU_COND_LS = 9,     /* Do when lower or same / is unsigned <= */
    CVMCPU_COND_HS = 2,     /* Do when higher or same / is unsigned >= */
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
#define CVMCPU_COND_UNORDERED_LT	32
    ,
    CVMCPU_COND_FEQ = CVMCPU_COND_EQ, /* Do when equal */
    CVMCPU_COND_FNE = CVMCPU_COND_NE, /* Do when NOT equal */
    CVMCPU_COND_FLT = CVMCPU_COND_LT, /* Do when less than */
    CVMCPU_COND_FGT = CVMCPU_COND_GT, /* Do when greater than */
    CVMCPU_COND_FLE = CVMCPU_COND_LE, /* Do when less than or equal */
    CVMCPU_COND_FGE = CVMCPU_COND_GE  /* Do when greater than or equal */
#endif /* CVM_JIT_USE_FP_HARDWARE */
    
} CVMCPUCondCode;

/**************************************************************
 * CPU Opcodes - The following are definition of opcode encodings
 * that are required by the RISC emitter porting layer.  Where
 * actual opcodes do not exists, pseudo opcodes are substituted.
 **************************************************************/

#define CVMCPU_NOP_INSTRUCTION     0xe1a00000  /* mov r0, r0 */

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
#define CVMCPU_NOP2_INSTRUCTION    0xe1a01001  /* mov r1, r1 */
#define CVMCPU_NOP3_INSTRUCTION    0xe1a02002  /* mov r2, r2 */
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

/* Memory Reference opcodes: */
#define CVMCPU_LDR64_OPCODE     0x00000000  /* Load signed 64 bit value. */
#define CVMCPU_STR64_OPCODE     0x00000001  /* Store 64 bit value. */
#define CVMCPU_LDR32_OPCODE     0x04100000  /* Load signed 32 bit value. */
#define CVMCPU_STR32_OPCODE     0x04000000  /* Store 32 bit value. */
#define CVMCPU_STR8_OPCODE      0x04400000  /* Store 8 bit value. */

/* The following are ARMv4 only, and use addressing mode 3 */
#define CVMCPU_LDR16U_OPCODE    0x005000b0  /* Load unsigned 16 bit value. */
#define CVMCPU_LDR16_OPCODE     0x005000f0  /* Load signed 16 bit value. */
#define CVMCPU_STR16_OPCODE     0x004000b0  /* Store 16 bit value. */
#define CVMCPU_LDR8U_OPCODE     0x04500000  /* Load unsigned 8 bit value. */
#define CVMCPU_LDR8_OPCODE      0x005000d0  /* Load signed 8 bit value. */

/* Floating Point Mememory reference opcodes using addressing mode 5 */
#define CVMCPU_FLDR32_OPCODE 0x0d100a00 /* Load float 32 bit value */
#define CVMCPU_FLDR64_OPCODE 0x0d100b00 /* Load float 64 bit value */
#define CVMCPU_FSTR32_OPCODE 0x0d000a00 /* Store float 32 bit value */
#define CVMCPU_FSTR64_OPCODE 0x0d000b00 /* Store float 64 bit value */

/* Floating-point Memory reference opcodes: */
/* Used only within ARM-specific code generation */
#define CVMARM_FLDRM32_OPCODE 0x0c100a00 /* Load multiple float 32 bit value */
#define CVMARM_FLDRM64_OPCODE 0x0c100b00 /* Load multiple float 64 bit value */
#define CVMARM_FSTRM32_OPCODE 0x0c000a00 /* Store multiple float 32 bit value */
#define CVMARM_FSTRM64_OPCODE 0x0c000b00 /* Store multiple float 64 bit value */


/* 32 bit ALU opcodes: */
#define CVMCPU_MOV_OPCODE   (0x1a << 20) /* reg32 = aluRhs32. */
#define CVMCPU_NEG_OPCODE   0x00000003   /* reg32 = -reg32. */
#define CVMCPU_NOT_OPCODE   0x00000004   /* reg32 = (reg32 == 0)?1:0. */
#define CVMCPU_INT2BIT_OPCODE 0x00000005 /* reg32 = (reg32 != 0)?1:0. */
#define CVMCPU_ADD_OPCODE   (0x08 << 20) /* reg32 = reg32 + aluRhs32. */
#define CVMCPU_SUB_OPCODE   (0x04 << 20) /* reg32 = reg32 - aluRhs32. */
#define CVMCPU_AND_OPCODE   (0x00 << 20) /* reg32 = reg32 AND aluRhs32. */
#define CVMCPU_OR_OPCODE    (0x18 << 20) /* reg32 = reg32 OR aluRhs32. */
#define CVMCPU_XOR_OPCODE   (0x02 << 20) /* reg32 = reg32 XOR aluRhs32. */
#define CVMCPU_BIC_OPCODE   (0x1c << 20) /* reg32 = reg32 AND ~aluRhs32. */
#define CVMARM_RSB_OPCODE   (0x06 << 20) /* reg32 = aluRhs32 - reg32. */

/* Mode 1 and 2 shift ops: */
#define CVMCPU_SLL_OPCODE       (0 << 5)
#define CVMCPU_SRL_OPCODE       (1 << 5)
#define CVMCPU_SRA_OPCODE       (2 << 5)
#define CVMARM_NOSHIFT_OPCODE   CVMCPU_SLL_OPCODE

#define CVMARM_MLA_OPCODE   0x00200090   /* reg32 = reg32 + (reg32 * reg32) */
#define CVMCPU_MULL_OPCODE  0x00000090   /* reg32 = LO32(reg32 * reg32). */
#define CVMCPU_MULH_OPCODE  0x00C00090   /* reg32 = HI32(reg32 * reg32). */
#define CVMCPU_CMP_OPCODE   (0x15 << 20) /* cmp reg32, aluRhs32 => set cc. */
#define CVMCPU_CMN_OPCODE   (0x17 << 20) /* cmp reg32, -aluRhs32 => set cc. */

#ifdef CVMJIT_SIMPLE_SYNC_METHODS
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
/* Atomic swap opcode */
#define CVMARM_SWP_OPCODE  0x01000090
#endif
#endif

/* 32-bit floating point opcodes: */
#define CVMCPU_FADD_OPCODE ((0x1c << 23) | (0x3 << 20) | (0xa << 8) \
                          | (0x0 << 6))
#define CVMCPU_FSUB_OPCODE ((0x1c << 23) | (0x3 << 20) | (0xa << 8) \
                          | (0x1 << 6))
#define CVMCPU_FMUL_OPCODE ((0x1c << 23) | (0x2 << 20) | (0xa << 8) \
                          | (0x0 << 6))
#define CVMCPU_FDIV_OPCODE ((0x1d << 23) | (0x0 << 20) | (0xa << 8) \
                          | (0x0 << 6))
#define CVMCPU_FCMP_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x4 << 16) \
                          | (0xa << 8) | (0x0 << 7) | (0x1 << 6))
#define CVMCPU_FNEG_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x1 << 16) \
                          | (0xa << 8) | (0x0 << 7) | (0x1 << 6))
/* Floating-point opcode to move within floating point registers: */
#define CVMCPU_FMOV_OPCODE ((0x1d << 23) | (0x3 << 20) | (0xa << 8) \
                          | (0x0 << 7) | (0x1 << 6)) /* FCPYS */

/* 64-bit floating point opcodes: */
#define CVMCPU_DADD_OPCODE ((0x1c << 23) | (0x3 << 20) | (0xb << 8) \
                          | (0x0 << 6))
#define CVMCPU_DSUB_OPCODE ((0x1c << 23) | (0x3 << 20) | (0xb << 8) \
                          | (0x1 << 6))   
#define CVMCPU_DMUL_OPCODE ((0x1c << 23) | (0x2 << 20) | (0xb << 8) \
                          | (0x0 << 6))   
#define CVMCPU_DDIV_OPCODE ((0x1d << 23) | (0x0 << 20) | (0xb << 8) \
                          | (0x0 << 6))   
#define CVMCPU_DCMP_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x4 << 16) \
                          | (0xb << 8) | (0x0 << 7) | (0x1 << 6))
#define CVMCPU_DNEG_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x1 << 16) \
                          | (0xb << 8) | (0x0 << 7) | (0x1 << 6))
/* Floating-point opcode to move within floating point registers: */
#define CVMCPU_DMOV_OPCODE ((0x1d << 23) | (0x3 << 20) | (0xb << 8) \
                          | (0x0 << 7) | (0x1 << 6)) /* FCPYD */


/* Floating-point conversion opcodes: */
/* Used only within ARM-specific code generation */
#define CVMARM_I2F_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x8 << 16) \
                         | (0xa << 8) | (0x1 << 7) | (0x1 << 6))
#define CVMARM_F2I_OPCODE ((0x1d << 23) | (0x3 << 20) | (0xd << 16) \
                         | (0xa << 8) | (0x1 << 7) | (0x1 << 6)) 
#define CVMARM_I2D_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x8 << 16) \
                         | (0xb << 8) | (0x1 << 7) | (0x1 << 6)) 
#define CVMARM_D2I_OPCODE ((0x1d << 23) | (0x3 << 20) | (0xd << 16) \
                         | (0xb << 8) | (0x1 << 7) | (0x1 << 6)) 
#define CVMARM_F2D_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x7 << 16) \
                         | (0xa << 8) | (0x1 << 7) | (0x1 << 6)) 
#define CVMARM_D2F_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x7 << 16) \
                         | (0xb << 8) | (0x1 << 7) | (0x1 << 6)) 

/* Floating-point comparison opcodes: */
/* Used only within ARM-specific code generation */
#define CVMARM_FMSTAT_OPCODE ((0xef1fa << 8) | (0x1 << 4))
#define CVMARM_FCMPES_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x4 << 16) \
                            | (0xa << 8) | (0x1 << 7) | (0x1 << 6))
#define CVMARM_DCMPED_OPCODE ((0x1d << 23) | (0x3 << 20) | (0x4 << 16) \
                            | (0xb << 8) | (0x1 << 7) | (0x1 << 6))

/* Floating-point move to general purpose register opcodes: */
/* Used only within ARM-specific code generation */
#define CVMARM_MOVFA_OPCODE ((0xe0 << 20) | (0xa << 8) | 0x10) /* FMSR */
#define CVMARM_MOVAF_OPCODE ((0xe1 << 20) | (0xa << 8) | 0x10) /* FMRS */
#define CVMARM_MOVDA_OPCODE ((0xc4 << 20) | (0xb << 8) | (0x01 << 4)) /* FMDRR */
#define CVMARM_MOVAD_OPCODE ((0xc5 << 20) | (0xb << 8) | (0x01 << 4)) /* FMRRD */


/* Floating-point status register opcodes: */
#define CVMARM_FPSID 0x00000000
#define CVMARM_FPSCR 0x00000001
#define CVMARM_FPEXC 0x00001000
#define CVMARM_MOVSA_OPCODE ((0xee << 20) | (0xa << 8) | 0x10) /* FMXR */
#define CVMARM_MOVAS_OPCODE ((0xeF << 20) | (0xa << 8) | 0x10) /* FMRX */


/* 64 bit ALU opcodes:
 * NOTE: The ALU64 opcodes are actually encoded in terms of 2 32 bit ARM
 *       opcodes and a boolean:
 *           ((lowOpcode << 16) | (highOpcode << 8) | setCCForLowOpcode)
 */
#define CVMCPU_NEG64_OPCODE 0x00000006
#define CVMCPU_ADD64_OPCODE ((0x08 << 16)|(0x0a << 8)|1) /* ADD,ADC,true */
#define CVMCPU_SUB64_OPCODE ((0x04 << 16)|(0x0c << 8)|1) /* SUB,SBC,true */
#define CVMCPU_AND64_OPCODE ((0x00 << 16)|(0x00 << 8)|0) /* AND,AND,false */
#define CVMCPU_OR64_OPCODE  ((0x18 << 16)|(0x18 << 8)|0) /* OR,OR,false */
#define CVMCPU_XOR64_OPCODE ((0x02 << 16)|(0x02 << 8)|0) /* XOR,XOR,false */

#define CVMCPU_MUL64_OPCODE 0x00000007
#define CVMCPU_DIV64_OPCODE 0x00000008
#define CVMCPU_REM64_OPCODE 0x00000009
#define CVMCPU_CMP64_OPCODE 0x0000000a

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the types for the ALURhs abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

/* In the ARM implementation the CVMCPUALURhs (i.e. the struct used to
 * represent an "aluRhs" operand) can have up to two register designations and
 * a shift direction packed into a single 'addressing' mode.  CVMCPUALURhs
 * pointers can be managed as elements of the code-gen time semantic stack.
 */

typedef enum {
    CVMARM_ALURHS_CONSTANT,
    CVMARM_ALURHS_SHIFT_BY_REGISTER,
    CVMARM_ALURHS_SHIFT_BY_CONSTANT
} CVMARMALURhsType;

/*
 * We could use a union to make this more compact,
 * but at much loss of clarity.
 */
typedef struct {
    CVMARMALURhsType    type;
    int                 shiftOp;
    CVMInt32            constValue;
    CVMRMResource*      r1;
    CVMRMResource*      r2;
} CVMCPUALURhs;

typedef CVMUint32 CVMCPUALURhsToken;

/* ARM specific ALURhs encoding bits: */
#define CVMARM_MODE1_CONSTANT       (1 << 25)
#define CVMARM_MODE1_SHIFT_CONSTANT (0) 
#define CVMARM_MODE1_SHIFT_REGISTER (1 << 4)

#define CVMCPUALURhsTokenConstZero  (1 << 25) /* Mode 1 constant 0. */

/**************************************************************
 * CPU MemSpec and associated types - The following are definition
 * of the types for the MemSpec abstraction required by the RISC
 * emitter porting layer.
 **************************************************************/

typedef enum {
    CVMCPU_MEMSPEC_IMMEDIATE_OFFSET,
    CVMCPU_MEMSPEC_REG_OFFSET,
    CVMCPU_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET,
    CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET
} CVMCPUMemSpecType;

/* Class: CVMCPUMemSpec
   Purpose: Encapsulates the parameters for encoding a memory access
            specification for the use of a memory reference instruction.
*/
typedef struct CVMCPUMemSpec CVMCPUMemSpec;
struct CVMCPUMemSpec {
    CVMCPUMemSpecType type;

    /* The following are the parameters needed to encode the fields in a
       memory reference instruction.  The method of encoding is determined by
       the addressMode above.  The field which are specific to each address
       mode could be set up as a union for compactness but is left as below
       for clarity. */

    /* Address mode: CVMCPU_MEMSPEC_IMMEDIATE_OFFSET: */
    CVMInt32       offsetValue;

    /* Address mode: CVMCPU_MEMSPEC_REG_OFFSET: */
    CVMRMResource *offsetReg;
    CVMInt32       shiftOpcode;
    CVMInt32       shiftAmount;
};

typedef CVMUint32 CVMCPUMemSpecToken;

/**************************************************************
 * CPU MemSpec and associated types
 **************************************************************/

/* MemSpec private definitions: =========================================== */

/*
 * These are the address-mode computation bits used for modes 2 and 3.
 * the P, U, and W bits (pre/post index, offset add/subtract, and write back)
 * are common and are ORed directly into the instruction. The imm/reg offset
 * indicator bit has to be steered based on opcode. And of course the
 * representation of an immediate value is opcode dependent as well.
 *
 * Be careful of the U bit: for an immediate offset it is NOT the same
 * as a sign bit. Default needs to be U bit set.
 * 
 * Be careful of the P bit: for the usual operations, you want to
 * preindex. Failing to do so will compute the wrong address AND
 * cause the base register to be updated. Default needs to be P bit set.
 */
#define ARM_LOADSTORE_PREINDEX	0x01000000	/* P bit */
#define ARM_LOADSTORE_ADDOFFSET	0x00800000	/* U bit */
#define ARM_LOADSTORE_WRITEBACK 0x00200000	/* W bit */ 
#define ARM_LOADSTORE_IMMEDIATE_OFFSET 0
#define ARM_LOADSTORE_REGISTER_OFFSET (1 << 25)
#define ARM_LOADSTORE_MODE3_IMMEDIATE_OFFSET (1 << 22)

/* The common combinations: reg+reg addressing, reg+imm addressing */
#define ARM_MEMSPEC_IMMEDIATE_OFFSET \
    (ARM_LOADSTORE_PREINDEX | ARM_LOADSTORE_ADDOFFSET | \
    ARM_LOADSTORE_IMMEDIATE_OFFSET)

#define ARM_MEMSPEC_REG_OFFSET \
    (ARM_LOADSTORE_PREINDEX | ARM_LOADSTORE_ADDOFFSET | \
    ARM_LOADSTORE_REGISTER_OFFSET)

/*
 * How to use the load/store word instructions to access a stack:
 * These assume that stack grows + and that the stack pointer is
 * addressing the EMPTY cell after the top. Thus post-increment and
 * pre-decrement are what you want.
 */
#define ARM_MEMSPEC_POSTINCREMENT_IMMEDIATE_OFFSET \
	(ARM_LOADSTORE_IMMEDIATE_OFFSET | ARM_LOADSTORE_ADDOFFSET)

#define ARM_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET \
	(ARM_LOADSTORE_PREINDEX | ARM_LOADSTORE_IMMEDIATE_OFFSET | \
	ARM_LOADSTORE_WRITEBACK)

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/
typedef struct CVMCPUCallContext CVMCPUCallContext;
struct CVMCPUCallContext
{
    CVMRMResource *reservedRes;
};

#define CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GCTRAP_INSTRUCTION \
    (CVMCPU_COND_AL << 28 | CVMCPU_LDR32_OPCODE | \
     ARM_LOADSTORE_PREINDEX  | \
     CVMCPU_GC_REG << 16 | CVMCPU_GC_REG << 12)
#define CVMCPU_GCTRAP_INSTRUCTION_MASK ~0x00800fff
#endif

#endif /* _INCLUDED_ARM_JITRISCEMITTERDEFS_CPU_H */
