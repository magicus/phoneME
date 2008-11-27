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

package com.sun.midp.appmanager;

import com.sun.midp.i3test.TestCase;

import com.sun.midp.main.*;
import com.sun.midp.midlet.MIDletSuite;

public class TestUserNotification extends TestCase implements
    MIDletProxyListListener {

    private static final String DUMMY_MIDLET1_CLASS_NAME =
        "com.sun.midp.appmanager.DummyMIDlet1";
    private static final String DUMMY_MIDLET2_CLASS_NAME =
        "com.sun.midp.appmanager.DummyMIDlet2";

    private MIDletProxyList proxyList;
    private MIDletProxy midlet1, midlet2;
    private boolean midlet1InForeground;
    private boolean midlet2InForeground;

    private void setUp() {
        proxyList = MIDletProxyList.getMIDletProxyList();
        proxyList.addListener(this);

        IndicatorManager.init(proxyList);
    }

    private void startMIDlet1() {
        try {
            // Start a new instance of DummyMIDlet1
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                DUMMY_MIDLET1_CLASS_NAME, "DummyMIDlet1");

            // Wait for async request to be processed
            synchronized (this) {
                if (midlet1 == null) {
                    // We only wait the full time on a failure
                    wait(10000);
                }
            }

            assertTrue(DUMMY_MIDLET1_CLASS_NAME + " not started",
                       midlet1 != null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void startMIDlet2() {
        try {
            // Start a new instance of DummyMIDlet2
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                DUMMY_MIDLET2_CLASS_NAME, "DummyMIDlet2");

            // Wait for async request to be processed
            synchronized (this) {
                if (midlet2 == null) {
                    // We only wait the full time on a failure
                    wait(10000);
                }
            }

            assertTrue(DUMMY_MIDLET2_CLASS_NAME + " not started",
                       midlet2 != null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void test1() {
        declare("One");
        // Launch MIDlet 1
        startMIDlet1();

        // Wait for async request to be processed
        // Headless MIDlet also occupy foreground
        synchronized (midlet1) {
            if (!midlet1InForeground) {
                waitForMIDletToGetForeground(midlet1);
            }

            assertTrue(DUMMY_MIDLET1_CLASS_NAME + " not in foreground",
                       midlet1InForeground);
        }

        /*
         * Launching a headless MIDlet shouldn't trigger UNS
         */
        assertTrue(IndicatorManager.getHomeIconState() == false);
    }

    private void test3() {
        declare("Three");

	// Launch MIDlet 2
	startMIDlet2();

	/*
         * MIDlet 2 will automatically get foreground when it requests it
         * in startApp and MIDlet 1 will be put in background
         */

	// Wait for async request to be processed
        synchronized (midlet2) {
            if (!midlet2InForeground) {
                waitForMIDletToGetForeground(midlet2);
            }

            assertTrue(DUMMY_MIDLET2_CLASS_NAME + " not in foreground",
                       midlet2InForeground);
        }

	// Merely moving MIDlet 1 to background shouldn't trigger home icon
	assertTrue(IndicatorManager.getHomeIconState() == false);
    }

    private void test4() {
        declare("Four");

	/*
         * Force MIDlet 1 in the background to request foreground,
         * by getting startApp to be called a third time.
         */
	midlet1.pauseMidlet();

        // Give pauseApp time to get called before activating
        try {
            Thread.sleep(100);
        } catch (InterruptedException ie) {
            // ignore
        }

        midlet1.activateMidlet();

        // No foreground change so just wait 1 sec.
        try {
            Thread.sleep(1000);
        } catch (InterruptedException ie) {
            // ignore
        }

	// Home icon should be on upon background display foreground request
	assertTrue(IndicatorManager.getHomeIconState() == true);
    }

    private void test5() {
        declare("Five");

	// Background Display requests for background
	midlet1.pauseMidlet();

        // The test manager may get the background so just wait 1 sec.
        try {
            Thread.sleep(1000);
        } catch (InterruptedException ie) {
            // ignore
        }

	// previous home icon request should be cancelled
	assertTrue(IndicatorManager.getHomeIconState() == false);
    }

    private void tearDown() {
        proxyList.removeListener(this);

        if (midlet1 != null) {
            midlet1.destroyMidlet();
        }

        if (midlet2 != null) {
            midlet2.destroyMidlet();
        }
    }

    private void waitForMIDletToGetForeground(MIDletProxy midlet) {
        synchronized (midlet) {
            try {
                // We only wait the full time on a failure
                midlet.wait(10000);
            } catch (InterruptedException ie) {
                // ignore
            }
        }
    }

    public void runTests() {
        setUp();

        try {
            test1();
            // test2() is obsolete
            test3();
            test4();
            test5();
        } finally {
            tearDown();
        }
    }

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        if (DUMMY_MIDLET1_CLASS_NAME.equals(midlet.getClassName())) {
            synchronized (this) {
                midlet1 = midlet;
                notifyAll();
            }

            return;
        }

        if (DUMMY_MIDLET2_CLASS_NAME.equals(midlet.getClassName())) {
            synchronized (this) {
                midlet2 = midlet;
                notifyAll();
            }

            return;
        }
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
        MIDletProxy foreground = proxyList.getForegroundMIDlet();

        if (midlet1 != null && midlet1 == foreground) {
            synchronized (midlet1) {
                midlet2InForeground = false;
                midlet1InForeground = true;
                midlet1.notifyAll();
            }
        } else if (midlet2 != null && midlet2 == foreground) {
            synchronized (midlet2) {
                midlet1InForeground = false;
                midlet2InForeground = true;
                midlet2.notifyAll();
            }
        } else {
            midlet1InForeground = false;
            midlet2InForeground = false;
        }

    }

    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {}

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
        String className, int errorCode, String errorDetails) {}
}
