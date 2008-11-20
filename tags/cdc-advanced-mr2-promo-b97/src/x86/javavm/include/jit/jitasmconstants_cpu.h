/*
 * @(#)jitasmconstants_cpu.h	1.5 06/10/24
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

#ifndef _INCLUDED_JITASMCONSTANTS_CPU_H
#define _INCLUDED_JITASMCONSTANTS_CPU_H

/* jit.h is needed for the CVMCPU_HAS_CP_REG flag */
#include "javavm/include/porting/jit/jit.h"

/*
 * Special registers we use in assembler code that also need to be
 * exported to jitrisc_cpu.h and other C code. See the CVMCPU_MAP_REGNAME
 * macro to see how to map these register names to register numbers.
 */
#ifdef CVMCPU_HAS_CP_REG
#define CVMSPARC_CP_REGNAME  i2
#endif
#define CVMSPARC_CHUNKEND_REGNAME l7

#define CVMX86_JFP_REGNAME ebp
#define CVMX86_JSP_REGNAME edi

#define CVMX86_ARG1_REGNAME eax
#define CVMX86_ARG2_REGNAME edx
#define CVMX86_ARG3_REGNAME ebx
#define CVMX86_ARG4_REGNAME ecx

#define CVMX86_PHI1_REGNAME esi

/* only used in static assembler glue */
#define CVMX86_ARG1w_REGNAME ea
#define CVMX86_ARG2w_REGNAME ed
#define CVMX86_ARG3w_REGNAME eb
#define CVMX86_ARG4w_REGNAME ec
#define CVMX86_ARG1b_REGNAME al
#define CVMX86_ARG2b_REGNAME dl
#define CVMX86_ARG3b_REGNAME bl
#define CVMX86_ARG4b_REGNAME cl

/* The registers that need to be returned by prologue code */
#define CVMX86_PREVFRAME_REGNAME   %ebx  /* Used for non-sync invocations */
#define CVMX86_NEWJFP_REGNAME      %ebp  /* Used for sync invocations     */

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#ifdef CVMCPU_HAS_VOLATILE_GC_REG
#define CVMX86_GC_REGNAME       %esi
#else
#error "Can't dedicate a register for GC!"
#endif
#endif

#endif /* _INCLUDED_JITASMCONSTANTS_CPU_H */
