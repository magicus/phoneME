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

/** Connection string parser. */
final class ConnectionName {
    /**
     * Connection protocol.
     *
     * <p>Always in lowercase.</p> 
     */
    public final String protocol;

    /** Connection reminder. */
    public final String targetAndParams;

    /**
     * @param protocol connection protocol
     * @param targetAndParams connection reminder
     */
    private ConnectionName(final String protocol,
            final String targetAndParams) {
        this.protocol = protocol;
        this.targetAndParams = targetAndParams;
    }

    /** Protocol separator char. */
    private static final int PROTOCOL_SEP = ':';

    /**
     * Parses connectionName string.
     *
     * @param connectionName connection string to parse
     * (cannot be <code>null</code>)
     *
     * @return parsing results (cannot be <code>null</code>)
     */
    public static ConnectionName parse(final String connectionName) 
            throws javax.microedition.io.ConnectionNotFoundException {
        final int protocolPos = connectionName.indexOf(PROTOCOL_SEP);
        if (protocolPos == -1) {
            throw new
	    javax.microedition.io.ConnectionNotFoundException(connectionName);
        }

        final String protocol = connectionName
            .substring(0, protocolPos)
            .toLowerCase(); // IMPL_NOTE: according to RFC 2396 should translate
            // to lower case
        com.sun.cdc.io.InternalConnectorImpl.checkProtocol(protocol);

        final String targetAndParams =
            connectionName.substring(protocolPos + 1);

        return new ConnectionName(protocol, targetAndParams);
    }
}
