/*
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

package com.sun.tck.wma;

/** 
 * An interface representing a text message.
 * This is a subinterface of  
 * {@link Message Message} which
 * contains methods to get and set the text payload. The 
 * {@link #setPayloadText(String) setPayloadText} method sets the value 
 * of the payload in the data container without any checking whether the value
 * is valid in any way.
 * Methods for manipulating the address portion of 
 * the message are inherited from <tt>Message</tt>.
 * 
 * <p>Object instances implementing this interface are just
 * containers for the data that is passed in. 
 * </p>
 * 
 * <h3>Character Encoding Considerations</h3>
 * <p>Text messages using this interface
 * deal with <code>String</code>s encoded in Java.
 * The underlying implementation will convert the
 * <code>String</code>s into a suitable encoding for the messaging
 * protocol in question. Different protocols recognize different character 
 * sets. To ensure that characters are transmitted 
 * correctly across the network, an application should use the 
 * character set(s) recognized by the protocol.
 * If an application is unaware of the protocol, or uses a 
 * character set that the protocol does not recognize, then some characters 
 * might be transmitted incorrectly. 
 * </p>
 */

public interface TextMessage extends Message {

    /** 
     * Returns the message payload data as a <code>String</code>.
     *
     * @return the payload of this message, or <code>null</code> 
     * if the payload for the message is not set.
     * @see #setPayloadText
     */
    public String getPayloadText();

    /**
     * Sets the payload data of this message. The payload data 
     * may be <code>null</code>.
     * @param data payload data as a <code>String</code>
     * @see #getPayloadText
     */
    public void setPayloadText(String data);

}

