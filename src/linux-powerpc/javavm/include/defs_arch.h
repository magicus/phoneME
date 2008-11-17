/*
 * @(#)defs_arch.h	1.11 06/10/10
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

#ifndef _LINUX_POWERPC_DEFS_ARCH_H
#define _LINUX_POWERPC_DEFS_ARCH_H

/*
 * IAI_CACHEDCONSTANT: Move the guessCB for checkcast and instanceof into
 *     the runtime constant pool.
 * IAI_CACHEDCONSTANT_INLINING: Inline fast path part of
 *     CVMCCMruntimeCheckCastGlue and CVMCCMruntimeInstanceOfGlue, thus
 *     avoiding call to glue with guessCB is correct. 
 * NOTE: currently IAI_CACHEDCONSTANT_INLINING requires IAI_CACHEDCONSTANT.
 */
#if 0  /* NOTE: currently disabled by default */
#define IAI_CACHEDCONSTANT
#define IAI_CACHEDCONSTANT_INLINING
#endif
#if 0  /* NOTE: currently disabled by default */
#define IAI_ARRAY_INIT_BOUNDS_CHECK_ELIMINATION
#endif

/*
 * CVMatomicCompareAndSwap() and CVMatomicSwap() are supported.
 */
#define CVM_ADV_ATOMIC_CMPANDSWAP
#define CVM_ADV_ATOMIC_SWAP

/* We define CVM_ADV_SPINLOCK to indicate that we'll be providing
   platform specific implementations of:
       void CVMspinlockYield();
       void CVMvolatileStore(CVMAddr new_value, volatile CVMAddr *addr);
*/
#define CVM_ADV_SPINLOCK

/* CVMdynlinkSym() does not need to prepend an underscore. */
#undef CVM_DYNLINKSYM_PREPEND_UNDERSCORE

#endif /* _LINUX_POWERPC_DEFS_ARCH_H */
