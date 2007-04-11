/*
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
package com.sun.jumpimpl.module.pushregistry.persistence;

import com.sun.jump.module.contentstore.InMemoryContentStore;
import com.sun.jump.module.pushregistry.JUMPConnectionInfo;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import junit.framework.TestCase;

public final class StoreTest extends TestCase {
    private static final String [][] SAMPLE_CONNECTIONS = {
        new String [] {"sms://:500", "com.sun.FooMidlet", "12?4*"},
        new String [] {"socket://:5000", "com.sun.BarMidlet", "12?4*"},
        new String [] {"socket://", "com.sun.QuxMidlet", "12?4*"},
        new String [] {"datagram://:239", "com.sun.QuxMidlet", "???.???*"},
        new String [] {"datagram://", "com.sun.QuuxMidlet", "???.???*"},
        new String [] {"socket://:2006", "com.sun.QuuuxMidlet", "*"},
        new String [] {"datagram://:17", "com.sun.QuuuuxMidlet", "12.34"},
    };

    private static Store createStore() throws IOException {
        final String [] DIRS = {
            Store.CONNECTIONS_DIR, Store.ALARMS_DIR
        };
        final Store store = new Store(InMemoryContentStore.createStore(DIRS));
        store.readData();
        return store;
    }

    private static JUMPConnectionInfo createConnectionInfo(final int index) {
        final String [] info = SAMPLE_CONNECTIONS[index];
        return new JUMPConnectionInfo(info[0], info[1], info[2]);
    }

    private static JUMPConnectionInfo [] createConnectionInfos(
            final int [] indices) {
        final JUMPConnectionInfo [] cns
                = new JUMPConnectionInfo[indices.length];
        for (int i = 0; i < cns.length; i++) {
            cns[i] = createConnectionInfo(indices[i]);
        }
        return cns;
    }

    private static HashSet toSet(final JUMPConnectionInfo [] cns) {
        return new HashSet(Arrays.asList(cns));
    }

    private static class ExpectedConnections {
        final Map map = new HashMap();

        ExpectedConnections add(final int suiteId, final int [] indices) {
            map.put(new Integer(suiteId),
                    toSet(createConnectionInfos(indices)));
            return this;
        }
    }

    private static class ExpectedAlarms {
        final Map map = new HashMap();

        ExpectedAlarms add(
                final int suiteId,
                final String midlet,
                final long time) {
            final Integer key = new Integer(suiteId);
            Map suiteMap = (Map) map.get(key);
            if (suiteMap == null) {
                suiteMap = new HashMap();
                map.put(key, suiteMap);
            }
            suiteMap.put(midlet, new Long(time));

            return this;
        }
    }

    private void checkConnections(
            final Store store,
            final Map expectedConnections) {
        final Map actual = new HashMap();

        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int suiteId,
                    final JUMPConnectionInfo [] cns) {
                assertNull("reported twice: " + suiteId,
                        actual.put(new Integer(suiteId), toSet(cns)));
            }
        });

        assertEquals(expectedConnections, actual);
    }

    private void checkAlarms(
            final Store store,
            final Map expectedAlarms) {
        final Map actual = new HashMap();

        store.listAlarms(new Store.AlarmsConsumer() {
            public void consume(final int suiteId, final Map alarms) {
                assertNull("reported twice: " + suiteId,
                        actual.put(new Integer(suiteId), alarms));
            }
        });

        assertEquals(expectedAlarms, actual);
    }

    private void checkStore(
            final Store store,
            final Map expectedConnections,
            final Map expectedAlarms) throws IOException {
        checkConnections(store, expectedConnections);
        checkAlarms(store, expectedAlarms);

        store.readData();

        checkConnections(store, expectedConnections);
        checkAlarms(store, expectedAlarms);
    }

    private static void addConnection(
            final Store store,
            final int suiteId,
            final int connectionIndex) throws IOException {
        store.addConnection(suiteId, createConnectionInfo(connectionIndex));
    }

    private static void removeConnection(
            final Store store,
            final int suiteId,
            final int connectionIndex) throws IOException {
        store.removeConnection(suiteId, createConnectionInfo(connectionIndex));
    }

    private static void addConnections(
            final Store store,
            final int suiteId,
            final int [] connections) throws IOException {
        store.addConnections(suiteId, createConnectionInfos(connections));
    }

    private static void removeConnections(
            final Store store,
            final int suiteId) throws IOException {
        store.removeConnections(suiteId);
    }

    public void testCtor() {
        try {
            new Store(null);
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException _) {
        }
    }

    private static final Map EMPTY_MAP = new HashMap();

    public void testEmptyStore() throws Exception {
        final Store store = createStore();
        // No mutations
        checkStore(store, EMPTY_MAP, EMPTY_MAP);
    }

    public void testAddOneConnection() throws Exception {
        final Store store = createStore();

        addConnection(store, 0, 0);
        checkStore(store,
                new ExpectedConnections().add(0, new int [] {0}).map,
                EMPTY_MAP);
    }

    public void testAddAndRemoveConnection() throws Exception {
        final Store store = createStore();

        addConnection(store, 1, 1);
        removeConnection(store, 1, 1);

        checkStore(store, EMPTY_MAP, EMPTY_MAP);
    }

    private static final int suiteId = 1;
    private static final int [] cns = {1, 3, 4};
    private static final Map ec = new ExpectedConnections()
        .add(suiteId, cns)
        .map;

    public void testAddConnections() throws Exception {
        final Store store = createStore();

        addConnections(store, suiteId, cns);

        checkStore(store, ec, EMPTY_MAP);
    }

    public void testRemoveConnectionsCleansUp() throws Exception {
        final Store store = createStore();

        addConnections(store, suiteId, cns);
        removeConnections(store, suiteId);

        checkStore(store, EMPTY_MAP, EMPTY_MAP);
    }

    public void testReaddConnections() throws Exception {
        final Store store = createStore();

        addConnections(store, suiteId, cns);
        removeConnections(store, suiteId);
        addConnections(store, suiteId, cns);

        checkStore(store, ec, EMPTY_MAP);
    }

    private static final int suite0 = 11;
    private static final int suite1 = 239;
    private static final int suite2 = 1977;
    private static final int suite3 = 2006;
    private static final int suite4 = 1024;

    // NB: No connections for suites 2 and 4
    private static final Map ecRealistic = new ExpectedConnections()
        .add(suite0, new int [] {0, 1})
        .add(suite1, new int [] {2})
        .add(suite3, new int [] {3, 4})
        .map;

    public void testRealisticOneConnection() throws Exception {
        final Store store = createStore();

        // {}, {}, {}, {}, {}
        addConnection(store, suite0, 0);
        // { 0 }, {}, {}, {}, {}
        addConnection(store, suite1, 5); // to be removed (1)
        // { 0 }, { 5 }, {}, {}, {}
        addConnection(store, suite2, 6); // to be removed (2)
        // { 0 }, { 5 }, { 6 }, {}, {}
        addConnections(store, suite3, new int [] { 4, 3 });
        // { 0 }, { 5 }, { 6 }, { 3, 4 }, {}
        addConnection(store, suite0, 2); // to be removed (3)
        // { 0, 2 }, { 5 }, { 6 }, { 3, 4 }, {}
        removeConnection(store, suite0, 2); // see above (3)
        // { 0 }, { 5 }, { 6 }, { 3, 4 }, {}
        addConnection(store, suite1, 2);
        // { 0 }, { 2, 5 }, { 6 }, { 3, 4 }, {}
        removeConnections(store, suite2); // see above (2) suite2 is done
        // { 0 }, { 2, 5 }, {}, { 3, 4 }, {}
        removeConnection(store, suite1, 5); // see above (1) suite1 is done
        // { 0 }, { 2 }, {}, { 3, 4 }, {}
        addConnection(store, suite0, 1); // suite0 is done
        // { 0, 1 }, { 2 }, {}, { 3, 4 }, {}

        checkStore(store, ecRealistic, EMPTY_MAP);
    }

    public void testRealisticSeveralConnections() throws Exception {
        final Store store = createStore();

        // {}, {}, {}, {}, {}
        addConnection(store, suite0, 0);
        // { 0 }, {}, {}, {}, {}
        addConnection(store, suite1, 5); // to be removed (1)
        // { 0 }, { 5 }, {}, {}, {}
        addConnection(store, suite2, 6); // to be removed (2)
        // { 0 }, { 5 }, { 6 }, {}, {}
        addConnection(store, suite3, 4);
        // { 0 }, { 5 }, { 6 }, { 4 }, {}
        addConnection(store, suite0, 3); // to be removed (3)
        // { 0, 3 }, { 5 }, { 6 }, { 4 }, {}
        removeConnection(store, suite0, 3); // see above (3)
        // { 0 }, { 5 }, { 6 }, { 4 }, {}
        addConnection(store, suite1, 2);
        // { 0 }, { 2, 5 }, { 6 }, { 4 }, {}
        removeConnection(store, suite3, 4); // to be restored (4)
        // { 0 }, { 2, 5 }, { 6 }, {}, {}
        removeConnection(store, suite2, 6); // see above (2) suite2 is done
        // { 0 }, { 2, 5 }, {}, {}, {}
        removeConnection(store, suite1, 5); // see above (1) suite1 is done
        // { 0 }, { 2 }, {}, {}, {}
        addConnection(store, suite0, 1); // suite0 is done
        // { 0, 1 }, { 2 }, {}, {}, {}
        addConnection(store, suite3, 4);
        // { 0, 1 }, { 2 }, {}, { 4 }, {}
        addConnection(store, suite3, 3); // suite3 is done
        // { 0, 1 }, { 2 }, {}, { 3, 4 }, {}

        checkStore(store, ecRealistic, EMPTY_MAP);
    }

    private static final String midlet = "com.sun.foo";
    private static final long time = 239L;
    private static final String midlet2 = "com.sun.bar";
    private static final long time2 = 1977L;
    private static final int suiteId2 = 239;
    private static final String midlet3 = "com.sun.qux";
    private static final long time3 = 2007L;

    private static final Map ea = new ExpectedAlarms()
        .add(suiteId, midlet, time)
        .map;

    private static final Map ea3 = new ExpectedAlarms()
        .add(suiteId, midlet2, time3)
        .add(suiteId2, midlet, time2)
        .map;

    public void testAddOneAlarm() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet, time);

        checkStore(store, EMPTY_MAP, ea);
    }

    public void testAddAndRemoveOneAlarm() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet, time);
        store.removeAlarm(suiteId, midlet);

        checkStore(store, EMPTY_MAP, EMPTY_MAP);
    }

    public void testReaddOneAlarm() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet, time);
        store.removeAlarm(suiteId, midlet);
        store.addAlarm(suiteId, midlet, time);

        checkStore(store, EMPTY_MAP, ea);
    }

    public void testRemoveSecondAlarm() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet2, time);
        store.addAlarm(suiteId, midlet, time);
        store.removeAlarm(suiteId, midlet2);

        checkStore(store, EMPTY_MAP, ea);
    }

    public void testRemoveSecondAlarmImmediately() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet, time);
        store.addAlarm(suiteId, midlet2, time);
        store.removeAlarm(suiteId, midlet2);

        checkStore(store, EMPTY_MAP, ea);
    }

    public void testAddTwoAlarms() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId, midlet, time);
        store.addAlarm(suiteId, midlet2, time2);

        final ExpectedAlarms ea = new ExpectedAlarms()
            .add(suiteId, midlet, time)
            .add(suiteId, midlet2, time2);

        checkStore(store, EMPTY_MAP, ea.map);
    }

    public void testAddTwoAlarmsWithRemoval() throws Exception {
        final Store store = createStore();

        store.addAlarm(suiteId2, midlet2, time);
        store.addAlarm(suiteId2, midlet, time2);
        store.addAlarm(suiteId, midlet2, time3);
        store.removeAlarm(suiteId2, midlet2);

        checkStore(store, EMPTY_MAP, ea3);
    }

    public void testRealisticAll() throws Exception {
        final Store store = createStore();

        // {}, {}, {}, {}, {}
        addConnection(store, suite0, 0);
        // { 0 }, {}, {}, {}, {}
        addConnection(store, suite1, 5); // to be removed (1)
        // { 0 }, { 5 }, {}, {}, {}

        store.addAlarm(suiteId2, midlet2, time);

        addConnection(store, suite2, 6); // to be removed (2)
        // { 0 }, { 5 }, { 6 }, {}, {}
        addConnections(store, suite3, new int [] { 4, 3 });
        // { 0 }, { 5 }, { 6 }, { 3, 4 }, {}
        addConnection(store, suite0, 2); // to be removed (3)
        // { 0, 2 }, { 5 }, { 6 }, { 3, 4 }, {}
        removeConnection(store, suite0, 2); // see above (3)
        // { 0 }, { 5 }, { 6 }, { 3, 4 }, {}

        store.addAlarm(suiteId2, midlet, time2);
        store.addAlarm(suiteId, midlet2, time3);

        addConnection(store, suite1, 2);
        // { 0 }, { 2, 5 }, { 6 }, { 3, 4 }, {}
        removeConnections(store, suite2); // see above (2) suite2 is done
        // { 0 }, { 2, 5 }, {}, { 3, 4 }, {}
        removeConnection(store, suite1, 5); // see above (1) suite1 is done
        // { 0 }, { 2 }, {}, { 3, 4 }, {}

        store.removeAlarm(suiteId2, midlet2);

        addConnection(store, suite0, 1); // suite0 is done
        // { 0, 1 }, { 2 }, {}, { 3, 4 }, {}

        checkStore(store, ecRealistic, ea3);
    }
}
