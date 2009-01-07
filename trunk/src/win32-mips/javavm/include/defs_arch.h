/*
 * @(#)defs_arch.h	1.12 06/10/10
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

#ifndef _LINUX_MIPS_DEFS_ARCH_H
#define _LINUX_MIPS_DEFS_ARCH_H

/*
 * CVMatomicCompareAndSwap() is supported.
 * CVMatomicSwap() is supported.
 */
#define CVM_ADV_ATOMIC_CMPANDSWAP
#define CVM_ADV_ATOMIC_SWAP

/* CVMdynlinkSym() does not need to prepend an underscore. */
#undef CVM_DYNLINKSYM_PREPEND_UNDERSCORE

/* ABI definition */

#define CVMMIPS_MAX_ARG_REGS 4

#ifndef _MIPS64
#undef  CVMMIPS_HAS_64BIT_REGISTERS
#undef  CVMMIPS_HAS_64BIT_FP_REGISTERS
#undef  CVMMIPS_FPR_ARG_AFTER_GPR_ARG
#else
#define CVMMIPS_HAS_64BIT_REGISTERS
#define CVMMIPS_HAS_64BIT_FP_REGISTERS
#define CVMMIPS_FPR_ARG_AFTER_GPR_ARG
#endif

#endif /* _LINUX_MIPS_DEFS_ARCH_H */
