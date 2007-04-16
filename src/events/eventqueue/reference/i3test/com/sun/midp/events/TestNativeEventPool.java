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
 * Unit tests for the NativeEventPool class.
 */
public class TestNativeEventPool extends TestCase {

    /**
     * Tests putting multiple events into the pool.
     */
    void testMultiPut() {
        final int NUM_EVENTS = 3;
        assertTrue(NUM_EVENTS < NativeEventPool.DEFAULT_SIZE);

        NativeEventPool pool = new NativeEventPool();
        NativeEvent evts[] = new NativeEvent[NUM_EVENTS];
        for (int i = 0; i < NUM_EVENTS; i++) {
            evts[i] = new NativeEvent();
            pool.putBack(evts[i]);
        }

        assertEquals(NUM_EVENTS, pool.eventsInPool);
        for (int i = 0; i < NUM_EVENTS; i++) {
            assertSame(evts[i], pool.eventStack[i]);
        }
    }

    /**
     * Tests filling up the event pool.
     */
    void testFillPool() {
        NativeEventPool pool = new NativeEventPool();

        for (int i = 0; i < NativeEventPool.DEFAULT_SIZE; i++) {
            pool.putBack(new NativeEvent());
        }

        assertEquals(NativeEventPool.DEFAULT_SIZE, pool.eventsInPool);
        pool.putBack(new NativeEvent());
        assertEquals(NativeEventPool.DEFAULT_SIZE, pool.eventsInPool);
    }


    /**
     * Tests that the event is cleared when it is returned to the pool.  This 
     * is at least as much a test of NativeEvent.clear() as it is of the 
     * NativeEventPool's clearing function.
     */
    void testClear() {
        NativeEventPool pool = new NativeEventPool();
        NativeEvent evt = new NativeEvent(83);

        evt.intParam1 = 1;
        evt.intParam2 = 2;
        evt.intParam3 = 3;
        evt.intParam4 = 4;

        evt.stringParam1 = "one";
        evt.stringParam2 = "two";
        evt.stringParam3 = "three";
        evt.stringParam4 = "four";
        evt.stringParam5 = "five";
        evt.stringParam6 = "six";

        pool.putBack(evt);

        assertEquals(0, evt.type);
        assertEquals(0, evt.intParam1);
        assertEquals(0, evt.intParam2);
        assertEquals(0, evt.intParam3);
        assertEquals(0, evt.intParam4);
        assertEquals(null, evt.stringParam1);
        assertEquals(null, evt.stringParam2);
        assertEquals(null, evt.stringParam3);
        assertEquals(null, evt.stringParam4);
        assertEquals(null, evt.stringParam5);
        assertEquals(null, evt.stringParam6);
    }

    /**
     * Tests a simple get-put-get sequence.
     */
    void testGetPutGet() {
        NativeEventPool pool = new NativeEventPool();
        NativeEvent evt = pool.get();
        assertEquals(0, pool.eventsInPool);

        pool.putBack(evt);
        assertEquals(1, pool.eventsInPool);
        assertSame(evt, pool.eventStack[0]);

        NativeEvent evt2 = pool.get();
        assertSame(evt, evt2);
        assertEquals(0, pool.eventsInPool);
    }

    /**
     * Tests basic creation of a native event pool.
     */
    void testCreate() {
        NativeEventPool pool = new NativeEventPool();
        assertEquals(0, pool.eventsInPool);
        assertNotNull(pool.eventStack);
    }

    /**
     * Runs all tests.
     */
    public void runTests() throws Throwable {
        declare("testCreate");
        testCreate();
        declare("testGetPutGet");
        testGetPutGet();
        declare("testClear");
        testClear();
        declare("testFillPool");
        testFillPool();
        declare("testMultiPut");
        testMultiPut();
    }

}
