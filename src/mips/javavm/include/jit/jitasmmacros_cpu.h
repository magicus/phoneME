/*
 * @(#)jitasmmacros_cpu.h	1.14 06/10/10
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

/* First restore %gp from the CCEE, and then load the specified symbol
   off of %gp. */
#define LA(r,sym)                                               \
    lw	gp, OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_gp(sp);     \
    la	r, sym

/*
 * Shorthand for registers that are alo defined in jitasmconstants_cpu.h
 */
#define JFP CVMMIPS_JFP_REGNAME
#define JSP CVMMIPS_JSP_REGNAME
#define EE  CVMMIPS_EE_REGNAME
#define CP  CVMMIPS_CP_REGNAME
#define CHUNKEND CVMMIPS_CHUNKEND_REGNAME

#define CALL_VM_FUNCTION(VMFUNCTION)	\
	LA(jp, VMFUNCTION);		\
	jalr	jp
#define BRANCH_TO_VM_FUNCTION(CCMFUNCTION)	\
	LA(jp, CCMFUNCTION);			\
	jr	jp

/* 
 * The last word of ccmStorage is reserved on mips for the gp pointer.
 */
#define OFFSET_CVMCCExecEnv_gp OFFSET_CVMCCExecEnv_ccmStorage+56

/*
 * Some macros to assist with fixing up compiled frames. They only bother
 * calling CVMJITfixupFrames if the frames need fixing.
 */
/* NOTE: we use 4 words of ccmStorage[] starting at offset 12. */
#define FIXUP_FRAMES_STORAGE \
	OFFSET_CStack_CCEE+OFFSET_CVMCCExecEnv_ccmStorage+12

#define SAVE0(r1,o)	\
        sw      r1, FIXUP_FRAMES_STORAGE+o(sp)
#define RESTORE0(r1,o)	\
        lw      r1, FIXUP_FRAMES_STORAGE+o(sp)

#define SAVE1(r1)       SAVE0(r1, 0)
#define RESTORE1(r1)    RESTORE0(r1, 0)

#define SAVE2(r1, r2)	\
        SAVE1(r1);	\
        SAVE0(r2, 4)
#define RESTORE2(r1, r2)	\
        RESTORE1(r1);		\
        RESTORE0(r2, 4)

#define SAVE3(r1, r2, r3)       \
        SAVE2(r1, r2);          \
	SAVE0(r3, 8)
#define RESTORE3(r1, r2, r3)    \
        RESTORE2(r1, r2);       \
	RESTORE0(r3, 8)

#define SAVE4(r1, r2, r3, r4)	\
        SAVE3(r1, r2, r3);	\
	SAVE0(r4, 12)
#define RESTORE4(r1, r2, r3, r4)	\
        RESTORE3(r1, r2, r3);		\
	RESTORE0(r4, 12)

#define FIXUP_FRAMES_N(top, tmp, save, restore)		\
	lw	tmp, OFFSET_CVMFrame_prevX(top);	\
	andi	tmp, tmp, CONSTANT_CVM_FRAME_MASK_ALL;	\
	bne	tmp, zero, 9f;				\
	save;						\
	move a0, top;					\
	CALL_VM_FUNCTION(CVMJITfixupFrames);		\
	restore;					\
9:

#define SAVENONE
#define RESTORENONE

#define FIXUP_FRAMES_0(top, scratch) \
	FIXUP_FRAMES_N(top, scratch, SAVENONE, RESTORENONE)
#define FIXUP_FRAMES_1(jfp, scratch, r1)        \
        FIXUP_FRAMES_N(jfp, scratch, SAVE1(r1), RESTORE1(r1))
#define FIXUP_FRAMES_2(jfp, scratch, r1, r2)    \
        FIXUP_FRAMES_N(jfp, scratch, SAVE2(r1, r2), RESTORE2(r1, r2))
#define FIXUP_FRAMES_3(jfp, scratch, r1, r2, r3)        \
        FIXUP_FRAMES_N(jfp, scratch, SAVE3(r1, r2, r3), RESTORE3(r1, r2, r3))
#define FIXUP_FRAMES_4(jfp, scratch, r1, r2, r3, r4)	\
        FIXUP_FRAMES_N(jfp, scratch, SAVE4(r1, r2, r3, r4), \
				     RESTORE4(r1, r2, r3, r4))

#define FIXUP_FRAMES_ra(jfp, scratch)		\
        FIXUP_FRAMES_1(jfp, scratch, ra)
#define FIXUP_FRAMES_a0(jfp, scratch)		\
        FIXUP_FRAMES_1(jfp, scratch, a0)
#define FIXUP_FRAMES_a1(jfp, scratch)		\
        FIXUP_FRAMES_1(jfp, scratch, a1)
#define FIXUP_FRAMES_a0a1(jfp, scratch)		\
        FIXUP_FRAMES_2(jfp, scratch, a0, a1)
#define FIXUP_FRAMES_a1a2(jfp, scratch)		\
        FIXUP_FRAMES_2(jfp, scratch, a1, a2)
#define FIXUP_FRAMES_a1ra(jfp, scratch)		\
        FIXUP_FRAMES_2(jfp, scratch, a1, ra)
#define FIXUP_FRAMES_a2ra(jfp, scratch)		\
        FIXUP_FRAMES_2(jfp, scratch, a2, ra)
#define FIXUP_FRAMES_a0a1ra(jfp, scratch)	\
	FIXUP_FRAMES_3(jfp, scratch, a0, a1, ra)
#define FIXUP_FRAMES_a2a3ra(jfp, scratch)	\
	FIXUP_FRAMES_a2ra(jfp, scratch)
#define FIXUP_FRAMES_a1a2ra(jfp, scratch)	\
        FIXUP_FRAMES_3(jfp, scratch, a1, a2, ra)
#define FIXUP_FRAMES_a1a2a3ra(jfp, scratch)	\
	FIXUP_FRAMES_a1a2ra(jfp, scratch)
#define FIXUP_FRAMES_a0a1a2ra(jfp, scratch)	\
        FIXUP_FRAMES_4(jfp, scratch, a0, a1, a2, ra)

#endif /* _INCLUDED_JITASMMACROS_CPU_H */
