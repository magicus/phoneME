/*
 * @(#)jitasmmacros_cpu.h	1.13 06/10/10
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
 * Shorthand for registers that are alo defined in jitasmconstants_cpu.h
 */
#define JFP %CVMSPARC_JFP_REGNAME
#define JSP %CVMSPARC_JSP_REGNAME
#define EE  %CVMSPARC_EE_REGNAME
#define CP  %CVMSPARC_CP_REGNAME
#define CHUNKEND %CVMSPARC_CHUNKEND_REGNAME

#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define CALL_VM_FUNCTION(VMFUNCTION)       \
        call SYM_NAME(VMFUNCTION)
#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION) \
	b SYM_NAME(CCMFUNCTION)
#else
#define CALL_VM_FUNCTION(VMFUNCTION)       \
	call SYM_NAME(VMFUNCTION)
#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION) \
        b SYM_NAME(CCMFUNCTION)
#endif

/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * calling CVMJITfixupFrames if the frames need fixing.
 */
/* NOTE: we use 3 words of ccmStorage[] starting at offset 12. */
#define FIXUP_FRAMES_STORAGE \
	MINFRAME+OFFSET_CVMCCExecEnv_ccmStorage+12
#define SAVE1(r1)       \
        st      r1, [%sp + FIXUP_FRAMES_STORAGE]
#define RESTORE1(r1)    \
        ld      [%sp + FIXUP_FRAMES_STORAGE], r1

#define SAVE2(r1, r2)   \
        SAVE1(r1);      \
        st      r2, [%sp + FIXUP_FRAMES_STORAGE + 4]
#define RESTORE2(r1, r2)        \
        RESTORE1(r1);   \
        ld      [%sp + FIXUP_FRAMES_STORAGE + 4], r2

#define SAVE3(r1, r2, r3)       \
        SAVE2(r1, r2);          \
        st      r3, [%sp + FIXUP_FRAMES_STORAGE + 8]
#define RESTORE3(r1, r2, r3)    \
        RESTORE2(r1, r2);       \
        ld      [%sp + FIXUP_FRAMES_STORAGE + 8], r3

#define FIXUP_FRAMES_N(top, tmp, save, restore)      \
	ld [top + OFFSET_CVMFrame_prevX], tmp;       \
	andcc tmp, CONSTANT_CVM_FRAME_MASK_ALL, %g0; \
	bne 0f;                                      \
	nop;		        		     \
	save;                                        \
	mov top, %o0;                                \
	CALL_VM_FUNCTION(CVMJITfixupFrames);         \
	nop;					     \
	restore;                                     \
0:

#define FIXUP_FRAMES_0(top, scratch) FIXUP_FRAMES_N(top, scratch, /* */, /* */)
#define FIXUP_FRAMES_1(jfp, scratch, r1)        \
        FIXUP_FRAMES_N(jfp, scratch, SAVE1(r1), RESTORE1(r1))
#define FIXUP_FRAMES_2(jfp, scratch, r1, r2)    \
        FIXUP_FRAMES_N(jfp, scratch, SAVE2(r1, r2), RESTORE2(r1, r2))
#define FIXUP_FRAMES_3(jfp, scratch, r1, r2, r3)        \
        FIXUP_FRAMES_N(jfp, scratch, SAVE3(r1, r2, r3), RESTORE3(r1, r2, r3))

#endif /* _INCLUDED_JITASMMACROS_CPU_H */
