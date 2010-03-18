/*
 * @(#)asmmacros_cpu.h	1.6 06/10/10
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

#ifndef _INCLUDED_ASMMACROS_CPU_H
#define _INCLUDED_ASMMACROS_CPU_H

#ifndef _ASM
#define _ASM 
#endif

#include "javavm/include/asmmacros_arch.h"

#ifndef SYM_NAME
#define SYM_NAME(x) x
#endif

#ifndef SYMBOL
#define SYMBOL(x) =x
#endif

#ifndef CONSTANT
#define CONSTANT(x) =x
#endif

/*
 * Note the old-fashioned use of an empty comment
 * to do token pasting. This is necessary because
 * the -traditional flag gets passed to the preprocessor
 * which consequently won't do modern ANSI/ISO pasting.
 */
	
#if defined(__THUMB_INTERWORK__)
#ifdef __RVCT__
/* Branch through register may require changing to Thumb mode */
#define BRCOND_REG(regno,cond)		\
	bx##cond	regno
#else
#define BRCOND_REG(regno,cond)		\
	bx/**/cond	regno
#endif
#define BR_REG(regno)			\
	BRCOND_REG(regno,al)
#else
/* To branch through a register, just copy it into the PC */
#ifdef __RVCT__
#define BRCOND_REG(regno,cond)		\
	mov##cond	pc,regno
#else
#define BRCOND_REG(regno,cond)		\
	mov/**/cond	pc,regno
#endif
#define BR_REG(regno)			\
	BRCOND_REG(regno,al)
#endif

#ifdef CVM_MP_SAFE
#define swp "Can't use swp when CVM_MP_SAFE=true"
#endif

#endif /* _INCLUDED_ASMMACROS_CPU_H */
