/*
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

package com.sun.cdc.io.j2me.apdu;

/**
 * This class represents a slot for APDU connection.
 */
public class Slot {

    /** Size of respBuffer. */
    public static final int respBufferSize = 515; 
    
    /** Slot number. */
    int slot;
    
    /** Is SIM present in this slot?. */
    boolean SIMPresent;
    
    /** 
     * Internal field for native exchangeAPDU() method. 
     * Keeps current position of received data in a response buffer.
     */
    private int received;

    /** 
     * Internal field for native exchangeAPDU() method. Keeps internal locking
     * status.
     */
    private boolean locked;
    
    /**
     * Since at any time once channel always has to be open with the card
     * and that channel is generally the basic channel. If the basic
     * channel is not in use, the APDU manager can select the next
     * application on that channel.
     * This variable shows if basic channel is in use currently or not.
     */
    boolean basicChannelInUse;

    /**
     * This variable shows if the card currently powered or not.
     */
    boolean powered;

    /** Internal buffer for response data. */
    byte[] respBuffer;

    /** APDU for logical channel allocation. */
    byte[] getChannelAPDU;

    /** APDU for closing a logical channel. */
    byte[] closeChannelAPDU;

    /** APDU for the GET RESPONSE command. */
    byte[] getResponseAPDU;

    /** An isAlive command APDU. */
    byte[] isAliveAPDU;

    /** Answer-To-Reset from last reset command. */
    byte[] atr;
    
    /** File Control Information (FCI) from last selection command. */
    byte[] FCI;
    
    /**
     * Unique identifier of card session (period of time between card
     * power up and powerdown).This value is increased after any reset command.
     */
    int cardSessionId;

    /**
     * Creates a new slot with specified parameters.
     * @param slot slot number
     */
    Slot(int slot) {

        this.slot = slot;
        this.cardSessionId = 1;
        this.locked = false;
        this.powered = false;
        this.SIMPresent = false;
        this.basicChannelInUse = false;
        this.respBuffer = new byte[respBufferSize];
        this.atr = null;
        this.FCI = null;
        
        this.getChannelAPDU = new byte[] {0, 0x70, 0, 0, 1};
        this.closeChannelAPDU = new byte[] {0, 0x70, (byte) 0x80, 0};
        this.getResponseAPDU = new byte[] {0, (byte)0xC0, 0, 0, 0};
        this.isAliveAPDU = new byte[] {0, 0x70, (byte) 0x80, 0};

    }
}
