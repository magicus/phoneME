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

package com.sun.satsa.pki;

import java.io.ByteArrayOutputStream;
import java.io.UnsupportedEncodingException;
import com.sun.satsa.util.*;

/**
 * This class provides support for distinguished name formatted
 * according to RFC 2253.
 */
public class RFC2253Name {

    /**
     * In RFC2253, if the value has any of these characters in it, it
     * must be quoted by a preceding \.
     */
    private static final String specialChars2253 = ",+\"\\<>;=";

    /**
     * DomainComponent type OID.
     */
    private static final byte[] dcOID = {
        9, (byte) 0x92, (byte) 0x26, (byte) 0x89, (byte) 0x93,
        (byte) 0xF2, (byte) 0x2C, (byte) 0x64, (byte) 0x01,
        (byte) 0x19
    };
    
    /**
     * Component type names.
     */
    private static String[] names = {
        "CN",
        "C",
        "L",
        "ST",
        "STREET",
        "O",
        "OU",
        "DC",
        "UID"};

    /**
     * Component type OIDs.
     */
    private static byte[][] OIDs = {
        {85, 4, 3},
        {85, 4, 6},
        {85, 4, 7},
        {85, 4, 8},
        {85, 4, 9},
        {85, 4, 10},
        {85, 4, 11},
        dcOID,
        {9, (byte) 0x92, (byte) 0x26, (byte) 0x89, (byte) 0x93,
         (byte) 0xF2, (byte) 0x2C, (byte) 0x64, (byte) 0x01,
         (byte) 0x01}};

    /**
     * Original name.
     */
    private String src;
    /**
     * Current offset in the name.
     */
    private int index;
    /**
     * Is RelativeDistinguishedName complete?
     */
    private boolean complete;
    /**
     * Is value hex encoded (starts with #)?
     */
    private boolean encoded;
    /**
     * Temporary string buffer for parser.
     */
    private StringBuffer tmp = new StringBuffer();

    /**
     * This method converts distinguished name from string to DER
     * encoding.
     * @param name distinguished name
     * @return DER encoded name
     * @throws IllegalArgumentException on any error.
     */
    public static byte[] toDER(String name) {

        try {
            return new RFC2253Name(name).encode().getDERData();
        } catch (TLVException tlve) {
            throw new IllegalArgumentException(name);
        } catch (StringIndexOutOfBoundsException siobe) {
            throw new IllegalArgumentException(name);
        } catch (IllegalArgumentException iae) {
            throw new IllegalArgumentException(name);
        }
    }

    /**
     * This method converts distinguished name from string to TLV object.
     * @param name distinguished name
     * @return DER encoded name
     * @throws IllegalArgumentException on any error.
     */
    public static TLV toTLV(String name) {

        try {
            return new RFC2253Name(name).encode();
        } catch (TLVException e) {
            throw new IllegalArgumentException(name);
        } catch (StringIndexOutOfBoundsException siobe) {
            throw new IllegalArgumentException(name);
        } catch (IllegalArgumentException iae) {
            throw new IllegalArgumentException(name);
        }
    }


    /**
     * Creates a new RFC2253Name instance.
     * @param name distinguished name
     */
    private RFC2253Name(String name) {
        src = name;
    }

    /**
     * Parses the name and returns TLV object.
     * @return DER encoded name
     */
    private TLV encode() throws TLVException {

        if (src == null || src.trim().length() == 0) {
            throw new IllegalArgumentException();
        }

        ByteArrayOutputStream buf = null;
        TLV atvSet = null;
        TLV atvSetTail = null;
        TLV rdnSequence = null;

        while (index < src.length()) {

            String id = getName();
            String s = getValue();

            int i = -1;
            for (int j = 0; j < names.length; j++) {
                if (id.equals(names[j])) {
                    i = j;
                    break;
                }
            }

            TLV type;

            if (i == -1) {
                if (id.startsWith("OID.")) {
                    id = id.substring(4);
                }
                type = TLV.createOID(id);
            } else {
                type = new TLV(TLV.OID_TYPE, OIDs[i]);
            }

            TLV value = null;

            if (encoded) {

                if (buf == null) {
                    buf = new ByteArrayOutputStream(s.length() / 2);
                } else {
                    buf.reset();
                }

                for (i = 0; i < s.length(); i += 2) {
                    buf.write(Integer.parseInt(s.substring(i, i + 2), 16));
                }

                value = new TLV(buf.toByteArray(), 0);
                if (value.getDERSize() != (s.length() + 1) / 2) {
                    throw new IllegalArgumentException();
                }

            } else {
                byte[] typeOID = type.getValue();
                if (Utils.byteMatch(dcOID, 0, dcOID.length, 
                                    typeOID, 0, typeOID.length)) {
                    value = TLV.createIA5String(s);
                } else {
                    value = TLV.createUTF8String(s);
                }
            }

            TLV tv = TLV.createSequence();
            tv.setChild(type).setNext(value);

            if (atvSet == null) {
                atvSet = tv;
            } else {
                atvSetTail.setNext(tv);
            }
            atvSetTail = tv;

            if (complete) {
                TLV set = new TLV(TLV.SET_TYPE);
                set.setChild(atvSet);
                set.setNext(rdnSequence);
                rdnSequence = set;
                atvSet = null;
            }
        }

        if (! complete) {
            throw new IllegalArgumentException();
        }

        TLV name = TLV.createSequence();
        name.setChild(rdnSequence);

        return name;
    }

    /**
     * Returns the next attribute name.
     * @return name
     */
    private String getName() {

        int i = index;
        index = src.indexOf('=', index);
        if (index == -1) {
            throw new IllegalArgumentException();
        }
        return src.substring(i, index++).trim().toUpperCase();
    }

    /**
     * Returns the next attribute value.
     * @return attribute value
     */
    private String getValue() {

        tmp.delete(0, tmp.length());
        complete = true;
        encoded = false;
        boolean quote = false;
        int expectedLength = -1;
        boolean firstSpace = false;

        while (index < src.length()) {

            char c = src.charAt(index++);

            if (c == '\"') {
                if (quote) {
                    expectedLength = tmp.length();
                } else
                if (tmp.length() != 0) {
                    throw new IllegalArgumentException();
                }
                quote = ! quote;
                continue;
            }

            if (! quote && tmp.length() == 0) {
                if (c == ' ') {
                    continue;
                }
                if (c == '#') {
                    encoded = true;
                    continue;
                }
            }

            if (c == '\\') {

                c = src.charAt(index++);

                if (c == '#' || specialChars2253.indexOf(c) != -1) {
                    tmp.append(c);
                    continue;
                }

                if (! quote && c == ' ') {
                    if (expectedLength > 1) {
                        // only the 1st and the last space can be escaped
                        throw new IllegalArgumentException();
                    }
                    tmp.append(c);
                    expectedLength = tmp.length();
                    firstSpace = (tmp.length() == 1);
                    continue;
                }

                tmp.append((char) Integer.parseInt(
                        src.substring(index - 1, index + 1), 16));
                index++;
                continue;
            }

            if (! quote && (specialChars2253.indexOf(c) != -1)) {
                if (c == ',' || c == ';') {
                    if (index == src.length()) {
                        throw new IllegalArgumentException();
                    }
                    break;
                }
                if (c == '+') {
                    complete = false;
                    break;
                }
                throw new IllegalArgumentException();
            }

            tmp.append(c);
        }

        if (quote) {
            throw new IllegalArgumentException();
        }

        int i = tmp.length();
        while (i > 0 && i-- > expectedLength && tmp.charAt(i) == ' ') {
            tmp.deleteCharAt(i);
        }

        if (! (expectedLength == -1 || firstSpace ||
            tmp.length() == expectedLength)) {
            throw new IllegalArgumentException();
        }

        return tmp.toString();
    }

    /**
     * Compares two names in TLV form.
     * @param name1 the 1st name
     * @param name2 the 2nd name
     * @return true if the names are equal
     * @exception IllegalArgumentException in case of any error
     */
    public static boolean compare(TLV name1, TLV name2) {

        try {
            TLV set1 = name1.child;
            TLV set2 = name2.child;
    
            while (true) {
    
                if (set1 == null || set2 == null) {
                    return (set1 == set2);
                }
    
                TLV seq1 = set1.child;
                TLV seq2 = set2.child;
    
                while (true) {
    
                    if (seq1 == null && seq2 == null) {
                        break;
                    }
    
                    if (seq1 == null || seq2 == null) {
                        return false;
                    }
    
                    TLV v1 = seq1.child;
                    TLV v2 = seq2.child;
    
                    if (! v1.match(v2)) {
                        return false;
                    }
    
                    v1 = v1.next;
                    v2 = v2.next;
    
                    if (! (v1.match(v2) || stringsMatch(v1, v2))) {
                        return false;
                    }
    
                    seq1 = seq1.next;
                    seq2 = seq2.next;
                }
    
                set1 = set1.next;
                set2 = set2.next;
            }
        } catch (IllegalArgumentException iae) {
            throw iae;
        } catch (NullPointerException npe) {
            throw new IllegalArgumentException("Invalid TLV");
        } catch (IndexOutOfBoundsException iobe) {
            throw new IllegalArgumentException("Invalid TLV");
        }
    }

    /**
     * Compares two attribute values ignoring multiple spaces and
     * register.
     * @param v1 1st attribute value
     * @param v2 2nd attribute value
     * @return true if equal
     */
    private static boolean stringsMatch(TLV v1, TLV v2) {

        if (! (v1.isString() && v2.isString())) {
            return false;
        }

        char[] c1 = getChars(v1);
        char[] c2 = getChars(v2);

        int i1 = 0;
        int i2 = 0;

        while (true) {

            if (i1 == c1.length || i2 == c2.length) {
                return (i1 == c1.length && i2 == c2.length);
            }

            while (c1[i1] == ' ' && c1[i1 + 1] == ' ') {
                i1++;
            }

            while (c2[i2] == ' ' && c2[i2 + 1] == ' ') {
                i2++;
            }

            if (c1[i1++] != c2[i2++]) {
                return false;
            }
        }
    }

    /**
     * Returns array of characters for comparison.
     * @param v attribute value
     * @return array of characters
     * @throws IllegalArgumentException if conversion to
     *         utf-8 fails
     */
    private static char[] getChars(TLV v) {

        try {
            return (new String(v.data, v.valueOffset, v.length, Utils.utf8)).
                    trim().toUpperCase().toCharArray();
        } catch (UnsupportedEncodingException e) {
            throw new IllegalArgumentException("invalid encoding");
        }
    }

    /**
     * Returns string representation for parsed RFC2253 name.
     * @param name the name
     * @return string representation of the name or null in case of error
     */
    public static String NameToString(TLV name) {

        try {
            String result = "";
            name = name.child;
            while (name != null) {

                String s = "";
                TLV rdn = name.child;
                name = name.next;

                while (rdn != null) {

                    if (s.length() != 0) {
                        s = s + " + ";
                    }

                    TLV t = rdn.child;
                    rdn = rdn.next;

                    boolean encodeValue = true;

                    for (int i = 0; i < OIDs.length; i++) {
                        if (Utils.byteMatch(OIDs[i], 0, OIDs[i].length,
                                     t.data, t.valueOffset, t.length)) {
                            s += names[i];
                            encodeValue = false;
                            break;
                        }
                    }

                    if (encodeValue) {
                        s += "OID." + Utils.OIDtoString(t.data,
                                               t.valueOffset, t.length);
                    }

                    t = t.next;

                    String value;
                    try {
                        value = new String(t.data, t.valueOffset,
                                           t.length, Utils.utf8);
                    } catch (UnsupportedEncodingException uee) {
                        encodeValue = true;
                        value = null;
                    }

                    if (encodeValue) {
                        byte[] data = t.getDERData();
                        value = "#" + Utils.hexNumber(data, 0, data.length);
                    } else {
                        boolean special = value.startsWith(" ") ||
                                value.startsWith("#") ||
                                value.endsWith(" ");
                        for (int i = 0; i < specialChars2253.length(); i++) {
                            special |= value.indexOf(
                                       specialChars2253.charAt(i)) > -1;
                        }

                        if (special) {
                            int i = value.length();
                            while (i-- > 0) {
                                char c = value.charAt(i);
                                if (c == '"' || c == '\\') {
                                    value = value.substring(0, i) + "\\" +
                                            value.substring(i);
                                }
                            }
                            value = "\"" + value + "\"";
                        }

                        int i = value.length();
                        while (i-- > 0) {
                            char c = value.charAt(i);
                            if (c > -1 && c < 32) {
                                value = value.substring(0, i) +
                                        "\\" + Utils.hexByte(c) +
                                        value.substring(i + 1);
                            }
                        }
                    }
                    s += "=" + value;
                }
                result = result.length() == 0 ? s : s + ", " + result;
            }
            return result;
        } catch (NullPointerException npe) {
            return null;
        } catch (IndexOutOfBoundsException iobe) {
            return null;
        } catch (IllegalArgumentException iae) {
            return null;
        }
    }
}
