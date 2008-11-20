/*
 * HttpStreamconnection.java
 *
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
 *
 */

/**
 * HttpStreamConnection implements the StreamConnection
 * interface for a HTTP connection. This class is intended
 * to be used only by the HTTP protocol handler and will
 * open a socket connection to a given host and port.
 */

package com.sun.cdc.io.j2me.http;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.net.Socket;

import javax.microedition.io.StreamConnection;

import com.sun.cdc.io.j2me.UniversalFilterInputStream;
import com.sun.cdc.io.j2me.UniversalFilterOutputStream;

class HttpStreamConnection implements StreamConnection 
{

    // If hostname is empty, use localhost
    private final String LOCALHOST = "localhost";

    // Current host name
    private String host = null;

    // port number
    private int port = 0;

    // Flag indicating the underlying TCP connection is open.
    private boolean copen = false;

    // Socket handle
    private Socket socket;
    
    private InputStream is;
    private DataInputStream dis;
    private OutputStream os;
    private DataOutputStream dos;

    public HttpStreamConnection(String host, int port) throws IOException {

	if (port < 0) {
	    throw new IllegalArgumentException("bad port number: " + port);
	}
        this.host = host;
        this.port = port;

        final String hostName;
	if (host == null || host.length() == 0) {
	    hostName = LOCALHOST;
	} else {
	    hostName = host;
	}
        final int portNum = port;
        try {
            socket = (Socket)java.security.AccessController.doPrivileged(
                     new java.security.PrivilegedExceptionAction() {
                         public Object run() throws
                             java.security.PrivilegedActionException,
                             IOException {
                                 return new Socket(hostName, portNum);
                         } 
                     });
        } catch (java.security.PrivilegedActionException pae) {
            IOException ioe = (IOException)pae.getException();
            throw ioe;
        }
        copen = true;
    }

    /**
     * Returns the InputStream associated with this HttpStreamConnection.
     *
     * @return InputStream object from which bytes can be read
     * @exception IOException if the connection is not open or the stream was 
     * already open
     */ 
    synchronized public InputStream openInputStream() throws IOException {
        if (!copen) {
            throw new IOException("Connection closed");
        }
        if (is == null)
            is = new UniversalFilterInputStream(this, socket.getInputStream());
        return is;
    }
    
    /**
     * Returns the OutputStream associated with this HttpStreamConnection.
     * 
     * @return OutputStream object for writing bytes over this connection
     * @exception IOException if the connection is not open or the stream was 
     * already open
     */
    synchronized public OutputStream openOutputStream() throws IOException {
        if (!copen) {
            throw new IOException("Connection closed");
        }
        if (os == null)
            os = new UniversalFilterOutputStream(
                        this, socket.getOutputStream());
        return os;
    }
    
    /**
     * Returns the DataInputStream associated with this HttpStreamConnection.
     * @exception IOException if the connection is not open or the stream was 
     * already open
     * @return a DataInputStream object
     */ 
    public DataInputStream openDataInputStream() throws IOException {
        if (dis == null)
            dis = new DataInputStream(openInputStream());
        return dis;
    }
     
    /** 
     * Returns the DataOutputStream associated with this HttpStreamConnection.
     * @exception IOException if the connection is not open or the stream was 
     * already open
     * @return a DataOutputStream object
     */
    public DataOutputStream openDataOutputStream() throws IOException {
        if (dos == null)
            dos = new DataOutputStream(openOutputStream());
        return dos;
    }
            
    /**
     * Closes the connection. The underlying TCP socket is also closed.
     * @exception IOException if the connection could not be
     *                        terminated cleanly
     */ 
    synchronized public void close() throws IOException {
        if (copen) {
            copen = false;
            socket.close();
            is = null;
            os = null;
            dis = null;
            dos = null;
        }
        return;
    }

}
