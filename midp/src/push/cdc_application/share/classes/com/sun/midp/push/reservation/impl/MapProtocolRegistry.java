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

import com.sun.midp.push.reservation.ProtocolFactory;

import java.util.HashMap;
import java.util.Map;

/**
 * Simple in memory implementation of <code>ProtocolRegistry</code>.
 * 
 * <p>Might be used in OSGi being exposed as a servie.</p>
 */
public final class MapProtocolRegistry implements ProtocolRegistry {
    /** Mapping from protocols to factories. */
    private final Map map = new HashMap();

    /**
     * Binds a protocol to a factory.
     * 
     * <p><code>protocol</code> argument gets checked before putting and
     * <code>IllegalArgumentException</code> might be thrown.
     * 
     * @param protocol protocol to bind to
     * @param factory factory to use
     *
     * @return previous factory or <code>null</code> if none
     */
    public ProtocolFactory bind(final String protocol,
            final ProtocolFactory factory) {
        final String p = protocol.toLowerCase();
        com.sun.cdc.io.InternalConnectorImpl.checkProtocol(p);
        return (ProtocolFactory) map.put(p, factory);
    }

    /** {@inheritDoc} */
    public ProtocolFactory get(final String protocol) {
        return (ProtocolFactory) map.get(protocol);
    }
}
