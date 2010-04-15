/*
 * @(#)jitasmmacros_arch.h	1.8 06/10/10
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

#ifndef _INCLUDED_JITASMMACROS_ARCH_H
#define _INCLUDED_JITASMMACROS_ARCH_H

#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"

/*
 * Shorthand for registers that are also defined in jitasmconstants_cpu.h
 */
#define JFP CVMARM_JFP_REGNAME
#define JSP CVMARM_JSP_REGNAME

/*
 * Normally we the code in ccmcodecachecopy_cpu.o to the code cache
 * so it can be called with a bl instead of an ldr. However, in order
 * to support debugging, we also allow you to not copy the code and
 * run it directly out of ccmcodecachecopy_cpu.o. For this reason, we need to
 * macroize how we call functions outside of ccmcodecachecopy_cpu.o.
 */

#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define CALL_VM_FUNCTION(VMFUNCTION)	\
	M_CALL_VM_FUNCTION VMFUNCTION

	MACRO
	M_CALL_VM_FUNCTION $VMFUNCTION
	IMPORT $VMFUNCTION
	bl $VMFUNCTION
	MEND

#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION)	\
	M_BRANCH_TO_VM_FUNCTION CCMFUNCTION

	MACRO
	M_BRANCH_TO_VM_FUNCTION $CCMFUNCTION
	IMPORT $CCMFUNCTION
	b  $CCMFUNCTION
	MEND
#else
#define CALL_VM_FUNCTION(VMFUNCTION)	\
	M_CALL_VM_FUNCTION VMFUNCTION

	MACRO
	M_CALL_VM_FUNCTION $VMFUNCTION
	IMPORT $VMFUNCTION
	mov lr, pc
	ldr pc, =$VMFUNCTION
	MEND

#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION)	\
	M_BRANCH_TO_VM_FUNCTION CCMFUNCTION

	MACRO
	M_BRANCH_TO_VM_FUNCTION $CCMFUNCTION
	IMPORT $CCMFUNCTION
	ldr pc, =$CCMFUNCTION
	MEND
#endif


/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * calling CVMJITfixupFrames if the frames need fixing.
 */
	MACRO
	M_FIXUP_FRAMES_N $top, $save, $restore
	ldr ip, [$top,#OFFSET_CVMFrame_prevX]
	tst ip, #CONSTANT_CVM_FRAME_MASK_ALL
	bne %f2
	$save
	mov a1, $top
	CALL_VM_FUNCTION(CVMJITfixupFrames)
	$restore
2
	MEND

	MACRO
	M_FIXUP_FRAMES_0	$top
	M_FIXUP_FRAMES_N $top
	MEND

	MACRO
	M_SAVE	$saveset
        stmfd sp!, $saveset
	MEND

	MACRO
	M_RESTORE	$saveset
        ldmfd sp!, $saveset
	MEND


#define FIXUP_FRAMES_0(top) M_FIXUP_FRAMES_0 top
#define SAVE(saveset)	M_SAVE saveset
#define RESTORE(saveset)	M_RESTORE saveset
#define _FIXUP_FRAMES(top, saveset)	\
    M_FIXUP_FRAMES_N top, SAVE(saveset), RESTORE(saveset)

/* only a1, a2, ip, and lr are trashed */
SAVESET1234	RLIST {a1, a2, a3}
SAVESET1234lr	RLIST {a1, a2, a3, lr}
SAVESET23lr	RLIST {a2, a3, lr}
SAVESET34lr	RLIST {a3, lr}
SAVESET3lr	RLIST {a3, lr}
SAVESET1	RLIST {a1}
SAVESET12	RLIST {a1, a2}
SAVESET12lr	RLIST {a1, a2, lr}

	MACRO
	RETURN
	ldmfd   sp, {v1-v7, fp, sp, pc}
	MEND


#endif /* _INCLUDED_JITASMMACROS_ARCH_H */
