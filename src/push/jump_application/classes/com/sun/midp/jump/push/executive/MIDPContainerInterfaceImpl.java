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

import com.sun.midp.jump.push.executive.AlarmController;
import com.sun.midp.jump.push.executive.JUMPConnectionInfo;
import com.sun.midp.jump.push.executive.LifecycleAdapter;
import com.sun.midp.jump.push.executive.persistence.Store;
import com.sun.midp.jump.push.executive.remote.MIDPContainerInterface;
import java.io.IOException;
import java.rmi.RemoteException;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Implementation of
 *  {@link com.sun.midp.jump.push.executive.remote.MIDPContainerInterface}
 */
public final class MIDPContainerInterfaceImpl implements MIDPContainerInterface {
    /** Reference to a store. */
    private final Store store;

    /** Alarm controller to use. */
    private final AlarmController alarmController;

    /**
     * Creates an implementation.
     *
     * @param store Store to use
     */
    public MIDPContainerInterfaceImpl(final Store store) {
        this.store = store;
        this.alarmController = new AlarmController(store,
                new LifecycleAdapter() {
            public void launchMidlet(final int midletSuiteID, final String midlet) {
                // TBD: implement soft launch
            }
        });
    }

    /** {@inheritDoc} */
    public boolean unregisterConnection(
            final int midletSuiteId,
            final JUMPConnectionInfo connection) throws RemoteException {
        // TBD: Implement
        return false;
    }

    /** {@inheritDoc} */
    public boolean registerConnection(
            final int midletSuiteId,
            final JUMPConnectionInfo connection) throws RemoteException {
        // TBD: Implement
        return false;
    }

    /** {@inheritDoc} */
    public long registerAlarm(
            final int midletSuiteId,
            final String midlet,
            final long time)
                throws RemoteException, ConnectionNotFoundException  {
        return alarmController.registerAlarm(midletSuiteId, midlet, time);
    }
}
