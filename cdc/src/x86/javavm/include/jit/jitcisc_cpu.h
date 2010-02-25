/*
 * @(#)jitcisc_cpu.h	1.7 06/10/23
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

#ifndef _INCLUDED_X86_JITCISC_CPU_H
#define _INCLUDED_X86_JITCISC_CPU_H

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"


/* This file #defines all of the macros that shared cisc parts of the jit
 * use in a platform indendent way. The exported symbols are prefixed
 * with CVMCPU_. Other symbols should be considered private to the
 * x86 specific parts of the jit, including those with the
 * CVMX86_ prefix.
 */


/************************************************
 * Register Definitions.
 ************************************************/

/* The eight 32-bit general purpose registers: */
#define CVMX86_EAX   0  /* Accumulator for operands and result data. */
#define CVMX86_EDX   1  /* IO pointer. */
#define CVMX86_EBX   2  /* Pointer to data in the DS segment. */
#define CVMX86_ECX   3  /* Counter for string and loop operations. */
#define CVMX86_ESP   4  /* Stack pointer. */
#define CVMX86_EBP   5  /* Pointer to data on the stack. */
#define CVMX86_ESI   6  /* Source pointer for string operations. */
#define CVMX86_EDI   7  /* Destination pointer for string operations. */
#define CVMX86_EFLAGS  8  /* Should be treated as caller-save! */

/* virtual registers the compiler expects to exist */
#define CVMX86_VIRT_CP_REG                       9

/* registers for C helper arguments */
/*
 * These arg registers are used whenever the resource
 * manager don't work, e.g. when emitting the prologue. Don't
 * use these registers in emitter rules explicitely!
 * all arguments are delivered trough the stack
 * so this is a definition which convieniently works 
 * for 64-bit operands
 */
#define CVMCPU_ARG1_REG    CVMX86_EAX
#define CVMCPU_ARG2_REG    CVMX86_EDX
#define CVMCPU_ARG3_REG    CVMX86_EBX
#define CVMCPU_ARG4_REG    CVMX86_ECX


/* registers for C helper result */
#define CVMCPU_RESULT1_REG   CVMX86_EAX


#define CVMX86_SP  CVMX86_ESP
/* (SW): We need a real register for implementing jsr and ret. */
#define CVMX86_LR CVMX86_EAX


/* The IA-32 floating-point registers.  In FP instruction encodings,
 register numbers are understood as offsets to the current TOP value,
 so the same number does not always refer to the same register!. */
#define CVMX86_FST0  0
#define CVMX86_FST1  1
#define CVMX86_FST2  2
#define CVMX86_FST3  3
#define CVMX86_FST4  4
#define CVMX86_FST5  5
#define CVMX86_FST6  6
#define CVMX86_FST7  7

/* registers for C helper result */
#define CVMCPU_FPRESULT1_REG   CVMX86_FST0

/* Some registers known to regman and codegen.jcs */
#define CVMCPU_SP_REG                  CVMX86_SP
#define CVMCPU_JFP_REG                 CVMX86_EBP
#define CVMCPU_JSP_REG                 CVMX86_EDI
#define CVMCPU_PROLOGUE_PREVFRAME_REG  CVMCPU_ARG3_REG
#define CVMCPU_PROLOGUE_NEWJFP_REG     CVMCPU_ARG4_REG

#ifdef CVMCPU_HAS_ZERO_REG
// No zero-reg for X86.
#endif
#define CVMCPU_INVALID_REG     (-1)

/* 
 * More registers known to regman and codegen.jcs. These ones are
 * optional. If you have registers to spare then defining these macros
 * allows for more efficient code generation. The registers will
 * be setup by CVMJITgoNative() when compiled code is first entered.
 */
/* #undef CVMCPU_CHUNKEND_REG */
/* #undef CVMCPU_HAS_CP_REG */
#undef CVMCPU_EE_REG          
#ifdef CVMCPU_HAS_CP_REG
#define CVMCPU_CP_REG	       CVMX86_VIRT_CP_REG
#endif


/*******************************************************
 * Register Sets
 *******************************************************/

/* The set of all registers */
#define CVMCPU_ALL_SET  0x1fffff /* 17 registers + 4 stack allocated virtual registers */

/*
 * Registers that regman should never allocate. Only list registers
 * that haven't already been exported to regman. For example, there's
 * no need to tell regman about CVMCPU_SP_REG because regman already
 * knows that it is busy.
 */
#define CVMCPU_BUSY_SET ( 1U<<CVMX86_ESP | /* SP not allocatable */	    \
			  1U<<CVMX86_EFLAGS /* EFLAGS not allocatable, >max_interesting anyway */ )
  /* floats are handled separately. */

/* SVMC_JIT c5041613 (dk) 2004-01-26.  The register manager usually
 tries to prefer persistent registers during register selection. If
 too few persistent registers are available, this should be prevented.
 to this end, CVMCPU_NO_REGPREF_PERSISTENT should be defined. */
#define CVMCPU_NO_REGPREF_PERSISTENT

/*
 * The set of all non-volatile registers according to C calling conventions
 * There's no need to put CVMCPU_BUSY_SET registers in this set.
 * These are registers not globbered by C calls.
 */
#define CVMCPU_NON_VOLATILE_SET ( 1U<<CVMX86_EBP | 1U<<CVMX86_ESP | 1U<<CVMX86_ESI )

/* The set of all volatile registers according to C calling conventions */
#define CVMCPU_VOLATILE_SET \
    (CVMCPU_ALL_SET & ~CVMCPU_NON_VOLATILE_SET)

/* 
 * The set of registers dedicated for PHI spills. Usually the
 * first PHI is at the bottom of the range of available non-volatile registers.
 */
#define CVMCPU_PHI_REG_SET (			\
    1U<<CVMX86_ESI \
)

/*
 * The set of registers that regman should only use as a last resort.
 *
 * Usually this includes the set of phi registers, but if you have a limited
 * number of non-phi non-volatile registers available, then at most only
 * include the first couple of phi registers.
 *
 * It also should include the argument registers, since we would rather
 * that other non-volatile registers get allocated before these.
 */
#define CVMCPU_AVOID_SET (			\
    /* phi registers */				\
    CVMCPU_PHI_REG_SET)

/*
 * Alignment parameters of integer registers are trivial
 * All quantities in 32-bit words
 */
#define CVMCPU_SINGLE_REG_ALIGNMENT	1
#define CVMCPU_DOUBLE_REG_ALIGNMENT	2

/*
 * If we use a floating point unit 
 */
#ifdef CVM_JIT_USE_FP_HARDWARE

#define CVMCPU_FP_MIN_INTERESTING_REG	 0
/* 
 * SVMC_JIT d022609 (ML) 2003-08-28.  Leave out FR(7) on IA-32 since
 * we use it as top of stack register. The remaining 7 registers
 * (FR(0)-FR(6)) are used as a random access register file. Note that
 * the FP register numbers supplied in IA-32 instructions are actually
 * offsets(!) wrt to the register number in the FPU's TOP register
 * field. Generated code relies on and maintains the invariant that
 * before and after every complete FP operation, TOP will refer to
 * FR(0). Hence an `fld' will load into FR(7). From there the value
 * will be popped (fstp) into any of FR(0) ... FR(6). This makes a
 * single FP load/store cost two operations. But this way we can use
 * all available registers except FR(7) and do not require changes to
 * the CVM's register manager.
 */
#define CVMCPU_FP_MAX_INTERESTING_REG	 6
#define CVMCPU_FP_BUSY_SET               0
#define CVMCPU_FP_ALL_SET                0x7f
#define CVMCPU_FP_NON_VOLATILE_SET       0
#define CVMCPU_FP_VOLATILE_SET           CVMCPU_FP_ALL_SET

/* SVMC_JIT d022609 (ML) 2004-05-17. TODO: spend some phi regs? */
#define CVMCPU_FP_PHI_REG_SET (		\
    0 \
)

/*
 * Alignment parameters of floating registers
 * All quantities in 32-bit words
 */
#define CVMCPU_FP_SINGLE_REG_ALIGNMENT	1
/* IA-32 FPU has no alignment restrictions */
#define CVMCPU_FP_DOUBLE_REG_ALIGNMENT	1

#endif

/*
 * Set of registers that can be used for the jsr return address. If the LR
 * is a GPR, then just set it to the LR reg. Otherwise usually it is best
 * to allow any register to be used.
 */
#define CVMCPU_JSR_RETURN_ADDRESS_SET (1U << CVMX86_LR)


/* range of registers that regman should look at */
#define CVMCPU_MIN_INTERESTING_REG      CVMX86_EAX  /* encoding: 0 */
#define CVMCPU_MAX_INTERESTING_REG      CVMX86_EDI  /* encoding: 7 */

/************************************************************************
 * CPU features - These macros define various features of the processor.
 ************************************************************************/

/* no conditional instructions supported other than branches */
#undef  CVMCPU_HAS_CONDITIONAL_ALU_INSTRUCTIONS
/* TODO(rr) x86 _has_ conditional moves */
#undef  CVMCPU_HAS_CONDITIONAL_LOADSTORE_INSTRUCTIONS
#undef  CVMCPU_HAS_CONDITIONAL_CALL_INSTRUCTIONS

/* ALU instructions can set condition codes */
#define CVMCPU_HAS_ALU_SETCC

/* x86 has an integer mul by immediate instruction */
/* Implement CVMCPUemitMulConstant() if this #defined */
#define CVMCPU_HAS_IMUL_IMMEDIATE

/* x86 does not have a postincrement store */
#undef CVMCPU_HAS_POSTINCREMENT_STORE

/* Maximum offset (+/-) for a load/store word instruction. */
#define CVMCPU_MAX_LOADSTORE_OFFSET  0x7fffffff



/* Number of instructions reserved for setting up constant pool base
 * register in method prologue */
#ifdef CVMCPU_HAS_CP_REG
/* TODO(rr): find out, how many bytes to reserve */
#define CVMCPU_RESERVED_CP_REG_INSTRUCTIONS 0  
#endif

/*
 * SVMC_JIT (rr). Number of nop's needed for GC patching. We need 8,
 * because a call is 5 and a branch is 6 bytes and we have an atomic
 * compare and swap for 8 bytes.
 */
#define CVMCPU_NUM_NOPS_FOR_GC_PATCH 8

#define CVMCPU_ARGREGS ((1U<<CVMX86_EAX)|(1U<<CVMX86_EDX)|(1U<<CVMX86_EBX)|(1U<<CVMX86_ECX))

#endif /* _INCLUDED_X86_JITCISC_CPU_H */
