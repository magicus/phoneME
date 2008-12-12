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

package com.sun.tck.wma.cbs;

import com.sun.tck.wma.sms.MessagePacket;

import java.io.IOException;

/**
 * A CBS data packet which is the interface between an application and the data
 * stream. Methods are included to read/write data from/to the data stream, in
 * CBS format.
 */
public class CBSPacket extends MessagePacket {

    /**
     * Construct a new CBS data packet with no payload (Empty data stream.).
     */
    public CBSPacket() {
        super();
    }

    /**
     * Construct a new CBS data packet with a data stream (payload)..
     *
     * @param payload  The array of bytes representing the CBS data.
     */
    public CBSPacket(byte[] payload) {
        super(payload);
    }

    /**
     * Write the encoding type to the data stream.  The type can be text,
     * ucs2, binary.
     *
     * @param type  The encoding type.
     */
    public void setEncodingType(int type) throws IOException {
        putInt(type);
    }

    /**
     * Get the encoding type from the data stream.
     *
     * @return  The encoding type.
     */
    public int getEncodingType() {
        return getInt();
    }

    /**
     * Set the message ID, or the channel to listen to, in the data stream..
     *
     * @param msgID  The message ID.
     */
    public void setMessageID(int msgID) throws IOException {
        putInt(msgID);
    }

    /**
     * Get the message ID from the data stream..
     *
     * @return  The message ID.
     */
    public int getMessageID() {
        return getInt();
    }

    /**
     * Write the length of the message in the data stream..
     *
     * @param msgLen  The length, in bytes, of the CBS payload.
     */
    public void setMessageLength(int msgLen) throws IOException {
        putInt(msgLen);
    }

    /**
     * Get the length, in bytes, of the message from the data stream.
     *
     * @return  The length of the message.
     */
    public int getMessageLength() {
        return getInt();
    }

    /**
     * Write the message bytes to the data stream.
     *
     * @param msg  The array of bytes to write.
     */
    public void setMessage(byte[] msg) throws IOException {
        putBytes(msg);
    }

    /**
     * Get the message bytes from the data stream.
     *
     * @param len  The number of bytes to read from the stream.
     *
     * @return  The array of bytes read from the data stream.
     */
    public byte[] getMessage(int len) {
        return getBytes(len);
    }
}
