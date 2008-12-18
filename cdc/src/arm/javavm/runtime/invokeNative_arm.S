/*
 * @(#)invokeNative_arm.S	1.18 06/10/10
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

#include "javavm/include/porting/defs.h"
#include "javavm/include/asmmacros_cpu.h"

	SET_SECTION_EXEC(invokeNative_arm)

/* SUBROUTINE CVMjniInvokeNative */

/* This function translates the "Java" calling convention into the "C" */
/* calling convention used in native methods. Java VM passes all the  */
/* arguments in the Java stack, and expects the results to be placed there  */
/* as well. We therefore have to copy the arguments into the C stack (or  */
/* registers), and place the return values back into the Java stack. */

/* With a small sacrifise in efficiency, this approach avoids having to */
/* generate a stub function for every native methods. */

/* The most widely accepted StrongARM SA-100 programming model has */
/* arguments passed to an invoked subroutine in registers a1-a4, with */
/* any further arguments passed on the stack. */

/* The first argument to CVMjniInvokeNative is a pointer to the JNI */
/* environment, which should be passed unmodified as the first argument */
/* to the native method. */

/* The second argument is a pointer to the "real" native method function. */

/* The third argument (stk) is a pointer to the Java stack, where all */
/* the arguments are stored (as stk[0], stk[1], etc.). */

/* The fourth argument is the terse signature of the native method. */

/* The fifth argument is the total size (in 32-bit words) of the */
/* arguments on the Java stack. Note that the Java stack does not have */
/* any alignment requirement, and stores all arguments consecutively in */
/* words and double words. The argument size includes the "this" pointer */
/* for non-static methods. */

/* The sixth argument is 0 for non-static methods, or a jclass */
/* for static methods. Non-static native methods receive an object */
/* reference as the second argument (which is simply the address of */
/* stk[0]). The "real" method arguments to non-static methods begin at */
/* stk[1]. Static native methods receive a class reference as the second */
/* argument. */

/* The return value of the native method is placed at stk[0] for */
/* word-sized results, or at stk[0] and stk[1] for */
/* double-word-sized results. The return value of CVMjniInvokeNative is */
/* 0 if the native method returns void, 1 if the native */
/* method returns a word, 2 if the native method returns a */
/* double word, or -1 if the native method returns an object. */

	ALIGN(4)

	ENTRY(CVMjniInvokeNative)
ENTRY1(CVMjniInvokeNative)

/* Arguments: */

/* a1	   JNI environment */
/* a2	   native function */
/* a3	   Java stack top */
/* a4	   method descriptor */
/* stk1	   number of argument words to be passed to native function */
/* stk2	   class for static methods, or 0 for non-static methods */
/* stk3     result pointer */

/* Local registers: */
/* l0 */
/* l1	   return type syllable */
/* l2	   sig word buffer */

/* l4       dispatch base for arg/return type switch jump. */

/* Results:	 */
/* a1	return value word count or -1 for object */


#define SIGPTR	a4
#define SIGBYTE v3
#define JSTKPTR v1
#define CSTKPTR v7

#define RETTYPE v5
#define SIGBUFF  lr
#define SWITCHBASE v2

#define FuncPtr	ip

#define TYPEMASK #0xf
#define TYPESHIFT #4

#define SAVESET v1-v7,fp
#define SAVESIZE (8*4)

    /* Set up registers for processing the signature, etc. */

    stmfd   sp!, {SAVESET,lr}    /* IF YOU CHANGE THIS LINE, CHANGE THE MAP BELOW! */

#define stk1	[fp, #SAVESIZE+4]
#define stk2	[fp, #SAVESIZE+8]
#define stk3	[fp, #SAVESIZE+12]

/* transferring arguments */

/* first 4 in register */

/* The first argument (a1) is already the JNI env. */

/* The next one is a3 or stk2 if it is non-zero (for static methods). */

	mov	FuncPtr,a2
	mov	JSTKPTR,a3	/* keep a backup for a3 */
	mov	fp,sp

	ldr	SIGBUFF, [SIGPTR], #4	/* preload signature */
	and	RETTYPE,SIGBUFF,TYPEMASK /* stash return type for later use. */
	mov	SIGBUFF,SIGBUFF, lsr TYPESHIFT

	adr	SWITCHBASE,arg_jumps	/* load the base of the jump table */

        ldr	a2, stk1
	mov	a2, a2, asl #2
#ifdef AAPCS
	/* a2 is number of bytes of arguments. Be safe and assume all arguments */
	/* will require 4 bytes of padding for alignment. */
	sub	sp, sp, a2, asl #1
	/* stack must be 8-byte aligned for AAPCS */
	bic	sp, sp, #7
#else
	sub	sp, sp, a2
#endif
	mov	CSTKPTR,sp

        ldr	a2, stk2
	cmp	a2, #0
	moveq	a2, JSTKPTR
	addeq	JSTKPTR, JSTKPTR, #4

LABEL(args_loop)
	and	SIGBYTE,SIGBUFF,TYPEMASK
	mov	SIGBUFF,SIGBUFF, lsr TYPESHIFT
	ldr	pc, [SWITCHBASE, SIGBYTE, lsl #2]

LABEL(arg_32)			
	/* move a 32-bit value from [JSTKPTR] to [CSTKPTR]. */
	ldr	a3,[JSTKPTR], #4
	and	SIGBYTE,SIGBUFF,TYPEMASK
	str	a3,[CSTKPTR], #4

	mov	SIGBUFF,SIGBUFF, lsr TYPESHIFT
	ldr	pc, [SWITCHBASE, SIGBYTE, lsl #2]

LABEL(arg_64)
	ldmia	JSTKPTR!, {a3,SIGBYTE}
#ifdef AAPCS
	/* Round stack up to 8 byte boundary for AAPCS */
	add	CSTKPTR, CSTKPTR, #4
	bic	CSTKPTR, CSTKPTR, #7
#endif	
	stmia	CSTKPTR!, {a3,SIGBYTE}
	and	SIGBYTE,SIGBUFF,TYPEMASK
	mov	SIGBUFF, SIGBUFF, lsr TYPESHIFT
	ldr	pc, [SWITCHBASE, SIGBYTE, lsl #2]
		
LABEL(arg_object)
	ldr	a3,[JSTKPTR],#4
	cmp	a3,#0
	and	SIGBYTE,SIGBUFF,TYPEMASK
	beq	object_checked
	sub	a3,JSTKPTR,#4

LABEL(object_checked)

	str	a3,[CSTKPTR], #4
	mov	SIGBUFF,SIGBUFF, lsr TYPESHIFT
	ldr	pc, [SWITCHBASE, SIGBYTE, lsl #2]

LABEL(arg_reload)
	/* get another word full of types */
	/* then re-dispatch */
	/* since most signatures are short, this does not happen */
	/* very often. */
	ldr	SIGBUFF, [SIGPTR], #4	/* preload signature */
	and	SIGBYTE,SIGBUFF,TYPEMASK

	mov	SIGBUFF,SIGBUFF, lsr TYPESHIFT
	ldr	pc, [SWITCHBASE, SIGBYTE, lsl #2]

LABEL(args_done)
    /* The ARM procedure call standard we are using specifies that the first */
    /* four arguments are passed in registers a1-a4.  a1 and a2 are both set. */
    /* Now we just load up a3 and a4 with the first 2 arg method arguments. */
    /* If there are no such args, this does not hurt anything. */

	ldr	a3,[sp, #0]
	ldr	a4,[sp, #4]

#define TMP CSTKPTR

	ldr	TMP, stk1
	cmp	TMP,#2
	addge	sp, sp, #8

	adr	SWITCHBASE,ret_jumps
	
	mov	lr, pc			/* for the debugger, for now */
	BR_REG(FuncPtr)
	
	ldr	a4, stk3		/* pointer to result buffer */
	mov	sp, fp

	/* thread the return address to the */
	/* proper code for our return type */
	ldr	pc,[SWITCHBASE, RETTYPE, lsl #2]

LABEL(ret_obj)
	str	a1,[a4]
	mov	a1,#-1	/* -1 indicates object return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)
	
LABEL(ret_s32)	
	str	a1,[a4]
	mov	a1,#1	/* 1 indicates single-word return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)

LABEL(ret_s64)
	stmia	a4, {a1,a2}
	mov	a1,#2	/* 2 indicates double-word return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)

#ifdef CVM_ARM_FLOAT_RESULT_IN_FLOAT_REGISTER	
LABEL(ret_f32)	
	stfs	f0,[a4]
	mov	a1,#1	/* 1 indicates single-word return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)

LABEL(ret_f64)
	stfd	f0,[a4]
	mov	a1,#2	/* 2 indicates double-word return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)
#endif

LABEL(ret_void)
	mov	a1,#0	/* 0 indicates void return */
	ldmfd	sp!, {SAVESET, lr}
	BR_REG(lr)

LABEL(ret_jumps)		
	WORD(ret_void)	/* error */
	WORD(ret_void)	/* ENDFUNC should not get called */
	WORD(ret_void)	/* void */
	WORD(ret_s32)		/* int */
	WORD(ret_s32)		/* short */
	WORD(ret_s32)		/* char */
	WORD(ret_s64)		/* long */
	WORD(ret_s32)		/* byte */
#ifdef CVM_ARM_FLOAT_RESULT_IN_FLOAT_REGISTER	
	WORD(ret_f32)		/* float */
	WORD(ret_f64)		/* double */
#else
	WORD(ret_s32)		/* float */
	WORD(ret_s64)		/* double */
#endif
	WORD(ret_s32)		/* bool */
	WORD(ret_obj)
	WORD(ret_void)	/* this is invalid and should not get called */

LABEL(arg_jumps)
	WORD(arg_reload)	/* no more data this word: go get more */
	WORD(args_done) 	/* end-of-args */
	WORD(ret_void)	/* this is invalid and should not get called */
	WORD(arg_32)		/* int */
	WORD(arg_32)		/* short */
	WORD(arg_32)		/* char */
	WORD(arg_64)		/* long */
	WORD(arg_32)		/* byte */
	WORD(arg_32)		/* float */
	WORD(arg_64)		/* double */
	WORD(arg_32)		/* bool */
	WORD(arg_object)
	WORD(ret_void)	/* this is invalid and should not get called */
