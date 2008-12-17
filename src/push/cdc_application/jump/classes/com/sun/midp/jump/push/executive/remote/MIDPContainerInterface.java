/*
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

package com.sun.midp.jump.push.executive.remote;

import com.sun.midp.jump.push.share.JUMPReservationDescriptor;
import java.io.IOException;
import java.rmi.Remote;
import java.rmi.RemoteException;
import javax.microedition.io.ConnectionNotFoundException;

/** Remote interface for MIDP container. */
public interface MIDPContainerInterface extends Remote {
    /**
     * Registers connection reservation.
     *
     * <p>
     * <code>midletSuiteId</code> and <code>midlet</code> should refer
     * to valid <code>MIDlet</code>.
     * </p>
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to register
     *  connection for
     *
     * @param midlet <code>MIDlet</code> class name to reserve connection for
     * (cannot be <code>null</code>)
     *
     * @param reservationDescriptor reservation descriptor
     * (cannot be <code>null</code>)
     *
     * @throws IOException if connection cannot be reserved for the app
     * @throws RemoteException as requested by RMI spec.
     */
    void registerConnection(int midletSuiteId, String midlet,
            JUMPReservationDescriptor reservationDescriptor)
                throws IOException, RemoteException;

    /**
     * Unregisters connection.
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to unregister
     *  connection for
     *
     * @param connectionName connection to unregister
     * (cannot be <code>null</code>)
     *
     * @return <code>true</code> if the unregistration was successful,
     * <code>false</code> otherwise.
     *
     * @throws SecurityException if the connection was registered by
     * another <code>MIDlet</code>  suite
     * @throws RemoteException as requested by RMI spec.
     */
    boolean unregisterConnection(int midletSuiteId,
            String connectionName) throws SecurityException, RemoteException;

    /**
     * Lists all the connections registered by the suite.
     *
     * <p>
     * Close match to <code>PushRegistry.listConnections</code>
     * </p>
     *
     * @param midletSuiteId <code>MIDlet</code> suite to list connections for
     * @param available if <code>true</code>, list connections with available
     *  data
     *
     * @return connections
     *
     * @throws RemoteException as requested by RMI spec.
     */
    String [] listConnections(int midletSuiteId, boolean available)
        throws RemoteException;

    /**
     * Fetches the <code>MIDlet</code> by the connection.
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID to query for
     *
     * @param connectionName connection as passed into
     * {@link #registerConnection}
     * (cannot be <code>null</code>)
     *
     * @return <code>MIDlet</code> associated with <code>connection</code> or
     * <code>null</code> if there is no appropriate association
     *
     * @throws RemoteException as requested by RMI spec.
     */
    String getMIDlet(int midletSuiteID, String connectionName)
        throws RemoteException;

    /**
     * Fetches the filter by the connection.
     *
     * @param midletSuiteID <code>MIDlet</code> suite ID to query for
     *
     * @param connectionName connection as passed into
     * {@link #registerConnection}
     * (cannot be <code>null</code>)
     *
     * @return filter associated with <code>connection</code> or
     * <code>null</code> if there is no appropriate association
     *
     * @throws RemoteException as requested by RMI spec.
     */
    String getFilter(int midletSuiteID, String connectionName)
        throws RemoteException;

    /**
     * Registers an alarm.
     *
     * <p>
     * <code>midletSuiteId</code> and <code>midlet</code> should refer
     * to valid <code>MIDlet</code>.
     * </p>
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to register
     *  an alarm for
     *
     * @param midlet <code>MIDlet</code> class name to register an alarm for
     * (cannot be <code>null</code>)
     *
     * @param time alarm time
     *
     * @return time of previous registered (but not fired) alarm or 0
     *
     * @throws RemoteException as requested by RMI spec.
     * @throws ConnectionNotFoundException if it's impossible to register alarm
     */
    long registerAlarm(int midletSuiteId, String midlet, long time)
        throws RemoteException, ConnectionNotFoundException;
}
