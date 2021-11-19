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

import com.sun.satsa.util.Utils;

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
    public static final int FLAG_CASE_SENSITIVE    = 1;
    /** PIN flag constant. */
    public static final int FLAG_CHANGE_DISABLED   = 2;
    /** PIN flag constant. */
    public static final int FLAG_UNBLOCK_DISABLED  = 4;
    /** PIN flag constant. */
    public static final int FLAG_NEEDS_PADDING     = 8;
    /** PIN flag constant. */
    public static final int FLAG_DISABLE_ALLOWED   = 16;
    /** PIN flag constant. */
    public static final int FLAG_UNBLOCKING_PIN    = 32;

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
