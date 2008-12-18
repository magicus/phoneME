/*
 * @(#)cstates.h	1.28 06/10/10
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

#ifndef _INCLUDED_CSTATES_H
#define _INCLUDED_CSTATES_H

#include "javavm/include/defs.h"
#include "javavm/include/sync.h"
#include "javavm/include/porting/sync.h"

/*
 * Consistent states.  These are intended to be more general
 * than GC safe points.  The idea is that we can define different
 * kinds of global (VM) and private (thread) consistent states.
 * For example, GC safety may be split into several consistent
 * states:  a per-thread local-root state, a VM global-root state,
 * and a VM object-move state.
 *
 * The synchronization looks a lot like a readers/writer lock.  We
 * can have multiple inconsistent threads (readers), but when a
 * (writer) thread asks for a consistent state, it must wait until
 * all readers are consistent.  The operations are optimized for
 * thread readers.
 *
 * VM ("global") Consistent State
 *
 * Several threads may be inconsistent with respect to a VM consistent
 * state.  It should be relatively inexpensive for an individual thread
 * to make transitions between consistent and inconsistent, but may be
 * quite a bit more expensive to force all threads to become consistent.
 *
 * By associating a consistent state with a state structure, we
 * can enable things like limiting the number of threads that can be
 * inconsistent, which can allow the reach-consistent-state operations
 * to complete in a bounded amount of time.  The state logically
 * contains information about the state of each thread, but for practical
 * reasons the allocation of that per-thread information is the
 * responsibility of each thread.  For each VM consistent state, there
 * should be a global CVMCState declared and a per-thread CVMTCState
 * declared.
 *
 * As an example, global GC safety is modeled by a VM consistent state.
 */

typedef enum {
    CVM_GC_SAFE,		/* gc-safety (see directmem.h) */
    CVM_NUM_CONSISTENT_STATES   /* This enum should always be the last one. */
} CVMCStateID;

#define CVM_NOSUSPEND_SAFE	/* don't suspend safe threads */

/* The shared part of the consistent state */

struct CVMCState {
    volatile CVMBool request;
    CVMUint32 count;
    volatile CVMBool reached;
    CVMExecEnv *requester;

    volatile CVMUint32 inconsistentThreadCount;
    CVMSysMutex mutex;
    CVMCondVar consistentCV;
    CVMCondVar resumeCV;

    const char *name;		/* client managed */
};

/* The per-thread part of the consistent state */

struct CVMTCState {
    volatile CVMBool isConsistent;	/* maintained by thread */
    volatile CVMBool wasConsistent;	/* snapshot set by requestor */
};

/*
 * Initialize global state as unrequested
 */
extern CVMBool CVMcsInit(CVMCState *, 
			 const char *name,	/* client managed */
			 CVMUint8 rank);
extern void CVMcsDestroy(CVMCState *);
/*
 * Initialize thread state as consistent
 */
extern void CVMtcsInit(CVMTCState *);
extern void CVMtcsDestroy(CVMTCState *);

/*
 * Once a thread calls CVMcsReachConsistentState and the request has
 * been satisfied (i.e., all other threads have become consistent), it
 * is illegal for the requesting thread to call CVMcsRendezvous, or to
 * become inconsistent, before calling CVMcsResumeConsistentState. As
 * a practical example, between calls to CVMthreadSuspendGCSafeRequest
 * and CVMthreadSuspendGCSafeRelease, it is illegal for the thread
 * performing the suspension to read fields from Java objects.
 *
 * This restriction could be relaxed if a subset of threads, not
 * including the requestor, were being asked to become consistent.
 * For example, if CVMthreadSuspendGCSafeRequest() only caused
 * the target thread to become consistent.  This is not currently
 * supported.
 */

extern void CVMcsReachConsistentState(CVMExecEnv *, CVMCStateID);
extern void CVMcsResumeConsistentState(CVMExecEnv *, CVMCStateID);

extern CVMBool CVMcsCheckRequest(CVMCState *);
extern void CVMcsBecomeConsistent(CVMCState *, CVMTCState *);

extern void CVMcsBecomeInconsistent(CVMCState *, CVMTCState *);
extern void CVMcsRendezvous(CVMExecEnv *, CVMCState *, CVMTCState *,
	    CVMBool block);


#define CVMcsCheckRequest(cs)				\
	(((cs)->request))

#define CVMcsAllThreadsAreConsistent(cs)                \
        ((cs)->inconsistentThreadCount == 0)

#ifdef CVM_MP_SAFE
    /*
     * On a multi-processor system, we need to use a
     * memory barrier to make sure changes are visible to all threads.
     */
#define CVM_MEM_BARRIER() CVMmemoryBarrier()
#else
#define CVM_MEM_BARRIER()
#endif

#define CVMcsBecomeConsistent(cs, tcs)			\
	{                       			\
            CVMassert(!(tcs)->isConsistent);		\
	    (tcs)->isConsistent = CVM_TRUE;		\
	    CVM_MEM_BARRIER();				\
	}

#define CVMcsBecomeInconsistent(cs, tcs)		\
        {						\
            CVMassert((tcs)->isConsistent);		\
	    (tcs)->isConsistent = CVM_FALSE;		\
	    CVM_MEM_BARRIER();				\
        }

#define CVMcsIsConsistent(tcs)   ((tcs)->isConsistent)
#define CVMcsIsInconsistent(tcs) (!(tcs)->isConsistent)

#define CVMtcsInit(tcs)					\
        {						\
	    (tcs)->isConsistent = CVM_TRUE;		\
	    (tcs)->wasConsistent = CVM_TRUE;		\
        }

#define CVMtcsDestroy(tcs)				\
        {						\
        }

extern const char * const CVMcstateNames[CVM_NUM_CONSISTENT_STATES];

#endif /* _INCLUDED_CSTATES_H */
