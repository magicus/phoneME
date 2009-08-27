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

package com.sun.midp.io.j2me.https;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.io.IOException;

import java.net.MalformedURLException;
import java.net.URL;

import com.sun.j2me.security.AccessController;

public class Protocol extends com.sun.cdc.io.j2me.https.Protocol {

    private static final String HTTPS_PERMISSION_NAME =
	"javax.microedition.io.Connector.https";
    
    /**
     * This class overrides the openXXputStream() methods to restrict
     * the number of opened input or output streams to 1 since the
     * MIDP GCF Spec allows only 1 opened input/output stream.
     */
    
    /** Number of input streams that were opened. */
    protected int iStreams = 0;
    /**
     * Maximum number of open input streams. Set this
     * to zero to prevent openInputStream from giving out a stream in
     * write-only mode.
     */
    protected int maxIStreams = 1;
    /** Number of output streams were opened. */
    protected int oStreams = 0;
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
        iStreams++;
        return i;
    }
    
    
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
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
        OutputStream o = super.openDataOutputStream();
        maxOStreams--;
        oStreams++;
        return o;
    }
    
    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    /**
     * Check to see if the application has permission to use
     * the given resource.
     *
     * @param host the hostname
     * @param port the port to use
     * @param file the filename portion of the URL
     *
     * @exception SecurityException if the MIDP permission
     *            check fails.b
     */
    protected void checkPermission(String host, int port, String file) {
        AccessController.checkPermission(HTTPS_PERMISSION_NAME,
                                         host + ":" +
                                         port + file);
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
 
