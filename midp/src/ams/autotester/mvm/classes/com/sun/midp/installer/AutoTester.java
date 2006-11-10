/*
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

package com.sun.midp.installer;

import com.sun.cldc.isolate.*;

import com.sun.midp.i18n.Resource;

import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.AmsUtil;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

/**
 * Installs/Updates a test suite, runs the first MIDlet in the suite in a loop
 * specified number of iterations or until the new version of the suite is not
 * found, then removes the suite.
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for the test suite
 *   <li>arg-1: Used to override the default domain used when installing
 *    an unsigned suite. The default is maximum to allow the runtime API tests
 *    be performed automatically without tester interaction.
 *    <li>arg-2: Integer number, specifying how many iterations to run
 *    the suite. If argument is not given or less then zero, then suite
 *    will be run until the new version of the suite is not found.
 * </ol>
 * <p>
 * If arg-0 is not given then a form will be used to query the tester for
 * the arguments.</p>
 */
public class AutoTester extends AutoTesterBase implements AutoTesterInterface {

    /**
     * Create and initialize a new auto tester MIDlet.
     */
    public AutoTester() {
        super();

        if (url != null) {
            startBackgroundTester();
        } else if (restoreSession()) {
            // continuation of a previous session
            startBackgroundTester();
        } else {
            /**
             * No URL has been provided, ask the user.
             *
             * commandAction will subsequently call startBackgroundTester.
             */
            getUrl();
        }
    }

    /** Run the installer. */
    public void run() {
        installAndPerformTests(midletSuiteStorage, installer, url);
    }

    /**
     * Restore the data from the last session, since this version of the
     * autotester does not have sessions it just returns false.
     *
     * @return true if there was data saved from the last session
     */
    public boolean restoreSession() {
        return false;
    }

    /**
     * Installs and performs the tests.
     *
     * @param midletSuiteStorage MIDletSuiteStorage object
     * @param inp_installer Installer object
     * @param inp_url URL of the test suite
     */
    public void installAndPerformTests(
        MIDletSuiteStorage midletSuiteStorage,
        Installer inp_installer, String inp_url) {

        MIDletInfo midletInfo;
        String suiteID = null;

        try {
            Isolate testIsolate;

            for (; loopCount != 0; ) {
                // force an overwrite and remove the RMS data
                suiteID = inp_installer.installJad(inp_url, true, true,
						   installListener);

                midletInfo = getFirstMIDletOfSuite(suiteID,
                                                   midletSuiteStorage);
                testIsolate =
                    AmsUtil.startMidletInNewIsolate(suiteID,
                        midletInfo.classname, midletInfo.name, null,
                        null, null);

                testIsolate.waitForExit();

                if (loopCount > 0) {
                    loopCount -= 1;
                }
            }
        } catch (Throwable t) {
            handleInstallerException(suiteID, t);
        }

        if (midletSuiteStorage != null && suiteID != null) {
            try {
                midletSuiteStorage.remove(suiteID);
            } catch (Throwable ex) {
                // ignore
            }
        }

        notifyDestroyed();
    }
}
