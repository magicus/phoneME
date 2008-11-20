/*
 * @(#)Reference.java	1.51 06/10/10
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

package java.lang.ref;

import sun.misc.ThreadRegistry;
import sun.misc.CVM;

/**
 * Abstract base class for reference objects.  This class defines the
 * operations common to all reference objects.	Because reference objects are
 * implemented in close cooperation with the garbage collector, this class may
 * not be subclassed directly.
 *
 * @version  1.35 08/27/01
 * @author   Mark Reinhold
 * @since    1.2
 */

public abstract class Reference {

    /* A Reference instance is in one of four possible internal states:
     *
     *	   Active: Subject to special treatment by the garbage collector.  Some
     *	   time after the collector detects that the reachability of the
     *	   referent has changed to the appropriate state, it changes the
     *	   instance's state to either Pending or Inactive, depending upon
     *	   whether or not the instance was registered with a queue when it was
     *	   created.  In the former case it also adds the instance to the
     *	   pending-Reference list.  Newly-created instances are Active unless
     *	   their referents are null, in which case they are Inactive.
     *
     *	   Pending: An element of the pending-Reference list, waiting to be
     *	   enqueued by the Reference-handler thread.  Unregistered instances
     *	   are never in this state.
     *
     *	   Enqueued: An element of the queue with which the instance was
     *	   registered when it was created.  When an instance is removed from
     *	   its ReferenceQueue, it is made Inactive.  Unregistered instances are
     *	   never in this state.
     *
     *	   Inactive: Nothing more to do.  Once an instance becomes Inactive its
     *	   state will never change again.
     *
     * The state is encoded in the queue and next fields as follows:
     *
     *	   Active: queue = ReferenceQueue with which instance is registered, or
     *	   ReferenceQueue.NULL if it was not registered with a queue; next =
     *	   null.
     *
     *	   Pending: queue = ReferenceQueue with which instance is registered;
     *	   next = Following instance in queue, or this if at end of list.
     *
     *	   Enqueued: queue = ReferenceQueue.ENQUEUED; next = Following instance
     *	   in queue, or this if at end of list.
     *
     *	   Inactive: queue = ReferenceQueue.NULL; next = this.
     *
     * With this scheme the collector need only examine the next field in order
     * to determine whether a Reference instance requires special treatment: If
     * the next field is null then the instance is active; if it is non-null,
     * then the collector should treat the instance normally.
     */

    /*
     * IMPORTANT NOTE: The referent and the next fields should be the
     * first two fields of this class for efficient treatment by GC.
     * (11/30/99)
     */
    private Object referent;		/* Treated specially by GC */
    Reference next;
    ReferenceQueue queue;


    /* Object used to synchronize with the garbage collector.  The collector
     * must acquire this lock at the beginning of each collection cycle.  It is
     * therefore critical that any code holding this lock complete as quickly
     * as possible, allocate no new objects, and avoid calling user code.
     */
    private static Object lock = new Object();

    /*
     * lock must always have an inflated monitor associated with it so locking
     * it never requires grabbing the jitlock. Otherwise a deadlock may occur.
     */
    static {
	if (!sun.misc.CVM.objectInflatePermanently(lock)) {
	    throw new OutOfMemoryError();
	}
    }


    /* List of References waiting to be enqueued.  The collector adds
     * References to this list, while the Reference-handler thread removes
     * them.  This list is protected by the above lock object.
     */
    private static Reference pending = null;

    /* High-priority thread to enqueue pending References
     */
    private static class ReferenceHandler extends Thread {

	ReferenceHandler(ThreadGroup g, String name) {
	    super(g, name);
	}

	public void run() {
	    while (!ThreadRegistry.exitRequested()) {

		Reference r;
		/* We must disable compilation in this thread while holding
		 * Reference.lock() because initiating a compilation can
		 * lead to a deadlock in this case.
		 */
		sun.misc.CVM.setThreadNoCompilationsFlag(true);
		synchronized (lock) {
		    if (pending != null) {
			r = pending;
			Reference rn = r.next;
			pending = (rn == r) ? null : rn;
			r.next = r;
		    } else {
			try {
			    lock.wait();
			} catch (InterruptedException x) { }
			continue;
		    }
		}
		sun.misc.CVM.setThreadNoCompilationsFlag(false);

		ReferenceQueue q = r.queue;
		if (q != ReferenceQueue.NULL) q.enqueue(r);
	    }
	}
    }

    //
    // Distribute all enqueued references into their respective queues
    //
    static void handleAllEnqueued() {
	Reference r;
	while (pending != null) {
	    r = pending;
	    Reference rn = r.next;
	    pending = (rn == r) ? null : rn;
	    r.next = r;
	    ReferenceQueue q = r.queue;
	    if (q != ReferenceQueue.NULL) q.enqueue(r);
	}
    }

    static {
	if (CVM.inMainLVM()) { // %begin,end lvm
	    createReferenceHandlerThread();
	} // %begin,end lvm
    }
    private static void createReferenceHandlerThread() {
	ThreadGroup tg = Thread.currentThread().getThreadGroup();
	for (ThreadGroup tgn = tg;
	     tgn != null;
	     tg = tgn, tgn = tg.getParent());
	Thread handler = new ReferenceHandler(tg, "Reference Handler");
	/* If there were a special system-only priority greater than
	 * MAX_PRIORITY, it would be used here
	 */
	handler.setPriority(Thread.MAX_PRIORITY);
	handler.setDaemon(true);
	handler.start();
    }

    //
    // This is called from the mTASK code. We make this private in
    // this public class so we don't change its signature.
    //
    private static void restartReferenceThreads()
    {
	//
	// First of all, get rid of the kill system threads flag, so we
	// can get started
	//
	ThreadRegistry.resetExitRequest();

	//
	// Now re-start threads
	//
	Reference.createReferenceHandlerThread();
	Finalizer.startFinalizerThread();
    }
    
    //
    // This is called from the mTASK code. We make this private in
    // this public class so we don't change its signature.
    //
    private static void stopReferenceThreads()
    {
	// NOTE: Can add assertion here to make
	// sure that the reference threads are the only system threads
	// alive at this point.
	ThreadRegistry.waitAllSystemThreadsExit();

	// NOTE: The activity below can be moved to somewhere better,
	// like sun.misc.CVM. It has more to do with mtask and less to do
	// with stopping reference threads.

	// Now GC a few times to get rid of anything referred to
	// by the exiting system threads
	System.gc();
	System.gc();
	System.gc();
    }
    

    /* -- Referent accessor and setters -- */

    /**
     * Returns this reference object's referent.  If this reference object has
     * been cleared, either by the program or by the garbage collector, then
     * this method returns <code>null</code>.
     *
     * @return	 The object to which this reference refers, or
     *		 <code>null</code> if this reference object has been cleared
     */
    public Object get() {
	return this.referent;
    }

    /**
     * Clears this reference object.  Invoking this method will not cause this
     * object to be enqueued.
     */
    public void clear() {
	this.referent = null;
    }


    /* -- Queue operations -- */

    /**
     * Tells whether or not this reference object has been enqueued, either by
     * the program or by the garbage collector.	 If this reference object was
     * not registered with a queue when it was created, then this method will
     * always return <code>false</code>.
     *
     * @return	 <code>true</code> if and only if this reference object has
     *		 been enqueued
     */
    public boolean isEnqueued() {
	/* In terms of the internal states, this predicate actually tests
	   whether the instance is either Pending or Enqueued */
	synchronized (this) {
	    return (this.queue != ReferenceQueue.NULL) && (this.next != null);
	}
    }

    /**
     * Adds this reference object to the queue with which it is registered,
     * if any.
     *
     * @return	 <code>true</code> if this reference object was successfully
     *		 enqueued; <code>false</code> if it was already enqueued or if
     *		 it was not registered with a queue when it was created
     */
    public boolean enqueue() {
	return this.queue.enqueue(this);
    }


    /* -- Constructors -- */

    Reference(Object referent) {
	this(referent, null);
    }

    Reference(Object referent, ReferenceQueue queue) {
	this.referent = referent;
	this.queue = (queue == null) ? ReferenceQueue.NULL : queue;
    }

}
