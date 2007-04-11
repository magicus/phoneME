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

import com.sun.midp.jump.push.executive.ota.InstallerInterface;
import com.sun.midp.push.gcf.PermissionCallback;
import com.sun.midp.push.gcf.ReservationDescriptor;
import com.sun.midp.push.gcf.ReservationDescriptorFactory;
import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

/** Installer interface implementation. */
final class InstallerInterfaceImpl implements InstallerInterface {
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
            final JUMPConnectionInfo [] connections)
                throws  ConnectionNotFoundException, IOException,
                        SecurityException {
        final PermissionCallback permissionCallback = new PermissionCallback() {
            public void checkForPermission(
                    final String permissionName,
                    final String resource,
                    final String extraValue) {
                checkPermission(
                        midletSuiteId, permissionName, resource, extraValue);
            }
        };

        checkPushPermission(midletSuiteId);
        for (int i = 0; i < connections.length; i++) {
            final JUMPConnectionInfo ci = connections[i];
            try {
                pushController.registerConnection(
                        midletSuiteId, ci.midlet,
                        reservationDescriptorFactory.getDescriptor(
                            ci.connection, ci.filter,
                            permissionCallback));
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

    /** {@inheritDoc} */
    public boolean enableConnections(final int midletSuiteId) {
        // TBD: rethink if we need it
        return true;
    }

    /**
     * Checks if the suite is allowed to install static push reservations.
     *
     * @param midletSuiteId <code>MIDlet</code> suite id
     */
    private void checkPushPermission(final int midletSuiteId) {
        // TBD: implement
    }

    /**
     * Checks if the suite is allowed to install static push reservations.
     *
     * @param midletSuiteId <code>MIDlet</code> suite id
     */
    private void checkPermission(
            final int midletSuiteId,
            final String permissionName,
            final String resource, final String extraValue) {
        // TBD: implement
    }
}
