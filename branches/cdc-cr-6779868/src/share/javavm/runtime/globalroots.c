/*
 * @(#)globalroots.c	1.27 06/10/10
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
#include "javavm/include/globals.h"
#include "javavm/include/globalroots.h"

/*
 * Allocate  a global root from the specified stack. Returns NULL
 * on failure. No exception ever thrown.
 */
static CVMObjectICell*
CVMIDprivate_getGlobalRootNoLock(CVMExecEnv* ee, CVMStack* grStack)
{
    CVMStackVal32* result = NULL;
    CVMGlobalRootsFrame* singletonFrame =
	(CVMGlobalRootsFrame*)grStack->currentFrame;

    CVMframeAlloc(ee, grStack, singletonFrame, result);
    
    return &result->ref;
}

static CVMObjectICell*
CVMIDprivate_getGlobalRoot(CVMExecEnv* ee, CVMStack* grStack,
			   CVMSysMutex* lock)
{
    CVMObjectICell* result;
    CVMsysMutexLock(ee, lock);
    result = CVMIDprivate_getGlobalRootNoLock(ee, grStack);
    CVMsysMutexUnlock(ee, lock);
    
    return result;
}

CVMObjectICell*
CVMID_getGlobalRoot(CVMExecEnv* ee)
{
    return CVMIDprivate_getGlobalRoot(ee, &CVMglobals.globalRoots,
				      &CVMglobals.globalRootsLock);
}

CVMObjectICell*
CVMID_getClassGlobalRoot(CVMExecEnv* ee)
{
    return CVMIDprivate_getGlobalRoot(ee, &CVMglobals.classGlobalRoots,
				      &CVMglobals.classTableLock);
}

CVMObjectICell*
CVMID_getClassLoaderGlobalRoot(CVMExecEnv* ee)
{
    return CVMIDprivate_getGlobalRoot(ee,
				      &CVMglobals.classLoaderGlobalRoots,
				      &CVMglobals.classTableLock);
}

CVMObjectICell*
CVMID_getProtectionDomainGlobalRoot(CVMExecEnv* ee)
{
    return CVMIDprivate_getGlobalRoot(ee,
				      &CVMglobals.protectionDomainGlobalRoots,
				      &CVMglobals.globalRootsLock);
}

CVMObjectICell*
CVMID_getWeakGlobalRoot(CVMExecEnv* ee)
{
    return CVMIDprivate_getGlobalRoot(ee, &CVMglobals.weakGlobalRoots,
				      &CVMglobals.weakGlobalRootsLock);
}

static void
CVMIDprivate_freeGlobalRootNoLock(CVMExecEnv* ee, CVMObjectICell* glRoot,
				  CVMStack* grStack)
{
    CVMGlobalRootsFrame* singletonFrame =
	(CVMGlobalRootsFrame*)grStack->currentFrame;
    CVMStackVal32* refToFree = (CVMStackVal32*)glRoot;

    CVMframeFree(singletonFrame, refToFree);
}

static void
CVMIDprivate_freeGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot,
			    CVMStack* grStack, CVMSysMutex* lock)
{
    CVMsysMutexLock(ee, lock);
    CVMIDprivate_freeGlobalRootNoLock(ee, glRoot, grStack);
    CVMsysMutexUnlock(ee, lock);
}

void
CVMID_freeClassGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot)
{
    CVMIDprivate_freeGlobalRoot(ee, glRoot,
				&CVMglobals.classGlobalRoots,
				&CVMglobals.classTableLock);
}

void
CVMID_freeClassLoaderGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot)
{
    CVMIDprivate_freeGlobalRoot(ee, glRoot,
				&CVMglobals.classLoaderGlobalRoots,
				&CVMglobals.classTableLock);
}

void
CVMID_freeProtectionDomainGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot)
{
    CVMIDprivate_freeGlobalRoot(ee, glRoot,
				&CVMglobals.protectionDomainGlobalRoots,
				&CVMglobals.globalRootsLock);
}

void
CVMID_freeGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot)
{
    CVMIDprivate_freeGlobalRoot(ee, glRoot, &CVMglobals.globalRoots,
				&CVMglobals.globalRootsLock);
}

void
CVMID_freeWeakGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot)
{
    CVMIDprivate_freeGlobalRoot(ee, glRoot, &CVMglobals.weakGlobalRoots,
				&CVMglobals.weakGlobalRootsLock);
}

void
CVMglobalrootFrameScanner(CVMExecEnv* ee,
			  CVMFrame* grFrame, CVMStackChunk* grChunk,
			  CVMRefCallbackFunc callback, void* data,
			  CVMGCOptions* opts)
{
    CVMwalkRefsInGCRootFrame(grFrame,
			     ((CVMFreelistFrame*)grFrame)->vals,
			     grChunk, callback, data, CVM_TRUE);
}
