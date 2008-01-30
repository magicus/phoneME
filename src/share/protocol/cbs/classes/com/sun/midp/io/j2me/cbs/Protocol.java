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

package com.sun.midp.io.j2me.cbs;

import com.sun.midp.io.j2me.ProtocolBase;

import com.sun.midp.io.j2me.sms.TextEncoder;

import com.sun.j2me.security.WMAPermission;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Vector;
import javax.microedition.io.Connector;
import javax.microedition.io.Connection;
import javax.wireless.messaging.Message;
import javax.wireless.messaging.MessageConnection;

// Exceptions
import java.io.IOException;
import java.io.InterruptedIOException;


/**
 * CBS message connection implementation.
 *
 * Cell Broadcast message connections are receive only.
 *
 */
public class Protocol extends ProtocolBase {

    /** Currently opened connections. */
    static protected Vector openConnections = new Vector();

    /** Local handle for port number. */
    private int m_imsgid = 0;

    /** Name of current connection. */
    protected String url;

    /**  DCS: GSM Alphabet  */
    protected static final int GSM_TEXT = 0;

    /**  DCS: Binary */
    protected static final int GSM_BINARY = 1;

    /**  DCS: Unicode UCS-2 */
    protected static final int GSM_UCS2 = 2;

    /** Creates a message connection protocol handler. */
    public Protocol() {
        super();
        ADDRESS_PREFIX = "cbs://";
    }

    /**
     * Gets the connection parameter in string mode.
     * @return string that contains a parameter 
     */
    protected String getAppID() {
        if (m_imsgid > 0) {
            return new String(Integer.toString(m_imsgid));
        } else {
            return null;
        }
    }

    /**
     * Sets the connection parameter in string mode.
     * @param newValue new value of connection parameter 
     */
    protected void setAppID(String newValue) {
        try {
            m_imsgid = Integer.parseInt(newValue);
        } catch (NumberFormatException exc) {
            m_imsgid = 0;
        }
    }

    /**
     * The internal representation of the CBS data structure. This is also
     * intended to represent the serialized format of the data fields in a
     * CBS data packet.
     */
    private class CBSPacket {
        /** Type of message */
        public int encodingType;
        /** Message ID (Port/Channel number). */
        public int msgID;
        /** Message buffer */
        public byte[] message;
    };


    /*
     * Native function prototypes
     */

    /**
     * Native function to open a CBS connection.
     *
     * @param msgID    The message ID to be matched against incoming messages.
     * @param msid     Application Suite ID.
     *
     * @return    returns handle to the open CBS connection.
     */
    private native int open0(int msgID, int msid) throws IOException;

    /**
     * Unblock the receive thread.
     *
     * @param msid The Application suite ID.
     *
     * @return  returns handle to the connection.
     */
    protected int unblock00(int msid)
        throws IOException {
        return open0(0, msid);
    }

    /**
     * Native function to close cbs connection
     *
     * @param port    The port number to close.
     * @param handle  The CBS handle returned by open0.
     * @param deRegister Deregistration appID when parameter is 1.
     *
     * @return        0 on success, -1 on failure.
     */
    private native int close0(int port, int handle, int deRegister);

    /**
     * Close connection.
     *
     * @param connHandle handle returned by open0
     * @param deRegister Deregistration appID when parameter is 1.
     * @return    0 on success, -1 on failure
     */
    protected int close00(int connHandle, int deRegister) {
        return close0(m_imsgid, connHandle, deRegister);
    }

    /**
     * Receives a CBS message.
     *
     * @param port    The port used for incoming messages.
     * @param msid     Application Suite ID.
     * @param handle    The handle used to open the CBS connection.
     * @param cbsPacket    The received packet.
     *
     * @return    The number of bytes received.
     *
     * @exception IOException  if an I/O error occurs
     */
    private native int receive0(int port, int msid,
                                int handle,
                                CBSPacket cbsPacket) throws IOException;

    /**
     * Waits until message available
     *
     * @param port    The port used for incoming messages.
     * @param handle    The handle to the CBS connection.
     *
     * @return    <code>0</code> on success, <code>-1</code> on failure
     *
     * @exception IOException  if an I/O error occurs
     */
    private native int waitUntilMessageAvailable0(int port, int handle)
                               throws IOException;

    /**
     * Waits until message available
     *
     * @param handle handle to connection
     * @return 0 on success, -1 on failure
     * @exception IOException  if an I/O error occurs
     */
    protected int waitUntilMessageAvailable00(int handle) 
        throws IOException {
        return waitUntilMessageAvailable0(m_imsgid, handle);
    }


    /*
     * Helper methods
     */

    /**
     * Checks the internal setting of the receive permission. Called from
     * the <code>receive</code> and <code>setMessageListener</code> methods.
     *
     * @exception InterruptedIOException if permission dialog was pre-empted.
     */
    protected void checkReceivePermission() throws InterruptedIOException {

        /* Check if we have permission to receive. */
        if (readPermission == false) {
            try {
                appPackage.checkForPermission(WMAPermission.CBS_RECEIVE);
                readPermission = true;
            } catch (InterruptedException ie) {
                throw new InterruptedIOException("Interrupted while trying " +
                                               "to ask the user permission.");
            }
        }
    }

    /*
     * MessageConnection Interface
     */

    /**
     * Constructs a new message object of a binary or text type.
     * If the <code>TEXT_MESSAGE</code> constant is passed in, the
     * <code>TextMessage</code> interface is implemented by the created object.
     * If the <code>BINARY_MESSAGE</code> constant is passed in, the
     * <code>BinaryMessage</code> interface is implemented by the created
     * object. 
     * <p>
     * If this method is called in a sending mode, a new <code>Message</code>
     * object is requested from the connection. For example:
     * <p>
     * <code>Message msg = conn.newMessage(TEXT_MESSAGE);</code>
     * <p>
     * The <code>Message</code> object that was created doesn't have the
     * destination address set. It's the application's responsibility to set it
     * before the message is sent.
     * <p>
     * If this method is called in receiving mode, the
     * <code>Message</code> object does have
     * its address set. The application can act on the object to extract
     * the address and message data.
     * <p>
     * <!-- The <code>type</code> parameter indicates the number of bytes
     * that should be
     * allocated for the message. No restrictions are placed on the application
     * for the value of <code>size</code>.
     * A value of <code>null</code> is permitted and creates a
     * <code>Message</code> object
     * with a 0-length message. -->
     *
     * @param type <code>TEXT_MESSAGE</code> or
     *     <code>BINARY_MESSAGE</code>.
     *
     * @return A new CBS <code>Message</code> object.
     */
    public Message newMessage(String type) {

        /* Create the CBS-formatted URL. */
        String address = ADDRESS_PREFIX;
        if (m_imsgid != 0) {
            address = address + ":" + String.valueOf(m_imsgid);
        }

        return newMessage(type, address);
    }

    /**
     * Constructs a new message object of a text or binary type and specifies
     * a destination address.
     * If the <code>TEXT_MESSAGE</code> constant is passed in, the
     * <code>TextMessage</code> interface is implemented by the created object.
     * If the <code>BINARY_MESSAGE</code> constant is passed in, the
     * <code>BinaryMessage</code> interface is implemented by the created
     * object.
     * <p>
     * The destination address <code>addr</code> has the following format:
     * </p>
     * <p>
     * <code>cbs://<em>phone_number</em>:<em>port</em></code>
     * </p>
     *
     * @param type <code>TEXT_MESSAGE</code> or
     *     <code>BINARY_MESSAGE</code>.
     * @param addr The destination address of the message.
     *
     * @return  A new CBS <code>Message</code> object.
     */
    public Message newMessage(String type, String addr) {
        Message msg = null;

        if (type.equals(MessageConnection.TEXT_MESSAGE)) {
            msg = new TextObject(addr);

        } else if (type.equals(MessageConnection.BINARY_MESSAGE)) {
            msg = new BinaryObject(addr);

        } else {
            throw new IllegalArgumentException("Message type not supported.");
        }

        return msg;
    }

    /**
     * Cell broadcast connections are read-only connections. Calling this
     * method causes an <code>IOException</code> to be thrown.
     *
     * @param msg    Placeholder: A <code>Message</code> object.
     *
     * @exception    IOException always thrown.
     */
    public void send(Message msg) throws IOException {
        throw new IOException("Send not supported.");
    }

    /**
     * Receives the bytes that have been sent over the connection,
     * constructs a <code>Message</code> object, and returns it.
     * <p>
     * If there are no <code>Message</code>s waiting on the connection,
     * this method will block until the <code>MessageConnection</code>
     * is closed, or a message is received.
     *
     * @return a <code>Message</code> object
     * @exception java.io.IOException if an error occurs while receiving
     *         a message.
     * @exception java.io.InterruptedIOException if this
     *         <code>MessageConnection</code> object is closed during the
     *         call of this method.
     * @exception java.lang.SecurityException if the application doesn't have
     *         permission to receive messages on the given port.
     */
    public synchronized Message receive() throws IOException {

        checkReceivePermission();

        /* Make sure the connection is still open. */
        ensureOpen();

        /* The connection must be read-only with no host address. */
        if (m_mode == Connector.WRITE) {
            throw new IOException("Invalid connection mode.");
        }

        /* No message received yet. */
        Message msg = null;
        int length = 0;

        try {

            CBSPacket cbsPacket = new CBSPacket();

            /*
             * Packet has been received and deleted from inbox.
             * Time to wake up receive thread.
             */
                // Pick up the CBS message from the message pool.
            length = receive0(m_imsgid, appPackage.getId(),
                              connHandle, cbsPacket);

            if (length < 0) {
                throw new InterruptedIOException("Connection closed.");
            }

            /* Messages other than binary are assumed to be text. */
            String type = MessageConnection.TEXT_MESSAGE;
            boolean isTextMessage = true;
            if (cbsPacket.encodingType == GSM_BINARY)  {
                type = MessageConnection.BINARY_MESSAGE;
                isTextMessage = false;
            }

            /* Construct a message with proper encoding type and address. */
            msg = newMessage(type,
                new String(ADDRESS_PREFIX + ":" + cbsPacket.msgID));

            /* Set message payload as text or binary. Message can be null. */
            if (isTextMessage) {

                String text = null;
                if (cbsPacket.message != null) {
                    if (cbsPacket.encodingType == GSM_TEXT) {
                        text = new String(TextEncoder.toString(
                            TextEncoder.decode(cbsPacket.message)));
                    } else {
                        text = new String(TextEncoder.toString(
                            cbsPacket.message));

                    }
                } else {
                    // null message. Set to empty string
                    text = new String("");
                }
                ((TextObject)msg).setPayloadText(text);
            } else {

                if (cbsPacket.message != null) {
                    ((BinaryObject)msg).setPayloadData(cbsPacket.message);
                } else {
                    // null message. Set to empty byte array
                    ((BinaryObject)msg).setPayloadData(new byte[0]);
                }
            }

        } catch (InterruptedIOException ex) {
            throw new InterruptedIOException("MessageConnection closed.");
        } catch (IOException ex) {
            io2InterruptedIOExc(ex, "receiving");
        }

        return msg;
    }


    /**
     * Returns the number of segments required to send the given
     * <code>Message</code>.
     *
     * <p>Note: The message is not actually sent. The number of protocol
     * segments is simply computed.
     * </p>
     * <p>This method calculates the number of segments required
     * when this message is split into the protocol segments
     * utilizing the underlying protocol's features.
     * Possible implementation's limitations that may limit the number of
     * segments that can be sent using it are not taken into account. These
     * limitations are protocol specific. They are documented
     * with that protocol's adapter definition.
     * </p>
     * @param message The message to be used for the computation.
     *
     * @return The number of protocol segments needed for sending the message.
     *     Returns <code>0</code> if the <code>Message</code> object cannot be
     *     sent using the underlying protocol.
     */
    public int numberOfSegments(Message message) {

        /* When a message is present, there is always just one segment. */
        return (message != null) ? 1 : 0;
    }


    /**
     * Closes the connection. Resets the connection <code>open</code> flag to
     * <code>false</code>. Subsequent operations on a closed connection should
     * throw an appropriate exception.
     *
     * @exception IOException  if an I/O error occurs
     */
    public void close() throws IOException {

        /*
         * Set m_imsgid to 0, in order to quit out of the while loop
         * in the receiver thread.
         */
        int save_imsgid = m_imsgid;
        m_imsgid = 0;

        synchronized (closeLock) {
            if (open) {
                /*
                 * Reset open flag early to prevent receive0 executed by
                 * concurrent thread to operate on partially closed
                 * connection
                 */
                open = false;
                /* Close the connection and unregister the application ID. */
                close0(save_imsgid, connHandle, 1);

                setMessageListener(null);

                /*
                 * Reset handle and other params to default
                 * values. Multiple calls to close() are allowed
                 * by the spec and the resetting would prevent any
                 * strange behaviour.
                 */
                connHandle = 0;
                m_mode = 0;

                /*
                 * Remove this connection from the list of open connections.
                 */
                for (int i = 0, n = openConnections.size(); i < n; i++) {
                    if (openConnections.elementAt(i) == this) {
                        openConnections.removeElementAt(i);
                        break;
                    }
                }
            }
        }
    }

    /*
     * ConnectionBaseInterface Interface
     */

    /**
     * Opens a CBS connection. This method is called from the
     * <code>Connector.open()</code> method to obtain the destination
     * address given in the <code>name</code> parameter.
     * <p>
     * The format for the <code>name</code> string for this method is:
     * </p>
     * <p>
     * <code>cbs://<em>[phone_number</em>:<em>][port_number]</em></code>
     * </p>
     * <p>
     * where the <em>phone_number:</em> is optional. If the
     * <em>phone_number</em> parameter is present, the connection is being
     * opened in client mode. This means that messages can be sent. If the
     * parameter is absent, the connection is being opened in server mode.
     * This means that messages can be sent and received.
     * <p>
     * The connection that is opened is to a low-level transport mechanism
     * which can be any of the following:
     * <ul>
     * <li>a datagram Short Message Peer-to-Peer (SMPP)
     * to a service center </li>
     * <li>a <code>comm</code> connection to a phone device with
     *     AT-commands</li>.
     * <li>a native CBS stack</li>
     *  </ul>
     * Currently, the <code>mode</code> and <code>timeouts</code> parameters are
     * ignored.
     *
     * @param name the target of the connection
     * @param mode indicates whether the caller
     *             intends to write to the connection. Currently,
     *             this parameter is ignored.
     * @param timeouts indicates whether the caller
     *                 wants timeout exceptions. Currently,
     *             this parameter is ignored.
     * @return this connection
     * @exception IOException if the connection is closed or unavailable
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
        throws IOException {

        return openPrimInternal(name, mode, timeouts);
    }


    /*
     * StreamConnection Interface
     */

    /**
     * Open and return an input stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     *
     * @return                              An input stream.
     * @exception IOException               if an I/O error occurs.
     * @exception IllegalArgumentException  is thrown for all requests.
     */
    public InputStream openInputStream() throws IOException {

        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return a data input stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     *a
     * @return                              An input stream.
     * @exception IOException               if an I/O error occurs.
     * @exception IllegalArgumentException  is thrown for all requests.
     */
    public DataInputStream openDataInputStream() throws IOException {

        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return an output stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     *
     * @return                              An output stream.
     * @exception IOException               if an I/O error occurs.
     * @exception IllegalArgumentException  is thrown for all requests.
     */
    public OutputStream openOutputStream() throws IOException {

        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return a data output stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     *
     * @return                              An output stream.
     * @exception IOException               if an I/O error occurs.
     * @exception IllegalArgumentException  is thrown for all requests.
     */
    public DataOutputStream openDataOutputStream() throws IOException {

        throw new IllegalArgumentException("Not supported");
    }


    /*
     * Protocol members
     */

    /**
     * Opens a CBS connection. This is the internal entry point that
     * allows the CBS protocol handler to use the reserved port for
     * CBS emulated messages.
     *
     * @param name        The name of the connection, which as a format of
     *                    "//:portNumber" for CBS connections.
     * @param mode        Must be READ, only.
     * @param timeouts    Indicates whether the caller wants time-out
     *                    exceptions. Currently ignored.
     * @return the successfully opened connection.
     * @exception IOException if the connection is closed or unavailable.
     */
    public synchronized Connection openPrimInternal(String name,
                                                    int mode,
                                                    boolean timeouts)
                                       throws IOException {

        // The connection must not be WRITE-only. A form of READ is OK.
        if (mode == Connector.WRITE) {
            throw new IllegalArgumentException("WRITE not supported.");
        }

        // Check the I/O constraint.
        if ((mode != Connector.READ) &&
            (mode != Connector.READ_WRITE)) {
            throw new IllegalArgumentException("Invalid I/O constraint.");
        }

        /*
         * The general form of a CBS address is <code>cbs://host:port</code>.
         * CBS is receive-only, so the <code>host</code> must not be present
         * making the address form: <code>cbs://:port</code>.
         */
        if (name.charAt(0) != '/' || name.charAt(1) != '/') {
            throw new IllegalArgumentException("Missing protocol separator.");
        }

        /* Ensure no host name before extracting the port number text. */
        int colon = name.indexOf(':');
        if (colon != 2) {
            throw new IllegalArgumentException("Host not supported.");
        }
        String msgIDText = name.substring(colon + 1);

        /* Verify that the message ID is in range. */
        int msgID = 0;

        try {
            msgID = Integer.parseInt(msgIDText);
            if ((msgID > 65535) || (msgID < 0)) {
                throw new IllegalArgumentException("Message ID out of range.");
            }
        } catch (NumberFormatException nfe) {
            throw new IllegalArgumentException("Message ID formatted badly.");
        }

        /*
         * Perform a one-time check to see if the application has the permission
         * to use this connection type.
         */
        if (openPermission == false) {
            try {
                appPackage.checkForPermission(WMAPermission.CBS_SERVER);
                openPermission = true;
            } catch (InterruptedException ie) {
                throw new InterruptedIOException("Interrupted while trying " +
                                                "to ask the user permission.");
            }
        }

        /* See if the connection is already open. */
        for (int i = 0, n = openConnections.size(); i < n; i++) {
            if (((Protocol)openConnections.elementAt(i)).url.equals(name)) {
                throw new IOException("Connection already open.");
            }
        }

        /* Set up URL and port information, first. */
        url = name;
        m_imsgid = 0;
        if (msgIDText != null) {
            m_imsgid = msgID;
        }

        try {
            connHandle = open0(m_imsgid, appPackage.getId());
        } catch (IOException ioexcep) {
            m_mode = 0;
            throw new IOException("Unable to open CBS connection.");
        } catch (OutOfMemoryError oomexcep) {
            m_mode = 0;
            throw new IOException("Unable to open CBS connection.");
        }

        /* Save this connection, which is now populated with URL and port. */
        openConnections.addElement(this);

        m_mode = mode;  /* This should always be READ. */
        open = true;

        return this;
    }

    /**
     * Native finalizer
     */
    protected native void finalize();

}

