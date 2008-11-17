/*
 * @(#)jit_cpu.S	1.19 06/10/10
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
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"

	.text
	.align	8

#undef SA
#define SA(X) (((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))

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
ENTRY( CVMJITgoNative )
	! %o0 = exceptionObject - unused unless returning to exception handler
	! %o1 = EE
	! %o2 = JFP
	! %o3 = pc

        save %sp,-SA(MINFRAME+CONSTANT_CVMCCExecEnv_size+8),%sp

	! Note: Now %o registers renamed as %i registers.
        ! set up CVMCPU_CHUNKEND_REG
	ld	[%i1 + OFFSET_CVMExecEnv_interpreterStack+ OFFSET_CVMStack_stackChunkEnd], CHUNKEND
        st %i1, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_ee]
	st CHUNKEND, [%sp + MINFRAME + OFFSET_CVMCCExecEnv_stackChunkEnd]
        st %i6, [%sp + MINFRAME + CONSTANT_CVMCCExecEnv_size]
        st %i7, [%sp + MINFRAME + CONSTANT_CVMCCExecEnv_size+4]
	mov	%i2, JFP
	ld	[JFP + OFFSET_CVMFrame_topOfStack], JSP

	! If we are returning to compiled code with an exceptionObject,
	! compiled code expects the exceptionObject in %o0 register, so
	! we need to move it from %i0 to %o0.
	mov	%i0, %o0

#ifdef	CVMCPU_HAS_CP_REG
	ld	[JFP + OFFSET_CVMCompiledFrame_cpBaseRegX], CP
#endif
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	/*
	 * Load CVMSPARC_GC_REGNAME with the address that will cause a trap
	 * when a gc is requested.
	 */
	sethi	%hi(SYM_NAME(CVMgcTrapAddrPtr)), %CVMSPARC_GC_REGNAME
        ld      [%CVMSPARC_GC_REGNAME+%lo(SYM_NAME(CVMgcTrapAddrPtr))], %CVMSPARC_GC_REGNAME
        ld      [%CVMSPARC_GC_REGNAME], %CVMSPARC_GC_REGNAME
#endif

        jmp %i3
        mov %i1, EE
SET_SIZE( CVMJITgoNative ) 

/* 
 * Return from C helper function to interpreter. 
 * Basically longjmp.
 * void
 * CVMexitNative(CVMCCExecEnv*cc ee); 
 */ 
ENTRY( CVMJITexitNative ) 
	/* %o0 = ccee */

        /* return NULL, meaning we don't want the interpreter */
        /* to take any further action on our behalf */

	/* flush register windows */
	!ta	ST_FLUSH_WINDOWS
	FLUSH_WINDOWS
	/* restore from correct frame */
        ld	[%o0 + CONSTANT_CVMCCExecEnv_size], %i6
        ld	[%o0 + CONSTANT_CVMCCExecEnv_size+4], %i7

        mov	0, %i0
	ret
	restore
SET_SIZE( CVMJITexitNative ) 

	
/*
 * Fixup up uninitialized fields in compiled frames
 * extern void
 * CVMJITfixupFrames(CVMFrame *);
 */
ENTRY ( CVMJITfixupFrames )
#define CFP %o0
#define PREV %o1
#define TMP %o2
        ld  [CFP + OFFSET_CVMFrame_prevX], PREV
0:
	mov CONSTANT_CVM_FRAMETYPE_COMPILED, TMP
	stb TMP, [CFP + OFFSET_CVMFrame_type]
        stb %g0, [CFP + OFFSET_CVMFrame_flags]
        or  PREV, CONSTANT_CVM_FRAME_MASK_SPECIAL, TMP
        st  TMP, [CFP + OFFSET_CVMFrame_prevX]
	mov PREV, CFP
        ld  [CFP + OFFSET_CVMFrame_prevX], PREV
        andcc PREV, CONSTANT_CVM_FRAME_MASK_ALL, %g0
	beq 0b
	nop
	retl
	nop
#undef CFP
#undef PREV
#undef TMP
SET_SIZE( CVMJITfixupFrames ) 
