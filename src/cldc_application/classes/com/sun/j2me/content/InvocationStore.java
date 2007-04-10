/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import com.sun.midp.midlet.MIDletSuite;

/**
 * The store for pending Invocations.
 * New Invocations are queued with {@link #put} method and
 * retrieved with the {@link #get} method. The {@link #cancel}
 * method is used to unblock calls to blocking {@link #get} methods.
 * <p>
 * Synchronization is performed by the native methods; access
 * is serialized by the VM running in a single native thread and
 * by NOT preempting native method calls.
 * The native code uses the SNI ability to block a thread and
 * unblock it at a later time. The implementation does not poll for
 * requests but blocks, if requested, until it is unblocked.
 */
public class InvocationStore {

    /**
     * The count of cancel requests; access is not synchronized because
     * it is only incrementes in one place and it does not matter if it is
     * incremented once or twice.  A new cancel has occurred if
     * the value has been incremented since an operation was started.
     */
    private static int cancelCount;

    /** The mode for get to retrieve a new request. */
    private static final int MODE_REQUEST = 0;

    /** The mode for get to retrieve a new response. */
    private static final int MODE_RESPONSE = 1;

    /** The mode for get to retrieve a new cleanup. */
    private static final int MODE_CLEANUP = 2;

    /** The mode for listen for new unmarked request. */
    private static final int MODE_LREQUEST = 3;

    /** The mode for listen for a new unmarked response. */
    private static final int MODE_LRESPONSE = 4;

    /** The mode for get to retrieve a new ACTIVE, HOLD, or WAITING request. */
    // private static final int MODE_PENDING = 5;

    /** The mode for get to retreive byte <code>tid</code>. */
    private static final int MODE_TID = 6;

    /** The mode to get the Invocation after <code>tid</code>. */
    private static final int MODE_TID_NEXT = 7;

    /** The mode to get the Invocation before <code>tid</code>. */
    private static final int MODE_TID_PREV = 8;

    /**
     * Private constructor to prevent instance creation.
     */
    private InvocationStore() {
    }

    /**
     * Put a new Invocation into the store.
     * It can be modified by {@link #setStatus}.
     * The TID (transaction ID) is updated with a newly assigned value.
     *
     * @param invoc an InvocationImpl instance with the members properly
     *  initialized.
     * @see #getRequest
     * @see #getResponse
     */
    static void put(InvocationImpl invoc) {
        if (invoc.storageId == MIDletSuite.UNUSED_SUITE_ID ||
                invoc.classname == null) {
            throw new NullPointerException();
        }
        put0(invoc);
	if (AppProxy.LOG_INFO) {
	    AppProxy.getCurrent().logInfo("Store put0: " + invoc);
	}
    }

    /**
     * Get a new InvocationImpl request from the store using a MIDlet
     * suiteId and classname.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname with
     *  its status is set to ACTIVE;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getRequest(int suiteId, String classname,
                                     boolean shouldBlock) {
        InvocationImpl invoc = new InvocationImpl();
        if (suiteId == MIDletSuite.UNUSED_SUITE_ID || classname == null) {
            throw new NullPointerException();
        }
        invoc.storageId = suiteId;
        invoc.classname = classname;

        return get(invoc, MODE_REQUEST, shouldBlock);
    }

    /**
     * Get a new InvocationImpl response from the store using a
     * MIDlet suiteId and classname.
     * The response is removed from the store.
     *
     * @param invoc an InvocationImpl to fill with the response
     * @param suiteId the MIDletSuite ID
     * @param classname the classname
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname if one was requested;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getResponse(InvocationImpl invoc,
                                      int suiteId, String classname,
                                      boolean shouldBlock) {
        invoc.storageId = suiteId;
        invoc.classname = classname;

	return get(invoc, MODE_RESPONSE, shouldBlock);
    }

    /**
     * Performs cleanup for a ContentHandler
     * by suiteId and classname.
     * <p>
     * Any marked {@link #setCleanup} invocations still in the queue
     * are handled based on status:
     * <UL>
     * <li>ACTIVE Invocations are returned from this method
     *    so they can be have the ERROR status set and so the
     *    invoking application relaunched.</li>
     * <li>INIT Invocations are requeued to the invoking application
     *    with ERROR status. </li>
     * <li>OK, CANCELLED, ERROR, or INITIATED Invocations are
     *    discrded.</li>
     * <li>HOLD status Invocations are retained pending
     *    completion of previous Invocation.  TBD: Chained HOLDs...</li>
     * </ul>
     *
     * @param suiteId the MIDletSuite ID
     * @param classname the classname
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getCleanup(int suiteId, String classname) {
        InvocationImpl invoc = new InvocationImpl();
        invoc.storageId = suiteId;
        invoc.classname = classname;

	return get(invoc, MODE_CLEANUP, false);
    }

    /**
     * Get an Invocation from the store based on its <code>tid</code>.
     * The normal state transitions and dispositions are NOT performed.
     * If TID == 0 then the first tid is used as the reference.
     * If TID == 0 and relative == 0 then null is returned.
     * This method never waits.
     *
     * @param tid the <code>tid</code> to fetch
     * @param relative -1, 0, +1 to get previous, equal, or next
     * @return an InvocationImpl object if a matching tid was found;
     *  otherwise <code>null</code>
     */
    static InvocationImpl getByTid(int tid, int relative) {
        InvocationImpl invoc = new InvocationImpl();
        int mode = MODE_TID;
        if (tid != 0) {
            if (relative < 0) {
                mode = MODE_TID_PREV;
            } else if (relative > 0) {
                mode = MODE_TID_NEXT;
            }
        }
        invoc.storageId = MIDletSuite.UNUSED_SUITE_ID;
        invoc.classname = null;
        invoc.tid = tid;
        return get(invoc, mode, false);
    }

    /**
     * Get an InvocationImpl from the store using a MIDlet suiteId
     * and classname.
     * The mode controls whether getting an Invocation
     * from the store removes it from the store.
     *
     * @param invoc InvocationImpl to fill in with result
     * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
     *    or {@link #MODE_CLEANUP}, {@link #MODE_LREQUEST},
     *    or {@link #MODE_LRESPONSE}, {@link #MODE_TID}.
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname if one was requested;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    private static InvocationImpl get(InvocationImpl invoc,
				      int mode, boolean shouldBlock) {
	String classname = invoc.classname;
	invoc.setArgs(null);
	invoc.setData(null);

	int s = 0;
	int oldCancelCount = cancelCount;
	while ((s = get0(invoc, invoc.storageId, invoc.classname,
			 mode, shouldBlock)) != 1) {
	    if (s == -1) {
		/*
		 * Sizes of arguments and data buffers were insufficient
		 * reallocate and retry.
		 */
		invoc.setArgs(new String[invoc.argsLen]);
		invoc.setData(new byte[invoc.dataLen]);
		continue;
	    }
	    // Don't wait unless requested
	    if (!shouldBlock) {
		break;
	    }
	    // No matching request; retry unless cancelled
	    if (cancelCount > oldCancelCount) {
		// Was cancelled; s == 0 -> no Invocation
		break;
	    }
	}

	// Update the return if no invocation
	if (s == 0) {
	    invoc = null;
	}

	if (AppProxy.LOG_INFO) {
	    AppProxy.getCurrent().logInfo("Store get: " +
					  classname +
					  ", mode: " + mode +
					  ", " + invoc);
	}
        return invoc;
    }

    /**
     * Sets the status of an existing Invocation.
     * If the status is OK, CANCELLED, ERROR, or INITIATED
     * and a response is required then the invocation is
     * requeued to the invoking application; if no response
     * is required the request is discarded and the transaction id (tid)
     * is set to zero.
     *
     * @param invoc an InvocationImpl previously retrieved with get
     */
    static void setStatus(InvocationImpl invoc) {
	setStatus0(invoc);
	if (AppProxy.LOG_INFO) {
	    AppProxy.getCurrent().logInfo("Store setStatus0: " + invoc);
	}
    }

    /**
     * Updates the parameters of the invocation in the native store.
     * The ID, URL, type, action, arguments, and data are
     * stored again in native.
     *
     * @param invoc an InvocationImpl previously retrieved with get
     */
    static void setParams(InvocationImpl invoc) {
	setParams0(invoc);
	if (AppProxy.LOG_INFO) {
	    AppProxy.getCurrent().logInfo("Store setParams0: " + invoc);
	}
    }

    /**
     * Listen for a matching invocation.
     * When a matching invocation is present, true is returned.
     * Each Invocation instance is only returned once.
     * After it has been returned once; it is ignored subsequently.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param request true to listen for a request; else a response
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return true if a matching invocation is present; false otherwise
     */
    static boolean listen(int suiteId, String classname,
                          boolean request, boolean shouldBlock) {
        if (suiteId == MIDletSuite.UNUSED_SUITE_ID || classname == null) {
            throw new NullPointerException();
        }
        int mode = (request ? MODE_LREQUEST : MODE_LRESPONSE);
        boolean pending = false;

        int oldCancelCount = cancelCount;
        while ((pending = listen0(suiteId, classname,
                                  mode, shouldBlock)) == false &&
                                  shouldBlock) {
            // No pending request; retry unless cancelled
            if (cancelCount > oldCancelCount) {
                // Was cancelled; s == 0 -> no Invocation
                break;
            }
        }

        if (AppProxy.LOG_INFO) {
            AppProxy.getCurrent().logInfo("Store listen: " + classname +
                                          ", request: " + request +
                                          ", pending: " + pending);
        }
        return pending;
    }

    /**
     * Reset the flags for requests or responses that are pending.
     * Once reset, any pending requests or responses will be
     * returned when listen0 is called.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param request true to reset request notification flags;
     *   else reset response notification flags
     */
    static void setListenNotify(int suiteId, String classname,
                                boolean request) {
        if (suiteId == MIDletSuite.UNUSED_SUITE_ID || classname == null) {
            throw new NullPointerException();
        }

        int mode = (request ? MODE_LREQUEST : MODE_LRESPONSE);
        setListenNotify0(suiteId, classname, mode);

        if (AppProxy.LOG_INFO) {
            AppProxy.getCurrent().logInfo("Store setListenNotify: " +
                                          classname +
                                          ", request: " + request);
        }
    }

    /**
     * Cancel a blocked {@link #get}  or {@link #listen}
     * method if it is blocked in the native code.
     */
    static void cancel() {
	cancelCount++;
	cancel0();
    }

    /**
     * Marks any existing invocations for the content handler.
     * Any marked invocation will be modified by {@link #getCleanup}.
     *
     * @param suiteId the suite to mark
     * @param classname the MIDlet within the suite
     * @param cleanup <code>true</code> to mark the Invocation for
     *   cleanup at exit
     */

    public static void setCleanup(int suiteId, String classname, boolean cleanup) {
        if (AppProxy.LOG_INFO) {
            AppProxy.getCurrent().logInfo("Store setCleanup: " + classname +
                                          ": " + cleanup);
        }
        setCleanup0(suiteId, classname, cleanup);
    }

    /**
     * Return the number of invocations in the native queue.
     * @return the number of invocations in the native queue
     */
    static int size() {
        return size0();
    }

    /**
     * Native method to store a new Invocation.
     * All of the fields of the InvocationImpl are stored.
     * @param invoc the InvocationImpl to store
    */
    private static native void put0(InvocationImpl invoc);

    /**
     * Native method to fill an available InvocationImpl with an
     * available stored Invocation with the status (if non-zero),
     * the suiteId, classname in the prototype InvocationImpl.
     * Any InvocationImpl with a matching status, suite and
     * class will be returned.
     * Depending on the mode the stored invocation will be removed
     * from the store.
     * @param invoc the Invocation containing the suiteId and
     *  classname to fill in with an available invocation.
     * @param suiteId the MIDletSuite ID to match
     * @param classname the classname to match
     * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
     *    or {@link #MODE_CLEANUP}
     * @param shouldBlock True if the method should block until an
     *    Invocation is available
     * @return 1 if a matching invocation was found and returned
     *    in its entirety; zero if there was no matching invocation;
     *    -1 if the sizes of the arguments or parameter array were wrong
     * @see #get
     */
    private static native int get0(InvocationImpl invoc,
                                   int suiteId,
                                   String classname,
                                   int mode,
                                   boolean shouldBlock);

    /**
     * Sets the status of an existing Invocation
     * and handles response required behavior.
     *
     * @param invoc an InvocationImpl previously retrieved with get.
     */
    private static native void setStatus0(InvocationImpl invoc);

    /**
     * Updates the parameters of the invocation in the native store.
     * The ID, URL, type, action, arguments, and data are
     * stored again in native.
     *
     * @param invoc an InvocationImpl previously retrieved with get
     */
    private static native void setParams0(InvocationImpl invoc);

    /**
     * Native method to listen for pending invocations with
     * matching suite, classname, and status. Cancel() will
     * also cause this method to return if blocked.
     * Each Invocation will only be returned once to prevent
     * multiple notifications.
     *
     * @param suiteId the MIDletSuite ID to match
     * @param classname the classname to match
     * @param mode one of {@link #MODE_LREQUEST}, {@link #MODE_LRESPONSE}
     * @param shouldBlock true if the method should block until an
     *     Invocation is available
     * @return true if a matching invocation was found; otherwise false.
     * @see #get0
     */
    private static native boolean listen0(int suiteId, String classname,
                                          int mode, boolean shouldBlock);

    /**
     * Native method to reset the listen notified state for pending
     * invocations with matching suite, classname and status.
     * Each Invocation will only be returned once to prevent
     * multiple notifications.
     *
     * @param suiteId the MIDletSuite ID to match
     * @param classname the classname to match
     * @param mode one of {@link #MODE_LREQUEST}, {@link #MODE_LRESPONSE}
     *   <code>false</code> to reset the notified state for responses
     * @see #listen0
     */
    private static native void setListenNotify0(int suiteId, String classname,
                                                int mode);

    /**
     * Native method to unblock any threads that might be
     * waiting for an invocation by way of having called
     * {@link #get0}.
     *
     */
    private static native void cancel0();

    /**
     * Sets the cleanup flag in matching Invocations.
     * Any marked invocation will be modified by {@link #getCleanup}.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param cleanup <code>true</code> to mark the Invocation for
     *  cleanup at exit
     */
    private static native void setCleanup0(int suiteId, String classname,
                                           boolean cleanup);

    /**
     * Return the number of invocations in the native queue.
     * @return the number of invocations in the native queue
     */
    private static native int size0();
}
