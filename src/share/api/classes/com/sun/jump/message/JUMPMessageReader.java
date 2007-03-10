/*
 * %W% %E%
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

package com.sun.jump.message;

import java.io.IOException;

/**
 * <code>JUMPMessageReader</code> is used to read data from a message.
 */
public class JUMPMessageReader {
    private final byte[] messageDataBytes;
    private final int messageMarkOffset;
    int messageDataOffset;  // Package private access for JUMPMessage

    /**
     * Creates a new instance of JUMPMessageReader for message m
     */
    public JUMPMessageReader(JUMPMessage m) {
	this(m, m.messageUserDataOffset);
    }
    
    /**
     * Package private constructor to set initial offset to
     * start reading from.
     */
    JUMPMessageReader(JUMPMessage m, int offset) {
	this.messageDataBytes    = m.messageDataBytes;
	// Context starts from user data offset, past the header
	this.messageMarkOffset   = offset;
	this.messageDataOffset   = offset;
    }
    
    public int getInt() {
	// Get int at messageDataOffset
	int b1 = messageDataBytes[messageDataOffset]   & 0xff;
	int b2 = messageDataBytes[messageDataOffset+1] & 0xff;
	int b3 = messageDataBytes[messageDataOffset+2] & 0xff;
	int b4 = messageDataBytes[messageDataOffset+3] & 0xff;
	int i = (b1 << 24) + (b2 << 16) + (b3 << 8) + b4;
	if (false) {
	    System.err.println("Decoded int "+i+" from ["+
			       b1+", "+b2+", "+b3+", "+b4+"]");
	}
	messageDataOffset += 4;
	return i;
    }

    public long getLong() {
        long l = (((long)messageDataBytes[messageDataOffset+0] << 56) +
		  ((long)(messageDataBytes[messageDataOffset+1] & 255) << 48) +
		  ((long)(messageDataBytes[messageDataOffset+2] & 255) << 40) +
		  ((long)(messageDataBytes[messageDataOffset+3] & 255) << 32) +
		  ((long)(messageDataBytes[messageDataOffset+4] & 255) << 24) +
		  ((messageDataBytes[messageDataOffset+5] & 255) << 16) +
		  ((messageDataBytes[messageDataOffset+6] & 255) <<  8) +
		  ((messageDataBytes[messageDataOffset+7] & 255) <<  0));
	messageDataOffset += 8;
	return l;
    }

    public byte getByte() {
	byte b = messageDataBytes[messageDataOffset];
	messageDataOffset += 1;
	return b;
    }

    private char[] fromBytes(byte[] ascii) 
    {
	int n = ascii.length;
        char[] value = new char[n-1]; // Leave out the null termination

	for (int i = 0; i < n-1; i++) {
	    value[i] = (char) (ascii[i] & 0xff);
	}
	return value;
    }
    
    public String getUTF() {
	// FIXME: GEt string in Java-modified utf-8
	// Re-use implementation in DataInputStream.readUTF().
	return new String(fromBytes(getByteArray()));
    }

    public byte[] getByteArray() {
	int len = getInt();
	if (len == -1) {
	    return null;
	}
	byte[] b = new byte[len];
	System.arraycopy(messageDataBytes, messageDataOffset, b, 0, len);
	messageDataOffset += len;
	return b;
    }

    public String[] getUTFArray() {
	int len = getInt();
	if (len == -1) {
	    return null;
	}
	String[] s = new String[len];
	for (int i = 0; i < len; i++) {
	    s[i] = getUTF();
	}
	return s;
    }

    public void reset() {
	messageDataOffset = messageMarkOffset;
    }

}
