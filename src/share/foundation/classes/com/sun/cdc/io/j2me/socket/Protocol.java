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

package com.sun.cdc.io.j2me.socket;

import java.io.*;
import java.net.*;
import javax.microedition.io.*;
import com.sun.cdc.io.j2me.*;
import com.sun.cdc.io.*;

/**
 * GenericStreamConnection to the J2SE socket API.
 *
 * @author  Nik Shaylor
 * @version 1.0 10/08/99
 */

public class Protocol extends ConnectionBase implements StreamConnection, SocketConnection {

    /** Socket object */
    Socket socket;

    /** Open count */
    int opens = 0;

    /** IPv6 address */
    boolean ipv6 = false;

    /**
     * Open the connection
     */
    public void open(String name, int mode, boolean timeouts) throws IOException {
        throw new RuntimeException("Should not be called");
    }

    /*
     * Check permission to connect to the indicated host.
     * This should be overriden by the MIDP protocol handler
     * to check the proper MIDP permission.
     */
    protected void checkPermission(String host, int port) {
        // Check for SecurityManager.checkConnect()
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null){
            if (host != null) {
                sm.checkConnect(host, port);
            } else {
                sm.checkConnect("localhost", port);
            }
        }               
        return;
    }

    /*
     * Check permission when opening an OutputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. Throw a SecurityException if
     * the connection is not allowed. Currently the socket
     * protocol handler does not make a permission check at
     * this point so this method is empty.
     */
    protected void outputStreamPermissionCheck() {
        return;
    }

    /*
     * Check permission when opening an InputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. A SecurityException will be
     * raised if the connection is not allowed. Currently the
     * socket protocol handler does not make a permission
     * check at this point so this method is empty.
     */
    protected void inputStreamPermissionCheck() {
        return;
    }

    /*
     * Creates a server socket. MIDP
     * versions of the protocol handler should override this
     * method.
     */
    protected com.sun.cdc.io.j2me.serversocket.Protocol createServerSocket() {
        return new com.sun.cdc.io.j2me.serversocket.Protocol();
    }

    /**
     * Open the connection
     * @param name the target for the connection
     * @param writeable a flag that is true if the caller expects to write to the
     *        connection.
     * @param timeouts A flag to indicate that the called wants timeout exceptions
     * <p>
     * The name string for this protocol should be:
     * "<name or IP number>:<port number>
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
        throws IOException {
        if(name.charAt(0) != '/' || name.charAt(1) != '/') {
            throw new IllegalArgumentException("Protocol must start with \"//\" "+name);
        }

        name = name.substring(2);

        try {
            /* Host name or IP number */
            String nameOrIP = "";

            /* Port number */
            final int port;

            /* Look for the : */
            int colon = name.indexOf(':');

            if(colon != -1) {
               /* Strip off the protocol name */
                nameOrIP = parseHostName(name, colon);
               /* Host name should not contain reserved characters specified in RFC 2396 */
               if ((nameOrIP.indexOf('/') != -1) || (nameOrIP.indexOf('@') != -1) || (nameOrIP.indexOf('?') != -1) ||  (nameOrIP.indexOf(';') != -1)) {
                       throw new IllegalArgumentException("hostname " + nameOrIP + " cannot contain \"?\" , \"@\" , \";\", \":\", or \"/\" character.");
               }
            }

            if(nameOrIP.length() == 0) {
                /*
                 * If the open string is "socket://:nnnn" then we regard this as
                 * "serversocket://:nnnn"
                 */
                 /* socket:// and socket://: are also valid serversocket urls */
                com.sun.cdc.io.j2me.serversocket.Protocol con =
                    createServerSocket();
                con.open("//"+name, mode, timeouts);
                return con;
            }

            /* Get the port number */
            if (ipv6) {
                int lastColon = name.lastIndexOf(":");
                port = Integer.parseInt(name.substring(lastColon+1));
            } else {
                port = Integer.parseInt(name.substring(colon+1));
            }

            /* If the proper security check has not been made, make
             * it now.
             */
	    checkPermission(nameOrIP, port);

            /* Open the socket. Use a doPrivileged block
             * to avoid excessive prompting.
             */
            final String hostName = nameOrIP;
            socket = (Socket)java.security.AccessController.doPrivileged(
                 new java.security.PrivilegedExceptionAction() {
                     public Object run() throws
                         java.security.PrivilegedActionException,
                         IOException {
                             return new Socket(hostName, port);
                         } 
                     });
            opens++;
            return this;
        } catch(NumberFormatException x) {
            throw new IllegalArgumentException("Invalid port number in "+name);
        } catch(java.security.PrivilegedActionException pae) {
            IOException ioe = (IOException)pae.getException();
            throw ioe;
        }
    }

    /**
     * Open the connection
     * @param socket an already formed socket
     * <p>
     * This function is only used by com.sun.kjava.system.palm.protocol.socketserver;
     */
    public void open(Socket socket) throws IOException {
        this.socket = socket;
    }


    private String parseHostName(String connection, int colon) {
        /* IPv6 addresses are enclosed within [] */
        int beginIndex = connection.indexOf("[");
        int endIndex = connection.indexOf("]");
        
        if (beginIndex > endIndex) {
            throw new IllegalArgumentException("invalid host name " + connection);
        }
        if ((beginIndex ==0) && (endIndex >0)) {
            return parseIPv6Address(connection, endIndex);
        } else {
            return parseIPv4Address(connection, colon);
        }
    }


    private String parseIPv4Address(String name, int colon) {
        return name.substring(0, colon);
    }


    private String parseIPv6Address(String address, int closing) {
        ipv6 = true;
        /* beginning '[' and closing ']' should be included in the hostname*/
        return address.substring(0, closing+1);
    }

    /**
     * Returns an input stream for this socket.
     *
     * @return     an input stream for reading bytes from this socket.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          input stream.
     */
    public InputStream openInputStream() throws IOException {
        inputStreamPermissionCheck();
        InputStream is = new UniversalFilterInputStream(this, socket.getInputStream());
        opens++;
        return is;
    }

    /**
     * Returns an output stream for this socket.
     *
     * @return     an output stream for writing bytes to this socket.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          output stream.
     */
    public OutputStream openOutputStream() throws IOException {
        outputStreamPermissionCheck();
        OutputStream os = new UniversalFilterOutputStream(this,
                                 socket.getOutputStream());
        opens++;
        return os;
    }

    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs when closing the
     *                          connection.
     */
    public void close() throws IOException {
        if(--opens == 0) {
            socket.close();
        }
    }

    /**
     * Set a socket option for the connection.
     * <P>
     * Options inform the low level networking code about intended
     * usage patterns that the application will use in dealing with
     * the socket connection.
     * </P>
     * <P>
     * Calling <code>setSocketOption</code> to assign buffer sizes
     * is a hint to the platform of the sizes to set the underlying
     * network I/O buffers.
     * Calling <code>getSocketOption</code> can  be used to see what
     * sizes the system is using.
     * The system MAY adjust the buffer sizes to account for
     * better throughput available from Maximum Transmission Unit
     * (MTU) and Maximum Segment Size (MSS) data available
     * from current network information.
     * </P>
     *
     * @param option socket option identifier (KEEPALIVE, LINGER,
     * SNDBUF, RCVBUF, or DELAY)
     * @param value numeric value for specified option
     * @exception  IllegalArgumentException if  the value is not
     *              valid (e.g. negative value) or if the option
     *              identifier is not valid
     * @exception  IOException  if the connection was closed
     *
     * @see #getSocketOption
     */
    public void setSocketOption(byte option,  int value)
        throws IllegalArgumentException, IOException {

       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       /* value can never be negative */
       if (value < 0) {
               throw new IllegalArgumentException("Value cannot be negative");
       }
       switch (option) {
          case SocketConnection.DELAY: 
                       socket.setTcpNoDelay(value != 0);
                       break;				
          case SocketConnection.KEEPALIVE: 
                       socket.setKeepAlive(value != 0);
                       break;				
          case SocketConnection.LINGER: 
                       socket.setSoLinger(value != 0, value);
                       break;				
          case SocketConnection.RCVBUF: 
	               if (value > 0) {
			   socket.setReceiveBufferSize(value);
		       }
                       break;				
          case SocketConnection.SNDBUF: 
	              if (value > 0) {
			  socket.setSendBufferSize(value);
		      }
                       break;				
          default:
              throw new IllegalArgumentException("Option identifier is out of range");
       }
    }

    /**
     * Get a socket option for the connection.
     *
     * @param option socket option identifier (KEEPALIVE, LINGER,
     * SNDBUF, RCVBUF, or DELAY)
     * @return  numeric value for specified option or -1 if the
     *  value is not available.
     * @exception IllegalArgumentException if the option identifier is
     *  not valid
     * @exception  IOException  if the connection was closed
     * @see #setSocketOption
     */
    public  int getSocketOption(byte option)
        throws IllegalArgumentException, IOException { 

       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       switch (option) {
          case SocketConnection.DELAY: 
                       return (socket.getTcpNoDelay()) ? 1 : 0 ;
          case SocketConnection.KEEPALIVE: 
                       return (socket.getKeepAlive()) ? 1 : 0 ;
          case SocketConnection.LINGER: 
                       return socket.getSoLinger();
          case SocketConnection.RCVBUF: 
                       return socket.getReceiveBufferSize();
          case SocketConnection.SNDBUF: 
                       return socket.getSendBufferSize();
          default:
              throw new IllegalArgumentException("Option identifier is out of range");
       }
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
     * @exception  IOException  if the connection was closed.
     * @see ServerSocketConnection
     */
    public  String getLocalAddress() throws IOException { 
       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       return socket.getLocalAddress().getHostName();
    }

    /**
     * Returns the local port to which this socket is bound.
     *
     * @return the local port number to which this socket is connected.
     * @exception  IOException  if the connection was closed.
     * @see ServerSocketConnection
     */
    public  int  getLocalPort() throws IOException { 
       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       return socket.getLocalPort();
    }

    /**
     * Gets the remote address to which the socket is bound.
     * The address can be either the remote host name or the IP
     * address(if available).
     *
     * @return the remote address to which the socket is bound.
     * @exception  IOException  if the connection was closed.
     */
    public  String getAddress() throws IOException { 
       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       return socket.getInetAddress().getHostName();
    }

    /**
     * Returns the remote port to which this socket is bound.
     *
     * @return the remote port number to which this socket is connected.
     * @exception  IOException  if the connection was closed.
     */
    public  int  getPort() throws IOException { 
       if (socket.isClosed()) {
          throw new IOException("Socket is closed");
       }
       return socket.getPort();
    }
   
}
