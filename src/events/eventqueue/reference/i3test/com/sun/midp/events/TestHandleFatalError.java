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

package com.sun.midp.events;

import com.sun.midp.i3test.*;

import com.sun.midp.main.*;
import com.sun.midp.midlet.MIDletSuite;

/**
 * Test that the if an event listener in an application isolate throws a
 * runtime exception, that the isolate is killed.
 */
public class TestHandleFatalError extends TestCase
    implements MIDletProxyListListener {

    /** Timeout period. In milliseconds. */
    private static final int TIMEOUT_PERIOD = 10000; // 10 sec

    /** Class name of the test MIDlet. */
    private static final String TEST_MIDLET =
        "com.sun.midp.events.FatalMIDlet";

    /** Holds a reference to the test MIDlet. */
    private MIDletProxy testMidlet;

    /** Holds a reference to the MIDlet proxy list. */
    private MIDletProxyList proxyList;

    /**
     * True,
     * if the test MIDlet was removed from the list of running MIDlets.
     */
    private boolean testMidletRemovedFromList;

    /**
     * Launch the test MIDlet and it it ends by the end of the time out period
     * than consider the test passed.
     */
    void testMvmHandleFatalError() {
        boolean assertion;

        proxyList = MIDletProxyList.getMIDletProxyList();

        proxyList.addListener(this);

        if (MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                TEST_MIDLET, "no name")) {
            // This is SVM mode, so cancel the execute and end the test.
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                null, null);
            return;
        }

        synchronized (this) {
            try {
                wait(TIMEOUT_PERIOD);
            } catch (InterruptedException ie) {
            }
        }

        assertion = testMidlet != null;
        assertTrue(TEST_MIDLET + " not started", assertion);
        if (!assertion) {
            return;
        }

        if (!testMidletRemovedFromList) {
            fail(TEST_MIDLET + " not removed from list of running MIDlets");
            testMidlet.destroyMidlet();
            return;
        }
    }

    /** Run all tests. */
    public void runTests() {
        declare("testMvmHandleFatalError");
        testMvmHandleFatalError();

        proxyList.removeListener(this);
    }

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        if (TEST_MIDLET.equals(midlet.getClassName())) {
            testMidlet = midlet;
            return;
        }
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {}

    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        if (midlet == testMidlet) {
            synchronized (this) {
                testMidletRemovedFromList = true;
                notifyAll();
            }
        }
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error code
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int errorCode,
                                 String errorDetails) {}
}

