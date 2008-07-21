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

package com.sun.kvem.io.j2me.sms;

/**
 * Aggreagtes data returned from receive operation from low-level
 * transport.
 */
public class TransportMessage {
    /** Target address. */
    String address;

    /** Sender's address. */
    String saddress;

    /** The type of the message. */
    String messtype;

    /** Timestamp when the message was sent. */
    long sentAt;

    /** Buffer of message data. */
    byte[] buffer = null;

    /**
     * Constructs a message return value.
     * @param addr the destination address of the message
     * @param saddr the source address of the message
     * @param type the message type
     * @param time  timestamp for the message sending time
     * @param buf data holding area for message int transport
     */
    public TransportMessage(String addr, String saddr, String type, 
		     long time, byte[] buf) {
	address = addr;
	saddress = saddr;
	messtype = type;
	sentAt = time;
	buffer = buf;
    }

    /**
     * Returns the target address.
     * @return the message target address
     */
    public String getAddress() {
	return address;
    }

    /**
     * Returns the sender's address.
     * @return the message sender's address
     */
    public String getSenderAddress() {
	return saddress;
    }

    /**
     * Returns the message type.
     * @return the message type
     */
    public String getType() {
	return messtype;
    }

    /**
     * Returns the message timestamp.
     * @return time when the message was sent
     */
    public long getTimeStamp() {
	return sentAt;
    }

    /**
     * Returns the data buffer.
     * @return the stored message buffer
     */
    public byte[] getData() {
	return buffer;
    }
}
