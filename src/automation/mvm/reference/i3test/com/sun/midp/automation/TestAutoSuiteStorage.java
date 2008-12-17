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
 * i3test for AutoSuiteStorage 
 */
public class TestAutoSuiteStorage extends TestCase {
    /** URL of unexistent suite */
    private static final String NONEXISTENT_SUITE_URL = 
        "http://dead.end/nosuchmidlet.jad";

    /** URL of suite to install */
    private static final String SUITE_URL = 
        "http://localhost/~leonid/Hello.jad";
    
    /**
     * Test failures 
     */
    void testFailures() {
        boolean exceptionThrown;
        
        declare("Obtain Automation instance");
        Automation a = Automation.getInstance();
        assertNotNull("Failed to obtain Automation instance", a);

        declare("Obtain AutoSuiteStorage instance");
        AutoSuiteStorage storage = a.getStorage();
        assertNotNull("Failed to obtain AutoSuiteStorage instance", storage);

        declare("Install nonexistent suite");
        exceptionThrown = false;
        try {
            AutoSuiteDescriptor suite = null;
            suite = storage.installSuite(NONEXISTENT_SUITE_URL);
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertTrue("Installed nonexistent suite", exceptionThrown);

        declare("Install suite");
        AutoSuiteDescriptor suite = null;
        try {
            suite = storage.installSuite(SUITE_URL);
        } catch (Exception e) {
            e.printStackTrace();
        }
        assertNotNull("Failed to install suite", suite);


        declare("Uninstall suite");
        exceptionThrown = false;
        try {
            storage.uninstallSuite(suite);
        } catch (Exception e) {
            e.printStackTrace();
            exceptionThrown = true;
        }
        assertFalse("Failed to uninstall suite", exceptionThrown);

        declare("Uninstall not installed suite");
        exceptionThrown = false;
        try {
            storage.uninstallSuite(suite);
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertTrue("Uninstalled not installed suite", exceptionThrown);
    }
    
    /**
     * Tests operations with storage
     */
    void testStorage() {
        declare("Obtain Automation instance");
        Automation a = Automation.getInstance();
        assertNotNull("Failed to obtain Automation instance", a);

        declare("Obtain AutoSuiteStorage instance");
        AutoSuiteStorage storage = a.getStorage();
        assertNotNull("Failed to obtain AutoSuiteStorage instance", storage);

        Vector suites = storage.getInstalledSuites();
        assertNotNull("Failed to obtain list of installed suites", suites);

        int totalSuitesBefore = suites.size();

        declare("Install suite");
        AutoSuiteDescriptor suite = null;
        try {
            suite = storage.installSuite(SUITE_URL);
        } catch (Exception e) {
        }
        assertNotNull("Failed to install suite", suite);

        suites = storage.getInstalledSuites();
        int totalSuitesAfter = suites.size();
        assertTrue("Invalid number of installed suites", 
                totalSuitesAfter == totalSuitesBefore + 1);

        System.out.println("Suite name: " + suite.getSuiteName());
        Vector midlets = suite.getSuiteMIDlets();
        for (int i = 0; i < midlets.size(); ++i) {
            AutoMIDletDescriptor midlet;
            midlet = (AutoMIDletDescriptor)midlets.elementAt(i);
            System.out.println("MIDlet #" + i);
            System.out.println("  name : " + midlet.getMIDletName());
            System.out.println("  class: " + midlet.getMIDletClassName());
            System.out.println("");
        }
        
        declare("Uninstall suite");
        boolean exceptionThrown = false;
        try {
            storage.uninstallSuite(suite);
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertFalse("Failed to uninstall suite", exceptionThrown);

        suites = storage.getInstalledSuites();
        totalSuitesAfter = suites.size();
        assertTrue("Invalid number of installed suites", 
                totalSuitesAfter == totalSuitesBefore);
    }

    /**
     * Run tests
     */
    public void runTests() {
        testStorage();
        testFailures();
    }
    
}
