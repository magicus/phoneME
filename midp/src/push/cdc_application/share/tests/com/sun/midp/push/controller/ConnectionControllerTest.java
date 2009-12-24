/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.push.controller;

import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import junit.framework.*;
import com.sun.midp.push.reservation.ConnectionReservation;
import com.sun.midp.push.reservation.DataAvailableListener;
import com.sun.midp.push.reservation.ReservationDescriptor;
import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.persistence.StoreUtils;
import com.sun.j2me.security.AccessControlContext;
import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

/*
 * ConnectionController tests.
 *
 * <p>
 * TODO: failing store tests.
 * </p>
 */
public final class ConnectionControllerTest extends TestCase {

    public ConnectionControllerTest(final String testName) {
        super(testName);
    }

    /**
     * Delayble thread.
     *
     * Instances of this class can be scheduled for executuion,
     * but actual execution can be delayed with a lock.
     */
    private static abstract class DelayableThread extends Thread {
        private final Object lock = new Object();

        abstract protected void doRun();

        public final void run() {
            synchronized (lock) {
                doRun();
            }
        }
    }

    /**
     * Mock implementation of ConnectionReservation.
     */
    private static final class MockConnectionReservation
            implements ConnectionReservation {

        private final DataAvailableListener listener;

        private boolean isCancelled = false;

        private boolean hasAvailableData_ = false;

        MockConnectionReservation(final DataAvailableListener listener) {
            this.listener = listener;
        }

        /** {@inheritDoc} */
        public boolean hasAvailableData() {
            if (isCancelled) {
                throw new IllegalStateException("cancelled reservation");
            }
            return hasAvailableData_;
        }

        /** {@inheritDoc} */
        public void cancel() {
            if (isCancelled) {
                throw new IllegalStateException("double cancellation");
            }
            isCancelled = true;
        }

        /** Returns a thread that can be used to 'ping' the connection. */
        DelayableThread pingThread() {
            return new DelayableThread() {
                protected void doRun() {
                    listener.dataAvailable();
                }
            };
        }
    }

    /**
     * Mock implementation of ReservationDescriptor.
     */
    private static final class MockReservationDescriptor
            implements ReservationDescriptor {
        /** connection name. */
        final String connectionName;

        /** filter. */
        final String filter;

        /** reserved flag. */
        boolean isReserved = false;

        /** connection reservation. */
        MockConnectionReservation connectionReservation = null;

        /**
         * Creates a mock.
         *
         * @param connectionName connection name
         * (cannot be <code>null</code>)
         *
         * @param filter filter
         * (cannot be <code>null</code>)
         */
        MockReservationDescriptor(final String connectionName,
                final String filter) {
            this.connectionName = connectionName;
            this.filter = filter;
        }

        /** {@inheritDoc} */
        public ConnectionReservation reserve(
                final int midletSuiteId, final String midletClassName,
                final DataAvailableListener dataAvailableListener)
                    throws IOException {
            isReserved = true;
            connectionReservation =
                    new MockConnectionReservation(dataAvailableListener);
            return connectionReservation;
        }

        /** {@inheritDoc} */
        public String getConnectionName() {
            return connectionName;
        }

        /** {@inheritDoc} */
        public String getFilter() {
            return filter;
        }
        
        /** {@inheritDoc} */
        public boolean isConnectionNameEquivalent(String name) {
            return connectionName.equals(name);
        }
    }

    private static Store createStore() throws IOException {
        return StoreUtils.getInstance().getStore();
    }

    private final ReservationDescriptorFactory rdf =
            new ReservationDescriptorFactory() {
        public ReservationDescriptor getDescriptor(
                final String connectionName, final String filter,
                final AccessControlContext context)
                    throws IllegalArgumentException,
                        ConnectionNotFoundException {
            if (context == null) {
                throw new RuntimeException("null AccessControlContext");
            }
            return new MockReservationDescriptor(connectionName, filter);
        }
    };

    private ConnectionController createConnectionController(
            final Store store, final LifecycleAdapter lifecycleAdapter) {
        /*
         * IMPL_NOTE: don't forget mock parameter verifier if the signature
         * below change
         */
        return new ConnectionController(store, rdf, lifecycleAdapter);
    }

    private ConnectionController createConnectionController(
            final Store store) {
        return createConnectionController(store, new ListingLifecycleAdapter());
    }

    private ConnectionController.ReservationHandler
        createFakeReservationHandler(
            final int midletSuiteId,
            final String midlet,
            final String connection,
            final String filter) throws IOException {
        final ReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);
        return createConnectionController(createStore())
                .new ReservationHandler(midletSuiteId, midlet, descriptor);
    }

    public void testReservationHandlerCtor() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        assertEquals(midletSuiteId, h.getSuiteId());
        assertEquals(midlet, h.getMidlet());
        assertEquals(connection, h.getConnectionName());
        assertEquals(filter, h.getFilter());
    }

    private void assertSetsEqual(
            final Object [] expected, final Collection actual) {
        assertEquals(new HashSet(Arrays.asList(expected)), new HashSet(actual));
    }

    private void assertSetsEqual(
            final Object [] expected, final Object [] actual) {
        assertSetsEqual(expected, Arrays.asList(actual));
    }

    private void checkCollectionHasSingleHandler(
            final Collection reservations,
            final ConnectionController.ReservationHandler handler) {
        assertEquals(1, reservations.size());
        assertSame(handler, reservations.iterator().next());
    }

    public void testQueryByConnection() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h);

        assertSame(h, reservations.queryByConnectionName(connection));
        checkCollectionHasSingleHandler(reservations.getAllReservations(), h);
    }

    public void testQueryByConnectionMissing() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h);

        assertNull(reservations.queryByConnectionName(connection + "qux"));
        checkCollectionHasSingleHandler(reservations.getAllReservations(), h);
    }

    public void testQueryBySuite() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h);

        checkCollectionHasSingleHandler(
                reservations.queryBySuiteID(midletSuiteId), h);
        checkCollectionHasSingleHandler(reservations.getAllReservations(), h);
    }

    public void testQueryBySuiteMissing() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h);

        assertTrue(reservations.queryBySuiteID(midletSuiteId + 1).isEmpty());
        checkCollectionHasSingleHandler(reservations.getAllReservations(), h);
    }

    public void testEmptyReservations() {
        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();

        assertNull(reservations.queryByConnectionName("foo://bar"));
        assertTrue(reservations.queryBySuiteID(13).isEmpty());
        assertTrue(reservations.getAllReservations().isEmpty());
    }

    public void testAddAndRemove() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h);
        reservations.remove(h);

        assertNull(reservations.queryByConnectionName(connection));
        assertTrue(reservations.queryBySuiteID(midletSuiteId).isEmpty());
        assertTrue(reservations.getAllReservations().isEmpty());
    }

    public void testTwoHandlersReservations() throws IOException {
        final int midletSuiteId1 = 123;
        final String midlet1 = "com.sun.Foo";
        final String connection1 = "foo://bar";
        final String filter1 = "*.123";

        final ConnectionController.ReservationHandler h1 =
                createFakeReservationHandler(
                    midletSuiteId1, midlet1, connection1, filter1);

        final int midletSuiteId2 = 321;
        final String midlet2 = "com.sun.Bar";
        final String connection2 = "qux://bar";
        final String filter2 = "*";

        final ConnectionController.ReservationHandler h2 =
                createFakeReservationHandler(
                    midletSuiteId2, midlet2, connection2, filter2);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h1);
        reservations.add(h2);

        assertSame(h1, reservations.queryByConnectionName(connection1));
        assertSame(h2, reservations.queryByConnectionName(connection2));
        checkCollectionHasSingleHandler(
                reservations.queryBySuiteID(midletSuiteId1), h1);
        checkCollectionHasSingleHandler(
                reservations.queryBySuiteID(midletSuiteId2), h2);
        assertSetsEqual(new Object [] {h1, h2},
			reservations.getAllReservations());
    }

    public void testClear() throws IOException {
        final int midletSuiteId1 = 123;
        final String midlet1 = "com.sun.Foo";
        final String connection1 = "foo://bar";
        final String filter1 = "*.123";

        final ConnectionController.ReservationHandler h1 =
                createFakeReservationHandler(
                    midletSuiteId1, midlet1, connection1, filter1);

        final int midletSuiteId2 = 321;
        final String midlet2 = "com.sun.Bar";
        final String connection2 = "qux://bar";
        final String filter2 = "*";

        final ConnectionController.ReservationHandler h2 =
                createFakeReservationHandler(
                    midletSuiteId2, midlet2, connection2, filter2);

        final ConnectionController.Reservations reservations =
                new ConnectionController.Reservations();
        reservations.add(h1);
        reservations.add(h2);
        reservations.clear();

        assertNull(reservations.queryByConnectionName(connection1));
        assertNull(reservations.queryByConnectionName(connection2));
        assertTrue(reservations.queryBySuiteID(midletSuiteId1).isEmpty());
        assertTrue(reservations.queryBySuiteID(midletSuiteId2).isEmpty());
        assertTrue(reservations.getAllReservations().isEmpty());
    }

    private void checkStoreEmpty(final Store store) {
        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int id, final ConnectionInfo [] infos) {
                fail("store must be empty");
            }
        });
    }

    private void checkStoreHasSingleRecord(
            final Store store,
            final int midletSuiteId, final String midlet,
            final String connection, final String filter) {
        final ConnectionInfo info =
                new ConnectionInfo(connection, midlet, filter);

        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int id, final ConnectionInfo [] infos) {
                assertEquals(id, midletSuiteId);
                assertEquals(1, infos.length);
                assertEquals(info, infos[0]);
            }
        });
    }

    private void checkSingletonConnectionList(
            final ConnectionController cc,
            final int midletSuiteId,
            final String connection) {
        final String [] cns = cc.listConnections(midletSuiteId, false);
        assertEquals(1, cns.length);
        assertEquals(connection, cns[0]);
    }

    public void testRegisterConnectionState() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor);

        assertTrue(descriptor.isReserved);
        assertFalse(descriptor.connectionReservation.isCancelled);
        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet, connection, filter);
        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter, cc.getFilter(midletSuiteId, connection));
    }

    public void testReregistration() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final String filter2 = "*.*.*";

        final Store store = createStore();

        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection, filter);

        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection, filter2);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor1);
        cc.registerConnection(midletSuiteId, midlet, descriptor2);

        assertTrue(descriptor1.isReserved);
        assertTrue(descriptor1.connectionReservation.isCancelled);
        assertTrue(descriptor2.isReserved);
        assertFalse(descriptor2.connectionReservation.isCancelled);

        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet, connection, filter2);
        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter2, cc.getFilter(midletSuiteId, connection));
    }

    public void testReregistrationOfAnotherSuite() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final int midletSuiteId2 = midletSuiteId + 17;

        final Store store = createStore();

        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection, filter);

        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor1);

        boolean ioExceptionThrown = false;
        try {
            cc.registerConnection(midletSuiteId2, midlet, descriptor2);
        } catch (IOException ioex) {
            ioExceptionThrown = true;
        }

        assertTrue(ioExceptionThrown);

        assertTrue(descriptor1.isReserved);
        assertFalse(descriptor1.connectionReservation.isCancelled);

        assertFalse(descriptor2.isReserved);

        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet, connection, filter);

        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter, cc.getFilter(midletSuiteId, connection));

        assertEquals(0, cc.listConnections(midletSuiteId2, false).length);
        assertNull(cc.getMIDlet(midletSuiteId2, connection));
        assertNull(cc.getFilter(midletSuiteId2, connection));
    }

    public void testReregistrationOfAnotherMIDlet() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final String midlet2 = "com.sun.Bar";

        final Store store = createStore();

        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection, filter);

        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor1);
        cc.registerConnection(midletSuiteId, midlet2, descriptor2);

        assertTrue(descriptor1.isReserved);
        assertTrue(descriptor1.connectionReservation.isCancelled);

        assertTrue(descriptor2.isReserved);
        assertFalse(descriptor2.connectionReservation.isCancelled);

        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet2, connection, filter);
        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet2, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter, cc.getFilter(midletSuiteId, connection));
    }

    public void testRegistrationOfFailingReservation() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final ReservationDescriptor descriptor = new ReservationDescriptor() {
            public ConnectionReservation reserve(
                    final int midletSuiteId, final String midletClassName,
                    final DataAvailableListener dataAvailableListener)
                        throws IOException {
                throw new IOException("cannot be registered");
            }

            public String getConnectionName() {
                return connection;
            }

            public String getFilter() {
                return filter;
            }
            
            /** {@inheritDoc} */
            public boolean isConnectionNameEquivalent(String name) {
                return connection.equals(name);
            }
        };

        final ConnectionController cc = createConnectionController(store);

        boolean ioExceptionThrown = false;
        try {
            cc.registerConnection(midletSuiteId, midlet, descriptor);
        } catch (IOException ioex) {
            ioExceptionThrown = true;
        }

        assertTrue(ioExceptionThrown);

        checkStoreEmpty(store);
        assertEquals(0, cc.listConnections(midletSuiteId, false).length);
        assertNull(cc.getMIDlet(midletSuiteId, connection));
        assertNull(cc.getFilter(midletSuiteId, connection));
    }

    public void testListConnectionsAll() throws IOException {
        final int midletSuiteId1 = 123;

        final String midlet1 = "com.sun.Foo";
        final String connection1 = "foo://bar";
        final String filter1 = "*.123";

        final String connection2 = "foo2://bar";
        final String filter2 = "*.123";

        final String midlet3 = "com.sun.Bar";
        final String connection3 = "qux://bar";
        final String filter3 = "*.*";

        final int midletSuiteId2 = midletSuiteId1 + 17;

        final String connection4 = "foo4://bar";
        final String filter4 = "4.*.123";

        final Store store = createStore();

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId1, midlet1,
                new MockReservationDescriptor(connection1, filter1));
        cc.registerConnection(midletSuiteId1, midlet1,
                new MockReservationDescriptor(connection2, filter2));
        cc.registerConnection(midletSuiteId1, midlet3,
                new MockReservationDescriptor(connection3, filter3));
        cc.registerConnection(midletSuiteId2, midlet1,
                new MockReservationDescriptor(connection4, filter4));

        assertSetsEqual(
                new String [] { connection1, connection2, connection3 },
                cc.listConnections(midletSuiteId1, false));

        final String [] suite2cns = cc.listConnections(midletSuiteId2, false);
        assertEquals(1, suite2cns.length);
        assertEquals(connection4, suite2cns[0]);
    }

    public void testListConnectionsWithData() throws IOException {
        final int midletSuiteId = 123;

        final String midlet1 = "com.sun.Foo";

        final String connection1 = "foo://bar";
        final String filter1 = "*.123";
        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection1, filter1);

        final String connection2 = "foo2://bar";
        final String filter2 = "*.123";
        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection2, filter2);

        final String midlet3 = "com.sun.Bar";

        final String connection3 = "qux://bar";
        final String filter3 = "*.*";
        final MockReservationDescriptor descriptor3 =
                new MockReservationDescriptor(connection3, filter3);

        final Store store = createStore();

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet1, descriptor1);
        cc.registerConnection(midletSuiteId, midlet1, descriptor2);
        cc.registerConnection(midletSuiteId, midlet3, descriptor3);

        descriptor1.connectionReservation.hasAvailableData_ = true;
        descriptor2.connectionReservation.hasAvailableData_ = false;
        descriptor3.connectionReservation.hasAvailableData_ = true;

        assertSetsEqual(
                new String [] { connection1, connection3 },
                cc.listConnections(midletSuiteId, true));
    }

    public void testUnregisterConnectionInEmptyController() throws IOException {
        final int midletSuiteId = 123;
        final String connection = "foo://bar";

        final Store store = createStore();

        final ConnectionController cc = createConnectionController(store);

        assertFalse(cc.unregisterConnection(midletSuiteId, connection));

        checkStoreEmpty(store);
        assertEquals(0, cc.listConnections(midletSuiteId, false).length);
        assertNull(cc.getMIDlet(midletSuiteId, connection));
        assertNull(cc.getFilter(midletSuiteId, connection));
    }

    public void testUnregisterRegisteredConnection() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor);
        assertTrue(cc.unregisterConnection(midletSuiteId, connection));

        assertTrue(descriptor.connectionReservation.isCancelled);
        checkStoreEmpty(store);
        assertEquals(0, cc.listConnections(midletSuiteId, false).length);
        assertNull(cc.getMIDlet(midletSuiteId, connection));
        assertNull(cc.getFilter(midletSuiteId, connection));
    }

    public void testUnregisterNotRegisteredConnection() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final String connection2 = "com.sun.Bar";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor);

        assertFalse(cc.unregisterConnection(midletSuiteId, connection2));

        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet, connection, filter);
        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter, cc.getFilter(midletSuiteId, connection));
    }

    public void testUnregisterOtherSuiteConnection() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final int midletSuiteId2 = midletSuiteId + 17;

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ConnectionController cc = createConnectionController(store);

        cc.registerConnection(midletSuiteId, midlet, descriptor);

        boolean securityExceptionThrown = false;
        try {
            cc.unregisterConnection(midletSuiteId2, connection);
        } catch (SecurityException sex) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);
        checkStoreHasSingleRecord(store,
                midletSuiteId, midlet, connection, filter);
        checkSingletonConnectionList(cc, midletSuiteId, connection);
        assertEquals(midlet, cc.getMIDlet(midletSuiteId, connection));
        assertEquals(filter, cc.getFilter(midletSuiteId, connection));
    }

    public void testDataAvailableListener() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId, midlet, descriptor);
        final Thread t = descriptor.connectionReservation.pingThread();
        t.start();
        try {
            t.join();
        } catch (InterruptedException ie) {
            fail("Unexpected InterruptedException: " + ie);
        }

        assertTrue(lifecycleAdapter.hasBeenInvokedOnceFor(midletSuiteId,
							  midlet));
    }

    public void testConcurrentCancellation() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId, midlet, descriptor);

        final DelayableThread t = descriptor.connectionReservation.pingThread();

        synchronized (t.lock) {
            // start the thread first...
            t.start();
            // ...but before listener starts, unregister connection...
            assertTrue(cc.unregisterConnection(midletSuiteId, connection));
            // ...now let listener proceed
        }
        try {
            t.join();
        } catch (InterruptedException ie) {
            fail("Unexpected InterruptedException: " + ie);
        }

        assertTrue(lifecycleAdapter.hasNotBeenInvoked());
    }

    public void testUninstallSuiteWithNoConnections() throws IOException {
        final Store store = createStore();
        final ConnectionController cc = createConnectionController(store);
        cc.removeSuiteConnections(239);
        checkStoreEmpty(store);
    }

    public void testUninstallWithTwoConnections() throws IOException {
        final int midletSuiteId = 123;

        final String midlet1 = "com.sun.Foo";
        final String midlet2 = "com.sun.Bar";

        final String connection1 = "foo://bar";
        final String filter1 = "*.123";
        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection1, filter1);

        final String connection2 = "foo://qux";
        final String filter2 = "*.*";
        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection2, filter2);

        final Store store = createStore();

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId, midlet1, descriptor1);
        cc.registerConnection(midletSuiteId, midlet2, descriptor2);
        cc.removeSuiteConnections(midletSuiteId);

        assertTrue(descriptor1.connectionReservation.isCancelled);
        assertTrue(descriptor2.connectionReservation.isCancelled);

        assertFalse(cc.unregisterConnection(midletSuiteId, connection1));
        assertFalse(cc.unregisterConnection(midletSuiteId, connection2));

        assertEquals(0, cc.listConnections(midletSuiteId, false).length);
        assertNull(cc.getMIDlet(midletSuiteId, connection1));
        assertNull(cc.getFilter(midletSuiteId, connection1));
        assertNull(cc.getMIDlet(midletSuiteId, connection2));
        assertNull(cc.getFilter(midletSuiteId, connection2));

        checkStoreEmpty(store);
    }

    public void testConcurrentCancellationAndUninstall() throws IOException {
        final int midletSuiteId = 123;

        final String midlet1 = "com.sun.Foo";
        final String midlet2 = "com.sun.Bar";

        final String connection1 = "foo://bar";
        final String filter1 = "*.123";
        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection1, filter1);

        final String connection2 = "foo://qux";
        final String filter2 = "*.*";
        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection2, filter2);

        final Store store = createStore();

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId, midlet1, descriptor1);
        cc.registerConnection(midletSuiteId, midlet2, descriptor2);

        final DelayableThread t1 = descriptor1.connectionReservation
	    .pingThread();
        final DelayableThread t2 = descriptor2.connectionReservation
	    .pingThread();

        synchronized (t1.lock) {
            synchronized (t2.lock) {
                // start the threads first...
                t1.start(); t2.start();
                // ...but before listeners starts, uninstall the suite...
                cc.removeSuiteConnections(midletSuiteId);
                // ...now let listeners proceed
            }
        }
        try {
            t1.join(); t2.join();
        } catch (InterruptedException ie) {
            fail("Unexpected InterruptedException: " + ie);
        }

        assertTrue(lifecycleAdapter.hasNotBeenInvoked());
    }

    private void assertSetsEqualDeep(
            final Object [] expected, final Object [] actual) {
        /**
         * IMPL_NOTE: haven't found better way yet.  Technically
         * speaking it's not even quite correct: imho there is
         * no guarantees that "same" hash sets will produce same
         * toArray's, but chances are high, really
         */
        final HashSet e = new HashSet(Arrays.asList(expected));
        final HashSet a = new HashSet(Arrays.asList(actual));
        Arrays.equals(e.toArray(), a.toArray());
    }

    public void testStateAfterDispose() throws IOException {
        final int midletSuiteId1 = 123;

        final String midlet1 = "com.sun.Foo";
        final String connection1 = "foo://bar";
        final String filter1 = "*.123";
        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection1, filter1);

        final String midlet2 = "com.sun.Bar";
        final String connection2 = "foo://qux";
        final String filter2 = "*.*";
        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection2, filter2);

        final int midletSuiteId2 = midletSuiteId1 + 17;

        final String midlet3 = "com.sun.Qux";
        final String connection3 = "foo4://bar";
        final String filter3 = "4.*.123";
        final MockReservationDescriptor descriptor3 =
                new MockReservationDescriptor(connection3, filter3);

        final Store store = createStore();

        final ConnectionController cc =
                createConnectionController(store);

        cc.registerConnection(midletSuiteId1, midlet1, descriptor1);
        cc.registerConnection(midletSuiteId1, midlet2, descriptor2);
        cc.registerConnection(midletSuiteId2, midlet3, descriptor3);
        cc.dispose();

        // Check that reservations have been canceled
        assertTrue(descriptor1.connectionReservation.isCancelled);
        assertTrue(descriptor2.connectionReservation.isCancelled);
        assertTrue(descriptor3.connectionReservation.isCancelled);

        // Check that there is nothing registered
        assertNull(cc.getMIDlet(midletSuiteId1, connection1));
        assertNull(cc.getFilter(midletSuiteId1, connection1));

        assertNull(cc.getMIDlet(midletSuiteId1, connection2));
        assertNull(cc.getFilter(midletSuiteId1, connection2));

        assertEquals(0, cc.listConnections(midletSuiteId1, false).length);

        assertNull(cc.getMIDlet(midletSuiteId2, connection3));
        assertNull(cc.getFilter(midletSuiteId2, connection3));

        assertEquals(0, cc.listConnections(midletSuiteId2, false).length);

        // But check that all the connections reside in the persistent store
        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int id, final ConnectionInfo [] infos) {
                switch (id) {
                case midletSuiteId1:
                    assertSetsEqualDeep(new ConnectionInfo [] {
                        new ConnectionInfo(midlet1, connection1, filter1),
                        new ConnectionInfo(midlet2, connection2, filter2),
                    }, infos);
                    break;

                case midletSuiteId2:
                    assertSetsEqualDeep(new ConnectionInfo [] {
                        new ConnectionInfo(midlet3, connection3, filter3),
                    }, infos);
                    break;

                default:
                    fail("Unexpected suite id");
                }
            }
        });
    }

    public void testDisposeCancellation() throws IOException {
        final int midletSuiteId1 = 123;

        final String midlet1 = "com.sun.Foo";
        final String connection1 = "foo://bar";
        final String filter1 = "*.123";
        final MockReservationDescriptor descriptor1 =
                new MockReservationDescriptor(connection1, filter1);

        final String midlet2 = "com.sun.Bar";
        final String connection2 = "foo://qux";
        final String filter2 = "*.*";
        final MockReservationDescriptor descriptor2 =
                new MockReservationDescriptor(connection2, filter2);

        final int midletSuiteId2 = midletSuiteId1 + 17;

        final String midlet3 = "com.sun.Qux";
        final String connection3 = "foo4://bar";
        final String filter3 = "4.*.123";
        final MockReservationDescriptor descriptor3 =
                new MockReservationDescriptor(connection3, filter3);

        final Store store = createStore();

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId1, midlet1, descriptor1);
        cc.registerConnection(midletSuiteId1, midlet2, descriptor2);
        cc.registerConnection(midletSuiteId2, midlet3, descriptor3);

        final DelayableThread t1 = descriptor1.connectionReservation
	    .pingThread();
        final DelayableThread t2 = descriptor2.connectionReservation
	    .pingThread();
        final DelayableThread t3 = descriptor3.connectionReservation
	    .pingThread();

        synchronized (t1.lock) {
            synchronized (t2.lock) {
                synchronized (t3.lock) {
                    // start the threads first...
                    t1.start(); t2.start(); t3.start();
                    // ...but before listeners starts, dispose the controller...
                    cc.dispose();
                    // ...now let listeners proceed
                }
            }
        }
        try {
            t1.join(); t2.join(); t3.join();
        } catch (InterruptedException ie) {
            fail("Unexpected InterruptedException: " + ie);
        }

        // Check that nothing has been launched
        assertTrue(lifecycleAdapter.hasNotBeenInvoked());
    }

    private static final class Registration {
        private final MIDPApp app;
        private final String connection;
        private final String filter;

        Registration(final int midletSuiteId, final String midlet,
                final String connection, final String filter) {
            this.app = new MIDPApp(midletSuiteId, midlet);
            this.connection = connection;
            this.filter = filter;
        }
    }

    public void testStartup() throws IOException {
        final int suiteId1 = 123;
        final int suiteId2 = 321;
        final Registration [] registrations = {
            new Registration(suiteId1, "com.sun.Foo", "foo://bar", "*.123"),
            new Registration(suiteId2, "com.sun.Foo", "foo:qux", "*.*.*"),
            new Registration(suiteId1, "com.sun.Qux", "qux:123", "*"),
        };

        final Store store = createStore();
        for (int i = 0; i < registrations.length; i++) {
            final Registration r = registrations[i];
            final ConnectionInfo info = new ConnectionInfo(
                    r.connection, r.app.midlet, r.filter);
            store.addConnection(r.app.midletSuiteID, info);
        }

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        assertSetsEqual(
                new String [] {
                    registrations[0].connection,
                    registrations[2].connection,
                }, cc.listConnections(suiteId1, false));
        assertSetsEqual(
                new String [] {
                    registrations[1].connection,
                }, cc.listConnections(suiteId2, false));

        // And now check both MIDlets and filters
        for (int i = 0; i < registrations.length; i++) {
            final Registration r = registrations[i];
            final int id = r.app.midletSuiteID;
            final String c = r.connection;
            assertEquals(r.app.midlet,  cc.getMIDlet(id, c));
            assertEquals(r.filter,      cc.getFilter(id, c));
        }
    }

    public void testAccessorsForAnotherSuite() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final int midletSuiteId2 = 1001 - midletSuiteId;

        final ConnectionController cc = 
	    createConnectionController(createStore());

        cc.registerConnection(midletSuiteId, midlet,
                new MockReservationDescriptor(connection, filter));

        assertNull(cc.getMIDlet(midletSuiteId2, connection));
        assertNull(cc.getFilter(midletSuiteId2, connection));
    }

    public void testAccessorsForUnregistered() throws IOException {
        final int midletSuiteId = 123;
        final String connection = "foo://bar";

        final ConnectionController cc = 
	    createConnectionController(createStore());

        assertNull(cc.getMIDlet(midletSuiteId, connection));
        assertNull(cc.getFilter(midletSuiteId, connection));
    }

    private void pingDescriptor(final MockReservationDescriptor descriptor) {
        final Thread t = descriptor.connectionReservation.pingThread();
        t.start();
        try {
            t.join();
        } catch (InterruptedException ie) {
            fail("Unexpected InterruptedException: " + ie);
        }
    }

    public void testThrowingLifecycleAdapter() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final MockReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);

        final ListingLifecycleAdapter listingLifecycleAdapter =
                new ListingLifecycleAdapter();

        final LifecycleAdapter throwingLifecycleAdapter =
                new ThrowingLifecycleAdapter();

        final ProxyLifecycleAdapter lifecycleAdapter =
                new ProxyLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        cc.registerConnection(midletSuiteId, midlet, descriptor);

        lifecycleAdapter.setProxy(throwingLifecycleAdapter);
        pingDescriptor(descriptor);

        lifecycleAdapter.setProxy(listingLifecycleAdapter);
        pingDescriptor(descriptor);
        assertTrue(listingLifecycleAdapter
                .hasBeenInvokedOnceFor(midletSuiteId, midlet));
    }
}
