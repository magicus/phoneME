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
import java.io.IOException;

/** 
 * A connection handler for generic message sending and receiving.
 */ 

public interface MessageConnection {

    /**
     * Constant for a message type for <strong>text</strong> 
     * messages (value = "text").
     * If this constant is used for the <tt>type</tt> parameter in the 
     * <code>newMessage()</code> 
     * methods, then the newly created <code>Message</code>
     * will be an instance
     * implementing the <code>TextMessage</code> interface.
     */
    public static final String TEXT_MESSAGE = "text";

    /**
     * Constant for a message type for <strong>binary</strong>
     *  messages (value = "binary").
     * If this constant is used for the <tt>type</tt> parameter in the 
     * <code>newMessage()</code> 
     * methods, then the newly created <code>Message</code>
     *  will be an instance
     * implementing the <code>BinaryMessage</code> interface.
     */
    public static final String BINARY_MESSAGE = "binary";

    /**
     * Constant for a message type for <strong>multipart MIME</strong>
     *  messages (value = "multipart").
     * Using this constant as the <code>type</code> parameter in the
     * <code>newMessage()</code> methods will cause the newly created
     * <code>Message</code> to be an instance implementing the 
     * <code>MultipartMessage</code> interface.
     * @since WMA 2.0
     */
    public static final String MULTIPART_MESSAGE = "multipart";


    /**
     * Constructs a new message object of a given type. When the 
     * string <code>text</code> is passed in, the created 
     * object implements the <code>TextMessage</code> interface.
     * When the <code>binary</code> constant is passed in, the 
     * created object implements the <code>BinaryMessage</code> 
     * interface. Adapter definitions for messaging protocols can define
     * new constants and new subinterfaces for the <code>Message</code>s.
     * The type strings are case-sensitive.
     *
     * <p>For adapter definitions that are not defined within the JCP 
     * process, the strings used <strong>must</strong> begin with 
     * an inverted domain 
     * name controlled by the defining organization, as is
     * used for Java package names. Strings that do not contain a 
     * full stop character "." are reserved for specifications done 
     * within the JCP process and <strong>must not</strong> be used by 
     * other organizations
     * defining adapter specification.
     * </p> 
     * <p>When this method is called from a <em>client</em> mode connection,
     * the newly created <code>Message</code> has the destination address
     * set to the address identified when this <code>Connection</code> 
     * was created.
     * </p>
     * <p>When this method is called from a <em>server</em> mode connection,
     * the newly created Message does not have the destination
     * address set. It must be set by the application before
     * trying to send the message.
     * </p>
     * @param type the type of message to be created. There are 
     *             constants for basic types defined in 
     *             this interface
     * @throws java.lang.IllegalArgumentException if the message
     *         type is not <code>TEXT_MESSAGE</code> or
     *         <code>BINARY_MESSAGE</code>
     * @return Message object for a given type of message
     */
    public Message newMessage(String type);   

    /** 
     * Constructs a new message object of a given type and
     * initializes it with the given destination address.
     * The semantics related to the parameter <code>type</code>
     * are the same as for the method signature with just the
     * <code>type</code> parameter.
     * 
     * @param type the type of message to be created. There are 
     *             constants for basic types defined in 
     *             this interface.
     * @param address destination address for the new message
     * @return <code>Message</code> object for a given type of message
     * @throws java.lang.IllegalArgumentException if the message
     *         type is not <code>TEXT_MESSAGE</code> or
     *         <code>BINARY_MESSAGE</code>
     * @see #newMessage(String type)
     */
    public Message newMessage(String type, String address);

    /**
     * Sends a message.
     * 
     * @param msg the message to be sent
     * @throws java.io.IOException if the message could not be sent
     *          or because of network failure
     * @throws java.lang.IllegalArgumentException if the message is
     *         incomplete or contains invalid information
     *         This exception
     *         is also thrown if the payload of the message exceeds 
     *         the maximum length for the given messaging protocol.
     * @throws java.io.InterruptedIOException if a timeout occurs while
     *         trying to send the message or if this <code>Connection</code> 
     *         object is closed during this send operation
     * @throws java.lang.NullPointerException if the parameter is null
     * @throws java.lang.SecurityException if the application does not 
     *         have permission to send the message
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
     * @return a <code>Message</code> object representing the 
     *         information in the received message
     * @throws java.io.IOException if an error occurs while receiving
     *         a message
     * @throws java.io.InterruptedIOException if this 
     *         <code>MessageConnection</code> object
     *         is closed during this receive method call
     * @throws java.lang.SecurityException if the application does not 
     *         have permission to receive messages using the given port
     *         number
     * @see #send(Message)
     */
    public Message receive()
	throws java.io.IOException, java.io.InterruptedIOException;

    /** 
     * Close the connection.
     * When the connection has been closed access to all methods except this one
     * will cause an an IOException to be thrown. Closing an already closed
     * connection has no effect. Streams derived from the connection may be open
     * when method is called. Any open streams will cause the
     * connection to be held open until they themselves are closed.
     * @throws java.io.IOException - If an I/O error occurs
     */

    public void close() 
        throws IOException;
}



