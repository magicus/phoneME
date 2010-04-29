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

package com.sun.midp.io.j2me.http;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.io.IOException;

import com.sun.j2me.security.AccessController;

public class Protocol extends com.sun.cdc.io.j2me.http.Protocol {

    private static final String UNTRUSTED = "UNTRUSTED/1.0";
    private static final String HTTP_PERMISSION_NAME =
	"javax.microedition.io.Connector.http";
    
    /**
     * This class overrides the openXXputStream() methods to restrict
     * the number of opened input or output streams to 1 since the 
     * MIDP GCF Spec allows only 1 opened input/output stream.
     */
    
    /**
     * Maximum number of open input streams. Set this
     * to zero to prevent openInputStream from giving out a stream in
     * write-only mode.
     */
    protected int maxIStreams = 1;
    /**
     * Maximum number of output streams. Set this
     * to zero to prevent openOutputStream from giving out a stream in
     * read-only mode.
     */
    protected int maxOStreams = 1;

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
        return i;
    }
    
    public DataInputStream openDataInputStream() throws IOException {
        if (maxIStreams == 0) {
            throw new IOException("no more input streams available");
        }
        DataInputStream i = super.openDataInputStream();
        return i;
    }

    /*
     * Open the output stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public OutputStream openOutputStream() throws IOException {
        if (maxOStreams == 0) {
            throw new IOException("no more output streams available");
        }
        OutputStream o = super.openOutputStream();
        return o;
    }
    
    public DataOutputStream openDataOutputStream() throws IOException {
        if (maxOStreams == 0) {
            throw new IOException("no more output streams available");
        }
        DataOutputStream o = super.openDataOutputStream();
        maxOStreams--;
        return o;
    }

    /**
     * Check to see if the application has permission to use
     * the given resource.
     *
     * @param url the URL to which to connect
     *
     * @exception SecurityException if the MIDP permission
     *            check fails.
     */
    protected void checkPermission(String host, int port, String file) 
        throws SecurityException {
        AccessController.checkPermission(HTTP_PERMISSION_NAME,
                                         host + ":" + port);
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
    
    protected void connect() throws IOException {
        if (connected) {
            return;
        }

        try {
            AccessController.checkPermission(
                AccessController.TRUSTED_APP_PERMISSION_NAME);
        } catch (SecurityException exc) {
            String newUserAgentValue;
            String origUserAgentValue = getRequestProperty("User-Agent");
            if (origUserAgentValue != null) {
                /*
                 * HTTP header values can be concatenated, so original value
                 * of the "User-Agent" header field should not be ignored in 
                 * this case
                 */
                newUserAgentValue = origUserAgentValue;
                if (-1 == origUserAgentValue.indexOf(UNTRUSTED)) {
                    newUserAgentValue += " " + UNTRUSTED;
                }
            } else {
                newUserAgentValue = UNTRUSTED;
            }
            reqProperties.put("User-Agent", newUserAgentValue);
        }

        super.connect();
    }

}
