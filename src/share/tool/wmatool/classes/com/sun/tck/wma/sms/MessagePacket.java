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

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;

import java.io.IOException;

/**
 * A generic message packet that treats the packet as an input or output stream
 * of data.
 */
public class MessagePacket {

    /** The maximum payload size for a datagram. */
    private final int DATAGRAM_PACKET_LENGTH = 1500; 

    /** The current read-write index into the data buffer (stream). */
    private int index = 0;

    /** The data buffer (stream). */
    private byte[] data;

    private boolean firstWrite = true;

    /** The byte output stream, used when creating a new packet. */
    private ByteArrayOutputStream bos = null;

    /** The data output stream, used when creating a new packet. */
    private DataOutputStream dos = null;


    /**
     * Construct a new message packet that can be populated with some data.
     */
    public MessagePacket() {
        firstWrite = true;
    }

    /**
     * Construct a new message packet that contains the given payload.
     *
     * @param payload    The bytes to be read.
     */
    public MessagePacket(byte[] payload) {

        /* Make a clone of the payload (message) bytes. */
        int length = payload.length;
        data = new byte[length];
        for (int i = 0; i < length; i++) {
            data[i] = payload[i];
        }
    }

    /**
     * Read an integer (32-bit) value from the data stream, starting at the
     * current index.
     *
     * @return  The integer that was read from the data stream.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public int getInt() {
        int x1 = ((int)(data[index++] & 0xff));
        int x2 = ((int)(data[index++] & 0xff) << 8);
        int x3 = ((int)(data[index++] & 0xff) << 16);
        int x4 = ((int)(data[index++] & 0xff) << 24);
        return (x1 | x2 | x3 | x4);
    }

    /**
     * Write an integer (32-bit) value into the data stream and advance the
     * index.
     *
     * @param x    The integer to be written.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public void putInt(int x) throws IOException {
        checkFirstWrite();
        dos.writeByte((byte) (x & 0xff));
        dos.writeByte((byte)((x >> 8) & 0xff));
        dos.writeByte((byte)((x >> 16) & 0xff));
        dos.writeByte((byte)((x >> 24) & 0xff));
    }

    /**
     * Read a short (16-bit) value from the data stream, starting at the
     * current index.
     *
     * @return  The short that was read from the data stream.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public short getShort() {
        short x1 = ((short)(data[index++] & 0xff));
        short x2 = (short)((data[index++] & 0xff) << 8);
        return (short)(x1 | x2);
    }

    /**
     * Write a short (16-bit) value into the data stream and advance the
     * index.
     *
     * @param x    The short to be written.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public void putShort(short x) throws IOException {
        checkFirstWrite();
        dos.writeByte((byte) (x & 0xff));
        dos.writeByte((byte)((x >> 8) & 0xff));
    }

    /**
     * Read a long (64-bit) value from data stream, starting at the current
     * index.
     *
     * @return  The long value that was read from the data stream.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public long getLong() {
        long x1 = ((long)(data[index++] & 0xff));
        long x2 = ((long)(data[index++] & 0xff) << 8);
        long x3 = ((long)(data[index++] & 0xff) << 16);
        long x4 = ((long)(data[index++] & 0xff) << 24);
        long x5 = ((long)(data[index++] & 0xff) << 32);
        long x6 = ((long)(data[index++] & 0xff) << 40);
        long x7 = ((long)(data[index++] & 0xff) << 48);
        long x8 = ((long)(data[index++] & 0xff) << 56);
        return (x1 | x2 | x3 | x4 | x5 | x6 | x7 | x8);
    }

    /**
     * Write a long (64-bit) value into the data stream and advance the index.
     *
     * @param x  The long to be written.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public void putLong(long x) throws IOException {
        checkFirstWrite();
        dos.writeByte((byte) (x & 0xff));
        dos.writeByte((byte)((x >> 8) & 0xff));
        dos.writeByte((byte)((x >> 16) & 0xff));
        dos.writeByte((byte)((x >> 24) & 0xff));
        dos.writeByte((byte)((x >> 32) & 0xff));
        dos.writeByte((byte)((x >> 40) & 0xff));
        dos.writeByte((byte)((x >> 48) & 0xff));
        dos.writeByte((byte)((x >> 56) & 0xff));
    }

    /**
     * Write a string to the data stream, starting at the current index. The
     * characters in the string are written, followed by a null-character
     * terminator.
     *
     * @param s  The string to be written.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public void putString(String s) throws IOException {
        checkFirstWrite();
        dos.writeBytes(s);
        dos.writeByte(0);
    }

    /**
     * Read a string from the data stream, starting at the current index. The
     * string is assumed to be terminated by a null character.
     *
     * @return  The string of characters, converted to a
     *          <code>java.lang.String</code> object.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public String getString() {
        int offset = index;
        int len = 0;

        /* Determine length of string */
        for (int i = index; i < data.length; i++) {
            if (data[i] == '\0') {
                break;
            }
            len++;
        }
             
        /* update index */
        index += len + 1;

        return new String(data, offset, len);
    }

    /**
     * Write an array of bytes into the data stream, starting at the current
     * index.
     *
     * @param buf  The buffer of bytes to be written into the data stream.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public void putBytes(byte[] buf) throws IOException {
        checkFirstWrite();
        dos.write(buf, 0, buf.length);
    }

    /**
     * Read an array of bytes from the data stream, starting at the current
     * index. The number of bytes to be read is specified by
     * <code>length</code>. The index is advanced by <code>length</code>.
     *
     * @param length  The number of bytes to be read.
     *
     * @return  The buffer of bytes that were read.
     *
     * @exception ArrayIndexOutOfBoundsException  if reading goes beyond the
     *     end of the data stream.
     */
    public byte[] getBytes(int length) {

        byte[] buffer = new byte[length];

        for (int i = 0; i < length; i++) {
            buffer[i] = data[index++];
        }

        return buffer;
    }

    /**
     * Returns the contents of the packet as a byte array.
     *
     * @return The array of packet bytes.
     *
     * @exception IOException if there was a problem while closing the streams.
     */
    public byte[] getData() throws IOException {

        byte[] buffer = new byte[0];

        if (firstWrite == false) {
            dos.close();
            buffer = bos.toByteArray();
            bos.close();
        }

        return buffer;
    }

    /**
     * Checks to see if the data output streams need to be opened.
     */
    private void checkFirstWrite() {
        if (firstWrite == true) {
            firstWrite = false;
            bos = new ByteArrayOutputStream();
            dos = new DataOutputStream(bos);
        }
    }

}
