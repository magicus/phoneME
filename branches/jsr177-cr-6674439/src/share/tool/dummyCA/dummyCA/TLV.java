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

package dummyCA;

import java.io.UnsupportedEncodingException;
import java.io.IOException;
import java.io.PrintStream;
import java.util.StringTokenizer;
import java.util.Calendar;

/**
 * Used to represent each Type, Length, Value structure in a DER buffer.
 */
class TLV {
    /** ASN context specific flag used in types (0x80). */
    static final int CONTEXT = 0x80;
    /** ASN constructed flag used in types (0x20). */
    static final int CONSTRUCTED = 0x20;
    /** ASN constructed flag used in types (0x20). */
    static final int EXPLICIT = CONSTRUCTED;
    /** ANY_STRING type used as a place holder. [UNIVERSAL 0] */
    static final int ANY_STRING_TYPE = 0x00; // our own impl
    /** ASN BOOLEAN type used in certificate parsing. [UNIVERSAL 1] */
    static final int BOOLEAN_TYPE    = 1;
    /** ASN INTEGER type used in certificate parsing. [UNIVERSAL 2] */
    static final int INTEGER_TYPE    = 2;
    /** ASN BIT STRING type used in certificate parsing. [UNIVERSAL 3] */
    static final int BITSTRING_TYPE  = 3;
    /** ASN OCTET STRING type used in certificate parsing. [UNIVERSAL 4] */
    static final int OCTETSTR_TYPE   = 4;
    /** ASN NULL type used in certificate parsing. [UNIVERSAL 5] */
    static final int NULL_TYPE       = 5;
    /** ASN OBJECT ID type used in certificate parsing. [UNIVERSAL 6] */
    static final int OID_TYPE        = 6;
    /** ASN UTF8String type used in certificate parsing. [UNIVERSAL 12] */
    static final int UTF8STR_TYPE    = 12;
    /**
     *  ASN SEQUENCE type used in certificate parsing.
     * [UNIVERSAL CONSTRUCTED 16]
     */
    static final int SEQUENCE_TYPE   = CONSTRUCTED + 16;
    /**
     * ASN SET type used in certificate parsing.
     * [UNIVERSAL CONSTRUCTED 17]
     */
    static final int SET_TYPE        = CONSTRUCTED + 17;
    /** ASN PrintableString type used in certificate parsing. [UNIVERSAL 19] */
    static final int PRINTSTR_TYPE   = 19;
    /** ASN TELETEX STRING type used in certificate parsing. [UNIVERSAL 20] */
    static final int TELETEXSTR_TYPE = 20;
    /** ASN IA5 STRING type used in certificate parsing. [UNIVERSAL 22] */
    static final int IA5STR_TYPE     = 22;
    /** ASN UCT time type used in certificate parsing [UNIVERSAL 23] */
    static final int UCT_TIME_TYPE   = 23;
    /**
     * ASN Generalized time type used in certificate parsing.
     * [UNIVERSAL 24]
     */
    static final int GEN_TIME_TYPE   = 24;
    /**
     * ASN UniversalString type used in certificate parsing.
     * [UNIVERSAL 28].
     */
    static final int UNIVSTR_TYPE    = 28;
    /** ASN BIT STRING type used in certificate parsing. [UNIVERSAL 30] */
    static final int BMPSTR_TYPE  = 30;
    /**
     * Context specific explicit type for certificate version.
     * [CONTEXT EXPLICIT 0]
     */
    static final int VERSION_TYPE    = CONTEXT + EXPLICIT + 0;
    /**
     * Context specific explicit type for certificate extensions.
     * [CONTEXT EXPLICIT 3]
     */
    static final int EXTENSIONS_TYPE = CONTEXT + EXPLICIT + 3;

    /**
     * Converts a subsequence of bytes into a printable OID,
     * a string of decimal digits, each separated by a ".".
     *
     * @param buffer byte array containing the bytes to be converted
     * @param offset starting offset of the byte subsequence inside b
     * @param length number of bytes to be converted
     *
     * @return printable OID
     */
    static String OIDtoString(byte[] buffer, int offset, int length) {
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

    /** OID must fit into byte array of this size. */
    private static final int MAX_OID_SIZE = 50;

    /**
     * Converts a printable OID into a subsequence of bytes.
     * @param oid printable OID
     * @return  byte array containing the OID
     */
    static byte[] StringToOID(String oid) {

        StringTokenizer t = new StringTokenizer(oid, ".");

        int[] values = new int[t.countTokens()];
        for (int i = 0; i < values.length; i++) {
            values[i] = Integer.parseInt(t.nextToken(), 10);
        }

        byte[] x = new byte[MAX_OID_SIZE];

        int i = 0;
        x[i++] = (byte) (values[0] * 40 + values[1]);

        for (int j = 2; j < values.length; j++) {

            int k = values[j];

            int p = 0;

            while (true) {
                p += 1;
                k = k >> 7;
                if (k == 0) {
                    break;
                }
            }

            k = values[j];
            while (p > 0) {

                x[i] = (byte) (k >> ((p - 1) * 7));

                if (p == 1) {
                    x[i] &= 0x7f;
                } else {
                    x[i] |= 0x80;
                }
                p--;
                i++;
            }
        }

        byte[] data = new byte[i];
        System.arraycopy(x, 0, data, 0, i);
        return data;
    }



    /** Hexadecimal digits. */
    static char[] hc = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    /**
     * Converts a subsequence of bytes in a byte array into a
     * corresponding string of hexadecimal digits, each separated by a ":".
     *
     * @param b byte array containing the bytes to be converted
     * @param off starting offset of the byte subsequence inside b
     * @param len number of bytes to be converted
     * @param max print a single "+" instead of the bytes after max,
     *        -1 for no max.
     * @return a string of corresponding hexadecimal digits or
     * an error string
     */
    static String hexEncode(byte[] b, int off, int len, int max) {
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

        r = new char[len * 3];

        for (i = 0, j = 0; ; ) {
            v = b[off + i] & 0xff;
            r[j++] = hc[v >>> 4];
            r[j++] = hc[v & 0x0f];

            i++;
            if (i >= len) {
                break;
            }

            if (i == max) {
                r[j++] = ' ';
                r[j++] = '+';
                break;
            }

            r[j++] = ':';
        }

        return (new String(r, 0, j));
    }

    /** Raw DER type. */
    int type;
    /** Number of bytes that make up the value. */
    int length;
    /** Offset of the value. */
    int valueOffset;
    /** Non-null for constructed types, the first child TLV. */
    TLV child;
    /** The next TLV in the parent sequence. */
    TLV next;
    /** Size of tag and length in DER encoding. */
    int TLSize;
    /** Buffer that contains the DER encoded TLV. */
    byte[] data;

    /**
     * Constructs a TLV structure, recursing down for constructed types.
     *
     * @param buffer DER buffer
     * @param offset where to start parsing
     *
     * @exception IndexOutOfBoundException if the DER is corrupt
     */
    TLV(byte[] buffer, int offset) {

        boolean constructed;
        int size;

        int start = offset;
        data = buffer;

        type = buffer[offset++] & 0xff;

        // recurse for constructed types, bit 6 = 1
        constructed = (type & 0x20) == 0x20;

        if ((type & 0x1f) == 0x1f) {
            // multi byte type, 7 bits per byte, only last byte bit 8 as zero

            type = 0;
            for (; ; ) {
                int temp = buffer[offset++];
                type = type << 7;
                if (temp >= 0) {
                    type += temp;
                    break;
                }

                // stip off bit 8
                temp = temp & 0x7f;
                type += temp;
            }
        }

        size = buffer[offset++] & 0xff;
        if (size >= 128) {
            int sizeLen = size - 128;

            // NOTE: for now, all sizes must fit int two bytes
            if (sizeLen > 2) {
                throw new RuntimeException("TLV size to large");
            }

            size = 0;
            while (sizeLen > 0) {
                size = (size << 8) + (buffer[offset++] & 0xff);
                sizeLen--;
            }
        }

        TLSize = offset - start;
        length = size;
        valueOffset = offset;

        if (constructed) {
            int end;
            TLV temp;

            end = offset + length;

            child = new TLV(buffer, offset);
            temp = child;
            for (; ; ) {
                offset = temp.valueOffset + temp.length;
                if (offset >= end) {
                    break;
                }

                temp.next = new TLV(buffer, offset);
                temp = temp.next;
            }
        }
    }

    /**
     * Constructs a TLV structure.
     * @param tag tag of new TLV
     */
    TLV(int tag) {
        type = tag;
    }

    /**
     * Constructs a TLV structure.
     * @param tag tag of new TLV
     * @param bytes value of new TLV
     */
    public TLV(int tag, byte[] bytes) {

        type = tag;
        length = bytes.length;

        data = new byte[length + 4];
        int i = putHeader(data, 0);

        TLSize = i;
        valueOffset = i;
        System.arraycopy(bytes, 0, data, i, bytes.length);
    }

    /**
     * Creates UTCTime TLV structure for given date.
     * @param time date
     */
    public static TLV createUTCTime(Calendar time) {
        byte[] data = new byte[13];
        putDigits(data, 0, time.get(Calendar.YEAR));
        putDigits(data, 2, time.get(Calendar.MONTH) + 1);
        putDigits(data, 4, time.get(Calendar.DAY_OF_MONTH));
        putDigits(data, 6, time.get(Calendar.HOUR_OF_DAY));
        putDigits(data, 8, time.get(Calendar.MINUTE));
        putDigits(data, 10, time.get(Calendar.SECOND));
        data[12] = 0x5a;
        return new TLV(UCT_TIME_TYPE, data);
    }

    /**
     * Places two ASCII encoded decimal digits into byte array.
     * @param data byte aray
     * @param offset the index of the first byte
     * @param value the value to be placed into the buffer
     */
    private static void putDigits(byte[] data, int offset, int value) {

        value = value % 100;
        data[offset++] = (byte) (0x30 | (value / 10));
        data[offset++] = (byte) (0x30 | (value % 10));
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     */
    void print() {
        print(System.out, 0);
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     * @param out output stream
     */
    void print(PrintStream out) {
        print(out, 0);
    }

    /**
     * Prints the a TLV structure, recursing down for constructed types.
     * @param out output stream
     * @param level what level this TLV is at
     */
    private void print(PrintStream out, int level) {

        for (int i = 0; i < level; i++) {
            out.print("    ");
        }

        byte[] buffer;

        if (data != null) {
            buffer = data;
        } else {
            buffer = getDERData();
        }

        if (child == null) {
            out.print("Type: 0x" + Integer.toHexString(type) +
                             " length: " + length + " value: ");
            if (type == PRINTSTR_TYPE ||
                type == TELETEXSTR_TYPE ||
                type == UTF8STR_TYPE ||
                type == IA5STR_TYPE ||
                type == UNIVSTR_TYPE) {
                try {
                    out.print(new String(buffer, valueOffset, length,
                                                "UTF-8"));
                } catch (UnsupportedEncodingException e) {
                    // ignore
                }
            } else if (type == OID_TYPE) {
                out.print(OIDtoString(buffer, valueOffset, length));
            } else {
                out.print(hexEncode(buffer, valueOffset, length, 999999));
            }

            out.println("");
        } else {
            if (type == SET_TYPE) {
                out.print("Set:");
            } else {
                out.print("Sequence:");
            }

            out.println("  (0x" + Integer.toHexString(type) +
                             " " + length + ")");

            child.print(out, level + 1);
        }

        if (next != null) {
            next.print(out, level);
        }
    }

    /**
     * Returns string representation of OID represented by this TLV.
     * @return string representation of OID represented by this TLV
     * @throws IOException if TLV doesn't contain OID
     */
    String getOID() throws IOException {

        if (type != OID_TYPE) {
            throw new IOException("OID expected");
        }
        return OIDtoString(data, valueOffset, length);
    }

    /**
     * Returns the value field of this TLV.
     * @return the value field of this TLV
     */
    byte[] getValue() {

        if (data == null) {
            return copy().getValue();
        }

        byte[] x = new byte[length];
        System.arraycopy(data, valueOffset, x, 0, length);
        return x;
    }

    /**
     * Places tag and length values into the buffer.
     * @param x byte buffer
     * @param i offset
     * @return value offset in the buffer
     */
    private int putHeader(byte[] x, int i) {

        x[i++] = (byte) type;

        if (length < 128) {
            x[i++] = (byte) length;
        } else
        if (length < 256) {
            x[i++] = (byte) 0x81;
            x[i++] = (byte) length;
        } else {
            x[i++] = (byte) 0x82;
            x[i++] = (byte) (length >> 8);
            x[i++] = (byte) length;
        }
        return i;
    }

    /**
     * Returns DER encoded TLV.
     * @return DER encoded TLV
     */
    byte[] getDERData() {

        if (data == null) {

            byte[] x = new byte[getDERSize()];

            int i = putHeader(x, 0);

            TLV c = child;

            while (c != null) {
                byte[] cdata = c.getDERData();
                System.arraycopy(cdata, 0, x, i, cdata.length);
                i += cdata.length;
                c = c.next;
            }
             if (i != x.length) {
                 throw new RuntimeException("debug");
             }

            return x;
        }

        byte[] x = new byte[length + TLSize];
        System.arraycopy(data, valueOffset - TLSize, x, 0, length + TLSize);
        return x;
    }

    /**
     * Returns the size of DER encoded TLV.
     * @return the size of DER encoded TLV
     */
    private int getDERSize() {

        if (data == null) {

            int size = 0;

            TLV c = child;

            while (c != null) {
                size += c.getDERSize();
                c = c.next;
            }

            length = size;

            // length size
            if (size < 128) {
                size += 1;
            } else {
                size += 1;
                int i = size;
                while (i != 0) {
                    size += 1;
                    i = i >> 8;
                }
            }

            // tag size - only one byte tags are used
            size += 1;

            TLSize =  size - length;
        }
        return length + TLSize;
    }

    /**
     * Creates a copy of this TLV. The value of field next of the new TLV is
     * null.
     * @return a copy of this TLV
     */
    TLV copy() {
        return new TLV(getDERData(), 0);
    }
}
