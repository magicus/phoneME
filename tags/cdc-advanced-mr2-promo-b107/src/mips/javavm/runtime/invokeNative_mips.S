/*
 * @(#)invokeNative_mips.S	1.12 06/10/10
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

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/porting/endianness.h"

#if (defined(CVMCPU_HAS_64BIT_REGISTERS) &&		\
     !defined(CVMMIPS_HAS_64BIT_FP_REGISTERS)) ||	\
    (!defined(CVMCPU_HAS_64BIT_REGISTERS) &&		\
     defined(CVMMIPS_HAS_64BIT_FP_REGISTERS))
#error "CVMCPU_HAS_64BIT_REGISTERS != CVMMIPS_HAS_64BIT_FP_REGISTERS"
#endif

	.text
	.align	4
/*
  SUBROUTINE CVMjniInvokeNative
 
  This function translates the "Java" calling convention into the "C"
  calling convention used in native methods. Java VM passes all the 
  arguments in the Java stack, and expects the results to be placed there 
  as well. We therefore have to copy the arguments into the C stack (or 
  registers), and place the return values back into the Java stack.
 
  With a small sacrifise in efficiency, this approach avoids having to
  generate a stub function for every native methods.
 
  The first argument to CVMjniInvokeNative is a pointer to the JNI
  environment, which should be passed unmodified as the first argument
  to the native method.
 
  The second argument is a pointer to the "real" native method function.
 
  The third argument (stk) is a pointer to the Java stack, where all
  the arguments are stored (as stk[0], stk[1], etc.).
 
  The fourth argument is the terse signature of the native method.
 
  The fifth argument is the total size (in 32-bit words) of the
  arguments on the Java stack. Note that the Java stack does not have
  any alignment requirement, and stores all arguments consecutively in
  words and double words. The argument size includes the "this" pointer
  for non-static methods.
 
  The sixth argument is 0 for non-static methods, or a jclass
  for static methods. Non-static native methods receive an object
  reference as the second argument (which is simply the address of
  stk[0]). The "real" method arguments to non-static methods begin at
  stk[1]. Static native methods receive a class reference as the second
  argument.
 
  The return value of the native method is placed at stk[0] for
  word-sized results, or at stk[0] and stk[1] for
  double-word-sized results. The return value of CVMjniInvokeNative is
  0 if the native method returns void, 1 if the native
  method returns a word, 2 if the native method returns a
  double word, or -1 if the native method returns an object.
*/

/*
   Note for mipsEEel (EmotionEngine)

 o Argument passing convention of MIPS O32ABI is shown below.
   Basically the first 4 words of arguments are placed in registers (a0-a3).
   The rest are on the stack.
   The Return value is passed by using register v0 (and v1 if a 64bit value).


                  |      |
                  |      |
                  |locals|
         +------+ +------+Low Memory
      a0 | arg1 | |  --  | <- (sp)
      a1 | arg2 | |  --  |
      a2 | arg3 | |  --  |
      a3 | arg4 | |  --  |
         +------+ | arg5 |
                  | arg6 |High Memory


 o Since the first argument is scalar (void* env), all the arguments are
   passed through the scalar registers and the stack (no floating-point
   register is involved).

 o Double and int64_t (long in Java) arguments require doubleword align.

 o Scalar registers a0-a3, t0-t9, v0 and v1 can be used without
   being saved. Scalar registers s0-s7 need to be preserved across calls.

 o The floating return value is passed by using register f0

 o Stack alignment must be 4 words (128 bit) (mipsEEel specific).

*/

ENTRY(CVMjniInvokeNative)
	.globl arg_jumps 
	.globl ret_jumps

/*
  Arguments:
 
  a0	   JNI environment
  a1	   native function
  a2	   Java stack top
  a3	   method descriptor
  stk1	   number of argument words to be passed to native function
  stk2	   class for static methods, or 0 for non-static methods
  stk3     result pointer
 
  Results:	
  v0	return value word count or -1 for object
*/

#define SIGPTR	a3
#define SIGBYTE t2
#define JSTKPTR t0
#define CSTKPTR t4

#define RETTYPE s0
#define SIGBUFF t3
#define SWITCHBASE t1

#define FuncPtr	t9

#define TYPEMASK 0xf
#define TYPESHIFT 4

/* size of an argment on the stack in bytes */
#ifdef CVMCPU_HAS_64BIT_REGISTERS
#define ARGSIZE 8
#else
#define ARGSIZE 4
#endif

/* consts for stack layout */
#define SAVESIZE (4*4)

	/* sp: 4 word aligned - needed for 64-bit systems */
	framesize = (SAVESIZE + 15) & ~15 

#define stk1	framesize+4*ARGSIZE(fp)
#define stk2	framesize+5*ARGSIZE(fp)
#define stk3	framesize+6*ARGSIZE(fp)


	.frame	fp, framesize, ra

	maskra = 1 << 31
	maskfp = 1 << 30
	maskgp = 1 << 28
	masks0 = 1 << 16
	mask = maskra|maskfp|maskgp|masks0

	frameoffset = -4

	.mask	mask,frameoffset

	/* setup %gp */
	.set noreorder
	.cpload $25
	.set reorder

	subu	sp, sp, framesize  /* grow stack */

	_ra = framesize+frameoffset
	_fp = framesize+frameoffset-4
	__gp = framesize+frameoffset-8
	_s0 = framesize+frameoffset-12

	sw	ra,_ra(sp)
	sw	fp,_fp(sp)
	sw	gp,__gp(sp)
	sw	s0,_s0(sp)

	move	fp, sp

/*
 * transferring arguments
 *  -first 4 in register
 *  -The first argument (a0) is already the JNI env.
 *  -The next one is a2 or stk2 if it's non-zero (for static methods).
 */

	move	FuncPtr, a1
	move	JSTKPTR, a2	/* keep a backup for a2 */

	lw	SIGBUFF, (SIGPTR)  /* preload signature */
	addiu	SIGPTR, SIGPTR, 4
	andi	RETTYPE, SIGBUFF, TYPEMASK /* save return type for later use */
	srl	SIGBUFF, SIGBUFF, TYPESHIFT

	la	SWITCHBASE, arg_jumps /* load the address of the jump table */

	/*
	 * java ints only take up one word on the java stack but 2 words
	 * on the C stack on 64-bit systems. Also, on 32-bit systems a 32-bit
	 * argument can take up 2 words if followed by a 64-bit argument
	 * because of alignment requirements for 64-bit arguments. Because
	 * of this we're extra conservative and multiply stk1 by 8
	 * rather than take the time to count the actual number of words
	 * we need to allocate on the C stack.
	 */
	lw	a1, stk1
	sll	a1, 3

	addi	a1, 2*ARGSIZE  /* first 2 args */
	/* sp: 4 word aligned - needed for 64-bit systems */
	addi	a1, 15
	srl	a1, 4
	sll	a1, 4
	
	subu	sp, a1          /* grow stack for the function call */
	addiu	CSTKPTR, sp, 2*ARGSIZE  /* first 2 args */

	/* See if the called method is static (non-0) or non-static (0) */
	/* NOTE: This can be done better with movn if MIPS4 supported. */
	lw	a1, stk2
	bne	a1, zero, $isStatic

	move	a1, JSTKPTR
	addiu	JSTKPTR, JSTKPTR, 4
$isStatic:

#define NEXT_ARG \
	andi	SIGBYTE, SIGBUFF, TYPEMASK; \
	sll	SIGBYTE, 2;                 \
	addu	a2, SWITCHBASE, SIGBYTE;    \
	lw	a2, (a2);                   \
	srl	SIGBUFF, TYPESHIFT;	    \
	jr	a2;                         \

$args_loop:
	NEXT_ARG

$arg_32:		/* move a 32-bit value from [JSTKPTR] to [CSTKPTR]. */
	lw	a2, (JSTKPTR)
	addiu	JSTKPTR, JSTKPTR, 4
#ifdef CVMCPU_HAS_64BIT_REGISTERS
	sw	a2, (CSTKPTR)
#else
	sd	a2, (CSTKPTR)
#endif
	addiu	CSTKPTR, CSTKPTR, ARGSIZE
	NEXT_ARG

#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
$farg_32:		/* move a 32-bit float from [JSTKPTR] to [CSTKPTR]. */
	lwc1	$f0, (JSTKPTR)
	addiu	JSTKPTR, JSTKPTR, 4
	sdc1	$f0, (CSTKPTR)
	addiu	CSTKPTR, CSTKPTR, ARGSIZE
	NEXT_ARG
#endif
	
$arg_64:
#ifndef CVMCPU_HAS_64BIT_REGISTERS
	/* need 64 bit aligned, but may not already be if not 64-bit system */
	addi	CSTKPTR, CSTKPTR, 7
	srl	CSTKPTR, CSTKPTR, 3
	sll	CSTKPTR, CSTKPTR, 3
#endif

	lw	a2, (JSTKPTR)
	lw	t6, 4(JSTKPTR)
	sw	a2, (CSTKPTR)
	addiu	JSTKPTR, JSTKPTR, 8
	sw	t6, 4(CSTKPTR)
	addiu	CSTKPTR, CSTKPTR, 8
	NEXT_ARG

$arg_object:
	lw	a2, (JSTKPTR)
	beq	a2, zero, $is_null /* null check */

	addi	a2, JSTKPTR, 0
$is_null:
	addiu	JSTKPTR, JSTKPTR, 4
	sw	a2, (CSTKPTR)
	addiu	CSTKPTR, CSTKPTR, ARGSIZE
	NEXT_ARG

$arg_reload:
	/* get another word full of types
	 * then re-dispatch
	 * since most signatures are short, this doesn't happen
	 * very often. */
	lw	SIGBUFF, (SIGPTR)  /* preload signature */
	addiu	SIGPTR, SIGPTR, 4
	NEXT_ARG

$args_done:
	/*
	 * Depending on the mips calling conventions in place, various
	 * arguments are loaded into registers. We load argument registers
	 * optimistically. If there are no such args, this doesn't hurt
	 * anything.
	 */

#ifdef CVMCPU_HAS_64BIT_REGISTERS
#define LOADI ld
#define LOADF ldc1
#else
#define LOADI lw
#define LOADF lwc1
#endif

	LOADI	a2, 2*ARGSIZE(sp)
	LOADI	a3, 3*ARGSIZE(sp)
#if (CVMCPU_MAX_ARG_REGS == 8)
	LOADI	a4, 4*ARGSIZE(sp)
	LOADI	a5, 5*ARGSIZE(sp)
	LOADI	a6, 6*ARGSIZE(sp)
	LOADI	a7, 7*ARGSIZE(sp)
#endif

#ifdef CVMMIPS_FPR_ARG_AFTER_GPR_ARG
	LOADF	$f14, 2*ARGSIZE(sp)
	LOADF	$f15, 3*ARGSIZE(sp)
#if (CVMCPU_MAX_ARG_REGS == 8)
	LOADF	$f16, 4*ARGSIZE(sp)
	LOADF	$f17, 5*ARGSIZE(sp)
	LOADF	$f18, 6*ARGSIZE(sp)
	LOADF	$f19, 7*ARGSIZE(sp)
#endif
#endif /* CVMMIPS_FPR_ARG_AFTER_GPR_ARG */

	jalr	FuncPtr
	lw	gp, __gp(fp)  /* restore gp from the stack */

	la	SWITCHBASE, ret_jumps /* load the address of the jump table */

	lw	a3, stk3		/* pointer to result buffer */

	/*
	 * thread the return address to the
	 * proper code for our return type
	 */

	sll	RETTYPE, 2
	addu	a1, SWITCHBASE, RETTYPE
	lw	a1, (a1)
	move	sp, fp
	jr	a1

#define RETURN			\
	lw	ra, _ra(sp);	\
	lw	fp, _fp(sp);	\
	lw	s0, _s0(sp);	\
	addiu	sp, framesize;	\
	jr	ra;		\

$ret_obj:
	sw	v0, (a3)
	li	v0, -1	/* -1 indicates object return */
	RETURN
	
$ret_s32:	
	sw	v0, (a3)
	li	v0, 1	/* 1 indicates single-word return */
	RETURN

#ifndef __mips_soft_float
$ret_f32:
	swc1	$f0, (a3)
	li	v0, 1	/* 1 indicates single-word return */
	RETURN
#endif

$ret_s64:
#ifdef CVMCPU_HAS_64BIT_REGISTERS
	sd	v0, 0(a3)
#else
	sw	v0, 0(a3)
	sw	v1, 4(a3)
#endif
	li	v0, 2	/* 2 indicates double-word return */
	RETURN

#ifndef __mips_soft_float
$ret_f64:
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
	sdc1	$f0, (a3)
#elif (CVM_ENDIANNESS == CVM_BIG_ENDIAN)
	swc1	$f1, 0(a3)
	swc1	$f0, 4(a3)
#elif (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
	swc1	$f0, 0(a3)
	swc1	$f1, 4(a3)
#elif
#error "must #define CVM_ENDIANNESS"
#endif
	li	v0, 2   /* 2 indicates double-word return */
	RETURN
#endif

$ret_void:
	li	v0, 0	/* 0 indicates void return */
	RETURN

	SET_SIZE(CVMjniInvokeNative)

	.data
ret_jumps:		
	.word	$ret_void	/* error */
	.word	$ret_void	/* ENDFUNC shouldn't get called */
	.word	$ret_void	/* void */
	.word	$ret_s32	/* int */
	.word	$ret_s32	/* short */
	.word	$ret_s32	/* char */
	.word	$ret_s64	/* long */
	.word	$ret_s32	/* byte */
#ifndef __mips_soft_float
	.word	$ret_f32	/* float */
	.word	$ret_f64	/* double */
#else
	.word	$ret_s32	/* float */
	.word	$ret_s64	/* double */
#endif
	.word	$ret_s32	/* bool */
	.word	$ret_obj
	.word	$ret_void	/* this is invalid and shouldn't get called */

arg_jumps:
	.word	$arg_reload	/* no more data this word: go get more */
	.word	$args_done 	/* end-of-args */
	.word	$ret_void	/* this is invalid and shouldn't get called */
	.word	$arg_32		/* int */
	.word	$arg_32		/* short */
	.word	$arg_32		/* char */
	.word	$arg_64		/* long */
	.word	$arg_32		/* byte */
#ifdef CVMMIPS_HAS_64BIT_FP_REGISTERS
	.word	$farg_32	/* float */
#else
	.word	$arg_32		/* float */
#endif
	.word	$arg_64		/* double */
	.word	$arg_32		/* bool */
	.word	$arg_object
	.word	$ret_void	/* this is invalid and shouldn't get called */
