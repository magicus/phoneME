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

package com.sun.midp.io.j2me.datagram;

import com.sun.j2me.security.AccessController;
import javax.microedition.io.Datagram;
import java.io.IOException;

public class Protocol extends com.sun.cdc.io.j2me.datagram.Protocol {

    private static final String SERVER_PERMISSION_NAME =
        "javax.microedition.io.Connector.datagramreceiver";

    private static final String CLIENT_PERMISSION_NAME =
        "javax.microedition.io.Connector.datagram";

    /**
     * Check to see if the application has permission to use
     * the given resource.
     *
     * @param host the name of the host to contact. Can be
     *        <code>null</code>, which indicates a server
     *        endpoint on the given port.
     * @param port the port number to use. Must be greater
     *        than 0 if the host is specified.
     *
     * @exception SecurityException if the MIDP permission
     *            check fails
     */
    protected void checkPermission(String host, int port) 
        throws SecurityException {

        // If hostname is specified, we must have a valid port.
        if (host != null && port < 0) {
            throw new IllegalArgumentException("Missing port number");
        }

        // Check the MIDP permission
        if (host == null) {
            // A server endpoint. Use the port if valid; otherwise
            // it is assigned by the server.
            String endPoint = ":";
            if (port > 0) {
                endPoint += port;
            }
            AccessController.checkPermission(SERVER_PERMISSION_NAME, endPoint);
        } else {
            AccessController.checkPermission(CLIENT_PERMISSION_NAME,
                                             host + ":" + port);
        }

        return;
    }

    /**
     * A convenience method for creating a connection
     * where the port is allocated by the system.
     *
     * @param host the name of the host to contact. Can be
     *        <code>null</code>, which indicates a server
     *        endpoint on the given port.
     *
     * @exception SecurityException if the MIDP permission
     *            check fails
     */
    protected void checkPermission(String host)
        throws SecurityException {
        checkPermission(host, 0);
	return;
    }

    /*
     * For MIDP version of the protocol handler, only a single
     * check on open is required.
     */
    protected void outputStreamPermissionCheck() {
        return;
    }

    /*
     * For MIDP version of the protocol handler, only a single
     * check on open is required.
     */
    protected void inputStreamPermissionCheck() {
        return;
    }
    
    public void send(Datagram dgram) throws IOException, SecurityException {
        if (!open) {
            throw new IOException("Connection closed");
        }

        try {
            AccessController.checkPermission(
                AccessController.TRUSTED_APP_PERMISSION_NAME);
        } catch (SecurityException exc) {
            /*
             * JTWI security check, untrusted MIDlets cannot open
             * WAP gateway ports 9200-9203.
             */
            String addr = dgram.getAddress();
            if (addr != null) {
                int remotePort = getPort(addr);
                if (remotePort >= 9200 && remotePort <= 9203) {
                    throw new SecurityException(
                        "Target port denied to untrusted applications");
                }
            }
        }

        super.send(dgram);
    }
}
