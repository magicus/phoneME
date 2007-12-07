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
 * This class represents a handle for APDU connection.
 */
public class Handle {

    /** Slot number. */
    int slot;
    /** Logical channel number. */
    public int channel;
    /** Slot object. */
    Slot cardSlot;
    /** Is this connection opened? */
    boolean opened;
    /** FCI received from selected application. */
    byte[] FCI;
    
    /**
     * Unique identifier of card session (period of time between card
     * power up and powerdown) during which the connection was created.
     */
    public int cardSessionId;

    /**
     * Unique identifier of Handle instance.
     * It is used for debugging purposes.
     */
    int handleInstance;

    /**
     * Counter for handleInstance generation.
     * It is used for debugging purposes.
     */
    private static int instanceNo = 1;
    
    /**
     * Creates a new handle with specified parameters.
     * @param slot slot number
     * @param channel channel number
     */
    Handle(int slot, int channel) {

        this.slot = slot;
        this.channel = channel;
        this.cardSlot = APDUManager.slots[slot];
        this.FCI = cardSlot.FCI;
        this.opened = true;
        this.cardSessionId = cardSlot.cardSessionId;
        this.handleInstance = instanceNo++;
    }

    /**
     * Returns ATR of the card.
     * @return ATR.
     */
    public byte[] getATR() {
        return APDUManager.getATR(slot);
    }

    /**
     * Returns FCI of the selected application.
     * @return FCI.
     */
    public byte[] getFCI() {
        if (FCI != null && opened && cardSlot.powered && 
                cardSessionId == cardSlot.cardSessionId) {
            int len = FCI.length;
            byte[] result = new byte[len];
            System.arraycopy(FCI, 0, result, 0, len);
            return result;
        }
        return null;
    }
    
    /**
     * Returns current cardSessionId of the slot.
     * @return cardSessionId.
     */
    public int getCardSessionId() {
        return cardSlot.cardSessionId;
    }

}
