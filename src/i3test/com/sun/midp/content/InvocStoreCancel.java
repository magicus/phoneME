/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.content;

import com.sun.midp.i3test.TestCase;

import javax.microedition.content.Invocation;

import com.sun.midp.content.InvocationStore;
import com.sun.midp.content.InvocationImpl;

/**
 * A test thread to test cancel and get interactions.
 * The test strategy is to block for an Invocation
 * and then see.
 * Did the cancel unblock the request?
 */
class InvocStoreCancel implements Runnable {

    /** The number of gets attempted. */
    public int numTry;
    /** The number of null results. */
    public int numNotPending;
    /** The number of non-null results. */
    public int numPending;
    /** The AppProxy to monitor. */
    AppProxy appl;

    /** true to stop processing. */
    private boolean stopping;

    /** True to block using get; false to block using select. */
    private boolean type;

    /**
     * Construct a new context.
     * @param type true to blocking using get; false to use listen.
     */
    InvocStoreCancel(boolean type, AppProxy appl) {
	this.type = type;
	this.appl = appl;
    }

    /**
     * Reset the counters.
     */
    void reset() {
	numTry = 0;
	numNotPending = 0;
	numPending = 0;
    }

    /**
     * Check the counters and return the level.
     * @return the number of transactions.
     */
    public int check() {
	if (numNotPending + numPending != numTry) {
	    throw new RuntimeException("inconsistent counters");
	}
	return numTry;
    }

    /**
     * Loop getting Invocations in the standard form.
     * Keep track of how many are received that non-null
     * and null.
     */
    public void run() {
	if (type) {
	    blockingGet();
	} else {
	    blockingListen();
	}
    }

    /**
     * Run the test for blocking get and cancel.
     */
    private void blockingGet() {
	while (!stopping) {
	    numTry++;
	    InvocationImpl get =
		InvocationStore.getResponse(new InvocationImpl(),
					    appl.getStorageId(), appl.getClassname(),
					    true);
	    if (get == null) {
		numNotPending++;
	    } else {
		numPending++;
	    }
	}
    }

    /**
     * Run the test for blocking listen and cancel.
     */
    private void blockingListen() {
	while (!stopping) {
	    numTry++;
	    boolean pending = InvocationStore.listen(appl.getStorageId(),
						     appl.getClassname(),
						     false, true);
	    if (pending) {
		numPending++;
	    } else {
		numNotPending++;
	    }
	}
    }


    /**
     * Called to stop this thread.
     */
    void stop() {
	stopping = true;
    }
}
