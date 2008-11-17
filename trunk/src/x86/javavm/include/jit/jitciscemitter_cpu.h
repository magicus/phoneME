/*
 * @(#)jitciscemitter_cpu.h	1.5 06/10/24
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

#ifndef _INCLUDED_X86_JITCISCEMITTER_CPU_H
#define _INCLUDED_X86_JITCISCEMITTER_CPU_H

/*
 * This file #defines all of the emitter function prototypes and
 * macros that shared parts use in a platform indendent way in CISC
 * ports of the jit back end.
 *
 * The exported functions and macros are prefixed with CVMCPU_.  
 * 
 * Other functions and macros should be considered private to the X86
 * specific parts of the jit, including those with the CVMX86_ prefix.
 */

/**********************************************************************
 * Our stack layout and the names of the registers we're going to use
 * to access it:
 *
 *	+----------------------
 *	| Java local 0
 *	+----------------------
 *	...
 *	+----------------------
 *	| Java local n
 *	+----------------------
 * JFP->| control information here
 *	| of some size...
 *	+----------------------
 *	| Java temp 0	( code generation allocated )
 *	+----------------------
 *	...
 *	+----------------------
 *	| Java temp n	( code generation allocated )
 *	+----------------------
 * JSP->| Outgoing Java parameters.
 *	+----------------------
 *
 * Java locals and temps accessed via JFP,
 * JSP  used to access only the stack top.
 *
 * In truth, we really only need JSP, since the offsets of everything
 * else are known at compilation of each instruction. But we could never
 * debug the resulting code! This seems like a bad idea in the short run.
 *
 * DK: Seems that all this can be reused for the CISC interface.
 */

/**************************************************************
 * CPU ALURhs and associated types - The following are function
 * prototypes and macros for the ALURhs abstraction required by
 * the CISC emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

#define CVMX86_MAX_RHS_IMMEDIATE_OFFSET 0x7fffffff /* == max_int <= - min_int */


#define CVMCPUalurhsIsEncodableAsImmediate(constValue) \
    ((CVMInt32)(constValue) <= CVMX86_MAX_RHS_IMMEDIATE_OFFSET && \
     (CVMInt32)(constValue) >= (-CVMX86_MAX_RHS_IMMEDIATE_OFFSET - 1))

#define CVMCPUalurhsIsConstant(aluRhs) \
    ((aluRhs)->type == CVMCPU_ALURHS_CONSTANT)

#define CVMCPUalurhsGetConstantValue(aluRhs)    ((aluRhs)->constValue)

/* ALURhs token encoder APIs:============================================= */

/* Purpose: Gets the token for the ALURhs value for use in the instruction
            emitters. */
CVMCPUALURhsToken
CVMX86alurhsGetToken(CVMJITCompilationContext* con, const CVMCPUALURhs *aluRhs);

#define CVMCPUalurhsGetToken(con, aluRhs)  CVMX86alurhsGetToken((con), (aluRhs))

/* Purpose: Encodes an CVMCPUALURhsToken for the specified arguments. */
/* NOTE: This function is private to the X86 implementation and can only be
         called from the shared implementation via the CVMCPUxxx macros
         below. */
CVMCPUALURhsToken
CVMX86alurhsEncodeToken(CVMJITCompilationContext *con, CVMCPUALURhsType type, 
			int constValue, int rRegID);

/* Purpose: Encodes a CVMCPUALURhsToken for the specified constant value. */
#define CVMCPUalurhsEncodeConstantToken(con, constValue) \
     CVMX86alurhsEncodeToken((con), CVMCPU_ALURHS_CONSTANT, (constValue), \
         CVMCPU_INVALID_REG)

#define CVMCPUalurhsEncodeRegisterToken(con, regID) \
     CVMX86alurhsEncodeToken((con), CVMCPU_ALURHS_REGISTER, -1, (regID))

/**************************************************************
 * CPU MemSpec and associated types - The following are function
 * prototypes and macros for the MemSpec abstraction required by
 * the CISC emitter porting layer.
 **************************************************************/

/* MemSpec query APIs: ==================================================== */

#define CVMCPUmemspecIsEncodableAsImmediate(value) \
    ((INT_MIN <= ((CVMInt32)(value))) && (((CVMInt32)(value)) <= INT_MAX))

#define CVMCPUmemspecIsEncodableAsOpcodeSpecificImmediate(opcode, value) \
    CVMCPUmemspecIsEncodableAsImmediate(value)

/* MemSpec token encoder APIs: ============================================ */

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMX86memspecGetToken(CVMJITCompilationContext *con, const CVMCPUMemSpec *memSpec);

#define CVMCPUmemspecGetToken(con, memSpec) \
    ((void)(con), CVMX86memspecGetToken((con), memSpec))

/* Allocates a new token and returns it. All memoryspec types are supported,
   except for auto-modify addressing modes. */
CVMCPUMemSpecToken
CVMX86memspecEncodeToken(CVMJITCompilationContext *con,
			 CVMCPUMemSpecType type, int offset, int regid, int scale);

#define CVMCPUmemspecEncodeImmediateToken(con, offset) \
    ((void)(con), CVMX86memspecEncodeToken((con), CVMCPU_MEMSPEC_IMMEDIATE_OFFSET, \
                      (offset), (offset), CVMX86times_1))

#define CVMCPUmemspecEncodeAbsoluteToken(con, address)				\
    ((void)(con), CVMX86memspecEncodeToken((con), CVMCPU_MEMSPEC_ABSOLUTE,	\
                      (address), (address), CVMX86times_1))

/* NOTE: X86 does not support post-increment and pre-decrement memory
   access. Extra add/sub instruction is emitted in order to achieve
   it. */
#define CVMCPUmemspecEncodePreDecrementImmediateToken(con, decrement)         \
    ((void)(con), CVMassert(CVMCPUmemspecIsEncodableAsImmediate(decrement)),  \
     CVMX86memspecEncodeToken((con), CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET, \
                                (decrement), (decrement), CVMX86times_1))

/**************************************************************
 * CPU code emitters - The following are prototypes of code
 * emitters required by the CISC emitter porting layer.
 **************************************************************/

/* ===== 32 Bit ALU Emitter APIs ========================================== */

#define CVMCPUemitBinaryALURegister(con, opcode, destRegID, lhsRegID, \
                                    rhsRegID, setcc) \
    CVMCPUemitBinaryALU((con), (opcode), (destRegID), (lhsRegID), \
        CVMCPUalurhsEncodeRegisterToken((con), (rhsRegID)), (setcc))

#define CVMCPUemitMoveRegister(con, opcode, destRegID, srcRegID, setcc) \
    CVMCPUemitMove((con), (opcode), (destRegID), \
        CVMCPUalurhsEncodeRegisterToken((con), (srcRegID)), (setcc))

/* ===== Branch Emitter APIs ========================================== */
#define CVMCPUemitBranch(con, target, condCode) \
    CVMCPUemitBranchNeedFixup((con), (target), (condCode), NULL)

#define CVMCPUemitBranchLink(con, target) \
    CVMCPUemitBranchLinkNeedFixup((con), (target), NULL)

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
CVMCPUInstruction
CVMX86getBranchInstruction(int opcode, CVMCPUCondCode condCode, int offset);

/*
 * Make a PC-relative branch or branch-and-link instruction.
 */
void
CVMX86emitBranch(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode /*, CVMBool annul */);


/**************************************************************
 * x86 Assembler
 **************************************************************/

void CVMX86emit_byte(CVMJITCompilationContext* con, int x);
void CVMX86emit_word(CVMJITCompilationContext* con, int x);
void CVMX86emit_long(CVMJITCompilationContext* con, CVMInt32 x);

/* accessors */
CVMBool CVMCPUAddress_uses(CVMCPUAddress addr, CVMCPURegister reg);

/* creation */
CVMCPUAddress* 
CVMCPUinit_Address(CVMCPUAddress *addr, CVMCPUMemSpecType type);

CVMCPUAddress* 
CVMCPUinit_Address_disp(CVMCPUAddress *addr, int disp, CVMCPUMemSpecType type);

CVMCPUAddress* 
CVMCPUinit_Address_base_disp(CVMCPUAddress *addr, CVMCPURegister base, 
			     int disp, CVMCPUMemSpecType type);

CVMCPUAddress* 
CVMCPUinit_Address_base_memspec(CVMCPUAddress *addr, CVMCPURegister base, 
				CVMCPUMemSpec *ms);

CVMCPUAddress* 
CVMCPUinit_Address_base_memspectoken(CVMCPUAddress *addr, CVMCPURegister base, 
				     CVMCPUMemSpecToken mst);

CVMCPUAddress* 
CVMCPUinit_Address_base_idx_scale_disp(CVMCPUAddress* addr, CVMCPURegister base,
				       CVMCPURegister index, CVMCPUScaleFactor scale, 
				       int disp, CVMCPUMemSpecType type);

void CVMX86emit_data(CVMJITCompilationContext* con, CVMInt32 data);

/* Helper functions for groups of instructions */
void CVMX86emit_arith_reg_imm8(CVMJITCompilationContext* con, int op1, int op2, CVMX86Register dst, int imm8);
void CVMX86emit_arith_reg_reg(CVMJITCompilationContext* con, int op1, int op2, CVMX86Register dst, CVMX86Register src);
void CVMX86emit_farith(CVMJITCompilationContext* con, int b1, int b2, int i);


/* Stack */
void CVMX86pushad(CVMJITCompilationContext* con);
void CVMX86popad(CVMJITCompilationContext* con);

void CVMX86pushfd(CVMJITCompilationContext* con);
void CVMX86popfd(CVMJITCompilationContext* con);

void CVMX86pushl_imm32(CVMJITCompilationContext* con, int imm32);
void CVMX86pushl_reg(CVMJITCompilationContext* con, CVMX86Register src);
void CVMX86pushl_mem(CVMJITCompilationContext* con, CVMX86Address src);

void CVMX86popl_reg(CVMJITCompilationContext* con, CVMX86Register dst);
void CVMX86popl_mem(CVMJITCompilationContext* con, CVMX86Address dst);

/* Instruction prefixes */
void CVMX86prefix(CVMJITCompilationContext* con, CVMX86Prefix p);

/* Moves */
void CVMX86movb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movb_reg_imm8(CVMJITCompilationContext* con, CVMX86Address dst, int imm8);
void CVMX86movb_reg_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);

void CVMX86movw_reg_imm16(CVMJITCompilationContext* con, CVMX86Address dst, int imm16);
void CVMX86movw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movw_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);


/*
   ================ movl   reg, operand =================
*/
void CVMX86movl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, 
			  int imm32);
void CVMX86movl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);
void CVMX86movl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);


/*
   ================ movl   mem, operand =================
*/
void CVMX86movl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, 
			  int imm32);

void CVMX86movl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);


void CVMX86movsxb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movsxb_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86movsxw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movsxw_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86movzxb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movzxb_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86movzxw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86movzxw_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

/* Conditional moves (P6 only) */
void CVMX86cmovl_reg_reg(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86Register dst, CVMX86Register src);
void CVMX86cmovl_reg_mem(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86Register dst, CVMX86Address src);

/* Arithmetics */
void CVMX86adcl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86adcl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86adcl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86addl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32);
void CVMX86addl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);
void CVMX86addl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86addl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86addl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86andl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86andl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86andl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86cmpb_mem_imm8(CVMJITCompilationContext* con, CVMX86Address dst, int imm8);
void CVMX86cmpl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32);
void CVMX86cmpl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86cmpl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);
void CVMX86cmpl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86cmpl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);

void CVMX86decb_reg(CVMJITCompilationContext* con, CVMX86Register dst);
void CVMX86decl_reg(CVMJITCompilationContext* con, CVMX86Register dst);
void CVMX86decl_mem(CVMJITCompilationContext* con, CVMX86Address dst);

void CVMX86idivl_reg(CVMJITCompilationContext* con, CVMX86Register src);
void CVMX86cdql(CVMJITCompilationContext* con);

void CVMX86imull_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86imull_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86imull_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);
void CVMX86imull_reg_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int value);

void CVMX86incl_reg(CVMJITCompilationContext* con, CVMX86Register dst);
void CVMX86incl_mem(CVMJITCompilationContext* con, CVMX86Address dst);

void CVMX86leal_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src, CVMBool fixed_length);

void CVMX86mull_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86mull_reg(CVMJITCompilationContext* con, CVMX86Register src);

void CVMX86negl_reg(CVMJITCompilationContext* con, CVMX86Register dst);

void CVMX86notl_reg(CVMJITCompilationContext* con, CVMX86Register dst);

void CVMX86orl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86orl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86orl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86rcll_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8);

void CVMX86sarl_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8);
void CVMX86sarl_reg(CVMJITCompilationContext* con, CVMX86Register dst);

void CVMX86sbbl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86sbbl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86sbbl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86shldl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);
void CVMX86shldl_reg_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int imm8);

void CVMX86shll_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8);
void CVMX86shll_reg(CVMJITCompilationContext* con, CVMX86Register dst);

void CVMX86shrdl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);
void CVMX86shrdl_reg_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int imm8);

void CVMX86shrl_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8);
void CVMX86shrl_reg(CVMJITCompilationContext* con, CVMX86Register dst);

void CVMX86subl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32);
void CVMX86subl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86subl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86subl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86testb_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8);
void CVMX86testl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86testl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

void CVMX86xaddl_reg_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src);

void CVMX86xorl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32);
void CVMX86xorl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src);
void CVMX86xorl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src);

/* Miscellaneous */
void CVMX86bswap_reg(CVMJITCompilationContext* con, CVMX86Register reg);
void CVMX86lock(CVMJITCompilationContext* con);
void CVMX86xchg_reg_mem(CVMJITCompilationContext* con, CVMX86Register reg, CVMX86Address adr);
void CVMX86cmpxchg_reg_mem(CVMJITCompilationContext* con, CVMX86Register reg, CVMX86Address adr);
void CVMX86hlt(CVMJITCompilationContext* con);
void CVMX86int3(CVMJITCompilationContext* con);
void CVMX86nop(CVMJITCompilationContext* con);
void CVMX86ret_imm16(CVMJITCompilationContext* con, int imm16);
void CVMX86set_byte_if_not_zero_reg(CVMJITCompilationContext* con, CVMX86Register dst); /* sets reg to 1 if not zero, otherwise 0 */
void CVMX86rep_move(CVMJITCompilationContext* con);
void CVMX86rep_set(CVMJITCompilationContext* con);
void CVMX86setb_reg(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86Register dst);
void CVMX86membar(CVMJITCompilationContext* con);		/* Serializing memory-fence */
void CVMX86cpuid(CVMJITCompilationContext* con);

/* Calls */
void CVMX86call_imm32(CVMJITCompilationContext* con, CVMX86address entry);
void CVMX86call_reg(CVMJITCompilationContext* con, CVMX86Register reg);
void CVMX86call_mem(CVMJITCompilationContext* con, CVMX86Address adr);

/* Jumps */
void CVMX86jmp_imm8(CVMJITCompilationContext* con, CVMX86address dst);
void CVMX86jmp_imm32(CVMJITCompilationContext* con, CVMX86address entry);
void CVMX86jmp_reg(CVMJITCompilationContext* con, CVMX86Register reg);
void CVMX86jmp_mem(CVMJITCompilationContext* con, CVMX86Address adr);

void CVMX86jcc_imm8(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86address dst);
void CVMX86jcc_imm32(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86address dst);

/* Floating-point operations */
void CVMX86fld1();
void CVMX86fldz();

void CVMX86fld_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fld_s_reg(CVMJITCompilationContext* con, int index);
void CVMX86fld_d_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fld_x_mem(CVMJITCompilationContext* con, CVMX86Address adr);  /* extended-precision (80-bit) format */

void CVMX86fst_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fst_d_mem(CVMJITCompilationContext* con, CVMX86Address adr);

void CVMX86fstp_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fstp_d_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fstp_d_reg(CVMJITCompilationContext* con, int index);
void CVMX86fstp_x_mem(CVMJITCompilationContext* con, CVMX86Address adr); /* extended-precision (80-bit) format */

void CVMX86fild_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fild_d_mem(CVMJITCompilationContext* con, CVMX86Address adr);

void CVMX86fist_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fistp_s_mem(CVMJITCompilationContext* con, CVMX86Address adr);
void CVMX86fistp_d_mem(CVMJITCompilationContext* con, CVMX86Address adr);

void CVMX86fabs(CVMJITCompilationContext* con);
void CVMX86fchs(CVMJITCompilationContext* con);

void CVMX86fcos(CVMJITCompilationContext* con);
void CVMX86fsin(CVMJITCompilationContext* con);
void CVMX86fsqrt(CVMJITCompilationContext* con);

void CVMX86fadd_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fadd_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fadd_reg(CVMJITCompilationContext* con, int i);
void CVMX86faddp_reg(CVMJITCompilationContext* con, int i /* = 1 */);

void CVMX86fsub_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fsub_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fsubr_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fsubr_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fsub_reg(CVMJITCompilationContext* con, int i);
void CVMX86fsubp_reg(CVMJITCompilationContext* con, int i /* = 1 */);
void CVMX86fsubrp_reg(CVMJITCompilationContext* con, int i /* = 1 */);

void CVMX86fmul_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fmul_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fmul_reg(CVMJITCompilationContext* con, int i);
void CVMX86fmulp_reg(CVMJITCompilationContext* con, int i /* = 1 */);

void CVMX86fdiv_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fdiv_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fdivr_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fdivr_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fdiv_reg(CVMJITCompilationContext* con, int i);
void CVMX86fdivp_reg(CVMJITCompilationContext* con, int i /* = 1 */);
void CVMX86fdivrp_reg(CVMJITCompilationContext* con, int i /* = 1 */);

void CVMX86fprem(CVMJITCompilationContext* con);
void CVMX86fprem1(CVMJITCompilationContext* con);

void CVMX86fxch_reg(CVMJITCompilationContext* con, int i /* = 1 */);
void CVMX86fincstp(CVMJITCompilationContext* con);
void CVMX86fdecstp(CVMJITCompilationContext* con);
void CVMX86ffree_reg(CVMJITCompilationContext* con, int i /* = 0 */);

void CVMX86fcomp_s_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fcomp_d_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fcompp(CVMJITCompilationContext* con);

void CVMX86fucomi_reg(CVMJITCompilationContext* con, int i /* = 1 */);
void CVMX86fucomip_reg(CVMJITCompilationContext* con, int i /* = 1 */);

void CVMX86ftst(CVMJITCompilationContext* con);
void CVMX86fnstsw_ax(CVMJITCompilationContext* con);
void CVMX86fwait(CVMJITCompilationContext* con);
void CVMX86finit(CVMJITCompilationContext* con);
void CVMX86fldcw_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86fnstcw_mem(CVMJITCompilationContext* con, CVMX86Address src);

void CVMX86fnsave_mem(CVMJITCompilationContext* con, CVMX86Address dst);
void CVMX86frstor_mem(CVMJITCompilationContext* con, CVMX86Address src);
void CVMX86sahf(CVMJITCompilationContext* con);

#ifdef CVM_JIT_USE_FP_HARDWARE

void CVMCPUemitInitFPMode( CVMJITCompilationContext* con);
void CVMX86free_float_regs(CVMJITCompilationContext* con);
#define CVMCPUfree_float_regs CVMX86free_float_regs

#endif /* CVM_JIT_USE_FP_HARDWARE */

/* multiplication of 64 bit signed integers on 32 bit CISC architectures */
void
CVMCPUemitIMUL64(CVMJITCompilationContext *con,
		 int dstRegID, 
		 CVMCPUAddress rhs_lo, 
		 CVMCPUAddress rhs_hi, 
		 CVMCPUAddress lhs_lo, 
		 CVMCPUAddress lhs_hi, 
		 int tmpRegID);

/**************************************************************
 * CPU C Call convention abstraction - The following are implementations of
 * calling convention support function prototypes required by the CISC emitter
 * porting layer. See jitemitter_cpu.c for more.
 **************************************************************/

/*
 * The glue code calling convention has 4 argument registers. To have
 * a smoother fit, we pretend to have 4 arg regs in x86's C calling
 * convention as well. The emitter for the actual call
 * (CVMCPUemitAbsoluteCall_CCode) will push them on the stack.
 *
 *
 * In Future versions we might have less argument registers and use
 * something like CVMRMpushResource, which will be similar to
 * CVMRMpinResource, but push the resource on the C stack.
 */

/* Purpose: Dynamically instantiates an instance of the CVMCPUCallContext. 
   (SW): don't need a context */
#define CVMCPUCCallnewContext(con) ((CVMCPUCallContext *)NULL)

/* Purpose: Gets the registers required by a C call.  These register could be
            altered by the call being made. 
   (SW): we don't use regs so it's the empty set */
#define CVMCPUCCALLgetRequired(con, argsRequiredSet) (CVMRM_EMPTY_SET)

#define CVMARMCCALLargSize(argType) \
    (((argType == CVM_TYPEID_LONG) || (argType == CVM_TYPEID_DOUBLE)) ? 2 : 1)

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. 
    We push the arguments, so nothing to do here */
#define CVMCPUCCALLinitArgs(con, callContext, irec, forTargetting)

/* Purpose: Gets the register targets for the specified argument. */
#define CVMCPUCCALLgetArgTarget(con, callContext, \
                                argType, argNo, argWordIndex) \
    ((argWordIndex + CVMARMCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) ? \
      (1U << (CVMCPU_ARG1_REG + argWordIndex)) :                           \
      CVMRM_ANY_SET)

/* Purpose: Relinquish a previously pinned arguments. */
#define CVMCPUCCALLrelinquishArg(con, callContext, arg, argType, argNo, \
                                 argWordIndex)                          \
    if (argWordIndex + CVMARMCCALLargSize(argType) <= CVMCPU_MAX_ARG_REGS) { \
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);              \
    }

/* Purpose: Releases any resources allocated in CVMCPUCCALLinitArgs().
   the emitter of the call will take care of this */
#define CVMCPUCCALLdestroyArgs(con, callContext, irec, forTargetting) \
    ((void)callContext)

/* Purpose: Emits a constantpool dump with a branch around it if needed. */
extern void
CVMX86emitConstantPoolDumpWithBranchAroundIfNeeded(
    CVMJITCompilationContext* con);

#endif /* INCLUDED_X86_JITCISCEMITTER_CPU_H */
