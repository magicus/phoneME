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

import java.io.IOException;

/**
 * This class represents PKCS#15 PIN attributes.
 */
public class PINAttributes {

    /** PIN type constant. */
    private static final int TYPE_BCD        = 0;
    /** PIN type constant. */
    private static final int TYPE_ASCII      = 1;
    /** PIN type constant. */
    private static final int TYPE_UTF        = 2;
    /** PIN type constant. */
    private static final int TYPE_HN         = 3;
    /** PIN type constant. */
    private static final int TYPE_ISO        = 4;

    /** PIN flag constant. */
    public static final int FLAG_CASE_SENSITIVE            = 0x8000;
    /** PIN flag constant. */
    public static final int FLAG_LOCAL                     = 0x4000;
    /** PIN flag constant. */
    public static final int FLAG_CHANGE_DISABLED           = 0x2000;
    /** PIN flag constant. */
    public static final int FLAG_UNBLOCK_DISABLED          = 0x1000;
    /** PIN flag constant. */
    public static final int FLAG_INITIALIZED               = 0x800;
    /** PIN flag constant. */
    public static final int FLAG_NEEDS_PADDING             = 0x400;
    /** PIN flag constant. */
    public static final int FLAG_UNBLOCKING_PIN            = 0x200;
    /** PIN flag constant. */
    public static final int FLAG_SOPIN                     = 0x100;
    /** PIN flag constant. */
    public static final int FLAG_DISABLE_ALLOWED           = 0x80;
    /** PIN flag constant. */
    public static final int FLAG_INTEGRITY_PROTECTED       = 0x40;
    /** PIN flag constant. */
    public static final int FLAG_CONFIDENTIALITY_PROTECTED = 0x20;
    /** PIN flag constant. */
    public static final int FLAG_EXCHANGEREFDATA           = 0x10;

    /** PIN label. */
    public String label;
    /** PIN identifier. */
    public int id;
    /** pinType PIN attribute. */
    public int pinType;
    /** minLength PIN attribute. */
    public int minLength;
    /** storedLength PIN attribute. */
    public int storedLength;
    /** maxLength PIN attribute. */
    public int maxLength;
    /** pinReference PIN attribute. */
    public int pinReference;
    /** padChar PIN attribute. */
    public int padChar;
    /** pinFlags PIN attribute. */
    public int pinFlags;
    /** Path PIN attribute. */
    public short[] path;
    /** Pin record */
    private TLV pinRec;


    /**
     * Constructs PINAttributes object.
     */
    public PINAttributes() {}

    /**
     * Constructs PINAttributes object.
     * @param r reader for permissions file.
     * @throws java.io.IOException if I/O error occurs.
     */
    PINAttributes(ACLFileReader r) throws IOException {

        r.checkWord("{");

        while (true) {

            String s = r.readWord();

            if (s.equals("}")) {
                break;
            }


            if (s.equals("label")) {
                label = r.readLine();
                continue;
            }

            if (s.equals("id")) {
                id = r.readByte();
                continue;
            }

            String h = r.readWord();

            if (s.equals("type")) {
                if (h.equals("bcd")) {
                    pinType = TYPE_BCD;
                } else
                if (h.equals("ascii")) {
                    pinType = TYPE_ASCII;
                } else
                if (h.equals("utf")) {
                    pinType = TYPE_UTF;
                } else
                if (h.equals("half-nibble")) {
                    pinType = TYPE_HN;
                } else
                if (h.equals("iso")) {
                    pinType = TYPE_ISO;
                } else {
                    throw new IOException();
                }
                continue;
            }

            if (s.equals("min")) {
                minLength = Integer.parseInt(h, 16);
                continue;
            }

            if (s.equals("stored")) {
                storedLength = Integer.parseInt(h, 16);
                continue;
            }

            if (s.equals("max")) {
                maxLength = Integer.parseInt(h, 16);
                continue;
            }

            if (s.equals("reference")) {
                pinReference = Integer.parseInt(h, 16);
                continue;
            }

            if (s.equals("pad")) {
                padChar = Integer.parseInt(h, 16);
                continue;
            }

            if (s.equals("flag")) {
                if (h.equals("case-sensitive")) {
                    pinFlags |= FLAG_CASE_SENSITIVE;
                } else
                if (h.equals("change-disabled")) {
                    pinFlags |= FLAG_CHANGE_DISABLED;
                } else
                if (h.equals("unblock-disabled")) {
                    pinFlags |= FLAG_UNBLOCK_DISABLED;
                } else
                if (h.equals("needs-padding")) {
                    pinFlags |= FLAG_NEEDS_PADDING;
                } else
                if (h.equals("disable-allowed")) {
                    pinFlags |= FLAG_DISABLE_ALLOWED;
                } else
                if (h.equals("unblockingPIN")) {
                    pinFlags |= FLAG_UNBLOCKING_PIN;
                } else {
                    throw new IOException();
                }
                continue;
            }
            throw new IOException();
        }
        createPINRec();
    }

    /**
     * Generates TLV structure that represents this PIN.
     * @return TLV structure
     * SEQUENCE                        PKCS15Object
     *   SEQUENCE                       CommonObjectAttributes
     *     UTF8_STRING                    label        opt  +
     *     BIT_STRING                     flags        opt  -
     *   SEQUENCE                       CommonAuthObjectAttributes
     *     OCTET_STRING                   authID
     *   0xA0                           Choice Object from PathOrObject
     *     SEQUENCE                       PinAttributes
     *       BIT_STRING                     PinFlags
     *       ENUMERATED                     PinType
     *       INT                            minLength
     *       INT                            storedLength
     *       INT                            maxLength
     *       Reference                      pinReference
     *       OCTET_STRING                   padChar       opt
     */
    /**
     * Creates PIN record
     */
    private void createPINRec() {

        TLV t, c, v;

        t = TLV.createSequence();

        t.setChild(TLV.createSequence()).
               setChild(TLV.createUTF8String(label));   /* label */

        c = t.child;

        c.setNext(TLV.createSequence()).                /* authId */
               setChild(TLV.createOctetString(new byte[] {(byte) id}));

        c.next.setNext(new TLV(ACEntry.CONTEXT_CONSTRUCTED_1)). 
            setChild(TLV.createSequence()).
            setChild(TLV.createBitString(Utils.shortToBytes(pinFlags), 4)).
            setNext(new TLV(TLV.ENUMERATED_TYPE, Utils.shortToBytes(pinType))).
            setNext(TLV.createInteger(minLength)).
            setNext(TLV.createInteger(storedLength)).
            setNext(TLV.createInteger(maxLength)).
            setNext(TLV.createInteger(pinReference).setTag(0x80)).
            setNext(TLV.createOctetString(new byte[] {(byte) padChar}));

        pinRec = t;
    }

    /**
     * Returns pinRec value
     * @return pinRec value
     */
    public TLV getPINRec() {
        return pinRec;
    }
    /**
     * Returns true if this PIN is a number.
     * @return true if this PIN is a number.
     */
    public boolean isNumeric() {
        return (pinType != TYPE_UTF);
    }

    /**
     * Returns maximum PIN length in characters.
     * @return maximum PIN length in characters.
     */
    public int getMaxLength() {

        if ((pinFlags & FLAG_NEEDS_PADDING) == 0) {
            return maxLength;
        }

        if (pinType == TYPE_BCD) {
            return storedLength * 2;
        }

        // UTF symbol may occupy 1 or 2 bytes, additional check is necessary

        return storedLength;
    }

    /**
     * Verifies if the specified operation can be performed on this PIN.
     * @param action operation identifier.
     * @return true if the specified operation can be performed on this PIN.
     */
    public boolean check(int action) {

        if (action == ACLPermissions.CMD_CHANGE) {
            return (pinFlags & FLAG_CHANGE_DISABLED) == 0;
        }

        if (action == ACLPermissions.CMD_DISABLE) {
            return (pinFlags & FLAG_DISABLE_ALLOWED) != 0;
        }

        if (action == ACLPermissions.CMD_UNBLOCK) {
            return (pinFlags & FLAG_UNBLOCK_DISABLED) == 0;
        }

        return true;
    }

    /**
     * Verifies if this PIN can be used to unblock other PINs.
     * @return true if this PIN can be used to unblock other PINs.
     */
    public boolean isUnblockingPIN() {
        return (pinFlags & FLAG_UNBLOCKING_PIN) != 0;
    }

    /**
     * Transforms string entered by user according to PIN attributes.
     * @param s the value enterd by user.
     * @return converted and padded PIN value.
     */
    public byte[] transform(String s) {

        if (s.length() < minLength) {
            return null;
        }

        byte[] data = null;

        if (pinType == TYPE_UTF) {

            if ((pinFlags & FLAG_CASE_SENSITIVE) == 0) {
                s = s.toUpperCase();  // locale?
            }

            data = Utils.stringToBytes(s);

            if (data.length > getMaxLength()) {
                return null;
            }

        } else {

            byte[] tmp = new byte[s.length()];
            for (int i = 0; i < tmp.length; i++) {
                tmp[i] = (byte) (s.charAt(i));
            }

            if (pinType == TYPE_ASCII || pinType == TYPE_ISO) {
                data = tmp;
            } else {
                if (pinType == TYPE_HN) {
                    data = tmp;
                    for (int i = 0; i < data.length; i++) {
                        data[i] = (byte) (0xf0 | (data[i] - 0x30));
                    }
                } else {   // type == TYPE_BCD

                    data = new byte[(tmp.length + 1) / 2];

                    for (int i = 0; i < data.length; i++) {

                        int l = i * 2;
                        int b1 = tmp[l] - 0x30;
                        int b2;
                        if (l + 1 == tmp.length) {
                            b2 = padChar;
                        } else {
                            b2 = tmp[l + 1] - 0x30;
                        }
                        data[i] = (byte) ((b1 << 4) | (b2 & 0xf));
                    }

                }
            }
        }

        if (((pinFlags & FLAG_NEEDS_PADDING) == 0) ||
             (data.length == storedLength)) {
            return data;
        }

        byte[] r = new byte[storedLength];
        System.arraycopy(data, 0, r, 0, data.length);
        for (int i = data.length; i < storedLength; i++) {
            r[i] = (byte) padChar;
        }

        return r;
    }
}
