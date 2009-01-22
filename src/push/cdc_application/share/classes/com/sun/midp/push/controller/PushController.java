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

import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.reservation.ReservationDescriptor;
import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;
import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Push controller.
 *
 * <p>
 * Manages both connections and alarms.
 * </p>
 */
public final class PushController {
    /** Alarm controller. */
    private final AlarmController alarmController;

    /** Connection controller. */
    private final ConnectionController connectionController;

    /**
     * Creates an implementation.
     *
     * @param store Store to use
     * (cannot be <code>null</code>)
     *
     * @param lifecycleAdapter lifecycle adapter to use
     * (cannot be <code>null</code>)
     *
     * @param reservationDescriptorFactory reservation descriptor factory
     * (cannot be <code>null</code>)
     */
    public PushController(
            final Store store,
            final LifecycleAdapter lifecycleAdapter,
            final ReservationDescriptorFactory reservationDescriptorFactory) {
        this.alarmController = new AlarmController(store, lifecycleAdapter);
        this.connectionController = new ConnectionController(
                store, reservationDescriptorFactory, lifecycleAdapter);
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
    public void registerConnection(
            final int midletSuiteID,
            final String midlet,
            final ReservationDescriptor reservationDescriptor) throws
                IOException {
        connectionController.registerConnection(
                midletSuiteID, midlet, reservationDescriptor);
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
    public boolean  unregisterConnection(
            final int midletSuiteID,
            final String connectionName) throws
                SecurityException {
        return connectionController.unregisterConnection(
                midletSuiteID, connectionName);
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
    public String[] listConnections(
            final int midletSuiteID,
            final boolean available) {
        return connectionController.listConnections(midletSuiteID, available);
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
        return connectionController.getMIDlet(midletSuiteID, connectionName);
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
        return connectionController.getFilter(midletSuiteID, connectionName);
    }

    /**
     * Registers an alarm.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> parameter should refer to a valid
     *  <code>MIDlet</code> suite and <code>midlet</code> should refer to
     *  valid <code>MIDlet</code> from the given suite. <code>timer</code>
     *  parameters is the same as for corresponding <code>Date</code>
     *  constructor.  No checks are performed and no guarantees are
     *  given if parameters are invalid.
     * </p>
     *
     * @param midletSuiteID <code>MIDlet suite</code> ID
     *
     * @param midlet <code>MIDlet</code> class name
     * (cannot be <code>null</code>)
     *
     * @param time alarm time
     *
     * @throws ConnectionNotFoundException if for any reason alarm cannot be
     *  registered
     *
     * @return previous alarm time or 0 if none
     */
    public long registerAlarm(
            final int midletSuiteID,
            final String midlet,
            final long time) throws ConnectionNotFoundException {
        return alarmController.registerAlarm(midletSuiteID, midlet, time);
    }

    /**
     * Removes Push-related information for the suite being uninstalled;
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without alarms.
     * </p>
     *
     * @param midletSuiteID ID of the suite to remove Push info for
     */
    public void removeSuiteInfo(final int midletSuiteID) {
        alarmController.removeSuiteAlarms(midletSuiteID);
        connectionController.removeSuiteConnections(midletSuiteID);
    }

    /**
     * Dumps Push-related information for the suite to the specified store;
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without alarms.
     * </p>
     *
     * @param midletSuiteID ID of the suite to dump Push info for
     * @param store dump destination
     * @throws IOException if dump fails
     */
    public void dumpSuiteInfo(final int midletSuiteID,  final Store store)
            throws IOException {
        alarmController.dumpSuiteAlarms(midletSuiteID, store);
        connectionController.dumpSuiteConnections(midletSuiteID, store);
    }

    /**
     * Disposes the controller.
     *
     * <p>
     * This method needs to be called when closing the system.
     * </p>
     */
    public void dispose() {
        alarmController.dispose();
        connectionController.dispose();
    }
}
