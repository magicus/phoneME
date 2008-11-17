/*
 * @(#)jit_arch.h	1.8 06/10/10
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

#ifndef _VXWORKS_SPARC_JIT_ARCH_H
#define _VXWORKS_SPARC_JIT_ARCH_H

#include "javavm/include/jit/jit_cpu.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"
#include "javavm/include/jit/ccm_cpu.h"
#include "portlibs/jit/risc/include/export/jit_risc.h"

/*
 * The following are all needed by various sparc C and assembler files.
 *
 *   STACK_ALIGN:   stack alignment size (usually 8)
 *   WINDOWSIZE:    stack window size (usually 16*4)
 *   MINFRAME:      minimum frame size (usually 16*4 + 6*4 +4)
 *   FLUSH_WINDOWS: do the ST_FLUSH_WINDOWS trap
 */
#define STACK_ALIGN      8
#define REGWIN_SZ        0x40  /* from <asm-sparc/ptrace.h> */
#define WINDOWSIZE       REGWIN_SZ
#define STACKFRAME_SZ	 0x60  /* from <asm-sparc/ptrace.h> */
#define MINFRAME         STACKFRAME_SZ
#define FLUSH_WINDOWS			  \
	call	SYM_NAME(windowFlush)	; \
	nop				;

/*
 * Trap-based null checking is on for Solaris
 */
/* TODO: enable this when the segv handler is added to jit_arch.c */
#undef CVMJIT_TRAP_BASED_NULL_CHECKS

#ifndef _ASM
extern CVMBool
vxworksJITSyncInitArch();
#endif

#endif /* _VXWORKS_SPARC_JIT_ARCH_H */
