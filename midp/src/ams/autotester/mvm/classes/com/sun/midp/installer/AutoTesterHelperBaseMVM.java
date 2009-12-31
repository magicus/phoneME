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

import java.io.IOException;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;

import com.sun.cldc.isolate.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.AmsUtil;
import com.sun.midp.main.MIDletSuiteUtils;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;
import com.sun.midp.content.CHManager;

import com.sun.midp.events.*;

/**
 * Abstract class that implements most of auto testing functionality 
 * for MVM mode. The rest of functionality is implemented by subclasses. 
 * Provides bunch of progress notification callbacks that are needed 
 * for some of the subclasses (ODT auto tester, for example).
 */
abstract class AutoTesterHelperBaseMVM extends AutoTesterHelperBase 
    implements EventListener {

    /** ID of the test suite being run */
    protected int suiteId = MIDletSuite.UNUSED_SUITE_ID;

    /** Our event queue. */
    protected EventQueue eventQueue;
    
    /** True if all events in our queue were processed. */
    private boolean eventsInQueueProcessed;

    private final CHManager chmanager;

    /**
     * Constructor.
     *
     * @param theURL URL of the test suite
     * @param theDomain security domain to assign to unsigned suites
     * @param theLoopCount how many iterations to run the suite
     */
    AutoTesterHelperBaseMVM(String theURL, String theDomain, 
            int theLoopCount) {

        super(theURL, theDomain, theLoopCount);

        eventQueue = EventQueue.getEventQueue();
        eventQueue.registerEventListener(EventTypes.AUTOTESTER_EVENT, this);
        
        /* in case of CHAPI being not enabled, the stub class will be used */
        chmanager = CHManager.getManager(null);
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

    /**
     * Send an event to ourselves.
     * Main idea of it is to process all events that are in the
     * queue at the moment when the test isolate has exited
     * (because when testing CHAPI there may be requests to
     * start new isolates). When this event arrives, all events
     * that were placed in the queue before it are guaranteed
     * to be processed.
     */
    private void waitForOwnEvent() {
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
    }

    private Isolate getIsolateToWaitFor(Isolate[] isolatesBefore) {
        Isolate[] isolatesAfter = Isolate.getIsolates();

        for (int i = 0; i < isolatesAfter.length; i++) {
            int j;
            for (j = 0; j < isolatesBefore.length; j++) {
                try {
                    if (isolatesBefore[j].equals(isolatesAfter[i])) {
                        break;
                    }
                } catch (Exception e) {
                    // isolatesAfter[i] might already exit,
                    // no need to wait for it
                    break;
                }
            }

            if (j == isolatesBefore.length)
                return isolatesAfter[i];
        }
        return null;
    }

    /**
     * Installs and performs the tests.
     */
    void installAndPerformTests() 
        throws Exception {

        try {
            for (; loopCount != 0; ) {
                // force an overwrite and remove the RMS data
                suiteId = installer.installJad(url, 
                        Constants.INTERNAL_STORAGE_ID, true, true, null);
                onTestSuiteInstalled();

                Isolate[] isolatesBefore = Isolate.getIsolates();

                MIDletInfo midletInfo = getFirstMIDletOfSuite(suiteId);
                Isolate testIsolate = AmsUtil.startMidletInNewIsolate(suiteId,
                    midletInfo.classname, midletInfo.name, null, null, null);

                /*
                 * Wait for termination of all isolates contained in
                 * isolatesAfter[], but not in isolatesBefore[].
                 * This is needed to pass some tests (for example, CHAPI)
                 * that starting several isolates.
                 */
                while( testIsolate != null ){
                    try {
                        testIsolate.waitForExit();
                    } catch( Exception x ){}
                    testIsolate = null;
                    
                    for(;;){
                        testIsolate = getIsolateToWaitFor(isolatesBefore);
                        if( testIsolate == null &&
                                chmanager.getPendingRequestsCount(
                                            MIDletSuite.UNUSED_SUITE_ID) == 0 )
                            break;
                        // let the CHAPI to start a midlet
                        waitForOwnEvent();
                    }
                }

                if (loopCount > 0) {
                    loopCount -= 1;
                }
            }
        } finally {
            // we are done: cleanup
            if (midletSuiteStorage != null &&
                    suiteId != MIDletSuite.UNUSED_SUITE_ID) {
                try {
                    midletSuiteStorage.remove(suiteId);
                    onTestSuiteRemoved();
                } catch (Throwable ex) {
                    // ignore
                }
            }            
        }
    }

    /**
     * Gets ID of the current test suite.
     *
     * @return ID of the current test suite
     */
    int getTestSuiteId() {
        return suiteId;
    }

    /**
     * Called after test suite has been installed (updated).
     */
    abstract void onTestSuiteInstalled();

    /**
     * Called after test suite has been removed from storage (uninstalled).
     */
    abstract void onTestSuiteRemoved();
}
