/*
 * %W% %E%
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

package com.sun.cdc.io.j2me.serversocket;

import java.io.*;
import java.net.*;
import javax.microedition.io.*;
import com.sun.cdc.io.j2me.*;
import com.sun.cdc.io.*;

/**
 * StreamConnectionNotifier to the Palm Server Socket API.
 *
 * @author  Nik Shaylor
 * @version 1.0 10/08/99
 */
public class Protocol extends ConnectionBase 
                      implements StreamConnectionNotifier, ServerSocketConnection {

    /** Server Socket object */
    ServerSocket ssocket;

    /*
     * Check for listen permission on the given port. A SecurityException
     * will be thrown if the permission is not available. The MIDP
     * protocol handler should override this method to implement the
     * proper MIDP security check.
     */
    protected void checkPermission(int port) {
        // Check for SecurityManager.checkListen()
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null){
	    sm.checkListen(port);
	}
        return;
    }    
    
    /**
     * Open the connection
     * @param name the target for the connection
     * @param writeable a flag that is true if the caller expects to write to the
     *        connection.
     * @param timeouts A flag to indicate that the called wants timeout exceptions
     * <p>
     * The name string for this protocol should be:
     * "<port number>
     */
    public void open(String name, int mode, boolean timeouts) throws IOException {

        //if(name.charAt(0) != '/' || name.charAt(1) != '/' || name.charAt(2) != ':') {
        if(name.charAt(0) != '/' || name.charAt(1) != '/') { 
            throw new IllegalArgumentException("Protocol must start with \"//\" "+name);
        }

        /* socket:// case.  System assigned incoming port */
        if (name.length() == 2) {
           open();
           return;
        }

        if(name.charAt(2) != ':') { 
            throw new IllegalArgumentException("Protocol must start with \"//:\" "+name);
        }

        name = name.substring(3);
 
        /* socket://: case.  System assigned incoming port */
        if (name.length() == 0) {
            checkPermission(0);
            open();
            return;
        }

        try {
            final int port;

            /* Get the port number */
            port = Integer.parseInt(name);
            checkPermission(port);
            /* Open the socket: inbound server. Use a doPrivileged
             * block to avoid excessive prompting.
             */
            ssocket =
                (ServerSocket)java.security.AccessController.doPrivileged(
                 new java.security.PrivilegedExceptionAction() {
                     public Object run()
                         throws java.security.PrivilegedActionException,
                                IOException {
                         return new ServerSocket(port);
                     }
                 });
        } catch(NumberFormatException x) {
            throw new IllegalArgumentException("Invalid port number in "+name);
        } catch(java.security.PrivilegedActionException pae) {
            IOException ioe = (IOException)pae.getException();
            throw ioe;
        }
    }

    /**
     * Open the connection to an arbitrary system selected port and address
     */
     public void open() throws IOException {
        ssocket = new ServerSocket();
        /* binding to null to get the system assigned port */
        ssocket.bind(null);
     }

    /**
     * Returns a GenericConnection that represents a server side
     * socket connection
     * @return     a socket to communicate with a client.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          input stream.
     */
    public StreamConnection acceptAndOpen() throws IOException {
        Socket soc = ssocket.accept();
        com.sun.cdc.io.j2me.socket.Protocol con =
            new com.sun.cdc.io.j2me.socket.Protocol();
        con.open(soc);
        return con;
    }

    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs when closing the
     *                          connection.
     */
    public void close() throws IOException {
        ssocket.close();
    }

    /**
     * Gets the local address to which the socket is bound.
     *
     * <P>The host address(IP number) that can be used to connect to this
     * end of the socket connection from an external system.
     * Since IP addresses may be dynamically assigned, a remote application
     * will need to be robust in the face of IP number reassignment.</P>
     * <P> The local hostname (if available) can be accessed from
     * <code> System.getProperty("microedition.hostname")</code>
     * </P>
     *
     * @return the local address to which the socket is bound.
     * @exception  IOException  if the connection was closed
     * @see SocketConnection
     */
    public  String getLocalAddress() throws IOException { 
        if (ssocket.isClosed()) 
           throw new IOException("ServerSocketConnection is closed");

        /*
         * getLocalAddress calls SecurityManager.checkConnect,
         * so a privileged block is used to avoid excessive security prompting.
         */
	InetAddress addr =
            (InetAddress)java.security.AccessController.doPrivileged(
                 new java.security.PrivilegedAction() {
                     public Object run() {
                         try {
                             return InetAddress.getLocalHost();
                         } catch (java.net.UnknownHostException uhe) {
                             return null;
                         }
                     }
                 });

        if (addr == null) 
            return null;
        else 
            return addr.getHostAddress();
    }

    /**
     * Returns the local port to which this socket is bound.
     *
     * @return the local port number to which this socket is connected.
     * @exception  IOException  if the connection was closed
     * @see SocketConnection
     */
    public  int  getLocalPort() throws IOException {
        if (ssocket.isClosed()) 
           throw new IOException("ServerSocketConnection is closed");

        return  ssocket.getLocalPort();
    }
}
