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

import java.io.IOException;

/**
 * An SMS data packet which is the interface between an application and the data
 * stream. Methods are included to read/write data from/to the data stream, in
 * SMS format.
 */
public class SMSPacket extends MessagePacket {

    /** Length of message */
    private int msgLength = 0;

    /**
     * Constructs a new SMS data packet with no payload (Empty data stream.).
     */
    public SMSPacket() {
        super();
    }

    /**
     * Constructs a new SMS data packet with a data stream (payload)..
     *
     * @param payload  The array of bytes representing the SMS data.
     */
    public SMSPacket(byte[] payload) {
        super(payload);
    }

    /**
     * Writes the encoding type to the data stream.  The type can be text,
     * ucs2, binary.
     *
     * @param type  The encoding type.
     */
    public void setEncodingType(int type) throws IOException {
        putInt(type);
    }

    /**
     * Gets the encoding type from the data stream.
     *
     * @return The encoding type.
     */
    public int getEncodingType() {
        return getInt();
    }

    /**
     * Sets the address that is associated with the message.
     *
     * @param address  The address.
     */
    public void setAddress(String address) throws IOException {
        putString(address);
    }

    /**
     * Sets the phone number that is associated with the message.
     *
     * @param phNum  The phone number associated with the message.
     */
    public void setPhoneNumber(String phNum) throws IOException {
        putString(phNum);
    }

    /**
     * Gets the address from the data stream.
     *
     * @return The address.
     */
    public String getAddress() {
        return getString();
    }

    /**
     * Gets the phone number from the data stream.
     *
     * @return The phone number.
     */
    public String getPhoneNumber() {
        return getString();
    }

    /**
     * Sets the port that is associated with the message.
     *
     * @param port  The port.
     */
    public void setPort(int port) throws IOException {
        putInt(port);
    }

    /**
     * Gets the port from the data stream.
     *
     * @return The port.
     */
    public int getPort() {
        return getInt();
    }

    /**
     * Sets the timestamp that is associated with the message.
     *
     * @param timeStamp  The time stamp.
     */
    public void setTimeStamp(long timeStamp) throws IOException {
        putLong(timeStamp);
    }

    /**
     * Gets the time stamp from the data stream.
     *
     * @return The time stamp.
     */
    public long getTimeStamp() {
        return getLong();
    }

    /**
     * Writes the length of the message in the data stream..
     *
     * @param msgLen  The length, in bytes, of the SMS payload.
     */
    public void setMessageLength(int msgLen) throws IOException {
        msgLength = msgLen;
        putInt(msgLen);
    }

    /**
     * Gets the length, in bytes, of the message from the data stream.
     *
     * @return The length of the message.
     */
    public int getMessageLength() {
        return getInt();
    }

    /**
     * Writes the message bytes to the data stream.
     *
     * @param msg  The array of bytes to write.
     */
    public void setMessage(byte[] msg) throws IOException {
        putBytes(msg);
    }

    /**
     * Gets the message bytes from the data stream.
     *
     * @param len  The number of bytes to read from the stream.
     *
     * @return The array of bytes read from the data stream.
     */
    public byte[] getMessage(int len) {
        return getBytes(len);
    }
}
