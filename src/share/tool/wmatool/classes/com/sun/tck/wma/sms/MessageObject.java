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

package com.sun.tck.wma.sms;

import com.sun.tck.wma.Message;
import java.util.Date;

/**
 * A message for a message connection, composed of an address and data. There
 * are get and set methods for manipulating the address and data components of
 * the message. The data part can be the text, binary or multipart format. The
 * address part assumes this format:
 * <p> 
 * <code>protocol://[<em>phone_number</em>:][<em>port_number/ID</em>]</code>
 * <p>
 * and represents the address of a port/ID that can accept or receive messages. 
 *<p>
 * <code>MessageObject</code>s are instantiated when they are received from the
 * {@link com.sun.tck.wma.MessageConnection MessageConnection} or by using the
 * {@link MessageConnection#newMessage(String type)
 * MessageConnection.newMessage} message factory. Instances are freed when they
 * are garbage-collected or when they go out of scope. 
 */
public class MessageObject implements Message {

    /** High-level message type. */
    protected String msgType = null;

    /** High-level message address. */
    protected String msgAddress = null;

    /** The time stamp, indicating when the message was sent. */
    protected long sentAt = -1;

    /**
     * Creates a Message object without a buffer.
     *
     * @param type The message type: TEXT, BINARY or MULTIPART.
     * @param address The destination address of the message.
     */
    public MessageObject(String type, String address) {
	setType(type);
	setAddress(address);
    }

    /**
     * Sets the message type.
     *
     * @param type The message type: TEXT, BINARY or MULTIPART.
     *
     * @exception IllegalArgumentException if the type is not valid.
     */
    public void setType(String type) {
        if (type == null) {
            throw new IllegalArgumentException("Null message type.");
        }
        msgType = type;
    }

    /**
     * Gets the message type.
     *
     * @return The current message type or <code>null</code> if no message type
     *     has been set.
     */
    public String getType() {
        return msgType;
    }

    /**
     * Sets the address part of the message object. The address is a 
     * <code>String</code> and must be in the format:  
     * <code>protocol://<em>phone_number</em>:[<em>port</em>]</code>
     * The following code sample assigns an address to the <code>Message</code>
     * object.
     * <pre>
     *    ...
     *    String addr = "protocol://+123456789";
     *    Message msg = newMessage(MessageConnection.TEXT_MESSAGE); 
     *    msg.setAddress(addr);
     *    ...
     * </pre>
     *
     * @param address The address of the target device.
     *
     * @throws IllegalArgumentException if the address is not valid.
     *
     * @see #getAddress
     */
    public void setAddress(String address) {

        msgAddress = address;
    }

    /**
     * Gets the address from the message object as a <code>String</code>. If no
     * address is found in the message, this method returns <code>null</code>.
     * If the method is applied to an inbound message, the source address is
     * returned.  If it is applied to an outbound message, the destination
     * address is returned. 
     * <p>
     * The following code sample retrieves the address from a received message.
     * <pre>
     *    ...
     *    Message msg = conn.receive();
     *    String addr = msg.getAddress();
     *    ...
     * </pre>
     *    ...
     * @return The address in string form, or <code>null</code> if no 
     *         address was set.
     *
     * @see #setAddress
     */
    public String getAddress() {
	return msgAddress;
    }

    /**
     * Sets the timestamp for inbound SMS messages.
     *
     * @param timestamp The time stamp in the message.
     *
     * @see #getTimeStamp
     */
    public void setTimeStamp(long timestamp) {
	sentAt = timestamp;
    }

    /**
     * Returns the timestamp indicating when this message has been sent.
     * 
     * @return Date indicating the time stamp in the message or
     *         <code>null</code> if the time stamp was not set.
     *
     * @see #setTimeStamp
     */
    public Date getTimestamp() {
        return new Date(sentAt);
    }

}

