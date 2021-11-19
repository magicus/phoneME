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

import java.io.IOException;

/** 
 * A connection handler for generic message receiving and sending.
 */ 
public interface MessageConnection {

    /**
     * Constant designating a message type for <strong>text</strong>
     * messages, i.e. value = "text".
     * When this constant is used for the <code>newMessage()</code> methods
     * <code>type</code> parameter, the new <code>Message</code> object
     * will implement the <code>TextMessage</code> interface.
     */
    public static final String TEXT_MESSAGE = "text";

    /**
     * Constant designating a message type for <strong>binary</strong>
     * messages, i.e. value = "binary".
     * When this constant is used for the <code>newMessage()</code> methods
     * <code>type</code> parameter, the new <code>Message</code> object
     * will implement the <code>BinaryMessage</code> interface.
     */
    public static final String BINARY_MESSAGE = "binary";

    /**
     * Constant designating a message type for <strong>multipart MIME</strong>
     * messages, i.e. value = "multipart".
     * When this constant is used for the <code>newMessage()</code> methods
     * <code>type</code> parameter, the new <code>Message</code> object
     * will implement the <code>MultipartMessage</code> interface.
     * @since WMA 2.0
     */
    public static final String MULTIPART_MESSAGE = "multipart";

    /**
     * Constructs a new message object of a specified type.
     *
     * @param type the type of message to create.
     * @throws java.lang.IllegalArgumentException if the message
     *         type isn't <code>BINARY_MESSAGE</code> or
     *         <code>TEXT_MESSAGE</code>
     * @return a newly constructed message object of a given type
     */
    public Message newMessage(String type);   

    /** 
     * Constructs a new message object of a specified type
     * initialized with the specified destination address.
     *
     * @param type the type of message to create.
     * @param address the new message's destination address
     * @return a newly constructed message object of a given type
     * @throws java.lang.IllegalArgumentException if the message
     *         type isn't <code>BINARY_MESSAGE</code> or
     *         <code>TEXT_MESSAGE</code>
     * @see #newMessage(String type)
     */
    public Message newMessage(String type, String address);

    /**
     * Sends a message.
     * 
     * @param msg the message to be sent
     * @throws java.io.IOException network failure has occured, or the message
     *          couldn't be sent due to other reasons
     * @throws java.lang.IllegalArgumentException if the message contains
     *         invalid information or is incomplete, or the payload
     *         exceeds the maximal length allowed by the given messaging
     *         protocol.
     * @throws java.io.InterruptedIOException if the connection is closed during
     *         the send operation, or if a timeout occurs while sending
     *         the message
     * @throws java.lang.NullPointerException if <code>msg</code> is null
     * @throws java.lang.SecurityException if the application doesn't
     *         have permission for sending the message
     * @see #receive()
     */
    public void send(Message msg) 
	throws java.io.IOException, java.io.InterruptedIOException;

    /** 
     * Receives a message.
     * 
     * <p>If there are no <code>Message</code>s for this 
     * <code>MessageConnection</code> waiting,
     * this method will block until a message for this <code>Connection</code>
     * is received, or the <code>MessageConnection</code> is closed.
     * </p>
     * @return a received message represented by the <code>Message</code> object
     * @throws java.io.IOException if an error occurs while receiving
     *         a message
     * @throws java.io.InterruptedIOException if the connection is closed during
     *         the receive operation
     * @throws java.lang.SecurityException if the application doesn't
     *         have permission for receiving messages on the given port
     * @see #send(Message)
     */
    public Message receive()
	throws java.io.IOException, java.io.InterruptedIOException;

    /** 
     * Close the connection.
     * @throws java.io.IOException if an I/O error occurs
     */
    public void close()
        throws IOException;
}



