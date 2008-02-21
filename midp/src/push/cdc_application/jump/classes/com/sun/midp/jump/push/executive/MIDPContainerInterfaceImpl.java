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

package com.sun.midp.jump.push.executive;

import com.sun.midp.push.controller.PushController;

import com.sun.midp.jump.push.executive.remote.MIDPContainerInterface;
import com.sun.midp.jump.push.share.JUMPReservationDescriptor;
import java.io.IOException;
import java.rmi.RemoteException;
import javax.microedition.io.ConnectionNotFoundException;

/** Small wrapper class. */
final class MIDPContainerInterfaceImpl implements MIDPContainerInterface {
    /** Push controller. */
    private final PushController pushController;

    /**
     * Creates a wrapper.
     *
     * @param pushController push controller to use
     */
    public MIDPContainerInterfaceImpl(final PushController pushController) {
        this.pushController = pushController;
    }

    /** {@inheritDoc} */
    public void registerConnection(final int midletSuiteId, final String midlet,
            final JUMPReservationDescriptor reservationDescriptor)
                throws IOException, RemoteException {
        pushController.registerConnection(
                midletSuiteId, midlet, reservationDescriptor);
    }

    /** {@inheritDoc} */
    public boolean unregisterConnection(final int midletSuiteId,
            final String connectionName)
                throws SecurityException, RemoteException {
        return pushController.unregisterConnection(
                midletSuiteId, connectionName);
    }

    /** {@inheritDoc} */
    public String [] listConnections(final int midletSuiteId,
            final boolean available)
                throws RemoteException {
        return pushController.listConnections(midletSuiteId, available);
    }

    /** {@inheritDoc} */
    public String getMIDlet(
            final int midletSuiteID, final String connectionName)
                throws RemoteException {
        return pushController.getMIDlet(midletSuiteID, connectionName);
    }

    /** {@inheritDoc} */
    public String getFilter(
            final int midletSuiteID, final String connectionName)
                throws RemoteException {
        return pushController.getFilter(midletSuiteID, connectionName);
    }

    /** {@inheritDoc} */
    public long registerAlarm(
            final int midletSuiteId,
            final String midlet,
            final long time)
                throws RemoteException, ConnectionNotFoundException  {
        return pushController.registerAlarm(midletSuiteId, midlet, time);
    }
}
