/*
 * @(#)memory_arch.h	1.8 06/10/10
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

/*
 * CPU-specific memory definitions.
 */

#ifndef _LINUX_POWERPC_MEMORY_ARCH_H
#define _LINUX_POWERPC_MEMORY_ARCH_H

/* 
 * Assembler memmove functions.
 *
 * These don't seem to help, so we leave them disabled.
 */
#if 0

extern void
CVMPPCmemmove8Bit(void *s1, const void *s2, size_t n);

extern void
CVMPPCmemmove16Bit(void *s1, const void *s2, size_t n);

extern void
CVMPPCmemmove32Bit(void *s1, const void *s2, size_t n);

#define CVMmemmoveByte		CVMPPCmemmove8Bit
#define CVMmemmoveBoolean	CVMPPCmemmove8Bit
#define CVMmemmoveShort		CVMPPCmemmove16Bit
#define CVMmemmoveChar		CVMPPCmemmove16Bit
#define CVMmemmoveInt		CVMPPCmemmove32Bit
#define CVMmemmoveFloat		CVMPPCmemmove32Bit
#define CVMmemmoveRef		CVMPPCmemmove32Bit
#define CVMmemmoveLong		CVMPPCmemmove32Bit
#define CVMmemmoveDouble	CVMPPCmemmove32Bit

#endif

/*
 * The definition of memalign is here on Linux/PPC
 */
#include <malloc.h>

#endif /* _LINUX_POWERPC_MEMORY_ARCH_H */
