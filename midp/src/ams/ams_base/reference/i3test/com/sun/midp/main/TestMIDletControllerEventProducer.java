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

package com.sun.midp.main;

import com.sun.midp.events.StubEventQueue;
import com.sun.midp.events.Event;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.i3test.TestCase;
import com.sun.midp.security.SecurityToken;

import java.util.Random;

/**
 * Unit tests for the MIDletControllerEventProducer class. This class, and 
 * event producer classes in general, don't have much logic. However, they do 
 * implement the mapping between specific data and generic event fields (e.g., 
 * intParam1 or stringParam2) which is important to test.
 */
public class TestMIDletControllerEventProducer extends TestCase {

    private SecurityToken token;

    Random rand = new Random();

    // Constant test data.

    static final String SUITE_ID = "the midlet suite ID";
    static final String CLASS_NAME = "the midlet class name";
    static final String DISPLAY_NAME = "the display-name of the midlet";
    static final String TARGET_SUITE_ID = "the target midlet suite ID";
    static final String TARGET_CLASS_NAME = "the target midlet's class name";

    // The following instance variables comprise the test fixture.
    // They are freshly initialized before each test is run.

    int currentIsolateId;
    int amsIsolateId;
    int displayId;
    int displayId2;

    StubEventQueue queue;
    MIDletControllerEventProducer producer;

    /**
     * Initializes the test fixture with random data, creates the stub event 
     * queue, and creates the MIDletControllerEventProducer under test.
     */
    void setUp() {
        currentIsolateId = rand.nextInt();
        amsIsolateId = rand.nextInt();
        displayId = rand.nextInt();
        displayId2 = rand.nextInt();

        queue = new StubEventQueue();
        producer = new MIDletControllerEventProducer(token, queue,
            amsIsolateId, currentIsolateId);
    }

    /**
     * Nulls out the stub event queue and the event producer.
     */
    void tearDown() {
        queue = null;
        producer = null;
    }

    /**
     * Utility method to check the stub event queue's log to ensure that it 
     * contains exactly one native event.  Returns this event.
     */
    NativeEvent checkSingleNativeEvent() {
        Event[] log = queue.getEventLog();
        assertEquals("log should have one event", 1, log.length);
        assertTrue("event should be native event",
            log[0] instanceof NativeEvent);
        NativeEvent nev = (NativeEvent)log[0];
        return nev;
    }

    /**
     * Utility method to check the stub event queue's log to ensure that it
     * contains exactly numEvents native events.  Returns an array containing 
     * exactly that number of native events.
     */
    NativeEvent[] checkNativeEvents(int numEvents) {
        Event[] log = queue.getEventLog();
        assertEquals("log should have " + numEvents + " events",
            numEvents, log.length);

        for (int i = 0; i < log.length; i++) {
            assertTrue("log[" + i + "] should be a native event",
                log[i] instanceof NativeEvent);
        }

        NativeEvent nlog[] = new NativeEvent[log.length];
        System.arraycopy(log, 0, nlog, 0, log.length);
        return nlog;
    }

    // the actual tests

    /**
     * Tests putting events into the stub event queue and retrieving them.
     */
    void testStubEventQueue() {
        Event ev0 = new Event(0);
        Event ev1 = new Event(1);
        Event ev2 = new Event(2);

        queue.post(ev0);
        queue.post(ev1);
        queue.post(ev2);

        Event[] log = queue.getEventLog();

        assertEquals(3, log.length);
        assertSame(ev0, log[0]);
        assertSame(ev1, log[1]);
        assertSame(ev2, log[2]);
    }

    /**
     * Tests sendMIDletStartErrorEvent().
     */
    void testMIDletStartErrorEvent() {
        producer.sendMIDletStartErrorEvent(1, SUITE_ID, CLASS_NAME, 1);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_START_ERROR_EVENT, nev.getType());
        assertEquals(1, nev.intParam1);
        assertEquals(1, nev.intParam2);
        assertEquals(SUITE_ID, nev.stringParam1);
        assertEquals(CLASS_NAME, nev.stringParam2);
    }

    /**
     * Tests sendMIDletCreateNotifyEvent().
     */
    void testMIDletCreateNotifyEvent() {
        producer.sendMIDletCreateNotifyEvent(1, displayId, SUITE_ID,
            CLASS_NAME, DISPLAY_NAME);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_CREATED_NOTIFICATION, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(1, nev.intParam2);
        assertEquals(displayId, nev.intParam4);
        assertEquals(SUITE_ID, nev.stringParam1);
        assertEquals(CLASS_NAME, nev.stringParam2);
        assertEquals(DISPLAY_NAME, nev.stringParam3);
    }

    /**
     * Tests sendMIDletActiveNotifyEvent().
     */
    void testMIDletActiveNotifyEvent() {
        producer.sendMIDletActiveNotifyEvent(displayId);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_ACTIVE_NOTIFICATION, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(displayId, nev.intParam4);
    }

    /**
     * Tests sendMIDletPauseNotifyEvent().
     */
    void testMIDletPauseNotifyEvent() {
        producer.sendMIDletPauseNotifyEvent(displayId);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_PAUSED_NOTIFICATION, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(displayId, nev.intParam4);
    }

    /**
     * Tests sendMIDletDestroyNotifyEvent().
     */
    void testMIDletDestroyNotifyEvent() {
        producer.sendMIDletDestroyNotifyEvent(displayId);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_DESTROYED_NOTIFICATION, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(displayId, nev.intParam4);
    }

    /**
     * Tests sendMIDletDestroyRequestEvent().
     */
    void testMIDletDestroyRequestEvent() {
        producer.sendMIDletDestroyRequestEvent(displayId);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.MIDLET_DESTROY_REQUEST_EVENT, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(displayId, nev.intParam4);
    }

    /**
     * Tests sendMIDletForegroundTransferEvent().
     */
    void testMIDletForegroundTransferEvent() {
        producer.sendMIDletForegroundTransferEvent(
            SUITE_ID, CLASS_NAME, TARGET_SUITE_ID, TARGET_CLASS_NAME);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.FOREGROUND_TRANSFER_EVENT, nev.getType());
        assertEquals(SUITE_ID, nev.stringParam1);
        assertEquals(CLASS_NAME, nev.stringParam2);
        assertEquals(TARGET_SUITE_ID, nev.stringParam3);
        assertEquals(TARGET_CLASS_NAME, nev.stringParam4);
    }

    /**
     * Tests sendDisplayForegroundRequestEvent().
     */
    void testDisplayForegroundRequestEvent() {
        producer.sendDisplayForegroundRequestEvent(displayId, false);
        producer.sendDisplayForegroundRequestEvent(displayId2, true);

        NativeEvent nev[] = checkNativeEvents(2);

        assertEquals(EventTypes.FOREGROUND_REQUEST_EVENT, nev[0].getType());
        assertEquals(currentIsolateId, nev[0].intParam1);
        assertEquals(displayId, nev[0].intParam4);
        assertEquals(0, nev[0].intParam2); // false

        assertEquals(EventTypes.FOREGROUND_REQUEST_EVENT, nev[1].getType());
        assertEquals(currentIsolateId, nev[1].intParam1);
        assertEquals(displayId2, nev[1].intParam4);
        assertEquals(1, nev[1].intParam2); // true
    }

    /**
     * Tests sendDisplayBackgroundRequestEvent().
     */
    void testDisplayBackgroundRequestEvent() {
        producer.sendDisplayBackgroundRequestEvent(displayId);

        NativeEvent nev = checkSingleNativeEvent();

        assertEquals(EventTypes.BACKGROUND_REQUEST_EVENT, nev.getType());
        assertEquals(currentIsolateId, nev.intParam1);
        assertEquals(displayId, nev.intParam4);
    }

    /**
     * Tests sendDisplayPreemptStartEvent() and
     * sendDisplayPreemptStopEvent().
     */
    void testDisplayPreemptEvents() {
        producer.sendDisplayPreemptStartEvent(displayId);
        producer.sendDisplayPreemptStopEvent(displayId2);

        NativeEvent nev[] = checkNativeEvents(2);

        assertEquals(EventTypes.PREEMPT_EVENT, nev[0].getType());
        assertEquals(currentIsolateId, nev[0].intParam1);
        assertEquals(-1, nev[0].intParam2); // true
        assertEquals(displayId, nev[0].intParam4);

        assertEquals(EventTypes.PREEMPT_EVENT, nev[1].getType());
        assertEquals(currentIsolateId, nev[1].intParam1);
        assertEquals(0, nev[1].intParam2); // false
        assertEquals(displayId2, nev[1].intParam4);
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws Throwable {
        token = getSecurityToken();

        declare("testStubEventQueue");
        setUp();
        testStubEventQueue();
        tearDown();

        declare("testMIDletStartErrorEvent");
        setUp();
        testMIDletStartErrorEvent();
        tearDown();

        declare("testMIDletCreateNotifyEvent");
        setUp();
        testMIDletCreateNotifyEvent();
        tearDown();

        declare("testMIDletActiveNotifyEvent");
        setUp();
        testMIDletActiveNotifyEvent();
        tearDown();

        declare("testMIDletPauseNotifyEvent");
        setUp();
        testMIDletPauseNotifyEvent();
        tearDown();

        declare("testMIDletDestroyNotifyEvent");
        setUp();
        testMIDletDestroyNotifyEvent();
        tearDown();

        declare("testMIDletDestroyRequestEvent");
        setUp();
        testMIDletDestroyRequestEvent();
        tearDown();

        declare("testMIDletForegroundTransferEvent");
        setUp();
        testMIDletForegroundTransferEvent();
        tearDown();

        declare("testDisplayForegroundRequestEvent");
        setUp();
        testDisplayForegroundRequestEvent();
        tearDown();

        declare("testDisplayBackgroundRequestEvent");
        setUp();
        testDisplayBackgroundRequestEvent();
        tearDown();

        declare("testDisplayPreemptEvents");
        setUp();
        testDisplayPreemptEvents();
        tearDown();
    }

}
