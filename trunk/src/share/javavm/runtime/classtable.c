/*
 * @(#)classtable.c	1.28 06/10/10
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
#include "javavm/include/classes.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"

#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/*
 * Class table API's.
 *
 * NOTE: We make use of an CVMStack containing an CVMFreelistFrame
 * as the class table database. The CVMFreelistFrame was only designed
 * to hold CVMObject*'s, so we end up doing lots of casting to get it
 * to work as a general purpose freelist frame.
 */

/*
 * Allocate a slot in the class table. Returns NULL on failure. No
 * exception is ever thrown.
 */

CVMClassBlock**
CVMclassTableAllocateSlot(CVMExecEnv* ee)
{
    CVMStackVal32* result;
    CVM_CLASSTABLE_LOCK(ee);
    CVMframeAlloc(ee, &CVMglobals.classTable, 
		  (CVMFreelistFrame*)CVMglobals.classTable.currentFrame, 
		  result);
    CVM_CLASSTABLE_UNLOCK(ee);
    return (CVMClassBlock**)result;
}

/*
 * Free a classTable slot.
 */
void
CVMclassTableFreeSlot(CVMExecEnv* ee, CVMClassBlock** cbPtr)
{
    /* The slot should always have been set to null before freeing. */
    CVMassert(*cbPtr == NULL);
    CVM_CLASSTABLE_LOCK(ee);
    CVMframeFree((CVMFreelistFrame*)CVMglobals.classTable.currentFrame, 
	 	 (CVMStackVal32*)cbPtr);
    CVM_CLASSTABLE_UNLOCK(ee);
}

/*
 * Mark all class table slots belonging to classes that weren't referenced
 * in the gc scan. Note that isLive() will return true if the Class instance
 * is not in the current generation being scanned. This way we only free
 * classes that are known to be dead.
 */


typedef struct CVMClassTableMarkCallbackParams {
    CVMRefLivenessQueryFunc isLive;
    void*                   isLiveData;
    CVMExecEnv*             ee;
} CVMClassTableMarkCallbackParams;

static void
CVMclassTableMarkerCallback(CVMObject** cb_p, void* data)
{
    CVMClassTableMarkCallbackParams* params =
	(CVMClassTableMarkCallbackParams*)data;
    CVMClassBlock* cb = *(CVMClassBlock**)cb_p;
    CVMBool isLive =
	params->isLive((CVMObject**)CVMcbJavaInstance(cb), params->isLiveData);
    /*
     * If the class is not live, then mark it for later freeing. We can't
     * actually free the classTable slot or Class global root here because
     * this will introduce a race condition with any other thread that
     * currently holds the classTableLock. We need to wait until the pass #2
     * of class unloading, at which time we can grab the classTableLock.
     */
    if (!isLive) {
	CVMMethodBlock* mb;
	CVMtraceClassLoading(("CL: free class marked %C (cb=0x%x)\n",
			      cb, cb));

#ifdef CVM_JIT
	if (params->ee != NULL) { /* NULL ee means vm shutdown */
	    CVMassert(CVMsysMutexIAmOwner(CVMgetEE(), &CVMglobals.jitLock));
	    /* We need to invalidate the mbs in the hints because they may
	       reference methods that will be unloaded with this class: */
	    CVMjitInvalidateInvokeVirtualHints(cb);
	    CVMjitInvalidateInvokeInterfaceHints(cb);
	}
#endif

#ifdef CVM_JVMPI
        if (CVMjvmpiEventClassUnloadIsEnabled()) {
            CVMjvmpiPostClassUnloadEvent(cb);
        }
#endif /* CVM_JVMPI */

	/* NULL the Class global root so gc won't scan it. */
	CVMID_icellSetNull(CVMcbJavaInstance(cb));
	/* NULL the class table slot so no one thinks this class is loaded */
	*cb_p = NULL;
	/* Store the classTable slot where CVMclassFree() can find it. */
	CVMcbClassTableSlotPtr(cb) = (CVMClassBlock**)cb_p;
	/* Store the miranda method count where CVMclassFree() can find it.
	 * We need to do this now because accessing the last method in the
	 * method table is not allowed during class unloading because it 
	 * may refer to a superclass method that has already been freed.
	 */
	CVMcbMirandaMethodCount(cb) = 0;
	if (!CVMcbIs(cb, INTERFACE) && CVMcbCheckRuntimeFlag(cb, LINKED)
	    && CVMcbMethodTableCount(cb) != 0)
	{
	    mb = CVMcbMethodTableSlot(cb, CVMcbMethodTableCount(cb) - 1);
	    if (mb != NULL && CVMmbIsMiranda(mb) && CVMmbClassBlock(mb) == cb){
		CVMcbMirandaMethodCount(cb) = CVMmbMethodIndex(mb) + 1;
	    }
	}

	/* Add the class to the list of classs to call CVMclassFree() on. */
	CVMcbFreeClassLink(cb) = CVMglobals.freeClassList;
	CVMglobals.freeClassList = cb;
    }
}

extern void
CVMclassTableMarkUnscannedClasses(CVMExecEnv* ee,
				   CVMRefLivenessQueryFunc isLive,
				   void* isLiveData)
{
    CVMClassTableMarkCallbackParams params;
    params.isLive = isLive;
    params.isLiveData = isLiveData;
    params.ee = ee;

    /*
     * Add each unreachable class to CVMglobals.classTable,
     * NOTE: See above comment to understand why we are using
     * CVMstackScanGCRootFrameRoots().
     */
    CVMstackScanGCRootFrameRoots(&CVMglobals.classTable,
				 CVMclassTableMarkerCallback,
				 &params);
}

/*
 * Free all classes in the class table. This is used during vm shutdown.
 */

static CVMBool
CVMclassTableEverythingIsUnscanned(CVMObject** refAddr, void* data)
{
    return CVM_FALSE;
}

extern void
CVMclassTableFreeAllClasses(CVMExecEnv* ee)
{
    CVMsysMutexLock(ee, &CVMglobals.heapLock);
    /*
     * Add each class to CVMglobals.freeClassList. We take advantage of
     * CVMclassTableMarkUnscannedClasses() here. It expects to be passed
     * a gc callback that tells it if an object is live or not. Instead
     * we just give it a callback that always says the class is not live.
     */
    CVMclassTableMarkUnscannedClasses(NULL, /* NULL ee means vm shutdown */
				      CVMclassTableEverythingIsUnscanned,
				      NULL);

    /* Free all the classes added to the freeClassList by the above scan */
    {
	CVMClassBlock* cb = CVMglobals.freeClassList;
	CVMglobals.freeClassList = NULL;
	while (cb != NULL) {
	    CVMClassBlock* currCb = cb;
	    CVMtraceClassUnloading(("CLU: Freeing class %C (cb=0x%x)\n",
				    cb, cb));
	    cb = CVMcbFreeClassLink(cb);
	    CVMclassFree(ee, currCb);
	}
    }
    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
}

/*
 * Iterate over the class table. 
 */

typedef struct CVMScanData {
    CVMExecEnv*          ee;
    CVMClassCallbackFunc callback;
    void*                data;
} CVMScanData;

static void
CVMclassTableIteratorCallback(CVMObject** cb_p, void* data)
{
    CVMScanData* scandata = (CVMScanData*)data;
    CVMClassBlock* cb = *(CVMClassBlock**)cb_p;
    scandata->callback(scandata->ee, cb, scandata->data);
}


extern void
CVMclassTableIterate(CVMExecEnv* ee,
		     CVMClassCallbackFunc callback,
		     void* data)
{
    CVMScanData scandata;
    scandata.ee       = ee;
    scandata.callback = callback;
    scandata.data     = data;
 
   /*
     * NOTE: See above comment to understand why we are using
     * CVMstackScanGCRootFrameRoots().
     */
    CVMstackScanGCRootFrameRoots(&CVMglobals.classTable,
				 CVMclassTableIteratorCallback,
				 &scandata);
}

/*
 * Dump the class table.
 */

#ifdef CVM_DEBUG

static void
CVMclassTableDumperCallback(CVMObject** cb_p, void* data)
{
    CVMClassBlock* cb = *(CVMClassBlock**)cb_p;
    CVMconsolePrintf("CT: 0x%x: %C\n", cb, cb);
}

void
CVMclassTableDump(CVMExecEnv* ee)
{
    /*
     * NOTE ALERT: See above comment to understand why we are using
     * CVMstackScanGCRootFrameRoots().
     */
    CVMstackScanGCRootFrameRoots(&CVMglobals.classTable,
				 CVMclassTableDumperCallback,
				 NULL);
}

#endif /* CVM_DEBUG */
