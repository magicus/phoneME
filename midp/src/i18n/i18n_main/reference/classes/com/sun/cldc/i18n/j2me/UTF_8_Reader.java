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

package com.sun.cldc.i18n.j2me;

import java.io.*;

/** Reader for UTF-8 encoded input streams. */
public class UTF_8_Reader extends com.sun.cldc.i18n.StreamReader {
    /** signals that no byte is available, but not the end of stream */
    private static final int NO_BYTE = -2;
    /** 'replacement character' [Unicode 1.1.0] */ 
    private static final int RC = 0xFFFD; 
    /** read ahead buffer that to holds part of char from the last read */
    private int[] readAhead;
    /** when reading first of a char byte we need to know if the first read */
    private boolean newRead;

    /** Constructs a UTF-8 reader. */
    public UTF_8_Reader() {
        readAhead = new int[3];
    }

    public Reader open(InputStream in, String enc)
        throws UnsupportedEncodingException {
        super.open(in, enc);
        prepareForNextChar(NO_BYTE);
        return this;
    }

    /**
     * Read a block of UTF8 characters.
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
        int firstByte;
        int extraBytes;
        int currentChar = 0;
        int nextByte;
        int headByte = NO_BYTE;
        
        if (len == 0) {
            return 0;
        }

        newRead = true;
        while (count < len) {
            firstByte = getByteOfCurrentChar(0);
            if (firstByte < 0) {
                if (firstByte == -1 && count == 0) {
                    // end of stream
                    return -1;
                }

                return count;
            }
            /* Let's reduce amount of case-mode comparisons */
            if ((firstByte&0x80) == 0) {
                extraBytes = 0;
                currentChar = firstByte;
            } else {
                switch (firstByte >> 4) {
                case 12: case 13:
                    /* 11 bits: 110x xxxx   10xx xxxx */
                    extraBytes = 1;
                    currentChar = firstByte & 0x1F;
                    break;
    
                case 14:
                    /* 16 bits: 1110 xxxx  10xx xxxx  10xx xxxx */
                    extraBytes = 2;
                    currentChar = firstByte & 0x0F;
                    break;
    
                default:
                    /* we do replace malformed character with special symbol */
                    extraBytes = 0;
                    currentChar = RC;
                }
            }

            for (int j = 1; j <= extraBytes; j++) {
                nextByte = getByteOfCurrentChar(j);
                if (nextByte == NO_BYTE) {
                    // done for now, comeback later for the rest of char
                    return count;
                }

                if (nextByte == -1) {
                    // end of stream in the middle of char -- set 'RC'
                    currentChar = RC;
                    break;
                }

                if ((nextByte & 0xC0) != 0x80) {
                    // invalid byte - move it at head of next read sequence
                    currentChar = RC;
                    headByte = nextByte;
                    break;
                }

                // each extra byte has 6 bits more of the char
                currentChar = (currentChar << 6) + (nextByte & 0x3F);
            }

            cbuf[off + count] = (char)currentChar;
            count++;
            prepareForNextChar(headByte);
        }

        return count;
    }

    /**
     * Get one of the raw bytes for the current character to be converted
     * from look ahead buffer.
     *
     * @param byteOfChar which raw byte to get 0 for the first, 2 for the last
     *
     * @return a byte value, NO_BYTE for no byte available or -1 for end of
     *          stream
     *
     * @exception  IOException   if an I/O error occurs.
     */
    private int getByteOfCurrentChar(int byteOfChar) throws IOException {
        if (readAhead[byteOfChar] != NO_BYTE) {
            return readAhead[byteOfChar];
        }

        /*
         * Our read method must block until it gets one char so don't call
         * available on the first real stream for each new read().
         */
        if (!newRead && in.available() <= 0) {
            return NO_BYTE;
        }

        readAhead[byteOfChar] = in.read();

        /*
         * since we have read from the input stream,
         * this not a new read any more
         */
        newRead = false;

        return readAhead[byteOfChar];
    }

    /**
     * Prepare the reader for the next character by clearing the look
     * ahead buffer.
     * @param headByte value of first byte. If previous sequence is interrupted
     * by malformed byte - this byte should be moved at head of next sequence
     */
    private void prepareForNextChar(int headByte) {
        readAhead[0] = headByte;
        readAhead[1] = NO_BYTE;
        readAhead[2] = NO_BYTE;
    }

    /**
     * Tell whether this reader supports the mark() operation.
     * The UTF-8 implementation always returns false because it does not
     * support mark().
     *
     * @return false
     */
    public boolean markSupported() {
        /*
         * For readers mark() is in characters, since UTF-8 character are
         * variable length, so we can't just forward this to the underlying
         * byte InputStream like other readers do.
         * So this reader does not support mark at this time.
         */
        return false;
    }

    /**
     * Mark a read ahead character is not supported for UTF8
     * readers.
     * @param readAheadLimit number of characters to buffer ahead
     * @exception IOException is thrown, for all calls to this method
     * because marking is not supported for UTF8 readers
     */
    public void mark(int readAheadLimit) throws IOException {
        throw new IOException("mark() not supported");
    }

    /**
     * Reset the read ahead marks is not supported for UTF8 readers.
     * @exception IOException is thrown, for all calls to this method
     * because marking is not supported for UTF8 readers
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
        int count = 0;
        int endOfArray;
        int extraBytes;

        for (endOfArray = offset + length; offset < endOfArray; ) {
            count++;
            /* Reduce amount of case-mode comparisons */
            if ((array[offset]&0x80) == 0) {
                extraBytes = 0;
            } else {
                switch (((int)array[offset] & 0xff) >> 4) {
                case 12: case 13:
                    /* 110x xxxx   10xx xxxx */
                    extraBytes = 1;
                    break;
    
                case 14:
                    /* 1110 xxxx  10xx xxxx  10xx xxxx */
                    extraBytes = 2;
                    break;
    
                default:
                    /*
                     * we do not support characters greater than 16 bits
                     * this byte will be replaced with 'RC'
                     */
                    extraBytes = 0;
                }
            }
            offset++;
            // test if extra bytes are in form 10xx xxxx
            while (extraBytes-- > 0){
                if (offset < endOfArray) {
                    if ((((int)array[offset]) & 0xC0) != 0x80) {
                        break;  // test fails: char will be replaced with 'RC'
                    } else {
                        offset++;
                    }
                } else {
                    count--;    // broken sequence detected at tail of array
                    break;
                }
            }
        }

        return count;
    }
}
