/*
 * @(#)memory_aligned.c	1.6 06/10/10
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

/*
 * A shared, simple minded implementation of aligned memory allocation
 * for systems that don't support memalign(), or have buggy implementations
 *
 * For these systems, we are left with no choice but to allocate with
 * some padding so we can align the pointer returned by malloc. The
 * result is wasting "alignment" bytes.
 *
 * NOTE: We could reduce the number of bytes wasted somewhat by trying
 * things like realloc with a smaller size equal to the original
 * malloc size minus the amount we overdid the padding by, and hope that
 * we get the same pointer back, but it's not worth the effort for an
 * unsupported platform.
 */
#include "javavm/include/defs.h"
#include "javavm/include/clib.h"

void*
CVMmemAllocateAligned(CVMUint32 alignment, CVMUint32 size)
{
    void* mallocPtr = malloc(size+alignment);
    void* resultPtr;
    if (mallocPtr == NULL) {
	return NULL;
    }

    /* Round resultPtr up to the next aligned address, even if mallocPtr
     * is already aligned (we need room for the mallocPtr in the 4 bytes
     * before resultPtr).
     */
    resultPtr = (void*)(((int)mallocPtr + alignment) & ~(alignment-1));
    /* save away the mallocPtr */
    *((void**)resultPtr-1) = mallocPtr;
    return resultPtr;
}

void
CVMmemFreeAligned(void* memalignAllocedSpace)
{
    /* The address of the original aligned memory is in the word
       preceding the current allocated one */
    free(*((void**)memalignAllocedSpace-1));
}
