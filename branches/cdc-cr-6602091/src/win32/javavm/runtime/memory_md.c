/*
 * @(#)memory_md.c	1.6 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

#include "javavm/include/porting/memory.h"
#include "javavm/include/porting/ansi/assert.h"
#include <windows.h>

#define WIN32_VIRTUAL_ALLOC_ALIGNMENT (64 * 1024)

extern void*
CVMmemalignAlloc(size_t alignment, size_t size)
{
    assert(alignment == WIN32_VIRTUAL_ALLOC_ALIGNMENT);
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

extern void
CVMmemalignFree(void* memalignAllocedSpace)
{
    VirtualFree(memalignAllocedSpace, 0, MEM_RELEASE);
}
