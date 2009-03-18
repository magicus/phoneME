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

import com.sun.midp.push.reservation.ConnectionReservation;
import com.sun.midp.push.reservation.DataAvailableListener;
import com.sun.midp.push.reservation.ReservationDescriptor;
import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;
import com.sun.j2me.security.AccessControlContext;
import com.sun.j2me.security.AccessControlContextAdapter;
import java.io.IOException;
import java.util.Collection;
import java.util.TreeMap;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.Map;
import java.util.Vector;
import javax.microedition.io.ConnectionNotFoundException;

/** Push connection controller. */
final class ConnectionController {
    /** Store to save connection info. */
    private final Store store;

    /**
     * Reservation descriptor factory.
     *
     * IMPL_NOTE: hopefully will go away
     */
    private final ReservationDescriptorFactory reservationDescriptorFactory;

    /** Lifecycle adapter implementation. */
    private final LifecycleAdapter lifecycleAdapter;

    /** Current reservations. */
    private final Reservations reservations;

    /**
     * Creates an instance.
     *
     * @param store persistent store to save connection info into
     * (cannot be <code>null</code>)
     *
     * @param reservationDescriptorFactory reservation descriptor factory
     * (cannot be <code>null</code>
     *
     * @param lifecycleAdapter adapter to launch <code>MIDlet</code>
     * (cannot be <code>null</code>)
     */
    public ConnectionController(
            final Store store,
            final ReservationDescriptorFactory reservationDescriptorFactory,
            final LifecycleAdapter lifecycleAdapter) {
        this.store = store;
        this.reservationDescriptorFactory = reservationDescriptorFactory;
        this.lifecycleAdapter = lifecycleAdapter;
        this.reservations = new Reservations();

        reserveConnectionsFromStore();
    }

    /**
     * Registers the connection.
     *
     * <p>
     * Saves the connection into persistent store and reserves it
     * for <code>MIDlet</code>.
     * </p>
     *
     * <p>
     * The connection should be already preverified (see
     * <code>reservationDescriptor</code> parameter) and all the security
     * checks should be performed.
     * </p>
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID
     *
     * @param midlet <code>MIDlet</code> class name
     * (cannot be <code>null</code>)
     *
     * @param reservationDescriptor reservation descriptor
     * (cannot be <code>null</code>)
     *
     * @throws IOException if the connection is already registered or
     * if there are insufficient resources to handle the registration request
     */
    public synchronized void registerConnection(
            final int midletSuiteID,
            final String midlet,
            final ReservationDescriptor reservationDescriptor) throws
                IOException {
        final String connectionName = reservationDescriptor.getConnectionName();

        /*
         * IMPL_NOTE: due to ReservationDescriptor#reserve limitations,
         * need to unregister registered connection first
         */
        final ReservationHandler previous =
                reservations.queryByConnectionName(connectionName);
        if (previous != null) {
            throw new IOException("already registered");
        }

        final ReservationHandler reservationHandler = new ReservationHandler(
                midletSuiteID, midlet, reservationDescriptor);

        final String filter = reservationDescriptor.getFilter();

        try {
            // TODO: rethink if we need filter in ConnectionInfo
            final ConnectionInfo connectionInfo = new ConnectionInfo(
                    connectionName, midlet, filter);
            store.addConnection(midletSuiteID, connectionInfo);
        } catch (IOException ioex) {
            reservationHandler.cancel();
            throw ioex; // rethrow IOException
        }

        reservations.add(reservationHandler);
    }

    /**
     * Unregisters the connection.
     *
     * <p>
     * Removes the connection from persistent store and cancels connection
     * reservation.
     * </p>
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID
     *
     * @param connectionName connection to unregister
     * (cannot be <code>null</code>)
     *
     * @return <code>true</code> if the unregistration was successful,
     * <code>false</code> if the connection was not registered
     *
     * @throws SecurityException if the connection was registered by
     * another <code>MIDlet</code>  suite
     */
    public synchronized boolean  unregisterConnection(
            final int midletSuiteID,
            final String connectionName) throws
                SecurityException {
        final ReservationHandler reservationHandler =
                reservations.queryByConnectionName(connectionName);

        if (reservationHandler == null) {
            // Connection hasn't been registered
            return false;
        }

        if (reservationHandler.getSuiteId() != midletSuiteID) {
            throw new SecurityException(
                    "attempt to unregister connection of another suite");
        }

        try {
            removeRegistration(reservationHandler);
        } catch (IOException ioex) {
            return false;
        }

        return true;
    }

    /**
     * Transactionally removes the registration.
     *
     * @param reservationHandler reservation handler.
     *
     * @throws IOException if persistent store fails
     */
    private void removeRegistration(final ReservationHandler reservationHandler)
            throws IOException {
        final ConnectionInfo info = new ConnectionInfo(
                reservationHandler.getConnectionName(),
                reservationHandler.getMidlet(),
                reservationHandler.getFilter());
        store.removeConnection(reservationHandler.getSuiteId(), info);
        reservationHandler.cancel();
        reservations.remove(reservationHandler);
    }

    /**
     * Returns a list of registered connections for <code>MIDlet</code> suite.
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID
     *
     * @param available if <code>true</code>, only return
     * the list of connections with input available, otherwise
     * return the complete list of registered connections for
     * <code>MIDlet</code> suite
     *
     * @return array of registered connection strings, where each connection
     * is represented by the generic connection <em>protocol</em>,
     * <em>host</em> and <em>port</em> number identification
     */
    public synchronized String[] listConnections(
            final int midletSuiteID,
            final boolean available) {
        final Vector result = new Vector();

        final Iterator it = reservations
                .queryBySuiteID(midletSuiteID).iterator();
        while (it.hasNext()) {
            final ReservationHandler handler = (ReservationHandler) it.next();
            if ((!available) || handler.hasAvailableData()) {
                result.add(handler.getConnectionName());
            }
        }
        return (String[]) result.toArray(new String[result.size()]);
    }

    /**
     * Fetches the <code>MIDlet</code> by the connection.
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID to query for
     *
     * @param connectionName connectionName as passed into
     * {@link #registerConnection}
     * (cannot be <code>null</code>)
     *
     * @return <code>MIDlet</code> associated with <code>connectionName</code>
     * or <code>null</code> if there is no appropriate association
     */
    public synchronized String getMIDlet(
            final int midletSuiteID, final String connectionName) {
        final ReservationHandler reservationHandler =
                reservations.queryByConnectionName(connectionName);

        if ((reservationHandler == null)
            || (reservationHandler.getSuiteId() != midletSuiteID)) {
            return null;
        }

        return reservationHandler.getMidlet();
    }

    /**
     * Fetches the filter by the connection.
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID to query for
     *
     * @param connectionName connectionName as passed into
     * {@link #registerConnection}
     * (cannot be <code>null</code>)
     *
     * @return filter associated with <code>connectionName</code> or
     * <code>null</code> if there is no appropriate association
     */
    public synchronized String getFilter(
            final int midletSuiteID, final String connectionName) {
        final ReservationHandler reservationHandler =
                reservations.queryByConnectionName(connectionName);

        if ((reservationHandler == null)
            || (reservationHandler.getSuiteId() != midletSuiteID)) {
            return null;
        }

        return reservationHandler.getFilter();
    }

    /**
     * Removes connections for the given suite.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without connections.
     * </p>
     *
     * @param midletSuiteID ID of the suite to remove connections for
     */
    public synchronized void removeSuiteConnections(final int midletSuiteID) {
        /*
         * IMPL_NOTE: one shouldn't remove and iterate in same time.
         * The solution is to copy first.  It's safe for method is synchronized.
         */
        final Collection rs = reservations.queryBySuiteID(midletSuiteID);
        final ReservationHandler [] handlers =
                new ReservationHandler [rs.size()];
        rs.toArray(handlers);
        for (int i = 0; i < handlers.length; i++) {
            try {
                removeRegistration(handlers[i]);
            } catch (IOException ioex) {
                logError("failed to remove " + handlers[i] + ": " + ioex);
            }
        }
    }

    /**
     * Dumps connections for the given suite.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without connections.
     * </p>
     *
     * @param midletSuiteID ID of the suite to dump connections for
     * @param store dump destination
     * @throws IOException if dump fails
     */
    public synchronized void dumpSuiteConnections(
            final int midletSuiteID, final Store store) throws IOException {

        class ConnectionsConsumer implements Store.ConnectionsConsumer {
            private IOException ex;

            ConnectionsConsumer() throws IOException {
                ConnectionController.this.store.listConnections(this);
                if (ex != null) {
                    throw ex;
                }
            }

            public void consume(
                    final int suiteId, final ConnectionInfo [] connections) {
                if (midletSuiteID != suiteId) {
                    return;
                }

                ConnectionInfo info = null;
                try {
                    for (int i = 0; i < connections.length; i++) {
                        info = connections[i];
                        store.addConnection(midletSuiteID, info);
                    }
                } catch (IOException e) {
                    logError("failed to store " + info + ": " + e);
                    ex = e;
                }
            }
        }

        new ConnectionsConsumer();
    }

    /**
     * Disposes a connection controller.
     *
     * <p>
     * Cancels all the reservations and callbacks.
     * </p>
     *
     * <p>
     * The only thing one MUST do with disposed
     * <code>ConnectionController</code> is to garbage-collect it.
     * </p>
     */
    public synchronized void dispose() {
        for (Iterator it = reservations.getAllReservations().iterator();
                it.hasNext(); ) {
            final ReservationHandler rh = (ReservationHandler) it.next();
            rh.cancel();
        }
        reservations.clear();
    }

    /**
     * Noop permission checker for startup connection reservation.
     *
     * IMPL_NOTE: hopefully will go away when we'll get rid of
     * reservationDescriptorFactory
     */
    private final AccessControlContext noopAccessControlContext =
            new AccessControlContextAdapter() {
        public void checkPermissionImpl(
                        String name, String resource, String extraValue) {}
    };

    /**
     * Reads and reserves connections in persistent store.
     */
    private void reserveConnectionsFromStore() {
        /*
         * IMPL_NOTE: currently connection info is stored as plain data.
         * However, as reservation descriptors should be serializable
         * for jump, it might be a good idea to store descriptors
         * directly and thus get rid of reservation factory in
         * ConnectionController altogether.
         */
        store.listConnections(new Store.ConnectionsConsumer() {
            public void consume(
                    final int suiteId,
                    final ConnectionInfo [] connections) {
                for (int i = 0; i < connections.length; i++) {
                    final ConnectionInfo info = connections[i];
                    try {
                        registerConnection(suiteId, info.midlet,
                                reservationDescriptorFactory.getDescriptor(
                                    info.connection, info.filter,
                                    noopAccessControlContext));
                    } catch (ConnectionNotFoundException cnfe) {
                        logError("failed to register " + info + ": " + cnfe);
                    } catch (IOException ioex) {
                        logError("failed to register " + info + ": " + ioex);
                    }
                }
            }
        });
    }

    /**
     * Key to compare ReservationDescriptor-s for equality by connection name.
     */
    final static class ReservationNameKey implements Comparable {
        /** Connection name. */
        private String connectionName;

        /** Reservation descriptor. */
        private final ReservationDescriptor reservationDescriptor;

        ReservationNameKey(ReservationDescriptor reservationDescriptor) {
            this.connectionName = reservationDescriptor.getConnectionName();
            this.reservationDescriptor = reservationDescriptor;
        }

        ReservationNameKey(String connectionName) {
            this.connectionName = connectionName;
            this.reservationDescriptor = null;
        }

        public int compareTo(Object o) {
            final ReservationNameKey key = (ReservationNameKey)o;

            if (key.reservationDescriptor != null) {
                if (key.reservationDescriptor.isConnectionNameEquivalent(
                        connectionName)) {
                    return 0;
                }
            } else if (reservationDescriptor != null) {
                if (reservationDescriptor.isConnectionNameEquivalent(
                        key.connectionName)) {
                    return 0;
                }
            }

            return connectionName.compareTo(key.connectionName);
        }

        void setConnectionName(String connectionName) {
            if (reservationDescriptor != null) {
                throw new RuntimeException();
            }

            this.connectionName = connectionName;
        }
    }

    /**
     * Reservation listening handler.
     *
     * <p>
     * IMPL_NOTE: invariant is: one instance of a handler per registration
     * and thus default Object <code>equals</code> and <code>hashCode</code>
     * should be enough
     * </p>
     *
     * <p>
     * TODO: think if common code with AlarmRegistry.AlarmTask can be factored
     * </p>
     */
    final class ReservationHandler implements DataAvailableListener {
        /** Reservation's app. */
        private final MIDPApp midpApp;

        /** Connection name. */
        private final String connectionName;

        /** Connection filter. */
        private final String filter;

        /** Connection reservation. */
        private final ConnectionReservation connectionReservation;

        /** Key to use for comparison this object with other reservations. */
        private final ReservationNameKey key;

        /** Cancelation status. */
        private boolean cancelled = false;

        /**
         * Creates a handler and reserves the connection.
         *
         * @param midletSuiteID <code>MIDlet</code> suite ID of reservation
         *
         * @param midlet <code>MIDlet</code> suite ID of reservation.
         * (cannot be <code>null</code>)
         *
         * @param reservationDescriptor reservation descriptor
         * (cannot be <code>null</code>)
         *
         * @throws IOException if reservation fails
         */
        ReservationHandler(
                final int midletSuiteID, final String midlet,
                final ReservationDescriptor reservationDescriptor)
                    throws IOException {
            this.midpApp = new MIDPApp(midletSuiteID, midlet);

            this.connectionName = reservationDescriptor.getConnectionName();
            this.filter = reservationDescriptor.getFilter();

            this.connectionReservation =
                    reservationDescriptor.reserve(midletSuiteID, midlet, this);

            this.key = new ReservationNameKey(reservationDescriptor);
        }

        /**
         * Gets <code>MIDlet</code> suite ID.
         *
         * @return <code>MIDlet</code> suite ID
         */
        int getSuiteId() {
            return midpApp.midletSuiteID;
        }

        /**
         * Gets <code>MIDlet</code> class name.
         *
         * @return <code>MIDlet</code> class name
         */
        String getMidlet() {
            return midpApp.midlet;
        }

        /**
         * Gets connection name.
         *
         * @return connection name
         */
        String getConnectionName() {
            return connectionName;
        }

        /**
         * Gets connection filter.
         *
         * @return connection filter
         */
        String getFilter() {
            return filter;
        }

        /**
         * Cancels reservation.
         */
        void cancel() {
            cancelled = true;
            connectionReservation.cancel();
        }

        /**
         * Returns comparison key.
         */
        ReservationNameKey getKey() {
            return key;
        }

        /**
         * See {@link ConnectionReservation#hasAvailableData}.
         *
         * @return <code>true</code> if reservation has available data
         */
        boolean hasAvailableData() {
            return connectionReservation.hasAvailableData();
        }

        /** {@inheritDoc} */
        public void dataAvailable() {
            if (cancelled) {
                return;
            }

            try {
                lifecycleAdapter.launchMidlet(midpApp.midletSuiteID,
                        midpApp.midlet);
            } catch (Exception ex) {
                /* IMPL_NOTE: need to handle _all_ the exceptions. */
                /*
                 * TBD: uncomment when logging can be disabled
                 * (not to interfer with unittests)
                 * logError(
                 *      "Failed to launch \"" + midpApp.midlet + "\"" +
                 *       " (suite ID: " + midpApp.midletSuiteID + "): " +
                 *       ex);
                 */
            }
        }
    }

    /** Internal structure that manages needed mappings. */
    static final class Reservations {
        /** Mappings from connection to reservations. */
        private final Map connection2data = new TreeMap();

        /** Mappings from suite id to set of reservations. */
        private final Map suiteId2data = new HashMap();

        /** Preallocated ReservationNameKey to avoid temporary object creation. */
        private final ReservationNameKey keyPlaceholder =
            new ReservationNameKey((String)null);

        /**
         * Adds a reservation into reservations.
         *
         * @param reservationHandler reservation to add
         */
        void add(final ReservationHandler reservationHandler) {
            connection2data.put(reservationHandler.getKey(), reservationHandler);

            final Set data = getData(reservationHandler.getSuiteId());
            if (data != null) {
                data.add(reservationHandler);
            } else {
                final Set d = new HashSet();
                d.add(reservationHandler);
                suiteId2data.put(new Integer(reservationHandler.getSuiteId()),
                        d);
            }
        }

        /**
         * Removes a reservation from reservations.
         *
         * @param reservationHandler reservation to remove
         */
        void remove(final ReservationHandler reservationHandler) {
            connection2data.remove(reservationHandler.getKey());

            getData(reservationHandler.getSuiteId()).remove(reservationHandler);
        }

        /**
         * Queries the reservations by the connection.
         *
         * @param connectionName name of connection to query by
         * (cannot be <code>null</code>)
         *
         * @return reservation (<code>null</code> if absent)
         */
        ReservationHandler queryByConnectionName(final String connectionName) {
            keyPlaceholder.setConnectionName(connectionName);
            return (ReservationHandler) connection2data.get(keyPlaceholder);
        }

        /**
         * Queries the reservations by the suite id.
         *
         * @param midletSuiteID <code>MIDlet</code> suite ID to query by
         * @return collection of <code>ReservationHandler</code>s
         * (cannot be <code>null</code>)
         */
        Collection queryBySuiteID(final int midletSuiteID) {
            final Set data = getData(midletSuiteID);
            return (data == null) ? new HashSet() : data;
        }

        /**
         * Gets all the reservations.
         *
         * @return collection of reservations
         */
        Collection getAllReservations() {
            return connection2data.values();
        }

        /**
         * Clears all the reservations.
         */
        void clear() {
            connection2data.clear();
            suiteId2data.clear();
        }

        /**
         * Gets reservation set by suite id.
         *
         * @param midletSuiteID <code>MIDlet</code> suite ID
         *
         * @return set of reservations or <code>null</code> if absent
         */
        private Set getData(final int midletSuiteID) {
            return (Set) suiteId2data.get(new Integer(midletSuiteID));
        }
    }

    /**
     * Logs error message.
     *
     * <p>
     * TBD: common logging
     * </p>
     *
     * @param message message to log
     */
    private static void logError(final String message) {
        System.err.println(
                "ERROR [" + ConnectionController.class.getName() + "]: "
                + message);
    }
}
