/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
import com.sun.midp.main.MIDletSuiteUtils;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;

import com.sun.midp.events.*;

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
 *    be performed automatically without tester interaction. The domain name
 *    may be followed by a colon and a list of permissions that must be allowed
 *    even if they are not listed in the MIDlet-Permissions attribute in the
 *    application descriptor file. Instead of the list a keyword "all" can be
 *    specified indicating that all permissions must be allowed, for example:
 *    operator:all.
 *    <li>arg-2: Integer number, specifying how many iterations to run
 *    the suite. If argument is not given or less then zero, then suite
 *    will be run until the new version of the suite is not found.
 * </ol>
 * <p>
 * If arg-0 is not given then a form will be used to query the tester for
 * the arguments.</p>
 */
public class AutoTester extends AutoTesterBase
        implements AutoTesterInterface, EventListener {
    /** True if all events in our queue were processed. */
    private boolean eventsInQueueProcessed;

    /** Our event queue. */
    EventQueue eventQueue;

    /**
     * Create and initialize a new auto tester MIDlet.
     */
    public AutoTester() {
        super();

        eventQueue = EventQueue.getEventQueue();
        eventQueue.registerEventListener(EventTypes.AUTOTESTER_EVENT, this);

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
        int suiteId = MIDletSuite.UNUSED_SUITE_ID;

        try {
            Isolate testIsolate;

            for (; loopCount != 0; ) {
                // force an overwrite and remove the RMS data
                suiteId = inp_installer.installJad(inp_url,
                    Constants.INTERNAL_STORAGE_ID, true, true, installListener);

                midletInfo = getFirstMIDletOfSuite(suiteId,
                                                   midletSuiteStorage);

                Isolate[] isolatesBefore = Isolate.getIsolates();

                testIsolate =
                    AmsUtil.startMidletInNewIsolate(suiteId,
                        midletInfo.classname, midletInfo.name, null,
                        null, null);

                testIsolate.waitForExit();

                boolean newIsolatesFound;

                do {
                    newIsolatesFound = false;

                    /*
                     * Send an event to ourselves.
                     * Main idea of it is to process all events that are in the
                     * queue at the moment when the test isolate has exited
                     * (because when testing CHAPI there may be requests to
                     * start new isolates). When this event arrives, all events
                     * that were placed in the queue before it are guaranteed
                     * to be processed.
                     */
                    synchronized (this) {
                        eventsInQueueProcessed = false;

                        NativeEvent event = new NativeEvent(
                                EventTypes.AUTOTESTER_EVENT);
                        eventQueue.sendNativeEventToIsolate(event,
                                MIDletSuiteUtils.getIsolateId());

                        // and wait until it arrives
                        do {
                            try {
                                wait();
                            } catch(InterruptedException ie) {
                                // ignore
                            }
                        } while (!eventsInQueueProcessed);
                    }

                    Isolate[] isolatesAfter = Isolate.getIsolates();

                    /*
                     * Wait for termination of all isolates contained in
                     * isolatesAfter[], but not in isolatesBefore[].
                     * This is needed to pass some tests (for example, CHAPI)
                     * that starting several isolates.
                     */
                    int i, j;
                    for (i = 0; i < isolatesAfter.length; i++) {
                        for (j = 0; j < isolatesBefore.length; j++) {
                            try {
                                if (isolatesBefore[j].equals(
                                        isolatesAfter[i])) {
                                    break;
                                }
                            } catch (Exception e) {
                                // isolatesAfter[i] might already exit,
                                // no need to wait for it
                                break;
                            }
                        }

                        if (j == isolatesBefore.length) {
                            try {
                                newIsolatesFound = true;
                                isolatesAfter[i].waitForExit();
                            } catch (Exception e) {
                                // ignore: the isolate might already exit
                            }
                        }
                    }
                } while (newIsolatesFound);

                if (loopCount > 0) {
                    loopCount -= 1;
                }
            }
        } catch (Throwable t) {
            handleInstallerException(suiteId, t);
        }

        if (midletSuiteStorage != null &&
                suiteId != MIDletSuite.UNUSED_SUITE_ID) {
            try {
                midletSuiteStorage.remove(suiteId);
            } catch (Throwable ex) {
                // ignore
            }
        }

        notifyDestroyed();
    }

    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     *
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     *
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        NativeEvent nativeEvent = (NativeEvent)event;

        switch (nativeEvent.getType()) {
            case EventTypes.AUTOTESTER_EVENT: {
                synchronized (this) {
                    eventsInQueueProcessed = true;
                    notify();
                }
                break;
            }
        }
    }
}
