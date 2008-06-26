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
package com.sun.midp.push.controller;

import java.io.Serializable;

/**
 * PushRegistry connection info.
 *
 * <p>Simple, structure like class</p>
 */
public final class ConnectionInfo implements Serializable {
    /**
     * Generic connection <i>protocol</i>, <i>host</i> and <i>port
     * number</i> (optional parameters may be included separated with
     * semi-colons (;)).
     */
    public final String connection;

    /**
     * Class name of the <tt>MIDlet</tt> to be launched, when external data is
     * available.
     */
    public final String midlet;

    /**
     * A connection URL string indicating which senders are allowed to cause
     * <tt>MIDlet</tt> to be launched.
     */
    public final String filter;

    /**
     * Ctor to create filled structure.
     *
     * @param connection generic connection <i>protocol</i>, <i>host</i> and
     * <i>port number</i> (optional parameters may be included separated with
     * semi-colons (;))
     *
     * @param midlet class name of the <tt>MIDlet</tt> to be launched,
     * when external data is available
     *
     * @param filter a connection URL string indicating which senders are
     * allowed to cause <tt>MIDlet</tt> to be launched
     *
     * @throws IllegalArgumentException if any of <code>connection</code>,
     *  <code>midlet</code> or <code>filter</code> is <code>null</code>
     *
     * <p>
     * TBD: unittest <code>null</code> params
     * </p>
     */
    public ConnectionInfo(
            final String connection,
            final String midlet,
            final String filter) {
        if (connection == null) {
            throw new IllegalArgumentException("connection is null");
        }
        this.connection = connection;

        if (midlet == null) {
            throw new IllegalArgumentException("midlet is null");
        }
        this.midlet = midlet;

        if (filter == null) {
            throw new IllegalArgumentException("filter is null");
        }
        this.filter = filter;
    }

    /**
     * Implements <code>Object.equals()</code>.
     *
     * @param obj object to compare with
     *
     * @return <code>true</code> if equal, <code>fasle</code> otherwise
     */
    public boolean equals(final Object obj) {
        if (!(obj instanceof ConnectionInfo)) {
            return false;
        }

        final ConnectionInfo rhs = (ConnectionInfo) obj;
        return (this == rhs)
            || (connection.equals(rhs.connection)
                && midlet.equals(rhs.midlet)
                && filter.equals(rhs.filter));
    }

    /**
     * Implements <code>Object.hashCode()</code>.
     *
     * @return hash code
     */
    public int hashCode() {
        return connection.hashCode() + midlet.hashCode() + filter.hashCode();
    }
}
