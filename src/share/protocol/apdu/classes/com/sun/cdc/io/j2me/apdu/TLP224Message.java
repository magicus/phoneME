/*
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

package com.sun.cdc.io.j2me.apdu;

/**
 * The TLP224Message class represents the link level message between a CAD
 * (Card Acceptance Device) and a Terminal. All messages exchanged between 
 * the CAD and the Terminal are encoded using the TLP224 protocol. 
 * TLP224Messages are used internally to maintain context on the server 
 * side and to send commands to and from the client.
 */
class TLP224Message extends Exception {

    /**
     * This field contains the EOT code which is always sent as the last 
     * octet of any TLP224 message.
     */
    static final byte EOT = 0x03;
    
    /**
     * This field contains the ACK code which is returned to the sender as 
     * the first octet of a TLP224 response when a message has been 
     * successfully received.
     */
    static final byte ACK = 0x60;
    
    /**
     * This field contains the NACK code which is returned to the sender as 
     * the first octet of a TLP224 response when a transmission error occurs.
     */
    static final byte NACK = (byte) 0xe0;
    
    /**
     * This field contains the TLP224 command to power up the CAD.
     */
    static final byte POWER_UP = 0x6e;
    
    /**
     * This field contains the TLP224 command to send data to the card.
     */
    static final byte ISO_INPUT = (byte) 0xda;
    
    /**
     * This field contains the TLP224 command to read data from the card.
     */
    static final byte ISO_OUTPUT = (byte) 0xdb;
    
    /**
     * This status code is returned by both the Client and Server CAD's when
     * a command has been successfully executed.
     */
    static final byte STATUS_SUCCESS = 0;
    
    /**
     * This status code is returned by both the Client and Server CAD's when 
     * the first byte of a received message is neither an ACK or NACK.
     */
    static final byte STATUS_PROTOCOL_ERROR = 0x09;
    
    /**
     * This status code is returned by both the Client and Server CAD's 
     * when a received message exceeds the length of the internal buffers.
     */
    static final byte STATUS_MESSAGE_TOO_LONG = 0x12;
    
    /**
     * This status code is returned by the reader or ServerCad if the card
     * sends a Procedure Byte which aborts a ISO_IN or ISO_OUT command.
     */
    static final byte STATUS_INTERRUPTED_EXCHANGE = (byte) 0xe5;
    
    /**
     * This status code is returned by the reader or ServerCad
     * if SW1 SW2 are not equal to 0x9000.
     */
    static final byte STATUS_CARD_ERROR = (byte) 0xe7;
    
    /**
     * This status code is returned by the reader or ServerCad if the card was
     * removed between exchanges.
     */
    static final byte STATUS_CARD_REMOVED = (byte) 0xf7;
    
    /**
     * This status code is returned by the reader or ServerCad if there is
     * no card in the reader.
     */
    static final byte STATUS_CARD_MISSING = (byte) 0xfb;
    
    /**
     * This field contains the size of the largest possible TLP224 Message.
     * This message would be &lt;ACK&gt;&lt;LEN&gt;&lt;255 bytes of 
     * command&gt;&lt;LRC&gt;
     */
    private static final int MAX_MESSAGE_LEN = 264;
    
    /**
     * Local buffer for this TLP224 message.
     */
    private byte[] buf;
    
    /**
     * Current length of data in local buffer.
     */
    private int len;

    /**
     * Construct a new TLP224Message using the default (MAX_MESSAGE_LEN) 
     * message size.
     */
    TLP224Message() {
        this.buf = new byte[MAX_MESSAGE_LEN];
        this.len = 0;
    }

    /**
     * Retrieves the contents of this TLP224Message.
     * @return The non-null byte array containing the data to be sent or 
     * received. Any changes
     * to this byte array also affect the TLP224Message from which it came.
     */
    byte[] getData() { return buf; }

    /**
     * Retrieves the length of this TLP224Message.
     * @return The length in bytes of the internal buffer as previously set 
     * by setLength().
     */
    int getLength() { return len; }

    /**
     * Set the length of the data in this TLP224Message.
     * The length must be less than or equal to the length of the internal 
     * buffer.
     * @param newLen The length to set.
     * @exception IllegalArgumentException if newLen is greater than the 
     * length of the internal buffer.
     */
    void setLength(int newLen) {
        this.len = newLen;
    }
    
    /**
     * Compute the TLP224 LRC of this object.
     * The TLP224 LRC is the exclusive-or of all the bytes in the message.
     * @param length The number of bytes to compute the LRC over.
     * @return The computed LRC.
     */
    byte computeLRC(int length) {
        int lrc = 0;
        for (int i = 0; i < length; i++)
            lrc ^= buf[i];
        return (byte) lrc;
    }
}
