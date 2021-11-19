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

package com.sun.tck.wma;
import java.io.IOException;

/**
 * A factory class for creating new MessageConnection objects.
 */

public interface Connector {

    /**
     * Creates and opens a MessageConnection.
     *
     * @param name             the URL for the connection.
     * @return                 a new MessageConnection object.
     *
     * @exception IllegalArgumentException if a parameter is invalid.
     * @exception ConnectionNotFoundException if the requested connection
     *   cannot be made, or the protocol type does not exist.
     * @exception IOException  if some other kind of I/O error occurs.
     * @exception SecurityException if a requested protocol handler is not
     *             permitted
     */
    public MessageConnection open(String name) throws IOException;
}


