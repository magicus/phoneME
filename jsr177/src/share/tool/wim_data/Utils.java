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

package wim_data;

import java.io.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * This class implements miscellaneous utility methods.
 */
public class Utils {

    /** Hexadecimal digits. */
    private static char[] hc = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    /**
     * Converts a subsequence of bytes in a byte array into a
     * corresponding string of hexadecimal digits, each separated by a ":".
     * @param b byte array containing the bytes to be converted
     * @param off starting offset of the byte subsequence inside b
     * @param len number of bytes to be converted
     * @return a string of corresponding hexadecimal digits or
     * an error string
     */
    static String hexEncode(byte[] b, int off, int len) {
        return new String(hexEncodeToChars(b, off, len));
    }

    /**
     * Converts a subsequence of bytes in a byte array into a
     * corresponding string of hexadecimal digits, each separated by a ":".
     * @param b byte array containing the bytes to be converted
     * @param off starting offset of the byte subsequence inside b
     * @param len number of bytes to be converted
     * @return a string of corresponding hexadecimal digits or
     * an error string
     */
    static char[] hexEncodeToChars(byte[] b, int off, int len) {
        char[] r;
        int v;
        int i;
        int j;

        if ((b == null) || (len == 0)) {
            return new char[0];
        }

        if ((off < 0) || (len < 0)) {
            throw new ArrayIndexOutOfBoundsException();
        }

        if (len == 1) {
            r = new char[len * 2];
        } else {
            r = new char[(len * 3) - 1];
        }

        for (i = 0, j = 0; ; ) {
            v = b[off + i] & 0xff;
            r[j++] = hc[v >>> 4];
            r[j++] = hc[v & 0x0f];

            i++;
            if (i >= len) {
                break;
            }

            r[j++] = ':';
        }

        return r;
    }

    /**
     * Converts a byte array into a corresponding string of hexadecimal
     * digits. This is equivalent to hexEncode(b, 0, b.length).
     * @param b byte array to be converted
     * @return corresponding hexadecimal string
     */
    static String hexEncode(byte[] b) {
        if (b == null)
            return ("");
        else
            return hexEncode(b, 0, b.length);
    }

    /**
     * Converts a long value to a cooresponding 8-byte array
     * starting with the most significant byte.
     * @param n 64-bit long integer value
     * @return a corresponding 8-byte array in network byte order
     */
    static byte[] longToBytes(long n) {
        byte[] b = new byte[8];

        for (int i = 0; i < 64; i += 8) {
            b[i >> 3] = (byte) ((n >> (56 - i)) & 0xff);
        }
        return b;
    }

    /**
     * Checks if two byte arrays match.
     * @param a first byte array
     * @param aOff starting offset for comparison within a
     * @param b second byte array
     * @param bOff starting offset for comparison within b
     * @param len number of bytes to be compared
     * @return true if the sequence of len bytes in a starting at
     * aOff matches those in b starting at bOff, false otherwise
     */
    static boolean byteMatch(byte[] a, int aOff,
                             byte[] b, int bOff, int len) {
        if ((a.length < aOff + len) ||
            (b.length < bOff + len)) return false;

        for (int i = 0; i < len; i++) {
            if (a[i + aOff] != b[i + bOff])
                return false;
        }

        return true;
    }

    /**
     * Calculates SHA hash for given data.
     * @param data byte array containing the bytes to be hashed
     * @param offset starting offset
     * @param length number of bytes
     * @return hash value
     */
    public static byte[] getHash(byte[] data, int offset, int length) {

        MessageDigest SHA = null;
        try {
            SHA = MessageDigest.getInstance("SHA-1");
        } catch (NoSuchAlgorithmException e) {}

        SHA.reset();
        SHA.update(data, offset, length);
        return SHA.digest();
    }

    /**
     * Converts a sequence of bytes into a printable OID,
     * a string of decimal digits, each separated by a ".".
     * @param buffer byte array containing the bytes to be converted
     * @param offset starting offset of the byte subsequence inside b
     * @param length number of bytes to be converted
     * @return printable OID
     */
    public static String OIDtoString(byte[] buffer, int offset, int length) {
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

        /*
         * first byte (t) always represents the first 2 values (x, y).
         * t = (x * 40) + y;
         */
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
     * Converts short value into 2 byte array.
     * @param i source value
     * @return resulting byte array
     */
    public static byte[] shortToBytes(int i) {

        byte[] data = new byte[2];
        data[0] = (byte) (i >> 8);
        data[1] = (byte) i;
        return data;
    }

    /**
     * Converts multiple short values into byte array.
     * @param v source values
     * @return resulting byte array
     */
    public static byte[] shortToBytes(short[] v) {

        byte[] data = new byte[v.length * 2];
        for (int i = 0; i < v.length; i++) {
            data[i * 2] = (byte) (v[i] >> 8);
            data[i * 2 + 1] = (byte) v[i];
        }
        return data;
    }

    /**
     * Creates TLV structure for given card file path/offset/length.
     * @param id path
     * @param offset offset in file
     * @param len length of data
     * @return TLV structure for this path
     */
    public static TLV createPath(short[] id, int offset, int len) {

        TLV t = TLV.createSequence();

        t.setChild(TLV.createOctetString(shortToBytes(id))).
            setNext(new TLV(TLV.INTEGER_TYPE, shortToBytes(offset))).
            setNext(new TLV(TLV.INTEGER_TYPE, shortToBytes(len)).setTag(0x80));

        return t;
    }

    /**
     * Creates TLV structure for given card file path.
     * @param id path
     * @return TLV structure for this path
     */
    public static TLV createPath(short[] id) {

        TLV t = TLV.createSequence();
        t.setChild(TLV.createOctetString(shortToBytes(id)));
        return t;
    }

    /**
     * Creates TLV structure for label.
     * @param label label text
     * @return TLV structure for this label
     */
    public static TLV createLabel(String label) {

        // Only ASCII symbols should be used
        while (label.length() < 32) {
            label = label + " ";
        }
        return TLV.createUTF8String(label);
    }
}
