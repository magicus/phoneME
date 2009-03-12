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

package com.sun.midp.events;
import com.sun.midp.i3test.*;

/**
 * Unit tests for the EventQueue class.
 */
public class TestEventQueue extends TestCase {

    /**
     * Test simple creation of an event queue.
     */
    void testCreate() {
        EventQueue eq = new EventQueue();

        assertFalse(eq.alive);
        assertNotNull(eq.dispatchTable);
        assertTrue(eq.dispatchTable.length > 0);
        assertNotNull(eq.pool);
        assertNotNull(eq.eventQueueThread);
        assertNotNull(eq.eventMonitorThread);
        assertFalse(eq.eventQueueThread.isAlive());
        assertFalse(eq.eventMonitorThread.isAlive());
    }

    /**
     * Tests the ability to register an event.
     */
    void testRegister() {
        final int EVENT_TYPE = 7;
        EventQueue eq = new EventQueue();
        InstrumentedEventListener iel = new InstrumentedEventListener();
        eq.registerEventListener(EVENT_TYPE, iel);

        DispatchData dd = eq.dispatchTable[EVENT_TYPE-1];
        assertNotNull(dd);
        assertEquals(iel, dd.listener);
    }

    /**
     * Tests whether the dispatch table is grown properly.
     */
    void testGrowDispatchTable() {
        final int EVENT_TYPE_A = 4;
        final int EVENT_TYPE_B = 97;
            // must be larger than the dispatch table default size

        EventQueue eq = new EventQueue();

        assertTrue(EVENT_TYPE_B > eq.dispatchTable.length);

        InstrumentedEventListener iela = new InstrumentedEventListener();
        InstrumentedEventListener ielb = new InstrumentedEventListener();

        eq.registerEventListener(EVENT_TYPE_A, iela);
        eq.registerEventListener(EVENT_TYPE_B, ielb);

        DispatchData dda = eq.dispatchTable[EVENT_TYPE_A-1];
        assertNotNull(dda);
        assertEquals(iela, dda.listener);

        DispatchData ddb = eq.dispatchTable[EVENT_TYPE_B-1];
        assertNotNull(ddb);
        assertEquals(ielb, ddb.listener);
    }

    /**
     * Tests posting of an event.
     */
    void testPost1() {
        final int EVENT_TYPE = 14;
        EventQueue eq = new EventQueue();

        InstrumentedEventListener iel = new InstrumentedEventListener();
        eq.registerEventListener(EVENT_TYPE, iel);

        Event ev = new Event(EVENT_TYPE);
        eq.post(ev);

        // assertions on the event queue

        assertSame("nextEvent should be ev", ev, eq.nextEvent);
        assertSame("lastEvent should be ev", ev, eq.lastEvent);

        // assertions from the event listener

        Event[] arr;

        arr = iel.getProcessedEvents();
        assertEquals("processed should be length 0", 0, arr.length);

        arr = iel.getPreprocessedEvents();
        assertEquals("preprocessed should be length 1", 1, arr.length);
        assertSame("preprocessed[0] should be ev", ev, arr[0]);

        arr = iel.getWaitingEvents();
        assertEquals("waiting should be length 1", 1, arr.length);
        assertNull("waiting[0] should be null", arr[0]);
    }

    /**
     * Tests posting of three events.
     */
    void testPost3() {
        final int EVENT_TYPE_A = 5;
        final int EVENT_TYPE_B = 7;

        EventQueue eq = new EventQueue();

        InstrumentedEventListener iel = new InstrumentedEventListener();
        eq.registerEventListener(EVENT_TYPE_A, iel);
        eq.registerEventListener(EVENT_TYPE_B, iel);

        Event ev0 = new Event(EVENT_TYPE_A);
        Event ev1 = new Event(EVENT_TYPE_B);
        Event ev2 = new Event(EVENT_TYPE_A);
        eq.post(ev0);
        eq.post(ev1);
        eq.post(ev2);

        // assertions on the event queue

        assertSame("nextEvent should be ev0", ev0, eq.nextEvent);
        assertSame("lastEvent should be ev2", ev2, eq.lastEvent);
        assertSame("ev0.next should be ev1", ev1, ev0.next);
        assertSame("ev1.next should be ev2", ev2, ev1.next);
        assertNull("ev2.next should be null", ev2.next);

        // assertions from the event listener

        Event[] arr;

        arr = iel.getProcessedEvents();
        assertEquals("processed should be length 0", 0, arr.length);

        arr = iel.getPreprocessedEvents();
        assertEquals("preprocessed should be length 3", 3, arr.length);
        assertSame("preprocessed[0] should be ev0", ev0, arr[0]);
        assertSame("preprocessed[1] should be ev1", ev1, arr[1]);
        assertSame("preprocessed[2] should be ev2", ev2, arr[2]);

        arr = iel.getWaitingEvents();
        assertEquals("waiting should be length 3", 3, arr.length);
        assertNull("waiting[0] should be null", arr[0]);
        assertNull("waiting[1] should be null", arr[1]);
        assertEquals("waiting[2] should be ev0", ev0, arr[2]);
    }

    /**
     * Tests preprocessing of events.
     */
    void testPreprocess() {
        EventQueue eq = new EventQueue();
        InstrumentedEventListener iel = new InstrumentedEventListener(true);
        final int EVENT_TYPE = 10;

        eq.registerEventListener(EVENT_TYPE, iel);

        Event ev0 = new Event(EVENT_TYPE);
        Event ev1 = new Event(EVENT_TYPE);
        eq.post(ev0);
        iel.setPreprocess(false);
        eq.post(ev1);

        // assertions on the event queue

        assertSame("nextEvent should be ev0", ev0, eq.nextEvent);
        assertSame("lastEvent should be ev0", ev0, eq.lastEvent);
        assertNull("ev0.next should be null", ev0.next);

        // assertions from the event listener

        Event[] arr;

        arr = iel.getProcessedEvents();
        assertEquals("processed should be length 0", 0, arr.length);

        arr = iel.getPreprocessedEvents();
        assertEquals("preprocessed should be length 2", 2, arr.length);
        assertSame("preprocessed[0] should be ev0", ev0, arr[0]);
        assertSame("preprocessed[1] should be ev1", ev1, arr[1]);

        arr = iel.getWaitingEvents();
        assertEquals("waiting should be length 2", 2, arr.length);
        assertNull("waiting[0] should be null", arr[0]);
        assertEquals("waiting[1] should be ev0", ev0, arr[1]);
    }

    /**
     * Runs all tests.
     */
    public void runTests() throws Throwable {
        declare("testCreate");
        testCreate();
        declare("testRegister");
        testRegister();
        declare("testGrowDispatchTable");
        testGrowDispatchTable();
        declare("testPost1");
        testPost1();
        declare("testPost3");
        testPost3();
        declare("testPreprocess");
        testPreprocess();
    }

}
