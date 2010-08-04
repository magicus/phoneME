/*
 *
 * @(#)jitemitter_cpu.c	1.6 06/10/04
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
#include "javavm/include/jit/jitarchemitter.h"
#include "javavm/include/jit/ccmcisc.h"
#include "javavm/include/jit/jitregman.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/jit/jitfixup.h"
#include "javavm/include/jit/jitstackmap.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/clib.h"
#include "javavm/include/float_arch.h"
#include "javavm/include/bag.h"

struct CVMCPUALURhsTokenStruct CVMCPUALURhsTokenStructConstZero = {CVMCPU_ALURHS_CONSTANT,
								   0,
								   CVMCPU_INVALID_REG};

/* ------------------- x86 assembler implementation ---------------------- */

#ifdef CVM_TRACE_JIT
/* some headers of functions the assembler uses for tracing */
static void printPC(CVMJITCompilationContext* con);
static void getRegName(char *buf, int regID);
static void getcc(char *buf, CVMX86Condition cc);
static void printAddress(CVMX86Address addr);
#endif /*  CVM_TRACE_JIT */

static int CVMX86RegEnc[16] = {0,2,3,1,4,5,6,7,8,9,10,11,12,13,14,15};

#define CVMX86is8bit(x) (-0x80 <= (x) && (x) < 0x80)
#define CVMX86isByte(x) (0 <= (x) && (x) < 0x100)
#define CVMX86isWord(x) (-0x8000 <= (x) && (x) < 0x8000)
#define CVMX86is_valid(reg)  (0 <= (int)reg && (int)reg <= CVMCPU_MAX_INTERESTING_REG)
#define CVMX86RegisterEncoding(reg) CVMX86RegEnc[reg]
#define CVMX86encoding(reg)  (CVMassert(CVMX86is_valid(reg) /* invalid register */), CVMX86RegisterEncoding(reg) )
/* only eax, ebx, ecx, and edx can be used as byte registers */
#define  CVMX86has_byte_register(reg)  (0 <= (int)reg && (int)reg <= CVMX86_ECX)
#define CVMX86isShiftCount(x) (0 <= (x) && (x) < 32)

CVMBool
CVMCPUAddress_uses(CVMCPUAddress addr, CVMCPURegister reg)
{
  return addr._base == reg || addr._index == reg;
}


CVMCPUAddress*
CVMCPUinit_Address(CVMCPUAddress *addr, CVMCPUMemSpecType type)
{
  addr->_base = CVMX86no_reg;
  addr->_index = CVMX86no_reg;
  addr->_scale = CVMX86no_scale;
  addr->_disp = 0;
  addr->_type = type;
  return addr;
}


CVMCPUAddress*
CVMCPUinit_Address_disp(CVMCPUAddress *addr, int disp, CVMCPUMemSpecType type)
{
  addr->_base = CVMX86no_reg;
  addr->_index = CVMX86no_reg;
  addr->_scale = CVMX86no_scale;
  addr->_disp = disp;
  addr->_type = type;
  return addr;
}


CVMCPUAddress*
CVMCPUinit_Address_base_disp(CVMCPUAddress *addr, CVMCPURegister base, int disp, 
			     CVMCPUMemSpecType type)
{
  addr->_base = base;
  addr->_index = CVMX86no_reg;
  addr->_scale = CVMX86no_scale;
  addr->_disp = disp;
  addr->_type = type;
  return addr;
}

CVMCPUAddress*
CVMCPUinit_Address_base_memspec(CVMX86Address *addr, CVMX86Register base, 
				CVMCPUMemSpec *ms)
{
  addr->_base = base;
  if (NULL != ms->offsetReg)
    addr->_index = CVMRMgetRegisterNumber(ms->offsetReg);
  else
    addr->_index = CVMX86no_reg;
  addr->_disp = ms->offsetValue;
  addr->_scale = ms->scale;
  addr->_type = ms->type;
  return addr;
}

CVMCPUAddress*
CVMCPUinit_Address_base_memspectoken(CVMCPUAddress *addr, CVMCPURegister base, 
				     CVMCPUMemSpecToken mst)
{
  addr->_base = base;
  addr->_index = mst->regid;
  addr->_disp = mst->offset;
  addr->_scale = mst->scale;
  addr->_type = mst->type;
  return addr;
}


CVMCPUAddress*
CVMCPUinit_Address_base_idx_scale_disp(CVMCPUAddress* addr, CVMCPURegister base, 
				       CVMCPURegister index, CVMCPUScaleFactor scale, 
				       int disp /* = 0 */, CVMCPUMemSpecType type)
{
  addr->_base = base;
  addr->_index = index;
  addr->_scale = scale;
  addr->_disp = disp;
  addr->_type = type;

  CVMassert(!CVMX86is_valid(index) == (scale == CVMX86no_scale) /* inconsistent address */ );
  return addr;
}


void CVMX86emit_byte(CVMJITCompilationContext* con, int x)
{
  CVMassert(CVMX86isByte(x) /* not a byte */);
  CVMJITcbufEmit1(con, x);
}

/* `word' is 16 bit in x86 language */
void CVMX86emit_word(CVMJITCompilationContext* con, int x)
{
  CVMassert(CVMX86isWord(x) /* not a word */);
  CVMJITcbufEmit2(con, x);
}

void CVMX86emit_long(CVMJITCompilationContext* con, CVMInt32 x)
{
  CVMJITcbufEmit4(con, x);
}

void CVMX86emit_data(CVMJITCompilationContext* con, CVMInt32 data) {
  /*
    CVMassert(imm32_operand == 0, "default format must be imm32 in this file");
    CVMassert(_inst_mark != NULL, "must be inside InstructionMark");
  */
  CVMX86emit_long(con, data);
}


void CVMX86emit_arith_reg_imm8(CVMJITCompilationContext* con, int op1, int op2,
			       CVMX86Register dst, int imm8) {
  /* CVMassert(dst->has_byte_register(), "must have byte register"); */
  CVMassert(CVMX86isByte(op1) && CVMX86isByte(op2) /* wrong opcode */);
  CVMassert(CVMX86isByte(imm8) /* not a byte */);
  CVMassert((op1 & 0x01) == 0 /* should be 8bit operation */);

  CVMX86emit_byte(con, op1);
  CVMX86emit_byte(con, op2 | CVMX86encoding(dst));
  CVMX86emit_byte(con, imm8);
}


static void CVMX86emit_arith_reg_imm32(CVMJITCompilationContext* con,
                                       int op1, int op2, CVMX86Register dst,
                                       int imm32                             ) {
  CVMassert(CVMX86isByte(op1) && CVMX86isByte(op2) /* wrong opcode */);
  CVMassert((op1 & 0x01) == 1 /* should be 32bit operation */);
  CVMassert((op1 & 0x02) == 0 /* sign-extension bit should not be set */);

  if (CVMX86is8bit(imm32)) {
    CVMX86emit_byte(con, op1 | 0x02); // set sign bit
    CVMX86emit_byte(con, op2 | CVMX86encoding(dst));
    CVMX86emit_byte(con, imm32 & 0xFF);
  } else {
    CVMX86emit_byte(con, op1);
    CVMX86emit_byte(con, op2 | CVMX86encoding(dst));
    CVMX86emit_long(con, imm32);
  }
}


void CVMX86emit_arith_reg_reg(CVMJITCompilationContext* con, int op1,
			      int op2, CVMX86Register dst, CVMX86Register src) {
  CVMassert(CVMX86isByte(op1) && CVMX86isByte(op2) /* wrong opcode */);
  CVMX86emit_byte(con, op1);
  CVMX86emit_byte(con, op2 | CVMX86encoding(dst) << 3 | CVMX86encoding(src));
}


static void CVMX86emit_operand(CVMJITCompilationContext* con,
                               CVMX86Register reg, CVMX86Register base,
                               CVMX86Register index,
                               CVMX86ScaleFactor scale, int disp,
                               CVMBool fixed_length                     ) {
  if (CVMX86is_valid(base)) {
    if (CVMX86is_valid(index)) {
      CVMassert(scale != CVMX86no_scale /* inconsistent address */);
      /* [base + index*scale + disp] */
      if (disp == 0 && fixed_length != CVM_TRUE) {
	/* [base + index*scale]
	   [00 reg 100][ss index base] */
	CVMassert(index != CVMX86esp && base != CVMX86ebp /* illegal addressing mode */);
	CVMX86emit_byte(con, 0x04 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, scale << 6 | CVMX86encoding(index) << 3 | CVMX86encoding(base));
      } else if (CVMX86is8bit(disp) && fixed_length != CVM_TRUE) {
	/* [base + index*scale + imm8]
	   [01 reg 100][ss index base] imm8 */
	CVMassert(index != CVMX86esp /* illegal addressing mode */);
	CVMX86emit_byte(con, 0x44 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, scale << 6 | CVMX86encoding(index) << 3 | CVMX86encoding(base));
	CVMX86emit_byte(con, disp & 0xFF);
      } else {
	/* [base + index*scale + imm32]
	   [10 reg 100][ss index base] imm32 */
	CVMassert(index != CVMX86esp /* illegal addressing mode */);
	CVMX86emit_byte(con, 0x84 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, scale << 6 | CVMX86encoding(index) << 3 | CVMX86encoding(base));
	CVMX86emit_data(con, disp);
      }
    } else if (base == CVMX86esp) {
      /* [esp + disp] */
      if (disp == 0 && fixed_length != CVM_TRUE) {
	/* [esp]
	   [00 reg 100][00 100 100] */
	CVMX86emit_byte(con, 0x04 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, 0x24);
      } else if (CVMX86is8bit(disp) && fixed_length != CVM_TRUE) {
	/* [esp + imm8]
	   [01 reg 100][00 100 100] imm8 */
	CVMX86emit_byte(con, 0x44 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, 0x24);
	CVMX86emit_byte(con, disp & 0xFF);
      } else {
	/* [esp + imm32]
	   [10 reg 100][00 100 100] imm32 */
	CVMX86emit_byte(con, 0x84 | CVMX86encoding(reg) << 3);
	CVMX86emit_byte(con, 0x24);
	CVMX86emit_data(con, disp);
      }
    } else {
      // [base + disp]
      CVMassert(base != CVMX86esp /* "illegal addressing mode" */);
      if (disp == 0 && base != CVMX86ebp && fixed_length != CVM_TRUE) {
	/* [base]
	   [00 reg base] */
	CVMassert(base != CVMX86ebp /* illegal addressing mode */);
	CVMX86emit_byte(con, 0x00 | CVMX86encoding(reg) << 3 | CVMX86encoding(base));
      } else if (CVMX86is8bit(disp) && fixed_length != CVM_TRUE) {
	/* [base + imm8]
	   [01 reg base] imm8 */
	CVMX86emit_byte(con, 0x40 | CVMX86encoding(reg) << 3 | CVMX86encoding(base));
	CVMX86emit_byte(con, disp & 0xFF);
      } else {
	/* [base + imm32]
	   [10 reg base] imm32 */
	CVMX86emit_byte(con, 0x80 | CVMX86encoding(reg) << 3 | CVMX86encoding(base));
	CVMX86emit_data(con, disp);
      }
    }
  } else {
    if (CVMX86is_valid(index)) {
      CVMassert(scale != CVMX86no_scale /* inconsistent address */);
      /* [index*scale + disp]
	 [00 reg 100][ss index 101] imm32 */
      CVMassert(index != CVMX86esp /* illegal addressing mode */);
      CVMX86emit_byte(con, 0x04 | CVMX86encoding(reg) << 3);
      CVMX86emit_byte(con, scale << 6 | CVMX86encoding(index) << 3 | 0x05);
      CVMX86emit_data(con, disp);
    } else {
      /* [disp]
	 [00 reg 101] imm32 */
      CVMX86emit_byte(con, 0x05 | CVMX86encoding(reg) << 3);
      CVMX86emit_data(con, disp);
    }
  }
}


static void CVMX86emit_operand_wrapper(CVMJITCompilationContext* con,
                                       CVMX86Register reg,
                                       CVMX86Address adr,
                                       CVMBool fixed_length           ) {
  CVMX86emit_operand(con, reg, adr._base, adr._index,
                     adr._scale, adr._disp, fixed_length);
}


void CVMX86emit_farith(CVMJITCompilationContext* con, int b1, int b2, int i) {
  CVMassert(CVMX86isByte(b1) && CVMX86isByte(b2) /* wrong opcode */);
  CVMassert(0 <= i &&  i < 8 /* illegal stack offset */);
  CVMX86emit_byte(con, b1);
  CVMX86emit_byte(con, b2 + i);
}


void CVMX86pushad(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	pushad");
  });  

  CVMX86emit_byte(con, 0x60);

  CVMJITdumpCodegenComments(con);
}

void CVMX86popad(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	popad");
  });  

  CVMX86emit_byte(con, 0x61);

  CVMJITdumpCodegenComments(con);
}

void CVMX86pushfd(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	pushfd");
  });  

  CVMX86emit_byte(con, 0x9C);

  CVMJITdumpCodegenComments(con);
}

void CVMX86popfd(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	popfd");
  });  

  CVMX86emit_byte(con, 0x9D);

  CVMJITdumpCodegenComments(con);
}


void CVMX86pushl_imm32(CVMJITCompilationContext* con, int imm32) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	pushl $%d", imm32);
  });  

  CVMX86emit_byte(con, 0x68);
  CVMX86emit_data(con, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86pushl_reg(CVMJITCompilationContext* con, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	pushl	%s", srcRegBuf);
  });  

  CVMX86emit_byte(con, 0x50 | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86pushl_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	pushl");
  });  

  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_operand_wrapper(con, CVMX86esi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86popl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	popl	%s", dstRegBuf);
  });  

  CVMX86emit_byte(con, 0x58 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86popl_mem(CVMJITCompilationContext* con, CVMX86Address dst) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	popl");
  });  



  CVMX86emit_byte(con, 0x8F);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86prefix(CVMJITCompilationContext* con, CVMX86Prefix p) {
  CVMX86emit_byte(con, p);
}


/* CVMX86prefixMode:
   emits opcode to switch between 16 bit and 32 bit mode
*/
static void CVMX86prefixMode( CVMJITCompilationContext * con ) {
  CVMX86prefix( con, 0x66 );
}   /* CVMX86prefixMode */


void CVMX86movb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  CVMassert(CVMX86has_byte_register(dst) /* must have byte register */);
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movb	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x8A);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movb_mem_imm8(CVMJITCompilationContext* con, CVMX86Address dst, int imm8) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	movb	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm8);
  });  


  CVMX86emit_byte(con, 0xC6);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);
  CVMX86emit_byte(con, imm8);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movb_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  CVMassert(CVMX86has_byte_register(src) /* must have byte register */ );
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	movb	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x88);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movw_mem_imm16(CVMJITCompilationContext* con, CVMX86Address dst, int imm16) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	movb	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm16);
  });  


  CVMX86prefixMode( con );  // switch to 16 bit mode
  CVMX86emit_byte(con, 0xC7);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);
  CVMX86emit_word(con, imm16);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movw	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86prefixMode( con );  // switch to 16 bit mode
  CVMX86emit_byte(con, 0x8B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movw_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	movb	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86prefixMode( con );   // switch to 16 bit mode
  CVMX86emit_byte(con, 0x89);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


static void movl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, 
			   int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	movl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_byte(con, 0xB8 | CVMX86encoding(dst));
  CVMX86emit_long(con, imm32);

  CVMJITdumpCodegenComments(con);
}   /* movl_reg_imm32 */


void CVMX86movl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, 
			  int imm32) {

  movl_reg_imm32( con, dst, imm32 );
}


void CVMX86movl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x8B);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86movl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x8B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


static void movl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, 
                           int imm32) {

   /* InstructionMark im(this); */
   CVMtraceJITCodegenExec({
      printPC(con);
      CVMconsolePrintf("	movl	");
      printAddress(dst);
      CVMconsolePrintf(", $%d", imm32);
   });  


  CVMX86emit_byte(con, 0xC7);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);
  CVMX86emit_long(con, imm32);
  
  CVMJITdumpCodegenComments(con);
}   /* movl_mem_imm32 */


void CVMX86movl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, 
			  int imm32) {

  movl_mem_imm32( con, dst, imm32 );
}


void CVMX86movl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	movl	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x89);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movsxb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movsxb	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xBE);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movsxb_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMassert(CVMX86has_byte_register(src) /* must have byte register */);

  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movsxb	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xBE);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86movsxw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movsxw	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xBF);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);
}


void CVMX86movsxw_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movsxw	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xBF);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movzxb_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movszxb	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xB6);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movzxb_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movzxb	%s, %s", destRegBuf, srcRegBuf);
  });  

  CVMassert(CVMX86has_byte_register(src) /* must have byte register */);
  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xB6);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86movzxw_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movzxw	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xB7);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86movzxw_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	movzxw	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xB7);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmovl_reg_reg(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86Register dst,
			 CVMX86Register src) {
  /* guarantee(VM_Version::supports_cmov(), "illegal instruction"); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	cmovl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0x40 | cc);
  CVMX86emit_byte(con, 0xC0 | (CVMX86encoding(dst) << 3) | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmovl_reg_mem(CVMJITCompilationContext* con, CVMX86Condition cc, CVMX86Register dst, CVMX86Address src) {
  /* guarantee(VM_Version::supports_cmov(), "illegal instruction"); */
  /* The code below seems to be wrong - however the manual is inconclusive
     do not use for now (remember to enable all callers when fixing this) */
  /* Unimplemented(); */
  CVMassert(0);
  /* wrong bytes? */
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	cmovl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0x40 | cc);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86adcl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	adcl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xD0, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86adcl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	adcl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x13);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86adcl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	adcl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x13, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86addl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	addl	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm32);
  });  


  CVMX86emit_byte(con, 0x81);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);
  CVMX86emit_long(con, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86addl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	addl	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x01);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86addl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	addl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xC0, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86addl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	addl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x03);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86addl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	addl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x03, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86andl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	andl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xE0, dst, imm32);

  CVMJITdumpCodegenComments(con);
}

void CVMX86andl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	andl	%d, $%d", dst, imm32);
  });  


  CVMX86emit_byte(con,0x81);
  CVMX86emit_operand_wrapper(con,CVMX86esp,dst, CVM_FALSE);
  CVMX86emit_long(con,imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86andl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	andl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x23);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86andl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	andl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x23, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmpb_mem_imm8(CVMJITCompilationContext* con, CVMX86Address dst, int imm8) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	cmpb	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm8);
  });  


  CVMX86emit_byte(con, 0x80);
  CVMX86emit_operand_wrapper(con, CVMX86edi, dst, CVM_FALSE);
  CVMX86emit_byte(con, imm8);

  CVMJITdumpCodegenComments(con);
}

void CVMX86cmpw_mem_imm16(CVMJITCompilationContext* con, CVMX86Address dst, int imm16) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	cmpw	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm16);
  });

  CVMX86prefixMode( con );   // switch to 16 bit mode
  CVMX86emit_byte(con, 0x81);
  CVMX86emit_operand_wrapper(con, CVMX86edi, dst, CVM_FALSE);
  CVMX86emit_word(con, imm16);

  CVMJITdumpCodegenComments(con);
}

void CVMX86cmpl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	cmpl	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm32);
  });  


  CVMX86emit_byte(con, 0x81);
  CVMX86emit_operand_wrapper(con, CVMX86edi, dst, CVM_FALSE);
  CVMX86emit_long(con, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmpl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	cmpl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xF8, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmpl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	cmpl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x3B, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86cmpl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address  src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	cmpl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x3B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86cmpl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	cmpl	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x39);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86decb_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMassert(CVMX86has_byte_register(dst) /* must have byte register */);

  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	decb	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xFE);
  CVMX86emit_byte(con, 0xC8 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86decl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	decl	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0x48 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86decl_mem(CVMJITCompilationContext* con, CVMX86Address dst) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	decl	");
    printAddress(dst);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_operand_wrapper(con, CVMX86ecx, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86idivl_reg(CVMJITCompilationContext* con, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	idivl	%s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0xF7);
  CVMX86emit_byte(con, 0xF8 | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86cdql(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	cdql");
  });  


  CVMX86emit_byte(con, 0x99);

  CVMJITdumpCodegenComments(con);
}

void CVMX86imull_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	imull	");
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0xF7);
  /* TODO HS: verify */
  CVMX86emit_operand_wrapper(con, 1, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86imull_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	imull	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xAF);
  /* TODO HS: verify */
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86imull_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	imull	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xAF);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(dst) << 3 | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86imull_reg_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int value) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	imull	%s, %s, $%d", destRegBuf, srcRegBuf, value);
  });  


  if (CVMX86is8bit(value)) {
    CVMX86emit_byte(con, 0x6B);
    CVMX86emit_byte(con, 0xC0 | CVMX86encoding(dst) << 3 | CVMX86encoding(src));
    CVMX86emit_byte(con, value & 0xFF /* strip sign bits */);
  } else {
    CVMX86emit_byte(con, 0x69);
    CVMX86emit_byte(con, 0xC0 | CVMX86encoding(dst) << 3 | CVMX86encoding(src));
    CVMX86emit_long(con, value);
  }

  CVMJITdumpCodegenComments(con);
}


void CVMX86incl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	incl	%s", dstRegBuf);
  });  


  CVMX86emit_byte(con, 0x40 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86incl_mem(CVMJITCompilationContext* con, CVMX86Address dst) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	incl	");
    printAddress(dst);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_operand_wrapper(con, CVMX86eax, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86leal_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src, CVMBool fixed_length) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	leal	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x8D);
  CVMX86emit_operand_wrapper(con, dst, src, fixed_length);

  CVMJITdumpCodegenComments(con);
}


void CVMX86mull_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	mull	");
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0xF7);
  CVMX86emit_operand_wrapper(con, CVMX86esp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86mull_reg(CVMJITCompilationContext* con, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	mull	%s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0xF7);
  CVMX86emit_byte(con, 0xE0 | CVMX86encoding(src));

  CVMJITdumpCodegenComments(con);
}


void CVMX86negl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	negl	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xF7);
  CVMX86emit_byte(con, 0xD8 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86notl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	notl	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xF7);
  CVMX86emit_byte(con, 0xD0 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}

void CVMX86orl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	orll	%s, $%d", dstRegBuf, imm32);
  });  


  
  CVMX86emit_arith_reg_imm32(con, 0x81, 0xC8, dst, imm32);

  CVMJITdumpCodegenComments(con);
}

void CVMX86orl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32) {
    CVMtraceJITCodegenExec({
      printPC(con);
      CVMconsolePrintf("	orll	%d, $%d", dst, imm32);
    });  


  CVMX86emit_byte(con,0x81);
  CVMX86emit_operand_wrapper(con,CVMX86ecx,dst, CVM_FALSE);
  CVMX86emit_long(con,imm32);

  CVMJITdumpCodegenComments(con);
}

void CVMX86orl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	orl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x0B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86orl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	orl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x0B, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86rcll_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);

  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	orl	%s, $%d", dstRegBuf, imm8);
  });  


  if (imm8 == 1) {
    CVMX86emit_byte(con, 0xD1);
    CVMX86emit_byte(con, 0xD0 | CVMX86encoding(dst));
  } else {
    CVMX86emit_byte(con, 0xC1);
    CVMX86emit_byte(con, 0xD0 | CVMX86encoding(dst));
    CVMX86emit_byte(con, imm8);
  }

  CVMJITdumpCodegenComments(con);
}


void CVMX86sarl_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);

  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	sarl	%s, $%d", dstRegBuf, imm8);
  });  


  if (imm8 == 1) {
    CVMX86emit_byte(con, 0xD1);
    CVMX86emit_byte(con, 0xF8 | CVMX86encoding(dst));
  } else {
    CVMX86emit_byte(con, 0xC1);
    CVMX86emit_byte(con, 0xF8 | CVMX86encoding(dst));
    CVMX86emit_byte(con, imm8);
  }

  CVMJITdumpCodegenComments(con);
}


void CVMX86sarl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	sarll	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xD3);
  CVMX86emit_byte(con, 0xF8 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86sbbl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	sbbl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xD8, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86sbbl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	sbbl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x1B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86sbbl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	sbbl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x1B, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86shldl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	shldl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xA5);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(src) << 3 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}

void CVMX86shldl_reg_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);
  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xA4);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(src) << 3 | CVMX86encoding(dst));
  CVMX86emit_byte(con, imm8);
}


void CVMX86shll_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);

  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	shll	%s, $%d", dstRegBuf, imm8);
  });  


  if (imm8 == 1 ) {
    CVMX86emit_byte(con, 0xD1);
    CVMX86emit_byte(con, 0xE0 | CVMX86encoding(dst));
  } else {
    CVMX86emit_byte(con, 0xC1);
    CVMX86emit_byte(con, 0xE0 | CVMX86encoding(dst));
    CVMX86emit_byte(con, imm8);
  }

  CVMJITdumpCodegenComments(con);
}

void CVMX86shll_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	shll	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xD3);
  CVMX86emit_byte(con, 0xE0 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86shrdl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	shrdl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xAD);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(src) << 3 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}

void CVMX86shrdl_reg_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);
  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xAC);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(src) << 3 | CVMX86encoding(dst));
  CVMX86emit_byte(con, imm8);
}

void CVMX86shrl_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8) {
  CVMassert(CVMX86isShiftCount(imm8) /* illegal shift count */);

  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	shrl	%s, $%d", dstRegBuf, imm8);
  });  


  CVMX86emit_byte(con, 0xC1);
  CVMX86emit_byte(con, 0xE8 | CVMX86encoding(dst));
  CVMX86emit_byte(con, imm8);

  CVMJITdumpCodegenComments(con);
}


void CVMX86shrl_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	shrl	%s", destRegBuf);
  });  


  CVMX86emit_byte(con, 0xD3);
  CVMX86emit_byte(con, 0xE8 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86subl_mem_imm32(CVMJITCompilationContext* con, CVMX86Address dst, int imm32) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	subl	");
    printAddress(dst);
    CVMconsolePrintf(", $%d", imm32);
  });  


  if (CVMX86is8bit(imm32)) {
    /* InstructionMark im(this); */
    CVMX86emit_byte(con, 0x83);
    CVMX86emit_operand_wrapper(con, CVMX86ebp, dst, CVM_FALSE);
    CVMX86emit_byte(con, imm32 & 0xFF);
  } else {
    /* InstructionMark im(this); */
    CVMX86emit_byte(con, 0x81);
    CVMX86emit_operand_wrapper(con, CVMX86ebp, dst, CVM_FALSE);
    CVMX86emit_long(con, imm32);
  }

  CVMJITdumpCodegenComments(con);
}


void CVMX86subl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	subl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xE8, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86subl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	subl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMtraceJITCodegen(("0x"));
  CVMX86emit_byte(con, 0x2B);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86subl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	subl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x2B, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86testb_reg_imm8(CVMJITCompilationContext* con, CVMX86Register dst, int imm8) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	testb	%s, $%d", dstRegBuf, imm8);
  });  


  CVMassert(CVMX86has_byte_register(dst) /* must have byte register */);
  CVMX86emit_arith_reg_imm8(con, 0xF6, 0xC0, dst, imm8);

  CVMJITdumpCodegenComments(con);
}


void CVMX86testl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  /* not using emit_arith because test */
  /* doesn't support sign-extension of */
  /* 8bit operands */
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	testl	%s, $%d", dstRegBuf, imm32);
  });  


  if (CVMX86encoding(dst) == 0) {
    CVMX86emit_byte(con, 0xA9);
  } else {
    CVMX86emit_byte(con, 0xF7);
    CVMX86emit_byte(con, 0xC0 | CVMX86encoding(dst));
  }
  CVMX86emit_long(con, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86testl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	testl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x85, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}

void CVMX86xaddl_mem_reg(CVMJITCompilationContext* con, CVMX86Address dst, CVMX86Register src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    CVMconsolePrintf("	xaddl	");
    printAddress(dst);
    CVMconsolePrintf(", %s", srcRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xC1);
  CVMX86emit_operand_wrapper(con, src, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86xorl_reg_imm32(CVMJITCompilationContext* con, CVMX86Register dst, int imm32) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	xorl	%s, $%d", dstRegBuf, imm32);
  });  


  CVMX86emit_arith_reg_imm32(con, 0x81, 0xF0, dst, imm32);

  CVMJITdumpCodegenComments(con);
}


void CVMX86xorl_reg_mem(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Address src) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	xorl	%s, ", destRegBuf);
    printAddress(src);
  });  


  CVMX86emit_byte(con, 0x33);
  CVMX86emit_operand_wrapper(con, dst, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86xorl_reg_reg(CVMJITCompilationContext* con, CVMX86Register dst, CVMX86Register src) {
  CVMtraceJITCodegenExec({
    char srcRegBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(srcRegBuf, src);
    getRegName(destRegBuf, dst);
    CVMconsolePrintf("	xorl	%s, %s", destRegBuf, srcRegBuf);
  });  


  CVMX86emit_arith_reg_reg(con, 0x33, 0xC0, dst, src);

  CVMJITdumpCodegenComments(con);
}


void CVMX86bswap_reg(CVMJITCompilationContext* con, CVMX86Register reg) {
  CVMtraceJITCodegenExec({
    char regBuf[30];
    printPC(con);
    getRegName(regBuf, reg);
    CVMconsolePrintf("	swap	%s", regBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xC8 | CVMX86encoding(reg));

  CVMJITdumpCodegenComments(con);
}


void CVMX86lock(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	lock");
  });  


  CVMX86emit_byte(con, 0xF0);

  CVMJITdumpCodegenComments(con);
}


void CVMX86xchg_reg_mem(CVMJITCompilationContext* con, CVMX86Register reg, CVMX86Address adr) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char regBuf[30];
    printPC(con);
    getRegName(regBuf, reg);
    CVMconsolePrintf("	xchg	%s, ", regBuf);
    printAddress(adr);
  });  


  CVMX86emit_byte(con, 0x87);
  CVMX86emit_operand_wrapper(con, reg, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


/* The 32-bit cmpxchg compares the value at adr with the contents of CVMX86eax, */
/* and loads reg into adr if so; otherwise, adr is loaded into CVMX86eax.  The */
/* ZF is set if the compared values were equal, and cleared otherwise. */
void CVMX86cmpxchg_reg_mem(CVMJITCompilationContext* con, CVMX86Register reg, CVMX86Address adr) {
  /* InstructionMark im(this); */
  CVMtraceJITCodegenExec({
    char regBuf[30];
    printPC(con);
    getRegName(regBuf, reg);
    CVMconsolePrintf("	cmpxchg	%s, ", regBuf);
    printAddress(adr);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0xB1);
  CVMX86emit_operand_wrapper(con, reg, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86hlt(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	hlt");
  });  


  CVMX86emit_byte(con, 0xF4);

  CVMJITdumpCodegenComments(con);
}


void CVMX86nop(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	nop");
  });  


  CVMX86emit_byte(con, 0x90);

  CVMJITdumpCodegenComments(con);
}

void CVMX86ret_imm16(CVMJITCompilationContext* con, int imm16) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	ret	$%d", imm16);
  });  


  if (imm16 == 0) {
    CVMX86emit_byte(con, 0xC3);
  } else {
    CVMX86emit_byte(con, 0xC2);
    CVMX86emit_word(con, imm16);
  }

  CVMJITdumpCodegenComments(con);
}


void CVMX86set_byte_if_not_zero_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  /* SVMC_JIT rr TODO: what's that for? */
  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0x95);
  CVMX86emit_byte(con, 0xE0 | CVMX86encoding(dst));
}


/* copies data from [esi] to [edi] using ecx double words (m32) */
void CVMX86rep_move(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	rep mov");
  });  


  CVMX86emit_byte(con, 0xF3);
  CVMX86emit_byte(con, 0xA5);

  CVMJITdumpCodegenComments(con);
}


/* sets ecx double words (m32) with eax value at [edi] */
void CVMX86rep_set(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	rep set");
  });  


  CVMX86emit_byte(con, 0xF3);
  CVMX86emit_byte(con, 0xAB);

  CVMJITdumpCodegenComments(con);
}


void CVMX86setb_reg(CVMJITCompilationContext* con,
                    CVMX86Condition cc, CVMX86Register dst) {
  CVMassert(0 <= (int)cc && cc < 16 /* illegal cc */);

  CVMtraceJITCodegenExec({
    char ccBuf[30];
    char destRegBuf[30];
    printPC(con);
    getRegName(destRegBuf, dst);
    getcc(ccBuf, cc);
    CVMconsolePrintf("	setb%s	%s", ccBuf, destRegBuf);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0x90 | cc);
  CVMX86emit_byte(con, 0xC0 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}

/* Serializes memory. */
void CVMX86membar(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	membar");
  });  


  CVMX86emit_byte(con,  0x0F );		/* MFENCE; faster blows no regs */
  CVMX86emit_byte(con,  0xAE );
  CVMX86emit_byte(con,  0xF8 );

  CVMJITdumpCodegenComments(con);
}


void CVMX86call_imm32(CVMJITCompilationContext* con, CVMX86address entry) {
  CVMassert(entry != NULL /* call most probably wrong */);
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	call	$0x%x", entry);
  });  


  CVMX86emit_byte(con, 0xE8);
  CVMX86emit_data(con, (int)entry - ((int)con->curPhysicalPC /*_code_pos*/ + sizeof(long)));

  CVMJITdumpCodegenComments(con);
}


void CVMX86call_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	call	%s", dstRegBuf);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_byte(con, 0xD0 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86call_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	call	");
    printAddress(adr);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_operand_wrapper(con, CVMX86edx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86jmp_imm8(CVMJITCompilationContext* con, CVMX86address dst) {
   CVMInt32 offset = (int)dst - (int)CVMJITcbufGetPhysicalPC(con) - 2 /* size of jmp_imm8 */;
   CVMassert(dst != NULL /* jcc most probably wrong */);
   CVMassert(-128 <= offset && offset < 128 /* not a signed 8 bit value */);
   CVMtraceJITCodegenExec({
      printPC(con);
      CVMconsolePrintf("	jmp_imm8	$0x%x", dst);
   });  


   CVMX86emit_byte(con, 0xEB);
   CVMX86emit_byte(con, (unsigned char) offset);
   CVMJITdumpCodegenComments(con);
}


void CVMX86jmp_imm32(CVMJITCompilationContext* con, CVMX86address entry) {
  CVMassert(entry != NULL /* jmp most probably wrong */);
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	jmp	$0x%x", entry);
  });  

  CVMX86emit_byte(con, 0xE9);
  CVMX86emit_data(con, (int)entry - ((int)con->curPhysicalPC/*_code_pos*/ + sizeof(long)));

  CVMJITdumpCodegenComments(con);
}


static void
CVMX86patch_jmp_imm32(CVMUint32 *instructionPtr, CVMX86address dst) {
  CVMUint32 *dstField = (CVMUint32*)
    (((CVMUint32) instructionPtr) + 1);

  *dstField =  (int)dst - ((int)dstField + sizeof(long));
}


void CVMX86jmp_reg(CVMJITCompilationContext* con, CVMX86Register dst) {
  CVMtraceJITCodegenExec({
    char dstRegBuf[30];
    printPC(con);
    getRegName(dstRegBuf, dst);
    CVMconsolePrintf("	jmp	%s", dstRegBuf);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_byte(con, 0xE0 | CVMX86encoding(dst));

  CVMJITdumpCodegenComments(con);
}


void CVMX86jmp_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	jmp	");
    printAddress(adr);
  });  


  CVMX86emit_byte(con, 0xFF);
  CVMX86emit_operand_wrapper(con, CVMX86esp, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86jcc_imm8(CVMJITCompilationContext* con,
                    CVMX86Condition cc, CVMX86address dst) {
    CVMInt32 offset = (int)dst - (int)con->curPhysicalPC - 2 /* size of jcc_imm8 */;
    CVMassert((0 <= (int)cc) && (cc < 16) /* illegal cc */);
    CVMassert(dst != NULL /* jcc most probably wrong */);
    CVMassert(-128 <= offset && offset < 128 /* not a signed 8 bit value */);
    CVMtraceJITCodegenExec({
	char ccBuf[30];
	printPC(con);
	getcc(ccBuf, cc);
	CVMconsolePrintf("	j%s	$0x%x", ccBuf, dst);
    });  


    CVMX86emit_byte(con, 0x70 | cc);
    CVMX86emit_byte(con, (unsigned char)offset);
    CVMJITdumpCodegenComments(con);
}


void CVMX86jcc_imm32(CVMJITCompilationContext* con, 
                     CVMX86Condition cc, CVMX86address dst) {
  CVMUint32 offset = (int)dst - (int)con->curPhysicalPC - 6 /* size of jcc_imm32 */;
  CVMassert((0 <= (int)cc) && (cc < 16) /* illegal cc */);
  /* 0000 1111 1000 tttn #32-bit disp */
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    char ccBuf[30];
    printPC(con);
    getcc(ccBuf, cc);
    CVMconsolePrintf("	j%s	$0x%x", ccBuf, dst);
  });  


  CVMX86emit_byte(con, 0x0F);
  CVMX86emit_byte(con, 0x80 | cc);
  CVMX86emit_data(con, offset);

  CVMJITdumpCodegenComments(con);
}

#if 0
static void
CVMX86patch_jcc_imm8(CVMUint32* instructionPtr, CVMX86address dst) {
    CVMUint8* dstField = (CVMUint8*)(((CVMUint32)instructionPtr) + 1);

    *dstField = (int)dst - ((int)dstField + sizeof(char));
}
#endif

static void
CVMX86patch_jcc_imm32(CVMUint32 *instructionPtr, CVMX86address dst) {
  CVMUint32 *dstField = (CVMUint32*)
    (((CVMUint32) instructionPtr) + 2);

  *dstField =  (int)dst - ((int)dstField + sizeof(long));
}

static void
CVMX86patch_call_imm32(CVMUint32 *instructionPtr, CVMX86address dst) {
  CVMUint32 *dstField = (CVMUint32*)
    (((CVMUint32) instructionPtr) + 1);

  *dstField =  (int)dst - ((int)dstField + sizeof(long));
}

static void
CVMX86patch_fld_s_mem(CVMUint32 *instructionPtr, CVMX86address addr) {
  CVMUint32 *addrfield = (CVMUint32*)((char*)instructionPtr + 2);
  *addrfield = (int)addr;
}

static void
CVMX86patch_fld_d_mem(CVMUint32 *instructionPtr, CVMX86address addr) {
  CVMUint32 *addrfield = (CVMUint32*)((char*)instructionPtr + 2);
  *addrfield = (int)addr;
}


#ifdef CVM_JIT_USE_FP_HARDWARE
/* FPU instructions */

void CVMX86fld1(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fld1");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xE8);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fldz(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fldz");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xEE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fld_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fld_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_operand_wrapper(con, CVMX86eax, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fld_s_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fld_s	st(%d)", index);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_farith(con, 0xD9, 0xC0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fld_d_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fld_d	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDD);
  CVMX86emit_operand_wrapper(con, CVMX86eax, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fld_x_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fld_x	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_operand_wrapper(con, CVMX86ebp, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fst_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fst_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_operand_wrapper(con, CVMX86edx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fst_d_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fst_d	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDD);
  CVMX86emit_operand_wrapper(con, CVMX86edx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fstp_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fstp_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_operand_wrapper(con, CVMX86ebx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fstp_d_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fstp_d	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDD);
  CVMX86emit_operand_wrapper(con, CVMX86ebx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fstp_x_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fstp_x	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_operand_wrapper(con, CVMX86edi, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fstp_d_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fstp_d	st(%d)", index);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_farith(con, 0xDD, 0xD8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fild_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fild_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_operand_wrapper(con, CVMX86eax, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fild_d_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fild_d	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDF);
  CVMX86emit_operand_wrapper(con, CVMX86ebp, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fistp_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fistp_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_operand_wrapper(con, CVMX86ebx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fistp_d_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fistp_d	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDF);
  CVMX86emit_operand_wrapper(con, CVMX86edi, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fist_s_mem(CVMJITCompilationContext* con, CVMX86Address adr) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fist_s	");
    printAddress(adr);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_operand_wrapper(con, CVMX86edx, adr, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fabs(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fabs");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xE1);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsin(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsin");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xFE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fcos(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fcos");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xFF);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsqrt(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsqrt");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xFA);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fchs(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fchs");
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xE0);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fadd_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fadd_s	");
    printAddress(src);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86eax, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fadd_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fadd_d	");
    printAddress(src);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86eax, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fadd_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fadd_d	st(%d)", index);
  });  
  CVMassert(con->target.usesFPU);


  CVMX86emit_farith(con, 0xD8, 0xC0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsub_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsub_d	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86esp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsub_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsub_s	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86esp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsubr_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsubr_s	");
    printAddress(src);
  }); 
  CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86ebp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsubr_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsubr_d	");
    printAddress(src);
  }); 
  CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86ebp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fmul_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fmul_s	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86ecx, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fmul_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fmul_d	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86ecx, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fmul_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fmul	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xD8, 0xC8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdiv_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdiv_s	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86esi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdiv_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdiv_d	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86esi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdivr_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdivr_s	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86edi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdivr_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdivr_d	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86edi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsub_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsub	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xD8, 0xE0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdiv_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdiv	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xD8, 0xF0, index);

  CVMJITdumpCodegenComments(con);
}

void CVMX86fdivp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdivp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDE, 0xF8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdivrp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdivrp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);


  CVMX86emit_farith(con, 0xDE, 0xF0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsubp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fsubp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDE, 0xE8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fsubrp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	subrp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDE, 0xE0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86faddp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	faddp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDE, 0xC0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fmulp_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fmulp	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDE, 0xC8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fprem(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fprem");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xF8);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fprem1(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fprem1");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xF5);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fxch_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fxch	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xD9, 0xC8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fincstp(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fincstp");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xF7);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fdecstp(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fdecstp");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xF6);

  CVMJITdumpCodegenComments(con);
}


void CVMX86ffree_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	ffree	st(%d)", index);
  }); 

  CVMassert(con->target.usesFPU);
  CVMassert(index<32);


  CVMX86emit_farith(con, 0xDD, 0xC0, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fcomp_s_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fcomp_s	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD8);
  CVMX86emit_operand_wrapper(con, CVMX86ebx, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fcomp_d_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fcomp_d	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDC);
  CVMX86emit_operand_wrapper(con, CVMX86ebx, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fcompp(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fcompp");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDE);
  CVMX86emit_byte(con, 0xD9);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fucomi_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fucomi	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDB, 0xE8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fucomip_reg(CVMJITCompilationContext* con, int index) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fucomip	st(%d)", index);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_farith(con, 0xDF, 0xE8, index);

  CVMJITdumpCodegenComments(con);
}


void CVMX86ftst(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	ftst");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_byte(con, 0xE4);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fnstsw_ax(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fnstsw_ax");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xdF);
  CVMX86emit_byte(con, 0xE0);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fwait(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fwait");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0x9B);

  CVMJITdumpCodegenComments(con);
}


void CVMX86finit(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	finit");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0x9B);
  CVMX86emit_byte(con, 0xDB);
  CVMX86emit_byte(con, 0xE3);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fnclex(CVMJITCompilationContext* con) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fnclex	");
  }); 

  CVMassert(con->target.usesFPU);


  CVMX86emit_byte(con,0xdb); 
  CVMX86emit_byte(con,0xe2);

  CVMJITdumpCodegenComments(con);
}

void CVMX86fldcw_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fldcw	");
    printAddress(src);
  }); 

  CVMassert(con->target.usesFPU);  
  CVMassert(CVMCPU_MEMSPEC_ABSOLUTE == src._type);


  CVMX86emit_byte(con, 0xd9);
  CVMX86emit_operand_wrapper(con, CVMX86ebp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86fnstcw_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fnstcw	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0x9B);
  CVMX86emit_byte(con, 0xD9);
  CVMX86emit_operand_wrapper(con, CVMX86edi, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}

void CVMX86fnsave_mem(CVMJITCompilationContext* con, CVMX86Address dst) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	fnsave	");
    printAddress(dst);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDD);
  CVMX86emit_operand_wrapper(con, CVMX86esi, dst, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86frstor_mem(CVMJITCompilationContext* con, CVMX86Address src) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	frstor	");
    printAddress(src);
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0xDD);
  CVMX86emit_operand_wrapper(con, CVMX86esp, src, CVM_FALSE);

  CVMJITdumpCodegenComments(con);
}


void CVMX86sahf(CVMJITCompilationContext* con) {
  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	sahf");
  }); CVMassert(con->target.usesFPU);



  CVMX86emit_byte(con, 0x9E);

  CVMJITdumpCodegenComments(con);
}
#endif /* CVM_JIT_USE_FP_HARDWARE */

/* ---------------- end of x86 assembler implementation ------------------- */

/**************************************************************
 * CPU ALURhs and associated types - The following are implementations
 * of the functions for the ALURhs abstraction required by the CISC
 * emitter porting layer.
 **************************************************************/

/* ALURhs constructors and query APIs: ==================================== */

/* Purpose: Constructs a constant type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewConstant(CVMJITCompilationContext* con, CVMInt32 v)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMCPU_ALURHS_CONSTANT;
    ap->constValue = v;
    return ap;
}

/* Purpose: Constructs a register type CVMCPUALURhs. */
CVMCPUALURhs*
CVMCPUalurhsNewRegister(CVMJITCompilationContext* con, CVMRMResource* rp)
{
    CVMCPUALURhs* ap = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
                                    sizeof(CVMCPUALURhs));
    ap->type = CVMCPU_ALURHS_REGISTER;
    ap->r = rp;
    return ap;
}

/* Purpose: Encodes the token for the CVMCPUALURhs operand for use in the
            instruction emitters. */
CVMCPUALURhsToken
CVMX86alurhsEncodeToken(CVMJITCompilationContext* con, CVMCPUALURhsType type, 
			int constValue, int rRegID)
{
  CVMCPUALURhsToken tok = CVMJITmemNew(con, JIT_ALLOC_CGEN_ALURHS,
				       sizeof(struct CVMCPUALURhsTokenStruct));  
  if (type == CVMCPU_ALURHS_CONSTANT)
    CVMassert(CVMCPUalurhsIsEncodableAsImmediate(constValue));
  
  tok->type = type;
  tok->con = constValue;
  tok->reg = rRegID;
  return tok;
}

CVMCPUALURhsToken
CVMX86alurhsGetToken(CVMJITCompilationContext* con, const CVMCPUALURhs *aluRhs)
{
  CVMCPUALURhsToken result;
  int regID = CVMCPU_INVALID_REG;
  if (aluRhs->type == CVMCPU_ALURHS_REGISTER) {
    regID = CVMRMgetRegisterNumber(aluRhs->r);
  }
  result = CVMX86alurhsEncodeToken(con, aluRhs->type, aluRhs->constValue, regID);
  return result;
}

/* ALURhs resource management APIs: ======================================= */

/* Purpose: Pins the resources that this CVMCPUALURhs may use. */
void
CVMCPUalurhsPinResource(CVMJITRMContext* con, int opcode, CVMCPUALURhs* ap,
                        CVMRMregset target, CVMRMregset avoid)
{
  switch (ap->type) {
  case CVMCPU_ALURHS_CONSTANT:
    CVMassert(CVMCPUalurhsIsEncodableAsImmediate(ap->constValue));
    return;
    break;
  case CVMCPU_ALURHS_REGISTER:
    CVMRMpinResource(con, ap->r, target, avoid);
    return;
    break;
  }
}

void
CVMCPUalurhsPinResourceAddr(CVMJITRMContext* con, int opcode, CVMCPUALURhs* ap,
                        CVMRMregset target, CVMRMregset avoid)
{ CVMCPUalurhsPinResource(con, opcode, ap, target, avoid);
}

/* Purpose: Relinquishes the resources that this CVMCPUALURhsToken may use. */
void CVMCPUalurhsRelinquishResource(CVMJITRMContext* con, CVMCPUALURhs* ap)
{
  switch (ap->type) {
  case CVMCPU_ALURHS_CONSTANT:
    return;
  case CVMCPU_ALURHS_REGISTER:
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

#define CVMX86memspecGetTokenType(memSpecToken) ((memSpecToken)->type)

#define CVMX86memspecGetTokenOffset(memSpecToken) ((memSpecToken)->offset)

#define CVMX86memspecGetTokenRegid(memSpecToken) ((memSpecToken)->regid)

/* Purpose: Encodes a CVMCPUMemSpecToken from the specified memSpec. */
CVMCPUMemSpecToken
CVMX86memspecGetToken(CVMJITCompilationContext *con, const CVMCPUMemSpec *memSpec)
{
  CVMCPUMemSpecToken token = CVMJITmemNew(con, JIT_ALLOC_CGEN_MEMSPEC, 
					  sizeof(struct CVMCPUMemSpecTokenStruct));
  
  token->type = memSpec->type;
  token->offset = memSpec->offsetValue;
  
  if (NULL != memSpec->offsetReg)
    token->regid = CVMRMgetRegisterNumber(memSpec->offsetReg);
  else 
    token->regid = CVMCPU_INVALID_REG;
  
  return token;
}

CVMCPUMemSpecToken
CVMX86memspecEncodeToken(CVMJITCompilationContext *con,
			 CVMCPUMemSpecType type, int offset, int regid, int scale)
{
  CVMCPUMemSpecToken token = CVMJITmemNew(con, JIT_ALLOC_CGEN_MEMSPEC, 
					  sizeof(struct CVMCPUMemSpecTokenStruct));

  token->type = type;
  switch(type) {
  case CVMCPU_MEMSPEC_REG_OFFSET:
    token->regid = regid;
    token->offset = 0;
    token->scale = CVMX86times_1;
    break;
  case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
  case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
  case CVMCPU_MEMSPEC_ABSOLUTE:
    token->offset = offset;
    token->regid = CVMCPU_INVALID_REG;
    token->scale = CVMX86no_scale;
    break;
  case CVMCPU_MEMSPEC_SCALED_REG_OFFSET:
    token->regid = regid;
    token->offset = 0;
    token->scale = scale;
    break;
  case CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET:
    token->regid = regid;
    token->offset = offset;
    token->scale = scale;
    break;
  default:
    /* Other types not supported on CISC. */
    CVMassert(CVM_FALSE);
    break;
  }
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
    CVMassert(CVMCPUmemspecIsEncodableAsImmediate(self->offsetValue));
    return;
    break;
    
  case CVMCPU_MEMSPEC_REG_OFFSET:
  case CVMCPU_MEMSPEC_SCALED_REG_OFFSET:
  case CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET:
    CVMRMpinResource(con, self->offsetReg, target, avoid);
    return;
    break;

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
CVMCPUmemspecRelinquishResource(CVMJITRMContext *con, CVMCPUMemSpec *self)
{
    switch (self->type) {
    case CVMCPU_MEMSPEC_REG_OFFSET:
    case CVMCPU_MEMSPEC_SCALED_REG_OFFSET:
    case CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET:
      CVMRMrelinquishResource(con, self->offsetReg);
      break;
    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
    case CVMCPU_MEMSPEC_ABSOLUTE:
      break;
    default:
      CVMassert(CVM_FALSE);
      break;
    }
}

/**************************************************************
 * CPU code emitters - The following are implementations of code
 * emitters required by the RISC emitter porting layer.
 **************************************************************/

/* Private x86 specific defintions: ===================================== */

#define X86_ENCODE_JCC_IMM32 0xf
#define X86_ENCODE_JMP_IMM32 0xe9
#define X86_ENCODE_JMP_IMM8  0xeb
#define X86_ENCODE_JCC_IMM8  0x70 /* only highest 4 bits significant */
#define X86_ENCODE_FLD_S_MEM 0xd9
#define X86_ENCODE_FLD_D_MEM 0xdd
#define X86_ENCODE_CALL_IMM32 0xe8
#define X86_ENCODE_MOV_IMM32 0xb8 /* | 0x0 */

/* mapped from CVMCPUCondCode defined in jitriscemitterdefs_cpu.h*/
const CVMCPUCondCode CVMCPUoppositeCondCode[] = {
  CVMCPU_COND_NO, /* 0x0 */
  CVMCPU_COND_OV, /* 0x1 */
  CVMCPU_COND_HS, /* 0x2 */  
  CVMCPU_COND_LO, /* 0x3 */  
  CVMCPU_COND_NE, /* 0x4 */  
  CVMCPU_COND_EQ, /* 0x5 */  
  CVMCPU_COND_HI, /* 0x6 */  
  CVMCPU_COND_LS, /* 0x7 */  
  CVMCPU_COND_PL, /* 0x8 */  
  CVMCPU_COND_MI, /* 0x9 */  
  CVMCPU_COND_AL, /* 0xa unused */  
  CVMCPU_COND_AL, /* 0xb unused */  
  CVMCPU_COND_GE, /* 0xc */  
  CVMCPU_COND_LT, /* 0xd */  
  CVMCPU_COND_GT, /* 0xe */  
  CVMCPU_COND_LE  /* 0xf */  
};

#ifdef CVM_TRACE_JIT
static const char* const conditions[] = {
    "n", "e", "le", "l", "leu", "cs", "neg", "vs",
    "a", "ne", "g", "ge", "gu", "cc", "pos", "vc"
};

static void
printPC(CVMJITCompilationContext* con)
{
 CVMconsolePrintf("0x%8.8x\t%d:",
            CVMJITcbufGetPhysicalPC(con),
            CVMJITcbufGetLogicalPC(con));
}

static void
getRegName(char *buf, int regID)
{
    char *name = NULL;
    switch (regID) {
        case CVMX86_EAX: name = "eax"; break;
        case CVMX86_EBX: name = "ebx"; break;
        case CVMX86_ECX: name = "ecx"; break;
        case CVMX86_EDX: name = "edx"; break;
        case CVMX86_ESI: name = "esi"; break;
        case CVMX86_EDI: name = "edi"; break;
        case CVMX86_ESP: name = "esp"; break;
        case CVMX86_EBP: name = "ebp"; break;
        case CVMX86_EFLAGS: name = "eflags"; break;
        case CVMX86_VIRT_CP_REG: name = "virt_cp_reg"; break;
        default:
            CVMconsolePrintf("Unknown register r%d\n", regID);
            CVMassert(CVM_FALSE);
    }
    sprintf(buf, "%%%s", name);
}

static void
getcc(char *buf, CVMX86Condition cc)
{
  char *ccStr;
  switch(cc) 
  {
  case CVMX86zero: /* == CVMX86equal */
     ccStr = "z"; break;
     case CVMX86notZero: /* == CVMX86notEqual */
	ccStr = "nz"; break;
     case CVMX86less:
	ccStr = "l"; break;
     case CVMX86lessEqual:
	ccStr = "le"; break;
     case CVMX86greater:
	ccStr = "g"; break;
     case CVMX86greaterEqual:
        ccStr = "ge"; break;
     case CVMX86belowEqual:
	ccStr = "be"; break;
     case CVMX86above:
	ccStr = "a"; break;
     case CVMX86overflow:
	ccStr = "o"; break;
     case CVMX86noOverflow:
	ccStr = "no"; break;
     case CVMX86carrySet: /* == CVMX86below */
	ccStr = "c"; break;
     case CVMX86carryClear: /* == CVMX86aboveEqual */
	ccStr = "nc"; break;
     case CVMX86negative:
	ccStr = "s"; break;
     case CVMX86positive:
	ccStr = "ns"; break;
     case CVMX86parity:
	ccStr = "p"; break;
     case CVMX86noParity:
	ccStr = "np"; break;
     default:
	CVMconsolePrintf("Unknown condition code %d\n", cc);
	ccStr = NULL;
	CVMassert(CVM_FALSE);
  }
  strncpy(buf, ccStr, 3);
}
#endif /* CVM_TRACE_JIT */

/*
 * Instruction emitters.
 */

void
CVMJITemitWord(CVMJITCompilationContext *con, CVMInt32 wordVal)
{
    CVMtraceJITCodegenExec({
        printPC(con);
        CVMconsolePrintf("	.word	%d", wordVal);
    });

    CVMJITdumpCodegenComments(con);

    CVMX86emit_long(con, wordVal);
}

void
CVMCPUemitNop(CVMJITCompilationContext *con)
{
   /* only 1 byte on IA-32 */
   CVMX86nop(con);
}

#ifdef CVMCPU_HAS_CP_REG
/* Purpose: Set up constant pool base register */
void
CVMCPUemitLoadConstantPoolBaseRegister(CVMJITCompilationContext *con)
{
  /* SVMC_JIT rr TODO: for the time being CVMCPU_CP_REG is a virtual
     register. Find out how to load it (move the value to its stack
     location). */
}
#endif /* CVMCPU_HAS_CP_REG */

/* Purpose:  Loads the CCEE into the specified register. */
void
CVMCPUemitLoadCCEE(CVMJITCompilationContext *con, int destRegID)
{
  CVMX86movl_reg_reg(con, destRegID, CVMCPU_SP_REG);
}

/* Purpose:  Load or Store a field of the CCEE. */
void
CVMCPUemitCCEEReferenceImmediate(CVMJITCompilationContext *con,
                                 int opcode, int regID, int offset)
{
    CVMCPUemitMemoryReferenceImmediate(con, opcode, regID,
                                       CVMX86_SP, offset);
}

/* SVMC_JIT */
static void
emitALUConstant16Scaled(CVMJITCompilationContext *con, int opcode,
			int destRegID, int srcRegID,
			CVMUint32 constant, int scale, CVMBool fixed_length)
{
  CVMX86Address   addr;

  CVMUint32 value = constant << scale;
  CVMassert(CVMCPUalurhsIsEncodableAsImmediate(value));

  CVMCPUinit_Address_base_disp( &addr, srcRegID, value,
                                CVMCPU_MEMSPEC_IMMEDIATE_OFFSET );
	
  /* dst = dst +/- imm */
  switch(opcode) {
  case CVMCPU_ADD_OPCODE:
    CVMX86leal_reg_mem(con, destRegID, addr, fixed_length);
    break;
    
  case CVMCPU_SUB_OPCODE:
    addr._disp = -addr._disp;
    CVMX86leal_reg_mem(con, destRegID, addr, fixed_length);
    break;
    
  default:
    CVMassert(0); /* illegal opcode */
  }
}

/* Purpose:  Add/sub a 16-bits constant scaled by 2^scale. Called by
 *           method prologue and patch emission routines.
 * NOTE   :  CVMCPUemitAddConstant16Scaled should not rely on regman
 *           states because the regman context used to emit the method
 *           prologue is gone at the patching time.
 *
 * SVMC_JIT NOTE: number of emitted bytes may be dependent on input.
 */
void
CVMCPUemitALUConstant16Scaled(CVMJITCompilationContext *con, int opcode,
                              int destRegID, int srcRegID,
                              CVMUint32 constant, int scale)
{
    emitALUConstant16Scaled(con, opcode, destRegID, srcRegID, constant, scale, 
			    CVM_FALSE /* number of emitted bytes may vary */);
}

/* SVMC_JIT */
/* Purpose:  Add/sub a 16-bits constant scaled by 2^scale. Called by
 *           method prologue and patch emission routines.
 */
/*
 * NOTE   :  CVMCPUemitAddConstant16Scaled_fixed should not rely on regman
 *           states because the regman context used to emit the method
 *           prologue is gone at the patching time.
 * NOTE:     CVMCPUemitALUConstant16Scaled_fixed must always emit the same
 *           number of instructions, no matter what constant or scale
 *           is passed to it.
 */
void
CVMCPUemitALUConstant16Scaled_fixed(CVMJITCompilationContext *con, int opcode,
				    int destRegID, int srcRegID,
				    CVMUint32 constant, int scale)
{
  emitALUConstant16Scaled(con, opcode, destRegID, srcRegID, constant, scale, 
			  CVM_TRUE /* number of emitted bytes must be constant */);
}

/*
 * Purpose: Emit Stack limit check at the start of each method.
 */
void
CVMCPUemitStackLimitCheckAndStoreReturnAddr(CVMJITCompilationContext* con)
{
    CVMX86Address addr;
    CVMUint32 off = CVMoffsetof(CVMCCExecEnv, stackChunkEndX);
    CVMCPUinit_Address_base_disp(&addr, CVMX86_ESP, off, 
				 CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
    
    CVMJITaddCodegenComment((con, "Store LR into frame"));
    CVMX86popl_reg(con, CVMCPU_ARG4_REG);

    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE, CVMCPU_ARG4_REG,
				       CVMCPU_JFP_REG, CVMoffsetof(CVMCompiledFrame, pcX));

    CVMJITprintCodegenComment(("Stack limit check"));
    CVMJITaddCodegenComment((con, "check for chunk overflow"));
   
    CVMX86cmpl_mem_reg( con, addr, CVMCPU_ARG2_REG );

    CVMJITaddCodegenComment((con, "letInterpreterDoInvoke"));

    CVMCPUemitAbsoluteBranch_GlueCode(con, (void*) CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr,
				      CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK, CVMCPU_COND_LS);
}

/*
 *  Purpose: Emits code to invoke method through MB.
 *           MB is already in CVMCPU_ARG1_REG.
 */
void
CVMCPUemitInvokeMethod(CVMJITCompilationContext* con)
{
    CVMX86Address addr;
    CVMUint32 off = CVMoffsetof(CVMMethodBlock, jitInvokerX);
    CVMCPUinit_Address_base_disp(&addr, CVMCPU_ARG1_REG, off, 
				 CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);

#if defined(CVM_JIT_USE_FP_HARDWARE)
    CVMX86free_float_regs(con);
#endif

    CVMJITaddCodegenComment((con, "call method through mb"));
    CVMX86call_mem(con, addr);
}


/*
 * Move the JSR return address into regno. This is a no-op on
 * cpu's where the LR is a GPR.
 */
void
CVMCPUemitLoadReturnAddress(CVMJITCompilationContext* con, int regno)
{
/*    CVMassert(regno == CVMX86_LR);
      CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE,
                        regno, regno, CVMX86alurhsEncodeConstantToken(con, 8),
                        CVMJIT_NOSETCC);
*/
}

/*
 * Branch to the address in the specified register use jmpl instruction.
 */
void
CVMCPUemitRegisterBranch(CVMJITCompilationContext* con, int regno)
{
  CVMX86jmp_reg(con, regno);
}

/*
 * Do a branch for a tableswitch. We need to branch into the dispatch
 * table. The table entry for index 0 will be generated right after
 * any instructions that are generated here.
 */
void
CVMCPUemitTableSwitchBranch(CVMJITCompilationContext* con, int indexRegNo,
                            CVMBool patchableBranch)
{
    /* This is implemented as:
     *
     * !tmpreg = key * branch_size  ! each branch in the table has same size
     * imull tmpReg, indexReg, branch_size
     * ! basePC = CVMJITcbufGetPhysicalPC() + 8 ! two instructions in between
     * addl tmpReg, (CVMJITcbufGetPhysicalPC() + 2 * CVMCPU_INSTRUCTION_SIZE)
     * ! jump to tmpReg
     * jmp tmpReg
     * table basePC:
     */
    CVMRMResource *tmpResource;
    CVMUint32 tmpReg;
    CVMInt32 basePC;

    tmpResource = CVMRMgetResource(CVMRM_INT_REGS(con), CVMRM_ANY_SET, CVMRM_EMPTY_SET, 1);
    tmpReg = CVMRMgetRegisterNumber(tmpResource);

    /*
     * Normally the size of each branch in the branch table is 5 bytes (the
     * size of jmp instruction). When CVMJIT_PATCH_BASED_GC_CHECKS is used if
     * there is backwards branch in the branch table, all branches are emitted
     * as patchable, all jmps are padded out to CVMCPU_INSTRUCTION_SIZE with
     * nops.
     */
    if (!patchableBranch) {
        CVMX86imull_reg_reg_imm32(con, tmpReg, indexRegNo, 5);
    } else {
        CVMX86imull_reg_reg_imm32(con, tmpReg, indexRegNo,
                                  CVMCPU_INSTRUCTION_SIZE);
    }

    /* Compute table basePC and load it into a register. */  
    basePC = (CVMInt32)CVMJITcbufGetPhysicalPC(con) + 6 /* sizeof(add reg, imm32) */ + 2 /* sizeof(jmp *reg) */;
    CVMX86addl_reg_imm32(con, tmpReg, basePC);

    /* emit the jmpl instruction: */
    CVMCPUemitRegisterBranch(con, tmpReg);

    CVMRMrelinquishResource(CVMRM_INT_REGS(con), tmpResource);
}

void
CVMCPUemitPopFrame(CVMJITCompilationContext* con, int resultSize)
{
    CVMUint32 offset; /* offset from JFP for new JSP */

    /* We want to set JSP to the address of the locals + the resultSize */
    offset = (con->numberLocalWords - resultSize) * sizeof(CVMJavaVal32);

    CVMX86movl_reg_reg(con, CVMCPU_JSP_REG, CVMCPU_JFP_REG);
    CVMX86subl_reg_imm32(con, CVMCPU_JSP_REG, offset);
}

/*
 * Make a PC-relative branch or branch-and-link instruction
 */
void
CVMX86emitBranch(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode /*, CVMBool annul */)
{
    int realoffset = logicalPC - CVMJITcbufGetLogicalPC(con);

#ifdef CVM_JIT_USE_FP_HARDWARE
    if (condCode < CVMCPU_COND_FFIRST) 
    { /* integer condition */
#endif 
       switch(condCode) 
       {
	  case CVMCPU_COND_AL: /* always */
	     CVMX86jmp_imm32(con, CVMJITcbufGetPhysicalPC(con) + realoffset);
	     break;
	     
	  case CVMCPU_COND_NV:	/* never */
	     return;
	     
	  default: /* conditional */
	     CVMX86jcc_imm32(con, condCode, CVMJITcbufGetPhysicalPC(con) + realoffset);
       }
#ifdef CVM_JIT_USE_FP_HARDWARE
    } 
    else 
    { /* floating point condition. 

      The 4 mutually disjoint and complete floating point relations
      are represented by the ZF, PF and CF flags in the EFLAGS
      register as follows.
      
      `<' : (not ZF) and (not PF) and CF :         (0, 0, 1),
      `=' : ZF and (not PF) and (not CF) :         (1, 0, 0),
      `>' : (not ZF) and (not PF) and (not CF) :   (0, 0, 0),
      `U' : ZF and PF and CF:                      (1, 1, 1).

      Note that the PF flag is redundant. ZF and CF alone suffice to
      distinguish the 4 cases.
      */
       CVMUint8* dest = CVMJITcbufGetPhysicalPC(con) + realoffset;
       if (condCode & CVMCPU_COND_UNORDERED_LT)
       { /* must treat `U' condition as `<'. */
	  switch (condCode & ~CVMCPU_COND_UNORDERED_LT) 
	  {
	     case CVMCPU_COND_FEQ:
		/* satisfied on `='. not satisfied on `<', `>', `U'. */
		CVMX86jcc_imm8(con, CVMX86notZero, CVMJITcbufGetPhysicalPC(con) + 2 + 6);
		CVMX86jcc_imm32(con, CVMX86carryClear, dest);
		break;
	     case CVMCPU_COND_FNE:
		/* satisfied on `<', `>', `U'. not satisfied on `='. */

		 /* hint for CVMJITfixupAddress */
		 if (-1 == logicalPC) dest = NULL;

		CVMX86jcc_imm32(con, CVMX86parity, dest);
		CVMX86jcc_imm32(con, CVMX86notZero, dest);
		break;
	     case CVMCPU_COND_FLT:
		/* satisfied on `<', `U'. not satisfied on `=', `>'. */
		CVMX86jcc_imm32(con, CVMX86carrySet, dest);
		break;
	     case CVMCPU_COND_FGT:
		/* satisfied on `>'. not satisfied on `<', `=', `U'. */
		CVMX86jcc_imm32(con, CVMX86above, dest);
		break;
	     case CVMCPU_COND_FLE:
		/* satisfied on `<', `=', `U'. not satisfied on `>'. */
		CVMX86jcc_imm32(con, CVMX86belowEqual, dest);
		break;
	     case CVMCPU_COND_FGE:
		/* satisfied on `>', `='. not satisfied on `<', `U'. */
		CVMX86jcc_imm32(con, CVMX86carryClear, dest);
		break;
	     default: 
		CVMassert(CVM_FALSE); /* unexpected condition code */
	  }
       }
       else
       { /* must treat `U' condition as `>'. */
	  switch (condCode ) 
	  {
	     case CVMCPU_COND_FEQ:
		/* satisfied on `='. not satisfied on `<', `>', `U'. */
		CVMX86jcc_imm8(con, CVMX86notZero, CVMJITcbufGetPhysicalPC(con) + 2 + 6);
		CVMX86jcc_imm32(con, CVMX86carryClear, dest);
		break;
	     case CVMCPU_COND_FNE:
		/* satisfied on `<', `>', `U'. not satisfied on `='. */

		 /* hint for CVMJITfixupAddress */
		 if (-1 == logicalPC) dest = NULL;

		CVMX86jcc_imm32(con, CVMX86parity, dest);
		CVMX86jcc_imm32(con, CVMX86notZero, dest);
		break;
	     case CVMCPU_COND_FLT:
		/* satisfied on `<'. not satisfied on `=', `>', `U'. */
		CVMX86jcc_imm8(con, CVMX86parity, CVMJITcbufGetPhysicalPC(con) + 2 + 6);
		CVMX86jcc_imm32(con, CVMX86carrySet, dest);
		break;
	     case CVMCPU_COND_FGT:
		/* satisfied on `>', `U'. not satisfied on `<', `='. */

		 /* hint for CVMJITfixupAddress */
		 if (-1 == logicalPC) dest = NULL;

		CVMX86jcc_imm32(con, CVMX86above, dest);
		CVMX86jcc_imm32(con, CVMX86parity, dest);
		break;
	     case CVMCPU_COND_FLE:
		/* satisfied on `<', `='. not satisfied on `>', `U'. */
		CVMX86jcc_imm8(con, CVMX86parity, CVMJITcbufGetPhysicalPC(con) + 2 + 6);
		CVMX86jcc_imm32(con, CVMX86belowEqual, dest);
		break;
	     case CVMCPU_COND_FGE:
		/* satisfied on `>', `=', `U'. not satisfied on `<'. */

		 /* hint for CVMJITfixupAddress */
		 if (-1 == logicalPC) dest = NULL;

		CVMX86jcc_imm32(con, CVMX86zero, dest);
		CVMX86jcc_imm32(con, CVMX86carryClear, dest);
		break;
	     default: 
		CVMassert(CVM_FALSE); /* unexpected condition code */
	  }
       }
    }
#endif

    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);
}

int
CVMCPUemitBranchNeedFixup(CVMJITCompilationContext* con,
                 int logicalPC, CVMCPUCondCode condCode,
                 CVMJITFixupElement** fixupList)
{
    int fixupLogialPC = CVMJITcbufGetLogicalPC(con);
    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList, fixupLogialPC);
    }
    CVMX86emitBranch(con, logicalPC, condCode);
    return fixupLogialPC;
}

static void
CVMX86emitPatchableBranchPadding(CVMJITCompilationContext* con, int branchPC)
{
    int size = CVMJITcbufGetLogicalPC(con) - branchPC;
    int i;
    int limit = CVMCPU_INSTRUCTION_SIZE - size;
    
    CVMassert(CVMCPU_INSTRUCTION_SIZE == 8); /* for atomic patching */
    if (limit > 0) {
	CVMJITaddCodegenComment((con, "padding nops"));
	for (i = 0; i < limit; i++) {
	    CVMCPUemitNop(con);
	}
    }
}

void
CVMCPUemitPatchableBranch(CVMJITCompilationContext* con,
			  int logicalPC, CVMCPUCondCode condCode)
{
  int branchPC = CVMJITcbufGetLogicalPC(con);
  CVMX86emitBranch(con, logicalPC, condCode);
  CVMX86emitPatchableBranchPadding(con, branchPC);
}

int
CVMCPUemitPatchableBranchNeedFixup(CVMJITCompilationContext* con,
                                   int logicalPC, CVMCPUCondCode condCode,
                                   CVMJITFixupElement** fixupList)
{
    int branchPC = CVMJITcbufGetLogicalPC(con);
    int fixupLogicalPC = 
        CVMCPUemitBranchNeedFixup(con, logicalPC, condCode, fixupList);
    CVMX86emitPatchableBranchPadding(con, branchPC);
    return fixupLogicalPC;
}

static void
CVMX86emitCall(CVMJITCompilationContext* con, int logicalPC)
{
    CVMX86address target;

    target = (CVMX86address) CVMJITcbufLogicalToPhysical(con, 0) + logicalPC;
    CVMX86call_imm32(con, target);

    CVMJITstatsRecordInc(con, CVMJIT_STATS_BRANCHES);
}

/* Encoded using mov and jmp instructions. */
void
CVMCPUemitBranchLinkNeedFixup(CVMJITCompilationContext* con, int logicalPC,
                              CVMJITFixupElement** fixupList)
{
    if (fixupList != NULL) {
        CVMJITfixupAddElement(con, fixupList,
                              CVMJITcbufGetLogicalPC(con));
    }
    CVMX86movl_reg_imm32(con, CVMX86_LR, (int)con->curPhysicalPC + 5 + 5);
    CVMX86emitBranch(con, logicalPC, CVMCPU_COND_AL);
}

/* Purpose: Emits instructions to do the specified 32 bit unary ALU
            operation. */
/*
  SVMC_JIT d022609 (ML) 2004-04-14. 

  Clients must interpret the `setcc' argument as follows: 

  setcc == CVM_TRUE (== CVMJIT_SETCC): condition code MUST BE set. 
  setcc == CVM_FALSE (== CVMJIT_NOSETCC): condition code NEED NOT BE set. 

  The condition code actually will always be set on the current IA-32
  implementation, regardless of the value of the `setcc' argument.

  So clients MAY NOT assume that the condition code is preserved for
  `setcc == CVM_FALSE'.
 */
void
CVMCPUemitUnaryALU(CVMJITCompilationContext *con, int opcode,
                   int destRegID, int srcRegID, CVMBool setcc)
{
    switch (opcode) {
    case CVMCPU_NEG_OPCODE: 
        if(destRegID != srcRegID) {
            CVMX86movl_reg_reg(con, destRegID, srcRegID);
        }
        CVMX86negl_reg(con, destRegID);       
        break;

    case CVMCPU_NOT_OPCODE: 
        if(destRegID != srcRegID) {
            CVMX86movl_reg_reg(con, destRegID, srcRegID);
        }
        /* CVMX86notl_reg(con, destRegID);       */
        CVMX86testl_reg_reg(con, destRegID, destRegID); /* AND with self. */
        CVMX86setb_reg(con, CVMX86zero, destRegID); /* Set byte to 1 if 0. */
        CVMX86movzxb_reg_reg(con, destRegID, destRegID); /* Zero extend. */
        break;

    case CVMCPU_INT2BIT_OPCODE: 
        if(destRegID != srcRegID) {
            CVMX86movl_reg_reg(con, destRegID, srcRegID);
        }
        CVMX86testl_reg_reg(con, destRegID, destRegID); /* AND with self. */
        CVMX86setb_reg(con, CVMX86notZero, destRegID); /* Set to 1 if not 0. */
        CVMX86movzxb_reg_reg(con, destRegID, destRegID); /* Zero extend. */
        break;

    default:
        CVMassert(CVM_FALSE);
    }
}

void
CVMCPUemitUnaryALU64(CVMJITCompilationContext *con, int opcode,
                   int dstRegID, int srcRegID)
{
  CVMInt32 dst_hi;
  CVMInt32 dst_lo;
  if (dstRegID != srcRegID)
  {
     CVMX86movl_reg_reg(con, dstRegID, srcRegID);
     CVMX86movl_reg_reg(con, 1+dstRegID, 1+srcRegID);
  }
#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
  dst_hi = dstRegID+1;
  dst_lo = dstRegID;
#else
  dst_hi = dstRegID;
  dst_lo = dstRegID+1;
#endif
  switch (opcode) {
     case CVMCPU_NEG64_OPCODE: 
     {
	CVMX86negl_reg(con, dst_hi);
	CVMX86negl_reg(con, dst_lo);
	CVMX86sbbl_reg_imm32(con, dst_hi, 0) /* set condition code */;
	break;
     }
     default:
	CVMassert(CVM_FALSE);
  }
}

/* Purpose: Emits a div or rem. */
static void
CVMX86emitDivOrRem(CVMJITCompilationContext* con, CVMX86Register divisorReg)
{
    // Full implementation of Java idiv and irem; checks for
    // special case as described in JVM spec.
    //
    //         normal case                             special case
    //
    // input : eax: dividend                           min_int
    //         divisorReg: divisor                     -1
    //
    // output: eax: quotient  (= eax idiv divisorReg)  min_int
    //         edx: remainder (= eax irem divisorReg)  0
  CVMassert(divisorReg != CVMX86edx /* might get overwritten */);
  CVMJITaddCodegenComment((con, "check divisor == 0"));
  CVMX86cmpl_reg_imm32(con, divisorReg, 0x0);
  CVMCPUemitAbsoluteCallConditional(con, (void*)CVMCCMruntimeThrowDivideByZeroGlue,
				    CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK, CVMCPU_COND_EQ);
  CVMJITaddCodegenComment((con, "check dividend == min_int32"));
  CVMX86cmpl_reg_imm32(con, CVMX86_EAX, 0x80000000);
  CVMJITaddCodegenComment((con, "jcc normal case"));
  CVMX86jcc_imm32(con, CVMX86notEqual,
		  CVMJITcbufGetPhysicalPC(con) + 17 /* normal case */);
  CVMJITaddCodegenComment((con, "clear EDX"));
  CVMX86xorl_reg_reg(con, CVMX86_EDX, CVMX86_EDX);
  CVMJITaddCodegenComment((con, "check divisor == -1"));
  CVMX86cmpl_reg_imm32(con, divisorReg, -1);
  CVMX86jcc_imm32(con, CVMX86equal,
		  CVMJITcbufGetPhysicalPC(con) + 9 /* done */);
  CVMJITaddCodegenComment((con, "normal case."));
  CVMJITaddCodegenComment((con, " sign extend EAX into EDX:EAX."));
  CVMX86cdql(con);
  CVMX86idivl_reg(con, divisorReg);
  /* done */
}

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
/*
  SVMC_JIT d022609 (ML) 2004-04-14. 

  Clients must interpret the `setcc' argument as follows: 

  setcc == CVM_TRUE (== CVMJIT_SETCC): condition code MUST BE set. 
  setcc == CVM_FALSE (== CVMJIT_NOSETCC): condition code NEED NOT BE set. 

  The condition code actually will always be set on the current IA-32
  implementation, regardless of the value of the `setcc' argument.

  So clients MAY NOT assume that the condition code is preserved for
  `setcc == CVM_FALSE'.
 */
void
CVMCPUemitBinaryALU(CVMJITCompilationContext* con,
		    int opcode, int destRegID, int lhsRegID,
		    CVMCPUALURhsToken rhsToken, CVMBool setcc)
{
  if (destRegID != lhsRegID) {
     /* we must preserve the lhs register when modelling a ternary
      * RISC operation */
     CVMX86movl_reg_reg(con, destRegID, lhsRegID);
  }

  switch(rhsToken->type) {
  case  CVMCPU_ALURHS_CONSTANT:
    switch(opcode) {
    case CVMCPU_ADD_OPCODE:
      CVMX86addl_reg_imm32(con, destRegID, rhsToken->con);
      break;
    case CVMCPU_SUB_OPCODE:
      CVMX86subl_reg_imm32(con, destRegID, rhsToken->con);
      break;
    case CVMCPU_AND_OPCODE:
      CVMX86andl_reg_imm32(con, destRegID, rhsToken->con);
      break;
    case CVMCPU_OR_OPCODE:
      CVMX86orl_reg_imm32(con, destRegID, rhsToken->con);
      break;
    case CVMCPU_XOR_OPCODE:
      CVMX86xorl_reg_imm32(con, destRegID, rhsToken->con);
      break;
    case CVMCPU_MULL_OPCODE:
      CVMX86imull_reg_reg_imm32(con, destRegID, destRegID, rhsToken->con);
      break;
    case CVMCPU_BIC_OPCODE:
      CVMX86andl_reg_imm32(con, destRegID, ~rhsToken->con);
      break;
    case CVMCPU_DIV_OPCODE:
       /* implemented separately on IA-32 */
    case CVMCPU_REM_OPCODE:
       /* implemented separately on IA-32 */
    default:
      CVMassert(CVM_FALSE);
      break;
    }
    break;
  case  CVMCPU_ALURHS_REGISTER:
    switch(opcode) {
    case CVMCPU_ADD_OPCODE:
      CVMX86addl_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_SUB_OPCODE:
      CVMX86subl_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_AND_OPCODE:
      CVMX86andl_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_OR_OPCODE:
      CVMX86orl_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_XOR_OPCODE:
      CVMX86xorl_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_MULL_OPCODE:
      CVMX86imull_reg_reg(con, destRegID, rhsToken->reg);
      break;
    case CVMCPU_DIV_OPCODE:
       /* implemented separately on IA-32 */
    case CVMCPU_REM_OPCODE:
       /* implemented separately on IA-32 */
    default:
      CVMassert(CVM_FALSE);
      break;
    }
  }
}

/* SVMC_JIT d022609 (ML) 2004-03-23.  CISC variant of
   `CVMCPUemitBinaryALURegister'. */
void
CVMCPUemitBinaryALUMemory(CVMJITCompilationContext* con, int opcode, 
			  int dstRegID, int lhsRegID, CVMCPUAddress* rhs)
{
   CVMX86Address addr = *(CVMX86Address *)rhs;
   if (dstRegID != lhsRegID) 
   {
      CVMX86movl_reg_reg(con, dstRegID, lhsRegID);
   }
  switch(opcode) {
  case CVMCPU_ADD_OPCODE:
    CVMX86addl_reg_mem(con, dstRegID, addr);
    break;
  case CVMCPU_SUB_OPCODE:
    CVMX86subl_reg_mem(con, dstRegID, addr);
    break;
  case CVMCPU_AND_OPCODE:
    CVMX86andl_reg_mem(con, dstRegID, addr);
    break;
  case CVMCPU_OR_OPCODE:
    CVMX86orl_reg_mem(con, dstRegID, addr);
    break;
  case CVMCPU_XOR_OPCODE:
    CVMX86xorl_reg_mem(con, dstRegID, addr);
    break;
  case CVMCPU_MULL_OPCODE:
    CVMX86imull_reg_mem(con, dstRegID, addr);
    break;
  default:
    CVMassert(CVM_FALSE /* unexpected opcode */);
    break;
  }
}

/* Purpose: Emits instructions to do the specified 32 bit ALU operation. */
/*
  SVMC_JIT d022609 (ML) 2004-04-14. 

  Clients must interpret the `setcc' argument as follows: 

  setcc == CVM_TRUE (== CVMJIT_SETCC): condition code MUST BE set. 
  setcc == CVM_FALSE (== CVMJIT_NOSETCC): condition code NEED NOT BE set. 

  The condition code actually will always be set on the current IA-32
  implementation, regardless of the value of the `setcc' argument.

  So clients MAY NOT assume that the condition code is preserved for
  `setcc == CVM_FALSE'.
 */
void
CVMCPUemitBinaryALUConstant(CVMJITCompilationContext* con,
			    int opcode, int destRegID, int lhsRegID,
			    CVMInt32 rhsConstValue, CVMBool setcc)
{
    if ((opcode == CVMCPU_ADD_OPCODE || opcode == CVMCPU_SUB_OPCODE) &&
	destRegID != lhsRegID &&
        setcc == CVMJIT_NOSETCC)
    {
	if (opcode != CVMCPU_SUB_OPCODE ||
	    (-(-rhsConstValue) == rhsConstValue &&
	    -rhsConstValue != rhsConstValue))
	{
	    if (opcode == CVMCPU_SUB_OPCODE) {
		rhsConstValue = -rhsConstValue;
	    }
	    {
		CVMX86Address addr;
		CVMCPUinit_Address_base_disp(&addr, lhsRegID, rhsConstValue,
		    CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
		CVMX86leal_reg_mem(con, destRegID, addr, CVM_FALSE);
		return;
	    }
	}
    }

    {
	CVMCPUALURhsToken rhsToken =
	    CVMX86alurhsEncodeToken(con, CVMCPU_ALURHS_CONSTANT, 
		rhsConstValue, CVMCPU_INVALID_REG);
	CVMCPUemitBinaryALU(con, opcode, destRegID, lhsRegID, rhsToken, setcc);
    }
}

void
CVMCPUemitBinaryALU64(CVMJITCompilationContext *con,
		      int opcode, int dstRegID, int lhsRegID, int rhsRegID)
{
   CVMInt32 dst_hi = 1+dstRegID;
   CVMInt32 dst_lo = dstRegID;
   CVMInt32 rhs_hi = 1+rhsRegID;
   CVMInt32 rhs_lo = rhsRegID;
   
   if (dstRegID != lhsRegID) 
   {
      /* we must preserve the lhs register when modelling a ternary
       * RISC operation */
      CVMX86movl_reg_reg(con, dstRegID, lhsRegID);
      CVMX86movl_reg_reg(con, 1+dstRegID, 1+lhsRegID);
   }
   
   switch(opcode) 
   {
      case CVMCPU_ADD64_OPCODE:
	 CVMX86addl_reg_reg(con, dst_lo, rhs_lo);
	 CVMX86adcl_reg_reg(con, dst_hi, rhs_hi);
	 break;
      case CVMCPU_SUB64_OPCODE:
	 CVMX86subl_reg_reg(con, dst_lo, rhs_lo);
	 CVMX86sbbl_reg_reg(con, dst_hi, rhs_hi);
	 break;
      case CVMCPU_AND64_OPCODE:
	 CVMX86andl_reg_reg(con, dst_lo, rhs_lo);
	 CVMX86andl_reg_reg(con, dst_hi, rhs_hi);
	 break;
      case CVMCPU_OR64_OPCODE:
	 CVMX86orl_reg_reg(con, dst_lo, rhs_lo);
	 CVMX86orl_reg_reg(con, dst_hi, rhs_hi);
	 break;
      case CVMCPU_XOR64_OPCODE:
	 CVMX86xorl_reg_reg(con, dst_lo, rhs_lo);
	 CVMX86xorl_reg_reg(con, dst_hi, rhs_hi);
	 break;
      case CVMCPU_DIV_OPCODE:
      case CVMCPU_REM_OPCODE:
	 /* 32 bit DIV and REM are treated as 64 bit operations by the
	    emitter, because the register pair EAX:EDX is affected on
	    X86. */
	 CVMassert(dstRegID == CVMX86eax /* constraint on X86: result in EAX:EDX */
		   && lhsRegID == CVMX86eax /* constraint on X86: lhs in EAX:EDX */);
	 CVMX86emitDivOrRem(con, rhsRegID);
	 break;
      default:
	 CVMassert(CVM_FALSE /* unexpected opcode */);
	 break;
   }
}

void
CVMCPUemitIMUL64(CVMJITCompilationContext *con,
		 int dstRegID, 
		 CVMX86Address rhs_lo, 
		 CVMX86Address rhs_hi, 
		 CVMX86Address lhs_lo, 
		 CVMX86Address lhs_hi, 
		 int tmpRegID)
{
   CVMassert(dstRegID == CVMX86_EAX /* destination must be EAX:EDX */);

   /*
    * Karatsuba-Ofman algorithm
    *
    * lo(dst) = lo(rhs_lo * lhs_lo)
    * hi(dst) = hi(rhs_lo * lhs_lo) + lo(rhs_hi * lhs_lo)
    *                               + lo(rhs_lo * lhs_hi)
    */
   CVMJITaddCodegenComment((con, "begin Karatsuba-Ofman."));
   CVMX86movl_reg_mem(con, tmpRegID, rhs_lo);
   CVMX86imull_reg_mem(con, tmpRegID, lhs_hi);
   
   CVMX86movl_reg_mem(con, CVMX86_EDX, rhs_hi);
   CVMX86movl_reg_mem(con, CVMX86_EAX, lhs_lo);
   CVMX86imull_reg_reg(con, CVMX86_EDX, CVMX86_EAX);
   CVMX86addl_reg_reg(con, tmpRegID, CVMX86_EDX);
   
   CVMX86mull_mem(con,rhs_lo);
   CVMJITaddCodegenComment((con, "end Karatsuba-Ofman."));
   CVMX86addl_reg_reg(con, CVMX86_EDX, tmpRegID);
}

/* Purpose: Emits instructions to do the specified shift on a 32 bit
   operand. */
void
CVMCPUemitShiftByConstant(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, CVMUint32 shiftAmount)
{
   CVMassert(shiftAmount >> 5 == 0);
   if (destRegID != srcRegID)
   {
      CVMX86movl_reg_reg(con, destRegID, srcRegID);
   }
   switch(opcode)
   {
      case CVMCPU_SLL_OPCODE:
	 CVMX86shll_reg_imm8(con, destRegID, shiftAmount);
	 break;
      case CVMCPU_SRL_OPCODE: 
	 CVMX86shrl_reg_imm8(con, destRegID, shiftAmount);
	 break;
      case CVMCPU_SRA_OPCODE: 
	 CVMX86sarl_reg_imm8(con, destRegID, shiftAmount);
	 break;
      default: 
	 CVMassert(CVM_FALSE); /* Unexpected opcode. */
	 return;
   }
}

void
CVMCPUemitLongShiftByConstant(CVMJITCompilationContext *con, int opcode,
			      int dstRegID, int srcRegID, CVMUint32 shiftOffset)
{
  CVMInt32 dst_hi;
  CVMInt32 dst_lo;
  if (dstRegID != srcRegID)
  {
     CVMX86movl_reg_reg(con, dstRegID, srcRegID);
     CVMX86movl_reg_reg(con, 1+dstRegID, 1+srcRegID);
  }
#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
  dst_hi = dstRegID+1;
  dst_lo = dstRegID;
#else
  dst_hi = dstRegID;
  dst_lo = dstRegID+1;
#endif
  switch(opcode) {
     case CVMCPU_SLL64_OPCODE:
	if(shiftOffset > 31) {
	   CVMX86movl_reg_reg(con, dst_hi, dst_lo);
	   CVMX86shll_reg_imm8(con, dst_hi, shiftOffset - 32);
	   CVMX86xorl_reg_reg(con, dst_lo, dst_lo);
	} else {
	   CVMX86shldl_reg_reg_imm8(con, dst_hi, dst_lo, shiftOffset);
	   CVMX86shll_reg_imm8(con, dst_lo, shiftOffset);
	}
	break;
	
     case CVMCPU_SRL64_OPCODE:
	if(shiftOffset > 31) {
	   CVMX86movl_reg_reg(con, dst_lo, dst_hi);
	   CVMX86shrl_reg_imm8(con, dst_lo, shiftOffset - 32);
	   CVMX86xorl_reg_reg(con, dst_hi, dst_hi);
	} else {
	   CVMX86shrdl_reg_reg_imm8(con, dst_lo, dst_hi, shiftOffset);
	   CVMX86shrl_reg_imm8(con, dst_hi, shiftOffset);
	}
	break;
      
     case CVMCPU_SRA64_OPCODE: 
	if(shiftOffset > 31) {
	   CVMX86movl_reg_reg(con, dst_lo, dst_hi);
	   CVMX86sarl_reg_imm8(con, dst_lo, shiftOffset - 32);
	   CVMX86sarl_reg_imm8(con, dst_hi, 31);
	} else {
	   CVMX86shrdl_reg_reg_imm8(con, dst_lo, dst_hi, shiftOffset);
	   CVMX86sarl_reg_imm8(con, dst_hi, shiftOffset);
	}
	break;
	
     default:
	CVMassert(CVM_FALSE);
	break;
  }
}

void
CVMCPUemitShiftByRegister(CVMJITCompilationContext *con, int opcode,
                          int destRegID, int srcRegID, int shiftAmountRegID)
{
   CVMassert(opcode == CVMCPU_SLL_OPCODE || opcode == CVMCPU_SRL_OPCODE ||
	     opcode == CVMCPU_SRA_OPCODE);
   
   CVMassert(shiftAmountRegID == CVMX86_ECX /* IA-32 requires shift amount in ECX */);
   
   if (destRegID != srcRegID)
   {
      CVMX86movl_reg_reg(con, destRegID, srcRegID);
   }
   switch(opcode)
   {
      case CVMCPU_SLL_OPCODE:
	 CVMX86shll_reg(con, destRegID);
	 break;
      case CVMCPU_SRL_OPCODE: 
	 CVMX86shrl_reg(con, destRegID);
	 break;
      case CVMCPU_SRA_OPCODE: 
	 CVMX86sarl_reg(con, destRegID);
	 break;
      default: 
	 CVMassert(CVM_FALSE); /* Unexpected opcode. */
	 return;
   }
}

/* Purpose: Loads a 64-bit integer constant into a register. */
void
CVMCPUemitLoadLongConstant(CVMJITCompilationContext *con, int regID,
                           CVMUint64 value)
{
#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
    CVMCPUemitLoadConstant(con, regID, (int)value);
    CVMCPUemitLoadConstant(con, regID+1, (int)(value>>32));
#else
    CVMCPUemitLoadConstant(con, regID+1, (int)value);
    CVMCPUemitLoadConstant(con, regID, (int)(value>>32));
#endif
}

/* Purpose: Emits instructions to compare 2 64 bit integers X, Y for
            the specified condition. The `zero?' flag (ZF) and the
            `negative?' flag (SF) of the `EFLAGS' register are set
            according to the 64 bit signed difference X - Y.
*/
CVMCPUCondCode
CVMCPUemitCompare64(CVMJITCompilationContext *con,
		    int opcode, CVMCPUCondCode condCode,
                    int lhsRegID, int rhsRegID)
{
    int t;
    CVMX86address dst;

    CVMassert(opcode == CVMCPU_CMP64_OPCODE);
   
    switch (condCode) {
    case CVMCPU_COND_EQ:
    case CVMCPU_COND_NE:
      /* most significant 32 bits */
      CVMX86cmpl_reg_reg(con, 1+lhsRegID, 1+rhsRegID);
      dst = CVMJITcbufGetPhysicalPC(con) + 2 /* JCC */ + 2 /* CMP */;
      CVMX86jcc_imm8(con, CVMX86notEqual, dst);
      /* least significant 32 bits */
      CVMX86cmpl_reg_reg(con, lhsRegID, rhsRegID);
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
        CVMX86pushl_reg(con, lhsRegID);
	CVMX86subl_reg_reg(con, lhsRegID, rhsRegID);
	CVMX86movl_reg_reg(con, lhsRegID, lhsRegID + 1);
	CVMX86sbbl_reg_reg(con, lhsRegID, rhsRegID + 1);
	CVMX86popl_reg(con, lhsRegID);
        break;
    default:
        CVMassert(CVM_FALSE) /* Unsupported condition code. */;
    }
   return condCode;
}

/* Purpose: Emits instructions to convert a 32 bit int into a 64 bit int. */
void
CVMCPUemitInt2Long(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
    CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE,
			   destRegID+1, srcRegID, CVMJIT_NOSETCC);
    CVMCPUemitShiftByConstant(con, CVMCPU_SRA_OPCODE, destRegID,
                              srcRegID, 31);
}

/* Purpose: Emits instructions to convert a 64 bit int into a 32 bit int. */
void
CVMCPUemitLong2Int(CVMJITCompilationContext *con, int destRegID, int srcRegID)
{
   CVMCPUemitMoveRegister(con, CVMCPU_MOV_OPCODE, destRegID, 
			  srcRegID /* on x86 we have the lo bits in srcRegID and the 
				      hi bits in srcRegID+1 */, CVMJIT_NOSETCC);
}

void
CVMCPUemitMove(CVMJITCompilationContext* con, int opcode,
	       int destRegID, CVMCPUALURhsToken srcToken,
	       CVMBool setcc)
{
  switch (opcode){
  case CVMCPU_MOV_OPCODE:
    if (srcToken->type == CVMCPU_ALURHS_CONSTANT) {
       CVMX86movl_reg_imm32(con, (CVMX86Register) destRegID, 
			    (CVMX86Register) srcToken->con);
    }
    else {
      CVMassert(srcToken->type == CVMCPU_ALURHS_REGISTER);
      if (destRegID != srcToken->reg) {
	CVMX86movl_reg_reg(con, (CVMX86Register) destRegID, (CVMX86Register) srcToken->reg);
      }
    }
    if (setcc) {
      CVMX86cmpl_reg_imm32(con, destRegID, 0);
    }
   
    break;

#ifdef CVM_JIT_USE_FP_HARDWARE
    case CVMCPU_FMOV_OPCODE:
	CVMassert(srcToken->type != CVMCPU_ALURHS_CONSTANT);
	CVMassert(!setcc);
	CVMCPUemitUnaryFP(con, CVMCPU_FMOV_OPCODE, destRegID, srcToken->reg);
	break;
#endif
  default:
    CVMassert(CVM_FALSE);
  }
}

void
CVMCPUemitCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, CVMCPUALURhsToken rhsToken)
{
  CVMassert(opcode == CVMCPU_CMP_OPCODE);
  if (CVMCPU_ALURHS_REGISTER == rhsToken->type) {
    CVMX86cmpl_reg_reg(con, lhsRegID, rhsToken->reg);
  }
  else {
    CVMassert(CVMCPU_ALURHS_CONSTANT == rhsToken->type);
    CVMX86cmpl_reg_imm32(con, lhsRegID, rhsToken->con);
  }
}
/* #endif */

void
CVMCPUemitCompareConstant(CVMJITCompilationContext* con,
			  int opcode, CVMCPUCondCode condCode,
			  int lhsRegID, CVMInt32 rhsConstValue)
{
   CVMassert(CVMCPU_CMP_OPCODE == opcode);
   CVMX86cmpl_reg_imm32(con, lhsRegID, rhsConstValue);
}

void
CVMCPUemitCompareRegister(CVMJITCompilationContext* con,
			  int opcode, CVMCPUCondCode condCode,
			  int lhsRegID, int rhsRegID)
{
   CVMassert(CVMCPU_CMP_OPCODE == opcode);
   CVMX86cmpl_reg_reg(con, lhsRegID, rhsRegID);
}

void
CVMCPUemitCompareRegisterMemory(CVMJITCompilationContext* con,
				int opcode, CVMCPUCondCode condCode,
				int lhsRegID, CVMCPUAddress* rhs)
{
   CVMX86Address addr = *(CVMX86Address*)rhs;
   CVMassert(CVMCPU_CMP_OPCODE == opcode);
   CVMX86cmpl_reg_mem(con, lhsRegID, addr);
}

void
CVMCPUemitCompareMemoryRegister(CVMJITCompilationContext* con,
				int opcode, CVMCPUCondCode condCode,
				CVMCPUAddress* lhs, int rhsRegID)
{
   CVMX86Address addr = *(CVMX86Address*)lhs;
   CVMassert(CVMCPU_CMP_OPCODE == opcode);
   CVMX86cmpl_mem_reg(con, addr, rhsRegID);
}

void
CVMCPUemitCompareMemoryImmediate(CVMJITCompilationContext* con,
				 int opcode, CVMCPUCondCode condCode,
				 CVMCPUAddress* addr, int constant)
{
    CVMX86cmpl_mem_imm32(con, *addr, constant);    
}

/*
 * Putting a big (non-immediate on RISC) value into a register.
 */
void
CVMCPUemitLoadConstant(CVMJITCompilationContext* con, int regno, CVMInt32 v)
{
   CVMX86movl_reg_imm32(con, regno, v);
}


/*
 * Putting a constant address value into a register.
 */
void
CVMCPUemitLoadAddrConstant(CVMJITCompilationContext* con, int regno, CVMAddr v)
{
  CVMCPUemitLoadConstant( con, regno, ( CVMInt32 ) v );
}   /* CVMCPUemitLoadAddrConstant */


void
CVMCPUemitMemoryReferencePCRelative(CVMJITCompilationContext* con,
				    int opcode, int destRegID, int offset)
{
    CVMInt32 physicalPC = (CVMInt32)CVMJITcbufGetPhysicalPC(con);
    CVMAddr addr = physicalPC + offset;

    CVMCPUemitLoadAddrConstant(con, destRegID, addr);
    CVMCPUemitMemoryReference(con, opcode,
        destRegID, destRegID, CVMCPUmemspecEncodeImmediateToken(con, 0));
}

/*
 * This is for emitting the sequence necessary for doing a call to an
 * absolute target. The target can either be in the code cache
 * or to a vm function. If okToDumpCp is TRUE, the constant pool will
 * be dumped if possible. Set okToBranchAroundCpDump to FALSE if you plan
 * on generating a stackmap after calling CVMCPUemitAbsoluteCall.
 */
void
CVMCPUemitAbsoluteCall_GlueCode(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump)
{
    int helperOffset;

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else
#endif
    {
        helperOffset = (CVMUint8*)target - con->codeEntry;
    }

#ifdef CVM_JIT_USE_FP_HARDWARE
    CVMX86free_float_regs(con);
#endif
    CVMX86emitCall(con, (int)helperOffset);

    CVMJITresetSymbolName(con);
}

void
CVMCPUemitAbsoluteBranch_GlueCode(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump,
		       CVMCPUCondCode condCode)
{
    int helperOffset;

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else
#endif
    {
        helperOffset = (CVMUint8*)target - con->codeEntry;
    }

#ifdef CVM_JIT_USE_FP_HARDWARE
    CVMX86free_float_regs(con);
#endif
    CVMX86emitBranch(con, (int)helperOffset, condCode);

    CVMJITresetSymbolName(con);
}

/*
 * Use this function if the call may only be 2 bytes long.
 * This can be important for recomputing the java pc from the
 * native pc.
 */
void
CVMCPUemitIndirectCall_GlueCode(CVMJITCompilationContext* con,
                       const void* target,
		       CVMX86Register reg,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump)
{
    int helperOffset;
    CVMX86address addr;

#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    if (target >= (void*)&CVMCCMcodeCacheCopyStart &&
        target < (void*)&CVMCCMcodeCacheCopyEnd) {
        helperOffset = CVMCCMcodeCacheCopyHelperOffset(con, target);
    } else
#endif
    {
        helperOffset = (CVMUint8*)target - con->codeEntry;
    }

    addr = (CVMX86address) CVMJITcbufLogicalToPhysical(con, 0) + helperOffset;

#ifdef CVM_JIT_USE_FP_HARDWARE
    CVMX86free_float_regs(con);    
#endif
    CVMX86movl_reg_imm32(con, reg, (int) addr);
    CVMX86call_reg(con, reg);

    CVMJITresetSymbolName(con);
}

void
CVMCPUemitAbsoluteCall(CVMJITCompilationContext* con,
                       const void* target,
		       CVMBool okToDumpCp,
		       CVMBool okToBranchAroundCpDump,
		       int numberOfArgsWords)
{
  if (numberOfArgsWords >= 4) {
    CVMX86pushl_reg(con, CVMCPU_ARG4_REG);
  } 
  if (numberOfArgsWords >= 3) {
    CVMX86pushl_reg(con, CVMCPU_ARG3_REG);
  } 
  if (numberOfArgsWords >= 2) {
    CVMX86pushl_reg(con, CVMCPU_ARG2_REG);
  } 
  if (numberOfArgsWords >= 1) {
    CVMX86pushl_reg(con, CVMCPU_ARG1_REG);
  }

  CVMCPUemitAbsoluteCall_GlueCode(con,target,okToDumpCp,okToBranchAroundCpDump);

  if (0 != numberOfArgsWords) {
    CVMX86addl_reg_imm32(con, CVMCPU_SP_REG, numberOfArgsWords * 4);
  }
}

/*
 * Return using the correct CCM helper function.
 */
void
CVMCPUemitReturn(CVMJITCompilationContext* con, void* helper)
{
  CVMassert(helper != NULL /* jmp most probably wrong */);
  /* InstructionMark im(this); */

  CVMtraceJITCodegenExec({
    printPC(con);
    CVMconsolePrintf("	jmp	$0x%x", helper);
  });  


  CVMX86emit_byte(con, 0xE9);
  CVMX86emit_data(con, (int)helper - ((int)con->curPhysicalPC/*_code_pos*/ + sizeof(long)));

  CVMJITdumpCodegenComments(con);
}

#ifdef MIN
#undef MIN
#endif
#define MIN(x,y) ((x) < (y) ? (x) : (y))

void
CVMCPUemitFlushJavaStackFrameAndAbsoluteCall_CCode(CVMJITCompilationContext* con,
						   const void* target,
						   CVMBool okToDumpCp,
						   CVMBool okToBranchAroundCpDump,
						   int numberOfArgsWords)
{
    CVMInt32 logicalPC;
    CVMCodegenComment *comment;
    CVMInt32 saved_offset;
    CVMInt32 ffree_space=0;

    CVMJITpopCodegenComment(con, comment);
    /* load the return PC */
    CVMJITsetSymbolName((con, "return address"));
    
    /* Store the JSP into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush JSP to stack"));
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_STR32_OPCODE,
        CVMCPU_JSP_REG, CVMCPU_JFP_REG, offsetof(CVMFrame, topOfStack));

    /* Store the return PC into the compiled frame: */
    CVMJITaddCodegenComment((con, "flush return address to frame"));
    
    logicalPC = CVMJITcbufGetLogicalPC(con);

#ifdef CVM_JIT_USE_FP_HARDWARE
    ffree_space = (con->target.usesFPU) ? 16 : 0;
#endif

    saved_offset = ffree_space                      /* 8 x ffree */
      + 12                                          /* call + movl */
      + MIN(numberOfArgsWords, CVMCPU_MAX_ARG_REGS) /* pushls of arg regs */
      + 3                                           /* addl to pop args */;
    
    {
       CVMX86Address dst; 
       CVMCPUinit_Address_base_disp( &dst,
				     CVMCPU_JFP_REG, 
				     offsetof(CVMCompiledFrame, pcX),
				     CVMCPU_MEMSPEC_IMMEDIATE_OFFSET );
       CVMX86movl_mem_imm32(con, dst, 
			    (CVMInt32)CVMJITcbufGetPhysicalPC(con) 
			    + saved_offset);
    }

    /* call the function */
    CVMJITpushCodegenComment(con, comment);
    CVMCPUemitAbsoluteCall(con, target, CVMJIT_NOCPDUMP, CVMJIT_NOCPBRANCH,
			   numberOfArgsWords);

    {
        /* Make sure we really only emitted 6 instructions. Otherwise,
           fix up the return address.
         */
        CVMInt32 returnPcOffset = CVMJITcbufGetLogicalPC(con) - logicalPC;
        if (returnPcOffset != saved_offset) {
            CVMJITcbufPushFixup(con, logicalPC);
            CVMJITsetSymbolName((con, "return address"));
            CVMassert(0);
/*            CVMSPARCemitMakeConstant32(con, CVMSPARC_g1,
                                     (CVMInt32)CVMJITcbufGetPhysicalPC(con) +
                                     returnPcOffset);
*/            CVMJITcbufPop(con);
        }
    }
}

#ifdef CVM_TRACE_JIT
static void
printAddress(CVMX86Address addr)
{
  CVMtraceJITCodegenExec({
    char baseregbuf[30];
    char offsetregbuf[30];

    switch (addr._type) {
    case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
      getRegName(baseregbuf, addr._base);
      CVMconsolePrintf("%d(%s)", addr._disp, baseregbuf);
      break;

    case CVMCPU_MEMSPEC_REG_OFFSET:  
      getRegName(baseregbuf, addr._base);
      getRegName(offsetregbuf, addr._index);
      CVMconsolePrintf("0(%s, %s)", baseregbuf, offsetregbuf);
      break;

    case CVMCPU_MEMSPEC_ABSOLUTE:
      CVMconsolePrintf("0x%x(,1)", addr._disp);
      break;

    case CVMCPU_MEMSPEC_SCALED_REG_OFFSET: {
      int scaleFac;
      switch(addr._scale) {
      case CVMX86times_1:
	scaleFac = 1;
	break;
      case CVMX86times_2:
	scaleFac = 2;
	break;
      case CVMX86times_4:
	scaleFac = 4;
	break;
      case CVMX86times_8:
	scaleFac = 8;
	break;
      default:
	scaleFac = 0;
	CVMassert(0);
      }
      getRegName(baseregbuf, addr._base);
      getRegName(offsetregbuf, addr._index);
      CVMconsolePrintf("0(%s, %s, %d)", baseregbuf, offsetregbuf, scaleFac);
      break;
    }
      
    case CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET: {
      int scaleFac;
      switch(addr._scale) {
      case CVMX86times_1:
	scaleFac = 1;
	break;
      case CVMX86times_2:
	scaleFac = 2;
	break;
      case CVMX86times_4:
	scaleFac = 4;
	break;
      case CVMX86times_8:
	scaleFac = 8;
	break;
      default:
	scaleFac = 0;
	CVMassert(0);
      }
      getRegName(baseregbuf, addr._base);
      getRegName(offsetregbuf, addr._index);
      CVMconsolePrintf("%d(%s,%s,%d)", addr._disp, baseregbuf, offsetregbuf, scaleFac);
      break;
    }

    case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
      getRegName(baseregbuf, addr._base);
      CVMconsolePrintf("[--%s]", baseregbuf);
      break;
    default:
      CVMassert(CVM_FALSE);
    }});
}
#endif /* CVM_TRACE_JIT */

static void
CVMX86emitMemoryReference(CVMJITCompilationContext* con,
			  int opcode, int argreg, int basereg,
			  CVMCPUMemSpecToken memSpecToken)
{
  int type = CVMX86memspecGetTokenType(memSpecToken);
  int offset = CVMX86memspecGetTokenOffset(memSpecToken);
  CVMX86Address addr;
  
#ifdef CVM_JIT_USE_FP_HARDWARE
  CVMassert(opcode == CVMCPU_LDR32_OPCODE ||
	    opcode == CVMCPU_LDR16U_OPCODE ||
	    opcode == CVMCPU_LDR16_OPCODE ||
	    opcode == CVMCPU_LDR8U_OPCODE ||
	    opcode == CVMCPU_LDR8_OPCODE ||
	    opcode == CVMCPU_STR32_OPCODE ||
	    opcode == CVMCPU_STR16_OPCODE ||
	    opcode == CVMCPU_STR8_OPCODE || 
	    opcode == CVMCPU_FLDR32_OPCODE ||
	    opcode == CVMCPU_FSTR32_OPCODE ||
	    opcode == CVMCPU_FLDR64_OPCODE ||
	    opcode == CVMCPU_FSTR64_OPCODE);
#else
  CVMassert(opcode == CVMCPU_LDR32_OPCODE ||
	    opcode == CVMCPU_LDR16U_OPCODE ||
	    opcode == CVMCPU_LDR16_OPCODE ||
	    opcode == CVMCPU_LDR8U_OPCODE ||
	    opcode == CVMCPU_LDR8_OPCODE ||
	    opcode == CVMCPU_STR32_OPCODE ||
	    opcode == CVMCPU_STR16_OPCODE ||
	    opcode == CVMCPU_STR8_OPCODE);
#endif
  
  if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
      /* pre-decrement the base register */
      CVMX86subl_reg_imm32(con, basereg, offset);
      CVMX86memspecGetTokenOffset(memSpecToken) = 0;
      CVMX86memspecGetTokenType( memSpecToken )
	                         = CVMCPU_MEMSPEC_IMMEDIATE_OFFSET;
  }
  
  /* emit the load or store instruction */
  CVMCPUinit_Address_base_memspectoken(&addr, basereg, memSpecToken);
  switch(opcode) {
     case CVMCPU_LDR32_OPCODE:
	CVMX86movl_reg_mem(con, argreg, addr);
	break;
#ifdef CVM_JIT_USE_FP_HARDWARE
     case CVMCPU_FLDR32_OPCODE:
	CVMX86fld_s_mem(con, addr);
	CVMassert(argreg < CVMX86_FST7/* avoid TOS register */);
	/* register numbers are relative. did push. so must add one. */
	CVMX86fstp_d_reg(con, 1 + argreg);
	break;
     case CVMCPU_FLDR64_OPCODE:
	CVMX86fld_d_mem(con, addr);
	CVMassert(argreg < CVMX86_FST7/* avoid TOS register */);
	/* register numbers are relative. did push. so must add one. */
	CVMX86fstp_d_reg(con, 1 + argreg);
	break;
#endif /* CVM_JIT_USE_FP_HARDWARE */
     case CVMCPU_LDR16U_OPCODE:
	CVMX86movzxw_reg_mem(con, argreg, addr);
	break;
     case CVMCPU_LDR16_OPCODE:
	CVMX86movsxw_reg_mem(con, argreg, addr);
	break;
     case CVMCPU_LDR8U_OPCODE:
	CVMX86movzxb_reg_mem(con, argreg, addr);
	break;
     case CVMCPU_LDR8_OPCODE:
	CVMX86movsxb_reg_mem(con, argreg, addr);
	break;
     case CVMCPU_STR32_OPCODE:
	 CVMX86movl_mem_reg(con, addr, argreg);
	break;
#ifdef CVM_JIT_USE_FP_HARDWARE
     case CVMCPU_FSTR32_OPCODE:
	CVMassert(argreg < CVMX86_FST7/* avoid TOS register */);
	CVMX86fld_s_reg(con, argreg);
	CVMX86fstp_s_mem(con, addr);
	break;
     case CVMCPU_FSTR64_OPCODE:
	CVMassert(argreg < CVMX86_FST7/* avoid TOS register */);
	CVMX86fld_s_reg(con, argreg);
	CVMX86fstp_d_mem(con, addr);
	break;
#endif /* CVM_JIT_USE_FP_HARDWARE */
     case CVMCPU_STR16_OPCODE:
	 CVMX86movw_mem_reg(con, addr, argreg);
	break;
     case CVMCPU_STR8_OPCODE:
	   CVMX86movb_mem_reg(con, addr, argreg);
	break;
     default:
	CVMassert(CVM_FALSE);
  }
  
#ifdef CVM_JIT_USE_FP_HARDWARE
  CVMJITstatsExec({
    switch(opcode) {
    case CVMCPU_LDR32_OPCODE:
    case CVMCPU_LDR16U_OPCODE:
    case CVMCPU_LDR16_OPCODE:
    case CVMCPU_LDR8U_OPCODE:
    case CVMCPU_LDR8_OPCODE:
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
#else /* CVM_JIT_USE_FP_HARDWARE */
  CVMJITstatsExec({
    switch(opcode) {
    case CVMCPU_LDR32_OPCODE:
    case CVMCPU_LDR16U_OPCODE:
    case CVMCPU_LDR16_OPCODE:
    case CVMCPU_LDR8U_OPCODE:
    case CVMCPU_LDR8_OPCODE:
      CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMREAD); break;
    case CVMCPU_STR32_OPCODE:
    case CVMCPU_STR16_OPCODE:
    case CVMCPU_STR8_OPCODE:
      CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
    default:
      CVMassert(CVM_FALSE);
    }
  });
#endif /* CVM_JIT_USE_FP_HARDWARE */
}

static void
CVMX86emitMemoryReferenceConst(CVMJITCompilationContext* con,
			       int opcode, int imm_arg, int basereg,
			       CVMCPUMemSpecToken memSpecToken)
{
  int type = CVMX86memspecGetTokenType(memSpecToken);
  int offset = CVMX86memspecGetTokenOffset(memSpecToken);
  CVMX86Address addr;
  
#ifdef CVM_JIT_USE_FP_HARDWARE
  CVMassert(opcode == CVMCPU_STR32_OPCODE ||
	    opcode == CVMCPU_STR16_OPCODE ||
	    opcode == CVMCPU_STR8_OPCODE);
#else
  CVMassert(opcode == CVMCPU_STR32_OPCODE ||
	    opcode == CVMCPU_STR16_OPCODE ||
	    opcode == CVMCPU_STR8_OPCODE);
#endif
  
  if (type == CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET) {
      /* pre-decrement the base register */
      CVMX86subl_reg_imm32(con, basereg, offset);
      CVMX86memspecGetTokenOffset(memSpecToken) = 0;
      CVMX86memspecGetTokenType( memSpecToken )
	                         = CVMCPU_MEMSPEC_IMMEDIATE_OFFSET;
  }
  
  /* emit the load or store instruction */
  CVMCPUinit_Address_base_memspectoken(&addr, basereg, memSpecToken);
  switch(opcode) {
     case CVMCPU_STR32_OPCODE:
	 CVMX86movl_mem_imm32(con, addr, imm_arg);
	break;
     case CVMCPU_STR16_OPCODE:
	 CVMassert(CVMX86isWord(imm_arg));
	 CVMX86movw_mem_imm16(con, addr, imm_arg);
	break;
     case CVMCPU_STR8_OPCODE:
	   CVMassert(CVMX86isByte(imm_arg));
	   CVMX86movb_mem_imm8(con, addr, imm_arg);
	break;
     default:
	CVMassert(CVM_FALSE);
  }
  
#ifdef CVM_JIT_USE_FP_HARDWARE
  CVMJITstatsExec({
    switch(opcode) {
    case CVMCPU_STR32_OPCODE:
    case CVMCPU_STR16_OPCODE:
    case CVMCPU_STR8_OPCODE:
      CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
    default:
      CVMassert(CVM_FALSE);
    }
  });
#else
  CVMJITstatsExec({
    switch(opcode) {
    case CVMCPU_STR32_OPCODE:
    case CVMCPU_STR16_OPCODE:
    case CVMCPU_STR8_OPCODE:
      CVMJITstatsRecordInc(con, CVMJIT_STATS_MEMWRITE); break;
    default:
      CVMassert(CVM_FALSE);
    }
  });
#endif
}

void
CVMCPUemitMemoryReference(CVMJITCompilationContext* con,
                          int opcode, int destreg, int basereg,
                          CVMCPUMemSpecToken memSpecToken)
{
   /* 
      64 bit arguments on the java stack are only word aligned.  On
      CISC / IA-32 we nevertheless can take advantage of double-word
      load / store instructions, because they do not require 64 bit
      alignment.
   */
   int is64BitRef = CVM_FALSE;
   int isFPOp = CVM_FALSE;
#ifdef CVM_JIT_USE_FP_HARDWARE
   const int has64BitFPU = (CVMCPU_FP_MAX_REG_SIZE >= 2) /* `true' for x86/87 */;
#else
   const int has64BitFPU = CVM_FALSE;
#endif

   switch(opcode)
   {
      case CVMCPU_LDR8U_OPCODE:
      case CVMCPU_LDR8_OPCODE:
      case CVMCPU_LDR16_OPCODE:
      case CVMCPU_LDR16U_OPCODE:
      case CVMCPU_LDR32_OPCODE:
	 break;
      case CVMCPU_LDR64_OPCODE:
	 is64BitRef = CVM_TRUE;
	 opcode = CVMCPU_LDR32_OPCODE;
	 break;
      case CVMCPU_STR8_OPCODE:
      case CVMCPU_STR16_OPCODE:
      case CVMCPU_STR32_OPCODE:
	 break;
      case CVMCPU_STR64_OPCODE:
	 is64BitRef = CVM_TRUE;
	 opcode = CVMCPU_STR32_OPCODE;
	 break;
#ifdef CVM_JIT_USE_FP_HARDWARE
      case CVMCPU_FLDR32_OPCODE:
	 isFPOp = CVM_TRUE;
	 break;
      case CVMCPU_FLDR64_OPCODE:
	 is64BitRef = CVM_TRUE;
	 isFPOp = CVM_TRUE;
	 if (!has64BitFPU) opcode = CVMCPU_FLDR32_OPCODE;
	 break;
      case CVMCPU_FSTR32_OPCODE:
	 isFPOp = CVM_TRUE;
	 break;
      case CVMCPU_FSTR64_OPCODE:
	 is64BitRef = CVM_TRUE;
	 isFPOp = CVM_TRUE;
	 if (!has64BitFPU) opcode = CVMCPU_FSTR32_OPCODE;
	 break;
#endif
      default:
	 CVMassert(CVM_FALSE); /* unexpected opcode*/
	 return;
   }

   if (!is64BitRef)
   { /* 32 bit load/store */
      CVMX86emitMemoryReference(con, opcode, destreg, basereg, memSpecToken);
   }
   else
   { /* 64 bit load/store */
      int type = CVMX86memspecGetTokenType(memSpecToken);
      int offset = CVMX86memspecGetTokenOffset(memSpecToken);
      switch (type) 
      {
	 case CVMCPU_MEMSPEC_IMMEDIATE_OFFSET:
	    CVMX86emitMemoryReference(con, opcode, destreg, basereg, memSpecToken);
	    if (!(isFPOp) || !(has64BitFPU))
	    { /* most significant 32 bits */
	       CVMX86emitMemoryReference(con, opcode, 1+destreg, basereg, CVMCPUmemspecEncodeImmediateToken(con, offset+4));
	    }
	    break;
         case CVMCPU_MEMSPEC_ABSOLUTE:
	    CVMX86emitMemoryReference(con, opcode, destreg, basereg, memSpecToken);
	    if (!(isFPOp) || !(has64BitFPU))
	    { /* most significant 32 bits */
	       CVMX86emitMemoryReference(con, opcode, 1+destreg, basereg, CVMCPUmemspecEncodeAbsoluteToken(con, offset+4));
	    }
	    break;
	 case CVMCPU_MEMSPEC_REG_OFFSET:
	    CVMX86emitMemoryReference(con, opcode, destreg, basereg, memSpecToken);
	    if (!(isFPOp) || !(has64BitFPU))
	    { /* most significant 32 bits */
	       /* SVMC_JIT d026208 2004-08-11
                             do not overwrite base register and condition code.
                             change offset in memspec instead.
               */
	       CVMX86emitMemoryReference
                  ( con, opcode, destreg + 1, basereg,
		    CVMX86memspecEncodeToken( con,
                                              CVMCPU_MEMSPEC_SCALED_REG_IMMEDIATE_OFFSET,
                                              memSpecToken->offset + 4,
                                              memSpecToken->regid,
                                              CVMX86times_1
                                            ) );
	    }
	    break;
	 case CVMCPU_MEMSPEC_PREDECREMENT_IMMEDIATE_OFFSET:
	    /* SVMC_JIT d022609 (ML) TODO: we overwrite the condition code. OK? */
	    if (isFPOp && has64BitFPU)
	    { /* single load/store of 64 bits from/to basereg - offset suffices */
	       CVMX86emitMemoryReference(con, opcode, destreg, basereg,
					 CVMCPUmemspecEncodePreDecrementImmediateToken(con, offset));
	    }
	    else
	    {
	       /* most significant 32 bits */
	       CVMX86emitMemoryReference(con, opcode, 1+destreg, basereg,
					 CVMCPUmemspecEncodePreDecrementImmediateToken(con, offset-4));
	       /* least significant 32 bits */
	       CVMX86emitMemoryReference(con, opcode, destreg, basereg,
					 CVMCPUmemspecEncodePreDecrementImmediateToken(con, 4));
	    }
	   break;
	 default :
	    CVMassert(CVM_FALSE);
      }
   }
}

void
CVMCPUemitMemoryReferenceConst(CVMJITCompilationContext* con,
                          int opcode, int constant, int basereg,
                          CVMCPUMemSpecToken memSpecToken)
{
   switch(opcode)
   {
      case CVMCPU_STR8_OPCODE:
      case CVMCPU_STR16_OPCODE:
      case CVMCPU_STR32_OPCODE:
	 break;
#ifdef CVM_JIT_USE_FP_HARDWARE
      case CVMCPU_FSTR32_OPCODE:
	 break;
#endif
      default:
	 CVMassert(CVM_FALSE); /* unexpected opcode*/
	 return;
   }
   CVMX86emitMemoryReferenceConst(con, opcode, constant, basereg, memSpecToken);
}

/* Purpose: Emits instructions to do a load/store operation. */
void
CVMCPUemitMemoryReferenceImmediate(CVMJITCompilationContext* con,
				   int opcode, int reg, 
				   int baseRegID, CVMInt32 immOffset)
{
  CVMassert(CVMCPUmemspecIsEncodableAsImmediate(immOffset));
  CVMCPUemitMemoryReference(con, opcode, reg, baseRegID,
			    CVMCPUmemspecEncodeImmediateToken(con, immOffset));
}

void
CVMCPUemitMemoryReferenceImmediateConst(CVMJITCompilationContext* con,
				   int opcode, int constant, 
				   int baseRegID, CVMInt32 immOffset)
{
  CVMassert(CVMCPUmemspecIsEncodableAsImmediate(immOffset));
  CVMCPUemitMemoryReferenceConst(con, opcode, constant, baseRegID,
			    CVMCPUmemspecEncodeImmediateToken(con, immOffset));
}

/* Purpose: Emits instructions to do a load/store operation. */
void
CVMCPUemitMemoryReferenceImmAbsolute(CVMJITCompilationContext* con,
				     int opcode, int imm, 
				     CVMInt32 immOffset)
{
  CVMassert(CVMCPUmemspecIsEncodableAsImmediate(immOffset));
  CVMCPUemitMemoryReferenceConst(con, opcode, imm, CVMCPU_INVALID_REG,
				 CVMCPUmemspecEncodeAbsoluteToken(con, immOffset));
}

void
CVMCPUemitMemoryReferenceRegAbsolute(CVMJITCompilationContext* con,
				   int opcode, int reg, 
				   CVMInt32 immOffset)
{
  CVMassert(CVMCPUmemspecIsEncodableAsImmediate(immOffset));
  CVMCPUemitMemoryReference(con, opcode, reg, CVMCPU_INVALID_REG,
			    CVMCPUmemspecEncodeAbsoluteToken(con, immOffset));
}

/* Purpose: Emits instructions to do a load operation on a C style
            array element:
        ldr valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
*/
void
CVMCPUemitArrayElementLoad(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID, int indexRegID,
    int shiftOpcode, int shiftAmount)
{
  CVMX86Address addr;

  /* The base + index*scale + offset addressing mode can only be used, if a left
     shift is specified as shift operation. */
  if ((CVMCPU_SLL_OPCODE == shiftOpcode) && (0<=shiftAmount) && (shiftAmount<=3)) {
    CVMCPUinit_Address_base_idx_scale_disp(&addr, (CVMCPURegister) arrayRegID,
					   (CVMCPURegister) indexRegID, 
					   (CVMCPUScaleFactor) shiftAmount, 0,
					   CVMCPU_MEMSPEC_SCALED_REG_OFFSET);
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMX86memspecEncodeToken(con, CVMCPU_MEMSPEC_SCALED_REG_OFFSET, 0, indexRegID, (CVMX86ScaleFactor) shiftAmount));
  }
  else {
    CVMCPUemitShiftByConstant(con, shiftOpcode, valueRegID, indexRegID, shiftAmount);
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMX86memspecEncodeToken(con, CVMCPU_MEMSPEC_REG_OFFSET, valueRegID, valueRegID, CVMX86times_1));
  }
}

/* Purpose: Emits instructions to do a store operation on a C style
            array element:
        str valueRegID, arrayRegID[shiftOpcode(indexRegID, shiftAmount)]
*/
void
CVMCPUemitArrayElementStore(CVMJITCompilationContext* con,
    int opcode, int valueRegID, int arrayRegID, int indexRegID,
    int shiftOpcode, int shiftAmount, int arg_is_imm, int tmpRegID)
{
  CVMX86Address addr;

  /* The base + index*scale + offset addressing mode can only be used, if a left
     shift is specified as shift operation. */
  if ((CVMCPU_SLL_OPCODE == shiftOpcode) && (0<=shiftAmount) && (shiftAmount<=3)) {
    CVMCPUinit_Address_base_idx_scale_disp(&addr, (CVMCPURegister) arrayRegID,
					   (CVMCPURegister) indexRegID, 
					   (CVMCPUScaleFactor) shiftAmount, 0,
					   CVMCPU_MEMSPEC_SCALED_REG_OFFSET);
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMX86memspecEncodeToken(con, CVMCPU_MEMSPEC_SCALED_REG_OFFSET, 0, indexRegID, (CVMX86ScaleFactor) shiftAmount));
  }
  else {
    CVMCPUemitShiftByConstant(con, shiftOpcode, tmpRegID, indexRegID, shiftAmount);
    if (arg_is_imm)
    CVMCPUemitMemoryReferenceConst(con, opcode, valueRegID, arrayRegID,
        CVMX86memspecEncodeToken(con, CVMCPU_MEMSPEC_REG_OFFSET, tmpRegID, tmpRegID, CVMX86times_1));
    else
    CVMCPUemitMemoryReference(con, opcode, valueRegID, arrayRegID,
        CVMX86memspecEncodeToken(con, CVMCPU_MEMSPEC_REG_OFFSET, tmpRegID, tmpRegID, CVMX86times_1));
  }
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
  CVMX86Address addr;

  /* The base + index*scale + offset addressing mode can only be used, if a left
     shift is specified as shift operation. If the opcode is SUB, then we have
     to compute dest := base - index*scale + offset. This is done by setting
     dest := index; dest := -dest; dest:=base + dest*scale + offset. */
  if (CVMCPU_SUB_OPCODE == opcode) {
    CVMX86movl_reg_reg(con, destRegID, indexRegID);
    CVMX86negl_reg(con, destRegID);
  }
  if ((CVMCPU_SLL_OPCODE == shiftOpcode) && (0<=shiftAmount) && (shiftAmount<=3)) {
    CVMCPUinit_Address_base_idx_scale_disp(&addr, (CVMCPURegister) baseRegID,
		   (CVMCPU_SUB_OPCODE == opcode)?(CVMCPURegister)destRegID:(CVMCPURegister)indexRegID, 
		   (CVMCPUScaleFactor) shiftAmount, 0,
		   CVMCPU_MEMSPEC_SCALED_REG_OFFSET);
    CVMX86leal_reg_mem(con, (CVMX86Register) destRegID, addr, CVM_FALSE);
  }
  else {
    CVMCPUemitShiftByConstant(con, shiftOpcode, destRegID, indexRegID, shiftAmount);
    CVMCPUemitBinaryALU(con, opcode, destRegID, destRegID,
			CVMCPUalurhsEncodeRegisterToken(con, baseRegID),
			CVMJIT_NOSETCC);
  }
}

/* Purpose: Emits instructions for implementing the following arithmetic
            operation: destReg = (lhsReg << i) + lhsReg
            where i is an immediate value. */
void
CVMCPUemitMulByConstPowerOf2Plus1(CVMJITCompilationContext *con,
                                  int destRegID, int lhsRegID,
                                  CVMInt32 shiftAmount)
{
    /* This is implemented as:

           sll lhsRegID, shiftAmount, destRegID
           add lhsRegID, destRegID, destRegID
     */
    /* Use destReg as the scratch register. */
    CVMCPUemitShiftByConstant(con, CVMCPU_SLL_OPCODE, destRegID,
                              lhsRegID, shiftAmount);
    CVMCPUemitBinaryALU(con, CVMCPU_ADD_OPCODE, destRegID, lhsRegID,
			CVMCPUalurhsEncodeRegisterToken(con, destRegID),
			CVMJIT_NOSETCC);
}

/*
 * Emit code to do a gc rendezvous.
 */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
void
CVMCPUemitGcCheck(CVMJITCompilationContext* con, CVMBool cbufRewind)
{
    const void* target = CVMCCMruntimeGCRendezvousGlue;

    /* Emit the gc rendezvous instruction(s) */
    CVMJITaddCodegenComment((con, "call CVMCCMruntimeGCRendezvousGlue"));

    CVMCPUemitAbsoluteCall(con, target, CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK, 0);
    {
      int i;
      /* Emit the gc rendezvous instruction(s) */
      CVMJITaddCodegenComment((con, "padding nops"));
      for (i = 0; i < CVMCPU_NUM_NOPS_FOR_GC_PATCH - CVMCPU_GC_RENDEZVOUS_INSTRUCTION_SIZE; i++) {
	CVMCPUemitNop(con);
      }
    }
}
#endif /* CVMJIT_PATCH_BASED_GC_CHECKS */

#ifdef CVM_JIT_USE_FP_HARDWARE

void
CVMCPUemitInitFPMode( CVMJITCompilationContext* con)
{   
/* SVMC_JIT d022609 (ML) 2003-09-22. TODO: on linux-ia32
 * initialization of the FPU control register is already done in
 * `setFPMode'.  on w32-ia32 this seems to be done by the w32 runtime
 * library. so probably the precision, the rounding mode and the
 * exception masking have already been set. */

   CVMX86Address a;
   
   CVMCPUinit_Address_disp(&a, (CVMAddr) &(CVMglobals.jit.cpu.FPModeNearDbl), 
			   CVMCPU_MEMSPEC_ABSOLUTE);
   CVMJITaddCodegenComment((con, "setting default FP mode.")); 
   CVMX86fldcw_mem(con,a);

   CVMJITaddCodegenComment((con, "clearing pending FP exceptions.")); 
   CVMX86fnclex(con);
}

/* SVMC_JIT d022609 (ML). provide address of a small constant FP
 * multiplier for shifting exponent */
CVMX86Address CVMJITX86_addrFPFactorSmall() 
{
   CVMX86Address a;
   CVMCPUinit_Address_disp(&a,
			   (CVMAddr) &(CVMglobals.jit.cpu.FPFactorSmall),
			   CVMCPU_MEMSPEC_ABSOLUTE);
   return(a);
}

/* SVMC_JIT d022609 (ML). provide address of a large constant FP
 * multiplier for shifting exponent */
CVMX86Address CVMJITX86_addrFPFactorLarge() 
{
   CVMX86Address a;
   CVMCPUinit_Address_disp(&a,
			   (CVMAddr) &(CVMglobals.jit.cpu.FPFactorLarge),
			   CVMCPU_MEMSPEC_ABSOLUTE);
   return(a);
}


/* Purpose: Emits instructions to do the specified floating point
 * operation.
 */
void
CVMCPUemitBinaryFP( CVMJITCompilationContext* con, int opcode, 
		    int dstRegID, int lhsRegID, int rhsRegID)
{   
   CVMBool strictfp = CVMmcIsStrictfp(con->mc);
   CVMX86_FP_mode mode;
   CVMassert(con->target.usesFPU);
   switch (opcode)
   {
      case CVMCPU_FADD_OPCODE: 
      case CVMCPU_FSUB_OPCODE: 
      case CVMCPU_FMUL_OPCODE: 
      case CVMCPU_FDIV_OPCODE: 
	 mode = CVMX86_FP_mode_near_sgl;
	 break;
      case CVMCPU_DADD_OPCODE: 
      case CVMCPU_DSUB_OPCODE: 
      case CVMCPU_DMUL_OPCODE: 
      case CVMCPU_DDIV_OPCODE : 
	 mode = CVMX86_FP_mode_near_dbl;
	 break;
      default: 
	 CVMassert(CVM_FALSE /* unexpected opcode */);
	 return /* quiet compiler */;
   }
   switch(opcode)
   {
      case CVMCPU_FADD_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fadd_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_DADD_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fadd_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_FSUB_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fsub_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_DSUB_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fsub_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_FMUL_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fmul_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_DMUL_OPCODE: 
	 if (strictfp)
	 { /* ensure IEEE 754 double-precision denormals handling */
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorSmall());
	    CVMX86fmul_reg(con, 1 + lhsRegID) /* shift exponent */;
	    CVMX86fmul_reg(con, 1 + rhsRegID);
	    CVMX86fstp_d_reg(con, 1 + dstRegID);
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorLarge());
	    CVMX86fmul_reg(con, 1 + dstRegID) /* shift back exponent */;
	 }
	 else
	 { /* keep higher precision (and higher performance) */
	    CVMX86fld_s_reg(con, lhsRegID);
	    CVMX86fmul_reg(con, 1+rhsRegID);
	 }
	 break;
      case CVMCPU_FDIV_OPCODE: 
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fdiv_reg(con, 1+rhsRegID);
	 break;
      case CVMCPU_DDIV_OPCODE: 
	 if (strictfp)
	 { /* ensure IEEE 754 double-precision denormals handling */
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorSmall());
	    CVMX86fmul_reg(con, 1 + lhsRegID) /* shift exponent */;
	    CVMX86fdiv_reg(con, 1 + rhsRegID);
	    CVMX86fstp_d_reg(con, 1 + dstRegID);
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorLarge());
	    CVMX86fmul_reg(con, 1 + dstRegID) /* shift back exponent */;
	 }
	 else
	 { /* keep higher precision (and higher performance) */
	    CVMX86fld_s_reg(con, lhsRegID);
	    CVMX86fdiv_reg(con, 1+rhsRegID);
	 }
	 break;
      default: 
	 CVMassert(0); /* unexpected opcode */
   }
   if (1/*strictfp*/) 
   { /* ensure IEEE single/double precision rounding */
      CVMX86Address a;
      CVMCPUinit_Address_base_disp(&a, CVMX86esp, -8, CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
      if (CVMX86_FP_mode_near_sgl == mode)
      {
	 CVMX86fstp_s_mem(con, a);
	 CVMX86fld_s_mem(con, a);
      }
      else
      {
	 CVMX86fstp_d_mem(con, a);
	 CVMX86fld_d_mem(con, a);
      }
      CVMX86fstp_d_reg(con, 1 + dstRegID);
   }
   else
   { /* keep higher precision (and higher performance) */
      CVMX86fstp_d_reg(con, 1 + dstRegID);
   }
}

/* SVMC_JIT d022609 (ML) 2004-03-10.  CISC variant of
   `CVMCPUemitBinaryFP'. */
void
CVMCPUemitBinaryFPMemory(CVMJITCompilationContext* con, int opcode, 
			 int dstRegID, int lhsRegID, CVMCPUAddress* rhs)
{
   CVMBool strictfp = CVMmcIsStrictfp(con->mc);
   CVMX86Address rhsAddr = *(CVMX86Address*)rhs;
   CVMX86_FP_mode mode;
   CVMassert(con->target.usesFPU);
   switch (opcode)
   {
      case CVMCPU_FADD_OPCODE: 
      case CVMCPU_FSUB_OPCODE: 
      case CVMCPU_FMUL_OPCODE: 
      case CVMCPU_FDIV_OPCODE: 
	 mode = CVMX86_FP_mode_near_sgl;
	 break;
      case CVMCPU_DADD_OPCODE: 
      case CVMCPU_DSUB_OPCODE: 
      case CVMCPU_DMUL_OPCODE: 
      case CVMCPU_DDIV_OPCODE : 
	 mode = CVMX86_FP_mode_near_dbl;
	 break;
      default: 
	 CVMassert(CVM_FALSE /* unexpected opcode */);
	 return /* quiet compiler */;
   }
   switch (opcode)
   {
      case CVMCPU_FADD_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fadd_s_mem(con, rhsAddr);
	 break;
      case CVMCPU_FSUB_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fsub_s_mem(con, rhsAddr);
	 break;
      case CVMCPU_FMUL_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fmul_s_mem(con, rhsAddr); 
	 break;
      case CVMCPU_FDIV_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fdiv_s_mem(con, rhsAddr); 
	 break;
      case CVMCPU_DADD_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fadd_d_mem(con, rhsAddr);
	 break;
      case CVMCPU_DSUB_OPCODE:
	 CVMX86fld_s_reg(con, lhsRegID);
	 CVMX86fsub_d_mem(con, rhsAddr);
	 break;
      case CVMCPU_DMUL_OPCODE:
	 if (strictfp)
	 { /* ensure IEEE 754 double-precision denormals handling */
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorSmall());
	    CVMX86fmul_reg(con, 1 + lhsRegID) /* shift exponent */;
	    CVMX86fmul_d_mem(con, rhsAddr);
	    CVMX86fstp_d_reg(con, 1 + dstRegID);
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorLarge());
	    CVMX86fmul_reg(con, 1 + dstRegID) /* shift back exponent */;
	 }
	 else
	 { /* keep higher precision (and higher performance) */
	    CVMX86fld_s_reg(con, lhsRegID);
	    CVMX86fmul_d_mem(con, rhsAddr); 
	 }
	 break;
      case CVMCPU_DDIV_OPCODE:
	 if (strictfp)
	 { /* ensure IEEE 754 double-precision denormals handling */
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorSmall());
	    CVMX86fmul_reg(con, 1 + lhsRegID) /* shift exponent */;
	    CVMX86fdiv_d_mem(con, rhsAddr);
	    CVMX86fstp_d_reg(con, 1 + dstRegID);
	    CVMX86fld_x_mem(con, CVMJITX86_addrFPFactorLarge());
	    CVMX86fmul_reg(con, 1 + dstRegID) /* shift back exponent */;
	 }
	 else
	 { /* keep higher precision (and higher performance) */
	    CVMX86fld_s_reg(con, lhsRegID);
	    CVMX86fdiv_d_mem(con, rhsAddr);
	 }
	 break;
      default:
	 CVMassert(CVM_FALSE); /* unexpected opcode */
	 break;
   }
   if (1/*strictfp*/) 
   { /* ensure IEEE single/double precision rounding */
      CVMX86Address a;
      CVMCPUinit_Address_base_disp(&a, CVMX86esp, -8, CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
      if (CVMX86_FP_mode_near_sgl == mode)
      {
	 CVMX86fstp_s_mem(con, a);
	 CVMX86fld_s_mem(con, a);
      }
      else
      {
	 CVMX86fstp_d_mem(con, a);
	 CVMX86fld_d_mem(con, a);
      }
      CVMX86fstp_d_reg(con, 1 + dstRegID);
   }
   else
   { /* keep higher precision (and higher performance) */
      CVMX86fstp_d_reg(con, 1 + dstRegID);
   }
}

void
CVMCPUemitUnaryFP(CVMJITCompilationContext* con, int opcode, int destRegID, int srcRegID)
{
  CVMassert(con->target.usesFPU);
    switch (opcode)
    {
       case CVMCPU_DNEG_OPCODE:
	  /* push source register  */
	  CVMX86fld_s_reg(con, srcRegID);
	  /* ST(0) <- (-1) * ST(0) */
	  CVMX86fchs(con);
	  /* pop into destination register */
	  CVMX86fstp_d_reg(con, 1+destRegID);
	  break;
       case CVMCPU_FNEG_OPCODE:
	  /* push source register  */
	  CVMX86fld_s_reg(con, srcRegID);
	  /* ST(0) <- (-1) * ST(0) */
	  CVMX86fchs(con);
	  /* pop into destination register */
	  CVMX86fstp_d_reg(con, 1+destRegID);
	  break;
       case CVMCPU_FMOV_OPCODE:
	  /* push source FP register onto FPU stack */
	  CVMX86fld_s_reg(con, srcRegID);
	  /* pop from FPU stack into destination FP register */
	  CVMX86fstp_d_reg(con, 1+destRegID);
	  break;
       default:
	  CVMassert(0); /* unexpected opcode */
    }
}

void
CVMCPUemitFCompare(CVMJITCompilationContext* con,
                  int opcode, CVMCPUCondCode condCode,
		  int lhsRegID, int rhsRegID)
{
  CVMassert(con->target.usesFPU);
    CVMassert(opcode == CVMCPU_FCMP_OPCODE || opcode == CVMCPU_DCMP_OPCODE);
    /* push lhs FPU register  */
    CVMX86fld_s_reg(con, lhsRegID);
    /* compare with rhs and pop. */
    CVMX86fucomip_reg(con, 1+rhsRegID);
}

extern void
CVMCPUemitLoadConstantFP(
    CVMJITCompilationContext *con,
    int regID,
    CVMUint32 v)
{
    /*
     * there is no floating equivalent to sethi/or, so we will
     * load a constant from the constant pool.
     */
    CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
    CVMInt32 offset;
    CVMassert(con->target.usesFPU);
    offset = CVMJITgetRuntimeConstantReference32(con, logicalPC, v);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR32_OPCODE, regID, CVMCPU_CP_REG, offset);
}

/* Purpose: Loads a 64-bit double constant into an FP register. */
void
CVMCPUemitLoadLongConstantFP(CVMJITCompilationContext *con, int regID,
                             CVMUint64 value)
{
  CVMInt32 logicalPC = CVMJITcbufGetLogicalPC(con);
  CVMInt32 offset1, offset2;
  CVMJavaVal64 val;
  CVMassert(con->target.usesFPU);

  if (CVMCPU_FP_MAX_REG_SIZE <= 1) {
    CVMmemCopy64(val.v, ( CVMAddr * ) &value);

    offset1 = CVMJITgetRuntimeConstantReference32(con, logicalPC, val.v[0]);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR32_OPCODE, regID, CVMCPU_CP_REG, offset1);

    offset2 = CVMJITgetRuntimeConstantReference32(con, logicalPC, val.v[1]);
    CVMCPUemitMemoryReferenceImmediate(
        con, CVMCPU_FLDR32_OPCODE, regID+1, CVMCPU_CP_REG, offset2);

  }
  else {
    memset( &val, 0, sizeof( val ) );
    val.l = value;
    offset1 = CVMJITgetRuntimeConstantReference64(con, logicalPC, &val);
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_FLDR64_OPCODE, regID, 
				       CVMCPU_CP_REG, offset1);
  }
}

#endif /* CVM_JIT_USE_FP_HARDWARE */

/*
 * CVMJITcanReachAddress - Check if toPC can be reached by an
 * instruction at fromPC using the specified addressing mode. If
 * needMargin is true, then a margin of safety is added (usually the
 * allowed offset range is cut in half).
 */
CVMBool
CVMJITcanReachAddress(CVMJITCompilationContext* con,
                      int fromPC, int toPC,
                      CVMJITAddressMode mode, CVMBool needMargin)
{
   return CVM_TRUE;
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
    CVMX86address target;

    CVMtraceJITCodegen((":::::Fixed instruction at %d to reference %d\n",
            instructionLogicalAddress, targetLogicalAddress));

    if (!CVMJITcanReachAddress(con, instructionLogicalAddress,
                               targetLogicalAddress,
                               instructionAddressMode, CVM_FALSE)) {
        CVMJITerror(con, CANNOT_COMPILE, "Can't reach address");
    }

    instructionPtr = (CVMUint32*)
        CVMJITcbufLogicalToPhysical(con, instructionLogicalAddress);
    instruction = *instructionPtr;
    target = (CVMX86address)
      (((CVMUint32) instructionPtr) + targetLogicalAddress - instructionLogicalAddress);

    if ( X86_ENCODE_JMP_IMM32 == (instruction & 0xff) ) {
	CVMX86patch_jmp_imm32(instructionPtr, target);      
    }
    
    else if ( X86_ENCODE_JCC_IMM32 == (instruction & 0xff) ) { /* It is a jcc instruction */
	
	/* extract absolute target address */
	CVMInt8* address = (CVMInt8*)instructionPtr + *(CVMUint32*)((CVMInt8*)instructionPtr + 2) + 6;

	CVMX86patch_jcc_imm32(instructionPtr, target);

	if (NULL == address) {
	    /* get next instruction */
	    instructionPtr = (CVMUint32*)((CVMInt8*)instructionPtr + 6);
	    instruction = *instructionPtr;
	    address = (CVMInt8*)instructionPtr + *(CVMUint32*)((CVMInt8*)instructionPtr + 2 ) + 6;

	    /* first jmp target was NULL, next must be NULL, too
	       (see CVMX86emitBranch) */
	    CVMassert( X86_ENCODE_JCC_IMM32 == (instruction & 0xff) );
	    CVMassert( NULL == address );

	    /* jcc_imm32 <- must be patched
	       jcc_imm32 <- must be patched */
	    CVMX86patch_jcc_imm32(instructionPtr, target);	    
	}
    }

    else if ( X86_ENCODE_JCC_IMM8 == (instruction & 0xf0) ) {

	/* get next instruction */
	instructionPtr = (CVMUint32*)((CVMInt8*)instructionPtr + 2);
	instruction = *instructionPtr;

	if ( X86_ENCODE_JCC_IMM32 == (instruction & 0xff) ) {
	    /* jcc_imm8
	       jcc_imm32 <- must be patched */
	    CVMX86patch_jcc_imm32(instructionPtr, target);	    
	}
    }
    else if ( X86_ENCODE_FLD_S_MEM == (instruction & 0xff)) {
      CVMX86patch_fld_s_mem(instructionPtr, target);
    }

    else if ( X86_ENCODE_FLD_D_MEM == (instruction & 0xff)) {
      CVMX86patch_fld_d_mem(instructionPtr, target);
    }

    else if (X86_ENCODE_CALL_IMM32 == (instruction & 0xff) ) {
      CVMassert(0); /* this case occured formerly in jsr implementation */
      CVMX86patch_call_imm32(instructionPtr, target);	    
    }

    /* the jsr case (mov and jmp) */
    else if (X86_ENCODE_MOV_IMM32 == (instruction & 0xff)) {
	/* get next instruction */
	instructionPtr = (CVMUint32*)((CVMInt8*)instructionPtr + 5);
	instruction = *instructionPtr;

	if (X86_ENCODE_JMP_IMM32 == (instruction & 0xff)) {    
	    CVMX86patch_jmp_imm32(instructionPtr, target);
	} else {
	    CVMassert(CVM_FALSE);
	}
    }

    else {
	CVMassert(CVM_FALSE);
    }
}


/**************************************************************
 * CPU C Call convention abstraction - The following are implementations of
 * calling convention support function prototypes required by the CISC emitter
 * porting layer. See jitciscemitter_cpu.h for more.
 **************************************************************/

/* Purpose: Pins an arguments to the appropriate register or store it into the
            appropriate stack location.

   Has to be called for parameters from right to left.
 */
CVMRMResource *
CVMCPUCCALLpinArg(CVMJITCompilationContext *con,
                  CVMCPUCallContext *callContext, CVMRMResource *arg,
                  int argType, int argNo, int argWordIndex,
                  CVMRMregset *outgoingRegs)
{
    int argSize = CVMARMCCALLargSize(argType);

    if (argWordIndex + argSize <= CVMCPU_MAX_ARG_REGS) {
        int regno = CVMCPU_ARG1_REG + argWordIndex;
        arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
        CVMassert(regno + arg->size <= CVMCPU_MAX_ARG_REGS);
        *outgoingRegs |= arg->rmask;
    } else {
	int regno;
        CVMRMpinResource(CVMRM_INT_REGS(con), arg, CVMRM_ANY_SET, CVMRM_EMPTY_SET);
	regno = CVMRMgetRegisterNumber(arg);
	if (argSize == 1) {
	  CVMX86pushl_reg(con, regno);
        } else {
	  CVMX86pushl_reg(con, regno + 1); // big end
	  CVMX86pushl_reg(con, regno); // little end
	}
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);
    }
    return arg;
}


#ifdef CVM_JIT_USE_FP_HARDWARE
void CVMX86free_float_regs(CVMJITCompilationContext* con)
{
  if (!con->target.usesFPU)
    return;

  CVMX86ffree_reg(con, 0);
  CVMX86ffree_reg(con, 1);
  CVMX86ffree_reg(con, 2);
  CVMX86ffree_reg(con, 3);
  CVMX86ffree_reg(con, 4);
  CVMX86ffree_reg(con, 5);
  CVMX86ffree_reg(con, 6);
  CVMX86ffree_reg(con, 7);
}

#endif

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
    CVMX86Address adr;
    CVMCPUinit_Address_base_disp(&adr, addressReg, 0,
				 CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
    CVMX86xchg_reg_mem(con, destReg, adr);
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
    CVMX86Address adr;
    CVMCPUinit_Address_base_disp(&adr, addressReg, addressOffset,
				 CVMCPU_MEMSPEC_IMMEDIATE_OFFSET);
    CVMassert(oldValReg == CVMX86_EAX);
    CVMX86cmpxchg_reg_mem(con, newValReg, adr);
    {
	int branchLogicalPC = CVMJITcbufGetLogicalPC(con);
	CVMCPUemitBranch(con, branchLogicalPC, CVMCPU_COND_NE);
	return branchLogicalPC;
    }
}

#endif
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/* Purpose: Emits a constantpool dump with a branch around it if needed. */
void
CVMX86emitConstantPoolDumpWithBranchAroundIfNeeded(
    CVMJITCompilationContext* con)
{
    if (CVMJITcpoolNeedDump(con)) {
        CVMInt32 startPC = CVMJITcbufGetLogicalPC(con);
        CVMInt32 endPC;

	CVMJITaddCodegenComment((con, "branch over constant pool dump"));
        CVMCPUemitBranch(con, startPC, CVMCPU_COND_AL);
        CVMJITdumpRuntimeConstantPool(con, CVM_TRUE);
        endPC = CVMJITcbufGetLogicalPC(con);

        /* Emit branch around the constant pool dump: */
        CVMJITcbufPushFixup(con, startPC);
	CVMJITaddCodegenComment((con, "branch over constant pool dump"));
        CVMCPUemitBranch(con, endPC, CVMCPU_COND_AL);
        CVMJITcbufPop(con);
    }
}
