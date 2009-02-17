/*
 * @(#)ccminvokers_cpu.S	1.5 06/10/24
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

.file "ccminvokers_cpu.S"

#define NEW_JFP CVMX86_NEWJFP_REGNAME 
#define PREV    CVMX86_PREVFRAME_REGNAME

/* 
 * Helpers for invoking from and returning from compiled methods. 
 */ 

/* CVMCCMinvokeJNIMethod */
ENTRY(CVMCCMinvokeJNIMethod )
	# A1 = target mb
	
	FIXUP_FRAMES_2(JFP, A4 /* scratch */, A2, A3, CVMCCMinvokeJNIMethod_L1)
CVMCCMinvokeJNIMethod_L1:
	CALL_HELPER_2_NO_RET(A4 /* scratch */, EE_AS_ARG, AS_ARG(A1), SYM_NAME(CVMinvokeJNIHelper))

	/* jump to returnToInterpreter0 if retval is 0 */
	cmpl $0, %eax
	jne CVMCCMinvokeJNIMethod_L2
	addl $4, %esp
	jmp returnToInterpreter0	
CVMCCMinvokeJNIMethod_L2:
	ret
SET_SIZE( CVMCCMinvokeJNIMethod )

/* CVMCCMletInterpreterDoInvoke */
ENTRY(CVMCCMletInterpreterDoInvoke )
#define SCRATCH A4	
letInterpreterDoInvoke_store_lr:
	popl	SCRATCH		# return address
	movl	SCRATCH, OFFSET_CVMCompiledFrame_PC(JFP)
	
ENTRY(CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr ) 
letInterpreterDoInvoke: 
        /* 
         * Trying to invoke something beyond our ability. 
         * Return the mb to the interpreter and let it do the 
         * dirty work. 
         * we have already set up the return PC in our own frame 
         * We need to set topOfStack then return the target MB* 
         * as a C return value. 
         */ 
	FIXUP_FRAMES_0(JFP, SCRATCH, CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr_L1)
CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr_L1:	

        movl	(OFFSET_CVMCCExecEnv_ee)(%esp), SCRATCH
        movl	JSP, OFFSET_CVMFrame_topOfStack(JFP) 
        movl	JFP, (OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame)(SCRATCH)
	addl	$SA(CONSTANT_CVMCCExecEnv_size), %esp
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
#undef SCRATCH

	#
	# Do a GC check, and rendezvous if one is going on
	#
handleGCForReturn:
	hlt					
/*
 * The GC checks for the various return variants
 */
handleGCForDoReturn:	
	hlt
	
handleGCForDoSyncReturn:	
	hlt
	
/* CVMCCMinvokeCNIMethod */
ENTRY(CVMCCMinvokeCNIMethod )
#define SCRATCH A4
	/* A1 = target mb */
	
	movzbl OFFSET_CVMMethodBlock_argsSizeX(A1), A3
	shl $2, A3
	movl JSP, A2
	subl A3, A2 /* A2 now points to args */

	FIXUP_FRAMES_1(JFP, SCRATCH, A2, CVMCCMinvokeCNIMethod_L1)
CVMCCMinvokeCNIMethod_L1:

	CALL_HELPER_PROLOG(SCRATCH)
	pushl A2              /* spill arg pointer (A2) */	

	/* push args with correct offsets */
	/* **p_mb */
	pushl %esp
	addl $(16 + OFFSET_CVMCCExecEnv_ccmStorage), 0(%esp)
	movl A1,  20 + OFFSET_CVMCCExecEnv_ccmStorage(%esp)
	/* arguments */
	AS_ARG(A2)(4)
	/* ee */
	EE_AS_ARG(5)

	/* issue the call */
	movl OFFSET_CVMMethodBlock_codeX(A1), A3
	CALL_HELPER(SCRATCH, 4, *A3)

	movl -12(%esp), A2 /* restore arg pointer */

	/* if %eax >= 0 then %eax is the size in words of the method result
	   note that %eax is A1 */
	cmpl $0, A1
	jl CVMCCMinvokeCNIMethod_L2

	/* pop args and adjust for result */
	shl $2, A1
	movl A2, JSP
	addl A1, JSP
	
	/* retval is already in %eax / A1 */
	ret

CVMCCMinvokeCNIMethod_L2:
	/* check if a new mb to execute has been returned */
	cmpl $CONSTANT_CNI_NEW_MB, A1
	jne new_transition

	/* A1 = newMb */
	movl 4 + OFFSET_CVMCCExecEnv_ccmStorage(%esp), A1

	/* adjust TOS 'cause new method may have fewer args than the CNI method */
	movzbl OFFSET_CVMMethodBlock_argsSizeX(A1), A3
	shl $2, A3
	movl A2, JSP
	addl A3, JSP
	movl JSP, OFFSET_CVMFrame_topOfStack(JFP)

	addl $4, %esp /* pop return address */
	jmp returnToInterpreter1

new_transition:
	/* check if a new transition frame to execute has been setup */
	cmpl $CONSTANT_CNI_NEW_TRANSITION_FRAME, A1
	je CVMCCMinvokeCNIMethod_L3
	addl $4, %esp /* pop return address */
	jmp returnToInterpreter
CVMCCMinvokeCNIMethod_L3:

	movl A2, OFFSET_CVMFrame_topOfStack(JFP) /* pop args */
	addl $4, %esp /* pop return address */
	jmp returnToInterpreter0

#undef SCRATCH
SET_SIZE( CVMCCMinvokeCNIMethod ) 

/* CVMCCMreturnFromSyncMethod */
ENTRY(CVMCCMreturnFromSyncMethod )
returnToNativeDoSyncReturn:
#if 1 /* TODO(rr): port SPARC assembler instead of using interpreter */
	#
	# If you just want to call the C helper and write very little assembler
	# code, then just to branch to (and implement) returnToInterpreter.
	#
	jmp	returnToInterpreter
#endif
SET_SIZE( CVMCCMreturnFromSyncMethod )

/* 
 * CVMCCMreturnFromMethod
 *
 * Native code doing a return comes here. 
 * It may as well branch, since the return address is not interesting. 
 */
ENTRY(CVMCCMreturnFromMethod )
	/*  FIXME: For now let's just forget about GC check. */
	#b	handleGCForDoReturn
	#nop
	/* The GC check for non-sync returns comes back here */
returnToNativeDoReturn:	
        # see if previous frame is compiled or not 
        movl	OFFSET_CVMFrame_prevX(JFP), PREV
        andl	$CONSTANT_CVM_FRAME_MASK_SLOW, PREV
	jnz 	returnToInterpreter
doReturnToCompiled:

/*
	# TODO(SW): enable tracing
#ifdef CVM_TRACE
	mov	JFP, %o1
	CALL_VM_FUNCTION(CCMtraceMethodReturn)
	mov	EE, %o0
#endif
*/
        # returning from one native to another. 
        # do this ourselves. 
	# java sp already set
	movl	OFFSET_CVMFrame_prevX(JFP), JFP
	andl	$ ~CONSTANT_CVM_FRAME_MASK_ALL, JFP

        movl	OFFSET_CVMCompiledFrame_PC(JFP), %ebx

	/* TODO(SW): guess we don't need this -> check it */
#ifdef CVMCPU_HAS_CP_REG
#	ld	[JFP + OFFSET_CVMCompiledFrame_cpBaseRegX], CP
#endif
	jmpl *	%ebx


/* CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr */
ENTRY(CVMCCMreturnToInterpreter )
#define SCRATCH A1
returnToInterpreter:
	FIXUP_FRAMES_0(JFP, SCRATCH, returnToInterpreter2)
returnToInterpreter2:
	
	# ld [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee], EE
	movl OFFSET_CVMCCExecEnv_ee(%esp), SCRATCH

	# JSP needs to point just past any return value
        movl JSP, OFFSET_CVMFrame_topOfStack(JFP)

        # set stack->currentFrame to current value of JFP, 
        # then return NULL, meaning we donnot want the interpreter 
        # to take any further action on our behalf (except pop
	# the current frame)

        movl JFP, (OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame)(SCRATCH)
#undef SCRATCH
returnToInterpreter0:
        movl $0, %eax
returnToInterpreter1:
	addl $SA(CONSTANT_CVMCCExecEnv_size), %esp
	popl %ebx
	popl %edi
	popl %esi
	popl %ebp
	ret

SET_SIZE( CVMCCMreturnFromMethod )

/* CVMCCMtraceMethodCallGlue */
ENTRY(CVMCCMtraceMethodCallGlue )
	hlt # TODO(SW)
SET_SIZE( CVMCCMtraceMethodCallGlue )

/* CVMCCMinvokeStaticSyncMethodHelper */
ENTRY(CVMCCMinvokeStaticSyncMethodHelper )
        # A1 = target mb 
#if 1 /* TODO(rr): implement this in assembly */
	# If you just want to call the C helper and write very little assembler
	# code, then just to branch to (and implement) letInterpreterDoInvoke.
	#
	addl	$4, %esp			/* don't need return address */
        jmp 	letInterpreterDoInvoke
#endif
SET_SIZE( CVMCCMinvokeStaticSyncMethodHelper )

/* CVMCCMinvokeNonstaticSyncMethodHelper */
ENTRY(CVMCCMinvokeNonstaticSyncMethodHelper )
	# A1 = target mb
	# A2 = CVMObjectICell* of object to sync on
#if 1 /* TODO(rr) port SPARC assembler */
	# If you just want to call the C helper and write very little assemble
	# code, then just to branch to (and implement) letInterpreterDoInvoke.
	#
	addl	$4, %esp			/* don't need return address */
	jmp	letInterpreterDoInvoke
#endif
SET_SIZE( CVMCCMinvokeNonstaticSyncMethodHelper )
