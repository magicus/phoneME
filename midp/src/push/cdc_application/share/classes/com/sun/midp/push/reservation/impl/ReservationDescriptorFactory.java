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

package com.sun.midp.push.reservation.impl;

import com.sun.midp.push.reservation.ReservationDescriptor;

import com.sun.j2me.security.AccessControlContext;

import javax.microedition.io.ConnectionNotFoundException;

/**
 * Factory class to produce <code>ReservationDescriptor</code>s.
 */
public interface ReservationDescriptorFactory {
    /**
     * Returns push reservation descriptor.
     *
     * <p>
     * Validates parameters passed, checks connection permissions and if
     * everything's fine, creates an instance of {@link ReservationDescriptor}
     * which would allow to reserve the connection.
     * </p>
     *
     * @param connectionName same as <code>connection</code> parameter
     * passed into
     * {@link javax.microedition.io.PushRegistry#registerConnection}
     * (cannot be <code>null</code>)
     *
     * @param filter filter as passed into
     * {@link javax.microedition.io.PushRegistry#registerConnection}
     * (cannot be <code>null</code>)
     *
     * @param context object to check permissions (cannot be
     * <code>null</code>, use dummy implementation if there is need to bypass
     * security checks)
     *
     * @return connection descriptor (always not <code>null</code>)
     *
     * @throws IllegalArgumentException if the connection string is not valid,
     *  or if the filter string is not valid
     *
     * @throws ConnectionNotFoundException if the runtime system does not
     *  support push delivery for the requested connection protocol
     */
    ReservationDescriptor getDescriptor(
            String connectionName,
            String filter,
            AccessControlContext context) throws
                IllegalArgumentException,
                ConnectionNotFoundException;
}
