/*
 *   
 *
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

package com.sun.cdc.i18n.j2me;

import java.io.*;


/** Reader for UTF-16 encoded input streams. */
public class UTF_16_Reader extends com.sun.cdc.i18n.StreamReader {

    /** the first byte of a pair of bytes that represent a 16-bit char */
    protected int firstByte = -1;
    /** the byteOrder variable has this value when the byte order
     * has not yet been specified or detected */
    protected static final int UNKNOWN_BYTE_ORDER = 0;
    /** the byteOrder variable has this value when the byte order
     * is Big Endian */
    protected static final int BIG_ENDIAN = 1;
    /** the byteOrder variable has this value when the byte order
     * is Little Endian */
    protected static final int LITTLE_ENDIAN = 2;
    /** the byte order: one of BIG_ENDIAN, LITTLE_ENDIAN, UNKNOWN_BYTE_ORDER */
    protected int byteOrder = UNKNOWN_BYTE_ORDER;

    /** Constructs a UTF-16 reader. */
    public UTF_16_Reader() {
    }

    /**
     * Open the reader
     * @param in the input stream to be read
     * @param enc identifies the encoding to be used
     * @return a reader for the given input stream and encoding
     * @throws UnsupportedEncodingException
     */
    public Reader open(InputStream in, String enc)
        throws UnsupportedEncodingException {
        firstByte = -1;
        byteOrder = UNKNOWN_BYTE_ORDER;
        super.open(in, enc);
        return this;
    }

    /** Convert two bytes to a 16-bit char
     * assuming the big endian byte order.
     * @param firstByte the first of two bytes representing a char
     * @param secondByte the second of two bytes representing a char
     * @return the character represented by the two bytes
     */
    protected char mergeBytesBigEndian(int firstByte, int secondByte) {
        return (char) ((firstByte << 8) + secondByte);
    }
    /** Convert two bytes to a 16-bit char
     * assuming the little endian byte order.
     * @param firstByte the first of two bytes representing a char
     * @param secondByte the second of two bytes representing a char
     * @return the character represented by the two bytes
     */
    protected char mergeBytesLittleEndian(int firstByte, int secondByte) {
        return (char) ((secondByte << 8) + firstByte);
    }
    /**
     * If the two argument bytes represent a Byte Order Mark (BOM),
     * return the corresponding byte order constant; else return the
     * value corresponding to the default byte order.
     * @param firstByte the first of two bytes representing a char or BOM
     * @param secondByte the second of two bytes representing a char or BOM
     * @return either BIG_ENDIAN or LITTLE_ENDIAN
     */
    protected int bomDetect(int firstByte, int secondByte) {
        if (firstByte == 0xFE && secondByte == 0xFF) {
            return BIG_ENDIAN;
        } else if (firstByte == 0xFF && secondByte == 0xFE) {
            return LITTLE_ENDIAN;
        } else { // default
            // TODO: make a platform call or a fetch from XML configuration
            return BIG_ENDIAN;
        }
    }

    /**
     * Read a block of UTF16 characters.
     *
     * @param cbuf output buffer for converted characters read
     * @param off initial offset into the provided buffer
     * @param len length of characters in the buffer
     * @return the number of converted characters
     * @exception IOException is thrown if the input stream 
     * could not be read for the raw unconverted character
     */
    public int read(char cbuf[], int off, int len) throws IOException {
        int count = 0;
        int secondByte;
        if (len == 0) {
            return 0;
        }

        if (firstByte == -1) {
            firstByte = in.read();
        }
        for ( ; count < len; firstByte = in.read()) {
            if( -1 == firstByte || -1 == (secondByte = in.read())) {
                return (0 == count) ? -1 : count;
            }

            // only for the first two bytes: examine BOM
            if (byteOrder == UNKNOWN_BYTE_ORDER) {
                byteOrder = bomDetect(firstByte,secondByte);
            }

            if (byteOrder == BIG_ENDIAN) {
                cbuf[off + count] =
                        mergeBytesBigEndian(firstByte,secondByte);
            } else { // if (byteOrder == LITTLE_ENDIAN)
                cbuf[off + count] =
                        mergeBytesLittleEndian(firstByte,secondByte);
            }
            count++;
        }
        return count;
    }

    /**
     * Tell whether this reader supports the mark() operation.
     * The implementation always returns false because it does not
     * support mark().
     *
     * @return false
     */
    public boolean markSupported() {
        /*
         * For readers mark() is in characters; UTF-16 is easier than UTF-8,
         * but it's not supported yet.
         * So this reader does not support mark at this time.
         */
        return false;
    }

    /**
     * Mark a read ahead character is not supported for UTF16
     * readers.
     * @param readAheadLimit number of characters to buffer ahead
     * @exception IOException is thrown, for all calls to this method
     * because marking is not supported for UTF16 readers
     */
    public void mark(int readAheadLimit) throws IOException {
        throw new IOException("mark() not supported");
    }

    /**
     * Reset the read ahead marks is not supported for UTF16 readers.
     * @exception IOException is thrown, for all calls to this method
     * because marking is not supported for UTF16 readers
     */
    public void reset() throws IOException {
        throw new IOException("reset() not supported");
    }

    /**
     * Get the size in chars of an array of bytes.
     *
     * @param      array  Source buffer
     * @param      offset Offset at which to start counting characters
     * @param      length number of bytes to use for counting
     *
     * @return     number of characters that would be converted
     */
    /*
     * This method is only used by our internal Helper class in the method
     * byteToCharArray to know how much to allocate before using a
     * reader. If we encounter bad encoding we should return a count
     * that includes that character so the reader will throw an IOException
     */
    public int sizeOf(byte[] array, int offset, int length) {
        return length/2;
    }
}
