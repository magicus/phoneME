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

package com.sun.jumpimpl.module.pushregistry.MIDP;

import com.sun.jump.module.pushregistry.JUMPConnectionInfo;
import com.sun.jump.module.pushregistry.MIDP.JUMPPushRegistry;
import com.sun.jumpimpl.module.pushregistry.AlarmRegistry;
import com.sun.jumpimpl.module.pushregistry.persistence.Store;
import java.io.IOException;
import java.rmi.RemoteException;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Implementation of
 *  <code>com.sun.jump.module.pushregistry.MIDP.JUMPPushRegistry</code>.
 */
public final class MIDPPushRegistry implements JUMPPushRegistry {
    /** Reference to a store. */
    private final Store store;

    /** Alarm registry to use. */
    private final AlarmRegistry alarmRegistry;

    /**
     * Creats a registry.
     *
     * @param store Store to use
     */
    public MIDPPushRegistry(final Store store) {
        this.store = store;
        this.alarmRegistry = new AlarmRegistry(store, new AlarmRegistry.LifecycleAdapter() {
            public void launchMidlet(final int midletSuiteID, final String midlet) {
                // TBD: implement soft launch
            }
        });
    }

    /**
     * Unregisters previously registered connection.
     *
     * @param midletSuiteId <code>MIDlet suite</code> to touch
     * @param connection Connection to unregister
     *
     * @return <code>true</code> if succeed, <code>false</code> otherwise
     *
     * @throws RemoteException as requested by RMI spec.
     */
    public boolean unregisterConnection(
            final int midletSuiteId,
            final JUMPConnectionInfo connection) throws RemoteException {
        try {
            store.removeConnection(midletSuiteId, connection);
        } catch (IOException _) {
            return false;
        }
        return true;
    }

    /**
     * Registers previously registered connection.
     *
     * @param midletSuiteId <code>MIDlet suite</code> to touch
     * @param connection Connection to register
     *
     * @return <code>true</code> if succeed, <code>false</code> otherwise
     *
     * @throws RemoteException as requested by RMI spec.
     */
    public boolean registerConnection(
            final int midletSuiteId,
            final JUMPConnectionInfo connection) throws RemoteException {
        try {
            store.addConnection(midletSuiteId, connection);
        } catch (IOException _) {
            return false;
        }
        return true;
    }

    /**
     * Implements the corresponding interface method.
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to unregister
     *  connection for
     *
     * @param midlet <code>MIDlet</code> class name
     *
     * @param time alarm time
     *
     * @return time of previous registered (but not fired) alarm or 0
     *
     * @throws RemoteException as requested by RMI spec.
     * @throws ConnectionNotFoundException if it's impossible to register alarm
     */
    public long registerAlarm(
            final int midletSuiteId,
            final String midlet,
            final long time)
                throws RemoteException, ConnectionNotFoundException  {
        return alarmRegistry.registerAlarm(midletSuiteId, midlet, time);
    }
}
