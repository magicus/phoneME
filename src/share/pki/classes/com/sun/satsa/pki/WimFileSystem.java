/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.IOException;
import com.sun.satsa.util.*;

/**
 * This class provides interface to WIM file system.
 */
public class WimFileSystem extends FileSystemAbstract {

    /** Constant defines size of the card buffer */
    private int CARD_BUFFER = 240;
    /** Constant defines parameters P1, P2 in the apdu command */
    private int P1_P2 = 0x0;

    /**
     * Constructs new WimFileSystem object.
     * @param apdu connection to be used by this object.
     */
    public WimFileSystem(Connection apdu) {
        super(apdu);
        apdu.setCLAbyte((byte)0x80);
    }

    /**
     * Selects file by ID.
     * @param id file ID
     * @throws IOException if IOError occurs
     */
    public void select(short id) throws IOException {
        
        int P1P2 = (int) (P1_P2 | Constants.P2);

        byte[] data = apdu.resetCommand().
                           putShort(id).
                           sendCommand(INS_SELECT, P1P2);

        isEFSelected = false;
        currentFileSize = 0;

        if (data.length == 2) {
            return;       // FCI is empty
        }

        TLV t;              // parse FCI
        try {
            t = new TLV(data, 0);
        } catch (TLVException e) {
            throw new IOException();
        }

        t = t.child;
        while (t != null) {
            if (t.type == 0x80) {
                isEFSelected = true;
                currentFileSize = Utils.getU2(data, t.valueOffset);
                return;
            }
            t = t.next;
        }
        return;
    }

    /**
     * Reads part of selected file.
     * @param offset the offset into data buffer where data should be
     * placed
     * @param length data length
     * @param fileOffset file data offset
     * @throws IOException if IO error occurs
     * @return data byte array of the data
     */
    public byte[] readData(int offset, int length, int fileOffset)
            throws IOException {
        byte[] data = new byte[length + offset];
        while (length != 0) {
            int len = length > CARD_BUFFER ? CARD_BUFFER : length;
            byte[] result = apdu.resetCommand().
                           sendCommand(INS_READ, fileOffset, len, true);
           System.arraycopy(result, 0, data, offset, len);
            offset += len;
            fileOffset += len;
            length -= len;
        }
        return data;
    }

    /**
     * Writes data into selected file.
     * @param data buffer that contains data
     * @param offset offset of data in the buffer
     * @param length length of data in the buffer
     * @param fileOffset offset of updated data in the file
     * @throws IOException if I/O error occurs
     */
    public void writeData(byte[] data, int offset, int length, int fileOffset)
            throws IOException {

        while (length != 0) {
            int len = length > CARD_BUFFER ? CARD_BUFFER : length;
            apdu.resetCommand().
                 putBytes(data, offset, len).
                 sendCommand(INS_UPDATE, fileOffset, len, true);
            offset += len;
            fileOffset += len;
            length -= len;
        }
    }

}
