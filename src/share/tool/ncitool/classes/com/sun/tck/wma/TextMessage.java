/*
 * @(#)TextMessage.java	1.1 03/09/02 @(#)
 *
 * Copyright © 2002-2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * 
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

