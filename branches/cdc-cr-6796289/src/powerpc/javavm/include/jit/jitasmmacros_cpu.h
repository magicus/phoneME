/*
 * @(#)jitasmmacros_cpu.h	1.15 06/10/10
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

#ifndef _INCLUDED_JITASMMACROS_CPU_H
#define _INCLUDED_JITASMMACROS_CPU_H

#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * Shorthand for registers that are also defined in jitasmconstants_cpu.h
 */
#define JFP CVMPPC_JFP_REGNAME
#define JSP CVMPPC_JSP_REGNAME
#define CHUNKEND CVMPPC_CHUNKEND_REGNAME
#define CVMGLOBALS CVMPPC_CVMGLOBALS_REGNAME
#define EE CVMPPC_EE_REGNAME
#define CP CVMPPC_CP_REGNAME

/*
 * Normally we the code in ccmcodecachecopy_cpu.o to the code cache
 * so it can be called with a bl instead of an ldr. However, in order
 * to support debugging, we also allow you to not copy the code and
 * run it directly out of ccmcodecachecopy_cpu.o. For this reason, we need to
 * macroize how we call functions outside of ccmcodecachecopy_cpu.o.
 */
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define CALL_VM_FUNCTION(VMFUNCTION)	\
	bl SYM_NAME(VMFUNCTION)
#define BRANCH_TO_VM_FUNCTION(VMFUNCTION)	\
	b  SYM_NAME(VMFUNCTION)
#else
#define CALL_VM_FUNCTION(VMFUNCTION)		\
	lis   r12, HA16(VMFUNCTION)		_SE_\
	la    r12, LO16(VMFUNCTION)(r12)	_SE_\
        mtctr r12				_SE_\
        bctrl
#define BRANCH_TO_VM_FUNCTION(VMFUNCTION)	\
	lis   r12, HA16(VMFUNCTION)		_SE_\
	la    r12, LO16(VMFUNCTION)(r12)	_SE_\
        mtctr r12				_SE_\
        bctr
#endif

/*
 * Macro to assist with fixing up compiled frames. It only bothers
 * calling CVMJITfixupFrames if the frames need fixing.
 *
 * frame: frame to start flushing at (probably always JFP)
 * lrReg: LR gets trashed if CVMJITfixupFrames is called, but will
 *        be restored to the contents of lrReg. 
 *
 * Trashes r0 and r12. All other registers are preserved.
 *
 * TODO: we should consider more variations of FIXUP_FRAMES that just
 * restores the required registers.
 */

/* NOTE: we use 3 words of ccmStorage[] starting at offset 12. */
#define FIXUP_FRAMES_STORAGE \
	OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+12
#define FIXUP_FRAMES(frame, lrReg)			    \
        lwz   r0, OFFSET_CVMFrame_prevX(frame)		_SE_\
        andi. r0, r0, CONSTANT_CVM_FRAME_MASK_ALL	_SE_\
	bne+  0f					_SE_\
	stw   r3, FIXUP_FRAMES_STORAGE(sp)		_SE_\
	stw   r4, FIXUP_FRAMES_STORAGE+4(sp)		_SE_\
	stw   r5, FIXUP_FRAMES_STORAGE+8(sp)		_SE_\
	mr    r3, frame					_SE_\
	CALL_VM_FUNCTION(CVMJITfixupFrames)		_SE_\
	lwz   r3, FIXUP_FRAMES_STORAGE(sp)		_SE_\
	lwz   r4, FIXUP_FRAMES_STORAGE+4(sp)		_SE_\
	lwz   r5, FIXUP_FRAMES_STORAGE+8(sp)		_SE_\
	mtlr  lrReg					_SE_\
	0:
#define FIXUP_FRAMES_0(frame, lrReg)		            \
        lwz   r0, OFFSET_CVMFrame_prevX(frame)		_SE_\
        andi. r0, r0, CONSTANT_CVM_FRAME_MASK_ALL	_SE_\
	bne+  0f					_SE_\
	mr    r3, frame					_SE_\
	CALL_VM_FUNCTION(CVMJITfixupFrames)		_SE_\
	mtlr  lrReg					_SE_\
	0:

/*
 * Location in the CVMExecEnv.ccmStorage where we save and restore
 * the CR registers. No one else shoule use this slot.
 */
#define CVMCCExecEnv_ccmStorage_CR \
    OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+8*4

#endif /* _INCLUDED_JITASMMACROS_CPU_H */
