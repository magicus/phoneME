/*
 * @(#)jitasmmacros_cpu.h	1.12 06/10/10
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
#include "javavm/include/iai_opt_config.h"

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

#ifdef __RVCT__

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

#else /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */

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
#endif /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */

#define BIT_AND :AND:

#else /* !__RVCT__ */

#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE

#define CALL_VM_FUNCTION(VMFUNCTION)	\
	bl VMFUNCTION

#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION)	\
	b  CCMFUNCTION

#else /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */

#define CALL_VM_FUNCTION(VMFUNCTION)	\
	mov lr, pc;			\
	ldr pc, SYMBOL(VMFUNCTION)

#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION)	\
	ldr pc, SYMBOL(CCMFUNCTION)

#endif /* CVM_JIT_COPY_CCMCODE_TO_CODECACHE */

#define BIT_AND &

#endif /* __RVCT__ */

#ifndef __RVCT__
/* IAI - 14 */
#ifndef IAI_FIXUP_FRAME_VMFUNC_INLINING
/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * calling CVMJITfixupFrames if the frames need fixing.
 */
#define FIXUP_FRAMES_N(top, save, restore)	\
        ldr ip, [top,#OFFSET_CVMFrame_prevX];	\
        tst ip, #CONSTANT_CVM_FRAME_MASK_ALL;	\
	bne 0f;					\
	save;					\
	mov a1, top;				\
	CALL_VM_FUNCTION(CVMJITfixupFrames);	\
	restore;				\
0:
#else /* IAI - 14 */
/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * inlining execution the CVMJITfixupFrames function if the frames need fixing.
 */
#define FIXUP_FRAMES_N(top, save, restore)	\
        ldr ip, [top,#OFFSET_CVMFrame_prevX];	\
        tst ip, #CONSTANT_CVM_FRAME_MASK_ALL;	\
	bne 0f;					\
	save;					\
	mov a1, top;				\
        ldr a2, [a1,#OFFSET_CVMFrame_prevX];\
        mov a3, #0;\
1:\
        mov ip, #CONSTANT_CVM_FRAMETYPE_COMPILED;\
        strb ip, [a1, #OFFSET_CVMFrame_type];\
        strb a3, [a1, #OFFSET_CVMFrame_flags];\
        orr ip, a2, #CONSTANT_CVM_FRAME_MASK_SPECIAL;\
        str ip, [a1,#OFFSET_CVMFrame_prevX];\
        mov a1, a2;\
        ldr a2, [a1,#OFFSET_CVMFrame_prevX];\
        tst a2, #CONSTANT_CVM_FRAME_MASK_ALL;\
        beq 1b;\
	restore;				\
0:
#endif
/* IAI - 14 */

#define FIXUP_FRAMES_0(top) FIXUP_FRAMES_N(top, /* */, /* */)
#define SAVE(saveset)	\
	stmfd sp!, saveset
#define RESTORE(saveset)	\
	ldmfd sp!, saveset
#define FIXUP_FRAMES(top, saveset, num)	\
	FIXUP_FRAMES_N(top, SAVE(saveset), RESTORE(saveset))
#else /* __RVCT__ */

/*
 * The "RLIST" directive could also be used to pass the
 * register set as a single token.
 */
	MACRO
	FIXUP_FRAMES_N $top, $num, $save1, $save2, $save3, $save4
        ldr ip, [$top,#OFFSET_CVMFrame_prevX]
        tst ip, #CONSTANT_CVM_FRAME_MASK_ALL
	bne %f1
	IF $num = 1
		stmfd sp!, $save1
	ELSE
	IF $num = 2
		stmfd sp!, $save1, $save2
	ELSE
	IF $num = 3
		stmfd sp!, $save1, $save2, $save3
	ELSE
	IF $num = 4
		stmfd sp!, $save1, $save2, $save3, $save4
	ENDIF
	ENDIF
	ENDIF
	ENDIF
	mov a1, $top
	CALL_VM_FUNCTION(CVMJITfixupFrames)
	IF $num = 1
		ldmfd sp!, $save1
	ELSE
	IF $num = 2
		ldmfd sp!, $save1, $save2
	ELSE
	IF $num = 3
		ldmfd sp!, $save1, $save2, $save3
	ELSE
	IF $num = 4
		ldmfd sp!, $save1, $save2, $save3, $save4
	ENDIF
	ENDIF
	ENDIF
	ENDIF
1	
	MEND

#define FIXUP_FRAMES(top, saveset, num)	\
	FIXUP_FRAMES_N top, num, saveset

#define FIXUP_FRAMES_0(top)		\
	FIXUP_FRAMES_N top, 0, saveset

#endif /* __RVCT__ */

#endif /* _INCLUDED_JITASMMACROS_CPU_H */
