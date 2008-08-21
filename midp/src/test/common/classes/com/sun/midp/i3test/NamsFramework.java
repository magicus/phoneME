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

package com.sun.midp.i3test;

import javax.microedition.midlet.*;

// import com.sun.midp.log.Logging;
// import com.sun.midp.log.LogChannels;

/**
 * The Integrated Internal Interface (i3) test framework.
 */
public class NamsFramework extends Framework {
    
    /** list of test case to run - is filled manually */
    static String testNames[];
    
    static {
        // Add NAMS test cases here manually ...
        testNames = new String[1];
        testNames[0] = "com.sun.midp.main.TestNamsStartMidlet";
    }
    
    /** 
     * Redefined method from base class that executes test cases.
     * The only difference is that the list of test cases to execute is taken
     * from a different source.
     */
    public void run() {
        if (listmode) {
            for (int i = 0; i < testNames.length; i++) {
                System.out.println(testNames[i]);
            }
        } else if (selftest) {
            SelfTest.run();
        } else if (testClass != null) {
            TestCase.runTestCase(testClass);
            TestCase.report();
        } else {
             runTestCases(testNames);
            TestCase.report();
        }

        notifyDestroyed();
    }
}
