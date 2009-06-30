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

package com.sun.midp.scriptutil;

import javax.microedition.midlet.MIDlet;

import com.sun.midp.midletsuite.*;

import com.sun.midp.midlet.MIDletSuite;

/**
 * List all installed suites.
 */
public class SuiteLister extends MIDlet implements Runnable {
    /**
     * Create and initialize the MIDlet.
     */
    public SuiteLister() {
        new Thread(this).start();
    }

    /**
     * Start.
     */
    public void startApp() {
    }

    /**
     * Pause; there are no resources that need to be released.
     */
    public void pauseApp() {
    }

    /**
     * Destroy cleans up.
     *
     * @param unconditional is ignored; this object always
     * destroys itself when requested.
     */
    public void destroyApp(boolean unconditional) {
    }

    /** Remove a MIDlet suite. */
    public void run() {
        try {
            MIDletSuiteStorage storage =
                MIDletSuiteStorage.getMIDletSuiteStorage();

            int[] suiteIds = storage.getListOfSuites();

            if (suiteIds.length == 0) {
                System.out.println("No suites installed.");
                notifyDestroyed();
                return;
            }

            for (int i = 0; i < suiteIds.length; i++) {
                MIDletSuite suite = storage.getMIDletSuite(suiteIds[i], false);
                String value;

                System.out.println("Suite: " + suiteIds[i]);

                value = suite.getProperty("MIDlet-Name");
                System.out.println("  Name: " + value);

                value = suite.getProperty("MIDlet-Version");
                System.out.println("  Version: " + value);

                value = suite.getProperty("MIDlet-Vendor");
                System.out.println("  Vendor: " + value);

                value = suite.getProperty("MIDlet-Description");
                if (value != null) {
                    System.out.println("  Description: " + value);
                }

                for (int j = 1; ; j++) {
                    value = suite.getProperty("MIDlet-" + j);
                    if (value == null) {
                        break;
                    }

                    if (j == 1) {
                        System.out.println("  MIDlets:");
                    }

                    MIDletInfo midlet = new MIDletInfo(value);
                    System.out.println("    " + midlet.name +
                                       ": " + midlet.classname);
                }
            }
        } catch (MIDletSuiteLockedException msle) {
            System.err.println("Error: MIDlet suite is locked");
        } catch (Throwable t) {
            System.err.println("Error listing suites.");
            t.printStackTrace();
        }

        notifyDestroyed();
    }
}
