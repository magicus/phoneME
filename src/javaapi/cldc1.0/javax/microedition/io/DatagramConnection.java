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

package javax.microedition.io;

import java.io.*;

/**
 *
 * This interface defines the capabilities that a datagram connection
 * must have.
 * <p>
 * Reminder: In common with all the other addressing schemes
 * used for I/O in CLDC, the syntax for datagram addressing is not
 * defined in the CLDC Specification. Syntax definition can
 * only be take place at the profile level. The reason for this is 
 * that the datagram interface classes of CLDC can be used for 
 * implementing various kinds of datagram protocols. Examples 
 * include IP and WDP networks as well as infrared beaming 
 * protocols used by various PDAs and other devices. All these
 * protocols use very different addressing mechanisms.
 * <p>
 * In the sample implementation provided as part of the CLDC
 * implementation, the following addressing scheme
 * is used for UDP datagrams.
 * <p>
 * The parameter string describing the target of a connection
 * in the CLDC implementation takes the following form:
 *
 * <pre>
 * {protocol}://[{host}]:[{port}]
 * </pre>
 *
 * A datagram connection can be opened in a "client" mode or "server" mode.
 * If the "//{host}" part is missing then the connection  is opened as 
 * a "server" (by "server", we mean that a client application initiates
 * communication). When the "//{host}" part is specified, the connection
 * is opened as a "client".
 * <p>
 * Examples:
 * <p>
 *  A datagram connection for accepting datagrams<br>
 *  datagram://:1234<p>
 *  A datagram connection for sending to a server:<br>
 *  datagram://123.456.789.12:1234<p>
 *
 * Note that the port number in "server mode" (unspecified host name) is 
 * that of the receiving port. The port number in "client mode" (host name
 * specified) is that of the target port. The reply-to port in both cases
 * is never unspecified. In "server mode", the same port number is used for 
 * both receiving and sending. In "client mode", the reply-to port is 
 * always dynamically allocated.
 * <p>
 * The allocation of datagram objects is done in a more abstract way 
 * than in J2SE.  This is to allow a single platform to support several
 * different datagram interfaces simultaneously. Datagram objects must be
 * allocated by calling the "newDatagram" method of the DatagramConnection
 * object. The resulting object is defined using another interface type 
 * called "javax.microedition.io.Datagram".
 *
 * @version 1.1 1/7/2000
 */
public interface DatagramConnection extends Connection {

    /**
     * Get the maximum length a datagram can be.
     *
     * @return    The maximum length a datagram can be.
     * @exception IOException  If an I/O error occurs.
     */
    public int getMaximumLength() throws IOException;

    /**
     * Get the nominal length of a datagram.
     *
     * @return    The nominal length a datagram can be.
     * @exception IOException  If an I/O error occurs.
     */
    public int getNominalLength() throws IOException;

    /**
     * Send a datagram.
     *
     * @param     dgram        A datagram.
     * @exception IOException  If an I/O error occurs.
     * @exception InterruptedIOException Timeout or upon closing the
     *                                   connection with outstanding I/O.
     */
    public void send(Datagram dgram) throws IOException;

    /**
     * Receive a datagram.
     *
     * @param     dgram        A datagram.
     * @exception IOException  If an I/O error occurs.
     * @exception InterruptedIOException Timeout or upon closing the
     *                                   connection with outstanding I/O.
     */
    public void receive(Datagram dgram) throws IOException;

    /**
     * Make a new datagram object automatically allocating a buffer.
     *
     * @param  size            The length of the buffer to be allocated 
     *                         for the datagram
     * @return                 A new datagram
     * @exception IOException  If an I/O error occurs.
     * @exception IllegalArgumentException if the length is negative
     *                                     or larger than the buffer
     */
    public Datagram newDatagram(int size) throws IOException;

    /**
     * Make a new datagram object.
     *
     * @param  size            The length of the buffer to be used
     * @param  addr            The I/O address to which the datagram
     *                         will be sent
     * @return                 A new datagram
     * @exception IOException  If an I/O error occurs.
     * @exception IllegalArgumentException if the length is negative or
     *                         larger than the buffer, or if the address 
     *                         parameter is invalid
     */
    public Datagram newDatagram(int size, String addr) throws IOException;

    /**
     * Make a new datagram object.
     *
     * @param  buf             The buffer to be used in the datagram
     * @param  size            The length of the buffer to be allocated
     *                         for the datagram
     * @return                 A new datagram
     * @exception IOException  If an I/O error occurs.
     * @exception IllegalArgumentException if the length is negative or
     *                         larger than the buffer, or if the buffer
     *                         parameter is invalid
     */
    public Datagram newDatagram(byte[] buf, int size) throws IOException;

    /**
     * Make a new datagram object.
     *
     * @param  buf             The buffer to be used in the datagram
     * @param  size            The length of the buffer to be used
     * @param  addr            The I/O address to which the datagram
     *                         will be sent
     * @return                 A new datagram
     * @exception IOException  If an I/O error occurs.
     * @exception IllegalArgumentException if the length is negative or
     *                         larger than the buffer, or if the address
     *                         or buffer parameters is invalid
     */
    public Datagram newDatagram(byte[] buf, int size, String addr)
        throws IOException;

}
