/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
        checkProtocol(protocol);

        final String targetAndParams =
            connectionName.substring(protocolPos + 1);

        return new ConnectionName(protocol, targetAndParams);
    }

    /** Characters allowed as first char of scheme. */
    private static final String ALLOWED_FIRST_CHAR =
        "abcdefghijklmnopqrstuvwxyz";

    /** Characters allowed after the first char of scheme. */
    private static final String ALLOWED_CHAR =
        ALLOWED_FIRST_CHAR + "0123456789+-.";

    /**
     * Checks validity of protocol (a.k.a scheme) string.
     * 
     * <p>
     * Throws <code>IllegalArgumentException</code> if string is invalid.
     * </p>
     *
     * <p>
     * Note: <code>URL</code> class might be used, but it might be better
     * to be maximally independent of external stuff.
     * </p>
     * 
     * @param protocol string to check (must be lower case)
     */
    public static void checkProtocol(final String protocol) {
        if (protocol.length() == 0) {
            throw new IllegalArgumentException("protocols is empty string");
        }
        if (!isOneOf(protocol.charAt(0), ALLOWED_FIRST_CHAR)) {
            throw new IllegalArgumentException("wrong leading character: "
                    + protocol);
        }
        for (int i = 1; i < protocol.length(); i++) {
            final char c = protocol.charAt(i);
            if (!isOneOf(c, ALLOWED_CHAR)) {
                throw new IllegalArgumentException("wrong character at "
                        + i + ": " + protocol);
            }
        }
    }

    /**
     * Checks if the character is one of the given set.
     *
     * @param c character to check
     * @param set set to check against
     *
     * @return <code>true</code> iff <code>c</code> belongs to <code>set</code>
     */
    private static boolean isOneOf(final char c, final String set) {
        return set.indexOf(c) != -1;
    }
}
