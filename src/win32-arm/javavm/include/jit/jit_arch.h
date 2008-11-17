/*
 * @(#)jit_arch.h	1.12 06/10/10
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

#ifndef _WIN32_JIT_sarm_H
#define _WIN32_JIT_sarm_H

/*
 * TODO: Until we implement CVMCPUemitAtomicCompareAndSwap using 
 * InterlockedTestExchange, we need to make sure jit_risc.h does
 * not #define CVMJIT_SIMPLE_SYNC_METHODS.
 */
#define CVMCPU_NO_SIMPLE_SYNC_METHODS

#include "javavm/include/jit/jit_cpu.h"
#include "javavm/include/jit/jitasmconstants_cpu.h"
#include "javavm/include/jit/ccm_cpu.h"
#include "portlibs/jit/risc/include/export/jit_risc.h"
#include "javavm/include/flushcache_cpu.h"

/* TODO: add iai_opt_config.h like we do for linux-arm. */

/*
 * Trap-based null checking is on by default for Win32/SARM
 */
#ifndef WINCE_DISABLE_STATIC_CODECACHE
#define CVMJIT_TRAP_BASED_NULL_CHECKS
#else
#undef CVMJIT_TRAP_BASED_NULL_CHECKS
#endif

/*
 * Until we figure out how to make exceptions work outside of a
 * compile-time function...
 *
 * We must use a static code cache, otherwise WinCE exceptions
 * don't work, which would mean that we can't use trap-based
 * checks.
 */

#ifndef WINCE_DISABLE_STATIC_CODECACHE
#define CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
#define CVMJIT_HAVE_STATIC_CODECACHE
#else
#undef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
#undef CVMJIT_HAVE_STATIC_CODECACHE
#endif

#define CVMJITflushCache CVMflushCache

#endif /* _WIN32_JIT_sarm_H */
