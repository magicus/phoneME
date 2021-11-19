/*
 *
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
 */

package com.sun.kvem.io.j2me.sms;

import javax.wireless.messaging.*;
import java.io.*;
import com.sun.midp.io.j2me.sms.*;

/**
 * Network monitor implementation for SMS protocol.
 *
 */
public class Protocol extends com.sun.midp.io.j2me.sms.Protocol {

    /** Static field for generation the unique ID. */
    private static long nextGroupID = 1;

    /** Instance of class which sends SMS data to network monitor. */
    private static DatagramImpl smsImpl;

    /** Network monitor session ID. */
    private long groupID;
    
    /**
     * Initialization.
     *
     * The new network session ID is assigned.
     */
    public Protocol() {
        super();
        groupID = nextGroupID++;
        if (smsImpl == null) {
            smsImpl = new DatagramImpl();
        }
    }


    /**
     * Sends a message over the connection. This method extracts the data
     * payload from the <code>Message</code> object so that it can be sent as a
     * datagram.
     *
     * @param     dmsg a <code>Message</code> object
     * @exception java.io.IOException if the message could not be sent or
     *     because of network failure
     * @exception java.lang.IllegalArgumentException if the message is
     *     incomplete or contains invalid information. This exception is also
     *     thrown if the payload of the message exceeds the maximum length for
     *     the given messaging protocol.
     * @exception java.io.InterruptedIOException if a timeout occurs while
     *     either trying to send the message or if this <code>Connection</code>
     *     object is closed during this <code>send</code> operation.
     * @exception java.lang.NullPointerException if the parameter is
     *     <code>null</code>.
     * @exception java.lang.SecurityException if the application does not have
     *      permission to send the message.
     */
    public void send(Message dmsg) throws IOException {

        byte[] buf = null;

        super.send(dmsg);

        ensureOpen();
        if (dmsg instanceof TextMessage) {
            buf = ((com.sun.midp.io.j2me.sms.TextObject)dmsg).getBytes();
        } else if (dmsg instanceof BinaryMessage) {
            buf = ((BinaryMessage)dmsg).getPayloadData();
        }


        smsImpl.send(dmsg.getAddress(), buf,
                                 (getMsgHost(dmsg) == null ? Integer.toString(getMsgPort(dmsg)) : null),
                                 messageSendType, numberOfSegments(dmsg), groupID);

    }

    /**
     * Receives the bytes that have been sent over the connection, constructs a
     * <code>Message</code> object, and returns it.
     * <p>
     * If there are no <code>Message</code>s waiting on the connection, this
     * method will block until a message is received, or the
     * <code>MessageConnection</code> is closed.
     *
     * @return a <code>Message</code> object.
     * @exception java.io.IOException if an error occurs while receiving a
     *     message.
     * @exception java.io.InterruptedIOException if this
     *     <code>MessageConnection</code> object is closed during this receive
     *     method call.
     * @exception java.lang.SecurityException if the application does not have
     *      permission to receive messages using the given port number.
     */
    public synchronized Message receive()
        throws IOException {

        byte[] buf = null;
        Message msg = super.receive();

        if (msg instanceof TextMessage) {
            buf = ((com.sun.midp.io.j2me.sms.TextObject)msg).getBytes();
        } else if (msg instanceof BinaryMessage) {
            buf = ((BinaryMessage)msg).getPayloadData();
        }

        long sendTime = smsImpl.receive(messageRecType, msg.getAddress(), buf, getAppID(),
                                        numberOfSegments(msg), groupID, "sms");

        return msg;
    }
 
}
