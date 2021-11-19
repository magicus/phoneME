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

package acl_data;

import java.security.MessageDigest;

import java.io.ByteArrayOutputStream;
import java.io.UnsupportedEncodingException;

import java.util.*;
import java.io.PrintStream;

/**
 * This class implements miscellaneous utility methods including
 * those used for conversion of BigIntegers to
 * byte arrays, hexadecimal printing of byte arrays etc.
 */
public class Utils {

    /** UTF-8 encoding name. */
    public static final String utf8 = "UTF-8";

    /** Hexadecimal digits. */
    private static char[] hc = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    /**
     * Returns hex value for given sequence of bytes.
     * @param b source data
     * @param off offset of the first byte
     * @param len length of the value
     * @return hex value for given sequence of bytes.
     */
    public static String hexNumber(byte[] b, int off, int len) {

        char[] r;
        int v;
        int i;
        int j;

        if ((b == null) || (len == 0)) {
            return "";
        }

        if ((off < 0) || (len < 0)) {
            throw new ArrayIndexOutOfBoundsException();
        }

        r = new char[len * 2];

        for (i = 0, j = 0; ; ) {
            v = b[off + i] & 0xff;
            r[j++] = hc[v >>> 4];
            r[j++] = hc[v & 0x0f];

            i++;
            if (i >= len) {
                break;
            }
        }

        return (new String(r, 0, j));
    }

    /**
     * Returns hex value for given byte.
     * @param b source data
     * @return hex value.
     */
    public static String hexByte(int b) {
        b = b & 0xff;
        return new String(new char[] {hc[b >>> 4],  hc[b & 0x0f]});
    }

    /**
     * Checks if two byte arrays match.
     * @param a the first byte array
     * @param aOff starting offset for comparison within a
     * @param aLen length of data in the first array
     * @param b the second byte array
     * @param bOff starting offset for comparison within b
     * @param bLen length of data in the second array
     * @return true if the sequence of len bytes in a starting at
     * aOff matches those in b starting at bOff, false otherwise
     */
    public static boolean byteMatch(byte[] a, int aOff, int aLen,
                                    byte[] b, int bOff, int bLen) {
        if ((aLen != bLen) || (a.length < aOff + aLen) ||
                (b.length < bOff + bLen)) {
            return false;
        }

        for (int i = 0; i < aLen; i++) {
            if (a[i + aOff] != b[i + bOff])
                return false;
        }

        return true;
    }

    /**
     * Checks if two byte arrays match.
     * @param a the first byte array
     * @param b the second byte array
     * @return true if both arrays has the same length and contents
     */
    public static boolean byteMatch(byte[] a, byte[] b) {
        return byteMatch(a, 0, a.length, b, 0, b.length);
    }

    /**
     * Converts a sequence of bytes into a printable OID,
     * a string of decimal digits, each separated by a ".".
     * @param buffer byte array containing the bytes to be converted
     * @param offset starting offset of the byte subsequence inside b
     * @param length number of bytes to be converted
     * @return printable OID
     */
    public static String OIDtoString(byte[] buffer, int offset,
                                     int length) {

        StringBuffer result;
        int end;
        int t;
        int x;
        int y;

        if (length == 0) {
            return "";
        }

        result = new StringBuffer(40);

        end = offset + length;

        // first byte (t) always represents the first 2 values (x, y).
        // t = (x * 40) + y;

        t = buffer[offset++] & 0xff;
        x = t / 40;
        y = t - (x * 40);

        result.append(x);
        result.append('.');
        result.append(y);

        x = 0;
        while (offset < end) {
            // 7 bit per byte, bit 8 = 0 means the end of a value
            x = x << 7;

            t = buffer[offset++];
            if (t >= 0) {
                x += t;
                result.append('.');
                result.append(x);
                x = 0;
            } else {
                x += t & 0x7f;
            }
        }

        return result.toString();
    }

    /**
     * Converst OID from string representation into byte array.
     * @param oid string representation of OID
     * @return byte array containing DER value for this OID.
     */
    public static byte[] StringToOID(String oid) {

        if (oid.indexOf('-') != -1) {
            throw new IllegalArgumentException(oid);
        }

        ByteArrayOutputStream out = new ByteArrayOutputStream();

        int i = 0;
        int b1 = 0;
        int current = 0;

        while (current < oid.length()) {

            i++;

            int k = oid.indexOf('.', current);
            if (k == -1) {
                k = oid.length();
            }

            int v = Integer.parseInt(oid.substring(current, k));
            current = k + 1;

            if (i == 1) {
                b1 = v;
                continue;
            }

            if (i == 2) {
                v = b1 * 40 + v;
                if (v > 255) {
                    throw new IllegalArgumentException(oid);
                }
                out.write(v);
                continue;
            }

            int p = 0;
            k = v;

            while (true) {
                p += 1;
                k = k >> 7;
                if (k == 0) {
                    break;
                }
            }

            k = v;
            while (p > 0) {

                byte x = (byte) (k >> ((p - 1) * 7));

                if (p == 1) {
                    x &= 0x7f;
                } else {
                    x |= 0x80;
                }
                p--;
                out.write(x);
            }
        }

        if (i < 2) {
            throw new IllegalArgumentException(oid);
        }

        return out.toByteArray();
    }

    /**
     * Retrieves short value from byte array.
     * @param data byte array
     * @param offset value offset
     * @return the short value
     */
    public static short getShort(byte[] data, int offset) {
        return (short) getU2(data, offset);
    }

    /**
     * Retrieves unsigned 2-byte value from byte array.
     * @param data byte array
     * @param offset value offset
     * @return the value
     */
    public static int getU2(byte[] data, int offset) {
        return ((data[offset] & 0xff) << 8) | (data[offset + 1] & 0xff);
    }

    /**
     * Constructs integer value from byte array data.
     * @param data the byte array.
     * @param offset offset of the data.
     * @return the integer value.
     */
    public static int getInt(byte[] data, int offset) {

        int l = 0;
        for (int k = 0; k < 4; k++) {
            l = (l << 8) | (data[offset++] & 0xFF);
        }
        return l;
    }

    /**
     * Calculates SHA-1 hash for given data.
     * @param inBuf array containing the data
     * @param inOff data offset
     * @param inLen data length
     * @return SHA-1 hash
     */
    public static byte[] getHash(byte[] inBuf, int inOff, int inLen) {

        try {
            MessageDigest sha = MessageDigest.getInstance("SHA");
            sha.reset();
            byte[] hash = new byte[20];
            hash = sha.digest(inBuf);
            return hash;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Returns byte array which contains encoded short value.
     * @param i the value
     * @return byte array
     */
    public static byte[] shortToBytes(int i) {

        byte[] data = new byte[2];
        data[0] = (byte) (i >> 8);
        data[1] = (byte) i;
        return data;
    }

    /**
     * Returns byte array that contains sequence of encoded short values.
     * @param data the short values
     * @return byte array
     */

    public static byte[] shortsToBytes(short[] data) {

        byte[] d = new byte[2 * data.length];
        for (int i = 0; i < data.length; i++) {
            d[i * 2] = (byte) (data[i] >> 8);
            d[i * 2 + 1] = (byte) data[i];
        }
        return d;
    }

    /**
     * Returns UTF 8 encoding for this string.
     * @param s the string
     * @return UTF 8 encoding
     */
    public static byte[] stringToBytes(String s) {
        try {
            return s.getBytes(utf8);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        System.out.println("Internal error: unsupported encoding");
        return null;
    }

    /**
     * Writes hex representation of byte array elements.
     * @param writer where to write
     * @param data data to be written
     */
    public static void writeHex(PrintStream writer, byte[] data) {

        String s = "    ";

        for (int i = 0; i < data.length; i++) {

            if (data[i] > -1 && data[i] < 16) {
                s = s + "0";
            }
            s = s + Integer.toHexString(data[i] & 0xff) + " ";

            if ((i + 1) % 16 == 0) {
                writer.println(s);
                s = "    ";
            }
        }

        if (s.length() != 4) {
            writer.println(s);
        }
    }

    /**
     * Converts string to array of shorts
     * @param str input string
     * @return output array of shorts
     */
    public static short[] stringToShorts(String str) {
        byte[] b = str.getBytes();
        short[] res = new short[1+(b.length/4)];
        for (int i = 0; i < b.length; i++) {
            if ((b[i] >= 0x30) & (b[i] <= 0x39)) {
                b[i] = (byte)(b[i] - 0x30);
            } else {
                if ((b[i] >= 0x41) & (b[i] <= 0x46)) {
                    b[i] = (byte)(b[i] - 0x41 + 0xa);
                } else {
                    if ((b[i] >= 0x61) & (b[i] <= 0x66)) {
                        b[i] = (byte) (b[i] - 0x61 + 0xa);
                    }
                }

            }
        }
        res[0] = 0x3fff;
        for (int i = 0; i < res.length-1; i++) {
            res[i+1] =
                    (short)((b[4*i]<<12) | (b[4*i+1]<<8) |
                    (b[4*i+2]<<4) | b[4*i+3]);
        }
        return res;
    }
    /**
     * Creates java source for static array initialization.
     * @param writer where to write
     * @param name array name
     * @param data initial values
     */
    public static void writeDataArray(PrintStream writer, String name,
                                      byte[] data) {

        writer.println();

        String s = "    static byte[] " + name + " = { ";

        for (int i = 0; i < data.length; i++) {

            if (i != 0) {
                s = s + ", ";
            }

            String h = "" + data[i];

            if (s.length() + h.length() > 76) {
                writer.println(s);
                s = "        ";
            }

            s = s + h;
        }

        writer.println(s + "};");
        writer.println();
    }
}
