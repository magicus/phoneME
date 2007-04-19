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

package com.sun.tck.wma;

import com.sun.tck.wma.Message;
import java.util.Date;

/**
 * Implements a SMS message for the SMS message connection. This class
 * contains methods for manipulating message objects and their contents.
 * Messages can be composed of data and an address. MessageObject contains 
 * methods that can get and set the data and the address parts of a message 
 * separately. The data part can be either
 * text or binary format. The address part has the format:
 * <p> 
 * <code>sms://[<em>phone_number</em>:][<em>port_number</em>]</code>
 * <p>
 * and represents the address of a port that can accept or 
 * receive SMS messages. 
 *<p>
 * <code>MessageObject</code>s are instantiated when they are received from the
 * {@link com.sun.tck.wma.MessageConnection MessageConnection} 
 * or by using the {@link MessageConnection#newMessage(String type)
 * MessageConnection.newMessage} message factory. Instances are freed when they
 * are garbage-collected or when they go out of scope. 
 */
public abstract class MessageObject implements Message {

    /** High-level message type. */
    protected String msgType;

    /** High-level message address. */
    protected String msgAddress;

    /** The time stamp for a message that was sent. */
    protected long sentAt;

    /**
     * Creates a Message Object without a buffer.
     *
     * @param type The message type: TEXT, BINARY or MULTIPART.
     * @param  address The destination address of the message.
     */
    public MessageObject(String type, String address) {
	msgType = type;
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
     * @param addr The address of the target device.
     *
     * @throws IllegalArgumentException if the address is not valid.
     *
     * @see #getAddress
     */
    public abstract void setAddress(String addr);

    /**
     * (May be deleted) Set message address, copying the address 
     * from another message.
     *
     * @param reference the message who's address will be copied as
     * the new target address for this message.
     *
     * @exception IllegalArgumentException if the address is not valid
     *
     * @see #getAddress
     */
    public void setAddress(Message reference) {
	setAddress(reference.getAddress());
    }

    /**
     * Returns the timestamp indicating when this message has been
     * sent.  
     * 
     * @return Date indicating the timestamp in the message or
     *         <code>null</code> if the timestamp is not set.
     *
     * @see #setTimeStamp
     */
    public abstract java.util.Date getTimestamp();

    /**
     * Sets the timestamp for inbound SMS messages.
     *
     * @param timestamp  the date indicating the timestamp in the message 
     *
     * @see #getTimeStamp
     */
    public void setTimeStamp(long timestamp) {
	sentAt = timestamp;
    }

}

