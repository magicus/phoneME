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

package com.sun.midp.installer;

import java.io.*;

import java.util.*;

import com.sun.cldc.isolate.*;

import com.sun.midp.main.AmsUtil;

import javax.microedition.io.*;

import javax.microedition.lcdui.*;

import javax.microedition.midlet.*;

import javax.microedition.rms.*;

import com.sun.midp.io.j2me.storage.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.midlet.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;
import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Implements auto testing multi functionality.
 */
final class AutoTesterMultiHelper extends AutoTesterHelperBase {
    /** AutoTesterMulti instance for displaying error messages */
    private AutoTesterMulti autoTester;

    /**
     * Info about suites to install
     */
    private Vector installList;

    /**
     * Constructor.
     *
     * @param theAutoTester AutoTesterMulti instance
     * @param theURL URL of the test suite
     * @param theDomain security domain to assign to unsigned suites
     * @param theLoopCount how many iterations to run the suite
     */
    AutoTesterMultiHelper(AutoTesterMulti theAutoTester, 
            String theURL, String theDomain, int theLoopCount) {

        super(theURL, theDomain, theLoopCount);

        autoTester = theAutoTester;
        installList = new Vector();
    }

    /**
     * Gets list of test suites, for each suite starts a thread
     * that performs testing, waits until all threads have finished.
     */    
    void installAndPerformTests() {
        fetchInstallList(url);
        int totalSuites = installList.size();
        if (totalSuites > 0) {
            Thread[] threads = new Thread[totalSuites];

            // create threads
            for (int i = 0; i < totalSuites; i++) {
                SuiteDownloadInfo suite =
                    (SuiteDownloadInfo)installList.elementAt(i);
                threads[i] = new Thread(new AutoTesterRunner(suite.url));
            }

            // start threads
            for (int i = 0; i < totalSuites; i++) {
                threads[i].start();
            }

            // wait for threads to finish
            for (int i = 0; i < totalSuites; i++) {
                try {
                    threads[i].join();
                } catch (Exception ex) {
                    // just ignore
                }
            }
        }        
    }

    /**
     * Go to given URL, fetch and parse html page with links
     * to tests suites. If there was error while fetching
     * or parsing, display an alert.
     *
     * @param url URL for html page with links to suites
     */
    private void fetchInstallList(String url) {
        StreamConnection conn = null;
        InputStreamReader in = null;
        String errorMessage;

        try {
            conn = (StreamConnection)Connector.open(url, Connector.READ);
            in = new InputStreamReader(conn.openInputStream());
            try {
                installList = SuiteDownloadInfo.getDownloadInfoFromPage(in);
                if (installList.size() > 0) {
                    return;
                }
                errorMessage = Resource.getString(
                        ResourceConstants.AMS_DISC_APP_CHECK_URL_MSG);
            } catch (IllegalArgumentException ex) {
                errorMessage = Resource.getString(
                        ResourceConstants.AMS_DISC_APP_URL_FORMAT_MSG);
            } catch (Exception ex) {
                errorMessage = ex.getMessage();
            }
        } catch (Exception ex) {
            errorMessage = Resource.getString(
                    ResourceConstants.AMS_DISC_APP_CONN_FAILED_MSG);
        } finally {
            try {
                conn.close();
                in.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                            "close threw an Exception");
                }
            }
        }

        autoTester.displayFetchInstallListError(errorMessage);
    }

    /**
     * An runnable class that runs specified test suite in a loop.
     */
    private class AutoTesterRunner implements Runnable {
        /**
         * Number of retries to fetch a suite from given URL before exiting
         */
        private final static int MAX_RETRIES = 20;

        /**
         * Time to wait before trying to fetch a suite again
         */
        private final static int RETRY_WAIT_TIME = 30000;

        /**
         * URL for the test suite
         */
        private String url;


        /**
         * Constructor.
         *
         * @param theURL URL for the test suite
         */
        private AutoTesterRunner(String theURL) {
            url = theURL;
        }

        /**
         * In a loop, install/update the suite and run it until
         * new version of the suite is not found.
         */
        public void run() {
            // when installing suite for the first time,
            // do not retry because URL may be wrong and
            // we want immediately tell user about it.
            boolean retry = false;
            boolean hasSuite = true;

            int suiteId = MIDletSuite.UNUSED_SUITE_ID;
            int lastSuiteId = MIDletSuite.UNUSED_SUITE_ID;

            MIDletInfo midletInfo;
            Isolate testIsolate;

            for (; loopCount != 0 && hasSuite; ) {
                suiteId = installSuite(retry);
                if (suiteId != MIDletSuite.UNUSED_SUITE_ID) {
                    midletInfo = getFirstMIDletOfSuite(suiteId);
                    testIsolate = AmsUtil.startMidletInNewIsolate(suiteId,
                            midletInfo.classname, midletInfo.name, null,
                            null, null);

                    testIsolate.waitForExit();

                    if (loopCount > 0) {
                        loopCount -= 1;
                    }

                    lastSuiteId = suiteId;

                    // suite has been found, so URL isn't wrong.
                    // next time if there is no suite, do retries.
                    retry = true;
                } else {
                    hasSuite = false;
                }
            }

            if (midletSuiteStorage != null &&
                    lastSuiteId != MIDletSuite.UNUSED_SUITE_ID) {
                try {
                    midletSuiteStorage.remove(lastSuiteId);
                } catch (Throwable ex) {
                    // ignore
                }
            }
        }



        /**
         * Install the suite.
         *
         * @param retry if true, do a number of retries to
         * install a suite in case it hasn't been found.
         *
         * @return suiteId if the suite has been installed
         */
        private int installSuite(boolean retry) {
            int maxRetries = retry? MAX_RETRIES : 1;
            int retryCount = 0;
            int suiteId = MIDletSuite.UNUSED_SUITE_ID;

            for (; retryCount < maxRetries; ++retryCount) {
                try {
                    synchronized (installer) {
                        // force an overwrite and remove the RMS data
                        suiteId = installer.installJad(url,
                            Constants.INTERNAL_STORAGE_ID, true,
                                true, null);
                    }
                    return suiteId;

                } catch (Throwable t) {
                    // if retrying, just ignore exception and wait,
                    // otherwise display error message to user.
                    if (retry) {
                        try {
                            Thread.sleep(RETRY_WAIT_TIME);
                        } catch (Exception ex) {
                        }
                    } else {
                        String message = getInstallerExceptionMessage(
                                suiteId, t);
                        if (message != null) {
                            autoTester.displayInstallerError(message);
                        }
                    }
                }
            }

            return MIDletSuite.UNUSED_SUITE_ID;
        }
    }
}
