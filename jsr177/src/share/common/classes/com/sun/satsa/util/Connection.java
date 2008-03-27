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

package com.sun.satsa.util;

import com.sun.cdc.io.j2me.apdu.APDUManager;
import com.sun.cdc.io.j2me.apdu.Handle;

import java.io.IOException;

/**
 * Represents APDU connection to the WIM application.
 */
public class Connection {

    /*
        The set of command APDUs used by WIM application is very small.
        Methods of this class that place data into APDU buffer don't
        check for buffer overflow.
    */

    /** The last response APDU status word. */
    public int lastSW;

    /** Temporary buffer for APDU command. */
    private byte[] command;
    /** Current offset in the command buffer. */
    private int offset;
    /** APDU connection handle. */
    private Handle h;
    /** Unit size for the read command */
    private int unitSize;
    /** CLAbyte for the send command */
    private byte CLAbyte;

    /**
     * Constructs new connection object.
     * @param h APDU connection handle.
     */
    public Connection(Handle h) {
        this.h = h;
        command = new byte[255];
    }

    /**
     * Safely closes the connection.
     */
    public void done() {
        try {
            if (h != null) {
                APDUManager.closeConnection(h);
            }
        } catch (IOException e) {} // ignore
        h = null;
    }

    /**
     * Resets command buffer.
     * @return this connection object
     */
    public Connection resetCommand() {
        offset = 5;
        return this;
    }

    /**
     * Places one byte into command buffer.
     * @param data byte to be placed
     * @return this connection object
     */
    public Connection putByte(int data) {
        command[offset++] = (byte) data;
        return this;
    }

    /**
     * Places short value into the buffer.
     * @param data value to be placed
     * @return this connection object
     */
    public Connection putShort(int data) {
        command[offset++] = (byte) (data >> 8);
        command[offset++] = (byte) data;
        return this;
    }

    /**
     * Places the byte sequence into the buffer.
     * @param data data to be placed
     * @param off data offset
     * @param length data length
     * @return this connection object
     */
    public Connection putBytes(byte[] data, int off, int length) {
        System.arraycopy(data, off, command, offset, length);
        offset += length;
        return this;
    }

    /**
     * Sends the command.
     * @param INS INS byte for this command.
     * @param P1P2 APDU parameters value
     * @return response APDU
     * @throws IOException if IO error occurs
     */
    public byte[] sendCommand(int INS, int P1P2) throws IOException {
        return sendCommand(INS, P1P2, 240, true);
    }

    /**
     * Sends the command.
     * @param INS INS byte for this command.
     * @param P1P2 P1 and P2 values for this command
     * @param LE response expected length
     * @param check if true, verify that SW is 0x9000
     * @return response APDU
     * @throws IOException if IO error occurs
     */
    public byte[] sendCommand(int INS, int P1P2, int LE, boolean check)
            throws IOException {

        command[0] = (byte) (CLAbyte | h.channel);
        command[1] = (byte) INS;
        command[2] = (byte) (P1P2 >> 8);
        command[3] = (byte) P1P2;
        command[4] = (byte) (offset - 5);
        putByte(LE);

        byte[] tmp = APDUManager.exchangeAPDU(h, command);

        lastSW = ((tmp[tmp.length - 2] & 0xff) << 8) |
                  (tmp[tmp.length - 1] & 0xff);

        if (check && lastSW != 0x9000) {
            throw new IOException("SW = " + Integer.toHexString(lastSW));
        }
        return tmp;
    }

    /**
     * This method returns the ATR received from the card that this
     * Handle object is used to communicate with.
     * @return ATR information received from the card at startup or
     * reset. In case of I/O troubles returns null.
     */
    public byte[] getATR() {
        return h.getATR();
    }

    /**
     * This method returns the FCI received from the card that this
     * Handle object is used to communicate with.
     * @return FCI information received from the card at startup or
     * reset. In case of I/O troubles returns null.
     */
    public byte[] getFCI() {
        return h.getFCI();
    }

    /**
     * This method sets value for the unitSize variable
     * @param unitSize int Required value.
     */
    public void setUnitSize(int unitSize) {
        this.unitSize = unitSize;
    }

    /**
     * This method sets value for the CLAbyte variable
     * @param cla byte Required value.
     */
    public void setCLAbyte(byte cla) {
        this.CLAbyte = cla;
    }

}
