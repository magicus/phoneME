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

package com.sun.midp.io.j2me.push;

import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.midp.jump.push.executive.remote.MIDPContainerInterface;
import com.sun.midp.jump.push.share.Configuration;
import com.sun.midp.jump.push.share.JUMPReservationDescriptor;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.push.reservation.ReservationDescriptor;

import com.sun.j2me.security.AccessControlContext;
import com.sun.j2me.security.AccessControlContextAdapter;

import java.io.IOException;
import java.rmi.RemoteException;
import javax.microedition.io.ConnectionNotFoundException;

import sun.misc.MIDPConfig;

/**
 * JUMP Implementation of ConnectionRegistry.
 */
final class ConnectionRegistry {
    /**
     * Hides constructor.
     */
    private ConnectionRegistry() { }

    /** Internal helper to access remote interface. */
    private static class RemoteInterfaceHelper {
        /**
         * Reference to remote interface.
         *
         * <p>
         * This instance is created on demand.
         * </p>
         */
        final static MIDPContainerInterface IFC = getRemoteInterface();

        /**
         * Gets a reference to remote interface.
         *
         * @return a reference to remote interface
         * (cannot be <code>null</code>)
         */
        static MIDPContainerInterface getRemoteInterface() {
            final JUMPIsolateProcess jip = JUMPIsolateProcess.getInstance();

            final String URI = Configuration.MIDP_CONTAINER_INTERFACE_IXC_URI;

            final MIDPContainerInterface ifc = (MIDPContainerInterface)
                    jip.getRemoteService(URI);

            if (ifc == null) {
                throw new RuntimeException(
                        "failed to obtain remote push interface");
            }

            return ifc;
        }
    }

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
            final String connection,
            final String midlet,
            final String filter) throws ClassNotFoundException, IOException {

        final AccessControlContext context = new AccessControlContextAdapter() {
            public void checkPermissionImpl(
                    final String permissionName,
                    final String resource,
                    final String extraValue) throws SecurityException {
                // TBD: implement
            }
        };

        final ReservationDescriptor descriptor = Configuration
                .getReservationDescriptorFactory()
                .getDescriptor(connection, filter, context);
        JUMPReservationDescriptor jd = null;
        try {
            jd = (JUMPReservationDescriptor) descriptor;
        } catch (ClassCastException cce) {
            throw new ConnectionNotFoundException(
                    "protocol isn't supported by jump");
        }

        /*
         * IMPL_NOTE: as <code>RemoteException</code> is a subclass
         * of <code>IOException</code>, it's ok to let it out
         */
        getRemoteInterface()
            .registerConnection(midletSuite.getID(), midlet, jd);
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
        try {
            return getRemoteInterface()
                .unregisterConnection(midletSuite.getID(), connection);
        } catch (RemoteException re) {
            // Return reasonable default value
            return false;
        }
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
        try {
            return getRemoteInterface()
                .listConnections(midletSuite.getID(), available);
        } catch (RemoteException re) {
            // Return reasonable default value
            return new String [0];
        }
    }

    /**
     * Gets a <code>MIDlet</code> class name of connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.getMIDlet</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet</code> suite to fetch info for
     * (cannot be <code>null</code>)
     *
     * @param connection Connection to look <code>MIDlet</code> for
     *
     * @return <code>MIDlet</code> name
     */
    public static String getMIDlet(
            final MIDletSuite midletSuite, final String connection) {
        try {
            return getRemoteInterface()
                .getMIDlet(midletSuite.getID(), connection);
        } catch (RemoteException re) {
            // Return reasonable default value
            return null;
        }
    }

    /**
     * Gets a filter of connection.
     *
     * <p>
     * For details see
     * <code>javax.microedition.io.PushRegistry.getFilter</code>
     * </p>
     *
     * @param midletSuite <code>MIDlet</code> suite to fetch info for
     * (cannot be <code>null</code>)
     *
     * @param connection Connection to look a filter for
     *
     * @return Filter
     */
    public static String getFilter(
            final MIDletSuite midletSuite, final String connection) {
        try {
            return getRemoteInterface()
                .getFilter(midletSuite.getID(), connection);
        } catch (RemoteException re) {
            // Return reasonable default value
            return null;
        }
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
        try {
            return getRemoteInterface()
                .registerAlarm(midletSuite.getID(), midlet, time);
        } catch (RemoteException re) {
            // Rethrow it as CNFE---best thing that can be done
            throw new ConnectionNotFoundException("IXC failure: " + re);
        }
    }

    /**
     * Loads application class given its name.
     *
     * @param className name of class to load
     * @return instance of class
     * @throws ClassNotFoundException if the class cannot be located
     */
    static Class loadApplicationClass(final String className)
	throws ClassNotFoundException {
        final ClassLoader appClassLoader = MIDPConfig.getMIDletClassLoader();
        if (appClassLoader == null) {
	    /*
	     * IMPL_NOTE: that might happen if this method is invoked
	     * before the MIDlet app has started and class loader
	     * hasn't been created yet
	     */
	    logWarning("application class loader is null");
        }
        return Class.forName(className, true, appClassLoader);
    }

    /**
     * Fetches remote interafce to talk to the executive.
     *
     * @return remote interface instance (cannot be <code>null</code>)
     */
    private static MIDPContainerInterface getRemoteInterface() {
        return RemoteInterfaceHelper.IFC;
    }

    /**
     * Logs warning message.
     *
     * @param message message to log
     */
    private static void logWarning(final String message) {
        // TBD: proper logging
        System.err.println("[warning, " + ConnectionRegistry.class.getName()
            + "]: " + message);
    }
}
