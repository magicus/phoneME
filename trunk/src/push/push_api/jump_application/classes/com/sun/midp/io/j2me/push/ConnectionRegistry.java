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

package com.sun.midp.io.j2me.push;

import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

import com.sun.midp.midlet.MIDletSuite;

/**
 * JUMP Implementation of ConnectionRegistry.
 */
final class ConnectionRegistry {
    /**
     * Hides constructor.
     */
    private ConnectionRegistry() { }

    /**
     * Registers a connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.registerConnection</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet suite</code> to register connection for
     * @param connection Connection to register
     * @param midlet Class to invoke
     * @param filter Connection filter
     *
     * @throws ClassNotFoundException If <code>midlet</code> references wrong
     *  class
     * @throws IOException If connection cannot be registered
     */
    public static void registerConnection(
            final MIDletSuite midletSuite,
            final Connection connection,
            final String midlet,
            final String filter) throws ClassNotFoundException, IOException {
        /*
         * Should never get here currently as <code>checkRegistration</code>
         * should abort registration earlier
         */
    }

    /**
     * Check the registration arguments.
     *
     * @param connection connection to check
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *  when new external data is available
     * @param filter a connection URL string indicating which senders
     *  are allowed to cause the MIDlet to be launched
     *
     * @throws IllegalArgumentException if connection or filter string is not valid
     * @throws ConnectionNotFoundException if PushRegistry doesn't support
     *  this kind of connections
     */
    static void checkRegistration(
            final Connection connection,
            final String midlet,
            final String filter)
            throws ConnectionNotFoundException {
        // No implemented connections so far
        throw new ConnectionNotFoundException();
    }

    /**
     * Unregisters a connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.unregisterConnection</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet</code> suite to unregister connection
     *             for
     * @param connection Connection to unregister
     *
     * @return was unregistration succesful or not
     */
    public static boolean unregisterConnection(
            final MIDletSuite midletSuite,
            final String connection) {
        // As we cannot register connections, we cannot unregister them as well
        return false;
    }

    /**
     * Lists all connections.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.listConnections</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet</code> suite to list connections for
     * @param available if <code>true</code>, list connections with available
     *  data
     *
     * @return connections
     */
    public static String [] listConnections(
            final MIDletSuite midletSuite,
            final boolean available) {
        // No connections so far
        return new String [0];
    }

    /**
     * Gets a <code>MIDlet</code> class name of connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.getMIDlet</code>
     * </p>
     *
     * @param connection Connection to look <code>MIDlet</code> for
     *
     * @return <code>MIDlet</code> name
     */
    public static String getMIDlet(final String connection) {
        // As we cannot register connections for now...
        return null;
    }

    /**
     * Gets a filter of connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.getFilter</code>
     * </p>
     *
     * @param connection Connection to look a filter for
     *
     * @return Filter
     */
    public static String getFilter(final String connection) {
        // As we cannot register connections for now...
        return null;
    }

    /**
     * Registers time alaram.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.registerAlarm</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet</code> suite to register alarm for
     * @param midlet Class to invoke
     * @param time Time to invoke
     *
     * @return Previous time to invoke
     *
     * @throws ClassNotFoundException If <code>midlet</code> references wrong
     *  class
     * @throws ConnectionNotFoundException If the system
     *  doesn't support alarms
     */
    public static long registerAlarm(
            final MIDletSuite midletSuite,
            final String midlet,
            final long time)
            throws ClassNotFoundException, ConnectionNotFoundException {
        // As we cannot register connections for now...
        throw new ConnectionNotFoundException();
    }
}
