/*
 * @(#)weakrefs.c	1.31 06/10/10
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
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/utils.h"
#include "javavm/include/string.h"

#include "javavm/include/preloader_impl.h"

/* How weakrefs work?
   ==================
   There are 5 containers for the 5 strengths of weak references:
   1. CVMglobals.discoveredSoftRefs queue for instances of SoftReference
   2. CVMglobals.discoveredWeakRefs queue for instances of WeakReference
   3. CVMglobals.discoveredFinalRefs queue for instances of FinalReference
   4. CVMglobals.discoveredPhantomRefs queue for instances of PhantomReference
   5. CVMglobals.weakGlobalRoots stack for JNI weakrefs

   The first 4 correspond to the respective java.lang.ref.* subclasses
   of java.lang.ref.Reference.  The 5th corresponds to the JNI weakrefs
   that are allocated and deallocated.  Hence it is not really a stack.
   Unlike the other references, the JNI weakrefs have no Java object
   instances associated with it.  JNI weakrefs are not part of the Java
   language specification but are part of the JNI specification.  Its
   behavior is similar to that of WeakReference except that it's not
   associated with any Reference object.

   The idea of weakrefs is essentially that a weakref can reference an
   object but the garbage collector may still choose to nullify this
   reference and collect the referent object (object being referred to)
   under certain conditions.

   How does Garbage Collection handle weakrefs?
   ============================================
   There are 3 main functions in the weakrefs code that the GC calls:
   1. CVMweakrefDiscover()
   2. CVMweakrefProcessNonStrong()
   3. CVMweakrefFinalizeProcessing()


   CVMweakrefDiscover()
   ====================
   During a GC cycle, as GC scanning finds objects that are live, GC checks
   if the object being scanned is a subclass of Reference.  If so, GC calls
   this function to declare that a Reference has been found.  Every instance
   of Reference (and its subclasses) has 2 important fields:
   1. next
   2. referent

   CVMweakrefDiscover() is only called on a Reference object if its next field
   is NULL.  This indicates that the Reference object is not in any queue.
   CVMweakrefDiscover() checks to see if the referent field is NULL.  If it is,
   then this is a NULL Reference.  Hence, there is no garbage collection
   activity that needs to be performed on it.  If the referent is not NULL,
   the object is added to the queue for its type.

   The act of enqueuing will set the next field in the Reference to a non-NULL
   value.  The next field is used as the link in the queue.  The queue is
   not NULL terminated.  The last element in the queue will point to itself in
   its next field.  This ensures that no Reference that has been enqueued will
   have a NULL in its next field.  This is also used to demark that the Reference
   has already been discovered.

   NOTE: During live object scanning, GC will ignore the referent and next
         fields of Reference objects (see CVMobjectWalkRefsAux()).  This is
         because the GC doesn't know yet how the Reference wants to treat
         these references.  These fields are scanned later as needed in
         CVMweakrefProcessNonStrong().

   NOTE: Since CVMweakrefDiscover() is called on object instances, it does not
         apply to JNI weakrefs.

   NOTE: Before GC begins, the 4 Reference object queues are NULL and empty.
         During live object scanning, Reference instances are added to these
         queues.

   CVMweakrefProcessNonStrong()
   ============================
   At some point when GC has found all the objects that it thinks are live by
   reachability, it will call CVMgcProcessSpecialWithLivenessInfo() (or its
   equivalent) which calls CVMweakrefProcessNonStrong().

   CVMweakrefProcessNonStrong() will iterate through the 4 ref queues and
   determine whether to keep the References' referent objects alive or not.
   The decision criteria varies based on the type of Reference object.

   If the referent object is to be kept alive, it will also call back into
   the GC to scan the network of objects that would be kept alive by the
   referent.  The following is how CVMweakrefProcessNonStrong() works in
   detail:

   1. It iterates through the 4 queues.  For each enqueued Reference, it
      does the following.

   2. It checks with the GC if the referent object is being kept alive by
      hard references (previously determined by a GC root scan).  If so, it
      enqueues the Reference object in the CVMglobals.deferredWeakrefs queue,
      and it also calls back into the GC to scan the referent object and its
      sub-network of objects (i.e. keeping this network of objects alive if
      GC hasn't already done so previously).

      NOTE: Queueing the Reference in CVMglobals.deferredWeakrefs is
            essentially preparing for the finalize phase to restore the
	    Reference to its original state before the GC i.e.
	        i. not queued in any queues
	        ii. the next field is NULL.
		iii. the referent field remains pointing to the object.  The
                     referent will now be scanned by the GC transitively to
                     keep its sub-network of objects alive.

   3. If there were no hard references to the referent, then the weakref
      gets to determine if it wants to keep the referent object alive.
      Depending on the type of Reference, it calls the following handlers
      to check if it wants to keep the referent alive:

               Ref Type	         handler
          i.   SoftReference:    CVMweakrefClearConditional()
	  ii.  WeakReference:    CVMweakrefClearUnconditional()
	  iii. FinalReference:   CVMweakrefReferentKeep()
	  iv.  PhantomReference: CVMweakrefReferentKeep()

      In the current implementation, CVMweakrefClearConditional() simply calls
      CVMweakrefClearUnconditional().

      CVMweakrefClearUnconditional() euqueues the Reference in
      CVMglobals.deferredWeakrefsToClear.
      NOTE: Queueing the Reference in CVMglobals.deferredWeakrefsToClear is
            essentially preparing for the finalize phase to nullify the
	    Reference's referent field, and enqueue the Reference in the
	    pending queue.

      CVMweakrefReferentKeep() enqueues the Reference in
      CVMglobals.deferredWeakrefsToAddToPending.
      NOTE: Queueing the Reference in CVMglobals.deferredWeakrefsToAddToPending
            is essentially preparing for the finalize phase to enqueue the
	    Reference in the pending queue.

   CVMweakrefFinalizeProcessing()
   ==============================
   CVMweakrefFinalizeProcessing() carries out the work of:

   1. Executing the respective actions on the References enqueued in
      CVMglobals.deferredWeakrefs, CVMglobals.deferredWeakrefsToClear, and
      CVMglobals.deferredWeakrefsToAddToPending as described above.
      NOTE: References from CVMglobals.deferredWeakrefsToClear and
            CVMglobals.deferredWeakrefsToAddToPending gets enqueued in the
	    the pending queue.

   2. Nullifies all JNI weakrefs if their referent object isn't being
      kept alive by either a hard reference or a Reference object.

   3. Reset the 4 Reference discovery queue i.e. returning to their
      initial empty state prior to the GC cycle.

   4. CVMweakrefHandlePendingQueue() is called to scan the References in the
      pending queue with the GC transitive scanner.  This is redundant because
      each of the referent objects were already scanned in
      CVMweakrefDiscoveredQueueCallback() when the References were being
      enqueued in deferredWeakrefsToClear and deferredWeakrefsToAddToPending.

      This is unless CVMweakrefFinalizeProcessing() is also expected to update
      the object pointers in the next and referent fields of the references.
      This is dependent on the GC algorithm.  In that case, the GC transitive
      scanner will update the object pointer.

   5. CVMweakrefUpdate() is called with the transitiveScanner to update the
      object pointers which have been moved.  There won't actually be any
      transitive scanning because all dead references are nullified in step 2
      above, and all live references are due to the existence of strong
      references, which in turn means that the object has already been
      transitively scanned before weakrefs are processed.

      Like step 4, this is not needed unless CVMweakrefFinalizeProcessing()
      is also expected to update the object pointers in the next and referent
      fields of the references.  This is dependent on the GC algorithm.  In
      that case, the GC transitive scanner will update the object pointer.

   Hence, gcOpts->isUpdatingObjectPointers is provided to allow the GC to
   bypass executing step 4 and 5 when the GC algorithm does not need it.

   Aborting a GC and restoring weakrefs to a consistent state (resetting):
   =======================================================================
   It is assumed that aborting GC means that no object motion has occurred
   i.e. previous object pointer values are still valid.  GC aborts can happen
   using a setjmp/longjmp mechanism.  Hence, the abort can happen in the
   midst of a callback function to GC.  We have to be careful that we leave
   the system in a consistent state (that we can clean up after) before
   calling back into GC code.

   Phase 1: Weakref Discovery
   If GC is aborted while weakrefs are being discovered, then there may
   be some Reference objects in the 4 weakref queues.  To reset these,
   set their next field back to NULL.

   Phase 2: ProcessNonStrong
   During this phase, References were being moved from the 4 weakref queues
   to the deferred queues.  References in the deferred queues could be easily
   restored to their active state by setting their next field to NULL.

   Phase 3: FinalizeProcessing
   At this point, it is assumed that GC will not abort anymore.  This phase
   actually changes the state of the References in an irreversible way i.e.
   references cannot be reset once we get to this phase.

   The work in Phase 1 and 2 basically moved References between queues.
   This movement need to be done in such a way that will not leave the
   queues in a partially modified (i.e. corrupted) state should GC chooses
   to abort.  This ensures that we can reset the references in an abort.

   NOTE: Part of the work of supporting GC aborts is in the use of
   CVMweakrefIterateQueue() instead of 
   CVMweakrefIterateQueueWithoutDequeueing().  CVMweakrefIterateQueue() will
   dequeue the reference it is iterating on before calling its callback
   function.  It is assumed that the callback function will either enqueue the
   reference onto another queue or set its "next" field to NULL before going
   on to call back into GC code which can abort.  Together, both these actions
   of CVMweakrefIterateQueue() and its callback functions ensure that the
   queues being operated on are left in a consistent state should the GC
   choose to abort.  Consistency here means that a reference will not appear
   on more than one queue when we have to handle clean up for GC abort.
*/

#ifdef CVM_DEBUG_ASSERTS
/* Assert that all the discovered weak reference queues are empty. */
static void
CVMweakrefAssertClearedDiscoveredQueues()
{
    CVMassert(CVMglobals.discoveredSoftRefs == NULL);
    CVMassert(CVMglobals.discoveredWeakRefs == NULL);
    CVMassert(CVMglobals.discoveredFinalRefs == NULL);
    CVMassert(CVMglobals.discoveredPhantomRefs == NULL);
}
#else
#define CVMweakrefAssertClearedDiscoveredQueues() {}
#endif /* CVM_DEBUG_ASSERTS */

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
    } else if (queue == &CVMglobals.deferredWeakrefs) {
	return "weakRefs to keep alive";
    } else if (queue == &CVMglobals.deferredWeakrefsToClear) {
	return "weakRefs to be cleared";
    } else if (queue == &CVMglobals.deferredWeakrefsToAddToPending) {
	return "weakRefs to be cleared and put in pending";
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

    /* NOTE: There are no callbacks into the GC here.  Hence, we don't
       have to worry about a GC abort in the middle of an enqueue
       operation leaving the queues in a corrupted state. */

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

    /* We're going to enqueue the weakref below, and we're expecting that
       it's not on any queue yet: */
    CVMassert(CVMweakrefField(weakRef, next) == NULL);

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
    CVMweakrefEnqueue(&CVMglobals.deferredWeakrefsToClear, thisRef);
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
    CVMweakrefEnqueue(&CVMglobals.deferredWeakrefsToAddToPending, thisRef);
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
        *queue = (thisRef == nextRef) ? NULL: nextRef;	/* Dequeue the ref. */

	/* NOTE: We expect the callback here will enqueue the ref in another
	   queue or nullify the "next" field in the reference before a GC
	   abort can possibly happen.  This is the responsibility of the
	   callback function. */
	(*qcallback)(thisRef, data);

	if (thisRef == nextRef) {
	    break;
	} else {
	    thisRef = nextRef;
	}
    }
}

static void
CVMweakrefIterateQueueWithoutDequeueing(CVMObject** queue,
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

/* NOTE: CVMweakrefDiscoveredQueueCallback() will enqueue each Reference it is
   called on into one of the 3 deferred queues.  It will also callback into
   the GC.  Hence, to ensure that we can support GC abort, we need to ensure
   that queue are in a consistent state before calling back to the GC.
   The queue iterator would have dequeued the Reference from the previous
   queue.  To ensure that the Reference is not lost, make sure to enqueue it
   into the new queue before calling GC's transitive Scanner. */
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
        /* If we get here, then the GC must have discovered some hard
           reference to this referent object.  We need to keep it and its
           sub-network of objects alive by calling the transitiveScanner()
           on it: */

	/* Out of the queue, into the active state again */
	CVMassert(CVMweakrefField(thisRef, referent) != NULL);
	/* In case we ever have to undo this stage, defer the clean up
	   by enqueueing */
	CVMweakrefEnqueue(&CVMglobals.deferredWeakrefs, thisRef);
	/*
	CVMweakrefField(thisRef, next) = NULL;
	*/
	/* Keep this referent and its children alive.
         * NOTE: Though this referent object is already known to be alive,
         * it's pointer value in the weakref may still be outdated.  The
	 * call to transitiveScanner() below may be necessary to allow the
         * GC to update the pointer in the weakref.
	 */
	CVMtraceWeakrefs(("WR: Referent 0x%x of 0x%x, "
			  "is NOT dying\n",
			  CVMweakrefField(thisRef, referent),
			  thisRef));
	(*transitiveScanner)(CVMweakrefFieldAddr(thisRef, referent),
			     transitiveScannerData);
    } else {

	/* GC says this referent is dying (i.e. there are no hard references
           to the referent object anymore). Call the handleDying() callback
	   to see if the weak reference object wants to keep it alive or not.
	   NOTE: the handleDying() callback will either enqueue the reference
	   in the deferredWeakrefsToClear or deferredWeakrefsToAddToPending
	   queues.
	 */
	if ((*handleDying)(thisRef)) {
	    CVMassert(CVMweakrefField(thisRef, referent) != NULL);
	    (*transitiveScanner)(CVMweakrefFieldAddr(thisRef, referent),
				 transitiveScannerData);
	    /* The refs that are supposed to be in the pending queue are
	       in deferredWeakrefsToAddToPending. */
	    CVMtraceWeakrefs(("WR: Kept referent 0x%x of 0x%x, "
			      "class %C at address 0x%x\n",
			      CVMweakrefField(thisRef, referent),
			      thisRef, CVMobjectGetClass(thisRef),
			      CVMweakrefFieldAddr(thisRef, referent)));
	} else {
	    /* The refs with referents-to-be-cleared are enqueued
	       in deferredWeakrefsToClear. */
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

    /* NOTE: CVMweakrefDiscoveredQueueCallback() or some of the handleDying
       callbacks will enqueue the reference being iterated into one of the
       queues: deferredWeakrefs, deferredWeakrefsToClear, or
       deferredWeakredsToAddToPending.  But since the GC transitive scanner
       can abort with a longjmp, we need to ensure that the discovered queues
       are left in a consistent state for each iteration.  Hence, we need to
       use the iterator (i.e. CVMweakrefIterateQueue()) that dequeues the
       references for each iteration of the queue.  This avoids the problem
       of a reference appearing on both a deferred queue and a discovered
       queue. */

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
       been processed.  The discovered queues should be empty! */
    CVMweakrefAssertClearedDiscoveredQueues();
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
    
    /* For those weakrefs whose referents were supposed to be cleared --
       simply set them free again by setting their 'next' pointers to NULL */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefsToClear);

    /* Rollback scanned referents for dying refs */
    CVMweakrefIterateQueue(&CVMglobals.deferredWeakrefsToAddToPending,
			   CVMweakrefRollbackCallback,
			   &params);

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
	CVMweakrefIterateQueueWithoutDequeueing(pendingQueue,
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
    CVMweakrefAssertClearedDiscoveredQueues();
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
    *queue = NULL; /* The queue is now empty. */
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

    /* NOTE: In the following, we will end up dequeueing each ref from the 2
       deferred queues anyway.  However, we choose to use
       CVMweakrefIterateQueueWithoutDequeueing() instead because the callback
       function will not call back to the GC and thereby, there is no possible
       GC aborts to handle here.  We're safe to use this less strict but
       faster iterator. */

    /* And for the next deferred weak refs queue of dying refs, now
       clear their referents and place each in the pending queue */
    CVMweakrefIterateQueueWithoutDequeueing(
        &CVMglobals.deferredWeakrefsToClear,
	CVMweakrefClearReferentAndEnqueueCallback, NULL);
    CVMglobals.deferredWeakrefsToClear = NULL;
    
    /* And for the final deferred weak refs queue of dying refs with
       uncleared referents, enqueue each in the pending queue. */
    CVMweakrefIterateQueueWithoutDequeueing(
        &CVMglobals.deferredWeakrefsToAddToPending,
	CVMweakrefEnqueueInPendingCallback, NULL);
    CVMglobals.deferredWeakrefsToAddToPending = NULL;
    
    /*
     * Doing the JNI pass here makes the JNI weak refs the fifth weak
     * ref strength.
     *
     * See bug #4126360 for details.
     */
    params.handleDying = NULL; /* Special case. No java.lang.ref.*
                                  objects here */
    CVMweakrefHandleJNIWeakRefs(&params);

    /* Done classifying discovered references. Ensure that the discovered
     * queues have been cleared for the next discovery phase. */
    CVMweakrefAssertClearedDiscoveredQueues();

    /*
     * Now that we've processed all the discovered queues, it is time
     * for marking the pending queue, which might include items from the
     * previous iteration, as well as the currently enqueued items.
     * 
     * The 'next' and 'referent' fields need special scanning, since
     * they were skipped in the discovery phase.
     */
    if (gcOpts->isUpdatingObjectPointers) {
        /* We only need to scan these if the transitiveScanner is also
	   responsible for updating object pointers that may have moved.
	   Otehrwise, we can skip this part.
	*/
        CVMweakrefHandlePendingQueue(transitiveScanner, transitiveScannerData);
	CVMweakrefUpdate(ee, transitiveScanner, transitiveScannerData, gcOpts);
    }
}

/* Purpose: Cleans up if GC aborts before phase 3: FinalizeProcessing(). */
void
CVMweakrefCleanUpForGCAbort(CVMExecEnv* ee)
{
    /* There are no side effects from having being on the discovered queues.
       Simply releasing the references from the queues will restore them to
       their original state. */
    CVMweakrefRemoveDiscoveredQueueRefs();

    /* References in deferredWeakrefs refer to live objects.  Hence, simply
       releasing them from the queue will restore them to their original
       state. */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefs);

    /* References in deferredWeakrefsToClear refer to references whose
       referent should be cleared.  However, this hasn't been done yet.
       Hence, simply releasing them from the queue will restore them to their
       original state. */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefsToClear);

    /* References in deferredWeakrefsToAddToPending refer to references which
       are meant to be enqueued in the Reference pending queue.  However, this
       hasn't been done yet.  Hence, simply releasing them from the queue will
       restore them to their original state. */
    CVMweakrefRemoveQueueRefs(&CVMglobals.deferredWeakrefsToAddToPending);
}

static void
CVMweakrefRemoveDiscoveredQueueRefs()
{
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredSoftRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredWeakRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredFinalRefs);
    CVMweakrefRemoveQueueRefs(&CVMglobals.discoveredPhantomRefs);

    CVMweakrefAssertClearedDiscoveredQueues();
}
