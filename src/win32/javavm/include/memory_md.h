/*
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

#ifndef _WIN32_MEMORY_MD_H
#define _WIN32_MEMORY_MD_H

/* If an arch specific file is needed in the future, add the #include here:
#include "javavm/include/memory_arch.h"
*/

/* Note: We only define CVM_USE_MMAP_APIS if the arch specific file did not
   define it first.  For generic win32, we define it to 1 by default to
   enable the use of MMAP APIs. */
#ifndef CVM_USE_MMAP_APIS
#define CVM_USE_MMAP_APIS  1 /* 1 to enable, 0 to disable */
#endif

#endif /* WIN32_MEMORY_MD_H */
