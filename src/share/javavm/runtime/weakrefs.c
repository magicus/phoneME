/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/utils.h"
#include "javavm/include/string.h"

#include "javavm/include/preloader_impl.h"

/*
 * Four queues for the four strengths of weak references
 */
/*
static CVMObject* discoveredSoftRefs;
static CVMObject* discoveredWeakRefs;
static CVMObject* discoveredFinalRefs;
static CVMObject* discoveredPhantomRefs;
*/

/*
 * Clear all the discovered weak reference queues.
 */
static void
CVMweakrefClearDiscoveredQueues()
{
    CVMglobals.discoveredSoftRefs = NULL;
    CVMglobals.discoveredWeakRefs = NULL;
    CVMglobals.discoveredFinalRefs = NULL;
    CVMglobals.discoveredPhantomRefs = NULL;
}

/*
 * Get a pointer to the Reference.pending static variable.
 */
static CVMObject** 
CVMweakrefGetPendingQueue()
{
    return (CVMObject**)(&CVMglobals.java_lang_ref_Reference_pending->r);
}

#ifdef CVM_TRACE
static char*
CVMweakrefQueueName(CVMObject** queue)
{
    if (queue == &CVMglobals.discoveredSoftRefs) {
	return "discovered SoftReference";
    } else if (queue == &CVMglobals.discoveredWeakRefs) {
	return "discovered WeakReference";
    } else if (queue == &CVMglobals.discoveredFinalRefs) {
	return "discovered FinalReference";
    } else if (queue == &CVMglobals.discoveredPhantomRefs) {
	return "discovered PhantomReference";
    } else if (queue == CVMweakrefGetPendingQueue()) {
	return "Reference.pending";
    } else {
	return "<UNKNOWN QUEUE>";
    }
}
#endif

/*
 * Enqueue an item in a queue (discovered or pending). These queues are
 * to preserve the invariant that a weak reference object is active
 * iff its next field is NULL. So they make sure the list is terminated
 * with a self-referring weakref object.
 */
static void
CVMweakrefEnqueue(CVMObject** queue, CVMObject* weakRef)
{
    CVMObject* nextItem;
    if (*queue == NULL) {
	/*
	 * Don't allow for NULL next fields. Otherwise the weak
	 * reference may be discovered multiple times by
	 * CVMobjectWalkRefsWithSpecialHandling().
	 */
	nextItem = weakRef;
    } else {
	nextItem = *queue;
    }
    *queue = weakRef;
    CVMweakrefFieldSet(weakRef, next, nextItem);
    CVMtraceWeakrefs(("WR: Enqueued weak object 0x%x in the %s queue\n",
		      weakRef, CVMweakrefQueueName(queue)));
}

/*
 * Discover and enqueue new weak reference object.
 */
void
CVMweakrefDiscover(CVMExecEnv* ee, CVMObject* weakRef)
{
    CVMObject* referent = CVMweakrefField(weakRef, referent);
    CVMClassBlock* weakCb;
    /*
     * Nothing to do. We must have encountered this weak reference object
     * before its constructor has had a chance to run.
     */
    if (referent == NULL) {
	return;
    }

    CVMtraceWeakrefs(("WR: Discovered new weakref object 0x%x "
		      "of class %C, "
		      "referent 0x%x\n",
		      weakRef,
		      CVMobjectGetClass(weakRef),
		      referent));
    weakCb = CVMobjectGetClass(weakRef);
    /*
     * Enqueue the discovered ref in the right queue for its strength.
     *
     * Time-space tradeoff: If we had a 'reference strength'
     * field in the class block, we can make the following subclass
     * checks unnecessary. 
     */
    if (CVMisSubclassOf(ee, weakCb,
			CVMsystemClass(java_lang_ref_SoftReference))) {
	CVMweakrefEnqueue(&CVMglobals.discoveredSoftRefs, weakRef);
    } else if (!CVMlocalExceptionOccurred(ee) && 
               CVMisSubclassOf(ee, weakCb,
			       CVMsystemClass(java_lang_ref_WeakReference))) {
	CVMweakrefEnqueue(&CVMglobals.discoveredWeakRefs, weakRef);
    } else if (!CVMlocalExceptionOccurred(ee) && 
               CVMisSubclassOf(ee, weakCb,
			       CVMsystemClass(java_lang_ref_FinalReference))) {
	CVMweakrefEnqueue(&CVMglobals.discoveredFinalRefs, weakRef);
    } else if (!CVMlocalExceptionOccurred(ee) && 
               CVMisSubclassOf(ee, weakCb,
			       CVMsystemClass(java_lang_ref_PhantomReference))){
	CVMweakrefEnqueue(&CVMglobals.discoveredPhantomRefs, weakRef);
    }
}

/*
 * Enqueue a weak reference in the Reference.pending queue.
 */
static void
CVMweakrefEnqueuePending(CVMObject* thisRef)
{
    /*
     * One optimization here would be to check whether
     * the queue element of this guy is ReferenceQueue.NULL. If it is
     * the ReferenceHandler thread will do nothing with this reference
     * anyway, so we might as well discard it instead of enqueueing it.
     */
    CVMweakrefEnqueue(CVMweakrefGetPendingQueue(), thisRef);
    /* So that CVMgcStopTheWorldAndGCSafe() does notify the ReferenceHandler
     * thread */
    CVMglobals.referenceWorkTODO = CVM_TRUE;
}

/*
 * A function to handle a weak reference instance with a dying referent.
 *
 * An instance of this decides whether to clear the referent or not,
 * and whether to enqueue the weak reference in the pending queue or not.
 *
 * Return values:
 *
 *     CVM_TRUE:  The referent has to be marked.
 *         This happens with dying Finalizer and PhantomReference instances,
 *         as well as SoftReference instances we elect not to clear.
 *
 *     CVM_FALSE: The referent is cleared.
 *         This happens with dying WeakReference instances,
 *         as well as SoftReference instances we elect to clear.
 *
 */
typedef CVMBool (*CVMWeakrefHandleDying)(CVMObject* refAddr);

static CVMBool
CVMweakrefClearUnconditional(CVMObject* thisRef)
{
    /* The weakref is dead, and we want to clear the referent
     * unconditionally.
     */
    CVMtraceWeakrefs(("WR: CVMweakrefClearUnonditional: "
		      "Clearing ref=%x (class %C)\n",
		      thisRef, CVMobjectGetClass(thisRef)));
    /* And these go into yet another deferred queue */
    /* Out of the queue, into the active state again */
    /* In case we ever have to undo this stage, defer the referent
       clean up and enqueueing in the pending queue */
    CVMweakrefEnqueue(&CVMglobals.deferredWeakrefs2, thisRef);
    /*
    CVMweakrefField(thisRef, referent) = NULL;
    CVMweakrefEnqueuePending(thisRef);
    */
    return CVM_FALSE; /* No referent to scan */
}

static CVMBool
CVMweakrefClearConditional(CVMObject* thisRef)
{
    return CVMweakrefClearUnconditional(thisRef);
}

static CVMBool
CVMweakrefReferentKeep(CVMObject* thisRef)
{
    /* The weakref is dead, but we want to keep the referent alive
     * until the weakref object is cleared from the pending queue by
     * the ReferenceHandler thread. 
     */
    CVMtraceWeakrefs(("WR: CVMweakrefReferentKeep: "
		      "Enqueueing ref=%x (class %C), keeping referent\n",
		      thisRef, CVMobjectGetClass(thisRef)));
    CVMweakrefEnqueue(&CVMglobals.deferredWeakrefs3, thisRef);
    /*
    CVMweakrefEnqueuePending(thisRef);
    */
    return CVM_TRUE; /* Scan referent */
}


/*
 * A function to handle a weakref queue element. A weakref queue may
 * be one of the discovered references queues, or the pending
 * references queue.
 */
typedef void (*CVMWeakrefQueueCallback)(CVMObject* refAddr, void* data);

static void
CVMweakrefIterateQueue(CVMObject** queue,
		       CVMWeakrefQueueCallback qcallback,
		       void* data)
{
    CVMObject* thisRef = *queue;
    CVMObject* nextRef;
    if (thisRef == NULL) {
	return; /* Nothing to do */
    }
    while (CVM_TRUE) {
	nextRef = CVMweakrefField(thisRef, next);
	(*qcallback)(thisRef, data);
	if (thisRef == nextRef) {
	    break;
	} else {
	    thisRef = nextRef;
	}
    }
}

typedef struct CVMWeakrefHandlingParams {
    CVMRefLivenessQueryFunc isLive;
    void*                   isLiveData;
    CVMWeakrefHandleDying   handleDying;
    CVMRefCallbackFunc      transitiveScanner;
    void*                   transitiveScannerData;
} CVMWeakrefHandlingParams;

static void
CVMweakrefDiscoveredQueueCallback(CVMObject* thisRef, void* qdata)
{
    CVMWeakrefHandlingParams* params =
	(CVMWeakrefHandlingParams*)qdata;
    CVMRefLivenessQueryFunc isLive = params->isLive;
    CVMWeakrefHandleDying   handleDying = params->handleDying;
    CVMRefCallbackFunc      transitiveScanner = params->transitiveScanner;
    void* isLiveData = params->isLiveData;
    void* transitiveScannerData = params->transitiveScannerData;

    if ((*isLive)(CVMweakrefFieldAddr(thisRef, referent), isLiveData)) {
	/* Out of the queue, into the active state again */
	CVMassert(CVMweakrefField(thisRef, referent) != NULL);
	/* In case we ever have to undo this stage, defer the clean up
	   by enqueueing */
	CVMweakrefEnqueue(&CVMglobals.deferredWeakrefs, thisRef);
	/*
	CVMweakrefField(thisRef, next) = NULL;
	*/
	/*
	 * Keep this referent and its children alive
	 */
	CVMtraceWeakrefs(("WR: Referent 0x%x of 0x%x, "
			  "is NOT dying\n",
			  CVMweakrefField(thisRef, referent),
			  thisRef));
	(*transitiveScanner)(CVMweakrefFieldAddr(thisRef, referent),
			     transitiveScannerData);
    } else {
	/*
	 * GC says this referent is dying. Now let's see if
	 * handling this weak reference object causes us to keep
	 * the referent alive or not.  
	 */
	if ((*handleDying)(thisRef)) {
	    CVMassert(CVMweakrefField(thisRef, referent) != NULL);
	    (*transitiveScanner)(CVMweakrefFieldAddr(thisRef, referent),
				 transitiveScannerData);
	    /* The refs that are supposed to be in the pending queue are
	       in deferredWeakrefs3. */
	    CVMtraceWeakrefs(("WR: Kept referent 0x%x of 0x%x, "
			      "class %C at address 0x%x\n",
			      CVMweakrefField(thisRef, referent),
			      thisRef, CVMobjectGetClass(thisRef),
			      CVMweakrefFieldAddr(thisRef, referent)));
	} else {
	    /* The refs with referents-to-be-cleared are enqueued
	       in deferredWeakrefs2. */
	    CVMassert(CVMweakrefField(thisRef, referent) != NULL);
	    CVMassert(CVMweakrefField(thisRef, next) != NULL);
	}
    }
}

/*
 * Scan through and process weak references with non-strongly referenced
 * referents.
 */

/* Forward declaration */
static void
CVMweakrefHandlePendingQueue(CVMRefCallbackFunc callback, void* data);

/*
 * Clear JNI weak references, and transitively mark remaining live
 * ones.
 */
static void
CVMweakrefHandleJNIWeakRef(CVMObject** refPtr, void* qdata)
{
    CVMWeakrefHandlingParams* params =
	(CVMWeakrefHandlingParams*)qdata;
    CVMRefLivenessQueryFunc isLive = params->isLive;
    void* isLiveData = params->isLiveData;

    /*
     * No transitive scan required here.
     * Just delete non-live entries.
     */
    if (!(*isLive)(refPtr, isLiveData)) {
	*refPtr = NULL; /* Clear JNI weak ref */
	CVMtraceWeakrefs(("WR: Cleared JNI weak global ref "
			  "at address 0x%x\n", refPtr));
    }
}

static void
CVMweakrefHandleJNIWeakRefs(CVMWeakrefHandlingParams* params)
{
    CVMstackScanGCRootFrameRoots(&CVMglobals.weakGlobalRoots,
				 CVMweakrefHandleJNIWeakRef,
				 params);
}

void
CVMweakrefProcessNonStrong(CVMExecEnv* ee,
			   CVMRefLivenessQueryFunc isLive, void* isLiveData,
			   CVMRefCallbackFunc transitiveScanner,
			   void* transitiveScannerData,
			   CVMGCOptions* gcOpts)
{
    CVMWeakrefHandlingParams params;
    params.isLive = isLive;
    params.isLiveData = isLiveData;
    params.transitiveScanner = transitiveScanner;
    params.transitiveScannerData = transitiveScannerData;

    params.handleDying = CVMweakrefClearConditional;
    CVMweakrefIterateQueue(&CVMglobals.discoveredSoftRefs,
			   CVMweakrefDiscoveredQueueCallback,
			   &params);
    params.handleDying = CVMweakrefClearUnconditional;
    CVMweakrefIterateQueue(&CVMglobals.discoveredWeakRefs,
			   CVMweakrefDiscoveredQueueCallback,
			   &params);
    params.handleDying = CVMweakrefReferentKeep;
    CVMweakrefIterateQueue(&CVMglobals.discoveredFinalRefs,
			   CVMweakrefDiscoveredQueueCallback,
			   &params);
    params.handleDying = CVMweakrefReferentKeep;
    CVMweakrefIterateQueue(&CVMglobals.discoveredPhantomRefs,
			   CVMweakrefDiscoveredQueueCallback,
			   &params);

    /* At this point, all of the items of the discovered queues have
       been discovered. Clear that! */
    CVMweakrefClearDiscoveredQueues();
}

/*
 * Forward declarations */
static void
CVMweakrefRemoveQueueRefs(CVMObject** queue);

static void
CVMweakrefRemoveDiscoveredQueueRefs();

typedef struct CVMWeakrefUpdateParams {
    CVMRefCallbackFunc      callback;
    void*                   data;
} CVMWeakrefUpdateParams;

static void
CVMweakrefRollbackCallback(CVMObject* thisRef, void* qdata)
{
    CVMWeakrefUpdateParams* params = (CVMWeakrefUpdateParams*)qdata;
    CVMRefCallbackFunc callback = params->callback;
    void*              data = params->data;
    CVMObject**        refPtr;   
    CVMweakrefFieldSet(thisRef, next, NULL);
    refPtr = CVMweakrefFieldAddr(thisRef, referent);
    if (*refPtr != NULL) {
	(*callback)(refPtr, data);
    }
}

void
CVMweakrefRollbackHandling(CVMExecEnv* ee,
			   CVMGCOptions* gcOpts,
			   CVMRefCallbackFunc rootRollbackFunction,
			   void* rootRollbackData)
{
    CVMWeakrefUpdateParams params;

    params.callback = rootRollbackFunction;
    params.data = rootRollbackData;

    /* Rollback scanned referents, and return the containing reference
       objects to next==NULL state */
    CVMweakrefIterateQueue(&CVMglobals.deferredWeakrefs,
			   CVMweakrefRollbackCallback,
			   &params);
    
    CVMglobals.deferredWeakrefs = NULL;
    
    /* For those weakrefs whose referents were supposed to be cleared --
       simply set them free again by setting their 'next' pointers to NULL */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefs2);
    CVMglobals.deferredWeakrefs2 = NULL;

    /* Rollback scanned referents for dying refs */
    CVMweakrefIterateQueue(&CVMglobals.deferredWeakrefs3,
			   CVMweakrefRollbackCallback,
			   &params);
    
    CVMglobals.deferredWeakrefs3 = NULL;
    

    /* If we are doing the undo after CVMweakrefProcessNonStrong(),
       there won't be anything to remove in the discovered
       queues. Each element will have been queued either with
       deferredWeakRefs or in the pending queue.  If we do the undo
       before CVMweakrefProcessNonStrong(), none of the above would
       have done anything, and the below would be the primary
       activity. */
    CVMweakrefRemoveDiscoveredQueueRefs();
}

/*
 * Make sure that the 'next' fields of uncleared weak references are
 * properly scanned.  All the ones we care about are in the pending
 * queue.  
 *
 * There are two possibilities here:
 *
 * 1) The weak reference objects were discovered in their final
 * location. This would typically be the case in a semispace copying
 * GC. In that case, there is no need to update the next fields, since
 * these already point to the right objects.
 *
 * 2) The weak reference objects were discovered in their old
 * locations. This would typically be the case in a mark-compact like
 * GC. In that case, we need to update the next fields in the pending
 * queue to point to the new addresses.
 *
 * How can we avoid calling this for case #1?
 */
static void
CVMweakrefUpdateCallback(CVMObject* thisRef, void* qdata)
{
    CVMWeakrefUpdateParams* params = (CVMWeakrefUpdateParams*)qdata;
    CVMRefCallbackFunc callback = params->callback;
    void*              data = params->data;
    CVMObject**        refPtr;   
    refPtr = CVMweakrefFieldAddr(thisRef, next);
    CVMassert(*refPtr != NULL);
    (*callback)(refPtr, data);
    refPtr = CVMweakrefFieldAddr(thisRef, referent);
    if (*refPtr != NULL) {
	(*callback)(refPtr, data);
    }
}

static void
CVMweakrefHandlePendingQueue(CVMRefCallbackFunc callback, void* data)
{
    CVMWeakrefUpdateParams params;
    CVMObject** pendingQueue = CVMweakrefGetPendingQueue();
    if (*pendingQueue != NULL) {
	/* First handle the static Reference.pending, in case it just
	   got filled in */
	(*callback)(pendingQueue, data);
	/* And now the rest of the items */
	params.callback = callback;
	params.data = data;
	CVMweakrefIterateQueue(pendingQueue,
			       CVMweakrefUpdateCallback, &params);
    }
}

void
CVMweakrefUpdate(CVMExecEnv* ee,
		 CVMRefCallbackFunc refUpdate, void* updateData,
		 CVMGCOptions* gcOpts)
{
    /*
     * Update live JNI weak global refs
     */
    CVMstackScanRoots(ee, &CVMglobals.weakGlobalRoots,
		      refUpdate, updateData, gcOpts);
}

void 
CVMweakrefInit()
{
    CVMweakrefClearDiscoveredQueues();
    /*
     * Make sure that referent and next are the first two fields of
     * Reference.java. That makes the "map-shifting" possible
     * in CVMobjectWalkRefsAux().
     */
    CVMassert(CVMoffsetOfjava_lang_ref_Reference_referent == CVM_FIELD_OFFSET(0));
    CVMassert(CVMoffsetOfjava_lang_ref_Reference_next == CVM_FIELD_OFFSET(1));
}

/* Remove all enqueued refs waiting for ProcessNonStrong. This
   is normally done while backing out of a GC cycle
*/

static void
CVMweakrefRemoveQueueRefs(CVMObject** queue)
{
    CVMObject* thisRef = *queue;
    CVMObject* nextRef;
    if (thisRef == NULL) {
        return; /* Nothing to do */
    }
    while (CVM_TRUE) {
        nextRef = CVMweakrefField(thisRef, next);
	CVMweakrefFieldSet(thisRef, next, NULL);
        if (thisRef == nextRef) {
            break;
        } else {
            thisRef = nextRef;
        }
    }
}

static void
CVMweakrefClearReferentAndEnqueueCallback(CVMObject* thisRef, void* qdata)
{
    CVMweakrefFieldSet(thisRef, referent, NULL);
    CVMweakrefEnqueuePending(thisRef);
}

static void
CVMweakrefEnqueueInPendingCallback(CVMObject* thisRef, void* qdata)
{
    CVMweakrefEnqueuePending(thisRef);
}

void
CVMweakrefFinalizeProcessing(CVMExecEnv* ee,
			     CVMRefLivenessQueryFunc isLive, void* isLiveData,
			     CVMRefCallbackFunc transitiveScanner,
			     void* transitiveScannerData,
			     CVMGCOptions* gcOpts)
{
    CVMWeakrefHandlingParams params;
    params.isLive = isLive;
    params.isLiveData = isLiveData;
    params.transitiveScanner = transitiveScanner;
    params.transitiveScannerData = transitiveScannerData;

    /* We know that there will be no rollback. So do the final
       processing */

    /* First set the next fields of all those surviving weak
       refs. Their referents have already been scanned */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefs);
    CVMglobals.deferredWeakrefs = NULL;

    /* And for the next deferred weak refs queue of dying refs, now
       clear their referents and place each in the pending queue */
    CVMweakrefIterateQueue(&CVMglobals.deferredWeakrefs2,
			   CVMweakrefClearReferentAndEnqueueCallback,
			   NULL);
    CVMglobals.deferredWeakrefs2 = NULL;
    
    /* And for the final deferred weak refs queue of dying refs with
       uncleared referents, enqueue each in the pending queue. */
    CVMweakrefIterateQueue(&CVMglobals.deferredWeakrefs3,
			   CVMweakrefEnqueueInPendingCallback,
			   NULL);
    CVMglobals.deferredWeakrefs3 = NULL;
    
    /*
     * Doing the JNI pass here makes the JNI weak refs the fifth weak
     * ref strength.
     *
     * See bug #4126360 for details.
     */
    params.handleDying = NULL; /* Special case. No java.lang.ref.*
                                  objects here */
    CVMweakrefHandleJNIWeakRefs(&params);

    /*
     * Done classifying discovered references. Clear for next
     * discovery phase.
     */
    CVMweakrefClearDiscoveredQueues();

    /*
     * Now that we've processed all the discovered queues, it is time
     * for marking the pending queue, which might include items from the
     * previous iteration, as well as the currently enqueued items.
     * 
     * The 'next' and 'referent' fields need special scanning, since
     * they were skipped in the discovery phase.
     */
    CVMweakrefHandlePendingQueue(transitiveScanner, transitiveScannerData);

    CVMweakrefUpdate(ee, transitiveScanner, transitiveScannerData, gcOpts);

}

static void
CVMweakrefRemoveDiscoveredQueueRefs()
{
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredSoftRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredWeakRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredFinalRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredPhantomRefs);

    CVMweakrefClearDiscoveredQueues();
}
