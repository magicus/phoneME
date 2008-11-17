/*
 * @(#)jit_arch.h	1.6 06/10/10
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

#ifndef _LINUX_POWERPC_JIT_ARCH_H
#define _LINUX_POWERPC_JIT_ARCH_H

#include "javavm/include/jit/jit_cpu.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"
#include "javavm/include/jit/ccm_cpu.h"
#include "portlibs/jit/risc/include/export/jit_risc.h"

/*
 * The size in bytes of the region for which we want accurate profiling
 * information. In this case, we want to be accurate to within an instruction.
 */
#define CVMJIT_PROFILE_REGION_SIZE CVMCPU_INSTRUCTION_SIZE
#ifndef _ASM
#include "portlibs/posix/posix_jit_profil.h"
#endif

/*
 * Trap-based null checking is on for PowerPC
 */
#define CVMJIT_TRAP_BASED_NULL_CHECKS

#ifndef _ASM
extern CVMBool
linuxJITSyncInitArch();
#endif

#endif /* _LINUX_POWERPC_JIT_ARCH_H */
