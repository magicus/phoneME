/*
 * @(#)jitasmconstants_cpu.h	1.15 06/10/10
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

#ifndef _INCLUDED_JITASMCONSTANTS_CPU_H
#define _INCLUDED_JITASMCONSTANTS_CPU_H

/* jit.h is needed for the CVMCPU_HAS_CP_REG flag */
#include "javavm/include/porting/jit/jit.h"

/*
 * Special registers we use in assembler code that also need to be
 * exported to jitrisc_cpu.h and other C code. See the CVMCPU_MAP_REGNAME
 * macro to see how to map these register names to register numbers.
 */
#define CVMSPARC_EE_REGNAME  		i3
#define CVMSPARC_JFP_REGNAME  		i4
#define CVMSPARC_JSP_REGNAME  		i5
#ifdef CVMCPU_HAS_CP_REG
#define CVMSPARC_CP_REGNAME  		i2
#endif
#define CVMSPARC_CHUNKEND_REGNAME 	l7
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define CVMSPARC_GC_REGNAME		l6
#endif

/* The registers that need to be returned by prologue code */
#define CVMSPARC_PREVFRAME_REGNAME   i1  /* Used for non-sync invocations */
#define CVMSPARC_NEWJFP_REGNAME      i0  /* Used for sync invocations     */

#endif /* _INCLUDED_JITASMCONSTANTS_CPU_H */
