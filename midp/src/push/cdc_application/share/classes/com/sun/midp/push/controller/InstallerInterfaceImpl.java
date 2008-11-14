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

package com.sun.midp.push.controller;

import com.sun.midp.push.ota.InstallerInterface;
import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;

import com.sun.j2me.security.AccessControlContext;
import com.sun.j2me.security.AccessControlContextAdapter;

import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

/** Installer interface implementation. */
public final class InstallerInterfaceImpl implements InstallerInterface {
    /** Push controller. */
    private final PushController pushController;

    /** Reservation descriptor factory. */
    ReservationDescriptorFactory reservationDescriptorFactory;

    /**
     * Creates an implementation.
     *
     * @param pushController push controller
     * @param reservationDescriptorFactory reservation descriptor factory
     */
    public InstallerInterfaceImpl(
            final PushController pushController,
            final ReservationDescriptorFactory reservationDescriptorFactory) {
        this.pushController = pushController;
        this.reservationDescriptorFactory = reservationDescriptorFactory;
    }

    /** {@inheritDoc} */
    public void installConnections(
            final int midletSuiteId,
            final ConnectionInfo [] connections)
                throws  ConnectionNotFoundException, IOException,
                        SecurityException {
        final AccessControlContext context = new AccessControlContextAdapter() {
            public void checkPermissionImpl(
                    final String permissionName,
                    final String resource,
                    final String extraValue) throws SecurityException {
                InstallerInterfaceImpl.this.checkPermission(
                        midletSuiteId, permissionName, resource, extraValue);
            }
        };

        checkPushPermission(midletSuiteId);
        for (int i = 0; i < connections.length; i++) {
            final ConnectionInfo ci = connections[i];
            try {
                pushController.registerConnection(
                        midletSuiteId, ci.midlet,
                        reservationDescriptorFactory.getDescriptor(
                            ci.connection, ci.filter,
                            context));
            } catch (IOException ioex) {
                // NB: ConnectionNotFoundException is subclass of IOException
                // Quick'n'simple
                pushController.removeSuiteInfo(midletSuiteId);
                throw ioex;
            } catch (SecurityException sex) {
                // Quick'n'simple
                pushController.removeSuiteInfo(midletSuiteId);
                throw sex;
            }
        }
    }

    /** {@inheritDoc} */
    public void uninstallConnections(final int midletSuiteId) {
        pushController.removeSuiteInfo(midletSuiteId);
    }

    /**
     * Checks if the suite is allowed to install static push reservations.
     *
     * @param midletSuiteId <code>MIDlet</code> suite id
     */
    private void checkPushPermission(final int midletSuiteId) 
                    throws SecurityException {
        // TBD: implement
    }

    /**
     * Checks if the suite is allowed to install static push reservations.
     *
     * @param midletSuiteId <code>MIDlet</code> suite id
     * @param permissionName string representation of permission
     * @param resource name of resource
     * @param extraValue additional value
     */
    private void checkPermission(
            final int midletSuiteId,
            final String permissionName,
            final String resource, final String extraValue) 
                throws SecurityException {
        // TBD: implement
    }
}
