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

package com.sun.midp.push.reservation;

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
public interface ConnectionReservation {
    /**
     * Checks if the reservation has available data. 
     * Implementation should be nonblocking. This method is used to get 
     * snapshot of the <code>ConnectionReservation</code> if it has available
     * data to read or not.
     *
     * @return <code>true</code> if there is available data
     *
     * @throws IllegalStateException if invoked after reservation cancellation
     */
    boolean hasAvailableData() throws IllegalStateException;

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
    void cancel() throws IllegalStateException;
}
