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

import com.sun.tck.wma.TextMessage;
import com.sun.tck.wma.MessageConnection;
import com.sun.midp.io.j2me.sms.TextEncoder;

/**
 * Implements an instance of a text message.
 */
public class TextObject extends MessageObject implements TextMessage {

    /** Buffer that will hold the text payload. */
    private byte[] buffer;

    /**
     * Construct a text specific message.
     * @param  addr the destination address of the message.
     */
    public TextObject(String addr) {
        super(MessageConnection.TEXT_MESSAGE, addr);
    }

    /**
     * Returns a <code>String</code> representing the message payload data.
     *
     * @return <code>null</code> if the payload isn't set, or
     *         this message's payload
     * @see #setPayloadText
     */
    public String getPayloadText() {
        if (buffer == null) {
            return null;
        }
        return TextEncoder.toString(buffer);
    }

    /**
     * Sets this message's payload data. It may be <code>null</code>.
     * @param data a <code>String</code> representing the payload data
     * @see #getPayloadText
     */
    public void setPayloadText(String data) {
        if (data != null) {
            buffer = TextEncoder.toByteArray(data);
        } else {
            buffer = null;
        }
    }

    /**
     * Gets the raw byte array.
     * @return an array of raw UCS-2 payload data
     * @see #getPayloadText
     * @see #setBytes
     */
    public byte[] getPayloadData() {
        return buffer;
    }

    /**
     * Sets the raw byte array.
     * @param data an array of raw UCS-2 payload data.
     * @see #getBytes
     */
    public void setPayloadData(byte[] data) {
        buffer = data;
    }

}


