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

package com.sun.midp.jump.push.executive;

import com.sun.midp.push.gcf.PermissionCallback;
import com.sun.midp.push.gcf.ReservationDescriptorFactory;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Vector;
import junit.framework.*;
import com.sun.midp.push.gcf.ConnectionReservation;
import com.sun.midp.push.gcf.DataAvailableListener;
import com.sun.midp.push.gcf.ReservationDescriptor;
import com.sun.midp.jump.push.executive.persistence.Store;
import com.sun.midp.jump.push.executive.persistence.StoreUtils;
import java.io.IOException;
import java.util.Iterator;
import java.util.Set;
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
    }

    private static Store createStore() throws IOException {
        return StoreUtils.createInMemoryPushStore();
    }

    private final ReservationDescriptorFactory rdf =
            new ReservationDescriptorFactory () {
        public ReservationDescriptor getDescriptor(
                final String connectionName, final String filter,
                final PermissionCallback permissionCallback)
                    throws IllegalArgumentException,
                        ConnectionNotFoundException {
            if (permissionCallback == null) {
                throw new RuntimeException("null permission callback");
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
        /*
         * IMPL_NOTE: stricktly speaking, <code>null</code>
         * lifecycle adapters are prohibited, but they work currnently
         * unless invoked
         */
        return createConnectionController(store, null);
    }

    private ConnectionController.ReservationHandler
        createFakeReservationHandler(
            final ConnectionController connectionController,
            final int midletSuiteId,
            final String midlet,
            final String connection,
            final String filter) throws IOException {
        final ReservationDescriptor descriptor =
                new MockReservationDescriptor(connection, filter);
        return connectionController
                .new ReservationHandler(midletSuiteId, midlet, descriptor);
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

    public void testQueryByConnection() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Database db =
                new ConnectionController.Database();
        db.add(h);

        assertSame(h, db.queryByConnection(connection));
    }

    public void testQueryByConnectionMissing() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Database db =
                new ConnectionController.Database();
        db.add(h);

        assertNull(db.queryByConnection(connection + "qux"));
    }

    public void testQueryBySuite() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Database db =
                new ConnectionController.Database();
        db.add(h);

        final Iterator it = db.queryBySuiteID(midletSuiteId);
        assertTrue(it.hasNext());
        assertSame(h, it.next());
        assertFalse(it.hasNext());
    }

    public void testQueryBySuiteMissing() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Database db =
                new ConnectionController.Database();
        db.add(h);

        assertFalse(db.queryBySuiteID(midletSuiteId + 1).hasNext());
    }

    public void testEmptyDatabase() {
        final ConnectionController.Database db =
                new ConnectionController.Database();

        assertNull(db.queryByConnection("foo://bar"));
        assertFalse(db.queryBySuiteID(13).hasNext());
    }

    public void testAddAndRemove() throws IOException {
        final int midletSuiteId = 123;
        final String midlet = "com.sun.Foo";
        final String connection = "foo://bar";
        final String filter = "*.123";

        final ConnectionController.ReservationHandler h =
                createFakeReservationHandler(
                    midletSuiteId, midlet, connection, filter);

        final ConnectionController.Database db =
                new ConnectionController.Database();
        db.add(h);
        db.remove(h);

        final Iterator it = db.queryBySuiteID(midletSuiteId);
        assertNull(db.queryByConnection(connection));
        assertFalse(db.queryBySuiteID(midletSuiteId).hasNext());
    }

    private void checkStoreEmpty(final Store store) {
        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int id, final JUMPConnectionInfo [] infos) {
                fail("store must be empty");
            }
        });
    }

    private void checkStoreHasSingleRecord(
            final Store store,
            final int midletSuiteId, final String midlet,
            final String connection, final String filter) {
        final JUMPConnectionInfo info =
                new JUMPConnectionInfo(connection, midlet, filter);

        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int id, final JUMPConnectionInfo [] infos) {
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

        assertEquals(0, cc.listConnections(midletSuiteId2, false).length);
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

        final String [] suite1cns = cc.listConnections(midletSuiteId1, false);
        final Set expected = new HashSet(Arrays.asList(new String [] {
            connection3, connection2, connection1
        }));
        final Set actual = new HashSet(Arrays.asList(suite1cns));
        assertEquals(expected, actual);

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

        final String [] cns = cc.listConnections(midletSuiteId, true);

        final Set expected = new HashSet(Arrays.asList(new String [] {
            connection3, connection1
        }));
        final Set actual = new HashSet(Arrays.asList(cns));
        assertEquals(expected, actual);
    }

    public void testUnregisterConnectionInEmptyController() throws IOException {
        final int midletSuiteId = 123;
        final String connection = "foo://bar";
        final String filter = "*.123";

        final Store store = createStore();

        final ConnectionController cc = createConnectionController(store);

        assertFalse(cc.unregisterConnection(midletSuiteId, connection));

        checkStoreEmpty(store);
        assertEquals(0, cc.listConnections(midletSuiteId, false).length);
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
    }

    private static final class App {
        final int midletSuiteID;
        final String midlet;

        App(final int midletSuiteID, final String midlet) {
            this.midletSuiteID = midletSuiteID;
            this.midlet = midlet;
        }

        /** {@inheritDoc} */
        public boolean equals(Object obj) {
            if (!(obj instanceof App)) {
                return false;
            }

            final App rhs = (App) obj;
            return (midletSuiteID == rhs.midletSuiteID)
                && (midlet.equals(rhs.midlet));
        }

        /** {@inheritDoc} */
        public int hashCode() {
            return (midletSuiteID << 7) + midlet.hashCode();
        }
    }

    private static final class ListingLifecycleAdapter
            implements LifecycleAdapter {

        Vector apps = new Vector();

        public void launchMidlet(final int midletSuiteID, final String midlet) {
            apps.add(new App(midletSuiteID, midlet));
        }
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

        final Vector apps = lifecycleAdapter.apps;
        assertEquals(1, apps.size());
        assertEquals(new App(midletSuiteId, midlet), apps.get(0));
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

        final Vector apps = lifecycleAdapter.apps;
        assertTrue(apps.isEmpty());
    }

    private static final class Registration {
        private final App app;
        private final String connection;
        private final String filter;

        Registration(final int midletSuiteId, final String midlet,
                final String connection, final String filter) {
            this.app = new App(midletSuiteId, midlet);
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
            final JUMPConnectionInfo info = new JUMPConnectionInfo(
                    r.connection, r.app.midlet, r.filter);
            store.addConnection(r.app.midletSuiteID, info);
        }

        final ListingLifecycleAdapter lifecycleAdapter =
                new ListingLifecycleAdapter();

        final ConnectionController cc =
                createConnectionController(store, lifecycleAdapter);

        /*
         * IMPL_NOTE: Unfortunately, listConnections doesn't return information
         *  about filter and midlet, so test is somewhat incomplete
         */
        final String [] suite1cns = cc.listConnections(suiteId1, false);
        assertEquals(
                new HashSet(Arrays.asList(suite1cns)),
                new HashSet(Arrays.asList(new String [] {
                    registrations[0].connection,
                    registrations[2].connection,
        })));

        final String [] suite2cns = cc.listConnections(suiteId2, false);
        assertTrue(Arrays.equals(
                suite2cns,
                new String [] { registrations[1].connection }));
    }
}
