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

import java.io.UnsupportedEncodingException;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Calendar;
import java.lang.RuntimeException;

/**
 * Used to represent Type, Length, Value structure in a DER buffer.
 */
public class TLV {
    /** ASN context specific flag used in types (0x80). */
    static final int CONTEXT = 0x80;
    /** ASN constructed flag used in types (0x20). */
    static final int CONSTRUCTED = 0x20;
    /** ASN constructed flag used in types (0x20). */
    static final int EXPLICIT = CONSTRUCTED;
    /** ANY_STRING type used as a place holder. [UNIVERSAL 0] */
    static final int ANY_STRING_TYPE = 0x00;
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
    /** ASN ENUMERATED type. [UNIVERSAL 10] */
    static final int ENUMERATED_TYPE        = 10;
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
    /** Buffer that contains the DER encoded TLV. */
    byte[] data;

    /**
     * Constructs a TLV structure, recursing down for constructed types.
     * @param buffer DER buffer
     * @param offset where to start parsing
     * @exception java.lang.IndexOutOfBoundsException if the DER is corrupt
     */
    public TLV(byte[] buffer, int offset) {

        boolean constructed;
        int size;

        int start = offset;
        data = buffer;

        type = buffer[offset++] & 0xff;

        // recurse for constructed types, bit 6 = 1
        constructed = (type & 0x20) == 0x20;

        if ((type & 0x1f) == 0x1f) {
            // multi byte type, 7 bits per byte, only last byte bit 8 as zero
            throw new RuntimeException("Invalid tag");
        }

        size = buffer[offset++] & 0xff;
        if (size >= 128) {
            int sizeLen = size - 128;

            // NOTE: for now, all sizes must fit int 3 bytes
            if (sizeLen > 3) {
                throw new RuntimeException("TLV size to large");
            }

            size = 0;
            while (sizeLen > 0) {
                size = (size << 8) + (buffer[offset++] & 0xff);
                sizeLen--;
            }
        }

        length = size;
        valueOffset = offset;

        if (constructed && length != 0) {

            int end = offset + length;
            child = new TLV(buffer, offset);
            TLV temp = child;

            for (;;) {
                offset = temp.valueOffset + temp.length;
                if (offset == end) {
                    break;
                }
                if (offset > end) {
                    throw new RuntimeException("incorrect structure");
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

        valueOffset = i;
        System.arraycopy(bytes, 0, data, i, bytes.length);
    }

    /**
     * Creates UTCTime TLV structure for given date.
     * @param time date
     * @return new TLV object
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
     * Creates new sequence.
     * @return new TLV object
     */
    public static TLV createSequence() {
        return new TLV(SEQUENCE_TYPE);
    }

    /**
     * Creates new integer.
     * @param data buffer containing the value
     * @return new TLV object
     */
    public static TLV createInteger(byte[] data) {
        return new TLV(INTEGER_TYPE, data);
    }

    /**
     * Creates new octet string.
     * @param data buffer containing the value
     * @return new TLV object
     */
    public static TLV createOctetString(byte[] data) {
        return new TLV(OCTETSTR_TYPE, data);
    }

    /**
     * Creates new OID object.
     * @param oid string representation of OID.
     * @return new TLV object
     */
    public static TLV createOID(String oid) {
        return new TLV(TLV.OID_TYPE, Utils.StringToOID(oid));
    }

    /**
     * Creates new UTF8 string.
     * @param s string value
     * @return new TLV object
     */
    public static TLV createUTF8String(String s) {
        try {
            return new TLV(TLV.UTF8STR_TYPE, s.getBytes("UTF-8"));
        } catch (UnsupportedEncodingException e) {}
        return null;
    }

    /**
     * Creates new integer.
     * @param value the value of new integer
     * @return new TLV object
     */
    public static TLV createInteger(long value) {

        int check = (value < 0) ? -1 : 0;

        int i = 1;
        while (true) {
            if (value >> (i * 8) == check) {
                byte v = (byte) (value >> ((i - 1) * 8));
                if (value < 0 ? v > 0 : v < 0) {
                    i++;
                }
                break;
            }
            i++;
        }

        byte[] data = new byte[i];
        while (i > 0) {
            i--;
            data[i] = (byte) value;
            value = value >> 8;
        }
        return new TLV(TLV.INTEGER_TYPE, data);
    }

    /**
     * Creates a copy of this TLV. The value of field next of the new TLV is
     * null.
     * @return a copy of this TLV
     */
    public TLV copy() {
        return new TLV(getDERData(), 0);
    }

    /**
     * Sets the next DER entry for this object.
     * @param next TLV object
     * @return the value of next to allow call chaining
     */
    public TLV setNext(TLV next) {
        this.next = next;
        return next;
    }

    /**
     * Sets the child DER entry for this object.
     * @param child TLV object
     * @return the value of child field to allow call chaining
     */
    public TLV setChild(TLV child) {
        this.child = child;
        return child;
    }

    /**
     * Sets the (implicit) tag value for this object.
     * @param tag tag value
     * @return <code>this</code> value to allow call chaining
     */
    public TLV setTag(int tag) {
        this.type = tag;
        return this;
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     */
    public void print() {
        print(System.out, 0);
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     * @param out output stream
     */
    public void print(PrintStream out) {
        print(out, 0);
    }

    /**
     * Returns string representation of OID represented by this TLV.
     * @return string representation of OID represented by this TLV
     * @throws java.io.IOException if TLV doesn't contain OID
     */
    public String getOID() throws IOException {

        if (type != OID_TYPE) {
            throw new IOException("OID expected");
        }
        return Utils.OIDtoString(data, valueOffset, length);
    }

    /**
     * Returns the value field of this TLV.
     * @return the value field of this TLV
     */
    public byte[] getValue() {

        if (data == null) {
            getDERSize();
        }
        byte[] x = new byte[length];
        getValue_(x, 0);
        return x;
    }

    /**
     * Places the value field of this TLV into the buffer.
     * @param buffer destination buffer
     * @param offset offset in the buffer
     * @return TLV size in bytes
     */
    public int getValue(byte[] buffer, int offset) {

        if (data == null) {
            getDERSize();
        }
        return getValue_(buffer, offset);
    }

    /**
     * Returns DER encoded TLV.
     * @return DER encoded TLV
     */
    public byte[] getDERData() {

        byte[] x = new byte[getDERSize()];
        getDERData_(x, 0);
        return x;
    }

    /**
     * Returns DER encoded TLV.
     * @param buffer destination buffer
     * @param offset offset in the buffer
     * @return DER encoded TLV
     */
    public int getDERData(byte[] buffer, int offset) {

        getDERSize();
        return getDERData_(buffer, offset);
    }

    /**
     * Returns the size of DER encoded TLV.
     * @return the size of DER encoded TLV
     */
    public int getDERSize() {

        if (data == null) {
            length = 0;
            TLV c = child;
            while (c != null) {
                length += c.getDERSize();
                c = c.next;
            }
        }
        return length + getTLSize();
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
                out.print(Utils.OIDtoString(buffer, valueOffset, length));
            } else {
                out.print(Utils.hexEncode(buffer, valueOffset, length));
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
     * Places the value field of this TLV into the buffer.
     * @param buffer destination buffer
     * @param offset offset in the buffer
     * @return TLV size in bytes
     */
    private int getValue_(byte[] buffer, int offset) {

        if (data == null) {
            TLV c = child;
            while (c != null) {
                offset += c.getDERData_(buffer, offset);
                c = c.next;
            }
        } else {
            System.arraycopy(data, valueOffset, buffer, offset, length);
        }
        return length;
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
     * Places DER encoded TLV into the buffer.
     * @param buffer destination buffer
     * @param offset offset in the buffer
     * @return TLV size
     */
    private int getDERData_(byte[] buffer, int offset) {

        int initialOffset = offset;
        offset = putHeader(buffer, offset);

        if (data == null) {
            TLV c = child;
            while (c != null) {
                offset += c.getDERData_(buffer, offset);
                c = c.next;
            }
        } else {
            System.arraycopy(data, valueOffset, buffer, offset, length);
            offset += length;
        }
        return (offset - initialOffset);
    }

    /**
     * Returns size in bytes of tag and length.
     * @return size in bytes of tag and length
     */
    private int getTLSize() {

        int TLSize = 2;
        if (length >= 128) {
            int i = length;
            while (i != 0) {
                TLSize++;
                i = i >> 8;
            }
        }
        return TLSize;
    }
}
