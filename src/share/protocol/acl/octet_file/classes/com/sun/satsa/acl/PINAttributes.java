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

package com.sun.satsa.acl;

import java.util.Vector;
import com.sun.satsa.util.*;

/**
 * This class represents PKCS#15 PIN attributes.
 */
public class PINAttributes {

    /** BCD PIN type constant. */
    private static final int TYPE_BCD        = 0;
    /** ASCII PIN type constant. */
    private static final int TYPE_ASCII      = 1;
    /** UTF PIN type constant. */
    private static final int TYPE_UTF        = 2;
    /** HN PIN type constant. */
    private static final int TYPE_HN         = 3;
    /** ISO PIN type constant. */
    private static final int TYPE_ISO        = 4;

    /** CASE_SENSITIVE PIN flag constant. */
    public static final int FLAG_CASE_SENSITIVE            = 0x8000;
    /** LOCAL PIN flag constant. */
    public static final int FLAG_LOCAL                     = 0x4000;
    /** CHANGE_DISABLED PIN flag constant. */
    public static final int FLAG_CHANGE_DISABLED           = 0x2000;
    /** UNBLOCK_DISABLED PIN flag constant. */
    public static final int FLAG_UNBLOCK_DISABLED          = 0x1000;
    /** INITIALIZED PIN flag constant. */
    public static final int FLAG_INITIALIZED               = 0x800;
    /** NEEDS_PADDING PIN flag constant. */
    public static final int FLAG_NEEDS_PADDING             = 0x400;
    /** UNBLOCKING_PIN PIN flag constant. */
    public static final int FLAG_UNBLOCKING_PIN            = 0x200;
    /** SOPIN PIN flag constant. */
    public static final int FLAG_SOPIN                     = 0x100;
    /** DISABLE_ALLOWED PIN flag constant. */
    public static final int FLAG_DISABLE_ALLOWED           = 0x80;
    /** INTEGRITY_PROTECTED PIN flag constant. */
    public static final int FLAG_INTEGRITY_PROTECTED       = 0x40;
    /** CONFIDENTIALITY_PROTECTED PIN flag constant. */
    public static final int FLAG_CONFIDENTIALITY_PROTECTED = 0x20;
    /** EXCHANGEREFDATA PIN flag constant. */
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

    /** This vector contains parsed objects from PIN file. */
    private Vector PIN;
    /** This TLV contains root TLV */
    private TLV root;

    /**
     * Constructs PINAttributes.
     */
    public PINAttributes() {}
    /**
     * Constructs PINAttributes object from the pointed TLV.
     * @param root TLV
     * @throws TLVException if TLV error occurs
     */
    public PINAttributes(TLV root)
                                      throws TLVException {
        if (root.type != TLV.SEQUENCE_TYPE) {      // not a PIN object
            throw new TLVException("Bad PIN record");
        }
        this.root = root;
        readPINs();
    }

    /**
     * Read PIN information.
     * @throws TLVException if parsing error occurs
     */
    private void readPINs()
            throws TLVException {

        TLV t = root.child;        // commonObjectAttributes
        label = t.child.getUTF8().trim();
        t = t.next;         // CommonAuthenticationObjectAttributes
        id = t.child.getId();

        t = t.next;
        if (t.type != ACEntry.CONTEXT_CONSTRUCTED_1) {
            throw new TLVException("Incomplete PIN record");
        }
        t = t.child.child;   // PinAttributes.pinFlags
        byte[] buf = t.getValue();
        int mask = 0;
        for (int i = 0; i < buf[0]; i++) {
            mask = mask | (1 << i);
        }
        mask = ~mask;
        pinFlags = Utils.getShort(buf, 1) & mask;

        t = t.next;
        pinType = t.getEnumerated();

        t = t.next;
        minLength = t.getInteger();

        t = t.next;
        storedLength = t.getInteger();

        t = t.next;
        if (t.type == TLV.INTEGER_TYPE) {
            maxLength = t.getInteger();
            t = t.next;
        } else {
            maxLength = storedLength;
        }

        // this entry is optional, default value is 0
        if (t.type == 0x80) {
            pinReference = t.getInteger();
            t = t.next;
        }

        padChar = t.getId();
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
