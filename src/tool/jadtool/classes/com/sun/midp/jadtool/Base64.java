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

package com.sun.midp.jadtool;

/**
 * A class to do base64 encoding and decoding.
 * 
 * Base64 is a system for representing raw byte data as Ascii 
 * characters.  Each 3 input bytes will translate into 4 
 * base64 digits.  Each base64 digit is represented by an Ascii
 * character.  This table shows how bytes are converted to
 * base64 digits.
 *
 * base64  |           |           |           |           |
 * digits  |6 5 4 3 2 1|6 5 4 3 2 1|6 5 4 3 2 1|6 5 4 3 2 1|
 *         -------------------------------------------------
 * input   |8 7 6 5 4 3 2 1|8 7 6 5 4 3 2 1|8 7 6 5 4 3 2 1|
 * bytes   |               |               |               |
 *
 * Base64 encoding always extends the input data to a multiple
 * of 24 bits by padding with zeros.
 *
 * Base64 is fully described in RFC 1521.
 * ftp://ds.internic.net/rfc/rfc1521.txt
 *
 * 
 * Published in "Java Cryptography", 1998 by
 * O'Reilly & Associates.
 * 
 */

public class Base64 {
    /**
     * encode
     *
     * coverts a byte array to a string populated with
     * base64 digits.  It steps through the byte array
     * calling a helper method for each block of three
     * input bytes
     *
     * @param raw The byte array to encode
     * @return A string in base64 encoding
     */
    public static String encode(byte[] raw) {
        StringBuffer encoded = new StringBuffer();
        for (int i = 0; i < raw.length; i += 3) {
            encoded.append(encodeBlock(raw, i));
        }
        return encoded.toString();
    }
    /*
     * encodeBlock
     *
     * creates 4 base64 digits from three bytes of input data.
     * we use an integer, block, to hold the 24 bits of input data.
     *
     * @return An array of 4 characters
     */
    protected static char[] encodeBlock(byte[] raw, int offset) {
        int block = 0;
        // how much space left in input byte array
        int slack = raw.length - offset - 1;
        // if there are fewer than 3 bytes in this block, calculate end
        int end = (slack >= 2) ? 2 : slack;
        // convert signed quantities into unsigned
        for (int i = 0; i <= end; i++) {
            byte b = raw[offset + i];
            int neuter = (b < 0) ? b + 256 : b;
            block += neuter << (8 * (2 - i));
        }

        // extract the base64 digits, which are six bit quantities.
        char[] base64 = new char[4];
        for (int i = 0; i < 4; i++) {
            int sixbit = (block >>> (6 * (3 - i))) & 0x3f;
            base64[i] = getChar(sixbit);
        }
        // pad return block if needed
        if (slack < 1) base64[2] = '=';
        if (slack < 2) base64[3] = '=';
        // always returns an array of 4 characters
        return base64;
    }
    
    /*
     * getChar
     *
     * encapsulates the translation from six bit quantity
     * to base64 digit
     */
    protected static char getChar(int sixBit) {
        if (sixBit >= 0 && sixBit <= 25)
            return (char)('A' + sixBit);
        if (sixBit >= 26 && sixBit <= 51)
            return (char)('a' + (sixBit - 26));
        if (sixBit >= 52 && sixBit <= 61)
            return (char)('0' + (sixBit - 52));
        if (sixBit == 62) 
            return '+';
        if (sixBit == 63) 
            return '/';
        return '?';
    }
    
    /**
     * decode
     * 
     * convert a base64 string into an array of bytes.
     *
     * @param base64 A String of base64 digits to decode.
     * @return A byte array containing the decoded value of
     *         the base64 input string
     */
    public static byte[] decode(String base64) {
        // how many padding digits?
        int pad = 0;
        for (int i = base64.length() - 1; base64.charAt(i) == '='; i--)
            pad++;
        // we know know the length of the target byte array.
        int length = base64.length() * 6 / 8 - pad;
        byte[] raw = new byte[length];
        int rawIndex = 0;
        // loop through the base64 value.  A correctly formed
        // base64 string always has a multiple of 4 characters.
        for (int i = 0; i < base64.length(); i += 4) {
            int block = (getValue(base64.charAt(i)) << 18)
                + (getValue(base64.charAt(i + 1)) << 12)
                + (getValue(base64.charAt(i + 2)) << 6)
                + (getValue(base64.charAt(i + 3)));
            // based on the block, the byte array is filled with the
            // appropriate 8 bit values
            for (int j = 0; j < 3 && rawIndex + j < raw.length; j++)
                raw[rawIndex + j] = (byte)((block >> (8 * (2 - j))) & 0xff);
            rawIndex += 3;
        }
        return raw;
    }
    /*
     * getValue
     *
     * translates from base64 digits to their 6 bit value
     */
    protected static int getValue(char c) {
        if (c >= 'A' && c <= 'Z') 
            return c - 'A';
        if (c >= 'a' && c <= 'z') 
            return c - 'a' + 26;
        if (c >= '0' && c <= '9') 
            return c - '0' + 52;
        if (c == '+') 
            return 62;
        if (c == '/') 
            return 63;
        if (c == '=') 
            return 0;
        return -1;
    }
}
