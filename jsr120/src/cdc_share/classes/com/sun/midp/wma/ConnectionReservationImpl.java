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


/**
 * Connection reservation.
 *
 * <p>
 * Connection reservation is a push enabled protocol connection which is
 * reserved for some application and is managed by the protocol.  For details
 * of protocol obligations in respect to reserved connections, see doc
 * for {@link ReservationDescriptor#reserve}.
 * </p>
 */
public class ConnectionReservationImpl implements ConnectionReservation {

    int _port;
    String _filter;
    DataAvailableListener _dataAvailableListener;
    boolean _canceled = false;

    public ConnectionReservationImpl(
            int port,
	    String filter,
            int midletSuiteId,
            String midletClassName,
            DataAvailableListener dataAvailableListener) throws java.io.IOException {

	_port = port;
	_filter = filter;
        _dataAvailableListener = dataAvailableListener;
	PushConnectionsPool.addReservation(this);
    }

    public void notifyDataAvailable() {
        _dataAvailableListener.dataAvailable();
    }

    public int getPort() {
	return _port;
    }

    public String getFilter() {
	return _filter;
    }

    /**
     * Checks if the reservation has available data.
     *
     * @return <code>true</code> iff there are available data
     *
     * @throws IllegalStateException if invoked after reservation cancellation
     */
    public boolean hasAvailableData() { //throws IllegalStateException;

        if (_canceled) {
            throw new IllegalStateException("reservation canceled");
        }
        return (0 != PushConnectionsPool.hasAvailableData(_port));
    }

    /**
     * Cancels reservation.
     *
     * <p>
     * Cancelling should be guaranteed at least in
     * the following sense: cancelled reservation's callback won't be invoked.
     * </p>
     *
     * <p>
     * However as callbacks can be invoked from another thread,
     * the following scenario is possible and should be accounted for:
     * <ol>
     *  <li>callback is scheduled for execution, but hasn't started yet</li>
     *  <li><code>cancel</code> method is invoked</li>
     *  <li>callback starts execution</li>
     * </ol>
     * </p>
     *
     * @throws IllegalStateException if invoked after reservation cancellation
     */
    public void cancel() {
	PushConnectionsPool.removeReservation(this);
        _canceled = true;
    } 
}
