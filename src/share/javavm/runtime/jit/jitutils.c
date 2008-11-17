/*
 * @(#)jitutils.c	1.69 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/clib.h"

#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitmemory.h"

/*************************************
 * CVMJITStack implementation
 *************************************/

void
CVMJITstackInit(CVMJITCompilationContext* con, CVMJITStack* stack,
		CVMUint32 numSlots)
{
    stack->todoIdx = 0;
    stack->todo = (void **)
	CVMJITmemNew(con, JIT_ALLOC_COLLECTIONS, numSlots * sizeof(void *));
    stack->todoIdxMax = numSlots;
}

/*************************************
 * CVMJITGrowableArray implementation
 *************************************/

void
CVMJITgarrInit(CVMJITCompilationContext* con, CVMJITGrowableArray* gi,
	       CVMUint32 itemSize)
{
    gi->itemSize = itemSize;
    gi->capacity = 0;
    gi->numItems = 0;
    gi->items    = NULL;
}
    
/*
 * Expand capacity for new element
 */
void
CVMJITgarrExpandPrivate(CVMJITCompilationContext* con,
			CVMJITGrowableArray* garr)
{
    CVMUint32   newCapacity;
    void*       newItems;
    
    /* Need to expand */
    newCapacity = 2 * garr->capacity + 2;
    newItems    = CVMJITmemNew(con, JIT_ALLOC_COLLECTIONS,
			       newCapacity * garr->itemSize);
    
    if (garr->items != NULL) {
	memmove(newItems, garr->items, garr->numItems * garr->itemSize);
    }
    CVMtraceJITIROPT(("Expanded item capacity from %d to %d\n",
		      garr->capacity, newCapacity));
    garr->items    = newItems;
    garr->capacity = newCapacity;
}

/*************************************
 * JIT tracing utilities
 *************************************/

#ifdef CVM_TRACE_JIT

CVMInt32
CVMcheckDebugJITFlags(CVMInt32 flags)
{
    return CVMglobals.debugJITFlags & flags;
}

CVMInt32
CVMsetDebugJITFlags(CVMInt32 flags)
{
    CVMInt32 result = CVMcheckDebugJITFlags(flags);
    CVMInt32 new = CVMglobals.debugJITFlags | flags;
    CVMglobals.debugJITFlags = new;
    return result;
}

CVMInt32
CVMclearDebugJITFlags(CVMInt32 flags)
{
    CVMInt32 result = CVMcheckDebugJITFlags(flags);
    CVMglobals.debugJITFlags &= ~flags;
    return result;
}

CVMInt32
CVMrestoreDebugJITFlags(CVMInt32 flags, CVMInt32 oldvalue)
{
    CVMInt32 result = CVMcheckDebugJITFlags(flags);
    CVMInt32 new = (CVMglobals.debugJITFlags & ~flags) | oldvalue;
    CVMglobals.debugJITFlags = new;
    return result;
}

#endif /* CVM_TRACE_JIT */
