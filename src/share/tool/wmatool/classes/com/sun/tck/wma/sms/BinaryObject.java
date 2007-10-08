/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.tck.wma.BinaryMessage;
import com.sun.tck.wma.MessageConnection;

/**
 * Implements an instance of a binary message.
 */
public class BinaryObject extends MessageObject implements BinaryMessage {

    /** Buffer that will hold the data payload. */
    private byte[] buffer;

    /**
     * Construct a binary specific message.
     * @param  addr the destination address of the message.
     */
    public BinaryObject(String addr) {
	super(MessageConnection.BINARY_MESSAGE, addr);
    }

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
    public byte[] getPayloadData() {
	return buffer;
    }

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
    public void setPayloadData(byte[] data) {
	buffer = data;
    }

}


