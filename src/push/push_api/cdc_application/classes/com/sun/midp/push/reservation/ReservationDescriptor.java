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

package com.sun.midp.push.reservation;

import java.io.IOException;

/**
 * Protocol-dependent representation of connection which could become
 * a reservation later.
 *
 * <p>
 * The instance of this class are expected to be just data holders (see
 * probable additional requirements below).
 * </p>
 *
 * @see ConnectionReservation
 * @see ReservationDescriptorFactory
 */
public interface ReservationDescriptor {
    /**
     * Reserves the connection.
     *
     * <p>
     * The very moment this method returns, the correspondong connection cannot
     * be opened by any other application (including native ones) until
     * reservation is cancelled.  The connection cannot be reserved by
     * <em>any</em> application (including one for which it has been reserved).
     * <code>IOException</code> should be thrown to report such a situation.
     * </p>
     *
     * <p>
     * Pair <code>midletSuiteId</code> and <code>midletClassName</code>
     * should refer to valid <code>MIDlet</code>
     * </p>
     *
     * @param midletSuiteId <code>MIDlet</code> suite ID
     *
     * @param midletClassName name of <code>MIDlet</code> class
     *
     * @param dataAvailableListener data availability listener
     *
     * @return connection reservation
     *
     * @throws IOException if connection cannot be reserved
     *  for the given application
     */
    ConnectionReservation reserve(
            int midletSuiteId,
            String midletClassName,
            DataAvailableListener dataAvailableListener)
        throws IOException;

    /**
     * Gets connection name of descriptor.
     *
     * <p>
     * Should be identical to one passed into
     * {@link ReservationDescriptorFactory#getDescriptor}.
     * </p>
     *
     * @return connection name
     */
    String getConnectionName();

    /**
     * Gets filter of descriptor.
     *
     * <p>
     * Should be identical to one passed into
     * {@link ReservationDescriptorFactory#getDescriptor}.
     * </p>
     *
     * @return connection filter
     */
    String getFilter();

    /**
     * Tests if the given connection name is equivalent to this reservation
     * connection name.
     * <p>
     * Normally reservation can be addressed with only one connection name.
     * Though some protocols (e.g.: Bluetooth) require that the same
     * reservation can be addressed with more then one connection name.
     * This methods addresses requirements of such protocols.
     * </p>
     *
     * <p>
     * Typical implementation should be
     * <code>return getConnectionName().equals(connectionName);</code>
     * </p>
     *
     * @param connectionName connection name to test
     *
     * @return <code>true</code> if the given connection name is equivalent
     *      to this reservation connection name
     */
    boolean isConnectionNameEquivalent(String connectionName);
}
