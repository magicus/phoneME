/*
 * @(#)BinaryMessage.java	1.1 03/09/02 @(#)
 *
 * Copyright © 2002-2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * 
 */

package com.sun.tck.wma;

/** 
 * An interface representing a binary message.
 * This is a subinterface of  
 * {@link Message Message} which contains methods to get and set the 
 * binary data payload. The <code>setPayloadData()</code>
 * method sets the value of the payload in the 
 * data container without any checking whether the value
 * is valid in any way.
 *  Methods for manipulating the address portion of 
 * the message are inherited from <tt>Message</tt>.
 * 
 * <p>Object instances implementing this interface are just
 * containers for the data that is passed in. 
 * </p>
 */

public interface BinaryMessage extends Message {

    /** 
     * Returns the message payload data as an array
     * of bytes.
     * 
     * <p>Returns <code>null</code>, if the payload for the message 
     * is not set.
     * </p>
     * <p>The returned byte array is a reference to the 
     * byte array of this message and the same reference
     * is returned for all calls to this method made before the
     * next call to <code>setPayloadData</code>.
     *
     * @return the payload data of this message, or
     * <code>null</code> if the data has not been set 
     * @see #setPayloadData
     */
    public byte[] getPayloadData();

    /**
     * Sets the payload data of this message. The payload may
     * be set to <code>null</code>.
     * <p>Setting the payload using this method only sets the 
     * reference to the byte array. Changes made to the contents
     * of the byte array subsequently affect the contents of this
     * <code>BinaryMessage</code> object. Therefore, applications
     * shouldn't reuse this byte array before the message is sent and the
     * <code>MessageConnection.send</code> method returns.
     * </p>
     * @param data payload data as a byte array
     * @see #getPayloadData
     */
    public void setPayloadData(byte[] data);

}


