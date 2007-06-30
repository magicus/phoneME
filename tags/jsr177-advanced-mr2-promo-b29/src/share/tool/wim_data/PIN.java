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

/**
 * This class represents PIN.
 */
class PIN {

    /** Stored length for these PINs. */
    static final int STORED_LENGTH = 8;

    /** PIN label. */
    String label;

    /** PIN ID. */
    int id;

    /** PIN reference. */
    int ref;

    /** Path. */
    short[] path;

    /** PIN value. */
    String value;

    /**
     * Constructor.
     * @param label PIN label
     * @param id PIN ID
     * @param ref PIN reference
     * @param path path
     * @param value PIN value
     */
    PIN(String label, int id, int ref, short[] path, String value) {

        this.label = label;
        this.id = id;
        this.ref = ref;
        this.path = path;
        this.value = value;
    }

    /**
     * Generates TLV structure that represents this PIN.
     * @return TLV structure
     */
    TLV toTLV() {

        TLV t, c, v;

        t = TLV.createSequence();

        t.setChild(TLV.createSequence()).
                setChild(Utils.createLabel(label)).
                setNext(new TLV(TLV.BITSTRING_TYPE,
                        Utils.shortToBytes(0x0780)));

        c = t.child;

        c.setNext(TLV.createSequence()).
                setChild(TLV.createOctetString(new byte[] {(byte) id}));

        c.next.setNext(new TLV(0xa1)).
                setChild(TLV.createSequence()).
                setChild(new TLV(TLV.BITSTRING_TYPE,
                        Utils.shortToBytes(0x022c))).
                setNext(new TLV(TLV.ENUMERATED_TYPE, new byte[] {1})).
                setNext(TLV.createInteger(4)).
                setNext(TLV.createInteger(STORED_LENGTH)).
                setNext(TLV.createInteger(ref).setTag(0x80)).
                setNext(TLV.createOctetString(new byte[] {(byte) 0xff})).
                setNext(Utils.createPath(path));

        return t;
    }

    /**
     * Returns data for this PIN.
     * @return PIN data
     */
    public byte[] getData() {

        byte[] data = new byte[STORED_LENGTH];
        for (int i = 0; i < data.length; i++) {
            data[i] = -1;
        }

        try {
            byte[] t = value.getBytes("UTF-8");
            for (int i = 0; i < t.length; i++) {
                data[i] = t[i];
            }
            return data;
        } catch (UnsupportedEncodingException e) {
            return null;
        }
    }
}
