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

package com.sun.satsa.acl;

import java.io.IOException;
import java.util.Vector;
import com.sun.satsa.util.*;
import com.sun.satsa.util.pkcs15.*;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * This class provides interface to card file system.
 */
public class AclFileSystem extends FileSystemAbstract {

    /** Constant defines size of the card buffer */
    private static final int  CARD_BUFFER = 64;
    
    /** Internal buffer for ATR */
    private byte[] ATR;
    /** variable defines the unit size in the INS_READ command */
    private int unitSize = 1;
    /** variable defines the record size for linear file. -1 means transparent file */
    private int recordSize = -1;
    /** DIR file object */
    public DIRF DIR;
    /**
     * Constructs new AclFileSystem object.
     * @param apdu connection to be used by this object.
     */
    public AclFileSystem(Connection apdu) throws IOException {
        super(apdu);
        ATR = apdu.getATR();
        /* Get FCI from previous operation */
        byte[] prevFCI = apdu.getFCI();
        /*
         * It is assumed that in case of work with application the FCI
         * contains successfull result of the selectApplication operation,
         * i.e. 0x9000. In case of work with the card file system the FCI
         * array will contain actual FCI of the master file and length of the
         * array will be > 2
         */
        if (prevFCI.length > 2) {
            parseATR();
            apdu.setUnitSize(unitSize);
            try {
                DIR = new DIRF(this);
                DIR.load();
            } catch (TLVException te) {
                throw new IOException("Wrong file TLV structure");
            }
            catch (IOException ie) {
                throw new IOException("Wrong DIR file");
            }
        } else {
            apdu.setUnitSize(unitSize);
        }
        apdu.setCLAbyte((byte)0);
    }

    /**
     * Parses the ATR.
     * At the moment defines a unit size only
     */
    private void parseATR() {
        for (int i = ATR.length - 1; i > 0; i--) {
            if (((ATR[i] & 0xF0) == 0x70) &&
                ((ATR[i] & 0x0F) >= 2)) {
                unitSize = (1 << (ATR[i+2] & 0x07)) / 2;
                break;
            }
        }
    }

    /**
     * Selects file by ID.
     * @param id file ID
     * @throws IOException if IOError occurs
     */
    public void select(short id) throws IOException {

        int P1P2 = (int) (Constants.P1 << 8 | Constants.P2);
        
        byte[] data = apdu.resetCommand().
                           putShort(id).
                           sendCommand(INS_SELECT, P1P2);

        isEFSelected = false;
        currentFileSize = 0;
        recordSize = -1;

        if (data.length == 2) {
            return;       // FCI is empty
        }

        /* parse FCI */
        if (Utils.getU2(data, data.length - 2) != 0x9000) {
            throw new IOException("Bad FCI");
        }

        if (data[0] == 0x62) { /* FCP were received */
            TLV fci;
            try {
                fci = new TLV(data, 0);
            } catch (TLVException tlvException) {
                throw new IOException("Bad FCI structure: " + tlvException);
            }

            if (fci.child == null) {
                throw new IOException("Bad FCI structure");
            }

            for (TLV t = fci.child; t != null; t = t.next) {
                if (t.type == 0x82) { // File descriptor
                    if (t.length >= 4) {
                        byte[] descriptor = t.getValue();
                        /* 0x2 and 0x3 are allowed values for 'Linear fixed' 
                           EF structure field */
                        /* IMPL_NOTE: Was not tested when EF structure field
                                      equals 0x3 */
                        boolean isLinear = ((descriptor[0] & 0x7) == 0x2) || 
                                           ((descriptor[0] & 0x7) == 0x3);
                        if (isLinear) {
                            recordSize = Utils.getShort(descriptor, 2);
                        }
                    }
                    break;
                }
            }
        }

        isEFSelected = true;
    }

    /**
     * Reads the current EF.
     * @return array that contains EF body.
     * @throws IOException if IO error occurs
     */
    public byte[] readFile() throws IOException {
        if (recordSize < 0) {        
            return readData(0, currentFileSize, 0);
        } else {
            return readLinearFile();
        }
    }

    private byte[] readLinearFile() throws IOException {
        Vector v = new Vector();
        int readLength = 0;
        int recNo = 1;
        while (true) {
            int P1P2 = (recNo << 8) | 0x04; // select by record number
            byte[] result = apdu.resetCommand().
                    sendCommand(INS_READ_REC, P1P2, 0, false);
            if (Utils.getU2(result, result.length - 2) != 0x9000) {
                break;
            }
            
            v.addElement(result);
            readLength += result.length - 2;
            recNo++;
        }
        
        if (readLength == 0) {
            return new byte[0];
        }
        
        byte[] data = new byte[readLength];
        int offset = 0;
        for (int i = 0; i < v.size(); i++) {
            byte[] r = (byte[])v.elementAt(i);
            System.arraycopy(r, 0, data, offset, r.length - 2);
            offset += r.length - 2;
        }
        currentFileSize = readLength;
        return data;
    }

    /**
     * Reads part of selected file.
     * @param offset the offset into data buffer where data should be
     * placed
     * @param length read data length (is not used in this class)
     * @param fileOffset file data offset
     * @throws IOException if IO error occurs
     * @return data byte array of the data
     */
    public byte[] readData(int offset, int length /* NOT USED */,
                           int fileOffset) throws IOException {
        Vector v = new Vector();
        int len = CARD_BUFFER;
        int readLength = 0;
        while (true) {
            byte[] result = apdu.resetCommand().
                    sendCommand(INS_READ, fileOffset/unitSize, len, false);
            int lastSW = ((result[result.length - 2] & 0xff) << 8) |
                     (result[result.length - 1] & 0xff);
            if (lastSW != 0x9000) {
               break;
            }
            v.addElement(result);
            offset += len;
            fileOffset += len;
            readLength += result.length - 2;
            if (result.length - 2 < len) {
                break;
            }
        }
        byte[] data = new byte[readLength];
        offset = 0;
        for (int i = 0; i < v.size(); i++) {
            byte[] r = (byte[])v.elementAt(i);
            System.arraycopy(r, 0, data, offset, r.length - 2);
            offset += r.length - 2;
        }
        currentFileSize = readLength;
        return data;
    }
}
