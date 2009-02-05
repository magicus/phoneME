/*
 * @(#)memory_md.h	1.7 06/10/10
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

#ifndef _SOLARIS_MEMORY_MD_H
#define _SOLARIS_MEMORY_MD_H

#include "javavm/include/memory_arch.h"

#ifdef CVM_USE_CVM_MEMALIGN
#include "javavm/include/memory_aligned.h"
#define CVMmemalignAlloc  CVMmemAllocateAligned
#define CVMmemalignFree   CVMmemFreeAligned
#else
#define CVMmemalignAlloc  memalign
#define CVMmemalignFree   free
#endif

/* Note: We only define CVM_USE_MMAP_APIS if the arch specific file did not
   define it first.  For generic solaris, we define it to 1 by default to
   enable the use of MMAP APIs. */
#ifndef CVM_USE_MMAP_APIS
#define CVM_USE_MMAP_APIS  1 /* 1 to enable, 0 to disable */
#endif

#endif /* SOLARIS_MEMORY_MD_H */

