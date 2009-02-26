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


package com.sun.midp.io.j2me.socket;

import java.io.*;
import javax.microedition.io.*;

import com.sun.j2me.security.AccessController;

public class Protocol extends com.sun.cdc.io.j2me.socket.Protocol {

    public static final String CLIENT_PERMISSION_NAME =
	"javax.microedition.io.Connector.socket";

    private static final String SERVER_PERMISSION_NAME =
        "javax.microedition.io.Connector.serversocket";
    
    /** Number of input streams that were opened. */
    protected int iStreams = 0;
    /**
     * Maximum number of open input streams. Set this
     * to zero to prevent openInputStream from giving out a stream in
     * write-only mode.
     */
    protected int maxIStreams = 1;

    /*
     * Open the input stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public InputStream openInputStream() throws IOException {
        if (maxIStreams == 0) {
            throw new IOException("no more input streams available");
        }
        InputStream i = super.openInputStream();
        maxIStreams--;
        iStreams++;
        return i;
    }
    
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    /*
     * This class overrides the setSocketOption() to allow 0 as a valid
     * value for sendBufferSize and receiveBufferSize.
     * The underlying CDC networking layer considers 0 as illegal 
     * value and throws IAE which causes the TCK test to fail.
     */
    public void setSocketOption(byte option,  int value)
	throws IllegalArgumentException, IOException {
        if (option == SocketConnection.SNDBUF 
	    || option == SocketConnection.RCVBUF) {
            if (value == 0) {
                value = 1;
                super.setSocketOption(option, value);
            }
        }            
        super.setSocketOption(option, value);
    }

    /**
     * Check to see if the application has permission to use
     * the given resource.
     *
     * @param host the name of the host to contact. If the
     *             empty string, this is a request for a
     *             serversocket.
     *
     * @param port the port number to use.
     *
     * @exception SecurityException if the MIDP permission
     *            check fails.
     */
    protected void checkPermission(String host, int port)
        throws SecurityException {

        if (port < 0) {
            throw new IllegalArgumentException("bad port: " + port);
        }
        try {
            AccessController.checkPermission(AccessController.TRUSTED_APP_PERMISSION_NAME);
        } catch (SecurityException exc) {
            /*
             * JTWI security check, untrusted MIDlets cannot open port 80 or
             * 8080 or 443. This is so they cannot perform HTTP and HTTPS
             * requests on server without using the system code. The
             * system HTTP code will add a "UNTRUSTED/1.0" to the user agent
             * field for untrusted MIDlets.
             */
            if (port == 80 || port == 8080 || port == 443) {
                throw new SecurityException(
                    "Target port denied to untrusted applications");
            }
        }
	if ("".equals(host)) {
	    AccessController.checkPermission(SERVER_PERMISSION_NAME,
					     "localhost:" + port);
	} else {
	    AccessController.checkPermission(CLIENT_PERMISSION_NAME,
                                         host + ":" + port);
	}
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

}
