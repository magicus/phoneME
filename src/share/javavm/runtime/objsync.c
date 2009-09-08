/*
 * @(#)objsync.c	1.100 06/10/10
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
 */

#include "javavm/include/defs.h"
#include "javavm/include/sync.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/clib.h"
#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

/* NOTES: About CVM object monitors and their life-cycles
   ======================================================

   The Fast Locking mechanism:
   ==========================
   CVM implements a fast locking mechanism which attempts to speed up monitor
   locking/unlocking operations for the monitors which meet the following
   criteria:
     1. The object monitor is locked and unlocked only by one thread at a time
        (i.e. no thread contention occurs while trying to lock or unlock the
        monitor).
     2. The object monitor isn't used for the Object.wait(), Object.notify(),
        Object.notifyAll().

   If the above criteria is met, then the lock is implemented by simply
   setting the low 2 bits of a header bit flag word in each object using some
   form of atomic operation.  The fast lock mechanism basically provides some
   speed optimization benefits due to:
     1. Not having to actually invoke OS system calls to allocate and
        lock/unlock a monitor.
     2. Reduce the amount of memory/resource allocations necessary to implement
        the monitor.  Monitors are allocated per thread as well as globally and
        are reclaimed for reuse through various scavenging mechanism.  This
        reuse speeds up monitor operations if a previously reclaimed monitor is
        available for reuse.

   At any time that the criteria for fast locking/unlocking are not met, the
   monitor implementation will inflate itself into a heavy weight monitor which
   relies on the platform-specific mutex and condvar implementations defined by
   the porting layer to do its locking/unlocking.

   Data Structures & Ownership of the data structures:
   ==================================================

   CVM object monitors are implemented using 2 data structures:
     1. The CVMOwnedMonitor record.
     2. The CVMObjMonitor monitor implementation.

     The CVMOwnedMonitor record is used to track locking information with or
     without an actual monitor.  CVMOwnedMonitors are allocated, freed, and
     modified exclusively by its owner thread.  This avoids contention between
     threads over the manipulation of CVMOwnedMonitors.

     The CVMObjMonitor is the actual monitor that is attached to the object
     when a heavy-weight lock needs to employed.  CVMObjMonitors, unlike
     CVMOwnedMonitors, are shared between threads and held in the global list.
     Contention is minimized by having each thread cache one instance of the
     CVMObjMonitor for its own use during the inflation process.  Otherwise,
     CVMObjMonitors in the global lists are protected from contention between
     threads by the CVMglobals.syncLock.

   In CVMglobals, there are:
     1. CVMglobals.objLocksBound: This is the list of bound monitors i.e.
            CVMObjMonitors which are attached to an object.
     2. CVMglobals.objLocksUnbound: This is the list of unbound monitors i.e.
            CVMObjMonitors which used to be attached to objects which have now
            been GC'ed.  This means that the monitor's object pointer is NULL.
     3. CVMglobals.objLocksFree: This is the list of free monitors i.e
            CVMObjMonitors which are not attached to any objects yet.

     4. CVMglobals.syncLock: This is the global lock used for protecting the
            above 3 monitor lists and for synchronizing threads which wish to:
            inflate a monitor, set an object hash code, or do an unlock
            operation on a fast lock.

   Each thread's CVMExecEnv has:
     1. ee->objLockCurrent: This is a per-thread reference to a CVMObjMonitor
            that is in the midst of being processed and has not yet been
            attached to an object yet.  This is necessary to prevent the
            referenced CVMObjMonitor from being scavenged prematurely.
     2. ee->objLocksFreeUnlocked: This is supply of CVMObjMonitors for use by
            the the thread in the event that it needs to do a monitor
            inflation.  The inflation process will replenish
            ee->objLocksFreeUnlocked at the end of the inflation process.
            The current implementation only keeps one CVMObjMonitor available
            in ee->objLocksFreeUnlocked.
     3. ee->objLocksOwned: This is a list of all the CVMOwnedMonitors which are
            locked by the thread.  These CVMOwnedMonitors may or may not be
            attached to an actual CVMObjMonitor.
     4. ee->objLocksFreeOwned: This is a list of free CVMOwnedMonitors
            available for the thread's use.

        Write access to these fields of the CVMExecEnv is exclusively owned by
        owner thread.  This avoids any contention between threads, and, hence,
        no synchronization mechanism is needed to synchronize accesses to these
        fields.

   The Monitor Life-Cycles & Processes:
   ===================================

    Inflation:
        . This is the process by which an object gets attached to a
          CVMObjMonitor.  This process is protected by the CVMglobals.syncLock.

    Initial state:
        . The object header bits indicate CVM_LOCKSTATE_UNLOCKED.

    Locked without contention (fast lock):
        . The object header bits indicate CVM_LOCKSTATE_LOCKED and holds a
          pointer to a CVMOwnedMonitor instance.
        . The thread who owns the lock on that object also holds a pointer to
          the same CVMOwnedMonitor in its owned list.
        . If the object header bits already indicate CVM_LOCKSTATE_LOCKED upon
          attempting to lock the monitor, then the CVMOwnedMonitor's reentry
          count is incremented.

    Locked with contention (heavy weight lock):
        . The object header bits indicate CVM_LOCKSTATE_MONITOR and holds a
          pointer to a CVMObjMonitor instance.
        . The CVMObjMonitor is marked as CVM_OBJMON_OWNED and put on the
          global bound list.
        . The thread who owns the lock still have a pointer to the
          CVMOwnedMonitor record.  The CVMOwnedMonitor record has a pointer to
          the CVMObjMonitor for the lock.

    Unlocked while holding a fast lock:
        . The CVMOwnedMonitor's reentry count is decremented.  If this count
          has not reached 0 yet, then the unlock operation is done.  Otherwise,
          the following steps are executed as well.
        . The object header bits are changed back to CVM_LOCKSTATE_UNLOCKED.
        . The CVMOwnedMonitor is moved from the owner thread's ee's owned list
          to its free list.

    Unlocked while holding a heavy weight lock:
        . The object header bits remains CVM_LOCKSTATE_MONITOR and the object
          retains its pointer to the CVMObjMonitor instance.
        . The CVMObjMonitor is marked as CVM_OBJMON_BOUND and remains on the
          global bound list.

    Each thread always keep one free CVMObjMonitor around to accelerate
    impending monitor inflation operations.  After a monitor has been inflated,
    the thread will attempt to replenish its spare free CVMObjMonitor.  It does
    this by either:
        1. Getting one from the global free list.
        2. Scavenging for bound monitors which are not locked.
        3. Allocating new ones.

    During CVMObjMonitor scavenging:
        . CVMObjMonitors which are bound (on the global bound list) but not
          locked are released into the global free list.
        . CVMObjMonitors which are on the global unbound list (see notes on
          monitor GC below) are unlocked and released into the global free
          list.

    CVMOwnedMonitor scavenging:
        . CVMOwnedMonitor needed by most monitor operations and attained before
          the operation commences (at the top of the functions).
        . Usually, it is attained from the thread's free list.  If the free
          list is empty, then the thread will attempt to replenish the free
          CVMOwnedMonitor supply by one of the following ways:
            1. Allocate a new one.
            2. Run the scavenger which searches the thread's owned list for
               monitors which have fast locks but are no longer associated
               with a live object (see notes on monitor GC below).  These
               monitor records are unlocked and released back into the thread's
               free list.  Since this case does not naturally happen with Java
               code, it is unlikely to occur.  Hence a scavenging operation is
               unlikely to yield a free CVMOwnedMonitor.  This is why we opt to
               allocate a new one first before attempting a scavenging
               operation.

    During a monitor GC (invoked by the GC):
        . CVMObjMonitors which are locked but no longer associated with a live
          object (because the object has been GC'ed but the lock hasn't been
          released prior) are moved from the global bound list to the global
          unbound list.  See notes on monitor scavenging above to see how these
          monitors get released back into the free lists for reuse.

          The CVMObjMonitors do not get moved directly from the bound list to
          the free list because bound CVMObjMonitors are attached to
          CVMOwnedMonitors.  Hence, the best that the GC thread can do is to
          tag the monitor by putting it on the unbound list and relying on the
          owner thread to release the attached CVMOwnedMonitor as well as put
          the CVMObjMonitor back on the free list.

    During thread destruction:
        . When a thread shuts down, it will unlock all monitors whose lock it
          owns if those monitors are still associated with live objects.
        . While attempting to unlock monitors, if the thread encounters any
          monitors which are no longer associated with any live objects, it
          will invoke the appropriate scavengers to reclaim the monitors into
          the free lists.
        . After reclaiming or unlocking all the monitors on its owned list, the
          thread will release/free up all the free monitors on its free list
          before exiting.

    Deflation of heavy weight monitors:
    ==================================
    Heavy weight monitors are deflated in the scavenging process.  Since a
    scavenging process is invoked immediately after each monitor inflation
    process, unlocked inflated monitors don't stay around for long.

    Synchronization between various Monitor Methods:
    ===============================================
    The following methods must be protected from themselves and each other by
    sync'ing on the same lock.  The lock currently used for this purpose is
    CVMglobals.syncLock.  The methods are as follows:
        1. CVMobjectInflate()
        2. CVMobjectSetHashBitsIfNeeded()
        3. CVMfastUnlock()

    Assumptions:
    ===========
    1. It is assumed that we won't ever enter a monitor recursively up to an
       reentry count of CVM_MAX_REENTRY_COUNT.  This allows
       CVM_INVALID_REENTRY_COUNT to be above CVM_MAX_REENTRY_COUNT.
       Debug builds will check that count never reaches CVM_MAX_REENTRY_COUNT.
*/

#ifndef CVM_INITIAL_NUMBER_OF_OBJ_MONITORS
#define CVM_INITIAL_NUMBER_OF_OBJ_MONITORS      10
#endif

static void CVMmonitorAttachObjMonitor2OwnedMonitor(CVMExecEnv *ee,
                                                    CVMObjMonitor *mon);
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
static CVMBool CVMfastReentryTryLock(CVMExecEnv *ee, CVMObject *obj);
#endif
static void CVMsyncMonitorScavenge(CVMExecEnv *ee, CVMBool releaseOwnedRecs);
static void CVMsyncGCSafeAllMonitorScavengeInternal(CVMExecEnv *ee,
                                                    CVMBool releaseOwnedRecs);
static void CVMmonitorScavengeFast(CVMExecEnv *ee);
static void CVMownedMonitorInit(CVMOwnedMonitor *m, CVMExecEnv *owner);
static void CVMownedMonitorDestroy(CVMOwnedMonitor *m);
#ifdef CVM_THREAD_BOOST
static void CVMboost(CVMExecEnv *ee, CVMObjMonitor *mon, CVMExecEnv *owner);
static void CVMunboost(CVMExecEnv *ee, CVMObjMonitor *mon);
#endif

static CVMTryLockFunc   CVMdetTryUnlock, CVMdetTryLock;
static CVMLockFunc      CVMfastLock, CVMdetLock;
static CVMLockFunc      CVMfastUnlock, CVMdetUnlock;
static CVMNotifyFunc    CVMfastNotify, CVMdetNotify;
static CVMNotifyAllFunc CVMfastNotifyAll, CVMdetNotifyAll;
static CVMWaitFunc      CVMfastWait, CVMdetWait;

/*
 * The locking vector.
 */
const CVMSyncVector CVMsyncKinds[2] = {
    {CVMfastTryLock, CVMfastLock, CVMfastTryUnlock, CVMfastUnlock,
     CVMfastWait, CVMfastNotify, CVMfastNotifyAll, NULL},
    {CVMdetTryLock,  CVMdetLock,  CVMdetTryUnlock, CVMdetUnlock,
     CVMdetWait,  CVMdetNotify,  CVMdetNotifyAll, NULL}
};

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK
/* NOTE: The rendezvousOwnedRec below is used as a fake CVMOwnedMonitor that
   is not owned by any thread (hence, the owner is NULL).  It is used to
   replace the various words field in the object header when we want to
   inflate the monitor into a CVMObjMonitor.  This replacement ensures that
   no fast locking and unlocking activity occurs on this object without having
   to hold onto the object microlock the whole time we're doing the inflation.

   This also avoids any complications due to any side effects of holding onto
   the object microlock while waiting for various HPI functions (e.g.
   CVMmutexSetOwner()) to complete.
*/
static const CVMOwnedMonitor rendezvousOwnedRec = {
    /* owner    */ NULL,
    /* type     */ CVM_OWNEDMON_FAST,
    /* object   */ NULL,
    /* u        */ {{0}},
    /* next     */ NULL,
#ifdef CVM_DEBUG
    /* magic    */ CVM_OWNEDMON_MAGIC,
    /* state    */ CVM_OWNEDMON_OWNED,
#endif
    /* count    */ 1
};
#endif

static void
CVMmonEnter(CVMExecEnv *ee, CVMObjMonitor *mon)
{
    /* This is like a reference count, needed if count==0 */
    ee->objLockCurrent = mon;

#ifdef CVM_THREAD_BOOST
    if (mon->boost) {
        /* If we get here, then the ownership of the monitor has not actually
	   been claimed the owner thread yet though it has been marked as so.
	   The following ensures that ownership is claimed by the owner thread
	   before we enter the monitor. */
	CVMExecEnv *owner = CVMprofiledMonitorGetOwner(&mon->mon);
	if (owner == ee) {
            /* mon->boost being true and this thread being the owner implies
	       that another thread had inflated this monitor for us.  We'll
	       need to claim ownership of the monitor and unboost our thread
	       priority.  Only after that we'll we re-enter the the monitor.
	    */
	    CVMunboost(ee, mon);
	    CVMassert(!mon->boost);
	    CVMprofiledMonitorEnterUnsafe(&mon->mon, ee, CVM_TRUE);
	} else {
	    /* This is the case where we discovered a contended enter and
	       inflated the monitor.  We therefore need to boost the thread
	       priority of the owner and wait for it to claim ownership of
	       the monitor before we try to acquire it ourselves. */
	    CVMprofiledMonitorPreContendedEnterUnsafe(&mon->mon, ee);
	    CVMboost(ee, mon, owner);
	    CVMprofiledMonitorContendedEnterUnsafe(&mon->mon, ee);
	}
    } else {
	CVMprofiledMonitorEnterUnsafe(&mon->mon, ee, CVM_TRUE);
    }
    CVMassert(!mon->boost);
#else
    CVMprofiledMonitorEnterUnsafe(&mon->mon, ee, CVM_TRUE);
#endif

    ee->objLockCurrent = NULL;

    CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == ee);
}

static CVMBool
CVMmonTryEnter(CVMExecEnv *ee, CVMObjMonitor *mon)
{
#ifdef CVM_THREAD_BOOST
    if (mon->boost) {
	return CVM_FALSE;
    }
#endif
    if (!CVMprofiledMonitorTryEnter(&mon->mon, ee)) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

static void
CVMmonExit(CVMExecEnv *ee, CVMObjMonitor *mon)
{
#ifdef CVM_THREAD_BOOST
    if (mon->boost) {
        /* It is implied that we own the monitor since we're trying to exit
	   it.  mon->boost being true indicates that another thread had
	   inflated this monitor for us i.e. we have not actually claimed
	   ownership of the monitor.  Some contender thread could have boosted
	   our thread priority while waiting for us to claim ownership of the
	   monitor.  So, call CVMunboost() to claim the monitor and unboost
	   our thread priority.  If it turns out that we don't own this
	   monitor, CVMunboost() will do nothing. */
	CVMunboost(ee, mon);
    }
    CVMassert(!mon->boost);
#endif
    CVMprofiledMonitorExit(&mon->mon, ee);
}


/* Purpose: Attaches the specified CVMObjMonitor to a free CVMOwnedMonitor. */
static void CVMmonitorAttachObjMonitor2OwnedMonitor(CVMExecEnv *ee,
                                                    CVMObjMonitor *mon)
{
    CVMOwnedMonitor *o = ee->objLocksFreeOwned;

    /* Remove the CVMOwnedMonitor from the ee's free list: */
    ee->objLocksFreeOwned = o->next;

    /* Attach the CVMOwnedMonitor to the CVMObjMonitor: */
    CVMassert(o != NULL);
    CVMassert(mon->state == CVM_OBJMON_BOUND);
    mon->state = CVM_OBJMON_OWNED;
    o->type = CVM_OWNEDMON_HEAVY;
#ifdef CVM_DEBUG
    o->state = CVM_OWNEDMON_OWNED;
    mon->owner = o;
#endif
    o->object = mon->obj;
    o->u.heavy.mon = mon;

    /* Add the CVMOwnedMonitor to the ee's owned list: */
    o->next = ee->objLocksOwned;
    ee->objLocksOwned = o;
}

/* Pointer to the microlock so asm code can easily locate it */
CVMMicroLock* const CVMobjGlobalMicroLockPtr = &CVMglobals.objGlobalMicroLock;

/*
 * The fast lockers.
 * All functions are called while gc-unsafe, but Lock is
 * allowed to become safe because it can block.
 */

CVMBool
CVMfastTryLock(CVMExecEnv* ee, CVMObject* obj)
{
    CVMOwnedMonitor *o = ee->objLocksFreeOwned;

    /* return FALSE on:
     * - exception
     * - contention
     * - monitor not allocated
     */

    /* %comment l005 */
    CVMtraceFastLock(("fastTryLock(%x,%x)\n", ee, obj));

    if (o == NULL) {
	return CVM_FALSE;
    }
#ifdef CVM_JVMTI
    if (CVMjvmtiIsInDebugMode()) {
        /* just check for an available lockinfo */
        if (!CVMjvmtiCheckLockInfo(ee)) {
            return CVM_FALSE;
        }
    }
#endif

    /* %comment l006 */
#ifdef CVM_DEBUG
    CVMassert(o->state == CVM_OWNEDMON_FREE);
#endif
    CVMassert(o->type == CVM_OWNEDMON_FAST);
    o->object = obj;

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE

    o->count = 1;
#ifdef CVM_DEBUG
    o->state = CVM_OWNEDMON_OWNED;
#endif
    {
	/* 
	 * nbits can hold a native pointer 
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr nbits = (CVMAddr)(o) | CVM_LOCKSTATE_LOCKED;

	CVMassert(o->owner == ee);
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
	{
	    /*
	     * obits can hold a native pointer 
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    CVMAddr obits0;
	    CVMAddr obits =
		CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED;

	    o->u.fast.bits = obits; /* be optimistic */
	    /* 
	     * The address of the various32 field has to be 8 byte aligned on
	     * 64 bit platforms.
	     */
	    CVMassert((CVMAddr)CVMalignAddrUp(&obj->hdr.various32) == 
		      (CVMAddr)&obj->hdr.various32);
	    obits0 = CVMatomicCompareAndSwap(&obj->hdr.various32,
		nbits, obits);
	    if (obits0 != obits) {
		goto fast_failed;
	    }
	}

#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK

	/* NOTE: All changes to bits must be made under micro lock */
	CVMobjectMicroLock(ee, obj);
	{
	    /*
	     * obits can hold a native pointer 
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    CVMAddr obits = obj->hdr.various32;
	    if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_UNLOCKED) {
		o->u.fast.bits = obits;
		obj->hdr.various32 = nbits;
	    } else {
		CVMobjectMicroUnlock(ee, obj);
		goto fast_failed;
	    }
	}
	CVMobjectMicroUnlock(ee, obj);
#else
#error Unknown value for CVM_FASTLOCK_TYPE
#endif
	/*
	 * After this point, the monitor could have been inflated
	 * by another thread...
	 */
	CVMassert(ee->objLocksFreeOwned == o);

	/* remove from free list */
	ee->objLocksFreeOwned = o->next;

	o->next = ee->objLocksOwned;
	ee->objLocksOwned = o;
#ifdef CVM_JVMTI
	if (CVMjvmtiIsInDebugMode()) {
	    CVMjvmtiAddLockInfo(ee, NULL, o, CVM_FALSE);
	}
#endif
	return CVM_TRUE;
    }

fast_failed:

#ifdef CVM_DEBUG
    o->count = 0;
    o->state = CVM_OWNEDMON_FREE;
#endif

    /* If we're already in a fast lock, then try to reenter it: */
    if (CVMhdrBitsSync(CVMobjectVariousWord(obj)) == CVM_LOCKSTATE_LOCKED) {
        CVMBool success = CVMfastReentryTryLock(ee, obj);
        if (!success) {
            goto fast_reentry_failed;
        } else {
            return success;
        }
    }

fast_reentry_failed:

#endif

    /* If we're here, then we've failed to put a fast lock on this object: */
    if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
	CVMObjMonitor *mon = CVMobjMonitor(obj);
#ifdef CVM_DEBUG
	CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	CVMassert(mon->obj == obj);
        if (!CVMmonTryEnter(ee, mon)) {
	    return CVM_FALSE;
	}

        if (CVMprofiledMonitorGetCount(&mon->mon) == 1) {
	    CVMassert(ee->objLocksFreeOwned == o);
            CVMmonitorAttachObjMonitor2OwnedMonitor(ee, mon);
	}
#ifdef CVM_JVMTI
	if (CVMjvmtiIsInDebugMode()) {
	    CVMjvmtiAddLockInfo(ee, mon, NULL, CVM_FALSE);
	}
#endif
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE

/* Purpose: Performs a TryLock on an object that already has a fast lock. */
static CVMBool
CVMfastReentryTryLock(CVMExecEnv *ee, CVMObject *obj)
{
    /*
     * obits can hold a native pointer (see CVMobjectVariousWord())
     * therefore the type has to be CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr obits;
    CVMOwnedMonitor *o;

    /* %comment l005 */
    CVMtraceFastLock(("fastReentryTryLock(%x,%x)\n", ee, obj));

    /* Sample the bits needed to get the CVMOwnedMonitor for this fast lock: */
    obits = CVMobjectVariousWord(obj);

    /* The bits will contain the address of the CVMOwnedMonitor if and only if
       the SYNC bits equals CVM_LOCKSTATE_LOCKED and the ptr value is not
       NULL: */
    if (CVMhdrBitsSync(obits) != CVM_LOCKSTATE_LOCKED) {
        return CVM_FALSE;
    }
    o = (CVMOwnedMonitor *) CVMhdrBitsPtr(obits);
    if (o == NULL) {
        return CVM_FALSE;
    }
    /* Fail if this thread does not own this monitor: */
    if (o->owner != ee) {
        return CVM_FALSE;
    }

    /* If we get here then we know that we have a valid CVMOwnedMonitor. */
    {
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        {
	    /*
	     * expectedOldCount, newCount and oldCount are used in
	     * conjunction with CVMatomicCompareAndSwap() and can 
	     * therefore hold a native pointer.
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
            CVMAddr expectedOldCount = o->count;
            CVMAddr newCount;
            CVMAddr oldCount;

            /* If the count is CVM_INVALID_REENTRY_COUNT after we've acquired
               a fast lock, then it must mean that another thread is currently
               inflating this monitor: */
            if (expectedOldCount == CVM_INVALID_REENTRY_COUNT) {
                goto reentry_failed;
            }
            newCount = expectedOldCount + 1;
            oldCount = CVMatomicCompareAndSwap(&o->count, newCount,
                                               expectedOldCount);
            /* We either succeed in incrementing the count or not.  If not,
               then the CompareAndSwap will fail which means that another
               thread must be trying to inflate this monitor: */
            if (oldCount != expectedOldCount) {
                goto reentry_failed;
            }
        }

#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK

        /* NOTE: All changes to bits must be made under micro lock */
        CVMobjectMicroLock(ee, obj);
	{
	    /*
	     * obits can hold a native pointer 
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    CVMAddr obits0 = CVMobjectVariousWord(obj);
            /* Make sure that the monitor hasn't been inflated yet: */
            if (obits0 != obits) {
		CVMobjectMicroUnlock(ee, obj);
		return CVM_FALSE;
	    }
	    CVMassert(o->count < CVM_MAX_REENTRY_COUNT);
	    o->count++;
	}
	CVMobjectMicroUnlock(ee, obj);

#else
#error Unknown value for CVM_FASTLOCK_TYPE
#endif
    }
#ifdef CVM_JVMTI
    if (CVMjvmtiIsInDebugMode()) {
	CVMjvmtiAddLockInfo(ee, NULL, o, CVM_FALSE);
    }
#endif
    return CVM_TRUE;

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
reentry_failed:

    return CVM_FALSE;
#endif
}

#endif  /* CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE */

/* Purpose: Replenish the thread specific free list of CVMObjMonitors by
            tapping into the global free list. */
static void
CVMreplenishFreeListsNonBlocking(CVMExecEnv *ee)
{
    CVMObjMonitor *f = ee->objLocksFreeUnlocked;
    /*
     * Grab a free lock from the global pool, if available
     * CVMglobals.objLocksFree is protected by syncLock
     */
    if (f == NULL) {
	CVMObjMonitor *m = CVMglobals.objLocksFree;
	if (m != NULL) {
	    CVMassert(m->state == CVM_OBJMON_FREE);
            if (CVMprofiledMonitorGetOwner(&m->mon) == NULL) {
		CVMglobals.objLocksFree = m->next;
		m->next = ee->objLocksFreeUnlocked;
		ee->objLocksFreeUnlocked = f = m;
	    }
	}
	if (f == NULL && ee->threadExiting) {
	    CVMObjMonitor *o = ee->objLocksReservedUnlocked;
	    /* If this fails, increase CVM_RESERVED_OBJMON_COUNT below */
	    CVMassert(o != NULL);
	    ee->objLocksReservedUnlocked = o->next;
	    ee->objLocksFreeUnlocked = o;
	    o->next = NULL;
	}
    }
}

/* Purpose: Replenish the thread specific free list of CVMObjMonitors by
            scavenging for unused CVMObjMonitors or creating a new one. */
static void
CVMreplenishFreeListsBlocking(CVMExecEnv *ee)
{
    /* NOTE: We need to attempt scavenging first before allocating a new
       ObjMon because scavenging is the only mechanism by which these
       monitors get reused.  Otherwise, we will end up allocating as
       many monitors as memory will allow, and unused monitors that could
       have been reused will remain unused because the scavenger won't be
       called until we run out of memory. */

    if (ee->objLocksFreeUnlocked == NULL) {
        /* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed
            by CVMsyncMonitorScavenge().  Do not cache these values. */
        CVMsyncMonitorScavenge(ee, CVM_FALSE);
        if (CVMglobals.objLocksFree != NULL) {
            /* It appears that after scavenging, we have replenished the global
               free list.  So try to replenish the thread specific free list
               for CVMObjMonitors by tapping into the global free list. 
               NOTE: This is not guaranteed to succeed because some other
                 thread might come along and consume the monitor our scavenger
                 has just freed. */
            CVMsysMutexLock(ee, &CVMglobals.syncLock);
            CVMreplenishFreeListsNonBlocking(ee);
            CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
        }

        /* If after scavenging, we still aren't able to fill the need, then
           it's time to allocate a new one: */
        if (ee->objLocksFreeUnlocked == NULL) {
            CVMObjMonitor *mon;
            mon = (CVMObjMonitor *)calloc(1, sizeof (CVMObjMonitor));
            if (mon != NULL) {
                if (CVMobjMonitorInit(ee, mon, NULL, 0)) {
                    ee->objLocksFreeUnlocked = mon;
                    CVMassert(CVMobjMonitorOwner(mon) == NULL);
                } else {
                    free(mon);
                }
            }
        }
    }
}

/* Purpose: Replenish the thread specific CVMOwnedMonitor free list either by
            scavenging for it or creating a new one.  */
static void
CVMreplenishLockRecordUnsafe(CVMExecEnv *ee)
{
    CVMassert(CVMD_isgcUnsafe(ee));

    /* We attempt to create a new CVMOwnedMonitor first because it is unlikely
       that scavenging will turn up any unused CVMOwnedMonitors.  See full
       comments at the top of the file for details. */
    {
	CVMOwnedMonitor *o;
	CVMD_gcSafeExec(ee, {
	    o = (CVMOwnedMonitor *)calloc(1, sizeof (CVMOwnedMonitor));
	    if (o != NULL) {
		CVMownedMonitorInit(o, ee);
	    }
	});
	if (o != NULL) {
	    o->next = ee->objLocksFreeOwned;
	    ee->objLocksFreeOwned = o;
	}
    }

    /* Scavenge for unused CVMOwnedMonitors if necessary: */
    if (ee->objLocksFreeOwned == NULL) {
        /* We call CVMmonitorScavengeFast() here instead of
           CVMsyncMonitorScavenge() because we only need to replenish our
           supply of CVMOwnedMonitors and not CVMObjMonitors.  Since
           CVMsyncMonitorScavenge() is significantly more costly to run and
           does not get us any additional benefit to help satisfy our needs,
           calling CVMmonitorScavengeFast() instead is a preferred choice.

           NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed
             by CVMmonitorScavengeFast().  Do not cache these values.
        */
        CVMmonitorScavengeFast(ee);
    }

    if (ee->objLocksFreeOwned == NULL && ee->threadExiting) {
	CVMOwnedMonitor *o = ee->objLocksReservedOwned;
	/* If this fails, increase CVM_RESERVED_OWNMON_COUNT below */
	CVMassert(o != NULL);
	ee->objLocksReservedOwned = o->next;
	ee->objLocksFreeOwned = o;
	o->next = NULL;
    }
}

/* Pin an (inflated) object monitor.  It will remain inflated until */
/* it is unpinned. */
static void
CVMobjMonitorPin(CVMExecEnv *ee, CVMObjMonitor *mon)
{
    CVMassert(ee->objLocksPinnedCount < CVM_PINNED_OBJMON_COUNT);
    ee->objLocksPinned[ee->objLocksPinnedCount++] = mon;
}

/* Unpin all monitors that were pinned by this thread. */
static void
CVMobjMonitorUnpinAll(CVMExecEnv *ee)
{
#ifdef CVM_DEBUG_ASSERTS
    int i;
    int n = ee->objLocksPinnedCount;
    for (i = 0; i < n; ++i) {
	ee->objLocksPinned[i] = NULL;
    }
#endif
    CVMtraceExec(CVM_DEBUGFLAG(TRACE_MISC), {
	CVMconsolePrintf("Unpinned %d monitors\n", ee->objLocksPinnedCount);
    });
    ee->objLocksPinnedCount = 0;
}

/* Mark pinned monitors as busy */
static void
CVMobjMonitorMarkPinnedAsBusy(CVMExecEnv *ee)
{
    int i;
    for (i = 0; i < ee->objLocksPinnedCount; ++i) {
	CVMassert(ee->objLocksPinned[i]->obj != NULL);
#ifdef CVM_DEBUG_ASSERTS
	{
	    int state = ee->objLocksPinned[i]->state & ~CVM_OBJMON_BUSY;
	    CVMassert(state == CVM_OBJMON_BOUND || state == CVM_OBJMON_OWNED);
	}
#endif
	ee->objLocksPinned[i]->state |= CVM_OBJMON_BUSY;
    }
}

/* Clear the busy flag from pinned monitors.  Undoes what */
/* CVMobjMonitorMarkPinnedAsBusy() did. */
static void
CVMobjMonitorMarkPinnedAsUnbusy(CVMExecEnv *ee)
{
    int i;
    for (i = 0; i < ee->objLocksPinnedCount; ++i) {
	ee->objLocksPinned[i]->state &= ~CVM_OBJMON_BUSY;
    }
}

/* Unpin any unreferenced monitors.  They can never be */
/* inflated or synchronized on again. */
static void
CVMobjMonitorUnpinUnreferenced(CVMExecEnv *ee)
{
    int i = 0;
    while (i < ee->objLocksPinnedCount) {
	if (ee->objLocksPinned[i]->obj == NULL) {
#if 0
	    /* There should be a way to determine if the monitor is on */
	    /* the unbound list. */
	    CVMassert(ee->objLocksPinned[i]->state == CVM_OBJMON_UNBOUND);
#endif
	    ee->objLocksPinned[i] =
		ee->objLocksPinned[--ee->objLocksPinnedCount];
	    CVMtraceMisc(("Unpinned unreferenced monitor\n"));
	    continue;
	}
	++i;
    }
}

static CVMObjMonitor *
CVMobjectInflate(CVMExecEnv *ee, CVMObjectICell* indirectObj)
{
    CVMObjMonitor *mon, *omon;
    CVMObjMonitor **fromListPtr;
    /* 
     * bits can hold a native pointer (see CVMobjectVariousWord())
     * therefore the type has to be CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr bits;

    /* See if another thread already beat us to inflating the monitor: */
    {
	CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
	if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
	    CVMObjMonitor *mon = CVMobjMonitor(obj);
	    CVMassert(mon->obj == obj);
#ifdef CVM_THREAD_BOOST
	    if (mon->boost) {
	        /* Another thread beat us to inflating this monitor.  If we
		   own it, then we'll need to claim actual ownership of the
		   monitor and unboost our thread priority because we didn't
		   inflate it ourselves.  CVMunboost() will check for monitor
		   ownership before unboosting.  If we don't own it, then
		   CVMunboost() will do nothing. */
		CVMunboost(ee, mon);
	    }
#endif
	    return mon;
	}
    }

    /* %comment l008 */
    /* See notes above on "Synchronization between various Monitor Methods": */
    CVMD_gcSafeExec(ee, {
	CVMsysMutexLock(ee, &CVMglobals.syncLock);
    });

    {
	CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
	bits = obj->hdr.various32;
    }

    /* While acquiring the syncLock above, another thread could have beaten us
       to inflating the monitor.  Check if this is the case: */
    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {

        /* If we're here, the another thread did beat us to it: */
	CVMObjMonitor *mon = (CVMObjMonitor *)CVMhdrBitsPtr(bits);

	CVMassert(mon->state != CVM_OBJMON_FREE);
#ifdef CVM_DEBUG
	CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	CVMassert(mon->obj == CVMID_icellDirect(ee, indirectObj));

#ifdef CVM_THREAD_BOOST
        /* Another thread beat us to inflating this monitor.  If we own it,
	   then we'll need to claim actual ownership of the monitor and
	   unboost our thread priority because we didn't inflate it ourselves.
	   CVMunboost() will check for monitor ownership before unboosting.
	   If we don't own it, then CVMunboost() will do nothing. */
	if (mon->boost) {
	    CVMunboost(ee, mon);
	}
#endif

#ifdef CVM_AGGRESSIVE_REPLENISH
	/* replenish free lists */
	goto done;
#else
	CVMsysMutexUnlock(ee, &CVMglobals.syncLock);

	return mon;
#endif
    }

    /* If we get here, then we know for sure that no other thread has inflated
       the monitor yet.  And only we have the right to inflate it because we
       hold the syncLock. */

    /* In the following, we'll be doing one of the these change of states:
        1. unlocked --> monitor (heavy/deterministic), or
        2. locked (fast) --> monitor (heavy/deterministic)
    */

    /* Get a heavy CVMObjMonitor from the free list: */
    {
	fromListPtr = &ee->objLocksFreeUnlocked;
	mon = *fromListPtr;
        if (mon == NULL) {
            /* Failed because no CVMObjMonitor was available: */
            CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
            return NULL;
        }
        CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == NULL);
    }

    omon = mon;
    CVMassert(mon->state == CVM_OBJMON_FREE);
    mon->state = CVM_OBJMON_BOUND;

    {
	CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
        CVMUint32 reentryCount = 1;
	/*  
	 * obits can hold a native pointer
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr obits;

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

        /* Write CVM_LOCKSTATE_LOCKED to the obj's header bits to lock out any
           fast lock or monitor inflation attempts by any other threads: */
	obits = CVMatomicSwap(&(obj)->hdr.various32, CVM_LOCKSTATE_LOCKED);
        if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_LOCKED) {
            CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(obits);

            /* We need to use a CVMatomicSwap() here because the owner thread
               may already have attained its CVMOwnedMonitor corrently.  Hence,
               the above atomic swap of obj->hdr.various32 doesn't prevent a
               fastReentryTryLock() that's already in progress.  However,
               by doing an atomic swap on the reentry count, we can guarantee
               that we get the last reentryCount of the owner thread before we
               prevent it from doing anymore re-entries by swapping a
               CVM_INVALID_REENTRY_COUNT value into the reentryCount. */

            reentryCount = CVMatomicSwap(&o->count, CVM_INVALID_REENTRY_COUNT);
        }

#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK

        /* Acquire the microlock to lock out any fast lock or monitor inflation
           attempts by any other threads: */
	CVMobjectMicroLock(ee, obj);
        obits = CVMobjectVariousWord(obj);

	if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_LOCKED) {
	    CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(obits);
	    reentryCount = o->count;
#ifdef CVM_DEBUG
	    o->count = CVM_INVALID_REENTRY_COUNT;
#endif
	}
        /* Set the object's various word to a fake CVMOwnedMonitor record that
           is not owned by any thread (i.e. the owner field is NULL) to
           prevent any fast lock or unlock operations on this object while
           we're inflating.  After that, we can unlock the microlock. */
        CVMobjectVariousWord(obj) = (CVMAddr)(&rendezvousOwnedRec) |
                                     CVM_LOCKSTATE_LOCKED;
        CVMobjectMicroUnlock(ee, obj);
#endif

        /* If there is already a fast lock on the object, ... */
	if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_LOCKED) {
	    CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(obits);
	    CVMExecEnv *owner = o->owner;

#ifdef CVM_DEBUG
	    CVMassert(o->magic == CVM_OWNEDMON_MAGIC);
#endif
	    CVMassert(o->type == CVM_OWNEDMON_FAST);
	    if (owner != ee) {
		/* set mutex ownership */
                CVMprofiledMonitorGetCount(&mon->mon) = reentryCount;
                CVMprofiledMonitorGetOwner(&mon->mon) = owner;
#ifdef CVM_THREAD_BOOST
#ifdef CVM_DEBUG
		mon->boostThread = owner;
#endif
		/* Request priority boosting on owner thread so that it can
		   claim actual ownership of this monitor: */
		CVMassert(!mon->boost);
		mon->boost = CVM_TRUE;
#else /* !CVM_THREAD_BOOST */
                CVMprofiledMonitorSetMutexOwner(&mon->mon, ee, owner);
#ifdef CVM_DEBUG_ASSERTS
		{
                    CVMBool success = CVMprofiledMonitorTryEnter(&mon->mon, ee);
		    CVMassert(!success);
		}
#endif /* CVM_DEBUG */
#endif /* !CVM_THREAD_BOOST */
	    } else {
                CVMBool success = CVMprofiledMonitorTryEnter(&mon->mon, ee);
		CVMassert(success); (void) success;
                CVMassert(CVMprofiledMonitorGetCount(&mon->mon) == 1);
                CVMprofiledMonitorGetCount(&mon->mon) = reentryCount;
	    }

            /* Attach the CVMObjMonitor to the CVMOwnedMonitor: */
	    mon->bits = o->u.fast.bits;
	    mon->obj = obj;
#ifdef CVM_DEBUG
	    mon->owner = o;
#endif
	    o->type = CVM_OWNEDMON_HEAVY;
            o->object = mon->obj;
	    o->u.heavy.mon = mon;
	    mon->state = CVM_OBJMON_OWNED;

            CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == o->owner);

        /* Else if the object has not been locked yet, ... */
	} else if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_UNLOCKED) {
            CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == NULL);
	    mon->bits = obits;
	    mon->obj = obj;
#ifdef CVM_DEBUG
	    mon->owner = NULL;
#endif

	} else /* CVM_LOCKSTATE_MONITOR */ {
            CVMassert(CVM_FALSE); /* We should never get here. */
        }

#else /* !FAST */

#ifdef CVM_DEBUG
	CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	{
	    /* 
	     * Scalar variables intended to hold pointer values
	     * have to be of the type CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms.
	     */
	    CVMAddr bits = CVMobjectVariousWord(obj);
	    CVMassert(CVMhdrBitsSync(bits) == CVM_LOCKSTATE_UNLOCKED);
	    mon->bits = bits;
	}
	mon->obj = obj;
#endif
        CVMobjMonitorSet(obj, mon, CVM_LOCKSTATE_MONITOR);
    }

    /* From a free list or object header? */
    if (omon == mon) {
	*fromListPtr = mon->next;
	mon->next = CVMglobals.objLocksBound;
	CVMglobals.objLocksBound = mon;
    }

#ifdef CVM_AGGRESSIVE_REPLENISH
done:
#endif

    CVMassert(CVMobjMonitor(CVMID_icellDirect(ee, indirectObj)) == mon);
    CVMassert(mon->state != CVM_OBJMON_FREE);

    /* This is like a reference count, needed if count==0 */
    ee->objLockCurrent = mon;

    /* And another sort of reference count */
    if (ee->threadExiting) {
	CVMobjMonitorPin(ee, mon);
	CVMtraceExec(CVM_DEBUGFLAG(TRACE_MISC), {
	    CVMD_gcSafeExec(ee, {
		CVMobjectGetHashSafe(ee, indirectObj);
		CVMconsolePrintf("Inflating %I\n", indirectObj);
	    });
	});
    }

    /* %comment l010 */
    CVMD_gcSafeExec(ee, {
	CVMreplenishFreeListsNonBlocking(ee);
	CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
	CVMreplenishFreeListsBlocking(ee);
    });

    ee->objLockCurrent = NULL;

    return mon;
}

CVMObjMonitor *
CVMobjectInflatePermanently(CVMExecEnv *ee, CVMObjectICell* indirectObj)
{
    CVMObjMonitor* mon = CVMobjectInflate(ee, indirectObj);
    if (mon != NULL) {
	mon->sticky = CVM_TRUE;
    }
    return mon;
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if we're out of memory, and
            hence not able to lock the monitor. */
static CVMBool
CVMfastLock(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMObjMonitor *mon;

    CVMassert(CVMD_isgcUnsafe(ee));

    /* %comment l011 */
    CVMtraceFastLock(("fastLock(%x,%x)\n", ee,
	CVMID_icellDirect(ee, indirectObj)));

    if (ee->objLocksFreeOwned == NULL) {
	/* becomes safe */
        CVMreplenishLockRecordUnsafe(ee);
        if (ee->objLocksFreeOwned == NULL) {
            /* out of memory */
            return CVM_FALSE;
        }

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
        /* NOTE: If we got here, it could be because this thread tried to
           acquire more locks than there are CVMOwnedMonitor records in the
           ee's free list to satisfy the attempts.  Now that we've replenished
           the supply of CVMOwnedMonitors, let's try that TryLock() again.
           Doing this may actually save us the time and resources for inflating
           the monitor.

           Also note that if we had entered CVMfastLock() because the object
           had already been locked by another thread (and not because of a
           scarcity of CVMOwnedMonitors, then we would have bypassed this whole
           attempt at the first 'if' above. */
        {
            CVMObject *directObj = CVMID_icellDirect(ee, indirectObj);
            if (CVMobjectTryLock(ee, directObj) == CVM_TRUE) {
                return CVM_TRUE;
            }
        }
#endif /* CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE */
    }

    mon = CVMobjectInflate(ee, indirectObj);
    if (mon == NULL) {
	/* out of memory */
	return CVM_FALSE;
    }

    {
	CVMassert(mon->state != CVM_OBJMON_FREE);

        /* %comment l012 */
        CVMmonEnter(ee, mon);

	CVMassert(mon->state != CVM_OBJMON_FREE);
#ifdef CVM_DEBUG
	CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	CVMassert(mon->obj == CVMID_icellDirect(ee, indirectObj));

        /* We could have gotten here because of several possibilities:

           1. This thread has entered this monitor several times using the fast
              lock mechanism.  Thereafter, contention occurs which triggers
              inflation of the monitor.  The inflation code would have attached
              the inflated CVMObjMonitor to the CVMOwnedMonitor that was
              previously tracking the reentry count for this monitor.  In this
              case, we can expect the reentry count to not be 1.  In the least,
              it will be 2 (once from the original fast lock, and once from the
              CVMprofiledMonitorEnterUnsafe() above) if not more depending on
              how many fast lock reentries have taken place before we got here.

           2. While attempting to do a fast lock for the first time on this
              object, another thread was calling CVMobjectSetHashBitsIfNeeded()
              on the same object.  This caused the fast lock to fail and ended
              up here even though this thread didn't already have a lock on this
              object.  In this case, the CVMprofiledMonitorEnterUnsafe() above
              would be the first lock on the monitor, and a CVMOwnedMonitor
              record hasn't been attached to the CVMObjMonitor yet.  This is
              why we need to attach the CVMObjMonitor to a CVMOwnedMonitor if
              the reentry count is 1.
        */
        if (CVMprofiledMonitorGetCount(&mon->mon) == 1) {
            CVMmonitorAttachObjMonitor2OwnedMonitor(ee, mon);
        } else {
            CVMassert(mon->state == CVM_OBJMON_OWNED);
        }
    }
#ifdef CVM_JVMTI
    if (CVMjvmtiIsInDebugMode()) {
	CVMjvmtiAddLockInfo(ee, mon, NULL, CVM_TRUE);
    }
#endif
    return CVM_TRUE;
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if the current thread if not
            the owner of the specified lock, and hence not able to unlock the
            monitor. */
/* Note: This function is ONLY to be executed under GC unsafe conditions,
         hence we don't have to worry about GC compacting memory and
         changing the value of pointers. */
static CVMBool
CVMprivateUnlock(CVMExecEnv *ee, CVMObjMonitor *mon)
{
    CVMUint32 count = CVMprofiledMonitorGetCount(&mon->mon);
    CVMassert(mon->state != CVM_OBJMON_FREE);
#ifdef CVM_DEBUG
    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
    if (ee->objLocksOwned == NULL) {
        /* Fail because this ee (i.e thread) does not own the lock on this
           monitor.  Hence it is not allowed to unlock it. */
	return CVM_FALSE;
    }

    if (count > 0 && CVMprofiledMonitorGetOwner(&mon->mon) == ee) {
	CVMassert(mon->state == CVM_OBJMON_OWNED);
#ifdef CVM_JVMTI
	if (CVMjvmtiIsInDebugMode()) {
	    CVMjvmtiRemoveLockInfo(ee, mon, NULL);
	}
#endif
	if (count == 1) {
            CVMOwnedMonitor **prev = &ee->objLocksOwned;
            CVMOwnedMonitor *o = ee->objLocksOwned;

            /* Assume the most frequent case i.e. the last monitor locked is
               the one being unlocked and the appropriate CVMOwnedMonitor is
               the first on the owned list.  If this turns out to be untrue,
               then we'll have to search for the appropriate CVMOwnedMonitor:
            */
            if (o->u.heavy.mon != mon) {
                /* We use a do-while loop here instead of a while loop because
                   we've already eliminated the first element in the list as a
                   candidate and will not have to check it again. */
                do {
                    prev = &o->next;
                    o = o->next;
                } while ((o != NULL) && (o->u.heavy.mon != mon));

                /* If we've successfully found the appropriate CVMOwnedMonitor,
                   the ptr should not be NULL: */
                CVMassert (o != NULL);
            }
            /* By now, we should have attained the appropriate CVMOwnedMonitor
               as pointed to by the o pointer. */

	    CVMassert(o->owner == ee);
	    CVMassert(o->type == CVM_OWNEDMON_HEAVY);

            /* NOTE: The operation of moving the released CVMOwnedMonitor from
                the owned list to the free list is thread safe because the
                possible thread contentions are handled as follows:
                1. GC, who may want to compact memory, is held off because
                   this function is GC unsafe.
                2. Other threads who may want to inflate the monitor won't be
                   contending because the monitor has already been inflated. */

            mon->state = CVM_OBJMON_BOUND;

            /* Remove the released CVMOwnedMonitor from the ee's owned list: */
            *prev = o->next;

            /* Detach the CVMOwnedMonitor from the CVMObjMonitor: */
	    o->type = CVM_OWNEDMON_FAST;
#ifdef CVM_DEBUG
	    o->state = CVM_OWNEDMON_FREE;
	    o->u.fast.bits = 0;
	    o->object = NULL;
	    mon->owner = NULL;
#endif
            /* Add the released CVMOwnedMonitor back to the ee's free list: */
	    o->next = ee->objLocksFreeOwned;
	    ee->objLocksFreeOwned = o;
	}

        CVMmonExit(ee, mon);
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
/* 
 * Simple Sync helper function for unlocking in Simple Sync methods
 * when there is contention on the lock. It is only needed for
 * CVM_FASTLOCK_ATOMICOPS since CVM_FASTLOCK_MICROLOCK will never
 * allow for contention on the unlcok in Simple Sync methods.
 */
extern void
CVMsimpleSyncUnlock(CVMExecEnv *ee, CVMObject* obj)
{
    CVMObjMonitor *mon;

#if 0
    CVMconsolePrintf("CVMsimpleSyncUnlock ee=0x%x threadID=%-7d obj=0x%x\n",
		     ee, ee->threadID, obj);
    CVMdebugPrintf(("sync mb: %C.%M\n",
		   CVMmbClassBlock(ee->currentSimpleSyncMB),
		    ee->currentSimpleSyncMB));
    CVMdebugPrintf(("caller:  %C.%M\n",
		   CVMmbClassBlock(ee->currentMB),
		    ee->currentMB));
#endif

    CVMassert(CVMID_icellIsNull(CVMsyncICell(ee)));
    CVMID_icellSetDirect(ee, CVMsyncICell(ee), obj);
    CVMD_gcSafeExec(ee, {
	/* CVMgcRunGC(ee); */ /* for debugging */
	/* Wait for any inflation on this monitor to complete: */
        CVMsysMutexLock(ee, &CVMglobals.syncLock);
    });
    /* refresh obj since we become gcSafe */
    obj = CVMID_icellDirect(ee, CVMsyncICell(ee));
    CVMID_icellSetNull(CVMsyncICell(ee));
	
    /* The monitor must have been inflated because we'll only
       get here if there was contention on this simple lock: */
    CVMassert(CVMhdrBitsSync(CVMobjectVariousWord(obj))
	      == CVM_LOCKSTATE_MONITOR);

    mon = CVMobjMonitor(obj);

#ifdef CVM_DEBUG
    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
    CVMassert(mon->state == CVM_OBJMON_OWNED);
    CVMassert(CVMprofiledMonitorGetCount(&mon->mon) == 1);
    CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == ee);
    {
	CVMOwnedMonitor *o = &ee->simpleSyncReservedOwnedMonitor;
#ifdef CVM_DEBUG
	CVMassert(o == mon->owner);
#endif
	CVMassert(o->u.heavy.mon == mon);
	CVMassert(o->owner == ee);
	CVMassert(o->type == CVM_OWNEDMON_HEAVY);
#ifdef CVM_DEBUG
	CVMassert(o->state == CVM_OWNEDMON_OWNED);
#endif
	CVMassert(o->count == CVM_INVALID_REENTRY_COUNT);

	mon->state = CVM_OBJMON_BOUND;

	/* Detach the CVMOwnedMonitor from the CVMObjMonitor: */
	o->type = CVM_OWNEDMON_FAST;
	o->count = 1;
#ifdef CVM_DEBUG
#if 0  /* The state should always remain CVM_OWNEDMON_OWNED */
	o->state = CVM_OWNEDMON_FREE;
#endif
	o->u.fast.bits = 0;
	o->object = NULL;
	mon->owner = NULL;
#endif
    }
    
    CVMmonExit(ee, mon);

    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
}
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/* Returns: CVM_TRUE if successful.  CVM_FALSE if the current thread if not
            the owner of the specified lock, and hence not able to unlock the
            monitor. */
/* Note: This function is ONLY to be executed under GC unsafe conditions,
         hence we don't have to worry about GC compacting memory and
         changing the value of pointers. */
CVMBool
CVMfastTryUnlock(CVMExecEnv* ee, CVMObject* obj)
{
    CVMtraceFastLock(("fastUnlock(%x,%x)\n", ee, obj));

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE

    {
        CVMOwnedMonitor *o;
        CVMBool result = CVM_FALSE;
	/* 
	 * obits can hold a native pointer (see CVMobjectVariousWord())
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr obits;

        /* If we're in this function, then the object should already have a
           CVMOwnedMonitor.  Attempt to get the CVMOwnedMonitor from the
           object's header bits: */
        obits = CVMobjectVariousWord(obj);
        if (CVMhdrBitsSync(obits) != CVM_LOCKSTATE_LOCKED) {
            /* If we're here, we have failed because someone has changed the
               state of the object into something other than
               CVM_LOCKSTATE_LOCKED.  Hence, we aren't able to get the
               CVMOwnedMonitor needed: */
            goto fast_failed;
        }
        /* Get the CVMOwnedMonitor: */
        o = (CVMOwnedMonitor *) CVMhdrBitsPtr(obits);
        if (o == NULL) {
            /* If we're here, then there must be another thread which is
               currently inflating the monitor.  Hence, we shouldn't do any
               "fast" operations.  Fail and go do the heavy weight stuff: */
            goto fast_failed;
        }
        if (o->owner != ee) {
            /* If we're here, then fail because this thread does not have the
               right to unlock this monitor: */
            goto fast_failed;
        }
#if 0
	/* Detect bug 4955461 in optimized builds */
	if (o->object != obj) {
	    CVMpanic("obj mismatch");
	}
#endif

        /* By now, we should have attained the appropriate CVMOwnedMonitor as
           pointed to by the o pointer. */

#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        {
	    /* 
	     * expectedOldCount and newCount are used in
	     * conjunction with CVMatomicCompareAndSwap() and can 
	     * therefore hold a native pointer.
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
            CVMAddr expectedOldCount = o->count;
            CVMAddr newCount;

            /* If the count is CVM_INVALID_REENTRY_COUNT after we've acquired
               a fast lock, then it must mean that another thread is currently
               inflating this monitor: */
            if (expectedOldCount == CVM_INVALID_REENTRY_COUNT) {
                goto fast_failed;
            }
            newCount = expectedOldCount - 1;

            /* If the reentry count is going to reach 0, then we should remove
               the fast lock entirely: */
            if (newCount == 0) {
		/* 
		 * nbits and obits0 are used in
		 * conjunction with CVMatomicCompareAndSwap() and can 
		 * therefore hold a native pointer.
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
                CVMAddr obits0;
                CVMAddr nbits = o->u.fast.bits;
                obits0 = CVMatomicCompareAndSwap(&obj->hdr.various32,
                                                 nbits, obits);
                /* NOTE that we could get here and have a contention with
                   either CVMobjectInflate() or CVMobjectSetHashBitsIfNeeded().
                   If contention occurred, the atomicCompareAndSwap will fail
                   and we abort.  The reentry count stays at 1 because we never
                   modified it.  This is as we would want it.
                */
                result = (obits0 == obits);
            } else {
                /* If we get here, then the new reentry count is not 0.  Hence,
                   we have to try to reflect that in the CVMOwnedMonitor: */

		/* 
		 * oldCount is used in
		 * conjunction with CVMatomicCompareAndSwap() and can 
		 * therefore hold a native pointer.
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
                CVMAddr oldCount;
                oldCount = CVMatomicCompareAndSwap(&o->count, newCount,
                                                   expectedOldCount);
                /* We either succeed in decrementing the count or not.  If not,
                   then the CompareAndSwap will fail which means that another
                   thread must be trying to inflate this monitor: */
                if (oldCount == expectedOldCount) {
#ifdef CVM_JVMTI
		    if (CVMjvmtiIsInDebugMode()) {
			CVMjvmtiRemoveLockInfo(ee, NULL, o);
		    }
#endif
                    return CVM_TRUE;
                }
                goto fast_failed;
            }
        }

#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK

	CVMobjectMicroLock(ee, obj);
	{
	    /* 
	     * obits0 must be able to hold a native
	     * pointer because CVMobjectVariousWord(obj)
	     * returns CVMAddr
	     * therefore the type has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    CVMAddr obits0;
	    obits0 = CVMobjectVariousWord(obj);

            /* obits0 must be equal to obits to ensure that the monitor hasn't
               been inflated while we were acquiring the microlock: */
	    if (obits0 == obits) {

		o->count--;

		CVMassert(o->count < CVM_MAX_REENTRY_COUNT);

		/* If the reentry count has reached 0, then we should remove the
		   fast lock entirely: */
		if (o->count == 0) {
		    obj->hdr.various32 = o->u.fast.bits;
		    result = CVM_TRUE;
		} else {
		    CVMobjectMicroUnlock(ee, obj);
#ifdef CVM_JVMTI
		    if (CVMjvmtiIsInDebugMode()) {
			CVMjvmtiRemoveLockInfo(ee, NULL, o);
		    }
#endif
		    return CVM_TRUE;
		}
	    } else {
		result = CVM_FALSE;
	    }
	}
	CVMobjectMicroUnlock(ee, obj);
#else
#error
#endif

        /* If we've failed at this point, it would be because the monitor was
           inflated before we got to changing out the value of
           obj->hdr.various32 above.  No problem with that, just let the heavy
           TryUnlock() take care of it below through CVMprivateUnlock().
           If we're successful, we won't have to worry about this monitor
           being inflated because the CVMOwnedMonitor pointer is no longer
           attainable from the object being unlocked.
        */
	if (result) {

            CVMOwnedMonitor **prev;
            CVMOwnedMonitor *current;

            /* Check to ensure we own some locks: */
            current = ee->objLocksOwned;
            CVMassert(current != NULL);

            /* Assume the most frequent case i.e. the last monitor locked is the
               one being unlocked and the appropriate CVMOwnedMonitor is the first
               on the owned list.  If this turns out to be untrue, then we'll have
               to search for the appropriate CVMOwnedMonitor:
            */
            prev = &ee->objLocksOwned;
            if (current->object != obj) {
                /* We use a do-while loop here instead of a while loop because
                   we've already eliminated the first element in the list as a
                   candidate and will not have to check it again. */
                do {
                    prev = &current->next;
                    current = current->next;
                } while ((current != NULL) && (current->object != obj));

                /* If we've successfully found the appropriate CVMOwnedMonitor,
                   the ptr should not be NULL: */
                CVMassert(current != NULL);
            }

            /* NOTE: The operation of moving the released CVMOwnedMonitor from
                the owned list to the free list is thread safe because the
                possible thread contentions are handled as follows:
                1. GC, who may want to compact memory, is held off because
                   this function is GC unsafe.
                2. Other threads who may want to inflate the monitor won't be
                   contending because they won't attempt to inflate a monitor
                   that has already been unlocked (hence, they can't get to
                   the CVMOwnedMonitor pointer we're operating on). */

	    /* Remove the released CVMOwnedMonitor record from the owned list: */
	    *prev = o->next;

	    CVMassert(o->type == CVM_OWNEDMON_FAST);
#ifdef CVM_DEBUG
	    CVMassert(o->state == CVM_OWNEDMON_OWNED);
	    /* recycle mon */
	    o->state = CVM_OWNEDMON_FREE;
	    o->u.fast.bits = 0;
	    o->object = NULL;
#endif
#ifdef CVM_JVMTI
	    if (CVMjvmtiIsInDebugMode()) {
		CVMjvmtiRemoveLockInfo(ee, NULL, o);
	    }
#endif
            /* Add the released CVMOwnedMonitor record back on to the free list: */
	    o->next = ee->objLocksFreeOwned;
	    ee->objLocksFreeOwned = o;
	    CVMassert(o->owner == ee);
	    return CVM_TRUE;
	}
    }

fast_failed:

#endif /* CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE */

    if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
	CVMObjMonitor *mon = CVMobjMonitor(obj);

#ifdef CVM_THREAD_BOOST
	/* Unboosting might cause us to become safe */
	if (mon->boost) {
	    return CVM_FALSE;
	}
#endif
	return CVMprivateUnlock(ee, mon);
    }
    /* See comments in CVMfastUnlock() for a reason why CVMfastTryUnlock()
       could fail: */
    return CVM_FALSE;
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if the current thread if not
            the owner of the specified lock, and hence not able to unlock the
            monitor. */
/* NOTE: Called while GC unsafe.  Can become GC safe. */
static CVMBool
CVMfastUnlock(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMBool success;
    /* %comment l014 */

    /* If the monitor has already been inflated by another thread, just unlock
       it: */
    {
        CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
        if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
            return CVMprivateUnlock(ee, CVMobjMonitor(obj));
        }
    }

    /* See notes above on "Synchronization between various Monitor Methods": */
    CVMD_gcSafeExec(ee, {
        CVMsysMutexLock(ee, &CVMglobals.syncLock);
    });

    /* The only thing that would cause TryUnlock() to fail are as follows:
        1. The current thread is not the owner of the monitor.
        2. The process of TryUnlock() is interrupted by the inflation of this
           monitor or the setting of the object hash value by another thread.

       While the above are all valid reasons for why TryUnlock() to fail, the
       same does not hold true for Unlock().  The only valid reason for
       Unlock() to fail is reason 1.

       Acquiring CVMglobals.syncLock ensures that we cannot be interrupted by
       CVMobjectInflate() or CVMobjectSetHashBitsIfNeeded().  This eliminates
       reason 2 from interfering with TryUnlock().  Hence, after acquiring
       CVMglobals.syncLock, we can call upon TryUnlock() to do the work of
       Unlock().
    */
    success = CVMobjectTryUnlock(ee, CVMID_icellDirect(ee, indirectObj));
#ifdef CVM_THREAD_BOOST
    if (!success) {
        /* NOTE: We could have failed the TryUnlock() because the inflated
	   monitor needed to be unboosted.  Since this happens while GC safe,
           we can't do it in TryUnlock() but we can do it here in Unlock(). */
	CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
        if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
	    CVMObjMonitor *mon = CVMobjMonitor(obj);
	    /* may become gc-safe */
            success = CVMprivateUnlock(ee, mon);
	    /* obj is invalid */
        }
    }
#endif

    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);

    return success;
}

static CVMObjMonitor *
CVMprivateGetMonitor(CVMExecEnv* ee, CVMObjectICell* indirectObj, CVMBool fast)
{
    CVMObjMonitor *mon;
    {
	/*  
	 * bits can hold a native pointer (see CVMobjectVariousWord())
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr bits;
	{
	    CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
	    bits = obj->hdr.various32;
	}
	if (fast) {
	    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_UNLOCKED) {
		CVMthrowIllegalMonitorStateException(ee,
		    "no monitor associated with object");
		return NULL;
	    } else {
		mon = CVMobjectInflate(ee, indirectObj);
                if (mon == NULL) {
                    CVMthrowOutOfMemoryError(ee, NULL);
                    return NULL;
                }
	    }
	} else {
	    CVMassert(CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR);
	    mon = (CVMObjMonitor *)CVMhdrBitsPtr(bits);
#ifdef CVM_DEBUG
	    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	}
    }

    CVMassert(mon->obj == CVMID_icellDirect(ee, indirectObj));

    if (CVMobjMonitorOwner(mon) != ee) {
	CVMthrowIllegalMonitorStateException(ee,
	    "thread does not own monitor");
	return NULL;
    }
#ifdef CVM_THREAD_BOOST
    if (mon->boost) {
        /* Another thread inflated this monitor for us.  We own this monitor
	   and need to claim actual ownership of the it and unboost our thread
	   priority because we didn't inflate it ourselves. */
	CVMunboost(ee, mon);
	CVMassert(!mon->boost);
    }
#endif
    return mon;
}

/* These should be inlined by the compiler */
static CVMBool
CVMprivateWait(CVMExecEnv* ee, CVMObjectICell* indirectObj,
    CVMJavaLong millis, CVMBool fast)
{
    CVMWaitStatus waitStatus;
    CVMObjMonitor *mon = CVMprivateGetMonitor(ee, indirectObj, fast);

    if (mon == NULL) {
        /* CVM_WAIT_NOT_OWNER */
        CVMassert (CVMlocalExceptionOccurred(ee));
	return CVM_FALSE;
    }

    CVMassert(mon->state == CVM_OBJMON_OWNED);
#ifdef CVM_DEBUG
    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
    {
#ifdef CVM_DEBUG
	CVMOwnedMonitor *o = mon->owner;
	CVMassert(mon->owner->type == CVM_OWNEDMON_HEAVY);
	mon->owner = NULL;
#endif
	mon->state = CVM_OBJMON_BOUND;

	/* NOTE: the owned list is not modified */
	/* We don't allow the monitor to be recycled here */
	ee->objLockCurrent = mon;

#ifdef CVM_THREAD_BOOST
	CVMassert(!mon->boost);
#endif

	/* %comment l015 */
	CVMD_gcSafeExec(ee, {
            waitStatus = CVMprofiledMonitorWait(&mon->mon, ee, millis);
	});

	ee->objLockCurrent = NULL;
#ifdef CVM_DEBUG
	mon->owner = o;
	CVMassert(mon->owner->type == CVM_OWNEDMON_HEAVY);
#endif

	CVMassert(mon->state == CVM_OBJMON_BOUND);
#ifdef CVM_DEBUG
	CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	CVMassert(CVMobjMonitorOwner(mon) == ee);
	mon->state = CVM_OBJMON_OWNED;
	if (waitStatus != CVM_WAIT_OK) {
	    CVMassert(waitStatus == CVM_WAIT_INTERRUPTED);
	    CVMthrowInterruptedException(ee, "operation interrupted");
	    /* Clear interrupted state */
	    ee->threadState &= ~CVM_THREAD_INTERRUPTED;
	}
    }

    return waitStatus == CVM_WAIT_OK;
}

static CVMBool
CVMprivateNotify(CVMExecEnv* ee, CVMObjectICell* indirectObj, CVMBool fast)
{
    CVMObjMonitor *mon = CVMprivateGetMonitor(ee, indirectObj, fast);
    if (mon != NULL) {
	CVMcondvarNotify(&mon->mon._super.condvar);
	return CVM_TRUE;
    } else {
        CVMassert (CVMlocalExceptionOccurred(ee));
	return CVM_FALSE;
    }
}

static CVMBool
CVMprivateNotifyAll(CVMExecEnv* ee, CVMObjectICell* indirectObj, CVMBool fast)
{
    CVMObjMonitor *mon = CVMprivateGetMonitor(ee, indirectObj, fast);
    if (mon != NULL) {
	CVMcondvarNotifyAll(&mon->mon._super.condvar);
	return CVM_TRUE;
    } else {
        CVMassert (CVMlocalExceptionOccurred(ee));
	return CVM_FALSE;
    }
}


static CVMBool
CVMfastWait(CVMExecEnv* ee, CVMObjectICell* indirectObj, CVMJavaLong millis)
{
    CVMtraceFastLock(("fastWait(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateWait(ee, indirectObj, millis, CVM_TRUE);
}

static CVMBool
CVMfastNotify(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    /* %comment l016 */
    CVMtraceFastLock(("fastNotify(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateNotify(ee, indirectObj, CVM_TRUE);
}

static CVMBool
CVMfastNotifyAll(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    /* %comment l017 */
    CVMtraceFastLock(("fastNotifyAll(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateNotifyAll(ee, indirectObj, CVM_TRUE);
}


/*
 * The deterministic lockers
 * Object header should have forwarding pointer to
 * monitor structure, with "evacuated" bits set
 */
static CVMBool
CVMdetTryLock(CVMExecEnv* ee, CVMObject* obj)
{
    if (ee->objLocksFreeOwned != NULL) {
	CVMObjMonitor *mon = CVMobjMonitor(obj);

#ifdef CVM_JVMTI
	if (CVMjvmtiIsInDebugMode()) {
	    /* just check for an available lockinfo */
	    if (!CVMjvmtiCheckLockInfo(ee)) {
		return CVM_FALSE;
	    }
	}
#endif

	CVMtraceDetLock(("detTryLock(%x,%x)\n", ee, obj));
	CVMassert(CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR);

        if (!CVMmonTryEnter(ee, mon)) {
	    return CVM_FALSE;
	}

        if (CVMprofiledMonitorGetCount(&mon->mon) == 1) {
            CVMmonitorAttachObjMonitor2OwnedMonitor(ee, mon);
	}
#ifdef CVM_JVMTI
	if (CVMjvmtiIsInDebugMode()) {
	    CVMjvmtiAddLockInfo(ee, mon, NULL, CVM_FALSE);
	}
#endif
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if we're out of memory, and
            hence not able to lock the monitor. */
static CVMBool
CVMdetLock(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMObjMonitor *mon;

    CVMtraceDetLock(("detLock(%x,%x)\n", ee,
	CVMID_icellDirect(ee, indirectObj)));

    if (ee->objLocksFreeOwned == NULL) {
	/*
	 * The following call becomes GC-safe, so it is possible that
	 * when we return, the monitor is no longer allocated.  Therefore,
         * after replenishing, we redispatch through the vector.
	 */
        CVMreplenishLockRecordUnsafe(ee);
	if (ee->objLocksFreeOwned == NULL) {
	    /* out of memory */
	    return CVM_FALSE;
	}

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
        /* For the same reason explained in the comments in CVMfastLock(), we
           should first give TryLock() a chance to potentially satisfy this
           lock request with a fast lock.  Doing this lessens the probability
           of needing a heavy lock: */
        {
            CVMObject *directObj = CVMID_icellDirect(ee, indirectObj);
            if (CVMobjectTryLock(ee, directObj) == CVM_TRUE) {
                return CVM_TRUE;
            }
        }
#endif /* CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE */

        /* We can still fail with the TryLock() because other threads could
           have acquired the lock first.  Hence, dispatch through
           CVMobjectLock() to do a lock.  We know that this isn't going to
           cause an endless recursion because we've already replenished the
           supply of CVMOwnedMonitors above.  Hence, we won't get back to this
           spot again for the same lock attempt: */
	return CVMobjectLock(ee, indirectObj);
    }

    {
	CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
	mon = CVMobjMonitor(obj);
	CVMtraceDetLock(("detLock(%x,%x)\n", ee, obj));
	CVMassert(CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR);
	CVMassert(mon->obj == obj);
    }

    CVMassert(mon->state != CVM_OBJMON_FREE);
#ifdef CVM_DEBUG
    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif

    /* %comment l018 */
    CVMmonEnter(ee, mon);

#ifdef CVM_DEBUG
    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
    if (CVMprofiledMonitorGetCount(&mon->mon) == 1) {
        CVMmonitorAttachObjMonitor2OwnedMonitor(ee, mon);
    } else {
	CVMassert(mon->state == CVM_OBJMON_OWNED);
    }
#ifdef CVM_JVMTI
    if (CVMjvmtiIsInDebugMode()) {
	CVMjvmtiAddLockInfo(ee, mon, NULL, CVM_TRUE);
    }
#endif
    return CVM_TRUE;
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if the current thread if not
            the owner of the specified lock, and hence not able to unlock the
            monitor. */
static CVMBool
CVMdetTryUnlock(CVMExecEnv* ee, CVMObject* obj)
{
    CVMtraceDetLock(("detTryUnlock(%x,%x)\n", ee, obj));

    CVMassert(CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR);

    {
	CVMObjMonitor *mon = CVMobjMonitor(obj);

#ifdef CVM_THREAD_BOOST
	/* Unboosting might cause us to become safe */
	if (mon->boost) {
	    return CVM_FALSE;
	}
#endif
	return CVMprivateUnlock(ee, mon);
    }
}

/* Returns: CVM_TRUE if successful.  CVM_FALSE if the current thread if not
            the owner of the specified lock, and hence not able to unlock the
            monitor. */
static CVMBool
CVMdetUnlock(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
    CVMtraceDetLock(("detUnlock(%x,%x)\n", ee, obj));

    CVMassert(CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR);

    /* %comment l019 */
    return CVMprivateUnlock(ee, CVMobjMonitor(obj));
}

static CVMBool
CVMdetWait(CVMExecEnv* ee, CVMObjectICell* indirectObj, CVMJavaLong millis)
{
    CVMtraceDetLock(("detWait(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateWait(ee, indirectObj, millis, CVM_FALSE);
}

static CVMBool
CVMdetNotify(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMtraceDetLock(("detNotify(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateNotify(ee, indirectObj, CVM_FALSE);
}

static CVMBool
CVMdetNotifyAll(CVMExecEnv* ee, CVMObjectICell* indirectObj)
{
    CVMtraceDetLock(("detNotifyAll(%x,%x)\n", ee, 
	CVMID_icellDirect(ee, indirectObj)));
    return CVMprivateNotifyAll(ee, indirectObj, CVM_FALSE);
}

/* Purpose: Constructor of the CVMObjMonitor. */
/* NOTE: This function can only be called while GC safe. */
CVMBool
CVMobjMonitorInit(CVMExecEnv *ee, CVMObjMonitor *m, CVMExecEnv *owner,
		  CVMUint32 count)
{
    CVMassert(CVMD_isgcSafe(ee));
    m->sticky = CVM_FALSE;
    m->next = NULL;
    m->obj = NULL;
    m->state = CVM_OBJMON_FREE;
#ifdef CVM_DEBUG
    m->magic = CVM_OBJMON_MAGIC;
    m->owner = NULL;
#endif
#ifdef CVM_THREAD_BOOST
    m->boost = CVM_FALSE;
#ifdef CVM_DEBUG
    m->boostThread = NULL;
#endif
    if (!CVMthreadBoostInit(&m->boostInfo)) {
        return CVM_FALSE;
    }
#endif
    if (CVMprofiledMonitorInit(&m->mon, owner, count,
	CVM_LOCKTYPE_OBJ_MONITOR))
    {
#ifdef CVM_JVMPI
        CVMProfiledMonitor **firstPtr = &CVMglobals.objMonitorList;
        CVMProfiledMonitor *mon = CVMcastObjMonitor2ProfiledMonitor(m);
        CVMsysMutexLock(ee, &CVMglobals.jvmpiSyncLock);
        mon->next = *firstPtr;
        mon->previousPtr = firstPtr;
        if (*firstPtr != NULL) {
            (*firstPtr)->previousPtr = &mon->next;
        }
        *firstPtr = mon;
        CVMsysMutexUnlock(ee, &CVMglobals.jvmpiSyncLock);
#endif
	return CVM_TRUE;
    }
    /* profiledMonitor init failed */
#ifdef CVM_THREAD_BOOST
    CVMthreadBoostDestroy(&m->boostInfo);
#endif
    return CVM_FALSE;
}

/* Purpose: Destructor of CVMObjMonitor. */
/* NOTE: This function should only be called while GC safe. */
void CVMobjMonitorDestroy(CVMObjMonitor *self)
{
    CVMProfiledMonitor *mon = CVMcastObjMonitor2ProfiledMonitor(self);
#ifdef CVM_JVMPI
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));

    CVMsysMutexLock(ee, &CVMglobals.jvmpiSyncLock);
    *mon->previousPtr = mon->next;
    if (mon->next != NULL) {
        mon->next->previousPtr = mon->previousPtr;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.jvmpiSyncLock);
#endif
#ifdef CVM_THREAD_BOOST
    CVMthreadBoostDestroy(&self->boostInfo);
#endif
    CVMprofiledMonitorDestroy(mon);
}

static void
CVMownedMonitorInit(CVMOwnedMonitor *m, CVMExecEnv *owner)
{
    m->next = NULL;
    m->owner = owner;
    m->type = CVM_OWNEDMON_FAST;
    m->object = NULL;
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
    m->count = 0;
#endif
#ifdef CVM_DEBUG
    m->state = CVM_OWNEDMON_FREE;
    m->magic = CVM_OWNEDMON_MAGIC;
#endif
}

static void
CVMownedMonitorDestroy(CVMOwnedMonitor *m)
{
}

/* Purpose: Nullifies the object reference in any bound CVMObjMonitors and
            moves them from the global bound list to the global unbound list
            if they are no longer associated with a live object. */
/* NOTE: The CVMObjMonitors (and their CVMOwnedMonitors) will actually be
         cleaned up in CVMmonitorScavengeUnbound(). */
/* NOTE: This method is only called during a GC cycle. */
static void
deleteUnreferencedBound(CVMExecEnv *ee,
			CVMRefLivenessQueryFunc isLive, void* isLiveData,
			CVMObjMonitor **fromListPtr,
			CVMObjMonitor **toListPtr) 
{
    CVMObjMonitor **monPtr = fromListPtr;
    CVMObjMonitor *mon = *monPtr;
    while (mon != NULL) {
	CVMassert(mon->state != CVM_OBJMON_FREE);
	CVMassert(mon->obj != NULL);
	if (!(*isLive)(&mon->obj, isLiveData)) {
	    /* not referenced, remove */
	    mon->obj = NULL;
	    if (toListPtr != NULL) {
		*monPtr = mon->next;
#ifdef CVM_TRACE
		if (mon->sticky) {
		    CVMtraceMisc(("deleting unreferenced sticky monitor\n"));
		}
#endif
		mon->sticky = CVM_FALSE;
		mon->next = *toListPtr;
		*toListPtr = mon;

		mon = *monPtr;
		continue;
	    }
	}
	monPtr = &mon->next;
	mon = *monPtr;
    }
}

/* Purpose: Nullifies the object reference of any CVMOwnedMonitors if they are
            no longer associated with a live object. */
/* NOTE: The fast lock CVMOwnedMonitors will actually be cleaned up in
         CVMmonitorScavengeFast(). */
/* NOTE: This method is only called during a GC cycle. */
static void
deleteUnreferencedFast(CVMExecEnv *ee,
		       CVMRefLivenessQueryFunc isLive, void* isLiveData,
		       CVMOwnedMonitor **fromListPtr)
{
    CVMOwnedMonitor *mon = *fromListPtr;
    while (mon != NULL) {
        CVMObject **objPtr = &mon->object;
        if ((*objPtr != NULL) && !(*isLive)(objPtr, isLiveData)) {
            /* not referenced, remove reference to object */
#if 0
	    /* Detect bug 4955461 in optimized builds */
	    CVMpanic("unreachable locked object");
#endif
            *objPtr = NULL;
#ifdef CVM_DEBUG
            CVMconsolePrintf("Warning! GC found "
                "unreachable *locked* object!\n");
#endif
        }
	mon = mon->next;
    }
}

/* Purpose: Delete all monitors which correspond to unreferenced objects. */
/* NOTE: This method is only called during a GC cycle. */
void
CVMdeleteUnreferencedMonitors(CVMExecEnv *ee,
			      CVMRefLivenessQueryFunc isLive, 
			      void* isLiveData,
			      CVMRefCallbackFunc transitiveScanner,
			      void* transitiveScannerData)
{
    deleteUnreferencedBound(ee, isLive, isLiveData,
	&CVMglobals.objLocksBound,
	&CVMglobals.objLocksUnbound);
    CVM_WALK_ALL_THREADS(ee, targetEE, {
	/* fast locks are not on bound list */
	deleteUnreferencedFast(ee, isLive, isLiveData,
	    &targetEE->objLocksOwned);
	/* unpin any unbound monitors */
	CVMobjMonitorUnpinUnreferenced(targetEE);
    });

    CVMscanMonitors(ee, transitiveScanner, transitiveScannerData);
}

static void
scanListBound(CVMExecEnv *targetEE, CVMRefCallbackFunc refCallback, void* data,
    CVMObjMonitor **fromListPtr, CVMObjMonitor **toListPtr)
{
    CVMObjMonitor *mon = *fromListPtr;
    while (mon != NULL) {
	CVMassert(mon->state != CVM_OBJMON_FREE);
	CVMassert(mon->obj != NULL);
	(*refCallback)(&mon->obj, data);
	mon = mon->next;
    }
}

static void
scanListFast(CVMExecEnv *targetEE, CVMRefCallbackFunc refCallback, void* data)
{
    CVMOwnedMonitor *mon = targetEE->objLocksOwned;
    while (mon != NULL) {
#ifdef CVM_DEBUG
	CVMassert(mon->state != CVM_OWNEDMON_FREE);
#endif
        if (mon->object != NULL) {
            (*refCallback)(&mon->object, data);
	}
	mon = mon->next;
    }
}

/* Purpose: Called by GC to update object pointers in all the monitor data
            structures.
*/
void
CVMscanMonitors(CVMExecEnv *ee, CVMRefCallbackFunc refCallback, void* data)
{
    /* Set busy state */
    CVM_WALK_ALL_THREADS(ee, targetEE, {
	CVMObjMonitor *mon = targetEE->objLockCurrent;
	if (mon != NULL) {
	    CVMassert(mon->state != CVM_OBJMON_FREE);
	    mon->state |= CVM_OBJMON_BUSY;
	}
	CVMobjMonitorMarkPinnedAsBusy(targetEE);
    });

    /* Scan global "bound" list */
    scanListBound(ee, refCallback, data,
	&CVMglobals.objLocksBound,
	&CVMglobals.objLocksFree);

    /* Scan per-thread owned lists */
    CVM_WALK_ALL_THREADS(ee, targetEE, {
	/* fast locks are not on bound list */
	scanListFast(targetEE, refCallback, data);
    });

    /* Undo busy state */
    CVM_WALK_ALL_THREADS(ee, targetEE, {
	CVMObjMonitor *mon = targetEE->objLockCurrent;
	if (mon != NULL) {
	    mon->state &= ~CVM_OBJMON_BUSY;
	}
	CVMobjMonitorMarkPinnedAsUnbusy(targetEE);
    });
}

/* Purpose: Scavenge for CVMOwnedMonitor for fast lock monitors which are no
           longer associated with any live objects. */
/* NOTE: Scavenging of heavy weight monitors are done in
         CVMmonitorScavengeUnbound(). */
/* NOTE: This function assumes that it won't be interrupted by a GC cycle.
         This is either enforced by being in a GC unsafe state or by acquiring
         some of the GC locks before entering this function. */
/* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
         CVMmonitorScavengeFast().  Do not cache these values across a call to
         this function.*/
static void
CVMmonitorScavengeFast(CVMExecEnv *ee)
{
    CVMOwnedMonitor **prev;
    CVMOwnedMonitor *o;

    /* Scavenge for unused CVMOwnedMonitors: */
    prev = &ee->objLocksOwned;
    o = ee->objLocksOwned;
    while (o != NULL) {
        /* NOTE: This check is not in contention with monitor inflation
            because only CVMOwnedMonitors whose o->object is not NULL will
            have a chance of being inflated. */
        if ((o->object == NULL) && (o->type == CVM_OWNEDMON_FAST)) {
            /* Remove the released CVMOwnedMonitor record from the owned list: */
            *prev = o->next;
#ifdef CVM_DEBUG
            CVMassert(o->state == CVM_OWNEDMON_OWNED);
            /* recycle mon: */
            o->state = CVM_OWNEDMON_FREE;
            o->u.fast.bits = 0;
#endif
            /* Add the released CVMOwnedMonitor record back on to the free list: */
            o->next = ee->objLocksFreeOwned;
            ee->objLocksFreeOwned = o;
            CVMassert(o->owner == ee);

            o = *prev;
            continue;
        }
        prev = &o->next;
        o = o->next;
    }
}

/* Purpose: Scavenge for CVMObjMonitors which no longer have a lock on them
            (found on the global bound list) and release them to the thread
            specific or global free list. */
/* NOTE: This function is protected from GC cycles by CVMglobals.syncLock. */
static void
CVMmonitorScavengeBound(CVMExecEnv *ee, CVMObjMonitor **fromListPtr,
                   CVMObjMonitor **toListPtr)
{
    CVMObjMonitor **monPtr = fromListPtr;
    CVMObjMonitor *mon = *monPtr;

    while (mon != NULL) {

	CVMassert(mon->state != CVM_OBJMON_FREE);

	if (CVMobjMonitorCount(mon) == 0 && mon->state == CVM_OBJMON_BOUND) {

            /* If we get here, then we know that no other thread will be in
               the process of locking this monitor.  In order to lock an
               inflated monitor, a thread must first indicate that it is
               currently working on a monitor while it is GC unsafe.  Before
               we call this scavenger, we walked all ther threads and marked
               those monitors as CVM_OBJMON_BUSY.  Hence, the above
               (mon->state == CVM_OBJMON_BOUND) would fail if the monitor is
               being locked.

               If we get here, then it is save to deflate the monitor provided
               it has an entry count of 0.  Otherwise, there may be other
               threads (i.e. not the owner thread) which are already blocked
               on it due to contention.  We cannot deflate in that case.
            */
            CVMassert(CVMprofiledMonitorGetOwner(&mon->mon) == NULL);

	    if (!mon->sticky) {
                CVMassert(fromListPtr == &CVMglobals.objLocksBound);
                mon->obj->hdr.various32 = mon->bits;
                mon->obj = NULL;
		mon->state = CVM_OBJMON_FREE;

                /* NOTE: There is no CVMOwnedMonitor associated with this
                    CVMObjMonitor to be released because the object's lock
                    count is 0.  It was dis-associated with its CVMOwnedMonitor
                    back when it was unlocked for the last time. */

                /* Move the CVMObjMonitor over to the global free list: */
                *monPtr = mon->next;
                mon->next = CVMglobals.objLocksFree;
                CVMglobals.objLocksFree = mon;
		mon = *monPtr;
		continue;
	    }
	}
	monPtr = &mon->next;
	mon = *monPtr;
    }
}

/* Purpose: Scavenge for CVMObjMonitors which are no longer associated with a
            live object (found on the global unbound list) and release them to
            the thread specific or global free list. */
/* NOTE: This function is protected from GC cycles by CVMglobals.syncLock. */
static void
CVMmonitorScavengeUnbound(CVMExecEnv *ee)
{
    CVMObjMonitor **monPtr = &CVMglobals.objLocksUnbound;
    CVMObjMonitor *mon = CVMglobals.objLocksUnbound;

    while (mon != NULL) {
        CVMassert(mon->state != CVM_OBJMON_FREE);
        if (!mon->sticky) {
            if (CVMprofiledMonitorGetOwner(&mon->mon) == ee) {

                /* NOTE: If we get here, the following conditions must be true:
                   1. The object which this CVMObjMonitor is associated with
                      is no longer reachable by anyone (including both java and
                      JNI code) other than the GC.
                   2. That means that no other threads will be competing to
                      invoke trylock(), lock(), tryUnlock(), unlock(), wait(),
                      notify(), or notifyAll() on the monitor.
                   3. Also, we're in a GC safe region operating under the
                      directions of the GC.  When any thread wishes to invoke
                      trylock(), lock(), tryUnlock(), unlock(), wait(),
                      notify(), or notifyAll() on any monitor, it must first
                      become GC unsafe.  This means that there won't be any
                      other threads competing to modify the global bound and
                      unbound lists.
                   4. We have also acquired the CVMglobals.syncLock.  Hence,
                      GC won't be competing for the global bound and unbound
                      lists.
    
                   Hence, it is thread-safe to release this CVMObjMonitor (and
                   its corresponding CVMOwnedMonitor) which is no longer
                   associated with a live object: */

                CVMOwnedMonitor **prev = &ee->objLocksOwned;
                CVMOwnedMonitor *o = ee->objLocksOwned;

                /* Get the CVMOwnedMonitor for this CVMObjMonitor: */
                /* Assume the most frequent case i.e. the last monitor locked is
                   the one being unlocked and the appropriate CVMOwnedMonitor is
                   the first on the owned list.  If this turns out to be untrue,
                   then we'll have to search for the appropriate CVMOwnedMonitor:
                */
                if (o->u.heavy.mon != mon) {
                    /* We use a do-while loop here instead of a while loop because
                       we've already eliminated the first element in the list as a
                       candidate and will not have to check it again. */
                    do {
                        prev = &o->next;
                        o = o->next;
                    } while ((o != NULL) && (o->u.heavy.mon != mon));

                    /* If we've successfully found the appropriate CVMOwnedMonitor,
                       the ptr should not be NULL: */
                    CVMassert (o != NULL);
                }
                /* By now, we should have attained the appropriate CVMOwnedMonitor
                   as pointed to by the o pointer. */

                CVMassert(o->owner == ee);
                CVMassert(o->type == CVM_OWNEDMON_HEAVY);

                /* Remove the released CVMOwnedMonitor from the owned list: */
                *prev = o->next;

                o->type = CVM_OWNEDMON_FAST;
#ifdef CVM_DEBUG
                o->state = CVM_OWNEDMON_FREE;
                o->u.fast.bits = 0;
                o->object = NULL;
                mon->owner = NULL;
#endif
                /* Add the released CVMOwnedMonitor back on to the free list: */
                o->next = ee->objLocksFreeOwned;
                ee->objLocksFreeOwned = o;

                /* Release the lock: */
                CVMobjMonitorCount(mon) = 1;
#ifdef CVM_THREAD_BOOST
		CVMassert(!mon->boost);
#endif
                CVMprofiledMonitorExit(&mon->mon, ee);
            }
            mon->state = CVM_OBJMON_FREE;

            /* Move the CVMObjMonitor over to the global free list: */
            *monPtr = mon->next;
            mon->next = CVMglobals.objLocksFree;
            CVMglobals.objLocksFree = mon;
            mon = *monPtr;
            continue;
        }
        monPtr = &mon->next;
        mon = *monPtr;
    }
}

/* Purpose: Run all monitor scavengers. */
/* NOTE: This function should only be called after all threads are consistent
         i.e. GC safe. */
/* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
         CVMsyncMonitorScavenge().  Do not cache these values across a call to
         this function. */
void
CVMsyncGCSafeAllMonitorScavenge(CVMExecEnv *ee)
{
    CVMsyncGCSafeAllMonitorScavengeInternal(ee, CVM_FALSE);
}

static void
CVMsyncGCSafeAllMonitorScavengeInternal(CVMExecEnv *ee,
                                        CVMBool releaseOwnedRecs)
{
    CVMassert(CVMD_gcAllThreadsAreSafe());

    /* Set busy state */
    /* %comment l021 */ 
    CVM_WALK_ALL_THREADS_START(ee, targetEE) {
	CVMObjMonitor *mon = targetEE->objLockCurrent;
	if (mon != NULL) {
	    CVMassert(mon->state != CVM_OBJMON_FREE);
	    mon->state |= CVM_OBJMON_BUSY;
	}
	CVMobjMonitorMarkPinnedAsBusy(targetEE);
    } CVM_WALK_ALL_THREADS_END(ee, targetEE)

    CVMmonitorScavengeBound(ee, &CVMglobals.objLocksBound,
	&CVMglobals.objLocksFree);
    CVMmonitorScavengeUnbound(ee);
    CVMmonitorScavengeFast(ee);

    /* The ownedRecs of the current thread can only be released under GC safe
       all conditions because the contents of these ownedRecs may still be
       queried when they aren't use in locks.  The querying only occurs under
       GC unsafe conditions.  Hence, freeing these records under GC safe all
       conditions guarantees that we won't try to access them after they are
       freed. */
    if (releaseOwnedRecs) {
        CVMOwnedMonitor *freeList = ee->objLocksFreeOwned;
        while (freeList != NULL) {
            CVMOwnedMonitor *ownedRec = freeList;
            freeList = ownedRec->next;
            CVMownedMonitorDestroy(ownedRec);
            free(ownedRec);
        }
        ee->objLocksFreeOwned = freeList;
    }

    /* Undo busy state */
    CVM_WALK_ALL_THREADS_START(ee, targetEE) {
	CVMObjMonitor *mon = targetEE->objLockCurrent;
	if (mon != NULL) {
	    mon->state &= ~CVM_OBJMON_BUSY;
	}
	CVMobjMonitorMarkPinnedAsUnbusy(targetEE);
    } CVM_WALK_ALL_THREADS_END(ee, targetEE)
}


/* %comment l020 */
/* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
         CVMsyncMonitorScavenge().  Do not cache these values across a call to
         this function. */
static void
CVMsyncMonitorScavenge(CVMExecEnv *ee, CVMBool releaseOwnedRecs)
{
    /* NOTE: These locks prevents GC from interfering: */
#ifdef CVM_JIT
    CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    CVMsysMutexLock(ee, &CVMglobals.syncLock);

    /* NOTE: Becoming GC safe prevents other threads from interfering: */
    CVMD_gcBecomeSafeAll(ee);

    CVMsyncGCSafeAllMonitorScavengeInternal(ee, releaseOwnedRecs);

    CVMD_gcAllowUnsafeAll(ee);

    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
}

#define CVM_PREALLOC_OBJMON_COUNT 1
#define CVM_PREALLOC_OWNMON_COUNT 1
#define CVM_RESERVED_OBJMON_COUNT CVM_PINNED_OBJMON_COUNT
/*
   This number depends on locks needed by Thread.exit().
   Example of three locks is:
	java.lang.Thread.exit()
	sun.misc.ThreadRegistry.remove()
	java.util.Vector.removeElement()
*/
#define CVM_RESERVED_OWNMON_COUNT 3
#define CVM_OBJMON_COUNT (CVM_PREALLOC_OBJMON_COUNT + CVM_RESERVED_OBJMON_COUNT)
#define CVM_OWNMON_COUNT (CVM_PREALLOC_OWNMON_COUNT + CVM_RESERVED_OWNMON_COUNT)

CVMBool
CVMeeSyncInit(CVMExecEnv *ee)
{
    int i;
    CVMOwnedMonitor *owned = NULL;
    CVMObjMonitor *obj = NULL;

    /* %comment l022 */
    /* create pre-allocated and reserved monitors */

    for (i = 0; i < CVM_OBJMON_COUNT; ++i) {
	CVMObjMonitor *o = (CVMObjMonitor *)calloc(1, sizeof (CVMObjMonitor));
	if (o == NULL) {
	    goto clean0;
	}
	if (CVMobjMonitorInit(ee, o, NULL, 0)) {
	    o->next = obj;
	    obj = o;
	} else {
	    free(o);
	    goto clean0;
	}
    }

    for (i = 0; i < CVM_OWNMON_COUNT; ++i) {
	CVMOwnedMonitor *o =
	    (CVMOwnedMonitor *)calloc(1, sizeof (CVMOwnedMonitor));
	if (o == NULL) {
	    goto clean0;
	}
	CVMownedMonitorInit(o, ee);
	o->next = owned;
	owned = o;
    }

    ee->objLocksFreeOwned = NULL;

    for (i = 0; i < CVM_PREALLOC_OWNMON_COUNT; ++i) {
	CVMOwnedMonitor *o = owned;
	owned = owned->next;
	o->next = ee->objLocksFreeOwned;
	ee->objLocksFreeOwned = o;
    }

    ee->objLocksFreeUnlocked = NULL;

    for (i = 0; i < CVM_PREALLOC_OBJMON_COUNT; ++i) {
	CVMObjMonitor *o = obj;
	obj = obj->next;
	o->next = ee->objLocksFreeUnlocked;
	ee->objLocksFreeUnlocked = o;
    }

    ee->objLocksReservedOwned = owned;
    ee->objLocksReservedUnlocked = obj;

    /*
     * Initialized the CVMOwnedMonitor that each thread has for
     * Simple Sync methods. They are only needed when using
     * CVM_FASTLOCK_ATOMICOPS.
     */
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) \
    && CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
    ee->simpleSyncReservedOwnedMonitor.owner = ee;
    ee->simpleSyncReservedOwnedMonitor.type = CVM_OWNEDMON_FAST;
#ifdef CVM_DEBUG
    ee->simpleSyncReservedOwnedMonitor.state = CVM_OWNEDMON_OWNED;
    ee->simpleSyncReservedOwnedMonitor.magic = CVM_OWNEDMON_MAGIC;
#endif
    ee->simpleSyncReservedOwnedMonitor.count = 1;
#endif

    return CVM_TRUE;

clean0:
    while (obj != NULL) {
	CVMObjMonitor *o = obj;
	obj = obj->next;
	CVMobjMonitorDestroy(o);
	free(o);
    }

    while (owned != NULL) {
	CVMOwnedMonitor *o = owned;
	owned = owned->next;
	CVMownedMonitorDestroy(o);
	free(o);
    }

    return CVM_FALSE;
}

void
CVMeeSyncDestroy(CVMExecEnv *ee)
{
    CVMOwnedMonitor **prev = &ee->objLocksOwned;
    CVMOwnedMonitor *o = ee->objLocksOwned;

    /* Unpin any monitors we inflated during exit. */
    CVMD_gcUnsafeExec(ee, {
	CVMobjMonitorUnpinAll(ee);
    });

    /*
     * Unlock owned monitors
     */
#if defined(CVM_DEBUG) && !defined(CVM_LVM)
    /* hideya004 */
    if (ee->objLocksOwned != NULL) {
        CVMconsolePrintf("Thread %d (%x) exited with locks held\n",
            ee->threadID, ee);
    }
#endif

    /* NOTE: The purpose of this loop is to reclaim any locks which are still
        held by the thread but should be released because the thread is coming
        to an end.  Reclamation can occur in one of 2 ways:
            1. If the monitor still corresponds to a live object, then simply
               unlocking the monitor will put it back on the free lists which
               will be cleaned up at the bottom of this function.
            2. If the monitor no longer corresponds to a live object, then the
               unlock mechanisms won't be able to handle them.  We skip these
               and let the scavengers release them once we're out of the loop.
    */

    while (o != NULL) {
        CVMOwnedMonitor *next;
	CVMassert(o->owner == ee);

	CVMID_localrootBegin(ee) {
	    CVMID_localrootDeclare(CVMObjectICell, objectICell);
	    CVMD_gcUnsafeExec(ee, {
                CVMID_icellSetDirect(ee, objectICell, o->object);
	    });

            next = o->next;
            /* If we have a monitor with a NULL object reference, then there
               must be some monitors which need to be cleaned up by the
               scavenger first: */
            if (!CVMID_icellIsNull(objectICell)) {

                /* unlock multiple lockings */
                if (o->type == CVM_OWNEDMON_HEAVY) {
                    CVMassert(CVMobjMonitorCount(o->u.heavy.mon) > 0);
                    CVMobjMonitorCount(o->u.heavy.mon) = 1;
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
                } else if (o->type == CVM_OWNEDMON_FAST) {
                    CVMassert(o->count > 0);
                    o->count = 1;
#endif
                }
                {
                    CVMBool success = CVMgcSafeObjectUnlock(ee, objectICell);
                    CVMassert(success); (void) success;
                }
                CVMassert(*prev != o);
            } else {
                prev = &o->next;
            }
            o = next;
	} CVMID_localrootEnd();
    }

    /* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
        CVMsyncMonitorScavenge().  Do not cache these values.*/
    /* NOTE: We let the scavenger take care of freeing up all the
       CVMOwnedMonitors under a GC safe all condition. */
    CVMsyncMonitorScavenge(ee, CVM_TRUE);

    /* The scavenger should have released all the CVMOwnedMonitors already: */
    CVMassert(ee->objLocksFreeOwned == NULL);

    while (ee->objLocksReservedOwned != NULL) {
	CVMOwnedMonitor *ownedRec = ee->objLocksReservedOwned;
	ee->objLocksReservedOwned = ownedRec->next;
	CVMownedMonitorDestroy(ownedRec);
	free(ownedRec);
    }

    /*
     * Release locks from free lists
     */
    while (ee->objLocksFreeUnlocked != NULL) {
	CVMObjMonitor *m = ee->objLocksFreeUnlocked;
	ee->objLocksFreeUnlocked = m->next;
	CVMobjMonitorDestroy(m);
	free(m);
    }
    while (ee->objLocksReservedUnlocked != NULL) {
	CVMObjMonitor *m = ee->objLocksReservedUnlocked;
	ee->objLocksReservedUnlocked = m->next;
	CVMobjMonitorDestroy(m);
	free(m);
    }
}

CVMBool
CVMeeSyncInitGlobal(CVMExecEnv *ee, CVMGlobalState *gs)
{
    int i;
    /* Allocate an initial number of obj monitors because we'll probably need
       that many to begin with.  This will save us the cost of running that
       many unsuccessful scavenging runs at VM start-up. */
    for (i = 0; i < CVM_INITIAL_NUMBER_OF_OBJ_MONITORS; i++) {
        CVMObjMonitor *mon;

        mon = (CVMObjMonitor *)calloc(1, sizeof (CVMObjMonitor));
        if ((mon != NULL) && CVMobjMonitorInit(ee, mon, NULL, 0)) {
            CVMassert(CVMobjMonitorOwner(mon) == NULL);
            mon->next = gs->objLocksFree;
            gs->objLocksFree = mon;
        } else {
            return CVM_FALSE;
        }
    }
    return CVM_TRUE;
}

void
CVMeeSyncDestroyGlobal(CVMExecEnv *ee, CVMGlobalState *gs)
{
    /*
     * Transfer (hopefully all) locks from Bound and Unbound lists
     * to Free list.
     * NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
     *       CVMsyncMonitorScavenge().  Do not cache these values.
     */

    /* Unpin any monitors we inflated during exit */
    CVMobjMonitorUnpinAll(ee);

    {
	CVMObjMonitor *mon = gs->objLocksBound;
	while (mon != NULL) {
	    /* Probably because of CVM.objectInflatePermanently() */
	    mon->sticky = CVM_FALSE;
	    mon = mon->next;
	}
    }
    CVMsyncMonitorScavenge(ee, CVM_FALSE);

#ifdef CVM_DEBUG
    if (gs->objLocksBound != NULL) {
	CVMconsolePrintf("VM exited with bound busy monitors!\n");
    }
    if (gs->objLocksUnbound != NULL) {
        CVMconsolePrintf("VM exited with unbound busy monitors!\n");
    }
#endif

    /*
     * Release locks from free lists
     */
    while (gs->objLocksFree != NULL) {
	CVMObjMonitor *m = gs->objLocksFree;
	gs->objLocksFree = m->next;
	CVMobjMonitorDestroy(m);
	free(m);
    }

}

/* Purpose: Sets the hash value in an inflated object monitor. */
/* NOTE: Only called by CVMobjectSetHashBitsIfNeeded() while GC unsafe.  Must
         not become GC safe. */
static CVMInt32
CVMobjectSetHashBitsInInflatedMonitorIfNeeded(CVMExecEnv *ee,
					      CVMObject *directObj,
					      CVMUint32 hashValue)
{
    CVMObjMonitor *mon = CVMobjMonitor(directObj);
    CVMassert(CVMD_isgcUnsafe(ee));

    /* NOTE: Deflation only occurs while scavenging, but scavenging can
        only occur when all threads have become gc safe.  Hence, being gc
        unsafe protects us from deflation. */
    CVMobjectMicroLock(ee, directObj);
    {
        CVMUint32 h = CVMhdrBitsHashCode(mon->bits);
        if (h == CVM_OBJECT_NO_HASH) {
            CVMhdrSetHashCode(mon->bits, hashValue);
        } else {
            hashValue = h;
        }
    }
    CVMobjectMicroUnlock(ee, directObj);
    return hashValue;
}

/* NOTE: Only called while in a GC unsafe state. */
static CVMInt32
CVMobjectSetHashBitsIfNeeded(CVMExecEnv *ee, CVMObjectICell* indirectObj,
			     CVMUint32 hashValue)
{
    CVMAddr bits;

    CVMassert(CVMD_isgcUnsafe(ee));
    CVMassert((hashValue & ~CVM_OBJECT_HASH_MASK) == 0);

    /* Check to see if the monitor has already been inflated by anyone else: */
    {
        CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
        if (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR) {
            hashValue = CVMobjectSetHashBitsInInflatedMonitorIfNeeded(ee, obj,
                            hashValue);
            return hashValue;
        }
    }

    /* See notes above on "Synchronization between various Monitor Methods": */
    CVMD_gcSafeExec(ee, {
        CVMsysMutexLock(ee, &CVMglobals.syncLock);
    });

    {
        CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
        bits = obj->hdr.various32;
    }

    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
        /* If we're here, then another thread has inflated the monitor: */

        CVMObject *obj = CVMID_icellDirect(ee, indirectObj);
        hashValue = CVMobjectSetHashBitsInInflatedMonitorIfNeeded(ee, obj,
                        hashValue);
    } else {

        CVMObject *obj = CVMID_icellDirect(ee, indirectObj);

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
	/* 
	 * obits can hold a native pointer (see CVMobjectVariousWord())
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
        CVMAddr obits;

        /* Prevent any fast lock/unlock operations: */
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
        obits = CVMatomicSwap(&(obj)->hdr.various32, CVM_LOCKSTATE_LOCKED);
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK
        CVMobjectMicroLock(ee, obj);
        obits = obj->hdr.various32;
#endif

	/* Normally, "ROMized objects" are writable, as they are exposed
	 * to synchronization code which requires you to write into object
	 * headers.  These ROMized objects would all have their hashcodes
	 * set during preloader initialization.  Hence, we'll never need to
	 * set them below.
	 *
	 * There are some exceptional cases of ROMized objects that do not
	 * have their hashcode set by the preloader.  These include the
	 * shared char[] that comprises the body of ROMized String's, and
	 * also the various CharacterData internal arrays.  These objects
	 * are immutable and are not visible to application code.  Hence,
	 * no one will be able to get a hold of them to ask for a hashcode.
	 *
	 * The only case where we could ask for a hashcode on those
	 * exceptional ROMized objects is from VM debugging code.  However,
	 * VM code will never (and cannot) lock those objects.  Hence, their
	 * hashValues will have been fetched by CVMobjectGetHashNoSet() and
	 * we should never get to this point.  CVMobjectGetHashNoSet() will
	 * return the address of these ROMized objects as their hashcode,
	 * without touching their headers.
	 *
	 * We assert below that the object is not a ROMized before setting
	 * its hashcode value to make sure that this assumption hasn't
	 * changed.
	 */

        if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_LOCKED) {
            /* If we're here, then the object has been locked using the fast
               lock mechanism.  Hence, the hash value will have to be set in
               the corresponding CVMOwnedMonitor: */
            CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(obits);
            CVMUint32 h = CVMhdrBitsHashCode(o->u.fast.bits);

#ifdef CVM_DEBUG
            CVMassert(o->magic == CVM_OWNEDMON_MAGIC);
#endif
            CVMassert(o->type == CVM_OWNEDMON_FAST);

            if (h == CVM_OBJECT_NO_HASH) {
		CVMassert(!CVMobjectIsInROM(obj));
		CVMhdrSetHashCode(o->u.fast.bits, hashValue);
            } else {
                hashValue = h;
            }
        } else if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_UNLOCKED) {
            /* If we're here, then the object is not locked.  Hence, the hash
               value will have to be set in the object's header bits: */

            CVMUint32 h = CVMhdrBitsHashCode(obits);
            if (h == CVM_OBJECT_NO_HASH) {
		CVMassert(!CVMobjectIsInROM(obj));
		CVMhdrSetHashCode(obits, hashValue);
            } else {
                hashValue = h;
            }
            /* At this point the object header needs to be updated, but this
               done in common code below: */
        }

        /* Re-allow fast lock/unlock operations: */
        obj->hdr.various32 = obits;
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK
        CVMobjectMicroUnlock(ee, obj);
#endif

#else   /* CVM_FASTLOCK_TYPE == CVM_FASTLOCK_NONE */

        /* If we're here, then the monitor is not locked.  Hence, set the hash
           value in the object header: */
        CVMUint32 h = CVMhdrBitsHashCode(obj->hdr.various32);
        if (h == CVM_OBJECT_NO_HASH) {
	    CVMassert(!CVMobjectIsInROM(obj));
	    CVMhdrSetHashCode(obj->hdr.various32, hashValue);
        } else {
            hashValue = h;
        }
#endif
    }

    /* %comment l010 */
    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    return hashValue;
}

/* Use the lowest meaningful bits to fill in the upper bits of the object
   hash.  The lowest 4 bits are not meaningful because they are found to
   be normally 0 due to alignment.  Hence, use bit 4 and up as needed.  These
   bits will be xor'ed with the computed hash bits so that they won't be
   identifiable by the class of the object. */
#define HASH_WITH_CB_BITS(cb, hash) \
    ((hash) |								\
     ((((CVMAddr)(cb) & ~0xf)<<(CVM_HASH_BITS - 4)) ^ ((hash)<<CVM_HASH_BITS)))

/*
 * Compute the hash code for an object, store it in its header, and return
 * the hash code.
 */
static CVMInt32
CVMobjectComputeHash(CVMExecEnv *ee, CVMObjectICell* indirectObj)
{
    CVMInt32 hashVal;
    /*
     * Get the next random number that doesn't have an alternate meaning.
     */
    do {
	hashVal = CVMrandomNext();
	hashVal &= CVM_OBJECT_HASH_MASK;
    } while (hashVal == CVM_OBJECT_NO_HASH);

    CVMD_gcUnsafeExec(ee, {
	CVMObject *directObj;

	hashVal = CVMobjectSetHashBitsIfNeeded(ee, indirectObj, hashVal);
	/* The hash bits need to be combined with the cb bits to make a
	   complete 32-bit hashcode: */
	directObj = CVMID_icellDirect(ee, indirectObj);
	hashVal = HASH_WITH_CB_BITS(CVMobjectGetClass(directObj), hashVal);
    });
    return hashVal;
}

CVMInt32
CVMobjectGetHashSafe(CVMExecEnv *ee, CVMObjectICell* indirectObj)
{
    CVMInt32 hashValue;
    {
	CVMD_gcUnsafeExec(ee, {
	    CVMObject *directObj = CVMID_icellDirect(ee, indirectObj);
	    hashValue = CVMobjectGetHashNoSet(ee, directObj);
	});
    }
    if (hashValue == CVM_OBJECT_NO_HASH) {
	return CVMobjectComputeHash(ee, indirectObj);
    } else {
	return hashValue;
    }
}

/* NOTE: Only executes in a GC unsafe state. */
CVMInt32
CVMobjectGetHashNoSet(CVMExecEnv *ee, CVMObject* directObj)
{
    CVMInt32 hash;

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
#if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
    {
	/* 
	 * bits can hold a native pointer (see CVMobjectVariousWord())
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr bits = CVMobjectVariousWord(directObj);
	if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
	    CVMObjMonitor *mon = (CVMObjMonitor *)CVMhdrBitsPtr(bits);
#ifdef CVM_DEBUG
	    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	    hash = CVMhdrBitsHashCode(mon->bits);
	} else if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_UNLOCKED) {
	    hash = CVMhdrBitsHashCode(bits);
	} else /* CVM_LOCKSTATE_LOCKED */ {
            /* With the fast lock bit set, it is difficult to get a hold of
               the CVMOwnedMonitor in a consistent state (not in the midst of
               being changed by another thread) without first acquiring some
               synchronization locks.  We'll need to get a hold of the
               CVMOwnedMonitor because the object header bits are in it.
               Hence, force the slow path in order to get the hash value. */
	    return CVM_OBJECT_NO_HASH;
	}
    }
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK
    {
	/* 
	 * bits can hold a native pointer (see CVMobjectVariousWord())
	 * therefore the type has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMAddr bits;
	CVMobjectMicroLock(ee, directObj);
	bits = CVMobjectVariousWord(directObj);
	if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
	    CVMObjMonitor *mon = (CVMObjMonitor *)CVMhdrBitsPtr(bits);
#ifdef CVM_DEBUG
	    CVMassert(mon->magic == CVM_OBJMON_MAGIC);
#endif
	    hash = CVMhdrBitsHashCode(mon->bits);
	} else if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_LOCKED) {
	    CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(bits);
#ifdef CVM_DEBUG
	    CVMassert(o->magic == CVM_OWNEDMON_MAGIC);
#endif
	    CVMassert(o->type == CVM_OWNEDMON_FAST);
	    hash = CVMhdrBitsHashCode(o->u.fast.bits);
	} else {
	    hash = CVMhdrBitsHashCode(CVMobjectVariousWord(directObj));
	}
	CVMobjectMicroUnlock(ee, directObj);
    }
#else
#error
#endif
#else   /* CVM_FASTLOCK_TYPE == CVM_FASTLOCK_NONE */

    /* The following "if" is based on a sample of the object header bits.  In
       the else clause below, it is important to get the hash value from the
       previously sampled bits (as opposed to resampling) because if the monitor
       was inflated by another thread after the initial sampling, we would end
       up in the else clause but a resample of the bits could get us the
       address of the inflated monitor instead of the proper hash value.

           If the bits indicate that the monitor has been inflated, then we're
       safe in accessing the monitor bits without a lock because we're
       currently GC unsafe, and the monitor will not be deflated until this
       thread becomes GC safe.  Hence, the monitor bits are stable.

           If the bits indicate otherwise, then it may already have the hash
       value in it.  In this case, the proper hash value will be returned.  If
       it doesn't already have the hash value (even if the hash value was set
       by another thread after we've sampled the bits), then effectively,
       CVM_OBJECT_NO_HASH will be returned, and we'll just have to get the get
       the hash value the slow way (because we cannot reliably compete with a
       race condition):
    */
    /* 
     * Scalar variables intended to hold pointer values
     * have to be of the type CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms.
     */
    CVMAddr bits = CVMobjectVariousWord(directObj);
    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
	hash = CVMhdrBitsHashCode(CVMobjMonitor(directObj)->bits);
    } else {
        hash = CVMhdrBitsHashCode(bits);
    }
#endif
    if (hash != CVM_OBJECT_NO_HASH) {
	/* The hash bits needs to be combined with the cb bits to make a
	   complete 32-bit hashcode: */
	hash = HASH_WITH_CB_BITS(CVMobjectGetClass(directObj), hash);
    } else {
	/* This is to handle some exceptional cases of ROMized objects whose
	   hash values are not set.  The normal cases of ROMized objects would
	   have their hashcode values already set during preloader
	   initialization.
	*/
	if (CVMobjectIsInROM(directObj)) {
	    hash = (CVMUint32)CVMobjectGetClassWord(directObj);
	}
    }
    return hash;
}

/* Purpose: Checks to see if the current thread owns a lock on the specified
            object.  This function can become GC safe. */
CVMBool
CVMobjectLockedByCurrentThread(CVMExecEnv *ee, CVMObjectICell *objICell)
{
    CVMObject *directObj;
    CVMBool result;
    CVMAddr bits;

    CVMassert(CVMD_isgcUnsafe(ee));

    directObj = (CVMObject *)CVMID_icellDirect(ee, objICell);
    bits = CVMobjectVariousWord(directObj);
    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
        /* An inflated monitor can only be deflated when all threads are GC
           safe.  Since we are currently GC unsafe, we know that the inflated
           monitor will be kept intact and we can query it's owner safely: */
        CVMObjMonitor *objMon = (CVMObjMonitor *)CVMhdrBitsPtr(bits);
        CVMExecEnv *owner;
#ifdef CVM_DEBUG
        CVMassert(objMon->magic == CVM_OBJMON_MAGIC);
#endif
	owner = CVMprofiledMonitorGetOwner(&objMon->mon);
        result = (ee == owner);

    } else if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_UNLOCKED) {
        /* If the snapshot of the object bits indicate that the object is
           unlocked, then we know that the current thread does not own it.
           It doesn't matter if another thread locks it after this.  The
           fact still remains that the current thread does not own it. */
        result = CVM_FALSE;

    } else {
        CVMOwnedMonitor *ownedRec = (CVMOwnedMonitor *)CVMhdrBitsPtr(bits);
        CVMassert(CVMhdrBitsSync(bits) == CVM_LOCKSTATE_LOCKED);
        if (ownedRec == NULL) {
            /* If we're here, then there must be another thread which is
               currently inflating the monitor.  Hence, we should rendezvous
               on the syncLock to make sure that all heavy weight operations
               are done before we check this status again.
               NOTE: We recurse into this function again to do the check, but
               because we hold the syncLock this time, we're guaranteed that
               it won't recurse again: */
            CVMD_gcSafeExec(ee, {
                CVMsysMutexLock(ee, &CVMglobals.syncLock);
            });
            result = CVMobjectLockedByCurrentThread(ee, objICell);
            CVMsysMutexUnlock(ee, &CVMglobals.syncLock);

        } else {
            /* A CVMOwnedMonitor can only be freed when all threads are GC
              safe.  Since we are currently GC unsafe, we know that the
              CVMOwnedMonitor will be kept around while we're in this code
              and we can query it's owner safely: */
            result = (ee == ownedRec->owner);
        }
    }
    return result;
}

CVMBool
CVMgcSafeObjectLock(CVMExecEnv *ee, CVMObjectICell *o)
{
    CVMBool success;
    CVMD_gcUnsafeExec(ee, {
        /* %comment l002 */
        success = CVMobjectTryLock(ee, CVMID_icellDirect(ee, o));
        if (!success) {
            success = CVMobjectLock(ee, o);
        }
    });
    return success;
}

CVMBool
CVMgcSafeObjectUnlock(CVMExecEnv *ee, CVMObjectICell *o)
{
    CVMBool success;
    CVMD_gcUnsafeExec(ee, {
        /* %comment l003 */
        success = CVMobjectTryUnlock(ee, CVMID_icellDirect(ee, o));
        if (!success) {
            success = CVMobjectUnlock(ee, o);
        }
    });
    return success;
}

CVMBool
CVMgcSafeObjectWait(CVMExecEnv *ee, CVMObjectICell *o, CVMInt64 millis)
{
    CVMBool status;
    CVMD_gcUnsafeExec(ee, {
	status = CVMobjectWait(ee, o, millis);
    });
    return status;
}

CVMBool
CVMgcSafeObjectNotify(CVMExecEnv *ee, CVMObjectICell *o)
{
    CVMBool status;
    CVMD_gcUnsafeExec(ee, {
        status = CVMobjectNotify(ee, o);
    });
    return status;
}

CVMBool
CVMgcSafeObjectNotifyAll(CVMExecEnv *ee, CVMObjectICell *o)
{
    CVMBool status;
    CVMD_gcUnsafeExec(ee, {
        status = CVMobjectNotifyAll(ee, o);
    });
    return status;
}


#ifdef CVM_DEBUG
void
CVMobjectMicroLock(CVMExecEnv *ee, CVMObject *obj)
{
    CVMassert((CVMD_isgcUnsafe(ee) || CVMgcIsGCThread(ee) ||
               CVMsysMutexIAmOwner((ee), &CVMglobals.heapLock)) &&
              ++ee->microLock == 1);
    CVMobjectMicroLockImpl(obj);
}

void
CVMobjectMicroUnlock(CVMExecEnv *ee, CVMObject *obj)
{
    CVMassert((CVMD_isgcUnsafe(ee) || CVMgcIsGCThread(ee) ||
               CVMsysMutexIAmOwner((ee), &CVMglobals.heapLock)) &&
              --ee->microLock == 0);
    CVMobjectMicroUnlockImpl(obj);
}
#endif

#ifdef CVM_THREAD_BOOST
static
void CVMboost(CVMExecEnv *ee, CVMObjMonitor *mon, CVMExecEnv *owner)
{
    CVMD_gcSafeExec(ee, {
	CVMBool boost;
	/* Loop to handle spurious wakeups */
	do {
	    CVMsysMutexLock(ee, &CVMglobals.syncLock);
	    if (mon->boost) {
		CVMThreadBoostQueue *bq;

		CVMdebugExec(CVMassert(owner == mon->boostThread));

		/*
		   We need to save the queue we intend to wait on,
		   just in case the boost is canceled right after
		   we release the sync lock.
		*/
		bq = CVMthreadAddBooster(CVMexecEnv2threadID(ee),
		    &mon->boostInfo, CVMexecEnv2threadID(owner));

		CVMsysMutexUnlock(ee, &CVMglobals.syncLock);

		/*
		   The boost could have already been canceled.
		   (But a new boost is not allowed without a
		   deflation, which the caller prevents.)
		   Remove ourselves from the wait queue and
		   block if necessary.
		*/
		CVMthreadBoostAndWait(CVMexecEnv2threadID(ee),
		    &mon->boostInfo, bq);

		boost = CVM_TRUE;
	    } else {
		boost = CVM_FALSE;
		CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
	    }
	} while (boost);
    });
}

static
void CVMunboost(CVMExecEnv *ee, CVMObjMonitor *mon)
{
    CVMExecEnv *owner = CVMprofiledMonitorGetOwner(&mon->mon);
    if (owner == ee) {
	CVMprofiledMonitorAcquireMutex(ee, &mon->mon);
	CVMD_gcSafeExec(ee, {
	    CVMsysMutexLock(ee, &CVMglobals.syncLock);

	    /* NOTE: The value of mon->boost can only be changed under lock of
	       the syncLock. */
	    CVMassert(mon->boost);
	    CVMthreadCancelBoost(CVMexecEnv2threadID(ee), &mon->boostInfo);
	    mon->boost = CVM_FALSE;
	    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
	});
    }
}
#endif /* CVM_THREAD_BOOST */
