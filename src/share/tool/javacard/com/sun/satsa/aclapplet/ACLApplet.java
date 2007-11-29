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

package com.sun.satsa.aclapplet;

import javacard.framework.*;
import javacard.security.*;
import javacardx.crypto.Cipher;

/**
 * Card side application for ACL implementation. 
 */
public class ACLApplet extends Applet {

    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x0 = 0;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x1 = 1;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x2 = 2;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x3 = 3;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x4 = 4;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x5 = 5;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x6 = 6;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x8 = 8;

    /** INS byte for command APDU. */
    static final byte INS_SELECT    = (byte) 0xa4;
    /** INS byte for command APDU. */
    static final byte INS_READ      = (byte) 0xb0;
    /** INS byte for command APDU. */


    /** Root DF for entire file structure. */
    DFile top;
    /**
     * Root DF for WIM application. All relative paths start from
     * here. */
    DFile base;
    /** Currently selected file. */
    File current;
    /** Constructor. */
    ACLApplet() {
        register();
    }

    /**
     * To create an instance of the Applet subclass, the JCRE will call
     * this static method first.
     * @param bArray the array containing installation parameters
     * @param bOffset the starting offset in bArray
     * @param bLength the length in bytes of the parameter data in bArray
     */
    public static void install(byte[] bArray, short bOffset,
                               byte bLength) {
        new ACLApplet();
    }

    /**
     * Called by the JCRE to inform this applet that it has been
     * selected. When invoked first time initialises the file system.
     * @return true
     */
    public boolean select() {

        if (top == null) {
            init();
        }
        current = base;
        return true;
    }

    /**
     * Initialises the WIM data structures.
     */
    void init() {

        Parser.init(Data.Files);
        top = (DFile) readFile(null);

        if (base == null) {
            ISOException.throwIt((short) 0x9001);
        }

    }

    /**
     * Creates new file object.
     * @param parent parent DF for this file
     * @return the new file object
     */
    File readFile(DFile parent) {

        short id = Parser.getShort();
        short type = Parser.getByte();
        short length = Parser.getShort();

        if ((type & File.DIR) == 0) {

            EFile f;
            if ((type & File.EMPTY) == 0) {
                f = new EFile(parent, id, type, Parser.offset, length,
                              Data.Files);
                Parser.skip(length);
            } else {
                type &= ~File.EMPTY;
                byte[] data = new byte[length];
                short dlen = Parser.getShort();
                Util.arrayCopyNonAtomic(Data.Files, Parser.offset, data,
                        (short) 0, dlen);
                f = new EFile(parent, id, type, (short) 0, length, data);
                Parser.skip(dlen);
            }
            return f;
        }

        DFile f = new DFile(parent, id, type);

        File[] files = new File[length];
        for (short i = 0; i < length; i++) {
            files[i] = readFile(f);
        }

        f.files = files;

        if (type == File.WIM) {
            base = f;
        }

        return f;
    }

    /**
     * Main entry point.
     * @param apdu command APDU
     */
    public void process(APDU apdu) {

        byte[] data = apdu.getBuffer();
        byte CLA = (byte) (data[ISO7816.OFFSET_CLA] & 0xF0);
        byte INS = data[ISO7816.OFFSET_INS];

        if (CLA == 0 &&
            INS == (byte)(0xA4) &&
            data[ISO7816.OFFSET_P1] == 4) {
            return;
        }

        if (CLA != (byte) 0x00) {
            ISOException.throwIt(ISO7816.SW_CLA_NOT_SUPPORTED);
        }

        switch (INS) {

            case INS_SELECT:
                selectFile(apdu);
                return;

            case INS_READ:
                read(apdu);
                return;

        }
        ISOException.throwIt(ISO7816.SW_INS_NOT_SUPPORTED);
    }

    /**
     * Handles SELECT FILE APDU.
     * @param apdu command APDU
     */
    void selectFile(APDU apdu) {

        byte[] data = apdu.getBuffer();

/*        if (Util.getShort(data, x2) != 0) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }
*/
        checkDataSize(x2, apdu);

        File f = select(Util.getShort(data, x5));

        if (f == null) {
            ISOException.throwIt(ISO7816.SW_FILE_NOT_FOUND);
        }

        current = f;

        if (current.isDF()) {

            Util.setShort(data, x0, (short) 0x6f00);
            apdu.setOutgoingAndSend(x0, x2);
        } else {

            Util.setShort(data, x0, (short) 0x6f04);
            Util.setShort(data, x2, (short) 0x8002);
            Util.setShort(data, x4, ((EFile) current).length);
            apdu.setOutgoingAndSend(x0, x6);
        }
    }

    /**
     * Selects the file specified by file identifier.
     * @param id file identifier
     * @return selected file
     */
    File select(short id) {

        DFile f;
        if (current.isDF()) {
            f = (DFile) current;
        } else  {
            f = current.parent;
        }

        File x = f.getFile(id);
        if (x != null) {
            return x;
        }

        f = f.parent;

        if (f == null) {
            return null;
        }

        return f.getFile(id);
    }

    /**
     * Handles READ BINARY APDU.
     * @param apdu command APDU
     */
    void read(APDU apdu) {

        if (current.isDF()) {
            ISOException.throwIt(ISO7816.SW_COMMAND_NOT_ALLOWED);
        }

        EFile f = (EFile) current;

        byte[] data = apdu.getBuffer();

        short offset = Util.getShort(data, x2);
        if (offset < 0 || offset > f.length) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        short len = (short) (data[x4] & 0xff);
        if ((short)(offset + len) > f.length) {
            //ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
            len = (short)(f.length - offset);
            if (len < 0) {
                ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
            }
        }

        apdu.setOutgoing();
        apdu.setOutgoingLength(len);
        apdu.sendBytesLong(f.data, (short) (f.offset + offset), len);
    }

    /**
     * Returns the size of DER object for given value size.
     * @param i the value size
     * @return the size of DER object
     */
    static short getDERSize(short i) {

        if (i < 128) {
            return (short) (i + 2);
        }
        return (short) (i + 3);
    }

    /**
     * Places encoded length of DER object into the buffer.
     * @param data the buffer
     * @param offset offset in the buffer where the length must be
     * placed
     * @param length the length to be placed
     * @return the new offset
     */
    static short putLength(byte[] data, short offset, short length) {
        if (length >= 128) {
            data[offset++] = (byte) 0x81;
        }
        data[offset++] = (byte) length;
        return offset;
    }

    /**
     * Returns file object specified by path in the buffer.
     * @param data the buffer
     * @param index path offset
     * @param l path length
     * @return file object or null if not found
     */
    File getFile(byte[] data, short index, short l) {

        // path must contain even number of bytes
        if (l < 2 || l % 2 != 0) {
            return null;
        }
        l = (short) (l / 2);

        short id = Util.getShort(data, index);

        File x;

        if (l == 1) {
            x = base;
        } else {

            if (id == top.id) {
                x = top;
            } else {
                if (id == (short) 0x3fff) {
                    x = base;
                } else {
                    return null;
                }
            }
            index += 2;
            l--;
        }

        while (l != 0) {

            if (! x.isDF()) {
                return null;
            }
            File f = ((DFile) x).getFile(Util.getShort(data, index));
            if (f == null || f.parent != x) {
                return null;
            }
            x = f;
            index += 2;
            l--;
        }

        return x;
    }


    /**
     * Verifies that APDU contains correct number of data bytes.
     * @param expectedSize expected data size
     * @param apdu APDU object
     */
    private void checkDataSize(short expectedSize, APDU apdu) {
        if (expectedSize != apdu.setIncomingAndReceive()) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }
    }

}

