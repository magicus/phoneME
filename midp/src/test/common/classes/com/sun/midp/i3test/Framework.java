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

import java.util.Vector;
import javax.microedition.midlet.*;

/**
 * The Integrated Internal Interface (i3) test framework.
 */
public class Framework extends MIDlet implements Runnable {

    String argv[];
    boolean listmode = false;
    boolean selftest = false;
    boolean verbose = false;
    boolean error = false;
    String testClass = null;
    String filterString = null;
    /** Indicates whether a thread running test cases has been started. */
    private boolean started = false;

    void runTestCases(String[] nameArray) {
        for (int i = 0; i < nameArray.length; i++) {
            TestCase.runTestCase(nameArray[i]);
        }
        TestCase.report();
    }

    void listTestCases(String[] nameArray) {
        for (int i = 0; i < nameArray.length; i++) {
            System.out.println(nameArray[i]);
        }
    }

    String[] doFilter(String[] nameArray, String filter) {
        Vector vec = new Vector();

        for (int i = 0; i < nameArray.length; i++) {
            if (filter == null || (nameArray[i].indexOf(filter) >= 0)) {
                vec.addElement(nameArray[i]);
            }
        }

        String[] ret = new String[vec.size()];
        vec.copyInto(ret);
        return ret;
    }

    public Framework() {
        final int argc = 3;

        argv = new String[argc];
        argv[0] = getAppProperty("arg-0");
        argv[1] = getAppProperty("arg-1");
        argv[2] = getAppProperty("arg-2");

        for (int a = 0; a < argc; a++) {
            if (argv[a] == null) {
                break;
            } else if (argv[a].startsWith("-filter:")) {
                filterString = argv[a].substring(8);
            } else if ("-list".equals(argv[a])) {
                listmode = true;
            } else if ("-selftest".equals(argv[a])) {
                selftest = true;
            } else if ("-verbose".equals(argv[a])) {
                verbose = true;
            } else if (argv[a].startsWith("-")) {
                System.err.print(
                    "usage: i3test [option] [testclass]\n" +
                    "options:\n" +
                    "  -filter:pattern  " +
                            "runs tests whose names contain pattern\n" +
                    "  -list            prints a list of known tests\n" +
                    "  -selftest        runs the framework's self test\n" +
                    "  -verbose         enables verbose output\n" +
                    "Given a testclass argument, runs just that test.\n" +
                    "Without a testclass argument, runs all known tests.\n");
                error = true;
                break;
            } else {
                testClass = argv[a];
            }
        }

        TestCase.setVerbose(verbose);
    }

    public void run() {
        if (listmode) {
            listTestCases(doFilter(Repository.testNames, filterString));
        } else if (selftest) {
            SelfTest.run();
        } else if (testClass != null) {
            runTestCases(new String[] { testClass });
        } else {
            runTestCases(doFilter(Repository.testNames, filterString));
        }

        notifyDestroyed();
    }

    public void startApp() {
        if (error) {
            notifyDestroyed();
        } else if (!started) {
            started = true;
            new Thread(this).start();
        }
    }

    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
    }
}
