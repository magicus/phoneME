/*
 * @(#)Protocol.java	1.33 06/10/13
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

package com.sun.cdc.io.j2me.datagram;

import java.io.*;
import java.net.*;
import javax.microedition.io.*;
import com.sun.cdc.io.j2me.*;
import com.sun.cdc.io.*;

/**
 * This implements the "datagram://" protocol for J2SE in a not very
 * efficient way.
 *
 * @version 1.1 11/19/99
 */
public class Protocol extends ConnectionBase implements DatagramConnection,UDPDatagramConnection {

    DatagramSocket endpoint;

    /**
     * Machine name
     */
    private String host = null;

    /**
     * Port
     */
    private int port = 0;

    private boolean open;

    public  String getLocalAddress() throws IOException {
        if (!open) {
            throw new IOException("Connection closed");
        }
        if (host != null) 
            return host;
        else /* it is datagram://: string, server endpoint */
            return InetAddress.getLocalHost().getHostAddress();
    }

    public  int  getLocalPort() throws IOException {
        if (!open) {
            throw new IOException("Connection closed");
        }
        return endpoint.getLocalPort();
    } 

    /**
     * Local function to get the machine address from a string
     */
    protected static String getAddress(String name) throws IOException {
        /* Look for the : */
        int colon = name.indexOf(':');

        if(colon < 0) {
            throw new IllegalArgumentException("No ':' in protocol name "+name);
        }

        if(colon == 0) {
            return null;
        } else {
            return parseHostName(name, colon);
        }
    }

    
    protected static String parseHostName(String connection, int colon) {
        /* IPv6 addresses are enclosed within [] */
        if ((connection.indexOf("[") == 0) && (connection.indexOf("]") > 0)) {
            return parseIPv6Address(connection, colon);
        } else {
            return parseIPv4Address(connection, colon);
        }
    }


    protected static String parseIPv4Address(String name, int colon) {
        return name.substring(0, colon);
    }


    protected static String parseIPv6Address(String address, int colon) {
        int closing = address.indexOf("]");
        /* beginning '[' and closing ']' should be included in the hostname*/
        return address.substring(0, closing+1);
    }
    

    /**
     * Local function to get the port number from a string
     */
    protected static int getPort(String name) throws IOException, NumberFormatException {
        /* Look for the : */
        int colon = name.lastIndexOf(':');

        if(colon < 0) {
            throw new IllegalArgumentException("No ':' in protocol name "+name);
        }

        return Integer.parseInt(name.substring(colon+1));
    }

    /*
     * We will throw a SecurityException if permission
     * checks fail. For CDC the DatagramSocket obect
     * creation will do this itself so we can use that
     * checking. This method should be overriden by MIDP
     * protocol handlers to properly check MIDP permissions.
    */
    protected void checkPermission(String host, int port) {
        return;
    }

    /*
     * Check permission when opening an OutputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. Throw a SecurityException if
     * the connection is not allowed. Currently the datagram
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
     * datagram protocol handler does not make a permission
     * check at this point so this method is empty.
     */
    protected void inputStreamPermissionCheck() {
        return;
    }

    /**
     * Open a connection to a target. <p>
     *
     * The name string for this protocol should be:
     * "[address:][port]"
     *
     * @param name the target of the connection
     * @param writeable a flag that is true if the caller intends to
     *        write to the connection.
     * @param timeouts A flag to indicate that the called wants timeout exceptions
     */
    public void open(String name, int mode, boolean timeout) throws IOException {

        if(name.charAt(0) != '/' || name.charAt(1) != '/') {
            throw new IllegalArgumentException("Protocol must start with \"//\" "+name);
        }
        name = name.substring(2);

       /*
        * If 'name' == null then we are a server endpoint at port 'port'
        *
        * If 'name' != null we are a client endpoint at an port decided by the system
        *              and the default address for datagrams to be send is 'host':'port'
        */
        
        /* If name does not have port number, just a colon, it should at 
         * system assigned port. Otherwise use the port and host specified.
         */

        if (name.substring(name.indexOf(':') + 1).length() != 0) {
	    host = getAddress(name);
            port = getPort(name);
            if(port <= 0) {
                throw new IllegalArgumentException("Bad port number \"//\" "+name);
            }
	}

	checkPermission(host, port);

	if(host == null) {
	    /* Open a server datagram socket (no host given) */
	    endpoint = new DatagramSocket(port);
	} else { 
	    /* Open a random port for a datagram client */
	    endpoint = new DatagramSocket();
	}
            
        open = true;

        try {
	    
	    byte[] testbuf = new byte[256];
	    DatagramPacket testdgram = new DatagramPacket(testbuf, 256);

	    if (host != null) {
		testdgram.setAddress(InetAddress.getByName(host));	
	    } else {
		testdgram.setAddress(InetAddress.getByName("localhost"));
	    }
	    testdgram.setPort(port);
	} catch(NumberFormatException x) {
            throw new IllegalArgumentException("Invalid datagram address "
					       +host+":"+port);
        } catch(UnknownHostException x) {
            throw new IllegalArgumentException("Unknown host "+host+":"+port);
        } catch(SecurityException se) {
        // Don't need to report on the security
        // exceptions yet at this point
        } 
    }

   /**
     * Get the address represented by this datagram endpoint
     *
     * @return    address      The datagram addre4ss
     */
     public String getAddress() {
         InetAddress addr = endpoint.getLocalAddress();
         return "datagram://" + addr.getHostName() + ":" + addr.getHostAddress();
     }

    /**
     * Get the maximum length a datagram can be.
     *
     * @return    address      The length
     */
    public int getMaximumLength() throws IOException {
        try {
	    int receiveLen = endpoint.getReceiveBufferSize();
	    int sendLen = endpoint.getSendBufferSize();
	    /* return lesser of the two */
	    return (receiveLen < sendLen ? receiveLen : sendLen);
        } catch(java.net.SocketException x) {
            throw new IOException(x.getMessage());
        }
    }

    /**
     * Get the nominal length of a datagram.
     *
     * @return    address      The length
     */
    public int getNominalLength() throws IOException {
        return getMaximumLength();
    }

    /**
     * Change the timeout period
     *
     * @param     milliseconds The maximum time to wait
     */
    public void setTimeout(int milliseconds) {
    }

    /**
     * Send a datagram
     *
     * @param     dgram        A datagram
     * @exception IOException  If an I/O error occurs
     */
    public void send(Datagram dgram) throws IOException {
        DatagramObject dh = (DatagramObject)dgram;
        endpoint.send(dh.dgram);
    }

    /**
     * Receive a datagram
     *
     * @param     dgram        A datagram
     * @exception IOException  If an I/O error occurs
     */
    public void receive(Datagram dgram) throws IOException {
        DatagramObject dh = (DatagramObject)dgram;

        endpoint.receive(dh.dgram);

	// Set the return DatagramObject handle to have the address from the
	//   received DatagramPacket
    int recv_port = dh.dgram.getPort();
    String recv_host = dh.dgram.getAddress().getHostName();
    if(recv_host != null) {
            try {
                dh.setAddress("datagram://" + recv_host + ":" + recv_port);
            } catch(IOException x) {
                throw new 
		    RuntimeException("IOException in datagram::receive");
            }
        } else {
	    try {
		dh.setAddress("datagram://:"+recv_port);
	    } catch(IOException x) {
		throw new RuntimeException("IOException in datagram::receive");
	    }
	} 
        dh.pointer = 0;
    }

    /**
     * Close the connection to the target.
     *
     * @exception IOException  If an I/O error occurs
     */
    public void close() throws IOException {
        if (open) {
            open = false;
        }
        endpoint.close();
    }

    /**
     * Get a new datagram object
     *
     * @return                 A new datagram
     */
    public Datagram newDatagram(int size)  throws IllegalArgumentException, IOException {
        // Check for negative size
	if (size < 0) {
	    throw new IllegalArgumentException("Size is negative: "+size);
        }
        return newDatagram(new byte[size], size);
    }

    /**
     * Get a new datagram object
     *
     * @param     addr         The address to which the datagram must go
     * @return                 A new datagram
     */
    public Datagram newDatagram(int size, String addr) throws IOException ,
	IllegalArgumentException {
        // Check for negative size
	if (size < 0) {
	    throw new IllegalArgumentException("Size is negative: "+size);
        }
        return newDatagram(new byte[size], size, addr);
    }

    /**
     * Get a new datagram object
     *
     * @return                 A new datagram
     */
    public Datagram newDatagram(byte[] buf, int size)  throws IOException, IllegalArgumentException {
// Check for negative size
	if (size < 0) {
	    throw new IllegalArgumentException("Size is negative: "+size);
        }

        // Check if size > max size
        try {
          if (size > getMaximumLength()) {
            throw new IllegalArgumentException("Size: "+size+" is more than max size: "+getMaximumLength());
          }
        } catch (IOException e) {
          throw (e);
        }

        // Check for a null value for buf
	if (buf == null) {
	    throw new IllegalArgumentException("buf is null");
        }

        DatagramObject dg = new DatagramObject(new DatagramPacket(buf,size));
        if(host != null) {
            try {
                dg.setAddress("datagram://"+host+":"+port);
            } catch(IOException x) {
                throw new RuntimeException("IOException in datagram::newDatagram");
            }
        } 
        /* Fix CR 6557544 */
        /*
        else {
           try {
             dg.setAddress("datagram://:"+port);
           } catch(IOException x) {
             throw new RuntimeException("IOException in datagram::newDatagram");
           }
        }
        */
        return dg;
    }

    /**
     * Get a new datagram object
     *
     * @param     addr         The address to which the datagram must go
     * @return                 A new datagram
     */
    public Datagram newDatagram(byte[] buf, int size, String addr) throws IOException , IllegalArgumentException {
        // Check for negative size
	if (size < 0) {
	    throw new IllegalArgumentException("Size is negative: "+size);
        }

        // Check if size > max size
        try {
          if (size > getMaximumLength()) {
            throw new IllegalArgumentException("Size: "+size+" is more than max size: "+getMaximumLength());
          }
        } catch (IOException e) {
          throw (e);
        }

        // Check if addr is null
        if (addr == null) {
	    throw new IllegalArgumentException("addr is null");
        }

        // Check for a null value for buf
	if (buf == null) {
	    throw new IllegalArgumentException("buf is null");
        }

        DatagramObject dh = (DatagramObject)newDatagram(buf, size);
        dh.setAddress(addr);
        return dh;
    }

}
