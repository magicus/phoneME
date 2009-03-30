/*
 * @(#)cstates.c	1.37 06/10/10
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

#include "javavm/include/cstates.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/utils.h"
#include "javavm/include/porting/doubleword.h"
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif
#ifdef CVM_HW
#include "include/hw.h"
#endif

CVMBool
CVMcsInit(CVMCState *cs, const char *nm, CVMUint8 rank)
{
    cs->request = CVM_FALSE;
    cs->count = 0;
    cs->reached = CVM_FALSE;
    cs->requester = NULL;
    cs->name = nm;
    if (CVMsysMutexInit(&cs->mutex, nm, rank)) {
	if (CVMcondvarInit(&cs->consistentCV, &cs->mutex.rmutex.mutex)) {
	    if (!CVMcondvarInit(&cs->resumeCV, &cs->mutex.rmutex.mutex)) {
		goto bad0;
	    }
	} else {
	    goto bad1;
	}
    }
    return CVM_TRUE;

bad0:
    CVMcondvarDestroy(&cs->consistentCV);
bad1:
    CVMsysMutexDestroy(&cs->mutex);

    return CVM_FALSE;
}

void
CVMcsDestroy(CVMCState *cs)
{
    CVMcondvarDestroy(&cs->consistentCV);
    CVMcondvarDestroy(&cs->resumeCV);
    CVMsysMutexDestroy(&cs->mutex);
}

/*
 * Bring all other threads to a consistent state.
 */
void
CVMcsReachConsistentState(CVMExecEnv *ee, CVMCStateID csID)
{
    CVMCState *cs = CVM_CSTATE(csID);

    /* Check that current thread is consistent */
    CVMassert(CVMcsIsConsistent(CVM_TCSTATE(ee, csID)));
    /* and that a request hasn't already been made */
    CVMassert(!CVMcsCheckRequest(cs));
    /* and that the threadLock has been acquired */
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.threadLock));

    CVMsysMutexLock(ee, &cs->mutex);

    CVMassert(cs->requester == NULL);
    cs->requester = ee;
    ++cs->count;
    CVMassert(!cs->reached);
    CVMtraceCS(("<%d> CS<%d>: ee=0x%x, reaching consistent state %s\n",
	ee->threadID, cs->count, ee, cs->name));

    cs->inconsistentThreadCount = 0;
    cs->request = CVM_TRUE;

#ifdef CVM_MP_SAFE
    /*
     * On a multi-processor system, we need to use a
     * memory barrier to make sure changes are visible to all threads.
     */
    CVMmemoryBarrier();
#endif

    /*
     * After the request is set, threads will only transition from
     * unsafe to safe, so it is OK to check the per-thread state
     * without a lock and before we suspend it.
     */

    /* Roll other threads forward if necessary */
 
    {
        CVM_WALK_ALL_THREADS(ee, targetEE, {
	    if (targetEE != ee) {
		CVMTCState *tcs = CVM_TCSTATE(targetEE, csID);
		CVMUint32 suspended =
		    targetEE->threadState & CVM_THREAD_SUSPENDED;

		/*
		 * Check the state while the thread is running.
		 * State variable needs to be volatile and consistent
		 * with regard to cs->request (MP systems).
		 *
		 * A suspended thread is consistent, even though
		 * its state may be in flux if it was at a
		 * transition.
		 */
		tcs->wasConsistent = CVMcsIsConsistent(tcs);
		if (suspended || tcs->wasConsistent) {
		    /* thread is OK */
		    CVMtraceCS(("<%d> CS<%d>: %s thread <%d>\n",
			ee->threadID, cs->count,
			suspended ? "suspended" : "consistent",
			targetEE->threadID));
		} else {
		    ++cs->inconsistentThreadCount;
		    CVMtraceCS(("<%d> CS<%d>: inconsistent thread <%d>\n",
			ee->threadID, cs->count, targetEE->threadID));
		}
	    }
        });
    }

#ifdef CVM_JIT
#if defined(CVMJIT_PATCH_BASED_GC_CHECKS) || defined(CVMJIT_TRAP_BASED_GC_CHECKS)
    if (cs->inconsistentThreadCount > 0) {
	/* enable rendezvous calls in compiled methods */
	CVMJITenableRendezvousCalls(ee);
	CVMglobals.jit.csNeedDisable = CVM_TRUE;
    } else {
	CVMglobals.jit.csNeedDisable = CVM_FALSE;
    }
#endif
#endif

#ifdef CVM_HW
    CVMhwRequestConsistentState();
#endif

    {
	CVMBool interrupted = CVM_FALSE;
	while (cs->inconsistentThreadCount > 0) {
	    if (!CVMsysMutexWait(ee, &cs->mutex, &cs->consistentCV,
		    CVMlongConstZero()))
	    {
		interrupted = CVM_TRUE;
	    }
	}
	if (interrupted) {
	    CVMtraceCS(("<%d> CS<%d>: interrupted!\n",
		ee->threadID, cs->count));
	    CVMthreadInterruptWait(CVMexecEnv2threadID(ee));
	}
    }

    CVMtraceCS(("<%d> CS<%d>: done waiting for count to reach 0\n",
	ee->threadID, cs->count));

    cs->reached = CVM_TRUE;
    CVMsysMutexUnlock(ee, &cs->mutex);

    CVMtraceCS(("<%d> CS<%d>: mutex unlocked\n", ee->threadID, cs->count));
}

/*
 * Allow other threads to become inconsistent again.
 */
void
CVMcsResumeConsistentState(CVMExecEnv *ee, CVMCStateID csID)
{
    CVMCState *cs = CVM_CSTATE(csID);
    CVMtraceCS(("<%d> CS<%d>: ee=0x%x, resuming consistent state %s\n",
	ee->threadID, cs->count, ee, cs->name));

    cs->reached = CVM_FALSE;
    CVMassert(cs->requester == ee);
    cs->requester = NULL;
    CVMsysMutexLock(ee, &cs->mutex);
    cs->request = CVM_FALSE;
#ifdef CVM_JIT
#if defined(CVMJIT_PATCH_BASED_GC_CHECKS) || defined(CVMJIT_TRAP_BASED_GC_CHECKS)
    /* disable gc rendezvous checkpoints for compiled methods */
    if (CVMglobals.jit.csNeedDisable) {
	CVMJITdisableRendezvousCalls(ee);
    }
#endif
#endif

#ifdef CVM_HW
    CVMhwReleaseConsistentState();
#endif

    CVMcondvarNotifyAll(&cs->resumeCV);
    CVMsysMutexUnlock(ee, &cs->mutex);
}

/*
 * Rendezvous with the requestor thread.  We block only if "block" is
 * set to true.
 */
void
CVMcsRendezvous(CVMExecEnv *ee, CVMCState *cs, CVMTCState *tcs, CVMBool block)
{
    CVMBool isConsistent = tcs->isConsistent;
    CVMUint32 count;
    /* The requester should not be making transitions during the request */
        CVMassert(cs->requester != ee);

    /* must block if inconsistent */
    CVMassert(block != isConsistent);

    /* Force safe-->unsafe transition to appear safe */
    tcs->isConsistent = CVM_TRUE;

#ifdef CVM_MP_SAFE
    /*
     * On a multi-processor system, we need to use a
     * memory barrier to make sure changes are visible to all threads.
     * This is probably overkill here because typically the
     * mutex lock below will guarantee write consistency.
     */
    CVMmemoryBarrier();
#endif

    CVMsysMutexLock(ee, &cs->mutex);

    count = cs->count;
    CVMtraceCS(("<%d> CS<%d>: ee=0x%x, performing rendezvous for "
	"consistent state %s\n",
	ee->threadID, count, ee, cs->name));

    CVMtraceCS(("<%d> CS<%d>: ee=0x%x, isConsistent %d (%d)"
	" block? %d request? %d\n",
	ee->threadID, count, ee, isConsistent, tcs->wasConsistent,
	block, CVMcsCheckRequest(cs)));
    if (!tcs->wasConsistent) {
	CVMassert(CVMcsCheckRequest(cs));
	CVMassert(cs->inconsistentThreadCount > 0);
	tcs->wasConsistent = CVM_TRUE;
	--cs->inconsistentThreadCount;
	CVMtraceCS(("<%d> CS<%d>: decrementing count to %d \n",
	    ee->threadID, count, cs->inconsistentThreadCount));
	if (cs->inconsistentThreadCount == 0) {
	    CVMtraceCS(("<%d> CS<%d>: performing notify\n",
		ee->threadID, count));
	    /* notify condition variable thread */
	    CVMcondvarNotify(&cs->consistentCV);
	}
    }

    /* still on current cycle? */
    CVMassert(count == cs->count);

    if (block) {
	CVMBool interrupted = CVM_FALSE;
	while (CVMcsCheckRequest(cs)) {
	    CVMtraceCS(("<%d> CS<%d>: blocking\n",
		ee->threadID, cs->count));
	    if (!CVMsysMutexWait(ee, &cs->mutex, &cs->resumeCV,
		    CVMlongConstZero()))
	    {
		interrupted = CVM_TRUE;
	    }
	}
	if (interrupted) {
	    CVMtraceCS(("<%d> CS<%d>: interrupted!\n",
		ee->threadID, cs->count));
	    CVMthreadInterruptWait(CVMexecEnv2threadID(ee));
	}
	CVMtraceCS(("<%d> CS<%d>: done blocking\n", ee->threadID, count));
    } else {
	CVMtraceCS(("<%d> CS<%d>: not blocking\n", ee->threadID, count));
    }
    tcs->isConsistent = isConsistent;
    CVMtraceCS(("<%d> CS<%d>: isConsistent reset to %d\n",
	ee->threadID, count, isConsistent));

    /* Shouldn't be inconsistent if consistent state was reached */
    CVMassert(isConsistent || !cs->reached);

    CVMtraceCS(("<%d> CS<%d>: resuming execution with isConsistent %d\n",
	ee->threadID, cs->count, tcs->isConsistent));

    CVMsysMutexUnlock(ee, &cs->mutex);

    /* Shouldn't be inconsistent if consistent state was reached */
    CVMassert(isConsistent || !cs->reached);
}

const char * const CVMcstateNames[CVM_NUM_CONSISTENT_STATES] = {
    "gc-safe"	/* CVM_GC_SAFE */
};
