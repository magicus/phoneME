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

import com.sun.midp.push.gcf.ConnectionReservation;
import com.sun.midp.push.gcf.DataAvailableListener;
import com.sun.midp.push.gcf.ReservationDescriptor;
import com.sun.midp.jump.push.executive.persistence.Store;
import com.sun.midp.push.gcf.PermissionCallback;
import com.sun.midp.push.gcf.ReservationDescriptorFactory;
import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Push connection controller.
 *
 * TODO: system startup registration
 * TODO: mass operation for installation/uninstalltion (queries by suite id)
 */
public final class ConnectionController {
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
                reservations.queryByConnection(connectionName);
        if (previous != null) {
            if (previous.getSuiteId() != midletSuiteID) {
                // Already registered for another suite, fail quickly
                throw new IOException("registered for another suite");
            }
            removeRegistration(previous);
        }

        final ReservationHandler reservationHandler = new ReservationHandler(
                midletSuiteID, midlet, reservationDescriptor);

        final String filter = reservationDescriptor.getFilter();

        try {
            // TODO: rethink if we need filter in JUMPConnectionInfo
            final JUMPConnectionInfo connectionInfo = new JUMPConnectionInfo(
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
     * @param connection connection to unregister
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
            final String connection) throws
                SecurityException {
        final ReservationHandler reservationHandler =
                reservations.queryByConnection(connection);

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
        final JUMPConnectionInfo info = new JUMPConnectionInfo(
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

        final Iterator it = reservations.queryBySuiteID(midletSuiteID);
        while (it.hasNext()) {
            final ReservationHandler handler = (ReservationHandler) it.next();
            if ((!available) || handler.hasAvailableData()) {
                result.add(handler.getConnectionName());
            }
        }
        return (String[]) result.toArray(new String[result.size()]);
    }

    /**
     * Noop permission callback for startup connection reservation.
     *
     * IMPL_NOTE: hopefully will go away when we'll get rid of
     * reservationDescriptorFactory
     */
    private final PermissionCallback noopPermissionCallback =
            new PermissionCallback() {
        public void checkForPermission(
                final String permissionName,
                final String resource,
                final String extraValue) throws SecurityException { }
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
                    final JUMPConnectionInfo [] connections) {
                for (int i = 0; i < connections.length; i++) {
                    final JUMPConnectionInfo info = connections[i];
                    try {
                        registerConnection(suiteId, info.midlet,
                                reservationDescriptorFactory.getDescriptor(
                                    info.connection, info.filter,
                                    noopPermissionCallback));
                    } catch (ConnectionNotFoundException cnfe) {
                        // TBD: proper logging
                    } catch (IOException ioex) {
                        // TBD: proper logging
                    }
                }
            }
        });
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
        /** <code>MIDlet</code> suite ID of reservation. */
        private final int midletSuiteID;

        /** <code>MIDlet</code> class name of reservation. */
        private final String midlet;

        /** Connection name. */
        private final String connectionName;

        /** Connection filter. */
        private final String filter;

        /** Connection reservation. */
        private final ConnectionReservation connectionReservation;

        /** Cancelation status. */
        private boolean canceled = false;

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
            this.midletSuiteID = midletSuiteID;
            this.midlet = midlet;

            this.connectionName = reservationDescriptor.getConnectionName();
            this.filter = reservationDescriptor.getFilter();

            this.connectionReservation =
                    reservationDescriptor.reserve(midletSuiteID, midlet, this);
        }

        /**
         * Gets <code>MIDlet</code> suite ID.
         *
         * @return <code>MIDlet</code> suite ID
         */
        int getSuiteId() {
            return midletSuiteID;
        }

        /**
         * Gets <code>MIDlet</code> class name.
         *
         * @return <code>MIDlet</code> class name
         */
        String getMidlet() {
            return midlet;
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
            canceled = true;
            connectionReservation.cancel();
        }

        /**
         * See {@link ConnectionReservation#hasAvailableData}.
         *
         * @return <code>true</code> iff reservation has available data
         */
        boolean hasAvailableData() {
            return connectionReservation.hasAvailableData();
        }

        /** {@inheritDoc} */
        public void dataAvailable() {
            synchronized (ConnectionController.this) {
                if (canceled) {
                    return;
                }

                lifecycleAdapter.launchMidlet(midletSuiteID, midlet);
            }
        }
    }

    /** Internal structure that manages needed mappings. */
    static final class Reservations {
        /** Mappings from connection to reservations. */
        private final HashMap connection2data = new HashMap();

        /** Mappings from suite id to set of reservations. */
        private final HashMap suiteId2data = new HashMap();

        /**
         * Adds a reservation into reservations.
         *
         * @param reservationHandler reservation to add
         */
        void add(final ReservationHandler reservationHandler) {
            connection2data.put(
                    reservationHandler.getConnectionName(), reservationHandler);

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
            connection2data.remove(reservationHandler.getConnectionName());

            getData(reservationHandler.getSuiteId()).remove(reservationHandler);
        }

        /**
         * Queries the reservations by the connection.
         *
         * @param connection connection to query by
         * (cannot be <code>null</code>)
         * @return reservation (<code>null</code> if absent)
         */
        ReservationHandler queryByConnection(final String connection) {
            return (ReservationHandler) connection2data.get(connection);
        }

        /**
         * Queries the reservations by the suite id.
         *
         * @param midletSuiteID <code>MIDlet</code> suite ID to query by
         * @return iterator of <code>ReservationHandler</code>s
         * (cannot be <code>null</code>)
         */
        Iterator queryBySuiteID(final int midletSuiteID) {
            final Set data = getData(midletSuiteID);
            return ((data == null) ? new HashSet() : data).iterator();
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
}
