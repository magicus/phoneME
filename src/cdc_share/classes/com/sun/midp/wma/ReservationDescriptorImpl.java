/*
 *
 *
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

package com.sun.midp.wma;

import com.sun.midp.push.reservation.*;
import com.sun.j2me.security.AccessControlContext;
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
public class ReservationDescriptorImpl implements ReservationDescriptor {

    protected String _filter;
    protected String _protocol;
    protected String _targetAndParams;
    protected int _port;

    public ReservationDescriptorImpl(
            String protocol, String targetAndParams, 
            String filter, AccessControlContext context) {

        _protocol = protocol;
        _targetAndParams = targetAndParams;

        if (!protocol.startsWith("sms")) {
	    throw new IllegalArgumentException("Protocol not supported: " + protocol);
        }

	int colonPlace = targetAndParams.indexOf(":");
	if (colonPlace == -1) {
	    throw new IllegalArgumentException("SMS port not specified: " + protocol);
	} else {
	    String portString = targetAndParams.substring(colonPlace+1);
	    try {
		_port = Integer.parseInt(portString);
	    } catch (NumberFormatException excpt) {
		throw new IllegalArgumentException("SMS port not specified");
	    }
	}
        _filter = filter;
    }

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
    public ConnectionReservation reserve(
            int midletSuiteId,
            String midletClassName,
            DataAvailableListener dataAvailableListener)  throws IOException {

        ConnectionReservation reservation = new ConnectionReservationImpl(_port, _filter,
           midletSuiteId, midletClassName, dataAvailableListener);

	return reservation;
    }

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
    public String getConnectionName() {
        return _protocol + ":" + _targetAndParams; 
    }

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
    public String getFilter() { 
        return _filter; 
    }
}
