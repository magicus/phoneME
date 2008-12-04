/*
 * @(#)defs_arch.h	1.7 06/10/10
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

#ifndef _VXWORKS_PENTIUM_DEFS_ARCH_H
#define _VXWORKS_PENTIUM_DEFS_ARCH_H

#if defined(i486) || defined(__i486__) || defined(__i486) || \
    defined(i686) || defined(__i686__) || defined(__i686) || \
    defined(pentiumpro) || defined(__pentiumpro__) || defined(__pentiumpro)
#define CVM_ADV_ATOMIC_CMPANDSWAP
#define CVM_ADV_ATOMIC_SWAP
#endif

/* CVMdynlinkSym() needs to prepend an underscore. */
#define CVM_DYNLINKSYM_PREPEND_UNDERSCORE

#endif /* _VXWORKS_PENTIUM_DEFS_ARCH_H */
