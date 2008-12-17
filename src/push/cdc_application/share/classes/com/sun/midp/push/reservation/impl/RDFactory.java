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

import javax.microedition.io.ConnectionNotFoundException;

import com.sun.j2me.security.AccessControlContext;

import com.sun.midp.push.reservation.ProtocolFactory;
import com.sun.midp.push.reservation.ReservationDescriptor;

/**
 * Descriptor factories based impl of <code>ReservationDescriptorFactory</code>.
 */
public final class RDFactory implements ReservationDescriptorFactory {
    /** Protocol registry to use. */
    private final ProtocolRegistry protocolRegistry;

    /**
     * Ctor.
     *
     * @param protocolRegistry Protocol registry to use.
     */
    public RDFactory(final ProtocolRegistry protocolRegistry) {
        this.protocolRegistry = protocolRegistry;
    }

    /** {@inheritDoc} */
    public ReservationDescriptor getDescriptor(
            final String connectionName,
            final String filter, final AccessControlContext context)
                throws IllegalArgumentException, ConnectionNotFoundException {
        if (connectionName == null || connectionName.length() == 0) {
            throw new IllegalArgumentException(
                "Invalid connectionName=" + connectionName);
        }

        final ConnectionName cn = ConnectionName.parse(connectionName);
        final ProtocolFactory pf = protocolRegistry.get(cn.protocol);
        if (pf == null) {
            throw new ConnectionNotFoundException("Protocol `" + cn.protocol
                    + "` isn't supported (connectionName = " + connectionName
                    + ")");
        }

        return pf.createDescriptor(cn.protocol, cn.targetAndParams, filter,
                context);
    }
}
