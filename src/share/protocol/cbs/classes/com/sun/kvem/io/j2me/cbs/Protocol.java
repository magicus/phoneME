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

package com.sun.kvem.io.j2me.cbs;

import javax.wireless.messaging.*;
import java.io.*;
import com.sun.midp.io.HttpUrl;
import com.sun.midp.io.j2me.sms.*;
import com.sun.kvem.io.j2me.sms.DatagramImpl;

/**
 * Network monitor implementation for CBS protocol.
 *
 */
public class Protocol extends com.sun.midp.io.j2me.cbs.Protocol {

    /** Static field for generation the unique ID. */
    private static long nextGroupID = 1;

    /** Instance of class which sends CBS data to network monitor. */
    private static DatagramImpl cbsImpl;
    
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
        if (cbsImpl == null) {
            cbsImpl = new DatagramImpl();
        }
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

        byte[] buf = null;
        Message msg = super.receive();

        if (msg instanceof TextMessage) {
            buf = ((com.sun.midp.io.j2me.cbs.TextObject)msg).getBytes();
        } else if (msg instanceof BinaryMessage) {
            buf = ((BinaryMessage)msg).getPayloadData();
        }

        cbsImpl.receive(messageRecType, msg.getAddress(), buf, getAppID(),
                numberOfSegments(msg), groupID, "cbs");

        return msg;
    }

}
