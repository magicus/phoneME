/*
 * @(#)localroots.c	1.18 06/10/10
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
#include "javavm/include/objects.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/stacks.h"
#include "javavm/include/localroots.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/gc_common.h"

/*
 * The following function is private to the local roots implementation.
 * It allocates a new slot from a local roots frame.
 */
CVMObjectICell*
CVMIDprivate_allocateLocalRoot(CVMExecEnv* ee, 
			       CVMStack* lrStack, CVMFrame* lrFrame)
{
    CVMStackVal32* result = 0;
    CVMframeAllocFromTop(ee, lrStack, lrFrame, result);
    CVMassert(result != 0);
    return &result->ref;
}

CVMObjectICell*
CVMIDprivate_allocateLocalRootUnsafe(CVMExecEnv* ee, 
				     CVMStack* lrStack, CVMFrame* lrFrame)
{
    CVMStackVal32* result = 0;
    CVMD_gcSafeExec(ee, {
	CVMframeAllocFromTop(ee, lrStack, lrFrame, result);
    });
    CVMassert(result != 0);
    return &result->ref;
}

void
CVMlocalrootFrameScanner(CVMExecEnv* ee,
			 CVMFrame* lrFrame, CVMStackChunk* lrChunk,
			 CVMRefCallbackFunc callback, void* data,
			 CVMGCOptions* opts)
{
    CVMwalkRefsInGCRootFrame(lrFrame, (CVMStackVal32*)(lrFrame + 1),
			     lrChunk, callback, data, CVM_FALSE);
}    
