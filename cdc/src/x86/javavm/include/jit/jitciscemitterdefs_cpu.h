/*
 * @(#)jitciscemitterdefs_cpu.h	1.5 06/10/24
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

#ifndef _INCLUDED_X86_JITCISCEMITTERDEFS_CPU_H
#define _INCLUDED_X86_JITCISCEMITTERDEFS_CPU_H

/*
 * This file defines all of the emitter types and option definitions that
 * in a platform indendent way to be used by the common CISC jit back end.
 *
 * The exported symbols are prefixed with CVMCPU_.  Other symbols should be
 * considered private to the X86 specific parts of the jit, including those
 * with the CVMX86_ prefix.
 *
 * This file should be included at the top of jitciscemitter.h.
 */

/* TODO (Mirko): remove this temporary fix! */
#include "javavm/include/jit/jitcisc_cpu.h"

/**************************************************************
 * CPU condition codes - The following are definition of the condition
 * codes that are required by the CISC emitter porting layer.
 **************************************************************/

typedef enum CVMX86Condition {         /* The x86 condition codes used for conditional jumps/moves. */
  CVMX86zero          = 0x4,
  CVMX86notZero       = 0x5,
  CVMX86equal         = 0x4,
  CVMX86notEqual      = 0x5,
  CVMX86less          = 0xc,
  CVMX86lessEqual     = 0xe,
  CVMX86greater       = 0xf,
  CVMX86greaterEqual  = 0xd,
  CVMX86below         = 0x2,
  CVMX86belowEqual	  = 0x6,
  CVMX86above         = 0x7,
  CVMX86aboveEqual	  = 0x3,
  CVMX86overflow      = 0x0,
  CVMX86noOverflow	  = 0x1,
  CVMX86carrySet      = 0x2,
  CVMX86carryClear	  = 0x3,
  CVMX86negative      = 0x8,
  CVMX86positive      = 0x9,
  CVMX86parity        = 0xa,
  CVMX86noParity      = 0xb,
  CVMX86unused        = 0x10
} CVMX86Condition;

enum CVMCPUCondCode 
{
   CVMCPU_COND_EQ = CVMX86equal,        /* Do when equal */
   CVMCPU_COND_NE = CVMX86notEqual,     /* Do when NOT equal */
   CVMCPU_COND_MI = CVMX86negative,     /* Do when has minus / negative */
   CVMCPU_COND_PL = CVMX86positive,     /* Do when has plus / positive or zero */
   CVMCPU_COND_OV = CVMX86overflow,     /* Do when overflowed */
   CVMCPU_COND_NO = CVMX86noOverflow,   /* Do when NOT overflowed */
   CVMCPU_COND_LT = CVMX86less,         /* Do when signed less than */
   CVMCPU_COND_GT = CVMX86greater,      /* Do when signed greater than */
   CVMCPU_COND_LE = CVMX86lessEqual,    /* Do when signed less than or equal */
   CVMCPU_COND_GE = CVMX86greaterEqual, /* Do when signed greater than or equal */
   CVMCPU_COND_LO = CVMX86below,        /* Do when lower / unsigned less than */
   CVMCPU_COND_HI = CVMX86above,        /* Do when higher / unsigned greater than */
   CVMCPU_COND_LS = CVMX86belowEqual,   /* Do when lower or same / is unsigned <= */
   CVMCPU_COND_HS = CVMX86aboveEqual,   /* Do when higher or same / is unsigned >= */
   CVMCPU_COND_AL = CVMX86unused,       /* Do always */
   CVMCPU_COND_NV = 1+CVMX86unused     /* Do never */
   
#ifdef CVM_JIT_USE_FP_HARDWARE
   /*
    * Floating-point condition codes are used when floating point
    * hardware is accessible.
    *
    * If the UNORDERED_LT flag is set, then unordered is treated as
    * less than.
    * 
    * If the UNORDERED_LT flag is not set, then unordered is treated
    * as greater than.
    */
#define CVMCPU_COND_UNORDERED_LT	32 /* must be power of 2 that
					    * is greater than every
					    * condition code. then we
					    * can `or' it into the
					    * floating point condition
					    * code */
#define CVMCPU_COND_FFIRST (2+CVMX86unused)
   ,
   CVMCPU_COND_FEQ = 0+CVMCPU_COND_FFIRST, /* Do when equal */
   CVMCPU_COND_FNE = 1+CVMCPU_COND_FFIRST, /* Do when NOT equal */
   CVMCPU_COND_FLT = 2+CVMCPU_COND_FFIRST, /* Do when less than */
   CVMCPU_COND_FGT = 3+CVMCPU_COND_FFIRST, /* Do when U or greater than */
   CVMCPU_COND_FLE = 4+CVMCPU_COND_FFIRST, /* Do when less than or equal */
   CVMCPU_COND_FGE = 5+CVMCPU_COND_FFIRST  /* Do when U or greater than or equal */
#endif /* CVM_JIT_USE_FP_HARDWARE */
};
typedef enum CVMCPUCondCode CVMCPUCondCode;

enum CVMCPUOpCode
{
/* Virtual CPU opcodes. These are used in the porting API and are not
   actually used for encoding. */
   
   CVMCPU_NOP_INSTRUCTION /* nop */,
   
/* integer memory access */
   CVMCPU_LDR64_OPCODE      /* Load 64 bit value. */,
   CVMCPU_STR64_OPCODE      /* Store 64 bit value. */,
   CVMCPU_LDR32_OPCODE      /* Load 32 bit value. */,
   CVMCPU_STR32_OPCODE      /* Store 32 bit value. */,
   CVMCPU_LDR16U_OPCODE     /* Load unsigned 16 bit value. */,
   CVMCPU_LDR16_OPCODE      /* Load signed 16 bit value. */,
   CVMCPU_STR16_OPCODE      /* Store 16 bit value. */,
   CVMCPU_LDR8U_OPCODE      /* Load unsigned 8 bit value. */,
   CVMCPU_LDR8_OPCODE       /* Load signed 8 bit value. */,
   CVMCPU_STR8_OPCODE       /* Store 8 bit value. */,

/* 32 bit integer arithmetic */
   CVMCPU_ANDN_OPCODE  /* reg32 <- reg32 AND ~aluRhs32. */,
   CVMCPU_BIC_OPCODE = CVMCPU_ANDN_OPCODE /* reg32 <- reg32 AND ~aluRhs32*/,
   CVMCPU_ADD_OPCODE   /* reg32 <- reg32 + aluRhs32. */,
   CVMCPU_CMN_OPCODE = CVMCPU_ADD_OPCODE /* cmp reg32, -aluRhs32; set cc */, 
   CVMCPU_SUB_OPCODE   /* reg32 <- reg32 - aluRhs32. */,
   CVMCPU_CMP_OPCODE = CVMCPU_SUB_OPCODE /* cmp reg32, aluRhs32; set cc */,
   CVMCPU_AND_OPCODE   /* reg32 <- reg32 AND aluRhs32. */,
   CVMCPU_OR_OPCODE    /* reg32 <- reg32 OR aluRhs32. */,
   CVMCPU_XOR_OPCODE   /* reg32 <- reg32 XOR aluRhs32. */,
   CVMCPU_MOV_OPCODE   /* reg32 <- aluRhs32. */,
   CVMCPU_NEG_OPCODE   /* reg32 <- -reg32. */,
   CVMCPU_NOT_OPCODE   /* reg32 <- (reg32 == 0) ? 1 : 0. */,
   CVMCPU_INT2BIT_OPCODE /* reg32 <- (reg32 != 0) ? 1 : 0. */,
   CVMCPU_SLL_OPCODE   /* Shift Left Logical */,
   CVMCPU_SRL_OPCODE   /* Shift Right Logical */,
   CVMCPU_SRA_OPCODE   /* Shift Right Arithmetic */,
   CVMCPU_MULL_OPCODE  /* reg32 <- LO32(reg32 * reg32). */,
   CVMCPU_MULH_OPCODE  /* reg32 <- HI32(reg32 * reg32). */,
   CVMCPU_DIV_OPCODE,
   CVMCPU_REM_OPCODE,

/* 64 bit integer arithmetic */
#if 0 
/* not used by current X86 port */
   CVMCPU_MUL64_OPCODE,
   CVMCPU_DIV64_OPCODE,
   CVMCPU_REM64_OPCODE,
#endif
   CVMCPU_ADD64_OPCODE,
   CVMCPU_AND64_OPCODE,
   CVMCPU_CMP64_OPCODE,
   CVMCPU_NEG64_OPCODE,
   CVMCPU_OR64_OPCODE,
   CVMCPU_SLL64_OPCODE   /* Shift Left Logical */,
   CVMCPU_SRL64_OPCODE   /* Shift Right Logical */,
   CVMCPU_SRA64_OPCODE   /* Shift Right Arithmetic */,
   CVMCPU_SUB64_OPCODE,
   CVMCPU_XOR64_OPCODE

#ifdef CVM_JIT_USE_FP_HARDWARE
   ,

/* 32-bit floating point memory access */
   CVMCPU_FLDR32_OPCODE	 /* Load float 32 bit value */,
   CVMCPU_FSTR32_OPCODE	 /* Store float 32 bit value */,

/* 32-bit floating point arithmetic */
   CVMCPU_FADD_OPCODE,
   CVMCPU_FSUB_OPCODE,
   CVMCPU_FMUL_OPCODE,
   CVMCPU_FDIV_OPCODE,
   CVMCPU_FCMP_OPCODE,
   CVMCPU_FNEG_OPCODE,
   CVMCPU_FMOV_OPCODE,

/* 64-bit floating point memory access */
   CVMCPU_FLDR64_OPCODE	 /* Load float 64 bit value */,
   CVMCPU_FSTR64_OPCODE	 /* Store float 64 bit value */,

/* 64-bit floating point arithmetic */
   CVMCPU_DADD_OPCODE,
   CVMCPU_DSUB_OPCODE,
   CVMCPU_DMUL_OPCODE,
   CVMCPU_DDIV_OPCODE,
   CVMCPU_DCMP_OPCODE,
   CVMCPU_DNEG_OPCODE,

/* floating point conversion. */
   CVMCPU_I2F_OPCODE,
   CVMCPU_L2F_OPCODE,
   CVMCPU_F2I_OPCODE,
   CVMCPU_F2L_OPCODE,
   CVMCPU_I2D_OPCODE,
   CVMCPU_L2D_OPCODE,
   CVMCPU_D2I_OPCODE,
   CVMCPU_D2L_OPCODE,
   CVMCPU_F2D_OPCODE,
   CVMCPU_D2F_OPCODE

#endif /* CVM_JIT_USE_FP_HARDWARE */
};
typedef enum CVMCPUOpCode CVMCPUOpCode;

#define CVMCPU_LDRADDR_OPCODE   CVMCPU_LDR32_OPCODE      /* Load address value. */
#define CVMCPU_STRADDR_OPCODE   CVMCPU_STR32_OPCODE      /* Store address value. */

/**************************************************************
 * CPU ALURhs and associated types - The following are definition
 * of the types for the ALURhs abstraction required by the CISC
 * emitter porting layer.
 **************************************************************/

/* 
 * X86 addressing modes:
 *     - register + register
 *     - register + immediate
 */

enum CVMCPUALURhsType {
    CVMCPU_ALURHS_REGISTER,
    CVMCPU_ALURHS_CONSTANT
};
typedef enum CVMCPUALURhsType CVMCPUALURhsType;

/*
 * We could use a union to make this more compact,
 * but at much loss of clarity.
 */
typedef struct CVMCPUALURhs CVMCPUALURhs;
struct CVMCPUALURhs {
    CVMCPUALURhsType  type;
    CVMInt32            constValue;
    CVMRMResource*      r;
};

typedef struct CVMCPUALURhsTokenStruct {
  CVMCPUALURhsType  type;
  CVMInt32  con;
  CVMInt32  reg;
} *CVMCPUALURhsToken;

extern struct CVMCPUALURhsTokenStruct CVMCPUALURhsTokenStructConstZero;
#define CVMCPUALURhsTokenConstZero (&CVMCPUALURhsTokenStructConstZero) 

/**************************************************************
 * CPU MemSpec and associated types - The following are definition
 * of the types for the MemSpec abstraction required by the CISC
 * emitter porting layer.
 **************************************************************/

enum CVMCPUMemSpecType {
  CVMCPU_MEMSPEC_REG_OFFSET = 0,              /* basereg + indexreg */
  CVMCPU_MEMSPEC_IMMEDIATE_OFFSET = 1,        /* basereg + displacement */
  CVMCPU_MEMSPEC_ABSOLUTE,                    /* absolute_address */
  CVMCPU_MEMSPEC_SCALED_REG_OFFSET,           /* basereg + indexreg*scale */
  CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET, /* basereg + indexreg*scale + displacement */
  /* NOTE: X86 does not support post-increment and pre-decrement
     memory access. Extra add/sub instruction is emitted in order to
     achieve it. */ 
  CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET
};
typedef enum CVMCPUMemSpecType CVMCPUMemSpecType;

/* Class: CVMCPUMemSpec
   Purpose: Encapsulates the parameters for encoding a memory access
            specification for the use of a memory reference instruction.
   However, a base register is always specified outside of the CVMCPUMemSpec!
   A more appropriate name would have been CVMCPUOffsetSpec. Anyway, no
   base register is inserted for X86 for this reason.
*/
typedef struct CVMCPUMemSpec CVMCPUMemSpec;
struct CVMCPUMemSpec {

  CVMCPUMemSpecType     type;

  /* Used in:
     o CVMCPU_MEMSPEC_REG_OFFSET as only entry
     o CVMCPU_MEMSPEC_SCALED_REG_OFFSET together with scale
     o CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET together with scale and offsetValue */
  CVMRMResource  *offsetReg;   /* The X86 index register. */

  /* Used in:
     o CVMCPU_MEMSPEC_IMMEDIATE_OFFSET as only entry
     o CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET together with scale and offsetReg */
  CVMInt32       offsetValue;

  int            scale;        /* New for X86. */
};

typedef struct CVMCPUMemSpecTokenStruct {
  CVMCPUMemSpecType     type;
  CVMInt32              offset;
  CVMInt32              regid;
  CVMInt32              scale;
} *CVMCPUMemSpecToken;

/**************************************************************
 * CPU register and addressing types.  The following are definition of
 * the types for the MemSpec abstraction required by the CISC emitter
 * porting layer.
 **************************************************************/

typedef unsigned char* CVMX86address;

typedef enum CVMX86Prefix {
  /* segment overrides */
  CVMX86CS_segment = 0x2e,
  CVMX86SS_segment = 0x36,
  CVMX86DS_segment = 0x3e,
  CVMX86ES_segment = 0x26,
  CVMX86FS_segment = 0x64,
  CVMX86GS_segment = 0x65
} CVMX86Prefix;

typedef enum CVMX86ScaleFactor {
  CVMX86no_scale = -1,
  CVMX86times_1  =  0,
  CVMX86times_2  =  1,
  CVMX86times_4  =  2,
  CVMX86times_8  =  3
} CVMX86ScaleFactor;

typedef enum CVMX86Register {
  CVMX86no_reg = CVMCPU_INVALID_REG,
  CVMX86eax    = CVMX86_EAX,
  CVMX86ebx    = CVMX86_EBX,
  CVMX86ecx    = CVMX86_ECX,
  CVMX86edx    = CVMX86_EDX,
  CVMX86esi    = CVMX86_ESI,
  CVMX86edi    = CVMX86_EDI,
  CVMX86ebp    = CVMX86_EBP,
  CVMX86esp    = CVMX86_ESP,
  CVMX86eflags = CVMX86_EFLAGS
} CVMX86Register;

typedef struct CVMX86Address {
  CVMX86Register         _base;
  CVMX86Register         _index;
  CVMX86ScaleFactor      _scale;
  int                    _disp;
  CVMCPUMemSpecType      _type; 
} CVMX86Address;

/* export some types */
typedef enum CVMX86ScaleFactor CVMCPUScaleFactor;
typedef enum CVMX86Register CVMCPURegister;
typedef struct CVMX86Address CVMCPUAddress;

/**************************************************************
 * CPU emitter options
 **************************************************************/

/* ===== Optional Advanced Emitter APIs =================================== */

#undef CVMCPU_HAVE_MUL_BY_CONST_POWER_OF_2_PLUS_1

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the CISC emitter porting layer.
 **************************************************************/

#define CVMCPU_HAVE_PLATFORM_SPECIFIC_C_CALL_CONVENTION

/* define our own CVMCPUCallContext */
typedef CVMUint32 CVMCPUCallContext;

/* the IA-32 backend needs to know, whether the code it is producing
 belongs to a method using float registers. turning this switch on
 will provide the information in the compilation context. */
#define CVM_JIT_DETECT_FLOAT_CODE

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMCPU_GC_REG CVMCPU_ARG1_REG
#define CVMCPU_GCTRAP_INSTRUCTION 0x00408b
#define CVMCPU_GCTRAP_INSTRUCTION_MASK 0x00ffff
#endif

#endif /* _INCLUDED_X86_JITCISCEMITTERDEFS_CPU_H */
