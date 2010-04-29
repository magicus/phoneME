/*
 * @(#)jit_cpu.S	1.6 06/10/24
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

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

	.text
	.align	8
	.file "jit_cpu.S"
		
/* 
 * A note on C stack usage: 
 * There will be at most one native code frame for compiled methods on the
 * stack per interpreter invocation. The native code frame is shared by any
 * chain of compiled methods. It is created by CVMJITgoNative() and also
 * includes space for the CVMCCExecEnv struct.
 *
 * Recursion back into the interpreter only occurs if a compiled methods calls
 * a JNI method which invokes another Java method, or if the compiled method
 * calls a CCM helper which needs to execute Java bytecodes.  But if a
 * compiled method is going to call an interpreted method, we always return
 * back to the interpreter, even if there are still compiled frames on the
 * stack.  The interpreter and compiled code act like co-routines.
 */

/* 
 * Entry point from interpreted code: 
 * CVMMethodBlock* 
 * CVMJITgoNative(CVMObject* exceptionObject, CVMExecEnv* ee, 
 *	          CVMCompiledFrame *jfp, CVMUint8 *pc); 
 */

/*

C-Stack before execution of CVMJITgoNative():

        |                     
        |    Interpreter Frame
        |                     
        +============================
        | pc
        +----------------------------
     	| jfp
        +----------------------------
     	| ee
        +----------------------------
        | exceptionObject - unused unless returning to exception handler
        +----------------------------
 SP  -->| rp
        +============================



C-Stack after execution of CVMJITgoNative():


        |                     
        |    Interpreter Frame
        |                     
        +============================
        | pc
        +----------------------------
     	| jfp
        +----------------------------
     	| ee
        +----------------------------
        | exceptionObject - unused unless returning to exception handler
        +----------------------------
        | rp                         
        +----------------------------
        | old FP (%ebp)
        +----------------------------
        | callee saved registers
        | esi
	| edi
	| ebx
        +----------------------------
        |
        | CVMCCExecEnv
        | [ee, chunkend, ...]
 SP  -->|
        +============================

	ARG1_REG: exceptionObject
		
*/	

ENTRY(X86JITgoNative )
	
	# save rp, sp, bp, esi, edi, ebx
	pushl %ebp			# old fp
	pushl %esi			# callee saved
	pushl %edi			# callee saved
	pushl %ebx			# callee saved

	# load arguments
	movl (4 + 4*4)(%esp), %eax		# exception object
	movl (8 + 4*4)(%esp), %edx		# ee
	movl (12 + 4*4)(%esp), %ebx		# jfp
	movl (16 + 4*4)(%esp), %ecx		# pc
	
#define SCRATCH %esi

	# push frame for compiled code on C stack
	subl $SA(CONSTANT_CVMCCExecEnv_size), %esp

        # no CVMCPU_CHUNKEND_REG
	movl (OFFSET_CVMExecEnv_interpreterStack + OFFSET_CVMStack_stackChunkEnd)(%edx), SCRATCH
	movl SCRATCH,  OFFSET_CVMCCExecEnv_stackChunkEnd(%esp)
	
        movl %edx, OFFSET_CVMCCExecEnv_ee(%esp)

	movl %ebx, JFP
	movl (OFFSET_CVMFrame_topOfStack)(JFP), JSP

	# TODO(rr): do something for exception handling

#ifdef	CVMCPU_HAS_CP_REG
	# TODO(rr): do something for CVMCPU_CP_REG
#endif

/* Set up rGC if Trap-based GC Checks are enabled */
#ifdef  CVMJIT_TRAP_BASED_GC_CHECKS
	movl $SYM_NAME(CVMgcTrapAddrPtr), CVMX86_GC_REGNAME
	movl (CVMX86_GC_REGNAME), CVMX86_GC_REGNAME
	movl (CVMX86_GC_REGNAME), CVMX86_GC_REGNAME
#ifdef  CVMCPU_HAS_VOLATILE_GC_REG
	/* save CVMX86_GC_REGNAME in the ccee for easier access later */
	movl CVMX86_GC_REGNAME, OFFSET_CVMCCExecEnv_gcTrapAddr(%esp)
#endif  /* if CVMCPU_HAS_VOLATILE_GC_REG */
#endif  /* if CVMJIT_TRAP_BASED_GC_CHECKS */

	# TODO(rr): do somethind for CVMCPU_EE_REG
        jmp *%ecx

#undef SCRATCH
	
SET_SIZE( X86JITgoNative ) 

/* 
 * Return from C helper function to interpreter. 
 * Basically longjmp.
 * void
 * CVMexitNative(CVMCCExecEnv *ccee); 
 */
ENTRY(CVMJITexitNative )
	# pop frame for compiled code from C stack
	movl 4(%esp), %esp	
	addl $SA(CONSTANT_CVMCCExecEnv_size), %esp
	# restore rp, sp, bp
	popl %ebx
	popl %edi
	popl %esi
	popl %ebp		 # old fp, now rp is TOS
	movl $0, %eax		 # return value NULL
	ret			 # return
SET_SIZE( CVMJITexitNative ) 
	
/*
 * Fixup up uninitialized fields in compiled frames
 * extern void
 * CVMJITfixupFrames(CVMFrame *frame);
 */
ENTRY(CVMJITfixupFrames)
/*
 * CVMJITfixupFrames(frame *)
 * will be called after C calling convention so the incomming argument
 * can be found on the stack.
 * 
 */
	
#define CFP	A2  /* holds frame */
#define PREV    A3  /* killed ...see FIXUP_FRAMES_0|1|2|3*/
#define PREVb   A3b /* killed ...see FIXUP_FRAMES_0|1|2|3*/
	pushl   A3		# A3 == ebx is callee saved after c convention
	movl	8(%esp), CFP
	movl	OFFSET_CVMFrame_prevX(CFP), PREV
CVMJITfixupFrames_recurse:
	movb	$CONSTANT_CVM_FRAMETYPE_COMPILED, OFFSET_CVMFrame_type(CFP)
	movb	$0, OFFSET_CVMFrame_flags(CFP)
	orl	$CONSTANT_CVM_FRAME_MASK_SPECIAL, OFFSET_CVMFrame_prevX(CFP)
	movl	PREV, CFP
	movl	OFFSET_CVMFrame_prevX(CFP), PREV
	testb	$CONSTANT_CVM_FRAME_MASK_ALL, PREVb
	je	CVMJITfixupFrames_recurse
	popl    A3
	retl
#undef CFP
#undef PREV
SET_SIZE( CVMJITfixupFrames ) 
