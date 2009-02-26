/*
 * @(#)jitasmconstants_cpu.h	1.20 06/10/10
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights  
 * Reserved.  Use is subject to license terms.  
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
 */

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#ifndef _INCLUDED_JITASMCONSTANTS_CPU_H
#define _INCLUDED_JITASMCONSTANTS_CPU_H

/* jit.h is needed for the CVMCPU_HAS_CP_REG and CVMJIT_TRAP_BASED_GC_CHECKS */
#include "javavm/include/porting/jit/jit.h"

/* Make sure both CVMCPU_HAS_CP_REG and CVMJIT_TRAP_BASED_GC_CHECKS
 * are not #defined, because we don't have the available registers
 * to support both at the same time.
 */
#if defined(CVMCPU_HAS_CP_REG) && \
    defined(CVMJIT_TRAP_BASED_GC_CHECKS) && \
    !defined(CVMCPU_HAS_VOLATILE_GC_REG)
#error "ARM port does not support both CVMCPU_HAS_CP_REG and CVMJIT_TRAP_BASED_GC_CHECKS"
#endif

/*
 * Special registers we use in assembler code that also need to be
 * exported to jitrisc_cpu.h and other C code. See the CVMCPU_MAP_REGNAME
 * macro to see how to map these register names to register numbers.
 */

#define CVMARM_JFP_REGNAME v1
#define CVMARM_JSP_REGNAME v2
#ifdef CVMCPU_HAS_CP_REG
#define CVMARM_CP_REGNAME  v8
#endif

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#ifdef CVMCPU_HAS_VOLATILE_GC_REG
/* don't use a1, a3, or a4 without reworking CVMJITgoNative. */
#define CVMARM_GC_REGNAME  a2
#else
#define CVMARM_GC_REGNAME  v8
#endif
#endif

/* The registers that need to be returned by prologue code */
#define CVMARM_PREVFRAME_REGNAME   v3  /* Used for non-sync invocations */
#define CVMARM_NEWJFP_REGNAME      v4  /* Used for sync invocations     */

/* IAI-04 */
/* WMMX regsisters for &CVMglobals and &CVMglobals.objGlobalMicroLock */
#ifdef  IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
#define W_CVMGLOBALS wr10
#define W_MICROLOCK wr11
#endif

#endif /* _INCLUDED_JITASMCONSTANTS_CPU_H */
