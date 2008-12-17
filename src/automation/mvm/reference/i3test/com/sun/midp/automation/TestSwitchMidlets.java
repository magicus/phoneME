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

package com.sun.midp.automation;

import com.sun.midp.i3test.*;
import java.util.*;

/**
 * i3test for switching running midlets functionality
 */
public class TestSwitchMidlets extends TestCase {
    /** URL of the first suite to install */
    private static final String SUITE1_URL =
        "http://localhost/~leonid/Hello.jad";

    /** URL of the second suite to install */
    private static final String SUITE2_URL =
        "http://localhost/~leonid/TextInput.jad";

    /** Midlet suite storage */
    private AutoSuiteStorage storage = null;

    /** Descriptor of the 1st midlet suite */
    private AutoSuiteDescriptor suite1 = null;

    /** Descriptor of the 2nd midlet suite */
    private AutoSuiteDescriptor suite2 = null;

    /** Descriptor of the 1st midlet */
    private AutoMIDletDescriptor midletDescr1 = null;

    /** Descriptor of the 2nd midlet */
    private AutoMIDletDescriptor midletDescr2 = null;

    /**
     * Installs the test suites.
     */
    void installTestSuites() {
        Automation a = Automation.getInstance();

        declare("Install suites");
        storage = a.getStorage();

        try {
            suite1 = storage.installSuite(SUITE1_URL);
            suite2 = storage.installSuite(SUITE2_URL);
        } catch (Exception e) {
        }

        assertNotNull("Failed to install suite 1", suite1);
        assertNotNull("Failed to install suite 2", suite2);

        System.out.println("Suite 1 name: " + suite1.getSuiteName());
        System.out.println("Suite 2 name: " + suite2.getSuiteName());

        Vector midlets1 = suite1.getSuiteMIDlets();
        Vector midlets2 = suite2.getSuiteMIDlets();

        midletDescr1 = (AutoMIDletDescriptor)midlets1.elementAt(0);
        midletDescr2 = (AutoMIDletDescriptor)midlets2.elementAt(0);
    }

    /**
     * Uninstalls the test suites.
     */
    void uninstallTestSuites() {
        declare("Uninstall suites");
        boolean exceptionThrown = false;
        try {
            storage.uninstallSuite(suite1);
            storage.uninstallSuite(suite2);
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertFalse("Failed to uninstall suites", exceptionThrown);
    }

    /**
     * Tests midlets switching
     */
    void testSwitchMidlets() {
        installTestSuites();

        declare("Run suites");
        AutoMIDlet midlet1 = midletDescr1.start(null);
        AutoMIDlet midlet2 = midletDescr2.start(null);

        // Initial state is 'PAUSED'.
        midlet1.switchTo(AutoMIDletLifeCycleState.ACTIVE, true);
        midlet2.switchTo(AutoMIDletLifeCycleState.ACTIVE, true);

        assertTrue("Invalid state of the midlet #1.", midlet1.
            getLifeCycleState().equals(AutoMIDletLifeCycleState.ACTIVE));
        assertTrue("Invalid state of the midlet #2.", midlet2.
            getLifeCycleState().equals(AutoMIDletLifeCycleState.ACTIVE));

        midlet1.switchTo(AutoMIDletForegroundState.FOREGROUND, true);
        midlet2.switchTo(AutoMIDletForegroundState.BACKGROUND, true);

        try {
            Thread.sleep(2000);
        } catch (InterruptedException ie) {
        }

        midlet2.switchTo(AutoMIDletForegroundState.FOREGROUND, true);

        try {
            Thread.sleep(2000);
        } catch (InterruptedException ie) {
        }

        midlet1.switchTo(AutoMIDletLifeCycleState.DESTROYED, true);
        midlet2.switchTo(AutoMIDletLifeCycleState.DESTROYED, true);

        uninstallTestSuites();
    }

    /**
     * Run tests
     */
    public void runTests() {
        testSwitchMidlets();
    }
}
