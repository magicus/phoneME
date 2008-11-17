/*
 *
 * @(#)jitasmmacros_cpu.h	1.5 06/10/04
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

#ifndef _INCLUDED_JITASMMACROS_CPU_H
#define _INCLUDED_JITASMMACROS_CPU_H

#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * Shorthand for registers that are also defined in jitasmconstants_cpu.h
 */
#define JFP %CVMX86_JFP_REGNAME
#define JSP %CVMX86_JSP_REGNAME

#define A1 %CVMX86_ARG1_REGNAME
#define A2 %CVMX86_ARG2_REGNAME
#define A3 %CVMX86_ARG3_REGNAME
#define A4 %CVMX86_ARG4_REGNAME
#define A1w %CVMX86_ARG1w_REGNAME
#define A2w %CVMX86_ARG2w_REGNAME
#define A3w %CVMX86_ARG3w_REGNAME
#define A4w %CVMX86_ARG4w_REGNAME
#define A1b %CVMX86_ARG1b_REGNAME
#define A2b %CVMX86_ARG2b_REGNAME
#define A3b %CVMX86_ARG3b_REGNAME
#define A4b %CVMX86_ARG4b_REGNAME

#define PHI1 %CVMX86_PHI1_REGNAME

/* SVMC_JIT d022185(HS) 2004-06-29:
 * do not use SYMNAME here
 * VMFUNCTION could be a register
 */
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define CALL_VM_FUNCTION(VMFUNCTION)       \
        call VMFUNCTION
#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION) \
	jmp CCMFUNCTION
#else
#define CALL_VM_FUNCTION(VMFUNCTION)       \
	call VMFUNCTION
#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION) \
        jmp CCMFUNCTION
#endif

/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * calling CVMJITfixupFrames if the frames need fixing.
 */
/* NOTE: we use 3 words of ccmStorage[] starting at offset 12. */
#define FIXUP_FRAMES_STORAGE \
	OFFSET_CVMCCExecEnv_ccmStorage+12
#define SAVE1(r1)       \
        movl    r1, FIXUP_FRAMES_STORAGE(%esp)
#define RESTORE1(r1)    \
        movl    FIXUP_FRAMES_STORAGE(%esp), r1

#define SAVE2(r1, r2)   \
        SAVE1(r1);      \
        movl    r2, (4 + FIXUP_FRAMES_STORAGE)(%esp)
#define RESTORE2(r1, r2)        \
        RESTORE1(r1);   \
        movl    (4 + FIXUP_FRAMES_STORAGE)(%esp), r2

#define FIXUP_FRAMES_N(top, tmp, save, restore, label) \
	movl OFFSET_CVMFrame_prevX(top), tmp;          \
	andl $CONSTANT_CVM_FRAME_MASK_ALL, tmp;        \
	jne label;                                     \
	/* save regs used in CVMJITfixupFrames */      \
        save;                                          \
        pushl top;                                     \
	CALL_VM_FUNCTION(SYM_NAME(CVMJITfixupFrames));           \
        popl top;                                      \
	/* restore regs, top got popped in callee */   \
        restore;                                       \
0:

#define FIXUP_FRAMES_0(top, scratch, label) FIXUP_FRAMES_N(top, scratch, /* */, /* */, label)
#define FIXUP_FRAMES_1(jfp, scratch, r1, label)        \
        FIXUP_FRAMES_N(jfp, scratch, SAVE1(r1), RESTORE1(r1), label)
#define FIXUP_FRAMES_2(jfp, scratch, r1, r2, label)    \
        FIXUP_FRAMES_N(jfp, scratch, SAVE2(r1, r2), RESTORE2(r1, r2), label)

/* TODO(rr): check if stackframes are 4 byte aligned */
#define STACK_ALIGN32 4
#define SA32(X) (((X)+(STACK_ALIGN32-1)) & ~(STACK_ALIGN32-1))
#define SA(X) SA32(X)


/*
 * Internally the JIT compiler uses CVMCPU_ARG1_REG .. CVMCPU_ARG4_REG
 * as Registers to pass around non Java arguments (e.g. method block,
 * class block, ...). CALL_HELPER_N produces code to call a helper
 * routine implemented in C and pass the arguments according to the C
 * calling convention. CALL_HELPER_N expect other macros, let's call
 * them marshal macros, that know how to pass a single
 * argument. Marshal macros expect the argument number in the
 * parameter list (first parameter has number 0). There are
 * specialized marshal macros like CCEE_AS_ARG and one general named
 * AS_ARG. This one expects an operand in assembler syntax.
 *
 * Example: 
 *   o you want to call CVMCCMruntimeNew(CVMCCExecEnv *ccee, CVMClassBlock *newCb)
 *   o the second parameter is in A2 ( == %ebx )
 *   o you don't care about the value in %eax (first parameter to CALL_HELPER_N
 *     is a register that can be used for temp values)
 *
 * CALL_HELPER_2(%eax, CCEE_AS_ARG, AS_ARG(A2), CVMCCMruntimeNew)
 *
 *
 * Note: Call_HELPER_N modifies %esp while constructing the call!
 *
 * If you like, we can discuss this interface (or the beauty of C
 * macros) in more detail on a beer or two.  sw/rr.
 *
 */


/* macros to marshal arguments */
#define AND_FORGET_NUMBER(number)
#define AS_ARG(arg)             pushl   arg AND_FORGET_NUMBER
#define AS_ARG_FROM_STACK(offset)	pushl   (offset) + ARGS_ABOVE
#define ARGS_ABOVE(number)		number * 4 (%esp)
#define CCEE_AS_ARG(number)	pushl	%esp ;  addl $(( number + 1 ) * 4), (%esp)
#define EE_AS_ARG(number)	pushl	((( number + 1 ) * 4) + OFFSET_CVMCCExecEnv_ee)(%esp)

/* To allow for C stack walking this macro creates a C frame that looks like one
 * created by this standard x86 function prolog:
 *	pushl	%ebp
 *	movl	%esp, %ebp
 *
 * The assumption made by this macro is that the original frame pointer is saved
 * 4 stack slots above the CVMCCExecEnv (see CVMJITgoNative() in jit_cpu.S).
 *
 * Because %ebp is used as JFP setting %ebp to the created frame (thus making it
 * a CFP) is deferred until actually calling the helper (see CALL_HELPER() below).
 */
#define CALL_HELPER_PROLOG_N(scratch, nsargs)				\
	leal	(4 * (4 + nsargs) + SA(CONSTANT_CVMCCExecEnv_size))(%esp), scratch;	\
	pushl	scratch;						\
	movl	4(%esp), scratch;   /* return address */		\
        subl    $4, %esp;	    /* make room for JFP spilling */

#define CALL_HELPER_PROLOG(scratch) CALL_HELPER_PROLOG_N(scratch, 0)

#define CALL_HELPER_0(scratch, helper)                                  \
        CALL_HELPER_0_NO_RET(scratch, helper);                          \
        ret;

#define CALL_HELPER_0_NO_RET(scratch, helper)				\
	CALL_HELPER_PROLOG(scratch);					\
	CALL_HELPER(scratch, 0, helper); 

#define CALL_HELPER_1(scratch, ARG0, helper)			        \
        CALL_HELPER_1_NO_RET(scratch, ARG0, helper);                    \
        ret;

#define CALL_HELPER_1_NO_RET(scratch, ARG0, helper)			\
	CALL_HELPER_PROLOG(scratch);					\
	ARG0(2);							\
	CALL_HELPER(scratch, 1, helper); 

#define CALL_HELPER_2(scratch, ARG0, ARG1, helper)		        \
        CALL_HELPER_2_NO_RET(scratch, ARG0, ARG1, helper);              \
        ret;

#define CALL_HELPER_2_NO_RET(scratch, ARG0, ARG1, helper)		\
	CALL_HELPER_PROLOG(scratch);					\
	ARG1(2);							\
	ARG0(3);							\
	CALL_HELPER(scratch, 2, helper); 

#define CALL_HELPER_3(scratch, ARG0, ARG1, ARG2, helper)	        \
        CALL_HELPER_3_NO_RET(scratch, ARG0, ARG1, ARG2, helper);        \
        ret;

#define CALL_HELPER_3_NO_RET(scratch, ARG0, ARG1, ARG2, helper)		\
	CALL_HELPER_PROLOG(scratch);					\
	ARG2(2);							\
	ARG1(3);							\
	ARG0(4);							\
	CALL_HELPER(scratch, 3, helper); 

/*

Before CALL_HELPER_4
 
        |
        |       CVMCCExecEnv
        |       [ee, chunkend, ...]
        |
        +============================
 SP  -->|       return address
        +----------------------------

*/

#define CALL_HELPER_4(scratch, ARG0, ARG1, ARG2, ARG3, helper)	        \
        CALL_HELPER_4_NO_RET(scratch, ARG0, ARG1, ARG2, ARG3, helper);  \
        ret;

#define CALL_HELPER_4_NO_RET(scratch, ARG0, ARG1, ARG2, ARG3, helper)	\
	CALL_HELPER_PROLOG(scratch);					\
	ARG3(2);							\
	ARG2(3);							\
	ARG1(4);							\
	ARG0(5);							\
	CALL_HELPER(scratch, 4, helper); 


/*

Before CALL_HELPER
 
        |
        |       CVMCCExecEnv
        |       [ee, chunkend, ...]
        |
        +============================
        |       return address
        +----------------------------
        |       ARG3
        +----------------------------
        |       ARG2
        +----------------------------
        |       ARG1
        +----------------------------
 SP  -->|       ARG0
        +----------------------------

*/

#define CALL_HELPER(ret_addr, nargs, helper)									\
	movl	ret_addr, OFFSET_CVMCompiledFrame_PC(JFP);							\
	movl	JSP, OFFSET_CVMFrame_topOfStack(JFP);								\
	movl	( (nargs + 3) * 4 + OFFSET_CVMCCExecEnv_ee )(%esp), ret_addr; /* ret_addr now holds ee */	\
        movl	JFP, (OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame)(ret_addr);		\
        movl	JFP, (nargs * 4)(%esp); /* spill JFP */								\
	/* modify %ebp temporarily to allow for C stack walking */						\
	leal    ( (nargs + 1) * 4)(%esp), %ebp; /* %ebp (JFP) now holds CFP */					\
	CALL_VM_FUNCTION(helper);										\
        movl	(nargs * 4)(%esp), JFP; /* restore JFP */							\
        movl	OFFSET_CVMFrame_topOfStack(JFP), JSP;								\
        addl    $( (nargs + 2) * 4), %esp;


#define CVMJITX86_CMPXCHG(reg32, mem32) lock ; cmpxchgl reg32, mem32
#define CVMJITX86_CMPXCHG8B(mem64) lock ; cmpxchg8b mem64

#endif /* _INCLUDED_JITASMMACROS_CPU_H */
